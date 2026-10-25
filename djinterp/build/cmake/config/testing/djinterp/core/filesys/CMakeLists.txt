# =============================================================================
# djinterp [test] -- build/cmake/config/testing/djinterp/core/filesys/CMakeLists.txt
#
#   Test leaf for the C++ filesystem layer (core/filesys/file_*.hpp), driven through
# the DTest framework's native C++ builder.
#
#   RECIPE R1 (header-only module). The C++ layer is entirely inline -- there
# is no src/djinterp/core/filesys to compile -- so the executable is the runner plus
# the section translation units. The C modules the layer forwards to, and the
# d_string it builds paths on, come from djinterp_c_runtime, which
# djinterp_add_test_executable links automatically.
#
#   >>> TARGET NAME. Both c/filesys/dfile.h and core/filesys/dfile.hpp have the stem
#   `file`, and CMake target names are global, so the two suites cannot both
#   be `file_tests`. The C++ layer keeps the plain name; the C suite is the
#   qualified one. This leaf therefore builds `file_tests`, and the c/filesys leaf
#   must be changed to `c_file_tests` to match -- a one-word edit in its own
#   djinterp_add_test_executable call. The CTest name follows the target name
#   in both cases. <<<
#
#   The helper adds only inc/, so this leaf adds BOTH the layer's own include
# directory (for the bare "file.hpp") and its tests directory (for
# "file_tests.hpp"). The framework header the suite uses,
# "djinterp/test/test_builder.hpp", is full-from-inc and needs no entry.
#
#   C++23. The file_expected.hpp section compiles to nothing below C++23 --
# it gates on __cpp_lib_expected, so nothing here needs changing to keep it
# inert. To exercise it, raise the standard for this target:
#
#     target_compile_features(file_tests PRIVATE cxx_std_23)
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.07.22
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/core/filesys")

# The section translation units, one per module, plus the shared environment
# helpers in file_tests_util.cpp. The test header is a .hpp and the runner is a
# .cpp in THIS config directory, so neither is matched here.
file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/file_tests_*.cpp")

# What the C++ layer stands on, which djinterp_c_runtime does not carry: the C
# layer, and the text and memory support its path type uses.
file(GLOB _c_layer CONFIGURE_DEPENDS "${DJINTERP_SRC_ROOT}/djinterp/c/filesys/*.c")
set(_support
    "${DJINTERP_SRC_ROOT}/djinterp/c/dstring.c"
    "${DJINTERP_SRC_ROOT}/djinterp/c/memory/dmemory.c"
    "${DJINTERP_SRC_ROOT}/djinterp/c/string_fn.c")

# Guard against an older hand-rolled entry point being swept in beside the
# runner: two main()s collide at link.
# file_tests_sa.cpp is one of the standalone programs below, with a main()
# of its own
list(FILTER _sections EXCLUDE REGEX "_(main|sa)\\.cpp$")

djinterp_add_test_executable(file_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/file_tests_runner.cpp"
        ${_sections}
        ${_c_layer}
        ${_support}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/core/filesys"
        "${_tests_dir}")

# Group target over this leaf and any future ones under core/fs. The name is
# globally unique, as aggregates must be.
# The standalone programs: one per header, and file_tests_sa.cpp over all of
# them. Each carries its own main() and assertions.
file(GLOB _programs CONFIGURE_DEPENDS "${_tests_dir}/file_*tests_sa.cpp")

foreach(_program ${_programs})
    get_filename_component(_stem "${_program}" NAME_WE)

    djinterp_add_test_executable(core_${_stem}
        SOURCES
            "${_program}"
            ${_c_layer}
            ${_support}
        INCLUDES
            "${DJINTERP_INC_ROOT}/djinterp/core/filesys")
endforeach()

djinterp_add_test_subtree(all_djinterp_core_filesys)
