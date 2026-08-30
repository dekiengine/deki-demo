#pragma once

#include <stdint.h>

#include <functional>

/**
 * @brief Input event types
 */
enum class InputEventType
{
    MOUSE_MOVE,
    MOUSE_BUTTON_DOWN,
    MOUSE_BUTTON_UP,
    KEY_DOWN,
    KEY_UP,
    TOUCH_DOWN,
    TOUCH_UP,
    TOUCH_MOVE,
    APP_QUIT  // Application quit request
};

/**
 * @brief Input event data structure
 */
struct InputEvent
{
    InputEventType type;
    int32_t x, y;  // Position for mouse/touch events
    uint32_t key;  // Key code for keyboard events
    bool pressed;  // Button/key state
    uint32_t timestamp;  // Event timestamp
};

// Input event callback function type
using InputEventCallback = std::function<void(const InputEvent& event)>;

/**
 * @brief Abstract interface for input package operations
 *
 * This interface defines the contract that input packages must implement
 * to work with the Deki engine. It abstracts input initialization,
 * event handling, and input device management.
 */
class IDekiInput
{
   public:
    virtual ~IDekiInput() = default;

    /**
     * @brief Initialize the platform input system
     * @return true if initialization successful, false otherwise
     */
    virtual bool Initialize() = 0;

    /**
     * @brief Shutdown the input system and cleanup resources
     */
    virtual void Shutdown() = 0;

    /**
     * @brief Update input system and process events (called each frame)
     */
    virtual void Update() = 0;

    /**
     * @brief Register a callback for input events
     * @param callback Function to call when input events occur
     */
    virtual void RegisterEventCallback(const InputEventCallback& callback) = 0;

    /**
     * @brief Check if the input system is initialized
     * @return true if initialized, false otherwise
     */
    virtual bool IsInitialized() const = 0;

    /**
     * @brief Get current mouse/touch position
     * @param x Pointer to store X coordinate
     * @param y Pointer to store Y coordinate
     * @return true if position is valid, false otherwise
     */
    virtual bool GetPointerPosition(int32_t* x, int32_t* y) const = 0;

    /**
     * @brief Check if a key is currently pressed
     * @param key Key code to check
     * @return true if key is pressed, false otherwise
     */
    virtual bool IsKeyPressed(uint32_t key) const = 0;
};