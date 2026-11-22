/*******************************************************************************
* djinterp [c][utility]                                    color_tests_delta_e.c
*
* The color suite, block "delta_e": color.h's CIEDE2000 difference, checked
* against the reference pairs its authors published for exactly this
* purpose, and on the properties the formula guarantees.
*
*
* path:      /tests/djinterp/c/util/color/color_tests_delta_e.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.17
*                                                            revised: 2026.11.17
*******************************************************************************/
#include "./color_tests.h"  // corresponding header


// d_tests_color_de_pair
//   struct: one reference pair and its published difference.
struct d_tests_color_de_pair
{
    struct dj_color_lab first;
    struct dj_color_lab second;
    double              delta_e;
};

// d_tests_color_de_pairs
//   constant: the 34 test pairs of G. Sharma, W. Wu and E. N. Dalal, "The
// CIEDE2000 Color-Difference Formula: Implementation Notes, Supplementary
// Test Data, and Mathematical Observations", Color Research and Application
// 30(1), 21-30, 2005 (the authors' ciede2000testdata.txt). They exercise
// the hue-angle wrap, achromatic colors, the mean-hue cases and the rotation
// term, which the standard's own worked examples do not.
static const struct d_tests_color_de_pair d_tests_color_de_pairs[] =
{
    /*  1 */ { {  50.0000f,   2.6772f, -79.7751f },
              {  50.0000f,   0.0000f, -82.7485f },
              2.0425 },
    /*  2 */ { {  50.0000f,   3.1571f, -77.2803f },
              {  50.0000f,   0.0000f, -82.7485f },
              2.8615 },
    /*  3 */ { {  50.0000f,   2.8361f, -74.0200f },
              {  50.0000f,   0.0000f, -82.7485f },
              3.4412 },
    /*  4 */ { {  50.0000f,  -1.3802f, -84.2814f },
              {  50.0000f,   0.0000f, -82.7485f },
              1.0000 },
    /*  5 */ { {  50.0000f,  -1.1848f, -84.8006f },
              {  50.0000f,   0.0000f, -82.7485f },
              1.0000 },
    /*  6 */ { {  50.0000f,  -0.9009f, -85.5211f },
              {  50.0000f,   0.0000f, -82.7485f },
              1.0000 },
    /*  7 */ { {  50.0000f,   0.0000f,   0.0000f },
              {  50.0000f,  -1.0000f,   2.0000f },
              2.3669 },
    /*  8 */ { {  50.0000f,  -1.0000f,   2.0000f },
              {  50.0000f,   0.0000f,   0.0000f },
              2.3669 },
    /*  9 */ { {  50.0000f,   2.4900f,  -0.0010f },
              {  50.0000f,  -2.4900f,   0.0009f },
              7.1792 },
    /* 10 */ { {  50.0000f,   2.4900f,  -0.0010f },
              {  50.0000f,  -2.4900f,   0.0010f },
              7.1792 },
    /* 11 */ { {  50.0000f,   2.4900f,  -0.0010f },
              {  50.0000f,  -2.4900f,   0.0011f },
              7.2195 },
    /* 12 */ { {  50.0000f,   2.4900f,  -0.0010f },
              {  50.0000f,  -2.4900f,   0.0012f },
              7.2195 },
    /* 13 */ { {  50.0000f,  -0.0010f,   2.4900f },
              {  50.0000f,   0.0009f,  -2.4900f },
              4.8045 },
    /* 14 */ { {  50.0000f,  -0.0010f,   2.4900f },
              {  50.0000f,   0.0010f,  -2.4900f },
              4.8045 },
    /* 15 */ { {  50.0000f,  -0.0010f,   2.4900f },
              {  50.0000f,   0.0011f,  -2.4900f },
              4.7461 },
    /* 16 */ { {  50.0000f,   2.5000f,   0.0000f },
              {  50.0000f,   0.0000f,  -2.5000f },
              4.3065 },
    /* 17 */ { {  50.0000f,   2.5000f,   0.0000f },
              {  73.0000f,  25.0000f, -18.0000f },
              27.1492 },
    /* 18 */ { {  50.0000f,   2.5000f,   0.0000f },
              {  61.0000f,  -5.0000f,  29.0000f },
              22.8977 },
    /* 19 */ { {  50.0000f,   2.5000f,   0.0000f },
              {  56.0000f, -27.0000f,  -3.0000f },
              31.9030 },
    /* 20 */ { {  50.0000f,   2.5000f,   0.0000f },
              {  58.0000f,  24.0000f,  15.0000f },
              19.4535 },
    /* 21 */ { {  50.0000f,   2.5000f,   0.0000f },
              {  50.0000f,   3.1736f,   0.5854f },
              1.0000 },
    /* 22 */ { {  50.0000f,   2.5000f,   0.0000f },
              {  50.0000f,   3.2972f,   0.0000f },
              1.0000 },
    /* 23 */ { {  50.0000f,   2.5000f,   0.0000f },
              {  50.0000f,   1.8634f,   0.5757f },
              1.0000 },
    /* 24 */ { {  50.0000f,   2.5000f,   0.0000f },
              {  50.0000f,   3.2592f,   0.3350f },
              1.0000 },
    /* 25 */ { {  60.2574f, -34.0099f,  36.2677f },
              {  60.4626f, -34.1751f,  39.4387f },
              1.2644 },
    /* 26 */ { {  63.0109f, -31.0961f,  -5.8663f },
              {  62.8187f, -29.7946f,  -4.0864f },
              1.2630 },
    /* 27 */ { {  61.2901f,   3.7196f,  -5.3901f },
              {  61.4292f,   2.2480f,  -4.9620f },
              1.8731 },
    /* 28 */ { {  35.0831f, -44.1164f,   3.7933f },
              {  35.0232f, -40.0716f,   1.5901f },
              1.8645 },
    /* 29 */ { {  22.7233f,  20.0904f, -46.6940f },
              {  23.0331f,  14.9730f, -42.5619f },
              2.0373 },
    /* 30 */ { {  36.4612f,  47.8580f,  18.3852f },
              {  36.2715f,  50.5065f,  21.2231f },
              1.4146 },
    /* 31 */ { {  90.8027f,  -2.0831f,   1.4410f },
              {  91.1528f,  -1.6435f,   0.0447f },
              1.4441 },
    /* 32 */ { {  90.9257f,  -0.5406f,  -0.9208f },
              {  88.6381f,  -0.8985f,  -0.7239f },
              1.5381 },
    /* 33 */ { {   6.7747f,  -0.2908f,  -2.4247f },
              {   5.8714f,  -0.0985f,  -2.2286f },
              0.6377 },
    /* 34 */ { {   2.0776f,   0.0795f,  -1.1350f },
              {   0.9033f,  -0.0636f,  -0.5514f },
              0.9082 }
};

