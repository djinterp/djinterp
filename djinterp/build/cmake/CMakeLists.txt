###############################################################################
# djinterp - core module
# 
# Build configuration for core djinterp modules and tests
#
# Location: <root>/build/cmake/config/core/CMakeLists.txt
# 
# USAGE:
#   To generate MSVS project files in the correct location, run from the
#   msvs/core directory or use the -B flag:
#
#   Option 1 (from msvs/core):
#     cd <root>/build/cmake/msvs/core
#     cmake ../../config/core
#
#   Option 2 (from anywhere):
#     cmake -S <root>/build/cmake/config/core -B <root>/build/cmake/msvs/core
#
#   Option 3 (from config/core with -B):
#     cmake -B ../../msvs/core
#
# author(s): Samuel 'teer' Neal-Blim                          date: 2023.03.01
###############################################################################
cmake_minimum_required(VERSION 3.20)

# Project name
project(djinterp-core VERSION 0.1 LANGUAGES C CXX)

###############################################################################
# DIRECTORY SETUP
###############################################################################

# Determine the core root directory
# CMAKE_CURRENT_SOURCE_DIR is where CMakeLists.txt is located:
#   <root>/build/cmake/config/core/CMakeLists.txt
# So we need to go up 4 levels to reach the root
get_filename_component(CORE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)

# Set directory paths
set(SOURCE_DIR              "${CORE_ROOT}/src")
set(INCLUDE_DIR             "${CORE_ROOT}/inc")
set(TEST_DIR                "${CORE_ROOT}/tests")
set(TEST_FRAMEWORK_SRC_DIR  "${CORE_ROOT}/src/test")
set(TEST_FRAMEWORK_INC_DIR  "${CORE_ROOT}/inc/test")

# Set output directories for compiled artifacts
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CORE_ROOT}/lib/core")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CORE_ROOT}/lib/core")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CORE_ROOT}/bin/tests/core")

# For multi-config generators (Visual Studio)
foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CORE_ROOT}/bin/tests/core")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CORE_ROOT}/lib/core")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CORE_ROOT}/lib/core")
endforeach()

# Detect platform architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(PLATFORM_ARCH "x64")
else()
    set(PLATFORM_ARCH "x86")
endif()

# Check if building in source directory - recommend using msvs/core
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(WARNING "")
    message(WARNING "Building in source directory!")
    message(WARNING "Recommended: cmake -B ../../msvs/core")
    message(WARNING "This will generate MSVS files in build/cmake/msvs/core/")
    message(WARNING "")
endif()

# Verify build directory is in the expected location (msvs/core)
get_filename_component(BUILD_DIR_NAME "${CMAKE_BINARY_DIR}" NAME)
get_filename_component(BUILD_DIR_PARENT "${CMAKE_BINARY_DIR}" DIRECTORY)
get_filename_component(BUILD_DIR_PARENT_NAME "${BUILD_DIR_PARENT}" NAME)
if(NOT (BUILD_DIR_NAME STREQUAL "core" AND BUILD_DIR_PARENT_NAME STREQUAL "msvs"))
    message(STATUS "Note: Build directory is '${CMAKE_BINARY_DIR}'")
    message(STATUS "      Expected: <root>/build/cmake/msvs/core")
endif()

# Include common CMake utilities (relative to CMakeLists.txt location)
include(${CMAKE_CURRENT_SOURCE_DIR}/../common.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../tests_standalone.cmake)

# Set C/C++ standards
set(CMAKE_C_STANDARD 17 CACHE INTERNAL "")
set(CMAKE_C_STANDARD_REQUIRED ON CACHE INTERNAL "")
set(CMAKE_CXX_STANDARD 23 CACHE INTERNAL "")
set(CMAKE_CXX_STANDARD_REQUIRED ON CACHE INTERNAL "")

# Print configuration
message(STATUS "========================================")
message(STATUS "djinterp Core Module Configuration")
message(STATUS "========================================")
message(STATUS "Root:               ${CORE_ROOT}")
message(STATUS "Source dir:         ${SOURCE_DIR}")
message(STATUS "Include dir:        ${INCLUDE_DIR}")
message(STATUS "Test dir:           ${TEST_DIR}")
message(STATUS "Test framework src: ${TEST_FRAMEWORK_SRC_DIR}")
message(STATUS "Test framework inc: ${TEST_FRAMEWORK_INC_DIR}")
message(STATUS "Bin dir:            ${CORE_ROOT}/bin/tests/core")
message(STATUS "Lib dir:            ${CORE_ROOT}/lib/core")
message(STATUS "Build dir:          ${CMAKE_BINARY_DIR}")
message(STATUS "Platform:           ${PLATFORM_ARCH}")
message(STATUS "========================================")

