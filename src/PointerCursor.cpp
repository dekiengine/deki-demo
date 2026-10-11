#include "PointerCursor.h"

#include "CameraComponent.h"
#include "DekiInput.h"

#include <cstring>

using namespace DekiInput;

namespace
{
DekiRendering::CameraComponent* FindCamera(Deki::Object* obj)
{
    if (auto* camera = obj->GetComponent<DekiRendering::CameraComponent>())
    {
        return camera;
    }
    for (Deki::Object* child : obj->GetChildren())
    {
        if (auto* camera = FindCamera(child))
        {
            return camera;
        }
    }
    return nullptr;
}
}  // namespace

PointerCursor::~PointerCursor()
{
    if (m_Listener != static_cast<size_t>(-1))
    {
        DekiInput::DekiInput::UnregisterEventCallback(m_Listener);
    }
}

void PointerCursor::Start()
{
    // Where the pointer already is: it carries over from the last scene.
    if (IDekiInput* input = DekiInput::DekiInput::GetInput(source))
    {
        m_Have = input->GetPointerPosition(&m_X, &m_Y);
    }
    m_Listener = DekiInput::DekiInput::RegisterEventCallback(
        [this](const InputEvent& event)
        {
            const bool pointer = event.type == InputEventType::MouseMove ||
                                 event.type == InputEventType::MouseButtonDown ||
                                 event.type == InputEventType::MouseButtonUp;
            if (pointer && event.source && source == event.source)
            {
                m_X = event.x;
                m_Y = event.y;
                m_Have = true;
            }
        });
    Show(m_Have);
}

void PointerCursor::Update()
{
    Show(m_Have);
    if (!m_Have)
    {
        return;
    }
    Deki::Engine& engine = Deki::Engine::GetInstance();
    Deki::Scene* scene = engine.GetRootScene();
    if (!scene)
    {
        return;
    }
    DekiRendering::CameraComponent* camera = nullptr;
    for (Deki::Object* obj : scene->GetObjects())
    {
        if ((camera = FindCamera(obj)) != nullptr)
        {
            break;
        }
    }
    if (!camera)
    {
        return;
    }
    float x = 0.0f, y = 0.0f;
    camera->ScreenToWorld(static_cast<float>(m_X), static_cast<float>(m_Y), engine.GetScreenWidth(),
                          engine.GetScreenHeight(), x, y);
    GetOwner()->SetWorldPosition(x, y);
}

void PointerCursor::Show(bool visible)
{
    for (Deki::Object* child : GetOwner()->GetChildren())
    {
        if (child->IsActive() != visible)
        {
            child->SetActive(visible);
        }
    }
}
