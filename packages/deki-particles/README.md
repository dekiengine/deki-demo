# Deki Particles

Particle system for the Deki Engine: emission shapes (point/circle/rect), gravity, drag, initial velocity, and lifetime/size/color/rotation modifiers.

An effect is a **Particle Effect** asset, authored in the editor's Node Graph window: an Emitter node followed by a chain of modifier nodes, wired in the order they run. `ParticleEmitterComponent` references that asset and walks it once when it starts, so the per-frame cost is exactly the modifiers you wired, with no graph interpretation in the loop. One graph drives any number of emitters.

Part of the [Deki Engine](https://github.com/Kirbyrawr/deki-engine) package ecosystem.

## Installation

Install via the Package Manager inside the Deki Editor.

## Adding your own modifier

From any package or the project DLL, with no change to deki-particles:

1. Declare a `DEKI_NODE` struct in a category starting `Particles/` (see `ParticleNodes.h`).
2. Register its `ParticleModifierOps` with `REGISTER_PARTICLE_MODIFIER` (see `ParticleModifierLibrary.cpp`). Anything the modifier remembers between frames goes in the ops' state blob, never in the struct: the struct belongs to the shared asset.
3. Register the node type from your package entry, the way `DekiParticles_RegisterGraphTypes` does.

## Dependencies

| Dependency | Type |
|---|---|
| `deki-nodegraph` | Deki package |
| `deki-rendering` | Deki package |
| `deki-2d` | Deki package |

## License

Licensed under the Apache License, Version 2.0. See [LICENSE](LICENSE) for details.
