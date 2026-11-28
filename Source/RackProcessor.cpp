#include "PluginProcessor.h"

namespace
{
juce::String slotId (int slot, int parameter) { return "rack" + juce::String (slot) + "p" + juce::String (parameter); }
}
void ANKSModularProcessor::initializeRack()
{
    (void) mooger::catalog();
    for (int slot = 0; slot < mooger::maxModules; ++slot)
    {
        rackSteps[size_t (slot)].store (-1);
        for (int p = 0; p < mooger::maxParameters; ++p)
        {
            const auto id = slotId (slot, p);
            rackRaw[size_t (slot)][size_t (p)] = parameters.getRawParameterValue (id);
            rackObjects[size_t (slot)][size_t (p)] = parameters.getParameter (id);
        }
    }
    newFreeRack (1);
}
void ANKSModularProcessor::publishRackLocked()
{
    rackModel.revision = ++rackRevision; rackMailbox.publish (rackModel);
}
void ANKSModularProcessor::installRackValues (const mooger::RackState& state)
{
    for (int slot = 0; slot < mooger::maxModules; ++slot)
        for (int p = 0; p < mooger::maxParameters; ++p)
            rackObjects[size_t (slot)][size_t (p)]->setValueNotifyingHost (state.modules[size_t (slot)].parameters[size_t (p)]);
}
void ANKSModularProcessor::newFreeRack (int preset)
{
    const juce::ScopedLock guard (rackLock);
    rackModel = preset == 0 ? mooger::RackState {} : mooger::initialRack (preset == 2);
    rackModel.generation = ++rackGeneration;
    installRackValues (rackModel); publishRackLocked(); freeRackEnabled.store (true, std::memory_order_release);
}
mooger::RackState ANKSModularProcessor::rackState() const
{
    const juce::ScopedLock guard (rackLock);
    auto state = rackModel;
    for (int slot = 0; slot < mooger::maxModules; ++slot)
        for (int p = 0; p < mooger::maxParameters; ++p) state.modules[size_t (slot)].parameters[size_t (p)] = rackValue (slot, p);
    return state;
}
int ANKSModularProcessor::addRackModule (mooger::Type type, int col, int row)
{
    const juce::ScopedLock guard (rackLock);
    const int slot = mooger::add (rackModel, type, col, row);
    if (slot >= 0) { for (int p = 0; p < mooger::maxParameters; ++p) setRackValue (slot, p, rackModel.modules[size_t (slot)].parameters[size_t (p)]); publishRackLocked(); }
    return slot;
}
int ANKSModularProcessor::duplicateRackModule (int original)
{
    const juce::ScopedLock guard (rackLock);
    if (! mooger::validSlot (original) || ! rackModel.modules[size_t (original)].active) return -1;
    const int slot = mooger::add (rackModel, rackModel.modules[size_t (original)].type);
    if (slot >= 0)
    {
        for (int p = 0; p < mooger::maxParameters; ++p) setRackValue (slot, p, rackValue (original, p));
        publishRackLocked();
    }
    return slot;
}
bool ANKSModularProcessor::replaceRackModule (int slot, mooger::Type type)
{
    const juce::ScopedLock guard (rackLock);
    if (! mooger::validSlot (slot) || ! rackModel.modules[size_t (slot)].active || ! mooger::validType (type)) return false;
    if (rackModel.nextUid == UINT32_MAX) return false;
    // Preserve compatible port indices, remove only endpoints missing in the new type.
    rackModel.modules[size_t (slot)].type = type;
    rackModel.modules[size_t (slot)].uid = rackModel.nextUid++;
    const auto& def = mooger::info (type);
    for (int p = 0; p < mooger::maxParameters; ++p) setRackValue (slot, p,
        p < def.parameterCount ? def.parameters[size_t (p)].normalized (def.parameters[size_t (p)].initial) : 0);
    for (int i = rackModel.cableCount - 1; i >= 0; --i)
        if (! mooger::validCable (rackModel, rackModel.cables[size_t (i)]))
        { for (int j = i; j + 1 < rackModel.cableCount; ++j) rackModel.cables[size_t (j)] = rackModel.cables[size_t (j + 1)]; --rackModel.cableCount; }
    publishRackLocked(); return true;
}
void ANKSModularProcessor::removeRackModule (int slot)
{ const juce::ScopedLock guard (rackLock); mooger::remove (rackModel, slot); publishRackLocked(); }
bool ANKSModularProcessor::moveRackModule (int slot, int col, int row)
{ const juce::ScopedLock guard (rackLock); const bool ok = mooger::move (rackModel, slot, col, row); if (ok) publishRackLocked(); return ok; }
bool ANKSModularProcessor::connectRack (mooger::Cable c)
{ const juce::ScopedLock guard (rackLock); const bool ok = mooger::connect (rackModel, c); if (ok) publishRackLocked(); return ok; }
void ANKSModularProcessor::disconnectRack (int slot, int input)
{ const juce::ScopedLock guard (rackLock); mooger::disconnect (rackModel, slot, input); publishRackLocked(); }
juce::RangedAudioParameter* ANKSModularProcessor::rackParameter (int slot, int p) noexcept
{ return mooger::validSlot (slot) && p >= 0 && p < mooger::maxParameters ? rackObjects[size_t (slot)][size_t (p)] : nullptr; }
void ANKSModularProcessor::setRackValue (int slot, int p, float value)
{ if (auto* parameter = rackParameter (slot, p)) parameter->setValueNotifyingHost (std::isfinite (value) ? std::clamp (value, 0.0f, 1.0f) : 0.0f); }
float ANKSModularProcessor::rackValue (int slot, int p) const noexcept
{ return mooger::validSlot (slot) && p >= 0 && p < mooger::maxParameters ? rackRaw[size_t (slot)][size_t (p)]->load (std::memory_order_relaxed) : 0; }
void ANKSModularProcessor::triggerRackModule (int slot) noexcept
{ if (mooger::validSlot (slot)) rackTriggers.fetch_or (std::uint32_t (1) << slot, std::memory_order_release); }
juce::ValueTree ANKSModularProcessor::exportRack() const
{
    const auto state = rackState(); juce::ValueTree root ("FreeRack");
    root.setProperty ("enabled", isFreeRack(), nullptr);
    root.setProperty ("nextUid", juce::int64 (state.nextUid), nullptr);
    for (int i = 0; i < mooger::maxModules; ++i)
    {
        const auto& module = state.modules[size_t (i)]; if (! module.active) continue;
        juce::ValueTree n ("Module");
        n.setProperty ("slot", i, nullptr); n.setProperty ("type", mooger::info (module.type).id, nullptr);
        n.setProperty ("uid", juce::int64 (module.uid), nullptr); n.setProperty ("column", module.column, nullptr); n.setProperty ("row", module.row, nullptr);
        for (int p = 0; p < mooger::maxParameters; ++p) n.setProperty ("p" + juce::String (p), module.parameters[size_t (p)], nullptr);
        root.appendChild (n, nullptr);
    }
    for (int i = 0; i < state.cableCount; ++i)
    {
        const auto c = state.cables[size_t (i)]; juce::ValueTree n ("Cable");
        n.setProperty ("from", c.from, nullptr); n.setProperty ("output", c.output, nullptr);
        n.setProperty ("to", c.to, nullptr); n.setProperty ("input", c.input, nullptr); root.appendChild (n, nullptr);
    }
    return root;
}
bool ANKSModularProcessor::parseRackTree (const juce::ValueTree& tree, mooger::RackState& state)
{
    if (! tree.hasType ("FreeRack") || tree.getNumChildren() > mooger::maxModules + mooger::maxCables) return false;
    state = mooger::RackState {};
    const auto uid = juce::int64 (tree.getProperty ("nextUid", 0));
    if (uid < 1 || uid >= UINT32_MAX) return false;
    state.nextUid = std::uint32_t (uid);
    for (const auto& child : tree)
    {
        if (child.hasType ("Module"))
        {
            const int slot = child.getProperty ("slot", -1);
            const auto type = mooger::findType (child.getProperty ("type").toString().toRawUTF8());
            const auto moduleUid = juce::int64 (child.getProperty ("uid", 0));
            if (! mooger::validSlot (slot) || state.modules[size_t (slot)].active || ! mooger::validType (type) || moduleUid < 1 || moduleUid >= uid) return false;
            auto& module = state.modules[size_t (slot)]; module.active = true; module.type = type; module.uid = std::uint32_t (moduleUid);
            module.column = child.getProperty ("column", -1); module.row = child.getProperty ("row", -1);
            for (int p = 0; p < mooger::maxParameters; ++p) module.parameters[size_t (p)] = child.getProperty ("p" + juce::String (p), 0.0f);
        }
        else if (child.hasType ("Cable"))
        {
            if (state.cableCount >= mooger::maxCables) return false;
            state.cables[size_t (state.cableCount++)] = {child.getProperty ("from", -1), child.getProperty ("output", -1), child.getProperty ("to", -1), child.getProperty ("input", -1)};
        }
        else return false;
    }
    return mooger::validate (state);
}
bool ANKSModularProcessor::restoreRack (const juce::ValueTree& tree)
{
    mooger::RackState state;
    if (! parseRackTree (tree, state)) return false;
    const juce::ScopedLock guard (rackLock);
    state.generation = ++rackGeneration; rackModel = state; installRackValues (state); publishRackLocked();
    freeRackEnabled.store (bool (tree.getProperty ("enabled", true)), std::memory_order_release); return true;
}