// D_TESTS_COLOR_DE_PAIR_COUNT
//   constant: the number of reference pairs.
#define D_TESTS_COLOR_DE_PAIR_COUNT                                           \
    (sizeof(d_tests_color_de_pairs) / sizeof(d_tests_color_de_pairs[0]))


/*
d_tests_color_delta_e_reference
  The published values.
  Tests the following:
  - every one of the 34 pairs is within the data's precision of its
    published difference; a pair that is not is named in a note, and the
    rest are still checked
*/
bool
d_tests_color_delta_e_reference(void)
{
    size_t i  = 0;
    bool   ok = true;

    D_TEST_REQUIRE(D_TESTS_COLOR_DE_PAIR_COUNT == 34);

    for (i = 0; i < D_TESTS_COLOR_DE_PAIR_COUNT; ++i)
    {
        const struct d_tests_color_de_pair* pair = &d_tests_color_de_pairs[i];
        const float got = d_color_delta_e(pair->first, pair->second);

        if (!D_TEST_EXPECT(d_tests_color_de_near(got, pair->delta_e)))
        {
            d_test_suite_note_current("pair %u: %.4f, not %.4f",
                                      (unsigned)(i + 1),
                                      (double)got,
                                      pair->delta_e);
            ok = false;
        }
    }

    return ok;
}

/*
d_tests_color_delta_e_symmetric
  The order of the arguments.
  Tests the following:
  - every reference pair gives the same difference either way round, to
    within four units in the last place of a float
*/
bool
d_tests_color_delta_e_symmetric(void)
{
    size_t i  = 0;
    bool   ok = true;

    for (i = 0; i < D_TESTS_COLOR_DE_PAIR_COUNT; ++i)
    {
        const struct d_tests_color_de_pair* pair = &d_tests_color_de_pairs[i];
        const float forward = d_color_delta_e(pair->first, pair->second);
        const float back    = d_color_delta_e(pair->second, pair->first);

        if (!D_TEST_EXPECT(fabs((double)forward - (double)back) <=
                               4.0 * FLT_EPSILON * (double)forward))
        {
            d_test_suite_note_current("pair %u: %.7f one way, %.7f the other",
                                      (unsigned)(i + 1),
                                      (double)forward,
                                      (double)back);
            ok = false;
        }
    }

    return ok;
}

