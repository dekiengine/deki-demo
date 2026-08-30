#include "ParticleSystem.h"
#include <algorithm>

ParticleSystem& ParticleSystem::GetInstance()
{
    static ParticleSystem s_instance;
    return s_instance;
}

void ParticleSystem::RegisterEmitter(ParticleEmitterComponent* emitter)
{
    if (!emitter) return;
    auto it = std::find(m_Emitters.begin(), m_Emitters.end(), emitter);
    if (it != m_Emitters.end()) return;
    m_Emitters.push_back(emitter);
}

void ParticleSystem::UnregisterEmitter(ParticleEmitterComponent* emitter)
{
    if (!emitter) return;
    auto it = std::find(m_Emitters.begin(), m_Emitters.end(), emitter);
    if (it != m_Emitters.end()) m_Emitters.erase(it);
}

void ParticleSystem::ClearAll()
{
    m_Emitters.clear();
}
