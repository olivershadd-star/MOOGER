#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "FreeRackEditor.h"
#include <cmath>

namespace
{
using APVTS = juce::AudioProcessorValueTreeState;
using ID = juce::ParameterID;
void addFloat (APVTS::ParameterLayout& l, const char* id, const char* label,
               float lo, float hi, float def, float skew = 0.5f)
{
    juce::NormalisableRange<float> r {lo, hi};
    r.setSkewForCentre (lo + (hi - lo) * std::clamp (skew, 0.01f, 0.99f));
    const juce::String key (id);
    int precision = 2;
    if (key == "bpm" || key == "cutoff" || key == "lfoDepth" || key == "drumBpm"
        || key == "drumCutoff" || key == "drumPitchDepth" || key == "modFilterDepth") precision = 0;
    if (key == "tune" || key == "tune2" || key == "fine2" || key == "drumPitch") precision = 1;
    if (key == "attack" || key == "decay" || key == "release" || key == "drumDecay" || key == "drumPitchDecay"
        || key == "modRise" || key == "modFall") precision = 3;
    auto attributes = juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction ([precision] (float value, int) { return juce::String (value, precision); })
        .withValueFromStringFunction ([] (const juce::String& text) { return text.getFloatValue(); });
    l.add (std::make_unique<juce::AudioParameterFloat> (ID {id, 1}, label, r, def, attributes));
}
}

