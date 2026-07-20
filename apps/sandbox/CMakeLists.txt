add_executable(sandbox main.cpp)
target_link_libraries(sandbox PRIVATE nme::engine nme::warnings)
nme_enable_sanitizers(sandbox)
set_target_properties(sandbox PROPERTIES FOLDER "apps")
set_target_properties(sandbox PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:sandbox>")

include(GNUInstallDirs)

# Game-style flat layout: exe sits with its data, not in bin/
install(TARGETS sandbox RUNTIME DESTINATION .)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets/" DESTINATION assets OPTIONAL)
install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/shaders/" DESTINATION shaders OPTIONAL)

# Real linked runtime DLLs only (3.21+); the Vulkan loader is a system lib and won't appear.
install(FILES $<TARGET_RUNTIME_DLLS:sandbox> DESTINATION . COMPONENT runtime)