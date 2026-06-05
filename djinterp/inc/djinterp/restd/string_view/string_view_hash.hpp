/******************************************************************************
* djinterp [restd]                                         string_view_hash.hpp
*
* string_view hash support header:
*   restd::hash specialisations for the five view aliases. Per the
* standard, hashing a string_view yields the same value as hashing the
* equivalent basic_string — but restd::string does not exist yet, so
* these specialisations are self-contained: a simple polynomial roll
* over the character units. The value is therefore stable within restd
* but is NOT required to agree with any std::hash<std::string_view>.
*
*   PORTABILITY:
*   Gated on C++11 (std added these specialisations in C++17 — a back-
* port). The char8_t / u8string_view specialisation is gated on C++20.
*
*   DEPENDENCIES:
*   ../functional/hash.hpp for the primary restd::hash template that
* these specialise, plus the view aliases from this module.
*
*
* path:      /inc/djinterp/restd/string_view/string_view_hash.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.04
******************************************************************************/

#ifndef DJINTERP_RESTD_STRING_VIEW_STRING_VIEW_HASH_
#define DJINTERP_RESTD_STRING_VIEW_STRING_VIEW_HASH_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER


// djinterp
#include "./basic_string_view.hpp"
#include "./string_view_typedefs.hpp"
#include "../functional/hash.hpp"

// std (fundamental types only)
#include <cstddef>


NS_RESTD


// =============================================================================
// I.   INTERNAL: VIEW HASH ROLL
// =============================================================================

NS_INTERNAL

    // sv_hash_bytes
    //   function: polynomial roll over the character units of _v,
    // widening each through its unsigned counterpart so the high bit
    // never sign-extends. Self-contained so <string_view> has no run-
    // time hash dependency beyond the restd::hash primary template.
    template<typename _CharT,
             typename _Traits>
    inline std::size_t
    sv_hash_bytes(
        basic_string_view<_CharT, _Traits>  _v
    ) D_NOEXCEPT
    {
        std::size_t  _h = static_cast<std::size_t>(1469598103u);
        for (std::size_t _i = 0; _i < _v.size(); ++_i)
        {
            _h = _h * static_cast<std::size_t>(131u)
                 + static_cast<std::size_t>(
                       static_cast<unsigned long>(_v[_i]));
        }
        return _h;
    }

NS_END  // internal


// =============================================================================
// II.  SPECIALISATIONS
// =============================================================================

// hash<string_view>
template<>
struct hash<string_view>
{
    std::size_t
    operator()(
        string_view  _v
    ) const D_NOEXCEPT
    {
        return internal::sv_hash_bytes(_v);
    }
};

// hash<wstring_view>
template<>
struct hash<wstring_view>
{
    std::size_t
    operator()(
        wstring_view  _v
    ) const D_NOEXCEPT
    {
        return internal::sv_hash_bytes(_v);
    }
};

// hash<u16string_view>
template<>
struct hash<u16string_view>
{
    std::size_t
    operator()(
        u16string_view  _v
    ) const D_NOEXCEPT
    {
        return internal::sv_hash_bytes(_v);
    }
};

// hash<u32string_view>
template<>
struct hash<u32string_view>
{
    std::size_t
    operator()(
        u32string_view  _v
    ) const D_NOEXCEPT
    {
        return internal::sv_hash_bytes(_v);
    }
};


#if D_ENV_LANG_IS_CPP20_OR_HIGHER
// hash<u8string_view>  (C++20)
template<>
struct hash<u8string_view>
{
    std::size_t
    operator()(
        u8string_view  _v
    ) const noexcept
    {
        return internal::sv_hash_bytes(_v);
    }
};
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_STRING_VIEW_STRING_VIEW_HASH_
