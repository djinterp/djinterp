# =============================================================================
# testing/djinterp/core/container/array/
#   LEAF — builds array_tests: the array container family in one exe (base
# array + the threadsafe / atomic / cow wrappers + the array options machinery,
# Parts A / B / D of the suite).
#
#   Output binary lands at:
#     <root>/bin/<arch>-<Config>/testing/djinterp/core/container/array/array_tests.exe
#
#   NAMING EXCEPTION: this suite predates the "<stem>_tests_<section>.cpp"
# convention its siblings glob on.  Its section TUs are spelled
# "<component>_core_tests.cpp" and "array_options_<axis>_tests.cpp", so a
# "array_tests_*.cpp" glob would match NONE of them; the eight TUs are therefore
# named explicitly below (this is also why the directory shipped with its
# CMakeLists disabled).  The runner (array_tests_runner.cpp, this dir) owns
# main() and drives the handler-based master-suite template; the listed TUs
# define run_array_suite / run_threadsafe_array_suite / run_array_options_suite.
#
#   INCLUDES: the module inc dir for the bare "array.hpp" / "atomic_array.hpp" /
# "cow_array.hpp" / "threadsafe_array.hpp" / "array_options.hpp" family, and the
# suite's tests dir for the bare "array_tests.hpp".  The helper adds inc/ for
# <djinterp/core/djinterp.hpp> and the djinterp/test/* headers, links
# djinterp_c_runtime, sets the output dir, and registers CTest.
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/container/array")

djinterp_add_test_executable(array_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/array_tests_runner.cpp"
        "${_tests_dir}/array_core_tests.cpp"
        "${_tests_dir}/array_options_alias_tests.cpp"
        "${_tests_dir}/array_options_core_tests.cpp"
        "${_tests_dir}/array_options_edge_tests.cpp"
        "${_tests_dir}/array_options_factory_tests.cpp"
        "${_tests_dir}/atomic_array_core_tests.cpp"
        "${_tests_dir}/cow_array_core_tests.cpp"
        "${_tests_dir}/threadsafe_array_core_tests.cpp"
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/cpp/container/array"
        "${_tests_dir}")
