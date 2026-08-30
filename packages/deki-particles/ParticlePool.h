#pragma once

#include <cstdint>
#include <cstring>
#include "DekiMath.h"

namespace deki_particles {

/**
 * @brief Struct-of-arrays particle storage with lazy optional columns.
 *
 * Always-present columns (allocated when capacity is set):
 *   posX, posY, velX, velY, age, lifetime
 *
 * Optional shared columns (allocated on first use via Ensure*):
 *   rotation, rotationSpeed, scale, tintR, tintG, tintB, tintA
 *
 * Modifiers needing private per-particle data should allocate it in their
 * onAttach, out of the per-emitter state blob — keeping pool extension
 * reserved for shared state read by the render path.
 *
 * Death uses swap-with-last-alive: O(1), keeps [0, aliveCount) dense.
 */
class ParticlePool
{
public:
    ParticlePool() = default;
    ~ParticlePool() { Free(); }

    ParticlePool(const ParticlePool&) = delete;
    ParticlePool& operator=(const ParticlePool&) = delete;

    void SetCapacity(int newCapacity)
    {
        if (newCapacity == m_Capacity) return;
        Free();
        m_Capacity = newCapacity;
        m_AliveCount = 0;
        if (newCapacity <= 0) return;

        // Time columns (age/lifetime) are seconds; spatial columns are meters.
        posX = new float[newCapacity]{};
        posY = new float[newCapacity]{};
        velX = new float[newCapacity]{};
        velY = new float[newCapacity]{};
        age = new float[newCapacity]{};
        lifetime = new float[newCapacity]{};
    }

    int  Capacity() const { return m_Capacity; }
    int  AliveCount() const { return m_AliveCount; }

    // Spawn a new particle slot. Returns -1 if pool is full.
    int  Spawn()
    {
        if (m_AliveCount >= m_Capacity) return -1;
        int idx = m_AliveCount++;
        return idx;
    }

    // Kill particle at index by swapping with last alive. Caller is
    // responsible for swapping any optional columns it cares about.
    // Returns the index that the formerly-last particle was moved to
    // (== idx), or -1 if idx was already past the alive range.
    int  KillSwap(int idx)
    {
        if (idx < 0 || idx >= m_AliveCount) return -1;
        int last = m_AliveCount - 1;
        if (idx != last)
        {
            posX[idx] = posX[last];
            posY[idx] = posY[last];
            velX[idx] = velX[last];
            velY[idx] = velY[last];
            age[idx] = age[last];
            lifetime[idx] = lifetime[last];
            if (m_HasRotation)      { rotation[idx] = rotation[last]; rotationSpeed[idx] = rotationSpeed[last]; }
            if (m_HasScale)         { scale[idx] = scale[last]; }
            if (m_HasTint)          { tintR[idx] = tintR[last]; tintG[idx] = tintG[last]; tintB[idx] = tintB[last]; tintA[idx] = tintA[last]; }
        }
        m_AliveCount--;
        return idx;
    }

    // Optional columns — allocated on first request. Idempotent.
    void EnsureRotation()
    {
        if (m_HasRotation || m_Capacity <= 0) return;
        rotation = new float[m_Capacity]{};
        rotationSpeed = new float[m_Capacity]{};
        m_HasRotation = true;
    }
    void EnsureScale()
    {
        if (m_HasScale || m_Capacity <= 0) return;
        scale = new float[m_Capacity];
        for (int i = 0; i < m_Capacity; ++i) scale[i] = 1.0f;
        m_HasScale = true;
    }
    void EnsureTint()
    {
        if (m_HasTint || m_Capacity <= 0) return;
        tintR = new uint8_t[m_Capacity];
        tintG = new uint8_t[m_Capacity];
        tintB = new uint8_t[m_Capacity];
        tintA = new uint8_t[m_Capacity];
        std::memset(tintR, 255, m_Capacity);
        std::memset(tintG, 255, m_Capacity);
        std::memset(tintB, 255, m_Capacity);
        std::memset(tintA, 255, m_Capacity);
        m_HasTint = true;
    }

    bool HasRotation() const { return m_HasRotation; }
    bool HasScale() const    { return m_HasScale; }
    bool HasTint() const     { return m_HasTint; }

    // Always-present columns (public for tight inner loops). Spatial columns
    // in meters, time columns (age/lifetime) in seconds.
    float* posX = nullptr;
    float* posY = nullptr;
    float* velX = nullptr;
    float* velY = nullptr;
    float* age = nullptr;
    float* lifetime = nullptr;

    // Optional columns (nullptr until corresponding Ensure* called).
    // rotation is in radians (engine convention).
    float* rotation = nullptr;        // radians
    float* rotationSpeed = nullptr;   // radians/sec
    float* scale = nullptr;
    uint8_t*    tintR = nullptr;
    uint8_t*    tintG = nullptr;
    uint8_t*    tintB = nullptr;
    uint8_t*    tintA = nullptr;

private:
    int  m_Capacity = 0;
    int  m_AliveCount = 0;
    bool m_HasRotation = false;
    bool m_HasScale = false;
    bool m_HasTint = false;

    void Free()
    {
        delete[] posX;          posX = nullptr;
        delete[] posY;          posY = nullptr;
        delete[] velX;          velX = nullptr;
        delete[] velY;          velY = nullptr;
        delete[] age;           age = nullptr;
        delete[] lifetime;      lifetime = nullptr;
        delete[] rotation;      rotation = nullptr;
        delete[] rotationSpeed; rotationSpeed = nullptr;
        delete[] scale;         scale = nullptr;
        delete[] tintR;         tintR = nullptr;
        delete[] tintG;         tintG = nullptr;
        delete[] tintB;         tintB = nullptr;
        delete[] tintA;         tintA = nullptr;
        m_HasRotation = m_HasScale = m_HasTint = false;
        m_AliveCount = 0;
    }
};

} // namespace deki_particles
