#pragma once
#include "RackModel.h"
#include "PercussionEngine.h"
#include <vector>

namespace mooger
{
using RackControls = std::array<std::array<float, maxParameters>, maxModules>;
struct Stereo { float left = 0, right = 0; };
class RackEngine
{
public:
    void prepare (double rate)
    {
        const auto existing = plan; const bool hadPlan = applied;
        if (! std::isfinite (rate)) rate = 44100;
        sr = std::clamp (rate, 8000.0, 384000.0);
        for (auto& buffer : delay) buffer.assign (size_t (sr * 1.6) + 2, 0);
        for (auto& p : percussion) p.prepare (sr);
        states = {}; previous = {}; current = {}; controls = {}; midiVelocity.fill (0); midiOrder.fill (0);
        dcInL = dcInR = dcOutL = dcOutR = 0; last = {}; fadeFrom = {}; fadeRemaining = 0;
        heldNote = 48; noteSerial = 0; applied = false;
        orderCount = 0; if (hadPlan) apply (existing);
    }
    void apply (const RackState& next) noexcept
    {
        const bool resetAll = ! applied || plan.generation != next.generation;
        bool changed = resetAll || plan.cableCount != next.cableCount;
        for (int i = 0; i < maxModules; ++i)
        {
            const auto& node = next.modules[size_t (i)]; const auto& before = plan.modules[size_t (i)];
            if (resetAll || node.active != before.active || node.uid != before.uid || node.type != before.type)
            {
                changed = true; states[size_t (i)] = Node {}; states[size_t (i)].seed ^= node.uid * 1664525u;
                previous[size_t (i)] = {}; current[size_t (i)] = {}; percussion[size_t (i)].reset();
            }
        }
        for (int i = 0; i < std::min (plan.cableCount, next.cableCount); ++i)
        {
            const auto a = plan.cables[size_t (i)], b = next.cables[size_t (i)];
            if (a.from != b.from || a.to != b.to || a.input != b.input || a.output != b.output) changed = true;
        }
        plan = next; applied = true;
        if (! changed) return; // moving modules never resets oscillators or envelopes
        for (auto& row : sources) for (auto& source : row) source = Source {};
        std::array<int, maxModules> indegree {};
        for (int i = 0; i < plan.cableCount; ++i)
        {
            const auto c = plan.cables[size_t (i)]; sources[size_t (c.to)][size_t (c.input)] = {c.from, c.output, false};
            ++indegree[size_t (c.to)];
        }
        std::array<bool, maxModules> done {}; orderCount = 0; outputNodes = 0;
        for (const auto& module : plan.modules) if (module.active && module.type == Type::output) ++outputNodes;
        for (;;)
        {
            int chosen = -1;
            for (int i = 0; i < maxModules; ++i)
                if (plan.modules[size_t (i)].active && ! done[size_t (i)] && indegree[size_t (i)] == 0) { chosen = i; break; }
            if (chosen < 0) break;
            order[size_t (orderCount++)] = chosen; done[size_t (chosen)] = true;
            for (int i = 0; i < plan.cableCount; ++i) if (plan.cables[size_t (i)].from == chosen) --indegree[size_t (plan.cables[size_t (i)].to)];
        }
        // Cyclic subgraphs remain causal: backwards edges read the previous sample.
        for (int i = 0; i < maxModules; ++i)
            if (plan.modules[size_t (i)].active && ! done[size_t (i)]) order[size_t (orderCount++)] = i;
        std::array<int, maxModules> rank {};
        for (int i = 0; i < orderCount; ++i) rank[size_t (order[size_t (i)])] = i;
        for (int to = 0; to < maxModules; ++to) for (auto& source : sources[size_t (to)])
            if (source.slot >= 0) source.delayed = rank[size_t (source.slot)] >= rank[size_t (to)];
        fadeFrom = last; fadeRemaining = std::max (1, int (sr * 0.002));
    }
    void setControls (const RackControls& normalized) noexcept
    {
        for (int n = 0; n < maxModules; ++n) if (plan.modules[size_t (n)].active)
        {
            const auto& d = info (plan.modules[size_t (n)].type);
            for (int i = 0; i < d.parameterCount; ++i)
            {
                const float value = normalized[size_t (n)][size_t (i)];
                controls[size_t (n)][size_t (i)] = d.parameters[size_t (i)].physical (std::isfinite (value) ? value : 0);
            }
        }
    }
    void noteOn (int note, float velocity) noexcept
    {
        if (note < 0 || note > 127) return;
        midiVelocity[size_t (note)] = std::clamp (velocity, 0.0f, 1.0f); midiOrder[size_t (note)] = ++noteSerial; heldNote = note;
    }
    void noteOff (int note) noexcept
    {
        if (note < 0 || note > 127) return;
        midiVelocity[size_t (note)] = 0;
        if (note == heldNote)
        {
            std::uint64_t newest = 0;
            for (int i = 0; i < 128; ++i) if (midiVelocity[size_t (i)] > 0 && midiOrder[size_t (i)] > newest)
            { newest = midiOrder[size_t (i)]; heldNote = i; }
        }
    }
    void allNotesOff() noexcept { midiVelocity.fill (0); }
    void trigger (int slot) noexcept { if (validSlot (slot)) states[size_t (slot)].manual = true; }
    int step (int slot) const noexcept { return validSlot (slot) ? states[size_t (slot)].step : -1; }
    double output (int slot, int port = 0) const noexcept { return validSlot (slot) && port >= 0 && port < maxPorts ? current[size_t (slot)][size_t (port)] : 0; }
    Stereo process() noexcept
    {
        double left = 0, right = 0;
        for (int index = 0; index < orderCount; ++index)
        {
            const int slot = order[size_t (index)]; auto& s = states[size_t (slot)]; auto& out = current[size_t (slot)]; out = {};
            const auto& p = controls[size_t (slot)];
            const auto input = [&] (int port, double fallback = 0.0) { const auto c = sources[size_t (slot)][size_t (port)];
                return c.slot < 0 ? fallback : (c.delayed ? previous : current)[size_t (c.slot)][size_t (c.port)]; };
            const auto patched = [&] (int port) { return sources[size_t (slot)][size_t (port)].slot >= 0; };
            const auto pitch = [&] (int port, double fallback) {
                const auto c = sources[size_t (slot)][size_t (port)];
                return c.slot < 0 ? fallback : info (plan.modules[size_t (c.slot)].type).outputs[size_t (c.port)].signal == Signal::pitch
                    ? input (port) : 48.0 + input (port) * 12.0; };
            const auto edge = [&] (double gate) { const bool high = gate > 0.1; const bool rising = high && ! s.gate; s.gate = high; return rising; };
            const double slew = std::min (1.0, 1.0 / (sr * 0.002));
            switch (plan.modules[size_t (slot)].type)
            {
                case Type::vco:
                {
                    const double target = 440.0 * std::pow (2.0, (pitch (0, 48) - 69.0 + p[1] + p[2] * 0.01 + input (1) * p[3]) / 12.0);
                    s.frequency += (std::clamp (target, 1.0, sr * 0.45) - s.frequency) * std::min (1.0, 1.0 / (sr * 0.005));
                    const double dt = std::clamp (s.frequency / sr, 1e-7, 0.45);
                    const double phase = s.phase;
                    switch (int (p[0]))
                    {
                        case 0: out[0] = std::sin (tau * phase); break;
                        case 1: out[0] = 4 * std::abs (phase - 0.5) - 1; break;
                        case 3: out[0] = (phase < 0.5 ? 1.0 : -1.0) + blep (phase, dt) - blep (std::fmod (phase + 0.5, 1.0), dt); break;
                        default: out[0] = 2 * phase - 1 - blep (phase, dt); break;
                    }
                    s.gain += (p[4] - s.gain) * slew; out[0] *= s.gain * 0.28;
                    s.phase += dt; if (s.phase >= 1) s.phase -= 1; break;
                }
                case Type::vcf:
                {
                    s.cutoff += (std::clamp (double (p[0]) + input (1) * p[2], 25.0, sr * 0.42) - s.cutoff) * slew;
                    const double g = std::tan (tau * 0.5 * s.cutoff / sr), k = 1.0 / p[1];
                    const double a = 1.0 / (1.0 + g * (g + k)), v3 = input (0) - s.filter2;
                    const double v1 = a * s.filter1 + g * a * v3, v2 = s.filter2 + g * a * s.filter1 + g * g * a * v3;
                    s.filter1 = bounded (2 * v1 - s.filter1, 16); s.filter2 = bounded (2 * v2 - s.filter2, 16);
                    out[0] = v2; out[1] = input (0) - k * v1 - v2; break;
                }
                case Type::vca:
                    s.gain += (std::clamp (input (1, 1) + p[1], 0.0, 1.0) * p[0] - s.gain) * slew;
                    out[0] = input (0) * s.gain; break;
                case Type::envelope:
                {
                    const bool before = s.gate; const bool rising = edge (input (0));
                    if (rising) { s.stage = 1; s.increment = (1 - s.envelope) / (p[0] * sr); }
                    if (before && ! s.gate) { s.stage = 4; s.increment = s.envelope / (p[3] * sr); }
                    if (s.stage == 1) { s.envelope += s.increment; if (s.envelope >= 1) { s.envelope = 1; s.stage = 2; s.increment = (1 - p[2]) / (p[1] * sr); } }
                    else if (s.stage == 2) { s.envelope -= s.increment; if (s.envelope <= p[2]) { s.envelope = p[2]; s.stage = 3; } }
                    else if (s.stage == 3) s.envelope += (p[2] - s.envelope) * slew;
                    else if (s.stage == 4) { s.envelope -= s.increment; if (s.envelope <= 0) { s.envelope = 0; s.stage = 0; } }
                    out[0] = std::clamp (s.envelope, 0.0, 1.0); break;
                }
                case Type::lfo:
                    out[0] = (1 - 4 * std::abs (s.phase - 0.5)) * p[1] + p[2];
                    out[1] = (s.phase < 0.5 ? 1.0 : -1.0) * p[1] + p[2];
                    out[2] = std::sin (tau * s.phase) * p[1] + p[2];
                    s.phase += std::clamp (p[0] + input (0) * p[3], 0.01, 50.0) / sr;
                    if (s.phase >= 1) s.phase -= 1; break;
                case Type::sequencer:
                {
                    bool advance = false;
                    if (p[1] > 0.5f)
                    {
                        if (! s.running) { s.step = -1; s.countdown = 0; }
                        if (patched (0)) advance = edge (input (0));
                        else if (s.countdown <= 0) { advance = true; s.countdown += sr * 30.0 / p[0]; }
                        s.countdown -= 1;
                        if (advance) { s.step = (s.step + 1) % 8; s.gateSamples = int (sr * 30.0 / p[0] * p[2]); }
                        if (s.gateSamples > 0) --s.gateSamples;
                    }
                    else { s.gateSamples = 0; s.step = -1; s.gate = false; }
                    s.running = p[1] > 0.5f;
                    out[0] = p[11] + (s.step >= 0 ? std::round (p[size_t (3 + s.step)]) : 0) + input (1) * 12;
                    out[1] = s.gateSamples > 0 ? 1 : 0; break;
                }
                case Type::mixer:
                    for (int port = 0; port < 4; ++port) out[0] += input (port) * p[size_t (port)];
                    out[0] *= p[4]; break;
                case Type::output:
                {
                    s.gain += (p[0] - s.gain) * slew;
                    const double pan = (p[1] + 1) * tau / 8;
                    left += input (0) * s.gain * std::cos (pan) * 1.41421356237;
                    right += input (1, input (0)) * s.gain * std::sin (pan) * 1.41421356237; break;
                }
                case Type::noise: out[0] = noise (s) * p[0] * 0.28; break;
                case Type::sampleHold:
                {
                    const bool internalClock = s.phase < 0.5; s.phase += p[0] / sr; if (s.phase >= 1) s.phase -= 1;
                    const bool capture = edge (input (1, internalClock ? 1 : 0));
                    const double signal = patched (0) ? input (0) : noise (s);
                    if (capture || s.manual) s.held = bounded (signal, 1);
                    out[0] = s.held * p[1]; break;
                }
                case Type::attenuator: s.gain += (p[0] - s.gain) * slew; out[0] = input (0) * s.gain; break;
                case Type::slew:
                {
                    const double target = input (0); const double alpha = 1 - std::exp (-1.0 / (sr * (target > s.held ? p[0] : p[1])));
                    s.held += (target - s.held) * alpha; out[0] = s.held; break;
                }
                case Type::percussion:
                {
                    anks::DrumParameters d; d.pitchHz = p[0]; d.ratio = p[1]; d.noise = p[2]; d.decay = p[3];
                    d.fm = p[4]; d.cutoff = p[5]; d.level = p[6]; d.run = p[7] > 0.5f && ! patched (0);
                    d.bpm = p[8]; d.followTempo = false; d.tone = p[9]; d.resonance = p[10]; d.waveform = int (p[11]);
                    if ((patched (0) && edge (input (0))) || s.manual) percussion[size_t (slot)].trigger (d, int (pitch (1, 48) - 48), 1);
                    out[0] = percussion[size_t (slot)].process (d, d.bpm); s.step = percussion[size_t (slot)].currentStep(); break;
                }
                case Type::delay:
                {
                    auto& buffer = delay[size_t (slot)];
                    const int distance = std::clamp (int (p[0] * sr), 1, int (buffer.size()) - 1);
                    int read = s.write - distance; if (read < 0) read += int (buffer.size());
                    const double wet = s.valid >= distance ? buffer[size_t (read)] : 0;
                    buffer[size_t (s.write)] = float (bounded (input (0) + wet * p[1], 4));
                    if (++s.write == int (buffer.size())) s.write = 0;
                    s.valid = std::min (s.valid + 1, int (buffer.size())); out[0] = input (0) * (1 - p[2]) + wet * p[2]; break;
                }
                case Type::midi: out[0] = heldNote; out[1] = midiVelocity[size_t (heldNote)] > 0 ? 1 : 0; out[2] = midiVelocity[size_t (heldNote)]; break;
                case Type::cvMixer:
                    out[0] = p[4]; for (int port = 0; port < 4; ++port) out[0] += input (port) * p[size_t (port)];
                    out[0] = bounded (out[0] * p[5], 1); out[1] = -out[0]; break;
                default: break;
            }
            s.manual = false;
            const auto& d = info (plan.modules[size_t (slot)].type);
            for (int port = 0; port < d.outputCount; ++port)
                out[size_t (port)] = d.outputs[size_t (port)].signal == Signal::pitch
                    ? std::clamp (bounded (out[size_t (port)], 127), 0.0, 127.0)
                    : bounded (out[size_t (port)], d.outputs[size_t (port)].signal == Signal::audio ? 4 : 1);
        }
        previous = current;
        // DC rejection also makes creative CV-to-audio patches safe for the output bus.
        const double dcAlpha = std::exp (-tau * 10.0 / sr);
        dcOutL = bounded (left - dcInL + dcAlpha * dcOutL, 64); dcInL = left;
        dcOutR = bounded (right - dcInR + dcAlpha * dcOutR, 64); dcInR = right;
        if (outputNodes == 0) dcInL = dcInR = dcOutL = dcOutR = 0;
        Stereo result {float (std::tanh (dcOutL * 1.5) * 0.8), float (std::tanh (dcOutR * 1.5) * 0.8)};
        if (fadeRemaining > 0)
        {
            const float blend = float (fadeRemaining--) / float (std::max (1, int (sr * 0.002)));
            result.left = result.left * (1 - blend) + fadeFrom.left * blend;
            result.right = result.right * (1 - blend) + fadeFrom.right * blend;
        }
        last = result; return result;
    }
private:
    static constexpr double tau = 6.28318530717958647692;
    struct Source { int slot = -1, port = 0; bool delayed = false; };
    struct Node
    {
        double phase = 0, frequency = 130.8128, gain = 0, cutoff = 1200, filter1 = 0, filter2 = 0;
        double envelope = 0, increment = 0, held = 0, countdown = 0;
        int stage = 0, step = -1, gateSamples = 0, write = 0, valid = 0;
        bool gate = false, running = false, manual = false;
        std::uint32_t seed = 0x9e3779b9u;
    };
    static double bounded (double value, double maximum) noexcept { return std::isfinite (value) ? std::clamp (value, -maximum, maximum) : 0; }
    static double noise (Node& s) noexcept { s.seed ^= s.seed << 13; s.seed ^= s.seed >> 17; s.seed ^= s.seed << 5; return double (s.seed) / 2147483647.5 - 1; }
    static double blep (double t, double dt) noexcept { if (t < dt) { t /= dt; return t + t - t * t - 1; } if (t > 1 - dt) { t = (t - 1) / dt; return t * t + t + t + 1; } return 0; }
    double sr = 44100, dcInL = 0, dcInR = 0, dcOutL = 0, dcOutR = 0;
    RackState plan;
    RackControls controls {};
    std::array<Node, maxModules> states {};
    std::array<std::array<Source, maxPorts>, maxModules> sources {};
    std::array<std::array<double, maxPorts>, maxModules> current {}, previous {};
    std::array<int, maxModules> order {};
    std::array<anks::PercussionEngine, maxModules> percussion;
    std::array<std::vector<float>, maxModules> delay;
    std::array<float, 128> midiVelocity {};
    std::array<std::uint64_t, 128> midiOrder {};
    int orderCount = 0, outputNodes = 0, heldNote = 48, fadeRemaining = 0;
    std::uint64_t noteSerial = 0;
    bool applied = false;
    Stereo last {}, fadeFrom {};
};
} // namespace mooger
