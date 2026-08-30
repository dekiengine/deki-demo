#include "deki-nodegraph/editor/NodeGraphDocument.h"
#include "deki-nodegraph/editor/NodePropertyJson.h"

#include "deki-nodegraph/DekiNode.h"
#include "DekiLogSystem.h"

#include <algorithm>
#include <cstring>

namespace DekiEditor
{

using nlohmann::json;

namespace
{
    // Depth-first search across every graph level. Reports the node, the graph
    // holding it, and the id of the subgraph node owning that graph (0 = root).
    NodeGraphDocNode* FindIn(NodeGraphDocGraph& graph, uint32_t id, uint32_t ownerId,
                             NodeGraphDocGraph** outGraph, uint32_t* outOwner)
    {
        for (auto& n : graph.nodes)
        {
            if (n.id == id)
            {
                if (outGraph) *outGraph = &graph;
                if (outOwner) *outOwner = ownerId;
                return &n;
            }
            if (n.inner)
            {
                if (NodeGraphDocNode* hit = FindIn(*n.inner, id, n.id, outGraph, outOwner))
                    return hit;
            }
        }
        return nullptr;
    }

    void DestroyChildren(NodeGraphDocNode& node)
    {
        for (auto& c : node.children)
        {
            if (c.meta && c.meta->destroyFunc && c.instance)
                c.meta->destroyFunc(c.instance);
            c.instance = nullptr;
        }
        node.children.clear();
    }

    void DestroyGraphContents(NodeGraphDocGraph& graph);

    // Destroy one node's own instance, its child stack and its inner graph.
    void DestroyNode(NodeGraphDocNode& node)
    {
        DestroyChildren(node);
        if (node.inner)
        {
            DestroyGraphContents(*node.inner);
            delete node.inner;
            node.inner = nullptr;
        }
        if (node.meta && node.meta->destroyFunc && node.instance)
            node.meta->destroyFunc(node.instance);
        node.instance = nullptr;
    }

    void DestroyGraphContents(NodeGraphDocGraph& graph)
    {
        for (auto& n : graph.nodes)
            DestroyNode(n);
        graph.nodes.clear();
        graph.links.clear();
    }

