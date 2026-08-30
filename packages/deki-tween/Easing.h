#pragma once

#include <cstdint>
#include "DekiMath.h"

// DLL export macro
#ifdef DEKI_EDITOR
    #ifdef _WIN32
        #ifdef DEKI_TWEEN_EXPORTS
            #define DEKI_TWEEN_API __declspec(dllexport)
        #else
            #define DEKI_TWEEN_API __declspec(dllimport)
        #endif
    #else
        #define DEKI_TWEEN_API
    #endif
#else
    #define DEKI_TWEEN_API
#endif

namespace deki {

/**
 * @brief Easing function type: takes normalized progress t in [0,1],
 * returns eased value.
 */
using EasingFunc = float (*)(float t);

/**
 * @brief Easing types enum for serialization and editor selection
 */
enum class EaseType : uint8_t
{
    Linear = 0,

    // Sine
    SineIn,
    SineOut,
    SineInOut,

    // Quad
    QuadIn,
    QuadOut,
    QuadInOut,

    // Cubic
    CubicIn,
    CubicOut,
    CubicInOut,

    // Quart
    QuartIn,
    QuartOut,
    QuartInOut,

    // Quint
    QuintIn,
    QuintOut,
    QuintInOut,

    // Expo
    ExpoIn,
    ExpoOut,
    ExpoInOut,

    // Circ
    CircIn,
    CircOut,
    CircInOut,

    // Back (overshoots)
    BackIn,
    BackOut,
    BackInOut,

    // Elastic
    ElasticIn,
    ElasticOut,
    ElasticInOut,

    // Bounce
    BounceIn,
    BounceOut,
    BounceInOut,

    COUNT
};

/**
 * @brief Easing function implementations
 */
namespace Ease {

DEKI_TWEEN_API float Linear(float t);

DEKI_TWEEN_API float SineIn(float t);
DEKI_TWEEN_API float SineOut(float t);
DEKI_TWEEN_API float SineInOut(float t);

DEKI_TWEEN_API float QuadIn(float t);
DEKI_TWEEN_API float QuadOut(float t);
DEKI_TWEEN_API float QuadInOut(float t);

DEKI_TWEEN_API float CubicIn(float t);
DEKI_TWEEN_API float CubicOut(float t);
DEKI_TWEEN_API float CubicInOut(float t);

DEKI_TWEEN_API float QuartIn(float t);
DEKI_TWEEN_API float QuartOut(float t);
DEKI_TWEEN_API float QuartInOut(float t);

DEKI_TWEEN_API float QuintIn(float t);
DEKI_TWEEN_API float QuintOut(float t);
DEKI_TWEEN_API float QuintInOut(float t);

DEKI_TWEEN_API float ExpoIn(float t);
DEKI_TWEEN_API float ExpoOut(float t);
DEKI_TWEEN_API float ExpoInOut(float t);

DEKI_TWEEN_API float CircIn(float t);
DEKI_TWEEN_API float CircOut(float t);
DEKI_TWEEN_API float CircInOut(float t);

DEKI_TWEEN_API float BackIn(float t);
DEKI_TWEEN_API float BackOut(float t);
DEKI_TWEEN_API float BackInOut(float t);

DEKI_TWEEN_API float ElasticIn(float t);
DEKI_TWEEN_API float ElasticOut(float t);
DEKI_TWEEN_API float ElasticInOut(float t);

DEKI_TWEEN_API float BounceIn(float t);
DEKI_TWEEN_API float BounceOut(float t);
DEKI_TWEEN_API float BounceInOut(float t);

/**
 * @brief Get easing function by type enum
 */
DEKI_TWEEN_API EasingFunc GetFunction(EaseType type);

} // namespace Ease

} // namespace deki
