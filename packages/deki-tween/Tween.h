#pragma once

#include <cstdint>
#include <functional>
#include "Easing.h"
#include "DekiVector.h"
#include "Color.h"

namespace deki {

/**
 * @brief Tween state enum
 */
enum class TweenState : uint8_t
{
    Idle,      // Not started or completed
    Running,   // Currently animating
    Paused,    // Paused mid-animation
    Completed  // Finished (ready for removal or restart)
};

/**
 * @brief Base tween interface for type-erased storage in TweenManager
 */
class DEKI_TWEEN_API ITween
{
public:
    virtual ~ITween() = default;

    virtual void Update(float deltaTimeSeconds) = 0;
    virtual bool IsComplete() const = 0;
    virtual void Kill() = 0;
    virtual void Pause() = 0;
    virtual void Resume() = 0;
    virtual TweenState GetState() const = 0;
    virtual void Restart() = 0;

    // Unique ID for lookup
    uint32_t id = 0;
};

/**
 * @brief Templated tween for specific value types
 *
 * Supports: float, int32_t, DekiVector2, deki::Color
 */
template <typename T>
class Tween : public ITween
{
public:
    using UpdateCallback = std::function<void(const T&)>;
    using CompleteCallback = std::function<void()>;

    Tween()
        : m_Target(nullptr)
        , m_StartValue{}
        , m_EndValue{}
        , m_CurrentValue{}
        , m_Duration(1.0f)
        , m_Elapsed(0.0f)
        , m_Delay(0.0f)
        , m_DelayElapsed(0.0f)
        , m_EaseFunc(Ease::Linear)
        , m_State(TweenState::Idle)
        , m_Loops(0)
        , m_CurrentLoop(0)
        , m_PingPong(false)
        , m_Reversed(false)
    {
    }

    ~Tween() override = default;

    // ========== Configuration (Fluent API) ==========

    /**
     * @brief Set the target pointer to modify directly
     */
    Tween<T>& SetTarget(T* target)
    {
        m_Target = target;
        return *this;
    }

    /**
     * @brief Set start value
     */
    Tween<T>& From(const T& startValue)
    {
        m_StartValue = startValue;
        return *this;
    }

    /**
     * @brief Set end value
     */
    Tween<T>& To(const T& endValue)
    {
        m_EndValue = endValue;
        return *this;
    }

    /**
     * @brief Set duration in seconds.
     */
    Tween<T>& Duration(float seconds)
    {
        m_Duration = seconds > 0.0f ? seconds : 0.001f;
        return *this;
    }

    /**
     * @brief Set easing function by type
     */
    Tween<T>& SetEase(EaseType ease)
    {
        m_EaseFunc = Ease::GetFunction(ease);
        return *this;
    }

    /**
     * @brief Set easing function directly
     */
    Tween<T>& SetEase(EasingFunc easeFunc)
    {
        m_EaseFunc = easeFunc ? easeFunc : Ease::Linear;
        return *this;
    }

    /**
     * @brief Set delay before starting (seconds).
     */
    Tween<T>& Delay(float seconds)
    {
        m_Delay = seconds > 0.0f ? seconds : 0.0f;
        return *this;
    }

    /**
     * @brief Set number of loops (-1 = infinite, 0 = no loop)
     */
    Tween<T>& SetLoops(int32_t loops)
    {
        m_Loops = loops;
        return *this;
    }

    /**
     * @brief Enable ping-pong (reverse on each loop)
     */
    Tween<T>& SetPingPong(bool pingPong)
    {
        m_PingPong = pingPong;
        return *this;
    }

    /**
     * @brief Set update callback (called each frame with current value)
     */
    Tween<T>& OnUpdate(UpdateCallback callback)
    {
        m_OnUpdate = callback;
        return *this;
    }

    /**
     * @brief Set completion callback
     */
    Tween<T>& OnComplete(CompleteCallback callback)
    {
        m_OnComplete = callback;
        return *this;
    }

