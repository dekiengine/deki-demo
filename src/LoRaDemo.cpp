#include "LoRaDemo.h"

#include "DekiInput.h"
#include "DekiLoRa.h"
#include "Keys.h"

#include <deki/Time.h>

#include <cstdio>

namespace
{
constexpr size_t kHeardLines = 4;
}

LoRaDemo::~LoRaDemo()
{
    if (m_Listener != static_cast<size_t>(-1))
    {
        DekiInput::DekiInput::UnregisterEventCallback(m_Listener);
    }
}

void LoRaDemo::Start()
{
    if (auto* button = pingButton.Get())
    {
        button->AddOnClickCallback([this]() { Ping(); });
    }
    // Enter on the keyboard, or a trackball click (which arrives as Enter).
    m_Listener = DekiInput::DekiInput::RegisterEventCallback(
        [this](const DekiInput::InputEvent& event)
        {
            if (event.type == DekiInput::InputEventType::KeyDown && event.key == DekiInput::Keys::Enter)
            {
                Ping();
            }
        });
    Refresh();
}

void LoRaDemo::Ping()
{
    auto* radio = DekiLora::DekiLoRa::GetCurrent();
    if (!radio)
    {
        return;
    }
    if (radio->IsSending() || radio->MillisecondsUntilCanSend() > 0)
    {
        return;  // the status line says how long is left
    }
    char payload[48];
    const int length = std::snprintf(payload, sizeof(payload), "%s %u", message.c_str(),
                                     static_cast<unsigned>(m_Pings + 1));
    if (length > 0 && radio->Send(reinterpret_cast<const uint8_t*>(payload), static_cast<size_t>(length)))
    {
        ++m_Pings;
    }
    Refresh();
}

void LoRaDemo::Update()
{
    auto* radio = DekiLora::DekiLoRa::GetCurrent();
    if (radio)
    {
        DekiLora::LoRaPacket packet;
        while (radio->Receive(packet))
        {
            // Printable characters only: another radio's packet can be
            // anything.
            std::string text;
            for (uint8_t c : packet.data)
            {
                if (text.size() >= 14)
                {
                    text += "..";
                    break;
                }
                text += (c >= 32 && c < 127) ? static_cast<char>(c) : '.';
            }
            // One line each, so four fit: text, signal strength, SNR.
            char line[96];
            std::snprintf(line, sizeof(line), "'%s' %.0f dBm %.1f dB", text.c_str(), packet.rssi, packet.snr);
            m_Heard.push_front(line);
            if (m_Heard.size() > kHeardLines)
            {
                m_Heard.pop_back();
            }
        }
    }

    m_SinceRefreshMs += Deki::Time::GetDeltaTimeF();
    if (m_SinceRefreshMs >= 200.0f)
    {
        m_SinceRefreshMs = 0.0f;
        Refresh();
    }
}

void LoRaDemo::Refresh()
{
    auto* radio = DekiLora::DekiLoRa::GetCurrent();
    if (auto* text = status.Get())
    {
        if (!radio)
        {
            text->SetText("No LoRa radio on this board");
        }
        else
        {
            const DekiLora::LoRaSettings s = radio->Settings();
            char next[48];
            const uint32_t waitMs = radio->MillisecondsUntilCanSend();
            if (radio->IsSending())
            {
                std::snprintf(next, sizeof(next), "Sending...");
            }
            else if (waitMs > 0)
            {
                std::snprintf(next, sizeof(next), "Next ping in %.1f s", waitMs / 1000.0f);
            }
            else
            {
                std::snprintf(next, sizeof(next), "Ready: press Ping or Enter");
            }
            char lines[192];
            std::snprintf(lines, sizeof(lines), "%.1f MHz  SF%d  %u kHz  %d dBm\nSent %u  Heard %u  Bad %u\n%s",
                          s.frequencyHz / 1e6, s.spreadingFactor, static_cast<unsigned>(s.bandwidthHz / 1000),
                          s.txPowerDbm, static_cast<unsigned>(radio->PacketsSent()),
                          static_cast<unsigned>(radio->PacketsReceived()),
                          static_cast<unsigned>(radio->PacketsCorrupt()), next);
            text->SetText(lines);
        }
    }
    if (auto* text = heard.Get())
    {
        if (m_Heard.empty())
        {
            text->SetText(radio ? "Listening... nothing heard yet" : "");
        }
        else
        {
            std::string all;
            for (const std::string& line : m_Heard)
            {
                all += line + "\n";
            }
            text->SetText(all);
        }
    }
}
