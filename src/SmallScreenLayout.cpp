#include "SmallScreenLayout.h"

#include "BitmapFont.h"
#include "GlassSize.h"
#include "GradientComponent.h"
#include "InputCollider.h"
#include "MenuLayout.h"
#include "RollerComponent.h"
#include "SharpText.h"
#include "TextComponent.h"

#include <algorithm>
#include <map>
#include <tuple>
#include <cmath>

namespace
{
// Text sized by something else: the menu's tiles (MenuLayout) and a roller's
// rows, which are spaced for their text.
bool SizedElsewhere(const Deki::Object* object)
{
    return object->GetComponent<MenuLayout>() != nullptr || object->GetComponent<Deki2D::RollerComponent>() != nullptr;
}

bool IsTopPoint(Deki::ScreenPoint p)
{
    return p == Deki::ScreenPoint::TopLeft || p == Deki::ScreenPoint::Top || p == Deki::ScreenPoint::TopRight;
}

bool IsLeftPoint(Deki::ScreenPoint p)
{
    return p == Deki::ScreenPoint::TopLeft || p == Deki::ScreenPoint::Left || p == Deki::ScreenPoint::BottomLeft;
}
}  // namespace

int SmallScreenLayout::WholeScale(float size, float minimum)
{
    if (size <= 0.0f || size >= minimum)
    {
        return 1;
    }
    // The small bias keeps an exact fit (2.0000001) at 2.
    return static_cast<int>(std::ceil(minimum / size - 1e-4f));
}

void SmallScreenLayout::Start()
{
    DekiInput::InputCollider* collider = back.Get();
    for (Deki::Object* child : GetOwner()->GetChildren())
    {
        Save(child, false);
    }
    if (collider)
    {
        Deki::Object* backObj = collider->GetOwner();
        if (auto* anchor = backObj->GetComponent<Deki::ScreenAnchor>())
        {
            m_BackAnchor = { anchor, anchor->anchor, anchor->offsetX, anchor->offsetY };
            m_Anchors.erase(std::remove_if(m_Anchors.begin(), m_Anchors.end(),
                                           [anchor](const SavedAnchor& a) { return a.anchor == anchor; }),
                            m_Anchors.end());
        }
        m_BackW = collider->width;
        m_BackH = collider->height;
        m_BackPad[0] = collider->paddingLeft;
        m_BackPad[1] = collider->paddingRight;
        m_BackPad[2] = collider->paddingTop;
        m_BackPad[3] = collider->paddingBottom;
        m_BackScaleX = backObj->GetScaleX();
        m_BackScaleY = backObj->GetScaleY();
    }
}

void SmallScreenLayout::Save(Deki::Object* object, bool skipText)
{
    skipText = skipText || SizedElsewhere(object);
    auto* anchor = object->GetComponent<Deki::ScreenAnchor>();
    if (anchor)
    {
        m_Anchors.push_back({ anchor, anchor->anchor, anchor->offsetX, anchor->offsetY });
    }
    // Text pinned to the top is the scene's header: the title and its
    // subtitle at the top left, laid out together (LayoutHeader), and the
    // frame rate at the top right, which keeps its size.
    if (auto* text = object->GetComponent<Deki2D::TextComponent>(); text && !skipText)
    {
        SavedText saved{ object, text->width, text->height, text->pixelScale, object->GetX(), object->GetY(), {},
                         text->font.guid, text->GetFont() };
        if (anchor && anchor->anchor == Deki::ScreenPoint::TopLeft)
        {
            m_Header.push_back(saved);
        }
        else if (!anchor || !IsTopPoint(anchor->anchor))
        {
            m_Texts.push_back(saved);
        }
    }
    for (Deki::Object* child : object->GetChildren())
    {
        Save(child, skipText);
    }
}

void SmallScreenLayout::Update()
{
    Deki::Engine& engine = Deki::Engine::GetInstance();
    DekiRendering::CameraComponent* camera = GlassSize::SceneCamera(GetOwner());
    const int32_t w = engine.GetScreenWidth();
    const int32_t h = engine.GetScreenHeight();
    if (!camera || w <= 0 || h <= 0)
    {
        return;
    }
    const float glassPerMeter = GlassSize::PerWorldMeter(*camera);
    // When the screen changes (the editor's Play view can switch it while
    // playing), or a demo changes a text, which may then fit or not.
    if (w == m_ScreenW && h == m_ScreenH && glassPerMeter == m_GlassPerMeter && !TextChanged())
    {
        return;
    }
    m_ScreenW = w;
    m_ScreenH = h;
    m_GlassPerMeter = glassPerMeter;
    Layout(glassPerMeter, camera->GetVisibleHeight(w, h));
}

bool SmallScreenLayout::TextChanged() const
{
    for (const SavedText& t : m_Texts)
    {
        if (t.object->GetComponent<Deki2D::TextComponent>()->GetText() != t.laidOut)
        {
            return true;
        }
    }
    return false;
}

