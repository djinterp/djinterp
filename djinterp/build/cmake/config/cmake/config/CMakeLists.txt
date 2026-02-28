###############################################################################
# djinterp - master build configuration
# 
# Top-level CMake entry point for the djinterp project.
# Builds djinterp.lib — the combined library spanning all language modules.
#
# Location: <root>/build/cmake/config/CMakeLists.txt
#
# author(s): Samuel 'teer' Neal-Blim                          date: 2023.03.01
###############################################################################
cmake_minimum_required(VERSION 3.20)

# Project name
project(djinterp VERSION 0.1 LANGUAGES C CXX)

###############################################################################
# DIRECTORY SETUP
###############################################################################

# Determine the project root directory
# This CMakeLists.txt is located at: <root>/build/cmake/config/CMakeLists.txt
# So we need to go up 3 levels to reach the root
#   config -> cmake -> build -> <root>
get_filename_component(DJINTERP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)

# Detect platform architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(PLATFORM_ARCH "x64")
else()
    set(PLATFORM_ARCH "x86")
endif()

# Include common CMake utilities (lives alongside this file in config/)
include(${CMAKE_CURRENT_SOURCE_DIR}/common.cmake)

# Set C/C++ standards (propagated to all submodules)
set(CMAKE_C_STANDARD 17 CACHE INTERNAL "")
set(CMAKE_C_STANDARD_REQUIRED ON CACHE INTERNAL "")
set(CMAKE_CXX_STANDARD 23 CACHE INTERNAL "")
set(CMAKE_CXX_STANDARD_REQUIRED ON CACHE INTERNAL "")

# Print configuration
message(STATUS "========================================")
message(STATUS "djinterp Master Configuration")
message(STATUS "========================================")
message(STATUS "Root:              ${DJINTERP_ROOT}")
message(STATUS "Platform:          ${PLATFORM_ARCH}")
message(STATUS "C Standard:        ${CMAKE_C_STANDARD}")
message(STATUS "C++ Standard:      ${CMAKE_CXX_STANDARD}")
message(STATUS "========================================")

###############################################################################
# COMPILER FLAGS
###############################################################################

if(MSVC)
    add_compile_options(/FS)
endif()

###############################################################################
# LANGUAGE MODULES
###############################################################################

# C language module — builds djinterpc.lib and all C submodules
add_subdirectory(c)

# Future language modules would be added here:
# add_subdirectory(cpp)
# add_subdirectory(asm)

###############################################################################
# COMBINED LIBRARY: djinterp.lib
#
# Aggregates all language-specific object libraries into a single djinterp.lib
# that downstream consumers can link against.
###############################################################################

# Set output directories for the combined library
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${DJINTERP_ROOT}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${DJINTERP_ROOT}/lib")

foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${DJINTERP_ROOT}/lib")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${DJINTERP_ROOT}/lib")
endforeach()

# Gather all C object library objects
set(DJINTERP_ALL_OBJECTS "")
if(TARGET djinterpc-objects)
    list(APPEND DJINTERP_ALL_OBJECTS $<TARGET_OBJECTS:djinterpc-objects>)
endif()

# Future: add object libraries from other language modules
# if(TARGET djinterpcpp-objects)
#     list(APPEND DJINTERP_ALL_OBJECTS $<TARGET_OBJECTS:djinterpcpp-objects>)
# endif()

if(DJINTERP_ALL_OBJECTS)
    add_library(djinterp-lib STATIC ${DJINTERP_ALL_OBJECTS})
    set_target_properties(djinterp-lib PROPERTIES OUTPUT_NAME "djinterp")

    if(TARGET djinterpc)
        target_link_libraries(djinterp-lib PUBLIC djinterpc)
    endif()

    if(MSVC)
        set_target_properties(djinterp-lib PROPERTIES
            COMPILE_PDB_NAME "djinterp"
            COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/djinterp.dir"
        )
    endif()

    message(STATUS "Created combined library: djinterp.lib")
else()
    message(STATUS "No object libraries found — djinterp.lib not created")
endif()

###############################################################################
# SUMMARY
###############################################################################

message(STATUS "")
message(STATUS "djinterp Master Build Summary:")
message(STATUS "  Language modules: c")
message(STATUS "  Combined library: djinterp.lib")
message(STATUS "")