#pragma once

/**
 * @file NodeGraphDocument.h
 * @brief Editor-side authoring model for one open node-graph .asset.
 *
 * Owns live node instances (NodeTypeRegistry metas + createFunc) plus the link
 * table and layout. Mutators are called ONLY by the commands in
 * core/commands/NodeGraphCommands.h so every edit is undoable; every mutation
 * sets `dirty`. Commands hold the document via weak_ptr — after the window
 * closes (or hot reload destroys it) stale undo entries no-op with an error
 * log instead of touching freed node instances.
 *
 * NESTING. A document is a TREE of graphs: the root, plus one inner graph per
 * node whose type carries DEKI_NODE_SUBGRAPH (an FSM state's action flow, a
 * group of states). Node ids are unique across the WHOLE document, so a plain
 * `uint32_t nodeId` still identifies exactly one node however deep it sits and
 * commands never need to carry a path. Links, by contrast, are always local:
 * both endpoints of a link live in the same graph, so the graph a link belongs
 * to is the one containing its `fromNode`.
 *
 * Serialization is the node-graph .asset schema (see NodeGraphData.h): the
 * saved JSON is transcoded to the msgpack runtime cache by the generic
 * data-asset pipeline path unchanged.
 */

#include <nlohmann/json.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct DekiNodeMeta;
struct DekiNodeGraphDomain;

namespace DekiEditor
{

// One entry in a node's ordered child stack (see DEKI_NODE_CHILDREN — e.g. an
// FSM graph's variable declarations). Children are live instances like nodes
// but have no id, no canvas position, and no links; they are authored in the
// parent's inspector panel and serialized inside the parent's graph entry.
struct NodeGraphDocChild
{
    const DekiNodeMeta* meta = nullptr;
    void* instance = nullptr;    // meta->createFunc(); owned by the document
    bool enabled = true;
};

struct NodeGraphDocGraph;

struct NodeGraphDocNode
{
    uint32_t id = 0;
    const DekiNodeMeta* meta = nullptr;
    void* instance = nullptr;    // meta->createFunc(); owned by the document
    float x = 0.0f, y = 0.0f;    // canvas layout (graph units)
    std::vector<NodeGraphDocChild> children;   // ordered child stack (may be empty)
    // Inner graph of a DEKI_NODE_SUBGRAPH node (owned by the document, freed by
    // DestroyInstances). nullptr for an ordinary flat node.
    NodeGraphDocGraph* inner = nullptr;
};

struct NodeGraphDocLink
{
    uint32_t fromNode = 0;
    int fromPin = 0;
    uint32_t toNode = 0;
    int toPin = 0;

    bool operator==(const NodeGraphDocLink& o) const
    {
        return fromNode == o.fromNode && fromPin == o.fromPin &&
               toNode == o.toNode && toPin == o.toPin;
    }
};

// One graph level: the document root, or any subgraph node's contents. Both are
// edited identically; the canvas simply draws whichever one is open.
struct NodeGraphDocGraph
{
    std::vector<NodeGraphDocNode> nodes;
    std::vector<NodeGraphDocLink> links;
};

class NodeGraphDocument : public std::enable_shared_from_this<NodeGraphDocument>
{
public:
    ~NodeGraphDocument() { DestroyInstances(); }

    std::string assetPath;
    std::string cachePath;
    const DekiNodeGraphDomain* domain = nullptr;

    NodeGraphDocGraph root;
    uint32_t nextNodeId = 1;   // document-wide: ids are unique across all levels
    bool dirty = false;

    // ---- Graph lookup ----

    // The graph OWNED BY `ownerNodeId` (0 = the document root). Returns nullptr
    // if that node does not exist or is not a subgraph node.
    NodeGraphDocGraph* GraphOf(uint32_t ownerNodeId);
    const NodeGraphDocGraph* GraphOf(uint32_t ownerNodeId) const;

    // The graph that CONTAINS `nodeId` (the one holding it in `nodes`).
    NodeGraphDocGraph* GraphContaining(uint32_t nodeId);
    const NodeGraphDocGraph* GraphContaining(uint32_t nodeId) const;

    // Id of the subgraph node containing `nodeId`, or 0 when it sits at the
    // root (also 0 when `nodeId` is unknown — check with FindNode first).
    uint32_t OwnerOf(uint32_t nodeId) const;

    // ---- Mutators (commands only; every one sets dirty) ----

