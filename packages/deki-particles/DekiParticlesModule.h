#pragma once

/**
 * @file DekiParticlesPackage.h
 * @brief Central header for the Deki Particles Package
 *
 * Particle system driven by a graph asset. The emitter holds a fixed-size
 * particle pool and is rendered as a single QuadBlit; behavior (emission,
 * shape, gravity, color/size/rotation over lifetime) is authored as a chain
 * of modifier NODES in a ParticleGraph, walked once into a flat callback list
 * when the emitter starts. External packages ship new modifier types by
 * declaring a DEKI_NODE struct in a "Particles/" category and registering its
 * ParticleModifierOps — no change to this package.
 */

// DLL export macro lives in DekiParticlesAPI.h to avoid circular includes
// when component headers need DEKI_PARTICLES_API but cannot drag in this
// umbrella header (which would re-enter the component headers themselves).
#include "DekiParticlesAPI.h"

#ifdef DEKI_PACKAGE_PARTICLES

#include "ParticleEmitterComponent.h"
#include "ParticleGraph.h"
#include "ParticleNodes.h"
#include "ParticleModifierRegistry.h"
#include "ParticleSystem.h"

#endif // DEKI_PACKAGE_PARTICLES
