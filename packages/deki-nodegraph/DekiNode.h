#pragma once

/**
 * @file DekiNode.h
 * @brief Node-type metadata (DekiNodeMeta) + self-registration macros for the
 *        standalone node-graph tool. Independent of components/scenes.
 *
 * A node type is a plain reflected struct marked DEKI_NODE (see DekiProperty.h)
 * with DEKI_EXPORT fields. The codegen emits, per node:
 *   - a runtime `bool DeserializeMsgPack(T&, SceneMsgPackParser&, uint32_t)` (all platforms)
 *   - editor-only GetProperties()/GetPropertyCount()/GetNodeMeta()
 *   - REGISTER_RUNTIME_NODE(T) + (editor) REGISTER_NODE(T), inline in T.gen.cpp
 * so new node types self-register from their owning DLL with no editor rebuild.
 */

#include "reflection/DekiProperty.h"
#include "deki-nodegraph/NodeFactory.h"
#include "DekiComponent.h"   // DekiHashString/DekiHashStringLen: used by the
                                // registration macros below AND by every
                                // generated node .gen.cpp — this header must be
                                // self-contained (package DLLs have no PCH that
                                // would supply it transitively)
#ifdef DEKI_EDITOR
#include "deki-nodegraph/NodeTypeRegistry.h"
#include "deki-nodegraph/NodeGraphDomainRegistry.h"
#endif

/**
 * @brief Static metadata for one node type (editor-only; drives the node canvas).
 */
struct DekiNodeMeta {
    const char* name;                    // node type name (StaticNodeName); stable id
    const char* category;                // "Domain/MenuGroup" (StaticNodeCategory)
    uint32_t typeId;                     // DekiHashString(name)
    const DekiPropertyInfo* properties;  // reflected fields (from GetProperties())
    int propertyCount;
    const char* const* inputPins;        // input pin labels (nullptr = no inputs)
    int inputPinCount;
    const char* const* outputPins;       // output pin labels (nullptr = no fixed outputs)
    int outputPinCount;
    const char* dynamicOutputsProperty;  // DEKI_EXPORT property driving output count
                                         // (nullptr = fixed outputs)
    void* (*createFunc)();               // new T()
    void (*destroyFunc)(void*);          // delete (T*)p
    size_t instanceSize;                 // sizeof(T)
    // APPEND-ONLY past this point. The editor reads plugin-provided metas across
    // hot reload; inserting a field mid-struct shifts offsets and crashes the
    // still-running (not-yet-rebuilt) editor. New fields go at the end so a stale
    // editor reads the fields it knows at correct offsets and ignores the rest
    // (same rule as DekiPropertyInfo::physicalUnit trailing its struct).
    const char* displayName;             // editor label (StaticNodeDisplayName), or
                                         // nullptr to derive from name
    const char* childCategory = nullptr; // full category of child node types this node
                                         // stacks (StaticNodeChildCategory), or nullptr
    const char* titleProperty = nullptr; // String property used as the canvas title
                                         // (StaticNodeTitleProperty), or nullptr
    bool permanent = false;              // fixed part of every graph of its domain
                                         // (StaticNodePermanent): auto-seeded, hidden
                                         // from the add menu, not deletable
    bool declaresVariables = false;      // DEKI_NODE_VARIABLES: this node's child stack
                                         // declares the graph's variables. Each child's
                                         // titleProperty is the variable NAME and its
                                         // other exported property is the initial VALUE,
                                         // whose reflected type is the variable's type.
    const char* subgraphCategory = nullptr;  // DEKI_NODE_SUBGRAPH: this node owns an INNER
                                             // GRAPH (double-click to descend) whose add-node
                                             // menu offers this category. nullptr = flat node.
    const char* subgraphEntry = nullptr;     // StaticNodeName of the node seeded inside every
                                             // instance's inner graph as its entry point
                                             // (nullptr/"" = seed nothing).
    const char* description = nullptr;       // one line on what this node DOES
                                             // (StaticNodeDescription). Shown under the
                                             // name in the add picker, where a list of
                                             // near-identical names is otherwise a guess.
};

