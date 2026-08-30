#include "Spinner.h"

void Spinner::Awake()
{
    SetNeedsUpdate(true);

    if (DekiObject* owner = GetOwner())
        m_AngleDeg = owner->GetLocalRotation() * 57.29577951f;
}

void Spinner::Update()
{
    DekiObject* owner = GetOwner();
    if (!owner)
        return;

    const float dt_s = DekiTime::GetDeltaTimeF() * 0.001f;
    m_AngleDeg += degreesPerSecond * dt_s;

    // Keep the accumulator bounded so a long-running scene does not lose
    // precision in the float.
    if (m_AngleDeg >= 360.0f) m_AngleDeg -= 360.0f;
    if (m_AngleDeg <= -360.0f) m_AngleDeg += 360.0f;

    owner->SetRotation(m_AngleDeg * 0.01745329252f);
}