###############################################################################
# CORE MODULE DEPENDENCIES
# 
# This function defines the dependency graph for all core modules.
# It overrides/extends the generic version from tests_common.cmake.
###############################################################################

function(djinterp_get_core_module_dependencies MODULE OUTPUT_VAR)
    set(DEPS "")
    
    # Core module dependency definitions
    if(MODULE STREQUAL "djinterp")
        # djinterp is the core, no dependencies
        set(DEPS "")
        
    elseif(MODULE STREQUAL "env")
        # env depends on djinterp
        set(DEPS "djinterp")
        
    elseif(MODULE STREQUAL "dmacro")
        # dmacro is header-only, depends on djinterp
        set(DEPS "djinterp")
        
    elseif(MODULE STREQUAL "dmemory")
        # dmemory depends on djinterp
        set(DEPS "djinterp")
        
    elseif(MODULE STREQUAL "string_fn")
        # string_fn depends on dmemory (which depends on djinterp)
        set(DEPS "djinterp" "dmemory")
        
    elseif(MODULE STREQUAL "dfile")
        # dfile depends on djinterp, dmemory, and string_fn
        set(DEPS "djinterp" "dmemory" "string_fn")
        
    elseif(MODULE STREQUAL "dstring")
        # dstring depends on djinterp and dmemory
        set(DEPS "djinterp" "dmemory")
        
    elseif(MODULE STREQUAL "dtime")
        # dtime depends on djinterp and dmemory
        set(DEPS "djinterp" "dmemory")
        
    else()
        message(WARNING "Unknown core module: ${MODULE}, assuming depends on djinterp only")
        set(DEPS "djinterp")
    endif()
    
    # Add the module itself at the end
    list(APPEND DEPS ${MODULE})
    
    # Return via OUTPUT_VAR
    set(${OUTPUT_VAR} ${DEPS} PARENT_SCOPE)
endfunction()

# Override the generic function to use core-specific dependencies
macro(djinterp_get_module_dependencies MODULE OUTPUT_VAR)
    djinterp_get_core_module_dependencies(${MODULE} ${OUTPUT_VAR})
endmacro()

###############################################################################
# CORE LIBRARIES
###############################################################################

# djinterp module (core)
add_library(djinterp STATIC "${SOURCE_DIR}/djinterp.c")
target_include_directories(djinterp PUBLIC ${INCLUDE_DIR})

# env module
add_library(env STATIC "${SOURCE_DIR}/env.c")
target_include_directories(env PUBLIC ${INCLUDE_DIR})
target_link_libraries(env PUBLIC djinterp)

# dmacro module (header-only)
add_library(dmacro INTERFACE)
target_include_directories(dmacro INTERFACE ${INCLUDE_DIR})

# dmemory module
add_library(dmemory STATIC "${SOURCE_DIR}/dmemory.c")
target_include_directories(dmemory PUBLIC ${INCLUDE_DIR})
target_link_libraries(dmemory PUBLIC djinterp)

# string_fn module
add_library(string_fn STATIC "${SOURCE_DIR}/string_fn.c")
target_include_directories(string_fn PUBLIC ${INCLUDE_DIR})
target_link_libraries(string_fn PUBLIC dmemory djinterp)

# dfile module
add_library(dfile STATIC "${SOURCE_DIR}/dfile.c")
target_include_directories(dfile PUBLIC ${INCLUDE_DIR})
target_link_libraries(dfile PUBLIC djinterp dmemory string_fn)

# dstring module
add_library(dstring STATIC "${SOURCE_DIR}/dstring.c")
target_include_directories(dstring PUBLIC ${INCLUDE_DIR})
target_link_libraries(dstring PUBLIC djinterp dmemory)

# dtime module
add_library(dtime STATIC "${SOURCE_DIR}/dtime.c")
target_include_directories(dtime PUBLIC ${INCLUDE_DIR})
target_link_libraries(dtime PUBLIC djinterp dmemory)

###############################################################################
# COMPILER FLAGS
###############################################################################

# Add /FS flag for MSVC to avoid parallel compilation PDB issues
if(MSVC)
    add_compile_options(/FS)
endif()

###############################################################################
# TEST SUPPORT LIBRARY
###############################################################################

