/**
 * @file TweenPackage.cpp
 * @brief Package entry point for deki-tween DLL
 *
 * This file exports the standard Deki plugin interface so the editor
 * can load deki-tween.dll and register its components (TweenComponent).
 *
 * For linked DLLs (not dynamically loaded), DekiTween_EnsureRegistered()
 * must be called from the main executable to trigger the static initializers.
 */

#include "interop/DekiPlugin.h"
#include "DekiPackageFeatureMeta.h"
#include "TweenComponent.h"
#include "TweenManager.h"
#include "reflection/ComponentRegistry.h"
#include "reflection/ComponentFactory.h"

#ifdef DEKI_EDITOR

// =============================================================================
// Linked DLL initialization
// =============================================================================
// When deki-tween is linked (not dynamically loaded), the editor must call
// this function to ensure the DLL code is actually loaded and the static
// initializers (REGISTER_COMPONENT) have run.

// Auto-generated registration helpers
extern void DekiTween_RegisterComponents();
extern int DekiTween_GetAutoComponentCount();
extern const DekiComponentMeta* DekiTween_GetAutoComponentMeta(int index);

// Track if already registered to avoid duplicates
static bool s_TweenRegistered = false;

extern "C" {

/**
 * @brief Ensure deki-tween package is loaded and components are registered
 *
 * Call this from the editor at startup. Simply calling this function is enough
 * to force the linker to include the DLL and trigger static initializers.
 *
 * @return Number of components registered by this package
 */
DEKI_TWEEN_API int DekiTween_EnsureRegistered(void)
{
    if (s_TweenRegistered)
        return DekiTween_GetAutoComponentCount();
    s_TweenRegistered = true;

    // Auto-generated: registers all Tween components with ComponentRegistry + ComponentFactory
    DekiTween_RegisterComponents();

    return DekiTween_GetAutoComponentCount();
}

} // extern "C"

// =============================================================================
// Plugin metadata (for dynamic loading compatibility)
// =============================================================================

extern "C" {

DEKI_PLUGIN_API const char* DekiPlugin_GetName(void)
{
    return "Deki Tween Package";
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
    // Not used - we use component metadata instead
    return "{}";
}

DEKI_PLUGIN_API int DekiPlugin_Init(void)
{
    // No special initialization needed
    return 0;
}

DEKI_PLUGIN_API void DekiPlugin_Shutdown(void)
{
    s_TweenRegistered = false;
}

DEKI_PLUGIN_API int DekiPlugin_GetComponentCount(void)
{
    return DekiTween_GetAutoComponentCount();
}

DEKI_PLUGIN_API const DekiComponentMeta* DekiPlugin_GetComponentMeta(int index)
{
    return DekiTween_GetAutoComponentMeta(index);
}

DEKI_PLUGIN_API void DekiPlugin_RegisterComponents(void)
{
    DekiTween_EnsureRegistered();
}

DEKI_PLUGIN_API void DekiPlugin_OnPlayModeStop(void)
{
    deki::TweenManager::Instance().KillAll();
}

// deki-tween renders no editor UI of its own, so it links no ImGui and shares no
// ImGui context. Its component inspectors are drawn by the editor via reflection.

// =============================================================================
// Package Feature API
// =============================================================================

static const char* s_TweenGuids[] = { TweenComponent::StaticGuid };

static const DekiPackageFeatureInfo s_Features[] = {
    {"tween", "Tween", "Value interpolation with easing functions", true, "DEKI_FEATURE_TWEEN", s_TweenGuids, 1},
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

DEKI_TWEEN_API const char* DekiTween_GetName(void)
{
    return "Tween";
}

DEKI_TWEEN_API int DekiTween_GetFeatureCount(void)
{
    return DekiPlugin_GetFeatureCount();
}

DEKI_TWEEN_API const DekiPackageFeatureInfo* DekiTween_GetFeature(int index)
{
    return DekiPlugin_GetFeature(index);
}

} // extern "C"

#endif // DEKI_EDITOR