APVTS::ParameterLayout ANKSModularProcessor::createParameterLayout()
{
    APVTS::ParameterLayout result;
    result.add (std::make_unique<juce::AudioParameterBool> (ID {"run",1}, "Run", false));
    result.add (std::make_unique<juce::AudioParameterBool> (ID {"seqMode",1}, "Internal sequencer mode", true));
    addFloat (result, "bpm", "Tempo BPM", 40, 240, 110);
    result.add (std::make_unique<juce::AudioParameterChoice> (ID {"wave",1}, "Oscillator wave",
                                                             juce::StringArray {"Sine", "Triangle", "Saw", "Square"}, 2));
    addFloat (result, "tune", "Oscillator tune", -12, 12, 0);
    result.add (std::make_unique<juce::AudioParameterChoice> (ID {"wave2",1}, "VCO 2 wave",
                   juce::StringArray {"Sine", "Triangle", "Saw", "Square"}, 2));
    addFloat (result, "tune2", "VCO 2 tune semitones", -24, 24, 0, 0.5f);
    addFloat (result, "fine2", "VCO 2 fine cents", -50, 50, 7, 0.5f);
    addFloat (result, "mix1", "Mixer input 1", 0, 1, 0.65f, 0.5f);
    addFloat (result, "mix2", "Mixer input 2", 0, 1, 0.35f, 0.5f);
    addFloat (result, "mixLevel", "Mixer output", 0, 1, 0.8f, 0.5f);
    addFloat (result, "cutoff", "Filter cutoff Hz", 25, 18000, 1200, 0.16f);
    addFloat (result, "res", "Filter resonance Q", 0.5, 9, 1.2f);
    addFloat (result, "level", "VCA level", 0, 1, 0.65f);
    addFloat (result, "attack", "Envelope attack", 0.002f, 2, 0.012f, 0.2f);
    addFloat (result, "decay", "Envelope decay", 0.003f, 3, 0.12f, 0.2f);
    addFloat (result, "sustain", "Envelope sustain", 0, 1, 0.15f);
    addFloat (result, "release", "Envelope release", 0.003f, 4, 0.14f, 0.15f);
    addFloat (result, "lfoRate", "LFO rate Hz", 0.05f, 25.0f, 2.0f, 0.2f);
    addFloat (result, "lfoDepth", "LFO depth Hz", 0, 5000, 500, 0.2f);
    addFloat (result, "master", "Master level", 0, 1, 0.42f);
    const int notes[] = {0, 0, 3, 7, 0, 5, 3, 10};
    for (int i = 0; i < 8; ++i)
    {
        result.add (std::make_unique<juce::AudioParameterInt>
                    (ID {"note" + juce::String (i + 1), 1}, "Note " + juce::String (i + 1), -12, 12, notes[i]));
        result.add (std::make_unique<juce::AudioParameterBool>
                    (ID {"on" + juce::String (i + 1), 1}, "Step " + juce::String (i + 1) + " enabled", true));
    }
    // Append new IDs so V0.3 host parameter indices remain stable.
    result.add (std::make_unique<juce::AudioParameterBool> (ID {"drumRun",1}, "Percussion run", false));
    result.add (std::make_unique<juce::AudioParameterBool> (ID {"drumFollow",1}, "Percussion follows core tempo", true));
    result.add (std::make_unique<juce::AudioParameterChoice> (ID {"drumWave",1}, "Percussion waveform",
                   juce::StringArray {"Triangle", "Square"}, 0));
    addFloat (result, "drumBpm", "Percussion tempo", 40, 240, 110, 0.5f);
    addFloat (result, "drumPitch", "Percussion base Hz", 25, 800, 65.4064f, 0.12f);
    addFloat (result, "drumRatio", "Percussion VCO2 ratio", 0.25f, 8, 1.5f, 0.3f);
    addFloat (result, "drumTone", "Percussion VCO2 mix", 0, 1, 0.35f, 0.5f);
    addFloat (result, "drumNoise", "Percussion noise", 0, 1, 0.15f, 0.5f);
    addFloat (result, "drumFm", "Percussion FM", 0, 4, 0, 0.25f);
    addFloat (result, "drumDecay", "Percussion amp decay seconds", 0.02f, 2, 0.22f, 0.2f);
    addFloat (result, "drumPitchDecay", "Percussion pitch decay seconds", 0.005f, 1, 0.04f, 0.15f);
    addFloat (result, "drumPitchDepth", "Percussion pitch envelope semitones", 0, 48, 24, 0.5f);
    addFloat (result, "drumCutoff", "Percussion cutoff Hz", 40, 16000, 3500, 0.2f);
    addFloat (result, "drumRes", "Percussion resonance", 0.5f, 6, 0.8f, 0.25f);
    addFloat (result, "drumLevel", "Percussion level", 0, 1, 0.55f, 0.5f);
    const anks::DrumParameters defaults;
    for (int i = 0; i < 8; ++i)
    {
        const auto suffix = juce::String (i + 1);
        result.add (std::make_unique<juce::AudioParameterInt> (ID {"drumNote" + suffix, 1},
            "Percussion pitch " + suffix, -24, 24, defaults.steps[static_cast<size_t> (i)].pitch));
        addFloat (result, ("drumVel" + suffix).toRawUTF8(), ("Percussion velocity " + suffix).toRawUTF8(),
                  0, 1, defaults.steps[static_cast<size_t> (i)].velocity, 0.5f);
    }
    addFloat (result, "modRate", "Modulation LFO rate Hz", 0.05f, 30, 1, 0.2f);
    addFloat (result, "modAtten1", "CV attenuator 1", 0, 1, 0.5f);
    addFloat (result, "modAtten2", "CV attenuator 2", 0, 1, 0.5f);
    addFloat (result, "modMix1", "CV mixer input 1 level", 0, 1, 1);
    addFloat (result, "modMix2", "CV mixer input 2 level", 0, 1, 1);
    addFloat (result, "modOffset", "CV mixer offset", -1, 1, 0);
    addFloat (result, "modLevel", "CV mixer output level", 0, 1, 0.8f);
    addFloat (result, "modRise", "CV lag rise seconds", 0.001f, 2, 0.15f, 0.2f);
    addFloat (result, "modFall", "CV lag fall seconds", 0.001f, 2, 0.15f, 0.2f);
    addFloat (result, "modPitchDepth", "Pitch CV depth semitones", 0, 24, 2);
    addFloat (result, "modFilterDepth", "Filter CV depth Hz", 0, 8000, 1500);
    addFloat (result, "modAmpDepth", "VCA CV depth", 0, 1, 1);
    for (int slot = 0; slot < mooger::maxModules; ++slot)
        for (int p = 0; p < mooger::maxParameters; ++p)
            result.add (std::make_unique<juce::AudioParameterFloat>
                (ID {"rack" + juce::String (slot) + "p" + juce::String (p), 1},
                 "Rack " + juce::String (slot + 1) + " control " + juce::String (p + 1), 0.0f, 1.0f, 0.0f));
    return result;
}

