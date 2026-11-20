#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

ReChopProcessor::ReChopProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    ons.reserve (512);
    active.reserve (128);
}

AudioProcessorValueTreeState::ParameterLayout ReChopProcessor::createLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "root", 1 }, "Root",
                 StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 9));

    StringArray scales;
    for (int i = 0; i < 5; ++i) scales.add (rechop::scaleName (i));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "scale", 1 }, "Scale", scales, 1));

    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "filter", 1 }, "Family",
                 StringArray { "All", "Dance", "Pop", "Synthwave" }, 0));

    p.push_back (std::make_unique<AudioParameterInt> (ParameterID { "pattern", 1 }, "Pattern",
                 0, (int) rechop::library().size() - 1, 0));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "density", 1 }, "Density",
                 NormalisableRange<float> (0.0f, 1.0f), 1.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "swing", 1 }, "Swing",
                 NormalisableRange<float> (0.0f, 1.0f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "humanize", 1 }, "Humanize",
                 NormalisableRange<float> (0.0f, 1.0f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "gate", 1 }, "Gate",
                 NormalisableRange<float> (0.1f, 1.5f), 1.0f));

    p.push_back (std::make_unique<AudioParameterInt> (ParameterID { "octave", 1 }, "Octave", -2, 2, 0));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "fill", 1 }, "Fill", false));
    p.push_back (std::make_unique<AudioParameterInt> (ParameterID { "sliceBase", 1 }, "Slice Base Note", 0, 108, 36));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "exportBars", 1 }, "Export Bars",
                 StringArray { "2", "4", "8" }, 1));

    return { p.begin(), p.end() };
}

void ReChopProcessor::prepareToPlay (double, int)
{
    active.clear();
    wasPlaying = false;
}

bool ReChopProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == AudioChannelSet::stereo()
        || l.getMainOutputChannelSet() == AudioChannelSet::mono();
}

// ------------------------------------------------------------------ settings
rechop::Settings ReChopProcessor::getSettings() const
{
    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    rechop::Settings s;
    s.root      = (int) get ("root");
    s.scale     = (int) get ("scale");
    s.octave    = (int) get ("octave");
    s.sliceBase = (int) get ("sliceBase");
    s.density   = get ("density");
    s.swing     = get ("swing");
    s.humanize  = get ("humanize");
    s.gate      = get ("gate");
    s.fill      = get ("fill") > 0.5f;
    return s;
}

int ReChopProcessor::getPatternId() const
{
    const int n = (int) rechop::library().size();
    return jlimit (0, n - 1, (int) apvts.getRawParameterValue ("pattern")->load());
}

int ReChopProcessor::getFilter() const { return (int) apvts.getRawParameterValue ("filter")->load(); }

const rechop::Pattern& ReChopProcessor::getPattern() const { return rechop::library()[(size_t) getPatternId()]; }

