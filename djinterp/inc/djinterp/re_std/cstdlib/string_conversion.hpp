/******************************************************************************
* djinterp [re_std]                                       string_conversion.hpp
*
* the string-to-number conversions (re-exports):
*   atof / atoi / atol / atoll and the strto* family, surfaced in re_std::
* so a codebase can keep one prefix.
*
*   THESE ARE RUNTIME FUNCTIONS, NOT LIBRARY CODE:
*   strtod's result depends on the C locale's decimal point, on the
* platform's floating-point parsing and on errno -- state that lives in
* the C runtime. A reimplementation would be a second parser with
* different rounding on the last digit, which is exactly the class of
* silent divergence re_std exists to avoid. So they are re-exported:
* re_std::strtod IS std::strtod.
*
*   PREFER THE strto* FAMILY TO THE ato* FAMILY:
*   atoi and friends have no way to report failure -- atoi("hello") is 0,
* indistinguishable from atoi("0"), and out-of-range input is undefined
* behaviour rather than a diagnosable error. strtol sets errno to ERANGE
* and hands back an end pointer. The ato* family is surfaced for
* completeness and for porting existing code, not as a recommendation.
* From C++17 std::from_chars is better still; it is catalogued under
* <charconv> and not yet implemented.
*
*   C++11 FLOOR: atoll, strtof, strtold, strtoll and strtoull are C++11
* additions; the rest are C++98. The module floors at C++11 regardless,
* so all eleven are surfaced unconditionally.
*
*
* path:      /inc/djinterp/re_std/cstdlib/string_conversion.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CSTDLIB_STRING_CONVERSION_
#define DJINTERP_RE_STD_CSTDLIB_STRING_CONVERSION_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstdlib>


NS_RESTD

    // atof / atoi / atol / atoll
    //   function: parse a numeric prefix, with no error reporting. See the
    // header comment before reaching for these.
    using ::std::atof;
    using ::std::atoi;
    using ::std::atol;
    using ::std::atoll;

    // strtod / strtof / strtold
    //   function: parse a floating-point value, reporting the end position
    // and setting errno to ERANGE on overflow or underflow.
    using ::std::strtod;
    using ::std::strtof;
    using ::std::strtold;

    // strtol / strtoll / strtoul / strtoull
    //   function: parse an integer in the given base (0 auto-detects the
    // 0x / 0 prefixes), reporting the end position and range errors.
    using ::std::strtol;
    using ::std::strtoll;
    using ::std::strtoul;
    using ::std::strtoull;

NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_CSTDLIB_STRING_CONVERSION_
