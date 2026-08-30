#include "Easing.h"
#include "DekiMath.h"
#include <cmath>

namespace deki {
namespace Ease {

// Easing math runs in plain float. The engine convention is radians for all
// angular arguments, so trig functions get their arguments in radians directly
// (e.g. "t * pi" stays "t * DekiMath::kPi", "t * pi/2" becomes "t * kHalfPi").

float Linear(float t)
{
    return t;
}

// Sine
float SineIn(float t)
{
    // 1 - cos((pi/2) * t)
    return 1.0f - std::cos(t * DekiMath::kHalfPi);
}

float SineOut(float t)
{
    return std::sin(t * DekiMath::kHalfPi);
}

float SineInOut(float t)
{
    // (1 - cos(pi*t)) / 2
    return (1.0f - std::cos(t * DekiMath::kPi)) * 0.5f;
}

// Quad
float QuadIn(float t)
{
    return t * t;
}

float QuadOut(float t)
{
    float inv = 1.0f - t;
    return 1.0f - inv * inv;
}

float QuadInOut(float t)
{
    if (t < 0.5f)
    {
        return 2.0f * t * t;
    }
    float base = 2.0f - 2.0f * t;
    return 1.0f - (base * base) * 0.5f;
}

// Cubic
float CubicIn(float t)
{
    return t * t * t;
}

float CubicOut(float t)
{
    float inv = 1.0f - t;
    return 1.0f - inv * inv * inv;
}

float CubicInOut(float t)
{
    if (t < 0.5f)
    {
        return 4.0f * t * t * t;
    }
    float base = 2.0f - 2.0f * t;
    return 1.0f - (base * base * base) * 0.5f;
}

// Quart
float QuartIn(float t)
{
    float t2 = t * t;
    return t2 * t2;
}

float QuartOut(float t)
{
    float inv = 1.0f - t;
    float inv2 = inv * inv;
    return 1.0f - inv2 * inv2;
}

float QuartInOut(float t)
{
    if (t < 0.5f)
    {
        float t2 = t * t;
        return 8.0f * t2 * t2;
    }
    float base = 2.0f - 2.0f * t;
    float base2 = base * base;
    return 1.0f - (base2 * base2) * 0.5f;
}

// Quint
float QuintIn(float t)
{
    float t2 = t * t;
    return t2 * t2 * t;
}

float QuintOut(float t)
{
    float inv = 1.0f - t;
    float inv2 = inv * inv;
    return 1.0f - inv2 * inv2 * inv;
}

float QuintInOut(float t)
{
    if (t < 0.5f)
    {
        float t2 = t * t;
        return 16.0f * t2 * t2 * t;
    }
    float base = 2.0f - 2.0f * t;
    float base2 = base * base;
    return 1.0f - (base2 * base2 * base) * 0.5f;
}

// Expo
float ExpoIn(float t)
{
    if (t == 0.0f) return 0.0f;
    return std::pow(2.0f, 10.0f * t - 10.0f);
}

float ExpoOut(float t)
{
    if (t == 1.0f) return 1.0f;
    return 1.0f - std::pow(2.0f, -10.0f * t);
}

float ExpoInOut(float t)
{
    if (t == 0.0f) return 0.0f;
    if (t == 1.0f) return 1.0f;
    if (t < 0.5f)
    {
        return std::pow(2.0f, 20.0f * t - 10.0f) * 0.5f;
    }
    return (2.0f - std::pow(2.0f, 10.0f - 20.0f * t)) * 0.5f;
}

// Circ
float CircIn(float t)
{
    return 1.0f - std::sqrt(1.0f - t * t);
}

float CircOut(float t)
{
    float tm1 = t - 1.0f;
    return std::sqrt(1.0f - tm1 * tm1);
}

float CircInOut(float t)
{
    if (t < 0.5f)
    {
        float twoT = 2.0f * t;
        return (1.0f - std::sqrt(1.0f - twoT * twoT)) * 0.5f;
    }
    float base = 2.0f - 2.0f * t;
    return (std::sqrt(1.0f - base * base) + 1.0f) * 0.5f;
}

// Back constants (Penner: c1 = 1.70158, c2 = c1 * 1.525, c3 = c1 + 1).
static constexpr float kBack_c1 = 1.70158f;
static constexpr float kBack_c2 = 1.70158f * 1.525f;
static constexpr float kBack_c3 = 1.70158f + 1.0f;

float BackIn(float t)
{
    // c3*t^3 - c1*t^2
    float t2 = t * t;
    return kBack_c3 * t2 * t - kBack_c1 * t2;
}

float BackOut(float t)
{
    // 1 + c3*(t-1)^3 + c1*(t-1)^2
    float tm1 = t - 1.0f;
    float tm1_2 = tm1 * tm1;
    return 1.0f + kBack_c3 * tm1_2 * tm1 + kBack_c1 * tm1_2;
}

float BackInOut(float t)
{
    if (t < 0.5f)
    {
        float twoT = 2.0f * t;
        return (twoT * twoT * ((kBack_c2 + 1.0f) * twoT - kBack_c2)) * 0.5f;
    }
    float base = 2.0f * t - 2.0f;
    return (base * base * ((kBack_c2 + 1.0f) * (t * 2.0f - 2.0f) + kBack_c2) + 2.0f) * 0.5f;
}

// Elastic. Penner uses sin arguments in radians: (2pi)/3 and (2pi)/4.5.
static constexpr float kElastic_c4Rad = 2.0f * DekiMath::kPi / 3.0f;
static constexpr float kElastic_c5Rad = 2.0f * DekiMath::kPi / 4.5f;

float ElasticIn(float t)
{
    if (t == 0.0f) return 0.0f;
    if (t == 1.0f) return 1.0f;
    float amp = std::pow(2.0f, 10.0f * t - 10.0f);
    float phaseRad = (t * 10.0f - 10.75f) * kElastic_c4Rad;
    return -amp * std::sin(phaseRad);
}

float ElasticOut(float t)
{
    if (t == 0.0f) return 0.0f;
    if (t == 1.0f) return 1.0f;
    float amp = std::pow(2.0f, -10.0f * t);
    float phaseRad = (t * 10.0f - 0.75f) * kElastic_c4Rad;
    return amp * std::sin(phaseRad) + 1.0f;
}

float ElasticInOut(float t)
{
    if (t == 0.0f) return 0.0f;
    if (t == 1.0f) return 1.0f;
    float phaseRad = (20.0f * t - 11.125f) * kElastic_c5Rad;
    float s = std::sin(phaseRad);
    if (t < 0.5f)
    {
        float amp = std::pow(2.0f, 20.0f * t - 10.0f);
        return -(amp * s) * 0.5f;
    }
    float amp = std::pow(2.0f, 10.0f - 20.0f * t);
    return (amp * s) * 0.5f + 1.0f;
}

// Bounce. n1 = 7.5625, d1 = 2.75. Splits 0..1 into four bounces.
static constexpr float kBounce_n1 = 7.5625f;
static constexpr float kBounce_t1 = 1.0f / 2.75f;
static constexpr float kBounce_t2 = 2.0f / 2.75f;
static constexpr float kBounce_t3 = 2.5f / 2.75f;
static constexpr float kBounce_o1 = 1.5f / 2.75f;
static constexpr float kBounce_o2 = 2.25f / 2.75f;
static constexpr float kBounce_o3 = 2.625f / 2.75f;
static constexpr float kBounce_b2 = 0.75f;
static constexpr float kBounce_b3 = 0.9375f;
static constexpr float kBounce_b4 = 0.984375f;

static float BounceOutImpl(float t)
{
    if (t < kBounce_t1)
    {
        return kBounce_n1 * t * t;
    }
    else if (t < kBounce_t2)
    {
        float tt = t - kBounce_o1;
        return kBounce_n1 * tt * tt + kBounce_b2;
    }
    else if (t < kBounce_t3)
    {
        float tt = t - kBounce_o2;
        return kBounce_n1 * tt * tt + kBounce_b3;
    }
    else
    {
        float tt = t - kBounce_o3;
        return kBounce_n1 * tt * tt + kBounce_b4;
    }
}

float BounceIn(float t)
{
    return 1.0f - BounceOutImpl(1.0f - t);
}

float BounceOut(float t)
{
    return BounceOutImpl(t);
}

float BounceInOut(float t)
{
    if (t < 0.5f)
    {
        return (1.0f - BounceOutImpl(1.0f - 2.0f * t)) * 0.5f;
    }
    return (1.0f + BounceOutImpl(2.0f * t - 1.0f)) * 0.5f;
}

EasingFunc GetFunction(EaseType type)
{
    switch (type)
    {
    case EaseType::Linear:       return Linear;
    case EaseType::SineIn:       return SineIn;
    case EaseType::SineOut:      return SineOut;
    case EaseType::SineInOut:    return SineInOut;
    case EaseType::QuadIn:       return QuadIn;
    case EaseType::QuadOut:      return QuadOut;
    case EaseType::QuadInOut:    return QuadInOut;
    case EaseType::CubicIn:      return CubicIn;
    case EaseType::CubicOut:     return CubicOut;
    case EaseType::CubicInOut:   return CubicInOut;
    case EaseType::QuartIn:      return QuartIn;
    case EaseType::QuartOut:     return QuartOut;
    case EaseType::QuartInOut:   return QuartInOut;
    case EaseType::QuintIn:      return QuintIn;
    case EaseType::QuintOut:     return QuintOut;
    case EaseType::QuintInOut:   return QuintInOut;
    case EaseType::ExpoIn:       return ExpoIn;
    case EaseType::ExpoOut:      return ExpoOut;
    case EaseType::ExpoInOut:    return ExpoInOut;
    case EaseType::CircIn:       return CircIn;
    case EaseType::CircOut:      return CircOut;
    case EaseType::CircInOut:    return CircInOut;
    case EaseType::BackIn:       return BackIn;
    case EaseType::BackOut:      return BackOut;
    case EaseType::BackInOut:    return BackInOut;
    case EaseType::ElasticIn:    return ElasticIn;
    case EaseType::ElasticOut:   return ElasticOut;
    case EaseType::ElasticInOut: return ElasticInOut;
    case EaseType::BounceIn:     return BounceIn;
    case EaseType::BounceOut:    return BounceOut;
    case EaseType::BounceInOut:  return BounceInOut;
    default:
        return Linear;
    }
}

} // namespace Ease
} // namespace deki