// ---------------------------------------------------------------------------
// Self-registration (emitted inline in each node's .gen.cpp).
// ---------------------------------------------------------------------------

// Runtime (all platforms): register create/deserialize/destroy in NodeFactory.
// DeserializeMsgPack(T&, ...) is the generated free function (declared in T.gen.h).
#define REGISTER_RUNTIME_NODE(ClassName) \
    static struct ClassName##_NodeFactoryRegistrar { \
        ClassName##_NodeFactoryRegistrar() { \
            SceneFormat::NodeFactory::Instance().Register( \
                DekiHashString(ClassName::StaticNodeName), \
                []() -> void* { return new ClassName(); }, \
                [](void* p, SceneFormat::SceneMsgPackParser& parser, uint32_t mapSize) -> bool { \
                    return DeserializeMsgPack(*static_cast<ClassName*>(p), parser, mapSize); }, \
                [](void* p) { delete static_cast<ClassName*>(p); }); \
        } \
    } s_##ClassName##_NodeFactoryRegistrar

// Editor: register metadata in NodeTypeRegistry (add-node menu + inspector).
#ifdef DEKI_EDITOR
    #define REGISTER_NODE(ClassName) \
        static struct ClassName##_NodeRegistrar { \
            ClassName##_NodeRegistrar() { \
                NodeTypeRegistry::Instance().Register(&ClassName::GetNodeMeta(), \
                                                     sizeof(DekiNodeMeta)); \
            } \
        } s_##ClassName##_NodeRegistrar
#else
    #define REGISTER_NODE(ClassName) /* editor-only */
#endif

// Editor: register a node-graph domain from the owning package/project DLL.
// Declares which .asset type is a node graph, which category domain scopes its
// node set, and which node type new graphs are seeded with. Place at file scope
// in an editor-only translation unit (hand-written, not generated).
#ifdef DEKI_EDITOR
    #define REGISTER_NODE_GRAPH_DOMAIN(VarName, AssetType, Display, DomainKey, EntryType) \
        static const DekiNodeGraphDomain VarName{AssetType, Display, DomainKey, EntryType}; \
        static struct VarName##_DomainRegistrar { \
            VarName##_DomainRegistrar() { \
                NodeGraphDomainRegistry::Instance().Register(&VarName); \
            } \
        } s_##VarName##_DomainRegistrar

    // Same, plus a live preview (NodeGraphPreviewOps) so the Node Graph window
    // offers a Preview panel for this domain. `PreviewOps` is any expression
    // yielding a NodeGraphPreviewOps.
    #define REGISTER_NODE_GRAPH_DOMAIN_PREVIEW(VarName, AssetType, Display, DomainKey, EntryType, PreviewOps) \
        static const DekiNodeGraphDomain VarName{AssetType, Display, DomainKey, EntryType, PreviewOps}; \
        static struct VarName##_DomainRegistrar { \
            VarName##_DomainRegistrar() { \
                NodeGraphDomainRegistry::Instance().Register(&VarName); \
            } \
        } s_##VarName##_DomainRegistrar

    // Same again, plus per-node gizmos (NodeGraphNodeGizmoOps) so the
    // properties panel can illustrate the selected node.
    #define REGISTER_NODE_GRAPH_DOMAIN_PREVIEW_GIZMOS(VarName, AssetType, Display, DomainKey, EntryType, PreviewOps, GizmoOps) \
        static const DekiNodeGraphDomain VarName{AssetType, Display, DomainKey, EntryType, PreviewOps, GizmoOps}; \
        static struct VarName##_DomainRegistrar { \
            VarName##_DomainRegistrar() { \
                NodeGraphDomainRegistry::Instance().Register(&VarName); \
            } \
        } s_##VarName##_DomainRegistrar
#else
    #define REGISTER_NODE_GRAPH_DOMAIN(VarName, AssetType, Display, DomainKey, EntryType) /* editor-only */
    #define REGISTER_NODE_GRAPH_DOMAIN_PREVIEW(VarName, AssetType, Display, DomainKey, EntryType, PreviewOps) /* editor-only */
#endif
