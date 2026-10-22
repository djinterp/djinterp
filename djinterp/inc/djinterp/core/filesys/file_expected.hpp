/*******************************************************************************
* djinterp [core]                                              file_expected.hpp
*
* The C++ return surface by value (roadmap Phase 10).
*   re_re_std::expected<T, error> carries either a value or the error that
* stopped it, in the return type -- no out-parameter, and it composes
* (.and_then / .transform / .value_or) the way an error& out-parameter
* cannot. It is re_std's, as the C++ guide has every fallible C++ function
* report: one type at every level from C++11, where the standard's is C++23's
* alone, and a symbol's type may not change between levels.
*   This is ADDITIVE, and that is the whole point. Every query still has its
* error-code form -- status(p, ec), space(p, ec), file_size(p, ec),
* read_symlink(p, ec) -- unchanged, on every tier. This header ADDS a second,
* one-argument overload of each that RETURNS the expected. The two forms
* differ by arity, so they never collide, and code written against the
* error-code core keeps compiling exactly as before.
*   AVAILABILITY. Below C++11, where re_std's expected cannot be written, the
* header defines nothing and is harmless to include. The link overload is also
* gated on D_FILE_LINK_IS_AVAILABLE; the rest return what their error-code
* forms do at D_FILE_WIDTH, and exist where those do: wherever the width is
* not narrowed (D_FILE_WIDTH_IS_NARROWED).
*
*
* path:      /inc/djinterp/core/filesys/file_expected.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.10.20
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

#ifndef DJINTERP_FILESYS_FILE_EXPECTED_HPP
#define DJINTERP_FILESYS_FILE_EXPECTED_HPP 1

// djinterp
#include "file_path.hpp"    // path
#include "file_common.hpp"  // error, the D_* kit
#include "file_stat.hpp"    // status, file_size, file_status
#include "file_space.hpp"   // space, space_info
#include "file_link.hpp"    // read_symlink, D_FILE_LINK_IS_AVAILABLE


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "../../../re_std/expected/expected.hpp"    // re_std::expected
#include "../../../re_std/expected/unexpected.hpp"  // re_std::unexpected

// D_INTERNAL_HAVE_EXPECTED
//   macro: 1 when this header defines the expected overloads.
#define D_INTERNAL_HAVE_EXPECTED 1


NS_DJINTERP


//==============================================================================
// 1.  EXPECTED OVERLOADS
//==============================================================================
// Each returns the value on success and re_std::unexpected(error) on
// failure.


// 1.1    Metadata
//------------------------------------------------------------------------------
#if !D_FILE_WIDTH_IS_NARROWED
/**
 * @brief Retrieves the metadata for a path, or the error that prevented it;
 *        the one-argument overload of status(path, error&).
 *
 * @note "Not there" is a value whose exists() is false, NOT an unexpected:
 *       absence is an answer, not a failure. The error-code form reports it
 *       as a cleared error plus a type_not_found status.
 *
 * @param[in] _p  the path to query.
 * @return the status, or re_std::unexpected with the error.
 */
inline re_std::expected<file_status, error>
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
        return re_std::unexpected<error>(ec);
    }

    return s;
}
#endif  // !D_FILE_WIDTH_IS_NARROWED

// 1.2    Capacity
//------------------------------------------------------------------------------
#if !D_FILE_WIDTH_IS_NARROWED
/**
 * @brief Retrieves the capacity of the filesystem holding a path, or the
 *        error; the one-argument overload of space(path, error&).
 *
 * @param[in] _p  a path on the filesystem of interest.
 * @return the capacity, or re_std::unexpected with the error.
 */
inline re_std::expected<space_info, error>
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
        return re_std::unexpected<error>(ec);
    }

    return s;
}
#endif  // !D_FILE_WIDTH_IS_NARROWED

// 1.3    Size
//------------------------------------------------------------------------------
#if !D_FILE_WIDTH_IS_NARROWED
/**
 * @brief Retrieves the byte size of a regular file, or the error; the
 *        one-argument overload of file_size(path, error&).
 *
 * @param[in] _p  the file to measure.
 * @return the size, or re_std::unexpected with the error -- EINVAL for a
 *         non-regular path, ENOENT for an absent one, as the error-code form
 *         reports them.
 */
inline re_std::expected<file_status::size_type, error>
file_size(
    const path& _p
)
{
    error                  ec;
    file_status::size_type n = file_size(_p,
                                         ec);

    // a failed query carries its error in the return
    if (ec.failed())
    {
        return re_std::unexpected<error>(ec);
    }

    return n;
}
#endif  // !D_FILE_WIDTH_IS_NARROWED

// 1.4    Links
//------------------------------------------------------------------------------
#if D_FILE_LINK_IS_AVAILABLE
/**
 * @brief Retrieves a symbolic link's target text, or the error; the
 *        one-argument overload of read_symlink(path, error&).
 *
 * @param[in] _p  the symbolic link to read.
 * @return the target, unresolved, or re_std::unexpected with the error.
 */
inline re_std::expected<path, error>
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
        return re_std::unexpected<error>(ec);
    }

    return target;
}
#endif  // D_FILE_LINK_IS_AVAILABLE


NS_END  // djinterp


#else
    #define D_INTERNAL_HAVE_EXPECTED 0
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_FILESYS_FILE_EXPECTED_HPP
