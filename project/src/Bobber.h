#pragma once

#include "DekiEngine.h"

// Moves the owner up and down on a sine wave. Exported fields drive the
// inspector, serialization, and undo without any extra registration.
class Bobber : public DekiBehaviour
{
    DEKI_COMPONENT(Bobber, DekiBehaviour, "Demo", "91ce8c51-680b-4ff7-aa74-b44026097d74", "")
    DEKI_DESCRIPTION("Bobs the object up and down on a sine wave.")
public:
    DEKI_EXPORT DEKI_SLIDER(0, 64)
    float amplitude_px = 12.0f;

    DEKI_EXPORT DEKI_SLIDER(0, 4)
    float speed_hz = 0.6f;

    // Offsets the wave so a row of bobbers ripples instead of moving as one.
    DEKI_EXPORT DEKI_SLIDER(0, 1)
    float phase = 0.0f;

    Bobber() = default;

    void Awake() override;
    void Update() override;

private:
    float base_y = 0.0f;
    float elapsed_s = 0.0f;
};

#include "generated/Bobber.gen.h"
