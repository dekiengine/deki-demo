#include "Bobber.h"

#include <cmath>

void Bobber::Awake()
{
    SetNeedsUpdate(true);

    if (DekiObject* owner = GetOwner())
        base_y = owner->GetY();
}

void Bobber::Update()
{
    DekiObject* owner = GetOwner();
    if (!owner)
        return;

    elapsed_s += DekiTime::GetDeltaTimeF() * 0.001f;

    // Positions are in meters; the exported amplitude is in pixels so the
    // inspector value matches what you see on a 320x240 panel.
    const float ppm = DekiEngineSettings::Global().pixelsPerMeter;
    const float wave = std::sin((elapsed_s * speed_hz + phase) * 6.283185307f);

    owner->SetY(base_y + (amplitude_px / ppm) * wave);
}
