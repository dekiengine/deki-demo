#pragma once

#include <deki/Engine.h>

#include <cstdint>

// What a demo needs from the board.
enum class DemoHardware : uint8_t
{
    Gps = 0,
    LoRa = 1,
    Keyboard = 2,
    Trackball = 3
};

// Greys out the menu button on the same object when the board lacks what its
// demo needs. Asks the drivers the boot scene started, so a board's boot scene
// is the one place that says what it has.
DEKI_CATEGORY("Demo")
DEKI_DESCRIPTION("Disables this object's button when the board lacks the hardware its demo needs.")
class HardwareGate : public Deki::Component
{
public:
    DEKI_EXPORT
    DEKI_TOOLTIP("The hardware the demo behind this button needs.")
    DemoHardware hardware = DemoHardware::Gps;

    HardwareGate() = default;

    void Start() override;

    // Whether this board has `hardware` running.
    static bool BoardHas(DemoHardware hardware);
};
