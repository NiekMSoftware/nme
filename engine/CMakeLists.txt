add_library(nme_engine STATIC)
add_library(nme::engine ALIAS nme_engine)

# Public headers declared as a header set (CMake 3.23+). This makes them show up
# in IDE projects (Visual Studio, CLion, ...), registers include/ as a usage
# requirement for anything linking nme::engine, and enables install(FILE_SET)
# later. GLOB_RECURSE + CONFIGURE_DEPENDS re-scans at build time, so new headers
# are picked up without editing this file.
file(GLOB_RECURSE NME_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/nme/*.h")

# --- Platform ---
set(NME_PLATFORM_SOURCES
        src/platform/assert.cpp
        src/platform/filesys/path.cpp
        src/platform/filesys/read_file.cpp
        src/platform/timer/timer.cpp
)
set(NME_PLATFORM_HEADERS
        include/nme/platform/cache.h
        include/nme/platform/platform.h
        include/nme/platform/types.h

        # Containers
        include/nme/platform/collections/dynamic_array.h
        include/nme/platform/collections/fixed_array.h
        include/nme/platform/collections/hash_map.h
        include/nme/platform/collections/linked_list.h
        include/nme/platform/collections/ring_buffer.h

        # Debug
        include/nme/platform/debug/assert.h
        include/nme/platform/debug/debug_break.h

        # Error
        include/nme/platform/error/result.h

        # File system
        include/nme/platform/filesys/dir.h
        include/nme/platform/filesys/file.h
        include/nme/platform/filesys/path.h
        include/nme/platform/filesys/read_file.h

        # Graphics
        include/nme/platform/gfx/clip_space.h
        include/nme/platform/gfx/gfx_types.h
        include/nme/platform/gfx/gfx.h

        # Memory
        include/nme/platform/memory/align.h
        include/nme/platform/memory/allocator.h

        # Threading
        include/nme/platform/thread/atomics.h
        include/nme/platform/thread/condition_variable.h
        include/nme/platform/thread/mutex.h
        include/nme/platform/thread/thread.h
        include/nme/platform/thread/spsc_ring.h

        # Timer
        include/nme/platform/timer/timer.h
)

# --- Core ---
set(NME_CORE_SOURCES
        src/core/string_id.cpp
        src/core/config/config_file.cpp
        src/core/config/cvar.cpp
        src/core/jobs/job_system.cpp
        src/core/subsystem/kernel.cpp
)
set(NME_CORE_HEADERS
        include/nme/core/config/config_file.h
        include/nme/core/config/cvar.h

        include/nme/core/debug/log.h
        include/nme/core/debug/profiler.h
        include/nme/core/debug/scoped_timer.h

        include/nme/core/memory/heap_alloc.h
        include/nme/core/memory/pool_alloc.h
        include/nme/core/memory/scratch_alloc.h
        include/nme/core/memory/stack_alloc.h

        include/nme/core/jobs/job.h
        include/nme/core/jobs/job_system.h
        include/nme/core/jobs/ws_deque.h

        include/nme/core/math/matrix.hpp
        include/nme/core/math/vec.hpp

        include/nme/core/string/string_id.h

        include/nme/core/subsystem/subsystem.h
        include/nme/core/subsystem/kernel.h

        include/nme/core/util/scope_guard.h
        include/nme/core/util/unique_handle.h
)

# --- Resources ---
set(NME_RESOURCE_SOURCES
        src/resource/package.cpp
)
set(NME_RESOURCE_HEADERS
        include/nme/resource/package.h
)

# --- Renderer ---
set(NME_RENDERER_SOURCES
        src/renderer/renderer.cpp
)
set(NME_RENDERER_HEADERS
        include/nme/renderer/gdi/gdi.h
        include/nme/renderer/gdi/gdi_types.h
)

target_sources(nme_engine
    PRIVATE
        src/version.cpp

        ${NME_CORE_SOURCES}
        ${NME_PLATFORM_SOURCES}
        ${NME_RESOURCE_SOURCES}
        ${NME_RENDERER_SOURCES}
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include"
        FILES 
            ${NME_PLATFORM_HEADERS}
            ${NME_CORE_HEADERS}
            ${NME_RESOURCE_HEADERS}
            ${NME_RENDERER_HEADERS}
)

# --- Platform-specific sources ---
if (WIN32)
    target_sources(nme_engine
        PRIVATE
            src/platform/filesys/win32_dir.cpp
            src/platform/filesys/win32_file.cpp
            src/platform/gfx/surface/win32_surface.cpp
            src/platform/thread/win32/condition_variable.cpp
            src/platform/thread/win32/mutex.cpp
            src/platform/thread/win32/thread.cpp
    )
#elseif(UNIX)
#    target_sources(nme_engine
#        PRIVATE
#            src/platform/thread/posix/mutex.cpp
#    )
#elseif(LINUX and NOT APPLE)
#    target_sources(nme_engine
#        PRIVATE
#            src/platform/thread/linux/mutex.cpp
#    )
endif()

if (NME_GFX_BACKEND STREQUAL "Vulkan")
    target_sources(nme_engine PRIVATE
            src/renderer/backend/vulkan/vk_backend.cpp
            src/renderer/backend/vulkan/vk_buffer.cpp
            src/renderer/backend/vulkan/vk_swapchain.cpp
            src/renderer/backend/vulkan/vk_vma.cpp
    )
    target_compile_definitions(nme_engine PUBLIC NME_GFX_VULKAN=1)
else()
    message(FATAL_ERROR "NME_GFX_BACKEND must be Vulkan or Null (got '${NME_GFX_BACKEND}')")
endif()

# Nest the headers by folder in IDE project trees instead of one flat list.
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/include/nme"
    PREFIX "Header Files" FILES ${NME_HEADERS})

target_compile_features(nme_engine PUBLIC cxx_std_20)

target_compile_definitions(nme_engine PRIVATE
    NME_VERSION=${PROJECT_VERSION}
    NME_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    NME_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    NME_VERSION_PATCH=${PROJECT_VERSION_PATCH})

target_link_libraries(nme_engine PRIVATE nme::warnings)
set_target_properties(nme_engine PROPERTIES FOLDER "nme")

# Optional Tracy (Ch.2 stretch / Ch.10). OFF by default -> no network needed.
if(NME_ENABLE_TRACY)
    nme_declare(tracy
        GIT_REPOSITORY https://github.com/wolfpld/tracy.git
        GIT_TAG        v0.11.1
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(tracy)
    target_link_libraries(nme_engine PUBLIC TracyClient)
    target_compile_definitions(nme_engine PUBLIC NME_ENABLE_TRACY=1)
endif()

nme_enable_sanitizers(nme_engine)

# Third-party SDKs (see vendors/CMakeLists.txt).
add_subdirectory(vendors)
