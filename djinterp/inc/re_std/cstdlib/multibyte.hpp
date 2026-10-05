/*******************************************************************************
* djinterp [re_std]                                                multibyte.hpp
*
* the multibyte conversions (re-exports):
*   mblen / mbtowc / wctomb / mbstowcs / wcstombs. Re-exported because
* every one of them reads the current C locale, which is process-global
* runtime state that no library-side reimplementation could consult.
*
*   THESE FIVE CARRY HIDDEN CONVERSION STATE:
*   mblen, mbtowc and wctomb keep an internal shift state, which makes
* them non-reentrant and unsafe to call from two threads at once even on
* different strings. The <cwchar> restartable forms (mbrlen, mbrtowc,
* wcrtomb) take an explicit mbstate_t instead and should be preferred in
* anything threaded. <cwchar> is catalogued at priority 129 and not yet
* implemented.
*
*   WHAT "MULTIBYTE" MEANS HERE IS LOCALE-DEPENDENT:
*   Not UTF-8 unless the active locale says so. In the default "C" locale
* the encoding is typically single-byte, so a UTF-8 string passed through
* these functions is mangled rather than converted. Setting the locale is
* the caller's job -- and setlocale lives in <clocale>, priority 117.
*
*   MB_CUR_MAX is a macro (and on most runtimes not even a constant --
* it expands to a function call reading the current locale), so it has no
* re_std:: spelling. Including this header makes it available as
* <cstdlib> does. Note it is the CURRENT locale's maximum, where
* <climits>'s MB_LEN_MAX is the maximum across all locales; buffers sized
* for conversion want the latter.
*
*
* path:      /inc/re_std/cstdlib/multibyte.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CSTDLIB_MULTIBYTE_HPP
#define RE_STD_CSTDLIB_MULTIBYTE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstdlib>


namespace re_std
{

    // mblen
    //   function: length in bytes of the next multibyte character.
    using ::std::mblen;

    // mbtowc
    //   function: one multibyte character to one wide character.
    using ::std::mbtowc;

    // wctomb
    //   function: one wide character to a multibyte sequence.
    using ::std::wctomb;

    // mbstowcs
    //   function: a whole multibyte string to wide characters.
    using ::std::mbstowcs;

    // wcstombs
    //   function: a whole wide string to a multibyte sequence.
    using ::std::wcstombs;

}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_CSTDLIB_MULTIBYTE_HPP
