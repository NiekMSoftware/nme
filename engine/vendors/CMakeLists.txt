# vendors/ - hand-vendored third-party SDKs ("3rd Party SDKs" layer, sec. 1.5).
#
# Check dependencies into the repo under here, then expose them to the engine
# from engine/CMakeLists.txt. Graphics backends (directx/opengl/vulkan) are
# Volume II territory, so these stay empty placeholders for now.
#
# Typical patterns once a backend is added:
#   add_subdirectory(vulkan)                       # a CMake-based vendored SDK
#   find_package(Vulkan REQUIRED)                  # or a system/SDK install
# then in engine/CMakeLists.txt:
#   target_link_libraries(nme_engine PUBLIC Vulkan::Vulkan)

# (Nothing to build yet - subfolders are placeholders.)
