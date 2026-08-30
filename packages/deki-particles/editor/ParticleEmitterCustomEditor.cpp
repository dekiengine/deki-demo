/**
 * @file ParticleEmitterCustomEditor.cpp
 * @brief Inspector override for ParticleEmitterComponent.
 *
 *  - Renders a preview transport (play / pause / step / restart) plus a speed
 *    slider that drives editor-mode simulation via OnEditorUpdate.
 *  - Draws the emission shape as a gizmo when the object is selected.
 *
 * The modifiers are nodes of the assigned ParticleGraph asset, edited in the
 * Node Graph window (double-click the asset), so there is no package list here.
 * Restart rebuilds the chain from the graph, which is how a graph edit reaches
 * a running preview.
 */

#ifdef DEKI_EDITOR

#include <deki-editor/EditorRegistry.h>
#include <deki-editor/CustomEditor.h>
#include <deki-editor/EditorUI.h>
#include <deki-editor/SceneView.h>
#include "ParticleEmitterComponent.h"
#include "ParticleNodes.h"
#include "DekiObject.h"
// ImGui is provided transitively by <deki-editor/CustomEditor.h>
#include <unordered_map>
#include <cstdio>
#include <chrono>

// Lucide icons (UTF-8 bytes from deki-editor's IconsLucide.h — inlined here so
// the package doesn't depend on an editor-private header). Glyphs are merged
// into the default ImGui font, so we just embed the bytes in button labels.
#define LUCIDE_PLAY        "\xee\x84\xbc"
#define LUCIDE_PAUSE       "\xee\x84\xae"
#define LUCIDE_STEP_FWD    "\xee\x8f\xaa"
#define LUCIDE_ROTATE_CCW  "\xee\x85\x88"

namespace DekiEditor
{

namespace
{
    // The shape the gizmo draws lives in the graph's first Emission node.
    // Reading it here rather than caching keeps the gizmo honest while the
    // graph is being edited in the other window.
    const ParticleEmissionNode* FindEmission(ParticleEmitterComponent* emitter)
    {
        ParticleGraph* g = emitter->graph.Get();
        if (!g || !g->data) return nullptr;
        const NodeGraphData::NodeInstance* node = g->data->Root().FindFirstOfType(
            DekiHashString(ParticleEmissionNode::StaticNodeName));
        if (!node || !node->instance) return nullptr;
        return static_cast<const ParticleEmissionNode*>(node->instance);
    }
}

class ParticleEmitterCustomEditor : public CustomEditor
{
public:
    const char* GetComponentName() const override { return "ParticleEmitterComponent"; }

    bool WantsInspectorOverride(DekiComponent* /*comp*/) override { return true; }

    void OnEditorUpdate(DekiComponent* comp) override
    {
        auto* emitter = static_cast<ParticleEmitterComponent*>(comp);
        if (!emitter) return;

        // Measure the editor frame delta locally — DekiTime::GetDeltaTimeF()
        // only ticks in Play mode, so in edit mode it always reads 0 and the
        // sim's dt > 0 guard would no-op forever.
        auto now = std::chrono::steady_clock::now();
        auto& last = m_LastTick[emitter];
        float dtSeconds = 0.0f;
        if (last.time_since_epoch().count() != 0)
        {
            using fsec = std::chrono::duration<float>;
            dtSeconds = std::chrono::duration_cast<fsec>(now - last).count();
        }
        last = now;

        // Single-frame Step request fires once even when paused.
        if (m_StepRequested.count(emitter))
        {
            m_StepRequested.erase(emitter);
            emitter->Simulate(1.0f / 60.0f);
        }
        if (!emitter->IsEditorPreviewPlaying()) return;
        emitter->Simulate(dtSeconds * GetSpeed(emitter));
    }

    void OnInspectorGUI(DekiComponent* comp) override
    {
        auto* emitter = static_cast<ParticleEmitterComponent*>(comp);
        if (!emitter) return;

        EditorUI::Get().DrawDefaultInspector();
        DrawPreviewSection(emitter);
    }

