/******************************************************************************
* djinterp [re_std]                                             char_traits.hpp
*
* character traits header:
*   Provides re_std::char_traits<_CharT> — the generic primary template
* plus the standard specialisations (char, wchar_t, char8_t, char16_t,
* char32_t). This is the string-view-supporting subset of the standard
* char_traits surface: it carries char_type, int_type, and the static
* comparison / search / copy operations that basic_string_view relies
* on. The off_type / pos_type / state_type members (used only by the
* iostream subsystem) and the C++20 comparison_category member are
* deliberately omitted — they are outside what <string_view> needs and
* would pull in <iosfwd> / <cwchar> / <compare>. See the module notes.
*
*   PORTABILITY:
*   char_traits has existed since C++98 for char / wchar_t; the
* char16_t / char32_t specialisations arrived in C++11 and char8_t in
* C++20. re_std matches that: the char / wchar_t paths compile on
* C++98, char16_t / char32_t are gated on C++11, char8_t on C++20.
* The search / length / compare operations become constexpr on C++14
* (relaxed constexpr — loops) versus the standard's C++17, a three-
* year back-port; the trivial element ops (eq, lt, assign(1)) are
* constexpr from C++11.
*
*   DEPENDENCIES:
*   <cstddef> for size_t. <cstdint> for the char16_t / char32_t
* int_type aliases (uint_least16_t / uint_least32_t). <cwchar> for
* wchar_t's int_type (wint_t) and WEOF. No re_std dependencies.
*
*
* path:      /inc/djinterp/re_std/string_view/char_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.04
******************************************************************************/

#ifndef DJINTERP_RE_STD_STRING_VIEW_CHAR_TRAITS_
#define DJINTERP_RE_STD_STRING_VIEW_CHAR_TRAITS_ 1

// djinterp
#include "../../core/djinterp.hpp"

// std (fundamental types only)
#include <cstddef>


// D_CONSTEXPR_CPP14
//   macro: constexpr on C++14+ (relaxed constexpr — locals, loops),
// empty otherwise. Locally defined pending the global qualifier-macro-
// table entry (see roadmap meta note).
#ifndef D_CONSTEXPR_CPP14
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        #define D_CONSTEXPR_CPP14 constexpr
    #else
        #define D_CONSTEXPR_CPP14
    #endif
#endif
NS_RESTD
//   Opened here 2026-08-25. This file previously began its
// namespaced content with no NS_RESTD, so everything above the
// first NS_END lived at GLOBAL SCOPE and that NS_END closed a
// namespace that was never opened.






// =============================================================================
// I.   INTERNAL: SHARED TRAITS BASE
// =============================================================================

