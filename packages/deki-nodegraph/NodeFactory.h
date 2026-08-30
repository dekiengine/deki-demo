#pragma once

/**
 * @file NodeFactory.h
 * @brief Type-erased factory for node-graph node types (create/deserialize/destroy).
 *
 * Parallel to ComponentFactory, but DELIBERATELY SEPARATE: node types are not
 * components and must not leak into the scene/ECS systems. Nodes are plain
 * reflected structs (DEKI_NODE) with no common base class, so the factory stores
 * void* create/destroy and a deserialize thunk keyed by the node type's name hash.
 *
 * Every node's generated .gen.cpp self-registers via REGISTER_RUNTIME_NODE
 * (defined in DekiNode.h) at DLL load, so new node types come from package/project
 * DLLs with no editor rebuild.
 */

#include "deki-nodegraph/NodeGraphApi.h"

#include <cstdint>
#include <cstddef>
#include <unordered_map>

namespace SceneFormat {

class SceneMsgPackParser;

using NodeCreateFn      = void* (*)();
using NodeDeserializeFn = bool  (*)(void* node, SceneMsgPackParser& parser, uint32_t mapSize);
using NodeDestroyFn     = void  (*)(void* node);

struct NodeFactoryEntry {
    NodeCreateFn      create      = nullptr;
    NodeDeserializeFn deserialize = nullptr;
    NodeDestroyFn     destroy     = nullptr;
};

/**
 * @brief Hash-map factory for node types. O(1) lookup by type id (name hash).
 */
class DEKI_NODEGRAPH_API NodeFactory {
public:
    static NodeFactory& Instance();

    void Register(uint32_t typeId, NodeCreateFn create,
                  NodeDeserializeFn deserialize, NodeDestroyFn destroy) {
        m_Entries[typeId] = { create, deserialize, destroy };
    }

    void Unregister(uint32_t typeId) { m_Entries.erase(typeId); }

    // Create a default-constructed node of the given type, or nullptr if unknown.
    void* Create(uint32_t typeId) {
        auto it = m_Entries.find(typeId);
        return (it != m_Entries.end() && it->second.create) ? it->second.create() : nullptr;
    }

    // Deserialize `node` (a pointer from Create) from a MessagePack map. Returns
    // false if the type is unknown or has no deserialize thunk.
    bool Deserialize(uint32_t typeId, void* node, SceneMsgPackParser& parser, uint32_t mapSize) {
        auto it = m_Entries.find(typeId);
        return (it != m_Entries.end() && it->second.deserialize)
            ? it->second.deserialize(node, parser, mapSize) : false;
    }

    void Destroy(uint32_t typeId, void* node) {
        auto it = m_Entries.find(typeId);
        if (it != m_Entries.end() && it->second.destroy) it->second.destroy(node);
    }

    bool IsRegistered(uint32_t typeId) const { return m_Entries.count(typeId) > 0; }
    size_t GetRegisteredCount() const { return m_Entries.size(); }

    // Clear all entries (for plugin/package hot-reload teardown).
    void Clear() { m_Entries.clear(); }

private:
    NodeFactory() = default;
    std::unordered_map<uint32_t, NodeFactoryEntry> m_Entries;
};

} // namespace SceneFormat