# Create the shared test support library
# This is compiled once and linked to all test executables for efficiency
# Includes dfile since test_standalone uses d_fopen
add_library(test-standalone-support STATIC
    "${SOURCE_DIR}/djinterp.c"
    "${TEST_FRAMEWORK_SRC_DIR}/test_standalone.c"
    "${TEST_FRAMEWORK_SRC_DIR}/test_common.c"
    "${SOURCE_DIR}/dmemory.c"
    "${SOURCE_DIR}/string_fn.c"
    "${SOURCE_DIR}/dfile.c"
)
target_include_directories(test-standalone-support PUBLIC 
    ${INCLUDE_DIR} 
    ${TEST_FRAMEWORK_INC_DIR}
    ${TEST_DIR}
)

# Define D_TESTING for the support library
target_compile_definitions(test-standalone-support PUBLIC D_TESTING=1)

# MSVC-specific settings for support library
if(MSVC)
    target_compile_options(test-standalone-support PRIVATE /FS)
    set_target_properties(test-standalone-support PROPERTIES
        COMPILE_PDB_NAME "test-standalone-support"
        COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test-standalone-support.dir"
    )
endif()

###############################################################################
# TEST EXECUTABLES
###############################################################################

# Define .config directory for main.c files
# Note: Directory names use hyphens (e.g., string-fn not string_fn)
set(CONFIG_TEST_DIR "${CORE_ROOT}/.config/.msvs/testing/core")

# Track all test targets for the combined "all" target
set(ALL_CORE_TEST_TARGETS "")

# -----------------------------------------------------------------------------
# djinterp core tests (uses special name "header-tests" in .config)
# -----------------------------------------------------------------------------
set(DJINTERP_MAIN "${CONFIG_TEST_DIR}/djinterp-c-header-tests-sa/main.c")
if(EXISTS "${DJINTERP_MAIN}")
    djinterp_add_standalone_test(MODULE_NAME djinterp MAIN_FILE "${DJINTERP_MAIN}")
else()
    djinterp_add_standalone_test(MODULE_NAME djinterp)
endif()
list(APPEND ALL_CORE_TEST_TARGETS "djinterp-c-djinterp-tests-sa-${PLATFORM_ARCH}")

# -----------------------------------------------------------------------------
# env tests
# -----------------------------------------------------------------------------
set(ENV_MAIN "${CONFIG_TEST_DIR}/djinterp-c-env-tests-sa/main.c")
if(EXISTS "${ENV_MAIN}")
    djinterp_add_standalone_test(MODULE_NAME env EXTRA_LIBS env MAIN_FILE "${ENV_MAIN}")
else()
    djinterp_add_standalone_test(MODULE_NAME env EXTRA_LIBS env)
endif()
list(APPEND ALL_CORE_TEST_TARGETS "djinterp-c-env-tests-sa-${PLATFORM_ARCH}")

# -----------------------------------------------------------------------------
# dmacro tests  
# -----------------------------------------------------------------------------
set(DMACRO_MAIN "${CONFIG_TEST_DIR}/djinterp-c-dmacro-tests-sa/main.c")
if(EXISTS "${DMACRO_MAIN}")
    djinterp_add_standalone_test(MODULE_NAME dmacro EXTRA_LIBS dmacro MAIN_FILE "${DMACRO_MAIN}")
else()
    djinterp_add_standalone_test(MODULE_NAME dmacro EXTRA_LIBS dmacro)
endif()
list(APPEND ALL_CORE_TEST_TARGETS "djinterp-c-dmacro-tests-sa-${PLATFORM_ARCH}")

# -----------------------------------------------------------------------------
# dfile tests
# -----------------------------------------------------------------------------
set(DFILE_MAIN "${CONFIG_TEST_DIR}/djinterp-c-dfile-tests-sa/main.c")
if(EXISTS "${DFILE_MAIN}")
    djinterp_add_standalone_test(MODULE_NAME dfile EXTRA_LIBS dfile MAIN_FILE "${DFILE_MAIN}")
else()
    djinterp_add_standalone_test(MODULE_NAME dfile EXTRA_LIBS dfile)
endif()
list(APPEND ALL_CORE_TEST_TARGETS "djinterp-c-dfile-tests-sa-${PLATFORM_ARCH}")

# -----------------------------------------------------------------------------
# dmemory tests
# -----------------------------------------------------------------------------
set(DMEMORY_MAIN "${CONFIG_TEST_DIR}/djinterp-c-dmemory-tests-sa/main.c")
if(EXISTS "${DMEMORY_MAIN}")
    djinterp_add_standalone_test(MODULE_NAME dmemory EXTRA_LIBS dmemory MAIN_FILE "${DMEMORY_MAIN}")
