/******************************************************************************
* djinterp [color]                                          color_convert.hpp
*
*   Compile-time and runtime conversion between color models. Uses a
* trait-based dispatch pattern where each source-to-destination pair is
* a specialization of the internal color_convert_impl trait. New model
* pairs can be added by providing additional specializations without
* modifying existing code.
*
*   Conversions involving CIE L*a*b* route through CIE XYZ as an
* intermediate space and require the non-constexpr functions cbrt/pow
* from <cmath>. All other conversions are fully constexpr.
*
* 
* path:      /inc/djinterp/util/color/color_convert.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.04.12
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    CONVERSION INFRASTRUCTURE
      --------------------------
      i.    color_convert_impl (internal, primary template)

II.   RGB <-> HSL CONVERSIONS
      -------------------------
      i.    color_convert_impl<rgb, hsl>
      ii.   color_convert_impl<hsl, rgb>

III.  RGB <-> HSV CONVERSIONS
      -------------------------
      i.    color_convert_impl<rgb, hsv>
      ii.   color_convert_impl<hsv, rgb>

IV.   HSL <-> HSV CONVERSIONS
      -------------------------
      i.    color_convert_impl<hsl, hsv>
      ii.   color_convert_impl<hsv, hsl>

V.    RGB <-> CMYK CONVERSIONS
      --------------------------
      i.    color_convert_impl<rgb, cmyk>
      ii.   color_convert_impl<cmyk, rgb>

VI.   RGB <-> YCBCR CONVERSIONS (BT.601)
      ------------------------------------
      i.    color_convert_impl<rgb, ycbcr>
      ii.   color_convert_impl<ycbcr, rgb>

VII.  RGB <-> CIE XYZ <-> CIE LAB CONVERSIONS
      ------------------------------------------
      i.    linearize_srgb (internal)
      ii.   delinearize_srgb (internal)
      iii.  lab_f (internal)
      iv.   lab_f_inv (internal)
      v.    rgb_to_xyz (internal)
      vi.   xyz_to_rgb (internal)
      vii.  xyz_to_lab (internal)
      viii. lab_to_xyz (internal)
      ix.   color_convert_impl<rgb, cie_lab>
      x.    color_convert_impl<cie_lab, rgb>

VIII. PUBLIC INTERFACE
      -----------------
      i.    color_convert
      ii.   color_cast
      iii.  is_convertible_color
            a. has_color_convert_helper (internal)
            b. is_convertible_color
            c. is_convertible_color_v
*/

#ifndef DJINTERP_COLOR_CONVERT_
#define DJINTERP_COLOR_CONVERT_ 1

#include "../../djinterp.hpp"
#include "./color.hpp"
#include <cmath>


NS_DJINTERP
NS_COLOR


///////////////////////////////////////////////////////////////////////////////
///             I.   CONVERSION INFRASTRUCTURE                              ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // color_convert_impl
    //   trait: primary template for color model conversion.
    // Must be specialized for each valid source-destination
    // pair. Unspecialized instantiation is intentionally
    // incomplete (will produce a compile error for unsupported
    // conversions).
    template<typename _From,
             typename _To,
             typename = void>
    struct color_convert_impl;


