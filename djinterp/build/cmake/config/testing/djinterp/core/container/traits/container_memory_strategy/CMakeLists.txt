# =============================================================================
# testing/djinterp/core/container/traits/  ::  container_memory_strategy
#                                              (LEAF, AGENTS-cmake R1)
#
#   Builds container_memory_strategy_tests from the runner in THIS directory
# plus the section TUs globbed from tests/djinterp/core/container/traits/.
#
#   The suite tests the container-shape -> memory-strategy binding, which lives
# in the container subsystem (inc/djinterp/core/container/traits), not under
# memory/ - so the config dir mirrors the tests dir, per the four-mirrored-roots
# rule.  The suite header includes the shared memory-strategy fixtures (whose
# header sits under tests/.../memory/strategy) and, transitively, the
# traits/concepts/concrete strategy headers, so both strategy roots ride along
# on the include path.
#
#   Runnable alone: ctest -R container_memory_strategy_tests
# =============================================================================

set(_tests_dir       "${DJINTERP_TESTS_ROOT}/djinterp/core/container/traits")
set(_strat_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/core/memory")

file(GLOB _sources CONFIGURE_DEPENDS
     "${_tests_dir}/container_memory_strategy_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(container_memory_strategy_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/container_memory_strategy_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/core/container/traits"  # binding under test
        "${DJINTERP_INC_ROOT}/djinterp/core/memory"   # traits/concepts/concretes
        "${_tests_dir}"                                        # this suite's header
        "${_strat_tests_dir}")                                 # shared fixtures header
