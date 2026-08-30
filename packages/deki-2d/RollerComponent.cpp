#include "RollerComponent.h"
#include "deki-input/InputCollider.h"
#include "TextComponent.h"
#include "SpriteComponent.h"
#include "DekiObject.h"
#include "DekiEngine.h"
#include "DekiLogSystem.h"
#include "deki-rendering/CameraComponent.h"
#include "ClipComponent.h"
#include "deki-rendering/QuadBlit.h"
#include "BitmapFont.h"
#include "Texture2D.h"
#include "Sprite.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>

// All sizes/positions in this file are world meters (matches the engine's
// meters-internal convention; transform x/y and pointer callbacks are
// meters too). No pixels-per-meter conversions needed.

// ============================================================================
// Component Registration
// ============================================================================
// NOTE: s_Properties[] and s_ComponentMeta are now auto-generated in
// RollerComponent.gen.h (included at end of RollerComponent.h)


RollerComponent::RollerComponent()
    : DekiBehaviour(),
      width(static_cast<float>(6.25f)),
      item_height(static_cast<float>(1.875f)),
      visible_rows(3),
      infinite_scroll(false),
      reverse_drag(false),
      selected_index(0),
      deceleration(static_cast<float>(0.95f)),
      snap_speed(static_cast<float>(8.0f)),
      selected_item_height(static_cast<float>(2.5f)),
      selected_color(255, 255, 255),
      normal_color(128, 128, 128),
#ifdef DEKI_EDITOR
      selected_font_size(24),
      normal_font_size(16),
#endif
      m_ScrollOffset(0.0f),
      m_ScrollVelocity(0.0f),
      m_TargetOffset(0.0f),
      m_IsSnapping(false),
      m_NeedsSync(false),
      m_IsDragging(false),
      m_LastTouchY(0.0f),
      m_TouchStartY(0.0f),
      m_TouchStartOffset(0.0f),
      m_LastSyncedSelectedItemHeight(-1.0f),
      m_LastSyncedItemHeight(-1.0f)
{
}

RollerComponent::~RollerComponent()
{
    // Child objects are owned by the parent DekiObject, not by us
    // Just clear our references
    m_ClipObj = nullptr;
    m_BackgroundObj = nullptr;
    m_SelectionObj = nullptr;
    m_TextRowObjs.clear();
}

DekiObject* RollerComponent::FindOrCreateChild(DekiObject* owner, const char* name, const char* componentType)
{
    if (!owner) return nullptr;

    // Search existing children by name
    for (auto* child : owner->GetChildren())
    {
        if (child->GetName() == name)
            return child;
    }

#ifdef DEKI_EDITOR
    // Create new child object (editor only - runtime loads from scene)
    DekiObject* child = new DekiObject(name);
    owner->AddChild(child);
    child->AddComponent(componentType);
    return child;
#else
    // Runtime: child should exist from scene loading
    return nullptr;
#endif
}

