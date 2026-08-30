#pragma once

#include <cstdint>

namespace deki_particles {

// xorshift32 PRNG. One state per emitter; cheap and good enough for visuals.
struct Xorshift32
{
    uint32_t state;

    void Seed(uint32_t s) { state = s ? s : 0x9E3779B9u; }

    uint32_t NextU32()
    {
        uint32_t x = state;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        state = x;
        return x;
    }

    float NextFloat01()
    {
        return (NextU32() >> 8) * (1.0f / 16777216.0f);
    }

    float NextFloatRange(float lo, float hi)
    {
        return lo + (hi - lo) * NextFloat01();
    }
};

} // namespace deki_particles
