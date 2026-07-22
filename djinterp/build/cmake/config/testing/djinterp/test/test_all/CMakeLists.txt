# djinterp/test/test_all — the single-process umbrella exe (djinterp_tests_all).
#   Links the umbrella runner (this dir) + the 15 migrated suites' section TUs
# (tests/djinterp/test and .../output) + the two util TUs test_output's sections
# need at link (internal::compress_buffer / archive_create).  Emits one combined
# PDF.  ctest -R djinterp_tests_all, or build the target.
set(_tests_dir     "${DJINTERP_TESTS_ROOT}/djinterp/test")
set(_out_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/test/output")
set(_util_dir      "${DJINTERP_SRC_ROOT}/djinterp/core/util")

set(_all_src "")
foreach(_s test_common test_kind test_object test_tree test_session test_timer
           test_counter test_event test_traits test_defaults)
    file(GLOB _g CONFIGURE_DEPENDS "${_tests_dir}/${_s}_tests_*.cpp")
    list(APPEND _all_src ${_g})
endforeach()
foreach(_s test_output_config test_output test_render test_report)
    file(GLOB _g CONFIGURE_DEPENDS "${_out_tests_dir}/${_s}_tests_*.cpp")
    list(APPEND _all_src ${_g})
endforeach()
list(FILTER _all_src EXCLUDE REGEX "_tests_(runner|main)\\.cpp$")

djinterp_add_test_executable(djinterp_tests_all
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/tests_all_runner.cpp"
        ${_all_src}
        "${_util_dir}/compress.cpp"     # internal::compress_buffer
        "${_util_dir}/archive.cpp"      # internal::archive_create
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/test"
        "${DJINTERP_INC_ROOT}/djinterp/test/output"
        "${_tests_dir}"
        "${_out_tests_dir}")
