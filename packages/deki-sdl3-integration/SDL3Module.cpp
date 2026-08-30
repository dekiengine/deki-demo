/**
 * @file SDL3Package.cpp
 * @brief Package entry point for deki-sdl3 DLL
 *
 * This file exports the standard Deki plugin interface so the editor
 * can load deki-sdl3.dll and discover available SDL3 components.
 *
 * Display and input are set up by their SetupComponents; the SDL3 time provider
 * is registered in SDL3Display.cpp. The desktop program entry (main) and the
 * memory/filesystem HAL live in the deki-desktop-integration package.
 */

#include "SDL3Package.h"
#include "interop/DekiPlugin.h"
#include "DekiPackageFeatureMeta.h"
#include "DekiTime.h"
#include "providers/ITimeProvider.h"
#include "reflection/ComponentRegistry.h"
#include "reflection/ComponentFactory.h"

#ifdef DEKI_EDITOR

// Auto-generated registration helpers
extern void DekiSDL3_RegisterComponents();
extern int DekiSDL3_GetAutoComponentCount();
extern const DekiComponentMeta* DekiSDL3_GetAutoComponentMeta(int index);

// Track if already registered to avoid duplicates
static bool s_SDL3Registered = false;

extern "C" {

/**
 * @brief Ensure deki-sdl3 package is loaded and components are registered
 */
DEKI_SDL3_API int DekiSDL3_EnsureRegistered(void)
{
    if (s_SDL3Registered)
        return DekiSDL3_GetAutoComponentCount();
    s_SDL3Registered = true;

    // Auto-generated: registers all SDL3 components with ComponentRegistry + ComponentFactory
    DekiSDL3_RegisterComponents();

    return DekiSDL3_GetAutoComponentCount();
}

// =============================================================================
// Plugin metadata (for dynamic loading compatibility)
// =============================================================================

DEKI_PLUGIN_API const char* DekiPlugin_GetName(void)
{
    return "Deki SDL3 Package";
}

DEKI_PLUGIN_API const char* DekiPlugin_GetVersion(void)
{
#ifdef DEKI_PACKAGE_VERSION
    return DEKI_PACKAGE_VERSION;
#else
    return "0.0.0-dev";
#endif
}

DEKI_PLUGIN_API const char* DekiPlugin_GetReflectionJson(void)
{
    return "{}";
}

DEKI_PLUGIN_API int DekiPlugin_Init(void)
{
    return 0;
}

DEKI_PLUGIN_API void DekiPlugin_Shutdown(void)
{
    s_SDL3Registered = false;
    DekiTime::SetTimeProvider(nullptr);
}

DEKI_PLUGIN_API int DekiPlugin_GetComponentCount(void)
{
    return DekiSDL3_GetAutoComponentCount();
}

DEKI_PLUGIN_API const DekiComponentMeta* DekiPlugin_GetComponentMeta(int index)
{
    return DekiSDL3_GetAutoComponentMeta(index);
}

DEKI_PLUGIN_API void DekiPlugin_RegisterComponents(void)
{
    DekiSDL3_EnsureRegistered();
}

// =============================================================================
// Package Feature API
// =============================================================================

static const DekiPackageFeatureInfo s_Features[] = {
    {"display", "Display", "SDL3 window display", false},
    {"input", "Input", "SDL3 keyboard/mouse input", false},
};

DEKI_PLUGIN_API int DekiPlugin_GetFeatureCount(void)
{
    return sizeof(s_Features) / sizeof(s_Features[0]);
}

DEKI_PLUGIN_API const DekiPackageFeatureInfo* DekiPlugin_GetFeature(int index)
{
    if (index < 0 || index >= DekiPlugin_GetFeatureCount())
        return nullptr;
    return &s_Features[index];
}

// =============================================================================
// Package-specific feature API (for linked DLL access without name conflicts)
// =============================================================================

DEKI_SDL3_API const char* DekiSDL3_GetName(void)
{
    return "SDL3";
}

DEKI_SDL3_API int DekiSDL3_GetFeatureCount(void)
{
    return DekiPlugin_GetFeatureCount();
}

DEKI_SDL3_API const DekiPackageFeatureInfo* DekiSDL3_GetFeature(int index)
{
    return DekiPlugin_GetFeature(index);
}

} // extern "C"

#else // !DEKI_EDITOR - Runtime registration

// Component registration happens via the auto-generated DekiSDL3_RegisterComponents(),
// called from deki_register_project_packages(). Display/input run as boot SetupComponents,
// and the SDL3 time provider is registered by a static initializer in SDL3Display.cpp.

#endif // DEKI_EDITOR
