#*******************************************************************************
# bitwisedom [cmake]                                          OutputLayout.cmake
#
# Direct bitwisedom binaries to <djinterp>/bin/tools/bitwisedom/.
# Debug products live in the debug/ child directory. Other configurations
# use the parent directory. All owned binaries and libraries carry a target-
# architecture suffix (win64, win32, winarm64, linux64, macosarm64, ...).
# Third-party dependency targets are intentionally left alone.
#
# path:      /build/cmake/djinterp/tools/bitwisedom/cmake/OutputLayout.cmake
#*******************************************************************************
include_guard(GLOBAL)

# CMake's system name is the TARGET platform in cross builds, not the host.
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(_bitwisedom_os "win")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(_bitwisedom_os "macos")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(_bitwisedom_os "linux")
else()
    string(TOLOWER "${CMAKE_SYSTEM_NAME}" _bitwisedom_os)
    string(MAKE_C_IDENTIFIER "${_bitwisedom_os}" _bitwisedom_os)
endif()

# Prefer the compiler/VS target over the machine running CMake. Pointer
# width is the fallback for ordinary x86 builds and toolchain files.
set(_bitwisedom_cpu "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}")
if(CMAKE_GENERATOR_PLATFORM)
    set(_bitwisedom_cpu "${CMAKE_GENERATOR_PLATFORM}")
elseif(NOT _bitwisedom_cpu)
    set(_bitwisedom_cpu "${CMAKE_SYSTEM_PROCESSOR}")
endif()
string(TOLOWER "${_bitwisedom_cpu}" _bitwisedom_cpu)

if(_bitwisedom_cpu MATCHES "(aarch64|arm64)")
    set(_bitwisedom_arch "arm64")
elseif(_bitwisedom_cpu MATCHES "(armv[0-9]|^arm$)")
    set(_bitwisedom_arch "arm32")
elseif(_bitwisedom_cpu MATCHES "(riscv64)")
    set(_bitwisedom_arch "riscv64")
elseif(_bitwisedom_cpu MATCHES "(x86_64|amd64|^x64$)")
    set(_bitwisedom_arch "64")
elseif(_bitwisedom_cpu MATCHES "(i[3-6]86|^x86$|^win32$)")
    set(_bitwisedom_arch "32")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_bitwisedom_arch "64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(_bitwisedom_arch "32")
else()
    message(FATAL_ERROR
        "Could not determine bitwisedom target architecture. "
        "Set -DBITWISEDOM_PLATFORM_TAG=win64 (or equivalent).")
endif()

set(BITWISEDOM_PLATFORM_TAG "${_bitwisedom_os}${_bitwisedom_arch}"
    CACHE STRING "Suffix for built binaries, e.g. win64 or linux64")
set(BITWISEDOM_BIN_DIR "${BITWISEDOM_ROOT}/bin/tools/bitwisedom"
    CACHE PATH "Destination for bitwisedom executables and libraries")

# bitwisedom_output(<target>)
#   Use target-specific properties, not global CMAKE_*_OUTPUT_DIRECTORY:
# dependency projects such as uxoxo own their output layout. Multi-config
# generators must be told each configuration's directory to avoid adding
# /Release or /Debug after the requested directory.
function(bitwisedom_output _target)
    set(_stem "${_target}-${BITWISEDOM_PLATFORM_TAG}")
    set_target_properties(${_target} PROPERTIES
        OUTPUT_NAME "${_stem}"
        PDB_NAME "${_stem}"
        COMPILE_PDB_NAME "${_stem}-compile"
        RUNTIME_OUTPUT_DIRECTORY "${BITWISEDOM_BIN_DIR}"
        LIBRARY_OUTPUT_DIRECTORY "${BITWISEDOM_BIN_DIR}"
        ARCHIVE_OUTPUT_DIRECTORY "${BITWISEDOM_BIN_DIR}"
        PDB_OUTPUT_DIRECTORY "${BITWISEDOM_BIN_DIR}"
        COMPILE_PDB_OUTPUT_DIRECTORY "${BITWISEDOM_BIN_DIR}")

    get_target_property(_kind ${_target} TYPE)
    if(_kind MATCHES "^(STATIC_LIBRARY|SHARED_LIBRARY|MODULE_LIBRARY)$")
        # On non-Windows platforms, the default 'lib' prefix would make
        # the filename lib<name>-<architecture>.a instead of <name>-....
        set_target_properties(${_target} PROPERTIES PREFIX "")
    endif()

    foreach(_config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
        if(_config STREQUAL "DEBUG")
            set(_where "${BITWISEDOM_BIN_DIR}/debug")
        else()
            set(_where "${BITWISEDOM_BIN_DIR}")
        endif()
        set_target_properties(${_target} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_${_config} "${_where}"
            LIBRARY_OUTPUT_DIRECTORY_${_config} "${_where}"
            ARCHIVE_OUTPUT_DIRECTORY_${_config} "${_where}"
            PDB_OUTPUT_DIRECTORY_${_config} "${_where}"
            COMPILE_PDB_OUTPUT_DIRECTORY_${_config} "${_where}")
    endforeach()
endfunction()

message(STATUS "bitwisedom: output tag ${BITWISEDOM_PLATFORM_TAG}")
message(STATUS "bitwisedom: Release binaries ${BITWISEDOM_BIN_DIR}")
message(STATUS "bitwisedom: Debug binaries ${BITWISEDOM_BIN_DIR}/debug")
