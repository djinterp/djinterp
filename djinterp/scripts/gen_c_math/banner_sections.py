"""gen_c_math.banner_sections - the three sections the style guides ask of
every header's banner (ruled 2026.11.17), HOW TO USE, OPTIONS and
CONFIGURATION, for each header the script writes (chunk 62, S473's
alternative as the owner ruled it). common.banner adds them under the
description; a part of a split header (S436) takes PART's, naming its
header. A header with no entry here is refused, so that a new header must
say how it is used.

One module of scripts/gen_c_math_float.py, which runs them all; see it."""

NONE_RUN = "None: nothing in this header is chosen at run time."
FAMILY = ("Each name comes for each family, _f, _d and _ld after d_math_, "
          "as d_math_d_sqrt; in C++ the family's own name, d_math_sqrt, is "
          "overloaded on the three.")
TWICE = ("Which form runs is the caller's, by name: the _c form, a "
         "constant expression, or the plain one on the C library; nothing "
         "is chosen at run time.")
BUILD = ("it compiles as C, and as C++ from ISO strict C++98; its _c "
         "forms and its pure kernels are constant expressions from C++14 "
         "(math_common.h's D_MATH_FN).")
LEVELS = "None of its own: " + BUILD
# a header with no _c forms, its functions pure arithmetic
PURE = ("None of its own: it compiles as C, and as C++ from ISO strict "
        "C++98; its functions are constant expressions from C++14 "
        "(math_common.h's D_MATH_FN).")
WIDE = (" Where double is evaluated wider than double, a 32-bit x87, the "
        "double family's correctly rounded forms need -msse2 -mfpmath=sse "
        "(math_common.h's D_MATH_DBL_EVAL_WIDE).")
REFINE = ("One knob, D_CFG_MATH_MAT_REFINE_STEPS "
          "(config/c/modules/math/cfg_mat.h): how many steps the solve "
          "refines a solution, 10 by default, at least 1 (S407). The header "
          + BUILD[len("it "):])

