#pragma once

#include "DekiParticlesAPI.h"
#include "DekiComponent.h"   // DekiHashString (used by the registration macro)

#include <cstddef>
#include <cstdint>
#include <unordered_map>

class ParticleEmitterComponent;

/**
 * @brief Runtime behavior for one particle modifier node type.
 *
 * Modifier DATA lives in the reflected node structs of ParticleNodes.h, which
 * belong to the graph asset and are shared by every emitter using it. Anything
 * a modifier must remember between frames therefore goes in a per-emitter
 * blob: `stateSize` bytes, zero-initialized when the chain is built, handed
 * back to every callback. Same split as the FSM's action library.
 *
 * All callbacks are optional. A modifier that only sets up new particles fills
 * in onEmit; one that only pushes existing particles around fills in
 * onSimulate; one that needs a pool column (rotation, scale, tint) asks for it
 * in onAttach, where the capacity is already known.
 *
 * isEnabled reads the node's own `enabled` field. It is a hook rather than a
 * fixed offset because node structs are plain reflected data with no common
 * base, and the reflected property table is editor-only (a device build has no
 * metadata to look the field up in).
 */
struct ParticleModifierOps
{
    size_t stateSize = 0;

    // Once, when the chain is built: allocate private state, request pool
    // columns. Called again on an editor preview restart.
    void (*onAttach)(const void* data, void* state, ParticleEmitterComponent& emitter) = nullptr;

    // A particle was just spawned at index i. Position and velocity are zero;
    // lifetime is valid only if an emission node ahead of this one set it.
    void (*onEmit)(const void* data, void* state, ParticleEmitterComponent& emitter, int i) = nullptr;

    // Once per frame. Iterate the alive range [0, pool.AliveCount()).
    // Emission modifiers spawn from here.
    void (*onSimulate)(const void* data, void* state, ParticleEmitterComponent& emitter, float dt) = nullptr;

    // The node's authoring toggle. Null counts as always enabled.
    bool (*isEnabled)(const void* data) = nullptr;
};

/**
 * @brief typeId (DekiHashString of the node name) -> runtime ops.
 *
 * The data structs self-register into NodeFactory via their generated code;
 * this registry carries the behavior half. A node type in a "Particles/"
 * category with no entry here is a graph the emitter refuses to run, loudly:
 * silently skipping it would be a modifier that does nothing for no visible
 * reason.
 */
class DEKI_PARTICLES_API ParticleModifierRegistry
{
public:
    static ParticleModifierRegistry& Instance();

    void Register(uint32_t typeId, const ParticleModifierOps& ops) { m_Ops[typeId] = ops; }

    const ParticleModifierOps* Find(uint32_t typeId) const
    {
        auto it = m_Ops.find(typeId);
        return it != m_Ops.end() ? &it->second : nullptr;
    }

private:
    ParticleModifierRegistry() = default;
    std::unordered_map<uint32_t, ParticleModifierOps> m_Ops;
};

// Register runtime ops for a modifier node struct (place at file scope in a
// .cpp, next to the callbacks). ClassName must be a DEKI_NODE type; the key is
// the hash of its node name, matching what the graph loader stores.
#define REGISTER_PARTICLE_MODIFIER(ClassName, Ops) \
    static struct ClassName##_ParticleModifierRegistrar { \
        ClassName##_ParticleModifierRegistrar() { \
            ParticleModifierRegistry::Instance().Register( \
                DekiHashString(ClassName::StaticNodeName), Ops); \
        } \
    } s_##ClassName##_ParticleModifierRegistrar
