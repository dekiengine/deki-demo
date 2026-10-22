#include "MenuLayout.h"

#include "ClipComponent.h"
#include "GlassSize.h"
#include "InputCollider.h"
#include "ScrollComponent.h"
#include "ScrollElement.h"
#include "SharpText.h"
#include "SpriteComponent.h"
#include "TextComponent.h"

#include <algorithm>
#include <cmath>

int MenuLayout::StripScale(float tileSize, float glassPerMeter, float minSize, float room)
{
    if (tileSize <= 0.0f || glassPerMeter <= 0.0f || minSize <= 0.0f)
    {
        return 0;  // a screen of unknown size keeps the grid
    }
    const float onGlass = tileSize * glassPerMeter;
    if (onGlass >= minSize)
    {
        return 0;
    }
    // Whole multiples keep the tiles' art and text crisp. The small bias keeps
    // an exact fit (2.0000001) at 2.
    int k = static_cast<int>(std::ceil(minSize / onGlass - 1e-4f));
    // As big as reaches the size, but never taller than the room left.
    while (k > 1 && tileSize * static_cast<float>(k) > room)
    {
        --k;
    }
    return k >= 2 ? k : 0;
}

void MenuLayout::Start()
{
    m_Tiles.assign(GetOwner()->GetChildren().begin(), GetOwner()->GetChildren().end());
    for (Deki::Object* tile : m_Tiles)
    {
        SaveTree(tile);
    }
    // Every tile is the same size; the first one's sprite says it.
    if (!m_Tiles.empty())
    {
        if (auto* sprite = m_Tiles.front()->GetComponent<Deki2D::SpriteComponent>())
        {
            m_TileW = sprite->width;
            m_TileH = sprite->height;
        }
    }
    // Strip stays active until the first Update, which hides it for the
    // grid: an object switched on later never gets Start, and the scroll's
    // Start is what hooks up its drag.
}

void MenuLayout::Update()
{
    Deki::Engine& engine = Deki::Engine::GetInstance();
    DekiRendering::CameraComponent* camera = GlassSize::SceneCamera(GetOwner());
    const int32_t w = engine.GetScreenWidth();
    const int32_t h = engine.GetScreenHeight();
    if (!camera || w <= 0 || h <= 0 || !strip.Get())
    {
        return;
    }
    const float glassPerMeter = GlassSize::PerWorldMeter(*camera);
    // Only when the screen changes: the editor's Play view can switch it
    // while playing.
    if (w == m_ScreenW && h == m_ScreenH && glassPerMeter == m_GlassPerMeter)
    {
        return;
    }
    m_ScreenW = w;
    m_ScreenH = h;
    m_GlassPerMeter = glassPerMeter;

    const float visibleW = camera->GetVisibleWidth(w, h);
    const float visibleH = camera->GetVisibleHeight(w, h);
    const float room = visibleH - topMargin - bottomMargin;
    const int k = StripScale((std::min)(m_TileW, m_TileH), glassPerMeter, minTileSize, room);

    if (k == 0)
    {
        ShowGrid();
        return;
    }
    const float centerY = camera->GetPositionY() + visibleH * 0.5f - topMargin - room * 0.5f;
    ShowStrip(k, visibleW, camera->GetPositionX(), centerY);
}

void MenuLayout::SaveTree(Deki::Object* object)
{
    Saved s;
    s.object = object;
    s.x = object->GetX();
    s.y = object->GetY();
    s.scaleX = object->GetScaleX();
    s.scaleY = object->GetScaleY();
    if (auto* sprite = object->GetComponent<Deki2D::SpriteComponent>())
    {
        s.spriteW = sprite->width;
        s.spriteH = sprite->height;
    }
    if (auto* text = object->GetComponent<Deki2D::TextComponent>())
    {
        s.textW = text->width;
        s.textH = text->height;
        s.pixelScale = text->pixelScale;
        s.fontGuid = text->font.guid;
        s.fontSize = SharpText::VariantSize(*text, s.fontGuid);
    }
    if (auto* collider = object->GetComponent<DekiInput::InputCollider>())
    {
        s.colliderW = collider->width;
        s.colliderH = collider->height;
    }
    m_Saved.push_back(s);
    for (Deki::Object* child : object->GetChildren())
    {
        SaveTree(child);
    }
}

