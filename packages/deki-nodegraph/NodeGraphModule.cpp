/**
 * @file NodeGraphModule.cpp
 * @brief Package entry point for the deki-nodegraph DLL.
 *
 * deki-nodegraph owns the whole node-graph feature: the runtime container
 * (NodeGraphData) and factory that every platform needs, plus the editor-side
 * registries, document, undo commands and the generic Node Graph window.
 *
 * It declares no node types and no components of its own. Node types and graph
 * domains come from the packages and project DLLs that consume it (deki-fsm's
 * states and actions, a game's own nodes) via DEKI_NODE / the registration
 * macros in DekiNode.h.
 */

#include "interop/DekiPlugin.h"
#include "deki-nodegraph/DekiNode.h"
#include "DekiLogSystem.h"

#ifdef DEKI_EDITOR
// Auto-generated registration helpers (no components, but the codegen always
// emits the trio so the plugin interface below has something to call).
extern void DekiNodeGraph_RegisterComponents();
extern int  DekiNodeGraph_GetAutoComponentCount();
extern const DekiComponentMeta* DekiNodeGraph_GetAutoComponentMeta(int index);
#endif

static bool s_NodeGraphRegistered = false;

extern "C" {

DEKI_NODEGRAPH_API int DekiNodeGraph_EnsureRegistered(void)
{
#ifdef DEKI_EDITOR
    if (s_NodeGraphRegistered) return DekiNodeGraph_GetAutoComponentCount();
    s_NodeGraphRegistered = true;
    DekiNodeGraph_RegisterComponents();
    return DekiNodeGraph_GetAutoComponentCount();
#else
    return 0;
#endif
}

DEKI_PLUGIN_API const char* DekiPlugin_GetName(void) { return "Deki Node Graph Package"; }

DEKI_PLUGIN_API const char* DekiPlugin_GetVersion(void)
{
#ifdef DEKI_PACKAGE_VERSION
    return DEKI_PACKAGE_VERSION;
#else
    return "0.0.0-dev";
#endif
}

DEKI_PLUGIN_API const char* DekiPlugin_GetReflectionJson(void) { return "{}"; }

DEKI_PLUGIN_API int DekiPlugin_Init(void) { return 0; }

DEKI_PLUGIN_API void DekiPlugin_Shutdown(void) { s_NodeGraphRegistered = false; }

#ifdef DEKI_EDITOR
DEKI_PLUGIN_API int DekiPlugin_GetComponentCount(void)
{
    return DekiNodeGraph_GetAutoComponentCount();
}
DEKI_PLUGIN_API const DekiComponentMeta* DekiPlugin_GetComponentMeta(int index)
{
    return DekiNodeGraph_GetAutoComponentMeta(index);
}
#else
DEKI_PLUGIN_API int DekiPlugin_GetComponentCount(void) { return 0; }
DEKI_PLUGIN_API const DekiComponentMeta* DekiPlugin_GetComponentMeta(int) { return nullptr; }
#endif

DEKI_PLUGIN_API void DekiPlugin_RegisterComponents(void)
{
    DekiNodeGraph_EnsureRegistered();
}

#ifdef DEKI_EDITOR
/**
 * @brief Drop every registered node type, factory thunk and graph domain.
 *
 * The editor calls this on each loaded package before it unloads plugin or
 * package DLLs: the registries hold meta and thunk pointers INTO those DLLs,
 * and reading them after FreeLibrary is a crash. Owning the wipe here is what
 * keeps the editor free of node-graph knowledge — it just asks every package to
 * clean up after itself.
 *
 * Consumers repopulate on the way back up: full reloads rerun their static
 * registrars, plugin-only reloads go through DekiPlugin_RegisterComponents
 * (see deki-fsm's DekiFsm_RegisterGraphTypes).
 */
DEKI_PLUGIN_API void DekiPlugin_ClearRegistries(void)
{
    SceneFormat::NodeFactory::Instance().Clear();
    NodeTypeRegistry::Instance().Clear();
    NodeGraphDomainRegistry::Instance().Clear();
}
#endif

DEKI_PLUGIN_API int DekiPlugin_GetFeatureCount(void) { return 0; }
DEKI_PLUGIN_API const struct DekiPackageFeatureInfo* DekiPlugin_GetFeature(int) { return nullptr; }

// Infrastructure package — no systems to install into the engine at load.
DEKI_PLUGIN_API void DekiPlugin_RegisterPackages(void) {}

} // extern "C"
