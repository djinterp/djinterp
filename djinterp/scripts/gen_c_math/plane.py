"""gen_c_math.plane - the text of plane.h (S439).

One module of scripts/gen_c_math_float.py, which runs them all; see it."""

from .common import (DATE, FAMILIES, banner, instantiate, overload_section)


# -----------------------------------------------------------------------------
# plane.h
# -----------------------------------------------------------------------------

PLANE_TEMPLATE = r"""
//==============================================================================
// @SECTION@.  THE @UPPER@ FAMILY (_@S@)
//==============================================================================
// Points are a caller's (x, y) pairs; a polygon's vertices are interleaved,
// x0 y0 x1 y1 ..., n of them. The formulas are the 2D shape header's own, in
// their order, one operation per statement; each is a constant expression
// from C++14.


// @SECTION@.1    A segment, and the +x ray
//------------------------------------------------------------------------------

// d_plane_@S@_cross
//   a.x b.y - b.x a.y: the shoelace term, twice the signed area the segment
// ab sweeps about the origin.
D_MATH_FN @T@
d_plane_@S@_cross(
    const @T@* _a,
    const @T@* _b
)
{
    const @T@ first  = _a[0] * _b[1];
    const @T@ second = _b[0] * _a[1];

    return first - second;
}

// d_plane_@S@_ray_crosses
//   whether the ray from _p along +x crosses the segment _a _b: the
// segment straddles the ray's line (one end above, the other not - a
// half-open rule, so a vertex on the line counts once), and the crossing
// lies to the right of _p.
D_MATH_FN bool
d_plane_@S@_ray_crosses(
    const @T@* _p,
    const @T@* _a,
    const @T@* _b
)
{
    const bool a_above = (_a[1] > _p[1]);
    const bool b_above = (_b[1] > _p[1]);

    // both ends on one side: no crossing
    if (a_above == b_above)
    {
        return false;
    }

    {
        const @T@ dx  = _b[0] - _a[0];
        const @T@ dy  = _p[1] - _a[1];
        const @T@ run = dx * dy;
        const @T@ den = _b[1] - _a[1];
        const @T@ off = run / den;
        const @T@ x   = off + _a[0];

        return (_p[0] < x);
    }
}

// @SECTION@.2    A polygon
//------------------------------------------------------------------------------

// d_plane_@S@_polygon_contains
//   whether _p lies inside the polygon: the +x ray from it crosses an odd
// number of edges, each edge taken from vertex i to the one before it.
D_MATH_FN bool
d_plane_@S@_polygon_contains(
    const @T@* _xy,
    size_t     _n,
    const @T@* _p
)
{
    size_t crossings = 0u;
    size_t i         = 0u;
    size_t j         = _n - 1u;

    for (i = 0u; i < _n; j = i++)
    {
        // edge i -> j: one more crossing where the ray meets it
        if (d_plane_@S@_ray_crosses(_p, _xy + (2u * i), _xy + (2u * j)))
        {
            ++crossings;
        }
    }

    return ((crossings % 2u) == 1u);
}

// d_plane_@S@_polygon_area
//   |Σ cross(v_i, v_(i+1))| / 2, the shoelace formula.
D_MATH_FN @T@
d_plane_@S@_polygon_area(
    const @T@* _xy,
    size_t     _n
)
{
    @T@    sum = (@T@)0;
    size_t i   = 0u;

    for (i = 0u; i < _n; ++i)
    {
        const size_t j = (i + 1u) % _n;

        sum += d_plane_@S@_cross(_xy + (2u * i), _xy + (2u * j));
    }

    {
        const @T@ result = sum / (@T@)2;

        return (result < (@T@)0) ? -result : result;
    }
}

// d_plane_@S@_polygon_centroid
//   the polygon's centroid into _out, Σ (v_i + v_(i+1)) cross / (3 Σ
// cross) per coordinate; false, _out untouched, where the area is zero.
D_MATH_FN bool
d_plane_@S@_polygon_centroid(
    @T@*       _out,
    const @T@* _xy,
    size_t     _n
)
{
    @T@    cx       = (@T@)0;
    @T@    cy       = (@T@)0;
    @T@    area_sum = (@T@)0;
    size_t i        = 0u;

    for (i = 0u; i < _n; ++i)
    {
        const size_t j     = (i + 1u) % _n;
        const @T@*   vi    = _xy + (2u * i);
        const @T@*   vj    = _xy + (2u * j);
        const @T@    cross = d_plane_@S@_cross(vi, vj);
        const @T@    sx    = vi[0] + vj[0];
        const @T@    sy    = vi[1] + vj[1];
        const @T@    tx    = sx * cross;
        const @T@    ty    = sy * cross;

        cx       += tx;
        cy       += ty;
        area_sum += cross;
    }

    // no area: no centre to weigh
    if (area_sum == (@T@)0)
    {
        return false;
    }

    {
        const @T@ den = (@T@)3 * area_sum;

        _out[0] = cx / den;
        _out[1] = cy / den;
    }

    return true;
}
"""