void RollerComponent::UpdateTextRowCount(DekiObject* owner)
{
    if (!owner) return;

    // Find existing text rows
    m_TextRowObjs.clear();
    for (auto* child : owner->GetChildren())
    {
        if (child->GetName().rfind("TextRow", 0) == 0)
        {
            child->SetActive(true);  // Re-enable in case it was hidden
            m_TextRowObjs.push_back(child);
        }
    }

    // Sort by name to ensure correct order (TextRow0, TextRow1, etc.)
    std::sort(m_TextRowObjs.begin(), m_TextRowObjs.end(),
        [](DekiObject* a, DekiObject* b) {
            return a->GetName() < b->GetName();
        });

    // Create missing rows (visible_rows + 2 for smooth scrolling: 1 extra top + 1 extra bottom)
    int32_t totalRows = visible_rows + 2;

    // Find a template text component to copy font settings from
    TextComponent* templateTC = nullptr;
    for (auto* existingRow : m_TextRowObjs)
    {
        if (existingRow)
        {
            if (auto* tc = existingRow->GetComponent<TextComponent>())
            {
                if (!tc->font.guid.empty() || !tc->font.source.empty())
                {
                    templateTC = tc;
                    break;
                }
            }
        }
    }

    while (static_cast<int32_t>(m_TextRowObjs.size()) < totalRows)
    {
        std::string name = "TextRow" + std::to_string(m_TextRowObjs.size());
        DekiObject* row = FindOrCreateChild(owner, name.c_str(), "TextComponent");
        if (row)
        {
            // Set default text component properties
            if (auto* tc = row->GetComponent<TextComponent>())
            {
                tc->align = TextAlign::Center;
                tc->verticalAlign = TextVerticalAlign::Middle;

                // Copy font settings from template if available
                if (templateTC)
                {
                    tc->font = templateTC->font;
#ifdef DEKI_EDITOR
                    tc->fontSize = templateTC->fontSize;
#endif
                    tc->color = templateTC->color;
                }
            }
            m_TextRowObjs.push_back(row);
        }
    }

    // Hide excess rows (don't delete - user may have customized them)
    for (size_t i = totalRows; i < m_TextRowObjs.size(); ++i)
    {
        m_TextRowObjs[i]->SetActive(false);
    }
    // Trim to needed count
    if (m_TextRowObjs.size() > static_cast<size_t>(totalRows))
    {
        m_TextRowObjs.resize(totalRows);
    }
}

void RollerComponent::EnsureChildObjects(DekiObject* owner)
{
    if (!owner) return;

    // Find or create Clip child on owner
    m_ClipObj = FindOrCreateChild(owner, "Clip", "ClipComponent");

    // All other children go inside the Clip object (so they get clipped)
    if (m_ClipObj)
    {
        // Find or create Background child inside Clip
        m_BackgroundObj = FindOrCreateChild(m_ClipObj, "Background", "SpriteComponent");

        // Find or create Selection child inside Clip
        m_SelectionObj = FindOrCreateChild(m_ClipObj, "Selection", "SpriteComponent");

        // Update text row children inside Clip
        UpdateTextRowCount(m_ClipObj);
    }
}

