#pragma once

#ifdef DEKI_EDITOR

/**
 * @file NodeTypeRegistry.h
 * @brief Editor-only registry of node-type metadata (DekiNodeMeta) for the node
 *        canvas: the add-node menu and per-node inspector are driven by it.
 *
 * Parallel to ComponentRegistry but DELIBERATELY SEPARATE (nodes are not
 * components). Populated by each node's generated REGISTER_NODE static
 * initializer at DLL load; Clear() is called on package/plugin hot-reload
 * teardown so no stale meta pointers survive FreeLibrary.
 */

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <unordered_map>

#include "deki-nodegraph/NodeGraphApi.h"

struct DekiNodeMeta;  // full definition in deki-nodegraph/DekiNode.h

class DEKI_NODEGRAPH_API NodeTypeRegistry {
public:
    static NodeTypeRegistry& Instance();

    // `metaSize` is the REGISTERING DLL's compiled sizeof(DekiNodeMeta) — pass
    // it literally (REGISTER_NODE in DekiNode.h does), never a cached value.
    // The two sides must agree on the layout or every field past the first
    // divergence is read at the wrong offset, and a const char* pulled from
    // mid-struct is non-null garbage that sails past null checks and faults far
    // away. DekiNodeMeta is append-only, so a stale DLL still has a readable
    // PREFIX — but "older, shorter, safe" is indistinguishable from
    // "reordered, corrupt", so any disagreement is refused outright.
    //
    // Required parameter on purpose: this header only forward-declares
    // DekiNodeMeta (DekiNode.h includes it, so it cannot include back), which
    // rules out a default argument — and a caller who has to type it is a
    // caller who cannot silently skip the check.
    void Register(const DekiNodeMeta* meta, size_t metaSize);

    const DekiNodeMeta* GetMeta(uint32_t typeId) const;
    const DekiNodeMeta* GetMeta(const std::string& name) const;

    // True once Register() has refused a meta over a layout disagreement. That
    // package's node types are simply ABSENT, and the first thing anyone notices
    // is "unknown node type 'X'" when a graph asset loads, which points at the
    // asset rather than at the build. Nothing in a running process can fix it:
    // a loaded DLL's struct layout is fixed for the process lifetime, so hot
    // reload cannot bridge it and only a full editor restart will. Latched on
    // purpose (never cleared by Clear()) for exactly that reason.
    bool HasLayoutMismatch() const { return m_LayoutMismatch; }
    const std::string& LayoutMismatchDetail() const { return m_MismatchDetail; }

    // All registered node types, in registration order (for the add-node menu).
    const std::vector<const DekiNodeMeta*>& GetAllNodes() const { return m_Nodes; }

    // Remove every registered node type (hot-reload teardown).
    void Clear();

private:
    NodeTypeRegistry() = default;
    NodeTypeRegistry(const NodeTypeRegistry&) = delete;
    NodeTypeRegistry& operator=(const NodeTypeRegistry&) = delete;

    std::vector<const DekiNodeMeta*> m_Nodes;
    std::unordered_map<uint32_t, const DekiNodeMeta*> m_ByType;
    std::unordered_map<std::string, const DekiNodeMeta*> m_ByName;

    bool m_LayoutMismatch = false;
    std::string m_MismatchDetail;
};

#endif // DEKI_EDITOR
