#include "TweenComponent.h"
#include "TweenManager.h"
#include "DekiTime.h"
#include "DekiObject.h"

TweenComponent::TweenComponent()
    : target_type(TweenTargetType::Position)
    , end_value(0.0f, 0.0f, 0.0f)
    , duration(1.0f)
    , delay(0.0f)
    , ease_type(deki::EaseType::Linear)
    , loops(0)
    , ping_pong(false)
    , auto_play(true)
    , relative(false)
    , m_Elapsed(0.0f)
    , m_DelayElapsed(0.0f)
    , m_CurrentLoop(0)
    , m_Reversed(false)
    , m_HasCompleted(false)
    , m_IsPlaying(false)
    , m_IsPaused(false)
    , m_StartValue(0.0f, 0.0f, 0.0f)
{
    SetNeedsUpdate(true);
}

TweenComponent::~TweenComponent()
{
}

void TweenComponent::Awake()
{
    // Nothing to cache - we capture start value when Play() is called
}

void TweenComponent::Start()
{
    if (auto_play)
    {
        Play();
    }
}

void TweenComponent::Update()
{
    // Also update the global TweenManager for static API tweens
    deki::TweenManager::Instance().EnsureUpdatedThisFrame();

    if (!m_IsPlaying || m_IsPaused)
        return;

    // DekiTime returns float milliseconds; convert to seconds.
    float deltaSeconds = DekiTime::GetDeltaTimeF() / 1000.0f;

    // Handle delay
    if (m_DelayElapsed < delay)
    {
        m_DelayElapsed += deltaSeconds;
        if (m_DelayElapsed < delay)
            return;
        // Apply remaining time after delay
        deltaSeconds = m_DelayElapsed - delay;
    }

    m_Elapsed += deltaSeconds;
    float progress = duration > 0.0f ? (m_Elapsed / duration) : 1.0f;
    if (progress >= 1.0f)
    {
        progress = 1.0f;
    }

    // Apply easing
    float t = m_Reversed ? (1.0f - progress) : progress;
    float easedT = GetEasedProgress(t);

    // Apply interpolated value
    ApplyValue(easedT);

    // Call update callback
    if (on_update)
    {
        on_update(progress);
    }

    // Check completion
    if (progress >= 1.0f)
    {
        HandleLoopOrComplete();
    }
}

void TweenComponent::Play()
{
    m_IsPlaying = true;
    m_IsPaused = false;
    m_HasCompleted = false;
    m_Elapsed = 0.0f;
    m_DelayElapsed = 0.0f;
    m_CurrentLoop = 0;
    m_Reversed = false;

    // Capture current value as start value
    m_StartValue = GetCurrentValue();
}

void TweenComponent::Pause()
{
    if (m_IsPlaying)
    {
        m_IsPaused = true;
    }
}

void TweenComponent::Resume()
{
    if (m_IsPlaying && m_IsPaused)
    {
        m_IsPaused = false;
    }
}

void TweenComponent::Stop()
{
    m_IsPlaying = false;
    m_IsPaused = false;
    m_Elapsed = 0.0f;
    m_DelayElapsed = 0.0f;
    m_CurrentLoop = 0;
    m_Reversed = false;
}

void TweenComponent::SetOnComplete(std::function<void()> callback)
{
    on_complete = callback;
}

float TweenComponent::GetProgress() const
{
    if (duration <= 0.0f)
        return 1.0f;
    return m_Elapsed / duration;
}

DekiVector3 TweenComponent::GetCurrentValue() const
{
    DekiObject* owner = GetOwner();
    if (!owner)
        return DekiVector3(0.0f, 0.0f, 0.0f);

    switch (target_type)
    {
    case TweenTargetType::Position:
        return DekiVector3(owner->GetX(), owner->GetY(), 0.0f);
    case TweenTargetType::Scale:
        return DekiVector3(owner->GetScaleX(), owner->GetScaleY(), 0.0f);
    case TweenTargetType::Rotation:
        return DekiVector3(0.0f, 0.0f, owner->GetLocalRotation());
    default:
        return DekiVector3(0.0f, 0.0f, 0.0f);
    }
}

void TweenComponent::ApplyValue(float easedT)
{
    DekiObject* owner = GetOwner();
    if (!owner)
        return;

    // Calculate target value (relative adds to start, absolute uses end_value directly)
    DekiVector3 targetValue = relative ? m_StartValue + end_value : end_value;

    // Interpolate from start to target.
    DekiVector3 value = DekiVector3::Lerp(m_StartValue, targetValue, easedT);

    switch (target_type)
    {
    case TweenTargetType::Position:
        owner->SetLocalPosition(value.x, value.y);
        break;
    case TweenTargetType::Scale:
        owner->SetScale(value.x, value.y);
        break;
    case TweenTargetType::Rotation:
        owner->SetRotation(value.z);
        break;
    case TweenTargetType::COUNT:
        // Not a valid target type, ignore
        break;
    }
}

float TweenComponent::GetEasedProgress(float t) const
{
    deki::EasingFunc func = deki::Ease::GetFunction(ease_type);
    return func(t);
}

void TweenComponent::HandleLoopOrComplete()
{
    // Check if we should loop
    if (loops == -1 || m_CurrentLoop < loops - 1)
    {
        m_CurrentLoop++;
        m_Elapsed = 0.0f;

        if (ping_pong)
        {
            m_Reversed = !m_Reversed;
        }
    }
    else
    {
        // Complete
        m_IsPlaying = false;
        m_HasCompleted = true;

        if (on_complete)
        {
            on_complete();
        }
    }
}
