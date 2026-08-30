# Package descriptor for deki-engine auto-discovery
set(PACKAGE_DISPLAY_NAME "Node Graph")
set(PACKAGE_PREFIX "DekiNodeGraph")
set(PACKAGE_UPPER "NODEGRAPH")
set(PACKAGE_TARGET "deki-nodegraph")
set(PACKAGE_FILE_PREFIX "NodeGraph")
set(PACKAGE_ENTRY NodeGraphModule.cpp)
# The Node Graph window draws entirely through EditorUI / EditorTheme (both
# implemented in deki-editor.dll), so this package makes no ImGui calls of its
# own and needs no shared ImGuiContext. It only pulls imgui headers for the
# POD types (ImVec4, tree-node flags) those APIs take by value.
set(PACKAGE_NEEDS_IMGUI OFF)
set(PACKAGE_LINK_DEPS deki-editor)
set(PACKAGE_EXPORT_ALL_SYMBOLS ON)
