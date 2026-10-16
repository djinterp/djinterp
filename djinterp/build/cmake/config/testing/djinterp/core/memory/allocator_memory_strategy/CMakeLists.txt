# ==========================================================================
# djinterp allocator_memory_strategy_tests  (the std::allocator adapter)
#
# Per-module leaf recipe: this folder holds one executable and its runner.
# The section TUs are globbed from the tests tree; djinterp_add_test_executable
# adds the inc/ root and D_TESTING=1 and registers the CTest case.
# ==========================================================================

file(GLOB _sources CONFIGURE_DEPENDS
    "${DJINTERP_TESTS_ROOT}/djinterp/core/memory/allocator_memory_strategy_tests_*.cpp")

djinterp_add_test_executable(allocator_memory_strategy_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/allocator_memory_strategy_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/core/memory"
        "${DJINTERP_TESTS_ROOT}/djinterp/core/memory"
)
