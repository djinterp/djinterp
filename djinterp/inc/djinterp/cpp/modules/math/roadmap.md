# djinterp::math — roadmap

What the math module builds next, in the order the owner set on 2026.10.13:

1. the framework's bones: a generic value, then every header in place;
2. parsing and evaluating arithmetic, math functions, linear algebra and
   calculus;
3. precision, benchmarks and bignum.

`README.md` describes what exists today.

Status legend: `[ ]` todo, `[~]` in progress, `[x]` done. An item is about one
chunk: one coherent change, delivered with a record, a note and a zip of the
changed files, and reviewed before the next one starts.

---

## Where it stands

The C core and the scalar, interval, linear algebra, coordinate and geometry
faces compile from ISO strict C++98. The C core also holds the generic
value's core: exact rationals with correctly rounded conversions to floating
point (`rational.h`, chunks 16 and 17), the value itself, with its kinds,
tower and errors (`value.h`, chunk 18), correctly rounded reading of
decimal text (`decimal.h`, chunk 19), and literals read as exact values
(`literal.h`, chunk 20). `numeric_compare` compares numbers written as text
exactly, from ISO strict C++98 (chunk 21). The value takes boxed kinds, each
through a table of its own (chunk 22); the first real one is a wide integer
of 256 bits (chunks 23 and 24), and `value.h` did not change for it. Its
C++ face, `math::value`, holds any kind and a box as an `int` holds its
number, and with `inline_value` computes in constant expressions (chunk 25).
`number_traits` and `common_number` tell generic code what it needs of a
number type, so vectors and matrices of values compute exactly (chunk 26):
Phase 1 is done. All of it is checked by 25 C harnesses and 31 face tests in
360 and 1,984 builds.
`eigenvalues_qr` and `general_eigen` were fixed in chunk 13 (M1 and M2).

Not ported yet: the compile-time expression engine and what stands on it,
function, calculus and values (C++14 for now).