///////////////////////////////////////////////////////////////////////////////
///           II.   RGB <-> HSL CONVERSIONS                                 ///
///////////////////////////////////////////////////////////////////////////////

    // color_convert_impl<rgb, hsl>
    //   trait: converts RGB to HSL.
    template<>
    struct color_convert_impl<rgb, hsl>
    {
        D_STATIC_CONSTEXPR_INLINE hsl
        apply(
            const rgb& _src
        )
        {
            channel_t max_c = _src.r;
            channel_t min_c = _src.r;

            if (_src.g > max_c) max_c = _src.g;
            if (_src.b > max_c) max_c = _src.b;
            if (_src.g < min_c) min_c = _src.g;
            if (_src.b < min_c) min_c = _src.b;

            channel_t delta = max_c - min_c;
            channel_t l     = (max_c + min_c) / channel_t(2);
            channel_t s     = channel_t(0);
            channel_t h     = channel_t(0);

            if (delta != channel_t(0))
            {
                s = (l > channel_t(0.5))
                  ? delta / (channel_t(2) - max_c - min_c)
                  : delta / (max_c + min_c);

                if (max_c == _src.r)
                {
                    h = (_src.g - _src.b) / delta;
                }
                else if (max_c == _src.g)
                {
                    h = channel_t(2)
                      + (_src.b - _src.r) / delta;
                }
                else
                {
                    h = channel_t(4)
                      + (_src.r - _src.g) / delta;
                }

                h *= channel_t(60);

                if (h < channel_t(0))
                {
                    h += channel_t(360);
                }
            }

            return hsl(h, s, l, _src.a);
        }
    };

    // color_convert_impl<hsl, rgb>
    //   trait: converts HSL to RGB.
    template<>
    struct color_convert_impl<hsl, rgb>
    {
        D_STATIC_CONSTEXPR_INLINE channel_t
        hue_to_rgb(
            channel_t _p,
            channel_t _q,
            channel_t _t
        )
        {
            if (_t < channel_t(0))   _t += channel_t(1);
            if (_t > channel_t(1))   _t -= channel_t(1);

            if (_t < channel_t(1) / channel_t(6))
            {
                return _p + (_q - _p) * channel_t(6) * _t;
            }

            if (_t < channel_t(1) / channel_t(2))
            {
                return _q;
            }

            if (_t < channel_t(2) / channel_t(3))
            {
                return _p + (_q - _p)
                     * (channel_t(2) / channel_t(3) - _t)
                     * channel_t(6);
            }

            return _p;
        }

        D_STATIC_CONSTEXPR_INLINE rgb
        apply(
            const hsl& _src
        )
        {
            if (_src.s == channel_t(0))
            {
                return rgb(_src.l, _src.l, _src.l, _src.a);
            }

            channel_t q = (_src.l < channel_t(0.5))
                ? _src.l * (channel_t(1) + _src.s)
                : _src.l + _src.s
                  - _src.l * _src.s;

            channel_t p = channel_t(2) * _src.l - q;
            channel_t h_norm = _src.h / channel_t(360);

            return rgb(
                hue_to_rgb(p, q,
                           h_norm + channel_t(1) / channel_t(3)),
                hue_to_rgb(p, q,
                           h_norm),
                hue_to_rgb(p, q,
                           h_norm - channel_t(1) / channel_t(3)),
                _src.a
            );
        }
    };


