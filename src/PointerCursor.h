#pragma once

#include <deki/Engine.h>

#include <cstddef>
#include <cstdint>
#include <string>

// Draws a pointer where an input source's pointer is: the T-Deck's trackball
// in pointer mode. Put it on an object whose children draw the arrow, with the
// arrow's tip at the object's origin. The children are shown only while there
// is a pointer to follow, so on a board without one nothing appears.
DEKI_CATEGORY("Demo")
DEKI_DESCRIPTION("Moves this object to an input source's pointer, and shows its children only while there is one.")
class PointerCursor : public Deki::Component
{
public:
    DEKI_EXPORT
    DEKI_TOOLTIP("The input source whose pointer to follow, as it was registered: Trackball, SDL3, ...")
    std::string source = "Trackball";

    PointerCursor() = default;
    ~PointerCursor() override;

    void Start() override;
    void Update() override;

private:
    size_t m_Listener = static_cast<size_t>(-1);
    bool m_Have = false;
    int32_t m_X = 0;
    int32_t m_Y = 0;

    void Show(bool visible);
};
