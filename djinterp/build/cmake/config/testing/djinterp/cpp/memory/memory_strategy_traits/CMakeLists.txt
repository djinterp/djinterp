# ==========================================================================
# djinterp memory_strategy_traits_tests  (the agnostic memory-strategy core contract)
#
# Per-module leaf recipe: this folder holds one executable and its runner.
# The section TUs are globbed from the tests tree; djinterp_add_test_executable
# adds the inc/ root and D_TESTING=1 and registers the CTest case.
# ==========================================================================

file(GLOB _sources CONFIGURE_DEPENDS
    "${DJINTERP_TESTS_ROOT}/djinterp/cpp/memory/memory_strategy_traits_tests_*.cpp")

djinterp_add_test_executable(memory_strategy_traits_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/memory_strategy_traits_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/cpp/memory"
        "${DJINTERP_TESTS_ROOT}/djinterp/cpp/memory"
)