///////////////////////////////////////////////////////////////////////////////
///          III.   RGB <-> HSV CONVERSIONS                                 ///
///////////////////////////////////////////////////////////////////////////////

    // color_convert_impl<rgb, hsv>
    //   trait: converts RGB to HSV.
    template<>
    struct color_convert_impl<rgb, hsv>
    {
        D_STATIC_CONSTEXPR_INLINE hsv
        apply(
            const rgb& _src
        )
        {
            channel_t max_c = _src.r;
            channel_t min_c = _src.r;

            if (_src.g > max_c) max_c = _src.g;
            if (_src.b > max_c) max_c = _src.b;
            if (_src.g < min_c) min_c = _src.g;
            if (_src.b < min_c) min_c = _src.b;

            channel_t delta = max_c - min_c;
            channel_t v     = max_c;
            channel_t s     = channel_t(0);
            channel_t h     = channel_t(0);

            if (max_c != channel_t(0))
            {
                s = delta / max_c;
            }

            if (delta != channel_t(0))
            {
                if (max_c == _src.r)
                {
                    h = (_src.g - _src.b) / delta;
                }
                else if (max_c == _src.g)
                {
                    h = channel_t(2)
                      + (_src.b - _src.r) / delta;
                }
                else
                {
                    h = channel_t(4)
                      + (_src.r - _src.g) / delta;
                }

                h *= channel_t(60);

                if (h < channel_t(0))
                {
                    h += channel_t(360);
                }
            }

            return hsv(h, s, v, _src.a);
        }
    };

    // color_convert_impl<hsv, rgb>
    //   trait: converts HSV to RGB.
    template<>
    struct color_convert_impl<hsv, rgb>
    {
        D_STATIC_CONSTEXPR_INLINE rgb
        apply(
            const hsv& _src
        )
        {
            if (_src.s == channel_t(0))
            {
                return rgb(_src.v, _src.v, _src.v, _src.a);
            }

            channel_t hh = _src.h / channel_t(60);
            int       i  = static_cast<int>(hh);
            channel_t ff = hh - static_cast<channel_t>(i);

            channel_t p = _src.v * (channel_t(1) - _src.s);
            channel_t q = _src.v * (channel_t(1) - _src.s * ff);
            channel_t t = _src.v
                        * (channel_t(1)
                           - _src.s * (channel_t(1) - ff));

            switch (i % 6)
            {
                case 0:  return rgb(_src.v, t,      p,      _src.a);
                case 1:  return rgb(q,      _src.v, p,      _src.a);
                case 2:  return rgb(p,      _src.v, t,      _src.a);
                case 3:  return rgb(p,      q,      _src.v, _src.a);
                case 4:  return rgb(t,      p,      _src.v, _src.a);
                default: return rgb(_src.v, p,      q,      _src.a);
            }
        }
    };


///////////////////////////////////////////////////////////////////////////////
///           IV.   HSL <-> HSV CONVERSIONS                                 ///
///////////////////////////////////////////////////////////////////////////////

    // color_convert_impl<hsl, hsv>
    //   trait: converts HSL to HSV via RGB intermediate.
    template<>
    struct color_convert_impl<hsl, hsv>
    {
        D_STATIC_CONSTEXPR_INLINE hsv
        apply(
            const hsl& _src
        )
        {
            rgb intermediate = color_convert_impl<hsl, rgb>::apply(_src);

            return color_convert_impl<rgb, hsv>::apply(intermediate);
        }
    };

    // color_convert_impl<hsv, hsl>
    //   trait: converts HSV to HSL via RGB intermediate.
    template<>
    struct color_convert_impl<hsv, hsl>
    {
        D_STATIC_CONSTEXPR_INLINE hsl
        apply(
            const hsv& _src
        )
        {
            rgb intermediate = color_convert_impl<hsv, rgb>::apply(_src);

            return color_convert_impl<rgb, hsl>::apply(intermediate);
        }
    };


///////////////////////////////////////////////////////////////////////////////
///            V.   RGB <-> CMYK CONVERSIONS                                ///
///////////////////////////////////////////////////////////////////////////////

    // color_convert_impl<rgb, cmyk>
    //   trait: converts RGB to CMYK using the standard
    // max-component key extraction.
    template<>
    struct color_convert_impl<rgb, cmyk>
    {
        D_STATIC_CONSTEXPR_INLINE cmyk
        apply(
            const rgb& _src
        )
        {
            channel_t k = channel_t(1) - _src.r;

            if ((channel_t(1) - _src.g) < k)
            {
                k = channel_t(1) - _src.g;
            }

            if ((channel_t(1) - _src.b) < k)
            {
                k = channel_t(1) - _src.b;
            }

            // pure black
            if (k == channel_t(1))
            {
                return cmyk(channel_t(0),
                            channel_t(0),
                            channel_t(0),
                            channel_t(1));
            }

            channel_t inv_k = channel_t(1) - k;

            return cmyk(
                (channel_t(1) - _src.r - k) / inv_k,
                (channel_t(1) - _src.g - k) / inv_k,
                (channel_t(1) - _src.b - k) / inv_k,
                k
            );
        }
    };

    // color_convert_impl<cmyk, rgb>
    //   trait: converts CMYK to RGB.
    template<>
    struct color_convert_impl<cmyk, rgb>
    {
        D_STATIC_CONSTEXPR_INLINE rgb
        apply(
            const cmyk& _src
        )
        {
            channel_t inv_k = channel_t(1) - _src.k;

            return rgb(
                (channel_t(1) - _src.c) * inv_k,
                (channel_t(1) - _src.m) * inv_k,
                (channel_t(1) - _src.y) * inv_k
            );
        }
    };


