# ==========================================================================
# djinterp memory_strategy_all_tests  (AGGREGATE: run_suite over all seven providers, one combined report)
#
# Per-module leaf recipe: this folder holds one executable and its runner.
# The section TUs are globbed from the tests tree; djinterp_add_test_executable
# adds the inc/ root and D_TESTING=1 and registers the CTest case.
# The aggregate links every module's section TUs (both tests dirs) and, via
# its runner, includes all seven suite headers - legal because each suite's
# check macro has a distinct suffix, so they co-compile in one TU.
# ==========================================================================

file(GLOB _src0 CONFIGURE_DEPENDS
    "${DJINTERP_TESTS_ROOT}/djinterp/cpp/memory/*_tests_*.cpp")
file(GLOB _src1 CONFIGURE_DEPENDS
    "${DJINTERP_TESTS_ROOT}/djinterp/cpp/container/traits/container_memory_strategy_tests_*.cpp")

djinterp_add_test_executable(memory_strategy_all_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/memory_strategy_all_tests_runner.cpp"
        ${_src0}
        ${_src1}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/cpp/memory"
        "${DJINTERP_INC_ROOT}/djinterp/cpp/container/traits"
        "${DJINTERP_INC_ROOT}/djinterp/core/memory/pool"
        "${DJINTERP_TESTS_ROOT}/djinterp/cpp/memory"
        "${DJINTERP_TESTS_ROOT}/djinterp/cpp/container/traits"
    DEFINES
        D_MS_TEST_POOL_LAYER=1
)