    /**
     * @brief Start the tween (auto-called by TweenManager when added)
     */
    Tween<T>& Start()
    {
        if (m_State == TweenState::Idle || m_State == TweenState::Completed)
        {
            m_State = TweenState::Running;
            m_Elapsed = 0.0f;
            m_DelayElapsed = 0.0f;
            m_CurrentLoop = 0;
            m_Reversed = false;
            if (m_Target && m_Delay <= 0.0f)
            {
                m_CurrentValue = m_StartValue;
                *m_Target = m_CurrentValue;
            }
        }
        return *this;
    }

    // ========== Control ==========

    void Update(float deltaTimeSeconds) override
    {
        if (m_State != TweenState::Running)
            return;

        float dt = deltaTimeSeconds;

        // Handle delay
        if (m_DelayElapsed < m_Delay)
        {
            m_DelayElapsed += dt;
            if (m_DelayElapsed < m_Delay)
                return;
            // Apply remaining time after delay
            dt = m_DelayElapsed - m_Delay;
        }

        m_Elapsed += dt;

        float progress = m_Elapsed / m_Duration;
        if (progress >= 1.0f)
        {
            progress = 1.0f;
        }

        // Apply easing
        float easedInput = m_Reversed ? (1.0f - progress) : progress;
        float easedProgress = m_EaseFunc(easedInput);

        // Interpolate
        m_CurrentValue = Interpolate(easedProgress);

        // Apply value
        ApplyValue();

        // Check completion
        if (progress >= 1.0f)
        {
            HandleLoopOrComplete();
        }
    }

    bool IsComplete() const override
    {
        return m_State == TweenState::Completed;
    }

    void Kill() override
    {
        m_State = TweenState::Completed;
        // Note: Kill does NOT call OnComplete
    }

    void Pause() override
    {
        if (m_State == TweenState::Running)
        {
            m_State = TweenState::Paused;
        }
    }

    void Resume() override
    {
        if (m_State == TweenState::Paused)
        {
            m_State = TweenState::Running;
        }
    }

    TweenState GetState() const override
    {
        return m_State;
    }

    void Restart() override
    {
        m_State = TweenState::Idle;
        Start();
    }

    /**
     * @brief Get current interpolated value
     */
    const T& GetCurrentValue() const
    {
        return m_CurrentValue;
    }

    /**
     * @brief Get the target pointer
     */
    T* GetTarget() const
    {
        return m_Target;
    }

private:
    T* m_Target;       // Pointer to value being tweened (optional)
    T m_StartValue;    // Start value
    T m_EndValue;      // End value
    T m_CurrentValue;  // Current interpolated value

    float m_Duration;
    float m_Elapsed;
    float m_Delay;
    float m_DelayElapsed;

    EasingFunc m_EaseFunc;  // Easing function
    TweenState m_State;     // Current state

    int32_t m_Loops;       // Number of loops (-1 = infinite)
    int32_t m_CurrentLoop; // Current loop index
    bool m_PingPong;       // Reverse direction each loop
    bool m_Reversed;       // Currently playing in reverse

    UpdateCallback m_OnUpdate;
    CompleteCallback m_OnComplete;

    /**
     * @brief Interpolate between start and end values. t is normalized
     * progress in [0,1].
     */
    T Interpolate(float t) const;

    /**
     * @brief Apply current value to target and callbacks
     */
    void ApplyValue()
    {
        if (m_Target)
        {
            *m_Target = m_CurrentValue;
        }
        if (m_OnUpdate)
        {
            m_OnUpdate(m_CurrentValue);
        }
    }

    /**
     * @brief Handle loop completion or final completion
     */
    void HandleLoopOrComplete()
    {
        // Check if we should loop
        if (m_Loops == -1 || m_CurrentLoop < m_Loops)
        {
            m_CurrentLoop++;
            m_Elapsed = 0.0f;

            if (m_PingPong)
            {
                m_Reversed = !m_Reversed;
            }
        }
        else
        {
            // Complete
            m_State = TweenState::Completed;
            if (m_OnComplete)
            {
                m_OnComplete();
            }
        }
    }
};

// ========== Type-specific interpolation specializations ==========
// Implementations in Tween.cpp.

template <>
float Tween<float>::Interpolate(float t) const;

template <>
int32_t Tween<int32_t>::Interpolate(float t) const;

template <>
DekiVector2 Tween<DekiVector2>::Interpolate(float t) const;

template <>
deki::Color Tween<deki::Color>::Interpolate(float t) const;

} // namespace deki
