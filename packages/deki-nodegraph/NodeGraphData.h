#pragma once

/**
 * @file NodeGraphData.h
 * @brief Runtime container for a compiled node-graph asset (all platforms).
 *
 * A node-graph .asset is authored as JSON by the editor's NodeGraphEditorWindow
 * and compiled to MessagePack by the generic data-asset path (json::to_msgpack
 * minus the "type" key). This class parses that MessagePack into type-erased
 * node instances (via NodeFactory) plus a link table. Interpretation of pins
 * and links is entirely up to the consuming tool's interpreter; this container
 * is domain-agnostic.
 *
 * Wire schema (keys in nlohmann's alphabetical order):
 *   { "links":      [ { "from": u32, "fromPin": i32, "to": u32, "toPin": i32 }, ... ],
 *     "nextNodeId": u32,   // editor-side id counter; preserved but unused at runtime
 *     "nodes":      [ { "children": [ { "enabled": bool, "type": "...", "values": { ... } }, ... ],
 *                       "graph": { "links": [ ... ], "nodes": [ ... ] },
 *                       "id": u32, "type": "NodeTypeName", "values": { ... },
 *                       "x": f, "y": f }, ... ] }
 *
 * Within a node map the alphabetical order children < graph < id < type <
 * values < x < y guarantees "type" is read (instance created) before "values"
 * (instance populated) in a single sequential parse; the same enabled < type <
 * values ordering holds inside each child map. A file where "values" precedes
 * "type" is corrupt and fails the load loudly.
 *
 * A node carries at most one of two kinds of contents:
 *   - "children": the optional ordered STACK of child node instances a parent
 *     owns (DEKI_NODE_CHILDREN — e.g. an FSM graph's variable declarations).
 *     Children are full NodeFactory instances but take no part in any link table.
 *   - "graph": the optional INNER GRAPH a parent owns (DEKI_NODE_SUBGRAPH — e.g.
 *     an FSM state's action flow, or a group of states). An inner graph is a
 *     Graph exactly like the root, nested to any depth, with its own links.
 *
 * Node ids are unique across the WHOLE document, not just within one graph, so
 * an id identifies one node however deep it sits. Link endpoints always name
 * nodes in the SAME graph as the link, so pin-following never leaves its graph;
 * crossing a boundary is the interpreter's job (descend into a node's inner
 * graph, or ascend out of it).
 *
 * Loading is all-or-nothing: any error destroys every already-created instance
 * and returns nullptr. No partial graphs.
 */

#include "deki-nodegraph/NodeGraphApi.h"

#include <cstdint>
#include <cstddef>
#include <vector>

class DEKI_NODEGRAPH_API NodeGraphData {
public:
    // One child in a node's ordered stack (see DEKI_NODE_CHILDREN). Children
    // are NodeFactory instances like top-level nodes but have no id and no
    // links; interpretation (e.g. "these are the variables") is the consumer's.
    struct ChildInstance {
        uint32_t typeId = 0;      // DekiHashString(child type name)
        void* instance = nullptr; // NodeFactory-created struct, populated from "values"
        bool enabled = true;      // authoring toggle; disabled children are data only
    };

    struct Graph;

    struct NodeInstance {
        uint32_t id = 0;          // document-unique node id (link endpoints)
        uint32_t typeId = 0;      // DekiHashString(node type name)
        void* instance = nullptr; // NodeFactory-created struct, populated from "values"
        // Appended last (same cross-DLL append-only rule as DekiNodeMeta): a
        // stale reader still finds id/typeId/instance at their old offsets.
        std::vector<ChildInstance> children;
        Graph* inner = nullptr;   // DEKI_NODE_SUBGRAPH contents (owned), or nullptr
    };

    struct Link {
        uint32_t fromNode = 0;
        int32_t fromPin = 0;
        uint32_t toNode = 0;
        int32_t toPin = 0;
    };

    // One graph level: the document root, or any node's inner graph. Every
    // query is scoped to this level, because every link is.
    struct DEKI_NODEGRAPH_API Graph {
        std::vector<NodeInstance> nodes;
        std::vector<Link> links;

        const NodeInstance* FindNode(uint32_t id) const;
        const NodeInstance* FindFirstOfType(uint32_t typeId) const;

        // Follow the link leaving (nodeId, fromPin). Returns the destination
        // node in THIS graph, or nullptr if no such link exists. If multiple
        // links share one output pin (not produced by the editor), the first
        // one wins.
        const NodeInstance* Next(uint32_t nodeId, int32_t fromPin) const;
    };

    // Parse a compiled node-graph MessagePack blob. Returns nullptr (after
    // DEKI_LOG_ERROR) on any structural error, unknown node type, or failed
    // node deserialization.
    static NodeGraphData* LoadFromMemory(const uint8_t* data, size_t size);

    ~NodeGraphData();

    NodeGraphData(const NodeGraphData&) = delete;
    NodeGraphData& operator=(const NodeGraphData&) = delete;

    // The top-level graph. Inner graphs are reached through NodeInstance::inner.
    const Graph& Root() const { return m_Root; }

    // Root-level shorthands (an interpreter that never descends uses only these).
    const std::vector<NodeInstance>& Nodes() const { return m_Root.nodes; }
    const std::vector<Link>& Links() const { return m_Root.links; }
    const NodeInstance* FindNode(uint32_t id) const { return m_Root.FindNode(id); }
    const NodeInstance* FindFirstOfType(uint32_t typeId) const { return m_Root.FindFirstOfType(typeId); }
    const NodeInstance* Next(uint32_t nodeId, int32_t fromPin) const { return m_Root.Next(nodeId, fromPin); }

private:
    NodeGraphData() = default;

    Graph m_Root;
};
