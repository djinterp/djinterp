# =============================================================================
# djinterp [test]  -  build/cmake/config/testing/djinterp/cpp/util/document/document_bundle/CMakeLists.txt
#
#   Test leaf for cpp/util/document/document_bundle.hpp and the packaging it
# writes through, in the C spec form: the runner in this directory, the
# section translation unit from the tests tree, the compress and archive
# facades it packs with, and DTest's C face, from djinterp_c_test.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.23
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/util/document")
set(_src_dir   "${DJINTERP_SRC_ROOT}/djinterp/cpp/util")

file(GLOB _sections CONFIGURE_DEPENDS
    "${_tests_dir}/document_bundle_tests_*.cpp")

djinterp_add_test_executable(document_bundle_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/document_bundle_tests_runner.cpp"
        ${_sections}
        "${_src_dir}/archive/archive.cpp"
        "${_src_dir}/compress/compress.cpp"
    LIBRARIES
        djinterp_c_test
    INCLUDES
        "${_tests_dir}")
