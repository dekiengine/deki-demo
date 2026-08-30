#pragma once

#include <SDL3/SDL.h>

#include "providers/IDekiDisplay.h"

/**
 * @brief SDL3 implementation of display interface
 */
class SDL3Display : public IDekiDisplay
{
   private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* game_texture;
    SDL_Texture* ui_overlay_texture;

    int32_t display_width;
    int32_t display_height;
    bool initialized;

    // Game texture cache variables
    int last_fb_width, last_fb_height, last_fb_format;

   public:
    SDL3Display();
    virtual ~SDL3Display();

    // IPlatformDisplay interface
    bool Initialize(int32_t width, int32_t height) override;
    void Shutdown() override;
    void Present(const uint8_t* framebuffer, int width, int height, int format) override;
    void GetDisplaySize(int32_t* width, int32_t* height) const override;
    bool IsInitialized() const override;
    void RequestFullRefresh() override;
    bool ProcessEvents() override;

    // UI/Overlay support
    void* CreateUIOverlay(int32_t width, int32_t height) override;
    bool UpdateUIOverlay(
        void* overlay, int32_t x, int32_t y, int32_t width, int32_t height, const uint32_t* pixels) override;
    bool UpdateUIOverlayRGB565A8(
        void* overlay, int32_t x, int32_t y, int32_t width, int32_t height, const uint8_t* rgb565a8_pixels) override;
    void DestroyUIOverlay(void* overlay) override;
    void SetActiveUIOverlay(void* overlay) override;
    void ClearActiveUIOverlay() override;

    // SDL3-specific methods
    SDL_Renderer* GetRenderer() const { return renderer; }
    SDL_Window* GetWindow() const { return window; }
};
