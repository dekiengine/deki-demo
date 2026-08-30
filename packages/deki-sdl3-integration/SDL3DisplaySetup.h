#pragma once

#include <cstdint>
#include "SetupComponent.h"
#include "reflection/DekiProperty.h"
#include "SDL3Package.h"

/**
 * @brief Component to configure and initialize an SDL3 display window
 *
 * Add this component to your boot scene to set up an SDL3 window.
 * Configure the window dimensions and scale in the Inspector.
 *
 * Inherits from SetupComponent to participate in boot sequence.
 */
class DEKI_SDL3_API SDL3DisplaySetup : public SetupComponent
{
public:
    DEKI_COMPONENT(SDL3DisplaySetup, SetupComponent, "SDL3", "c3a8f1d0-5b7e-4a2c-8d91-6e3f4c7b2a50", "DEKI_FEATURE_SDL3_DISPLAY_SETUP")
    DEKI_DESCRIPTION("Opens the desktop window the game renders into.")

    DEKI_EXPORT
    DEKI_TOOLTIP("Window width in pixels")
    DEKI_RANGE(64, 3840)
    int32_t window_width = 320;

    DEKI_EXPORT
    DEKI_TOOLTIP("Window height in pixels")
    DEKI_RANGE(64, 2160)
    int32_t window_height = 240;

    DEKI_EXPORT
    DEKI_TOOLTIP("Window scale multiplier")
    DEKI_RANGE(1, 8)
    int32_t window_scale = 2;

    void Setup(SetupCallback onComplete) override;
    const char* GetSetupName() const override { return "SDL3 Display"; }
};

// Generated property metadata
#include "generated/SDL3DisplaySetup.gen.h"