The framework's shared expression core, `parse/expression/`, has a term type,
folds, a parser, a renderer and a compile-time term. P0 (chunk 27) fixed what
stopped each part compiling once used, and a face test now runs all of it.
On it stands the skeleton of math's run-time expression language,
`math/expr/` (2a, chunk 28): every header of Phases 2 to 4, its types whole
and its entries stubs, which the steps from 2b on fill. 2b (chunk 29) filled
its arithmetic: numbers and `+ - * / mod ^` parse, print and evaluate, in
each floating-point type and exactly as a value. 2c (chunk 30) filled its
functions: calls of the C core's functions, the factorial and the constants
`pi` and `e`, exact as values wherever a function's value is rational. 2d
(chunk 31) filled its relations and logic: `= != < <= > >=`, `and`, `or`
and `not`, booleans as values and 1 and 0 in a type, a chain `a < b <= c`
the conjunction of its links, and piecewise functions. 2e (chunk 32) filled
its handle, `math::expr`: a term or the error that kept it from being one,
whose members chain, `math::parse("x^2 * sin(x)").diff("x").str()`, an
error passing through each to the chain's end. 2f (chunk 33) filled its
rewrites: substitution, and simplification, which folds constants exactly,
applies the identities and collects like terms, `2 * x + 3 * x` being
`5 * x`. Phase 2 is done. 3a (chunk 34) began Phase 3: a matrix is a boxed
kind of the value, each element kept as it is given, densely where all are
floating-point numbers, and the literals `[1, 2, 3]` and `[1, 2; 3, 4]`
parse, print and evaluate to one. Chunk 34 also reviewed every decision
taken, S1 to S245 (`decisions.md`), and compared the framework as proposed
with similar libraries (`comparison.md`). 3b (chunk 35) filled the
operators on matrices: sums, scalar products and the matrix product, each
by linear algebra's kernels, the transpose `A'`, powers `A ^ n` by repeated
squaring, and indexing from 1, `v[i]` and `A[i, j]`; simplification keeps
the order of matrix factors. 3c (chunk 36) filled linear algebra's
functions, `det`, `inv`, `trace`, `norm`, `eig`, `dot`, `cross` and `solve`,
exact for exact matrices and in a dense one's type by the C core, with
negative powers, the empty matrix `[]` and block literals `[A, b]`; the C
core's `d_mat_power` squares as the language does. On 2026.10.27 the
owner added three steps to Phase 3 before Phase 4, 3d to 3f: complex
numbers and quaternions as kinds of the value (S265 to S278). 3d's first
part (chunk 37) made the C core's complex numbers, `cplx.h`: C99's 22
functions, 18 in both forms, each within the bound its header states,
Annex G's values as C17 has them, and the real `log1p`, `expm1`, `asinh`,
`acosh` and `atanh` they need, every line run by its harness against
references made by mpmath (S279 to S290). Its second part (chunk 38) made
the linear algebra they complete: a general real matrix's eigenvalues,
complex pairs among them, and eigenvectors by LAPACK's path, balancing,
the Hessenberg form, Francis's double-shift QR and the Schur vectors; a
complex matrix's LU, determinant, inverse and solution, a Hermitian one's
eigenvalues by Jacobi and any other's by complex QR, in `cmat.h`; the C
core's norms scaled at the range's ends; the elimination of [A | B] in
every family; and `number_traits` written again, its tolerance relative
(S291 to S311). Its third part (chunk 39) made the C++ face,
`math::complex<T>`, from C++98: `cplx.h`'s arithmetic and C99's functions,
the `_c` forms at every level, a field to `number_traits`, so that vectors
and matrices of it compute, their determinant, inverse and solution
`cmat.h`'s bit for bit; `general_eigen` gives every eigenvalue and
eigenvector as complex numbers; and in the language `im`, `jm` and `km`
answer `not_yet` and the stubs name their new steps (S312 to S323). 3d is
done.

From chunk 28 the lane works on the owner's tree of 2026.10.23, and from
chunk 37 on his tree of 2026.10.27, with the lane's files laid over it (the
README's section 8).

---

## Progress

Every item below has a step number, in the order the lane plans to take
them: 51 steps from Phase 0 to Phase 8. Each chunk's note says which step it
is, as "step 6 of 51". If the order changes, the numbers are redone and the
note says so: on 2026.10.27 the owner added 3d to 3f to Phase 3 (S265), and
every step from 22 on moved three further, 4a from step 22 to step 25.

| Phase | Steps | Done |
|---|---|---|
| 0: Ground truth (M0, chunk 12, M1 + M2, C4a, C2) | 1 to 5 | 5 of 5 |
| 1: The generic value (1a to 1f) | 6 to 11 | 6 of 6 |
| 2: The expression bones (P0, then 2a to 2f) | 12 to 18 | 7 of 7 |
| 3: Linear algebra, complex numbers and quaternions in expressions (3a to 3f) | 19 to 24 | 5 of 6 |
| 4: Calculus in expressions (4a to 4d) | 25 to 28 | 0 of 4 |
| 5: The compile-time face and the remaining ports (5a to 5g) | 29 to 35 | 0 of 7 |
| 6: Precision (6a to 6e) | 36 to 40 | 0 of 5 |
| 7: Comparison with other libraries (7a, 7b) | 41 and 42 | 0 of 2 |
| 8: Bignum (P1 to P7) | 43 to 49 | 1 of 7 |
| When ready: C6 (the 09.29 lane) and C4b (a CI workflow) | 50 and 51 | 0 of 2 |
| **All** | **1 to 51** | **24 of 51** |

"Later, not yet decided" is not counted.

---

## Principles

These are the owner's rulings. Each item below follows them.

1. **The C core is the foundation.** Algorithms are written once in C. The
   C++ faces wrap them and add what each language level allows. Other element
   types take a generic C++ path. (2026.10.04)
2. **Every level gets everything it can express.** Each language level
   supports the most functionality it can, gets every optimization it allows,
   and shares code with the other levels. (N1, 2026.10.13)
3. **Expression, function and calculus start at C++11.** A runtime
   expression for C++98 comes later, if users need it. (N1)
4. **One answer.** A call gives the same result at every language level, and
   the same result in constant evaluation as at run time. That result is the
   correctly rounded one or the C library's. Domain errors give the library's
   answers (NaN, plus or minus infinity). Whether transcendental functions
   must be bit-identical or only within an ulp is gate G6.
5. **Chaining.** Wherever it is appropriate, an operation returns a value
   that the next operation can be applied to. Steps that can fail chain
   through `result<T, E>` with `and_then`. (decision 3)
6. **Eager by default.** Laziness is an explicit opt-in type, available from
   C++11. A lazy evaluation performs the same operations in the same order
   as an eager one, so the bits do not change. (decision 2)
7. **`_fast` variants only on measured demand.** Each is named, has a
   documented ulp bound, and is tested against the `_c` reference. A `_fast`
   variant is never a global build switch, which would break the
   one-definition rule. (decision 1)
8. **The generic value comes before further features.** Any value type,
   bignum included once it exists, must fit without redesign. (decision 4)
9. **Bones before precision.** The headers and their interfaces come first.
   Benchmarks and bignum come last. (decisions 5 and 6)
10. **Stay mergeable.** The lane keeps its changes inside `math/`, `c/math/`
    and their tests, so that the net lane and the others can merge back into
    `master` with few conflicts.

---

## Open decisions (gates)

The lane takes each recommendation unless the owner says otherwise.

| ID | Question | Recommendation | Gates |
|---|---|---|---|
| G1 | The value's kinds and promotion rules | As in Phase 1's design | Phase 1 |
| G2 | What an integer overflow gives before bignum exists | It becomes an inexact `long double` with the inexact flag set. The alternatives are a range-error value, or wrapping (rejected). Once bigint exists, overflow promotes to it | 1b |
| G3 | How decimal literals are read | Exactly: `0.1` is the rational 1/10. Each result type rounds once, correctly | 1c, 2b |
| G4 | Where the runtime expression language lives | In a new directory, `math/expr/`, built on `parse/expression/`'s term, folds and renderer. A math-local parser layer adds calls, lists and matrices. This lane fixes `parse/expression/`'s two defects as its own small commit (Phase 0, P0) | Phase 2 |
| G5 | The surface syntax | The defaults in Phase 2's table. Implied multiplication (`2x`) and `\|x\|` are options, off by default | 2b |
| G6 | What "one answer" means for transcendental functions | Correctly rounded functions are bit-identical everywhere (as now). For the others, `_c` is the reference and the run-time library is within an ulp of it; `_c` can be called at run time when bits must match. Bit-identity everywhere follows once Phase 6 makes those `_c` forms correctly rounded | Phase 5, Phase 6 |
| G7 | Order of the small items | M1 and M2, then C4a, then C2, then Phase 1. P0 opens Phase 2; C4b and C6 come when ready. Each is one short chunk; any can move after Phase 1 | Phase 0 |
| G8 | Running on Windows | WSL 2 now (README section 5); a CMake and CTest runner that includes MSVC in C4c | C4 |
| G9 | The 09.29 lane | Mine it for what 1a's rationals lack, and for its D13 pattern. This needs `djinterp_math_modified_files.zip`, which is in no repository and not in this session | C6 |

---

## Phase 0 — Ground truth

- [x] **M0** (step 1) The later package (8ac2c70) merged (chunk 11).
- [x] **Chunk 12** (step 2) This README and roadmap. The C3 floor comments
  fixed in seven test headers. `master` made the main branch (C1).
- [x] **M1 + M2** (step 3) (chunk 13) `eigenvalues_qr`: a Hessenberg
  reduction, then Wilkinson-shifted single-shift QR steps confined to the
  unreduced block, deflation relative to the type's epsilon, 2x2 blocks solved
  in closed form. `general_eigen`: inverse iteration shifted in the element
  type's precision, on the matrix scaled by a power of two, U x = e first.
  Both in `decomp.h` (generated, `_c` and plain), `eigen.hpp` calling them.
- [x] **C4a** (step 4) (chunk 14) `ci/check_math_tests.sh` runs every harness
  and face test, found afresh on each run (then 17 and 26), JOBS at a time,
  in two tiers. The smoke tier uses gcc and g++ only: 95 builds, 8 minutes on
  one core here and 4 on two. The full tier is the matrix: 1,896 builds, 71
  minutes on two cores. The README's section 5 says how, WSL 2 included.
- [x] **C2** (step 5) (chunk 15) `D_MAYBE_UNUSED` on the root's two
  `D_STATIC_ASSERT` fallback typedefs, so the assertion serves inside
  functions at C99 and C++98 under `-Wall`. `linalg_requires` retired: its 31
  uses are `D_STATIC_ASSERT`s again, with messages from C++11. Both whole
  ladders rerun.
- [x] **P0** (step 12) (chunk 27) `parse/expression/` usable. Nothing had
  instantiated it, and five kinds of defect stopped each part compiling once
  used: the parser's `_state.offset()` (a data member, four sites); the
  renderer's `evaluate` call and every function of `expression_ops.hpp`,
  whose calls left OpId and Atom to be deduced through the term's alias,
  where they cannot be; `static_expr`'s constructor, not `constexpr`, so no
  static term was a constant; and a nullary static fold's unused handler,
  an error under `-Wall -Wextra -Werror`. Each is fixed where it was, with
  nothing else changed. `t_parse_expression_face.cpp` runs every part of the
  module, 4,143 checks from C++11 in the 64-build matrix, 2,000 random
  terms rendered and parsed back among them, and catches each of 59 planted
  faults.
- [ ] **C6** (step 50) The 09.29 lane: its runtime rational compared with
  1a's, and anything 1a lacks brought in; D13 kept as a pattern; its scalar
  and interval kernels dropped (math98 has them). Waits on the zip (G9).
- [ ] **C4b** (step 51) A GitHub workflow: the smoke tier on every push, the
  full matrix nightly with `-m32`.

---

## Phase 1 — The generic value

### Design (proposed; G1 to G3)

**Two faces, as everywhere in this module.**

- *Dynamic.* One run-time type that can hold any number. A new hand-written
  C header, `c/math/value.h` (C99, and C++98 when compiled as C++), defines
  `struct d_math_value`. A C++ class, `math::value` in `math/value.hpp`
  (C++98), wraps it. The expression evaluator, the parser's literals and the
  interpreter all use it.
- *Static.* Templates over a number type, as linear algebra and interval
  already are. `number_traits<T>` and a promotion trait, `common_number<A, B>`,
  steer them (C++98). A bignum type joins by specializing the traits.

**Kinds.** Every value has a kind.

- *Inline kinds* are stored in the value, with no allocation:
  - none;
  - error: domain, pole, range or invalid;
  - boolean;
  - integer: exact, `d_math_imax`;
  - rational: exact, reduced, a pair of `d_math_imax`;
  - `float`, `double`, `long double`.
- *Boxed kinds* come from a registry. Each registered kind supplies a table of
  operations: its name, its place in the tower, retain and release,
  arithmetic, comparison, conversion and text. Bigint, big rational, big
  float, complex, interval, vector and matrix will all be boxed kinds.
  Registering one changes neither `value.h`'s interface nor the inline kinds'
  code.

**The tower.** The order is integer, then rational, then `float`, `double`,
`long double`, then big float at a precision *p*.

- Exact operations stay exact when the result can be represented. `+`, `-`
  and `*` on integers give an integer. `/` gives a rational, so `1/3 + 1/6`
  is exactly `1/2`.
- Exact combined with inexact gives an inexact result in the inexact
  operand's format. Two inexact operands give the wider format.
- Transcendental functions of exact arguments are inexact. They are computed
  in the context's format, `double` by default.
- When an operation involves a boxed kind, the operand higher in the tower
  handles it and converts the other operand. This is the same protocol as
  Ruby's `coerce` and Python's reflected operators.

**Context.** A context carries the precision for big floats, the rounding mode
(round to nearest, ties to even, by default) and the error policy (errors as
values, or a flag). It is passed explicitly, as in MPFR. There is no global
state.

**Literals.** Text is read exactly:

- `42` is an integer;
- `0.1`, `1e-3` and `2/3` are rationals;
- a literal too long for two `d_math_imax` becomes the nearest `long double`,
  until bignum exists.

Converting a rational to `float`, `double` or `long double` is correctly
rounded: `d_rational_to_f`, `_to_d` and `_to_ld` (chunk 17), constant
expressions from C++14, so each type rounds a literal once.

**Errors.** Float kinds follow IEEE 754, as the C core does: NaN and plus or
minus infinity. Exact kinds have no NaN, so `1/0` and `0/0` on exact kinds give
an error value (pole, invalid). The error policy can choose a flag plus a NaN
instead.

### Items

- [x] **1a** (step 6) (chunk 16) `c/math/rational.h`, written here, as the
  zip had not arrived: rationals over `d_math_imax` in one reduced form, with
  make, neg, abs, inv, add, sub, mul, div, sign, eq and cmp. Each kernel
  stores the exact result or returns why it cannot (overflow, pole, invalid),
  and reports overflow only when the reduced result does not fit: Knuth's
  forms, with the intermediates in two words. Constant expressions from
  C++14. `t_rational.c` checks every kernel against an exact reference of
  256 bits in 16-bit limbs (the plan said 128; the checks need 190), in 16
  builds, and catches each of 21 faults planted on purpose.
- [x] **1b** (step 7) In two chunks, since promotion needs the
  conversions that were 1c's.
  - [x] (chunk 17) Correctly rounded conversions in `rational.h`: a rational
    to the nearest `float`, `double` and `long double`, ties to even,
    whatever the rounding mode; and a ratio of two-word integers to
    `long double`, for an exact result too wide for a rational (G2). The
    rounding is done once, in integers, and the result built exactly.
    `t_rational.c` checks each result against its neighbours, exactly, and
    catches each of 14 more planted faults.
  - [x] (chunk 18) `c/math/value.h`, `struct d_math_value` with the inline
    kinds, as designed above. Exact operations stay exact, an integer
    where the denominator comes to 1; an exact result too wide for
    `d_math_imax` is the nearest `long double`, flagged inexact (G2); exact
    with inexact converts to the inexact type, two inexact kinds meet in
    the wider. Errors are values; a boolean or none in arithmetic is a
    domain error. Comparison is exact across kinds. No union, so every
    operation is a constant expression from C++14; no context parameter
    yet, as nothing reads one. `t_value.c` checks every operation on every
    pairing of kinds against `math_tests_exact.h`, the exact reference now
    shared with `t_rational.c`, and catches each of 22 planted faults.
- [x] **1c** (step 8) In three chunks, since correct rounding from text
  needs a conversion of its own before text can become a value.
  - [x] (chunk 19) `c/math/decimal.h`: a decimal number of any length to
    the nearest `float`, `double` or `long double`, ties to even, whatever
    the rounding mode, with overflow and underflow reported. Simple Decimal
    Conversion, its buffer sized so that the rounding is exact for every
    input. Constant expressions from C++14. `t_decimal.c` checks every
    result against an exact reference of its own, integers of up to 54,400
    bits: values of each type and the midpoints between them, written out
    exactly and nudged both ways, within the buffer and past it; random
    texts of up to 2,000 digits; and texts of a million characters. It
    catches each of 34 planted faults.
  - [x] (chunk 20) `c/math/literal.h`: text to value. Integers, decimals,
    scientific notation, fractions (`2/3`) and mixed numbers (`1 1/2`). A
    literal is exact wherever its value is a rational over `d_math_imax`,
    so `0.1` is the rational 1/10, however many digits it is written with;
    otherwise it is the nearest `long double`, flagged inexact (G2). A
    fraction over 0 is a pole, or invalid for 0/0; a literal past every
    `long double`, or a fraction whose terms pass two words, a range error
    until bignum. Read for a floating-point type, a decimal literal rounds
    once, from the text, through `decimal.h` (G3). Constant expressions
    from C++14. `t_literal.c` checks every value against
    `math_tests_exact.h`, reading each literal itself, and catches each of
    25 planted faults; a check of the record's, on Python's exact
    rationals, agrees on some 25,000 more texts in each of four builds.
  - [x] (chunk 21) `math/numeric_compare.hpp` to C++98 on its header's
    design (M3, the owner's ruling of 2026.10.09): two numbers written as
    text, in `literal.h`'s syntax, compared exactly at any length and any
    exponent, as `rational_value`s of `integer_value` digits (a power of
    ten held apart, so `1e1000000` is two digits); a text that is not a
    number is unordered, as a NaN is. Its `.cpp` became a translation unit
    that includes it. `t_numeric_compare_face.cpp` checks it in 64 builds,
    against the C core's exact values and against rewrites of each number,
    and catches each of 28 planted faults; a check on Python's exact
    arithmetic agrees on 40,000 more pairs in each of four builds.
- [x] **1d** (step 9) The boxed-kind registry, plus one real kind to prove
  that a new kind needs no change elsewhere: a fixed-width wide integer,
  which is also bignum's first step (P1). In three chunks, since the
  integer's arithmetic is a header of its own:
  - [x] (chunk 22) The registry, in `value.h`. A kind that is not inline
    fills in a table, `struct d_math_boxed_kind`: its name; its rank, its
    place in the tower on one scale with the inline numbers', and its own;
    whether it is a number, and exact; retain and release, arithmetic,
    comparison, conversion and text. A boxed value points to its kind's
    table, so there is no list of kinds and no global state. The operand
    higher in the tower handles an operation: a boxed kind through its
    table; an inline number by converting the boxed operand to its own
    kind, or for a comparison to the nearest `long double` with the
    rounding's direction, so that comparison stays exact. Errors pass
    through, and a boolean or none is a domain error, before any kind is
    asked. A result is the caller's, given back with
    `d_math_value_release`; operands are lent. The inline kinds' code is
    unchanged, and still constant expressions from C++14.
    `t_value_boxed.c` checks it with four kinds of its own, three of them
    mirrors of inline values at ranks between and beyond theirs, against
    `value.h`'s own inline arithmetic, counting every call into a table and
    every box; it catches each of 29 planted faults, and a check of the
    record's, on Python's exact rationals, agrees on 20,000 cases in each
    of four builds.
  - [x] (chunk 23) `c/math/wide.h`: signed integers of 256 bits, a
    magnitude in limbs of half a word (32 bits, or 16 where no 64-bit
    integer exists) and a sign. Exact or nothing, as `rational.h`: add,
    subtract, multiply, divide with remainder (Knuth's algorithm D,
    truncated as C's), gcd, comparison; conversions to and from
    `d_math_imax`, `d_math_umax` and rationals, and text both ways; the
    nearest `float`, `double` and `long double`, ties to even, with the
    rounding's direction, a float past the largest an infinity. No
    allocation; constant expressions from C++14, the text writer apart.
    Under the type, limb kernels of any length, for the boxed kind's
    intermediates and bignum. `t_wide.c` checks it against a reference of
    its own, 640-bit naturals in 16-bit limbs, and a nearest-value oracle,
    in 16 builds at both limb widths; every line of `wide.h` runs under it,
    and it catches each of 58 planted faults. A check of the record's, on
    Python's integers, agrees in each of four builds. This is P1 (step
    40), done early, and checked off with it.
  - [x] (chunk 24) `c/math/value_wide.h`: the wide integer as a boxed kind,
    the first real one, and `value.h` unchanged. Rank 210, between the
    rational and the float. With a wide integer, an inline integer or a
    rational its arithmetic is exact: an integer stays a wide integer,
    however small; a result that is no integer is an inline rational where
    it fits the family; and past both, an integer of more than 256 bits or a
    ratio past `d_math_imax`, the nearest `long double`, flagged inexact
    (G2), rounded once from the exact ratio of terms up to 512 bits by
    `wide.h`'s division. Comparison is exact, and a float kind meets it as
    any boxed kind, converted to the nearest value of its type. Each value a
    box of its own, through two allocation knobs
    (`config/c/math/cfg_value_wide.h`); no storage, a memory error.
    `t_value_wide.c` checks it against `math_tests_exact.h` at 768 bits, in
    16 builds, counting every box and refusing some, with kinds of its own
    below the wide integer's rank; every line of the header runs under it,
    and it catches each of 67 planted faults. A check of the record's, on
    Python's fractions, agrees in each of four builds.
- [x] **1e** (step 10) (chunk 25) `math/value.hpp`, the value's C++ face.
  `math::value` holds a value of any kind, its box as an `int` holds its
  number: from C++98 a copy retains and a destructor releases, and from
  C++11 a value moves, which never fails; `adopt` and `detach` pass a hold
  to and from C. `math::inline_value` holds the inline kinds alone, a
  literal type whose operations are constant expressions from C++14;
  `math::value`'s are from C++20, for the inline kinds, where a destructor
  may be constexpr. Both are made implicitly from the arithmetic types, a
  `bool` and an error code, and refuse a pointer; an unsigned integer past
  `d_math_imax` is G2's long double, through `value.h`'s new
  `d_math_value_from_uint`. Operators, `abs` and the six comparisons are
  hidden friends, found by argument-dependent lookup only, and each makes
  its result in place from `value.h`'s, so it costs what `value.h`'s call
  does; from C++20 `<=>` is a partial ordering, and the concept `number`
  names the types generic code computes with. `t_value_face.cpp` checks
  every operation on 57 operands against `value.h`, counts every box
  through a counted copy of the wide integer's table, and catches each of
  51 planted faults, in 64 builds; `t_value.c` checks
  `d_math_value_from_uint` against its exact reference, and catches each of
  3 more. The expression engine's factory
  named `value`, which hid the class, is gone: `constant` does the same.
- [x] **1f** (step 11) (chunk 26) The static side, in
  `math/number_traits.hpp`. `number_traits<T>` says what generic code needs
  of a number type: whether its arithmetic is exact and whether it holds
  integers alone, its zero and one, `|x|`, its square root, its conversions
  to and from `double`, and its tolerance; the standard integer and
  floating-point types have their own, `math::value` and `inline_value`
  theirs, with a root exact for the square of a rational, and a type with
  none is taken through `double`, as the generic path always took one.
  `common_number<A, B>` is the type of `a + b` from C++98, and the concept
  `number` moved here. Linear algebra's generic path reads its constants,
  roots and tolerances from the traits; a vector or matrix made by default
  is zero in any number type, and its components may be the type's own
  values. `t_number_traits_face.cpp` checks every arithmetic type's traits
  and every pair's `common_number`, and runs `vector` and `matrix` over
  `math::value` and over a rational type of the test's own: the Hilbert
  matrix's determinant and inverse exactly, and from C++14 as constant
  expressions over `inline_value`; it catches each of 26 planted faults, in
  64 builds. `value_wide.h`'s table entries are `D_MATH_FN_ENTRY`, never
  forced inline, so it builds at gcc's `-Og`.

---

## Phase 2 — The expression bones: parse, evaluate, print

### Design (proposed; G4, G5)

**Layers.** The language is built in six layers:

1. **Tokens.** Math text becomes numbers, names, operators, brackets, commas
   and semicolons.
2. **Grammar.** A math operator table (`parse/expression`'s
   `operator_signature`) and a math-local parser layer. That layer adds what
   the shared parser lacks: calls with argument lists, vector and matrix
   literals, and telling unary minus from binary minus by position.
3. **Term.** `parse/expression`'s `expression<math_op, math_atom>`. An atom is
   a literal value, a variable or a named constant.
4. **Folds.**
   - `evaluate<T>` for `float`, `double` and `long double`: typed and fast,
     with each literal rounded once and functions taken from the C core.
   - `evaluate(context)`: gives a `value`, exact where possible.
   - `render`: back to text.
5. **Rewrites.** Substitution, simplification (rules applied to a fixed
   point), and the derivative (Phase 4).
6. **The handle.** `math::expr`, which wraps all of the above in chainable
   members.

**Headers.** 2a (chunk 28) made them all, under `math/expr/`, in namespace
`djinterp::math`; each step fills its part:

| Header | Layer | Filled by |
|---|---|---|
| `expr_common.hpp` | the error, `expr_result`, G5's switches | 2a (whole) |
| `token.hpp` | 1, tokens | 2b; 2c (the comma); 3a (`[ ] ;`) |
| `operator.hpp` | 2, the operator table | 2b; 2c (calls, `!`); 2d (relations, logic); 3a, 3b |
| `parse.hpp` | 2, the parser layer | 2b; 2c (calls); 2d (chains, piecewise); 3a (literals); 3b (indexing, `'`); 3c (`[]`, blocks) |
| `term.hpp` | 3, the term | 2a (whole) |
| `evaluate.hpp` | 4, to numbers and values | 2b; 2c; 2d; 3b; 3c (blocks) |
| `function.hpp` | 4, the function and constant tables | 2c; 2d (piecewise, `true`, `false`); 3b (`^` and relations of matrices); 3c; 4a (rules) |
| `render.hpp` | 4, to text | 2b; 2c; 2d; 3a; 3b (indexing) |
| `substitute.hpp` | 5 | 2f |
| `simplify.hpp` | 5 | 2f; 3b (matrix factors in order); 3c (blocks never square by structure); 4a |
| `expr.hpp` | 6, the handle, and every header | 2e |
| `matrix.hpp` | Phase 3 | 3a, 3b, 3c |
| `derivative.hpp` | Phase 4 | 4a |
| `numeric.hpp` | Phase 4 | 4b |
| `series.hpp` | Phase 4 | 4c |
| `antiderivative.hpp` | Phase 4 | 4d |

