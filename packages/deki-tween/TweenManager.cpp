#include "TweenManager.h"
#include "DekiTime.h"
#include "profiling/DekiProfiler.h"
#include <algorithm>

namespace deki {

TweenManager::TweenManager()
    : m_NextId(1)
    , m_IsUpdating(false)
    , m_LastUpdateTime(0)
{
}

TweenManager::~TweenManager()
{
    KillAll();
}

TweenManager& TweenManager::Instance()
{
    static TweenManager instance;
    return instance;
}

void TweenManager::Update(float deltaTimeSeconds)
{
    DEKI_PROFILE_SCOPE_N("TweenManager::Update");
    if (m_Tweens.empty())
        return;

    m_IsUpdating = true;

    // Update all tweens
    for (auto& tween : m_Tweens)
    {
        if (tween && !tween->IsComplete())
        {
            tween->Update(deltaTimeSeconds);
        }
    }

    m_IsUpdating = false;

    // Add any tweens that were created during update
    for (auto& tween : m_TweensToAdd)
    {
        m_Tweens.push_back(std::move(tween));
    }
    m_TweensToAdd.clear();

    // Remove completed tweens
    m_Tweens.erase(
        std::remove_if(m_Tweens.begin(), m_Tweens.end(),
                       [](const std::unique_ptr<ITween>& t) { return !t || t->IsComplete(); }),
        m_Tweens.end());
}

void TweenManager::EnsureUpdatedThisFrame()
{
    uint32_t currentTime = DekiTime::GetTime();

    // Only update if we haven't already this frame
    if (currentTime != m_LastUpdateTime)
    {
        m_LastUpdateTime = currentTime;
        float deltaSeconds = DekiTime::GetDeltaTimeF() / 1000.0f;
        Update(deltaSeconds);
    }
}

void TweenManager::KillAll()
{
    for (auto& tween : m_Tweens)
    {
        if (tween)
        {
            tween->Kill();
        }
    }
    m_Tweens.clear();
    m_TweensToAdd.clear();
}

// ========== Static Factory API ==========

Tween<float>& TweenManager::To(float* target, float endValue, float duration)
{
    return FromTo(target, target ? *target : 0.0f, endValue, duration);
}

Tween<float>& TweenManager::FromTo(float* target, float startValue, float endValue, float duration)
{
    auto tween = std::make_unique<Tween<float>>();
    tween->SetTarget(target).From(startValue).To(endValue).Duration(duration).Start();
    return Instance().AddTween(std::move(tween));
}

Tween<int32_t>& TweenManager::To(int32_t* target, int32_t endValue, float duration)
{
    return FromTo(target, target ? *target : 0, endValue, duration);
}

Tween<int32_t>& TweenManager::FromTo(int32_t* target, int32_t startValue, int32_t endValue, float duration)
{
    auto tween = std::make_unique<Tween<int32_t>>();
    tween->SetTarget(target).From(startValue).To(endValue).Duration(duration).Start();
    return Instance().AddTween(std::move(tween));
}

Tween<DekiVector2>& TweenManager::To(DekiVector2* target, const DekiVector2& endValue, float duration)
{
    return FromTo(target, target ? *target : DekiVector2::Zero(), endValue, duration);
}

Tween<DekiVector2>& TweenManager::FromTo(DekiVector2* target, const DekiVector2& startValue, const DekiVector2& endValue, float duration)
{
    auto tween = std::make_unique<Tween<DekiVector2>>();
    tween->SetTarget(target).From(startValue).To(endValue).Duration(duration).Start();
    return Instance().AddTween(std::move(tween));
}

Tween<Color>& TweenManager::To(Color* target, const Color& endValue, float duration)
{
    // Use inline Color constructor instead of Color::White to avoid dllimport issues
    return FromTo(target, target ? *target : Color(255, 255, 255), endValue, duration);
}

Tween<Color>& TweenManager::FromTo(Color* target, const Color& startValue, const Color& endValue, float duration)
{
    auto tween = std::make_unique<Tween<Color>>();
    tween->SetTarget(target).From(startValue).To(endValue).Duration(duration).Start();
    return Instance().AddTween(std::move(tween));
}

Tween<float>& TweenManager::DelayedCall(float delay, std::function<void()> callback)
{
    auto tween = std::make_unique<Tween<float>>();
    tween->From(0.0f).To(1.0f).Duration(0.001f).Delay(delay).OnComplete(callback).Start();
    return Instance().AddTween(std::move(tween));
}

} // namespace deki
