#include "SceneLink.h"

#include "ButtonComponent.h"

#include <deki/LogSystem.h>

void SceneLink::Start()
{
    auto* button = GetOwner()->GetComponent<Deki2D::ButtonComponent>();
    if (!button)
    {
        DEKI_LOG_WARNING("SceneLink on '%s' has no ButtonComponent to listen to", GetOwner()->GetName().c_str());
        return;
    }
    button->AddOnClickCallback([this]() { Open(); });
}

void SceneLink::Open()
{
    if (scene.guid.empty())
    {
        DEKI_LOG_WARNING("SceneLink on '%s' has no scene to open", GetOwner()->GetName().c_str());
        return;
    }
    // Opened after this frame, once this scene is gone: the two are never in
    // memory at once.
    Deki::Engine::GetInstance().OpenScene(scene);
}
