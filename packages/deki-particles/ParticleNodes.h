#pragma once

#include <cstdint>

#include "DekiMath.h"
#include "Color.h"
#include "deki-nodegraph/DekiNode.h"

// Node vocabulary for the particle graph ("Particles" domain).
//
// A particle graph is the RECIPE for an effect: the Emitter node, then a chain
// of modifier nodes wired one to the next. ParticleEmitterComponent references
// the graph asset and walks it ONCE when it starts, building a flat list of
// modifier callbacks. Nothing here is interpreted per particle: the per-frame
// cost is exactly the cost of the modifiers you wired, the same code that ran
// when these were sibling components.
//
// WIRE ORDER IS EXECUTION ORDER. The chain runs from the Emitter's output
// forward, and that is the only ordering authority — there are no hidden
// phases. The order that makes sense is the one these categories are listed
// in: Emission first (it spawns the particles everything else acts on), then
// the Initial nodes (they set a new particle's starting state), then Forces,
// then the Over Lifetime nodes. Wiring a spawn-time node ahead of Emission is
// legal and simply means it does not see the particles Emission spawns until
// the following frame.
//
// Data here is SHARED by every emitter using the graph. Anything a modifier
// needs to remember between frames (a rate accumulator, a burst latch) lives
// in the per-emitter state blob the chain allocates, never in these structs.
// See ParticleModifierRegistry.h.
//
// A new modifier type is a struct here (plus its generated include at the
// bottom), an ops registration in ParticleModifierLibrary.cpp, and a line in
// DekiParticles_RegisterGraphTypes. Another package can add one the same way
// with no change to deki-particles: its category just has to start "Particles/".

// ---------------------------------------------------------------------------
// Flow
// ---------------------------------------------------------------------------

// Where the chain begins. Permanent: seeded into every particle graph, absent
// from the add menu, not deletable. Wire its output to the first modifier.
struct ParticleEmitNode
{
    DEKI_NODE(ParticleEmitNode, "ParticleEmit", "Particles/Flow")
    static constexpr const char* StaticNodeDisplayName = "Emitter";
    static constexpr const char* StaticNodeDescription = "Where the chain starts. Wire it to the first modifier.";
    DEKI_NODE_OUTPUTS("chain")
    DEKI_NODE_PERMANENT()
};

// ---------------------------------------------------------------------------
// Emission
// ---------------------------------------------------------------------------

enum class EmitterShapeKind : uint8_t
{
    Point  = 0,
    Circle = 1,
    Rect   = 2,
};

// Spawning: how often, where, and for how long. Continuous and burst are
// additive rather than exclusive — a rate for steady output, a burst count for
// puffs, both for "ambient plus the occasional gust".
struct ParticleEmissionNode
{
    DEKI_NODE(ParticleEmissionNode, "ParticleEmission", "Particles/Emission")
    static constexpr const char* StaticNodeDisplayName = "Emission";
    static constexpr const char* StaticNodeDescription = "How often particles spawn, where, and how long they live.";
    DEKI_NODE_INPUTS("in")
    DEKI_NODE_OUTPUTS("next")
public:
    DEKI_EXPORT bool enabled = true;

    // ---- Lifetime ----------------------------------------------------------
    DEKI_GROUP("Lifetime")
    DEKI_EXPORT
    DEKI_RANGE(0.01f, 60.0f)
    float lifetimeMin = 1.0f;

    DEKI_EXPORT
    DEKI_RANGE(0.01f, 60.0f)
    float lifetimeMax = 1.0f;

    // ---- Continuous --------------------------------------------------------
    DEKI_GROUP("Continuous")
    DEKI_EXPORT
    DEKI_RANGE(0, 1000)
    float emissionRate = 20.0f;       // particles/second (0 = disable continuous)

    // ---- Burst -------------------------------------------------------------
    DEKI_GROUP("Burst")
    DEKI_EXPORT
    DEKI_RANGE(0, 1000)
    int32_t burstCount = 0;           // particles per burst (0 = no burst)

    DEKI_EXPORT
    DEKI_RANGE(0, 60)
    float burstInterval = 0.0f;       // seconds between bursts (0 = single burst at start)

    // ---- Shape -------------------------------------------------------------
    // Sampled in emitter-local space; the emitter's world position is added on
    // spawn when worldSpace is on.
    DEKI_GROUP("Shape")
    DEKI_EXPORT
    EmitterShapeKind shape = EmitterShapeKind::Point;