NS_INTERNAL

    // char_traits_base
    //   class: parameterised implementation of the char_traits static
    // surface shared by every specialisation. _CharT is the character
    // type; _IntT is the matching int_type. Each public specialisation
    // derives from an instantiation of this and supplies only the
    // members whose value the standard mandates (int_type, eof()).
    template<typename _CharT,
             typename _IntT>
    struct char_traits_base
    {
        typedef _CharT  char_type;
        typedef _IntT   int_type;

        // assign
        //   function: write _c into _r.
        static D_CONSTEXPR_CPP14 void
        assign(
            char_type&        _r,
            const char_type&  _c
        ) D_NOEXCEPT
        {
            _r = _c;
            return;
        }

        // eq
        //   function: true if _a and _b compare equal.
        static D_CONSTEXPR bool
        eq(
            const char_type&  _a,
            const char_type&  _b
        ) D_NOEXCEPT
        {
            return _a == _b;
        }

        // lt
        //   function: true if _a orders before _b.
        static D_CONSTEXPR bool
        lt(
            const char_type&  _a,
            const char_type&  _b
        ) D_NOEXCEPT
        {
            return _a < _b;
        }

        // compare
        //   function: lexicographically compare two ranges of _count
        // characters. Returns <0, 0, >0.
        static D_CONSTEXPR_CPP14 int
        compare(
            const char_type*  _s1,
            const char_type*  _s2,
            std::size_t       _count
        ) D_NOEXCEPT
        {
            for (std::size_t _i = 0; _i < _count; ++_i)
            {
                if (lt(_s1[_i], _s2[_i]))
                {
                    return -1;
                }
                if (lt(_s2[_i], _s1[_i]))
                {
                    return 1;
                }
            }
            return 0;
        }

        // length
        //   function: number of characters before the null terminator.
        static D_CONSTEXPR_CPP14 std::size_t
        length(
            const char_type*  _s
        ) D_NOEXCEPT
        {
            std::size_t _n = 0;
            while (!eq(_s[_n], char_type()))
            {
                ++_n;
            }
            return _n;
        }

        // find
        //   function: pointer to the first occurrence of _c in the
        // first _count characters of _s, or null.
        static D_CONSTEXPR_CPP14 const char_type*
        find(
            const char_type*  _s,
            std::size_t       _count,
            const char_type&  _c
        ) D_NOEXCEPT
        {
            for (std::size_t _i = 0; _i < _count; ++_i)
            {
                if (eq(_s[_i], _c))
                {
                    return _s + _i;
                }
            }
            return 0;
        }

        // move
        //   function: copy _count characters from _src to _dst, correct
        // for overlapping ranges.
        static D_CONSTEXPR_CPP14 char_type*
        move(
            char_type*        _dst,
            const char_type*  _src,
            std::size_t       _count
        ) D_NOEXCEPT
        {
            if (_count == 0 || _dst == _src)
            {
                return _dst;
            }
            if (_dst < _src)
            {
                for (std::size_t _i = 0; _i < _count; ++_i)
                {
                    assign(_dst[_i], _src[_i]);
                }
            }
            else
            {
                for (std::size_t _i = _count; _i != 0; --_i)
                {
                    assign(_dst[_i - 1], _src[_i - 1]);
                }
            }
            return _dst;
        }

        // copy
        //   function: copy _count characters from _src to _dst. Ranges
        // must not overlap.
        static D_CONSTEXPR_CPP14 char_type*
        copy(
            char_type*        _dst,
            const char_type*  _src,
            std::size_t       _count
        ) D_NOEXCEPT
        {
            for (std::size_t _i = 0; _i < _count; ++_i)
            {
                assign(_dst[_i], _src[_i]);
            }
            return _dst;
        }

        // assign (range)
        //   function: write _count copies of _c into _s.
        static D_CONSTEXPR_CPP14 char_type*
        assign(
            char_type*   _s,
            std::size_t  _count,
            char_type    _c
        ) D_NOEXCEPT
        {
            for (std::size_t _i = 0; _i < _count; ++_i)
            {
                assign(_s[_i], _c);
            }
            return _s;
        }

        // to_char_type
        //   function: narrow an int_type back to its char_type.
        static D_CONSTEXPR char_type
        to_char_type(
            const int_type&  _i
        ) D_NOEXCEPT
        {
            return static_cast<char_type>(_i);
        }

        // to_int_type
        //   function: widen a char_type to its int_type.
        static D_CONSTEXPR int_type
        to_int_type(
            const char_type&  _c
        ) D_NOEXCEPT
        {
            return static_cast<int_type>(_c);
        }

        // eq_int_type
        //   function: true if two int_type values are equal (eof
        // compares equal only to eof).
        static D_CONSTEXPR bool
        eq_int_type(
            const int_type&  _a,
            const int_type&  _b
        ) D_NOEXCEPT
        {
            return _a == _b;
        }
    };

NS_END  // internal


// =============================================================================
// II.  GENERIC PRIMARY TEMPLATE
// =============================================================================

// char_traits
//   class: generic primary template. Best-effort traits for any
// char-like _CharT. The standard leaves the primary template's
// behaviour unspecified and only mandates the explicit specialisations
// below; this generic body is a courtesy so user-defined narrow
// char-like types work with basic_string_view. int_type defaults to a
// signed integer wide enough to hold a sentinel distinct from any
// value.
template<typename _CharT>
struct char_traits
    : internal::char_traits_base<_CharT, long>
{
    typedef _CharT  char_type;
    typedef long    int_type;

    // eof
    //   function: the end-of-file sentinel for the generic primary.
    static D_CONSTEXPR int_type
    eof() D_NOEXCEPT
    {
        return static_cast<int_type>(-1);
    }

    // not_eof
    //   function: _i if it is not eof, else a value that is not eof.
    static D_CONSTEXPR int_type
    not_eof(
        const int_type&  _i
    ) D_NOEXCEPT
    {
        return eq_int_type(_i, eof()) ? static_cast<int_type>(0) : _i;
    }

    using internal::char_traits_base<_CharT, long>::eq_int_type;
};


// =============================================================================
// III. char SPECIALISATION  (C++98)
// =============================================================================

