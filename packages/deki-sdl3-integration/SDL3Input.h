#pragma once

#include <SDL3/SDL.h>

#include <unordered_map>
#include <vector>

#include "IDekiInput.h"  // from deki-input

/**
 * @brief SDL3 implementation of input interface
 */
class SDL3Input : public IDekiInput
{
   private:
    bool initialized;
    std::vector<InputEventCallback> event_callbacks;
    bool quit_flag;  // Added to handle quit events properly

    // Key state tracking
    std::unordered_map<uint32_t, bool> key_states;

    // Mouse state
    int32_t mouse_x, mouse_y;
    bool mouse_pressed;

    // Internal methods
    void ProcessSDLEvent(const SDL_Event& event);
    void NotifyCallbacks(const InputEvent& event);
    uint32_t ConvertSDLKeyToGeneric(SDL_Keycode sdl_key);

   public:
    SDL3Input();
    virtual ~SDL3Input();

    // IDekiInput interface
    bool Initialize() override;
    void Shutdown() override;
    void Update() override;
    void RegisterEventCallback(const InputEventCallback& callback) override;
    bool IsInitialized() const override;
    bool GetPointerPosition(int32_t* x, int32_t* y) const override;
    bool IsKeyPressed(uint32_t key) const override;

    // SDL3-specific methods
    bool CheckForQuit() const;
};
