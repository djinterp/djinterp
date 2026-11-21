# =============================================================================
# djinterp - build/cmake/djinterp/net.cmake
#
#   net's outside libraries, found once for every leaf of c/modules/net:
# OpenSSL (TLS and STARTTLS), libcurl, and libssh2 (SSH and SFTP), through
# its CMake package or, failing that, pkg-config. A library that isn't found
# defines nothing; a leaf asks for what it needs with
# djinterp_net_requires, and is skipped, with a status line, when something
# is missing. No leaf calls find_package itself.
#   Each library found is given one name the leaves link, whatever found it:
#     djinterp::openssl   OpenSSL::SSL and OpenSSL::Crypto
#     djinterp::curl      CURL::libcurl
#     djinterp::libssh2   libssh2's own target, or pkg-config's
# =============================================================================

find_package(OpenSSL QUIET)
if (TARGET OpenSSL::SSL AND TARGET OpenSSL::Crypto)
    add_library(djinterp_net_openssl INTERFACE)
    target_link_libraries(djinterp_net_openssl
        INTERFACE OpenSSL::SSL OpenSSL::Crypto)
    add_library(djinterp::openssl ALIAS djinterp_net_openssl)
endif()

find_package(CURL QUIET)
if (TARGET CURL::libcurl)
    add_library(djinterp_net_curl INTERFACE)
    target_link_libraries(djinterp_net_curl INTERFACE CURL::libcurl)
    add_library(djinterp::curl ALIAS djinterp_net_curl)
endif()

#   libssh2 ships a CMake package under either of two names, and many
# systems have only its pkg-config file.
find_package(libssh2 CONFIG QUIET)
if (TARGET libssh2::libssh2)
    set(_djinterp_libssh2 libssh2::libssh2)
elseif (TARGET Libssh2::libssh2)
    set(_djinterp_libssh2 Libssh2::libssh2)
else()
    find_package(PkgConfig QUIET)
    if (PKG_CONFIG_FOUND)
        pkg_check_modules(DJINTERP_LIBSSH2 QUIET IMPORTED_TARGET libssh2)
        if (TARGET PkgConfig::DJINTERP_LIBSSH2)
            set(_djinterp_libssh2 PkgConfig::DJINTERP_LIBSSH2)
        endif()
    endif()
endif()
if (DEFINED _djinterp_libssh2)
    add_library(djinterp_net_libssh2 INTERFACE)
    target_link_libraries(djinterp_net_libssh2 INTERFACE ${_djinterp_libssh2})
    add_library(djinterp::libssh2 ALIAS djinterp_net_libssh2)
endif()

foreach (_djinterp_net_lib openssl curl libssh2)
    if (TARGET djinterp::${_djinterp_net_lib})
        message(STATUS "djinterp: net: ${_djinterp_net_lib} found")
    else()
        message(STATUS "djinterp: net: ${_djinterp_net_lib} not found")
    endif()
endforeach()

# djinterp_net_requires(LEAF RESULT LIBRARY ...)
#   sets RESULT in the caller's scope to TRUE when every LIBRARY (openssl,
# curl, libssh2) was found, and to FALSE otherwise, saying which LEAF is
# skipped and what it lacks.
function(djinterp_net_requires leaf result)
    set(_missing "")
    foreach (_lib IN LISTS ARGN)
        if (NOT TARGET djinterp::${_lib})
            list(APPEND _missing ${_lib})
        endif()
    endforeach()
    if (_missing)
        list(JOIN _missing ", " _missing_text)
        message(STATUS
            "djinterp: ${leaf} skipped: it needs ${_missing_text}")
        set(${result} FALSE PARENT_SCOPE)
    else()
        set(${result} TRUE PARENT_SCOPE)
    endif()
endfunction()
