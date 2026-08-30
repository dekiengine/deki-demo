#pragma once

#include <cstdint>
#include <functional>
#include "DekiBehaviour.h"
#include "reflection/DekiProperty.h"
#include "Easing.h"
#include "DekiVector.h"
#include "Color.h"

/**
 * @brief Tween target type - which property to animate
 *
 * Uses DekiVector3 end_value:
 * - Position: X,Y from end_value
 * - Scale: X,Y from end_value
 * - Rotation: Z from end_value (radians; engine convention)
 */
enum class TweenTargetType : uint8_t
{
    Position = 0,  // Animate X,Y position
    Scale,         // Animate X,Y scale
    Rotation,      // Animate rotation (uses Z component)
    COUNT
};

/**
 * @brief Editor-configurable tween component
 *
 * Allows designers to set up tweens in the inspector without code.
 * Extends DekiBehaviour for Update() lifecycle.
 *
 * Tweens from current state to end_value:
 * - Position: X,Y from end_value
 * - Scale: X,Y from end_value
 * - Rotation: Z from end_value (radians; engine convention)
 *
 * Features:
 * - Target property selection (position, scale, rotation)
 * - Duration, delay, looping, ping-pong
 * - Easing selection dropdown
 * - Auto-play on start option
 * - Completion events (for chaining or triggering other behaviours)
 */
class DEKI_TWEEN_API TweenComponent : public DekiBehaviour
{
public:
    DEKI_COMPONENT(TweenComponent, DekiBehaviour, "Animation", "9e113121-59df-4786-9752-0099935f1378", "DEKI_FEATURE_TWEEN")
    DEKI_DESCRIPTION("Animates the object's position, scale or rotation along an easing curve.")

    // ========== Inspector Properties ==========

    /** @brief Target type to animate */
    DEKI_EXPORT
    TweenTargetType target_type = TweenTargetType::Position;

    /** @brief End value - Position/Scale use X,Y; Rotation uses Z */
    DEKI_EXPORT
    DekiVector3 end_value = DekiVector3(0.0f, 0.0f, 0.0f);

    /** @brief Duration in seconds. */
    DEKI_EXPORT
    DEKI_SLIDER(0.1f, 10.0f)
    float duration = 1.0f;

    /** @brief Delay before starting (seconds). */
    DEKI_EXPORT
    DEKI_SLIDER(0.0f, 5.0f)
    float delay = 0.0f;

    /** @brief Easing type */
    DEKI_EXPORT
    deki::EaseType ease_type = deki::EaseType::Linear;

    /** @brief Number of loops (-1 = infinite, 0 = no loop) */
    DEKI_EXPORT
    DEKI_RANGE(-1, 100)
    int32_t loops = 0;

    /** @brief Reverse direction each loop */
    DEKI_EXPORT
    bool ping_pong = false;

    /** @brief Auto-play when Start() is called */
    DEKI_EXPORT
    bool auto_play = true;

    /** @brief Use relative values (add to current instead of absolute) */
    DEKI_EXPORT
    bool relative = false;

    // ========== Runtime Callbacks ==========

    std::function<void()> on_complete;
    std::function<void(float progress)> on_update;

    // ========== Lifecycle ==========

    TweenComponent();
    virtual ~TweenComponent();

    void Awake() override;
    void Start() override;
    void Update() override;

    // ========== Control API ==========

    /**
     * @brief Start or restart the tween
     */
    void Play();

    /**
     * @brief Pause the tween
     */
    void Pause();

    /**
     * @brief Resume a paused tween
     */
    void Resume();

    /**
     * @brief Stop and reset the tween
     */
    void Stop();

    /**
     * @brief Set completion callback
     */
    void SetOnComplete(std::function<void()> callback);

    /**
     * @brief Check if tween is currently playing
     */
    bool IsPlaying() const { return m_IsPlaying; }

    /**
     * @brief Check if tween has completed
     */
    bool HasCompleted() const { return m_HasCompleted; }

    /**
     * @brief Get current progress (0-1).
     */
    float GetProgress() const;

private:
    // Runtime state.
    float m_Elapsed = 0.0f;
    float m_DelayElapsed = 0.0f;
    int32_t m_CurrentLoop = 0;
    bool m_Reversed = false;
    bool m_HasCompleted = false;
    bool m_IsPlaying = false;
    bool m_IsPaused = false;

    // Cached initial value (captured when tween starts)
    DekiVector3 m_StartValue = DekiVector3(0.0f, 0.0f, 0.0f);

    /**
     * @brief Get the current value of the target property as DekiVector3
     * Position: X,Y from object position
     * Scale: X,Y from object scale
     * Rotation: Z from object rotation
     */
    DekiVector3 GetCurrentValue() const;

    /**
     * @brief Apply interpolated DekiVector3 value to target
     */
    void ApplyValue(float easedT);

    /**
     * @brief Get the eased progress value.
     */
    float GetEasedProgress(float t) const;

    /**
     * @brief Handle loop completion or final completion
     */
    void HandleLoopOrComplete();
};

// Generated property metadata (after class definition for offsetof)
#include "generated/TweenComponent.gen.h"