// char_traits<char>
//   class: traits for the narrow character type. int_type is int;
// eof() is the conventional -1 (EOF). The standard requires lt() to
// compare as unsigned char, so the comparison overrides the base.
template<>
struct char_traits<char>
    : internal::char_traits_base<char, int>
{
    // lt
    //   function: ordered comparison as unsigned char, per the
    // standard's requirement for char_traits<char>.
    static D_CONSTEXPR bool
    lt(
        const char&  _a,
        const char&  _b
    ) D_NOEXCEPT
    {
        return static_cast<unsigned char>(_a) < static_cast<unsigned char>(_b);
    }

    // compare
    //   function: re-expressed in terms of the unsigned-char lt above.
    static D_CONSTEXPR_CPP14 int
    compare(
        const char*  _s1,
        const char*  _s2,
        std::size_t  _count
    ) D_NOEXCEPT
    {
        for (std::size_t _i = 0; _i < _count; ++_i)
        {
            if (lt(_s1[_i], _s2[_i]))
            {
                return -1;
            }
            if (lt(_s2[_i], _s1[_i]))
            {
                return 1;
            }
        }
        return 0;
    }

    // to_int_type
    //   function: widen through unsigned char so the high bit does not
    // sign-extend (matches the standard).
    static D_CONSTEXPR int
    to_int_type(
        const char&  _c
    ) D_NOEXCEPT
    {
        return static_cast<int>(static_cast<unsigned char>(_c));
    }

    static D_CONSTEXPR int
    eof() D_NOEXCEPT
    {
        return static_cast<int>(-1);
    }

    static D_CONSTEXPR int
    not_eof(
        const int&  _i
    ) D_NOEXCEPT
    {
        return eq_int_type(_i, eof()) ? 0 : _i;
    }
};


// =============================================================================
// IV.  wchar_t SPECIALISATION  (C++98)
// =============================================================================

NS_END  // re_std

// std (fundamental types — wchar_t int_type / WEOF)
#include <cwchar>

NS_RESTD
//   Reopened 2026-08-25. The NS_END above exists so <cwchar> is
// included at global scope, but nothing reopened re_std afterwards,
// leaving char_traits<wchar_t> at global scope.


// char_traits<wchar_t>
//   class: traits for the wide character type. int_type is wint_t and
// eof() is WEOF, both from <cwchar>.
template<>
struct char_traits<wchar_t>
    : internal::char_traits_base<wchar_t, wint_t>
{
    static D_CONSTEXPR wint_t
    eof() D_NOEXCEPT
    {
        return static_cast<wint_t>(WEOF);
    }

    static D_CONSTEXPR wint_t
    not_eof(
        const wint_t&  _i
    ) D_NOEXCEPT
    {
        return eq_int_type(_i, eof()) ? static_cast<wint_t>(0) : _i;
    }
};


// =============================================================================
// V.   char16_t / char32_t SPECIALISATIONS  (C++11)
// =============================================================================

NS_END  // re_std


#if D_ENV_LANG_IS_CPP11_OR_HIGHER


// std (fundamental types — uint_least16_t / uint_least32_t)
#include <cstdint>

NS_RESTD

// char_traits<char16_t>
//   class: traits for char16_t. int_type is uint_least16_t; eof() is
// the all-ones sentinel.
template<>
struct char_traits<char16_t>
    : internal::char_traits_base<char16_t, std::uint_least16_t>
{
    static D_CONSTEXPR std::uint_least16_t
    eof() D_NOEXCEPT
    {
        return static_cast<std::uint_least16_t>(-1);
    }

    static D_CONSTEXPR std::uint_least16_t
    not_eof(
        const std::uint_least16_t&  _i
    ) D_NOEXCEPT
    {
        return eq_int_type(_i, eof())
                   ? static_cast<std::uint_least16_t>(0)
                   : _i;
    }
};

// char_traits<char32_t>
//   class: traits for char32_t. int_type is uint_least32_t; eof() is
// the all-ones sentinel.
template<>
struct char_traits<char32_t>
    : internal::char_traits_base<char32_t, std::uint_least32_t>
{
    static D_CONSTEXPR std::uint_least32_t
    eof() D_NOEXCEPT
    {
        return static_cast<std::uint_least32_t>(-1);
    }

    static D_CONSTEXPR std::uint_least32_t
    not_eof(
        const std::uint_least32_t&  _i
    ) D_NOEXCEPT
    {
        return eq_int_type(_i, eof())
                   ? static_cast<std::uint_least32_t>(0)
                   : _i;
    }
};

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


// =============================================================================
// VI.  char8_t SPECIALISATION  (C++20)
// =============================================================================

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// char_traits<char8_t>
//   class: traits for the C++20 UTF-8 character type. int_type is
// unsigned int per the standard; eof() is the all-ones sentinel.
template<>
struct char_traits<char8_t>
    : internal::char_traits_base<char8_t, unsigned int>
{
    static D_CONSTEXPR unsigned int
    eof() D_NOEXCEPT
    {
        return static_cast<unsigned int>(-1);
    }

    static D_CONSTEXPR unsigned int
    not_eof(
        const unsigned int&  _i
    ) D_NOEXCEPT
    {
        return eq_int_type(_i, eof()) ? 0u : _i;
    }
};

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER




NS_END  // re_std   (added 2026-08-25 -- was never closed)

#endif  // DJINTERP_RE_STD_STRING_VIEW_CHAR_TRAITS_
