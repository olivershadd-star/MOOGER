#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace anks
{
struct ModulationParameters
{
    float rate = 1.0f;
    float atten1 = 0.5f, atten2 = 0.5f;
    float mix1 = 1.0f, mix2 = 1.0f, offset = 0.0f, level = 0.8f;
    float rise = 0.15f, fall = 0.15f;
    float pitchDepth = 2.0f, filterDepth = 1500.0f, ampDepth = 1.0f;
};

enum class ModulationInput { sample, clock, atten1, atten2, mix1, mix2, mix3, mix4, lag };

struct ModulationValues
{
    double triangle = -1.0, square = 1.0, noise = 0.0, held = 0.0;
    double atten1 = 0.0, atten2 = 0.0, mixed = 0.0, inverted = 0.0, lag = 0.0;
};

// Fixed feed-forward CV chain. The resolver supplies only upstream signals.
// All signals are normalized to [-1, 1]; these are not hardware voltages.
class ModulationEngine
{
public:
    void prepare (double sampleRate) noexcept { sr = std::max (8000.0, sampleRate); reset(); }
    void reset() noexcept
    {
        values = ModulationValues {};
        phase = 0.0; wasHigh = false; manualSample = false;
        seed = 0xa341316cu;
        gain1 = gain2 = 0.5; mixGain1 = mixGain2 = 1.0;
        mixOffset = 0.0; mixLevel = 0.8;
        lastRise = lastFall = -1.0f;
    }
    const ModulationValues& outputs() const noexcept { return values; }
    void requestSample() noexcept { manualSample = true; }

    template <typename Resolver>
    const ModulationValues& process (const ModulationParameters& p, Resolver&& input) noexcept
    {
        values.triangle = 1.0 - 4.0 * std::abs (phase - 0.5);
        values.square = phase < 0.5 ? 1.0 : -1.0;
        phase += std::clamp (double (p.rate), 0.05, 30.0) / sr;
        if (phase >= 1.0) phase -= 1.0;
        seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
        values.noise = double (seed) / 2147483647.5 - 1.0;

        const bool high = input (ModulationInput::clock) > 0.5;
        if ((high && ! wasHigh) || manualSample)
            values.held = bounded (input (ModulationInput::sample));
        wasHigh = high; manualSample = false;

        const double ramp = std::min (1.0, 1.0 / (sr * 0.005));
        gain1 += (unit (p.atten1) - gain1) * ramp;
        gain2 += (unit (p.atten2) - gain2) * ramp;
        mixGain1 += (unit (p.mix1) - mixGain1) * ramp;
        mixGain2 += (unit (p.mix2) - mixGain2) * ramp;
        mixOffset += (bounded (p.offset) - mixOffset) * ramp;
        mixLevel += (unit (p.level) - mixLevel) * ramp;
        values.atten1 = bounded (input (ModulationInput::atten1)) * gain1;
        values.atten2 = bounded (input (ModulationInput::atten2)) * gain2;
        values.mixed = bounded ((bounded (input (ModulationInput::mix1)) * mixGain1
                             + bounded (input (ModulationInput::mix2)) * mixGain2
                             + bounded (input (ModulationInput::mix3))
                             + bounded (input (ModulationInput::mix4)) + mixOffset) * mixLevel);
        values.inverted = -values.mixed;

        if (lastRise != p.rise)
        {
            lastRise = p.rise;
            riseAlpha = 1.0 - std::exp (-1.0 / (sr * std::clamp (double (p.rise), 0.001, 2.0)));
        }
        if (lastFall != p.fall)
        {
            lastFall = p.fall;
            fallAlpha = 1.0 - std::exp (-1.0 / (sr * std::clamp (double (p.fall), 0.001, 2.0)));
        }
        const double target = bounded (input (ModulationInput::lag));
        values.lag += (target - values.lag) * (target > values.lag ? riseAlpha : fallAlpha);
        values.lag = bounded (values.lag);
        return values;
    }

private:
    static double bounded (double value) noexcept { return std::isfinite (value) ? std::clamp (value, -1.0, 1.0) : 0.0; }
    static double unit (double value) noexcept { return std::isfinite (value) ? std::clamp (value, 0.0, 1.0) : 0.0; }
    double sr = 44100.0, phase = 0.0;
    double gain1 = 0.5, gain2 = 0.5, mixGain1 = 1.0, mixGain2 = 1.0, mixOffset = 0.0, mixLevel = 0.8;
    double riseAlpha = 0.0, fallAlpha = 0.0;
    float lastRise = -1, lastFall = -1;
    bool wasHigh = false, manualSample = false;
    std::uint32_t seed = 0xa341316cu;
    ModulationValues values;
};
} // namespace anks