void RollerComponent::SyncChildObjects(DekiObject* owner)
{
    EnsureChildObjects(owner);

    // Sync scroll offset when selected_index or height properties change externally (e.g., from inspector)
    // Only do this when not actively scrolling
    bool indexChanged = (selected_index != m_LastSyncedSelectedIndex);
    bool heightsChanged = (selected_item_height != m_LastSyncedSelectedItemHeight ||
                           item_height != m_LastSyncedItemHeight);

    if (!m_IsDragging && !m_IsSnapping && (indexChanged || heightsChanged))
    {
        m_ScrollOffset = GetItemOffset(selected_index);
        m_LastSyncedSelectedIndex = selected_index;
        m_LastSyncedSelectedItemHeight = selected_item_height;
        m_LastSyncedItemHeight = item_height;
    }

    int32_t centerRow = visible_rows / 2;

    // Sync clip child position (user controls ClipComponent width/height directly)
    if (m_ClipObj)
    {
        m_ClipObj->SetLocalPosition(0.0f, 0.0f);
    }

    // Sync background child position (sprite sizing controlled by user via texture or scale)
    if (m_BackgroundObj)
    {
        m_BackgroundObj->SetLocalPosition(0.0f, 0.0f);
    }

    // Sync selection child position (at center row, Y=0 in center-based coordinates)
    if (m_SelectionObj)
    {
        m_SelectionObj->SetLocalPosition(0.0f, 0.0f);
    }

    // Sync text row children
    // Get the item index that should be shown at the center based on scroll offset
    float scrollOffset = m_ScrollOffset;
    int32_t rawCenteredIndex = GetRawIndexAtOffset(scrollOffset);

    // Use raw (unwrapped) index for offset calculation so subItemOffset stays small
    float snapOffset = GetItemOffset(rawCenteredIndex);
    float subItemOffset = ((scrollOffset) - (snapOffset));

    // We have visible_rows + 2 text rows for smooth scrolling
    // Row 0 is 1 position above top visible, last row is 1 below bottom visible
    int32_t totalRows = visible_rows + 2;

    for (int32_t row = 0; row < static_cast<int32_t>(m_TextRowObjs.size()) && row < totalRows; row++)
    {
        DekiObject* textObj = m_TextRowObjs[row];
        if (!textObj) continue;

        // Virtual row: row 0 maps to position -1 (above top), row 1 to position 0 (top visible), etc.
        int32_t virtualRow = row - 1;

        // Calculate item index for this row (used for text and styling)
        // Use raw (unwrapped) centered index so offsets stay in the same domain
        int32_t itemIndex = rawCenteredIndex + (virtualRow - centerRow);
        bool validIndex = true;

        if (infinite_scroll)
        {
            int32_t optSize = static_cast<int32_t>(options.size());
            if (optSize > 0)
                itemIndex = ((itemIndex % optSize) + optSize) % optSize;
        }
        else
        {
            validIndex = (itemIndex >= 0 && itemIndex < static_cast<int32_t>(options.size()));
        }

        // Determine if this is the center row (always gets selected styling for consistent layout)
        bool isCenterRow = (virtualRow == centerRow);
        float rowHeight = isCenterRow ? selected_item_height : item_height;

        if (auto* tc = textObj->GetComponent<TextComponent>())
        {
            // Sync size - use different height for selected row.
            tc->width = width;
            tc->height = rowHeight;

            // Apply color based on row position (center row gets selected styling)
            tc->color = isCenterRow ? selected_color : normal_color;

#ifdef DEKI_EDITOR
            // Apply font size based on row position (for sharp text rendering)
            tc->fontSize = isCenterRow ? selected_font_size : normal_font_size;
#endif

            // Set text from options
            if (!options.empty())
            {
                if (infinite_scroll || validIndex)
                {
                    tc->text = options[itemIndex];
                }
                else
                {
                    // Clear text when out of range (non-infinite scroll)
                    tc->text.clear();
                }
            }
        }

        // Position text rows from edges of center row.
        // Center row ALWAYS stays at Y=0, only non-center rows move with scroll.
        float rowY = 0.0f;
        int32_t rowDist = centerRow - virtualRow;  // positive = above center, negative = below

        if (rowDist > 0)
        {
            // Above center: first row's bottom touches center's top
            float bottomEdge = ((selected_item_height) * (0.5f));
            bottomEdge = ((bottomEdge) + (((static_cast<float>(rowDist - 1)) * (item_height))));
            rowY = ((((bottomEdge) + (((rowHeight) * (0.5f))))) + (subItemOffset));
        }
        else if (rowDist < 0)
        {
            // Below center: first row's top touches center's bottom
            float topEdge = ((0.0f) - (((selected_item_height) * (0.5f))));
            topEdge = ((topEdge) - (((static_cast<float>(-rowDist - 1)) * (item_height))));
            rowY = ((((topEdge) - (((rowHeight) * (0.5f))))) + (subItemOffset));
        }
        // else rowDist == 0 (center row): rowY stays 0, no subItemOffset applied

        textObj->SetLocalPosition(0.0f, rowY);
        // Don't force active - respect user's active state
    }
}

// Accessors for editor rendering (get components from child objects)
SpriteComponent* RollerComponent::GetBackgroundSprite()
{
    if (m_BackgroundObj)
        return m_BackgroundObj->GetComponent<SpriteComponent>();
    return nullptr;
}

SpriteComponent* RollerComponent::GetSelectionSprite()
{
    if (m_SelectionObj)
        return m_SelectionObj->GetComponent<SpriteComponent>();
    return nullptr;
}

TextComponent* RollerComponent::GetTextComponent(int32_t row)
{
    if (row >= 0 && row < static_cast<int32_t>(m_TextRowObjs.size()))
    {
        if (m_TextRowObjs[row])
            return m_TextRowObjs[row]->GetComponent<TextComponent>();
    }
    return nullptr;
}

