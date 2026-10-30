# archive.hpp / archive.cpp unit-test leaf.
#
# Builds the `archive_tests` executable from the runner (this config dir), the
# section translation units (tests/), and the module's production sources
# (src/).  djinterp_add_test_executable links djinterp_c_runtime, defines
# D_TESTING=1, sets the per-config output dir, and registers the exe with CTest
# (run it with `ctest -R archive_tests`).  Any ancestor djinterp_add_test_subtree
# picks this leaf up automatically -- no extra wiring here.
#
# path: /build/cmake/config/testing/djinterp/core/util/CMakeLists.txt

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/util")
set(_src_dir   "${DJINTERP_SRC_ROOT}/djinterp/cpp/util")

# section TUs (one per module section).  Exclude any stray *_main.cpp so it can
# never collide with the runner's main() at link time.
file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/archive_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

# Production sources compiled into this exe (recipe R2, approach A).  Within
# util/, archive.cpp's only production dependency is compress.cpp, so the two
# are named explicitly rather than globbing "${_src_dir}/*.cpp" -- util/ also
# holds other components' facades that this suite must not drag in.  The C env
# sources (env_archive.c / env_compress.c) and the configured codec backend link
# arrive via djinterp_c_runtime, which the helper links automatically.
set(_prod
    "${_src_dir}/archive.cpp"
    "${_src_dir}/compress.cpp")

djinterp_add_test_executable(archive_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/archive_tests_runner.cpp"
        ${_sources}
        ${_prod}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/cpp/util"
        "${_tests_dir}")