// Every size and offset k times the authored one; 1 is the grid's.
void MenuLayout::Scale(int k)
{
    const float f = static_cast<float>(k);
    for (const Saved& s : m_Saved)
    {
        // A tile's own position belongs to whichever layout holds it.
        const bool isTile = std::find(m_Tiles.begin(), m_Tiles.end(), s.object) != m_Tiles.end();
        if (!isTile)
        {
            s.object->SetLocalPosition(s.x * f, s.y * f);
        }
        if (auto* sprite = s.object->GetComponent<Deki2D::SpriteComponent>())
        {
            if (sprite->renderMode == Deki2D::SpriteRenderMode::Normal)
            {
                // A plain sprite draws its own pixels, scaled with its object.
                s.object->SetScale(s.scaleX * f, s.scaleY * f);
            }
            else
            {
                // A nine-slice grows by its middle, keeping its corners.
                sprite->width = s.spriteW * f;
                sprite->height = s.spriteH * f;
            }
        }
        if (auto* text = s.object->GetComponent<Deki2D::TextComponent>())
        {
            text->width = s.textW * f;
            text->height = s.textH * f;
            SharpText::Scale(*text, s.fontGuid, s.fontSize, s.pixelScale, k);
        }
        if (auto* collider = s.object->GetComponent<DekiInput::InputCollider>())
        {
            collider->width = s.colliderW * f;
            collider->height = s.colliderH * f;
        }
        if (auto* element = s.object->GetComponent<Deki2D::ScrollElement>())
        {
            element->width = s.spriteW * f;
            element->height = s.spriteH * f;
        }
    }
}

void MenuLayout::Restore()
{
    Scale(1);
    for (const Saved& s : m_Saved)
    {
        if (std::find(m_Tiles.begin(), m_Tiles.end(), s.object) != m_Tiles.end())
        {
            s.object->SetLocalPosition(s.x, s.y);
        }
    }
}

void MenuLayout::ShowGrid()
{
    if (m_Scale == 0)
    {
        return;  // already the grid
    }
    m_Scale = 0;
    for (Deki::Object* tile : m_Tiles)
    {
        tile->SetParent(GetOwner());
        tile->SetActive(true);  // the scroll hides the tiles out of its view
    }
    Restore();
    strip.Get()->GetOwner()->SetActive(false);
}

void MenuLayout::ShowStrip(int k, float visibleW, float centerX, float centerY)
{
    Deki2D::ScrollComponent* scroll = strip.Get();
    Deki::Object* stripObj = scroll->GetOwner();
    Deki::Object* clipObj = stripObj->FindChildByName("Clip");
    if (!clipObj)
    {
        return;
    }
    m_Scale = k;
    const float tileH = m_TileH * static_cast<float>(k);

    stripObj->SetActive(true);
    stripObj->SetWorldPosition(centerX, centerY);
    if (auto* clip = clipObj->GetComponent<Deki2D::ClipComponent>())
    {
        clip->width = visibleW;
        clip->height = tileH;
    }
    if (DekiInput::InputCollider* collider = scroll->inputCollider.Get())
    {
        collider->width = visibleW;
        collider->height = tileH;
    }
    scroll->itemSpacing = gap;
    scroll->paddingLeft = gap;
    scroll->paddingRight = gap;

    Scale(k);
    for (Deki::Object* tile : m_Tiles)
    {
        tile->SetParent(clipObj);
    }
    // The tiles' sizes changed as well as their number, which the scroll
    // alone would not notice.
    scroll->EnsureChildObjects(stripObj);
    scroll->ScrollTo(0.0f, false);
}
