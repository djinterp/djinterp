################################################################################
# djinterp [test]                                                  CMakeLists.txt
#
#   Test leaf for the djinterp/core/util archive_options vocabulary.
#
#     archive_options_tests   archive_options.hpp vocabulary + what the facade
#                             actually honours
#
#   Recipe R2 (approach A): the suite drives the facade, so archive.cpp and its
# only production dependency, compress.cpp, are compiled straight into the exe.
# They are named explicitly rather than globbed - util/ also holds other
# components' facades that this suite must not drag in.  The C env sources
# (env_archive.c / env_compress.c) and the configured codec backend link arrive
# via djinterp_c_runtime, which the helper links automatically.
#
#   NOTE: this file used to be an R5 cluster building archive_tests here too,
# from back when both components shared one directory at .../core/util/.  That
# suite now has its own leaf at ../archive/, which owns archive_tests_runner.cpp
# - a runner this directory does not contain.  Keeping the cluster here
# duplicated the target name (fatal once util/ gains an aggregate) and named a
# source that does not exist.
#
# path: /build/cmake/config/testing/djinterp/core/util/archive_options/
#       CMakeLists.txt
################################################################################

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/util")
set(_src_dir   "${DJINTERP_SRC_ROOT}/djinterp/cpp/util")

file(GLOB _sections CONFIGURE_DEPENDS
     "${_tests_dir}/archive_options_tests_*.cpp")
list(FILTER _sections EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(archive_options_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/archive_options_tests_runner.cpp"
        ${_sections}
        "${_src_dir}/archive.cpp"
        "${_src_dir}/compress.cpp"
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/cpp/util"
        "${_tests_dir}")
