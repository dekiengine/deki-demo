#pragma once

#include "SetupComponent.h"
#include "reflection/DekiProperty.h"
#include "SDL3Package.h"

/**
 * @brief Component to configure and initialize SDL3 keyboard/mouse input
 *
 * Add this component to your boot scene to enable SDL3 input.
 *
 * Inherits from SetupComponent to participate in boot sequence.
 */
class DEKI_SDL3_API SDL3InputSetup : public SetupComponent
{
public:
    DEKI_COMPONENT(SDL3InputSetup, SetupComponent, "SDL3", "d4b9e2f1-6c8a-4d3b-9e72-7f4g5d8c3b61", "DEKI_FEATURE_SDL3_INPUT_SETUP")
    DEKI_DESCRIPTION("Feeds desktop keyboard and mouse input to the engine.")

    DEKI_EXPORT
    DEKI_TOOLTIP("Enable keyboard input")
    bool enable_keyboard = true;

    DEKI_EXPORT
    DEKI_TOOLTIP("Enable mouse/pointer input")
    bool enable_mouse = true;

    void Setup(SetupCallback onComplete) override;
    const char* GetSetupName() const override { return "SDL3 Input"; }
};

// Generated property metadata
#include "generated/SDL3InputSetup.gen.h"
