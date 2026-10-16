# ============================================================================
# djinterp [test] : restd move-family test module wiring
#
#   Header-only leaf recipe for the restd move-family suite. The runner
# (move_family_tests_runner.cpp) lives beside this file; the per-section
# translation units live under the tests tree. Globbing the sections keeps the
# list maintenance-free as sections are added or removed.
#
#   This suite is cross-cutting: it exercises headers from several restd
# components (utility, algorithm, memory, iterator, type_traits), all reached
# from the standard include root that djinterp_add_test_executable adds
# automatically. The leaf therefore only needs the tests directory on the
# include path (for the bare "move_family_tests.hpp" include).
#
#   Slug: djinterp/restd/move_family
#     suite header : ${DJINTERP_TESTS_ROOT}/djinterp/restd/move_family/move_family_tests.hpp
#     sections     : ${DJINTERP_TESTS_ROOT}/djinterp/restd/move_family/move_family_tests_*.cpp
#     runner       : ${CMAKE_CURRENT_SOURCE_DIR}/move_family_tests_runner.cpp
# ============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/restd/move_family")

# every per-section translation unit for this suite
file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/move_family_tests_*.cpp")

# never sweep in a hand-rolled entry point
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

# The sections and runner include a bare "move_family_tests.hpp", and the suite
# header reaches the headers under test via their restd / djinterp include paths
# from the auto-added include root.
djinterp_add_test_executable(move_family_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/move_family_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${_tests_dir}")
