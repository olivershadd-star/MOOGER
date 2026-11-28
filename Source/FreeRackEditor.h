#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
class ANKSModularProcessor;
class FreeRackEditor final : public juce::AudioProcessorEditor, public juce::DragAndDropContainer
{
public:
    explicit FreeRackEditor (ANKSModularProcessor&, bool embedded = false);
    ~FreeRackEditor() override;
    void resized() override;
    void paint (juce::Graphics&) override;
    void setRear (bool);
    bool isRear() const;
    int visibleMusicalControls() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
