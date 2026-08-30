#include "DekiInput.h"
#include "DekiLogSystem.h"

// Static member definitions
std::map<std::string, std::unique_ptr<IDekiInput>> DekiInput::active_inputs;
std::vector<InputEventCallback> DekiInput::global_callbacks;
bool DekiInput::initialized = false;
bool DekiInput::should_exit = false;

bool DekiInput::SetInput(std::unique_ptr<IDekiInput> input, const std::string& name)
{
    if (!input)
    {
        DEKI_LOG_ERROR("DekiInput::SetInput: null input");
        return false;
    }

    // Check if already registered
    if (active_inputs.find(name) != active_inputs.end())
    {
        DEKI_LOG_INTERNAL("Input package '%s' already registered", name.c_str());
        return true;
    }

    // Register internal callback to distribute events
    input->RegisterEventCallback([](const InputEvent& event) {
        DistributeEvent(event);
    });

    // Store the input
    active_inputs[name] = std::move(input);
    initialized = true;

    DEKI_LOG_INTERNAL("DekiInput: Input '%s' set", name.c_str());
    return true;
}

void DekiInput::Shutdown()
{
    if (!initialized)
    {
        return;
    }

    // Shutdown all active inputs
    for (auto& [name, input] : active_inputs)
    {
        if (input)
        {
            input->Shutdown();
        }
    }

    active_inputs.clear();
    global_callbacks.clear();
    initialized = false;
    should_exit = false;

    DEKI_LOG_INTERNAL("DekiInput shutdown");
}

IDekiInput* DekiInput::GetInput(const std::string& name)
{
    auto it = active_inputs.find(name);
    if (it != active_inputs.end())
    {
        return it->second.get();
    }
    return nullptr;
}

void DekiInput::RegisterEventCallback(const InputEventCallback& callback)
{
    global_callbacks.push_back(callback);
}

void DekiInput::ClearEventCallbacks()
{
    global_callbacks.clear();
}

void DekiInput::Update()
{
    if (!initialized)
    {
        return;
    }

    // Update all active input packages
    for (auto& [name, input] : active_inputs)
    {
        if (input && input->IsInitialized())
        {
            input->Update();
        }
    }
}

bool DekiInput::GetPointerPosition(int32_t* x, int32_t* y)
{
    if (!initialized || !x || !y)
    {
        return false;
    }

    // Try to get position from first available input
    for (auto& [name, input] : active_inputs)
    {
        if (input && input->IsInitialized())
        {
            if (input->GetPointerPosition(x, y))
            {
                return true;
            }
        }
    }

    return false;
}

bool DekiInput::IsKeyPressed(uint32_t key)
{
    if (!initialized)
    {
        return false;
    }

    // Check if key is pressed in any active input
    for (auto& [name, input] : active_inputs)
    {
        if (input && input->IsInitialized())
        {
            if (input->IsKeyPressed(key))
            {
                return true;
            }
        }
    }

    return false;
}

bool DekiInput::ShouldExit()
{
    return should_exit;
}

void DekiInput::SetShouldExit(bool exit)
{
    should_exit = exit;
}

std::vector<std::string> DekiInput::GetActiveInputs()
{
    std::vector<std::string> names;
    for (const auto& [name, input] : active_inputs)
    {
        names.push_back(name);
    }
    return names;
}

void DekiInput::DistributeEvent(const InputEvent& event)
{
    // Handle APP_QUIT events
    if (event.type == InputEventType::APP_QUIT)
    {
        should_exit = true;
        DEKI_LOG_INTERNAL("DekiInput: APP_QUIT event received");
    }

    // Distribute event to all registered global callbacks
    for (const auto& callback : global_callbacks)
    {
        if (callback)
        {
            callback(event);
        }
    }
}
