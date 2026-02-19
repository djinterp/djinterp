# tests_standalone.cmake
# Standalone test framework specific infrastructure
# Provides functions for creating standalone test executables

# include the common test infrastructure
# Use CMAKE_CURRENT_LIST_DIR to get the directory where this file is located
get_filename_component(TESTS_STANDALONE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
include(${TESTS_STANDALONE_DIR}/tests_common.cmake)


###############################################################################
# STANDALONE FRAMEWORK UTILITIES
###############################################################################

# djinterp_add_standalone_framework
#   function: adds the standalone test framework files to a source list
# 
# Parameters:
#   OUTPUT_VAR:            name of variable to store/append the framework files
#   TEST_FRAMEWORK_SRC_DIR: directory where test framework .c files are located
#                           (defaults to ${SOURCE_DIR}/test if not specified)
# 
# Adds:
#   - test_standalone.c
#   - test_common.c
# 
# Example:
#   djinterp_add_standalone_framework(OUTPUT_VAR MY_SOURCES)
#   djinterp_add_standalone_framework(
#       OUTPUT_VAR MY_SOURCES 
#       TEST_FRAMEWORK_SRC_DIR "${SOURCE_DIR}/test"
#   )
function(djinterp_add_standalone_framework)
    # parse arguments
    set(options "")
    set(oneValueArgs OUTPUT_VAR TEST_FRAMEWORK_SRC_DIR)
    set(multiValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    # validate required arguments
    if(NOT DEFINED ARG_OUTPUT_VAR)
        message(FATAL_ERROR "djinterp_add_standalone_framework: OUTPUT_VAR is required")
    endif()
    
    # use custom framework dir if provided, otherwise use SOURCE_DIR/test
    if(DEFINED ARG_TEST_FRAMEWORK_SRC_DIR)
        set(FRAMEWORK_DIR "${ARG_TEST_FRAMEWORK_SRC_DIR}")
    elseif(DEFINED TEST_FRAMEWORK_SRC_DIR)
        set(FRAMEWORK_DIR "${TEST_FRAMEWORK_SRC_DIR}")
    elseif(DEFINED SOURCE_DIR)
        set(FRAMEWORK_DIR "${SOURCE_DIR}/test")
    else()
        message(FATAL_ERROR "djinterp_add_standalone_framework: Could not determine framework directory")
    endif()
    
    # start with existing value if variable is already defined
    if(DEFINED ${ARG_OUTPUT_VAR})
        set(RESULT_LIST ${${ARG_OUTPUT_VAR}})
    else()
        set(RESULT_LIST "")
    endif()
    
    # add framework files
    list(APPEND RESULT_LIST 
        "${FRAMEWORK_DIR}/test_standalone.c"
        "${FRAMEWORK_DIR}/test_common.c"
    )
    
    # return via OUTPUT_VAR
    set(${ARG_OUTPUT_VAR} ${RESULT_LIST} PARENT_SCOPE)
endfunction()


# djinterp_get_standalone_dependencies
#   function: returns all dependencies needed by the standalone test framework
# 
# Parameters:
#   OUTPUT_VAR: name of variable to store the dependency module list
# 
# The standalone test framework requires:
#   - djinterp.h (core)
#   - dmemory.h (for d_memcpy, d_memset)
#   - string_fn.h (for string utilities)
#   - dfile.h (for d_fopen in test_standalone)
# 
# Returns modules in dependency order
function(djinterp_get_standalone_dependencies OUTPUT_VAR)
    # the standalone framework needs:
    # - djinterp (core)
    # - dmemory (memory functions)
    # - string_fn (string functions, which also includes dmemory)
    # - dfile (file operations - d_fopen used in test_standalone)
    
    set(DEPS 
        "djinterp"
        "dmemory"
        "string_fn"
        "dfile"
    )
    
    # return via OUTPUT_VAR
    set(${OUTPUT_VAR} ${DEPS} PARENT_SCOPE)
endfunction()


###############################################################################
# NAME CONVERSION UTILITIES
###############################################################################

# djinterp_module_name_to_path
#   function: converts a module name to a path-friendly format
#   Replaces underscores with hyphens for directory naming consistency.
# 
# Parameters:
#   MODULE_NAME: the module name (e.g., "string_fn")
#   OUTPUT_VAR:  name of variable to store the result (e.g., "string-fn")
# 
# Example:
#   djinterp_module_name_to_path("string_fn" PATH_NAME)
#   # PATH_NAME will be "string-fn"
function(djinterp_module_name_to_path MODULE_NAME OUTPUT_VAR)
    # replace underscores with hyphens
    string(REPLACE "_" "-" RESULT "${MODULE_NAME}")
    set(${OUTPUT_VAR} "${RESULT}" PARENT_SCOPE)
endfunction()


###############################################################################
# PRIMARY TEST CREATION FUNCTION
###############################################################################

# djinterp_add_standalone_test
#   function: simplified function to create a standalone test executable
# 
# This is the PRIMARY function you should use for creating standalone tests.
# It handles all the complexity of gathering files, linking libraries, etc.
#
# Parameters:
#   MODULE_NAME:    name of the module being tested (e.g., "dfile", "dmemory")
#   TEST_PATTERN:   glob pattern for test files (default: "${MODULE_NAME}_tests_sa*.c")
#   EXTRA_LIBS:     additional libraries to link (optional)
#   EXTRA_SOURCES:  additional source files to include (optional)
#   MAIN_FILE:      path to main.c file (optional, will search for it)
# 
# Automatically creates:
#   - Executable named: djinterp-c-${MODULE_NAME}-tests-sa-${PLATFORM_ARCH}
#   - Links all required dependencies
#   - Sets up include directories
#
# Note: Directory paths use hyphens instead of underscores (e.g., string_fn -> string-fn)
#
# Example:
#   djinterp_add_standalone_test(MODULE_NAME dfile)
#   djinterp_add_standalone_test(MODULE_NAME dtime EXTRA_LIBS dtime)
function(djinterp_add_standalone_test)
    # parse arguments
    set(options "")
    set(oneValueArgs MODULE_NAME TEST_PATTERN MAIN_FILE)
    set(multiValueArgs EXTRA_LIBS EXTRA_SOURCES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    # validate required arguments
    if(NOT DEFINED ARG_MODULE_NAME)
        message(FATAL_ERROR "djinterp_add_standalone_test: MODULE_NAME is required")
    endif()
    
    # convert module name to path format (underscores -> hyphens)
    djinterp_module_name_to_path("${ARG_MODULE_NAME}" MODULE_PATH_NAME)
    
    # default test pattern if not specified
    if(NOT DEFINED ARG_TEST_PATTERN)
        set(ARG_TEST_PATTERN "${ARG_MODULE_NAME}_tests_sa*.c")
    endif()
    
    # construct target name (uses hyphenated version)
    set(TARGET_NAME "djinterp-c-${MODULE_PATH_NAME}-tests-sa-${PLATFORM_ARCH}")
    
    # gather test files (excluding *_main.c to avoid duplicate main() definitions)
    file(GLOB TEST_FILES "${TEST_DIR}/${ARG_TEST_PATTERN}")
    
    if(NOT TEST_FILES)
        message(WARNING "No test files found for pattern: ${ARG_TEST_PATTERN}")
        return()
    endif()
    
    # Separate main files from test files to avoid duplicates
    set(MAIN_FILE_IN_GLOB "")
    set(FILTERED_TEST_FILES "")
    foreach(FILE ${TEST_FILES})
        get_filename_component(FILENAME "${FILE}" NAME)
        if(FILENAME MATCHES "_main\\.c$")
            # This is a main file - save it separately
            set(MAIN_FILE_IN_GLOB "${FILE}")
            message(STATUS "  Found main in glob: ${FILENAME}")
        else()
            # Regular test file
            list(APPEND FILTERED_TEST_FILES "${FILE}")
        endif()
    endforeach()
    set(TEST_FILES ${FILTERED_TEST_FILES})
    
    # look for main.c in various locations if not specified
    # Priority: 1) Already found in glob, 2) Explicit ARG_MAIN_FILE, 3) Search paths
    if(MAIN_FILE_IN_GLOB)
        # Use the main file found in the glob pattern
        set(ARG_MAIN_FILE "${MAIN_FILE_IN_GLOB}")
    elseif(NOT DEFINED ARG_MAIN_FILE)
        # Try different locations for main.c in priority order
        set(MAIN_SEARCH_PATHS "")
        
        # 1. First priority: tests directory with standard naming
        list(APPEND MAIN_SEARCH_PATHS "${TEST_DIR}/${ARG_MODULE_NAME}_tests_sa_main.c")
        
        # 2. Second priority: CONFIG_TEST_DIR with hyphenated module name
        #    (e.g., string_fn -> djinterp-c-string-fn-tests-sa)
        #    CONFIG_TEST_DIR should be set by the calling CMakeLists.txt
        #    (e.g., "${CORE_ROOT}/.config/.msvs/testing/c/core")
        if(DEFINED CONFIG_TEST_DIR)
            list(APPEND MAIN_SEARCH_PATHS "${CONFIG_TEST_DIR}/djinterp-c-${MODULE_PATH_NAME}-tests-sa/main.c")
            
            # 3. Third priority: special case for djinterp module (uses "header-tests" name)
            if(ARG_MODULE_NAME STREQUAL "djinterp")
                list(APPEND MAIN_SEARCH_PATHS "${CONFIG_TEST_DIR}/djinterp-c-header-tests-sa/main.c")
            endif()
        endif()
        
        foreach(MAIN_PATH ${MAIN_SEARCH_PATHS})
            if(EXISTS "${MAIN_PATH}")
                set(ARG_MAIN_FILE "${MAIN_PATH}")
                message(STATUS "  Found main.c: ${MAIN_PATH}")
                break()
            endif()
        endforeach()
        
        if(NOT DEFINED ARG_MAIN_FILE)
            message(WARNING "  No main.c found for ${TARGET_NAME}")
            message(WARNING "  Searched locations:")
            foreach(PATH ${MAIN_SEARCH_PATHS})
                message(WARNING "    - ${PATH}")
            endforeach()
            # Continue anyway - maybe there's no main needed
        endif()
    endif()
    
    # create executable with test files and main
    if(DEFINED ARG_MAIN_FILE AND EXISTS "${ARG_MAIN_FILE}")
        add_executable(${TARGET_NAME} ${TEST_FILES} "${ARG_MAIN_FILE}")
    else()
        add_executable(${TARGET_NAME} ${TEST_FILES})
    endif()
    
    # =========================================================================
    # MSVC parallel compilation fixes
    # /FS - serialize PDB access (allows multiple CL.EXE to share PDB)
    # /Fd - unique PDB path per target to avoid conflicts entirely
    # =========================================================================
    if(MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE /FS)
        set_target_properties(${TARGET_NAME} PROPERTIES
            COMPILE_PDB_NAME "${TARGET_NAME}"
            COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.dir"
        )
    endif()
    
    # =========================================================================
    # CRITICAL FIX: Define D_TESTING to disable inline linkage
    # This ensures d_index_convert_fast, d_index_is_valid, etc. have external
    # linkage and are exported from the library
    # =========================================================================
    target_compile_definitions(${TARGET_NAME} PRIVATE D_TESTING=1)
    
    # set include directories
    target_include_directories(${TARGET_NAME} PRIVATE 
        ${INCLUDE_DIR} 
        ${TEST_FRAMEWORK_INC_DIR}
        ${TEST_DIR}
    )
    
    # link the test support library
    target_link_libraries(${TARGET_NAME} PRIVATE test-standalone-support)
    
    # get module dependencies and link them
    # Note: This uses the djinterp_get_module_dependencies macro which can be
    # overridden by module-specific CMakeLists.txt files
	djinterp_get_module_dependencies(${ARG_MODULE_NAME} MODULE_DEPS)
	foreach(DEP ${MODULE_DEPS})
		# Only skip dependencies already in test-standalone-support
		if(NOT DEP STREQUAL "djinterp" AND 
		   NOT DEP STREQUAL "dmemory" AND 
		   NOT DEP STREQUAL "string_fn" AND
		   NOT DEP STREQUAL "dfile" AND
		   NOT DEP STREQUAL "dtime" AND
		   NOT DEP STREQUAL "dstring")  # ADD THIS LINE
			if(TARGET ${DEP})
				target_link_libraries(${TARGET_NAME} PRIVATE ${DEP})
			endif()
		endif()
	endforeach()
    
    # link extra libraries if specified
    if(DEFINED ARG_EXTRA_LIBS)
        foreach(LIB ${ARG_EXTRA_LIBS})
            if(TARGET ${LIB})
                target_link_libraries(${TARGET_NAME} PRIVATE ${LIB})
            endif()
        endforeach()
    endif()
    
    message(STATUS "Created test executable: ${TARGET_NAME}")
endfunction()


###############################################################################
# SUPPORT LIBRARY CREATION
###############################################################################

# djinterp_create_standalone_support_library
#   function: creates the shared test support library
#
# This library contains:
#   - djinterp.c (core)
#   - test_standalone.c and test_common.c (test framework)
#   - dmemory.c, string_fn.c, and dfile.c (dependencies)
#
# Should be called ONCE before creating any test executables.
#
# Parameters:
#   LIBRARY_NAME: name of the support library (default: "test-standalone-support")
#
# Example:
#   djinterp_create_standalone_support_library()
#   djinterp_create_standalone_support_library(LIBRARY_NAME my-test-support)
function(djinterp_create_standalone_support_library)
    # parse arguments
    set(options "")
    set(oneValueArgs LIBRARY_NAME)
    set(multiValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    # default library name
    if(NOT DEFINED ARG_LIBRARY_NAME)
        set(ARG_LIBRARY_NAME "test-standalone-support")
    endif()
    
    # check if already created
    if(TARGET ${ARG_LIBRARY_NAME})
        message(STATUS "Test support library '${ARG_LIBRARY_NAME}' already exists")
        return()
    endif()
    
    # gather all sources for the support library
    set(SUPPORT_SOURCES "")
    
    # add core djinterp
    list(APPEND SUPPORT_SOURCES "${SOURCE_DIR}/djinterp.c")
    
    # add test framework files
    list(APPEND SUPPORT_SOURCES 
        "${TEST_FRAMEWORK_SRC_DIR}/test_standalone.c"
        "${TEST_FRAMEWORK_SRC_DIR}/test_common.c"
    )
    
    # add framework dependencies (including dfile for d_fopen)
    djinterp_get_standalone_dependencies(FRAMEWORK_DEPS)
    foreach(DEP ${FRAMEWORK_DEPS})
        if(NOT DEP STREQUAL "djinterp")  # already added
            if(NOT DEP STREQUAL "dmacro")  # header-only
                list(APPEND SUPPORT_SOURCES "${SOURCE_DIR}/${DEP}.c")
            endif()
        endif()
    endforeach()
    
    # create the library
    add_library(${ARG_LIBRARY_NAME} STATIC ${SUPPORT_SOURCES})
    
    # set include directories
    target_include_directories(${ARG_LIBRARY_NAME} PUBLIC 
        ${INCLUDE_DIR} 
        ${TEST_FRAMEWORK_INC_DIR}
        ${TEST_DIR}
    )
    
    # =========================================================================
    # CRITICAL FIX: Define D_TESTING to disable inline linkage
    # This ensures d_index_convert_fast, d_index_is_valid, etc. have external
    # linkage and are exported from the library
    # =========================================================================
    target_compile_definitions(${ARG_LIBRARY_NAME} PUBLIC D_TESTING=1)
    
    # =========================================================================
    # MSVC parallel compilation fixes
    # /FS - serialize PDB access (allows multiple CL.EXE to share PDB)
    # Unique PDB path to avoid conflicts entirely
    # =========================================================================
    if(MSVC)
        target_compile_options(${ARG_LIBRARY_NAME} PRIVATE /FS)
        set_target_properties(${ARG_LIBRARY_NAME} PROPERTIES
            COMPILE_PDB_NAME "${ARG_LIBRARY_NAME}"
            COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${ARG_LIBRARY_NAME}.dir"
        )
    endif()
    
    message(STATUS "Created test support library: ${ARG_LIBRARY_NAME}")
endfunction()


###############################################################################
# LEGACY/ALTERNATIVE FUNCTIONS
# These are kept for backwards compatibility
###############################################################################

# djinterp_create_standalone_test_executable
#   function: creates a standalone test executable with all required dependencies
# 
# DEPRECATED: Use djinterp_add_standalone_test instead for simpler usage
# 
# Parameters:
#   TARGET:         name of the test executable target
#   TEST_PATTERN:   glob pattern for test files (e.g., "dfile_tests_sa*.c")
#   EXTRA_MODULES:  additional module dependencies beyond framework defaults (optional)
#   LINK_LIBRARIES: additional libraries to link against (optional)
# 
# Example:
#   djinterp_create_standalone_test_executable(
#       TARGET djinterp-c-dfile-tests-sa-${PLATFORM_ARCH}
#       TEST_PATTERN "dfile_tests_sa*.c"
#       LINK_LIBRARIES dfile
#   )
function(djinterp_create_standalone_test_executable)
    # parse arguments
    set(options "")
    set(oneValueArgs TARGET TEST_PATTERN)
    set(multiValueArgs EXTRA_MODULES LINK_LIBRARIES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    # validate required arguments
    if(NOT DEFINED ARG_TARGET)
        message(FATAL_ERROR "djinterp_create_standalone_test_executable: TARGET is required")
    endif()
    if(NOT DEFINED ARG_TEST_PATTERN)
        message(FATAL_ERROR "djinterp_create_standalone_test_executable: TEST_PATTERN is required")
    endif()
    
    # gather test files
    file(GLOB TEST_FILES "${TEST_DIR}/${ARG_TEST_PATTERN}")
    
    # create executable
    add_executable(${ARG_TARGET} ${TEST_FILES})
    
    # set include directories
    target_include_directories(${ARG_TARGET} PRIVATE 
        ${INCLUDE_DIR} 
        ${TEST_FRAMEWORK_INC_DIR}
        ${TEST_DIR}
    )
    
    # =========================================================================
    # CRITICAL FIX: Define D_TESTING to disable inline linkage
    # =========================================================================
    target_compile_definitions(${ARG_TARGET} PRIVATE D_TESTING=1)
    
    # =========================================================================
    # MSVC parallel compilation fixes
    # =========================================================================
    if(MSVC)
        target_compile_options(${ARG_TARGET} PRIVATE /FS)
        set_target_properties(${ARG_TARGET} PROPERTIES
            COMPILE_PDB_NAME "${ARG_TARGET}"
            COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${ARG_TARGET}.dir"
        )
    endif()
    
    # link the support library
    target_link_libraries(${ARG_TARGET} PRIVATE test-standalone-support)
    
    # link extra modules if specified
    if(DEFINED ARG_EXTRA_MODULES)
        foreach(MODULE ${ARG_EXTRA_MODULES})
            if(TARGET ${MODULE})
                target_link_libraries(${ARG_TARGET} PRIVATE ${MODULE})
            endif()
        endforeach()
    endif()
    
    # link additional libraries if specified
    if(DEFINED ARG_LINK_LIBRARIES)
        foreach(LIB ${ARG_LINK_LIBRARIES})
            if(TARGET ${LIB})
                target_link_libraries(${ARG_TARGET} PRIVATE ${LIB})
            endif()
        endforeach()
    endif()
endfunction()