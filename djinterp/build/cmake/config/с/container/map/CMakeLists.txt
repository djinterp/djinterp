###############################################################################
# djinterp - container/map module
# 
# Build configuration for map container tests.
# Includes enum_map_entry and min_enum_map test executables.
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

# .config directory for main.c files
set(CONFIG_MAP_DIR "${CONTAINER_ROOT}/.config/.msvs/testing/c/container/map")

# Print configuration
message(STATUS "  ---- Map Tests ----")
message(STATUS "  Map test dir:     ${MAP_TEST_DIR}")
message(STATUS "  Map config dir:   ${CONFIG_MAP_DIR}")

###############################################################################
# enum_map_entry tests
###############################################################################

set(ENUM_MAP_ENTRY_MAIN "${CONFIG_MAP_DIR}/djinterp-c-container-enum-map-entry-tests-sa/main.c")
if(EXISTS "${ENUM_MAP_ENTRY_MAIN}")
    # Gather enum_map_entry test files
    file(GLOB ENUM_MAP_ENTRY_TEST_FILES
        "${MAP_TEST_DIR}/enum_map_entry_tests_sa*.c"
        "${MAP_TEST_DIR}/enum_map_tests_sa*.c"
    )

    # Filter out _main.c files
    set(ENUM_MAP_ENTRY_FILTERED "")
    foreach(FILE ${ENUM_MAP_ENTRY_TEST_FILES})
        get_filename_component(FILENAME "${FILE}" NAME)
        if(NOT FILENAME MATCHES "_main\\.c$")
            list(APPEND ENUM_MAP_ENTRY_FILTERED "${FILE}")
        endif()
    endforeach()

    set(ENUM_MAP_ENTRY_TARGET "djinterp-c-container-enum-map-entry-tests-sa-${PLATFORM_ARCH}")

    add_executable(${ENUM_MAP_ENTRY_TARGET}
        "${ENUM_MAP_ENTRY_MAIN}"
        ${ENUM_MAP_ENTRY_FILTERED}
    )

    target_include_directories(${ENUM_MAP_ENTRY_TARGET} PRIVATE
        ${INCLUDE_DIR}
        ${TEST_FRAMEWORK_INC_DIR}
        ${CORE_TEST_DIR}
        ${TEST_DIR}
        ${MAP_TEST_DIR}
    )

    target_compile_definitions(${ENUM_MAP_ENTRY_TARGET} PRIVATE D_TESTING=1)

    if(MSVC)
        target_compile_options(${ENUM_MAP_ENTRY_TARGET} PRIVATE /FS)
        set_target_properties(${ENUM_MAP_ENTRY_TARGET} PROPERTIES
            COMPILE_PDB_NAME "${ENUM_MAP_ENTRY_TARGET}"
            COMPILE_PDB_OUTPUT_DIRECTORY
                "${CMAKE_CURRENT_BINARY_DIR}/${ENUM_MAP_ENTRY_TARGET}.dir"
        )
    endif()

    target_link_libraries(${ENUM_MAP_ENTRY_TARGET} PRIVATE
        test-standalone-support
        container
    )

    message(STATUS "  Created test executable: ${ENUM_MAP_ENTRY_TARGET}")
else()
    message(STATUS "  Skipping enum_map_entry tests (no main.c at ${ENUM_MAP_ENTRY_MAIN})")
endif()

###############################################################################
# min_enum_map tests
###############################################################################

set(MIN_ENUM_MAP_MAIN "${CONFIG_MAP_DIR}/djinterp-c-container-min-enum-map-tests-sa/main.c")
if(EXISTS "${MIN_ENUM_MAP_MAIN}")
    # Gather min_enum_map test files
    file(GLOB MIN_ENUM_MAP_TEST_FILES
        "${MAP_TEST_DIR}/min_enum_map_tests_sa*.c"
    )

    # Filter out _main.c files
    set(MIN_ENUM_MAP_FILTERED "")
    foreach(FILE ${MIN_ENUM_MAP_TEST_FILES})
        get_filename_component(FILENAME "${FILE}" NAME)
        if(NOT FILENAME MATCHES "_main\\.c$")
            list(APPEND MIN_ENUM_MAP_FILTERED "${FILE}")
        endif()
    endforeach()

    set(MIN_ENUM_MAP_TARGET "djinterp-c-container-min-enum-map-tests-sa-${PLATFORM_ARCH}")

    add_executable(${MIN_ENUM_MAP_TARGET}
        "${MIN_ENUM_MAP_MAIN}"
        ${MIN_ENUM_MAP_FILTERED}
    )

    target_include_directories(${MIN_ENUM_MAP_TARGET} PRIVATE
        ${INCLUDE_DIR}
        ${TEST_FRAMEWORK_INC_DIR}
        ${CORE_TEST_DIR}
        ${TEST_DIR}
        ${MAP_TEST_DIR}
    )

    target_compile_definitions(${MIN_ENUM_MAP_TARGET} PRIVATE D_TESTING=1)

    if(MSVC)
        target_compile_options(${MIN_ENUM_MAP_TARGET} PRIVATE /FS)
        set_target_properties(${MIN_ENUM_MAP_TARGET} PROPERTIES
            COMPILE_PDB_NAME "${MIN_ENUM_MAP_TARGET}"
            COMPILE_PDB_OUTPUT_DIRECTORY
                "${CMAKE_CURRENT_BINARY_DIR}/${MIN_ENUM_MAP_TARGET}.dir"
        )
    endif()

    target_link_libraries(${MIN_ENUM_MAP_TARGET} PRIVATE
        test-standalone-support
        container
    )

    message(STATUS "  Created test executable: ${MIN_ENUM_MAP_TARGET}")
else()
    message(STATUS "  Skipping min_enum_map tests (no main.c at ${MIN_ENUM_MAP_MAIN})")
endif()
