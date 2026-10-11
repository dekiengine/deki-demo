#pragma once

#include <deki/Engine.h>
#include <deki/reflection/ObjectRef.h>

#include "ButtonComponent.h"
#include "TextComponent.h"

#include <cstddef>
#include <string>

// Shows what is typed on the keyboard: the T-Deck's, or a desktop's. Two
// buttons dim and brighten the keyboard's backlight, where it has one.
DEKI_CATEGORY("Demo")
DEKI_DESCRIPTION("Shows what is typed on the keyboard, and the last key pressed.")
class KeyboardDemo : public Deki::Component
{
public:
    DEKI_EXPORT
    DEKI_TOOLTIP("Where the typed text goes.")
    Deki::ObjectRef<Deki2D::TextComponent> typed;

    DEKI_EXPORT
    DEKI_TOOLTIP("Where the last key is described.")
    Deki::ObjectRef<Deki2D::TextComponent> lastKey;

    DEKI_EXPORT DEKI_SLIDER(20, 300)
    int32_t maxCharacters = 120;

    DEKI_EXPORT
    DEKI_TOOLTIP("Dims the keyboard's backlight a step.")
    Deki::ObjectRef<Deki2D::ButtonComponent> lightDown;

    DEKI_EXPORT
    DEKI_TOOLTIP("Brightens the keyboard's backlight a step.")
    Deki::ObjectRef<Deki2D::ButtonComponent> lightUp;

    DEKI_EXPORT
    DEKI_TOOLTIP("Shows the backlight level.")
    Deki::ObjectRef<Deki2D::TextComponent> lightLevel;

    KeyboardDemo() = default;
    ~KeyboardDemo() override;

    void Start() override;
    void Update() override;

private:
    size_t m_Listener = static_cast<size_t>(-1);
    std::string m_Text;
    float m_BlinkMs = 0.0f;
    bool m_CaretOn = true;

    void OnKey(uint32_t key, uint32_t character, const char* source);
    void StepLight(int direction);
    void RefreshLight();
    void Refresh();
};
