# ==========================================================================
# djinterp static_buffer_strategy_tests  (the inline element buffer + static-extent marker)
#
# Per-module leaf recipe: this folder holds one executable and its runner.
# The section TUs are globbed from the tests tree; djinterp_add_test_executable
# adds the inc/ root and D_TESTING=1 and registers the CTest case.
# ==========================================================================

file(GLOB _sources CONFIGURE_DEPENDS
    "${DJINTERP_TESTS_ROOT}/djinterp/cpp/memory/static_buffer_strategy_tests_*.cpp")

djinterp_add_test_executable(static_buffer_strategy_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/static_buffer_strategy_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/cpp/memory"
        "${DJINTERP_TESTS_ROOT}/djinterp/cpp/memory"
)
