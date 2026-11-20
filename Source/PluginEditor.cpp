#include "PluginEditor.h"

using namespace juce;

namespace
{
    const Colour bg      (0xff121212);
    const Colour panel   (0xff181818);
    const Colour outline (0xff2e2e2e);
    const Colour textHi  (0xffd2d2d2);
    const Colour textLo  (0xff7d7d7d);

    String noteName (int n)
    {
        static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        return String (names[n % 12]) + String (n / 12 - 2);   // Cubase naming: C3 = 60
    }
}

// ================================================================ look
ReChopLook::ReChopLook()
{
    setColour (ResizableWindow::backgroundColourId, bg);
    setColour (ComboBox::backgroundColourId, panel);
    setColour (ComboBox::outlineColourId, outline);
    setColour (ComboBox::textColourId, textHi);
    setColour (ComboBox::arrowColourId, textLo);
    setColour (PopupMenu::backgroundColourId, Colour (0xff1b1b1b));
    setColour (PopupMenu::textColourId, textHi);
    setColour (PopupMenu::highlightedBackgroundColourId, orange().withAlpha (0.25f));
    setColour (PopupMenu::highlightedTextColourId, Colours::white);
    setColour (Label::textColourId, textHi);
    setColour (Slider::textBoxTextColourId, textLo);
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (Slider::thumbColourId, orange());
    setColour (Slider::trackColourId, orange().withAlpha (0.8f));
    setColour (Slider::backgroundColourId, outline);
    setColour (ListBox::backgroundColourId, panel);
    setColour (ListBox::outlineColourId, outline);
    setColour (ScrollBar::thumbColourId, Colour (0xff3a3a3a));
}

void ReChopLook::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                   float startAngle, float endAngle, Slider&)
{
    auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (8.0f);
    const float rad = jmin (b.getWidth(), b.getHeight()) / 2.0f;
    const auto c = b.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);

    g.setColour (Colour (0xff1d1d1d));
    g.fillEllipse (c.x - rad, c.y - rad, rad * 2, rad * 2);
    g.setColour (outline);
    g.drawEllipse (c.x - rad, c.y - rad, rad * 2, rad * 2, 1.0f);

    Path track;
    track.addCentredArc (c.x, c.y, rad - 5, rad - 5, 0.0f, startAngle, endAngle, true);
    g.setColour (Colour (0xff303030));
    g.strokePath (track, PathStrokeType (2.0f));

    Path val;
    val.addCentredArc (c.x, c.y, rad - 5, rad - 5, 0.0f, startAngle, angle, true);
    g.setColour (orange().withAlpha (0.16f));
    g.strokePath (val, PathStrokeType (7.0f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (orange());
    g.strokePath (val, PathStrokeType (2.2f, PathStrokeType::curved, PathStrokeType::rounded));

    Path ptr;
    ptr.addRoundedRectangle (-1.1f, -(rad - 11.0f), 2.2f, rad * 0.32f, 1.0f);
    g.setColour (textHi);
    g.fillPath (ptr, AffineTransform::rotation (angle).translated (c.x, c.y));
}

void ReChopLook::drawToggleButton (Graphics& g, ToggleButton& t, bool highlighted, bool)
{
    auto r = t.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = t.getToggleState();
    g.setColour (on ? orange().withAlpha (0.14f) : panel);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (on ? orange().withAlpha (0.8f) : (highlighted ? Colour (0xff4a4a4a) : outline));
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
    g.setColour (on ? Colours::white : textLo);
    g.setFont (Font (FontOptions (13.0f)));
    g.drawText (t.getButtonText(), t.getLocalBounds(), Justification::centred);
}

// ================================================================ pattern preview
void PatternView::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (Colour (0xff151515));
    g.fillRoundedRectangle (b, 6.0f);
    g.setColour (outline);
    g.drawRoundedRectangle (b.reduced (0.5f), 6.0f, 1.0f);

    const auto& pat = proc.getPattern();
    const int id = proc.getPatternId();
    const auto s = proc.getSettings();
    const int L = (int) pat.steps.size();

    auto area = b.reduced (12.0f, 22.0f);

    for (int i = 0; i <= L; ++i)
    {
        const float x = area.getX() + area.getWidth() * (float) i / (float) L;
        const float a = (i % 16 == 0) ? 0.30f : (i % 4 == 0 ? 0.14f : 0.05f);
        g.setColour (Colours::white.withAlpha (a));
        g.drawVerticalLine ((int) x, area.getY(), area.getBottom());
    }

    g.setColour (textLo);
    g.setFont (Font (FontOptions (12.0f)));
    String info = String (pat.name) + "   " + (pat.kind == rechop::Kind::Melodic ? "melodic" : "slices from " + noteName (s.sliceBase))
                  + (L > 16 ? "   2 bars" : "   1 bar");
    g.drawText (info, getLocalBounds().removeFromTop (22).reduced (12, 0), Justification::centredLeft);

    std::vector<rechop::Ev> evs;
    int lo = 127, hi = 0;
    for (int st = 0; st < L; ++st)
    {
        rechop::Ev e;
        if (rechop::stepEvent (pat, id, s, st, e)) { evs.push_back (e); lo = jmin (lo, e.note); hi = jmax (hi, e.note); }
    }

    if (evs.empty())
    {
        g.drawText ("no notes (density too low)", getLocalBounds(), Justification::centred);
        return;
    }

    const int span = jmax (5, hi - lo + 1);
    const float rowH = jmin (area.getHeight() / (float) span, 18.0f);
    const float yBase = area.getCentreY() + (float) (hi - lo + 1) * rowH * 0.5f;

    for (const auto& e : evs)
    {
        const float x = area.getX() + area.getWidth() * (float) (e.start / L);
        const float w = jmin (area.getRight() - x, area.getWidth() * (float) (e.len / L));
        const float y = yBase - (float) (e.note - lo + 1) * rowH;
        Rectangle<float> r (x, y + 1.0f, jmax (w - 1.0f, 2.0f), rowH - 2.0f);

        const float a = 0.45f + 0.55f * (float) e.vel / 127.0f;
        g.setColour (ReChopLook::orange().withAlpha (0.12f));
        g.fillRoundedRectangle (r.expanded (2.0f), 3.0f);
        g.setColour (ReChopLook::orange().withAlpha (a));
        g.fillRoundedRectangle (r, 2.0f);
    }
}

