# =============================================================================
# testing/djinterp/core/container/array/
#   Leaf module: builds array_tests.exe.
#
#   Output binary lands at:
#     <root>/bin/<arch>-<Config>/testing/djinterp/core/container/array/array_tests.exe
#
#   To clone for a new module, copy this file to the matching path under
# build/cmake/config/testing/djinterp/<rel>/<module>/, edit the source list,
# and (optionally) update DJINTERP_TESTS_ROOT path to point at the right
# tests/<rel>/<module>/ directory.  No other change required.
# =============================================================================

djinterp_add_test_executable(array_tests
    SOURCES
        # entry point lives next to this CMakeLists
        main.cpp

        # test sources live under <root>/tests/, mirrored at the same rel path
        "${DJINTERP_TESTS_ROOT}/djinterp/core/container/array/array_core_tests.cpp"
        "${DJINTERP_TESTS_ROOT}/djinterp/core/container/array/atomic_array_core_tests.cpp"
        "${DJINTERP_TESTS_ROOT}/djinterp/core/container/array/cow_array_core_tests.cpp"

        # header listed for IDE solution explorer convenience
        "${DJINTERP_TESTS_ROOT}/djinterp/core/container/array/array_tests.hpp"
)
