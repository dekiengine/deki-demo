#pragma once

#include <deki/Engine.h>

// Spins the owner at a constant rate.
DEKI_CATEGORY("Demo")
DEKI_DESCRIPTION("Rotates the object at a constant number of degrees per second.")
class Spinner : public Deki::Component
{
public:
    DEKI_EXPORT DEKI_SLIDER(-360, 360)
    float degreesPerSecond = 45.0f;

    Spinner() = default;

    void Awake() override;
    void Update() override;

private:
    float m_AngleDeg = 0.0f;
};

