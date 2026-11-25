"""gen_c_math.coord - the text of coord.h (S439).

One module of scripts/gen_c_math_float.py, which runs them all; see it."""

from .common import (DATE, FAMILIES, auto_section, banner, instantiate,
                     overload_section)


# -----------------------------------------------------------------------------
# coord.h
# -----------------------------------------------------------------------------

COORD_TEMPLATE = r"""
//==============================================================================
// @SECTION@.  THE @UPPER@ FAMILY (_@S@)
//==============================================================================
// A point is a caller's array: polar (r, theta), cylindrical (rho, phi, z),
// spherical (r, theta, phi) with theta the polar angle from +z. The formulas
// are the coordinate headers' own, in their order, one operation per
// statement; an output may be the input.


// @SECTION@.1    Angles and canonical forms
//------------------------------------------------------------------------------
// Exact: the remainder is fmod_c's, which is fmod's, so one form serves.

// d_coord_@S@_normalize_angle
//   _angle in [0, 2 pi): its remainder by 2 pi, a negative one moved up.
D_MATH_FN @T@
d_coord_@S@_normalize_angle(
    @T@ _angle
)
{
    const @T@ two_pi = (@T@)2 * (@T@)3.14159265358979323846L;
    @T@       result = d_math_@S@_fmod_c(_angle, two_pi);

    // a negative remainder: one turn up
    if (result < (@T@)0)
    {
        result += two_pi;
    }

    return result;
}

// d_coord_@S@_polar_canonicalize
//   _out = _p with r >= 0 (a negative r turned a half turn) and theta in
// [0, 2 pi).
D_MATH_FN void
d_coord_@S@_polar_canonicalize(
    @T@*       _out,
    const @T@* _p
)
{
    @T@ r = _p[0];
    @T@ t = _p[1];

    // a negative radius: the same point a half turn round
    if (r < (@T@)0)
    {
        r  = -r;
        t += (@T@)3.14159265358979323846L;
    }

    _out[0] = r;
    _out[1] = d_coord_@S@_normalize_angle(t);

    return;
}

// d_coord_@S@_cylindrical_canonicalize
//   _out = _p with rho >= 0 and phi in [0, 2 pi).
D_MATH_FN void
d_coord_@S@_cylindrical_canonicalize(
    @T@*       _out,
    const @T@* _p
)
{
    @T@ r = _p[0];
    @T@ f = _p[1];

    // a negative radius: the same point a half turn round
    if (r < (@T@)0)
    {
        r  = -r;
        f += (@T@)3.14159265358979323846L;
    }

    _out[0] = r;
    _out[1] = d_coord_@S@_normalize_angle(f);
    _out[2] = _p[2];

    return;
}

// d_coord_@S@_spherical_canonicalize
//   _out = _p with r >= 0, theta in [0, pi] and phi in [0, 2 pi): a negative
// r turned through the origin, a theta past pi folded back with phi a half
// turn round.
D_MATH_FN void
d_coord_@S@_spherical_canonicalize(
    @T@*       _out,
    const @T@* _p
)
{
    const @T@ pi     = (@T@)3.14159265358979323846L;
    const @T@ two_pi = (@T@)2 * pi;
    @T@       r      = _p[0];
    @T@       t      = _p[1];
    @T@       f      = _p[2];

    // a negative radius: through the origin
    if (r < (@T@)0)
    {
        r = -r;
        t = pi - t;
    }

    t = d_math_@S@_fmod_c(t, two_pi);

    // a negative remainder: one turn up
    if (t < (@T@)0)
    {
        t += two_pi;
    }

    // past the south pole: back, and half way round
    if (t > pi)
    {
        t  = two_pi - t;
        f += pi;
    }

    f = d_math_@S@_fmod_c(f, two_pi);

    // a negative remainder: one turn up
    if (f < (@T@)0)
    {
        f += two_pi;
    }

    _out[0] = r;
    _out[1] = t;
    _out[2] = f;

    return;
}

// @SECTION@.2    Conversions, distances, measures
//------------------------------------------------------------------------------
// Each needs a root or a sine, and comes twice: the _c form a constant
// expression from C++14, the plain one on the C library's.
@FORMS@"""

