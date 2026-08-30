#pragma once

#include <stdint.h>

/**
 * @brief 2D axis-aligned bounding box
 *
 * Simple rectangle defined by width, height, and optional padding on each side.
 * All values are in world meters (matches the engine's meters-internal
 * convention). Used by ButtonComponent and other components that need a hit
 * area. Editor gizmo paths multiply by the camera's pixelsPerMeter to get
 * screen pixels.
 */
struct Bounds2D
{
    float width = 0.0f;
    float height = 0.0f;
    float padding_left = 0.0f;
    float padding_right = 0.0f;
    float padding_top = 0.0f;
    float padding_bottom = 0.0f;

    Bounds2D() = default;

    Bounds2D(float w, float h)
        : width(w), height(h) {}

    /**
     * @brief Get total width including padding
     */
    float GetTotalWidth() const { return padding_left + width + padding_right; }

    /**
     * @brief Get total height including padding
     */
    float GetTotalHeight() const { return padding_top + height + padding_bottom; }

    /**
     * @brief Set uniform padding on all sides
     */
    void SetPadding(float padding)
    {
        padding_left = padding_right = padding_top = padding_bottom = padding;
    }

    /**
     * @brief Set padding per side
     */
    void SetPadding(float left, float right, float top, float bottom)
    {
        padding_left = left;
        padding_right = right;
        padding_top = top;
        padding_bottom = bottom;
    }

    /**
     * @brief Hit test a point against the bounds
     * @param x Point X relative to the bounds origin (top-left of the content area)
     * @param y Point Y relative to the bounds origin (top-left of the content area)
     * @return true if the point is inside (including padding)
     */
    bool Contains(float x, float y) const
    {
        return x >= -padding_left &&
               x <= width + padding_right &&
               y >= -padding_top &&
               y <= height + padding_bottom;
    }
};
