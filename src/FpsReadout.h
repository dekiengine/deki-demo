#pragma once

#include <deki/Engine.h>

#include <cstdint>

// Shows the frame rate on the TextComponent of the same object, with how long
// rendering and presenting a frame take, averaged over half a second:
// "17 fps  r25 p31".
DEKI_CATEGORY("Demo")
DEKI_DESCRIPTION("Shows the frames per second, and the render and present times, on this object's text.")
class FpsReadout : public Deki::Component
{
public:
    DEKI_EXPORT
    DEKI_TOOLTIP("Also show how many milliseconds rendering and presenting a frame take.")
    bool showTimes = true;

    FpsReadout() = default;

    void Update() override;

private:
    float m_WindowMs = 0.0f;
    uint32_t m_Frames = 0;
    uint32_t m_RenderMs = 0;
    uint32_t m_PresentMs = 0;
};
