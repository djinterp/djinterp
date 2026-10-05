/*******************************************************************************
* djinterp [re_std]                                     string_view_literals.hpp
*
* string_view literal operators header:
*   The operator""sv user-defined literals, one per built-in character
* type, living in re_std::literals::string_view_literals (a doubly
* inline namespace, mirroring std). Each yields the matching view type
* over the literal's data and length, with no allocation.
*
*   PORTABILITY:
*   User-defined literals are a C++11 feature, so the whole file is
* gated on C++11 (std introduced the sv literals in C++17 — a back-
* port). The char8_t overload is gated on C++20. The suffix "sv" is
* reserved for the standard library, so the definitions are wrapped in
* a compiler-diagnostic suppression to keep -Wall -Wextra clean while
* still matching the standard spelling users expect.
*
*
* path:      /inc/re_std/string_view/string_view_literals.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.04
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_STRING_VIEW_STRING_VIEW_LITERALS_HPP
#define RE_STD_STRING_VIEW_STRING_VIEW_LITERALS_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER


// re_std
#include "./basic_string_view.hpp"
#include "./string_view_typedefs.hpp"

// std (fundamental types only)
// std
#include <cstddef>


namespace re_std
{

inline namespace literals
{
inline namespace string_view_literals
{

// The "sv" suffix lacks the leading underscore reserved for user code;
// it is the standard-mandated spelling. Suppress the reserved-suffix
// diagnostic around these definitions only. The space in `operator"" sv` is
// C++11's required spelling (C++14 made it optional, CWG 1473); every later
// standard accepts it, as chrono_literals.hpp's operators rely on too.
#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wuser-defined-literals"
#elif defined(__GNUC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wliteral-suffix"
#endif

// operator"" sv(const char*, size_t)
//   function: a string_view over the char literal.
inline RE_STD_CONSTEXPR string_view
operator"" sv(
    const char*  _s,
    std::size_t  _len
) RE_STD_NOEXCEPT
{
    return string_view(_s, _len);
}

// operator"" sv(const wchar_t*, size_t)
//   function: a wstring_view over the wide literal.
inline RE_STD_CONSTEXPR wstring_view
operator"" sv(
    const wchar_t*  _s,
    std::size_t     _len
) RE_STD_NOEXCEPT
{
    return wstring_view(_s, _len);
}

// operator"" sv(const char16_t*, size_t)
//   function: a u16string_view over the char16_t literal.
inline RE_STD_CONSTEXPR u16string_view
operator"" sv(
    const char16_t*  _s,
    std::size_t      _len
) RE_STD_NOEXCEPT
{
    return u16string_view(_s, _len);
}

// operator"" sv(const char32_t*, size_t)
//   function: a u32string_view over the char32_t literal.
inline RE_STD_CONSTEXPR u32string_view
operator"" sv(
    const char32_t*  _s,
    std::size_t      _len
) RE_STD_NOEXCEPT
{
    return u32string_view(_s, _len);
}

#if RE_STD_LANG_IS_CPP20_OR_HIGHER
// operator"" sv(const char8_t*, size_t)
//   function: a u8string_view over the char8_t literal (C++20).
inline constexpr u8string_view
operator"" sv(
    const char8_t*  _s,
    std::size_t     _len
) noexcept
{
    return u8string_view(_s, _len);
}
#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER

#if defined(__clang__)
    #pragma clang diagnostic pop
#elif defined(__GNUC__)
    #pragma GCC diagnostic pop
#endif

}  // namespace string_view_literals
}  // namespace literals

}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_STRING_VIEW_STRING_VIEW_LITERALS_HPP
