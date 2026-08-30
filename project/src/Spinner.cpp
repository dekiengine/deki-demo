#include "Spinner.h"

void Spinner::Awake()
{
    SetNeedsUpdate(true);

    if (DekiObject* owner = GetOwner())
        angle_deg = owner->GetLocalRotation() * 57.29577951f;
}

void Spinner::Update()
{
    DekiObject* owner = GetOwner();
    if (!owner)
        return;

    const float dt_s = DekiTime::GetDeltaTimeF() * 0.001f;
    angle_deg += degrees_per_second * dt_s;

    // Keep the accumulator bounded so a long-running scene does not lose
    // precision in the float.
    if (angle_deg >= 360.0f) angle_deg -= 360.0f;
    if (angle_deg <= -360.0f) angle_deg += 360.0f;

    owner->SetRotation(angle_deg * 0.01745329252f);
}
