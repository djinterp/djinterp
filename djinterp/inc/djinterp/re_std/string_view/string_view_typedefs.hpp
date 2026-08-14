/******************************************************************************
* djinterp [restd]                                     string_view_typedefs.hpp
*
* string_view alias header:
*   The standard convenience aliases of basic_string_view for the
* built-in character types: string_view (char), wstring_view (wchar_t),
* u16string_view (char16_t), u32string_view (char32_t), and
* u8string_view (char8_t).
*
*   PORTABILITY:
*   string_view / wstring_view / u16string_view / u32string_view are
* available from C++11 in restd (std introduced them in C++17).
* u8string_view follows char8_t and is gated on C++20.
*
*
* path:      /inc/djinterp/re_std/string_view/string_view_typedefs.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.04
******************************************************************************/

#ifndef DJINTERP_RESTD_STRING_VIEW_STRING_VIEW_TYPEDEFS_
#define DJINTERP_RESTD_STRING_VIEW_STRING_VIEW_TYPEDEFS_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER


// djinterp
#include "./basic_string_view.hpp"


NS_RESTD


// string_view
//   typedef: view over char.
typedef basic_string_view<char>      string_view;

// wstring_view
//   typedef: view over wchar_t.
typedef basic_string_view<wchar_t>   wstring_view;

// u16string_view
//   typedef: view over char16_t.
typedef basic_string_view<char16_t>  u16string_view;

// u32string_view
//   typedef: view over char32_t.
typedef basic_string_view<char32_t>  u32string_view;


#if D_ENV_LANG_IS_CPP20_OR_HIGHER
// u8string_view
//   typedef: view over char8_t (C++20).
typedef basic_string_view<char8_t>   u8string_view;
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_STRING_VIEW_STRING_VIEW_TYPEDEFS_
