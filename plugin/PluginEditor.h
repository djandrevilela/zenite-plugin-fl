#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include <functional>
#include <memory>

#include "PluginProcessor.h"

class DragHandle : public juce::Component {
public:
    std::function<juce::File()> makeFile;
    juce::String text;
    void paint(juce::Graphics&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override { dragging = false; }

private:
    bool dragging = false;
};

// Linha do tempo: secções, curvas do riser e mini piano-roll dos drums.
class TimelineView : public juce::Component {
public:
    explicit TimelineView(ZeniteProcessor& p) : proc(p) {}
    void paint(juce::Graphics&) override;
    void setPlayhead(double b) {
        if (b != playhead) { playhead = b; repaint(); }
    }

private:
    ZeniteProcessor& proc;
    double playhead = -1;
};

class SectionRow : public juce::Component {
public:
    SectionRow(ZeniteProcessor&, int index);
    void paint(juce::Graphics&) override;
    void resized() override;
    void setSection(const zenite::SectionSpan*, int defaultBars, bool editable);

    int defBars = 4;

private:
    int idx;
    juce::Slider bars;
    juce::Label tip;
    juce::String name;
    juce::Colour colour;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

class ZeniteEditor : public juce::AudioProcessorEditor, private juce::ChangeListener, private juce::Timer {
public:
    explicit ZeniteEditor(ZeniteProcessor&);
    ~ZeniteEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void changeListenerCallback(juce::ChangeBroadcaster*) override { refresh(); }
    void timerCallback() override;
    void refresh();
    template <typename T> T* own(T* c) { owned.add(c); addAndMakeVisible(c); return c; }
    juce::ComboBox* addCombo(const char* id, const juce::String& label, std::vector<juce::Component*>& col);
    juce::Slider* addSlider(const char* id, const juce::String& label, std::vector<juce::Component*>& col, bool incDec = false);
    juce::ToggleButton* addToggle(const char* id, const juce::String& text, std::vector<juce::Component*>& col);

    ZeniteProcessor& proc;
    juce::LookAndFeel_V4 lnf;
    juce::OwnedArray<juce::Component> owned;
    juce::OwnedArray<juce::Label> labels;
    std::vector<juce::Component*> topRow, drumCol, buildCol, mixRow;
    std::vector<juce::Label*> mixLabels;
    juce::ComboBox* modeBox = nullptr;
    TimelineView timeline;
    juce::OwnedArray<SectionRow> rows;
    DragHandle dragDrums, dragRiser, dragGuide;
    juce::Label hint;

    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::vector<std::unique_ptr<CA>> cAtt;
    std::vector<std::unique_ptr<BA>> bAtt;
    std::vector<std::unique_ptr<SA>> sAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZeniteEditor)
};
