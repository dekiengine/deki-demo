/**
 * @file ParticlePreview.cpp
 * @brief Live preview of a particle graph inside the Node Graph window.
 *
 * Runs the effect being edited, not the saved asset: the window hands over the
 * LIVE document each tick, so a value typed in the properties panel shows up
 * on the very next frame. The chain points at the document's node instances,
 * which is what makes that work without rebuilding anything.
 *
 * The chain is rebuilt only when the graph's TOPOLOGY changes (a node added,
 * deleted or rewired), tracked by a cheap signature. Rebuilding every frame
 * would reallocate the state blobs and so reset every accumulator, and an
 * emission rate that resets each frame never emits.
 *
 * Particles draw as plain dots here, not sprites: the sprite lives on
 * ParticleEmitterComponent, not in the graph, so a graph on its own has no
 * texture to show. Motion, spread, gravity, drag, size and color all read
 * correctly from dots; only the artwork is missing.
 *
 * The emitter's SHAPE is outlined under them at the same scale, so a radius is
 * shown against the spray it produces rather than as a number in a field. It
 * draws whether or not the chain builds, since wiring is exactly when it helps.
 */

#ifdef DEKI_EDITOR

#include "ParticleEmitterComponent.h"
#include "ParticleChain.h"
#include "ParticleNodes.h"

#include "deki-nodegraph/DekiNode.h"
#include "deki-nodegraph/NodeGraphPreview.h"

#include <cmath>
#include <cstdint>
#include <vector>