    DEKI_EXPORT
    DEKI_VISIBLE_WHEN(shape, Circle)
    DEKI_RANGE(0.0f, 50.0f)
    DEKI_UNIT(Distance)
    float radius = 0.0f;

    DEKI_EXPORT
    DEKI_VISIBLE_WHEN(shape, Rect)
    DEKI_RANGE(0.0f, 50.0f)
    DEKI_UNIT(Distance)
    float width = 0.0f;

    DEKI_EXPORT
    DEKI_VISIBLE_WHEN(shape, Rect)
    DEKI_RANGE(0.0f, 50.0f)
    DEKI_UNIT(Distance)
    float height = 0.0f;
};

// ---------------------------------------------------------------------------
// Initial state (runs when a particle is born)
// ---------------------------------------------------------------------------

struct ParticleInitialVelocityNode
{
    DEKI_NODE(ParticleInitialVelocityNode, "ParticleInitialVelocity", "Particles/Initial")
    static constexpr const char* StaticNodeDisplayName = "Initial Velocity";
    static constexpr const char* StaticNodeDescription = "Gives each new particle a starting speed and direction.";
    DEKI_NODE_INPUTS("in")
    DEKI_NODE_OUTPUTS("next")
public:
    DEKI_EXPORT bool enabled = true;

    DEKI_EXPORT
    DEKI_RANGE(-50.0f, 50.0f)
    DEKI_UNIT(Velocity)
    float speedMin = 2.0f;            // m/s

    DEKI_EXPORT
    DEKI_RANGE(-50.0f, 50.0f)
    DEKI_UNIT(Velocity)
    float speedMax = 2.0f;            // m/s

    DEKI_EXPORT
    DEKI_RANGE(-DekiMath::kTwoPi, 2.0f * DekiMath::kTwoPi)
    DEKI_UNIT(Angle)
    float angleMin = 0.0f;            // radians, 0 = +X right, pi/2 = +Y up

    DEKI_EXPORT
    DEKI_RANGE(-DekiMath::kTwoPi, 2.0f * DekiMath::kTwoPi)
    DEKI_UNIT(Angle)
    float angleMax = DekiMath::kTwoPi;
};

// Sets a new particle's angle and spin rate, and integrates that spin every
// frame so spin works without a Rotation over Lifetime node in the chain.
struct ParticleInitialRotationNode
{
    DEKI_NODE(ParticleInitialRotationNode, "ParticleInitialRotation", "Particles/Initial")
    static constexpr const char* StaticNodeDisplayName = "Initial Rotation";
    static constexpr const char* StaticNodeDescription = "Gives each new particle a starting angle and spin rate.";
    DEKI_NODE_INPUTS("in")
    DEKI_NODE_OUTPUTS("next")
public:
    DEKI_EXPORT bool enabled = true;

    DEKI_EXPORT
    DEKI_RANGE(-DekiMath::kTwoPi, DekiMath::kTwoPi)
    DEKI_UNIT(Angle)
    float rotationMin = 0.0f;          // radians

    DEKI_EXPORT
    DEKI_RANGE(-DekiMath::kTwoPi, DekiMath::kTwoPi)
    DEKI_UNIT(Angle)
    float rotationMax = 0.0f;

    DEKI_EXPORT
    DEKI_RANGE(-2.0f * DekiMath::kTwoPi, 2.0f * DekiMath::kTwoPi)
    DEKI_UNIT(Angle)
    float spinSpeedMin = 0.0f;         // radians per second

    DEKI_EXPORT
    DEKI_RANGE(-2.0f * DekiMath::kTwoPi, 2.0f * DekiMath::kTwoPi)
    DEKI_UNIT(Angle)
    float spinSpeedMax = 0.0f;
};

// ---------------------------------------------------------------------------
// Forces (velocity is integrated into position by the emitter, after the whole
// chain has run, so a graph with no forces still moves its particles)
// ---------------------------------------------------------------------------

struct ParticleGravityNode
{
    DEKI_NODE(ParticleGravityNode, "ParticleGravity", "Particles/Forces")
    static constexpr const char* StaticNodeDisplayName = "Gravity";
    static constexpr const char* StaticNodeDescription = "Pulls particles with a constant acceleration.";
    DEKI_NODE_INPUTS("in")
    DEKI_NODE_OUTPUTS("next")
public:
    DEKI_EXPORT bool enabled = true;

    DEKI_EXPORT
    DEKI_RANGE(-100.0f, 100.0f)
    DEKI_UNIT(Acceleration)
    float gravityX = 0.0f;             // m/s^2

