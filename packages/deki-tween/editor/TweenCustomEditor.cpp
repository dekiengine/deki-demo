#ifdef DEKI_EDITOR

#include <deki-editor/CustomEditor.h>
#include <deki-editor/EditorUI.h>
#include <deki-editor/EditorRegistry.h>
#include "../TweenComponent.h"
#include <cstdio>

namespace {

class TweenCustomEditor : public DekiEditor::CustomEditor
{
public:
    const char* GetComponentName() const override { return "TweenComponent"; }

    bool WantsInspectorOverride(DekiComponent* comp) override { return true; }

    void OnInspectorGUI(DekiComponent* comp) override
    {
        auto& ui = DekiEditor::EditorUI::Get();
        // Draw all default properties first
        ui.DrawDefaultInspector();

        // Show calculated total duration
        auto* tween = static_cast<TweenComponent*>(comp);
        ui.Space();
        ui.Separator();
        if (tween->loops == -1)
        {
            ui.TextDisabled("Total Duration: infinite");
        }
        else
        {
            int plays = tween->loops <= 1 ? 1 : tween->loops;
            float total = tween->delay + tween->duration * plays;
            char buf[64];
            std::snprintf(buf, sizeof(buf), "Total Duration: %.2fs", total);
            ui.TextDisabled(buf);
        }
    }
};

} // anonymous namespace

REGISTER_EDITOR(TweenCustomEditor)

#endif // DEKI_EDITOR
