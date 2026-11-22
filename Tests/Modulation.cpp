#include "../Source/SynthEngine.h"
#include <cstdlib>
#include <iostream>

namespace
{
void require (bool ok, const char* message)
{
    if (! ok) { std::cerr << "FAIL: " << message << '\n'; std::exit (EXIT_FAILURE); }
}
void render (anks::Engine& engine, const anks::Parameters& p, const anks::Routes& routes, int count)
{
    for (int i = 0; i < count; ++i)
    {
        const float y = engine.process (p, routes);
        require (std::isfinite (y) && std::abs (y) < 1.0f, "unstable modulated audio");
    }
}
int frequency (anks::Engine& engine, const anks::Parameters& p, const anks::Routes& r, int rate)
{
    render (engine, p, r, rate);
    float before = engine.process (p, r);
    int rising = 0;
    for (int i = 0; i < rate; ++i)
    {
        const float y = engine.process (p, r);
        if (before <= 0 && y > 0) ++rising;
        before = y;
    }
    return rising;
}
}

int main()
{
    using namespace anks;
    static_assert (index (Port::drumMixIn) == 20, "V0.4 ports changed");
    static_assert (! validConnection (Port::modLagOut, Port::modSampleIn), "feedback accepted");
    static_assert (! validConnection (Port::modMixOut, Port::modAtt1In), "feedback accepted");
    static_assert (! validConnection (Port::oscOut, Port::modSampleIn), "audio accepted as CV");
    static_assert (validConnection (Port::seqGate, Port::modClockIn));
    for (int rate : {44100, 48000, 96000})
    {
        ModulationEngine mod; mod.prepare (rate);
        ModulationParameters params;
        double sample = 0.3, clock = 0;
        auto shInput = [&] (ModulationInput input) { return input == ModulationInput::sample ? sample
                                                        : input == ModulationInput::clock ? clock : 0.0; };
        mod.process (params, shInput);
        require (mod.outputs().held == 0, "S&H sampled without a trigger");
        clock = 1; mod.process (params, shInput);
        require (std::abs (mod.outputs().held - 0.3) < 1e-12, "S&H missed rising edge");
        sample = -0.7; mod.process (params, shInput);
        require (std::abs (mod.outputs().held - 0.3) < 1e-12, "S&H tracks instead of holding");
        clock = 0; mod.process (params, shInput);
        require (std::abs (mod.outputs().held - 0.3) < 1e-12, "S&H captures falling edge");
        clock = 1; mod.process (params, shInput);
        require (std::abs (mod.outputs().held + 0.7) < 1e-12, "S&H missed next rising edge");
        clock = 0; sample = 0.9; mod.requestSample(); mod.process (params, shInput);
        require (std::abs (mod.outputs().held - 0.9) < 1e-12, "manual SAMPLE failed");

        params.atten1 = 0.25f; params.atten2 = 0.75f;
        params.mix1 = 0.5f; params.mix2 = 0.25f; params.offset = 0.15f; params.level = 0.6f;
        auto mixInput = [&] (ModulationInput input) -> double
        {
            switch (input)
            {
                case ModulationInput::atten1: return 0.8;
                case ModulationInput::atten2: return -0.4;
                case ModulationInput::mix1: return mod.outputs().atten1;
                case ModulationInput::mix2: return mod.outputs().atten2;
                case ModulationInput::mix3: return 0.1;
                case ModulationInput::mix4: return -0.2;
                default: return 0;
            }
        };
        for (int i = 0; i < rate / 4; ++i) mod.process (params, mixInput);
        require (std::abs (mod.outputs().atten1 - 0.2) < 1e-6, "attenuator 1 gain wrong");
        require (std::abs (mod.outputs().atten2 + 0.3) < 1e-6, "attenuator 2 gain wrong");
        require (std::abs (mod.outputs().mixed - 0.045) < 1e-6, "four-input CV mix or offset wrong");
        require (mod.outputs().inverted == -mod.outputs().mixed, "inverted mixer output wrong");

        mod.reset(); params = ModulationParameters {}; params.rise = 0.1f; params.fall = 0.5f;
        double target = 1;
        auto lagInput = [&] (ModulationInput input) { return input == ModulationInput::lag ? target : 0.0; };
        for (int i = 0; i < rate / 10; ++i) mod.process (params, lagInput);
        require (std::abs (mod.outputs().lag - (1 - std::exp (-1.0))) < 1e-5, "lag rise timing wrong");
        const double top = mod.outputs().lag;
        target = -1;
        for (int i = 0; i < rate / 2; ++i) mod.process (params, lagInput);
        require (std::abs (mod.outputs().lag - (-1 + (top + 1) * std::exp (-1.0))) < 1e-5, "lag fall timing wrong");

        Engine e, reference; e.prepare (rate); reference.prepare (rate);
        Parameters p; Routes r, isolated;
        for (int i = index (Port::modTriangle); i < portCount; ++i)
            if (! isOutput (static_cast<Port> (i))) isolated.disconnect (static_cast<Port> (i));
        p.run = p.drums.run = true;
        int changes = 0; double held = 0; double square = -1;
        for (int i = 0; i < rate * 2; ++i)
        {
            const float actual = e.process (p, r);
            require (actual == reference.process (p, isolated), "unpatched CV panel changes existing sound");
            const auto& m = e.getModulation();
            require (m.triangle >= -1 && m.triangle <= 1 && m.noise >= -1 && m.noise <= 1, "CV source outside bounds");
            if (m.held != held)
            {
                require (square <= 0.5 && m.square > 0.5, "default S&H changes between clock edges");
                ++changes;
            }
            held = m.held; square = m.square;
        }
        require (changes >= 2 && changes <= 3, "LFO clock rate incorrect");

        // Constant CV from the offset raises VCO1 exactly one octave.
        e.reset(); p = Parameters {}; r.reset(); p.seqMode = false; p.waveform = 0;
        e.noteOn (69, p, r);
        r.connect (Port::oscOut, Port::outputIn);
        require (std::abs (frequency (e, p, r, rate) - 440) <= 1, "base MIDI pitch wrong");
        r.disconnect (Port::modMixIn1); r.disconnect (Port::modMixIn2);
        p.modulation.offset = p.modulation.level = 1; p.modulation.pitchDepth = 12;
        r.connect (Port::modMixOut, Port::oscMod1In);
        require (std::abs (frequency (e, p, r, rate) - 880) <= 1, "pitch CV depth wrong");

        // Every public modulation output can drive every new destination safely.
        for (Port source : {Port::modTriangle, Port::modSquare, Port::modNoise, Port::modSampleOut,
                            Port::modAtt1Out, Port::modAtt2Out, Port::modMixOut, Port::modInvertOut, Port::modLagOut})
        for (Port destination : {Port::filterCV, Port::vcaCV, Port::oscMod1In, Port::oscMod2In})
        {
            e.reset(); p = Parameters {}; r.reset(); p.run = true;
            p.modulation.rate = 30; p.modulation.filterDepth = 8000; p.modulation.pitchDepth = 24;
            require (r.connect (source, destination), "valid modulation cable rejected");
            render (e, p, r, rate / 10);
        }
    }
    std::cout << "PASS: S&H edges/manual capture, attenuators, four-input CV mixer, inversion, asymmetric lag, legacy sound and pitch/filter/VCA routing at 3 rates\n";
}
