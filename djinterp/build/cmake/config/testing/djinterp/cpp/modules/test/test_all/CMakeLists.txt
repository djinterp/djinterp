# djinterp/test/test_all — the single-process umbrella exe (djinterp_tests_all).
#   Links the umbrella runner (this dir) + the 12 migrated suites' section TUs
# (tests/djinterp/cpp/modules/test and .../output) + the two util TUs the
# report's packaging needs at link (internal::compress_buffer /
# archive_create).  Emits one combined PDF.  ctest -R djinterp_tests_all, or
# build the target.
set(_tests_dir     "${DJINTERP_TESTS_ROOT}/djinterp/cpp/modules/test")
set(_out_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/modules/test/output")
set(_util_dir      "${DJINTERP_SRC_ROOT}/djinterp/cpp/util")

set(_all_src "")
foreach(_s test_common test_kind test_object test_tree test_session test_timer
           test_counter test_event test_traits test_defaults)
    file(GLOB _g CONFIGURE_DEPENDS "${_tests_dir}/${_s}_tests_*.cpp")
    list(APPEND _all_src ${_g})
endforeach()
foreach(_s test_report)
    file(GLOB _g CONFIGURE_DEPENDS "${_out_tests_dir}/${_s}_tests_*.cpp")
    list(APPEND _all_src ${_g})
endforeach()
list(FILTER _all_src EXCLUDE REGEX "_tests_(runner|main)\\.cpp$")

djinterp_add_test_executable(djinterp_tests_all
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/tests_all_runner.cpp"
        ${_all_src}
        "${_util_dir}/compress/compress.cpp"  # internal::compress_buffer
        "${_util_dir}/archive/archive.cpp"    # internal::archive_create
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/cpp/modules/test"
        "${DJINTERP_INC_ROOT}/djinterp/cpp/modules/test/output"
        "${_tests_dir}"
        "${_out_tests_dir}")
