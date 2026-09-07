#pragma once

#include <deki/Engine.h>

// Moves the owner up and down on a sine wave. Exported fields drive the
// inspector, serialization, and undo without any extra registration.
DEKI_CATEGORY("Demo")
DEKI_DESCRIPTION("Bobs the object up and down on a sine wave.")
class Bobber : public Deki::Behaviour
{
public:
    DEKI_EXPORT DEKI_SLIDER(0, 64)
    float amplitudePx = 12.0f;

    DEKI_EXPORT DEKI_SLIDER(0, 4)
    float speedHz = 0.6f;

    // Offsets the wave so a row of bobbers ripples instead of moving as one.
    DEKI_EXPORT DEKI_SLIDER(0, 1)
    float phase = 0.0f;

    Bobber() = default;

    void Awake() override;
    void Update() override;

private:
    float m_BaseY = 0.0f;
    float m_ElapsedS = 0.0f;
};