// ================================================================ drag button
void DragMidiButton::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (hover ? ReChopLook::orange().withAlpha (0.14f) : panel);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (ReChopLook::orange().withAlpha (hover ? 0.9f : 0.55f));
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
    g.setColour (hover ? Colours::white : textHi);
    g.setFont (Font (FontOptions (13.0f)));
    g.drawText ("DRAG MIDI  >  CUBASE", getLocalBounds(), Justification::centred);
}

void DragMidiButton::mouseDrag (const MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 6) return;
    dragging = true;

    const int idx = (int) proc.apvts.getRawParameterValue ("exportBars")->load();
    const int bars = 2 << jlimit (0, 2, idx);
    auto file = proc.exportMidiFile (bars);

    if (file.existsAsFile())
        DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() }, false, this,
                                                              [this] { dragging = false; });
    else
        dragging = false;
}

// ================================================================ editor
ReChopEditor::ReChopEditor (ReChopProcessor& p)
    : AudioProcessorEditor (&p), proc (p), patternView (p), dragButton (p)
{
    setLookAndFeel (&look);

    auto choices = [&] (const char* id) { return static_cast<AudioParameterChoice*> (proc.apvts.getParameter (id))->choices; };

    title.setText ("ReChop", dontSendNotification);
    title.setFont (Font (FontOptions (26.0f, Font::bold)));
    title.setColour (Label::textColourId, textHi);
    addAndMakeVisible (title);

    version.setText (String ("vocal chop patterns   v") + JucePlugin_VersionString, dontSendNotification);
    version.setFont (Font (FontOptions (12.0f)));
    version.setColour (Label::textColourId, textLo);
    version.setJustificationType (Justification::centredRight);
    addAndMakeVisible (version);

    credit.setText ("BY POURIA MOTABEAN", dontSendNotification);
    credit.setFont (Font (FontOptions (11.0f)));
    credit.setColour (Label::textColourId, Colour (0xff5a5a5a));
    addAndMakeVisible (credit);

    familyBox.addItemList (choices ("filter"), 1);
    rootBox.addItemList   (choices ("root"), 1);
    scaleBox.addItemList  (choices ("scale"), 1);
    barsBox.addItemList   (choices ("exportBars"), 1);
    for (auto* c : { &familyBox, &rootBox, &scaleBox, &barsBox }) addAndMakeVisible (c);

    familyAtt = std::make_unique<CA> (proc.apvts, "filter", familyBox);
    rootAtt   = std::make_unique<CA> (proc.apvts, "root", rootBox);
    scaleAtt  = std::make_unique<CA> (proc.apvts, "scale", scaleBox);
    barsAtt   = std::make_unique<CA> (proc.apvts, "exportBars", barsBox);
    familyBox.onChange = [this] { rebuildList(); };

    list.setModel (this);
    list.setRowHeight (30);
    list.setOutlineThickness (1);
    addAndMakeVisible (list);

    addAndMakeVisible (patternView);
    addAndMakeVisible (dragButton);

    for (auto* s : { &octaveSlider, &sliceSlider })
    {
        s->setSliderStyle (Slider::LinearHorizontal);
        s->setTextBoxStyle (Slider::TextBoxRight, false, 44, 22);
        addAndMakeVisible (s);
    }
    sliceSlider.textFromValueFunction = [] (double v) { return noteName ((int) v); };
    sliceSlider.setTextBoxIsEditable (false);

    for (auto* k : { &densityKnob, &swingKnob, &humanizeKnob, &gateKnob })
    {
        k->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        k->setTextBoxStyle (Slider::TextBoxBelow, false, 60, 18);
        k->setRotaryParameters (MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, true);
        k->setDoubleClickReturnValue (true, k == &densityKnob ? 1.0 : (k == &gateKnob ? 1.0 : 0.0));
        addAndMakeVisible (k);
    }
    densityKnob.textFromValueFunction  = [] (double v) { return String (roundToInt (v * 100)) + "%"; };
    swingKnob.textFromValueFunction    = [] (double v) { return String (roundToInt (v * 100)) + "%"; };
    humanizeKnob.textFromValueFunction = [] (double v) { return String (roundToInt (v * 100)) + "%"; };
    gateKnob.textFromValueFunction     = [] (double v) { return String (v, 2) + "x"; };

    octaveAtt   = std::make_unique<SA> (proc.apvts, "octave", octaveSlider);
    sliceAtt    = std::make_unique<SA> (proc.apvts, "sliceBase", sliceSlider);
    densityAtt  = std::make_unique<SA> (proc.apvts, "density", densityKnob);
    swingAtt    = std::make_unique<SA> (proc.apvts, "swing", swingKnob);
    humanizeAtt = std::make_unique<SA> (proc.apvts, "humanize", humanizeKnob);
    gateAtt     = std::make_unique<SA> (proc.apvts, "gate", gateKnob);

    addAndMakeVisible (fillButton);
    fillAtt = std::make_unique<BA> (proc.apvts, "fill", fillButton);

    rebuildList();

    setResizable (true, true);
    setResizeLimits (640, 400, 1200, 720);
    getConstrainer()->setFixedAspectRatio (800.0 / 500.0);
    setSize (800, 500);

    startTimerHz (10);
}

