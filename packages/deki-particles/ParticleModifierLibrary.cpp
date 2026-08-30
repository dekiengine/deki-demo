/**
 * @file ParticleModifierLibrary.cpp
 * @brief Runtime behavior (ParticleModifierOps) for the built-in modifier set.
 *
 * Data structs live in ParticleNodes.h; this file supplies each one's
 * callbacks and registers them keyed by node-name hash
 * (REGISTER_PARTICLE_MODIFIER). Per-emitter state goes in the chain's
 * zero-initialized blob (ops.stateSize), never in the shared data struct: many
 * emitters may run the same graph asset, and a rate accumulator kept in the
 * struct would have them all fighting over one counter.
 *
 * The hot loops here are the same loops these modifiers ran as components. The
 * graph is walked once, at chain-build time, and never touched again.
 */

#include "ParticleNodes.h"
#include "ParticleModifierRegistry.h"
#include "ParticleEmitterComponent.h"
#include "ParticleMath.h"

#include "DekiObject.h"

#include <cmath>
#include <cstdint>

namespace
{

// The `enabled` reader every modifier node shares. One template beats nine
// copies of the same cast.
template <typename T>
bool NodeEnabled(const void* data)
{
    return static_cast<const T*>(data)->enabled;
}

// ---------------------------------------------------------------------------
// Emission
// ---------------------------------------------------------------------------

// Per-emitter spawn bookkeeping. POD: the blob is zero-initialized, and zero
// is the correct starting value for all three.
struct EmissionState
{
    float rateAccumulator;
    float burstTimer;
    bool  firedFirstBurst;
};

void EmissionAttach(const void* /*data*/, void* state, ParticleEmitterComponent& /*emitter*/)
{
    auto* s = static_cast<EmissionState*>(state);
    s->rateAccumulator = 0.0f;
    s->burstTimer = 0.0f;
    s->firedFirstBurst = false;
}

void EmissionEmit(const void* data, void* /*state*/, ParticleEmitterComponent& emitter, int i)
{
    const auto* d = static_cast<const ParticleEmissionNode*>(data);

    // 1. Lifetime — random in [min, max], clamped to a sane minimum.
    float tN = emitter.rng.NextFloat01();
    float life = d->lifetimeMin + (d->lifetimeMax - d->lifetimeMin) * tN;
    static const float kMinLife = 0.001f;
    if (life < kMinLife) life = kMinLife;
    emitter.pool.lifetime[i] = life;

    // 2. Spawn position — sample the configured shape in emitter-local space,
    //    then add the emitter's world origin if worldSpace is on so the
    //    particle starts at the emitter's location.
    float ox = 0.0f, oy = 0.0f;
    switch (d->shape)
    {
        case EmitterShapeKind::Point:
            break;
        case EmitterShapeKind::Circle:
        {
            // Uniform sample inside disc: r = R*sqrt(u), theta = 2pi*v.
            // theta is in radians (engine convention).
            float u = emitter.rng.NextFloat01();
            float v = emitter.rng.NextFloat01();
            float r = d->radius * std::sqrt(u);
            float theta = v * DekiMath::kTwoPi;
            ox = r * std::cos(theta);
            oy = r * std::sin(theta);
            break;
        }
        case EmitterShapeKind::Rect:
        {
            float rx = emitter.rng.NextFloat01() - 0.5f;
            float ry = emitter.rng.NextFloat01() - 0.5f;
            ox = rx * d->width;
            oy = ry * d->height;
            break;
        }
    }

    if (emitter.worldSpace && emitter.GetOwner())
    {
        DekiObject* o = emitter.GetOwner();
        emitter.pool.posX[i] = o->GetWorldX() + ox;
        emitter.pool.posY[i] = o->GetWorldY() + oy;
    }
    else
    {
        emitter.pool.posX[i] = ox;
        emitter.pool.posY[i] = oy;
    }
}

void EmissionSimulate(const void* data, void* state, ParticleEmitterComponent& emitter, float dt)
{
    const auto* d = static_cast<const ParticleEmissionNode*>(data);
    auto* s = static_cast<EmissionState*>(state);

    // Continuous emission — accumulator-based so fractional rates work.
    if (d->emissionRate > 0.0f)
    {
        s->rateAccumulator += d->emissionRate * dt;
        while (s->rateAccumulator >= 1.0f)
        {
            s->rateAccumulator -= 1.0f;
            if (emitter.Spawn() < 0)
            {
                s->rateAccumulator = 0.0f;  // pool full, drop pending spawns
                break;
            }
        }
    }

    // Burst emission — independent of `emissionRate`. Both can run together
    // for "ambient plus occasional puff" effects (the inspector splits them
    // into separate groups so the relationship is visible).
    if (d->burstCount > 0)
    {
        if (!s->firedFirstBurst)
        {
            for (int n = 0; n < d->burstCount; ++n)
                if (emitter.Spawn() < 0) break;
            s->firedFirstBurst = true;
            s->burstTimer = 0.0f;
        }
        else if (d->burstInterval > 0.0f)
        {
            s->burstTimer += dt;
            while (s->burstTimer >= d->burstInterval)
            {
                s->burstTimer -= d->burstInterval;
                for (int n = 0; n < d->burstCount; ++n)
                    if (emitter.Spawn() < 0) break;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Initial velocity
// ---------------------------------------------------------------------------

void InitialVelocityEmit(const void* data, void* /*state*/, ParticleEmitterComponent& emitter, int i)
{
    const auto* d = static_cast<const ParticleInitialVelocityNode*>(data);
    float speed = emitter.rng.NextFloatRange(d->speedMin, d->speedMax);
    float angle = emitter.rng.NextFloatRange(d->angleMin, d->angleMax);
    // Angle in radians (engine convention).
    emitter.pool.velX[i] = speed * std::cos(angle);
    emitter.pool.velY[i] = speed * std::sin(angle);
}

// ---------------------------------------------------------------------------
// Initial rotation
// ---------------------------------------------------------------------------

void InitialRotationAttach(const void* /*data*/, void* /*state*/, ParticleEmitterComponent& emitter)
{
    emitter.pool.EnsureRotation();
}

void InitialRotationEmit(const void* data, void* /*state*/, ParticleEmitterComponent& emitter, int i)
{
    const auto* d = static_cast<const ParticleInitialRotationNode*>(data);
    // rotation/rotationSpeed are radians (engine convention).
    emitter.pool.rotation[i] = emitter.rng.NextFloatRange(d->rotationMin, d->rotationMax);
    emitter.pool.rotationSpeed[i] = emitter.rng.NextFloatRange(d->spinSpeedMin, d->spinSpeedMax);
}

void InitialRotationSimulate(const void* /*data*/, void* /*state*/, ParticleEmitterComponent& emitter, float dt)
{
    // Integrate spin so per-particle rotationSpeed has effect even when no
    // Rotation over Lifetime node is in the chain.
    int n = emitter.pool.AliveCount();
    float* rot = emitter.pool.rotation;
    float* spd = emitter.pool.rotationSpeed;
    for (int i = 0; i < n; ++i)
        rot[i] += spd[i] * dt;
}

// ---------------------------------------------------------------------------
// Gravity
// ---------------------------------------------------------------------------

void GravitySimulate(const void* data, void* /*state*/, ParticleEmitterComponent& emitter, float dt)
{
    const auto* d = static_cast<const ParticleGravityNode*>(data);
    int n = emitter.pool.AliveCount();
    float* vx = emitter.pool.velX;
    float* vy = emitter.pool.velY;
    // Fold dt into the delta once, outside the hot loop.
    float dvx = d->gravityX * dt;
    float dvy = d->gravityY * dt;
    for (int i = 0; i < n; ++i)
    {
        vx[i] += dvx;
        vy[i] += dvy;
    }
}

// ---------------------------------------------------------------------------
// Drag
// ---------------------------------------------------------------------------

void DragSimulate(const void* data, void* /*state*/, ParticleEmitterComponent& emitter, float dt)
{
    const auto* d = static_cast<const ParticleDragNode*>(data);
    // k = max(0, 1 - drag*dt)
    float k = 1.0f - d->drag * dt;
    if (k < 0.0f) k = 0.0f;
    int n = emitter.pool.AliveCount();
    float* vx = emitter.pool.velX;
    float* vy = emitter.pool.velY;
    for (int i = 0; i < n; ++i)
    {
        vx[i] *= k;
        vy[i] *= k;
    }
}

// ---------------------------------------------------------------------------
// Size over lifetime
// ---------------------------------------------------------------------------

void SizeOverLifetimeAttach(const void* /*data*/, void* /*state*/, ParticleEmitterComponent& emitter)
{
    emitter.pool.EnsureScale();
}

void SizeOverLifetimeSimulate(const void* data, void* /*state*/, ParticleEmitterComponent& emitter, float /*dt*/)
{
    const auto* d = static_cast<const ParticleSizeOverLifetimeNode*>(data);
    int n = emitter.pool.AliveCount();
    float* age  = emitter.pool.age;
    float* life = emitter.pool.lifetime;
    float* sc   = emitter.pool.scale;
    float s0 = d->sizeAt0;
    float ds = d->sizeAt1 - d->sizeAt0;
    for (int i = 0; i < n; ++i)
    {
        float t = (life[i] > 0.0f) ? (age[i] / life[i]) : 0.0f;
        if (t > 1.0f) t = 1.0f;
        sc[i] = s0 + ds * t;
    }
}

// ---------------------------------------------------------------------------
// Color over lifetime
// ---------------------------------------------------------------------------

void ColorOverLifetimeAttach(const void* /*data*/, void* /*state*/, ParticleEmitterComponent& emitter)
{
    emitter.pool.EnsureTint();
}

void ColorOverLifetimeSimulate(const void* data, void* /*state*/, ParticleEmitterComponent& emitter, float /*dt*/)
{
    const auto* d = static_cast<const ParticleColorOverLifetimeNode*>(data);
    int n = emitter.pool.AliveCount();
    float* age  = emitter.pool.age;
    float* life = emitter.pool.lifetime;
    uint8_t* tR = emitter.pool.tintR;
    uint8_t* tG = emitter.pool.tintG;
    uint8_t* tB = emitter.pool.tintB;
    uint8_t* tA = emitter.pool.tintA;

    int r0 = d->colorAt0.r, g0 = d->colorAt0.g, b0 = d->colorAt0.b, a0 = d->colorAt0.a;
    int dr = (int)d->colorAt1.r - r0;
    int dg = (int)d->colorAt1.g - g0;
    int db = (int)d->colorAt1.b - b0;
    int da = (int)d->colorAt1.a - a0;
    // 256 in float for the 8.8 lerp factor.
    static const float k256 = 256.0f;

    for (int i = 0; i < n; ++i)
    {
        float t = (life[i] > 0.0f) ? (age[i] / life[i]) : 0.0f;
        if (t > 1.0f) t = 1.0f;
        // 8.8 fixed-point lerp avoids the float-to-int truncation pattern in
        // the hot loop on MCUs without fast int-from-float.
        int ti = static_cast<int>(t * k256);
        tR[i] = (uint8_t)(r0 + ((dr * ti) >> 8));
        tG[i] = (uint8_t)(g0 + ((dg * ti) >> 8));
        tB[i] = (uint8_t)(b0 + ((db * ti) >> 8));
        tA[i] = (uint8_t)(a0 + ((da * ti) >> 8));
    }
}

// ---------------------------------------------------------------------------
// Rotation over lifetime
// ---------------------------------------------------------------------------

void RotationOverLifetimeAttach(const void* /*data*/, void* /*state*/, ParticleEmitterComponent& emitter)
{
    emitter.pool.EnsureRotation();
}

void RotationOverLifetimeSimulate(const void* data, void* /*state*/, ParticleEmitterComponent& emitter, float dt)
{
    const auto* d = static_cast<const ParticleRotationOverLifetimeNode*>(data);
    int n = emitter.pool.AliveCount();
    // rotation is float radians (engine convention); speeds are radians/sec,
    // so integration is a simple unit-agnostic accumulate.
    float a0 = d->spinSpeedAt0;
    float da = d->spinSpeedAt1 - a0;
    float* age  = emitter.pool.age;
    float* life = emitter.pool.lifetime;
    float* rot  = emitter.pool.rotation;
    for (int i = 0; i < n; ++i)
    {
        float t = (life[i] > 0.0f) ? (age[i] / life[i]) : 0.0f;
        if (t > 1.0f) t = 1.0f;
        float spd = a0 + da * t;
        rot[i] += spd * dt;
    }
}

} // namespace

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

REGISTER_PARTICLE_MODIFIER(ParticleEmissionNode, ([] {
    ParticleModifierOps ops;
    ops.stateSize  = sizeof(EmissionState);
    ops.onAttach   = &EmissionAttach;
    ops.onEmit     = &EmissionEmit;
    ops.onSimulate = &EmissionSimulate;
    ops.isEnabled  = &NodeEnabled<ParticleEmissionNode>;
    return ops;
}()));

REGISTER_PARTICLE_MODIFIER(ParticleInitialVelocityNode, ([] {
    ParticleModifierOps ops;
    ops.onEmit    = &InitialVelocityEmit;
    ops.isEnabled = &NodeEnabled<ParticleInitialVelocityNode>;
    return ops;
}()));

REGISTER_PARTICLE_MODIFIER(ParticleInitialRotationNode, ([] {
    ParticleModifierOps ops;
    ops.onAttach   = &InitialRotationAttach;
    ops.onEmit     = &InitialRotationEmit;
    ops.onSimulate = &InitialRotationSimulate;
    ops.isEnabled  = &NodeEnabled<ParticleInitialRotationNode>;
    return ops;
}()));

REGISTER_PARTICLE_MODIFIER(ParticleGravityNode, ([] {
    ParticleModifierOps ops;
    ops.onSimulate = &GravitySimulate;
    ops.isEnabled  = &NodeEnabled<ParticleGravityNode>;
    return ops;
}()));

REGISTER_PARTICLE_MODIFIER(ParticleDragNode, ([] {
    ParticleModifierOps ops;
    ops.onSimulate = &DragSimulate;
    ops.isEnabled  = &NodeEnabled<ParticleDragNode>;
    return ops;
}()));

REGISTER_PARTICLE_MODIFIER(ParticleSizeOverLifetimeNode, ([] {
    ParticleModifierOps ops;
    ops.onAttach   = &SizeOverLifetimeAttach;
    ops.onSimulate = &SizeOverLifetimeSimulate;
    ops.isEnabled  = &NodeEnabled<ParticleSizeOverLifetimeNode>;
    return ops;
}()));

REGISTER_PARTICLE_MODIFIER(ParticleColorOverLifetimeNode, ([] {
    ParticleModifierOps ops;
    ops.onAttach   = &ColorOverLifetimeAttach;
    ops.onSimulate = &ColorOverLifetimeSimulate;
    ops.isEnabled  = &NodeEnabled<ParticleColorOverLifetimeNode>;
    return ops;
}()));

REGISTER_PARTICLE_MODIFIER(ParticleRotationOverLifetimeNode, ([] {
    ParticleModifierOps ops;
    ops.onAttach   = &RotationOverLifetimeAttach;
    ops.onSimulate = &RotationOverLifetimeSimulate;
    ops.isEnabled  = &NodeEnabled<ParticleRotationOverLifetimeNode>;
    return ops;
}()));
