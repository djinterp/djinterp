###############################################################################
# djinterp - container/registry module
# 
# Build configuration for registry container tests.
# Includes registry_common and registry individual tests plus a combined
# test executable.
#
# Location: <root>/build/cmake/config/c/container/registry/CMakeLists.txt
#
# author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.25
###############################################################################
cmake_minimum_required(VERSION 3.20)

###############################################################################
# DIRECTORY SETUP
###############################################################################

set(REGISTRY_TEST_DIR "${TEST_DIR}/registry")
set(CONFIG_REGISTRY_DIR "${CONFIG_TEST_DIR}/registry")

# Override TEST_DIR for djinterp_add_standalone_test
set(TEST_DIR "${REGISTRY_TEST_DIR}")

message(STATUS "  ---- Registry Tests ----")
message(STATUS "  Registry test dir:     ${REGISTRY_TEST_DIR}")
message(STATUS "  Registry config dir:   ${CONFIG_REGISTRY_DIR}")

###############################################################################
# HELPER MACRO
###############################################################################

macro(_container_registry_add_test MODULE_NAME)
    set(_extra_args ${ARGN})
    djinterp_module_name_to_path("${MODULE_NAME}" _path_name)
    set(_main "${CONFIG_REGISTRY_DIR}/djinterp-c-container-${_path_name}-tests-sa/main.c")
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

# registry_common tests
_container_registry_add_test(registry_common
    EXTRA_LIBS container)

# registry tests
_container_registry_add_test(registry
    EXTRA_LIBS container)

###############################################################################
# COMBINED TEST EXECUTABLE: registry-tests-sa-all
###############################################################################

set(REGISTRY_ALL_MAIN "${CONFIG_REGISTRY_DIR}/djinterp-c-container-registry-tests-sa-all/main.c")
if(EXISTS "${REGISTRY_ALL_MAIN}")
    file(GLOB REGISTRY_ALL_TEST_FILES "${REGISTRY_TEST_DIR}/*_tests_sa*.c")

    set(REGISTRY_ALL_FILTERED "")
    foreach(FILE ${REGISTRY_ALL_TEST_FILES})
        get_filename_component(FILENAME "${FILE}" NAME)
        if(NOT FILENAME MATCHES "_main\\.c$")
            list(APPEND REGISTRY_ALL_FILTERED "${FILE}")
        endif()
    endforeach()

    set(REGISTRY_ALL_TARGET "djinterp-c-container-registry-tests-sa-all-${PLATFORM_ARCH}")

    add_executable(${REGISTRY_ALL_TARGET}
        "${REGISTRY_ALL_MAIN}"
        ${REGISTRY_ALL_FILTERED}
    )

    target_include_directories(${REGISTRY_ALL_TARGET} PRIVATE
        ${INCLUDE_DIR}
        ${TEST_FRAMEWORK_INC_DIR}
        ${C_TEST_BASE_DIR}
        ${REGISTRY_TEST_DIR}
    )

    target_compile_definitions(${REGISTRY_ALL_TARGET} PRIVATE D_TESTING=1)

    if(MSVC)
        target_compile_options(${REGISTRY_ALL_TARGET} PRIVATE /FS)
        set_target_properties(${REGISTRY_ALL_TARGET} PROPERTIES
            COMPILE_PDB_NAME "${REGISTRY_ALL_TARGET}"
            COMPILE_PDB_OUTPUT_DIRECTORY
                "${CMAKE_CURRENT_BINARY_DIR}/${REGISTRY_ALL_TARGET}.dir"
        )
    endif()

    target_link_libraries(${REGISTRY_ALL_TARGET} PRIVATE
        test-standalone-support
        container
    )

    message(STATUS "  Created test executable: ${REGISTRY_ALL_TARGET}")
else()
    message(STATUS "  Skipping registry-tests-sa-all (no main.c at ${REGISTRY_ALL_MAIN})")
endif()