    // Serialize one graph level ({"links", "nodes"}); recurses through
    // NodeToJson for any node that owns an inner graph.
    json GraphToJson(const NodeGraphDocument& doc, const NodeGraphDocGraph& graph)
    {
        json jnodes = json::array();
        for (const auto& n : graph.nodes)
            jnodes.push_back(doc.NodeToJson(n));

        json jlinks = json::array();
        for (const auto& l : graph.links)
        {
            json jl;
            jl["from"] = l.fromNode;
            jl["fromPin"] = l.fromPin;
            jl["to"] = l.toNode;
            jl["toPin"] = l.toPin;
            jlinks.push_back(std::move(jl));
        }

        json j;
        j["links"] = std::move(jlinks);
        j["nodes"] = std::move(jnodes);
        return j;
    }
}

// ---------------------------------------------------------------------------
// Graph lookup
// ---------------------------------------------------------------------------

NodeGraphDocGraph* NodeGraphDocument::GraphOf(uint32_t ownerNodeId)
{
    if (ownerNodeId == 0)
        return &root;
    NodeGraphDocNode* owner = FindNode(ownerNodeId);
    return owner ? owner->inner : nullptr;
}

const NodeGraphDocGraph* NodeGraphDocument::GraphOf(uint32_t ownerNodeId) const
{
    return const_cast<NodeGraphDocument*>(this)->GraphOf(ownerNodeId);
}

NodeGraphDocGraph* NodeGraphDocument::GraphContaining(uint32_t nodeId)
{
    NodeGraphDocGraph* graph = nullptr;
    return FindIn(root, nodeId, 0, &graph, nullptr) ? graph : nullptr;
}

const NodeGraphDocGraph* NodeGraphDocument::GraphContaining(uint32_t nodeId) const
{
    return const_cast<NodeGraphDocument*>(this)->GraphContaining(nodeId);
}

uint32_t NodeGraphDocument::OwnerOf(uint32_t nodeId) const
{
    uint32_t owner = 0;
    FindIn(const_cast<NodeGraphDocument*>(this)->root, nodeId, 0, nullptr, &owner);
    return owner;
}

// ---------------------------------------------------------------------------
// Node mutators
// ---------------------------------------------------------------------------

uint32_t NodeGraphDocument::AddNode(uint32_t typeId, float x, float y, uint32_t ownerNodeId)
{
    const DekiNodeMeta* meta = NodeTypeRegistry::Instance().GetMeta(typeId);
    if (!meta || !meta->createFunc)
    {
        DEKI_LOG_ERROR("NodeGraphDocument: AddNode with unregistered node type id %u", typeId);
        return 0;
    }
    NodeGraphDocGraph* graph = GraphOf(ownerNodeId);
    if (!graph)
    {
        DEKI_LOG_ERROR("NodeGraphDocument: AddNode into node %u, which owns no graph", ownerNodeId);
        return 0;
    }

    NodeGraphDocNode node;
    node.id = nextNodeId++;
    node.meta = meta;
    node.instance = meta->createFunc();
    node.x = x;
    node.y = y;
    const uint32_t newId = node.id;
    graph->nodes.push_back(node);
    dirty = true;

    // A subgraph node is born with its inner graph and its entry node, so
    // descending into a freshly added state never lands on a blank canvas.
    EnsureSubgraph(newId);
    return newId;
}

bool NodeGraphDocument::AddNodeWithId(uint32_t id, const std::string& typeName,
                                      float x, float y, const nlohmann::json& values,
                                      uint32_t ownerNodeId)
{
    const DekiNodeMeta* meta = NodeTypeRegistry::Instance().GetMeta(typeName);
    if (!meta || !meta->createFunc)
    {
        DEKI_LOG_ERROR("NodeGraphDocument: unknown node type '%s'", typeName.c_str());
        return false;
    }
    if (FindNode(id))
    {
        DEKI_LOG_ERROR("NodeGraphDocument: duplicate node id %u", id);
        return false;
    }
    NodeGraphDocGraph* graph = GraphOf(ownerNodeId);
    if (!graph)
    {
        DEKI_LOG_ERROR("NodeGraphDocument: AddNodeWithId into node %u, which owns no graph",
                       ownerNodeId);
        return false;
    }

    NodeGraphDocNode node;
    node.id = id;
    node.meta = meta;
    node.instance = meta->createFunc();
    node.x = x;
    node.y = y;
    if (!NodePropertiesFromJson(node.instance, *meta, values))
    {
        if (meta->destroyFunc) meta->destroyFunc(node.instance);
        return false;
    }

    graph->nodes.push_back(node);
    if (id >= nextNodeId)
        nextNodeId = id + 1;
    dirty = true;
    return true;
}

bool NodeGraphDocument::RemoveNode(uint32_t id)
{
    NodeGraphDocGraph* graph = GraphContaining(id);
    if (!graph)
        return false;

    auto it = std::find_if(graph->nodes.begin(), graph->nodes.end(),
                           [id](const NodeGraphDocNode& n) { return n.id == id; });
    if (it == graph->nodes.end())
        return false;

    DestroyNode(*it);
    graph->nodes.erase(it);

    graph->links.erase(std::remove_if(graph->links.begin(), graph->links.end(),
                                      [id](const NodeGraphDocLink& l)
                                      { return l.fromNode == id || l.toNode == id; }),
                       graph->links.end());
    dirty = true;
    return true;
}

bool NodeGraphDocument::AddLink(const NodeGraphDocLink& link)
{
    NodeGraphDocGraph* graph = GraphContaining(link.fromNode);
    if (!graph)
    {
        DEKI_LOG_ERROR("NodeGraphDocument: AddLink from unknown node %u", link.fromNode);
        return false;
    }
    if (std::find(graph->links.begin(), graph->links.end(), link) != graph->links.end())
        return false;
    graph->links.push_back(link);
    dirty = true;
    return true;
}

bool NodeGraphDocument::RemoveLink(const NodeGraphDocLink& link)
{
    NodeGraphDocGraph* graph = GraphContaining(link.fromNode);
    if (!graph)
        return false;
    auto it = std::find(graph->links.begin(), graph->links.end(), link);
    if (it == graph->links.end())
        return false;
    graph->links.erase(it);
    dirty = true;
    return true;
}

bool NodeGraphDocument::RemoveLinkFromPin(uint32_t fromNode, int fromPin,
                                          NodeGraphDocLink* outRemoved)
{
    NodeGraphDocGraph* graph = GraphContaining(fromNode);
    if (!graph)
        return false;
    auto it = std::find_if(graph->links.begin(), graph->links.end(),
                           [&](const NodeGraphDocLink& l)
                           { return l.fromNode == fromNode && l.fromPin == fromPin; });
    if (it == graph->links.end())
        return false;
    if (outRemoved)
        *outRemoved = *it;
    graph->links.erase(it);
    dirty = true;
    return true;
}

void NodeGraphDocument::SetNodePos(uint32_t id, float x, float y)
{
    if (NodeGraphDocNode* node = FindNode(id))
    {
        if (node->x != x || node->y != y)
        {
            node->x = x;
            node->y = y;
            dirty = true;
        }
    }
}

// ---------------------------------------------------------------------------
// Subgraphs
// ---------------------------------------------------------------------------

bool NodeGraphDocument::EnsureSubgraph(uint32_t nodeId)
{
    NodeGraphDocNode* node = FindNode(nodeId);
    if (!node || !node->meta || !node->meta->subgraphCategory)
        return false;   // not a subgraph node: nothing to ensure

    if (!node->inner)
    {
        node->inner = new NodeGraphDocGraph();
        dirty = true;
    }
    const char* entryType = node->meta->subgraphEntry;
    if (!entryType || entryType[0] == '\0')
        return true;                       // subgraph with no seeded entry
    if (!node->inner->nodes.empty())
        return true;                       // already seeded or authored

    const DekiNodeMeta* entryMeta = NodeTypeRegistry::Instance().GetMeta(std::string(entryType));
    if (!entryMeta)
    {
        DEKI_LOG_ERROR("NodeGraphDocument: node type '%s' declares subgraph entry '%s', "
                       "which is not a registered node type",
                       node->meta->name, entryType);
        return false;
    }
    // Placed a little in from the corner so the entry is visible without panning.
    AddNode(entryMeta->typeId, 60.0f, 60.0f, nodeId);
    return true;
}

nlohmann::json NodeGraphDocument::NodeToJson(const NodeGraphDocNode& node) const
{
    json values;
    if (!NodePropertiesToJson(node.instance, *node.meta, values))
        values = json::object();   // logged by NodePropertiesToJson

    json jn;
    if (!node.children.empty())
        jn["children"] = ChildrenToJson(node);
    if (node.inner)
        jn["graph"] = GraphToJson(*this, *node.inner);
    jn["id"] = node.id;
    jn["type"] = node.meta->name;
    jn["values"] = std::move(values);
    jn["x"] = node.x;
    jn["y"] = node.y;
    return jn;
}

bool NodeGraphDocument::NodeFromJson(uint32_t ownerNodeId, const nlohmann::json& jn)
{
    const uint32_t id = jn.at("id").get<uint32_t>();
    const std::string typeName = jn.at("type").get<std::string>();
    const float x = jn.value("x", 0.0f);
    const float y = jn.value("y", 0.0f);

    if (!AddNodeWithId(id, typeName, x, y, jn.value("values", json::object()), ownerNodeId))
        return false;

    if (jn.contains("children") && !ChildrenFromJson(id, jn.at("children")))
    {
        DEKI_LOG_ERROR("NodeGraphDocument: invalid child stack on node id %u", id);
        return false;
    }

    if (jn.contains("graph"))
    {
        const json& jg = jn.at("graph");
        NodeGraphDocNode* node = FindNode(id);
        if (!node)
            return false;
        if (!node->inner)
            node->inner = new NodeGraphDocGraph();

        if (jg.contains("nodes"))
        {
            for (const auto& jchild : jg.at("nodes"))
            {
                if (!NodeFromJson(id, jchild))
                {
                    DEKI_LOG_ERROR("NodeGraphDocument: invalid inner graph of node id %u", id);
                    return false;
                }
            }
        }
        if (jg.contains("links"))
        {
            NodeGraphDocGraph* inner = GraphOf(id);
            if (!inner)
                return false;
            for (const auto& jl : jg.at("links"))
            {
                NodeGraphDocLink l;
                l.fromNode = jl.at("from").get<uint32_t>();
                l.fromPin = jl.at("fromPin").get<int>();
                l.toNode = jl.at("to").get<uint32_t>();
                l.toPin = jl.at("toPin").get<int>();
                inner->links.push_back(l);
            }
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Child stack
// ---------------------------------------------------------------------------

int NodeGraphDocument::AddChild(uint32_t nodeId, uint32_t childTypeId, int index)
{
    NodeGraphDocNode* node = FindNode(nodeId);
    if (!node)
    {
        DEKI_LOG_ERROR("NodeGraphDocument: AddChild on unknown node %u", nodeId);
        return -1;
    }
    const DekiNodeMeta* meta = NodeTypeRegistry::Instance().GetMeta(childTypeId);
    if (!meta || !meta->createFunc)
    {
        DEKI_LOG_ERROR("NodeGraphDocument: AddChild with unregistered child type id %u", childTypeId);
        return -1;
    }

    NodeGraphDocChild child;
    child.meta = meta;
    child.instance = meta->createFunc();
    child.enabled = true;

    if (index < 0 || index > static_cast<int>(node->children.size()))
        index = static_cast<int>(node->children.size());
    node->children.insert(node->children.begin() + index, child);
    dirty = true;
    return index;
}

bool NodeGraphDocument::AddChildFromJson(uint32_t nodeId, int index, const std::string& typeName,
                                         const nlohmann::json& values, bool enabled)
{
    NodeGraphDocNode* node = FindNode(nodeId);
    if (!node)
    {
        DEKI_LOG_ERROR("NodeGraphDocument: AddChildFromJson on unknown node %u", nodeId);
        return false;
    }
    const DekiNodeMeta* meta = NodeTypeRegistry::Instance().GetMeta(typeName);
    if (!meta || !meta->createFunc)
    {
        DEKI_LOG_ERROR("NodeGraphDocument: unknown child type '%s'", typeName.c_str());
        return false;
    }

    NodeGraphDocChild child;
    child.meta = meta;
    child.instance = meta->createFunc();
    child.enabled = enabled;
    if (!NodePropertiesFromJson(child.instance, *meta, values))
    {
        if (meta->destroyFunc) meta->destroyFunc(child.instance);
        return false;
    }

    if (index < 0 || index > static_cast<int>(node->children.size()))
        index = static_cast<int>(node->children.size());
    node->children.insert(node->children.begin() + index, child);
    dirty = true;
    return true;
}

bool NodeGraphDocument::RemoveChild(uint32_t nodeId, int index)
{
    NodeGraphDocNode* node = FindNode(nodeId);
    if (!node || index < 0 || index >= static_cast<int>(node->children.size()))
        return false;

    NodeGraphDocChild& child = node->children[index];
    if (child.meta && child.meta->destroyFunc && child.instance)
        child.meta->destroyFunc(child.instance);
    node->children.erase(node->children.begin() + index);
    dirty = true;
    return true;
}

bool NodeGraphDocument::MoveChild(uint32_t nodeId, int from, int to)
{
    NodeGraphDocNode* node = FindNode(nodeId);
    if (!node)
        return false;
    const int count = static_cast<int>(node->children.size());
    if (from < 0 || from >= count || to < 0 || to >= count || from == to)
        return false;

    NodeGraphDocChild child = node->children[from];
    node->children.erase(node->children.begin() + from);
    node->children.insert(node->children.begin() + to, child);
    dirty = true;
    return true;
}

bool NodeGraphDocument::SetChildEnabled(uint32_t nodeId, int index, bool enabled)
{
    NodeGraphDocNode* node = FindNode(nodeId);
    if (!node || index < 0 || index >= static_cast<int>(node->children.size()))
        return false;
    if (node->children[index].enabled == enabled)
        return true;
    node->children[index].enabled = enabled;
    dirty = true;
    return true;
}

nlohmann::json NodeGraphDocument::ChildrenToJson(const NodeGraphDocNode& node) const
{
    json arr = json::array();
    for (const auto& c : node.children)
    {
        json values;
        if (!NodePropertiesToJson(c.instance, *c.meta, values))
            values = json::object();   // logged by NodePropertiesToJson

        json jc;
        jc["enabled"] = c.enabled;
        jc["type"] = c.meta->name;
        jc["values"] = std::move(values);
        arr.push_back(std::move(jc));
    }
    return arr;
}

bool NodeGraphDocument::ChildrenFromJson(uint32_t nodeId, const nlohmann::json& children)
{
    NodeGraphDocNode* node = FindNode(nodeId);
    if (!node)
        return false;

    DestroyChildren(*node);
    if (!children.is_array())
        return children.is_null();   // absent = empty stack

    for (const auto& jc : children)
    {
        const std::string typeName = jc.value("type", "");
        if (!AddChildFromJson(nodeId, -1, typeName,
                              jc.value("values", json::object()), jc.value("enabled", true)))
        {
            DEKI_LOG_ERROR("NodeGraphDocument: bad child '%s' in node %u", typeName.c_str(), nodeId);
            if (NodeGraphDocNode* n = FindNode(nodeId))
                DestroyChildren(*n);
            return false;
        }
    }
    dirty = true;
    return true;
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

NodeGraphDocNode* NodeGraphDocument::FindNode(uint32_t id)
{
    return FindIn(root, id, 0, nullptr, nullptr);
}

const NodeGraphDocNode* NodeGraphDocument::FindNode(uint32_t id) const
{
    return const_cast<NodeGraphDocument*>(this)->FindNode(id);
}

std::vector<NodeGraphDocLink> NodeGraphDocument::AttachedLinks(uint32_t nodeId) const
{
    std::vector<NodeGraphDocLink> result;
    const NodeGraphDocGraph* graph = GraphContaining(nodeId);
    if (!graph)
        return result;
    for (const auto& l : graph->links)
        if (l.fromNode == nodeId || l.toNode == nodeId)
            result.push_back(l);
    return result;
}

const NodeGraphDocLink* NodeGraphDocument::FindLinkFromPin(uint32_t fromNode, int fromPin) const
{
    const NodeGraphDocGraph* graph = GraphContaining(fromNode);
    if (!graph)
        return nullptr;
    for (const auto& l : graph->links)
        if (l.fromNode == fromNode && l.fromPin == fromPin)
            return &l;
    return nullptr;
}

int NodeGraphDocument::OutputPinCount(const NodeGraphDocNode& node) const
{
    if (!node.meta)
        return 0;
    const DekiNodeMeta& meta = *node.meta;
    if (!meta.dynamicOutputsProperty)
        return meta.outputPinCount;

    for (int i = 0; i < meta.propertyCount; ++i)
    {
        const DekiPropertyInfo& p = meta.properties[i];
        if (std::string(p.name) != meta.dynamicOutputsProperty)
            continue;

        const char* field = static_cast<const char*>(node.instance) + p.offset;
        if (p.type == DekiPropertyType::Int32)
        {
            int32_t v;
            std::memcpy(&v, field, sizeof v);
            return v > 0 ? v : 0;
        }
        if (p.type == DekiPropertyType::Array)
        {
            switch (p.elementType)
            {
                case DekiPropertyType::Float:
                    return static_cast<int>(reinterpret_cast<const std::vector<float>*>(field)->size());
                case DekiPropertyType::Int32:
                    return static_cast<int>(reinterpret_cast<const std::vector<int32_t>*>(field)->size());
                default:
                    return static_cast<int>(reinterpret_cast<const std::vector<std::string>*>(field)->size());
            }
        }
        DEKI_LOG_ERROR("NodeGraphDocument: node type '%s' dynamicOutputsProperty '%s' must be "
                       "Int32 or Array", meta.name, meta.dynamicOutputsProperty);
        return 0;
    }

    DEKI_LOG_ERROR("NodeGraphDocument: node type '%s' dynamicOutputsProperty '%s' not found",
                   meta.name, meta.dynamicOutputsProperty);
    return 0;
}

// ---------------------------------------------------------------------------
// Serialization
// ---------------------------------------------------------------------------

nlohmann::json NodeGraphDocument::ToJson() const
{
    json j = GraphToJson(*this, root);
    j["type"] = domain ? domain->assetTypeName : "";
    j["nextNodeId"] = nextNodeId;
    return j;
}

bool NodeGraphDocument::FromJson(const nlohmann::json& j, std::string& outError)
{
    DestroyInstances();
    nextNodeId = 1;

    try
    {
        if (j.contains("nodes"))
        {
            for (const auto& jn : j.at("nodes"))
            {
                if (!NodeFromJson(0, jn))
                {
                    outError = "Invalid node (id " +
                               std::to_string(jn.value("id", 0u)) + ", type '" +
                               jn.value("type", std::string()) + "')";
                    DestroyInstances();
                    return false;
                }
            }
        }

        if (j.contains("links"))
        {
            for (const auto& jl : j.at("links"))
            {
                NodeGraphDocLink l;
                l.fromNode = jl.at("from").get<uint32_t>();
                l.fromPin = jl.at("fromPin").get<int>();
                l.toNode = jl.at("to").get<uint32_t>();
                l.toPin = jl.at("toPin").get<int>();
                root.links.push_back(l);
            }
        }

        if (j.contains("nextNodeId"))
        {
            const uint32_t stored = j.at("nextNodeId").get<uint32_t>();
            if (stored > nextNodeId)
                nextNodeId = stored;
        }
    }
    catch (const json::exception& e)
    {
        outError = std::string("Malformed node graph JSON: ") + e.what();
        DestroyInstances();
        return false;
    }

    dirty = false;   // freshly loaded == on-disk state
    return true;
}

void NodeGraphDocument::DestroyInstances()
{
    DestroyGraphContents(root);
}

} // namespace DekiEditor
