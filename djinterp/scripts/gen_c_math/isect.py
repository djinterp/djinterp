"""gen_c_math.isect - the text of isect.h (S439).

One module of scripts/gen_c_math_float.py, which runs them all; see it."""

from .common import (DATE, FAMILIES, banner, instantiate, overload_section)


# -----------------------------------------------------------------------------
# isect.h
# -----------------------------------------------------------------------------

ISECT_TEMPLATE = r"""
//==============================================================================
// @SECTION@.  THE @UPPER@ FAMILY (_@S@)
//==============================================================================
// Rays are an origin and a direction, 3-vectors; the formulas are the
// geometry headers' own, in their order, one operation per statement. Each
// is a constant expression from C++14.


// @SECTION@.1    A ray against a box and a triangle
//------------------------------------------------------------------------------

// d_isect_@S@_ray_aabb
//   whether the ray from _origin along _dir meets the box [_box_min,
// _box_max] at a non-negative parameter, by Kay and Kajiya's slabs; if so,
// *_t_near and *_t_far are where it enters and leaves (an origin inside the
// box enters behind it). A direction component within 1e-12 of zero is
// parallel to its slab, which the origin must then lie between.
D_MATH_FN bool
d_isect_@S@_ray_aabb(
    const @T@* _origin,
    const @T@* _dir,
    const @T@* _box_min,
    const @T@* _box_max,
    @T@*       _t_near,
    @T@*       _t_far
)
{
    @T@    t_min = -d_internal_math_@S@_infinity();
    @T@    t_max = d_internal_math_@S@_infinity();
    size_t i     = 0u;

    for (i = 0u; i < 3u; ++i)
    {
        const @T@ o = _origin[i];
        const @T@ d = _dir[i];

        // parallel to the slab: inside it, or no hit
        if ( (d > -(@T@)1e-12) &&
             (d < (@T@)1e-12) )
        {
            if ( (o < _box_min[i]) ||
                 (o > _box_max[i]) )
            {
                return false;
            }

            continue;
        }

        {
            const @T@ inv   = (@T@)1 / d;
            const @T@ to_lo = _box_min[i] - o;
            const @T@ to_hi = _box_max[i] - o;
            @T@       t1    = to_lo * inv;
            @T@       t2    = to_hi * inv;

            // the near face first
            if (t1 > t2)
            {
                const @T@ tmp = t1;

                t1 = t2;
                t2 = tmp;
            }

            if (t1 > t_min)
            {
                t_min = t1;
            }

            if (t2 < t_max)
            {
                t_max = t2;
            }

            // the slabs' intervals no longer overlap: no hit
            if (t_min > t_max)
            {
                return false;
            }
        }
    }

    // the box is behind the ray
    if (t_max < (@T@)0)
    {
        return false;
    }

    *_t_near = t_min;
    *_t_far  = t_max;

    return true;
}

// d_isect_@S@_ray_triangle
//   whether the ray from _origin along _dir meets the triangle (_v0, _v1,
// _v2) at a parameter above _epsilon, by Moller and Trumbore's method; if
// so, *_t_out is the parameter. A ray within _epsilon of the triangle's
// plane misses it.
D_MATH_FN bool
d_isect_@S@_ray_triangle(
    const @T@* _origin,
    const @T@* _dir,
    const @T@* _v0,
    const @T@* _v1,
    const @T@* _v2,
    @T@        _epsilon,
    @T@*       _t_out
)
{
    @T@ edge1[3] = { (@T@)0, (@T@)0, (@T@)0 };
    @T@ edge2[3] = { (@T@)0, (@T@)0, (@T@)0 };
    @T@ h[3]     = { (@T@)0, (@T@)0, (@T@)0 };
    @T@ s[3]     = { (@T@)0, (@T@)0, (@T@)0 };
    @T@ q[3]     = { (@T@)0, (@T@)0, (@T@)0 };
    @T@ a        = (@T@)0;

    d_vec_@S@_sub(edge1, _v1, _v0, 3u);
    d_vec_@S@_sub(edge2, _v2, _v0, 3u);
    d_vec_@S@_cross_fast(h, _dir, edge2);
    a = d_vec_@S@_dot_fast(edge1, h, 3u);

    // parallel to the triangle's plane
    if ( (a > -_epsilon) &&
         (a < _epsilon) )
    {
        return false;
    }

    {
        const @T@ f = (@T@)1 / a;
        @T@       u = (@T@)0;
        @T@       v = (@T@)0;
        @T@       t = (@T@)0;

        d_vec_@S@_sub(s, _origin, _v0, 3u);
        u = f * d_vec_@S@_dot_fast(s, h, 3u);

        // outside the edge opposite v2 or v1
        if ( (u < (@T@)0) ||
             (u > (@T@)1) )
        {
            return false;
        }

        d_vec_@S@_cross_fast(q, s, edge1);
        v = f * d_vec_@S@_dot_fast(_dir, q, 3u);

        // outside the remaining edges
        if ( (v < (@T@)0) ||
             ((u + v) > (@T@)1) )
        {
            return false;
        }

        t = f * d_vec_@S@_dot_fast(edge2, q, 3u);

        // only ahead of the origin
        if (t > _epsilon)
        {
            *_t_out = t;

            return true;
        }
    }

    return false;
}
"""

