#include "Runner.h"

#include "AnimationComponent.h"
#include "CameraComponent.h"
#include "SpriteComponent.h"
#include "TextComponent.h"
#include "TilemapComponent.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace
{
// Sizes and timings in pixels and seconds, at the project's pixels per meter.
// The player's half width, for touching things.
constexpr float kBodyHalfWidthPx = 8.0f;
// How far before an obstacle, and before a platform starts, the player jumps.
constexpr float kObstacleLeadPx = 24.0f;
constexpr float kPlatformLeadPx = 40.0f;
// A coin this close to the player is collected.
constexpr float kCoinReachXPx = 14.0f;
constexpr float kCoinReachYPx = 18.0f;
constexpr float kCoinBobPx = 3.0f;
constexpr float kCoinBobHz = 0.6f;
// Flyers move left through the level this fast.
constexpr float kFlyerSpeedPx = 40.0f;
constexpr float kFlyerBobPx = 4.0f;
// A longer frame moves things as if it took this long.
constexpr float kMaxStepS = 0.05f;
constexpr uint32_t kCoinPoints = 10;
}  // namespace

void Runner::Start()
{
    m_Ppm = Deki::EngineSettings::Global().pixelsPerMeter;
    m_TileM = tilePx / m_Ppm;
    m_Random = seed != 0 ? seed : 1;

    Deki::Object* cameraObject = camera.GetObject();
    Deki::Object* playerObject = player.GetObject();
    if (!cameraObject || !playerObject)
    {
        return;
    }
    m_StartCameraX = cameraObject->GetX();
    m_PlayerOffsetX = playerObject->GetX() - m_StartCameraX;
    m_GroundY = playerObject->GetY();
    m_PlayerY = m_GroundY;
    if (Deki::Object* hillsObject = hills.GetObject())
    {
        m_HillsBaseX = hillsObject->GetX();
    }

    auto collect = [](const auto& refs, std::vector<Deki::Object*>& out)
    {
        for (const auto& ref : refs)
        {
            if (Deki::Object* object = ref.GetObject())
            {
                object->SetActive(false);
                out.push_back(object);
            }
        }
    };
    collect(obstacles, m_Obstacles);
    collect(decorations, m_Decorations);
    collect(coins, m_Coins);
    collect(flyers, m_Flyers);
    m_CoinBaseY.assign(m_Coins.size(), 0.0f);
    m_FlyerBaseY.assign(m_Flyers.size(), 0.0f);

    // Things on the ground stand on it, so the first one tells where its top is.
    m_GroundTop = m_GroundY;
    if (!m_Obstacles.empty())
    {
        m_GroundTop = m_Obstacles[0]->GetY() - m_TileM * 0.5f;
    }

    for (const auto& ref : platforms)
    {
        Deki::Object* object = ref.GetObject();
        if (!object)
        {
            continue;
        }
        float right = 0.0f;
        for (Deki::Object* child : object->GetChildren())
        {
            right = std::max(right, child->GetX());
        }
        object->SetActive(false);
        m_Platforms.push_back({ object, right + m_TileM });
    }
    m_FreeScratch.reserve(std::max({ m_Obstacles.size(), m_Decorations.size(), m_Platforms.size() }));

    if (Deki2D::AnimationComponent* anim = player.Get())
    {
        anim->PlayAnimation(runAnimation.c_str());
    }
    for (const auto& ref : flyers)
    {
        if (Deki2D::AnimationComponent* anim = ref.Get())
        {
            anim->Play();
        }
    }

    // A short run before the first thing to jump.
    m_NextX = playerObject->GetX() + 120.0f / m_Ppm;
}