ANKSModularProcessor::ANKSModularProcessor()
  : juce::AudioProcessor (BusesProperties().withOutput ("Stereo output", juce::AudioChannelSet::stereo(), true)),
    parameters (*this, nullptr, "Parameters", createParameterLayout())
{
    resetPatch();
    initializeRack();
}

void ANKSModularProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = std::max (8000.0, sampleRate);
    engine.prepare (currentSampleRate);
    rackEngine.prepare (currentSampleRate);
    activeStep.store (-1, std::memory_order_relaxed);
    activeDrumStep.store (-1, std::memory_order_relaxed);
    drumTriggerRequested.store (false, std::memory_order_relaxed);
    sampleRequested.store (false, std::memory_order_relaxed);
    modTriangle.store (-1, std::memory_order_relaxed);
    modHeld.store (0, std::memory_order_relaxed); modLag.store (0, std::memory_order_relaxed);
    previewCountdown = 0;
}
void ANKSModularProcessor::releaseResources() {}

bool ANKSModularProcessor::isBusesLayoutSupported (const BusesLayout& layout) const
{
    return layout.getMainInputChannelSet().isDisabled()
        && (layout.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
            || layout.getMainOutputChannelSet() == juce::AudioChannelSet::stereo());
}

anks::Routes ANKSModularProcessor::getRoutes() const noexcept
{
    anks::Routes routes;
    for (int i = 0; i < anks::portCount; ++i)
        routes.source[static_cast<size_t> (i)] = inputSources[static_cast<size_t> (i)].load (std::memory_order_acquire);
    return routes;
}

bool ANKSModularProcessor::connect (anks::Port src, anks::Port dst) noexcept
{
    if (! anks::validConnection (src, dst)) return false;
    inputSources[static_cast<size_t> (anks::index (dst))].store (anks::index (src), std::memory_order_release);
    return true;
}
void ANKSModularProcessor::disconnect (anks::Port dst) noexcept
{
    if (dst == anks::Port::count || anks::isOutput (dst)) return;
    inputSources[static_cast<size_t> (anks::index (dst))].store (-1, std::memory_order_release);
}
void ANKSModularProcessor::resetPatch() noexcept
{
    const anks::Routes r;
    for (int i = 0; i < anks::portCount; ++i)
        inputSources[static_cast<size_t> (i)].store (r.source[static_cast<size_t> (i)], std::memory_order_release);
}

anks::Parameters ANKSModularProcessor::readParameters() const noexcept
{
    anks::Parameters p;
    auto val = [this] (const char* key) noexcept -> float
    { return parameters.getRawParameterValue (key)->load (std::memory_order_relaxed); };
    p.run = val ("run") > 0.5f;
    p.seqMode = val ("seqMode") > 0.5f;
    p.bpm = val ("bpm");
    p.waveform = static_cast<int> (val ("wave"));
    p.tune = val ("tune");
    p.waveform2 = static_cast<int> (val ("wave2"));
    p.tune2 = val ("tune2"); p.fine2 = val ("fine2");
    p.mix1 = val ("mix1"); p.mix2 = val ("mix2"); p.mixLevel = val ("mixLevel");
    p.cutoff = val ("cutoff");
    p.resonance = val ("res");
    p.level = val ("level");
    p.attack = val ("attack"); p.decay = val ("decay");
    p.sustain = val ("sustain"); p.release = val ("release");
    p.lfoRate = val ("lfoRate"); p.lfoDepth = val ("lfoDepth");
    p.master = val ("master");
    p.drums.run = val ("drumRun") > 0.5f;
    p.drums.followTempo = val ("drumFollow") > 0.5f;
    p.drums.waveform = static_cast<int> (val ("drumWave"));
    p.drums.bpm = val ("drumBpm"); p.drums.pitchHz = val ("drumPitch");
    p.drums.ratio = val ("drumRatio"); p.drums.tone = val ("drumTone");
    p.drums.noise = val ("drumNoise"); p.drums.fm = val ("drumFm");
    p.drums.decay = val ("drumDecay"); p.drums.pitchDecay = val ("drumPitchDecay");
    p.drums.pitchDepth = val ("drumPitchDepth"); p.drums.cutoff = val ("drumCutoff");
    p.drums.resonance = val ("drumRes"); p.drums.level = val ("drumLevel");
    p.modulation.rate = val ("modRate");
    p.modulation.atten1 = val ("modAtten1"); p.modulation.atten2 = val ("modAtten2");
    p.modulation.mix1 = val ("modMix1"); p.modulation.mix2 = val ("modMix2");
    p.modulation.offset = val ("modOffset"); p.modulation.level = val ("modLevel");
    p.modulation.rise = val ("modRise"); p.modulation.fall = val ("modFall");
    p.modulation.pitchDepth = val ("modPitchDepth"); p.modulation.filterDepth = val ("modFilterDepth");
    p.modulation.ampDepth = val ("modAmpDepth");
    for (int i = 0; i < 8; ++i)
    {
        const auto noteID = "note" + juce::String (i + 1);
        const auto stepID = "on" + juce::String (i + 1);
        p.steps[static_cast<size_t> (i)].semitones = static_cast<int> (parameters.getRawParameterValue (noteID)->load());
        p.steps[static_cast<size_t> (i)].enabled = parameters.getRawParameterValue (stepID)->load() >= 0.5f;
        const auto drumNoteID = "drumNote" + juce::String (i + 1);
        const auto drumVelID = "drumVel" + juce::String (i + 1);
        p.drums.steps[static_cast<size_t> (i)].pitch = static_cast<int> (parameters.getRawParameterValue (drumNoteID)->load());
        p.drums.steps[static_cast<size_t> (i)].velocity = parameters.getRawParameterValue (drumVelID)->load();
    }
    return p;
}

