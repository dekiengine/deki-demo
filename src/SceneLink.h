#pragma once

#include <deki/Engine.h>
#include <deki/Scene.h>
#include <deki/assets/AssetRef.h>

// Opens a scene when the button on the same object is pressed: the demo menu's
// entries, and each demo's way back to the menu.
DEKI_CATEGORY("Demo")
DEKI_DESCRIPTION("Opens a scene when this object's button is pressed.")
class SceneLink : public Deki::Component
{
public:
    DEKI_EXPORT
    Deki::AssetRef<Deki::Scene> scene;

    SceneLink() = default;

    void Start() override;

private:
    void Open();
};
