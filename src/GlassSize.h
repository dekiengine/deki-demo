#pragma once

#include "CameraComponent.h"

#include <deki/Engine.h>
#include <deki/ScreenScale.h>

// How big things come out on the glass, for laying out on a physically small
// screen. Shared by the demo's layout components.
namespace GlassSize
{

// The camera of the scene `obj` is in: under its topmost ancestor, which is
// the scene's root whether or not that scene is the engine's root scene.
inline DekiRendering::CameraComponent* SceneCamera(Deki::Object* obj)
{
    while (obj->GetParent())
    {
        obj = obj->GetParent();
    }
    struct Find
    {
        static DekiRendering::CameraComponent* In(Deki::Object* o)
        {
            if (auto* camera = o->GetComponent<DekiRendering::CameraComponent>())
            {
                return camera;
            }
            for (Deki::Object* child : o->GetChildren())
            {
                if (auto* camera = In(child))
                {
                    return camera;
                }
            }
            return nullptr;
        }
    };
    return Find::In(obj);
}

// Meters of glass one meter of world covers through `camera`, or 0 when the
// screen's size is unknown (a desktop window, a board that does not say).
inline float PerWorldMeter(const DekiRendering::CameraComponent& camera)
{
    Deki::Engine& engine = Deki::Engine::GetInstance();
    const float ppi = engine.GetScreenPixelsPerInch();
    const float ppm = camera.GetPixelsPerMeter(engine.GetScreenWidth(), engine.GetScreenHeight());
    return ppi > 0.0f && ppm > 0.0f ? ppm / ppi * Deki::kMetersPerInch : 0.0f;
}

}  // namespace GlassSize
