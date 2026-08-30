#pragma once

/**
 * @file DekiInputPackage.h
 * @brief Central header for the Deki Input Package
 *
 * The input package provides:
 * - InputCollider: Hit area component with pointer event callbacks
 * - DekiInputSystem: Dispatches input events to InputCollider components
 * - InputDispatch: Gesture claim system (in deki-engine-core)
 */

// DLL export macro
#ifdef DEKI_EDITOR
    #ifdef _WIN32
        #if defined(DEKI_INPUT_EXPORTS) || defined(DEKI_PLUGIN_EXPORTS)
            #define DEKI_INPUT_API __declspec(dllexport)
        #else
            #define DEKI_INPUT_API __declspec(dllimport)
        #endif
    #else
        #define DEKI_INPUT_API
    #endif
#else
    #define DEKI_INPUT_API
#endif

// Include all package headers when package is enabled
#ifdef DEKI_PACKAGE_INPUT

#include "InputCollider.h"
#include "DekiInputSystem.h"

#endif // DEKI_PACKAGE_INPUT
