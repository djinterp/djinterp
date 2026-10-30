#*******************************************************************************
# vparse [cmake]                                              OutputLayout.cmake
#
# Directs the workbench's binaries to <djinterp>/bin/tools/vparse/, as
# bitwisedom's go to bin/tools/bitwisedom/: Debug products in the debug/
# child directory, other configurations in the directory itself. Every owned
# binary and library carries a target-architecture suffix (win64, win32,
# winarm64, linux64, macosarm64, ...). Dependencies -- uxoxo, Dear ImGui --
# and the test executables keep their own layout.
#
# path:      /build/cmake/config/djinterp/tools/vparse/cmake/OutputLayout.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
#                                                            revised: 2026.10.15
#*******************************************************************************
include_guard(GLOBAL)

# CMake's system name is the TARGET platform in cross builds, not the host.
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(_vparse_os "win")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(_vparse_os "macos")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(_vparse_os "linux")
else()
    string(TOLOWER "${CMAKE_SYSTEM_NAME}" _vparse_os)
    string(MAKE_C_IDENTIFIER "${_vparse_os}" _vparse_os)
endif()

# Prefer the compiler/VS target over the machine running CMake. Pointer
# width is the fallback for ordinary x86 builds and toolchain files.
set(_vparse_cpu "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}")
if(CMAKE_GENERATOR_PLATFORM)
    set(_vparse_cpu "${CMAKE_GENERATOR_PLATFORM}")
elseif(NOT _vparse_cpu)
    set(_vparse_cpu "${CMAKE_SYSTEM_PROCESSOR}")
endif()
string(TOLOWER "${_vparse_cpu}" _vparse_cpu)

if(_vparse_cpu MATCHES "(aarch64|arm64)")
    set(_vparse_arch "arm64")
elseif(_vparse_cpu MATCHES "(armv[0-9]|^arm$)")
    set(_vparse_arch "arm32")
elseif(_vparse_cpu MATCHES "(riscv64)")
    set(_vparse_arch "riscv64")
elseif(_vparse_cpu MATCHES "(x86_64|amd64|^x64$)")
    set(_vparse_arch "64")
elseif(_vparse_cpu MATCHES "(i[3-6]86|^x86$|^win32$)")
    set(_vparse_arch "32")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_vparse_arch "64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(_vparse_arch "32")
else()
    message(FATAL_ERROR
        "Could not determine vparse target architecture. "
        "Set -DVPARSE_PLATFORM_TAG=win64 (or equivalent).")
endif()

set(VPARSE_PLATFORM_TAG "${_vparse_os}${_vparse_arch}"
    CACHE STRING "Suffix for built binaries, e.g. win64 or linux64")
set(VPARSE_BIN_DIR "${DJINTERP_ROOT}/bin/tools/vparse"
    CACHE PATH "Destination for vparse executables and libraries")

# vparse_output(<target>)
#   Use target-specific properties, not global CMAKE_*_OUTPUT_DIRECTORY:
# dependency projects such as uxoxo own their output layout. Multi-config
# generators must be told each configuration's directory to avoid adding
# /Release or /Debug after the requested directory.
function(vparse_output _target)
    set(_stem "${_target}-${VPARSE_PLATFORM_TAG}")
    set_target_properties(${_target} PROPERTIES
        OUTPUT_NAME "${_stem}"
        PDB_NAME "${_stem}"
        COMPILE_PDB_NAME "${_stem}-compile"
        RUNTIME_OUTPUT_DIRECTORY "${VPARSE_BIN_DIR}"
        LIBRARY_OUTPUT_DIRECTORY "${VPARSE_BIN_DIR}"
        ARCHIVE_OUTPUT_DIRECTORY "${VPARSE_BIN_DIR}"
        PDB_OUTPUT_DIRECTORY "${VPARSE_BIN_DIR}"
        COMPILE_PDB_OUTPUT_DIRECTORY "${VPARSE_BIN_DIR}")

    get_target_property(_kind ${_target} TYPE)
    if(_kind MATCHES "^(STATIC_LIBRARY|SHARED_LIBRARY|MODULE_LIBRARY)$")
        # On non-Windows platforms, the default 'lib' prefix would make
        # the filename lib<name>-<architecture>.a instead of <name>-....
        set_target_properties(${_target} PROPERTIES PREFIX "")
    endif()

    foreach(_config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
        if(_config STREQUAL "DEBUG")
            set(_where "${VPARSE_BIN_DIR}/debug")
        else()
            set(_where "${VPARSE_BIN_DIR}")
        endif()
        set_target_properties(${_target} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_${_config} "${_where}"
            LIBRARY_OUTPUT_DIRECTORY_${_config} "${_where}"
            ARCHIVE_OUTPUT_DIRECTORY_${_config} "${_where}"
            PDB_OUTPUT_DIRECTORY_${_config} "${_where}"
            COMPILE_PDB_OUTPUT_DIRECTORY_${_config} "${_where}")
    endforeach()
endfunction()

message(STATUS "vparse: output tag ${VPARSE_PLATFORM_TAG}")
message(STATUS "vparse: Release binaries ${VPARSE_BIN_DIR}")
message(STATUS "vparse: Debug binaries ${VPARSE_BIN_DIR}/debug")