PLANE_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE FLOAT FAMILY (_f)
    ---------------------
    1.  A segment, and the +x ray
    2.  A polygon
2.  THE DOUBLE FAMILY (_d)
    ----------------------
    1.  A segment, and the +x ray
    2.  A polygon
3.  THE LONG DOUBLE FAMILY (_ld)
    ----------------------------
    1.  A segment, and the +x ray
    2.  A polygon
4.  ONE NAME FOR EVERY FAMILY (C++)
    -------------------------------
*/

#ifndef DJINTERP_C_MATH_PLANE_H
#define DJINTERP_C_MATH_PLANE_H 1

// std
#include <stddef.h>          // size_t
// djinterp
#include "./math_common.h"   // D_MATH_FN
"""

PLANE_TAIL = r"""

#endif  // DJINTERP_C_MATH_PLANE_H
"""

_PC = lambda n: ("const @T@*", n)
PLANE_OVERLOADS = [
    ("D_MATH_FN", "@T@", "d_plane_cross", [_PC("_a"), _PC("_b")]),
    ("D_MATH_FN", "bool", "d_plane_ray_crosses", [_PC("_p"), _PC("_a"), _PC("_b")]),
    ("D_MATH_FN", "bool", "d_plane_polygon_contains", [_PC("_xy"), ("size_t", "_n"), _PC("_p")]),
    ("D_MATH_FN", "@T@", "d_plane_polygon_area", [_PC("_xy"), ("size_t", "_n")]),
    ("D_MATH_FN", "bool", "d_plane_polygon_centroid", [("@T@*", "_out"), _PC("_xy"), ("size_t", "_n")]),
]


def plane():
    lines = [
        "The plane: the C core of the 2D shape header's algorithms - the +x",
        "ray's crossing test, and a polygon's containment, area and centroid -",
        "for float, double and long double.",
        "  GENERATED by scripts/gen_c_math_float.py from one text for the three",
        "families: edit the generator, not this file.",
        "",
        "  Points are a caller's (x, y) pairs and a polygon's vertices",
        "interleaved, the faces' vectors' storage converted to Cartesian. Each",
        "kernel is a constant expression from C++14.",
    ]
    text = banner("plane.h", lines, DATE, "2026.11.21")
    text += "\n" + PLANE_HEAD
    for i, (suffix, ctype, prefix, prose) in enumerate(FAMILIES):
        upper = prose.upper()
        body = PLANE_TEMPLATE.replace("@SECTION@", str(i + 1)).replace("@UPPER@", upper)
        text += instantiate(body, suffix, ctype, prefix, prose)
    text += overload_section("4.  ONE NAME FOR EVERY FAMILY (C++)", PLANE_OVERLOADS)
    text += PLANE_TAIL
    return text
