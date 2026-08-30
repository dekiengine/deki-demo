#pragma once

#include <cstdint>

#include "providers/IDekiInputSystem.h"

// Forward declarations
class DekiObject;
struct InputEvent;

/**
 * @brief Input dispatch system — routes input events to InputCollider components
 *
 * Concrete implementation of IDekiInputSystem, lives in the input package DLL.
 * Registered on DekiEngine via SetInputSystem() during package init.
 *
 * For embedded runtime: registers as a callback on DekiInput and
 * auto-dispatches when input events arrive.
 *
 * For editor play mode: call DispatchInput() directly from PlayViewPanel.
 *
 * Coordinates: DispatchInput takes WORLD UNITS (float). Raw device pixels
 * coming from InputEvent are converted via Camera::ScreenToWorld in
 * OnInputEvent before reaching dispatch.
 */
class DekiInputSystem : public IDekiInputSystem
{
public:
    DekiInputSystem();
    ~DekiInputSystem() override;

    void Initialize() override;
    void Shutdown() override;
    void DispatchInput(Scene* scene, float x, float y,
                       bool down, bool move, bool up) override;

    /**
     * @brief Inject a key state change from the host (editor play view).
     *
     * Backed by an "Injected" IDekiInput driver registered on demand, so
     * DekiInput::IsKeyPressed() aggregates injected keys exactly like keys
     * from a real driver (SDL3 on the desktop simulator).
     */
    void DispatchKey(uint32_t key, bool down) override;

    bool IsInitialized() const override { return m_Initialized; }

    void Update() override;
    bool ShouldExit() const override;

private:
    bool m_Initialized = false;

    /**
     * @brief Callback from DekiInput — converts screen→world and dispatches
     */
    void OnInputEvent(const InputEvent& event);

    /**
     * @brief Recursively dispatch input to an object and its children
     *
     * Uses children-first dispatch: deepest child processes first,
     * giving inner/frontmost elements priority over parents.
     *
     * @return true if input was consumed (a collider with consume_input=true handled it)
     */
    bool DispatchToObject(DekiObject* obj, float x, float y,
                          bool down, bool move, bool up);
};