void Runner::Update()
{
    Deki::Object* cameraObject = camera.GetObject();
    Deki::Object* playerObject = player.GetObject();
    if (!cameraObject || !playerObject)
    {
        return;
    }

    const float dtS = std::min(Deki::Time::GetDeltaTimeF() * 0.001f, kMaxStepS);
    m_ElapsedS += dtS;

    const float cameraX = cameraObject->GetX() + (speedPx / m_Ppm) * dtS;
    cameraObject->SetX(cameraX);
    if (Deki::Object* hillsObject = hills.GetObject())
    {
        hillsObject->SetX(m_HillsBaseX + (cameraX - m_StartCameraX) * hillsFollow);
    }

    // A tile past each edge, so things come and go off screen.
    const float half = HalfViewWidth();
    const float leftM = cameraX - half - m_TileM;
    const float rightM = cameraX + half + m_TileM;
    ParkBehind(leftM);
    LayOutAhead(rightM);

    const float x = cameraX + m_PlayerOffsetX;
    Steer(x, dtS);
    playerObject->SetX(x);
    playerObject->SetY(m_PlayerY);
    Collect(x, m_PlayerY);
    MoveFlyers(leftM, dtS);

    for (size_t i = 0; i < m_Coins.size(); ++i)
    {
        if (m_Coins[i]->IsActive())
        {
            const float wave = std::sin((m_ElapsedS * kCoinBobHz + static_cast<float>(i) * 0.11f) * 6.283185307f);
            m_Coins[i]->SetY(m_CoinBaseY[i] + (kCoinBobPx / m_Ppm) * wave);
        }
    }

    if (m_Score != m_ShownScore)
    {
        if (Deki2D::TextComponent* text = score.Get())
        {
            char line[16];
            std::snprintf(line, sizeof(line), "%u", static_cast<unsigned>(m_Score));
            text->SetText(line);
        }
        m_ShownScore = m_Score;
    }

    if (wrapEveryPx > 0.0f && cameraX - m_StartCameraX >= wrapEveryPx / m_Ppm)
    {
        WrapBack();
    }
}

void Runner::LayOutAhead(float rightM)
{
    while (m_NextX < rightM)
    {
        const float x = m_NextX;
        const float pick = Random01();
        if (pick < 0.4f)
        {
            PlaceObstacle(x);
        }
        else if (pick < 0.7f)
        {
            PlacePlatform(x);
        }
        else if (pick < 0.9f)
        {
            PlaceDecoration(x);
        }
        else
        {
            m_NextX = x + 4.0f * m_TileM;
        }
        if (Random01() < 0.3f)
        {
            LaunchFlyer(rightM + Random01() * 4.0f * m_TileM);
        }
    }
}

void Runner::PlaceObstacle(float x)
{
    const int index = PickFree(m_Obstacles.size(), [&](size_t i) { return m_Obstacles[i]->IsActive(); });
    if (index < 0)
    {
        m_NextX = x + 4.0f * m_TileM;
        return;
    }
    const float centerX = x + m_TileM * 0.5f;
    Deki::Object* obstacle = m_Obstacles[static_cast<size_t>(index)];
    obstacle->SetX(centerX);
    obstacle->SetActive(true);

    // Coins along the top of the jump.
    PlaceCoin(centerX - 20.0f / m_Ppm, m_GroundY + 38.0f / m_Ppm);
    PlaceCoin(centerX, m_GroundY + 46.0f / m_Ppm);
    PlaceCoin(centerX + 20.0f / m_Ppm, m_GroundY + 38.0f / m_Ppm);
    m_NextX = x + (8.0f + std::floor(Random01() * 4.0f)) * m_TileM;
}

void Runner::PlacePlatform(float x)
{
    const int index = PickFree(m_Platforms.size(), [&](size_t i) { return m_Platforms[i].object->IsActive(); });
    if (index < 0)
    {
        m_NextX = x + 4.0f * m_TileM;
        return;
    }
    const Platform& platform = m_Platforms[static_cast<size_t>(index)];

    // Its top 30, 36 or 42 pixels above the ground: high enough to stand out,
    // low enough to land on from a jump.
    const float risePx = 30.0f + 6.0f * std::floor(Random01() * 3.0f);
    const float topY = m_GroundTop + risePx / m_Ppm;
    platform.object->SetX(x + m_TileM * 0.5f);
    platform.object->SetY(topY - m_TileM * 0.5f);
    platform.object->SetActive(true);

    // A coin over each tile, where the player runs along it.
    const float standY = topY + (m_GroundY - m_GroundTop);
    const int tiles = static_cast<int>(std::lround(platform.widthM / m_TileM));
    for (int i = 0; i < tiles; ++i)
    {
        PlaceCoin(x + (static_cast<float>(i) + 0.5f) * m_TileM, standY);
    }
    m_NextX = x + platform.widthM + (5.0f + std::floor(Random01() * 3.0f)) * m_TileM;
}

