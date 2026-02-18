###############################################################################
# djinterp - core module
# 
# Build configuration for core djinterp modules and tests
#
# Location: <root>/build/cmake/config/c/core/CMakeLists.txt
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
# This CMakeLists.txt is located at: <root>/build/cmake/config/c/core/CMakeLists.txt
# So we need to go up 5 levels to reach the root
get_filename_component(CORE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.." ABSOLUTE)

# Set directory paths (all C sources live under src/c/, inc/c/, tests/c/)
set(SOURCE_DIR              "${CORE_ROOT}/src/c")
set(INCLUDE_DIR             "${CORE_ROOT}/inc/c")
set(TEST_DIR                "${CORE_ROOT}/tests/c")
set(TEST_FRAMEWORK_SRC_DIR  "${CORE_ROOT}/src/c/test")
set(TEST_FRAMEWORK_INC_DIR  "${CORE_ROOT}/inc/c/test")

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CORE_ROOT}/lib/core/c")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CORE_ROOT}/lib/core/c")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CORE_ROOT}/bin/tests/c/core")

# For multi-config generators (Visual Studio)
foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CORE_ROOT}/bin/tests/c/core")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CORE_ROOT}/lib/core/c")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CORE_ROOT}/lib/core/c")
endforeach()

# Detect platform architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(PLATFORM_ARCH "x64")
else()
    set(PLATFORM_ARCH "x86")
endif()

# Include common CMake utilities
# Located 2 levels up from c/core/ -> config/
include(${CMAKE_CURRENT_SOURCE_DIR}/../../common.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../tests_standalone.cmake)

# Set C/C++ standards
set(CMAKE_C_STANDARD 17 CACHE INTERNAL "")
set(CMAKE_C_STANDARD_REQUIRED ON CACHE INTERNAL "")
set(CMAKE_CXX_STANDARD 23 CACHE INTERNAL "")
set(CMAKE_CXX_STANDARD_REQUIRED ON CACHE INTERNAL "")

