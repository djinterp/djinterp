/*******************************************************************************
* djinterp [re_std]                                     string_view_typedefs.hpp
*
* string_view alias header:
*   The standard convenience aliases of basic_string_view for the
* built-in character types: string_view (char), wstring_view (wchar_t),
* u16string_view (char16_t), u32string_view (char32_t), and
* u8string_view (char8_t).
*
*   PORTABILITY:
*   string_view / wstring_view are available from C++98 in re_std (std
* introduced them in C++17); u16string_view / u32string_view follow
* char16_t / char32_t from C++11, and u8string_view follows char8_t
* from C++20.
*
*
* path:      /inc/re_std/string_view/string_view_typedefs.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.04
*                                                            revised: 2026.10.04
*******************************************************************************/

#ifndef RE_STD_STRING_VIEW_STRING_VIEW_TYPEDEFS_HPP
#define RE_STD_STRING_VIEW_STRING_VIEW_TYPEDEFS_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// re_std
#include "./basic_string_view.hpp"


namespace re_std
{


// string_view
//   typedef: view over char.
typedef basic_string_view<char>      string_view;

// wstring_view
//   typedef: view over wchar_t.
typedef basic_string_view<wchar_t>   wstring_view;

#if RE_STD_LANG_IS_CPP11_OR_HIGHER
// u16string_view
//   typedef: view over char16_t (C++11: the type is C++11's).
typedef basic_string_view<char16_t>  u16string_view;

// u32string_view
//   typedef: view over char32_t (C++11: the type is C++11's).
typedef basic_string_view<char32_t>  u32string_view;
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#if RE_STD_LANG_IS_CPP20_OR_HIGHER
// u8string_view
//   typedef: view over char8_t (C++20).
typedef basic_string_view<char8_t>   u8string_view;
#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER


}  // re_std


#endif  // RE_STD_STRING_VIEW_STRING_VIEW_TYPEDEFS_HPP
