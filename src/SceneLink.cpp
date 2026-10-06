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
    Deki::Scene* next = scene.Get();
    if (!next)
    {
        DEKI_LOG_WARNING("SceneLink on '%s' has no scene to open", GetOwner()->GetName().c_str());
        return;
    }
    Deki::Engine::GetInstance().SetRootScene(next, true);
}
