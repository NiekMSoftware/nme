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
    # Timer
    src/platform/timer/timer.cpp
)
set(NME_PLATFORM_HEADERS
    include/nme/platform/cache.h
    include/nme/platform/platform.h
    include/nme/platform/types.h

    # Debug
    include/nme/platform/debug/assert.h
    include/nme/platform/debug/debug_break.h

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
    src/core/assert.cpp

    src/core/jobs/job_system.cpp

    src/core/result/error.cpp
)
set(NME_CORE_HEADERS
    include/nme/core/assert/assert.h

    include/nme/core/debug/log.h
    include/nme/core/debug/profiler.h
    include/nme/core/debug/scoped_timer.h

    include/nme/core/jobs/job.h
    include/nme/core/jobs/job_system.h
    include/nme/core/jobs/ws_deque.h

    include/nme/core/math/vec.h

    include/nme/core/result/error.h
    include/nme/core/result/result.h

    include/nme/core/util/scope_guard.h
    include/nme/core/util/unique_handle.h
)

target_sources(nme_engine
    PRIVATE
        src/version.cpp

        ${NME_CORE_SOURCES}
        ${NME_PLATFORM_SOURCES}
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include"
        FILES 
            ${NME_PLATFORM_HEADERS}
            ${NME_CORE_HEADERS}
)

# --- Platform-specific sources ---
if (WIN32)
    target_sources(nme_engine
        PRIVATE
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

# Nest the headers by folder in IDE project trees instead of one flat list.
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/include/nme"
    PREFIX "Header Files" FILES ${NME_HEADERS})

target_compile_features(nme_engine PUBLIC cxx_std_20)

target_compile_definitions(nme_engine PRIVATE
    NME_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    NME_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    NME_VERSION_PATCH=${PROJECT_VERSION_PATCH})

target_link_libraries(nme_engine PRIVATE nme::warnings)
set_target_properties(nme_engine PROPERTIES FOLDER "nme")

# Optional Tracy (Ch.2 stretch / Ch.10). OFF by default -> no network needed.
if(NME_ENABLE_TRACY)
    include(FetchContent)
    FetchContent_Declare(tracy
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
