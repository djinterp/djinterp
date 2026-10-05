/*******************************************************************************
* djinterp [core]                                               fixed_string.hpp
*
*   fixed_string<N>: structural string-literal carrier suitable for use
* as a C++20 class-type NTTP.  Raw const char[N] doesn't satisfy the
* structural-type constraints for NTTPs; fixed_string<N> does (public
* data, no user-provided assignment / move / destructor, no virtuals).
*   Lives in /meta because it is consumed by multiple unrelated
* subsystems (options, cli, ...) and belongs to neither.  N includes
* the terminating null, matching the underlying array length of a
* string literal.
*
*
* path:      /inc/djinterp/core/meta/fixed_string.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.24
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_META_FIXED_STRING_HPP
#define DJINTERP_META_FIXED_STRING_HPP 1

// THE MODULE FLOOR is C++17 (std::string_view), and below it this header is
// EMPTY rather than an error (README rule 5: a facility is absent from a
// level it cannot express). env comes first, as the floor reads it.
#include "../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <string_view>
// djinterp
#include "../../djinterp.hpp"


NS_DJINTERP


// fixed_string
//   helper: structural string-literal carrier for use as a C++20
// class-type NTTP.  Stores the literal byte-for-byte in a public
// data[] member so the type satisfies the structural-type rules.
template<std::size_t N>
struct fixed_string
{
    char data[N];

    constexpr fixed_string(
        const char (&_s)[N]
    )
    {
        for (std::size_t i = 0; i < N; ++i)
        {
            data[i] = _s[i];
        }
    }

    // c_str
    //   accessor: null-terminated C-string view of the payload.
    constexpr const char*
    c_str() const noexcept
    {
        return data;
    }

    // size
    //   accessor: payload length excluding the terminating null.
    constexpr std::size_t
    size() const noexcept
    {
        return N - 1;
    }

    // view
    //   accessor: string_view over the payload (no null included).
    // Constexpr so it composes into both compile-time and runtime
    // contexts without ceremony.
    constexpr std::string_view
    view() const noexcept
    {
        return std::string_view{data, N - 1};
    }
};

// deduction guide
template<std::size_t N>
fixed_string(const char (&)[N]) -> fixed_string<N>;


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


#endif  // DJINTERP_META_FIXED_STRING_HPP
