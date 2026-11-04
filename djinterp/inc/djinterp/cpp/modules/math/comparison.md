# djinterp math: the framework as proposed, beside similar libraries

A comparison of features, made in chunk 34 (step 19 of 48) at the owner's
asking, and brought up to date by each chunk since (chunk 35, step 20, its
matrix arithmetic; chunk 36, step 21, linear algebra's functions and the
owner's rulings on complex numbers and quaternions; chunk 37, step 22's
first part, the C core's complex numbers; chunk 38, its second part, the
linear algebra they complete; chunk 39, its third, the C++ face of complex
numbers; chunk 40, step 23's first part, complex numbers in the language;
chunk 41, the first chunk of its second part, their functions; chunk 42,
its second chunk, the five complex functions the C core lacked; chunk 43,
its third part, matrices of complex numbers):
what djinterp's math
framework does or is planned to do, by the roadmap's 51 steps (48 until
chunk 36's rulings added 3d to 3f, every step from 22 on three further),
beside the libraries a user would weigh it
against. It compares features alone. How fast each is, and how accurate its
functions are in ulps, are Phase 7's measurements (steps 41 and 42); nothing
here is measured.

In the djinterp column, "done" means in the tree as of chunk 43, with the
step that did it; "step N" means the roadmap plans it at that step; "later"
means the roadmap lists it under "Later, not yet decided"; "none" means no
step plans it. In the other columns, "yes" is a documented feature, "part"
a narrower one, and "no" one the library does not have. Where a cell is not
certain from the library's own documentation it says so.

---

## 1. The libraries

| Library | Language | Kind | Licence |
|---|---|---|---|
| **djinterp math** | C99 core; C++98 to C++23 faces; expressions from C++11 | numeric core, exact value tower, expression language, linear algebra | the framework's |
| SymPy | Python | computer algebra system | BSD |
| SymEngine | C++, with a C wrapper; Python, Julia and R bindings | symbolic core, SymPy's fast engine | MIT |
| GiNaC (with CLN) | C++ | symbolic computation embedded in C++ | GPL |
| math.js | JavaScript | expression parser, numbers, matrices, some algebra | Apache 2.0 |
| ExprTk | C++, header-only | expression evaluator and small language | MIT |
| muParser, TinyExpr | C++; C | fast numeric expression evaluators | BSD; zlib |
| Eigen | C++, header-only | dense and sparse linear algebra | MPL 2.0 |
| Boost.Math | C++, mostly header-only | special functions, distributions, quadrature, differentiation | Boost |
| GMP, MPFR, FLINT (Arb within it) | C | big integers and rationals; correctly rounded big floats; number theory, exact matrices, ball arithmetic | LGPL |
| CORE-MATH, LLVM libc | C | correctly rounded elementary functions | MIT; Apache 2.0 with LLVM exception |
| gcem; C++23 and C++26 `constexpr` `<cmath>` | C++ | elementary functions in constant expressions | Apache 2.0; the standard |

---

## 2. Numbers