COORD_FORMS = r"""
// d_coord_@S@_polar_to_cartesian@F@
//   _out = (r cos theta, r sin theta).
D_MATH_FN@RT@ void
d_coord_@S@_polar_to_cartesian@F@(
    @T@*       _out,
    const @T@* _p
)
{
    const @T@ r = _p[0];
    const @T@ c = d_math_@S@_cos@F@(_p[1]);
    const @T@ s = d_math_@S@_sin@F@(_p[1]);

    _out[0] = r * c;
    _out[1] = r * s;

    return;
}

// d_coord_@S@_polar_from_cartesian@F@
//   _out = (sqrt(x^2 + y^2), atan2(y, x)).
D_MATH_FN@RT@ void
d_coord_@S@_polar_from_cartesian@F@(
    @T@*       _out,
    const @T@* _c
)
{
    const @T@ x   = _c[0];
    const @T@ y   = _c[1];
    const @T@ xx  = x * x;
    const @T@ yy  = y * y;
    const @T@ sum = xx + yy;

    _out[0] = d_math_@S@_sqrt@F@(sum);
    _out[1] = d_math_@S@_atan2@F@(y, x);

    return;
}

// d_coord_@S@_polar_distance@F@
//   sqrt(r1^2 + r2^2 - 2 r1 r2 cos(theta1 - theta2)), the law of cosines.
D_MATH_FN@RT@ @T@
d_coord_@S@_polar_distance@F@(
    const @T@* _a,
    const @T@* _b
)
{
    const @T@ r1   = _a[0];
    const @T@ r2   = _b[0];
    const @T@ dt   = _a[1] - _b[1];
    const @T@ p1   = r1 * r1;
    const @T@ p2   = r2 * r2;
    const @T@ s    = p1 + p2;
    const @T@ q1   = (@T@)2 * r1;
    const @T@ q2   = q1 * r2;
    const @T@ q3   = q2 * d_math_@S@_cos@F@(dt);
    const @T@ diff = s - q3;

    return d_math_@S@_sqrt@F@(diff);
}

// d_coord_@S@_cylindrical_to_cartesian@F@
//   _out = (rho cos phi, rho sin phi, z).
D_MATH_FN@RT@ void
d_coord_@S@_cylindrical_to_cartesian@F@(
    @T@*       _out,
    const @T@* _p
)
{
    const @T@ r = _p[0];
    const @T@ c = d_math_@S@_cos@F@(_p[1]);
    const @T@ s = d_math_@S@_sin@F@(_p[1]);
    const @T@ z = _p[2];

    _out[0] = r * c;
    _out[1] = r * s;
    _out[2] = z;

    return;
}

// d_coord_@S@_cylindrical_from_cartesian@F@
//   _out = (sqrt(x^2 + y^2), atan2(y, x), z).
D_MATH_FN@RT@ void
d_coord_@S@_cylindrical_from_cartesian@F@(
    @T@*       _out,
    const @T@* _c
)
{
    const @T@ x   = _c[0];
    const @T@ y   = _c[1];
    const @T@ z   = _c[2];
    const @T@ xx  = x * x;
    const @T@ yy  = y * y;
    const @T@ sum = xx + yy;

    _out[0] = d_math_@S@_sqrt@F@(sum);
    _out[1] = d_math_@S@_atan2@F@(y, x);
    _out[2] = z;

    return;
}

// d_coord_@S@_cylindrical_to_spherical@F@
//   _out = (sqrt(rho^2 + z^2), atan2(rho, z), phi).
D_MATH_FN@RT@ void
d_coord_@S@_cylindrical_to_spherical@F@(
    @T@*       _out,
    const @T@* _p
)
{
    const @T@ r   = _p[0];
    const @T@ f   = _p[1];
    const @T@ z   = _p[2];
    const @T@ rr  = r * r;
    const @T@ zz  = z * z;
    const @T@ sum = rr + zz;

    _out[0] = d_math_@S@_sqrt@F@(sum);
    _out[1] = d_math_@S@_atan2@F@(r, z);
    _out[2] = f;

    return;
}

// d_coord_@S@_spherical_to_cylindrical@F@
//   _out = (r sin theta, phi, r cos theta).
D_MATH_FN@RT@ void
d_coord_@S@_spherical_to_cylindrical@F@(
    @T@*       _out,
    const @T@* _p
)
{
    const @T@ r = _p[0];
    const @T@ s = d_math_@S@_sin@F@(_p[1]);
    const @T@ c = d_math_@S@_cos@F@(_p[1]);
    const @T@ f = _p[2];

    _out[0] = r * s;
    _out[1] = f;
    _out[2] = r * c;

    return;
}

// d_coord_@S@_cylindrical_distance@F@
//   sqrt(r1^2 + r2^2 - 2 r1 r2 cos(phi1 - phi2) + (z1 - z2)^2).
D_MATH_FN@RT@ @T@
d_coord_@S@_cylindrical_distance@F@(
    const @T@* _a,
    const @T@* _b
)
{
    const @T@ r1   = _a[0];
    const @T@ r2   = _b[0];
    const @T@ dp   = _a[1] - _b[1];
    const @T@ dz   = _a[2] - _b[2];
    const @T@ p1   = r1 * r1;
    const @T@ p2   = r2 * r2;
    const @T@ s    = p1 + p2;
    const @T@ q1   = (@T@)2 * r1;
    const @T@ q2   = q1 * r2;
    const @T@ q3   = q2 * d_math_@S@_cos@F@(dp);
    const @T@ diff = s - q3;
    const @T@ zz   = dz * dz;
    const @T@ all  = diff + zz;

    return d_math_@S@_sqrt@F@(all);
}

// d_coord_@S@_cylindrical_norm@F@
//   sqrt(rho^2 + z^2), the distance from the origin.
D_MATH_FN@RT@ @T@
d_coord_@S@_cylindrical_norm@F@(
    const @T@* _p
)
{
    const @T@ rr  = _p[0] * _p[0];
    const @T@ zz  = _p[2] * _p[2];
    const @T@ sum = rr + zz;

    return d_math_@S@_sqrt@F@(sum);
}

// d_coord_@S@_spherical_to_cartesian@F@
//   _out = (r sin theta cos phi, r sin theta sin phi, r cos theta).
D_MATH_FN@RT@ void
d_coord_@S@_spherical_to_cartesian@F@(
    @T@*       _out,
    const @T@* _p
)
{
    const @T@ r   = _p[0];
    const @T@ st  = d_math_@S@_sin@F@(_p[1]);
    const @T@ ct  = d_math_@S@_cos@F@(_p[1]);
    const @T@ cp  = d_math_@S@_cos@F@(_p[2]);
    const @T@ sp  = d_math_@S@_sin@F@(_p[2]);
    const @T@ rst = r * st;

    _out[0] = rst * cp;
    _out[1] = rst * sp;
    _out[2] = r * ct;

    return;
}

// d_coord_@S@_spherical_from_cartesian@F@
//   _out = (r, atan2(sqrt(x^2 + y^2), z), atan2(y, x)), r = sqrt(x^2 + y^2 +
// z^2); the origin for the origin. The polar angle is an arc-tangent, not
// acos(z / r): near a pole acos's slope is unbounded, and a float point a
// thousandth of a radian from the pole lost two percent of its angle.
D_MATH_FN@RT@ void
d_coord_@S@_spherical_from_cartesian@F@(
    @T@*       _out,
    const @T@* _c
)
{
    const @T@ x   = _c[0];
    const @T@ y   = _c[1];
    const @T@ z   = _c[2];
    const @T@ xx  = x * x;
    const @T@ yy  = y * y;
    const @T@ zz  = z * z;
    const @T@ xy  = xx + yy;
    const @T@ sum = xy + zz;
    const @T@ r   = d_math_@S@_sqrt@F@(sum);

    // the origin has no angles: the origin
    if (r == (@T@)0)
    {
        _out[0] = (@T@)0;
        _out[1] = (@T@)0;
        _out[2] = (@T@)0;

        return;
    }

    {
        const @T@ rho = d_math_@S@_sqrt@F@(xy);

        _out[0] = r;
        _out[1] = d_math_@S@_atan2@F@(rho, z);
        _out[2] = d_math_@S@_atan2@F@(y, x);
    }

    return;
}

// d_coord_@S@_spherical_distance@F@
//   the distance between two points, through their cartesian forms.
D_MATH_FN@RT@ @T@
d_coord_@S@_spherical_distance@F@(
    const @T@* _a,
    const @T@* _b
)
{
    @T@ ca[3] = { (@T@)0, (@T@)0, (@T@)0 };
    @T@ cb[3] = { (@T@)0, (@T@)0, (@T@)0 };

    d_coord_@S@_spherical_to_cartesian@F@(ca, _a);
    d_coord_@S@_spherical_to_cartesian@F@(cb, _b);

    {
        const @T@ dx  = ca[0] - cb[0];
        const @T@ dy  = ca[1] - cb[1];
        const @T@ dz  = ca[2] - cb[2];
        const @T@ xx  = dx * dx;
        const @T@ yy  = dy * dy;
        const @T@ zz  = dz * dz;
        const @T@ xy  = xx + yy;
        const @T@ sum = xy + zz;

        return d_math_@S@_sqrt@F@(sum);
    }
}

// d_coord_@S@_spherical_angular_distance@F@
//   the angle between two directions, by the Vincenty form: atan2 of the
// cross product's length over the dot product, accurate at every angle.
D_MATH_FN@RT@ @T@
d_coord_@S@_spherical_angular_distance@F@(
    const @T@* _a,
    const @T@* _b
)
{
    const @T@ st1 = d_math_@S@_sin@F@(_a[1]);
    const @T@ ct1 = d_math_@S@_cos@F@(_a[1]);
    const @T@ st2 = d_math_@S@_sin@F@(_b[1]);
    const @T@ ct2 = d_math_@S@_cos@F@(_b[1]);
    const @T@ dp  = _a[2] - _b[2];
    const @T@ cdp = d_math_@S@_cos@F@(dp);
    const @T@ sdp = d_math_@S@_sin@F@(dp);
    // n1 = st2 sdp; n2 = st1 ct2 - ct1 st2 cdp
    const @T@ n1  = st2 * sdp;
    const @T@ a1  = st1 * ct2;
    const @T@ b1  = ct1 * st2;
    const @T@ b2  = b1 * cdp;
    const @T@ n2  = a1 - b2;
    const @T@ n11 = n1 * n1;
    const @T@ n22 = n2 * n2;
    const @T@ nn  = n11 + n22;
    const @T@ num = d_math_@S@_sqrt@F@(nn);
    // den = ct1 ct2 + st1 st2 cdp
    const @T@ c12 = ct1 * ct2;
    const @T@ s12 = st1 * st2;
    const @T@ s3  = s12 * cdp;
    const @T@ den = c12 + s3;

    return d_math_@S@_atan2@F@(num, den);
}

// d_coord_@S@_spherical_scale_factors@F@
//   _out = (1, r, r sin theta), the Lame coefficients.
D_MATH_FN@RT@ void
d_coord_@S@_spherical_scale_factors@F@(
    @T@*       _out,
    const @T@* _p
)
{
    const @T@ r  = _p[0];
    const @T@ st = d_math_@S@_sin@F@(_p[1]);

    _out[0] = (@T@)1;
    _out[1] = r;
    _out[2] = r * st;

    return;
}

// d_coord_@S@_spherical_jacobian@F@
//   r^2 sin theta, the volume element.
D_MATH_FN@RT@ @T@
d_coord_@S@_spherical_jacobian@F@(
    const @T@* _p
)
{
    const @T@ rr = _p[0] * _p[0];

    return rr * d_math_@S@_sin@F@(_p[1]);
}

// d_coord_@S@_spherical_gradient_scale@F@
//   _out = (1, 1 / r, 1 / (r sin theta)), 0 where the denominator is.
D_MATH_FN@RT@ void
d_coord_@S@_spherical_gradient_scale@F@(
    @T@*       _out,
    const @T@* _p
)
{
    const @T@ r  = _p[0];
    const @T@ st = d_math_@S@_sin@F@(_p[1]);

    _out[0] = (@T@)1;
    _out[1] = (r == (@T@)0) ? (@T@)0 : ((@T@)1 / r);

    // r sin theta, unless either factor is zero
    if ( (r == (@T@)0) ||
         (st == (@T@)0) )
    {
        _out[2] = (@T@)0;
    }
    else
    {
        const @T@ rs = r * st;

        _out[2] = (@T@)1 / rs;
    }

    return;
}
"""

