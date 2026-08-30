/**
 * @file ScrollCustomEditor.cpp
 * @brief Editor support for ScrollComponent
 *
 * Handles all edit-mode structural fixups: ensures Clip exists, and in
 * Template mode shows only the Template child (instantiated from item_scene
 * on demand) so the user sees exactly the item they are authoring. The
 * runtime-only Slot pool is stripped in edit mode and rebuilt at Play.
 */

#ifdef DEKI_EDITOR

#include <deki-editor/EditorRegistry.h>
#include <deki-editor/CustomEditor.h>
#include "ScrollComponent.h"
#include "DekiObject.h"
#include "Scene.h"

namespace DekiEditor
{

namespace
{

DekiObject* FindChildByName(DekiObject* parent, const char* name)
{
    if (!parent) return nullptr;
    for (auto* child : parent->GetChildren())
    {
        if (child->GetName() == name)
            return child;
    }
    return nullptr;
}

DekiObject* EnsureChild(DekiObject* parent, const char* name, const char* componentType)
{
    if (!parent) return nullptr;
    if (DekiObject* found = FindChildByName(parent, name)) return found;

    DekiObject* child = new DekiObject(name);
    parent->AddChild(child);
    if (componentType) child->AddComponent(componentType);
    return child;
}

DekiObject* InstantiateTemplateFromScene(DekiObject* clip, Scene* itemScene)
{
    if (!clip || !itemScene) return nullptr;

    Scene* ownerScene = clip->GetOwnerScene();
    if (!ownerScene) return nullptr;

    DekiObject* instance = itemScene->Instantiate(ownerScene);
    if (!instance) return nullptr;

    clip->AddChild(instance);
    instance->SetName("Template");
    return instance;
}

void DeleteSlotChildren(DekiObject* clip)
{
    if (!clip) return;
    auto children = clip->GetChildren(); // copy
    for (auto* child : children)
    {
        if (child->GetName().rfind("Slot", 0) == 0)
        {
            clip->RemoveChild(child);
            delete child;
        }
    }
}

void DeleteChild(DekiObject* parent, DekiObject* child)
{
    if (!parent || !child) return;
    parent->RemoveChild(child);
    delete child;
}

// The scene identity used to compare scroll->item_scene against an existing
// Template child. AssetRef carries both a runtime guid and an editor-facing
// source guid; prefer source when set, fall back to guid.
const std::string& SceneIdentity(const Deki::AssetRef<Scene>& ref)
{
    return !ref.source.empty() ? ref.source : ref.guid;
}

} // namespace

class ScrollCustomEditor : public CustomEditor
{
public:
    const char* GetComponentName() const override { return "ScrollComponent"; }

    void OnEditorUpdate(DekiComponent* comp) override
    {
        auto* scroll = static_cast<ScrollComponent*>(comp);
        if (!scroll) return;

        DekiObject* owner = scroll->GetOwner();
        if (!owner) return;

        DekiObject* clip = EnsureChild(owner, "Clip", "ClipComponent");
        if (!clip) return;

        if (scroll->mode == ScrollMode::Template)
        {
            EnsureTemplatePreview(scroll, clip);
        }
        else
        {
            SyncNonTemplate(scroll, owner, clip);
        }
    }

private:
    void EnsureTemplatePreview(ScrollComponent* scroll, DekiObject* clip)
    {
        Scene* itemScene = scroll->item_scene.Get();
        const std::string& selectedGuid = SceneIdentity(scroll->item_scene);

        DekiObject* tmpl = FindChildByName(clip, "Template");

        // If the user swapped item_scene to a different (or no) scene, the
        // existing Template is stale — rebuild it. We stamp the source scene
        // GUID onto the Template when we instantiate so later ticks (and later
        // editor sessions via ExpandInstance on load) can detect the mismatch.
        if (tmpl && tmpl->GetSourceSceneGuid() != selectedGuid)
        {
            DeleteChild(clip, tmpl);
            tmpl = nullptr;
        }

        if (!tmpl && itemScene && !selectedGuid.empty())
        {
            tmpl = InstantiateTemplateFromScene(clip, itemScene);
            if (tmpl) tmpl->SetSourceSceneGuid(selectedGuid);
        }

        if (tmpl)
        {
            tmpl->SetActive(true);
            tmpl->SetLocalPosition(0.0f, 0.0f);
        }

        DeleteSlotChildren(clip);
        m_LastChildCount = -1; // force resync if user switches back to NonTemplate
    }

    void SyncNonTemplate(ScrollComponent* scroll, DekiObject* owner, DekiObject* clip)
    {
        int childCount = static_cast<int>(clip->GetChildren().size());
        if (childCount != m_LastChildCount)
        {
            m_LastChildCount = childCount;
            scroll->SyncChildObjects(owner);
        }
    }

    int m_LastChildCount = -1;
};

REGISTER_EDITOR(ScrollCustomEditor)

} // namespace DekiEditor

#endif // DEKI_EDITOR
