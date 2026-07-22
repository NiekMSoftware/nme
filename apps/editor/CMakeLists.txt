# apps/editor/CMakeLists.txt
#
# Mirrors apps/sandbox: an executable that links the engine + warnings target,
# lands in the "apps" IDE folder, and runs the sanitizer hook.

add_executable(editor
        main.cpp
        project.cpp)

target_link_libraries(editor PRIVATE nme::engine nme::warnings)
nme_enable_sanitizers(editor)
set_target_properties(editor PROPERTIES FOLDER "apps")

# The editor scans a projects/ folder next to the exe. main.cpp also creates it
# on first run, so this POST_BUILD step only matters if you later want to ship
# seed projects alongside the editor.
#add_custom_command(TARGET editor POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E make_directory
#        "$<TARGET_FILE_DIR:editor>/projects"
#        COMMENT "Ensuring editor projects/ dir at $<TARGET_FILE_DIR:editor>/projects"
#        VERBATIM)

set_target_properties(editor PROPERTIES
        OUTPUT_NAME "nme_editor"
        PROJECT_LABEL "NME - Editor v${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")