else()
    djinterp_add_standalone_test(MODULE_NAME dmemory EXTRA_LIBS dmemory)
endif()
list(APPEND ALL_CORE_TEST_TARGETS "djinterp-c-dmemory-tests-sa-${PLATFORM_ARCH}")

# -----------------------------------------------------------------------------
# dstring tests
# -----------------------------------------------------------------------------
set(DSTRING_MAIN "${CONFIG_TEST_DIR}/djinterp-c-dstring-tests-sa/main.c")
if(EXISTS "${DSTRING_MAIN}")
    djinterp_add_standalone_test(MODULE_NAME dstring EXTRA_LIBS dstring MAIN_FILE "${DSTRING_MAIN}")
else()
    djinterp_add_standalone_test(MODULE_NAME dstring EXTRA_LIBS dstring)
endif()
list(APPEND ALL_CORE_TEST_TARGETS "djinterp-c-dstring-tests-sa-${PLATFORM_ARCH}")

# -----------------------------------------------------------------------------
# dtime tests
# -----------------------------------------------------------------------------
set(DTIME_MAIN "${CONFIG_TEST_DIR}/djinterp-c-dtime-tests-sa/main.c")
if(EXISTS "${DTIME_MAIN}")
    djinterp_add_standalone_test(MODULE_NAME dtime EXTRA_LIBS dtime MAIN_FILE "${DTIME_MAIN}")
else()
    djinterp_add_standalone_test(MODULE_NAME dtime EXTRA_LIBS dtime)
endif()
list(APPEND ALL_CORE_TEST_TARGETS "djinterp-c-dtime-tests-sa-${PLATFORM_ARCH}")

# -----------------------------------------------------------------------------
# string_fn tests
# Note: Directory uses hyphen (string-fn) but module name uses underscore
# -----------------------------------------------------------------------------
set(STRING_FN_MAIN "${CONFIG_TEST_DIR}/djinterp-c-string-fn-tests-sa/main.c")
if(EXISTS "${STRING_FN_MAIN}")
    djinterp_add_standalone_test(MODULE_NAME string_fn EXTRA_LIBS string_fn MAIN_FILE "${STRING_FN_MAIN}")
else()
    djinterp_add_standalone_test(MODULE_NAME string_fn EXTRA_LIBS string_fn)
endif()
list(APPEND ALL_CORE_TEST_TARGETS "djinterp-c-string-fn-tests-sa-${PLATFORM_ARCH}")

###############################################################################
# COMBINED TEST TARGET: djinterp-core-tests-all
#
# This target builds all core test executables.
# Building this target will compile all tests; it doesn't run them.
# To run all tests, use ctest after building.
###############################################################################

# Create a custom target that depends on all individual test executables
add_custom_target(djinterp-core-tests-all
    COMMENT "Building all core test executables"
)

# Add dependencies on all test targets (only if they exist)
foreach(TEST_TARGET ${ALL_CORE_TEST_TARGETS})
    if(TARGET ${TEST_TARGET})
        add_dependencies(djinterp-core-tests-all ${TEST_TARGET})
    endif()
endforeach()

###############################################################################
# CTEST INTEGRATION
# 
# Register all tests with CTest for automated test running
###############################################################################

enable_testing()

# Register each test executable with CTest
foreach(TEST_TARGET ${ALL_CORE_TEST_TARGETS})
    if(TARGET ${TEST_TARGET})
        add_test(
            NAME ${TEST_TARGET}
            COMMAND ${TEST_TARGET}
            WORKING_DIRECTORY "${CORE_ROOT}/bin/tests/core"
        )
    endif()
endforeach()

# Custom target to build and run all tests
add_custom_target(djinterp-core-run-all-tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS djinterp-core-tests-all
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all core tests"
)

###############################################################################
# SUMMARY
###############################################################################

message(STATUS "")
message(STATUS "Build Summary:")
message(STATUS "  Libraries:        djinterp, env, dmacro, dfile, dmemory, dstring, dtime, string_fn")
message(STATUS "  Test executables: 8")
message(STATUS "  Test framework:   Standalone (library-based)")
message(STATUS "  D_TESTING:        Enabled (inline functions have external linkage)")
message(STATUS "")
message(STATUS "Build Targets:")
message(STATUS "  djinterp-core-tests-all       - Build all test executables")
message(STATUS "  djinterp-core-run-all-tests   - Build and run all tests")
message(STATUS "")
message(STATUS "To run tests after building:")
message(STATUS "  cd ${CMAKE_BINARY_DIR} && ctest --output-on-failure")
message(STATUS "")