void ANKSModularProcessor::processBlock (juce::AudioBuffer<float>& audio, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals guard;
    if (isFreeRack())
    {
        mooger::RackState incoming;
        if (rackMailbox.read (incoming)) rackEngine.apply (incoming);
        mooger::RackControls values {};
        for (int slot = 0; slot < mooger::maxModules; ++slot)
            for (int p = 0; p < mooger::maxParameters; ++p) values[size_t (slot)][size_t (p)] = rackValue (slot, p);
        rackEngine.setControls (values);
        const auto triggers = rackTriggers.exchange (0, std::memory_order_acq_rel);
        for (int slot = 0; slot < mooger::maxModules; ++slot)
            if ((triggers & (std::uint32_t (1) << slot)) != 0) rackEngine.trigger (slot);
        audio.clear(); auto event = midi.cbegin();
        for (int frame = 0; frame < audio.getNumSamples(); ++frame)
        {
            while (event != midi.cend() && (*event).samplePosition <= frame)
            {
                const auto message = *event;
                if (message.numBytes >= 3)
                {
                    const int kind = message.data[0] & 0xf0;
                    if (kind == 0x90 && message.data[2] > 0) rackEngine.noteOn (message.data[1], message.data[2] / 127.0f);
                    if (kind == 0x80 || (kind == 0x90 && message.data[2] == 0)) rackEngine.noteOff (message.data[1]);
                    if (kind == 0xb0 && (message.data[1] == 120 || message.data[1] == 123)) rackEngine.allNotesOff();
                }
                ++event;
            }
            const auto sample = rackEngine.process();
            if (audio.getNumChannels() > 0) audio.setSample (0, frame, audio.getNumChannels() == 1 ? (sample.left + sample.right) * 0.5f : sample.left);
            if (audio.getNumChannels() > 1) audio.setSample (1, frame, sample.right);
        }
        for (int slot = 0; slot < mooger::maxModules; ++slot) rackSteps[size_t (slot)].store (rackEngine.step (slot), std::memory_order_relaxed);
        return;
    }
    const auto p = readParameters();
    const auto routes = getRoutes();
    audio.clear();
    juce::MidiBuffer::Iterator iterator (midi);
    juce::MidiMessage midiEvent;
    int eventSample = 0;
    bool hasEvent = iterator.getNextEvent (midiEvent, eventSample);
    const int channels = audio.getNumChannels();
    const int n = audio.getNumSamples();
    auto* left = channels > 0 ? audio.getWritePointer (0) : nullptr;
    auto* right = channels > 1 ? audio.getWritePointer (1) : nullptr;
    if (drumTriggerRequested.exchange (false, std::memory_order_acq_rel)) engine.triggerDrum (p);
    if (sampleRequested.exchange (false, std::memory_order_acq_rel)) engine.sampleModulation();

    for (int i = 0; i < n; ++i)
    {
        while (hasEvent && eventSample <= i)
        {
            if (! p.seqMode)
            {
                if (midiEvent.isNoteOn()) engine.noteOn (midiEvent.getNoteNumber(), p, routes);
                if (midiEvent.isNoteOff() || midiEvent.isAllNotesOff()) engine.noteOff (p, routes);
            }
            hasEvent = iterator.getNextEvent (midiEvent, eventSample);
        }
        if (previewRequested.exchange (false, std::memory_order_acq_rel))
        {
            engine.noteOn (48, p, routes);
            previewCountdown = static_cast<int> (currentSampleRate * 0.24);
        }
        if (previewCountdown > 0 && --previewCountdown == 0) engine.noteOff (p, routes);
        const float y = engine.process (p, routes);
        if (left != nullptr) left[i] = y;
        if (right != nullptr) right[i] = y;
    }
    activeStep.store (p.run && p.seqMode ? engine.getCurrentStep() : -1, std::memory_order_relaxed);
    activeDrumStep.store (p.drums.run ? engine.getDrumStep() : -1, std::memory_order_relaxed);
    const auto& mod = engine.getModulation();
    modTriangle.store (static_cast<float> (mod.triangle), std::memory_order_relaxed);
    modHeld.store (static_cast<float> (mod.held), std::memory_order_relaxed);
    modLag.store (static_cast<float> (mod.lag), std::memory_order_relaxed);
}

void ANKSModularProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    juce::ValueTree root ("ANKSModularState");
    root.setProperty ("version", 6, nullptr);
    root.addChild (parameters.copyState(), -1, nullptr);
    juce::ValueTree patch ("PatchCables");
    const auto r = getRoutes();
    for (int i = 0; i < anks::portCount; ++i)
        patch.setProperty ("in" + juce::String (i), r.source[static_cast<size_t> (i)], nullptr);
    root.addChild (patch, -1, nullptr);
    root.addChild (exportRack(), -1, nullptr);
    if (auto xml = root.createXml()) juce::AudioProcessor::copyXmlToBinary (*xml, dest);
}
void ANKSModularProcessor::setStateInformation (const void* data, int bytes)
{
    if (auto xml = juce::AudioProcessor::getXmlFromBinary (data, bytes))
    {
        const auto root = juce::ValueTree::fromXml (*xml);
        if (! root.hasType ("ANKSModularState")) return;
        const auto freeRack = root.getChildWithName ("FreeRack");
        const int version = root.getProperty ("version", 5);
        mooger::RackState parsed;
        if (version >= 6 && ! parseRackTree (freeRack, parsed)) return;
        auto parameterTree = root.getChildWithName ("Parameters");
        if (parameterTree.isValid())
        {
            // Fill missing IDs with defaults when loading older states.
            for (auto* baseParameter : getParameters())
            {
                const auto* parameter = dynamic_cast<const juce::RangedAudioParameter*> (baseParameter);
                if (parameter == nullptr) continue;
                const auto& id = parameter->paramID;
                if (parameterTree.getChildWithProperty ("id", id).isValid()) continue;
                juce::ValueTree child ("PARAM");
                child.setProperty ("id", id, nullptr);
                child.setProperty ("value", parameter->convertFrom0to1 (parameter->getDefaultValue()), nullptr);
                parameterTree.appendChild (child, nullptr);
            }
            parameters.replaceState (parameterTree);
        }
        const auto patch = root.getChildWithName ("PatchCables");
        if (patch.isValid())
        {
            resetPatch();
            for (int i = 0; i < anks::portCount; ++i)
            {
                const auto dst = static_cast<anks::Port> (i);
                if (anks::isOutput (dst)) continue;
                const auto key = "in" + juce::String (i);
                if (! patch.hasProperty (key)) continue;
                const int srcValue = static_cast<int> (patch.getProperty (key));
                if (srcValue < 0) { disconnect (dst); continue; }
                if (srcValue < anks::portCount) connect (static_cast<anks::Port> (srcValue), dst);
            }
        }
        if (version >= 6) restoreRack (freeRack);
        else setLegacyMode();
    }
}

juce::AudioProcessorEditor* ANKSModularProcessor::createEditor()
{ return isFreeRack() ? static_cast<juce::AudioProcessorEditor*> (new FreeRackEditor (*this)) : new ANKSModularEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ANKSModularProcessor(); }
