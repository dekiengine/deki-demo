#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include <functional>
#include "Tween.h"

namespace deki {

/**
 * @brief Singleton manager for all active tweens
 *
 * TweenManager holds and updates all tweens created via the static API.
 * It is updated automatically when any TweenComponent's Update() is called,
 * using frame tracking to ensure it only processes once per frame.
 *
 * For the static API to work, at least one TweenComponent must exist in
 * the scene to drive updates. Alternatively, you can manually call
 * TweenManager::Instance().Update() each frame.
 */
class DEKI_TWEEN_API TweenManager
{
public:
    /**
     * @brief Get singleton instance
     */
    static TweenManager& Instance();

    /**
     * @brief Update all active tweens
     * @param deltaTimeSeconds Delta time in seconds
     *
     * Called by TweenComponent::Update() or manually if no components exist.
     */
    void Update(float deltaTimeSeconds);

    /**
     * @brief Ensure manager is updated for the current frame
     *
     * Uses DekiTime to track frames and only updates once per frame.
     * Called automatically by TweenComponent::Update().
     */
    void EnsureUpdatedThisFrame();

    /**
     * @brief Kill all active tweens
     */
    void KillAll();

    /**
     * @brief Kill tweens targeting a specific pointer
     */
    template <typename T>
    void KillTweensOf(T* target);

    /**
     * @brief Get number of active tweens
     */
    size_t GetActiveTweenCount() const { return m_Tweens.size(); }

    // ========== Static Factory API ==========

    /**
     * @brief Create a float tween from current value to end value.
     * `duration` is float seconds.
     */
    static Tween<float>& To(float* target, float endValue, float duration);

    /**
     * @brief Create a float tween from start value to end value
     */
    static Tween<float>& FromTo(float* target, float startValue, float endValue, float duration);

    /**
     * @brief Create an int32 tween from current value to end value
     */
    static Tween<int32_t>& To(int32_t* target, int32_t endValue, float duration);

    /**
     * @brief Create an int32 tween from start value to end value
     */
    static Tween<int32_t>& FromTo(int32_t* target, int32_t startValue, int32_t endValue, float duration);

    /**
     * @brief Create a DekiVector2 tween from current value to end value
     */
    static Tween<DekiVector2>& To(DekiVector2* target, const DekiVector2& endValue, float duration);

    /**
     * @brief Create a DekiVector2 tween from start value to end value
     */
    static Tween<DekiVector2>& FromTo(DekiVector2* target, const DekiVector2& startValue, const DekiVector2& endValue, float duration);

    /**
     * @brief Create a Color tween from current value to end value
     */
    static Tween<Color>& To(Color* target, const Color& endValue, float duration);

    /**
     * @brief Create a Color tween from start value to end value
     */
    static Tween<Color>& FromTo(Color* target, const Color& startValue, const Color& endValue, float duration);

    /**
     * @brief Create a delayed callback (no value interpolation)
     */
    static Tween<float>& DelayedCall(float delay, std::function<void()> callback);

private:
    TweenManager();
    ~TweenManager();
    TweenManager(const TweenManager&) = delete;
    TweenManager& operator=(const TweenManager&) = delete;

    /**
     * @brief Add a tween to the manager
     * @return Reference to the added tween for chaining
     */
    template <typename T>
    Tween<T>& AddTween(std::unique_ptr<Tween<T>> tween);

    std::vector<std::unique_ptr<ITween>> m_Tweens;
    uint32_t m_NextId = 1;

    // Temporary storage during Update() to avoid iterator invalidation
    std::vector<std::unique_ptr<ITween>> m_TweensToAdd;
    bool m_IsUpdating = false;

    // Frame tracking for EnsureUpdatedThisFrame()
    uint32_t m_LastUpdateTime = 0;
};

// ========== Template Implementation ==========

template <typename T>
void TweenManager::KillTweensOf(T* target)
{
    for (auto& tween : m_Tweens)
    {
        auto* typedTween = dynamic_cast<Tween<T>*>(tween.get());
        if (typedTween && typedTween->GetTarget() == target)
        {
            typedTween->Kill();
        }
    }
}

template <typename T>
Tween<T>& TweenManager::AddTween(std::unique_ptr<Tween<T>> tween)
{
    tween->id = m_NextId++;
    Tween<T>& tweenRef = *tween;

    if (m_IsUpdating)
    {
        m_TweensToAdd.push_back(std::move(tween));
    }
    else
    {
        m_Tweens.push_back(std::move(tween));
    }

    return tweenRef;
}

} // namespace deki
