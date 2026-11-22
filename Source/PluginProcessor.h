#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "SynthEngine.h"
#include <array>
#include <atomic>

class ANKSModularProcessor final : public juce::AudioProcessor
{
public:
    ANKSModularProcessor();
    ~ANKSModularProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "MOOGER Init"; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    bool connect (anks::Port src, anks::Port dst) noexcept;
    void disconnect (anks::Port dst) noexcept;
    void resetPatch() noexcept;
    anks::Routes getRoutes() const noexcept;
    int displayedStep() const noexcept { return activeStep.load (std::memory_order_relaxed); }
    int displayedDrumStep() const noexcept { return activeDrumStep.load (std::memory_order_relaxed); }
    void requestDrumTrigger() noexcept { drumTriggerRequested.store (true, std::memory_order_release); }
    void requestPreview() noexcept { previewRequested.store (true, std::memory_order_release); }
    juce::AudioProcessorValueTreeState parameters;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    anks::Parameters readParameters() const noexcept;
    anks::Engine engine;
    std::array<std::atomic<int>, anks::portCount> inputSources {};
    std::atomic<int> activeStep {-1};
    std::atomic<int> activeDrumStep {-1};
    std::atomic<bool> drumTriggerRequested {false};
    std::atomic<bool> previewRequested {false};
    int previewCountdown = 0;
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ANKSModularProcessor)
};
