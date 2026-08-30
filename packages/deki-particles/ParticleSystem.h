#pragma once

#include <vector>
#include "DekiParticlesAPI.h"

class ParticleEmitterComponent;

/**
 * @brief Lightweight registry of live particle emitters.
 *
 * Mirrors AnimationSystem in shape. Existence is justified by play-mode-stop
 * cleanup: when the editor stops Play, this clears all emitter pointers so
 * stale references don't leak into the next session.
 *
 * Per-frame simulation is driven from ParticleEmitterComponent::Update() —
 * this class does NOT iterate emitters per frame.
 */
class DEKI_PARTICLES_API ParticleSystem
{
public:
    static ParticleSystem& GetInstance();

    void RegisterEmitter(ParticleEmitterComponent* emitter);
    void UnregisterEmitter(ParticleEmitterComponent* emitter);
    void ClearAll();

    int  EmitterCount() const { return (int)m_Emitters.size(); }

private:
    ParticleSystem() = default;
    std::vector<ParticleEmitterComponent*> m_Emitters;
};
