#pragma once

#include <SDL3/SDL.h>

#include "providers/ITimeProvider.h"

class SDL3TimeProvider : public ITimeProvider
{
public:
    uint32_t GetTicksMs() const override { return static_cast<uint32_t>(SDL_GetTicks()); }
    void DelayMs(uint32_t ms) const override { SDL_Delay(ms); }
};