void RollerComponent::SetOptions(const std::vector<std::string>& newOptions)
{
    options = newOptions;

    // Reset selection if out of bounds
    if (selected_index >= static_cast<int32_t>(options.size()))
    {
        selected_index = options.empty() ? 0 : static_cast<int32_t>(options.size()) - 1;
    }

    // Reset scroll to selected item
    m_ScrollOffset = GetItemOffset(selected_index);
    m_ScrollVelocity = 0.0f;
    m_IsSnapping = false;
    m_NeedsSync = true;
    if (GetOwner())
        SyncChildObjects(GetOwner());
}

std::string RollerComponent::GetSelectedValue() const
{
    if (selected_index >= 0 && selected_index < static_cast<int32_t>(options.size()))
    {
        return options[selected_index];
    }
    return "";
}

void RollerComponent::SetSelectedIndex(int32_t index, bool animated)
{
    if (options.empty())
        return;

    // Clamp index
    if (infinite_scroll)
    {
        index = ((index % static_cast<int32_t>(options.size())) + static_cast<int32_t>(options.size())) % static_cast<int32_t>(options.size());
    }
    else
    {
        index = std::max(static_cast<int32_t>(0), std::min(index, static_cast<int32_t>(options.size()) - 1));
    }

    int32_t oldIndex = selected_index;
    selected_index = index;
    m_LastSyncedSelectedIndex = index;  // Track programmatic change

    if (animated)
    {
        m_TargetOffset = GetItemOffset(index);
        m_IsSnapping = true;
        m_ScrollVelocity = 0.0f;
    }
    else
    {
        m_ScrollOffset = GetItemOffset(index);
        m_ScrollVelocity = 0.0f;
        m_IsSnapping = false;
        m_NeedsSync = true;
    }

    // Fire callback if selection changed
    if (oldIndex != selected_index && m_OnSelectionChanged)
    {
        m_OnSelectionChanged(selected_index, GetSelectedValue());
    }

    if (m_NeedsSync && GetOwner())
    {
        SyncChildObjects(GetOwner());
        m_NeedsSync = false;
    }
}

void RollerComponent::SetVisibleRowCount(int32_t rows)
{
    // Ensure odd number for centered selection
    visible_rows = std::max(static_cast<int32_t>(1), rows);
    if (visible_rows % 2 == 0)
    {
        visible_rows++;
    }
}

void RollerComponent::SetOnSelectionChanged(const RollerCallback& callback)
{
    m_OnSelectionChanged = callback;
}

void RollerComponent::SetOnValueCommitted(const RollerCallback& callback)
{
    m_OnValueCommitted = callback;
}

float RollerComponent::GetItemOffset(int32_t index) const
{
    // Center the selected item in the visible area.
    // The center row has selected_item_height, other rows have item_height.
    // Distance from center to adjacent item = half of each height.
    int32_t centerRow = visible_rows / 2;
    int32_t dist = index - centerRow;

    if (dist == 0) return 0.0f;

    // First step from center uses the transition distance (half of each height).
    // Subsequent steps use uniform item_height.
    float transitionDist = ((((selected_item_height) + (item_height))) * (0.5f));

    if (dist > 0)
    {
        return ((transitionDist) + (((static_cast<float>(dist - 1)) * (item_height))));
    }
    else
    {
        float pos = ((transitionDist) + (((static_cast<float>(-dist - 1)) * (item_height))));
        return ((0.0f) - (pos));
    }
}

