#include "ParticleEmitterComponent.h"
#include "ParticleNodes.h"
#include "ParticleSystem.h"
#include "DekiObject.h"
#include "DekiTime.h"
#include "DekiLogSystem.h"
#include "deki-2d/Texture2D.h"
#include "deki-rendering/QuadBlit.h"
#include "DekiEngine.h"  // for DekiColorFormat enum
#include <cmath>
#include <cstring>
#include <cstdint>

ParticleEmitterComponent::ParticleEmitterComponent()
{
    SetNeedsUpdate(true);
    rng.Seed((uint32_t)(uintptr_t)this);
}

ParticleEmitterComponent::~ParticleEmitterComponent()
{
    ParticleSystem::GetInstance().UnregisterEmitter(this);
    FreeBboxBuf();
    FreeChain();
}

void ParticleEmitterComponent::Awake()
{
    ParticleSystem::GetInstance().RegisterEmitter(this);
}

void ParticleEmitterComponent::Start()
{
    EnsureReady();
}

void ParticleEmitterComponent::EnsureReady()
{
    EnsurePoolAllocated();
    if (m_ChainAttached) return;

    if (!RebuildChain())
        return;  // No graph loaded yet — try again next tick.

    // Give every modifier a chance to allocate its private state and request
    // pool extension columns now that capacity is known.
    for (const ParticleChainEntry& e : m_Chain)
        if (e.ops->onAttach) e.ops->onAttach(e.data, e.state, *this);
    m_ChainAttached = true;
}

void ParticleEmitterComponent::EnsurePoolAllocated()
{
    if (m_PoolAllocated && pool.Capacity() == maxParticles)
        return;
    pool.SetCapacity(maxParticles);
    m_PoolAllocated = true;
}

void ParticleEmitterComponent::FreeChain()
{
    FreeParticleChain(m_Chain);
}

bool ParticleEmitterComponent::RebuildChain()
{
    FreeChain();

    ParticleGraph* g = graph.Get();
    if (!g || !g->data)
        return false;   // Not loaded yet. Not an error: assets resolve later.

    const char* error = nullptr;
    if (BuildParticleChain(g->data->Root(),
                           DekiHashString(ParticleEmitNode::StaticNodeName),
                           m_Chain, &error))
        return true;

    if (error && !m_LoggedBadGraph)
    {
        DEKI_LOG_ERROR("ParticleEmitterComponent: %s", error);
        m_LoggedBadGraph = true;
    }
    return false;
}

#ifdef DEKI_EDITOR
void ParticleEmitterComponent::AdoptChain(std::vector<ParticleChainEntry>&& chain)
{
    FreeChain();
    m_Chain = std::move(chain);
    EnsurePoolAllocated();
    for (const ParticleChainEntry& e : m_Chain)
        if (e.ops->onAttach) e.ops->onAttach(e.data, e.state, *this);
    m_ChainAttached = true;
}
#endif

int ParticleEmitterComponent::Spawn()
{
    int idx = pool.Spawn();
    if (idx < 0) return -1;

    // Reset particle state for this slot. All numeric state is float.
    pool.posX[idx] = 0.0f;
    pool.posY[idx] = 0.0f;
    pool.velX[idx] = 0.0f;
    pool.velY[idx] = 0.0f;
    pool.age[idx] = 0.0f;
    pool.lifetime[idx] = 1.0f;
    if (pool.HasRotation())      { pool.rotation[idx] = 0.0f; pool.rotationSpeed[idx] = 0.0f; }
    if (pool.HasScale())         { pool.scale[idx] = 1.0f; }
    if (pool.HasTint())          { pool.tintR[idx] = 255; pool.tintG[idx] = 255; pool.tintB[idx] = 255; pool.tintA[idx] = 255; }

    // Drive onEmit through every ENABLED modifier in chain order. The
    // Emission node is the typical caller; the spawn-time setters wired after
    // it read the lifetime it set and write initial pos/vel/rot.
    for (const ParticleChainEntry& e : m_Chain)
    {
        if (!e.ops->onEmit) continue;
        if (e.ops->isEnabled && !e.ops->isEnabled(e.data)) continue;
        e.ops->onEmit(e.data, e.state, *this, idx);
    }

    return idx;
}

void ParticleEmitterComponent::Update()
{
    float dt = DekiTime::GetDeltaTimeF() / 1000.0f;
    Simulate(dt);
}

