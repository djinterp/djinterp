###############################################################################
# common.cmake
# 
# This module contains useful CMake functions that can be used in other 
# modules.
#
# author(s): Samuel 'teer' Neal-Blim                          date: 2023.03.01
###############################################################################
cmake_minimum_required(VERSION 3.20)


# find_parent_directory_named
#   function: given a String `key`, parses the current list directory path and
# returns a string corresponding to the path 
function(find_parent_directory_named key result)
    # set default value for find_last (default is FALSE)
    if (NOT DEFINED find_last)
        set(find_last FALSE)
    endif()

    # start with the path of the calling file
    set(current_dir "${CMAKE_CURRENT_LIST_DIR}")

    # Convert the current directory to a list of components
    string(FIND "${current_dir}" "/${key}/" key_pos)
    
    if (key_pos EQUAL -1)
        # key not found; return empty string
        set(${result} "" PARENT_SCOPE)
    else()
        # key found; calculate the length of the key
        string(LENGTH "${key}" key_length)
        # truncate subsequent string and return value
        math(EXPR key_end "${key_pos} + ${key_length} + 1")
        string(SUBSTRING "${current_dir}" 0 ${key_end} found_dir)
        set(${result} "${found_dir}" PARENT_SCOPE)
    endif()
endfunction()


# set_module_local_directories
#   function: sets the local directory paths for a given module.
function(set_module_local_directories root_dir project_path)
    # extract relative path
    file(RELATIVE_PATH rel_path ${root_dir}/build/cmake ${project_path})

    # set the modules relative path so it can be used in the module
    set(MODULE_REL_PATH     "${rel_path}"                   PARENT_SCOPE)
    
    # set all of the directories, based on the root path
    set(MODULE_BINARY_DIR   "${root_dir}/bin/${rel_path}"  PARENT_SCOPE)
    set(MODULE_BUILD_DIR    "${root_dir}/build/${rel_path}" PARENT_SCOPE)
    set(MODULE_INCLUDE_DIR  "${root_dir}/inc/${rel_path}"  PARENT_SCOPE)
    set(MODULE_RESOURCE_DIR "${root_dir}/res/${rel_path}"  PARENT_SCOPE)
    set(MODULE_SOURCE_DIR   "${root_dir}/src/${rel_path}"  PARENT_SCOPE)
    set(MODULE_TEST_DIR     "${root_dir}/test/${rel_path}" PARENT_SCOPE)
endfunction()


# set_module_dependencies
#   function: adds dependencies to a project based on its relative path.
# E.g. `/foo/bar/some_module` presumably inherits from `bar`, which itself is
# derived from `foo`. So this would add `foo` --> `bar` --> `some_module`
function(set_module_dependencies project_path root_dir)
    # Handle dependencies
    set(dependencies "")
    file(RELATIVE_PATH rel_path ${root_dir}/build/cmake ${project_path})
    string(REPLACE "/" ";" path_parts "${rel_path}")
    set(current_path "")
    
    foreach(part ${path_parts})
        set(current_path "${current_path}/${part}")
        if(EXISTS "${root_dir}/inc${current_path}/${part}.hpp")
            list(APPEND dependencies "${root_dir}/inc${current_path}/${part}.hpp")
        endif()
    endforeach()
    
    set(MODULE_DEPENDENCIES ${dependencies} PARENT_SCOPE)
endfunction()