COORD_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE FLOAT FAMILY (_f)
    ---------------------
    1.  Angles and canonical forms
    2.  Conversions, distances, measures
2.  THE DOUBLE FAMILY (_d)
    ----------------------
    1.  Angles and canonical forms
    2.  Conversions, distances, measures
3.  THE LONG DOUBLE FAMILY (_ld)
    ----------------------------
    1.  Angles and canonical forms
    2.  Conversions, distances, measures
4.  ONE NAME FOR EVERY FAMILY (C++)
    -------------------------------
5.  ONE NAME FOR EVERY LEVEL, AND ANY TYPE (C++)
    --------------------------------------------
*/

#ifndef DJINTERP_C_MATH_COORD_H
#define DJINTERP_C_MATH_COORD_H 1

// djinterp
#include "./elementary.h"    // d_math_*_sqrt, _sin, _cos and their _c forms
#include "./math_common.h"   // D_MATH_FN
#include "./remainder.h"     // d_math_*_fmod_c
#include "./tangent.h"       // d_math_*_atan2 and its _c form
"""

COORD_TAIL = r"""

#endif  // DJINTERP_C_MATH_COORD_H
"""

_CO = ("@T@*", "_out")
_CP = ("const @T@*", "_p")
_CC = ("const @T@*", "_c")
_CA = ("const @T@*", "_a")
_CB = ("const @T@*", "_b")
COORD_OVERLOADS = [
    ("D_MATH_FN", "@T@", "d_coord_normalize_angle", [("@T@", "_angle")]),
    ("D_MATH_FN", "void", "d_coord_polar_canonicalize", [_CO, _CP]),
    ("D_MATH_FN", "void", "d_coord_cylindrical_canonicalize", [_CO, _CP]),
    ("D_MATH_FN", "void", "d_coord_spherical_canonicalize", [_CO, _CP]),
]
for _f, _m in (("_c", "D_MATH_FN"), ("", "D_MATH_FN_RT")):
    COORD_OVERLOADS += [
        (_m, "void", "d_coord_polar_to_cartesian" + _f, [_CO, _CP]),
        (_m, "void", "d_coord_polar_from_cartesian" + _f, [_CO, _CC]),
        (_m, "@T@", "d_coord_polar_distance" + _f, [_CA, _CB]),
        (_m, "void", "d_coord_cylindrical_to_cartesian" + _f, [_CO, _CP]),
        (_m, "void", "d_coord_cylindrical_from_cartesian" + _f, [_CO, _CC]),
        (_m, "void", "d_coord_cylindrical_to_spherical" + _f, [_CO, _CP]),
        (_m, "void", "d_coord_spherical_to_cylindrical" + _f, [_CO, _CP]),
        (_m, "@T@", "d_coord_cylindrical_distance" + _f, [_CA, _CB]),
        (_m, "@T@", "d_coord_cylindrical_norm" + _f, [_CP]),
        (_m, "void", "d_coord_spherical_to_cartesian" + _f, [_CO, _CP]),
        (_m, "void", "d_coord_spherical_from_cartesian" + _f, [_CO, _CC]),
        (_m, "@T@", "d_coord_spherical_distance" + _f, [_CA, _CB]),
        (_m, "@T@", "d_coord_spherical_angular_distance" + _f, [_CA, _CB]),
        (_m, "void", "d_coord_spherical_scale_factors" + _f, [_CO, _CP]),
        (_m, "@T@", "d_coord_spherical_jacobian" + _f, [_CP]),
        (_m, "void", "d_coord_spherical_gradient_scale" + _f, [_CO, _CP]),
    ]


_A2 = lambda n: ("const @T@*", n, 2)
_A3 = lambda n: ("const @T@*", n, 3)
_O2 = ("@T@*", "_out", 2)
_O3 = ("@T@*", "_out", 3)
COORD_AUTO = [
    ("void", "d_coord_polar_to_cartesian", [_O2, _A2("_p")]),
    ("void", "d_coord_polar_from_cartesian", [_O2, _A2("_c")]),
    ("@T@", "d_coord_polar_distance", [_A2("_a"), _A2("_b")]),
    ("void", "d_coord_cylindrical_to_cartesian", [_O3, _A3("_p")]),
    ("void", "d_coord_cylindrical_from_cartesian", [_O3, _A3("_c")]),
    ("void", "d_coord_cylindrical_to_spherical", [_O3, _A3("_p")]),
    ("void", "d_coord_spherical_to_cylindrical", [_O3, _A3("_p")]),
    ("@T@", "d_coord_cylindrical_distance", [_A3("_a"), _A3("_b")]),
    ("@T@", "d_coord_cylindrical_norm", [_A3("_p")]),
    ("void", "d_coord_spherical_to_cartesian", [_O3, _A3("_p")]),
    ("void", "d_coord_spherical_from_cartesian", [_O3, _A3("_c")]),
    ("@T@", "d_coord_spherical_distance", [_A3("_a"), _A3("_b")]),
    ("@T@", "d_coord_spherical_angular_distance", [_A3("_a"), _A3("_b")]),
    ("void", "d_coord_spherical_scale_factors", [_O3, _A3("_p")]),
    ("@T@", "d_coord_spherical_jacobian", [_A3("_p")]),
    ("void", "d_coord_spherical_gradient_scale", [_O3, _A3("_p")]),
]
COORD_EXACT = [
    ("@T@", "d_coord_normalize_angle", [("@T@", "_angle", None)]),
    ("void", "d_coord_polar_canonicalize", [_O2, _A2("_p")]),
    ("void", "d_coord_cylindrical_canonicalize", [_O3, _A3("_p")]),
    ("void", "d_coord_spherical_canonicalize", [_O3, _A3("_p")]),
]


def coord():
    lines = [
        "Coordinate systems: the C core of the math/coordinate headers - polar,",
        "cylindrical and spherical conversions, distances, measures and canonical",
        "forms - for float, double and long double.",
        "  GENERATED by scripts/gen_c_math_float.py from one text for the three",
        "families: edit the generator, not this file.",
        "",
        "  A point here is a caller's array; the faces' point type is",
        "linalg::vector, whose storage it is. What needs a root, a sine or an",
        "arc-tangent comes twice, as the elementary functions do: the _c form a",
        "constant expression from C++14, the plain one on the C library's. The",
        "angles' canonical forms are exact, so one form serves.",
    ]
    text = banner("coord.h", lines, DATE, "2026.11.21")
    text += "\n" + COORD_HEAD
    for i, (suffix, ctype, prefix, prose) in enumerate(FAMILIES):
        upper = prose.upper()
        forms = ""
        for f, rt in (("_c", ""), ("", "_RT")):
            forms += COORD_FORMS.replace("@F@", f).replace("@RT@", rt)
        body = COORD_TEMPLATE.replace("@FORMS@", forms)
        body = body.replace("@SECTION@", str(i + 1)).replace("@UPPER@", upper)
        text += instantiate(body, suffix, ctype, prefix, prose)
    text += overload_section("4.  ONE NAME FOR EVERY FAMILY (C++)", COORD_OVERLOADS)
    text += auto_section("5.  ONE NAME FOR EVERY LEVEL, AND ANY TYPE (C++)", COORD_AUTO, COORD_EXACT)
    text += COORD_TAIL
    return text