///////////////////////////////////////////////////////////////////////////////
///          VI.   RGB <-> YCBCR CONVERSIONS (BT.601)                       ///
///////////////////////////////////////////////////////////////////////////////

    // color_convert_impl<rgb, ycbcr>
    //   trait: converts RGB to YCbCr using ITU-R BT.601
    // coefficients.
    template<>
    struct color_convert_impl<rgb, ycbcr>
    {
        D_STATIC_CONSTEXPR_INLINE ycbcr
        apply(
            const rgb& _src
        )
        {
            // BT.601 luma coefficients
            D_CONSTEXPR channel_t kr = channel_t(0.299);
            D_CONSTEXPR channel_t kg = channel_t(0.587);
            D_CONSTEXPR channel_t kb = channel_t(0.114);

            channel_t y_val = kr * _src.r
                            + kg * _src.g
                            + kb * _src.b;

            channel_t cb = ((_src.b - y_val)
                         / (channel_t(2) * (channel_t(1) - kb)));

            channel_t cr = ((_src.r - y_val)
                         / (channel_t(2) * (channel_t(1) - kr)));

            return ycbcr(y_val, cb, cr);
        }
    };

    // color_convert_impl<ycbcr, rgb>
    //   trait: converts YCbCr to RGB using ITU-R BT.601
    // coefficients.
    template<>
    struct color_convert_impl<ycbcr, rgb>
    {
        D_STATIC_CONSTEXPR_INLINE rgb
        apply(
            const ycbcr& _src
        )
        {
            D_CONSTEXPR channel_t kr = channel_t(0.299);
            D_CONSTEXPR channel_t kg = channel_t(0.587);
            D_CONSTEXPR channel_t kb = channel_t(0.114);

            channel_t r = _src.y
                        + channel_t(2) * (channel_t(1) - kr)
                        * _src.cr;

            channel_t b = _src.y
                        + channel_t(2) * (channel_t(1) - kb)
                        * _src.cb;

            channel_t g = (_src.y - kr * r - kb * b) / kg;

            return rgb(r, g, b);
        }
    };


