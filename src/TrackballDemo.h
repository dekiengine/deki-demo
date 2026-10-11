#pragma once

#include <deki/Engine.h>
#include <deki/reflection/ObjectRef.h>

#include "SpriteComponent.h"
#include "TextComponent.h"

#include <cstddef>
#include <cstdint>

// A pointer test for the trackball: the object it is on is a target. Click it
// with the pointer and it changes colour and jumps somewhere else in the
// field; the readout counts hits and misses and shows where the pointer is.
// Needs a ButtonComponent on the same object.
DEKI_CATEGORY("Demo")
DEKI_DESCRIPTION("A target to click with the pointer: it jumps elsewhere on each hit.")
class TrackballDemo : public Deki::Component
{
public:
    DEKI_EXPORT
    DEKI_TOOLTIP("The sprite tinted on each hit.")
    Deki::ObjectRef<Deki2D::SpriteComponent> sprite;

    DEKI_EXPORT
    DEKI_TOOLTIP("Where hits, misses and the pointer position are shown.")
    Deki::ObjectRef<Deki2D::TextComponent> readout;

    DEKI_EXPORT
    DEKI_TOOLTIP("How far from its start the target may jump, sideways, in meters.")
    DEKI_SLIDER(1, 10)
    float halfWidth = 6.5f;

    DEKI_EXPORT
    DEKI_TOOLTIP("How far from its start the target may jump, up and down, in meters.")
    DEKI_SLIDER(1, 10)
    float halfHeight = 2.75f;

    TrackballDemo() = default;
    ~TrackballDemo() override;

    void Start() override;

private:
    size_t m_Listener = static_cast<size_t>(-1);
    float m_HomeX = 0.0f;
    float m_HomeY = 0.0f;
    uint32_t m_Random = 0x2545F491u;
    int m_Hits = 0;
    int m_Presses = 0;
    int32_t m_PointerX = -1;
    int32_t m_PointerY = -1;

    void OnHit();
    float NextRandom();  // -1 to 1
    void Refresh();
};
