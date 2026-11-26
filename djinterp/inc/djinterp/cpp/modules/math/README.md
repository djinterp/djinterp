# djinterp::math

The math subframework: scalar kernels, intervals, linear algebra, coordinate
systems, geometry, and (in progress) expressions, functions and calculus.
Everything is header-only. The numeric work is written once in C, under
`inc/djinterp/c/modules/math/`; the C++ headers under
`inc/djinterp/cpp/modules/math/` are faces over it that add what each
language level allows.

This file says what is here and how to build and check it. `roadmap.md`, beside
it, says what comes next and in what order; `decisions.md` holds every
decision the lane has taken, S1 on, each with its alternative and a review
of it, its advantages and drawbacks and the path recommended, the
outstanding ones first, by group and urgency; and
`comparison.md` compares the framework as proposed with similar libraries,
feature by feature.

---

## Quick start

Include the header of each part you use, as below, from C++11; `math.hpp`
includes every header of the module, at every level, each part from the
level it begins at: the compile-time core, the forms on it, the calculus
headers and `reify` from C++11 (chunks 53, 56, 58 and 59), empty below it.

```cpp
#include "djinterp/cpp/modules/math/expr/expr.hpp"              // the language
#include "djinterp/cpp/modules/math/complex.hpp"                // complex<T>
#include "djinterp/cpp/modules/math/quaternion.hpp"             // quaternion<T>
#include "djinterp/cpp/modules/math/linear_algebra/linalg.hpp"  // linalg

namespace m  = djinterp::math;
namespace la = djinterp::math::linalg;

// a formula as text: parsed once, then evaluated, exactly or in a type
const m::expr  f = m::parse("x ^ 2 - 2 * x * y + y ^ 2");
const double   d = f.eval<double>(m::at("x", 0.5).at("y", 2.0)).value();
const m::value h = m::parse("1/3 + 1/6").eval().value();   // exactly 1/2
const m::value r = m::parse("sqrt(-4)").eval().value();    // exactly 2im

// compiled once for a type, then run many times, the same bits as eval
const m::expr_program<double> p = f.compile<double>().value();  // x, y
const double                  xy[2] = { 0.5, 2.0 };
const double                  q = p.run(xy);                    // 2.25

// one number of any kind, exact where it can be
const m::value tenth = m::value(1) / m::value(10);
const m::value sum   = tenth + tenth + tenth;                 // exactly 3/10

// the C core's faces: complex numbers, quaternions, linear algebra
const m::complex<double>    z(3.0, 4.0);                      // abs(z) is 5
const m::quaternion<double> turn =                            // a quarter
    m::from_axis_angle(la::vector<double, 3>(0.0, 0.0, 1.0),  // turn about
                       1.5707963267948966);                   // z
const la::vector<double, 3> v =                               // about
    m::rotate(turn, la::vector<double, 3>(1.0, 0.0, 0.0));    // (0, 1, 0)
const double                a_[4] = { 2.0, 1.0,
                                      1.0, 3.0 };
const double                b_[2] = { 3.0, 5.0 };
const la::vector<double, 2> s =                               // (0.8, 1.4)
    la::solve(la::matrix<double, 2, 2>(a_), la::vector<double, 2>(b_));
```

Each value in a comment is what the program `quickstart_c48.cpp` of chunk
48's record prints, built with g++ and clang++ from C++11 to C++20. A step
of the language that can fail returns `expr_result<T>`, its answer or an
`expr_error`; `value()` takes the answer, and an error passes through every
step chained after it.

**Where to start.**

| To | Use | Header |
|---|---|---|
| parse, evaluate, compile, simplify a formula given as text | `math::expr`, `math::parse` | `expr/expr.hpp`; `expr/expr_common.hpp`'s banner maps the language's headers |
| hold one number of any kind, exact where it can be | `math::value` | `value.hpp` |
| compute with complex numbers or quaternions | `math::complex<T>`, `math::quaternion<T>` (`linalg::quaternion<T>` is it) | `complex.hpp`, `quaternion.hpp` |
| compute with vectors and matrices | `linalg::vector`, `linalg::matrix`, `linalg::dynamic_matrix`, `solve`, `eigen_symmetric`, `eigen_general`, `svd` | `linear_algebra/linalg.hpp` |
| print, simplify or evaluate by name a compile-time expression, or its derivative | `math::reify(e)`, then `expr_render`, `expr_simplify`, `math::expr` | `expr/reify.hpp` |
| compute an expression of vectors or matrices in one pass, the eager bits | `linalg::lazy(a)`, then `eval` or `assign` | `linear_algebra/lazy.hpp` (C++11) |
| write C, or reach the kernels under every face | `d_math_*`, `dj_math_value`, `cplx.h`, `quat.h`, `mat.h`, ... | `inc/djinterp/c/modules/math/` (section 3) |
| take all of it | everything above, in `djinterp::math` | `math.hpp` (every level) |

---

## 1. Status (2026.11.23)

| Part | Headers | Compiles from | Notes |
|---|---|---|---|
| C core (`c/modules/math/`) | 28 | C99; ISO strict C++98 as C++ | `_c` forms, the rational kernels, the value layer, decimal reading, literals and wide integers are constant expressions from C++14; the value layer takes boxed kinds through a table each one fills in (chunk 22), asks a kind whether each of its values is exact, and counts every box's holders atomically (chunk 40); signed integers of 256 bits (chunk 23), a boxed kind of the value since chunk 24; complex numbers, `cplx.h`, with C99's 22 functions, 18 of them in both forms, and `log1p`, `expm1`, `asinh`, `acosh` and `atanh` (chunk 37); a general matrix's eigenvalues and eigenvectors by LAPACK's path, complex matrices in `cmat.h`, the norms scaled and the solution of [A \| B] (chunk 38); `exp2`, `log2`, `log10`, `log1p` and `expm1` of a complex number, each one long double kernel in pairs, both forms rounded once from it, `exp2`'s angle reduced exactly for every y (chunk 42); whole powers z^n carried in long double pairs and rounded once, each part within its bound, both forms (chunk 45); quaternions, `quat.h`, accurate by default: the product, inverse and quotients by Dot2 in pairs, the rotation q v q^-1 of any q not 0 and its matrices, the scaled norm and normalization, Shepperd's method, two vectors' quaternion, nlerp, a shorter-arc slerp accurate near 0, and exp, log, sqrt and real powers as the complex functions in q's plane, each part rounded once; the former formulas the `_fast` forms (chunk 44); two inline numbers' arithmetic sent to their kind's at once, before the checks that cannot hold for them, the same results, a sum or product of two doubles three times sooner (chunk 47, S396); `dot`, `cross`, `sum`, the matrix products and the trace compensated by default, each a sum rounded once from Dot2's or Sum2's pair, and `solve` refined with extra precision as LAPACK's `dgerfsx` refines, each with a `_fast` form as written (chunk 49, S404 to S406); `project` and `reject` scaled at the range's ends, and complex matrices' products compensated, each part rounded once from Dot2's pair, and their `solve` refined, each with a `_fast` form as written (chunk 50, S409, S410); the double `exp_c` correctly rounded in double arithmetic alone, by three phases with proven bounds, at 9.9 ns a call; `dd.h`, a public pair of doubles with proven bounds; every exact product immune to a compiler that fuses; the double family refused where double is evaluated wider; and `self_test.h` (chunk 51, S414 to S419); the double `log_c` correctly rounded so too, at 18.2 ns a call, its bounds proven in each of three cases, and in the self-test (chunk 52, S424 to S426) |
| interval | 5 | ISO strict C++98 | |
| linear algebra | 13 | ISO strict C++98; `lazy.hpp` C++11, empty below | general eigenvalues by balancing, the Hessenberg form and Francis's double-shift QR, eigenvectors from the Schur form (chunk 38, after chunk 13's), every eigenvalue and eigenvector as `math::complex<T>` (chunk 39); vectors and matrices of any number type through `number_traits`, of `math::value` exactly (chunk 26); `solve` for any number type but an integer one, products in their order, so quaternion matrices too (chunk 38); of `math::complex<T>`, their determinant, inverse and solution `cmat.h`'s bit for bit, tolerances of the real type (chunk 39); `vdot`, the conjugating inner product, the norms of the moduli, `frobenius_norm` and `is_unitary` of them, and the generic norms of the moduli, so of values holding complex numbers too (chunk 43); `dynamic_matrix` of every element type the fixed-size matrix takes, complex numbers and values among them (chunk 46); of `float`, `double` and `long double`, `dot`, `cross`, `sum`, products, the trace and `solve` the C core's compensated and refined kernels, `dot_fast` ... `solve_fast` the formulas as written (chunk 49); of `math::complex<T>` too, `cmat.h`'s bit for bit (chunk 50, S411); the opt-in lazy type, `lazy(a) * k + lazy(b)` computed each element once by `eval` or `assign`, the eager bits, a product's element fenced where the build fuses (chunk 60, step 34, S463 to S467) |
| coordinate | 5 | ISO strict C++98 | points are `linalg::vector`s |
| geometry | 14 | ISO strict C++98 | collections are `re_std::array`s |
| `constants.hpp`, `quaternion.hpp`, `math_common.hpp` | 3 | ISO strict C++98 | named constants are constant expressions from C++11; the one quaternion, `math::quaternion<T>`, which `linalg::quaternion<T>` names (chunk 48, S398), takes every function of `quat.h`, the accurate forms by default, constant expressions from C++14, and the `_fast` forms for a renderer (chunk 44) |
| `expression.hpp` | 1 | C++11, empty below | the compile-time core (chunk 53, step 29, S430): the nodes, + - * /, `squared`, `abs` and `dot` constant expressions from C++11; `exp`, `sin`, `cos`, `tan` and `sqrt` the C core's `_c` kernels, one answer at every level, constant expressions from C++14; the placeholders `x` to `phi` in `math::variables` (S403) |
| `values.hpp`, `function/function.hpp`, `functional.hpp` | 3 | C++11, empty below | on the compile-time core (chunk 56, step 30, S441 to S445): relations, logic, implicit functions and level sets constant expressions from C++11, piecewise, vector-valued and parametric forms and `math_function`'s calls from a `std::array`, a `std::tuple` or a system's point from C++14, where `std::get` and a point's constructors are constant expressions; `math_function` takes a coordinate system's point, a `linalg::vector` (S443); `sample` from C++11, `sample_over` from C++14, at an interval's members (S444); the bridge's adapters with the functional subframework itself |
| calculus (6), `math.hpp` | 7 | C++11, empty below; `math.hpp` at every level | the calculus from C++11 (chunk 58, steps 31 and 33, S450 to S454): derivatives, the gradient, the 5-point rule, Taylor evaluation, the arithmetic sequence and sums and the scalar helpers that are one return constant expressions from C++11, the quadrature rules, sums, sequences' loops, `binomial`, `horner` and the transcendentals from C++14, every form's bits as before but `log`'s and `atan`'s, now the C core's kernels, the logarithm correctly rounded, one answer at every level (N3, S453); a rule for a new node `derivative(wrt<I>, const node&)` (S452); `math.hpp` includes every header of the module, `expr/` and `linear_algebra/` among them (chunk 48, S399), clean under `-Wshadow` since chunk 53 (S403), and compiles at every rung |
| `numeric_compare.hpp` | 1 | ISO strict C++98 | numbers written as text compared exactly (chunk 21, M3) |
| `value.hpp` | 1 | ISO strict C++98 | the generic value's face (chunk 25): `math::value` holds any kind, a box as an `int` holds its number; `inline_value`'s operations are constant expressions from C++14, `math::value`'s from C++20; a complex value's modulus squared, exact where it is (chunk 43) |
| `number_traits.hpp` | 1 | ISO strict C++98 | the static side (chunk 26): what generic code needs of a number type, the type two numbers combine to, and from C++20 the concept `number`; the field's and the order's traits, the magnitude, the epsilon and a relative tolerance (chunk 38); `number_modulus_squared`, \|x\|^2, which a type of numbers not all real overloads (chunk 43) |
| `complex.hpp` | 1 | ISO strict C++98 | complex numbers (chunk 39): `math::complex<T>`, of `float`, `double` and `long double`, the face of `cplx.h`'s struct, its operators and C99's functions the C core's, by their own names the plain forms at run time and the `_c` forms in a constant expression from C++20, by the names ending `_c` the `_c` forms at every level, constant expressions from C++14 (chunk 46, S336); made from a `std::complex<T>` implicitly, giving one by `to_std()`; a field with no order to `number_traits`, its magnitude \|re\| + \|im\|; `exp2`, `log2`, `log10`, `log1p` and `expm1` (chunk 42) |
| `expr/` | 27 | C++11, empty below | the run-time expression language (chunks 28 to 36 and 39 to 48): every header of Phases 2 to 4 in place, its types whole; numbers, `+ - * / mod ^`, the factorial, calls of the C core's functions, the constants `pi`, `e`, `true` and `false`, the relations, logic and piecewise parse, print and evaluate, in each floating-point type and exactly as a value; substitution and simplification; the handle, `math::expr`, chains them; vector and matrix literals, `[1, 2; 3, 4]`, blocks `[A, b]` and the empty matrix `[]`, parse, print and evaluate to matrix values, exact or dense, with their sums, products, transposes, powers and indexing, `A[i, j]`, and linear algebra's functions, `det`, `inv`, `trace`, `norm`, `eig`, `dot`, `cross` and `solve`; complex numbers, `3 + 4im`, a kind of the value exact where their parts are, with `complex`, `real`, `imag`, `conj`, `arg`, the modulus, `z'`, `=` and `!=`, and exact powers to a whole exponent (chunk 40); each function's principal complex value, exact where it is a Gaussian rational, a real function's outside its real domain unless the context asks real results, `log1p`, `expm1`, `asinh`, `acosh`, `atanh`, `sign`, `min` and `max`, and the folds in `math::complex<T>` (chunk 41); `exp2`, `log2`, `log10`, `log1p` and `expm1` of a complex value, `cplx.h`'s, and of a real one outside its real domain (chunk 42); matrices of complex numbers, dense or exact, their arithmetic, `A'` conjugating, `transpose`, the parts of a matrix and `complex(A, B)`, linear algebra's functions of them, `dot` conjugating, and every eigenvalue, complex where it is, exact where it is a Gaussian rational (chunk 43); quaternions, `1 + 2jm - km`, a kind of the value exact where their parts are, `jm` and `km` their units, with `quat`, `parts`, `real`, `conj`, the norm, `inv`, `sign`, `exp`, `log`, `sqrt`, powers to a real exponent and `rotate`, products in their order, `p / q` = `p inv(q)`, and simplification keeping a quaternion's factors in their order (chunk 45); matrices of quaternions, dense or exact, their arithmetic the kind's own in each product's order, `A'` conjugating, linear algebra's functions of them and of a quaternion, `det` a domain error, Zhang's standard eigenvalues, and `rotmat`, `rotquat` and `slerp`; every evaluator given the fold's context, and a value's complex functions the `_c` forms, the face's by their own names its plain forms at run time (chunk 46); a term compiled once for a type, `expr_compile<T>`, its runs the typed fold's answers bit for bit, 26 to 55 times sooner (chunk 47); each header named for what it holds, none of a name another math header has, `matrix.hpp` and `function.hpp` split in five (chunk 48, S400, S401); the matrix kind in four parts (chunk 55, S437); `reify.hpp`, a compile-time expression made a term of the language, `math::reify(e)`, its variables x, y, z and t or the names given, its integral constants exact, so that a reified derivative simplifies (chunk 59, step 32, S456 to S461); the rest stubs |

