add_executable(sandbox main.cpp)
target_link_libraries(sandbox PRIVATE nme::engine nme::warnings)
nme_enable_sanitizers(sandbox)
set_target_properties(sandbox PROPERTIES FOLDER "apps")
add_custom_command(TARGET sandbox POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/config"
        "$<TARGET_FILE_DIR:sandbox>/config"
        COMMENT "Staging sandbox config to $<TARGET_FILE_DIR:sandbox>/config"
        VERBATIM)
set_target_properties(sandbox PROPERTIES
        OUTPUT_NAME "nme_editor"
        PROJECT_LABEL "NME - Editor v${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

include(GNUInstallDirs)

# Game-style flat layout: exe sits with its data, not in bin/
install(TARGETS sandbox RUNTIME DESTINATION .)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets/"  DESTINATION assets OPTIONAL)
install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/shaders/" DESTINATION shaders OPTIONAL)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/config/"  DESTINATION config)

# Real linked runtime DLLs only (3.21+); the Vulkan loader is a system lib and won't appear.
install(FILES $<TARGET_RUNTIME_DLLS:sandbox> DESTINATION . COMPONENT runtime)