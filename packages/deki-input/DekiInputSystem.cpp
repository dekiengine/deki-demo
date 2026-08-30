#include "DekiInputSystem.h"
#include "InputCollider.h"
#include "DekiEngine.h"
#include "DekiObject.h"
#include "Scene.h"
#include "deki-rendering/CameraComponent.h"
#include "DekiInput.h"  // now local to this package
#include "providers/IDekiRenderSystem.h"

#include <map>
#include <memory>

namespace
{

// Keyboard state fed by DispatchKey() rather than a physical device driver.
// Registered with DekiInput on first injected key so IsKeyPressed() aggregates
// injected keys exactly like keys from a real driver (SDL3 on the desktop
// simulator). Owned by DekiInput once registered.
class InjectedKeyboard : public IDekiInput
{
public:
    bool Initialize() override { return true; }
    void Shutdown() override { m_Keys.clear(); }
    void Update() override {}
    void RegisterEventCallback(const InputEventCallback& callback) override { m_Callback = callback; }
    bool IsInitialized() const override { return true; }
    bool GetPointerPosition(int32_t*, int32_t*) const override { return false; }

    bool IsKeyPressed(uint32_t key) const override
    {
        auto it = m_Keys.find(key);
        return it != m_Keys.end() && it->second;
    }

    void SetKey(uint32_t key, bool down)
    {
        m_Keys[key] = down;

        if (m_Callback)
        {
            InputEvent event{};
            event.type = down ? InputEventType::KEY_DOWN : InputEventType::KEY_UP;
            event.key = key;
            event.pressed = down;
            m_Callback(event);
        }
    }

private:
    std::map<uint32_t, bool> m_Keys;
    InputEventCallback m_Callback;
};

constexpr const char* kInjectedKeyboardName = "Injected";

} // namespace

DekiInputSystem::DekiInputSystem()
{
}

DekiInputSystem::~DekiInputSystem()
{
    Shutdown();
}

void DekiInputSystem::Initialize()
{
    if (m_Initialized)
        return;

    // Register callback on DekiInput to receive input events
    DekiInput::RegisterEventCallback([this](const InputEvent& event) {
        OnInputEvent(event);
    });

    m_Initialized = true;
}

void DekiInputSystem::Shutdown()
{
    // Clear callbacks BEFORE DLL unload — the std::function objects in
    // DekiInput::global_callbacks hold lambdas whose code lives
    // in this DLL. After FreeLibrary, those function pointers are stale
    // and any operation on them (move, copy, destroy) will crash.
    DekiInput::ClearEventCallbacks();

    // Tear down active input drivers (mouse / keyboard / touch). DekiEngine
    // reaches this through the interface so it doesn't need to link to the
    // static DekiInput symbol in this package DLL.
    DekiInput::Shutdown();

    m_Initialized = false;
}

void DekiInputSystem::Update()
{
    DekiInput::Update();
}

void DekiInputSystem::DispatchKey(uint32_t key, bool down)
{
    auto* driver = DekiInput::GetInput(kInjectedKeyboardName);
    if (!driver)
    {
        // Registered on demand so the driver only exists for hosts that
        // actually inject keys; DekiInput owns it from here on and clears it
        // on DekiInput::Shutdown (looked up by name each call, never cached).
        DekiInput::SetInput(std::make_unique<InjectedKeyboard>(), kInjectedKeyboardName);
        driver = DekiInput::GetInput(kInjectedKeyboardName);
        if (!driver)
            return;
    }

    static_cast<InjectedKeyboard*>(driver)->SetKey(key, down);
}

bool DekiInputSystem::ShouldExit() const
{
    return DekiInput::ShouldExit();
}

void DekiInputSystem::OnInputEvent(const InputEvent& event)
{
    DekiEngine& engine = DekiEngine::GetInstance();
    if (!engine.IsInitialized())
        return;

    Scene* scene = engine.GetRootScene();
    if (!scene)
        return;

    bool isDown = (event.type == InputEventType::MOUSE_BUTTON_DOWN);
    bool isMove = (event.type == InputEventType::MOUSE_MOVE);
    bool isUp   = (event.type == InputEventType::MOUSE_BUTTON_UP);

    if (!isDown && !isMove && !isUp)
        return;

    // Find camera for screen-to-world coordinate conversion (search recursively)
    CameraComponent* cam = nullptr;
    std::function<CameraComponent*(DekiObject*)> findCamera = [&](DekiObject* obj) -> CameraComponent* {
        CameraComponent* c = obj->GetComponent<CameraComponent>();
        if (c) return c;
        for (auto* child : obj->GetChildren())
        {
            c = findCamera(child);
            if (c) return c;
        }
        return nullptr;
    };
    for (DekiObject* obj : scene->GetObjects())
    {
        cam = findCamera(obj);
        if (cam) break;
    }

    float worldX = static_cast<float>(event.x);
    float worldY = static_cast<float>(event.y);

    if (cam && engine.GetRenderSystem())
    {
        cam->ScreenToWorld(static_cast<float>(event.x), static_cast<float>(event.y),
                           engine.GetRenderSystem()->GetScreenWidth(),
                           engine.GetRenderSystem()->GetScreenHeight(),
                           worldX, worldY);
    }

    DispatchInput(scene, worldX, worldY, isDown, isMove, isUp);
}

void DekiInputSystem::DispatchInput(Scene* scene, float x, float y,
                                     bool down, bool move, bool up)
{
    if (!scene)
        return;

    for (DekiObject* obj : scene->GetObjects())
    {
        DispatchToObject(obj, x, y, down, move, up);
    }
}

bool DekiInputSystem::DispatchToObject(DekiObject* obj, float x, float y,
                                        bool down, bool move, bool up)
{
    if (!obj)
        return false;

    // Phase 1: Recurse to children FIRST (deepest child gets priority)
    // All siblings are dispatched so they can track hover state (pointer_exit).
    bool childConsumed = false;
    for (DekiObject* child : obj->GetChildren())
    {
        if (DispatchToObject(child, x, y, down, move, up))
            childConsumed = true;
    }

    // Phase 2: Process this object's InputCollider
    for (DekiComponent* comp : obj->GetComponents())
    {
        if (comp->getType() == InputCollider::StaticType ||
            comp->getBaseType() == InputCollider::StaticType)
        {
            auto* collider = static_cast<InputCollider*>(comp);
            // Process if: no child consumed, OR this is a non-consuming collider (e.g. scroll)
            if (!childConsumed || !collider->consume_input)
            {
                bool handled = collider->ProcessInput(x, y, down, move, up);
                if (handled && collider->consume_input)
                    return true;
            }
            break;
        }
    }

    return childConsumed;
}
