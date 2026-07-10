# ============================================================================
# djinterp [test] : leaf CMakeLists for djinterp/paradigm/adapter
#
#   Header-only module (no src/), so this follows Recipe R1: glob the section
# TUs from the tests tree, prepend the runner (which owns main()), and give the
# target both the module's inc dir and its tests dir so the bare
#   #include "adapter.hpp" / #include "adapter_tests.hpp"
# forms resolve.  The helper supplies inc/ and the output dir automatically.
# ============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/paradigm/adapter")

file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/adapter_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(adapter_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/adapter_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/paradigm/adapter"
        "${_tests_dir}")