ReChopEditor::~ReChopEditor()
{
    stopTimer();
    list.setModel (nullptr);
    setLookAndFeel (nullptr);
}

void ReChopEditor::paint (Graphics& g)
{
    g.fillAll (bg);
    g.setColour (textLo);
    g.setFont (Font (FontOptions (11.0f)));
    for (auto& c : captions) g.drawText (c.first, c.second, Justification::centredLeft);
}

void ReChopEditor::resized()
{
    captions.clear();
    auto r = getLocalBounds().reduced (22);

    auto top = r.removeFromTop (38);
    title.setBounds (top.removeFromLeft (200));
    version.setBounds (top);
    r.removeFromTop (8);

    auto left = r.removeFromLeft (proportionOfWidth (0.33f));
    r.removeFromLeft (18);

    familyBox.setBounds (left.removeFromTop (28));
    left.removeFromTop (8);
    credit.setBounds (left.removeFromBottom (18));
    list.setBounds (left);

    patternView.setBounds (r.removeFromTop ((int) (r.getHeight() * 0.40f)));
    r.removeFromTop (14);

    // key / scale / octave / slice base
    auto row = r.removeFromTop (46);
    const int cw = row.getWidth() / 4;
    auto cell = [&] (const String& cap) { auto c = row.removeFromLeft (cw).reduced (4, 0); captions.push_back ({ cap, c.removeFromTop (16) }); return c; };
    rootBox.setBounds (cell ("KEY ROOT"));
    scaleBox.setBounds (cell ("SCALE"));
    octaveSlider.setBounds (cell ("OCTAVE"));
    sliceSlider.setBounds (cell ("SLICE 1 NOTE"));

    r.removeFromTop (8);

    // bottom row first, then knobs fill the rest
    auto bottom = r.removeFromBottom (32);
    fillButton.setBounds (bottom.removeFromLeft (80));
    bottom.removeFromLeft (10);
    {
        auto b = bottom.removeFromLeft (90);
        barsBox.setBounds (b);
    }
    captions.push_back ({ "BARS", Rectangle<int> (barsBox.getRight() + 6, bottom.getY(), 40, 32) });
    bottom.removeFromLeft (46);
    dragButton.setBounds (bottom);

    r.removeFromBottom (6);
    const int kw = r.getWidth() / 4;
    auto knob = [&] (Slider& k, const String& cap)
    {
        auto c = r.removeFromLeft (kw);
        captions.push_back ({ cap, c.removeFromTop (16).reduced (4, 0) });
        k.setBounds (c);
    };
    knob (densityKnob, "DENSITY");
    knob (swingKnob, "SWING");
    knob (humanizeKnob, "HUMANIZE");
    knob (gateKnob, "GATE");
}

