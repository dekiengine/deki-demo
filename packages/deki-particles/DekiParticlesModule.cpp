/**
 * @file DekiParticlesPackage.cpp
 * @brief Package entry point for deki-particles DLL
 *
 * Exports the standard Deki plugin interface so the editor can load
 * deki-particles.dll and register its one component (the emitter) plus the
 * particle-graph node vocabulary: the Emitter entry and the built-in modifier
 * nodes (Emission, Initial Velocity, Initial Rotation, Gravity, Drag, and the
 * Size / Color / Rotation over Lifetime trio).
 *
 * External particle packages ship modifiers the same way: declare a DEKI_NODE
 * struct in a category starting "Particles/", register its ParticleModifierOps
 * with REGISTER_PARTICLE_MODIFIER, and register the node type from your own
 * package entry. No changes to deki-particles required.
 */

#include "interop/DekiPlugin.h"
#include "DekiPackageFeatureMeta.h"
#include "ParticleEmitterComponent.h"
#include "ParticleNodes.h"
#include "ParticleSystem.h"
#include "reflection/ComponentRegistry.h"
#include "reflection/ComponentFactory.h"
#include "deki-nodegraph/DekiNode.h"   // NodeFactory + NodeTypeRegistry (editor)

#ifdef DEKI_EDITOR

extern void DekiParticles_RegisterComponents();
extern int  DekiParticles_GetAutoComponentCount();
extern const DekiComponentMeta* DekiParticles_GetAutoComponentMeta(int index);

// Defined in editor/ParticleGraphEditor.cpp (re-registers the graph domain).
extern "C" void DekiParticles_RegisterEditorGraphDomain(void);

static bool s_ParticlesRegistered = false;

namespace
{
    // Re-runnable mirror of the generated REGISTER_RUNTIME_NODE/REGISTER_NODE
    // static registrars. Those run once at DLL load; the editor's plugin-only
    // hot reload wipes the shared node registries WITHOUT unloading this
    // package, so registration must be repeatable on demand. NodeFactory
    // overwrites by typeId and NodeTypeRegistry dedupes, so this is idempotent.
    template<typename T>
    void RegisterParticleNodeType()
    {
        SceneFormat::NodeFactory::Instance().Register(
            DekiHashString(T::StaticNodeName),
            []() -> void* { return new T(); },
            [](void* p, SceneFormat::SceneMsgPackParser& parser, uint32_t mapSize) -> bool {
                return DeserializeMsgPack(*static_cast<T*>(p), parser, mapSize); },
            [](void* p) { delete static_cast<T*>(p); });
        NodeTypeRegistry::Instance().Register(&T::GetNodeMeta(), sizeof(DekiNodeMeta));
    }
}

extern "C" {

/**
 * @brief (Re-)register this package's node graph types: modifier node
 * factories, editor metas, and the Particles graph domain. Called at package
 * load via DekiPlugin_RegisterComponents and again after any registry wipe
 * that keeps this DLL loaded (plugin-only hot reload).
 */
DEKI_PARTICLES_API void DekiParticles_RegisterGraphTypes(void)
{
    RegisterParticleNodeType<ParticleEmitNode>();
    RegisterParticleNodeType<ParticleEmissionNode>();
    RegisterParticleNodeType<ParticleInitialVelocityNode>();
    RegisterParticleNodeType<ParticleInitialRotationNode>();
    RegisterParticleNodeType<ParticleGravityNode>();
    RegisterParticleNodeType<ParticleDragNode>();
    RegisterParticleNodeType<ParticleSizeOverLifetimeNode>();
    RegisterParticleNodeType<ParticleColorOverLifetimeNode>();
    RegisterParticleNodeType<ParticleRotationOverLifetimeNode>();
    DekiParticles_RegisterEditorGraphDomain();
}

DEKI_PARTICLES_API int DekiParticles_EnsureRegistered(void)
{
    if (s_ParticlesRegistered)
        return DekiParticles_GetAutoComponentCount();
    s_ParticlesRegistered = true;
    DekiParticles_RegisterComponents();
    return DekiParticles_GetAutoComponentCount();
}

DEKI_PLUGIN_API const char* DekiPlugin_GetName(void)
{
    return "Deki Particles Package";
}

DEKI_PLUGIN_API const char* DekiPlugin_GetVersion(void)
{
#ifdef DEKI_PACKAGE_VERSION
    return DEKI_PACKAGE_VERSION;
#else
    return "0.0.0-dev";
#endif
}

DEKI_PLUGIN_API const char* DekiPlugin_GetReflectionJson(void)
{
    return "{}";
}

DEKI_PLUGIN_API int DekiPlugin_Init(void)
{
    return 0;
}

DEKI_PLUGIN_API void DekiPlugin_Shutdown(void)
{
    s_ParticlesRegistered = false;
    ParticleSystem::GetInstance().ClearAll();
}

DEKI_PLUGIN_API int DekiPlugin_GetComponentCount(void)
{
    return DekiParticles_GetAutoComponentCount();
}

DEKI_PLUGIN_API const DekiComponentMeta* DekiPlugin_GetComponentMeta(int index)
{
    return DekiParticles_GetAutoComponentMeta(index);
}

DEKI_PLUGIN_API void DekiPlugin_RegisterComponents(void)
{
    DekiParticles_EnsureRegistered();
    // Deliberately OUTSIDE the s_ParticlesRegistered latch: node registries
    // are wiped on every hot reload (full or plugin-only) and this export is
    // the re-registration path for the plugin-only case.
    DekiParticles_RegisterGraphTypes();
}

DEKI_PLUGIN_API void DekiPlugin_OnPlayModeStop(void)
{
    ParticleSystem::GetInstance().ClearAll();
}


// =============================================================================
// Package Feature API
// =============================================================================

// One component now: the modifiers are graph node types, not components.
static const char* s_ParticleGuids[] = {
    ParticleEmitterComponent::StaticGuid,
};

static const DekiPackageFeatureInfo s_Features[] = {
    {
        "particle-emitter", "Particle Emitter",
        "Particle system: an emitter driven by a graph of behavior modifiers",
        true, "DEKI_FEATURE_PARTICLE_EMITTER",
        s_ParticleGuids,
        sizeof(s_ParticleGuids) / sizeof(s_ParticleGuids[0])
    },
};

DEKI_PLUGIN_API int DekiPlugin_GetFeatureCount(void)
{
    return sizeof(s_Features) / sizeof(s_Features[0]);
}

DEKI_PLUGIN_API const DekiPackageFeatureInfo* DekiPlugin_GetFeature(int index)
{
    if (index < 0 || index >= DekiPlugin_GetFeatureCount())
        return nullptr;
    return &s_Features[index];
}

// Package-specific feature API (for linked-DLL access without name conflicts)
DEKI_PARTICLES_API const char* DekiParticles_GetName(void)        { return "Particles"; }
DEKI_PARTICLES_API int DekiParticles_GetFeatureCount(void)        { return DekiPlugin_GetFeatureCount(); }
DEKI_PARTICLES_API const DekiPackageFeatureInfo* DekiParticles_GetFeature(int index) { return DekiPlugin_GetFeature(index); }

} // extern "C"

#endif // DEKI_EDITOR
