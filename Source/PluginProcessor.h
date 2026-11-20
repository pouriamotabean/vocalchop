#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "Generator.h"

class ReChopProcessor : public juce::AudioProcessor
{
public:
    ReChopProcessor();
    ~ReChopProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "ReChop"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- shared with the editor
    rechop::Settings getSettings() const;
    int  getPatternId() const;
    int  getFilter() const;                         // 0 = all, 1..3 = family
    const rechop::Pattern& getPattern() const;
    juce::File exportMidiFile (int bars) const;     // for drag-out into Cubase
    double getLastBpm() const { return lastBpm.load(); }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    struct Pending { int note; int vel; double start; double end; };
    struct Active  { int note; double end; };

    std::vector<Pending> ons;
    std::vector<Active>  active;
    bool   wasPlaying = false;
    double lastEnd = 0.0;
    std::atomic<double> lastBpm { 120.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReChopProcessor)
};
