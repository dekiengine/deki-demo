# Package descriptor for deki-engine auto-discovery
set(PACKAGE_DISPLAY_NAME "SDL3")
set(PACKAGE_PREFIX "DekiSDL3")
set(PACKAGE_UPPER "SDL3")
set(PACKAGE_TARGET "deki-sdl3")
set(PACKAGE_FILE_PREFIX "SDL3")
set(PACKAGE_SOURCES
    SDL3DisplaySetup.cpp
    SDL3InputSetup.cpp
    SDL3Display.cpp
    SDL3Input.cpp
)
set(PACKAGE_ENTRY SDL3Package.cpp)
set(PACKAGE_LINK_DEPS deki-input)