    DEKI_EXPORT
    DEKI_RANGE(-100.0f, 100.0f)
    DEKI_UNIT(Acceleration)
    float gravityY = -9.8f;            // m/s^2 (world Y+ is up; gravity pulls down)
};

// Wire it after Gravity, or the force is undone the moment it is applied.
struct ParticleDragNode
{
    DEKI_NODE(ParticleDragNode, "ParticleDrag", "Particles/Forces")
    static constexpr const char* StaticNodeDisplayName = "Drag";
    static constexpr const char* StaticNodeDescription = "Slows particles down over time.";
    DEKI_NODE_INPUTS("in")
    DEKI_NODE_OUTPUTS("next")
public:
    DEKI_EXPORT bool enabled = true;

    DEKI_EXPORT
    DEKI_RANGE(0.0f, 20.0f)
    float drag = 1.0f;   // 1/sec — at 1.0 a particle loses ~63% of its speed per second
};

// ---------------------------------------------------------------------------
// Over lifetime (driven by age / lifetime, so they need no state of their own)
// ---------------------------------------------------------------------------

struct ParticleSizeOverLifetimeNode
{
    DEKI_NODE(ParticleSizeOverLifetimeNode, "ParticleSizeOverLifetime", "Particles/Over Lifetime")
    static constexpr const char* StaticNodeDisplayName = "Size over Lifetime";
    static constexpr const char* StaticNodeDescription = "Grows or shrinks a particle as it ages.";
    DEKI_NODE_INPUTS("in")
    DEKI_NODE_OUTPUTS("next")
public:
    DEKI_EXPORT bool enabled = true;

    DEKI_EXPORT
    DEKI_RANGE(0.0f, 10.0f)
    float sizeAt0 = 1.0f;

    DEKI_EXPORT
    DEKI_RANGE(0.0f, 10.0f)
    float sizeAt1 = 1.0f;
};

struct ParticleColorOverLifetimeNode
{
    DEKI_NODE(ParticleColorOverLifetimeNode, "ParticleColorOverLifetime", "Particles/Over Lifetime")
    static constexpr const char* StaticNodeDisplayName = "Color over Lifetime";
    static constexpr const char* StaticNodeDescription = "Fades a particle's color and alpha as it ages.";
    DEKI_NODE_INPUTS("in")
    DEKI_NODE_OUTPUTS("next")
public:
    DEKI_EXPORT bool enabled = true;

    DEKI_EXPORT deki::Color colorAt0 = deki::Color::White;
    DEKI_EXPORT deki::Color colorAt1 = deki::Color::Transparent;
};

// Lerps the spin RATE across the particle's life and integrates it. Pair it
// with Initial Rotation for a starting offset, or use it alone for spin that
// always begins at zero.
struct ParticleRotationOverLifetimeNode
{
    DEKI_NODE(ParticleRotationOverLifetimeNode, "ParticleRotationOverLifetime", "Particles/Over Lifetime")
    static constexpr const char* StaticNodeDisplayName = "Rotation over Lifetime";
    static constexpr const char* StaticNodeDescription = "Ramps a particle's spin rate as it ages.";
    DEKI_NODE_INPUTS("in")
    DEKI_NODE_OUTPUTS("next")
public:
    DEKI_EXPORT bool enabled = true;

    DEKI_EXPORT
    DEKI_RANGE(-2.0f * DekiMath::kTwoPi, 2.0f * DekiMath::kTwoPi)
    DEKI_UNIT(Angle)
    float spinSpeedAt0 = 0.0f;          // radians/sec at birth

    DEKI_EXPORT
    DEKI_RANGE(-2.0f * DekiMath::kTwoPi, 2.0f * DekiMath::kTwoPi)
    DEKI_UNIT(Angle)
    float spinSpeedAt1 = 0.0f;          // radians/sec at death
};

#include "generated/ParticleEmitNode.gen.h"
#include "generated/ParticleEmissionNode.gen.h"
#include "generated/ParticleInitialVelocityNode.gen.h"
#include "generated/ParticleInitialRotationNode.gen.h"
#include "generated/ParticleGravityNode.gen.h"
#include "generated/ParticleDragNode.gen.h"
#include "generated/ParticleSizeOverLifetimeNode.gen.h"
#include "generated/ParticleColorOverLifetimeNode.gen.h"
#include "generated/ParticleRotationOverLifetimeNode.gen.h"
