#include "../Source/PluginProcessor.h"
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
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    for (int rate : {44100, 48000, 96000})
    for (int block : {64, 128, 256, 512})
    {
        ANKSModularProcessor p;
        p.prepareToPlay (rate, block);
        require (render (p, rate / 8, block) == 0, "new processor not silent");
        set (p, "drumRun", 1);
        require (render (p, rate, block) > 0.01, "JUCE drum parameters not reaching DSP");
        require (p.displayedStep() == -1 && p.displayedDrumStep() >= 0, "independent run state failed");
    }

    ANKSModularProcessor p;
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
        if (params.getChild (i).getProperty ("id").toString().startsWith ("drum")) params.removeChild (i, nullptr);
    auto patch = old.getChildWithName ("PatchCables");
    patch.removeProperty ("in19", nullptr); patch.removeProperty ("in20", nullptr);
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

    auto editor = std::unique_ptr<juce::AudioProcessorEditor> (p.createEditor());
    require (editor != nullptr && editor->getWidth() == 1120 && editor->getHeight() == 950, "rack size invalid");
    saveImage (*editor, "MOOGER-rack.png");
    editor->setSize (784, 665);
    saveImage (*editor, "MOOGER-rack-small.png");
    std::cout << "PASS: JUCE 3 rates x 4 block sizes, parameter/cable state, legacy migration, HIT, editor previews\n";
}
