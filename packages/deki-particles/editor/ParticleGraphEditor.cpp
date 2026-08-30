/**
 * @file ParticleGraphEditor.cpp
 * @brief Editor-side registration for the ParticleGraph effect asset.
 *
 * Registers (a) the asset type so the Asset Browser's Create... menu offers
 * "Particle Effect" with a valid starting graph, and (b) the node-graph domain
 * so the generic Node Graph window claims this asset type and scopes its
 * add-node menu to the "Particles" node categories (see ParticleNodes.h).
 * Compilation needs no code here: the type has a runtime loader, so the
 * generic data-asset path transcodes the JSON to a MessagePack cache.
 */

#ifdef DEKI_EDITOR

#include <deki-editor/EditorExtension.h>
#include <deki-editor/EditorRegistry.h>

#include "deki-nodegraph/DekiNode.h"

namespace DekiEditor
{

class ParticleGraphAssetEditor : public AssetTypeEditor
{
public:
    const char* GetTypeName() const override    { return "ParticleGraph"; }
    const char* GetDisplayName() const override { return "Particle Effect"; }
    const char* GetExtension() const override   { return ".asset"; }

    // A new effect starts as the permanent Emitter node wired to an Emission
    // node, which is the smallest graph that actually produces particles. An
    // effect with no emission would spawn nothing and look broken on creation.
    const char* GetDefaultContent() const override
    {
        return R"({
  "links": [
    { "from": 1, "fromPin": 0, "to": 2, "toPin": 0 }
  ],
  "nextNodeId": 3,
  "nodes": [
    { "id": 1, "type": "ParticleEmit", "values": {}, "x": 60.0, "y": 120.0 },
    { "id": 2, "type": "ParticleEmission", "values": {}, "x": 300.0, "y": 120.0 }
  ],
  "type": "ParticleGraph"
})";
    }

    int GetCompileTarget() const override { return 2; }  // Data
};

REGISTER_EDITOR(ParticleGraphAssetEditor)

} // namespace DekiEditor

// Implemented in ParticlePreview.cpp: runs the graph being edited and draws
// its particles, so the Node Graph window can offer a Preview panel.
NodeGraphPreviewOps DekiParticles_PreviewOps();

// Implemented in ParticleNodeGizmos.cpp: draws the selected node's shape, arc
// or ramp in the properties panel, under its title.
NodeGraphNodeGizmoOps DekiParticles_GizmoOps();

REGISTER_NODE_GRAPH_DOMAIN_PREVIEW_GIZMOS(g_ParticleDomain,
                                          "ParticleGraph", "Particle Effect",
                                          "Particles", "ParticleEmit",
                                          DekiParticles_PreviewOps(),
                                          DekiParticles_GizmoOps());

// Re-registration hook for plugin-only hot reload: the editor wipes the domain
// registry while this DLL stays loaded, so the static registrar above never
// reruns. Registry Register() dedupes, so calling this repeatedly is safe.
// Invoked from DekiParticles_RegisterGraphTypes (DekiParticlesPackage.cpp).
extern "C" void DekiParticles_RegisterEditorGraphDomain(void)
{
    NodeGraphDomainRegistry::Instance().Register(&g_ParticleDomain);
}

#endif // DEKI_EDITOR