**What the shared parser does today.** This was measured with chunk 12's
probe, `_handoffs/math98_chunk12_2026.10.13/`, with P0's fixes applied:

- It parses, prints and evaluates these correctly: `2 * x ^ 2 + 1`,
  `-x ^ 2` (that is -4 at x = 2), `2 ^ 3 ^ 2` (512), and
  `sin(x) ^ 2 + cos(x) ^ 2`.
- It rejects `atan2(1, x)`, `[1, 2, 3]`, `2x` and `|x - 3|`.
- It prints a function as a prefix operator, `sin x ^ 2`, which a person
  reads as sin(x²).
- Its operator lookup cannot see position, so the probe had to track whether
  a minus was unary or binary itself.
- It reads no nullary operator back, though the renderer prints one, and it
  groups a non-associative chain, `a = b = c`, to the right rather than
  refusing it (chunk 27's record, section 6). A constant is an atom here, and
  2d (chunk 31) reads a chain of relations as the conjunction of its links.

**Proposed syntax (G5).** The defaults are conventional, so that common
inputs parse as a reader expects.

| Binds | Form | Associativity | Example |
|---|---|---|---|
| loosest | `,` and `;` (separators in calls and literals) | — | `f(a, b)`, `[1, 2; 3, 4]` |
| | `or`, then `and`, then `not` | left; `not` prefix | `x > 0 and x < 1` |
| | `=` `!=` `<` `<=` `>` `>=` | none: a chain is the conjunction of its links | `x^2 < 2`, `0 < x <= 1` |
| | `+` `-` | left | |
| | `*` `/` `mod` | left | |
| | unary `-` `+` | prefix | `-x^2` = -(x²) |
| | postfix `'` (transpose) | postfix | `A'`, `A^2'` = (A²)ᵀ |
| | `^` | right | `2^3^2` = 2⁹ |
| | postfix `!` (factorial) | postfix | `n!` |
| tightest | calls `f(x, y)`, indexing `v[i]`, brackets | — | `atan2(y, x)`, `A[1, 2]` |

The table first listed the relations loosest, above logic, which would have
read its own example `x > 0 and x < 1` as `x > (0 and x) < 1`. 2d (chunk
31) put logic below the relations, as C, Python and Mathematica have it, so
that `not x = y` is `not (x = y)` (S213), and reads a chain `a < b <= c` as
`a < b and b <= c`, as Python and mathematics do (S214). The table first
put `'` beside `!`, tighter than `^`, which would have read `A^2'` as
`A^(2')`, that is `A^2`, where MATLAB, whose `'` the language takes, reads
`(A^2)'`. 3b (chunk 35) put the transpose between the unary minus and `^`,
so that `A^2'` is `(A^2)'` and `A'^2` is `(A')^2`, as MATLAB reads both
(S252).

Further conventions:

- `log` is the natural logarithm, as in C. `log10` and `log2` are separate
  functions.
- The constants are `pi` and `e`, and the booleans `true` and `false`.
- Piecewise functions are written in `function.hpp`'s vocabulary:
  `piecewise(when(x < 0, -x), otherwise(x))`.
- `'` is the transpose, as in MATLAB. Derivatives are written with `diff`.
- Calculus is written as functions: `diff(f, x)`, `diff(f, x, n)`,
  `int(f, x)`, `int(f, x, a, b)`, `sum(f, k, a, b)`, `prod(f, k, a, b)`,
  `lim(f, x, a)`, `taylor(f, x, a, n)`.
- Linear algebra is written as `[1, 2, 3]` and `[1, 2; 3, 4]`, `A * B`, `A'`,
  and functions: `det`, `inv`, `solve`, `dot`, `cross`, `norm`, `trace`,
  `eig`.

**The handle (chaining, decision 3)**. Names are provisional:

```cpp
math::expr  f = math::parse("x^2 * sin(x)");
double      d = f.diff("x").simplify().eval<double>(math::at("x", 2.0));
std::string s = f.diff("x").simplify().str();    // "2*x*sin(x) + x^2*cos(x)"
math::value q = math::parse("1/3 + 1/6").eval(); // exactly 1/2
math::expr  g = f.subs("x", math::parse("t + 1"));
auto        r = math::try_parse(text).map(       // result<expr, parse_error>
    [](const math::expr& e) { return e.diff("x"); });
```

Each member returns a new `expr`, so every step chains. A `value` chains the
same way: `v.abs().sqrt()`, and for linear algebra `A.transpose().inverse()`.

2e (chunk 32) made the handle so, with what 2a's skeleton changed: a member
that ends a chain in a number gives an `expr_result`, as every step that can
fail does (principle 5), so that `d` above is an `expr_result<double>` and
`q` an `expr_result<value>`, holding exactly 1/2. A handle holding an error
passes it through every member after it, and `str()` is then its message;
of a term, `str()` writes as the renderer does, `x ^ 2 * sin(x)`.

### Items

- [x] **2a The skeleton.** (step 13) (chunk 28) Every header of Phases 2 to
  4 under `math/expr/`, sixteen, each with its banner, table of contents and
  declarations, from C++11 and empty below (the table under **Headers**).
  Its types are whole: the error and its statuses; the token; every
  operator kind of the three phases, and the operator table; the atom and
  the term, `expression<expr_op, expr_atom>`, a function an operator of it,
  with constructors and observers; the function and constant tables; the
  bindings and the context; a matrix's shape; the handle, which holds a term
  or an error. Each entry is a stub, which answers `not_yet`, its message
  naming the step that fills it, or where it cannot fail the placeholder its
  contract names. `t_expr_face.cpp` checks every type and every stub, 122
  checks from C++11 and 1 below, in the 64-build matrix, and catches each of
  65 planted faults. The lane's tree is now the owner's of 2026.10.23, with
  the lane's files laid over it.
- [x] **2b Arithmetic.** (step 14) (chunk 29) Tokens, the operator table
  and the parser layer. The tokenizer reads numbers by `decimal.h`'s
  grammar, names, every symbol of G5 and the round brackets. The table holds
  `+ - * / mod ^` and the unary `-` and `+` at G5's levels. The parser layer
  gives each token its role, a minus unary or binary by where it stands and
  G5's implied multiplication an option, off by default, and hands them to
  `parse/expression`'s climber; every error is at its place in the text.
  Both folds compute the arithmetic: in `float`, `double` and `long double`,
  each literal rounded once (G3), `^` the C core's pow and `mod` the floored
  remainder, which has the divisor's sign; and to a `value`, exact where it
  can be: `1/3 + 1/6` is 1/2, `4 ^ 0.5` is 2 and `7.5 mod 2` is 3/2. The
  renderer writes a term as text that `expr_parse` reads back as the same
  term, every literal kept. `t_expr_face.cpp` checks each, 2,000 random
  terms rendered and parsed back among them: 219 checks from C++11 and 1
  below, in the 64-build matrix, and it catches each of 142 planted
  faults.
- [x] **2c Math functions.** (step 15) (chunk 30) A function registry:
  name, arity, an evaluator per type from the C core, and a slot for the
  derivative rule. The standard table holds the C core's 25 functions with
  `abs` and `factorial`, the constant table `pi` and `e` from the literals of
  `constants::of<T>`. Calls with argument lists: a name before an open
  bracket is a call, read by the parser layer as one operand, its arguments
  split at the commas; the factorial is postfix, tighter than `^`; with G5's
  bars on, `|x|` is `abs(x)`. In a type a function is the C core's plain
  form; as a value, exact wherever its value is rational (`sqrt(4/9)` is
  2/3, `log10(1000)` 3, `sin(0)` 0), an exact argument outside its domain a
  domain error, and otherwise the C core's function in the tower's kind.
  `t_expr_face.cpp` checks each, calls, constants and the factorial in the
  random round trip among them: 266 checks from C++11 and 1 below, in the
  64-build matrix, and it catches each of 312 planted faults.
- [x] **2d Relations, logic and piecewise.** (step 16) (chunk 31) The
  table holds `= != < <= > >=` and `and`, `or` and `not`, logic below the
  relations (S213), the relations non-associative. The parser layer reads a
  chain, `a < b <= c`, as the conjunction of its links (S214), and every
  pair of round brackets as one operand, so that a bracketed relation is no
  link (S215). As a value a relation is a boolean, by `value.h`'s exact
  order, so that `0.1 + 0.2 = 0.3` holds; logic is Kleene's, a false operand
  making `and` false whatever the other is, errors among them; in a type
  the booleans are 1 and 0, a relation C's comparison. From
  `function.hpp`'s vocabulary, `piecewise(when(c, v), otherwise(d))`, read
  as the call `piecewise(c, v, d)` and written back in its branches (S216),
  and `all_of` and `any_of`; the constants `true` and `false`.
  `t_expr_face.cpp` checks each, relations, logic and piecewise in the
  random round trip among them: 314 checks from C++11 and 1 below, in the
  64-build matrix, and it catches each of 424 planted faults.
- [x] **2e The `expr` handle.** (step 17) (chunk 32) Chaining, and errors as
  `result<>`. `math::expr` holds a term, or the error that kept it from
  being one, and each member is a term-level entry's: `str()` the term
  rendered; `eval<T>` and `eval` the folds, in a type and as a value, each
  an `expr_result`, `eval()` taking no binding and the default context
  (S224); `subs`, `simplify` and `diff` handles of the rewrites' terms or
  errors. `parse` and `try_parse` are `expr_parse`'s, the one a handle and
  the other an `expr_result<expr>`. An error passes through every member
  after it and comes out where the chain ends, `str()` its message and
  `eval` its error; of two, `subs` keeps the handle's own (S225). The
  members delegate (S223): until 2f, `subs` and `simplify` give the term
  unchanged, as their entries do, and until 4a, `diff` a handle of
  `expr_derivative`'s `not_yet`. `t_expr_face.cpp` checks each, roadmap
  Phase 2's examples and the handle against the entries on 1,000 random
  terms among them: 346 checks from C++11 and 1 below, in the 64-build
  matrix, and it catches each of 445 planted faults.
- [x] **2f Substitution and basic simplification.** (step 18) (chunk 33)
  Constant folding, identities (0 + x, 1 * x, x ^ 1), and collecting like
  terms. `expr_substitute` puts a term in place of a variable, by
  `parse/expression`'s substitute; a constant or a function's name is no
  variable. `expr_simplify` folds a node whose operands are exact numbers,
  `true` or `false` to its value, where that is exact or a boolean (S229);
  applies x ^ 1 = x, x ^ 0 = 1 and 1 ^ x = 1; collects a sum's terms by
  their other factors, in any order, their exact coefficients summed
  (S230); and multiplies a product's exact numbers into one coefficient. A
  rule applies only where it makes the term smaller (S231), pass after pass
  until one changes nothing, each sum and product collected once, at its
  root (S232). The value is kept at every binding at which each part of the
  term is finite; elsewhere it may differ, as in other computer algebra
  systems: x - x is 0 at any x (S228). The handle's `subs` and `simplify`
  are theirs. `t_expr_face.cpp` checks each, and on random terms the value
  kept at random exact bindings: 388 checks from C++11 and 1 below, in the
  64-build matrix, and it catches each of 500 planted faults.

---

## Phase 3 — Linear algebra, complex numbers and quaternions in expressions

- [x] **3a** (step 19) (chunk 34) Vector and matrix kinds, boxed: elements are
  `value`s (exact matrices) or a dense `float`, `double` or `long double`
  array. Literals `[1, 2, 3]` and `[1, 2; 3, 4]`. One kind, "matrix", of rank
  1000, past every number's, its table in C++ in `expr/matrix.hpp` (S237,
  S238); each element kept as it is given, as values where any is exact, and
  as a dense array where every one is a `float`, `double` or `long double`
  (S239). `expr_matrix` and `expr_vector` make one, a vector one row (S240),
  and `expr_is_matrix`, `expr_shape_of` and `expr_element` read it. Each
  element is a number: an error passes, the first, and what is no number is a
  domain error; there is no empty literal, and a matrix literal's rows are of
  one length, else `bad_shape` at its place (S241). The literals parse, each
  element any term, render back, and evaluate to a matrix as a value; in a
  type a literal has no value, `bad_shape`, and arithmetic on a matrix is a
  domain error until 3b (S242). A `[` after an operand is indexing, `not_yet`
  until 3b (S243). `t_expr_face.cpp` checks each, 1,000 random literals among
  them: 425 checks from C++11 and 1 below, in the 64-build matrix, and it
  catches each of 569 planted faults.
