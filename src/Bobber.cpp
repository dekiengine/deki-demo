#include "Bobber.h"

#include <cmath>

void Bobber::Awake()
{

    if (Deki::Object* owner = GetOwner())
        m_BaseY = owner->GetY();
}

void Bobber::Update()
{
    Deki::Object* owner = GetOwner();
    if (!owner)
        return;

    m_ElapsedS += Deki::Time::GetDeltaTimeF() * 0.001f;

    // Positions are in meters; the exported amplitude is in pixels so the
    // inspector value matches what you see on a 320x240 panel.
    const float ppm = Deki::EngineSettings::Global().pixelsPerMeter;
    const float wave = std::sin((m_ElapsedS * speedHz + phase) * 6.283185307f);

    owner->SetY(m_BaseY + (amplitudePx / ppm) * wave);
}
