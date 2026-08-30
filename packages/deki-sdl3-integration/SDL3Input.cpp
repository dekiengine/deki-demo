#include "SDL3Input.h"

#include "DekiEngine.h"
#include "providers/IDekiRenderSystem.h"

namespace {
// Mouse events arrive in window pixels, but the engine's screen->world math expects
// framebuffer pixels. The desktop window is created larger than the framebuffer
// (window_scale), so scale window coords down to framebuffer coords before dispatch.
// Window and framebuffer share an aspect ratio (window = framebuffer * scale), so a
// straight ratio is exact with no letterbox offset to account for.
void WindowToFramebuffer(SDL_WindowID windowID, float& x, float& y)
{
    SDL_Window* window = SDL_GetWindowFromID(windowID);
    if (!window) return;
    int winW = 0, winH = 0;
    SDL_GetWindowSize(window, &winW, &winH);
    if (winW <= 0 || winH <= 0) return;

    IDekiRenderSystem* rs = DekiEngine::GetInstance().GetRenderSystem();
    if (!rs) return;
    int32_t fbW = rs->GetScreenWidth();
    int32_t fbH = rs->GetScreenHeight();
    if (fbW <= 0 || fbH <= 0) return;

    x *= static_cast<float>(fbW) / static_cast<float>(winW);
    y *= static_cast<float>(fbH) / static_cast<float>(winH);
}
}  // namespace

SDL3Input::SDL3Input() : initialized(false), quit_flag(false), mouse_x(0), mouse_y(0), mouse_pressed(false) {}

SDL3Input::~SDL3Input()
{
    Shutdown();
}

bool SDL3Input::Initialize()
{
    if (initialized)
    {
        return true;
    }
    initialized = true;
    quit_flag = false;
    return true;
}

void SDL3Input::Shutdown()
{
    initialized = false;
}

void SDL3Input::Update()
{
    if (!initialized)
    {
        return;
    }

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ProcessSDLEvent(event);
    }

    // Update mouse state (scale window pixels down to framebuffer pixels)
    float mx, my;
    SDL_MouseButtonFlags mouse_state = SDL_GetMouseState(&mx, &my);
    if (SDL_Window* focus = SDL_GetMouseFocus())
        WindowToFramebuffer(SDL_GetWindowID(focus), mx, my);
    mouse_x = (int32_t)mx;
    mouse_y = (int32_t)my;
    mouse_pressed = (mouse_state & SDL_BUTTON_LMASK) != 0;
}

void SDL3Input::ProcessSDLEvent(const SDL_Event& event)
{
    InputEvent input_event;
    input_event.timestamp = static_cast<uint32_t>(SDL_GetTicks());

    if (event.type == SDL_EVENT_QUIT)
    {
        quit_flag = true;
        input_event.type = InputEventType::APP_QUIT;
        NotifyCallbacks(input_event);
        return;
    }

    switch (event.type)
    {
        case SDL_EVENT_MOUSE_MOTION:
        {
            float ex = event.motion.x, ey = event.motion.y;
            WindowToFramebuffer(event.motion.windowID, ex, ey);
            input_event.type = InputEventType::MOUSE_MOVE;
            input_event.x = (int32_t)ex;
            input_event.y = (int32_t)ey;
            NotifyCallbacks(input_event);
            break;
        }

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        {
            float ex = event.button.x, ey = event.button.y;
            WindowToFramebuffer(event.button.windowID, ex, ey);
            input_event.type = InputEventType::MOUSE_BUTTON_DOWN;
            input_event.x = (int32_t)ex;
            input_event.y = (int32_t)ey;
            input_event.pressed = true;
            NotifyCallbacks(input_event);
            break;
        }

        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            float ex = event.button.x, ey = event.button.y;
            WindowToFramebuffer(event.button.windowID, ex, ey);
            input_event.type = InputEventType::MOUSE_BUTTON_UP;
            input_event.x = (int32_t)ex;
            input_event.y = (int32_t)ey;
            input_event.pressed = false;
            NotifyCallbacks(input_event);
            break;
        }

        case SDL_EVENT_KEY_DOWN:
        {
            uint32_t generic_key = ConvertSDLKeyToGeneric(event.key.key);
            key_states[generic_key] = true;

            input_event.type = InputEventType::KEY_DOWN;
            input_event.key = generic_key;
            input_event.pressed = true;
            NotifyCallbacks(input_event);
        }
        break;

        case SDL_EVENT_KEY_UP:
        {
            uint32_t generic_key = ConvertSDLKeyToGeneric(event.key.key);
            key_states[generic_key] = false;

            input_event.type = InputEventType::KEY_UP;
            input_event.key = generic_key;
            input_event.pressed = false;
            NotifyCallbacks(input_event);
        }
        break;
    }
}

void SDL3Input::NotifyCallbacks(const InputEvent& event)
{
    for (const auto& callback : event_callbacks)
    {
        callback(event);
    }
}

uint32_t SDL3Input::ConvertSDLKeyToGeneric(SDL_Keycode sdl_key)
{
    const uint32_t KEY_ENTER = 13;
    const uint32_t KEY_ESC = 27;
    const uint32_t KEY_BACKSPACE = 8;
    const uint32_t KEY_UP = 1001;
    const uint32_t KEY_DOWN = 1002;
    const uint32_t KEY_LEFT = 1003;
    const uint32_t KEY_RIGHT = 1004;

    switch (sdl_key)
    {
        case SDLK_RETURN:
            return KEY_ENTER;
        case SDLK_ESCAPE:
            return KEY_ESC;
        case SDLK_BACKSPACE:
            return KEY_BACKSPACE;
        case SDLK_UP:
            return KEY_UP;
        case SDLK_DOWN:
            return KEY_DOWN;
        case SDLK_LEFT:
            return KEY_LEFT;
        case SDLK_RIGHT:
            return KEY_RIGHT;
        default:
            if (sdl_key >= 32 && sdl_key <= 126)
            {
                return sdl_key;
            }
            return 0;
    }
}

void SDL3Input::RegisterEventCallback(const InputEventCallback& callback)
{
    event_callbacks.push_back(callback);
}

bool SDL3Input::IsInitialized() const
{
    return initialized;
}

bool SDL3Input::GetPointerPosition(int32_t* x, int32_t* y) const
{
    if (x) *x = mouse_x;
    if (y) *y = mouse_y;
    return true;
}

bool SDL3Input::IsKeyPressed(uint32_t key) const
{
    auto it = key_states.find(key);
    if (it != key_states.end())
    {
        return it->second;
    }
    return false;
}

bool SDL3Input::CheckForQuit() const
{
    return quit_flag;
}