- [x] **3b** (step 20) (chunk 35) Operators, through the matrix kind's
  table: `A + B` and `A - B` of one shape, `-A`, `A * s`, `s * A` and
  `A / s`, and `A * B` where A's columns are B's rows; anything else on a
  matrix, `A + s`, `s / A`, `A / B`, `|A|`, a domain error, and a 1 by 1
  result a matrix (S246). Each by linear algebra's kernels, a dense
  matrix's by `c/math/mat.h` and `vec.h` in the wider type, a matrix of
  values in `math::value`'s arithmetic, exact; a dense element carries its
  matrix's flags (S247). The transpose `A'`; `A ^ n` of a square matrix and
  a whole n >= 0, exact or floating-point, by repeated squaring, `A ^ 0` the
  identity in A's storage (S248). Indexing from 1: `A[i, j]`, `v[i]` of a
  row or a column, `A[i]` another matrix's row; an index any whole number;
  in a type a NaN (S249). The parser keeps a nesting's frame small (S250).
  Simplification keeps matrix factors in order, and makes no error of
  matrices a value, a variable taken for a number (S251). Indexing is a
  postfix operator of the parser layer's own, at 100; the transpose stands
  between the unary minus and `^`, so that `A ^ 2'` is `(A ^ 2)'` (S252).
  `A = B` and `A != B` compare two matrices (S253). `t_expr_face.cpp`
  checks each, 1,000 random terms of matrices against the test's own
  arithmetic among them: 468 checks from C++11 and 1 below, in the 64-build
  matrix, and it catches each of 648 planted faults.
- [x] **3c** (step 21) (chunk 36) Functions, in the function table:
  `det`, `inv`, `trace`, `norm` and `eig` of one argument, `dot`, `cross`
  and `solve` of two; a number a scalar, as in Julia, `det(2)` 2 and
  `inv(2)` 1/2; in a type the scalar's, `cross` a NaN; a shape a function
  does not take a domain error as a value, `bad_shape` from the C++
  functions `expr_det` to `expr_solve` (S256). The square algebra in one
  type: a dense matrix's own, by the C core, LU with partial pivoting and
  Gauss-Jordan; an exact one's exactly, by the generic kernels, Bareiss's
  elimination for `det`; a mixed one's in its widest floating-point
  element's type; a singular matrix's inverse and solution a pole (S257).
  `eig` ascending, a symmetric matrix's by Jacobi and any other's by
  Hessenberg QR, an exact matrix's rational eigenvalues exact, found as
  roots of its characteristic polynomial, and a complex pair a domain error
  until 3e gives the value a complex kind (S258, S45), which chunk 43 did
  (S352). They run on linear
  algebra's kernels, which `dynamic_matrix` uses, and on the C core
  underneath. `A ^ (-n)` is `inv(A) ^ n` (S259); `[]` the empty matrix, 0 by
  0, `det([])` 1 (S260); a literal's elements blocks, `[A, b]` and `[A; B]`
  (S261); and the C core's `d_mat_power`, the generic kernel's and
  `dynamic_matrix`'s by repeated squaring, as the language's (S262).
  `t_expr_face.cpp` checks each, 1,000 random exact matrices against the
  test's own arithmetic among them: 510 checks from C++11 and 1 below, in
  the 64-build matrix, and it catches each of 753 planted faults; `t_mat.c`
  and `t_matrix_face.cpp` check the power, and catch each of 11 more.
- [x] **3d** (step 22) The C core's complex numbers, and the linear algebra
  they complete (S265 to S270), in three chunks (S279). **Part 1 of 3
  (chunk 37), done:** `c/math/cplx.h`, a struct per floating-point type,
  `struct d_cplx_d` and its kin, of the layout of `T[2]`, never `_Complex`
  (S267, S280); C99's 22 complex functions, the four exact ones once and
  each other in a `_c` form, one long double kernel for the three families
  (`D_MATH_FN_KERNEL`), and a plain form on the C library's real functions
  (S282, S283), with the arithmetic by exact products, the quotient with a
  residual's step (S281), Annex G's values as C17 has them (S284) and
  `_Complex`'s conversions in C (S285);
  the real `log1p`, `expm1`, `asinh`, `acosh` and `atanh` in the C core,
  and exp's kernel on expm1's series, within 0.53 of an ulp (S286); the
  float and double families' exact steps in long double where a target
  evaluates them wider, the x87 (S287). `t_cplx.c` checks each against
  references made by mpmath, kept in the tree, at points exact in float,
  at each family's ends and at double's and long double's full width, and
  every line of `cplx.h` runs (S288). **Part 2 (chunk 38):** Francis's
  double-shift QR after balancing, its eigenvalues complex and its
  eigenvectors from the Schur form (S45, S47, S49); a complex matrix's LU,
  inverse and solution, a Hermitian one's eigenvalues by Jacobi and any
  other's by complex QR; the C core's norms scaled (S256); an elimination
  of [A | B] in both kernel families (S257); `number_traits` of a field
  apart from an order's, the generic kernels keeping each product's order,
  with S157's and S158's recommendations (S269). **Done:** `decomp.h`'s
  `d_decomp_<S>_balance`, `_balance_back`, `_hessenberg_q`, `_schur`,
  `_schur_vectors` and the driver `_eigen`, LAPACK's `dgeev` path, its
  values ascending and its vectors unit with their largest entry real and
  positive, 2^k A's results 2^k times A's bit for bit (S292 to S296);
  `c/math/cmat.h`, complex matrices: LU by |re| + |im|, the determinant,
  Gauss-Jordan's inverse and the solution of [A | B] (S297), Hermitian
  Jacobi with complex rotations (S298) and `zgeev`'s path (S299), their
  kernels never forced inline (S300); `vec.h`'s norms, distance, unit
  vector and cosine scaled past the squares' overflow and underflow, every
  bit in range kept (S301); `mat.h`'s `d_mat_<S>_solve`, LU's bits, and the
  C++ face's `solve` for every number type but an integer one (S302);
  `number_traits` with the field's and the order's traits, the magnitude,
  the epsilon and a relative tolerance, 0 for an exact pair (S303, S304);
  the generic kernels' products in order, pivoting by magnitude (S305); the
  Jacobi tangent past 2^(EMAX/2) (S306). `t_decomp.c` and the new
  `t_cmat.c` check every eigenvalue against references made by mpmath, to
  its condition number, and every part alone (S307). **Part 3 (chunk 39):**
  `math::complex<T>`, from C++98, the struct's face (S270); in the
  language, `im`, `jm` and `km` answer `not_yet`, and the stubs' messages
  take the new step numbers. **Done:** `math/complex.hpp`, its operators
  and functions `cplx.h`'s, the `_c` forms at every level, found by
  argument-dependent lookup (S313); a commutative field with no order to
  `number_traits`, its magnitude LAPACK's |re| + |im| (S314); vectors and
  matrices of it on the generic kernels, the determinant by LU, so that
  determinant, inverse and solution are `cmat.h`'s bit for bit (S315); the
  faces' tolerances of the magnitude's type (S316); `general_eigen`'s
  eigenvalues and eigenvectors complex (S308, S317); `im`, `jm` and `km`
  in the constant table, answering `not_yet` (S318); the stubs' steps 25 to
  28 (S319); `t_expr_face.cpp`'s empty matrix asking first that `[]`
  parsed (S320). `t_complex_face.cpp` checks every operator and function
  against `cplx.h` bit for bit, never its plain forms, and every matrix's
  determinant, inverse and solution against `cmat.h`'s (S323).
- [x] **3e** (step 23) The complex kind in the language (S271 to S273): of
  rank 900, its parts of any real kind, dense or values (S266); `im`,
  `complex(x, y)`, `real`, `imag`, `conj`, `arg` and the modulus; each
  function of the table its principal complex value, `log1p`, `expm1`,
  `asinh`, `acosh` and `atanh` joining it, and `expr_context`'s option for
  real results only (S272); the relations; folds in `math::complex<T>`;
  matrices of complex numbers, dense and of values, `A'` the conjugate
  transpose and `transpose(A)` the transpose, `dot` conjugating, and `eig`
  complex, ordered, and exact where its eigenvalues are Gaussian rationals
  (S273). With it, every boxed kind's holders atomic behind a knob (S134),
  and `min`, `max` and `sign` (S203). In three chunks (S324). **Part 1 of
  3 (chunk 40), done:** the kind, "complex", in `math/expr/complex.hpp`,
  its exact parts values and its floating-point ones a `math::complex<T>`
  (S327), exact or not value by value, as `value.h`'s table now asks
  (S325); its arithmetic exact, or `cplx.h`'s with a real operand as
  itself, its modulus exact where rational (S328); equal by the parts and
  in no order, converting to a real type where its imaginary part is 0
  (S329); written `3 + 4im` by the value layer and `(3 + 4 * im)` by the
  renderer (S330); `im` its unit, a NaN in each type (S332); `complex`,
  `real`, `imag`, `conj` and `arg` in the table, the modulus, `z'` the
  conjugate, `=` and `!=`, `<` and the rest a domain error, exact powers to
  a whole exponent (S333), the functions of the real line's order a domain
  error; what the later parts compute `not_yet` (S331); every box's holders
  atomic by default, `cfg_value.h`'s knob (S326), and the lane's
  configurations in `dconfig.h` (S334). `t_expr_complex_face.cpp` checks
  the exact arithmetic against its own Gaussian rationals and the dense
  arithmetic against `cplx.h` bit for bit. **Part 2, in two chunks
  (S335). Its first (chunk 41), done:** each function's principal complex
  value where `cplx.h` has its complex form, exact where it is a Gaussian
  rational, roots and rational powers by a Gaussian integer found in long
  double and verified exactly (S337), and otherwise `math::complex<T>`'s,
  so the `_c` form, in the tower's type (S336); a real function's complex
  value outside its domain, a real argument on a cut taken as x + 0i
  (S341), and `expr_context`'s `real_results` (S272, S340); `log1p`,
  `expm1`, `asinh`, `acosh`, `atanh`, `sign`, `min` and `max` in the table,
  51 functions (S203, S338); the folds in `math::complex<T>`, a real number
  real there (S339). `t_expr_complex_function_face.cpp` checks the exact
  values on random Gaussian rationals' powers, the dense ones against the
  face bit for bit, and the long double ones against mpmath. **Its second
  (chunk 42), done:** the C core's complex `exp2`, `log2`, `log10`,
  `log1p` and `expm1`, each one long double kernel in pairs, both forms of
  every family that kernel rounded once (S342), the real function's on the
  real axis (S343), `exp2`'s y ln 2 reduced exactly for every y (S344),
  `expm1`'s cancellation near e^x cos y = 1 kept to the pairs' precision
  (S345), and the special values of the functions they are forms of
  (S346), `t_cplx.c` checking them against mpmath; `math::complex<T>`'s
  five; and their values in the language, `not_yet` left to the third
  part (S347), `t_expr_complex_five_face.cpp` their test. **Part 3 (chunk
  43), done:** matrices of complex numbers, dense as `math::complex<T>` of
  the widest type, a real element x + 0i, or values (S348); their
  arithmetic in the widest type by `cplx.h`'s operators, a real operand as
  itself (S349); `A'` the conjugate transpose, `transpose(A)` the
  transpose, the parts of a matrix element by element and `complex(A, B)`
  (S350); `det`, `inv` and `solve` `cmat.h`'s, exact where exact, `norm` of
  the moduli, `dot` conjugating (S351); `eig` every eigenvalue, a
  Hermitian matrix's real, an exact one's exact where it is a Gaussian
  rational, ordered by real part, then imaginary part (S258, S352); no
  `not_yet` left but `jm` and `km` (S353); the C++ face's `vdot`, complex
  norms, `frobenius_norm` and `is_unitary` (S322, S354); and the generic
  norms of the moduli, so a `vector<value>`'s of complex numbers the
  language's (S355). `t_expr_complex_matrix_face.cpp` checks them, the
  eigenvalues against mpmath.
