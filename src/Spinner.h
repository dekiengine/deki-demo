#pragma once

#include "DekiEngine.h"

// Spins the owner at a constant rate.
class Spinner : public Deki::Behaviour
{
    DEKI_COMPONENT(Spinner, Deki::Behaviour, "Demo", "e335b7e0-e04f-4921-b3b0-b88faef7f519", "")
    DEKI_DESCRIPTION("Rotates the object at a constant number of degrees per second.")
public:
    DEKI_EXPORT DEKI_SLIDER(-360, 360)
    float degreesPerSecond = 45.0f;

    Spinner() = default;

    void Awake() override;
    void Update() override;

private:
    float m_AngleDeg = 0.0f;
};

#include "generated/Spinner.gen.h"
