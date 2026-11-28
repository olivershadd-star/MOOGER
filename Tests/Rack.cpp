#include "../Source/RackEngine.h"
#include <cstdlib>
#include <iostream>
#include <thread>
#include <new>
#include <sstream>
#include <fstream>

static std::atomic<bool> countAllocations {false};
static std::atomic<int> allocations {0};
void* operator new (std::size_t n) { if (countAllocations.load()) ++allocations; if (auto* p = std::malloc (n)) return p; throw std::bad_alloc(); }
void operator delete (void* p) noexcept { std::free (p); }
void operator delete (void* p, std::size_t) noexcept { std::free (p); }
void* operator new[] (std::size_t n) { return ::operator new (n); }
void operator delete[] (void* p) noexcept { std::free (p); }
void operator delete[] (void* p, std::size_t) noexcept { std::free (p); }

namespace
{
void check (bool ok, const char* message) { if (! ok) { std::cerr << "FAIL: " << message << '\n'; std::exit (1); } }
mooger::RackControls controls (const mooger::RackState& state)
{ mooger::RackControls values {}; for (int i = 0; i < mooger::maxModules; ++i) values[size_t (i)] = state.modules[size_t (i)].parameters; return values; }
void set (mooger::RackState& state, int slot, int p, float physical)
{ state.modules[size_t (slot)].parameters[size_t (p)] = mooger::info (state.modules[size_t (slot)].type).parameters[size_t (p)].normalized (physical); }
double render (mooger::RackEngine& e, int count)
{
    double energy = 0;
    for (int i = 0; i < count; ++i) { const auto y = e.process(); check (std::isfinite (y.left) && std::isfinite (y.right) && std::abs (y.left) <= 0.801f && std::abs (y.right) <= 0.801f, "output not finite/bounded"); energy += y.left * y.left + y.right * y.right; }
    return energy;
}
}
std::string catalogJson()
{
    std::ostringstream out; out << "{\n  \"schema\": 1,\n  \"modules\": [\n";
    const auto& bank = mooger::catalog();
    for (size_t i = 0; i < bank.size(); ++i)
    {
        const auto& d = bank[i];
        out << "    {\"id\": \"" << d.id << "\", \"name\": \"" << d.name << "\", \"parameters\": [";
        for (int p = 0; p < d.parameterCount; ++p)
        {
            const auto& value = d.parameters[size_t (p)]; if (p) out << ", ";
            out << "{\"name\": \"" << value.name << "\", \"min\": " << value.low << ", \"max\": " << value.high
                << ", \"default\": " << value.initial << ", \"curve\": \"" << (value.logarithmic ? "log" : "linear") << "\"}";
        }
        out << "], \"inputs\": [";
        for (int p = 0; p < d.inputCount; ++p) { if (p) out << ", "; out << "{\"name\": \"" << d.inputs[size_t (p)].name << "\", \"signal\": " << int (d.inputs[size_t (p)].signal) << "}"; }
        out << "], \"outputs\": [";
        for (int p = 0; p < d.outputCount; ++p) { if (p) out << ", "; out << "{\"name\": \"" << d.outputs[size_t (p)].name << "\", \"signal\": " << int (d.outputs[size_t (p)].signal) << "}"; }
        out << "]}" << (i + 1 < bank.size() ? "," : "") << "\n";
    }
    out << "  ]\n}\n"; return out.str();
}
int main (int argc, char** argv)
{
    using namespace mooger;
    if (argc >= 2 && std::string (argv[1]) == "--catalog") { std::cout << catalogJson(); return 0; }
    if (argc >= 3 && std::string (argv[1]) == "--check-catalog")
    {
        std::ifstream file (argv[2]); check (file.good(), "catalog file missing");
        std::string actual ((std::istreambuf_iterator<char> (file)), std::istreambuf_iterator<char>());
        actual.erase (std::remove (actual.begin(), actual.end(), '\r'), actual.end());
        check (actual == catalogJson(), "Library/modules.json differs from the executable module bank");
        std::cout << "PASS: catalog database matches runtime registry\n"; return 0;
    }
    for (const auto& d : catalog())
    {
        check (d.parameterCount <= maxParameters && d.inputCount <= maxPorts && d.outputCount <= maxPorts, "catalog exceeds capacity");
        check (findType (d.id) != Type::count, "catalog ID missing");
        for (int p = 0; p < d.parameterCount; ++p) check (std::abs (d.parameters[size_t (p)].physical (d.parameters[size_t (p)].normalized (d.parameters[size_t (p)].initial)) - d.parameters[size_t (p)].initial) < 0.02f, "catalog default conversion");
    }
    RackState capacity;
    for (int i = 0; i < maxModules; ++i) check (add (capacity, Type::vco) >= 0, "cannot add 32 independent modules");
    check (add (capacity, Type::lfo) == -1 && validate (capacity), "capacity handling invalid");
    auto invalid = capacity; invalid.modules[1].uid = invalid.modules[0].uid; check (! validate (invalid), "duplicate UID accepted");

    for (int rate : {44100, 48000, 96000})
    {
        auto state = initialRack(); set (state, 0, 1, 1);
        RackEngine e, reference; e.prepare (rate); reference.prepare (rate);
        e.apply (state); reference.apply (state); e.setControls (controls (state)); reference.setControls (controls (state));
        double energy = 0; unsigned visited = 0;
        for (int i = 0; i < rate * 3; ++i) { const auto y = e.process(); reference.process(); energy += y.left * y.left; if (e.step (0) >= 0) visited |= 1u << e.step (0); }
        check (energy > 0.01 && visited == 255, "starter rack not sounding/eight steps");
        move (state, 1, 3, 2); e.apply (state);
        for (int i = 0; i < rate / 4; ++i) { const auto a = e.process(), b = reference.process(); check (a.left == b.left && a.right == b.right, "moving a module changed sound/phase"); }

        RackState stereo;
        const int a = add (stereo, Type::vco), b = add (stereo, Type::vco), out = add (stereo, Type::output);
        set (stereo, a, 0, 0); set (stereo, b, 0, 0); set (stereo, a, 1, -12); set (stereo, b, 1, 12);
        connect (stereo, {a, 0, out, 0}); connect (stereo, {b, 0, out, 1});
        e.apply (stereo); e.setControls (controls (stereo)); render (e, rate);
        int leftCrossings = 0, rightCrossings = 0; auto before = e.process();
        for (int i = 0; i < rate; ++i) { const auto y = e.process(); if (before.left <= 0 && y.left > 0) ++leftCrossings; if (before.right <= 0 && y.right > 0) ++rightCrossings; before = y; }
        check (std::abs (leftCrossings - 65.4064) < 2 && std::abs (rightCrossings - 261.626) < 2, "VCO instances share tuning or DSP");
        remove (stereo, out); e.apply (stereo); render (e, rate / 100);
        check (render (e, 100) == 0, "rack without output is not silent");

        RackState independent;
        int lfo1 = add (independent, Type::lfo), lfo2 = add (independent, Type::lfo);
        int env1 = add (independent, Type::envelope), env2 = add (independent, Type::envelope);
        set (independent, lfo1, 0, 1); set (independent, lfo2, 0, 2);
        for (int env : {env1, env2}) { set (independent, env, 0, 0.01f); set (independent, env, 1, 0.01f); set (independent, env, 2, 0.8f); set (independent, env, 3, 0.01f); }
        connect (independent, {lfo1, 1, env1, 0}); connect (independent, {lfo2, 1, env2, 0});
        e.apply (independent); e.setControls (controls (independent)); render (e, int (rate * 0.65));
        check (e.output (env1) < 1e-6 && e.output (env2) > 0.7, "envelope/LFO instances share state");

        // MIDI last-note priority, independent note offs.
        auto midi = initialRack (true); e.apply (midi); e.setControls (controls (midi));
        e.noteOn (60, 1); e.noteOn (67, 0.7f); e.noteOff (60); render (e, rate / 10);
        check (e.output (0, 0) == 67 && e.output (0, 1) == 1, "unrelated note-off released the current note");
        e.noteOff (67); render (e, rate); check (e.output (0, 1) == 0, "MIDI gate stuck");

        // Free routing, including an audio feedback path through a filter and delay.
        RackState feedback;
        int osc = add (feedback, Type::vco), mix = add (feedback, Type::mixer), filter = add (feedback, Type::vcf);
        int effect = add (feedback, Type::delay), output = add (feedback, Type::output);
        set (feedback, effect, 0, 0.01f); set (feedback, effect, 1, 0.9f); set (feedback, filter, 1, 9);
        connect (feedback, {osc, 0, mix, 0}); connect (feedback, {effect, 0, mix, 1});
        connect (feedback, {mix, 0, filter, 0}); connect (feedback, {filter, 0, effect, 0}); connect (feedback, {effect, 0, output, 0});
        check (validate (feedback), "feedback graph refused"); e.apply (feedback); e.setControls (controls (feedback));
        countAllocations.store (true); const double sound = render (e, rate); countAllocations.store (false);
        check (sound > 0.01 && allocations.load() == 0, "feedback silent or audio allocates");

        // Every catalog type can be instantiated repeatedly without invalid audio.
        for (int type = 0; type < int (Type::count); ++type)
        {
            RackState rack; const int node = add (rack, Type (type)); const int sink = add (rack, Type::output);
            if (info (Type (type)).outputCount) connect (rack, {node, 0, sink, 0});
            e.apply (rack); e.setControls (controls (rack)); render (e, rate / 50);
        }
        e.prepare (rate); e.apply (state); e.setControls (controls (state)); check (render (e, rate) > 0.01, "reprepare stopped rack");
    }
    RackMailbox mailbox; std::atomic<bool> done {false};
    std::thread writer ([&] { for (int i = 1; i <= 10000; ++i) { RackState state; state.revision = std::uint64_t (i); state.nextUid = std::uint32_t (i + 1); state.modules[0].active = true; state.modules[0].uid = std::uint32_t (i); mailbox.publish (state); } done.store (true); });
    RackState snapshot; std::uint64_t last = 0;
    while (! done.load() || last < 10000) if (mailbox.read (snapshot)) { check (snapshot.revision == snapshot.modules[0].uid && snapshot.nextUid == snapshot.modules[0].uid + 1, "torn graph snapshot"); last = snapshot.revision; }
    writer.join();
    std::cout << "PASS: 16 catalog types, 32 modules, independent VCO/LFO/envelopes, move invariance, MIDI, feedback, no audio allocation, concurrent publication\n";
}
