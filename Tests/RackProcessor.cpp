#include "../Source/PluginProcessor.h"
#include "../Source/FreeRackEditor.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <cstdlib>
#include <iostream>

namespace
{
void check (bool ok, const char* text) { if (! ok) { std::cerr << "FAIL free rack: " << text << '\n'; std::exit (1); } }
double render (ANKSModularProcessor& p, int frames, int block)
{
    double energy = 0;
    while (frames > 0)
    {
        const int count = std::min (frames, block); juce::AudioBuffer<float> audio (2, count); juce::MidiBuffer midi;
        p.processBlock (audio, midi);
        for (int i = 0; i < count; ++i) for (int channel = 0; channel < 2; ++channel)
        { const float v = audio.getSample (channel, i); check (std::isfinite (v) && std::abs (v) <= 0.801f, "output clipping or NaN"); energy += v * v; }
        frames -= count;
    }
    return energy;
}
void snapshot (juce::AudioProcessorEditor& editor, const char* name)
{
    auto image = editor.createComponentSnapshot (editor.getLocalBounds()); check (image.isValid(), "snapshot invalid");
    auto file = juce::File::getCurrentWorkingDirectory().getChildFile (name); file.deleteFile(); auto stream = file.createOutputStream();
    juce::PNGImageFormat png; check (stream && png.writeImageToStream (image, *stream), "PNG write failed");
}
juce::Component* find (juce::Component& root, const juce::String& id)
{
    if (root.getComponentID() == id) return &root;
    for (auto* child : root.getChildren()) if (auto* result = find (*child, id)) return result;
    return nullptr;
}
juce::MouseEvent mouse (juce::Component* component, juce::Point<float> at, juce::Point<float> down)
{
    const auto now = juce::Time::getCurrentTime();
    return {juce::Desktop::getInstance().getMainMouseSource(), at, juce::ModifierKeys (juce::ModifierKeys::leftButtonModifier),
        1.0f, 0, 0, 0, 0, component, component, now, down, now, 1, at != down};
}
}
void runFreeRackTests()
{
    for (int rate : {44100, 48000, 96000}) for (int block : {64, 128, 256, 512})
    {
        ANKSModularProcessor p; check (p.isFreeRack(), "fresh instance is not free rack");
        p.prepareToPlay (rate, block); check (render (p, rate / 10, block) == 0, "starter rack starts sounding without RUN");
        p.setRackValue (0, 1, 1); check (render (p, rate, block) > 0.01, "sequence rack has no audio");
        p.prepareToPlay (rate, block); check (render (p, rate, block) > 0.01, "reprepare lost published graph");
    }
    ANKSModularProcessor p; p.prepareToPlay (48000, 128);
    const int vco2 = p.duplicateRackModule (1), env2 = p.duplicateRackModule (3), lfo2 = p.duplicateRackModule (6);
    check (vco2 >= 0 && env2 >= 0 && lfo2 >= 0, "cannot duplicate VCO/envelope/LFO");
    auto model = p.rackState();
    check (model.modules[size_t (vco2)].uid != model.modules[1].uid, "duplicate shares ID");
    p.setRackValue (vco2, 1, 0.9f); check (p.rackValue (1, 1) != p.rackValue (vco2, 1), "duplicate parameters shared");
    check (p.moveRackModule (vco2, 2, 4), "move failed");
    check (p.connectRack ({lfo2, 1, env2, 0}), "free CV cable rejected");
    check (p.connectRack ({vco2, 0, 2, 0}), "audio rerouting rejected");
    check (p.replaceRackModule (2, mooger::Type::delay), "replace filter with effect failed");
    model = p.rackState(); check (mooger::validate (model), "invalid graph after replacement");
    check (model.modules[2].type == mooger::Type::delay && model.modules[size_t (vco2)].row == 4, "replacement/move state not applied");
    const auto savedModel = p.exportRack(); juce::MemoryBlock saved; p.getStateInformation (saved);
    p.newFreeRack (0); check (p.rackState().cableCount == 0, "empty rack has cables");
    p.setStateInformation (saved.getData(), int (saved.getSize()));
    check (p.isFreeRack() && p.exportRack().isEquivalentTo (savedModel), "rack round trip lost nodes/positions/cables/parameters");
    auto invalid = savedModel.createCopy(); invalid.getChildWithName ("Module").setProperty ("type", "unknown-module", nullptr);
    check (! p.restoreRack (invalid) && p.exportRack().isEquivalentTo (savedModel), "invalid catalog entry destroyed rack");
    p.removeRackModule (vco2); model = p.rackState();
    for (int i = 0; i < model.cableCount; ++i) check (model.cables[size_t (i)].from != vco2 && model.cables[size_t (i)].to != vco2, "removed module leaves dangling cables");

    // MIDI events reach an independently assembled keyboard rack.
    p.newFreeRack (2); p.prepareToPlay (48000, 128);
    juce::AudioBuffer<float> audio (2, 128); juce::MidiBuffer notes;
    notes.addEvent (juce::MidiMessage::noteOn (1, 60, 0.8f), 64); p.processBlock (audio, notes);
    for (int i = 0; i < 64; ++i) check (audio.getSample (0, i) == 0, "MIDI started before event offset");
    check (render (p, 24000, 128) > 0.01, "MIDI rack silent");
    notes.clear(); notes.addEvent (juce::MidiMessage::noteOff (1, 60), 0); p.processBlock (audio, notes);
    render (p, 48000, 128); check (render (p, 128, 128) < 1e-9, "MIDI release stuck");

    p.newFreeRack (1);
    auto editor = std::unique_ptr<juce::AudioProcessorEditor> (p.createEditor());
    auto* rack = dynamic_cast<FreeRackEditor*> (editor.get()); check (rack != nullptr, "fresh free rack did not open free editor");
    check (! rack->isRear() && rack->visibleMusicalControls() > 0, "front face has no controls");
    const auto beforeFlip = p.exportRack();
    snapshot (*rack, "MOOGER-rack-free-front.png");
    rack->setRear (true); check (rack->isRear() && rack->visibleMusicalControls() == 0, "rear face still contains musical controls");
    check (p.exportRack().isEquivalentTo (beforeFlip), "flipping rack changed audio graph or parameters");
    auto* oscillatorCard = find (*rack, "rack-module-1"); auto* filterCard = find (*rack, "rack-module-2");
    check (oscillatorCard && filterCard, "module faces not found");
    // Real UI handlers: unplug a rear input, then connect output to input by clicks.
    filterCard->mouseDown (mouse (filterCard, {14, 68}, {14, 68}));
    check (p.rackState().cableCount == 5, "rear input click did not unplug");
    oscillatorCard->mouseDown (mouse (oscillatorCard, {266, 68}, {266, 68}));
    filterCard->mouseDown (mouse (filterCard, {14, 68}, {14, 68}));
    check (p.rackState().cableCount == 6, "rear output/input clicks did not reconnect");
    snapshot (*rack, "MOOGER-rack-free-back.png");
    rack->setRear (false); check (rack->visibleMusicalControls() > 0, "flipping back lost controls");
    const auto down = mouse (oscillatorCard, {30, 15}, {30, 15}); oscillatorCard->mouseDown (down);
    oscillatorCard->mouseDrag (mouse (oscillatorCard, {622, 311}, {30, 15}));
    oscillatorCard->mouseUp (mouse (oscillatorCard, {30, 15}, {30, 15}));
    check (p.rackState().modules[1].column == 3 && p.rackState().modules[1].row == 1, "header drag did not move module");
    p.moveRackModule (1, 1, 0);
    auto* canvas = find (*rack, "rack-canvas"); auto* bank = find (*rack, "rack-bank");
    auto* dropTarget = dynamic_cast<juce::DragAndDropTarget*> (canvas);
    check (dropTarget && bank, "library drag/drop not attached");
    juce::DragAndDropTarget::SourceDetails drop ("catalog:4", bank, {600, 600});
    dropTarget->itemDropped (drop);
    check (p.rackState().modules[7].active && p.rackState().modules[7].type == mooger::Type::lfo, "catalog drop did not instantiate LFO");
    rack->setSize (1100, 750); snapshot (*rack, "MOOGER-rack-free-small.png");
    std::cout << "PASS: free rack host matrix, duplication, replacement, layout/state, MIDI, invalid presets, separate front/back faces\n";
}