// ------------------------------------------------------------------ audio
void ReChopProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    buffer.clear();
    midi.clear();   // the incoming keyboard is not passed on; ReChop is the only MIDI source

    const int n = buffer.getNumSamples();

    Optional<AudioPlayHead::PositionInfo> pos;
    if (auto* ph = getPlayHead()) pos = ph->getPosition();

    bool ok = false;
    double bpm = 120.0, ppq = 0.0;
    if (pos.hasValue())
    {
        const auto& pi = *pos;
        if (pi.getIsPlaying() && pi.getPpqPosition().hasValue() && pi.getBpm().hasValue())
        {
            bpm = *pi.getBpm();
            ppq = *pi.getPpqPosition();
            ok = bpm > 1.0;
        }
    }

    if (! ok)
    {
        for (auto& a : active) midi.addEvent (MidiMessage::noteOff (1, a.note), 0);
        active.clear();
        wasPlaying = false;
        return;
    }

    lastBpm.store (bpm);

    const double stepsPerSample = (bpm / 60.0 * 4.0) / getSampleRate();
    const double b0 = ppq * 4.0;
    const double b1 = b0 + n * stepsPerSample;

    if (! wasPlaying || std::abs (b0 - lastEnd) > 0.5)   // start or playhead jump: release everything
    {
        for (auto& a : active) midi.addEvent (MidiMessage::noteOff (1, a.note), 0);
        active.clear();
    }

    auto offset = [&] (double step) { return jlimit (0, juce::jmax (0, n - 1), (int) ((step - b0) / stepsPerSample)); };

    const auto settings = getSettings();
    const int id = getPatternId();
    const auto& pat = rechop::library()[(size_t) id];

    ons.clear();
    for (int64_t st = (int64_t) std::floor (b0) - 1; st <= (int64_t) std::ceil (b1); ++st)
    {
        rechop::Ev e;
        if (rechop::stepEvent (pat, id, settings, st, e) && e.start >= b0 && e.start < b1)
            ons.push_back ({ e.note, e.vel, e.start, e.start + e.len });
    }
    std::sort (ons.begin(), ons.end(), [] (const Pending& a, const Pending& b) { return a.start < b.start; });

    // notes still ringing from earlier blocks (cut early if the same note is re-triggered)
    for (auto it = active.begin(); it != active.end();)
    {
        double end = it->end;
        for (const auto& o : ons)
            if (o.note == it->note && o.start < end) { end = o.start; break; }

        if (end < b1) { midi.addEvent (MidiMessage::noteOff (1, it->note), offset (end)); it = active.erase (it); }
        else ++it;
    }

    for (size_t i = 0; i < ons.size(); ++i)
    {
        const auto& o = ons[i];
        double end = o.end;
        for (size_t j = i + 1; j < ons.size(); ++j)
            if (ons[j].note == o.note && ons[j].start < end) { end = ons[j].start; break; }

        midi.addEvent (MidiMessage::noteOn (1, o.note, (uint8) o.vel), offset (o.start));
        if (end < b1) midi.addEvent (MidiMessage::noteOff (1, o.note), offset (end));
        else          active.push_back ({ o.note, end });
    }

    lastEnd = b1;
    wasPlaying = true;
}

// ------------------------------------------------------------------ MIDI export (drag into Cubase)
File ReChopProcessor::exportMidiFile (int bars) const
{
    const auto settings = getSettings();
    const int id = getPatternId();
    const auto& pat = rechop::library()[(size_t) id];

    std::vector<rechop::Ev> evs;
    const int64_t total = (int64_t) bars * 16;
    for (int64_t st = 0; st < total; ++st)
    {
        rechop::Ev e;
        if (rechop::stepEvent (pat, id, settings, st, e)) evs.push_back (e);
    }
    std::sort (evs.begin(), evs.end(), [] (const rechop::Ev& a, const rechop::Ev& b) { return a.start < b.start; });

    const int tpq = 960;
    const double ticksPerStep = tpq / 4.0;

    MidiMessageSequence notes;
    for (size_t i = 0; i < evs.size(); ++i)
    {
        double end = evs[i].start + evs[i].len;
        for (size_t j = i + 1; j < evs.size(); ++j)
            if (evs[j].note == evs[i].note && evs[j].start < end) { end = evs[j].start; break; }

        notes.addEvent (MidiMessage::noteOn (1, evs[i].note, (uint8) evs[i].vel), evs[i].start * ticksPerStep);
        notes.addEvent (MidiMessage::noteOff (1, evs[i].note), end * ticksPerStep);
    }
    notes.addEvent (MidiMessage::endOfTrack(), (double) total * ticksPerStep);
    notes.sort();

    MidiMessageSequence tempo;
    tempo.addEvent (MidiMessage::timeSignatureMetaEvent (4, 4), 0);
    tempo.addEvent (MidiMessage::tempoMetaEvent ((int) (60000000.0 / jmax (30.0, lastBpm.load()))), 0);

    MidiFile mf;
    mf.setTicksPerQuarterNote (tpq);
    mf.addTrack (tempo);
    mf.addTrack (notes);

    auto dir = File::getSpecialLocation (File::tempDirectory).getChildFile ("ReChop");
    dir.createDirectory();
    auto file = dir.getChildFile (File::createLegalFileName ("ReChop " + String (pat.name) + " " + String (bars) + "bars") + ".mid");

    FileOutputStream os (file);
    if (os.openedOk())
    {
        os.setPosition (0);
        os.truncate();
        mf.writeTo (os);
    }
    return file;
}

// ------------------------------------------------------------------ state / editor
void ReChopProcessor::getStateInformation (MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}

void ReChopProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (ValueTree::fromXml (*xml));
}

AudioProcessorEditor* ReChopProcessor::createEditor() { return new ReChopEditor (*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ReChopProcessor(); }
