cmake_minimum_required(VERSION 3.24)

# =========================================================
#						Config
# =========================================================

set(NME_PROJECT_NAME "NME")
set(NME_PROJECT_VERSION_MAJOR 0)
set(NME_PROJECT_VERSION_MINOR 1)
set(NME_PROJECT_VERSION_PATCH 10)

set(NME_CXX_VERSION 20)

# =========================================================
#						Project
# =========================================================

project(${NME_PROJECT_NAME}
	VERSION "${NME_PROJECT_VERSION_MAJOR}.${NME_PROJECT_VERSION_MINOR}.${NME_PROJECT_VERSION_PATCH}"
	DESCRIPTION "A learning game engine following Gregory's Game Engine Architecture, 4th ed."
	LANGUAGES C CXX
)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are disabled. Use e.g. `cmake -S . -B build/{platform}`.")
endif()

get_property(_is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT _is_multi_config AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# --- Language ---
set(CMAKE_CXX_STANDARD ${NME_CXX_VERSION})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =========================================================
#					CMake Settings
# =========================================================
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

option(NME_WARNINGS_AS_ERRORS "Treat compiler warnings as errors"		ON)
option(NME_ENABLE_SANITIZERS  "Enable address sanitizer + UBSan"		OFF)
option(NME_ENABLE_TRACY		  "Fetch and enable the Tracy profiler"		OFF)
option(NME_BUILD_TESTS		  "Build the unit-test target (needs network on first configure)" ON)
option(NME_BUILD_ASAN_CHECK	  "Build the deliberate-bug program used to verify ASan" OFF)
option(NME_BUILD_BENCHMARKS   "Build the benchmarks targets"            ON)

include(cmake/compiler_warnings.cmake)
add_library(nme_warnings INTERFACE)
add_library(nme::warnings ALIAS nme_warnings)
nme_set_project_warnings(nme_warnings ${NME_WARNINGS_AS_ERRORS})

include(cmake/sanitizers.cmake)

# =========================================================
#					Subdirectories
# =========================================================
add_subdirectory(engine)
add_subdirectory(apps/sandbox)

if (NME_BUILD_BENCHMARKS)
    add_subdirectory(apps/benchmarks)
endif()

if (NME_BUILD_ASAN_CHECK)
	add_subdirectory(apps/asan_check)
endif()

if (NME_BUILD_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif()

# --- Output ---
message(STATUS "")
message(STATUS "nme ${PROJECT_VERSION} configured:")
message(STATUS "  Generator          : ${CMAKE_GENERATOR}")
message(STATUS "  Build type         : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler           : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Warnings as errors : ${NME_WARNINGS_AS_ERRORS}")
message(STATUS "  Sanitizers         : ${NME_ENABLE_SANITIZERS}")
message(STATUS "  Tracy profiler     : ${NME_ENABLE_TRACY}")
message(STATUS "  Build tests        : ${NME_BUILD_TESTS}")
message(STATUS "  Build benchmarks   : ${NME_BUILD_BENCHMARKS}")
message(STATUS "")