void ParticleEmitterComponent::Simulate(float dt)
{
    // Self-bootstrap so the editor preview path works without Start() ever
    // having fired (Play mode is the only context where the engine drives
    // the lifecycle).
    EnsureReady();

    if (m_Chain.empty())
        return;

    if (dt <= 0.0f) return;
    if (dt > 0.1f) dt = 0.1f;  // Clamp huge deltas (loading, paused, etc.) to avoid teleport.

    // Age + kill expired particles BEFORE driving simulation hooks. This way
    // OnSimulate iterates only currently-alive particles. age/lifetime are
    // float; convert dt once before the loop.
    float dtN = static_cast<float>(dt);
    int n = pool.AliveCount();
    for (int i = 0; i < n; )
    {
        pool.age[i] = ((pool.age[i]) + (dtN));
        if (pool.age[i] >= pool.lifetime[i])
        {
            pool.KillSwap(i);
            n--;
            continue;
        }
        ++i;
    }

    for (const ParticleChainEntry& e : m_Chain)
    {
        if (!e.ops->onSimulate) continue;
        if (e.ops->isEnabled && !e.ops->isEnabled(e.data)) continue;
        e.ops->onSimulate(e.data, e.state, *this, dt);
    }

    // Final kinematic integration (pos += vel * dt) — runs once after all
    // force modifiers have mutated velocity. Always happens, even with no
    // modifiers attached, so that a programmatic Spawn() with non-zero
    // velocity still moves.
    int alive = pool.AliveCount();
    float* px = pool.posX; float* py = pool.posY;
    float* vx = pool.velX; float* vy = pool.velY;
    // dtN was already converted from `dt` earlier in this function for the
    // age-aging loop — reuse it here instead of redefining.
    for (int i = 0; i < alive; ++i)
    {
        px[i] = ((px[i]) + (((vx[i]) * (dtN))));
        py[i] = ((py[i]) + (((vy[i]) * (dtN))));
    }
}

#ifdef DEKI_EDITOR
void ParticleEmitterComponent::EditorPreviewRestart()
{
    // Wipe live particles. We don't free the pool — capacity stays so the
    // user's previewed particle count survives across restarts.
    if (m_PoolAllocated)
    {
        // Drain to zero alive without touching column pointers.
        while (pool.AliveCount() > 0)
            pool.KillSwap(pool.AliveCount() - 1);
    }
    // Rebuild the chain from the currently loaded graph, so every modifier's
    // state blob starts over (the burst latch, the rate accumulator) and a
    // reimported graph asset takes effect. Clearing the attach flag makes
    // EnsureReady re-run the whole pass.
    m_ChainAttached = false;
    m_LoggedBadGraph = false;
    EnsureReady();
}
#endif

void ParticleEmitterComponent::UnloadAssets()
{
    sprite.ptr = nullptr;
    sprite.loadAttempted = false;

    // The chain points INTO the graph asset's node instances, so it cannot
    // outlive the asset. Drop it before releasing the reference.
    FreeChain();
    m_ChainAttached = false;
    graph.ptr = nullptr;
    graph.loadAttempted = false;

    FreeBboxBuf();
    m_LoggedMissingSprite = false;
    m_LoggedBadGraph = false;
}

void ParticleEmitterComponent::FreeBboxBuf()
{
    delete[] m_BboxBuf;
    m_BboxBuf = nullptr;
    m_BboxBufBytes = 0;
}

