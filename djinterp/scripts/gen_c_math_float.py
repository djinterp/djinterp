#!/usr/bin/env python3
"""gen_c_math_float.py - write the C math core's floating-point families.

C has no templates, and a floating-point kernel cannot be written once over
the widest type and reached by conversion, as interval's integer kernels
are: a float computed as a long double is a different float. So each
floating-point kernel exists three times, for float, double and long double
(the _f, _d and _ld families), and this script writes all three from one
text, a template below in which @T@ is the type, @S@ the family's suffix and
@P@, @EMIN@, @EMAX@, @MIN@, @MAX@, @EPS@, @DIG@ its <float.h> properties.

It writes, each into inc/djinterp/c/modules/math/:

  float_order.h      the order of a floating-point type's values: successor,
                     predecessor, ordinal distance and advance - in pure
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

The eight largest - decomp.h, cmat.h, cplx.h, quat.h, vec.h, coord.h, mat.h
and exponential.h - are written in parts of at most about 4,000 lines (S436),
each a header that stands alone (decomp_f.h, decomp_f_eigen_c.h, ...), and
the header of the name includes them in order: SPLITS, below, names them.

usage:  scripts/gen_c_math_float.py           write every header
        scripts/gen_c_math_float.py --check   exit 1 if any differs from
                                              what the script would write
"""
import os
import sys

# the package beside this script, and no bytecode written into it
sys.dont_write_bytecode = True
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from gen_c_math.driver import main  # noqa: E402


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