- [ ] **3f** (step 24) Quaternions (S275 to S277): `quat.h` grown, accurate
  by default, with `_fast` forms for rendering, the inverse, quotients,
  exp, log, powers, Shepperd's method and a shorter-arc slerp (S276); the
  kind, of rank 950, `jm`, `km`, `quat(w, x, y, z)` and its functions,
  rotations among them (S275); their factors kept in order by
  simplification, p / q the quotient p inv(q), and matrices of
  quaternions, Zhang's standard eigenvalues by complex QR (S277).

## Phase 4 — Calculus in expressions

- [ ] **4a** (step 25) The symbolic derivative: rules for each operator (sum,
  product, quotient, chain) and each registered function, followed by
  simplification. Partial derivatives and the gradient. Simplification's
  variables of declared domains, a variable not declared a complex number,
  a quaternion or a matrix declared kept in order (S274, S251).
- [ ] **4b** (step 26) Numeric calculus: definite integrals (the quadrature
  rules of `calculus/integration.hpp`, on typed evaluation) and the derivative
  at a point.
- [ ] **4c** (step 27) Sums, products, sequences and series. Taylor series
  from 4a. Limits, numeric first.
- [ ] **4d** (step 28) Antiderivatives from a table of elementary forms. This
  is later work: symbolic integration in general is large.

