#include "Walker.h"

#include "AnimationComponent.h"

// AnimationComponent moved into the Deki2D namespace in deki-2d 0.16.0.
using namespace Deki2D;

void Walker::Awake()
{

    if (Deki::Object* owner = GetOwner())
        m_BaseX = owner->GetX();

    m_Leftwards = startLeftwards;
    Face(m_Leftwards);
}

void Walker::Update()
{
    Deki::Object* owner = GetOwner();
    if (!owner)
        return;

    const float dt = Deki::Time::GetDeltaTimeF() * 0.001f;

    // Positions are in meters; the exported speed and distance are in pixels
    // so the inspector values match the 320x240 panel.
    const float ppm = Deki::EngineSettings::Global().pixelsPerMeter;
    const float reach = patrolPx / ppm;
    const float step = (speedPx / ppm) * dt;

    float x = owner->GetX() + (m_Leftwards ? -step : step);
    if (x >= m_BaseX + reach)
    {
        x = m_BaseX + reach;
        m_Leftwards = true;
        Face(true);
    }
    else if (x <= m_BaseX - reach)
    {
        x = m_BaseX - reach;
        m_Leftwards = false;
        Face(false);
    }

    owner->SetX(x);
}

void Walker::Face(bool leftwards)
{
    const std::string& name = leftwards ? walkLeftAnimation : walkRightAnimation;
    if (name.empty())
        return;

    Deki::Object* owner = GetOwner();
    AnimationComponent* anim = owner ? owner->GetComponent<AnimationComponent>() : nullptr;
    if (anim)
        anim->PlayAnimation(name.c_str());
}