void Runner::PlaceDecoration(float x)
{
    const int index = PickFree(m_Decorations.size(), [&](size_t i) { return m_Decorations[i]->IsActive(); });
    if (index >= 0)
    {
        Deki::Object* decoration = m_Decorations[static_cast<size_t>(index)];
        decoration->SetX(x + m_TileM * 0.5f);
        decoration->SetActive(true);
    }
    for (int i = 0; i < 3; ++i)
    {
        PlaceCoin(x + (static_cast<float>(i) + 2.5f) * m_TileM, m_GroundY);
    }
    m_NextX = x + (6.0f + std::floor(Random01() * 2.0f)) * m_TileM;
}

void Runner::PlaceCoin(float x, float y)
{
    for (size_t i = 0; i < m_Coins.size(); ++i)
    {
        if (!m_Coins[i]->IsActive())
        {
            m_Coins[i]->SetX(x);
            m_Coins[i]->SetY(y);
            m_Coins[i]->SetActive(true);
            m_CoinBaseY[i] = y;
            return;
        }
    }
}

void Runner::LaunchFlyer(float x)
{
    for (size_t i = 0; i < m_Flyers.size(); ++i)
    {
        if (!m_Flyers[i]->IsActive())
        {
            // Above the top of a jump, so the player never meets one.
            m_FlyerBaseY[i] = m_GroundY + (72.0f + Random01() * 28.0f) / m_Ppm;
            m_Flyers[i]->SetX(x);
            m_Flyers[i]->SetY(m_FlyerBaseY[i]);
            m_Flyers[i]->SetActive(true);
            return;
        }
    }
}

void Runner::ParkBehind(float leftM)
{
    const float halfTile = m_TileM * 0.5f;
    auto park = [&](std::vector<Deki::Object*>& pool)
    {
        for (Deki::Object* object : pool)
        {
            if (object->IsActive() && object->GetX() + halfTile < leftM)
            {
                object->SetActive(false);
            }
        }
    };
    park(m_Obstacles);
    park(m_Decorations);
    park(m_Coins);
    for (const Platform& platform : m_Platforms)
    {
        if (platform.object->IsActive() && platform.object->GetX() - halfTile + platform.widthM < leftM)
        {
            platform.object->SetActive(false);
        }
    }
}

void Runner::Steer(float x, float dtS)
{
    const float halfTile = m_TileM * 0.5f;
    const float body = kBodyHalfWidthPx / m_Ppm;
    const float standAbove = m_GroundY - m_GroundTop;

    if (m_OnGround)
    {
        bool jump = false;
        if (m_OnPlatform < 0)
        {
            for (Deki::Object* obstacle : m_Obstacles)
            {
                const float gap = (obstacle->GetX() - halfTile) - (x + body);
                jump = jump || (obstacle->IsActive() && gap > 0.0f && gap <= kObstacleLeadPx / m_Ppm);
            }
            for (const Platform& platform : m_Platforms)
            {
                const float gap = (platform.object->GetX() - halfTile) - x;
                jump = jump || (platform.object->IsActive() && gap > 0.0f && gap <= kPlatformLeadPx / m_Ppm);
            }
        }
        else
        {
            const Platform& platform = m_Platforms[static_cast<size_t>(m_OnPlatform)];
            const float right = platform.object->GetX() - halfTile + platform.widthM;
            if (x - body > right)
            {
                // Ran off the end: fall.
                m_OnGround = false;
                m_OnPlatform = -1;
                m_PlayerVy = 0.0f;
            }
        }
        if (jump)
        {
            m_OnGround = false;
            m_PlayerVy = jumpSpeedPx / m_Ppm;
            if (Deki2D::AnimationComponent* anim = player.Get())
            {
                anim->Pause();
            }
        }
    }
    if (m_OnGround)
    {
        return;
    }

    const float previousY = m_PlayerY;
    m_PlayerVy -= (gravityPx / m_Ppm) * dtS;
    m_PlayerY += m_PlayerVy * dtS;
    if (m_PlayerVy > 0.0f)
    {
        return;
    }

    // Platforms are landed on from above and passed through from below.
    float landY = m_GroundY;
    int landOn = -1;
    for (size_t i = 0; i < m_Platforms.size(); ++i)
    {
        const Platform& platform = m_Platforms[i];
        const float left = platform.object->GetX() - halfTile;
        const float standY = platform.object->GetY() + halfTile + standAbove;
        if (platform.object->IsActive() && x + body >= left && x - body <= left + platform.widthM &&
            previousY >= standY && m_PlayerY <= standY && standY > landY)
        {
            landY = standY;
            landOn = static_cast<int>(i);
        }
    }
    if (m_PlayerY <= landY)
    {
        m_PlayerY = landY;
        m_PlayerVy = 0.0f;
        m_OnGround = true;
        m_OnPlatform = landOn;
        if (Deki2D::AnimationComponent* anim = player.Get())
        {
            anim->Resume();
        }
    }
}

