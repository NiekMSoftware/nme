# Applies a strong, curated warning set to an INTERFACE target.
# Link (PRIVATE) into first-party targets only -- never third-party code.
function(nme_set_project_warnings target warnings_as_errors)

	set(MSVC_WARNINGS
		/W4 /permissive-
		/w14242 /w14254 /w14263 /w14265 /w14287 /we4289
        /w14296 /w14311 /w14545 /w14546 /w14547 /w14549
        /w14555 /w14619 /w14640 /w14826 /w14905 /w14906 /w14928)
	
	set(CLANG_WARNINGS
        -Wall -Wextra -Wpedantic
        -Wshadow -Wnon-virtual-dtor -Wold-style-cast -Wcast-align
        -Wunused -Woverloaded-virtual -Wconversion -Wsign-conversion
        -Wnull-dereference -Wdouble-promotion -Wformat=2 -Wimplicit-fallthrough)

    set(GCC_WARNINGS ${CLANG_WARNINGS}
        -Wmisleading-indentation -Wduplicated-cond
        -Wduplicated-branches -Wlogical-op -Wuseless-cast)

    if(warnings_as_errors)
        list(APPEND MSVC_WARNINGS  /WX)
        list(APPEND CLANG_WARNINGS -Werror)
        list(APPEND GCC_WARNINGS   -Werror)
    endif()

    if(MSVC)
        set(_warnings ${MSVC_WARNINGS})
    elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
        set(_warnings ${CLANG_WARNINGS})
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(_warnings ${GCC_WARNINGS})
    else()
        message(WARNING "nme_set_project_warnings: no flags for '${CMAKE_CXX_COMPILER_ID}'")
        set(_warnings "")
    endif()

    target_compile_options(${target} INTERFACE ${_warnings})

endfunction()