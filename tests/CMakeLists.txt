# Unit tests via doctest - one executable per component.
# Disable with -DNME_BUILD_TESTS=OFF if offline.
nme_declare(doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.4.11
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  __skip_doctest_build__
)
FetchContent_MakeAvailable(doctest)

option(NME_TEST_AGAINST_GLM "Validate nme::math against GLM (fetches GLM)" ON)

if(NME_TEST_AGAINST_GLM)
    nme_declare(glm
            GIT_REPOSITORY https://github.com/g-truc/glm.git
            GIT_TAG        1.0.1
            GIT_SHALLOW    TRUE
            SOURCE_SUBDIR  __skip_glm_build__
    )
    FetchContent_MakeAvailable(glm)

    add_library(glm_header INTERFACE)
    target_include_directories(glm_header SYSTEM INTERFACE ${glm_SOURCE_DIR})
endif()

add_library(doctest_header INTERFACE)
target_include_directories(doctest_header SYSTEM INTERFACE ${doctest_SOURCE_DIR})

# doctest's CTest bridge - provides doctest_discover_tests().
include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)

# One executable per test TU. test_main.cpp is compiled *into* each exe (not a
# shared static archive) so a TEST_CASE's self-registration is never dropped by
# the linker. Each case is then discovered as its own CTest entry, namespaced
# by binary name.
function(nme_add_test name)
    add_executable(${name} test_main.cpp ${ARGN})
    target_link_libraries(${name} PRIVATE nme::engine doctest_header)
    set_target_properties(${name} PROPERTIES FOLDER "tests")
    nme_enable_sanitizers(${name})
    # nme::warnings intentionally omitted: doctest macros trip our -Werror set.
    doctest_discover_tests(${name} TEST_PREFIX "${name}:")
endfunction()

nme_add_test(test_version    test_version.cpp)
nme_add_test(test_mpmc_queue test_mpmc_queue.cpp)
nme_add_test(test_spsc_ring  test_spsc_ring.cpp)
nme_add_test(test_deque      test_deque.cpp)

nme_add_test(test_vec        test_vec.cpp)
nme_add_test(test_matrix     test_matrix.cpp)
if(NME_TEST_AGAINST_GLM)
    target_link_libraries(test_vec PRIVATE glm_header)
    target_compile_definitions(test_vec PRIVATE NME_TEST_WITH_GLM)
    target_link_libraries(test_matrix PRIVATE glm_header)
    target_compile_definitions(test_matrix PRIVATE NME_TEST_WITH_GLM)
endif()