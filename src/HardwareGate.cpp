#include "HardwareGate.h"

#include "ButtonComponent.h"
#include "DekiGPS.h"
#include "DekiInput.h"
#include "DekiLoRa.h"
#include "TextComponent.h"

namespace
{
// The input sources that type: the T-Deck's keyboard, a desktop's, and the
// keys the editor's play mode passes on.
bool HasKeyboard()
{
#ifdef DEKI_EDITOR
    return true;
#else
    using Input = DekiInput::DekiInput;
    return Input::GetInput("I2CKeyboard") || Input::GetInput("SDL3") || Input::GetInput("Injected");
#endif
}
}  // namespace

bool HardwareGate::BoardHas(DemoHardware hardware)
{
    switch (hardware)
    {
        case DemoHardware::Gps: return DekiGps::DekiGPS::GetCurrent() != nullptr;
        case DemoHardware::LoRa: return DekiLora::DekiLoRa::GetCurrent() != nullptr;
        case DemoHardware::Keyboard: return HasKeyboard();
        case DemoHardware::Trackball: return DekiInput::DekiInput::GetInput("Trackball") != nullptr;
    }
    return false;
}

void HardwareGate::Start()
{
    if (BoardHas(hardware))
    {
        return;
    }
    if (auto* button = GetOwner()->GetComponent<Deki2D::ButtonComponent>())
    {
        button->SetEnabled(false);
    }
    // The label dims with the button.
    for (Deki::Object* child : GetOwner()->GetChildren())
    {
        if (auto* label = child->GetComponent<Deki2D::TextComponent>())
        {
            label->SetColor(Deki::Color(0x80, 0x86, 0x96, 0xFF));
        }
    }
}