From chunk 45b the lane works in the restructured tree: the C core in
`inc/djinterp/c/modules/math/`, its configuration in
`inc/djinterp/config/c/modules/math/`, the faces in
`inc/djinterp/cpp/modules/math/` and the expression core in
`inc/djinterp/cpp/modules/parse/expression/`, the tests likewise under
`tests/djinterp/c/modules/math/` and `tests/djinterp/cpp/modules/math/`; and
every C type is named with `dj_`, the restructure's prefix (`struct
dj_cplx_d`, `dj_math_umax`), its functions keeping `d_`.

"Compiles from" means the C++ ladder (`scripts/ci/ladder.py`) passes the
header with g++ and clang++, plain and testing, at every rung from that one
to C++26. Every header not on the ladder's known list
(`scripts/ci/ladder_known.txt`) passes all of them.

Tests: 29 C harnesses (`tests/djinterp/c/modules/math/`) and 46 face tests
(`tests/djinterp/cpp/modules/math/`), one of them for `parse/expression/`,
one for the lazy type (chunk 60),
five for the compile-time core (chunks 53, 56 and 58), one for the bridge
from it to the language, `reify` (chunk 59), and eight for `expr/`: four
its complex numbers', two its quaternions' and one its compiled terms'
(section 3). In the full matrix (section 5) every
harness and face test passes in every configuration.

---

## 2. Architecture

**A C core with C++ faces.** Every algorithm lives once in the C layer, and C
programs call it directly. The C++ headers forward to it instead of restating
it. Element types the C core does not cover (anything other than `float`,
`double` and `long double`) take a generic C++ path. These are the owner's
rulings of 2026.10.04.

**Kernels.** A kernel is a function defined in a header, marked with one of
five qualifiers from `c/modules/math/math_common.h`:

- `D_MATH_FN`: pure arithmetic. A `static inline` function in C, and an
  `inline` one in C++ that is `constexpr` from C++14.
- `D_MATH_FN_RT`: calls the C library or writes to a buffer, so it is never
  `constexpr`.
- `D_MATH_FN_ENTRY`: a function a table points to, as a boxed kind's
  entries are. A header inline the compiler is never told it must inline,
  as it is told the other two's outside testing builds, so a call through
  the table builds at every optimization level, gcc's `-Og` included
  (chunk 26).
- `D_MATH_FN_KERNEL`: a large kernel of pure arithmetic that many
  functions share, as `cplx.h`'s long double kernels serve its three
  families: `constexpr` from C++14, as `D_MATH_FN` is, but never forced
  inline, so that a translation unit compiles it once however many callers
  it has (chunk 37).
- `D_MATH_FN_KERNEL_RT`: such a kernel that calls the C library, so never
  `constexpr`: the eigenvalue paths' plain forms (chunk 38).

**Two forms of each function.**

- The `_c` form (`d_math_d_sqrt_c`) is pure arithmetic and a constant
  expression from C++14. It is correctly rounded for sqrt, cbrt, hypot, fmod,
  remainder and the rounding functions, and within an ulp of the C library
  for the transcendental functions.
- The plain form (`d_math_d_sqrt`) calls the C library and is fast at run
  time.

A face picks one form per language level, using `std::is_constant_evaluated`
from C++20. For the correctly rounded functions the two forms are
bit-identical. Domain errors give the C library's answers in both forms: NaN,
or minus infinity for `log(0)`. In C++ each header ends with one overloaded
name for all three families (`d_math_sqrt_c(x)`).

**One operation per statement.** Kernel bodies compute one operation per
statement. This keeps x87 excess precision and FMA contraction from changing
results, so every build in the matrix computes the same bits. Clang on the
32-bit x87 is the known exception, and stays out of the matrix (S37). It is
not enough where gcc fuses across statements, as it does in its GNU modes
wherever the target has a fused multiply-add: there each exact product takes
its low part by an explicit one, the bits Dekker's product gives elsewhere,
and every generated header tells clang not to fuse (S414); the plain
formulas, the `_fast` forms among them, follow the build (S415). The double
family's correctly rounded functions refuse to compile where double is
evaluated wider than double, as on a 32-bit x87 (S418).

**Generated code.** Twenty of the core headers are generated by
`scripts/gen_c_math_float.py`, each from one template written out for each
family (`f`, `d`, `ld`). Edit the generator, never its output. `--check`
fails if the headers are out of step. `math_common.h`, `interval.h`,
`rational.h`, `value.h`, `decimal.h`, `literal.h`, `wide.h` and `value_wide.h`
are written by hand. The generator is a package, `scripts/gen_c_math/`, of
modules of at most about 4,000 lines, one for a header's text or two, and
the script runs them (S439). It writes the eight largest headers in parts of
at most about 4,000 lines - `decomp.h` in ten, `cmat.h` in seven,
`exponential.h` in six (S502), `cplx.h` and `quat.h` in five, `vec.h`,
`coord.h` and `mat.h` in four - each part a header that stands alone,
named for its header and what it holds (`decomp_f.h`,
`decomp_f_eigen_c.h`, ..., `decomp_cpp.h`), and the
header of the name includes them in order, so that an includer sees the
same code in the same order (S436). It aligns each parameter list's names
(S435).

**Exact rationals.** `rational.h` holds rationals over the widest integer,
`dj_math_imax`, in one reduced form. Each kernel either stores the exact
result or stores nothing and says why: overflow, a pole or an invalid
operation. Overflow means that the reduced result does not fit, never that
an intermediate did not: intermediates are held in two words. A rational
converts to the nearest `float`, `double` or `long double`, ties to even,
whatever the rounding mode.

**The generic value.** `value.h`'s `struct dj_math_value` holds one value of
any kind: none, an error, a boolean, an integer, a rational, a `float`, a
`double` or a `long double`. Its operations follow a tower:

- Exact numbers stay exact. A rational whose denominator comes to 1 is an
  integer. An exact result too wide for `dj_math_imax` becomes the nearest
  `long double`, flagged inexact, until bignum exists.
- An exact number meeting a floating-point one is converted to the latter's
  type, correctly rounded. Two floating-point kinds meet in the wider.
- Errors are values that pass through operations. A boolean or none in
  arithmetic is a domain error.
- Comparison is exact across kinds.

**Boxed kinds.** A kind of number that does not fit in the value, such as a
wide integer or a big float, is boxed. It fills in a table,
`struct dj_math_boxed_kind`, which is how it registers:

- its name, and its rank: its place in the tower, on one scale with the
  inline numbers (integer 100, rational 200, float 300, double 400, long
  double 500), and its own;
- whether it is a number, and whether an exact one;
- its operations: retain and release, arithmetic, comparison, conversion to
  the inline numbers, and text.

A boxed value holds a pointer to its kind's table and one to its storage, so
`value.h` needs no list of kinds and keeps no state. In an operation, the
operand higher in the tower handles it. A boxed kind does so through its
table. An inline number converts the boxed operand to its own kind; to
compare, it uses the nearest `long double` and the rounding's direction, so
comparison stays exact. A result is the caller's to release with
`d_math_value_release`, and an operand is only lent. A new kind changes
neither `value.h` nor the inline kinds' code, and the inline kinds are still
constant expressions.

This is roadmap Phase 1's C core; its C++ face is `math::value` (below).

**Wide integers.** `wide.h` holds signed integers of 256 bits: a magnitude
in limbs of half a word (32 bits, or 16 where the build has no 64-bit
integer) and a sign, so the range is symmetric and 0 has one form. As in
`rational.h`, each kernel stores the exact result or stores nothing and says
why: overflow, division by zero, 0/0, a rational that is no integer, or text
that is no integer. Division truncates, as C's does. The kernels are add,
subtract, multiply, divide with remainder (Knuth's algorithm D), gcd,
comparison, conversion to and from `dj_math_imax`, `dj_math_umax` and
rationals, text both ways, and the nearest `float`, `double` or
`long double`, ties to even, with the rounding's direction; a float past
the largest is an infinity. Nothing allocates, and every kernel but the
text writer is a constant expression from C++14. Under the type are limb
kernels that take any length, which its boxed kind (below) and bignum's
later kinds build on. This is roadmap item P1.

**The wide integer as a value.** `value_wide.h` registers `wide.h`'s
integer with `value.h` as a boxed kind, the first real one, and `value.h`
did not change. Its rank is 210, between the rational's and the float's.
`d_math_value_from_wide` makes a value of it, and `d_math_value_to_wide`
reads an integer back. With a wide integer, an inline integer or a
rational, its arithmetic is exact:

- an integer stays a wide integer, however small, so that arithmetic begun
  with one stays exact past `dj_math_imax`;
- a result that is no integer is an inline rational, where it fits the
  family;
- a result past both, an integer of more than 256 bits or a ratio past
  `dj_math_imax`, is the nearest `long double`, flagged inexact (G2),
  rounded once from the exact ratio, whose terms run to 512 bits, by
  `wide.h`'s division.

A float, double or long double meets it as `value.h` has an inline number
meet any boxed kind: converted to the nearest value of its type, so a
comparison stays exact too. Each value is a box of its own, allocated
through two knobs in `config/c/modules/math/cfg_value_wide.h` (`malloc`
and `free` unless set otherwise); where no storage is given, the result is a
memory error.

**The value in C++.** `math/value.hpp` is the value's face. `math::value`
holds one value of any kind, a boxed kind's included, and holds its box as
an `int` holds its number: a copy is one holder more, and a value destroyed
or overwritten one fewer. From C++11 a value moves, and a move never fails.
`math::inline_value` holds the inline kinds alone, never a box, so it is a
literal type, and its operations are constant expressions from C++14; from
C++20, where a destructor may be constexpr, `math::value`'s are too, for the
inline kinds. Both are made implicitly from the arithmetic types, a `bool`
and an error code, and refuse a pointer: an unsigned integer past
`dj_math_imax` is G2's long double, through `value.h`'s
`d_math_value_from_uint`. Their operators, `abs` and the six comparisons
are found by argument-dependent lookup only, and an operation makes its
result in place from `value.h`'s, so it costs what `value.h`'s call does.
Each comparison asks `value.h`'s exact order, so a NaN, an error and unlike
kinds are unordered, and from C++20 `<=>` is a partial ordering. `to(out)`
converts a value where it has one of `out`'s type. From C++20 the concept
`number` names the types generic code computes with: the arithmetic types
but `bool`, and the two values.

**Numbers in generic code.** `math/number_traits.hpp` is the static side of
the value. `number_traits<T>` says what a template needs of a number type:
whether its arithmetic is exact, whether it holds integers alone, whether
its quotients are a field's and its products commute, its zero and one;
whether `<` orders it, and `|x|`; the magnitude generic code pivots by, `|x|`
for an ordered type and a nonnegative number of an ordered type for one
without an order (a complex number, a quaternion); its square root, exact
for an integer type, rounded down; its conversions to and from `double`;
its epsilon, the precision a value carries, 0 for an exact one; and its
tolerance, four epsilons relative to the larger magnitude, 0 where both
values are exact (chunk 38, S303). The C++ face's comparisons given no
tolerance take it, relative to their operands' scale; given one, they take
it as an absolute one, as before (S304). The standard integer
and floating-point types have their own; `math::value` and `inline_value`
theirs, whose root is exact for the square of a rational of the family, so
the norm of (3, 4) is 5; and any other type, a bignum among them, joins by
specializing it. A type with none is taken as the linear-algebra
subframework always took one: made from an `int`, and through `double`; an
integral type with none, a character type or `bool`, is exact and an
integer, as the language has it. `common_number<A, B>` is the type of `a + b`,
from C++98: the usual arithmetic conversions for the arithmetic types, and
for the values the classes' own. Linear algebra's generic path, for any
element type but `float`, `double` and `long double`, now reads its zero,
one, roots and tolerances from the traits, and a vector or matrix made by
default is zero in any number type, so vectors and matrices of
`math::value` compute exactly: the Hilbert matrix's determinant and inverse
come out as rationals, its product with its inverse the identity. The
generic kernels keep each product in its order and pivot by magnitude, so
that `solve` and `inverse` of a quaternion matrix are right, and
`solve(A, b)` and `solve(A, B)` take every number type but an integer one,
by elimination of [A | B]: for `float`, `double` and `long double` the C
core's `d_mat_solve`, LU's solution refined with extra precision (chunk 38,
S302, S305; chunk 49, S406), for `math::complex<T>` `cmat.h`'s
`d_cmat_solve`, refined the same way (chunk 50, S411), and `solve_fast` the
elimination alone, LU's bits.

**The run-time expression language.** `math/expr/` is roadmap Phases 2 to
4's language, from C++11: math text read into a term, which is evaluated,
written back as text, rewritten and differentiated. It stands on
`parse/expression/` (gate G4). Its term, `expr_term`, is that module's
`expression<expr_op, expr_atom>`: an operator applied to its operands, or an
atom, which is a number (a `math::value`, with the literal it was written
as, so that each type rounds the literal once), a variable or a named
constant. A function is an operator of the term, so every fold and rewrite
takes a call as it takes a sum. The six layers:

1. tokens (`token.hpp`);
2. the operator table and the parser layer (`operator.hpp`, `parse.hpp`);
3. the term (`term.hpp`);
4. the folds, to numbers and values (`evaluate.hpp`, with the function and
   constant tables of `function_table.hpp`, the arithmetic of
   `arithmetic.hpp` and the relations and logic of `logic.hpp`), to a
   program run many times (`program.hpp`), and to text (`render.hpp`);
5. the rewrites (`substitute.hpp`, `simplify.hpp`, and Phase 4's
   `derivative.hpp`);
6. the handle, `math::expr` (`expr.hpp`, which includes every header but
   `reify.hpp`).

Phase 3's kinds, `complex_kind.hpp`, `quaternion_kind.hpp` and
`matrix_kind.hpp`, with `matrix_function.hpp` and `rotation.hpp`, and Phase
4's `numeric.hpp`, `summation.hpp` and `antiderivative.hpp` stand beside
them, and `reify.hpp`, which makes a compile-time expression a term (step
32), `math.hpp` including it so that the language needs no compile-time
core; `expr_common.hpp`'s banner maps them all. Chunk 48 renamed the four
whose names another math header had (`complex.hpp`, `quaternion.hpp`,
`matrix.hpp`, `series.hpp`) and split the two largest, `function.hpp` into
the tables, the arithmetic and the logic and `matrix.hpp` into the kind and
its functions (S400, S401); the steps below name each header as it is now. A
step that can fail returns `expr_result<T>`, which is `result<T,
expr_error>`; the handle holds a term or an error, and passes an error
through every member chained after it. Step 13 (chunk 28) made every header:
its types are whole, and each entry is a stub, which answers
`expr_status::not_yet` with a message naming the step that fills it, or,
where it cannot fail, the placeholder its contract names. Each step from 14
on fills one or two headers.

Step 14 (chunk 29) filled the arithmetic. The tokenizer reads numbers by
`decimal.h`'s grammar, names, symbols and round brackets. The operator table
holds `+ - * / mod ^` and the unary `-` and `+` at gate G5's levels: `^` to
the right, `2 ^ 3 ^ 2` being `2 ^ 9`, and the unary minus below it, `-x ^ 2`
being `-(x ^ 2)`. The parser layer gives each token its role, a minus unary
or binary by where it stands, with G5's implied multiplication (`2x`) an
option, off by default, and hands the tokens to `parse/expression`'s
climber; every error is at its place in the text. `expr_evaluate<T>` gives
a term's value in `float`, `double` or `long double`, each literal rounded
once from its text (G3), `^` the C core's pow and `mod` the floored
remainder, with the divisor's sign. `expr_evaluate` with a context gives a
`math::value`, exact where it can be: `1/3 + 1/6` is exactly 1/2, `4 ^ 0.5`
exactly 2. `expr_render` writes a term as text that `expr_parse` reads back
as the same term, every literal kept.

Step 15 (chunk 30) filled the functions. `function_table.hpp`'s standard table
holds the C core's 25 functions, `sqrt` to `trunc`, with `abs` and
`factorial`, and its constant table `pi` and `e`, from the literals of
`constants::of<T>`. A name before an open bracket is a call, `atan2(y, x)`,
read by the parser layer as one operand, its arguments split at the commas;
a name the constant table has is a constant; `n!` is the factorial, postfix
and tighter than `^`; with G5's bars on, `|x|` is `abs(x)`. A fold looks
each call's function up when it meets it: in a type, the C core's plain
form; as a value, exact wherever the function's value is rational, as
`sqrt(4/9)` is 2/3, `log10(1000)` 3, `sin(0)` 0 and `20!` an integer, and
otherwise the C core's function in the tower's kind, a long double for
exact arguments. The renderer writes a call as its function's name and its
arguments, `sin(x) ^ 2`.

Step 16 (chunk 31) filled the relations and logic. The operator table holds
`= != < <= > >=` and `and`, `or` and `not`, logic below the relations, so
that `x > 0 and x < 1` is `(x > 0) and (x < 1)` and `not x = y` is
`not (x = y)`. A chain of relations is the conjunction of its links:
`a < b <= c` is `a < b and b <= c`, and a bracketed relation is one operand,
no link. As a value a relation is a boolean, by `value.h`'s exact order, so
that `0.1 + 0.2 = 0.3` holds, and logic is Kleene's: `false and x` is false
whatever `x` is, an error among what it may be. In a type the booleans are
1 and 0, a relation is C's comparison, and logic reads a NaN as unknown.
`piecewise(when(x < 0, -x), otherwise(x))`, in `function/function.hpp`'s vocabulary,
is the call `piecewise(x < 0, -x, x)`, written back in its branches; its
value is that of the first branch whose condition holds. `all_of` and
`any_of` are `and` and `or` of any number of operands, and `true` and
`false` are constants.

Step 17 (chunk 32) filled the handle. `math::expr` holds a term, or the
error that kept it from being one, and each member makes a new handle or an
answer, so that steps chain: `math::parse("x^2 * sin(x)").diff("x").str()`.
Each member is a term-level entry's: `str()` is `expr_render`'s text;
`eval<T>(bindings)` the fold in a type and `eval(bindings, context)` the
exact one, each an `expr_result`, so that `math::parse("1/3 + 1/6").eval()`
holds exactly 1/2; and `subs`, `simplify` and `diff` are handles of the
rewrites' terms. An error passes through every member after it and comes
out at the chain's end, as `str()`'s message or `eval`'s error;
`math::try_parse(text)` gives the parse as an `expr_result<expr>`, for a
caller who branches on it. Until step 25 fills the derivative, `diff`
gives the error `not_yet`.

Step 18 (chunk 33) filled the rewrites. `expr_substitute`, the handle's
`subs`, puts a term in place of a variable: `x ^ 2 * sin(x)` with `x` by
`t + 1` is `(t + 1) ^ 2 * sin(t + 1)`. `expr_simplify`, the handle's
`simplify`, rewrites a term until no rule changes it: a node of exact
numbers, `true` and `false` is folded to its value where that is exact or
a boolean (`1/3 + 1/6` is `1/2`, `1 < 2` is `true`, and `sqrt(2)` stays);
`x ^ 1` is `x`, `x ^ 0` and `1 ^ x` are 1; a sum's terms are collected by
their other factors, in any order, their exact coefficients summed
(`2 * x + 3 * x` is `5 * x`, `x * y + y * x` is `2 * x * y`, `x - x` is 0);
and a product's exact numbers are multiplied into one coefficient
(`2 * x * 3` is `6 * x`). A rule applies only where it makes the term
smaller, so that a term with nothing to simplify comes back as written. The
value is kept at every binding at which each part of the term has a finite
value; elsewhere, as in other computer algebra systems, it may differ:
`0 * (1 / x)` is 0 even at x = 0.

Step 19 (chunk 34) filled the matrix kind and the literals, the first of
Phase 3. A matrix is a boxed kind of the value layer, "matrix", as the wide
integer is, its table written in C++ in `expr/matrix_kind.hpp`; it is no number,
and ranks past every number, so that its own table handles an operation of
a matrix and a number. It keeps each element as it is given: as a dense
array of `float`, `double` or `long double` where every element is one of
those, in the widest one's type, and as `math::value`s otherwise, so that a
matrix of exact numbers is exact and no element is rounded by being put in
one.
`expr_matrix(rows, columns, elements)` and `expr_vector(elements)` make one,
a vector being one row; `expr_is_matrix`, `expr_shape_of` and
`expr_element` read it. The literals `[1, 2, 3]` and `[1, 2; 3, 4]` parse,
each element any term, `[x, 2 * y]`, print back as written, and evaluate
to a matrix as a value: `math::parse("[1, 2; 3, 4]").eval()` is the exact 2
by 2 matrix. In a floating-point type a matrix has no value (`bad_shape`).

Step 20 (chunk 35) filled the operators on matrices, through the kind's
table: `A + B` and `A - B` of one shape, `-A`, `A * s`, `s * A` and
`A / s`, and `A * B` where A's columns are B's rows, each computed by
linear algebra's kernels, a dense matrix's by the C core's `mat.h` in its
type and one of values exactly; the transpose `A'`, between the unary
minus and `^`, so that `A ^ 2'` is `(A ^ 2)'` as in MATLAB; `A ^ n` of a
square matrix and a whole n >= 0, by repeated squaring; and indexing from
1, `A[i, j]`, `v[i]` and `A[i]`, a row. Anything else on a matrix is a
domain error: `A + 1`, `1 / A`, `A / B`.
`math::parse("[1, 1; 1, 0] ^ 90").eval()` is exact, F(91), F(90) and F(89)
its elements. Simplification keeps the order of matrix factors:
`A * B + B * A` of two literals stays as written, while a variable is taken
for a number.

