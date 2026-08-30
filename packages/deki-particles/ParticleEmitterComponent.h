#pragma once

#include "DekiParticlesAPI.h"
#include "deki-rendering/RendererComponent.h"
#include "deki-2d/Sprite.h"
#include "assets/AssetRef.h"
#include "reflection/DekiProperty.h"
#include "ParticlePool.h"
#include "ParticleMath.h"
#include "ParticleGraph.h"
#include "ParticleChain.h"
#include <vector>

/**
 * @brief Renderable particle emitter.
 *
 * Owns a fixed-size particle pool. All authoring (emission rate, spawn shape,
 * gravity, color/size/rotation curves, etc.) lives in a ParticleGraph asset:
 * an Emitter node followed by a chain of modifier nodes, wired in the order
 * they should run. The emitter walks that graph ONCE (EnsureReady) into a flat
 * list of callbacks; per particle, the cost is exactly the modifiers wired,
 * with no graph interpretation in the loop.
 *
 * One graph drives any number of emitters. The node instances holding the
 * tuning values belong to the asset and are shared; each emitter owns only the
 * small per-modifier state blobs its chain allocated.
 *
 * Render path: rasterizes all alive particles into a single intermediate
 * RGB565A8 buffer sized to the tight screen-space bounding box and returns
 * one QuadBlit::Source. The framework does the final blit at the emitter's
 * world transform — sort order is per-emitter, never per-particle.
 *
 * Per CLAUDE.md "NEVER USE FALLBACKS": no sprite ⇒ renders nothing, logs
 * once. No graph ⇒ no particles ever spawn. A graph naming a modifier type
 * with no registered behavior refuses to build its chain, loudly. maxParticles
 * is honored exactly.
 */
class DEKI_PARTICLES_API ParticleEmitterComponent : public RendererComponent
{
public:
    DEKI_COMPONENT(ParticleEmitterComponent, RendererComponent, "Particles", "b1e0e1a0-1111-4002-9002-000000000010", "DEKI_FEATURE_PARTICLE_EMITTER")
    DEKI_DESCRIPTION("Spawns and draws particles, following a particle graph asset.")

    DEKI_EXPORT
    Deki::AssetRef<Sprite> sprite;

    // The effect recipe. Assign a ".asset" of type "ParticleGraph", authored
    // in the Node Graph window. No graph means no chain and no particles.
    DEKI_EXPORT
    Deki::AssetRef<ParticleGraph> graph;

    DEKI_EXPORT
    DEKI_RANGE(0, 4096)
    int32_t maxParticles = 64;

    DEKI_EXPORT
    bool playOnAwake = true;

    DEKI_EXPORT
    bool looping = true;

    DEKI_EXPORT
    bool worldSpace = true;

    ParticleEmitterComponent();
    virtual ~ParticleEmitterComponent();

    void Awake() override;
    void Start() override;
    void Update() override;
    void UnloadAssets() override;

    bool RenderContent(const DekiObject* owner,
                       QuadBlit::Source& outSource,
                       float& outPivotX,
                       float& outPivotY,
                       uint8_t& outTintR,
                       uint8_t& outTintG,
                       uint8_t& outTintB,
                       uint8_t& outTintA) override;

    // Public so modifiers can read/write directly. Hot-path inner loops touch
    // these without going through accessors.
    deki_particles::ParticlePool pool;
    deki_particles::Xorshift32   rng;

    // Walk the graph asset into m_Chain (see ParticleChain.h). Called by
    // EnsureReady; call it directly after assigning a different graph asset.
    // Returns false (leaving the chain empty) when there is no graph to walk yet.
    bool RebuildChain();

    const std::vector<ParticleChainEntry>& Chain() const { return m_Chain; }

#ifdef DEKI_EDITOR
    // Take a chain built elsewhere and run its attach pass. The editor preview
    // uses this to drive a graph that has no asset yet: the one being edited.
    // Takes ownership of the state blobs.
    void AdoptChain(std::vector<ParticleChainEntry>&& chain);
#endif

    // Spawn one particle. Returns its index in [0, AliveCount), or -1 if full.
    // Calls OnEmit on every modifier in phase order (including the modifier
    // that called Spawn — modifiers above its phase will not see this
    // particle until next frame, which is intentional and consistent).
    int  Spawn();

    // Single tick of simulation: age, kill, dispatch modifiers, integrate.
    // Update() calls this with the engine's frame delta. The editor preview
    // path also calls it (with the editor's frame delta) so emitters animate
    // in edit mode without needing Play.
    void Simulate(float dt);

#ifdef DEKI_EDITOR
    // Editor-only preview controls. State is NOT serialized.
    bool IsEditorPreviewPlaying() const { return m_EditorPreviewPlaying; }
    void EditorPreviewSetPlaying(bool play) { m_EditorPreviewPlaying = play; }
    // Kill all live particles and rebuild the chain from the graph, which
    // resets every modifier's state (the burst latch, the rate accumulator).
    void EditorPreviewRestart();
#endif

    // Pool allocation + chain build + per-modifier onAttach bootstrap,
    // factored out of Start() so the editor preview path can run it in edit
    // mode (where Start() never fires). Idempotent.
    void EnsureReady();

private:
    bool m_PoolAllocated = false;
    bool m_ChainAttached = false;
    bool m_LoggedMissingSprite = false;
    bool m_LoggedBadGraph = false;
#ifdef DEKI_EDITOR
    bool m_EditorPreviewPlaying = false;
#endif

    // Render-side persistent buffer (grows to fit, never shrinks for jitter).
    uint8_t* m_BboxBuf = nullptr;
    int      m_BboxBufBytes = 0;

    // The built chain, in wire order.
    std::vector<ParticleChainEntry> m_Chain;

    void EnsurePoolAllocated();
    void FreeBboxBuf();
    void FreeChain();
};

// Generated property metadata
#include "generated/ParticleEmitterComponent.gen.h"
