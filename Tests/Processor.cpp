#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <cstdlib>
#include <iostream>

namespace
{
void require (bool ok, const char* message)
{
    if (! ok) { std::cerr << "FAIL: " << message << '\n'; std::exit (EXIT_FAILURE); }
}
void set (ANKSModularProcessor& p, const char* id, float value)
{
    auto* parameter = p.parameters.getParameter (id);
    require (parameter != nullptr, "parameter missing");
    parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
}
double render (ANKSModularProcessor& p, int frames, int blockSize)
{
    double energy = 0;
    while (frames > 0)
    {
        const int count = std::min (frames, blockSize);
        juce::AudioBuffer<float> audio (2, count);
        juce::MidiBuffer midi;
        p.processBlock (audio, midi);
        for (int i = 0; i < count; ++i)
        {
            const float y = audio.getSample (0, i);
            require (std::isfinite (y) && std::abs (y) < 1, "processor audio invalid");
            require (audio.getSample (1, i) == y, "unexpected stereo mismatch");
            energy += y * y;
        }
        frames -= count;
    }
    return energy;
}
void saveImage (juce::AudioProcessorEditor& editor, const char* name)
{
    auto image = editor.createComponentSnapshot (editor.getLocalBounds());
    require (image.isValid(), "editor snapshot invalid");
    auto file = juce::File::getCurrentWorkingDirectory().getChildFile (name);
    file.deleteFile();
    auto stream = file.createOutputStream();
    juce::PNGImageFormat png;
    require (stream != nullptr && png.writeImageToStream (image, *stream), "could not save rack preview");
}
bool buttonVisible (juce::AudioProcessorEditor& editor, const char* text)
{
    for (auto* child : editor.getChildren())
        if (auto* button = dynamic_cast<juce::Button*> (child))
            if (button->getButtonText() == text) return button->isVisible();
    require (false, "expected panel button missing");
    return false;
}
}