Step 21 (chunk 36) filled linear algebra's functions, the last of Phase
3's linear algebra:
`det`, `inv`, `trace`, `norm` and `eig` of one argument, `dot`, `cross`
and `solve` of two, in the function table, so that `det([1, 2; 3, 4])` is
-2 and `solve(A, b)` the x of `A x = b`. An exact matrix's determinant,
inverse and solutions are exact, by the generic kernels; a dense one's are
computed in its own type by the C core, LU with partial pivoting and
Gauss-Jordan; a matrix mixed of exact and floating-point elements in the
widest floating-point one's type. `eig` gives the eigenvalues, ascending, a
symmetric matrix's by Jacobi and any other's by Hessenberg QR; an exact
matrix's rational eigenvalues are exact, `eig([2, 1; 1, 2])` being
`[1, 3]`, and a complex pair is a domain error until step 23 gives the
value a complex kind. A number is a scalar, as in Julia: `det(2)` is 2 and
`inv(2)` is 1/2. `A ^ (-n)` is `inv(A) ^ n`; `[]` is the empty matrix,
0 by 0, `det([])` 1; and a literal's elements are blocks, so that `[A, b]`
is A with the column b beside it and `[A; B]` A above B. The C core's
`d_mat_power` now squares as the language does, so that a dense power has
one answer in both. On 2026.10.27 the owner added three steps before Phase
4, 3d to 3f, complex numbers and quaternions as kinds of the value, in the
C core and the language (`roadmap.md`; `decisions.md`, S265 to S278).

**Complex numbers.** `cplx.h` holds the C core's complex numbers, step 22's
first part (chunk 37): `struct dj_cplx_f`, `dj_cplx_d` and `dj_cplx_ld`, one
member, `part[2]`, the real part first, the layout of `T[2]` that C99's
`_Complex` and C++'s `std::complex` share (S267, S280). Its functions are
C99's 22: `d_cplx_d_real`, `_imag`, `_conj` and `_proj`, exact, once, with
the arithmetic of two complex numbers and of a complex number and a real
one; and `_abs`, `_arg`, `_exp`, `_log`, `_pow`, `_sqrt`, the trigonometric
and hyperbolic functions and their inverses, each in both forms. In C++
there is one name for every family, `d_cplx_exp(z)`. The arithmetic is pure
and comes once: a product's parts by exact products, a quotient by the
conjugate over the squared modulus and a residual's step, each part within a
step of the correctly rounded one (S281), a zero part of the sign the
textbook formula gives it, a quotient's as a product's (S382). The `_c`
forms are one long double kernel for the three families, compiled once in a
unit (`D_MATH_FN_KERNEL`, S282); the plain forms the same algorithms in each
family's type on the C library's real functions, but for the plain `pow`'s
general case, the `_c` kernel's (S283). The values at the cuts, the zeros,
the infinities and the NaNs are C99's Annex G's, as C17 corrects them
(S284), and in C a value converts to and from `_Complex` (S285). Where a
target evaluates float and double wider than their types, as a 32-bit x87
does, their exact steps are taken in long double (S287). The real functions
it needs, `log1p` and `expm1` in `exponential.h`, `asinh`, `acosh` and
`atanh` in `hyperbolic.h`, are the core's too, in every family and both
forms, their long double kernels and exp's within 0.53 of an ulp (S286).
Each function's bound in steps is in the header's banner, and
`tests/djinterp/c/modules/math/t_cplx.c` checks them against references made by
mpmath, kept beside it (S288). Chunk 42 adds the five C99 has only for real
arguments, `exp2`, `log2`, `log10`, `log1p` and `expm1` of a complex number:
each one long double kernel of pure arithmetic, its parts carried as pairs
to past twice the value's precision and rounded once, and both forms of
every family that kernel rounded to the family, as standard C's library has
none of them to be faster with, glibc and the UCRT giving `clog10` alone
(S342); on the real axis, where the value is real, each form is the family's
real function in that form, so that `log2(x + 0i)` is `log2(x)` bit for bit
and `log2(-8)` 3 + 4.532...i (S343). `exp2`'s angle y ln 2 is reduced by
pi/2 exactly for every y, by Payne and Hanek's method with 2 ln 2 / pi's
digits (S344); `expm1`'s real part, which vanishes on the curve e^x cos y =
1, is formed of terms good to some 2^-(P+28), by series in pairs of its own
for the exponential, the sine and the cosine (S345); and at an infinite or
NaN part each takes the value of the function it is a form of, clog's or
cexp's (S346). Each part is within a step of the correctly rounded one in
every family and both forms; `t_cplx.c` checks them against
`math_tests_cplx_five_refs.h`, made by mpmath.

