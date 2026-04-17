###############################################################################
# djinterp - container/vector module
# 
# Build configuration for vector container tests.
# Includes ptr_vector, vector_common, and vector test executables.
#
# Location: <root>/build/cmake/config/c/container/vector/CMakeLists.txt
#
# author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.25
###############################################################################
cmake_minimum_required(VERSION 3.20)

###############################################################################
# DIRECTORY SETUP
###############################################################################

set(VECTOR_TEST_DIR "${TEST_DIR}/vector")
set(CONFIG_VECTOR_DIR "${CONFIG_TEST_DIR}/vector")

# Override TEST_DIR for djinterp_add_standalone_test
set(TEST_DIR "${VECTOR_TEST_DIR}")

message(STATUS "  ---- Vector Tests ----")
message(STATUS "  Vector test dir:     ${VECTOR_TEST_DIR}")
message(STATUS "  Vector config dir:   ${CONFIG_VECTOR_DIR}")

###############################################################################
# HELPER MACRO
###############################################################################

macro(_container_vector_add_test MODULE_NAME)
    set(_extra_args ${ARGN})
    djinterp_module_name_to_path("${MODULE_NAME}" _path_name)
    set(_main "${CONFIG_VECTOR_DIR}/djinterp-c-container-${_path_name}-tests-sa/main.c")
    if(EXISTS "${_main}")
        djinterp_add_standalone_test(
            MODULE_NAME ${MODULE_NAME}
            ${_extra_args}
            MAIN_FILE "${_main}"
        )
    else()
        djinterp_add_standalone_test(
            MODULE_NAME ${MODULE_NAME}
            ${_extra_args}
        )
    endif()
endmacro()

###############################################################################
# INDIVIDUAL TEST EXECUTABLES
###############################################################################

# ptr_vector tests
_container_vector_add_test(ptr_vector
    EXTRA_LIBS container)

# vector_common tests
_container_vector_add_test(vector_common
    EXTRA_LIBS container)

# vector tests
_container_vector_add_test(vector
    EXTRA_LIBS container)