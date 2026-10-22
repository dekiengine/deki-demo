#pragma once

#include <deki/Engine.h>
#include <deki/reflection/ObjectRef.h>

#include <cstdint>
#include <string>
#include <vector>

namespace Deki2D
{
class ScrollComponent;
}

// Lays the menu's tiles out for the glass they are on. Put it on the object
// whose children are the tiles, laid out as the grid.
//
// Where a grid tile comes out big enough to read and tap (Min Tile Size, on
// the glass), the grid stays as authored: the T-Deck, a desktop. On a
// physically small screen, such as the 1.91" T-Display S3 AMOLED, the tiles
// move into Strip, a horizontal scroll, drawn a whole number of times bigger
// so they reach that size, and are swiped left and right. Their labels use
// the font baked at that size where the build has it (SharpText).
//
// The size on the glass comes from the board's screen diagonal; a screen of
// unknown size keeps the grid.
DEKI_CATEGORY("Demo")
DEKI_DESCRIPTION("Shows this object's tiles as a grid, or as a scrolling row on a physically small screen.")
class MenuLayout : public Deki::Component
{
public:
    DEKI_EXPORT
    DEKI_TOOLTIP("The horizontal scroll the tiles move into on a small screen. Its Clip child holds them there.")
    Deki::ObjectRef<Deki2D::ScrollComponent> strip;

    DEKI_EXPORT
    DEKI_TOOLTIP("The smallest a tile may be on the glass, in meters (0.009 is 9 mm, a comfortable finger). A grid "
                 "whose tiles come out smaller turns into the row.")
    float minTileSize = 0.009f;

    DEKI_EXPORT
    DEKI_TOOLTIP("Room kept above the row for the title, from the top of the screen.")
    DEKI_UNIT(Distance)
    float topMargin = 3.0f;

    DEKI_EXPORT
    DEKI_TOOLTIP("Room kept below the row.")
    DEKI_UNIT(Distance)
    float bottomMargin = 1.0f;

    DEKI_EXPORT
    DEKI_TOOLTIP("Gap between tiles in the row, and before the first and after the last.")
    DEKI_UNIT(Distance)
    float gap = 0.5f;

    MenuLayout() = default;

    void Start() override;
    void Update() override;

    // How many times bigger the tiles are drawn in the row, or 0 for the grid:
    // the whole multiple that brings a `tileSize` meter tile to `minSize` on
    // glass with `glassPerMeter` meters of glass to a world meter, as long as
    // it fits `room` meters of height. Public for the tests.
    static int StripScale(float tileSize, float glassPerMeter, float minSize, float room);

private:
    // What every object under a tile had as authored, so the grid comes back
    // exactly when the screen changes.
    struct Saved
    {
        Deki::Object* object = nullptr;
        float x = 0.0f, y = 0.0f;
        float scaleX = 1.0f, scaleY = 1.0f;
        float spriteW = 0.0f, spriteH = 0.0f;
        float textW = 0.0f, textH = 0.0f;
        int32_t pixelScale = 1;
        std::string fontGuid;  // the size variant it was authored with
        int fontSize = 0;      // of that variant; 0 unknown
        float colliderW = 0.0f, colliderH = 0.0f;
    };
    std::vector<Deki::Object*> m_Tiles;
    std::vector<Saved> m_Saved;
    float m_TileW = 0.0f, m_TileH = 0.0f;

    int m_Scale = -1;  // -1: not laid out yet, 0: the grid, else the row at that scale
    int32_t m_ScreenW = -1, m_ScreenH = -1;
    float m_GlassPerMeter = -1.0f;

    void SaveTree(Deki::Object* object);
    void Restore();
    void Scale(int k);
    void ShowGrid();
    void ShowStrip(int k, float visibleW, float centerX, float centerY);
};
