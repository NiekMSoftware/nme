# vendors/

Hand-vendored third-party SDKs and middleware -- the "3rd Party SDKs" layer of
Gregory's runtime architecture (sec. 1.5). Drop libraries you want to check into
the repo here (e.g. Eigen, FreeType, a physics SDK) and `add_subdirectory` /
`target_link_libraries` them from `engine/CMakeLists.txt`.

Libraries pulled at configure time instead (doctest, optionally Tracy) use CMake
FetchContent and land under `out/.../_deps/`, so they are not committed here.
