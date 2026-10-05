/*******************************************************************************
* djinterp [core]                                              file_expected.hpp
*
* The C++23 return surface (roadmap Phase 10).
*   std::expected<T, error> carries either a value or the error that stopped
* it, in the return type -- no out-parameter, and it composes (.and_then /
* .transform / .value_or) the way an error& out-parameter cannot.
*   This is ADDITIVE, and that is the whole point. Every query still has its
* error-code form -- status(p, ec), space(p, ec), file_size(p, ec),
* read_symlink(p, ec) -- unchanged, on every tier. This header ADDS a second,
* one-argument overload of each that RETURNS the expected, for callers on
* C++23 who want it. The two forms differ by arity, so they never collide, and
* code written against the error-code core keeps compiling exactly as before.
* A higher standard adds a surface; it does not alter the one below.
*   AVAILABILITY. The whole header is inert unless the library actually has
* <expected>. It gates on __cpp_lib_expected (via <version>), NOT on a
* __cplusplus value -- a compiler may report C++23 as an intermediate number
* well before 202302, so the feature-test macro is the only reliable signal.
* Where <expected> is absent, this header defines nothing and is harmless to
* include.
*
*
* path:      /inc/djinterp/core/fs/file_expected.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.10.03
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  EXPECTED OVERLOADS
    ------------------
    1.  Metadata
    2.  Capacity
    3.  Size
    4.  Links
*/

#ifndef DJINTERP_FS_FILE_EXPECTED_HPP
#define DJINTERP_FS_FILE_EXPECTED_HPP 1

// std
#include <version>  // __cpp_lib_expected
// djinterp
#include "file_path.hpp"    // path
#include "file_common.hpp"  // error, the D_* kit
#include "file_stat.hpp"    // status, file_size, file_status
#include "file_space.hpp"   // space, space_info
#include "file_link.hpp"    // read_symlink, D_FILE_LINK_IS_AVAILABLE
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


#if ( (defined(__cpp_lib_expected)) && \
      (__cpp_lib_expected >= 202202L) )
    // std
    #include <expected>  // std::expected, std::unexpected

    // D_INTERNAL_HAVE_EXPECTED
    //   macro: 1 when this header defines the expected overloads.
    #define D_INTERNAL_HAVE_EXPECTED 1


NS_DJINTERP


//==============================================================================
// 1.  EXPECTED OVERLOADS
//==============================================================================
// Each returns the value on success and std::unexpected(error) on failure.


// 1.1    Metadata
//------------------------------------------------------------------------------
/**
 * @brief Retrieves the metadata for a path, or the error that prevented it;
 *        the one-argument overload of status(path, error&).
 *
 * @note "Not there" is a value whose exists() is false, NOT an unexpected:
 *       absence is an answer, not a failure. The error-code form reports it
 *       as a cleared error plus a type_not_found status.
 *
 * @param[in] _p  the path to query.
 * @return the status, or std::unexpected with the error.
 */
inline std::expected<file_status, error>
status(
    const path& _p
)
{
    error       ec;
    file_status s = status(_p,
                           ec);

    // a failed query carries its error in the return
    if (ec.failed())
    {
        return std::unexpected(ec);
    }

    return s;
}

// 1.2    Capacity
//------------------------------------------------------------------------------
/**
 * @brief Retrieves the capacity of the filesystem holding a path, or the
 *        error; the one-argument overload of space(path, error&).
 *
 * @param[in] _p  a path on the filesystem of interest.
 * @return the capacity, or std::unexpected with the error.
 */
inline std::expected<space_info, error>
space(
    const path& _p
)
{
    error      ec;
    space_info s = space(_p,
                         ec);

    // a failed query carries its error in the return
    if (ec.failed())
    {
        return std::unexpected(ec);
    }

    return s;
}

// 1.3    Size
//------------------------------------------------------------------------------
/**
 * @brief Retrieves the byte size of a regular file, or the error; the
 *        one-argument overload of file_size(path, error&).
 *
 * @param[in] _p  the file to measure.
 * @return the size, or std::unexpected with the error -- EINVAL for a
 *         non-regular path, ENOENT for an absent one, as the error-code form
 *         reports them.
 */
inline std::expected<uint64_t, error>
file_size(
    const path& _p
)
{
    error    ec;
    uint64_t n = file_size(_p,
                           ec);

    // a failed query carries its error in the return
    if (ec.failed())
    {
        return std::unexpected(ec);
    }

    return n;
}

// 1.4    Links
//------------------------------------------------------------------------------
#if D_FILE_LINK_IS_AVAILABLE
/**
 * @brief Retrieves a symbolic link's target text, or the error; the
 *        one-argument overload of read_symlink(path, error&).
 *
 * @param[in] _p  the symbolic link to read.
 * @return the target, unresolved, or std::unexpected with the error.
 */
inline std::expected<path, error>
read_symlink(
    const path& _p
)
{
    error ec;
    path  target = read_symlink(_p,
                                ec);

    // a failed read carries its error in the return
    if (ec.failed())
    {
        return std::unexpected(ec);
    }

    return target;
}
#endif  // D_FILE_LINK_IS_AVAILABLE


NS_END  // djinterp


#else
    #define D_INTERNAL_HAVE_EXPECTED 0
#endif  // __cpp_lib_expected

#endif  // defined(INT64_MAX)

#endif  // DJINTERP_FS_FILE_EXPECTED_HPP
