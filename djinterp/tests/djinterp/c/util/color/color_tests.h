/*******************************************************************************
* djinterp [c][utility]                                            color_tests.h
*
* The color module's C suite in DTest's C face (c/modules/test/test_suite.h):
* color.h's CIEDE2000 difference, d_color_delta_e and d_color_rgb_delta_e,
* checked against the published reference pairs.
*
*
* path:      /tests/djinterp/c/util/color/color_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.17
*                                                            revised: 2026.11.17
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FIXTURE
    -------
    1.  Tolerance
         1.  d_tests_color_de_near
2.  TESTS
    -----
    1.  Perceptual difference
3.  SPEC
    ----
    1.  The provider
*/

#ifndef DJINTERP_TESTS_C_UTIL_COLOR_TESTS_H
#define DJINTERP_TESTS_C_UTIL_COLOR_TESTS_H 1

// std
#include <float.h>  // FLT_EPSILON
#include <math.h>   // fabs
// djinterp
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*
// tested
#include "../../../../../inc/djinterp/c/util/color/color.h"  // d_color_delta_e


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  FIXTURE
//==============================================================================


// 1.1    Tolerance
//------------------------------------------------------------------------------
// 1.1.1
// d_tests_color_de_near
//   function: whether _got is within 1e-4 of _want plus 2e-5 of _want. The
// first term covers the reference values' rounding to four decimal places
// (5e-5 at most); the second covers single precision, whose error against a
// double-precision evaluation of the same formula is under 9e-6 of the
// value. Both are well below any real fault: the one this suite was written
// to catch moved a reference value by 5e-4 and another by 3.1.
D_STATIC_INLINE
bool
d_tests_color_de_near(
    float  _got,
    double _want
)
{
    return fabs((double)_got - _want) <= 1e-4 + 2e-5 * fabs(_want);
}


//==============================================================================
// 2.  TESTS
//==============================================================================


// 2.1    Perceptual difference
//------------------------------------------------------------------------------
bool
d_tests_color_delta_e_reference(void);

bool
d_tests_color_delta_e_symmetric(void);

bool
d_tests_color_delta_e_identity(void);

bool
d_tests_color_delta_e_lightness(void);

bool
d_tests_color_delta_e_rotation(void);

bool
d_tests_color_rgb_delta_e(void);


//==============================================================================
// 3.  SPEC
//==============================================================================


// 3.1    The provider
//------------------------------------------------------------------------------
#ifdef DTEST_SPEC_MODE

// d_tests_color_delta_e_rows
//   constant: the perceptual-difference block's rows.
static const struct dj_test_case_spec d_tests_color_delta_e_rows[] =
{
    { "d_tests_color_delta_e_reference",
      "Sharma, Wu and Dalal's 34 pairs, each within the data's precision",
      d_tests_color_delta_e_reference },
    { "d_tests_color_delta_e_symmetric",
      "swapping the two colors leaves the difference unchanged",
      d_tests_color_delta_e_symmetric },
    { "d_tests_color_delta_e_identity",
      "a color differs from itself by exactly 0, achromatic ones included",
      d_tests_color_delta_e_identity },
    { "d_tests_color_delta_e_lightness",
      "grays centred on L* = 50 differ by exactly their lightness gap",
      d_tests_color_delta_e_lightness },
    { "d_tests_color_delta_e_rotation",
      "the rotation term is weighted by the adjusted chromas' mean",
      d_tests_color_delta_e_rotation },
    { "d_tests_color_rgb_delta_e",
      "the RGB form is the L*a*b* form after conversion",
      d_tests_color_rgb_delta_e }
};

// d_tests_color_blocks
//   constant: the suite's blocks.
static const struct dj_test_block_spec d_tests_color_blocks[] =
{
    { "delta_e",
      "color.h's CIEDE2000 difference",
      d_tests_color_delta_e_rows,
      sizeof(d_tests_color_delta_e_rows) /
          sizeof(d_tests_color_delta_e_rows[0]) }
};

// d_tests_color_module
//   constant: the module spec.
static const struct dj_test_module_spec d_tests_color_module =
{
    "color",
    "c/util/color: the CIEDE2000 color difference",
    d_tests_color_blocks,
    sizeof(d_tests_color_blocks) / sizeof(d_tests_color_blocks[0])
};

// d_tests_color_spec
//   function: the suite's provider.
D_STATIC_INLINE
const struct dj_test_module_spec*
d_tests_color_spec(void)
{
    return &d_tests_color_module;
}

#endif  // DTEST_SPEC_MODE


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_UTIL_COLOR_TESTS_H