SECTIONS = {
    "dd.h": (
        "Included by the double family's correctly rounded functions; a "
        "caller may include it from C or C++ for the pairs themselves: "
        "d_math_dd_two_sum and d_math_dd_two_product, the exact sum and "
        "product of two doubles, then d_math_dd_add, _sub, _mul, _div and "
        "_sqrt of pairs, d_math_dd_of to make one and d_math_dd_value to "
        "round one to a double.",
        NONE_RUN,
        "None of its own: every function is a constant expression from "
        "C++14. Its bounds need double evaluated in double: on a 32-bit "
        "x87, build with -msse2 -mfpmath=sse."),
    "self_test.h": (
        "Include it from C or C++ and call d_math_self_test() once, at "
        "start-up: it says whether this translation unit's build keeps the "
        "double family's guarantees. d_math_formulas_fuse() says whether "
        "the build fuses a multiply and an add, which changes the plain "
        "formulas' bits and the _fast forms', never the correctly rounded "
        "functions'.",
        "None: what it checks is fixed; what to do on a failure is the "
        "caller's.",
        "None of its own: it checks the build it is compiled in, such "
        "flags as -ffast-math among what it can catch."),
    "float_order.h": (
        "Include it from C or C++. d_math_d_next_up and _next_down give a "
        "double's neighbours, d_math_d_steps the count of values between "
        "two and d_math_d_advance the value some steps away, with _f and "
        "_ld beside them; interval_float.h walks its intervals by them.",
        NONE_RUN,
        "None of its own: it is pure arithmetic, a constant expression "
        "from C++14. It assumes subnormals: a build that flushes them to "
        "zero sees gaps the order does not know."),
    "interval_float.h": (
        "Include it from C or C++, or through the C++ interval templates "
        "with a floating-point value type. Make an interval with "
        "d_interval_d_make, its bounds, their openness and its step, then "
        "ask it what it contains and how it meets another, as interval.h's "
        "integer families do; _f and _ld beside it.",
        "None: an interval's bounds, openness and step are its arguments.",
        "None of its own: every kernel but formatting is a constant "
        "expression from C++14; it assumes subnormals, as float_order.h "
        "does."),
    "elementary.h": (
        "Include it from C or C++. d_math_d_sqrt, _asin, _acos, _sin and "
        "_cos call the C library; d_math_d_sqrt_c and the other _c forms "
        "compute the same in pure arithmetic. " + FAMILY,
        TWICE,
        LEVELS + " A long double wider than the reduction by pi/2 was "
        "written for is refused at compile time."),
    "exponential.h": (
        "Include it from C or C++; it includes its five parts. "
        "d_math_d_exp, _log, _exp2, _log2, _log10, _log1p and _expm1 call "
        "the C library, but for d_math_d_exp, which is d_math_d_exp_c, "
        "correctly rounded, wherever double is evaluated in double (S478), "
        "and d_math_d_exp_fast, e^x within 0.511 of an ulp at the C "
        "library's time (S490); their _c forms compute the same in pure "
        "arithmetic, the double family's exp_c and log_c correctly rounded. "
        + FAMILY,
        "Which form runs is the caller's, by name: the _c form, a constant "
        "expression, the plain one on the C library, or for double's "
        "exponential the fast one; nothing is chosen at run time.",
        LEVELS + WIDE),
    "tangent.h": (
        "Include it from C or C++. d_math_d_tan, _atan and _atan2 call the "
        "C library; their _c forms compute the same in pure arithmetic. "
        + FAMILY,
        TWICE,
        LEVELS),
    "hyperbolic.h": (
        "Include it from C or C++. d_math_d_sinh, _cosh, _tanh, _asinh, "
        "_acosh and _atanh call the C library; their _c forms compute the "
        "same in pure arithmetic. " + FAMILY,
        TWICE,
        LEVELS),
    "power.h": (
        "Include it from C or C++. d_math_d_cbrt, _pow and _hypot call "
        "the C library; their _c forms compute the same in pure "
        "arithmetic, cbrt_c and hypot_c correctly rounded. " + FAMILY,
        TWICE,
        LEVELS),
    "remainder.h": (
        "Include it from C or C++. d_math_d_fmod, _remainder and _remquo, "
        "and their _c forms, agree bit for bit; the plain ones call the C "
        "library. " + FAMILY,
        "None: the two forms give the same bits, so which runs changes "
        "nothing.",
        LEVELS),
    "nearest_integer.h": (
        "Include it from C or C++. d_math_d_floor, _ceil, _trunc and "
        "_round, and their _c forms, agree bit for bit, a zero's sign "
        "included; the plain ones call the C library. " + FAMILY,
        "None: the two forms give the same bits, so which runs changes "
        "nothing.",
        LEVELS),
    "scalar.h": (
        "Include it from C or C++. d_math_d_abs, _copysign and _is_finite, "
        "d_math_d_min and _max, IEEE 754-2019's minimum and maximum, "
        "d_math_d_min_num and _max_num, which pass a NaN over, "
        "d_math_d_clamp, d_math_d_lerp, and d_math_d_ldexp and _frexp; "
        "abs, copysign, is_finite, ldexp and frexp have _c forms too. "
        + FAMILY,
        "None: nothing in this header is chosen at run time; the _c form "
        "or the plain one is the caller's, by name, and both give the same "
        "bits.",
        "None of its own: it compiles as C, and as C++ from ISO strict "
        "C++98, where copysign and is_finite are the _c forms; every "
        "function but the plain forms is a constant expression from C++14 "
        "(math_common.h's D_MATH_FN). It assumes subnormals, as "
        "float_order.h does."),
    "vec.h": (
        "Include it from C or C++; it includes its four parts. A vector "
        "is the caller's array and its count: d_vec_d_add, _dot, _cross, "
        "_norm, _normalize, _lerp and the rest, with _f and _ld beside "
        "them; linear_algebra/vector.hpp is its C++ face.",
        "None: dot, cross and sum are compensated, and their _fast forms "
        "are the formulas as written, chosen by name (S404); a root takes "
        "the _c form or the plain one, by name.",
        LEVELS + WIDE),
    "mat.h": (
        "Include it from C or C++; it includes its four parts. A matrix "
        "is the caller's row-major array: d_mat_d_multiply, _transpose, "
        "_determinant, _inverse, _solve and the rest, with _f and _ld "
        "beside them; linear_algebra/matrix.hpp is its C++ face.",
        "None: the products, the trace and the solve have _fast forms, "
        "the formulas as written, chosen by name (S404, S406).",
        REFINE),
    "quat.h": (
        "Include it from C or C++; it includes its five parts. A "
        "quaternion is the caller's array [x, y, z, w]: d_quat_d_multiply, "
        "_inverse, _from_axis_angle, _rotate, _slerp, _exp, _log and the "
        "rest, with _f and _ld beside them; quaternion.hpp is its C++ "
        "face.",
        "None: each accurate operation has a _fast form, the formula as "
        "written, chosen by name (S405); a root or a sine takes the _c "
        "form or the plain one, by name.",
        LEVELS + WIDE),
    "coord.h": (
        "Include it from C or C++; it includes its four parts. A point is "
        "the caller's array: d_coord_d_polar_from_cartesian and the other "
        "conversions, then each system's distances, norms and canonical "
        "forms, with _f and _ld beside them; in C++ "
        "d_coord_polar_from_cartesian and the rest are overloaded on the "
        "three, and the math/coordinate headers are its C++ faces.",
        "None: each function that needs a root, a sine or an arc-tangent "
        "comes as a _c form and a plain one, and in C++ an _auto form that "
        "from C++20 takes the _c one in a constant expression; the choice "
        "is the name's.",
        LEVELS),
    "isect.h": (
        "Include it from C or C++. d_isect_d_ray_aabb and "
        "d_isect_d_ray_triangle test a ray, its origin and direction "
        "3-arrays, against a box or a triangle, with _f and _ld beside "
        "them; geometry/ray.hpp is its C++ face.",
        NONE_RUN,
        PURE),
    "plane.h": (
        "Include it from C or C++. d_plane_d_ray_crosses, "
        "d_plane_d_polygon_contains, _polygon_area and _polygon_centroid "
        "take a point as an (x, y) pair and a polygon's vertices "
        "interleaved, with _f and _ld beside them; geometry/shape_2d.hpp's "
        "polygons are its C++ face.",
        NONE_RUN,
        PURE),
    "decomp.h": (
        "Include it from C or C++; it includes its ten parts. Matrices "
        "are the caller's row-major arrays: d_decomp_d_lu, _cholesky, "
        "_qr, the substitutions, the SVD and the eigensolvers, with _f and "
        "_ld beside them; linear_algebra/decomposition.hpp, solve.hpp, "
        "svd.hpp and eigen.hpp are its C++ faces.",
        "The Jacobi kernels take a tolerance, and the general "
        "eigensolvers a step budget, as arguments; what takes a root comes "
        "as a _c form and a plain one, by name.",
        LEVELS),
    "cplx.h": (
        "Include it from C or C++; it includes its five parts. A complex "
        "number is struct dj_cplx_d (and _f, _ld), the real part first: "
        "d_cplx_d_add and the arithmetic, then C99's 22 functions, "
        "d_cplx_d_exp, _log, _sqrt, _pow and the rest; complex.hpp is its "
        "C++ face. Where the C compiler has _Complex, its section 5 "
        "converts.",
        "None: the functions' _c forms and plain forms are chosen by "
        "name, not at run time.",
        LEVELS + " Where float and double are evaluated wider, as on a "
        "32-bit x87, their exact steps are taken in long double "
        "(D_MATH_FLT_EVAL_WIDE, D_MATH_DBL_EVAL_WIDE)."),
    "cmat.h": (
        "Include it from C or C++; it includes its seven parts. Matrices "
        "are the caller's row-major arrays of struct dj_cplx_d (and _f, "
        "_ld): d_cmat_d_lu, _determinant, _inverse, _solve, "
        "_hermitian_eigen and _eigen, with _f and _ld beside them; the "
        "linear-algebra faces' complex element types reach them.",
        "None: the product and the solve have _fast forms, the formulas "
        "as written, chosen by name (S410); the iterative kernels take a "
        "tolerance and a step budget as arguments.",
        REFINE),
}

# a part of a split header: its header's name and its count of parts
PART = (
    "Include %(header)s, which includes its %(count)s parts in order; or "
    "this part alone, which includes the parts before it whose names it "
    "uses.",
    "As %(header)s's, whose banner says.",
    "As %(header)s's, whose banner says.")
