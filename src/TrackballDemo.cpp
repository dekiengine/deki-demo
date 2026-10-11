#include "TrackballDemo.h"

#include "ButtonComponent.h"
#include "DekiInput.h"

#include <deki/Time.h>

#include <cstdio>
#include <cstring>

using namespace DekiInput;

namespace
{
// The colours a hit steps through.
const Deki::Color kColours[] = { Deki::Color(0x6B, 0xD3, 0xFF), Deki::Color(0xFF, 0xC8, 0x57),
                                 Deki::Color(0x7D, 0xE0, 0x8C), Deki::Color(0xFF, 0x7A, 0x8A),
                                 Deki::Color(0xC4, 0x9B, 0xFF) };
constexpr int kColourCount = sizeof(kColours) / sizeof(kColours[0]);
}  // namespace

TrackballDemo::~TrackballDemo()
{
    if (m_Listener != static_cast<size_t>(-1))
    {
        DekiInput::DekiInput::UnregisterEventCallback(m_Listener);
    }
}

void TrackballDemo::Start()
{
    m_HomeX = GetOwner()->GetX();
    m_HomeY = GetOwner()->GetY();
    m_Random ^= Deki::Time::GetTime();

    if (auto* button = GetOwner()->GetComponent<Deki2D::ButtonComponent>())
    {
        button->AddOnClickCallback([this]() { OnHit(); });
    }
    // Every press of the pointer, hit or not, and where the pointer is.
    m_Listener = DekiInput::DekiInput::RegisterEventCallback(
        [this](const InputEvent& event)
        {
            if (event.type == InputEventType::MouseMove || event.type == InputEventType::MouseButtonDown)
            {
                m_PointerX = event.x;
                m_PointerY = event.y;
            }
            if (event.type == InputEventType::MouseButtonDown || event.type == InputEventType::TouchDown)
            {
                ++m_Presses;
            }
            if (event.type != InputEventType::KeyDown && event.type != InputEventType::KeyUp)
            {
                Refresh();
            }
        });
    if (auto* s = sprite.Get())
    {
        s->SetTint(kColours[0]);
    }
    Refresh();
}

float TrackballDemo::NextRandom()
{
    // xorshift32: enough to scatter a target.
    m_Random ^= m_Random << 13;
    m_Random ^= m_Random >> 17;
    m_Random ^= m_Random << 5;
    return static_cast<float>(m_Random % 2001u) / 1000.0f - 1.0f;
}

void TrackballDemo::OnHit()
{
    ++m_Hits;
    if (auto* s = sprite.Get())
    {
        s->SetTint(kColours[m_Hits % kColourCount]);
    }
    GetOwner()->SetLocalPosition(m_HomeX + NextRandom() * halfWidth, m_HomeY + NextRandom() * halfHeight);
    Refresh();
}

void TrackballDemo::Refresh()
{
    if (auto* text = readout.Get())
    {
        const int misses = m_Presses > m_Hits ? m_Presses - m_Hits : 0;
        char line[96];
        if (m_PointerX >= 0)
        {
            std::snprintf(line, sizeof(line), "Hits %d  Misses %d\nPointer %d, %d", m_Hits, misses,
                          static_cast<int>(m_PointerX), static_cast<int>(m_PointerY));
        }
        else
        {
            std::snprintf(line, sizeof(line), "Hits %d  Misses %d\nRoll the ball to move the pointer", m_Hits, misses);
        }
        text->SetText(line);
    }
}