    // Create a node of `typeId` at (x, y) inside `ownerNodeId`'s graph (0 =
    // root). Returns the new id, or 0 if the type is not registered or the
    // owner has no inner graph (logged). A node that is itself a subgraph gets
    // its inner graph created and its declared entry node seeded.
    uint32_t AddNode(uint32_t typeId, float x, float y, uint32_t ownerNodeId = 0);

    // Undo/redo restore: recreate a node with a specific id and values in
    // `ownerNodeId`'s graph. Bumps nextNodeId past `id` if needed. Returns
    // false on unknown type or bad values (logged). Does NOT seed a subgraph
    // entry: the caller restores the exact inner graph via NodeFromJson.
    bool AddNodeWithId(uint32_t id, const std::string& typeName, float x, float y,
                       const nlohmann::json& values, uint32_t ownerNodeId = 0);

    // Remove a node and every link touching it in ITS graph. An inner graph is
    // destroyed with it (recursively). Returns false if absent.
    bool RemoveNode(uint32_t id);

    bool AddLink(const NodeGraphDocLink& link);
    bool RemoveLink(const NodeGraphDocLink& link);
    // Remove any link OUT of (fromNode, fromPin); returns it via outRemoved.
    bool RemoveLinkFromPin(uint32_t fromNode, int fromPin, NodeGraphDocLink* outRemoved);

    void SetNodePos(uint32_t id, float x, float y);

    // ---- Subgraph ----

    // Give `nodeId` an inner graph if its type declares one, seeding the entry
    // node named by meta->subgraphEntry when the graph is empty. Idempotent;
    // called on add, on load, and when the canvas descends into a node.
    bool EnsureSubgraph(uint32_t nodeId);

    // One node as a complete SUBTREE (values + child stack + inner graph and
    // its links), i.e. everything an undo entry must restore to bring a
    // deleted subgraph node back exactly as it was.
    nlohmann::json NodeToJson(const NodeGraphDocNode& node) const;

    // Recreate a node subtree produced by NodeToJson inside `ownerNodeId`'s
    // graph. Hard-fails (logged) on unknown types or bad values.
    bool NodeFromJson(uint32_t ownerNodeId, const nlohmann::json& jn);

    // ---- Child stack mutators (commands only; see NodeGraphDocChild) ----

    // Append/insert a default child of `childTypeId` at `index` (clamped;
    // -1 = append). Returns the actual index, or -1 on unknown type / node.
    int AddChild(uint32_t nodeId, uint32_t childTypeId, int index);

    // Undo/redo restore: recreate one child with explicit values + enabled.
    bool AddChildFromJson(uint32_t nodeId, int index, const std::string& typeName,
                          const nlohmann::json& values, bool enabled);

    bool RemoveChild(uint32_t nodeId, int index);
    bool MoveChild(uint32_t nodeId, int from, int to);
    bool SetChildEnabled(uint32_t nodeId, int index, bool enabled);

    // Full child stack as the serialized "children" array (undo capture /
    // ToJson). Empty array when the node has no children.
    nlohmann::json ChildrenToJson(const NodeGraphDocNode& node) const;

    // Replace a node's child stack from a "children" array (undo restore).
    // Hard-fails (logged, stack left empty) on unknown types or bad values.
    bool ChildrenFromJson(uint32_t nodeId, const nlohmann::json& children);

    // ---- Queries ----

    // Recursive across every graph level: one id, one node, anywhere.
    NodeGraphDocNode* FindNode(uint32_t id);
    const NodeGraphDocNode* FindNode(uint32_t id) const;

    // Links touching `nodeId` within the graph that contains it.
    std::vector<NodeGraphDocLink> AttachedLinks(uint32_t nodeId) const;
    const NodeGraphDocLink* FindLinkFromPin(uint32_t fromNode, int fromPin) const;

    // Output pin count for a node, resolving meta->dynamicOutputsProperty off
    // the live instance (Int32 property = value, Array property = size).
    int OutputPinCount(const NodeGraphDocNode& node) const;

    // ---- Serialization ----

    // Full .asset body, including "type" (the domain's asset type name).
    nlohmann::json ToJson() const;

    // Rebuild from a .asset body. Hard-fails (outError set, document emptied)
    // on unknown node types or malformed values — never a partial load.
    bool FromJson(const nlohmann::json& j, std::string& outError);

    // Destroy every node instance and inner graph (idempotent). The window's
    // destructor runs this while the owning DLLs are still loaded
    // (ToolHost::ClearWindows).
    void DestroyInstances();
};

} // namespace DekiEditor
