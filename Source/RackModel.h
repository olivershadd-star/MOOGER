#pragma once
#include "ModuleRegistry.h"
#include <atomic>
#include <cstdint>

namespace mooger
{
struct Module
{
    bool active = false;
    Type type = Type::vco;
    std::uint32_t uid = 0;
    int column = 0, row = 0;
    std::array<float, maxParameters> parameters {};
};
struct Cable { int from = -1, output = 0, to = -1, input = 0; };
struct RackState
{
    std::array<Module, maxModules> modules {};
    std::array<Cable, maxCables> cables {};
    int cableCount = 0;
    std::uint32_t nextUid = 1;
    std::uint64_t revision = 1;
    std::uint64_t generation = 1;
};
inline bool validSlot (int slot) noexcept { return slot >= 0 && slot < maxModules; }
inline bool validCable (const RackState& r, Cable c) noexcept
{
    return validSlot (c.from) && validSlot (c.to) && r.modules[size_t (c.from)].active && r.modules[size_t (c.to)].active
        && validType (r.modules[size_t (c.from)].type) && validType (r.modules[size_t (c.to)].type)
        && c.output >= 0 && c.output < info (r.modules[size_t (c.from)].type).outputCount
        && c.input >= 0 && c.input < info (r.modules[size_t (c.to)].type).inputCount;
}
inline bool validate (const RackState& r) noexcept
{
    if (r.cableCount < 0 || r.cableCount > maxCables || r.nextUid == 0) return false;
    for (int i = 0; i < maxModules; ++i)
    {
        const auto& m = r.modules[size_t (i)]; if (! m.active) continue;
        if (! validType (m.type) || m.uid == 0 || m.uid >= r.nextUid || m.column < 0 || m.column > 7 || m.row < 0 || m.row > 31) return false;
        for (float value : m.parameters) if (! std::isfinite (value) || value < 0 || value > 1) return false;
        for (int j = 0; j < i; ++j)
            if (r.modules[size_t (j)].active && (r.modules[size_t (j)].uid == m.uid
                || (r.modules[size_t (j)].column == m.column && r.modules[size_t (j)].row == m.row))) return false;
    }
    for (int i = 0; i < r.cableCount; ++i)
    {
        if (! validCable (r, r.cables[size_t (i)])) return false;
        for (int j = 0; j < i; ++j)
            if (r.cables[size_t (j)].to == r.cables[size_t (i)].to && r.cables[size_t (j)].input == r.cables[size_t (i)].input) return false;
    }
    return true;
}
inline int occupant (const RackState& r, int col, int row) noexcept
{
    for (int i = 0; i < maxModules; ++i) if (r.modules[size_t (i)].active && r.modules[size_t (i)].column == col && r.modules[size_t (i)].row == row) return i;
    return -1;
}
inline void remove (RackState& r, int slot) noexcept
{
    if (! validSlot (slot)) return;
    r.modules[size_t (slot)].active = false;
    for (int i = r.cableCount - 1; i >= 0; --i)
        if (r.cables[size_t (i)].from == slot || r.cables[size_t (i)].to == slot)
        { for (int j = i; j + 1 < r.cableCount; ++j) r.cables[size_t (j)] = r.cables[size_t (j + 1)]; --r.cableCount; }
    ++r.revision;
}
inline int add (RackState& r, Type type, int col = -1, int row = -1) noexcept
{
    if (! validType (type) || r.nextUid == UINT32_MAX) return -1;
    if (col >= 0) col = std::clamp (col, 0, 7);
    if (row >= 0) row = std::clamp (row, 0, 31);
    int slot = -1;
    for (int i = 0; i < maxModules; ++i) if (! r.modules[size_t (i)].active) { slot = i; break; }
    if (slot < 0) return -1;
    if (col < 0 || row < 0 || occupant (r, col, row) >= 0)
    {
        for (int cell = 0; cell < 128; ++cell)
            if (occupant (r, cell % 4, cell / 4) < 0) { col = cell % 4; row = cell / 4; break; }
    }
    auto& m = r.modules[size_t (slot)]; m = Module {};
    m.active = true; m.type = type; m.uid = r.nextUid++;
    m.column = std::clamp (col, 0, 7); m.row = std::clamp (row, 0, 31);
    const auto& d = info (type);
    for (int i = 0; i < d.parameterCount; ++i) m.parameters[size_t (i)] = d.parameters[size_t (i)].normalized (d.parameters[size_t (i)].initial);
    ++r.revision; return slot;
}
inline bool connect (RackState& r, Cable c) noexcept
{
    if (! validCable (r, c)) return false;
    for (int i = 0; i < r.cableCount; ++i)
        if (r.cables[size_t (i)].to == c.to && r.cables[size_t (i)].input == c.input)
        { r.cables[size_t (i)] = c; ++r.revision; return true; }
    if (r.cableCount == maxCables) return false;
    r.cables[size_t (r.cableCount++)] = c; ++r.revision; return true;
}
inline void disconnect (RackState& r, int slot, int input) noexcept
{
    for (int i = r.cableCount - 1; i >= 0; --i) if (r.cables[size_t (i)].to == slot && r.cables[size_t (i)].input == input)
    { for (int j = i; j + 1 < r.cableCount; ++j) r.cables[size_t (j)] = r.cables[size_t (j + 1)]; --r.cableCount; ++r.revision; }
}
inline bool move (RackState& r, int slot, int col, int row) noexcept
{
    if (! validSlot (slot) || ! r.modules[size_t (slot)].active) return false;
    col = std::clamp (col, 0, 7); row = std::clamp (row, 0, 31);
    auto& m = r.modules[size_t (slot)]; const int other = occupant (r, col, row);
    if (other >= 0 && other != slot) { r.modules[size_t (other)].column = m.column; r.modules[size_t (other)].row = m.row; }
    m.column = col; m.row = row; ++r.revision; return true;
}
inline RackState initialRack (bool midi = false)
{
    RackState r;
    const int control = add (r, midi ? Type::midi : Type::sequencer);
    const int oscillator = add (r, Type::vco), filter = add (r, Type::vcf), env = add (r, Type::envelope);
    const int amp = add (r, Type::vca), output = add (r, Type::output), lfo = add (r, Type::lfo);
    (void) lfo;
    connect (r, {control, 0, oscillator, 0}); connect (r, {control, 1, env, 0});
    connect (r, {oscillator, 0, filter, 0}); connect (r, {filter, 0, amp, 0});
    connect (r, {env, 0, amp, 1}); connect (r, {amp, 0, output, 0});
    return r;
}

// Three POD buffers; serialized producers use the processor's UI/state lock.
// The audio reader never locks, allocates, or waits for the producer.
class RackMailbox
{
public:
    void publish (const RackState& state) noexcept
    {
        const int busy = reader.load(), current = published.load();
        int free = 0; while (free == busy || free == current) ++free;
        buffers[size_t (free)] = state; published.store (free);
    }
    bool read (RackState& result) noexcept
    {
        int index;
        do { index = published.load(); reader.store (index); } while (index != published.load());
        if (index < 0) { reader.store (-1); return false; }
        const bool changed = buffers[size_t (index)].revision != consumed;
        if (changed) { result = buffers[size_t (index)]; consumed = result.revision; }
        reader.store (-1); return changed;
    }
private:
    std::array<RackState, 3> buffers {};
    std::atomic<int> published {-1}, reader {-1};
    std::uint64_t consumed = 0;
};
} // namespace mooger
