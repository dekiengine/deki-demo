#include "KeyboardDemo.h"

#include "DekiInput.h"
#include "I2CKeyboard.h"
#include "Keys.h"

#include <deki/Time.h>

#include <algorithm>
#include <cstdio>
#include <cstring>

using namespace DekiInput;

namespace
{
// The keyboard with a backlight, if there is one: the T-Deck's.
I2CKeyboard* LitKeyboard()
{
    // Only I2CKeyboardComponent registers this name, and always with an
    // I2CKeyboard.
    auto* keyboard = static_cast<I2CKeyboard*>(DekiInput::DekiInput::GetInput("I2CKeyboard"));
    return keyboard && keyboard->HasBacklight() ? keyboard : nullptr;
}

// What a source is called on screen.
const char* SourceName(const char* source)
{
    if (!source)
    {
        return "?";
    }
    if (std::strcmp(source, "I2CKeyboard") == 0)
    {
        return "keyboard";
    }
    if (std::strcmp(source, "SDL3") == 0)
    {
        return "PC";
    }
    if (std::strcmp(source, "Injected") == 0)
    {
        return "editor";
    }
    return source;
}

// The backlight's steps: off, then eight up to full.
constexpr int kLightSteps = 8;
}  // namespace

KeyboardDemo::~KeyboardDemo()
{
    // The callback points at this object; it must not outlive it.
    if (m_Listener != static_cast<size_t>(-1))
    {
        DekiInput::DekiInput::UnregisterEventCallback(m_Listener);
    }
}

void KeyboardDemo::Start()
{
    m_Listener = DekiInput::DekiInput::RegisterEventCallback(
        [this](const InputEvent& event)
        {
            if (event.type == InputEventType::KeyDown)
            {
                OnKey(event.key, event.character, event.source);
            }
        });

    const bool lit = LitKeyboard() != nullptr;
    if (auto* button = lightDown.Get())
    {
        button->AddOnClickCallback([this]() { StepLight(-1); });
        button->SetEnabled(lit);
    }
    if (auto* button = lightUp.Get())
    {
        button->AddOnClickCallback([this]() { StepLight(+1); });
        button->SetEnabled(lit);
    }
    RefreshLight();
    Refresh();
}

void KeyboardDemo::Update()
{
    m_BlinkMs += Deki::Time::GetDeltaTimeF();
    if (m_BlinkMs >= 500.0f)
    {
        m_BlinkMs = 0.0f;
        m_CaretOn = !m_CaretOn;
        Refresh();
    }
}

void KeyboardDemo::OnKey(uint32_t key, uint32_t character, const char* source)
{
    char description[64];
    if (key == Keys::Backspace || key == Keys::Delete)
    {
        if (!m_Text.empty())
        {
            m_Text.pop_back();
        }
        std::snprintf(description, sizeof(description), "Backspace");
    }
    else if (key == Keys::Enter)
    {
        m_Text += '\n';
        std::snprintf(description, sizeof(description), "Enter");
    }
    else
    {
        // A desktop keyboard gives the key but not the character it types.
        uint32_t typedChar = character;
        if (typedChar == 0 && key >= 32 && key < 127)
        {
            typedChar = key;
        }
        if (typedChar >= 32 && typedChar < 127)
        {
            m_Text += static_cast<char>(typedChar);
            std::snprintf(description, sizeof(description), "'%c'  (%u)", static_cast<char>(typedChar),
                          static_cast<unsigned>(typedChar));
        }
        else if (key == Keys::Up || key == Keys::Down || key == Keys::Left || key == Keys::Right)
        {
            static const char* const kArrows[] = { "Up", "Down", "Left", "Right" };
            std::snprintf(description, sizeof(description), "%s", kArrows[key - Keys::Up]);
        }
        else
        {
            std::snprintf(description, sizeof(description), "key %u", static_cast<unsigned>(key));
        }
    }

    // Keep the end: the box shows the latest typing.
    if (m_Text.size() > static_cast<size_t>(maxCharacters))
    {
        m_Text.erase(0, m_Text.size() - static_cast<size_t>(maxCharacters));
    }

    if (auto* text = lastKey.Get())
    {
        char line[96];
        std::snprintf(line, sizeof(line), "Key %s from %s", description, SourceName(source));
        text->SetText(line);
    }
    m_CaretOn = true;
    m_BlinkMs = 0.0f;
    Refresh();
}

void KeyboardDemo::StepLight(int direction)
{
    I2CKeyboard* keyboard = LitKeyboard();
    if (!keyboard)
    {
        return;
    }
    // From the level to the nearest step, then one step on.
    const int step = (keyboard->GetBacklight() * kLightSteps + 127) / 255;
    const int next = std::min(std::max(step + direction, 0), kLightSteps);
    keyboard->SetBacklight(static_cast<uint8_t>(next * 255 / kLightSteps));
    RefreshLight();
}

void KeyboardDemo::RefreshLight()
{
    if (auto* text = lightLevel.Get())
    {
        I2CKeyboard* keyboard = LitKeyboard();
        char line[32];
        if (keyboard)
        {
            std::snprintf(line, sizeof(line), "Light %d%%", keyboard->GetBacklight() * 100 / 255);
        }
        else
        {
            std::snprintf(line, sizeof(line), "No key light");
        }
        text->SetText(line);
    }
}

void KeyboardDemo::Refresh()
{
    if (auto* text = typed.Get())
    {
        text->SetText(m_Text + (m_CaretOn ? "_" : " "));
    }
}
