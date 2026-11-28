#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "SynthEngine.h"
#include "RackEngine.h"
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
    double getTailLengthSeconds() const override { return isFreeRack() ? 180.0 : 6.0; }
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
    void requestModulationSample() noexcept { sampleRequested.store (true, std::memory_order_release); }
    std::array<float, 3> displayedModulation() const noexcept
    {
        return {{ modTriangle.load (std::memory_order_relaxed), modHeld.load (std::memory_order_relaxed),
                  modLag.load (std::memory_order_relaxed) }};
    }
    void requestPreview() noexcept { previewRequested.store (true, std::memory_order_release); }
    juce::AudioProcessorValueTreeState parameters;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    bool isFreeRack() const noexcept { return freeRackEnabled.load (std::memory_order_acquire); }
    void setLegacyMode() noexcept { freeRackEnabled.store (false, std::memory_order_release); }
    void newFreeRack (int preset = 1);
    mooger::RackState rackState() const;
    int addRackModule (mooger::Type, int column = -1, int row = -1);
    int duplicateRackModule (int slot);
    bool replaceRackModule (int slot, mooger::Type);
    void removeRackModule (int slot);
    bool moveRackModule (int slot, int column, int row);
    bool connectRack (mooger::Cable);
    void disconnectRack (int slot, int input);
    void setRackValue (int slot, int parameter, float normalized);
    float rackValue (int slot, int parameter) const noexcept;
    juce::RangedAudioParameter* rackParameter (int slot, int parameter) noexcept;
    void triggerRackModule (int slot) noexcept;
    int rackDisplayedStep (int slot) const noexcept { return mooger::validSlot (slot) ? rackSteps[size_t (slot)].load() : -1; }
    juce::ValueTree exportRack() const;
    bool restoreRack (const juce::ValueTree&);

private:
    anks::Parameters readParameters() const noexcept;
    anks::Engine engine;
    std::array<std::atomic<int>, anks::portCount> inputSources {};
    std::atomic<int> activeStep {-1};
    std::atomic<int> activeDrumStep {-1};
    std::atomic<bool> drumTriggerRequested {false};
    std::atomic<bool> sampleRequested {false};
    std::atomic<float> modTriangle {-1}, modHeld {0}, modLag {0};
    std::atomic<bool> previewRequested {false};
    int previewCountdown = 0;
    double currentSampleRate = 44100.0;
    void initializeRack();
    static bool parseRackTree (const juce::ValueTree&, mooger::RackState&);
    void publishRackLocked();
    void installRackValues (const mooger::RackState&);
    mutable juce::CriticalSection rackLock;
    mooger::RackState rackModel;
    mooger::RackMailbox rackMailbox;
    mooger::RackEngine rackEngine;
    std::array<std::array<std::atomic<float>*, mooger::maxParameters>, mooger::maxModules> rackRaw {};
    std::array<std::array<juce::RangedAudioParameter*, mooger::maxParameters>, mooger::maxModules> rackObjects {};
    std::array<std::atomic<int>, mooger::maxModules> rackSteps {};
    std::atomic<bool> freeRackEnabled {true};
    std::atomic<std::uint32_t> rackTriggers {0};
    std::uint64_t rackRevision = 1, rackGeneration = 1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ANKSModularProcessor)
};
