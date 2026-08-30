#pragma once

#include <cstdint>
#include "DekiComponent.h"
#include "reflection/DekiProperty.h"
#include "IClipProvider.h"
#include "ISortableProvider.h"

/**
 * @brief Component that clips children to a rectangular region
 *
 * Add this component to any object to clip its children's rendering
 * to the specified width/height bounds. The clip region is centered
 * on the object's position.
 *
 * Usage:
 * @code
 * auto* clip = entity->AddComponent<ClipComponent>();
 * clip->width = 100;
 * clip->height = 50;
 * // All children will be clipped to this 100x50 region
 * @endcode
 *
 * ClipComponents can be nested - child clips are intersected with parent clips.
 */
class ClipComponent : public DekiComponent, public IClipProvider, public ISortableProvider
{
public:
    DEKI_COMPONENT(ClipComponent, DekiComponent, "Core", "063a42d4-bfe1-49b9-b620-0cbf87ad720f", "DEKI_FEATURE_CLIP")
    DEKI_DESCRIPTION("Clips its children's rendering to a rectangle.")

    /** @brief Width of clip region in meters */
    DEKI_EXPORT
    DEKI_UNIT(Distance)
    float width = 6.25f;

    /** @brief Height of clip region in meters */
    DEKI_EXPORT
    DEKI_UNIT(Distance)
    float height = 6.25f;

    /** @brief Sorting order for this clip region (lower = behind) */
    DEKI_EXPORT
    int32_t sorting_order = 0;

    ClipComponent() = default;
    virtual ~ClipComponent() = default;

    // ISortableProvider
    int32_t GetSortingOrder() const override { return sorting_order; }

    // IClipProvider
    float GetClipWidth() const override { return width; }
    float GetClipHeight() const override { return height; }

    /**
     * @brief Get clip bounds in screen coordinates (pixels)
     *
     * Caller is responsible for converting meters to screen pixels and
     * passing screenX/screenY accordingly; this helper just centers a
     * pixel-sized rectangle on the given screen point.
     */
    void GetClipBounds(int32_t screenX, int32_t screenY, int32_t pixelW, int32_t pixelH,
                       int32_t& outLeft, int32_t& outTop,
                       int32_t& outRight, int32_t& outBottom) const
    {
        outLeft = screenX - pixelW / 2;
        outTop = screenY - pixelH / 2;
        outRight = outLeft + pixelW;
        outBottom = outTop + pixelH;
    }
};

// Generated property metadata (after class definition for offsetof)
#include "generated/ClipComponent.gen.h"
