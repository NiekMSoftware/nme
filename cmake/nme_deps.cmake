include(FetchContent)

set(NME_DEPS_CACHE "${CMAKE_SOURCE_DIR}/.cache/deps"
    CACHE PATH "Shared source cache for fetched deps")

function(nme_declare name)
    string(TOUPPER "${name}" _u)
    set(_src "${NME_DEPS_CACHE}/${name}-src")

    file(GLOB _have "${_src}/*")
    if (_have)
        # Already cloned -> point at it, skip the download step (fast).
        set(FETCHCONTENT_SOURCE_DIR_${_u} "${_src}" CACHE PATH "" FORCE)
        FetchContent_Declare(${name} ${ARGN})
    else ()
        # First time on this machine -> clone into the shared cache.
        FetchContent_Declare(${name} ${ARGN} SOURCE_DIR "${_src}")
    endif ()
endfunction()