    // -------------------------------------------------------------------
    // Gizmo: visualize the emission shape in the scene view.
    // OnDrawGizmosSelected fires only when the emitter's owning object is
    // selected — same convention as Unity's "show shape only when selected".
    // -------------------------------------------------------------------
    void OnDrawGizmosSelected(DekiComponent* comp) override
    {
        auto* emitter = static_cast<ParticleEmitterComponent*>(comp);
        if (!emitter || !emitter->GetOwner()) return;

        const ParticleEmissionNode* em = FindEmission(emitter);
        if (!em) return;

        auto& view = SceneView::Get();

        const float cx = view.GetScreenX();
        const float cy = view.GetScreenY();
        // Crosshair: fixed-screen accent — scales with editor wheel zoom only.
        // Shape extents (radius/width/height): world meters — scale by
        // GetWorldToScreenScale so they match the rendered emission area.
        const float zoom      = view.GetZoom();
        const float worldToPx = view.GetWorldToScreenScale();
        const uint32_t color = SceneView::Rgba(255, 200, 60, 220);

        switch (em->shape)
        {
            case EmitterShapeKind::Point:
            {
                const float k = 6.0f * zoom;
                view.DrawLine(cx - k, cy, cx + k, cy, color, 1.0f);
                view.DrawLine(cx, cy - k, cx, cy + k, color, 1.0f);
                break;
            }
            case EmitterShapeKind::Circle:
            {
                const float r = em->radius * worldToPx;
                if (r > 0.5f)
                    view.DrawCircle(cx, cy, r, color, 1.0f);
                break;
            }
            case EmitterShapeKind::Rect:
            {
                const float halfW = 0.5f * em->width  * worldToPx;
                const float halfH = 0.5f * em->height * worldToPx;
                if (halfW > 0.5f && halfH > 0.5f)
                    view.DrawRect(cx - halfW, cy - halfH, cx + halfW, cy + halfH, color, 1.0f);
                break;
            }
        }
    }

private:
    struct EditorState { float speed = 1.0f; };
    std::unordered_map<ParticleEmitterComponent*, EditorState> m_State;
    std::unordered_map<ParticleEmitterComponent*, bool>        m_StepRequested;
    std::unordered_map<ParticleEmitterComponent*, std::chrono::steady_clock::time_point> m_LastTick;

    float GetSpeed(ParticleEmitterComponent* e)
    {
        auto it = m_State.find(e);
        return (it == m_State.end()) ? 1.0f : it->second.speed;
    }

    void DrawPreviewSection(ParticleEmitterComponent* emitter)
    {
        auto& ui = EditorUI::Get();
        ui.Space();
        ui.Separator();
        ui.TextDisabled("Preview");
        ui.Space();

        // An emitter with no graph has no chain and will never spawn. Say so
        // here rather than leaving the user staring at a transport that does
        // nothing.
        if (!emitter->graph.Get())
        {
            ui.TextDisabled("Assign a Particle Effect graph to see particles.");
            return;
        }

        // Auto-width buttons so the larger Lucide glyphs fit without truncation.
        const bool playing = emitter->IsEditorPreviewPlaying();
        const char* playLabel = playing ? LUCIDE_PAUSE " Pause" : LUCIDE_PLAY " Play";
        if (ui.Button(playLabel))
            emitter->EditorPreviewSetPlaying(!playing);
        ui.SameLine();
        if (ui.Button(LUCIDE_STEP_FWD " Step"))
            m_StepRequested[emitter] = true;
        ui.SameLine();
        // Restart also rebuilds the chain, so it is how a reimported graph
        // reaches this preview.
        if (ui.Button(LUCIDE_ROTATE_CCW " Restart"))
            emitter->EditorPreviewRestart();
        ui.SameLine();
        ui.AlignTextToFramePadding();
        char aliveBuf[64];
        std::snprintf(aliveBuf, sizeof(aliveBuf), "%d / %d alive",
                      emitter->pool.AliveCount(), emitter->pool.Capacity());
        ui.TextDisabled(aliveBuf);

        // Editor-only preview speed (not document state) -> low-level, no undo.
        float& speed = m_State[emitter].speed;
        ui.PropertyRow("Speed");
        ui.SliderFloat("##preview_speed", &speed, 0.0f, 4.0f, "%.2fx");
    }
};

REGISTER_EDITOR(ParticleEmitterCustomEditor)

} // namespace DekiEditor

#endif // DEKI_EDITOR