ISECT_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE FLOAT FAMILY (_f)
    ---------------------
    1.  A ray against a box and a triangle
2.  THE DOUBLE FAMILY (_d)
    ----------------------
    1.  A ray against a box and a triangle
3.  THE LONG DOUBLE FAMILY (_ld)
    ----------------------------
    1.  A ray against a box and a triangle
4.  ONE NAME FOR EVERY FAMILY (C++)
    -------------------------------
*/

#ifndef DJINTERP_C_MATH_ISECT_H
#define DJINTERP_C_MATH_ISECT_H 1

// std
#include <stddef.h>          // size_t
// djinterp
#include "./exponential.h"   // d_internal_math_*_infinity
#include "./math_common.h"   // D_MATH_FN
#include "./vec.h"           // d_vec_*_sub, _cross, _dot
"""

ISECT_TAIL = r"""

#endif  // DJINTERP_C_MATH_ISECT_H
"""

_IC = lambda n: ("const @T@*", n)
ISECT_OVERLOADS = [
    ("D_MATH_FN", "bool", "d_isect_ray_aabb", [_IC("_origin"), _IC("_dir"), _IC("_box_min"), _IC("_box_max"), ("@T@*", "_t_near"), ("@T@*", "_t_far")]),
    ("D_MATH_FN", "bool", "d_isect_ray_triangle", [_IC("_origin"), _IC("_dir"), _IC("_v0"), _IC("_v1"), _IC("_v2"), ("@T@", "_epsilon"), ("@T@*", "_t_out")]),
]


def isect():
    lines = [
        "Intersections: the C core of the geometry headers' ray tests - a ray",
        "against an axis-aligned box and against a triangle - for float, double",
        "and long double.",
        "  GENERATED by scripts/gen_c_math_float.py from one text for the three",
        "families: edit the generator, not this file.",
        "",
        "  Points and directions are a caller's 3-arrays, the faces' vectors'",
        "storage. Each test is a constant expression from C++14.",
    ]
    text = banner("isect.h", lines, DATE, "2026.11.24")
    text += "\n" + ISECT_HEAD
    for i, (suffix, ctype, prefix, prose) in enumerate(FAMILIES):
        upper = prose.upper()
        body = ISECT_TEMPLATE.replace("@SECTION@", str(i + 1)).replace("@UPPER@", upper)
        text += instantiate(body, suffix, ctype, prefix, prose)
    text += overload_section("4.  ONE NAME FOR EVERY FAMILY (C++)", ISECT_OVERLOADS)
    text += ISECT_TAIL
    return text
