#!/usr/bin/env python3
"""gen_c_math_float.py -- write the C math core's floating-point families.

C has no templates, and a floating-point kernel cannot be written once over
the widest type and reached by conversion, as interval's integer kernels
are: a float computed as a long double is a different float. So each
floating-point kernel exists three times, for float, double and long double
(the _f, _d and _ld families), and this script writes all three from one
text, a template below in which @T@ is the type, @S@ the family's suffix and
@P@, @EMIN@, @EMAX@, @MIN@, @MAX@, @EPS@, @DIG@ its <float.h> properties.

It writes, each into inc/djinterp/c/modules/math/:

  float_order.h      the order of a floating-point type's values: successor,
                     predecessor, ordinal distance and advance -- in pure
                     arithmetic, so the same text is constexpr from C++14
  interval_float.h   interval's floating-point families
  elementary.h       square root, arc-sine, arc-cosine, sine and cosine, each
                     as a pure-arithmetic (constexpr) form and a C library
                     form, and the long double kernels the headers below share
  exponential.h      exponentials and logarithms, to bases e, 2 and 10, in the
                     same two forms
  tangent.h          tangent, arc-tangent and the two-argument arc-tangent, in
                     the same two forms
  hyperbolic.h       hyperbolic sine, cosine and tangent, in the same two
                     forms
  power.h            the cube root, the power and the hypotenuse, in the same
                     two forms
  remainder.h        the remainders, fmod, remainder and remquo: exact, in the
                     same two forms
  nearest_integer.h  floor, ceil, trunc and round: exact, over the whole
                     range, in the same two forms
  vec.h              fixed-size vectors, the C core of
                     linear_algebra/vector.hpp
  mat.h              fixed-size matrices, the C core of
                     linear_algebra/matrix.hpp
  quat.h             quaternions, the C core of both quaternion headers
  coord.h            coordinate systems, the C core of math/coordinate/*.hpp
  isect.h            ray intersections, the C core of the geometry headers'
                     ray tests
  plane.h            the plane: crossings and polygons, the C core of
                     shape_2d.hpp
  decomp.h           LU, QR and Cholesky, the C core of
                     linear_algebra/decomposition.hpp
  cplx.h             complex numbers: the complex type, its arithmetic and
                     C99's complex functions, the C core of complex.hpp
  cmat.h             complex matrices: LU, inverse, solution and eigensystems

usage:  scripts/gen_c_math_float.py           write every header
        scripts/gen_c_math_float.py --check   exit 1 if any differs from
                                              what the script would write
"""
import os
import re
import textwrap
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))

# the folder every header is written to, from the root
CORE = "inc/djinterp/c/modules/math"

FAMILIES = [
    # suffix, type, <float.h> prefix, name in prose
    ("f",  "float",       "FLT",  "float"),
    ("d",  "double",      "DBL",  "double"),
    ("ld", "long double", "LDBL", "long double"),
]

DATE = "2026.10.04"


def banner(filename, lines, created_on=DATE, revised_on=DATE):
    """the standard header banner, its fixed geometry included."""
    path = "/" + CORE + "/" + filename
    title = "* djinterp [c/math]"
    head = title + " " * (80 - len(title) - len(filename)) + filename
    body = "\n".join(("*" + (" " + l if l else "")).rstrip() for l in lines)
    author = "* author(s): Samuel 'teer' Neal-Blim"
    created = "created: " + created_on
    revised = "revised: " + revised_on
    return "\n".join([
        "/" + "*" * 79,
        head,
        "*",
        body,
        "*",
        "*",
        "* path:      " + path,
        "* link(s):   TBA",
        author + " " * (80 - len(author) - len(created)) + created,
        "*" + " " * (80 - 1 - len(revised)) + revised,
        "*" * 79 + "/",
    ])


def instantiate(template, suffix, ctype, prefix, prose):
    """one family's text: the template with its placeholders filled."""
    out = template
    for key, value in (
        ("@T@", ctype),
        ("@S@", suffix),
        ("@NAME@", prose),
        ("@P@", prefix + "_MANT_DIG"),
        ("@EMIN@", prefix + "_MIN_EXP"),
        ("@EMAX@", prefix + "_MAX_EXP"),
        ("@MIN@", prefix + "_MIN"),
        ("@MAX@", prefix + "_MAX"),
        ("@EPS@", prefix + "_EPSILON"),
        ("@DIG@", prefix + "_DIG"),
        ("@PRINTF@", "L" if ctype == "long double" else ""),
        ("@SCANF@", {"float": "", "double": "l", "long double": "L"}[ctype]),
    ):
        out = out.replace(key, value)
    return out




# -----------------------------------------------------------------------------
# C++ overloads
# -----------------------------------------------------------------------------
# Compiled as C++, each generated header also names its kernels once for all
# three families -- d_interval_contains(core, value) for d_interval_f_contains,
# _d_ and _ld_ -- chosen by argument type: what lets one C++ template serve
# every floating-point type without a macro.

def overload(qual, ret, name, target, params):
    """one C++ overload forwarding to a family's kernel, in the house form."""
    width = max(len(t) for t, _ in params) if params else 0
    lines = [qual + " " + ret, name + "("]
    for i, (ptype, pname) in enumerate(params):
        sep = "," if i + 1 < len(params) else ""
        lines.append("    " + ptype.ljust(width) + " " + pname + sep)
    lines.append(")")
    lines.append("{")
    args = ", ".join(p for _, p in params)
    call = "    return " + target + "(" + args + ");"
    if len(call) > 80:
        call = "    return " + target + "(" + (",\n" + " " * (len("    return " + target + "("))).join(p for _, p in params) + ");"
    lines.append(call)
    lines.append("}")
    return "\n".join(lines)


def overload_section(title, specs):
    """the C++-only section: every spec, for every family."""
    out = ["", "", "#ifdef __cplusplus", "",
           "//" + "=" * 78,
           "// " + title,
           "//" + "=" * 78,
           "// C++ only: the kernels above under one name each, chosen by argument type,",
           "// so a C++ template serves float, double and long double alike.", "", ""]
    for suffix, ctype, prefix, prose in FAMILIES:
        for qual, ret, name, params in specs:
            r = ret.replace("@T@", ctype).replace("@S@", suffix)
            ps = [(t.replace("@T@", ctype).replace("@S@", suffix), n) for t, n in params]
            # the family's suffix goes after the module's prefix: d_vec_add
            # forwards to d_vec_d_add, d_math_next_up to d_math_d_next_up
            target = None
            for prefix_name in ("d_interval_", "d_math_", "d_vec_", "d_mat_", "d_quat_", "d_coord_", "d_isect_", "d_plane_", "d_decomp_", "d_cplx_", "d_cmat_"):
                if name.startswith(prefix_name):
                    target = prefix_name + suffix + "_" + name[len(prefix_name):]
            assert target is not None, name
            out.append(overload(qual, r, name, target, ps))
            out.append("")
    out.append("#endif  // __cplusplus")
    return "\n".join(out)


def auto_section(title, specs, exact_specs):
    """the C++-only section: each two-form kernel under one name for every
    family and level (_auto: the C library's at run time, the _c form in
    constant evaluation from C++20), then a template for any other element
    type, through double. specs: (ret, name, [(type, param, length)]);
    exact_specs, the one-form kernels: the template only."""
    out = ["", "", "#ifdef __cplusplus", "",
           "//" + "=" * 78,
           "// " + title,
           "//" + "=" * 78,
           "// C++ only. Each two-form kernel under one name, _auto, for every family",
           "// and level: the C library's form at run time at every level, and from",
           "// C++20 the _c form when std::is_constant_evaluated says so. Then, for",
           "// an element type no family holds, a template through double -- the",
           "// subframework's generic path -- for the _auto names and for the exact",
           "// one-form kernels alike.", "",
           "#if D_ENV_LANG_IS_CPP20_OR_HIGHER",
           "    #include <type_traits>  // std::is_constant_evaluated",
           "#endif", "", ""]

    def decl(ret, name, ps, constexpr_kw):
        lines = [constexpr_kw + " " + ret, name + "("]
        width = max(len(t) for t, _ in ps)
        for i, (t, n) in enumerate(ps):
            lines.append("    " + t.ljust(width) + " " + n + ("," if i + 1 < len(ps) else ""))
        lines.append(") D_NOEXCEPT")
        return lines

    for suffix, ctype, prefix, prose in FAMILIES:
        for ret, name, params in specs:
            r = ret.replace("@T@", ctype)
            ps = [(t.replace("@T@", ctype), n) for t, n, _ in params]
            target = name.replace("d_coord_", "d_coord_" + suffix + "_")
            args = ", ".join(n for _, n in ps)
            auto_name = name + "_auto"
            ret_kw = "return " if r != "void" else ""
            out.append("#if D_ENV_LANG_IS_CPP20_OR_HIGHER")
            out += decl(r, auto_name, ps, "constexpr")
            out.append("{")
            if r == "void":
                out += ["    if (std::is_constant_evaluated())", "    {",
                        "        " + target + "_c(" + args + ");", "    }",
                        "    else", "    {",
                        "        " + target + "(" + args + ");", "    }", "",
                        "    return;"]
            else:
                out += ["    if (std::is_constant_evaluated())", "    {",
                        "        return " + target + "_c(" + args + ");", "    }", "",
                        "    return " + target + "(" + args + ");"]
            out.append("}")
            out.append("#else")
            out += decl(r, auto_name, ps, "inline")
            out.append("{")
            out.append("    " + ret_kw + target + "(" + args + ");")
            if r == "void":
                out += ["", "    return;"]
            out.append("}")
            out.append("#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER")
            out.append("")

    def through_double(ret, name, params, constexpr_kw):
        r = ret.replace("@T@", "Type")
        ps = [(t.replace("@T@", "Type"), n) for t, n, _ in params]
        lines = ["template<typename Type>"] + decl(r, name, ps, constexpr_kw)
        lines.append("{")
        call_args = []
        writes = []
        for t, n, length in params:
            local = "d" + n
            if length is None:
                lines.append("    const double " + local + " = static_cast<double>(" + n + ");")
            elif t.startswith("const"):
                elems = ", ".join("static_cast<double>(" + n + "[" + str(k) + "])" for k in range(length))
                lines.append("    const double " + local + "[" + str(length) + "] = { " + elems + " };")
            else:
                zeros = ", ".join("0.0" for _ in range(length))
                lines.append("    double       " + local + "[" + str(length) + "] = { " + zeros + " };")
                writes.append((n, local, length))
            call_args.append(local)
        call = name + "(" + ", ".join(call_args) + ")"
        if r == "void":
            lines.append("")
            lines.append("    " + call + ";")
            for n, local, length in writes:
                for k in range(length):
                    lines.append("    " + n + "[" + str(k) + "] = static_cast<Type>(" + local + "[" + str(k) + "]);")
            lines += ["", "    return;"]
        else:
            lines.append("")
            lines.append("    return static_cast<Type>(" + call + ");")
        lines.append("}")
        return lines

    out.append("// for any other element type, through double")
    for ret, name, params in specs:
        out += through_double(ret, name + "_auto", params, "D_CONSTEXPR_CPP20")
        out.append("")
    for ret, name, params in exact_specs:
        out += through_double(ret, name, params, "D_CONSTEXPR_CPP14")
        out.append("")
    out.append("#endif  // __cplusplus")
    return "\n".join(out)


FLOAT_ORDER_OVERLOADS = [
    ("D_MATH_FN", "@T@", "d_math_next_up", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_next_down", [("@T@", "_x")]),
    ("D_MATH_FN", "dj_math_umax", "d_math_steps", [("@T@", "_from"), ("@T@", "_to")]),
    ("D_MATH_FN", "@T@", "d_math_advance", [("@T@", "_from"), ("dj_math_umax", "_steps")]),
]

IV = "struct dj_interval_@S@"
INTERVAL_FLOAT_OVERLOADS = [
    ("D_MATH_FN", IV, "d_interval_make", [("@T@", "_lower"), ("@T@", "_upper"), ("unsigned", "_bounds"), ("@T@", "_step")]),
    ("D_MATH_FN", "bool", "d_interval_is_valid", [(IV, "_interval")]),
    ("D_MATH_FN", "bool", "d_interval_is_discrete", [(IV, "_interval")]),
    ("D_MATH_FN", "bool", "d_interval_is_empty", [(IV, "_interval")]),
    ("D_MATH_FN", "@T@", "d_interval_inclusive_lower", [(IV, "_interval")]),
    ("D_MATH_FN", "@T@", "d_interval_inclusive_upper", [(IV, "_interval")]),
    ("D_MATH_FN", "@T@", "d_interval_first", [(IV, "_interval")]),
    ("D_MATH_FN", "@T@", "d_interval_last", [(IV, "_interval")]),
    ("D_MATH_FN", "dj_math_umax", "d_interval_count", [(IV, "_interval")]),
    ("D_MATH_FN", "@T@", "d_interval_at", [(IV, "_interval"), ("dj_math_umax", "_index")]),
    ("D_MATH_FN", "bool", "d_interval_contains_in_range", [(IV, "_interval"), ("@T@", "_value")]),
    ("D_MATH_FN", "bool", "d_interval_contains", [(IV, "_interval"), ("@T@", "_value")]),
    ("D_MATH_FN", "@T@", "d_interval_clamp", [(IV, "_interval"), ("@T@", "_value")]),
    ("D_MATH_FN", "@T@", "d_interval_clamp_nearest", [(IV, "_interval"), ("@T@", "_value")]),
    ("D_MATH_FN", "dj_math_umax", "d_interval_index_of", [(IV, "_interval"), ("@T@", "_value")]),
    ("D_MATH_FN", "bool", "d_interval_overlaps", [(IV, "_left"), (IV, "_right")]),
    ("D_MATH_FN", "bool", "d_interval_overlaps_continuous", [(IV, "_left"), (IV, "_right")]),
    ("D_MATH_FN", "bool", "d_interval_intersects", [(IV, "_left"), (IV, "_right")]),
    ("D_MATH_FN", "struct dj_math_ratio", "d_interval_normalize_terms", [(IV, "_interval"), ("@T@", "_value")]),
    ("D_MATH_FN", "struct dj_math_ratio", "d_interval_normalize_effective_terms", [(IV, "_interval"), ("@T@", "_value")]),
    ("D_MATH_FN", "struct dj_math_ratio", "d_interval_normalize_discrete_terms", [(IV, "_interval"), ("@T@", "_value")]),
    ("D_MATH_FN_RT", "size_t", "d_interval_format", [("char*", "_buffer"), ("size_t", "_capacity"), (IV, "_interval")]),
]

# -----------------------------------------------------------------------------
# float_order.h
# -----------------------------------------------------------------------------

FLOAT_ORDER_TEMPLATE = r"""
//==============================================================================
// @SECTION@.  THE @UPPER@ FAMILY (_@S@)
//==============================================================================


// @SECTION@.1    Internal helpers
//------------------------------------------------------------------------------

// dj_internal_math_@S@_binade
//   struct: a positive normal value's binade: the exponent E and 2^E, so that
// 2^E <= x < 2^(E + 1).
struct dj_internal_math_@S@_binade
{
    int exponent;  // E
    @T@ power;     // 2^E, exact
};

// d_internal_math_@S@_pow2
//   2^_exponent, exact, for |_exponent| up to @EMAX@ - 1 -- every
// normal @NAME@'s exponent among them: 2^|_exponent| from its bits, the
// least first, each square 2^(2^k) formed only while a bit above k remains,
// so that none passes 2^|_exponent|; for a negative exponent, then, its
// reciprocal, one exact division.
D_MATH_FN @T@
d_internal_math_@S@_pow2(
    int _exponent
)
{
    @T@ result = (@T@)1;
    @T@ square = (@T@)2;
    int left   = (_exponent < 0) ? -_exponent : _exponent;

    while (left > 0)
    {
        if ((left % 2) == 1)
        {
            result *= square;
        }

        left /= 2;

        if (left > 0)
        {
            square *= square;
        }
    }

    return (_exponent < 0) ? ((@T@)1 / result) : result;
}

// d_internal_math_@S@_veltkamp
//   Veltkamp's constant 2^ceil(P/2) + 1, P the family's precision, which
// splits a value into halves whose products are exact (Dekker's product):
// written out for the precisions there are -- 24, 53, 64 and 113 bits -- so
// that no call builds it, and built for any other.
D_MATH_FN @T@
d_internal_math_@S@_veltkamp(void)
{
#if (@P@ == 24)
    return (@T@)4097.0L;                  // 2^12 + 1
#elif (@P@ == 53)
    return (@T@)134217729.0L;             // 2^27 + 1
#elif (@P@ == 64)
    return (@T@)4294967297.0L;            // 2^32 + 1
#elif (@P@ == 113)
    return (@T@)144115188075855873.0L;    // 2^57 + 1
#else
    return d_internal_math_@S@_pow2((@P@ + 1) / 2) + (@T@)1;
#endif
}

// d_internal_math_@S@_binade_of
//   the binade of _x, which must be normal and positive: @MIN@ <= _x <=
// @MAX@. A binary search over the squares 2^(2^k) -- below 1, scaling _x
// itself up toward 1 and dividing once at the end; no value it forms leaves
// the normal range, so every step is exact and, from C++14, a constant
// expression.
D_MATH_FN struct dj_internal_math_@S@_binade
d_internal_math_@S@_binade_of(
    @T@ _x
)
{
    struct dj_internal_math_@S@_binade result = { 0, (@T@)1 };
    @T@ squares[16] = { 0 };
    int count       = 1;
    int k           = 0;

    squares[0] = (@T@)2;

    // 2^(2^k) for every k with 2^k at most the largest exponent
    while ( (count < 16) &&
            ((1 << count) <= (@EMAX@ - 1)) )
    {
        squares[count] = squares[count - 1] * squares[count - 1];
        ++count;
    }

    // at or above 1: climb while the next power stays at or below _x
    if (_x >= (@T@)1)
    {
        for (k = count - 1; k >= 0; --k)
        {
            if ( ((result.exponent + (1 << k)) <= (@EMAX@ - 1)) &&
                 ((result.power * squares[k]) <= _x) )
            {
                result.power    *= squares[k];
                result.exponent += (1 << k);
            }
        }

        return result;
    }

    // below 1: scale _x up by 2^e while it stays below 1 -- then it lies in
    // [1/2, 1), E is -e - 1, and 2^E the reciprocal of 2^(e + 1)
    {
        @T@ climbed = _x;
        @T@ up     = (@T@)1;
        int e      = 0;

        for (k = count - 1; k >= 0; --k)
        {
            if ( ((e + (1 << k)) <= -@EMIN@) &&
                 ((climbed * squares[k]) < (@T@)1) )
            {
                climbed *= squares[k];
                up      *= squares[k];
                e       += (1 << k);
            }
        }

        result.exponent = -e - 1;
        result.power    = (@T@)1 / (up + up);
    }

    return result;
}

// d_internal_math_@S@_true_min
//   the least positive @NAME@: the subnormal spacing,
// @MIN@ * @EPS@.
D_MATH_FN @T@
d_internal_math_@S@_true_min(void)
{
    return (@MIN@ * @EPS@);
}

// dj_internal_math_@S@_place
//   a non-negative value's place in the order: binade B and offset O within
// it, so that its ordinal is B * 2^(P-1) + O. B is 0 for zero and the
// subnormals; O is an integer below 2^(P-1), held exactly in @T@;
// +inf is the first place past the largest binade.
struct dj_internal_math_@S@_place
{
    int binade;  // B
    @T@ offset;  // O
};

D_MATH_FN struct dj_internal_math_@S@_place
d_internal_math_@S@_place_of(
    @T@ _x
)
{
    struct dj_internal_math_@S@_place result = { 0, (@T@)0 };

    // +inf is the order's end: one step past the largest finite value, the
    // first place of the binade after the last
    if (_x > @MAX@)
    {
        result.binade = (@EMAX@ - (@EMIN@ - 1)) + 1;

        return result;
    }

    // zero and the subnormals: one binade, in steps of the least value
    if (_x < @MIN@)
    {
        result.offset = _x / d_internal_math_@S@_true_min();

        return result;
    }

    {
        const struct dj_internal_math_@S@_binade b =
            d_internal_math_@S@_binade_of(_x);

        result.binade = b.exponent - (@EMIN@ - 1) + 1;
        result.offset = (_x - b.power) / (b.power * @EPS@);
    }

    return result;
}

// d_internal_math_@S@_value_of
//   the inverse of place_of: the value at binade _binade, offset _offset.
D_MATH_FN @T@
d_internal_math_@S@_value_of(
    int _binade,
    @T@ _offset
)
{
    // the subnormal binade
    if (_binade == 0)
    {
        return (_offset * d_internal_math_@S@_true_min());
    }

    {
        const @T@ power =
            d_internal_math_@S@_pow2(_binade - 1 + (@EMIN@ - 1));

        return (power + (_offset * (power * @EPS@)));
    }
}

// d_internal_math_@S@_umax_bits
//   the bits of dj_math_umax, assuming none is padding.
D_MATH_FN int
d_internal_math_@S@_umax_bits(void)
{
    return (int)(sizeof(dj_math_umax) * 8u);
}

// d_internal_math_@S@_full
//   2^(P-1), the count of one binade, as dj_math_umax: asked for only where it
// fits, and the shift clamped so the expression is well-formed where it does
// not (a 32-bit dj_math_umax and a double's 52).
D_MATH_FN dj_math_umax
d_internal_math_@S@_full(void)
{
    return ((dj_math_umax)1)
               << ( ((@P@ - 1) < (int)(sizeof(dj_math_umax) * 8u))
                        ? (@P@ - 1)
                        : 0 );
}

// d_internal_math_@S@_steps_up
//   how many values lie in [_from, _to), for 0 <= _from <= _to, +inf among
// them: from their places, so the count is exact wherever it fits
// dj_math_umax, however far from zero the two lie; it saturates at
// D_MATH_UMAX_MAX.
D_MATH_FN dj_math_umax
d_internal_math_@S@_steps_up(
    @T@ _from,
    @T@ _to
)
{
    const struct dj_internal_math_@S@_place low  =
        d_internal_math_@S@_place_of(_from);
    const struct dj_internal_math_@S@_place high =
        d_internal_math_@S@_place_of(_to);
    const int binades = high.binade - low.binade;

    // a binade holds more than dj_math_umax counts: a count fits within one,
    // or across one edge -- the rest of the lower binade and the start of
    // the upper, each below 2^(P-1) and their sum exact in @T@
    if ((@P@ - 1) >= d_internal_math_@S@_umax_bits())
    {
        const @T@ full = d_internal_math_@S@_pow2(@P@ - 1);
        const @T@ count =
            (binades == 0) ? (high.offset - low.offset)
                           : ((full - low.offset) + high.offset);

        if ( (binades > 1) ||
             (!(count < (@T@)D_MATH_UMAX_MAX)) )
        {
            return D_MATH_UMAX_MAX;
        }

        return (dj_math_umax)count;
    }

    {
        const dj_math_umax full        = d_internal_math_@S@_full();
        const dj_math_umax from_offset = (dj_math_umax)low.offset;
        const dj_math_umax to_offset   = (dj_math_umax)high.offset;
        const dj_math_umax whole       = (dj_math_umax)binades;

        // whole * full + to_offset - from_offset, saturating
        if (whole > ((D_MATH_UMAX_MAX - to_offset) / full))
        {
            return D_MATH_UMAX_MAX;
        }

        return (((whole * full) + to_offset) - from_offset);
    }
}

// d_internal_math_@S@_climb
//   the value _steps above a non-negative finite _from; +inf past the
// largest finite value.
D_MATH_FN @T@
d_internal_math_@S@_climb(
    @T@          _from,
    dj_math_umax _steps
)
{
    const struct dj_internal_math_@S@_place p =
        d_internal_math_@S@_place_of(_from);
    // the largest binade
    const int top    = (@EMAX@ - (@EMIN@ - 1));
    int       binade = p.binade;
    @T@       offset = p.offset;

    // a binade holds more than dj_math_umax: the steps cross at most one edge
    if ((@P@ - 1) >= d_internal_math_@S@_umax_bits())
    {
        const @T@ full = d_internal_math_@S@_pow2(@P@ - 1);

        offset += (@T@)_steps;

        if (!(offset < full))
        {
            offset -= full;
            ++binade;
        }
    }
    else
    {
        const dj_math_umax full  = d_internal_math_@S@_full();
        const dj_math_umax whole = _steps / full;
        dj_math_umax       rest  = ((dj_math_umax)offset) + (_steps % full);

        // whole binades first, then what is left within one
        if (whole > (dj_math_umax)top)
        {
            return (@T@)HUGE_VAL;
        }

        binade += (int)whole;

        if (rest >= full)
        {
            rest -= full;
            ++binade;
        }

        offset = (@T@)rest;
    }

    // past the largest binade lies infinity
    if (binade > top)
    {
        return (@T@)HUGE_VAL;
    }

    return d_internal_math_@S@_value_of(binade, offset);
}

// d_internal_math_@S@_descend
//   the value _steps below a positive _from, +inf among them; _steps must be
// fewer than the values in [0, _from), and from +inf at least one -- its
// place lies past the last binade, which value_of does not build.
D_MATH_FN @T@
d_internal_math_@S@_descend(
    @T@          _from,
    dj_math_umax _steps
)
{
    const struct dj_internal_math_@S@_place p =
        d_internal_math_@S@_place_of(_from);
    int binade = p.binade;
    @T@ offset = p.offset;

    // a binade holds more than dj_math_umax: at most one edge is crossed
    if ((@P@ - 1) >= d_internal_math_@S@_umax_bits())
    {
        const @T@ back = (@T@)_steps;

        if (back <= offset)
        {
            offset -= back;
        }
        else
        {
            const @T@ full = d_internal_math_@S@_pow2(@P@ - 1);

            offset = (offset + full) - back;
            --binade;
        }
    }
    else
    {
        const dj_math_umax full  = d_internal_math_@S@_full();
        const dj_math_umax rest  = _steps % full;
        dj_math_umax       place = (dj_math_umax)offset;

        // whole binades first, then what is left within one
        binade -= (int)(_steps / full);

        if (rest > place)
        {
            place = (place + full) - rest;
            --binade;
        }
        else
        {
            place -= rest;
        }

        offset = (@T@)place;
    }

    return d_internal_math_@S@_value_of(binade, offset);
}

// @SECTION@.2    Successor and predecessor
//------------------------------------------------------------------------------
// next_up is the least value above _x, next_down the greatest below:
// nextafter toward +inf and toward -inf, in pure arithmetic. +0 and -0 are
// one value, so next_up(-least) is 0. NaN stays NaN; next_up(+inf) is +inf
// and next_up(-inf) the most negative finite value.

D_MATH_FN @T@
d_math_@S@_next_up(
    @T@ _x
)
{
    // NaN and +inf are their own successors
    if ( (_x != _x) ||
         (_x > @MAX@) )
    {
        return _x;
    }

    // -inf rises to the most negative finite value
    if (_x < -@MAX@)
    {
        return -@MAX@;
    }

    // the largest finite value rises to +inf
    if (_x == @MAX@)
    {
        return (@T@)HUGE_VAL;
    }

    // zero and the subnormals are evenly spaced by the least value
    if ( (_x < @MIN@) &&
         (_x > -@MIN@) )
    {
        return (_x + d_internal_math_@S@_true_min());
    }

    // a positive normal value: one spacing of its binade up
    if (_x > (@T@)0)
    {
        const struct dj_internal_math_@S@_binade b =
            d_internal_math_@S@_binade_of(_x);

        return (_x + (b.power * @EPS@));
    }

    {
        // a negative normal value: toward zero by the spacing below its
        // magnitude, which below a power of two is half its own -- except
        // below the least normal value, where it is the subnormal spacing
        const @T@ magnitude = -_x;
        const struct dj_internal_math_@S@_binade b =
            d_internal_math_@S@_binade_of(magnitude);
        const @T@ spacing =
            ( (magnitude == b.power) && (magnitude > @MIN@) )
                ? ((b.power * @EPS@) / (@T@)2)
                : (b.power * @EPS@);

        return -(magnitude - spacing);
    }
}

D_MATH_FN @T@
d_math_@S@_next_down(
    @T@ _x
)
{
    return -d_math_@S@_next_up(-_x);
}

// @SECTION@.3    Ordinal distance and advance
//------------------------------------------------------------------------------
// steps is how many values lie in [_from, _to) -- for adjacent values 1 --
// for _from <= _to, saturating at D_MATH_UMAX_MAX; advance is the value
// _steps above _from, +inf past the largest finite value, and no steps
// leave _from where it is. The infinities are the order's ends, one step
// past the largest finite values; NaN is in no order, so advance keeps a
// NaN, as next_up does, and a count with one saturates: there is none to
// give. So advance(a, steps(a, b)) == b wherever steps(a, b) does not
// saturate.

D_MATH_FN dj_math_umax
d_math_@S@_steps(
    @T@ _from,
    @T@ _to
)
{
    // a NaN: no count
    if ( (_from != _from) ||
         (_to != _to) )
    {
        return D_MATH_UMAX_MAX;
    }

    // both at or above zero
    if (_from >= (@T@)0)
    {
        return d_internal_math_@S@_steps_up(_from, _to);
    }

    // both at or below zero: the mirror image
    if (_to <= (@T@)0)
    {
        return d_internal_math_@S@_steps_up(-_to, -_from);
    }

    {
        // across zero: up to zero, then on from it, saturating
        const dj_math_umax below =
            d_internal_math_@S@_steps_up((@T@)0, -_from);
        const dj_math_umax above =
            d_internal_math_@S@_steps_up((@T@)0, _to);

        return (above > (D_MATH_UMAX_MAX - below)) ? D_MATH_UMAX_MAX
                                                   : (below + above);
    }
}

D_MATH_FN @T@
d_math_@S@_advance(
    @T@          _from,
    dj_math_umax _steps
)
{
    // a NaN stays NaN
    if (_from != _from)
    {
        return _from;
    }

    // no steps stay put -- from -inf too, where descend would start past
    // the last binade; a zero comes back as +0, as the climb gives it
    if (_steps == 0u)
    {
        return (_from == (@T@)0) ? (@T@)0 : _from;
    }

    // from zero or above: climb
    if (_from >= (@T@)0)
    {
        return d_internal_math_@S@_climb(_from, _steps);
    }

    {
        // from below zero: as many steps as reach zero, at most, stay below
        // it -- a count that saturates is more than _steps can be
        const dj_math_umax to_zero =
            d_internal_math_@S@_steps_up((@T@)0, -_from);

        if ( (_steps < to_zero) ||
             (to_zero == D_MATH_UMAX_MAX) )
        {
            return -d_internal_math_@S@_descend(-_from, _steps);
        }

        return d_internal_math_@S@_climb((@T@)0, _steps - to_zero);
    }
}
"""

FLOAT_ORDER_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE FLOAT FAMILY (_f)
    ---------------------
    1.  Internal helpers
    2.  Successor and predecessor
    3.  Ordinal distance and advance
2.  THE DOUBLE FAMILY (_d)
    ----------------------
    1.  Internal helpers
    2.  Successor and predecessor
    3.  Ordinal distance and advance
3.  THE LONG DOUBLE FAMILY (_ld)
    ----------------------------
    1.  Internal helpers
    2.  Successor and predecessor
    3.  Ordinal distance and advance
4.  ONE NAME FOR EVERY FAMILY (C++)
    -------------------------------
*/

#ifndef DJINTERP_C_MATH_FLOAT_ORDER_H
#define DJINTERP_C_MATH_FLOAT_ORDER_H 1

// std
#include <float.h>           // FLT_MANT_DIG ... LDBL_EPSILON
#include <math.h>            // HUGE_VAL
// djinterp
#include "./math_common.h"   // D_MATH_FN, dj_math_umax, D_MATH_UMAX_MAX
"""

FLOAT_ORDER_TAIL = r"""

#endif  // DJINTERP_C_MATH_FLOAT_ORDER_H
"""


def float_order():
    lines = [
        "The order of a floating-point type's values: successor, predecessor,",
        "ordinal distance and advance, for float, double and long double.",
        "  GENERATED by scripts/gen_c_math_float.py from one text for the three",
        "families: edit the generator, not this file.",
        "",
        "  Every function is pure arithmetic -- no bit pattern is read, no C",
        "library function called -- so the same text is constexpr from C++14,",
        "where a face needs a successor in a template argument. The values are",
        "taken as numbers, so +0 and -0 are one value; NaN is no value's",
        "neighbour. Subnormals are assumed (IEEE 754): a build that flushes them",
        "to zero sees gaps the order does not know about.",
    ]
    text = banner("float_order.h", lines, DATE, "2026.11.07")
    text += "\n" + FLOAT_ORDER_HEAD
    for i, (suffix, ctype, prefix, prose) in enumerate(FAMILIES):
        upper = prose.upper()
        body = FLOAT_ORDER_TEMPLATE.replace("@SECTION@", str(i + 1)).replace("@UPPER@", upper)
        text += instantiate(body, suffix, ctype, prefix, prose)
    text += overload_section("4.  ONE NAME FOR EVERY FAMILY (C++)", FLOAT_ORDER_OVERLOADS)
    text += FLOAT_ORDER_TAIL
    return text



INTERVAL_FLOAT_TEMPLATE = r"""
//==============================================================================
// @SECTION@.  THE @UPPER@ FAMILY (_@S@)
//==============================================================================


// @SECTION@.1    Layout
//------------------------------------------------------------------------------
// @SECTION@.1.1
// dj_interval_@S@
//   struct: an interval over @NAME@.
struct dj_interval_@S@
{
    @T@      lower;   // the lower bound
    @T@      upper;   // the upper bound
    @T@      step;    // the stride: 0 continuous, > 0 discrete
    unsigned bounds;  // D_INTERVAL_LEFT_OPEN | D_INTERVAL_RIGHT_OPEN
};

// @SECTION@.2    Internal helpers
//------------------------------------------------------------------------------

// d_internal_interval_@S@_member
//   a discrete interval's member _index steps past _first: _first + _index *
// _step, the product formed apart so no compiler fuses it into the sum.
D_MATH_FN @T@
d_internal_interval_@S@_member(
    @T@          _first,
    @T@          _step,
    dj_math_umax _index
)
{
    // the first member is the first, whatever the step (0 * inf is NaN)
    if (_index == 0u)
    {
        return _first;
    }

    {
        const @T@ offset = (@T@)_index * _step;

        return (_first + offset);
    }
}

// d_internal_interval_@S@_snap_finite
//   the index of the last member at or below a finite _value, for _value >=
// _first and finite _first and _step: the rounded quotient, settled by
// testing the members beside it.
D_MATH_FN dj_math_umax
d_internal_interval_@S@_snap_finite(
    @T@ _first,
    @T@ _step,
    @T@ _value
)
{
    const @T@    quotient = (_value - _first) / _step;
    dj_math_umax index    = 0u;
    int          tries    = 0;

    // the quotient as an index: saturated, or truncated
    if (!(quotient < (@T@)D_MATH_UMAX_MAX))
    {
        index = D_MATH_UMAX_MAX;
    }
    else if (quotient > (@T@)0)
    {
        index = (dj_math_umax)quotient;
    }

    // the quotient was rounded: down while the member passes _value...
    for (tries = 0;
         ( (tries < 4) &&
           (index > 0u) &&
           (d_internal_interval_@S@_member(_first, _step, index) > _value) );
         ++tries)
    {
        --index;
    }

    // ...and up while the next one does not
    for (tries = 0;
         ( (tries < 4) &&
           (index < D_MATH_UMAX_MAX) &&
           (d_internal_interval_@S@_member(_first, _step, index + 1u) <=
            _value) );
         ++tries)
    {
        ++index;
    }

    return index;
}

// d_internal_interval_@S@_snap
//   the index of the last member at or below _value, for _value >= _first.
// An infinite first member or step leaves one member, the first; members
// past the largest finite value round to +inf, one member, after the last
// finite one.
D_MATH_FN dj_math_umax
d_internal_interval_@S@_snap(
    @T@ _first,
    @T@ _step,
    @T@ _value
)
{
    // an infinite first member or step: the first is the only member
    if ( (_first > @MAX@) ||
         (_first < -@MAX@) ||
         (_step > @MAX@) )
    {
        return 0u;
    }

    // +inf: the member after the last finite one
    if (_value > @MAX@)
    {
        const dj_math_umax last =
            d_internal_interval_@S@_snap_finite(_first, _step, @MAX@);

        return (last == D_MATH_UMAX_MAX) ? last : (last + 1u);
    }

    return d_internal_interval_@S@_snap_finite(_first, _step, _value);
}

// d_internal_interval_@S@_is_void
//   true when the interval is empty on the real line: reversed or NaN
// bounds, or one point with either bound open.
D_MATH_FN bool
d_internal_interval_@S@_is_void(
    struct dj_interval_@S@ _interval
)
{
    return ( (!(_interval.lower <= _interval.upper)) ||
             ( (_interval.lower == _interval.upper) &&
               (_interval.bounds != D_INTERVAL_CLOSED) ) );
}

// @SECTION@.3    Construction, validity and bounds
//------------------------------------------------------------------------------
// as for the integer families; an open bound moves to the neighbouring
// value (float_order.h's next_up and next_down), so inclusive_lower is the
// least value the interval holds and inclusive_upper the greatest.

D_MATH_FN struct dj_interval_@S@
d_interval_@S@_make(
    @T@      _lower,
    @T@      _upper,
    unsigned _bounds,
    @T@      _step
)
{
    struct dj_interval_@S@ result = { (@T@)0,
                                      (@T@)0,
                                      (@T@)0,
                                      0u };

    result.lower  = _lower;
    result.upper  = _upper;
    result.step   = _step;
    result.bounds = _bounds;

    return result;
}

D_MATH_FN bool
d_interval_@S@_is_valid(
    struct dj_interval_@S@ _interval
)
{
    return ( (_interval.lower <= _interval.upper) &&
             (_interval.step >= (@T@)0) );
}

D_MATH_FN bool
d_interval_@S@_is_discrete(
    struct dj_interval_@S@ _interval
)
{
    return (_interval.step > (@T@)0);
}

D_MATH_FN @T@
d_interval_@S@_inclusive_lower(
    struct dj_interval_@S@ _interval
)
{
    return ((_interval.bounds & D_INTERVAL_LEFT_OPEN) != 0u)
               ? d_math_@S@_next_up(_interval.lower)
               : _interval.lower;
}

D_MATH_FN @T@
d_interval_@S@_inclusive_upper(
    struct dj_interval_@S@ _interval
)
{
    return ((_interval.bounds & D_INTERVAL_RIGHT_OPEN) != 0u)
               ? d_math_@S@_next_down(_interval.upper)
               : _interval.upper;
}

D_MATH_FN bool
d_interval_@S@_is_empty(
    struct dj_interval_@S@ _interval
)
{
    // an open bound at an infinity excludes it, and nothing lies beyond it
    if ( ( ((_interval.bounds & D_INTERVAL_LEFT_OPEN) != 0u) &&
           (_interval.lower > @MAX@) ) ||
         ( ((_interval.bounds & D_INTERVAL_RIGHT_OPEN) != 0u) &&
           (_interval.upper < -@MAX@) ) )
    {
        return true;
    }

    return !( d_interval_@S@_inclusive_lower(_interval) <=
              d_interval_@S@_inclusive_upper(_interval) );
}

// @SECTION@.4    Size and access
//------------------------------------------------------------------------------
// A continuous interval's members are every value of the type within its
// bounds, so count is their number (float_order.h's steps, plus one), at
// the value that many steps up, and index_of the steps to a value. A
// discrete interval's members are first + k * step, each product formed
// apart. Both counts saturate at D_MATH_UMAX_MAX.

D_MATH_FN @T@
d_interval_@S@_first(
    struct dj_interval_@S@ _interval
)
{
    return d_interval_@S@_inclusive_lower(_interval);
}

D_MATH_FN @T@
d_interval_@S@_last(
    struct dj_interval_@S@ _interval
)
{
    const @T@ low  = d_interval_@S@_inclusive_lower(_interval);
    const @T@ high = d_interval_@S@_inclusive_upper(_interval);

    // a discrete interval ends on its last whole step
    if ( (_interval.step > (@T@)0) &&
         (low <= high) )
    {
        return d_internal_interval_@S@_member(
                   low,
                   _interval.step,
                   d_internal_interval_@S@_snap(low, _interval.step, high));
    }

    return high;
}

D_MATH_FN dj_math_umax
d_interval_@S@_count(
    struct dj_interval_@S@ _interval
)
{
    const @T@ low  = d_interval_@S@_inclusive_lower(_interval);
    const @T@ high = d_interval_@S@_inclusive_upper(_interval);
    dj_math_umax steps = 0u;

    // an empty interval counts nothing
    if (d_interval_@S@_is_empty(_interval))
    {
        return 0u;
    }

    steps = (_interval.step > (@T@)0)
                ? d_internal_interval_@S@_snap(low, _interval.step, high)
                : d_math_@S@_steps(low, high);

    return (steps == D_MATH_UMAX_MAX) ? steps : (steps + 1u);
}

D_MATH_FN @T@
d_interval_@S@_at(
    struct dj_interval_@S@ _interval,
    dj_math_umax           _index
)
{
    const @T@ low = d_interval_@S@_inclusive_lower(_interval);

    // a discrete member, or the value _index steps up
    if (_interval.step > (@T@)0)
    {
        return d_internal_interval_@S@_member(low, _interval.step, _index);
    }

    return d_math_@S@_advance(low, _index);
}

// @SECTION@.5    Membership and clamping
//------------------------------------------------------------------------------
// contains honours both bounds exactly and, for a discrete interval, the
// members; contains_in_range the bounds alone; NaN is in no interval. clamp
// moves a value to the nearest member toward the interval -- the
// neighbouring value inside an open bound -- and, inside a discrete one,
// down to a member; clamp_nearest to the nearer member, a tie going down.
// NaN clamps to NaN.

D_MATH_FN bool
d_interval_@S@_contains_in_range(
    struct dj_interval_@S@ _interval,
    @T@                    _value
)
{
    const bool left_ok =
        ((_interval.bounds & D_INTERVAL_LEFT_OPEN) != 0u)
            ? (_value > _interval.lower)
            : (_value >= _interval.lower);
    const bool right_ok =
        ((_interval.bounds & D_INTERVAL_RIGHT_OPEN) != 0u)
            ? (_value < _interval.upper)
            : (_value <= _interval.upper);

    return (left_ok && right_ok);
}

D_MATH_FN bool
d_interval_@S@_contains(
    struct dj_interval_@S@ _interval,
    @T@                    _value
)
{
    // outside the bounds is outside the interval
    if (!d_interval_@S@_contains_in_range(_interval, _value))
    {
        return false;
    }

    // a discrete member is one of the computed members
    if (_interval.step > (@T@)0)
    {
        const @T@ low = d_interval_@S@_inclusive_lower(_interval);

        return ( d_internal_interval_@S@_member(
                     low,
                     _interval.step,
                     d_internal_interval_@S@_snap(low,
                                                  _interval.step,
                                                  _value)) == _value );
    }

    return true;
}

D_MATH_FN @T@
d_interval_@S@_clamp(
    struct dj_interval_@S@ _interval,
    @T@                    _value
)
{
    const @T@ low  = d_interval_@S@_inclusive_lower(_interval);
    const @T@ high = d_interval_@S@_inclusive_upper(_interval);

    // NaN has no nearer member
    if (_value != _value)
    {
        return _value;
    }

    // below the interval: its first value
    if (_value < low)
    {
        return low;
    }

    // above the interval: its last value
    if (_value > high)
    {
        return d_interval_@S@_last(_interval);
    }

    // inside a discrete interval: down to a member
    if (_interval.step > (@T@)0)
    {
        return d_internal_interval_@S@_member(
                   low,
                   _interval.step,
                   d_internal_interval_@S@_snap(low, _interval.step, _value));
    }

    return _value;
}

D_MATH_FN @T@
d_interval_@S@_clamp_nearest(
    struct dj_interval_@S@ _interval,
    @T@                    _value
)
{
    // NaN, a continuous interval, and anything outside one: as clamp
    if ( (_value != _value) ||
         (!(_interval.step > (@T@)0)) ||
         (!d_interval_@S@_contains_in_range(_interval, _value)) )
    {
        return d_interval_@S@_clamp(_interval, _value);
    }

    const @T@ low = d_interval_@S@_inclusive_lower(_interval);
    const @T@ top = d_interval_@S@_last(_interval);

    // past the last member: the last member
    if ( (_value < low) ||
         (_value >= top) )
    {
        return d_interval_@S@_clamp(_interval, _value);
    }

    const dj_math_umax index =
        d_internal_interval_@S@_snap(low, _interval.step, _value);
    const @T@ below =
        d_internal_interval_@S@_member(low, _interval.step, index);
    const @T@ above =
        d_internal_interval_@S@_member(low, _interval.step, index + 1u);

    // the member above is nearer, and still inside the interval
    if ( (above <= top) &&
         ((_value - below) > (above - _value)) )
    {
        return above;
    }

    return below;
}

D_MATH_FN dj_math_umax
d_interval_@S@_index_of(
    struct dj_interval_@S@ _interval,
    @T@                    _value
)
{
    // a value that is not a member has the index one past the last
    if (!d_interval_@S@_contains(_interval, _value))
    {
        return d_interval_@S@_count(_interval);
    }

    {
        const @T@ low = d_interval_@S@_inclusive_lower(_interval);

        return (_interval.step > (@T@)0)
                   ? d_internal_interval_@S@_snap(low, _interval.step, _value)
                   : d_math_@S@_steps(low, _value);
    }
}

// @SECTION@.6    Relations
//------------------------------------------------------------------------------
// as for the integer families: overlaps compares the inclusive ranges,
// ignoring strides; overlaps_continuous the real line; intersects asks for
// a shared member -- for two discrete intervals by testing the members of
// the sparser one within the ranges' common part, so its cost is the count
// of those.

D_MATH_FN bool
d_interval_@S@_overlaps(
    struct dj_interval_@S@ _left,
    struct dj_interval_@S@ _right
)
{
    // an empty interval shares a position with nothing
    if ( (d_interval_@S@_is_empty(_left)) ||
         (d_interval_@S@_is_empty(_right)) )
    {
        return false;
    }

    return !( (d_interval_@S@_inclusive_upper(_left) <
               d_interval_@S@_inclusive_lower(_right)) ||
              (d_interval_@S@_inclusive_lower(_left) >
               d_interval_@S@_inclusive_upper(_right)) );
}

D_MATH_FN bool
d_interval_@S@_overlaps_continuous(
    struct dj_interval_@S@ _left,
    struct dj_interval_@S@ _right
)
{
    // an interval empty on the real line meets nothing
    if ( (d_internal_interval_@S@_is_void(_left)) ||
         (d_internal_interval_@S@_is_void(_right)) )
    {
        return false;
    }

    const bool left_meets_closed =
        ( ((_left.bounds  & D_INTERVAL_LEFT_OPEN)  == 0u) &&
          ((_right.bounds & D_INTERVAL_RIGHT_OPEN) == 0u) );
    const bool right_meets_closed =
        ( ((_left.bounds  & D_INTERVAL_RIGHT_OPEN) == 0u) &&
          ((_right.bounds & D_INTERVAL_LEFT_OPEN)  == 0u) );
    const bool left_ok =
        left_meets_closed ? (_left.lower <= _right.upper)
                          : (_left.lower <  _right.upper);
    const bool right_ok =
        right_meets_closed ? (_right.lower <= _left.upper)
                           : (_right.lower <  _left.upper);

    return (left_ok && right_ok);
}

D_MATH_FN bool
d_interval_@S@_intersects(
    struct dj_interval_@S@ _left,
    struct dj_interval_@S@ _right
)
{
    // two continuous intervals share a value where their ranges meet
    if ( (!(_left.step > (@T@)0)) &&
         (!(_right.step > (@T@)0)) )
    {
        return d_interval_@S@_overlaps(_left, _right);
    }

    // an empty interval has no member to share
    if ( (d_interval_@S@_is_empty(_left)) ||
         (d_interval_@S@_is_empty(_right)) )
    {
        return false;
    }

    {
        // walk the sparser discrete one; test each member in the other
        const bool                   left_walks =
            ( (_left.step > (@T@)0) &&
              ( (!(_right.step > (@T@)0)) ||
                (_left.step >= _right.step) ) );
        const struct dj_interval_@S@ walk  = left_walks ? _left  : _right;
        const struct dj_interval_@S@ other = left_walks ? _right : _left;
        const @T@ first = d_interval_@S@_inclusive_lower(walk);
        const @T@ low   = (first > d_interval_@S@_inclusive_lower(other))
                              ? first
                              : d_interval_@S@_inclusive_lower(other);
        const @T@ high  = (d_interval_@S@_last(walk) <
                           d_interval_@S@_inclusive_upper(other))
                              ? d_interval_@S@_last(walk)
                              : d_interval_@S@_inclusive_upper(other);
        const dj_math_umax total = d_interval_@S@_count(walk);
        dj_math_umax index = d_internal_interval_@S@_snap(first, walk.step, low);

        // the first member at or above the common part's start
        if (d_internal_interval_@S@_member(first, walk.step, index) < low)
        {
            ++index;
        }

        // every member within the common part, until one is shared
        while ( (index < total) &&
                (d_internal_interval_@S@_member(first, walk.step, index) <=
                 high) )
        {
            if (d_interval_@S@_contains(
                    other,
                    d_internal_interval_@S@_member(first, walk.step, index)))
            {
                return true;
            }

            ++index;
        }

        return false;
    }
}

// @SECTION@.7    Normalization
//------------------------------------------------------------------------------
// as for the integer families, the terms in long double.

D_MATH_FN struct dj_math_ratio
d_internal_interval_@S@_terms(
    @T@ _origin,
    @T@ _end,
    @T@ _value
)
{
    struct dj_math_ratio result = { 0.0L, 1.0L };

    // a degenerate span maps everything to 0
    if (_end == _origin)
    {
        return result;
    }

    result.numerator   = (long double)_value - (long double)_origin;
    result.denominator = (long double)_end   - (long double)_origin;

    return result;
}

D_MATH_FN struct dj_math_ratio
d_interval_@S@_normalize_terms(
    struct dj_interval_@S@ _interval,
    @T@                    _value
)
{
    return d_internal_interval_@S@_terms(_interval.lower,
                                         _interval.upper,
                                         _value);
}

D_MATH_FN struct dj_math_ratio
d_interval_@S@_normalize_effective_terms(
    struct dj_interval_@S@ _interval,
    @T@                    _value
)
{
    return d_internal_interval_@S@_terms(
               d_interval_@S@_inclusive_lower(_interval),
               d_interval_@S@_inclusive_upper(_interval),
               _value);
}

D_MATH_FN struct dj_math_ratio
d_interval_@S@_normalize_discrete_terms(
    struct dj_interval_@S@ _interval,
    @T@                    _value
)
{
    // a continuous interval normalizes over its inclusive bounds
    if (!(_interval.step > (@T@)0))
    {
        return d_interval_@S@_normalize_effective_terms(_interval, _value);
    }

    struct dj_math_ratio result = { 0.0L, 1.0L };
    const dj_math_umax   count  = d_interval_@S@_count(_interval);
    const @T@            low    =
        d_interval_@S@_inclusive_lower(_interval);

    // one member or none: nothing to spread over
    if (count <= 1u)
    {
        return result;
    }

    // at or above the first member: the index of the member at or below;
    // below it, the quotient truncated toward zero, a zero kept +0
    if (_value >= low)
    {
        result.numerator = (long double)d_internal_interval_@S@_snap(
                               low,
                               _interval.step,
                               _value);
    }
    else
    {
        const long double back = ((long double)low - (long double)_value) /
                                 (long double)_interval.step;
        const dj_math_umax whole =
            (back < (long double)D_MATH_UMAX_MAX) ? (dj_math_umax)back
                                                  : D_MATH_UMAX_MAX;

        result.numerator = (whole == 0u) ? 0.0L : -(long double)whole;
    }

    result.denominator = (long double)(count - 1u);

    return result;
}

// @SECTION@.8    Formatting
//------------------------------------------------------------------------------

// d_internal_interval_@S@_put_value
//   writes _value at _position as the fewest significant digits (from
// @DIG@) that read back as the same value -- 0.1, not 0.10000000000000001 --
// and returns the position after it.
D_MATH_FN_RT size_t
d_internal_interval_@S@_put_value(
    char*  _buffer,
    size_t _capacity,
    size_t _position,
    @T@    _value
)
{
    char   text[64] = { 0 };
    int    digits   = @DIG@;
    size_t i        = 0u;
    size_t position = _position;

    // the shortest round trip; NaN never compares equal, and keeps the last
    for (digits = @DIG@; digits <= (@DIG@ + 3); ++digits)
    {
        @T@ back = (@T@)0;

        (void)sprintf(text, "%.*@PRINTF@g", digits, (@PRINTCAST@)_value);

        if ( (sscanf(text, "%@SCANF@g", &back) == 1) &&
             (back == _value) )
        {
            break;
        }
    }

    for (i = 0u; text[i] != '\0'; ++i)
    {
        position = d_internal_interval_put_char(_buffer,
                                                _capacity,
                                                position,
                                                text[i]);
    }

    return position;
}

/**
 * @brief Writes an interval of the @NAME@ family as text, as
 *        d_interval_imax_format does, each value in the fewest digits that
 *        read back as it.
 *
 * @param[out] _buffer    where the text goes; may be NULL if _capacity is 0.
 * @param[in]  _capacity  the bytes _buffer holds.
 * @param[in]  _interval  the interval to write.
 * @return the length of the whole text, not counting the terminator.
 */
D_MATH_FN_RT size_t
d_interval_@S@_format(
    char*                  _buffer,
    size_t                 _capacity,
    struct dj_interval_@S@ _interval
)
{
    const char left  =
        ((_interval.bounds & D_INTERVAL_LEFT_OPEN) != 0u) ? '(' : '[';
    const char right =
        ((_interval.bounds & D_INTERVAL_RIGHT_OPEN) != 0u) ? ')' : ']';
    size_t     at    = d_internal_interval_put_char(_buffer,
                                                    _capacity,
                                                    0u,
                                                    left);

    at = d_internal_interval_@S@_put_value(_buffer,
                                           _capacity,
                                           at,
                                           _interval.lower);

    // a discrete interval shows its stride between its bounds
    if (_interval.step > (@T@)0)
    {
        at = d_internal_interval_put_char(_buffer, _capacity, at, ':');
        at = d_internal_interval_@S@_put_value(_buffer,
                                               _capacity,
                                               at,
                                               _interval.step);
        at = d_internal_interval_put_char(_buffer, _capacity, at, ':');
    }
    else
    {
        at = d_internal_interval_put_char(_buffer, _capacity, at, ',');
        at = d_internal_interval_put_char(_buffer, _capacity, at, ' ');
    }

    at = d_internal_interval_@S@_put_value(_buffer,
                                           _capacity,
                                           at,
                                           _interval.upper);
    at = d_internal_interval_put_char(_buffer, _capacity, at, right);

    return d_internal_interval_finish(_buffer, _capacity, at);
}
"""

INTERVAL_FLOAT_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE FLOAT FAMILY (_f)
    ---------------------
    1.  Layout
         1.  dj_interval_f
    2.  Internal helpers
    3.  Construction, validity and bounds
    4.  Size and access
    5.  Membership and clamping
    6.  Relations
    7.  Normalization
    8.  Formatting
2.  THE DOUBLE FAMILY (_d)
    ----------------------
    1.  Layout
         1.  dj_interval_d
    2.  Internal helpers
    3.  Construction, validity and bounds
    4.  Size and access
    5.  Membership and clamping
    6.  Relations
    7.  Normalization
    8.  Formatting
3.  THE LONG DOUBLE FAMILY (_ld)
    ----------------------------
    1.  Layout
         1.  dj_interval_ld
    2.  Internal helpers
    3.  Construction, validity and bounds
    4.  Size and access
    5.  Membership and clamping
    6.  Relations
    7.  Normalization
    8.  Formatting
4.  ONE NAME FOR EVERY FAMILY (C++)
    -------------------------------
*/

#ifndef DJINTERP_C_MATH_INTERVAL_FLOAT_H
#define DJINTERP_C_MATH_INTERVAL_FLOAT_H 1

// std
#include <stddef.h>          // size_t, NULL
#include <stdio.h>           // sprintf, sscanf
// djinterp
#include "./float_order.h"   // d_math_*_next_up, _next_down, _steps, _advance
#include "./interval.h"      // dj_interval_bound, the formatting helpers
#include "./math_common.h"   // D_MATH_FN, dj_math_umax, dj_math_ratio
"""

INTERVAL_FLOAT_TAIL = r"""

#endif  // DJINTERP_C_MATH_INTERVAL_FLOAT_H
"""


def interval_float():
    lines = [
        "Floating-point intervals: the C core of math/interval for float,",
        "double and long double.",
        "  GENERATED by scripts/gen_c_math_float.py from one text for the three",
        "families: edit the generator, not this file.",
        "",
        "  An interval's members are the values of its type within its bounds,",
        "as for the integer families: here every representable value, so an open",
        "bound moves to the neighbouring value (float_order.h), count is how many",
        "values lie between the inclusive bounds, at and index_of walk them, and",
        "clamp lands inside an open bound on the nearest value it holds. A",
        "discrete interval's members are first + k * step, the product formed",
        "apart from the sum so no compiler fuses the two and the members are the",
        "same everywhere. An infinite first member or step leaves one member, the",
        "first; members past the largest finite value are one member, +inf. NaN",
        "is in no interval.",
        "Every kernel but formatting is pure arithmetic, so constexpr from C++14.",
    ]
    text = banner("interval_float.h", lines, DATE, "2026.11.07")
    text += "\n" + INTERVAL_FLOAT_HEAD
    for i, (suffix, ctype, prefix, prose) in enumerate(FAMILIES):
        upper = prose.upper()
        body = INTERVAL_FLOAT_TEMPLATE.replace("@SECTION@", str(i + 1)).replace("@UPPER@", upper)
        body = body.replace("@PRINTCAST@", "long double" if ctype == "long double" else "double")
        text += instantiate(body, suffix, ctype, prefix, prose)
    text += overload_section("4.  ONE NAME FOR EVERY FAMILY (C++)", INTERVAL_FLOAT_OVERLOADS)
    text += INTERVAL_FLOAT_TAIL
    return text



# -----------------------------------------------------------------------------
# elementary.h
# -----------------------------------------------------------------------------
# The sine's and cosine's kernels and the arc-tangent's, with the exact sums
# and products that exponential.h's and tangent.h's kernels share, are written
# once, in long double (the header's section 1); each family's square root,
# arc-sine, arc-cosine, sine and cosine follow.

# 2/pi to 704 digits of 24 bits, the reduction's table: computed here, by
# Machin's formula in Python's integers, rather than pasted in, so that the
# header's digits can be traced to their source (the slice's tools check them
# against mpmath).
TWO_OVER_PI_DIGITS = 704


def two_over_pi_bits(count):
    """floor(2/pi 2^count), from pi to count + 64 bits by Machin's formula,
    pi = 16 atan(1/5) - 4 atan(1/239), each series summed in integers with 64
    guard bits; the floor is taken at both ends of pi's error, which must
    agree."""
    bits = count + 64
    guard = 64
    one = 1 << (bits + guard)

    def atan_inv(n):
        total = term = one // n
        k, sign = 1, 1
        while term:
            term //= n * n
            k += 2
            sign = -sign
            total += sign * (term // k)
        return total

    pi = (16 * atan_inv(5) - 4 * atan_inv(239)) >> guard
    low = (1 << (count + 1 + bits)) // (pi + 2)
    high = (1 << (count + 1 + bits)) // (pi - 2)
    assert low == high, "2/pi: a run of equal bits at the cut"
    return low


def two_over_pi_literal():
    """the table as a C string literal, three bytes a digit, 16 bytes a
    line, highest first."""
    data = two_over_pi_bits(24 * TWO_OVER_PI_DIGITS).to_bytes(
        3 * TWO_OVER_PI_DIGITS, "big")
    rows = []
    for i in range(0, len(data), 16):
        rows.append('        "' +
                    "".join("\\x%02X" % b for b in data[i:i + 16]) + '"')
    return "\n".join(rows)


ELEMENTARY_KERNELS = r"""
//==============================================================================
// 1.  THE KERNELS (long double)
//==============================================================================
// What every family's sine, cosine, arc-sine and arc-cosine -- and
// tangent.h's tangent and arc-tangents -- are computed with, once, in long
// double: exact sums and products (which exponential.h's kernels use too),
// 2/pi to as many bits as the widest long double reaches, the reduction of an
// argument by its nearest multiple of pi/2, the sine and cosine of what
// remains, the arc-tangent of the unit interval, and the arc-sine and
// arc-cosine on it; each result kept as an exact pair -- a value and the
// rounding error below it -- so that a family rounds it once. Nothing depends
// on long double's format beyond <float.h>: the text is the same for an x87's
// 64 bits, for a long double that is double, and for a quad's 113 bits.

// the reduction reaches a long double of at most 120 bits whose values lie
// below 2^16384: an x87's, a quad's, and double's
#if ( (LDBL_MANT_DIG > 120) ||                                                \
      (LDBL_MAX_EXP > 16384) )
    #error "elementary.h: no reduction by pi/2 for so wide a long double"
#endif


// 1.1    Exact sums and products
//------------------------------------------------------------------------------

// dj_internal_math_ld_pair
//   struct: a value as an unevaluated sum, high + low, the low part below the
// high part's rounding.
struct dj_internal_math_ld_pair
{
    long double high;
    long double low;
};

// d_internal_math_ld_two_sum
//   _a + _b exactly, as high + low: Knuth's two-sum, which needs no order
// between the operands' magnitudes.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_two_sum(
    long double _a,
    long double _b
)
{
    struct dj_internal_math_ld_pair result = { 0.0L, 0.0L };
    long double                     a_part = 0.0L;
    long double                     b_part = 0.0L;

    result.high = _a + _b;
    a_part      = result.high - _b;
    b_part      = result.high - a_part;
    result.low  = (_a - a_part) + (_b - b_part);

    return result;
}

// d_internal_math_ld_product_exact
//   _a * _b exactly, as high + low: Dekker's product, each factor split by
// Veltkamp's constant 2^ceil(P/2) + 1, P long double's precision
// (float_order.h). The factors must be normal (or zero) and their product far
// from overflow.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_product_exact(
    long double _a,
    long double _b
)
{
    struct dj_internal_math_ld_pair result = { 0.0L, 0.0L };
    const long double split = d_internal_math_ld_veltkamp();
    const long double ta    = split * _a;
    const long double a_hi  = ta - (ta - _a);
    const long double a_lo  = _a - a_hi;
    const long double tb    = split * _b;
    const long double b_hi  = tb - (tb - _b);
    const long double b_lo  = _b - b_hi;

    result.high = _a * _b;
    result.low  = ( (((a_hi * b_hi) - result.high) + (a_hi * b_lo)) +
                    (a_lo * b_hi) ) +
                  (a_lo * b_lo);

    return result;
}

// 1.2    The digits of 2/pi
//------------------------------------------------------------------------------
// An argument past 2^20 quarter turns is reduced with 2/pi's bits from its
// exponent's place to 480 past it, the reduction's columns (section 1.3):
// for the largest value of a 15-bit exponent, an x87's or a quad's, 2/pi's
// first 16,848 bits. Here are @DIGITS@ digits of 24 bits, @BITS@ bits, as a
// string literal, three bytes a digit: a literal's characters can be read in
// a constant expression with no constexpr object to declare, are never copied
// onto the stack, cost nothing to a translation unit that never reaches them,
// and have no linkage for C and C++ to disagree on. The generator computes
// them, by Machin's formula.

// d_internal_math_ld_two_over_pi_digit
//   2/pi's digit _index, base 2^24, from the binary point: 2/pi = the sum of
// digit(j) 2^(-24 (j + 1)) over j >= 0. 0 for a negative _index, 2/pi having
// no integer part.
D_MATH_FN long double
d_internal_math_ld_two_over_pi_digit(
    int _index
)
{
    // 2/pi's first @BITS@ bits, three bytes a digit, highest first
    const char* const bytes =
@TWO_OVER_PI@;

    if ( (_index < 0) ||
         (_index >= @DIGITS@) )
    {
        return 0.0L;
    }

    return ( ((long double)(unsigned char)bytes[3 * _index] * 65536.0L) +
             ((long double)(unsigned char)bytes[(3 * _index) + 1] * 256.0L) ) +
           (long double)(unsigned char)bytes[(3 * _index) + 2];
}

// 1.3    Reduction by pi/2
//------------------------------------------------------------------------------
// x = n pi/2 + r, n the integer nearest x 2/pi and |r| <= pi/4, r carried as
// r + r_lo. Below 2^20 quarter turns, Cody and Waite's: pi/2 in pieces of at
// most 32 bits, written exactly, so that each product with n is exact in any
// format of 53 bits or more, and each difference's error kept. Past them, and
// within 2^-53 of a multiple of pi/2, where those pieces would not reach,
// Payne and Hanek's: x's digits, base 2^24, times as many of 2/pi's as x's
// exponent reaches, in exact columns of 48-bit products; the integer part kept
// only mod 4, the fraction from its first nonzero digit -- the digits before
// it are what x's nearness to a multiple of pi/2 cancelled -- to past twice
// long double's precision, and then multiplied by pi/2. Either way r is
// within 2^-(P+30) of itself, P long double's precision, at every finite x.

// dj_internal_math_ld_reduced
//   struct: an argument reduced by pi/2: x = n pi/2 + (r + r_lo).
struct dj_internal_math_ld_reduced
{
    long double r;         // |r| <= pi/4, rounded once
    long double r_lo;      // the rest, below r's last place
    int         quadrant;  // n mod 4, in [0, 4)
};

// d_internal_math_ld_floor_small
//   the greatest integer at most _v, for 0 <= _v < 2^(P-2): _v rounded to an
// integer by adding and subtracting 1.5 2^(P-1) (LDBL_EPSILON being 2^(1-P)),
// then lowered if that rounded up.
D_MATH_FN long double
d_internal_math_ld_floor_small(
    long double _v
)
{
    const long double magic = 1.5L / LDBL_EPSILON;
    const long double whole = (_v + magic) - magic;

    return (whole > _v) ? (whole - 1.0L)
                        : whole;
}

// d_internal_math_ld_reduce_far
//   _x reduced by pi/2 by Payne and Hanek's method, for |_x| >= 1 and finite.
D_MATH_FN struct dj_internal_math_ld_reduced
d_internal_math_ld_reduce_far(
    long double _x
)
{
    const long double radix   = 16777216.0L;
    const long double ax      = (_x < 0.0L) ? -_x : _x;
    const struct dj_internal_math_ld_binade binade =
        d_internal_math_ld_binade_of(ax);
    // 2^(24 a) <= |x| < 2^(24 (a + 1))
    const int         a       = binade.exponent / 24;
    // pi/2 = pio2_hi + pio2_lo, pio2_hi of 31 bits: exact as written
    const long double pio2_hi = 1.570796326734125614166259765625L;
    const long double pio2_lo = 6.077100506506192601475144209858469968755e-11L;
    long double       digits[6]   = { 0.0L, 0.0L, 0.0L, 0.0L, 0.0L, 0.0L };
    long double       columns[21] = { 0.0L };
    long double       y           = ax * d_internal_math_ld_pow2(-24 * a);
    long double       quadrant    = 0.0L;
    bool              negative    = false;
    int               count       = 0;
    int               first       = 1;
    int               i           = 0;
    int               c           = 0;
    struct dj_internal_math_ld_reduced result = { 0.0L, 0.0L, 0 };

    // x's digits, base 2^24, from 2^(24 a) down: six reach 120 bits
    for (i = 0; ( (i < 6) && (y != 0.0L) ); ++i)
    {
        digits[i] = d_internal_math_ld_floor_small(y);
        y         = (y - digits[i]) * radix;
        count     = i + 1;
    }

    // x 2/pi in columns of weight 2^(-24 c): x's digit i times 2/pi's digit
    // j lands in column i + j + 1 - a, and the columns of weight 2^24 and up,
    // multiples of 4, are left out. A product is below 2^48 and a column below
    // 2^51: every sum exact
    for (c = 0; c <= 20; ++c)
    {
        long double sum = 0.0L;

        for (i = 0; i < count; ++i)
        {
            sum += digits[i] *
                   d_internal_math_ld_two_over_pi_digit(c + a - 1 - i);
        }

        columns[c] = sum;
    }

    // the carries, from the last column up, leave each fraction column a
    // digit
    for (c = 20; c >= 1; --c)
    {
        const long double carry =
            d_internal_math_ld_floor_small(columns[c] / radix);

        columns[c]     -= carry * radix;
        columns[c - 1] += carry;
    }

    // the integer part, mod 4
    quadrant = columns[0] -
               (4.0L * d_internal_math_ld_floor_small(columns[0] / 4.0L));

    // to the nearest integer: a fraction of a half or more is one more turn
    // and, below it, 1 - the fraction, its digits complemented
    if (columns[1] >= (radix / 2.0L))
    {
        negative = true;
        quadrant = (quadrant >= 3.0L) ? 0.0L
                                      : (quadrant + 1.0L);

        for (c = 1; c <= 20; ++c)
        {
            columns[c] = (radix - 1.0L) - columns[c];
        }

        columns[20] += 1.0L;

        for (c = 20; ( (c >= 2) && (columns[c] >= radix) ); --c)
        {
            columns[c]     -= radix;
            columns[c - 1] += 1.0L;
        }
    }

    // the fraction's first nonzero digit
    while ( (first < 20) &&
            (columns[first] == 0.0L) )
    {
        ++first;
    }

    {
        // the fraction as hi + lo, from that digit to past twice long
        // double's precision (fast two-sums: each digit is below the sum)
        const int   last  = ((first + ((2 * LDBL_MANT_DIG) / 24) + 2) < 20)
                                ? (first + ((2 * LDBL_MANT_DIG) / 24) + 2)
                                : 20;
        long double scale = d_internal_math_ld_pow2(-24 * first);
        long double hi    = 0.0L;
        long double lo    = 0.0L;

        for (c = first; c <= last; ++c)
        {
            const long double term = columns[c] * scale;
            const long double sum  = hi + term;

            lo    += term - (sum - hi);
            hi     = sum;
            scale /= radix;
        }

        {
            // renormalized, then times pi/2: f_hi pio2_hi exactly, the rest
            // rounded below it
            const long double f_hi = hi + lo;
            const long double f_lo = lo - (f_hi - hi);
            const struct dj_internal_math_ld_pair head =
                d_internal_math_ld_product_exact(f_hi, pio2_hi);
            const long double rest = head.low +
                                     ((f_hi * pio2_lo) + (f_lo * pio2_hi));

            result.r    = head.high + rest;
            result.r_lo = rest - (result.r - head.high);
        }
    }

    // the signs: the fraction's, then x's, -x being (-n) pi/2 - r
    if (negative != (_x < 0.0L))
    {
        result.r    = -result.r;
        result.r_lo = -result.r_lo;
    }

    if ( (_x < 0.0L) &&
         (quadrant != 0.0L) )
    {
        quadrant = 4.0L - quadrant;
    }

    result.quadrant = (int)quadrant;

    return result;
}

// d_internal_math_ld_reduce
//   _x reduced by pi/2, _x finite: Cody and Waite's way below 2^20 quarter
// turns, Payne and Hanek's past them and within 2^-53 of a multiple of pi/2.
D_MATH_FN struct dj_internal_math_ld_reduced
d_internal_math_ld_reduce(
    long double _x
)
{
    const long double two_over_pi = 0.636619772367581343075535053490057448L;
    const long double turns       = _x * two_over_pi;

    if (!( (turns < 1048576.0L) &&
           (turns > -1048576.0L) ))
    {
        return d_internal_math_ld_reduce_far(_x);
    }

    {
        // pi/2 = pio2_1 + pio2_2 + pio2_3 + pio2_3t, the first three of 31,
        // 32 and 28 bits, written exactly: as decimals of 21 digits they round
        // to the same values in 53 and 64 bits, but not in a quad's 113
        const long double pio2_1   = 1.570796326734125614166259765625L;
        const long double pio2_2   = 2242054355.0L / 36893488147419103232.0L;
        const long double pio2_3   = 160220439.0L /
                                     79228162514264337593543950336.0L;
        const long double pio2_3t  =
            8.478427660368899643958701469386701878e-32L;
        // 1.5 2^(P-1): adding and subtracting it rounds turns to an integer
        const long double magic    = 1.5L / LDBL_EPSILON;
        // 2^-53
        const long double tiny     = 1.0L / 9007199254740992.0L;
        const long double n        = (turns + magic) - magic;
        // x - n pi/2: three exact products, each difference's error kept
        // (two-sum: the first operand is the larger)
        const long double a        = _x - (n * pio2_1);
        const long double b        = n * pio2_2;
        const long double r1       = a - b;
        const long double e1       = (a - r1) - b;
        const long double c        = n * pio2_3;
        const long double r2       = r1 - c;
        const long double e2       = (r1 - r2) - c;
        const long double low      = (e1 + e2) - (n * pio2_3t);
        // the quadrant, n mod 4
        const long double fourths  = n / 4.0L;
        const long double whole    = ((fourths + magic) - magic);
        long double       quadrant = n - (4.0L * whole);
        struct dj_internal_math_ld_reduced result = { 0.0L, 0.0L, 0 };

        // the rounding of n / 4 may land a quadrant of -1 or 4
        quadrant = (quadrant < 0.0L) ? (quadrant + 4.0L)
                                     : ((quadrant >= 4.0L) ? (quadrant - 4.0L)
                                                           : quadrant);

        result.r        = r2 + low;
        result.r_lo     = low - (result.r - r2);
        result.quadrant = (int)quadrant;

        // the pieces hold pi/2 to 2^-(103+P), so n pi/2 to 2^-(83+P), which
        // is 2^-(P+30) of r while |r| is 2^-53 or more; nearer a multiple of
        // pi/2 -- in a quad, where the cancellation can pass 100 bits -- x
        // takes the exact way
        if ( (n != 0.0L) &&
             (result.r < tiny) &&
             (result.r > -tiny) )
        {
            return d_internal_math_ld_reduce_far(_x);
        }

        return result;
    }
}

// 1.4    Sine and cosine of the reduced argument
//------------------------------------------------------------------------------
// sin r and cos r by their Taylor series on [-pi/4, pi/4], sixteen terms past
// a quad's precision, each summed with Kahan's compensation; r_lo enters at
// the end, sin(r + r_lo) = sin r + r_lo cos r and cos(r + r_lo) = cos r -
// r_lo sin r, and each result is kept as a pair, the compensation in its low
// part.

// dj_internal_math_ld_sin_cos
//   struct: the sine and cosine of an argument's remainder after its
// reduction by pi/2, and the quadrant that remainder lies in.
struct dj_internal_math_ld_sin_cos
{
    struct dj_internal_math_ld_pair sine;      // sin(r + r_lo)
    struct dj_internal_math_ld_pair cosine;    // cos(r + r_lo)
    int                             quadrant;  // n mod 4
};

// d_internal_math_ld_sin_cos_of
//   _x reduced by pi/2, and the sine and cosine of what remains; _x finite.
D_MATH_FN struct dj_internal_math_ld_sin_cos
d_internal_math_ld_sin_cos_of(
    long double _x
)
{
    const struct dj_internal_math_ld_reduced reduced =
        d_internal_math_ld_reduce(_x);
    const long double r        = reduced.r;
    const long double r2sq     = r * r;
    long double       sin_sum  = r;
    long double       sin_comp = 0.0L;
    long double       cos_sum  = 1.0L;
    long double       cos_comp = 0.0L;
    long double       sin_term = r;
    long double       cos_term = 1.0L;
    int               k        = 1;
    struct dj_internal_math_ld_sin_cos result =
        { { 0.0L, 0.0L }, { 0.0L, 0.0L }, 0 };

    for (k = 1; k < 17; ++k)
    {
        sin_term = -sin_term * r2sq /
                   (long double)((2 * k) * ((2 * k) + 1));
        cos_term = -cos_term * r2sq /
                   (long double)(((2 * k) - 1) * (2 * k));

        {
            const long double ys = sin_term - sin_comp;
            const long double ts = sin_sum + ys;
            const long double yc = cos_term - cos_comp;
            const long double tc = cos_sum + yc;

            sin_comp = (ts - sin_sum) - ys;
            sin_sum  = ts;
            cos_comp = (tc - cos_sum) - yc;
            cos_sum  = tc;
        }
    }

    // the sums, with r_lo's terms and each compensation (Kahan's sum is the
    // sum less its compensation) below them
    result.sine     = d_internal_math_ld_two_sum(
                          sin_sum, (reduced.r_lo * cos_sum) - sin_comp);
    result.cosine   = d_internal_math_ld_two_sum(
                          cos_sum, -(reduced.r_lo * sin_sum) - cos_comp);
    result.quadrant = reduced.quadrant;

    return result;
}

// d_internal_math_ld_trig
//   sin(_x) when _cosine is false, cos(_x) when it is true, as high + low;
// _x finite.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_trig(
    long double _x,
    bool        _cosine
)
{
    const struct dj_internal_math_ld_sin_cos sc =
        d_internal_math_ld_sin_cos_of(_x);
    // cos x = sin(x + pi/2): a quarter turn more
    const int quadrant = _cosine ? ((sc.quadrant + 1) % 4)
                                 : sc.quadrant;
    // sin r, cos r, -sin r, -cos r, by quadrant
    struct dj_internal_math_ld_pair result =
        ((quadrant % 2) == 0) ? sc.sine : sc.cosine;

    if (quadrant >= 2)
    {
        result.high = -result.high;
        result.low  = -result.low;
    }

    return result;
}

// 1.5    The arc-tangent of the unit interval
//------------------------------------------------------------------------------
// q pi/2 + atan u or q pi/2 - atan u, for u in [0, 1] given as a pair and q
// in {0, 1, 2}: what every arc-tangent, arc-sine and arc-cosine comes to once
// its argument's signs are taken off and the larger magnitude divided into
// the smaller (tangent.h's atan and atan2, section 1.6's asin and acos). u is
// reduced by the nearest of 0, 1/4, 1/2, 3/4 and 1, c: atan u = atan c +
// atan t, t = (u - c) / (1 + c u), |t| <= 1/8, carried as t + t_lo (u - c
// exact by Sterbenz's lemma, the quotient's residual by Dekker's product).
// atan t = t - t^3/3 + t^5/5 - ..., to its 24th term, past a quad's
// precision; atan c and pi/2 are each held as a head of at most 32 bits,
// exact as written, and the rest; and the heads and t are summed exactly --
// q pi/2 + atan c is exact for each q -- so that only the last addition
// rounds, and what it rounds off is kept.

// d_internal_math_ld_ratio
//   (_n_hi + _n_lo) / (_d_hi + _d_lo) as high + low, for 0 <= _n <= _d,
// _d > 0 and finite: the quotient with its residual, the residual's first
// product taken on copies scaled by _d's binade into [1, 2), where Dekker's
// product can neither overflow nor lose its low part. Below 2^(2P) of the
// normal range's bottom the quotient goes alone: its own rounding is then
// the result's.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_ratio(
    long double _n_hi,
    long double _n_lo,
    long double _d_hi,
    long double _d_lo
)
{
    // a subnormal _d is first lifted, with _n, into the normal range
    const long double lift = (_d_hi < LDBL_MIN)
                                 ? d_internal_math_ld_pow2(LDBL_MANT_DIG)
                                 : 1.0L;
    const long double n    = _n_hi * lift;
    const long double d    = _d_hi * lift;
    struct dj_internal_math_ld_pair result = { 0.0L, 0.0L };

    result.high = n / d;

    if (result.high >= d_internal_math_ld_pow2(LDBL_MIN_EXP +
                                               (2 * LDBL_MANT_DIG)))
    {
        const struct dj_internal_math_ld_binade binade =
            d_internal_math_ld_binade_of(d);
        const long double scale = d_internal_math_ld_pow2(-binade.exponent);
        const long double n2    = n * scale;
        const long double d2    = d * scale;
        const struct dj_internal_math_ld_pair p =
            d_internal_math_ld_product_exact(result.high, d2);

        // n - q d: the highs' part exact, the lows' below it
        result.low = (((n2 - p.high) - p.low) / d2) +
                     ((_n_lo - (result.high * _d_lo)) / _d_hi);
    }

    return result;
}

// d_internal_math_ld_atan_reduced
//   _quarters pi/2 + atan(_u), or _quarters pi/2 - atan(_u) when _minus is
// true, as high + low, for _u = _u_hi + _u_lo in [0, 1] and _quarters 0, 1
// or 2.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_atan_reduced(
    long double _u_hi,
    long double _u_lo,
    int         _quarters,
    bool        _minus
)
{
    // pi/2 = pio2_hi + pio2_lo, pio2_hi of 31 bits: exact as written
    const long double pio2_hi = 1.570796326734125614166259765625L;
    const long double pio2_lo = 6.077100506506192601475144209858469968755e-11L;
    // q pi/2, both parts exact multiples
    const long double base_hi = (long double)_quarters * pio2_hi;
    const long double base_lo = (long double)_quarters * pio2_lo;
    const long double sign    = _minus ? -1.0L : 1.0L;
    long double       c       = 0.0L;
    long double       at_hi   = 0.0L;
    long double       at_lo   = 0.0L;

    // c, the nearest of 0, 1/4, 1/2, 3/4 and 1, and atan c = at_hi + at_lo
    if (_u_hi >= 0.875L)
    {
        c     = 1.0L;
        at_hi = 3373259426.0L / 4294967296.0L;
        at_lo = 3.038550253253096300737572104929234984378e-11L;
    }
    else if (_u_hi >= 0.625L)
    {
        c     = 0.75L;
        at_hi = 2763816217.0L / 4294967296.0L;
        at_lo = 4.817137162794350606107263804151059111531e-11L;
    }
    else if (_u_hi >= 0.375L)
    {
        c     = 0.5L;
        at_hi = 3982702635.0L / 8589934592.0L;
        at_lo = 6.299816718559209976839402028537054286120e-12L;
    }
    else if (_u_hi >= 0.125L)
    {
        c     = 0.25L;
        at_hi = 4208701385.0L / 17179869184.0L;
        at_lo = 2.274351222528987867221331091414409838118e-11L;
    }

    {
        // t = (u - c) / (1 + c u): the numerator exact, the denominator as
        // a pair, the quotient with its residual
        const long double num  = _u_hi - c;
        const struct dj_internal_math_ld_pair cu =
            d_internal_math_ld_product_exact(c, _u_hi);
        const struct dj_internal_math_ld_pair den =
            d_internal_math_ld_two_sum(1.0L, cu.high);
        const long double den_lo = den.low + (cu.low + (c * _u_lo));
        const long double t_hi   = num / den.high;
        const struct dj_internal_math_ld_pair td =
            d_internal_math_ld_product_exact(t_hi, den.high);
        const long double t_lo   = ( ((num - td.high) - td.low) +
                                     (_u_lo - (t_hi * den_lo)) ) / den.high;
        const long double z      = t_hi * t_hi;
        long double       h      = 0.0L;
        int               j      = 0;

        // -1/3 + z/5 - z^2/7 + ..., from its 24th term in
        for (j = 24; j >= 1; --j)
        {
            h = ((((j % 2) != 0) ? -1.0L : 1.0L) /
                 (long double)((2 * j) + 1)) +
                (z * h);
        }

        {
            // atan t - t
            const long double tail = (t_hi * z) * h;
            // q pi/2 +- atan c is exact, and so is its sum with +-t; then
            // the small parts below them
            const struct dj_internal_math_ld_pair head =
                d_internal_math_ld_two_sum(base_hi + (sign * at_hi),
                                           sign * t_hi);
            const long double small = head.low +
                                      ( (base_lo + (sign * at_lo)) +
                                        (sign * (t_lo + tail)) );

            return d_internal_math_ld_two_sum(head.high, small);
        }
    }
}

// 1.6    Arc-sine and arc-cosine
//------------------------------------------------------------------------------
// asin x = atan(x / s) and acos x = atan(s / x), s = sqrt(1 - x^2), each with
// the smaller of |x| and s divided into the larger (section 1.5). s comes
// from (1 - |x|)(1 + |x|), each factor and their product kept as pairs, and
// its root's low part from the residual q - s^2, so that s is as precise as
// the rest; then the quotient is a pair too.

// d_internal_math_ld_cathetus
//   sqrt(1 - _a^2) as high + low, for 0 <= _a <= 1: the other leg of a right
// triangle whose hypotenuse is 1 and one leg _a.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_cathetus(
    long double _a
)
{
    // 1 - a and 1 + a, exactly
    const struct dj_internal_math_ld_pair below =
        d_internal_math_ld_two_sum(1.0L, -_a);
    const struct dj_internal_math_ld_pair above =
        d_internal_math_ld_two_sum(1.0L, _a);
    // their product, q = 1 - a^2, as q_hi + q_lo (fast two-sum)
    const struct dj_internal_math_ld_pair p =
        d_internal_math_ld_product_exact(below.high, above.high);
    const long double rest = p.low + ( (below.high * above.low) +
                                       (below.low * above.high) );
    const long double q_hi = p.high + rest;
    const long double q_lo = rest - (q_hi - p.high);
    struct dj_internal_math_ld_pair result = { 0.0L, 0.0L };

    // q is 0 only at a = 1; elsewhere its correctly rounded root, and the
    // residual's share below it
    if (q_hi > 0.0L)
    {
        const long double root = d_math_ld_sqrt_c(q_hi);
        const struct dj_internal_math_ld_pair square =
            d_internal_math_ld_product_exact(root, root);

        result.high = root;
        result.low  = (((q_hi - square.high) - square.low) + q_lo) /
                      (2.0L * root);
    }

    return result;
}

// d_internal_math_ld_asin_of
//   asin(_x) as high + low, for -1 <= _x <= 1.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_asin_of(
    long double _x
)
{
    const long double ax = (_x < 0.0L) ? -_x : _x;
    const struct dj_internal_math_ld_pair s = d_internal_math_ld_cathetus(ax);
    struct dj_internal_math_ld_pair result = { 0.0L, 0.0L };

    // up to 1/sqrt(2): atan(|x| / s); past it pi/2 - atan(s / |x|)
    if (ax <= s.high)
    {
        const struct dj_internal_math_ld_pair u =
            d_internal_math_ld_ratio(ax, 0.0L, s.high, s.low);

        result = d_internal_math_ld_atan_reduced(u.high, u.low, 0, false);
    }
    else
    {
        const struct dj_internal_math_ld_pair u =
            d_internal_math_ld_ratio(s.high, s.low, ax, 0.0L);

        result = d_internal_math_ld_atan_reduced(u.high, u.low, 1, true);
    }

    // asin(-x) = -asin x
    if (_x < 0.0L)
    {
        result.high = -result.high;
        result.low  = -result.low;
    }

    return result;
}

// d_internal_math_ld_acos_of
//   acos(_x) as high + low, for -1 <= _x <= 1.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_acos_of(
    long double _x
)
{
    const long double ax = (_x < 0.0L) ? -_x : _x;
    const struct dj_internal_math_ld_pair s = d_internal_math_ld_cathetus(ax);

    // from 1/sqrt(2) out: atan(s / |x|), and pi less it below 0
    if (s.high <= ax)
    {
        const struct dj_internal_math_ld_pair u =
            d_internal_math_ld_ratio(s.high, s.low, ax, 0.0L);

        return (_x > 0.0L) ? d_internal_math_ld_atan_reduced(u.high, u.low,
                                                             0, false)
                           : d_internal_math_ld_atan_reduced(u.high, u.low,
                                                             2, true);
    }

    // within it: pi/2 - atan(x / s), and pi/2 + atan(|x| / s) below 0
    {
        const struct dj_internal_math_ld_pair u =
            d_internal_math_ld_ratio(ax, 0.0L, s.high, s.low);

        return (_x < 0.0L) ? d_internal_math_ld_atan_reduced(u.high, u.low,
                                                             1, false)
                           : d_internal_math_ld_atan_reduced(u.high, u.low,
                                                             1, true);
    }
}
"""

# the pair each family's square root settles its last bit with; the long
# double family's is section 1's, which the kernels use too
ELEMENTARY_PAIR = r"""// dj_internal_math_@S@_pair
//   struct: a value as an unevaluated sum, high + low, the low part below the
// high part's rounding.
struct dj_internal_math_@S@_pair
{
    @T@ high;
    @T@ low;
};
"""

ELEMENTARY_PAIR_LD = r"""// dj_internal_math_ld_pair is section 1's (1.1), which the kernels share.
"""

ELEMENTARY_TEMPLATE = r"""
//==============================================================================
// @SECTION@.  THE @UPPER@ FAMILY (_@S@)
//==============================================================================


// @SECTION@.1    Internal helpers
//------------------------------------------------------------------------------

@PAIR@
// d_internal_math_@S@_nan
//   a quiet NaN: a constant expression where the build defines NAN.
D_MATH_FN @T@
d_internal_math_@S@_nan(void)
{
#if defined(NAN)
    return (@T@)NAN;
#else
    return (@T@)(HUGE_VAL - HUGE_VAL);
#endif
}

// d_internal_math_@S@_round_pair
//   a kernel's _hi + _lo rounded once to the family's type: _hi is the sum
// rounded to long double, and converting it rounds again, which is the same
// rounding but where _hi lies exactly between two values of the type -- the
// first rounding having landed on the midpoint, as it does for log(1 - 2^-52)
// and exp(2^-53) -- and then _lo's sign decides. A finite _hi past the
// type's range is an infinity, but for the midpoint above the largest finite
// value with the sum below it, which rounds there. A long double is _hi.
D_MATH_FN @T@
d_internal_math_@S@_round_pair(
    long double _hi,
    long double _lo
)
{
    const @T@ near = (@T@)_hi;
    // what the second rounding moved, exactly: at most half a gap
    const long double rest = _hi - (long double)near;

@OVERFLOW{@
    // a finite _hi the conversion took past the largest finite value
    if ( ( (near > @MAX@) ||
           (near < -@MAX@) ) &&
         (_hi <= LDBL_MAX) &&
         (_hi >= -LDBL_MAX) )
    {
        const long double gap = (long double)@MAX@ -
                                (long double)d_math_@S@_next_down(@MAX@);
        const long double mid = (near > (@T@)0) ?
                                    ((long double)@MAX@ + (gap / 2.0L)) :
                                    (-(long double)@MAX@ - (gap / 2.0L));

        // the midpoint above it, the sum below: the largest finite value
        if ( (_hi == mid) &&
             ( (near > (@T@)0) ? (_lo < 0.0L) : (_lo > 0.0L) ) )
        {
            return (near > (@T@)0) ? @MAX@ : -@MAX@;
        }

        return near;
    }
@}OVERFLOW@

    // _hi rounded down to near, from the midpoint, and the sum lies above it
    if ( (rest > 0.0L) &&
         (_lo > 0.0L) )
    {
        const @T@ up = d_math_@S@_next_up(near);

        if ((rest + rest) == ((long double)up - (long double)near))
        {
            return up;
        }
    }

    // _hi rounded up to near, from the midpoint, and the sum lies below it
    if ( (rest < 0.0L) &&
         (_lo < 0.0L) )
    {
        const @T@ down = d_math_@S@_next_down(near);

        if ((rest + rest) == ((long double)down - (long double)near))
        {
            return down;
        }
    }

    return near;
}

// d_internal_math_@S@_abs
//   |_x| by comparison, as the C++ headers took it (a -0 stays -0); a
// constant expression, where fabs is not.
D_MATH_FN @T@
d_internal_math_@S@_abs(
    @T@ _x
)
{
    return (_x < (@T@)0) ? -_x : _x;
}

// d_internal_math_@S@_square_exact
//   _c * _c exactly, as high + low: Dekker's product, the factors split by
// Veltkamp's constant 2^ceil(P/2) + 1 (float_order.h). _c must be normal and
// its square finite.
D_MATH_FN struct dj_internal_math_@S@_pair
d_internal_math_@S@_square_exact(
    @T@ _c
)
{
    struct dj_internal_math_@S@_pair result = { (@T@)0, (@T@)0 };
    const @T@ split = d_internal_math_@S@_veltkamp();
    const @T@ t     = split * _c;
    const @T@ hi    = t - (t - _c);
    const @T@ lo    = _c - hi;

    result.high = _c * _c;
    result.low  = (((hi * hi) - result.high) + (((@T@)2 * hi) * lo)) +
                  (lo * lo);

    return result;
}

// dj_internal_math_@S@_wide
//   struct: an unsigned integer of two dj_math_umax limbs, high * 2^bits + low.
struct dj_internal_math_@S@_wide
{
    dj_math_umax high;
    dj_math_umax low;
};

// d_internal_math_@S@_limb_bits
//   the bits of dj_math_umax.
D_MATH_FN int
d_internal_math_@S@_limb_bits(void)
{
    return (int)(sizeof(dj_math_umax) * 8u);
}

// d_internal_math_@S@_wide_shift
//   _value * 2^_shift as two limbs, for 0 < _shift < the limb's bits.
D_MATH_FN struct dj_internal_math_@S@_wide
d_internal_math_@S@_wide_shift(
    dj_math_umax _value,
    int          _shift
)
{
    const int bits = d_internal_math_@S@_limb_bits();
    struct dj_internal_math_@S@_wide result = { 0u, 0u };

    result.high = _value >> ((bits - _shift) % bits);
    result.low  = _value << (_shift % bits);

    return result;
}

// d_internal_math_@S@_wide_square
//   _a * _a as two limbs: _a split into half-limbs, so each partial product
// fits one limb, and the cross term added twice with its carries.
D_MATH_FN struct dj_internal_math_@S@_wide
d_internal_math_@S@_wide_square(
    dj_math_umax _a
)
{
    const int          half  = d_internal_math_@S@_limb_bits() / 2;
    const dj_math_umax mask  = (((dj_math_umax)1) << half) - 1u;
    const dj_math_umax ah    = _a >> half;
    const dj_math_umax al    = _a & mask;
    const dj_math_umax cross = ah * al;
    const dj_math_umax lo_in = cross << half;
    const dj_math_umax hi_in = cross >> half;
    struct dj_internal_math_@S@_wide result = { 0u, 0u };
    int pass = 0;

    result.high = ah * ah;
    result.low  = al * al;

    // the cross term twice, each time with the carry out of the low limb
    for (pass = 0; pass < 2; ++pass)
    {
        result.low  += lo_in;
        result.high += hi_in + ((result.low < lo_in) ? 1u : 0u);
    }

    return result;
}

// d_internal_math_@S@_wide_less
//   whether _a < _b.
D_MATH_FN bool
d_internal_math_@S@_wide_less(
    struct dj_internal_math_@S@_wide _a,
    struct dj_internal_math_@S@_wide _b
)
{
    return ( (_a.high < _b.high) ||
             ( (_a.high == _b.high) &&
               (_a.low < _b.low) ) );
}

// d_internal_math_@S@_sqrt_settle
//   the last bit of the root of _m in [1, 4), settled in integers: with S =
// 2^(P-1), the root is R / S for an integer R, the midpoint above it is
// (2R + 1) / 2S, and sqrt(m) passes it exactly when 4 S M > (2R + 1)^2, M =
// m S being an integer too. Integer arithmetic is the same under every
// floating-point evaluation method -- an x87's excess precision, which
// breaks Dekker's product, included -- and in constant evaluation.
D_MATH_FN @T@
d_internal_math_@S@_sqrt_settle(
    @T@ _m,
    @T@ _root
)
{
    const @T@          scale = d_internal_math_@S@_pow2(@P@ - 1);
    const dj_math_umax s     = (dj_math_umax)scale;
    const dj_math_umax m     = (dj_math_umax)(_m * scale);
    const struct dj_internal_math_@S@_wide four_s_m =
        d_internal_math_@S@_wide_shift(m, @P@ + 1);
    dj_math_umax      r     = (_root >= (@T@)2) ? ((2u * s) - 1u)
                                                : (dj_math_umax)(_root * scale);
    int               tries = 0;

    // up while the root lies past the midpoint above...
    for (tries = 0;
         ( (tries < 4) &&
           (d_internal_math_@S@_wide_less(
                d_internal_math_@S@_wide_square((2u * r) + 1u),
                four_s_m)) );
         ++tries)
    {
        ++r;
    }

    // ...down while it lies short of the midpoint below (from 1 it cannot)
    for (tries = 0;
         ( (tries < 4) &&
           (r > s) &&
           (d_internal_math_@S@_wide_less(
                four_s_m,
                d_internal_math_@S@_wide_square((2u * r) - 1u))) );
         ++tries)
    {
        --r;
    }

    return ((@T@)r / scale);
}

// dj_internal_math_@S@_digits
//   struct: a whole number in digits of 16 bits, the least first, each held
// in a dj_math_umax: a digit times a digit, with a digit and a carry added,
// stays below 2^32, which the narrowest dj_math_umax holds. Room for numbers
// below 2^(2P+4): sqrt_settle_digits's, where two limbs do not hold them --
// S M, the largest, lies below 2^(2P).
struct dj_internal_math_@S@_digits
{
    dj_math_umax digit[2 * ((@P@ + 17) / 16)];
};

// d_internal_math_@S@_digit_count
//   the digits a dj_internal_math_@S@_digits holds.
D_MATH_FN int
d_internal_math_@S@_digit_count(void)
{
    return (2 * ((@P@ + 17) / 16));
}

// d_internal_math_@S@_digits_of
//   the whole part of _value, from 0 to below 2^64, in digits: its two
// 32-bit halves first, each a whole number that converts exactly, whatever
// precision the build evaluates the family in.
D_MATH_FN struct dj_internal_math_@S@_digits
d_internal_math_@S@_digits_of(
    @T@ _value
)
{
    const @DG_T@ word = (@T@)4294967296.0;  // 2^32
    const @DG_UMAX@ high = (dj_math_umax)(_value / word);
    const @DG_UMAX@ low  =
        (dj_math_umax)(_value - ((@T@)high * word));
    struct dj_internal_math_@S@_digits result = { { 0u } };

    result.digit[0] = low & 0xFFFFu;
    result.digit[1] = (low >> 16) & 0xFFFFu;
    result.digit[2] = high & 0xFFFFu;
    result.digit[3] = (high >> 16) & 0xFFFFu;

    return result;
}

// d_internal_math_@S@_digits_value
//   _a as a value of the family, from the most significant digit down:
// exact while _a lies below 2^P, as every partial sum does then.
D_MATH_FN @T@
d_internal_math_@S@_digits_value(
    struct dj_internal_math_@S@_digits _a
)
{
    @T@ result = (@T@)0;
    @DG_INT@ k      = 0;

    for (k = d_internal_math_@S@_digit_count() - 1; k >= 0; --k)
    {
        result = (result * (@T@)65536) + (@T@)_a.digit[k];
    }

    return result;
}

// d_internal_math_@S@_digits_step
//   _a + 1 where _up, and else _a - 1, _a above 0: a carry or a borrow taken
// up the digits.
D_MATH_FN struct dj_internal_math_@S@_digits
d_internal_math_@S@_digits_step(
    struct dj_internal_math_@S@_digits _a,
    @DG_BOOL@ _up
)
{
    struct dj_internal_math_@S@_digits result = _a;
    int k = 0;

    // a digit that can take the step takes it, and ends the carry or the
    // borrow; one that cannot wraps, and passes it on
    for (k = 0; k < d_internal_math_@S@_digit_count(); ++k)
    {
        if (_up)
        {
            if (result.digit[k] < 0xFFFFu)
            {
                ++result.digit[k];

                break;
            }

            result.digit[k] = 0u;
        }
        else
        {
            if (result.digit[k] > 0u)
            {
                --result.digit[k];

                break;
            }

            result.digit[k] = 0xFFFFu;
        }
    }

    return result;
}

// d_internal_math_@S@_digits_shift
//   _a * 2^_bits: each digit moved up _bits / 16 places and _bits % 16 bits,
// what passes 16 bits carried into the next. The product must fit.
D_MATH_FN struct dj_internal_math_@S@_digits
d_internal_math_@S@_digits_shift(
    struct dj_internal_math_@S@_digits _a,
    @DG_INTP@ _bits
)
{
    const int    whole = _bits / 16;
    const int    part  = _bits % 16;
    dj_math_umax carry = 0u;
    int          k     = 0;
    struct dj_internal_math_@S@_digits result = { { 0u } };

    for (k = 0; (k + whole) < d_internal_math_@S@_digit_count(); ++k)
    {
        const dj_math_umax moved = (_a.digit[k] << part) | carry;

        result.digit[k + whole] = moved & 0xFFFFu;
        carry                   = moved >> 16;
    }

    return result;
}

// d_internal_math_@S@_low_digit_count
//   the digits, the lowest, that sqrt_settle_digits takes its residual in:
// P + 9 bits or more, so that the residual, below 2^(P+4) in size, keeps its
// sign in their top bit.
D_MATH_FN int
d_internal_math_@S@_low_digit_count(void)
{
    return ((@P@ + 24) / 16);
}

// d_internal_math_@S@_digits_low_add
//   _a + _b where _up, and else _a - _b, in the low digits alone, modulo
// 2^16 to their count: a digit at a time, the carry or the borrow taken into
// the next, and the one out of the top dropped. The digits above are 0.
D_MATH_FN struct dj_internal_math_@S@_digits
d_internal_math_@S@_digits_low_add(
    struct dj_internal_math_@S@_digits _a,
    struct dj_internal_math_@S@_digits _b,
    @DG_BOOL@ _up
)
{
    dj_math_umax carry = 0u;
    int          k     = 0;
    struct dj_internal_math_@S@_digits result = { { 0u } };

    for (k = 0; k < d_internal_math_@S@_low_digit_count(); ++k)
    {
        if (_up)
        {
            const dj_math_umax sum = (_a.digit[k] + _b.digit[k]) + carry;

            result.digit[k] = sum & 0xFFFFu;
            carry           = sum >> 16;
        }
        else
        {
            // the digit above lends 2^16, and the borrow pays it back
            const dj_math_umax difference =
                ((_a.digit[k] + 0x10000u) - _b.digit[k]) - carry;

            result.digit[k] = difference & 0xFFFFu;
            carry           = 1u - (difference >> 16);
        }
    }

    return result;
}

// d_internal_math_@S@_digits_low_square
//   _a * _a in the low digits alone, modulo 2^16 to their count: only the
// products of two digits that land there, each added in with the digit
// already there and the carry, which stays below 2^32; the carry out of the
// top dropped.
D_MATH_FN struct dj_internal_math_@S@_digits
d_internal_math_@S@_digits_low_square(
    struct dj_internal_math_@S@_digits _a
)
{
    const int low = d_internal_math_@S@_low_digit_count();
    int       i   = 0;
    int       j   = 0;
    struct dj_internal_math_@S@_digits result = { { 0u } };

    for (i = 0; i < low; ++i)
    {
        dj_math_umax carry = 0u;

        for (j = 0; (i + j) < low; ++j)
        {
            const dj_math_umax sum = result.digit[i + j] +
                                     (_a.digit[i] * _a.digit[j]) + carry;

            result.digit[i + j] = sum & 0xFFFFu;
            carry               = sum >> 16;
        }
    }

    return result;
}

// d_internal_math_@S@_digits_negative
//   whether the low digits, read in two's complement, are below 0: the top
// bit of the top one.
D_MATH_FN bool
d_internal_math_@S@_digits_negative(
    struct dj_internal_math_@S@_digits _a
)
{
    return ((_a.digit[d_internal_math_@S@_low_digit_count() - 1] >> 15) != 0u);
}

// d_internal_math_@S@_digits_less
//   whether _a < _b: the most significant digit that differs decides.
D_MATH_FN bool
d_internal_math_@S@_digits_less(
    struct dj_internal_math_@S@_digits _a,
    struct dj_internal_math_@S@_digits _b
)
{
    int k = 0;

    for (k = d_internal_math_@S@_digit_count() - 1; k >= 0; --k)
    {
        if (_a.digit[k] != _b.digit[k])
        {
            return (_a.digit[k] < _b.digit[k]);
        }
    }

    return false;
}

// d_internal_math_@S@_sqrt_settle_digits
//   sqrt_settle where two limbs do not hold (2^(P+1))^2 -- a double where
// dj_math_umax has 32 bits -- in digits of 16 bits instead, exact in
// integers, so that nothing depends on how the build evaluates floating
// point. R / S is the rounded root when (2R - 1)^2 < 4 S M < (2R + 1)^2,
// that is when the residual G = S M - R^2 + R - 1 lies in [0, 2R): R is
// short where G - 2R, G once R steps up, is 0 or more, and past where G is
// below 0; a step down adds 2R, R the new one. While R lies within four
// steps of the root's, G is below 2^(P+4) in size, so it is taken in the low
// digits alone, whose top bit keeps its sign: R is squared once, and only as
// far as they reach. R starts from the root's whole part, which a root held
// in a wider register than the family's leaves within a step or two of it.
// For a precision P up to 63, so that M and R lie below 2^64.
D_MATH_FN @T@
d_internal_math_@S@_sqrt_settle_digits(
    @T@ _m,
    @T@ _root
)
{
    const @T@ scale = d_internal_math_@S@_pow2(@P@ - 1);
    const struct dj_internal_math_@S@_digits s =
        d_internal_math_@S@_digits_of(scale);
    // R from the root's whole part in units of 1 / S, below 2S
    const @T@ top = ((@T@)2 * scale) - (@T@)1;
    struct dj_internal_math_@S@_digits r =
        d_internal_math_@S@_digits_of((_root >= (@T@)2) ? top
                                                        : (_root * scale));
    // G = S M - R^2 + (R - 1), M = m S
    struct dj_internal_math_@S@_digits g =
        d_internal_math_@S@_digits_low_add(
            d_internal_math_@S@_digits_low_add(
                d_internal_math_@S@_digits_shift(
                    d_internal_math_@S@_digits_of(_m * scale),
                    @P@ - 1),
                d_internal_math_@S@_digits_low_square(r),
                false),
            d_internal_math_@S@_digits_step(r, false),
            true);
    // G - 2R
    struct dj_internal_math_@S@_digits next =
        d_internal_math_@S@_digits_low_add(
            g,
            d_internal_math_@S@_digits_low_add(r, r, true),
            false);
    int tries = 0;

    // up while the root lies past the midpoint above...
    for (tries = 0;
         ( (tries < 4) &&
           (!d_internal_math_@S@_digits_negative(next)) );
         ++tries)
    {
        g    = next;
        r    = d_internal_math_@S@_digits_step(r, true);
        next = d_internal_math_@S@_digits_low_add(
                   g,
                   d_internal_math_@S@_digits_low_add(r, r, true),
                   false);
    }

    // ...down while it lies short of the midpoint below (from 1 it cannot)
    for (tries = 0;
         ( (tries < 4) &&
           (d_internal_math_@S@_digits_negative(g)) &&
           (d_internal_math_@S@_digits_less(s, r)) );
         ++tries)
    {
        r = d_internal_math_@S@_digits_step(r, false);
        g = d_internal_math_@S@_digits_low_add(
                g,
                d_internal_math_@S@_digits_low_add(r, r, true),
                true);
    }

    return (d_internal_math_@S@_digits_value(r) / scale);
}

// d_internal_math_@S@_sqrt_unit
//   the correctly rounded square root of _m in [1, 4): Newton's iteration
// from a line within 6% of it -- six steps pass any precision to a quad's --
// then the last bit settled. Where two limbs hold (2^(P+1))^2 -- float, and
// double with a 64-bit dj_math_umax -- it is settled in integers
// (sqrt_settle), exact however the build evaluates floating point. Where
// they do not and P is below 64 -- a double where dj_math_umax has 32 bits,
// which an x87 holds in registers wider than a double -- in integers too, in
// digits of 16 bits (sqrt_settle_digits). Otherwise -- a long double of 64
// bits or more -- by the residual m - root^2, taken exactly by Dekker's
// product, against the midpoints with the root's neighbours: arithmetic
// that is its own evaluation format, so that no excess precision intervenes.
D_MATH_FN @T@
d_internal_math_@S@_sqrt_unit(
    @T@ _m
)
{
    @T@ root  = (@T@)0.6 + ((@T@)0.4 * _m);
    int i     = 0;
    int tries = 0;

    // Newton's iteration
    for (i = 0; i < 6; ++i)
    {
        root = (@T@)0.5 * (root + (_m / root));
    }

    // the last bit, in integers where they reach
    if (((2 * @P@) + 2) <= (2 * d_internal_math_@S@_limb_bits()))
    {
        return d_internal_math_@S@_sqrt_settle(_m, root);
    }

    // in digits where they do not, for a precision below 64: Dekker's
    // product, below, might be evaluated wider than the family -- on the
    // x87, by clang even where long double is a double
    if (@P@ < 64)
    {
        return d_internal_math_@S@_sqrt_settle_digits(_m, root);
    }

    // the last bit: up past the upper midpoint, down past the lower one
    for (tries = 0; tries < 3; ++tries)
    {
        const struct dj_internal_math_@S@_pair square =
            d_internal_math_@S@_square_exact(root);
        const @T@ residual = (_m - square.high) - square.low;
        const struct dj_internal_math_@S@_binade b =
            d_internal_math_@S@_binade_of(root);
        const @T@ above = b.power * @EPS@;
        const @T@ below = (root == b.power) ? (above / (@T@)2) : above;

        if ((residual - (root * above)) > ((above * above) / (@T@)4))
        {
            root = d_math_@S@_next_up(root);
        }
        else if ((residual + (root * below)) < ((below * below) / (@T@)4))
        {
            root = d_math_@S@_next_down(root);
        }
        else
        {
            break;
        }
    }

    return root;
}

// @SECTION@.2    Square root
//------------------------------------------------------------------------------
// sqrt_c is the correctly rounded square root in pure arithmetic, so the same
// text is a constant expression from C++14, and it agrees with sqrt, the C
// library's (correctly rounded too, by IEEE 754), bit for bit: a face may use
// either and not know which ran. Where the target evaluates the family wider
// than its type -- float and double on a 32-bit x87 -- the library's root can
// come back wider too (glibc's sqrtf leaves the x87's 64 bits), and sqrt
// stores it, so that it is the family's, as sqrt_c's is. -0 stays -0, a
// negative or NaN gives NaN, +inf stays +inf.

D_MATH_FN @T@
d_math_@S@_sqrt_c(
    @T@ _x
)
{
    // NaN, zero (of either sign) and +inf are their own roots
    if ( (_x != _x) ||
         (_x == (@T@)0) ||
         (_x > @MAX@) )
    {
        return _x;
    }

    // a negative value has none
    if (_x < (@T@)0)
    {
        return d_internal_math_@S@_nan();
    }

    {
        // a subnormal is first scaled into the normal range by an even power
        const int lift  = (_x < @MIN@) ? (((@P@ + 1) / 2) * 2) : 0;
        const @T@ value = (lift > 0) ? (_x * d_internal_math_@S@_pow2(lift))
                                     : _x;
        const struct dj_internal_math_@S@_binade b =
            d_internal_math_@S@_binade_of(value);
        // value = m * 4^k with m in [1, 4); k = floor(E / 2)
        const int k = (b.exponent >= 0) ? (b.exponent / 2)
                                        : -((1 - b.exponent) / 2);
        const @T@ m = value / d_internal_math_@S@_pow2(2 * k);

        return ( d_internal_math_@S@_sqrt_unit(m) *
                 d_internal_math_@S@_pow2(k - (lift / 2)) );
    }
}

D_MATH_FN_RT @T@
d_math_@S@_sqrt(
    @T@ _x
)
{
#ifdef __cplusplus
    const @T@ root = std::sqrt(_x);
#else
    const @T@ root = @CSQRT@(_x);
#endif  // __cplusplus

    // where the target evaluates the family wider than its type, stored, so
    // that it is a @NAME@: a compiler takes a call's result to be of its
    // type, and keeps what the library left in the register
    if (@EVAL_WIDE@)
    {
        volatile @T@ stored = root;

        return stored;
    }

    return root;
}

// @SECTION@.3    Arc-sine and arc-cosine
//------------------------------------------------------------------------------
// asin_c and acos_c are pure arithmetic, constant expressions from C++14: the
// kernels' pairs (section 1.6) rounded once to the family -- the correctly
// rounded value but where the exact one lies within the kernels' error of a
// midpoint, and so within an ulp of asin and acos, which are the C library's.
// A value outside [-1, 1], or NaN, gives NaN, as the library's do; asin keeps
// a zero's sign, and acos(1) is +0.

D_MATH_FN @T@
d_math_@S@_asin_c(
    @T@ _x
)
{
    // NaN, and zero of either sign, are their own arc-sines
    if ( (_x != _x) ||
         (_x == (@T@)0) )
    {
        return _x;
    }

    // outside the domain
    if (!( (_x >= (@T@)-1) &&
           (_x <= (@T@)1) ))
    {
        return d_internal_math_@S@_nan();
    }

    {
        const struct dj_internal_math_ld_pair result =
            d_internal_math_ld_asin_of((long double)_x);

        return d_internal_math_@S@_round_pair(result.high, result.low);
    }
}

D_MATH_FN_RT @T@
d_math_@S@_asin(
    @T@ _x
)
{
#ifdef __cplusplus
    return std::asin(_x);
#else
    return @CASIN@(_x);
#endif
}

D_MATH_FN @T@
d_math_@S@_acos_c(
    @T@ _x
)
{
    // outside the domain, and NaN
    if (!( (_x >= (@T@)-1) &&
           (_x <= (@T@)1) ))
    {
        return d_internal_math_@S@_nan();
    }

    {
        const struct dj_internal_math_ld_pair result =
            d_internal_math_ld_acos_of((long double)_x);

        return d_internal_math_@S@_round_pair(result.high, result.low);
    }
}

D_MATH_FN_RT @T@
d_math_@S@_acos(
    @T@ _x
)
{
#ifdef __cplusplus
    return std::acos(_x);
#else
    return @CACOS@(_x);
#endif
}

// @SECTION@.4    Sine and cosine
//------------------------------------------------------------------------------
// sin_c and cos_c are pure arithmetic, constant expressions from C++14, at
// every finite argument: the kernel's pair (section 1.4) -- the argument
// reduced exactly, however large -- rounded once to the family: the correctly
// rounded value but where the exact one lies within the kernel's error of a
// midpoint, and so within an ulp of sin and cos, which are the C library's.
// NaN and an infinity give NaN; -0 stays -0 for sin.

D_MATH_FN @T@
d_math_@S@_sin_c(
    @T@ _x
)
{
    // NaN and the infinities
    if (!( (_x <= @MAX@) &&
           (_x >= -@MAX@) ))
    {
        return d_internal_math_@S@_nan();
    }

    // zero, of either sign, is its own sine
    if (_x == (@T@)0)
    {
        return _x;
    }

    {
        const struct dj_internal_math_ld_pair result =
            d_internal_math_ld_trig((long double)_x, false);

        return d_internal_math_@S@_round_pair(result.high, result.low);
    }
}

D_MATH_FN @T@
d_math_@S@_cos_c(
    @T@ _x
)
{
    // NaN and the infinities
    if (!( (_x <= @MAX@) &&
           (_x >= -@MAX@) ))
    {
        return d_internal_math_@S@_nan();
    }

    {
        const struct dj_internal_math_ld_pair result =
            d_internal_math_ld_trig((long double)_x, true);

        return d_internal_math_@S@_round_pair(result.high, result.low);
    }
}

D_MATH_FN_RT @T@
d_math_@S@_sin(
    @T@ _x
)
{
#ifdef __cplusplus
    return std::sin(_x);
#else
    return @CSIN@(_x);
#endif
}

D_MATH_FN_RT @T@
d_math_@S@_cos(
    @T@ _x
)
{
#ifdef __cplusplus
    return std::cos(_x);
#else
    return @CCOS@(_x);
#endif
}
"""

ELEMENTARY_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE KERNELS (long double)
    -------------------------
    1.  Exact sums and products
    2.  The digits of 2/pi
    3.  Reduction by pi/2
    4.  Sine and cosine of the reduced argument
    5.  The arc-tangent of the unit interval
    6.  Arc-sine and arc-cosine
2.  THE FLOAT FAMILY (_f)
    ---------------------
    1.  Internal helpers
    2.  Square root
    3.  Arc-sine and arc-cosine
    4.  Sine and cosine
3.  THE DOUBLE FAMILY (_d)
    ----------------------
    1.  Internal helpers
    2.  Square root
    3.  Arc-sine and arc-cosine
    4.  Sine and cosine
4.  THE LONG DOUBLE FAMILY (_ld)
    ----------------------------
    1.  Internal helpers
    2.  Square root
    3.  Arc-sine and arc-cosine
    4.  Sine and cosine
5.  ONE NAME FOR EVERY FAMILY (C++)
    -------------------------------
*/

#ifndef DJINTERP_C_MATH_ELEMENTARY_H
#define DJINTERP_C_MATH_ELEMENTARY_H 1

// std
#ifdef __cplusplus
    #include <cmath>         // std::sqrt, std::asin, std::acos, std::sin, ...
#endif  // __cplusplus
#include <float.h>           // LDBL_MANT_DIG, LDBL_MAX_EXP, LDBL_EPSILON
#include <math.h>            // sqrtf ... cosl, HUGE_VAL, NAN
// djinterp
#include "./float_order.h"   // d_math_*_next_up, d_internal_math_*_binade_of
#include "./math_common.h"   // D_MATH_FN


// the arc-sine's and arc-cosine's kernel (section 1.6) takes its root in
// long double, the long double family's (section 4.2)
D_MATH_FN long double
d_math_ld_sqrt_c(
    long double _x
);
"""

ELEMENTARY_TAIL = r"""

#endif  // DJINTERP_C_MATH_ELEMENTARY_H
"""

ELEMENTARY_OVERLOADS = [
    ("D_MATH_FN", "@T@", "d_math_sqrt_c", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_sqrt", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_asin_c", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_asin", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_acos_c", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_acos", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_sin_c", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_sin", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_cos_c", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_cos", [("@T@", "_x")]),
]


def elementary():
    lines = [
        "Elementary functions: the C core's square root, arc-sine, arc-cosine,",
        "sine and cosine, for float, double and long double, and the long double",
        "kernels its other floating-point headers share.",
        "  GENERATED by scripts/gen_c_math_float.py from one text for the three",
        "families: edit the generator, not this file.",
        "",
        "  Each function comes twice. The _c form is pure arithmetic, so the same",
        "text is a constant expression from C++14; the plain form calls the C",
        "library (std:: in C++, which has the float and long double overloads from",
        "C++98), fast at run time. sqrt_c is correctly rounded, so the two square",
        "roots agree bit for bit. asin_c, acos_c, sin_c and cos_c are computed",
        "once, in long double, as exact pairs -- the sine's argument reduced by",
        "pi/2 exactly, however large -- and rounded once to the family, within an",
        "ulp of the library's. A face picks one by level -- from C++20 by",
        "std::is_constant_evaluated -- and, for the square root, cannot be seen to",
        "have picked.",
    ]
    text = banner("elementary.h", lines, DATE, "2026.11.07")
    text += "\n" + ELEMENTARY_HEAD
    text += ELEMENTARY_KERNELS.replace("@TWO_OVER_PI@", two_over_pi_literal()) \
                              .replace("@DIGITS@", str(TWO_OVER_PI_DIGITS)) \
                              .replace("@BITS@", "{:,}".format(24 * TWO_OVER_PI_DIGITS))
    for i, (suffix, ctype, prefix, prose) in enumerate(FAMILIES):
        upper = prose.upper()
        body = ELEMENTARY_TEMPLATE.replace("@SECTION@", str(i + 2)).replace("@UPPER@", upper)
        body = body.replace("@PAIR@", ELEMENTARY_PAIR_LD if suffix == "ld" else ELEMENTARY_PAIR)
        # round_pair's overflow: a long double is _hi itself, never past its
        # range where _hi is not, so its family has no such branch
        if suffix == "ld":
            body = re.sub(r"@OVERFLOW\{@\n.*?@\}OVERFLOW@\n\n?", "", body,
                          flags=re.S)
        else:
            body = body.replace("@OVERFLOW{@\n", "").replace("@}OVERFLOW@\n", "")
        body = body.replace("@CSQRT@", {"float": "sqrtf", "double": "sqrt", "long double": "sqrtl"}[ctype])
        body = body.replace("@CASIN@", {"float": "asinf", "double": "asin", "long double": "asinl"}[ctype])
        body = body.replace("@CACOS@", {"float": "acosf", "double": "acos", "long double": "acosl"}[ctype])
        body = body.replace("@CSIN@", {"float": "sinf", "double": "sin", "long double": "sinl"}[ctype])
        body = body.replace("@CCOS@", {"float": "cosf", "double": "cos", "long double": "cosl"}[ctype])
        # math_common.h's word on whether the target evaluates the family
        # wider than its type
        body = body.replace("@EVAL_WIDE@", "D_MATH_%s_EVAL_WIDE" % prefix)
        # the digits' declarations and parameters in line, whatever the
        # family's type and suffix
        width = max(len(ctype), len("dj_math_umax"))
        digits = "struct dj_internal_math_%s_digits" % suffix
        body = body.replace("@DG_T@", ctype.ljust(width)).replace("@DG_UMAX@", "dj_math_umax".ljust(width))
        body = body.replace("@DG_INT@", "int".ljust(len(ctype)))
        body = body.replace("@DG_BOOL@", "bool".ljust(len(digits))).replace("@DG_INTP@", "int".ljust(len(digits)))
        text += instantiate(body, suffix, ctype, prefix, prose)
    text += overload_section("5.  ONE NAME FOR EVERY FAMILY (C++)", ELEMENTARY_OVERLOADS)
    text += ELEMENTARY_TAIL
    return text



# -----------------------------------------------------------------------------
# exponential.h
# -----------------------------------------------------------------------------
# The kernels are written once, in long double, and every family's function
# reaches them by conversion: unlike a whole kernel run in a wider type (the
# note at the top), this one is built to be right in long double itself, so a
# float or double result is a long double one rounded once more.

EXPONENTIAL_KERNELS = r"""
//==============================================================================
// 1.  THE KERNELS (long double)
//==============================================================================
// Every family's exponentials and logarithms, to bases e, 2 and 10, are
// computed here, in long double, whose precision is at least double's: a
// float or double result is the kernel's value rounded once more, a long
// double result is the kernel's value. To be right in long double itself the
// kernels carry the reduced argument, and the last sum, as exact pairs -- a
// value and the rounding error below it, by elementary.h's exact sums and
// products (its section 1.1) -- so that only the final addition rounds.
// Nothing depends on long double's format beyond <float.h>: the text is the
// same for an x87's 64 bits, for a long double that is double, and for a
// quad's 113 bits.


// 1.1    The exponential's core
//------------------------------------------------------------------------------
// exp(x) = 2^k exp(r), k the integer nearest x / ln 2 and r = x - k ln 2, so
// |r| <= ln 2 / 2. ln 2 is held in two parts, the first cut to 32 bits, so
// that k ln2_hi is exact for every k a long double reaches and x - k ln2_hi
// is exact by Sterbenz's lemma; the second difference's error is kept, so r
// is carried as r + rlo. exp(r) - 1 - r = r^2/2 (1 + r/3 (1 + r/4 (...))),
// in the nested form to the 26th term, past a quad's precision; then 1 + r
// exactly, and every small part joins it in the one last rounding, what it
// rounds off kept.

// dj_internal_math_ld_exp_split
//   struct: exp(x) as (y + y_lo) 2^k, y within an ulp of [1/sqrt(2),
// sqrt(2)] and y_lo below its last place.
struct dj_internal_math_ld_exp_split
{
    long double y;     // exp(x - k ln 2), rounded once
    long double y_lo;  // the rest
    int         k;
};

// d_internal_math_ld_expm1_series
//   e^_r - 1, |_r| <= ln 2 / 2 (a last place past it does no harm), as high +
// low: r + r^2/2 + r^3/6 + (r^4/24) q, q = 1 + r/5 (1 + r/6 (...)) from its
// 26th term in. r^2/2 is exact, r^3/6 a pair -- r^3/2 exact, its third with
// the division's residual -- and the three summed exactly, so that the
// series' rounding is confined to its fourth term on, some r^3/8 of the
// result below its last place, and only the last sum rounds.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_expm1_series(
    long double _r
)
{
    const struct dj_internal_math_ld_pair z =
        d_internal_math_ld_product_exact(_r, _r);
    // r^2/2, exactly
    const long double half_hi = z.high / 2.0L;
    const long double half_lo = z.low / 2.0L;
    // r^3/2, exactly but for half_lo r, below 2^-2P of it; then its third
    const struct dj_internal_math_ld_pair cube =
        d_internal_math_ld_product_exact(half_hi, _r);
    const long double third_hi = cube.high / 3.0L;
    const struct dj_internal_math_ld_pair back =
        d_internal_math_ld_product_exact(third_hi, 3.0L);
    const long double third_lo =
        ((((cube.high - back.high) - back.low) + cube.low) +
         (half_lo * _r)) / 3.0L;
    long double q = 1.0L;
    int         n = 0;

    for (n = 26; n >= 5; --n)
    {
        q = 1.0L + ((_r * q) / (long double)n);
    }

    {
        // r^4/24 q, as (r^3/6)(r/4) q
        const long double fourth = third_hi * ((_r * q) / 4.0L);
        const struct dj_internal_math_ld_pair head =
            d_internal_math_ld_two_sum(_r, half_hi);
        const struct dj_internal_math_ld_pair next =
            d_internal_math_ld_two_sum(head.high, third_hi);
        const long double small = ((head.low + next.low) + half_lo) +
                                  (third_lo + fourth);

        return d_internal_math_ld_two_sum(next.high, small);
    }
}

// d_internal_math_ld_exp_reduced
//   exp(_r.high + _r.low), |_r.high| <= ln 2 / 2 (a last place past it does
// no harm), as high + low: high the value rounded once, low the rest. The
// base-2 exponential's core (section 1.3) reduces its argument to the same.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_exp_reduced(
    struct dj_internal_math_ld_pair _r
)
{
    // e^r - 1 by the series, then e^(r + rlo) = e^r (1 + rlo): 1 + (e^r - 1)
    // exactly, and rlo's term and the series' low part below its last place
    const struct dj_internal_math_ld_pair e =
        d_internal_math_ld_expm1_series(_r.high);
    const struct dj_internal_math_ld_pair one =
        d_internal_math_ld_two_sum(1.0L, e.high);
    const long double small =
        one.low + (e.low + (_r.low * (1.0L + e.high)));
    struct dj_internal_math_ld_pair result = { 1.0L, 0.0L };

    // then 1 + (e^r - 1) with them, and what that rounding left (fast
    // two-sum)
    result.high = one.high + small;
    result.low  = small - (result.high - one.high);

    return result;
}

// d_internal_math_ld_exp_split_of
//   exp(_x) as (y + y_lo) 2^k, for |_x| below 2^15: k fits an int.
D_MATH_FN struct dj_internal_math_ld_exp_split
d_internal_math_ld_exp_split_of(
    long double _x
)
{
    const long double log2e  = 1.442695040888963407359924681001892137L;
    // ln 2 = ln2_hi + ln2_lo, ln2_hi cut to 32 bits: exact as written
    const long double ln2_hi = 0.69314718036912381649017333984375L;
    const long double ln2_lo = 1.908214929270587816144265680755001344e-10L;
    const long double t      = _x * log2e;
    const int         k      = (int)((t < 0.0L) ? (t - 0.5L) : (t + 0.5L));
    const long double kk     = (long double)k;
    // r = x - k ln 2: x - k ln2_hi exact, the second difference's error kept
    const struct dj_internal_math_ld_pair r =
        d_internal_math_ld_two_sum(_x - (kk * ln2_hi), -(kk * ln2_lo));
    const struct dj_internal_math_ld_pair y =
        d_internal_math_ld_exp_reduced(r);
    struct dj_internal_math_ld_exp_split result = { 1.0L, 0.0L, 0 };

    result.y    = y.high;
    result.y_lo = y.low;
    result.k    = k;

    return result;
}

// 1.2    The logarithm's core
//------------------------------------------------------------------------------
// log(m 2^e) = e ln 2 + log(1 + f), f = m - 1 with m in [1/sqrt(2), sqrt(2)],
// and log(1 + f) = 2 atanh(s) = 2s + 2s^3 (1/3 + s^2/5 + s^4/7 + ...), s = f /
// (2 + f), |s| < 0.172: 24 terms reach past a quad's precision. s is carried
// as s + slo -- 2 + f kept as a pair, the quotient's residual taken by an
// exact product -- and e ln2_hi + 2s is taken exactly, so that only the final
// addition rounds, and what it rounds off is kept.

// dj_internal_math_ld_log_arg
//   struct: a positive finite value as m 2^exponent, m in [1/sqrt(2),
// sqrt(2)] -- what every family's logarithms hand the kernels.
struct dj_internal_math_ld_log_arg
{
    long double m;
    int         exponent;
};

// dj_internal_math_ld_log_parts
//   struct: log(m) = two_s + two_s_lo + tail, two_s exact and the other two
// below its last place, for the logarithms in every base.
struct dj_internal_math_ld_log_parts
{
    long double two_s;     // 2s
    long double two_s_lo;  // 2 s_lo, what s's rounding left
    long double tail;      // 2s^3 (1/3 + s^2/5 + s^4/7 + ...)
};

// d_internal_math_ld_log1p_parts_of
//   log(1 + _f), 1 + _f in [1/sqrt(2), sqrt(2)], in its three parts: _f
// itself is exact, so 1 + _f is never formed, and log1p's core (section 1.5)
// takes it as it is given.
D_MATH_FN struct dj_internal_math_ld_log_parts
d_internal_math_ld_log1p_parts_of(
    long double _f
)
{
    // 2 + f as u + u_lo (fast two-sum)
    const long double f      = _f;
    const long double u      = 2.0L + f;
    const long double u_lo   = f - (u - 2.0L);
    // s = f / (2 + f) as s + s_lo: the residual f - s (u + u_lo), its first
    // product exact
    const long double s      = f / u;
    const struct dj_internal_math_ld_pair su =
        d_internal_math_ld_product_exact(s, u);
    const long double s_lo   = (((f - su.high) - su.low) - (s * u_lo)) / u;
    const long double z      = s * s;
    long double       h      = 0.0L;
    int               j      = 0;
    struct dj_internal_math_ld_log_parts result = { 0.0L, 0.0L, 0.0L };

    // 1/3 + z/5 + z^2/7 + ..., from its 24th term in
    for (j = 24; j >= 1; --j)
    {
        h = (1.0L / (long double)((2 * j) + 1)) + (z * h);
    }

    result.two_s    = 2.0L * s;
    result.two_s_lo = 2.0L * s_lo;
    result.tail     = ((2.0L * s) * z) * h;

    return result;
}

// d_internal_math_ld_log_parts_of
//   log(_m), _m in [1/sqrt(2), sqrt(2)], in its three parts: log(1 + f) of
// f = m - 1, exact by Sterbenz's lemma.
D_MATH_FN struct dj_internal_math_ld_log_parts
d_internal_math_ld_log_parts_of(
    long double _m
)
{
    return d_internal_math_ld_log1p_parts_of(_m - 1.0L);
}

// d_internal_math_ld_log_of
//   log(_m 2^_exponent), _m in [1/sqrt(2), sqrt(2)], as high + low: high the
// value rounded once, low the rest.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_log_of(
    int         _exponent,
    long double _m
)
{
    const long double ln2_hi = 0.69314718036912381649017333984375L;
    const long double ln2_lo = 1.908214929270587816144265680755001344e-10L;
    const long double e      = (long double)_exponent;
    const struct dj_internal_math_ld_log_parts l =
        d_internal_math_ld_log_parts_of(_m);
    // e ln2_hi is exact (at most 15 bits by 32), and so is its sum with 2s,
    // as a pair
    const struct dj_internal_math_ld_pair head =
        d_internal_math_ld_two_sum(e * ln2_hi, l.two_s);
    const long double small = head.low +
                              (l.two_s_lo + (l.tail + (e * ln2_lo)));

    // the head with the small parts, and what that rounding left
    return d_internal_math_ld_two_sum(head.high, small);
}

// 1.3    The base-2 exponential's core
//------------------------------------------------------------------------------
// 2^x = 2^k exp(r ln 2), k the integer nearest x and r = x - k, exact, so
// that |r ln 2| <= ln 2 / 2, the exponential's own reach: r ln 2 as a pair --
// ln 2 in the exponential's two parts, r ln2_hi by Dekker's exact product --
// and the exponential's series (section 1.1) on it. For an integer x, r is 0
// and 2^x is 2^k exactly.

// d_internal_math_ld_exp2_split_of
//   2^_x as (y + y_lo) 2^k, for |_x| below 2^15: k fits an int.
D_MATH_FN struct dj_internal_math_ld_exp_split
d_internal_math_ld_exp2_split_of(
    long double _x
)
{
    // ln 2 = ln2_hi + ln2_lo, ln2_hi cut to 32 bits: exact as written
    const long double ln2_hi = 0.69314718036912381649017333984375L;
    const long double ln2_lo = 1.908214929270587816144265680755001344e-10L;
    const int         k      = (int)((_x < 0.0L) ? (_x - 0.5L)
                                                 : (_x + 0.5L));
    // r = x - k, exact: by Sterbenz's lemma, or x itself where k is 0
    const long double r      = _x - (long double)k;
    // r ln 2 = r ln2_hi + r ln2_lo, the first product exact
    const struct dj_internal_math_ld_pair p =
        d_internal_math_ld_product_exact(r, ln2_hi);
    const struct dj_internal_math_ld_pair u =
        d_internal_math_ld_two_sum(p.high, p.low + (r * ln2_lo));
    const struct dj_internal_math_ld_pair y =
        d_internal_math_ld_exp_reduced(u);
    struct dj_internal_math_ld_exp_split result = { 1.0L, 0.0L, 0 };

    result.y    = y.high;
    result.y_lo = y.low;
    result.k    = k;

    return result;
}

// 1.4    The base-2 and base-10 logarithms' cores
//------------------------------------------------------------------------------
// log2(m 2^e) = e + log(m) log2(e), and log10(m 2^e) = e log10(2) + log(m)
// log10(e), log(m) in the logarithm's parts (section 1.2). Each constant is
// held as ln 2 is, a head cut to 32 bits, exact as written, and the rest: e
// log10(2)'s head is exact, 2s times a head is exact by Dekker's product, the
// smaller parts are taken against the whole constant, and the sum of the
// heads is exact, so that only the final addition rounds and what it rounds
// off is kept. A power of 2 has its exponent as its log2 exactly, and a power
// of 10 its exponent as its log10: the pair's error lies far below the half
// place that would move it.

// d_internal_math_ld_log2_of
//   log2(_m 2^_exponent), _m in [1/sqrt(2), sqrt(2)], as high + low.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_log2_of(
    int         _exponent,
    long double _m
)
{
    // log2(e) = c_hi + c_lo, c_hi cut to 32 bits: exact as written
    const long double c_hi = 1.4426950407214462757110595703125L;
    const long double c_lo = 1.67517131648865110689392137426645954153e-10L;
    const long double c    = c_hi + c_lo;
    const struct dj_internal_math_ld_log_parts l =
        d_internal_math_ld_log_parts_of(_m);
    // 2s c_hi, exactly; the rest of log(m) log2(e) below it
    const struct dj_internal_math_ld_pair p =
        d_internal_math_ld_product_exact(l.two_s, c_hi);
    const long double small = p.low +
                              ((l.two_s * c_lo) +
                               ((l.two_s_lo + l.tail) * c));
    // e + 2s c_hi, exactly
    const struct dj_internal_math_ld_pair head =
        d_internal_math_ld_two_sum((long double)_exponent, p.high);

    return d_internal_math_ld_two_sum(head.high, head.low + small);
}

// d_internal_math_ld_log10_of
//   log10(_m 2^_exponent), _m in [1/sqrt(2), sqrt(2)], as high + low.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_log10_of(
    int         _exponent,
    long double _m
)
{
    // log10(2) = a_hi + a_lo and log10(e) = b_hi + b_lo, the heads cut to
    // 32 bits: exact as written
    const long double a_hi = 0.3010299955494701862335205078125L;
    const long double a_lo = 1.145110089802183869119930267681898814621e-10L;
    const long double b_hi = 0.434294481878168880939483642578125L;
    const long double b_lo = 2.508294671164527633848008229439700580367e-11L;
    const long double b    = b_hi + b_lo;
    const long double e    = (long double)_exponent;
    const struct dj_internal_math_ld_log_parts l =
        d_internal_math_ld_log_parts_of(_m);
    // 2s b_hi, exactly; the rest of log(m) log10(e) below it
    const struct dj_internal_math_ld_pair p =
        d_internal_math_ld_product_exact(l.two_s, b_hi);
    const long double small = p.low +
                              ((l.two_s * b_lo) +
                               ((l.two_s_lo + l.tail) * b));
    // e a_hi (at most 15 bits by 32) + 2s b_hi, exactly
    const struct dj_internal_math_ld_pair head =
        d_internal_math_ld_two_sum(e * a_hi, p.high);

    return d_internal_math_ld_two_sum(head.high,
                                      head.low + (small + (e * a_lo)));
}

// 1.5    The logarithm of 1 + x and the exponential less 1
//------------------------------------------------------------------------------
// log(1 + x) for x > -1: where 1 + x lies in [1/sqrt(2), sqrt(2)], the
// logarithm's core on f = x itself, exact, so that 1 + x is never rounded;
// past it, 1 + x = u + u_lo exactly, by two-sum, and log(u) + u_lo / u, u =
// m 2^e as the logarithm's core takes it -- the second term's own second
// order, (u_lo / u)^2 / 2, lies below 2^-2P of the first. e^x - 1: within
// ln 2 / 2 of 0, the exponential's series on r = x without its leading 1, x
// + x^2/2 (1 + x/3 (1 + x/4 (...))), x^2 by Dekker's exact product, so that
// no 1 + x is formed either; past it, (y + y_lo) 2^k - 1 from the
// exponential's split, 2^k y - 1 exact by two-sum while k lies within P + 3
// of 0. Further out the family needs neither: past (P + 2) ln 2 the 1 lies
// below half the result's last place, and below -(P + 3) ln 2 e^x lies
// below half of 1's.

// d_internal_math_ld_log_arg_normal
//   a positive normal long double as m 2^exponent, m in [1/sqrt(2),
// sqrt(2)], as the logarithm's core takes it: the long double family's
// argument (its section 4.1) for a value that needs no lift from the
// subnormals, which the kernels below, ahead of that family, reach.
D_MATH_FN struct dj_internal_math_ld_log_arg
d_internal_math_ld_log_arg_normal(
    long double _v
)
{
    const struct dj_internal_math_ld_binade b =
        d_internal_math_ld_binade_of(_v);
    // past sqrt(2), m / 2 and E + 1
    const long double m    = _v / b.power;
    const bool        high = (m > 1.41421356237309504880L);
    struct dj_internal_math_ld_log_arg result = { 1.0L, 0 };

    result.m        = high ? (m / 2.0L) : m;
    result.exponent = b.exponent + (high ? 1 : 0);

    return result;
}

// d_internal_math_ld_log1p_of
//   log(1 + _x), _x > -1 finite, as high + low: high the value rounded once,
// low the rest. _x from 2^-P in magnitude, below which the family's value is
// _x's own.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_log1p_of(
    long double _x
)
{
    // 1 + x within [1/sqrt(2), sqrt(2)]: the logarithm's core on x, exact
    if ( (_x >= -0.29289321881345247559915563789515096L) &&
         (_x <= 0.41421356237309504880168872420969808L) )
    {
        const struct dj_internal_math_ld_log_parts l =
            d_internal_math_ld_log1p_parts_of(_x);

        return d_internal_math_ld_two_sum(l.two_s, l.two_s_lo + l.tail);
    }

    {
        // 1 + x = u + u_lo, exactly; log(u) + u_lo / u
        const struct dj_internal_math_ld_pair u =
            d_internal_math_ld_two_sum(1.0L, _x);
        const struct dj_internal_math_ld_log_arg arg =
            d_internal_math_ld_log_arg_normal(u.high);
        const struct dj_internal_math_ld_pair l =
            d_internal_math_ld_log_of(arg.exponent, arg.m);

        return d_internal_math_ld_two_sum(l.high, l.low + (u.low / u.high));
    }
}

// d_internal_math_ld_log1p_of_pair
//   log(1 + _u.high + _u.low), _u.high > -1, as high + low: log1p's core of
// the high part, and the low part over 1 + the high part, its own second
// order below 2^-2P of the result.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_log1p_of_pair(
    struct dj_internal_math_ld_pair _u
)
{
    const struct dj_internal_math_ld_pair l =
        d_internal_math_ld_log1p_of(_u.high);

    return d_internal_math_ld_two_sum(l.high,
                                      l.low + (_u.low / (1.0L + _u.high)));
}

// d_internal_math_ld_expm1_of
//   e^_x - 1, as high + low, for _x from 2^-P in magnitude, below which the
// family's value is _x's own, to (P + 2) ln 2, past which it is e^x's, and
// from -(P + 3) ln 2, below which it is -1. Within ln 2 / 2 the series;
// past it x = k ln 2 + r, |r| <= ln 2 / 2 as a pair, and e^x - 1 = (2^k - 1)
// + 2^k E, E = e^r - 1 by the series, r's low part taken by its derivative:
// 2^k E exact, and 2^k - 1 exact where k is within P of 0, so that the two
// are summed exactly and nothing magnifies E's error, which e^r's would
// carry near e^x = 1 (where 2^k e^r - 1 loses e^r's leading bits).
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_expm1_of(
    long double _x
)
{
    // within ln 2 / 2: the series
    if ( (_x >= -0.34657359027997265470861606072908828L) &&
         (_x <= 0.34657359027997265470861606072908828L) )
    {
        return d_internal_math_ld_expm1_series(_x);
    }

    {
        const long double log2e  = 1.442695040888963407359924681001892137L;
        // ln 2 = ln2_hi + ln2_lo, ln2_hi cut to 32 bits: exact as written
        const long double ln2_hi = 0.69314718036912381649017333984375L;
        const long double ln2_lo = 1.908214929270587816144265680755001344e-10L;
        const long double t      = _x * log2e;
        const int         k      = (int)((t < 0.0L) ? (t - 0.5L) : (t + 0.5L));
        const long double kk     = (long double)k;
        // r = x - k ln 2: x - k ln2_hi exact, the second difference's error
        // kept
        const struct dj_internal_math_ld_pair r =
            d_internal_math_ld_two_sum(_x - (kk * ln2_hi), -(kk * ln2_lo));
        // E = e^r - 1: the series at r's high part, and r's low part times
        // e^r
        const struct dj_internal_math_ld_pair e =
            d_internal_math_ld_expm1_series(r.high);
        const struct dj_internal_math_ld_pair big_e =
            d_internal_math_ld_two_sum(
                e.high,
                e.low + (r.low * (1.0L + e.high)));
        const long double scale = d_internal_math_ld_pow2(k);
        const long double bh    = big_e.high * scale;
        const long double bl    = big_e.low * scale;

        // 2^k - 1 exact: summed with 2^k E exactly
        if ( (k <= LDBL_MANT_DIG) &&
             (k >= -LDBL_MANT_DIG) )
        {
            const struct dj_internal_math_ld_pair s =
                d_internal_math_ld_two_sum(scale - 1.0L, bh);

            return d_internal_math_ld_two_sum(s.high, s.low + bl);
        }

        // 2^k far past 1: the 1 among 2^k E's low part
        if (k > 0)
        {
            const struct dj_internal_math_ld_pair s =
                d_internal_math_ld_two_sum(scale, bh);

            return d_internal_math_ld_two_sum(s.high, s.low + (bl - 1.0L));
        }

        // 2^k far below 1: -1 and the rest
        {
            const struct dj_internal_math_ld_pair s =
                d_internal_math_ld_two_sum(-1.0L, scale + bh);

            return d_internal_math_ld_two_sum(s.high, s.low + bl);
        }
    }
}
"""

EXPONENTIAL_TEMPLATE = r"""
//==============================================================================
// @SECTION@.  THE @UPPER@ FAMILY (_@S@)
//==============================================================================


// @SECTION@.1    Internal helpers
//------------------------------------------------------------------------------

// d_internal_math_@S@_infinity
//   +inf: HUGE_VAL, which C89 and C++98 both define and which is +inf wherever
// the type has one -- a constant, so that no overflow is evaluated to make it
// (GCC refuses one in a constant expression).
D_MATH_FN @T@
d_internal_math_@S@_infinity(void)
{
    return (@T@)HUGE_VAL;
}

// d_internal_math_@S@_exp_scale
//   a kernel's (y + y_lo) 2^k (section 1.1 or 1.3), y within an ulp of
// [1/sqrt(2), sqrt(2)], scaled by 2^k and rounded once to the family: +inf
// past the largest finite value, +0 below half the least subnormal.
D_MATH_FN @T@
d_internal_math_@S@_exp_scale(
    struct dj_internal_math_ld_exp_split _split
)
{
    const int k = _split.k;

    // y 2^k with y above 0.7: past the largest finite value from k =
    // EMAX + 1
    if (k > @EMAX@)
    {
        return d_internal_math_@S@_infinity();
    }

    // at k = EMAX, finite only if y rounds below 1, and then exactly
    // (2y) 2^(EMAX - 1)
    if (k == @EMAX@)
    {
        const @T@ top =
            d_internal_math_@S@_round_pair(_split.y, _split.y_lo);

        if (top >= (@T@)1)
        {
            return d_internal_math_@S@_infinity();
        }

        return (top + top) * d_internal_math_@S@_pow2(@EMAX@ - 1);
    }

    // y below 1.42: under half the least subnormal from k = EMIN - P - 2
    if (k < (@EMIN@ - @P@ - 1))
    {
        return (@T@)0;
    }

    {
        // 2^k in two halves, the first product exact; the second is exact
        // too where long double's range is wider than the family's, and the
        // pair is then rounded once, into the subnormals included --
        // otherwise the second is that rounding, of y alone
        const long double a    = d_internal_math_ld_pow2(k / 2);
        const long double b    = d_internal_math_ld_pow2(k - (k / 2));
        const long double half = _split.y * a;
        const long double lo   = (_split.y_lo * a) * b;
        long double       hi   = half * b;

        // in long double's subnormals, y rounded onto a midpoint between two
        // of them: y_lo's sign decides, as round_pair's does
        if ( (hi < LDBL_MIN) &&
             (_split.y_lo != 0.0L) )
        {
            // what the rounding moved, exactly, at half's scale
            const long double rest = half - (hi / b);

            if ( (rest != 0.0L) &&
                 ((rest > 0.0L) == (_split.y_lo > 0.0L)) )
            {
                const long double next = hi + ((rest + rest) * b);

                // a whole gap, exactly, only from a midpoint
                if (((next - hi) / b) == (rest + rest))
                {
                    hi = next;
                }
            }
        }

        return d_internal_math_@S@_round_pair(hi, lo);
    }
}

// d_internal_math_@S@_log_arg_of
//   a positive, finite, nonzero _x as m 2^e with m in [1/sqrt(2), sqrt(2)],
// exactly: a subnormal first scaled into the normal range, then the binade
// found by its search (float_order.h).
D_MATH_FN struct dj_internal_math_ld_log_arg
d_internal_math_@S@_log_arg_of(
    @T@ _x
)
{
    // a subnormal is first scaled into the normal range
    const int lift = (_x < @MIN@) ? @P@ : 0;
    // then value = m 2^E with m in [1, 2), exactly
    const @T@ value =
        (lift > 0) ? (_x * d_internal_math_@S@_pow2(lift)) : _x;
    const struct dj_internal_math_@S@_binade b =
        d_internal_math_@S@_binade_of(value);
    // past sqrt(2), m / 2 and E + 1
    const long double m    = (long double)(value / b.power);
    const bool        high = (m > 1.41421356237309504880L);
    struct dj_internal_math_ld_log_arg result = { 1.0L, 0 };

    result.m        = high ? (m / 2.0L) : m;
    result.exponent = (b.exponent - lift) + (high ? 1 : 0);

    return result;
}

// @SECTION@.2    Exponential
//------------------------------------------------------------------------------
// exp_c is the exponential in pure arithmetic, a constant expression from
// C++14: the kernel's (y + y_lo) 2^k (section 1.1), scaled by 2^k and rounded
// once to the family. A result past the largest finite value is +inf, one
// below half the least subnormal +0; NaN gives NaN. exp is the C library's.

D_MATH_FN @T@
d_math_@S@_exp_c(
    @T@ _x
)
{
    const long double x = (long double)_x;

    // NaN is its own exponential
    if (_x != _x)
    {
        return _x;
    }

    // well past the largest finite result, +inf among them, and well below
    // half the least subnormal, -inf among them: the bounds keep k an int
    if (x > ((long double)(@EMAX@ + 1) * 0.6932L))
    {
        return d_internal_math_@S@_infinity();
    }

    if (x < ((long double)(@EMIN@ - @P@ - 2) * 0.6932L))
    {
        return (@T@)0;
    }

    return d_internal_math_@S@_exp_scale(d_internal_math_ld_exp_split_of(x));
}

D_MATH_FN_RT @T@
d_math_@S@_exp(
    @T@ _x
)
{
#ifdef __cplusplus
    return std::exp(_x);
#else
    return @CEXP@(_x);
#endif
}

// @SECTION@.3    Natural logarithm
//------------------------------------------------------------------------------
// log_c is the natural logarithm in pure arithmetic, a constant expression
// from C++14: x = m 2^e with m in [1/sqrt(2), sqrt(2)], found exactly by the
// binade's search (float_order.h), and the kernel's log(m 2^e) (section 1.2)
// rounded once to the family. log(1) is +0, log(+-0) is -inf, +inf stays
// +inf, and a negative value or NaN gives NaN. log is the C library's.

D_MATH_FN @T@
d_math_@S@_log_c(
    @T@ _x
)
{
    // NaN and +inf are their own logarithms
    if ( (_x != _x) ||
         (_x > @MAX@) )
    {
        return _x;
    }

    // zero, of either sign, has -inf
    if (_x == (@T@)0)
    {
        return -d_internal_math_@S@_infinity();
    }

    // a negative value has none
    if (_x < (@T@)0)
    {
        return d_internal_math_@S@_nan();
    }

    {
        const struct dj_internal_math_ld_log_arg arg =
            d_internal_math_@S@_log_arg_of(_x);
        const struct dj_internal_math_ld_pair result =
            d_internal_math_ld_log_of(arg.exponent, arg.m);

        return d_internal_math_@S@_round_pair(result.high, result.low);
    }
}

D_MATH_FN_RT @T@
d_math_@S@_log(
    @T@ _x
)
{
#ifdef __cplusplus
    return std::log(_x);
#else
    return @CLOG@(_x);
#endif
}

// @SECTION@.4    Base-2 exponential
//------------------------------------------------------------------------------
// exp2_c is 2^x in pure arithmetic, a constant expression from C++14: the
// kernel's (y + y_lo) 2^k (section 1.3), scaled and rounded once to the
// family as exp_c's is. 2^x for an integer x is exact; a result past the
// largest finite value is +inf, one at or below half the least subnormal +0
// (at half, the tie goes to the even 0); NaN gives NaN. exp2 is the C
// library's, from C++11 in C++; C++ before it has none, and there exp2 is
// exp2_c.

D_MATH_FN @T@
d_math_@S@_exp2_c(
    @T@ _x
)
{
    const long double x = (long double)_x;

    // NaN gives NaN
    if (_x != _x)
    {
        return _x;
    }

    // past the largest finite result, +inf among them, and below half the
    // least subnormal, -inf among them: the bounds keep k an int
    if (x > (long double)(@EMAX@ + 1))
    {
        return d_internal_math_@S@_infinity();
    }

    if (x < (long double)(@EMIN@ - @P@ - 2))
    {
        return (@T@)0;
    }

    return d_internal_math_@S@_exp_scale(
        d_internal_math_ld_exp2_split_of(x));
}

D_MATH_FN_RT @T@
d_math_@S@_exp2(
    @T@ _x
)
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    return std::exp2(_x);
#elif defined(__cplusplus)
    // C++ before C++11 has no exp2: the pure-arithmetic form serves
    return d_math_@S@_exp2_c(_x);
#else
    return @CEXP2@(_x);
#endif
}

// @SECTION@.5    Base-2 and base-10 logarithms
//------------------------------------------------------------------------------
// log2_c and log10_c are the logarithms to bases 2 and 10 in pure
// arithmetic, constant expressions from C++14: x = m 2^e as log_c finds it,
// and the kernel's log2 or log10 of m 2^e (section 1.4) rounded once to the
// family. A power of 2 has its exponent as its log2_c, and a power of 10 its
// own as its log10_c, exactly; both are +0 at 1 and -inf at +-0, +inf stays
// +inf, and a negative value or NaN gives NaN. log2 and log10 are the C
// library's; C++ before C++11 has no log2, and there log2 is log2_c.

D_MATH_FN @T@
d_math_@S@_log2_c(
    @T@ _x
)
{
    // NaN and +inf are their own logarithms
    if ( (_x != _x) ||
         (_x > @MAX@) )
    {
        return _x;
    }

    // zero, of either sign, has -inf
    if (_x == (@T@)0)
    {
        return -d_internal_math_@S@_infinity();
    }

    // a negative value has none
    if (_x < (@T@)0)
    {
        return d_internal_math_@S@_nan();
    }

    {
        const struct dj_internal_math_ld_log_arg arg =
            d_internal_math_@S@_log_arg_of(_x);
        const struct dj_internal_math_ld_pair result =
            d_internal_math_ld_log2_of(arg.exponent, arg.m);

        return d_internal_math_@S@_round_pair(result.high, result.low);
    }
}

D_MATH_FN_RT @T@
d_math_@S@_log2(
    @T@ _x
)
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    return std::log2(_x);
#elif defined(__cplusplus)
    // C++ before C++11 has no log2: the pure-arithmetic form serves
    return d_math_@S@_log2_c(_x);
#else
    return @CLOG2@(_x);
#endif
}

D_MATH_FN @T@
d_math_@S@_log10_c(
    @T@ _x
)
{
    // NaN and +inf are their own logarithms
    if ( (_x != _x) ||
         (_x > @MAX@) )
    {
        return _x;
    }

    // zero, of either sign, has -inf
    if (_x == (@T@)0)
    {
        return -d_internal_math_@S@_infinity();
    }

    // a negative value has none
    if (_x < (@T@)0)
    {
        return d_internal_math_@S@_nan();
    }

    {
        const struct dj_internal_math_ld_log_arg arg =
            d_internal_math_@S@_log_arg_of(_x);
        const struct dj_internal_math_ld_pair result =
            d_internal_math_ld_log10_of(arg.exponent, arg.m);

        return d_internal_math_@S@_round_pair(result.high, result.low);
    }
}

D_MATH_FN_RT @T@
d_math_@S@_log10(
    @T@ _x
)
{
#ifdef __cplusplus
    return std::log10(_x);
#else
    return @CLOG10@(_x);
#endif
}

// @SECTION@.6    Logarithm of 1 + x
//------------------------------------------------------------------------------
// log1p_c is log(1 + x) in pure arithmetic, a constant expression from
// C++14: the kernel's pair (section 1.5) rounded once to the family. Below
// 2^-P in magnitude, P long double's precision, a value is its own -- the
// square's term lies below half its last place -- the zeros among them; +inf
// stays +inf, -1 gives -inf, and a value below -1 or NaN gives NaN. log1p is
// the C library's; C++ before C++11 has none, and there log1p is log1p_c.

D_MATH_FN @T@
d_math_@S@_log1p_c(
    @T@ _x
)
{
    const long double tiny = d_internal_math_ld_pow2(-LDBL_MANT_DIG);

    // NaN, +inf, and the values that are their own
    if ( (_x != _x) ||
         (_x > @MAX@) ||
         ( ((long double)_x < tiny) &&
           ((long double)_x > -tiny) ) )
    {
        return _x;
    }

    // -1 has -inf, and below it there is none
    if (_x <= (@T@)-1)
    {
        return (_x == (@T@)-1) ? -d_internal_math_@S@_infinity()
                               : d_internal_math_@S@_nan();
    }

    {
        const struct dj_internal_math_ld_pair result =
            d_internal_math_ld_log1p_of((long double)_x);

        return d_internal_math_@S@_round_pair(result.high, result.low);
    }
}

D_MATH_FN_RT @T@
d_math_@S@_log1p(
    @T@ _x
)
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    return std::log1p(_x);
#elif defined(__cplusplus)
    // C++ before C++11 has no log1p: the pure-arithmetic form serves
    return d_math_@S@_log1p_c(_x);
#else
    return @CLOG1P@(_x);
#endif
}

// @SECTION@.7    Exponential less 1
//------------------------------------------------------------------------------
// expm1_c is e^x - 1 in pure arithmetic, a constant expression from C++14:
// the kernel's pair (section 1.5) rounded once to the family. Below 2^-P in
// magnitude a value is its own, the zeros among them; past (P + 2) ln 2 it
// is exp_c's, the overflow and +inf among them; below -(P + 3) ln 2 it is -1,
// -inf among them; NaN gives NaN. expm1 is the C library's; C++ before C++11
// has none, and there expm1 is expm1_c.

D_MATH_FN @T@
d_math_@S@_expm1_c(
    @T@ _x
)
{
    const long double x    = (long double)_x;
    const long double tiny = d_internal_math_ld_pow2(-LDBL_MANT_DIG);

    // NaN, and the values that are their own
    if ( (_x != _x) ||
         ( (x < tiny) &&
           (x > -tiny) ) )
    {
        return _x;
    }

    // far below 0: e^x lies below half of 1's last place
    if (x < -((long double)(LDBL_MANT_DIG + 3) * 0.6932L))
    {
        return (@T@)-1;
    }

    // far above 0: the 1 lies below half of e^x's last place
    if (x > ((long double)(LDBL_MANT_DIG + 2) * 0.6931L))
    {
        return d_math_@S@_exp_c(_x);
    }

    {
        const struct dj_internal_math_ld_pair result =
            d_internal_math_ld_expm1_of(x);

        return d_internal_math_@S@_round_pair(result.high, result.low);
    }
}

D_MATH_FN_RT @T@
d_math_@S@_expm1(
    @T@ _x
)
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    return std::expm1(_x);
#elif defined(__cplusplus)
    // C++ before C++11 has no expm1: the pure-arithmetic form serves
    return d_math_@S@_expm1_c(_x);
#else
    return @CEXPM1@(_x);
#endif
}
"""

EXPONENTIAL_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE KERNELS (long double)
    -------------------------
    1.  The exponential's core
    2.  The logarithm's core
    3.  The base-2 exponential's core
    4.  The base-2 and base-10 logarithms' cores
    5.  The logarithm of 1 + x and the exponential less 1
2.  THE FLOAT FAMILY (_f)
    ---------------------
    1.  Internal helpers
    2.  Exponential
    3.  Natural logarithm
    4.  Base-2 exponential
    5.  Base-2 and base-10 logarithms
    6.  Logarithm of 1 + x
    7.  Exponential less 1
3.  THE DOUBLE FAMILY (_d)
    ----------------------
    1.  Internal helpers
    2.  Exponential
    3.  Natural logarithm
    4.  Base-2 exponential
    5.  Base-2 and base-10 logarithms
    6.  Logarithm of 1 + x
    7.  Exponential less 1
4.  THE LONG DOUBLE FAMILY (_ld)
    ----------------------------
    1.  Internal helpers
    2.  Exponential
    3.  Natural logarithm
    4.  Base-2 exponential
    5.  Base-2 and base-10 logarithms
    6.  Logarithm of 1 + x
    7.  Exponential less 1
5.  ONE NAME FOR EVERY FAMILY (C++)
    -------------------------------
*/

#ifndef DJINTERP_C_MATH_EXPONENTIAL_H
#define DJINTERP_C_MATH_EXPONENTIAL_H 1

// std
#ifdef __cplusplus
    #include <cmath>         // std::exp, std::log, std::exp2, std::log1p
#endif  // __cplusplus
#include <float.h>           // LDBL_MANT_DIG
#include <math.h>            // expf ... expm1l, HUGE_VAL
// djinterp
#include "./elementary.h"    // d_internal_math_ld_two_sum, *_round_pair
#include "./float_order.h"   // d_internal_math_*_pow2, *_binade_of
#include "./math_common.h"   // D_MATH_FN
"""

EXPONENTIAL_TAIL = r"""

#endif  // DJINTERP_C_MATH_EXPONENTIAL_H
"""

EXPONENTIAL_OVERLOADS = [
    ("D_MATH_FN", "@T@", "d_math_exp_c", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_exp", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_log_c", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_log", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_exp2_c", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_exp2", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_log2_c", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_log2", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_log10_c", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_log10", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_log1p_c", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_log1p", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_expm1_c", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_expm1", [("@T@", "_x")]),
]


def exponential():
    lines = [
        "Exponentials and logarithms: the C core's e^x and natural logarithm,",
        "2^x and the logarithms to bases 2 and 10, and log(1 + x) and e^x - 1,",
        "for float, double and long double.",
        "  GENERATED by scripts/gen_c_math_float.py from one text for the three",
        "families: edit the generator, not this file.",
        "",
        "  Each function comes twice, as in elementary.h. The _c form is pure",
        "arithmetic, so the same text is a constant expression from C++14; the",
        "plain form calls the C library (std:: in C++), fast at run time. The _c",
        "forms are computed once, in long double, by kernels that carry their",
        "reduced argument and last sum as exact pairs, and rounded once to the",
        "family: a long double result is within an ulp of the exact one, exp's,",
        "log1p's and expm1's within 0.53 of one, a float or double result",
        "correctly rounded but where the exact value lies within the kernel's",
        "error of a midpoint. C++ before C++11 has no exp2, log2,",
        "log1p or expm1, and there the plain forms are the _c ones. No overflow",
        "is evaluated, as a constant expression forbids: an infinity is",
        "HUGE_VAL, a NaN NAN.",
    ]
    text = banner("exponential.h", lines, "2026.10.08", "2026.11.07")
    text += "\n" + EXPONENTIAL_HEAD
    text += EXPONENTIAL_KERNELS
    for i, (suffix, ctype, prefix, prose) in enumerate(FAMILIES):
        upper = prose.upper()
        body = EXPONENTIAL_TEMPLATE.replace("@SECTION@", str(i + 2)).replace("@UPPER@", upper)
        body = body.replace("@CEXP@", {"float": "expf", "double": "exp", "long double": "expl"}[ctype])
        body = body.replace("@CLOG@", {"float": "logf", "double": "log", "long double": "logl"}[ctype])
        body = body.replace("@CEXP2@", {"float": "exp2f", "double": "exp2", "long double": "exp2l"}[ctype])
        body = body.replace("@CLOG2@", {"float": "log2f", "double": "log2", "long double": "log2l"}[ctype])
        body = body.replace("@CLOG10@", {"float": "log10f", "double": "log10", "long double": "log10l"}[ctype])
        body = body.replace("@CLOG1P@", {"float": "log1pf", "double": "log1p", "long double": "log1pl"}[ctype])
        body = body.replace("@CEXPM1@", {"float": "expm1f", "double": "expm1", "long double": "expm1l"}[ctype])
        text += instantiate(body, suffix, ctype, prefix, prose)
    text += overload_section("5.  ONE NAME FOR EVERY FAMILY (C++)", EXPONENTIAL_OVERLOADS)
    text += EXPONENTIAL_TAIL
    return text



# -----------------------------------------------------------------------------
# tangent.h
# -----------------------------------------------------------------------------
# As exponential.h's: kernels written once, in long double, built on
# elementary.h's, and every family rounding a kernel's pair once.

TANGENT_KERNELS = r"""
//==============================================================================
// 1.  THE KERNELS (long double)
//==============================================================================
// Every family's tangent and arc-tangent are computed here, in long double,
// as exact pairs -- elementary.h's (its section 1.1) -- and rounded once by
// the family: a float or double result is the kernel's value rounded once
// more, a long double result is the kernel's value. Nothing depends on long
// double's format beyond <float.h>.


// 1.1    The tangent's core
//------------------------------------------------------------------------------
// tan x = sin r / cos r where x's quadrant is even and -cos r / sin r where
// it is odd, r + r_lo being x reduced by pi/2 -- exactly, however large x is
// -- and the sine and cosine its pairs (elementary.h, sections 1.3 and 1.4).
// The quotient is taken with its residual, num - q den, whose first product
// is exact by Dekker's method, so that it keeps the pairs' precision.

// d_internal_math_ld_tan_of
//   tan(_x) as high + low, _x finite.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_tan_of(
    long double _x
)
{
    const struct dj_internal_math_ld_sin_cos sc =
        d_internal_math_ld_sin_cos_of(_x);
    const bool                            odd = ((sc.quadrant % 2) != 0);
    const struct dj_internal_math_ld_pair num = odd ? sc.cosine : sc.sine;
    const struct dj_internal_math_ld_pair den = odd ? sc.sine : sc.cosine;
    const long double                     q   = num.high / den.high;
    const struct dj_internal_math_ld_pair qd  =
        d_internal_math_ld_product_exact(q, den.high);
    // num - q den: num.high - qd.high is exact (Sterbenz's lemma)
    const long double q_lo = ( ((num.high - qd.high) - qd.low) +
                               (num.low - (q * den.low)) ) / den.high;
    struct dj_internal_math_ld_pair result =
        d_internal_math_ld_two_sum(q, q_lo);

    // an odd quadrant's quotient is negated
    if (odd)
    {
        result.high = -result.high;
        result.low  = -result.low;
    }

    return result;
}

// 1.2    The arc-tangent's core
//------------------------------------------------------------------------------
// atan(-x) = -atan x, and past 1 atan x = pi/2 - atan u, u = 1/x carried as
// u + u_lo (its residual 1 - u x by Dekker's product, until x is so large
// that u is below pi/2's last place); then elementary.h's arc-tangent of the
// unit interval (its section 1.5).

// d_internal_math_ld_atan_of
//   atan(_x) as high + low, _x not NaN; an infinity gives +-pi/2.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_atan_of(
    long double _x
)
{
    const bool        negative = (_x < 0.0L);
    const long double ax       = negative ? -_x : _x;
    const bool        inverted = (ax > 1.0L);
    long double       u_hi     = ax;
    long double       u_lo     = 0.0L;
    struct dj_internal_math_ld_pair result = { 0.0L, 0.0L };

    // past 1: u = 1/x, and below x = 2^(P+8) its residual (u x stays far
    // from overflow there; past it u is below pi/2's last place)
    if (inverted)
    {
        u_hi = 1.0L / ax;

        if (ax < d_internal_math_ld_pow2(LDBL_MANT_DIG + 8))
        {
            const struct dj_internal_math_ld_pair ux =
                d_internal_math_ld_product_exact(u_hi, ax);

            u_lo = ((1.0L - ux.high) - ux.low) / ax;
        }
    }

    // atan u, or pi/2 - atan u past 1
    result = d_internal_math_ld_atan_reduced(u_hi, u_lo, inverted ? 1 : 0,
                                             inverted);

    if (negative)
    {
        result.high = -result.high;
        result.low  = -result.low;
    }

    return result;
}

// 1.3    The two-argument arc-tangent's core
//------------------------------------------------------------------------------
// atan2(y, x) for y and x finite and nonzero: the smaller magnitude divided
// into the larger as a pair (elementary.h's ratio), then the quadrant from
// the signs -- atan u, pi - atan u, or pi/2 -+ atan u past the diagonal -- by
// elementary.h's arc-tangent of the unit interval, and y's sign last,
// atan2(-y, x) being -atan2(y, x). No quotient past 1 is formed, so none can
// overflow, for any two finite values.

// d_internal_math_ld_atan2_of
//   atan2(_y, _x) as high + low, _y and _x finite and nonzero.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_atan2_of(
    long double _y,
    long double _x
)
{
    const long double ay       = (_y < 0.0L) ? -_y : _y;
    const long double ax       = (_x < 0.0L) ? -_x : _x;
    // past the diagonal, x / y and a quarter turn
    const bool        steep    = (ay > ax);
    const struct dj_internal_math_ld_pair u =
        steep ? d_internal_math_ld_ratio(ax, 0.0L, ay, 0.0L)
              : d_internal_math_ld_ratio(ay, 0.0L, ax, 0.0L);
    // atan u for x > 0, pi - atan u for x < 0; past the diagonal pi/2 -
    // atan u for x > 0 and pi/2 + atan u for x < 0
    const int         quarters = steep ? 1 : ((_x < 0.0L) ? 2 : 0);
    const bool        minus    = steep ? (_x > 0.0L) : (_x < 0.0L);
    struct dj_internal_math_ld_pair result =
        d_internal_math_ld_atan_reduced(u.high, u.low, quarters, minus);

    if (_y < 0.0L)
    {
        result.high = -result.high;
        result.low  = -result.low;
    }

    return result;
}
"""

TANGENT_TEMPLATE = r"""
//==============================================================================
// @SECTION@.  THE @UPPER@ FAMILY (_@S@)
//==============================================================================


// @SECTION@.1    Tangent
//------------------------------------------------------------------------------
// tan_c is the tangent in pure arithmetic, a constant expression from C++14,
// at every finite argument: the kernel's pair (section 1.1) rounded once to
// the family (elementary.h's round_pair). NaN and an infinity give NaN; -0
// stays -0. tan is the C library's.

D_MATH_FN @T@
d_math_@S@_tan_c(
    @T@ _x
)
{
    // NaN and the infinities
    if (!( (_x <= @MAX@) &&
           (_x >= -@MAX@) ))
    {
        return d_internal_math_@S@_nan();
    }

    // zero, of either sign, is its own tangent
    if (_x == (@T@)0)
    {
        return _x;
    }

    {
        const struct dj_internal_math_ld_pair result =
            d_internal_math_ld_tan_of((long double)_x);

        return d_internal_math_@S@_round_pair(result.high, result.low);
    }
}

D_MATH_FN_RT @T@
d_math_@S@_tan(
    @T@ _x
)
{
#ifdef __cplusplus
    return std::tan(_x);
#else
    return @CTAN@(_x);
#endif
}

// @SECTION@.2    Arc-tangent
//------------------------------------------------------------------------------
// atan_c is the arc-tangent in pure arithmetic, a constant expression from
// C++14: the kernel's pair (section 1.2) rounded once to the family. An
// infinity gives +-pi/2, rounded to the family; NaN gives NaN; -0 stays -0.
// atan is the C library's.

D_MATH_FN @T@
d_math_@S@_atan_c(
    @T@ _x
)
{
    // NaN, and zero of either sign, are their own arc-tangents
    if ( (_x != _x) ||
         (_x == (@T@)0) )
    {
        return _x;
    }

    {
        const struct dj_internal_math_ld_pair result =
            d_internal_math_ld_atan_of((long double)_x);

        return d_internal_math_@S@_round_pair(result.high, result.low);
    }
}

D_MATH_FN_RT @T@
d_math_@S@_atan(
    @T@ _x
)
{
#ifdef __cplusplus
    return std::atan(_x);
#else
    return @CATAN@(_x);
#endif
}

// @SECTION@.3    Two-argument arc-tangent
//------------------------------------------------------------------------------
// atan2_c is the angle of (_x, _y) in pure arithmetic, a constant expression
// from C++14: the kernel's pair (section 1.3) rounded once to the family, and
// the zeros and infinities as C99's Annex F has them -- atan2(+-0, x) is +-0
// for x +0 or positive and +-pi for x -0 or negative; a zero x, or an
// infinite y against a finite x, gives +-pi/2; an infinite y against an
// infinite x +-pi/4, or +-3pi/4 for -inf; a finite y against an infinite x
// +-0, or +-pi for -inf. NaN in either gives NaN. atan2 is the C library's.

// d_internal_math_@S@_negative
//   whether _x is negative, -0 among them. No comparison sees a zero's sign:
// GCC and Clang read it with copysign, a builtin either evaluates in a
// constant expression; elsewhere it is 1 / _x, -inf for -0 at run time but no
// constant expression.
D_MATH_FN bool
d_internal_math_@S@_negative(
    @T@ _x
)
{
    if (_x != (@T@)0)
    {
        return (_x < (@T@)0);
    }

#if ( (defined(D_ENV_COMPILER_GCC)) ||                                         \
      (defined(D_ENV_COMPILER_CLANG)) )
    return (@COPYSIGN@((@T@)1, _x) < (@T@)0);
#else
    return (((@T@)1 / _x) < (@T@)0);
#endif
}

// d_internal_math_@S@_turns
//   _quarters pi/2 + atan(_u), or less it when _minus is true, rounded once
// to the family and negated when _negative is true: atan2's values at the
// zeros and infinities -- pi/2, pi, pi/4, 3pi/4 -- by the kernel of the rest.
D_MATH_FN @T@
d_internal_math_@S@_turns(
    long double _u,
    int         _quarters,
    bool        _minus,
    bool        _negative
)
{
    const struct dj_internal_math_ld_pair angle =
        d_internal_math_ld_atan_reduced(_u, 0.0L, _quarters, _minus);
    const @T@ value =
        d_internal_math_@S@_round_pair(angle.high, angle.low);

    return _negative ? -value
                     : value;
}

D_MATH_FN @T@
d_math_@S@_atan2_c(
    @T@ _y,
    @T@ _x
)
{
    // NaN in either
    if (_y != _y)
    {
        return _y;
    }

    if (_x != _x)
    {
        return _x;
    }

    {
        const bool y_negative = d_internal_math_@S@_negative(_y);
        const bool x_negative = d_internal_math_@S@_negative(_x);
        const bool y_infinite = !( (_y <= @MAX@) &&
                                   (_y >= -@MAX@) );
        const bool x_infinite = !( (_x <= @MAX@) &&
                                   (_x >= -@MAX@) );

        // y = +-0: +-0 toward +0 and the positives, +-pi toward -0 and the
        // negatives
        if (_y == (@T@)0)
        {
            return x_negative ? d_internal_math_@S@_turns(0.0L, 2, false,
                                                          y_negative)
                              : _y;
        }

        // x = +-0, or y infinite and x finite: +-pi/2
        if ( (_x == (@T@)0) ||
             ( (y_infinite) &&
               (!x_infinite) ) )
        {
            return d_internal_math_@S@_turns(0.0L, 1, false, y_negative);
        }

        // both infinite: +-pi/4, or +-3pi/4 toward -inf
        if (y_infinite)
        {
            return x_negative ? d_internal_math_@S@_turns(1.0L, 2, true,
                                                          y_negative)
                              : d_internal_math_@S@_turns(1.0L, 0, false,
                                                          y_negative);
        }

        // y finite, x infinite: +-0, or +-pi toward -inf
        if (x_infinite)
        {
            return x_negative ? d_internal_math_@S@_turns(0.0L, 2, false,
                                                          y_negative)
                              : (y_negative ? -(@T@)0
                                            : (@T@)0);
        }

        {
            const struct dj_internal_math_ld_pair result =
                d_internal_math_ld_atan2_of((long double)_y,
                                            (long double)_x);

            return d_internal_math_@S@_round_pair(result.high, result.low);
        }
    }
}

D_MATH_FN_RT @T@
d_math_@S@_atan2(
    @T@ _y,
    @T@ _x
)
{
#ifdef __cplusplus
    return std::atan2(_y, _x);
#else
    return @CATAN2@(_y, _x);
#endif
}
"""

TANGENT_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE KERNELS (long double)
    -------------------------
    1.  The tangent's core
    2.  The arc-tangent's core
    3.  The two-argument arc-tangent's core
2.  THE FLOAT FAMILY (_f)
    ---------------------
    1.  Tangent
    2.  Arc-tangent
    3.  Two-argument arc-tangent
3.  THE DOUBLE FAMILY (_d)
    ----------------------
    1.  Tangent
    2.  Arc-tangent
    3.  Two-argument arc-tangent
4.  THE LONG DOUBLE FAMILY (_ld)
    ----------------------------
    1.  Tangent
    2.  Arc-tangent
    3.  Two-argument arc-tangent
5.  ONE NAME FOR EVERY FAMILY (C++)
    -------------------------------
*/

#ifndef DJINTERP_C_MATH_TANGENT_H
#define DJINTERP_C_MATH_TANGENT_H 1

// std
#ifdef __cplusplus
    #include <cmath>         // std::tan, std::atan, std::atan2
#endif  // __cplusplus
#include <float.h>           // FLT_MAX ... LDBL_MAX, LDBL_MANT_DIG
#include <math.h>            // tanf ... atan2l
// djinterp
#include "./elementary.h"    // d_internal_math_ld_sin_cos_of, *_atan_reduced
#include "./float_order.h"   // d_internal_math_ld_pow2
#include "./math_common.h"   // D_MATH_FN
"""

TANGENT_TAIL = r"""

#endif  // DJINTERP_C_MATH_TANGENT_H
"""

TANGENT_OVERLOADS = [
    ("D_MATH_FN", "@T@", "d_math_tan_c", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_tan", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_atan_c", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_atan", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_atan2_c", [("@T@", "_y"), ("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_atan2", [("@T@", "_y"), ("@T@", "_x")]),
]


def tangent():
    lines = [
        "Tangent and arc-tangent: the C core's tan, atan and atan2, for float,",
        "double and long double.",
        "  GENERATED by scripts/gen_c_math_float.py from one text for the three",
        "families: edit the generator, not this file.",
        "",
        "  Each function comes twice, as in elementary.h. The _c form is pure",
        "arithmetic, so the same text is a constant expression from C++14; the",
        "plain form calls the C library (std:: in C++), fast at run time. The _c",
        "forms are computed once, in long double, as exact pairs, and rounded once",
        "to the family: the correctly rounded value but where the exact one lies",
        "within the kernel's error of a midpoint, and so within an ulp of the",
        "library's. tan_c reduces its argument by pi/2 exactly at every finite",
        "value, as elementary.h's sine and cosine do.",
    ]
    text = banner("tangent.h", lines, "2026.10.09", "2026.11.07")
    text += "\n" + TANGENT_HEAD
    text += TANGENT_KERNELS
    for i, (suffix, ctype, prefix, prose) in enumerate(FAMILIES):
        upper = prose.upper()
        body = TANGENT_TEMPLATE.replace("@SECTION@", str(i + 2)).replace("@UPPER@", upper)
        body = body.replace("@CTAN@", {"float": "tanf", "double": "tan", "long double": "tanl"}[ctype])
        body = body.replace("@CATAN2@", {"float": "atan2f", "double": "atan2", "long double": "atan2l"}[ctype])
        body = body.replace("@CATAN@", {"float": "atanf", "double": "atan", "long double": "atanl"}[ctype])
        body = body.replace("@COPYSIGN@", {"float": "__builtin_copysignf", "double": "__builtin_copysign", "long double": "__builtin_copysignl"}[ctype])
        text += instantiate(body, suffix, ctype, prefix, prose)
    text += overload_section("5.  ONE NAME FOR EVERY FAMILY (C++)", TANGENT_OVERLOADS)
    text += TANGENT_TAIL
    return text



# -----------------------------------------------------------------------------
# hyperbolic.h
# -----------------------------------------------------------------------------
# As tangent.h's: one kernel, written once in long double on exponential.h's
# and elementary.h's, and every family rounding its pairs once.

HYPERBOLIC_KERNELS = r"""
//==============================================================================
// 1.  THE KERNELS (long double)
//==============================================================================
// Every family's hyperbolic functions and their inverses are computed here, in
// long double, as exact pairs -- elementary.h's (its section 1.1) -- and
// rounded once by the family: a float or double result is the kernel's value
// rounded once more, a long double result is the kernel's value. Nothing
// depends on long double's format beyond <float.h>.


// 1.1    The hyperbolic sine and cosine
//------------------------------------------------------------------------------
// sinh a and cosh a for a >= 0, both as pairs scaled by a power of 2, so that
// neither overflows before its family rounds it. Below 1/2 by their Taylor
// series: a^2, a^3 = a a^2 and a^4 by Dekker's exact products, a^3/6 and
// a^4/24 with their residuals, and the terms past them -- under 2^-10 of the
// sum -- nested to the 31st power, past a quad's precision; a + a^3/6 and
// 1 + a^2/2 + a^4/24 are summed exactly, so that only the last addition
// rounds. From 1/2 by (E - 1/E) / 2 and (E + 1/E) / 2, E = e^a from
// exponential.h's kernel as (y + y_lo) 2^k and 1/E as a pair, its residual by
// Dekker's product: the sine loses under a bit to the difference, at a = 1/2,
// and 1/E is dropped once it lies below 2^-(2P + 8) of E.

// dj_internal_math_ld_sinh_cosh
//   struct: sinh a and cosh a, each 2^exponent times its pair.
struct dj_internal_math_ld_sinh_cosh
{
    struct dj_internal_math_ld_pair sinh_part;  // sinh a / 2^exponent
    struct dj_internal_math_ld_pair cosh_part;  // cosh a / 2^exponent
    int                             exponent;
};

// d_internal_math_ld_sinh_cosh_of
//   sinh(_a) and cosh(_a), for _a from 2^-(P/2 + 1) -- where the squares are
// exact -- to (EMAX + 2) ln 2, where the exponential's k still fits an int.
D_MATH_FN struct dj_internal_math_ld_sinh_cosh
d_internal_math_ld_sinh_cosh_of(
    long double _a
)
{
    struct dj_internal_math_ld_sinh_cosh result = { { 0.0L, 0.0L },
                                                    { 1.0L, 0.0L },
                                                    0 };

    // below 1/2: the series
    if (_a < 0.5L)
    {
        // z = a^2, exactly; a^3 = a z_hi + a z_lo
        const struct dj_internal_math_ld_pair z =
            d_internal_math_ld_product_exact(_a, _a);
        const struct dj_internal_math_ld_pair cube =
            d_internal_math_ld_product_exact(_a, z.high);
        const long double cube_lo = cube.low + (_a * z.low);
        // a^3/6, with its residual
        const long double t3 = cube.high / 6.0L;
        const struct dj_internal_math_ld_pair p3 =
            d_internal_math_ld_product_exact(t3, 6.0L);
        const long double t3_lo = (((cube.high - p3.high) - p3.low) +
                                   cube_lo) / 6.0L;
        // a^4 = z_hi^2 + 2 z_hi z_lo, and a^4/24 with its residual
        const struct dj_internal_math_ld_pair fourth =
            d_internal_math_ld_product_exact(z.high, z.high);
        const long double fourth_lo = fourth.low + ((2.0L * z.high) * z.low);
        const long double t4 = fourth.high / 24.0L;
        const struct dj_internal_math_ld_pair p4 =
            d_internal_math_ld_product_exact(t4, 24.0L);
        const long double t4_lo = (((fourth.high - p4.high) - p4.low) +
                                   fourth_lo) / 24.0L;
        long double       gs = 1.0L;
        long double       gc = 1.0L;
        int               n  = 0;

        // the sine's terms past a^5/120, 1 + z/42 (1 + z/72 (...)), and the
        // cosine's past a^6/720, 1 + z/56 (1 + z/90 (...)), from the 31st
        // and 30th powers in
        for (n = 15; n >= 3; --n)
        {
            gs = 1.0L + ((z.high * gs) /
                         (long double)((2 * n) * ((2 * n) + 1)));
        }

        for (n = 15; n >= 4; --n)
        {
            gc = 1.0L + ((z.high * gc) /
                         (long double)(((2 * n) - 1) * (2 * n)));
        }

        {
            // a^5/120 (...) and a^6/720 (...)
            const long double rest_s = (t3 * (z.high / 20.0L)) * gs;
            const long double rest_c = (t4 * (z.high / 30.0L)) * gc;
            // a + a^3/6, and 1 + a^2/2 + a^4/24, exactly
            const struct dj_internal_math_ld_pair s =
                d_internal_math_ld_two_sum(_a, t3);
            const struct dj_internal_math_ld_pair c1 =
                d_internal_math_ld_two_sum(1.0L, z.high / 2.0L);
            const struct dj_internal_math_ld_pair c2 =
                d_internal_math_ld_two_sum(c1.high, t4);
            // and the small parts below them
            const long double s_small = s.low + (t3_lo + rest_s);
            const long double c_small = (c1.low + c2.low) +
                                        ( (z.low / 2.0L) +
                                          (t4_lo + rest_c) );

            result.sinh_part = d_internal_math_ld_two_sum(s.high, s_small);
            result.cosh_part = d_internal_math_ld_two_sum(c2.high, c_small);
            result.exponent  = 0;
        }

        return result;
    }

    // from 1/2: (E -+ 1/E) / 2, as 2^(k - 1) times (y + y_lo) -+ (1/E) 2^-k
    {
        const struct dj_internal_math_ld_exp_split e =
            d_internal_math_ld_exp_split_of(_a);
        long double v    = 0.0L;
        long double v_lo = 0.0L;

        // 1/E = (i + i_lo) 2^-k, i = 1/y with its residual; so (1/E) 2^-k,
        // beside y, is (i + i_lo) 2^-2k, dropped past 2^-(2P + 8)
        if ((2 * e.k) <= ((2 * LDBL_MANT_DIG) + 8))
        {
            const long double i     = 1.0L / e.y;
            const struct dj_internal_math_ld_pair iy =
                d_internal_math_ld_product_exact(i, e.y);
            const long double i_lo  = (((1.0L - iy.high) - iy.low) -
                                       (i * e.y_lo)) / e.y;
            const long double scale = d_internal_math_ld_pow2(-2 * e.k);

            v    = i * scale;
            v_lo = i_lo * scale;
        }

        {
            // y -+ v exactly, then the small parts below them
            const struct dj_internal_math_ld_pair difference =
                d_internal_math_ld_two_sum(e.y, -v);
            const struct dj_internal_math_ld_pair sum =
                d_internal_math_ld_two_sum(e.y, v);
            const long double d_small = difference.low + (e.y_lo - v_lo);
            const long double s_small = sum.low + (e.y_lo + v_lo);

            result.sinh_part = d_internal_math_ld_two_sum(difference.high,
                                                          d_small);
            result.cosh_part = d_internal_math_ld_two_sum(sum.high, s_small);
            result.exponent  = e.k - 1;
        }

        return result;
    }
}

// 1.2    The inverse hyperbolic functions
//------------------------------------------------------------------------------
// asinh a = log(a + sqrt(a^2 + 1)) and acosh x = log(x + sqrt(x^2 - 1)): the
// square and its sum with 1 or -1 exact as a pair, by Dekker's product and
// two-sum, its root as a pair -- the long double root, and the residual over
// twice it -- and the logarithm of the sum with a or x as a pair, by
// exponential.h's cores. Near 0 asinh a is log1p(a + a^2 / (1 + sqrt(1 +
// a^2))), and near 1 acosh x is log1p(t + sqrt(2t + t^2)), t = x - 1 exact
// by Sterbenz's lemma, so that no 1 + something small is rounded; past
// 2^(P/2 + 1) both are log(2a), the next term, 1/(4a^2), below 2^-(P + 2).
// atanh a = log1p(2a / (1 - a)) / 2, 1 - a a pair, exact by Sterbenz's lemma
// from 1/2, and the quotient with its residual.

// d_internal_math_ld_sqrt_pair
//   sqrt(_s.high + _s.low), _s.high positive and normal, as high + low: the
// long double root t, and the residual (_s.high - t^2 + _s.low) / (2t), t^2
// by Dekker's exact product.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_sqrt_pair(
    struct dj_internal_math_ld_pair _s
)
{
    const long double t = d_math_ld_sqrt_c(_s.high);
    const struct dj_internal_math_ld_pair square =
        d_internal_math_ld_product_exact(t, t);
    struct dj_internal_math_ld_pair result = { 0.0L, 0.0L };

    result.high = t;
    result.low  = (((_s.high - square.high) - square.low) + _s.low) /
                  (2.0L * t);

    return result;
}

// d_internal_math_ld_log_of_pair
//   log(_w.high + _w.low), _w.high positive and normal, as high + low: the
// logarithm's core of the high part, and the low part over it.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_log_of_pair(
    struct dj_internal_math_ld_pair _w
)
{
    const struct dj_internal_math_ld_log_arg arg =
        d_internal_math_ld_log_arg_normal(_w.high);
    const struct dj_internal_math_ld_pair l =
        d_internal_math_ld_log_of(arg.exponent, arg.m);

    return d_internal_math_ld_two_sum(l.high, l.low + (_w.low / _w.high));
}

// d_internal_math_ld_log_twice
//   log(2 _a), _a positive and normal, as a pair: _a's exponent one more, so
// that 2a is never formed and the largest long double has its own.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_log_twice(
    long double _a
)
{
    const struct dj_internal_math_ld_log_arg arg =
        d_internal_math_ld_log_arg_normal(_a);

    return d_internal_math_ld_log_of(arg.exponent + 1, arg.m);
}

// d_internal_math_ld_asinh_of
//   asinh(_a), _a finite, from 2^-(P/2 + 1), below which the family's value
// is _a's own, as a pair.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_asinh_of(
    long double _a
)
{
    // past 2^(P/2 + 1): log(2a)
    if (_a > d_internal_math_ld_pow2((LDBL_MANT_DIG / 2) + 1))
    {
        return d_internal_math_ld_log_twice(_a);
    }

    {
        // a^2 + 1, exactly, and its root as a pair
        const struct dj_internal_math_ld_pair z =
            d_internal_math_ld_product_exact(_a, _a);
        const struct dj_internal_math_ld_pair s0 =
            d_internal_math_ld_two_sum(z.high, 1.0L);
        struct dj_internal_math_ld_pair s = s0;
        struct dj_internal_math_ld_pair t = { 0.0L, 0.0L };

        s.low = s0.low + z.low;
        t     = d_internal_math_ld_sqrt_pair(s);

        // from 1/2: log(a + t)
        if (_a >= 0.5L)
        {
            const struct dj_internal_math_ld_pair w0 =
                d_internal_math_ld_two_sum(_a, t.high);
            struct dj_internal_math_ld_pair w = w0;

            w.low = w0.low + t.low;

            return d_internal_math_ld_log_of_pair(w);
        }

        {
            // below it: log1p(a + a^2 / (1 + t)), the quotient with its
            // residual
            const struct dj_internal_math_ld_pair d0 =
                d_internal_math_ld_two_sum(1.0L, t.high);
            const long double d_lo = d0.low + t.low;
            const long double q    = z.high / d0.high;
            const struct dj_internal_math_ld_pair qd =
                d_internal_math_ld_product_exact(q, d0.high);
            const long double q_lo = ( (((z.high - qd.high) - qd.low) +
                                        z.low) -
                                       (q * d_lo) ) / d0.high;
            const struct dj_internal_math_ld_pair u0 =
                d_internal_math_ld_two_sum(_a, q);
            struct dj_internal_math_ld_pair u = u0;

            u.low = u0.low + q_lo;

            return d_internal_math_ld_log1p_of_pair(u);
        }
    }
}

// d_internal_math_ld_acosh_of
//   acosh(_x), _x above 1 and finite, as a pair.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_acosh_of(
    long double _x
)
{
    // past 2^(P/2 + 1): log(2x)
    if (_x > d_internal_math_ld_pow2((LDBL_MANT_DIG / 2) + 1))
    {
        return d_internal_math_ld_log_twice(_x);
    }

    // from 2: log(x + sqrt(x^2 - 1)), x^2 - 1 exactly
    if (_x >= 2.0L)
    {
        const struct dj_internal_math_ld_pair z =
            d_internal_math_ld_product_exact(_x, _x);
        const struct dj_internal_math_ld_pair s0 =
            d_internal_math_ld_two_sum(z.high, -1.0L);
        struct dj_internal_math_ld_pair s = s0;
        struct dj_internal_math_ld_pair t = { 0.0L, 0.0L };
        struct dj_internal_math_ld_pair w = { 0.0L, 0.0L };

        s.low = s0.low + z.low;
        t     = d_internal_math_ld_sqrt_pair(s);
        w     = d_internal_math_ld_two_sum(_x, t.high);
        w.low = w.low + t.low;

        return d_internal_math_ld_log_of_pair(w);
    }

    {
        // below 2: log1p(t + sqrt(2t + t^2)), t = x - 1 exact
        const long double t = _x - 1.0L;
        const struct dj_internal_math_ld_pair z =
            d_internal_math_ld_product_exact(t, t);
        const struct dj_internal_math_ld_pair s0 =
            d_internal_math_ld_two_sum(2.0L * t, z.high);
        struct dj_internal_math_ld_pair s = s0;
        struct dj_internal_math_ld_pair r = { 0.0L, 0.0L };
        struct dj_internal_math_ld_pair u = { 0.0L, 0.0L };

        s.low = s0.low + z.low;
        r     = d_internal_math_ld_sqrt_pair(s);
        u     = d_internal_math_ld_two_sum(t, r.high);
        u.low = u.low + r.low;

        return d_internal_math_ld_log1p_of_pair(u);
    }
}

// d_internal_math_ld_atanh_of
//   atanh(_a), _a in [2^-(P/2 + 1), 1), as a pair.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_atanh_of(
    long double _a
)
{
    // 1 - a as a pair, and 2a / (1 - a) with its residual
    const struct dj_internal_math_ld_pair d =
        d_internal_math_ld_two_sum(1.0L, -_a);
    const long double two_a = 2.0L * _a;
    const long double q     = two_a / d.high;
    const struct dj_internal_math_ld_pair qd =
        d_internal_math_ld_product_exact(q, d.high);
    struct dj_internal_math_ld_pair u = { 0.0L, 0.0L };
    struct dj_internal_math_ld_pair l = { 0.0L, 0.0L };

    u.high = q;
    u.low  = (((two_a - qd.high) - qd.low) - (q * d.low)) / d.high;
    l      = d_internal_math_ld_log1p_of_pair(u);

    // the half, exactly
    l.high = l.high / 2.0L;
    l.low  = l.low / 2.0L;

    return l;
}
"""

HYPERBOLIC_TEMPLATE = r"""
//==============================================================================
// @SECTION@.  THE @UPPER@ FAMILY (_@S@)
//==============================================================================


// @SECTION@.1    Internal helpers
//------------------------------------------------------------------------------

// d_internal_math_@S@_scale_pair
//   _value 2^_exponent rounded once to the family, for a pair _value of at
// least 2^-(P/2 + 1) and below 2 and a result in the normal range or past it:
// the pair rounded, then scaled exactly; +inf past the largest finite value.
D_MATH_FN @T@
d_internal_math_@S@_scale_pair(
    struct dj_internal_math_ld_pair _value,
    int                             _exponent
)
{
    const @T@ value =
        d_internal_math_@S@_round_pair(_value.high, _value.low);

    // in [1, 2) it reaches 2^EMAX from _exponent = EMAX, below 1 from EMAX + 1
    if ( ( (value >= (@T@)1) && (_exponent >= @EMAX@) ) ||
         (_exponent > @EMAX@) )
    {
        return d_internal_math_@S@_infinity();
    }

    // 2^_exponent in two halves, each a normal value
    return (value * d_internal_math_@S@_pow2(_exponent / 2)) *
           d_internal_math_@S@_pow2(_exponent - (_exponent / 2));
}

// @SECTION@.2    Hyperbolic sine
//------------------------------------------------------------------------------
// sinh_c is the hyperbolic sine in pure arithmetic, a constant expression from
// C++14: the kernel's pair (section 1.1) rounded once to the family. It is
// odd; below 2^-(P/2 + 1), P long double's precision, a value is its own --
// the cube's term lies below half its last place -- and so are the zeros and
// the infinities; past the largest finite result it is +-inf; NaN gives NaN.
// sinh is the C library's.

D_MATH_FN @T@
d_math_@S@_sinh_c(
    @T@ _x
)
{
    const bool        negative = (_x < (@T@)0);
    const long double a        = negative ? -(long double)_x
                                          : (long double)_x;

    // NaN, and the values that are their own hyperbolic sines
    if ( (_x != _x) ||
         (a > (long double)@MAX@) ||
         (a < d_internal_math_ld_pow2(-(LDBL_MANT_DIG / 2) - 1)) )
    {
        return _x;
    }

    // well past the largest finite result: the bound keeps k an int
    if (a > ((long double)(@EMAX@ + 2) * 0.6932L))
    {
        return negative ? -d_internal_math_@S@_infinity()
                        : d_internal_math_@S@_infinity();
    }

    {
        const struct dj_internal_math_ld_sinh_cosh h =
            d_internal_math_ld_sinh_cosh_of(a);
        const @T@ value =
            d_internal_math_@S@_scale_pair(h.sinh_part, h.exponent);

        if (negative)
        {
            return -value;
        }

        return value;
    }
}

D_MATH_FN_RT @T@
d_math_@S@_sinh(
    @T@ _x
)
{
#ifdef __cplusplus
    return std::sinh(_x);
#else
    return @CSINH@(_x);
#endif
}

// @SECTION@.3    Hyperbolic cosine
//------------------------------------------------------------------------------
// cosh_c is the hyperbolic cosine in pure arithmetic, a constant expression
// from C++14: the kernel's pair (section 1.1) rounded once to the family. It
// is even; below 2^-(P/2 + 1) it is 1, the zeros among them; past the largest
// finite result it is +inf, the infinities among them; NaN gives NaN. cosh
// is the C library's.

D_MATH_FN @T@
d_math_@S@_cosh_c(
    @T@ _x
)
{
    const long double a = (_x < (@T@)0) ? -(long double)_x
                                        : (long double)_x;

    // NaN is its own hyperbolic cosine
    if (_x != _x)
    {
        return _x;
    }

    // the infinities, and well past the largest finite result: the bound
    // keeps k an int
    if (a > ((long double)(@EMAX@ + 2) * 0.6932L))
    {
        return d_internal_math_@S@_infinity();
    }

    // so near 0 that a^2/2 lies below half an ulp of 1
    if (a < d_internal_math_ld_pow2(-(LDBL_MANT_DIG / 2) - 1))
    {
        return (@T@)1;
    }

    {
        const struct dj_internal_math_ld_sinh_cosh h =
            d_internal_math_ld_sinh_cosh_of(a);

        return d_internal_math_@S@_scale_pair(h.cosh_part, h.exponent);
    }
}

D_MATH_FN_RT @T@
d_math_@S@_cosh(
    @T@ _x
)
{
#ifdef __cplusplus
    return std::cosh(_x);
#else
    return @CCOSH@(_x);
#endif
}

// @SECTION@.4    Hyperbolic tangent
//------------------------------------------------------------------------------
// tanh_c is the hyperbolic tangent in pure arithmetic, a constant expression
// from C++14: the kernel's sine pair divided by its cosine pair, as a pair
// (elementary.h's ratio), rounded once to the family. It is odd; below
// 2^-(P/2 + 1) a value is its own, the zeros among them; past (P + 4) ln 2 / 2
// it is +-1, 1 - tanh lying below 2^-(P + 3), and so are the infinities; NaN
// gives NaN. tanh is the C library's.

D_MATH_FN @T@
d_math_@S@_tanh_c(
    @T@ _x
)
{
    const bool        negative = (_x < (@T@)0);
    const long double a        = negative ? -(long double)_x
                                          : (long double)_x;

    // NaN, and the values that are their own hyperbolic tangents
    if ( (_x != _x) ||
         (a < d_internal_math_ld_pow2(-(LDBL_MANT_DIG / 2) - 1)) )
    {
        return _x;
    }

    // +-1, the infinities among them
    if (a > ((long double)(LDBL_MANT_DIG + 4) * 0.3466L))
    {
        return negative ? (@T@)-1
                        : (@T@)1;
    }

    {
        const struct dj_internal_math_ld_sinh_cosh h =
            d_internal_math_ld_sinh_cosh_of(a);
        // the quotient with its residual, then one pair again
        const struct dj_internal_math_ld_pair q =
            d_internal_math_ld_ratio(h.sinh_part.high, h.sinh_part.low,
                                     h.cosh_part.high, h.cosh_part.low);
        const struct dj_internal_math_ld_pair t =
            d_internal_math_ld_two_sum(q.high, q.low);
        const @T@ value = d_internal_math_@S@_round_pair(t.high, t.low);

        if (negative)
        {
            return -value;
        }

        return value;
    }
}

D_MATH_FN_RT @T@
d_math_@S@_tanh(
    @T@ _x
)
{
#ifdef __cplusplus
    return std::tanh(_x);
#else
    return @CTANH@(_x);
#endif
}

// @SECTION@.5    Inverse hyperbolic sine
//------------------------------------------------------------------------------
// asinh_c is the inverse hyperbolic sine in pure arithmetic, a constant
// expression from C++14: the kernel's pair (section 1.2) rounded once to the
// family. It is odd; below 2^-(P/2 + 1) a value is its own, the zeros among
// them, and so are the infinities; NaN gives NaN. asinh is the C library's;
// C++ before C++11 has none, and there asinh is asinh_c.

D_MATH_FN @T@
d_math_@S@_asinh_c(
    @T@ _x
)
{
    const bool        negative = (_x < (@T@)0);
    const long double a        = negative ? -(long double)_x
                                          : (long double)_x;

    // NaN, and the values that are their own
    if ( (_x != _x) ||
         (a > (long double)@MAX@) ||
         (a < d_internal_math_ld_pow2(-(LDBL_MANT_DIG / 2) - 1)) )
    {
        return _x;
    }

    {
        const struct dj_internal_math_ld_pair r =
            d_internal_math_ld_asinh_of(a);

        return negative ? d_internal_math_@S@_round_pair(-r.high, -r.low)
                        : d_internal_math_@S@_round_pair(r.high, r.low);
    }
}

D_MATH_FN_RT @T@
d_math_@S@_asinh(
    @T@ _x
)
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    return std::asinh(_x);
#elif defined(__cplusplus)
    // C++ before C++11 has no asinh: the pure-arithmetic form serves
    return d_math_@S@_asinh_c(_x);
#else
    return @CASINH@(_x);
#endif
}

// @SECTION@.6    Inverse hyperbolic cosine
//------------------------------------------------------------------------------
// acosh_c is the inverse hyperbolic cosine in pure arithmetic, a constant
// expression from C++14: the kernel's pair (section 1.2) rounded once to the
// family. 1 gives +0 and +inf stays +inf; a value below 1, -inf among them,
// or NaN gives NaN. acosh is the C library's; C++ before C++11 has none, and
// there acosh is acosh_c.

D_MATH_FN @T@
d_math_@S@_acosh_c(
    @T@ _x
)
{
    // NaN and +inf are their own
    if ( (_x != _x) ||
         (_x > @MAX@) )
    {
        return _x;
    }

    // 1 has +0, and below it there is none
    if (_x <= (@T@)1)
    {
        return (_x == (@T@)1) ? (@T@)0
                              : d_internal_math_@S@_nan();
    }

    {
        const struct dj_internal_math_ld_pair r =
            d_internal_math_ld_acosh_of((long double)_x);

        return d_internal_math_@S@_round_pair(r.high, r.low);
    }
}

D_MATH_FN_RT @T@
d_math_@S@_acosh(
    @T@ _x
)
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    return std::acosh(_x);
#elif defined(__cplusplus)
    // C++ before C++11 has no acosh: the pure-arithmetic form serves
    return d_math_@S@_acosh_c(_x);
#else
    return @CACOSH@(_x);
#endif
}

// @SECTION@.7    Inverse hyperbolic tangent
//------------------------------------------------------------------------------
// atanh_c is the inverse hyperbolic tangent in pure arithmetic, a constant
// expression from C++14: the kernel's pair (section 1.2) rounded once to the
// family. It is odd; below 2^-(P/2 + 1) a value is its own, the zeros among
// them; +-1 gives +-inf, and a value past them, or NaN, gives NaN. atanh is
// the C library's; C++ before C++11 has none, and there atanh is atanh_c.

D_MATH_FN @T@
d_math_@S@_atanh_c(
    @T@ _x
)
{
    const bool        negative = (_x < (@T@)0);
    const long double a        = negative ? -(long double)_x
                                          : (long double)_x;

    // NaN, and the values that are their own
    if ( (_x != _x) ||
         (a < d_internal_math_ld_pow2(-(LDBL_MANT_DIG / 2) - 1)) )
    {
        return _x;
    }

    // +-1 have +-inf, and past them there is none
    if (a >= 1.0L)
    {
        if (a > 1.0L)
        {
            return d_internal_math_@S@_nan();
        }

        return negative ? -d_internal_math_@S@_infinity()
                        : d_internal_math_@S@_infinity();
    }

    {
        const struct dj_internal_math_ld_pair r =
            d_internal_math_ld_atanh_of(a);

        return negative ? d_internal_math_@S@_round_pair(-r.high, -r.low)
                        : d_internal_math_@S@_round_pair(r.high, r.low);
    }
}

D_MATH_FN_RT @T@
d_math_@S@_atanh(
    @T@ _x
)
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    return std::atanh(_x);
#elif defined(__cplusplus)
    // C++ before C++11 has no atanh: the pure-arithmetic form serves
    return d_math_@S@_atanh_c(_x);
#else
    return @CATANH@(_x);
#endif
}
"""

HYPERBOLIC_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE KERNELS (long double)
    -------------------------
    1.  The hyperbolic sine and cosine
    2.  The inverse hyperbolic functions
2.  THE FLOAT FAMILY (_f)
    ---------------------
    1.  Internal helpers
    2.  Hyperbolic sine
    3.  Hyperbolic cosine
    4.  Hyperbolic tangent
    5.  Inverse hyperbolic sine
    6.  Inverse hyperbolic cosine
    7.  Inverse hyperbolic tangent
3.  THE DOUBLE FAMILY (_d)
    ----------------------
    1.  Internal helpers
    2.  Hyperbolic sine
    3.  Hyperbolic cosine
    4.  Hyperbolic tangent
    5.  Inverse hyperbolic sine
    6.  Inverse hyperbolic cosine
    7.  Inverse hyperbolic tangent
4.  THE LONG DOUBLE FAMILY (_ld)
    ----------------------------
    1.  Internal helpers
    2.  Hyperbolic sine
    3.  Hyperbolic cosine
    4.  Hyperbolic tangent
    5.  Inverse hyperbolic sine
    6.  Inverse hyperbolic cosine
    7.  Inverse hyperbolic tangent
5.  ONE NAME FOR EVERY FAMILY (C++)
    -------------------------------
*/

#ifndef DJINTERP_C_MATH_HYPERBOLIC_H
#define DJINTERP_C_MATH_HYPERBOLIC_H 1

// std
#ifdef __cplusplus
    #include <cmath>         // std::sinh, std::cosh, std::tanh, std::asinh
#endif  // __cplusplus
#include <float.h>           // FLT_MAX ... LDBL_MAX, LDBL_MANT_DIG
#include <math.h>            // sinhf ... atanhl
// djinterp
#include "./elementary.h"    // d_internal_math_ld_product_exact, *_ratio
#include "./exponential.h"   // d_internal_math_ld_exp_split_of, *_log1p_of
#include "./float_order.h"   // d_internal_math_*_pow2
#include "./math_common.h"   // D_MATH_FN
"""

HYPERBOLIC_TAIL = r"""

#endif  // DJINTERP_C_MATH_HYPERBOLIC_H
"""

HYPERBOLIC_OVERLOADS = [
    ("D_MATH_FN", "@T@", "d_math_sinh_c", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_sinh", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_cosh_c", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_cosh", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_tanh_c", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_tanh", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_asinh_c", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_asinh", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_acosh_c", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_acosh", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_atanh_c", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_atanh", [("@T@", "_x")]),
]


def hyperbolic():
    lines = [
        "Hyperbolic functions: the C core's sinh, cosh and tanh, and their",
        "inverses asinh, acosh and atanh, for float, double and long double.",
        "  GENERATED by scripts/gen_c_math_float.py from one text for the three",
        "families: edit the generator, not this file.",
        "",
        "  Each function comes twice, as in elementary.h. The _c form is pure",
        "arithmetic, so the same text is a constant expression from C++14; the",
        "plain form calls the C library (std:: in C++), fast at run time. The _c",
        "forms are computed once, in long double, as exact pairs -- near 0 by",
        "their series, further out from exponential.h's kernels, the inverses",
        "from its logarithms -- and rounded once to the family: the correctly",
        "rounded value but where the exact one lies within the kernel's error of",
        "a midpoint. The library's need not be so close -- glibc's are up to",
        "three ulps from these -- and so the two forms can differ in their last",
        "bits. C++ before C++11 has no asinh, acosh or atanh, and there the",
        "plain forms are the _c ones.",
    ]
    text = banner("hyperbolic.h", lines, "2026.10.09", "2026.11.07")
    text += "\n" + HYPERBOLIC_HEAD
    text += HYPERBOLIC_KERNELS
    for i, (suffix, ctype, prefix, prose) in enumerate(FAMILIES):
        upper = prose.upper()
        body = HYPERBOLIC_TEMPLATE.replace("@SECTION@", str(i + 2)).replace("@UPPER@", upper)
        body = body.replace("@CSINH@", {"float": "sinhf", "double": "sinh", "long double": "sinhl"}[ctype])
        body = body.replace("@CCOSH@", {"float": "coshf", "double": "cosh", "long double": "coshl"}[ctype])
        body = body.replace("@CTANH@", {"float": "tanhf", "double": "tanh", "long double": "tanhl"}[ctype])
        body = body.replace("@CASINH@", {"float": "asinhf", "double": "asinh", "long double": "asinhl"}[ctype])
        body = body.replace("@CACOSH@", {"float": "acoshf", "double": "acosh", "long double": "acoshl"}[ctype])
        body = body.replace("@CATANH@", {"float": "atanhf", "double": "atanh", "long double": "atanhl"}[ctype])
        text += instantiate(body, suffix, ctype, prefix, prose)
    text += overload_section("5.  ONE NAME FOR EVERY FAMILY (C++)", HYPERBOLIC_OVERLOADS)
    text += HYPERBOLIC_TAIL
    return text



# -----------------------------------------------------------------------------
# power.h
# -----------------------------------------------------------------------------
# As exponential.h's: kernels written once, in long double, on elementary.h's
# exact sums and products, every family rounding once. The cube root and the
# hypotenuse are settled to the family's last bit, as elementary.h's square
# root is.

POWER_KERNELS = r"""
//==============================================================================
// 1.  THE KERNELS (long double)
//==============================================================================
// Every family's cube root and power are computed here, in long double. The
// cube root: Newton's iteration to within a long double's last place or two,
// rounded to the family, then the family's last bit settled exactly -- by the
// sign of (t + h)^3 - m, t the candidate and t + h a midpoint beside it, from
// an approximation where its error bound decides it and otherwise from an
// exact sum. The cube root of a value of the family never lies on one of its
// midpoints -- a midpoint has a bit past the family's precision, and its cube
// three times as many -- so the sign is never 0, and the root is correctly
// rounded. The power: exp(y log x), the logarithm and the exponential carried
// in pairs to 18 bits and more past long double's precision, and the exact
// powers -- the only ones that can lie on a midpoint -- computed exactly.
// Nothing depends on long double's format beyond <float.h>: the text is the
// same for an x87's 64 bits, for a long double that is double, and for a
// quad's 113 bits.


// 1.1    Exact sums of many terms
//------------------------------------------------------------------------------
// A sum of long doubles held exactly as an expansion: nonoverlapping parts,
// the least first, each term added by Shewchuk's grow-expansion -- the term
// carried up through the parts by two-sums, what each leaves kept and its
// zeros dropped. The sum's sign is its largest part's.

// dj_internal_math_ld_expansion
//   struct: an exact sum of up to 16 long doubles, as nonoverlapping parts,
// the least first, and how many parts are in use.
struct dj_internal_math_ld_expansion
{
    long double part[16];
    int         count;
};

// d_internal_math_ld_expansion_grow
//   _e + _b, exactly, as an expansion of at most one more part.
D_MATH_FN struct dj_internal_math_ld_expansion
d_internal_math_ld_expansion_grow(
    struct dj_internal_math_ld_expansion _e,
    long double                          _b
)
{
    struct dj_internal_math_ld_expansion result = { { 0.0L }, 0 };
    long double                          carry  = _b;
    int                                  i      = 0;

    for (i = 0; i < _e.count; ++i)
    {
        const struct dj_internal_math_ld_pair s =
            d_internal_math_ld_two_sum(carry, _e.part[i]);

        carry = s.high;

        if (s.low != 0.0L)
        {
            result.part[result.count] = s.low;
            ++result.count;
        }
    }

    if (carry != 0.0L)
    {
        result.part[result.count] = carry;
        ++result.count;
    }

    return result;
}

// 1.2    The sign of a cube about a midpoint
//------------------------------------------------------------------------------
// (t + h)^3 - m = (t^3 - m) + 3t^2 h + 3t h^2 + h^3, for m in [1, 8), t near
// its cube root and h a power of 2 or its negation -- half the family's gap
// above t or below it. t^2 is exact as a pair by Dekker's product, and t^3 as
// two more; c1, the first of them, lies within a factor of 2 of m, so c1 - m
// is exact (Sterbenz's lemma); h times any of them is exact. The terms summed
// in long double give the sign wherever the sum lies past its error bound;
// within it, every term is added exactly instead. The bound reaches past the
// sum only for a root within 2^(4 - P) ulp of a midpoint, P long double's
// precision.

// d_internal_math_ld_cube_sign_exact
//   the sign of (_t + _h)^3 - _m, from its twelve terms summed exactly.
D_MATH_FN int
d_internal_math_ld_cube_sign_exact(
    long double _m,
    long double _t,
    long double _h
)
{
    const struct dj_internal_math_ld_pair square =
        d_internal_math_ld_product_exact(_t, _t);
    // t^3 = t square.high + t square.low, each exactly
    const struct dj_internal_math_ld_pair c1 =
        d_internal_math_ld_product_exact(_t, square.high);
    const struct dj_internal_math_ld_pair c2 =
        d_internal_math_ld_product_exact(_t, square.low);
    // 3t^2 h = 3 (h square.high) + 3 (h square.low), and 3t h^2: each
    // tripled exactly as a pair, 2v + v
    const long double ha = _h * square.high;
    const long double hb = _h * square.low;
    const long double hh = _h * _h;
    const long double th = _t * hh;
    const struct dj_internal_math_ld_pair d1 =
        d_internal_math_ld_two_sum(ha + ha, ha);
    const struct dj_internal_math_ld_pair d2 =
        d_internal_math_ld_two_sum(hb + hb, hb);
    const struct dj_internal_math_ld_pair d3 =
        d_internal_math_ld_two_sum(th + th, th);
    struct dj_internal_math_ld_expansion sum = { { 0.0L }, 0 };

    sum = d_internal_math_ld_expansion_grow(sum, -_m);
    sum = d_internal_math_ld_expansion_grow(sum, c1.high);
    sum = d_internal_math_ld_expansion_grow(sum, c1.low);
    sum = d_internal_math_ld_expansion_grow(sum, c2.high);
    sum = d_internal_math_ld_expansion_grow(sum, c2.low);
    sum = d_internal_math_ld_expansion_grow(sum, d1.high);
    sum = d_internal_math_ld_expansion_grow(sum, d1.low);
    sum = d_internal_math_ld_expansion_grow(sum, d2.high);
    sum = d_internal_math_ld_expansion_grow(sum, d2.low);
    sum = d_internal_math_ld_expansion_grow(sum, d3.high);
    sum = d_internal_math_ld_expansion_grow(sum, d3.low);
    sum = d_internal_math_ld_expansion_grow(sum, hh * _h);

    if (sum.count == 0)
    {
        return 0;
    }

    return (sum.part[sum.count - 1] > 0.0L) ? 1 : -1;
}

// d_internal_math_ld_cube_sign
//   the sign of (_t + _h)^3 - _m: the terms summed in long double, by at most
// ten roundings, each off by at most half a last place of the terms'
// magnitudes summed, decide it where the sum lies past 16 such half places;
// the exact sum otherwise.
D_MATH_FN int
d_internal_math_ld_cube_sign(
    long double _m,
    long double _t,
    long double _h
)
{
    const struct dj_internal_math_ld_pair square =
        d_internal_math_ld_product_exact(_t, _t);
    const struct dj_internal_math_ld_pair c1 =
        d_internal_math_ld_product_exact(_t, square.high);
    const long double g   = c1.high - _m;
    const long double tb  = _t * square.low;
    const long double k   = _h * square.high;
    const long double hb  = _h * square.low;
    const long double th2 = (_t * _h) * _h;
    const long double h3  = (_h * _h) * _h;
    const long double sum = ((g + (3.0L * k)) +
                             ((c1.low + tb) + (3.0L * hb))) +
                            ((3.0L * th2) + h3);
    // the terms' magnitudes, summed with the factors of 3 they carry
    const long double abs_g = (g < 0.0L) ? -g : g;
    const long double abs_k = (k < 0.0L) ? -k : k;
    const long double abs_c = (c1.low < 0.0L) ? -c1.low : c1.low;
    const long double abs_t = (tb < 0.0L) ? -tb : tb;
    const long double abs_b = (hb < 0.0L) ? -hb : hb;
    const long double abs_s = (th2 < 0.0L) ? -th2 : th2;
    const long double abs_h = (h3 < 0.0L) ? -h3 : h3;
    const long double bound = ( abs_g + (3.0L * abs_k) + abs_c + abs_t +
                                (3.0L * abs_b) + (3.0L * abs_s) + abs_h ) *
                              (8.0L * LDBL_EPSILON);

    if (sum > bound)
    {
        return 1;
    }

    if (sum < -bound)
    {
        return -1;
    }

    return d_internal_math_ld_cube_sign_exact(_m, _t, _h);
}

// 1.3    The cube root's approximation
//------------------------------------------------------------------------------

// d_internal_math_ld_cbrt_newton
//   cbrt(_m), _m in [1, 8), to within a long double's last place or two: a
// quadratic within 1.6% of it, then five steps of Newton's iteration, each
// squaring the relative error -- past a quad's precision.
D_MATH_FN long double
d_internal_math_ld_cbrt_newton(
    long double _m
)
{
    long double root = 0.7625L + (_m * (0.2681L - (_m * 0.01467L)));
    int         i    = 0;

    for (i = 0; i < 5; ++i)
    {
        root = root - ((((root * root) * root) - _m) /
                       ((3.0L * root) * root));
    }

    return root;
}

// 1.4    Sums and products of pairs
//------------------------------------------------------------------------------
// A pair is high + low, low below high's last place (elementary.h's section
// 1.1). The sum and the product of two pairs are pairs again: the highs'
// sum or product exactly, by Knuth's two-sum or Dekker's product, the lows'
// terms added to what that left, and the whole renormalized by a two-sum --
// off by a few units of the low part's last place, some 2^(-2P) of the value.

// d_internal_math_ld_pair_sum
//   _a + _b, both pairs, as a pair.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_pair_sum(
    struct dj_internal_math_ld_pair _a,
    struct dj_internal_math_ld_pair _b
)
{
    const struct dj_internal_math_ld_pair s =
        d_internal_math_ld_two_sum(_a.high, _b.high);

    return d_internal_math_ld_two_sum(s.high, s.low + (_a.low + _b.low));
}

// d_internal_math_ld_pair_product
//   _a _b, both pairs, as a pair; the highs normal or zero, their product far
// from overflow.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_pair_product(
    struct dj_internal_math_ld_pair _a,
    struct dj_internal_math_ld_pair _b
)
{
    const struct dj_internal_math_ld_pair p =
        d_internal_math_ld_product_exact(_a.high, _b.high);

    return d_internal_math_ld_two_sum(p.high,
                                      p.low + ((_a.high * _b.low) +
                                               (_a.low * _b.high)));
}

// d_internal_math_ld_pair_of
//   _high + _low as a pair, _low below _high's last place.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_pair_of(
    long double _high,
    long double _low
)
{
    struct dj_internal_math_ld_pair result = { 0.0L, 0.0L };

    result.high = _high;
    result.low  = _low;

    return result;
}

// 1.5    The logarithm, past long double's precision
//------------------------------------------------------------------------------
// x^y = exp(y log x) magnifies log x's relative error by y log x, up to
// 2^14: the power needs the logarithm to some 30 bits past long double's
// precision. log(m 2^e) = e ln 2 + 2 atanh(s), s = (m - 1) / (m + 1) carried
// as s + s_lo, as exponential.h's section 1.2 has it, but atanh's series in
// pairs: 2s (1 + z (1/3 + z (1/5 + z (1/7 + z (1/9 + z (1/11 + z q)))))),
// z = s^2 below 0.03, each coefficient held as ln 2 is -- a head cut to 32
// bits, exact as written, and the rest -- and q, the series from 1/13, in
// long double: z^6 times its error lies past 2^-30 of the value. e ln 2 is
// e ln2_hi, exact, and e ln2_lo by Dekker's product, the sum of the heads
// exact; ln 2's two parts carry it to 2^-(P + 32) for every exponent.

// d_internal_math_ld_log_accurate_of
//   log(_m 2^_exponent), _m in [1/sqrt(2), sqrt(2)], as a pair, to about
// 2^-(P + 30) of the value.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_log_accurate_of(
    int         _exponent,
    long double _m
)
{
    const long double ln2_hi = 0.69314718036912381649017333984375L;
    const long double ln2_lo = 1.908214929270587816144265680755001344e-10L;
    // 1/3, 1/5, 1/7, 1/9 and 1/11: heads of 32 bits, exact as written
    const struct dj_internal_math_ld_pair c3 = d_internal_math_ld_pair_of(
        0.33333333325572311878204345703125L,
        7.761021455128987630208333333333333333333e-11L);
    const struct dj_internal_math_ld_pair c5 = d_internal_math_ld_pair_of(
        0.19999999995343387126922607421875L,
        4.656612873077392578125e-11L);
    const struct dj_internal_math_ld_pair c7 = d_internal_math_ld_pair_of(
        0.142857142840512096881866455078125L,
        1.663076026099068777901785714285714285714e-11L);
    const struct dj_internal_math_ld_pair c9 = d_internal_math_ld_pair_of(
        0.11111111109494231641292572021484375L,
        1.616879469818539089626736111111111111111e-11L);
    const struct dj_internal_math_ld_pair c11 = d_internal_math_ld_pair_of(
        0.0909090908826328814029693603515625L,
        2.645802768793973055752840909090909090909e-11L);
    // f = m - 1, exact; 2 + f as u + u_lo; s = f / (2 + f) as s + s_lo
    const long double f    = _m - 1.0L;
    const long double u    = 2.0L + f;
    const long double u_lo = f - (u - 2.0L);
    const long double s    = f / u;
    const struct dj_internal_math_ld_pair su =
        d_internal_math_ld_product_exact(s, u);
    const long double s_lo = (((f - su.high) - su.low) - (s * u_lo)) / u;
    const struct dj_internal_math_ld_pair two_s =
        d_internal_math_ld_pair_of(2.0L * s, 2.0L * s_lo);
    // z = s^2 as a pair: s s exactly, and 2 s s_lo below it
    const struct dj_internal_math_ld_pair ss =
        d_internal_math_ld_product_exact(s, s);
    const struct dj_internal_math_ld_pair z =
        d_internal_math_ld_two_sum(ss.high, ss.low + ((2.0L * s) * s_lo));
    struct dj_internal_math_ld_pair series = { 0.0L, 0.0L };
    long double                     q      = 0.0L;
    int                             j      = 0;

    // 1/13 + z/15 + z^2/17 + ..., from its 19th term in
    for (j = 24; j >= 6; --j)
    {
        q = (1.0L / (long double)((2 * j) + 1)) + (z.high * q);
    }

    // 1/3 + z (1/5 + z (1/7 + z (1/9 + z (1/11 + z q)))), in pairs
    series = d_internal_math_ld_pair_sum(
        c11, d_internal_math_ld_pair_product(
                 z, d_internal_math_ld_pair_of(q, 0.0L)));
    series = d_internal_math_ld_pair_sum(
        c9, d_internal_math_ld_pair_product(z, series));
    series = d_internal_math_ld_pair_sum(
        c7, d_internal_math_ld_pair_product(z, series));
    series = d_internal_math_ld_pair_sum(
        c5, d_internal_math_ld_pair_product(z, series));
    series = d_internal_math_ld_pair_sum(
        c3, d_internal_math_ld_pair_product(z, series));

    {
        // log(m) = 2s + 2s z series
        const struct dj_internal_math_ld_pair log_m =
            d_internal_math_ld_pair_sum(
                two_s,
                d_internal_math_ld_pair_product(
                    two_s, d_internal_math_ld_pair_product(z, series)));
        // e ln2_hi (at most 15 bits by 32) and log m's head, exactly; then
        // e ln2_lo's head, exactly, by Dekker's product
        const long double e = (long double)_exponent;
        const struct dj_internal_math_ld_pair el =
            d_internal_math_ld_product_exact(e, ln2_lo);
        const struct dj_internal_math_ld_pair head =
            d_internal_math_ld_two_sum(e * ln2_hi, log_m.high);
        const struct dj_internal_math_ld_pair top =
            d_internal_math_ld_two_sum(head.high, el.high);

        return d_internal_math_ld_two_sum(
            top.high, top.low + (head.low + (el.low + log_m.low)));
    }
}

// 1.6    The exponential of a pair, past long double's precision
//------------------------------------------------------------------------------
// exp(w), w = w_hi + w_lo, as section 1.1 of exponential.h has exp(x), but in
// pairs throughout: r = w - k ln 2 kept as a pair, and exp(r) = 1 + r + r^2
// (1/2 + r (1/6 + r (1/24 + r (1/120 + r g)))), |r| <= ln 2 / 2, in pairs
// to r^5 -- the coefficients' heads of 32 bits, exact as written -- with g =
// (1/720) (1 + r/7 (1 + r/8 (...))) in long double: r^6 times its error lies
// past 2^-20 of the value, and the whole is good to about 2^-(P + 18).
// 1 + r + r^2 (...) is then three parts, exact; the first two are rounded
// together, and where that lands on a midpoint the third decides -- so that
// the pair's high part is the value correctly rounded to long double
// wherever the pair is that close. 2^v, v a pair, is exp(v ln 2) by the
// same reduction, v - k exact.

// d_internal_math_ld_exp_accurate
//   exp(_r.high + _r.low), |_r.high| <= ln 2 / 2 (a last place past it does
// no harm), as high + low to about 2^-(P + 18): high the value rounded once.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_exp_accurate(
    struct dj_internal_math_ld_pair _r
)
{
    // 1/6, 1/24 and 1/120: heads of 32 bits, exact as written
    const struct dj_internal_math_ld_pair c6 = d_internal_math_ld_pair_of(
        0.166666666627861559391021728515625L,
        3.880510727564493815104166666666666666667e-11L);
    const struct dj_internal_math_ld_pair c24 = d_internal_math_ld_pair_of(
        0.04166666665696538984775543212890625L,
        9.701276818911234537760416666666666666667e-12L);
    const struct dj_internal_math_ld_pair c120 = d_internal_math_ld_pair_of(
        0.00833333333139307796955108642578125L,
        1.940255363782246907552083333333333333333e-12L);
    // r^2 as a pair: r r exactly, and 2 r r_lo below it
    const struct dj_internal_math_ld_pair rr =
        d_internal_math_ld_product_exact(_r.high, _r.high);
    const struct dj_internal_math_ld_pair r2 =
        d_internal_math_ld_two_sum(rr.high,
                                   rr.low + ((2.0L * _r.high) * _r.low));
    struct dj_internal_math_ld_pair series = { 0.0L, 0.0L };
    long double                     g      = 1.0L;
    int                             n      = 0;

    // 720 g = 1 + r/7 (1 + r/8 (...)), from its 26th term in
    for (n = 26; n >= 7; --n)
    {
        g = 1.0L + ((_r.high / (long double)n) * g);
    }

    // 1/2 + r (1/6 + r (1/24 + r (1/120 + r g))), in pairs
    series = d_internal_math_ld_pair_sum(
        c120, d_internal_math_ld_pair_product(
                  _r, d_internal_math_ld_pair_of(g / 720.0L, 0.0L)));
    series = d_internal_math_ld_pair_sum(
        c24, d_internal_math_ld_pair_product(_r, series));
    series = d_internal_math_ld_pair_sum(
        c6, d_internal_math_ld_pair_product(_r, series));
    series = d_internal_math_ld_pair_sum(
        d_internal_math_ld_pair_of(0.5L, 0.0L),
        d_internal_math_ld_pair_product(_r, series));

    {
        // exp(r) - 1 = r + r^2 series; then 1 + it as three exact parts:
        // one.high + rest.high + rest.low
        const struct dj_internal_math_ld_pair tail =
            d_internal_math_ld_pair_sum(
                _r, d_internal_math_ld_pair_product(r2, series));
        const struct dj_internal_math_ld_pair one =
            d_internal_math_ld_two_sum(1.0L, tail.high);
        const struct dj_internal_math_ld_pair rest =
            d_internal_math_ld_two_sum(one.low, tail.low);
        // the first two rounded together, and what that left (fast
        // two-sum: rest.high lies below one.high's last place)
        long double high = one.high + rest.high;
        long double left = rest.high - (high - one.high);
        struct dj_internal_math_ld_pair result = { 1.0L, 0.0L };

        // on a midpoint, the rounding went the way rest.low does not:
        // a whole gap the other way, and what is left changes sign
        if ( (left != 0.0L) &&
             (rest.low != 0.0L) &&
             ((left > 0.0L) == (rest.low > 0.0L)) )
        {
            const long double next = high + (left + left);

            if ((next - high) == (left + left))
            {
                high = next;
                left = -left;
            }
        }

        result.high = high;
        result.low  = left + rest.low;

        return result;
    }
}

// d_internal_math_ld_exp_pair_split_of
//   exp(_w.high + _w.low) as (y + y_lo) 2^k, for |_w.high| below 2^14: k
// fits an int.
D_MATH_FN struct dj_internal_math_ld_exp_split
d_internal_math_ld_exp_pair_split_of(
    struct dj_internal_math_ld_pair _w
)
{
    const long double log2e  = 1.442695040888963407359924681001892137L;
    // ln 2 = ln2_hi + ln2_lo, ln2_hi cut to 32 bits: exact as written
    const long double ln2_hi = 0.69314718036912381649017333984375L;
    const long double ln2_lo = 1.908214929270587816144265680755001344e-10L;
    const long double t      = _w.high * log2e;
    const int         k      = (int)((t < 0.0L) ? (t - 0.5L) : (t + 0.5L));
    const long double kk     = (long double)k;
    // r = w - k ln 2: w_hi - k ln2_hi exact (Sterbenz's lemma), k ln2_lo
    // exactly by Dekker's product, and every error kept
    const struct dj_internal_math_ld_pair c =
        d_internal_math_ld_product_exact(kk, ln2_lo);
    const struct dj_internal_math_ld_pair d =
        d_internal_math_ld_two_sum(_w.high - (kk * ln2_hi), -c.high);
    const struct dj_internal_math_ld_pair y =
        d_internal_math_ld_exp_accurate(
            d_internal_math_ld_two_sum(d.high,
                                       (d.low - c.low) + _w.low));
    struct dj_internal_math_ld_exp_split result = { 1.0L, 0.0L, 0 };

    result.y    = y.high;
    result.y_lo = y.low;
    result.k    = k;

    return result;
}

// d_internal_math_ld_exp2_pair_split_of
//   2^(_v.high + _v.low) as (y + y_lo) 2^k, for |_v.high| below 2^15: k
// fits an int.
D_MATH_FN struct dj_internal_math_ld_exp_split
d_internal_math_ld_exp2_pair_split_of(
    struct dj_internal_math_ld_pair _v
)
{
    // ln 2 = ln2_hi + ln2_lo, ln2_hi cut to 32 bits: exact as written
    const long double ln2_hi = 0.69314718036912381649017333984375L;
    const long double ln2_lo = 1.908214929270587816144265680755001344e-10L;
    const int         k      = (int)((_v.high < 0.0L) ? (_v.high - 0.5L)
                                                      : (_v.high + 0.5L));
    // r = v - k: v_hi - k exact, by Sterbenz's lemma or as v_hi itself
    const struct dj_internal_math_ld_pair r =
        d_internal_math_ld_two_sum(_v.high - (long double)k, _v.low);
    // r ln 2: r_hi ln2_hi exactly, the rest below it
    const struct dj_internal_math_ld_pair p =
        d_internal_math_ld_product_exact(r.high, ln2_hi);
    const struct dj_internal_math_ld_pair y =
        d_internal_math_ld_exp_accurate(
            d_internal_math_ld_two_sum(
                p.high,
                p.low + ((r.high * ln2_lo) + (r.low * (ln2_hi + ln2_lo)))));
    struct dj_internal_math_ld_exp_split result = { 1.0L, 0.0L, 0 };

    result.y    = y.high;
    result.y_lo = y.low;
    result.k    = k;

    return result;
}

// 1.7    Exact powers
//------------------------------------------------------------------------------
// x^y can be exact, or lie on a family's midpoint where no approximation can
// round it, only for x a power of 2 -- 2^(e y), e y exact -- or for y = n /
// 2^k past 1, x a perfect 2^k-th power: then x^y = r^n, r = x^(1/2^k), every
// square root exact. With r = m 2^e as the logarithm has it (exponential.h),
// r^n = m^n 2^(e n), and m^n lies within 2^(n/2) of 1, far from both ends
// of the range. A family's midpoint needs at most one bit past long
// double's precision, and m^(n-1) fewer bits than m^n, so m^(n-1) is then a
// long double, and m^n = m^(n-1) m is exact as a pair by Dekker's product:
// (y + y_lo) 2^k as the exponential has it, for the family's one rounding,
// into the subnormals included. k is at most 6: the 2^k-th power of an odd
// number past 1 has 2^k log2(3) bits, more than 113 from k = 7. Below 1, y =
// n / 2^k gives r^n with fewer bits than x, never on a midpoint; a negative
// y, 1 / r^|n|, is exact only for a power of 2.

// d_internal_math_ld_is_integer
//   whether _v is an integer: past 2^(P - 1) every long double is, and below
// it adding 2^(P - 1) and taking it away again rounds _v to one. NaN is
// none, decided before any arithmetic, which would yield a NaN: no constant
// expression under Clang.
D_MATH_FN bool
d_internal_math_ld_is_integer(
    long double _v
)
{
    const long double big = 1.0L / LDBL_EPSILON;  // 2^(P - 1)
    const long double a   = (_v < 0.0L) ? -_v : _v;

    if (_v != _v)
    {
        return false;
    }

    if (a >= big)
    {
        return true;
    }

    return (((a + big) - big) == a);
}

// d_internal_math_ld_is_odd
//   whether _v is an odd integer: none is past 2^P.
D_MATH_FN bool
d_internal_math_ld_is_odd(
    long double _v
)
{
    const long double a = (_v < 0.0L) ? -_v : _v;

    if (a >= (2.0L / LDBL_EPSILON))
    {
        return false;
    }

    return ( d_internal_math_ld_is_integer(a) &&
             !d_internal_math_ld_is_integer(a / 2.0L) );
}

// dj_internal_math_ld_exact_power
//   struct: whether x^y is exact, and if so x^y as (y + y_lo) 2^k.
struct dj_internal_math_ld_exact_power
{
    bool                                 exact;
    struct dj_internal_math_ld_exp_split value;
};

// d_internal_math_ld_exact_power_of
//   _a^_y exactly, as (y + y_lo) 2^k, where the section finds it so: _a
// positive and neither 1 nor a power of 2, _y past 1. Otherwise exact is
// false.
D_MATH_FN struct dj_internal_math_ld_exact_power
d_internal_math_ld_exact_power_of(
    long double _a,
    long double _y
)
{
    struct dj_internal_math_ld_exact_power result =
        { false, { 1.0L, 0.0L, 0 } };
    long double root = _a;
    long double n    = _y;
    long double i    = 0.0L;
    int         k    = 0;

    if (!(_y > 1.0L))
    {
        return result;
    }

    // y = n / 2^k, n an integer
    while ( (k < 6) &&
            !d_internal_math_ld_is_integer(n) )
    {
        n += n;
        ++k;
    }

    if (!d_internal_math_ld_is_integer(n))
    {
        return result;
    }

    // r = a^(1/2^k): k square roots, each exact or none is
    while (k > 0)
    {
        const long double c = d_math_ld_sqrt_c(root);
        const struct dj_internal_math_ld_pair square =
            d_internal_math_ld_product_exact(c, c);

        if ( (square.high != root) ||
             (square.low != 0.0L) )
        {
            return result;
        }

        root = c;
        --k;
    }

    {
        // r = m 2^e: m^n = m^(n-1) m, each power below it exact; each has a
        // bit more than the last -- m is no power of 2 -- so that some 113
        // steps at most are taken
        const struct dj_internal_math_ld_log_arg r =
            d_internal_math_ld_log_arg_of(root);
        long double power = r.m;

        if (r.m == 1.0L)
        {
            return result;
        }

        for (i = 1.0L; i < n; i += 1.0L)
        {
            const struct dj_internal_math_ld_pair p =
                d_internal_math_ld_product_exact(power, r.m);

            if ((i + 1.0L) >= n)
            {
                // m^n = M 2^j, M in [1/sqrt(2), sqrt(2)]: y = M, k = e n + j
                const struct dj_internal_math_ld_log_arg top =
                    d_internal_math_ld_log_arg_of(p.high);

                result.exact      = true;
                result.value.y    = top.m;
                result.value.y_lo = p.low *
                                    d_internal_math_ld_pow2(-top.exponent);
                result.value.k    = ((int)n * r.exponent) + top.exponent;

                return result;
            }

            if (p.low != 0.0L)
            {
                return result;
            }

            power = p.high;
        }
    }

    return result;
}

// 1.8    The root of a sum of two squares
//------------------------------------------------------------------------------
// hypot(a, b) is settled as the cube root is (section 1.2): by the sign of
// (t + h)^2 - (a^2 + b^2), t the candidate and t + h a midpoint beside it.
// a, b, t and h scaled by one power of 2 -- a into [1, 2), b then no less
// than 2^-(P' + 2), P' the family's precision -- every term is exact: the
// squares as pairs by Dekker's product, 2th and h^2 since h is a power of 2.
// The terms summed in long double give the sign wherever the sum lies past
// its error bound; within it, they are added exactly instead. The sign is 0
// only where the root lies on the midpoint, as a Pythagorean triple can put
// it: a^2 + b^2 is then the midpoint's square exactly.

// d_internal_math_ld_scale
//   _v 2^_e, by two powers of 2 of half _e each: exact where the result and
// the first product are normal, for |_e| up to 2 (LDBL_MAX_EXP - 1).
D_MATH_FN long double
d_internal_math_ld_scale(
    long double _v,
    int         _e
)
{
    return (_v * d_internal_math_ld_pow2(_e / 2)) *
           d_internal_math_ld_pow2(_e - (_e / 2));
}

// d_internal_math_ld_hypot_root
//   sqrt(_v), _v in [1, 8], to within a long double's last place or two: a
// quadratic within 1.7% of it, then five steps of Newton's iteration, each
// squaring the relative error -- past a quad's precision.
D_MATH_FN long double
d_internal_math_ld_hypot_root(
    long double _v
)
{
    long double root = 0.5998L + (_v * (0.437L - (_v * 0.0205L)));
    int         i    = 0;

    for (i = 0; i < 5; ++i)
    {
        root = 0.5L * (root + (_v / root));
    }

    return root;
}

// d_internal_math_ld_hypot_sign_exact
//   the sign of (_t + _h)^2 - (a^2 + b^2), _a2 and _b2 the squares as
// Dekker's pairs, from its eight terms summed exactly.
D_MATH_FN int
d_internal_math_ld_hypot_sign_exact(
    struct dj_internal_math_ld_pair _a2,
    struct dj_internal_math_ld_pair _b2,
    long double                     _t,
    long double                     _h
)
{
    const struct dj_internal_math_ld_pair t2 =
        d_internal_math_ld_product_exact(_t, _t);
    struct dj_internal_math_ld_expansion sum = { { 0.0L }, 0 };

    sum = d_internal_math_ld_expansion_grow(sum, -_a2.high);
    sum = d_internal_math_ld_expansion_grow(sum, -_a2.low);
    sum = d_internal_math_ld_expansion_grow(sum, -_b2.high);
    sum = d_internal_math_ld_expansion_grow(sum, -_b2.low);
    sum = d_internal_math_ld_expansion_grow(sum, t2.high);
    sum = d_internal_math_ld_expansion_grow(sum, t2.low);
    sum = d_internal_math_ld_expansion_grow(sum, (2.0L * _t) * _h);
    sum = d_internal_math_ld_expansion_grow(sum, _h * _h);

    if (sum.count == 0)
    {
        return 0;
    }

    return (sum.part[sum.count - 1] > 0.0L) ? 1 : -1;
}

// d_internal_math_ld_hypot_sign
//   the sign of (_t + _h)^2 - (a^2 + b^2), _a2 and _b2 the squares as
// Dekker's pairs, for a in [1, 2), b in [2^-(P' + 2), a] and _h a power of
// 2 or its negation: the terms summed in long double, by seven roundings,
// each off by at most half a last place of the terms' magnitudes summed --
// t^2 and a^2 counted by their difference, which cancels -- decide it where
// the sum lies past 16 such half places; the exact sum otherwise.
D_MATH_FN int
d_internal_math_ld_hypot_sign(
    struct dj_internal_math_ld_pair _a2,
    struct dj_internal_math_ld_pair _b2,
    long double                     _t,
    long double                     _h
)
{
    const struct dj_internal_math_ld_pair t2 =
        d_internal_math_ld_product_exact(_t, _t);
    const long double g   = t2.high - _a2.high;
    const long double th  = (2.0L * _t) * _h;
    const long double hh  = _h * _h;
    const long double sum = ((g - _b2.high) + (th + hh)) +
                            ((t2.low - _a2.low) - _b2.low);
    // the terms' magnitudes, t^2 - a^2 as g
    const long double abs_g  = (g < 0.0L) ? -g : g;
    const long double abs_th = (th < 0.0L) ? -th : th;
    const long double abs_tl = (t2.low < 0.0L) ? -t2.low : t2.low;
    const long double abs_al = (_a2.low < 0.0L) ? -_a2.low : _a2.low;
    const long double abs_bl = (_b2.low < 0.0L) ? -_b2.low : _b2.low;
    const long double bound  = ( abs_g + _b2.high + abs_th + hh + abs_tl +
                                 abs_al + abs_bl ) *
                               (8.0L * LDBL_EPSILON);

    if (sum > bound)
    {
        return 1;
    }

    if (sum < -bound)
    {
        return -1;
    }

    return d_internal_math_ld_hypot_sign_exact(_a2, _b2, _t, _h);
}
"""

POWER_TEMPLATE = r"""
//==============================================================================
// @SECTION@.  THE @UPPER@ FAMILY (_@S@)
//==============================================================================


// @SECTION@.1    Internal helpers
//------------------------------------------------------------------------------

// d_internal_math_@S@_cbrt_unit
//   the correctly rounded cube root of _m in [1, 8): Newton's, in long
// double (section 1.3), rounded to the family, then the last bit settled --
// up while the root lies past the midpoint above, down while it lies short of
// the one below, the gap below a power of 2 being half the gap above.
D_MATH_FN @T@
d_internal_math_@S@_cbrt_unit(
    @T@ _m
)
{
    const long double m = (long double)_m;
    @T@ root = (@T@)d_internal_math_ld_cbrt_newton(m);
    int tries = 0;

    for (tries = 0; tries < 3; ++tries)
    {
        const struct dj_internal_math_@S@_binade b =
            d_internal_math_@S@_binade_of(root);
        // half the gap above root, and half the gap below it
        const long double above = (long double)(b.power * @EPS@) / 2.0L;
        const long double below = (root == b.power) ? (above / 2.0L)
                                                    : above;

        if (d_internal_math_ld_cube_sign(m, (long double)root, above) < 0)
        {
            root = d_math_@S@_next_up(root);
        }
        else if (d_internal_math_ld_cube_sign(m,
                                              (long double)root,
                                              -below) > 0)
        {
            root = d_math_@S@_next_down(root);
        }
        else
        {
            break;
        }
    }

    return root;
}

// d_internal_math_@S@_hypot_settle
//   the correctly rounded root of (_a^2 + _b^2) 2^(-2 _s), _a and _b the
// larger and the smaller scaled by 2^_s so that _a lies in [1, 2), from the
// candidate _t, a value of the family within a few last places of the root:
// up while the root lies past the midpoint above t, down while it lies
// short of the one below -- half the family's last place at t, the
// subnormals' spacing below the normal range, and below a power of 2 half
// of it -- and on a midpoint to the even neighbour (section 1.8). +inf on
// or past the largest finite value's midpoint above.
D_MATH_FN @T@
d_internal_math_@S@_hypot_settle(
    long double _a,
    long double _b,
    long double _t,
    int         _s
)
{
    const struct dj_internal_math_ld_pair a2 =
        d_internal_math_ld_product_exact(_a, _a);
    const struct dj_internal_math_ld_pair b2 =
        d_internal_math_ld_product_exact(_b, _b);
    @T@ t = (@T@)_t;
    int tries = 0;

    for (tries = 0; tries < 4; ++tries)
    {
        // t scaled lies in [1/2, 4): its binade 2^et, et = k - s, and below
        // the normal range the least normal one, whose last place is the
        // subnormals' spacing: half that last place, and half the one below
        // t, scaled by 2^_s
        const long double ts    = d_internal_math_ld_scale((long double)t, _s);
        const int         k     = (ts >= 2.0L) ? 1 : ((ts >= 1.0L) ? 0 : -1);
        const int         et    = k - _s;
        const int         least = @EMIN@ - 1;
        const int         place = ((et < least) ? least : et) - @P@;
        const long double up    = d_internal_math_ld_pow2(place + _s);
        const long double down  =
            ( (et > least) &&
              (ts == d_internal_math_ld_pow2(k)) ) ? (up / 2.0L) : up;
        // t over its last place, an integer whose parity is t's last bit
        const bool odd   = d_internal_math_ld_is_odd(ts / (up + up));
        const int  above = d_internal_math_ld_hypot_sign(a2, b2, ts, up);

        if ( (above < 0) ||
             ( (above == 0) && odd ) )
        {
            // past the midpoint above, or on it from an odd t
            if (t == @MAX@)
            {
                return d_internal_math_@S@_infinity();
            }

            t = d_math_@S@_next_up(t);

            if (above == 0)
            {
                break;
            }
        }
        else if (above == 0)
        {
            break;
        }
        else
        {
            const int below =
                d_internal_math_ld_hypot_sign(a2, b2, ts, -down);

            if ( (below > 0) ||
                 ( (below == 0) && odd ) )
            {
                // short of the midpoint below, or on it from an odd t
                t = d_math_@S@_next_down(t);

                if (below == 0)
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
    }

    return t;
}

// @SECTION@.2    Cube root
//------------------------------------------------------------------------------
// cbrt_c is the correctly rounded cube root in pure arithmetic, a constant
// expression from C++14: |x| = m 2^(3q) with m in [1, 8), exactly, the root
// of m settled to the family's last bit (section @SECTION@.1) and scaled by
// 2^q. It is odd; the zeros, the infinities and NaN are their own. cbrt is the
// C library's, which need not be correctly rounded; C++ has it from C++11,
// and before, cbrt is cbrt_c.

D_MATH_FN @T@
d_math_@S@_cbrt_c(
    @T@ _x
)
{
    // NaN, the zeros and the infinities are their own cube roots
    if ( (_x != _x) ||
         (_x == (@T@)0) ||
         (_x > @MAX@) ||
         (_x < -@MAX@) )
    {
        return _x;
    }

    {
        const bool negative = (_x < (@T@)0);
        const @T@ a = negative ? -_x : _x;
        // a subnormal is first scaled into the normal range, by a cube
        const int lift = (a < @MIN@) ? (3 * ((@P@ + 2) / 3)) : 0;
        const @T@ value =
            (lift > 0) ? (a * d_internal_math_@S@_pow2(lift)) : a;
        // value = M 2^E with M in [1, 2), and E = 3q + j with j in 0 to 2
        const struct dj_internal_math_@S@_binade b =
            d_internal_math_@S@_binade_of(value);
        const int q = (b.exponent >= 0) ? (b.exponent / 3)
                                        : -((2 - b.exponent) / 3);
        const @T@ m =
            (value / b.power) * d_internal_math_@S@_pow2(b.exponent - (3 * q));
        // the root of m, in [1, 2], times 2^q: a normal value, exactly
        const @T@ root =
            d_internal_math_@S@_cbrt_unit(m) *
            d_internal_math_@S@_pow2(q - (lift / 3));

        if (negative)
        {
            return -root;
        }

        return root;
    }
}

D_MATH_FN_RT @T@
d_math_@S@_cbrt(
    @T@ _x
)
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    return std::cbrt(_x);
#elif defined(__cplusplus)
    // C++ before C++11 has no cbrt: the pure-arithmetic form serves
    return d_math_@S@_cbrt_c(_x);
#else
    return @CCBRT@(_x);
#endif
}

// @SECTION@.3    Power
//------------------------------------------------------------------------------
// pow_c is x^y in pure arithmetic, a constant expression from C++14:
// exp(y log x), the logarithm past long double's precision (section 1.5),
// its product with y as a pair, and the pair's exponential (section 1.6)
// rounded once to the family -- correctly but where x^y lies within some
// 2^-(P + 16) of a midpoint, P long double's precision. The results that
// can lie on one are computed exactly (section 1.7): a power of 2 to any y,
// as 2^(e y); and for y past 1, an integer power or a root's, as a pair.
// x^1 is x, x^(1/2) sqrt_c's, and x^-1 the quotient 1/m and its remainder,
// both exact, scaled by 2^-e. The rest is C's (Annex F): x^+-0 = 1 and 1^y =
// 1 for any x and y, NaN too; NaN otherwise gives NaN; a negative x has a
// power only for an integer y, negative for an odd one, and NaN otherwise;
// +-0 and +-inf go by y's sign, keeping their own only for an odd y; |x| = 1
// to +-inf gives 1, and |x| below or past 1 to +-inf gives +0 or +inf. pow
// is the C library's.

D_MATH_FN @T@
d_math_@S@_pow_c(
    @T@ _x,
    @T@ _y
)
{
    const long double y   = (long double)_y;
    const bool        odd = d_internal_math_ld_is_odd(y);
    const @T@ inf = d_internal_math_@S@_infinity();

    // x^+-0 = 1 and 1^y = 1, NaN or not
    if ( (_y == (@T@)0) ||
         (_x == (@T@)1) )
    {
        return (@T@)1;
    }

    // NaN otherwise gives NaN: the argument itself, as atan2_c's, since
    // Clang's constant evaluation takes no arithmetic that yields a NaN
    if (_x != _x)
    {
        return _x;
    }

    if (_y != _y)
    {
        return _y;
    }

    // the zeros: +-inf by a negative y and +-0 by a positive one, the sign
    // kept only by an odd y
    if (_x == (@T@)0)
    {
        const bool minus = odd && d_internal_math_@S@_negative(_x);

        if (_y < (@T@)0)
        {
            return minus ? -inf : inf;
        }

        return minus ? -(@T@)0 : (@T@)0;
    }

    // y infinite: 1 for |x| = 1, else +0 or +inf as |x|^y tends
    if ( (_y > @MAX@) ||
         (_y < -@MAX@) )
    {
        const @T@ a = (_x < (@T@)0) ? -_x : _x;

        if (a == (@T@)1)
        {
            return (@T@)1;
        }

        if ((a < (@T@)1) == (_y > (@T@)0))
        {
            return (@T@)0;
        }

        return inf;
    }

    // x infinite: +0 by a negative y and +inf by a positive one, -inf's sign
    // kept only by an odd y
    if ( (_x > @MAX@) ||
         (_x < -@MAX@) )
    {
        const bool minus = odd && (_x < (@T@)0);

        if (_y < (@T@)0)
        {
            return minus ? -(@T@)0 : (@T@)0;
        }

        return minus ? -inf : inf;
    }

    // a negative x has a power only for an integer y
    if ( (_x < (@T@)0) &&
         !d_internal_math_ld_is_integer(y) )
    {
        return d_internal_math_@S@_nan();
    }

    {
        const @T@ at = (_x < (@T@)0) ? -_x : _x;
        const long double a = (long double)at;
        // |x| = m 2^e, m in [1/sqrt(2), sqrt(2)], exactly
        const struct dj_internal_math_ld_log_arg arg =
            d_internal_math_@S@_log_arg_of(at);
        // 2^v is +inf from v = top and below half the least subnormal from
        // v = bottom; past reach, either way, in v or in log's units
        const long double top    = (long double)(@EMAX@ + 1);
        const long double bottom = (long double)(@EMIN@ - 2) -
                                   (long double)@P@;
        const long double reach  = top - bottom;
        @T@ r = (@T@)0;

        if (a == 1.0L)
        {
            // (-1)^y, y an integer
            r = (@T@)1;
        }
        else if (_y == (@T@)1)
        {
            r = at;
        }
        else if (_y == (@T@)0.5)
        {
            r = d_math_@S@_sqrt_c(at);
        }
        else if (arg.m == 1.0L)
        {
            // a power of 2: 2^(e y), e y exact
            const long double e = (long double)arg.exponent;

            if ( (y > reach) ||
                 (y < -reach) )
            {
                r = ((e > 0.0L) == (y > 0.0L)) ? inf : (@T@)0;
            }
            else
            {
                const struct dj_internal_math_ld_pair v =
                    d_internal_math_ld_product_exact(e, y);

                if (v.high > top)
                {
                    r = inf;
                }
                else if (v.high < bottom)
                {
                    r = (@T@)0;
                }
                else
                {
                    r = d_internal_math_@S@_exp_scale(
                        d_internal_math_ld_exp2_pair_split_of(v));
                }
            }
        }
        else if (_y == (@T@)-1)
        {
            // 1/x = (1/m) 2^-e, 1/m in [1/sqrt(2), sqrt(2)]: the quotient
            // and its remainder, both exact
            const long double quotient = 1.0L / arg.m;
            const struct dj_internal_math_ld_pair p =
                d_internal_math_ld_product_exact(quotient, arg.m);
            struct dj_internal_math_ld_exp_split split = { 1.0L, 0.0L, 0 };

            split.y    = quotient;
            split.y_lo = ((1.0L - p.high) - p.low) / arg.m;
            split.k    = -arg.exponent;
            r          = d_internal_math_@S@_exp_scale(split);
        }
        else
        {
            const struct dj_internal_math_ld_exact_power exact =
                d_internal_math_ld_exact_power_of(a, y);

            if (exact.exact)
            {
                r = d_internal_math_@S@_exp_scale(exact.value);
            }
            else
            {
                // exp(y log x), y log x as a pair: past the reach, a zero or
                // an infinity by its sign, which keeps the product finite
                const struct dj_internal_math_ld_pair l =
                    d_internal_math_ld_log_accurate_of(arg.exponent, arg.m);
                const long double size = (l.high < 0.0L) ? -l.high : l.high;

                if ( (y > (reach / size)) ||
                     (y < -(reach / size)) )
                {
                    r = ((l.high > 0.0L) == (y > 0.0L)) ? inf : (@T@)0;
                }
                else
                {
                    const struct dj_internal_math_ld_pair yl =
                        d_internal_math_ld_product_exact(y, l.high);
                    const struct dj_internal_math_ld_pair w =
                        d_internal_math_ld_two_sum(yl.high,
                                                   yl.low + (y * l.low));

                    // as exp_c: well past the largest finite result, and
                    // well below half the least subnormal
                    if (w.high > (top * 0.6932L))
                    {
                        r = inf;
                    }
                    else if (w.high < (bottom * 0.6932L))
                    {
                        r = (@T@)0;
                    }
                    else
                    {
                        r = d_internal_math_@S@_exp_scale(
                            d_internal_math_ld_exp_pair_split_of(w));
                    }
                }
            }
        }

        if ( (_x < (@T@)0) &&
             odd )
        {
            return -r;
        }

        return r;
    }
}

D_MATH_FN_RT @T@
d_math_@S@_pow(
    @T@ _x,
    @T@ _y
)
{
#ifdef __cplusplus
    return std::pow(_x, _y);
#else
    return @CPOW@(_x, _y);
#endif
}

// @SECTION@.4    Hypotenuse
//------------------------------------------------------------------------------
// hypot_c is sqrt(x^2 + y^2) correctly rounded, in pure arithmetic, a
// constant expression from C++14, and with no overflow or underflow on the
// way: |x| and |y| scaled by one power of 2, the larger into [1, 2), the
// root of their squares' sum taken in long double, put on the family's
// values, and its last bit settled exactly (sections @SECTION@.1 and 1.8) --
// on the subnormals' spacing below the normal range, so that a result there
// is rounded once, and on a midpoint, where a Pythagorean triple can put it,
// to even. Where the smaller lies below 2^-(P + 2) of the larger it moves
// the root by less than a quarter of the larger's last place, and the result
// is the larger. The rest is C's (Annex F): an infinity gives +inf, the
// other NaN or not; NaN otherwise gives NaN; hypot(x, +-0) is |x|. hypot is
// the C library's; C++ has it from C++11, and before, hypot is hypot_c.

D_MATH_FN @T@
d_math_@S@_hypot_c(
    @T@ _x,
    @T@ _y
)
{
    // an infinity gives +inf, the other NaN or not
    if ( (_x > @MAX@)  ||
         (_x < -@MAX@) ||
         (_y > @MAX@)  ||
         (_y < -@MAX@) )
    {
        return d_internal_math_@S@_infinity();
    }

    // NaN otherwise gives NaN: the argument itself, as in pow_c
    if (_x != _x)
    {
        return _x;
    }

    if (_y != _y)
    {
        return _y;
    }

    {
        const @T@ ax = (_x < (@T@)0) ? -_x : _x;
        const @T@ ay = (_y < (@T@)0) ? -_y : _y;
        const @T@ a  = (ax < ay) ? ay : ax;
        const @T@ b  = (ax < ay) ? ax : ay;

        // hypot(x, +-0) is |x|, and +0 for either zero
        if (b == (@T@)0)
        {
            return a + (@T@)0;
        }

        {
            // a = M 2^E, M in [1, 2), a subnormal lifted into the normal
            // range first; 2^s takes a into [1, 2), and b with it, exactly
            const int lift = (a < @MIN@) ? @P@ : 0;
            const struct dj_internal_math_@S@_binade e =
                d_internal_math_@S@_binade_of(
                    (lift > 0) ? (a * d_internal_math_@S@_pow2(lift)) : a);
            const int s = lift - e.exponent;
            const long double sa = d_internal_math_ld_scale((long double)a, s);
            const long double sb = d_internal_math_ld_scale((long double)b, s);

            // b below 2^-(P + 2) of a: the root is a
            if (sb < d_internal_math_ld_pow2(-(@P@ + 2)))
            {
                return a;
            }

            {
                // the root to within a long double's last place or two, put
                // on the family's values -- the largest at most, which only
                // a in the top binade can pass
                const long double root =
                    d_internal_math_ld_hypot_root((sa * sa) + (sb * sb));
                const bool top =
                    ( ((e.exponent - lift) == (@EMAX@ - 1)) &&
                      (root >= d_internal_math_ld_scale((long double)@MAX@,
                                                        s)) );
                const @T@ t =
                    top ? @MAX@
                        : (@T@)d_internal_math_ld_scale(root, -s);

                return d_internal_math_@S@_hypot_settle(
                    sa, sb, (long double)t, s);
            }
        }
    }
}

D_MATH_FN_RT @T@
d_math_@S@_hypot(
    @T@ _x,
    @T@ _y
)
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    return std::hypot(_x, _y);
#elif defined(__cplusplus)
    // C++ before C++11 has no hypot: the pure-arithmetic form serves
    return d_math_@S@_hypot_c(_x, _y);
#else
    return @CHYPOT@(_x, _y);
#endif
}
"""

POWER_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE KERNELS (long double)
    -------------------------
    1.  Exact sums of many terms
    2.  The sign of a cube about a midpoint
    3.  The cube root's approximation
    4.  Sums and products of pairs
    5.  The logarithm, past long double's precision
    6.  The exponential of a pair, past long double's precision
    7.  Exact powers
    8.  The root of a sum of two squares
2.  THE FLOAT FAMILY (_f)
    ---------------------
    1.  Internal helpers
    2.  Cube root
    3.  Power
    4.  Hypotenuse
3.  THE DOUBLE FAMILY (_d)
    ----------------------
    1.  Internal helpers
    2.  Cube root
    3.  Power
    4.  Hypotenuse
4.  THE LONG DOUBLE FAMILY (_ld)
    ----------------------------
    1.  Internal helpers
    2.  Cube root
    3.  Power
    4.  Hypotenuse
5.  ONE NAME FOR EVERY FAMILY (C++)
    -------------------------------
*/

#ifndef DJINTERP_C_MATH_POWER_H
#define DJINTERP_C_MATH_POWER_H 1

// std
#ifdef __cplusplus
    #include <cmath>         // std::cbrt, std::pow, std::hypot
#endif  // __cplusplus
#include <float.h>           // FLT_MAX ... LDBL_MAX, LDBL_EPSILON
#include <math.h>            // cbrtf ... hypotl
// djinterp
#include "./elementary.h"    // d_internal_math_ld_two_sum, *_product_exact
#include "./exponential.h"   // d_internal_math_*_exp_scale, *_log_arg_of
#include "./float_order.h"   // d_internal_math_*_binade_of, d_math_*_next_up
#include "./math_common.h"   // D_MATH_FN
#include "./tangent.h"       // d_internal_math_*_negative
"""

POWER_TAIL = r"""

#endif  // DJINTERP_C_MATH_POWER_H
"""

POWER_OVERLOADS = [
    ("D_MATH_FN", "@T@", "d_math_cbrt_c", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_cbrt", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_pow_c", [("@T@", "_x"), ("@T@", "_y")]),
    ("D_MATH_FN_RT", "@T@", "d_math_pow", [("@T@", "_x"), ("@T@", "_y")]),
    ("D_MATH_FN", "@T@", "d_math_hypot_c", [("@T@", "_x"), ("@T@", "_y")]),
    ("D_MATH_FN_RT", "@T@", "d_math_hypot", [("@T@", "_x"), ("@T@", "_y")]),
]


def power():
    lines = [
        "Powers and roots: the C core's cube root, power and hypotenuse, for",
        "float, double and long double.",
        "  GENERATED by scripts/gen_c_math_float.py from one text for the three",
        "families: edit the generator, not this file.",
        "",
        "  Each function comes twice, as in elementary.h. The _c form is pure",
        "arithmetic, so the same text is a constant expression from C++14; the",
        "plain form calls the C library (std:: in C++), fast at run time. cbrt_c",
        "is correctly rounded, as elementary.h's sqrt_c is: Newton's iteration in",
        "long double, then the family's last bit settled exactly. The library's",
        "cube root need not be -- glibc's differs from it at about half of all",
        "doubles -- and C++ before C++11 has none, so that there cbrt is cbrt_c.",
        "pow_c is exp(y log x) carried some 18 bits past long double, the exact",
        "powers -- integer powers, roots' powers, powers of 2 -- computed",
        "exactly: correctly rounded wherever it has been measured. hypot_c is",
        "correctly rounded, as cbrt_c is: the root of the squares' sum in long",
        "double, its last bit settled by an exact sum, with no overflow or",
        "underflow on the way; C++ before C++11 has no hypot either.",
    ]
    text = banner("power.h", lines, "2026.10.10", "2026.11.07")
    text += "\n" + POWER_HEAD
    text += POWER_KERNELS
    for i, (suffix, ctype, prefix, prose) in enumerate(FAMILIES):
        upper = prose.upper()
        body = POWER_TEMPLATE.replace("@SECTION@", str(i + 2)).replace("@UPPER@", upper)
        body = body.replace("@CCBRT@", {"float": "cbrtf", "double": "cbrt", "long double": "cbrtl"}[ctype])
        body = body.replace("@CPOW@", {"float": "powf", "double": "pow", "long double": "powl"}[ctype])
        body = body.replace("@CHYPOT@", {"float": "hypotf", "double": "hypot", "long double": "hypotl"}[ctype])
        text += instantiate(body, suffix, ctype, prefix, prose)
    text += overload_section("5.  ONE NAME FOR EVERY FAMILY (C++)", POWER_OVERLOADS)
    text += POWER_TAIL
    return text



# -----------------------------------------------------------------------------
# remainder.h
# -----------------------------------------------------------------------------
# C's remainder functions (7.12.10): fmod, remainder and remquo. Exact --
# the remainder of two values of a family is a value of it -- so each
# family's own arithmetic computes it, no kernel in long double and nothing
# rounded.

REMAINDER_TEMPLATE = r"""
//==============================================================================
// @SECTION@.  THE @UPPER@ FAMILY (_@S@)
//==============================================================================


// @SECTION@.1    The truncated remainder
//------------------------------------------------------------------------------
// fmod_c is x - n y, n the quotient x / y truncated toward 0, exactly --
// the remainder of two values of the family is a value of it -- in pure
// arithmetic, a constant expression from C++14. m = |y| 2^j, the largest
// not past |x|, is taken away where it fits, then halved, down to |y|: the
// remainder stays below 2m, so every difference is exact (Sterbenz's lemma).
// Twice as many steps as |x|'s binade lies above |y|'s: some 4,200 at most
// for a double, 66,000 for an x87's long double. The result has x's sign, a
// zero's included. The rest is C's (Annex F): an infinite x, or y zero,
// gives NaN; an infinite y gives x; NaN gives NaN. fmod is the C library's,
// exact too: the two agree bit for bit.

D_MATH_FN @T@
d_math_@S@_fmod_c(
    @T@ _x,
    @T@ _y
)
{
    // NaN gives NaN: the argument itself, as in pow_c
    if (_x != _x)
    {
        return _x;
    }

    if (_y != _y)
    {
        return _y;
    }

    // an infinite x, or y zero: NaN
    if ( (_x > @MAX@)  ||
         (_x < -@MAX@) ||
         (_y == (@T@)0) )
    {
        return d_internal_math_@S@_nan();
    }

    {
        const @T@ a = (_x < (@T@)0) ? -_x : _x;
        const @T@ b = (_y < (@T@)0) ? -_y : _y;
        @T@ r = a;
        @T@ m = b;
        int j = 0;

        // |x| below |y| -- x a zero, or y infinite, among them -- is its own
        // remainder
        if (a < b)
        {
            return _x;
        }

        // m = b 2^j, the largest not past a: doubled while twice it is not
        // past a. a - m is exact or rounded, and rounding keeps the order,
        // so it decides as 2m <= a would, and 2m never overflows
        while (m <= (a - m))
        {
            m += m;
            ++j;
        }

        // take m away where it fits, then halve it, j times, down to b: r <
        // 2m throughout. Counted rather than compared with b, which may be
        // subnormal: an x87 takes some 60 times as long over a step that
        // reads one
        for (;;)
        {
            if (r >= m)
            {
                r -= m;
            }

            if (j == 0)
            {
                break;
            }

            m /= (@T@)2;
            --j;
        }

        return (_x < (@T@)0) ? -r : r;
    }
}

D_MATH_FN_RT @T@
d_math_@S@_fmod(
    @T@ _x,
    @T@ _y
)
{
#ifdef __cplusplus
    return std::fmod(_x, _y);
#else
    return @CFMOD@(_x, _y);
#endif
}

// @SECTION@.2    The rounded remainder
//------------------------------------------------------------------------------
// remainder_c is IEEE's remainder, C's remainder: x - n y, n the quotient
// x / y rounded to the nearest integer, a half to the even one -- exactly,
// so that it lies within |y| / 2 of zero. In pure arithmetic, a constant
// expression from C++14: fmod_c by 2|y| first, which keeps the quotient's
// parity, then |y| taken away where the remainder is past |y| / 2, and
// again where it is past it still -- each difference exact (Sterbenz's
// lemma). A zero result has x's sign. The rest is C's (Annex F): an
// infinite x, or y zero, gives NaN; an infinite y gives x; NaN gives NaN,
// the argument itself. remainder is the C library's, exact too: the two
// agree bit for bit. C++ before C++11 has no remainder, so there it is
// remainder_c.

D_MATH_FN @T@
d_math_@S@_remainder_c(
    @T@ _x,
    @T@ _y
)
{
    // NaN gives NaN: the argument itself
    if (_x != _x)
    {
        return _x;
    }

    if (_y != _y)
    {
        return _y;
    }

    // an infinite x, or y zero: NaN
    if ( (_x > @MAX@)  ||
         (_x < -@MAX@) ||
         (_y == (@T@)0) )
    {
        return d_internal_math_@S@_nan();
    }

    // an infinite y: x, a finite x lying nearer zero than any multiple
    if ( (_y > @MAX@) ||
         (_y < -@MAX@) )
    {
        return _x;
    }

    {
        const @T@ b = (_y < (@T@)0) ? -_y : _y;
        @T@       r = (_x < (@T@)0) ? -_x : _x;

        // |x| below 2b, the quotient's parity kept: where 2b would
        // overflow, |x| is below it already
        if (b <= (@MAX@ / (@T@)2))
        {
            r = d_math_@S@_fmod_c(r, b + b);
        }

        // past b / 2 take b away, and again past it still: a half stays
        // where the quotient is even. Below twice the least normal value
        // b / 2 may not be exact, but 2r is
        if (b < (@MIN@ + @MIN@))
        {
            if ((r + r) > b)
            {
                r -= b;

                if ((r + r) >= b)
                {
                    r -= b;
                }
            }
        }
        else
        {
            const @T@ half = b / (@T@)2;

            if (r > half)
            {
                r -= b;

                if (r >= half)
                {
                    r -= b;
                }
            }
        }

        // x's sign, a zero's included
        return (_x < (@T@)0) ? -r : r;
    }
}

D_MATH_FN_RT @T@
d_math_@S@_remainder(
    @T@ _x,
    @T@ _y
)
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    return std::remainder(_x, _y);
#elif defined(__cplusplus)
    // C++ before C++11 has no remainder: the pure-arithmetic form serves
    return d_math_@S@_remainder_c(_x, _y);
#else
    return @CREMAINDER@(_x, _y);
#endif
}

// @SECTION@.3    The rounded remainder and its quotient
//------------------------------------------------------------------------------
// remquo_c is C's remquo: remainder_c's result, and through _quo the last
// three bits of the quotient n it rounded -- |n| mod 8, 0 to 7, with the
// sign of x / y -- what C asks for at least, and what a reduction by a
// quarter turn wants for its quadrant. Pure arithmetic, a constant
// expression from C++14 where the int _quo points to is the evaluation's
// own, as with C++23's std::remquo: fmod_c by 8|y| first, which keeps the
// quotient's last three bits (where 8|y| would overflow, |x| is below it
// already); then 4|y| and 2|y| taken away where they fit, and |y| as
// remainder_c takes it, each difference exact and each counted. The special
// values are remainder_c's, with the quotient 0. A null _quo stores
// nothing. remquo is the C library's: the same remainder, and a quotient
// with the same last three bits -- glibc's on x86-64 gives 8 or -8 where
// remquo_c gives 0. C++ before C++11 has no remquo, so there it is
// remquo_c.

D_MATH_FN @T@
d_math_@S@_remquo_c(
    @T@ _x,
    @T@ _y,
    @QP@ _quo
)
{
    @T@ result   = _x;
    @QI@ quotient = 0;

    // NaN gives NaN, the argument itself; an infinite x, or y zero, NaN; an
    // infinite y, x, a finite x lying nearer zero than any multiple
    if (_x != _x)
    {
        result = _x;
    }
    else if (_y != _y)
    {
        result = _y;
    }
    else if ( (_x > @MAX@)  ||
              (_x < -@MAX@) ||
              (_y == (@T@)0) )
    {
        result = d_internal_math_@S@_nan();
    }
    else if ( (_y > @MAX@) ||
              (_y < -@MAX@) )
    {
        result = _x;
    }
    else
    {
        const @T@ b = (_y < (@T@)0) ? -_y : _y;
        @T@       r = (_x < (@T@)0) ? -_x : _x;

        // |x| below 8b, the quotient's last three bits kept: where 8b would
        // overflow, |x| is below it already
        if (b <= (@MAX@ / (@T@)8))
        {
            r = d_math_@S@_fmod_c(r, b * (@T@)8);
        }

        // 4b taken away where it fits, then 2b: below 8b and 4b, each
        // difference is exact. Where 4b or 2b would overflow, r is below it
        if ( (b <= (@MAX@ / (@T@)4)) &&
             (r >= (b * (@T@)4)) )
        {
            r        -= b * (@T@)4;
            quotient += 4;
        }

        if ( (b <= (@MAX@ / (@T@)2)) &&
             (r >= (b + b)) )
        {
            r        -= b + b;
            quotient += 2;
        }

        // r below 2b and the count even: past b / 2 take b away, and again
        // past it still, as remainder_c does -- a half stays where the
        // quotient is even. Below twice the least normal value b / 2 may not
        // be exact, but 2r is
        if (b < (@MIN@ + @MIN@))
        {
            if ((r + r) > b)
            {
                r -= b;
                ++quotient;

                if ((r + r) >= b)
                {
                    r -= b;
                    ++quotient;
                }
            }
        }
        else
        {
            const @T@ half = b / (@T@)2;

            if (r > half)
            {
                r -= b;
                ++quotient;

                if (r >= half)
                {
                    r -= b;
                    ++quotient;
                }
            }
        }

        // x's sign, a zero's included; the quotient's last three bits, 8
        // among them as 0, with the sign of x / y
        result   = (_x < (@T@)0) ? -r : r;
        quotient = quotient % 8;

        if ((_x < (@T@)0) != (_y < (@T@)0))
        {
            quotient = -quotient;
        }
    }

    // a null _quo: the remainder alone
    if (_quo != NULL)
    {
        *_quo = quotient;
    }

    return result;
}

D_MATH_FN_RT @T@
d_math_@S@_remquo(
    @T@ _x,
    @T@ _y,
    @QP@ _quo
)
{
    // a null _quo: the remainder alone, as remquo_c gives it -- the
    // library's must have somewhere to store
    int  quotient = 0;
    int* where    = (_quo != NULL) ? _quo : &quotient;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    return std::remquo(_x, _y, where);
#elif defined(__cplusplus)
    // C++ before C++11 has no remquo: the pure-arithmetic form serves
    return d_math_@S@_remquo_c(_x, _y, where);
#else
    return @CREMQUO@(_x, _y, where);
#endif
}
"""

REMAINDER_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE FLOAT FAMILY (_f)
    ---------------------
    1.  The truncated remainder
    2.  The rounded remainder
    3.  The rounded remainder and its quotient
2.  THE DOUBLE FAMILY (_d)
    ----------------------
    1.  The truncated remainder
    2.  The rounded remainder
    3.  The rounded remainder and its quotient
3.  THE LONG DOUBLE FAMILY (_ld)
    ----------------------------
    1.  The truncated remainder
    2.  The rounded remainder
    3.  The rounded remainder and its quotient
4.  ONE NAME FOR EVERY FAMILY (C++)
    -------------------------------
*/

#ifndef DJINTERP_C_MATH_REMAINDER_H
#define DJINTERP_C_MATH_REMAINDER_H 1

// std
#ifdef __cplusplus
    #include <cmath>         // std::fmod, std::remainder, std::remquo
#endif  // __cplusplus
#include <float.h>           // FLT_MIN ... LDBL_MAX
#include <math.h>            // fmodf ... remquol
#include <stddef.h>          // NULL
// djinterp
#include "./elementary.h"    // d_internal_math_*_nan
#include "./math_common.h"   // D_MATH_FN
"""

REMAINDER_TAIL = r"""

#endif  // DJINTERP_C_MATH_REMAINDER_H
"""

REMAINDER_OVERLOADS = [
    ("D_MATH_FN", "@T@", "d_math_fmod_c", [("@T@", "_x"), ("@T@", "_y")]),
    ("D_MATH_FN", "@T@", "d_math_remainder_c", [("@T@", "_x"), ("@T@", "_y")]),
    ("D_MATH_FN", "@T@", "d_math_remquo_c", [("@T@", "_x"), ("@T@", "_y"), ("int*", "_quo")]),
    ("D_MATH_FN_RT", "@T@", "d_math_fmod", [("@T@", "_x"), ("@T@", "_y")]),
    ("D_MATH_FN_RT", "@T@", "d_math_remainder", [("@T@", "_x"), ("@T@", "_y")]),
    ("D_MATH_FN_RT", "@T@", "d_math_remquo", [("@T@", "_x"), ("@T@", "_y"), ("int*", "_quo")]),
]


def remainder():
    lines = [
        "Remainders: the C core's fmod, remainder and remquo, for float, double",
        "and long double.",
        "  GENERATED by scripts/gen_c_math_float.py from one text for the three",
        "families: edit the generator, not this file.",
        "",
        "  Each function comes twice, as in elementary.h. The _c form is pure",
        "arithmetic, so the same text is a constant expression from C++14; the",
        "plain form calls the C library (std:: in C++), fast at run time. A",
        "remainder is exact -- that of two values of a family is a value of it",
        "-- so the _c forms compute it in the family's own arithmetic, by",
        "differences each exact, and agree with the library's bit for bit: a",
        "face may use either and not know which ran. Of remquo's quotient C",
        "promises the last three bits, and those are what agree. C++ before",
        "C++11 has neither remainder nor remquo, so that there they are the _c",
        "forms.",
    ]
    text = banner("remainder.h", lines, "2026.10.10", "2026.11.07")
    text += "\n" + REMAINDER_HEAD
    for i, (suffix, ctype, prefix, prose) in enumerate(FAMILIES):
        upper = prose.upper()
        body = REMAINDER_TEMPLATE.replace("@SECTION@", str(i + 1)).replace("@UPPER@", upper)
        body = body.replace("@CFMOD@", {"float": "fmodf", "double": "fmod", "long double": "fmodl"}[ctype])
        body = body.replace("@CREMAINDER@", {"float": "remainderf", "double": "remainder", "long double": "remainderl"}[ctype])
        body = body.replace("@CREMQUO@", {"float": "remquof", "double": "remquo", "long double": "remquol"}[ctype])
        # remquo's int and int* in line with the family's type
        body = body.replace("@QP@", "int*".ljust(len(ctype))).replace("@QI@", "int".ljust(len(ctype)))
        text += instantiate(body, suffix, ctype, prefix, prose)
    text += overload_section("4.  ONE NAME FOR EVERY FAMILY (C++)", REMAINDER_OVERLOADS)
    text += REMAINDER_TAIL
    return text



# -----------------------------------------------------------------------------
# nearest_integer.h
# -----------------------------------------------------------------------------
# C's nearest integer functions (7.12.9) that do not read the rounding mode:
# floor, ceil, trunc and round. Exact -- each is a value of the family -- and
# right over the whole range, past 2^63 too (N5).

NEAREST_INTEGER_TEMPLATE = r"""
//==============================================================================
// @SECTION@.  THE @UPPER@ FAMILY (_@S@)
//==============================================================================


// @SECTION@.1    Internal helpers
//------------------------------------------------------------------------------

// d_internal_math_@S@_own_integer
//   true for a value that floor, ceil, trunc and round all leave as it is:
// NaN, a zero, and from 2^(P-1) up, where every value is an integer -- the
// infinities among them. 2^(P-1) is 1 / @EPS@, exactly.
D_MATH_FN bool
d_internal_math_@S@_own_integer(
    @T@ _x
)
{
    const @T@ top = (@T@)1 / @EPS@;

    return ( (_x != _x)     ||
             (_x == (@T@)0) ||
             (!(_x < top))  ||
             (!(_x > -top)) );
}

// d_internal_math_@S@_whole
//   the integer part of _a, 0 <= _a < 2^(P-1), exactly. A conversion to
// dj_math_umax truncates exactly any value below 2^W, W its bits: every _a,
// where P - 1 <= W. Past 2^W -- a quad's long double, or a family under a
// 32-bit dj_math_umax -- _a is taken W bits at a time from the top: each part
// a whole multiple of a power of 2^W, each difference exact.
D_MATH_FN @T@
d_internal_math_@S@_whole(
    @T@ _a
)
{
    // 2^W: 2^(W-1) is exact as an integer and as a value of the family, and
    // so is twice it
    const @T@ span =
        (@T@)((D_MATH_UMAX_MAX >> 1) + 1u) * (@T@)2;

    if (_a < span)
    {
        return (@T@)(dj_math_umax)_a;
    }

    {
        @T@ scale = span;
        @T@ whole = (@T@)0;
        @T@ rest  = _a;

        // the largest power of 2^W at or below _a
        while (!(rest < (scale * span)))
        {
            scale *= span;
        }

        // then its digits, from the top, each below 2^W
        for (;;)
        {
            const @T@ part =
                (@T@)(dj_math_umax)(rest / scale) * scale;

            whole += part;
            rest  -= part;

            if (scale == (@T@)1)
            {
                break;
            }

            scale /= span;
        }

        return whole;
    }
}

// @SECTION@.2    Nearest integers
//------------------------------------------------------------------------------
// floor_c, ceil_c, trunc_c and round_c are C's floor, ceil, trunc and round,
// exactly -- the integer each picks is a value of the family -- in pure
// arithmetic, a constant expression from C++14, over the whole range. round
// takes a half away from zero. A zero keeps its sign, and so does a result
// that is a zero: ceil(-0.5), trunc(-0.5) and round(-0.25) are -0. The
// infinities are their own; NaN gives NaN, the argument itself. The plain
// forms are the C library's, exact too: the two agree bit for bit. C++
// before C++11 has no trunc or round, so there those are the _c forms.

D_MATH_FN @T@
d_math_@S@_floor_c(
    @T@ _x
)
{
    if (d_internal_math_@S@_own_integer(_x))
    {
        return _x;
    }

    {
        const @T@ a = (_x < (@T@)0) ? -_x : _x;
        const @T@ w = d_internal_math_@S@_whole(a);

        if (_x > (@T@)0)
        {
            return w;
        }

        // below zero, a step further down unless the value is whole
        return (w == a) ? _x : -(w + (@T@)1);
    }
}

D_MATH_FN @T@
d_math_@S@_ceil_c(
    @T@ _x
)
{
    if (d_internal_math_@S@_own_integer(_x))
    {
        return _x;
    }

    {
        const @T@ a = (_x < (@T@)0) ? -_x : _x;
        const @T@ w = d_internal_math_@S@_whole(a);

        // below zero the integer part, -0 for a fraction
        if (_x < (@T@)0)
        {
            return -w;
        }

        return (w == a) ? _x : (w + (@T@)1);
    }
}

D_MATH_FN @T@
d_math_@S@_trunc_c(
    @T@ _x
)
{
    if (d_internal_math_@S@_own_integer(_x))
    {
        return _x;
    }

    {
        const @T@ a = (_x < (@T@)0) ? -_x : _x;
        const @T@ w = d_internal_math_@S@_whole(a);

        return (_x < (@T@)0) ? -w : w;
    }
}

D_MATH_FN @T@
d_math_@S@_round_c(
    @T@ _x
)
{
    if (d_internal_math_@S@_own_integer(_x))
    {
        return _x;
    }

    {
        const @T@ a = (_x < (@T@)0) ? -_x : _x;
        const @T@ w = d_internal_math_@S@_whole(a);
        // the fraction a - w is exact: a half or more goes away from zero
        const @T@ r =
            ((a - w) < (@T@)0.5) ? w : (w + (@T@)1);

        return (_x < (@T@)0) ? -r : r;
    }
}

D_MATH_FN_RT @T@
d_math_@S@_floor(
    @T@ _x
)
{
#ifdef __cplusplus
    return std::floor(_x);
#else
    return @CFLOOR@(_x);
#endif
}

D_MATH_FN_RT @T@
d_math_@S@_ceil(
    @T@ _x
)
{
#ifdef __cplusplus
    return std::ceil(_x);
#else
    return @CCEIL@(_x);
#endif
}

D_MATH_FN_RT @T@
d_math_@S@_trunc(
    @T@ _x
)
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    return std::trunc(_x);
#elif defined(__cplusplus)
    // C++ before C++11 has no trunc: the pure-arithmetic form serves
    return d_math_@S@_trunc_c(_x);
#else
    return @CTRUNC@(_x);
#endif
}

D_MATH_FN_RT @T@
d_math_@S@_round(
    @T@ _x
)
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    return std::round(_x);
#elif defined(__cplusplus)
    // C++ before C++11 has no round: the pure-arithmetic form serves
    return d_math_@S@_round_c(_x);
#else
    return @CROUND@(_x);
#endif
}
"""

NEAREST_INTEGER_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE FLOAT FAMILY (_f)
    ---------------------
    1.  Internal helpers
    2.  Nearest integers
2.  THE DOUBLE FAMILY (_d)
    ----------------------
    1.  Internal helpers
    2.  Nearest integers
3.  THE LONG DOUBLE FAMILY (_ld)
    ----------------------------
    1.  Internal helpers
    2.  Nearest integers
4.  ONE NAME FOR EVERY FAMILY (C++)
    -------------------------------
*/

#ifndef DJINTERP_C_MATH_NEAREST_INTEGER_H
#define DJINTERP_C_MATH_NEAREST_INTEGER_H 1

// std
#ifdef __cplusplus
    #include <cmath>         // std::floor, std::ceil, std::trunc, std::round
#endif  // __cplusplus
#include <float.h>           // FLT_EPSILON ... LDBL_EPSILON
#include <math.h>            // floorf ... roundl
// djinterp
#include "./math_common.h"   // D_MATH_FN, dj_math_umax, D_MATH_UMAX_MAX
"""

NEAREST_INTEGER_TAIL = r"""

#endif  // DJINTERP_C_MATH_NEAREST_INTEGER_H
"""

NEAREST_INTEGER_OVERLOADS = [
    ("D_MATH_FN", "@T@", "d_math_floor_c", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_ceil_c", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_trunc_c", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_round_c", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_floor", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_ceil", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_trunc", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_round", [("@T@", "_x")]),
]


def nearest_integer():
    lines = [
        "Nearest integers: the C core's floor, ceil, trunc and round, for",
        "float, double and long double.",
        "  GENERATED by scripts/gen_c_math_float.py from one text for the three",
        "families: edit the generator, not this file.",
        "",
        "  Each function comes twice, as in elementary.h. The _c form is pure",
        "arithmetic, so the same text is a constant expression from C++14; the",
        "plain form calls the C library (std:: in C++), fast at run time. Each",
        "is exact -- the integer it picks is a value of the family -- and right",
        "over the whole range: the integer part is taken by conversion to",
        "dj_math_umax, never long long, a wide value's a part at a time, and",
        "from 2^(P-1) up every value is its own. So the _c forms agree with the",
        "library bit for bit, a zero's sign included, and a face may use",
        "either and not know which ran. C++ before C++11 has no trunc or round,",
        "so that there those are the _c forms.",
    ]
    text = banner("nearest_integer.h", lines, "2026.10.11", "2026.11.07")
    text += "\n" + NEAREST_INTEGER_HEAD
    for i, (suffix, ctype, prefix, prose) in enumerate(FAMILIES):
        upper = prose.upper()
        body = NEAREST_INTEGER_TEMPLATE.replace("@SECTION@", str(i + 1)).replace("@UPPER@", upper)
        for key, names in (("@CFLOOR@", ("floorf", "floor", "floorl")),
                           ("@CCEIL@", ("ceilf", "ceil", "ceill")),
                           ("@CTRUNC@", ("truncf", "trunc", "truncl")),
                           ("@CROUND@", ("roundf", "round", "roundl"))):
            body = body.replace(key, names[("float", "double", "long double").index(ctype)])
        text += instantiate(body, suffix, ctype, prefix, prose)
    text += overload_section("4.  ONE NAME FOR EVERY FAMILY (C++)", NEAREST_INTEGER_OVERLOADS)
    text += NEAREST_INTEGER_TAIL
    return text



# -----------------------------------------------------------------------------
# vec.h
# -----------------------------------------------------------------------------

VEC_TEMPLATE = r"""
//==============================================================================
// @SECTION@.  THE @UPPER@ FAMILY (_@S@)
//==============================================================================
// _count components each; _out may be _a or _b.


// @SECTION@.1    Element-wise arithmetic
//------------------------------------------------------------------------------

D_MATH_FN void
d_vec_@S@_fill(
    @T@*   _out,
    size_t _count,
    @T@    _value
)
{
    size_t i = 0u;

    for (i = 0u; i < _count; ++i)
    {
        _out[i] = _value;
    }

    return;
}

D_MATH_FN void
d_vec_@S@_add(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b,
    size_t     _count
)
{
    size_t i = 0u;

    for (i = 0u; i < _count; ++i)
    {
        _out[i] = _a[i] + _b[i];
    }

    return;
}

D_MATH_FN void
d_vec_@S@_sub(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b,
    size_t     _count
)
{
    size_t i = 0u;

    for (i = 0u; i < _count; ++i)
    {
        _out[i] = _a[i] - _b[i];
    }

    return;
}

D_MATH_FN void
d_vec_@S@_scale(
    @T@*       _out,
    const @T@* _a,
    @T@        _s,
    size_t     _count
)
{
    size_t i = 0u;

    for (i = 0u; i < _count; ++i)
    {
        _out[i] = _a[i] * _s;
    }

    return;
}

D_MATH_FN void
d_vec_@S@_divide(
    @T@*       _out,
    const @T@* _a,
    @T@        _s,
    size_t     _count
)
{
    size_t i = 0u;

    for (i = 0u; i < _count; ++i)
    {
        _out[i] = _a[i] / _s;
    }

    return;
}

D_MATH_FN void
d_vec_@S@_negate(
    @T@*       _out,
    const @T@* _a,
    size_t     _count
)
{
    size_t i = 0u;

    for (i = 0u; i < _count; ++i)
    {
        _out[i] = -_a[i];
    }

    return;
}

D_MATH_FN void
d_vec_@S@_hadamard(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b,
    size_t     _count
)
{
    size_t i = 0u;

    for (i = 0u; i < _count; ++i)
    {
        _out[i] = _a[i] * _b[i];
    }

    return;
}

D_MATH_FN void
d_vec_@S@_lerp(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b,
    @T@        _t,
    size_t     _count
)
{
    const @T@ keep = (@T@)1 - _t;
    size_t    i    = 0u;

    // each product rounded apart, then summed: a(1 - t) + bt
    for (i = 0u; i < _count; ++i)
    {
        const @T@ from = _a[i] * keep;
        const @T@ to   = _b[i] * _t;

        _out[i] = from + to;
    }

    return;
}

// @SECTION@.2    Products and reductions
//------------------------------------------------------------------------------
// Each product is rounded before it is summed, so no compiler can fuse the
// two and a result is the same on every target.

D_MATH_FN @T@
d_vec_@S@_dot(
    const @T@* _a,
    const @T@* _b,
    size_t     _count
)
{
    @T@    acc = (@T@)0;
    size_t i   = 0u;

    for (i = 0u; i < _count; ++i)
    {
        const @T@ product = _a[i] * _b[i];

        acc += product;
    }

    return acc;
}

D_MATH_FN void
d_vec_@S@_cross(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b
)
{
    // all three from the inputs before any output is written
    const @T@ x1 = _a[1] * _b[2];
    const @T@ x2 = _a[2] * _b[1];
    const @T@ y1 = _a[2] * _b[0];
    const @T@ y2 = _a[0] * _b[2];
    const @T@ z1 = _a[0] * _b[1];
    const @T@ z2 = _a[1] * _b[0];

    _out[0] = x1 - x2;
    _out[1] = y1 - y2;
    _out[2] = z1 - z2;

    return;
}

D_MATH_FN @T@
d_vec_@S@_sum(
    const @T@* _a,
    size_t     _count
)
{
    @T@    acc = (@T@)0;
    size_t i   = 0u;

    for (i = 0u; i < _count; ++i)
    {
        acc += _a[i];
    }

    return acc;
}

D_MATH_FN @T@
d_vec_@S@_product(
    const @T@* _a,
    size_t     _count
)
{
    @T@    acc = (@T@)1;
    size_t i   = 0u;

    for (i = 0u; i < _count; ++i)
    {
        acc *= _a[i];
    }

    return acc;
}

D_MATH_FN @T@
d_vec_@S@_min(
    const @T@* _a,
    size_t     _count
)
{
    @T@    m = _a[0];
    size_t i = 1u;

    for (i = 1u; i < _count; ++i)
    {
        if (_a[i] < m)
        {
            m = _a[i];
        }
    }

    return m;
}

D_MATH_FN @T@
d_vec_@S@_max(
    const @T@* _a,
    size_t     _count
)
{
    @T@    m = _a[0];
    size_t i = 1u;

    for (i = 1u; i < _count; ++i)
    {
        if (_a[i] > m)
        {
            m = _a[i];
        }
    }

    return m;
}

// @SECTION@.3    Norms and distances
//------------------------------------------------------------------------------
// Each root comes in two forms, the same value bit for bit (elementary.h's
// two square roots are both correctly rounded): the _c form a constant
// expression from C++14, the plain one the C library's root. A norm is the
// root of the sum of squares wherever that root is in range -- at most the
// family's largest value, and at least the root of its least normal over its
// epsilon, where no square overflowed or lost its precision to underflow --
// and elsewhere the norm of the entries scaled exactly by a power of two to
// a largest magnitude in [1, 2), scaled back, as LAPACK's dnrm2 and Julia's
// norm scale (S256): a norm the family holds is never an infinity, nor 0 for
// a vector that is not zero, and a norm in range keeps its bits. distance,
// normalize and cos_angle scale as norm does.

// d_internal_vec_@S@_largest
//   the largest magnitude of the _count entries _a[i] - _b[i], or _a[i]
// where _b is NULL: 0 for the zero vector, and -1 where an entry is a NaN or
// an infinity, for which nothing is scaled.
D_MATH_FN @T@
d_internal_vec_@S@_largest(
    const @T@* _a,
    const @T@* _b,
    size_t     _count
)
{
    @T@    largest = (@T@)0;
    size_t i       = 0u;

    for (i = 0u; i < _count; ++i)
    {
        const @T@ e = (_b != NULL) ? (_a[i] - _b[i]) : _a[i];
        const @T@ v = (e < (@T@)0) ? -e : e;

        // a NaN or an infinity
        if (!(v <= @MAX@))
        {
            return (@T@)-1;
        }

        if (v > largest)
        {
            largest = v;
        }
    }

    return largest;
}

// d_internal_vec_@S@_power_of
//   the power of two that brings a positive finite _largest into [1, 2) --
// its binade -- as *_power, times *_pre, 2^P where _largest is subnormal and
// is lifted first, else 1: an entry scaled by it, times *_pre and then over
// *_power, is exact (but where it underflows, below the largest's rounding).
D_MATH_FN void
d_internal_vec_@S@_power_of(
    @T@  _largest,
    @T@* _pre,
    @T@* _power
)
{
    @T@ lifted = _largest;

    *_pre = (@T@)1;

    // a subnormal largest: lifted into the normal range first
    if (_largest < @MIN@)
    {
        *_pre  = d_internal_math_@S@_pow2(@P@);
        lifted = _largest * *_pre;
    }

    {
        const struct dj_internal_math_@S@_binade b =
            d_internal_math_@S@_binade_of(lifted);

        *_power = b.power;
    }

    return;
}

// d_internal_vec_@S@_scaled_entry
//   entry _i of the vector -- _a[_i] - _b[_i], or _a[_i] where _b is NULL --
// times _pre, then over _power.
D_MATH_FN @T@
d_internal_vec_@S@_scaled_entry(
    const @T@* _a,
    const @T@* _b,
    size_t     _i,
    @T@        _pre,
    @T@        _power
)
{
    const @T@ e      = (_b != NULL) ? (_a[_i] - _b[_i]) : _a[_i];
    const @T@ lifted = e * _pre;

    return lifted / _power;
}

// d_internal_vec_@S@_scaled_sum
//   the sum of the squares of the entries scaled by _pre and _power, as
// d_vec_@S@_dot sums: each square rounded, then added.
D_MATH_FN @T@
d_internal_vec_@S@_scaled_sum(
    const @T@* _a,
    const @T@* _b,
    size_t     _count,
    @T@        _pre,
    @T@        _power
)
{
    @T@    acc = (@T@)0;
    size_t i   = 0u;

    for (i = 0u; i < _count; ++i)
    {
        const @T@ v      = d_internal_vec_@S@_scaled_entry(_a, _b, i, _pre,
                                                           _power);
        const @T@ square = v * v;

        acc += square;
    }

    return acc;
}

D_MATH_FN @T@
d_vec_@S@_norm_squared(
    const @T@* _a,
    size_t     _count
)
{
    return d_vec_@S@_dot(_a, _a, _count);
}

// d_internal_vec_@S@_in_range_c
//   whether _norm, the root of a sum of squares, is in the range where no
// square overflowed or lost its precision to underflow: at most the family's
// largest value, and at least the root of its least normal over its epsilon.
D_MATH_FN bool
d_internal_vec_@S@_in_range_c(
    @T@ _norm
)
{
    const @T@ small = d_math_@S@_sqrt_c(@MIN@ / @EPS@);

    return ( (_norm <= @MAX@) &&
             (!(_norm < small)) );
}

// d_internal_vec_@S@_rescued_c
//   _norm, the root of the sum of squares of the entries _a[i] - _b[i] (or
// _a[i] where _b is NULL), where it is in range; elsewhere the norm of the
// entries scaled by a power of two to a largest magnitude in [1, 2), scaled
// back; _norm itself for the zero vector or where an entry is a NaN or an
// infinity.
D_MATH_FN @T@
d_internal_vec_@S@_rescued_c(
    const @T@* _a,
    const @T@* _b,
    size_t     _count,
    @T@        _norm
)
{
    const @T@ largest = d_internal_vec_@S@_largest(_a, _b, _count);
    @T@       pre     = (@T@)1;
    @T@       power   = (@T@)1;

    // in range, or nothing to scale
    if ( (d_internal_vec_@S@_in_range_c(_norm)) ||
         (!(largest > (@T@)0)) )
    {
        return _norm;
    }

    d_internal_vec_@S@_power_of(largest, &pre, &power);

    {
        const @T@ sum      = d_internal_vec_@S@_scaled_sum(_a, _b, _count,
                                                           pre, power);
        const @T@ root     = d_math_@S@_sqrt_c(sum);
        const @T@ restored = root * power;

        return restored / pre;
    }
}

D_MATH_FN @T@
d_vec_@S@_norm_c(
    const @T@* _a,
    size_t     _count
)
{
    const @T@ norm = d_math_@S@_sqrt_c(d_vec_@S@_norm_squared(_a, _count));

    return d_internal_vec_@S@_rescued_c(_a, NULL, _count, norm);
}

// d_internal_vec_@S@_in_range
//   whether _norm, the root of a sum of squares, is in the range where no
// square overflowed or lost its precision to underflow: at most the family's
// largest value, and at least the root of its least normal over its epsilon.
D_MATH_FN_RT bool
d_internal_vec_@S@_in_range(
    @T@ _norm
)
{
    const @T@ small = d_math_@S@_sqrt(@MIN@ / @EPS@);

    return ( (_norm <= @MAX@) &&
             (!(_norm < small)) );
}

// d_internal_vec_@S@_rescued
//   _norm, the root of the sum of squares of the entries _a[i] - _b[i] (or
// _a[i] where _b is NULL), where it is in range; elsewhere the norm of the
// entries scaled by a power of two to a largest magnitude in [1, 2), scaled
// back; _norm itself for the zero vector or where an entry is a NaN or an
// infinity.
D_MATH_FN_RT @T@
d_internal_vec_@S@_rescued(
    const @T@* _a,
    const @T@* _b,
    size_t     _count,
    @T@        _norm
)
{
    const @T@ largest = d_internal_vec_@S@_largest(_a, _b, _count);
    @T@       pre     = (@T@)1;
    @T@       power   = (@T@)1;

    // in range, or nothing to scale
    if ( (d_internal_vec_@S@_in_range(_norm)) ||
         (!(largest > (@T@)0)) )
    {
        return _norm;
    }

    d_internal_vec_@S@_power_of(largest, &pre, &power);

    {
        const @T@ sum      = d_internal_vec_@S@_scaled_sum(_a, _b, _count,
                                                           pre, power);
        const @T@ root     = d_math_@S@_sqrt(sum);
        const @T@ restored = root * power;

        return restored / pre;
    }
}

D_MATH_FN_RT @T@
d_vec_@S@_norm(
    const @T@* _a,
    size_t     _count
)
{
    const @T@ norm = d_math_@S@_sqrt(d_vec_@S@_norm_squared(_a, _count));

    return d_internal_vec_@S@_rescued(_a, NULL, _count, norm);
}

D_MATH_FN @T@
d_vec_@S@_distance_squared(
    const @T@* _a,
    const @T@* _b,
    size_t     _count
)
{
    @T@    acc = (@T@)0;
    size_t i   = 0u;

    // the difference's own square, as |a - b|^2 is
    for (i = 0u; i < _count; ++i)
    {
        const @T@ d      = _a[i] - _b[i];
        const @T@ square = d * d;

        acc += square;
    }

    return acc;
}

D_MATH_FN @T@
d_vec_@S@_distance_c(
    const @T@* _a,
    const @T@* _b,
    size_t     _count
)
{
    const @T@ norm =
        d_math_@S@_sqrt_c(d_vec_@S@_distance_squared(_a, _b, _count));

    return d_internal_vec_@S@_rescued_c(_a, _b, _count, norm);
}

D_MATH_FN_RT @T@
d_vec_@S@_distance(
    const @T@* _a,
    const @T@* _b,
    size_t     _count
)
{
    const @T@ norm =
        d_math_@S@_sqrt(d_vec_@S@_distance_squared(_a, _b, _count));

    return d_internal_vec_@S@_rescued(_a, _b, _count, norm);
}

D_MATH_FN void
d_vec_@S@_normalize_c(
    @T@*       _out,
    const @T@* _a,
    size_t     _count
)
{
    const @T@ n = d_math_@S@_sqrt_c(d_vec_@S@_norm_squared(_a, _count));
    @T@       largest = (@T@)0;

    // in range: over the norm, as ever
    if (d_internal_vec_@S@_in_range_c(n))
    {
        d_vec_@S@_divide(_out, _a, n, _count);

        return;
    }

    largest = d_internal_vec_@S@_largest(_a, NULL, _count);

    // the zero vector stays zero
    if (largest == (@T@)0)
    {
        d_vec_@S@_fill(_out, _count, (@T@)0);
    }
    // a NaN or an infinity: over the norm, as ever
    else if (!(largest > (@T@)0))
    {
        d_vec_@S@_divide(_out, _a, n, _count);
    }
    // the vector scaled by a power of two first, which leaves its direction
    else
    {
        @T@    pre   = (@T@)1;
        @T@    power = (@T@)1;
        @T@    root  = (@T@)0;
        size_t i     = 0u;

        d_internal_vec_@S@_power_of(largest, &pre, &power);
        root = d_math_@S@_sqrt_c(
                   d_internal_vec_@S@_scaled_sum(_a, NULL, _count, pre,
                                                 power));

        for (i = 0u; i < _count; ++i)
        {
            _out[i] = d_internal_vec_@S@_scaled_entry(_a, NULL, i, pre,
                                                      power) / root;
        }
    }

    return;
}

D_MATH_FN_RT void
d_vec_@S@_normalize(
    @T@*       _out,
    const @T@* _a,
    size_t     _count
)
{
    const @T@ n = d_math_@S@_sqrt(d_vec_@S@_norm_squared(_a, _count));
    @T@       largest = (@T@)0;

    // in range: over the norm, as ever
    if (d_internal_vec_@S@_in_range(n))
    {
        d_vec_@S@_divide(_out, _a, n, _count);

        return;
    }

    largest = d_internal_vec_@S@_largest(_a, NULL, _count);

    // the zero vector stays zero
    if (largest == (@T@)0)
    {
        d_vec_@S@_fill(_out, _count, (@T@)0);
    }
    // a NaN or an infinity: over the norm, as ever
    else if (!(largest > (@T@)0))
    {
        d_vec_@S@_divide(_out, _a, n, _count);
    }
    // the vector scaled by a power of two first, which leaves its direction
    else
    {
        @T@    pre   = (@T@)1;
        @T@    power = (@T@)1;
        @T@    root  = (@T@)0;
        size_t i     = 0u;

        d_internal_vec_@S@_power_of(largest, &pre, &power);
        root = d_math_@S@_sqrt(
                   d_internal_vec_@S@_scaled_sum(_a, NULL, _count, pre,
                                                 power));

        for (i = 0u; i < _count; ++i)
        {
            _out[i] = d_internal_vec_@S@_scaled_entry(_a, NULL, i, pre,
                                                      power) / root;
        }
    }

    return;
}

// @SECTION@.4    Projection and angle
//------------------------------------------------------------------------------
// project is _a's component along _onto, reject what is left; the zero
// vector projects to zero. cos_angle is 0 when either vector is zero, and
// scaled as the norms are where their product would leave the range; angle
// clamps it into [-1, 1] first, so rounding past 1 gives 0, not NaN.

D_MATH_FN void
d_vec_@S@_project(
    @T@*       _out,
    const @T@* _a,
    const @T@* _onto,
    size_t     _count
)
{
    const @T@ d = d_vec_@S@_norm_squared(_onto, _count);

    // onto the zero vector there is nothing to project
    if (d > (@T@)0)
    {
        d_vec_@S@_scale(_out,
                        _onto,
                        d_vec_@S@_dot(_a, _onto, _count) / d,
                        _count);
    }
    else
    {
        d_vec_@S@_fill(_out, _count, (@T@)0);
    }

    return;
}

D_MATH_FN void
d_vec_@S@_reject(
    @T@*       _out,
    const @T@* _a,
    const @T@* _from,
    size_t     _count
)
{
    size_t i = 0u;

    // _a less its projection, formed component by component so _out may be
    // either input
    {
        const @T@ d     = d_vec_@S@_norm_squared(_from, _count);
        const @T@ ratio = (d > (@T@)0)
                              ? (d_vec_@S@_dot(_a, _from, _count) / d)
                              : (@T@)0;

        for (i = 0u; i < _count; ++i)
        {
            const @T@ along = _from[i] * ratio;

            _out[i] = _a[i] - along;
        }
    }

    return;
}

D_MATH_FN @T@
d_vec_@S@_cos_angle_c(
    const @T@* _a,
    const @T@* _b,
    size_t     _count
)
{
    const @T@ na = d_math_@S@_sqrt_c(d_vec_@S@_norm_squared(_a, _count));
    const @T@ nb = d_math_@S@_sqrt_c(d_vec_@S@_norm_squared(_b, _count));
    const @T@ d  = na * nb;
    @T@       la = (@T@)0;
    @T@       lb = (@T@)0;

    // in range: the dot product over the norms' product, as ever
    if ( (d_internal_vec_@S@_in_range_c(na)) &&
         (d_internal_vec_@S@_in_range_c(nb)) &&
         (d <= @MAX@) )
    {
        return d_vec_@S@_dot(_a, _b, _count) / d;
    }

    la = d_internal_vec_@S@_largest(_a, NULL, _count);
    lb = d_internal_vec_@S@_largest(_b, NULL, _count);

    // a NaN or an infinity: as ever
    if ( (la < (@T@)0) ||
         (lb < (@T@)0) )
    {
        return (d > (@T@)0) ? (d_vec_@S@_dot(_a, _b, _count) / d) : (@T@)0;
    }

    // a zero vector: 0
    if ( (la == (@T@)0) ||
         (lb == (@T@)0) )
    {
        return (@T@)0;
    }

    // each vector scaled by a power of two, which leaves the cosine
    {
        @T@    pa  = (@T@)1;
        @T@    wa  = (@T@)1;
        @T@    pb  = (@T@)1;
        @T@    wb  = (@T@)1;
        @T@    dot = (@T@)0;
        size_t i   = 0u;

        d_internal_vec_@S@_power_of(la, &pa, &wa);
        d_internal_vec_@S@_power_of(lb, &pb, &wb);

        for (i = 0u; i < _count; ++i)
        {
            const @T@ x       = d_internal_vec_@S@_scaled_entry(_a, NULL, i,
                                                                pa, wa);
            const @T@ y       = d_internal_vec_@S@_scaled_entry(_b, NULL, i,
                                                                pb, wb);
            const @T@ product = x * y;

            dot += product;
        }

        {
            const @T@ sa = d_math_@S@_sqrt_c(
                               d_internal_vec_@S@_scaled_sum(_a, NULL, _count,
                                                             pa, wa));
            const @T@ sb = d_math_@S@_sqrt_c(
                               d_internal_vec_@S@_scaled_sum(_b, NULL, _count,
                                                             pb, wb));
            const @T@ ds = sa * sb;

            return dot / ds;
        }
    }
}

D_MATH_FN_RT @T@
d_vec_@S@_cos_angle(
    const @T@* _a,
    const @T@* _b,
    size_t     _count
)
{
    const @T@ na = d_math_@S@_sqrt(d_vec_@S@_norm_squared(_a, _count));
    const @T@ nb = d_math_@S@_sqrt(d_vec_@S@_norm_squared(_b, _count));
    const @T@ d  = na * nb;
    @T@       la = (@T@)0;
    @T@       lb = (@T@)0;

    // in range: the dot product over the norms' product, as ever
    if ( (d_internal_vec_@S@_in_range(na)) &&
         (d_internal_vec_@S@_in_range(nb)) &&
         (d <= @MAX@) )
    {
        return d_vec_@S@_dot(_a, _b, _count) / d;
    }

    la = d_internal_vec_@S@_largest(_a, NULL, _count);
    lb = d_internal_vec_@S@_largest(_b, NULL, _count);

    // a NaN or an infinity: as ever
    if ( (la < (@T@)0) ||
         (lb < (@T@)0) )
    {
        return (d > (@T@)0) ? (d_vec_@S@_dot(_a, _b, _count) / d) : (@T@)0;
    }

    // a zero vector: 0
    if ( (la == (@T@)0) ||
         (lb == (@T@)0) )
    {
        return (@T@)0;
    }

    // each vector scaled by a power of two, which leaves the cosine
    {
        @T@    pa  = (@T@)1;
        @T@    wa  = (@T@)1;
        @T@    pb  = (@T@)1;
        @T@    wb  = (@T@)1;
        @T@    dot = (@T@)0;
        size_t i   = 0u;

        d_internal_vec_@S@_power_of(la, &pa, &wa);
        d_internal_vec_@S@_power_of(lb, &pb, &wb);

        for (i = 0u; i < _count; ++i)
        {
            const @T@ x       = d_internal_vec_@S@_scaled_entry(_a, NULL, i,
                                                                pa, wa);
            const @T@ y       = d_internal_vec_@S@_scaled_entry(_b, NULL, i,
                                                                pb, wb);
            const @T@ product = x * y;

            dot += product;
        }

        {
            const @T@ sa = d_math_@S@_sqrt(
                               d_internal_vec_@S@_scaled_sum(_a, NULL, _count,
                                                             pa, wa));
            const @T@ sb = d_math_@S@_sqrt(
                               d_internal_vec_@S@_scaled_sum(_b, NULL, _count,
                                                             pb, wb));
            const @T@ ds = sa * sb;

            return dot / ds;
        }
    }
}

D_MATH_FN @T@
d_vec_@S@_angle_c(
    const @T@* _a,
    const @T@* _b,
    size_t     _count
)
{
    const @T@ c = d_vec_@S@_cos_angle_c(_a, _b, _count);

    return d_math_@S@_acos_c( (c > (@T@)1)  ? (@T@)1  :
                              (c < (@T@)-1) ? (@T@)-1 : c );
}

D_MATH_FN_RT @T@
d_vec_@S@_angle(
    const @T@* _a,
    const @T@* _b,
    size_t     _count
)
{
    const @T@ c = d_vec_@S@_cos_angle(_a, _b, _count);

    return d_math_@S@_acos( (c > (@T@)1)  ? (@T@)1  :
                            (c < (@T@)-1) ? (@T@)-1 : c );
}

// @SECTION@.5    Comparison
//------------------------------------------------------------------------------
// equal is component-wise ==; equals allows each component _tolerance.

D_MATH_FN bool
d_vec_@S@_equal(
    const @T@* _a,
    const @T@* _b,
    size_t     _count
)
{
    size_t i = 0u;

    for (i = 0u; i < _count; ++i)
    {
        if (!(_a[i] == _b[i]))
        {
            return false;
        }
    }

    return true;
}

D_MATH_FN bool
d_vec_@S@_equals(
    const @T@* _a,
    const @T@* _b,
    @T@        _tolerance,
    size_t     _count
)
{
    size_t i = 0u;

    for (i = 0u; i < _count; ++i)
    {
        const @T@ d = _a[i] - _b[i];

        if (((d < (@T@)0) ? -d : d) > _tolerance)
        {
            return false;
        }
    }

    return true;
}
"""

VEC_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE FLOAT FAMILY (_f)
    ---------------------
    1.  Element-wise arithmetic
    2.  Products and reductions
    3.  Norms and distances
    4.  Projection and angle
    5.  Comparison
2.  THE DOUBLE FAMILY (_d)
    ----------------------
    1.  Element-wise arithmetic
    2.  Products and reductions
    3.  Norms and distances
    4.  Projection and angle
    5.  Comparison
3.  THE LONG DOUBLE FAMILY (_ld)
    ----------------------------
    1.  Element-wise arithmetic
    2.  Products and reductions
    3.  Norms and distances
    4.  Projection and angle
    5.  Comparison
4.  ONE NAME FOR EVERY FAMILY (C++)
    -------------------------------
*/

#ifndef DJINTERP_C_MATH_VEC_H
#define DJINTERP_C_MATH_VEC_H 1

// std
#include <stddef.h>          // size_t
// djinterp
#include "./elementary.h"    // d_math_*_sqrt, _sqrt_c, _acos, _acos_c
#include "./float_order.h"   // d_internal_math_*_binade_of,
                             // d_internal_math_*_pow2: the scaling
#include "./math_common.h"   // D_MATH_FN
"""

VEC_TAIL = r"""

#endif  // DJINTERP_C_MATH_VEC_H
"""

_A = ("const @T@*", "_a")
_B = ("const @T@*", "_b")
_O = ("@T@*", "_out")
_N = ("size_t", "_count")
VEC_OVERLOADS = [
    ("D_MATH_FN", "void", "d_vec_fill", [_O, _N, ("@T@", "_value")]),
    ("D_MATH_FN", "void", "d_vec_add", [_O, _A, _B, _N]),
    ("D_MATH_FN", "void", "d_vec_sub", [_O, _A, _B, _N]),
    ("D_MATH_FN", "void", "d_vec_scale", [_O, _A, ("@T@", "_s"), _N]),
    ("D_MATH_FN", "void", "d_vec_divide", [_O, _A, ("@T@", "_s"), _N]),
    ("D_MATH_FN", "void", "d_vec_negate", [_O, _A, _N]),
    ("D_MATH_FN", "void", "d_vec_hadamard", [_O, _A, _B, _N]),
    ("D_MATH_FN", "void", "d_vec_lerp", [_O, _A, _B, ("@T@", "_t"), _N]),
    ("D_MATH_FN", "@T@", "d_vec_dot", [_A, _B, _N]),
    ("D_MATH_FN", "void", "d_vec_cross", [_O, _A, _B]),
    ("D_MATH_FN", "@T@", "d_vec_sum", [_A, _N]),
    ("D_MATH_FN", "@T@", "d_vec_product", [_A, _N]),
    ("D_MATH_FN", "@T@", "d_vec_min", [_A, _N]),
    ("D_MATH_FN", "@T@", "d_vec_max", [_A, _N]),
    ("D_MATH_FN", "@T@", "d_vec_norm_squared", [_A, _N]),
    ("D_MATH_FN", "@T@", "d_vec_norm_c", [_A, _N]),
    ("D_MATH_FN_RT", "@T@", "d_vec_norm", [_A, _N]),
    ("D_MATH_FN", "@T@", "d_vec_distance_squared", [_A, _B, _N]),
    ("D_MATH_FN", "@T@", "d_vec_distance_c", [_A, _B, _N]),
    ("D_MATH_FN_RT", "@T@", "d_vec_distance", [_A, _B, _N]),
    ("D_MATH_FN", "void", "d_vec_normalize_c", [_O, _A, _N]),
    ("D_MATH_FN_RT", "void", "d_vec_normalize", [_O, _A, _N]),
    ("D_MATH_FN", "void", "d_vec_project", [_O, _A, ("const @T@*", "_onto"), _N]),
    ("D_MATH_FN", "void", "d_vec_reject", [_O, _A, ("const @T@*", "_from"), _N]),
    ("D_MATH_FN", "@T@", "d_vec_cos_angle_c", [_A, _B, _N]),
    ("D_MATH_FN_RT", "@T@", "d_vec_cos_angle", [_A, _B, _N]),
    ("D_MATH_FN", "@T@", "d_vec_angle_c", [_A, _B, _N]),
    ("D_MATH_FN_RT", "@T@", "d_vec_angle", [_A, _B, _N]),
    ("D_MATH_FN", "bool", "d_vec_equal", [_A, _B, _N]),
    ("D_MATH_FN", "bool", "d_vec_equals", [_A, _B, ("@T@", "_tolerance"), _N]),
]


def vec():
    lines = [
        "Fixed-size vectors: the C core of math/linear_algebra/vector.hpp, for",
        "float, double and long double.",
        "  GENERATED by scripts/gen_c_math_float.py from one text for the three",
        "families: edit the generator, not this file.",
        "",
        "  A vector here is an array of _count components the caller owns, as",
        "BLAS takes them; the C++ face's storage is that array. Each kernel is a",
        "loop the face's constant dimension unrolls, a constant expression from",
        "C++14 but for the plain root forms, which call the C library. A norm",
        "is scaled by a power of two where its sum of squares would leave the",
        "range, and keeps its bits where it does not (S256).",
    ]
    text = banner("vec.h", lines, DATE, "2026.11.07")
    text += "\n" + VEC_HEAD
    for i, (suffix, ctype, prefix, prose) in enumerate(FAMILIES):
        upper = prose.upper()
        body = VEC_TEMPLATE.replace("@SECTION@", str(i + 1)).replace("@UPPER@", upper)
        text += instantiate(body, suffix, ctype, prefix, prose)
    text += overload_section("4.  ONE NAME FOR EVERY FAMILY (C++)", VEC_OVERLOADS)
    text += VEC_TAIL
    return text



# -----------------------------------------------------------------------------
# mat.h
# -----------------------------------------------------------------------------

MAT_TEMPLATE = r"""
//==============================================================================
// @SECTION@.  THE @UPPER@ FAMILY (_@S@)
//==============================================================================
// Row-major: entry (i, j) of a matrix with _cols columns is [i * _cols + j].
// A product's output must not be either input; every other output may be.
// Entry-wise operations are vec.h's over the rows * cols entries.


// @SECTION@.1    Construction
//------------------------------------------------------------------------------

D_MATH_FN void
d_mat_@S@_identity(
    @T@*   _out,
    size_t _n
)
{
    size_t i = 0u;

    d_vec_@S@_fill(_out, _n * _n, (@T@)0);

    for (i = 0u; i < _n; ++i)
    {
        _out[(i * _n) + i] = (@T@)1;
    }

    return;
}

D_MATH_FN void
d_mat_@S@_diagonal(
    @T@*       _out,
    const @T@* _diagonal,
    size_t     _n
)
{
    size_t i = 0u;

    d_vec_@S@_fill(_out, _n * _n, (@T@)0);

    for (i = 0u; i < _n; ++i)
    {
        _out[(i * _n) + i] = _diagonal[i];
    }

    return;
}

// @SECTION@.2    Rows, columns and the transpose
//------------------------------------------------------------------------------

D_MATH_FN void
d_mat_@S@_row(
    @T@*       _out,
    const @T@* _a,
    size_t     _i,
    size_t     _cols
)
{
    size_t j = 0u;

    for (j = 0u; j < _cols; ++j)
    {
        _out[j] = _a[(_i * _cols) + j];
    }

    return;
}

D_MATH_FN void
d_mat_@S@_col(
    @T@*       _out,
    const @T@* _a,
    size_t     _j,
    size_t     _rows,
    size_t     _cols
)
{
    size_t i = 0u;

    for (i = 0u; i < _rows; ++i)
    {
        _out[i] = _a[(i * _cols) + _j];
    }

    return;
}

// d_mat_@S@_transpose
//   _out (_cols x _rows) is _a (_rows x _cols) transposed; _out must not be
// _a.
D_MATH_FN void
d_mat_@S@_transpose(
    @T@*       _out,
    const @T@* _a,
    size_t     _rows,
    size_t     _cols
)
{
    size_t i = 0u;

    for (i = 0u; i < _rows; ++i)
    {
        size_t j = 0u;

        for (j = 0u; j < _cols; ++j)
        {
            _out[(j * _rows) + i] = _a[(i * _cols) + j];
        }
    }

    return;
}

// @SECTION@.3    Products
//------------------------------------------------------------------------------
// Each entry is its dot product in order of k, each product rounded before it
// is summed. _out must not be an input.

// d_mat_@S@_multiply
//   _out (_rows x _cols) = _a (_rows x _inner) * _b (_inner x _cols).
D_MATH_FN void
d_mat_@S@_multiply(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b,
    size_t     _rows,
    size_t     _inner,
    size_t     _cols
)
{
    size_t i = 0u;

    for (i = 0u; i < _rows; ++i)
    {
        size_t j = 0u;

        for (j = 0u; j < _cols; ++j)
        {
            @T@    acc = (@T@)0;
            size_t k   = 0u;

            for (k = 0u; k < _inner; ++k)
            {
                const @T@ product = _a[(i * _inner) + k] * _b[(k * _cols) + j];

                acc += product;
            }

            _out[(i * _cols) + j] = acc;
        }
    }

    return;
}

// d_mat_@S@_multiply_vector
//   _out (_rows) = _a (_rows x _cols) * _v (_cols).
D_MATH_FN void
d_mat_@S@_multiply_vector(
    @T@*       _out,
    const @T@* _a,
    const @T@* _v,
    size_t     _rows,
    size_t     _cols
)
{
    size_t i = 0u;

    for (i = 0u; i < _rows; ++i)
    {
        _out[i] = d_vec_@S@_dot(_a + (i * _cols), _v, _cols);
    }

    return;
}

// d_mat_@S@_power
//   _out = _a^_exponent for an _n x _n matrix, the identity for 0: by
// squaring, from the exponent's highest bit down, the power begun as _a and,
// at each bit below it, squared, then multiplied by _a on the left where the
// bit is 1, as A^(2k) = A^k A^k and A^(2k+1) = A A^(2k) compose: at most
// 2 log2(_exponent) products, where A^k = A * A^(k-1) takes _exponent - 1.
// _scratch holds _n * _n entries; neither it nor _out may be _a.
D_MATH_FN void
d_mat_@S@_power(
    @T@*       _out,
    @T@*       _scratch,
    const @T@* _a,
    size_t     _n,
    size_t     _exponent
)
{
    size_t top = 1u;
    size_t bit = 0u;
    size_t i   = 0u;

    // the identity, for 0
    if (_exponent == 0u)
    {
        d_mat_@S@_identity(_out, _n);

        return;
    }

    // the exponent's highest bit
    while (top <= (_exponent / 2u))
    {
        top *= 2u;
    }

    // the power begun as _a
    for (i = 0u; i < (_n * _n); ++i)
    {
        _out[i] = _a[i];
    }

    // each bit below: the power squared, then times _a where the bit is 1
    for (bit = top / 2u; bit > 0u; bit /= 2u)
    {
        d_mat_@S@_multiply(_scratch, _out, _out, _n, _n, _n);

        for (i = 0u; i < (_n * _n); ++i)
        {
            _out[i] = _scratch[i];
        }

        if ((_exponent & bit) != 0u)
        {
            d_mat_@S@_multiply(_scratch, _a, _out, _n, _n, _n);

            for (i = 0u; i < (_n * _n); ++i)
            {
                _out[i] = _scratch[i];
            }
        }
    }

    return;
}

// @SECTION@.4    Square-matrix queries
//------------------------------------------------------------------------------
// trace sums the diagonal; is_symmetric and is_identity allow each entry
// _tolerance.

D_MATH_FN @T@
d_mat_@S@_trace(
    const @T@* _a,
    size_t     _n
)
{
    @T@    acc = (@T@)0;
    size_t i   = 0u;

    for (i = 0u; i < _n; ++i)
    {
        acc += _a[(i * _n) + i];
    }

    return acc;
}

D_MATH_FN bool
d_mat_@S@_is_symmetric(
    const @T@* _a,
    size_t     _n,
    @T@        _tolerance
)
{
    size_t i = 0u;

    for (i = 0u; i < _n; ++i)
    {
        size_t j = i + 1u;

        for (j = i + 1u; j < _n; ++j)
        {
            const @T@ d = _a[(i * _n) + j] - _a[(j * _n) + i];

            if (((d < (@T@)0) ? -d : d) > _tolerance)
            {
                return false;
            }
        }
    }

    return true;
}

D_MATH_FN bool
d_mat_@S@_is_identity(
    const @T@* _a,
    size_t     _n,
    @T@        _tolerance
)
{
    size_t i = 0u;

    for (i = 0u; i < _n; ++i)
    {
        size_t j = 0u;

        for (j = 0u; j < _n; ++j)
        {
            const @T@ d = _a[(i * _n) + j] - ((i == j) ? (@T@)1 : (@T@)0);

            if (((d < (@T@)0) ? -d : d) > _tolerance)
            {
                return false;
            }
        }
    }

    return true;
}

// @SECTION@.5    Determinant, inverse, solution and cofactors
//------------------------------------------------------------------------------
// The square matrix's algebra, as linear_algebra/square.hpp computes it: the
// determinant by Bareiss's fraction-free elimination (exact for integers,
// and the subframework's algorithm), the inverse by Gauss-Jordan with
// partial pivoting, and the solution of A X = B by Gaussian elimination of
// [A | B] with partial pivoting, LAPACK's dgesv (S257). Each product is
// rounded before it is combined. Every kernel takes the scratch it needs
// from the caller.

// d_mat_@S@_determinant
//   the determinant of the _n x _n matrix _a; _scratch holds _n * _n
// entries. Bareiss: a row with a zero pivot is swapped for the first below
// it with a non-zero entry in the column, flipping the sign; none means 0.
D_MATH_FN @T@
d_mat_@S@_determinant(
    const @T@* _a,
    @T@*       _scratch,
    size_t     _n
)
{
    @T@    prev = (@T@)1;
    @T@    sign = (@T@)1;
    size_t k    = 0u;

    for (k = 0u; k < (_n * _n); ++k)
    {
        _scratch[k] = _a[k];
    }

    for (k = 0u; k < _n; ++k)
    {
        size_t i = 0u;

        // a zero pivot: swap in the first row below with a non-zero entry
        if (_scratch[(k * _n) + k] == (@T@)0)
        {
            size_t swap_row = k;
            size_t r        = k + 1u;
            size_t c        = 0u;

            for (r = k + 1u; r < _n; ++r)
            {
                if (_scratch[(r * _n) + k] != (@T@)0)
                {
                    swap_row = r;
                    break;
                }
            }

            // the column is zero below the diagonal: the matrix is singular
            if (swap_row == k)
            {
                return (@T@)0;
            }

            for (c = 0u; c < _n; ++c)
            {
                const @T@ t = _scratch[(k * _n) + c];

                _scratch[(k * _n) + c]        = _scratch[(swap_row * _n) + c];
                _scratch[(swap_row * _n) + c] = t;
            }

            sign = -sign;
        }

        // the fraction-free update of the block below and right
        for (i = k + 1u; i < _n; ++i)
        {
            size_t j = k + 1u;

            for (j = k + 1u; j < _n; ++j)
            {
                const @T@ keep  = _scratch[(k * _n) + k] *
                                  _scratch[(i * _n) + j];
                const @T@ cross = _scratch[(i * _n) + k] *
                                  _scratch[(k * _n) + j];
                // one operation per statement: an x87 evaluates a whole
                // expression at 64 bits and rounds once, at the end
                const @T@ diff  = keep - cross;

                _scratch[(i * _n) + j] = diff / prev;
            }
        }

        prev = _scratch[(k * _n) + k];
    }

    return sign * prev;
}

// d_mat_@S@_inverse
//   _out = the inverse of the _n x _n matrix _a, true; or, for a singular
// one, the zero matrix and false. _scratch holds _n * _n entries; neither it
// nor _out may be _a. Gauss-Jordan: the pivot is the first entry of largest
// magnitude at or below the diagonal.
D_MATH_FN bool
d_mat_@S@_inverse(
    @T@*       _out,
    @T@*       _scratch,
    const @T@* _a,
    size_t     _n
)
{
    size_t col = 0u;
    size_t k   = 0u;

    for (k = 0u; k < (_n * _n); ++k)
    {
        _scratch[k] = _a[k];
    }

    d_mat_@S@_identity(_out, _n);

    for (col = 0u; col < _n; ++col)
    {
        size_t    pivot = col;
        @T@       maxv  = (_scratch[(col * _n) + col] < (@T@)0)
                              ? -_scratch[(col * _n) + col]
                              : _scratch[(col * _n) + col];
        size_t    r     = col + 1u;
        size_t    c     = 0u;
        @T@       d     = (@T@)0;

        // the first entry of largest magnitude at or below the diagonal
        for (r = col + 1u; r < _n; ++r)
        {
            const @T@ v = (_scratch[(r * _n) + col] < (@T@)0)
                              ? -_scratch[(r * _n) + col]
                              : _scratch[(r * _n) + col];

            if (v > maxv)
            {
                maxv  = v;
                pivot = r;
            }
        }

        // no pivot: singular
        if (_scratch[(pivot * _n) + col] == (@T@)0)
        {
            d_vec_@S@_fill(_out, _n * _n, (@T@)0);

            return false;
        }

        // the pivot row up into place, in both
        if (pivot != col)
        {
            for (c = 0u; c < _n; ++c)
            {
                const @T@ ta = _scratch[(col * _n) + c];
                const @T@ ti = _out[(col * _n) + c];

                _scratch[(col * _n) + c]   = _scratch[(pivot * _n) + c];
                _scratch[(pivot * _n) + c] = ta;
                _out[(col * _n) + c]       = _out[(pivot * _n) + c];
                _out[(pivot * _n) + c]     = ti;
            }
        }

        d = _scratch[(col * _n) + col];

        // the pivot row divided through
        for (c = 0u; c < _n; ++c)
        {
            _scratch[(col * _n) + c] = _scratch[(col * _n) + c] / d;
            _out[(col * _n) + c]     = _out[(col * _n) + c] / d;
        }

        // every other row cleared in the pivot's column
        for (r = 0u; r < _n; ++r)
        {
            if (r != col)
            {
                const @T@ f = _scratch[(r * _n) + col];

                for (c = 0u; c < _n; ++c)
                {
                    const @T@ fa = f * _scratch[(col * _n) + c];
                    const @T@ fi = f * _out[(col * _n) + c];

                    _scratch[(r * _n) + c] = _scratch[(r * _n) + c] - fa;
                    _out[(r * _n) + c]     = _out[(r * _n) + c] - fi;
                }
            }
        }
    }

    return true;
}

// d_mat_@S@_solve
//   _x = the solution X of A X = B, A n by n and B n by _m, true; or, for a
// singular A, the zero matrix and false. Gaussian elimination of [A | B]
// with partial pivoting -- the pivot the first entry of largest magnitude
// at or below the diagonal, the rows below less their multiples of it, B's
// rows along -- then back-substitution, as LAPACK's dgesv (S257): for each
// column of B the same bits as decomp.h's d_decomp_@S@_lu and
// d_decomp_@S@_lu_solve give. _scratch holds n * n entries; neither it nor
// _x may be _a or _b.
D_MATH_FN bool
d_mat_@S@_solve(
    @T@*       _x,
    @T@*       _scratch,
    const @T@* _a,
    const @T@* _b,
    size_t     _n,
    size_t     _m
)
{
    size_t col = 0u;
    size_t k   = 0u;

    for (k = 0u; k < (_n * _n); ++k)
    {
        _scratch[k] = _a[k];
    }

    for (k = 0u; k < (_n * _m); ++k)
    {
        _x[k] = _b[k];
    }

    // elimination: [A | B] to [U | Y]
    for (col = 0u; col < _n; ++col)
    {
        size_t pivot = col;
        @T@    maxv  = (_scratch[(col * _n) + col] < (@T@)0)
                           ? -_scratch[(col * _n) + col]
                           : _scratch[(col * _n) + col];
        size_t r     = 0u;
        size_t c     = 0u;

        // the largest magnitude in the column, at or below the diagonal
        for (r = col + 1u; r < _n; ++r)
        {
            const @T@ v = (_scratch[(r * _n) + col] < (@T@)0)
                              ? -_scratch[(r * _n) + col]
                              : _scratch[(r * _n) + col];

            if (v > maxv)
            {
                maxv  = v;
                pivot = r;
            }
        }

        // no pivot: singular
        if (_scratch[(pivot * _n) + col] == (@T@)0)
        {
            for (k = 0u; k < (_n * _m); ++k)
            {
                _x[k] = (@T@)0;
            }

            return false;
        }

        // the pivot row up, in A and in B
        if (pivot != col)
        {
            for (c = 0u; c < _n; ++c)
            {
                const @T@ t = _scratch[(col * _n) + c];

                _scratch[(col * _n) + c]   = _scratch[(pivot * _n) + c];
                _scratch[(pivot * _n) + c] = t;
            }

            for (c = 0u; c < _m; ++c)
            {
                const @T@ t = _x[(col * _m) + c];

                _x[(col * _m) + c]   = _x[(pivot * _m) + c];
                _x[(pivot * _m) + c] = t;
            }
        }

        // the rows below less their multiples of it
        {
            const @T@ pivot_value = _scratch[(col * _n) + col];

            for (r = col + 1u; r < _n; ++r)
            {
                const @T@ f = _scratch[(r * _n) + col] / pivot_value;

                for (c = col + 1u; c < _n; ++c)
                {
                    const @T@ prod = f * _scratch[(col * _n) + c];

                    _scratch[(r * _n) + c] = _scratch[(r * _n) + c] - prod;
                }

                for (c = 0u; c < _m; ++c)
                {
                    const @T@ prod = f * _x[(col * _m) + c];

                    _x[(r * _m) + c] = _x[(r * _m) + c] - prod;
                }
            }
        }
    }

    // back-substitution, column by column of B, the last row first
    for (k = 0u; k < _m; ++k)
    {
        size_t i = 0u;

        for (i = _n; i-- > 0u; )
        {
            @T@    s = _x[(i * _m) + k];
            size_t j = 0u;

            for (j = i + 1u; j < _n; ++j)
            {
                const @T@ prod = _scratch[(i * _n) + j] * _x[(j * _m) + k];

                s = s - prod;
            }

            _x[(i * _m) + k] = s / _scratch[(i * _n) + i];
        }
    }

    return true;
}

// d_mat_@S@_submatrix
//   _out ((_n - 1) x (_n - 1)) = _a without row _row and column _col.
D_MATH_FN void
d_mat_@S@_submatrix(
    @T@*       _out,
    const @T@* _a,
    size_t     _n,
    size_t     _row,
    size_t     _col
)
{
    size_t rr = 0u;
    size_t r  = 0u;

    for (r = 0u; r < _n; ++r)
    {
        if (r != _row)
        {
            size_t cc = 0u;
            size_t c  = 0u;

            for (c = 0u; c < _n; ++c)
            {
                if (c != _col)
                {
                    _out[(rr * (_n - 1u)) + cc] = _a[(r * _n) + c];
                    ++cc;
                }
            }

            ++rr;
        }
    }

    return;
}

// d_mat_@S@_cofactor
//   (-1)^(_row + _col) times the minor, the determinant of the submatrix
// without _row and _col; _scratch holds 2 (_n - 1)^2 entries.
D_MATH_FN @T@
d_mat_@S@_cofactor(
    const @T@* _a,
    @T@*       _scratch,
    size_t     _n,
    size_t     _row,
    size_t     _col
)
{
    const size_t m    = _n - 1u;
    const @T@    sign = (((_row + _col) % 2u) == 0u) ? (@T@)1 : (@T@)-1;

    d_mat_@S@_submatrix(_scratch, _a, _n, _row, _col);

    return sign * d_mat_@S@_determinant(_scratch, _scratch + (m * m), m);
}

// d_mat_@S@_cofactor_matrix
//   _out (_n x _n) = every cofactor of _a, in place; _scratch holds
// 2 (_n - 1)^2 entries. _adjugate set writes the transpose instead: the
// adjugate.
D_MATH_FN void
d_mat_@S@_cofactor_matrix(
    @T@*       _out,
    const @T@* _a,
    @T@*       _scratch,
    size_t     _n,
    bool       _adjugate
)
{
    size_t i = 0u;

    for (i = 0u; i < _n; ++i)
    {
        size_t j = 0u;

        for (j = 0u; j < _n; ++j)
        {
            const @T@ c = d_mat_@S@_cofactor(_a, _scratch, _n, i, j);

            _out[_adjugate ? ((j * _n) + i) : ((i * _n) + j)] = c;
        }
    }

    return;
}

// @SECTION@.6    Rotations
//------------------------------------------------------------------------------
// The rotation matrices of linear_algebra/transform.hpp, from an angle's
// cosine _c and sine _s, which the face computes as its level allows.
// Counter-clockwise; one operation per statement.

// d_mat_@S@_rotation_2d
//   _out (2 x 2) = [c -s; s c].
D_MATH_FN void
d_mat_@S@_rotation_2d(
    @T@* _out,
    @T@  _c,
    @T@  _s
)
{
    _out[0] = _c;
    _out[1] = -_s;
    _out[2] = _s;
    _out[3] = _c;

    return;
}

// d_mat_@S@_rotation_3d
//   _out (3 x 3) = the rotation about axis _axis (0 x, 1 y, 2 z).
D_MATH_FN void
d_mat_@S@_rotation_3d(
    @T@*   _out,
    size_t _axis,
    @T@    _c,
    @T@    _s
)
{
    // the two axes the rotation turns, in right-handed order
    const size_t a = (_axis + 1u) % 3u;
    const size_t b = (_axis + 2u) % 3u;

    d_mat_@S@_identity(_out, 3u);

    _out[(a * 3u) + a] = _c;
    _out[(a * 3u) + b] = -_s;
    _out[(b * 3u) + a] = _s;
    _out[(b * 3u) + b] = _c;

    return;
}

// d_mat_@S@_rotation_axis
//   _out (3 x 3) = the rotation about the unit axis _k by the angle whose
// cosine and sine are _c and _s, Rodrigues's formula: c I + (1 - c) k k^T +
// s [k]x, each entry formed in the order c + (k_i k_j) t, (k_i k_j) t -+
// k_l s.
D_MATH_FN void
d_mat_@S@_rotation_axis(
    @T@*       _out,
    const @T@* _k,
    @T@        _c,
    @T@        _s
)
{
    const @T@ t = (@T@)1 - _c;
    size_t    i = 0u;

    for (i = 0u; i < 3u; ++i)
    {
        size_t j = 0u;

        for (j = 0u; j < 3u; ++j)
        {
            const @T@ kk    = _k[i] * _k[j];
            const @T@ along = kk * t;

            // the diagonal: c + k_i^2 t
            if (i == j)
            {
                _out[(i * 3u) + j] = _c + along;
            }
            // off the diagonal: k_i k_j t -+ k_l s, l the third axis
            else
            {
                const size_t l    = 3u - i - j;
                const @T@    turn = _k[l] * _s;
                // the cross term's sign: +s where (i, j, l) is an even
                // permutation's (i, j) = (1, 0), (0, 2), (2, 1)
                const bool   plus = ( ((i == 1u) && (j == 0u)) ||
                                      ((i == 0u) && (j == 2u)) ||
                                      ((i == 2u) && (j == 1u)) );

                _out[(i * 3u) + j] = plus ? (along + turn) : (along - turn);
            }
        }
    }

    return;
}
"""

MAT_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE FLOAT FAMILY (_f)
    ---------------------
    1.  Construction
    2.  Rows, columns and the transpose
    3.  Products
    4.  Square-matrix queries
    5.  Determinant, inverse, solution and cofactors
    6.  Rotations
2.  THE DOUBLE FAMILY (_d)
    ----------------------
    1.  Construction
    2.  Rows, columns and the transpose
    3.  Products
    4.  Square-matrix queries
    5.  Determinant, inverse, solution and cofactors
    6.  Rotations
3.  THE LONG DOUBLE FAMILY (_ld)
    ----------------------------
    1.  Construction
    2.  Rows, columns and the transpose
    3.  Products
    4.  Square-matrix queries
    5.  Determinant, inverse, solution and cofactors
    6.  Rotations
4.  ONE NAME FOR EVERY FAMILY (C++)
    -------------------------------
*/

#ifndef DJINTERP_C_MATH_MAT_H
#define DJINTERP_C_MATH_MAT_H 1

// std
#include <stddef.h>          // size_t
// djinterp
#include "./math_common.h"   // D_MATH_FN
#include "./vec.h"           // d_vec_*_fill, _dot: the entry-wise kernels
"""

MAT_TAIL = r"""

#endif  // DJINTERP_C_MATH_MAT_H
"""

_MA = ("const @T@*", "_a")
_MO = ("@T@*", "_out")
_SZ = lambda n: ("size_t", n)
MAT_OVERLOADS = [
    ("D_MATH_FN", "void", "d_mat_identity", [_MO, _SZ("_n")]),
    ("D_MATH_FN", "void", "d_mat_diagonal", [_MO, ("const @T@*", "_diagonal"), _SZ("_n")]),
    ("D_MATH_FN", "void", "d_mat_row", [_MO, _MA, _SZ("_i"), _SZ("_cols")]),
    ("D_MATH_FN", "void", "d_mat_col", [_MO, _MA, _SZ("_j"), _SZ("_rows"), _SZ("_cols")]),
    ("D_MATH_FN", "void", "d_mat_transpose", [_MO, _MA, _SZ("_rows"), _SZ("_cols")]),
    ("D_MATH_FN", "void", "d_mat_multiply", [_MO, _MA, ("const @T@*", "_b"), _SZ("_rows"), _SZ("_inner"), _SZ("_cols")]),
    ("D_MATH_FN", "void", "d_mat_multiply_vector", [_MO, _MA, ("const @T@*", "_v"), _SZ("_rows"), _SZ("_cols")]),
    ("D_MATH_FN", "void", "d_mat_power", [_MO, ("@T@*", "_scratch"), _MA, _SZ("_n"), _SZ("_exponent")]),
    ("D_MATH_FN", "@T@", "d_mat_trace", [_MA, _SZ("_n")]),
    ("D_MATH_FN", "bool", "d_mat_is_symmetric", [_MA, _SZ("_n"), ("@T@", "_tolerance")]),
    ("D_MATH_FN", "bool", "d_mat_is_identity", [_MA, _SZ("_n"), ("@T@", "_tolerance")]),
    ("D_MATH_FN", "@T@", "d_mat_determinant", [_MA, ("@T@*", "_scratch"), _SZ("_n")]),
    ("D_MATH_FN", "bool", "d_mat_inverse", [_MO, ("@T@*", "_scratch"), _MA, _SZ("_n")]),
    ("D_MATH_FN", "bool", "d_mat_solve", [("@T@*", "_x"), ("@T@*", "_scratch"), _MA, ("const @T@*", "_b"), _SZ("_n"), _SZ("_m")]),
    ("D_MATH_FN", "void", "d_mat_submatrix", [_MO, _MA, _SZ("_n"), _SZ("_row"), _SZ("_col")]),
    ("D_MATH_FN", "@T@", "d_mat_cofactor", [_MA, ("@T@*", "_scratch"), _SZ("_n"), _SZ("_row"), _SZ("_col")]),
    ("D_MATH_FN", "void", "d_mat_cofactor_matrix", [_MO, _MA, ("@T@*", "_scratch"), _SZ("_n"), ("bool", "_adjugate")]),
    ("D_MATH_FN", "void", "d_mat_rotation_2d", [_MO, ("@T@", "_c"), ("@T@", "_s")]),
    ("D_MATH_FN", "void", "d_mat_rotation_3d", [_MO, _SZ("_axis"), ("@T@", "_c"), ("@T@", "_s")]),
    ("D_MATH_FN", "void", "d_mat_rotation_axis", [_MO, ("const @T@*", "_k"), ("@T@", "_c"), ("@T@", "_s")]),
]


def mat():
    lines = [
        "Fixed-size matrices: the C core of math/linear_algebra/matrix.hpp, for",
        "float, double and long double.",
        "  GENERATED by scripts/gen_c_math_float.py from one text for the three",
        "families: edit the generator, not this file.",
        "",
        "  A matrix here is a caller's row-major array of rows * cols entries, as",
        "BLAS takes them; the C++ face's storage is that array. What is entry-wise",
        "-- sums, scaling, norms, comparison -- is vec.h's over the entries; this",
        "header has what knows the shape. Every kernel is a constant expression",
        "from C++14.",
    ]
    text = banner("mat.h", lines, revised_on="2026.11.07")
    text += "\n" + MAT_HEAD
    for i, (suffix, ctype, prefix, prose) in enumerate(FAMILIES):
        upper = prose.upper()
        body = MAT_TEMPLATE.replace("@SECTION@", str(i + 1)).replace("@UPPER@", upper)
        text += instantiate(body, suffix, ctype, prefix, prose)
    text += overload_section("4.  ONE NAME FOR EVERY FAMILY (C++)", MAT_OVERLOADS)
    text += MAT_TAIL
    return text



# -----------------------------------------------------------------------------
# quat.h
# -----------------------------------------------------------------------------

QUAT_KERNELS_PURE = r"""

//==============================================================================
// 1.  THE KERNELS
//==============================================================================
// Once, in long double, shared by the three families (math_common.h's
// D_MATH_FN_KERNEL). A family's values are long doubles exactly, and its part
// is the kernel's rounded once (d_internal_math_*_round_pair). Sums that
// cancel are carried as pairs (elementary.h's section 1.1), each product of
// two parts exact by Dekker's, so that a part's error is some 2^-2P of its
// terms, P long double's precision -- past twice a double's on the x87, and
// twice a double's where long double is double. Operands are scaled by powers
// of 2 to a largest part in [1, 2) where Dekker's products could overflow or
// lose their low parts.


// 1.1    Pairs and scaling
//------------------------------------------------------------------------------

// d_internal_quat_ld_finite
//   whether each of _v's _n parts is finite.
D_MATH_FN_KERNEL bool
d_internal_quat_ld_finite(
    const long double* _v,
    size_t             _n
)
{
    size_t i = 0u;

    // a NaN or an infinity among the parts
    for (i = 0u; i < _n; ++i)
    {
        if ( (_v[i] != _v[i]) ||
             (d_internal_cplx_ld_is_inf(_v[i])) )
        {
            return false;
        }
    }

    return true;
}

// d_internal_quat_ld_has_nan
//   whether any of _v's _n parts is a NaN.
D_MATH_FN_KERNEL bool
d_internal_quat_ld_has_nan(
    const long double* _v,
    size_t             _n
)
{
    size_t i = 0u;

    // a NaN among the parts
    for (i = 0u; i < _n; ++i)
    {
        if (_v[i] != _v[i])
        {
            return true;
        }
    }

    return false;
}

// d_internal_quat_ld_has_inf
//   whether any of _v's _n parts is an infinity.
D_MATH_FN_KERNEL bool
d_internal_quat_ld_has_inf(
    const long double* _v,
    size_t             _n
)
{
    size_t i = 0u;

    // an infinity among the parts
    for (i = 0u; i < _n; ++i)
    {
        if (d_internal_cplx_ld_is_inf(_v[i]))
        {
            return true;
        }
    }

    return false;
}

// d_internal_quat_ld_is_zero
//   whether each of _v's _n parts is 0, of either sign.
D_MATH_FN_KERNEL bool
d_internal_quat_ld_is_zero(
    const long double* _v,
    size_t             _n
)
{
    size_t i = 0u;

    // a part that is not 0
    for (i = 0u; i < _n; ++i)
    {
        if (_v[i] != 0.0L)
        {
            return false;
        }
    }

    return true;
}

// d_internal_quat_ld_safe
//   whether each of _v's _n parts lies where Dekker's products of two such
// parts neither overflow nor lose their low parts: 0, or from 2^(EMIN/2 + P)
// to 2^(EMAX/2 - P) in magnitude, as cplx.h's d_internal_cplx_ld_safe.
D_MATH_FN_KERNEL bool
d_internal_quat_ld_safe(
    const long double* _v,
    size_t             _n
)
{
    const long double low  =
        d_internal_math_ld_pow2((LDBL_MIN_EXP / 2) + LDBL_MANT_DIG);
    const long double high =
        d_internal_math_ld_pow2((LDBL_MAX_EXP / 2) - LDBL_MANT_DIG);
    size_t            i    = 0u;

    // a part outside the range
    for (i = 0u; i < _n; ++i)
    {
        const long double a = d_internal_cplx_ld_abs(_v[i]);

        if ( (a != 0.0L) &&
             ( (a < low) ||
               (a > high) ) )
        {
            return false;
        }
    }

    return true;
}

// d_internal_quat_ld_exponent
//   the binade of the largest magnitude among _v's _n parts, each finite and
// one not 0.
D_MATH_FN_KERNEL int
d_internal_quat_ld_exponent(
    const long double* _v,
    size_t             _n
)
{
    long double largest = 0.0L;
    size_t      i       = 0u;

    // the largest magnitude
    for (i = 0u; i < _n; ++i)
    {
        const long double a = d_internal_cplx_ld_abs(_v[i]);

        if (a > largest)
        {
            largest = a;
        }
    }

    return d_internal_cplx_ld_exponent(largest);
}

// d_internal_quat_ld_scale_all
//   _out = each of _v's _n parts times 2^_k, each rounded once: exact but
// where it falls into the subnormals, 2^-2P and more below the largest part
// once that is in [1, 2). _out may be _v.
D_MATH_FN_KERNEL void
d_internal_quat_ld_scale_all(
    long double*       _out,
    const long double* _v,
    size_t             _n,
    int                _k
)
{
    size_t i = 0u;

    // each part by the power
    for (i = 0u; i < _n; ++i)
    {
        _out[i] = d_internal_cplx_ld_scale(_v[i], _k);
    }

    return;
}

// d_internal_quat_ld_scale_pair
//   _p 2^_k, each part scaled: exact but past the largest finite value or in
// the subnormals.
D_MATH_FN_KERNEL struct dj_internal_math_ld_pair
d_internal_quat_ld_scale_pair(
    struct dj_internal_math_ld_pair _p,
    int                             _k
)
{
    struct dj_internal_math_ld_pair result = { 0.0L, 0.0L };

    result.high = d_internal_cplx_ld_scale(_p.high, _k);
    result.low  = d_internal_cplx_ld_scale(_p.low, _k);

    return result;
}

// d_internal_quat_ld_fill_nan
//   _out = _n pairs, each a NaN.
D_MATH_FN_KERNEL void
d_internal_quat_ld_fill_nan(
    struct dj_internal_math_ld_pair* _out,
    size_t                           _n
)
{
    size_t i = 0u;

    // every part a NaN
    for (i = 0u; i < _n; ++i)
    {
        _out[i].high = d_internal_math_ld_nan();
        _out[i].low  = 0.0L;
    }

    return;
}

// d_internal_quat_ld_direction
//   _out = the direction of _v's _n parts, at least one infinite: each
// infinite part +-1 / sqrt(their count), the rest 0 of their own signs -- the
// limit of v / |v| as the finite parts are left behind.
D_MATH_FN_KERNEL void
d_internal_quat_ld_direction(
    long double*       _out,
    const long double* _v,
    size_t             _n
)
{
    size_t      count = 0u;
    size_t      i     = 0u;
    long double unit  = 1.0L;

    // the infinite parts' count
    for (i = 0u; i < _n; ++i)
    {
        if (d_internal_cplx_ld_is_inf(_v[i]))
        {
            ++count;
        }
    }

    // 1 / sqrt(count), for a count of 2, 3 or 4
    if (count == 2u)
    {
        unit = 0.707106781186547524400844362104849039L;
    }
    else if (count == 3u)
    {
        unit = 0.577350269189625764509148780501957456L;
    }
    else if (count == 4u)
    {
        unit = 0.5L;
    }

    // each part along the infinities
    for (i = 0u; i < _n; ++i)
    {
        const bool infinite = d_internal_cplx_ld_is_inf(_v[i]);

        _out[i] = d_internal_cplx_ld_copysign(infinite ? unit : 0.0L, _v[i]);
    }

    return;
}

// d_internal_quat_ld_along
//   _u _s, where _u is 0 a zero, its sign _u's times _s's: a part of a vector
// that is 0 stays 0, whatever the value it is taken along -- an infinity or a
// NaN among them -- as an exact 0 times anything is 0.
D_MATH_FN_KERNEL long double
d_internal_quat_ld_along(
    long double _u,
    long double _s
)
{
    // a zero part: a zero, of the product's sign
    if (_u == 0.0L)
    {
        const long double zero = d_internal_cplx_ld_copysign(0.0L, _s);

        return (d_internal_cplx_ld_copysign(1.0L, _u) < 0.0L) ? -zero : zero;
    }

    return _u * _s;
}

// d_internal_quat_ld_dot
//   the sum of the _n products _a[i] _b[i], in order, as a pair: each product
// exact (Dekker's, its factors safe or scaled), the high parts summed by
// two-sums, the low parts and the two-sums' errors after, and the pair
// renormalized -- Ogita, Rump and Oishi's Dot2, as accurate as the sum
// computed in twice the precision and rounded once, so that its high part is
// the sum rounded once wherever it does not cancel past 2^P of its terms. A
// sum of 0 is +0, as IEEE's arithmetic makes an exact sum of terms not all
// 0; where every product is 0, their sum in order, -0 where each is -0.
D_MATH_FN_KERNEL struct dj_internal_math_ld_pair
d_internal_quat_ld_dot(
    const long double* _a,
    const long double* _b,
    size_t             _n
)
{
    struct dj_internal_math_ld_pair result = { 0.0L, 0.0L };
    long double                     high   = 0.0L;
    long double                     rest   = 0.0L;
    long double                     zeros  = 0.0L;
    bool                            zero   = true;
    size_t                          i      = 0u;

    // each product exactly, its high part into the running sum
    for (i = 0u; i < _n; ++i)
    {
        const struct dj_internal_math_ld_pair p =
            d_internal_math_ld_product_exact(_a[i], _b[i]);
        const struct dj_internal_math_ld_pair s =
            d_internal_math_ld_two_sum(high, p.high);
        const long double term = _a[i] * _b[i];

        high  = s.high;
        rest  = rest + (s.low + p.low);
        zero  = ( (zero) &&
                  (term == 0.0L) );
        zeros = (i == 0u) ? term : (zeros + term);
    }

    result = d_internal_math_ld_two_sum(high, rest);

    // an exact zero: +0, or the zero products' own sum
    if (result.high == 0.0L)
    {
        result.high = zero ? zeros : 0.0L;
        result.low  = 0.0L;
    }

    return result;
}

// d_internal_quat_ld_over
//   (_u.high + _u.low) / (_n.high + _n.low) as a pair, _n.high in [1, 32):
// cplx.h's d_internal_cplx_ld_over with its last sum kept as a pair -- the
// quotient q of the high parts, the residual u - q n formed exactly by
// Dekker's product, and q with the residual over n -- the quotient to some
// 2^-2P of it. A zero is q itself, of its sign; a quotient too near the
// subnormals for Dekker's product q less q n_lo / n.
D_MATH_FN_KERNEL struct dj_internal_math_ld_pair
d_internal_quat_ld_over(
    struct dj_internal_math_ld_pair _u,
    struct dj_internal_math_ld_pair _n
)
{
    const long double               q0     = _u.high / _n.high;
    const long double               least  =
        LDBL_MIN * d_internal_math_ld_pow2(LDBL_MANT_DIG + 4);
    struct dj_internal_math_ld_pair result = { 0.0L, 0.0L };

    result.high = q0;

    // a zero: itself, its sign kept
    if (q0 == 0.0L)
    {
        return result;
    }

    // too near the subnormals for Dekker's product
    if (d_internal_cplx_ld_abs(q0) < least)
    {
        result.high = q0 - (q0 * (_n.low / _n.high));

        return result;
    }

    {
        const struct dj_internal_math_ld_pair t =
            d_internal_math_ld_product_exact(q0, _n.high);
        // u - q n: u's high part less t's is exact, the two within an ulp
        const long double head = (_u.high - t.high) - t.low;
        const long double r    = (head + _u.low) - (q0 * _n.low);

        result = d_internal_math_ld_two_sum(q0, r / _n.high);
    }

    return result;
}

// d_internal_quat_ld_lerp
//   (1 - _t) _a + _t _b as a pair: 1 - t exactly as a pair, each product
// a pair, their sum a pair -- exact at t = 0 and t = 1.
D_MATH_FN_KERNEL struct dj_internal_math_ld_pair
d_internal_quat_ld_lerp(
    long double _a,
    long double _b,
    long double _t
)
{
    const struct dj_internal_math_ld_pair rest =
        d_internal_math_ld_two_sum(1.0L, -_t);
    const struct dj_internal_math_ld_pair from =
        d_internal_math_ld_pair_product(
            rest, d_internal_math_ld_pair_of(_a, 0.0L));
    const struct dj_internal_math_ld_pair to =
        d_internal_math_ld_product_exact(_t, _b);

    return d_internal_math_ld_pair_sum(from, to);
}

// d_internal_quat_ld_dot_sign
//   the sign of _a . _b, _n parts each, finite: their dot product's high part
// (d_internal_quat_ld_dot), each operand scaled first where it leaves the
// safe range; 0 where either is 0.
D_MATH_FN_KERNEL long double
d_internal_quat_ld_dot_sign(
    const long double* _a,
    const long double* _b,
    size_t             _n
)
{
    long double a[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    long double b[4] = { 0.0L, 0.0L, 0.0L, 0.0L };

    // a zero operand: no sign
    if ( (d_internal_quat_ld_is_zero(_a, _n)) ||
         (d_internal_quat_ld_is_zero(_b, _n)) )
    {
        return 0.0L;
    }

    d_internal_quat_ld_scale_all(a, _a, _n,
                                 -d_internal_quat_ld_exponent(_a, _n));
    d_internal_quat_ld_scale_all(b, _b, _n,
                                 -d_internal_quat_ld_exponent(_b, _n));

    return d_internal_quat_ld_dot(a, b, _n).high;
}


// 1.2    The algebra
//------------------------------------------------------------------------------
// Hamilton's product and the quotients, each part a sum of four products as
// d_internal_quat_ld_dot sums them, for finite operands; the families take
// the formulas as written where a part is infinite or a NaN.

// d_internal_quat_ld_hamilton
//   _out = _a _b, each part a pair (d_internal_quat_ld_dot), its four
// products in the order of the formula (d_quat_*_multiply_fast): x = w x' +
// x w' + y z' - z y', y = w y' - x z' + y w' + z x', z = w z' + x y' - y x' + z
// w', w = w w' - x x' - y y' - z z'. The factors safe, or scaled by the
// caller.
D_MATH_FN_KERNEL void
d_internal_quat_ld_hamilton(
    struct dj_internal_math_ld_pair* _out,
    const long double*               _a,
    const long double*               _b
)
{
    const long double x_a[4] = { _a[3],  _a[0],  _a[1], -_a[2] };
    const long double x_b[4] = { _b[0],  _b[3],  _b[2],  _b[1] };
    const long double y_a[4] = { _a[3], -_a[0],  _a[1],  _a[2] };
    const long double y_b[4] = { _b[1],  _b[2],  _b[3],  _b[0] };
    const long double z_a[4] = { _a[3],  _a[0], -_a[1],  _a[2] };
    const long double z_b[4] = { _b[2],  _b[1],  _b[0],  _b[3] };
    const long double w_a[4] = { _a[3], -_a[0], -_a[1], -_a[2] };
    const long double w_b[4] = { _b[3],  _b[0],  _b[1],  _b[2] };

    _out[0] = d_internal_quat_ld_dot(x_a, x_b, 4u);
    _out[1] = d_internal_quat_ld_dot(y_a, y_b, 4u);
    _out[2] = d_internal_quat_ld_dot(z_a, z_b, 4u);
    _out[3] = d_internal_quat_ld_dot(w_a, w_b, 4u);

    return;
}

// d_internal_quat_ld_multiply
//   _out = _a _b, both finite (d_internal_quat_ld_hamilton): where either
// leaves the safe range, each scaled to a largest part in [1, 2) and the
// parts scaled back, each rounded once more where it falls into the
// subnormals.
D_MATH_FN_KERNEL void
d_internal_quat_ld_multiply(
    struct dj_internal_math_ld_pair* _out,
    const long double*               _a,
    const long double*               _b
)
{
    // safe, or a zero operand: the parts as they are
    if ( ( (d_internal_quat_ld_safe(_a, 4u)) &&
           (d_internal_quat_ld_safe(_b, 4u)) ) ||
         (d_internal_quat_ld_is_zero(_a, 4u)) ||
         (d_internal_quat_ld_is_zero(_b, 4u)) )
    {
        d_internal_quat_ld_hamilton(_out, _a, _b);

        return;
    }

    {
        const int   ea   = d_internal_quat_ld_exponent(_a, 4u);
        const int   eb   = d_internal_quat_ld_exponent(_b, 4u);
        long double a[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
        long double b[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
        size_t      i    = 0u;

        d_internal_quat_ld_scale_all(a, _a, 4u, -ea);
        d_internal_quat_ld_scale_all(b, _b, 4u, -eb);
        d_internal_quat_ld_hamilton(_out, a, b);

        // each part back to its scale
        for (i = 0u; i < 4u; ++i)
        {
            _out[i] = d_internal_quat_ld_scale_pair(_out[i], ea + eb);
        }
    }

    return;
}

// d_internal_quat_ld_inverse
//   _out = 1 / _q = q* / |q|^2, q finite and not 0: q scaled to a largest
// part in [1, 2), so that |q|^2, the sum of its exact squares as a pair, lies
// in [1, 16); each part of q* over it (d_internal_quat_ld_over), and scaled
// back.
D_MATH_FN_KERNEL void
d_internal_quat_ld_inverse(
    struct dj_internal_math_ld_pair* _out,
    const long double*               _q
)
{
    const int   e    = d_internal_quat_ld_exponent(_q, 4u);
    long double q[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    size_t      i    = 0u;

    d_internal_quat_ld_scale_all(q, _q, 4u, -e);

    {
        const struct dj_internal_math_ld_pair n =
            d_internal_quat_ld_dot(q, q, 4u);

        // each part of the conjugate over the squared norm
        for (i = 0u; i < 4u; ++i)
        {
            const long double part = (i < 3u) ? -q[i] : q[i];

            _out[i] = d_internal_quat_ld_scale_pair(
                d_internal_quat_ld_over(
                    d_internal_math_ld_pair_of(part, 0.0L), n),
                -e);
        }
    }

    return;
}

// d_internal_quat_ld_divide
//   _out = _p / _q = p q* / |q|^2 where _right, the quotient on the right, and
// q^-1 p = q* p / |q|^2 otherwise, both finite and q not 0: each operand
// scaled to a largest part in [1, 2), p left as it is where it is 0; the
// numerator's parts as pairs (d_internal_quat_ld_hamilton), each over |q|^2's
// pair (d_internal_quat_ld_over), scaled back by p's exponent less q's.
D_MATH_FN_KERNEL void
d_internal_quat_ld_divide(
    struct dj_internal_math_ld_pair* _out,
    const long double*               _p,
    const long double*               _q,
    bool                             _right
)
{
    const int   eq   = d_internal_quat_ld_exponent(_q, 4u);
    const int   ep   = (d_internal_quat_ld_is_zero(_p, 4u)) ?
                           0 : d_internal_quat_ld_exponent(_p, 4u);
    long double p[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    long double q[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    long double c[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    size_t      i    = 0u;

    d_internal_quat_ld_scale_all(p, _p, 4u, -ep);
    d_internal_quat_ld_scale_all(q, _q, 4u, -eq);
    c[0] = -q[0];
    c[1] = -q[1];
    c[2] = -q[2];
    c[3] = q[3];

    {
        const struct dj_internal_math_ld_pair n =
            d_internal_quat_ld_dot(q, q, 4u);
        struct dj_internal_math_ld_pair num[4] = { { 0.0L, 0.0L },
                                                   { 0.0L, 0.0L },
                                                   { 0.0L, 0.0L },
                                                   { 0.0L, 0.0L } };

        // the numerator in its order
        if (_right)
        {
            d_internal_quat_ld_hamilton(num, p, c);
        }
        else
        {
            d_internal_quat_ld_hamilton(num, c, p);
        }

        // each part over the squared norm, back to its scale
        for (i = 0u; i < 4u; ++i)
        {
            _out[i] = d_internal_quat_ld_scale_pair(
                d_internal_quat_ld_over(num[i], n), ep - eq);
        }
    }

    return;
}


// 1.3    The algebra in double
//------------------------------------------------------------------------------
// The product, the inverse and the quotients again, in double's pairs, for
// float's and double's where double is evaluated in its own type, as SSE
// evaluates it, and long double is not: a float's products exact in double,
// a double's by Dekker's (cplx.h's), each sum Dot2's -- as accurate as
// twice double's precision, some ten times faster than the x87's long
// double. Where double is evaluated wider, as on a 32-bit x87, the families
// take section 1.2's kernels.
#if !D_MATH_DBL_EVAL_WIDE

// d_internal_quat_d2_is_zero
//   whether each of _v's _n parts is 0, of either sign.
D_MATH_FN_KERNEL bool
d_internal_quat_d2_is_zero(
    const double* _v,
    size_t        _n
)
{
    size_t i = 0u;

    // a part that is not 0
    for (i = 0u; i < _n; ++i)
    {
        if (_v[i] != 0.0)
        {
            return false;
        }
    }

    return true;
}

// d_internal_quat_d2_safe
//   whether each of _v's _n parts is 0 or lies from 2^(EMIN/2 + P) to 2^(EMAX/2
// - P) in double's terms, where Dekker's products neither overflow nor lose
// their low parts (cplx.h's d_internal_cplx_d_safe).
D_MATH_FN_KERNEL bool
d_internal_quat_d2_safe(
    const double* _v,
    size_t        _n
)
{
    const double low  = d_internal_math_d_pow2((DBL_MIN_EXP / 2) +
                                               DBL_MANT_DIG);
    const double high = d_internal_math_d_pow2((DBL_MAX_EXP / 2) -
                                               DBL_MANT_DIG);
    size_t       i    = 0u;

    // a part outside the range
    for (i = 0u; i < _n; ++i)
    {
        const double a = d_internal_cplx_d_abs(_v[i]);

        if ( (a != 0.0) &&
             ( (a < low) ||
               (a > high) ) )
        {
            return false;
        }
    }

    return true;
}

// d_internal_quat_d2_scaled
//   _out = _v's _n parts times 2^-e, e the binade of the largest, which is
// then in [1, 2); e itself returned. _v finite and not 0.
D_MATH_FN_KERNEL int
d_internal_quat_d2_scaled(
    double*       _out,
    const double* _v,
    size_t        _n
)
{
    double largest = 0.0;
    int    e       = 0;
    size_t i       = 0u;

    // the largest magnitude
    for (i = 0u; i < _n; ++i)
    {
        const double a = d_internal_cplx_d_abs(_v[i]);

        if (a > largest)
        {
            largest = a;
        }
    }

    e = d_internal_cplx_d_exponent(largest);

    // each part by the power
    for (i = 0u; i < _n; ++i)
    {
        _out[i] = d_internal_cplx_d_scale(_v[i], -e);
    }

    return e;
}

// d_internal_quat_d2_scale_pair
//   _p 2^_k, each part scaled.
D_MATH_FN_KERNEL struct dj_internal_math_d_pair
d_internal_quat_d2_scale_pair(
    struct dj_internal_math_d_pair _p,
    int                            _k
)
{
    struct dj_internal_math_d_pair result = { 0.0, 0.0 };

    result.high = d_internal_cplx_d_scale(_p.high, _k);
    result.low  = d_internal_cplx_d_scale(_p.low, _k);

    return result;
}

// d_internal_quat_d2_dot
//   d_internal_quat_ld_dot in double: the sum of the _n products as a pair,
// each exact by Dekker's (cplx.h's d_internal_cplx_d_product), Dot2's sum;
// an exact zero +0, or the zero products' own sum.
D_MATH_FN_KERNEL struct dj_internal_math_d_pair
d_internal_quat_d2_dot(
    const double* _a,
    const double* _b,
    size_t        _n
)
{
    struct dj_internal_math_d_pair result = { 0.0, 0.0 };
    double                         high   = 0.0;
    double                         rest   = 0.0;
    double                         zeros  = 0.0;
    bool                           zero   = true;
    size_t                         i      = 0u;

    // each product exactly, its high part into the running sum
    for (i = 0u; i < _n; ++i)
    {
        const struct dj_internal_math_d_pair p =
            d_internal_cplx_d_product(_a[i], _b[i]);
        const struct dj_internal_math_d_pair s =
            d_internal_cplx_d_two_sum(high, p.high);
        const double term = _a[i] * _b[i];

        high  = s.high;
        rest  = rest + (s.low + p.low);
        zero  = ( (zero) &&
                  (term == 0.0) );
        zeros = (i == 0u) ? term : (zeros + term);
    }

    result = d_internal_cplx_d_two_sum(high, rest);

    // an exact zero: +0, or the zero products' own sum
    if (result.high == 0.0)
    {
        result.high = zero ? zeros : 0.0;
        result.low  = 0.0;
    }

    return result;
}

// d_internal_quat_d2_hamilton
//   d_internal_quat_ld_hamilton in double.
D_MATH_FN_KERNEL void
d_internal_quat_d2_hamilton(
    struct dj_internal_math_d_pair* _out,
    const double*                   _a,
    const double*                   _b
)
{
    const double x_a[4] = { _a[3],  _a[0],  _a[1], -_a[2] };
    const double x_b[4] = { _b[0],  _b[3],  _b[2],  _b[1] };
    const double y_a[4] = { _a[3], -_a[0],  _a[1],  _a[2] };
    const double y_b[4] = { _b[1],  _b[2],  _b[3],  _b[0] };
    const double z_a[4] = { _a[3],  _a[0], -_a[1],  _a[2] };
    const double z_b[4] = { _b[2],  _b[1],  _b[0],  _b[3] };
    const double w_a[4] = { _a[3], -_a[0], -_a[1], -_a[2] };
    const double w_b[4] = { _b[3],  _b[0],  _b[1],  _b[2] };

    _out[0] = d_internal_quat_d2_dot(x_a, x_b, 4u);
    _out[1] = d_internal_quat_d2_dot(y_a, y_b, 4u);
    _out[2] = d_internal_quat_d2_dot(z_a, z_b, 4u);
    _out[3] = d_internal_quat_d2_dot(w_a, w_b, 4u);

    return;
}

// d_internal_quat_d2_multiply
//   _out = _a _b, both finite (d_internal_quat_d2_hamilton), each scaled to
// a largest part in [1, 2) where either leaves the safe range, the parts
// scaled back.
D_MATH_FN_KERNEL void
d_internal_quat_d2_multiply(
    struct dj_internal_math_d_pair* _out,
    const double*                   _a,
    const double*                   _b
)
{
    // safe, or a zero operand: the parts as they are
    if ( ( (d_internal_quat_d2_safe(_a, 4u)) &&
           (d_internal_quat_d2_safe(_b, 4u)) ) ||
         (d_internal_quat_d2_is_zero(_a, 4u)) ||
         (d_internal_quat_d2_is_zero(_b, 4u)) )
    {
        d_internal_quat_d2_hamilton(_out, _a, _b);

        return;
    }

    {
        double    a[4] = { 0.0, 0.0, 0.0, 0.0 };
        double    b[4] = { 0.0, 0.0, 0.0, 0.0 };
        const int ea   = d_internal_quat_d2_scaled(a, _a, 4u);
        const int eb   = d_internal_quat_d2_scaled(b, _b, 4u);
        size_t    i    = 0u;

        d_internal_quat_d2_hamilton(_out, a, b);

        // each part back to its scale
        for (i = 0u; i < 4u; ++i)
        {
            _out[i] = d_internal_quat_d2_scale_pair(_out[i], ea + eb);
        }
    }

    return;
}

// d_internal_quat_d2_over
//   d_internal_quat_ld_over in double: (_u.high + _u.low) / (_n.high +
// _n.low) as a pair, _n.high in [1, 32).
D_MATH_FN_KERNEL struct dj_internal_math_d_pair
d_internal_quat_d2_over(
    struct dj_internal_math_d_pair _u,
    struct dj_internal_math_d_pair _n
)
{
    const double                   q0     = _u.high / _n.high;
    const double                   least  =
        DBL_MIN * d_internal_math_d_pow2(DBL_MANT_DIG + 4);
    struct dj_internal_math_d_pair result = { 0.0, 0.0 };

    result.high = q0;

    // a zero: itself, its sign kept
    if (q0 == 0.0)
    {
        return result;
    }

    // too near the subnormals for Dekker's product
    if (d_internal_cplx_d_abs(q0) < least)
    {
        result.high = q0 - (q0 * (_n.low / _n.high));

        return result;
    }

    {
        const struct dj_internal_math_d_pair t =
            d_internal_cplx_d_product(q0, _n.high);
        // u - q n: u's high part less t's is exact, the two within an ulp
        const double head = (_u.high - t.high) - t.low;
        const double r    = (head + _u.low) - (q0 * _n.low);

        result = d_internal_cplx_d_two_sum(q0, r / _n.high);
    }

    return result;
}

// d_internal_quat_d2_divide
//   d_internal_quat_ld_divide in double, _q's inverse where _p is NULL: _p
// / _q where _right, q^-1 p otherwise, both finite and q not 0, each scaled
// to a largest part in [1, 2), the parts over |q|^2's pair, scaled back.
D_MATH_FN_KERNEL void
d_internal_quat_d2_divide(
    struct dj_internal_math_d_pair* _out,
    const double*                   _p,
    const double*                   _q,
    bool                            _right
)
{
    const double one[4] = { 0.0, 0.0, 0.0, 1.0 };
    double       p[4]   = { 0.0, 0.0, 0.0, 0.0 };
    double       q[4]   = { 0.0, 0.0, 0.0, 0.0 };
    double       c[4]   = { 0.0, 0.0, 0.0, 0.0 };
    const double* num_p = (_p == NULL) ? one : _p;
    const int    eq     = d_internal_quat_d2_scaled(q, _q, 4u);
    const int    ep     = (d_internal_quat_d2_is_zero(num_p, 4u)) ?
                              0 : d_internal_quat_d2_scaled(p, num_p, 4u);
    size_t       i      = 0u;

    c[0] = -q[0];
    c[1] = -q[1];
    c[2] = -q[2];
    c[3] = q[3];

    {
        const struct dj_internal_math_d_pair n =
            d_internal_quat_d2_dot(q, q, 4u);
        struct dj_internal_math_d_pair num[4] = { { 0.0, 0.0 },
                                                  { 0.0, 0.0 },
                                                  { 0.0, 0.0 },
                                                  { 0.0, 0.0 } };

        // the numerator in its order: the inverse's is q* itself
        if (_p == NULL)
        {
            for (i = 0u; i < 4u; ++i)
            {
                num[i].high = c[i];
            }
        }
        else if (_right)
        {
            d_internal_quat_d2_hamilton(num, p, c);
        }
        else
        {
            d_internal_quat_d2_hamilton(num, c, p);
        }

        // each part over the squared norm, back to its scale
        for (i = 0u; i < 4u; ++i)
        {
            _out[i] = d_internal_quat_d2_scale_pair(
                d_internal_quat_d2_over(num[i], n), ep - eq);
        }
    }

    return;
}

#endif  // !D_MATH_DBL_EVAL_WIDE


// 1.4    Rotation
//------------------------------------------------------------------------------
// q v q^-1 and its matrix, for any q not 0: q's norm cancels, so a q a few
// steps from unit length names the rotation of q / |q|, as glTF's, Eigen's
// and GLM's unit quaternions name theirs.

// d_internal_quat_ld_rotate
//   _out = q v q^-1, the rotation _q names applied to the 3-vector _v, both
// finite and not 0: ((w^2 - |u|^2) v + 2 (u . v) u + 2 w (u x v)) / |q|^2, q
// = (u, w). q and v are scaled to largest parts in [1, 2); the squares and
// products exact and their sums pairs, each part's three terms a pair, over
// |q|^2's pair (d_internal_quat_ld_over), v's scale restored.
D_MATH_FN_KERNEL void
d_internal_quat_ld_rotate(
    struct dj_internal_math_ld_pair* _out,
    const long double*               _q,
    const long double*               _v
)
{
    const int   eq   = d_internal_quat_ld_exponent(_q, 4u);
    const int   ev   = d_internal_quat_ld_exponent(_v, 3u);
    long double q[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    long double v[3] = { 0.0L, 0.0L, 0.0L };
    size_t      i    = 0u;

    d_internal_quat_ld_scale_all(q, _q, 4u, -eq);
    d_internal_quat_ld_scale_all(v, _v, 3u, -ev);

    {
        // w^2 - |u|^2, |q|^2 and u . v
        const long double sq_a[4] = { q[3], -q[0], -q[1], -q[2] };
        const long double sq_b[4] = { q[3],  q[0],  q[1],  q[2] };
        const long double c0_a[2] = { q[1], -q[2] };
        const long double c0_b[2] = { v[2],  v[1] };
        const long double c1_a[2] = { q[2], -q[0] };
        const long double c1_b[2] = { v[0],  v[2] };
        const long double c2_a[2] = { q[0], -q[1] };
        const long double c2_b[2] = { v[1],   v[0] };
        const long double q_v[3]  = { q[0],   q[1],  q[2] };
        const struct dj_internal_math_ld_pair a =
            d_internal_quat_ld_dot(sq_a, sq_b, 4u);
        const struct dj_internal_math_ld_pair n =
            d_internal_quat_ld_dot(q, q, 4u);
        const struct dj_internal_math_ld_pair b =
            d_internal_quat_ld_dot(q_v, v, 3u);
        // u x v
        const struct dj_internal_math_ld_pair c[3] = {
            d_internal_quat_ld_dot(c0_a, c0_b, 2u),
            d_internal_quat_ld_dot(c1_a, c1_b, 2u),
            d_internal_quat_ld_dot(c2_a, c2_b, 2u) };

        // each part's three terms, over |q|^2
        for (i = 0u; i < 3u; ++i)
        {
            const struct dj_internal_math_ld_pair t1 =
                d_internal_math_ld_pair_product(
                    a, d_internal_math_ld_pair_of(v[i], 0.0L));
            const struct dj_internal_math_ld_pair t2 =
                d_internal_math_ld_pair_product(
                    b, d_internal_math_ld_pair_of(2.0L * q[i], 0.0L));
            const struct dj_internal_math_ld_pair t3 =
                d_internal_math_ld_pair_product(
                    c[i], d_internal_math_ld_pair_of(2.0L * q[3], 0.0L));
            const struct dj_internal_math_ld_pair s =
                d_internal_math_ld_pair_sum(
                    d_internal_math_ld_pair_sum(t1, t2), t3);

            _out[i] = d_internal_quat_ld_scale_pair(
                d_internal_quat_ld_over(s, n), ev);
        }
    }

    return;
}

// d_internal_quat_ld_to_matrix3
//   _out (3 x 3, by rows) = the rotation matrix of _q, finite and not 0: [w^2 +
// x^2 - y^2 - z^2, 2 (x y - w z), 2 (x z + w y); 2 (x y + w z), w^2 - x^2 +
// y^2 - z^2, 2 (y z - w x); 2 (x z - w y), 2 (y z + w x), w^2 - x^2 - y^2 +
// z^2] / |q|^2, each numerator a pair of exact products, over |q|^2's pair.
D_MATH_FN_KERNEL void
d_internal_quat_ld_to_matrix3(
    struct dj_internal_math_ld_pair* _out,
    const long double*               _q
)
{
    const int   e    = d_internal_quat_ld_exponent(_q, 4u);
    long double q[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    size_t      i    = 0u;

    d_internal_quat_ld_scale_all(q, _q, 4u, -e);

    {
        const long double x  = q[0];
        const long double y  = q[1];
        const long double z  = q[2];
        const long double w  = q[3];
        const long double d0[4]   = { w,  x, -y, -z };
        const long double d1[4]   = { w, -x,  y, -z };
        const long double d2[4]   = { w, -x, -y,  z };
        const long double wxyz[4] = { w,  x,  y,  z };
        const long double xw_a[2] = { 2.0L * x, -2.0L * w };
        const long double xw_p[2] = { 2.0L * x,  2.0L * w };
        const long double yw_a[2] = { 2.0L * y, -2.0L * w };
        const long double yw_p[2] = { 2.0L * y,  2.0L * w };
        const long double yz[2]   = { y, z };
        const long double zy[2]   = { z, y };
        const long double zx[2]   = { z, x };
        const struct dj_internal_math_ld_pair n =
            d_internal_quat_ld_dot(q, q, 4u);
        const struct dj_internal_math_ld_pair num[9] = {
            d_internal_quat_ld_dot(d0, wxyz, 4u),
            d_internal_quat_ld_dot(xw_a, yz, 2u),
            d_internal_quat_ld_dot(xw_p, zy, 2u),
            d_internal_quat_ld_dot(xw_p, yz, 2u),
            d_internal_quat_ld_dot(d1, wxyz, 4u),
            d_internal_quat_ld_dot(yw_a, zx, 2u),
            d_internal_quat_ld_dot(xw_a, zy, 2u),
            d_internal_quat_ld_dot(yw_p, zx, 2u),
            d_internal_quat_ld_dot(d2, wxyz, 4u) };

        // each entry over |q|^2
        for (i = 0u; i < 9u; ++i)
        {
            _out[i] = d_internal_quat_ld_over(num[i], n);
        }
    }

    return;
}
"""


# the kernels that take a square root and nothing else: once with the core's
# _c root, a constant expression, once with the C library's; a correctly
# rounded root either way, so the two give the same bits.
QUAT_KERNELS_ROOT = r"""

// 1.@SUB@    @TITLE@
//------------------------------------------------------------------------------
// @PROSE@

// d_internal_quat_ld_root@KS@
//   the square root of the pair _s, _s.high normal and positive, as a pair:
// the root r of the high part, rounded once, and (s - r^2) / 2r, r^2 exact.
@KQ@ struct dj_internal_math_ld_pair
d_internal_quat_ld_root@KS@(
    struct dj_internal_math_ld_pair _s
)
{
    const long double                     r   = @SQRT@(_s.high);
    const struct dj_internal_math_ld_pair t   =
        d_internal_math_ld_product_exact(r, r);
    // s - r^2: s's high part less t's is exact, the two within an ulp
    const long double                    rest =
        ((_s.high - t.high) - t.low) + _s.low;

    return d_internal_math_ld_two_sum(r, rest / (2.0L * r));
}

// d_internal_quat_ld_norm@KS@
//   |_v|, _n parts, at most 4, as a pair: +inf where a part is infinite, a
// NaN part a NaN, +0 for 0; otherwise v scaled to a largest part in [1, 2),
// the sum of its exact squares as a pair (d_internal_quat_ld_dot), its root
// as a pair, scaled back -- the norm to some 2^-2P of it, rounded once more
// where it falls into the subnormals.
@KQ@ struct dj_internal_math_ld_pair
d_internal_quat_ld_norm@KS@(
    const long double* _v,
    size_t             _n
)
{
    struct dj_internal_math_ld_pair result = { 0.0L, 0.0L };

    // an infinite part: an infinite norm, a NaN beside it or not
    if (d_internal_quat_ld_has_inf(_v, _n))
    {
        result.high = d_internal_math_ld_infinity();

        return result;
    }

    // a NaN part
    if (d_internal_quat_ld_has_nan(_v, _n))
    {
        result.high = d_internal_math_ld_nan();

        return result;
    }

    // a zero vector: +0
    if (d_internal_quat_ld_is_zero(_v, _n))
    {
        return result;
    }

    {
        const int   e    = d_internal_quat_ld_exponent(_v, _n);
        long double v[4] = { 0.0L, 0.0L, 0.0L, 0.0L };

        d_internal_quat_ld_scale_all(v, _v, _n, -e);
        result = d_internal_quat_ld_scale_pair(
            d_internal_quat_ld_root@KS@(d_internal_quat_ld_dot(v, v, _n)), e);
    }

    return result;
}

// d_internal_quat_ld_normalize@KS@
//   _out = _q / |q|, each part a pair: q scaled to a largest part in [1, 2),
// |q| the root of its exact squares' sum as a pair, each part over it
// (d_internal_quat_ld_over). A zero q is left as it is; a NaN part makes
// each a NaN; an infinite part, none a NaN, the direction of the infinities
// (d_internal_quat_ld_direction), the limit of q / |q|.
@KQ@ void
d_internal_quat_ld_normalize@KS@(
    struct dj_internal_math_ld_pair* _out,
    const long double*               _q
)
{
    long double q[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    size_t      i    = 0u;

    // a NaN part: no direction
    if (d_internal_quat_ld_has_nan(_q, 4u))
    {
        d_internal_quat_ld_fill_nan(_out, 4u);

        return;
    }

    // an infinite part: the direction of the infinities
    if (d_internal_quat_ld_has_inf(_q, 4u))
    {
        d_internal_quat_ld_direction(q, _q, 4u);

        for (i = 0u; i < 4u; ++i)
        {
            _out[i] = d_internal_math_ld_pair_of(q[i], 0.0L);
        }

        return;
    }

    // zero: left as it is, signs of zero included
    if (d_internal_quat_ld_is_zero(_q, 4u))
    {
        for (i = 0u; i < 4u; ++i)
        {
            _out[i] = d_internal_math_ld_pair_of(_q[i], 0.0L);
        }

        return;
    }

    d_internal_quat_ld_scale_all(q, _q, 4u,
                                 -d_internal_quat_ld_exponent(_q, 4u));

    {
        const struct dj_internal_math_ld_pair n =
            d_internal_quat_ld_root@KS@(d_internal_quat_ld_dot(q, q, 4u));

        // each part over the norm
        for (i = 0u; i < 4u; ++i)
        {
            _out[i] = d_internal_quat_ld_over(
                d_internal_math_ld_pair_of(q[i], 0.0L), n);
        }
    }

    return;
}

// d_internal_quat_ld_from_matrix3@KS@
//   _out = the unit quaternion of the rotation matrix _m (3 x 3, by rows), w
// >= 0, by Shepperd's method: of 4w^2 = 1 + m00 + m11 + m22, 4x^2 = 1 + m00 -
// m11 - m22, 4y^2 = 1 - m00 + m11 - m22 and 4z^2 = 1 - m00 - m11 + m22, each
// summed as a pair, the largest (w first among equals, then x, y and z) --
// at least 1, as the four sum to 4 -- gives its part, its root over 2, and
// the others are the off-diagonal sums and differences over 4 times it; then
// all four negated where w is negative, so that q is the one of q and -q with
// w >= 0. A matrix a few steps from orthogonal gives a quaternion as many
// from unit length. A NaN or infinite entry: a NaN in each part.
@KQ@ void
d_internal_quat_ld_from_matrix3@KS@(
    struct dj_internal_math_ld_pair* _out,
    const long double*               _m
)
{
    const long double one[4] = { 1.0L, 1.0L, 1.0L, 1.0L };
    const long double cx[4]  = { 1.0L,  _m[0], -_m[4], -_m[8] };
    const long double cy[4]  = { 1.0L, -_m[0],  _m[4], -_m[8] };
    const long double cz[4]  = { 1.0L, -_m[0], -_m[4],  _m[8] };
    const long double cw[4]  = { 1.0L,  _m[0],  _m[4],  _m[8] };
    size_t            k      = 3u;
    size_t            i      = 0u;

    // a NaN or an infinity: no rotation
    if (!d_internal_quat_ld_finite(_m, 9u))
    {
        d_internal_quat_ld_fill_nan(_out, 4u);

        return;
    }

    {
        const struct dj_internal_math_ld_pair c[4] = {
            d_internal_quat_ld_dot(cx, one, 4u),
            d_internal_quat_ld_dot(cy, one, 4u),
            d_internal_quat_ld_dot(cz, one, 4u),
            d_internal_quat_ld_dot(cw, one, 4u) };

        // the largest candidate, w first among equals, then x, y, z
        for (i = 0u; i < 3u; ++i)
        {
            if (c[i].high > c[k].high)
            {
                k = i;
            }
        }

        {
            const struct dj_internal_math_ld_pair s =
                d_internal_quat_ld_root@KS@(c[k]);
            // 4 times the part: twice the root
            const struct dj_internal_math_ld_pair den =
                d_internal_math_ld_pair_of(2.0L * s.high, 2.0L * s.low);
            // the other three parts' numerators, in x, y, z, w order
            const long double m01 = _m[1];
            const long double m02 = _m[2];
            const long double m10 = _m[3];
            const long double m12 = _m[5];
            const long double m20 = _m[6];
            const long double m21 = _m[7];
            struct dj_internal_math_ld_pair num[4] = { { 0.0L, 0.0L },
                                                       { 0.0L, 0.0L },
                                                       { 0.0L, 0.0L },
                                                       { 0.0L, 0.0L } };

            // the off-diagonal sums and differences k's part divides
            if (k == 0u)
            {
                num[1] = d_internal_math_ld_two_sum(m01, m10);
                num[2] = d_internal_math_ld_two_sum(m02, m20);
                num[3] = d_internal_math_ld_two_sum(m21, -m12);
            }
            else if (k == 1u)
            {
                num[0] = d_internal_math_ld_two_sum(m01, m10);
                num[2] = d_internal_math_ld_two_sum(m12, m21);
                num[3] = d_internal_math_ld_two_sum(m02, -m20);
            }
            else if (k == 2u)
            {
                num[0] = d_internal_math_ld_two_sum(m02, m20);
                num[1] = d_internal_math_ld_two_sum(m12, m21);
                num[3] = d_internal_math_ld_two_sum(m10, -m01);
            }
            else
            {
                num[0] = d_internal_math_ld_two_sum(m21, -m12);
                num[1] = d_internal_math_ld_two_sum(m02, -m20);
                num[2] = d_internal_math_ld_two_sum(m10, -m01);
            }

            // k's part the root over 2, the others over 4 times it
            for (i = 0u; i < 4u; ++i)
            {
                _out[i] = (i == k) ?
                    d_internal_math_ld_pair_of(s.high / 2.0L, s.low / 2.0L) :
                    d_internal_quat_ld_over(num[i], den);
            }
        }
    }

    // q or -q: the one with w >= 0
    if (_out[3].high < 0.0L)
    {
        for (i = 0u; i < 4u; ++i)
        {
            _out[i].high = -_out[i].high;
            _out[i].low  = -_out[i].low;
        }
    }

    return;
}

// d_internal_quat_ld_from_two_vectors@KS@
//   _out = the unit quaternion of the shortest rotation turning the direction
// of _a into _b's, both 3-vectors: (a x b, |a| |b| + a . b) normalized, the
// sum's cancellation, where a . b < 0, avoided as |a x b|^2 / (|a| |b| - a .
// b), each term a pair of a and b scaled to largest parts in [1, 2). Where
// the two are exactly opposed, the half turn about the axis a x e_k, e_k the
// unit vector of a's smallest part (the first among equals). A zero vector:
// no rotation, the identity; a NaN or an infinite part: a NaN in each part.
@KQ@ void
d_internal_quat_ld_from_two_vectors@KS@(
    struct dj_internal_math_ld_pair* _out,
    const long double*               _a,
    const long double*               _b
)
{
    long double a[3] = { 0.0L, 0.0L, 0.0L };
    long double b[3] = { 0.0L, 0.0L, 0.0L };
    long double q[4] = { 0.0L, 0.0L, 0.0L, 0.0L };

    // a NaN or an infinity: no rotation
    if ( (!d_internal_quat_ld_finite(_a, 3u)) ||
         (!d_internal_quat_ld_finite(_b, 3u)) )
    {
        d_internal_quat_ld_fill_nan(_out, 4u);

        return;
    }

    // a zero vector names no direction: the identity
    if ( (d_internal_quat_ld_is_zero(_a, 3u)) ||
         (d_internal_quat_ld_is_zero(_b, 3u)) )
    {
        _out[0] = d_internal_math_ld_pair_of(0.0L, 0.0L);
        _out[1] = d_internal_math_ld_pair_of(0.0L, 0.0L);
        _out[2] = d_internal_math_ld_pair_of(0.0L, 0.0L);
        _out[3] = d_internal_math_ld_pair_of(1.0L, 0.0L);

        return;
    }

    d_internal_quat_ld_scale_all(a, _a, 3u,
                                 -d_internal_quat_ld_exponent(_a, 3u));
    d_internal_quat_ld_scale_all(b, _b, 3u,
                                 -d_internal_quat_ld_exponent(_b, 3u));

    {
        const long double c0_a[2] = { a[1], -a[2] };
        const long double c0_b[2] = { b[2],  b[1] };
        const long double c1_a[2] = { a[2], -a[0] };
        const long double c1_b[2] = { b[0],  b[2] };
        const long double c2_a[2] = { a[0], -a[1] };
        const long double c2_b[2] = { b[1],   b[0] };
        const struct dj_internal_math_ld_pair c[3] = {
            d_internal_quat_ld_dot(c0_a, c0_b, 2u),
            d_internal_quat_ld_dot(c1_a, c1_b, 2u),
            d_internal_quat_ld_dot(c2_a, c2_b, 2u) };
        const struct dj_internal_math_ld_pair d =
            d_internal_quat_ld_dot(a, b, 3u);
        // |a| |b|, the root of the product of the squared norms
        const struct dj_internal_math_ld_pair m =
            d_internal_quat_ld_root@KS@(d_internal_math_ld_pair_product(
                d_internal_quat_ld_dot(a, a, 3u),
                d_internal_quat_ld_dot(b, b, 3u)));
        struct dj_internal_math_ld_pair w = { 0.0L, 0.0L };

        // |a| |b| + a . b, as a sum where the two agree in sign
        if (d.high >= 0.0L)
        {
            w = d_internal_math_ld_pair_sum(m, d);
        }
        else
        {
            const struct dj_internal_math_ld_pair cc =
                d_internal_math_ld_pair_sum(
                    d_internal_math_ld_pair_sum(
                        d_internal_math_ld_pair_product(c[0], c[0]),
                        d_internal_math_ld_pair_product(c[1], c[1])),
                    d_internal_math_ld_pair_product(c[2], c[2]));
            const struct dj_internal_math_ld_pair den =
                d_internal_math_ld_pair_sum(
                    m, d_internal_math_ld_pair_of(-d.high, -d.low));

            // exactly opposed: |a x b| is 0
            if (cc.high != 0.0L)
            {
                w = d_internal_quat_ld_over(cc, den);
            }
        }

        q[0] = c[0].high;
        q[1] = c[1].high;
        q[2] = c[2].high;
        q[3] = w.high;
    }

    // exactly opposed: a half turn about an axis perpendicular to a
    if (d_internal_quat_ld_is_zero(q, 4u))
    {
        const long double x = d_internal_cplx_ld_abs(a[0]);
        const long double y = d_internal_cplx_ld_abs(a[1]);
        const long double z = d_internal_cplx_ld_abs(a[2]);

        // a x e_k, e_k along a's smallest part
        if ( (x <= y) &&
             (x <= z) )
        {
            q[1] = a[2];
            q[2] = -a[1];
        }
        else if (y <= z)
        {
            q[0] = -a[2];
            q[2] = a[0];
        }
        else
        {
            q[0] = a[1];
            q[1] = -a[0];
        }
    }

    d_internal_quat_ld_normalize@KS@(_out, q);

    return;
}

// d_internal_quat_ld_nlerp@KS@
//   _out = the normalized linear interpolation from _a (t = 0) to _b (t = 1),
// along the shorter arc -- b negated where a . b < 0, as q and -q are one
// rotation: ((1 - t) a + t b) / |...|, the sum's parts as pairs of a and b
// scaled alike to a largest part in [1, 2) (d_internal_quat_ld_lerp), then
// normalized. A NaN or an infinite part, of t too: a NaN in each part.
@KQ@ void
d_internal_quat_ld_nlerp@KS@(
    struct dj_internal_math_ld_pair* _out,
    const long double*               _a,
    const long double*               _b,
    long double                      _t
)
{
    long double a[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    long double b[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    long double l[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    size_t      i    = 0u;

    // a NaN or an infinity: nothing to interpolate
    if ( (!d_internal_quat_ld_finite(_a, 4u)) ||
         (!d_internal_quat_ld_finite(_b, 4u)) ||
         (!d_internal_quat_ld_finite(&_t, 1u)) )
    {
        d_internal_quat_ld_fill_nan(_out, 4u);

        return;
    }

    {
        const long double flip = (d_internal_quat_ld_dot_sign(_a, _b, 4u) <
                                  0.0L) ? -1.0L : 1.0L;
        const long double both[8] = { _a[0], _a[1], _a[2], _a[3],
                                      _b[0], _b[1], _b[2], _b[3] };
        const int         e       =
            (d_internal_quat_ld_is_zero(both, 8u)) ?
                0 : d_internal_quat_ld_exponent(both, 8u);

        d_internal_quat_ld_scale_all(a, _a, 4u, -e);
        d_internal_quat_ld_scale_all(b, _b, 4u, -e);

        // each part of the line, the shorter arc's end
        for (i = 0u; i < 4u; ++i)
        {
            l[i] = d_internal_quat_ld_lerp(a[i], flip * b[i], _t).high;
        }
    }

    d_internal_quat_ld_normalize@KS@(_out, l);

    return;
}
"""


QUAT_KERNELS_FUNCTIONS = r"""

// 1.7    The _c forms' functions
//------------------------------------------------------------------------------
// The exponential, logarithm, square root and real powers of a quaternion
// are the complex ones in its plane (S275): q = w + v, r = |v|, u = v / r,
// f(q) = Re f(w + r i) + u Im f(w + r i) -- for a vector part along im alone
// the complex function itself (each family's section 6). r is carried as a
// pair, and f corrected to first order for its low part, as the plane's
// point is w + (r_hi + r_lo) i; the logarithm and the root take q scaled by
// a power of 2 where it leaves the safe range, so that nothing overflows or
// falls into the subnormals. Each is cplx.h's kernel, a constant expression
// from C++14.

// dj_internal_quat_ld_plane
//   a quaternion's plane: the point w + r i, r = |v| as a pair, v's direction
// u, and the power, 2^-scale, that w and r were taken at.
struct dj_internal_quat_ld_plane
{
    long double                     w;
    struct dj_internal_math_ld_pair r;
    long double                     u[3];
    int                             scale;
};

// d_internal_quat_ld_plane_c
//   _q's plane, its vector part not 0 and no part a NaN: where _scale is 1
// and q leaves the safe range (d_internal_quat_ld_safe), w and r taken at
// 2^-k, k the binade of q's largest part, and where it is 2 at the even one
// of k and k + 1; otherwise at 1. u is v over |v|, both as pairs of v
// scaled to a largest part in [1, 2), each part rounded once. A part of v
// infinite: r +inf and u the direction of v's infinities
// (d_internal_quat_ld_direction), q taken at 1.
D_MATH_FN_KERNEL struct dj_internal_quat_ld_plane
d_internal_quat_ld_plane_c(
    const long double* _q,
    int                _scale
)
{
    struct dj_internal_quat_ld_plane result = { 0.0L,
                                                { 0.0L, 0.0L },
                                                { 0.0L, 0.0L, 0.0L },
                                                0 };
    long double                      v[3]   = { _q[0], _q[1], _q[2] };
    size_t                           i      = 0u;

    result.w = _q[3];

    // an infinite part of v: an infinite r along v's infinities
    if (d_internal_quat_ld_has_inf(v, 3u))
    {
        result.r.high = d_internal_math_ld_infinity();
        d_internal_quat_ld_direction(result.u, v, 3u);

        return result;
    }

    // past the safe range: q's largest part into [1, 2), or [1/2, 2) at an
    // even power -- only there, as log |q| near 0 would cancel against the
    // scale's k ln 2
    if ( (_scale > 0) &&
         (d_internal_quat_ld_finite(_q, 4u)) &&
         (!d_internal_quat_ld_safe(_q, 4u)) )
    {
        result.scale = d_internal_quat_ld_exponent(_q, 4u);

        if ( (_scale == 2) &&
             ((result.scale % 2) != 0) )
        {
            result.scale = result.scale + 1;
        }

        result.w = d_internal_cplx_ld_scale(_q[3], -result.scale);
    }

    {
        const int   e    = d_internal_quat_ld_exponent(v, 3u);
        long double s[3] = { 0.0L, 0.0L, 0.0L };

        d_internal_quat_ld_scale_all(s, v, 3u, -e);

        {
            const struct dj_internal_math_ld_pair root =
                d_internal_quat_ld_root_c(d_internal_quat_ld_dot(s, s, 3u));

            // u = v / |v|
            for (i = 0u; i < 3u; ++i)
            {
                result.u[i] = d_internal_quat_ld_over(
                    d_internal_math_ld_pair_of(s[i], 0.0L), root).high;
            }

            result.r = d_internal_quat_ld_scale_pair(root, e - result.scale);
        }
    }

    return result;
}

// d_internal_quat_ld_relative
//   (r_lo i) / z, z = _w + _r.high i: the plane point's low part relative to
// it, the first-order correction's factor, from w and r scaled alike to a
// larger one in [1, 2); 0 where r's low part is 0 or z is not finite.
D_MATH_FN_KERNEL struct dj_cplx_ld
d_internal_quat_ld_relative(
    long double                     _w,
    struct dj_internal_math_ld_pair _r
)
{
    // nothing to correct
    if ( (_r.low == 0.0L) ||
         (_w != _w) ||
         (d_internal_cplx_ld_is_inf(_w)) ||
         (d_internal_cplx_ld_is_inf(_r.high)) )
    {
        return d_internal_cplx_ld_make(0.0L, 0.0L);
    }

    {
        const long double big   = (d_internal_cplx_ld_abs(_w) > _r.high) ?
                                      d_internal_cplx_ld_abs(_w) : _r.high;
        const int         k     = d_internal_cplx_ld_exponent(big);
        const long double a     = d_internal_cplx_ld_scale(_r.high, -k);
        const long double b     = d_internal_cplx_ld_scale(_w, -k);
        const long double m     = (a * a) + (b * b);
        const long double ratio = _r.low / _r.high;
        // i ratio a / (b + a i) = ratio a (a + b i) / m
        const long double f     = (ratio * a) / m;

        return d_internal_cplx_ld_make(f * a, f * b);
    }
}

// d_internal_quat_ld_corrected
//   _f + _g _rho, a complex value plus its first-order correction: _f
// itself where either is not finite or _rho is 0.
D_MATH_FN_KERNEL struct dj_cplx_ld
d_internal_quat_ld_corrected(
    struct dj_cplx_ld _f,
    struct dj_cplx_ld _g,
    struct dj_cplx_ld _rho
)
{
    // nothing to add
    if ( ( (_rho.part[0] == 0.0L) &&
           (_rho.part[1] == 0.0L) ) ||
         (!d_internal_cplx_ld_finite(_f)) ||
         (!d_internal_cplx_ld_finite(_g)) )
    {
        return _f;
    }

    {
        const long double re = (_g.part[0] * _rho.part[0]) -
                               (_g.part[1] * _rho.part[1]);
        const long double im = (_g.part[0] * _rho.part[1]) +
                               (_g.part[1] * _rho.part[0]);

        return d_internal_cplx_ld_make(_f.part[0] + re, _f.part[1] + im);
    }
}

// d_internal_quat_ld_exp_c
//   _out = e^q = e^w (cos r + u sin r), _q's vector part not along im alone
// and no part a NaN: cplx.h's exponential at w + r_hi i, plus e^z r_lo i, its
// imaginary part along u (d_internal_quat_ld_along).
D_MATH_FN_KERNEL void
d_internal_quat_ld_exp_c(
    struct dj_internal_math_ld_pair* _out,
    const long double*               _q
)
{
    const struct dj_internal_quat_ld_plane p =
        d_internal_quat_ld_plane_c(_q, 0);
    const struct dj_cplx_ld f = d_internal_cplx_ld_exp_c(
        d_internal_cplx_ld_make(p.w, p.r.high));
    // d e^z = e^z dz, dz = r_lo i
    const struct dj_cplx_ld c = d_internal_quat_ld_corrected(
        f, f, d_internal_cplx_ld_make(0.0L, p.r.low));
    size_t                  i = 0u;

    // the imaginary part along u
    for (i = 0u; i < 3u; ++i)
    {
        _out[i] = d_internal_math_ld_pair_of(
            d_internal_quat_ld_along(p.u[i], c.part[1]), 0.0L);
    }

    _out[3] = d_internal_math_ld_pair_of(c.part[0], 0.0L);

    return;
}

// d_internal_quat_ld_log_c
//   _out = log q = log|q| + u atan2(r, w), _q's vector part not along im alone
// and no part a NaN: cplx.h's logarithm at q's plane taken at 2^-k, plus
// r_lo i / z, and k ln 2 added to the real part as a pair (power.h's
// logarithm of 2^k). Where w > 0 and r < w 2^-(P/2 + 2), atan2(r, w) / r is
// 1 / w to the last place, and each part of v over w is the vector part.
D_MATH_FN_KERNEL void
d_internal_quat_ld_log_c(
    struct dj_internal_math_ld_pair* _out,
    const long double*               _q
)
{
    const struct dj_internal_quat_ld_plane p =
        d_internal_quat_ld_plane_c(_q, 1);
    const struct dj_cplx_ld f = d_internal_cplx_ld_log_c(
        d_internal_cplx_ld_make(p.w, p.r.high));
    // d log z = dz / z
    const struct dj_cplx_ld c = d_internal_quat_ld_corrected(
        f, d_internal_cplx_ld_make(1.0L, 0.0L),
        d_internal_quat_ld_relative(p.w, p.r));
    const struct dj_internal_math_ld_pair k_ln2 = (p.scale == 0) ?
        d_internal_math_ld_pair_of(0.0L, 0.0L) :
        d_internal_math_ld_log_accurate_of(p.scale, 1.0L);
    const long double tiny =
        d_internal_math_ld_pow2(-((LDBL_MANT_DIG / 2) + 2));
    size_t            i    = 0u;

    // r far below w > 0: v / w
    if ( (p.w > 0.0L) &&
         (!d_internal_cplx_ld_is_inf(p.w)) &&
         (p.r.high < (p.w * tiny)) )
    {
        for (i = 0u; i < 3u; ++i)
        {
            _out[i] = d_internal_math_ld_pair_of(_q[i] / _q[3], 0.0L);
        }
    }
    else
    {
        for (i = 0u; i < 3u; ++i)
        {
            _out[i] = d_internal_math_ld_pair_of(
                d_internal_quat_ld_along(p.u[i], c.part[1]), 0.0L);
        }
    }

    // k ln 2 added where q was scaled, and so finite
    _out[3] = (p.scale == 0) ?
        d_internal_math_ld_pair_of(c.part[0], 0.0L) :
        d_internal_math_ld_pair_sum(
            d_internal_math_ld_pair_of(c.part[0], 0.0L), k_ln2);

    return;
}

// d_internal_quat_ld_sqrt_c
//   _out = sqrt q, the principal root, its real part >= 0: cplx.h's root at
// q's plane taken at 2^-2j, plus sqrt(z) (r_lo i / z) / 2, scaled back by
// 2^j; the vector part, where the real part is the larger, v / (2 Re), as
// 2 Re Im = r exactly, and otherwise Im along u. _q's vector part not along
// im alone and no part a NaN.
D_MATH_FN_KERNEL void
d_internal_quat_ld_sqrt_c(
    struct dj_internal_math_ld_pair* _out,
    const long double*               _q
)
{
    const struct dj_internal_quat_ld_plane p =
        d_internal_quat_ld_plane_c(_q, 2);
    const struct dj_cplx_ld f = d_internal_cplx_ld_sqrt_c(
        d_internal_cplx_ld_make(p.w, p.r.high));
    const struct dj_cplx_ld rho = d_internal_quat_ld_relative(p.w, p.r);
    // d sqrt(z) = sqrt(z) (dz / z) / 2
    const struct dj_cplx_ld c = d_internal_quat_ld_corrected(
        f, d_internal_cplx_ld_make(f.part[0] / 2.0L, f.part[1] / 2.0L), rho);
    const int         half  = p.scale / 2;
    const long double re    = d_internal_cplx_ld_scale(c.part[0], half);
    const long double im    = d_internal_cplx_ld_scale(c.part[1], half);
    size_t            i     = 0u;

    // the real part the larger, in a finite q: v / (2 Re)
    if ( (d_internal_quat_ld_finite(_q, 4u)) &&
         (c.part[0] >= c.part[1]) &&
         (re > 0.0L) )
    {
        for (i = 0u; i < 3u; ++i)
        {
            _out[i] = d_internal_math_ld_pair_of(_q[i] / (2.0L * re), 0.0L);
        }
    }
    else
    {
        for (i = 0u; i < 3u; ++i)
        {
            _out[i] = d_internal_math_ld_pair_of(
                d_internal_quat_ld_along(p.u[i], im), 0.0L);
        }
    }

    _out[3] = d_internal_math_ld_pair_of(re, 0.0L);

    return;
}

// d_internal_quat_ld_pow_c
//   _out = q^_p = exp(p log q), p real: cplx.h's power of q's plane point by
// p + 0i, plus p z^p (r_lo i / z), its imaginary part along u. _q's vector
// part not along im alone and no part a NaN; past |v| = LDBL_MAX, r is
// infinite and the value an infinite r's.
D_MATH_FN_KERNEL void
d_internal_quat_ld_pow_c(
    struct dj_internal_math_ld_pair* _out,
    const long double*               _q,
    long double                      _p
)
{
    const struct dj_internal_quat_ld_plane p =
        d_internal_quat_ld_plane_c(_q, 0);
    const struct dj_cplx_ld f = d_internal_cplx_ld_pow_c(
        d_internal_cplx_ld_make(p.w, p.r.high),
        d_internal_cplx_ld_make(_p, 0.0L));
    // d z^p = p z^p dz / z
    const struct dj_cplx_ld c = d_internal_quat_ld_corrected(
        f, d_internal_cplx_ld_make(_p * f.part[0], _p * f.part[1]),
        d_internal_quat_ld_relative(p.w, p.r));
    size_t                  i = 0u;

    // the imaginary part along u
    for (i = 0u; i < 3u; ++i)
    {
        _out[i] = d_internal_math_ld_pair_of(
            d_internal_quat_ld_along(p.u[i], c.part[1]), 0.0L);
    }

    _out[3] = d_internal_math_ld_pair_of(c.part[0], 0.0L);

    return;
}

// d_internal_quat_ld_slerp_c
//   _out = the spherical linear interpolation from _a (t = 0) to _b (t = 1),
// both unit quaternions, along the shorter arc -- b negated where a . b < 0:
// sin((1 - t) theta) / sin theta a + sin(t theta) / sin theta b, theta = 2
// atan2(|a - b|, |a + b|), the angle between them accurate near 0 as acos(a
// . b) is not; at theta = 0, (1 - t) a + t b (d_internal_quat_ld_lerp). Each
// part a pair of the two products. a at t = 0 and b at t = 1 exactly. A NaN
// or an infinite part, of t too: a NaN in each part.
D_MATH_FN_KERNEL void
d_internal_quat_ld_slerp_c(
    struct dj_internal_math_ld_pair* _out,
    const long double*               _a,
    const long double*               _b,
    long double                      _t
)
{
    long double b[4]    = { 0.0L, 0.0L, 0.0L, 0.0L };
    long double diff[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    long double sum[4]  = { 0.0L, 0.0L, 0.0L, 0.0L };
    size_t      i       = 0u;

    // a NaN or an infinity: nothing to interpolate
    if ( (!d_internal_quat_ld_finite(_a, 4u)) ||
         (!d_internal_quat_ld_finite(_b, 4u)) ||
         (!d_internal_quat_ld_finite(&_t, 1u)) )
    {
        d_internal_quat_ld_fill_nan(_out, 4u);

        return;
    }

    {
        const long double flip = (d_internal_quat_ld_dot_sign(_a, _b, 4u) <
                                  0.0L) ? -1.0L : 1.0L;

        // the shorter arc's end, and the chords to it and through it
        for (i = 0u; i < 4u; ++i)
        {
            b[i]    = flip * _b[i];
            diff[i] = _a[i] - b[i];
            sum[i]  = _a[i] + b[i];
        }
    }

    {
        const long double theta = 2.0L * d_math_ld_atan2_c(
            d_internal_quat_ld_norm_c(diff, 4u).high,
            d_internal_quat_ld_norm_c(sum, 4u).high);

        // no angle: the line
        if (theta == 0.0L)
        {
            for (i = 0u; i < 4u; ++i)
            {
                _out[i] = d_internal_quat_ld_lerp(_a[i], b[i], _t);
            }

            return;
        }

        {
            const long double s    = d_math_ld_sin_c(theta);
            const long double rest = 1.0L - _t;
            const long double s_a  = d_math_ld_sin_c(rest * theta) / s;
            const long double s_b  = d_math_ld_sin_c(_t * theta) / s;
            const long double f[2] = { s_a, s_b };

            // each part of the arc
            for (i = 0u; i < 4u; ++i)
            {
                const long double ends[2] = { _a[i], b[i] };

                _out[i] = d_internal_quat_ld_dot(f, ends, 2u);
            }
        }
    }

    return;
}
"""


QUAT_FAMILY = r"""

//==============================================================================
// @SECTION@.  THE @UPPER@ FAMILY (_@S@)
//==============================================================================
// A quaternion is a caller's array [x, y, z, w]: x i + y j + z k + w. Each
// accurate function widens its operands to long double, exactly, takes
// section 1's kernel and rounds each part once; the _fast forms are the
// former formulas in the family's own type, for a renderer's inner loops; the
// plain forms of what takes a sine or a cosine are the same algorithms in the
// family's type on the C library's, and of what takes a square root alone
// the kernel on the library's root, which gives the same bits. _out may be
// an operand everywhere.


// @SECTION@.1    Helpers
//------------------------------------------------------------------------------

// d_internal_quat_@S@_widen
//   _out = _v's _n parts as long doubles, exactly.
D_MATH_FN void
d_internal_quat_@S@_widen(
    long double* _out,
    const @T@*   _v,
    size_t       _n
)
{
    size_t i = 0u;

    // each part, exactly
    for (i = 0u; i < _n; ++i)
    {
        _out[i] = (long double)_v[i];
    }

    return;
}

// d_internal_quat_@S@_narrow
//   _out = each of _p's _n pairs rounded once to the family
// (d_internal_math_@S@_round_pair).
D_MATH_FN void
d_internal_quat_@S@_narrow(
    @T@*                                   _out,
    const struct dj_internal_math_ld_pair* _p,
    size_t                                 _n
)
{
    size_t i = 0u;

    // each pair, rounded once
    for (i = 0u; i < _n; ++i)
    {
        _out[i] = d_internal_math_@S@_round_pair(_p[i].high, _p[i].low);
    }

    return;
}

// d_internal_quat_@S@_all_finite
//   whether each of _v's _n parts is finite.
D_MATH_FN bool
d_internal_quat_@S@_all_finite(
    const @T@* _v,
    size_t     _n
)
{
    size_t i = 0u;

    // a NaN or an infinity among the parts
    for (i = 0u; i < _n; ++i)
    {
        if ( (_v[i] != _v[i]) ||
             (d_internal_cplx_@S@_is_inf(_v[i])) )
        {
            return false;
        }
    }

    return true;
}

// d_internal_quat_@S@_all_zero
//   whether each of _v's _n parts is 0, of either sign.
D_MATH_FN bool
d_internal_quat_@S@_all_zero(
    const @T@* _v,
    size_t     _n
)
{
    size_t i = 0u;

    // a part that is not 0
    for (i = 0u; i < _n; ++i)
    {
        if (_v[i] != (@T@)0)
        {
            return false;
        }
    }

    return true;
}

#if @D2@
// d_internal_quat_@S@_widen_d2
//   _out = _v's _n parts as doubles, exactly.
D_MATH_FN void
d_internal_quat_@S@_widen_d2(
    double*    _out,
    const @T@* _v,
    size_t     _n
)
{
    size_t i = 0u;

    // each part, exactly
    for (i = 0u; i < _n; ++i)
    {
        _out[i] = (double)_v[i];
    }

    return;
}

@NARROW_D2@
#endif  // @D2@

// d_internal_quat_@S@_nans
//   _out = _n NaNs.
D_MATH_FN void
d_internal_quat_@S@_nans(
    @T@*   _out,
    size_t _n
)
{
    size_t i = 0u;

    // every part a NaN
    for (i = 0u; i < _n; ++i)
    {
        _out[i] = d_internal_math_@S@_nan();
    }

    return;
}

// d_internal_quat_@S@_any_nan
//   whether any of _v's _n parts is a NaN.
D_MATH_FN bool
d_internal_quat_@S@_any_nan(
    const @T@* _v,
    size_t     _n
)
{
    size_t i = 0u;

    // a NaN among the parts
    for (i = 0u; i < _n; ++i)
    {
        if (_v[i] != _v[i])
        {
            return true;
        }
    }

    return false;
}

// d_internal_quat_@S@_part_along
//   _u _s, a zero where _u is 0, its sign _u's times _s's
// (d_internal_quat_ld_along).
D_MATH_FN @T@
d_internal_quat_@S@_part_along(
    @T@ _u,
    @T@ _s
)
{
    const @T@ zero = (@T@)0;
    const @T@ one  = (@T@)1;

    // a zero part: a zero, of the product's sign
    if (_u == zero)
    {
        const @T@ signed_zero = d_internal_cplx_@S@_copysign(zero, _s);

        return (d_internal_cplx_@S@_copysign(one, _u) < zero) ?
                   -signed_zero : signed_zero;
    }

    return _u * _s;
}

// dj_internal_quat_@S@_plain_plane
//   a quaternion's plane in the family (section 1.7), for the plain forms.
struct dj_internal_quat_@S@_plain_plane
{
    @T@ w;
    @T@ r;
    @T@ u[3];
};

// d_internal_quat_@S@_plain_plane_of
//   _q's plane, its vector part not 0 and no part a NaN: r = |v| by vec.h's
// scaled norm, u = v / r; a part of v infinite, r +inf and u the direction of
// v's infinities (d_internal_quat_ld_direction).
D_MATH_FN_RT struct dj_internal_quat_@S@_plain_plane
d_internal_quat_@S@_plain_plane_of(
    const @T@* _q
)
{
    struct dj_internal_quat_@S@_plain_plane result = {
        (@T@)0, (@T@)0,
        { (@T@)0, (@T@)0, (@T@)0 } };
    long double v[3] = { 0.0L, 0.0L, 0.0L };
    long double u[3] = { 0.0L, 0.0L, 0.0L };
    size_t      i    = 0u;

    d_internal_quat_@S@_widen(v, _q, 3u);
    result.w = _q[3];

    // an infinite part of v: an infinite r along v's infinities
    if (d_internal_quat_ld_has_inf(v, 3u))
    {
        d_internal_quat_ld_direction(u, v, 3u);
        result.r = d_internal_math_@S@_infinity();

        for (i = 0u; i < 3u; ++i)
        {
            result.u[i] = (@T@)u[i];
        }

        return result;
    }

    result.r = d_vec_@S@_norm(_q, 3u);

    // u = v / |v|
    for (i = 0u; i < 3u; ++i)
    {
        result.u[i] = _q[i] / result.r;
    }

    return result;
}


// @SECTION@.2    Algebra
//------------------------------------------------------------------------------
// Each part of the product, the inverse and the quotients is a pair's sum
// rounded once, as accurate as twice the pairs' precision: double's pairs
// where double is evaluated in its own type (section 1.3), a float's
// products exact in them; long double's for long double, and for float and
// double where double is evaluated wider, as on a 32-bit x87 (section 1.2).

// d_quat_@S@_multiply_fast
//   _out = _a _b by the formula, x = w x' + x w' + y z' - z y' and so on, 16
// products and 12 sums in the family's type, in order: for a renderer's inner
// loops, each part's error unbounded where its products cancel.
D_MATH_FN void
d_quat_@S@_multiply_fast(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b
)
{
    const @T@ ax = _a[0], ay = _a[1], az = _a[2], aw = _a[3];
    const @T@ bx = _b[0], by = _b[1], bz = _b[2], bw = _b[3];
    // x: wx' + xw' + yz' - zy'
    const @T@ x1 = aw * bx;
    const @T@ x2 = ax * bw;
    const @T@ x3 = ay * bz;
    const @T@ x4 = az * by;
    const @T@ x12 = x1 + x2;
    const @T@ x123 = x12 + x3;
    // y: wy' - xz' + yw' + zx'
    const @T@ y1 = aw * by;
    const @T@ y2 = ax * bz;
    const @T@ y3 = ay * bw;
    const @T@ y4 = az * bx;
    const @T@ y12 = y1 - y2;
    const @T@ y123 = y12 + y3;
    // z: wz' + xy' - yx' + zw'
    const @T@ z1 = aw * bz;
    const @T@ z2 = ax * by;
    const @T@ z3 = ay * bx;
    const @T@ z4 = az * bw;
    const @T@ z12 = z1 + z2;
    const @T@ z123 = z12 - z3;
    // w: ww' - xx' - yy' - zz'
    const @T@ w1 = aw * bw;
    const @T@ w2 = ax * bx;
    const @T@ w3 = ay * by;
    const @T@ w4 = az * bz;
    const @T@ w12 = w1 - w2;
    const @T@ w123 = w12 - w3;

    _out[0] = x123 - x4;
    _out[1] = y123 + y4;
    _out[2] = z123 + z4;
    _out[3] = w123 - w4;

    return;
}

// d_quat_@S@_multiply
//   _out = _a _b, Hamilton's product: the rotation _b, then _a. Each part
// the sum of its four products as Dot2 sums them, rounded once: in double's
// pairs where double is evaluated in its own type (section 1.3), long
// double's otherwise (section 1.2); where an operand has an infinite or NaN
// part, the formula as written (d_quat_@S@_multiply_fast).
D_MATH_FN void
d_quat_@S@_multiply(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b
)
{
    // an infinity or a NaN: the formula as written
    if ( (!d_internal_quat_@S@_all_finite(_a, 4u)) ||
         (!d_internal_quat_@S@_all_finite(_b, 4u)) )
    {
        d_quat_@S@_multiply_fast(_out, _a, _b);

        return;
    }

#if @D2@
    {
        double                         a[4] = { 0.0, 0.0, 0.0, 0.0 };
        double                         b[4] = { 0.0, 0.0, 0.0, 0.0 };
        struct dj_internal_math_d_pair r[4] = { { 0.0, 0.0 }, { 0.0, 0.0 },
                                                { 0.0, 0.0 }, { 0.0, 0.0 } };

        d_internal_quat_@S@_widen_d2(a, _a, 4u);
        d_internal_quat_@S@_widen_d2(b, _b, 4u);
        d_internal_quat_d2_multiply(r, a, b);
        d_internal_quat_@S@_narrow_d2(_out, r, 4u);
    }
#else  // @D2@
    {
        long double                     a[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
        long double                     b[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
        struct dj_internal_math_ld_pair r[4] = { { 0.0L, 0.0L },
                                                 { 0.0L, 0.0L },
                                                 { 0.0L, 0.0L },
                                                 { 0.0L, 0.0L } };

        d_internal_quat_@S@_widen(a, _a, 4u);
        d_internal_quat_@S@_widen(b, _b, 4u);
        d_internal_quat_ld_multiply(r, a, b);
        d_internal_quat_@S@_narrow(_out, r, 4u);
    }
#endif  // @D2@

    return;
}

// d_quat_@S@_conjugate
//   _out = (-x, -y, -z, w): the inverse of a unit quaternion.
D_MATH_FN void
d_quat_@S@_conjugate(
    @T@*       _out,
    const @T@* _q
)
{
    _out[0] = -_q[0];
    _out[1] = -_q[1];
    _out[2] = -_q[2];
    _out[3] = _q[3];

    return;
}

// d_internal_quat_@S@_quotient
//   _out = _p / _q where _right, q^-1 p otherwise, and q's inverse where _p
// is NULL, both finite and q not 0: each part the kernel's pair rounded
// once, in double's pairs or long double's as d_quat_@S@_multiply takes
// them.
D_MATH_FN void
d_internal_quat_@S@_quotient(
    @T@*       _out,
    const @T@* _p,
    const @T@* _q,
    bool       _right
)
{
#if @D2@
    double                         p[4] = { 0.0, 0.0, 0.0, 0.0 };
    double                         q[4] = { 0.0, 0.0, 0.0, 0.0 };
    struct dj_internal_math_d_pair r[4] = { { 0.0, 0.0 }, { 0.0, 0.0 },
                                            { 0.0, 0.0 }, { 0.0, 0.0 } };

    d_internal_quat_@S@_widen_d2(q, _q, 4u);

    // the inverse: no numerator
    if (_p == NULL)
    {
        d_internal_quat_d2_divide(r, NULL, q, true);
    }
    else
    {
        d_internal_quat_@S@_widen_d2(p, _p, 4u);
        d_internal_quat_d2_divide(r, p, q, _right);
    }

    d_internal_quat_@S@_narrow_d2(_out, r, 4u);
#else  // @D2@
    long double                     p[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    long double                     q[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    struct dj_internal_math_ld_pair r[4] = { { 0.0L, 0.0L },
                                             { 0.0L, 0.0L },
                                             { 0.0L, 0.0L },
                                             { 0.0L, 0.0L } };

    d_internal_quat_@S@_widen(q, _q, 4u);

    // the inverse: no numerator
    if (_p == NULL)
    {
        d_internal_quat_ld_inverse(r, q);
    }
    else
    {
        d_internal_quat_@S@_widen(p, _p, 4u);
        d_internal_quat_ld_divide(r, p, q, _right);
    }

    d_internal_quat_@S@_narrow(_out, r, 4u);
#endif  // @D2@

    return;
}

// d_quat_@S@_inverse
//   _out = 1 / _q = q* / |q|^2, each part the kernel's pair rounded once
// (d_internal_quat_@S@_quotient). q = 0: a NaN in each part, the formula's 0
// / 0; a NaN part: NaNs; an infinite part, none a NaN: the zeros of q*'s
// parts' signs, as 1 / inf is 0.
D_MATH_FN void
d_quat_@S@_inverse(
    @T@*       _out,
    const @T@* _q
)
{
    long double q[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    size_t      i    = 0u;

    d_internal_quat_@S@_widen(q, _q, 4u);

    // a NaN, or 0: no inverse
    if ( (d_internal_quat_ld_has_nan(q, 4u)) ||
         (d_internal_quat_ld_is_zero(q, 4u)) )
    {
        d_internal_quat_@S@_nans(_out, 4u);

        return;
    }

    // an infinity: the conjugate's zeros
    if (d_internal_quat_ld_has_inf(q, 4u))
    {
        for (i = 0u; i < 4u; ++i)
        {
            const long double part = (i < 3u) ? -q[i] : q[i];

            _out[i] = (@T@)d_internal_cplx_ld_copysign(0.0L, part);
        }

        return;
    }

    d_internal_quat_@S@_quotient(_out, NULL, _q, true);

    return;
}

// d_quat_@S@_divide
//   _out = _a / _b = a b^-1, the quotient on the right (S277): a b* / |b|^2,
// the numerator's parts as pairs over |b|^2's, each rounded once
// (d_internal_quat_@S@_quotient). Where an operand has an infinite or NaN
// part, or b is 0, a times b's inverse (d_quat_@S@_inverse), the product as
// written.
D_MATH_FN void
d_quat_@S@_divide(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b
)
{
    // finite, b not 0: the kernel
    if ( (d_internal_quat_@S@_all_finite(_a, 4u)) &&
         (d_internal_quat_@S@_all_finite(_b, 4u)) &&
         (!d_internal_quat_@S@_all_zero(_b, 4u)) )
    {
        d_internal_quat_@S@_quotient(_out, _a, _b, true);

        return;
    }

    {
        const @T@ left[4]  = { _a[0], _a[1], _a[2], _a[3] };
        @T@       right[4] = { (@T@)0, (@T@)0,
                               (@T@)0, (@T@)0 };

        d_quat_@S@_inverse(right, _b);
        d_quat_@S@_multiply_fast(_out, left, right);
    }

    return;
}

// d_quat_@S@_left_divide
//   _out = _a \ _b = a^-1 b, the quotient on the left: a* b / |a|^2, as
// d_quat_@S@_divide computes its own; where an operand has an infinite or
// NaN part, or a is 0, a's inverse times b, the product as written.
D_MATH_FN void
d_quat_@S@_left_divide(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b
)
{
    // finite, a not 0: the kernel
    if ( (d_internal_quat_@S@_all_finite(_a, 4u)) &&
         (d_internal_quat_@S@_all_finite(_b, 4u)) &&
         (!d_internal_quat_@S@_all_zero(_a, 4u)) )
    {
        d_internal_quat_@S@_quotient(_out, _b, _a, false);

        return;
    }

    {
        const @T@ right[4] = { _b[0], _b[1], _b[2], _b[3] };
        @T@       left[4]  = { (@T@)0, (@T@)0,
                               (@T@)0, (@T@)0 };

        d_quat_@S@_inverse(left, _a);
        d_quat_@S@_multiply_fast(_out, left, right);
    }

    return;
}


// @SECTION@.3    Norms
//------------------------------------------------------------------------------

// d_quat_@S@_norm_c, d_quat_@S@_norm
//   function: |q|, the kernel's pair rounded once (d_internal_quat_ld_norm_c):
// scaled, so that it overflows only where |q| does; +inf where a part is
// infinite, NaN where one is a NaN. The _c form a constant expression from
// C++14, the plain one on the C library's square root, the same value.
D_MATH_FN @T@
d_quat_@S@_norm_c(
    const @T@* _q
)
{
    long double                          q[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    struct dj_internal_math_ld_pair      n    = { 0.0L, 0.0L };

    d_internal_quat_@S@_widen(q, _q, 4u);
    n = d_internal_quat_ld_norm_c(q, 4u);

    return d_internal_math_@S@_round_pair(n.high, n.low);
}

D_MATH_FN_RT @T@
d_quat_@S@_norm(
    const @T@* _q
)
{
    long double                          q[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    struct dj_internal_math_ld_pair      n    = { 0.0L, 0.0L };

    d_internal_quat_@S@_widen(q, _q, 4u);
    n = d_internal_quat_ld_norm(q, 4u);

    return d_internal_math_@S@_round_pair(n.high, n.low);
}

// d_quat_@S@_normalize_c, d_quat_@S@_normalize
//   _out = _q / |q|, each part the kernel's pair rounded once
// (d_internal_quat_ld_normalize_c), q scaled so that nothing overflows. A
// zero q is left as it is, signs of zero included; a NaN part gives NaNs, an
// infinite one the direction of q's infinities. The _c form a constant
// expression from C++14, the plain one on the C library's square root, the
// same value.
D_MATH_FN void
d_quat_@S@_normalize_c(
    @T@*       _out,
    const @T@* _q
)
{
    long double                     q[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    struct dj_internal_math_ld_pair r[4] = { { 0.0L, 0.0L }, { 0.0L, 0.0L },
                                             { 0.0L, 0.0L }, { 0.0L, 0.0L } };

    d_internal_quat_@S@_widen(q, _q, 4u);
    d_internal_quat_ld_normalize_c(r, q);
    d_internal_quat_@S@_narrow(_out, r, 4u);

    return;
}

D_MATH_FN_RT void
d_quat_@S@_normalize(
    @T@*       _out,
    const @T@* _q
)
{
    long double                     q[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    struct dj_internal_math_ld_pair r[4] = { { 0.0L, 0.0L }, { 0.0L, 0.0L },
                                             { 0.0L, 0.0L }, { 0.0L, 0.0L } };

    d_internal_quat_@S@_widen(q, _q, 4u);
    d_internal_quat_ld_normalize(r, q);
    d_internal_quat_@S@_narrow(_out, r, 4u);

    return;
}

// d_quat_@S@_normalize_fast
//   _out = _q times 1 / sqrt(x^2 + y^2 + z^2 + w^2), the reciprocal square
// root a renderer's normalization takes, in the family's type and unscaled:
// for |q|^2 within the type's range. A zero q is left as it is.
D_MATH_FN_RT void
d_quat_@S@_normalize_fast(
    @T@*       _out,
    const @T@* _q
)
{
    const @T@ xx  = _q[0] * _q[0];
    const @T@ yy  = _q[1] * _q[1];
    const @T@ zz  = _q[2] * _q[2];
    const @T@ ww  = _q[3] * _q[3];
    const @T@ xy  = xx + yy;
    const @T@ xyz = xy + zz;
    const @T@ sum = xyz + ww;
    size_t    i   = 0u;

    // a zero quaternion: left as it is
    if (sum == (@T@)0)
    {
        for (i = 0u; i < 4u; ++i)
        {
            _out[i] = _q[i];
        }

        return;
    }

    {
        const @T@ s = (@T@)1 / d_math_@S@_sqrt(sum);

        // each part by the reciprocal root
        for (i = 0u; i < 4u; ++i)
        {
            _out[i] = _q[i] * s;
        }
    }

    return;
}


// @SECTION@.4    Rotation
//------------------------------------------------------------------------------
// A rotation is q v q^-1, active and right-handed, as glTF's, Eigen's and
// GLM's (S276): for a unit q, q v q*. The accurate forms take any q not 0,
// as its norm cancels; the _fast forms assume a unit q, as before.

// d_quat_@S@_rotate_fast
//   _out = _v rotated by the unit quaternion _q, v + w t + u x t with u the
// vector part and t = 2 (u x v), in the family's type, without building a
// matrix.
D_MATH_FN void
d_quat_@S@_rotate_fast(
    @T@*       _out,
    const @T@* _q,
    const @T@* _v
)
{
    @T@ t[3]  = { (@T@)0, (@T@)0, (@T@)0 };
    @T@ wt[3] = { (@T@)0, (@T@)0, (@T@)0 };
    @T@ ut[3] = { (@T@)0, (@T@)0, (@T@)0 };

    d_vec_@S@_cross(t, _q, _v);
    d_vec_@S@_scale(t, t, (@T@)2, 3u);
    d_vec_@S@_scale(wt, t, _q[3], 3u);
    d_vec_@S@_cross(ut, _q, t);
    d_vec_@S@_add(_out, _v, wt, 3u);
    d_vec_@S@_add(_out, _out, ut, 3u);

    return;
}

// d_quat_@S@_rotate
//   _out = q v q^-1, the 3-vector _v turned by the rotation _q names, for any
// q not 0: each part the kernel's pair rounded once
// (d_internal_quat_ld_rotate). v = 0 is left as it is; q = 0, or a NaN or
// infinite part, gives a NaN in each part.
D_MATH_FN void
d_quat_@S@_rotate(
    @T@*       _out,
    const @T@* _q,
    const @T@* _v
)
{
    long double                     q[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    long double                     v[3] = { 0.0L, 0.0L, 0.0L };
    struct dj_internal_math_ld_pair r[3] = { { 0.0L, 0.0L }, { 0.0L, 0.0L },
                                             { 0.0L, 0.0L } };
    size_t                          i    = 0u;

    d_internal_quat_@S@_widen(q, _q, 4u);
    d_internal_quat_@S@_widen(v, _v, 3u);

    // no rotation: a zero q, a NaN or an infinity
    if ( (!d_internal_quat_ld_finite(q, 4u)) ||
         (!d_internal_quat_ld_finite(v, 3u)) ||
         (d_internal_quat_ld_is_zero(q, 4u)) )
    {
        d_internal_quat_@S@_nans(_out, 3u);

        return;
    }

    // the zero vector: as it is
    if (d_internal_quat_ld_is_zero(v, 3u))
    {
        for (i = 0u; i < 3u; ++i)
        {
            _out[i] = _v[i];
        }

        return;
    }

    d_internal_quat_ld_rotate(r, q, v);
    d_internal_quat_@S@_narrow(_out, r, 3u);

    return;
}

// d_quat_@S@_to_matrix3_fast
//   _out (3 x 3, by rows) = the rotation matrix of the unit quaternion _q, 1
// - 2 (y^2 + z^2) and so on, in the family's type.
D_MATH_FN void
d_quat_@S@_to_matrix3_fast(
    @T@*       _out,
    const @T@* _q
)
{
    const @T@ x  = _q[0];
    const @T@ y  = _q[1];
    const @T@ z  = _q[2];
    const @T@ w  = _q[3];
    const @T@ xx = x * x;
    const @T@ yy = y * y;
    const @T@ zz = z * z;
    const @T@ xy = x * y;
    const @T@ xz = x * z;
    const @T@ yz = y * z;
    const @T@ wx = w * x;
    const @T@ wy = w * y;
    const @T@ wz = w * z;
    // the sums and differences the entries double
    const @T@ yy_zz = yy + zz;
    const @T@ xx_zz = xx + zz;
    const @T@ xx_yy = xx + yy;
    const @T@ xy_m  = xy - wz;
    const @T@ xy_p  = xy + wz;
    const @T@ xz_p  = xz + wy;
    const @T@ xz_m  = xz - wy;
    const @T@ yz_m  = yz - wx;
    const @T@ yz_p  = yz + wx;
    const @T@ d00   = (@T@)2 * yy_zz;
    const @T@ d11   = (@T@)2 * xx_zz;
    const @T@ d22   = (@T@)2 * xx_yy;

    _out[0] = (@T@)1 - d00;
    _out[1] = (@T@)2 * xy_m;
    _out[2] = (@T@)2 * xz_p;
    _out[3] = (@T@)2 * xy_p;
    _out[4] = (@T@)1 - d11;
    _out[5] = (@T@)2 * yz_m;
    _out[6] = (@T@)2 * xz_m;
    _out[7] = (@T@)2 * yz_p;
    _out[8] = (@T@)1 - d22;

    return;
}

// d_quat_@S@_to_matrix3
//   _out (3 x 3, by rows) = the rotation matrix of _q, any q not 0, each
// entry the kernel's pair rounded once (d_internal_quat_ld_to_matrix3). q =
// 0, or a NaN or infinite part, gives NaN entries.
D_MATH_FN void
d_quat_@S@_to_matrix3(
    @T@*       _out,
    const @T@* _q
)
{
    long double                     q[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    struct dj_internal_math_ld_pair r[9] = { { 0.0L, 0.0L }, { 0.0L, 0.0L },
                                             { 0.0L, 0.0L }, { 0.0L, 0.0L },
                                             { 0.0L, 0.0L }, { 0.0L, 0.0L },
                                             { 0.0L, 0.0L }, { 0.0L, 0.0L },
                                             { 0.0L, 0.0L } };

    d_internal_quat_@S@_widen(q, _q, 4u);

    // no rotation: a zero q, a NaN or an infinity
    if ( (!d_internal_quat_ld_finite(q, 4u)) ||
         (d_internal_quat_ld_is_zero(q, 4u)) )
    {
        d_internal_quat_@S@_nans(_out, 9u);

        return;
    }

    d_internal_quat_ld_to_matrix3(r, q);
    d_internal_quat_@S@_narrow(_out, r, 9u);

    return;
}

// d_quat_@S@_to_matrix4_fast, d_quat_@S@_to_matrix4
//   _out (4 x 4, by rows) = the homogeneous rotation matrix: the 3 x 3 of
// d_quat_@S@_to_matrix3_fast or d_quat_@S@_to_matrix3 in its upper left, the
// identity's last row and column.
D_MATH_FN void
d_quat_@S@_to_matrix4_fast(
    @T@*       _out,
    const @T@* _q
)
{
    @T@    r[9] = { (@T@)0, (@T@)0, (@T@)0,
                    (@T@)0, (@T@)0, (@T@)0,
                    (@T@)0, (@T@)0, (@T@)0 };
    size_t i    = 0u;

    d_quat_@S@_to_matrix3_fast(r, _q);
    d_mat_@S@_identity(_out, 4u);

    // the 3 x 3 into the upper left
    for (i = 0u; i < 3u; ++i)
    {
        size_t j = 0u;

        for (j = 0u; j < 3u; ++j)
        {
            _out[(i * 4u) + j] = r[(i * 3u) + j];
        }
    }

    return;
}

D_MATH_FN void
d_quat_@S@_to_matrix4(
    @T@*       _out,
    const @T@* _q
)
{
    @T@    r[9] = { (@T@)0, (@T@)0, (@T@)0,
                    (@T@)0, (@T@)0, (@T@)0,
                    (@T@)0, (@T@)0, (@T@)0 };
    size_t i    = 0u;

    d_quat_@S@_to_matrix3(r, _q);
    d_mat_@S@_identity(_out, 4u);

    // the 3 x 3 into the upper left
    for (i = 0u; i < 3u; ++i)
    {
        size_t j = 0u;

        for (j = 0u; j < 3u; ++j)
        {
            _out[(i * 4u) + j] = r[(i * 3u) + j];
        }
    }

    return;
}

// d_quat_@S@_from_matrix3_c, d_quat_@S@_from_matrix3
//   _out = the unit quaternion, w >= 0, of the rotation matrix _m (3 x 3, by
// rows), by Shepperd's method (d_internal_quat_ld_from_matrix3_c), each part
// rounded once: to_matrix3's inverse, q or -q. A NaN or infinite entry gives
// NaNs. The _c form a constant expression from C++14, the plain one on the C
// library's square root, the same value.
D_MATH_FN void
d_quat_@S@_from_matrix3_c(
    @T@*       _out,
    const @T@* _m
)
{
    long double                     m[9] = { 0.0L, 0.0L, 0.0L, 0.0L, 0.0L,
                                             0.0L, 0.0L, 0.0L, 0.0L };
    struct dj_internal_math_ld_pair r[4] = { { 0.0L, 0.0L }, { 0.0L, 0.0L },
                                             { 0.0L, 0.0L }, { 0.0L, 0.0L } };

    d_internal_quat_@S@_widen(m, _m, 9u);
    d_internal_quat_ld_from_matrix3_c(r, m);
    d_internal_quat_@S@_narrow(_out, r, 4u);

    return;
}

D_MATH_FN_RT void
d_quat_@S@_from_matrix3(
    @T@*       _out,
    const @T@* _m
)
{
    long double                     m[9] = { 0.0L, 0.0L, 0.0L, 0.0L, 0.0L,
                                             0.0L, 0.0L, 0.0L, 0.0L };
    struct dj_internal_math_ld_pair r[4] = { { 0.0L, 0.0L }, { 0.0L, 0.0L },
                                             { 0.0L, 0.0L }, { 0.0L, 0.0L } };

    d_internal_quat_@S@_widen(m, _m, 9u);
    d_internal_quat_ld_from_matrix3(r, m);
    d_internal_quat_@S@_narrow(_out, r, 4u);

    return;
}

// d_quat_@S@_from_matrix4_c, d_quat_@S@_from_matrix4
//   _out = the unit quaternion of the 4 x 4 matrix _m's upper-left 3 x 3
// (d_quat_@S@_from_matrix3_c).
D_MATH_FN void
d_quat_@S@_from_matrix4_c(
    @T@*       _out,
    const @T@* _m
)
{
    const @T@ m[9] = { _m[0], _m[1], _m[2],
                       _m[4], _m[5], _m[6],
                       _m[8], _m[9], _m[10] };

    d_quat_@S@_from_matrix3_c(_out, m);

    return;
}

D_MATH_FN_RT void
d_quat_@S@_from_matrix4(
    @T@*       _out,
    const @T@* _m
)
{
    const @T@ m[9] = { _m[0], _m[1], _m[2],
                       _m[4], _m[5], _m[6],
                       _m[8], _m[9], _m[10] };

    d_quat_@S@_from_matrix3(_out, m);

    return;
}

// d_quat_@S@_from_two_vectors_c, d_quat_@S@_from_two_vectors
//   _out = the unit quaternion of the shortest rotation turning the direction
// of the 3-vector _a into _b's (d_internal_quat_ld_from_two_vectors_c), each
// part rounded once: exactly opposed, a half turn about an axis
// perpendicular to a; a zero vector, the identity; a NaN or infinite part,
// NaNs. The _c form a constant expression from C++14, the plain one on the C
// library's square root, the same value.
D_MATH_FN void
d_quat_@S@_from_two_vectors_c(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b
)
{
    long double                     a[3] = { 0.0L, 0.0L, 0.0L };
    long double                     b[3] = { 0.0L, 0.0L, 0.0L };
    struct dj_internal_math_ld_pair r[4] = { { 0.0L, 0.0L }, { 0.0L, 0.0L },
                                             { 0.0L, 0.0L }, { 0.0L, 0.0L } };

    d_internal_quat_@S@_widen(a, _a, 3u);
    d_internal_quat_@S@_widen(b, _b, 3u);
    d_internal_quat_ld_from_two_vectors_c(r, a, b);
    d_internal_quat_@S@_narrow(_out, r, 4u);

    return;
}

D_MATH_FN_RT void
d_quat_@S@_from_two_vectors(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b
)
{
    long double                     a[3] = { 0.0L, 0.0L, 0.0L };
    long double                     b[3] = { 0.0L, 0.0L, 0.0L };
    struct dj_internal_math_ld_pair r[4] = { { 0.0L, 0.0L }, { 0.0L, 0.0L },
                                             { 0.0L, 0.0L }, { 0.0L, 0.0L } };

    d_internal_quat_@S@_widen(a, _a, 3u);
    d_internal_quat_@S@_widen(b, _b, 3u);
    d_internal_quat_ld_from_two_vectors(r, a, b);
    d_internal_quat_@S@_narrow(_out, r, 4u);

    return;
}


// @SECTION@.5    Construction and interpolation
//------------------------------------------------------------------------------
// What needs a sine or a cosine comes twice, as the elementary functions do:
// the _c form a constant expression from C++14, the plain one on the C
// library's.
@TRIG_FORMS@
// d_quat_@S@_nlerp_c, d_quat_@S@_nlerp
//   _out = the normalized linear interpolation from _a (t = 0) to _b (t = 1),
// along the shorter arc (d_internal_quat_ld_nlerp_c), each part rounded once:
// the cheap curve between two rotations, whose speed varies along it. A NaN
// or infinite part, of t too, gives NaNs. The _c form a constant expression
// from C++14, the plain one on the C library's square root, the same value.
D_MATH_FN void
d_quat_@S@_nlerp_c(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b,
    @T@        _t
)
{
    long double                     a[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    long double                     b[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    struct dj_internal_math_ld_pair r[4] = { { 0.0L, 0.0L }, { 0.0L, 0.0L },
                                             { 0.0L, 0.0L }, { 0.0L, 0.0L } };

    d_internal_quat_@S@_widen(a, _a, 4u);
    d_internal_quat_@S@_widen(b, _b, 4u);
    d_internal_quat_ld_nlerp_c(r, a, b, (long double)_t);
    d_internal_quat_@S@_narrow(_out, r, 4u);

    return;
}

D_MATH_FN_RT void
d_quat_@S@_nlerp(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b,
    @T@        _t
)
{
    long double                     a[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    long double                     b[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    struct dj_internal_math_ld_pair r[4] = { { 0.0L, 0.0L }, { 0.0L, 0.0L },
                                             { 0.0L, 0.0L }, { 0.0L, 0.0L } };

    d_internal_quat_@S@_widen(a, _a, 4u);
    d_internal_quat_@S@_widen(b, _b, 4u);
    d_internal_quat_ld_nlerp(r, a, b, (long double)_t);
    d_internal_quat_@S@_narrow(_out, r, 4u);

    return;
}

// d_quat_@S@_slerp_c
//   _out = the spherical linear interpolation from _a (t = 0) to _b (t = 1),
// both unit quaternions, along the shorter arc at a constant speed
// (d_internal_quat_ld_slerp_c), each part rounded once: the angle from
// atan2 of the chords, accurate near 0, and a at t = 0, b or -b at t = 1,
// exactly. A NaN or infinite part, of t too, gives NaNs.
D_MATH_FN void
d_quat_@S@_slerp_c(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b,
    @T@        _t
)
{
    long double                     a[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    long double                     b[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    struct dj_internal_math_ld_pair r[4] = { { 0.0L, 0.0L }, { 0.0L, 0.0L },
                                             { 0.0L, 0.0L }, { 0.0L, 0.0L } };

    d_internal_quat_@S@_widen(a, _a, 4u);
    d_internal_quat_@S@_widen(b, _b, 4u);
    d_internal_quat_ld_slerp_c(r, a, b, (long double)_t);
    d_internal_quat_@S@_narrow(_out, r, 4u);

    return;
}

// d_quat_@S@_slerp
//   the same algorithm in the family's type on the C library's: theta = 2
// atan2(|a - b|, |a + b|) by vec.h's scaled norms, the sines the library's,
// each part the sum of the two products; NaNs where the arithmetic gives
// them.
D_MATH_FN_RT void
d_quat_@S@_slerp(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b,
    @T@        _t
)
{
    const @T@ zero    = (@T@)0;
    const @T@ one     = (@T@)1;
    @T@       b[4]    = { zero, zero, zero, zero };
    @T@       diff[4] = { zero, zero, zero, zero };
    @T@       sum[4]  = { zero, zero, zero, zero };
    const @T@ d       = d_vec_@S@_dot(_a, _b, 4u);
    const @T@ flip    = (d < zero) ? -one : one;
    size_t    i       = 0u;

    // the shorter arc's end, and the chords to it and through it
    for (i = 0u; i < 4u; ++i)
    {
        b[i]    = flip * _b[i];
        diff[i] = _a[i] - b[i];
        sum[i]  = _a[i] + b[i];
    }

    {
        const @T@ chord = d_vec_@S@_norm(diff, 4u);
        const @T@ other = d_vec_@S@_norm(sum, 4u);
        const @T@ theta = (one + one) * d_math_@S@_atan2(chord, other);
        @T@       s_a   = one - _t;
        @T@       s_b   = _t;

        // an angle: the arc's sines
        if (theta != zero)
        {
            const @T@ s    = d_math_@S@_sin(theta);
            const @T@ rest = one - _t;

            s_a = d_math_@S@_sin(rest * theta) / s;
            s_b = d_math_@S@_sin(_t * theta) / s;
        }

        // each part of the arc
        for (i = 0u; i < 4u; ++i)
        {
            const @T@ from = s_a * _a[i];
            const @T@ to   = s_b * b[i];

            _out[i] = from + to;
        }
    }

    return;
}


// @SECTION@.6    Exponential, logarithm and powers
//------------------------------------------------------------------------------
// The complex functions in q's plane (section 1.7), principal values (S275):
// for a vector part along im alone -- y and z 0 -- the complex function of w
// + x i, cplx.h's in the same form, y and z as they are, so that a complex
// number's is its own bit for bit; otherwise the _c form the kernel rounded
// once, the plain one the same in the family's type on cplx.h's plain
// functions. A NaN part, there, gives NaNs.
@EXP_FORMS@"""


QUAT_TRIG = r"""
// d_quat_@S@_from_axis_angle@F@
//   _out = the rotation of _angle radians about _axis, which need not be a
// unit vector: (axis sin(a/2) / |axis|, cos(a/2)). A zero axis names no
// rotation: the identity.
D_MATH_FN@RT@ void
d_quat_@S@_from_axis_angle@F@(
    @T@*       _out,
    const @T@* _axis,
    @T@        _angle
)
{
    const @T@ length = d_vec_@S@_norm_c(_axis, 3u);

    // a zero axis names no rotation
    if (length == (@T@)0)
    {
        _out[0] = (@T@)0;
        _out[1] = (@T@)0;
        _out[2] = (@T@)0;
        _out[3] = (@T@)1;
    }
    else
    {
        const @T@ half = _angle / (@T@)2;
        const @T@ sine = d_math_@S@_sin@F@(half);
        const @T@ s    = sine / length;

        _out[0] = _axis[0] * s;
        _out[1] = _axis[1] * s;
        _out[2] = _axis[2] * s;
        _out[3] = d_math_@S@_cos@F@(half);
    }

    return;
}

// d_quat_@S@_from_unit_axis_angle@F@
//   _out = (axis sin(a/2), cos(a/2)) for an axis the caller normalized (a
// zero one gives (0, 0, 0, cos(a/2))).
D_MATH_FN@RT@ void
d_quat_@S@_from_unit_axis_angle@F@(
    @T@*       _out,
    const @T@* _axis,
    @T@        _angle
)
{
    const @T@ half = _angle / (@T@)2;
    const @T@ s    = d_math_@S@_sin@F@(half);

    _out[0] = _axis[0] * s;
    _out[1] = _axis[1] * s;
    _out[2] = _axis[2] * s;
    _out[3] = d_math_@S@_cos@F@(half);

    return;
}
"""


# one of the four functions of section @SECTION@.6, in both forms: @FN@ the
# name, @DOC@ its line, @ARGS@ and @PARAMS@ its extra argument, @KARGS@ the
# kernel's, @CPLX_EXTRA@ the complex function's.
QUAT_EXP = r"""
// d_quat_@S@_@FN@_c, d_quat_@S@_@FN@
//   _out = @DOC@
D_MATH_FN void
d_quat_@S@_@FN@_c(
    @T@*       _out,
    const @T@* _q@PARAMS@
)
{
    long double                     q[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    struct dj_internal_math_ld_pair r[4] = { { 0.0L, 0.0L }, { 0.0L, 0.0L },
                                             { 0.0L, 0.0L }, { 0.0L, 0.0L } };

    // along im alone: the complex function
    if ( (_q[1] == (@T@)0) &&
         (_q[2] == (@T@)0) )
    {
        const struct dj_cplx_@S@ c = d_cplx_@S@_@FN@_c(
            d_cplx_@S@_make(_q[3], _q[0])@CPLX_EXTRA@);

        _out[0] = c.part[1];
        _out[1] = _q[1];
        _out[2] = _q[2];
        _out[3] = c.part[0];

        return;
    }

    d_internal_quat_@S@_widen(q, _q, 4u);

    // a NaN: NaNs
    if ( (d_internal_quat_ld_has_nan(q, 4u))@NAN_EXTRA@ )
    {
        d_internal_quat_@S@_nans(_out, 4u);

        return;
    }

    d_internal_quat_ld_@FN@_c(r, q@KARGS@);
    d_internal_quat_@S@_narrow(_out, r, 4u);

    return;
}

D_MATH_FN_RT void
d_quat_@S@_@FN@(
    @T@*       _out,
    const @T@* _q@PARAMS@
)
{
    @T@    r[4] = { (@T@)0, (@T@)0,
                    (@T@)0, (@T@)0 };
    size_t i    = 0u;

    // along im alone: the complex function
    if ( (_q[1] == (@T@)0) &&
         (_q[2] == (@T@)0) )
    {
        const struct dj_cplx_@S@ c = d_cplx_@S@_@FN@(
            d_cplx_@S@_make(_q[3], _q[0])@CPLX_EXTRA@);

        _out[0] = c.part[1];
        _out[1] = _q[1];
        _out[2] = _q[2];
        _out[3] = c.part[0];

        return;
    }

    // a NaN: NaNs
    if ( (d_internal_quat_@S@_any_nan(_q, 4u))@NAN_EXTRA@ )
    {
        d_internal_quat_@S@_nans(_out, 4u);

        return;
    }

    {
        const struct dj_internal_quat_@S@_plain_plane p =
            d_internal_quat_@S@_plain_plane_of(_q);
        const struct dj_cplx_@S@ c = d_cplx_@S@_@FN@(
            d_cplx_@S@_make(p.w, p.r)@CPLX_EXTRA@);
@PLAIN_BODY@
        r[3] = c.part[0];
    }

    // the parts, after every read of _q
    for (i = 0u; i < 4u; ++i)
    {
        _out[i] = r[i];
    }

    return;
}
"""

QUAT_EXP_ALONG = r"""
        // the imaginary part along u
        for (i = 0u; i < 3u; ++i)
        {
            r[i] = d_internal_quat_@S@_part_along(p.u[i], c.part[1]);
        }
"""

QUAT_LOG_PLAIN = r"""
        const @T@ tiny =
            d_internal_math_@S@_pow2(-((@P@ / 2) + 2));

        // r far below w > 0: v / w, atan2(r, w) / r being 1 / w
        if ( (p.w > (@T@)0) &&
             (!d_internal_cplx_@S@_is_inf(p.w)) &&
             (p.r < (p.w * tiny)) )
        {
            for (i = 0u; i < 3u; ++i)
            {
                r[i] = _q[i] / _q[3];
            }
        }
        else
        {
            for (i = 0u; i < 3u; ++i)
            {
                r[i] = d_internal_quat_@S@_part_along(p.u[i], c.part[1]);
            }
        }
"""

QUAT_SQRT_PLAIN = r"""
        // the real part the larger, q finite: v / (2 Re), as 2 Re Im = r
        if ( (!d_internal_cplx_@S@_is_inf(p.r)) &&
             (!d_internal_cplx_@S@_is_inf(p.w)) &&
             (c.part[0] >= c.part[1]) &&
             (c.part[0] > (@T@)0) )
        {
            const @T@ twice = (@T@)2 * c.part[0];

            for (i = 0u; i < 3u; ++i)
            {
                r[i] = _q[i] / twice;
            }
        }
        else
        {
            for (i = 0u; i < 3u; ++i)
            {
                r[i] = d_internal_quat_@S@_part_along(p.u[i], c.part[1]);
            }
        }
"""

QUAT_EXP_SPECS = [
    # name, doc, params, kernel args, complex extra, nan extra, plain body
    ("exp", "e^q = e^w (cos |v| + u sin |v|), u = v / |v|.",
     "", "", "", "", QUAT_EXP_ALONG),
    ("log", "log q = log |q| + u atan2(|v|, w), the principal logarithm,\n"
            "// u = im where v is 0, so that log(-1) is pi im (S275).",
     "", "", "", "", QUAT_LOG_PLAIN),
    ("sqrt", "sqrt q, the principal square root, its real part >= 0: the\n"
             "// complex root of w + |v| i, its imaginary part along u.",
     "", "", "", "", QUAT_SQRT_PLAIN),
    ("pow", "q^_p = exp(p log q) for a real p, the principal power: the\n"
            "// complex one of w + |v| i, its imaginary part along u.",
     ",\n    @T@        _p", ", (long double)_p",
     ",\n            d_cplx_@S@_make(_p, (@T@)0)", " ||\n         (_p != _p)",
     QUAT_EXP_ALONG),
]


QUAT_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE KERNELS
    -----------
    1.  Pairs and scaling
    2.  The algebra
    3.  The algebra in double
    4.  Rotation
    5.  Roots: the _c forms'
    6.  Roots: the plain forms'
    7.  The _c forms' functions
2.  THE FLOAT FAMILY (_f)
    ---------------------
    1.  Helpers
    2.  Algebra
    3.  Norms
    4.  Rotation
    5.  Construction and interpolation
    6.  Exponential, logarithm and powers
3.  THE DOUBLE FAMILY (_d)
    ----------------------
    1.  Helpers
    2.  Algebra
    3.  Norms
    4.  Rotation
    5.  Construction and interpolation
    6.  Exponential, logarithm and powers
4.  THE LONG DOUBLE FAMILY (_ld)
    ----------------------------
    1.  Helpers
    2.  Algebra
    3.  Norms
    4.  Rotation
    5.  Construction and interpolation
    6.  Exponential, logarithm and powers
5.  ONE NAME FOR EVERY FAMILY (C++)
    -------------------------------
*/

#ifndef DJINTERP_C_MATH_QUAT_H
#define DJINTERP_C_MATH_QUAT_H 1

// std
#include <float.h>           // LDBL_MANT_DIG, LDBL_MIN, LDBL_MIN_EXP, ...
#include <stddef.h>          // size_t
// djinterp
#include "./cplx.h"          // d_cplx_*_exp, _log, _sqrt, _pow, their kernels
#include "./elementary.h"    // d_math_*_sin, _cos, _sqrt, pairs, round_pair
#include "./exponential.h"   // d_internal_math_*_infinity
#include "./float_order.h"   // d_internal_math_*_pow2
#include "./mat.h"           // d_mat_*_identity
#include "./math_common.h"   // D_MATH_FN, D_MATH_FN_KERNEL, D_MATH_FN_RT
#include "./power.h"         // d_internal_math_ld_pair_sum, _pair_product
#include "./tangent.h"       // d_math_*_atan2 and its _c form
#include "./vec.h"           // d_vec_*_cross, _dot, _norm, _scale, _add
"""

QUAT_TAIL = r"""

#endif  // DJINTERP_C_MATH_QUAT_H
"""

QUAT_ROOT_FORMS = (
    (5, "_c", "D_MATH_FN_KERNEL", "d_math_ld_sqrt_c", "Roots: the _c forms'",
     "What takes a square root and no other real function, on the core's\n"
     "// _c root: a constant expression from C++14."),
    (6, "", "D_MATH_FN_RT", "d_math_ld_sqrt", "Roots: the plain forms'",
     "The same, on the C library's square root, correctly rounded as the\n"
     "// _c root is: the same bits, faster."),
)

_QO = ("@T@*", "_out")
_QQ = ("const @T@*", "_q")
_QA = ("const @T@*", "_a")
_QB = ("const @T@*", "_b")
_QV = ("const @T@*", "_v")
_QM = ("const @T@*", "_m")
QUAT_OVERLOADS = [
    ("D_MATH_FN", "void", "d_quat_multiply_fast", [_QO, _QA, _QB]),
    ("D_MATH_FN", "void", "d_quat_multiply", [_QO, _QA, _QB]),
    ("D_MATH_FN", "void", "d_quat_conjugate", [_QO, _QQ]),
    ("D_MATH_FN", "void", "d_quat_inverse", [_QO, _QQ]),
    ("D_MATH_FN", "void", "d_quat_divide", [_QO, _QA, _QB]),
    ("D_MATH_FN", "void", "d_quat_left_divide", [_QO, _QA, _QB]),
    ("D_MATH_FN", "@T@", "d_quat_norm_c", [_QQ]),
    ("D_MATH_FN_RT", "@T@", "d_quat_norm", [_QQ]),
    ("D_MATH_FN", "void", "d_quat_normalize_c", [_QO, _QQ]),
    ("D_MATH_FN_RT", "void", "d_quat_normalize", [_QO, _QQ]),
    ("D_MATH_FN_RT", "void", "d_quat_normalize_fast", [_QO, _QQ]),
    ("D_MATH_FN", "void", "d_quat_rotate_fast", [_QO, _QQ, _QV]),
    ("D_MATH_FN", "void", "d_quat_rotate", [_QO, _QQ, _QV]),
    ("D_MATH_FN", "void", "d_quat_to_matrix3_fast", [_QO, _QQ]),
    ("D_MATH_FN", "void", "d_quat_to_matrix3", [_QO, _QQ]),
    ("D_MATH_FN", "void", "d_quat_to_matrix4_fast", [_QO, _QQ]),
    ("D_MATH_FN", "void", "d_quat_to_matrix4", [_QO, _QQ]),
]
for _f, _m in (("_c", "D_MATH_FN"), ("", "D_MATH_FN_RT")):
    QUAT_OVERLOADS += [
        (_m, "void", "d_quat_from_matrix3" + _f, [_QO, _QM]),
        (_m, "void", "d_quat_from_matrix4" + _f, [_QO, _QM]),
        (_m, "void", "d_quat_from_two_vectors" + _f, [_QO, _QA, _QB]),
        (_m, "void", "d_quat_from_axis_angle" + _f, [_QO, ("const @T@*", "_axis"), ("@T@", "_angle")]),
        (_m, "void", "d_quat_from_unit_axis_angle" + _f, [_QO, ("const @T@*", "_axis"), ("@T@", "_angle")]),
        (_m, "void", "d_quat_nlerp" + _f, [_QO, _QA, _QB, ("@T@", "_t")]),
        (_m, "void", "d_quat_slerp" + _f, [_QO, _QA, _QB, ("@T@", "_t")]),
        (_m, "void", "d_quat_exp" + _f, [_QO, _QQ]),
        (_m, "void", "d_quat_log" + _f, [_QO, _QQ]),
        (_m, "void", "d_quat_sqrt" + _f, [_QO, _QQ]),
        (_m, "void", "d_quat_pow" + _f, [_QO, _QQ, ("@T@", "_p")]),
    ]


def quat():
    # (a blank line before it, its text): each wrapped to 72 columns
    paragraphs = [
        (False,
         'Quaternions: the C core of math/quaternion.hpp and '
         "math/linear_algebra/quaternion.hpp, and of the language's "
         'quaternions, for float, double and long double.'),
        (False,
         'GENERATED by scripts/gen_c_math_float.py from one text for the '
         'three families: edit the generator, not this file.'),
        (True,
         "A quaternion here is a caller's array [x, y, z, w], x i + y j + "
         'z k + w, the order of glTF, Eigen and GLM, so that a renderer '
         "takes one as it is (S275); the product is Hamilton's, "
         "right-handed, and a rotation active, v' = q v q^-1, q v q* for a "
         'unit q (S276).'),
        (True,
         'Accurate by default (S276): the product, the inverse and the two '
         'quotients, the rotation and its matrix, the norm and '
         "normalization, the quaternion of a rotation matrix (Shepperd's "
         'method) and of two vectors, and nlerp are each one long double '
         'kernel, its parts carried as pairs where they cancel -- every '
         "product of two parts exact by Dekker's, every sum of products "
         "Ogita, Rump and Oishi's Dot2 -- and each family's part the "
         "kernel's rounded once, the operands scaled by powers of 2 where "
         "Dekker's products could overflow or lose their low parts; "
         "float's and double's product, inverse and quotients the same in "
         "double's pairs where double is evaluated in its own type, as SSE "
         "evaluates it, some ten times faster than the x87's long double. "
         "The product's parts are as accurate as their sums computed in "
         "twice the pairs' precision and rounded once. The rotation and "
         'its matrix take any q not 0, its norm cancelling, so that a q a '
         'few steps from unit length names the rotation of q / |q|. What '
         'takes a root and no other real function comes twice, the _c form '
         'a constant expression from C++14 and the plain one on the C '
         "library's root, the two giving the same bits."),
        (True,
         'slerp, exp, log, sqrt and real powers come twice, as the '
         'elementary functions do: the _c form one long double kernel on '
         "the core's _c functions; the plain form the same algorithm in "
         "the family's type on the C library's. slerp along the shorter "
         'arc, its angle 2 atan2(|a - b|, |a + b|), accurate near 0 where '
         'acos(a . b) is not, a at t = 0 and b at t = 1 exactly. exp, log, '
         "sqrt and pow are the complex functions in q's plane, principal "
         'values (S275): f(q) = Re f(w + r i) + u Im f(w + r i), r = |v|, '
         "u = v / r; for v along im alone, cplx.h's function itself, bit "
         'for bit, y and z as they are; log(-1) is pi im. In the _c '
         'kernels r is a pair and f corrected to first order for its low '
         'part, and log and sqrt take q scaled by a power of 2 where it '
         "leaves the range Dekker's products need."),
        (True,
         "The former formulas stay, in the family's type, as the _fast "
         'forms of the product, the rotation, its matrices and '
         "normalization (by a reciprocal square root), for a renderer's "
         "inner loops: their bits are the earlier header's, each part's "
         'error unbounded where products cancel. from_axis_angle and '
         'from_unit_axis_angle are as before.'),
        (True,
         'Where a part is infinite or a NaN: the product as written (the '
         "_fast formula); the quotients a times b's inverse, which is the "
         "conjugate's zeros for an infinite q and NaNs for a zero or NaN "
         "one; NaNs from the rotation, its matrix, Shepperd's method, two "
         'vectors, nlerp and slerp; the direction of the infinities from '
         "normalization, +inf from the norm; and in q's plane, an infinite "
         "r along v's infinities, a NaN part giving NaNs, a part of v that "
         'is 0 staying 0.'),
        (True,
         'The bounds, in steps (float_order.h) of each part from the part '
         'correctly rounded, as tests/djinterp/c/modules/math/t_quat.c checks them '
         'at the points of math_tests_quat_refs.h, computed by mpmath: '
         'points exact in float, which every family takes, at the ends of '
         "each family's range, and at double's and long double's full "
         'width. The product within 1 but where its sum cancels, by u^2 '
         "times its terms' magnitudes (measured 0, but 8 for double where "
         'a sum cancels to 2^-50 of its terms); the inverse, the two '
         'quotients, the rotation and its matrix, the norm, normalization '
         "and Shepperd's method within 1 (measured 0, but 1 for long "
         "double's rotation); two vectors' quaternion and nlerp within 1 "
         'for float and double and 2 for long double (measured 0, 0 and '
         '1); slerp_c within 1 step of its largest part, as its parts are '
         'sums that may cancel, and 2 for long double (measured 0, 0 and '
         '2); exp_c, log_c, sqrt_c and pow_c within 1 for float and double '
         'and 4 for long double (measured 0, and 2 each), a whole power in '
         "q's plane being cplx.h's, carried in pairs (S370). The plain "
         'slerp within 4 steps of its largest part (measured 2); the plain '
         "exp, log, sqrt and pow, which take |v| rounded once in the "
         "family's type, within 12, 4, 4 and 12 steps of the largest part "
         'where |v| is below 16 and |p| below 8 (measured 8, 3, 2 and 5): '
         "exp's and pow's grow with |v| and p, as the cosine of a rounded "
         '|v| does.'),
    ]
    lines = []
    for n, (blank, paragraph) in enumerate(paragraphs):
        # each paragraph wrapped, the first flush, the rest indented two
        # spaces as the banners' paragraphs are
        lead = "" if n == 0 else "  "
        lines += ([""] if blank else []) + textwrap.wrap(
            lead + paragraph, 72, break_long_words=False,
            break_on_hyphens=False)
    text = banner("quat.h", lines, "2026.10.04", "2026.11.07")
    text += "\n" + QUAT_HEAD
    text += QUAT_KERNELS_PURE
    for sub, ks, kq, root, title, prose in QUAT_ROOT_FORMS:
        text += (QUAT_KERNELS_ROOT.replace("@SUB@", str(sub))
                 .replace("@TITLE@", title).replace("@PROSE@", prose)
                 .replace("@KS@", ks).replace("@KQ@", kq)
                 .replace("@SQRT@", root))
    text += QUAT_KERNELS_FUNCTIONS
    for i, (suffix, ctype, prefix, prose) in enumerate(FAMILIES):
        upper = prose.upper()
        forms = ""
        for f, rt in (("_c", ""), ("", "_RT")):
            forms += QUAT_TRIG.replace("@F@", f).replace("@RT@", rt)
        exps = ""
        for fn, doc, params, kargs, cextra, nextra, plain in QUAT_EXP_SPECS:
            exps += (QUAT_EXP.replace("@PLAIN_BODY@", plain.rstrip("\n"))
                     .replace("@FN@", fn).replace("@DOC@", doc)
                     .replace("@PARAMS@", params).replace("@KARGS@", kargs)
                     .replace("@CPLX_EXTRA@", cextra)
                     .replace("@NAN_EXTRA@", nextra))
        body = QUAT_FAMILY.replace("@TRIG_FORMS@", forms)
        body = body.replace("@EXP_FORMS@", exps)
        body = body.replace("@NARROW_D2@",
                            QUAT_NARROW_D2.get(suffix, "").rstrip("\n"))
        body = body.replace("@SECTION@", str(i + 2)).replace("@UPPER@", upper)
        family = quat_d2(instantiate(body, suffix, ctype, prefix, prose),
                         suffix)
        # long double's algebra takes long double's pairs alone: its prose
        # says so where float's and double's names both pairs
        if suffix == "ld":
            for old, new in QUAT_LD_PROSE:
                assert family.count(old) == 1, old
                family = family.replace(old, new)
        text += family
    text += overload_section("5.  ONE NAME FOR EVERY FAMILY (C++)",
                             QUAT_OVERLOADS)
    text += QUAT_TAIL
    return text


QUAT_NARROW_D2 = {"f": r"""// d_internal_quat_f_narrow_d2
//   _out = each of _p's _n double pairs rounded once to float, as
// d_internal_math_f_round_pair rounds a long double one: the conversion,
// and where it lands on a midpoint between two floats the low part's sign;
// a finite sum past float's range an infinity, but for the midpoint above
// the largest finite float with the sum below it.
D_MATH_FN void
d_internal_quat_f_narrow_d2(
    float*                                _out,
    const struct dj_internal_math_d_pair* _p,
    size_t                                _n
)
{
    size_t i = 0u;

    // each pair, rounded once
    for (i = 0u; i < _n; ++i)
    {
        const double hi    = _p[i].high;
        const double lo    = _p[i].low;
        const float  near  = (float)hi;
        const double rest  = hi - (double)near;
        float        value = near;

        // a finite sum the conversion took past the largest finite float
        if ( ( (near > FLT_MAX) ||
               (near < -FLT_MAX) ) &&
             (hi <= DBL_MAX) &&
             (hi >= -DBL_MAX) )
        {
            const double gap = (double)FLT_MAX -
                               (double)d_math_f_next_down(FLT_MAX);
            const double mid = (near > 0.0f) ?
                                   ((double)FLT_MAX + (gap / 2.0)) :
                                   (-(double)FLT_MAX - (gap / 2.0));

            // the midpoint above it, the sum below: the largest float
            if ( (hi == mid) &&
                 ( (near > 0.0f) ? (lo < 0.0) : (lo > 0.0) ) )
            {
                value = (near > 0.0f) ? FLT_MAX : -FLT_MAX;
            }
        }
        // hi rounded down to near, from the midpoint, the sum above it
        else if ( (rest > 0.0) &&
                  (lo > 0.0) )
        {
            const float up = d_math_f_next_up(near);

            if ((rest + rest) == ((double)up - (double)near))
            {
                value = up;
            }
        }
        // hi rounded up to near, from the midpoint, the sum below it
        else if ( (rest < 0.0) &&
                  (lo < 0.0) )
        {
            const float down = d_math_f_next_down(near);

            if ((rest + rest) == ((double)down - (double)near))
            {
                value = down;
            }
        }

        _out[i] = value;
    }

    return;
}
""", "d": r"""// d_internal_quat_d_narrow_d2
//   _out = each of _p's _n pairs' high part, the sum rounded once.
D_MATH_FN void
d_internal_quat_d_narrow_d2(
    double*                               _out,
    const struct dj_internal_math_d_pair* _p,
    size_t                                _n
)
{
    size_t i = 0u;

    // each pair's high part
    for (i = 0u; i < _n; ++i)
    {
        _out[i] = _p[i].high;
    }

    return;
}
"""}


QUAT_LD_PROSE = (
    ("// the sum of its four products as Dot2 sums them, rounded once: in "
     "double's\n// pairs where double is evaluated in its own type (section "
     "1.3), long\n// double's otherwise (section 1.2); where an operand has "
     "an infinite or NaN\n// part, the formula as written "
     "(d_quat_ld_multiply_fast).",
     "// the sum of its four products as Dot2 sums them in long double's "
     "pairs\n// (section 1.2), rounded once; where an operand has an "
     "infinite or NaN\n// part, the formula as written "
     "(d_quat_ld_multiply_fast)."),
    ("// once, in double's pairs or long double's as d_quat_ld_multiply "
     "takes\n// them.",
     "// once, in long double's pairs as d_quat_ld_multiply takes them."),
)


def quat_d2(text, suffix):
    """text with each block "#if @D2@ ... #else  // @D2@ ... #endif  // @D2@"
    made the test of double's evaluation for float and double, whose
    algebra takes double's pairs where double is evaluated in its own type;
    for long double its #else part alone (cplx_wide's way)."""
    keep = suffix in ("f", "d")
    out = []
    state = None
    for line in text.split("\n"):
        if line == "#if @D2@":
            state = "d2"
            if keep:
                out.append("#if !D_MATH_DBL_EVAL_WIDE")
            continue
        if line == "#else  // @D2@":
            state = "else"
            if keep:
                out.append("#else  // !D_MATH_DBL_EVAL_WIDE")
            continue
        if line == "#endif  // @D2@":
            state = None
            if keep:
                out.append("#endif  // !D_MATH_DBL_EVAL_WIDE")
            continue
        if (state == "d2") and (not keep):
            continue
        out.append(line)
    return "\n".join(out)






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
        "Coordinate systems: the C core of the math/coordinate headers -- polar,",
        "cylindrical and spherical conversions, distances, measures and canonical",
        "forms -- for float, double and long double.",
        "  GENERATED by scripts/gen_c_math_float.py from one text for the three",
        "families: edit the generator, not this file.",
        "",
        "  A point here is a caller's array; the faces' point type is",
        "linalg::vector, whose storage it is. What needs a root, a sine or an",
        "arc-tangent comes twice, as the elementary functions do: the _c form a",
        "constant expression from C++14, the plain one on the C library's. The",
        "angles' canonical forms are exact, so one form serves.",
    ]
    text = banner("coord.h", lines, DATE, "2026.11.07")
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
    d_vec_@S@_cross(h, _dir, edge2);
    a = d_vec_@S@_dot(edge1, h, 3u);

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
        u = f * d_vec_@S@_dot(s, h, 3u);

        // outside the edge opposite v2 or v1
        if ( (u < (@T@)0) ||
             (u > (@T@)1) )
        {
            return false;
        }

        d_vec_@S@_cross(q, s, edge1);
        v = f * d_vec_@S@_dot(_dir, q, 3u);

        // outside the remaining edges
        if ( (v < (@T@)0) ||
             ((u + v) > (@T@)1) )
        {
            return false;
        }

        t = f * d_vec_@S@_dot(edge2, q, 3u);

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
        "Intersections: the C core of the geometry headers' ray tests -- a ray",
        "against an axis-aligned box and against a triangle -- for float, double",
        "and long double.",
        "  GENERATED by scripts/gen_c_math_float.py from one text for the three",
        "families: edit the generator, not this file.",
        "",
        "  Points and directions are a caller's 3-arrays, the faces' vectors'",
        "storage. Each test is a constant expression from C++14.",
    ]
    text = banner("isect.h", lines, DATE, "2026.11.07")
    text += "\n" + ISECT_HEAD
    for i, (suffix, ctype, prefix, prose) in enumerate(FAMILIES):
        upper = prose.upper()
        body = ISECT_TEMPLATE.replace("@SECTION@", str(i + 1)).replace("@UPPER@", upper)
        text += instantiate(body, suffix, ctype, prefix, prose)
    text += overload_section("4.  ONE NAME FOR EVERY FAMILY (C++)", ISECT_OVERLOADS)
    text += ISECT_TAIL
    return text



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
// segment straddles the ray's line (one end above, the other not -- a
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
        "The plane: the C core of the 2D shape header's algorithms -- the +x",
        "ray's crossing test, and a polygon's containment, area and centroid --",
        "for float, double and long double.",
        "  GENERATED by scripts/gen_c_math_float.py from one text for the three",
        "families: edit the generator, not this file.",
        "",
        "  Points are a caller's (x, y) pairs and a polygon's vertices",
        "interleaved, the faces' vectors' storage converted to Cartesian. Each",
        "kernel is a constant expression from C++14.",
    ]
    text = banner("plane.h", lines, DATE, "2026.11.07")
    text += "\n" + PLANE_HEAD
    for i, (suffix, ctype, prefix, prose) in enumerate(FAMILIES):
        upper = prose.upper()
        body = PLANE_TEMPLATE.replace("@SECTION@", str(i + 1)).replace("@UPPER@", upper)
        text += instantiate(body, suffix, ctype, prefix, prose)
    text += overload_section("4.  ONE NAME FOR EVERY FAMILY (C++)", PLANE_OVERLOADS)
    text += PLANE_TAIL
    return text



# -----------------------------------------------------------------------------
# decomp.h
# -----------------------------------------------------------------------------

DECOMP_TEMPLATE = r"""
//==============================================================================
// @SECTION@.  THE @UPPER@ FAMILY (_@S@)
//==============================================================================
// Matrices are a caller's row-major arrays: element (i, j) of an r-by-c
// matrix at [i c + j]. The algorithms are the decomposition header's own,
// in its order, one operation per statement.


// @SECTION@.1    LU, with partial pivoting
//------------------------------------------------------------------------------
// Exact in its arithmetic's order and needing no root: one form, a constant
// expression from C++14.

// d_decomp_@S@_lu
//   factors the n-by-n _lu in place into P A = L U -- L's multipliers below
// the diagonal, U on and above -- choosing each column's largest pivot (the
// first, among equals). _perm receives the row permutation and *_sign its
// parity (+1 or -1). Returns whether some pivot column was zero, the matrix
// singular; that column is then skipped, U's diagonal zero there.
D_MATH_FN bool
d_decomp_@S@_lu(
    @T@*    _lu,
    size_t  _n,
    size_t* _perm,
    @T@*    _sign
)
{
    bool   singular = false;
    size_t i        = 0u;
    size_t col      = 0u;

    *_sign = (@T@)1;

    for (i = 0u; i < _n; ++i)
    {
        _perm[i] = i;
    }

    for (col = 0u; col < _n; ++col)
    {
        size_t pivot = col;
        @T@    maxv  = d_internal_math_@S@_abs(_lu[(col * _n) + col]);
        size_t r     = 0u;

        // the largest magnitude in the column, at or below the diagonal
        for (r = col + 1u; r < _n; ++r)
        {
            const @T@ v = d_internal_math_@S@_abs(_lu[(r * _n) + col]);

            if (v > maxv)
            {
                maxv  = v;
                pivot = r;
            }
        }

        // a zero column: singular, and nothing to eliminate
        if (_lu[(pivot * _n) + col] == (@T@)0)
        {
            singular = true;

            continue;
        }

        // the pivot row up
        if (pivot != col)
        {
            size_t c = 0u;

            for (c = 0u; c < _n; ++c)
            {
                const @T@ tmp = _lu[(col * _n) + c];

                _lu[(col * _n) + c]   = _lu[(pivot * _n) + c];
                _lu[(pivot * _n) + c] = tmp;
            }

            {
                const size_t tmp = _perm[col];

                _perm[col]   = _perm[pivot];
                _perm[pivot] = tmp;
            }

            *_sign = -*_sign;
        }

        {
            const @T@ pivot_value = _lu[(col * _n) + col];

            for (r = col + 1u; r < _n; ++r)
            {
                const @T@ f = _lu[(r * _n) + col] / pivot_value;
                size_t    c = 0u;

                _lu[(r * _n) + col] = f;

                for (c = col + 1u; c < _n; ++c)
                {
                    const @T@ prod = f * _lu[(col * _n) + c];

                    _lu[(r * _n) + c] = _lu[(r * _n) + c] - prod;
                }
            }
        }
    }

    return singular;
}

// d_decomp_@S@_lu_solve
//   solves A _x = _b from d_decomp_@S@_lu's factors: _b permuted, forward
// through L (unit diagonal), back through U. _x and _b must not overlap.
D_MATH_FN void
d_decomp_@S@_lu_solve(
    const @T@*    _lu,
    const size_t* _perm,
    size_t        _n,
    const @T@*    _b,
    @T@*          _x
)
{
    size_t i = 0u;

    // forward: y into _x
    for (i = 0u; i < _n; ++i)
    {
        @T@    s = _b[_perm[i]];
        size_t k = 0u;

        for (k = 0u; k < i; ++k)
        {
            const @T@ prod = _lu[(i * _n) + k] * _x[k];

            s = s - prod;
        }

        _x[i] = s;
    }

    // back: _x over y, last row first
    for (i = _n; i-- > 0u; )
    {
        @T@    s = _x[i];
        size_t k = 0u;

        for (k = i + 1u; k < _n; ++k)
        {
            const @T@ prod = _lu[(i * _n) + k] * _x[k];

            s = s - prod;
        }

        _x[i] = s / _lu[(i * _n) + i];
    }

    return;
}

// d_decomp_@S@_lu_det
//   the determinant from the factors: the parity times U's diagonal, in
// order.
D_MATH_FN @T@
d_decomp_@S@_lu_det(
    const @T@* _lu,
    size_t     _n,
    @T@        _sign
)
{
    @T@    d = _sign;
    size_t i = 0u;

    for (i = 0u; i < _n; ++i)
    {
        d = d * _lu[(i * _n) + i];
    }

    return d;
}

// d_decomp_@S@_cholesky_solve
//   solves A _x = _b from a Cholesky factor _l (A = L L^T): forward through
// L, back through L^T. _x and _b must not overlap.
D_MATH_FN void
d_decomp_@S@_cholesky_solve(
    const @T@* _l,
    size_t     _n,
    const @T@* _b,
    @T@*       _x
)
{
    size_t i = 0u;

    // forward: y into _x
    for (i = 0u; i < _n; ++i)
    {
        @T@    s = _b[i];
        size_t k = 0u;

        for (k = 0u; k < i; ++k)
        {
            const @T@ prod = _l[(i * _n) + k] * _x[k];

            s = s - prod;
        }

        _x[i] = s / _l[(i * _n) + i];
    }

    // back, through L^T: _x over y, last row first
    for (i = _n; i-- > 0u; )
    {
        @T@    s = _x[i];
        size_t k = 0u;

        for (k = i + 1u; k < _n; ++k)
        {
            const @T@ prod = _l[(k * _n) + i] * _x[k];

            s = s - prod;
        }

        _x[i] = s / _l[(i * _n) + i];
    }

    return;
}

// d_decomp_@S@_cholesky_det
//   the determinant from a Cholesky factor: L's diagonal product, squared.
D_MATH_FN @T@
d_decomp_@S@_cholesky_det(
    const @T@* _l,
    size_t     _n
)
{
    @T@    d = (@T@)1;
    size_t i = 0u;

    for (i = 0u; i < _n; ++i)
    {
        d = d * _l[(i * _n) + i];
    }

    return d * d;
}

// d_decomp_@S@_forward_sub
//   solves L _x = _b for the n-by-n lower triangle of _l (its diagonal
// general, not unit), row by row. _x and _b must not overlap.
D_MATH_FN void
d_decomp_@S@_forward_sub(
    const @T@* _l,
    size_t     _n,
    const @T@* _b,
    @T@*       _x
)
{
    size_t i = 0u;

    for (i = 0u; i < _n; ++i)
    {
        @T@    s = _b[i];
        size_t k = 0u;

        for (k = 0u; k < i; ++k)
        {
            const @T@ prod = _l[(i * _n) + k] * _x[k];

            s = s - prod;
        }

        _x[i] = s / _l[(i * _n) + i];
    }

    return;
}

// d_decomp_@S@_back_sub
//   solves U _x = _b for the n-by-n upper triangle of _u (its diagonal
// general), last row first. _u may be the first n rows of a taller
// row-major matrix of n columns, as an R from QR is. _x and _b must not
// overlap.
D_MATH_FN void
d_decomp_@S@_back_sub(
    const @T@* _u,
    size_t     _n,
    const @T@* _b,
    @T@*       _x
)
{
    size_t i = 0u;

    for (i = _n; i-- > 0u; )
    {
        @T@    s = _b[i];
        size_t k = 0u;

        for (k = i + 1u; k < _n; ++k)
        {
            const @T@ prod = _u[(i * _n) + k] * _x[k];

            s = s - prod;
        }

        _x[i] = s / _u[(i * _n) + i];
    }

    return;
}

// d_decomp_@S@_qt_mul
//   _c = (Q^T _b)[0:n] for the m-by-m _q: each of the first n columns of Q
// against _b, summed over the rows in order.
D_MATH_FN void
d_decomp_@S@_qt_mul(
    const @T@* _q,
    size_t     _m,
    size_t     _n,
    const @T@* _b,
    @T@*       _c
)
{
    size_t i = 0u;

    for (i = 0u; i < _n; ++i)
    {
        @T@    s = (@T@)0;
        size_t k = 0u;

        for (k = 0u; k < _m; ++k)
        {
            const @T@ prod = _q[(k * _m) + i] * _b[k];

            s = s + prod;
        }

        _c[i] = s;
    }

    return;
}

// @SECTION@.2    QR, the SVD, the symmetric eigensolver and Cholesky
//------------------------------------------------------------------------------
// Each takes square roots, and comes twice: the _c form a constant
// expression from C++14, the plain one on the C library's root.
@FORMS@
// @SECTION@.3    The eigenvalues and eigenvectors of a general matrix
//------------------------------------------------------------------------------
// LAPACK's path, as its dgeev takes it (S269): the matrix scaled by a power
// of two so that its largest magnitude lies in [1, 2), balanced (dgebal),
// reduced to upper Hessenberg form by Householder reflectors (dgehd2), its
// real Schur form found by Francis's implicit double-shift QR (dlahqr), its
// blocks of two brought to standard form (dlanv2), so that each complex
// pair is exactly conjugate and each real eigenvalue's imaginary part
// exactly +0; then the eigenvectors from the Schur form by back-substitution
// (dtrevc), back-transformed (dgebak). The eigenvalues are cplx.h's complex
// numbers, ascending by real part, then by imaginary part. The scaling is
// exact, so 2^k A has 2^k times A's eigenvalues, bit for bit, barring
// overflow and underflow. The rotation Hessenberg form and an eigenvector
// by inverse iteration, for one eigenvalue, stay beside them. What takes a
// root comes twice, as above: the two forms the same bit for bit, as their
// roots are correctly rounded both.

// gcc's -Warray-bounds is set aside for this section's kernels, and restored
// after it: where a caller's matrix is a constant gcc folds, its value ranges
// take loops below for reachable with indices they never take (gcc 13 at
// -O2, a 2-by-2 rotation in main), a false warning, and under -Werror an
// error. Every index below lies within the caller's n by n.
#if ( defined(__GNUC__) &&                                                    \
      !defined(__clang__) )
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Warray-bounds"
#endif

// dj_internal_decomp_@S@_rotation
//   struct: a plane rotation and the length it leaves: c x + s y = r and
// c y - s x = 0, for the pair (x, y) it was made from.
struct dj_internal_decomp_@S@_rotation
{
    @T@ c;  // the cosine
    @T@ s;  // the sine
    @T@ r;  // the pair's length, sqrt(x^2 + y^2) >= 0
};

// dj_internal_decomp_@S@_scaling
//   struct: how a matrix is brought to a largest magnitude in [1, 2): times
// pre, then divided by power, both powers of two, so both steps are exact
// (but for an entry that underflows, which is below the largest's rounding).
struct dj_internal_decomp_@S@_scaling
{
    @T@  pre;     // 2^P for a subnormal largest magnitude, else 1
    @T@  power;   // the largest magnitude's binade, 2^E, times pre
    bool finite;  // whether every entry is finite
    bool zero;    // whether every entry is zero
};

// d_internal_decomp_@S@_scaling_of
//   the scaling of the _count entries of _a: their largest magnitude's
// binade, found exactly (float_order.h), after a lift by 2^P where that
// magnitude is subnormal. Not finite, or zero, when an entry is NaN or
// infinite, or every entry is zero; pre and power are then 1.
D_MATH_FN struct dj_internal_decomp_@S@_scaling
d_internal_decomp_@S@_scaling_of(
    const @T@* _a,
    size_t     _count
)
{
    struct dj_internal_decomp_@S@_scaling result =
        { (@T@)1, (@T@)1, true, false };
    @T@    largest = (@T@)0;
    size_t i       = 0u;

    for (i = 0u; i < _count; ++i)
    {
        const @T@ v = d_internal_math_@S@_abs(_a[i]);

        // a NaN, or an infinity: nothing to scale
        if ( (v != v) ||
             (v > @MAX@) )
        {
            result.finite = false;

            return result;
        }

        if (v > largest)
        {
            largest = v;
        }
    }

    // every entry zero
    if (largest == (@T@)0)
    {
        result.zero = true;

        return result;
    }

    // a subnormal largest: lifted into the normal range first
    if (largest < @MIN@)
    {
        result.pre = d_internal_math_@S@_pow2(@P@);
        largest    = largest * result.pre;
    }

    {
        const struct dj_internal_math_@S@_binade b =
            d_internal_math_@S@_binade_of(largest);

        result.power = b.power;
    }

    return result;
}

// d_decomp_@S@_balance_back
//   undoes d_decomp_@S@_balance on the n-by-_m row-major _v, whose columns
// are right eigenvectors of the balanced matrix, as LAPACK's dgebak does:
// each row from _ilo to _ihi multiplied by its scaling factor, exactly, then
// the permutations undone, the last first -- the columns pushed to the top,
// then the rows pushed to the bottom -- each a swap of two rows. The columns
// are then eigenvectors of the matrix as it was.
D_MATH_FN_KERNEL void
d_decomp_@S@_balance_back(
    @T@*          _v,
    size_t        _n,
    size_t        _m,
    const size_t* _perm,
    const @T@*    _scale,
    size_t        _ilo,
    size_t        _ihi
)
{
    size_t i  = 0u;
    size_t c  = 0u;
    size_t ii = 0u;

    // nothing to undo
    if (_n == 0u)
    {
        return;
    }

    // the scaling
    for (i = _ilo; i <= _ihi; ++i)
    {
        for (c = 0u; c < _m; ++c)
        {
            _v[(i * _m) + c] = _v[(i * _m) + c] * _scale[i];
        }
    }

    // the permutations, the last first
    for (ii = 0u; ii < _n; ++ii)
    {
        size_t row   = ii;
        size_t other = 0u;

        // inside the balanced block: no swap
        if ( (row >= _ilo) &&
             (row <= _ihi) )
        {
            continue;
        }

        // a column pushed to the top: the last pushed first
        if (row < _ilo)
        {
            row = _ilo - 1u - ii;
        }

        other = _perm[row];

        // swapped with itself: nothing to undo
        if (other == row)
        {
            continue;
        }

        for (c = 0u; c < _m; ++c)
        {
            const @T@ tmp = _v[(row * _m) + c];

            _v[(row * _m) + c]   = _v[(other * _m) + c];
            _v[(other * _m) + c] = tmp;
        }
    }

    return;
}

// d_internal_decomp_@S@_swap_isolated
//   the swap balancing makes to isolate an eigenvalue: columns _i and _j in
// rows 0 to _l, then rows _i and _j in columns _k to n - 1 -- a similarity
// by a permutation.
D_MATH_FN void
d_internal_decomp_@S@_swap_isolated(
    @T@*   _a,
    size_t _n,
    size_t _i,
    size_t _j,
    size_t _k,
    size_t _l
)
{
    size_t r = 0u;

    for (r = 0u; r <= _l; ++r)
    {
        const @T@ tmp = _a[(r * _n) + _i];

        _a[(r * _n) + _i] = _a[(r * _n) + _j];
        _a[(r * _n) + _j] = tmp;
    }

    for (r = _k; r < _n; ++r)
    {
        const @T@ tmp = _a[(_i * _n) + r];

        _a[(_i * _n) + r] = _a[(_j * _n) + r];
        _a[(_j * _n) + r] = tmp;
    }

    return;
}

// d_internal_decomp_@S@_small_solve
//   solves the 1-by-1 or 2-by-2 system (T - w I) X = scale B, as LAPACK's
// dlaln2 does for dtrevc's right eigenvectors: T the block of _na rows at
// _t (row stride _n), w = _wr, or _wr + _wi i where _nw is 2, B's real
// parts in _br and its imaginary ones in _bi, X likewise into _xr and _xi.
// A pivot smaller than _smin is taken as _smin, so that a nearly singular
// system perturbs rather than fails; *_scale, at most 1, is chosen so that
// X does not overflow, and *_xnorm receives X's largest magnitude (|re| +
// |im| where complex). Gaussian elimination with complete pivoting for 2 by
// 2; complex quotients are cplx.h's.
D_MATH_FN_KERNEL void
d_internal_decomp_@S@_small_solve(
    size_t     _na,
    size_t     _nw,
    @T@        _smin,
    const @T@* _t,
    size_t     _n,
    const @T@* _br,
    const @T@* _bi,
    @T@        _wr,
    @T@        _wi,
    @T@*       _xr,
    @T@*       _xi,
    @T@*       _scale,
    @T@*       _xnorm
)
{
    // the pivots' orders for each choice of largest entry, by columns: an
    // entry's place in (c11, c21, c12, c22) after the pivot is moved to c11
    const size_t pivots[4][4] = {
        { 0u, 1u, 2u, 3u },
        { 1u, 0u, 3u, 2u },
        { 2u, 3u, 0u, 1u },
        { 3u, 2u, 1u, 0u }
    };
    const bool   rswap[4]     = { false, true, false, true };
    const bool   zswap[4]     = { false, false, true, true };
    const @T@ smlnum = (@T@)2 * @MIN@;
    const @T@ bignum = (@T@)1 / smlnum;
    const @T@ smini  = (_smin > smlnum) ? _smin : smlnum;

    *_scale = (@T@)1;

    // 1 by 1, real: x = b / (t - wr)
    if ( (_na == 1u) &&
         (_nw == 1u) )
    {
        @T@       csr   = _t[0] - _wr;
        @T@       cnorm = d_internal_math_@S@_abs(csr);
        const @T@ bnorm = d_internal_math_@S@_abs(_br[0]);

        // a pivot below smin: smin
        if (cnorm < smini)
        {
            csr   = smini;
            cnorm = smini;
        }

        // b / c would overflow: scaled
        if ( (cnorm < (@T@)1) &&
             (bnorm > (@T@)1) )
        {
            const @T@ bound = bignum * cnorm;

            if (bnorm > bound)
            {
                *_scale = (@T@)1 / bnorm;
            }
        }

        {
            const @T@ scaled = _br[0] * *_scale;

            _xr[0] = scaled / csr;
        }

        *_xnorm = d_internal_math_@S@_abs(_xr[0]);

        return;
    }

    // 1 by 1, complex: x = b / (t - wr - wi i)
    if (_na == 1u)
    {
        @T@       csr   = _t[0] - _wr;
        @T@       csi   = -_wi;
        const @T@ c1    = d_internal_math_@S@_abs(csr);
        const @T@ c2    = d_internal_math_@S@_abs(csi);
        @T@       cnorm = c1 + c2;
        const @T@ b1    = d_internal_math_@S@_abs(_br[0]);
        const @T@ b2    = d_internal_math_@S@_abs(_bi[0]);
        const @T@ bnorm = b1 + b2;

        // a pivot below smin: smin
        if (cnorm < smini)
        {
            csr   = smini;
            csi   = (@T@)0;
            cnorm = smini;
        }

        // b / c would overflow: scaled
        if ( (cnorm < (@T@)1) &&
             (bnorm > (@T@)1) )
        {
            const @T@ bound = bignum * cnorm;

            if (bnorm > bound)
            {
                *_scale = (@T@)1 / bnorm;
            }
        }

        {
            const @T@ sr = *_scale * _br[0];
            const @T@ si = *_scale * _bi[0];
            const struct dj_cplx_@S@ q =
                d_cplx_@S@_div(d_cplx_@S@_make(sr, si),
                               d_cplx_@S@_make(csr, csi));
            const @T@ x1 = d_internal_math_@S@_abs(q.part[0]);
            const @T@ x2 = d_internal_math_@S@_abs(q.part[1]);

            _xr[0]  = q.part[0];
            _xi[0]  = q.part[1];
            *_xnorm = x1 + x2;
        }

        return;
    }

    {
        // C = T - w I, by columns: c11, c21, c12, c22
        const @T@ crv[4] = { _t[0] - _wr, _t[_n], _t[1], _t[_n + 1u] - _wr };
        const @T@ civ[4] = { -_wi, (@T@)0, (@T@)0, -_wi };
        @T@       cmax   = (@T@)0;
        size_t    icmax  = 0u;
        size_t    j      = 0u;

        // the largest entry, |re| + |im|: the first among equals
        for (j = 0u; j < 4u; ++j)
        {
            const @T@ m1 = d_internal_math_@S@_abs(crv[j]);
            const @T@ m2 = (_nw == 2u) ? d_internal_math_@S@_abs(civ[j])
                                       : (@T@)0;
            const @T@ m  = m1 + m2;

            if (m > cmax)
            {
                cmax  = m;
                icmax = j;
            }
        }

        // C below smin: smin times the identity
        if (cmax < smini)
        {
            const @T@ r1    = d_internal_math_@S@_abs(_br[0]);
            const @T@ r2    = d_internal_math_@S@_abs(_br[1]);
            const @T@ i1    = (_nw == 2u) ? d_internal_math_@S@_abs(_bi[0])
                                          : (@T@)0;
            const @T@ i2    = (_nw == 2u) ? d_internal_math_@S@_abs(_bi[1])
                                          : (@T@)0;
            const @T@ s1    = r1 + i1;
            const @T@ s2    = r2 + i2;
            const @T@ bnorm = (s1 > s2) ? s1 : s2;
            @T@       temp  = (@T@)0;

            if ( (smini < (@T@)1) &&
                 (bnorm > (@T@)1) )
            {
                const @T@ bound = bignum * smini;

                if (bnorm > bound)
                {
                    *_scale = (@T@)1 / bnorm;
                }
            }

            temp   = *_scale / smini;
            _xr[0] = temp * _br[0];
            _xr[1] = temp * _br[1];

            if (_nw == 2u)
            {
                _xi[0] = temp * _bi[0];
                _xi[1] = temp * _bi[1];
            }

            *_xnorm = temp * bnorm;

            return;
        }

        // real: elimination with complete pivoting
        if (_nw == 1u)
        {
            const @T@ ur11  = crv[icmax];
            const @T@ cr21  = crv[pivots[icmax][1]];
            const @T@ ur12  = crv[pivots[icmax][2]];
            const @T@ cr22  = crv[pivots[icmax][3]];
            const @T@ ur11r = (@T@)1 / ur11;
            const @T@ lr21  = ur11r * cr21;
            const @T@ cross = ur12 * lr21;
            @T@       ur22  = cr22 - cross;
            @T@       br1   = rswap[icmax] ? _br[1] : _br[0];
            @T@       br2   = rswap[icmax] ? _br[0] : _br[1];
            @T@       bbnd  = (@T@)0;
            @T@       xr1   = (@T@)0;
            @T@       xr2   = (@T@)0;

            // the second pivot below smin: smin
            if (d_internal_math_@S@_abs(ur22) < smini)
            {
                ur22 = smini;
            }

            {
                const @T@ l = lr21 * br1;

                br2 = br2 - l;
            }

            {
                const @T@ ratio = ur22 * ur11r;
                const @T@ p     = br1 * ratio;
                const @T@ a1    = d_internal_math_@S@_abs(p);
                const @T@ a2    = d_internal_math_@S@_abs(br2);

                bbnd = (a1 > a2) ? a1 : a2;
            }

            // the solution would overflow: scaled
            if ( (bbnd > (@T@)1) &&
                 (d_internal_math_@S@_abs(ur22) < (@T@)1) )
            {
                const @T@ bound = bignum * d_internal_math_@S@_abs(ur22);

                if (bbnd >= bound)
                {
                    *_scale = (@T@)1 / bbnd;
                }
            }

            {
                const @T@ s2 = br2 * *_scale;
                const @T@ s1 = *_scale * br1;
                const @T@ f1 = s1 * ur11r;
                const @T@ f2 = ur11r * ur12;

                xr2 = s2 / ur22;

                {
                    const @T@ g = xr2 * f2;

                    xr1 = f1 - g;
                }
            }

            _xr[0] = zswap[icmax] ? xr2 : xr1;
            _xr[1] = zswap[icmax] ? xr1 : xr2;

            {
                const @T@ a1 = d_internal_math_@S@_abs(xr1);
                const @T@ a2 = d_internal_math_@S@_abs(xr2);

                *_xnorm = (a1 > a2) ? a1 : a2;
            }

            // |C| |X| would overflow: scaled again
            if ( (*_xnorm > (@T@)1) &&
                 (cmax > (@T@)1) )
            {
                const @T@ bound = bignum / cmax;

                if (*_xnorm > bound)
                {
                    const @T@ temp = cmax / bignum;

                    _xr[0]  = temp * _xr[0];
                    _xr[1]  = temp * _xr[1];
                    *_xnorm = temp * *_xnorm;
                    *_scale = temp * *_scale;
                }
            }

            return;
        }

        // complex: elimination with complete pivoting
        {
            const @T@ ur11  = crv[icmax];
            const @T@ ui11  = civ[icmax];
            const @T@ cr21  = crv[pivots[icmax][1]];
            const @T@ ci21  = civ[pivots[icmax][1]];
            const @T@ ur12  = crv[pivots[icmax][2]];
            const @T@ ui12  = civ[pivots[icmax][2]];
            const @T@ cr22  = crv[pivots[icmax][3]];
            const @T@ ci22  = civ[pivots[icmax][3]];
            @T@       ur11r = (@T@)0;
            @T@       ui11r = (@T@)0;
            @T@       lr21  = (@T@)0;
            @T@       li21  = (@T@)0;
            @T@       ur12s = (@T@)0;
            @T@       ui12s = (@T@)0;
            @T@       ur22  = (@T@)0;
            @T@       ui22  = (@T@)0;
            @T@       u22abs = (@T@)0;
            @T@       br1   = rswap[icmax] ? _br[1] : _br[0];
            @T@       br2   = rswap[icmax] ? _br[0] : _br[1];
            @T@       bi1   = rswap[icmax] ? _bi[1] : _bi[0];
            @T@       bi2   = rswap[icmax] ? _bi[0] : _bi[1];
            @T@       bbnd  = (@T@)0;
            @T@       xr1   = (@T@)0;
            @T@       xi1   = (@T@)0;
            @T@       xr2   = (@T@)0;
            @T@       xi2   = (@T@)0;

            // the pivot on the diagonal: its off-diagonal partners real
            if ( (icmax == 0u) ||
                 (icmax == 3u) )
            {
                // 1 / (ur11 + ui11 i), the larger part divided out first
                if (d_internal_math_@S@_abs(ur11) >
                    d_internal_math_@S@_abs(ui11))
                {
                    const @T@ temp = ui11 / ur11;
                    const @T@ tt   = temp * temp;
                    const @T@ one  = (@T@)1 + tt;
                    const @T@ den  = ur11 * one;

                    ur11r = (@T@)1 / den;
                    ui11r = -temp * ur11r;
                }
                else
                {
                    const @T@ temp = ur11 / ui11;
                    const @T@ tt   = temp * temp;
                    const @T@ one  = (@T@)1 + tt;
                    const @T@ den  = ui11 * one;

                    ui11r = (@T@)-1 / den;
                    ur11r = -temp * ui11r;
                }

                lr21  = cr21 * ur11r;
                li21  = cr21 * ui11r;
                ur12s = ur12 * ur11r;
                ui12s = ur12 * ui11r;

                {
                    const @T@ p1 = ur12 * lr21;
                    const @T@ p2 = ur12 * li21;

                    ur22 = cr22 - p1;
                    ui22 = ci22 - p2;
                }
            }
            // the pivot off the diagonal: the diagonal real
            else
            {
                ur11r = (@T@)1 / ur11;
                ui11r = (@T@)0;
                lr21  = cr21 * ur11r;
                li21  = ci21 * ur11r;
                ur12s = ur12 * ur11r;
                ui12s = ui12 * ur11r;

                {
                    const @T@ p1 = ur12 * lr21;
                    const @T@ p2 = ui12 * li21;
                    const @T@ p3 = ur12 * li21;
                    const @T@ p4 = ui12 * lr21;
                    const @T@ d1 = cr22 - p1;
                    const @T@ m3 = -p3;

                    ur22 = d1 + p2;
                    ui22 = m3 - p4;
                }
            }

            {
                const @T@ a1 = d_internal_math_@S@_abs(ur22);
                const @T@ a2 = d_internal_math_@S@_abs(ui22);

                u22abs = a1 + a2;
            }

            // the second pivot below smin: smin
            if (u22abs < smini)
            {
                ur22   = smini;
                ui22   = (@T@)0;
            }

            {
                const @T@ p1 = lr21 * br1;
                const @T@ p2 = li21 * bi1;
                const @T@ p3 = li21 * br1;
                const @T@ p4 = lr21 * bi1;
                const @T@ d1 = br2 - p1;
                const @T@ d2 = bi2 - p3;

                br2 = d1 + p2;
                bi2 = d2 - p4;
            }

            {
                const @T@ a1  = d_internal_math_@S@_abs(br1);
                const @T@ a2  = d_internal_math_@S@_abs(bi1);
                const @T@ a3  = d_internal_math_@S@_abs(ur11r);
                const @T@ a4  = d_internal_math_@S@_abs(ui11r);
                const @T@ a5  = d_internal_math_@S@_abs(br2);
                const @T@ a6  = d_internal_math_@S@_abs(bi2);
                const @T@ s12 = a1 + a2;
                const @T@ s34 = a3 + a4;
                const @T@ u   = u22abs * s34;
                const @T@ e1  = s12 * u;
                const @T@ e2  = a5 + a6;

                bbnd = (e1 > e2) ? e1 : e2;
            }

            // the solution would overflow: scaled
            if ( (bbnd > (@T@)1) &&
                 (u22abs < (@T@)1) )
            {
                const @T@ bound = bignum * u22abs;

                if (bbnd >= bound)
                {
                    *_scale = (@T@)1 / bbnd;
                    br1     = *_scale * br1;
                    bi1     = *_scale * bi1;
                    br2     = *_scale * br2;
                    bi2     = *_scale * bi2;
                }
            }

            {
                const struct dj_cplx_@S@ q =
                    d_cplx_@S@_div(d_cplx_@S@_make(br2, bi2),
                                   d_cplx_@S@_make(ur22, ui22));

                xr2 = q.part[0];
                xi2 = q.part[1];
            }

            {
                const @T@ p1 = ur11r * br1;
                const @T@ p2 = ui11r * bi1;
                const @T@ p3 = ur12s * xr2;
                const @T@ p4 = ui12s * xi2;
                const @T@ d1 = p1 - p2;
                const @T@ d2 = d1 - p3;

                xr1 = d2 + p4;
            }

            {
                const @T@ p1 = ui11r * br1;
                const @T@ p2 = ur11r * bi1;
                const @T@ p3 = ui12s * xr2;
                const @T@ p4 = ur12s * xi2;
                const @T@ s1 = p1 + p2;
                const @T@ d2 = s1 - p3;

                xi1 = d2 - p4;
            }

            _xr[0] = zswap[icmax] ? xr2 : xr1;
            _xr[1] = zswap[icmax] ? xr1 : xr2;
            _xi[0] = zswap[icmax] ? xi2 : xi1;
            _xi[1] = zswap[icmax] ? xi1 : xi2;

            {
                const @T@ a1 = d_internal_math_@S@_abs(xr1);
                const @T@ a2 = d_internal_math_@S@_abs(xi1);
                const @T@ a3 = d_internal_math_@S@_abs(xr2);
                const @T@ a4 = d_internal_math_@S@_abs(xi2);
                const @T@ s1 = a1 + a2;
                const @T@ s2 = a3 + a4;

                *_xnorm = (s1 > s2) ? s1 : s2;
            }

            // |C| |X| would overflow: scaled again
            if ( (*_xnorm > (@T@)1) &&
                 (cmax > (@T@)1) )
            {
                const @T@ bound = bignum / cmax;

                if (*_xnorm > bound)
                {
                    const @T@ temp = cmax / bignum;

                    _xr[0]  = temp * _xr[0];
                    _xr[1]  = temp * _xr[1];
                    _xi[0]  = temp * _xi[0];
                    _xi[1]  = temp * _xi[1];
                    *_xnorm = temp * *_xnorm;
                    *_scale = temp * *_scale;
                }
            }
        }
    }

    return;
}

// d_internal_decomp_@S@_put
//   eigenvalue _j, _re + _im i, put where the caller asked: into _values[_j]
// unless _values is NULL, its real part into _real[_j] unless _real is NULL,
// and *_complex set where _im is not zero, unless _complex is NULL.
D_MATH_FN void
d_internal_decomp_@S@_put(
    struct dj_cplx_@S@* _values,
    @T@*                _real,
    bool*               _complex,
    size_t              _j,
    @T@                 _re,
    @T@                 _im
)
{
    if (_values != NULL)
    {
        _values[_j].part[0] = _re;
        _values[_j].part[1] = _im;
    }

    if (_real != NULL)
    {
        _real[_j] = _re;
    }

    if ( (_complex != NULL) &&
         (_im != (@T@)0) )
    {
        *_complex = true;
    }

    return;
}

// d_internal_decomp_@S@_reflect3
//   applies the reflector I - t1 (1, v2, v3)(1, v2, v3)^T, t2 = t1 v2 and
// t3 = t1 v3, to the three entries at _x, _x + _step and _x + 2 _step, or
// with _three false the reflector of (1, v2) to the first two: each entry
// less its share of their sum, as LAPACK's dlahqr writes it.
D_MATH_FN void
d_internal_decomp_@S@_reflect3(
    @T@*   _x,
    size_t _step,
    bool   _three,
    @T@    _v2,
    @T@    _v3,
    @T@    _t1,
    @T@    _t2,
    @T@    _t3
)
{
    const @T@ p2  = _v2 * _x[_step];
    @T@       sum = _x[0] + p2;

    if (_three)
    {
        const @T@ p3 = _v3 * _x[2u * _step];

        sum = sum + p3;
    }

    {
        const @T@ s1 = sum * _t1;
        const @T@ s2 = sum * _t2;

        _x[0]     = _x[0] - s1;
        _x[_step] = _x[_step] - s2;
    }

    if (_three)
    {
        const @T@ s3 = sum * _t3;

        _x[2u * _step] = _x[2u * _step] - s3;
    }

    return;
}

// d_internal_decomp_@S@_rotate
//   the plane rotation (x, y) -> (cs x + sn y, cs y - sn x) of the _count
// pairs _x[0], _y[0], _x[_stride], _y[_stride], ..., as BLAS's drot.
D_MATH_FN void
d_internal_decomp_@S@_rotate(
    @T@*   _x,
    @T@*   _y,
    size_t _count,
    size_t _stride,
    @T@    _cs,
    @T@    _sn
)
{
    size_t i = 0u;

    for (i = 0u; i < _count; ++i)
    {
        const @T@ x  = _x[i * _stride];
        const @T@ y  = _y[i * _stride];
        const @T@ a1 = _cs * x;
        const @T@ a2 = _sn * y;
        const @T@ b1 = _cs * y;
        const @T@ b2 = _sn * x;

        _x[i * _stride] = a1 + a2;
        _y[i * _stride] = b1 - b2;
    }

    return;
}

@EIGEN_FORMS@"""

DECOMP_FORMS = r"""
// d_decomp_@S@_qr@F@
//   Householder QR of the m-by-n _r (m >= n) in place, A = Q R: _q, the
// m-by-m identity on entry, receives Q, and _r becomes R. _v is scratch of m
// elements. A column already zero below the diagonal is left as it is.
D_MATH_FN@RT@ void
d_decomp_@S@_qr@F@(
    @T@*   _q,
    @T@*   _r,
    size_t _m,
    size_t _n,
    @T@*   _v
)
{
    size_t k = 0u;

    for (k = 0u; k < _n; ++k)
    {
        @T@    norm_sq = (@T@)0;
        @T@    normx   = (@T@)0;
        @T@    vtv     = (@T@)0;
        size_t i       = 0u;
        size_t j       = 0u;

        for (i = k; i < _m; ++i)
        {
            _v[i] = _r[(i * _n) + k];
        }

        for (i = k; i < _m; ++i)
        {
            const @T@ sq = _v[i] * _v[i];

            norm_sq = norm_sq + sq;
        }

        normx = d_math_@S@_sqrt@F@(norm_sq);

        // nothing below the diagonal to reflect away
        if (normx == (@T@)0)
        {
            continue;
        }

        {
            const @T@ alpha = (_v[k] >= (@T@)0) ? -normx : normx;

            _v[k] = _v[k] - alpha;
        }

        for (i = k; i < _m; ++i)
        {
            const @T@ sq = _v[i] * _v[i];

            vtv = vtv + sq;
        }

        // a degenerate reflector: leave the column
        if (vtv == (@T@)0)
        {
            continue;
        }

        {
            const @T@ two_over = (@T@)2 / vtv;
            size_t    row      = 0u;

            // R <- H R, column by column
            for (j = 0u; j < _n; ++j)
            {
                @T@ s = (@T@)0;

                for (i = k; i < _m; ++i)
                {
                    const @T@ prod = _v[i] * _r[(i * _n) + j];

                    s = s + prod;
                }

                {
                    const @T@ f = two_over * s;

                    for (i = k; i < _m; ++i)
                    {
                        const @T@ prod = f * _v[i];

                        _r[(i * _n) + j] = _r[(i * _n) + j] - prod;
                    }
                }
            }

            // Q <- Q H, row by row
            for (row = 0u; row < _m; ++row)
            {
                @T@ s = (@T@)0;

                for (i = k; i < _m; ++i)
                {
                    const @T@ prod = _q[(row * _m) + i] * _v[i];

                    s = s + prod;
                }

                {
                    const @T@ f = two_over * s;

                    for (i = k; i < _m; ++i)
                    {
                        const @T@ prod = f * _v[i];

                        _q[(row * _m) + i] = _q[(row * _m) + i] - prod;
                    }
                }
            }
        }
    }

    return;
}

// d_decomp_@S@_svd@F@
//   the thin SVD of the m-by-n _work (m >= n), A = U S V^T, by one-sided
// (Hestenes) Jacobi: column pairs rotated until each pair's correlation
// |gamma| is within _tol of the product of their norms, at most 60 sweeps.
// _work is consumed; _v, the n-by-n identity on entry, receives V; _s the
// singular values, descending (the first largest, among equals); _u the
// normalized columns, a zero one for a zero singular value. U's and V's
// columns follow their values in the sort.
D_MATH_FN@RT@ void
d_decomp_@S@_svd@F@(
    @T@*   _work,
    @T@*   _u,
    @T@*   _s,
    @T@*   _v,
    size_t _m,
    size_t _n,
    @T@    _tol
)
{
    size_t sweep = 0u;
    size_t i     = 0u;
    size_t j     = 0u;
    size_t k     = 0u;

    for (sweep = 0u; sweep < 60u; ++sweep)
    {
        bool rotated = false;

        for (i = 0u; i < _n; ++i)
        {
            for (j = i + 1u; j < _n; ++j)
            {
                @T@ alpha = (@T@)0;
                @T@ beta  = (@T@)0;
                @T@ gamma = (@T@)0;

                for (k = 0u; k < _m; ++k)
                {
                    const @T@ wki = _work[(k * _n) + i];
                    const @T@ wkj = _work[(k * _n) + j];
                    const @T@ aa  = wki * wki;
                    const @T@ bb  = wkj * wkj;
                    const @T@ ab  = wki * wkj;

                    alpha = alpha + aa;
                    beta  = beta + bb;
                    gamma = gamma + ab;
                }

                {
                    const @T@ ab_sq   = alpha * beta;
                    const @T@ product = d_math_@S@_sqrt@F@(ab_sq);
                    const @T@ bound   = _tol * product;

                    // the pair is orthogonal enough: no rotation
                    if (!(d_internal_math_@S@_abs(gamma) > bound))
                    {
                        continue;
                    }
                }

                rotated = true;

                {
                    const @T@ diff  = beta - alpha;
                    const @T@ twog  = (@T@)2 * gamma;
                    const @T@ half  = d_internal_math_@S@_pow2(@EMAX@ / 2);
                    const @T@ cut   = d_internal_math_@S@_abs(diff) / half;
                    const bool   far   = (cut >= d_internal_math_@S@_abs(twog));
                    const @T@ zeta  = far ? (@T@)0 : (diff / twog);
                    @T@       t     = (@T@)1;
                    @T@       c     = (@T@)0;
                    @T@       s     = (@T@)0;

                    // the smaller root of t^2 + 2 zeta t - 1 = 0; past
                    // 2^(EMAX / 2), where zeta^2 would overflow, 1 / (2
                    // zeta), gamma / diff, which it equals within a part in
                    // 2^EMAX
                    if ( (far) ||
                         (d_internal_math_@S@_abs(zeta) >= half) )
                    {
                        t = gamma / diff;
                    }
                    else if (zeta != (@T@)0)
                    {
                        const @T@ sgn  = (zeta > (@T@)0) ? (@T@)1
                                                         : (@T@)-1;
                        const @T@ zz   = zeta * zeta;
                        const @T@ one  = (@T@)1 + zz;
                        const @T@ root = d_math_@S@_sqrt@F@(one);
                        const @T@ mag  = d_internal_math_@S@_abs(zeta);
                        const @T@ den  = mag + root;

                        t = sgn / den;
                    }

                    {
                        const @T@ tt   = t * t;
                        const @T@ one  = (@T@)1 + tt;
                        const @T@ root = d_math_@S@_sqrt@F@(one);

                        c = (@T@)1 / root;
                        s = c * t;
                    }

                    // rotate the columns of the work matrix, then of V
                    for (k = 0u; k < _m; ++k)
                    {
                        const @T@ wki = _work[(k * _n) + i];
                        const @T@ wkj = _work[(k * _n) + j];
                        const @T@ a1  = c * wki;
                        const @T@ a2  = s * wkj;
                        const @T@ b1  = s * wki;
                        const @T@ b2  = c * wkj;

                        _work[(k * _n) + i] = a1 - a2;
                        _work[(k * _n) + j] = b1 + b2;
                    }

                    for (k = 0u; k < _n; ++k)
                    {
                        const @T@ vki = _v[(k * _n) + i];
                        const @T@ vkj = _v[(k * _n) + j];
                        const @T@ a1  = c * vki;
                        const @T@ a2  = s * vkj;
                        const @T@ b1  = s * vki;
                        const @T@ b2  = c * vkj;

                        _v[(k * _n) + i] = a1 - a2;
                        _v[(k * _n) + j] = b1 + b2;
                    }
                }
            }
        }

        // a sweep without a rotation: converged
        if (!rotated)
        {
            break;
        }
    }

    // the singular values and U, column by column
    for (j = 0u; j < _n; ++j)
    {
        @T@ norm_sq = (@T@)0;
        @T@ sigma   = (@T@)0;

        for (k = 0u; k < _m; ++k)
        {
            const @T@ sq = _work[(k * _n) + j] * _work[(k * _n) + j];

            norm_sq = norm_sq + sq;
        }

        sigma  = d_math_@S@_sqrt@F@(norm_sq);
        _s[j]  = sigma;

        for (k = 0u; k < _m; ++k)
        {
            const @T@ w = _work[(k * _n) + j];

            _u[(k * _n) + j] = (sigma > (@T@)0) ? (w / sigma) : (@T@)0;
        }
    }

    // descending, U's and V's columns along
    for (i = 0u; i < _n; ++i)
    {
        size_t sel = i;

        for (k = i + 1u; k < _n; ++k)
        {
            if (_s[k] > _s[sel])
            {
                sel = k;
            }
        }

        if (sel != i)
        {
            const @T@ tmp = _s[i];

            _s[i]   = _s[sel];
            _s[sel] = tmp;

            for (k = 0u; k < _m; ++k)
            {
                const @T@ t = _u[(k * _n) + i];

                _u[(k * _n) + i]   = _u[(k * _n) + sel];
                _u[(k * _n) + sel] = t;
            }

            for (k = 0u; k < _n; ++k)
            {
                const @T@ t = _v[(k * _n) + i];

                _v[(k * _n) + i]   = _v[(k * _n) + sel];
                _v[(k * _n) + sel] = t;
            }
        }
    }

    return;
}

// d_decomp_@S@_sym_eigen@F@
//   the eigenvalues and eigenvectors of the symmetric n-by-n _work by the
// cyclic Jacobi method: sweeps of plane rotations, each annihilating an
// off-diagonal pair, until the off-diagonal norm is within _threshold, at
// most 100 sweeps. _work is consumed; _vectors, the n-by-n identity on
// entry, receives the eigenvectors as columns; _values the eigenvalues,
// ascending (the first smallest, among equals), the columns following.
D_MATH_FN@RT@ void
d_decomp_@S@_sym_eigen@F@(
    @T@*   _work,
    @T@*   _values,
    @T@*   _vectors,
    size_t _n,
    @T@    _threshold
)
{
    size_t sweep = 0u;
    size_t p     = 0u;
    size_t q     = 0u;
    size_t i     = 0u;

    for (sweep = 0u; sweep < 100u; ++sweep)
    {
        @T@ off_sq = (@T@)0;

        for (p = 0u; p < _n; ++p)
        {
            for (q = p + 1u; q < _n; ++q)
            {
                const @T@ sq = _work[(p * _n) + q] * _work[(p * _n) + q];

                off_sq = off_sq + sq;
            }
        }

        // the off-diagonal part is small enough: converged
        if (d_math_@S@_sqrt@F@(off_sq) <= _threshold)
        {
            break;
        }

        for (p = 0u; p < _n; ++p)
        {
            for (q = p + 1u; q < _n; ++q)
            {
                const @T@ apq = _work[(p * _n) + q];

                // already zero: nothing to annihilate
                if (apq == (@T@)0)
                {
                    continue;
                }

                {
                    const @T@ aqq   = _work[(q * _n) + q];
                    const @T@ app   = _work[(p * _n) + p];
                    const @T@ diff  = aqq - app;
                    const @T@ twoa  = (@T@)2 * apq;
                    const @T@ half  = d_internal_math_@S@_pow2(@EMAX@ / 2);
                    const @T@ cut   = d_internal_math_@S@_abs(diff) / half;
                    const bool   far   = (cut >= d_internal_math_@S@_abs(twoa));
                    const @T@ theta = far ? (@T@)0 : (diff / twoa);
                    @T@       t     = (@T@)1;
                    @T@       c     = (@T@)0;
                    @T@       s     = (@T@)0;

                    // the smaller root of t^2 + 2 theta t - 1 = 0; past
                    // 2^(EMAX / 2), where theta^2 would overflow, 1 / (2
                    // theta), apq / diff, which it equals within a part in
                    // 2^EMAX
                    if ( (far) ||
                         (d_internal_math_@S@_abs(theta) >= half) )
                    {
                        t = apq / diff;
                    }
                    else if (theta != (@T@)0)
                    {
                        const @T@ sgn  = (theta > (@T@)0) ? (@T@)1
                                                          : (@T@)-1;
                        const @T@ tt   = theta * theta;
                        const @T@ one  = tt + (@T@)1;
                        const @T@ root = d_math_@S@_sqrt@F@(one);
                        const @T@ mag  = d_internal_math_@S@_abs(theta);
                        const @T@ den  = mag + root;

                        t = sgn / den;
                    }

                    {
                        const @T@ tt   = t * t;
                        const @T@ one  = tt + (@T@)1;
                        const @T@ root = d_math_@S@_sqrt@F@(one);

                        c = (@T@)1 / root;
                        s = t * c;
                    }

                    // the columns p and q, then the rows, then V's columns
                    for (i = 0u; i < _n; ++i)
                    {
                        const @T@ wip = _work[(i * _n) + p];
                        const @T@ wiq = _work[(i * _n) + q];
                        const @T@ a1  = c * wip;
                        const @T@ a2  = s * wiq;
                        const @T@ b1  = s * wip;
                        const @T@ b2  = c * wiq;

                        _work[(i * _n) + p] = a1 - a2;
                        _work[(i * _n) + q] = b1 + b2;
                    }

                    for (i = 0u; i < _n; ++i)
                    {
                        const @T@ wpj = _work[(p * _n) + i];
                        const @T@ wqj = _work[(q * _n) + i];
                        const @T@ a1  = c * wpj;
                        const @T@ a2  = s * wqj;
                        const @T@ b1  = s * wpj;
                        const @T@ b2  = c * wqj;

                        _work[(p * _n) + i] = a1 - a2;
                        _work[(q * _n) + i] = b1 + b2;
                    }

                    for (i = 0u; i < _n; ++i)
                    {
                        const @T@ vip = _vectors[(i * _n) + p];
                        const @T@ viq = _vectors[(i * _n) + q];
                        const @T@ a1  = c * vip;
                        const @T@ a2  = s * viq;
                        const @T@ b1  = s * vip;
                        const @T@ b2  = c * viq;

                        _vectors[(i * _n) + p] = a1 - a2;
                        _vectors[(i * _n) + q] = b1 + b2;
                    }
                }
            }
        }
    }

    for (i = 0u; i < _n; ++i)
    {
        _values[i] = _work[(i * _n) + i];
    }

    // ascending, the vectors' columns along
    for (i = 0u; i < _n; ++i)
    {
        size_t sel = i;
        size_t k   = 0u;

        for (k = i + 1u; k < _n; ++k)
        {
            if (_values[k] < _values[sel])
            {
                sel = k;
            }
        }

        if (sel != i)
        {
            const @T@ tmp = _values[i];

            _values[i]   = _values[sel];
            _values[sel] = tmp;

            for (k = 0u; k < _n; ++k)
            {
                const @T@ t = _vectors[(k * _n) + i];

                _vectors[(k * _n) + i]   = _vectors[(k * _n) + sel];
                _vectors[(k * _n) + sel] = t;
            }
        }
    }

    return;
}

// d_decomp_@S@_cholesky@F@
//   the Cholesky factor of the symmetric n-by-n _a (its lower triangle
// read) into _l, zero on entry: A = L L^T. Returns whether A is positive
// definite; at the first non-positive pivot it stops, _l filled to there.
D_MATH_FN@RT@ bool
d_decomp_@S@_cholesky@F@(
    @T@*       _l,
    const @T@* _a,
    size_t     _n
)
{
    size_t i = 0u;

    for (i = 0u; i < _n; ++i)
    {
        size_t j = 0u;

        for (j = 0u; j <= i; ++j)
        {
            @T@    sum = _a[(i * _n) + j];
            size_t k   = 0u;

            for (k = 0u; k < j; ++k)
            {
                const @T@ prod = _l[(i * _n) + k] * _l[(j * _n) + k];

                sum = sum - prod;
            }

            if (i == j)
            {
                // not positive definite: stop
                if (sum <= (@T@)0)
                {
                    return false;
                }

                _l[(i * _n) + j] = d_math_@S@_sqrt@F@(sum);
            }
            else
            {
                _l[(i * _n) + j] = sum / _l[(j * _n) + j];
            }
        }
    }

    return true;
}
"""

DECOMP_EIGEN_FORMS = r"""
// d_internal_decomp_@S@_rotation_of@F@
//   the plane rotation that takes (_x, _y) to (r, 0): c = _x / r and
// s = _y / r, formed on the pair divided by its larger magnitude, so that
// the sum of squares neither overflows nor underflows. (1, 0, 0) for
// (0, 0).
D_MATH_FN@RT@ struct dj_internal_decomp_@S@_rotation
d_internal_decomp_@S@_rotation_of@F@(
    @T@ _x,
    @T@ _y
)
{
    struct dj_internal_decomp_@S@_rotation g =
        { (@T@)1, (@T@)0, (@T@)0 };
    const @T@ ax = d_internal_math_@S@_abs(_x);
    const @T@ ay = d_internal_math_@S@_abs(_y);
    const @T@ m  = (ax > ay) ? ax : ay;

    // nothing to rotate
    if (m == (@T@)0)
    {
        return g;
    }

    {
        const @T@ xs   = _x / m;
        const @T@ ys   = _y / m;
        const @T@ xx   = xs * xs;
        const @T@ yy   = ys * ys;
        const @T@ sum  = xx + yy;
        const @T@ root = d_math_@S@_sqrt@F@(sum);

        g.c = xs / root;
        g.s = ys / root;
        g.r = m * root;
    }

    return g;
}

// d_decomp_@S@_hessenberg@F@
//   reduces the n-by-n _h in place to upper Hessenberg form by plane
// rotations: column by column, each entry below the subdiagonal from the
// bottom up is rotated into the one above it, the rotation applied to both
// rows and both columns -- a similarity, so the eigenvalues stay. The
// entries below the subdiagonal end exactly zero. Q is not formed.
D_MATH_FN@RT@ void
d_decomp_@S@_hessenberg@F@(
    @T@*   _h,
    size_t _n
)
{
    size_t j = 0u;

    for (j = 0u; (j + 2u) < _n; ++j)
    {
        size_t i = 0u;

        // the column's entries below the subdiagonal, the last row's first
        for (i = _n - 1u; i >= (j + 2u); --i)
        {
            const @T@ below = _h[(i * _n) + j];

            // already zero: nothing to rotate
            if (below == (@T@)0)
            {
                continue;
            }

            {
                const @T@ above = _h[((i - 1u) * _n) + j];
                const struct dj_internal_decomp_@S@_rotation g =
                    d_internal_decomp_@S@_rotation_of@F@(above, below);
                size_t k = 0u;

                // rows i - 1 and i, from column j on
                for (k = j; k < _n; ++k)
                {
                    const @T@ t1 = _h[((i - 1u) * _n) + k];
                    const @T@ t2 = _h[(i * _n) + k];
                    const @T@ a1 = g.c * t1;
                    const @T@ a2 = g.s * t2;
                    const @T@ b1 = g.c * t2;
                    const @T@ b2 = g.s * t1;

                    _h[((i - 1u) * _n) + k] = a1 + a2;
                    _h[(i * _n) + k]        = b1 - b2;
                }

                // the rotated column, exactly (r, 0)
                _h[((i - 1u) * _n) + j] = g.r;
                _h[(i * _n) + j]        = (@T@)0;

                // columns i - 1 and i, every row
                for (k = 0u; k < _n; ++k)
                {
                    const @T@ t1 = _h[(k * _n) + (i - 1u)];
                    const @T@ t2 = _h[(k * _n) + i];
                    const @T@ a1 = g.c * t1;
                    const @T@ a2 = g.s * t2;
                    const @T@ b1 = g.c * t2;
                    const @T@ b2 = g.s * t1;

                    _h[(k * _n) + (i - 1u)] = a1 + a2;
                    _h[(k * _n) + i]        = b1 - b2;
                }
            }
        }
    }

    return;
}

// d_internal_decomp_@S@_hypot@F@
//   sqrt(_x^2 + _y^2), as LAPACK's dlapy2: the larger magnitude times the
// root of 1 and the smaller's ratio to it squared, so that nothing on the
// way overflows or underflows; the larger magnitude itself where the
// smaller is zero or the larger infinite; a NaN for a NaN.
D_MATH_FN@RT@ @T@
d_internal_decomp_@S@_hypot@F@(
    @T@ _x,
    @T@ _y
)
{
    const @T@ ax = d_internal_math_@S@_abs(_x);
    const @T@ ay = d_internal_math_@S@_abs(_y);
    const @T@ w  = (ax > ay) ? ax : ay;
    const @T@ z  = (ax > ay) ? ay : ax;

    // a NaN
    if ( (_x != _x) ||
         (_y != _y) )
    {
        return d_internal_math_@S@_nan();
    }

    // nothing to add, or an infinity
    if ( (z == (@T@)0) ||
         (w > @MAX@) )
    {
        return w;
    }

    {
        const @T@ ratio = z / w;
        const @T@ sq    = ratio * ratio;
        const @T@ one   = (@T@)1 + sq;
        const @T@ root  = d_math_@S@_sqrt@F@(one);

        return w * root;
    }
}

// d_internal_decomp_@S@_norm@F@
//   the Euclidean norm of the _count entries _x[0], _x[_stride], ...: the
// root of their sum of squares; where that norm passes the family's range or
// falls below the root of its least normal over its epsilon, the norm of the
// entries scaled exactly by a power of two to a largest magnitude in [1, 2),
// scaled back (S256); with a NaN or an infinity among them, or all zero, the
// sum's own root.
D_MATH_FN@RT@ @T@
d_internal_decomp_@S@_norm@F@(
    const @T@* _x,
    size_t     _count,
    size_t     _stride
)
{
    const @T@ small   = d_math_@S@_sqrt@F@(@MIN@ / @EPS@);
    @T@       sum     = (@T@)0;
    @T@       norm    = (@T@)0;
    @T@       largest = (@T@)0;
    size_t    i       = 0u;

    for (i = 0u; i < _count; ++i)
    {
        const @T@ v  = _x[i * _stride];
        const @T@ sq = v * v;

        sum = sum + sq;
    }

    norm = d_math_@S@_sqrt@F@(sum);

    // in range: the sum's root
    if ( (norm <= @MAX@) &&
         (!(norm < small)) )
    {
        return norm;
    }

    // the largest magnitude; a NaN or an infinity, the sum's root
    for (i = 0u; i < _count; ++i)
    {
        const @T@ v = d_internal_math_@S@_abs(_x[i * _stride]);

        if (!(v <= @MAX@))
        {
            return norm;
        }

        if (v > largest)
        {
            largest = v;
        }
    }

    // the zero vector
    if (largest == (@T@)0)
    {
        return norm;
    }

    {
        @T@ pre   = (@T@)1;
        @T@ power = (@T@)1;

        // a subnormal largest: lifted into the normal range first
        if (largest < @MIN@)
        {
            pre     = d_internal_math_@S@_pow2(@P@);
            largest = largest * pre;
        }

        {
            const struct dj_internal_math_@S@_binade b =
                d_internal_math_@S@_binade_of(largest);

            power = b.power;
        }

        sum = (@T@)0;

        for (i = 0u; i < _count; ++i)
        {
            const @T@ lifted = _x[i * _stride] * pre;
            const @T@ v      = lifted / power;
            const @T@ sq     = v * v;

            sum = sum + sq;
        }

        {
            const @T@ root     = d_math_@S@_sqrt@F@(sum);
            const @T@ restored = root * power;

            return restored / pre;
        }
    }
}

// d_decomp_@S@_balance@F@
//   balances the n-by-n _a in place, as LAPACK's dgebal does with job 'B':
// the rows and columns that isolate an eigenvalue permuted to the ends --
// each row zero off the diagonal, within the columns not yet isolated, to
// the bottom, then each such column to the top -- so that the eigenvalues
// outside rows *_ilo to *_ihi stand on the diagonal; then each row and
// column inside, by turns, scaled by a power of two, its row divided and its
// column multiplied, wherever that brings the sum of the two norms below
// 0.95 of what it was, until none does: a diagonal similarity, exact unless
// an entry passes below the least normal on the way, as dgebal lets it
// (S311). The eigenvalues stay; the norm, before a QR iteration, falls.
// _scale (n entries) receives each factor, 1 outside *_ilo..*_ihi; _perm
// (n entries), unless NULL, each outside row's swap. Indices count from 0.
D_MATH_FN_KERNEL@RT@ void
d_decomp_@S@_balance@F@(
    @T@*    _a,
    size_t  _n,
    size_t* _perm,
    @T@*    _scale,
    size_t* _ilo,
    size_t* _ihi
)
{
    const @T@ sfmin1 = @MIN@ / @EPS@;
    const @T@ sfmax1 = (@T@)1 / sfmin1;
    const @T@ sfmin2 = sfmin1 * (@T@)2;
    const @T@ sfmax2 = (@T@)1 / sfmin2;
    size_t    k      = 0u;
    size_t    l      = 0u;
    size_t    i      = 0u;
    size_t    j      = 0u;
    bool      noconv = true;

    *_ilo = 0u;
    *_ihi = 0u;

    // nothing to balance
    if (_n == 0u)
    {
        return;
    }

    for (i = 0u; i < _n; ++i)
    {
        _scale[i] = (@T@)1;

        if (_perm != NULL)
        {
            _perm[i] = i;
        }
    }

    l = _n - 1u;

    // rows isolating an eigenvalue, pushed to the bottom
    while (noconv)
    {
        const size_t top = l;

        noconv = false;

        for (i = top + 1u; i-- > 0u; )
        {
            bool canswap = true;

            for (j = 0u; j <= l; ++j)
            {
                if ( (i != j) &&
                     (_a[(i * _n) + j] != (@T@)0) )
                {
                    canswap = false;

                    break;
                }
            }

            // the row is zero off the diagonal: to the bottom
            if (canswap)
            {
                if (_perm != NULL)
                {
                    _perm[l] = i;
                }

                if (i != l)
                {
                    d_internal_decomp_@S@_swap_isolated(_a, _n, i, l, k, l);
                }

                noconv = true;

                // every row isolated: the matrix is triangular
                if (l == 0u)
                {
                    return;
                }

                --l;
            }
        }
    }

    // columns isolating an eigenvalue, pushed to the top
    noconv = true;

    while (noconv)
    {
        const size_t first = k;

        noconv = false;

        for (j = first; j <= l; ++j)
        {
            bool canswap = true;

            for (i = k; i <= l; ++i)
            {
                if ( (i != j) &&
                     (_a[(i * _n) + j] != (@T@)0) )
                {
                    canswap = false;

                    break;
                }
            }

            // the column is zero off the diagonal: to the top
            if (canswap)
            {
                if (_perm != NULL)
                {
                    _perm[k] = j;
                }

                if (j != k)
                {
                    d_internal_decomp_@S@_swap_isolated(_a, _n, j, k, k, l);
                }

                noconv = true;
                ++k;
            }
        }
    }

    // every column isolated: one row is left, alone
    if (k > l)
    {
        k = l;
    }

    // the norms brought near, row and column by turns
    noconv = true;

    while (noconv)
    {
        noconv = false;

        for (i = k; i <= l; ++i)
        {
            const size_t count = (l - k) + 1u;
            @T@          c     = d_internal_decomp_@S@_norm@F@(
                                     _a + ((k * _n) + i), count, _n);
            @T@          r     = d_internal_decomp_@S@_norm@F@(
                                     _a + ((i * _n) + k), count, 1u);
            @T@          ca    = (@T@)0;
            @T@          ra    = (@T@)0;
            @T@          g     = (@T@)0;
            @T@          f     = (@T@)1;
            @T@          s     = (@T@)0;

            for (j = 0u; j <= l; ++j)
            {
                const @T@ v = d_internal_math_@S@_abs(_a[(j * _n) + i]);

                if (v > ca)
                {
                    ca = v;
                }
            }

            for (j = k; j < _n; ++j)
            {
                const @T@ v = d_internal_math_@S@_abs(_a[(i * _n) + j]);

                if (v > ra)
                {
                    ra = v;
                }
            }

            // a zero norm, through underflow: nothing to bring near
            if ( (c == (@T@)0) ||
                 (r == (@T@)0) )
            {
                continue;
            }

            g = r / (@T@)2;
            s = c + r;

            // the column too small: doubled while it stays below half the row
            while (c < g)
            {
                const @T@ big1  = (f > c) ? f : c;
                const @T@ big   = (big1 > ca) ? big1 : ca;
                const @T@ tiny1 = (r < g) ? r : g;
                const @T@ tiny  = (tiny1 < ra) ? tiny1 : ra;

                if ( (!(big < sfmax2)) ||
                     (!(tiny > sfmin2)) )
                {
                    break;
                }

                f  = f * (@T@)2;
                c  = c * (@T@)2;
                ca = ca * (@T@)2;
                r  = r / (@T@)2;
                g  = g / (@T@)2;
                ra = ra / (@T@)2;
            }

            g = c / (@T@)2;

            // the column too large: halved while half of it is at least the row
            while (g >= r)
            {
                const @T@ big   = (r > ra) ? r : ra;
                const @T@ tiny1 = (f < c) ? f : c;
                const @T@ tiny2 = (g < ca) ? g : ca;
                const @T@ tiny  = (tiny1 < tiny2) ? tiny1 : tiny2;

                if ( (!(big < sfmax2)) ||
                     (!(tiny > sfmin2)) )
                {
                    break;
                }

                f  = f / (@T@)2;
                c  = c / (@T@)2;
                g  = g / (@T@)2;
                ca = ca / (@T@)2;
                r  = r * (@T@)2;
                ra = ra * (@T@)2;
            }

            {
                const @T@ sum   = c + r;
                const @T@ bound = (@T@)0.95 * s;

                // not enough gained
                if (sum >= bound)
                {
                    continue;
                }
            }

            // the factor would leave the safe range
            if ( (f < (@T@)1) &&
                 (_scale[i] < (@T@)1) )
            {
                const @T@ product = f * _scale[i];

                if (product <= sfmin1)
                {
                    continue;
                }
            }

            if ( (f > (@T@)1) &&
                 (_scale[i] > (@T@)1) )
            {
                const @T@ room = sfmax1 / f;

                if (_scale[i] >= room)
                {
                    continue;
                }
            }

            g         = (@T@)1 / f;
            _scale[i] = _scale[i] * f;
            noconv    = true;

            for (j = k; j < _n; ++j)
            {
                _a[(i * _n) + j] = _a[(i * _n) + j] * g;
            }

            for (j = 0u; j <= l; ++j)
            {
                _a[(j * _n) + i] = _a[(j * _n) + i] * f;
            }
        }
    }

    *_ilo = k;
    *_ihi = l;

    return;
}

// d_internal_decomp_@S@_reflector@F@
//   the elementary reflector H = I - tau v v^T, v = (1, x), x the _count
// entries _x[0], _x[_stride], ..., that takes (*_alpha, x) to (beta, 0), as
// LAPACK's dlarfg: beta = -sign(alpha) times the pair's norm (alpha's
// sign, +, for a zero), tau = (beta - alpha) / beta, x multiplied by
// 1 / (alpha - beta); where beta is below the family's least normal over
// its epsilon, the vector is lifted first and beta lowered after. *_alpha
// becomes beta, _x becomes v's tail, and tau is returned: 0, H = I, where x
// is zero.
D_MATH_FN@RT@ @T@
d_internal_decomp_@S@_reflector@F@(
    @T@*   _alpha,
    @T@*   _x,
    size_t _count,
    size_t _stride
)
{
    const @T@ safmin = @MIN@ / (@EPS@ / (@T@)2);
    const @T@ rsafmn = (@T@)1 / safmin;
    @T@       xnorm  = d_internal_decomp_@S@_norm@F@(_x, _count, _stride);
    @T@       beta   = (@T@)0;
    @T@       tau    = (@T@)0;
    size_t    knt    = 0u;
    size_t    i      = 0u;

    // x zero: H = I
    if (xnorm == (@T@)0)
    {
        return (@T@)0;
    }

    {
        const @T@ h = d_internal_decomp_@S@_hypot@F@(*_alpha, xnorm);

        beta = (*_alpha < (@T@)0) ? h : -h;
    }

    // beta below the safe minimum: lifted, and found again
    if (d_internal_math_@S@_abs(beta) < safmin)
    {
        do
        {
            ++knt;

            for (i = 0u; i < _count; ++i)
            {
                _x[i * _stride] = _x[i * _stride] * rsafmn;
            }

            beta    = beta * rsafmn;
            *_alpha = *_alpha * rsafmn;
        }
        while ( (d_internal_math_@S@_abs(beta) < safmin) &&
                (knt < 20u) );

        xnorm = d_internal_decomp_@S@_norm@F@(_x, _count, _stride);

        {
            const @T@ h = d_internal_decomp_@S@_hypot@F@(*_alpha, xnorm);

            beta = (*_alpha < (@T@)0) ? h : -h;
        }
    }

    {
        const @T@ diff = beta - *_alpha;
        const @T@ den  = *_alpha - beta;
        const @T@ rec  = (@T@)1 / den;

        tau = diff / beta;

        for (i = 0u; i < _count; ++i)
        {
            _x[i * _stride] = _x[i * _stride] * rec;
        }
    }

    // lowered back
    for (i = 0u; i < knt; ++i)
    {
        beta = beta * safmin;
    }

    *_alpha = beta;

    return tau;
}

// d_decomp_@S@_hessenberg_q@F@
//   reduces rows and columns _ilo to _ihi of the n-by-n _h to upper
// Hessenberg form in place by Householder reflectors, as LAPACK's dgehd2:
// for each column from _ilo, the reflector that zeroes it below the
// subdiagonal, applied from the right to rows 0 to _ihi and from the left to
// columns to the end -- a similarity, H = Q^T A Q, so the eigenvalues stay.
// The rows and columns outside, as balancing leaves them, keep their zeros.
// _q, unless NULL, the n-by-n identity on entry, receives Q, each reflector
// multiplied in on the right as it is made. The entries below the
// subdiagonal end exactly zero.
D_MATH_FN_KERNEL@RT@ void
d_decomp_@S@_hessenberg_q@F@(
    @T@*   _h,
    size_t _n,
    size_t _ilo,
    size_t _ihi,
    @T@*   _q
)
{
    size_t i = 0u;

    // nothing to reduce
    if ( (_n == 0u) ||
         (_ihi <= _ilo) )
    {
        return;
    }

    for (i = _ilo; (i + 1u) < _ihi; ++i)
    {
        const size_t below = _ihi - i - 1u;
        @T@          alpha = _h[((i + 1u) * _n) + i];
        const @T@    tau   = d_internal_decomp_@S@_reflector@F@(
                                 &alpha, _h + (((i + 2u) * _n) + i), below,
                                 _n);
        size_t       r     = 0u;
        size_t       c     = 0u;

        _h[((i + 1u) * _n) + i] = alpha;

        // H = I: nothing to apply
        if (tau != (@T@)0)
        {
            // from the right: rows 0 to ihi, columns i + 1 to ihi
            for (r = 0u; r <= _ihi; ++r)
            {
                @T@ w = _h[(r * _n) + (i + 1u)];

                for (c = i + 2u; c <= _ihi; ++c)
                {
                    const @T@ p = _h[(r * _n) + c] * _h[(c * _n) + i];

                    w = w + p;
                }

                {
                    const @T@ t = tau * w;

                    _h[(r * _n) + (i + 1u)] = _h[(r * _n) + (i + 1u)] - t;

                    for (c = i + 2u; c <= _ihi; ++c)
                    {
                        const @T@ p = t * _h[(c * _n) + i];

                        _h[(r * _n) + c] = _h[(r * _n) + c] - p;
                    }
                }
            }

            // from the left: rows i + 1 to ihi, columns i + 1 to the end
            for (c = i + 1u; c < _n; ++c)
            {
                @T@ w = _h[((i + 1u) * _n) + c];

                for (r = i + 2u; r <= _ihi; ++r)
                {
                    const @T@ p = _h[(r * _n) + i] * _h[(r * _n) + c];

                    w = w + p;
                }

                {
                    const @T@ t = tau * w;

                    _h[((i + 1u) * _n) + c] = _h[((i + 1u) * _n) + c] - t;

                    for (r = i + 2u; r <= _ihi; ++r)
                    {
                        const @T@ p = _h[(r * _n) + i] * t;

                        _h[(r * _n) + c] = _h[(r * _n) + c] - p;
                    }
                }
            }

            // Q's columns i + 1 to ihi, from the right
            if (_q != NULL)
            {
                for (r = _ilo; r <= _ihi; ++r)
                {
                    @T@ w = _q[(r * _n) + (i + 1u)];

                    for (c = i + 2u; c <= _ihi; ++c)
                    {
                        const @T@ p = _q[(r * _n) + c] * _h[(c * _n) + i];

                        w = w + p;
                    }

                    {
                        const @T@ t = tau * w;

                        _q[(r * _n) + (i + 1u)] =
                            _q[(r * _n) + (i + 1u)] - t;

                        for (c = i + 2u; c <= _ihi; ++c)
                        {
                            const @T@ p = t * _h[(c * _n) + i];

                            _q[(r * _n) + c] = _q[(r * _n) + c] - p;
                        }
                    }
                }
            }
        }

        // the reflector's vector, kept below the subdiagonal until now
        for (r = i + 2u; r <= _ihi; ++r)
        {
            _h[(r * _n) + i] = (@T@)0;
        }
    }

    return;
}

// d_internal_decomp_@S@_standard_pair@F@
//   the 2-by-2 block [[*_a, *_b], [*_c, *_d]] brought to its standard Schur
// form by a rotation [[cs, -sn], [sn, cs]], as LAPACK's dlanv2: upper
// triangular where its eigenvalues are real, else with equal diagonals and
// off-diagonals of opposite signs. Its eigenvalues go to _values[0] and
// _values[1]: a real one's imaginary part exactly +0, a complex pair's
// sqrt(|b|) sqrt(|c|) and its negation, so that the two are exactly
// conjugate, the positive first.
D_MATH_FN@RT@ void
d_internal_decomp_@S@_standard_pair@F@(
    @T@*                _a,
    @T@*                _b,
    @T@*                _c,
    @T@*                _d,
    struct dj_cplx_@S@* _values,
    @T@*                _cs,
    @T@*                _sn
)
{
    const @T@ eps    = @EPS@;
    const @T@ safmn2 = d_internal_math_@S@_pow2((@EMIN@ - 2 + @P@) / 2);
    const @T@ safmx2 = (@T@)1 / safmn2;
    @T@       a      = *_a;
    @T@       b      = *_b;
    @T@       c      = *_c;
    @T@       d      = *_d;
    @T@       cs     = (@T@)1;
    @T@       sn     = (@T@)0;

    // already triangular
    if (c == (@T@)0)
    {
        cs = (@T@)1;
        sn = (@T@)0;
    }
    // lower triangular: the rows and columns swapped
    else if (b == (@T@)0)
    {
        const @T@ temp = d;

        cs = (@T@)0;
        sn = (@T@)1;
        d  = a;
        a  = temp;
        b  = -c;
        c  = (@T@)0;
    }
    // already standard: equal diagonals, off-diagonals of opposite signs
    else if ( ((a - d) == (@T@)0) &&
              ((b < (@T@)0) != (c < (@T@)0)) )
    {
        cs = (@T@)1;
        sn = (@T@)0;
    }
    else
    {
        const @T@ temp0 = a - d;
        @T@       p     = temp0 / (@T@)2;
        const @T@ ab    = d_internal_math_@S@_abs(b);
        const @T@ ac    = d_internal_math_@S@_abs(c);
        const @T@ bcmax = (ab > ac) ? ab : ac;
        const @T@ bcmn  = (ab > ac) ? ac : ab;
        const @T@ sgnb  = (b < (@T@)0) ? (@T@)-1 : (@T@)1;
        const @T@ sgnc  = (c < (@T@)0) ? (@T@)-1 : (@T@)1;
        const @T@ bcmis = (bcmn * sgnb) * sgnc;
        const @T@ ap    = d_internal_math_@S@_abs(p);
        const @T@ scale = (ap > bcmax) ? ap : bcmax;
        @T@       z     = (@T@)0;

        {
            const @T@ r1 = p / scale;
            const @T@ t1 = r1 * p;
            const @T@ r2 = bcmax / scale;
            const @T@ t2 = r2 * bcmis;

            z = t1 + t2;
        }

        // real eigenvalues, clearly: A and D, and the rotation
        if (z >= ((@T@)4 * eps))
        {
            const @T@ rs   = d_math_@S@_sqrt@F@(scale);
            const @T@ rz   = d_math_@S@_sqrt@F@(z);
            const @T@ root = rs * rz;
            @T@       tau  = (@T@)0;

            z = (p < (@T@)0) ? (p - root) : (p + root);
            a = d + z;

            {
                const @T@ q = bcmax / z;
                const @T@ m = q * bcmis;

                d = d - m;
            }

            tau = d_internal_decomp_@S@_hypot@F@(c, z);
            cs  = z / tau;
            sn  = c / tau;
            b   = b - c;
            c   = (@T@)0;
        }
        // complex, or real and nearly equal: the diagonals made equal
        else
        {
            @T@    sigma = b + c;
            @T@    temp  = temp0;
            size_t count = 0u;
            @T@    tau   = (@T@)0;

            // sigma and temp brought into the safe range
            for (;;)
            {
                const @T@ at  = d_internal_math_@S@_abs(temp);
                const @T@ asg = d_internal_math_@S@_abs(sigma);
                const @T@ big = (at > asg) ? at : asg;

                ++count;

                if (big >= safmx2)
                {
                    sigma = sigma * safmn2;
                    temp  = temp * safmn2;

                    if (count <= 20u)
                    {
                        continue;
                    }
                }

                if (big <= safmn2)
                {
                    sigma = sigma * safmx2;
                    temp  = temp * safmx2;

                    if (count <= 20u)
                    {
                        continue;
                    }
                }

                break;
            }

            p   = temp / (@T@)2;
            tau = d_internal_decomp_@S@_hypot@F@(sigma, temp);

            {
                const @T@ ratio = d_internal_math_@S@_abs(sigma) / tau;
                const @T@ one   = (@T@)1 + ratio;
                const @T@ half  = one / (@T@)2;

                cs = d_math_@S@_sqrt@F@(half);

                {
                    const @T@ den = tau * cs;
                    const @T@ q   = p / den;

                    sn = (sigma < (@T@)0) ? q : -q;
                }
            }

            {
                // [aa bb; cc dd] = [a b; c d] [cs -sn; sn cs]
                const @T@ a1 = a * cs;
                const @T@ a2 = b * sn;
                const @T@ b1 = a * sn;
                const @T@ b2 = b * cs;
                const @T@ c1 = c * cs;
                const @T@ c2 = d * sn;
                const @T@ d1 = c * sn;
                const @T@ d2 = d * cs;
                const @T@ aa = a1 + a2;
                const @T@ bb = b2 - b1;
                const @T@ cc = c1 + c2;
                const @T@ dd = d2 - d1;

                // [a b; c d] = [cs sn; -sn cs] [aa bb; cc dd]
                {
                    const @T@ e1 = aa * cs;
                    const @T@ e2 = cc * sn;
                    const @T@ f1 = bb * cs;
                    const @T@ f2 = dd * sn;
                    const @T@ g1 = aa * sn;
                    const @T@ g2 = cc * cs;
                    const @T@ h1 = bb * sn;
                    const @T@ h2 = dd * cs;

                    a = e1 + e2;
                    b = f1 + f2;
                    c = g2 - g1;
                    d = h2 - h1;
                }
            }

            {
                const @T@ sum = a + d;

                temp = sum / (@T@)2;
            }

            a = temp;
            d = temp;

            if (c != (@T@)0)
            {
                if (b != (@T@)0)
                {
                    // real after all: reduced to upper triangular
                    if ((b < (@T@)0) == (c < (@T@)0))
                    {
                        const @T@ sab  = d_math_@S@_sqrt@F@(
                                             d_internal_math_@S@_abs(b));
                        const @T@ sac  = d_math_@S@_sqrt@F@(
                                             d_internal_math_@S@_abs(c));
                        const @T@ prod = sab * sac;
                        const @T@ pp   = (c < (@T@)0) ? -prod : prod;
                        const @T@ bc   = b + c;
                        const @T@ rt   = d_math_@S@_sqrt@F@(
                                             d_internal_math_@S@_abs(bc));
                        const @T@ t2   = (@T@)1 / rt;
                        const @T@ cs1  = sab * t2;
                        const @T@ sn1  = sac * t2;

                        a = temp + pp;
                        d = temp - pp;
                        b = b - c;
                        c = (@T@)0;

                        {
                            const @T@ m1 = cs * cs1;
                            const @T@ m2 = sn * sn1;
                            const @T@ m3 = cs * sn1;
                            const @T@ m4 = sn * cs1;
                            const @T@ nc = m1 - m2;

                            sn = m3 + m4;
                            cs = nc;
                        }
                    }
                }
                else
                {
                    const @T@ t = cs;

                    b  = -c;
                    c  = (@T@)0;
                    cs = -sn;
                    sn = t;
                }
            }
        }
    }

    *_a  = a;
    *_b  = b;
    *_c  = c;
    *_d  = d;
    *_cs = cs;
    *_sn = sn;

    _values[0].part[0] = a;
    _values[1].part[0] = d;

    // real
    if (c == (@T@)0)
    {
        _values[0].part[1] = (@T@)0;
        _values[1].part[1] = (@T@)0;
    }
    // a complex pair, exactly conjugate
    else
    {
        const @T@ rb = d_math_@S@_sqrt@F@(d_internal_math_@S@_abs(b));
        const @T@ rc = d_math_@S@_sqrt@F@(d_internal_math_@S@_abs(c));
        const @T@ im = rb * rc;

        _values[0].part[1] = im;
        _values[1].part[1] = -im;
    }

    return;
}

// d_internal_decomp_@S@_schur@F@
//   d_decomp_@S@_schur@F@'s iteration, its eigenvalues put by
// d_internal_decomp_@S@_put: into _values, unless NULL, and their real
// parts into _real, unless NULL, *_complex, unless NULL, set where one is
// not real.
D_MATH_FN_KERNEL@RT@ bool
d_internal_decomp_@S@_schur@F@(
    @T@*                _h,
    size_t              _n,
    size_t              _ilo,
    size_t              _ihi,
    @T@*                _z,
    struct dj_cplx_@S@* _values,
    @T@*                _real,
    bool*               _complex,
    size_t              _max_iter,
    @T@                 _tol
)
{
    const @T@    dat1   = (@T@)0.75;
    const @T@    dat2   = (@T@)-0.4375;
    const size_t kexsh  = 10u;
    const @T@    ulp    = @EPS@;
    const @T@    tol    = (_tol > ulp) ? _tol : ulp;
    const bool   wantt  = (_z != NULL);
    size_t       nh     = 0u;
    @T@          smlnum = (@T@)0;
    size_t       i1     = 0u;
    size_t       i2     = 0u;
    size_t       kdefl  = 0u;
    size_t       steps  = 0u;
    size_t       top    = 0u;
    size_t       j      = 0u;

    // nothing to find
    if (_n == 0u)
    {
        return true;
    }

    // the eigenvalues outside the unreduced part: the diagonal
    for (j = 0u; j < _n; ++j)
    {
        if ( (j < _ilo) ||
             (j > _ihi) )
        {
            d_internal_decomp_@S@_put(_values, _real, _complex, j,
                                      _h[(j * _n) + j], (@T@)0);
        }
    }

    // a block of one
    if (_ilo == _ihi)
    {
        d_internal_decomp_@S@_put(_values, _real, _complex, _ilo,
                                  _h[(_ilo * _n) + _ilo], (@T@)0);

        return true;
    }

    // what lies below the subdiagonal, cleared
    for (j = _ilo; (j + 3u) <= _ihi; ++j)
    {
        _h[((j + 2u) * _n) + j] = (@T@)0;
        _h[((j + 3u) * _n) + j] = (@T@)0;
    }

    if ((_ilo + 2u) <= _ihi)
    {
        _h[(_ihi * _n) + (_ihi - 2u)] = (@T@)0;
    }

    nh     = (_ihi - _ilo) + 1u;
    smlnum = @MIN@ * ((@T@)nh / ulp);

    if (wantt)
    {
        i1 = 0u;
        i2 = _n - 1u;
    }

    // top is one past the last row still to be solved
    top = _ihi + 1u;

    while (top > _ilo)
    {
        const size_t i = top - 1u;
        size_t       l = _ilo;
        size_t       k = 0u;

        for (;;)
        {
            size_t m = 0u;
            @T@    v[3] = { (@T@)0, (@T@)0, (@T@)0 };
            @T@    rt1r = (@T@)0;
            @T@    rt1i = (@T@)0;
            @T@    rt2r = (@T@)0;
            @T@    rt2i = (@T@)0;

            // the lowest negligible subdiagonal entry
            for (k = i; k > l; --k)
            {
                const @T@ sub = d_internal_math_@S@_abs(
                                    _h[(k * _n) + (k - 1u)]);
                @T@       tst = (@T@)0;

                if (sub <= smlnum)
                {
                    break;
                }

                {
                    const @T@ d1 = d_internal_math_@S@_abs(
                                       _h[((k - 1u) * _n) + (k - 1u)]);
                    const @T@ d2 = d_internal_math_@S@_abs(
                                       _h[(k * _n) + k]);

                    tst = d1 + d2;
                }

                // both neighbours zero: the next ones out
                if (tst == (@T@)0)
                {
                    if (k >= (_ilo + 2u))
                    {
                        tst = tst + d_internal_math_@S@_abs(
                                        _h[((k - 1u) * _n) + (k - 2u)]);
                    }

                    if ((k + 1u) <= _ihi)
                    {
                        tst = tst + d_internal_math_@S@_abs(
                                        _h[((k + 1u) * _n) + k]);
                    }
                }

                {
                    const @T@ bound = tol * tst;

                    // Ahues and Kressner's test
                    if (sub <= bound)
                    {
                        const @T@ sup  = d_internal_math_@S@_abs(
                                             _h[((k - 1u) * _n) + k]);
                        const @T@ ab   = (sub > sup) ? sub : sup;
                        const @T@ ba   = (sub > sup) ? sup : sub;
                        const @T@ hkk  = d_internal_math_@S@_abs(
                                             _h[(k * _n) + k]);
                        const @T@ diff = _h[((k - 1u) * _n) + (k - 1u)] -
                                         _h[(k * _n) + k];
                        const @T@ ad   = d_internal_math_@S@_abs(diff);
                        const @T@ aa   = (hkk > ad) ? hkk : ad;
                        const @T@ bb   = (hkk > ad) ? ad : hkk;
                        const @T@ s    = aa + ab;
                        const @T@ r1   = ab / s;
                        const @T@ lhs  = ba * r1;
                        const @T@ r2   = aa / s;
                        const @T@ p    = bb * r2;
                        const @T@ rhs1 = tol * p;
                        const @T@ rhs  = (smlnum > rhs1) ? smlnum : rhs1;

                        if (lhs <= rhs)
                        {
                            break;
                        }
                    }
                }
            }

            l = k;

            // negligible: zero
            if (l > _ilo)
            {
                _h[(l * _n) + (l - 1u)] = (@T@)0;
            }

            // a block of one or two split off
            if ((l + 1u) >= i)
            {
                break;
            }

            // the budget spent: the rows not reached, their diagonal
            if (steps >= _max_iter)
            {
                for (j = _ilo; j <= i; ++j)
                {
                    d_internal_decomp_@S@_put(_values, _real, _complex, j,
                                              _h[(j * _n) + j], (@T@)0);
                }

                return false;
            }

            ++steps;
            ++kdefl;

            // eigenvalues alone: only the active block transformed
            if (!wantt)
            {
                i1 = l;
                i2 = i;
            }

            {
                @T@ h11 = (@T@)0;
                @T@ h12 = (@T@)0;
                @T@ h21 = (@T@)0;
                @T@ h22 = (@T@)0;
                @T@ s   = (@T@)0;

                // exceptional shifts, at the bottom, then at the top
                if ((kdefl % (2u * kexsh)) == 0u)
                {
                    const @T@ e1 = d_internal_math_@S@_abs(
                                       _h[(i * _n) + (i - 1u)]);
                    const @T@ e2 = d_internal_math_@S@_abs(
                                       _h[((i - 1u) * _n) + (i - 2u)]);

                    s = e1 + e2;

                    {
                        const @T@ p = dat1 * s;

                        h11 = p + _h[(i * _n) + i];
                    }

                    h12 = dat2 * s;
                    h21 = s;
                    h22 = h11;
                }
                else if ((kdefl % kexsh) == 0u)
                {
                    const @T@ e1 = d_internal_math_@S@_abs(
                                       _h[((l + 1u) * _n) + l]);
                    const @T@ e2 = d_internal_math_@S@_abs(
                                       _h[((l + 2u) * _n) + (l + 1u)]);

                    s = e1 + e2;

                    {
                        const @T@ p = dat1 * s;

                        h11 = p + _h[(l * _n) + l];
                    }

                    h12 = dat2 * s;
                    h21 = s;
                    h22 = h11;
                }
                // Francis's: the trailing 2-by-2
                else
                {
                    h11 = _h[((i - 1u) * _n) + (i - 1u)];
                    h21 = _h[(i * _n) + (i - 1u)];
                    h12 = _h[((i - 1u) * _n) + i];
                    h22 = _h[(i * _n) + i];
                }

                {
                    const @T@ a1 = d_internal_math_@S@_abs(h11);
                    const @T@ a2 = d_internal_math_@S@_abs(h12);
                    const @T@ a3 = d_internal_math_@S@_abs(h21);
                    const @T@ a4 = d_internal_math_@S@_abs(h22);
                    const @T@ s1 = a1 + a2;
                    const @T@ s2 = s1 + a3;

                    s = s2 + a4;
                }

                // the shifts: the block's eigenvalues, scaled
                if (s != (@T@)0)
                {
                    @T@ tr     = (@T@)0;
                    @T@ det    = (@T@)0;
                    @T@ rtdisc = (@T@)0;

                    h11 = h11 / s;
                    h21 = h21 / s;
                    h12 = h12 / s;
                    h22 = h22 / s;

                    {
                        const @T@ sum = h11 + h22;

                        tr = sum / (@T@)2;
                    }

                    {
                        const @T@ e1 = h11 - tr;
                        const @T@ e2 = h22 - tr;
                        const @T@ p1 = e1 * e2;
                        const @T@ p2 = h12 * h21;

                        det = p1 - p2;
                    }

                    rtdisc = d_math_@S@_sqrt@F@(
                                 d_internal_math_@S@_abs(det));

                    // complex conjugate shifts
                    if (det >= (@T@)0)
                    {
                        rt1r = tr * s;
                        rt2r = rt1r;
                        rt1i = rtdisc * s;
                        rt2i = -rt1i;
                    }
                    // real shifts: the one nearer h22, twice
                    else
                    {
                        const @T@ x1 = tr + rtdisc;
                        const @T@ x2 = tr - rtdisc;
                        const @T@ d1 = x1 - h22;
                        const @T@ d2 = x2 - h22;

                        if (d_internal_math_@S@_abs(d1) <=
                            d_internal_math_@S@_abs(d2))
                        {
                            rt1r = x1 * s;
                            rt2r = rt1r;
                        }
                        else
                        {
                            rt2r = x2 * s;
                            rt1r = rt2r;
                        }

                        rt1i = (@T@)0;
                        rt2i = (@T@)0;
                    }
                }
            }

            // the row the step starts at: two small subdiagonal entries
            for (m = i - 2u; ; --m)
            {
                const @T@ hmm  = _h[(m * _n) + m];
                const @T@ hm1  = _h[((m + 1u) * _n) + (m + 1u)];
                @T@       h21s = _h[((m + 1u) * _n) + m];
                @T@       s    = (@T@)0;

                {
                    const @T@ e  = hmm - rt2r;
                    const @T@ a1 = d_internal_math_@S@_abs(e);
                    const @T@ a2 = d_internal_math_@S@_abs(rt2i);
                    const @T@ a3 = d_internal_math_@S@_abs(h21s);
                    const @T@ s1 = a1 + a2;

                    s = s1 + a3;
                }

                h21s = _h[((m + 1u) * _n) + m] / s;

                {
                    const @T@ p1 = h21s * _h[(m * _n) + (m + 1u)];
                    const @T@ e1 = hmm - rt1r;
                    const @T@ e2 = hmm - rt2r;
                    const @T@ q2 = e2 / s;
                    const @T@ p2 = e1 * q2;
                    const @T@ q3 = rt2i / s;
                    const @T@ p3 = rt1i * q3;
                    const @T@ s1 = p1 + p2;

                    v[0] = s1 - p3;
                }

                {
                    const @T@ s1 = hmm + hm1;
                    const @T@ s2 = s1 - rt1r;
                    const @T@ s3 = s2 - rt2r;

                    v[1] = h21s * s3;
                }

                v[2] = h21s * _h[((m + 2u) * _n) + (m + 1u)];

                {
                    const @T@ a1 = d_internal_math_@S@_abs(v[0]);
                    const @T@ a2 = d_internal_math_@S@_abs(v[1]);
                    const @T@ a3 = d_internal_math_@S@_abs(v[2]);
                    const @T@ s1 = a1 + a2;

                    s = s1 + a3;
                }

                v[0] = v[0] / s;
                v[1] = v[1] / s;
                v[2] = v[2] / s;

                // the top of the block
                if (m == l)
                {
                    break;
                }

                {
                    const @T@ a0  = d_internal_math_@S@_abs(v[0]);
                    const @T@ a1  = d_internal_math_@S@_abs(v[1]);
                    const @T@ a2  = d_internal_math_@S@_abs(v[2]);
                    const @T@ e1  = d_internal_math_@S@_abs(
                                        _h[(m * _n) + (m - 1u)]);
                    const @T@ e2  = d_internal_math_@S@_abs(
                                        _h[((m - 1u) * _n) + (m - 1u)]);
                    const @T@ e3  = d_internal_math_@S@_abs(hmm);
                    const @T@ e4  = d_internal_math_@S@_abs(hm1);
                    const @T@ s12 = a1 + a2;
                    const @T@ h00 = e1 * s12;
                    const @T@ f1  = ulp * a0;
                    const @T@ g1  = e2 + e3;
                    const @T@ g2  = g1 + e4;
                    const @T@ h01 = f1 * g2;

                    // starting here leaves H(m, m - 1) negligible
                    if (h00 <= h01)
                    {
                        break;
                    }
                }
            }

            // the double-shift step: the bulge made, then chased out
            for (k = m; (k + 1u) <= i; ++k)
            {
                const size_t nr  = ((i - k) + 1u < 3u) ? ((i - k) + 1u) : 3u;
                const bool   three = (nr == 3u);
                @T@          t1  = (@T@)0;
                @T@          v2  = (@T@)0;
                @T@          v3  = (@T@)0;
                @T@          t2  = (@T@)0;
                @T@          t3  = (@T@)0;
                size_t       jj  = 0u;
                size_t       last = 0u;

                // past the first: the bulge's column, to be reflected
                if (k > m)
                {
                    v[0] = _h[(k * _n) + (k - 1u)];
                    v[1] = _h[((k + 1u) * _n) + (k - 1u)];

                    if (three)
                    {
                        v[2] = _h[((k + 2u) * _n) + (k - 1u)];
                    }
                }

                t1 = d_internal_decomp_@S@_reflector@F@(&v[0], &v[1],
                                                        nr - 1u, 1u);

                if (k > m)
                {
                    _h[(k * _n) + (k - 1u)]        = v[0];
                    _h[((k + 1u) * _n) + (k - 1u)] = (@T@)0;

                    if (k < (i - 1u))
                    {
                        _h[((k + 2u) * _n) + (k - 1u)] = (@T@)0;
                    }
                }
                // the first, inside the block: H(k, k - 1) times 1 - t1,
                // which v's underflow cannot spoil
                else if (m > l)
                {
                    const @T@ f = (@T@)1 - t1;

                    _h[(k * _n) + (k - 1u)] = _h[(k * _n) + (k - 1u)] * f;
                }

                v2 = v[1];
                t2 = t1 * v2;

                if (three)
                {
                    v3 = v[2];
                    t3 = t1 * v3;
                }

                // from the left: rows k to k + nr - 1, columns k to i2
                for (jj = k; jj <= i2; ++jj)
                {
                    d_internal_decomp_@S@_reflect3(_h + ((k * _n) + jj), _n,
                                                   three, v2, v3, t1, t2,
                                                   t3);
                }

                // from the right: rows i1 to min(k + 3, i)
                last = three ? (((k + 3u) < i) ? (k + 3u) : i) : i;

                for (jj = i1; jj <= last; ++jj)
                {
                    d_internal_decomp_@S@_reflect3(_h + ((jj * _n) + k), 1u,
                                                   three, v2, v3, t1, t2,
                                                   t3);
                }

                // Z's columns
                if (_z != NULL)
                {
                    for (jj = 0u; jj < _n; ++jj)
                    {
                        d_internal_decomp_@S@_reflect3(_z + ((jj * _n) + k),
                                                       1u, three, v2, v3,
                                                       t1, t2, t3);
                    }
                }
            }
        }

        // one eigenvalue
        if (l == i)
        {
            d_internal_decomp_@S@_put(_values, _real, _complex, i,
                                      _h[(i * _n) + i], (@T@)0);
        }
        // a pair, the block brought to standard form
        else
        {
            struct dj_cplx_@S@ pair[2] = {
                { { (@T@)0, (@T@)0 } },
                { { (@T@)0, (@T@)0 } }
            };
            @T@                cs = (@T@)1;
            @T@                sn = (@T@)0;

            d_internal_decomp_@S@_standard_pair@F@(
                &_h[((i - 1u) * _n) + (i - 1u)],
                &_h[((i - 1u) * _n) + i],
                &_h[(i * _n) + (i - 1u)],
                &_h[(i * _n) + i],
                pair,
                &cs,
                &sn);

            d_internal_decomp_@S@_put(_values, _real, _complex, i - 1u,
                                      pair[0].part[0], pair[0].part[1]);
            d_internal_decomp_@S@_put(_values, _real, _complex, i,
                                      pair[1].part[0], pair[1].part[1]);

            if (wantt)
            {
                // the rest of rows i - 1 and i, then of the columns
                if (i2 > i)
                {
                    d_internal_decomp_@S@_rotate(
                        _h + (((i - 1u) * _n) + (i + 1u)),
                        _h + ((i * _n) + (i + 1u)),
                        i2 - i, 1u, cs, sn);
                }

                d_internal_decomp_@S@_rotate(_h + ((i1 * _n) + (i - 1u)),
                                             _h + ((i1 * _n) + i),
                                             (i - i1) - 1u, _n, cs, sn);
            }

            if (_z != NULL)
            {
                d_internal_decomp_@S@_rotate(_z + (i - 1u), _z + i, _n, _n,
                                             cs, sn);
            }
        }

        kdefl = 0u;
        top   = l;
    }

    return true;
}

// d_decomp_@S@_schur@F@
//   the eigenvalues of the n-by-n upper Hessenberg _h, rows and columns
// _ilo to _ihi its unreduced part (the rest triangular, as balancing leaves
// it), into _values, by Francis's implicit double-shift QR, as LAPACK's
// dlahqr: from the bottom up, a subdiagonal entry negligible by Ahues and
// Kressner's test -- relative to its neighbours, _tol below the family's
// epsilon taken as the epsilon -- set to zero and the block below it split
// off, a block of one its eigenvalue, a block of two brought to standard
// form, a complex pair exactly conjugate and a real eigenvalue's imaginary
// part exactly +0; else a double-shift step on the unreduced block from the
// row where two small subdiagonal entries let it start, its shifts the
// trailing 2-by-2's eigenvalues, a real pair as one of them twice, and at
// every tenth step without a split an exceptional shift. At most _max_iter
// steps in all. With _z NULL only the eigenvalues are found; else _h ends
// in real Schur form, quasi-triangular, its blocks of two standard, and the
// n-by-n _z, Q on entry, is multiplied by each transformation, so that
// Q^T A Q = Z T Z^T. _values receives the eigenvalues in the order of T's
// diagonal, each pair positive first, the rows outside _ilo.._ihi their
// diagonal entries. Returns whether all were found; if not, the rows not
// reached take their diagonal entries.
D_MATH_FN@RT@ bool
d_decomp_@S@_schur@F@(
    @T@*                _h,
    size_t              _n,
    size_t              _ilo,
    size_t              _ihi,
    @T@*                _z,
    struct dj_cplx_@S@* _values,
    size_t              _max_iter,
    @T@                 _tol
)
{
    return d_internal_decomp_@S@_schur@F@(_h, _n, _ilo, _ihi, _z, _values,
                                          NULL, NULL, _max_iter, _tol);
}

// d_decomp_@S@_schur_vectors@F@
//   the right eigenvectors of the n-by-n real Schur form _t, back-
// transformed by the Schur vectors, as LAPACK's dtrevc with job 'B': _vr
// holds Z on entry, and on exit the eigenvectors of Z T Z^T in LAPACK's
// storage -- a real eigenvalue's vector its column, a pair's, for the
// eigenvalue of positive imaginary part, real and imaginary parts in its two
// columns -- each scaled to a largest magnitude of 1 (|re| + |im| for a
// pair). From the last eigenvalue up, each by back-substitution through the
// blocks above it, (T - w I) x = 0 with x's own entry 1, each block's
// system solved by d_internal_decomp_@S@_small_solve, its pivots no smaller
// than ulp times |w|, and x scaled down wherever the next step could
// overflow; then Z x. _work is scratch of 3 n entries.
D_MATH_FN_KERNEL@RT@ void
d_decomp_@S@_schur_vectors@F@(
    const @T@* _t,
    size_t     _n,
    @T@*       _vr,
    @T@*       _work
)
{
    const @T@ ulp    = @EPS@;
    const @T@ smlnum = @MIN@ * ((@T@)_n / ulp);
    const @T@ bignum = ((@T@)1 - ulp) / smlnum;
    @T@*      norms  = _work;
    @T@*      xr     = _work + _n;
    @T@*      xi     = _work + (2u * _n);
    int       ip     = 0;
    size_t    j      = 0u;
    size_t    k      = 0u;
    size_t    r      = 0u;
    size_t    top    = 0u;

    // nothing to find
    if (_n == 0u)
    {
        return;
    }

    // each column's 1-norm above the diagonal, to foresee overflow
    norms[0] = (@T@)0;

    for (j = 1u; j < _n; ++j)
    {
        norms[j] = (@T@)0;

        for (k = 0u; k < j; ++k)
        {
            norms[j] = norms[j] + d_internal_math_@S@_abs(_t[(k * _n) + j]);
        }
    }

    for (top = _n; top > 0u; --top)
    {
        const size_t ki   = top - 1u;
        @T@          wr   = (@T@)0;
        @T@          wi   = (@T@)0;
        @T@          smin = (@T@)0;

        // the first of a pair, found with the second
        if (ip == 1)
        {
            ip = 0;

            continue;
        }

        // the second of a pair
        if ( (ki > 0u) &&
             (_t[(ki * _n) + (ki - 1u)] != (@T@)0) )
        {
            ip = -1;
        }

        wr = _t[(ki * _n) + ki];

        if (ip != 0)
        {
            const @T@ r1 = d_math_@S@_sqrt@F@(
                               d_internal_math_@S@_abs(
                                   _t[(ki * _n) + (ki - 1u)]));
            const @T@ r2 = d_math_@S@_sqrt@F@(
                               d_internal_math_@S@_abs(
                                   _t[((ki - 1u) * _n) + ki]));

            wi = r1 * r2;
        }

        {
            const @T@ a1 = d_internal_math_@S@_abs(wr);
            const @T@ a2 = d_internal_math_@S@_abs(wi);
            const @T@ s  = a1 + a2;
            const @T@ u  = ulp * s;

            smin = (u > smlnum) ? u : smlnum;
        }

        // a real eigenvector
        if (ip == 0)
        {
            size_t jnext = ki;
            size_t jj    = 0u;

            xr[ki] = (@T@)1;

            for (k = 0u; k < ki; ++k)
            {
                xr[k] = -_t[(k * _n) + ki];
            }

            // (T(0..ki-1, 0..ki-1) - wr) x = scale x, block by block
            for (jj = ki; jj > 0u; --jj)
            {
                const size_t j2    = jj - 1u;
                size_t       j1    = j2;
                @T@          x[2]  = { (@T@)0, (@T@)0 };
                @T@          scale = (@T@)1;
                @T@          xnorm = (@T@)0;

                // inside a block already solved
                if ((j2 + 1u) > jnext)
                {
                    continue;
                }

                jnext = j2;

                if ( (j2 > 0u) &&
                     (_t[(j2 * _n) + (j2 - 1u)] != (@T@)0) )
                {
                    j1    = j2 - 1u;
                    jnext = j2 - 1u;
                }

                // a block of one
                if (j1 == j2)
                {
                    d_internal_decomp_@S@_small_solve(
                        1u, 1u, smin, _t + ((j2 * _n) + j2), _n, xr + j2,
                        NULL, wr, (@T@)0, x, NULL, &scale, &xnorm);

                    // x scaled down where the update could overflow
                    if (xnorm > (@T@)1)
                    {
                        const @T@ bound = bignum / xnorm;

                        if (norms[j2] > bound)
                        {
                            x[0]  = x[0] / xnorm;
                            scale = scale / xnorm;
                        }
                    }

                    if (scale != (@T@)1)
                    {
                        for (k = 0u; k <= ki; ++k)
                        {
                            xr[k] = scale * xr[k];
                        }
                    }

                    xr[j2] = x[0];

                    for (k = 0u; k < j2; ++k)
                    {
                        const @T@ p = -x[0] * _t[(k * _n) + j2];

                        xr[k] = xr[k] + p;
                    }
                }
                // a block of two
                else
                {
                    d_internal_decomp_@S@_small_solve(
                        2u, 1u, smin, _t + ((j1 * _n) + j1), _n, xr + j1,
                        NULL, wr, (@T@)0, x, NULL, &scale, &xnorm);

                    if (xnorm > (@T@)1)
                    {
                        const @T@ beta  = (norms[j1] > norms[j2])
                                              ? norms[j1] : norms[j2];
                        const @T@ bound = bignum / xnorm;

                        if (beta > bound)
                        {
                            x[0]  = x[0] / xnorm;
                            x[1]  = x[1] / xnorm;
                            scale = scale / xnorm;
                        }
                    }

                    if (scale != (@T@)1)
                    {
                        for (k = 0u; k <= ki; ++k)
                        {
                            xr[k] = scale * xr[k];
                        }
                    }

                    xr[j1] = x[0];
                    xr[j2] = x[1];

                    for (k = 0u; k < j1; ++k)
                    {
                        const @T@ p = -x[0] * _t[(k * _n) + j1];

                        xr[k] = xr[k] + p;
                    }

                    for (k = 0u; k < j1; ++k)
                    {
                        const @T@ p = -x[1] * _t[(k * _n) + j2];

                        xr[k] = xr[k] + p;
                    }
                }
            }

            // Z x, into the column
            if (ki > 0u)
            {
                const @T@ beta = xr[ki];

                for (r = 0u; r < _n; ++r)
                {
                    if (beta == (@T@)0)
                    {
                        _vr[(r * _n) + ki] = (@T@)0;
                    }
                    else if (beta != (@T@)1)
                    {
                        _vr[(r * _n) + ki] = beta * _vr[(r * _n) + ki];
                    }
                }

                for (k = 0u; k < ki; ++k)
                {
                    const @T@ temp = xr[k];

                    for (r = 0u; r < _n; ++r)
                    {
                        const @T@ p = temp * _vr[(r * _n) + k];

                        _vr[(r * _n) + ki] = _vr[(r * _n) + ki] + p;
                    }
                }
            }

            // the largest magnitude 1
            {
                size_t    best = 0u;
                @T@       remax = (@T@)0;

                for (r = 1u; r < _n; ++r)
                {
                    if (d_internal_math_@S@_abs(_vr[(r * _n) + ki]) >
                        d_internal_math_@S@_abs(_vr[(best * _n) + ki]))
                    {
                        best = r;
                    }
                }

                remax = (@T@)1 /
                        d_internal_math_@S@_abs(_vr[(best * _n) + ki]);

                for (r = 0u; r < _n; ++r)
                {
                    _vr[(r * _n) + ki] = remax * _vr[(r * _n) + ki];
                }
            }
        }
        // a complex pair's eigenvector, for wr + wi i
        else
        {
            size_t jnext = ki - 1u;
            size_t jj    = 0u;

            // the block's own: [T(ki-1..ki, ki-1..ki) - w] x = 0
            if (d_internal_math_@S@_abs(_t[((ki - 1u) * _n) + ki]) >=
                d_internal_math_@S@_abs(_t[(ki * _n) + (ki - 1u)]))
            {
                xr[ki - 1u] = (@T@)1;
                xi[ki]      = wi / _t[((ki - 1u) * _n) + ki];
            }
            else
            {
                xr[ki - 1u] = -wi / _t[(ki * _n) + (ki - 1u)];
                xi[ki]      = (@T@)1;
            }

            xr[ki]      = (@T@)0;
            xi[ki - 1u] = (@T@)0;

            for (k = 0u; (k + 1u) < ki; ++k)
            {
                xr[k] = -xr[ki - 1u] * _t[(k * _n) + (ki - 1u)];
                xi[k] = -xi[ki] * _t[(k * _n) + ki];
            }

            // (T(0..ki-2, 0..ki-2) - w) x = scale x, block by block
            for (jj = ki - 1u; jj > 0u; --jj)
            {
                const size_t j2    = jj - 1u;
                size_t       j1    = j2;
                @T@          x[2]  = { (@T@)0, (@T@)0 };
                @T@          y[2]  = { (@T@)0, (@T@)0 };
                @T@          scale = (@T@)1;
                @T@          xnorm = (@T@)0;

                // inside a block already solved
                if ((j2 + 1u) > jnext)
                {
                    continue;
                }

                jnext = j2;

                if ( (j2 > 0u) &&
                     (_t[(j2 * _n) + (j2 - 1u)] != (@T@)0) )
                {
                    j1    = j2 - 1u;
                    jnext = j2 - 1u;
                }

                // a block of one
                if (j1 == j2)
                {
                    d_internal_decomp_@S@_small_solve(
                        1u, 2u, smin, _t + ((j2 * _n) + j2), _n, xr + j2,
                        xi + j2, wr, wi, x, y, &scale, &xnorm);

                    if (xnorm > (@T@)1)
                    {
                        const @T@ bound = bignum / xnorm;

                        if (norms[j2] > bound)
                        {
                            x[0]  = x[0] / xnorm;
                            y[0]  = y[0] / xnorm;
                            scale = scale / xnorm;
                        }
                    }

                    if (scale != (@T@)1)
                    {
                        for (k = 0u; k <= ki; ++k)
                        {
                            xr[k] = scale * xr[k];
                            xi[k] = scale * xi[k];
                        }
                    }

                    xr[j2] = x[0];
                    xi[j2] = y[0];

                    for (k = 0u; k < j2; ++k)
                    {
                        const @T@ p = -x[0] * _t[(k * _n) + j2];

                        xr[k] = xr[k] + p;
                    }

                    for (k = 0u; k < j2; ++k)
                    {
                        const @T@ p = -y[0] * _t[(k * _n) + j2];

                        xi[k] = xi[k] + p;
                    }
                }
                // a block of two
                else
                {
                    d_internal_decomp_@S@_small_solve(
                        2u, 2u, smin, _t + ((j1 * _n) + j1), _n, xr + j1,
                        xi + j1, wr, wi, x, y, &scale, &xnorm);

                    if (xnorm > (@T@)1)
                    {
                        const @T@ beta  = (norms[j1] > norms[j2])
                                              ? norms[j1] : norms[j2];
                        const @T@ bound = bignum / xnorm;

                        if (beta > bound)
                        {
                            const @T@ rec = (@T@)1 / xnorm;

                            x[0]  = x[0] * rec;
                            y[0]  = y[0] * rec;
                            x[1]  = x[1] * rec;
                            y[1]  = y[1] * rec;
                            scale = scale * rec;
                        }
                    }

                    if (scale != (@T@)1)
                    {
                        for (k = 0u; k <= ki; ++k)
                        {
                            xr[k] = scale * xr[k];
                            xi[k] = scale * xi[k];
                        }
                    }

                    xr[j1] = x[0];
                    xr[j2] = x[1];
                    xi[j1] = y[0];
                    xi[j2] = y[1];

                    for (k = 0u; k < j1; ++k)
                    {
                        const @T@ p = -x[0] * _t[(k * _n) + j1];

                        xr[k] = xr[k] + p;
                    }

                    for (k = 0u; k < j1; ++k)
                    {
                        const @T@ p = -x[1] * _t[(k * _n) + j2];

                        xr[k] = xr[k] + p;
                    }

                    for (k = 0u; k < j1; ++k)
                    {
                        const @T@ p = -y[0] * _t[(k * _n) + j1];

                        xi[k] = xi[k] + p;
                    }

                    for (k = 0u; k < j1; ++k)
                    {
                        const @T@ p = -y[1] * _t[(k * _n) + j2];

                        xi[k] = xi[k] + p;
                    }
                }
            }

            // Z x, into the two columns
            if (ki > 1u)
            {
                const @T@ beta_r = xr[ki - 1u];
                const @T@ beta_i = xi[ki];

                for (r = 0u; r < _n; ++r)
                {
                    if (beta_r == (@T@)0)
                    {
                        _vr[(r * _n) + (ki - 1u)] = (@T@)0;
                    }
                    else if (beta_r != (@T@)1)
                    {
                        _vr[(r * _n) + (ki - 1u)] =
                            beta_r * _vr[(r * _n) + (ki - 1u)];
                    }

                    if (beta_i == (@T@)0)
                    {
                        _vr[(r * _n) + ki] = (@T@)0;
                    }
                    else if (beta_i != (@T@)1)
                    {
                        _vr[(r * _n) + ki] = beta_i * _vr[(r * _n) + ki];
                    }
                }

                for (k = 0u; (k + 1u) < ki; ++k)
                {
                    const @T@ temp = xr[k];

                    for (r = 0u; r < _n; ++r)
                    {
                        const @T@ p = temp * _vr[(r * _n) + k];

                        _vr[(r * _n) + (ki - 1u)] =
                            _vr[(r * _n) + (ki - 1u)] + p;
                    }
                }

                for (k = 0u; (k + 1u) < ki; ++k)
                {
                    const @T@ temp = xi[k];

                    for (r = 0u; r < _n; ++r)
                    {
                        const @T@ p = temp * _vr[(r * _n) + k];

                        _vr[(r * _n) + ki] = _vr[(r * _n) + ki] + p;
                    }
                }
            }
            else
            {
                for (r = 0u; r < _n; ++r)
                {
                    _vr[(r * _n) + (ki - 1u)] =
                        xr[ki - 1u] * _vr[(r * _n) + (ki - 1u)];
                    _vr[(r * _n) + ki] = xi[ki] * _vr[(r * _n) + ki];
                }
            }

            // the largest |re| + |im| 1
            {
                @T@ emax  = (@T@)0;
                @T@ remax = (@T@)0;

                for (r = 0u; r < _n; ++r)
                {
                    const @T@ a1 = d_internal_math_@S@_abs(
                                       _vr[(r * _n) + (ki - 1u)]);
                    const @T@ a2 = d_internal_math_@S@_abs(
                                       _vr[(r * _n) + ki]);
                    const @T@ s  = a1 + a2;

                    if (s > emax)
                    {
                        emax = s;
                    }
                }

                remax = (@T@)1 / emax;

                for (r = 0u; r < _n; ++r)
                {
                    _vr[(r * _n) + (ki - 1u)] =
                        remax * _vr[(r * _n) + (ki - 1u)];
                    _vr[(r * _n) + ki] = remax * _vr[(r * _n) + ki];
                }
            }
        }

        // past the second of a pair: its first comes next, and is done
        if (ip == -1)
        {
            ip = 1;
        }
    }

    return;
}

// d_internal_decomp_@S@_cnorm@F@
//   the Euclidean norm of the _count complex entries _x[0], _x[_stride],
// ...: the root of the sum of their parts' squares, scaled where that
// passes the range or underflows as d_internal_decomp_@S@_norm@F@ scales
// it (S256).
D_MATH_FN@RT@ @T@
d_internal_decomp_@S@_cnorm@F@(
    const struct dj_cplx_@S@* _x,
    size_t                    _count,
    size_t                    _stride
)
{
    const @T@ small   = d_math_@S@_sqrt@F@(@MIN@ / @EPS@);
    @T@       sum     = (@T@)0;
    @T@       norm    = (@T@)0;
    @T@       largest = (@T@)0;
    size_t    i       = 0u;
    size_t    p       = 0u;

    for (i = 0u; i < _count; ++i)
    {
        for (p = 0u; p < 2u; ++p)
        {
            const @T@ v  = _x[i * _stride].part[p];
            const @T@ sq = v * v;

            sum = sum + sq;
        }
    }

    norm = d_math_@S@_sqrt@F@(sum);

    // in range: the sum's root
    if ( (norm <= @MAX@) &&
         (!(norm < small)) )
    {
        return norm;
    }

    // the largest magnitude; a NaN or an infinity, the sum's root
    for (i = 0u; i < _count; ++i)
    {
        for (p = 0u; p < 2u; ++p)
        {
            const @T@ v = d_internal_math_@S@_abs(_x[i * _stride].part[p]);

            if (!(v <= @MAX@))
            {
                return norm;
            }

            if (v > largest)
            {
                largest = v;
            }
        }
    }

    // the zero vector
    if (largest == (@T@)0)
    {
        return norm;
    }

    {
        @T@ pre   = (@T@)1;
        @T@ power = (@T@)1;

        // a subnormal largest: lifted into the normal range first
        if (largest < @MIN@)
        {
            pre     = d_internal_math_@S@_pow2(@P@);
            largest = largest * pre;
        }

        {
            const struct dj_internal_math_@S@_binade b =
                d_internal_math_@S@_binade_of(largest);

            power = b.power;
        }

        sum = (@T@)0;

        for (i = 0u; i < _count; ++i)
        {
            for (p = 0u; p < 2u; ++p)
            {
                const @T@ lifted = _x[i * _stride].part[p] * pre;
                const @T@ v      = lifted / power;
                const @T@ sq     = v * v;

                sum = sum + sq;
            }
        }

        {
            const @T@ root     = d_math_@S@_sqrt@F@(sum);
            const @T@ restored = root * power;

            return restored / pre;
        }
    }
}

// d_internal_decomp_@S@_unit_vectors@F@
//   the complex eigenvectors of the real Schur form _t, from _vr as
// d_decomp_@S@_schur_vectors@F@ and d_decomp_@S@_balance_back
// leave them, into the n-by-n _vectors, column for column: each of unit
// length, its entry of largest modulus (the first among equals) real and
// positive, a real eigenvalue's vector real, its imaginary parts +0, and
// the second of a pair the first's conjugate.
D_MATH_FN_KERNEL@RT@ void
d_internal_decomp_@S@_unit_vectors@F@(
    const @T@*          _t,
    const @T@*          _vr,
    size_t              _n,
    struct dj_cplx_@S@* _vectors
)
{
    size_t j = 0u;
    size_t r = 0u;

    while (j < _n)
    {
        size_t best = 0u;

        // a real eigenvalue's vector
        if ( ((j + 1u) == _n) ||
             (_t[((j + 1u) * _n) + j] == (@T@)0) )
        {
            const @T@ norm = d_internal_decomp_@S@_norm@F@(_vr + j, _n,
                                                           _n);
            bool      flip = false;

            for (r = 1u; r < _n; ++r)
            {
                if (d_internal_math_@S@_abs(_vr[(r * _n) + j]) >
                    d_internal_math_@S@_abs(_vr[(best * _n) + j]))
                {
                    best = r;
                }
            }

            flip = (_vr[(best * _n) + j] < (@T@)0);

            for (r = 0u; r < _n; ++r)
            {
                const @T@ unit = _vr[(r * _n) + j] / norm;

                _vectors[(r * _n) + j].part[0] = flip ? -unit : unit;
                _vectors[(r * _n) + j].part[1] = (@T@)0;
            }

            ++j;

            continue;
        }

        // a pair: columns j and j + 1 the first's real and imaginary parts
        {
            @T@ largest = (@T@)0;
            @T@ modulus = (@T@)0;
            @T@ norm    = (@T@)0;
            @T@ re      = (@T@)0;
            @T@ im      = (@T@)0;

            for (r = 0u; r < _n; ++r)
            {
                const @T@ a  = _vr[(r * _n) + j];
                const @T@ b  = _vr[(r * _n) + (j + 1u)];
                const @T@ aa = a * a;
                const @T@ bb = b * b;
                const @T@ m  = aa + bb;

                if ( (r == 0u) ||
                     (m > largest) )
                {
                    largest = m;
                    best    = r;
                }
            }

            re      = _vr[(best * _n) + j];
            im      = _vr[(best * _n) + (j + 1u)];
            modulus = d_internal_decomp_@S@_hypot@F@(re, im);

            // turned so that the largest entry is real and positive
            {
                const @T@                ur = re / modulus;
                const @T@                ui = im / modulus;
                const struct dj_cplx_@S@ u  = d_cplx_@S@_make(ur, -ui);

                for (r = 0u; r < _n; ++r)
                {
                    if (r == best)
                    {
                        _vectors[(r * _n) + j] =
                            d_cplx_@S@_make(modulus, (@T@)0);
                    }
                    else
                    {
                        _vectors[(r * _n) + j] = d_cplx_@S@_mul(
                            d_cplx_@S@_make(_vr[(r * _n) + j],
                                            _vr[(r * _n) + (j + 1u)]),
                            u);
                    }
                }
            }

            // unit length
            norm = d_internal_decomp_@S@_cnorm@F@(_vectors + j, _n, _n);

            for (r = 0u; r < _n; ++r)
            {
                const @T@ a = _vectors[(r * _n) + j].part[0] / norm;
                const @T@ b = _vectors[(r * _n) + j].part[1] / norm;

                _vectors[(r * _n) + j].part[0]        = a;
                _vectors[(r * _n) + j].part[1]        = b;
                _vectors[(r * _n) + (j + 1u)].part[0] = a;
                _vectors[(r * _n) + (j + 1u)].part[1] = -b;
            }

            _vectors[(best * _n) + j].part[1]        = (@T@)0;
            _vectors[(best * _n) + (j + 1u)].part[1] = (@T@)0;

            j += 2u;
        }
    }

    return;
}

// d_decomp_@S@_eigen@F@
//   the eigenvalues of the general n-by-n _a and, unless _vectors is NULL,
// an eigenvector for each, as LAPACK's dgeev finds them: _a scaled by a
// power of two to a largest magnitude in [1, 2), exactly, so that 2^k A's
// eigenvalues are 2^k times A's bit for bit (barring overflow and
// underflow); balanced (d_decomp_@S@_balance@F@); reduced to Hessenberg
// form (d_decomp_@S@_hessenberg_q@F@); its real Schur form found by
// Francis's double-shift QR (d_decomp_@S@_schur@F@), at most _max_iter
// steps, _tol its deflation's tolerance (the epsilon where smaller); and
// each eigenvector from the Schur form by back-substitution, back-
// transformed, of unit length with its entry of largest modulus real and
// positive (the first among equals). _values receives the eigenvalues,
// ascending by real part, then by imaginary part: a complex pair exactly
// conjugate, side by side, negative first where no real eigenvalue shares
// their real part, a real eigenvalue's imaginary part exactly +0; and
// column j of the row-major n-by-n _vectors the eigenvector of _values[j],
// a real eigenvalue's real, a pair's conjugate. _a is consumed; _work is
// scratch of n (n + 4) entries, or n where _vectors is NULL; _perm of n,
// unused where _vectors is NULL. Returns whether every eigenvalue was found.
// If not, those found are given and the rest the diagonal's entries, and
// every entry of _vectors is a NaN; a NaN or an infinity in _a gives every
// eigenvalue and entry a NaN, and returns false; the zero matrix has zero
// eigenvalues and the identity's columns.
D_MATH_FN_KERNEL@RT@ bool
d_decomp_@S@_eigen@F@(
    @T@*                _a,
    size_t              _n,
    struct dj_cplx_@S@* _values,
    struct dj_cplx_@S@* _vectors,
    @T@*                _work,
    size_t*             _perm,
    size_t              _max_iter,
    @T@                 _tol
)
{
    const struct dj_internal_decomp_@S@_scaling sc =
        d_internal_decomp_@S@_scaling_of(_a, _n * _n);
    @T@*   scale     = _work;
    @T@*   z         = NULL;
    size_t ilo       = 0u;
    size_t ihi       = 0u;
    size_t i         = 0u;
    size_t k         = 0u;
    bool   converged = true;

    // nothing to find
    if (_n == 0u)
    {
        return true;
    }

    // a NaN or an infinity: no eigenvalue, no eigenvector
    if (!sc.finite)
    {
        for (i = 0u; i < _n; ++i)
        {
            _values[i].part[0] = d_internal_math_@S@_nan();
            _values[i].part[1] = d_internal_math_@S@_nan();
        }

        if (_vectors != NULL)
        {
            for (i = 0u; i < (_n * _n); ++i)
            {
                _vectors[i].part[0] = d_internal_math_@S@_nan();
                _vectors[i].part[1] = d_internal_math_@S@_nan();
            }
        }

        return false;
    }

    // the zero matrix: zeros, and the identity's columns
    if (sc.zero)
    {
        for (i = 0u; i < _n; ++i)
        {
            _values[i].part[0] = (@T@)0;
            _values[i].part[1] = (@T@)0;
        }

        if (_vectors != NULL)
        {
            for (i = 0u; i < _n; ++i)
            {
                for (k = 0u; k < _n; ++k)
                {
                    _vectors[(i * _n) + k].part[0] =
                        (i == k) ? (@T@)1 : (@T@)0;
                    _vectors[(i * _n) + k].part[1] = (@T@)0;
                }
            }
        }

        return true;
    }

    // the largest magnitude into [1, 2), exactly
    for (i = 0u; i < (_n * _n); ++i)
    {
        const @T@ lifted = _a[i] * sc.pre;

        _a[i] = lifted / sc.power;
    }

    d_decomp_@S@_balance@F@(_a, _n, _perm, scale, &ilo, &ihi);

    // Q, then Z, from the identity
    if (_vectors != NULL)
    {
        z = _work + _n;

        for (i = 0u; i < _n; ++i)
        {
            for (k = 0u; k < _n; ++k)
            {
                z[(i * _n) + k] = (i == k) ? (@T@)1 : (@T@)0;
            }
        }
    }

    d_decomp_@S@_hessenberg_q@F@(_a, _n, ilo, ihi, z);

    converged = d_internal_decomp_@S@_schur@F@(_a, _n, ilo, ihi, z, _values,
                                               NULL, NULL, _max_iter, _tol);

    // the eigenvectors, or NaNs where the iteration stopped short
    if (_vectors != NULL)
    {
        if (converged)
        {
            d_decomp_@S@_schur_vectors@F@(_a, _n, z,
                                                   z + (_n * _n));
            d_decomp_@S@_balance_back(z, _n, _n, _perm, scale, ilo, ihi);
            d_internal_decomp_@S@_unit_vectors@F@(_a, z, _n, _vectors);
        }
        else
        {
            for (i = 0u; i < (_n * _n); ++i)
            {
                _vectors[i].part[0] = d_internal_math_@S@_nan();
                _vectors[i].part[1] = d_internal_math_@S@_nan();
            }
        }
    }

    // back to the matrix's scale: exact unless a value overflows or
    // underflows
    for (i = 0u; i < _n; ++i)
    {
        const @T@ re = _values[i].part[0] * sc.power;
        const @T@ im = _values[i].part[1] * sc.power;

        _values[i].part[0] = re / sc.pre;
        _values[i].part[1] = im / sc.pre;
    }

    // ascending by real part, then imaginary, the vectors' columns along
    for (i = 0u; i < _n; ++i)
    {
        size_t sel = i;

        for (k = i + 1u; k < _n; ++k)
        {
            const bool lower = (_values[k].part[0] < _values[sel].part[0]);
            const bool level = (_values[k].part[0] == _values[sel].part[0]);
            const bool below = (_values[k].part[1] < _values[sel].part[1]);

            if ( (lower) ||
                 ((level) && (below)) )
            {
                sel = k;
            }
        }

        if (sel != i)
        {
            const struct dj_cplx_@S@ tmp = _values[i];

            _values[i]   = _values[sel];
            _values[sel] = tmp;

            if (_vectors != NULL)
            {
                for (k = 0u; k < _n; ++k)
                {
                    const struct dj_cplx_@S@ t = _vectors[(k * _n) + i];

                    _vectors[(k * _n) + i]   = _vectors[(k * _n) + sel];
                    _vectors[(k * _n) + sel] = t;
                }
            }
        }
    }

    return converged;
}

// d_decomp_@S@_eigenvalues@F@
//   the eigenvalues of the general n-by-n _h, ascending, into _values, as
// d_decomp_@S@_eigen@F@ finds them -- scaled by a power of two, balanced,
// reduced to Hessenberg form, and Francis's double-shift QR, at most
// _max_iter steps, _tol its deflation's tolerance (the epsilon where
// smaller) -- their real parts: the same bits as its real eigenvalues.
// _h is consumed. Returns whether every eigenvalue was found and real. A
// complex pair gives its real part twice, the rest of the spectrum still
// found; a spent budget gives the diagonal for what is left unsolved; a NaN
// or an infinity in _h gives NaN for every eigenvalue. All three return
// false.
D_MATH_FN@RT@ bool
d_decomp_@S@_eigenvalues@F@(
    @T@*   _h,
    size_t _n,
    @T@*   _values,
    size_t _max_iter,
    @T@    _tol
)
{
    const struct dj_internal_decomp_@S@_scaling sc =
        d_internal_decomp_@S@_scaling_of(_h, _n * _n);
    size_t ilo       = 0u;
    size_t ihi       = 0u;
    size_t i         = 0u;
    bool   converged = true;
    bool   complex   = false;

    // a NaN or an infinity: no eigenvalue
    if (!sc.finite)
    {
        for (i = 0u; i < _n; ++i)
        {
            _values[i] = d_internal_math_@S@_nan();
        }

        return false;
    }

    // the zero matrix
    if (sc.zero)
    {
        for (i = 0u; i < _n; ++i)
        {
            _values[i] = (@T@)0;
        }

        return true;
    }

    // the largest magnitude into [1, 2), exactly
    for (i = 0u; i < (_n * _n); ++i)
    {
        const @T@ lifted = _h[i] * sc.pre;

        _h[i] = lifted / sc.power;
    }

    // balanced: _values holds the scaling until the eigenvalues come
    d_decomp_@S@_balance@F@(_h, _n, NULL, _values, &ilo, &ihi);
    d_decomp_@S@_hessenberg_q@F@(_h, _n, ilo, ihi, NULL);

    converged = d_internal_decomp_@S@_schur@F@(_h, _n, ilo, ihi, NULL, NULL,
                                               _values, &complex, _max_iter,
                                               _tol);

    // back to the matrix's scale: exact unless a value overflows or
    // underflows
    for (i = 0u; i < _n; ++i)
    {
        const @T@ restored = _values[i] * sc.power;

        _values[i] = restored / sc.pre;
    }

    // ascending
    for (i = 0u; i < _n; ++i)
    {
        size_t sel = i;
        size_t k   = 0u;

        for (k = i + 1u; k < _n; ++k)
        {
            if (_values[k] < _values[sel])
            {
                sel = k;
            }
        }

        if (sel != i)
        {
            const @T@ tmp = _values[i];

            _values[i]   = _values[sel];
            _values[sel] = tmp;
        }
    }

    return ( (converged) &&
             (!complex) );
}

// d_decomp_@S@_eigenvector@F@
//   an eigenvector of the n-by-n _a for its eigenvalue _lambda into _x, of
// unit length, its largest entry (the first, among equals) positive, by
// inverse iteration on _a scaled as d_decomp_@S@_eigenvalues@F@ scales it:
// shifted past the scaled eigenvalue by 4 epsilon times its magnitude (at
// least 4 epsilon), so that the shift differs from it in the family's own
// precision, and moved 16 times further, up to three times, while the
// shifted matrix is exactly singular; then U x = e solved first, e all
// ones, as LAPACK's dlaein does -- in effect a start of P^T L e, which
// follows the shifted matrix, where any fixed start, e itself among them,
// is orthogonal to some eigenvectors -- and two full solves, each iterate
// divided by its largest magnitude. _lu (n by n), _perm (n) and _y (n) are
// scratch. Returns false where there is nothing to find -- _a or _lambda
// not finite, _a zero, the shifted matrix singular at every shift tried, or
// an iterate overflowing or vanishing -- and _x is then no eigenvector.
D_MATH_FN@RT@ bool
d_decomp_@S@_eigenvector@F@(
    const @T@* _a,
    size_t     _n,
    @T@        _lambda,
    @T@*       _lu,
    size_t*    _perm,
    @T@*       _x,
    @T@*       _y
)
{
    const struct dj_internal_decomp_@S@_scaling sc =
        d_internal_decomp_@S@_scaling_of(_a, _n * _n);
    const @T@ four_eps = (@T@)4 * @EPS@;
    @T@       sign     = (@T@)1;
    @T@       lam      = (@T@)0;
    @T@       delta    = (@T@)0;
    bool   singular = true;
    size_t attempt  = 0u;
    size_t step     = 0u;
    size_t i        = 0u;

    // nothing to find: a NaN or an infinity, the zero matrix, or no value
    if ( (!sc.finite) ||
         (sc.zero) ||
         (_lambda != _lambda) ||
         (d_internal_math_@S@_abs(_lambda) > @MAX@) )
    {
        return false;
    }

    {
        const @T@ lifted = _lambda * sc.pre;

        lam = lifted / sc.power;

        {
            const @T@ mag = d_internal_math_@S@_abs(lam);

            delta = four_eps * ((mag > (@T@)1) ? mag : (@T@)1);
        }
    }

    // A - (lambda + delta) I, scaled; a shift moved on while exactly
    // singular
    for (attempt = 0u; (attempt < 4u) && singular; ++attempt)
    {
        const @T@ shift = lam + delta;

        for (i = 0u; i < (_n * _n); ++i)
        {
            const @T@ lifted = _a[i] * sc.pre;

            _lu[i] = lifted / sc.power;
        }

        for (i = 0u; i < _n; ++i)
        {
            _lu[(i * _n) + i] = _lu[(i * _n) + i] - shift;
        }

        singular = d_decomp_@S@_lu(_lu, _n, _perm, &sign);
        delta    = delta * (@T@)16;
    }

    // singular at every shift
    if (singular)
    {
        return false;
    }

    // the first step: U x = e
    for (i = 0u; i < _n; ++i)
    {
        _y[i] = (@T@)1;
    }

    d_decomp_@S@_back_sub(_lu, _n, _y, _x);

    for (step = 0u; step < 3u; ++step)
    {
        @T@ largest = (@T@)0;

        // the iterate's largest magnitude: a NaN or an infinity ends it
        for (i = 0u; i < _n; ++i)
        {
            const @T@ v = d_internal_math_@S@_abs(_x[i]);

            if ( (v != v) ||
                 (v > @MAX@) )
            {
                return false;
            }

            if (v > largest)
            {
                largest = v;
            }
        }

        // the zero vector: no direction
        if (largest == (@T@)0)
        {
            return false;
        }

        for (i = 0u; i < _n; ++i)
        {
            _x[i] = _x[i] / largest;
        }

        // two full solves after the first
        if (step < 2u)
        {
            d_decomp_@S@_lu_solve(_lu, _perm, _n, _x, _y);

            for (i = 0u; i < _n; ++i)
            {
                _x[i] = _y[i];
            }
        }
    }

    {
        @T@    norm_sq = (@T@)0;
        @T@    norm    = (@T@)0;
        size_t top     = 0u;

        for (i = 0u; i < _n; ++i)
        {
            const @T@ sq = _x[i] * _x[i];

            norm_sq = norm_sq + sq;

            if (d_internal_math_@S@_abs(_x[i]) >
                d_internal_math_@S@_abs(_x[top]))
            {
                top = i;
            }
        }

        norm = d_math_@S@_sqrt@F@(norm_sq);

        {
            const bool flip = (_x[top] < (@T@)0);

            // unit length, the largest entry positive
            for (i = 0u; i < _n; ++i)
            {
                const @T@ unit = _x[i] / norm;

                _x[i] = flip ? -unit : unit;
            }
        }
    }

    return true;
}
"""

DECOMP_POP = r"""
// gcc's -Warray-bounds, restored
#if ( defined(__GNUC__) &&                                                    \
      !defined(__clang__) )
    #pragma GCC diagnostic pop
#endif
"""


DECOMP_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE FLOAT FAMILY (_f)
    ---------------------
    1.  LU, with partial pivoting
    2.  QR, the SVD, the symmetric eigensolver and Cholesky
    3.  The eigenvalues and eigenvectors of a general matrix
2.  THE DOUBLE FAMILY (_d)
    ----------------------
    1.  LU, with partial pivoting
    2.  QR, the SVD, the symmetric eigensolver and Cholesky
    3.  The eigenvalues and eigenvectors of a general matrix
3.  THE LONG DOUBLE FAMILY (_ld)
    ----------------------------
    1.  LU, with partial pivoting
    2.  QR, the SVD, the symmetric eigensolver and Cholesky
    3.  The eigenvalues and eigenvectors of a general matrix
4.  ONE NAME FOR EVERY FAMILY (C++)
    -------------------------------
*/

#ifndef DJINTERP_C_MATH_DECOMP_H
#define DJINTERP_C_MATH_DECOMP_H 1

// std
#include <float.h>           // FLT_EPSILON ... LDBL_MAX: the families'
                             // limits
#include <stddef.h>          // size_t
// djinterp
#include "./cplx.h"          // struct dj_cplx_*, d_cplx_*_make, _mul and
                             // _div: the eigenvalues and eigenvectors
#include "./elementary.h"    // d_math_*_sqrt and its _c form,
                             // d_internal_math_*_abs, d_internal_math_*_nan
#include "./float_order.h"   // d_internal_math_*_binade_of,
                             // d_internal_math_*_pow2
#include "./math_common.h"   // D_MATH_FN
"""

DECOMP_TAIL = r"""

#endif  // DJINTERP_C_MATH_DECOMP_H
"""

DECOMP_OVERLOADS = [
    ("D_MATH_FN", "bool", "d_decomp_lu", [("@T@*", "_lu"), ("size_t", "_n"), ("size_t*", "_perm"), ("@T@*", "_sign")]),
    ("D_MATH_FN", "void", "d_decomp_lu_solve", [("const @T@*", "_lu"), ("const size_t*", "_perm"), ("size_t", "_n"), ("const @T@*", "_b"), ("@T@*", "_x")]),
    ("D_MATH_FN", "@T@", "d_decomp_lu_det", [("const @T@*", "_lu"), ("size_t", "_n"), ("@T@", "_sign")]),
    ("D_MATH_FN", "void", "d_decomp_cholesky_solve", [("const @T@*", "_l"), ("size_t", "_n"), ("const @T@*", "_b"), ("@T@*", "_x")]),
    ("D_MATH_FN", "@T@", "d_decomp_cholesky_det", [("const @T@*", "_l"), ("size_t", "_n")]),
    ("D_MATH_FN", "void", "d_decomp_forward_sub", [("const @T@*", "_l"), ("size_t", "_n"), ("const @T@*", "_b"), ("@T@*", "_x")]),
    ("D_MATH_FN", "void", "d_decomp_back_sub", [("const @T@*", "_u"), ("size_t", "_n"), ("const @T@*", "_b"), ("@T@*", "_x")]),
    ("D_MATH_FN", "void", "d_decomp_qt_mul", [("const @T@*", "_q"), ("size_t", "_m"), ("size_t", "_n"), ("const @T@*", "_b"), ("@T@*", "_c")]),
    ("D_MATH_FN", "void", "d_decomp_balance_back", [("@T@*", "_v"), ("size_t", "_n"), ("size_t", "_m"), ("const size_t*", "_perm"), ("const @T@*", "_scale"), ("size_t", "_ilo"), ("size_t", "_ihi")]),
]
for _f, _m in (("_c", "D_MATH_FN"), ("", "D_MATH_FN_RT")):
    DECOMP_OVERLOADS += [
        (_m, "void", "d_decomp_qr" + _f, [("@T@*", "_q"), ("@T@*", "_r"), ("size_t", "_m"), ("size_t", "_n"), ("@T@*", "_v")]),
        (_m, "bool", "d_decomp_cholesky" + _f, [("@T@*", "_l"), ("const @T@*", "_a"), ("size_t", "_n")]),
        (_m, "void", "d_decomp_svd" + _f, [("@T@*", "_work"), ("@T@*", "_u"), ("@T@*", "_s"), ("@T@*", "_v"), ("size_t", "_m"), ("size_t", "_n"), ("@T@", "_tol")]),
        (_m, "void", "d_decomp_sym_eigen" + _f, [("@T@*", "_work"), ("@T@*", "_values"), ("@T@*", "_vectors"), ("size_t", "_n"), ("@T@", "_threshold")]),
        (_m, "void", "d_decomp_hessenberg" + _f, [("@T@*", "_h"), ("size_t", "_n")]),
        (_m, "bool", "d_decomp_eigenvalues" + _f, [("@T@*", "_h"), ("size_t", "_n"), ("@T@*", "_values"), ("size_t", "_max_iter"), ("@T@", "_tol")]),
        (_m, "bool", "d_decomp_eigenvector" + _f, [("const @T@*", "_a"), ("size_t", "_n"), ("@T@", "_lambda"), ("@T@*", "_lu"), ("size_t*", "_perm"), ("@T@*", "_x"), ("@T@*", "_y")]),
        (_m, "void", "d_decomp_balance" + _f, [("@T@*", "_a"), ("size_t", "_n"), ("size_t*", "_perm"), ("@T@*", "_scale"), ("size_t*", "_ilo"), ("size_t*", "_ihi")]),
        (_m, "void", "d_decomp_hessenberg_q" + _f, [("@T@*", "_h"), ("size_t", "_n"), ("size_t", "_ilo"), ("size_t", "_ihi"), ("@T@*", "_q")]),
        (_m, "bool", "d_decomp_schur" + _f, [("@T@*", "_h"), ("size_t", "_n"), ("size_t", "_ilo"), ("size_t", "_ihi"), ("@T@*", "_z"), ("struct dj_cplx_@S@*", "_values"), ("size_t", "_max_iter"), ("@T@", "_tol")]),
        (_m, "void", "d_decomp_schur_vectors" + _f, [("const @T@*", "_t"), ("size_t", "_n"), ("@T@*", "_vr"), ("@T@*", "_work")]),
        (_m, "bool", "d_decomp_eigen" + _f, [("@T@*", "_a"), ("size_t", "_n"), ("struct dj_cplx_@S@*", "_values"), ("struct dj_cplx_@S@*", "_vectors"), ("@T@*", "_work"), ("size_t*", "_perm"), ("size_t", "_max_iter"), ("@T@", "_tol")]),
    ]


def decomp():
    lines = [
        "Decompositions: the C core of linear_algebra/decomposition.hpp,",
        "solve.hpp, svd.hpp and eigen.hpp -- LU with partial pivoting and its",
        "solve and determinant, Householder QR, Cholesky and its solve and",
        "determinant, triangular substitution, Q^T b for least squares, the",
        "one-sided Jacobi SVD and the cyclic Jacobi symmetric eigensolver, and",
        "for a general matrix LAPACK's path to its eigenvalues and",
        "eigenvectors: balancing, the Hessenberg form, the real Schur form by",
        "Francis's double-shift QR, the eigenvectors from it, the eigenvalues",
        "complex numbers of cplx.h -- and an eigenvector by inverse iteration,",
        "for float, double and long double.",
        "  GENERATED by scripts/gen_c_math_float.py from one text for the three",
        "families: edit the generator, not this file.",
        "",
        "  Matrices are row-major arrays, the faces' matrices' storage. LU and",
        "the solves need no root and come once; what takes a root comes twice,",
        "as the elementary functions do: the _c form a constant expression from",
        "C++14, the plain one on the C library's root.",
    ]
    text = banner("decomp.h", lines, DATE, "2026.11.07")
    text += "\n" + DECOMP_HEAD
    for i, (suffix, ctype, prefix, prose) in enumerate(FAMILIES):
        upper = prose.upper()
        forms = ""
        eigen_forms = ""
        for f, rt in (("_c", ""), ("", "_RT")):
            forms += DECOMP_FORMS.replace("@F@", f).replace("@RT@", rt)
            eigen_forms += DECOMP_EIGEN_FORMS.replace("@F@", f).replace("@RT@", rt)
        body = DECOMP_TEMPLATE.replace("@FORMS@", forms)
        body = body.replace("@EIGEN_FORMS@", eigen_forms + DECOMP_POP)
        body = body.replace("@SECTION@", str(i + 1)).replace("@UPPER@", upper)
        text += instantiate(body, suffix, ctype, prefix, prose)
    text += overload_section("4.  ONE NAME FOR EVERY FAMILY (C++)", DECOMP_OVERLOADS)
    text += DECOMP_TAIL
    return text


# -----------------------------------------------------------------------------
# cplx.h
# -----------------------------------------------------------------------------
# Complex numbers (S265 to S270): a struct per family whose one member is an
# array of the two parts, so that it has the layout of T[2] that C99's
# _Complex T and C++'s std::complex<T> share; the arithmetic once per family,
# pure, with real, imag, conj and proj, which are exact; and C99's other 18
# functions twice, as every real function here is -- the _c form on one long
# double kernel of pure arithmetic, rounded once to the family, and the plain
# form on a kernel of each family's own type on the C library's real
# functions. One kernel text serves both: @W@ is its working
# type, @KS@ its names' suffix and @R@ the suffix of the real functions it
# calls.

CPLX_TYPES = r"""
//==============================================================================
// 1.  THE TYPES
//==============================================================================
// One struct per family, of one member: the two parts, real first, as an
// array, so that the struct has the layout of T[2] -- no padding lies between
// an array's elements, as it may between a struct's members -- which C99's
// _Complex T (6.2.5) and C++'s std::complex<T> ([complex.numbers]) both
// promise. A value of either converts to one of these by a copy of its bytes.
// No typedef hides the struct (the C guide's rule): it is spelled struct
// dj_cplx_d.


// 1.1    The complex types
//------------------------------------------------------------------------------
@STRUCTS@
"""

CPLX_STRUCT = r"""// 1.1.@N@
// dj_cplx_@S@
//   struct: a complex number of @NAME@ parts: part[0] the real part,
// part[1] the imaginary.
struct dj_cplx_@S@
{
    @T@ part[2];
};

D_STATIC_ASSERT( (sizeof(struct dj_cplx_@S@) == (2 * sizeof(@T@))),
                 "struct dj_cplx_@S@ has the layout of @T@[2]" );
"""

# the arithmetic and its helpers, once per family: pure, so one form
CPLX_ARITH = r"""
// @SECTION@.@SUB@    The @NAME@ family (_@S@)
//------------------------------------------------------------------------------
// The helpers first -- an exact product and sum, a power-of-2 scaling that
// never overflows on its way, the sign of a zero -- then the operations. A
// product's parts are each an exact product less or plus another, by
// Dekker's product and two-sum, rounded once (Kahan's way, its error within
// 2 ulps of each part); a quotient multiplies by the conjugate over the
// squared modulus, each sum so formed, the denominator's low part kept. The
// operands are first scaled by powers of 2 so that no square or product
// overflows or loses its low part to underflow, and the result scaled back
// once. A zero part takes the sign the textbook formula gives it, a
// quotient's as a product's (S382), and an infinity C99's Annex G's
// recovery (G.5.1, its examples), so that an infinite operand gives an
// infinity where the formula's terms are NaN.

// d_internal_cplx_@S@_make
//   the complex number _x + _y i.
D_MATH_FN struct dj_cplx_@S@
d_internal_cplx_@S@_make(
    @T@ _x,
    @T@ _y
)
{
    struct dj_cplx_@S@ result = { { (@T@)0, (@T@)0 } };

    result.part[0] = _x;
    result.part[1] = _y;

    return result;
}

// d_internal_cplx_@S@_is_inf
//   whether _x is an infinity of either sign.
D_MATH_FN bool
d_internal_cplx_@S@_is_inf(
    @T@ _x
)
{
    return ( (_x > @MAX@) ||
             (_x < -@MAX@) );
}

// d_internal_cplx_@S@_copysign
//   |_x| with the sign of _s, a zero's and a NaN's sign bit among them as
// tangent.h's negative reads them.
D_MATH_FN @T@
d_internal_cplx_@S@_copysign(
    @T@ _x,
    @T@ _s
)
{
    const @T@ magnitude = d_internal_math_@S@_negative(_x) ? -_x : _x;

    return d_internal_math_@S@_negative(_s) ? -magnitude : magnitude;
}

// d_internal_cplx_@S@_abs
//   |_x| by comparison, a constant expression, and +0 for either zero, as a
// magnitude is: elementary.h's own leaves a -0 as it is, which a part built
// of a magnitude would carry.
D_MATH_FN @T@
d_internal_cplx_@S@_abs(
    @T@ _x
)
{
    return (_x < (@T@)0) ? -_x : (_x + (@T@)0);
}

// d_internal_cplx_@S@_two_sum
//   _a + _b exactly, as high + low: Knuth's two-sum.
D_MATH_FN struct dj_internal_math_@S@_pair
d_internal_cplx_@S@_two_sum(
    @T@ _a,
    @T@ _b
)
{
    struct dj_internal_math_@S@_pair result = { (@T@)0, (@T@)0 };
    const @T@ high   = _a + _b;
    const @T@ a_part = high - _b;
    const @T@ b_part = high - a_part;

    result.high = high;
    result.low  = (_a - a_part) + (_b - b_part);

    return result;
}

// d_internal_cplx_@S@_product
//   _a * _b exactly, as high + low: Dekker's product, each factor split by
// Veltkamp's constant (float_order.h). The factors are within 2^(EMAX/2 - P)
// in magnitude and their product, where it is not 0, at least 2^(EMIN + 2P),
// as the scalings below keep them, so that nothing overflows and the low
// part is exact.
D_MATH_FN struct dj_internal_math_@S@_pair
d_internal_cplx_@S@_product(
    @T@ _a,
    @T@ _b
)
{
    struct dj_internal_math_@S@_pair result = { (@T@)0, (@T@)0 };
    const @T@ split = d_internal_math_@S@_veltkamp();
    const @T@ ta    = split * _a;
    const @T@ a_hi  = ta - (ta - _a);
    const @T@ a_lo  = _a - a_hi;
    const @T@ tb    = split * _b;
    const @T@ b_hi  = tb - (tb - _b);
    const @T@ b_lo  = _b - b_hi;
    const @T@ high  = _a * _b;

    result.high = high;
    result.low  = ( (((a_hi * b_hi) - high) + (a_hi * b_lo)) +
                    (a_lo * b_hi) ) +
                  (a_lo * b_lo);

    return result;
}

#if @WIDE@
// d_internal_cplx_@S@_dot_wide
//   _a _b + _c _d as a long double pair, high the sum rounded to long double,
// where the target evaluates @T@ wider than its type, as a 32-bit x87 does:
// there Dekker's split and the two-sum, which need each operation rounded to
// @T@, cannot be made to have it in pure arithmetic -- a value may keep the
// register's bits or lose them as the compiler spills it -- so the products
// are taken exactly in long double, whose operations the target rounds in
// their own type, and their sum as one more pair.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_cplx_@S@_dot_wide(
    @T@ _a,
    @T@ _b,
    @T@ _c,
    @T@ _d
)
{
    const struct dj_internal_math_ld_pair p =
        d_internal_math_ld_product_exact((long double)_a, (long double)_b);
    const struct dj_internal_math_ld_pair q =
        d_internal_math_ld_product_exact((long double)_c, (long double)_d);
    const struct dj_internal_math_ld_pair s =
        d_internal_math_ld_two_sum(p.high, q.high);
    struct dj_internal_math_ld_pair t =
        d_internal_math_ld_two_sum(s.high, s.low + (p.low + q.low));

    // an exact zero: the formula's own, with its sign
    if (t.high == 0.0L)
    {
        t.high = (long double)((_a * _b) + (_c * _d));
    }

    return t;
}

// d_internal_cplx_@S@_over_wide
//   (_u.high + _u.low) / (_n.high + _n.low), _n.high in [1, 8), rounded once
// to @T@, as d_internal_cplx_@S@_over takes it, in long double: a zero q
// itself, of its sign (S382).
D_MATH_FN @T@
d_internal_cplx_@S@_over_wide(
    struct dj_internal_math_ld_pair _u,
    struct dj_internal_math_ld_pair _n
)
{
    const long double q0    = _u.high / _n.high;
    const long double least =
        LDBL_MIN * d_internal_math_ld_pow2(LDBL_MANT_DIG + 4);

    // a zero: itself, its sign kept
    if (q0 == 0.0L)
    {
        return (@T@)q0;
    }

    // far below what @T@ holds: the quotient as it is
    if ( (q0 < least) &&
         (q0 > -least) )
    {
        return (@T@)(q0 - (q0 * (_n.low / _n.high)));
    }

    {
        const struct dj_internal_math_ld_pair t =
            d_internal_math_ld_product_exact(q0, _n.high);
        const long double head = (_u.high - t.high) - t.low;
        const long double r    = (head + _u.low) - (q0 * _n.low);
        const struct dj_internal_math_ld_pair v =
            d_internal_math_ld_two_sum(q0, r / _n.high);

        return d_internal_math_@S@_round_pair(v.high, v.low);
    }
}

#endif  // @WIDE@
// d_internal_cplx_@S@_dot_pair
//   _a _b + _c _d as high + low, high the sum rounded once from its exact
// terms: the two products exact, their high parts summed exactly, and the
// rest below. Where the exact sum is 0 the textbook formula's zero is taken,
// so that its sign is IEEE's.
D_MATH_FN struct dj_internal_math_@S@_pair
d_internal_cplx_@S@_dot_pair(
    @T@ _a,
    @T@ _b,
    @T@ _c,
    @T@ _d
)
{
    const struct dj_internal_math_@S@_pair p =
        d_internal_cplx_@S@_product(_a, _b);
    const struct dj_internal_math_@S@_pair q =
        d_internal_cplx_@S@_product(_c, _d);
    const struct dj_internal_math_@S@_pair s =
        d_internal_cplx_@S@_two_sum(p.high, q.high);
    struct dj_internal_math_@S@_pair t =
        d_internal_cplx_@S@_two_sum(s.high, s.low + (p.low + q.low));

    // an exact zero: the formula's own, with its sign
    if (t.high == (@T@)0)
    {
        t.high = (_a * _b) + (_c * _d);
    }

    return t;
}

// d_internal_cplx_@S@_dot
//   _a _b + _c _d, rounded once from its exact terms, as the pair's high
// part (d_internal_cplx_@S@_dot_pair); where a target evaluates float and
// double wider than their types, as a 32-bit x87 does, the long double pair
// of d_internal_cplx_@S@_dot_wide rounded once.
D_MATH_FN @T@
d_internal_cplx_@S@_dot(
    @T@ _a,
    @T@ _b,
    @T@ _c,
    @T@ _d
)
{
#if @WIDE@
    const struct dj_internal_math_ld_pair t =
        d_internal_cplx_@S@_dot_wide(_a, _b, _c, _d);

    return d_internal_math_@S@_round_pair(t.high, t.low);
#else  // @WIDE@
    return d_internal_cplx_@S@_dot_pair(_a, _b, _c, _d).high;
#endif  // @WIDE@
}

// d_internal_cplx_@S@_over
//   (_u.high + _u.low) / (_n.high + _n.low), _n.high in [1, 8): the quotient
// q of the high parts, then the residual u - q n formed exactly of Dekker's
// product of q and n's high part, and q plus the residual over n, rounded
// once, so that the quotient is within a step of the correctly rounded one.
// A zero is q itself, the numerator's formula's zero over n, of its sign
// (S382); a quotient too near the subnormals for Dekker's product is q less
// q n_lo / n.
D_MATH_FN @T@
d_internal_cplx_@S@_over(
    struct dj_internal_math_@S@_pair _u,
    struct dj_internal_math_@S@_pair _n
)
{
    const @T@ q0    = _u.high / _n.high;
    const @T@ least =
        @MIN@ * d_internal_math_@S@_pow2(@P@ + 4);

    // a zero: itself, its sign kept
    if (q0 == (@T@)0)
    {
        return q0;
    }

    // too near the subnormals for Dekker's product
    if (d_internal_cplx_@S@_abs(q0) < least)
    {
        return q0 - (q0 * (_n.low / _n.high));
    }

    {
        const struct dj_internal_math_@S@_pair t =
            d_internal_cplx_@S@_product(q0, _n.high);
        // u - q n: u's high part less t's is exact, the two within an ulp
        const @T@ head = (_u.high - t.high) - t.low;
        const @T@ r    = (head + _u.low) - (q0 * _n.low);

        return q0 + (r / _n.high);
    }
}

// d_internal_cplx_@S@_exponent
//   the binade E of a finite nonzero _x, 2^E <= |_x| < 2^(E + 1): a
// subnormal scaled into the normal range first.
D_MATH_FN int
d_internal_cplx_@S@_exponent(
    @T@ _x
)
{
    const @T@ a    = d_internal_cplx_@S@_abs(_x);
    const int lift = (a < @MIN@) ? @P@ : 0;
    const @T@ v    = (lift > 0) ? (a * d_internal_math_@S@_pow2(lift)) : a;

    return d_internal_math_@S@_binade_of(v).exponent - lift;
}

// d_internal_cplx_@S@_scale
//   _x 2^_k, rounded once: in steps of at most 2^(EMAX - 1), so that no step
// overflows or underflows before the last, which rounds where the result is
// subnormal.
D_MATH_FN @T@
d_internal_cplx_@S@_scale(
    @T@ _x,
    int _k
)
{
    @T@ value = _x;
    int left  = _k;

    // whole steps up or down while more than one is left
    while (left > (@EMAX@ - 1))
    {
        value = value * d_internal_math_@S@_pow2(@EMAX@ - 1);
        left  = left - (@EMAX@ - 1);
    }

    while (left < (@EMIN@ - 1))
    {
        value = value * d_internal_math_@S@_pow2(@EMIN@ - 1);
        left  = left - (@EMIN@ - 1);
    }

    return value * d_internal_math_@S@_pow2(left);
}

// d_internal_cplx_@S@_magnitude_exponent
//   the binade of the larger part's magnitude of _z, both parts finite and
// one nonzero.
D_MATH_FN int
d_internal_cplx_@S@_magnitude_exponent(
    struct dj_cplx_@S@ _z
)
{
    const @T@ a = d_internal_cplx_@S@_abs(_z.part[0]);
    const @T@ b = d_internal_cplx_@S@_abs(_z.part[1]);

    return d_internal_cplx_@S@_exponent((a > b) ? a : b);
}

// d_internal_cplx_@S@_safe
//   whether every part of _z lies where Dekker's products of the parts with
// another value's of the same reach neither overflow nor lose their low
// parts: from 2^(EMIN/2 + P) to 2^(EMAX/2 - P), or 0.
D_MATH_FN bool
d_internal_cplx_@S@_safe(
    struct dj_cplx_@S@ _z
)
{
    const @T@ low  = d_internal_math_@S@_pow2((@EMIN@ / 2) + @P@);
    const @T@ high = d_internal_math_@S@_pow2((@EMAX@ / 2) - @P@);
    const @T@ a    = d_internal_cplx_@S@_abs(_z.part[0]);
    const @T@ b    = d_internal_cplx_@S@_abs(_z.part[1]);

    return ( ( (a == (@T@)0) ||
               ( (a >= low) &&
                 (a <= high) ) ) &&
             ( (b == (@T@)0) ||
               ( (b >= low) &&
                 (b <= high) ) ) );
}

// d_internal_cplx_@S@_finite
//   whether both parts of _z are finite.
D_MATH_FN bool
d_internal_cplx_@S@_finite(
    struct dj_cplx_@S@ _z
)
{
    return ( (_z.part[0] == _z.part[0]) &&
             (_z.part[1] == _z.part[1]) &&
             (!d_internal_cplx_@S@_is_inf(_z.part[0])) &&
             (!d_internal_cplx_@S@_is_inf(_z.part[1])) );
}

// d_internal_cplx_@S@_scaled_by
//   both parts of _z times 2^_k, each rounded once.
D_MATH_FN struct dj_cplx_@S@
d_internal_cplx_@S@_scaled_by(
    struct dj_cplx_@S@ _z,
    int                _k
)
{
    return d_internal_cplx_@S@_make(
        d_internal_cplx_@S@_scale(_z.part[0], _k),
        d_internal_cplx_@S@_scale(_z.part[1], _k));
}

// d_internal_cplx_@S@_mul_finite
//   _z _w, both finite: each operand scaled to a larger part in [1, 2) where
// either leaves the safe range, the parts by d_internal_cplx_@S@_dot, the
// result scaled back.
D_MATH_FN struct dj_cplx_@S@
d_internal_cplx_@S@_mul_finite(
    struct dj_cplx_@S@ _z,
    struct dj_cplx_@S@ _w
)
{
    // within the safe range, or a zero operand: the parts as they are
    if ( ( (d_internal_cplx_@S@_safe(_z)) &&
           (d_internal_cplx_@S@_safe(_w)) ) ||
         ( (_z.part[0] == (@T@)0) &&
           (_z.part[1] == (@T@)0) ) ||
         ( (_w.part[0] == (@T@)0) &&
           (_w.part[1] == (@T@)0) ) )
    {
        return d_internal_cplx_@S@_make(
            d_internal_cplx_@S@_dot(_z.part[0], _w.part[0],
                                    -_z.part[1], _w.part[1]),
            d_internal_cplx_@S@_dot(_z.part[0], _w.part[1],
                                    _z.part[1], _w.part[0]));
    }

    {
        // each operand's larger part into [1, 2); what lies far below it
        // may underflow in the products, under 2^-2P of the result
        const int ez = d_internal_cplx_@S@_magnitude_exponent(_z);
        const int ew = d_internal_cplx_@S@_magnitude_exponent(_w);
        const struct dj_cplx_@S@ z = d_internal_cplx_@S@_scaled_by(_z, -ez);
        const struct dj_cplx_@S@ w = d_internal_cplx_@S@_scaled_by(_w, -ew);
        const struct dj_cplx_@S@ p = d_internal_cplx_@S@_make(
            d_internal_cplx_@S@_dot(z.part[0], w.part[0],
                                    -z.part[1], w.part[1]),
            d_internal_cplx_@S@_dot(z.part[0], w.part[1],
                                    z.part[1], w.part[0]));

        return d_internal_cplx_@S@_scaled_by(p, ez + ew);
    }
}

// d_internal_cplx_@S@_unit_or_zero
//   +-1 for an infinity and +-0 for any other value, the sign _x's: Annex G's
// boxing of an infinite operand (G.5.1, example 1).
D_MATH_FN @T@
d_internal_cplx_@S@_unit_or_zero(
    @T@ _x
)
{
    return d_internal_cplx_@S@_copysign(
        d_internal_cplx_@S@_is_inf(_x) ? (@T@)1 : (@T@)0,
        _x);
}

// d_internal_cplx_@S@_zero_if_nan
//   _x, or +-0 for a NaN, its sign bit kept.
D_MATH_FN @T@
d_internal_cplx_@S@_zero_if_nan(
    @T@ _x
)
{
    return (_x != _x) ? d_internal_cplx_@S@_copysign((@T@)0, _x) : _x;
}

// d_cplx_@S@_make
//   function: _x + _y i, each part as given, a signed zero, an infinity or a
// NaN among them -- what C11's CMPLX makes and the sum _x + _y i, for a NaN
// or an infinite _y, cannot.
D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_make(
    @T@ _x,
    @T@ _y
)
{
    return d_internal_cplx_@S@_make(_x,
                                    _y);
}

// d_cplx_@S@_real, d_cplx_@S@_imag
//   function: the real and the imaginary part of _z (C99's creal, cimag).
D_MATH_FN @T@
d_cplx_@S@_real(
    struct dj_cplx_@S@ _z
)
{
    return _z.part[0];
}

D_MATH_FN @T@
d_cplx_@S@_imag(
    struct dj_cplx_@S@ _z
)
{
    return _z.part[1];
}

// d_cplx_@S@_conj, d_cplx_@S@_proj, d_cplx_@S@_neg
//   function: the conjugate, the imaginary part negated (C99's conj); the
// projection onto the Riemann sphere, every infinity to +inf + i(+-0), the
// zero's sign the imaginary part's (C99's cproj); and the negation.
D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_conj(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_@S@_make(_z.part[0],
                                    -_z.part[1]);
}

D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_proj(
    struct dj_cplx_@S@ _z
)
{
    // an infinite part, the other perhaps a NaN: the one infinity
    if ( (d_internal_cplx_@S@_is_inf(_z.part[0])) ||
         (d_internal_cplx_@S@_is_inf(_z.part[1])) )
    {
        return d_internal_cplx_@S@_make(
            d_internal_math_@S@_infinity(),
            d_internal_cplx_@S@_copysign((@T@)0, _z.part[1]));
    }

    return _z;
}

D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_neg(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_@S@_make(-_z.part[0],
                                    -_z.part[1]);
}

// d_cplx_@S@_add, d_cplx_@S@_sub
//   function: _z + _w and _z - _w, part by part.
D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_add(
    struct dj_cplx_@S@ _z,
    struct dj_cplx_@S@ _w
)
{
    return d_internal_cplx_@S@_make(_z.part[0] + _w.part[0],
                                    _z.part[1] + _w.part[1]);
}

D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_sub(
    struct dj_cplx_@S@ _z,
    struct dj_cplx_@S@ _w
)
{
    return d_internal_cplx_@S@_make(_z.part[0] - _w.part[0],
                                    _z.part[1] - _w.part[1]);
}

// d_cplx_@S@_mul
//   function: _z _w, each part within 2 ulps where no part of the operands
// is an infinity or a NaN; and where the formula's terms give NaN for both
// parts while an operand is infinite, the infinity Annex G recovers (G.5.1).
D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_mul(
    struct dj_cplx_@S@ _z,
    struct dj_cplx_@S@ _w
)
{
    // finite operands: the exact products
    if ( (d_internal_cplx_@S@_finite(_z)) &&
         (d_internal_cplx_@S@_finite(_w)) )
    {
        return d_internal_cplx_@S@_mul_finite(_z, _w);
    }

    {
        const @T@ a = _z.part[0];
        const @T@ b = _z.part[1];
        const @T@ c = _w.part[0];
        const @T@ d = _w.part[1];
        const @T@ x = (a * c) - (b * d);
        const @T@ y = (a * d) + (b * c);

        // a part not NaN: the formula's
        if ( (x == x) ||
             (y == y) )
        {
            return d_internal_cplx_@S@_make(x, y);
        }

        // an infinite operand: its parts boxed, the other's NaNs as 0s
        if ( (d_internal_cplx_@S@_is_inf(a)) ||
             (d_internal_cplx_@S@_is_inf(b)) ||
             (d_internal_cplx_@S@_is_inf(c)) ||
             (d_internal_cplx_@S@_is_inf(d)) )
        {
            const bool z_inf = ( (d_internal_cplx_@S@_is_inf(a)) ||
                                 (d_internal_cplx_@S@_is_inf(b)) );
            const bool w_inf = ( (d_internal_cplx_@S@_is_inf(c)) ||
                                 (d_internal_cplx_@S@_is_inf(d)) );
            const @T@ a1 = z_inf ? d_internal_cplx_@S@_unit_or_zero(a)
                                 : d_internal_cplx_@S@_zero_if_nan(a);
            const @T@ b1 = z_inf ? d_internal_cplx_@S@_unit_or_zero(b)
                                 : d_internal_cplx_@S@_zero_if_nan(b);
            const @T@ c1 = w_inf ? d_internal_cplx_@S@_unit_or_zero(c)
                                 : d_internal_cplx_@S@_zero_if_nan(c);
            const @T@ d1 = w_inf ? d_internal_cplx_@S@_unit_or_zero(d)
                                 : d_internal_cplx_@S@_zero_if_nan(d);
            const @T@ inf = d_internal_math_@S@_infinity();

            return d_internal_cplx_@S@_make(
                inf * ((a1 * c1) - (b1 * d1)),
                inf * ((a1 * d1) + (b1 * c1)));
        }

        return d_internal_cplx_@S@_make(x, y);
    }
}

// d_internal_cplx_@S@_div_finite
//   _z / _w, both finite and _w not 0: both scaled to a larger part in
// [1, 2), so that the squared modulus n of _w lies in [1, 8) and every
// product below is exact; the numerator's parts as pairs, each over n by
// d_internal_cplx_@S@_over, the result scaled back by the operands'
// exponents. Where a target evaluates float and double wider than their
// types, n and the numerator are long double pairs, as in
// d_internal_cplx_@S@_dot_wide, and each quotient the _over_wide one's.
D_MATH_FN struct dj_cplx_@S@
d_internal_cplx_@S@_div_finite(
    struct dj_cplx_@S@ _z,
    struct dj_cplx_@S@ _w
)
{
    const int ew = d_internal_cplx_@S@_magnitude_exponent(_w);
    const struct dj_cplx_@S@ w = d_internal_cplx_@S@_scaled_by(_w, -ew);
    const bool zero = ( (_z.part[0] == (@T@)0) &&
                        (_z.part[1] == (@T@)0) );
    const int ez = zero ? 0 : d_internal_cplx_@S@_magnitude_exponent(_z);
    const struct dj_cplx_@S@ z = d_internal_cplx_@S@_scaled_by(_z, -ez);
#if @WIDE@
    // n = c^2 + d^2, exactly as a long double pair, then one pair again
    const struct dj_internal_math_ld_pair cc =
        d_internal_math_ld_product_exact((long double)w.part[0],
                                         (long double)w.part[0]);
    const struct dj_internal_math_ld_pair dd =
        d_internal_math_ld_product_exact((long double)w.part[1],
                                         (long double)w.part[1]);
    const struct dj_internal_math_ld_pair n0 =
        d_internal_math_ld_two_sum(cc.high, dd.high);
    const struct dj_internal_math_ld_pair n =
        d_internal_math_ld_two_sum(n0.high, n0.low + (cc.low + dd.low));
    // the numerator (a + bi)(c - di), each part a long double pair
    const struct dj_internal_math_ld_pair re =
        d_internal_cplx_@S@_dot_wide(z.part[0], w.part[0],
                                     z.part[1], w.part[1]);
    const struct dj_internal_math_ld_pair im =
        d_internal_cplx_@S@_dot_wide(z.part[1], w.part[0],
                                     -z.part[0], w.part[1]);

    return d_internal_cplx_@S@_scaled_by(
        d_internal_cplx_@S@_make(d_internal_cplx_@S@_over_wide(re, n),
                                 d_internal_cplx_@S@_over_wide(im, n)),
        ez - ew);
#else  // @WIDE@
    // n = c^2 + d^2, exactly as high + low, then one pair again
    const struct dj_internal_math_@S@_pair cc =
        d_internal_cplx_@S@_product(w.part[0], w.part[0]);
    const struct dj_internal_math_@S@_pair dd =
        d_internal_cplx_@S@_product(w.part[1], w.part[1]);
    const struct dj_internal_math_@S@_pair n0 =
        d_internal_cplx_@S@_two_sum(cc.high, dd.high);
    const struct dj_internal_math_@S@_pair n =
        d_internal_cplx_@S@_two_sum(n0.high, n0.low + (cc.low + dd.low));
    // the numerator (a + bi)(c - di), each part a pair
    const struct dj_internal_math_@S@_pair re =
        d_internal_cplx_@S@_dot_pair(z.part[0], w.part[0],
                                     z.part[1], w.part[1]);
    const struct dj_internal_math_@S@_pair im =
        d_internal_cplx_@S@_dot_pair(z.part[1], w.part[0],
                                     -z.part[0], w.part[1]);

    return d_internal_cplx_@S@_scaled_by(
        d_internal_cplx_@S@_make(d_internal_cplx_@S@_over(re, n),
                                 d_internal_cplx_@S@_over(im, n)),
        ez - ew);
#endif  // @WIDE@
}

// d_cplx_@S@_div
//   function: _z / _w, each part within a step of the correctly rounded one
// (d_internal_cplx_@S@_over) where no part of the operands
// is an infinity or a NaN and _w is not 0; and where the formula gives NaN
// for both parts, the infinity or the zero Annex G recovers (G.5.1): an
// infinity for a nonzero _z over 0, an infinity for an infinite _z over a
// finite _w, a zero for a finite _z over an infinite _w.
D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_div(
    struct dj_cplx_@S@ _z,
    struct dj_cplx_@S@ _w
)
{
    const bool w_zero = ( (_w.part[0] == (@T@)0) &&
                          (_w.part[1] == (@T@)0) );

    // finite operands, _w not 0: the scaled quotient
    if ( (d_internal_cplx_@S@_finite(_z)) &&
         (d_internal_cplx_@S@_finite(_w)) &&
         (!w_zero) )
    {
        return d_internal_cplx_@S@_div_finite(_z, _w);
    }

    {
        const @T@ a   = _z.part[0];
        const @T@ b   = _z.part[1];
        const @T@ c   = _w.part[0];
        const @T@ d   = _w.part[1];
        const @T@ inf = d_internal_math_@S@_infinity();

        // over 0: an infinity in each part, the signs the formula's, where
        // _z is not NaN through
        if ( (w_zero) &&
             ( (a == a) ||
               (b == b) ) )
        {
            const @T@ s = d_internal_cplx_@S@_copysign(inf, c);

            return d_internal_cplx_@S@_make(s * a, s * b);
        }

        // an infinite _z over a finite _w
        if ( ( (d_internal_cplx_@S@_is_inf(a)) ||
               (d_internal_cplx_@S@_is_inf(b)) ) &&
             (d_internal_cplx_@S@_finite(_w)) )
        {
            const @T@ a1 = d_internal_cplx_@S@_unit_or_zero(a);
            const @T@ b1 = d_internal_cplx_@S@_unit_or_zero(b);

            return d_internal_cplx_@S@_make(inf * ((a1 * c) + (b1 * d)),
                                            inf * ((b1 * c) - (a1 * d)));
        }

        // a finite _z over an infinite _w
        if ( ( (d_internal_cplx_@S@_is_inf(c)) ||
               (d_internal_cplx_@S@_is_inf(d)) ) &&
             (d_internal_cplx_@S@_finite(_z)) )
        {
            const @T@ c1 = d_internal_cplx_@S@_unit_or_zero(c);
            const @T@ d1 = d_internal_cplx_@S@_unit_or_zero(d);

            return d_internal_cplx_@S@_make((@T@)0 * ((a * c1) + (b * d1)),
                                            (@T@)0 * ((b * c1) - (a * d1)));
        }

        // every other: NaNs
        {
            const @T@ nan = d_internal_math_@S@_nan();

            return d_internal_cplx_@S@_make(nan, nan);
        }
    }
}

// d_cplx_@S@_add_real, d_cplx_@S@_sub_real, d_cplx_@S@_real_sub,
// d_cplx_@S@_mul_real, d_cplx_@S@_div_real, d_cplx_@S@_real_div
//   function: an operation of a complex _z and a real _x, the real taken as
// itself, not as _x + 0i, as Annex G takes a real operand (G.5): _z + _x,
// _z - _x and _x - _z keep _z's imaginary part, its zero's sign among it;
// _z _x and _z / _x scale both parts, so that an infinite _z by a finite _x
// is no NaN; _x / _z is _x over _z, as d_cplx_@S@_div gives it.
D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_add_real(
    struct dj_cplx_@S@ _z,
    @T@                _x
)
{
    return d_internal_cplx_@S@_make(_z.part[0] + _x,
                                    _z.part[1]);
}

D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_sub_real(
    struct dj_cplx_@S@ _z,
    @T@                _x
)
{
    return d_internal_cplx_@S@_make(_z.part[0] - _x,
                                    _z.part[1]);
}

D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_real_sub(
    @T@                _x,
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_@S@_make(_x - _z.part[0],
                                    -_z.part[1]);
}

D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_mul_real(
    struct dj_cplx_@S@ _z,
    @T@                _x
)
{
    return d_internal_cplx_@S@_make(_z.part[0] * _x,
                                    _z.part[1] * _x);
}

D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_div_real(
    struct dj_cplx_@S@ _z,
    @T@                _x
)
{
    return d_internal_cplx_@S@_make(_z.part[0] / _x,
                                    _z.part[1] / _x);
}

D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_real_div(
    @T@                _x,
    struct dj_cplx_@S@ _z
)
{
    return d_cplx_@S@_div(d_internal_cplx_@S@_make(_x, (@T@)0),
                          _z);
}

// d_cplx_@S@_equal
//   function: whether _z and _w are equal part by part, as == finds them: a
// NaN part equals nothing, and -0 equals +0.
D_MATH_FN bool
d_cplx_@S@_equal(
    struct dj_cplx_@S@ _z,
    struct dj_cplx_@S@ _w
)
{
    return ( (_z.part[0] == _w.part[0]) &&
             (_z.part[1] == _w.part[1]) );
}
"""

# the kernels of C99's functions, in @T@ on the real functions of suffix @R@,
# their names suffixed @KS@ and qualified @KQ@: once in long double on the
# _c real functions (@KS@ = @R@ = _c), and once in each family's own type on
# the C library's (@KS@ = @R@ = empty)
CPLX_KERNELS = r"""
// @SECTION@.@SUB@    @TITLE@
//------------------------------------------------------------------------------

// d_internal_cplx_@S@_exp_times@KS@
//   e^_x _c, _x finite, without the overflow of e^_x where the product is in
// range: past ln(MAX) as (e^(x/2) _c) e^(x/2), x/2 exact, within 3 ulps.
@KQ@ @T@
d_internal_cplx_@S@_exp_times@KS@(
    @T@ _x,
    @T@ _c
)
{
    // within range: the exponential itself
    if (_x <= ((@T@)(@EMAX@ - 1) * (@T@)0.6931L))
    {
        return d_math_@S@_exp@R@(_x) * _c;
    }

    {
        const @T@ h = d_math_@S@_exp@R@(_x / (@T@)2);

        return (h * _c) * h;
    }
}

// d_internal_cplx_@S@_exp@KS@
//   e^_z: e^x (cos y + i sin y), and Annex G's values (G.6.3.1) where a part
// is an infinity or a NaN: conj(exp z) = exp(conj z) throughout.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_exp@KS@(
    struct dj_cplx_@S@ _z
)
{
    const @T@ x   = _z.part[0];
    const @T@ y   = _z.part[1];
    const @T@ nan = d_internal_math_@S@_nan();

    // a zero imaginary part: e^x and that zero, NaN + i0 among them
    if (y == (@T@)0)
    {
        return d_internal_cplx_@S@_make(d_math_@S@_exp@R@(x), y);
    }

    // x a NaN, y not 0: NaNs
    if (x != x)
    {
        return d_internal_cplx_@S@_make(x, x);
    }

    // y infinite or NaN
    if ( (y != y) ||
         (d_internal_cplx_@S@_is_inf(y)) )
    {
        // -inf: +-0 +- i0; +inf: +-inf + iNaN; finite x: NaNs
        if (d_internal_cplx_@S@_is_inf(x))
        {
            if (x < (@T@)0)
            {
                return d_internal_cplx_@S@_make((@T@)0, (@T@)0);
            }

            return d_internal_cplx_@S@_make(x, nan);
        }

        return d_internal_cplx_@S@_make(nan, nan);
    }

    // x infinite, y finite and not 0: +0 cis(y) or +inf cis(y)
    if (d_internal_cplx_@S@_is_inf(x))
    {
        const @T@ m = (x < (@T@)0) ? (@T@)0 : x;

        return d_internal_cplx_@S@_make(m * d_math_@S@_cos@R@(y),
                                        m * d_math_@S@_sin@R@(y));
    }

    return d_internal_cplx_@S@_make(
        d_internal_cplx_@S@_exp_times@KS@(x, d_math_@S@_cos@R@(y)),
        d_internal_cplx_@S@_exp_times@KS@(x, d_math_@S@_sin@R@(y)));
}

// d_internal_cplx_@S@_log_modulus@KS@
//   log |_z|, both parts finite and one nonzero: near the unit circle, for
// |z|^2 in [1/2, 2], log1p(x^2 + y^2 - 1) / 2, the argument's terms exact
// and summed so that only small ones round; elsewhere log(hypot(x, y)),
// scaled by a power of 2 past half the range or into the normal numbers, the
// scale's ln 2 added back.
@KQ@ @T@
d_internal_cplx_@S@_log_modulus@KS@(
    @T@ _x,
    @T@ _y
)
{
    const @T@ a = d_internal_cplx_@S@_abs(_x);
    const @T@ b = d_internal_cplx_@S@_abs(_y);
    const @T@ big   = (a > b) ? a : b;
    const @T@ small = (a > b) ? b : a;

    // near the unit circle: |z|^2 - 1 exactly as far as it goes
    if ( (big >= (@T@)0.5) &&
         (big <= (@T@)1.5) &&
         (small < (@T@)1.0) )
    {
#if @WIDE@
        // the squares and their sums as long double pairs, as the
        // arithmetic's are where @T@ is evaluated wider (section 2)
        const struct dj_internal_math_ld_pair bb =
            d_internal_math_ld_product_exact((long double)big,
                                             (long double)big);
        const struct dj_internal_math_ld_pair ss =
            d_internal_math_ld_product_exact((long double)small,
                                             (long double)small);
        const long double m2 = bb.high + (ss.high + ss.low);

        if ( (m2 >= 0.5L) &&
             (m2 <= 2.0L) )
        {
            const struct dj_internal_math_ld_pair t1 =
                d_internal_math_ld_two_sum(bb.high, -1.0L);
            const struct dj_internal_math_ld_pair head =
                d_internal_math_ld_two_sum(t1.high, ss.high);
            const struct dj_internal_math_ld_pair v =
                d_internal_math_ld_two_sum(
                    head.high,
                    ((head.low + t1.low) + bb.low) + ss.low);

            return d_math_@S@_log1p@R@(
                       d_internal_math_@S@_round_pair(v.high, v.low)) /
                   (@T@)2;
        }
#else  // @WIDE@
        const struct dj_internal_math_@S@_pair bb =
            d_internal_cplx_@S@_product(big, big);
        const struct dj_internal_math_@S@_pair ss =
            d_internal_cplx_@S@_product(small, small);
        const @T@ s2 = ss.high + ss.low;
        const @T@ m2 = bb.high + s2;

        if ( (m2 >= (@T@)0.5) &&
             (m2 <= (@T@)2) )
        {
            // b^2 - 1 exactly as a pair: Sterbenz's lemma makes it one value
            // only where b^2 is 1/2 or more, and both parts near 1/sqrt(2)
            // put it below; then the small squares
            const struct dj_internal_math_@S@_pair t1 =
                d_internal_cplx_@S@_two_sum(bb.high, -(@T@)1);
            const struct dj_internal_math_@S@_pair head =
                d_internal_cplx_@S@_two_sum(t1.high, ss.high);
            const @T@ v = head.high +
                          (((head.low + t1.low) + bb.low) + ss.low);

            return d_math_@S@_log1p@R@(v) / (@T@)2;
        }
#endif  // @WIDE@
    }

    // past half the range: both halved, ln 2 added
    if (big > (@MAX@ / (@T@)2))
    {
        return d_math_@S@_log@R@(d_math_@S@_hypot@R@(_x / (@T@)2,
                                                     _y / (@T@)2)) +
               (@T@)0.693147180559945309417232121458176568L;
    }

    // below the normal numbers: both raised by 2^P, P ln 2 taken off
    if (big < (@MIN@ * d_internal_math_@S@_pow2(@P@)))
    {
        const @T@ lift = d_internal_math_@S@_pow2(@P@);

        return d_math_@S@_log@R@(d_math_@S@_hypot@R@(_x * lift, _y * lift)) -
               ((@T@)@P@ * (@T@)0.693147180559945309417232121458176568L);
    }

    return d_math_@S@_log@R@(d_math_@S@_hypot@R@(_x, _y));
}

// d_internal_cplx_@S@_log@KS@
//   log _z: log |z| + i arg z, the angle atan2's, so that Annex G's values
// (G.6.3.2) follow from hypot's and atan2's own -- log(+-0 + i0) is -inf + i0
// or i pi, an infinite part gives +inf, a NaN with a finite part NaNs.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_log@KS@(
    struct dj_cplx_@S@ _z
)
{
    const @T@ x = _z.part[0];
    const @T@ y = _z.part[1];
    const @T@ angle = d_math_@S@_atan2@R@(y, x);

    // zero, an infinite part or a NaN: hypot's logarithm
    if ( ( (x == (@T@)0) &&
           (y == (@T@)0) ) ||
         (!d_internal_cplx_@S@_finite(_z)) )
    {
        return d_internal_cplx_@S@_make(
            d_math_@S@_log@R@(d_math_@S@_hypot@R@(x, y)),
            angle);
    }

    return d_internal_cplx_@S@_make(
        d_internal_cplx_@S@_log_modulus@KS@(x, y),
        angle);
}

// d_internal_cplx_@S@_sqrt@KS@
//   sqrt _z, the principal root, Kahan's: t = sqrt((|x| + |z|) / 2), then
// (t, y / 2t) for x >= 0 and (|y| / 2t, +-t) for x < 0, the sign y's;
// scaled by 4^k where |x| + |z| would overflow or underflow. Annex G's values
// (G.6.4.2): an infinite y gives +inf + i y, a NaN with a finite part NaNs,
// -inf gives +0 + i(+-inf) and +inf gives +inf + i(+-0).
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_sqrt@KS@(
    struct dj_cplx_@S@ _z
)
{
    const @T@ x   = _z.part[0];
    const @T@ y   = _z.part[1];
    const @T@ inf = d_internal_math_@S@_infinity();

    // an infinite imaginary part, x anything: +inf + i y
    if (d_internal_cplx_@S@_is_inf(y))
    {
        return d_internal_cplx_@S@_make(inf, y);
    }

    // x infinite
    if (d_internal_cplx_@S@_is_inf(x))
    {
        // -inf: NaN +- i inf for a NaN y, else +0 + i(+-inf)
        if (x < (@T@)0)
        {
            if (y != y)
            {
                return d_internal_cplx_@S@_make(y, inf);
            }

            return d_internal_cplx_@S@_make(
                (@T@)0,
                d_internal_cplx_@S@_copysign(inf, y));
        }

        // +inf: +inf + iNaN, or +inf + i(+-0)
        if (y != y)
        {
            return d_internal_cplx_@S@_make(x, y);
        }

        return d_internal_cplx_@S@_make(
            x,
            d_internal_cplx_@S@_copysign((@T@)0, y));
    }

    // a NaN with the rest finite: NaNs
    if ( (x != x) ||
         (y != y) )
    {
        const @T@ nan = d_internal_math_@S@_nan();

        return d_internal_cplx_@S@_make(nan, nan);
    }

    // zero: +0 and y's zero
    if ( (x == (@T@)0) &&
         (y == (@T@)0) )
    {
        return d_internal_cplx_@S@_make((@T@)0, y);
    }

    {
        const @T@ a   = d_internal_cplx_@S@_abs(x);
        const @T@ b   = d_internal_cplx_@S@_abs(y);
        const @T@ big = (a > b) ? a : b;
        // past a quarter of the range, the parts over 4 and t times 2;
        // below the normal numbers, times 4^k and t over 2^k
        const int down = (big > (@MAX@ / (@T@)4)) ? 1 : 0;
        const int up   = (big < (@MIN@ * d_internal_math_@S@_pow2(@P@)))
                             ? ((@P@ + 1) / 2)
                             : 0;
        const int k    = up - down;
        const @T@ sa   = d_internal_cplx_@S@_scale(a, 2 * k);
        const @T@ sb   = d_internal_cplx_@S@_scale(b, 2 * k);
        const @T@ m    = d_math_@S@_hypot@R@(sa, sb);
        const @T@ st   = d_math_@S@_sqrt@R@((sa + m) / (@T@)2);
        // t and |y| / 2t in the scaled numbers, then back
        const @T@ other = (sb / st) / (@T@)2;
        const @T@ t     = d_internal_cplx_@S@_scale(st, -k);
        const @T@ u     = d_internal_cplx_@S@_scale(other, -k);

        if (x >= (@T@)0)
        {
            return d_internal_cplx_@S@_make(
                t,
                d_internal_cplx_@S@_copysign(u, y));
        }

        return d_internal_cplx_@S@_make(
            u,
            d_internal_cplx_@S@_copysign(t, y));
    }
}

// d_internal_cplx_@S@_cispi@KS@
//   cos(pi _r) + i sin(pi _r), |_r| <= 1, by the octant of _r: its distance
// from 0, 1/2 or 1, exact by Sterbenz's lemma, times pi, so that the angle
// handed to sin and cos is small and accurate, and 0, 1/2 and 1 give their
// exact values.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_cispi@KS@(
    @T@ _r
)
{
    const @T@ pi = (@T@)3.141592653589793238462643383279502884L;
    const bool negative = d_internal_math_@S@_negative(_r);
    const @T@ r = negative ? -_r : _r;
    @T@ c = (@T@)1;
    @T@ s = (@T@)0;

    // [0, 1/4]: cos(pi r), sin(pi r)
    if (r <= (@T@)0.25)
    {
        c = d_math_@S@_cos@R@(pi * r);
        s = d_math_@S@_sin@R@(pi * r);
    }
    // (1/4, 3/4]: sin(pi (1/2 - r)), cos(pi (1/2 - r))
    else if (r <= (@T@)0.75)
    {
        const @T@ t = (@T@)0.5 - r;

        c = d_math_@S@_sin@R@(pi * t);
        s = d_math_@S@_cos@R@(pi * t);
    }
    // (3/4, 1]: -cos(pi (1 - r)), sin(pi (1 - r))
    else
    {
        const @T@ t = (@T@)1 - r;

        c = -d_math_@S@_cos@R@(pi * t);
        s = d_math_@S@_sin@R@(pi * t);
    }

    return d_internal_cplx_@S@_make(c, negative ? -s : s);
}

// d_internal_cplx_@S@_turn@KS@
//   cos(pi _a) + i sin(pi _a) for any finite _a: _a less its nearest even
// integer, exact, into [-1, 1], then cispi.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_turn@KS@(
    @T@ _a
)
{
    const @T@ half = _a / (@T@)2;
    const @T@ k    = d_math_@S@_floor_c(half + (@T@)0.5);
    const @T@ r    = _a - ((@T@)2 * k);

    return d_internal_cplx_@S@_cispi@KS@(r);
}

@POW_GENERAL@
@POW_INT@// d_internal_cplx_@S@_pow@KS@
//   _z^_w, the principal value exp(w log z), with these exact: z^0 is 1 for
// every z; a whole w, from -2^30 to 2^30, repeated squaring; a positive real
// z to a real w the real pow, and a negative real or imaginary one |z|^w
// times cos + i sin of w pi or w pi/2, by turns, so that (-4)^(1/2) is 2i;
// 0^w is 0 for Re w > 0, +inf for a real w < 0, and NaNs otherwise.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_pow@KS@(
    struct dj_cplx_@S@ _z,
    struct dj_cplx_@S@ _w
)
{
    const @T@ x   = _z.part[0];
    const @T@ y   = _z.part[1];
    const @T@ a   = _w.part[0];
    const @T@ b   = _w.part[1];
    const @T@ nan = d_internal_math_@S@_nan();

    // z^0 is 1
    if ( (a == (@T@)0) &&
         (b == (@T@)0) )
    {
        return d_internal_cplx_@S@_make((@T@)1, (@T@)0);
    }

    // a NaN anywhere: NaNs
    if ( (x != x) ||
         (y != y) ||
         (a != a) ||
         (b != b) )
    {
        return d_internal_cplx_@S@_make(nan, nan);
    }

    // a real exponent
    if (b == (@T@)0)
    {
        // whole, of an int: repeated squaring of a finite z not 0
        if ( (d_math_@S@_floor_c(a) == a) &&
             (a >= (@T@)-1073741824.0L) &&
             (a <= (@T@)1073741824.0L) &&
             (d_internal_cplx_@S@_finite(_z)) &&
             ( (x != (@T@)0) ||
               (y != (@T@)0) ) )
        {
            return d_internal_cplx_@S@_pow_int@KS@(_z, (int)a);
        }

        // a real z
        if (y == (@T@)0)
        {
            // a positive one: the real pow, the zero's sign by w's
            if (x > (@T@)0)
            {
                return d_internal_cplx_@S@_make(d_math_@S@_pow@R@(x, a),
                                                a * y);
            }

            // a negative one: |x|^w (cos + i sin)(w pi), the sign of pi y's
            if (x < (@T@)0)
            {
                const @T@ r = d_math_@S@_pow@R@(-x, a);
                const struct dj_cplx_@S@ c = d_internal_cplx_@S@_turn@KS@(
                    d_internal_math_@S@_negative(y) ? -a : a);

                return d_cplx_@S@_mul_real(c, r);
            }
        }

        // an imaginary z: |y|^w (cos + i sin)(+-w pi/2)
        if ( (x == (@T@)0) &&
             (y != (@T@)0) &&
             (!d_internal_cplx_@S@_is_inf(y)) )
        {
            const @T@ r = d_math_@S@_pow@R@(d_internal_cplx_@S@_abs(y), a);
            const struct dj_cplx_@S@ c = d_internal_cplx_@S@_turn@KS@(
                (y < (@T@)0) ? (-a / (@T@)2) : (a / (@T@)2));

            return d_cplx_@S@_mul_real(c, r);
        }
    }

    // 0^w
    if ( (x == (@T@)0) &&
         (y == (@T@)0) )
    {
        if (a > (@T@)0)
        {
            return d_internal_cplx_@S@_make((@T@)0, (@T@)0);
        }

        if ( (a < (@T@)0) &&
             (b == (@T@)0) )
        {
            return d_internal_cplx_@S@_make(d_internal_math_@S@_infinity(),
                                            (@T@)0);
        }

        return d_internal_cplx_@S@_make(nan, nan);
    }

    return d_internal_cplx_@S@_pow_general@KS@(_z, _w);
}

// d_internal_cplx_@S@_cosh_sinh_times@KS@
//   cosh(_x) _c and sinh(_x) _c at once, _x finite: past ln(MAX) both are
// e^|x| / 2 times _c, by the exponential's scaled product, the sine's sign
// x's.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_cosh_sinh_times@KS@(
    @T@ _x,
    @T@ _c,
    @T@ _s
)
{
    const @T@ ax = d_internal_cplx_@S@_abs(_x);

    // within range: cosh x _c and sinh x _s themselves
    if (ax <= ((@T@)(@EMAX@ - 1) * (@T@)0.6931L))
    {
        return d_internal_cplx_@S@_make(d_math_@S@_cosh@R@(_x) * _c,
                                        d_math_@S@_sinh@R@(_x) * _s);
    }

    {
        const @T@ half_c = d_internal_cplx_@S@_exp_times@KS@(ax,
                                                             _c / (@T@)2);
        const @T@ half_s = d_internal_cplx_@S@_exp_times@KS@(ax,
                                                             _s / (@T@)2);

        return d_internal_cplx_@S@_make(half_c,
                                        (_x < (@T@)0) ? -half_s : half_s);
    }
}

// d_internal_cplx_@S@_sinh@KS@
//   sinh _z = sinh x cos y + i cosh x sin y, Annex G's values (G.6.2.5) at
// the infinities and NaNs: it is odd and conj(sinh z) = sinh(conj z).
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_sinh@KS@(
    struct dj_cplx_@S@ _z
)
{
    const @T@ x   = _z.part[0];
    const @T@ y   = _z.part[1];
    const @T@ nan = d_internal_math_@S@_nan();

    // a finite argument
    if (d_internal_cplx_@S@_finite(_z))
    {
        // y = 0: sinh x and y's zero
        if (y == (@T@)0)
        {
            return d_internal_cplx_@S@_make(d_math_@S@_sinh@R@(x), y);
        }

        {
            const struct dj_cplx_@S@ ch =
                d_internal_cplx_@S@_cosh_sinh_times@KS@(
                    x,
                    d_math_@S@_sin@R@(y),
                    d_math_@S@_cos@R@(y));

            return d_internal_cplx_@S@_make(ch.part[1], ch.part[0]);
        }
    }

    // x 0, y infinite or NaN: +-0 + iNaN
    if (x == (@T@)0)
    {
        return d_internal_cplx_@S@_make(x, nan);
    }

    // x infinite
    if (d_internal_cplx_@S@_is_inf(x))
    {
        // y 0: x + i y
        if (y == (@T@)0)
        {
            return d_internal_cplx_@S@_make(x, y);
        }

        // y infinite or NaN: +-inf + iNaN
        if ( (y != y) ||
             (d_internal_cplx_@S@_is_inf(y)) )
        {
            return d_internal_cplx_@S@_make(x, nan);
        }

        // y finite: inf cis(y), the real part's sign x's
        return d_internal_cplx_@S@_make(x * d_math_@S@_cos@R@(y),
                                        d_internal_cplx_@S@_copysign(
                                            x, (@T@)1) *
                                        d_math_@S@_sin@R@(y));
    }

    // x NaN: NaN + i0 for y = 0, else NaNs
    if ( (x != x) &&
         (y == (@T@)0) )
    {
        return d_internal_cplx_@S@_make(x, y);
    }

    return d_internal_cplx_@S@_make(nan, nan);
}

// d_internal_cplx_@S@_cosh@KS@
//   cosh _z = cosh x cos y + i sinh x sin y, Annex G's values (G.6.2.4) at
// the infinities and NaNs: it is even and conj(cosh z) = cosh(conj z).
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_cosh@KS@(
    struct dj_cplx_@S@ _z
)
{
    const @T@ x   = _z.part[0];
    const @T@ y   = _z.part[1];
    const @T@ nan = d_internal_math_@S@_nan();

    // a finite argument
    if (d_internal_cplx_@S@_finite(_z))
    {
        // y = 0: cosh x and the zero x y gives
        if (y == (@T@)0)
        {
            return d_internal_cplx_@S@_make(d_math_@S@_cosh@R@(x), x * y);
        }

        return d_internal_cplx_@S@_cosh_sinh_times@KS@(
            x,
            d_math_@S@_cos@R@(y),
            d_math_@S@_sin@R@(y));
    }

    // x 0, y infinite or NaN: NaN + i0
    if (x == (@T@)0)
    {
        return d_internal_cplx_@S@_make(nan, (@T@)0);
    }

    // x infinite
    if (d_internal_cplx_@S@_is_inf(x))
    {
        const @T@ inf = d_internal_math_@S@_infinity();

        // y 0: +inf + i0, the zero signed as sinh x sin y is
        if (y == (@T@)0)
        {
            return d_internal_cplx_@S@_make(
                inf,
                d_internal_cplx_@S@_copysign((@T@)0, x) * y);
        }

        // y infinite or NaN: +inf + iNaN
        if ( (y != y) ||
             (d_internal_cplx_@S@_is_inf(y)) )
        {
            return d_internal_cplx_@S@_make(inf, nan);
        }

        // y finite: +inf cis(y), the sine's term signed by x
        return d_internal_cplx_@S@_make(inf * d_math_@S@_cos@R@(y),
                                        x * d_math_@S@_sin@R@(y));
    }

    // x NaN: NaN + i0 for y = 0, else NaNs
    if (y == (@T@)0)
    {
        return d_internal_cplx_@S@_make(x, (@T@)0);
    }

    return d_internal_cplx_@S@_make(nan, nan);
}

// d_internal_cplx_@S@_tanh@KS@
//   tanh _z by Kahan's formula: t = tan y, b = 1 + t^2, s = sinh x, r =
// sqrt(1 + s^2), tanh z = (b r s + i t) / (1 + b s^2); past (P + 4) ln 2 / 2,
// where tanh x is +-1 in the type, +-1 + i 4 sin y cos y e^(-2|x|). Annex G's
// values (G.6.2.6, as C17 has them, its DR 471): it is odd, conj(tanh z) =
// tanh(conj z), +inf + iy gives 1 + i0 sin 2y, +-0 + i(inf or NaN) +-0 +
// iNaN.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_tanh@KS@(
    struct dj_cplx_@S@ _z
)
{
    const @T@ x   = _z.part[0];
    const @T@ y   = _z.part[1];
    const @T@ nan = d_internal_math_@S@_nan();

    // x infinite: +-1, and a zero signed by sin 2y, or +-0
    if (d_internal_cplx_@S@_is_inf(x))
    {
        const @T@ one = (x < (@T@)0) ? (@T@)-1 : (@T@)1;

        if ( (y != y) ||
             (d_internal_cplx_@S@_is_inf(y)) )
        {
            return d_internal_cplx_@S@_make(one, (@T@)0);
        }

        return d_internal_cplx_@S@_make(
            one,
            d_internal_cplx_@S@_copysign(
                (@T@)0,
                d_math_@S@_sin@R@(y) * d_math_@S@_cos@R@(y)));
    }

    // y infinite or NaN, x finite: +-0 + iNaN for x 0, else NaNs
    if ( (y != y) ||
         (d_internal_cplx_@S@_is_inf(y)) )
    {
        if (x == (@T@)0)
        {
            return d_internal_cplx_@S@_make(x, nan);
        }

        return d_internal_cplx_@S@_make(nan, nan);
    }

    // x NaN: NaN + i0 for y = 0, else NaNs
    if (x != x)
    {
        if (y == (@T@)0)
        {
            return d_internal_cplx_@S@_make(x, y);
        }

        return d_internal_cplx_@S@_make(nan, nan);
    }

    // y = 0: tanh x and y's zero
    if (y == (@T@)0)
    {
        return d_internal_cplx_@S@_make(d_math_@S@_tanh@R@(x), y);
    }

    // far from 0: +-1 + i 4 sin y cos y e^(-2|x|)
    if (d_internal_cplx_@S@_abs(x) > ((@T@)(@P@ + 4) * (@T@)0.3466L))
    {
        const @T@ e = d_math_@S@_exp@R@((@T@)-2 * d_internal_cplx_@S@_abs(x));

        return d_internal_cplx_@S@_make(
            (x < (@T@)0) ? (@T@)-1 : (@T@)1,
            (((@T@)4 * d_math_@S@_sin@R@(y)) * d_math_@S@_cos@R@(y)) * e);
    }

    {
        const @T@ t     = d_math_@S@_tan@R@(y);
        const @T@ beta  = (@T@)1 + (t * t);
        const @T@ s     = d_math_@S@_sinh@R@(x);
        const @T@ rho   = d_math_@S@_sqrt@R@((@T@)1 + (s * s));
        const @T@ denom = (@T@)1 + ((beta * s) * s);

        return d_internal_cplx_@S@_make(((beta * rho) * s) / denom,
                                        t / denom);
    }
}
"""

CPLX_INVERSE = r"""
// d_internal_cplx_@S@_half_excess@KS@
//   f(_a, _b) of Hull, Fairgrieve and Tang, with _h = hypot(_a, _b): half of
// _h - _b, written so that it never subtracts nearly equal values: (_h - _b)
// / 2 for _b < 0, _a / 2 for _b = 0, and _a^2 / (_h + _b) / 2 otherwise.
@KQ@ @T@
d_internal_cplx_@S@_half_excess@KS@(
    @T@ _a,
    @T@ _b,
    @T@ _h
)
{
    if (_b < (@T@)0)
    {
        return (_h - _b) / (@T@)2;
    }

    if (_b == (@T@)0)
    {
        return _a / (@T@)2;
    }

    return ((_a * _a) / (_h + _b)) / (@T@)2;
}

// dj_internal_cplx_@S@_hard_work
//   struct: what dj_internal_cplx_@S@_hard_work@KS@ finds of z = x + iy, x
// and y nonnegative: the real part of asinh z (rx), and the imaginary
// part's two ways, B = y / A where it is usable, else the arguments of an
// atan2.
struct dj_internal_cplx_@S@_hard_work@KS@
{
    @T@  rx;           // Re asinh(x + iy) = log(A + sqrt(A^2 - 1))
    bool b_is_usable;  // whether asin(B) gives the imaginary part
    @T@  b;            // B = y / A
    @T@  sqrt_a2my2;   // sqrt(A^2 - y^2), for atan2(new_y, it)
    @T@  new_y;        // y, scaled with it
};

// d_internal_cplx_@S@_hard_work_of@KS@
//   Hull, Fairgrieve and Tang's A and B of x + iy, x and y nonnegative and
// finite, below 1/EPSILON: R = |z + i|, S = |z - i|, A = (R + S) / 2 and B =
// y / A, so that asinh z = log(A + sqrt(A^2 - 1)) + i asin B, with A - 1
// and A - y written to keep their accuracy where A is near 1 or y, as
// FreeBSD's catrig.c writes them: A below 10 takes log1p of A - 1, B past
// 0.6417 takes atan2 of y and sqrt(A^2 - y^2).
@KQ@ struct dj_internal_cplx_@S@_hard_work@KS@
d_internal_cplx_@S@_hard_work_of@KS@(
    @T@ _x,
    @T@ _y
)
{
    const @T@ eps          = @EPS@;
    const @T@ four_sqrt_min = d_internal_math_@S@_pow2(((@EMIN@ - 1) / 2) + 2);
    const @T@ r            = d_math_@S@_hypot@R@(_x, _y + (@T@)1);
    const @T@ s            = d_math_@S@_hypot@R@(_x, _y - (@T@)1);
    const @T@ a0           = (r + s) / (@T@)2;
    // A >= 1 in exact arithmetic; so it is made here
    const @T@ a            = (a0 < (@T@)1) ? (@T@)1 : a0;
    struct dj_internal_cplx_@S@_hard_work@KS@ result =
        { (@T@)0, false, (@T@)0, (@T@)0, (@T@)0 };

    // the real part
    if (a < (@T@)10)
    {
        if ( (_y == (@T@)1) &&
             (_x < ((eps * eps) / (@T@)128)) )
        {
            // A - 1 of order x / 2, A = 1 inexactly
            result.rx = d_math_@S@_sqrt@R@(_x);
        }
        else if (_x >= (eps * d_internal_cplx_@S@_abs(_y - (@T@)1)))
        {
            const @T@ am1 =
                d_internal_cplx_@S@_half_excess@KS@(_x, (@T@)1 + _y, r) +
                d_internal_cplx_@S@_half_excess@KS@(_x, (@T@)1 - _y, s);

            result.rx = d_math_@S@_log1p@R@(
                am1 + d_math_@S@_sqrt@R@(am1 * (a + (@T@)1)));
        }
        else if (_y < (@T@)1)
        {
            result.rx = _x / d_math_@S@_sqrt@R@(((@T@)1 - _y) *
                                                ((@T@)1 + _y));
        }
        else
        {
            result.rx = d_math_@S@_log1p@R@(
                (_y - (@T@)1) +
                d_math_@S@_sqrt@R@((_y - (@T@)1) * (_y + (@T@)1)));
        }
    }
    else
    {
        result.rx = d_math_@S@_log@R@(
            a + d_math_@S@_sqrt@R@((a * a) - (@T@)1));
    }

    result.new_y = _y;

    // y tiny: y / A would underflow; atan2 of both scaled up instead
    if (_y < four_sqrt_min)
    {
        result.b_is_usable = false;
        result.sqrt_a2my2  = a * ((@T@)2 / eps);
        result.new_y       = _y * ((@T@)2 / eps);

        return result;
    }

    result.b           = _y / a;
    result.b_is_usable = true;

    // B near 1: asin loses its accuracy; atan2 instead
    if (result.b > (@T@)0.6417)
    {
        result.b_is_usable = false;

        if ( (_y == (@T@)1) &&
             (_x < (eps / (@T@)128)) )
        {
            result.sqrt_a2my2 = d_math_@S@_sqrt@R@(_x) *
                                d_math_@S@_sqrt@R@((a + _y) / (@T@)2);
        }
        else if (_x >= (eps * d_internal_cplx_@S@_abs(_y - (@T@)1)))
        {
            const @T@ amy =
                d_internal_cplx_@S@_half_excess@KS@(_x, _y + (@T@)1, r) +
                d_internal_cplx_@S@_half_excess@KS@(_x, _y - (@T@)1, s);

            result.sqrt_a2my2 = d_math_@S@_sqrt@R@(amy * (a + _y));
        }
        else if (_y > (@T@)1)
        {
            const @T@ lift = (@T@)4 / (eps * eps);

            result.sqrt_a2my2 = ((_x * lift) * _y) /
                                d_math_@S@_sqrt@R@((_y + (@T@)1) *
                                                   (_y - (@T@)1));
            result.new_y      = _y * lift;
        }
        else
        {
            result.sqrt_a2my2 = d_math_@S@_sqrt@R@(((@T@)1 - _y) *
                                                   ((@T@)1 + _y));
        }
    }

    return result;
}

// d_internal_cplx_@S@_log_large@KS@
//   log _z for |_z| past 1/EPSILON and finite, without hypot's overflow:
// past half the range both parts halved and ln 2 added, as for log |z|.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_log_large@KS@(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_@S@_make(
        d_internal_cplx_@S@_log_modulus@KS@(_z.part[0], _z.part[1]),
        d_math_@S@_atan2@R@(_z.part[1], _z.part[0]));
}

// d_internal_cplx_@S@_asinh@KS@
//   asinh _z: Hull, Fairgrieve and Tang's algorithm, as FreeBSD's catrig.c
// has it, on |x| and |y|, the signs put back: past 1/EPSILON log(2z) by the
// logarithm of large values; below sqrt(6 EPSILON) / 4 in both parts z
// itself; and Annex G's values (G.6.2.2) at the infinities and NaNs.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_asinh@KS@(
    struct dj_cplx_@S@ _z
)
{
    const @T@ x   = _z.part[0];
    const @T@ y   = _z.part[1];
    const @T@ ax  = d_internal_cplx_@S@_abs(x);
    const @T@ ay  = d_internal_cplx_@S@_abs(y);
    const @T@ nan = d_internal_math_@S@_nan();
    const @T@ recip_eps = (@T@)1 / @EPS@;

    // a NaN part
    if ( (x != x) ||
         (y != y) )
    {
        // +-inf + iNaN: +-inf + iNaN; NaN + i(+-inf): +-inf + iNaN
        if (d_internal_cplx_@S@_is_inf(x))
        {
            return d_internal_cplx_@S@_make(x, y);
        }

        if (d_internal_cplx_@S@_is_inf(y))
        {
            return d_internal_cplx_@S@_make(y, x);
        }

        // NaN + i0: NaN + i0
        if (y == (@T@)0)
        {
            return d_internal_cplx_@S@_make(x, y);
        }

        return d_internal_cplx_@S@_make(nan, nan);
    }

    // far from 0, infinities among it: log(2|z|) + i arg, signed back
    if ( (ax > recip_eps) ||
         (ay > recip_eps) )
    {
        const struct dj_cplx_@S@ w = d_internal_cplx_@S@_log_large@KS@(
            d_internal_math_@S@_negative(x) ? d_cplx_@S@_neg(_z) : _z);

        return d_internal_cplx_@S@_make(
            d_internal_cplx_@S@_copysign(
                w.part[0] + (@T@)0.693147180559945309417232121458176568L,
                x),
            d_internal_cplx_@S@_copysign(w.part[1], y));
    }

    // 0, and values whose next term lies below their last place
    if ( ( (x == (@T@)0) &&
           (y == (@T@)0) ) ||
         ( (ax < (d_math_@S@_sqrt_c((@T@)6 * @EPS@) / (@T@)4)) &&
           (ay < (d_math_@S@_sqrt_c((@T@)6 * @EPS@) / (@T@)4)) ) )
    {
        return _z;
    }

    {
        const struct dj_internal_cplx_@S@_hard_work@KS@ h =
            d_internal_cplx_@S@_hard_work_of@KS@(ax, ay);
        const @T@ ry = h.b_is_usable
                           ? d_math_@S@_asin@R@(h.b)
                           : d_math_@S@_atan2@R@(h.new_y, h.sqrt_a2my2);

        return d_internal_cplx_@S@_make(d_internal_cplx_@S@_copysign(h.rx, x),
                                        d_internal_cplx_@S@_copysign(ry, y));
    }
}

// d_internal_cplx_@S@_acos@KS@
//   acos _z: Hull, Fairgrieve and Tang's algorithm on |y| and |x|, as
// FreeBSD's catrig.c has it: the real part from B or atan2, signed by x, the
// imaginary part -sign(y) Re asinh; and Annex G's values (G.6.1.1).
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_acos@KS@(
    struct dj_cplx_@S@ _z
)
{
    const @T@ x    = _z.part[0];
    const @T@ y    = _z.part[1];
    const @T@ ax   = d_internal_cplx_@S@_abs(x);
    const @T@ ay   = d_internal_cplx_@S@_abs(y);
    const bool sx  = d_internal_math_@S@_negative(x);
    const bool sy  = d_internal_math_@S@_negative(y);
    const @T@ nan  = d_internal_math_@S@_nan();
    const @T@ pio2 = (@T@)1.570796326794896619231321691639751442L;
    const @T@ recip_eps = (@T@)1 / @EPS@;

    // a NaN part
    if ( (x != x) ||
         (y != y) )
    {
        // +-inf + iNaN: NaN +- i inf
        if (d_internal_cplx_@S@_is_inf(x))
        {
            return d_internal_cplx_@S@_make(nan,
                                            -d_internal_math_@S@_infinity());
        }

        // NaN + i(+-inf): NaN -+ i inf
        if (d_internal_cplx_@S@_is_inf(y))
        {
            return d_internal_cplx_@S@_make(nan, -y);
        }

        // +-0 + iNaN: pi/2 + iNaN
        if (x == (@T@)0)
        {
            return d_internal_cplx_@S@_make(pio2, nan);
        }

        return d_internal_cplx_@S@_make(nan, nan);
    }

    // far from 0, infinities among it
    if ( (ax > recip_eps) ||
         (ay > recip_eps) )
    {
        const struct dj_cplx_@S@ w = d_internal_cplx_@S@_log_large@KS@(_z);
        const @T@ rx = d_internal_cplx_@S@_abs(w.part[1]);
        const @T@ ry = w.part[0] +
                       (@T@)0.693147180559945309417232121458176568L;

        return d_internal_cplx_@S@_make(rx, sy ? ry : -ry);
    }

    // 1 + i0: 0 - i0
    if ( (x == (@T@)1) &&
         (y == (@T@)0) )
    {
        return d_internal_cplx_@S@_make((@T@)0, -y);
    }

    // near 0: pi/2 - x - iy
    if ( (ax < (d_math_@S@_sqrt_c((@T@)6 * @EPS@) / (@T@)4)) &&
         (ay < (d_math_@S@_sqrt_c((@T@)6 * @EPS@) / (@T@)4)) )
    {
        return d_internal_cplx_@S@_make(pio2 - x, -y);
    }

    {
        const struct dj_internal_cplx_@S@_hard_work@KS@ h =
            d_internal_cplx_@S@_hard_work_of@KS@(ay, ax);
        const @T@ rx = h.b_is_usable
                           ? d_math_@S@_acos@R@(sx ? -h.b : h.b)
                           : d_math_@S@_atan2@R@(h.sqrt_a2my2,
                                                 sx ? -h.new_y : h.new_y);

        return d_internal_cplx_@S@_make(rx, sy ? h.rx : -h.rx);
    }
}

// d_internal_cplx_@S@_acosh@KS@
//   acosh _z from acos _z, as FreeBSD's catrig.c takes it: i acos z or
// -i acos z, whichever has a nonnegative real part, and Annex G's values
// (G.6.2.1) where a part is a NaN.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_acosh@KS@(
    struct dj_cplx_@S@ _z
)
{
    const struct dj_cplx_@S@ w = d_internal_cplx_@S@_acos@KS@(_z);
    const @T@ rx = w.part[0];
    const @T@ ry = w.part[1];

    // NaNs: NaNs
    if ( (rx != rx) &&
         (ry != ry) )
    {
        return d_internal_cplx_@S@_make(ry, rx);
    }

    // an infinity and a NaN: +inf + iNaN
    if (rx != rx)
    {
        return d_internal_cplx_@S@_make(d_internal_cplx_@S@_abs(ry), rx);
    }

    // +-0 + iNaN, whose acos is pi/2 + iNaN: NaN + i pi/2, as C17 has it
    // (its DR 471), where C11 and FreeBSD's give NaNs
    if (ry != ry)
    {
        return d_internal_cplx_@S@_make(ry, rx);
    }

    return d_internal_cplx_@S@_make(
        d_internal_cplx_@S@_abs(ry),
        d_internal_cplx_@S@_copysign(rx, _z.part[1]));
}

// d_internal_cplx_@S@_one_less_squares
//   1 - _a^2 - _b^2, _a and _b nonnegative and finite: the squares exact,
// their high parts summed with -1 exactly, so that the cancellation near
// the unit circle loses nothing but the last rounding.
D_MATH_FN @T@
d_internal_cplx_@S@_one_less_squares@KS@(
    @T@ _a,
    @T@ _b
)
{
#if @WIDE@
    // as long double pairs where @T@ is evaluated wider (section 2), the sum
    // rounded once
    const struct dj_internal_math_ld_pair aa =
        d_internal_math_ld_product_exact((long double)_a, (long double)_a);
    const struct dj_internal_math_ld_pair bb =
        d_internal_math_ld_product_exact((long double)_b, (long double)_b);
    const struct dj_internal_math_ld_pair t1 =
        d_internal_math_ld_two_sum(1.0L, -aa.high);
    const struct dj_internal_math_ld_pair t2 =
        d_internal_math_ld_two_sum(t1.high, -bb.high);
    const struct dj_internal_math_ld_pair v =
        d_internal_math_ld_two_sum(t2.high,
                                   (t1.low + t2.low) - (aa.low + bb.low));

    return d_internal_math_@S@_round_pair(v.high, v.low);
#else  // @WIDE@
    const struct dj_internal_math_@S@_pair aa =
        d_internal_cplx_@S@_product(_a, _a);
    const struct dj_internal_math_@S@_pair bb =
        d_internal_cplx_@S@_product(_b, _b);
    const struct dj_internal_math_@S@_pair t1 =
        d_internal_cplx_@S@_two_sum((@T@)1, -aa.high);
    const struct dj_internal_math_@S@_pair t2 =
        d_internal_cplx_@S@_two_sum(t1.high, -bb.high);

    return t2.high + ((t1.low + t2.low) - (aa.low + bb.low));
#endif  // @WIDE@
}

// d_internal_cplx_@S@_atanh@KS@
//   atanh _z: as FreeBSD's catrig.c has it, Kahan's formulas -- Re =
// log1p(4|x| / ((|x| - 1)^2 + y^2)) / 4, Im = atan2(2|y|, 1 - x^2 - y^2) / 2
// -- the second's 1 - x^2 - y^2 here taken exactly; far from 0, Re (1/z) +
// i(+-pi/2), by the quotient; near 0, z itself; and Annex G's values
// (G.6.2.3) at the infinities and NaNs.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_atanh@KS@(
    struct dj_cplx_@S@ _z
)
{
    const @T@ x    = _z.part[0];
    const @T@ y    = _z.part[1];
    const @T@ ax   = d_internal_cplx_@S@_abs(x);
    const @T@ ay   = d_internal_cplx_@S@_abs(y);
    const @T@ nan  = d_internal_math_@S@_nan();
    const @T@ pio2 = (@T@)1.570796326794896619231321691639751442L;
    const @T@ ln2  = (@T@)0.693147180559945309417232121458176568L;
    const @T@ eps  = @EPS@;
    const @T@ recip_eps = (@T@)1 / @EPS@;

    // the real segment [-1, 1]: the real atanh, y's zero
    if ( (y == (@T@)0) &&
         (ax <= (@T@)1) )
    {
        return d_internal_cplx_@S@_make(d_math_@S@_atanh@R@(x), y);
    }

    // the imaginary axis: x's zero and atan y, as accurate as atan
    if (x == (@T@)0)
    {
        return d_internal_cplx_@S@_make(x, d_math_@S@_atan@R@(y));
    }

    // a NaN part
    if ( (x != x) ||
         (y != y) )
    {
        // +-inf + iNaN: +-0 + iNaN
        if (d_internal_cplx_@S@_is_inf(x))
        {
            return d_internal_cplx_@S@_make(
                d_internal_cplx_@S@_copysign((@T@)0, x),
                y);
        }

        // NaN + i(+-inf): +-0 + i(+-pi/2)
        if (d_internal_cplx_@S@_is_inf(y))
        {
            return d_internal_cplx_@S@_make(
                d_internal_cplx_@S@_copysign((@T@)0, x),
                d_internal_cplx_@S@_copysign(pio2, y));
        }

        return d_internal_cplx_@S@_make(nan, nan);
    }

    // far from 0, infinities among it: Re(1/z) + i(+-pi/2)
    if ( (ax > recip_eps) ||
         (ay > recip_eps) )
    {
        const struct dj_cplx_@S@ q =
            d_cplx_@S@_div(d_internal_cplx_@S@_make((@T@)1, (@T@)0), _z);

        return d_internal_cplx_@S@_make(
            d_internal_cplx_@S@_copysign(q.part[0], x),
            d_internal_cplx_@S@_copysign(pio2, y));
    }

    // near 0: z itself
    if ( (ax < (d_math_@S@_sqrt_c((@T@)3 * eps) / (@T@)2)) &&
         (ay < (d_math_@S@_sqrt_c((@T@)3 * eps) / (@T@)2)) )
    {
        return _z;
    }

    {
        @T@ rx = (@T@)0;
        @T@ ry = (@T@)0;

        // the real part: near |x| = 1 and y = 0 its log form
        if ( (ax == (@T@)1) &&
             (ay < eps) )
        {
            rx = (ln2 - d_math_@S@_log@R@(ay)) / (@T@)2;
        }
        else
        {
            const @T@ xm1  = ax - (@T@)1;
            const @T@ sum2 = (ay < d_internal_math_@S@_pow2((@EMIN@ - 1) / 2))
                                 ? (xm1 * xm1)
                                 : ((xm1 * xm1) + (ay * ay));

            rx = d_math_@S@_log1p@R@(((@T@)4 * ax) / sum2) / (@T@)4;
        }

        // the imaginary part
        if (ax == (@T@)1)
        {
            ry = d_math_@S@_atan2@R@((@T@)2, -ay) / (@T@)2;
        }
        else
        {
            ry = d_math_@S@_atan2@R@(
                     (@T@)2 * ay,
                     d_internal_cplx_@S@_one_less_squares@KS@(ax, ay)) /
                 (@T@)2;
        }

        return d_internal_cplx_@S@_make(d_internal_cplx_@S@_copysign(rx, x),
                                        d_internal_cplx_@S@_copysign(ry, y));
    }
}
"""

# compositions: the trigonometric functions by the hyperbolic ones, asin and
# atan by asinh and atanh, as Annex G defines them
CPLX_COMPOSED = r"""
// d_internal_cplx_@S@_sin@KS@, d_internal_cplx_@S@_cos@KS@,
// d_internal_cplx_@S@_tan@KS@
//   sin z = -i sinh(iz), cos z = cosh(iz), tan z = -i tanh(iz): Annex G's
// definitions (G.6, its first paragraph), so that their special values are
// the hyperbolic functions' turned.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_sin@KS@(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_@S@_unturned(
        d_internal_cplx_@S@_sinh@KS@(d_internal_cplx_@S@_turned(_z)));
}

@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_cos@KS@(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_@S@_cosh@KS@(d_internal_cplx_@S@_turned(_z));
}

@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_tan@KS@(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_@S@_unturned(
        d_internal_cplx_@S@_tanh@KS@(d_internal_cplx_@S@_turned(_z)));
}

// d_internal_cplx_@S@_asin@KS@, d_internal_cplx_@S@_atan@KS@
//   asin z = -i asinh(iz) and atan z = -i atanh(iz), each the swapped
// function of the swapped argument (Annex G, G.6).
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_asin@KS@(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_@S@_swapped(
        d_internal_cplx_@S@_asinh@KS@(d_internal_cplx_@S@_swapped(_z)));
}

@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_atan@KS@(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_@S@_swapped(
        d_internal_cplx_@S@_atanh@KS@(d_internal_cplx_@S@_swapped(_z)));
}
"""

# the turns, once per family with the arithmetic
CPLX_TURNS = r"""
// d_internal_cplx_@S@_turned
//   i _z's coordinates: (-y, x), so that sin z = -i sinh(iz), cos z =
// cosh(iz) and tan z = -i tanh(iz) take the hyperbolic kernels.
D_MATH_FN struct dj_cplx_@S@
d_internal_cplx_@S@_turned(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_@S@_make(-_z.part[1], _z.part[0]);
}

// d_internal_cplx_@S@_unturned
//   -i _z's coordinates: (y, -x).
D_MATH_FN struct dj_cplx_@S@
d_internal_cplx_@S@_unturned(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_@S@_make(_z.part[1], -_z.part[0]);
}

// d_internal_cplx_@S@_swapped
//   (y, x): asin and atan by asinh and atanh, as Hull, Fairgrieve and Tang
// and FreeBSD's catrig.c take them.
D_MATH_FN struct dj_cplx_@S@
d_internal_cplx_@S@_swapped(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_@S@_make(_z.part[1], _z.part[0]);
}
"""

# the family's functions, each twice: _c on the long double kernel, rounded
# once to the family, and plain on the family's own kernel
CPLX_FUNCTIONS_HEAD = r"""
// @SECTION@.@SUB@    The @NAME@ family (_@S@)
//------------------------------------------------------------------------------
// Each function twice, as every real function in the core: the _c form a
// constant expression from C++14, the long double kernel of pure arithmetic
// (section 3.1) rounded once to the family -- a float or double part within
// half an ulp but where the kernel's error reaches a midpoint, a long double
// part within the kernel's few ulps -- and the plain form the family's own
// kernel on the C library's real functions (sections 3.2 to 3.4), within a
// few ulps of each part and fast. abs and arg are hypot and atan2 of the
// family themselves.

// d_internal_cplx_@S@_widen, d_internal_cplx_@S@_narrow
//   _z's parts as long doubles, exactly, and a long double result's parts
// rounded once to the family.
D_MATH_FN struct dj_cplx_ld
d_internal_cplx_@S@_widen(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_ld_make((long double)_z.part[0],
                                   (long double)_z.part[1]);
}

D_MATH_FN struct dj_cplx_@S@
d_internal_cplx_@S@_narrow(
    struct dj_cplx_ld _z
)
{
    return d_internal_cplx_@S@_make((@T@)_z.part[0],
                                    (@T@)_z.part[1]);
}

// @SECTION@.@SUB@.1
// d_cplx_@S@_abs_c, d_cplx_@S@_abs, d_cplx_@S@_arg_c, d_cplx_@S@_arg
//   function: the modulus, hypot(x, y), and the argument, atan2(y, x), in
// (-pi, pi], the family's own (C99's cabs and carg): +inf for an infinite
// part, the other a NaN or not.
D_MATH_FN @T@
d_cplx_@S@_abs_c(
    struct dj_cplx_@S@ _z
)
{
    return d_math_@S@_hypot_c(_z.part[0],
                              _z.part[1]);
}

D_MATH_FN_RT @T@
d_cplx_@S@_abs(
    struct dj_cplx_@S@ _z
)
{
    return d_math_@S@_hypot(_z.part[0],
                            _z.part[1]);
}

D_MATH_FN @T@
d_cplx_@S@_arg_c(
    struct dj_cplx_@S@ _z
)
{
    return d_math_@S@_atan2_c(_z.part[1],
                              _z.part[0]);
}

D_MATH_FN_RT @T@
d_cplx_@S@_arg(
    struct dj_cplx_@S@ _z
)
{
    return d_math_@S@_atan2(_z.part[1],
                            _z.part[0]);
}
"""

CPLX_UNARY_DOC = {
    "exp": ("e^z, e^x (cos y + i sin y) (C99's cexp)", 2),
    "log": ("the principal logarithm, log |z| + i arg z, its imaginary part in\n// (-pi, pi] (C99's clog)", 3),
    "sqrt": ("the principal square root, its real part nonnegative (C99's\n// csqrt)", 4),
    "sin": ("the sine, -i sinh(iz) (C99's csin)", 6),
    "cos": ("the cosine, cosh(iz) (C99's ccos)", 6),
    "tan": ("the tangent, -i tanh(iz) (C99's ctan)", 6),
    "asin": ("the principal arc-sine, -i asinh(iz) (C99's casin)", 8),
    "acos": ("the principal arc-cosine, its real part in [0, pi] (C99's\n// cacos)", 8),
    "atan": ("the principal arc-tangent, -i atanh(iz) (C99's catan)", 8),
    "sinh": ("the hyperbolic sine (C99's csinh)", 7),
    "cosh": ("the hyperbolic cosine (C99's ccosh)", 7),
    "tanh": ("the hyperbolic tangent (C99's ctanh)", 7),
    "asinh": ("the principal inverse hyperbolic sine (C99's casinh)", 9),
    "acosh": ("the principal inverse hyperbolic cosine, its real part\n// nonnegative (C99's cacosh)", 9),
    "atanh": ("the principal inverse hyperbolic tangent (C99's catanh)", 9),
}

CPLX_UNARY = r"""
// d_cplx_@S@_@F@_c, d_cplx_@S@_@F@
//   function: @DOC@.
D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_@F@_c(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_@S@_narrow(
        d_internal_cplx_ld_@F@_c(d_internal_cplx_@S@_widen(_z)));
}

D_MATH_FN_RT struct dj_cplx_@S@
d_cplx_@S@_@F@(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_@S@_@F@(_z);
}
"""

CPLX_POW = r"""
// d_cplx_@S@_pow_c, d_cplx_@S@_pow
//   function: the principal power, exp(w log z) (C99's cpow), with z^0 = 1,
// whole powers by repeated squaring, and the real and imaginary cases exact
// where their value is (section 3's pow).
D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_pow_c(
    struct dj_cplx_@S@ _z,
    struct dj_cplx_@S@ _w
)
{
    return d_internal_cplx_@S@_narrow(
        d_internal_cplx_ld_pow_c(d_internal_cplx_@S@_widen(_z),
                                 d_internal_cplx_@S@_widen(_w)));
}

D_MATH_FN_RT struct dj_cplx_@S@
d_cplx_@S@_pow(
    struct dj_cplx_@S@ _z,
    struct dj_cplx_@S@ _w
)
{
    return d_internal_cplx_@S@_pow(_z,
                                   _w);
}
"""

CPLX_COMPLEX = r"""
// @SECTION@.@SUB@    The @NAME@ family (_@S@)
//------------------------------------------------------------------------------
// d_cplx_@S@_from_complex, d_cplx_@S@_to_complex
//   function: a @T@ _Complex as a struct dj_cplx_@S@ and back, by a copy of
// its bytes: the two have one layout (C99 6.2.5).
D_MATH_FN_RT struct dj_cplx_@S@
d_cplx_@S@_from_complex(
    @T@ _Complex _z
)
{
    struct dj_cplx_@S@ result = { { (@T@)0, (@T@)0 } };

    memcpy(result.part,
           &_z,
           sizeof(result.part));

    return result;
}

D_MATH_FN_RT @T@ _Complex
d_cplx_@S@_to_complex(
    struct dj_cplx_@S@ _z
)
{
    @T@ _Complex result = (@T@)0;

    memcpy(&result,
           _z.part,
           sizeof(_z.part));

    return result;
}
"""

# the five's docs, whole, and where on the real axis each is the real
# function's: everywhere, x positive, or x past -1
CPLX_FIVE_DOC = {
    "exp2": ("2^z, 2^x (cos(y ln 2) + i sin(y ln 2)), y ln 2 reduced\n"
             "// by pi/2 exactly for every y. On the real axis, the real "
             "function's value\n"
             "// and y's zero, the _c form's _c and the plain form's plain; "
             "else, in both\n"
             "// forms, the long double kernel (section 3.1) rounded once "
             "to the family.", ""),
    "log2": ("the principal base-2 logarithm, log z / ln 2. On the\n"
             "// real axis, x positive, the real function's value and y's "
             "zero, the _c\n"
             "// form's _c and the plain form's plain; else, in both forms, "
             "the long\n"
             "// double kernel (section 3.1) rounded once to the family.",
             "positive"),
    "log10": ("the principal base-10 logarithm, log z / ln 10. On the\n"
              "// real axis, x positive, the real function's value and y's "
              "zero, the _c\n"
              "// form's _c and the plain form's plain; else, in both "
              "forms, the long\n"
              "// double kernel (section 3.1) rounded once to the family.",
              "positive"),
    "log1p": ("the principal logarithm of 1 + z, 1 + z never rounded.\n"
              "// On the real axis, x past -1, the real function's value "
              "and y's zero, the\n"
              "// _c form's _c and the plain form's plain; else, in both "
              "forms, the long\n"
              "// double kernel (section 3.1) rounded once to the family.",
              "past -1"),
    "expm1": ("e^z - 1, without the cancellation of e^z less 1. On\n"
              "// the real axis, the real function's value and y's zero, "
              "the _c form's _c\n"
              "// and the plain form's plain; else, in both forms, the "
              "long double kernel\n"
              "// (section 3.1) rounded once to the family.", ""),
}

CPLX_FIVE = r"""
// d_cplx_@S@_@F@_c, d_cplx_@S@_@F@
//   function: @DOC@
D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_@F@_c(
    struct dj_cplx_@S@ _z
)
{
    // on the real axis@WHERE@: the real function's own value
    if @AXIS@
    {
        return d_internal_cplx_@S@_make(d_math_@S@_@F@_c(_z.part[0]),
                                        _z.part[1]);
    }

    return d_internal_cplx_@S@_narrow(
        d_internal_cplx_ld_@F@_c(d_internal_cplx_@S@_widen(_z)));
}

D_MATH_FN_RT struct dj_cplx_@S@
d_cplx_@S@_@F@(
    struct dj_cplx_@S@ _z
)
{
    // on the real axis@WHERE@: the real function's own value
    if @AXIS@
    {
        return d_internal_cplx_@S@_make(d_math_@S@_@F@(_z.part[0]),
                                        _z.part[1]);
    }

    return d_internal_cplx_@S@_narrow(
        d_internal_cplx_ld_@F@_c(d_internal_cplx_@S@_widen(_z)));
}
"""

CPLX_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE TYPES
    ---------
    1.  The complex types
         1.  dj_cplx_f
         2.  dj_cplx_d
         3.  dj_cplx_ld
2.  THE ARITHMETIC
    --------------
    1.  The float family (_f)
    2.  The double family (_d)
    3.  The long double family (_ld)
3.  THE KERNELS
    -----------
    1.  Pure arithmetic, in long double
    2.  On the C library, in float
    3.  On the C library, in double
    4.  On the C library, in long double
4.  THE FUNCTIONS
    -------------
    1.  The float family (_f)
    2.  The double family (_d)
    3.  The long double family (_ld)
5.  C99'S _Complex (C ONLY)
    -----------------------
    1.  The float family (_f)
    2.  The double family (_d)
    3.  The long double family (_ld)
6.  ONE NAME FOR EVERY FAMILY (C++)
    -------------------------------
*/

#ifndef DJINTERP_C_MATH_CPLX_H
#define DJINTERP_C_MATH_CPLX_H 1

// std
#include <float.h>               // FLT_MAX ... LDBL_MAX, LDBL_MANT_DIG
#include <string.h>              // memcpy, for _Complex's conversions
// djinterp
#include "./elementary.h"        // d_math_*_sqrt_c, *_sin_c, *_asin_c, *_nan
#include "./exponential.h"       // d_math_*_exp_c, *_log1p_c, *_infinity
#include "./float_order.h"       // d_internal_math_*_pow2, *_binade_of
#include "./hyperbolic.h"        // d_math_*_sinh_c, *_atanh_c
#include "./math_common.h"       // D_MATH_FN
#include "./nearest_integer.h"   // d_math_*_floor_c
#include "./power.h"             // d_math_*_hypot_c, *_pow_c
#include "./tangent.h"           // d_math_*_atan2_c, *_negative
"""

CPLX_TAIL = r"""

#endif  // DJINTERP_C_MATH_CPLX_H
"""

CPLX_UNARIES = ["exp", "log", "sqrt", "sin", "cos", "tan", "asin", "acos",
                "atan", "sinh", "cosh", "tanh", "asinh", "acosh", "atanh"]

# the five C99 has only for real arguments, of a complex one
CPLX_FIVE_UNARIES = ["exp2", "log2", "log10", "log1p", "expm1"]

_CZ = ("struct dj_cplx_@S@", "_z")
_CW = ("struct dj_cplx_@S@", "_w")
_CX = ("@T@", "_x")
_CY = ("@T@", "_y")
CPLX_OVERLOADS = (
    [("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_make", [_CX, _CY]),
     ("D_MATH_FN", "@T@", "d_cplx_real", [_CZ]),
     ("D_MATH_FN", "@T@", "d_cplx_imag", [_CZ]),
     ("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_conj", [_CZ]),
     ("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_proj", [_CZ]),
     ("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_neg", [_CZ]),
     ("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_add", [_CZ, _CW]),
     ("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_sub", [_CZ, _CW]),
     ("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_mul", [_CZ, _CW]),
     ("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_div", [_CZ, _CW]),
     ("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_add_real", [_CZ, _CX]),
     ("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_sub_real", [_CZ, _CX]),
     ("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_real_sub", [_CX, _CZ]),
     ("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_mul_real", [_CZ, _CX]),
     ("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_div_real", [_CZ, _CX]),
     ("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_real_div", [_CX, _CZ]),
     ("D_MATH_FN", "bool", "d_cplx_equal", [_CZ, _CW]),
     ("D_MATH_FN", "@T@", "d_cplx_abs_c", [_CZ]),
     ("D_MATH_FN_RT", "@T@", "d_cplx_abs", [_CZ]),
     ("D_MATH_FN", "@T@", "d_cplx_arg_c", [_CZ]),
     ("D_MATH_FN_RT", "@T@", "d_cplx_arg", [_CZ])]
    + [entry
       for f in CPLX_UNARIES
       for entry in (("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_" + f + "_c", [_CZ]),
                     ("D_MATH_FN_RT", "struct dj_cplx_@S@", "d_cplx_" + f, [_CZ]))]
    + [("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_pow_c", [_CZ, _CW]),
       ("D_MATH_FN_RT", "struct dj_cplx_@S@", "d_cplx_pow", [_CZ, _CW])]
    + [entry
       for f in CPLX_FIVE_UNARIES
       for entry in (("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_" + f + "_c", [_CZ]),
                     ("D_MATH_FN_RT", "struct dj_cplx_@S@", "d_cplx_" + f, [_CZ]))]
)


CPLX_POW_GENERAL_PLAIN = r"""// d_internal_cplx_@S@_pow_general@KS@
//   _z^_w = exp(w log z) by the _c kernel's general case (section 3.1),
// rounded once to the family. C99's formula in the family's own type would
// carry the error exp magnifies, |w log z| times log's -- hundreds of steps
// at |w log z| near 16 -- and no C library function forms w log z any
// closer; each operation is as accurate as the best algorithm for it
// (S266), so the plain pow is fast only where section 3's pow takes a case
// of its own first, on the family's real functions.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_pow_general@KS@(
    struct dj_cplx_@S@ _z,
    struct dj_cplx_@S@ _w
)
{
    const struct dj_cplx_ld r = d_internal_cplx_ld_pow_general_c(
        d_internal_cplx_ld_make((long double)_z.part[0],
                                (long double)_z.part[1]),
        d_internal_cplx_ld_make((long double)_w.part[0],
                                (long double)_w.part[1]));

    return d_internal_cplx_@S@_make((@T@)r.part[0],
                                    (@T@)r.part[1]);
}

"""


CPLX_POW_GENERAL_PURE = r"""// d_internal_cplx_@S@_scale_pair@KS@
//   _p 2^_k rounded once, _p a pair: its magnitude as m 2^e, m in [1/sqrt(2),
// sqrt(2)], exactly, then scaled as the exponential scales its own (its
// section 4.1), into the subnormals and past the largest finite value; the
// sign last. A zero pair is a zero of its high part's sign.
@KQ@ @T@
d_internal_cplx_@S@_scale_pair@KS@(
    struct dj_internal_math_ld_pair _p,
    int                             _k
)
{
    const bool        negative = (_p.high < (@T@)0);
    const long double high     = negative ? -_p.high : _p.high;
    const long double low      = negative ? -_p.low : _p.low;

    if (_p.high == (@T@)0)
    {
        return _p.high;
    }

    {
        const struct dj_internal_math_ld_log_arg arg =
            d_internal_math_ld_log_arg_of(high);
        struct dj_internal_math_ld_exp_split split = { 1.0L, 0.0L, 0 };
        long double                          value = 0.0L;

        split.y    = arg.m;
        split.y_lo = d_internal_cplx_@S@_scale(low, -arg.exponent);
        split.k    = _k + arg.exponent;
        value      = d_internal_math_ld_exp_scale(split);

        return negative ? -value : value;
    }
}

// d_internal_cplx_@S@_log_modulus_pair@KS@
//   log |(_x + _x_lo) + i _y| as a pair, both parts finite and one nonzero,
// _x_lo below _x's last place (0 but where x is itself a pair): half the
// logarithm of |z|^2, the larger part scaled into [1, 2) by a power of 2 --
// the squares exactly, their sum and 2 x x_lo as a pair to 2^-2P of it, a
// smaller part that the scaling takes into the subnormals far below that --
// by the logarithm past long double's precision (power.h's section 1.5), the
// square in [1, 8): a normal value.
@KQ@ struct dj_internal_math_ld_pair
d_internal_cplx_@S@_log_modulus_pair@KS@(
    @T@ _x,
    @T@ _x_lo,
    @T@ _y
)
{
    const int         e0 = d_internal_cplx_@S@_magnitude_exponent(
                               d_internal_cplx_@S@_make(_x, _y));
    const long double sx    = d_internal_cplx_@S@_scale(_x, -e0);
    const long double sx_lo = d_internal_cplx_@S@_scale(_x_lo, -e0);
    const long double sy    = d_internal_cplx_@S@_scale(_y, -e0);
    const struct dj_internal_math_ld_pair xx =
        d_internal_math_ld_product_exact(sx, sx);
    const struct dj_internal_math_ld_pair yy =
        d_internal_math_ld_product_exact(sy, sy);
    const struct dj_internal_math_ld_pair head =
        d_internal_math_ld_two_sum(xx.high, yy.high);
    const struct dj_internal_math_ld_pair square =
        d_internal_math_ld_two_sum(
            head.high,
            head.low + ((xx.low + yy.low) + ((2.0L * sx) * sx_lo)));
    // log |z|^2 = log(square.high 2^(2 e0)) + square.low / square.high
    const struct dj_internal_math_ld_log_arg sa =
        d_internal_math_ld_log_arg_normal(square.high);
    const struct dj_internal_math_ld_pair twice =
        d_internal_math_ld_pair_sum(
            d_internal_math_ld_log_accurate_of(sa.exponent + (2 * e0),
                                               sa.m),
            d_internal_math_ld_pair_of(square.low / square.high, 0.0L));

    return d_internal_math_ld_pair_of(twice.high / 2.0L, twice.low / 2.0L);
}

// d_internal_cplx_@S@_pow_general@KS@
//   _z^_w = exp(w log z), with w log z carried as pairs, so that the error
// exp magnifies by |w log z| is the pairs' own, some 2^-(2P - 8) of it: log
// |z| as log_modulus_pair's (above); arg z as the arc-tangent's pair
// (tangent.h's section 1.3); each product of w's parts by Dekker's. Then
// e^Re as a pair (power.h's section 1.6), and the cosine and sine of Im,
// its high part reduced exactly (elementary.h's section 1.4) and its low
// part taken by the angle's sum; each part the product of the two, rounded
// once. For a part
// of _z or _w infinite or NaN, for _z = 0 and past |w| = 2^(EMAX/2), where
// Dekker's products could overflow, exp(w log z) itself, Annex G's values.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_pow_general@KS@(
    struct dj_cplx_@S@ _z,
    struct dj_cplx_@S@ _w
)
{
    const @T@ x   = _z.part[0];
    const @T@ y   = _z.part[1];
    const @T@ a   = _w.part[0];
    const @T@ b   = _w.part[1];
    const @T@ big = d_internal_math_@S@_pow2(@EMAX@ / 2);

    // what the pairs do not take: C99's formula, Annex G's values
    if ( (!d_internal_cplx_@S@_finite(_z)) ||
         (!d_internal_cplx_@S@_finite(_w)) ||
         ( (x == (@T@)0) &&
           (y == (@T@)0) ) ||
         (d_internal_cplx_@S@_abs(a) > big) ||
         (d_internal_cplx_@S@_abs(b) > big) )
    {
        return d_internal_cplx_@S@_exp@KS@(
            d_cplx_@S@_mul(_w, d_internal_cplx_@S@_log@KS@(_z)));
    }

    {
        // log |z|, as a pair
        const struct dj_internal_math_ld_pair lr =
            d_internal_cplx_@S@_log_modulus_pair@KS@(x, (@T@)0, y);
        struct dj_internal_math_ld_pair li = { 0.0L, 0.0L };

        // arg z: on an axis a multiple of pi/2, else the arc-tangent's pair
        if (y == (@T@)0)
        {
            li = (x > (@T@)0)
                     ? d_internal_math_ld_pair_of(y, 0.0L)
                     : d_internal_math_ld_atan_reduced(0.0L, 0.0L, 2, false);
        }
        else if (x == (@T@)0)
        {
            li = d_internal_math_ld_atan_reduced(0.0L, 0.0L, 1, false);
        }
        else
        {
            li = d_internal_math_ld_atan2_of(d_internal_cplx_@S@_abs(y), x);
        }

        if (d_internal_math_@S@_negative(y))
        {
            li.high = -li.high;
            li.low  = -li.low;
        }

        {
            // w log z = (a lr - b li) + i (a li + b lr), in pairs
            const struct dj_internal_math_ld_pair ap =
                d_internal_math_ld_pair_of(a, 0.0L);
            const struct dj_internal_math_ld_pair bp =
                d_internal_math_ld_pair_of(b, 0.0L);
            const struct dj_internal_math_ld_pair bli =
                d_internal_math_ld_pair_product(bp, li);
            const struct dj_internal_math_ld_pair tr =
                d_internal_math_ld_pair_sum(
                    d_internal_math_ld_pair_product(ap, lr),
                    d_internal_math_ld_pair_of(-bli.high, -bli.low));
            const struct dj_internal_math_ld_pair ti =
                d_internal_math_ld_pair_sum(
                    d_internal_math_ld_pair_product(ap, li),
                    d_internal_math_ld_pair_product(bp, lr));
            // the cosine and sine of ti's high part, by its quadrant
            const struct dj_internal_math_ld_sin_cos sc =
                d_internal_math_ld_sin_cos_of(ti.high);
            struct dj_internal_math_ld_pair c =
                ((sc.quadrant % 2) == 0) ? sc.cosine : sc.sine;
            struct dj_internal_math_ld_pair s =
                ((sc.quadrant % 2) == 0) ? sc.sine : sc.cosine;
            const @T@ inf = d_internal_math_@S@_infinity();

            if ( (sc.quadrant == 1) ||
                 (sc.quadrant == 2) )
            {
                c.high = -c.high;
                c.low  = -c.low;
            }

            if (sc.quadrant >= 2)
            {
                s.high = -s.high;
                s.low  = -s.low;
            }

            // ti's low part: to first order below 2^-40, where its square
            // lies past the pairs' precision; else by the angle's sum
            if (d_internal_math_ld_abs(ti.low) < d_internal_math_ld_pow2(-40))
            {
                const struct dj_internal_math_ld_pair c0 = c;

                c = d_internal_math_ld_two_sum(c.high,
                                               c.low - (ti.low * s.high));
                s = d_internal_math_ld_two_sum(s.high,
                                               s.low + (ti.low * c0.high));
            }
            else
            {
                const struct dj_internal_math_ld_sin_cos lo =
                    d_internal_math_ld_sin_cos_of(ti.low);
                const struct dj_internal_math_ld_pair cl =
                    ((lo.quadrant % 2) == 0) ? lo.cosine : lo.sine;
                const struct dj_internal_math_ld_pair sl =
                    ((lo.quadrant % 2) == 0) ? lo.sine : lo.cosine;
                const long double cs = ( (lo.quadrant == 1) ||
                                         (lo.quadrant == 2) ) ? -1.0L : 1.0L;
                const long double ss = (lo.quadrant >= 2) ? -1.0L : 1.0L;
                const struct dj_internal_math_ld_pair cl2 =
                    d_internal_math_ld_pair_of(cs * cl.high, cs * cl.low);
                const struct dj_internal_math_ld_pair sl2 =
                    d_internal_math_ld_pair_of(ss * sl.high, ss * sl.low);
                const struct dj_internal_math_ld_pair cc =
                    d_internal_math_ld_pair_product(c, cl2);
                const struct dj_internal_math_ld_pair snl =
                    d_internal_math_ld_pair_product(s, sl2);
                const struct dj_internal_math_ld_pair scl =
                    d_internal_math_ld_pair_product(s, cl2);
                const struct dj_internal_math_ld_pair csl =
                    d_internal_math_ld_pair_product(c, sl2);

                c = d_internal_math_ld_pair_sum(
                        cc, d_internal_math_ld_pair_of(-snl.high, -snl.low));
                s = d_internal_math_ld_pair_sum(scl, csl);
            }

            // far past either end of the range: infinities or zeros in the
            // angle's directions
            if (tr.high >= 16000.0L)
            {
                return d_internal_cplx_@S@_make(
                    (c.high < 0.0L) ? -inf : inf,
                    (s.high == 0.0L) ? (@T@)s.high
                                     : ((s.high < 0.0L) ? -inf : inf));
            }

            if (tr.high <= -16000.0L)
            {
                return d_internal_cplx_@S@_make(
                    (c.high < 0.0L) ? (@T@)-0.0L : (@T@)0,
                    (s.high < 0.0L) ? (@T@)-0.0L : (@T@)0);
            }

            {
                // e^tr as (y + y_lo) 2^k, times each of the angle's pairs
                const struct dj_internal_math_ld_exp_split e =
                    d_internal_math_ld_exp_pair_split_of(tr);
                const struct dj_internal_math_ld_pair m =
                    d_internal_math_ld_pair_of(e.y, e.y_lo);

                return d_internal_cplx_@S@_make(
                    d_internal_cplx_@S@_scale_pair@KS@(
                        d_internal_math_ld_pair_product(m, c), e.k),
                    d_internal_cplx_@S@_scale_pair@KS@(
                        d_internal_math_ld_pair_product(m, s), e.k));
            }
        }
    }
}

"""


# the five functions C99 has only for real arguments, of a complex one: their
# pure kernels, once, in long double (section 3.1), which both forms of every
# family round
CPLX_POW_INT_PURE = r"""// dj_internal_cplx_@S@_power@KS@
//   struct: a whole power as it is formed, (re + i im) 2^e, each part a pair,
// high + low, carried to some 2^-2P of the larger part (S370).
struct dj_internal_cplx_@S@_power@KS@
{
    struct dj_internal_math_ld_pair re;
    struct dj_internal_math_ld_pair im;
    long                            e;
};

// d_internal_cplx_@S@_pair_dot@KS@
//   _a _b + _c _d, each operand a pair, as a pair, by power.h's products and
// sums of pairs: to some 2^-2P of the larger term. Where the sum is exactly
// 0, the highs' own formula's zero, so that its sign is the one
// d_cplx_@S@_mul's exact products give (section 2.3).
@KQ@ struct dj_internal_math_ld_pair
d_internal_cplx_@S@_pair_dot@KS@(
    struct dj_internal_math_ld_pair _a,
    struct dj_internal_math_ld_pair _b,
    struct dj_internal_math_ld_pair _c,
    struct dj_internal_math_ld_pair _d
)
{
    struct dj_internal_math_ld_pair s = d_internal_math_ld_pair_sum(
        d_internal_math_ld_pair_product(_a, _b),
        d_internal_math_ld_pair_product(_c, _d));

    // exactly 0: the formula's zero, with its sign
    if (s.high == (@T@)0)
    {
        const @T@ t = (_a.high * _b.high) + (_c.high * _d.high);

        s.high = (t == (@T@)0) ? t : (@T@)0;
        s.low  = (@T@)0;
    }

    return s;
}

// d_internal_cplx_@S@_pair_negated@KS@
//   -_p, exactly.
@KQ@ struct dj_internal_math_ld_pair
d_internal_cplx_@S@_pair_negated@KS@(
    struct dj_internal_math_ld_pair _p
)
{
    return d_internal_math_ld_pair_of(-_p.high, -_p.low);
}

// d_internal_cplx_@S@_pair_over@KS@
//   _u / _n, both pairs, |_u| <= _n and _n in [1, 8), as a pair: the
// magnitudes' quotient by elementary.h's ratio, renormalized, the sign
// _u's. Where _u is exactly 0, _zero, the formula's zero, over _n, as
// d_internal_cplx_@S@_over takes a zero, so that the sign is the division's,
// the formula's (S382).
@KQ@ struct dj_internal_math_ld_pair
d_internal_cplx_@S@_pair_over@KS@(
    struct dj_internal_math_ld_pair _u,
    @T@                             _zero,
    struct dj_internal_math_ld_pair _n
)
{
    // exactly 0: the division's zero, its sign kept
    if (_u.high == (@T@)0)
    {
        const @T@ q = _zero / _n.high;

        return d_internal_math_ld_pair_of(q, (@T@)0);
    }

    {
        const bool negative = (_u.high < (@T@)0);
        const struct dj_internal_math_ld_pair a =
            negative ? d_internal_cplx_@S@_pair_negated@KS@(_u) : _u;
        const struct dj_internal_math_ld_pair r =
            d_internal_math_ld_ratio(a.high, a.low, _n.high, _n.low);
        const struct dj_internal_math_ld_pair q =
            d_internal_math_ld_two_sum(r.high, r.low);

        return negative ? d_internal_cplx_@S@_pair_negated@KS@(q) : q;
    }
}

// d_internal_cplx_@S@_power_times@KS@
//   _m _w, as pairs: (a + bi)(c + di) = (ac - bd) + (ad + bc)i, each part by
// d_internal_cplx_@S@_pair_dot@KS@, its terms in d_cplx_@S@_mul's order;
// _m's exponent kept.
@KQ@ struct dj_internal_cplx_@S@_power@KS@
d_internal_cplx_@S@_power_times@KS@(
    struct dj_internal_cplx_@S@_power@KS@ _m,
    struct dj_internal_cplx_@S@_power@KS@ _w
)
{
    struct dj_internal_cplx_@S@_power@KS@ result = _m;

    result.re = d_internal_cplx_@S@_pair_dot@KS@(
        _m.re, _w.re, d_internal_cplx_@S@_pair_negated@KS@(_m.im), _w.im);
    result.im = d_internal_cplx_@S@_pair_dot@KS@(_m.re, _w.im, _m.im, _w.re);

    return result;
}

// d_internal_cplx_@S@_power_inverse@KS@
//   1 / _m, as pairs, as d_cplx_@S@_div takes 1 over _m: the conjugate over
// the squared modulus n = re^2 + im^2, a pair in [1, 8) where _m's larger
// part lies in [1, 2), so that neither part exceeds it; each part's quotient
// by d_internal_cplx_@S@_pair_over@KS@, its zero the division's. _m's
// exponent kept.
@KQ@ struct dj_internal_cplx_@S@_power@KS@
d_internal_cplx_@S@_power_inverse@KS@(
    struct dj_internal_cplx_@S@_power@KS@ _m
)
{
    const struct dj_internal_math_ld_pair n =
        d_internal_cplx_@S@_pair_dot@KS@(_m.re, _m.re, _m.im, _m.im);
    struct dj_internal_cplx_@S@_power@KS@ result = _m;

    // the numerator (1 + 0i)(re - im i): 1 re + 0 im, and 0 re - 1 im
    result.re = d_internal_cplx_@S@_pair_over@KS@(
        _m.re,
        ((@T@)1 * _m.re.high) + ((@T@)0 * _m.im.high),
        n);
    result.im = d_internal_cplx_@S@_pair_over@KS@(
        d_internal_cplx_@S@_pair_negated@KS@(_m.im),
        ((@T@)0 * _m.re.high) + ((@T@)-1 * _m.im.high),
        n);

    return result;
}

// d_internal_cplx_@S@_power_rescaled@KS@
//   _m with the binade E of its larger part's high taken out of both parts,
// high and low, exactly, and added to its exponent: that part into [1, 2),
// so that the next product neither overflows nor underflows but far below
// it. _m not 0.
@KQ@ struct dj_internal_cplx_@S@_power@KS@
d_internal_cplx_@S@_power_rescaled@KS@(
    struct dj_internal_cplx_@S@_power@KS@ _m
)
{
    const @T@ a = d_internal_cplx_@S@_abs(_m.re.high);
    const @T@ b = d_internal_cplx_@S@_abs(_m.im.high);
    const int k = d_internal_cplx_@S@_exponent((a > b) ? a : b);
    struct dj_internal_cplx_@S@_power@KS@ result = _m;

    result.re = d_internal_math_ld_pair_of(
        d_internal_cplx_@S@_scale(_m.re.high, -k),
        d_internal_cplx_@S@_scale(_m.re.low, -k));
    result.im = d_internal_math_ld_pair_of(
        d_internal_cplx_@S@_scale(_m.im.high, -k),
        d_internal_cplx_@S@_scale(_m.im.low, -k));
    result.e  = _m.e + (long)k;

    return result;
}

// d_internal_cplx_@S@_pow_int_pairs@KS@
//   _z^_n, _z finite and not 0, _n an int not 0, as pairs and a power of 2:
// repeated squaring from the exponent's highest bit down, as the matrix
// power is (S262), each product of pairs and rescaled after it so that no
// step overflows or underflows, the exponent clamped where the result is
// certainly an infinity or 0; for _n < 0, 1 / z^|n| at the end, as pairs.
// Each product is within some 2^-2P of its larger part, so that a part that
// cancels keeps its own precision to within that, and a power exact in twice
// long double's precision comes exactly (S370).
@KQ@ struct dj_internal_cplx_@S@_power@KS@
d_internal_cplx_@S@_pow_int_pairs@KS@(
    struct dj_cplx_@S@ _z,
    int                _n
)
{
    const unsigned           count = (_n < 0) ? (0u - (unsigned)_n)
                                              : (unsigned)_n;
    const long               limit =
        4L * ((long)@EMAX@ + (long)@P@);
    const int                e0    = d_internal_cplx_@S@_magnitude_exponent(_z);
    const struct dj_cplx_@S@ z     = d_internal_cplx_@S@_scaled_by(_z, -e0);
    struct dj_internal_cplx_@S@_power@KS@ base =
        { { (@T@)0, (@T@)0 },
          { (@T@)0, (@T@)0 },
          0L };
    struct dj_internal_cplx_@S@_power@KS@ m =
        { { (@T@)0, (@T@)0 },
          { (@T@)0, (@T@)0 },
          0L };
    unsigned bit = 1u;

    base.re = d_internal_math_ld_pair_of(z.part[0], (@T@)0);
    base.im = d_internal_math_ld_pair_of(z.part[1], (@T@)0);
    base.e  = (long)e0;
    m       = base;

    // the highest bit
    while ((bit <= (count / 2u)) && (bit < 0x40000000u))
    {
        bit = bit * 2u;
    }

    for (bit = bit / 2u; bit > 0u; bit = bit / 2u)
    {
        m   = d_internal_cplx_@S@_power_times@KS@(m, m);
        m.e = 2L * m.e;
        m   = d_internal_cplx_@S@_power_rescaled@KS@(m);

        if ((count & bit) != 0u)
        {
            m   = d_internal_cplx_@S@_power_times@KS@(m, base);
            m.e = m.e + (long)e0;
            m   = d_internal_cplx_@S@_power_rescaled@KS@(m);
        }

        // far past either end: the clamp keeps e in a long
        if (m.e > limit)
        {
            m.e = limit;
        }
        else if (m.e < -limit)
        {
            m.e = -limit;
        }
    }

    if (_n < 0)
    {
        m   = d_internal_cplx_@S@_power_inverse@KS@(m);
        m.e = -m.e;
    }

    return m;
}

// d_internal_cplx_@S@_pow_int@KS@
//   _z^_n, _z finite and not 0, _n an int not 0: the pairs of
// d_internal_cplx_@S@_pow_int_pairs@KS@, each part rounded once
// (d_internal_cplx_@S@_scale_pair@KS@), into the subnormals and past the
// largest finite value.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_pow_int@KS@(
    struct dj_cplx_@S@ _z,
    int                _n
)
{
    const struct dj_internal_cplx_@S@_power@KS@ m =
        d_internal_cplx_@S@_pow_int_pairs@KS@(_z, _n);

    return d_internal_cplx_@S@_make(
        d_internal_cplx_@S@_scale_pair@KS@(m.re, (int)m.e),
        d_internal_cplx_@S@_scale_pair@KS@(m.im, (int)m.e));
}

"""


CPLX_POW_INT_PLAIN = r"""// d_internal_cplx_@S@_pow_int@KS@
//   _z^_n, _z finite and not 0, _n an int not 0: the _c kernel's (section
// 3.1), its repeated squaring carried in pairs, rounded to long double once
// and then to the family, as the _c form's is (S370): no faster in the
// family's own type to the same precision, so the plain form's whole power
// is the _c form's, bit for bit.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_pow_int@KS@(
    struct dj_cplx_@S@ _z,
    int                _n
)
{
    const struct dj_cplx_ld r = d_internal_cplx_ld_pow_int_c(
        d_internal_cplx_ld_make((long double)_z.part[0],
                                (long double)_z.part[1]),
        _n);

    return d_internal_cplx_@S@_make((@T@)r.part[0],
                                    (@T@)r.part[1]);
}

"""


CPLX_FIVE_PURE = r"""
// The five functions C99 has only for real arguments, of a complex one:
// each in pure arithmetic, once, in long double, its parts carried as pairs
// to past twice the value's precision and rounded once, so that the _c and
// the plain forms alike are this kernel rounded to the family (section 4).
// Where a part is infinite or NaN, the value of the function C99 has that
// this one is a form of: clog's for log2, log10 and log1p, cexp's for exp2
// and expm1 (G.6.3.1, G.6.3.2).

// d_internal_cplx_@S@_times_pair@KS@
//   _p _k rounded once, _p and _k pairs: a zero _p its own zero, of its sign.
@KQ@ @T@
d_internal_cplx_@S@_times_pair@KS@(
    struct dj_internal_math_ld_pair _p,
    struct dj_internal_math_ld_pair _k
)
{
    // a zero: its sign kept, as the product's two-sum would not
    if (_p.high == 0.0L)
    {
        return _p.high;
    }

    return d_internal_math_ld_pair_product(_p, _k).high;
}

// d_internal_cplx_@S@_arg_pair@KS@
//   arg(_x + i _y) as a pair, neither part a NaN: an infinite part taken as
// +-1 and a finite one beside it as a zero of its sign, so that the angle is
// atan2's at the infinities (G.6.3.2); on the real axis 0 for a positive x
// or +0 and pi for a negative one or -0, on the imaginary axis pi/2, each as
// its pair holds it; else the arc-tangent's pair (tangent.h's section 1.3);
// y's sign last, a zero's among them.
@KQ@ struct dj_internal_math_ld_pair
d_internal_cplx_@S@_arg_pair@KS@(
    @T@ _x,
    @T@ _y
)
{
    const bool        infinite = ( (d_internal_cplx_@S@_is_inf(_x)) ||
                                   (d_internal_cplx_@S@_is_inf(_y)) );
    const long double one_x    = d_internal_cplx_@S@_is_inf(_x) ? 1.0L
                                                                : 0.0L;
    const long double one_y    = d_internal_cplx_@S@_is_inf(_y) ? 1.0L
                                                                : 0.0L;
    const long double x = infinite
                              ? d_internal_cplx_@S@_copysign(one_x, _x)
                              : _x;
    const long double y = infinite
                              ? d_internal_cplx_@S@_copysign(one_y, _y)
                              : _y;
    struct dj_internal_math_ld_pair result = { 0.0L, 0.0L };

    // the real axis: 0 or pi, by x's sign
    if (y == (@T@)0)
    {
        result = d_internal_math_@S@_negative(x)
                     ? d_internal_math_ld_atan_reduced(0.0L, 0.0L, 2, false)
                     : d_internal_math_ld_pair_of(0.0L, 0.0L);
    }
    // the imaginary axis: pi/2
    else if (x == (@T@)0)
    {
        result = d_internal_math_ld_atan_reduced(0.0L, 0.0L, 1, false);
    }
    else
    {
        result = d_internal_math_ld_atan2_of(d_internal_cplx_@S@_abs(y), x);
    }

    // y's sign, a zero's among them
    if (d_internal_math_@S@_negative(y))
    {
        result.high = -result.high;
        result.low  = -result.low;
    }

    return result;
}

// d_internal_cplx_@S@_log_in@KS@
//   log _z / ln 2 for _ten false, log _z / ln 10 for _ten true: log |z| and
// arg z as pairs, each times 1/ln b as a pair -- a head of 32 bits, exact as
// written, and the rest, as exponential.h's section 1.4 holds them -- and
// rounded once; on an axis, where |z| is |x| or |y|, the real part the real
// function's, so that a power of b gives its exponent exactly. At 0 and an
// infinite part, clog's real part, -inf or +inf, and atan2's angle over ln
// b; at a NaN, clog's NaNs.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_log_in@KS@(
    struct dj_cplx_@S@ _z,
    bool               _ten
)
{
    const @T@ x = _z.part[0];
    const @T@ y = _z.part[1];
    // 1/ln b = k_hi + k_lo, k_hi cut to 32 bits: exact as written
    const struct dj_internal_math_ld_pair k = _ten
        ? d_internal_math_ld_pair_of(
              0.434294481878168880939483642578125L,
              2.508294671164527633848008229439700580367e-11L)
        : d_internal_math_ld_pair_of(
              1.4426950407214462757110595703125L,
              1.67517131648865110689392137426645954153e-10L);

    // a NaN part: clog's value, its imaginary part a NaN
    if ( (x != x) ||
         (y != y) )
    {
        return d_internal_cplx_@S@_log@KS@(_z);
    }

    // zero or an infinite part: log |z| is -inf or +inf
    if ( ( (x == (@T@)0) &&
           (y == (@T@)0) ) ||
         (!d_internal_cplx_@S@_finite(_z)) )
    {
        const long double size = d_math_@S@_hypot@R@(x, y);

        return d_internal_cplx_@S@_make(
            d_math_@S@_log@R@(size),
            d_internal_cplx_@S@_times_pair@KS@(
                d_internal_cplx_@S@_arg_pair@KS@(x, y), k));
    }

    // an axis: |z| a part's magnitude, the real function's logarithm of it
    if ( (x == (@T@)0) ||
         (y == (@T@)0) )
    {
        const long double m = d_internal_cplx_@S@_abs((y == 0.0L) ? x
                                                                  : y);

        return d_internal_cplx_@S@_make(
            _ten ? d_math_@S@_log10@R@(m)
                 : d_math_@S@_log2@R@(m),
            d_internal_cplx_@S@_times_pair@KS@(
                d_internal_cplx_@S@_arg_pair@KS@(x, y), k));
    }

    return d_internal_cplx_@S@_make(
        d_internal_cplx_@S@_times_pair@KS@(
            d_internal_cplx_@S@_log_modulus_pair@KS@(x, (@T@)0, y), k),
        d_internal_cplx_@S@_times_pair@KS@(
            d_internal_cplx_@S@_arg_pair@KS@(x, y), k));
}

// d_internal_cplx_@S@_log2@KS@, d_internal_cplx_@S@_log10@KS@
//   log2 _z and log10 _z (above).
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_log2@KS@(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_@S@_log_in@KS@(_z, false);
}

@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_log10@KS@(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_@S@_log_in@KS@(_z, true);
}

// d_internal_cplx_@S@_log1p@KS@
//   log(1 + _z), 1 + z never rounded: 1 + x is the pair two-sum gives, and
// its arc-tangent with y the pair's to first order in its low part. log |1 +
// z| is half the logarithm of |1 + z|^2 = 1 + w, w = 2x + x^2 + y^2 summed
// exactly as an expansion (power.h's section 1.1) and rounded to a pair,
// where 1 + w lies in [1/2, 2] -- so that the logarithm of a value near 1
// keeps w's own precision, however near w is to 0 -- and elsewhere of the
// squares of 1 + x's pair and y, as clog's power takes |z|^2 (above); past
// 2^30 in a part, of 1 + x rounded, which then moves log |1 + z|, past 20,
// by under 2^-(P+1) of it. At an infinite or NaN part, clog's value at 1 +
// z; at z = -1, -inf with y's zero.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_log1p@KS@(
    struct dj_cplx_@S@ _z
)
{
    const @T@ x   = _z.part[0];
    const @T@ y   = _z.part[1];
    const @T@ big = (@T@)1073741824.0L;

    // an infinite or NaN part: clog of 1 + z, which keeps it
    if (!d_internal_cplx_@S@_finite(_z))
    {
        return d_internal_cplx_@S@_log@KS@(
            d_internal_cplx_@S@_make(x + (@T@)1, y));
    }

    // -1: the logarithm of a zero
    if ( (x == (@T@)-1) &&
         (y == (@T@)0) )
    {
        return d_internal_cplx_@S@_make(-d_internal_math_@S@_infinity(), y);
    }

    {
        // 1 + x exactly, as a pair
        const struct dj_internal_math_ld_pair u =
            d_internal_math_ld_two_sum(1.0L, x);
        const @T@ ax = d_internal_cplx_@S@_abs(x);
        const @T@ ay = d_internal_cplx_@S@_abs(y);
        struct dj_internal_math_ld_pair half = { 0.0L, 0.0L };
        struct dj_internal_math_ld_pair angle = { 0.0L, 0.0L };

        // far out: log |1 + z| of 1 + x rounded
        if ( (ax >= big) ||
             (ay >= big) )
        {
            half = d_internal_cplx_@S@_log_modulus_pair@KS@(u.high, (@T@)0,
                                                            y);
        }
        else
        {
            // |1 + z|^2 roughly, to choose the way
            const long double rough = (u.high * u.high) + (y * y);

            if ( (rough >= 0.5L) &&
                 (rough <= 2.0L) )
            {
                // w = 2x + x^2 + y^2, every term exact, summed exactly
                const struct dj_internal_math_ld_pair xx =
                    d_internal_math_ld_product_exact(x, x);
                const struct dj_internal_math_ld_pair yy =
                    d_internal_math_ld_product_exact(y, y);
                struct dj_internal_math_ld_expansion w = { { 0.0L }, 0 };
                struct dj_internal_math_ld_pair      one_w = { 1.0L, 0.0L };
                int                                  i     = 0;

                w = d_internal_math_ld_expansion_grow(w, 2.0L * x);
                w = d_internal_math_ld_expansion_grow(w, xx.high);
                w = d_internal_math_ld_expansion_grow(w, xx.low);
                w = d_internal_math_ld_expansion_grow(w, yy.high);
                w = d_internal_math_ld_expansion_grow(w, yy.low);

                // 1 + w, from the least part up: the parts do not overlap,
                // so each pair sum loses only below 2^-2P of the whole
                for (i = 0; i < w.count; ++i)
                {
                    one_w = d_internal_math_ld_pair_sum(
                        one_w, d_internal_math_ld_pair_of(w.part[i], 0.0L));
                }

                {
                    // log(1 + w) = log(high) + low / high, high in [1/2, 2]
                    const struct dj_internal_math_ld_log_arg a =
                        d_internal_math_ld_log_arg_normal(one_w.high);
                    const struct dj_internal_math_ld_pair twice =
                        d_internal_math_ld_pair_sum(
                            d_internal_math_ld_log_accurate_of(a.exponent,
                                                               a.m),
                            d_internal_math_ld_pair_of(
                                one_w.low / one_w.high, 0.0L));

                    half = d_internal_math_ld_pair_of(twice.high / 2.0L,
                                                      twice.low / 2.0L);
                }
            }
            else
            {
                half = d_internal_cplx_@S@_log_modulus_pair@KS@(u.high,
                                                                u.low, y);
            }
        }

        // arg(1 + z): on the real axis pi, on the imaginary pi/2; else the
        // arc-tangent of y over 1 + x's high part, less y u_lo / |1 + z|^2
        if (y == (@T@)0)
        {
            angle = d_internal_math_ld_atan_reduced(0.0L, 0.0L, 2, false);
        }
        else if (u.high == 0.0L)
        {
            angle = d_internal_math_ld_atan_reduced(0.0L, 0.0L, 1, false);
        }
        else
        {
            const long double d = (u.high * u.high) + (ay * ay);

            angle = d_internal_math_ld_atan2_of(ay, u.high);

            // the low part's correction: below the angle's last place, its
            // square below 2^-2P of it; d is finite while |z| < 2^30
            if ( (u.low != 0.0L) &&
                 (ax < big) &&
                 (ay < big) )
            {
                angle = d_internal_math_ld_two_sum(
                    angle.high, angle.low - ((ay * u.low) / d));
            }
        }

        // y's sign, a zero's among them
        if (d_internal_math_@S@_negative(y))
        {
            angle.high = -angle.high;
            angle.low  = -angle.low;
        }

        return d_internal_cplx_@S@_make(half.high, angle.high);
    }
}

// d_internal_cplx_@S@_sin_cos_series@KS@
//   sin _r and cos _r as pairs, |_r| <= pi/4 (a last place past it does no
// harm), in the struct's sine and cosine, its quadrant 0: their Taylor series
// in pairs, z = r^2 -- sin r = r (1 + z (-1/6 + z (1/120 + ... + z (1/11! + z
// q)))) and cos r = 1 + z (-1/2 + z (1/24 + ... + z (1/12! + z q'))), each
// coefficient's head cut to 32 bits, exact as written, and the rest, q and q'
// the series' remainders in long double -- so that each value is good to
// some 2^-(P+28) of itself, past elementary.h's own, whose terms round in
// long double: where two such values are subtracted, as e^z - 1 subtracts
// them, the difference keeps that precision.
@KQ@ struct dj_internal_math_ld_sin_cos
d_internal_cplx_@S@_sin_cos_series@KS@(
    long double _r
)
{
    // 1/3!, 1/5!, 1/7!, 1/9!, 1/11!: heads of 32 bits, exact as written
    const struct dj_internal_math_ld_pair s3 = d_internal_math_ld_pair_of(
        0.166666666627861559391021728515625L,
        3.880510727564493815104166666666666666667e-11L);
    const struct dj_internal_math_ld_pair s5 = d_internal_math_ld_pair_of(
        0.00833333333139307796955108642578125L,
        1.940255363782246907552083333333333333333e-12L);
    const struct dj_internal_math_ld_pair s7 = d_internal_math_ld_pair_of(
        0.0001984126984098111279308795928955078125L,
        2.887284767533105517190600198412698412698e-15L);
    const struct dj_internal_math_ld_pair s9 = d_internal_math_ld_pair_of(
        0.0000027557319217663689414621330797672271728515625L,
        6.322201237935988426313618924041694223986e-16L);
    const struct dj_internal_math_ld_pair s11 = d_internal_math_ld_pair_of(
        0.000000025052108380829718470295119914226233959197998046875L,
        4.612000304756988471215484815854110338567e-18L);
    // 1/4!, 1/6!, 1/8!, 1/10!, 1/12!: heads of 32 bits, exact as written
    const struct dj_internal_math_ld_pair c4 = d_internal_math_ld_pair_of(
        0.04166666665696538984775543212890625L,
        9.701276818911234537760416666666666666667e-12L);
    const struct dj_internal_math_ld_pair c6 = d_internal_math_ld_pair_of(
        0.0013888888888686778955161571502685546875L,
        2.021099337273173862033420138888888888889e-14L);
    const struct dj_internal_math_ld_pair c8 = d_internal_math_ld_pair_of(
        0.0000248015873012263909913599491119384765625L,
        3.609105959416381896488250248015873015873e-16L);
    const struct dj_internal_math_ld_pair c10 = d_internal_math_ld_pair_of(
        0.00000027557319215443243365371017716825008392333984375L,
        8.542647287186301507160882260223334848986e-17L);
    const struct dj_internal_math_ld_pair c12 = d_internal_math_ld_pair_of(
        0.000000002087675698113355959861792143783532083034515380859375L,
        6.734539380592168883366111482198269845941e-19L);
    const struct dj_internal_math_ld_pair r = d_internal_math_ld_pair_of(_r,
                                                                         0.0L);
    // z = r^2, exactly
    const struct dj_internal_math_ld_pair z =
        d_internal_math_ld_product_exact(_r, _r);
    struct dj_internal_math_ld_pair    sine   = { 0.0L, 0.0L };
    struct dj_internal_math_ld_pair    cosine = { 0.0L, 0.0L };
    struct dj_internal_math_ld_sin_cos result =
        { { 0.0L, 0.0L }, { 0.0L, 0.0L }, 0 };
    long double                        q      = 0.0L;
    long double                        qc     = 0.0L;
    int                                k      = 0;

    // the remainders: sin's from -1/13!, cos's from -1/14!, each by its
    // terms' ratios, to their 22nd terms
    for (k = 22; k >= 6; --k)
    {
        q = 1.0L - ((z.high * q) /
                    (long double)(((2 * k) + 2) * ((2 * k) + 3)));
    }

    for (k = 22; k >= 7; --k)
    {
        qc = 1.0L - ((z.high * qc) /
                     (long double)(((2 * k) + 1) * ((2 * k) + 2)));
    }

    q  = -q / 6227020800.0L;
    qc = -qc / 87178291200.0L;

    // sin r = r (1 - z (1/3! - z (1/5! - z (1/7! - z (1/9! - z (1/11! +
    // z q)))))), in pairs
    sine = d_internal_math_ld_pair_sum(
        s11, d_internal_math_ld_pair_product(
                 z, d_internal_math_ld_pair_of(q, 0.0L)));
    sine = d_internal_math_ld_pair_sum(
        s9, d_internal_math_ld_pair_product(
                z, d_internal_math_ld_pair_of(-sine.high, -sine.low)));
    sine = d_internal_math_ld_pair_sum(
        s7, d_internal_math_ld_pair_product(
                z, d_internal_math_ld_pair_of(-sine.high, -sine.low)));
    sine = d_internal_math_ld_pair_sum(
        s5, d_internal_math_ld_pair_product(
                z, d_internal_math_ld_pair_of(-sine.high, -sine.low)));
    sine = d_internal_math_ld_pair_sum(
        s3, d_internal_math_ld_pair_product(
                z, d_internal_math_ld_pair_of(-sine.high, -sine.low)));
    sine = d_internal_math_ld_pair_product(
        z, d_internal_math_ld_pair_of(-sine.high, -sine.low));
    sine = d_internal_math_ld_pair_sum(
        r, d_internal_math_ld_pair_product(r, sine));

    // cos r = 1 - z (1/2 - z (1/4! - z (1/6! - z (1/8! - z (1/10! - z
    // (1/12! + z q')))))), in pairs
    cosine = d_internal_math_ld_pair_sum(
        c12, d_internal_math_ld_pair_product(
                 z, d_internal_math_ld_pair_of(qc, 0.0L)));
    cosine = d_internal_math_ld_pair_sum(
        c10, d_internal_math_ld_pair_product(
                 z, d_internal_math_ld_pair_of(-cosine.high, -cosine.low)));
    cosine = d_internal_math_ld_pair_sum(
        c8, d_internal_math_ld_pair_product(
                z, d_internal_math_ld_pair_of(-cosine.high, -cosine.low)));
    cosine = d_internal_math_ld_pair_sum(
        c6, d_internal_math_ld_pair_product(
                z, d_internal_math_ld_pair_of(-cosine.high, -cosine.low)));
    cosine = d_internal_math_ld_pair_sum(
        c4, d_internal_math_ld_pair_product(
                z, d_internal_math_ld_pair_of(-cosine.high, -cosine.low)));
    cosine = d_internal_math_ld_pair_sum(
        d_internal_math_ld_pair_of(0.5L, 0.0L),
        d_internal_math_ld_pair_product(
            z, d_internal_math_ld_pair_of(-cosine.high, -cosine.low)));
    cosine = d_internal_math_ld_pair_sum(
        d_internal_math_ld_pair_of(1.0L, 0.0L),
        d_internal_math_ld_pair_product(
            z, d_internal_math_ld_pair_of(-cosine.high, -cosine.low)));

    result.sine   = sine;
    result.cosine = cosine;

    return result;
}

// d_internal_cplx_@S@_angle@KS@
//   cos and sin of _q pi/2 + _r + _r_lo as pairs, in the struct's cosine and
// sine, its quadrant 0: _r finite, reduced by pi/2 (elementary.h's section
// 1.3) and the remainder's sine and cosine taken by the series above, _q
// quarter turns added to its quadrant, and _r_lo, below 2^-60 of 1, and the
// reduction's own low part, to first order, their squares past the pairs'
// precision.
@KQ@ struct dj_internal_math_ld_sin_cos
d_internal_cplx_@S@_angle@KS@(
    long double _r,
    long double _r_lo,
    int         _q
)
{
    const struct dj_internal_math_ld_reduced reduced =
        d_internal_math_ld_reduce(_r);
    const struct dj_internal_math_ld_sin_cos sc =
        d_internal_cplx_@S@_sin_cos_series@KS@(reduced.r);
    const long double low = reduced.r_lo + _r_lo;
    const int q = (reduced.quadrant + _q) % 4;
    // cos: c, -s, -c, s; sin: s, c, -s, -c, by quadrant
    struct dj_internal_math_ld_pair c = ((q % 2) == 0) ? sc.cosine : sc.sine;
    struct dj_internal_math_ld_pair s = ((q % 2) == 0) ? sc.sine : sc.cosine;
    struct dj_internal_math_ld_sin_cos result =
        { { 0.0L, 0.0L }, { 0.0L, 0.0L }, 0 };

    if ( (q == 1) ||
         (q == 2) )
    {
        c.high = -c.high;
        c.low  = -c.low;
    }

    if (q >= 2)
    {
        s.high = -s.high;
        s.low  = -s.low;
    }

    result.cosine = d_internal_math_ld_two_sum(c.high,
                                               c.low - (low * s.high));
    result.sine   = d_internal_math_ld_two_sum(s.high,
                                               s.low + (low * c.high));

    return result;
}

// d_internal_cplx_@S@_ln2_turns_digit
//   2 ln 2 / pi's digit _index, base 2^24, from the binary point, as
// elementary.h's section 1.2 holds 2/pi's -- 704 digits, 16,896 bits, as a
// string literal, three bytes a digit, computed by the generator -- so that y
// ln 2 = (y 2 ln 2 / pi) pi/2 is reduced as elementary.h reduces x = (x 2 /
// pi) pi/2. 0 for a negative _index, 2 ln 2 / pi having no integer part.
@KQ@ long double
d_internal_cplx_@S@_ln2_turns_digit(
    int _index
)
{
    // 2 ln 2 / pi's first 16,896 bits, three bytes a digit, highest first
    const char* const bytes =
@LN2_TURNS@;

    if ( (_index < 0) ||
         (_index >= 704) )
    {
        return 0.0L;
    }

    return ( ((long double)(unsigned char)bytes[3 * _index] * 65536.0L) +
             ((long double)(unsigned char)bytes[(3 * _index) + 1] * 256.0L) ) +
           (long double)(unsigned char)bytes[(3 * _index) + 2];
}

// d_internal_cplx_@S@_ln2_reduced@KS@
//   _y ln 2 = n pi/2 + (r + r_lo), |r| <= pi/4, for |_y| >= 1 and finite:
// elementary.h's reduction by Payne and Hanek's method (its section 1.3),
// with 2 ln 2 / pi's digits in 2/pi's place -- y's digits, base 2^24, times
// as many of them as y's exponent reaches, in exact columns of 48-bit
// products; the integer part kept only mod 4, the fraction from its first
// nonzero digit to past twice long double's precision, then times pi/2 --
// so that y ln 2 is reduced exactly for every y, as no product y ln 2 in any
// precision could be past 2^P.
@KQ@ struct dj_internal_math_ld_reduced
d_internal_cplx_@S@_ln2_reduced@KS@(
    long double _y
)
{
    const long double radix   = 16777216.0L;
    const long double ay      = (_y < 0.0L) ? -_y : _y;
    const struct dj_internal_math_ld_binade binade =
        d_internal_math_ld_binade_of(ay);
    // 2^(24 a) <= |y| < 2^(24 (a + 1))
    const int         a       = binade.exponent / 24;
    // pi/2 = pio2_hi + pio2_lo, pio2_hi of 31 bits: exact as written
    const long double pio2_hi = 1.570796326734125614166259765625L;
    const long double pio2_lo = 6.077100506506192601475144209858469968755e-11L;
    long double       digits[6]   = { 0.0L, 0.0L, 0.0L, 0.0L, 0.0L, 0.0L };
    long double       columns[21] = { 0.0L };
    long double       v           = ay * d_internal_math_ld_pow2(-24 * a);
    long double       quadrant    = 0.0L;
    bool              negative    = false;
    int               count       = 0;
    int               first       = 1;
    int               i           = 0;
    int               c           = 0;
    struct dj_internal_math_ld_reduced result = { 0.0L, 0.0L, 0 };

    // y's digits, base 2^24, from 2^(24 a) down: six reach 120 bits
    for (i = 0; ( (i < 6) && (v != 0.0L) ); ++i)
    {
        digits[i] = d_internal_math_ld_floor_small(v);
        v         = (v - digits[i]) * radix;
        count     = i + 1;
    }

    // y 2 ln 2 / pi in columns of weight 2^(-24 c): the columns of weight
    // 2^24 and up, multiples of 4, left out; every sum exact
    for (c = 0; c <= 20; ++c)
    {
        long double sum = 0.0L;

        for (i = 0; i < count; ++i)
        {
            sum += digits[i] *
                   d_internal_cplx_@S@_ln2_turns_digit(c + a - 1 - i);
        }

        columns[c] = sum;
    }

    // the carries, from the last column up
    for (c = 20; c >= 1; --c)
    {
        const long double carry =
            d_internal_math_ld_floor_small(columns[c] / radix);

        columns[c]     -= carry * radix;
        columns[c - 1] += carry;
    }

    // the integer part, mod 4
    quadrant = columns[0] -
               (4.0L * d_internal_math_ld_floor_small(columns[0] / 4.0L));

    // to the nearest integer: a fraction of a half or more is one more turn
    // and, below it, 1 - the fraction, its digits complemented
    if (columns[1] >= (radix / 2.0L))
    {
        negative = true;
        quadrant = (quadrant >= 3.0L) ? 0.0L
                                      : (quadrant + 1.0L);

        for (c = 1; c <= 20; ++c)
        {
            columns[c] = (radix - 1.0L) - columns[c];
        }

        columns[20] += 1.0L;

        for (c = 20; ( (c >= 2) && (columns[c] >= radix) ); --c)
        {
            columns[c]     -= radix;
            columns[c - 1] += 1.0L;
        }
    }

    // the fraction's first nonzero digit
    while ( (first < 20) &&
            (columns[first] == 0.0L) )
    {
        ++first;
    }

    {
        // the fraction as hi + lo, from that digit to past twice long
        // double's precision (fast two-sums: each digit is below the sum)
        const int   last  = ((first + ((2 * LDBL_MANT_DIG) / 24) + 2) < 20)
                                ? (first + ((2 * LDBL_MANT_DIG) / 24) + 2)
                                : 20;
        long double scale = d_internal_math_ld_pow2(-24 * first);
        long double hi    = 0.0L;
        long double lo    = 0.0L;

        for (c = first; c <= last; ++c)
        {
            const long double term = columns[c] * scale;
            const long double sum  = hi + term;

            lo    += term - (sum - hi);
            hi     = sum;
            scale /= radix;
        }

        {
            // renormalized, then times pi/2: f_hi pio2_hi exactly, the rest
            // rounded below it
            const long double f_hi = hi + lo;
            const long double f_lo = lo - (f_hi - hi);
            const struct dj_internal_math_ld_pair head =
                d_internal_math_ld_product_exact(f_hi, pio2_hi);
            const long double rest = head.low +
                                     ((f_hi * pio2_lo) + (f_lo * pio2_hi));

            result.r    = head.high + rest;
            result.r_lo = rest - (result.r - head.high);
        }
    }

    // the signs: the fraction's, then y's, -y being (-n) pi/2 - r
    if (negative != (_y < 0.0L))
    {
        result.r    = -result.r;
        result.r_lo = -result.r_lo;
    }

    if ( (_y < 0.0L) &&
         (quadrant != 0.0L) )
    {
        quadrant = 4.0L - quadrant;
    }

    result.quadrant = (int)quadrant;

    return result;
}

// d_internal_cplx_@S@_cis_ln2@KS@
//   cos(_y ln 2) and sin(_y ln 2) as pairs (angle's), _y finite: below 1 in
// magnitude, y ln 2 itself as a pair, ln 2 held as the exponential holds it,
// within pi/4; past it, reduced by pi/2 exactly (ln2_reduced).
@KQ@ struct dj_internal_math_ld_sin_cos
d_internal_cplx_@S@_cis_ln2@KS@(
    long double _y
)
{
    // ln 2 = ln2_hi + ln2_lo, ln2_hi cut to 32 bits: exact as written
    const long double ln2_hi = 0.69314718036912381649017333984375L;
    const long double ln2_lo = 1.908214929270587816144265680755001344e-10L;

    if ( (_y < 1.0L) &&
         (_y > -1.0L) )
    {
        const struct dj_internal_math_ld_pair p =
            d_internal_math_ld_product_exact(_y, ln2_hi);
        const struct dj_internal_math_ld_pair t =
            d_internal_math_ld_two_sum(p.high, p.low + (_y * ln2_lo));

        return d_internal_cplx_@S@_angle@KS@(t.high, t.low, 0);
    }

    {
        const struct dj_internal_math_ld_reduced r =
            d_internal_cplx_@S@_ln2_reduced@KS@(_y);

        return d_internal_cplx_@S@_angle@KS@(r.r, r.r_lo, r.quadrant);
    }
}

// d_internal_cplx_@S@_exp2@KS@
//   2^_z = 2^x (cos(y ln 2) + i sin(y ln 2)): 2^x as (m + m_lo) 2^k, the
// base-2 exponential's pair (power.h's section 1.6), times each of the
// angle's pairs (cis_ln2), each part rounded once by its scaling (pow's
// scale_pair), into the subnormals and past the largest finite value; a y
// below 2^-100 raised by 2^200 first, sin(y ln 2) being y ln 2 there to
// 2^-200 of it and no subnormal y ln 2 to lose its bits. Past 2^16600 and
// below 2^-16600, infinities and zeros in the angle's directions; where a
// part is infinite or NaN, cexp's values (G.6.3.1) with y ln 2 for y. y is
// not 0: the real axis is each family's real function's (section 4).
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_exp2@KS@(
    struct dj_cplx_@S@ _z
)
{
    const @T@ x   = _z.part[0];
    const @T@ y   = _z.part[1];
    const @T@ nan = d_internal_math_@S@_nan();
    const @T@ inf = d_internal_math_@S@_infinity();

    // x a NaN, y not 0: NaNs
    if (x != x)
    {
        return d_internal_cplx_@S@_make(x, x);
    }

    // y infinite or NaN: -inf gives +0 + i0, +inf +inf + iNaN, a finite x
    // NaNs
    if ( (y != y) ||
         (d_internal_cplx_@S@_is_inf(y)) )
    {
        if (d_internal_cplx_@S@_is_inf(x))
        {
            if (x < (@T@)0)
            {
                return d_internal_cplx_@S@_make((@T@)0, (@T@)0);
            }

            return d_internal_cplx_@S@_make(x, nan);
        }

        return d_internal_cplx_@S@_make(nan, nan);
    }

    {
        const struct dj_internal_math_ld_sin_cos a =
            d_internal_cplx_@S@_cis_ln2@KS@(y);

        // x infinite, y finite and not 0: +0 or +inf in the angle's
        // directions; and past either end of every family's range
        if ( (d_internal_cplx_@S@_is_inf(x)) ||
             (x >= (@T@)16600) ||
             (x <= (@T@)-16600) )
        {
            const bool up = (x > (@T@)0);

            return d_internal_cplx_@S@_make(
                (a.cosine.high < 0.0L) ? (up ? -inf : (@T@)-0.0L)
                                       : (up ? inf : (@T@)0),
                (a.sine.high < 0.0L) ? (up ? -inf : (@T@)-0.0L)
                                     : (up ? inf : (@T@)0));
        }

        {
            const struct dj_internal_math_ld_exp_split e =
                d_internal_math_ld_exp2_pair_split_of(
                    d_internal_math_ld_pair_of(x, 0.0L));
            const struct dj_internal_math_ld_pair m =
                d_internal_math_ld_pair_of(e.y, e.y_lo);
            const @T@ tiny = d_internal_math_@S@_pow2(-100);

            // a tiny y: sin(y ln 2) is y ln 2, cos 1, y raised by 2^200
            if ( (y < tiny) &&
                 (y > -tiny) )
            {
                // ln 2 = ln2_hi + ln2_lo, ln2_hi cut to 32 bits: exact
                const long double ln2_hi = 0.69314718036912381649017333984375L;
                const long double ln2_lo =
                    1.908214929270587816144265680755001344e-10L;
                const long double ys = y * d_internal_math_ld_pow2(200);
                const struct dj_internal_math_ld_pair p =
                    d_internal_math_ld_product_exact(ys, ln2_hi);
                const struct dj_internal_math_ld_pair t =
                    d_internal_math_ld_two_sum(p.high,
                                               p.low + (ys * ln2_lo));

                return d_internal_cplx_@S@_make(
                    d_internal_cplx_@S@_scale_pair@KS@(m, e.k),
                    d_internal_cplx_@S@_scale_pair@KS@(
                        d_internal_math_ld_pair_product(m, t), e.k - 200));
            }

            return d_internal_cplx_@S@_make(
                d_internal_cplx_@S@_scale_pair@KS@(
                    d_internal_math_ld_pair_product(m, a.cosine), e.k),
                d_internal_cplx_@S@_scale_pair@KS@(
                    d_internal_math_ld_pair_product(m, a.sine), e.k));
        }
    }
}

// d_internal_cplx_@S@_expm1_pair@KS@
//   e^_r - 1 as a pair, |_r| <= ln 2 / 2 (a last place past it does no
// harm): the exponential's series in pairs, as power.h's section 1.6 has it
// but to r^8 -- r + r^2 (1/2 + r (1/3! + r (1/4! + ... + r (1/8! + r g)))),
// the coefficients' heads of 32 bits, exact as written, and g = (1/9!)(1 +
// r/10 (1 + r/11 (...))) in long double -- without its leading 1, so that the
// pair keeps its precision relative to e^r - 1 itself, some 2^-(P+28) of it.
@KQ@ struct dj_internal_math_ld_pair
d_internal_cplx_@S@_expm1_pair@KS@(
    long double _r
)
{
    // 1/3! ... 1/8!: heads of 32 bits, exact as written
    const struct dj_internal_math_ld_pair c[6] = {
        { 0.166666666627861559391021728515625L,
          3.880510727564493815104166666666666666667e-11L },
        { 0.04166666665696538984775543212890625L,
          9.701276818911234537760416666666666666667e-12L },
        { 0.00833333333139307796955108642578125L,
          1.940255363782246907552083333333333333333e-12L },
        { 0.0013888888888686778955161571502685546875L,
          2.021099337273173862033420138888888888889e-14L },
        { 0.0001984126984098111279308795928955078125L,
          2.887284767533105517190600198412698412698e-15L },
        { 0.0000248015873012263909913599491119384765625L,
          3.609105959416381896488250248015873015873e-16L } };
    const struct dj_internal_math_ld_pair r =
        d_internal_math_ld_pair_of(_r, 0.0L);
    // r^2 as a pair: r r exactly
    const struct dj_internal_math_ld_pair r2 =
        d_internal_math_ld_product_exact(_r, _r);
    struct dj_internal_math_ld_pair series = { 0.0L, 0.0L };
    long double                     g      = 1.0L;
    int                             n      = 0;

    // 9! g = 1 + r/10 (1 + r/11 (...)), from its 28th term in
    for (n = 28; n >= 10; --n)
    {
        g = 1.0L + ((_r / (long double)n) * g);
    }

    // 1/8! + r g, then 1/7! + r (...) down to 1/3!, in pairs
    series = d_internal_math_ld_pair_sum(
        c[5], d_internal_math_ld_pair_product(
                  r, d_internal_math_ld_pair_of(g / 362880.0L, 0.0L)));

    for (n = 4; n >= 0; --n)
    {
        series = d_internal_math_ld_pair_sum(
            c[n], d_internal_math_ld_pair_product(r, series));
    }

    // 1/2 + r (...), then r + r^2 (...)
    series = d_internal_math_ld_pair_sum(
        d_internal_math_ld_pair_of(0.5L, 0.0L),
        d_internal_math_ld_pair_product(r, series));

    return d_internal_math_ld_pair_sum(
        r, d_internal_math_ld_pair_product(r2, series));
}

// d_internal_cplx_@S@_exp_split@KS@
//   e^_x as (y + y_lo) 2^k, for |_x| below 2^14, to some 2^-(P+28) of it:
// k the integer nearest x / ln 2, r = x - k ln 2 with ln 2 in three parts --
// two heads of 32 bits and the rest, so that k times either head is exact
// and r a pair to 2^-(2P - 15) of 1 -- and e^r = 1 + (e^r_hi - 1)(1 + r_lo) +
// r_lo by the series above: power.h's exponential of a pair, whose ln 2 has
// two parts and whose series rounds from r^6 on in long double, is good to
// some 2^-(P+18), which a difference near e^x cos y = 1 would lose.
@KQ@ struct dj_internal_math_ld_exp_split
d_internal_cplx_@S@_exp_split@KS@(
    long double _x
)
{
    const long double log2e = 1.442695040888963407359924681001892137L;
    // ln 2 = l1 + l2 + l3, l1 and l2 of 32 bits each: exact as written
    const long double l1 = 0.69314718036912381649017333984375L;
    const long double l2 =
        0.0000000001908214928843070257169500791860627941787242889404296875L;
    const long double l3 = 4.275175589747648888943734018153096518025e-20L;
    const long double t  = _x * log2e;
    const int         k  = (int)((t < 0.0L) ? (t - 0.5L) : (t + 0.5L));
    const long double kk = (long double)k;
    // x - k l1 exact (Sterbenz's lemma), k l2 exact, k l3 by Dekker's
    // product, every error kept
    const struct dj_internal_math_ld_pair a =
        d_internal_math_ld_two_sum(_x - (kk * l1), -(kk * l2));
    const struct dj_internal_math_ld_pair b =
        d_internal_math_ld_product_exact(kk, l3);
    const struct dj_internal_math_ld_pair r =
        d_internal_math_ld_pair_sum(
            a, d_internal_math_ld_pair_of(-b.high, -b.low));
    const struct dj_internal_math_ld_pair e =
        d_internal_cplx_@S@_expm1_pair@KS@(r.high);
    // 1 + e (1 + r_lo) + r_lo: r_lo below 2^-60, its square past the pair
    const struct dj_internal_math_ld_pair tail =
        d_internal_math_ld_pair_sum(
            e, d_internal_math_ld_pair_of(r.low * (1.0L + e.high), 0.0L));
    const struct dj_internal_math_ld_pair one =
        d_internal_math_ld_two_sum(1.0L, tail.high);
    const struct dj_internal_math_ld_pair y =
        d_internal_math_ld_two_sum(one.high, one.low + tail.low);
    struct dj_internal_math_ld_exp_split result = { 1.0L, 0.0L, 0 };

    result.y    = y.high;
    result.y_lo = y.low;
    result.k    = k;

    return result;
}

// d_internal_cplx_@S@_expm1@KS@
//   e^_z - 1, as pairs throughout and rounded once. Within ln 2 / 2 of the
// imaginary axis, Re = (e^x - 1) - 2 e^x sin^2(y/2) and Im = 2 e^x sin(y/2)
// cos(y/2), e^x - 1 by the series above and y/2's sine and cosine pairs, so
// that neither term rounds before their difference, which loses to the
// cancellation near the curve e^x cos y = 1 through 0 only the pairs' own
// precision, some 2^-(P+28) of the terms; on the imaginary axis Re is -2
// sin^2(y/2), its zero's sign kept where it underflows; a y below 2^-8300,
// whose half a subnormal y would round, takes Im = e^x y and Re = e^x - 1,
// the square past every value. Elsewhere Re = e^x cos y - 1 and Im = e^x
// sin y, e^x as (m + m_lo) 2^k (exp_split) and the 1 subtracted as 2^-k
// before the scaling, so that the difference is formed to the pairs'
// precision too; past k = 16000, the 1 lies below every value, and below k
// = -(2P + 8) the value below half of 1's last place; past e^16000 and
// below e^-16000, infinities in the angle's directions and -1. Where a part
// is infinite or NaN, cexp's value (G.6.3.1) less 1.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_expm1@KS@(
    struct dj_cplx_@S@ _z
)
{
    const @T@ x   = _z.part[0];
    const @T@ y   = _z.part[1];
    const @T@ ax  = d_internal_cplx_@S@_abs(x);
    const @T@ ay  = d_internal_cplx_@S@_abs(y);
    const @T@ inf = d_internal_math_@S@_infinity();

    // an infinite or NaN part: cexp's value, less 1
    if (!d_internal_cplx_@S@_finite(_z))
    {
        const struct dj_cplx_@S@ e = d_internal_cplx_@S@_exp@KS@(_z);

        return d_internal_cplx_@S@_make(e.part[0] - (@T@)1, e.part[1]);
    }

    // near the imaginary axis: the half angle's way
    if (ax <= (@T@)0.346573590279972654708616060729088284L)
    {
        const struct dj_internal_math_ld_pair e1 =
            d_internal_cplx_@S@_expm1_pair@KS@(x);
        const struct dj_internal_math_ld_pair one =
            d_internal_math_ld_two_sum(1.0L, e1.high);
        const struct dj_internal_math_ld_pair ex =
            d_internal_math_ld_two_sum(one.high, one.low + e1.low);

        // a tiny y: e^x y, and e^x - 1 with y^2 past it
        if (ay < d_internal_math_@S@_pow2(-8300))
        {
            return d_internal_cplx_@S@_make(
                (x == (@T@)0) ? (@T@)-0.0L : e1.high,
                d_internal_math_ld_pair_product(
                    ex, d_internal_math_ld_pair_of(y, 0.0L)).high);
        }

        {
            const struct dj_internal_math_ld_sin_cos h =
                d_internal_cplx_@S@_angle@KS@(y / (@T@)2, 0.0L, 0);
            const struct dj_internal_math_ld_pair es =
                d_internal_math_ld_pair_product(ex, h.sine);
            // 2 e^x sin^2(y/2) and 2 e^x sin(y/2) cos(y/2), doubled exactly
            const struct dj_internal_math_ld_pair t =
                d_internal_math_ld_pair_product(es, h.sine);
            const struct dj_internal_math_ld_pair s =
                d_internal_math_ld_pair_product(es, h.cosine);
            const struct dj_internal_math_ld_pair re =
                d_internal_math_ld_pair_sum(
                    e1, d_internal_math_ld_pair_of(-2.0L * t.high,
                                                   -2.0L * t.low));

            // x = 0: e^iy - 1, its real part -2 sin^2(y/2), a zero's sign
            // kept where it underflows
            return d_internal_cplx_@S@_make(
                (x == (@T@)0) ? (-2.0L * t.high) : re.high,
                2.0L * s.high);
        }
    }

    {
        const struct dj_internal_math_ld_sin_cos a =
            d_internal_cplx_@S@_angle@KS@(y, 0.0L, 0);

        // far past the range: infinities in the angle's directions, or -1
        if (x >= (@T@)16000)
        {
            return d_internal_cplx_@S@_make(
                (a.cosine.high < 0.0L) ? -inf : inf,
                (a.sine.high < 0.0L) ? -inf : inf);
        }

        if (x <= (@T@)-16000)
        {
            return d_internal_cplx_@S@_make(
                -1.0L, (a.sine.high < 0.0L) ? -0.0L : 0.0L);
        }

        {
            const struct dj_internal_math_ld_exp_split e =
                d_internal_cplx_@S@_exp_split@KS@(x);
            const struct dj_internal_math_ld_pair m =
                d_internal_math_ld_pair_of(e.y, e.y_lo);
            const struct dj_internal_math_ld_pair mc =
                d_internal_math_ld_pair_product(m, a.cosine);
            const @T@ im = d_internal_cplx_@S@_scale_pair@KS@(
                d_internal_math_ld_pair_product(m, a.sine), e.k);

            // e^x cos y below half of 1's last place
            if (e.k < -((2 * LDBL_MANT_DIG) + 8))
            {
                return d_internal_cplx_@S@_make((@T@)-1, im);
            }

            // the 1 below every value
            if (e.k > 16000)
            {
                return d_internal_cplx_@S@_make(
                    d_internal_cplx_@S@_scale_pair@KS@(mc, e.k), im);
            }

            return d_internal_cplx_@S@_make(
                d_internal_cplx_@S@_scale_pair@KS@(
                    d_internal_math_ld_pair_sum(
                        mc,
                        d_internal_math_ld_pair_of(
                            -d_internal_math_ld_pow2(-e.k), 0.0L)),
                    e.k),
                im);
        }
    }
}
"""


# 2 ln 2 / pi to 704 digits of 24 bits, exp2's reduction's table: y ln 2 is
# reduced by pi/2 as elementary.h reduces x, with 2 ln 2 / pi's digits in
# 2/pi's place, computed here as 2/pi's are
def ln2_turns_bits(count):
    """floor(2 ln 2 / pi 2^count), from pi by Machin's formula and ln 2 =
    2 atanh(1/3), each series summed in integers with 64 guard bits, to count
    + 64 bits; the floor is taken at both ends of their errors, which must
    agree."""
    bits = count + 64
    guard = 64
    one = 1 << (bits + guard)

    def atan_inv(n):
        total = term = one // n
        k, sign = 1, 1
        while term:
            term //= n * n
            k += 2
            sign = -sign
            total += sign * (term // k)
        return total

    def atanh_inv(n):
        total = term = one // n
        k = 1
        while term:
            term //= n * n
            k += 2
            total += term // k
        return total

    pi = (16 * atan_inv(5) - 4 * atan_inv(239)) >> guard
    ln2 = (2 * atanh_inv(3)) >> guard
    low = (2 * (ln2 - 2) << count) // (pi + 2)
    high = (2 * (ln2 + 2) << count) // (pi - 2)
    assert low == high, "2 ln 2 / pi: a run of equal bits at the cut"
    return low


def ln2_turns_literal():
    """the table as a C string literal, three bytes a digit, 16 bytes a
    line, highest first."""
    data = ln2_turns_bits(24 * TWO_OVER_PI_DIGITS).to_bytes(
        3 * TWO_OVER_PI_DIGITS, "big")
    rows = []
    for i in range(0, len(data), 16):
        rows.append('        "' +
                    "".join("\\x%02X" % b for b in data[i:i + 16]) + '"')
    return "\n".join(rows)


# the real functions the pure kernels call, by their arity
CPLX_REAL_BINARY = ("atan2", "hypot", "pow")

CPLX_REAL_SHARED = r"""
// The real functions the kernels call, each once: a wrapper of the core's
// _c function, shared, so that a translation unit compiles each real kernel
// once rather than into every complex kernel that calls it (math_common.h's
// D_MATH_FN_KERNEL). The value is the core's own, bit for bit.

"""


def cplx_real_wrappers(text):
    """text with every call of a long double _c real function routed to a
    shared wrapper, and the wrappers, in the order of their names."""
    import re
    names = sorted(set(re.findall(r"\bd_math_ld_([a-z0-9]+)_c\(", text)))
    text = re.sub(r"\bd_math_ld_([a-z0-9]+)_c\(",
                  r"d_internal_cplx_ld_real_\1_c(", text)
    out = [CPLX_REAL_SHARED]
    for name in names:
        out.append("// d_internal_cplx_ld_real_%s_c\n" % name)
        out.append("//   d_math_ld_%s_c, shared.\n" % name)
        out.append("D_MATH_FN_KERNEL long double\n")
        out.append("d_internal_cplx_ld_real_%s_c(\n" % name)
        if name in CPLX_REAL_BINARY:
            out.append("    long double _a,\n    long double _b\n)\n{\n")
            out.append("    return d_math_ld_%s_c(_a, _b);\n}\n\n" % name)
        else:
            out.append("    long double _x\n)\n{\n")
            out.append("    return d_math_ld_%s_c(_x);\n}\n\n" % name)
    return "".join(out), text


# the families a target may evaluate wider than their type, and the test of
# it (math_common.h's section 4)
CPLX_WIDE = {"f": "D_MATH_FLT_EVAL_WIDE", "d": "D_MATH_DBL_EVAL_WIDE"}


def cplx_wide(text, suffix, keep):
    """text with each block "#if @WIDE@ ... [#else  // @WIDE@ ...] #endif  //
    @WIDE@" made the family's test of its width where keep and the family is
    float or double, which a 32-bit x87 evaluates in long double; for long
    double, and for the pure kernels, which are long double's, the block's
    first part left out and its #else part kept."""
    macro = CPLX_WIDE.get(suffix) if keep else None
    out = []
    state = None
    for line in text.split("\n"):
        if line == "#if @WIDE@":
            state = "wide"
            if macro:
                out.append("#if " + macro)
            continue
        if line == "#else  // @WIDE@":
            state = "else"
            if macro:
                out.append("#else  // " + macro)
            continue
        if line == "#endif  // @WIDE@":
            state = None
            if macro:
                out.append("#endif  // " + macro)
            continue
        if (state == "wide") and (not macro):
            continue
        out.append(line)
    return "\n".join(out)


def cplx_kernels(suffix, ctype, prefix, prose, section, sub, title, pure):
    """the kernel text in one working type, its names suffixed _c and on the
    _c real functions where pure, plain on the C library's otherwise. The
    pure kernels are D_MATH_FN_KERNEL, shared by the three families' _c
    forms, and call the real functions through shared wrappers."""
    body = CPLX_KERNELS + CPLX_INVERSE + CPLX_COMPOSED
    body = body.replace("@POW_GENERAL@", CPLX_POW_GENERAL_PURE if pure
                        else CPLX_POW_GENERAL_PLAIN)
    body = body.replace("@POW_INT@", CPLX_POW_INT_PURE if pure
                        else CPLX_POW_INT_PLAIN)
    if pure:
        body += CPLX_FIVE_PURE.replace("@LN2_TURNS@", ln2_turns_literal())
    body = body.replace("@TITLE@", title)
    body = body.replace("@SECTION@", str(section)).replace("@SUB@", str(sub))
    body = body.replace("@KS@", "_c" if pure else "")
    body = body.replace("@R@", "_c" if pure else "")
    body = body.replace("@KQ@", "D_MATH_FN_KERNEL" if pure else "D_MATH_FN_RT")
    text = cplx_wide(instantiate(body, suffix, ctype, prefix, prose), suffix,
                     not pure)
    if pure:
        head, rest = text.split("//" + "-" * 78 + "\n", 1)
        wrappers, rest = cplx_real_wrappers(rest)
        text = head + "//" + "-" * 78 + "\n" + wrappers + rest
    return text


def cplx():
    lines = [
        "Complex numbers: the C core's complex type and C99's complex",
        "functions, for float, double and long double.",
        "  GENERATED by scripts/gen_c_math_float.py from one text for the three",
        "families: edit the generator, not this file.",
        "",
        "  A complex number is struct dj_cplx_f, _d or _ld, one member, part[2],",
        "the real part first: the layout of T[2], which C99's _Complex T and",
        "C++'s std::complex<T> share, so a value converts to either by a copy",
        "(S267). No _Complex is needed, as C11 makes it optional, MSVC has none",
        "and C++ no such type; where the C compiler has it, section 5 converts.",
        "The arithmetic is pure and comes once: a product's parts by exact",
        "products, a quotient by the conjugate over the squared modulus with",
        "its residual's step, the operands scaled by powers of 2 so that",
        "nothing overflows on the way, and Annex G's infinities where an",
        "operand is infinite; an operation with a real operand takes it as",
        "itself (G.5). Where a target evaluates float and double wider than",
        "their types, as a 32-bit x87 does, their exact steps are taken in long",
        "double, which it rounds in its own type.",
        "",
        "  C99's 22 functions: real, imag, conj and proj, exact, once, with the",
        "arithmetic; the other 18 twice, as every real function in the core: the",
        "_c form a constant expression from C++14, one long double kernel of",
        "pure arithmetic on the core's _c real functions, rounded once to the",
        "family, each kernel shared by the three families (D_MATH_FN_KERNEL);",
        "the plain form the same algorithms in the family's own type on the C",
        "library's real functions, fast. The algorithms are the published ones",
        "(S268): Kahan's for the square root, the tangents and the cuts; Hull,",
        "Fairgrieve and Tang's for the inverse functions, as FreeBSD's catrig.c",
        "writes them; log near the unit circle by log1p of the exact |z|^2 - 1;",
        "pow's exp(w log z) with w log z carried as pairs, in both forms. The",
        "branch cuts and the values at the zeros, infinities and NaNs are C99's",
        "Annex G's (G.6), cacosh's and ctanh's as C17 has them (its DR 471); no",
        "floating-point exception is promised, as a constant expression cannot",
        "raise one.",
        "",
        "  And the five C99 has only for real arguments, of a complex number:",
        "exp2, log2, log10, log1p and expm1 (S342). Each is one long double",
        "kernel of pure arithmetic, its parts carried as pairs to past twice the",
        "value's precision and rounded once: log |z| and arg z as power.h's and",
        "tangent.h's pairs, times 1/ln b as a pair; log |1 + z| of 1 + w, w = 2x",
        "+ x^2 + y^2 summed exactly, and 1 + z's angle with 1 + x as a pair;",
        "e^x - 1 and e^x by series in pairs, and sin and cos by their own, so",
        "that expm1's real part, (e^x - 1) - 2 e^x sin^2(y/2), cancels near the",
        "curve e^x cos y = 1 only to some 2^-(P+28) of its terms; and y ln 2",
        "reduced by pi/2 exactly for every y, by 2 ln 2 / pi's digits as",
        "elementary.h reduces x by 2/pi's (S344). Both forms are the kernel",
        "rounded to the family, as standard C's library has none of them to be",
        "faster with, C99 reserving their names and glibc and the UCRT giving",
        "clog10 alone (S266); on the real axis, where the value is real, each",
        "form is the family's real function in that form, and y's zero (S343).",
        "Where a part is infinite or NaN, the value of the function each is a",
        "form of: clog's for log2 and log10, the angle over ln b, and for log1p",
        "at 1 + z; cexp's for exp2, y ln 2 for y, and for expm1, less 1.",
        "",
        "  The bounds, in steps (float_order.h) of each part from the part",
        "correctly rounded, as tests/djinterp/c/modules/math/t_cplx.c checks them at",
        "the points of math_tests_cplx_refs.h and, for the five,",
        "math_tests_cplx_five_refs.h, computed by mpmath: points exact in",
        "float, each family's at the ends of its range, and double's and long",
        "double's at their full width. The product, the quotient, the",
        "modulus and the argument within 1, the product and the quotient",
        "measured 0; the _c forms within 1 for float and double, where long",
        "double carries 11 bits past double's, and for long double within 4 for",
        "tan and tanh, 3 for exp, sin, cos, acos, cosh and asinh, and 2 for the",
        "rest, pow's among them (each measured one less, or better, but asin's",
        "and acosh's, measured 2); the plain forms within 4, 3 for log and",
        "sqrt, 2 for the modulus and the argument and 5 for tan and tanh (6 for",
        "long double's), the C library's own errors among them; the plain",
        "pow's as the _c form's, its general case being the _c kernel's",
        "(S266): C99's exp(w log z) in the family's type would carry |w log z|",
        "times log's error. The five within 1 in both forms and every family,",
        "measured 0: the kernel rounded to long double and then to the family,",
        "a float or double part a step off where the long double one lies on",
        "the family's midpoint. A whole power, z^n for n from -2^30 to 2^30, is",
        "repeated squaring carried in long double's pairs, each part rounded",
        "once, in both forms (S370): exact where the power is exact in twice",
        "long double's precision; each part within a step of the part correctly",
        "rounded where it is at least |n| 2^(4 - P) of the larger part, P long",
        "double's precision, as t_cplx.c checks it at the points of",
        "math_tests_cplx_pow_refs.h, where a part cancels (measured 0 in every",
        "family); and a part below that, which may cancel past what the pairs",
        "carry, within |n| 2^(3 - 2P) of the larger part (measured 0.031 of it).",
        "The plain form's whole power is the _c form's, bit for bit.",
    ]
    text = banner("cplx.h", lines, "2026.10.27", "2026.11.07")
    text += "\n" + CPLX_HEAD
    structs = []
    for n, (suffix, ctype, prefix, prose) in enumerate(FAMILIES, 1):
        structs.append(instantiate(CPLX_STRUCT.replace("@N@", str(n)),
                                   suffix, ctype, prefix, prose))
    text += CPLX_TYPES.replace("@STRUCTS@", "\n".join(structs))
    text += "\n\n//" + "=" * 78 + "\n// 2.  THE ARITHMETIC\n//" + "=" * 78 + "\n"
    text += ("// Pure, once per family: the parts as given, a product's and a quotient's\n"
             "// parts by exact products, and Annex G's infinities (G.5).\n\n")
    for n, (suffix, ctype, prefix, prose) in enumerate(FAMILIES, 1):
        body = (CPLX_ARITH + CPLX_TURNS).replace("@SECTION@", "2").replace("@SUB@", str(n))
        text += cplx_wide(instantiate(body, suffix, ctype, prefix, prose),
                          suffix, True)
    text += "\n\n//" + "=" * 78 + "\n// 3.  THE KERNELS\n//" + "=" * 78 + "\n"
    text += ("// C99's functions, once in pure arithmetic in long double, the _c forms'\n"
             "// kernel, and once in each family's type on the C library's real\n"
             "// functions, the plain forms': one text, the same algorithms.\n\n")
    ld = FAMILIES[2]
    text += cplx_kernels(*ld, 3, 1, "Pure arithmetic, in long double", True)
    for n, fam in enumerate(FAMILIES, 2):
        text += cplx_kernels(*fam, 3, n, "On the C library, in " + fam[3], False)
    text += "\n\n//" + "=" * 78 + "\n// 4.  THE FUNCTIONS\n//" + "=" * 78 + "\n"
    text += "// Each family's C99 functions, in both forms.\n\n"
    for n, (suffix, ctype, prefix, prose) in enumerate(FAMILIES, 1):
        body = CPLX_FUNCTIONS_HEAD.replace("@SECTION@", "4").replace("@SUB@", str(n))
        item = 2
        for f in CPLX_UNARIES:
            doc = CPLX_UNARY_DOC[f][0]
            body += CPLX_UNARY.replace("@F@", f).replace("@DOC@", doc)
        body += CPLX_POW
        body += ("\n// The five C99 has only for real arguments, of a complex "
                 "one: exp2, log2,\n// log10, log1p and expm1.\n")
        for f in CPLX_FIVE_UNARIES:
            doc, where = CPLX_FIVE_DOC[f]
            if where == "positive":
                axis = ("( (_z.part[1] == (@T@)0) &&\n"
                        "         (_z.part[0] > (@T@)0) )")
                where_text = ", x positive"
            elif where == "past -1":
                axis = ("( (_z.part[1] == (@T@)0) &&\n"
                        "         (_z.part[0] > (@T@)-1) )")
                where_text = ", x past -1"
            else:
                axis = "(_z.part[1] == (@T@)0)"
                where_text = ""
            body += (CPLX_FIVE.replace("@F@", f).replace("@DOC@", doc)
                     .replace("@AXIS@", axis).replace("@WHERE@", where_text))
        text += instantiate(body, suffix, ctype, prefix, prose)
    text += "\n\n//" + "=" * 78 + "\n// 5.  C99'S _Complex (C ONLY)\n//" + "=" * 78 + "\n"
    text += ("// Where the C compiler has C99's complex types -- C, from C99, but for\n"
             "// __STDC_NO_COMPLEX__ and MSVC -- each family's converts to and from its\n"
             "// struct by a copy. <complex.h> is not included: its I and complex are\n"
             "// the user's to take, and _Complex is a keyword.\n\n")
    text += ("#if ( (!defined(__cplusplus))          &&                                     \\\n"
             "      (!defined(__STDC_NO_COMPLEX__)) &&                                     \\\n"
             "      (!defined(D_ENV_COMPILER_MSVC)) )\n")
    for n, (suffix, ctype, prefix, prose) in enumerate(FAMILIES, 1):
        body = CPLX_COMPLEX.replace("@SECTION@", "5").replace("@SUB@", str(n))
        text += instantiate(body, suffix, ctype, prefix, prose)
    text += "\n#endif  // C99's _Complex\n"
    text += overload_section("6.  ONE NAME FOR EVERY FAMILY (C++)", CPLX_OVERLOADS)
    text += CPLX_TAIL
    return text


# -----------------------------------------------------------------------------
# driver
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# cmat.h
# -----------------------------------------------------------------------------
# Complex matrices (S269), on cplx.h's numbers: LU, determinant, inverse and
# solution, once per family; a Hermitian matrix's eigensystem by Jacobi's
# method; a general one's by LAPACK's zgeev path; what takes a root twice.

CMAT_FAMILY_HEAD = r"""
//==============================================================================
// @SECTION@.  THE @UPPER@ FAMILY (_@S@)
//==============================================================================
// Matrices are a caller's row-major arrays of cplx.h's complex numbers:
// element (i, j) of an r-by-c matrix at [i c + j], as LAPACK lays out its
// complex matrices but by rows. The real helpers -- the scaled norms and
// hypot, the scaling by powers of two -- are decomp.h's.

"""

CMAT_LU = r"""
// @SECTION@.1    LU, determinant, inverse and solution
//------------------------------------------------------------------------------
// Gaussian elimination with partial pivoting, as LAPACK's zgetrf and
// zgesv: each column's pivot its entry of largest |re| + |im| at or below
// the diagonal (the first among equals), as LAPACK's izamax chooses it; every
// product and quotient cplx.h's, each part correctly rounded or within a
// step of it. Needing no root, each comes once, a constant expression from
// C++14.

// d_internal_cmat_@S@_mul, d_internal_cmat_@S@_div
//   cplx.h's product and quotient, compiled once in a translation unit and
// called, where the kernels below would otherwise each take a copy of them
// at every call.
D_MATH_FN_KERNEL struct dj_cplx_@S@
d_internal_cmat_@S@_mul(
    struct dj_cplx_@S@ _z,
    struct dj_cplx_@S@ _w
)
{
    return d_cplx_@S@_mul(_z,
                          _w);
}

D_MATH_FN_KERNEL struct dj_cplx_@S@
d_internal_cmat_@S@_div(
    struct dj_cplx_@S@ _z,
    struct dj_cplx_@S@ _w
)
{
    return d_cplx_@S@_div(_z,
                          _w);
}

// d_internal_cmat_@S@_abs1
//   |re| + |im| of _z: the magnitude LAPACK pivots by, needing no root.
D_MATH_FN @T@
d_internal_cmat_@S@_abs1(
    struct dj_cplx_@S@ _z
)
{
    const @T@ a = d_internal_math_@S@_abs(_z.part[0]);
    const @T@ b = d_internal_math_@S@_abs(_z.part[1]);

    return a + b;
}

// d_internal_cmat_@S@_is_zero
//   whether both parts of _z are zero.
D_MATH_FN bool
d_internal_cmat_@S@_is_zero(
    struct dj_cplx_@S@ _z
)
{
    return ( (_z.part[0] == (@T@)0) &&
             (_z.part[1] == (@T@)0) );
}

// d_internal_cmat_@S@_less_product
//   _a - _b _c, the product cplx.h's, then the difference part by part.
D_MATH_FN_KERNEL struct dj_cplx_@S@
d_internal_cmat_@S@_less_product(
    struct dj_cplx_@S@ _a,
    struct dj_cplx_@S@ _b,
    struct dj_cplx_@S@ _c
)
{
    return d_cplx_@S@_sub(_a,
                          d_internal_cmat_@S@_mul(_b,
                                         _c));
}

// d_cmat_@S@_lu
//   factors the n-by-n _lu in place into P A = L U -- L's multipliers below
// the diagonal, U on and above -- choosing each column's pivot of largest
// |re| + |im| (the first, among equals). _perm, unless NULL, receives the row
// permutation, and *_sign its parity (+1 or -1). Returns whether some pivot
// column was zero, the matrix singular; that column is then skipped, U's
// diagonal zero there.
D_MATH_FN_KERNEL bool
d_cmat_@S@_lu(
    struct dj_cplx_@S@* _lu,
    size_t              _n,
    size_t*             _perm,
    @T@*                _sign
)
{
    bool   singular = false;
    size_t i        = 0u;
    size_t col      = 0u;

    *_sign = (@T@)1;

    if (_perm != NULL)
    {
        for (i = 0u; i < _n; ++i)
        {
            _perm[i] = i;
        }
    }

    for (col = 0u; col < _n; ++col)
    {
        size_t pivot = col;
        @T@    maxv  = d_internal_cmat_@S@_abs1(_lu[(col * _n) + col]);
        size_t r     = 0u;

        // the largest |re| + |im| in the column, at or below the diagonal
        for (r = col + 1u; r < _n; ++r)
        {
            const @T@ v = d_internal_cmat_@S@_abs1(_lu[(r * _n) + col]);

            if (v > maxv)
            {
                maxv  = v;
                pivot = r;
            }
        }

        // a zero column: singular, and nothing to eliminate
        if (d_internal_cmat_@S@_is_zero(_lu[(pivot * _n) + col]))
        {
            singular = true;

            continue;
        }

        // the pivot row up
        if (pivot != col)
        {
            size_t c = 0u;

            for (c = 0u; c < _n; ++c)
            {
                const struct dj_cplx_@S@ tmp = _lu[(col * _n) + c];

                _lu[(col * _n) + c]   = _lu[(pivot * _n) + c];
                _lu[(pivot * _n) + c] = tmp;
            }

            if (_perm != NULL)
            {
                const size_t tmp = _perm[col];

                _perm[col]   = _perm[pivot];
                _perm[pivot] = tmp;
            }

            *_sign = -*_sign;
        }

        {
            const struct dj_cplx_@S@ pivot_value = _lu[(col * _n) + col];

            for (r = col + 1u; r < _n; ++r)
            {
                const struct dj_cplx_@S@ f =
                    d_internal_cmat_@S@_div(_lu[(r * _n) + col], pivot_value);
                size_t c = 0u;

                _lu[(r * _n) + col] = f;

                for (c = col + 1u; c < _n; ++c)
                {
                    _lu[(r * _n) + c] = d_internal_cmat_@S@_less_product(
                        _lu[(r * _n) + c], f, _lu[(col * _n) + c]);
                }
            }
        }
    }

    return singular;
}

// d_cmat_@S@_lu_solve
//   solves A _x = _b from d_cmat_@S@_lu's factors: _b permuted, forward
// through L (unit diagonal), back through U. _x and _b must not overlap.
D_MATH_FN_KERNEL void
d_cmat_@S@_lu_solve(
    const struct dj_cplx_@S@* _lu,
    const size_t*             _perm,
    size_t                    _n,
    const struct dj_cplx_@S@* _b,
    struct dj_cplx_@S@*       _x
)
{
    size_t i = 0u;

    // forward: y into _x
    for (i = 0u; i < _n; ++i)
    {
        struct dj_cplx_@S@ s = _b[_perm[i]];
        size_t             k = 0u;

        for (k = 0u; k < i; ++k)
        {
            s = d_internal_cmat_@S@_less_product(s, _lu[(i * _n) + k],
                                                 _x[k]);
        }

        _x[i] = s;
    }

    // back: _x over y, last row first
    for (i = _n; i-- > 0u; )
    {
        struct dj_cplx_@S@ s = _x[i];
        size_t             k = 0u;

        for (k = i + 1u; k < _n; ++k)
        {
            s = d_internal_cmat_@S@_less_product(s, _lu[(i * _n) + k],
                                                 _x[k]);
        }

        _x[i] = d_internal_cmat_@S@_div(s, _lu[(i * _n) + i]);
    }

    return;
}

// d_cmat_@S@_lu_det
//   the determinant from the factors: the parity times U's diagonal, in
// order.
D_MATH_FN struct dj_cplx_@S@
d_cmat_@S@_lu_det(
    const struct dj_cplx_@S@* _lu,
    size_t                    _n,
    @T@                       _sign
)
{
    struct dj_cplx_@S@ d = d_cplx_@S@_make(_sign, (@T@)0);
    size_t             i = 0u;

    for (i = 0u; i < _n; ++i)
    {
        d = d_internal_cmat_@S@_mul(d, _lu[(i * _n) + i]);
    }

    return d;
}

// d_cmat_@S@_determinant
//   the determinant of the n-by-n _a, by its LU factors in _scratch (n by
// n): the parity times U's diagonal; 0 for a singular one.
D_MATH_FN_KERNEL struct dj_cplx_@S@
d_cmat_@S@_determinant(
    const struct dj_cplx_@S@* _a,
    struct dj_cplx_@S@*       _scratch,
    size_t                    _n
)
{
    @T@    sign = (@T@)1;
    size_t k    = 0u;

    for (k = 0u; k < (_n * _n); ++k)
    {
        _scratch[k] = _a[k];
    }

    // a singular matrix: 0
    if (d_cmat_@S@_lu(_scratch, _n, NULL, &sign))
    {
        return d_cplx_@S@_make((@T@)0, (@T@)0);
    }

    return d_cmat_@S@_lu_det(_scratch, _n, sign);
}

// d_cmat_@S@_inverse
//   _out = the inverse of the n-by-n _a, true; or, for a singular one, the
// zero matrix and false. _scratch holds n * n entries; neither it nor _out
// may be _a. Gauss-Jordan, as mat.h's d_mat_@S@_inverse: the pivot the
// first entry of largest |re| + |im| at or below the diagonal, its row
// divided by it, the others' multiples of it subtracted.
D_MATH_FN_KERNEL bool
d_cmat_@S@_inverse(
    struct dj_cplx_@S@*       _out,
    struct dj_cplx_@S@*       _scratch,
    const struct dj_cplx_@S@* _a,
    size_t                    _n
)
{
    size_t col = 0u;
    size_t k   = 0u;

    for (k = 0u; k < (_n * _n); ++k)
    {
        _scratch[k] = _a[k];
    }

    // the identity
    for (k = 0u; k < (_n * _n); ++k)
    {
        const bool diagonal = ((k / _n) == (k % _n));

        _out[k] = d_cplx_@S@_make(diagonal ? (@T@)1 : (@T@)0, (@T@)0);
    }

    for (col = 0u; col < _n; ++col)
    {
        size_t pivot = col;
        @T@    maxv  = d_internal_cmat_@S@_abs1(_scratch[(col * _n) + col]);
        size_t r     = 0u;
        size_t c     = 0u;

        for (r = col + 1u; r < _n; ++r)
        {
            const @T@ v = d_internal_cmat_@S@_abs1(_scratch[(r * _n) + col]);

            if (v > maxv)
            {
                maxv  = v;
                pivot = r;
            }
        }

        // no pivot: singular
        if (d_internal_cmat_@S@_is_zero(_scratch[(pivot * _n) + col]))
        {
            for (k = 0u; k < (_n * _n); ++k)
            {
                _out[k] = d_cplx_@S@_make((@T@)0, (@T@)0);
            }

            return false;
        }

        // the pivot row up
        if (pivot != col)
        {
            for (c = 0u; c < _n; ++c)
            {
                const struct dj_cplx_@S@ ta = _scratch[(col * _n) + c];
                const struct dj_cplx_@S@ ti = _out[(col * _n) + c];

                _scratch[(col * _n) + c]   = _scratch[(pivot * _n) + c];
                _scratch[(pivot * _n) + c] = ta;
                _out[(col * _n) + c]       = _out[(pivot * _n) + c];
                _out[(pivot * _n) + c]     = ti;
            }
        }

        // the pivot row over the pivot
        {
            const struct dj_cplx_@S@ d = _scratch[(col * _n) + col];

            for (c = 0u; c < _n; ++c)
            {
                _scratch[(col * _n) + c] =
                    d_internal_cmat_@S@_div(_scratch[(col * _n) + c], d);
                _out[(col * _n) + c] = d_internal_cmat_@S@_div(_out[(col * _n) + c],
                                                      d);
            }
        }

        // the other rows less their multiples of it
        for (r = 0u; r < _n; ++r)
        {
            if (r != col)
            {
                const struct dj_cplx_@S@ f = _scratch[(r * _n) + col];

                for (c = 0u; c < _n; ++c)
                {
                    _scratch[(r * _n) + c] = d_internal_cmat_@S@_less_product(
                        _scratch[(r * _n) + c], f, _scratch[(col * _n) + c]);
                    _out[(r * _n) + c] = d_internal_cmat_@S@_less_product(
                        _out[(r * _n) + c], f, _out[(col * _n) + c]);
                }
            }
        }
    }

    return true;
}

// d_cmat_@S@_solve
//   _x = the solution X of A X = B, A n by n and B n by _m, true; or, for a
// singular A, the zero matrix and false. Gaussian elimination of [A | B]
// with partial pivoting, the pivot the first entry of largest |re| + |im| at
// or below the diagonal, then back-substitution, as LAPACK's zgesv: for each
// column of B the same bits as d_cmat_@S@_lu and d_cmat_@S@_lu_solve give.
// _scratch holds n * n entries; neither it nor _x may be _a or _b.
D_MATH_FN_KERNEL bool
d_cmat_@S@_solve(
    struct dj_cplx_@S@*       _x,
    struct dj_cplx_@S@*       _scratch,
    const struct dj_cplx_@S@* _a,
    const struct dj_cplx_@S@* _b,
    size_t                    _n,
    size_t                    _m
)
{
    size_t col = 0u;
    size_t k   = 0u;

    for (k = 0u; k < (_n * _n); ++k)
    {
        _scratch[k] = _a[k];
    }

    for (k = 0u; k < (_n * _m); ++k)
    {
        _x[k] = _b[k];
    }

    // elimination: [A | B] to [U | Y]
    for (col = 0u; col < _n; ++col)
    {
        size_t pivot = col;
        @T@    maxv  = d_internal_cmat_@S@_abs1(_scratch[(col * _n) + col]);
        size_t r     = 0u;
        size_t c     = 0u;

        for (r = col + 1u; r < _n; ++r)
        {
            const @T@ v = d_internal_cmat_@S@_abs1(_scratch[(r * _n) + col]);

            if (v > maxv)
            {
                maxv  = v;
                pivot = r;
            }
        }

        // no pivot: singular
        if (d_internal_cmat_@S@_is_zero(_scratch[(pivot * _n) + col]))
        {
            for (k = 0u; k < (_n * _m); ++k)
            {
                _x[k] = d_cplx_@S@_make((@T@)0, (@T@)0);
            }

            return false;
        }

        // the pivot row up, in A and in B
        if (pivot != col)
        {
            for (c = 0u; c < _n; ++c)
            {
                const struct dj_cplx_@S@ t = _scratch[(col * _n) + c];

                _scratch[(col * _n) + c]   = _scratch[(pivot * _n) + c];
                _scratch[(pivot * _n) + c] = t;
            }

            for (c = 0u; c < _m; ++c)
            {
                const struct dj_cplx_@S@ t = _x[(col * _m) + c];

                _x[(col * _m) + c]   = _x[(pivot * _m) + c];
                _x[(pivot * _m) + c] = t;
            }
        }

        // the rows below less their multiples of it
        {
            const struct dj_cplx_@S@ pivot_value = _scratch[(col * _n) + col];

            for (r = col + 1u; r < _n; ++r)
            {
                const struct dj_cplx_@S@ f =
                    d_internal_cmat_@S@_div(_scratch[(r * _n) + col], pivot_value);

                for (c = col + 1u; c < _n; ++c)
                {
                    _scratch[(r * _n) + c] = d_internal_cmat_@S@_less_product(
                        _scratch[(r * _n) + c], f, _scratch[(col * _n) + c]);
                }

                for (c = 0u; c < _m; ++c)
                {
                    _x[(r * _m) + c] = d_internal_cmat_@S@_less_product(
                        _x[(r * _m) + c], f, _x[(col * _m) + c]);
                }
            }
        }
    }

    // back-substitution, column by column of B, the last row first
    for (k = 0u; k < _m; ++k)
    {
        size_t i = 0u;

        for (i = _n; i-- > 0u; )
        {
            struct dj_cplx_@S@ s = _x[(i * _m) + k];
            size_t             j = 0u;

            for (j = i + 1u; j < _n; ++j)
            {
                s = d_internal_cmat_@S@_less_product(s, _scratch[(i * _n) + j],
                                                     _x[(j * _m) + k]);
            }

            _x[(i * _m) + k] = d_internal_cmat_@S@_div(s, _scratch[(i * _n) + i]);
        }
    }

    return true;
}
"""

CMAT_HERMITIAN_HEAD = r"""
// @SECTION@.2    Hermitian matrices
//------------------------------------------------------------------------------
// A Hermitian matrix's eigenvalues, real, and its eigenvectors, unitary, by
// the cyclic Jacobi method with complex rotations (S269): decomp.h's real
// symmetric method with each pair's phase taken out first. What takes a root
// comes twice: the _c form a constant expression from C++14, the plain one
// on the C library's root, the same bit for bit.

// gcc's -Warray-bounds is set aside for this section's kernels, and restored
// after it: where a caller's matrix is a constant gcc folds, its value ranges
// take loops below for reachable with indices they never take (gcc 13 at
// -O2, a 2-by-2 rotation in main), a false warning, and under -Werror an
// error. Every index below lies within the caller's n by n.
#if ( defined(__GNUC__) &&                                                    \
      !defined(__clang__) )
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Warray-bounds"
#endif
"""

CMAT_HERMITIAN_FORMS = r"""
// d_cmat_@S@_hermitian_eigen@F@
//   the eigenvalues and eigenvectors of the Hermitian n-by-n _work by the
// cyclic Jacobi method with complex rotations, as decomp.h's
// d_decomp_@S@_sym_eigen@F@ finds a symmetric matrix's: sweeps of rotations,
// each annihilating an off-diagonal pair -- its phase u = a_pq / |a_pq| taken
// out by the unitary diag(1, conj(u)) on q, then the real rotation of the
// pair [a_pp, |a_pq|; |a_pq|, a_qq] -- until the off-diagonal part's norm is
// within _threshold, at most 100 sweeps. _work is consumed; _vectors, the
// n-by-n identity on entry, receives the eigenvectors as columns, a unitary
// matrix; _values the eigenvalues, the diagonal's real parts, ascending
// (the first smallest, among equals), the columns following. A real
// symmetric matrix gets d_decomp_@S@_sym_eigen@F@'s values bit for bit.
D_MATH_FN_KERNEL@RT@ void
d_cmat_@S@_hermitian_eigen@F@(
    struct dj_cplx_@S@* _work,
    @T@*                _values,
    struct dj_cplx_@S@* _vectors,
    size_t              _n,
    @T@                 _threshold
)
{
    size_t sweep = 0u;
    size_t p     = 0u;
    size_t q     = 0u;
    size_t i     = 0u;

    for (sweep = 0u; sweep < 100u; ++sweep)
    {
        @T@ off_sq = (@T@)0;

        for (p = 0u; p < _n; ++p)
        {
            for (q = p + 1u; q < _n; ++q)
            {
                const @T@ re  = _work[(p * _n) + q].part[0];
                const @T@ im  = _work[(p * _n) + q].part[1];
                const @T@ rr  = re * re;
                const @T@ ii  = im * im;

                off_sq = off_sq + rr;
                off_sq = off_sq + ii;
            }
        }

        // the off-diagonal part is small enough: converged
        if (d_math_@S@_sqrt@F@(off_sq) <= _threshold)
        {
            break;
        }

        for (p = 0u; p < _n; ++p)
        {
            for (q = p + 1u; q < _n; ++q)
            {
                const struct dj_cplx_@S@ apq = _work[(p * _n) + q];

                // already zero: nothing to annihilate
                if (d_internal_cmat_@S@_is_zero(apq))
                {
                    continue;
                }

                {
                    const @T@ r     = d_internal_decomp_@S@_hypot@F@(
                                          apq.part[0], apq.part[1]);
                    const struct dj_cplx_@S@ u =
                        d_cplx_@S@_make(apq.part[0] / r, apq.part[1] / r);
                    const struct dj_cplx_@S@ uc = d_cplx_@S@_conj(u);
                    const @T@ aqq   = _work[(q * _n) + q].part[0];
                    const @T@ app   = _work[(p * _n) + p].part[0];
                    const @T@ diff  = aqq - app;
                    const @T@ twoa  = (@T@)2 * r;
                    const @T@ half  = d_internal_math_@S@_pow2(@EMAX@ / 2);
                    const @T@ cut   = d_internal_math_@S@_abs(diff) / half;
                    const bool   far   = (cut >= twoa);
                    const @T@ theta = far ? (@T@)0 : (diff / twoa);
                    @T@       t     = (@T@)1;
                    @T@       c     = (@T@)0;
                    @T@       s     = (@T@)0;

                    // the smaller root of t^2 + 2 theta t - 1 = 0; past
                    // 2^(EMAX / 2), where theta^2 would overflow, 1 / (2
                    // theta), |a_pq| / diff, which it equals within a part
                    // in 2^EMAX
                    if ( (far) ||
                         (d_internal_math_@S@_abs(theta) >= half) )
                    {
                        t = r / diff;
                    }
                    else if (theta != (@T@)0)
                    {
                        const @T@ sgn  = (theta > (@T@)0) ? (@T@)1
                                                          : (@T@)-1;
                        const @T@ tt   = theta * theta;
                        const @T@ one  = tt + (@T@)1;
                        const @T@ root = d_math_@S@_sqrt@F@(one);
                        const @T@ mag  = d_internal_math_@S@_abs(theta);
                        const @T@ den  = mag + root;

                        t = sgn / den;
                    }

                    {
                        const @T@ tt   = t * t;
                        const @T@ one  = tt + (@T@)1;
                        const @T@ root = d_math_@S@_sqrt@F@(one);

                        c = (@T@)1 / root;
                        s = t * c;
                    }

                    // the columns p and q, column q turned by conj(u) first
                    for (i = 0u; i < _n; ++i)
                    {
                        const struct dj_cplx_@S@ wip = _work[(i * _n) + p];
                        const struct dj_cplx_@S@ wiq =
                            d_internal_cmat_@S@_mul(_work[(i * _n) + q], uc);
                        size_t k = 0u;

                        for (k = 0u; k < 2u; ++k)
                        {
                            const @T@ a1 = c * wip.part[k];
                            const @T@ a2 = s * wiq.part[k];
                            const @T@ b1 = s * wip.part[k];
                            const @T@ b2 = c * wiq.part[k];

                            _work[(i * _n) + p].part[k] = a1 - a2;
                            _work[(i * _n) + q].part[k] = b1 + b2;
                        }
                    }

                    // the rows p and q, row q turned by u first
                    for (i = 0u; i < _n; ++i)
                    {
                        const struct dj_cplx_@S@ wpj = _work[(p * _n) + i];
                        const struct dj_cplx_@S@ wqj =
                            d_internal_cmat_@S@_mul(u, _work[(q * _n) + i]);
                        size_t k = 0u;

                        for (k = 0u; k < 2u; ++k)
                        {
                            const @T@ a1 = c * wpj.part[k];
                            const @T@ a2 = s * wqj.part[k];
                            const @T@ b1 = s * wpj.part[k];
                            const @T@ b2 = c * wqj.part[k];

                            _work[(p * _n) + i].part[k] = a1 - a2;
                            _work[(q * _n) + i].part[k] = b1 + b2;
                        }
                    }

                    // V's columns, as A's
                    for (i = 0u; i < _n; ++i)
                    {
                        const struct dj_cplx_@S@ vip = _vectors[(i * _n) + p];
                        const struct dj_cplx_@S@ viq =
                            d_internal_cmat_@S@_mul(_vectors[(i * _n) + q], uc);
                        size_t k = 0u;

                        for (k = 0u; k < 2u; ++k)
                        {
                            const @T@ a1 = c * vip.part[k];
                            const @T@ a2 = s * viq.part[k];
                            const @T@ b1 = s * vip.part[k];
                            const @T@ b2 = c * viq.part[k];

                            _vectors[(i * _n) + p].part[k] = a1 - a2;
                            _vectors[(i * _n) + q].part[k] = b1 + b2;
                        }
                    }
                }
            }
        }
    }

    for (i = 0u; i < _n; ++i)
    {
        _values[i] = _work[(i * _n) + i].part[0];
    }

    // ascending, the vectors' columns along
    for (i = 0u; i < _n; ++i)
    {
        size_t sel = i;
        size_t k   = 0u;

        for (k = i + 1u; k < _n; ++k)
        {
            if (_values[k] < _values[sel])
            {
                sel = k;
            }
        }

        if (sel != i)
        {
            const @T@ tmp = _values[i];

            _values[i]   = _values[sel];
            _values[sel] = tmp;

            for (k = 0u; k < _n; ++k)
            {
                const struct dj_cplx_@S@ t = _vectors[(k * _n) + i];

                _vectors[(k * _n) + i]   = _vectors[(k * _n) + sel];
                _vectors[(k * _n) + sel] = t;
            }
        }
    }

    return;
}

"""

CMAT_GENERAL_HEAD = r"""
// @SECTION@.3    The eigenvalues and eigenvectors of a general matrix
//------------------------------------------------------------------------------
// LAPACK's path, as its zgeev takes it (S269): the matrix scaled by a power
// of two so that its largest part lies in [1, 2), balanced (zgebal), reduced
// to upper Hessenberg form by Householder reflectors (zgehd2), its complex
// Schur form found by single-shift QR (zlahqr), the subdiagonal kept real;
// then the eigenvectors from the Schur form by back-substitution, as ztrevc,
// back-transformed (zgebak). The eigenvalues ascend by real part, then by
// imaginary part. The scaling is exact, so 2^k A has 2^k times A's
// eigenvalues, bit for bit, barring overflow and underflow. Each large
// kernel is compiled once in a translation unit (D_MATH_FN_KERNEL), and
// cplx.h's product and quotient through it, so that its many calls are not
// each a copy. What takes a root comes twice, the same bit for bit.
"""

CMAT_GENERAL_ONCE = r"""
// d_internal_cmat_@S@_swap_isolated
//   the swap balancing makes to isolate an eigenvalue: columns _i and _j in
// rows 0 to _l, then rows _i and _j in columns _k to n - 1 -- a similarity
// by a permutation.
D_MATH_FN void
d_internal_cmat_@S@_swap_isolated(
    struct dj_cplx_@S@* _a,
    size_t              _n,
    size_t              _i,
    size_t              _j,
    size_t              _k,
    size_t              _l
)
{
    size_t r = 0u;

    for (r = 0u; r <= _l; ++r)
    {
        const struct dj_cplx_@S@ tmp = _a[(r * _n) + _i];

        _a[(r * _n) + _i] = _a[(r * _n) + _j];
        _a[(r * _n) + _j] = tmp;
    }

    for (r = _k; r < _n; ++r)
    {
        const struct dj_cplx_@S@ tmp = _a[(_i * _n) + r];

        _a[(_i * _n) + r] = _a[(_j * _n) + r];
        _a[(_j * _n) + r] = tmp;
    }

    return;
}

// d_cmat_@S@_balance_back
//   undoes d_cmat_@S@_balance on the n-by-_m row-major _v, whose columns
// are right eigenvectors of the balanced matrix, as LAPACK's zgebak does:
// each row from _ilo to _ihi multiplied by its scaling factor, exactly, then
// the permutations undone, the last first.
D_MATH_FN_KERNEL void
d_cmat_@S@_balance_back(
    struct dj_cplx_@S@* _v,
    size_t              _n,
    size_t              _m,
    const size_t*       _perm,
    const @T@*          _scale,
    size_t              _ilo,
    size_t              _ihi
)
{
    size_t i  = 0u;
    size_t c  = 0u;
    size_t ii = 0u;

    // nothing to undo
    if (_n == 0u)
    {
        return;
    }

    // the scaling
    for (i = _ilo; i <= _ihi; ++i)
    {
        for (c = 0u; c < _m; ++c)
        {
            _v[(i * _m) + c] = d_cplx_@S@_mul_real(_v[(i * _m) + c],
                                                   _scale[i]);
        }
    }

    // the permutations, the last first
    for (ii = 0u; ii < _n; ++ii)
    {
        size_t row   = ii;
        size_t other = 0u;

        // inside the balanced block: no swap
        if ( (row >= _ilo) &&
             (row <= _ihi) )
        {
            continue;
        }

        // a column pushed to the top: the last pushed first
        if (row < _ilo)
        {
            row = _ilo - 1u - ii;
        }

        other = _perm[row];

        // swapped with itself: nothing to undo
        if (other == row)
        {
            continue;
        }

        for (c = 0u; c < _m; ++c)
        {
            const struct dj_cplx_@S@ tmp = _v[(row * _m) + c];

            _v[(row * _m) + c]   = _v[(other * _m) + c];
            _v[(other * _m) + c] = tmp;
        }
    }

    return;
}

// d_internal_cmat_@S@_scaling_of
//   decomp.h's d_internal_decomp_@S@_scaling_of for the _count complex
// entries of _a: their parts' largest magnitude's binade, found exactly,
// after a lift by 2^P where that magnitude is subnormal; not finite where a
// part is NaN or infinite, zero where every part is zero.
D_MATH_FN struct dj_internal_decomp_@S@_scaling
d_internal_cmat_@S@_scaling_of(
    const struct dj_cplx_@S@* _a,
    size_t                    _count
)
{
    struct dj_internal_decomp_@S@_scaling result =
        { (@T@)1, (@T@)1, true, false };
    @T@    largest = (@T@)0;
    size_t i       = 0u;
    size_t p       = 0u;

    for (i = 0u; i < _count; ++i)
    {
        for (p = 0u; p < 2u; ++p)
        {
            const @T@ v = d_internal_math_@S@_abs(_a[i].part[p]);

            // a NaN, or an infinity: nothing to scale
            if ( (v != v) ||
                 (v > @MAX@) )
            {
                result.finite = false;

                return result;
            }

            if (v > largest)
            {
                largest = v;
            }
        }
    }

    // every part zero
    if (largest == (@T@)0)
    {
        result.zero = true;

        return result;
    }

    // a subnormal largest: lifted into the normal range first
    if (largest < @MIN@)
    {
        result.pre = d_internal_math_@S@_pow2(@P@);
        largest    = largest * result.pre;
    }

    {
        const struct dj_internal_math_@S@_binade b =
            d_internal_math_@S@_binade_of(largest);

        result.power = b.power;
    }

    return result;
}

// d_internal_cmat_@S@_turn
//   the _count entries _x[0], _x[_stride], ... each multiplied by _s, as
// BLAS's zscal: the phase LAPACK's zlahqr turns rows and columns by, to keep
// the subdiagonal real.
D_MATH_FN void
d_internal_cmat_@S@_turn(
    struct dj_cplx_@S@* _x,
    size_t              _count,
    size_t              _stride,
    struct dj_cplx_@S@  _s
)
{
    size_t i = 0u;

    for (i = 0u; i < _count; ++i)
    {
        _x[i * _stride] = d_internal_cmat_@S@_mul(_s, _x[i * _stride]);
    }

    return;
}
"""

CMAT_GENERAL_FORMS = r"""
// d_internal_cmat_@S@_abs@F@
//   |_z|, the modulus, by d_internal_decomp_@S@_hypot@F@: neither
// overflowing nor underflowing on the way, the same bits in both forms.
D_MATH_FN@RT@ @T@
d_internal_cmat_@S@_abs@F@(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_decomp_@S@_hypot@F@(_z.part[0], _z.part[1]);
}

// d_internal_cmat_@S@_sqrt@F@
//   the principal square root of _z, as the shifts need it: t = sqrt((|z| +
// |x|) / 2), each half taken first, then (t, y / (2 t)) where x >= 0, else
// (|y| / (2 t), +-t) with y's sign -- on the core's real roots, the same bits
// in both forms; 0 for 0.
D_MATH_FN@RT@ struct dj_cplx_@S@
d_internal_cmat_@S@_sqrt@F@(
    struct dj_cplx_@S@ _z
)
{
    const @T@ x = _z.part[0];
    const @T@ y = _z.part[1];

    // zero: zero
    if (d_internal_cmat_@S@_is_zero(_z))
    {
        return d_cplx_@S@_make((@T@)0, (@T@)0);
    }

    {
        const @T@ m    = d_internal_decomp_@S@_hypot@F@(x, y);
        const @T@ ax   = d_internal_math_@S@_abs(x);
        const @T@ hm   = m / (@T@)2;
        const @T@ hx   = ax / (@T@)2;
        const @T@ sum  = hm + hx;
        const @T@ t    = d_math_@S@_sqrt@F@(sum);
        const @T@ twot = (@T@)2 * t;

        if (x >= (@T@)0)
        {
            return d_cplx_@S@_make(t, y / twot);
        }

        {
            const @T@ ay = d_internal_math_@S@_abs(y);

            return d_cplx_@S@_make(ay / twot, (y < (@T@)0) ? -t : t);
        }
    }
}

// d_internal_cmat_@S@_hypot3@F@
//   sqrt(_x^2 + _y^2 + _z^2), as LAPACK's dlapy3: the largest magnitude
// times the root of the sum of the squared ratios to it; the sum of the
// magnitudes where that is zero.
D_MATH_FN@RT@ @T@
d_internal_cmat_@S@_hypot3@F@(
    @T@ _x,
    @T@ _y,
    @T@ _z
)
{
    const @T@ ax = d_internal_math_@S@_abs(_x);
    const @T@ ay = d_internal_math_@S@_abs(_y);
    const @T@ az = d_internal_math_@S@_abs(_z);
    const @T@ w1 = (ax > ay) ? ax : ay;
    const @T@ w  = (w1 > az) ? w1 : az;

    // all zero, or an infinity
    if ( (w == (@T@)0) ||
         (w > @MAX@) )
    {
        const @T@ s = ax + ay;

        return s + az;
    }

    {
        const @T@ rx = ax / w;
        const @T@ ry = ay / w;
        const @T@ rz = az / w;
        const @T@ xx = rx * rx;
        const @T@ yy = ry * ry;
        const @T@ zz = rz * rz;
        const @T@ s1 = xx + yy;
        const @T@ s2 = s1 + zz;
        const @T@ r  = d_math_@S@_sqrt@F@(s2);

        return w * r;
    }
}

// d_cmat_@S@_balance@F@
//   balances the complex n-by-n _a in place, as LAPACK's zgebal does with
// job 'B', and decomp.h's d_decomp_@S@_balance@F@ for a real one: the rows
// and columns that isolate an eigenvalue permuted to the ends, then each
// row and column inside *_ilo..*_ihi scaled by a power of two while that
// brings the sum of their norms below 0.95 of what it was. _scale (n)
// receives the factors, 1 outside; _perm (n), unless NULL, each row's swap.
// Indices count from 0.
D_MATH_FN_KERNEL@RT@ void
d_cmat_@S@_balance@F@(
    struct dj_cplx_@S@* _a,
    size_t              _n,
    size_t*             _perm,
    @T@*                _scale,
    size_t*             _ilo,
    size_t*             _ihi
)
{
    const @T@ sfmin1 = @MIN@ / @EPS@;
    const @T@ sfmax1 = (@T@)1 / sfmin1;
    const @T@ sfmin2 = sfmin1 * (@T@)2;
    const @T@ sfmax2 = (@T@)1 / sfmin2;
    size_t    k      = 0u;
    size_t    l      = 0u;
    size_t    i      = 0u;
    size_t    j      = 0u;
    bool      noconv = true;

    *_ilo = 0u;
    *_ihi = 0u;

    // nothing to balance
    if (_n == 0u)
    {
        return;
    }

    for (i = 0u; i < _n; ++i)
    {
        _scale[i] = (@T@)1;

        if (_perm != NULL)
        {
            _perm[i] = i;
        }
    }

    l = _n - 1u;

    // rows isolating an eigenvalue, pushed to the bottom
    while (noconv)
    {
        const size_t top = l;

        noconv = false;

        for (i = top + 1u; i-- > 0u; )
        {
            bool canswap = true;

            for (j = 0u; j <= l; ++j)
            {
                if ( (i != j) &&
                     (!d_internal_cmat_@S@_is_zero(_a[(i * _n) + j])) )
                {
                    canswap = false;

                    break;
                }
            }

            // the row is zero off the diagonal: to the bottom
            if (canswap)
            {
                if (_perm != NULL)
                {
                    _perm[l] = i;
                }

                if (i != l)
                {
                    d_internal_cmat_@S@_swap_isolated(_a, _n, i, l, k, l);
                }

                noconv = true;

                // every row isolated: the matrix is triangular
                if (l == 0u)
                {
                    return;
                }

                --l;
            }
        }
    }

    // columns isolating an eigenvalue, pushed to the top
    noconv = true;

    while (noconv)
    {
        const size_t first = k;

        noconv = false;

        for (j = first; j <= l; ++j)
        {
            bool canswap = true;

            for (i = k; i <= l; ++i)
            {
                if ( (i != j) &&
                     (!d_internal_cmat_@S@_is_zero(_a[(i * _n) + j])) )
                {
                    canswap = false;

                    break;
                }
            }

            // the column is zero off the diagonal: to the top
            if (canswap)
            {
                if (_perm != NULL)
                {
                    _perm[k] = j;
                }

                if (j != k)
                {
                    d_internal_cmat_@S@_swap_isolated(_a, _n, j, k, k, l);
                }

                noconv = true;
                ++k;
            }
        }
    }

    // every column isolated: one row is left, alone
    if (k > l)
    {
        k = l;
    }

    // the norms brought near, row and column by turns
    noconv = true;

    while (noconv)
    {
        noconv = false;

        for (i = k; i <= l; ++i)
        {
            const size_t count = (l - k) + 1u;
            @T@          c     = d_internal_decomp_@S@_cnorm@F@(
                                     _a + ((k * _n) + i), count, _n);
            @T@          r     = d_internal_decomp_@S@_cnorm@F@(
                                     _a + ((i * _n) + k), count, 1u);
            @T@          ca    = (@T@)0;
            @T@          ra    = (@T@)0;
            @T@          g     = (@T@)0;
            @T@          f     = (@T@)1;
            @T@          s     = (@T@)0;

            // the modulus of the entry of largest |re| + |im|, the first
            {
                size_t best = 0u;

                for (j = 1u; j <= l; ++j)
                {
                    if (d_internal_cmat_@S@_abs1(_a[(j * _n) + i]) >
                        d_internal_cmat_@S@_abs1(_a[(best * _n) + i]))
                    {
                        best = j;
                    }
                }

                ca = d_internal_cmat_@S@_abs@F@(_a[(best * _n) + i]);
                best = k;

                for (j = k + 1u; j < _n; ++j)
                {
                    if (d_internal_cmat_@S@_abs1(_a[(i * _n) + j]) >
                        d_internal_cmat_@S@_abs1(_a[(i * _n) + best]))
                    {
                        best = j;
                    }
                }

                ra = d_internal_cmat_@S@_abs@F@(_a[(i * _n) + best]);
            }

            // a zero norm, through underflow: nothing to bring near
            if ( (c == (@T@)0) ||
                 (r == (@T@)0) )
            {
                continue;
            }

            g = r / (@T@)2;
            s = c + r;

            // the column too small: doubled while it stays below half the row
            while (c < g)
            {
                const @T@ big1  = (f > c) ? f : c;
                const @T@ big   = (big1 > ca) ? big1 : ca;
                const @T@ tiny1 = (r < g) ? r : g;
                const @T@ tiny  = (tiny1 < ra) ? tiny1 : ra;

                if ( (!(big < sfmax2)) ||
                     (!(tiny > sfmin2)) )
                {
                    break;
                }

                f  = f * (@T@)2;
                c  = c * (@T@)2;
                ca = ca * (@T@)2;
                r  = r / (@T@)2;
                g  = g / (@T@)2;
                ra = ra / (@T@)2;
            }

            g = c / (@T@)2;

            // the column too large: halved while half of it is at least the row
            while (g >= r)
            {
                const @T@ big   = (r > ra) ? r : ra;
                const @T@ tiny1 = (f < c) ? f : c;
                const @T@ tiny2 = (g < ca) ? g : ca;
                const @T@ tiny  = (tiny1 < tiny2) ? tiny1 : tiny2;

                if ( (!(big < sfmax2)) ||
                     (!(tiny > sfmin2)) )
                {
                    break;
                }

                f  = f / (@T@)2;
                c  = c / (@T@)2;
                g  = g / (@T@)2;
                ca = ca / (@T@)2;
                r  = r * (@T@)2;
                ra = ra * (@T@)2;
            }

            {
                const @T@ sum   = c + r;
                const @T@ bound = (@T@)0.95 * s;

                // not enough gained
                if (sum >= bound)
                {
                    continue;
                }
            }

            // the factor would leave the safe range
            if ( (f < (@T@)1) &&
                 (_scale[i] < (@T@)1) )
            {
                const @T@ product = f * _scale[i];

                if (product <= sfmin1)
                {
                    continue;
                }
            }

            if ( (f > (@T@)1) &&
                 (_scale[i] > (@T@)1) )
            {
                const @T@ room = sfmax1 / f;

                if (_scale[i] >= room)
                {
                    continue;
                }
            }

            g         = (@T@)1 / f;
            _scale[i] = _scale[i] * f;
            noconv    = true;

            for (j = k; j < _n; ++j)
            {
                _a[(i * _n) + j] = d_cplx_@S@_mul_real(_a[(i * _n) + j], g);
            }

            for (j = 0u; j <= l; ++j)
            {
                _a[(j * _n) + i] = d_cplx_@S@_mul_real(_a[(j * _n) + i], f);
            }
        }
    }

    *_ilo = k;
    *_ihi = l;

    return;
}

// d_internal_cmat_@S@_reflector@F@
//   the elementary reflector H = I - tau v v^H, v = (1, x), x the _count
// entries _x[0], _x[_stride], ..., that takes (*_alpha, x) to (beta, 0) with
// beta real, as LAPACK's zlarfg: beta = -sign(re alpha) times the norm of
// (alpha, x), tau = ((beta - re alpha) / beta, -im alpha / beta), x
// multiplied by 1 / (alpha - beta); lifted first where beta is below the
// family's least normal over its epsilon, and lowered after. *_alpha
// becomes beta, _x v's tail, and tau is returned: 0, H = I, where x is zero
// and alpha real.
D_MATH_FN_KERNEL@RT@ struct dj_cplx_@S@
d_internal_cmat_@S@_reflector@F@(
    struct dj_cplx_@S@* _alpha,
    struct dj_cplx_@S@* _x,
    size_t              _count,
    size_t              _stride
)
{
    const @T@ safmin = @MIN@ / (@EPS@ / (@T@)2);
    const @T@ rsafmn = (@T@)1 / safmin;
    @T@       xnorm  = d_internal_decomp_@S@_cnorm@F@(_x, _count, _stride);
    @T@       alphr  = _alpha->part[0];
    @T@       alphi  = _alpha->part[1];
    @T@       beta   = (@T@)0;
    size_t    knt    = 0u;
    size_t    i      = 0u;
    struct dj_cplx_@S@ tau = d_cplx_@S@_make((@T@)0, (@T@)0);

    // x zero and alpha real: H = I
    if ( (xnorm == (@T@)0) &&
         (alphi == (@T@)0) )
    {
        return tau;
    }

    {
        const @T@ h = d_internal_cmat_@S@_hypot3@F@(alphr, alphi, xnorm);

        beta = (alphr < (@T@)0) ? h : -h;
    }

    // beta below the safe minimum: lifted, and found again
    if (d_internal_math_@S@_abs(beta) < safmin)
    {
        do
        {
            ++knt;

            for (i = 0u; i < _count; ++i)
            {
                _x[i * _stride] = d_cplx_@S@_mul_real(_x[i * _stride],
                                                      rsafmn);
            }

            beta  = beta * rsafmn;
            alphi = alphi * rsafmn;
            alphr = alphr * rsafmn;
        }
        while ( (d_internal_math_@S@_abs(beta) < safmin) &&
                (knt < 20u) );

        xnorm = d_internal_decomp_@S@_cnorm@F@(_x, _count, _stride);

        {
            const @T@ h = d_internal_cmat_@S@_hypot3@F@(alphr, alphi, xnorm);

            beta = (alphr < (@T@)0) ? h : -h;
        }
    }

    {
        const @T@ diff = beta - alphr;
        const @T@ mi   = -alphi;
        const struct dj_cplx_@S@ rec = d_internal_cmat_@S@_div(
            d_cplx_@S@_make((@T@)1, (@T@)0),
            d_cplx_@S@_make(alphr - beta, alphi));

        tau = d_cplx_@S@_make(diff / beta, mi / beta);

        for (i = 0u; i < _count; ++i)
        {
            _x[i * _stride] = d_internal_cmat_@S@_mul(rec, _x[i * _stride]);
        }
    }

    // lowered back
    for (i = 0u; i < knt; ++i)
    {
        beta = beta * safmin;
    }

    *_alpha = d_cplx_@S@_make(beta, (@T@)0);

    return tau;
}

// d_cmat_@S@_hessenberg_q@F@
//   reduces rows and columns _ilo to _ihi of the complex n-by-n _h to upper
// Hessenberg form in place by Householder reflectors, as LAPACK's zgehd2:
// for each column from _ilo, the reflector H that zeroes it below the
// subdiagonal and makes the subdiagonal entry real, H applied from the right
// to rows 0 to _ihi and H^H from the left to columns to the end -- a
// similarity, H = Q^H A Q. _q, unless NULL, the n-by-n identity on entry,
// receives Q, each reflector multiplied in on the right as it is made. The
// entries below the subdiagonal end exactly zero.
D_MATH_FN_KERNEL@RT@ void
d_cmat_@S@_hessenberg_q@F@(
    struct dj_cplx_@S@* _h,
    size_t              _n,
    size_t              _ilo,
    size_t              _ihi,
    struct dj_cplx_@S@* _q
)
{
    size_t i = 0u;

    // nothing to reduce
    if ( (_n == 0u) ||
         (_ihi <= _ilo) )
    {
        return;
    }

    for (i = _ilo; i < _ihi; ++i)
    {
        const size_t             below = _ihi - i - 1u;
        struct dj_cplx_@S@       alpha = _h[((i + 1u) * _n) + i];
        const struct dj_cplx_@S@ tau   = d_internal_cmat_@S@_reflector@F@(
            &alpha,
            _h + (((((i + 2u) < _n) ? (i + 2u) : (_n - 1u)) * _n) + i),
            below,
            _n);
        const struct dj_cplx_@S@ ctau  = d_cplx_@S@_conj(tau);
        size_t                   r     = 0u;
        size_t                   c     = 0u;

        _h[((i + 1u) * _n) + i] = alpha;

        // H = I: nothing to apply
        if (!d_internal_cmat_@S@_is_zero(tau))
        {
            // from the right, H: rows 0 to ihi, columns i + 1 to ihi
            for (r = 0u; r <= _ihi; ++r)
            {
                struct dj_cplx_@S@ w = _h[(r * _n) + (i + 1u)];

                for (c = i + 2u; c <= _ihi; ++c)
                {
                    w = d_cplx_@S@_add(w, d_internal_cmat_@S@_mul(_h[(r * _n) + c],
                                                         _h[(c * _n) + i]));
                }

                {
                    const struct dj_cplx_@S@ t =
                        d_internal_cmat_@S@_mul(tau, w);

                    _h[(r * _n) + (i + 1u)] =
                        d_cplx_@S@_sub(_h[(r * _n) + (i + 1u)], t);

                    for (c = i + 2u; c <= _ihi; ++c)
                    {
                        _h[(r * _n) + c] = d_internal_cmat_@S@_less_product(
                            _h[(r * _n) + c], t,
                            d_cplx_@S@_conj(_h[(c * _n) + i]));
                    }
                }
            }

            // from the left, H^H: rows i + 1 to ihi, columns i + 1 to the end
            for (c = i + 1u; c < _n; ++c)
            {
                struct dj_cplx_@S@ w = _h[((i + 1u) * _n) + c];

                for (r = i + 2u; r <= _ihi; ++r)
                {
                    w = d_cplx_@S@_add(w, d_internal_cmat_@S@_mul(
                                              d_cplx_@S@_conj(
                                                  _h[(r * _n) + i]),
                                              _h[(r * _n) + c]));
                }

                {
                    const struct dj_cplx_@S@ t = d_internal_cmat_@S@_mul(ctau, w);

                    _h[((i + 1u) * _n) + c] =
                        d_cplx_@S@_sub(_h[((i + 1u) * _n) + c], t);

                    for (r = i + 2u; r <= _ihi; ++r)
                    {
                        _h[(r * _n) + c] = d_internal_cmat_@S@_less_product(
                            _h[(r * _n) + c], _h[(r * _n) + i], t);
                    }
                }
            }

            // Q's columns i + 1 to ihi, from the right
            if (_q != NULL)
            {
                for (r = _ilo; r <= _ihi; ++r)
                {
                    struct dj_cplx_@S@ w = _q[(r * _n) + (i + 1u)];

                    for (c = i + 2u; c <= _ihi; ++c)
                    {
                        w = d_cplx_@S@_add(w, d_internal_cmat_@S@_mul(
                                                  _q[(r * _n) + c],
                                                  _h[(c * _n) + i]));
                    }

                    {
                        const struct dj_cplx_@S@ t =
                            d_internal_cmat_@S@_mul(tau, w);

                        _q[(r * _n) + (i + 1u)] =
                            d_cplx_@S@_sub(_q[(r * _n) + (i + 1u)], t);

                        for (c = i + 2u; c <= _ihi; ++c)
                        {
                            _q[(r * _n) + c] =
                                d_internal_cmat_@S@_less_product(
                                    _q[(r * _n) + c], t,
                                    d_cplx_@S@_conj(_h[(c * _n) + i]));
                        }
                    }
                }
            }
        }

        // the reflector's vector, kept below the subdiagonal until now
        for (r = i + 2u; r <= _ihi; ++r)
        {
            _h[(r * _n) + i] = d_cplx_@S@_make((@T@)0, (@T@)0);
        }
    }

    return;
}

// d_cmat_@S@_schur@F@
//   the eigenvalues of the complex n-by-n upper Hessenberg _h, rows and
// columns _ilo to _ihi its unreduced part (the rest triangular), into
// _values, by single-shift complex QR, as LAPACK's zlahqr: the subdiagonal
// made real first, by turning rows and columns; then from the bottom up, a
// subdiagonal entry negligible by Ahues and Kressner's test -- relative to
// its neighbours, _tol below the family's epsilon taken as the epsilon --
// set to zero and the eigenvalue below it split off, or else a single-shift
// step on the unreduced block from the row where two small subdiagonal
// entries let it start: Wilkinson's shift, the trailing 2-by-2's eigenvalue
// nearer its last entry, and at every tenth step without a split an
// exceptional shift; the subdiagonal kept real. At most _max_iter steps in
// all. With _z NULL only the eigenvalues are found; else _h ends upper
// triangular, its complex Schur form, and the n-by-n _z, Q on entry, is
// multiplied by each transformation. _values receives the eigenvalues in the
// order of T's diagonal. Returns whether all were found; if not, the rows
// not reached take their diagonal entries.
D_MATH_FN_KERNEL@RT@ bool
d_cmat_@S@_schur@F@(
    struct dj_cplx_@S@* _h,
    size_t              _n,
    size_t              _ilo,
    size_t              _ihi,
    struct dj_cplx_@S@* _z,
    struct dj_cplx_@S@* _values,
    size_t              _max_iter,
    @T@                 _tol
)
{
    const @T@    dat1   = (@T@)0.75;
    const size_t kexsh  = 10u;
    const @T@    ulp    = @EPS@;
    const @T@    tol    = (_tol > ulp) ? _tol : ulp;
    const bool   wantt  = (_z != NULL);
    const struct dj_cplx_@S@ zero = d_cplx_@S@_make((@T@)0, (@T@)0);
    size_t       nh     = 0u;
    @T@          smlnum = (@T@)0;
    size_t       i1     = 0u;
    size_t       i2     = 0u;
    size_t       kdefl  = 0u;
    size_t       steps  = 0u;
    size_t       top    = 0u;
    size_t       j      = 0u;

    // nothing to find
    if (_n == 0u)
    {
        return true;
    }

    // the eigenvalues outside the unreduced part: the diagonal
    for (j = 0u; j < _n; ++j)
    {
        if ( (j < _ilo) ||
             (j > _ihi) )
        {
            _values[j] = _h[(j * _n) + j];
        }
    }

    // a block of one
    if (_ilo == _ihi)
    {
        _values[_ilo] = _h[(_ilo * _n) + _ilo];

        return true;
    }

    // what lies below the subdiagonal, cleared
    for (j = _ilo; (j + 3u) <= _ihi; ++j)
    {
        _h[((j + 2u) * _n) + j] = zero;
        _h[((j + 3u) * _n) + j] = zero;
    }

    if ((_ilo + 2u) <= _ihi)
    {
        _h[(_ihi * _n) + (_ihi - 2u)] = zero;
    }

    // the subdiagonal made real, each entry turned by its phase
    {
        const size_t jlo = wantt ? 0u : _ilo;
        const size_t jhi = wantt ? (_n - 1u) : _ihi;
        size_t       i   = 0u;

        for (i = _ilo + 1u; i <= _ihi; ++i)
        {
            const struct dj_cplx_@S@ h = _h[(i * _n) + (i - 1u)];

            if (h.part[1] != (@T@)0)
            {
                const struct dj_cplx_@S@ s1 =
                    d_cplx_@S@_div_real(h, d_internal_cmat_@S@_abs1(h));
                const struct dj_cplx_@S@ sc =
                    d_cplx_@S@_div_real(d_cplx_@S@_conj(s1),
                                        d_internal_cmat_@S@_abs@F@(s1));
                const size_t last = (jhi < (i + 1u)) ? jhi : (i + 1u);

                _h[(i * _n) + (i - 1u)] =
                    d_cplx_@S@_make(d_internal_cmat_@S@_abs@F@(h), (@T@)0);
                d_internal_cmat_@S@_turn(_h + ((i * _n) + i), (jhi - i) + 1u,
                                         1u, sc);
                d_internal_cmat_@S@_turn(_h + ((jlo * _n) + i),
                                         (last - jlo) + 1u, _n,
                                         d_cplx_@S@_conj(sc));

                if (_z != NULL)
                {
                    d_internal_cmat_@S@_turn(_z + i, _n, _n,
                                             d_cplx_@S@_conj(sc));
                }
            }
        }
    }

    nh     = (_ihi - _ilo) + 1u;
    smlnum = @MIN@ * ((@T@)nh / ulp);

    if (wantt)
    {
        i1 = 0u;
        i2 = _n - 1u;
    }

    // top is one past the last row still to be solved
    top = _ihi + 1u;

    while (top > _ilo)
    {
        const size_t i = top - 1u;
        size_t       l = _ilo;
        size_t       k = 0u;

        for (;;)
        {
            size_t             m = 0u;
            struct dj_cplx_@S@ v[2] = { { { (@T@)0, (@T@)0 } },
                                        { { (@T@)0, (@T@)0 } } };
            struct dj_cplx_@S@ t    = zero;

            // the lowest negligible subdiagonal entry
            for (k = i; k > l; --k)
            {
                const struct dj_cplx_@S@ hk = _h[(k * _n) + (k - 1u)];
                @T@                      tst = (@T@)0;

                if (d_internal_cmat_@S@_abs1(hk) <= smlnum)
                {
                    break;
                }

                {
                    const @T@ d1 = d_internal_cmat_@S@_abs1(
                                       _h[((k - 1u) * _n) + (k - 1u)]);
                    const @T@ d2 = d_internal_cmat_@S@_abs1(
                                       _h[(k * _n) + k]);

                    tst = d1 + d2;
                }

                // both neighbours zero: the next ones out
                if (tst == (@T@)0)
                {
                    if (k >= (_ilo + 2u))
                    {
                        tst = tst + d_internal_math_@S@_abs(
                                        _h[((k - 1u) * _n) + (k - 2u)].part[0]);
                    }

                    if ((k + 1u) <= _ihi)
                    {
                        tst = tst + d_internal_math_@S@_abs(
                                        _h[((k + 1u) * _n) + k].part[0]);
                    }
                }

                {
                    const @T@ sub   = d_internal_math_@S@_abs(hk.part[0]);
                    const @T@ bound = tol * tst;

                    // Ahues and Kressner's test
                    if (sub <= bound)
                    {
                        const @T@ e1   = d_internal_cmat_@S@_abs1(hk);
                        const @T@ e2   = d_internal_cmat_@S@_abs1(
                                             _h[((k - 1u) * _n) + k]);
                        const @T@ ab   = (e1 > e2) ? e1 : e2;
                        const @T@ ba   = (e1 > e2) ? e2 : e1;
                        const @T@ hkk  = d_internal_cmat_@S@_abs1(
                                             _h[(k * _n) + k]);
                        const @T@ ad   = d_internal_cmat_@S@_abs1(
                                             d_cplx_@S@_sub(
                                                 _h[((k - 1u) * _n) +
                                                    (k - 1u)],
                                                 _h[(k * _n) + k]));
                        const @T@ aa   = (hkk > ad) ? hkk : ad;
                        const @T@ bb   = (hkk > ad) ? ad : hkk;
                        const @T@ s    = aa + ab;
                        const @T@ r1   = ab / s;
                        const @T@ lhs  = ba * r1;
                        const @T@ r2   = aa / s;
                        const @T@ p    = bb * r2;
                        const @T@ rhs1 = tol * p;
                        const @T@ rhs  = (smlnum > rhs1) ? smlnum : rhs1;

                        if (lhs <= rhs)
                        {
                            break;
                        }
                    }
                }
            }

            l = k;

            // negligible: zero
            if (l > _ilo)
            {
                _h[(l * _n) + (l - 1u)] = zero;
            }

            // an eigenvalue split off
            if (l >= i)
            {
                break;
            }

            // the budget spent: the rows not reached, their diagonal
            if (steps >= _max_iter)
            {
                for (j = _ilo; j <= i; ++j)
                {
                    _values[j] = _h[(j * _n) + j];
                }

                return false;
            }

            ++steps;
            ++kdefl;

            // eigenvalues alone: only the active block transformed
            if (!wantt)
            {
                i1 = l;
                i2 = i;
            }

            // exceptional shifts, at the bottom, then at the top
            if ((kdefl % (2u * kexsh)) == 0u)
            {
                const @T@ s = dat1 * d_internal_math_@S@_abs(
                                         _h[(i * _n) + (i - 1u)].part[0]);

                t = d_cplx_@S@_add_real(_h[(i * _n) + i], s);
            }
            else if ((kdefl % kexsh) == 0u)
            {
                const @T@ s = dat1 * d_internal_math_@S@_abs(
                                         _h[((l + 1u) * _n) + l].part[0]);

                t = d_cplx_@S@_add_real(_h[(l * _n) + l], s);
            }
            // Wilkinson's
            else
            {
                const struct dj_cplx_@S@ u = d_internal_cmat_@S@_mul(
                    d_internal_cmat_@S@_sqrt@F@(_h[((i - 1u) * _n) + i]),
                    d_internal_cmat_@S@_sqrt@F@(_h[(i * _n) + (i - 1u)]));
                @T@ s = d_internal_cmat_@S@_abs1(u);

                t = _h[(i * _n) + i];

                if (s != (@T@)0)
                {
                    const struct dj_cplx_@S@ x = d_cplx_@S@_mul_real(
                        d_cplx_@S@_sub(_h[((i - 1u) * _n) + (i - 1u)], t),
                        (@T@)0.5);
                    const @T@ sx = d_internal_cmat_@S@_abs1(x);
                    struct dj_cplx_@S@ y = zero;

                    s = (s > sx) ? s : sx;

                    {
                        const struct dj_cplx_@S@ xs = d_cplx_@S@_div_real(x, s);
                        const struct dj_cplx_@S@ us = d_cplx_@S@_div_real(u, s);
                        const struct dj_cplx_@S@ sq = d_cplx_@S@_add(
                            d_internal_cmat_@S@_mul(xs, xs), d_internal_cmat_@S@_mul(us, us));

                        y = d_cplx_@S@_mul_real(
                            d_internal_cmat_@S@_sqrt@F@(sq), s);
                    }

                    // y's sign: the root nearer x
                    if (sx > (@T@)0)
                    {
                        const struct dj_cplx_@S@ xn = d_cplx_@S@_div_real(x, sx);
                        const @T@ p1  = xn.part[0] * y.part[0];
                        const @T@ p2  = xn.part[1] * y.part[1];
                        const @T@ dot = p1 + p2;

                        if (dot < (@T@)0)
                        {
                            y = d_cplx_@S@_neg(y);
                        }
                    }

                    t = d_internal_cmat_@S@_less_product(
                        t, u, d_internal_cmat_@S@_div(u, d_cplx_@S@_add(x, y)));
                }
            }

            // the row the step starts at: two small subdiagonal entries
            for (m = i - 1u; m > l; --m)
            {
                const struct dj_cplx_@S@ h11  = _h[(m * _n) + m];
                const struct dj_cplx_@S@ h22  = _h[((m + 1u) * _n) + (m + 1u)];
                struct dj_cplx_@S@       h11s = d_cplx_@S@_sub(h11, t);
                @T@                      h21  = _h[((m + 1u) * _n) + m].part[0];
                const @T@                h10  = _h[(m * _n) + (m - 1u)].part[0];
                @T@                      s    = (@T@)0;

                {
                    const @T@ a1 = d_internal_cmat_@S@_abs1(h11s);
                    const @T@ a2 = d_internal_math_@S@_abs(h21);

                    s = a1 + a2;
                }

                h11s = d_cplx_@S@_div_real(h11s, s);
                h21  = h21 / s;
                v[0] = h11s;
                v[1] = d_cplx_@S@_make(h21, (@T@)0);

                {
                    const @T@ a10 = d_internal_math_@S@_abs(h10);
                    const @T@ a21 = d_internal_math_@S@_abs(h21);
                    const @T@ lhs = a10 * a21;
                    const @T@ e1  = d_internal_cmat_@S@_abs1(h11s);
                    const @T@ e2  = d_internal_cmat_@S@_abs1(h11);
                    const @T@ e3  = d_internal_cmat_@S@_abs1(h22);
                    const @T@ e23 = e2 + e3;
                    const @T@ p   = e1 * e23;
                    const @T@ rhs = ulp * p;

                    // starting here leaves H(m, m - 1) negligible
                    if (lhs <= rhs)
                    {
                        break;
                    }
                }
            }

            // the top of the block
            if (m == l)
            {
                const struct dj_cplx_@S@ h11  = _h[(l * _n) + l];
                struct dj_cplx_@S@       h11s = d_cplx_@S@_sub(h11, t);
                @T@                      h21  = _h[((l + 1u) * _n) + l].part[0];
                @T@                      s    = (@T@)0;

                {
                    const @T@ a1 = d_internal_cmat_@S@_abs1(h11s);
                    const @T@ a2 = d_internal_math_@S@_abs(h21);

                    s = a1 + a2;
                }

                h11s = d_cplx_@S@_div_real(h11s, s);
                h21  = h21 / s;
                v[0] = h11s;
                v[1] = d_cplx_@S@_make(h21, (@T@)0);
            }

            // the single-shift step: the bulge made, then chased out
            for (k = m; k < i; ++k)
            {
                struct dj_cplx_@S@ t1   = zero;
                struct dj_cplx_@S@ v2   = zero;
                @T@                t2   = (@T@)0;
                size_t             jj   = 0u;
                size_t             last = ((k + 2u) < i) ? (k + 2u) : i;

                // past the first: the bulge's column, to be reflected
                if (k > m)
                {
                    v[0] = _h[(k * _n) + (k - 1u)];
                    v[1] = _h[((k + 1u) * _n) + (k - 1u)];
                }

                t1 = d_internal_cmat_@S@_reflector@F@(&v[0], &v[1], 1u, 1u);

                if (k > m)
                {
                    _h[(k * _n) + (k - 1u)]        = v[0];
                    _h[((k + 1u) * _n) + (k - 1u)] = zero;
                }

                v2 = v[1];
                t2 = d_internal_cmat_@S@_mul(t1, v2).part[0];

                // from the left: rows k and k + 1, columns k to i2
                for (jj = k; jj <= i2; ++jj)
                {
                    const struct dj_cplx_@S@ sum = d_cplx_@S@_add(
                        d_internal_cmat_@S@_mul(d_cplx_@S@_conj(t1),
                                       _h[(k * _n) + jj]),
                        d_cplx_@S@_mul_real(_h[((k + 1u) * _n) + jj], t2));

                    _h[(k * _n) + jj] = d_cplx_@S@_sub(_h[(k * _n) + jj], sum);
                    _h[((k + 1u) * _n) + jj] = d_internal_cmat_@S@_less_product(
                        _h[((k + 1u) * _n) + jj], sum, v2);
                }

                // from the right: rows i1 to min(k + 2, i)
                for (jj = i1; jj <= last; ++jj)
                {
                    const struct dj_cplx_@S@ sum = d_cplx_@S@_add(
                        d_internal_cmat_@S@_mul(t1, _h[(jj * _n) + k]),
                        d_cplx_@S@_mul_real(_h[(jj * _n) + (k + 1u)], t2));

                    _h[(jj * _n) + k] = d_cplx_@S@_sub(_h[(jj * _n) + k], sum);
                    _h[(jj * _n) + (k + 1u)] = d_internal_cmat_@S@_less_product(
                        _h[(jj * _n) + (k + 1u)], sum, d_cplx_@S@_conj(v2));
                }

                // Z's columns
                if (_z != NULL)
                {
                    for (jj = 0u; jj < _n; ++jj)
                    {
                        const struct dj_cplx_@S@ sum = d_cplx_@S@_add(
                            d_internal_cmat_@S@_mul(t1, _z[(jj * _n) + k]),
                            d_cplx_@S@_mul_real(_z[(jj * _n) + (k + 1u)], t2));

                        _z[(jj * _n) + k] = d_cplx_@S@_sub(_z[(jj * _n) + k],
                                                           sum);
                        _z[(jj * _n) + (k + 1u)] =
                            d_internal_cmat_@S@_less_product(
                                _z[(jj * _n) + (k + 1u)], sum,
                                d_cplx_@S@_conj(v2));
                    }
                }

                // a step begun below the top: H(m, m - 1) kept real
                if ( (k == m) &&
                     (m > l) )
                {
                    const struct dj_cplx_@S@ one_less = d_cplx_@S@_sub(
                        d_cplx_@S@_make((@T@)1, (@T@)0), t1);
                    const struct dj_cplx_@S@ temp = d_cplx_@S@_div_real(
                        one_less, d_internal_cmat_@S@_abs@F@(one_less));
                    const struct dj_cplx_@S@ ctemp = d_cplx_@S@_conj(temp);

                    _h[((m + 1u) * _n) + m] =
                        d_internal_cmat_@S@_mul(_h[((m + 1u) * _n) + m], ctemp);

                    if ((m + 2u) <= i)
                    {
                        _h[((m + 2u) * _n) + (m + 1u)] =
                            d_internal_cmat_@S@_mul(_h[((m + 2u) * _n) + (m + 1u)],
                                           temp);
                    }

                    for (jj = m; jj <= i; ++jj)
                    {
                        if (jj != (m + 1u))
                        {
                            if (i2 > jj)
                            {
                                d_internal_cmat_@S@_turn(
                                    _h + ((jj * _n) + (jj + 1u)), i2 - jj,
                                    1u, temp);
                            }

                            d_internal_cmat_@S@_turn(_h + ((i1 * _n) + jj),
                                                     jj - i1, _n, ctemp);

                            if (_z != NULL)
                            {
                                d_internal_cmat_@S@_turn(_z + jj, _n, _n,
                                                         ctemp);
                            }
                        }
                    }
                }
            }

            // H(i, i - 1) kept real
            {
                const struct dj_cplx_@S@ temp0 = _h[(i * _n) + (i - 1u)];

                if (temp0.part[1] != (@T@)0)
                {
                    const @T@ rtemp = d_internal_cmat_@S@_abs@F@(temp0);
                    const struct dj_cplx_@S@ temp =
                        d_cplx_@S@_div_real(temp0, rtemp);

                    _h[(i * _n) + (i - 1u)] = d_cplx_@S@_make(rtemp, (@T@)0);

                    if (i2 > i)
                    {
                        d_internal_cmat_@S@_turn(_h + ((i * _n) + (i + 1u)),
                                                 i2 - i, 1u,
                                                 d_cplx_@S@_conj(temp));
                    }

                    d_internal_cmat_@S@_turn(_h + ((i1 * _n) + i), i - i1,
                                             _n, temp);

                    if (_z != NULL)
                    {
                        d_internal_cmat_@S@_turn(_z + i, _n, _n, temp);
                    }
                }
            }
        }

        // one eigenvalue
        _values[i] = _h[(i * _n) + i];
        kdefl      = 0u;
        top        = l;
    }

    return true;
}

// d_cmat_@S@_schur_vectors@F@
//   the right eigenvectors of the n-by-n upper triangular complex Schur
// form _t, back-transformed by the Schur vectors, as LAPACK's ztrevc with
// job 'B': _vr holds Z on entry, and on exit the eigenvectors of Z T Z^H,
// column for column, each scaled to a largest |re| + |im| of 1. From the
// last up, each by back-substitution, (T - t_kk I) x = 0 with x_k = 1: a
// divisor smaller than ulp |t_kk| taken as that, and x scaled down wherever
// the next step could overflow, as LAPACK's dtrevc does for its blocks of
// one; then Z x. _x (n entries) and _norms (n) are scratch.
D_MATH_FN_KERNEL@RT@ void
d_cmat_@S@_schur_vectors@F@(
    const struct dj_cplx_@S@* _t,
    size_t                    _n,
    struct dj_cplx_@S@*       _vr,
    struct dj_cplx_@S@*       _x,
    @T@*                      _norms
)
{
    const @T@ ulp    = @EPS@;
    const @T@ smlnum = @MIN@ * ((@T@)_n / ulp);
    const @T@ bignum = ((@T@)1 - ulp) / smlnum;
    size_t    j      = 0u;
    size_t    k      = 0u;
    size_t    r      = 0u;
    size_t    top    = 0u;

    // nothing to find
    if (_n == 0u)
    {
        return;
    }

    // each column's |re| + |im| sum above the diagonal, to foresee overflow
    _norms[0] = (@T@)0;

    for (j = 1u; j < _n; ++j)
    {
        _norms[j] = (@T@)0;

        for (k = 0u; k < j; ++k)
        {
            _norms[j] = _norms[j] +
                        d_internal_cmat_@S@_abs1(_t[(k * _n) + j]);
        }
    }

    for (top = _n; top > 0u; --top)
    {
        const size_t             ki   = top - 1u;
        const struct dj_cplx_@S@ w    = _t[(ki * _n) + ki];
        @T@                      smin = (@T@)0;
        size_t                   jj   = 0u;

        {
            const @T@ u = ulp * d_internal_cmat_@S@_abs1(w);

            smin = (u > smlnum) ? u : smlnum;
        }

        _x[ki] = d_cplx_@S@_make((@T@)1, (@T@)0);

        for (k = 0u; k < ki; ++k)
        {
            _x[k] = d_cplx_@S@_neg(_t[(k * _n) + ki]);
        }

        // (T(0..ki-1, 0..ki-1) - w) x = scale x, the last row first
        for (jj = ki; jj > 0u; --jj)
        {
            const size_t       jr    = jj - 1u;
            struct dj_cplx_@S@ c     = d_cplx_@S@_sub(_t[(jr * _n) + jr], w);
            struct dj_cplx_@S@ xj    = _x[jr];
            @T@                scale = (@T@)1;
            @T@                cnorm = d_internal_cmat_@S@_abs1(c);
            const @T@          bnorm = d_internal_cmat_@S@_abs1(xj);
            @T@                xnorm = (@T@)0;

            // a divisor below smin: smin
            if (cnorm < smin)
            {
                c     = d_cplx_@S@_make(smin, (@T@)0);
                cnorm = smin;
            }

            // b / c would overflow: scaled
            if ( (cnorm < (@T@)1) &&
                 (bnorm > (@T@)1) )
            {
                const @T@ bound = bignum * cnorm;

                if (bnorm > bound)
                {
                    scale = (@T@)1 / bnorm;
                }
            }

            xj    = d_internal_cmat_@S@_div(d_cplx_@S@_mul_real(xj, scale), c);
            xnorm = d_internal_cmat_@S@_abs1(xj);

            // x scaled down where the update could overflow
            if (xnorm > (@T@)1)
            {
                const @T@ bound = bignum / xnorm;

                if (_norms[jr] > bound)
                {
                    xj    = d_cplx_@S@_div_real(xj, xnorm);
                    scale = scale / xnorm;
                }
            }

            if (scale != (@T@)1)
            {
                for (k = 0u; k <= ki; ++k)
                {
                    _x[k] = d_cplx_@S@_mul_real(_x[k], scale);
                }
            }

            _x[jr] = xj;

            for (k = 0u; k < jr; ++k)
            {
                _x[k] = d_internal_cmat_@S@_less_product(
                    _x[k], xj, _t[(k * _n) + jr]);
            }
        }

        // Z x, into the column
        if (ki > 0u)
        {
            const struct dj_cplx_@S@ beta = _x[ki];
            const bool beta_zero = d_internal_cmat_@S@_is_zero(beta);
            const bool beta_one  = ( (beta.part[0] == (@T@)1) &&
                                     (beta.part[1] == (@T@)0) );

            for (r = 0u; r < _n; ++r)
            {
                if (beta_zero)
                {
                    _vr[(r * _n) + ki] = d_cplx_@S@_make((@T@)0, (@T@)0);
                }
                else if (!beta_one)
                {
                    _vr[(r * _n) + ki] = d_internal_cmat_@S@_mul(beta,
                                                        _vr[(r * _n) + ki]);
                }
            }

            for (k = 0u; k < ki; ++k)
            {
                const struct dj_cplx_@S@ temp = _x[k];

                for (r = 0u; r < _n; ++r)
                {
                    _vr[(r * _n) + ki] = d_cplx_@S@_add(
                        _vr[(r * _n) + ki],
                        d_internal_cmat_@S@_mul(temp, _vr[(r * _n) + k]));
                }
            }
        }

        // the largest |re| + |im| 1
        {
            size_t best  = 0u;
            @T@    remax = (@T@)0;

            for (r = 1u; r < _n; ++r)
            {
                if (d_internal_cmat_@S@_abs1(_vr[(r * _n) + ki]) >
                    d_internal_cmat_@S@_abs1(_vr[(best * _n) + ki]))
                {
                    best = r;
                }
            }

            remax = (@T@)1 / d_internal_cmat_@S@_abs1(_vr[(best * _n) + ki]);

            for (r = 0u; r < _n; ++r)
            {
                _vr[(r * _n) + ki] = d_cplx_@S@_mul_real(_vr[(r * _n) + ki],
                                                         remax);
            }
        }
    }

    return;
}

// d_internal_cmat_@S@_unit_vectors@F@
//   the eigenvectors in the columns of the n-by-n _v made of unit length,
// in place, each turned so that its entry of largest modulus (the first
// among equals) is real and positive.
D_MATH_FN_KERNEL@RT@ void
d_internal_cmat_@S@_unit_vectors@F@(
    struct dj_cplx_@S@* _v,
    size_t              _n
)
{
    size_t j = 0u;
    size_t r = 0u;

    for (j = 0u; j < _n; ++j)
    {
        size_t best    = 0u;
        @T@    largest = (@T@)0;
        @T@    modulus = (@T@)0;
        @T@    norm    = (@T@)0;

        for (r = 0u; r < _n; ++r)
        {
            const @T@ a  = _v[(r * _n) + j].part[0];
            const @T@ b  = _v[(r * _n) + j].part[1];
            const @T@ aa = a * a;
            const @T@ bb = b * b;
            const @T@ m  = aa + bb;

            if ( (r == 0u) ||
                 (m > largest) )
            {
                largest = m;
                best    = r;
            }
        }

        modulus = d_internal_cmat_@S@_abs@F@(_v[(best * _n) + j]);

        // turned so that the largest entry is real and positive
        {
            const @T@ ur = _v[(best * _n) + j].part[0] / modulus;
            const @T@ ui = _v[(best * _n) + j].part[1] / modulus;
            const struct dj_cplx_@S@ u = d_cplx_@S@_make(ur, -ui);

            for (r = 0u; r < _n; ++r)
            {
                _v[(r * _n) + j] = (r == best)
                    ? d_cplx_@S@_make(modulus, (@T@)0)
                    : d_internal_cmat_@S@_mul(_v[(r * _n) + j], u);
            }
        }

        norm = d_internal_decomp_@S@_cnorm@F@(_v + j, _n, _n);

        for (r = 0u; r < _n; ++r)
        {
            _v[(r * _n) + j] = d_cplx_@S@_div_real(_v[(r * _n) + j], norm);
        }

        _v[(best * _n) + j].part[1] = (@T@)0;
    }

    return;
}

// d_cmat_@S@_eigen@F@
//   the eigenvalues of the complex n-by-n _a and, unless _vectors is NULL,
// an eigenvector for each, as LAPACK's zgeev finds them, and decomp.h's
// d_decomp_@S@_eigen@F@ a real matrix's: _a scaled by a power of two to a
// largest part in [1, 2), exactly; balanced (d_cmat_@S@_balance@F@);
// reduced to Hessenberg form (d_cmat_@S@_hessenberg_q@F@); its complex
// Schur form found by single-shift QR (d_cmat_@S@_schur@F@), at most
// _max_iter steps, _tol its deflation's tolerance (the epsilon where
// smaller); and each eigenvector from the Schur form by back-substitution
// (d_cmat_@S@_schur_vectors@F@), back-transformed, of unit length with its
// entry of largest modulus real and positive (the first among equals).
// _values receives the eigenvalues, ascending by real part, then by
// imaginary part, and column j of the row-major n-by-n _vectors the
// eigenvector of _values[j]. _a is consumed; _work is complex scratch of n
// (n + 1) entries, unused where _vectors is NULL; _rwork real scratch of 2 n
// entries, or n where _vectors is NULL; _perm of n, unused where _vectors is
// NULL. Returns whether every eigenvalue was found. If not, those found are
// given and the rest the diagonal's entries, and every entry of _vectors is
// a NaN; a NaN or an infinity in _a gives every eigenvalue and entry a NaN,
// and returns false; the zero matrix has zero eigenvalues and the
// identity's columns.
D_MATH_FN_KERNEL@RT@ bool
d_cmat_@S@_eigen@F@(
    struct dj_cplx_@S@* _a,
    size_t              _n,
    struct dj_cplx_@S@* _values,
    struct dj_cplx_@S@* _vectors,
    struct dj_cplx_@S@* _work,
    @T@*                _rwork,
    size_t*             _perm,
    size_t              _max_iter,
    @T@                 _tol
)
{
    const struct dj_internal_decomp_@S@_scaling sc =
        d_internal_cmat_@S@_scaling_of(_a, _n * _n);
    const struct dj_cplx_@S@ not_a_number =
        d_cplx_@S@_make(d_internal_math_@S@_nan(), d_internal_math_@S@_nan());
    @T@*                scale     = _rwork;
    struct dj_cplx_@S@* z         = NULL;
    size_t              ilo       = 0u;
    size_t              ihi       = 0u;
    size_t              i         = 0u;
    size_t              k         = 0u;
    bool                converged = true;

    // nothing to find
    if (_n == 0u)
    {
        return true;
    }

    // a NaN or an infinity: no eigenvalue, no eigenvector
    if (!sc.finite)
    {
        for (i = 0u; i < _n; ++i)
        {
            _values[i] = not_a_number;
        }

        if (_vectors != NULL)
        {
            for (i = 0u; i < (_n * _n); ++i)
            {
                _vectors[i] = not_a_number;
            }
        }

        return false;
    }

    // the zero matrix: zeros, and the identity's columns
    if (sc.zero)
    {
        for (i = 0u; i < _n; ++i)
        {
            _values[i] = d_cplx_@S@_make((@T@)0, (@T@)0);
        }

        if (_vectors != NULL)
        {
            for (i = 0u; i < _n; ++i)
            {
                for (k = 0u; k < _n; ++k)
                {
                    _vectors[(i * _n) + k] =
                        d_cplx_@S@_make((i == k) ? (@T@)1 : (@T@)0, (@T@)0);
                }
            }
        }

        return true;
    }

    // the largest part into [1, 2), exactly
    for (i = 0u; i < (_n * _n); ++i)
    {
        size_t p = 0u;

        for (p = 0u; p < 2u; ++p)
        {
            const @T@ lifted = _a[i].part[p] * sc.pre;

            _a[i].part[p] = lifted / sc.power;
        }
    }

    d_cmat_@S@_balance@F@(_a, _n, _perm, scale, &ilo, &ihi);

    // Q, then Z, from the identity
    if (_vectors != NULL)
    {
        z = _work;

        for (i = 0u; i < _n; ++i)
        {
            for (k = 0u; k < _n; ++k)
            {
                z[(i * _n) + k] =
                    d_cplx_@S@_make((i == k) ? (@T@)1 : (@T@)0, (@T@)0);
            }
        }
    }

    d_cmat_@S@_hessenberg_q@F@(_a, _n, ilo, ihi, z);

    converged = d_cmat_@S@_schur@F@(_a, _n, ilo, ihi, z, _values, _max_iter,
                                    _tol);

    // the eigenvectors, or NaNs where the iteration stopped short
    if (_vectors != NULL)
    {
        if (converged)
        {
            d_cmat_@S@_schur_vectors@F@(_a, _n, z, z + (_n * _n),
                                        _rwork + _n);
            d_cmat_@S@_balance_back(z, _n, _n, _perm, scale, ilo, ihi);

            for (i = 0u; i < (_n * _n); ++i)
            {
                _vectors[i] = z[i];
            }

            d_internal_cmat_@S@_unit_vectors@F@(_vectors, _n);
        }
        else
        {
            for (i = 0u; i < (_n * _n); ++i)
            {
                _vectors[i] = not_a_number;
            }
        }
    }

    // back to the matrix's scale: exact unless a value overflows or
    // underflows
    for (i = 0u; i < _n; ++i)
    {
        const @T@ re = _values[i].part[0] * sc.power;
        const @T@ im = _values[i].part[1] * sc.power;

        _values[i].part[0] = re / sc.pre;
        _values[i].part[1] = im / sc.pre;
    }

    // ascending by real part, then imaginary, the vectors' columns along
    for (i = 0u; i < _n; ++i)
    {
        size_t sel = i;

        for (k = i + 1u; k < _n; ++k)
        {
            const bool lower = (_values[k].part[0] < _values[sel].part[0]);
            const bool level = (_values[k].part[0] == _values[sel].part[0]);
            const bool below = (_values[k].part[1] < _values[sel].part[1]);

            if ( (lower) ||
                 ((level) && (below)) )
            {
                sel = k;
            }
        }

        if (sel != i)
        {
            const struct dj_cplx_@S@ tmp = _values[i];

            _values[i]   = _values[sel];
            _values[sel] = tmp;

            if (_vectors != NULL)
            {
                for (k = 0u; k < _n; ++k)
                {
                    const struct dj_cplx_@S@ t = _vectors[(k * _n) + i];

                    _vectors[(k * _n) + i]   = _vectors[(k * _n) + sel];
                    _vectors[(k * _n) + sel] = t;
                }
            }
        }
    }

    return converged;
}
"""

CMAT_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE FLOAT FAMILY (_f)
    ---------------------
    1.  LU, determinant, inverse and solution
    2.  Hermitian matrices
    3.  The eigenvalues and eigenvectors of a general matrix
2.  THE DOUBLE FAMILY (_d)
    ----------------------
    1.  LU, determinant, inverse and solution
    2.  Hermitian matrices
    3.  The eigenvalues and eigenvectors of a general matrix
3.  THE LONG DOUBLE FAMILY (_ld)
    ----------------------------
    1.  LU, determinant, inverse and solution
    2.  Hermitian matrices
    3.  The eigenvalues and eigenvectors of a general matrix
4.  ONE NAME FOR EVERY FAMILY (C++)
    -------------------------------
*/

#ifndef DJINTERP_C_MATH_CMAT_H
#define DJINTERP_C_MATH_CMAT_H 1

// std
#include <float.h>           // FLT_EPSILON ... LDBL_MAX: the families'
                             // limits
#include <stddef.h>          // size_t
// djinterp
#include "./cplx.h"          // struct dj_cplx_*, its arithmetic
#include "./decomp.h"        // d_internal_decomp_*_hypot, _cnorm and
                             // _scaling: the real helpers
#include "./elementary.h"    // d_math_*_sqrt and its _c form,
                             // d_internal_math_*_abs, d_internal_math_*_nan
#include "./float_order.h"   // d_internal_math_*_binade_of,
                             // d_internal_math_*_pow2
#include "./math_common.h"   // D_MATH_FN, D_MATH_FN_KERNEL
"""

CMAT_TAIL = r"""

#endif  // DJINTERP_C_MATH_CMAT_H
"""

CMAT_OVERLOADS = [
    ("D_MATH_FN_KERNEL", "bool", "d_cmat_lu", [("struct dj_cplx_@S@*", "_lu"), ("size_t", "_n"), ("size_t*", "_perm"), ("@T@*", "_sign")]),
    ("D_MATH_FN_KERNEL", "void", "d_cmat_lu_solve", [("const struct dj_cplx_@S@*", "_lu"), ("const size_t*", "_perm"), ("size_t", "_n"), ("const struct dj_cplx_@S@*", "_b"), ("struct dj_cplx_@S@*", "_x")]),
    ("D_MATH_FN", "struct dj_cplx_@S@", "d_cmat_lu_det", [("const struct dj_cplx_@S@*", "_lu"), ("size_t", "_n"), ("@T@", "_sign")]),
    ("D_MATH_FN_KERNEL", "struct dj_cplx_@S@", "d_cmat_determinant", [("const struct dj_cplx_@S@*", "_a"), ("struct dj_cplx_@S@*", "_scratch"), ("size_t", "_n")]),
    ("D_MATH_FN_KERNEL", "bool", "d_cmat_inverse", [("struct dj_cplx_@S@*", "_out"), ("struct dj_cplx_@S@*", "_scratch"), ("const struct dj_cplx_@S@*", "_a"), ("size_t", "_n")]),
    ("D_MATH_FN_KERNEL", "bool", "d_cmat_solve", [("struct dj_cplx_@S@*", "_x"), ("struct dj_cplx_@S@*", "_scratch"), ("const struct dj_cplx_@S@*", "_a"), ("const struct dj_cplx_@S@*", "_b"), ("size_t", "_n"), ("size_t", "_m")]),
    ("D_MATH_FN_KERNEL", "void", "d_cmat_balance_back", [("struct dj_cplx_@S@*", "_v"), ("size_t", "_n"), ("size_t", "_m"), ("const size_t*", "_perm"), ("const @T@*", "_scale"), ("size_t", "_ilo"), ("size_t", "_ihi")]),
]
for _f, _m in (("_c", "D_MATH_FN_KERNEL"), ("", "D_MATH_FN_KERNEL_RT")):
    CMAT_OVERLOADS += [
        (_m, "void", "d_cmat_hermitian_eigen" + _f, [("struct dj_cplx_@S@*", "_work"), ("@T@*", "_values"), ("struct dj_cplx_@S@*", "_vectors"), ("size_t", "_n"), ("@T@", "_threshold")]),
        (_m, "void", "d_cmat_balance" + _f, [("struct dj_cplx_@S@*", "_a"), ("size_t", "_n"), ("size_t*", "_perm"), ("@T@*", "_scale"), ("size_t*", "_ilo"), ("size_t*", "_ihi")]),
        (_m, "void", "d_cmat_hessenberg_q" + _f, [("struct dj_cplx_@S@*", "_h"), ("size_t", "_n"), ("size_t", "_ilo"), ("size_t", "_ihi"), ("struct dj_cplx_@S@*", "_q")]),
        (_m, "bool", "d_cmat_schur" + _f, [("struct dj_cplx_@S@*", "_h"), ("size_t", "_n"), ("size_t", "_ilo"), ("size_t", "_ihi"), ("struct dj_cplx_@S@*", "_z"), ("struct dj_cplx_@S@*", "_values"), ("size_t", "_max_iter"), ("@T@", "_tol")]),
        (_m, "void", "d_cmat_schur_vectors" + _f, [("const struct dj_cplx_@S@*", "_t"), ("size_t", "_n"), ("struct dj_cplx_@S@*", "_vr"), ("struct dj_cplx_@S@*", "_x"), ("@T@*", "_norms")]),
        (_m, "bool", "d_cmat_eigen" + _f, [("struct dj_cplx_@S@*", "_a"), ("size_t", "_n"), ("struct dj_cplx_@S@*", "_values"), ("struct dj_cplx_@S@*", "_vectors"), ("struct dj_cplx_@S@*", "_work"), ("@T@*", "_rwork"), ("size_t*", "_perm"), ("size_t", "_max_iter"), ("@T@", "_tol")]),
    ]


def cmat():
    lines = [
        "Complex matrices: the C core of complex linear algebra (S269) -- LU",
        "with partial pivoting and its solve and determinant, a complex",
        "matrix's determinant, inverse and solution, a Hermitian matrix's",
        "eigenvalues and eigenvectors by Jacobi's method with complex",
        "rotations, and a general complex matrix's by LAPACK's path:",
        "balancing, the Hessenberg form, the complex Schur form by",
        "single-shift QR, the eigenvectors from it -- for float, double and",
        "long double, on cplx.h's complex numbers.",
        "  GENERATED by scripts/gen_c_math_float.py from one text for the three",
        "families: edit the generator, not this file.",
        "",
        "  Matrices are row-major arrays of struct dj_cplx_f, _d or _ld. What",
        "needs no root comes once, a constant expression from C++14; what",
        "takes a root comes twice, as decomp.h's kernels do, the _c form a",
        "constant expression from C++14 and the plain one on the C library's",
        "root, the same bit for bit, their roots correctly rounded both.",
    ]
    text = banner("cmat.h", lines, "2026.10.29", "2026.11.07")
    text += "\n" + CMAT_HEAD
    for i, (suffix, ctype, prefix, prose) in enumerate(FAMILIES):
        body = CMAT_FAMILY_HEAD + CMAT_LU + CMAT_HERMITIAN_HEAD
        for f, rt in (("_c", ""), ("", "_RT")):
            body += CMAT_HERMITIAN_FORMS.replace("@F@", f).replace("@RT@",
                                                                    rt)
        body += CMAT_GENERAL_HEAD + CMAT_GENERAL_ONCE
        for f, rt in (("_c", ""), ("", "_RT")):
            body += CMAT_GENERAL_FORMS.replace("@F@", f).replace("@RT@", rt)
        body += DECOMP_POP
        body = body.replace("@SECTION@", str(i + 1))
        body = body.replace("@UPPER@", prose.upper())
        text += instantiate(body, suffix, ctype, prefix, prose)
    text += overload_section("4.  ONE NAME FOR EVERY FAMILY (C++)",
                             CMAT_OVERLOADS)
    text += CMAT_TAIL
    return text


OUTPUTS = {
    "float_order.h": float_order,
    "interval_float.h": interval_float,
    "elementary.h": elementary,
    "exponential.h": exponential,
    "tangent.h": tangent,
    "hyperbolic.h": hyperbolic,
    "power.h": power,
    "remainder.h": remainder,
    "nearest_integer.h": nearest_integer,
    "vec.h": vec,
    "mat.h": mat,
    "quat.h": quat,
    "coord.h": coord,
    "isect.h": isect,
    "plane.h": plane,
    "decomp.h": decomp,
    "cplx.h": cplx,
    "cmat.h": cmat,
}


def main(argv):
    check = "--check" in argv
    status = 0
    for name, make in OUTPUTS.items():
        rel = CORE + "/" + name
        path = os.path.join(ROOT, rel)
        text = make()
        if check:
            try:
                with open(path) as f:
                    current = f.read()
            except FileNotFoundError:
                current = None
            if current != text:
                print("differs: " + rel)
                status = 1
        else:
            with open(path, "w") as f:
                f.write(text)
            print("wrote " + rel)
    return status


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
