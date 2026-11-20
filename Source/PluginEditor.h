#pragma once
#include "PluginProcessor.h"

// ---------------------------------------------------------------- look (dark, near-monochrome, faint orange)
struct ReChopLook : public juce::LookAndFeel_V4
{
    static juce::Colour orange() { return juce::Colour (0xffff7a1a); }
    ReChopLook();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
};

// ---------------------------------------------------------------- pattern preview (shows exactly what will be played)
class PatternView : public juce::Component
{
public:
    explicit PatternView (ReChopProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
private:
    ReChopProcessor& proc;
};

// ---------------------------------------------------------------- drag the pattern out as a .mid file
class DragMidiButton : public juce::Component
{
public:
    explicit DragMidiButton (ReChopProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { dragging = false; }
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { dragging = false; }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit  (const juce::MouseEvent&) override { hover = false; repaint(); }
private:
    ReChopProcessor& proc;
    bool dragging = false, hover = false;
};

// ---------------------------------------------------------------- editor
class ReChopEditor : public juce::AudioProcessorEditor,
                     private juce::Timer,
                     private juce::ListBoxModel
{
public:
    explicit ReChopEditor (ReChopProcessor&);
    ~ReChopEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    // ListBoxModel
    int getNumRows() override { return (int) ids.size(); }
    void paintListBoxItem (int row, juce::Graphics&, int w, int h, bool selected) override;
    void selectedRowsChanged (int lastRow) override;

    void rebuildList();
    void syncSelection();
    void setPattern (int id);

    ReChopProcessor& proc;
    ReChopLook look;

    std::vector<int> ids;
    bool updatingList = false;
    int lastId = -1, lastFilter = -1;

    juce::Label title, version, credit;
    juce::ComboBox familyBox, rootBox, scaleBox, barsBox;
    juce::ListBox list;
    PatternView patternView;
    DragMidiButton dragButton;
    juce::Slider octaveSlider, sliceSlider, densityKnob, swingKnob, humanizeKnob, gateKnob;
    juce::ToggleButton fillButton { "Fill" };

    std::vector<std::pair<juce::String, juce::Rectangle<int>>> captions;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<CA> familyAtt, rootAtt, scaleAtt, barsAtt;
    std::unique_ptr<SA> octaveAtt, sliceAtt, densityAtt, swingAtt, humanizeAtt, gateAtt;
    std::unique_ptr<BA> fillAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReChopEditor)
};
