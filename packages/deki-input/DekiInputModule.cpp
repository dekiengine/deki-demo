/**
 * @file DekiInputPackage.cpp
 * @brief Package entry point for deki-input DLL
 *
 * Exports the standard Deki plugin interface so the editor
 * can load deki-input.dll and register its components (InputCollider).
 */

#include "DekiInputPackage.h"
#include "interop/DekiPlugin.h"
#include "DekiPackageFeatureMeta.h"
#include "InputCollider.h"
#include "DekiInputInit.h"
#include "DekiEngine.h"
#include "reflection/ComponentRegistry.h"
#include "reflection/ComponentFactory.h"

#ifdef DEKI_EDITOR

#ifndef DEKI_PLUGIN_EXPORTS
// Auto-generated registration helpers (standalone DLL only)
extern void DekiInput_RegisterComponents();
extern int DekiInput_GetAutoComponentCount();
extern const DekiComponentMeta* DekiInput_GetAutoComponentMeta(int index);

// Track if already registered to avoid duplicates
static bool s_InputRegistered = false;

extern "C" {

/**
 * @brief Ensure deki-input package is loaded and components are registered
 */
DEKI_INPUT_API int DekiInput_EnsureRegistered(void)
{
    if (s_InputRegistered)
        return DekiInput_GetAutoComponentCount();
    s_InputRegistered = true;

    DekiInput_RegisterComponents();

    // Initialize input system (idempotent — may already be initialized
    // by deki_init_package_systems() during DekiEngine::Initialize())
    DekiInput_InitSystem();

    return DekiInput_GetAutoComponentCount();
}

} // extern "C"
#endif // DEKI_PLUGIN_EXPORTS

// =============================================================================
// Plugin metadata (for dynamic loading compatibility)
// =============================================================================

extern "C" {

#ifndef DEKI_PLUGIN_EXPORTS
DEKI_PLUGIN_API const char* DekiPlugin_GetName(void)
{
    return "Deki Input Package";
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
    DekiInput_ShutdownSystem();
    s_InputRegistered = false;
}

DEKI_PLUGIN_API int DekiPlugin_GetComponentCount(void)
{
    return DekiInput_GetAutoComponentCount();
}

DEKI_PLUGIN_API const DekiComponentMeta* DekiPlugin_GetComponentMeta(int index)
{
    return DekiInput_GetAutoComponentMeta(index);
}

DEKI_PLUGIN_API void DekiPlugin_RegisterComponents(void)
{
    DekiInput_EnsureRegistered();
}

#endif // DEKI_PLUGIN_EXPORTS (resume after feature data)

// =============================================================================
// Package Feature API
// =============================================================================

static const char* s_InputGuids[] = { InputCollider::StaticGuid };

static const DekiPackageFeatureInfo s_Features[] = {
    {"input", "Input", "Input dispatch with InputCollider hit areas", true, "DEKI_FEATURE_INPUT", s_InputGuids, 1},
};

#ifndef DEKI_PLUGIN_EXPORTS
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

#endif // DEKI_PLUGIN_EXPORTS

// =============================================================================
// Package-specific feature API
// =============================================================================

DEKI_INPUT_API const char* DekiInput_GetName(void)
{
    return "Input";
}

DEKI_INPUT_API int DekiInput_GetFeatureCount(void)
{
    return static_cast<int>(sizeof(s_Features) / sizeof(s_Features[0]));
}

DEKI_INPUT_API const DekiPackageFeatureInfo* DekiInput_GetFeature(int index)
{
    if (index < 0 || index >= DekiInput_GetFeatureCount())
        return nullptr;
    return &s_Features[index];
}

} // extern "C"

#endif // DEKI_EDITOR
