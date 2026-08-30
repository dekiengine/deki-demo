#pragma once

#include "ParticleModifierRegistry.h"

#include <cstdint>
#include <vector>

/**
 * @file ParticleChain.h
 * @brief Walking a particle graph into a flat list of modifier callbacks.
 *
 * Two callers with the same shape: the runtime emitter walking a loaded
 * ParticleGraph asset, and the editor preview walking the LIVE document being
 * edited. Both graph types answer FindFirstOfType(typeId) and Next(nodeId,
 * pin) and expose nodes with {id, instance}, so the walk is written once as a
 * template rather than twice with a copy-paste drift risk.
 */

// One modifier in a built chain: the node's shared authoring data, the owner's
// private state blob, and the behavior to run.
struct ParticleChainEntry
{
    const void*                data  = nullptr;   // node instance (owned by the graph)
    void*                      state = nullptr;   // stateSize bytes, owned by the chain
    const ParticleModifierOps* ops   = nullptr;
};

inline void FreeParticleChain(std::vector<ParticleChainEntry>& chain)
{
    for (ParticleChainEntry& e : chain)
        delete[] static_cast<uint8_t*>(e.state);
    chain.clear();
}

// Loop guard for a hand-edited file that wires a cycle. A real effect is a
// handful of modifiers; nothing legitimate comes close.
constexpr int kMaxParticleChainLength = 256;

/**
 * @brief Walk from the Emitter node forward, one output pin per hop.
 *
 * Wire order IS execution order. Returns false with *outError set (never
 * partially built) when there is no entry node or a node names a modifier type
 * with no registered behavior; a silent no-op modifier would be the kind of
 * quiet nothing this project refuses.
 */
template <typename GraphT>
bool BuildParticleChain(const GraphT& graph, uint32_t entryTypeId,
                        std::vector<ParticleChainEntry>& out, const char** outError)
{
    FreeParticleChain(out);

    const auto* node = graph.FindFirstOfType(entryTypeId);
    if (!node)
    {
        *outError = "graph has no Emitter node - nothing to run";
        return false;
    }

    for (int step = 0; step < kMaxParticleChainLength; ++step)
    {
        const auto* next = graph.Next(node->id, 0);
        if (!next)
            return !out.empty();   // End of the chain.

        const ParticleModifierOps* ops = ParticleModifierRegistry::Instance().Find(next->typeId);
        if (!ops)
        {
            FreeParticleChain(out);
            *outError = "graph uses a modifier type with no runtime behavior registered";
            return false;
        }

        ParticleChainEntry e;
        e.data = next->instance;
        e.ops  = ops;
        // Zero-initialized: every modifier's state starts at "nothing has
        // happened yet", which is what a fresh accumulator or latch means.
        e.state = ops->stateSize ? new uint8_t[ops->stateSize]{} : nullptr;
        out.push_back(e);

        node = next;
    }

    FreeParticleChain(out);
    *outError = "graph chain is longer than the guard allows (wired in a cycle?)";
    return false;
}
