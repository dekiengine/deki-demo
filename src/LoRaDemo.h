#pragma once

#include <deki/Engine.h>
#include <deki/reflection/ObjectRef.h>

#include "ButtonComponent.h"
#include "TextComponent.h"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>

// Pings over LoRa and lists what is heard. A ping goes on the button, Enter or
// a trackball click, as often as the radio's duty cycle allows; two boards
// running it hear each other.
DEKI_CATEGORY("Demo")
DEKI_DESCRIPTION("Sends numbered pings over LoRa and lists every packet heard, with its signal.")
class LoRaDemo : public Deki::Component
{
public:
    DEKI_EXPORT
    DEKI_TOOLTIP("Sends a ping when pressed.")
    Deki::ObjectRef<Deki2D::ButtonComponent> pingButton;

    DEKI_EXPORT
    DEKI_TOOLTIP("The radio settings, the counts and when the next ping may go.")
    Deki::ObjectRef<Deki2D::TextComponent> status;

    DEKI_EXPORT
    DEKI_TOOLTIP("The last packets heard.")
    Deki::ObjectRef<Deki2D::TextComponent> heard;

    DEKI_EXPORT
    DEKI_TOOLTIP("Shown at the start of each ping, so this demo's pings can be told from other traffic.")
    std::string message = "deki ping";

    LoRaDemo() = default;
    ~LoRaDemo() override;

    void Start() override;
    void Update() override;

private:
    size_t m_Listener = static_cast<size_t>(-1);
    uint32_t m_Pings = 0;
    std::deque<std::string> m_Heard;
    float m_SinceRefreshMs = 0.0f;

    void Ping();
    void Refresh();
};
