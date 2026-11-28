#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include <memory>
#include <vector>
class FreeRackEditor;

class ANKSModularEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ANKSModularEditor (ANKSModularProcessor&);
    ~ANKSModularEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void setModulationPanelVisible (bool);

private:
    struct Jack
    {
        anks::Port id;
        juce::Point<float> position;
        const char* label;
        int signalType; // 0 audio, 1 pitch, 2 gate, 3 cv
    };
    void timerCallback() override;
    void addKnob (const juce::String& id, const juce::String& title,
                  int x, int y, int w, int h);
    int hitJack (juce::Point<float>) const;
    void drawCable (juce::Graphics&, juce::Point<float>, juce::Point<float>, int, bool preview = false);
    void drawJack (juce::Graphics&, const Jack&);
    bool jackVisible (const Jack&) const noexcept;
    void drawModule (juce::Graphics&, juce::Rectangle<int>, const juce::String&, const juce::String&);
    static juce::Colour signalColor (int);

    ANKSModularProcessor& processor;
    juce::LookAndFeel_V4 skin;
    std::vector<std::unique_ptr<juce::Slider>> knobs;
    std::vector<std::unique_ptr<juce::Label>> knobNames;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> knobAttachments;
    std::array<std::unique_ptr<juce::Slider>, 8> stepSliders;
    std::array<std::unique_ptr<juce::ToggleButton>, 8> stepButtons;
    std::array<std::unique_ptr<juce::Slider>, 8> drumPitchSliders, drumVelocitySliders;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> stepAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> stepButtonAttachments;
    juce::ComboBox wave, wave2, drumWave;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveAttachment, wave2Attachment, drumWaveAttachment;
    juce::TextButton runButton, seqButton, previewButton, resetButton;
    juce::TextButton drumRunButton, drumTriggerButton, drumFollowButton;
    juce::TextButton panelButton, sampleButton, animateButton;
    juce::TextButton newFreeRackButton;
    std::unique_ptr<FreeRackEditor> freeRackOverlay;
    std::vector<juce::Component*> drumControls, modulationControls;
    bool modulationPanelVisible = true;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> runAttachment, seqAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> drumRunAttachment, drumFollowAttachment;
    const std::array<Jack, 41> jacks;
    int selectedJack = -1;
    bool dragging = false;
    juce::Point<float> dragPosition;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ANKSModularEditor)
};