bool ParticleEmitterComponent::RenderContent(const DekiObject* owner,
                                              QuadBlit::Source& outSource,
                                              float& outPivotX,
                                              float& outPivotY,
                                              uint8_t& outTintR,
                                              uint8_t& outTintG,
                                              uint8_t& outTintB,
                                              uint8_t& outTintA)
{
    outTintR = outTintG = outTintB = outTintA = 255;

    Sprite* spr = sprite.Get();
    if (!spr || !spr->data)
    {
        if (!m_LoggedMissingSprite)
        {
            DEKI_LOG_ERROR("ParticleEmitterComponent: no sprite assigned — emitter renders nothing");
            m_LoggedMissingSprite = true;
        }
        return false;
    }

    int n = pool.AliveCount();
    if (n <= 0)
        return false;

    // Anchor for converting particle positions to bbox-local coordinates.
    // worldSpace=true: particles store world coords; subtract emitter's world
    // origin so the bbox sits at the emitter's location after final transform.
    // worldSpace=false: particles store emitter-local coords; no subtraction.
    float anchorX = 0.0f, anchorY = 0.0f;
    if (worldSpace && owner)
    {
        anchorX = (owner->GetWorldX());
        anchorY = (owner->GetWorldY());
    }

    const float spriteW = (float)spr->width;
    const float spriteH = (float)spr->height;
    // Rotation expands AABB up to sqrt(2). 1.45 leaves a one-pixel guard band.
    constexpr float kRotPad = 1.45f;

    // First pass: tight integer bbox over all alive particles. The rasterizer
    // boundary uses float pixels; convert float positions/scale once per
    // particle and stay in float for the bbox arithmetic.
    float minX =  1e9f, minY =  1e9f;
    float maxX = -1e9f, maxY = -1e9f;
    for (int i = 0; i < n; ++i)
    {
        float lx = (pool.posX[i]) - anchorX;
        float ly = (pool.posY[i]) - anchorY;
        float s  = pool.HasScale() ? (pool.scale[i]) : 1.0f;
        float halfW = 0.5f * spriteW * s * kRotPad;
        float halfH = 0.5f * spriteH * s * kRotPad;
        if (lx - halfW < minX) minX = lx - halfW;
        if (ly - halfH < minY) minY = ly - halfH;
        if (lx + halfW > maxX) maxX = lx + halfW;
        if (ly + halfH > maxY) maxY = ly + halfH;
    }

    int bboxMinX = (int)std::floor(minX);
    int bboxMinY = (int)std::floor(minY);
    int bboxMaxX = (int)std::ceil(maxX);
    int bboxMaxY = (int)std::ceil(maxY);
    int bboxW = bboxMaxX - bboxMinX;
    int bboxH = bboxMaxY - bboxMinY;
    if (bboxW <= 0 || bboxH <= 0)
        return false;

    // ARGB8888 intermediate (4 bytes/pixel). QuadBlit's only RGB565-family
    // target is plain RGB565 with no alpha, which can't accumulate alpha-blended
    // particles. ARGB8888 is the smallest target format that preserves alpha.
    const int bytesPerPixel = 4;
    int needBytes = bboxW * bboxH * bytesPerPixel;
    if (needBytes > m_BboxBufBytes)
    {
        delete[] m_BboxBuf;
        m_BboxBuf = new uint8_t[needBytes];
        m_BboxBufBytes = needBytes;
    }
    // memset to 0 → alpha=0 (fully transparent) regardless of byte order.
    std::memset(m_BboxBuf, 0, needBytes);

    // Source descriptor for the sprite — same for every particle blit.
    bool isRGB565 = (spr->format == Texture2D::TextureFormat::RGB565 ||
                     spr->format == Texture2D::TextureFormat::RGB565A8);
    int srcBpp = Texture2D::GetBytesPerPixel(spr->format);
    QuadBlit::Source src = QuadBlit::MakeSource(
        spr->data, spr->width, spr->height,
        srcBpp, spr->has_alpha, isRGB565,
        /*ownsPixels=*/false,
        spr->alphaRowSpans);
    if (spr->has_chroma_key)
    {
        src.hasChromaKey = true;
        if (isRGB565)
        {
            src.keyR = (uint8_t)((spr->transparent_r >> 3) << 3);
            src.keyG = (uint8_t)((spr->transparent_g >> 2) << 2);
            src.keyB = (uint8_t)((spr->transparent_b >> 3) << 3);
        }
        else
        {
            src.keyR = spr->transparent_r;
            src.keyG = spr->transparent_g;
            src.keyB = spr->transparent_b;
        }
        src.chromaRowSpans = spr->chromaRowSpans;
    }

    // Per-particle blit into the bbox buffer. The bbox buffer's clip stack
    // is independent of the framebuffer's; we disable clip enforcement
    // for this nested render so global scene clips don't accidentally
    // suppress particles outside the scene view.
    bool prevClipEnabled = QuadBlit::IsClipEnabled();
    QuadBlit::SetClipEnabled(false);

    for (int i = 0; i < n; ++i)
    {
        float lx = (pool.posX[i]) - anchorX - (float)bboxMinX;
        float ly = (pool.posY[i]) - anchorY - (float)bboxMinY;
        float s  = pool.HasScale() ? (pool.scale[i]) : 1.0f;
        // rotation column is float radians; QuadBlit takes float radians.
        float r  = pool.HasRotation() ? (pool.rotation[i]) : 0.0f;
        uint8_t tR = pool.HasTint() ? pool.tintR[i] : 255;
        uint8_t tG = pool.HasTint() ? pool.tintG[i] : 255;
        uint8_t tB = pool.HasTint() ? pool.tintB[i] : 255;
        uint8_t tA = pool.HasTint() ? pool.tintA[i] : 255;
        if (tA == 0) continue;

        QuadBlit::Blit(
            src,
            m_BboxBuf, bboxW, bboxH, DekiColorFormat::ARGB8888,
            (int32_t)lx, (int32_t)ly,
            s, s, r,
            0.5f, 0.5f,
            tR, tG, tB, tA);
    }

    QuadBlit::SetClipEnabled(prevClipEnabled);

    // Hand the bbox buffer back to the framework. The framework owns nothing —
    // we keep m_BboxBuf around for next frame, so ownsPixels=false.
    outSource = QuadBlit::MakeSource(
        m_BboxBuf, bboxW, bboxH,
        bytesPerPixel,
        /*hasAlpha=*/true,
        /*isRGB565=*/false,
        /*ownsPixels=*/false);

    // Pivot is the emitter's local origin within the bbox.
    outPivotX = (bboxW > 0) ? (-(float)bboxMinX / (float)bboxW) : 0.5f;
    outPivotY = (bboxH > 0) ? (-(float)bboxMinY / (float)bboxH) : 0.5f;
    return true;
}
