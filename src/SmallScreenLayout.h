#pragma once

#include <deki/Engine.h>
#include <deki/ScreenAnchor.h>
#include <deki/reflection/ObjectRef.h>

#include <cstdint>
#include <string>
#include <vector>

namespace DekiInput
{
class InputCollider;
}

namespace Deki2D
{
class BitmapFont;
class TextComponent;
}

// Lays a demo scene out for a physically small screen, such as the 1.91"
// T-Display S3 AMOLED. Put it on the scene's root. On a screen where the
// scene comes out big enough (the T-Deck, a desktop) it leaves everything as
// authored.
//
// On small glass:
// - On a screen shorter than Small Screen Height, Back becomes a bar down the
//   whole left edge, with its button twice the size in the middle, so it is
//   easy to hit. Everything else anchored to the
//   left moves right to make room. Back's Bar child draws the bar.
// - Text whose capital letters would be smaller than Min Cap Height on the
//   glass is drawn twice the size, where it still fits its box: in the
//   same font baked twice as big, when the font lists that size (and the
//   project force-includes it, as no scene uses it), else pixel-doubled. Texts with
//   the same parent and font change together, so neighbours stay alike.
//   A roller's rows and the menu's tiles keep their own sizes.
// - On a screen shorter than Small Screen Height, the title and subtitle at
//   the top left grow together: the title twice the size, the lines under it
//   half again, stacked so the title stays the bigger one. The frame rate at the top right keeps its size.
//
// Elsewhere Back keeps its place, and its touch area grows to Min Touch Size
// where it falls short.
//
// The size on the glass comes from the board's screen diagonal; on a screen
// of unknown size nothing changes.
DEKI_CATEGORY("Demo")
DEKI_DESCRIPTION("Lays the scene out for a physically small screen: Back as a side bar, and bigger text.")
class SmallScreenLayout : public Deki::Component
{
public:
    DEKI_EXPORT
    DEKI_TOOLTIP("The Back button. On a small screen it becomes a bar down the left edge.")
    Deki::ObjectRef<DekiInput::InputCollider> back;

    DEKI_EXPORT
    DEKI_TOOLTIP("A screen shorter than this on the glass, in meters, gets Back as a bar down its left edge (0.03 "
                 "is 30 mm: the 1.91-inch AMOLED is 20 mm tall, the 2.8-inch T-Deck 43 mm).")
    float smallScreenHeight = 0.03f;

    DEKI_EXPORT
    DEKI_TOOLTIP("The smallest Back's touch area may be on the glass, in meters (0.009 is 9 mm, a comfortable finger).")
    float minTouchSize = 0.009f;

    DEKI_EXPORT
    DEKI_TOOLTIP("The smallest capital letters may be on the glass, in meters (0.001 is 1 mm). Smaller text is "
                 "drawn bigger.")
    float minCapHeight = 0.001f;

    DEKI_EXPORT
    DEKI_TOOLTIP("Room between the bar and the text anchored to the left of the screen.")
    DEKI_UNIT(Distance)
    float gap = 0.5f;

    SmallScreenLayout() = default;

    void Start() override;
    void Update() override;

    // The whole multiple (at least 1) that brings `size` to `minimum`, both on
    // the glass. Public for the tests.
    static int WholeScale(float size, float minimum);

private:
    struct SavedText
    {
        Deki::Object* object = nullptr;
        float width = 0.0f, height = 0.0f;
        int32_t pixelScale = 1;
        float x = 0.0f, y = 0.0f;  // local position
        std::string laidOut;      // the text it was last laid out for
        std::string fontGuid;     // the size variant it was authored with
        const Deki2D::BitmapFont* font = nullptr;
        int fontSize = -1;        // of that variant; -1 until looked up, 0 unknown
    };
    struct SavedAnchor
    {
        Deki::ScreenAnchor* anchor = nullptr;
        Deki::ScreenPoint point = Deki::ScreenPoint::TopLeft;
        float offsetX = 0.0f, offsetY = 0.0f;
    };
    std::vector<SavedText> m_Texts;
    std::vector<SavedText> m_Header;  // the title and subtitle at the top left
    std::vector<SavedAnchor> m_Anchors;  // every anchor but Back's
    SavedAnchor m_BackAnchor;
    float m_BackW = 0.0f, m_BackH = 0.0f;
    float m_BackPad[4] = {};  // left, right, top, bottom
    float m_BackScaleX = 1.0f, m_BackScaleY = 1.0f;

    int32_t m_ScreenW = -1, m_ScreenH = -1;
    float m_GlassPerMeter = -1.0f;

    void Save(Deki::Object* object, bool skipText);
    void Layout(float glassPerMeter, float visibleH);
    // How many times bigger text `t` may be drawn, alone.
    int TextScale(const SavedText& t, float glassPerMeter) const;
    bool TextChanged() const;
    // Draws `t` at `k` times its size, sharp where the build has the font at
    // that size (SharpText).
    void SetTextScale(SavedText& t, int k);
    float LayoutBack(DekiInput::InputCollider& collider, float glassPerMeter, float visibleH);
    void LayoutHeader(float shift);
    const SavedAnchor* FindAnchor(const Deki::ScreenAnchor* anchor) const;
};
