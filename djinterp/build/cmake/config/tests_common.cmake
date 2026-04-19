# tests_common.cmake
# Common test infrastructure functions for all test types
# Provides functions for gathering test files and their dependencies

###############################################################################
# FILE GATHERING UTILITIES
###############################################################################

# djinterp_gather_test_files
#   function: gathers test files matching a pattern and prepends core dependencies
# 
# Parameters:
#   OUTPUT_VAR:     name of variable to store the resulting file list
#   PATTERN:        glob pattern for test files (e.g., "dfile_tests_sa*.c")
#   PREPEND_CORE:   whether to prepend djinterp.c (ON by default)
#   TEST_DIR:       directory containing test files (optional, uses TEST_DIR if not specified)
#   SOURCE_DIR:     directory containing source files (optional, uses SOURCE_DIR if not specified)
# 
# Example:
#   djinterp_gather_test_files(
#       OUTPUT_VAR MY_TEST_FILES
#       PATTERN "dfile_tests_sa*.c"
#       PREPEND_CORE ON
#   )
function(djinterp_gather_test_files)
    # parse arguments
    set(options "")
    set(oneValueArgs OUTPUT_VAR PATTERN PREPEND_CORE TEST_DIR SOURCE_DIR)
    set(multiValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    # validate required arguments
    if(NOT DEFINED ARG_OUTPUT_VAR)
        message(FATAL_ERROR "djinterp_gather_test_files: OUTPUT_VAR is required")
    endif()
    if(NOT DEFINED ARG_PATTERN)
        message(FATAL_ERROR "djinterp_gather_test_files: PATTERN is required")
    endif()
    
    # use provided directories or fall back to parent scope variables
    if(DEFINED ARG_TEST_DIR)
        set(LOCAL_TEST_DIR "${ARG_TEST_DIR}")
    else()
        set(LOCAL_TEST_DIR "${TEST_DIR}")
    endif()
    
    if(DEFINED ARG_SOURCE_DIR)
        set(LOCAL_SOURCE_DIR "${ARG_SOURCE_DIR}")
    else()
        set(LOCAL_SOURCE_DIR "${SOURCE_DIR}")
    endif()
    
    # default PREPEND_CORE to ON if not specified
    if(NOT DEFINED ARG_PREPEND_CORE)
        set(ARG_PREPEND_CORE ON)
    endif()
    
    # gather test files matching pattern
    file(GLOB TEST_FILES "${LOCAL_TEST_DIR}/${ARG_PATTERN}")
    
    # build result list
    set(RESULT_LIST "")
    
    # prepend djinterp.c if requested
    if(ARG_PREPEND_CORE)
        list(APPEND RESULT_LIST "${LOCAL_SOURCE_DIR}/djinterp.c")
    endif()
    
    # add the gathered test files
    list(APPEND RESULT_LIST ${TEST_FILES})
    
    # return via OUTPUT_VAR
    set(${ARG_OUTPUT_VAR} ${RESULT_LIST} PARENT_SCOPE)
endfunction()


# djinterp_add_module_dependencies
#   function: adds module source files to a list for specific modules
# 
# Parameters:
#   OUTPUT_VAR:  name of variable to store/append the resulting file list
#   MODULES:     list of module names (e.g., dfile dmemory string_fn)
#   SOURCE_DIR:  directory containing source files (optional, uses SOURCE_DIR if not specified)
# 
# Example:
#   djinterp_add_module_dependencies(
#       OUTPUT_VAR MY_SOURCES
#       MODULES dfile dmemory string_fn
#   )
function(djinterp_add_module_dependencies)
    # parse arguments
    set(options "")
    set(oneValueArgs OUTPUT_VAR SOURCE_DIR)
    set(multiValueArgs MODULES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    # validate required arguments
    if(NOT DEFINED ARG_OUTPUT_VAR)
        message(FATAL_ERROR "djinterp_add_module_dependencies: OUTPUT_VAR is required")
    endif()
    if(NOT DEFINED ARG_MODULES)
        message(FATAL_ERROR "djinterp_add_module_dependencies: MODULES is required")
    endif()
    
    # use provided source dir or fall back to parent scope variable
    if(DEFINED ARG_SOURCE_DIR)
        set(LOCAL_SOURCE_DIR "${ARG_SOURCE_DIR}")
    else()
        set(LOCAL_SOURCE_DIR "${SOURCE_DIR}")
    endif()
    
    # start with existing value if variable is already defined
    if(DEFINED ${ARG_OUTPUT_VAR})
        set(RESULT_LIST ${${ARG_OUTPUT_VAR}})
    else()
        set(RESULT_LIST "")
    endif()
    
    # add each module's source file
    foreach(MODULE ${ARG_MODULES})
        # skip dmacro as it's header-only
        if(NOT MODULE STREQUAL "dmacro")
            list(APPEND RESULT_LIST "${LOCAL_SOURCE_DIR}/${MODULE}.c")
        endif()
    endforeach()
    
    # return via OUTPUT_VAR
    set(${ARG_OUTPUT_VAR} ${RESULT_LIST} PARENT_SCOPE)
endfunction()


###############################################################################
# MODULE DEPENDENCY RESOLUTION
#
# This provides a DEFAULT implementation that can be overridden by module-
# specific CMakeLists.txt files. Modules can define their own dependency
# function and use the macro mechanism to override this one.
#
# To override in your module's CMakeLists.txt:
#
#   function(my_module_get_dependencies MODULE OUTPUT_VAR)
#       # Define your dependencies here
#       set(${OUTPUT_VAR} ${DEPS} PARENT_SCOPE)
#   endfunction()
#
#   macro(djinterp_get_module_dependencies MODULE OUTPUT_VAR)
#       my_module_get_dependencies(${MODULE} ${OUTPUT_VAR})
#   endmacro()
#
###############################################################################

# djinterp_get_module_dependencies_default
#   function: DEFAULT implementation - returns the dependency chain for a module
# 
# Parameters:
#   MODULE:     module name (e.g., dfile)
#   OUTPUT_VAR: name of variable to store the dependency list
# 
# The function returns modules in dependency order (dependencies first)
#
# NOTE: This is the fallback implementation. Module-specific CMakeLists.txt
# files should override djinterp_get_module_dependencies for their modules.
function(djinterp_get_module_dependencies_default MODULE OUTPUT_VAR)
    set(DEPS "")
    
    # Default dependency chains for known modules
    # These can be overridden by module-specific implementations
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
        
    elseif(MODULE STREQUAL "dio")
        # dio depends on djinterp
        set(DEPS "djinterp")

    elseif(MODULE STREQUAL "dstring")
        # dstring depends on djinterp and dmemory
        set(DEPS "djinterp" "dmemory")
        
    elseif(MODULE STREQUAL "dtime")
        # dtime depends on djinterp and dmemory
        set(DEPS "djinterp" "dmemory")

    elseif(MODULE STREQUAL "datomic")
        # datomic depends on djinterp
        set(DEPS "djinterp")

    elseif(MODULE STREQUAL "dmutex")
        # dmutex depends on djinterp
        set(DEPS "djinterp")

    elseif(MODULE STREQUAL "dconfig")
        # dconfig depends on djinterp, dmemory, string_fn
        set(DEPS "djinterp" "dmemory" "string_fn")

    # --- container modules ---
    elseif(MODULE STREQUAL "container" OR
           MODULE STREQUAL "array_common" OR MODULE STREQUAL "array_filter" OR
           MODULE STREQUAL "circular_array" OR MODULE STREQUAL "ptr_array" OR
           MODULE STREQUAL "enum_map_entry" OR MODULE STREQUAL "min_enum_map" OR
           MODULE STREQUAL "dictionary" OR MODULE STREQUAL "enum_map" OR
           MODULE STREQUAL "registry_common" OR MODULE STREQUAL "registry" OR
           MODULE STREQUAL "vector_common" OR MODULE STREQUAL "vector" OR
           MODULE STREQUAL "ptr_vector")
        set(DEPS "djinterp" "dmemory" "string_fn")

    # --- functional modules ---
    elseif(MODULE STREQUAL "functional" OR MODULE STREQUAL "functional_common")
        set(DEPS "djinterp" "dmemory")

    elseif(MODULE STREQUAL "compose")
        set(DEPS "djinterp" "dmemory" "functional" "functional_common")

    elseif(MODULE STREQUAL "filter")
        set(DEPS "djinterp" "dmemory" "dio" "functional" "functional_common")

    elseif(MODULE STREQUAL "fn_builder")
        set(DEPS "djinterp" "dmemory" "functional" "functional_common")

    elseif(MODULE STREQUAL "pipeline")
        set(DEPS "djinterp" "dmemory" "functional" "functional_common" "compose")

    elseif(MODULE STREQUAL "predicate")
        set(DEPS "djinterp" "dmemory" "functional" "functional_common")

    # --- event modules ---
    elseif(MODULE STREQUAL "event" OR MODULE STREQUAL "event_table_common")
        set(DEPS "djinterp" "dmemory" "string_fn")

    elseif(MODULE STREQUAL "event_table")
        set(DEPS "djinterp" "dmemory" "event_table_common" "event")

    elseif(MODULE STREQUAL "event_handler")
        set(DEPS "djinterp" "dmemory" "event_table" "event")

    # --- test framework modules ---
    elseif(MODULE STREQUAL "test_common" OR MODULE STREQUAL "test_standalone" OR
           MODULE STREQUAL "test_config" OR MODULE STREQUAL "test_cvar" OR
           MODULE STREQUAL "test_module" OR MODULE STREQUAL "test_assert" OR
           MODULE STREQUAL "assert_tests_sa")
        set(DEPS "djinterp" "dmemory" "string_fn")

    else()
        message(WARNING "Unknown module: ${MODULE}, assuming depends on djinterp only")
        set(DEPS "djinterp")
    endif()
    
    # add the module itself at the end
    list(APPEND DEPS ${MODULE})
    
    # return via OUTPUT_VAR
    set(${OUTPUT_VAR} ${DEPS} PARENT_SCOPE)
endfunction()

# Default macro that calls the default function
# This can be overridden by module-specific CMakeLists.txt files
# by defining their own macro with the same name AFTER including this file
macro(djinterp_get_module_dependencies MODULE OUTPUT_VAR)
    djinterp_get_module_dependencies_default(${MODULE} ${OUTPUT_VAR})
endmacro()


###############################################################################
# HEADER-ONLY MODULE DETECTION
###############################################################################

# djinterp_is_header_only_module
#   function: checks if a module is header-only
#
# Parameters:
#   MODULE:     module name (e.g., dmacro)
#   OUTPUT_VAR: name of variable to store the result (TRUE/FALSE)
function(djinterp_is_header_only_module MODULE OUTPUT_VAR)
    # List of known header-only modules
    set(HEADER_ONLY_MODULES "dmacro")
    
    if(MODULE IN_LIST HEADER_ONLY_MODULES)
        set(${OUTPUT_VAR} TRUE PARENT_SCOPE)
    else()
        set(${OUTPUT_VAR} FALSE PARENT_SCOPE)
    endif()
endfunction()
