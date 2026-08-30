#pragma once

#include <deki-editor/EditorWindow.h>
#include <deki-editor/NodeCanvas.h>

#include "deki-nodegraph/editor/NodeGraphDocument.h"

#include "reflection/DekiProperty.h"       // DekiPropertyType (by value below)

#include <nlohmann/json.hpp>

#include <functional>
#include <memory>
#include <string>
#include <vector>

struct DekiNodeMeta;

namespace DekiEditor
{

/**
 * @brief Generic node-graph editor window (Tools > Node Graph).
 *
 * Lives in deki-nodegraph.dll together with the node registries it drives, so
 * editing the canvas never rebuilds the engine or the editor. Undo
 * (CommandHistory) and the schematic theme come from deki-editor.dll; the
 * NodeCanvas view widget stays there too.
 *
 * Claims any .asset whose JSON "type" is a registered node-graph domain
 * (NodeGraphDomainRegistry); the add-node menu is scoped to node types whose
 * category's first path segment matches the domain. The window itself knows
 * nothing about any specific tool's node set.
 */
class NodeGraphEditorWindow : public EditorWindow
{
public:
    const char* GetTitle() override { return "Node Graph"; }
    const char* GetMenuPath() override { return "Tools/Node Graph"; }

    ~NodeGraphEditorWindow() override;

    void OnGUI() override;

    bool CanOpenAssetType(const char* assetType) override;
    void OpenFile(const char* filePath, const char* cachePath) override;

    // EditorWindow session hooks: survive hot reload (node instances die with
    // the DLLs; state crosses the reload as plain JSON).
    bool SaveSession(std::string& outJson) override;
    void RestoreSession(const std::string& json) override;

private:
    // Document lifecycle.
    bool LoadDocument(const std::string& filePath, const std::string& cachePath,
                      std::string& outError);
    void SaveDocument();
    void CloseDocument();

    // OnGUI pieces.
    void DrawToolbar();
    void DrawBreadcrumb();
    void DrawCanvas(float width, float height);
    void DrawPropertiesPanel(float width, float height);
    void DrawAddNodeMenu();
    void DrawModals();

    // ---- Live preview (NodeGraphPreview.h) ----
    // Only drawn for a domain that supplies preview ops; the window itself has
    // no idea what any graph does. The instance belongs to the domain's DLL, so
    // it must die before that DLL can go away: CloseDocument and SaveSession
    // (the pre-hot-reload hook) both destroy it.
    //
    // Drawn as an overlay pinned to the bottom-left of the canvas, over the
    // graph rather than beside it, so the effect and the nodes producing it are
    // in one field of view. Submitted after the canvas so its controls take
    // hover priority over the canvas's pan/select surface.
    void DrawPreviewOverlay(float canvasX, float canvasY, float canvasW, float canvasH);
    void DestroyPreview();

    // ---- Per-node gizmo (NodeGraphNodeGizmoOps) ----
    // The picture of the selected node - a shape, a ramp, a gradient - drawn
    // between its title and its fields, for a domain that supplies gizmo ops.
    // Silent (draws nothing, takes no space) for a node that has none, which is
    // the normal case.
    void DrawNodeGizmo(const NodeGraphDocNode& node);
    void HandleCanvasEvents(const NodeCanvasEvents& events);
    void DeleteSelection();

    // ---- Nesting (DEKI_NODE_SUBGRAPH) ----
    // The graph currently on the canvas: the root, or the inner graph of the
    // last node in m_GraphPath. Never null while a document is open.
    NodeGraphDocGraph& OpenGraph();
    const NodeGraphDocGraph& OpenGraph() const;
    // Id of the node owning the open graph (0 = root), i.e. where new nodes go.
    uint32_t OpenGraphOwner() const { return m_GraphPath.empty() ? 0u : m_GraphPath.back(); }
    // Descend into a subgraph node (no-op for a node without one), or pop back
    // out to `depth` levels of nesting (0 = root). Both reset selection and
    // reframe the canvas on the graph they land in.
    void EnterSubgraph(uint32_t nodeId);
    void NavigateToDepth(size_t depth);
    // Drop any trailing path entries whose node no longer exists (undo of an
    // add, a delete while inside, hot reload). Keeps the canvas on a real graph.
    void ValidateGraphPath();

    // The selected node's verbs (Open its subgraph / Delete it) as a toolbar
    // strip. Returns true when it acted on something that leaves the node
    // invalid, i.e. the caller must stop drawing the panel this frame.
    bool DrawNodeActionsToolbar(NodeGraphDocNode& node);

    // The node's title block: its title property as the heading with the type
    // name under it, bounded by a separator. Not collapsible, and the heading
    // is a plain label until it is clicked, which turns it into the rename
    // field. `titleProp` null (no title property) heads the block with the type
    // name and nothing is editable.
    void DrawNodeHeader(NodeGraphDocNode& node, const DekiPropertyInfo* titleProp);

    // Property widgets. DrawPropertyControl is the shared value editor for any
    // reflected instance (node or child): `commit` receives (old, new) JSON and
    // pushes the right command; `editKey` uniquely identifies the control for
    // the activate/deactivate capture pattern; `selfNodeId` excludes the owning
    // node from NodeRef dropdowns.
    using CommitFn = std::function<void(const nlohmann::json&, const nlohmann::json&)>;
    void DrawPropertyControl(void* instance, const DekiNodeMeta& meta,
                             const DekiPropertyInfo& p, const std::string& editKey,
                             uint32_t selfNodeId, const CommitFn& commit);
    // Chevron button + popup listing the open scene's objects, optionally
    // filtered to those carrying `componentFilter`. Call right after the name
    // field it belongs to; `onPick` receives the chosen object NAME ("" = the
    // object the graph runs on), which is what the runtime resolves.
    void DrawObjectNamePicker(const char* componentFilter, const std::string& current,
                              const std::function<void(const std::string&)>& onPick);

