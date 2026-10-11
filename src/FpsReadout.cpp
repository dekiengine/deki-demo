#include "FpsReadout.h"

#include "TextComponent.h"

#include <deki/Time.h>

#include <cstdio>

void FpsReadout::Update()
{
    // The last frame's times: this runs in the next frame's update.
    const Deki::Engine::FrameTimings t = Deki::Engine::GetInstance().GetLastFrameTimings();
    m_WindowMs += Deki::Time::GetDeltaTimeF();
    ++m_Frames;
    m_RenderMs += t.render;
    m_PresentMs += t.present;
    if (m_WindowMs < 500.0f)
    {
        return;
    }

    auto* text = GetOwner()->GetComponent<Deki2D::TextComponent>();
    if (text)
    {
        char line[48];
        const float fps = m_Frames * 1000.0f / m_WindowMs;
        if (showTimes)
        {
            std::snprintf(line, sizeof(line), "%.0f fps  r%u p%u", fps, static_cast<unsigned>(m_RenderMs / m_Frames),
                          static_cast<unsigned>(m_PresentMs / m_Frames));
        }
        else
        {
            std::snprintf(line, sizeof(line), "%.0f fps", fps);
        }
        text->SetText(line);
    }
    m_WindowMs = 0.0f;
    m_Frames = 0;
    m_RenderMs = 0;
    m_PresentMs = 0;
}