///////////////////////////////////////////////////////////////////////////////
///      VII.   RGB <-> CIE XYZ <-> CIE LAB CONVERSIONS                    ///
///////////////////////////////////////////////////////////////////////////////

    // linearize_srgb
    //   function: applies the sRGB inverse companding function,
    // converting a gamma-encoded channel to linear light.
    // NOTE: std::pow is not constexpr in standard C++; this
    // function is therefore not constexpr.
    D_STATIC D_INLINE channel_t
    linearize_srgb(
        channel_t _c
    )
    {
        return (_c <= channel_t(0.04045))
             ? _c / channel_t(12.92)
             : std::pow((_c + channel_t(0.055))
                        / channel_t(1.055),
                        channel_t(2.4));
    }

    // delinearize_srgb
    //   function: applies the sRGB companding function,
    // converting a linear-light value to gamma-encoded sRGB.
    D_STATIC D_INLINE channel_t
    delinearize_srgb(
        channel_t _c
    )
    {
        return (_c <= channel_t(0.0031308))
             ? _c * channel_t(12.92)
             : channel_t(1.055)
               * std::pow(_c,
                          channel_t(1) / channel_t(2.4))
               - channel_t(0.055);
    }

    // lab_f
    //   function: CIE L*a*b* forward transfer function.
    D_STATIC D_INLINE channel_t
    lab_f(
        channel_t _t
    )
    {
        D_CONSTEXPR channel_t delta  = channel_t(6)
                                     / channel_t(29);
        D_CONSTEXPR channel_t delta3 = delta * delta * delta;

        return (_t > delta3)
             ? std::cbrt(_t)
             : _t / (channel_t(3) * delta * delta)
               + channel_t(4) / channel_t(29);
    }

    // lab_f_inv
    //   function: CIE L*a*b* inverse transfer function.
    D_STATIC D_INLINE channel_t
    lab_f_inv(
        channel_t _t
    )
    {
        D_CONSTEXPR channel_t delta = channel_t(6)
                                    / channel_t(29);

        return (_t > delta)
             ? _t * _t * _t
             : channel_t(3) * delta * delta
               * (_t - channel_t(4) / channel_t(29));
    }

    // rgb_to_xyz
    //   function: converts linear-light sRGB to CIE XYZ
    // (D65 illuminant).
    D_STATIC D_INLINE cie_xyz
    rgb_to_xyz(
        const rgb& _src
    )
    {
        channel_t rl = linearize_srgb(_src.r);
        channel_t gl = linearize_srgb(_src.g);
        channel_t bl = linearize_srgb(_src.b);

        // sRGB -> XYZ (D65) matrix (IEC 61966-2-1)
        return cie_xyz(
            channel_t(0.4124564) * rl
          + channel_t(0.3575761) * gl
          + channel_t(0.1804375) * bl,
            channel_t(0.2126729) * rl
          + channel_t(0.7151522) * gl
          + channel_t(0.0721750) * bl,
            channel_t(0.0193339) * rl
          + channel_t(0.1191920) * gl
          + channel_t(0.9503041) * bl
        );
    }

    // xyz_to_rgb
    //   function: converts CIE XYZ (D65) to gamma-encoded
    // sRGB.
    D_STATIC D_INLINE rgb
    xyz_to_rgb(
        const cie_xyz& _src
    )
    {
        // XYZ -> sRGB (D65) matrix (inverse of above)
        channel_t rl =  channel_t( 3.2404542) * _src.x
                      + channel_t(-1.5371385) * _src.y
                      + channel_t(-0.4985314) * _src.z;

        channel_t gl =  channel_t(-0.9692660) * _src.x
                      + channel_t( 1.8760108) * _src.y
                      + channel_t( 0.0415560) * _src.z;

        channel_t bl =  channel_t( 0.0556434) * _src.x
                      + channel_t(-0.2040259) * _src.y
                      + channel_t( 1.0572252) * _src.z;

        return rgb(delinearize_srgb(rl),
                   delinearize_srgb(gl),
                   delinearize_srgb(bl));
    }

    // xyz_to_lab
    //   function: converts CIE XYZ to CIE L*a*b* relative
    // to the D65 white point.
    D_STATIC D_INLINE cie_lab
    xyz_to_lab(
        const cie_xyz& _src
    )
    {
        channel_t fx = lab_f(_src.x / d65_x);
        channel_t fy = lab_f(_src.y / d65_y);
        channel_t fz = lab_f(_src.z / d65_z);

        return cie_lab(
            channel_t(116) * fy - channel_t(16),
            channel_t(500) * (fx - fy),
            channel_t(200) * (fy - fz)
        );
    }

    // lab_to_xyz
    //   function: converts CIE L*a*b* to CIE XYZ relative
    // to the D65 white point.
    D_STATIC D_INLINE cie_xyz
    lab_to_xyz(
        const cie_lab& _src
    )
    {
        channel_t fy = (_src.l + channel_t(16))
                     / channel_t(116);
        channel_t fx = _src.a / channel_t(500) + fy;
        channel_t fz = fy - _src.b / channel_t(200);

        return cie_xyz(d65_x * lab_f_inv(fx),
                       d65_y * lab_f_inv(fy),
                       d65_z * lab_f_inv(fz));
    }

    // color_convert_impl<rgb, cie_lab>
    //   trait: converts sRGB to CIE L*a*b* via XYZ
    // intermediate.
    template<>
    struct color_convert_impl<rgb, cie_lab>
    {
        D_STATIC D_INLINE cie_lab
        apply(
            const rgb& _src
        )
        {
            return xyz_to_lab(rgb_to_xyz(_src));
        }
    };

    // color_convert_impl<cie_lab, rgb>
    //   trait: converts CIE L*a*b* to sRGB via XYZ
    // intermediate.
    template<>
    struct color_convert_impl<cie_lab, rgb>
    {
        D_STATIC D_INLINE rgb
        apply(
            const cie_lab& _src
        )
        {
            return xyz_to_rgb(lab_to_xyz(_src));
        }
    };


NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///               VIII.   PUBLIC INTERFACE                                   ///
///////////////////////////////////////////////////////////////////////////////

// ================================================================
//  color_convert
// ================================================================

// color_convert
//   trait: public interface for converting between color
// models. Delegates to the appropriate internal
// color_convert_impl specialization.
template<typename _To,
         typename _From>
struct color_convert
{
    D_STATIC_CONSTEXPR_INLINE _To
    apply(
        const _From& _src
    )
    {
        return internal::color_convert_impl<
            clean_t<_From>,
            clean_t<_To>
        >::apply(_src);
    }
};


// ================================================================
//  color_cast
// ================================================================

// color_cast
//   function: convenience function template for color model
// conversion.
// Usage: auto my_hsl = color_cast<hsl>(my_rgb);
template<typename _To,
         typename _From>
D_CONSTEXPR_INLINE _To
color_cast(
    const _From& _src
)
{
    return color_convert<_To, _From>::apply(_src);
}


// ================================================================
//  is_convertible_color
// ================================================================

// is_convertible_color
//   trait: detects whether a conversion from _From to _To is
// defined.

NS_INTERNAL

    // has_color_convert_helper
    //   trait: SFINAE detection for a valid apply() member
    // in color_convert_impl (primary / failure case).
    template<typename _From,
             typename _To,
             typename = void>
    struct has_color_convert_helper
    {
        D_STATIC_CONSTEXPR bool value = false;
    };

    // has_color_convert_helper (specialization)
    //   trait: success case when apply() is well-formed.
    template<typename _From,
             typename _To>
    struct has_color_convert_helper<
        _From,
        _To,
        void_t<decltype(
            color_convert_impl<_From, _To>::apply(
                *static_cast<const _From*>(nullptr)
            )
        )>
    >
    {
        D_STATIC_CONSTEXPR bool value = true;
    };

NS_END  // internal

// is_convertible_color
//   trait: true if a conversion path from _From to _To
// exists.
template<typename _From,
         typename _To>
struct is_convertible_color
{
    D_STATIC_CONSTEXPR bool value =
        internal::has_color_convert_helper<
            clean_t<_From>,
            clean_t<_To>
        >::value;
};

// is_convertible_color_v
//   constant: convenience accessor for
// is_convertible_color<_From, _To>::value.
template<typename _From,
         typename _To>
D_STATIC_CONSTEXPR bool is_convertible_color_v =
    is_convertible_color<_From, _To>::value;


// ================================================================
//  C++20 concepts (conditional)
// ================================================================

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// color_model_type
//   concept: constrains types that are valid djinterp color
// models (i.e. types with a nested model_tag).
template<typename _T>
concept color_model_type = is_color_model_v<_T>;

// convertible_color_type
//   concept: constrains that a conversion from _From to _To
// is defined.
template<typename _From,
         typename _To>
concept convertible_color_type =
    is_convertible_color_v<_From, _To>;

#endif  // C++20


NS_END  // color
NS_END  // djinterp

#endif  // DJINTERP_COLOR_CONVERT_
