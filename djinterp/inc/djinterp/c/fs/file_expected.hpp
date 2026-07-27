/******************************************************************************
* djinterp [fs]                                              file_expected.hpp
*
*   The C++23 return surface (roadmap Phase 10). std::expected<T, error> carries
* either a value or the error that stopped it, in the return type -- no
* out-parameter, and it composes (.and_then / .transform / .value_or) the way an
* error& out-parameter cannot.
*
*   This is ADDITIVE, and that is the whole point. Every query still has its
* error-code form -- status(p, ec), space(p, ec), file_size(p, ec),
* read_symlink(p, ec) -- unchanged, on every tier from C++98 up. This header
* ADDS a second, one-argument overload of each that RETURNS the expected, for
* callers on C++23 who want it. The two forms differ by arity, so they never
* collide, and code written against the error-code core keeps compiling exactly
* as before. A higher standard adds a surface; it does not alter the one below.
*
*   AVAILABILITY. The whole header is inert unless the library actually has
* <expected>. It gates on __cpp_lib_expected (via <version>), NOT on a
* __cplusplus value -- a compiler may report C++23 as an intermediate number
* well before 202302, so the feature-test macro is the only reliable signal.
* Where <expected> is absent, this header defines nothing and is harmless to
* include.
*
* 
* path:      /inc/djinterp/core/fs/file_expected.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.19
******************************************************************************/

#ifndef DJINTERP_FS_FILE_EXPECTED_
#define DJINTERP_FS_FILE_EXPECTED_ 1

#include "file_path.hpp"
#include "file_common.hpp"
#include "file_stat.hpp"
#include "file_space.hpp"
#include "file_link.hpp"

#include <version>   // populate the library feature-test macros

#if defined(__cpp_lib_expected) && (__cpp_lib_expected >= 202202L)

#include <expected>

#define D_INTERNAL_HAVE_EXPECTED 1


NS_DJINTERP

// status (expected form)
//   function: the metadata for _p, or the error that prevented it. The
// one-argument overload of status(path, error&); a value on success,
// std::unexpected(error) on failure -- including the deliberate "not there"
// case, which the error-code form reports as a cleared error plus a
// type_not_found status, and which here is simply a value whose exists() is
// false (NOT an unexpected -- absence is an answer, not a failure).
inline std::expected<file_status, error>
status(const path& _p)
{
    error       ec;
    file_status s = status(_p, ec);

    if (ec.failed())
    {
        return std::unexpected(ec);
    }

    return s;
}

// space (expected form)
//   function: filesystem capacity for _p, or the error.
inline std::expected<space_info, error>
space(const path& _p)
{
    error      ec;
    space_info s = space(_p, ec);

    if (ec.failed())
    {
        return std::unexpected(ec);
    }

    return s;
}

// file_size (expected form)
//   function: the byte size of a regular file, or the error (EINVAL for a
// non-regular path, ENOENT for an absent one -- the same reasons the error-code
// form reports, now carried in the return).
inline std::expected<uint64_t, error>
file_size(const path& _p)
{
    error    ec;
    uint64_t n = file_size(_p, ec);

    if (ec.failed())
    {
        return std::unexpected(ec);
    }

    return n;
}

#if D_FILE_LINK_IS_AVAILABLE
// read_symlink (expected form)
//   function: a symlink's target text, or the error.
inline std::expected<path, error>
read_symlink(const path& _p)
{
    error ec;
    path  target = read_symlink(_p, ec);

    if (ec.failed())
    {
        return std::unexpected(ec);
    }

    return target;
}
#endif // D_FILE_LINK_IS_AVAILABLE

NS_END  // djinterp

#else   // no <expected>

#define D_INTERNAL_HAVE_EXPECTED 0

#endif  // __cpp_lib_expected


#endif // DJINTERP_FS_FILE_EXPECTED_
