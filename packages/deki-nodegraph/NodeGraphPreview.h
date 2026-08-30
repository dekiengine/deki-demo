#pragma once

#ifdef DEKI_EDITOR

/**
 * @file NodeGraphPreview.h
 * @brief Optional live preview for a node-graph domain (editor-only).
 *
 * The Node Graph window knows nothing about what any domain's nodes MEAN, so
 * it cannot preview them itself. A domain that can show its graph running
 * supplies these hooks; the window then offers a Preview panel, ticks it with
 * the editor's frame delta, and hands it a rectangle to draw in.
 *
 * The graph passed to Tick is the LIVE document being edited, not the saved
 * asset, so the preview reflects an edit the moment it is made. It is a flat
 * view of one graph level (the root): the same {id, typeId, instance} + link
 * shape the runtime loader produces, so a domain's interpreter can walk either
 * with the same code.
 *
 * Drawing goes through Canvas rather than the provider calling the editor's UI
 * directly: package DLLs share the editor's ImGui context only by pointer, and
 * a preview provider has no business touching it. Coordinates are absolute
 * screen pixels and the window has already clipped to the preview rect.
 */

#include <cstdint>

struct NodeGraphPreviewNode
{
    uint32_t id = 0;
    uint32_t typeId = 0;
    void*    instance = nullptr;   // live node struct, owned by the document
};

struct NodeGraphPreviewLink
{
    uint32_t fromNode = 0;
    int32_t  fromPin = 0;
    uint32_t toNode = 0;
    int32_t  toPin = 0;
};

// One graph level, borrowed for the duration of the Tick call. Never stored.
struct NodeGraphPreviewGraph
{
    const NodeGraphPreviewNode* nodes = nullptr;
    int                         nodeCount = 0;
    const NodeGraphPreviewLink* links = nullptr;
    int                         linkCount = 0;

    // The first node of `typeId`, or nullptr. Mirrors NodeGraphData::Graph.
    const NodeGraphPreviewNode* FindFirstOfType(uint32_t typeId) const
    {
        for (int i = 0; i < nodeCount; ++i)
            if (nodes[i].typeId == typeId) return &nodes[i];
        return nullptr;
    }

    // Follow the link leaving (nodeId, fromPin). Mirrors NodeGraphData::Graph.
    const NodeGraphPreviewNode* Next(uint32_t nodeId, int32_t fromPin) const
    {
        for (int i = 0; i < linkCount; ++i)
        {
            if (links[i].fromNode != nodeId || links[i].fromPin != fromPin) continue;
            for (int n = 0; n < nodeCount; ++n)
                if (nodes[n].id == links[i].toNode) return &nodes[n];
        }
        return nullptr;
    }
};

// Pack a color for the primitives below. Byte order matches EditorUI::Rgba
// (and ImGui's IM_COL32), which is NOT the 0xRRGGBBAA a reader would guess:
// use this rather than shifting by hand.
inline uint32_t NodeGraphPreviewRgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
{
    return (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(b) << 16) |
           (static_cast<uint32_t>(g) << 8)  |  static_cast<uint32_t>(r);
}

// The drawing primitives a preview may use, bound to the window's draw list.
// Colors come from NodeGraphPreviewRgba.
struct NodeGraphPreviewCanvas
{
    void* ctx = nullptr;
    void (*circleFilled)(void* ctx, float cx, float cy, float radius, uint32_t rgba) = nullptr;
    void (*rectFilled)(void* ctx, float x0, float y0, float x1, float y1, uint32_t rgba) = nullptr;
    void (*line)(void* ctx, float x0, float y0, float x1, float y1, uint32_t rgba, float thickness) = nullptr;
};

/**
 * @brief Per-node illustration, drawn in the properties panel under the
 *        selected node's title (editor-only, optional).
 *
 * A number in a field says what a value IS; it does not say what it LOOKS like.
 * A domain that can picture one of its nodes - an emitter's shape, a ramp, a
 * gradient, a force vector - supplies these and the panel gives that node a
 * band to draw it in, refreshed as the fields underneath are edited.
 *
 * Same rules as the preview above: the instance is the LIVE node struct, and
 * drawing goes through NodeGraphPreviewCanvas rather than the editor's UI,
 * because a provider lives in another DLL. Coordinates are absolute screen
 * pixels, and the window has clipped to the band before calling.
 */
struct NodeGraphNodeGizmoOps
{
    // Height in CSS px this node wants, or 0 for "nothing to show" (which is
    // the answer for most node types). Asked every frame with the live
    // instance, so a node can size itself by its own values.
    float (*height)(uint32_t typeId, const void* instance) = nullptr;

    // Draw into (x, y, w, h), screen pixels. `dpi` is the editor's scale, for
    // line thickness and anything else measured in pixels rather than in the
    // band's own proportions.
    void  (*draw)(uint32_t typeId, const void* instance,
                  float x, float y, float w, float h, float dpi,
                  const NodeGraphPreviewCanvas& canvas) = nullptr;
};

/**
 * @brief A domain's preview implementation. All four hooks are required for
 *        the window to offer a preview; a domain that leaves them null simply
 *        has no Preview panel.
 */
struct NodeGraphPreviewOps
{
    // One preview instance per open document.
    void* (*create)() = nullptr;
    void  (*destroy)(void* preview) = nullptr;

    // Back to the starting state (the transport's Restart).
    void  (*reset)(void* preview) = nullptr;

    // Advance by dt seconds and draw. (x, y) is the top-left of the preview
    // rect in screen pixels, (w, h) its size; pixelsPerMeter converts the
    // domain's world units to that rect. dt is 0 when the transport is paused,
    // which must still draw the current state.
    void  (*tick)(void* preview, const NodeGraphPreviewGraph& graph, float dt,
                  float x, float y, float w, float h, float pixelsPerMeter,
                  const NodeGraphPreviewCanvas& canvas) = nullptr;
};

#endif // DEKI_EDITOR