namespace
{

// The preview's own pool size. maxParticles is a component property, and the
// graph does not carry one, so the preview picks a ceiling generous enough
// that a burst is not silently clipped while you tune it.
constexpr int kPreviewMaxParticles = 512;

struct ParticlePreviewState
{
    ParticleEmitterComponent emitter;
    uint64_t topology = 0;      // signature of the last graph the chain was built from
    bool     built = false;
};

// Cheap signature of what the chain depends on: which nodes exist, of what
// type, and how they are wired. Property VALUES are deliberately absent — the
// chain holds pointers to the live instances, so an edited value needs no
// rebuild and must not cause one.
uint64_t TopologyOf(const NodeGraphPreviewGraph& graph)
{
    uint64_t h = 1469598103934665603ull;   // FNV-1a 64
    auto mix = [&h](uint64_t v) {
        h ^= v;
        h *= 1099511628211ull;
    };
    for (int i = 0; i < graph.nodeCount; ++i)
    {
        mix(graph.nodes[i].id);
        mix(graph.nodes[i].typeId);
        mix(reinterpret_cast<uint64_t>(graph.nodes[i].instance));
    }
    for (int i = 0; i < graph.linkCount; ++i)
    {
        mix(graph.links[i].fromNode);
        mix(static_cast<uint64_t>(graph.links[i].fromPin));
        mix(graph.links[i].toNode);
        mix(static_cast<uint64_t>(graph.links[i].toPin));
    }
    return h;
}

// Polyline ring: the canvas has a filled circle and a line, and an outline is
// the one thing a shape gizmo actually needs.
void StrokeCircle(const NodeGraphPreviewCanvas& canvas, float cx, float cy, float r,
                  uint32_t rgba, float thickness)
{
    if (r <= 0.5f)
        return;
    int seg = static_cast<int>(r * 0.9f);
    if (seg < 16) seg = 16;
    if (seg > 96) seg = 96;
    float px = cx + r, py = cy;
    for (int i = 1; i <= seg; ++i)
    {
        const float a = (DekiMath::kTwoPi * static_cast<float>(i)) / static_cast<float>(seg);
        const float qx = cx + std::cos(a) * r;
        const float qy = cy - std::sin(a) * r;
        canvas.line(canvas.ctx, px, py, qx, qy, rgba, thickness);
        px = qx;
        py = qy;
    }
}

// The emitter's SHAPE, drawn where particles are actually born and at the scale
// they move in. This is the one place a radius is a MEASURE rather than a
// proportion: the panel's px/m slider scales the outline and the particles
// together, so a 0.3 m circle looks 0.3 m next to the spray leaving it. Kept
// faint and drawn under the particles - the effect is the subject, this is the
// frame around it.
void DrawEmitterShape(const NodeGraphPreviewGraph& graph, float cx, float cy,
                      float pixelsPerMeter, const NodeGraphPreviewCanvas& canvas)
{
    const NodeGraphPreviewNode* node =
        graph.FindFirstOfType(DekiHashString(ParticleEmissionNode::StaticNodeName));
    if (!node || !node->instance)
        return;
    const auto& e = *static_cast<const ParticleEmissionNode*>(node->instance);

    const uint32_t line = NodeGraphPreviewRgba(122, 190, 255, e.enabled ? 90 : 40);
    const uint32_t mark = NodeGraphPreviewRgba(255, 255, 255, 60);

    switch (e.shape)
    {
        case EmitterShapeKind::Circle:
            StrokeCircle(canvas, cx, cy, e.radius * pixelsPerMeter, line, 1.0f);
            break;
        case EmitterShapeKind::Rect:
        {
            const float hw = e.width  * 0.5f * pixelsPerMeter;
            const float hh = e.height * 0.5f * pixelsPerMeter;
            if (hw > 0.5f || hh > 0.5f)
            {
                canvas.line(canvas.ctx, cx - hw, cy - hh, cx + hw, cy - hh, line, 1.0f);
                canvas.line(canvas.ctx, cx + hw, cy - hh, cx + hw, cy + hh, line, 1.0f);
                canvas.line(canvas.ctx, cx + hw, cy + hh, cx - hw, cy + hh, line, 1.0f);
                canvas.line(canvas.ctx, cx - hw, cy + hh, cx - hw, cy - hh, line, 1.0f);
            }
            break;
        }
        case EmitterShapeKind::Point:
        default:
            break;
    }

    // Origin, always: with a Point shape it is the whole gizmo, and with the
    // others it says which way the effect is offset from its centre.
    canvas.line(canvas.ctx, cx - 4.0f, cy, cx + 4.0f, cy, mark, 1.0f);
    canvas.line(canvas.ctx, cx, cy - 4.0f, cx, cy + 4.0f, mark, 1.0f);
}

void* PreviewCreate()
{
    auto* p = new ParticlePreviewState();
    p->emitter.maxParticles = kPreviewMaxParticles;
    // No owner object here, so world space has no origin to add: local space
    // puts the effect at the preview's centre. (EmissionEmit already guards on
    // GetOwner(), so this is belt and braces.)
    p->emitter.worldSpace = false;
    return p;
}

void PreviewDestroy(void* preview)
{
    delete static_cast<ParticlePreviewState*>(preview);
}

void PreviewReset(void* preview)
{
    auto* p = static_cast<ParticlePreviewState*>(preview);
    // Drop every live particle and force a rebuild, which re-zeroes the state
    // blobs (burst latch, rate accumulator).
    while (p->emitter.pool.AliveCount() > 0)
        p->emitter.pool.KillSwap(p->emitter.pool.AliveCount() - 1);
    p->built = false;
    p->topology = 0;
}

void PreviewTick(void* preview, const NodeGraphPreviewGraph& graph, float dt,
                 float x, float y, float w, float h, float pixelsPerMeter,
                 const NodeGraphPreviewCanvas& canvas)
{
    auto* p = static_cast<ParticlePreviewState*>(preview);

    const uint64_t topology = TopologyOf(graph);
    if (!p->built || topology != p->topology)
    {
        std::vector<ParticleChainEntry> chain;
        const char* error = nullptr;
        // A half-wired graph is the normal state while authoring, so a failed
        // build is not worth logging here: the panel simply shows nothing.
        if (BuildParticleChain(graph, DekiHashString(ParticleEmitNode::StaticNodeName),
                               chain, &error))
        {
            p->emitter.AdoptChain(std::move(chain));
            p->built = true;
        }
        else
        {
            p->built = false;
        }
        p->topology = topology;
    }

    if (p->built && dt > 0.0f)
        p->emitter.Simulate(dt);

    // Origin at the centre of the preview rect, Y up (world convention) mapped
    // to Y down (screen).
    const float cx = x + w * 0.5f;
    const float cy = y + h * 0.5f;

    // Before the early-out: a half-wired graph draws no particles, and the
    // shape is exactly what you want to see while wiring it up.
    DrawEmitterShape(graph, cx, cy, pixelsPerMeter, canvas);

    if (!p->built)
        return;

    auto& pool = p->emitter.pool;
    const int n = pool.AliveCount();
    const float baseRadius = 2.5f;

    for (int i = 0; i < n; ++i)
    {
        const float px = cx + pool.posX[i] * pixelsPerMeter;
        const float py = cy - pool.posY[i] * pixelsPerMeter;

        const float scale = pool.HasScale() ? pool.scale[i] : 1.0f;
        float radius = baseRadius * scale;
        if (radius < 0.75f) radius = 0.75f;

        // Skip what falls outside the panel. The window clips too; this just
        // saves the draw calls for an effect that flies off screen.
        if (px + radius < x || px - radius > x + w ||
            py + radius < y || py - radius > y + h)
            continue;

        uint8_t r = 255, g = 255, b = 255, a = 255;
        if (pool.HasTint())
        {
            r = pool.tintR[i]; g = pool.tintG[i];
            b = pool.tintB[i]; a = pool.tintA[i];
        }
        if (a == 0) continue;

        const uint32_t rgba = NodeGraphPreviewRgba(r, g, b, a);

        canvas.circleFilled(canvas.ctx, px, py, radius, rgba);

        // A dot cannot show spin, so rotating particles get a spoke. Without
        // it, Initial Rotation and Rotation over Lifetime would look like they
        // do nothing at all.
        if (pool.HasRotation() && radius >= 2.0f)
        {
            const float ang = pool.rotation[i];
            canvas.line(canvas.ctx, px, py,
                        px + std::cos(ang) * radius,
                        py - std::sin(ang) * radius,
                        rgba, 1.0f);
        }
    }
}

NodeGraphPreviewOps MakePreviewOps()
{
    NodeGraphPreviewOps ops;
    ops.create  = &PreviewCreate;
    ops.destroy = &PreviewDestroy;
    ops.reset   = &PreviewReset;
    ops.tick    = &PreviewTick;
    return ops;
}

} // namespace

NodeGraphPreviewOps DekiParticles_PreviewOps()
{
    static const NodeGraphPreviewOps ops = MakePreviewOps();
    return ops;
}

#endif // DEKI_EDITOR
