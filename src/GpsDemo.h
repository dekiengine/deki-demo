#pragma once

#include <deki/Engine.h>
#include <deki/reflection/ObjectRef.h>

#include "TextComponent.h"

// Shows what the GPS receiver reports: the fix when there is one, and
// otherwise how far it has got (satellites seen, sentences read).
DEKI_CATEGORY("Demo")
DEKI_DESCRIPTION("Shows the GPS position, satellites and time, or how the search is going.")
class GpsDemo : public Deki::Component
{
public:
    DEKI_EXPORT
    DEKI_TOOLTIP("One line: whether there is a fix.")
    Deki::ObjectRef<Deki2D::TextComponent> status;

    DEKI_EXPORT
    DEKI_TOOLTIP("Several lines: position, satellites, altitude, time, receiver.")
    Deki::ObjectRef<Deki2D::TextComponent> details;

    GpsDemo() = default;

    void Start() override;
    void Update() override;

private:
    float m_SinceRefreshMs = 0.0f;

    void Refresh();
};
