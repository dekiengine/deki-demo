#pragma once

#ifdef DEKI_EDITOR

/**
 * @file NodeGraphDomainRegistry.h
 * @brief Editor-only registry of node-graph domains: which .asset types are
 *        node graphs, and which node categories belong to each graph type.
 *
 * A domain is registered by the owning package/project DLL (via
 * REGISTER_NODE_GRAPH_DOMAIN in DekiNode.h), never hardcoded in the editor.
 * The generic NodeGraphEditorWindow claims an asset type iff a domain exists
 * for it, and filters NodeTypeRegistry to node metas whose category's first
 * path segment ("Domain/MenuGroup") equals the domain's domainKey.
 * Clear() is called on package/plugin hot-reload teardown so no stale pointers
 * survive FreeLibrary.
 */

#include <string>
#include <vector>
#include <unordered_map>

#include "deki-nodegraph/NodeGraphApi.h"
#include "deki-nodegraph/NodeGraphPreview.h"

/**
 * @brief One node-graph domain (all strings are static storage in the owning DLL).
 */
struct DekiNodeGraphDomain {
    const char* assetTypeName;     // .asset "type" + AssetManager loader key
    const char* displayName;       // human-readable ("Hero Behavior Graph")
    const char* domainKey;         // category first-segment filter ("HeroBehavior")
    const char* entryNodeTypeName; // node type new graphs seed with ("FsmEntry")

    // APPEND-ONLY past this point, same rule as DekiNodeMeta: the editor reads
    // domains provided by package DLLs across hot reload, so a field inserted
    // mid-struct would shift offsets under a still-running editor.

    // Optional live preview (see NodeGraphPreview.h). All-null = no Preview
    // panel for this domain, which is the default for a domain that has
    // nothing meaningful to show while you edit it.
    NodeGraphPreviewOps preview{};

    // Optional per-node illustration in the properties panel (see
    // NodeGraphNodeGizmoOps). Null = the panel is fields only, as before.
    NodeGraphNodeGizmoOps gizmos{};
};

class DEKI_NODEGRAPH_API NodeGraphDomainRegistry {
public:
    static NodeGraphDomainRegistry& Instance();

    void Register(const DekiNodeGraphDomain* domain);

    const DekiNodeGraphDomain* Get(const std::string& assetTypeName) const;

    // All registered domains, in registration order.
    const std::vector<const DekiNodeGraphDomain*>& GetAll() const { return m_Domains; }

    // Remove every registered domain (hot-reload teardown).
    void Clear();

private:
    NodeGraphDomainRegistry() = default;
    NodeGraphDomainRegistry(const NodeGraphDomainRegistry&) = delete;
    NodeGraphDomainRegistry& operator=(const NodeGraphDomainRegistry&) = delete;

    std::vector<const DekiNodeGraphDomain*> m_Domains;
    std::unordered_map<std::string, const DekiNodeGraphDomain*> m_ByAssetType;
};

#endif // DEKI_EDITOR
