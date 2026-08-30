#pragma once

/**
 * @file TweenPackage.h
 * @brief Central header for the Deki Tween Package
 *
 * The tween package provides value interpolation with easing functions.
 *
 * Usage:
 *
 * 1. Static API (requires at least one TweenComponent in scene):
 *    @code
 *    #include "TweenPackage.h"
 *
 *    // Tween a float value
 *    deki::TweenManager::To(&myValue, 100.0f, 0.5f)
 *        .SetEase(deki::EaseType::QuadOut)
 *        .OnComplete([]() { });
 *
 *    // Tween a DekiVector2
 *    deki::TweenManager::To(&position, DekiVector2(100, 200), 1.0f)
 *        .SetEase(deki::EaseType::SineInOut);
 *
 *    // Delayed callback
 *    deki::TweenManager::DelayedCall(2.0f, []() { });
 *    @endcode
 *
 * 2. TweenComponent (editor-configurable):
 *    Add TweenComponent to a DekiObject in the editor and configure
 *    target type, start/end values, duration, easing, etc.
 *
 * Available easing types:
 * - Linear
 * - Sine: SineIn, SineOut, SineInOut
 * - Quad: QuadIn, QuadOut, QuadInOut
 * - Cubic: CubicIn, CubicOut, CubicInOut
 * - Quart: QuartIn, QuartOut, QuartInOut
 * - Quint: QuintIn, QuintOut, QuintInOut
 * - Expo: ExpoIn, ExpoOut, ExpoInOut
 * - Circ: CircIn, CircOut, CircInOut
 * - Back: BackIn, BackOut, BackInOut
 * - Elastic: ElasticIn, ElasticOut, ElasticInOut
 * - Bounce: BounceIn, BounceOut, BounceInOut
 */

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

// Include all package headers when package is enabled
#ifdef DEKI_PACKAGE_TWEEN

#include "Easing.h"
#include "Tween.h"
#include "TweenManager.h"
#include "TweenComponent.h"

#endif // DEKI_PACKAGE_TWEEN