| Feature | djinterp | SymPy | SymEngine | GiNaC | math.js | ExprTk |
|---|---|---|---|---|---|---|
| `float`, `double`, `long double` | done | `float` (Python's double) | double | double via CLN | double | float, double, long double |
| Exact rationals | done, over the widest integer (step 6) | yes, unbounded | yes, unbounded (GMP or Boost) | yes, unbounded (CLN) | yes (`fraction`) | no (a custom type can be) |
| One value of any kind, exact kept exact, inexact where a float enters (the tower) | done (steps 7 to 11) | yes | yes | yes | part: literals of one configured type; mixed types convert | no |
| Decimal literals read exactly, `0.1` the rational 1/10, each type rounding once | done (step 8) | no: `0.1` parses as a Float unless asked otherwise | no | no | only with `number: 'Fraction'` configured | no |
| Comparison exact across kinds | done | yes | unclear | unclear | within one type | no |
| Fixed-width wide integers, 256 bits | done (step 43) | n/a, unbounded | n/a, unbounded | n/a, unbounded | n/a | no |
| Unbounded integers | step 44 | yes | yes | yes | yes (`bigint`) | no |
| Unbounded rationals | step 45 | yes | yes | yes | unclear | no |
| Big floats, correctly rounded | step 47 (the MPFR model) | part (mpmath's precision; correct rounding not promised) | yes (MPFR, optional) | part (CLN's floats) | part (`bignumber`, decimal) | yes, MPFR as its type |
| Ball arithmetic | step 48 | no (mpmath has intervals) | part (Arb, optional) | no | no | no |
| Complex numbers | the C core's done (step 22, `cplx.h`, S280 to S288): float, double and long double, C99's 22 functions; their C++ face, `math::complex<T>`, from C++98, the C core's bits at every level (step 22, chunk 39, S313); a kind of the value, exact Gaussian rationals and each floating-point type, its arithmetic `cplx.h`'s or exact (step 23, chunk 40, S327, S328); each function's principal complex value, exact where it is a Gaussian rational (chunk 41, S336, S337), and `exp2`, `log2`, `log10`, `log1p` and `expm1`'s, in the C core, the face and the language (chunk 42, S342 to S347); matrices of them, dense or exact (chunk 43, S348) | yes | yes | yes | yes | no |
| Quaternions | the C core's and the C++ faces done (`quat.h`, `quaternion.hpp`); a kind of the value at step 24 (S275 to S277) | yes (`Quaternion`) | no | part (Clifford algebras) | no | no |
| Units of measure | none | yes (`sympy.physics.units`) | no | no | yes | no |
| Errors as values that pass through, the first kept; Kleene's logic | done (steps 14 to 17) | no: exceptions, or unevaluated forms | no: exceptions | no: exceptions | no: exceptions | part: NaN |
| A new number kind without changing the core (boxed kinds) | done (step 9) | yes (Python classes) | part (C++ subclasses) | part | yes (typed-function types) | yes (the numeric type is a template parameter) |

Against GMP, MPFR and FLINT, the bignum phase (steps 44 to 49) plans the
same kinds, with multiplication up to Toom-3 (step 46); GMP and FLINT
multiply large numbers by FFT, which no step plans, and their kernels are
tuned in assembly. MPFR's correct rounding at every precision and its
breadth of functions are the model of steps 47 and 48, and its documented,
proven behaviour is the oracle of step 41.

---

## 3. Elementary functions and one answer

| Feature | djinterp | CORE-MATH, LLVM libc | glibc | MSVC | gcem | C++23 / C++26 `<cmath>` | Boost.Math |
|---|---|---|---|---|---|---|---|
| Correctly rounded sqrt, cbrt, hypot, fmod, remainder, rounding | done | yes | sqrt, fmod, remainder and rounding, which IEEE 754 makes exact; cbrt and hypot not promised | as glibc | no | as glibc, where IEEE 754 holds | no |
| Correctly rounded exp, log, sin, cos, tan, atan, pow, hyperbolic | step 36 | yes, binary32 and binary64 | being taken from CORE-MATH function by function since 2025, binary32 first (`cosf`, `sincosf`: patches of 2026-03) | `/Zc:cmath` takes LLVM libc's, experimental in the 14.52 tools | no | not required | no |
| The same bits at every language level and in constant evaluation as at run time | done for the correctly rounded functions; all of them from step 36 (G6) | the same bits on every platform at run time; constant evaluation through the compilers adopting it | no promise | from `/Zc:cmath` | constant evaluation only, its own approximations | no promise: accuracy is the implementation's | no |
| Constant expressions | from C++14, the `_c` forms (done) | through clang and MSVC adopting LLVM libc | no | C++23 with `/Zc:cmath` | from C++11 | C++23 (P0533) and C++26 (P1383), a subset | part (`ccmath`) |
| Works in C99 and in strict C++98 | done | C | C | C and C++ | C++11 | C++23 and C++26 | C++14 |
| Domain errors as the C library gives them | done | yes | yes | yes | part | yes | by policy: exceptions, errno or values |
| Special functions: gamma, erf, Bessel, zeta, elliptic, Lambert W | none (the C core has no `tgamma`, `lgamma`, `erf`, `erfc`) | part (some of C's set, binary32 first) | C's set | C's set | part (gamma, erf, beta) | C's set | yes, the widest of any here |
| Inverse hyperbolic functions, `log1p`, `expm1` | done (step 22): every family, `_c` forms constant expressions from C++14, long double within 0.53 of an ulp (S286) | yes | yes | yes | part | yes | yes |
| C99's complex functions, with Annex G's cuts and special values | done (step 22): the four exact ones once, and each other in a `_c` form, a constant expression from C++14, and a plain one, its bound stated in steps and checked against mpmath; Annex G as C17 corrects it (S280 to S288); in C++, `math::complex<T>`'s functions the `_c` forms, constant expressions from C++14 (chunk 39, S313) | CORE-MATH: none listed; LLVM libc: part, being added (`carg` in 2026-06) | yes, `<complex.h>`; `cpow` as `cexp(w clog(z))` | `<complex.h>`'s functions on `_Dcomplex`, no `_Complex` | no | `<complex>`'s `std::complex`: the functions; its arithmetic `constexpr` from C++20, its functions not | part (the inverse trigonometric and hyperbolic functions) |
| `exp2`, `log2`, `log10`, `log1p` and `expm1` of a complex number | done (step 23, chunk 42): every family, each form one long double kernel in pairs rounded once, each part within a step of the correctly rounded one, checked against mpmath; the real function's bits on the real axis; Annex G's values of the functions they are forms of (S342 to S346); in C++, `math::complex<T>`'s | none | `clog10` alone, a GNU extension | `clog10`, `clog10f` and `clog10l` in the UCRT | no | `std::log10` of a `std::complex` alone | no |
| Statistical distributions | none (`statistics/` is empty) | no | no | no | no | no | yes |
| `_fast` variants of documented ulp bounds | step 40, on measured demand | no | no | no | no | no | policies trade precision |
| A standard proposal of the same aim | P3864R1 (2026-02): `constexpr` `cr_add`, `cr_sub`, `cr_mul`, `cr_div`, `cr_sqrt`, correctly rounded whatever the rounding mode; djinterp's `_c` `sqrt` is correctly rounded and `constexpr` from C++14, in the default rounding mode, which is the only one in constant evaluation | | | | | | |

---

## 4. The expression language

| Feature | djinterp | SymPy | SymEngine | GiNaC | math.js | ExprTk | muParser, TinyExpr |
|---|---|---|---|---|---|---|---|
| Parse text to a term, render a term to text that reads back as itself | done (steps 13, 14) | yes | yes | yes | yes | parse only | parse only |
| `+ - * / mod ^ !`, precedence, unary minus, `\|x\|` (an option) | done (step 14) | yes | yes | yes | yes | yes | yes |
| Implied multiplication, `2x` | done, an option (G5) | an option (transformations) | unclear | unclear | yes, by default | yes | no |
| Calls of the standard functions | done, 25 and `abs`, `factorial` (step 15), the complex numbers' five (chunk 40), `log1p`, `expm1`, `asinh`, `acosh`, `atanh`, `sign`, `min` and `max` (chunk 41, S338), `transpose` (chunk 43, S350): 52 with piecewise's three and linear algebra's nine | hundreds | many | many | hundreds | a few dozen | about 20 |
| User functions and constants in the language | none: the context is empty | yes | yes | yes | yes | yes | yes |
| Relations, logic, piecewise | done (step 16) | yes | yes | part (relations) | yes | yes | part |
| Complex numbers in the language, `3 + 4im` | done (step 23, chunks 40 to 43): `im`, `complex`, `real`, `imag`, `conj`, `arg`, the modulus, `z'`, `=` and `!=`, exact powers, exact Gaussian rationals or dense parts (S327 to S333); each function's principal value, exact roots and rational powers where Gaussian, a real function's complex value outside its domain unless the context asks real results, and folds in `math::complex<T>` (chunk 41, S336 to S341); `exp2`, `log2`, `log10`, `log1p` and `expm1` of a complex value, the C core's (chunk 42, S347); matrices of them (chunk 43, S348 to S353) | yes, `I` | yes, `I` | yes, `I` | yes, `i` | unclear | no (muParserX has them) |
| Vector and matrix literals, `[1, 2; 3, 4]` | done (step 19); blocks `[A, b; C, d]` and the empty `[]` (step 21) | no (constructors; `BlockMatrix`) | no | no (constructors) | yes, the same syntax, blocks by `concat` | no (vectors declared) | no (muParserX has them) |
| Matrix arithmetic, transpose, indexing in the language | done (step 20): sums, scalar and matrix products, `A'`, `A ^ n`, `v[i]`, `A[i, j]`, from 1; `A ^ (-n)` (step 21); of complex matrices, `A'` the conjugate transpose and `transpose(A)` the transpose (step 23, chunk 43, S349, S350) | yes | yes | yes | yes | vectors only | no |
| Matrices of complex numbers in the language, `[1, 2im; 3, 4]` | done (step 23, chunk 43): dense as `math::complex<T>` or exact, each element kept as given (S348); `real`, `imag`, `conj` and `arg` of a matrix element by element, `complex(A, B)` (S350) | yes | yes | yes | yes | no | no |
| Element-wise operators and a scalar broadcast, `A .* B`, `A + 1` | none: `A + 1` is a domain error (S246) | part (`multiply_elementwise`) | unclear | unclear | yes, both | yes, on vectors | no |
| Slices and an index from the end, `A[1:2, :]`, `end` | none | yes | unclear | part (`sub_matrix`) | yes | part (vector ranges) | no |
| `det`, `inv`, `solve`, `trace`, `eig`, `dot`, `cross`, `norm` | done (step 21): exact for exact matrices, rational eigenvalues exact; of complex matrices, `dot` conjugating, `norm` of the moduli, and every eigenvalue, complex where it is, exact where it is a Gaussian rational (step 23, chunk 43, S351, S352) | yes | part (det, inv, LU) | part (det, inverse, linear solve) | yes, and `expm`, `sqrtm`, `pinv`, `schur` | dot only | no |
| Evaluation in a chosen type, and as an exact value | done | yes (`evalf`, exact by default) | yes | yes | one configured type | the template type | double |
| Evaluation at any precision | step 49 | yes | yes (MPFR) | yes | yes (`bignumber`) | yes (MPFR type) | no |
| A handle that chains each step, failure through `result` | done (step 17) | methods, exceptions | methods, exceptions | methods, exceptions | methods, exceptions | no | no |
| Loops, assignment, strings, `return` | none | Python itself | no | C++ itself | part (assignment, functions) | yes | no |
| Compile once, evaluate fast many times | part: a term folds each time | `lambdify` to NumPy | `lambdify`, LLVM back end | part | `compile()` | yes, its design | yes, its design |
| Compile-time expressions, and `reify` from compile time to run time | steps 29 to 32 | no | no | no | no | no | no |
| LaTeX out and in | later | yes, both | out | out | out | no | no |
| Code generation, C or Fortran | none | yes | part (LLVM) | out as C | no | no | no |

---

## 5. Symbolic computation

| Feature | djinterp | SymPy | SymEngine | GiNaC | math.js |
|---|---|---|---|---|---|
| Substitution | done (step 18) | yes | yes | yes | yes (`resolve`, scope) |
| Simplification: constant folding, like terms, identities | done, the value kept wherever each part is finite (step 18, S228) | yes, many strategies | part | part (`normal`, `collect`) | yes, rule-based, with a context of assumptions such as commutativity |
| Non-commutative factors kept in order | done for matrix literals and values (step 20, S251); a variable is taken for a number, no matrix variable declared | yes, with matrix symbols | unclear | yes | yes, by context |
| Expand, factor, polynomial gcd, a canonical form | none | yes | part (expand) | yes | part (`rationalize`) |
| The symbolic derivative, partial derivatives, gradient | step 25 | yes | yes | yes | yes (`derivative`) |
| Series: Taylor | step 27 | yes, Laurent and Puiseux too | yes | yes | no |
| Limits | step 27, numeric first | yes, symbolic (Gruntz) | no | no | no |
| Sums and products | step 27 | yes, closed forms | no | no | no |
| Antiderivatives | step 28, a table of elementary forms | yes (Risch in part, heuristics) | no | no | no |
| Numeric integrals and derivatives of a term | step 26 (`calculus/` has the quadrature) | yes (mpmath) | no | integrals (`integral`) | no |
| Symbolic equation solving | none beyond `solve` of a linear system (done, step 21) | yes | part (polynomials) | linear systems | no (numeric `lsolve`, `polynomialRoot`) |
| Differential equations | none | yes | no | no | no |
| Assumptions (x real, positive, finite) | none | yes | unclear | part (`realsymbol`, `possymbol`) | part (the simplify context) |

---

## 6. Linear algebra

| Feature | djinterp | Eigen | math.js | SymPy | FLINT |
|---|---|---|---|---|---|
| Fixed-size vectors and matrices, constant expressions | done; kernels `constexpr` from C++14 | fixed-size, not `constexpr` | no | no | no |
| Dynamic matrices | done | yes | yes | yes | yes |
| Any number type, exact `math::value` among them | done (step 11) | any scalar type, by traits | its number types | any SymPy expression | integers, rationals, finite fields, balls |
| A matrix of mixed exact and floating elements, each kept as given | done (step 19, S239) | no: one scalar type | unclear | yes | no |
| LU, QR, Cholesky, SVD, pseudoinverse | done | yes, several of each | LU, QR, pinv | yes | LU, exact |
| Symmetric eigensolver; general eigenvalues | done; complex eigenvalues and eigenvectors by LAPACK's path, balancing, Francis's QR and the Schur vectors, in the C core (step 22, chunk 38, S292) and the C++ face, as `math::complex<T>` (chunk 39, S317); in the language every one, complex where it is, ordered by real part, then imaginary part (step 23, chunk 43, S352) | yes, complex eigenvalues too | yes | exact, with complex roots | exact charpoly, ball roots |
| Complex matrices: LU, inverse, solution, Hermitian and general eigensolvers | done in the C core (step 22, chunk 38, S297 to S299); in the C++ face, vectors and matrices of `math::complex<T>`, their determinant, inverse and solution the C core's bits (chunk 39, S315); in the language, the C core's where dense and exact where exact, a Hermitian matrix's eigenvalues real (step 23, chunk 43, S351, S352) | yes | yes | yes | balls (`acb_mat`) |
| The conjugating inner product, the adjoint, a unitary test | done: the language's `dot` and `A'` conjugate (chunk 43, S273, S351); the C++ face's `vdot`, complex norms and `is_unitary`, its `dot` bilinear (S322, S354) | yes: `dot` conjugates its first, `adjoint()`, `isUnitary()` | part: `ctranspose`; whether `dot` conjugates unclear | part: `H`, `dot` with `hermitian=True` | part: `acb_mat_conjugate_transpose` |
| Runtime-sized complex matrices in C++ | none: `dynamic_matrix` holds arithmetic elements alone (S354, flagged) | yes, `MatrixXcd` | not C++ | not C++ | not C++ (`acb_mat`, in C) |
| Norms that neither overflow nor underflow at the range's ends | done in the C core and every face, every bit in range kept (chunk 38, S301) | `stableNorm()` and `blueNorm()` beside the plain `norm()` | no | exact | balls |
| A default tolerance relative to the operands' scale | done: four epsilons of the type, 0 for exact values (chunk 38, S303, S304) | yes, `isApprox`, `dummy_precision()` 1e-12 for double | an absolute and a relative epsilon, configured | exact comparison | balls |
| Matrices over a type whose products do not commute (quaternions) | done: products kept in their order, `solve` and `inverse` exact over exact quaternions (chunk 38, S305) | no: a scalar must commute | no | part (noncommutative symbols) | no |
| Exact determinant and inverse of rational matrices | done (step 21): Bareiss's elimination and Gauss-Jordan on exact values | no (a custom scalar can) | with fractions | yes | yes, fast |
| Exact rational eigenvalues of a rational matrix | done (step 21): found in long double, made exact as roots of the exact characteristic polynomial | no | no | yes, and algebraic ones | yes, exact charpoly and its roots |
| Sparse matrices | none | yes | yes | yes | unclear |
| SIMD vectorization, expression templates | none (eager by default; a lazy type at step 34) | yes | no | no | n/a |
| `mdspan`, a `std::linalg` layer | later | part (`Map`) | no | no | no |

---

## 7. Portability, form and assurance

| Feature | djinterp | SymPy | SymEngine | GiNaC | math.js | ExprTk | Eigen | Boost.Math |
|---|---|---|---|---|---|---|---|---|
| Header-only, no dependency | yes | no | no | no (CLN) | n/a | yes | yes | mostly |
| C callers | the core | no | yes (C wrapper) | no | no | no | no | no |
| Strict C++98 | the core and most faces; expressions from C++11 | n/a | no | no | n/a | not stated | Eigen 3.4 C++03; Eigen 5 C++14 | no (C++14) |
| Thread-safe sharing of values | part: boxed holders are not atomic (S134) | the interpreter's lock | unclear | unclear | n/a | per expression | yes | yes |
| One answer at every level and in constant evaluation | a principle, tested in a 64-build matrix | n/a | no | no | n/a | no | no | no |
| Planted-fault (mutation) checks of every chunk | yes, 53 in chunk 38 | not stated | not stated | not stated | not stated | not stated | not stated | not stated |
| Language bindings | none | Python | Python, Julia, R and others | Python (third-party) | n/a | no | many, third-party | no |

---

## 8. Where djinterp meets, exceeds and falls short

**It exceeds** the others where its principles go further than any of
them:

- **One answer.** The same bits at every language level from C99 and C++98
  to C++23, and in constant evaluation as at run time. LLVM libc and
  CORE-MATH give the same bits across platforms at run time, and clang and
  MSVC are bringing them to constant evaluation; no expression library
  promises it of a whole language, and no C++ constant-evaluation library
  (gcem, the standard's `constexpr` `<cmath>`) promises correct rounding.
- **Exact by default in the language.** `0.1` is 1/10 and `0.1 + 0.2 =
  0.3` holds, where SymPy reads `0.1` as a float unless asked and math.js
  needs a configuration; exactness crosses kinds in comparison and survives
  being put in a matrix (S239).
- **Errors as values.** Every failure is a value that passes through, the
  first kept, with Kleene's logic, where the others raise exceptions or
  give NaN.
- **Reach.** A C core and C++98 faces under the same answers. Of the
  symbolic libraries only SymEngine has C callers, through a wrapper of its
  C++; TinyExpr is C, and evaluates doubles alone; none has a C++98 floor.
- **Compile time to run time.** `reify` (step 32) carries a compile-time
  expression to a run-time term, which none of the others has.

**It meets** them, once the planned steps are done, in: parsing, printing
and evaluating a language of arithmetic, relations, logic and piecewise
(ExprTk, muParser, math.js); MATLAB's matrix literals, blocks, products,
transpose, indexing and its `det`, `inv`, `solve` and `eig` (math.js); the
basic rewrites of math.js's `simplify`;
correctly rounded elementary functions (CORE-MATH, LLVM libc, from step 36);
the dense decompositions most users need (Eigen's main ones); and the kinds
of big numbers GMP and MPFR have (steps 44 to 49), though not their speed at
large sizes.

**It falls short**, with no step planned, in:

1. user functions and constants in the language, which every evaluator
   here has;
2. special functions: C99's own `tgamma`, `lgamma`, `erf` and `erfc` are
   missing from the core (`asinh`, `acosh`, `atanh`, `log1p` and `expm1`
   came with step 22's complex functions), and Boost.Math's breadth
   (Bessel, zeta, elliptic, distributions) is far off;
3. computer algebra past the basics: expand, factor and gcd, symbolic
   solving, symbolic limits, differential equations and assumptions past
   step 25's declared domains (S274), where SymPy and GiNaC have two
   decades of work and SymEngine more than one;
4. element-wise operators, broadcasting and slices, which math.js and
   NumPy-style languages have (S246, S249);
5. sparse matrices and SIMD, which Eigen has;
6. LaTeX, code generation and language bindings;
7. multiplication of very large numbers by FFT, which GMP and FLINT have;
8. maturity: most of the others have a decade or more of users, which no
   plan replaces.

## 9. What the comparison suggests

For the owner's consideration, the best additions by what they give, each
a decision of his to take or leave:

1. **Done in steps 20 and 21:** products of matrices kept in order by
   simplification (S230's review, S251); matrix arithmetic delegated to the
   C core's `mat.h` and `vec.h` and to `linear_algebra`'s generic matrices
   (S237's review, S247); and the C core's `d_mat_power` by squaring, so
   that a dense power has one answer (S248's review, S262). Matrix
   variables declared are ruled for step 25, as declared domains (S274).
2. **User functions and constants** in `expr_context`, a small step with a
   large gain: every evaluator compared has them.
3. **C99's missing functions** in the C core, then in the language: the
   inverse hyperbolic functions, `log1p` and `expm1` are in the C core
   (step 22, chunk 37, S286) and in the language since chunk 41 (S203,
   S338), their complex values with them, `log1p`'s and `expm1`'s since
   chunk 42 (S342, S347);
   the gamma and error functions are still open, each to have its `_c`
   form, so "one answer" covers them too.
4. **Complex numbers as a value kind**, so that `eig`'s eigenvalues are
   whole: ruled by the owner on 2026.10.27 for steps 22 and 23, with
   quaternions in step 24 (S265 to S278); the C core finds every pair since
   chunk 38 (S292), the C++ face gives them as `math::complex<T>` since
   chunk 39 (S317), a complex number is a value of the language since
   chunk 40 (S327), with its functions since chunks 41 and 42 (S336,
   S347), and the language's `eig` gives every eigenvalue, of complex
   matrices too, since step 23's third part, chunk 43 (S352): done.
5. **Polynomial algebra** (expand, factor, gcd, a canonical form) beside
   `simplify` (S231's review), before step 25's derivative needs a
   canonical form to simplify into.
6. **Assumptions**, the conditions under which simplification holds
   (S228's review).

Sources, as of 2026-10-25:

- math.js, its features and function reference:
  https://mathjs.org/ and https://mathjs.org/docs/reference/functions;
  its algebra, `simplify`'s context and `realContext`:
  https://mathjs.org/docs/expressions/algebra.html
- SymEngine: https://symengine.org
- GiNaC: https://en.wikipedia.org/wiki/GiNaC and
  https://www.ginac.de/RelatedSoftware.html (CLN)
- ExprTk: https://www.partow.net/programming/exprtk/index.html
- P3864R1, Correctly rounded floating-point maths functions (2026-02-22):
  https://open-std.org/JTC1/SC22/WG21/docs/papers/2026/p3864r1.html
- MSVC's C++23 `constexpr` `<cmath>` with LLVM libc (2026-09-10):
  https://devblogs.microsoft.com/cppblog/msvc-c23-constexpr-cmath-with-llvm-libc/
- Correctly rounded math in production with LLVM libc (2026-09-16):
  https://devblogs.microsoft.com/cppblog/bringing-correctly-rounded-math-to-production-with-llvm-libc/
- LLVM libc's math functions: https://libc.llvm.org/math
- math.js's `ctranspose`, the conjugate transpose (chunk 43, 2026-11-02):
  https://mathjs.org/docs/reference/functions/ctranspose.html
- glibc's `clog10`, a GNU extension (chunk 42, 2026-11-02):
  https://linux.die.net/man/3/clog10
- The UCRT's `clog10`, `clog10f` and `clog10l` (chunk 42, 2026-11-02):
  https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/clog10-clog10f-clog10l
- glibc taking `cosf` and `sincosf` from CORE-MATH (patches of 2026-03-25):
  https://patchwork.sourceware.org/project/glibc/patch/20260325192357.1284741-6-adhemerval.zanella@linaro.org/
- The rest from the libraries' documentation as of the lane's knowledge:
  SymPy, muParser, TinyExpr, Eigen, Boost.Math, GMP, MPFR, FLINT, gcem.
