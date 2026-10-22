#pragma once

#include "TextComponent.h"

#include <deki/Guid.h>

#include <cstdint>
#include <string>

// Text drawn bigger without blocky pixels: in the same font baked at the
// bigger size. A font is baked once per size its .data file lists, each size
// a variant whose GUID the editor hashes from the font's GUID and the size, so
// the variant for any size can be found here. A build carries only the
// variants a scene uses or the project force-includes; without the bigger one
// the text repeats pixels instead.
namespace SharpText
{

// The size `guid`, a variant of the text's font, was baked at; 0 when unknown.
inline int VariantSize(const Deki2D::TextComponent& text, const std::string& guid)
{
    for (int size = 4; size <= 96 && !text.font.source.empty(); ++size)
    {
        if (Deki::GenerateDeterministicGuid(text.font.source + ":" + std::to_string(size)) == guid)
        {
            return size;
        }
    }
    return 0;
}

inline void UseVariant(Deki2D::TextComponent& text, const std::string& guid, int size)
{
    if (text.font.guid == guid)
    {
        return;
    }
    text.font.guid = guid;
    text.font.ptr = nullptr;
    text.font.loadAttempted = false;
#ifdef DEKI_EDITOR
    // The editor draws the variant of fontSize.
    if (size > 0)
    {
        text.fontSize = size;
    }
#else
    (void)size;
#endif
}

// Draws `text` at `k` times the size it was authored at (`guid`, a variant of
// `size` px, at `pixelScale`).
inline void Scale(Deki2D::TextComponent& text, const std::string& guid, int size, int32_t pixelScale, int k)
{
    UseVariant(text, guid, size);
    text.pixelScale = pixelScale * k;
    if (k == 1 || size == 0)
    {
        return;
    }
    UseVariant(text, Deki::GenerateDeterministicGuid(text.font.source + ":" + std::to_string(size * k)), size * k);
    if (text.GetFont())
    {
        text.pixelScale = pixelScale;
        return;
    }
    UseVariant(text, guid, size);  // not in this build
}

// Draws `text` in the variant of `target` px when the build has it, else as
// authored. For sizes that are no whole multiple, which repeated pixels
// cannot make.
inline bool ScaleTo(Deki2D::TextComponent& text, const std::string& guid, int size, int32_t pixelScale, int target)
{
    UseVariant(text, guid, size);
    text.pixelScale = pixelScale;
    if (size == 0 || target == size)
    {
        return target == size;
    }
    UseVariant(text, Deki::GenerateDeterministicGuid(text.font.source + ":" + std::to_string(target)), target);
    if (text.GetFont())
    {
        return true;
    }
    UseVariant(text, guid, size);  // not in this build
    return false;
}

}  // namespace SharpText