## Phase 5 — The compile-time face and the remaining ports

- [ ] **5a** (step 29) `expression.hpp` to C++11, with its kernels taken from
  the C core (N3). This fixes its own sqrt, exp and sin, which today give
  wrong answers: `sqrt(1e300)` is 7.89e269, `exp` is up to 10,800 ulps off,
  and a C++20 program gets two answers to one call.
- [ ] **5b** (step 30) `function.hpp`, `functional.hpp` and `values.hpp` to
  C++11.
- [ ] **5c** (step 31) `calculus/*` to C++11.
- [ ] **5d** (step 32) `reify`: compile-time expression to run-time term, so
  that a compile-time expression can be printed and differentiated at run
  time.
- [ ] **5e** (step 33) The `math.hpp` umbrella. With this and 1c, the ladder's
  known-failures list has no math entries left.
- [ ] **5f** (step 34) The opt-in lazy type (decision 2), from C++11, for linear
  algebra first.
- [ ] **5g** (step 35) Style debt in these headers: 238 template parameter
  declarations with a reserved leading underscore (`typename _Type`), and
  Roman-numeral sections. The current guides require `Type` and decimal
  numbering.

## Phase 6 — Precision

- [ ] **6a** (step 36) Correctly rounded `_c` forms for exp, log, sin, cos,
  tan, atan, pow and the hyperbolic functions, so that one answer becomes
  bit-identity everywhere (G6). The model is CORE-MATH and LLVM libc.