# Print configuration
message(STATUS "========================================")
message(STATUS "djinterp Core Module Configuration")
message(STATUS "========================================")
message(STATUS "Root:              ${CORE_ROOT}")
message(STATUS "Source dir:        ${SOURCE_DIR}")
message(STATUS "Include dir:       ${INCLUDE_DIR}")
message(STATUS "Test dir:          ${TEST_DIR}")
message(STATUS "Test framework src: ${TEST_FRAMEWORK_SRC_DIR}")
message(STATUS "Test framework inc: ${TEST_FRAMEWORK_INC_DIR}")
message(STATUS "Bin dir:           ${CORE_ROOT}/bin/tests/c/core")
message(STATUS "Lib dir:           ${CORE_ROOT}/lib/core/c")
message(STATUS "Build dir:         ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "Platform:          ${PLATFORM_ARCH}")
message(STATUS "========================================")

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

# datomic module
add_library(datomic STATIC "${SOURCE_DIR}/datomic.c")
target_include_directories(datomic PUBLIC ${INCLUDE_DIR})
target_link_libraries(datomic PUBLIC djinterp)

# dio module
add_library(dio STATIC "${SOURCE_DIR}/dio.c")
target_include_directories(dio PUBLIC ${INCLUDE_DIR})
target_link_libraries(dio PUBLIC djinterp dmemory)

# dmutex module
add_library(dmutex STATIC "${SOURCE_DIR}/dmutex.c")
target_include_directories(dmutex PUBLIC ${INCLUDE_DIR})
target_link_libraries(dmutex PUBLIC djinterp)
# POSIX platforms need pthreads for threading support
if(NOT MSVC)
    find_package(Threads REQUIRED)
    target_link_libraries(dmutex PUBLIC Threads::Threads)
endif()

# dconfig module
add_library(dconfig STATIC "${SOURCE_DIR}/dconfig.c")
target_include_directories(dconfig PUBLIC ${INCLUDE_DIR})
target_link_libraries(dconfig PUBLIC djinterp env)

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

# CRITICAL: Define D_TESTING to disable inline linkage
# This ensures d_index_convert_fast, d_index_is_valid, etc. have external
# linkage and are exported from the library
target_compile_definitions(test-standalone-support PUBLIC D_TESTING=1)

# MSVC parallel compilation fixes
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
set(CONFIG_TEST_DIR "${CORE_ROOT}/.config/.msvs/testing/c/core")

# --------------------------------------------------------------------------
# djinterp core tests (uses special name "header-tests" in .config)
# --------------------------------------------------------------------------
set(DJINTERP_MAIN "${CONFIG_TEST_DIR}/djinterp-c-header-tests-sa/main.c")
if(EXISTS "${DJINTERP_MAIN}")
    djinterp_add_standalone_test(MODULE_NAME djinterp MAIN_FILE "${DJINTERP_MAIN}")
else()
    djinterp_add_standalone_test(MODULE_NAME djinterp)
endif()

# --------------------------------------------------------------------------
# env tests
# --------------------------------------------------------------------------
set(ENV_MAIN "${CONFIG_TEST_DIR}/djinterp-c-env-tests-sa/main.c")
if(EXISTS "${ENV_MAIN}")
    djinterp_add_standalone_test(MODULE_NAME env EXTRA_LIBS env MAIN_FILE "${ENV_MAIN}")
else()
    djinterp_add_standalone_test(MODULE_NAME env EXTRA_LIBS env)
endif()

# --------------------------------------------------------------------------
# dmacro tests
# --------------------------------------------------------------------------
set(DMACRO_MAIN "${CONFIG_TEST_DIR}/djinterp-c-dmacro-tests-sa/main.c")
if(EXISTS "${DMACRO_MAIN}")
    djinterp_add_standalone_test(MODULE_NAME dmacro EXTRA_LIBS dmacro MAIN_FILE "${DMACRO_MAIN}")
else()
    djinterp_add_standalone_test(MODULE_NAME dmacro EXTRA_LIBS dmacro)
endif()

# --------------------------------------------------------------------------
# dfile tests
# --------------------------------------------------------------------------
set(DFILE_MAIN "${CONFIG_TEST_DIR}/djinterp-c-dfile-tests-sa/main.c")
if(EXISTS "${DFILE_MAIN}")
    djinterp_add_standalone_test(MODULE_NAME dfile EXTRA_LIBS dfile MAIN_FILE "${DFILE_MAIN}")
else()
    djinterp_add_standalone_test(MODULE_NAME dfile EXTRA_LIBS dfile)
endif()

# --------------------------------------------------------------------------
# dmemory tests
# --------------------------------------------------------------------------
set(DMEMORY_MAIN "${CONFIG_TEST_DIR}/djinterp-c-dmemory-tests-sa/main.c")
if(EXISTS "${DMEMORY_MAIN}")
    djinterp_add_standalone_test(MODULE_NAME dmemory EXTRA_LIBS dmemory MAIN_FILE "${DMEMORY_MAIN}")
else()
    djinterp_add_standalone_test(MODULE_NAME dmemory EXTRA_LIBS dmemory)
endif()

# --------------------------------------------------------------------------
# dstring tests
# --------------------------------------------------------------------------
set(DSTRING_MAIN "${CONFIG_TEST_DIR}/djinterp-c-dstring-tests-sa/main.c")
if(EXISTS "${DSTRING_MAIN}")
    djinterp_add_standalone_test(MODULE_NAME dstring EXTRA_LIBS dstring MAIN_FILE "${DSTRING_MAIN}")
else()
    djinterp_add_standalone_test(MODULE_NAME dstring EXTRA_LIBS dstring)
endif()

# --------------------------------------------------------------------------
# dtime tests
# --------------------------------------------------------------------------
set(DTIME_MAIN "${CONFIG_TEST_DIR}/djinterp-c-dtime-tests-sa/main.c")
if(EXISTS "${DTIME_MAIN}")
    djinterp_add_standalone_test(MODULE_NAME dtime EXTRA_LIBS dtime MAIN_FILE "${DTIME_MAIN}")
else()
    djinterp_add_standalone_test(MODULE_NAME dtime EXTRA_LIBS dtime)
endif()

# --------------------------------------------------------------------------
# string_fn tests
# FIX: directory uses hyphens ("string-fn"), not underscores ("string_fn")
# --------------------------------------------------------------------------
set(STRING_FN_MAIN "${CONFIG_TEST_DIR}/djinterp-c-string-fn-tests-sa/main.c")
if(EXISTS "${STRING_FN_MAIN}")
    djinterp_add_standalone_test(MODULE_NAME string_fn EXTRA_LIBS string_fn MAIN_FILE "${STRING_FN_MAIN}")
else()
    djinterp_add_standalone_test(MODULE_NAME string_fn EXTRA_LIBS string_fn)
endif()

# --------------------------------------------------------------------------
# datomic tests
# --------------------------------------------------------------------------
set(DATOMIC_MAIN "${CONFIG_TEST_DIR}/djinterp-c-datomic-tests-sa/main.c")
if(EXISTS "${DATOMIC_MAIN}")
    djinterp_add_standalone_test(MODULE_NAME datomic EXTRA_LIBS datomic MAIN_FILE "${DATOMIC_MAIN}")
else()
    djinterp_add_standalone_test(MODULE_NAME datomic EXTRA_LIBS datomic)
endif()

# --------------------------------------------------------------------------
# dio tests
# --------------------------------------------------------------------------
set(DIO_MAIN "${CONFIG_TEST_DIR}/djinterp-c-dio-tests-sa/main.c")
if(EXISTS "${DIO_MAIN}")
    djinterp_add_standalone_test(MODULE_NAME dio EXTRA_LIBS dio MAIN_FILE "${DIO_MAIN}")
else()
    djinterp_add_standalone_test(MODULE_NAME dio EXTRA_LIBS dio)
endif()

# --------------------------------------------------------------------------
# dmutex tests
# --------------------------------------------------------------------------
set(DMUTEX_MAIN "${CONFIG_TEST_DIR}/djinterp-c-dmutex-tests-sa/main.c")
if(EXISTS "${DMUTEX_MAIN}")
    djinterp_add_standalone_test(MODULE_NAME dmutex EXTRA_LIBS dmutex MAIN_FILE "${DMUTEX_MAIN}")
else()
    djinterp_add_standalone_test(MODULE_NAME dmutex EXTRA_LIBS dmutex)
endif()

###############################################################################
# SUMMARY
###############################################################################

message(STATUS "")
message(STATUS "Build Summary:")
message(STATUS "  Libraries:        djinterp, env, dmacro, dfile, dmemory, dstring, dtime, string_fn,")
message(STATUS "                    datomic, dio, dmutex, dconfig")
message(STATUS "  Test executables: 11")
message(STATUS "  Test framework:   Standalone (library-based)")
message(STATUS "")