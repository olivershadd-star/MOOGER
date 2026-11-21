#include "../Source/SynthEngine.h"
#include <cstdlib>
#include <iostream>

namespace
{
void require (bool ok, const char* message)
{
    if (! ok) { std::cerr << "FAIL: " << message << '\n'; std::exit (EXIT_FAILURE); }
}

double energy (anks::Engine& e, const anks::Parameters& p, const anks::Routes& r, int count)
{
    double sum = 0.0;
    for (int i = 0; i < count; ++i)
    {
        const float y = e.process (p, r);
        require (std::isfinite (y) && std::abs (y) < 1.0f, "invalid module output");
        sum += y * y;
    }
    return sum;
}

double frequency (anks::Engine& e, const anks::Parameters& p, const anks::Routes& r, int rate)
{
    energy (e, p, r, rate / 2); // let pitch glide settle
    int crossings = 0;
    float previous = e.process (p, r);
    for (int i = 0; i < rate; ++i)
    {
        const float y = e.process (p, r);
        if (previous <= 0 && y > 0) ++crossings;
        previous = y;
    }
    return crossings;
}
}

int main()
{
    using namespace anks;
    for (int rate : {44100, 48000, 96000})
    {
        Engine e;
        Parameters p;
        Routes r;
        e.prepare (rate);
        p.seqMode = false;
        p.waveform = p.waveform2 = 0;
        p.fine2 = 0;
        e.noteOn (69, p, r);
        r.connect (Port::osc2Out, Port::outputIn);
        require (std::abs (frequency (e, p, r, rate) - 440.0) <= 1, "VCO2 does not follow MIDI pitch");
        p.tune2 = 12;
        require (std::abs (frequency (e, p, r, rate) - 880.0) <= 1, "live VCO2 octave does not work");
        p.fine2 = 50;
        require (std::abs (frequency (e, p, r, rate) - 880.0 * std::pow (2.0, 0.5 / 12.0)) <= 1,
                 "VCO2 fine tuning does not work");
        r.disconnect (Port::osc2Pitch);
        p.tune2 = p.fine2 = 0;
        require (std::abs (frequency (e, p, r, rate) - 130.8128) <= 1, "unpatched VCO2 pitch is not independent");
        r.connect (Port::oscOut, Port::outputIn);
        require (std::abs (frequency (e, p, r, rate) - 440.0) <= 1, "VCO2 tuning changed VCO1");
        p.tune = 12;
        require (std::abs (frequency (e, p, r, rate) - 880.0) <= 1, "live VCO1 tuning does not work");

        for (int wave = 0; wave < 4; ++wave)
        {
            e.reset(); r.reset(); p = Parameters {};
            p.waveform2 = wave;
            p.mix1 = 0; p.mix2 = 1;
            r.connect (Port::mixerOut, Port::outputIn);
            energy (e, p, r, rate / 2);
            require (energy (e, p, r, rate / 4) > 0.01, "VCO2 missing from mixer");
            r.disconnect (Port::mixerIn2);
            require (energy (e, p, r, rate / 4) < 1e-10, "disconnected mixer input still sounds");
            r.connect (Port::oscOut, Port::mixerIn2);
            require (energy (e, p, r, rate / 4) > 0.01, "mixer cannot accept alternative oscillator");
            const auto before = r.source;
            require (! r.connect (Port::mixerOut, Port::mixerIn1), "mixer feedback accepted");
            require (! r.connect (Port::lfoCV, Port::mixerIn2), "CV accepted at mixer audio input");
            require (r.source == before, "rejected cable mutated patch");
            p.mixLevel = 0;
            energy (e, p, r, rate / 2);
            require (energy (e, p, r, rate / 4) < 1e-10, "mixer output level cannot mute");
        }

        // Both channels contribute; returning one input to zero leaves the other audible.
        e.reset(); r.reset(); p = Parameters {};
        r.connect (Port::mixerOut, Port::outputIn);
        p.mix1 = 1; p.mix2 = 0;
        energy (e, p, r, rate / 2);
        require (energy (e, p, r, rate / 4) > 0.01, "VCO1 missing from mixer");
        p.mix1 = 0; p.mix2 = 1;
        energy (e, p, r, rate / 2);
        require (energy (e, p, r, rate / 4) > 0.01, "VCO2 channel cannot play independently");

        // Mixer sweeps are smoothed instead of stepping the signal gain.
        // Use sines to isolate gain changes from saw/square waveform edges.
        p.waveform = p.waveform2 = 0;
        energy (e, p, r, rate / 2);
        float previous = e.process (p, r);
        double largestJump = 0;
        for (int i = 0; i < rate / 2; ++i)
        {
            if (i % 256 == 0)
            {
                p.mix1 = 1 - p.mix1; p.mix2 = 1 - p.mix2;
                p.mixLevel = p.mixLevel > 0 ? 0.0f : 1.0f;
            }
            const float y = e.process (p, r);
            largestJump = std::max (largestJump, std::abs (double (y) - previous));
            previous = y;
        }
        require (largestJump < 0.02, "mixer sweep creates large discontinuity");
    }
    std::cout << "PASS: VCO2 MIDI/independent pitch, live tuning, four waves, mixer inputs/mute, routing and gain sweeps at 3 rates\n";
}
