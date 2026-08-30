#pragma once

/**
 * @file SDL3Package.h
 * @brief Central header for the Deki SDL3 Package
 *
 * This package provides SDL3-specific components:
 * - Display setup (SDL3 window creation and rendering)
 * - Input setup (keyboard and mouse input)
 */

// DLL export macro
#ifdef _WIN32
    #ifdef DEKI_SDL3_EXPORTS
        #define DEKI_SDL3_API __declspec(dllexport)
    #else
        #define DEKI_SDL3_API __declspec(dllimport)
    #endif
#else
    #define DEKI_SDL3_API __attribute__((visibility("default")))
#endif
