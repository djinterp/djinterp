/******************************************************************************
* djinterp [utility]                                             color_common.h
*
*   Common foundation for the djinterp color module, shared verbatim by the
* C library and the C++ layer. Defines the language-portable qualifier
* macros that let every color-math kernel be written exactly once, plus the
* scalar helper routines (min/max/clamp/modulo) used across all color
* models.
*   This header is pure POD/C-style code. When compiled as C++ it is pulled
* into the `djinterp` namespace and its kernels become `constexpr`; when
* compiled as C it provides the same routines with `static inline` linkage
* at global scope. The matching .hpp sibling (color_common.hpp) adds the
* C++-only ergonomic aliases and traits.
*
*
* path:      /inc/djinterp/c/util/color/color_common.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.06.20
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    PORTABILITY LAYER
      ------------------
      i.    D_COLOR_NS_OPEN / D_COLOR_NS_CLOSE   (namespace bridge)
      ii.   D_COLOR_FN                           (constexpr-capable kernel)
      iii.  D_COLOR_FN_RT                        (runtime-only kernel)
      iv.   D_COLOR_LITERAL                      (aggregate construction)
II.   SHARED CONSTANTS
      -----------------
III.  SCALAR HELPERS
      --------------
      i.    d_color_max3
      ii.   d_color_min3
      iii.  d_color_clamp_01
      iv.   d_color_clamp_range
      v.    d_color_fmodf
      vi.   d_color_fabsf
*/

#ifndef  DJINTERP_C_COLOR_COMMON_
#define  DJINTERP_C_COLOR_COMMON_ 1

// djinterp
#include "../../djinterp.h"

///////////////////////////////////////////////////////////////////////////////
///                     I.   PORTABILITY LAYER                              ///
///////////////////////////////////////////////////////////////////////////////
/*   The macros below are the entire trick behind the shared kernel. Each
* color-math routine is written one time using these, and resolves to the
* right qualifier set in each language:
*
*     D_COLOR_FN     -> constexpr inline   (C++)   /  inline   (C)
*     D_COLOR_FN_RT  -> inline             (C++)   /  inline   (C)
*
* D_COLOR_FN marks math that is valid in a constant expression (pure
* arithmetic). D_COLOR_FN_RT marks math that calls <math.h> transcendentals
* (powf, cbrtf, sqrtf, trig, ...) which are not constexpr before C++23 and
* must therefore stay runtime-only on the C++ side.
*
*   The C bodies live here in the headers (so C++ and inlining C callers use
* them directly with no duplication). For a normal compiled C library, each
* module ships a .c that re-declares its prototypes; under C99 inline rules
* that emits exactly one out-of-line external definition per function, so
* non-inlined C calls and taken addresses resolve. Define D_COLOR_HEADER_ONLY
* to switch C to `static inline` and drop the .c files entirely.
*/

#ifdef __cplusplus
    #include "../../../core/djinterp.hpp"

    #define D_COLOR_NS_OPEN         NS_DJINTERP
    #define D_COLOR_NS_CLOSE        NS_END

    #define D_COLOR_FN              D_CONSTEXPR_INLINE
    #define D_COLOR_FN_RT           D_INLINE

    // aggregate construction: T{ ... }
    #define D_COLOR_LITERAL(T, ...) T{ __VA_ARGS__ }
#else
    #include "../../djinterp.h"
    #include <math.h>

    #define D_COLOR_NS_OPEN
    #define D_COLOR_NS_CLOSE

    /* C linkage model for the kernel functions (see the banner above):
     *
     *   default              - functions are `inline`; their bodies live
     *                          here in the headers (so C++ and inlining C
     *                          callers use them directly), and each module's
     *                          .c re-declares its prototypes to emit exactly
     *                          one out-of-line external definition. Build and
     *                          link the color_*.c files for a normal C lib.
     *
     *   D_COLOR_HEADER_ONLY  - functions are `static inline`; every TU gets
     *                          its own copy and no .c files are required.
     */
    #if defined(D_COLOR_HEADER_ONLY)
        #define D_COLOR_FN          static inline
        #define D_COLOR_FN_RT       static inline
    #else
        #define D_COLOR_FN          inline
        #define D_COLOR_FN_RT       inline
    #endif

    // aggregate construction: (struct T){ ... }  (C99 compound literal)
    #define D_COLOR_LITERAL(T, ...) (struct T){ __VA_ARGS__ }
#endif


D_COLOR_NS_OPEN


///////////////////////////////////////////////////////////////////////////////
///                     II.   SHARED CONSTANTS                              ///
///////////////////////////////////////////////////////////////////////////////

// D_COLOR_EPSILON
//   constant: tolerance for floating-point comparisons and
// near-zero guards throughout the color kernels.
#define D_COLOR_EPSILON 1e-6f


///////////////////////////////////////////////////////////////////////////////
///                      III.   SCALAR HELPERS                              ///
///////////////////////////////////////////////////////////////////////////////

/*
d_color_max3
  Returns the greatest of three float values.

Parameter(s):
  _a: first value.
  _b: second value.
  _c: third value.
Return:
  The maximum of _a, _b, and _c.
*/
D_COLOR_FN float
d_color_max3(
    float _a,
    float _b,
    float _c
)
{
    float max = _a;

    if (_b > max)
    {
        max = _b;
    }
    if (_c > max)
    {
        max = _c;
    }

    return max;
}

/*
d_color_min3
  Returns the least of three float values.

Parameter(s):
  _a: first value.
  _b: second value.
  _c: third value.
Return:
  The minimum of _a, _b, and _c.
*/
D_COLOR_FN float
d_color_min3(
    float _a,
    float _b,
    float _c
)
{
    float min = _a;

    if (_b < min)
    {
        min = _b;
    }
    if (_c < min)
    {
        min = _c;
    }

    return min;
}

/*
d_color_clamp_01
  Clamps a value to the closed interval [0, 1].

Parameter(s):
  _value: value to clamp.
Return:
  _value constrained to [0, 1].
*/
D_COLOR_FN float
d_color_clamp_01(
    float _value
)
{
    if (_value < 0.0f)
    {
        return 0.0f;
    }
    if (_value > 1.0f)
    {
        return 1.0f;
    }

    return _value;
}

/*
d_color_clamp_range
  Clamps a value to the closed interval [_min, _max].

Parameter(s):
  _value: value to clamp.
  _min:   lower bound.
  _max:   upper bound.
Return:
  _value constrained to [_min, _max].
*/
D_COLOR_FN float
d_color_clamp_range(
    float _value,
    float _min,
    float _max
)
{
    if (_value < _min)
    {
        return _min;
    }
    if (_value > _max)
    {
        return _max;
    }

    return _value;
}

/*
d_color_fmodf
  Truncated floating-point remainder (_x mod _y), constexpr-friendly so that
  hue arithmetic remains usable in constant expressions. Matches fmodf for
  the finite, in-range operands used by the color kernels.

Parameter(s):
  _x: dividend.
  _y: divisor (assumed non-zero).
Return:
  The remainder of _x / _y with the sign of _x.
*/
D_COLOR_FN float
d_color_fmodf(
    float _x,
    float _y
)
{
    return _x - (float)( (long long)(_x / _y) ) * _y;
}

/*
d_color_fabsf
  Absolute value of a float, constexpr-friendly.

Parameter(s):
  _x: input value.
Return:
  |_x|.
*/
D_COLOR_FN float
d_color_fabsf(
    float _x
)
{
    return (_x < 0.0f) ? -_x : _x;
}


D_COLOR_NS_CLOSE


#endif  // DJINTERP_C_COLOR_COMMON_ */
