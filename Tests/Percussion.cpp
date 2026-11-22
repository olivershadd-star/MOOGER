#include "../Source/SynthEngine.h"
#include <cstdlib>
#include <iostream>

namespace
{
void require (bool ok, const char* message)
{
    if (! ok) { std::cerr << "FAIL: " << message << '\n'; std::exit (EXIT_FAILURE); }
}
double render (anks::Engine& e, const anks::Parameters& p, const anks::Routes& r, int samples)
{
    double energy = 0;
    for (int i = 0; i < samples; ++i)
    {
        const float y = e.process (p, r);
        require (std::isfinite (y) && std::abs (y) < 1, "invalid or clipping output");
        energy += y * y;
    }
    return energy;
}
}

int main()
{
    using namespace anks;
    static_assert (index (Port::mixerOut) == 18, "legacy ports changed");
    static_assert (validConnection (Port::drumOut, Port::mixerIn1));
    static_assert (! validConnection (Port::mixerOut, Port::drumMixIn));
    for (int rate : {44100, 48000, 96000})
    {
        Engine e; e.prepare (rate);
        Parameters p; Routes r;
        require (render (e, p, r, rate / 4) == 0, "new instance is not silent");
        p.drums.run = true;
        double energy = 0;
        unsigned visited = 0;
        for (int i = 0; i < rate * 2; ++i)
        {
            const float y = e.process (p, r);
            require (std::isfinite (y) && std::abs (y) < 1, "drum output invalid");
            energy += y * y;
            require (e.getCurrentStep() == -1, "drum run started melody");
            const int step = e.getDrumStep();
            require (step >= 0 && step < 8, "invalid percussion step");
            visited |= 1u << step;
        }
        require (energy > 0.01 && visited == 255, "drums silent or not eight steps");
        r.disconnect (Port::drumMixIn);
        require (render (e, p, r, rate / 4) == 0, "drum bus disconnect did not mute");
        r.connect (Port::drumOut, Port::outputIn);
        require (render (e, p, r, rate / 4) > 0.001, "drum audio output cannot be repatched");
        r.reset();
        p.drums.run = false;
        render (e, p, r, rate * 6);
        require (e.getDrumStep() == -1 && render (e, p, r, rate / 4) == 0, "STOP tail never decays");
        e.triggerDrum (p);
        require (render (e, p, r, rate / 2) > 0.01, "manual HIT fails with sequencer stopped");
        e.reset();
        p.drums.run = true;
        for (auto& step : p.drums.steps) step.velocity = 0;
        require (render (e, p, r, rate) == 0, "zero-velocity rests still trigger");

        // Clock linking follows BPM only; both run controls remain independent.
        p = Parameters {}; p.drums.run = true; p.bpm = 120; p.drums.bpm = 240;
        e.reset(); render (e, p, r, rate / 2);
        require (e.getDrumStep() == 3, "linked tempo is wrong");
        p.drums.followTempo = false;
        e.reset(); render (e, p, r, rate / 2);
        require (e.getDrumStep() == 7, "independent tempo is wrong");

        // Loud, long-decay percussion and melody together at extreme controls.
        for (int shape = 0; shape < 2; ++shape)
        {
            e.reset(); p = Parameters {}; p.run = p.drums.run = true;
            p.drums.waveform = shape; p.drums.pitchHz = 800; p.drums.ratio = 8;
            p.drums.fm = 4; p.drums.noise = 1; p.drums.tone = 1;
            p.drums.pitchDepth = 48; p.drums.resonance = 6;
            p.drums.cutoff = 16000; p.drums.decay = 2; p.drums.level = 1;
            p.master = 1;
            require (render (e, p, r, rate * 2) > 0.01, "combined voice silent");
        }

        // A short drum hit must not reset a long sustained melodic envelope.
        e.reset(); p = Parameters {}; p.seqMode = false;
        p.sustain = 1; p.decay = 0.01f;
        e.noteOn (60, p, r); render (e, p, r, rate / 2);
        e.triggerDrum (p); render (e, p, r, rate);
        p.drums.level = 0;
        require (render (e, p, r, rate / 4) > 0.01, "percussion killed sustained melody");
    }
    std::cout << "PASS: percussion, 8 steps, rests, separate clocks/run/envelopes, manual hit, patching, tails, extremes at 3 rates\n";
}