// ---------------------------------------------------------------- list
void ReChopEditor::rebuildList()
{
    const int f = proc.getFilter();
    ids.clear();
    const auto& lib = rechop::library();
    for (size_t i = 0; i < lib.size(); ++i)
        if (f == 0 || (int) lib[i].family == f - 1) ids.push_back ((int) i);

    list.updateContent();
    lastFilter = f;

    const int cur = proc.getPatternId();
    if (std::find (ids.begin(), ids.end(), cur) == ids.end() && ! ids.empty())
        setPattern (ids.front());

    syncSelection();
    list.repaint();
}

void ReChopEditor::syncSelection()
{
    const int cur = proc.getPatternId();
    auto it = std::find (ids.begin(), ids.end(), cur);
    updatingList = true;
    if (it != ids.end()) list.selectRow ((int) (it - ids.begin()));
    updatingList = false;
    lastId = cur;
}

void ReChopEditor::setPattern (int id)
{
    if (auto* p = proc.apvts.getParameter ("pattern"))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 ((float) id));
        p->endChangeGesture();
    }
}

void ReChopEditor::paintListBoxItem (int row, Graphics& g, int w, int h, bool selected)
{
    if (row < 0 || row >= (int) ids.size()) return;
    const auto& p = rechop::library()[(size_t) ids[(size_t) row]];

    if (selected)
    {
        g.setColour (ReChopLook::orange().withAlpha (0.12f));
        g.fillRect (0, 0, w, h);
        g.setColour (ReChopLook::orange());
        g.fillRect (0, 4, 2, h - 8);
    }
    g.setColour (selected ? Colours::white : textHi);
    g.setFont (Font (FontOptions (14.0f)));
    g.drawText (String (p.name), 12, 0, w - 24, h, Justification::centredLeft);
    g.setColour (textLo);
    g.setFont (Font (FontOptions (11.0f)));
    g.drawText (String (p.style) + (p.kind == rechop::Kind::Slices ? "  / slices" : ""), 12, 0, w - 24, h, Justification::centredRight);
}

void ReChopEditor::selectedRowsChanged (int lastRow)
{
    if (updatingList || lastRow < 0 || lastRow >= (int) ids.size()) return;
    setPattern (ids[(size_t) lastRow]);
}

void ReChopEditor::timerCallback()
{
    if (proc.getFilter() != lastFilter) rebuildList();
    else if (proc.getPatternId() != lastId) syncSelection();
    patternView.repaint();
}
