#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <cstring>

namespace mooger
{
constexpr int maxModules = 32, maxParameters = 16, maxPorts = 4, maxCables = 128;
enum class Type { vco, vcf, vca, envelope, lfo, sequencer, mixer, output, noise,
                  sampleHold, attenuator, slew, percussion, delay, midi, cvMixer, count };
enum class Signal { audio, cv, pitch, gate };
enum class ParamKind { continuous, toggle, choice };
struct Parameter
{
    const char* name = "";
    float low = 0, high = 1, initial = 0;
    bool logarithmic = false;
    int decimals = 2;
    ParamKind kind = ParamKind::continuous;
    const char* choices = "";
    float physical (float value) const noexcept
    {
        value = std::clamp (value, 0.0f, 1.0f);
        const float result = logarithmic ? low * std::pow (high / low, value) : low + (high - low) * value;
        return kind == ParamKind::choice ? std::round (result) : result;
    }
    float normalized (float value) const noexcept
    {
        value = std::clamp (value, low, high);
        return logarithmic ? std::log (value / low) / std::log (high / low) : (value - low) / (high - low);
    }
};
struct Port { const char* name = ""; Signal signal = Signal::audio; };
struct Definition
{
    const char* id = "";
    const char* name = "";
    std::array<Parameter, maxParameters> parameters {};
    std::array<Port, maxPorts> inputs {}, outputs {};
    int parameterCount = 0, inputCount = 0, outputCount = 0;
    bool trigger = false;
};
inline Parameter p (const char* name, float lo, float hi, float initial, int decimals = 2, bool log = false)
{ return {name, lo, hi, initial, log, decimals, ParamKind::continuous, ""}; }
inline Parameter toggle (const char* name, bool initial = false)
{ return {name, 0, 1, initial ? 1.0f : 0.0f, false, 0, ParamKind::toggle, ""}; }
inline Parameter choice (const char* name, const char* names, int maximum, int initial = 0)
{ return {name, 0, float (maximum), float (initial), false, 0, ParamKind::choice, names}; }
inline Definition definition (const char* id, const char* name, std::initializer_list<Parameter> parameters,
                              std::initializer_list<Port> inputs, std::initializer_list<Port> outputs, bool trigger = false)
{
    Definition d; d.id = id; d.name = name; d.trigger = trigger;
    for (auto value : parameters) d.parameters[static_cast<size_t> (d.parameterCount++)] = value;
    for (auto value : inputs) d.inputs[static_cast<size_t> (d.inputCount++)] = value;
    for (auto value : outputs) d.outputs[static_cast<size_t> (d.outputCount++)] = value;
    return d;
}
inline const std::array<Definition, static_cast<int> (Type::count)>& catalog()
{
    static const std::array<Definition, static_cast<int> (Type::count)> bank {{
        definition ("vco", "VCO", {choice ("ONDE", "SIN|TRI|SAW|SQR", 3, 2), p ("ACCORD", -24, 24, 0, 1),
            p ("FIN cents", -50, 50, 0, 1), p ("MOD demi-t", 0, 48, 12, 1), p ("NIVEAU", 0, 1, 0.7f)},
            {{"PITCH", Signal::pitch}, {"MOD / FM", Signal::cv}}, {{"AUDIO", Signal::audio}}),
        definition ("vcf", "VCF", {p ("CUTOFF Hz", 25, 18000, 1200, 0, true), p ("RESONANCE", 0.5f, 9, 1.2f),
            p ("CV Hz", 0, 8000, 2000, 0)}, {{"AUDIO", Signal::audio}, {"CUTOFF CV", Signal::cv}},
            {{"LOW PASS", Signal::audio}, {"HIGH PASS", Signal::audio}}),
        definition ("vca", "VCA", {p ("NIVEAU", 0, 1, 0.7f), p ("BIAS", 0, 1, 0)},
            {{"AUDIO", Signal::audio}, {"GAIN CV", Signal::cv}}, {{"AUDIO", Signal::audio}}),
        definition ("adsr", "ENV ADSR", {p ("ATTACK s", 0.002f, 2, 0.012f, 3, true), p ("DECAY s", 0.003f, 3, 0.12f, 3, true),
            p ("SUSTAIN", 0, 1, 0.15f), p ("RELEASE s", 0.003f, 4, 0.14f, 3, true)},
            {{"GATE", Signal::gate}}, {{"ENVELOPPE", Signal::cv}}),
        definition ("lfo", "LFO", {p ("RATE Hz", 0.05f, 30, 2, 2, true), p ("PROFONDEUR", 0, 1, 1),
            p ("OFFSET", -1, 1, 0), p ("RATE CV", 0, 10, 2)}, {{"RATE CV", Signal::cv}},
            {{"TRIANGLE", Signal::cv}, {"CARRE", Signal::cv}, {"SINUS", Signal::cv}}),
        definition ("seq8", "SEQ 8", {p ("BPM", 40, 240, 110, 0), toggle ("RUN"), p ("GATE", 0.05f, 0.95f, 0.6f),
            p ("PAS 1", -24, 24, 0, 0), p ("PAS 2", -24, 24, 0, 0), p ("PAS 3", -24, 24, 3, 0),
            p ("PAS 4", -24, 24, 7, 0), p ("PAS 5", -24, 24, 0, 0), p ("PAS 6", -24, 24, 5, 0),
            p ("PAS 7", -24, 24, 3, 0), p ("PAS 8", -24, 24, 10, 0), p ("BASE MIDI", 24, 84, 48, 0)},
            {{"CLOCK", Signal::gate}, {"TRANSPOSE", Signal::cv}}, {{"PITCH", Signal::pitch}, {"GATE", Signal::gate}}),
        definition ("mixer", "MIX AUDIO", {p ("IN 1", 0, 1, 0.6f), p ("IN 2", 0, 1, 0.6f), p ("IN 3", 0, 1, 0.6f),
            p ("IN 4", 0, 1, 0.6f), p ("MASTER", 0, 1, 0.8f)},
            {{"IN 1", Signal::audio}, {"IN 2", Signal::audio}, {"IN 3", Signal::audio}, {"IN 4", Signal::audio}}, {{"MIX", Signal::audio}}),
        definition ("output", "SORTIE AUDIO", {p ("VOLUME", 0, 1, 0.5f), p ("PAN", -1, 1, 0)},
            {{"LEFT / MONO", Signal::audio}, {"RIGHT", Signal::audio}}, {}),
        definition ("noise", "BRUIT", {p ("NIVEAU", 0, 1, 0.5f)}, {}, {{"NOISE", Signal::audio}}),
        definition ("sample-hold", "SAMPLE & HOLD", {p ("RATE Hz", 0.05f, 30, 2, 2, true), p ("NIVEAU", 0, 1, 1)},
            {{"SIGNAL", Signal::cv}, {"CLOCK", Signal::gate}}, {{"S&H", Signal::cv}}, true),
        definition ("attenuator", "ATTENUVERT.", {p ("GAIN", -1, 1, 0.5f)}, {{"CV / AUDIO", Signal::cv}}, {{"OUT", Signal::cv}}),
        definition ("slew", "LISSAGE", {p ("RISE s", 0.001f, 2, 0.15f, 3, true), p ("FALL s", 0.001f, 2, 0.15f, 3, true)},
            {{"SIGNAL", Signal::cv}}, {{"LAG", Signal::cv}}),
        definition ("percussion", "PERCUSSION", {p ("BASE Hz", 25, 800, 65.4f, 1, true), p ("VCO2 RATIO", 0.25f, 8, 1.5f),
            p ("BRUIT", 0, 1, 0.15f), p ("DECAY s", 0.02f, 2, 0.22f, 3, true), p ("FM", 0, 4, 0),
            p ("CUTOFF Hz", 40, 16000, 3500, 0, true), p ("NIVEAU", 0, 1, 0.55f), toggle ("RUN"),
            p ("BPM", 40, 240, 110, 0), p ("VCO2 MIX", 0, 1, 0.35f), p ("RESONANCE", 0.5f, 6, 0.8f),
            choice ("ONDE", "TRI|SQR", 1)}, {{"TRIGGER", Signal::gate}, {"PITCH", Signal::pitch}}, {{"AUDIO", Signal::audio}}, true),
        definition ("delay", "DELAY", {p ("TEMPS s", 0.01f, 1.5f, 0.25f, 3, true), p ("FEEDBACK", 0, 0.9f, 0.3f),
            p ("MIX", 0, 1, 0.35f)}, {{"AUDIO", Signal::audio}}, {{"AUDIO", Signal::audio}}),
        definition ("midi", "MIDI", {}, {}, {{"PITCH", Signal::pitch}, {"GATE", Signal::gate}, {"VELOCITE", Signal::cv}}),
        definition ("cv-mixer", "MIX CV", {p ("IN 1", 0, 1, 1), p ("IN 2", 0, 1, 1), p ("IN 3", 0, 1, 1),
            p ("IN 4", 0, 1, 1), p ("OFFSET", -1, 1, 0), p ("NIVEAU", 0, 1, 0.8f)},
            {{"IN 1", Signal::cv}, {"IN 2", Signal::cv}, {"IN 3", Signal::cv}, {"IN 4", Signal::cv}},
            {{"MIX +", Signal::cv}, {"MIX -", Signal::cv}})
    }};
    return bank;
}
inline bool validType (Type type) noexcept { return int (type) >= 0 && int (type) < int (Type::count); }
inline const Definition& info (Type type) noexcept { return catalog()[static_cast<size_t> (type)]; }
inline Type findType (const char* id) noexcept
{
    for (int i = 0; i < int (Type::count); ++i) if (std::strcmp (catalog()[size_t (i)].id, id) == 0) return Type (i);
    return Type::count;
}
} // namespace mooger