**Eigenvalues and complex matrices.** Step 22's second part (chunk 38).
`decomp.h` finds a general real matrix's eigenvalues and eigenvectors by
LAPACK's `dgeev` path, each part public in every family and both forms:
`d_decomp_d_balance` and `_balance_back`, `_hessenberg_q`, `_schur`
(Francis's double-shift QR, as `dlahqr`), `_schur_vectors` (as `dtrevc`) and
the driver, `d_decomp_d_eigen`, its eigenvalues `struct dj_cplx_d`, ascending
by real part then imaginary, a pair exactly conjugate, and its eigenvectors
of unit length with their largest entry real and positive, 2^k A's 2^k
times A's bit for bit (S292 to S296). `cmat.h` holds complex matrices: LU,
the determinant, the inverse and the solution of [A | B] (S297), a
Hermitian matrix's eigenvalues by Jacobi's method with complex rotations
(S298) and any other's by `zgeev`'s path (S299), `d_cmat_d_eigen`. `vec.h`'s
norms, distance, unit vector and cosine are scaled where the squares would
overflow or underflow, every bit in range as before (S301); `mat.h`'s
`d_mat_d_solve` is the elimination of [A | B] with LU's bits (S302).
`t_decomp.c` and `t_cmat.c` check every eigenvalue against references made
by mpmath, to its condition number, and each part alone (S307).

**The C++ face of complex numbers.** Step 22's third part (chunk 39).
`math/complex.hpp` holds `math::complex<T>`, of `float`, `double` and
`long double`, from C++98: the face of `cplx.h`'s struct, as `math::value`
is the value's (S270). Its operators, with a complex or a real operand on
either side, and their assignments are `cplx.h`'s arithmetic, a real
operand taken as itself; `real`, `imag`, `conj`, `proj`, `abs`, `arg` and
C99's 16 other functions, and from chunk 42 `exp2`, `log2`, `log10`,
`log1p` and `expm1`, are `cplx.h`'s, each the `_c` form at every level,
so that a result has the same bits on every compiler, in a constant
expression or not, from C++14 (S313); from chunk 46 those by their own
names are the plain forms at run time and the names ending `_c` the `_c`
forms, as the owner ruled (S336, below). They are found by argument-dependent
lookup, as `math::value`'s are. A `std::complex<T>` converts to it
implicitly, and `to_std()` gives one back, so that no expression of the two
is ambiguous. To `number_traits` it is a commutative field with no order,
its magnitude `|re| + |im|`, LAPACK's, and its tolerance four of T's
epsilons (S314), so that `linear_algebra`'s vectors and matrices of it
compute: their determinant, by LU in the generic kernels, their inverse and
their solution are `cmat.h`'s bit for bit (S315), and the faces'
tolerances are of the real type, the magnitude's (S316). `general_eigen`
gives every eigenvalue and eigenvector of a real matrix as
`math::complex<T>`, a complex pair among them, converged() false only where
not every eigenvalue was found (S308, S317). `math/expr/`'s constant table
names `im`, `jm` and `km`, which answer `not_yet` until steps 23 and 24 give
them values (S271, S275, S318), and the stubs' messages name the steps
S265 made: 4a step 25 to 4d step 28 (S319).

**Complex numbers in the language.** Step 23's first part (chunk 40), of
three (S324). `math/expr/complex_kind.hpp` makes complex numbers a boxed kind of
the value, "complex", of rank 900, past every real kind's, written in C++
from C++11 beside the matrix kind (S327). Its parts are exact where both
are given exact, kept as values, so that (1 + 2im)(3 - im) is exactly
5 + 5im, and otherwise dense, a `math::complex<T>` of the wider
floating-point type; an exact number whose imaginary part comes to 0 is
real, an inexact one keeps its zeros' signs. `value.h` asks the kind
whether each value is exact, by a new last entry of its table (S325). Its
arithmetic is exact for exact operands, G2's long double past the family,
and otherwise `cplx.h`'s, a real operand taken as itself, so that
(1 + 0im)(-1) is -1 - 0im; |z| is exact where it is rational, |3 + 4im|
being 5 (S328). A complex number equals a number of equal parts, a real
one's imaginary part 0, stands in no order, and converts to a real type
where its imaginary part is 0 (S329). The value layer writes it `3 + 4im`
and the renderer `(3 + 4 * im)`, which reads back with implied
multiplication off, as by default, or on (S330). In the language `im` is
its unit, exact, and a NaN in each floating-point type (S332); `complex(x,
y)`, `real`, `imag`, `conj` and `arg` join the table, `abs` and the bars
give the modulus, `z'` is the conjugate (S333), `=` and `!=` compare the
parts and `<`, `<=`, `>` and `>=` are a domain error; an exact complex
number to a whole power is exact, and the functions of the real line's
order, `floor`, `mod` and the rest, are a domain error of one. Each other
function's principal complex value, any other power, and matrices of
complex numbers answer `not_yet`, naming step 23's part that brings them
(S331). Every boxed kind's holders, the wide integer's and the matrix's
too, are counted by `value.h`'s `d_math_boxed_hold` and
`d_math_boxed_unhold`, atomically by default wherever the compiler can,
behind the knob `D_CFG_MATH_VALUE_ATOMIC_HOLDERS` of the new
`config/c/modules/math/cfg_value.h`, so that values may be shared between
threads (S326, closing S134's flag).

**Complex functions in the language.** Step 23's second part comes in two
chunks (S335), of which chunk 41 is the first. Each function of a complex
argument gives its principal value, with C99 Annex G's cuts (S271): exact
where it is a Gaussian rational, `sqrt(2im)` being 1 + im, `sqrt(3 + 4im)`
2 + im and `sign(3 + 4im)` 3/5 + 4/5im, an exact number to a rational power
p/q exact where its principal q-th root is a Gaussian rational, found as D
times the long double root rounded to whole numbers and taken where its
q-th power is the number exactly, `(-2 + 2im) ^ (1/3)` being 1 + im (S337);
otherwise `cplx.h`'s `_c` form, `math::complex<T>`'s `exp_c` and its kin
from chunk 46, in the tower's type, a long double where every argument is
exact, dense and never made real, so that a value has the same bits on
every build (S336). So too a real
function of a real argument outside its real domain (S272): `sqrt(-4)` is
2im, `log(-1)` pi im, `(-8) ^ (1/3)` 1 + 1.732...im while `cbrt(-8)` is -2,
and `asin(2)` pi/2 + 1.317...im, a real argument on a cut taken as x + 0i,
as C, Python and NumPy take it (S341). `expr_context` gains its first
field, `real_results`: set, an operation of real operands whose value is
complex alone, `complex(x, y)` apart, is a domain error (S340). `log1p`,
`expm1`, `asinh`, `acosh` and `atanh`, the C core's since chunk 37, join the
table with `sign`, and `min` and `max` of any number of arguments, IEEE
754-2019's `minimum` and `maximum` in a type and the least or the greatest
argument itself as a value (S203, S338): 51 functions. The folds take
`math::complex<T>` for each floating-point T, where a real number stays
real, so that a term whose every number is real has its bits in T, plus
+0i, and `sqrt(-4)` is 2i (S339). Chunk 42, the second, gives `exp2`,
`log2`, `log10`, `log1p` and `expm1` of a complex number, and `log2` and
`log10` of a negative number and `log1p` of one below -1, their principal
values, `cplx.h`'s new `_c` forms (S347): `log2(-8)` is 3 + 4.532...im, its
real part 3 exactly; none of theirs at a Gaussian rational is one but a
real value, so each is dense; and `real_results` makes the real ones a
domain error.

**Matrices of complex numbers.** Step 23's third part (chunk 43), which
ends the step. A matrix holds a complex element as it holds a real one
(S348): densely, as `math::complex<T>` of the widest type, where every
element is a float, a double, a long double or a complex number of such
parts, a real one x then x + 0i; as values, each of its own kind, where any
is exact or boxed. Its arithmetic is the kind's, a complex number among the
scalars: dense in the widest type, element by element by `cplx.h`'s
operators, a real operand taken as itself, a product's sums from 0 in order
as the face's kernels sum them; exact for exact elements (S349). `A'` is
the conjugate transpose and the new `transpose(A)` the transpose, of 52
functions now; `real`, `imag`, `conj` and `arg` of a matrix are its
elements', and `complex(A, B)` of two of one shape the matrix of
`complex(a_ij, b_ij)` (S350). `det`, `inv` and `solve` are `cmat.h`'s bit
for bit where dense and exact where exact; `norm` is of the moduli, scaled
where dense; `dot(a, b)` sums conj(a_i) b_i; `cross` conjugates nothing
(S351). `eig` gives every eigenvalue: a real matrix's complex pairs by
`decomp.h`'s `d_decomp_eigen`, a Hermitian matrix's real by `cmat.h`'s
Jacobi method, any other complex matrix's by `d_cmat_eigen`, and an exact
matrix's exact wherever they are Gaussian rationals, `eig([0, -1; 1, 0])`
being `[-im, im]`; a spectrum ascends by real part, then by imaginary part,
and `real_results` makes a complex spectrum of a real matrix a domain error;
each solver is given 30 QR steps a row, at least 1000, the budget under which
a matrix of 700 rows ran out (S352). No node of the language answered
`not_yet` then but `jm` and `km`, step 24's (S353). The C++ face gives
vectors of `math::complex<T>` `vdot`, the conjugating inner product, and
`norm`, `length`, `norm_squared`, `normalize` and `distance` of the moduli,
and matrices of them `frobenius_norm` and `is_unitary` (S354); and the
generic kernels' norms
sum `number_modulus_squared`, so that a `vector<value>` holding complex
numbers has the norm of its moduli, the language's, exactly (S355).
`t_expr_complex_matrix_face.cpp` checks them, the eigenvalues against
references mpmath computes, `math_tests_expr_cmatrix_refs.h`.

**Whole powers in pairs.** Chunk 45 (S370, as S368 recommended).
`cplx.h`'s whole powers, z^n for n from -2^30 to 2^30, are repeated
squaring carried in long double's pairs, each product's parts exact sums of
exact products, the result scaled by a power of two and rounded once: the
`_c` kernel's to each family, and the plain forms by the same kernel, bit
for bit, as no C library has a faster whole power to lean on. A power exact
in twice long double's precision comes exactly; each part is within a step
of the part correctly rounded where it is at least |n| 2^(4 - P) of the
larger part, P long double's precision, and below that, where it may cancel
past what the pairs carry, within |n| 2^(3 - 2P) of the larger part, so a
part that cancels is no longer some steps of the larger part off, as chunk
44's references found. `quat.h`'s `pow_c`, whose plane's power it is, is
bounded of each part again. `t_cplx.c` checks 1,027 powers in the three
families at points where a part cancels, against `math_tests_cplx_pow_refs.h`,
made by mpmath.

**Quaternions in the language.** Step 24's second part (chunk 45), as the
owner's rulings S275 and S277 have it. `expr/quaternion_kind.hpp` makes a
quaternion a boxed kind of the value, of rank 950, past the complex number's
900 and below the matrix's 1000: exact where its four parts are, kept as
values, and otherwise dense in the widest floating-point type of its parts,
`math::quaternion<T>`, `quat.h`'s array; an exact one whose `jm` and `km`
parts come to 0 is the complex number of the other two, or a real one, and
an inexact one keeps its four parts (S371). `jm` and `km` are its units, so
`1 + 2jm - km` is one. Its arithmetic is exact on exact parts, Hamilton's
product of sixteen products and `p / q` = `p inv(q)`, the quotient on the
right, and dense by `quat.h`'s accurate forms, each part rounded once, a real
operand taken as itself and a complex one with its `im` the first unit
(S372). `|q|` is the norm, exact where it is rational, `|1 + 2im + 2jm +
4km|` being 5; a quaternion equals a number whose parts equal its own and
stands in no order; it converts to a real type where its other parts are 0;
its text is `1 + 2im - 3jm + 4km` (S373). `quat(w, x, y, z)` makes one,
`parts(q)` gives the row [w, x, y, z]; `real`, `conj` and `q'`, `abs` and the
bars, `inv` and `sign` take one, exactly where they can; `exp`, `log` and
`sqrt` are the complex functions in q's plane, `quat.h`'s `_c` forms, `sqrt`
exact where the root is a quaternion of rationals; `q ^ r` takes a real
exponent, exactly to a whole one and to a rational one whose root is
rational, `(-46 + 3im + 6jm + 6km) ^ (1/3)` being `2 + im + 2jm + 2km`; and
the others, `imag` and `arg` among them, and any complex or quaternion
exponent, are a domain error (S374). `rotate(q, v)` is the 3-vector q v q^-1
of any q not 0, exact where q and v are, of v's shape (S375). Simplification
keeps a quaternion's factors in their order, and a complex number's beside
one, so that `jm * km + km * jm` stays; a real number and a variable still
commute (S376). In a type `jm` and `km` are NaNs, `quat` its real or complex
value where it is one, and `real_results` leaves `quat` apart, as it does
`complex` (S377). A literal holding a quaternion, a product or quotient of a
matrix and one, and linear algebra's functions of one but `inv` answered
`not_yet` until chunk 46, matrices of quaternions (S369).
`t_expr_quaternion_face.cpp` checks them: the kind, exact arithmetic against
the test's own quaternions of rationals, dense arithmetic and the functions
and rotation in each type `quat.h`'s bit for bit, the language, the folds
in each type, the renderer and simplification.

**Matrices of quaternions.** Step 24's third part (chunk 46), as the
owner's ruling S277 has it, which ends the step. A matrix holds a
quaternion as it holds a complex number (S386): densely, as
`math::quaternion<T>` of the widest type, where every element is a float, a
double, a long double or a complex number or a quaternion of such parts, a
real one x then x + 0im + 0jm + 0km and a complex one x + yi x + yim + 0jm +
0km; as values, each of its own kind, where any is exact or boxed. Its
arithmetic is the quaternion kind's own, element by element: the generic
kernels compute on the kind's operands, `expr_quaternion_operand`, whose
operators are the kind's dense arithmetic, a real or complex operand taken
as itself, so that each element of a sum, of a product's sum or of a scaling
is the kind's own operation on its two, bit for bit; a product's element
sums a row's products with a column's, a_ik b_kj in that order, from 0;
and a number's product with a matrix keeps its order, `q * A` and `A * q`
two matrices, `A / q` being `A inv(q)` (S387). `A'` conjugates each
element, `transpose` moves it, `real` and `conj` of a matrix are its
elements', and `imag`, `arg` and `complex` of one a domain error. `inv` and
`solve` are the generic kernels' Gauss-Jordan and elimination of [A | B],
each product in its order and each row divided on the left, as A X = B
asks: on the kind's operands where dense, and on values whose traits say
they do not commute where exact, so that an exact inverse is exact; a
singular matrix's a pole. `det` is a domain error, no determinant of
quaternion matrices being both multiplicative and the complex one on
complex matrices; `trace` is the kernels', `norm` of the moduli, `dot(a,
b)` sums conj(a_i) b_i in that order, and `cross` conjugates nothing, each
product in its order (S388). `eig` gives Zhang's standard right
eigenvalues, the n complex ones of imaginary part 0 or more to one of which
each right eigenvalue is similar: those of the complex adjoint
[A1, A2; -conj(A2), conj(A1)], A = A1 + A2 jm, 2n by 2n, found as a complex
matrix's are, by Jacobi where A is Hermitian and exactly where they are
Gaussian rationals, one of each conjugate pair; a triangular matrix's are
its diagonal's, each w + |v|i (S389). A quaternion is a scalar to linear
algebra too: `det(q)` a domain error, `trace(q)` q, `norm(q)` |q|, `eig(q)`
its standard eigenvalue, `dot(p, q)` conj(p) q, and `solve(q, B)` q^-1 B, on
the left, `quat.h`'s left quotient where dense (S390). `A ^ 0` is the
identity of A's storage, a dense complex matrix's too, which was the exact
identity until now. No node of the language answers `not_yet` (S369).
`t_expr_quaternion_matrix_face.cpp` checks them, the eigenvalues against
references mpmath computes, `math_tests_expr_qmatrix_refs.h`.

**Rotations.** `expr/rotation.hpp`, new in chunk 46, holds a quaternion's
functions that give or take a vector or a matrix, so that the matrix kind,
which holds quaternions now, comes after the quaternion kind: `parts` and
`rotate`, from `quaternion_kind.hpp`, and three new ones (S391). `rotmat(q)` is
the 3 by 3 matrix of the rotation q names, R v = q v q^-1, as MATLAB's
`rotmat(q, 'point')` is: exact where q is, and otherwise `quat.h`'s
`to_matrix3`, each element within a step of the correctly rounded one.
`rotquat(R)` is the unit quaternion of a rotation's matrix, w >= 0. Where R
is exact and exactly a rotation's, R^T R = I and det(R) = 1, it is
Shepperd's method as `quat.h` computes it: exact where the root its largest
candidate takes is rational, `rotquat(rotmat(1 + im + jm + km))` being
`1/2 + im/2 + jm/2 + km/2`, otherwise `quat.h`'s `from_matrix3` in long
double. Every other R, a dense one or an exact one that is no rotation's, is
taken as a rotation's matrix that has drifted, and gives the quaternion of
the rotation nearest it in the Frobenius norm, by Bar-Itzhack's method
(2000): the eigenvector of the largest eigenvalue of a symmetric 4 by 4
matrix R's sums and differences make, by `decomp.h`'s cyclic Jacobi, in the
elements' widest type (chunk 53, S429). A reflection, det(R) <= 0, and a
matrix farther than 1/2 from the nearest rotation's, far from any, are
domain errors. Of a 4 by 4 matrix it takes the upper left. `slerp(p, q, t)`
is the shorter arc between the rotations p and q name, `quat.h`'s accurate
slerp of the two normalized: 58 functions. MATLAB's equivalents, in its
navigation, sensor fusion and robotics toolboxes, are `rotmat(q, 'point')`
and `quaternion(R, 'rotmat', 'point')`, with the Robotics toolbox's
`quat2rotm` and `rotm2quat`; SciPy's are `Rotation.as_matrix` and
`Rotation.from_matrix`, which orthogonalizes a matrix that is no rotation's
before it converts it, as rotquat takes the nearest rotation (S391, S429).

**The owner's rulings S336 and S340.** Chunk 46 makes them, as he ruled on
2026.11.04. S336: `math::complex<T>`'s functions by their own names are
`cplx.h`'s plain forms at run time, on the C library's real functions, and
its `_c` forms in a constant expression from C++20; the names ending `_c`,
`exp_c` and the rest, are the `_c` forms at every level, constant
expressions from C++14. The language's values take the `_c` forms, so that
a value has the same bits on every build, and the fold in
`math::complex<T>` the plain ones, as a fold in a real type takes the C
library's. S340: every evaluator of the function table takes the fold's
`expr_context`, which moves to `expr_common.hpp`, so that an evaluator is
`Type (*)(const Type*, std::size_t, const expr_context&)`; every value
evaluator holds its own answer real where the context asks
(`expr_real_only`), so that the fold no longer judges an answer after the
call; and the typed folds take a context too.

**`dynamic_matrix` of every element type.** Chunk 46 (S354, S392).
`linalg::dynamic_matrix<T>` takes every element type the fixed-size matrix
takes: float, double and long double on the C core's kernels, any other
arithmetic type through double, and `math::complex<T>`, `math::value` and
`math::inline_value` on the generic path, each zero, one and tolerance the
traits', the `frobenius_norm` of a complex one a T, as the fixed-size
matrix's is. Quaternions neither face's matrix takes, as the quaternion has
no sums or traits; the language's matrices of them compute with the kind's
own operands.

**A term compiled once.** Chunk 47 (S395). A fold walks the tree at every
evaluation: it builds a vector of its children's answers at each node and
looks each function and constant up by name, and in `double` it takes 79 to
624 times the C++ it stands for (chunk 47's benchmark, five terms).
`expr_compile<T>(term)`, in `expr/program.hpp`, does that work once. It
gives an `expr_program<T>`, for `float`, `double` or `long double` or the
`math::complex` of one: the term's operations in postfix order, each literal
already a number of T, each constant and function found, each variable a
slot. A run is a loop over those steps on a stack of T whose depth is known
at compile time, held in the caller's frame up to
`D_CFG_MATH_EXPR_PROGRAM_STACK` (32, in `cfg_expr.h`), so a run allocates
nothing; in a complex type too, as the complex fold's operations take their
operands where they lie, but for a call of more than eight arguments.

A run is the typed fold, not an approximation of it. Postfix order is the
order in which the fold computes, children left to right and then the
parent, and each step calls the function the fold calls on the same numbers.
So each answer is the fold's bit for bit, and each error is the fold's
first: the errors of a term's shape (an unknown name, a wrong number of
arguments, a vector) at compile time, an unbound variable at a run of
bindings. A run takes the variables' numbers in slot order (the term's own
order unless another is given), `math::value`s, or bindings by name. The
handle compiles too, `parse("x ^ 2 - y").compile<double>()`. A run of
doubles takes 2.9 to 11.4 times the C++, a run of bindings 1.5 to 3.1 times
a run of doubles, and a compile pays for itself at the second run.
`t_expr_program_face.cpp` checks it against the fold on 2,000 random terms
in each of the six types, every operator, call, constant and literal at
every sort of value, and on its own edges.

**One header per concept.** Chunk 48 (S398 to S401). The framework had two
quaternions, `math::quaternion` and `linalg::quaternion`, each a face of
`quat.h`, and two of every function, so that a call such as
`from_matrix(m)` of a `linalg::matrix` was ambiguous wherever both were
read. Now `math::quaternion<T>` is the one class, with linear algebra's
names, members `m_x`, `m_y`, `m_z` and `m_w`, `to_matrix3` and the default
element type `double`, and `linear_algebra/quaternion.hpp` names it and
each of its functions in `linalg` by using-declarations, so that
`linalg::from_axis_angle` and `math::from_axis_angle` are one function
(S398). `math.hpp` includes every header of the module, `expr/` and
`linear_algebra/` among them, with its entry points first (S399). Of the
language's headers, the four whose names another math header had are named
for what they hold, `complex_kind.hpp`, `quaternion_kind.hpp`,
`matrix_kind.hpp` and `summation.hpp` (S400); and the two largest are
split along their concepts: `function.hpp`, 4,363 lines, into the tables
(`function_table.hpp`), the arithmetic (`arithmetic.hpp`) and the relations
and logic (`logic.hpp`), and `matrix.hpp`, 7,979 lines, into the kind
(`matrix_kind.hpp`, 4,811) and linear algebra's functions of it
(`matrix_function.hpp`, 3,256) (S401). No code changed in the move: every
test's output is the same, byte for byte.

**Compensated sums and a refined solve.** Chunk 49 (S404 to S408). The C
core's dot products, sums, cross products, matrix products and traces
were the formulas as written, each product rounded and summed in order,
their error unbounded where the terms cancel. Now each is a sum rounded
once from Ogita, Rump and Oishi's Dot2 or Sum2 - each product exact as a
pair by Dekker's product, the terms summed by two-sums, the errors summed
apart and added last - as accurate as the sum computed in twice the
precision and rounded once, in pairs of double for the float and double
families where double is evaluated in its own type and of long double
otherwise; operands outside the range where the products are exact are
scaled by a power of two, a NaN or an infinity gives the formula's value,
and an exact zero is +0 (S404). `solve` refines each column of its
solution as LAPACK's `dgerfsx` does: the residual by Dot2, the solution
carried with a tail, corrected through the LU factors it keeps until the
corrections fall below half an ulp or stop shrinking, at most
`D_CFG_MATH_MAT_REFINE_STEPS` steps (`cfg_mat.h`), so that a system well
away from singular is solved correctly rounded, entry by entry (S406,
S407). Each kernel keeps its formula as written, bit for bit, under a
`_fast` name, `dot_fast` to `solve_fast`, in the C core and both faces;
the norms keep their sums, which cannot cancel (S405).
`t_compensated.c` checks each result exactly, by integers of 768 bits,
against Dot2's guarantee, and the refined solve against 65 systems whose
solutions `math_tests_solve_refs.h` holds exactly rounded. The complex
dense kernels follow (S408, flagged).

**Complex matrices compensated and refined; projections scaled.** Chunk 50
(S409 to S413). `cmat.h`'s product of complex matrices, new as
`d_cmat_multiply`, rounds each part of each entry once from Dot2's pair --
the real part's products re re and -im im, the imaginary part's re im and
im re, each exact as a pair - through an accumulator `vec.h` now keeps for
callers whose terms are not arrays of one family; `d_cmat_solve` refines
its solution as `d_mat_solve` does, part by part, and takes the factors'
room, `(x, scratch, pivots, a, b, n, m)`; their formulas are
`d_cmat_multiply_fast` and `d_cmat_solve_fast`, the old bits (S410). The
generic path's complex sums and solve mirror them in C++ on the same
accumulator, so the faces' matrices of `math::complex<T>` are `cmat.h`'s
bit for bit, from C++98 and as constant expressions from C++14 (S411), and
the language's complex products, a real operand's element taken as
itself, are the kernels' (S412); its solves stay unrefined (S413,
flagged). `project` and `reject` scale both operands where the squared
norm or the ratio would leave the range, so that a projection in range is
computed in range wherever its operands lie, every bit in range kept
(S409, ruled as S301). `t_compensated.c` checks every part of 120,000
complex dots exactly and the refined complex solve against 65 systems
whose solutions `math_tests_csolve_refs.h` holds exactly rounded.

**The double family: exp correctly rounded, pairs, a self-test.** Chunk 51
(S414 to S423), the first of the double family's rewrite under the owner's
rulings of 2026.11.16. `d_math_d_exp_c` is the exponential correctly rounded
on every platform in double arithmetic alone, the lane's own algorithm in
three phases, each bound proven with Sollya and Gappa from the code's own
operations, fused or not: x reduced by ln2/128 against a table of 2^(j/128)
in 128 pairs; phase 1 in doubles, which decides 99.86% of arguments; phase 2
in pairs; phase 3 in fixed point, within 2^-218.3, for the arguments within
2^-100 of a rounding boundary. It costs 9.9 ns a call over its range,
against the C library's 6.7 and chunk 50's 266, and is a constant
expression from C++14 through all three phases (S416); the plain
`d_math_d_exp` stayed the C library's until the owner's ruling 12 (chunk
63, below). `dd.h`,
new, is a public pair of doubles, `struct dj_math_dd`: the exact sum and
product, then sums, products, quotients and a square root, each within a
proven bound (S417). Every exact product in the core takes its low part by
an explicit fused multiply-add where the target has one, the bits Dekker's
product gives where it has none, so that a compiler that fuses cannot break
it, and every generated header tells clang not to fuse (S414). Where double
is evaluated wider than double, as on a 32-bit x87, a translation unit that
calls the double family's correctly rounded functions does not compile, the
message naming the flags, `-msse2 -mfpmath=sse` (S418). `self_test.h`, new,
says at start-up whether the build keeps those guarantees,
`d_math_self_test()`, 0 where it does, and whether it fuses the plain
formulas, `d_math_formulas_fuse()` (S419): a program that needs the same
bits everywhere calls the first once and stops where it is not 0.
`t_exponential.c` checks `exp_c` against 2,398 references made by mpmath at
400 bits, each phase alone at every one (S422), and `t_dd.c`, new, every
pair operation exactly, in integers of 768 bits.

**The double family: log correctly rounded.** Chunk 52 (S424 to S427), the
second of the rewrite. `d_math_d_log_c` is the natural logarithm correctly
rounded on every platform in double arithmetic alone, as `exp_c` is: x = m
2^e reduced against a table of 129 inverses of 1 + i/128 and their
logarithms, so that log(x) = e ln2 + L + log1p(r), r exact as a pair and
below 2^-8; phase 1 in doubles, which decides 99.93% of arguments; phase 2
in pairs; phase 3 in `exp_c`'s fixed point, within 2^-165.8. Each phase's
bound is proven with Sollya and Gappa in each of three cases - e not 0;
e = 0 with L and log1p(r) partly cancelling; x near 1, where r = x - 1 --
and each phase's test checked sound (S424). It cost 18.2 ns a call, against
the C library's 3.9 and the long double kernel's 157 to 175, and is a
constant expression from C++14 through all three phases; its speed was
flagged, and chunk 54 made it faster (below). `t_exponential.c` checks it against 2,354 references made by mpmath
at 400 bits, each phase alone at every one, and `d_math_self_test()` takes
it at four arguments, its bit 4 (S425). On the x87 it refuses to compile,
as `exp_c` does (S426).

**The compile-time core at C++11, and 0.8's items.** Chunk 53 (S428 to
S431), step 29 (5a), with the items the owner ruled on 2026.11.16 for 0.8.
`expression.hpp` compiles from C++11 and is empty below it (N1): its nodes,
+ - * /, `squared`, `abs` and `dot` are constant expressions from C++11,
and `exp`, `sin`, `cos`, `tan` and `sqrt` are the C core's `_c` kernels at
compile time and at run time alike, constant expressions from C++14 (N3,
S430). Its own kernels gave `sqrt(1e300)` as 7.89e269, `exp` up to 13,408
ulps off and `sin(1e22)` a NaN, and a C++20 program had two answers to one
call; now there is one. Its placeholders `x` to `phi` live in
`djinterp::math::variables` (S403): bring them in with `using namespace
djinterp::math::variables;`, and `math.hpp` is clean under `-Wshadow`.
`t_expression_face.cpp`, new, checks it at every rung. The language's dense
solves, real and complex, are the C core's refined ones, each part
correctly rounded where the system is well conditioned, as `la::solve`'s
are (S428). `rotquat(R)` gives the quaternion of the rotation nearest R, by
Bar-Itzhack's method, where R is no exact rotation's matrix, and refuses a
reflection or a matrix far from any rotation (S429); the docs list MATLAB's
and SciPy's equivalents (section 2's rotations).

**The double logarithm faster.** Chunk 54 (S432 to S434), the next step of
6a. `log_c` is 1.70 to 1.85 times as fast as chunk 52's, its results the
same bits at 26 million arguments: the table's inverses are of 26 bits and
held halved from i = 53, so that m is never halved - a branch that a
random argument mispredicted half the time - and m inv is Dekker's product
with m alone split; every positive normal x is taken in one test; phase 1
sums rh^2 q(rh), q of degree 6, and decides where its result is within
2^-90 of itself plus 11 2^-55 rh^2 of the logarithm, the shape of its own
error, with no branch (S433). It costs 12.2 ns a call where chunk 52's took
22.2 and the C library 5.3 (gcc -O2), 151 instructions where chunk 52's
took 206; 1.95 to 3.03 times the library's time across compilers and
arguments, still past the twice that ruling 12's recommendation asks in
most of them. Each
bound is proven again, phase 1's in two parts, Z and W, joined in exact
arithmetic (chunk 54's record, `proofs/`).

**The largest files in parts.** Chunk 55 (S435 to S440), as the owner ruled
on 2026.11.21: no modules of math's own, but no file past about 4,000 lines.
The generator writes the eight largest headers in 44 parts, from 3,989
lines down, each a header that stands alone, and the header of each name
includes its parts in order: the code an includer sees is the same, token
for token, in the same order (S436). `expr/matrix_kind.hpp` is in three
parts, the storage, the arithmetic and the transpose (S437); the face test
`t_expr_face.cpp` in eight, one program with the same 520 checks (S438);
and the generator, 32,000 lines, is a package of 26 modules, writing the
same bytes (S439). The generator also aligns each parameter list's names,
as the owner aligned `cmat.h`'s, and writes the banners' converted tag
(S435). No result changed.

**Functions, sampling and the bridge from C++11.** Chunk 56 (S441 to
S446), step 30 (5b): `function/function.hpp`, `values.hpp` and
`functional.hpp` stand on the compile-time core from C++11, each empty
below it as the core is (S441). Each constexpr function is one return, the
index sequences are re_std's, and the traits' `_v` forms wait for variable
templates: the relations, the logic, implicit functions and level sets are
constant expressions from C++11, the forms over `std::tuple` and
`math_function`'s calls from a `std::array` or a `std::tuple` from C++14,
where `std::get` is (S442). Two of the headers' own failures went on the
way: `math_function` now takes a coordinate system's point, a
`linalg::vector`, as its banner said it did (S443), and `sample_over`
samples an interval's members, where an interval open below gave its bound
and lost its last member, (0, 4] giving 0 to 3 (S444). Three face tests
check them at every rung, the bridge's with the functional subframework
itself (S445).

**One hyphen for a dash.** Chunk 57 (S447 to S449) cut chunks 55's and 56's
zips again on the owner's push of 2026.11.23, which gives the tree's prose
one hyphen for a dash: the lane's files have it too, and so do the strings
the generator writes its headers' comments from, so that its `--check`
holds on his main again (S448). No C or C++ token changed.

**The calculus from C++11.** Chunk 58 (S450 to S455), step 31 (5c), and
with it step 33 (5e): the six calculus headers compile at every level, each
empty below C++11 as the core they stand on, and `math.hpp` compiles at
every rung with them, so that the ladder needs no math line (S450). A form
that is one return is a constant expression from C++11: each derivative,
the higher derivatives and the gradient, the 5-point rule, the Taylor and
Maclaurin evaluations, the arithmetic sequence and sums, `factorial` by
halving its product and `gcd` by Euclid's steps; one that loops as many
times as an argument says, reads a `std::array` or calls a transcendental
is one from C++14, the same code at run time below it, so that every form
keeps its bits but `log` and `atan` (S451). Each derivative rule takes
`wrt<I>`, so that its type is deduced from its arguments, as GCC needs at
C++11 (S452). `log` and `atan` are the C core's kernels: the header's own
logarithm was not correctly rounded at a third of 20,000 arguments, and at
the smallest subnormal had 13 of its 53 bits right; its arctangent was 4.25
ulps off (S453). `t_calculus_face.cpp` checks the six headers at every rung
(S454).

**From compile time to run time.** Chunk 59 (S456 to S462), step 32 (5d):
`math::reify(e)`, in `expr/reify.hpp`, makes a compile-time expression a
term of the run-time language, node for node in its shape, nothing
rewritten, so that it prints, simplifies, substitutes and evaluates by
name, and `math::expr` wraps it (S456). Its variables are x, y, z and t by
index, or the names given, `reify(e, "r", "theta")`, each checked at
compile time (S457). A constant that is an integer below 2^53, -0 apart, is
the exact integer, so that `expr_simplify` collects a reified derivative's
coefficients: `derivative<0>(x * sin(x))` reified is `1 * sin(x) + x *
(cos(x) * 1)`, simplified `sin(x) + x * cos(x)` (S458). A piecewise's last
branch is its otherwise, as C++ evaluates it, and arithmetic in an integer
type is refused (S459). In double the term's arithmetic, sqrt and abs give
the expression's value bit for bit; its exp, sin, cos, tan, log and atan are
the language's plain functions, one ulp from the C core's correctly rounded
kernels at 1 to 56 of 20,000 arguments each, until ruling 12 (S460); exp's
since chunk 63 the kernel's, bit for bit (S480). The
run-time derivative is step 25's (4a), which takes the term when it comes.
`t_reify_face.cpp` checks reify at every rung (S461).

**The lazy type.** Chunk 60 (S463 to S468), step 34 (5f), as decision 2
asks: `lazy(x)`, in `linear_algebra/lazy.hpp`, wraps a vector, a matrix or
a dynamic matrix by reference, and its operators build an expression that
`eval(e)` or `assign(dst, e)` computes each element once, with no temporary
a step: sums, differences, negations, scalings, a vector divided,
`hadamard`, the matrix products and `transpose` (S463). Each element is the
eager arithmetic's, bit for bit: its operations in the eager loops' order,
the products and transposes the eager operations' own, computed once before
the elements, and each product of the loop fenced where the build may fuse
a multiply and an add, as gcc's fuses a fifth of them otherwise (S464).
Operands are lazy on both sides and held by reference, `lazy` of a
temporary refused (S465); `assign` writes in place, safe where the
expression reads the destination (S466). For a four-step expression of
dynamic matrices of double, from 4 x 4 to 512 x 512, `eval` took a half to
a quarter of the eager arithmetic's time and `assign` a third to a fifth,
and about half where the fence is (chunk 60's record, `raw/measure/`).
`t_lazy_face.cpp` checks it at every rung (S467).

**The style debt.** Chunk 61 (S469 to S473), step 35 (5g), which closes
Phase 5. The compile-time headers' template parameters, and `reify.hpp`'s,
no longer begin with the underscore and capital the language reserves:
`Type`, `T`, `Args`, as the style guides spell them, each name everywhere in
its file, the code token for token as it was and its objects the same bytes
(S469, S472). Their Roman-numeral sections are numbered 1. on, each header
with a table of contents (S470). The four functions of math past the
owner's limit of 100 lines, two in `decimal.h`, one in `wide.h` and
`parse_number`, are split along a seam each, every result as before
(S471). Left as they were: the banners' HOW TO USE, OPTIONS and
CONFIGURATION, which 91 of math's headers did not have yet, and two
`created: TBA`, both on the owner's punch list (S473).

**The banners' sections.** Chunk 62 (S474 to S477), as the owner ruled
S473 on 2026.11.24. Every header of math has the three sections the
style guides ask of a banner under its description: HOW TO USE, how a
caller reaches the header and the names it gives; OPTIONS, what a run or a
caller chooses, and how; CONFIGURATION, the `D_CFG_` knobs it reads, the
levels it compiles at and any flag it needs (S474). The 91 hand-written
headers that lacked them have them; the 64 the generator writes take
theirs from `scripts/gen_c_math/banner_sections.py`, which refuses a new
header without them, each part of a split header pointing to its header
(S475). The code is chunk 61's token for token (S476), and, as the owner
ruled, its objects are too, by g++ and clang over every smoke build, two
tests' differing only in their asserts' line numbers (chunk 63, S481). The
two `created: TBA` stay, their dates not known (S477).

**The plain double exp correctly rounded.** Chunk 63 (S478 to S482), as
the owner ruled on 2026.11.24: ruling 12 makes each plain double function
the correctly rounded one once that is within about twice the C library's
time, and his decision 48 asked for exp's switch first. `d_math_d_exp` is
`d_math_d_exp_c` wherever double is evaluated in double, so that every
plain exp call, the language's `exp` and the complex numbers' plain
exponentials among them, gives the correctly rounded value, the same bits
on every platform and at compile time (S478); on a 32-bit x87 it stays the
C library's, so that the plain form builds everywhere (S479). Measured
again, the plain exp takes 1.3 to 1.7 times the library's time over exp's
whole range but 2.1 to 2.6 on ordinary arguments with gcc, up to 3.0 with
clang, past the twice there; chunk 65's speed step brought it to 1.6 to 2.2
(S489), and `d_math_d_exp_fast` is there for glibc's time (S490), as the
owner's note asked. `t_exponential` checks the plain exp against `exp_c`
bit for bit at ten million arguments, and `t_reify_face.cpp` a reified
term's exp against the expression's (S480). The float and long double
plain exp, and every other plain double function, stay the library's
until each comes within twice; `log` switched after its speed step, in
chunk 66 (S496).

**The scalar helpers.** Chunk 64 (S483 to S488), as the owner's ruling 13
has them, step 52 of 62. `c/modules/math/scalar.h`, generated, gives each
family `abs`, `copysign` and `is_finite`; `min` and `max`, IEEE 754-2019's
minimum and maximum, a NaN in either giving that NaN and -0 below +0, as
the expression language's (S338); `min_num` and `max_num`, which pass a NaN
over, as C23's `fminimum_num` does; `clamp`, max then min, which keeps a
NaN; `lerp`, C++20's, exact at both ends, monotonic and never past an end;
and `ldexp` and `frexp` (S483). Each is exact: `abs`, `copysign`,
`is_finite`, `ldexp` and `frexp` come twice, the `_c` form pure arithmetic
and the plain form the C library's, bit for bit the same; the rest, which
C99 lacks, once; every one but the plain forms a constant expression from
C++14 (S484, S485). `t_scalar` checks them against the C library, against
glibc's C23 `fminimum` and its kin, and in C++20 `lerp` against
`std::lerp` where the target has no fused multiply-add, each to the bit
(S486). The owner's note on chunk 63's exp,
a fast and a slow form unless `exp_c` can match glibc, is step 53's plan
(S487); his ruling numbered the terps' needs as steps 52 to 62, Phase 9
(S488).

**exp's speed, and a fast exp.** Chunk 65 (S489 to S494), step 53 of 62,
as the owner's note on chunk 63's exp has it: a fast and a slow form,
unless `exp_c` can be made as good as glibc's. `exp_c`'s common path,
2^-53 < |x| <= 708 with its first phase deciding, is now small enough to
inline: its second and third phases are out of line, under
`math_common.h`'s new `D_MATH_FN_COLD`; the table holds each high part
with its split; the reduction's index is taken without a division; there
the result is normal, so the subnormals' test is skipped; and the range is
tested by x^2, with no branch on the sign. The bits are the three phases'
at every argument. On ordinary arguments the plain exp, which is `exp_c`,
went from 2.5 to 2.9 times glibc's time to 1.6 to 2.2 (S489): faster, but
not glibc's, whose exp this machine runs with fused multiply-adds. So
`d_math_d_exp_fast` sits beside it, e^x within 0.511 of an ulp, proven by
Gappa whether or not the build fuses, in glibc's shape and in pure
arithmetic: 0.8 to 1.07 times glibc's time (S490). It is the double
family's alone, with no C++ overload, never a constant expression, `exp_c`
outside the common path and the C library's on a 32-bit x87; the plain exp
stays `exp_c`, as ruling 12 has it (S491). `t_exponential` checks the
common path against the three phases at ten million arguments, and
`exp_fast` within an ulp of `exp_c` and within 0.511 of the exact value
(S492). The owner's question of 2026.11.25, how other libraries handle a
NaN, found one precedent named wrongly: Eigen's inverse of the zero
quaternion is the zero quaternion, not NaNs; `quat.h`'s stays NaN, as the
formula and GLM give it (S494, correcting S359's reason).

**log's speed, and the plain log correctly rounded.** Chunk 66 (S495 to
S499), step 54 of 62, as ruling 12 has it: a speed step for `log_c`, then
the plain double `log` switched once it is within about twice glibc's
time. Where the language reads a double's bits, as every build of the
tiers does, `log_c`'s reduction takes e, the table's row and m's split for
the exact product from them by integer operations, where it took them in
floating point; its second and third phases are out of line under
`D_MATH_FN_COLD`, as exp's are; a positive normal x is one comparison of
its bits; and the first phase's absolute value is a negation and a
maximum. The reduction gives the same values at every argument, so the
bits are the three phases' and chunk 54's proofs hold as recorded. A call
takes 7.0 ns where it took 10.2 (gcc -O2, every binade): 1.6 to 2.6 times
glibc's time where it took 2.4 to 3.6 (S495). So `d_math_d_log` is now
`d_math_d_log_c`, the correctly rounded logarithm, wherever double is
evaluated in double, and through it the language's `log` and the complex
numbers' plain functions that take a real logarithm; on a 32-bit x87 the
C library's (S496, S497). `t_exponential` checks the plain log against
`log_c` bit for bit, the common path against the three phases and the
reduction against one by `frexp` and an exact product, each at ten million
arguments; `t_reify_face.cpp` a reified term's log against the
expression's (S498). S424's flag closes; S460's narrows to sin, cos, tan
and atan (S499).

**A fast log.** Chunk 67 (S500 to S504), as the owner's decision 71 of
2026.11.25 asked, before step 55. `d_math_d_log_fast` sits beside `log_c`
as `exp_fast` sits beside `exp_c`: log x within 0.53 of an ulp, glibc's
method in pure arithmetic, proven by Gappa whether or not the build fuses.
Near 1, x within [1 - 2^-4, 1 + 0x1.09p-4), it takes r = x - 1 and
log1p(r) as r - r^2/2 + r^3 B(r), r^2/2 split so that the first sum is
exact; elsewhere x = 2^k z, z in [0x1.6p-1, 0x1.6p0), a row of 128 from z's
bits, r = (z - chi - clo) inv, z - chi - clo exact, and log x = (k LN2H +
L1) + r + (k LN2L + L2) + r^2 A(r) (S500). It takes 1.10 to
1.24 times glibc's time by pointer and 0.84 to 1.04 inline where the build
does not fuse, 0.82 to 1.08 and 0.70 to 0.87 where it does; measured,
0.5210 of an ulp at worst. It is the double family's alone, never a
constant expression, `log_c` at the specials and subnormals and wherever
the language cannot read a double's bits, and the C library's on a 32-bit
x87; the plain log stays `log_c` (S501). With it `exponential_d.h` would
have passed 4,000 lines, so the double family's logarithms are a part of
their own, `exponential_d_log.h` (S502). `t_exponential` checks it within
an ulp of `log_c` and within each path's bound of the exact value at ten
million arguments, about nine seams, and `log_c`'s value where it hands
over (S503).

**Decimal text.** `decimal.h` reads a decimal number of any length as the
nearest `float`, `double` or `long double`, ties to even, whatever the
rounding mode, and says when the result overflowed to an infinity or
underflowed to an inexact subnormal or zero. It is the C core's `strtod`,
with one answer on every build and a constant expression from C++14. It
reads only the number itself: no space, locale, hexadecimal, infinity or
NaN, which are other tokens' business. The algorithm is Simple Decimal
Conversion, as in Go and Wuffs. Its buffer holds every digit that a value
halfway between two `long double`s can have, so the rounding is exact for
any input, however long.

**Literals.** `literal.h` reads a number's text as a value: an integer, a
decimal, scientific notation, a fraction (`2/3`) or a mixed number
(`1 1/2`). The value is exact wherever it is a rational over `dj_math_imax`,
so `0.1` is the rational 1/10 and `1 1/2` is 3/2. Otherwise it is the long
double nearest the literal, flagged inexact, as an exact result too wide
for `dj_math_imax` is. A fraction over 0 is a pole, or invalid for 0/0. A
literal past every long double, or a fraction whose terms pass two words,
is a range error until bignum exists. A decimal literal read straight into
a `float`, `double` or `long double` goes through `decimal.h`, rounded once.

**Comparing numbers as text.** `numeric_compare.hpp`, a C++ face from
C++98, compares two numbers written as text, in `literal.h`'s syntax,
exactly at any length: `0.1` is below `1/9`, and `1e100000000000000000000`
below `2e100000000000000000000`. It reads them as `rational_value`s, signed
rationals of any size times a power of ten of any size, whose digits are
`integer_value`s in a `std::string`; until bignum, the C core has no
arbitrary precision of its own. A text that is not a number, or a fraction
over 0, is unordered with everything, as a NaN is.

---

## 3. Layout

```
inc/djinterp/c/modules/math/            the C core
  math_common.h        kernel qualifiers, widest integers, evaluation width
  float_order.h        next_up, next_down, advance, steps between values
  interval.h           integer intervals (hand-written)
  interval_float.h     floating-point intervals
  rational.h           exact rationals, correct rounding to float (hand-written)
  value.h              the generic value: kinds, tower, errors, boxed kinds
                       (hand-written)
  wide.h               signed integers of 256 bits in limbs (hand-written)
  value_wide.h         the wide integer as a boxed kind of the value
                       (hand-written)
  decimal.h            decimal text to float, double, long double (hand-written)
  literal.h            number literals read as exact values (hand-written)
  elementary.h         sqrt, sin, cos, asin, acos, exact helpers
  exponential.h        exp, log, exp2, log2, log10, log1p, expm1; the
                       double exp_c and log_c correctly rounded in double
                       arithmetic (chunks 51 and 52), the plain double
                       exp and log those (chunks 63 and 66); the double
                       exp_fast and log_fast within 0.511 and 0.53 of an
                       ulp (chunks 65 and 67); in parts
  dd.h                 pairs of doubles: exact sums and products, proven
                       bounds (chunk 51)
  self_test.h          the double family's start-up self-test; whether the
                       build fuses (chunk 51)
  tangent.h            tan, atan, atan2
  hyperbolic.h         sinh, cosh, tanh, asinh, acosh, atanh
  power.h              cbrt, pow, hypot
  remainder.h          fmod, remainder, remquo
  nearest_integer.h    floor, ceil, trunc, round (whole range)
  scalar.h             abs, copysign, is_finite, min, max, min_num,
                       max_num, clamp, lerp, ldexp, frexp (chunk 64)
  vec.h, mat.h, quat.h vectors, matrices, quaternions
                       (each in parts: S436)
  decomp.h             LU, QR, Cholesky, SVD, symmetric Jacobi eigen,
                       balancing, Hessenberg form, real Schur form,
                       general eigenvalues and vectors (in parts)
  coord.h              coordinate conversions (in parts)
  isect.h, plane.h     ray intersections, planar polygons
  cplx.h               complex numbers: the arithmetic, C99's 22
                       functions, and exp2, log2, log10, log1p and
                       expm1 of a complex number (in parts)
  cmat.h               complex matrices: LU, inverse, solution,
                       Hermitian and general eigenvalues and vectors
                       (in parts)
  decomp_*.h, cmat_*.h, cplx_*.h, quat_*.h, vec_*.h, coord_*.h, mat_*.h,
  exponential_*.h      the eight headers' parts, each standing alone

inc/djinterp/cpp/modules/math/          the C++ faces
  math_common.hpp      NS_MATH and the environment
  constants.hpp        constants::of<T>::pi() and kin; number bases
  quaternion.hpp       the one quaternion: math::quaternion<T>, quat.h's face
  interval/            interval, closed/open/discrete, interval_common
  linear_algebra/      vector, matrix, square, transform, decomposition,
                       solve, svd, eigen, dynamic_matrix, linalg_common,
                       linalg.hpp (umbrella), and quaternion, which names
                       math::quaternion and its functions in linalg
  coordinate/          cartesian, polar, cylindrical, spherical, coordinate
  geometry/            13 headers and geometry.hpp (umbrella)
  expression.hpp       compile-time expression templates (C++11)
  values.hpp           sampling an expression
  function/            math_function, piecewise, parametric, implicit
  functional.hpp       bridge to the functional subframework
  calculus/            differentiation, integration, sequence, series,
                       elementary, calculus.hpp (umbrella)
  numeric_compare.hpp  numbers written as text, compared exactly
  value.hpp            the generic value: math::value and inline_value
  number_traits.hpp    number_traits, common_number and the concept number
  complex.hpp          complex numbers: math::complex<T>, cplx.h's face
  expr/                the run-time expression language (C++11): token,
                       operator, parse, term, evaluate, render,
                       substitute, simplify, derivative, numeric,
                       summation, antiderivative; function_table, the
                       function and constant tables; arithmetic, the
                       functions' evaluators and ^ and mod; logic, the
                       relations, logic and piecewise; complex_kind,
                       quaternion_kind and matrix_kind, the kinds,
                       matrix_kind in three parts (storage, arithmetic,
                       transpose: S437);
                       matrix_function, linear algebra's functions of
                       them; rotation, a quaternion's functions of
                       vectors and matrices; program, a term compiled
                       once for a type; expr_common, what they share and
                       a map of them; expr, the handle, which includes
                       them all
  math.hpp             umbrella: every header of the module
  README.md, roadmap.md, decisions.md, comparison.md

inc/djinterp/config/c/modules/math/     the C core's knobs
  cfg_value.h          how every box counts its holders
  cfg_value_wide.h     where the wide integer's boxes are stored

inc/djinterp/config/cpp/modules/math/expr/
  cfg_expr.h           the language's knobs: a compiled term's stack

inc/djinterp/cpp/modules/parse/expression/
                       the expression core the language is built on

tests/djinterp/c/modules/math/t_*.c     C harnesses, one per core header
tests/djinterp/c/modules/math/math_tests_*.h
                       their shared random numbers, exact reference and
                       mpmath's references
tests/djinterp/cpp/modules/math/**/t_*_face.cpp
                       face tests (t_expr_face.cpp in eight parts,
                       t_expr_face_*.hpp, one program: S438)
scripts/gen_c_math_float.py             the generator, which runs
scripts/gen_c_math/                     its modules (S439)
scripts/ci/check_math_tests.sh          the tests: smoke, full and fused tiers
scripts/ci/ladder.py                    the C and C++ ladders
                                        (+ ladder_known.txt, the known list)
scripts/ci/check_style.py               the guides' measurable rules
                                        (+ style_known.txt)
_handoffs/math98_*                      one record per delivered step,
                                        kept with the lane's deliveries,
                                        not in the tree (from 2026.11.21)
```

The run-time expression language, `math/expr/`, is built on
`inc/djinterp/cpp/modules/parse/expression/`, the framework's shared
expression core: a term type, folds, a precedence-climbing parser, a
renderer and a compile-time term. Its floor is C++11. Chunk 27 fixed what
stopped each part compiling once used, and
`tests/djinterp/cpp/modules/math/t_parse_expression_face.cpp` runs all of
it, over a small language of its own;
`tests/djinterp/cpp/modules/math/expr/t_expr_face.cpp` runs `math/expr/`,
and `t_expr_complex_face.cpp` beside it its complex
numbers, and `t_expr_complex_function_face.cpp` their functions, with the
references of `math_tests_expr_complex_refs.h`, which
`_handoffs/math98_chunk41_2026.11.01/tools/refs_c41.py` makes from mpmath,
`t_expr_complex_five_face.cpp` the five of chunk 42, and
`t_expr_complex_matrix_face.cpp` matrices of complex numbers, with
`math_tests_expr_cmatrix_refs.h`, which
`_handoffs/math98_chunk43_2026.11.02/tools/refs_c43.py` makes.

---

## 4. Levels

Each face compiles from its floor and adds features as the language allows:

| Level | What the faces gain |
|---|---|
| ISO strict C++98 | everything in the first six rows of section 1; no `long long`, and on a 32-bit target no 64-bit integer at all |
| C++11 | `constexpr` constants; conversions to and from `std::array`; `math::value` moves; `common_number_t`; the run-time expression language, `math/expr/`; the compile-time core, `expression.hpp`, and on it functions, sampling and the bridge to the functional subframework |
| C++14 | the `_c` kernels are constant expressions; variable templates (`constants::pi<T>`); `math::inline_value`'s operations are constant expressions, and vectors and matrices of inline values; the compile-time core's transcendentals and forms over `std::tuple` are constant expressions, and `sample_over` |
| C++20 | `std::is_constant_evaluated` lets a face use `_c` in constant evaluation and the library at run time; `math::value` is a constant expression for the inline kinds, with `<=>` and the concept `number` |

ISO strict means `-std=c++98 -pedantic-errors -DD_CFG_ENV_ISO_STRICT=1
-DRE_STD_CFG_ISO_STRICT=1`. With `-m32` that build has no 64-bit integer, which
is the hardest case the C core supports.

---

## 5. Building and checking

Everything was verified on Ubuntu 24.04 with GCC 13.3 and Clang 18.1, plus
the 32-bit libraries. From the repository root:

```bash
bash scripts/ci/check_math_tests.sh               # the tests, smoke tier
bash scripts/ci/check_math_tests.sh full          # the tests, full matrix
bash scripts/ci/check_math_tests.sh full t_decomp # one test, every build
bash scripts/ci/check_math_tests.sh fused         # as compilers that fuse
python3 scripts/gen_c_math_float.py --check       # generated headers in step
python3 scripts/ci/ladder.py --known scripts/ci/ladder_known.txt \
    inc/djinterp/c/modules/math inc/djinterp/cpp/modules/math
                                                  # the ladders, the module
python3 scripts/ci/check_style.py --known scripts/ci/style_known.txt \
    inc/djinterp/c/modules/math inc/djinterp/cpp/modules/math
                                                  # the guides' rules
```

`scripts/ci/check_math_tests.sh` finds every C harness
(`tests/djinterp/c/modules/math/t_*.c`) and every face test
(`tests/djinterp/cpp/modules/math/**/t_*_face.cpp`) on each run, then builds
and runs them in one of three tiers. A build passes when it compiles with
warnings as errors, exits 0, and reports 0 failures.

- `JOBS` sets how many builds run at once; the default is one per core.
- `--list` prints the commands without running them.
- `--save <file>` keeps every build's result line.
- Naming tests (`t_decomp`, `t_eigen_face`) runs only those.

**The smoke tier** is the default. It builds every harness once (gcc, C99) and
every face test at three rungs (g++ at ISO strict C++98, C++14 and C++20): 145
builds after chunk 51, 167 after chunk 64. It needs only gcc and g++. On a
2.1 GHz Xeon it took 8 minutes with one job and 4 with two; chunk 40's, on
two cores of it, 7, chunk 41's, 6, chunks 42's and 43's, 7, chunk 44's, 9,
and chunk 45's, 7.

**The full tier** is the matrix. Each C harness is built 12 ways:

- gcc and clang at C99 and C11;
- g++ and clang++ at ISO strict C++98;
- gcc and clang `-m32` on SSE;
- gcc `-m32` on the x87;
- g++ and clang++ ISO strict C++98 `-m32` on SSE;
- g++ ISO strict C++98 `-m32` on the x87.

A harness with a C++14 section, where it checks constant evaluation
(twenty after chunk 64), is also built at C++14 and C++20 by g++ and
clang++. Each face test is built 64 ways: 2 compilers, 64-bit and `-m32`,
8 rungs, plain and testing. That made 384 harness builds and 2,368 face
builds at chunk 26, and makes 428 and 2,944 after chunk 64. On two
cores of a 2.8 GHz Xeon, chunk 26's run took 107 minutes: 85 for the
harnesses, a third of it `t_power`'s, whose 16 builds compile slowly, and 22
for the face tests; chunk 27's face test adds a minute, and chunk 28's, as
chunks 29 and 30 grew it, two minutes. On two cores again, chunk 37's ran in
parts: the 360 harness builds in some 70 minutes, `t_power`'s and
`t_elementary`'s the slowest, and the 1,984 face builds in 20; chunk 38's,
the 380 harness builds in 79 minutes and the face builds in 21; chunk 39's
2,048 face builds, `t_complex_face.cpp`'s among them, in 31; chunk 40's
2,112, `t_expr_complex_face.cpp`'s among them, in 32, and the 64 builds of
the four harnesses that include `value.h` in 6; chunk 41's, over the owner's
restructured tree, the 380 harness builds in 74 minutes and the 2,176 face
builds, `t_expr_complex_function_face.cpp`'s among them, in 33; chunk 42's,
the 2,240 face builds, `t_expr_complex_five_face.cpp`'s among them, in 38,
and the 380 harness builds, `t_cplx.c`'s references grown, in 80; chunk
43's, the 2,304 face builds, `t_expr_complex_matrix_face.cpp`'s among them,
in 40; chunk 44's, the 2,304 face builds, `t_quaternion_face.cpp` grown,
in 47, and the 384 harness builds, `t_quat.c`'s 16 among them, in 97;
chunk 45's, the 2,368 face builds, `t_expr_quaternion_face.cpp`'s among
them, in 39, and the 64 builds of the four harnesses that include `cplx.h`
in 21.
The full tier needs clang and the 32-bit libraries; if either is missing,
it stops before building anything.

**The fused tier** (chunk 51, S414) builds every test as compilers that
fuse a multiply and an add build it: each harness by gcc and clang in GNU
C11, a harness with a C++14 section by g++ and clang++ in GNU C++14 and
C++20 too, and each face test by g++ and clang++ in GNU C++17, plain and
testing, all with the host's fused multiply-add (`-march=x86-64-v3`): 288
builds. Where a build fuses, the harnesses leave out the checks that a
function's bits are another computation's, and say so (S421).

**The double family on 32-bit x86.** Its correctly rounded functions, `dd.h`
and `d_math_self_test` need double evaluated as double: build with `-msse2
-mfpmath=sse`. Without them a translation unit that calls one does not
compile (S418); the full tier's x87 builds leave them out with a note. A
program that needs the same bits everywhere calls `d_math_self_test()` once
at start-up, in each translation unit whose build may differ, and stops
where it is not 0 (S419).

### On Windows

- **WSL 2 with Ubuntu 24.04** runs all of the above unchanged. This is the
  recommended route because it is the environment everything was measured in:

  ```bash
  sudo apt-get update
  sudo apt-get install -y build-essential clang python3 git \
      gcc-multilib g++-multilib lib32stdc++-13-dev libc6-dev-i386 libssl-dev
  ```

  Clone into the Linux file system (`~/`), not `/mnt/c`. File access across
  the boundary is many times slower. Then run
  `scripts/ci/check_math_tests.sh` from the clone's root.
- **MSYS2** (MinGW-w64 GCC or Clang, with bash) should run the 64-bit builds.
  The `-m32` builds need the 32-bit MinGW environment. Not yet tried.
- **MSVC** has never compiled the math core. On MSVC `long double` is
  `double`, and the signed-zero test that GCC and Clang make with
  `__builtin_copysign` falls back to `1 / x`, which is not a constant
  expression. A CMake and CTest runner that includes MSVC is roadmap item C4c.

---

## 6. How the work is done

The math lane works in small chunks, one coherent change each. Every chunk
consists of:

1. **The change, as plain files.** It must build and pass on its own. From
   chunk 25 the lane commits nothing: the owner tests the files on his own
   machine and commits them himself (his ruling of 2026.10.18). Through
   chunk 24 each change was a commit on the lane's branch.
2. **A record**, `_handoffs/math98_chunkN_<date>/`. It holds a `README.md`
   (what changed, what was measured, decisions with their alternatives,
   what was noticed and not changed, how to reproduce), plus `tools/` and
   `raw/`. Every number in a record comes from a tool in `tools/` whose
   output is in `raw/`.
3. **A short note** to the owner.
4. **The changed files**, as a zip with a manifest, laid over earlier
   trees and tested there. From chunk 28 it carries every file the lane has
   made or changed since the owner's tree of 2026.10.07, and the records
   from chunk 16, so that it applies over the owner's tree of 2026.10.23 as
   over any lane tree from chunk 15. From chunk 37, at his word of
   2026.10.28 ("One ZIP"), it carries every file of the lane's tree his tree
   has not or holds otherwise - every record, and the repository's own
   directories - so that it alone, over his tree, gives the lane's. From
   chunk 16 to 27 it was cumulative since chunk 15; through chunk 15, a git
   bundle of the new commits.

The owner reviews each delivery. His next message after a note accepts that
note's decisions; he may ask for several steps at once, as for the rest of
Phase 1.

---

## 7. Decisions

The lane's own decisions are numbered S1 onward, each recorded with its
alternative in the record that took it. The owner's rulings that shape this
module:

| When | Ruling |
|---|---|
| 2026.10.04 | C core as the foundation; C++ as a wrapper adding features per level; a generic C++ path for other element types |
| 2026.10.04 | floating-point intervals contain the representable values within their bounds |
| 2026.10.05 | geometry and coordinate from C++98; points become `linalg::vector`, collections `re_std::array` |
| 2026.10.09 | fix `eigenvalues_qr` (M1) and `general_eigen` (M2); `numeric_compare` to C++98 (M3) |
| 2026.10.13 | expression, function and calculus to a C++11 floor for now, with a runtime expression for C++98 later if needed. Lower levels get every feature they can express; higher levels get every optimization they allow; code is shared between levels (N1) |
| 2026.10.13 | `_fast` variants only where measured demand exists, each with a documented ulp bound; eager values by default and laziness as an opt-in from C++11; chaining wherever it is appropriate; a generic value mechanism before further features; benchmarks and bignum near the end of the roadmap |
| 2026.10.13 | the root's `D_STATIC_ASSERT` fallback gets `D_MAYBE_UNUSED` (C2); the lane fixes the test headers' floor comments (C3); the 09.29 lane is mined only for what is missing (C6) |
| 2026.10.18 | no commits from the lane: everything in plain files, delivered as a zip of the modified files, which the owner tests on his machine and commits himself; work through the rest of Phase 1 |
| 2026.11.04 | `math::complex<T>`'s functions by their own names `cplx.h`'s plain forms at run time, the `_c` forms in constant evaluation and by the `_c` names; the language's values the `_c` forms (S336); every evaluator given the fold's context, each value evaluator holding its answer real where it asks (S340) |
| 2026.11.12 | before the itinerary goes on, the four groups of fixes of the lane's evaluation: speed, consolidation, `math::expr::` and accuracy |
| 2026.11.14 | the language flat in `djinterp::math`, no namespace below it; a clean break for the old names; the prefix left to the lane's recommendation, fewer or no levels below `math` (S397) |
| 2026.11.16 | the double family rewritten on pairs of doubles: one result, the correctly rounded one, on every platform, under eight rules, by the lane's own algorithms proven with Gappa and Sollya; exp and log first, then 0.8's items, core v2 in December, then the rest; the x87 refused for the family; a public pair type; kernels immune to fusing (rulings 1 to 7, S414 to S419); before 0.8, the language's solves refined, `rotquat` the nearest rotation, `expression.hpp`'s variables in a namespace (rulings 8 to 10) |
| 2026.11.21 | math in no modules of its own: its largest files split instead, each to about 4,000 lines at most, the old names kept as headers that include their parts; the deeper modularization not now (S436 to S439) |
| 2026.11.21 | chunk 53's decisions stand ("continue"): `log_c`'s speed next |
| 2026.11.20 | chunk 51's decisions stand ("continue for now"), and chunk 52's ("continue"): step 29 (5a) with 0.8's items next |
| 2026.11.14 | accuracy before 0.8 for the float and double families: compensated sums and a refined solve, then their `_c` forms correctly rounded, provably, at today's speed; the long double family and the plain forms at step 36 (S404 to S408) |
| 2026.11.23 | chunks 55 to 58's decisions stand ("continue"): step 32 (5d), `reify`, next |
| 2026.11.23 | chunk 59's decisions stand ("continue"): step 34 (5f), the lazy type, next |
| 2026.11.24 | chunk 60's decisions stand ("continue"): step 35 (5g), the style debt, next |
| 2026.11.24 | S473 ruled: the banners' HOW TO USE, OPTIONS and CONFIGURATION written today; chunk 61's decisions stand ("continue") |
| 2026.11.24 | rulings 12, 13, 14's 9b and 15, each as recommended: each plain double function correctly rounded once within about twice the C library's time; `min` and `max` as IEEE 754-2019's, with `min_num` and `max_num`; random numbers math's, the seeds and keys simterp's; the elementary block first, then root finding, banded solves and table lookups, the number types, random numbers, the rest; chunk 62's S474, S475 and S477 stand |
| 2026.11.24 | ruling 14's 9a and 9c: the ODE steppers math's, `st_math` the terps'; S476 B, chunk 62's objects compared as well; the rulings' work now, exp's switch first, then the scalar helpers |
| 2026.11.24 | chunk 63's decisions stand, S482 B: the terps' needs numbered now, steps 52 on; on S478, a fast and a slow exp unless `exp_c` can match glibc |
| 2026.11.25 | chunk 64's decisions stand ("continue"): step 53, exp's speed, next |
| 2026.11.25 | chunk 65's decisions ruled, each as recommended (08:38): step 54, log's speed, next |
| 2026.11.25 | chunk 66's decisions ruled, each as recommended (14:42); decision 71: a fast log next, in chunk 67, before step 55 |

---

## 8. Branches and other lanes

`master` is the main branch. As of chunk 12 it points at the math lane's tip,
which contains the owner's tree of 2026.10.07 (`ef7c81f`) and every lane
merged since. The math lane committed on its own branch, `consolidated`,
through chunk 24; from chunk 25 it commits nothing, and the owner commits
its files. Other lanes that started from the same tree (net, and any
others) merge into `master` when they finish, and so does this one.

From chunk 28 the lane works on the owner's tree of 2026.10.23, and from
chunk 37 on his tree of 2026.10.27, with the lane's files laid over it. That
tree had none of the lane's files: it had moved on from 2026.10.07 elsewhere
(`fs` renamed `filesys`, net, re_std, the root's `D_STATIC_ASSERT`). Of the
files the lane changed, two the owner had changed too, each with the lane's
change in a form of his own, and his versions stand: `c/djinterp.h` (chunk
15's C2) and re_std's `reverse_iterator.hpp`.

From chunk 41 the lane is built and checked over the owner's restructured
tree of 2026.11.01, the restructure's run without the math lane
(`restructure_2026_10_29`): every other file moved to the `c/` and `cpp/`
layout and its C and C++ types renamed. The lane keeps its old paths and
names, as the owner runs the restructuring script over it on his machine;
the build has, at the old paths, one-line shims to the headers the
restructure moved, and the old names of the two renames the lane meets,
`parse_status` and `kv_pair`, none of which the delivery holds
(`_handoffs/math98_chunk41_2026.11.01/tools/shims_c41.py`). The
restructured tree holds older copies, of 2026.09.21 to 2026.10.02, of 55
files the lane changed, at the paths the restructure gave them; no test of
the lane's reaches one, and the lane's copies are the ones the script
should move into those paths (chunk 41's record, `raw/base_tree.txt`).

From chunk 45b (2026.11.07) the lane works in the owner's restructured tree
itself, as lane A ruled for chunk 44 on: chunks 44 and 45 laid into his tree
of 2026.11.07, every file of the lane at its restructured path, its banners'
paths and tags and its generator's paths as the tree has them, every C type
renamed `dj_`, and no shims (chunk 45b's record).

From chunk 51 (2026.11.16) the lane works over the owner's tree of
2026.11.16, which holds chunks 44 to 50 as he committed them, and delivers a
zip over it of the files the chunk changed or made, with its record (S423);
chunk 52's holds chunk 51's files with its own, so that it is right over
that tree whether or not chunk 51's zip is in it (S427), chunk 53's
chunks 51's and 52's with its own (S431), and chunk 54's chunks 51's to
53's with its own (S434). From chunk 55 (2026.11.21) the zip goes over the
owner's main as his push names it, with the chunks before it that have not
landed laid over it, and carries no record: the records stay with the
lane's own deliveries (S440); chunk 56's goes over his main of 2026.11.21
with chunks 52's and 55's zips laid over it, and holds chunk 56's files
alone (S446). Both were cut again on his main of 2026.11.23, Friday's over
Wednesday's, their files at the tree's root as his merge window reads a
zip (S447, S449); chunk 58's goes over that main with both laid over it, in
order (S455), chunk 59's over that with chunk 58's laid too (S462), and
chunk 60's after chunk 59's (S468), chunk 61's after chunk 60's
(S473), chunk 62's after chunk 61's (S477), chunk 63's after chunk 62's
(S482), chunk 64's after chunk 63's (S488), chunk 65's after chunk 64's
(S493), chunk 66's after chunk 65's (S499), and chunk 67's after chunk 66's
(S504).
From 2026.11.18 the lane's notes and deliveries go out once a day.

Outside `cpp/modules/math/`, `c/modules/math/` and their tests, this lane
changed:

- `re_std`'s `array` (5 files), and `reverse_iterator.hpp` (to C++98; the
  owner's tree of 2026.10.23 has the same port, and its version stands);
- the root's `D_STATIC_ASSERT` fallback (chunk 15, C2; the owner's tree of
  2026.10.23 has his own form of it, which stands);
- the ladders' known lists and the stdint lint's, then in `ci/` (lane A's
  `scripts/ci/` holds their successors), and
  `scripts/ci/check_math_tests.sh` (new);
- `src/djinterp/cpp/modules/math/numeric_compare.cpp`, now a translation
  unit that only includes its header, as its neighbours there do;
- `scripts/gen_c_math_float.py` (new), and its modules,
  `scripts/gen_c_math/` (new, S439);
- `config/c/modules/math/cfg_value_wide.h` (new), the wide integer's
  allocation hooks;
- `cpp/modules/parse/expression/`'s four headers `expression_ops.hpp`,
  `expression_parse.hpp`, `expression_render.hpp` and `expression_static.hpp`
  (chunk 27, P0): their defects fixed, nothing else changed;
- seven test headers' comments.

It also carries the register lane's retirement of four `env/db` configs
(#93b), which `master` still had at `ef7c81f`; the owner's tree of
2026.10.23 still has them, and a zip of files cannot delete them. These are
the files a merge from another lane could touch.
