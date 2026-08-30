#include "SDL3InputSetup.h"
#include "DekiLogSystem.h"

#if !defined(DEKI_EDITOR) && defined(DEKI_PACKAGE_SDL3)

#include "SDL3Input.h"
#include "DekiInput.h"  // from deki-input
#include "DekiInputInit.h"  // DekiInput_InitSystem (from deki-input)
#include "DekiEngine.h"
#include "providers/IDekiInputSystem.h"

void SDL3InputSetup::Setup(SetupCallback onComplete)
{
    DEKI_LOG_INFO("SDL3InputSetup: Initializing SDL3 input (keyboard=%d, mouse=%d)",
                  enable_keyboard, enable_mouse);

    auto input = std::make_unique<SDL3Input>();
    if (input->Initialize())
    {
        DekiInput::SetInput(std::move(input), "SDL3");

        // Ensure the input dispatch system (deki-input) is created, initialized, and
        // registered with the engine. On editor/firmware builds the generated
        // deki_init_package_systems() already does this; the static desktop sim links the
        // engine's empty stub for that symbol, so do it here. Idempotent if already done.
        DekiInput_InitSystem();

        DEKI_LOG_INFO("SDL3InputSetup: Input initialized successfully");
        onComplete(true);
    }
    else
    {
        DEKI_LOG_ERROR("SDL3InputSetup: Failed to initialize SDL3 input");
        onComplete(false);
    }
}

#else

void SDL3InputSetup::Setup(SetupCallback onComplete)
{
    // Editor/ESP32: SDL3 input not applicable
    onComplete(true);
}

#endif
