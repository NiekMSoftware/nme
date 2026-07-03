# Unit test via doctest
# Disable with -DNME_BUILD_TESTS=OFF if offline
include(FetchContent)

FetchContent_Declare(doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.4.11
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  __skip_doctest_build__
)
FetchContent_MakeAvailable(doctest)

add_library(doctest_header INTERFACE)
target_include_directories(doctest_header SYSTEM INTERFACE ${doctest_SOURCE_DIR})

add_executable(nme_tests
    test_main.cpp
    test_version.cpp
    test_mpmc_queue.cpp
)

# nme::warnings intentionally omitted: doctest macros expand in the test TUs and
# would trip our aggressive -Werror set.
target_link_libraries(nme_tests PRIVATE nme::engine doctest_header)
nme_enable_sanitizers(nme_tests)

include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(nme_tests)