- [ ] **6b** (step 37) The speed of `sqrt_c` in 32-bit builds (chunk 10's
  options), and `binade_of`'s table.
- [ ] **6c** (step 38) The generator's 189 misaligned parameter lists
  (whitespace only).
- [ ] **6d** (step 39) Clang on the 32-bit x87 (S37). MSVC, where `long
  double` is `double`. PowerPC's double-double `long double`.
- [ ] **6e** (step 40) `_fast` variants where measured demand exists: rsqrt and
  normalize, sincos, exp.

## Phase 7 — Comparison with other libraries (decision 5)

- [ ] **7a** (step 41) An ulp sweep against an MPFR oracle, for the `_c` and
  plain forms. Compared against glibc, musl, LLVM libc, MSVC's UCRT and MSVC's
  `/Zc:cmath` mode, and CORE-MATH.
- [ ] **7b** (step 42) A benchmark of small dense linear algebra against Eigen's
  fixed-size types and LAPACK. Constant-evaluation coverage against GCEM and
  C++26's `constexpr` `<cmath>`.

## Phase 8 — Bignum (decision 6)

Each kind registers as a boxed kind (Phase 1d). From C++14, kinds with a fixed
capacity are constant-evaluable; dynamic ones are constant-evaluable from
C++20.

- [x] **P1** (step 43) Fixed-width wide integers built from limbs: 32-bit
  limbs, or 16-bit where no 64-bit integer exists. Done as 1d's second part
  (chunk 23), since 1d's real kind needs them: `c/math/wide.h`, 256 bits.
