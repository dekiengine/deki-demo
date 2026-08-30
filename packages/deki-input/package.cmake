# Package descriptor for deki-engine auto-discovery
set(PACKAGE_DISPLAY_NAME "Input")
set(PACKAGE_PREFIX "DekiInput")
set(PACKAGE_UPPER "INPUT")
set(PACKAGE_TARGET "deki-input")
set(PACKAGE_FILE_PREFIX "Input")
set(PACKAGE_HAS_SYSTEM_INIT ON)
set(PACKAGE_SOURCES
    InputDispatch.cpp
    InputCollider.cpp
    DekiInputSystem.cpp
    DekiInputInit.cpp
    DekiInput.cpp
)
set(PACKAGE_ENTRY DekiInputPackage.cpp)
set(PACKAGE_LINK_DEPS deki-rendering)
