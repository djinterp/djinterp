###############################################################################
# djinterp - container/map module
# 
# Build configuration for map container tests.
# Includes enum_map_entry, min_enum_map, dictionary, and enum_map test
# executables.
#
# Location: <root>/build/cmake/config/c/container/map/CMakeLists.txt
#
# author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.18
###############################################################################
cmake_minimum_required(VERSION 3.20)

###############################################################################
# DIRECTORY SETUP
###############################################################################

# Map test files live under tests/c/container/map/
set(MAP_TEST_DIR "${TEST_DIR}/map")
set(CONFIG_MAP_DIR "${CONFIG_TEST_DIR}/map")

# Override TEST_DIR for djinterp_add_standalone_test
set(TEST_DIR "${MAP_TEST_DIR}")

# Print configuration
message(STATUS "  ---- Map Tests ----")
message(STATUS "  Map test dir:     ${MAP_TEST_DIR}")
message(STATUS "  Map config dir:   ${CONFIG_MAP_DIR}")

###############################################################################
# HELPER MACRO
###############################################################################

macro(_container_map_add_test MODULE_NAME)
    set(_extra_args ${ARGN})
    djinterp_module_name_to_path("${MODULE_NAME}" _path_name)
    set(_main "${CONFIG_MAP_DIR}/djinterp-c-container-${_path_name}-tests-sa/main.c")
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

# enum_map_entry tests
_container_map_add_test(enum_map_entry
    EXTRA_LIBS container)

# min_enum_map tests
_container_map_add_test(min_enum_map
    EXTRA_LIBS container)

###############################################################################
# SPECIAL-CASE TEST EXECUTABLES
#
# These have non-standard .config directory naming.
###############################################################################

# dictionary tests (config: djinterp-c-container-dictionary-tests)
set(DICTIONARY_MAIN "${CONFIG_MAP_DIR}/djinterp-c-container-dictionary-tests/main.c")
if(EXISTS "${DICTIONARY_MAIN}")
    file(GLOB DICTIONARY_TEST_FILES "${MAP_TEST_DIR}/dictionary_tests_sa*.c")

    set(DICTIONARY_FILTERED "")
    foreach(FILE ${DICTIONARY_TEST_FILES})
        get_filename_component(FILENAME "${FILE}" NAME)
        if(NOT FILENAME MATCHES "_main\\.c$")
            list(APPEND DICTIONARY_FILTERED "${FILE}")
        endif()
    endforeach()

    if(DICTIONARY_FILTERED)
        set(DICTIONARY_TARGET "djinterp-c-container-dictionary-tests-${PLATFORM_ARCH}")

        add_executable(${DICTIONARY_TARGET}
            "${DICTIONARY_MAIN}"
            ${DICTIONARY_FILTERED}
        )

        target_include_directories(${DICTIONARY_TARGET} PRIVATE
            ${INCLUDE_DIR}
            ${TEST_FRAMEWORK_INC_DIR}
            ${C_TEST_BASE_DIR}
            ${MAP_TEST_DIR}
        )

        target_compile_definitions(${DICTIONARY_TARGET} PRIVATE D_TESTING=1)

        if(MSVC)
            target_compile_options(${DICTIONARY_TARGET} PRIVATE /FS)
            set_target_properties(${DICTIONARY_TARGET} PROPERTIES
                COMPILE_PDB_NAME "${DICTIONARY_TARGET}"
                COMPILE_PDB_OUTPUT_DIRECTORY
                    "${CMAKE_CURRENT_BINARY_DIR}/${DICTIONARY_TARGET}.dir"
            )
        endif()

        target_link_libraries(${DICTIONARY_TARGET} PRIVATE
            test-standalone-support
            container
        )

        message(STATUS "  Created test executable: ${DICTIONARY_TARGET}")
    endif()
else()
    message(STATUS "  Skipping dictionary tests (no main.c at ${DICTIONARY_MAIN})")
endif()

# enum_map tests (config: djinterp-c-enum-map-tests-standalone — no "container-" prefix)
set(ENUM_MAP_MAIN "${CONFIG_MAP_DIR}/djinterp-c-enum-map-tests-standalone/main.c")
if(EXISTS "${ENUM_MAP_MAIN}")
    file(GLOB ENUM_MAP_TEST_FILES "${MAP_TEST_DIR}/enum_map_tests_sa*.c")

    set(ENUM_MAP_FILTERED "")
    foreach(FILE ${ENUM_MAP_TEST_FILES})
        get_filename_component(FILENAME "${FILE}" NAME)
        if(NOT FILENAME MATCHES "_main\\.c$")
            list(APPEND ENUM_MAP_FILTERED "${FILE}")
        endif()
    endforeach()

    if(ENUM_MAP_FILTERED)
        set(ENUM_MAP_TARGET "djinterp-c-enum-map-tests-standalone-${PLATFORM_ARCH}")

        add_executable(${ENUM_MAP_TARGET}
            "${ENUM_MAP_MAIN}"
            ${ENUM_MAP_FILTERED}
        )

        target_include_directories(${ENUM_MAP_TARGET} PRIVATE
            ${INCLUDE_DIR}
            ${TEST_FRAMEWORK_INC_DIR}
            ${C_TEST_BASE_DIR}
            ${MAP_TEST_DIR}
        )

        target_compile_definitions(${ENUM_MAP_TARGET} PRIVATE D_TESTING=1)

        if(MSVC)
            target_compile_options(${ENUM_MAP_TARGET} PRIVATE /FS)
            set_target_properties(${ENUM_MAP_TARGET} PROPERTIES
                COMPILE_PDB_NAME "${ENUM_MAP_TARGET}"
                COMPILE_PDB_OUTPUT_DIRECTORY
                    "${CMAKE_CURRENT_BINARY_DIR}/${ENUM_MAP_TARGET}.dir"
            )
        endif()

        target_link_libraries(${ENUM_MAP_TARGET} PRIVATE
            test-standalone-support
            container
        )

        message(STATUS "  Created test executable: ${ENUM_MAP_TARGET}")
    endif()
else()
    message(STATUS "  Skipping enum_map tests (no main.c at ${ENUM_MAP_MAIN})")
endif()
