# deki-nodegraph

The node-graph feature for Deki Engine: the runtime container that loads a
compiled graph asset, and the editor that authors one. Nothing in here knows
about any particular graph — a consuming package declares its own node types and
its own graph domain, and this package draws and runs them.

`deki-fsm` is the first consumer; a project can add node types of its own the
same way.

## What a consumer provides

A node type is a plain reflected struct marked `DEKI_NODE` with `DEKI_EXPORT`
fields. The reflection codegen emits its msgpack deserializer, its editor
metadata, and the self-registration that puts it in the registries at DLL load
— so new node types never rebuild the editor.

```cpp
#include "deki-nodegraph/DekiNode.h"

DEKI_NODE("MyGraph/Flow")           // category: "<DomainKey>/<MenuGroup>"
struct MyWaitNode {
    DEKI_NODE_NAME("MyWait")
    DEKI_NODE_OUTPUTS("Done")
    DEKI_EXPORT float seconds = 1.0f;
};
```

A **domain** ties an `.asset` type to the node categories that may appear in
it, and names the node a fresh graph is seeded with. Register it from an
editor-only translation unit in the owning DLL:

```cpp
REGISTER_NODE_GRAPH_DOMAIN(g_MyDomain,
                           "MyGraph",        // .asset "type" + loader key
                           "My Graph",       // human-readable
                           "MyGraph",        // category first-segment filter
                           "MyStart");       // seed node type
```

The generic Node Graph window claims any `.asset` whose `type` matches a
registered domain and scopes its add-node menu to that domain's categories.

## Nesting: nodes that contain graphs

A node type can declare that it **contains an inner graph** rather than being a
leaf. Double-clicking such a node on the canvas descends into its graph; a
breadcrumb walks back out.

```cpp
DEKI_NODE_SUBGRAPH("MyGraph/Steps",   // category the inner add-node menu offers
                   "MyStepEntry")     // node auto-seeded inside as the entry point
```

- An inner graph is a **full graph**: its own nodes, links and pin semantics,
  nested to any depth, interpreted by the consumer exactly like the root.
- Node ids are unique across the **whole document**, so an id identifies one
  node however deep it sits and the undo commands never carry a path.
- Links are always **local**: both endpoints of a link live in the same graph.
  Crossing a boundary is the consumer's job — descend into a node's inner graph,
  or ascend out of it. (`deki-fsm` does both: a State's inner graph is its
  action flow, and a Group's is a sub-flow entered and left through tunnel
  nodes.)
- The entry type is seeded per instance, never at the root, and a type used as
  someone's `subgraphEntry` is hidden from the add menu and cannot be deleted
  when it is also marked `DEKI_NODE_PERMANENT`.
- A category used as a subgraph category **different from its owner's own**
  category is treated as inner-only and never offered on the root canvas. A node
  whose contents are the same kind as itself (a group of groups) stays available
  at both levels.

Mutually exclusive with `DEKI_NODE_CHILDREN`: a node either stacks children in
its inspector or owns a graph, never both.

## Layout

| Path | Build | Role |
| --- | --- | --- |
| `DekiNode.h` | all platforms | `DekiNodeMeta` + the registration macros |
| `NodeFactory.h/.cpp` | all platforms | type-erased create / deserialize / destroy by type id |
| `NodeGraphData.h/.cpp` | all platforms | parses a compiled graph into node instances + a link table |
| `NodeRef.h` | all platforms | reference to another node in the same graph |
| `NodeTypeRegistry.*` | editor | node metadata for the add menu and inspector |
| `NodeGraphDomainRegistry.*` | editor | which `.asset` types are graphs |
| `editor/NodeGraphDocument.*` | editor | the in-memory authoring document |
| `editor/NodePropertyJson.*` | editor | reflected value <-> JSON, for the document and undo |
| `editor/NodeGraphCommands.h` | editor | undoable edits (add/move/delete/set/resize) |
| `editor/NodeGraphEditorWindow.*` | editor | the Tools > Node Graph window |

## Runtime

`NodeGraphData::LoadFromMemory` parses the compiled MessagePack into
`NodeFactory`-created node instances plus the link table, recursing through any
inner graphs, and loading is all-or-nothing — any structural error, unknown node
type or failed node deserialize destroys everything already created and returns
`nullptr`. `Root()` is the top-level `Graph`; a subgraph node's contents hang
off `NodeInstance::inner`, and every query (`FindNode`, `FindFirstOfType`,
`Next`) is scoped to one `Graph` because every link is. Interpreting pins and
links is the consumer's job: `deki-fsm` walks them as a state machine, another
tool could walk the same data as a dialogue tree.

## Editor dependencies

The window is built entirely from `EditorUI` and `EditorTheme` (both live in
`deki-editor.dll`), makes no ImGui calls of its own, and pushes its edits onto
the editor's shared `CommandHistory` so node-graph undo interleaves with the
rest of the editor. It survives hot reload through
`EditorWindow::SaveSession` / `RestoreSession`.
