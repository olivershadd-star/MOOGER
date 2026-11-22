#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace anks
{
struct DrumStep
{
    int pitch = 0;
    float velocity = 1.0f;
};

struct DrumParameters
{
    bool run = false, followTempo = true;
    float bpm = 110.0f;
    float pitchHz = 65.4064f, ratio = 1.5f, tone = 0.35f;
    float noise = 0.15f, fm = 0.0f;
    float decay = 0.22f, pitchDecay = 0.04f, pitchDepth = 24.0f;
    float cutoff = 3500.0f, resonance = 0.8f, level = 0.55f;
    int waveform = 0; // triangle / band-limited square
    std::array<DrumStep, 8> steps {{ {0,1}, {7,0.35f}, {-5,0.7f}, {12,0.45f},
                                   {0,1}, {5,0.35f}, {-12,0.8f}, {7,0.5f} }};
};

// Independent percussion voice and sample-driven sixteenth-note sequencer.
// Original DSP inspired by the role of DFAM; not a circuit emulation.
class PercussionEngine
{
public:
    void prepare (double rate) noexcept { sr = std::max (8000.0, rate); reset(); }
    void reset() noexcept
    {
        phase1 = phase2 = amp = pitchEnvelope = 0.0;
        filter1 = filter2 = 0.0;
        smoothLevel = 0.0; smoothCutoff = 3500.0; smoothFrequency = 65.4064;
        samplesToStep = 0.0; stepRatio = 1.0; peak = 0.0;
        cachedDecay = cachedPitchDecay = -1.0f;
        attackSamples = 0; stepIndex = -1; wasRunning = false;
        seed = 0x6d2b79f5u;
    }
    int currentStep() const noexcept { return stepIndex; }

    void trigger (const DrumParameters&, int pitch = 0, float velocity = 1.0f) noexcept
    {
        velocity = std::clamp (velocity, 0.0f, 1.0f);
        if (velocity == 0.0f) return; // a rest lets the previous hit decay
        stepRatio = std::pow (2.0, std::clamp (pitch, -24, 24) / 12.0);
        peak = velocity;
        attackSamples = std::max (1, static_cast<int> (sr * 0.0015));
        pitchEnvelope = velocity;
        // Keep oscillator phases and amplitude continuous on retrigger.
    }

    float process (const DrumParameters& p, float coreBpm) noexcept
    {
        if (p.run && ! wasRunning) { samplesToStep = 0; stepIndex = -1; }
        if (! p.run && wasRunning) stepIndex = -1;
        wasRunning = p.run;
        if (p.run)
        {
            if (samplesToStep <= 0.0)
            {
                stepIndex = (stepIndex + 1) % 8;
                const auto& step = p.steps[static_cast<size_t> (stepIndex)];
                trigger (p, step.pitch, step.velocity);
                const double bpm = std::clamp (double (p.followTempo ? coreBpm : p.bpm), 40.0, 240.0);
                samplesToStep += sr * 15.0 / bpm;
            }
            samplesToStep -= 1.0;
        }

        if (cachedDecay != p.decay)
        {
            cachedDecay = p.decay;
            ampCoefficient = std::exp (-6.90775527898 / (sr * std::clamp (double (p.decay), 0.02, 2.0)));
        }
        if (cachedPitchDecay != p.pitchDecay)
        {
            cachedPitchDecay = p.pitchDecay;
            pitchCoefficient = std::exp (-6.90775527898 / (sr * std::clamp (double (p.pitchDecay), 0.005, 1.0)));
        }
        if (attackSamples > 0)
        {
            amp += (peak - amp) / attackSamples;
            --attackSamples;
        }
        else amp *= ampCoefficient;
        if (amp < 1e-8) amp = 0.0;
        pitchEnvelope *= pitchCoefficient;
        if (pitchEnvelope < 1e-8) pitchEnvelope = 0.0;

        const double controlSlew = std::min (1.0, 1.0 / (sr * 0.005));
        const double base = std::clamp (double (p.pitchHz), 25.0, 800.0) * stepRatio;
        const double frequency = base * std::pow (2.0, pitchEnvelope * std::clamp (double (p.pitchDepth), 0.0, 48.0) / 12.0);
        smoothFrequency += (frequency - smoothFrequency) * controlSlew;
        const double dt2 = std::clamp (smoothFrequency * std::clamp (double (p.ratio), 0.25, 8.0) / sr, 1e-7, 0.45);
        const double osc2 = wave (phase2, dt2, p.waveform);
        const double dt1 = std::clamp (smoothFrequency * (1.0 + osc2 * std::clamp (double (p.fm), 0.0, 4.0)) / sr, 1e-7, 0.45);
        const double osc1 = wave (phase1, dt1, p.waveform);
        phase1 += dt1; if (phase1 >= 1.0) phase1 -= 1.0;
        phase2 += dt2; if (phase2 >= 1.0) phase2 -= 1.0;

        seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
        const double noise = double (seed) / 2147483647.5 - 1.0;
        const double input = (osc1 * 0.65 + osc2 * std::clamp (double (p.tone), 0.0, 1.0)
                            + noise * std::clamp (double (p.noise), 0.0, 1.0)) * 0.22;
        smoothCutoff += (std::clamp (double (p.cutoff), 40.0, sr * 0.42) - smoothCutoff) * controlSlew;
        const double g = std::tan (3.141592653589793 * std::clamp (smoothCutoff, 40.0, sr * 0.42) / sr);
        const double k = 1.0 / std::clamp (double (p.resonance), 0.5, 6.0);
        const double a1 = 1.0 / (1.0 + g * (g + k));
        const double v3 = input - filter2;
        const double v1 = a1 * filter1 + g * a1 * v3;
        const double v2 = filter2 + g * a1 * filter1 + g * g * a1 * v3;
        filter1 = 2.0 * v1 - filter1; filter2 = 2.0 * v2 - filter2;
        if (! std::isfinite (filter1) || ! std::isfinite (filter2)) filter1 = filter2 = 0.0;
        smoothLevel += (std::clamp (double (p.level), 0.0, 1.0) - smoothLevel)
                       * std::min (1.0, 1.0 / (sr * 0.010));
        const double output = std::clamp (v2, -4.0, 4.0) * amp * smoothLevel;
        return std::isfinite (output) ? static_cast<float> (output) : 0.0f;
    }

private:
    static double blep (double t, double dt) noexcept
    {
        if (t < dt) { t /= dt; return t + t - t * t - 1.0; }
        if (t > 1.0 - dt) { t = (t - 1.0) / dt; return t * t + t + t + 1.0; }
        return 0.0;
    }
    static double wave (double phase, double dt, int shape) noexcept
    {
        if (shape == 0) return 4.0 * std::abs (phase - 0.5) - 1.0;
        return (phase < 0.5 ? 1.0 : -1.0) + blep (phase, dt) - blep (std::fmod (phase + 0.5, 1.0), dt);
    }
    double sr = 44100.0, phase1 = 0.0, phase2 = 0.0;
    double amp = 0.0, peak = 0.0, pitchEnvelope = 0.0;
    double ampCoefficient = 0.0, pitchCoefficient = 0.0;
    float cachedDecay = -1, cachedPitchDecay = -1;
    double filter1 = 0.0, filter2 = 0.0;
    double smoothLevel = 0.0, smoothCutoff = 3500, smoothFrequency = 65.4064;
    double samplesToStep = 0.0, stepRatio = 1.0;
    int attackSamples = 0, stepIndex = -1;
    bool wasRunning = false;
    uint32_t seed = 0x6d2b79f5u;
};
} // namespace anks