int32_t RollerComponent::GetIndexAtOffset(float offset) const
{
    if (options.empty())
        return 0;

    // Calculate which item is closest to the center selection area.
    // An item should be selected when more than 50% of it is in the center row;
    // this matches the snap behavior - the item that will be snapped to should
    // be highlighted.

    int32_t centerRow = visible_rows / 2;
    float transitionDist = ((((selected_item_height) + (item_height))) * (0.5f));
    float halfTransition = ((transitionDist) * (0.5f));  // 50% threshold for first item

    int32_t index;
    float absOffset = DekiMath::Abs(offset);

    if (absOffset < halfTransition)
    {
        index = centerRow;
    }
    else if (offset > 0.0f)
    {
        if (absOffset < transitionDist)
        {
            index = centerRow + 1;
        }
        else
        {
            float pastFirst = ((absOffset) - (transitionDist));
            float adjusted = ((pastFirst) + (((item_height) * (0.5f))));
            int32_t additionalItems = static_cast<int32_t>(std::floor(((adjusted) / (item_height))));
            index = centerRow + 1 + additionalItems;
        }
    }
    else
    {
        if (absOffset < transitionDist)
        {
            index = centerRow - 1;
        }
        else
        {
            float pastFirst = ((absOffset) - (transitionDist));
            float adjusted = ((pastFirst) + (((item_height) * (0.5f))));
            int32_t additionalItems = static_cast<int32_t>(std::floor(((adjusted) / (item_height))));
            index = centerRow - 1 - additionalItems;
        }
    }

    if (infinite_scroll)
    {
        index = ((index % static_cast<int32_t>(options.size())) + static_cast<int32_t>(options.size())) % static_cast<int32_t>(options.size());
    }
    else
    {
        index = std::max(static_cast<int32_t>(0), std::min(index, static_cast<int32_t>(options.size()) - 1));
    }

    return index;
}

int32_t RollerComponent::GetRawIndexAtOffset(float offset) const
{
    if (options.empty())
        return 0;

    int32_t centerRow = visible_rows / 2;
    float transitionDist = ((((selected_item_height) + (item_height))) * (0.5f));
    float halfTransition = ((transitionDist) * (0.5f));

    int32_t index;
    float absOffset = DekiMath::Abs(offset);

    if (absOffset < halfTransition)
    {
        index = centerRow;
    }
    else if (offset > 0.0f)
    {
        if (absOffset < transitionDist)
        {
            index = centerRow + 1;
        }
        else
        {
            float pastFirst = ((absOffset) - (transitionDist));
            float adjusted = ((pastFirst) + (((item_height) * (0.5f))));
            int32_t additionalItems = static_cast<int32_t>(std::floor(((adjusted) / (item_height))));
            index = centerRow + 1 + additionalItems;
        }
    }
    else
    {
        if (absOffset < transitionDist)
        {
            index = centerRow - 1;
        }
        else
        {
            float pastFirst = ((absOffset) - (transitionDist));
            float adjusted = ((pastFirst) + (((item_height) * (0.5f))));
            int32_t additionalItems = static_cast<int32_t>(std::floor(((adjusted) / (item_height))));
            index = centerRow - 1 - additionalItems;
        }
    }

    // No wrapping — return raw index
    return index;
}

void RollerComponent::ClampScrollOffset()
{
    if (options.empty() || infinite_scroll)
        return;

    float minOffset = GetItemOffset(0);
    float maxOffset = GetItemOffset(static_cast<int32_t>(options.size()) - 1);

    if (m_ScrollOffset < minOffset)
    {
        m_ScrollOffset = minOffset;
        m_ScrollVelocity = 0.0f;
    }
    else if (m_ScrollOffset > maxOffset)
    {
        m_ScrollOffset = maxOffset;
        m_ScrollVelocity = 0.0f;
    }
}

void RollerComponent::SnapToNearestItem()
{
    if (options.empty())
        return;

    int32_t rawIndex = GetRawIndexAtOffset(m_ScrollOffset);

    // Use raw (unwrapped) index so target is in the same domain as m_ScrollOffset
    m_TargetOffset = GetItemOffset(rawIndex);
    m_IsSnapping = true;
}

void RollerComponent::UpdateSelection()
{
    if (options.empty())
        return;

    int32_t rawIndex = GetRawIndexAtOffset(m_ScrollOffset);

    // Wrap for actual selection
    int32_t newIndex;
    if (infinite_scroll)
    {
        int32_t optSize = static_cast<int32_t>(options.size());
        newIndex = ((rawIndex % optSize) + optSize) % optSize;
    }
    else
    {
        newIndex = std::max(static_cast<int32_t>(0), std::min(rawIndex, static_cast<int32_t>(options.size()) - 1));
    }

    if (newIndex != selected_index)
    {
        selected_index = newIndex;
        m_LastSyncedSelectedIndex = newIndex;  // Track internal change

        if (m_OnSelectionChanged)
        {
            m_OnSelectionChanged(selected_index, GetSelectedValue());
        }
    }
}