void Runner::Collect(float x, float y)
{
    const float reachX = kCoinReachXPx / m_Ppm;
    const float reachY = kCoinReachYPx / m_Ppm;
    for (Deki::Object* coin : m_Coins)
    {
        if (coin->IsActive() && std::fabs(coin->GetX() - x) < reachX && std::fabs(coin->GetY() - y) < reachY)
        {
            coin->SetActive(false);
            m_Score += kCoinPoints;
        }
    }
}

void Runner::MoveFlyers(float leftM, float dtS)
{
    const float step = (kFlyerSpeedPx / m_Ppm) * dtS;
    for (size_t i = 0; i < m_Flyers.size(); ++i)
    {
        Deki::Object* flyer = m_Flyers[i];
        if (!flyer->IsActive())
        {
            continue;
        }
        const float x = flyer->GetX() - step;
        if (x + m_TileM < leftM)
        {
            flyer->SetActive(false);
            continue;
        }
        const float wave = std::sin((m_ElapsedS * 0.8f + static_cast<float>(i) * 0.5f) * 6.283185307f);
        flyer->SetX(x);
        flyer->SetY(m_FlyerBaseY[i] + (kFlyerBobPx / m_Ppm) * wave);
    }
}

void Runner::WrapBack()
{
    const float shift = wrapEveryPx / m_Ppm;
    auto move = [shift](Deki::Object* object) { object->SetX(object->GetX() - shift); };
    move(camera.GetObject());
    move(player.GetObject());
    for (Deki::Object* object : m_Obstacles)
    {
        move(object);
    }
    for (Deki::Object* object : m_Decorations)
    {
        move(object);
    }
    for (Deki::Object* object : m_Coins)
    {
        move(object);
    }
    for (Deki::Object* object : m_Flyers)
    {
        move(object);
    }
    for (const Platform& platform : m_Platforms)
    {
        move(platform.object);
    }
    m_NextX -= shift;
    if (Deki::Object* hillsObject = hills.GetObject())
    {
        const float cameraX = camera.GetObject()->GetX();
        hillsObject->SetX(m_HillsBaseX + (cameraX - m_StartCameraX) * hillsFollow);
    }
}

float Runner::Random01()
{
    // xorshift32: the same sequence on every chip and compiler.
    m_Random ^= m_Random << 13;
    m_Random ^= m_Random >> 17;
    m_Random ^= m_Random << 5;
    return static_cast<float>(m_Random >> 8) * (1.0f / 16777216.0f);
}

float Runner::HalfViewWidth() const
{
    const Deki::Engine& engine = Deki::Engine::GetInstance();
    const DekiRendering::CameraComponent* cam = camera.Get();
    const int32_t w = engine.GetScreenWidth();
    const int32_t h = engine.GetScreenHeight();
    if (!cam || w <= 0 || h <= 0)
    {
        return 20.0f;
    }
    return cam->GetVisibleWidth(w, h) * 0.5f;
}