int SmallScreenLayout::TextScale(const SavedText& t, float glassPerMeter) const
{
    const float artPpm = Deki::EngineSettings::Global().pixelsPerMeter;
    const auto* text = t.object->GetComponent<Deki2D::TextComponent>();
    const Deki2D::BitmapFont* font = t.font;  // the authored size, whatever is drawn now
    if (!font || glassPerMeter <= 0.0f || artPpm <= 0.0f)
    {
        return 1;
    }
    const float cap =
        static_cast<float>(font->GetCapHeight()) / artPpm * static_cast<float>(t.pixelScale) * glassPerMeter;
    if (WholeScale(cap, minCapHeight) < 2)
    {
        return 1;
    }
    // Twice the size, if the widest line still fits the box.
    const std::string& s = text->GetText();
    int32_t widest = 0;
    for (size_t begin = 0; begin <= s.size();)
    {
        size_t end = s.find('\n', begin);
        if (end == std::string::npos)
        {
            end = s.size();
        }
        widest = (std::max)(widest, font->MeasureWidth(s.c_str() + begin, end - begin));
        begin = end + 1;
    }
    return static_cast<float>(widest * t.pixelScale * 2) <= t.width * artPpm ? 2 : 1;
}

void SmallScreenLayout::Layout(float glassPerMeter, float visibleH)
{
    // Back: as authored where its touch area is big enough on the glass, else
    // a bar down the left edge. `shift` is how far what was to its right moves.
    float shift = 0.0f;
    if (DekiInput::InputCollider* collider = back.Get())
    {
        shift = LayoutBack(*collider, glassPerMeter, visibleH);
    }

    // Text: twice the size where its capital letters would be under
    // minCapHeight on the glass and it fits, the same for every text with the
    // same parent and font.
    using Group = std::tuple<const Deki::Object*, const Deki2D::BitmapFont*, int32_t>;
    std::map<Group, int> groupScale;
    auto groupOf = [](const SavedText& t)
    {
        return Group{ t.object->GetParent(), t.font, t.pixelScale };
    };
    for (const SavedText& t : m_Texts)
    {
        const int k = TextScale(t, glassPerMeter);
        auto [it, added] = groupScale.emplace(groupOf(t), k);
        if (!added)
        {
            it->second = (std::min)(it->second, k);
        }
    }

    for (SavedText& t : m_Texts)
    {
        auto* text = t.object->GetComponent<Deki2D::TextComponent>();
        t.laidOut = text->GetText();
        const int k = groupScale[groupOf(t)];
        const float f = static_cast<float>(k);
        SetTextScale(t, k);
        text->width = t.width * f;
        text->height = t.height * f;

        // It grows from the side it is aligned to.
        const float grow = t.width * (f - 1.0f) * 0.5f;
        const float dx = text->align == Deki2D::TextAlign::Left    ? grow
                         : text->align == Deki2D::TextAlign::Right ? -grow
                                                                   : 0.0f;
        auto* anchor = t.object->GetComponent<Deki::ScreenAnchor>();
        if (const SavedAnchor* saved = anchor ? FindAnchor(anchor) : nullptr)
        {
            anchor->offsetX = saved->offsetX + (IsLeftPoint(saved->point) ? shift : 0.0f) + dx;
        }
        else if (!anchor)
        {
            t.object->SetLocalPosition(t.x + dx, t.y);
        }
    }

    LayoutHeader(shift);

    // Every other anchored object moves with the bar alone.
    auto owns = [](const std::vector<SavedText>& texts, const Deki::Object* object)
    { return std::any_of(texts.begin(), texts.end(), [object](const SavedText& t) { return t.object == object; }); };
    for (const SavedAnchor& a : m_Anchors)
    {
        const Deki::Object* object = a.anchor->GetOwner();
        if (!owns(m_Texts, object) && !owns(m_Header, object))
        {
            a.anchor->offsetX = a.offsetX + (IsLeftPoint(a.point) ? shift : 0.0f);
        }
    }
}

