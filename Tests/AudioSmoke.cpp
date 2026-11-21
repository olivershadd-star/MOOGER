#include "../Source/SynthEngine.h"
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace
{
void require (bool condition, const char* message)
{
    if (! condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        std::exit (EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace anks;
    static_assert (validConnection (Port::oscOut, Port::filterIn));
    static_assert (! validConnection (Port::lfoCV, Port::outputIn));
    float peak = 0.0f;
    for (double rate : {44100.0, 48000.0, 96000.0})
    for (int wave = 0; wave < 4; ++wave)
    {
        Routes wires;
        Parameters p;
        Engine engine;
        engine.prepare (rate);
        p.waveform = wave;
        p.run = true;
        double energy = 0.0;
        unsigned stepsSeen = 0;
        for (int i = 0; i < static_cast<int> (rate * 3); ++i)
        {
            const float y = engine.process (p, wires);
            require (std::isfinite (y), "non-finite sequence output");
            require (std::abs (y) < 1.0f, "output exceeds headroom");
            energy += y * y;
            peak = std::max (peak, std::abs (y));
            const int step = engine.getCurrentStep();
            require (step >= 0 && step < 8, "invalid step index");
            stepsSeen |= 1u << step;
        }
        require (energy > 0.001, "silent sequence");
        require (stepsSeen == 255u, "sequence did not visit all eight steps");

        wires.disconnect (Port::outputIn);
        for (int i = 0; i < static_cast<int> (rate); ++i)
            require (std::abs (engine.process (p, wires)) < 1e-7f, "disconnected output is not silent");

        wires.connect (Port::vcaOut, Port::outputIn);
        wires.connect (Port::lfoCV, Port::filterCV);
        p.cutoff = 15000.0f; p.resonance = 9.0f; p.lfoDepth = 5000.0f;
        for (int i = 0; i < static_cast<int> (rate); ++i)
        {
            const float y = engine.process (p, wires);
            require (std::isfinite (y) && std::abs (y) < 1.0f, "unstable CV modulation");
        }

        engine.reset();
        wires.reset();
        p = Parameters {};
        p.seqMode = false;
        p.waveform = wave;
        engine.noteOn (60, p, wires);
        energy = 0.0;
        for (int i = 0; i < static_cast<int> (rate * 0.5); ++i)
        {
            const float y = engine.process (p, wires);
            require (std::isfinite (y), "non-finite note output");
            energy += y * y;
        }
        require (energy > 0.001, "silent triggered note");
        engine.noteOff (p, wires);
        float tail = 1.0f;
        for (int i = 0; i < static_cast<int> (rate * 0.5); ++i)
            tail = engine.process (p, wires);
        require (std::isfinite (tail) && std::abs (tail) < 1e-7f, "note off did not release");
    }
    std::cout << "PASS: 3 sample rates x 4 waves; eight steps, headroom, output disconnect, CV, note release. peak="
              << peak << '\n';
}
