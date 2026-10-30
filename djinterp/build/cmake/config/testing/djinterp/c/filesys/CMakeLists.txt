# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/c/filesys/CMakeLists.txt
#
#   Test leaf for the fs subframework's C unit tests (c/filesys/file_*), driven
# through the C-C++ DTest bridge.
#
#   RECIPE R2 (production sources compiled into the exe). The fs implementation
# in src/djinterp/c/filesys/*.c is compiled straight into the test executable,
# alongside the C test sections and the C++ runner. The DTest bridge and the
# core runtime support the fs code leans on (memory, configuration, the notify
# machinery) come from djinterp_c_runtime, which djinterp_add_test_executable
# links automatically. If a given build already carries src/djinterp/c/filesys in a
# shared library instead, delete the _prod glob below and name that library
# under LIBRARIES (Recipe R3) to keep a single source of truth.
#
#   The helper adds only inc/ to the include path, so this leaf adds BOTH the
# module's inc directory (for bare includes such as "file.h") and its tests
# directory (for "file_tests.h"), per the authoring guide.
#
#   TARGET NAME. `file_tests` is taken by the C++ leaf at core/fs, whose header
# has the same `file` stem; CMake target names are global, so this one is the
# qualified pair: `c_file_tests` here, `file_tests` there. The CTest name
# follows the target name.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.07.21
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/filesys")
set(_src_dir   "${DJINTERP_SRC_ROOT}/djinterp/c/filesys")

# The section translation units (C), one per like-group module. This glob also
# picks up file_tests_util.c (the shared environment helpers). The test header
# file_tests.h is not matched (.h), and the runner is a .cpp in THIS config
# directory -- added explicitly below -- so it is not matched either.
file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/file_tests_*.c")

# The standalone half of the suite (file_tests_sa*.c) shares this directory and
# its file_tests_ prefix; it runs under its own driver, below.
list(FILTER _sections EXCLUDE REGEX "/file_tests_(sa|sa_[a-z]+)\\.c$")

# The production sources under test (C).
file(GLOB _prod CONFIGURE_DEPENDS "${_src_dir}/*.c")

djinterp_add_test_executable(c_file_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/file_tests_runner.cpp"
        ${_sections}
        ${_prod}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/c/filesys"
        "${_tests_dir}")

# The standalone half of the suite: its sections under the standalone driver,
# which supplies main() and the test-object model the D_ASSERT_* macros build.
set(_driver "${DJINTERP_TESTS_ROOT}/djinterp/c/modules/test/test_standalone_min.c")
file(GLOB _standalone CONFIGURE_DEPENDS "${_tests_dir}/file_tests_sa*.c")

djinterp_add_test_executable(c_file_tests_sa
    SOURCES
        ${_standalone}
        "${_driver}"
        ${_prod}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/c/filesys"
        "${_tests_dir}")

# The dfile suite (c/filesys/dfile.h), under the same driver: its entry point
# is d_tests_dfile_run_all, which the driver calls under the suite's name, and
# it uses string_fn's d_strcpy_s, which stands on dmemory, as well.
file(GLOB _dfile CONFIGURE_DEPENDS
     "${DJINTERP_TESTS_ROOT}/djinterp/c/dfile_tests_sa*.c")

djinterp_add_test_executable(c_dfile_tests_sa
    SOURCES
        ${_dfile}
        "${_driver}"
        ${_prod}
        "${DJINTERP_SRC_ROOT}/djinterp/c/string_fn.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/memory/dmemory.c"
    DEFINES
        d_tests_file_run_all=d_tests_dfile_run_all
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/c/filesys"
        "${DJINTERP_TESTS_ROOT}/djinterp/c")

# Optional convenience: a group meta-target over this leaf (and any future
# child leaves under c/filesys). The name is globally unique across the tree.
djinterp_add_test_subtree(all_djinterp_c_filesys)
