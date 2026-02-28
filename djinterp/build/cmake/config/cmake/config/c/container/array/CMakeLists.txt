###############################################################################
# djinterp - container/array module
# 
# Build configuration for array container tests.
# Includes array_common, array_filter, circular_array individual tests
# and one combined test executable.
#
# Location: <root>/build/cmake/config/c/container/array/CMakeLists.txt
#
# author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.25
###############################################################################
cmake_minimum_required(VERSION 3.20)

###############################################################################
# DIRECTORY SETUP
###############################################################################

# Array test files live under tests/c/container/array/
set(ARRAY_TEST_DIR "${TEST_DIR}/array")
set(CONFIG_ARRAY_DIR "${CONFIG_TEST_DIR}/array")

# Registry test dir needed for circular_array fix (see below)
set(REGISTRY_TEST_DIR "${C_TEST_BASE_DIR}/container/registry")

# Override TEST_DIR for djinterp_add_standalone_test
set(TEST_DIR "${ARRAY_TEST_DIR}")

# Print configuration
message(STATUS "  ---- Array Tests ----")
message(STATUS "  Array test dir:     ${ARRAY_TEST_DIR}")
message(STATUS "  Array config dir:   ${CONFIG_ARRAY_DIR}")

###############################################################################
# HELPER MACRO
###############################################################################

macro(_container_array_add_test MODULE_NAME)
    set(_extra_args ${ARGN})
    djinterp_module_name_to_path("${MODULE_NAME}" _path_name)
    set(_main "${CONFIG_ARRAY_DIR}/djinterp-c-container-${_path_name}-tests-sa/main.c")
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

# array_common tests
_container_array_add_test(array_common
    EXTRA_LIBS container)

# array_filter tests
_container_array_add_test(array_filter
    EXTRA_LIBS container filter predicate functional functional_common dio)

###############################################################################
# CIRCULAR ARRAY TEST EXECUTABLE (manual — bypasses macro)
#
# FIX: The auto-generated main.c at:
#   .config/.msvs/testing/c/container/array/djinterp-c-container-circular-array-tests-sa/main.c
# calls d_tests_sa_registry_common_run_all in addition to the circular_array runner.
# This symbol lives in tests/c/container/registry/registry_common_tests_sa*.c.
#
# Because EXTRA_SOURCES is parsed but not forwarded to add_executable() in
# djinterp_add_standalone_test, we build this target manually so we can
# include the registry_common test sources directly.
###############################################################################

set(CIRC_ARRAY_MAIN "${CONFIG_ARRAY_DIR}/djinterp-c-container-circular-array-tests-sa/main.c")
if(EXISTS "${CIRC_ARRAY_MAIN}")
    # Gather circular_array test files
    file(GLOB CIRC_ARRAY_TEST_FILES "${ARRAY_TEST_DIR}/circular_array_tests_sa*.c")

    # Gather registry_common test files needed by main.c
    file(GLOB REGISTRY_COMMON_TEST_FILES "${REGISTRY_TEST_DIR}/registry_common_tests_sa*.c")

    set(CIRC_ARRAY_TARGET "djinterp-c-circular-array-tests-sa-${PLATFORM_ARCH}")

    add_executable(${CIRC_ARRAY_TARGET}
        "${CIRC_ARRAY_MAIN}"
        ${CIRC_ARRAY_TEST_FILES}
        ${REGISTRY_COMMON_TEST_FILES}
    )

    target_include_directories(${CIRC_ARRAY_TARGET} PRIVATE
        ${INCLUDE_DIR}
        ${TEST_FRAMEWORK_INC_DIR}
        ${C_TEST_BASE_DIR}
        ${ARRAY_TEST_DIR}
        ${REGISTRY_TEST_DIR}
    )

    target_compile_definitions(${CIRC_ARRAY_TARGET} PRIVATE D_TESTING=1)

    if(MSVC)
        target_compile_options(${CIRC_ARRAY_TARGET} PRIVATE /FS)
        set_target_properties(${CIRC_ARRAY_TARGET} PROPERTIES
            COMPILE_PDB_NAME "${CIRC_ARRAY_TARGET}"
            COMPILE_PDB_OUTPUT_DIRECTORY
                "${CMAKE_CURRENT_BINARY_DIR}/${CIRC_ARRAY_TARGET}.dir"
        )
    endif()

    target_link_libraries(${CIRC_ARRAY_TARGET} PRIVATE
        test-standalone-support
        container
    )

    message(STATUS "  Created test executable: ${CIRC_ARRAY_TARGET}")
else()
    message(STATUS "  Skipping circular_array tests (no main.c at ${CIRC_ARRAY_MAIN})")
endif()

###############################################################################
# COMBINED TEST EXECUTABLE: array-tests-sa-all
###############################################################################

set(ARRAY_ALL_MAIN "${CONFIG_ARRAY_DIR}/djinterp-c-container-array-tests-sa-all/main.c")
if(EXISTS "${ARRAY_ALL_MAIN}")
    # Gather ALL array test files
    file(GLOB ARRAY_ALL_TEST_FILES "${ARRAY_TEST_DIR}/*_tests_sa*.c")

    # FIX: Filter out _main.c files AND *_helpers.c files.
    #      array_tests_sa_helpers.c defines d_test_int_comparator, which is also defined in
    #      array_common_tests_sa.c → LNK2005 "multiply defined symbol" (LNK1169).
    #      *_helpers.c files contain shared test utilities, not standalone test runners,
    #      and must be excluded from the combined target to avoid the duplicate definition.
    set(ARRAY_ALL_FILTERED "")
    foreach(FILE ${ARRAY_ALL_TEST_FILES})
        get_filename_component(FILENAME "${FILE}" NAME)
        if(NOT FILENAME MATCHES "_main\\.c$" AND NOT FILENAME MATCHES "_helpers\\.c$")
            list(APPEND ARRAY_ALL_FILTERED "${FILE}")
        endif()
    endforeach()

    set(ARRAY_ALL_TARGET "djinterp-c-container-array-tests-sa-all-${PLATFORM_ARCH}")

    add_executable(${ARRAY_ALL_TARGET}
        "${ARRAY_ALL_MAIN}"
        ${ARRAY_ALL_FILTERED}
    )

    target_include_directories(${ARRAY_ALL_TARGET} PRIVATE
        ${INCLUDE_DIR}
        ${TEST_FRAMEWORK_INC_DIR}
        ${C_TEST_BASE_DIR}
        ${ARRAY_TEST_DIR}
    )

    target_compile_definitions(${ARRAY_ALL_TARGET} PRIVATE D_TESTING=1)

    if(MSVC)
        target_compile_options(${ARRAY_ALL_TARGET} PRIVATE /FS)
        set_target_properties(${ARRAY_ALL_TARGET} PROPERTIES
            COMPILE_PDB_NAME "${ARRAY_ALL_TARGET}"
            COMPILE_PDB_OUTPUT_DIRECTORY
                "${CMAKE_CURRENT_BINARY_DIR}/${ARRAY_ALL_TARGET}.dir"
        )
    endif()

    target_link_libraries(${ARRAY_ALL_TARGET} PRIVATE
        test-standalone-support
        container
        filter predicate functional functional_common dio
    )

    message(STATUS "  Created test executable: ${ARRAY_ALL_TARGET}")
else()
    message(STATUS "  Skipping array-tests-sa-all (no main.c at ${ARRAY_ALL_MAIN})")
endif()
