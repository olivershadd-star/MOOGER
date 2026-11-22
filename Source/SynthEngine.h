#pragma once

// MOOGER - independent real-time DSP core.
// No dynamic allocation, file I/O, UI calls, locks or timers in this code.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include "PercussionEngine.h"

namespace anks
{
constexpr double pi = 3.14159265358979323846264338327950288;

enum class Port : int
{
    seqPitch, seqGate, oscPitch, oscOut, filterIn, filterOut,
    filterCV, vcaIn, vcaCV, vcaOut, envGate, envCV, lfoCV,
    outputIn,
    // Append ports to preserve saved V0.2 cable indices.
    osc2Pitch, osc2Out, mixerIn1, mixerIn2, mixerOut,
    drumOut, drumMixIn, count
};
constexpr int portCount = static_cast<int> (Port::count);
constexpr int index (Port p) noexcept { return static_cast<int> (p); }
constexpr bool isOutput (Port p) noexcept
{
    switch (p)
    {
        case Port::seqPitch: case Port::seqGate: case Port::oscOut:
        case Port::filterOut: case Port::vcaOut: case Port::envCV:
        case Port::lfoCV: case Port::osc2Out: case Port::mixerOut: case Port::drumOut: return true;
        default: return false;
    }
}
constexpr bool validConnection (Port src, Port dst) noexcept
{
    if (! isOutput (src)) return false;
    switch (dst)
    {
        case Port::oscPitch: case Port::osc2Pitch: return src == Port::seqPitch;
        case Port::envGate: return src == Port::seqGate;
        case Port::mixerIn1: case Port::mixerIn2:
            return src == Port::oscOut || src == Port::osc2Out || src == Port::drumOut;
        case Port::filterIn:
            return src == Port::oscOut || src == Port::osc2Out || src == Port::mixerOut || src == Port::drumOut;
        case Port::filterCV: return src == Port::lfoCV || src == Port::envCV;
        case Port::vcaIn:
            return src == Port::oscOut || src == Port::osc2Out || src == Port::mixerOut || src == Port::filterOut || src == Port::drumOut;
        case Port::vcaCV: return src == Port::envCV || src == Port::lfoCV;
        case Port::outputIn:
            return src == Port::vcaOut || src == Port::filterOut || src == Port::oscOut
                || src == Port::osc2Out || src == Port::mixerOut || src == Port::drumOut;
        case Port::drumMixIn: return src == Port::drumOut;
        default: return false;
    }
}

struct Routes
{
    std::array<int, portCount> source {};
    Routes() noexcept { source.fill (-1); reset(); }
    void reset() noexcept
    {
        source.fill (-1);
        connect (Port::seqPitch, Port::oscPitch);
        connect (Port::seqPitch, Port::osc2Pitch);
        connect (Port::seqGate, Port::envGate);
        connect (Port::oscOut, Port::mixerIn1);
        connect (Port::osc2Out, Port::mixerIn2);
        connect (Port::mixerOut, Port::filterIn);
        connect (Port::filterOut, Port::vcaIn);
        connect (Port::envCV, Port::vcaCV);
        connect (Port::vcaOut, Port::outputIn);
        connect (Port::drumOut, Port::drumMixIn);
    }
    bool connect (Port src, Port dst) noexcept
    {
        if (! validConnection (src, dst)) return false;
        source[index (dst)] = index (src);
        return true;
    }
    void disconnect (Port dst) noexcept
    {
        if (dst != Port::count && ! isOutput (dst)) source[index (dst)] = -1;
    }
    bool connected (Port src, Port dst) const noexcept
    {
        return source[index (dst)] == index (src);
    }
};

struct Step
{
    int semitones = 0;
    bool enabled = true;
};

struct Parameters
{
    float bpm = 110.0f;
    int waveform = 2; // 0 sine, 1 triangle, 2 saw, 3 square
    float tune = 0.0f;
    int waveform2 = 2;
    float tune2 = 0.0f, fine2 = 7.0f; // semitones / cents
    float mix1 = 0.65f, mix2 = 0.35f, mixLevel = 0.8f;
    float cutoff = 1200.0f;
    float resonance = 1.2f;  // Q
    float level = 0.65f;
    float attack = 0.012f, decay = 0.12f, sustain = 0.15f, release = 0.14f;
    float lfoRate = 2.0f, lfoDepth = 500.0f; // cutoff Hz
    float master = 0.42f;
    bool seqMode = true;
    bool run = false;
    DrumParameters drums;
    std::array<Step, 8> steps { Step {0,true}, Step {0,true}, Step {3,true}, Step {7,true},
                                Step {0,true}, Step {5,true}, Step {3,true}, Step {10,true} };
};

class Envelope
{
public:
    void reset() noexcept { stage = Stage::idle; level = 0.0; increment = 0.0; }
    void on (const Parameters& p, double sr) noexcept
    {
        stage = Stage::attack;
        increment = (1.0 - level) / (std::max (0.002f, p.attack) * sr);
    }
    void off (const Parameters& p, double sr) noexcept
    {
        if (stage == Stage::idle) return;
        stage = Stage::release;
        increment = level / (std::max (0.003f, p.release) * sr);
    }
    float next (const Parameters& p, double sr) noexcept
    {
        switch (stage)
        {
            case Stage::attack:
                level += increment;
                if (level >= 1.0)
                {
                    level = 1.0;
                    stage = Stage::decay;
                    increment = (1.0 - p.sustain) / (std::max (0.003f, p.decay) * sr);
                }
                break;
            case Stage::decay:
                level -= increment;
                if (level <= p.sustain) { level = p.sustain; stage = Stage::sustain; }
                break;
            case Stage::sustain:
                level += (p.sustain - level) * 0.0005;
                break;
            case Stage::release:
                level -= increment;
                if (level <= 0.0) reset();
                break;
            case Stage::idle: break;
        }
        level = std::clamp (level, 0.0, 1.0);
        return static_cast<float> (level);
    }
private:
    enum class Stage { idle, attack, decay, sustain, release };
    Stage stage = Stage::idle;
    double level = 0.0, increment = 0.0;
};

class Engine
{
public:
    void prepare (double newRate) noexcept
    {
        sr = std::max (8000.0, newRate);
        percussion.prepare (sr);
        reset();
    }
    void reset() noexcept
    {
        phase = phase2 = lfoPhase = 0.0;
        filt1 = filt2 = 0.0;
        envelope.reset();
        percussion.reset();
        currentFrequency = targetFrequency = 130.81278265;
        currentFrequency2 = 130.81278265;
        midiPitch = 48;
        smoothMaster = 0.0;
        smoothMix1 = smoothMix2 = smoothMixLevel = 0.0;
        samplesToStep = 0.0;
        gateSamples = 0;
        sequenceIndex = -1;
        lastRunning = false;
    }
    int getCurrentStep() const noexcept { return sequenceIndex; }
    int getDrumStep() const noexcept { return percussion.currentStep(); }
    void triggerDrum (const Parameters& p) noexcept { percussion.trigger (p.drums); }
    void noteOn (int midiNote, const Parameters& p, const Routes& r) noexcept
    {
        midiNote = std::clamp (midiNote, 0, 127);
        midiPitch = midiNote;
        if (r.source[index (Port::envGate)] == index (Port::seqGate))
            envelope.on (p, sr);
    }
    void noteOff (const Parameters& p, const Routes& r) noexcept
    {
        if (r.source[index (Port::envGate)] == index (Port::seqGate))
            envelope.off (p, sr);
    }
    void startSequence() noexcept
    {
        samplesToStep = 0.0;
        gateSamples = 0;
        sequenceIndex = -1;
        lastRunning = false;
    }
    float process (const Parameters& p, const Routes& r) noexcept
    {
        const double drumVoice = percussion.process (p.drums, p.bpm);
        const bool runSequence = p.run && p.seqMode;
        if (runSequence && ! lastRunning) startSequence();
        if (! runSequence && lastRunning)
        {
            envelope.off (p, sr);
            sequenceIndex = -1;
            gateSamples = 0;
        }
        lastRunning = runSequence;

        if (runSequence)
        {
            if (samplesToStep <= 0.0)
            {
                sequenceIndex = (sequenceIndex + 1) % 8;
                const auto& step = p.steps[static_cast<size_t> (sequenceIndex)];
                if (step.enabled)
                {
                    const int midi = 48 + step.semitones;
                    noteOn (midi, p, r);
                    gateSamples = static_cast<int> (std::max (1.0, stepLength (p) * 0.64));
                }
                else { gateSamples = 0; noteOff (p, r); }
                samplesToStep += stepLength (p);
            }
            samplesToStep -= 1.0;
            if (gateSamples > 0 && --gateSamples == 0) noteOff (p, r);
        }

        const double note1 = r.connected (Port::seqPitch, Port::oscPitch) ? midiPitch : 48;
        const double note2 = r.connected (Port::seqPitch, Port::osc2Pitch) ? midiPitch : 48;
        targetFrequency = 440.0 * std::pow (2.0, (note1 - 69.0 + p.tune) / 12.0);
        const double target2 = 440.0 * std::pow (2.0, (note2 - 69.0 + p.tune2 + p.fine2 * 0.01) / 12.0);
        currentFrequency += (targetFrequency - currentFrequency) * std::min (1.0, 1.0 / (0.005 * sr));
        currentFrequency2 += (target2 - currentFrequency2) * std::min (1.0, 1.0 / (0.005 * sr));
        const double dt = std::clamp (currentFrequency / sr, 0.0000001, 0.49);
        const double source = oscillator (p.waveform, phase, dt) * 0.28;
        phase += dt;
        if (phase >= 1.0) phase -= 1.0;
        const double dt2 = std::clamp (currentFrequency2 / sr, 0.0000001, 0.49);
        const double source2 = oscillator (p.waveform2, phase2, dt2) * 0.28;
        phase2 += dt2;
        if (phase2 >= 1.0) phase2 -= 1.0;

        // Only oscillator sources can feed the mixer: no feedback or order ambiguity.
        const auto oscillatorInput = [&] (Port input) noexcept
        {
            switch (r.source[index (input)])
            {
                case index (Port::oscOut): return source;
                case index (Port::osc2Out): return source2;
                case index (Port::drumOut): return drumVoice;
                default: return 0.0;
            }
        };
        const double ramp = std::min (1.0, 1.0 / (sr * 0.010));
        smoothMix1 += (std::clamp (static_cast<double> (p.mix1), 0.0, 1.0) - smoothMix1) * ramp;
        smoothMix2 += (std::clamp (static_cast<double> (p.mix2), 0.0, 1.0) - smoothMix2) * ramp;
        smoothMixLevel += (std::clamp (static_cast<double> (p.mixLevel), 0.0, 1.0) - smoothMixLevel) * ramp;
        const double mixed = (oscillatorInput (Port::mixerIn1) * smoothMix1
                            + oscillatorInput (Port::mixerIn2) * smoothMix2) * smoothMixLevel;

        const double lfo = std::sin (2.0 * pi * lfoPhase);
        lfoPhase += std::max (0.01f, p.lfoRate) / sr;
        if (lfoPhase >= 1.0) lfoPhase -= 1.0;
        const float env = envelope.next (p, sr);

        double effectiveCutoff = p.cutoff;
        switch (r.source[index (Port::filterCV)])
        {
            case index (Port::lfoCV): effectiveCutoff += lfo * p.lfoDepth; break;
            case index (Port::envCV): effectiveCutoff += env * std::max (0.0f, p.lfoDepth); break;
            default: break;
        }
        const auto preFilterInput = [&] (Port input) noexcept
        {
            return r.connected (Port::mixerOut, input) ? mixed : oscillatorInput (input);
        };
        const double filterInput = preFilterInput (Port::filterIn);
        const double filtered = lowpass (filterInput, std::clamp (effectiveCutoff, 25.0, sr * 0.42), p.resonance);
        const double vcaInput = r.connected (Port::filterOut, Port::vcaIn)
                              ? filtered : preFilterInput (Port::vcaIn);
        double cv = 1.0;
        switch (r.source[index (Port::vcaCV)])
        {
            case index (Port::envCV): cv = env; break;
            case index (Port::lfoCV): cv = 0.5 + 0.5 * lfo; break;
            default: break;
        }
        const double vca = vcaInput * std::clamp (cv, 0.0, 1.0) * p.level;
        double audioOut = 0.0;
        switch (r.source[index (Port::outputIn)])
        {
            case index (Port::vcaOut): audioOut = vca; break;
            case index (Port::filterOut): audioOut = filtered; break;
            default: audioOut = preFilterInput (Port::outputIn); break;
        }
        smoothMaster += (std::clamp (static_cast<double> (p.master), 0.0, 1.0) - smoothMaster)
                        * std::min (1.0, 1.0 / (sr * 0.010));
        const double drumBus = r.connected (Port::drumOut, Port::drumMixIn) ? drumVoice : 0.0;
        const double limited = std::tanh ((audioOut + drumBus) * smoothMaster * 1.5) * 0.80;
        return std::isfinite (limited) ? static_cast<float> (limited) : 0.0f;
    }

private:
    double stepLength (const Parameters& p) const noexcept
    {
        return sr * 30.0 / std::clamp (static_cast<double> (p.bpm), 40.0, 240.0); // eighths
    }
    static double polyBlep (double t, double dt) noexcept
    {
        if (t < dt) { t /= dt; return t + t - t * t - 1.0; }
        if (t > 1.0 - dt) { t = (t - 1.0) / dt; return t * t + t + t + 1.0; }
        return 0.0;
    }
    static double oscillator (int wave, double p, double dt) noexcept
    {
        switch (wave)
        {
            case 0: return std::sin (2.0 * pi * p);
            case 1: return 4.0 * std::abs (p - 0.5) - 1.0;
            case 3:
            {
                double s = p < 0.5 ? 1.0 : -1.0;
                s += polyBlep (p, dt);
                s -= polyBlep (std::fmod (p + 0.5, 1.0), dt);
                return s;
            }
            default: return 2.0 * p - 1.0 - polyBlep (p, dt);
        }
    }
    double lowpass (double in, double hz, double q) noexcept
    {
        // Topology-preserving state variable filter. Stable near self-oscillation.
        const double g = std::tan (pi * hz / sr);
        const double k = 1.0 / std::clamp (q, 0.5, 9.0);
        const double a1 = 1.0 / (1.0 + g * (g + k));
        const double a2 = g * a1;
        const double a3 = g * a2;
        const double v3 = in - filt2;
        const double v1 = a1 * filt1 + a2 * v3;
        const double v2 = filt2 + a2 * filt1 + a3 * v3;
        filt1 = 2.0 * v1 - filt1;
        filt2 = 2.0 * v2 - filt2;
        if (! std::isfinite (filt1) || ! std::isfinite (filt2)) filt1 = filt2 = 0.0;
        return std::clamp (v2, -8.0, 8.0);
    }

    double sr = 44100.0;
    double phase = 0.0, phase2 = 0.0, lfoPhase = 0.0;
    double filt1 = 0.0, filt2 = 0.0;
    double currentFrequency = 130.81278265, targetFrequency = 130.81278265;
    double currentFrequency2 = 130.81278265;
    double smoothMix1 = 0.0, smoothMix2 = 0.0, smoothMixLevel = 0.0;
    double smoothMaster = 0.0;
    double samplesToStep = 0.0;
    int gateSamples = 0, sequenceIndex = -1, midiPitch = 48;
    bool lastRunning = false;
    Envelope envelope;
    PercussionEngine percussion;
};
} // namespace anks