void RollerComponent::HandlePointerDown(float x, float y)
{
    (void)x;
    // Pointer y arrives in world meters as float (input FFI); convert to
    // float at the boundary to stay in the same numeric space as state.
    float touchY = static_cast<float>(y);
    m_IsDragging = true;
    m_LastTouchY = touchY;
    m_TouchStartY = touchY;
    m_TouchStartOffset = m_ScrollOffset;
    m_ScrollVelocity = 0.0f;
    m_IsSnapping = false;
}

void RollerComponent::HandlePointerMove(float x, float y)
{
    (void)x;
    if (!m_IsDragging) return;

    float touchY = static_cast<float>(y);
    float delta = ((touchY) - (m_LastTouchY));

    // Default: drag follows touch direction (like ScrollComponent)
    // reverse_drag: drag moves opposite to touch
    float scrollDelta = reverse_drag ? ((0.0f) - (delta)) : delta;
    m_ScrollOffset = ((m_ScrollOffset) + (scrollDelta));

    // Track velocity for momentum (reduced for less aggressive scrolling)
    static const float kThird = static_cast<float>(1.0f / 3.0f);
    m_ScrollVelocity = ((scrollDelta) * (kThird));

    m_LastTouchY = touchY;

    ClampScrollOffset();
    UpdateSelection();

    // Update visual positions of text rows
    if (GetOwner())
        SyncChildObjects(GetOwner());
}

void RollerComponent::HandlePointerUp(float x, float y)
{
    (void)x;
    if (!m_IsDragging) return;

    m_IsDragging = false;

    float touchY = static_cast<float>(y);

    // Tap-vs-drag threshold: one-third of an item.
    static const float kThird = static_cast<float>(1.0f / 3.0f);
    float dragDistance = DekiMath::Abs(((touchY) - (m_TouchStartY)));
    float tapThreshold = ((item_height) * (kThird));
    if (dragDistance < tapThreshold)
    {
        DekiObject* owner = GetOwner();
        if (owner)
        {
            // Calculate which row was tapped — everything in meters.
            float centerY = owner->GetWorldY();
            float rollerTop = ((centerY) - (((GetHeight()) * (0.5f))));
            float relativeY = ((touchY) - (rollerTop));
            int32_t tappedRow = static_cast<int32_t>(std::floor(((relativeY) / (item_height))));

            // Clamp to valid row range
            if (tappedRow >= 0 && tappedRow < visible_rows)
            {
                int32_t centerRow = visible_rows / 2;
                int32_t rowOffset = tappedRow - centerRow;

                if (rowOffset != 0)
                {
                    // Calculate the target index
                    int32_t targetIndex = selected_index + rowOffset;

                    // Clamp or wrap based on infinite_scroll
                    if (infinite_scroll)
                    {
                        int32_t optSize = static_cast<int32_t>(options.size());
                        targetIndex = ((targetIndex % optSize) + optSize) % optSize;
                    }
                    else
                    {
                        targetIndex = std::max(static_cast<int32_t>(0), std::min(targetIndex, static_cast<int32_t>(options.size()) - 1));
                    }

                    // Animate to the tapped item
                    if (targetIndex != selected_index)
                    {
                        SetSelectedIndex(targetIndex, true);
                        return;
                    }
                }
            }
        }

        // If tapped center row or invalid, just snap
        m_ScrollVelocity = 0.0f;
        SnapToNearestItem();
        return;
    }

    // If velocity is low (slow drag or stopped between items), snap immediately.
    // Threshold: ~3 px/frame at ppm=16.
    static const float kLowVelocity = static_cast<float>(0.1875f);
    if (DekiMath::Abs(m_ScrollVelocity) < kLowVelocity)
    {
        m_ScrollVelocity = 0.0f;
        SnapToNearestItem();
    }
    // Otherwise let momentum continue, Update() will snap when velocity dies down
}