void runFreeRackTests();
int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    for (int rate : {44100, 48000, 96000})
    for (int block : {64, 128, 256, 512})
    {
        ANKSModularProcessor p;
        p.setLegacyMode();
        p.prepareToPlay (rate, block);
        require (render (p, rate / 8, block) == 0, "new processor not silent");
        set (p, "drumRun", 1);
        require (render (p, rate, block) > 0.01, "JUCE drum parameters not reaching DSP");
        require (p.displayedStep() == -1 && p.displayedDrumStep() >= 0, "independent run state failed");
    }

    ANKSModularProcessor p;
    p.setLegacyMode();
    p.prepareToPlay (48000, 128);
    set (p, "drumNoise", 0.72f); set (p, "drumNote4", -13); set (p, "drumVel4", 0);
    p.disconnect (anks::Port::drumMixIn);
    p.connect (anks::Port::drumOut, anks::Port::mixerIn2);
    juce::MemoryBlock saved;
    p.getStateInformation (saved);
    set (p, "drumNoise", 0); set (p, "drumNote4", 0); set (p, "drumVel4", 1);
    p.resetPatch();
    p.setStateInformation (saved.getData(), static_cast<int> (saved.getSize()));
    require (std::abs (p.parameters.getRawParameterValue ("drumNoise")->load() - 0.72f) < 1e-6f, "noise state lost");
    require (p.parameters.getRawParameterValue ("drumNote4")->load() == -13, "pitch state lost");
    require (p.parameters.getRawParameterValue ("drumVel4")->load() == 0, "rest state lost");
    require (p.getRoutes().connected (anks::Port::drumOut, anks::Port::mixerIn2), "drum cable state lost");
    require (p.getRoutes().source[anks::index (anks::Port::drumMixIn)] == -1, "drum bus unplug state lost");

    // Reconstruct V0.3 XML without percussion IDs/ports, then load over active drums.
    auto xml = juce::AudioProcessor::getXmlFromBinary (saved.getData(), static_cast<int> (saved.getSize()));
    require (xml != nullptr, "saved XML invalid");
    auto old = juce::ValueTree::fromXml (*xml);
    old.setProperty ("version", 3, nullptr);
    auto params = old.getChildWithName ("Parameters");
    for (int i = params.getNumChildren() - 1; i >= 0; --i)
    {
        const auto id = params.getChild (i).getProperty ("id").toString();
        if (id.startsWith ("drum") || id.startsWith ("mod")) params.removeChild (i, nullptr);
    }
    auto patch = old.getChildWithName ("PatchCables");
    for (int i = 19; i < anks::portCount; ++i) patch.removeProperty ("in" + juce::String (i), nullptr);
    patch.setProperty ("in17", anks::index (anks::Port::osc2Out), nullptr);
    juce::MemoryBlock legacy;
    juce::AudioProcessor::copyXmlToBinary (*old.createXml(), legacy);
    set (p, "drumRun", 1); set (p, "drumNoise", 1);
    p.setStateInformation (legacy.getData(), static_cast<int> (legacy.getSize()));
    require (p.parameters.getRawParameterValue ("drumRun")->load() == 0, "legacy state unexpectedly starts drums");
    require (std::abs (p.parameters.getRawParameterValue ("drumNoise")->load() - 0.15f) < 1e-6f, "legacy default not restored");
    p.prepareToPlay (48000, 128);
    require (render (p, 24000, 128) == 0, "legacy stopped patch not silent");
    p.requestDrumTrigger();
    require (render (p, 24000, 128) > 0.01, "HIT request does not reach DSP");

    // New CV parameters and cables survive a round trip.
    set (p, "modRate", 4.5f); set (p, "modOffset", 0.3f); set (p, "modRise", 0.01f);
    p.connect (anks::Port::modLagOut, anks::Port::filterCV);
    p.connect (anks::Port::modMixOut, anks::Port::oscMod1In);
    juce::MemoryBlock modulationState;
    p.getStateInformation (modulationState);
    set (p, "modRate", 9); set (p, "modOffset", 0);
    p.disconnect (anks::Port::filterCV); p.disconnect (anks::Port::oscMod1In);
    p.setStateInformation (modulationState.getData(), static_cast<int> (modulationState.getSize()));
    require (std::abs (p.parameters.getRawParameterValue ("modRate")->load() - 4.5f) < 1e-6f, "modulation rate state lost");
    require (std::abs (p.parameters.getRawParameterValue ("modOffset")->load() - 0.3f) < 1e-6f, "modulation offset state lost");
    require (p.getRoutes().connected (anks::Port::modLagOut, anks::Port::filterCV), "modulation filter cable state lost");
    require (p.getRoutes().connected (anks::Port::modMixOut, anks::Port::oscMod1In), "pitch modulation cable state lost");

    auto modXml = juce::AudioProcessor::getXmlFromBinary (modulationState.getData(), static_cast<int> (modulationState.getSize()));
    require (modXml != nullptr, "modulation XML invalid");
    auto v04 = juce::ValueTree::fromXml (*modXml);
    v04.setProperty ("version", 4, nullptr);
    auto v04Params = v04.getChildWithName ("Parameters");
    for (int i = v04Params.getNumChildren() - 1; i >= 0; --i)
        if (v04Params.getChild (i).getProperty ("id").toString().startsWith ("mod")) v04Params.removeChild (i, nullptr);
    auto v04Patch = v04.getChildWithName ("PatchCables");
    for (int i = 21; i < anks::portCount; ++i) v04Patch.removeProperty ("in" + juce::String (i), nullptr);
    v04Patch.setProperty ("in6", -1, nullptr);
    juce::MemoryBlock v04State;
    juce::AudioProcessor::copyXmlToBinary (*v04.createXml(), v04State);
    p.setStateInformation (v04State.getData(), static_cast<int> (v04State.getSize()));
    require (p.parameters.getRawParameterValue ("modRate")->load() == 1, "V0.4 rate default not restored");
    require (p.parameters.getRawParameterValue ("modOffset")->load() == 0, "V0.4 offset default not restored");
    require (p.getRoutes().source[anks::index (anks::Port::filterCV)] == -1, "V0.4 state acquired a modulation cable");
    p.prepareToPlay (48000, 128);
    p.disconnect (anks::Port::modClockIn);
    render (p, 128, 128);
    require (p.displayedModulation()[1] == 0, "unclocked S&H captured unexpectedly");
    p.requestModulationSample(); render (p, 128, 128);
    const float heldSample = p.displayedModulation()[1];
    require (heldSample != 0 && std::abs (heldSample) <= 1, "SAMPLE request did not reach DSP");
    render (p, 128, 128);
    require (p.displayedModulation()[1] == heldSample, "S&H did not hold between manual samples");
    p.resetPatch();

    auto editor = std::unique_ptr<juce::AudioProcessorEditor> (p.createEditor());
    require (editor != nullptr && editor->getWidth() == 1120 && editor->getHeight() == 950, "rack size invalid");
    require (std::abs (p.parameters.getRawParameterValue ("res")->load() - 1.2f) < 1e-5f, "opening editor changed resonance");
    require (std::abs (p.parameters.getRawParameterValue ("sustain")->load() - 0.15f) < 1e-5f, "opening editor changed sustain");
    require (std::abs (p.parameters.getRawParameterValue ("level")->load() - 0.65f) < 1e-5f, "opening editor changed VCA level");
    saveImage (*editor, "MOOGER-rack.png");
    require (buttonVisible (*editor, "SAMPLE") && ! buttonVisible (*editor, "DRUM RUN"), "modulation view contains percussion controls");
    auto* rack = dynamic_cast<ANKSModularEditor*> (editor.get());
    require (rack != nullptr, "unexpected rack type");
    const auto beforeViewSwitch = p.getRoutes().source;
    rack->setModulationPanelVisible (false);
    require (p.getRoutes().source == beforeViewSwitch, "view switch changed patch");
    require (buttonVisible (*editor, "DRUM RUN") && ! buttonVisible (*editor, "SAMPLE"), "percussion view contains modulation controls");
    saveImage (*editor, "MOOGER-rack-drums.png");
    rack->setModulationPanelVisible (true);
    editor->setSize (784, 665);
    saveImage (*editor, "MOOGER-rack-small.png");
    runFreeRackTests();
    std::cout << "PASS: JUCE 3 rates x 4 block sizes, CV/percussion state, V0.3/V0.4 migration, HIT/SAMPLE, panel switching and editor previews\n";
}