void SmallScreenLayout::LayoutHeader(float shift)
{
    // On a short screen the title is drawn twice the size and the lines under
    // it half again as big, so the title stays the bigger one and the header
    // still clears what is below it. Each keeps its left edge, the topmost its
    // top edge, and every line moves down by what the lines above it grew.
    const float screenH = Deki::Engine::GetInstance().GetScreenHeightMeters();
    const bool small = screenH > 0.0f && screenH < smallScreenHeight;
    const float artPpm = Deki::EngineSettings::Global().pixelsPerMeter;

    std::vector<SavedText*> lines;
    for (SavedText& t : m_Header)
    {
        lines.push_back(&t);
    }
    auto offsetY = [this](const SavedText* t)
    {
        const SavedAnchor* saved = FindAnchor(t->object->GetComponent<Deki::ScreenAnchor>());
        return saved ? saved->offsetY : 0.0f;
    };
    std::sort(lines.begin(), lines.end(), [&](const SavedText* a, const SavedText* b) { return offsetY(a) > offsetY(b); });

    float below = 0.0f;  // how far the lines above have pushed this one down
    for (SavedText* t : lines)
    {
        auto* text = t->object->GetComponent<Deki2D::TextComponent>();
        auto* anchor = t->object->GetComponent<Deki::ScreenAnchor>();
        const SavedAnchor* saved = FindAnchor(anchor);
        float f = 1.0f;
        if (t == lines.front())
        {
            SetTextScale(*t, small ? 2 : 1);
            f = small ? 2.0f : 1.0f;
        }
        else
        {
            if (t->fontSize < 0)
            {
                t->fontSize = SharpText::VariantSize(*text, t->fontGuid);
            }
            const int target = small ? (t->fontSize * 3) / 2 : t->fontSize;
            if (SharpText::ScaleTo(*text, t->fontGuid, t->fontSize, t->pixelScale, target) && t->fontSize > 0)
            {
                f = static_cast<float>(target) / static_cast<float>(t->fontSize);
            }
        }
        text->width = t->width * f;
        text->height = t->height * f;
        const float cap = t->font && artPpm > 0.0f
                              ? static_cast<float>(t->font->GetCapHeight() * t->pixelScale) / artPpm
                              : 0.0f;
        const float grow = cap * (f - 1.0f);
        if (saved)
        {
            const float dx = text->align == Deki2D::TextAlign::Left ? t->width * (f - 1.0f) * 0.5f : 0.0f;
            anchor->offsetX = saved->offsetX + shift + dx;
            anchor->offsetY = saved->offsetY - below - grow * 0.5f;
        }
        below += grow;
    }
}

void SmallScreenLayout::SetTextScale(SavedText& t, int k)
{
    auto* text = t.object->GetComponent<Deki2D::TextComponent>();
    if (t.fontSize < 0)
    {
        t.fontSize = SharpText::VariantSize(*text, t.fontGuid);
    }
    SharpText::Scale(*text, t.fontGuid, t.fontSize, t.pixelScale, k);
}

const SmallScreenLayout::SavedAnchor* SmallScreenLayout::FindAnchor(const Deki::ScreenAnchor* anchor) const
{
    for (const SavedAnchor& a : m_Anchors)
    {
        if (a.anchor == anchor)
        {
            return &a;
        }
    }
    return nullptr;
}

float SmallScreenLayout::LayoutBack(DekiInput::InputCollider& collider, float glassPerMeter, float visibleH)
{
    Deki::Object* backObj = collider.GetOwner();
    Deki::Object* bar = backObj->FindChildByName("Bar");
    Deki::ScreenAnchor* backAnchor = m_BackAnchor.anchor;

    // The bar is for a short screen, where there is room at the side but
    // little above and below; elsewhere Back keeps its place.
    const float screenH = Deki::Engine::GetInstance().GetScreenHeightMeters();
    const bool small = screenH > 0.0f && screenH < smallScreenHeight;

    if (!small)
    {
        // As authored, the touch area grown to minTouchSize where it falls short.
        const float minWorld = glassPerMeter > 0.0f ? minTouchSize / glassPerMeter : 0.0f;
        auto pad = [](float size, float authored, float minimum)
        { return (std::max)(authored, (minimum - size) * 0.5f); };
        collider.width = m_BackW;
        collider.height = m_BackH;
        collider.paddingLeft = pad(m_BackW, m_BackPad[0], minWorld);
        collider.paddingRight = pad(m_BackW, m_BackPad[1], minWorld);
        collider.paddingTop = pad(m_BackH, m_BackPad[2], minWorld);
        collider.paddingBottom = pad(m_BackH, m_BackPad[3], minWorld);
        backObj->SetScale(m_BackScaleX, m_BackScaleY);
        if (backAnchor)
        {
            backAnchor->anchor = m_BackAnchor.point;
            backAnchor->offsetX = m_BackAnchor.offsetX;
            backAnchor->offsetY = m_BackAnchor.offsetY;
        }
        if (bar)
        {
            bar->SetActive(false);
        }
        return 0.0f;
    }

    // A bar down the left edge, the button twice the size in its middle.
    constexpr float kScale = 2.0f;
    const float barW = m_BackW * kScale;
    backObj->SetScale(m_BackScaleX * kScale, m_BackScaleY * kScale);
    if (backAnchor)
    {
        backAnchor->anchor = Deki::ScreenPoint::Left;
        backAnchor->offsetX = barW * 0.5f;
        backAnchor->offsetY = 0.0f;
    }
    // The whole bar takes the tap; the screen's edge stops a finger that
    // overshoots, so it needs no padding.
    collider.width = barW;
    collider.height = visibleH;
    collider.paddingLeft = collider.paddingRight = collider.paddingTop = collider.paddingBottom = 0.0f;
    if (bar)
    {
        // Bar sits under Back, so the scale applies to it too.
        if (auto* fill = bar->GetComponent<Deki2D::GradientComponent>())
        {
            fill->width = barW / kScale;
            fill->height = visibleH / kScale;
        }
        bar->SetActive(true);
    }

    // What was to the right of Back moves right of the bar.
    const float backRight = m_BackAnchor.offsetX + m_BackW * 0.5f;
    return (std::max)(0.0f, barW + gap - backRight);
}