void RollerComponent::Update(float delta_time)
{
    if (m_IsDragging)
        return;

    bool needsSync = false;

    // Velocity thresholds in meters/frame (preserve prior px-based feel at ppm=16):
    static const float kVelocityActive = static_cast<float>(0.0625f);  // > 1 px/frame
    static const float kVelocityCutoff = static_cast<float>(0.125f);   // < 2 px/frame triggers snap
    static const float kSnapArrived    = static_cast<float>(0.004f);   // < ~0.06 px from target

    // Phase 1: Momentum scrolling (after release, before snap)
    if (!m_IsSnapping && DekiMath::Abs(m_ScrollVelocity) > kVelocityActive)
    {
        // Apply velocity to scroll
        m_ScrollOffset = ((m_ScrollOffset) + (m_ScrollVelocity));

        // Decelerate (friction)
        m_ScrollVelocity = ((m_ScrollVelocity) * (deceleration));

        ClampScrollOffset();

        // When velocity gets low enough, start snapping to nearest item
        if (DekiMath::Abs(m_ScrollVelocity) < kVelocityCutoff)
        {
            m_ScrollVelocity = 0.0f;
            SnapToNearestItem();
        }

        needsSync = true;
    }
    // Phase 2: Snap animation (smooth ease to target)
    else if (m_IsSnapping)
    {
        float diff = ((m_TargetOffset) - (m_ScrollOffset));

        // Simple ease-out: move a fraction of the remaining distance.
        // snap_speed controls how fast (higher = faster snap). easeFactor is
        // a pure float coefficient (frame scaling), wrapped to mix safely.
        float easeFactorF = (snap_speed) * 0.15f * delta_time * 60.0f;
        if (easeFactorF > 0.5f) easeFactorF = 0.5f;
        float easeFactor = static_cast<float>(easeFactorF);

        m_ScrollOffset = ((m_ScrollOffset) + (((diff) * (easeFactor))));

        // Check if we've arrived
        if (DekiMath::Abs(((m_TargetOffset) - (m_ScrollOffset))) < kSnapArrived)
        {
            m_ScrollOffset = m_TargetOffset;
            m_IsSnapping = false;

            if (m_OnValueCommitted)
            {
                m_OnValueCommitted(selected_index, GetSelectedValue());
            }
        }

        ClampScrollOffset();
        needsSync = true;
    }

    UpdateSelection();

    // Update visual positions of text rows when animating
    if (needsSync && GetOwner())
        SyncChildObjects(GetOwner());
}

// ============================================================================
// LIFECYCLE METHODS FOR PLAY MODE
// ============================================================================

void RollerComponent::Start()
{
    // Initialize child object references when play mode starts
    // This is critical for duplicated rollers whose runtime pointers are null
    if (GetOwner())
        SyncChildObjects(GetOwner());

    InputCollider* collider = input_collider.Get();
    if (!collider)
    {
        DEKI_LOG_WARNING("RollerComponent: No InputCollider referenced on '%s'",
                         GetOwner()->GetName().c_str());
        return;
    }

    collider->on_pointer_down.push_back([this](float x, float y) {
        HandlePointerDown(x, y);
    });

    collider->on_pointer_move.push_back([this](float x, float y) {
        HandlePointerMove(x, y);
    });

    collider->on_pointer_up.push_back([this](float x, float y) {
        HandlePointerUp(x, y);
    });
}

bool RollerComponent::NeedsRuntimeUpdate() const
{
    return true;  // Roller always needs update for physics/animation
}

void RollerComponent::RuntimeUpdate(float deltaTime)
{
    Update(deltaTime);
}

void RollerComponent::OnPropertyChanged(const char* propertyName)
{
    (void)propertyName;

    // Sync child objects when any property changes
    // This updates TextComponent width/height and other derived values
    if (GetOwner())
    {
        SyncChildObjects(GetOwner());
    }
}
