#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>

#include "IDekiInput.h"

/**
 * @brief Input Provider for handling multiple input implementations
 *
 * Centralized registry for input sources (mouse / keyboard / touch / gamepad
 * / etc.). Multiple input packages can be active simultaneously; events from
 * every active source are aggregated into the global event stream.
 *
 * Concrete drivers live in platform integration packages and register via
 * SetInput(); this package owns only the interface and the dispatch logic.
 */
class DekiInput
{
private:
    static std::map<std::string, std::unique_ptr<IDekiInput>> active_inputs;
    static std::vector<InputEventCallback> global_callbacks;
    static bool initialized;
    static bool should_exit;

public:
    /**
     * @brief Set a pre-initialized input package directly
     * @param input The input package to use (takes ownership)
     * @param name Input package name for identification
     * @return true if set successfully
     */
    static bool SetInput(std::unique_ptr<IDekiInput> input, const std::string& name);

    /**
     * @brief Shutdown all input packages
     */
    static void Shutdown();

    /**
     * @brief Get specific input by name
     * @param name Input package name
     * @return Pointer to input package, or nullptr if not found
     */
    static IDekiInput* GetInput(const std::string& name);

    /**
     * @brief Register global event callback (receives events from ALL inputs)
     * @param callback Function to call when input events occur
     */
    static void RegisterEventCallback(const InputEventCallback& callback);

    /**
     * @brief Clear all global event callbacks
     *
     * Must be called before freeing DLLs that registered callbacks,
     * otherwise the std::function internals point to freed code.
     */
    static void ClearEventCallbacks();

    /**
     * @brief Update all active inputs (called each frame)
     * This processes events from all registered input sources
     */
    static void Update();

    /**
     * @brief Get current pointer/mouse position from first available input
     * @param x Pointer to store X coordinate
     * @param y Pointer to store Y coordinate
     * @return true if position is valid
     */
    static bool GetPointerPosition(int32_t* x, int32_t* y);

    /**
     * @brief Check if a key is currently pressed across all inputs
     * @param key Key code to check
     * @return true if key is pressed in any active input
     */
    static bool IsKeyPressed(uint32_t key);

    /**
     * @brief Check if the application should exit
     * @return true if quit event was received from any input
     */
    static bool ShouldExit();

    /**
     * @brief Set the exit flag (called by input packages on quit events)
     * @param exit Should exit flag
     */
    static void SetShouldExit(bool exit);

    /**
     * @brief Check if the input backend is initialized
     * @return true if initialized, false otherwise
     */
    static bool IsInitialized() { return initialized; }

    /**
     * @brief Get list of all active input package names
     * @return Vector of active input names
     */
    static std::vector<std::string> GetActiveInputs();

private:
    /**
     * @brief Internal callback for distributing events to global callbacks
     */
    static void DistributeEvent(const InputEvent& event);
};
