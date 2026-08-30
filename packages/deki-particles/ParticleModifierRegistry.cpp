#include "ParticleModifierRegistry.h"

ParticleModifierRegistry& ParticleModifierRegistry::Instance()
{
    static ParticleModifierRegistry instance;
    return instance;
}
