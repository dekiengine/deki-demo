#include "SDL3DisplaySetup.h"
#include "DekiLogSystem.h"

#if !defined(DEKI_EDITOR) && defined(DEKI_PACKAGE_SDL3)

#include "SDL3Display.h"
#include "DekiEngine.h"

// Package owns the SDL3Display lifetime now (engine-core just holds the pointer).
// File-scope unique_ptr keeps it alive for the program's lifetime.
static std::unique_ptr<SDL3Display> s_SDL3Display;

void SDL3DisplaySetup::Setup(SetupCallback onComplete)
{
    int32_t scaled_width = window_width * window_scale;
    int32_t scaled_height = window_height * window_scale;

    DEKI_LOG_INFO("SDL3DisplaySetup: Creating SDL3 display (%dx%d, scale=%d)",
                  window_width, window_height, window_scale);

    s_SDL3Display = std::make_unique<SDL3Display>();
    if (s_SDL3Display && s_SDL3Display->Initialize(scaled_width, scaled_height))
    {
        DekiEngine::GetInstance().SetDisplay(s_SDL3Display.get(), "SDL3");
        DEKI_LOG_INFO("SDL3DisplaySetup: Display initialized successfully");
        onComplete(true);
    }
    else
    {
        DEKI_LOG_ERROR("SDL3DisplaySetup: Failed to initialize SDL3 display");
        s_SDL3Display.reset();
        onComplete(false);
    }
}

#else

void SDL3DisplaySetup::Setup(SetupCallback onComplete)
{
    // Editor/ESP32: SDL3 display not applicable
    onComplete(true);
}

#endif