/*
d_tests_color_delta_e_identity
  A color against itself.
  Tests the following:
  - every reference color differs from itself by exactly 0
  - so do black, white and a gray, whose hue is undefined
*/
bool
d_tests_color_delta_e_identity(void)
{
    const struct dj_color_lab grays[3] =
    {
        { 0.0f,   0.0f, 0.0f },
        { 100.0f, 0.0f, 0.0f },
        { 50.0f,  0.0f, 0.0f }
    };
    size_t                    i     = 0;

    for (i = 0; i < D_TESTS_COLOR_DE_PAIR_COUNT; ++i)
    {
        const struct d_tests_color_de_pair* pair = &d_tests_color_de_pairs[i];

        D_TEST_REQUIRE(d_color_delta_e(pair->first, pair->first) == 0.0f);
        D_TEST_REQUIRE(d_color_delta_e(pair->second, pair->second) == 0.0f);
    }

    for (i = 0; i < 3; ++i)
    {
        D_TEST_REQUIRE(d_color_delta_e(grays[i], grays[i]) == 0.0f);
    }

    return true;
}

/*
d_tests_color_delta_e_lightness
  Grays, where only lightness differs.
  Tests the following:
  - black and white differ by exactly 100: their mean lightness is 50,
    where the lightness weighting S_L is exactly 1
  - so do any two grays centred on 50, by their lightness gap
  - grays centred elsewhere differ by less than their gap, S_L being
    greater than 1 there
*/
bool
d_tests_color_delta_e_lightness(void)
{
    const struct dj_color_lab black = { 0.0f,   0.0f, 0.0f };
    const struct dj_color_lab white = { 100.0f, 0.0f, 0.0f };
    const struct dj_color_lab dark  = { 40.0f,  0.0f, 0.0f };
    const struct dj_color_lab light = { 60.0f,  0.0f, 0.0f };
    const struct dj_color_lab pale  = { 80.0f,  0.0f, 0.0f };

    D_TEST_REQUIRE(d_color_delta_e(black, white) == 100.0f);
    D_TEST_REQUIRE(d_color_delta_e(dark, light) == 20.0f);
    D_TEST_REQUIRE(d_color_delta_e(light, pale) < 20.0f);
    D_TEST_REQUIRE(d_color_delta_e(light, pale) > 0.0f);

    return true;
}

/*
d_tests_color_delta_e_rotation
  The rotation term's chroma weighting, R_C.
  Tests the following:
  - reference pair 19, near the blue region where the rotation term is
    largest, and with chromas small enough that G scales a* by about half,
    so that the mean of the adjusted chromas and of the unadjusted ones
    give R_C values far apart: 31.9030 with the adjusted mean, as the
    formula specifies, and 28.7560 with the other
  - pair 27, where the same mistake moves the result by 5e-4
*/
bool
d_tests_color_delta_e_rotation(void)
{
    const struct dj_color_lab p19a = { 50.0000f,   2.5000f,  0.0000f };
    const struct dj_color_lab p19b = { 56.0000f, -27.0000f, -3.0000f };
    const struct dj_color_lab p27a = { 61.2901f,   3.7196f, -5.3901f };
    const struct dj_color_lab p27b = { 61.4292f,   2.2480f, -4.9620f };

    D_TEST_REQUIRE(d_tests_color_de_near(d_color_delta_e(p19a, p19b),
                                         31.9030));
    D_TEST_REQUIRE(d_tests_color_de_near(d_color_delta_e(p27a, p27b),
                                         1.8731));

    return true;
}

/*
d_tests_color_rgb_delta_e
  The RGB convenience form.
  Tests the following:
  - it is d_color_delta_e on the two colors converted to L*a*b*
  - a color differs from itself by 0, and black from white by 100
*/
bool
d_tests_color_rgb_delta_e(void)
{
    const struct dj_color_rgb black = { 0.0f,  0.0f,  0.0f  };
    const struct dj_color_rgb white = { 1.0f,  1.0f,  1.0f  };
    const struct dj_color_rgb red   = { 1.0f,  0.0f,  0.0f  };
    const struct dj_color_rgb teal  = { 0.0f,  0.5f,  0.5f  };
    const float               via   =
        d_color_delta_e(d_color_convert_rgb_to_lab(red),
                        d_color_convert_rgb_to_lab(teal));

    D_TEST_REQUIRE(fabs((double)d_color_rgb_delta_e(red, teal) -
                        (double)via) <= 1e-5 * (double)via);
    D_TEST_REQUIRE(d_color_rgb_delta_e(red, red) == 0.0f);
    D_TEST_REQUIRE(d_tests_color_de_near(d_color_rgb_delta_e(black, white),
                                         100.0));

    return true;
}
