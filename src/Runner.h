#pragma once

#include <deki/Engine.h>
#include <deki/reflection/ObjectRef.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace Deki2D
{
class AnimationComponent;
class SpriteComponent;
class TextComponent;
}  // namespace Deki2D

namespace DekiRendering
{
class CameraComponent;
}

namespace DekiTiledMap
{
class TilemapComponent;
}

// Plays an endless side-scrolling run by itself, so the whole screen moves the
// way it does in a real game.
//
// The camera runs right at a steady speed with the player beside it. The
// level ahead is laid out from the pools below as the camera reaches it, the
// same way every time for the same seed: obstacles on the ground with coins
// over them, floating platforms with coins on top, and flyers crossing above.
// What falls behind the screen goes back into its pool. The player jumps by
// itself: over obstacles, and onto platforms.
//
// Speeds and sizes are in pixels at the project's pixels per meter, like the
// other demo components.
DEKI_CATEGORY("Demo")
DEKI_DESCRIPTION("Plays an endless run by itself: scrolls the level, lays out what comes next and jumps the player.")
class Runner : public Deki::Component
{
public:
    DEKI_EXPORT
    DEKI_TOOLTIP("The camera that runs along with the player.")
    Deki::ObjectRef<DekiRendering::CameraComponent> camera;

    DEKI_EXPORT
    DEKI_TOOLTIP("The player. It starts standing on the ground; its height there is the ground's.")
    Deki::ObjectRef<Deki2D::AnimationComponent> player;

    DEKI_EXPORT
    DEKI_TOOLTIP("A far layer that moves slower than the ground, for depth. Optional.")
    Deki::ObjectRef<DekiTiledMap::TilemapComponent> hills;

    DEKI_EXPORT
    DEKI_TOOLTIP("Shows the score. Optional.")
    Deki::ObjectRef<Deki2D::TextComponent> score;

    DEKI_EXPORT
    DEKI_TOOLTIP("Things on the ground the player jumps over, standing where the ground is.")
    std::vector<Deki::ObjectRef<Deki2D::SpriteComponent>> obstacles;

    DEKI_EXPORT
    DEKI_TOOLTIP("Things on the ground the player runs past, standing where the ground is.")
    std::vector<Deki::ObjectRef<Deki2D::SpriteComponent>> decorations;

    DEKI_EXPORT
    DEKI_TOOLTIP("Floating platforms. Each is its leftmost tile, with the other tiles as children to its right.")
    std::vector<Deki::ObjectRef<Deki2D::SpriteComponent>> platforms;

    DEKI_EXPORT
    DEKI_TOOLTIP("Coins the player collects.")
    std::vector<Deki::ObjectRef<Deki2D::SpriteComponent>> coins;

    DEKI_EXPORT
    DEKI_TOOLTIP("Flyers that cross above the player.")
    std::vector<Deki::ObjectRef<Deki2D::AnimationComponent>> flyers;

    DEKI_EXPORT DEKI_SLIDER(0, 240)
        DEKI_TOOLTIP("How fast the camera and the player run, in pixels a second.") float speedPx = 120.0f;

    DEKI_EXPORT DEKI_SLIDER(0, 1) DEKI_TOOLTIP("How much of the camera's movement the hills follow: 0 keeps them "
                                               "still, 1 keeps them on screen.") float hillsFollow = 0.5f;

    DEKI_EXPORT DEKI_SLIDER(0, 600) DEKI_TOOLTIP(
        "How fast the player leaves the ground when it jumps, in pixels a second.") float jumpSpeedPx = 300.0f;

    DEKI_EXPORT DEKI_SLIDER(0, 2000)
        DEKI_TOOLTIP("How fast the player falls, in pixels a second each second.") float gravityPx = 900.0f;

    DEKI_EXPORT
    DEKI_TOOLTIP("The width of one tile of the platforms and the ground things, in pixels.")
    float tilePx = 18.0f;

    DEKI_EXPORT
    DEKI_TOOLTIP("Every this many pixels, the camera and everything on screen move back by the same amount, so "
                 "positions stay small. Keep it a whole number of the ground's width, and of the hills' width "
                 "divided by Hills Follow.")
    float wrapEveryPx = 4608.0f;

    DEKI_EXPORT
    DEKI_TOOLTIP("The player's animation while it runs. It holds still in the air.")
    std::string runAnimation = "run";

    DEKI_EXPORT
    DEKI_TOOLTIP("Picks the level. The same seed lays out the same level every time.")
    uint32_t seed = 1;

    Runner() = default;

    void Start() override;
    void Update() override;

private:
    struct Platform
    {
        Deki::Object* object = nullptr;
        float widthM = 0.0f;
    };

    void LayOutAhead(float rightM);
    void ParkBehind(float leftM);
    void Steer(float x, float dtS);
    void Collect(float x, float y);
    void MoveFlyers(float leftM, float dtS);
    void WrapBack();

    void PlaceObstacle(float x);
    void PlacePlatform(float x);
    void PlaceDecoration(float x);
    void PlaceCoin(float x, float y);
    void LaunchFlyer(float x);

    float Random01();
    float HalfViewWidth() const;
    // A random one of `count` pool items that `isActive` says are free, or -1.
    template <typename IsActive>
    int PickFree(size_t count, IsActive isActive)
    {
        m_FreeScratch.clear();
        for (size_t i = 0; i < count; ++i)
        {
            if (!isActive(i))
            {
                m_FreeScratch.push_back(static_cast<int>(i));
            }
        }
        if (m_FreeScratch.empty())
        {
            return -1;
        }
        const auto pick = static_cast<size_t>(Random01() * static_cast<float>(m_FreeScratch.size()));
        return m_FreeScratch[std::min(pick, m_FreeScratch.size() - 1)];
    }

    std::vector<Deki::Object*> m_Obstacles;
    std::vector<Deki::Object*> m_Decorations;
    std::vector<Platform> m_Platforms;
    std::vector<Deki::Object*> m_Coins;
    std::vector<Deki::Object*> m_Flyers;
    std::vector<float> m_CoinBaseY;
    std::vector<float> m_FlyerBaseY;
    std::vector<int> m_FreeScratch;

    float m_Ppm = 16.0f;
    float m_TileM = 1.125f;
    float m_StartCameraX = 0.0f;
    float m_PlayerOffsetX = 0.0f;
    float m_GroundY = 0.0f;
    float m_GroundTop = 0.0f;
    float m_HillsBaseX = 0.0f;
    float m_NextX = 0.0f;
    float m_PlayerY = 0.0f;
    float m_PlayerVy = 0.0f;
    float m_ElapsedS = 0.0f;
    int m_OnPlatform = -1;
    bool m_OnGround = true;
    uint32_t m_Random = 1;
    uint32_t m_Score = 0;
    uint32_t m_ShownScore = UINT32_MAX;
};