    // A PropertyRef property: three labeled rows (object / component / field),
    // each a dropdown over what actually exists, so an invalid reference cannot
    // be authored. Commits the whole reference as one undo step.
    void DrawPropertyRefControl(const DekiPropertyInfo& p, void* instance,
                                const std::string& label, const std::string& editKey,
                                const CommitFn& commit);

    // A DEKI_VALUE_OF String property: the literal written to / compared with
    // whatever its PropertyRef points at, drawn typed to that field (drag for
    // numbers, checkbox for bool, dropdown for enums) and stored as canonical
    // text. Falls back to a plain text field while nothing is picked yet.
    void DrawTypedLiteralControl(const DekiPropertyInfo& p, void* instance,
                                 const DekiNodeMeta& meta, const std::string& editKey,
                                 const CommitFn& commit);
    void DrawPropertyWidget(NodeGraphDocNode& node, const DekiPropertyInfo& p);
    void DrawWeightsWidget(NodeGraphDocNode& node, const DekiPropertyInfo& p);
    // Dynamic-outputs String array (e.g. FSM transition events): rows rename in
    // place; add/remove resizes pins (ResizeDynamicOutputsCommand prunes links).
    void DrawTransitionsWidget(NodeGraphDocNode& node, const DekiPropertyInfo& p);
    // Ordered child stack (DEKI_NODE_CHILDREN): PlayMaker-style action list.
    void DrawChildStack(NodeGraphDocNode& node);

    // Canvas title: meta->titleProperty's String value when set and non-empty,
    // else the type display name.
    std::string NodeTitle(const NodeGraphDocNode& node) const;

    // One variable declared by this document (DEKI_NODE_VARIABLES): the child's
    // title property is the name, its other exported property gives the type.
    struct GraphVariable
    {
        std::string name;
        DekiPropertyType type = DekiPropertyType::Float;
    };
    // Every variable the open document declares, in stack order (empty when the
    // domain has no variables node or nothing is declared yet).
    std::vector<GraphVariable> CollectGraphVariables() const;

    // Guarantee every permanent node type of the document's domain exists
    // (fixed lifecycle nodes like an FSM's Awake/Start/Update). Runs after
    // load/restore; seeds missing ones and marks the document dirty.
    void EnsurePermanentNodes();

    std::shared_ptr<NodeGraphDocument> m_Doc;
    NodeCanvas m_Canvas;

    // Canvas size the current pan was computed for, so a resize can be absorbed
    // into it (see DrawCanvas). 0 = no previous size yet (first draw).
    float m_CanvasPannedForW = 0.0f;
    float m_CanvasPannedForH = 0.0f;

    // Selection (by id/index into the doc, never pointers). A link index is an
    // index into the OPEN graph's link vector, so it is only valid for the
    // graph the canvas is showing — navigating clears it.
    uint32_t m_SelectedNode = 0;
    int m_SelectedLink = -1;

    // Drill-down trail: node ids from the root down to the open graph's owner.
    // Empty = editing the root graph. Survives hot reload via the session JSON.
    std::vector<uint32_t> m_GraphPath;

    // Add-node context menu state (OpenPopup deferred to the parent ID scope).
    bool m_AddMenuPending = false;
    float m_AddMenuGraphX = 0.0f;
    float m_AddMenuGraphY = 0.0f;

    // Property-edit commit tracking: value captured when a widget activates.
    std::string m_EditingProperty;
    nlohmann::json m_EditingOldValue;

    // Inline rename in the title block: the node whose heading is currently a
    // field instead of a label (0 = none), and whether that field still owes
    // itself keyboard focus (it is created the frame after the click).
    uint32_t m_RenamingNode = 0;
    bool m_RenameFocusPending = false;

    // The child-stack add popup is the editor's shared picker; this tells it to
    // clear the query and take keyboard focus on the frame it opens.
    bool m_AddChildJustOpened = true;

    // Object-name picker search box (one picker is open at a time).
    char m_ObjectPickerSearch[128] = {};

    // Live preview state. m_Preview is opaque: it is created and destroyed by
    // the domain's own ops, and this window never looks inside it.
    void* m_Preview = nullptr;
    const NodeGraphPreviewOps* m_PreviewOps = nullptr;   // the ops that created it
    bool  m_PreviewPlaying = true;
    bool  m_PreviewCollapsed = false;
    float m_PreviewZoom = 48.0f;   // preview pixels per world meter

    // Unsaved-changes confirmation: what to do after Save/Discard.
    enum class ConfirmAction { None, CloseWindow, OpenPending };
    ConfirmAction m_ConfirmAction = ConfirmAction::None;
    bool m_ConfirmPopupPending = false;

    // Pending open (waiting on the dirty-check modal) + error modal.
    std::string m_PendingOpenPath;
    std::string m_PendingOpenCache;
    std::string m_ErrorText;
    bool m_ErrorPopupPending = false;
};

} // namespace DekiEditor
