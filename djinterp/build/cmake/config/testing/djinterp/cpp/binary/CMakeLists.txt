# djinterp/core/binary - serialization test cluster (recipe R5)
#
#   One folder, four header-only leaf suites (encode, decode, encode_options,
# decode_options) plus a group target over them.  Each suite is a runner
# (main(), here in the config tree) linked with its globbed section TUs (which
# live in tests/ and define the flat djinterp::testing predicates).  Every leaf
# adds BOTH its inc/ slug dir and its tests/ slug dir to INCLUDES so the bare
# includes in the runner, the test header, and the module headers resolve; the
# helper supplies inc/ (the root) and D_TESTING=1 on its own.

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/binary")


# binary_suite
#   define one header-only test executable <name>_tests from its runner and the
# section TUs matching <name>_tests_*.cpp (the *_main.cpp guard is kept for
# parity with the template though this tree has none).
function(binary_suite _name)
    file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/${_name}_tests_*.cpp")
    list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

    djinterp_add_test_executable(${_name}_tests
        SOURCES
            "${CMAKE_CURRENT_SOURCE_DIR}/${_name}_tests_runner.cpp"
            ${_sources}
        INCLUDES
            "${DJINTERP_INC_ROOT}/djinterp/cpp/binary"
            "${_tests_dir}")
endfunction()


binary_suite(encode)
binary_suite(decode)
binary_suite(encode_options)
binary_suite(decode_options)


# group target over the four suites above (globally-unique name).  Higher
# aggregates (all_djinterp_core, all_djinterp, all_tests) recurse into this dir
# on their own and need no wiring here.
djinterp_add_test_subtree(all_binary_serial)
