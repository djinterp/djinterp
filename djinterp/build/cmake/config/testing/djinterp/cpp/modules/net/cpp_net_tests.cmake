# =============================================================================
# djinterp [test] -
# build/cmake/config/testing/djinterp/cpp/modules/net/cpp_net_tests.cmake
#
#  The function net's C++ leaves make their programs with. A program is one
# of tests/djinterp/cpp/modules/net's mains with its tests, built twice, at
# C++98, the face's floor, and at C++20, as cpp_net_<name>_tests_cxx98 and
# _cxx20: CTest tests whose exit status is the result. Each links net's core
# and codec, net_common.cpp, the C sources it names, and dmutex; a library
# it asks for is linked where CMake finds it and configured away where it
# doesn't, as net's C leaves do. The programs use alarm and SIGPIPE, so they
# are built on POSIX systems alone.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.20
# =============================================================================

# djinterp_cpp_net_program(NAME <name> SOURCES <tests...> [C <sources...>]
#                          [CXX <sources...>] [OPENSSL] [CURL] [LIBSSH2])
function(djinterp_cpp_net_program)
    cmake_parse_arguments(_p "OPENSSL;CURL;LIBSSH2" "NAME" "SOURCES;C;CXX"
                          ${ARGN})

    if (WIN32)
        return()
    endif()

    set(_root "${DJINTERP_SRC_ROOT}/djinterp/c/modules/net")
    file(GLOB _url   CONFIGURE_DEPENDS "${_root}/net_url*.c")
    file(GLOB _codec CONFIGURE_DEPENDS "${_root}/codec/codec_*.c")
    set(_core "${_root}/net_common.c" "${_root}/net_memory.c"
              ${_url} ${_codec})

    find_package(Threads REQUIRED)
    if (_p_OPENSSL)
        find_package(OpenSSL QUIET)
    endif()
    if (_p_CURL)
        find_package(CURL QUIET)
    endif()
    if (_p_LIBSSH2)
        find_path(DJINTERP_LIBSSH2_INCLUDE NAMES libssh2.h)
        find_library(DJINTERP_LIBSSH2_LIBRARY NAMES ssh2 libssh2)
    endif()

    foreach(_level 98 20)
        set(_t cpp_net_${_p_NAME}_tests_cxx${_level})
        djinterp_add_test_executable(${_t}
            SOURCES
                ${_p_SOURCES}
                ${_p_CXX}
                "${DJINTERP_SRC_ROOT}/djinterp/cpp/modules/net/net_common.cpp"
                ${_p_C}
                ${_core})
        # the level asked, not the C++20 every target is given: C++98 drops
        # it, and without extensions is the framework's strict ISO mode
        set_target_properties(${_t} PROPERTIES
            CXX_STANDARD          ${_level}
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS        OFF)
        if (_level EQUAL 98)
            set_property(TARGET ${_t} PROPERTY COMPILE_FEATURES c_std_17)
            target_compile_definitions(${_t} PRIVATE
                $<$<COMPILE_LANGUAGE:CXX>:D_CFG_ENV_ISO_STRICT=1>)
        endif()
        target_link_libraries(${_t} PRIVATE djinterp_c_sync Threads::Threads)

        # each library where CMake finds it, and configured away where it
        # doesn't, so headers CMake didn't find aren't used either
        if (_p_OPENSSL)
            if (OpenSSL_FOUND)
                target_link_libraries(${_t} PRIVATE
                                      OpenSSL::SSL OpenSSL::Crypto)
            else()
                target_compile_definitions(${_t} PRIVATE
                    D_CFG_TLS_OPENSSL=0 D_ENV_TLS_HAS_OPENSSL=0)
            endif()
        endif()
        if (_p_CURL)
            if (CURL_FOUND)
                target_link_libraries(${_t} PRIVATE CURL::libcurl)
            else()
                target_compile_definitions(${_t} PRIVATE D_CFG_CURL=0)
            endif()
        endif()
        if (_p_LIBSSH2)
            if (DJINTERP_LIBSSH2_INCLUDE AND DJINTERP_LIBSSH2_LIBRARY)
                target_include_directories(${_t} PRIVATE
                                           "${DJINTERP_LIBSSH2_INCLUDE}")
                target_link_libraries(${_t} PRIVATE
                                      "${DJINTERP_LIBSSH2_LIBRARY}")
            else()
                target_compile_definitions(${_t} PRIVATE D_CFG_SSH_LIBSSH2=0)
            endif()
        endif()
    endforeach()
endfunction()