- [ ] **P2** (step 44) Bigint: schoolbook multiplication and Knuth's division,
  with caller-provided storage and a small-size buffer. GMP is the test
  oracle. Integer overflow promotes to it instead of to `long double` (G2).
- [ ] **P3** (step 45) Big rational: rational overflow promotes to it.
- [ ] **P4** (step 46) Karatsuba and Toom-3 multiplication, and fast base
  conversion.
- [ ] **P5** (step 47) Big float with correct rounding (the MPFR model), and
  constants by binary splitting.
- [ ] **P6** (step 48) Elementary functions at any precision, and ball
  arithmetic.
- [ ] **P7** (step 49) `evaluate(context)` at any precision, and an optional
  GMP/MPFR backend behind a configuration switch.

## Later, not yet decided

- `re_std::mdspan`, then a `std::linalg`-style layer, once a standard library
  ships `<linalg>` to test against. An `as_mdspan()` member on our matrix and
  vector types.
- A run-time expression at C++98 (N1's later item). If arities are ever fixed
  there (N2): 4 arguments, 8 pieces, 4 components.
- LaTeX in and out; pipe syntax (`v | f`) from C++20.
- Intervals as a kind of the value, by S266's rules; algebraic numbers,
  exact roots, after P2 and P3; dual quaternions, for rigid motions and
  skinning, when a renderer asks (S278).
- Element-wise operators and comparison of matrices, spelled apart from the
  algebraic ones (`.*`, `./`, `.^`), slices `A[1:2, :]` and an index from
  the end (S246's, S249's and S253's reviews).

---

## Cross-cutting

- **Tests.** Every new C header gets a harness in the 12-build matrix. Every
  new face gets a face test in the 64-build matrix. Every parser item gets
  round-trip tests.
- **CI and machines.** C4a (runner), C4b (workflow), and C4c (CMake and CTest,
  including MSVC and clang-cl; MSVC has never compiled the math core).
- **Merging with other lanes.** The net lane and the others merge into
  `master`. The ladders' known lists are regenerated after a merge, never
  merged by hand. The matrix and face tests run again after every merge.
  README section 8 lists the files outside `math/` this lane has touched.
- **Documentation.** `README.md` is updated by every chunk that changes what
  it says.

## Next

Step 23 of 51, 3e, is done, in three parts (S324): chunk 40 made its
first, the complex kind of the value and what a complex number is in the
language, with every box's holders atomic (S134); chunks 41 and 42 its
second (S335): each function's principal complex value, exact where it is
a Gaussian rational, S272's option and S203's eight functions, and the
folds in `math::complex<T>` (chunk 41); and the C core's complex `exp2`,
`log2`, `log10`, `log1p` and `expm1`, with references and bounds, the
face's, and their values in the language (chunk 42); and chunk 43 its
third, matrices of complex numbers, every eigenvalue, the C++ face's
`vdot`, complex norms and `is_unitary`, and the generic norms of moduli
(S348 to S356). Step 24, 3f, quaternions, is next (S275 to S277): `quat.h`
grown, accurate by default with `_fast` forms, the kind of rank 950, `jm`,
`km` and `quat(w, x, y, z)`, and matrices of quaternions, in chunks the
step's first will set out, as S324 set out step 23's.
The outstanding decisions of `decisions.md`, its section 1, by group and
urgency, and the suggestions of `comparison.md`'s section 9 are the
owner's to take or leave before then.
