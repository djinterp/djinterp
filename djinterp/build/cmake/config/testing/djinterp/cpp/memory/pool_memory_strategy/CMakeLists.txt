# ==========================================================================
# djinterp pool_memory_strategy_tests  (the pool-backed strategy (opt-in: D_MS_TEST_POOL_LAYER))
#
# Per-module leaf recipe: this folder holds one executable and its runner.
# The section TUs are globbed from the tests tree; djinterp_add_test_executable
# adds the inc/ root and D_TESTING=1 and registers the CTest case.
# ==========================================================================

file(GLOB _sources CONFIGURE_DEPENDS
    "${DJINTERP_TESTS_ROOT}/djinterp/cpp/memory/pool_memory_strategy_tests_*.cpp")

djinterp_add_test_executable(pool_memory_strategy_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/pool_memory_strategy_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/cpp/memory"
        "${DJINTERP_INC_ROOT}/djinterp/core/memory/pool"
        "${DJINTERP_TESTS_ROOT}/djinterp/cpp/memory"
    DEFINES
        D_MS_TEST_POOL_LAYER=1
)
