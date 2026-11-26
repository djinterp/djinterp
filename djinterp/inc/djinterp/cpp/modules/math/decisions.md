# djinterp math: the lane's decisions, S1 to S468

Every decision the math lane has taken, in the words of the record that took
it, with the alternative it set aside. The numbers run across the lane's
records from S1 (slice 3, 2026.10.08); each record's section of decisions is
here as its README has it, its own first lines among it. A decision was the
lane's to take within the owner's rulings, and stands once his next word on
its delivery, his "continue", has accepted it: S1 to S462 stand, his own
rulings of 2026.10.27 on complex numbers and quaternions, S265 to S278, the
design of steps 22 to 24, and his choice of 2026.11.07, S383, among them.
His rulings of 2026.11.03 and 2026.11.04 on the flagged decisions are in the
reviews they rule, each "Ruled 2026.11.04", two of them, S336 and S340, his
own choice of an alternative the review had set aside, made in chunk 46
(S384, S385). His rulings of 2026.11.16 on the decisions of 2026.11.15 are
in chunk 51's opening lines and, for S391, S403 and S413, in their reviews,
each "Ruled 2026.11.16", and each done in chunk 53; chunk 51's decisions,
the first of the double family's rewrite as he ruled it, stand by his
"continue for now" of 2026.11.20, and chunk 52's, the second, `log_c`
correctly rounded, by his "continue" of the same day, and chunk 53's, step
29 (5a) with 0.8's items as he ruled them, by his "continue" of 2026.11.21,
and chunk 54's, `log_c` 1.7 to 1.85 times as fast, by his "continue" of the
same day. Chunks 55's to 58's stand by his "continue" of 2026.11.23 at
19:54: chunk 55's, math's largest files split, as he ruled on 2026.11.21,
each old name kept - the eight largest generated headers, the language's
matrix kind, its face test and the generator - and the delivery over his
main of 2026.11.21 with chunk 52 laid over it, without the records; chunk
56's, step 30 (5b), `function.hpp`, `functional.hpp` and `values.hpp` from
C++11, with two of their own failures fixed; chunk 57's, chunks 55's and
56's zips cut again on his push of 2026.11.23, the lane's prose and the
generator's strings with one hyphen for a dash; and chunk 58's, step 31
(5c), the calculus headers from C++11, and with them step 33 (5e),
`math.hpp` at every rung; and chunk 59's, step 32 (5d), `reify`, a
compile-time expression made a term of the run-time language, by his
"continue" of 21:32 the same day. S463 to S468 are chunk 60's, open until
his word on them: step 34 (5f), the opt-in lazy type of linear algebra. His
word of 2026.11.14 on the language's names, which kept them flat in
`djinterp::math`, is S397's. Where a later decision changes an earlier one,
the later one says so; the earlier one stays here as it was taken.

A decision names files as they were when it was taken. Chunk 48 merged the
two quaternion classes (S398) and renamed or split five of the language's
headers (S400, S401): `expr/complex.hpp` is `expr/complex_kind.hpp`,
`expr/quaternion.hpp` `expr/quaternion_kind.hpp`, `expr/series.hpp`
`expr/summation.hpp`; `expr/matrix.hpp` became `expr/matrix_kind.hpp` and
`expr/matrix_function.hpp`, and `expr/function.hpp`
`expr/function_table.hpp`, `expr/arithmetic.hpp` and `expr/logic.hpp`;
`linalg::quaternion`'s `to_matrix` is `to_matrix3` in both names. Chunk 55
split the largest files (S436 to S439): each old name is still a header,
which includes its parts. A record's text is here as the record has it, but
for the merge window's rewordings of two of chunk 51's lines and the
lane-name check's of a line each of chunks 52's and 53's (S440), and the
owner's push of 2026.11.23, which gave every dash one hyphen and reworded a
line of chunk 52's (S447). `expr/expr_common.hpp`'s banner maps the
language's headers as they are.

Each decision is followed by its review, written from chunk 34 on at the
owner's asking, each chunk reviewing its own: what the path taken gains and
what it costs, beside the alternatives, and the path the review recommends,
judged on accuracy and functionality, the best solution rather than the
cheapest. A review ends "Taken" where the path taken is the one it
recommends, or "Ruled" where the owner ruled it; it ends in a flag where the
path taken is not the one it recommends, or has a significant drawback. The
22 flagged decisions, the outstanding ones, come first, in five groups by
what they concern, the most urgent first in each (section 1); then every
decision, by record (section 2).

The questions put to the owner before the S numbers began, M0 to M3, C1 to
C6 and N1 to N6, are in the consolidation's record
(`_handoffs/consolidate_2026.10.07/README.md`, section 5) and the notes that
followed it; his rulings on them are the roadmap's principles and items
(`roadmap.md`). From chunk 34 on, each chunk's delivery brings this file,
its own decisions added at the end. This file is made by
`_handoffs/math98_chunk60_2026.11.23/tools/decisions_c60.py`.

## 1. Outstanding decisions

The 22 flagged decisions: those whose review recommends what is not yet
done, or names a significant drawback of what was. Each gives when it is
due, by the roadmap's steps, its flag and its review's recommendation; the
decision itself and its whole review are in its record's part of section 2.
The groups and the steps are `decisions_outstanding_c60.md`'s, in chunk 60's
record.

### 1.1 The expression language

- S190. When: before step 25 (4a): decided on 2026.11.06 with lane F,
  against expression core v2. Not the recommendation; a growing cost.
  Recommended: the alternative, before Phase 4's rewrites and any further
  operator make the layer larger. Ruled 2026.11.04: decided on 2026.11.06
  with lane F against expression core v2's design: core v2 if its parser
  holds postfix operators and calls, S209 and S252 then following it;
  otherwise the language's own precedence parser, as S190's review
  recommends.
- S209. When: with S190. Follows from S190. Recommended: S190's alternative
  would make it unnecessary. Ruled 2026.11.04: decided on 2026.11.06 with
  lane F against expression core v2's design: core v2 if its parser holds
  postfix operators and calls, S209 and S252 then following it; otherwise
  the language's own precedence parser, as S190's review recommends.
- S252. When: with S190. Follows from S190. Recommended: as taken within
  S190's shared climber, whose own alternative, a precedence parser of the
  language's, would hold postfix operators with operands as it holds calls.
  Ruled 2026.11.04: decided on 2026.11.06 with lane F against expression
  core v2's design: core v2 if its parser holds postfix operators and calls,
  S209 and S252 then following it; otherwise the language's own precedence
  parser, as S190's review recommends.
- S203. When: step 36 (6a): `gamma`, `lgamma`, `erf` and `erfc` with their
  kernels and the correctly rounded forms (S379). Functionality gap, now
  `gamma`, `erf` and the rest alone. Recommended: those five in the table in
  step 23 (S271), and `min`, `max` and `sign` with them (exact, no kernel
  needed), sign(z) being z / |z| of a complex z; `gamma`, `erf` and the rest
  with their kernels; chunk 40 placed the eight in step 23's second part,
  chunk 41 (S324), which put them in the table (S338), each with its complex
  value but the next chunk's two (S335); `gamma`, `erf` and the rest wait
  for their kernels, which no step has yet. Ruled 2026.11.04: `gamma`, `erf`
  and the rest wait for their kernels, the lane to place them: at step 36
  (6a), `gamma`, `lgamma`, `erf` and `erfc`, with the correctly rounded
  forms (S379).
- S220. When: at any step that touches the folds' logic; minor. Not the
  recommendation; a minor point, changeable later, as it makes an error a
  value. Recommended: the alternative, the identities, with `piecewise` kept
  at one argument at least. Ruled 2026.11.04: the alternative, as
  recommended, at any step that touches the folds' logic.
- S272. When: the option on every evaluator since chunk 46 (S385); a
  documented drawback, nothing due. Significant drawback, documented: a
  stray negative goes complex silently unless the option is set.
  Recommended: as ruled, the option given with the kind's functions, in step
  23's second part, chunk 41 (S324), which gave it, `expr_context`'s
  `real_results` (S340). Ruled 2026.11.04: S272 follows S340's ruling, the
  option on each function's evaluator, made in chunk 46 (S385).

### 1.2 Simplification and variables

- S251. When: step 25 (4a), with declared domains (S274), as ruled.
  Significant drawback for variables bound to matrices. Recommended:
  declared domains (S274), declared matrix variables among them, as SymPy's
  `MatrixSymbol` and its non-commutative symbols are: a binding's or the
  syntax's mark that a name is a matrix, of a shape where known, or a
  quaternion, which simplification then treats as a literal is treated now,
  its powers' identities included; in step 25 (4a), or sooner where a user
  simplifies terms of matrix variables. Ruled 2026.11.04: declared domains,
  matrix variables among them, at step 25 (4a); S376's variables beside
  quaternions with them.
- S376. When: step 25 (4a), with S251's declared domains. Significant
  drawback until 4a: a variable bound to a number that does not commute with
  a quaternion's factors gets another value from a simplified term.
  Recommended: as taken, with S251's declared domains at step 25 (4a), a
  variable declared complex or a quaternion then keeping its place.

### 1.3 Precision and one answer

- S15. When: step 36 (6a), where one answer becomes bit-identity; sooner at
  no cost. Not the recommendation; two answers across levels. Recommended:
  the library's functions wherever `<math.h>` declares them, the `_c` forms
  only where it does not. Ruled 2026.11.04 (the owner's, settled): as
  recommended, one answer per platform, at step 36 (6a).
- S424. When: before the plain double `log` follows ruling 12 in early
  December: `log_c` 1.95 to 3.03 times the C library's time after chunk 54
  (S432, S433), past twice. `log_c` is still 1.95 to 3.03 times the C
  library's time, past the twice that ruling 12's recommendation asks before
  the plain form becomes the correctly rounded one. Recommended: as taken,
  then inverses of 8 bits, so that r is exact in one double, before the
  plain double `log` follows ruling 12; an outside review of the proofs when
  one can be had. In chunk 54 (S432, S433) `log_c` became 1.70 to 1.85 times
  as fast, its results the same bits.
- S460. When: with ruling 12 in early December: a reified term's exp, sin,
  cos, tan, log and atan the plain functions, one ulp from the C core's
  correctly rounded kernels at 1 to 56 of 20,000 arguments each. Until
  ruling 12, a reified term's transcendentals can differ from the
  expression's in the last bit, the expression's being the right one.
  Recommended: as taken, the plain functions made the correctly rounded
  kernels as ruling 12 decides.
- S9. When: step 39 (6d), with MSVC. Significant drawback on compilers
  without the builtin. Recommended: as taken, plus `std::bit_cast` from
  C++20 where the builtin is missing, so every C++20 compiler gets it. Taken
  in part. Ruled 2026.11.04: as recommended, at step 39 (6d).
- S37. When: step 39 (6d), where clang on the x87 is taken up; documented
  until then. Significant drawback on clang's x87, documented. Recommended:
  as taken, given that C allows no fix inside a constant expression; keep
  the SSE advice prominent. Taken. Ruled 2026.11.04: as taken, the SSE
  advice kept prominent.
- S39. When: step 39 (6d), with the x87. Not the recommendation.
  Recommended: the alternative, as S38 does for the roots. Ruled 2026.11.04:
  the alternative, as recommended, at step 39 (6d).
- S370. When: step 40 (6e): chunk 42's whole power, in the family's own
  type, a `_fast` form where measured demand asks for one. Significant
  drawback: the plain forms' whole powers cost 2.0 to 3.5 times chunk 44's
  time for double and 3.3 to 6.2 for float, n from 2 to 100. Recommended: as
  taken, every part within its bound before a power is fast, as the
  roadmap's seventh principle has it; chunk 42's squaring in the family's
  own type, the faster, kept for a `_fast` form at step 40 (6e), where
  measured demand asks for one.
- S20. When: at any step that touches `power.h`; minor. Not the
  recommendation (minor). Recommended: the alternative, at its small cost.
  Ruled 2026.11.04: the alternative, as recommended, at any step that
  touches `power.h`.

### 1.4 Linear algebra and eigenvalues

- S392. When: step 40 (6e), or sooner on demand: `math::quaternion<T>` a
  number type, both faces' matrices taking it. Not the earlier review's
  recommendation in full: neither face's matrix holds quaternions; the
  language's does. Recommended: the first alternative, with step 40 (6e) or
  sooner on demand, both faces' matrices at once; as taken until then.
- S306. When: at the next step that touches the Jacobi functions; minor.
  Minor: no scaling before the Jacobi sweeps. Recommended: as taken, with
  the matrix scaled by a power of two first, as the driver is, at the next
  step that touches the Jacobi functions, the bits in range unchanged. Ruled
  2026.11.04: as recommended, at the next step that touches the Jacobi
  functions.
- S311. When: at the next step that touches the balancing; minor. Minor: the
  balancing exact only where no entry passes below the least normal.
  Recommended: the first alternative, the factors applied once at the end,
  at the next step that touches the balancing: exact wherever representable,
  the same bits wherever no entry underflows. Ruled 2026.11.04: the first
  alternative, the factors applied once at the end, as recommended.
- S360. When: at the next step that touches `quat.h`'s kernels; minor.
  Minor: the accurate rotation costs some 900 ns, near two hundred times the
  `_fast` form's. Recommended: as taken, and the rotation and its matrices
  in double's pairs for float and double, as the product's (S358), at the
  next step that touches `quat.h`'s kernels.
- S364. When: at the next step that touches `quat.h`'s kernels; minor.
  Minor: the plain exp's and pow's error grows with |v| and p. Recommended:
  as taken for the `_c` forms, which the faces take; the plain forms
  carrying |v| as a pair, as the product's double pairs do, at the next step
  that touches `quat.h`'s kernels.

### 1.5 Number types and the value's kinds

- S130. When: step 45 (P3), big rationals, as planned. Significant drawback
  until P3. Recommended: as taken as an interim, big rationals in P3, as
  planned. Ruled 2026.11.04: the interim stands; big rationals at P3, step
  45.

## 2. Every decision, by record

### S1 to S3: slice 3, 2026.10.08

The record: `_handoffs/math98_slice3_2026.10.08/README.md`.

New with this slice; the earlier ones (M1 to M3, C1 to C4, N1 to N5) are in
the consolidation record's section 5 and stand.

- **S1. The constants' spelling below C++14.** Taken:
  `constants::of<Type>::name()`, numeric_limits' form, constexpr from C++11.
  The alternatives: gate the named constants at C++14, as re_std's numbers.hpp
  does ("a struct-of-static-members would be a different API"), leaving C++98
  users without them; or one struct per constant, `pi_c<Type>::value`. The
  name `of` is the session's.

> **Review.** Taken: for, one spelling from C++98, constexpr from C++11, in
> the numeric_limits idiom C++ readers know; against, a call where C++20's
> `std::numbers::pi_v<T>` is a variable, so it is not the standard's
> spelling. The alternatives: gating at C++14 leaves C++98 and C++11 without
> constants; one struct per constant cannot hold a floating-point value in
> class before C++11. Recommended: as taken, with `std::numbers`-named
> aliases of the same values from C++14, so that code moving to `<numbers>`
> finds them. Taken.

- **S2. A non-static member counts as absent** in `is_rational`,
  `has_number_base` and `is_using_base`, at every level, as in interval's
  traits; a radix that is not static cannot serve as a base. The
  alternative, counting it, as `decltype` did from C++14, would need a probe
  that takes a pointer to member as present at C++98, where Clang does not
  find the name.

> **Review.** Taken: for, one answer at every level, which a C++98 probe can
> give; against, a type whose radix is a non-static member is read as having
> none. The alternative counts it only where `decltype` can, so the same
> type answers two ways by level. Recommended: as taken; one answer across
> levels is the framework's principle (4). Taken.

- **S3. rational stays at C++14**, where expression.hpp is: N1 decides where
  both go.
- **N3, sharpened.** exp_c and log_c are correctly rounded for float and
  double (but 3.9 double exponentials in a million) and glibc's exp and log
  are not (one in 1,600 and one in 14,000 differ by an ulp). A face that
  takes the library at run time and the `_c` form in constant evaluation, as
  `linalg_elementary` does for acos, sin and cos, gives two answers at those
  points; one that takes the `_c` form everywhere gives one answer and pays
  about 30 times the library's cost.

> **Review.** Taken: a holding decision until N1, which the owner ruled; 1a
> (chunk 16) then wrote the exact rationals in C. Superseded; nothing to
> recommend now.

### S4 to S7: slice 4, 2026.10.09

The record: `_handoffs/math98_slice4_2026.10.09/README.md`.

New with this slice; the earlier ones (M0 to M3, C1 to C4, N1 to N5 in the
consolidation record's section 5; S1 to S3 in slice 3's) stand.

- **S4. The sine past 2^20: exact everywhere.** Taken: Payne and Hanek's
  reduction in pure arithmetic, so sin_c, cos_c and tan_c are right and
  constant expressions at every finite argument, as the library's functions
  are. The alternative was a stated bound - NaN, or a documented loss of
  digits, past 2^20 quarter turns - which would leave `_c` and the library
  disagreeing there and the C++ faces unable to promise one answer (N3).
  The cost is 2,112 bytes of literal and about a microsecond an argument
  past the bound.

> **Review.** Taken: for, every finite argument right, a constant
> expression, and the same bits as a correct library; against, 2 KB of
> digits and about a microsecond past 2^20. The alternative's bound would
> give wrong or NaN results there and break one answer (N3). Recommended: as
> taken. Taken.

- **S5. sin_c and cos_c rounded once.** Taken, so that all four of this
  slice's functions, like exp_c and log_c, are correctly rounded for float
  and double but at a few arguments in a million. It changes, below 2^20,
  234 double sines and 261 cosines in a million (231 and 257 to the
  correctly rounded value, 3 and 4 from it) and some 7.5% of long double
  ones (section 2.4). The alternative keeps every result below 2^20 as it
  was and only adds the far reduction.

> **Review.** Taken: for, correctly rounded but at a few arguments in a
> million; against, not proven, so "nearly" remains. The alternative keeps
> known double roundings. Recommended: as taken, then proven correct
> rounding (step 33, 6a, CORE-MATH's method). Taken.

- **S6. 2/pi as a string literal.** A `static const long` array at file
  scope is what fdlibm has, but GCC emits it, 5.6 KB, into every translation
  unit that includes the header at -O0 (C and C++); from C++11 a constant
  expression can read it only if it is `constexpr`, so it needs a spelling
  per level; and below C++17 an internal-linkage table read by an
  external-linkage inline function is a technical ODR question. An array
  inside the function avoids those, but is 5.6 KB of stack that GCC copies
  on every call. The literal has none of these costs. Its alternative is the
  array, with a math_common.h qualifier for its spelling.

> **Review.** Taken: for, no per-unit copy, no stack copy, one text for C99
> to C++23; against, digits read from a string are slower to reach than an
> array's and an unusual idiom. The alternative's array costs 5.6 KB a unit
> at -O0 and a spelling per level. Recommended: as taken while one text
> serves every level; an `inline constexpr` array from C++17 would be faster
> at run time without changing a bit. Taken.

- **S7. tangent.h, and the shared kernels in elementary.h.** tan and atan
  have a header of their own, as exp and log do; the exact arithmetic and
  `round_pair` moved down into `elementary.h`, which the other two include,
  rather than into a new header beneath all three.
- **N3, sharpened again.** sin_c, cos_c, tan_c and atan_c are now correctly
  rounded for float and nearly so for double; glibc's double sin, cos and
  tan differ from them by an ulp at about 1 argument in 650, 780 and 400,
  atan at 1 in 4,300 - and where they differ the `_c` result is the
  correctly rounded one, but at the few counted above. `linalg_elementary`'s
  rule - the library at run time, `_c` in constant evaluation - gives two
  answers at those points.

> **Review.** Taken: for, no new header; against, `elementary.h` grows into
> the kernels' catch-all. The alternative's header beneath all three is the
> cleaner layering in the long run. Recommended: either; a shared kernel
> header is slightly better once more families need the exact arithmetic.
> Taken, a minor point.

### S8 to S10: slice 5, 2026.10.09

The record: `_handoffs/math98_slice5_2026.10.09/README.md`.

New with this slice; the earlier ones (M0 to M3, C1 to C4, N1 to N5 in the
consolidation record's section 5; S1 to S3 in slice 3's; S4 to S7 in slice
4's) stand.

- **S8. acos_c onto the arc-tangent kernel.** Taken, so that asin_c and
  acos_c are one computation, each rounded once, and acos_c is correctly
  rounded for float and double like the slice's other functions. It changes
  267 double results in a million on the x87 - every one to the correctly
  rounded value - and 23.8% of long double ones (237,788 to it, 88 from
  it); where long double is double, a quarter of double ones (section 4).
  The alternative keeps the old acos_c, within 2 ulps, beside an asin_c on
  the kernel, the two then computed two ways and rounded differently.

> **Review.** Taken: for, asin and acos one computation, correctly rounded;
> against, three times the old cost (N6). The alternative keeps two methods
> rounding two ways. Recommended: as taken. Taken.

- **S9. A zero's sign by a compiler builtin.** atan2 is the one function
  here whose result depends on the sign of a zero, which no comparison sees.
  Taken: `__builtin_copysign` (and `f`, `l`) on GCC and Clang - a constant
  expression in both, where `__builtin_signbit` is not in Clang 18 - and
  1 / x elsewhere, which is right at run time but not a constant expression,
  so that atan2_c of a zero is not one there. The alternatives: `bit_cast`
  from C++20, which the C text cannot use; or treating -0 as +0 in constant
  evaluation, which would make the constant and the run time disagree.

> **Review.** Taken: for, a constant expression on GCC and Clang; against,
> elsewhere (MSVC, which has never compiled the core) atan2_c of a zero is
> no constant expression. The alternatives: `std::bit_cast` from C++20; or
> -0 as +0, a second answer. Recommended: as taken, plus `std::bit_cast`
> from C++20 where the builtin is missing, so every C++20 compiler gets it.
> Taken in part. Ruled 2026.11.04: as recommended, at step 39 (6d).
> **Flag:** significant drawback on compilers without the builtin.

- **S10. Where the kernel lives.** In `elementary.h`, beside the sine's,
  because asin and acos are there and tangent.h includes it; asin_c beside
  acos_c; atan2_c beside atan_c in `tangent.h`. The alternative, a header of
  the arc functions beneath both, would move acos out of `elementary.h`.
- **N3, sharpened again.** atan2_c, asin_c and acos_c are correctly rounded
  for float and nearly so for double; glibc's atan2f differs from them at
  one float pair in six, asinf and acosf at about one float in 400 in
  [-1, 1], and the double atan2, asin and acos at 380, 740 and 380
  arguments in a million - where the `_c` result is the correctly rounded
  one. `linalg_elementary`'s rule gives two answers at those points.
- **N6. The arc-sine's cost.** asin_c and acos_c take about 1.1 to 1.2 us,
  three times the old acos_c; a third of it is the long double square root,
  `sqrt_c` in pure arithmetic (370 ns). Where it is not constant-evaluated, a
  call could take the library's `sqrtl` instead - correctly rounded too, so
  with the same bits - through `__builtin_is_constant_evaluated`, which GCC
  and Clang (9 and up) offer in C++ at every level, or
  `std::is_constant_evaluated` from C++20; a C build, which evaluates
  nothing at compile time, would always call it. The same would serve every
  `_c` function that takes a root. Not done: it would be the first `_c` text
  that runs differently at run time and in constant evaluation, which is the
  owner's call.

> **Review.** Taken: placement only; sound. Recommended: as taken. Taken.

### S11 and S12: slice 6, 2026.10.10

The record: `_handoffs/math98_slice6_2026.10.10/README.md`.

New with this slice; the earlier ones (M0 to M3, C1 to C4, N1 to N6, S1 to
S10, in the consolidation record and slices 3 to 5) stand.

- **S11. A header of their own.** `hyperbolic.h`, on exponential.h's kernel
  and elementary.h's exact arithmetic, as tangent.h stands on elementary.h's.
  The alternative was exponential.h's own sections, which would make every
  user of exp and log compile the hyperbolic functions too.

> **Review.** Taken: for, users of exp and log do not compile the hyperbolic
> functions; against, one header more. Recommended: as taken. Taken.

- **S12. The series below 1/2, the exponential from it.** At 1/2 the series'
  terms past the exact ones are under 2^-10 of the sum, and the difference
  E - 1/E loses under a bit; a lower crossing would make the difference
  lose more, a higher one the series' plain terms count for more. Below it,
  no double result was seen not correctly rounded; from it the error is the
  exponential's, as exp_c's is.
- **N3, again.** sinh_c, cosh_c and tanh_c are correctly rounded for float
  and nearly so for double; glibc's differ from them at a quarter of the
  floats and doubles from 1/2 to 2, where the `_c` result is the correctly
  rounded one. `linalg_elementary`'s rule gives two answers at those points.
- **N6, again.** sinh_c and cosh_c cost about 500 ns, tanh_c up to a
  microsecond, where glibc's take 7 to 17 ns. The series divides at each
  nested term, which a table of reciprocals would avoid. Slice 5's way out
  - the library at run time, where a call is not constant-evaluated - is
  no way out here: it keeps the bits only where the library's are correctly
  rounded, as sqrtl is and glibc's hyperbolic functions are not.

> **Review.** Taken: a measured crossing; for, no double result seen
> misrounded below it; against, measured, not proven. Recommended: as taken,
> with a worst-case search when 6a proves the family. Taken.

### S13 to S16: slice 7, 2026.10.10

The record: `_handoffs/math98_slice7_2026.10.10/README.md`.

New with this slice; the earlier ones (M0 to M3, C1 to C4, N1 to N6, S1 to
S12, in the consolidation record and slices 3 to 6) stand.

- **S13. exp2, log2 and log10 in `exponential.h`**, on its kernels split so
  that five functions share them. They are the exponential and the
  logarithm in other bases, as C's `<math.h>` groups them, and add a few
  dozen lines a family. The alternative, a header of their own as
  `hyperbolic.h` is, would copy the series or reach into exponential.h's
  internals all the same.

> **Review.** Taken: for, C's grouping and shared kernels; against, a larger
> header. Recommended: as taken. Taken.

- **S14. cbrt in a new `power.h`**, where `pow` and `hypot` - which
  geometry and the calculus headers call - can go when they come. The
  alternative is `elementary.h`, beside `sqrt_c`, which every linear-algebra
  face includes.

> **Review.** Taken: for, pow and hypot have a home off the linear algebra
> faces' include path; against, one header more. Recommended: as taken.
> Taken.

- **S15. Below C++11, the plain `exp2`, `log2` and `cbrt` are the `_c`
  forms.** C++98 has no `std::exp2`, `std::log2` or `std::cbrt`, and
  `::exp2f` and its kin are C99's, which a C++98 library need not declare.
  So the plain forms give the correctly rounded (or nearly) bits there, at
  the `_c` forms' cost.

> **Review.** Taken: for, exp2, log2 and cbrt exist at C++98, correctly
> rounded; against, at C++98 the plain forms are the `_c` forms, 30 to 100
> times slower, and give other bits than C++11's plain forms, the library's,
> where the library is not correctly rounded: two answers across levels,
> against principle 4. The alternative (calling C99's functions, which every
> real C++98 platform's `<math.h>` declares) keeps one answer per platform
> and the library's speed. Recommended: the library's functions wherever
> `<math.h>` declares them, the `_c` forms only where it does not. Ruled
> 2026.11.04 (the owner's, settled): as recommended, one answer per
> platform, at step 36 (6a). **Flag:** not the recommendation; two answers
> across levels.

- **S16. cbrt_c correctly rounded**, by the exact settle, as `sqrt_c` is.
  The settle costs a few products where its approximation decides, and the
  exact sum is reached only for a root within 2^(4 - P) ulp of a midpoint,
  2^-60 on an x87. The alternative, Newton's root rounded to the family, is
  faithful but not correctly rounded, and a constant would then differ from
  a correctly rounded library's at some arguments.
- **N3, again.** glibc's `cbrt` differs from `cbrt_c` at about half of all
  doubles, `log10f` and `log10` at a third of those from 1/2 to 2:
  `linalg_elementary`'s rule gives two answers there. Its `exp2` and `log2`
  differ at fewer than one double in a thousand.
- **N6, again.** exp2_c about 450 ns, log2_c and log10_c about 320,
  cbrt_c 620, where glibc's take 3 to 13 ns. The library at run time keeps
  the `_c` bits only where the library's are correctly rounded: glibc's exp2
  and log2 are at all but about one double in a thousand, its log10 at seven
  in ten from 1/2 to 2, its cbrt at fewer than half of all doubles.

> **Review.** Taken: for, correctly rounded by an exact settle, with no
> worst-case search; against, a few products more. Recommended: as taken.
> Taken.

### S17 to S20: slice 8, 2026.10.10

The record: `_handoffs/math98_slice8_2026.10.10/README.md`.

New with this slice; the earlier ones (M0 to M3, C1 to C4, N1 to N6, S1 to
S16, in the consolidation record and slices 3 to 7) stand.

- **S17. pow_c as exp(y log x) carried past long double, the exact powers
  exactly.** Correctly rounded wherever measured, not proven: a proof needs
  the worst cases of x^y, which for double are not known in full, and an
  exact settle like `cbrt_c`'s has no cheap form for a transcendental
  power. The exact routes cover every result that can be exact or on a
  midpoint, so that no tie is left to the approximation. The alternative,
  exp_c(y log_c x) on the existing kernels, misrounds about 5 doubles in a
  million and 1.2% of x87 long doubles (measured on the prototype).

> **Review.** Taken: for, correctly rounded wherever measured, exact cases
> exact; against, not proven, and 690 ns where glibc takes 15. The
> alternative misrounds 5 doubles in a million. Recommended: as taken, then
> a proof by CORE-MATH's method in 6a. Taken.

- **S18. The shared kernels made faster without a bit changed** (section
  2): an improvement every `_c` function shares. The alternative, a pow of
  its own Dekker product, would leave the rest 2 to 3 times slower than
  they need be.

> **Review.** Taken: for, every `_c` function faster, no bit changed;
> against, none. Recommended: as taken. Taken.

- **S19. exp_scale breaks a tie in long double's subnormals by y_lo**
  (section 3), which changes `exp_c` and `exp2_c` there: toward the
  correctly rounded value at 99.8% of the changes, away at 0.2%, where the
  fast kernel's error passes the true y_lo. The alternative, a scaling of
  pow's own, would leave exp_c's double rounding in place and add a second
  scaling to keep in step.

> **Review.** Taken: for, 99.8% of the changes toward correct rounding;
> against, 0.2% away, in long double's subnormals. The alternative keeps a
> known double rounding. Recommended: as taken until 6a makes the family
> correctly rounded there. Taken.

- **S20. power.h includes `exponential.h` and `tangent.h`**, the latter for
  `d_internal_math_*_negative`: pow(-0, -3) is -inf, and no comparison sees
  a zero's sign in a constant expression. The alternative is to move
  `negative` into `float_order.h`, beside the other order helpers, at the
  cost of touching a third header.
- **N3, again.** glibc's `powf`, `pow` and `powl` differ from `pow_c` at
  0.2%, 0.07% and 8% of the pairs measured; `linalg_elementary`'s rule gives
  two answers there.
- **N6, again.** pow_c about 690 ns a double, where glibc's pow takes 15: the
  `_c` forms are for constant evaluation and as a reference.

> **Review.** Taken: for, no third header touched; against, `power.h`
> includes `tangent.h` for a sign helper that belongs with the order
> helpers. The alternative, the helper in `float_order.h`, is the better
> layering. Recommended: the alternative, at its small cost. Ruled
> 2026.11.04: the alternative, as recommended, at any step that touches
> `power.h`. **Flag:** not the recommendation (minor).

### S21 to S24: slice 9, 2026.10.10

The record: `_handoffs/math98_slice9_2026.10.10/README.md`.

New with this slice; the earlier ones (M0 to M3, C1 to C4, N1 to N6, S1 to
S20, in the consolidation record and slices 3 to 8) stand.

- **C5. The merge takes master's four later rulings** (section 2.2). They
  are the owner's, made after the register lane's fifth batch, on entries
  that lane left open; the lane's own versions are what they were applied
  to. The alternative is the lane's versions, which would undo the owner.
- **S21. hypot_c correctly rounded by an exact settle**, as `sqrt_c` and
  `cbrt_c` are: no worst-case search needed, the sign of an exact sum
  decides every bit. The alternative, a pair to some 2P bits rounded once,
  needs the root kept from a midpoint by more than the pair's error; for
  hypot that distance is bounded below only by some 2^-(3P) of a last
  place, past what a long double pair holds for double.

> **Review.** Taken: for, correctly rounded with no worst-case search;
> against, cost. Recommended: as taken. Taken.

- **S22. A _c function returns its NaN argument**, never x + y: Clang's
  constant evaluation takes no arithmetic that yields a NaN. `atan2_c`
  already did; `pow_c` now does, and `hypot_c` and `fmod_c` do.

> **Review.** Taken: for, Clang's constant evaluation, and IEEE 754's advice
> to return an input NaN; against, none. Recommended: as taken. Taken.

- **S23. fmod in a header of its own, `remainder.h`**, for C's remainder
  functions, rather than in `power.h` or `float_order.h`: neither is about
  remainders, and `remainder` and `remquo` can join it. It needs no long
  double kernel.

> **Review.** Taken: placement after C's grouping; sound. Recommended: as
> taken. Taken.

- **S24. hypot_c's candidate by Newton's iteration**, not `sqrt_c`: the
  settle makes the candidate's last bit irrelevant, and `sqrt_c` was half
  the cost.
- **N3, again.** Through `calculus.hpp` and `series.hpp` the merge makes
  the C++20 run-time dispatch of `expression.hpp`'s and
  `calculus/elementary.hpp`'s kernels independent of include order (section
  2.3). glibc's `hypot` differs from `hypot_c` at 0.28% of the pairs, `fmod`
  nowhere: under `linalg_elementary`'s rule hypot gives two answers there,
  fmod one.

> **Review.** Taken: for, half the cost, the settle making the candidate's
> last bit moot; against, none. Recommended: as taken. Taken.

### S25: chunk 2, 2026.10.11

The record: `_handoffs/math98_chunk2_2026.10.11/README.md`.

New with this chunk; the earlier ones (M0 to M3, C1 to C5, N1 to N6, S1 to
S24) stand.

- **S25. A NaN is in no order: `advance` keeps it, `steps` saturates.**
  `advance(NaN, n)` is that NaN, as `next_up(NaN)` and C's `nextafter`
  are. `steps` with a NaN is `D_MATH_UMAX_MAX`, the count that says there
  is none to give: `advance(a, steps(a, b)) == b` still holds wherever
  `steps` does not saturate, and a test `steps(a, b) <= k` never takes a
  NaN for a neighbour. The alternative, 0, would make a NaN every value's
  neighbour.

> **Review.** Taken: for, NaN in no order, as `nextafter` treats it, and a
> saturating count that no test mistakes for a neighbour; against, none.
> Recommended: as taken. Taken.

### S26 and S27: chunk 3, 2026.10.11

The record: `_handoffs/math98_chunk3_2026.10.11/README.md`.

New with this chunk; the earlier ones (M0 to M3, C1 to C5, N1 to N6, S1 to
S25) stand.

- **N5, ruled: the rewrite fixes the range.** The owner, asked with the
  consolidation's recommendation named, said to continue; this chunk reads
  that as taking the recommendation. The C core's four are right
  everywhere; whether the C++ port keeps any wrap is now moot, since it
  will call these. Say so if the reading is wrong.
- **S26. floor, ceil, trunc and round in a header of their own,
  `nearest_integer.h`**, after C's nearest integer functions (7.12.9), as
  `remainder.h` is after its remainder functions (S23). Not `remainder.h`,
  not `float_order.h`: neither is about integers. `rint`, `nearbyint` and
  `lrint` belong to the same group but read the rounding mode, which pure
  arithmetic cannot; they are left out.

> **Review.** Taken: for, exact floor, ceil, trunc and round in pure
> arithmetic; against, the C core has no rint, nearbyint or lrint.
> Recommended: as taken, and those three from the library at run time,
> round-to-nearest-even in constant evaluation (which assumes the default
> mode), documented as such. Taken in part: the gap remains.

- **S27. The integer part by conversion to `d_math_umax`**, a part at a
  time past 2^W. The alternatives: adding and subtracting 2^(P-1), which
  rounds to an integer only in the default rounding mode and, on the x87,
  only where excess precision does not keep the fraction; or building the
  integer a bit at a time, up to 112 steps for a quad. The conversion
  truncates exactly in any mode, costs a few ns, and is a constant
  expression.

> **Review.** Taken: for, exact in every rounding mode, a constant
> expression, a few ns; against, none. Recommended: as taken. Taken.

### S28: chunk 4, 2026.10.11

The record: `_handoffs/math98_chunk4_2026.10.11/README.md`.

New with this chunk; the earlier ones (M0 to M3, C1 to C5, N1 to N6, S1 to
S27) stand.

- **S28. A zero remainder has x's sign, as IEEE 754 says**, where glibc's
  double `remainder` sometimes gives the other. `remainder_c` follows the
  standard, and every other implementation measured here; the harness
  counts glibc's departures instead of failing them, and fails
  `remainder_c` if its zero has the wrong sign. The alternative, copying
  glibc, would make the C core's answer depend on the library it was
  measured against.

> **Review.** Taken: for, IEEE 754's sign, independent of the library
> measured; against, differs from glibc's double remainder at some points.
> Recommended: as taken. Taken.

### S29 to S31: chunk 5, 2026.10.11

The record: `_handoffs/math98_chunk5_2026.10.11/README.md`.

New with this chunk; the earlier ones (M0 to M3, C1 to C5, N1 to N6, S1 to
S28) stand.

- **S29. A constant-expression kernel may answer twice, through a
  pointer.** `remquo_c` is `D_MATH_FN` and keeps C's signature, `(x, y,
  int*)`; `math_common.h` says so. A constant expression reads its quotient
  through an int of its own, as with C++23's `std::remquo`. The
  alternative was a struct returned by value, as C's `div` returns `div_t`:
  no rule to change, but a signature C has not, for `remquo` and later for
  `frexp` and `modf`.

> **Review.** Taken: for, C's signature kept; against, constant evaluation
> needs a local int. Recommended: as taken. Taken.

- **S30. The quotient is |n| mod 8, 0 to 7, with x / y's sign; 0 where the
  result is NaN or y is infinite.** C promises three bits and lets the
  library keep more. glibc on x86-64 gives 8 or -8 for some quotients
  whose last three bits are 0 - where its count carries to 8 - and on
  32-bit x86 never, so only the last three bits agree anywhere, and those
  are what `remquo_c` gives. 0 to 7 indexes a table of eight.

> **Review.** Taken: for, the three bits C promises, the same on every
> platform; against, fewer bits than some libraries give. Recommended: as
> taken. Taken.

- **S31. A null `quo` stores nothing**, in both forms, the house's check of
  a parameter first. C's `remquo` has no such case; the library form hands
  it an int of its own.

> **Review.** Taken: for, a null pointer is safe; against, none.
> Recommended: as taken. Taken.

### S32 and S33: chunk 6, 2026.10.11

The record: `_handoffs/math98_chunk6_2026.10.11/README.md`.

New with this chunk; the earlier ones (M0 to M3, C1 to C5, N1 to N6, S1 to
S31) stand.

- **S32. The harnesses' random numbers come from one generator in 32-bit
  halves, and are the numbers they were.** The alternative, a 32-bit
  generator as `t_nearest_integer.c` has, would have changed every
  argument the six harnesses check, and every count their records give.

> **Review.** Taken: for, every earlier count stays comparable; against,
> none. Recommended: as taken. Taken.

- **S33. The builds with a 32-bit `d_math_umax` run on SSE**, so that they
  test the width and nothing else, as the matrix's other 32-bit builds do;
  the x87 keeps gcc's build. The x87 with a 32-bit `d_math_umax` is the
  configuration `sqrt_c` fails in, and goes in the matrix with its mend.

> **Review.** Taken: for, each build tests one thing; against, none.
> Recommended: as taken. Taken.

### S34 and S35: chunk 7, 2026.10.11

The record: `_handoffs/math98_chunk7_2026.10.11/README.md`.

New with this chunk; the earlier ones (M0 to M3, C1 to C5, N1 to N6, S1 to
S33) stand.

- **S34. Where two `d_math_umax` limbs cannot hold (2^(P+1))^2 and P is
  below 64 - a double where `d_math_umax` has 32 bits - `sqrt_c` settles
  its last bit in integers too, in digits of 16 bits; Dekker's product is
  left to long doubles of 64 bits or more.** Integers give the same bits
  under every evaluation method and in constant evaluation. The other
  ways: Dekker's product with each step stored through a volatile (no
  constant expression, and a step a compiler keeps in a register is still
  wide); setting the x87's precision (no portable way in C99 or C++98,
  none in constant evaluation); limbs of half `d_math_umax` at every width
  (the digits serve only where it has 32 bits, whose half is 16).

> **Review.** Taken: for, the same bits under every evaluation method and in
> constant evaluation; against, a slower path on 32-bit-word targets.
> Recommended: as taken. Taken.

- **S35. g++'s ISO-strict C++98 `-m32` build on the x87 joins the matrix,
  for every harness**: the configuration `sqrt_c` failed in. clang++'s
  stays out, as section 4 says.

> **Review.** Taken: for, the configuration that failed is tested; against,
> clang's x87 build stays untested. Recommended: as taken, with clang's x87
> in 6d. Taken.

### S36 and S37: chunk 8, 2026.10.12

The record: `_handoffs/math98_chunk8_2026.10.12/README.md`.

New with this chunk; the earlier ones (M0 to M3, C1 to C5, N1 to N6, S1 to
S35) stand.

- **S36. The harnesses compare values as stored.** A value a harness
  computes in float or double arithmetic and then compares, or hands to a
  kernel and to the library both, is stored through a volatile first, so
  that every build compares values of their type. GCC rounds at the
  assignment, as C says, and the store changes nothing there, nor on any
  build that evaluates in the type (the fingerprints); it is what makes an
  x87 build's verdict the library's and not the register allocator's.

> **Review.** Taken: for, verdicts that are the library's, not the register
> allocator's; against, none. Recommended: as taken. Taken.

- **S37. clang on the x87 is left as it is.** It keeps the x87's 64 bits
  through assignments, casts and arguments, which C says remove them, and
  the library's float and double kernels then give values that are not
  float or double arithmetic's: `t_vec`, `t_quat` and `t_interval_float`
  record them. The library cannot round a value to its type inside a
  constant expression - a volatile read is not one - and rounding every
  step of every kernel through memory would cost every target. clang's
  x87 builds stay out of the matrix (S35); with clang, a 32-bit x86 build
  should use SSE (`-msse2 -mfpmath=sse`), as the matrix's do.

> **Review.** Taken: for, no cost on any other target; against, on clang's
> x87 the float and double kernels give values of no float or double
> arithmetic, so results differ by compiler there. Recommended: as taken,
> given that C allows no fix inside a constant expression; keep the SSE
> advice prominent. Taken. Ruled 2026.11.04: as taken, the SSE advice kept
> prominent. **Flag:** significant drawback on clang's x87, documented.

### S38 and S39: chunk 9, 2026.10.12

The record: `_handoffs/math98_chunk9_2026.10.12/README.md`.

New with this chunk; the earlier ones (M0 to M3, C1 to C5, N1 to N6, S1 to
S37) stand.

- **S38. The plain roots store the C library's root only where their
  family is evaluated wider than its type** (FLT_EVAL_METHOD, through
  `math_common.h`'s `D_MATH_*_EVAL_WIDE`). Everywhere else the root is
  returned as it comes, and costs what it did. Storing always would cost a
  store and a load on every target, for builds - every one on SSE or
  x86-64 - whose roots were right: GCC and clang compute them there with
  `sqrtss` and `sqrtsd`.

> **Review.** Taken: for, the store only where wide evaluation makes it
> needed; against, none. Recommended: as taken. Taken.

- **S39. Only the roots.** The other plain functions return the C library's
  result as it comes, glibc's i386 `asin` and its 64 bits included: their
  word is "the C library's", not "`_c`'s, bit for bit", and no kernel
  computes further with them.

> **Review.** Taken: for, no cost; against, on x87 targets the plain
> functions can return values wider than their type, which then vary with
> register allocation. The alternative, storing every plain result where
> evaluation is wide, costs a store and a load on the x87 alone and makes
> every plain result a value of its type. Chunk 37's plain complex functions
> show it on the x87, within their bounds (S288). Recommended: the
> alternative, as S38 does for the roots. Ruled 2026.11.04: the alternative,
> as recommended, at step 39 (6d). **Flag:** not the recommendation.

### S40: chunk 10, 2026.10.12

The record: `_handoffs/math98_chunk10_2026.10.12/README.md`.

New with this chunk; the earlier ones (M0 to M3, C1 to C5, N1 to N6, S1 to
S39) stand.

- **S40. `sqrt_settle_digits` decides the last bit by the residual
  G = S M - R^2 + R - 1, taken in the low digits - P + 9 bits or more --
  modulo a power of 2.** Its sign survives there because the loops keep R
  within four steps of the root, and `t_elementary` checks the settle over
  all of that reach. The other way, the two midpoints' squares at full width
  and moved by 8R a step, saved a fifth to two fifths as much.

> **Review.** Taken: for, a cheaper settle, checked over its whole reach;
> against, a subtler proof. Recommended: as taken. Taken.

### S41 and S42: chunk 11, 2026.10.13

The record: `_handoffs/math98_chunk11_2026.10.13/README.md`.

**S41. The arc-tangent and the remainder are this side's** - `tangent.h`'s
`atan2` and `atan2_c`, `remainder.h`'s `fmod` and `fmod_c` - not the
package's section of `elementary.h`. They are the later and the more measured
(slices 5 and 9; 51,859,520 and 15,300,041 checks a build, the 184-build
matrix). The package's harness passes on them in all 12 builds and its
oracles print the same, byte for byte, at three levels: the faces take
trigonometry from the C library at run time at every level, and a
remainder's `_c` and plain forms agree. Alternative: the package's, which
would move every caller slices 5 and 9 made, and their checks, onto
functions measured less.

> **Review.** Taken: for, the more measured functions kept; against, none.
> Recommended: as taken. Taken.

**S42. `infinity` stays in `exponential.h`**, where this side defines it, and
`isect.h` includes `exponential.h` for it; the package's `abs` helper goes
into `elementary.h`'s internal helpers, where the package had it.
Alternative: `infinity` moved into `elementary.h` beside `abs` - the base
header, a tidier home - at the cost of changing a second measured header and
the include comments of `hyperbolic.h` and `power.h`.

> **Review.** Taken: for, no measured header touched; against, `infinity`
> lives in an unexpected header. Recommended: either; the alternative is
> tidier. Taken, a minor point.

### S43 and S44: chunk 12, 2026.10.13

The record: `_handoffs/math98_chunk12_2026.10.13/README.md`.

**S43. The module's README and roadmap live in `inc/djinterp/math/`**, as
`README.md` and `roadmap.md`, the way `inc/djinterp/ui/` keeps its own.
Alternative: keep them in the lane's records and the project only, where a
reader of the tree would not find them.

> **Review.** Taken: for, documents beside the code they describe; against,
> none. Recommended: as taken. Taken.

**S44. `master` fast-forwards to this chunk's tip, and the lane keeps
committing on `consolidated`.** This is the owner's C1. `master` is where
every lane, this one included, merges back. Keeping the lane on its own branch
keeps each bundle's base known while other lanes merge into `master`.
Alternatives:

- commit on `master` directly from now on. That is simpler while this lane is
  the only one landing, but a bundle could then meet another lane's merge;
- leave `master` where it is until the lanes reunite.

The roadmap's gates G1 to G9 are proposals, not decisions taken. Each states
its recommendation and alternatives in `roadmap.md`. The lane takes each
recommendation when its phase begins, unless the owner says otherwise.

> **Review.** Taken: superseded by the owner's ruling of 2026.10.18 (no
> commits, plain files). Nothing to recommend now.

### S45 to S49: chunk 13, 2026.10.13

The record: `_handoffs/math98_chunk13_2026.10.13/README.md`.

**S45. Single-shift QR on the Hessenberg form, Wilkinson's shift.** Real
shifts only, one rotation per bulge step. The result type holds real
eigenvalues, and a complex pair is flagged rather than found.
- Alternative: Francis's double-shift QR (LAPACK's `dlahqr`). It converges
  on complex pairs as fast as on real ones, and could give them once complex
  numbers are a value kind (the roadmap's Later). Switching then changes
  only the kernel.

> **Review.** Taken: for, one simple kernel, every real eigenvalue found;
> against, a complex pair was flagged, not found, so a real matrix with
> complex eigenvalues had no full answer, and the language's `eig` (chunk
> 36, S258) gives it as a domain error. The alternative, Francis's
> double-shift QR (LAPACK's), finds complex pairs as fast as real ones and
> is the standard method. Recommended: the alternative, each pair given as a
> complex number of the C core's (S267), as the owner ruled on 2026.10.27.
> Taken in chunk 38: Francis's double-shift QR after balancing finds every
> pair, each a complex number of the C core's (S292), the old function on
> its path (S296).

**S46. `_tol` defaults to 0, meaning the type's epsilon, and the epsilon is
also its floor.** A caller can loosen the deflation test, not tighten it past
what the type resolves. The old default, 1e-12, was below a `float`'s
resolution and about 4,500 epsilons for a `double`.
- Alternative: keep 1e-12 as the default.

> **Review.** Taken: for, a deflation test the type can resolve, epsilon as
> the floor, as LAPACK's is; against, none. Recommended: as taken. Taken.

**S47. `converged()` is false in three cases, and each still gives an answer
for every entry:**
- a complex pair gives its real part twice, and the rest of the spectrum is
  still found;
- a spent budget gives the diagonal of what is left unsolved;
- a NaN or an infinity anywhere gives NaN for every eigenvalue. This holds
  even where the eigenvalues are finite, as for an infinity above the
  diagonal of a triangular matrix, which the old code solved. numpy's
  `eigvals` raises an error on such input.

Alternatives: stop at the first complex pair; give NaN for a complex pair's
two entries.

> **Review.** Taken: for, every entry answered, NaN where any input is not
> finite; against, a complex pair's real part given twice is a wrong value,
> flagged only by `converged()`. Recommended: S45's alternative, which
> removes the first case. Taken in chunk 38: the driver gives each pair
> (S292); the old function still gives a pair's real part twice, flagged,
> for its callers (S296).

**S48. Both kernels scale the matrix by a power of two, exactly.** 2^k A has
2^k times A's eigenvalues bit for bit (barring overflow and underflow), and
the iteration works the same at the range's ends: section D converges from
1e-300 to 1e300.
- Alternative: divide by the largest magnitude. That is inexact, so the
  2^k property is lost.
- Alternative: no scaling. That fails at the range's ends, as the old code
  did at 1e300.

> **Review.** Taken: for, exact scaling, the 2^k property kept, the range's
> ends handled; against, scaling as a whole, not balancing. Recommended: as
> taken, plus LAPACK's balancing (`dgebal`: a diagonal similarity by powers
> of two), which also keeps bits exact and improves accuracy on badly scaled
> matrices. Taken in part.

**S49. Eigenvectors by inverse iteration, from the eigenvalues.** The shift is
4 epsilon of the eigenvalue's magnitude, or of the matrix's scale if larger,
moved 16 times further while the shifted matrix is exactly singular. U x = e
is solved first, then two full solves. The vector has unit length with its
largest entry positive, and the column stays the identity's where there is
nothing to find. Costs O(n^3) per vector.
- Alternative: accumulate the QR steps' rotations and back-substitute in the
  Schur form (LAPACK's `dtrevc`). That gives independent vectors for a
  repeated eigenvalue (section 6) and could handle complex pairs, at the cost
  of a second kernel.

> **Review.** Taken: for, one kernel, simple; against, dependent vectors for
> a repeated eigenvalue and no complex vectors. The alternative, Schur
> vectors and back-substitution (`dtrevc`), gives independent vectors and
> handles complex pairs. Recommended: the alternative, with S45's. Taken in
> chunk 38: the Schur vectors by back-substitution, complex for a pair
> (S292), the C++ face's general eigen among their callers (S296).

### S50 to S52: chunk 14, 2026.10.14

The record: `_handoffs/math98_chunk14_2026.10.14/README.md`.

**S50. The runner lives in `ci/`, in the ladders' style:** the same banner,
`JOBS`, `--save`, and exit statuses 0, 1 and 2. It is the third of the
tree's checks, next to the two ladders.
- Alternative: `tools/`, beside the generator.
- Alternative: a Makefile or CMake target. That is C4c's job, with MSVC.

> **Review.** Taken: for, the tree's checks side by side, one style;
> against, none. Recommended: as taken. Taken.

**S51. The tests are found by their names on each run, not listed.** A
harness's C++14 and C++20 builds follow from its C++14 section. A new test
therefore joins both tiers with no edit to the runner; section 4's four new
failing tests were found that way.
- Alternative: a list in the runner, as chunk 7's had. It went stale (13 of
  17 harnesses, 7 of 26 face tests).

> **Review.** Taken: for, no list to go stale; against, a test named against
> the pattern is missed silently. Recommended: as taken. Taken.

**S52. The full tier stops before it starts when a compiler or the 32-bit
libraries are missing**, and says what to install. It never skips a build,
as the C++ ladder never skips a configuration, so a full run that passes
always means all 1,896 builds passed.
- Alternative: skip the builds that cannot run and report them. A pass
  would then depend on the machine.

> **Review.** Taken: for, a pass means every build passed; against, none.
> Recommended: as taken. Taken.

### S53 and S54: chunk 15, 2026.10.14

The record: `_handoffs/math98_chunk15_2026.10.14/README.md`.

**S53. `D_MAYBE_UNUSED` leads the typedef.** All three placements work with
GCC and Clang. The leading one is also where a standard `[[maybe_unused]]`
goes on a typedef, so the macro stays right if `D_MAYBE_UNUSED` is ever
defined that way at these levels.
- Alternative: after the declarator, the usual place for a GNU attribute on
  a typedef. That place would be a syntax error for `[[maybe_unused]]`.

> **Review.** Taken: for, right for a standard attribute too; against, none.
> Recommended: as taken. Taken.

**S54. The class-scope checks stay where they are.** `solve_requires_float`,
`decomp_requires_float`, the `measure_*_check` structs and `stellate_check`
assert at class scope, for the functions that name them. They could now be
`D_STATIC_ASSERT`s inside each function, but that is a change in five more
headers for no change in behaviour.
- Alternative: move them into the functions now, to finish the job in one
  chunk.

> **Review.** Taken: for, no change in behaviour, five headers untouched;
> against, two styles of check remain. Recommended: as taken, and moved when
> those headers are next changed. Taken.

### S55 to S59: chunk 16, 2026.10.14

The record: `_handoffs/math98_chunk16_2026.10.14/README.md`.

**S55. A kernel that can fail returns a status and writes through `_out`.**
The caller cannot use a result without seeing whether there is one. The
status tells overflow, which 1b's value promotes, from a pole or 0/0, which
are error values. Only `D_RATIONAL_OK` writes.
- Alternative: return the rational, with failure marked in it, such as a
  zero denominator. One argument fewer, but every caller must test the
  result, and each kind of failure needs its own in-band mark.
- Alternative: return a struct of the result and the status. The same
  information, at more length for C callers.

> **Review.** Taken: for, a result cannot be used unchecked, and each
> failure is told apart; against, an out parameter. Recommended: as taken;
> it is C's idiom. Taken.

**S56. Overflow is exact, by double width in `d_math_umax`.** A kernel
reports overflow only when the reduced result does not fit (section 2.3).
- Alternative: report overflow as soon as an intermediate does not fit, by
  a checked multiply. Simpler, but then MAX/3 + MAX/6, and every sum whose
  cross term passes a word, would overflow though the result fits, and 1b
  would turn exact values into inexact ones for nothing. F11 and F14 are
  this alternative.
- Alternative: `__int128`, where the compiler has one, for the double
  width. It is not ISO C99 or C++98: `-pedantic-errors` rejects it unless it
  is marked `__extension__`, 32-bit targets lack it, and MSVC has none. The
  two-word code would be needed anyway.

> **Review.** Taken: for, overflow only where the result does not fit, so
> exact values stay exact; against, more code than a checked multiply.
> Recommended: as taken. Taken.

**S57. The harness's reference is 256 bits, in 16-bit limbs.** The roadmap
said 128 bits. But a check multiplies an exact result, below 2^127, by a
term, below 2^64, so it needs 190 bits. And with 16-bit limbs the product of
two limbs fits 32 bits, so the reference needs no 64-bit type and runs the
same in ISO strict C++98 `-m32`.
- Alternative: `__int128`, where the compiler has one, as `t_interval.c`
  uses it (marked `__extension__`). 32-bit targets lack it, so 6 of the 16
  builds would have no reference, and it is too narrow for the checks
  anyway.

> **Review.** Taken: for, a reference wide enough, in every build; against,
> slower than `__int128`. Recommended: as taken. Taken.

**S58. The harness checks the double-width helpers on their own.** The
kernels divide only by a gcd of two denominators, which is below 2^(w-1), so
they never reach `divmod_wide`'s carried bit (section 6). The helper stays
general, for any divisor, and the harness checks it directly, as
`t_interval.c` checks its lattice helpers.
- Alternative: narrow `divmod_wide` to divisors below 2^(w-1) and drop the
  carry. Less code today, but the helper would be wrong for larger divisors
  with only a comment to say so, and rounding a rational to a float (1c)
  may want them.

> **Review.** Taken: for, a helper right for every divisor, and checked;
> against, code no caller needs yet. Recommended: as taken. Taken.

**S59. The steps are numbered in the planned order, not the list's.** P0 is
listed in Phase 0, but it is step 12, because G7 opens Phase 2 with it. C6
and C4b wait on things outside the lane, so they are counted last, as steps
47 and 48, though either may come sooner. If one does, the steps are
renumbered and the note says so.
- Alternative: number the items as listed. Simpler, but the count would
  jump whenever P0, C6 or C4b were done out of turn.

> **Review.** Taken: for, a count that does not jump; against, none.
> Recommended: as taken. Taken.

### S60 to S64: chunk 17, 2026.10.14

The record: `_handoffs/math98_chunk17_2026.10.14/README.md`.

**S60. A conversion rounds once, in integers, and then builds its result
exactly.** That is the only way to give the nearest value for every ratio,
in every build, and in constant evaluation.
- Alternative: floating-point division, `(double)n / (double)d`. It rounds
  up to three times: n and d each past 2^53, then the quotient. And on the
  x87, a quotient computed in extended precision is rounded again when it
  is stored.
- Alternative: the C library's `strtod` and its kin, which round correctly.
  They need the decimal text, and they are not constant expressions.

> **Review.** Taken: for, the nearest value for every ratio, in every build
> and in constant evaluation; against, slower than a division. Recommended:
> as taken. Taken.

**S61. Nearest, ties to even, whatever the environment's rounding mode.**
Under the roadmap's design, the value's context will carry a rounding mode.
Until then, the conversions do not read the floating-point environment, and
constant evaluation could not read it anyway.
- Alternative: follow `fegetround()` at run time. Run time and constant
  evaluation could then give different bits for one call, which breaks the
  roadmap's principle 4, "one answer".

> **Review.** Taken: for, one answer at run time and in constant evaluation;
> against, a caller's `fesetround` is ignored. Recommended: as taken, with
> the rounding mode carried in the context when a kind reads one, as
> planned. Taken.

**S62. 1b takes two chunks.** The conversions moved from 1c into 1b, because
the value's promotion needs them. 1c keeps text to value and
`numeric_compare`, and the count stays 48.
- Alternative: make the conversions a step of their own, and renumber to
  49.
- Alternative: one large chunk for all of 1b. The owner asked for small
  chunks.

> **Review.** Taken: process; sound. Recommended: as taken. Taken.

**S63. Two-word ratios convert to `long double` only.** That is all the
value needs: G2 sends an overflowing exact result to `long double`. To
`float`, a two-word ratio could pass the type's range, 2^±128, and then
needs subnormals and overflow handled.
- Alternative: all three types, with that range handling. More code, with
  no caller.

> **Review.** Taken: for, only what the value needs; against, a two-word
> ratio to float or double is missing. Recommended: as taken until a caller
> needs it. Taken.

**S64. The harness's oracle uses the C library's `nextafter` and `frexp`.**
They are independent of the kernels, and of `float_order.h`, whose powers of
two the kernels use to build their results.
- Alternative: `float_order.h`'s own `next_up` and `next_down`. Those are
  checked by `t_float_order.c`, but the oracle would then share a header
  with what it checks.

> **Review.** Taken: for, an oracle independent of what it checks; against,
> none. Recommended: as taken. Taken.

### S65 to S72: chunk 18, 2026.10.14

The record: `_handoffs/math98_chunk18_2026.10.14/README.md`.

**S65. No union.** Each kind's value has its own field, and the struct is
48 bytes on a 64-bit target (`raw/x87_probe.txt`) instead of about 32. In
return every operation is a constant expression from C++14, as in the rest
of the C core.
- Alternative: a union, constant-evaluable only from C++20, where changing
  the active member became allowed.

> **Review.** Taken: for, every operation a constant expression from C++14;
> against, 48 bytes a value instead of about 32 (64 on x86-64 since S112),
> which an exact matrix of values pays per element. Recommended: as taken;
> dense matrices (S239) carry the bulk data. Taken.

**S66. One representation.** A rational whose reduced denominator is 1 is an
integer, and every field a kind does not use holds 0 (`exact` 0/1). Equality
of kinds then means something, and 3/1 and 3 are never two different
values.
- Alternative: keep integer and rational apart by how a value was made. Then
  1/3 + 2/3 would be the rational 1/1, and every reader would have to check
  for it.

> **Review.** Taken: for, equality of kinds means equality of
> representation; against, none. Recommended: as taken. Taken.

**S67. A boolean or none in arithmetic is a domain error.** The math
language keeps truth values and numbers apart, as its relations and logic
(2d) will.
- Alternative: booleans as 0 and 1, as in C and Python. This makes
  `true + 1` legal and `x < y < z` subtly wrong.

> **Review.** Taken: for, numbers and truth kept apart, as the language's
> logic needs; against, unlike C and Python. Recommended: as taken. Taken.

**S68. The inexact flag marks every overflow into `long double`,** even
where the `long double` is exact (2^(w-1)). The flag says that the value has
left the exact kinds, which is what G2 asks for. It is kept by every result
computed from the value.
- Alternative: set it only where the rounding lost something, as IEEE's
  inexact flag does. A value would then turn floating-point without notice,
  wherever the overflow happened to land on a representable number.

> **Review.** Taken: for, the flag says the value left the exact kinds,
> always; against, none. Recommended: as taken. Taken.

**S69. Errors pass through, the first of two, with every flag.** This is
the same rule as for a NaN.
- Alternative: an order among the errors, such as invalid above pole above
  domain. That needs a rule nobody has asked for, and makes `a + b` and
  `b + a` differ less obviously.

> **Review.** Taken: for, the NaN rule, simple and predictable; against,
> which error comes first depends on operand order. Recommended: as taken.
> Taken.

**S70. No context parameter yet.** The roadmap's design passes a context
(precision, rounding mode, error policy). Nothing in this chunk would read
one: rounding is to nearest (S61), errors are values, and there are no big
floats. Its first reader adds it, while the only callers are the harness's.
- Alternative: take a context now and ignore it, so that no signature
  changes later. Every call would pass a value that does nothing.

> **Review.** Taken: for, no parameter that does nothing; against,
> signatures change when the first reader comes. Recommended: as taken.
> Taken.

**S71. `rational.h`'s sums and products return their whole result,** and its
rounding reports its direction. These are internal, for the value layer,
and `rational.h`'s own kernels are unchanged (section 2.5).
- Alternative: compute the exact result again in `value.h`. That would mean
  two copies of Knuth's forms.

> **Review.** Taken: for, one copy of the exact forms; against, none.
> Recommended: as taken. Taken.

**S72. The exact reference is shared.** `math_tests_exact.h` holds the
256-bit integers, the exact rationals and the nearest-value oracle for both
`t_rational.c` and `t_value.c`. Its functions are `static inline`, as in
`math_tests_random.h`, so a harness that does not use one is not warned
about it.
- Alternative: a second copy in `t_value.c`, about 800 lines.

> **Review.** Taken: for, one reference, checked by two harnesses; against,
> none. Recommended: as taken. Taken.

### S73 to S81: chunk 19, 2026.10.14

The record: `_handoffs/math98_chunk19_2026.10.14/README.md`.

**S73. Simple Decimal Conversion.** One algorithm, exact for every input,
in integer arithmetic on decimal digits: no table of powers, nothing wider
than a word, and constant-evaluable from C++14. It is slow at the far ends
of a `long double`'s range (section 2.3), which a literal reader can afford.
- Alternative: Eisel and Lemire's algorithm, as in Go's and Rust's standard
  libraries and in fast_float, which GCC's `std::from_chars` uses. Its usual
  case is one 128-bit product, where this one works over every digit for
  every 60 bits of exponent, so it is far faster. But it needs a table of
  128-bit powers of five, a 64 by 64 bit product into 128 bits, which the
  32-bit ISO strict C++98 build, with no 64-bit integer at all, would have
  to assemble from four words, and a slow path for its hard cases anyway.
  It can come later as a fast path in front of this one, where measured
  demand exists.

> **Review.** Taken: for, exact for every input, in every build,
> constant-evaluable; against, far slower than Eisel and Lemire's algorithm,
> the fastest correct method. Recommended: as taken, with Eisel-Lemire as a
> fast path where a 64-bit product exists, both correct, so no bit changes.
> Taken; the fast path is still to come.

**S74. One buffer, for the widest type.** Its capacity is the bound for a
`long double` (11,516 digits on the x87), whatever the type converted to.
- Alternative: a capacity per type (769 digits for a `double`), which saves
  stack for `float` and `double`, at the cost of three buffers or a size
  passed around; C++98 has no arrays of variable length.

> **Review.** Taken: for, one buffer, simple; against, about 11.5 KB of
> stack a call for every type, which small stacks feel. Recommended: as
> taken on hosted targets; a per-type capacity where stack is scarce. Taken.

**S75. The number and nothing else.** The syntax is the decimal part of a C
floating literal. No locale, no leading space, no hexadecimal, infinity or
NaN. Its callers tokenize first, and a decimal number token is exactly this.
- Alternative: `strtod`'s syntax, which would make the result depend on the
  locale's decimal point and accept text that other tokens own.

> **Review.** Taken: for, no locale, nothing other tokens own; against, no
> hexadecimal, infinity or NaN text. Recommended: as taken; the expression
> language names inf and nan as words. Taken.

**S76. Always to nearest, ties to even,** whatever the environment's
rounding mode, as chunk 17's conversions do (S61). The result is then the
same on every build and in constant evaluation.
- Alternative: honour the rounding mode, as `strtod` does. The result would
  then depend on run-time state that constant evaluation cannot see.

> **Review.** Taken: as S61. Recommended: as taken. Taken.

**S77. Underflow is a subnormal or zero result that is not exact,** judged
after rounding: a value that rounds up to the least normal value is not an
underflow, and an exact subnormal is not one either.
- Alternative: IEEE's tininess before rounding, under which a value just
  below the least normal one underflows even where it rounds to it. C leaves
  `strtod`'s `ERANGE` here to the implementation.

> **Review.** Taken: for, tininess after rounding, as x86 judges it;
> against, other platforms judge before. Recommended: as taken, documented.
> Taken.

**S78. The point's place is counted exactly, then held.** The digits before
the point, or the zeros after it, are counted in `size_t`, which cannot
overflow for a text in memory; the exponent's size is held at `SIZE_MAX`.
The two are combined with their signs, and only the sum is held within
10^6 of 0.
- Alternative: hold each within 10^6 on its own. A million zeros after the
  point with an exponent of 1,000,006, which is 1, would then read as 10^-1.

> **Review.** Taken: for, the exponent's sum held, not its parts, so every
> text reads right; against, none. Recommended: as taken. Taken.

**S79. The harness's own reference.** Integers of up to 54,400 bits,
independent of `decimal.h`'s algorithm and of the C library. A midpoint in a
`long double`'s lowest binade needs 38,000 bits; `math_tests_exact.h` has
256.
- Alternative: glibc's `strtod` as the reference. It is correctly rounded,
  and the record's tools check `decimal.h` against it, but a C library
  elsewhere need not be, and the harness should mean the same on any.

> **Review.** Taken: for, a reference that means the same on any platform;
> against, a large harness. Recommended: as taken. Taken.

**S80. A nudge past the buffer, or at its last digit, is checked against
the same nudge within it,** which the reference has checked. Both must round
the same way, since both lie strictly between the same two midpoints, or on
the same side of one value. The run takes 10.6 seconds.
- Alternative: the reference for every text. It agrees on all 17,858 of
  them, but its numbers there reach 38,000 bits, and the run takes 67
  seconds (`raw/allref.txt`).

> **Review.** Taken: for, the same verdict in a sixth of the time, each
> nudge's side proved; against, an argument in place of a direct check.
> Recommended: as taken. Taken.

**S81. The smoke tier, not the full matrix.** Nothing outside `t_decimal.c`
includes `decimal.h` (`raw/includers.txt`), so no other test's build can
change. `t_decimal`'s 16 builds ran in full, and the smoke tier shows every
other test's result unchanged, byte for byte, from chunk 18's full run.
- Alternative: the full matrix, about an hour on two cores, which would
  show the same.

> **Review.** Taken: for, the include closure shows no other build can
> change; against, the full matrix is skipped. Recommended: as taken. Taken.

### S82 to S89: chunk 20, 2026.10.15

The record: `_handoffs/math98_chunk20_2026.10.15/README.md`.

**S82. A header of its own.** `literal.h` reads text into a value;
`value.h` stays arithmetic on values, and `decimal.h` stays floating point.
A file that includes `value.h` does not compile `decimal.h` too.
- Alternative: `d_math_value_from_text` in `value.h`, one header fewer,
  which every user of the value would compile.

> **Review.** Taken: for, users of the value do not compile the decimal
> reader; against, one header more. Recommended: as taken. Taken.

**S83. Three forms, and nothing between them.** A fraction's terms are
digits only, with no point, exponent or sign; a mixed number is separated
by spaces, and its sign is the whole number's, in front. So `1.5/2` and
`1/-2` are not literals: the literals at their start are `1.5` and `1`,
and a parser sees the division. Spaces only, not tabs or line breaks, and
none around the literal: a tokenizer decides what surrounds a token.
- Alternative: decimal terms, or signs on terms, which make a fraction a
  division of other literals; or `1+1/2` for a mixed number, which an
  expression reads as an addition.

> **Review.** Taken: for, a literal that never swallows what a parser should
> see; against, `1.5/2` is a division, not a literal. Recommended: as taken.
> Taken.

**S84. Exact at any length.** A decimal is judged by its value, not by how
it is written: the 2s and 5s it shares with its power of ten are taken out
of its digits, and zeros at either end do not count. So `0.1` followed by a
thousand zeros is 1/10, and `1` followed by a thousand zeros and `e-1000` is
the integer 1. Only a decimal with more significant digits than any exact
one has (63 or 31) is not worked on.
- Alternative: read the digits into two words and give up past them, so
  that `0.10` written with forty zeros would be inexact and `0.1` exact.

> **Review.** Taken: for, a decimal exact by its value, at any length;
> against, none. Recommended: as taken. Taken.

**S85. Not exact: the nearest `long double`, from the text, or a range
error.** A decimal that is not exact is read by `decimal.h` straight into a
`long double`, rounded once (G3), and flagged inexact (G2). Past the
largest `long double` it is a range error, not an infinity: the literal is
an exact number the value cannot hold yet, not an IEEE result. Below the
least subnormal it is 0 or the nearest subnormal, flagged, with its sign:
that is the nearest `long double`, and no error.
- Alternative: an infinity past the largest, as `strtold` gives; the exact
  tower would then hold an infinity that no exact operation made, and
  `1e5000 - 1e5000` would be a NaN, not an error.
- Alternative: a range error below the least subnormal too, as `strtold`
  reports `ERANGE`. The flag already says the value is not exact.

> **Review.** Taken: for, no infinity in the exact tower, and the nearest
> long double below the subnormals; against, a range error where `strtold`
> gives an infinity. Recommended: as taken until bignum holds the value
> exactly. Taken.

**S86. Fractions in two words.** A fraction's terms are read into two
words, the width that `rational.h` reduces and rounds in; past that, a
range error until bignum (P1). The zeros both terms end with, a common
factor of a power of ten, are dropped first, so `10/20` with a thousand
zeros on each term is 1/2. A mixed number w n/d is (w d + n)/d, whose
numerator must fit too.
- Alternative: a gcd on the decimal digits first, so that any fraction
  whose reduced terms fit is read. That is bignum's work.

> **Review.** Taken: for, the terms `rational.h` works in; against, a
> fraction whose reduced terms fit but whose written terms do not is a range
> error. Recommended: as taken until bignum (P2, P3). Taken.

**S87. Over 0 first.** A fraction over 0 is a pole, or invalid for 0/0,
whatever the size of the other terms: a numerator of a hundred digits over
0 is a pole, and `1 0/0` is invalid, as an exact division by 0 is in
`value.h`.
- Alternative: the range first, so that a long numerator over 0 is a range
  error. The literal has no value either way; the pole says why.

> **Review.** Taken: for, the error that says why; against, none.
> Recommended: as taken. Taken.

**S88. The harness reads each literal itself, on the shared reference.**
`t_literal.c` has its own scan and reads the terms as `math_tests_exact.h`'s
256-bit integers, the reference that `t_rational.c` and `t_value.c` already
check the rounding with. Past those bits, a decimal is checked against
`decimal.h`'s reading, which chunk 19 checked. The record adds a check on
Python's arbitrary-precision rationals, which reaches what 256 bits cannot:
decimals of 400 and 11,500 digits, and the fractions nearest the midpoints
between `long double`s.
- Alternative: a reference of 54,400 bits, as `t_decimal.c` has. The
  values `literal.h` computes have terms of at most two words, whose
  products 256 bits hold; the long decimals are `decimal.h`'s, checked
  there.

> **Review.** Taken: for, a check independent of `literal.h`, carried past
> 256 bits by an outside tool; against, none. Recommended: as taken. Taken.

**S89. The smoke tier, not the full matrix, as in S81.** Nothing outside
`t_literal.c` includes `literal.h` (`raw/includers.txt`). Its 16 builds ran
in full, and the smoke tier shows every other test's result unchanged.
- Alternative: the full matrix, about an hour on two cores, which would
  show the same.

> **Review.** Taken: as S81. Recommended: as taken. Taken.

### S90 to S99: chunk 21, 2026.10.15

The record: `_handoffs/math98_chunk21_2026.10.15/README.md`.

**S90. The header's design, kept.** `integer_value`, `rational_value`,
`parse_number`, `compare` and the six predicates, as the old header
declared them. Its internal helpers (`big_add`, `big_mul`, `normalize` and
the rest) are gone: `integer_value`'s members do their work. Added:
`numeric_text` (S94), `integer_value::digits` and `!=`, and
`rational_value::is_number` (S93). Changed: `integer_value` is made from
`d_math_umax`, since strict C++98 `-m32` has no 64-bit integer for
`std::uint64_t`; its text constructor reads the digits at the start, as
`strtoul` does; its subtraction gives 0 where the result would be
negative. The class stays `rational_value`, as the old header named it:
`math::rational` is `constants.hpp`'s compile-time rational, from C++14.
- Alternative: the old `.cpp`'s design, `integer_constant` and
  `decimal_constant` with `std::strong_ordering`, which needs C++20 (the
  owner chose C++98).

> **Review.** Taken: for, the old design's names and shape kept, built for
> C++98; against, none of weight. Recommended: as taken. Taken.

**S91. The arithmetic is the face's, on decimal digits, until bignum.**
Exact comparison at any length needs storage that grows with the input,
and the C core's kernels allocate nothing. So `integer_value` keeps its
digits in a `std::string` and works on them one at a time. Bignum (P1 to
P4) brings binary limbs and faster products to the C core; this face can
then stand on them.
- Alternative: digit kernels in the C core now, on caller buffers, which
  P1's binary limbs would replace.

> **Review.** Taken: for, exact comparison at any length now; against,
> decimal digits in a string, one at a time: slow, quadratic in length, and
> a second big-number arithmetic beside bignum's. Recommended: as taken
> until bignum (P2 to P4), then this face on its limbs, as planned. Taken.

**S92. A power of ten held apart.** A `rational_value` is n / d times 10^k,
k of any size, so `1e100000000000000000000` holds a few digits, not 10^20;
orders of magnitude settle a comparison before anything is multiplied, and
cross products are formed only for numbers within a factor of 100.
- Alternative: n / d alone, as the old header had it, which makes 10^k's
  digits. A number with a 20-digit exponent could not be read.

> **Review.** Taken: for, numbers with huge exponents read and compared in a
> few digits; against, none. Recommended: as taken. Taken.

**S93. Not a number is unordered, as a NaN is; nothing throws.** A text
that is not one literal, or a fraction over 0, compares as 2, value.h's
`D_MATH_VALUE_UNORDERED`, with everything, itself included; every predicate
but `not_equal` is false for it. `parse_number` returns it as a
`rational_value` whose `is_number` is false. The framework is used with
exceptions off (the C++ guide's error policy), and value.h already treats
a NaN and an error this way.
- Alternative: throw `std::invalid_argument`, as the old header's
  `<stdexcept>` suggested.
- Alternative: order non-numbers before every number, a total order for
  sorting. `equal("abc", "xyz")` would then be true.

> **Review.** Taken: for, nothing throws, as the framework runs with
> exceptions off, and a non-number behaves as a NaN does; against, sorting
> needs its own order. Recommended: as taken. Taken.

**S94. `numeric_text` in place of `std::string_view`.** C++98 has no
`string_view`, and re_std's begins at C++11. A pointer and a length, made
implicitly from a C string, a `std::string` and, from C++17, a
`std::string_view`, gives every function one signature at every level.
- Alternative: an overload of each function for each kind of string, three
  or four apiece; or re_std's `string_view`, from C++11 only.

> **Review.** Taken: for, one signature at every level; against, a type of
> the framework's own where C++17 has `string_view`. Recommended: as taken.
> Taken.

**S95. One syntax, `literal.h`'s.** `parse_number` asks
`d_math_literal_length` whether the whole text is one literal, so the C
core and the face read the same numbers: a text the value layer reads is
one numeric_compare can compare.
- Alternative: a syntax of numeric_compare's own. The old `.cpp`'s skipped
  leading spaces and read no scientific notation or mixed numbers.

> **Review.** Taken: for, the C core and the face read the same numbers;
> against, none. Recommended: as taken. Taken.

**S96. The `.cpp` stays, as a translation unit that includes the header.**
Its neighbours in `src/djinterp/core/math/` are the same, and the zip of
plain files can replace a file but not delete one.
- Alternative: delete it, which the owner would then have to do by hand.

> **Review.** Taken: for, a zip of files cannot delete; against, an almost
> empty unit. Recommended: as taken. Taken.

**S97. `literal.h`'s scan struct renamed, a fix to chunk 20.** Its
`struct d_internal_math_literal_scan` and its function of the same name
make g++'s `-Wshadow` fail in C++ ("hides constructor"), for any file that
includes `literal.h`; the face tests build with `-Wshadow`. The struct is
now `d_internal_math_literal_parts`; the function keeps its name, so chunk
20's planted faults still apply. `raw/shadow_probe.txt` shows chunk 20's
`literal.h` failing and this one passing.
- Alternative: rename the function, which chunk 20's tools name.

> **Review.** Taken: a fix; sound. Recommended: as taken. Taken.

**S98. A cross-check in Python with methods of its own.** The face test's
references stop where `d_math_umax` and the C core's family do; past them,
it checks the comparison against rewrites of each number and against
itself. Python's `Fraction`, `Decimal` and integers check it from outside,
up to 300 digits and exponents of 30.
- Alternative: an exact reference in the face test, as `t_literal.c` has
  `math_tests_exact.h`. Its 256 bits stop well short of a 30-digit
  exponent.

> **Review.** Taken: for, a check from outside, past the face test's
> references; against, it runs only in the record. Recommended: as taken.
> Taken.

**S99. The smoke tier, not the full matrix, as in S81 and S89.** Only the
face test includes `numeric_compare.hpp`, and only `t_literal.c` and
`numeric_compare.hpp` include `literal.h` (`raw/includers.txt`). Their 64
and 16 builds ran in full, and the smoke tier shows every other test's
result unchanged.
- Alternative: the full matrix, about an hour on two cores.

> **Review.** Taken: as S81. Recommended: as taken. Taken.

### S100 to S112: chunk 22, 2026.10.15

The record: `_handoffs/math98_chunk22_2026.10.15/README.md`.

**S100. The registry is the table each kind fills in, and each value
carries it.** A boxed value points to its kind's `struct d_math_boxed_kind`,
so dispatch needs no lookup, and there is no list of kinds anywhere: no
global state, as Phase 1's design asks, and nothing to register at start-up
or to keep in step across translation units. A lookup by name, which the
parser (2b) may want, can then be a list the caller owns.
- Alternative: a registry object holding the kinds, numbered as they are
  added, and a value holding the number. Every operation would need the
  registry passed to it, so `value.h`'s every signature would change, and
  values from two registries would not mix.
- Alternative: one global registry, filled at start-up: global state, in
  a header-only C core, where a static array is one per translation unit.

> **Review.** Taken: for, no global state, no lookup, values from any unit
> mixing; against, no list of kinds to enumerate. Recommended: as taken.
> Taken.

**S101. One scale of ranks, the inline numbers' included; a rank is its
kind's own.** The inline numbers stand at 100 to 500, so a boxed kind's
rank says where it sits among them as well as among boxed kinds: an exact
kind past the rational's, an inexact one past the long double's, a
container past every number's. No two kinds share a rank, which makes the
rank the kind's identity: in C, every translation unit that includes a
kind's header has its own copy of its table, at its own address, so a kind
knows its own values by their rank, not by the table's address.
- Alternative: a kind's place given as "just above" an inline kind, and a
  separate order among boxed kinds: two fields where one does.
- Alternative: identity by the table's address, which fails across
  translation units in C; or by its name, a string compare per operation.

> **Review.** Taken: for, one scale says where a kind stands among all
> numbers, and gives it an identity across C's units; against, two
> independently written kinds can choose one rank and be taken for each
> other, with nothing to detect it. Recommended: as taken, with the ranks in
> use listed in `value.h`'s banner and a rule for choosing new ones. Taken
> in part.

**S102. The operand higher in the tower handles an operation, an inline
number too.** Where a boxed kind is the higher, its table does the work;
where an inline number is, `value.h` converts the boxed operand to that
number's kind through the kind's `convert` and carries on as with two
inline numbers. So a wide integer meeting a double becomes a double
correctly rounded, as an exact rational does, with no code in the kind.
For a comparison the boxed number goes to the nearest `long double`, and
`convert` reports which way it rounded: where the rounding equals the
other operand, the direction is the order, so every comparison stays
exact, as `value.h`'s own `cmp_exact` does it for rationals.
- Alternative: a boxed kind handles every operation with an inline number,
  which puts conversions to each floating-point type, and exact comparison
  with each, into every kind.
- Alternative: Python's reflected operators, the first operand asked
  first and the second if it declines: two calls where one does, and
  results that depend on the order.

> **Review.** Taken: for, no conversion code in any kind, comparisons kept
> exact; against, none. Recommended: as taken. Taken.

**S103. A result is the caller's; operands are lent.** Values pass by
value in C, so ownership is the caller's to keep: every result is to be
given back with `d_math_value_release` (which does nothing more for an
inline value, so a caller need not ask), no operation keeps or releases
an operand, and `d_math_value_retain` makes a copy. `release` takes a
pointer and leaves the value none, so a released value cannot be used by
mistake. A kind keeps its boxes as it likes; the harness's kinds count
their holders. `math::value` (1e) will do the retaining and releasing in its
copies and destructor.
- Alternative: operations that consume their operands, which makes
  `a + a` a double release.
- Alternative: `release` by value, leaving the caller a copy of a freed
  box.

> **Review.** Taken: for, C's value semantics with one owner, and a released
> value left none; against, a caller must release every result. Recommended:
> as taken; `math::value` does it in C++. Taken.

**S104. Errors, booleans and none are settled before any kind is asked.**
An error passes through, a boolean or none in arithmetic is a domain
error, and either beside a boxed value is unordered, exactly as with
inline operands. No kind has to handle them, and none can handle them
differently.
- Alternative: hand them to the kind, so that, say, a vector of booleans
  could take `true` as an operand. Logic is 2d's, and can add its own
  operations then.

> **Review.** Taken: for, every kind meets only numbers, and none can
> disagree on errors; against, a kind cannot give booleans a meaning of its
> own. Recommended: as taken. Taken.

**S105. A memory error.** A kind that allocates must be able to fail, and
`value.h`'s errors are values, so `D_MATH_VALUE_ERROR_MEMORY` joins them.
`value.h` itself never makes one.
- Alternative: a range error, which means something else; or failing
  outside the value, which C would have to do with a global flag.

> **Review.** Taken: for, failure as a value; against, none. Recommended: as
> taken. Taken.

**S106. One entry for the four operations, and one for the two unary
ones, each taking an `enum d_math_value_op`.** A new operation then adds
an enumerator, not a table entry: a new entry would change every kind's
table, and with `-Wextra` (missing field initializers) break every kind
that had not added it. The four operations were already numbered inside
`value.h`; the numbers are now public.
- Alternative: one entry per operation, clearer to read in a table, and a
  change to every kind for each new one.

> **Review.** Taken: for, a new operation changes no kind's table; against,
> a switch in every kind. Recommended: as taken. Taken.

**S107. `text` is in the table, and `value.h` does not call it yet.** The
design lists text among a kind's operations, and adding the entry later
would change every kind's table (S106). But an inline value has no text
of its own yet: how a `float` is written is the renderer's question (2b),
so `value.h` has no text function to dispatch from.
- Alternative: leave `text` out until 2b.

> **Review.** Taken: for, no table change later; against, an entry nothing
> called then. Recommended: as taken. Taken.

**S108. `t_value_boxed.c` registers kinds of its own, which mirror inline
values.** A real kind (chunk 24) needs a reference of its own; three
mirrors of inline values at ranks of their own (150, between the integer
and the rational; 250, between the rational and the float; 600, past the
long double) make `value.h`'s inline arithmetic the reference for every
boxed path, and a fourth, a pair, is a kind that is no number. Every
check also counts the calls into the tables, so a result that comes out
right by the wrong path is still caught.
- Alternative: test the registry only with the wide integer, in chunk 24:
  the protocol and the kind would then be checked through each other.

> **Review.** Taken: for, the inline arithmetic as the reference for every
> boxed path, and the path itself checked; against, none. Recommended: as
> taken. Taken.

**S109. Step 9 in three parts.** The roadmap said two, the registry and
the kind. The kind's arithmetic, wide integers from limbs with division
and correct rounding to floating point, is a header and a harness of its
own, as `rational.h` was before `value.h`; putting it beside its
registration as a kind would make one chunk of some 3,000 lines. So: the
registry (this chunk); `c/math/wide.h`, which is P1, step 40, done early
(chunk 23); and the wide integer as a boxed kind (chunk 24), whose diff
then shows that `value.h` did not change.
- Alternative: the integer and its kind together in chunk 23, as the
  roadmap first had it.

> **Review.** Taken: process; sound. Recommended: as taken. Taken.

**S110. The exact comparison of a rational and a floating-point value is
shared.** `t_value.c`'s own copy moved into `math_tests_exact.h`, as S72
did with the rest of the reference; `t_value_boxed.c` finds its kinds'
rounding directions with it.
- Alternative: a second copy, about 70 lines, in the new harness.

> **Review.** Taken: as S72. Recommended: as taken. Taken.

**S111. The smoke tier, not the full matrix, as in S99.** Every test that
includes `value.h` or `math_tests_exact.h` ran in full (`raw/includers.txt`):
`t_value`, `t_value_boxed`, `t_literal`, `t_rational` and
`t_numeric_compare_face`. The smoke tier shows every other test's result
unchanged.
- Alternative: the full matrix, about an hour and a half on two cores.

> **Review.** Taken: as S81. Recommended: as taken. Taken.

**S112. Two pointers in every value.** `type` and `box` are fields of
their own, NULL in an inline value, as every unused field holds 0 (S66).
A value grows from 48 bytes to 64 on x86-64, and from 40 to 48 at 32 bits
(`raw/sizes.txt`). On x86-64 one pointer would cost as much: a
`long double` aligns the struct to 16 bytes, so 48 plus 8 is 64 too.
- Alternative: one pointer, to a box that starts with its kind's table:
  an indirection on every dispatch, a layout imposed on every kind's box,
  and 4 bytes saved at 32 bits.
- Alternative: the pointers in place of the unused `exact` field, through
  a union, which constant evaluation cannot switch between before C++20
  (S65).

> **Review.** Taken: for, dispatch with no indirection and no layout imposed
> on a kind's box; against, 64 bytes a value on x86-64. Recommended: as
> taken; the size is the price of constant evaluation before C++20 (S65).
> Taken.

### S113 to S126: chunk 23, 2026.10.17

The record: `_handoffs/math98_chunk23_2026.10.17/README.md`.

**S113. One width, 256 bits, in one type, `struct d_math_wide`.** P1 asks
for fixed-width integers from limbs and names no width. 256 bits holds the
product of any two 128-bit values, and of two `d_math_imax` with room to
spare; it is the next width in common use after 128, as in 256-bit hashes
and the Ethereum virtual machine's word; and it keeps a value at 36 bytes,
and its arithmetic, a gcd apart, under a hundred nanoseconds on x86-64
(section 2.5). The limb kernels take any length, so another width later is a
thin type over them.
- Alternative: 128 bits, two words: little past what `rational.h` already
  does in double width.
- Alternative: the width set by a configuration macro, which changes the
  layout of every value for every user of the build.
- Alternative: a family of widths, 128, 256 and 512, as types of their own:
  more code than one step needs, before any kind asks for them.

> **Review.** Taken: for, one width that holds the products the value needs,
> and kernels of any length beneath it; against, a fixed width, until
> bignum. Recommended: as taken. Taken.

**S114. Sign and magnitude, not two's complement.** The range is then
symmetric, so negation and the absolute value never fail; 0 has one form;
and the magnitude is a plain natural for the limb kernels, as the products,
quotients and roundings want it, and as the boxed kind's ratios will.
- Alternative: two's complement, -2^255 to 2^255 - 1, as C's integers:
  addition and subtraction need no sign test, but -(-2^255) overflows,
  products and quotients take magnitudes anyway, and the positive range is a
  bit shorter.

> **Review.** Taken: for, a symmetric range, one zero, and magnitudes for
> the kernels; against, a sign test in addition. Recommended: as taken.
> Taken.

**S115. Limbs of half a word, chosen as `math_common.h` chooses the word.**
`uint32_t` where `dstdint.h` declares `intmax_t`, which is where the word is
64 bits, and `uint16_t` where it does not, where the word is 32. A limb
times a limb plus two limbs fits a word, so no step needs a product of two
words, which `rational.h` has to build from four half-products; the code is
shorter, and as constant-evaluable as the rest. Two static assertions keep
the limb at half a word.
- Alternative: limbs of a word, the products split in halves as `rational.h`
  splits them: half the limbs, four multiplications and the carries for each
  product, and more code in every kernel.

> **Review.** Taken: for, no double-word product, the same code at every
> width, constant evaluation; against, two to four times the work of word
> limbs where a 128-bit product exists. Recommended: as taken, with a
> word-limb path where `unsigned __int128` exists, giving the same bits.
> Taken in part.

**S116. Exact or nothing, `rational.h`'s first four statuses by number, and
two more.** A kernel that fails stores nothing anywhere, `divmod`'s two
destinations included, so a caller may pass an operand's own storage as the
destination and still have it after a refusal. `FRACTION` is a rational that
is no integer, `SYNTAX` a text that is no integer.
- Alternative: one status for every failure, which leaves a caller to work
  out which it was.

> **Review.** Taken: for, nothing stored on failure, so a caller may reuse
> an operand's storage; against, none. Recommended: as taken. Taken.

**S117. Division truncates, as C's, and either result may be left out.** The
quotient goes toward zero and the remainder takes the dividend's sign, as
`/` and `%` do in C99 and C++11, so a wide integer divides as a C programmer
expects. Floor division (Python's) and the Euclidean remainder follow from
the truncated pair in a line, by the caller. `divmod` with a NULL
destination gives the other result alone, so there is no `div` or `mod` to
keep in step.
- Alternative: floor division, which C programmers would have to unlearn; or
  separate `div` and `mod`, each doing the whole division.

> **Review.** Taken: for, C's division; against, the language's `mod` is
> floored, so the two layers differ, each as its users expect. Recommended:
> as taken. Taken.

**S118. The limb kernels serve any length, in GMP's mpn style, and Knuth's
division takes its scratch from the caller.** Chunk 24's ratios need the
same kernels at twice the width, and bignum (P2) at any; with the lengths
given and the scratch the caller's, they need neither allocation nor a
longest length. The type's kernels give them arrays on the stack.
- Alternative: kernels for 256 bits only, rewritten for 512 in chunk 24, and
  again for P2.
- Alternative: division a bit at a time, as the harness's reference does:
  simpler, with a step for each bit of the quotient where Knuth's takes one
  for each limb.

> **Review.** Taken: for, one kernel set for 256 bits, 512 and bignum,
> allocation-free; against, none. Recommended: as taken. Taken.

**S119. The conversions to floating point report the rounding's direction,
through a pointer that may be NULL.** `value.h`'s `convert` entry needs it
(S102): a boxed number compared with a float goes to the nearest `long
double`, and where that ties, the direction is the order. A second answer
through a pointer is how `remquo` gives its quotient, and `remainder.h`
already takes a NULL one.
- Alternative: `rational.h`'s form, the value alone, with the direction from
  an internal function: the kind would repeat `to_f`'s handling of an
  infinity.

> **Review.** Taken: for, the direction exact comparison needs; against,
> none. Recommended: as taken. Taken.

**S120. A float past the largest is an infinity of the integer's sign, its
direction away from 0.** That is IEEE 754's round to nearest, and what
`decimal.h` stores past the range. A double's and a long double's range hold
every wide integer, which a static assertion checks, so `to_f` alone tests
for it.
- Alternative: an overflow status, as `decimal.h` returns. Its kernels
  return a status anyway, since a text may be no number; here a value with
  its direction says as much.

> **Review.** Taken: for, IEEE 754's round to nearest; against, none.
> Recommended: as taken. Taken.

**S121. Rounding reuses `rational.h`'s result and its builders.** The kernel
rounds an integer to P bits from its bits, the top P, the one below and
whether anything is set lower, into `d_internal_rational_rounded`, and
`d_internal_rational_build_f`, `_d` and `_ld` make the value, so `wide.h`
has no floating-point code of its own. A ratio, as chunk 24 will have,
rounds through the same kernel: its quotient carried two bits past P, with a
remainder that is not 0 set into the lowest bit.
- Alternative: a rounding of its own, long division a bit at a time as
  `rational.h`'s, a step for each bit of the precision, where an integer
  needs no division at all.

> **Review.** Taken: for, one rounding to trust; against, none. Recommended:
> as taken. Taken.

**S122. Text is decimal: an optional sign and digits, every character of the
text.** Leading zeros are allowed, -0 is 0, and a text that is both no
integer and too large is `SYNTAX`: its form is found before its size.
`d_math_wide_to_text` writes as `snprintf` writes, as `interval.h`'s
formatters do, and is `D_MATH_FN_RT`, `math_common.h`'s rule for writers.
- Alternative: hexadecimal, digit separators and spaces, which are a
  literal's business (`literal.h`), not an integer's.
- Alternative: `to_text` as a constant expression into the evaluation's own
  buffer, which the C core's rule does not allow.

> **Review.** Taken: for, an integer's text and nothing a literal owns;
> against, none. Recommended: as taken. Taken.

**S123. gcd by Euclid over the same division.** One division algorithm to
trust, and fast enough: about 6 microseconds for two random integers of 256
bits on x86-64 (section 2.5). The harness's reference finds its gcd the
binary way, so the two do not share a mistake.
- Alternative: Stein's binary gcd, shifts and subtractions only: a second
  algorithm in the unit.

> **Review.** Taken: for, one division to trust; against, about 6 us, where
> Lehmer's gcd is several times faster. Recommended: as taken, Lehmer's in
> P4 where speed is asked. Taken.

**S124. `t_wide.c` has a reference of its own.** `math_tests_exact.h`'s
integers are 256 bits: short of a product of two operands (512), of the limb
kernels' operands at twice the width, and of the oracle's values, which
reach 2^256 and pass it. Its limb count cannot be raised for one harness
without a second definition of the macro, which the macro lint refuses, nor
for all without changing three other harnesses. The new reference, 640 bits,
is written another way than `wide.h`.
- Alternative: widen the shared reference to 640 bits: its users would run
  slower, and their checks would change for no gain of theirs.

> **Review.** Taken: for, a reference wide enough, written another way;
> against, a second reference to keep. Recommended: as taken. Taken.

**S125. Coverage as a check of the record.** gcov counts every line of
`wide.h` that `t_wide.c` runs, at both limb widths, and the rarer paths of
Knuth's division (`raw/coverage.txt`): every line runs. A line no check
reaches is a line no fault planted there could show.
- Alternative: rely on the planted faults alone, which sample lines.

> **Review.** Taken: for, every line reached by some check; against, none.
> Recommended: as taken. Taken.

**S126. The smoke tier, not the full matrix, as in S99 and S111.** Nothing
but `wide.h` and `t_wide.c` changed in the code, and nothing includes
`wide.h` but its harness (`raw/includers.txt`). `t_wide`'s 16 builds ran in
full; the smoke tier shows every other test's result unchanged.
- Alternative: the full matrix, about an hour and a half on two cores.

> **Review.** Taken: as S81. Recommended: as taken. Taken.

### S127 to S140: chunk 24, 2026.10.17

The record: `_handoffs/math98_chunk24_2026.10.17/README.md`.

**S127. The kind is a header of its own, `c/math/value_wide.h`, over
`value.h` and `wide.h`, and neither changes.** The registration needs both
the integer's arithmetic and the value's table; a header beside them that
includes both keeps `wide.h` free of the value layer and of `malloc`, and
shows, by the diff, that `value.h` took no change (`raw/unchanged.txt`).
- Alternative: the kind at the end of `wide.h`: a program that wants only
  wide integers would get the value layer and an allocator with them.
- Alternative: the kind in `value.h`, the change 1d set out to show a new
  kind does not need.

> **Review.** Taken: for, `wide.h` free of the value layer and the
> allocator, `value.h` unchanged; against, a header more. Recommended: as
> taken. Taken.

**S128. Rank 210.** `value.h` places an exact kind that holds more than
`d_math_imax` past the rational's 200 and below the float's 300. Of those,
the wide integer holds the least, a fixed 256 bits; bignum's kinds (P2, P3)
will hold every wide integer, so they belong above it, and 210 leaves them
220 to 290.
- Alternative: 250, the middle, which gives no reason of its own and leaves
  less room above.
- Alternative: just below the float's, which leaves bignum's kinds no room
  above the wide integer.

> **Review.** Taken: for, room above for bignum's exact kinds; against,
> none. Recommended: as taken. Taken.

**S129. An integer result stays a wide integer, however small.** An
operation of the kind's gives its own kind where the result is an integer
within 256 bits, as an operation of a float gives a float. Until bignum
promotes it (P2), an inline integer that overflows becomes the nearest
`long double` (G2), so a wide integer given back as an inline one where it
fits would lose the exactness it was made for at the next overflow: a
factorial begun from a wide 1 would turn inexact at 21!.
- Alternative: an integer that fits `d_math_imax` given back inline, as Ruby
  and Scheme normalize their bignums. That suits a small kind whose overflow
  promotes, which `d_math_imax`'s does not yet. P2 can revisit it, when
  overflow promotes to bigint.

> **Review.** Taken: for, an exact value never turns inexact at the next
> overflow; against, small integers stay boxed, an allocation each, and
> arithmetic of 256 bits for numbers that fit a word. Recommended: as taken
> until bignum promotes overflow (P2); then normalize to the inline kind, as
> Python and Scheme do. Taken.

**S130. No wide rational: a result that is no integer is an inline rational
where it fits, and anything past both kinds the nearest `long double`,
flagged inexact.** A quotient of two wide integers, or a sum or product
with a rational, is a rational; the kind has no rational of its own, so it
keeps one of the family inline, and past the family does as G2 does for
every exact result that does not fit (`value.h`, gate G2), as does an
integer past 2^256 - 1.
- Alternative: a range error past the kinds, which G2 already rejected for
  the inline kinds (1b).
- Alternative: a wide rational kind, a pair of wide integers: a kind and
  harness of its own, and much of P3's work, ahead of it.

> **Review.** Taken: for, no new kind ahead of its step; against, a rational
> of wide terms becomes the nearest long double, flagged inexact: exactness
> lost where a CAS keeps it. Recommended: as taken as an interim, big
> rationals in P3, as planned. Ruled 2026.11.04: the interim stands; big
> rationals at P3, step 45. **Flag:** significant drawback until P3.

**S131. The long double is rounded once from the exact ratio, at twice the
width, by `wide.h`'s division.** A result's terms run to 512 bits: the
product of two wide integers, or a wide integer times a word. Its numerator
is moved up until it has P + 2 bits more than its denominator, Knuth's
division takes the quotient, a remainder that is not 0 is set into the
quotient's lowest bit, and `wide.h`'s rounding kernel rounds it to P bits,
as S121 planned. The quotient's bit below the mantissa is its own, so the
lowest bit is below it, and the remainder's place is kept exactly.
- Alternative: `rational.h`'s loop, a bit at a time, on terms of 512 bits:
  a step of 16-limb arithmetic for each bit of the precision.
- Alternative: the terms each rounded to `long double` and divided: two
  roundings, sometimes an ulp off, and no direction.

> **Review.** Taken: for, rounded once, with its direction; against, none.
> Recommended: as taken. Taken.

**S132. Every result is a box of its own.** No result shares an operand's
box, not even |x| of a positive x or x + 0. One allocation per result:
a value made and given back takes 17.5 ns on x86-64 (section 2.5), against
239 ns for a sum. And one rule for ownership, which the harness checks on
every operation.
- Alternative: the operand's own box with a holder more where the value is
  the same: an allocation saved in a few cases, and a second rule to state
  and test.

> **Review.** Taken: for, one ownership rule, checked on every operation;
> against, an allocation where a holder more would do. Recommended: as
> taken. Taken.

**S133. A lower boxed operand is read through its own table, as an exact
number of the family, and only an exact kind is asked.** `value.h` hands
the kind any operand no higher than itself; the kind asks an exact one to
convert to a rational, and takes what comes back only where the kind says
it converted. Anything else, a kind that is no number, an inexact one, or
an exact value past the family, gives a domain error, or no order, as
`value.h` gives where an inline number cannot read a lower boxed operand.
- Alternative: the operands' nearest `long double`s, the result inexact: a
  guess presented as an answer, for a kind `value.h`'s rank rules place
  above the long double in the first place.

> **Review.** Taken: for, no inexact guess presented as an exact answer;
> against, none. Recommended: as taken. Taken.

**S134. Storage through two knobs of a configuration header of its own, and
holders in a plain integer.** `config/c/math/cfg_value_wide.h` defines
`D_CFG_MATH_VALUE_WIDE_MALLOC` and `_FREE`, `malloc` and `free` unless set
before the kind is first included, as fs's allocation hooks are
(`cfg_file_common.h`), and as `dconfig`'s rule wants a module's knobs: in
its `cfg_*.h`, the module reading them only. A host with an arena takes
every box with two definitions; the harness counts every box and refuses
some. C99 has no atomics, so holders are a `size_t`, as the harness kinds'
were (chunk 22, section 6).
- Alternative: `malloc` and `free` called directly: no arena, and no way for
  a harness to refuse storage.
- Alternative: the knobs' defaults in `value_wide.h`, which `dconfig`'s
  localization rule forbids.
- Alternative: one knob for all of math: no other kind allocates yet. When
  bignum's do, an aggregate default can stand above these, as `dconfig`'s
  aggregates do.

> **Review.** Taken: for, a host's arena, and a harness that refuses
> storage; against, holders in a plain `size_t`: a value copied in two
> threads at once races, so values cannot be shared across threads.
> Recommended: as taken for the knobs; holders atomic where C11 or C++11
> atomics exist, behind a knob, plain elsewhere. The matrix kind (S237)
> holds its holders the same way, and the complex and quaternion kinds will
> (S271, S275): one helper for every kind's holders, in step 23, before
> those two are written. Taken in chunk 40: every kind's holders counted by
> `value.h`'s `d_math_boxed_hold` and `d_math_boxed_unhold`, atomically by
> default wherever the compiler can, C89 and C++98 among them, behind the
> knob `D_CFG_MATH_VALUE_ATOMIC_HOLDERS` (S326).

**S135. The table is a static object of a function, `d_math_value_wide_kind`,
and a result is made under the table of the operand of the kind that
handled it.** In C each translation unit still has a copy of its own, as
`value.h` expects (S101), and the header defines no object that a unit
might not use. Taking the operand's table keeps the kind's code free of a
forward declaration, and keeps a value made under one unit's copy under it.
- Alternative: a `static const` object in the header.
- Alternative: every result under the handling unit's own table.

> **Review.** Taken: for, no object a unit might not use, and a value kept
> under the table it was made under; against, none. Recommended: as taken.
> Taken.

**S136. Text is the integer's decimal digits, as `wide.h` writes them, with
no mark of the kind.** How a value is written is the renderer's question
(2b), as S107 left it for the inline kinds.
- Alternative: a suffix such as `w`, which no reader accepts yet.

> **Review.** Taken: for, the writing left to the renderer; against, none.
> Recommended: as taken. Taken.

**S137. The exact arithmetic, the order and the rounding are pure
arithmetic, `D_MATH_FN`; the entries and the constructors, which allocate
or read a box, `D_MATH_FN_RT`.** `math_common.h`'s rule, and the harness
checks the pure part as constant expressions from C++14.
- Alternative: everything `D_MATH_FN_RT`, as no constant expression can make
  a box: the pure part would then be untestable at compile time for no gain.

> **Review.** Taken: for, the pure part checked as constant expressions;
> against, none. Recommended: as taken. Taken.

**S138. `t_value_wide.c` uses the shared reference, at 768 bits, through a
size knob.** `math_tests_exact.h` has exact rationals, Stein's gcd, the
family test and an oracle for the value of a type nearest a ratio: what the
kind's results need. It now takes `D_TESTS_EXACT_LIMBS`, 16 by default, so
its four other includers compile as before, and `t_value_wide.c` sets 48.
This also corrects S124, which said a second limb count would need a second
definition that the macro lint refuses: the lint reads headers, and an
`#ifndef`-guarded default with the count set by the `.c` harness is no
second definition.
- Alternative: `t_wide.c`'s 640-bit naturals moved into a shared header:
  integers only, with no ratio or oracle for ratios.
- Alternative: a third reference.

> **Review.** Taken: for, one reference, widened where one harness needs it;
> against, none. Recommended: as taken. Taken.

**S139. Three kinds of the harness's own below the wide integer, and a
counted copy of its table.** "small" (exact, 150), "blur" (inexact, 130) and
"token" (no number, 120) are the three cases of S133; a refusing small
writes a value anyway, as `value.h`'s contract allows, so a kind that read
it regardless would be seen. A second copy of the wide integer's table,
which counts the calls into it, stands for another translation unit's.
- Alternative: the wide integer checked alone, as no other boxed kind
  exists yet: S133's three cases and S135's tables would go untested.

> **Review.** Taken: for, every path of S133 and S135 checked; against,
> none. Recommended: as taken. Taken.

**S140. The smoke tier, not the full matrix, as in S99, S111 and S126.**
Every harness that includes a changed header ran in full: `t_value_wide`,
and `math_tests_exact.h`'s four other includers, `t_rational`, `t_value`,
`t_value_boxed` and `t_literal` (`raw/includers.txt`). The smoke tier
shows every other test's result unchanged.
- Alternative: the full matrix, about an hour and a half on two cores.

> **Review.** Taken: as S81. Recommended: as taken. Taken.

### S141 to S154: chunk 25, 2026.10.18

The record: `_handoffs/math98_chunk25_2026.10.18/README.md`.

**S141. Two classes: `math::value`, which holds any kind, and
`math::inline_value`, which holds the inline kinds alone.** A value that
may hold a box must give it back when destroyed, and before C++20 a class
whose destructor does anything is no literal type: one class could not both
hold boxes and be a constant expression at C++14 and C++17, where
`value.h`'s inline operations already are. An operation on two inline
values is inline, so `inline_value`'s operations never leave it, and it
converts to `value` implicitly where a box may come.
- Alternative: `math::value` alone, a constant expression from C++20 only:
  `value.h`'s constant expressions lost in C++ at C++14 and C++17.
- Alternative: one class with no destructor, and holds given back by hand,
  as in C: the bookkeeping the face is for.

> **Review.** Taken: for, constant expressions at C++14 and C++17 kept in
> C++; against, two classes to learn, and generic code that must accept
> both. Recommended: as taken; C++20 lets `value` be one class for both,
> which a later step can offer. Taken.

**S142. A value holds its box as an `int` holds its number.** A copy
retains, a destructor releases, copy assignment copies and swaps, and from
C++11 a move takes the hold and leaves none, `noexcept`. Boxes are never
changed once made (S132), so values may share one: a copy needs no storage
and cannot fail. Copy and swap keeps a value assigned itself whole, its
only holder included; `adopt` and `detach` move a hold between C and C++
without a retain or release.
- Alternative: copy assignment that releases the old value before
  retaining the new: a value assigned itself, its only holder, gives its
  box away first (F34 plants it).
- Alternative: a deep copy of the box: storage for every copy, and a copy
  that can fail, to guard a box nothing changes.
- Alternative: no move constructor: a `std::vector` that grows retains and
  releases every value it holds (F37 plants a move that may throw).

> **Review.** Taken: for, cheap copies that cannot fail, self-assignment
> safe, `noexcept` moves; against, shared boxes need atomic holders for
> threads (S134). Recommended: as taken, with S134's atomic holders. Taken.

**S143. Implicit construction from the arithmetic types, `bool`, the error
codes and `d_rational`; a pointer refused.** A value is a number, so `v +
1`, `1 + v` and `x == 0.5` read as they would with a `double`, and step
11's generic code can take literals. Each type has its own constructor, an
exact match, so no call is ambiguous, and `char` and `short` promote to
`int`. A pointer would convert to `bool`, and become a boolean; a deleted
constructor template matches every pointer better, so a pointer does not
convert. `D_DELETED_FN` deletes it from C++11; below, it is private and
never defined, so a C++98 build stops at it as well.
- Alternative: explicit constructors: `v + math::value(1)` everywhere, and
  no number type that generic code can mix with literals.
- Alternative: one template constructor for the arithmetic types: needs
  `enable_if` or its C++98 equivalent, for what the overloads already do at
  every level.

> **Review.** Taken: for, a value mixes with literals as a double does;
> against, implicit conversions can surprise, a `long double` taken as
> itself where a double was meant. Recommended: as taken; a number type must
> convert. Taken.

**S144. `d_math_value_from_uint` in `value.h`, for the unsigned types.** An
unsigned integer past `D_MATH_IMAX_MAX` has no inline integer; G2 says it is
the long double nearest it, flagged inexact. `value.h` builds that from the
exact ratio with a function of its own, which the face could not reach
without calling into the core's internals, so the core gains a public
constructor, 13 lines that call that function, and C callers gain it too.
`t_value.c` checks it against the exact reference at every 2^k - 1, 2^k and
2^k + 1 and 2,000 values drawn at random, and from C++14 as a constant
expression.
- Alternative: the face converting an unsigned past the family to `long
  double` itself: a second rounding path beside the core's, and the flag to
  set by hand.
- Alternative: a range error past `d_math_imax`, which G2 decided against
  for every exact value.

> **Review.** Taken: for, one rounding path, the core's, reachable by C
> callers too; against, none. Recommended: as taken. Taken.

**S145. The operators, `abs` and `swap` are hidden friends, found by
argument-dependent lookup only.** A free `abs(const value&)` in
`djinterp::math` would hide `::abs` and `std::abs` from every unqualified
call in the namespace; a free `compare` would be an overload of
`numeric_compare.hpp`'s `compare` of two texts, so `compare` is a member. A
hidden friend is found only where an argument is its class: `1 + v` finds
`value`'s `+`, and 1 converts. `inline_value` does not convert to `value`'s
friends' other operand the other way, so an `inline_value` with a `value`
has one candidate, `value`'s, and the result is a `value`.
- Alternative: free operators in the namespace: the hiding above, and
  every operator call in the namespace weighing a conversion to a value.
- Alternative: member operators: `1 + v` does not compile, as a member's
  left operand is never converted.

> **Review.** Taken: for, no hiding of `::abs` or `std::abs`, and `1 + v`
> works; against, none. Recommended: as taken. Taken.

**S146. Each comparison is `value.h`'s exact order; `==` is true only where
it is EQUAL; `<=>` is a `std::partial_ordering` from C++20.** `value.h`
orders across kinds exactly, and leaves a NaN, an error and unlike kinds
unordered. The operators follow IEEE 754's comparisons: every one false
where the two are unordered, but `!=`, which is `==` negated, so an error
is not equal to itself, as a NaN is not. `partial_ordering`'s four values
are `value.h`'s four.
- Alternative: a total order, errors and NaNs at one end, as a sort might
  like: a second order beside the core's. A sort can order by `compare`
  with a rule of its own for the unordered.
- Alternative: two errors with the same code equal: an error is no number,
  and `value.h` leaves it unordered with everything.

> **Review.** Taken: for, IEEE 754's rules, the core's exact order,
> `partial_ordering`; against, values cannot be sorted by `<` where NaNs or
> errors are among them. Recommended: as taken; a sort can order by
> `compare`. Taken.

**S147. Conversion out is `to(T& out)`, one overload per type, true where
the value has one of that type, `out` as it was where not.** Nothing
throws, and a value may be no number, so a conversion must be able to say
no; `value.h`'s `to_*` functions say it the same way. One name for every
type lets generic code write `x.to(d)`.
- Alternative: `explicit operator double()`: a double for none or an error,
  where there is none.
- Alternative: `std::optional<T> as<T>()`: C++17 only.
- Alternative: a name per type (`to_double`): what step 11's traits would
  then have to pick among.

> **Review.** Taken: for, a conversion that can say no, one name for every
> type; against, an out parameter. Recommended: as taken. Taken.

**S148. `inline_value` is a constant expression from C++14, `value` from
C++20, and `value`'s destructor releases only a boxed value.**
`D_CONSTEXPR_CPP14` and `D_CONSTEXPR_CPP20` mark the two, as `value.h`'s
`D_MATH_FN` marks its operations from C++14. `d_math_value_release` tests
its argument against `NULL` before the kind, which a constant expression
cannot do with a member of a temporary, so `value`'s destructor tests the
kind first: no constant expression holds a box, as no constant expression
can make one, so it never calls the release.
- Alternative: `value` constant from C++14: not possible, as above (S141).
- Alternative: `value.h`'s release changed for C++20's sake, where a test in
  the face suffices.

> **Review.** Taken: for, the most each level allows; against, none.
> Recommended: as taken. Taken.

**S149. The concept `number`, from C++20, in `value.hpp` until step 11.**
1e named a `number` concept from C++20. It asks what generic code uses:
copyable, `+ - * /` and unary `-` giving the type, `==` and `<` giving a
`bool`. `bool` is excluded by name, since `true + true` is the `int` 2,
which converts back to `bool`, and `value.h` holds a boolean as no number.
Step 11 moves it beside `number_traits`, in `number_traits.hpp`.
- Alternative: the concept with the traits, in step 11: 1e's item names it,
  and its test shows both classes satisfy it.
- Alternative: `std::totally_ordered`: its semantics require a total order,
  which a NaN and an error break.

> **Review.** Taken: for, the concept 1e named, satisfied by both classes;
> against, none. Recommended: as taken. Taken.

**S150. `expression.hpp`'s factory `value(v)` removed, and `math.hpp`
includes `value.hpp`.** The factory, a second name for `constant(v)`, was a
function template `math::value`, which hid the class wherever both were
seen: with the umbrella, `math::value x = 1;` named the function. No code
calls it: every file that includes `expression.hpp`, directly or not, is in
the C++ ladder's directories, whose verdicts are chunk 24's
(`raw/includers.txt`, `raw/ladder_cpp_compare.txt`). `constant(v)` does
what it did.
- Alternative: another name for the class: the roadmap, the C core and
  every document call it the value.
- Alternative: the umbrella left without `value.hpp`: the clash only
  postponed to the first program that includes both.

> **Review.** Taken: for, the class's name kept, the clash removed, nothing
> called it; against, a function removed from another module. Recommended:
> as taken. Taken.

**S151. The face test checks the face against the C core, and has no
Python check of its own.** The face adds no arithmetic. What it can get
wrong is which call it makes, on which operands, with which holds, so the
test runs every operation of the face beside `value.h`'s on the same C
values, over every pair of 57 operands, and counts holds through a counted
table. The C core's harnesses check `value.h` against exact references and
Python's fractions (chunks 18, 22 and 24).
- Alternative: a Python driver through the face: `value.h`'s answers
  checked a second time, and nothing of ownership.

> **Review.** Taken: for, the face checked where it can go wrong, ownership
> included; against, none. Recommended: as taken. Taken.

**S152. Nothing committed; the record's checks run on copies of the working
tree.** The owner's instruction of 2026.10.18. Each run of
`verify_queue_c25.sh` copies the tree, without `.git` and `_handoffs`, and
checks the copy. `files_zip_c25.py` takes what the working tree has, git's
files changed and new ones not ignored, against the base;
`zip_check_c25.sh` compares a base with the zip over it to the working
tree. Chunk 16's tools took two commits.
- Alternative: commits on the lane's branch, as before: what the owner
  stopped.

> **Review.** Taken: the owner's ruling. Recommended: as taken. Taken.

**S153. The smoke tier, not the full matrix, as in S99, S111, S126 and
S140; every test that includes a changed header, directly or not, in
full.** `include_closure_c25.py` follows the includes
(`raw/includers.txt`): `value.h` reaches six tests, `t_value_face` and the
five of step 3, all run in full, 192 builds; `value.hpp` reaches
`t_value_face` alone; `expression.hpp`, which lost a function no one
called, reaches `t_constants_face`, in the smoke tier, and ten headers, all
on the C++ ladder. The smoke tier shows every other test's result
unchanged.
- Alternative: the full matrix, about an hour and a half on two cores.

> **Review.** Taken: for, every includer of a changed header run in full,
> the rest in the smoke tier; against, none. Recommended: as taken. Taken.

**S154. Each operation makes `value.h`'s result in place.** Two private
constructor templates in each class take a type that names one of
`value.h`'s operations, with its operands, and initialize the value's member
from the call, so the result is made where the value keeps it, with no copy
between. The first version took the result through the constructor that
adopts a C value, and g++ copied it once more: 19 instructions more in each
sum at C++20, of either class and either kind, and 19 to 21 at ISO strict
C++98, as callgrind counts them, a count no machine's speed changes
(`adopted_c25.sh`, `raw/adopted.txt`). What the copy costs in time depends
on the machine. On the queue's, at C++20, the first version's integer sum
took 36.5 ns against `value.h`'s 23.5 in the same run
(`raw/timing_adopting.txt`), where in place it takes 22.0 against 19.6
(`raw/timing.txt`). On a later machine, the two versions run in turn, it
took 48.0 ns against 45.9 in place, and the wide sum 245.2 against 228.1,
though `value.h`'s own wide sum, the same code in both builds, differed by
8.7 ns between them (section 8). In place, the face costs what `value.h`'s
call does on both. `value`'s assignment forms still copy and swap, around
a result made in place: they count 7 instructions more than the first
version's at C++20 and 17 fewer at ISO strict C++98, and take the same time
within 0.2 ns at C++20 and 3.7 ns less at ISO strict C++98 `-m32`. The
templates stand at the top of each class, above the operators that use
them: clang reads a template's body only where it stands, and an
instantiation it made before then is no constant expression.
- Alternative: the operation as a pointer to `value.h`'s function, tried
  first: `value.h`'s functions are always_inline outside testing builds,
  and at `-O1` with the sanitizers g++ could not inline a call through the
  pointer, an error (`raw/pointer_sanitizers.txt`), though `-O0` and `-O2`
  built.
- Alternative: the adopting constructor taking the C value by value: the
  parameter is made in place, then copied into the member in its turn.
- Alternative: a private constructor per operation, each with a tag:
  twelve constructors where four do.

> **Review.** Taken: for, the face costs what the C call costs; against, a
> subtler construction. Recommended: as taken. Taken.

### S155 to S166: chunk 26, 2026.10.18

The record: `_handoffs/math98_chunk26_2026.10.18/README.md`.

**S155. `number_traits<T>`, static functions in a class template, says
what generic code needs of a number type.** It is the one place a template
asks for a type's zero, root or tolerance, and a type joins by specializing
it, as a bignum would. Its flags take `std::numeric_limits`'s names where
they mean the same. Its members are functions, not constants: at C++98 a
static constant in a class must be integral, and a value's zero is not.
- Alternative: `std::numeric_limits` specialized for the value classes: it
  says nothing of roots or conversions, and most of its members, `max()`,
  `epsilon()`, mean nothing for a type that holds any kind.
- Alternative: free functions found by argument-dependent lookup,
  `zero(x)`, `sqrt(x)`: the arithmetic types' would live in
  `djinterp::math` and hide `::sqrt`, as S145 found for `abs`.

> **Review.** Taken: for, one place for what generic code asks, open to any
> type; against, a trait class where C++20 has concepts. Recommended: as
> taken. Taken.

**S156. The primary template serves any type as the generic path always
took one.** Made from an `int`, through `double`, ordered by `<`, its
tolerance 1e-12 cast to the type, `is_specialized` false. Every element
type the linear-algebra headers took before still compiles, and computes as
it did. An integral type it serves, a character type or `bool`, it calls
exact and an integer, as the language does, so `inverse` refuses a matrix
of `char` as it did (F3 plants the fault).
- Alternative: no primary definition, so a type without traits stops the
  build: every user type that worked before would stop.
- Alternative: traits of their own for `char` and the other character
  types: types no one computes with as numbers, which the language already
  says are integers.

> **Review.** Taken: for, every element type that compiled before still
> computes as it did; against, a user's type without traits has its roots
> and tolerances taken through `double` silently, an accuracy loss a
> high-precision type would not expect. Recommended: as taken for
> compatibility, with `is_specialized` false reported where an exact or
> precise answer is asked (a static assertion in `inverse` and the roots,
> say). Taken in part.

**S157. The arithmetic types' traits: an integer type exact, its root the
double's truncated, its tolerance 0; a floating-point type inexact, its own
correctly rounded root, its tolerance 1e-12 in the type.** The integer
types keep what the generic path computed. The floating-point types take
the C core's root, as their C kernels already did: `d_math_sqrt_c` in a
constant expression, from C++14, and the C library's at run time, which
the C core keeps the same bit for bit.
- Alternative: an integer's root rounded to nearest: a change to what the
  generic path gave (F4 plants it).
- Alternative: a `long double`'s root through `double`: two roundings, and
  a different answer (F7 plants it).

> **Review.** Taken: for, the C core's correctly rounded roots for the
> floating-point types; against, two weaknesses: an integer's root was the
> double's truncated, off by one past 2^53, where an exact integer square
> root is cheap; and the tolerance was an absolute 1e-12 in every type,
> below a float's resolution (as S46 found for the eigenvalues) and blind to
> the operands' scale. Recommended: an exact integer square root, and a
> relative tolerance of a few epsilons of the type times the operands'
> scale, as S46 chose for the eigenvalues. Taken in chunk 38: the integer
> root exact, rounded down, and the tolerance four epsilons relative to the
> larger magnitude (S303, S304).

**S158. The value classes' traits: inexact; the root exact for the square
of a rational of the family, and the long double's otherwise.** A value
may hold a float or G2's long double, so its arithmetic is exact only where
its operands are, and `is_exact` says no. A root is exact where it can be,
so a vector of values has an exact norm and an exact unit vector wherever
they are rationals; elsewhere the long double's root, as the floating-point
types have theirs. An exact negative number has no root, a domain error, as
the C library reports one for `sqrt`, a wide integer past the family among
them, which reads as no rational; an inexact one's is a NaN. The tolerance
is the double 1e-12: a value compares as a double does.
- Alternative: `is_exact` true: generic code that trusts it would compare
  values holding doubles with no tolerance.
- Alternative: every root through the long double: the norm of (3, 4) a
  long double 5, and no exact unit vector (F12 plants it).
- Alternative: a negative number's root a NaN wherever it reads as no
  rational: a negative wide integer's root no error (F14 plants it).
- Alternative: a tolerance of the integer 0: approximate comparisons of
  values that hold doubles would ask for equality (F16 plants it).

> **Review.** Taken: for, exact roots wherever the answer is rational;
> against, the same absolute 1e-12 for every value, exact ones included,
> which compared exact rationals with a tolerance they do not need.
> Recommended: tolerance 0 where both operands are exact, and S157's
> relative tolerance otherwise. Taken in chunk 38: a pair's tolerance 0
> where both are exact, else four of the larger epsilon times the larger
> magnitude (S303).

**S159. `common_number<A, B>` is the type of `a + b`, from C++98.** For
two arithmetic types C++98 tells it through `sizeof`, which evaluates
nothing, from the overload of a function `a + b` would call; from C++11 any
other pair through `decltype`; the value classes say theirs at every level.
Where there is none, there is no `type`, so a use stops the build where it
is, not inside the template that used it.
- Alternative: `std::common_type`: C++11, and the conditional operator's
  type, not the sum's: `bool` for two `bool`s, whose sum is an `int`.
- Alternative: `decltype` alone: nothing below C++11, where the lane's
  rungs start.

> **Review.** Taken: for, the sum's type, from C++98; against, none.
> Recommended: as taken. Taken.

**S160. The concept `number` moves to `number_traits.hpp`, unchanged.** As
S149 planned: the concept and the traits are the static side together, and
`value.hpp` includes them.
- Alternative: the concept left in `value.hpp`: a template that asks for a
  number would include the value classes to name it.

> **Review.** Taken: as planned. Recommended: as taken. Taken.

**S161. Linear algebra's generic path takes its constants, roots,
conversions and tolerances from `number_traits`.** The generic path is
every element type's but the three floating-point types'. Its casts of 0,
1, 2 and 0.9995, and its roots through `double`, gave a value the right
constants but an inexact root, and a type with no conversion from `int` or
to `double` none at all. Through the traits, each type computes in its own
way, and the types that had no traits before compute as they did.
- Alternative: a second generic path for the value classes: two paths to
  keep in step.
- Alternative: the default tolerance left as 1e-12 cast to the type: a type
  whose author chose another, as the test's rational does, would not get
  it (F22 plants it).

> **Review.** Taken: for, each type computes in its own way, through one
> path; against, none. Recommended: as taken. Taken.

**S162. A vector or matrix made by default is zero in any number type.**
An arithmetic type's components are value-initialized, as before, a
constant expression from C++11; any other type's are set to its traits'
zero. A value made by default is none, so a vector of values made by
default would otherwise hold none, and `vector()` would not be `zeros()`.
From C++11 two default constructors, chosen by the element type, keep the
arithmetic types' a constant expression from C++11; at C++98, where
nothing is a constant expression, one constructor tests the type.
- Alternative: one constructor that sets every type's zero: the arithmetic
  types' default construction a constant expression from C++14 only, a
  loop in its body.
- Alternative: none by default for the value classes: every sum with a
  vector made by default none, an error component by component (F23 to F26
  plant it).

> **Review.** Taken: for, `vector()` zero in every type, a constant
> expression from C++11 for the arithmetic types; against, two constructors.
> Recommended: as taken. Taken.

**S163. A vector's components may be the element type's own values.** The
component constructors took arithmetic types alone. They now take the
element type too, so a vector of values is written with values, a wide
integer among them, and a vector of a user's rational with its rationals.
Any other type stays out, so no component constructor is taken for a copy,
or for construction from an array.
- Alternative: any type that converts to the element type: a vector of
  values would take a vector of doubles as one component, and C++98 has no
  standard conversion test.

> **Review.** Taken: for, vectors written in their own values; against,
> none. Recommended: as taken. Taken.

**S164. `inverse` takes any element type that `number_traits` does not
call an integer type.** It took floating-point types alone. A value's
inverse is exact where its entries are rationals of the family, and so is a
user's exact rational's; an integer matrix has no integer inverse in
general, and stays refused, by static assertion.
- Alternative: floating-point types and the value classes only: a user's
  exact type refused for no reason.

> **Review.** Taken: for, exact inverses for exact types; against, none.
> Recommended: as taken. Taken.

**S165. `D_MATH_FN_ENTRY`, for a function a table points to: a header
inline no compiler is told it must inline.** A boxed kind's entries are
reached through the kind's table, so a forced inline gains nothing there,
and at gcc's `-Og` stops the build. In C the qualifier is a `static`
`D_ENV_COMPILER_INLINE`, so each translation unit has its own copy, as
`D_INLINE` gives; in C++ an `inline` function, so the copies are one.
- Alternative: `D_MATH_FN_RT` never forced: every run-time kernel's
  inlining changed, for a fault in table entries alone.
- Alternative: no change: no program that includes `value_wide.h` builds at
  `-Og`.

> **Review.** Taken: for, tables build at every level; against, none.
> Recommended: as taken. Taken.

**S166. The full tier, in four parts, and every test's lines against its
last full run.** `math_common.h` is included by every C core header, so
every test includes a changed header, and runs in full, as S153 has it.
The tier runs as four parts, the harnesses and then the face tests ten at
a time, so that the queue can stop and start between runs: a background
command here may run two hours, and the harnesses alone took 85 minutes.
The parts' results are joined, line for line, as one run's would be, and
each test's lines are compared with its last full run in an earlier
record, so that a count that moved shows, not only a failure.
- Alternative: the smoke tier: the full tier is what S153 asks of a test
  that includes a changed header.
- Alternative: one run of the whole tier: past the two hours on this
  machine.

> **Review.** Taken: for, the full tier within two hours a part, counts
> compared line by line; against, none. Recommended: as taken. Taken.

### S167 to S175: chunk 27, 2026.10.18

The record: `_handoffs/math98_chunk27_2026.10.18/README.md`.

**S167. The five defects fixed where they are, each by the least change
that makes its part compile once used, and nothing else of the module
changed.** The module is parse's, and its author's design stands: the
term an alias of `mu`, its folds `cata`'s, its comments and its style.
- Alternative: rework the term so that OpId and Atom are deduced, a class
  deriving from `mu` in place of the alias: every caller simpler, and a
  change to the design of a module other lanes use (`core/util/document/
  layout/` builds on it).

> **Review.** Taken: for, another lane's module and design respected;
> against, its weaknesses stay: callers name OpId and Atom, and its folds
> stop on a term made by default inside a term (chunks 32 to 34's finding).
> Recommended: as taken here, with those two fixes proposed to the module's
> owner, the guard first. Taken; the guard is still open.

**S168. Callers keep naming OpId and Atom; the module's internal calls now
do too.** The alias makes them undeducible for every caller:
`make_expression_grammar` and `reify` already ask for them by name, and
every other function needs them the same way.
- Alternative: overloads over `mu<F>` that recover OpId and Atom from F: a
  template template parameter matched against a member alias template,
  which compilers treat differently.

> **Review.** Taken: follows from S167. Recommended: as taken. Taken.

**S169. `static_expr`'s constructor `D_CONSTEXPR`, as its nodes'.** A
testing build keeps `constexpr` unless `D_CFG_TESTING_STRIP_CONSTEXPR` is
set, and then strips every other `constexpr` of the module with it.
- Alternative: `D_CONSTEXPR_REQ`, kept even where testing strips: the
  nodes' constructors would still be stripped, so nothing gained.

> **Review.** Taken: for, the constructor stripped where the module's others
> are; against, none. Recommended: as taken. Taken.

**S170. A nullary fold's leaf handler marked used with a cast.**
- Alternative: an overload of `static_evaluate_apply` for an empty operand
  pack: a second definition of the fold for one case.

> **Review.** Taken: for, one definition of the fold; against, none.
> Recommended: as taken. Taken.

**S171. The test is a face test in `tests/djinterp/math/`, where the
runner builds it in the 64-build matrix.** The module is the base of
math's expression language (G4), and every later item of Phase 2 adds to
what it checks. Moving it is one `git mv`.
- Alternative: beside parse's own tests, `tests/djinterp/parse/`, which no
  runner of the lane's builds.

> **Review.** Taken: for, the test built where the runner builds; against,
> away from parse's own tests. Recommended: as taken. Taken.

**S172. The test's language tells a unary minus from a binary one in its
tokenizer.** The parser asks `op_of` of a token alone, so one spelling for
two operators has to be told apart before it. Chunk 12's probe kept state
in its recognizers instead, set as atoms and brackets are read; nothing
resets it when an operator is consumed, so a minus after a binary operator
reads as binary there. 2b decides the same for math, in a tokenizer or in
its parser layer.

> **Review.** Taken: a finding for 2b, which 2b settled in its parser layer.
> Nothing to recommend now.

**S173. Three behaviours the module's comments say otherwise of are kept,
and recorded (section 6).** P0 fixes what stops the module compiling once
used. What a nullary operator, a chain of non-associative operators or a
literal's static leaf should do is its design, and Phase 2 meets each: a
constant is an atom in math (Phase 2's design), and 2d decides what
`a = b = c` means.
- Alternative: change them here: the parser taking a nullary operator
  where an expression begins, refusing `a = b = c`, and
  `make_static_leaf` decaying `const Atom`: three changes of behaviour in a
  module other lanes use, none needed by step 13.

> **Review.** Taken: for, another lane's module changed only where it had to
> be; against, its stated and actual behaviours differ until a step meets
> each. Recommended: as taken; 2d met the chains. Taken.

**S174. The smoke tier, and the new test in full, not the full matrix.**
No harness and no other face test includes a changed header
(`raw/includers.txt`), as S153 asks: 114 smoke builds, every earlier line
against chunk 26's full tier, and the test's 64 builds.

> **Review.** Taken: as S153. Recommended: as taken. Taken.

**S175. The C++ ladder over the module, every includer of the changed
headers and the test, against a base run of the same units at chunk 26's
delivery; and the C ladder whole, though no C file changed.** The
includers, nine headers of `core/util/document/layout/`, `report_*` and
`test/output/`, are on no list of the lane's, and the ladder compiles each
alone.

> **Review.** Taken: for, every includer compiled, the C ladder whole;
> against, none. Recommended: as taken. Taken.

### S176 to S188: chunk 28, 2026.10.23

The record: `_handoffs/math98_chunk28_2026.10.23/README.md`.

**S176. The lane's tree is now the owner's tree of 2026.10.23 with the
lane's files laid over it: every file the lane changed alone since
`ef7c81f`, the tree both last shared. Where both changed a file the owner's
version stands, as each already holds the lane's change in his own form:
`c/djinterp.h` and re_std's `reverse_iterator.hpp`. A file the lane deleted
and he kept stays.** His message sends the tree as the framework to continue
on, and a delivery he can lay over it needs the tree it was checked on.
- Alternative: keep the lane's repository as the tree, and bring in from
  his only the framework files the lane builds on (the root, env, re_std,
  config): a smaller change, on a tree he does not have.

> **Review.** Taken: the owner's direction, his tree as the base; for, a
> delivery checked on the tree he applies it to; against, none. Recommended:
> as taken. Taken.

**S177. The delivery carries every file of the lane's since `ef7c81f`, not
since chunk 15, and the records of chunks 16 to 28.** His tree has none of
the lane's files, so a zip of chunk 28's changes alone, or of chunks 16 to
28's, would leave it without the C core. The same zip serves a lane tree
from chunk 15 on, where the older files are already as they are here.
- Alternative: two zips, one for each kind of tree: twice the checking, and
  a choice for him to make before he can apply either.

> **Review.** Taken: for, one zip for every tree he may have; against, a
> larger zip each time. Recommended: as taken. Taken.

**S178. Sixteen headers under `math/expr/`: one for each part of the six
layers, `matrix.hpp` for Phase 3, one for each item of Phase 4, the common
part, and the handle as the umbrella** (section 2.2's table).
- Alternative: one header a layer, Phases 3 and 4 inside the folds and the
  rewrites: six files, each filled by many steps.

> **Review.** Taken: for, each header one part, filled by one step; against,
> sixteen files. Recommended: as taken. Taken.

**S179. Everything in `djinterp::math`, as every subdirectory of `math/`
is; the term-level vocabulary named `expr_...`, and the handle and its
entries by the roadmap's provisional names, `expr`, `parse`, `try_parse`
and `at`.** The prefix keeps the run-time names apart from the compile-time
language's in the same namespace (`expression.hpp`, `function.hpp`) and
from `parse/expression/`'s in `djinterp` (`evaluate`, `render`,
`substitute`, `expr_leaf`), which a name of math's own would hide.
- Alternative: a namespace of its own for the term level, `math::rt`: no
  prefix, and one more namespace to name.

> **Review.** Taken: for, names that never hide the shared core's or the
> compile-time language's; against, a long prefix. Recommended: as taken.
> Taken.

**S180. The term: its OpId `expr_op`, a kind and, for a call, its
function's name; its Atom `expr_atom`, a number held as a `math::value` with
the literal it was written as, or a variable or a named constant by name.
A function is an operator, so every fold, every rewrite and the shared
renderer take a call as any application.** The literal lets each typed fold
round it once, from its text (G3), where a value too long for the family is
already a rounded long double.
- Alternative: one `call` operator whose first child names the function:
  every fold and rewrite would have to step around that child, and the
  shared renderer could not print the call.

> **Review.** Taken: for, one term shape for every fold and rewrite, each
> literal rounded once from its text in a type (G3); against, a call's
> function in the operator, which a table cannot describe by kind.
> Recommended: as taken. Taken.

**S181. In 2a the types and data are whole, and every entry is a stub. One
that can fail returns `expr_result<T>`, and its stub the error `not_yet`
with a message naming the step that fills it; one that cannot gives the
placeholder its contract names: the empty string (`expr_render`,
`expr::str`), the term unchanged (`expr_substitute`, `expr_simplify`).**
`expr_is_matrix` answers false, which is true until 3a makes the kinds.
- Alternative: declarations without definitions: nothing a test can call,
  and a link error where anything does.

> **Review.** Taken: for, every entry callable and testable from the start;
> against, stubs to keep in step with their steps. Recommended: as taken.
> Taken.

**S182. Every step that can fail returns `result<T, expr_error>` (principle
5), the folds included; the handle holds a term or an error, and each
member passes an error on, as arithmetic passes an error value; `try_parse`
gives a result, for a caller who branches.**
- Alternative: the handle's members return results, chained by
  `and_then`: the roadmap's `f.diff("x").simplify().str()` would not read
  as it does.

> **Review.** Taken: for, chained members that read as the roadmap writes
> them, errors carried to the end; against, the error is seen only at the
> chain's end. Recommended: as taken, with `try_parse` for a caller who
> branches early. Taken.

**S183. The typed fold and Phase 4's numeric entries take `float`, `double`
and `long double` alone, by static assertion; the fold to a value takes an
`expr_context`, which has no field until a kind reads one,** as S70 left
`value.h`.
- Alternative: any number type through `number_traits`: the C core's
  kernels serve the three, and `math::value` has its own fold.

> **Review.** Taken: for, the C core's kernels behind every typed fold;
> against, no fold over other number types, an interval type above all,
> whose fold would give a term's value with guaranteed bounds. Recommended:
> as taken now, and a fold over any type `number_traits` describes later,
> intervals first. Taken.

**S184. The statuses are `not_yet` and the eleven the declared contracts
name, each marked with the step that first gives it.**
- Alternative: `not_yet` alone, each step adding its own: the contracts of
  2a would name statuses that do not exist.

> **Review.** Taken: for, contracts that name real statuses from the start;
> against, none. Recommended: as taken. Taken.

**S185. The test is `tests/djinterp/math/expr/t_expr_face.cpp`, in the
64-build matrix: every type, every stub's answer, and each entry's type by
static assertion.** Each later step turns its stubs' checks into checks of
what it fills.

> **Review.** Taken: for, every entry checked from 2a on; against, none.
> Recommended: as taken. Taken.

**S186. The full tier on the tree before the change, which is the lane on
the owner's framework; after it, the smoke tier and the new test in full.**
Chunk 28 changes no header an earlier test includes (S153), and the
integration changes many of the files every test includes.

> **Review.** Taken: as S153. Recommended: as taken. Taken.

**S187. The C ladder whole on the new tree, every unit's failures against
the lane's list; the C++ ladder over the math directories and
`parse/expression/`, against the same over the lane's tree at chunk 27; and
the lints on the new tree against his tree with the repository's tools.**
The units of his tree outside the lane's are his; the comparisons show what
the lane's files change.

> **Review.** Taken: for, the lane's effect shown against his tree; against,
> none. Recommended: as taken. Taken.

**S188. `math.hpp` does not include `math/expr/` yet.** 5e makes the
umbrella, where the compile-time and run-time languages meet.
- Alternative: include `expr/expr.hpp` from `math.hpp` now, as `value.hpp`
  and `number_traits.hpp` were: every includer of `math.hpp` would read
  `parse/expression/` and `core/functional/`.

> **Review.** Taken: for, includers of `math.hpp` not made to read the
> run-time language; against, the umbrella is incomplete until 5e.
> Recommended: as taken. Taken.

### S189 to S200: chunk 29, 2026.10.23

The record: `_handoffs/math98_chunk29_2026.10.23/README.md`.

**S189. Numbers by `decimal.h`'s grammar: digits, a point, an exponent,
from a digit or a point before one, never a sign nor a fraction.** A sign
is an operator, and `2/3^2` must be a division, 2/9.
- Alternative: `literal.h`'s grammar, fractions and mixed numbers among it:
  `2/3^2` would be (2/3)^2 and `1 1/2` one number.

> **Review.** Taken: for, `2/3^2` a division, as every language reads it;
> against, fractions not literals. Recommended: as taken. Taken.

**S190. The parser layer is a pass that gives each token its role before
`parse/expression`'s climber sees it, then a check of the brackets, then the
climb; its errors are mapped to the statuses at their characters.** The
shared parser cannot see where a minus stands (chunk 12's probe), and counts
tokens, not characters (chunk 27).
- Alternative: a climber of the lane's own, with errors of its own: gate G4
  builds the language on the shared parser.

> **Review.** Taken: for, gate G4's shared parser kept; against, the parser
> layer now does most of the work itself (roles, groups, chains, errors by
> character), and the shared climber's limits leak: groups put back as
> atoms, a postfix operator with operands awkward to express, folds that
> stop on a term made by default. 3b's indexing went through it (S252), as a
> postfix operator of a table the parser layer adds, its indices put back
> after the climb. The alternative, a precedence parser of the language's
> own over the same term type, would be simpler and more capable, the term
> and its folds still shared. Recommended: the alternative, before Phase 4's
> rewrites and any further operator make the layer larger. Ruled 2026.11.04:
> decided on 2026.11.06 with lane F against expression core v2's design:
> core v2 if its parser holds postfix operators and calls, S209 and S252
> then following it; otherwise the language's own precedence parser, as
> S190's review recommends. **Flag:** not the recommendation; a growing
> cost.

**S191. G5's precedences as the table's, ten apart: relations 10, `or` 20,
`and` 30, `not` 40, `+ -` 50, `* / mod` 60, the unary `-` and `+` 70, `^`
80, the postfix 90; `^` to the right; `+` and `*` commutative and
associative.** The order is G5's, the roadmap's recommendation, which the
lane takes as the owner has not said otherwise: so `-x ^ 2` is -(x²). The
numbers are the lane's: ten apart leaves room for a level between two.
- Alternative: the levels 1 to 9: a level between two would renumber every
  level above it.

> **Review.** Taken: for, G5's order, the unary minus below `^` as in
> mathematics, room between levels; against, none. Recommended: as taken
> (with S213). Taken.

**S192. `a mod b` is the floored remainder, with `b`'s sign, as Python's
`%`, MATLAB's `mod` and Mathematica's `Mod`.** It is what "mod" means in
mathematics: `-7 mod 3` is 2.
- Alternative: C's `fmod`, the truncated remainder with `a`'s sign, as
  MATLAB's `rem`: `-7 mod 3` would be -1.

> **Review.** Taken: for, "mod" as mathematics and Python mean it; against,
> differs from C's `fmod`. Recommended: as taken. Taken.

**S193. `^` and `mod` on values follow value.h's tower: errors pass, the
first; a boolean or none is a domain error; exact where the result can be,
`^` for whole exponents and for rational ones of whole roots, `mod` for
exact operands; a negative exact base to a power not whole a domain error,
exact 0 to a negative power a pole; otherwise the C core's function in the
tower's kind, the operands' flags carried.** value.h has neither, and the
fold to a value is "exact where possible" (the roadmap).
- Alternative: `^` exact for whole exponents alone: `4 ^ 0.5` would be the
  long double 2.

> **Review.** Taken: for, exact wherever the result can be; against, none.
> Recommended: as taken. Taken.

**S194. The typed fold computes `^` and `mod` with the C core's plain
forms, the C library's, as the roadmap's "typed and fast" asks.**
- Alternative: the `_c` forms, correctly rounded and the same in constant
  evaluation: slower, and the typed fold is no constant expression.

> **Review.** Taken: for, the typed fold fast; against, its bits are the
> platform library's, so one term in double can differ by an ulp between
> platforms. Recommended: as taken, plus a reproducible mode, the `_c` forms
> in the typed fold, for callers who want the same bits everywhere. Taken in
> part.

**S195. A number a fold made is written exactly where exact, as the
shortest decimal that reads back in its type where floating, and by name
where it has no literal.** 0.1 as a `double` is written `0.1`, and reads back
as that `double` in `double`.
- Alternative: a floating number's exact decimal expansion, which reads back
  as the same number as a value too: `0.1` as a `double` would take 55
  digits.

> **Review.** Taken: for, the shortest text that reads back, as Python and
> JavaScript write; against, a double read back as a value is the exact
> decimal, not the double (it reads back the same in its type). Recommended:
> as taken. Taken.

**S196. Implied multiplication, where on, is a `*` between an operand's end
and the next one's start, as tight as `*`; never between two numbers, nor
before a call's bracket.** `1/2x` is (1/2) x, as in Mathematica.
- Alternative: implied multiplication tighter than `/`, as some calculators
  have it: `1/2x` would be 1/(2x).

> **Review.** Taken: for, Mathematica's reading, `1/2x` as (1/2) x; against,
> some calculators read 1/(2x). Recommended: as taken; implied
> multiplication is off by default. Taken.

**S197. What a later step brings answers `not_yet` with its step, at its
place where it has one; `and`, `or` and `not` are kept from variables now,
so that a text means the same before 2d and after.** A constant's name is
another matter: `pi` and `e` are variables until 2c, which fills the
constant table and so decides which names are constants.
- Alternative: such names variables until their step: `and` would be a
  variable now and an operator after 2d.

> **Review.** Taken: for, a text means the same before and after each step;
> against, none. Recommended: as taken. Taken.

**S198. The face test grows in place: each stub check of 2b's entries
becomes a check of what it does; random terms round-trip; random whole
terms check the typed folds against the exact one.** S185 has each step
turn its stubs' checks into checks of what it fills.
- Alternative: a test of 2b's own beside it: 64 builds more in the full
  tier, and the stub checks it replaces left to delete.

> **Review.** Taken: as S185. Recommended: as taken. Taken.

**S199. The checks: the new test in full and the smoke tier, as no older
test includes `math/expr/` (S153); the C ladder whole, and the C++ ladder
over `math/expr/` and its test, against chunk 28's delivered tree; the
lints; 142 planted faults.**
- Alternative: the full tier again, 2,328 builds and two hours, of which
  only the new test's 64 can tell this change from chunk 28's.

> **Review.** Taken: as S153. Recommended: as taken. Taken.

**S200. Three statuses widened: `wrong_arity` for an operator applied to a
number of operands it does not take, `unknown_name` for an operator of no
kind, `missing_operand` for a term made by default.** The folds meet such
terms only where a caller made them, and must answer something.
- Alternative: new statuses for malformed terms: twelve name every way a
  step fails today (S184).

> **Review.** Taken: for, twelve statuses name every way a step fails;
> against, a status covers both text and term errors. Recommended: as taken.
> Taken.

### S201 to S212: chunk 30, 2026.10.23

The record: `_handoffs/math98_chunk30_2026.10.23/README.md`.

**S201. A call is a matter of syntax: a name before an open round bracket
is a call of the function of that name, whatever the name, and a fold looks
the function up when it meets the call.** The parser needs no function
table, so a text means the same with any table; a name no table has is the
fold's `unknown_name`, at the call. With implied multiplication on, `x(y)`
is therefore a call, never x·y, as S196 has it; `2(y)` and `(x)(y)` are
products. A function's name alone, `sin`, is a variable.
- Alternative: the parser looks each name up, and a name before a bracket
  that no function has is a product or an error: a text's meaning would
  change with the table, and an unknown function would be found before its
  arguments are parsed.

> **Review.** Taken: for, a text's meaning independent of any table,
> undefined functions kept as symbols, as computer algebra systems keep
> them; against, a typo is found only at the fold. Recommended: as taken.
> Taken.

**S202. The constants are a table, like the functions, and a name it has is
a constant at parse; their values are the 40-digit literals of
`constants::of<T>`, written in `function.hpp` rather than taken from
`constants.hpp`.** From C++14, `constants.hpp` includes
`math/expression.hpp`, whose `x`, `y`, `z`, `t`, `r`, `rho`, `theta` and
`phi` stand in `djinterp::math`: with it, every test and caller that names
a variable `x` meets `-Wshadow` errors, as the lane's first build did. The
test checks each type's value against `of<T>`'s.
- Alternative: include `constants.hpp`: one source of the digits, and
  those names beside this language's from C++14.

> **Review.** Taken: for, no names leaked into the language from
> `constants.hpp`; against, two sources of the digits, checked against each
> other by the test. Recommended: as taken, and the cause removed:
> `math/expression.hpp`'s variables `x`, `y`, `z` and the rest moved into a
> namespace of their own (chunk 30's finding). Taken in part.

**S203. The standard functions are the C core's 25, each of the number of
arguments C gives it and named as in C, with `abs` and `factorial`; `log` is
the natural logarithm.** They are what the C core has kernels for in every
type, and G5 names `log` natural.
- Alternative: more names (`ln`, `lg`, `sign`, `min`, `max`, `gamma`):
  each a decision of its own, and `gamma` a kernel the C core has not.

> **Review.** Taken: for, every function a C core kernel in every type;
> against, a narrower set than other systems': no `min`, `max`, `sign`,
> `gamma`, `erf`, inverse hyperbolic functions, `log1p` or `expm1`. Chunk 37
> put `log1p`, `expm1` and the inverse hyperbolic functions in the C core,
> in both forms and every family (S286); the table has none of them yet.
> Recommended: those five in the table in step 23 (S271), and `min`, `max`
> and `sign` with them (exact, no kernel needed), sign(z) being z / |z| of a
> complex z; `gamma`, `erf` and the rest with their kernels; chunk 40 placed
> the eight in step 23's second part, chunk 41 (S324), which put them in the
> table (S338), each with its complex value but the next chunk's two (S335);
> `gamma`, `erf` and the rest wait for their kernels, which no step has yet.
> Ruled 2026.11.04: `gamma`, `erf` and the rest wait for their kernels, the
> lane to place them: at step 36 (6a), `gamma`, `lgamma`, `erf` and `erfc`,
> with the correctly rounded forms (S379). **Flag:** functionality gap, now
> `gamma`, `erf` and the rest alone.

**S204. In a type, a function is the C core's plain form, as `^` is
(S194); `factorial(n)` the product rounded as it is made, for a whole n ≥ 0,
and a NaN for any other n.** The typed folds are the fast ones.
- Alternative: the `_c` forms, correctly rounded and the same in constant
  evaluation; and for the factorial, Γ(n + 1), which the C core has not.

> **Review.** Taken: for, the fast plain forms; against, the factorial of a
> non-integer a NaN, where other systems give Gamma(n + 1). Recommended: as
> taken, Gamma with its kernel. Taken.

**S205. As a value, a function is exact wherever its value is a rational
number of the family; an exact argument outside its domain is a domain
error, and the logarithm of 0 a pole; elsewhere its value is the C core's
in the tower's kind, its arguments' flags carried.** Exact where possible
(the roadmap), and by Lindemann and Weierstrass the transcendental
functions are rational at their one point each and nowhere else, so the
rule loses no exact value. The errors are value.h's, as `^`'s are (S193).
- Alternative: exact only where the result is plainly exact (`abs`, the
  roundings, the remainders): `sqrt(4)` would be the long double 2.

> **Review.** Taken: for, exact wherever the value is rational, which by
> Lindemann and Weierstrass loses no exact value; against, none.
> Recommended: as taken. Taken.

**S206. The factorial is postfix at G5's postfix level, 90, tighter than
`^`, and to the left.** So `2 ^ 3!` is 2⁶, `-3!` is -(3!), and `n!!` is
(n!)!, not the double factorial.
- Alternative: `!!` as the double factorial, a token of its own: a
  function the C core has not, and `n!!` would mean two things.

> **Review.** Taken: for, one meaning for `!`; against, Mathematica reads
> `n!!` as the double factorial. Recommended: as taken, documented. Taken.

**S207. With G5's bars on, `|x|` is the call `abs(x)`: a bar opens where no
operand ends before it or where a bar is not the innermost group open, and
closes after an operand where one is.** `||x| - 1|` and `|x| |y|` read as
written, and `|x|` round-trips as `abs(x)`.
- Alternative: bars as a bracket of the climber's: `parse/expression`'s
  climber knows one kind of bracket, whose open and close differ, and a
  bar is both.

> **Review.** Taken: for, bars read as written, round-tripping as `abs`;
> against, an option that changes the grammar. Recommended: as taken. Taken.

**S208. The renderer describes a call to `parse/expression`'s renderer as
an operator of its own, spelled as its function, through an adapter over
the table.** The renderer writes an operator it has no description of as
`?(a, b)`, and the table describes kinds: every call is one kind, of many
names.
- Alternative: a renderer of the lane's own for calls: two renderers to
  keep in step, with the brackets each needs.

> **Review.** Taken: for, one renderer; against, an adapter to keep.
> Recommended: as taken. Taken.

**S209. The parser layer reads each call and each pair of bars as one
operand before the climb, an atom standing in its place, and puts it back
after; of an argument's error and the climb's, the further left is
given.** The climber's leaves are atoms (gate G4 builds on it as it is,
S190), and the first error in the text is the one a reader looks for.
- Alternative: the climber taught calls: `parse/expression` is shared, and
  P0 (chunk 27) changed only what stopped it compiling.

> **Review.** Taken: follows from S190. Recommended: S190's alternative
> would make it unnecessary. Ruled 2026.11.04: decided on 2026.11.06 with
> lane F against expression core v2's design: core v2 if its parser holds
> postfix operators and calls, S209 and S252 then following it; otherwise
> the language's own precedence parser, as S190's review recommends.
> **Flag:** follows from S190.

**S210. `^`'s and `mod`'s arithmetic moves from `evaluate.hpp` to
`function.hpp`, beside the functions'; the folds take functions from the
standard tables alone.** `pow` is `^`, and `fmod`, `remainder` and `mod` are
one remainder rounded three ways: one header holds what each computes.
- Alternative: the arithmetic left in `evaluate.hpp`, which `function.hpp`
  would then include, and every caller of the tables the folds with it.

> **Review.** Taken: for, one header computes each remainder and power;
> against, none. Recommended: as taken. Taken.

**S211. The face test grows in place: the stub checks of calls and
constants become checks of what they do; calls, constants and the factorial
join the random round trip; a boxed kind of the test's own, not exact,
checks the tower's kind for it; and each answer of the C core's a fold is
held to is computed as the test runs, by the C library.** As S198; the fold
computes with the C library, which G6 allows an ulp from the correctly
rounded answer the compiler would compute (section 8).
- Alternative: a test of 2c's own beside it; and answers from literal
  arguments, which the compiler computes, so that a check would fail at
  `-O1` and above wherever the library is an ulp away.

> **Review.** Taken: for, answers held to the library the fold uses,
> computed as the test runs; against, none. Recommended: as taken. Taken.

**S212. The checks: as S199, against chunk 29's delivered tree, with 312
planted faults, among them each standard function's kernel in a type
swapped for another's and each one-argument function's exact rule taken
away.**
- Alternative: the full tier again, 2,328 builds and two hours, of which
  only the new test's 64 can tell this change from chunk 29's.

> **Review.** Taken: as S199. Recommended: as taken. Taken.

### S213 to S222: chunk 31, 2026.10.24

The record: `_handoffs/math98_chunk31_2026.10.24/README.md`.

**S213. Logic binds below the relations: `or` at 10, `and` at 20, `not` at
30, the relations at 40, the arithmetic as before.** G5's table listed the
relations loosest, above `or`, and gave `x > 0 and x < 1` as its example,
which that order reads as `x > (0 and x) < 1`. C, Python and Mathematica
put `and` and `or` below the relations, and Python and Mathematica `not`
too, so that `not x = y` is `not (x = y)`. The roadmap's table now has the
order, with a note.
- Alternative: G5's table as written, its example then needing brackets,
  `(x > 0) and (x < 1)`.

> **Review.** Taken: for, the order C, Python and Mathematica share;
> against, a departure from G5's table as written. Recommended: as taken.
> Taken.

**S214. A chain of relations is the conjunction of its links:
`a < b <= c` is `a < b and b <= c`, `a = b = c` is `a = b and b = c`, and
`a < b > c` is `a < b and b > c`.** As in mathematics and in Python. The
relations are non-associative in the table, the climber groups a chain to
the right, and the parser layer joins its links after the climb, left to
right, in one pass down the chain. An operand between two links stands in
both, so that the term holds it twice (section 6).
- Alternatives: a chain refused, as chunk 27 thought a parser might, so
  that `a < b < c` is an error; or C's reading, `(a < b) <= c`, a boolean
  compared with a number, which a value calls a domain error.

> **Review.** Taken: for, mathematics' and Python's chains; against, the
> shared operand appears twice in the term, so it is folded twice and the
> term is larger. Recommended: as taken in meaning, the operand shared in
> the representation later. Taken.

**S215. A pair of round brackets is read as one operand before the climb,
as calls and bars are (S209).** The climber then meets no bracket, so that
a relation it makes an operand of another is a link of the same chain, and
a bracketed relation, `(a < b) < c`, is one operand and no link. The term
keeps no trace of the brackets, as before.
- Alternative: a node for brackets in the term, which every fold and
  rewrite would have to step through; or the climber left the brackets and
  the chains told by the tokens' places.

> **Review.** Taken: for, a bracketed relation is no link, and folds see no
> bracket nodes; against, the author's redundant brackets are not kept in
> the text. Recommended: as taken. Taken.

**S216. `piecewise` is read in `math/function/function.hpp`'s vocabulary:
`piecewise(when(c1, v1), ..., otherwise(d))` is the call
`piecewise(c1, v1, ..., d)`, and the renderer writes it back in its
branches.** The term stays one call of the function table, its arguments
the conditions and values in turn, so the folds need nothing new; `when`
and `otherwise` stand only as whole arguments of `piecewise`, an
`otherwise` only last, and each misuse is an error at its place.
- Alternatives: `when` and `otherwise` as functions of their own, whose
  values would be pairs, a kind the language has not; or an operator kind
  of its own, which `expr_op_kind` has not.

> **Review.** Taken: for, one call, no new kind, written back in its
> branches; against, none. Recommended: as taken. Taken.

**S217. As a value a relation is a boolean: of two numbers by value.h's
exact order, across kinds, none holding of a NaN but `!=`; of two booleans
`=` and `!=` alone; an error passes, the first; anything else is a domain
error. Logic is Kleene's: a false operand makes `and` false and a true one
makes `or` true whatever the others are; otherwise an error passes, the
first, and what is no boolean is a domain error. A boolean carries no
flags.** Exact where possible (`0.1 + 0.2 = 0.3` holds), and an error is
what value.h passes. Kleene's logic is the one in which an unknown operand
that cannot change the answer does not: `false and sqrt(-1) > 0` is false.
value.h's comparison gives an order, and no flags, so the boolean has
none.
- Alternatives: booleans ordered as value.h orders them, false below true;
  a number's truth as C's in logic, nonzero true; strict logic, in which an
  error anywhere passes.

> **Review.** Taken: for, relations exact across kinds, so `0.1 + 0.2 = 0.3`
> holds, and Kleene's logic, the one SQL and three-valued logics share, in
> which an unknown operand that cannot change the answer does not; against,
> a comparison of a NaN is false, not unknown, as IEEE 754's is. The
> alternatives: C's truth of a number in logic hides a slip, and strict
> logic lets an error that cannot matter spoil the answer. Recommended: as
> taken. Taken.

**S218. In a type the booleans are 1 and 0: a relation is C's comparison,
which holds of no NaN but `!=`; logic reads 0 as false, a NaN as unknown,
as an error is as a value, and any other number as true; a boolean bound
to a variable, or a number a fold made, is 1 or 0 where 2b made it a
NaN.** The typed folds are C's arithmetic, and the booleans 1 and 0 are
C's; a NaN is a type's error.
- Alternative: a relation of a NaN a NaN in a type, unknown, which no C
  comparison gives.

> **Review.** Taken: for, the typed folds stay C's arithmetic, and the
> booleans 1 and 0 are what C gives; against, a relation of a NaN is 0 in a
> type where its logic reads a NaN as unknown. The alternative, a NaN for
> such a relation, is what no C comparison gives. Recommended: as taken.
> Taken.

**S219. `true` and `false` are constants of the table: 1 and 0 in each
type, the booleans as values.** A fold's boolean is written `true` or
`false`, as render.hpp has written it since 2b, and now reads back as the
constant, whose value it is.
- Alternative: no names, the booleans only the relations' values.

> **Review.** Taken: for, a fold's boolean reads back as itself; against,
> two names taken from the variables. Recommended: as taken. Taken.

**S220. `all_of` and `any_of`, from the vocabulary, are `and` and `or` of
any number of arguments from 1, and `piecewise` takes any number from 1; a
function of any number of arguments says so in its arity's error, "all_of
takes at least 1 argument, not 0".** The vocabulary's own names.
- Alternative: `all_of()` true and `any_of()` false, the empty conjunction
  and disjunction: a text with no operand is more likely a slip.

> **Review.** Taken: for, an empty call is more likely a slip than meant;
> against, `all_of()` and `any_of()` have values, true and false, the empty
> conjunction's and disjunction's, which Python's `all` and `any`,
> Mathematica's `And[]` and `Or[]` and SymPy's `And()` and `Or()` give, and
> which a call over a list made by a program, a vector's elements once 3b
> can spread them, will meet. Recommended: the alternative, the identities,
> with `piecewise` kept at one argument at least. Ruled 2026.11.04: the
> alternative, as recommended, at any step that touches the folds' logic.
> **Flag:** not the recommendation; a minor point, changeable later, as it
> makes an error a value.

**S221. The face test grows in place: the `not_yet` checks of the
relations and logic become checks of what they do; a boolean bound in a
type is 1 or 0; the relations, logic, `true`, `false`, `piecewise` and
`all_of` join the random round trip.** As S211.
- Alternative: a test of 2d's own beside it.

> **Review.** Taken: as S211. Recommended: as taken. Taken.

**S222. The checks: as S212, against chunk 30's delivered tree, with 424
planted faults; the depth probe folds to a value too, with two shapes of
2d's.**
- Alternative: the full tier again, of which only the new test's 64 builds
  can tell this change from chunk 30's.

> **Review.** Taken: as S212. Recommended: as taken. Taken.

### S223 to S227: chunk 32, 2026.10.24

The record: `_handoffs/math98_chunk32_2026.10.24/README.md`.

**S223. Each member of the handle delegates to its term-level entry, and
answers as the entry does now: `subs` and `simplify` give the term
unchanged until 2f, as `expr_substitute` and `expr_simplify` do, and
`diff` a handle of `expr_derivative`'s `not_yet` until 4a.** 2a's skeleton
said the members would ("each answers as they do once it is filled"); so
2f and 4a fill the members when they fill the entries, and change nothing
in `expr.hpp`.
- Alternative: `subs` and `simplify` answering `not_yet` until 2f, as a
  handle can, where `expr_substitute`, which gives a term, cannot; 2f
  would then change `expr.hpp` as well.

> **Review.** Taken: for, the handle never answers otherwise than its entry,
> and 2f and 4a change nothing in `expr.hpp`; against, none. Recommended: as
> taken. Taken.

**S224. `eval` to a value takes no binding and the default context by
default: `eval()`, `eval(bindings)` and `eval(bindings, context)`.**
Roadmap Phase 2's example writes `math::parse("1/3 + 1/6").eval()`. It
cannot be taken for `eval<T>`, which names its type and takes no context.
- Alternative: both arguments named, as `expr_evaluate` has them,
  `eval(expr_bindings(), expr_context())`.

> **Review.** Taken: for, the roadmap's own spelling, `parse("1/3 +
> 1/6").eval()`; against, none. Recommended: as taken. Taken.

**S225. An error passes through every member unchanged. Of `subs`'s two,
the handle's own comes out; the replacement's comes out whether or not its
variable occurs in the term.** As 2a's contracts have it: a chain's error
is the first made in it, read left to right, as value.h passes the first,
and whether `subs` fails does not hang on what the term holds.
- Alternative: the replacement's error only where the variable occurs,
  where `expr_substitute` would put it in the term.

> **Review.** Taken: for, whether `subs` fails does not hang on what the
> term holds, and an error is the first made, read left to right; against,
> an error put in for a variable the term does not hold fails a call that
> could have succeeded. Recommended: as taken; an error given as a
> replacement is a mistake whatever the term. Taken.

**S226. The face test grows in place: the handle's stub checks become
checks of what it does, in five parts; the round trip's alphabet is shared
with the handle's random terms; the test's comparison of terms takes a
term made by default.** As S221.
- Alternative: a test of the handle's own beside it.

> **Review.** Taken: as S221. Recommended: as taken. Taken.

**S227. The checks: as S222, against chunk 31's delivered tree, with 445
planted faults. The fuzz reads each text by `parse` and `try_parse` as
well, comparing the handle's every member with its entry; the comparison
with chunk 31 reads its fuzz's alphabet, 2d's words among it, and must
find every line the same.**
- Alternative: the full tier again, of which only the new test's 64 builds
  can tell this change from chunk 31's.

> **Review.** Taken: as S222. Recommended: as taken. Taken.

### S228 to S236: chunk 33, 2026.10.24

The record: `_handoffs/math98_chunk33_2026.10.24/README.md`.

**S228. Simplification keeps the value at every binding at which each part
of the term has a finite value, a number or a boolean: exactly where the
term's value is exact, and the same real number where it is not. Where a
part's value is an error, an infinity or a NaN, the simplified term's may
differ; a part drops only where it is well formed.** The roadmap's own
rules ask for it: `0 + x` is `x`, and like terms collected make `x - x` 0,
which is no longer the value where x is infinite or an error. A simplifier
that kept each binding's value, its errors and infinities among them, could
drop nothing that might be infinite, `0 * y` keeping its y. Computer
algebra systems commonly make `x - x` 0 in the same way. Being well formed
keeps the errors no binding takes away, an unknown name's or a wrong
arity's, which mark a mistake in the text, not a point outside the domain.
- Alternative: each binding's value kept, its errors and infinities
  included: `x - x` and `0 * x` as they are, until the variables can be
  known finite, of which Phase 2 has no way to say.

> **Review.** Taken: for, the rules every computer algebra system applies,
> `x - x` 0 and `0 * y` 0, kept honest by a stated guarantee, the value kept
> wherever each part is finite, and `x / x` kept, as it is undefined at 0;
> against, `x - x` is 0 where x is infinite, where its value is a NaN, and
> nothing in the result says so. The alternative keeps every value but
> simplifies almost nothing. Recommended: as taken now; once the language
> can state assumptions (x finite, x nonzero), simplification under them,
> and an entry that gives the conditions a rewrite took, as Mathematica's
> `ConditionalExpression` and SymPy's assumptions do. Taken.

**S229. A node folds only where each operand is an exact number, `true` or
`false`, and only to an exact value or a boolean, the boolean written as
the constant `true` or `false`; an inexact value, or an error, leaves the
node as it is.** An exact fold changes nothing a later step can tell:
`1/3 + 1/6` and 1/2 are one number, in every type and as a value. A rounded
one would: `sqrt(2)`, folded to the long double nearest it, is the square
root of 2 no longer, to the derivative or to a fold in another type; and a
relation of inexact operands, `pi < 4`, answers by a rounding. The
constants keep the language's text: `1 < 2` is `true`, which reads back as
itself.
- Alternative: every closed node folded to its value, rounded where it is
  inexact, as a numeric simplifier would; or a node of any closed operands,
  `pi` among them, folded where its value comes out exact or a boolean.

> **Review.** Taken: for, no fold rounds, so `sqrt(2)` stays the square root
> of 2 for the derivative and for a fold in any type; against, `sqrt(2) *
> sqrt(2)` stays as written, as no rule of radicals makes it 2. The
> alternative would make simplification depend on a rounding. Recommended:
> as taken, with exact rules of radicals and powers later (Phase 4's
> simplification). Taken.

**S230. Like terms are terms with the same other factors, in any order,
each dividing as it does, compared by parse/expression's structural
equality; a coefficient is an exact number, and division by an exact number
other than 0 goes into it. A collected sum keeps the order in which each
group's first term stands, and writes a coefficient first.** So
`x * y + y * x` is `2 * x * y` and `x / 2 + x / 2` is `x`, and a term keeps
the order its author wrote: `x + 1 + x` is `2 * x + 1`, where a canonical
order would make `1 + x` into `x + 1`.
- Alternative: like terms only with their factors in the same order,
  `x * y` and `y * x` two terms; or a canonical order of terms and factors,
  sorted, which would also find `sin(x * y)` and `sin(y * x)` alike
  (section 6), but rewrites every term, one with nothing to collect among
  them.

> **Review.** Taken: for, `x * y + y * x` is `2 * x * y`, and the author's
> order is kept; against, two drawbacks, the first mended in 3b. Factors
> were taken to commute, so a matrix product was collected as if it did: in
> chunk 34 `[1, 2] * [3; 4] - [3; 4] * [1, 2]` simplified to `0 * [1, 2] *
> [3; 4]` and `A * B + B * A` of two literals to `2 * A * B`; from 3b a
> factor that holds a matrix keeps its place (S251). And like terms are
> found by structure alone, so `sin(x * y)` and `sin(y * x)` are two terms.
> Recommended: a canonical comparison key, built for the comparison and
> never written into the term, so the author's order is kept and `sin(x *
> y)` and `sin(y * x)` are alike; and factors commuting only where each is a
> number, as 3b made them (S251), matrix variables declared as S251's review
> asks. Taken, its drawback from 3b mended by S251; the key a minor point.

**S231. A rule applies only where it makes the node smaller, by
parse/expression's size, or a sum as small with fewer terms.**
Simplification then never makes a term larger, a pass that changes the
term makes it smaller or a sum in it shorter, and a term with nothing to
simplify comes back as it was: `x * 2` as `x * 2`, and `a - (b - c)` as
written.
- Alternative: each sum and product written in its collected form,
  whatever its size: `x * 2` as `2 * x`, and `a - (b - c)` as `a - b + c`.

> **Review.** Taken: for, simplification never makes a term larger and keeps
> what has nothing to simplify as written; against, no canonical form, so
> two equal terms may simplify to different texts. Recommended: as taken for
> `simplify`, with `expand`, `factor` and a canonical form as entries of
> their own, as SymPy's `expand`, `factor` and `together` are. Taken.

**S232. Simplification makes passes of its own over the term, bottom up,
collecting each sum and product once a pass, at the root of its chain,
until a pass changes nothing, and at most 1,024 passes, as many as
`rewrite_to_fixpoint` makes; not `rewrite_to_fixpoint` itself, with the
rules its rewrite at every node.** A chain's terms are collected together,
where a rewrite at a node sees that node: collected at each node, a chain
of n terms would be collected below each of its n nodes, n times over,
where once at its root does it.
- Alternative: `rewrite_to_fixpoint`, each node's chain collected where the
  node stands.

> **Review.** Taken: for, a chain collected once at its root, not once below
> each of its nodes; against, a pass of its own beside
> `rewrite_to_fixpoint`. Recommended: as taken. Taken.

**S233. Substitution puts a term in place of a variable alone: a named
constant and a function's name stay. The replacement goes in whole, and is
not searched again. Substitution in a term made by default gives it back,
and such a term put in place of a variable the term holds makes the result
one.** The folds' bindings bind variables alone, and substitution binds a
variable to a term; one fold over the term keeps x by `x + 1` from going on
without end. A term made by default is the language's no-term: in it there
is no node to replace, and put in, it leaves none where the variable stood.
- Alternative: a named constant replaced too, `pi` by 3 making
  `pi * f(x)` into `3 * f(x)`; and a term made by default, put in, leaving
  the term as it was.

> **Review.** Taken: for, substitution binds what the folds' bindings bind,
> and one pass cannot run on without end; against, a named constant cannot
> be replaced, which a binding of its name does. Recommended: as taken.
> Taken.

**S234. The factorial's loops in `function.hpp`, in a type and exactly, end
at the type's largest binary exponent at the latest, as well as where the
product passes the type's range.** n! is past 2^n from n = 4, and so past
the range by then: where `std::isinf` answers truly, the loop has ended
before, and no answer changes. Where it does not, as under valgrind, each
loop ran to n, and the 18 large factorials 2f's random checks ask for kept
valgrind on the test past the queue's two hours (section 2.5). The change
is in 2c's code, outside 2f's own: it changes no answer, and without it the
test cannot be run under valgrind.
- Alternative: `function.hpp` as it was, and the test no longer run under
  valgrind; or 2f's random checks kept from large factorials, which the
  round trip's alphabet, its literal 123456789 and the factorial, makes.

> **Review.** Taken: for, no answer changes and the test runs under
> valgrind; against, a change in 2c's code made in 2f's chunk. Recommended:
> as taken. Taken.

**S235. The face test grows in place: the rewrites' part keeps the
derivative's 3 stubs, and eight new parts check substitution and
simplification, two of them on random terms, of 2d's language and of an
alphabet of arithmetic beside it.** As S226.
- Alternative: a test of the rewrites' own beside it.

> **Review.** Taken: as S226. Recommended: as taken. Taken.

**S236. The checks: as S227, against chunk 32's delivered tree, with 500
planted faults. The fuzz simplifies each random term, no larger, a fixed
point and read back the same, its value kept at an exact binding where each
part is finite, and substitutes a random term in each, as a value and in
`double`; the depth probe simplifies each shape too; the comparison with
chunk 32 reads chunk 32's fuzz's texts by both trees and must find every
line the same.**
- Alternative: the full tier again, of which only the new test's 64 builds
  can tell this change from chunk 32's.

> **Review.** Taken: as S227. Recommended: as taken. Taken.

### S237 to S245: chunk 34, 2026.10.25

The record: `_handoffs/math98_chunk34_2026.10.25/README.md`.

S237 to S245, chunk 34's, as `decisions.md` has them; each is reviewed there
(section 2.6), the 18 flagged decisions of S1 to S245 listed first.

**S237. The matrix kind's table is written in C++, in `math/expr/matrix.hpp`,
from C++11, as the language that makes and reads matrices is; not in C, as
`value_wide.h`'s wide integer is.** Its elements are `math::value`s, which
hold their boxes as C++ objects do, and its storage is `std::vector`s;
`value.h` meets it through the table alone, unchanged. A kind in C would
serve C callers, who have no expression language to make a matrix with,
and would hold its elements' boxes by hand.
- Alternative: the table in C, `c/math/value_matrix.h` beside
  `value_wide.h`, with a C harness of its own, and `math/expr/` reading it
  through the C functions.

> **Review.** Taken: for, elements held as C++ objects hold their boxes, the
> table sits beside the one language that makes matrices, and the owner's
> principle 1 gives element types other than the three floating-point ones a
> generic C++ path; against, a C program has no matrix value, and the risk
> that 3b and 3c write matrix arithmetic again in `matrix.hpp`. The
> alternative, the kind in C beside `value_wide.h`, would serve C callers
> who have no language to make a matrix with, at the cost of holding every
> element's box by hand. Recommended: as taken, with 3b's and 3c's
> arithmetic not written in `matrix.hpp` but delegated, a dense matrix's to
> the C core's `mat.h` and `vec.h`, whose row-major arrays its storage
> already is, and a matrix of values to `linear_algebra`'s generic matrices
> of `math::value` (chunk 26, from C++98), so each algorithm is written
> once. Taken; the delegation done by 3b's arithmetic (S247) and 3c's
> functions (S257), the C core's decompositions beside the kernels.

**S238. One kind, "matrix", of rank 1000, past every number's: the long
double's 500 and the wide integer's 210, with room left below it for
bignum's and the complex numbers'. A matrix is no number: it has no trait,
converts to no number, and `value.h` hands every operation with a matrix
among its operands to the matrix's table.** Its storage, values or a dense
array of one floating-point type, is the box's, not the kind's; a vector is
a matrix.
- Alternative: four kinds, an exact matrix and a dense one of each
  floating-point type, each of a rank of its own; or a vector kind beside
  the matrix.

> **Review.** Taken: for, one table, the storage the box's; against, none.
> The alternative's four kinds would multiply the tables and their
> cross-kind cases. Recommended: as taken. Taken.

**S239. A matrix keeps each element as it is given: as a dense array of
`float`, `double` or `long double` where every element is one of those, in
the widest one's type, to which each widens exactly; and as `math::value`s,
each of its own kind, where any element is exact or boxed.** No element is
rounded by being put in a matrix, so an exact matrix stays exact for 3c's
determinant and inverse, an exact element beside an inexact one stays
exact until an operation of 3b combines them, and the dense array, what
Eigen and BLAS take, is what a matrix of floating-point numbers is. (Taken
first, before this chunk's delivery, as the tower's storage: dense in the
widest inexact element's type wherever one element was inexact, each exact
element its nearest there. That rounded an exact `1/3` beside a `double`
to its nearest `double` on making the matrix, before any operation asked
for it; it was changed in this chunk, and the faults that test the storage
were planted again on the new passage.)
- Alternative: the tower's storage, as first taken; always values, a dense
  array only on request; or a dense array wherever the elements fit one,
  exact integers among them.

> **Review.** Taken, revised in this chunk: for, no element is rounded by
> being put in a matrix, an exact matrix stays exact for 3c, and a matrix of
> floating-point numbers is the dense array Eigen and BLAS take; against, a
> matrix of one exact element among thousands of doubles is held as values,
> slower than dense, until an operation combines them. The rule first taken,
> the tower's storage, rounded an exact element on making the matrix, before
> any operation asked for it. Recommended: as taken now. Taken.

**S240. A vector is a matrix of one row: `[1, 2, 3]` is 1 by 3, and a column
is written `[1; 2; 3]`, as MATLAB writes them; `expr_vector` makes the one
row.** Every shape then has a literal that reads back as itself: `[a]` is 1
by 1, `[a, b]` 1 by 2, `[a; b]` 2 by 1.
- Alternative: a vector a column, n by 1, the shape A v takes in a
  textbook; a matrix of one row would then have no literal until 3b's
  transpose, `[1, 2, 3]'`.

> **Review.** Taken: for, every shape has a literal that reads back as
> itself, as MATLAB writes them; against, SymPy's `Matrix([1, 2, 3])`,
> Eigen's `VectorXd` and Julia's `[1, 2, 3]` are columns, as textbooks write
> `A v`, so `A * [1, 2]` will be a shape error where a reader of those
> expects a product. The alternative leaves a row with no literal before
> 3b's transpose. Recommended: as taken, documented with the column `[1; 2]`
> beside it. Taken.

**S241. An element is a number: an element that is an error passes, the
first, and one that is no number, a boolean, none or a matrix, makes the
matrix a domain error, as in arithmetic; so `[[1, 2], 3]` is a domain
error, not a concatenation. A matrix has a row and a column at least: `[]`
is a missing operand, as an empty element or row is. A matrix literal's
rows have as many elements each as its first, or the literal is bad_shape,
at the semicolon or the bracket that ends the row of another number.** The
place keeps the parse's errors met left to right, as each element's own
error stands before the end of its row.
- Alternative: a nested literal concatenated, as MATLAB's `[[1, 2], 3]` is
  `[1, 2, 3]`; an empty matrix, 0 by 0; the shape's error at the row's
  first element.

> **Review.** Taken: for, an element is a number, as in arithmetic, and a
> ragged row's error stands at its place; against, no empty matrix, which
> 3c's kernels and null spaces give (an n by 0 basis), and no block literal,
> `[A, B; C, D]` of matrices of fitting shapes, which MATLAB, Julia and
> Eigen's comma initializer have. Recommended: as taken for 3a; empty
> matrices and block literals by 3c, both of which make an error a value and
> so change no literal that reads now. Taken in full with chunk 36: the
> empty matrix (S260) and blocks (S261), a literal of numbers alone read and
> refused as before.

**S242. Until step 20 (3b) a matrix computes nothing. In a type a literal
has no value, bad_shape, an operand's error coming first, as at any node.
As a value, arithmetic, relations, logic and functions on a matrix are a
domain error, by the kind's table and by `function.hpp`'s guard. The kind's
comparison finds two matrices equal where they are of one shape and their
elements are equal, place by place, by `value.h`'s order, whatever their
storage, and finds no order otherwise.** Two atoms of one matrix are then
one atom, as structural equality, and simplification's fixed point, ask. A
variable bound to a matrix is a NaN in a type, as one bound to none is.
- Alternative: not_yet for each operation on a matrix, naming step 20; or
  the sums and the scalar products now, ahead of their step.

> **Review.** Taken: for, nothing computes before its step, and the
> comparison two atoms of one matrix need is there; against, none.
> Recommended: as taken. Taken.

**S243. Indexing waits for step 20 (3b): a `[` after an operand's end is
indexing, `v[i]`, and answers not_yet, naming step 20, and is never an
implied product, with implied multiplication on too: `2[1]` is not
`2 * [1]`.** The roadmap's 3a names the kinds and the literals alone;
`operator.hpp`'s note had placed `index` in 3a, and now places it in 3b,
the step of the other postfix operator, the transpose, whose roadmap item
now names indexing as well.
- Alternative: `v[i]` and `A[i, j]` now, by `expr_element`, counted from 1
  as in mathematics, or from 0 as in C.

> **Review.** Taken: for, `2[1]` is never a silent product, and indexing
> waits for its step; against, none. Recommended: as taken; in 3b, indices
> counted from 1 in the language, as mathematics, MATLAB, Julia and
> Mathematica count, and from 0 in the C++ functions, as `expr_element`
> counts. Taken.

**S244. The face test grows in place: section 11 checks the kind and the
entries, and four new parts the literals in the parser, in the renderer and
in the folds, and on 1,000 random literals drawn by a generator of their
own, so that the random parts before them draw what they drew. Chunk 33's
two checks that took a parse's value without asking whether it held one
now ask (chunk 33's finding), and the new checks read every result, every
shape and every box through helpers that give none, or nothing, where a
fault leaves none.** A fault then fails checks rather than read what is not
there; one stops the test by a signal only where the code it faults does.
- Alternative: a test of the literals' own beside the face test; or the
  literals in the round trip's generator, which would change what every
  random part after it draws.

> **Review.** Taken: for, every random part before the literals draws what
> it drew, and a fault fails checks rather than read what is not there;
> against, none. Recommended: as taken. Taken.

**S245. The checks: as S236, against chunk 33's delivered tree, with 569
planted faults: chunk 33's 500, 18 of them moved with the passages step 19
wrote again, F199 put in anew, and 69 in step 19's code, run in three
parts. The fuzz as chunk 33's, its random texts now spelling literals; the
depth probe with a third sweep, of nested literals; and the comparison with
chunk 33's random texts, every line the same but those chunk 33 answered
with step 19's not_yet, each of which must read otherwise now.**
- Alternative: the full tier again, of which only the face test's 64
  builds can tell this change from chunk 33's.

> **Review.** Taken: as S236. Recommended: as taken. Taken.

### S246 to S255: chunk 35, 2026.10.26

The record: `_handoffs/math98_chunk35_2026.10.26/README.md`.

S246 to S255, chunk 35's, as `decisions.md` has them; each is reviewed there
(section 2.5), the 20 flagged decisions of S1 to S255 listed first.

**S246. The matrix kind computes what linear algebra defines, and nothing
else: A + B and A - B of one shape, element by element; -A; A * s, s * A
and A / s, of a matrix and a number, each element by the number; and A * B,
A's columns as many as B's rows, the matrix product. Every other operation
on a matrix is a domain error: A + s, s + A, A - s and s - A; s / A and
A / B; |A|; and two matrices of shapes the operation does not take. A
1 by 1 matrix stays a matrix: [1, 2] * [3; 4] is [11], not 11.** Each of
these has one meaning in every text on linear algebra; a matrix plus a
number has two (each element plus the number, as MATLAB and NumPy
broadcast it, or the number times the identity, as some texts write it),
and a quotient of matrices is A times B's inverse on one side or the other,
which 3c's `inv` and `solve` will say outright. An error of shapes is then
an error at its place, never a broadcast that hides it.
- Alternative: MATLAB's and NumPy's broadcasting, A + s each element plus
  s and A / B as A * inv(B); or a 1 by 1 result taken for its number.

> **Review.** Taken: for, every operation the kind computes has the one
> meaning linear algebra gives it, and an error of shapes is an error at its
> place, as SymPy's matrices and Eigen's give it, never a broadcast that
> hides a mistake; against, what MATLAB and NumPy users write by habit, A +
> 1 for each element plus 1, is an error here, and there is no element-wise
> product, quotient or power, A .* B, A ./ B, A .^ 2, which MATLAB, Julia
> and NumPy have. The alternative, broadcasting, would make A + s mean one
> thing where texts on linear algebra mean another, and A / B would be an
> inverse that 3c has not brought. Recommended: as taken, with the
> element-wise operators as operators of their own, spelled apart from the
> algebraic ones (MATLAB's and Julia's `.*`, `./`, `.^`), when a step asks
> for them; A / B as 3c's `solve`. Taken.

**S247. Each operation is computed by linear algebra's kernels, written
once (`linear_algebra/linalg_common.hpp`): a dense matrix's by the C core's
`vec.h` and `mat.h`, in the wider of its operands' types, each element
widened exactly first, and an exact number converted to that type as the
tower converts it, so that a matrix of floats times 2 stays floats; a
matrix that holds values, or any operation with one, by the generic path,
in `math::value`'s own arithmetic, exact wherever its elements are. A
result is stored as S239 stores any matrix: dense where every element is
floating, as values otherwise. A dense matrix keeps no flags of its own
elements, so each element read from it, by `expr_element`, by indexing or
by the arithmetic of values, carries the matrix's flags: an inexact
element (G2) is never read as an exact one's neighbour without its flag.
Storage that cannot be had is `D_MATH_VALUE_ERROR_MEMORY`, as `value.h`
asks of a kind's entries, and nothing throws.** The kernels are those 3c's
`det`, `inv` and `solve` will use, so the language and `linear_algebra`
give the same bits for the same product. (The flags: chunk 34's
`expr_element` read a dense element without them; it now gives them, as
this step's indexing does.)
- Alternative: the arithmetic written here, element by element, on
  `math::value`s alone; or every operation on dense matrices in long
  double, whatever their type.

> **Review.** Taken: for, each operation written once, in the kernels 3c and
> `linear_algebra` share, so a product has the same bits wherever it is
> computed, and an exact matrix stays exact through the generic path;
> against, a matrix of values and a dense one meet on the generic path, one
> element at a time, slower than the dense kernel, and a dense matrix's
> elements share one set of flags. The alternative of arithmetic written
> here would be a second implementation to keep in step; long double
> throughout would round a float matrix's products otherwise than the type
> does. Recommended: as taken; per-element flags are not needed while the
> one flag is G2's inexact, which every element computed from an inexact one
> carries anyway. Taken.

**S248. A ^ n is the power of a square matrix to a whole n >= 0, exact or
floating-point; to a floating-point n, the matrix is first taken to the
wider of its type and n's, as the tower takes a number to a power of another
kind. It is computed by repeated squaring, A's square, its square's, and so
on, multiplied into the power where n's bits are 1, each product the
kernels'. A ^ 0 is the identity of A's size in A's storage: exact for a
matrix of values, in its type for a dense one, as a floating-point number to
the power 0 is 1 in its type. Any other power is a domain error: a matrix
not square, n negative (which needs A's inverse, 3c's), n not whole,
infinite, a NaN or past the exact integers' range, and a number to a
matrix's power; `pow(A, n)` is A ^ n.** Squaring takes at most 2 log2(n)
products where repeated multiplication takes n - 1, and a dense power's
rounding grows with their number: the 90th power of [1, 1; 1, 0], exact,
F(91) and F(90) among its elements, takes 9. The C core's `d_mat_power`
multiplies A ^ (k - 1) by A, so a dense matrix's power here and there may
differ in their last bits.
- Alternative: the C core's `d_mat_power`, n - 1 products; an exact n
  alone, a floating-point 2 a domain error; A ^ 0 always exact.

> **Review.** Taken: for, at most 2 log2(n) products where n - 1 would do,
> exact powers of exact matrices whatever their size, and a floating-point 2
> taken as 2, the power in the wider type, as the tower takes a number to
> it; against, the C core's `d_mat_power` computes A ^ (k - 1) A, so a dense
> matrix's power differs in its last bits between the language and the C
> core's function, two answers for one operation where the framework's
> principle (4) wants one; and a negative power waits for 3c's inverse. The
> alternative, the C core's n - 1 products, would agree with it, at more
> products and more rounding. Recommended: as taken, with the C core's
> `d_mat_power` changed to repeated squaring, its own tests then checking
> the same powers, so both agree; and A ^ (-n) as inv(A) ^ n in 3c, for an
> invertible A. Taken in full with chunk 36: `d_mat_power`, the generic
> kernel's power and `dynamic_matrix`'s square as the language does (S262),
> and A ^ (-n) is inv(A) ^ n (S259).

**S249. Indexing counts from 1, as mathematics, MATLAB, Julia and
Mathematica count: A[i, j] is the element in row i, column j; v[i] the i-th
element of a vector, a matrix of one row or of one column; and A[i] of any
other matrix its row i, a matrix of one row, the row Mathematica's `A[[i]]`
and NumPy's `A[i]` give. An index is a whole number of any kind, exact or
floating-point, from 1 to the places it counts; anything else is a domain
error, as is indexing what is no matrix, and an error passes, the first, the
matrix's before an index's. A[i, j] with i or j out of range is a domain
error, not bad_shape, as the index is a value the fold computed. In a type,
where no value is a matrix, indexing is a NaN, as a variable bound to a
matrix is. A' of a number is the number, a number being its own transpose;
of an error the error; of a boolean or none a domain error.** The C++
functions count from 0 (`expr_element`), as C++ does (S243's review).
- Alternative: indices from 0, as C and NumPy count; MATLAB's A(i), the
  i-th element down the columns, for any matrix; an exact index alone.

> **Review.** Taken: for, the indices of mathematics and of every language
> of linear algebra but NumPy's, an index computed by any fold accepted
> where it is whole, and the errors of value.h's order; against, no slices,
> A[1:2, :], no index from the end, MATLAB's `end`, and no logical indexing,
> which MATLAB, Julia and NumPy have. The alternatives: indices from 0 would
> read against every text the language's users write from; MATLAB's linear
> A(i) gives a column-major element of a matrix whose storage is row by row,
> and an exact index alone would refuse the 2.0 a fold in double gives.
> Recommended: as taken, with slices and an `end` as later items, when a
> step asks for them; the C++ functions counting from 0, as `expr_element`
> does. Taken.

**S250. The parser keeps each nesting's stack frame small: the work of a
group, its end, its term, its errors and its rows, is done in functions of
its own, kept out of line, outside the function that recurses into nested
groups, so that each level of nesting costs that function's frame and the
range's alone.** Chunk 34 kept a literal's rows in every bracket's frame,
and nested brackets stopped the program sooner than chunk 33's (chunk 34's
finding); step 20, which rewrote that function for indexing, moved the
work out, and nested brackets and calls now read as deep as chunk 33's did
or deeper (section 6). The depth itself is still the owner's to bound
(chunk 33's finding).
- Alternative: the work inline, as chunk 34 had it; or an explicit stack
  of groups, no recursion at all, which would bound the depth by memory
  rather than by the stack.

> **Review.** Taken: for, nested brackets and calls read as deep as chunk
> 33's or deeper on the same stack, and the depth no longer falls with each
> step that adds to a group; against, the depth is still the stack's, and a
> deep enough text still ends the program (chunk 33's finding). The
> alternative, an explicit stack of groups, would read any depth memory
> holds; the folds, the renderer and simplification recurse as deep as the
> term all the same, so a parser that read deeper would hand them terms they
> cannot fold. Recommended: as taken, with a depth guard over the whole
> module, as chunk 33's finding asks of the owner: an error, too_deep, past
> a bound the owner sets. Taken.

**S251. Simplification keeps the order of every factor that holds a matrix,
a literal or a matrix a fold made: two terms are alike only where those
factors stand in the same order, so that A * B + B * A stays and A * B +
A * B is 2 * A * B, while numbers, variables and any other factor commute
with everything, so that 2 * A * x + x * A is 3 * A * x. No rule makes an
error of matrices a value: A ^ 1 is A only where A is square by its shape,
a literal or a matrix a fold made; in a sum that holds a matrix the terms
of coefficient 0 it drops are written as one 0, since a matrix plus a
number is an error (S246): [1, 2] + 0 stays, and x - x + [1, 2] is
0 + [1, 2]. A literal, or a matrix a fold made, is never well formed
(S228), so 0 * A stays and [1, 2] - [1, 2] is 0 * [1, 2], the zero matrix.
A variable is taken for a number, as a symbol is in SymPy until it is
declared a matrix: bound to matrices, x * y + y * x is 2 * x * y all the
same, and x ^ 0 is 1.** This is the order S230's review (chunk 34) asked
of step 20, before matrix products had values.
- Alternative: every factor kept in its order, numbers among them, so that
  x * y + y * x stays; or variables taken for matrices, which commute with
  nothing.

> **Review.** Taken: for, A * B + B * A stays and A * B + A * B collects,
> numbers and variables still commute, and no simplification turns an error
> of matrices into a value; against, a variable is taken for a number, so
> for variables bound to matrices A * B + B * A becomes 2 * A * B and A ^ 0
> becomes 1, a different value, and the terms of variables a matrix user
> writes are those most often simplified. The alternatives: no factor
> commuting would leave x * y + y * x uncollected, every variable a matrix
> would collect nothing. Recommended: declared domains (S274), declared
> matrix variables among them, as SymPy's `MatrixSymbol` and its
> non-commutative symbols are: a binding's or the syntax's mark that a name
> is a matrix, of a shape where known, or a quaternion, which simplification
> then treats as a literal is treated now, its powers' identities included;
> in step 25 (4a), or sooner where a user simplifies terms of matrix
> variables. Ruled 2026.11.04: declared domains, matrix variables among
> them, at step 25 (4a); S376's variables beside quaternions with them.
> **Flag:** significant drawback for variables bound to matrices.

**S252. Indexing goes through the shared climber: a [ after an operand's
end opens the operand's indices, never an implied product (S243), read as
one group, each index split at the group's commas and parsed in turn, and
stands in the climb as a postfix operator of a table the parser layer adds
to the standard one, at a precedence above every other, 100, so that -v[1]
is -(v[1]), 2 ^ v[1] is 2 ^ (v[1]), A[1][2] indexes the row A[1] and x'[1]
is (x')[1]; once the climb is done it is put back as the term index of its
operand and its indices. The transpose, A', is the standard table's,
postfix, between the unary minus and ^, at 75: A' ^ 2 is (A') ^ 2, A ^ 2'
is (A ^ 2)', as MATLAB reads both, -A' is -(A'), and x!' is (x!)'. The
renderer writes an index after its operand, bare where the operand is a
leaf, a call, a literal, an index or a postfix operator's, and in round
brackets otherwise, each reading back as itself.** An index's operand is
whatever stands before it, the climb's own operand, so that a postfix
operator of the climber's is the one way to give it one; and A ^ 2' is the
power's transpose wherever it is written, as a MATLAB user means it. G5's
table, which put ' beside ! above ^, is revised so, as S213 revised it for
logic (`roadmap.md`).
- Alternative: the transpose at the factorial's place, 90, tighter than ^,
  so that A ^ 2' is A ^ (2'), A ^ 2 silently; indexing as a group of its
  own before the climb, as a call is, which cannot index a parenthesized
  term, (a + b)[1], or a transpose.

> **Review.** Taken: for, indexing of any operand, a group's and a
> transpose's among them, through the shared climber as S190 kept it, and A
> ^ 2' read as MATLAB reads it, so that no MATLAB text means another thing
> silently; against, a table of the parser layer's own beside the standard
> one, a postfix operator whose indices the climber never sees, put back
> after it, and a transpose looser than the factorial, so that 2 ^ 3! is 2 ^
> 6 while 2 ^ 3' is (2 ^ 3)'. The alternative transpose, at 90, would read A
> ^ 2' as A ^ 2 with no error; indexing as a group before the climb could
> not index (a + b) or A'. Recommended: as taken within S190's shared
> climber, whose own alternative, a precedence parser of the language's,
> would hold postfix operators with operands as it holds calls. Ruled
> 2026.11.04: decided on 2026.11.06 with lane F against expression core v2's
> design: core v2 if its parser holds postfix operators and calls, S209 and
> S252 then following it; otherwise the language's own precedence parser, as
> S190's review recommends. **Flag:** follows from S190.

**S253. A relation of two matrices is = or !=: they are equal where they are
of one shape and their elements are equal, place by place, by `value.h`'s
order, whatever their storage, as the kind's comparison finds them
(S242). An order of matrices, A < B, and a relation of a matrix and a
number, A = 1, are domain errors.** So A' = A asks whether A is symmetric,
and (A * B)' = B' * A' holds.
- Alternative: = and != element by element, a matrix of booleans, as
  MATLAB's `==` gives; or an order of matrices, by their elements in
  turn.

> **Review.** Taken: for, A' = A asks a question with one answer, a boolean,
> as a relation's value is everywhere else in the language, and a matrix and
> a number never compare silently; against, no element-wise comparison,
> which MATLAB's `A == B` gives as a matrix of booleans. The alternatives: a
> matrix of booleans would make a relation's value no boolean, and logic and
> piecewise would need matrices of booleans; an order of matrices has no
> meaning in linear algebra. Recommended: as taken, element-wise comparison
> as an operator or a function of its own, beside S246's element-wise
> operators, when a step asks for it. Taken.

**S254. The face test grows in place: d_tests_matrices checks what the
kind refuses and expr_transpose; seven new parts check the kind's
arithmetic and each result's storage, the transpose, powers, indexing and
relations as values, indexing and the transpose in the parser, in the
renderer and in both folds, and simplification of matrices; and the last
draws 1,000 random terms of matrices by a generator of its own, each folded
and compared with the test's own arithmetic, by the definitions, element
by element in the value's arithmetic apart from the kind and its kernels,
powers by n - 1 products rather than by squaring, the same matrix exactly
or a domain error where the generator drew shapes that do not fit.** Its
generator is its own, so the random parts before it draw what they drew;
an oracle apart from the code tested catches what a round trip cannot, a
wrong product that reads back as itself.
- Alternative: the arithmetic checked against hand-computed matrices
  alone; or random matrices compared with the kind's own results under a
  second path, which would share the kernels' faults.

> **Review.** Taken: for, an oracle apart from the kind, the definitions in
> the value's own arithmetic, so a wrong product that reads back as itself
> is caught; its powers by n - 1 products check the squaring; and every
> random part before it draws what it drew; against, the oracle shares
> value.h's arithmetic with the kind, so a fault in value.h would pass both.
> The alternatives: hand results alone would check a few shapes; a second
> path through the kernels would share their faults. Recommended: as taken;
> value.h's arithmetic is checked by its own tests. Taken.

**S255. The checks: as S245, against chunk 34's delivered tree, with 648
planted faults: chunk 34's 569, 49 of them moved with the passages step 20
wrote again (six put in anew, their passages gone), and 79 in step 20's
code, run in parts. The fuzz as chunk 34's, with a third part, 100,000
random terms of matrices checked against the fuzz's own arithmetic, dense
ones among them; the depth probe with a fourth sweep, of nested indices
and of transposes, and the bisection of chunk 34's finding with nested
indices beside it; and the comparison with chunk 34's random texts, every
line the same but those chunk 34 answered with step 20's not_yet, each of
which must read otherwise now, and those whose value chunk 34 gave as a
domain error of matrix arithmetic, whose term and fold in double must be
the same.**
- Alternative: the full tier again, of which only the face test's 64
  builds can tell this change from chunk 34's.

> **Review.** Taken: as S245, the fuzz's matrices beside its terms and
> texts. Recommended: as taken. Taken.

### S256 to S278: chunk 36, 2026.10.26

The record: `_handoffs/math98_chunk36_2026.10.26/README.md`.

S256 to S264, chunk 36's, and S265 to S278, the owner's rulings of
2026.10.27 (2.7), as `decisions.md` has them; each is reviewed there
(sections 2.6 and 2.7), the 22 outstanding decisions of S1 to S278 first,
by group and urgency.

**S256. Linear algebra's functions are functions of the language's table:
det(A), inv(A), trace(A), norm(A) and eig(A) of one argument, dot(a, b),
cross(a, b) and solve(A, B) of two, each called by name as sqrt is, each
also a C++ function, `expr_det` to `expr_solve`. Of a matrix, det, inv,
trace and eig take a square one; norm any, a vector's Euclidean norm and a
matrix's Frobenius norm, one formula, the root of the sum of the squares of
the elements; dot two vectors of one length, each a row or a column; cross
two vectors of 3 elements, the result of the first's shape; solve(A, B) A
square and B of its rows, or a vector of as many elements, X then of B's
shape. A norm whose sum of squares, in floating point, passes its type's
range or falls below its normal numbers' precision is computed again of
the elements scaled exactly by a power of two, the norm scaled back, so
that norm([1e200, 1e200]) is 1.414e200 and not an infinity, as LAPACK's
dnrm2 and Julia's norm scale; elsewhere it is the kernels' norm, bit for
bit. A number is a scalar, as in Julia: det, trace and eig of x are x,
inv(x) is 1 / x, norm(x) is |x|, dot(x, y) is x y, solve(a, B) is B / a,
and cross takes no number. In a type, where no value is a matrix, each is
the scalar's, and cross a NaN, as a variable bound to a matrix is. As a
value, a shape the function does not take, a boolean and none are domain
errors, and an argument's error passes, the first; the C++ functions give
bad_shape for a shape they do not take, with its message, as `expr_matrix`
does.** Julia's rule makes a 1 by 1 answer and a number agree, so a term
written for matrices folds in a type, as every term must (S190's one
grammar, two folds), and a scalar's det is the det of its 1 by 1 matrix.
- Alternative: the functions of matrices alone, a number a domain error, as
  SymPy's `Matrix.det` takes no number; or a 1 by 1 result taken for its
  number (S246 refuses that); and the norm unscaled, as NumPy's
  `linalg.norm` and Eigen's `norm()` compute it, an infinity past the root
  of the type's range.

> **Review.** Taken: for, every function has one meaning in each fold, a 1
> by 1 answer and a number agree, and a term written for matrices folds in a
> type, as Julia's `det(2)`, `inv(2)` and `tr(2)` give the scalar's answers;
> a norm the type holds is never an infinity or 0 for a square that
> overflowed or underflowed, and the scaling, by a power of two, changes no
> bit of a norm in range; against, cross of numbers is a NaN in a type and a
> domain error as a value, and a variable bound to a matrix folds in a type
> to a NaN, so a type cannot tell a matrix's det from a number's. The
> alternatives: functions of matrices alone would make det(x) of a number x
> an error in the one fold where every value is a number; a 1 by 1 result
> taken for its number is S246's refusal; the norm unscaled is NumPy's and
> Eigen's `norm()`, wrong at the type's ends. Recommended: as taken, with
> the same scaling in the C core's `d_vec_norm` and its `_c` form, where it
> is cheapest and serves every face, the language's wrapper then gone. Taken
> in chunk 38: the C core's norms, distance, unit vector and cosine scaled,
> the wrapper gone (S301).

**S257. The square algebra, det, inv, solve and eig, is computed in one
type, chosen from the matrix's storage: a dense matrix in its own type, by
the C core: LU with partial pivoting for det and solve (`decomp.h`'s
`d_decomp_lu`, `_lu_det`, `_lu_solve`), Gauss-Jordan elimination with
partial pivoting for inv (`mat.h`'s `d_mat_inverse`, through the kernels);
a matrix of exact values exactly, by the generic kernels in the value's own
arithmetic: Bareiss's fraction-free elimination for det, Gauss-Jordan for
inv, and solve as inv(A) B; a matrix of values with an inexact element in
the widest inexact element's type, each exact element its nearest there,
as the tower takes an operation's arguments; and a matrix with a boxed
inexact element on the generic path, by its kind's own arithmetic. solve
of two matrices takes the wider of their types, an exact one taking the
other's. A singular matrix's inverse and solution are a pole, as an exact
number over 0 is; its determinant is 0. Each result carries its operands'
flags. trace, norm, dot and cross follow S247's rule, a matrix of values by
the generic path, as the arithmetic does.** An exact matrix's
determinant and inverse are exact, which no floating-point library gives
and every symbolic one does; a dense one's are LAPACK's algorithms, the C
core's bits. The C core's LU reports a singular matrix only where a pivot
is exactly 0, as LAPACK's `dgesv` reports it: a nearly singular dense
matrix has an inverse of large elements, not a pole.
- Alternative: every matrix in long double; or exact matrices in long
  double too, as NumPy computes every determinant.

> **Review.** Taken: for, an exact matrix's determinant, inverse and
> solution exact, as SymPy's are and no floating-point library's are; a
> dense matrix's by LAPACK's algorithms, the C core's bits, in its own type;
> and a matrix mixed of exact and inexact elements computed as the tower
> computes an operation of them; against, a dense singular matrix is a pole
> only where a pivot is exactly 0, as LAPACK's `dgesv` reports it, so a
> nearly singular one has an inverse of huge elements and no warning. The
> alternatives: long double for every matrix would round an exact one and a
> float one otherwise than their types; NumPy's floating-point determinant
> of [1, 2; 3, 4] is -2.0000000000000004. Recommended: as taken, with an
> elimination of [A | B] in the kernels' families beside `inverse` (the C
> core's as `d_mat_solve`) for exact and boxed solves, and a condition
> estimate, LAPACK's `rcond`, as a function of its own, `cond`, when a step
> asks for one. Taken in chunk 38: the elimination of [A | B] in every
> family, the C++ face's `solve` on it (S302); `cond` when a step asks.

**S258. eig(A) gives a square matrix's eigenvalues, ascending, as a vector
of one row, in A's algebra type (S257): a symmetric A's by the cyclic
Jacobi method (`d_decomp_sym_eigen`), A first scaled exactly by a power of
two so that its largest magnitude is in [1, 2), the threshold its Frobenius
norm times the type's epsilon; any other A's by Hessenberg QR with
Wilkinson's shifts (`d_decomp_eigenvalues`, 1,000 iterations); NaNs of A's
type where A holds a NaN or an infinity. Where they are not all found real
the value is a domain error, as a complex pair has no value while the value
has no complex kind (S45). An exact A's eigenvalues are found so in long
double and each made exact where it is rational: A's characteristic
polynomial is computed exactly, by Faddeev and LeVerrier; every rational
eigenvalue is a multiple of 1/d, d the least common multiple of A's
denominators (an eigenvalue being a root of a monic polynomial over the
integers once A is scaled by d); so each found eigenvalue's nearest such
multiple is tried as a root, exactly, and each root found is given to the
nearest found eigenvalue not yet made exact, the polynomial divided by it
once for each time it is a root. An exact A whose eigenvalues are each so
found is answered even where long double found a complex pair, as it does
for a defective A's repeated eigenvalue; irrational eigenvalues stay long
doubles.** So eig([2, 1; 1, 2]) is [1, 3], exact, as SymPy's
`eigenvals` gives them, and [1, 1; -1, 3]'s eigenvalue 2, a double one
that long double finds as a complex pair, is found twice, [2, 2]; a
symmetric matrix's spectrum is real, and Jacobi's is accurate to its
norm's epsilon.
- Alternative: eigenvalues in long double alone, NumPy's; or exact
  algebraic numbers for every eigenvalue, SymPy's roots, which needs a kind
  of algebraic numbers.

> **Review.** Taken: for, a symmetric matrix's eigenvalues by Jacobi,
> accurate to its norm's epsilon; any other's by the C core's Hessenberg QR;
> an exact matrix's rational eigenvalues exact, found as roots of its
> characteristic polynomial in exact arithmetic, each as often as it is
> repeated, so that a defective matrix's repeated eigenvalue, which
> floating-point QR finds as a complex pair, is answered; against, a real
> matrix with complex eigenvalues is a domain error, a dense defective or
> nearly repeated eigenvalue can be found as a complex pair and so be a
> domain error too, irrational eigenvalues of an exact matrix are long
> doubles, not exact algebraic numbers, and a boxed inexact matrix's are
> computed in long double, past which its kind may hold more digits. The
> alternatives: long double alone would round eigenvalues that are exact
> rationals; SymPy's exact roots need a kind of algebraic numbers.
> Recommended: as taken, with complex eigenvalues of the complex kind, as
> the owner ruled on 2026.10.27 (S265, S273), the C core's Francis's
> double-shift QR, done in chunk 38 (S292), finding each pair; the review's
> interim, a pair as two rows, is withdrawn. Taken in chunk 43, step 23's
> third part: every eigenvalue, complex where it is, a Hermitian matrix's
> real, an exact matrix's exact where it is a Gaussian rational (S352).

**S259. A ^ (-n) of a square matrix is its inverse's power, inv(A) ^ n,
exact for an exact A, in its type for a dense one, a pole where A is
singular; to a floating-point -n as to a floating-point n (S248). `pow(A,
-n)` is the same.** S248's review asked for it once 3c brought the inverse.
- Alternative: negative powers a domain error still, inv(A) ^ n written
  out.

> **Review.** Taken: for, A ^ (-n) means what linear algebra means, inv(A) ^
> n, as MATLAB and Julia compute it, a singular matrix's a pole; against,
> none. Recommended: as taken. Taken.

**S260. The empty matrix is 0 by 0, a matrix of values with no element,
written `[]`, which is also its literal, the term vector of no element. A
shape of no row or no column but not neither stays bad_shape. Its arithmetic
follows the rules of shapes: [] + [], -[], 2 * [], [] * [], []', [] ^ n,
inv([]) and eig([]) are [], solve([], []) is [], det([]) is 1, trace([]),
norm([]) and dot([], []) are 0; it equals itself alone; an index of it and
cross of it are domain errors.** These are the values Julia and NumPy give
(det of the empty matrix is the empty product), and the empty matrix is
what an operation that removes every row gives, a block skipped in a
literal (S261).
- Alternative: no empty matrix, as S241 had it; or empty matrices of every
  shape, n by 0 and 0 by n, as MATLAB and NumPy keep them, which a
  language whose literals have no shape of their own cannot write.

> **Review.** Taken: for, the values Julia and NumPy give the empty matrix,
> det the empty product 1 and trace the empty sum 0, and a block that
> vanishes in a literal; against, one empty matrix, 0 by 0, where MATLAB and
> NumPy keep n by 0 and 0 by n, so the shape of an empty result, A's rows
> with none of its columns, is lost. The alternative, no empty matrix, would
> make every operation that removes all rows an error. Recommended: as
> taken; n by 0 shapes once an operation makes them (slices, null spaces),
> with a literal such as `zeros(n, 0)`. Taken.

**S261. A literal's elements are blocks: in a row, numbers and matrices
stand beside one another, each of the first's rows, its columns theirs in
turn; a matrix literal's rows stand one above another, each of the first's
columns; so [A, b] is A with the column b beside it, [A; B] is A above B,
and [[1, 2], 3; 4, 5, 6] is [1, 2, 3; 4, 5, 6]. A number is a block of 1 by
1, the empty matrix is skipped, and blocks that do not fit are a domain
error, as in arithmetic of shapes. The parser can know a row's width only
where each element is surely a number, a term of number atoms and no
literal: it reports a ragged row, bad_shape at its end, only where the
first row and the row are both such; otherwise the shapes are the fold's.
In a type a literal of blocks has no value, as any literal. A literal of
blocks is never square by its structure in simplification.** MATLAB,
Julia and Eigen's comma initializer write blocks so, and S241's review asked
for them by 3c; a literal of numbers alone reads, prints and errs as it
did.
- Alternative: no blocks, a literal's elements numbers alone; or blocks in
  a function of their own, `hcat` and `vcat` as Julia names them.

> **Review.** Taken: for, MATLAB's, Julia's and Eigen's block literals, [A,
> b] and [A; B], with every literal of numbers alone read, printed and
> refused as before; against, a ragged literal of variables, [x, 2; 3], is
> refused only when folded, a domain error, not bad_shape at the parse,
> since x may be bound to a matrix. The alternatives: no blocks would leave
> [A, b] to a function of its own; Julia's `hcat` and `vcat` are spellings
> beside the literal, not instead of it. Recommended: as taken. Taken.

**S262. The C core's `d_mat_power` squares as the language does: from the
exponent's highest bit down, the power begun as A and at each bit below it
squared, then multiplied by A on the left where the bit is 1; and so do the
generic kernel's power and `dynamic_matrix::power`. One algorithm, so that
a dense power is the same bits in the language, in `linear_algebra` and in
the C core.** S248's review asked for it; `d_mat_power` took n - 1
products, A ^ (k - 1) A, and now takes at most 2 log2(n). Its value
changes in its last bits for n > 2, the change S248 recommended; t_mat.c
checks it against the binary method and, exactly, against A A ^ (k - 1)
where the products are exact.
- Alternative: the C core's n - 1 products kept, the language's power
  differing from it (S248's flag).

> **Review.** Taken: for, one algorithm for one power, so a dense power is
> the same bits in the language, in `linear_algebra` and in the C core, at 2
> log2(n) products; against, `d_mat_power`'s value for n > 2 changes in its
> last bits from chunk 35's, which a caller comparing bits with an earlier
> result would see. The alternative keeps S248's two answers. Recommended:
> as taken, S248's recommendation. Taken.

**S263. The face test grows in place: its function table holds the eight
functions; seven new parts check the functions of exact and dense matrices
against the C core's algorithms bit for bit, and each storage's type;
eigenvalues, exact, irrational, repeated, defective and complex; negative
powers, and every dense power against `d_mat_power`, bit for bit; the
empty matrix; literals of blocks in the parser, as values and in
simplification; the functions in both folds; and 1,000 random exact
matrices of 1 to 4 rows against the test's own arithmetic: det by cofactor
expansion, inv and solve by A inv(A) = I and A x = b, each exact eigenvalue
a root of det(A - x I), and a triangular matrix's eigenvalues its diagonal.
t_mat.c checks `d_mat_power` against the binary method, and
t_matrix_face.cpp the generic kernel's power against the C core's.** The
oracle is the definitions in the value's arithmetic, apart from the
kernels, as S254's; its generator is its own.
- Alternative: hand-computed matrices alone; or the random matrices
  compared with NumPy's answers, which the workspace cannot run in the
  queue.

> **Review.** Taken: as S254, an oracle apart from the kernels, the
> definitions in the value's own arithmetic, with each dense type's
> algorithms checked bit for bit against the C core's own functions.
> Recommended: as taken. Taken.

**S264. The checks: as S255, against chunk 35's delivered tree, with 753
planted faults in math/expr/: chunk 35's 648, 22 of them moved with the
passages step 21 wrote again, and 105 in step 21's code, run in parts; 11
more in the power by squaring of the C core, the kernels and
`dynamic_matrix`, caught by their own tests; the full tier for the 28 tests
of the module the C core's change reaches; the fuzz as chunk 35's, with a
fourth part, random terms of the functions and of blocks against the fuzz's
own arithmetic; and the comparison with chunk 35's random texts, every line
the same but those chunk 35 answered as unknown functions or a ragged
literal, each of which must read otherwise now.**
- Alternative: the face test's ladder alone, which cannot see the C core's
  change.

> **Review.** Taken: as S255, with the full tier for the module's tests the
> changed headers reach, as the C core changed, and the power's faults in
> the C core, the kernels and `dynamic_matrix` beside math/expr/'s.
> Recommended: as taken. Taken.

**S265. Complex numbers and quaternions become kinds of the value, in three
steps added to Phase 3 before Phase 4: 3d (step 22), the C core's complex
numbers and the linear algebra they complete; 3e (step 23), the complex
kind in the language; 3f (step 24), quaternions, in the C core and the
language. The roadmap has 51 steps, Phase 4's 4a is step 25, and every
step after it moves three further on. S265 to S278 are the owner's rulings
of 2026.10.27, made during chunk 36 on its review's questions, and the
design of those three steps; where he left a choice to the lane ("whatever
allows the most accuracy, functionality and versatility ... the smallest
chance of needing to be revised"), the ruling is the review's
recommendation.** Phase 4's derivative rules and simplification's declared
domains (S274) are then written once, for the numbers they will keep. The
stubs of `math/expr/` name the old step numbers in their `not_yet`
messages, as the test checks them, until step 22 renumbers both.
- Alternative: complex numbers before Phase 4 and quaternions after it, 50
  steps; or both after Phase 4, 4a's rules and simplification's guarantee
  then written again; or both left in the roadmap's "Later", `eig`'s flag
  kept.
- Alternative: no complex kind yet, each complex eigenvalue given as its
  real and imaginary parts, a matrix of two rows (S258's review's interim),
  `eig`'s answer changing its shape now and again when the kind comes.

> **Review.** Ruled: for, Phase 4's calculus written once for numbers that
> may be complex or quaternions, the eigenvalue flags (S45, S47, S49, S258)
> cleared within two steps, and no interim answer to revise; against,
> calculus three steps later, and the roadmap's step numbers changed, which
> the notes and the stubs' messages must follow. Recommended: as ruled.
> Ruled; steps 22 to 24.

**S266. Every number kind past the real ones is made by one set of rules,
complex numbers and quaternions first, intervals and the others of S278
after them, so that no kind revises another. Its parts are of one real
kind: exact where they are given exact, and else of one floating-point
type, the wider of its parts', as the tower takes an operation's operands.
It is stored densely, as the C core's own type, where that type is float,
double or long double, and as values otherwise, as a matrix is (S239). It
has a rank of its own past the kinds it holds, a value of a kind below it
taken into it exactly. An exact result is made the simplest kind that
holds it exactly; an inexact one never is, its zero parts kept with their
signs. Each function gives its principal value, with the branch cuts and
signed zeros of C99's Annex G, after Kahan. Each operation is as accurate
as the best published algorithm for it, its bound written in its header
and checked against references of high precision; a `_fast` form is added
only where a use asks for speed, rendering among them. The C core's type
is a struct of C99, never `_Complex`, each function with its `_c` form; a
C++ face over it, from C++98, converts to and from the standard library's
type. A name the language reserves for a kind answers `not_yet` from the
step before its own (S197).** The rules are the lane's own, G2's
exactness and G6's one answer, carried to each kind.
- Alternative: each kind designed on its own, as libraries grew theirs:
  NumPy's complex numbers of doubles and floats alone, Eigen's quaternions
  of one floating-point type, SymPy's of symbols.

> **Review.** Ruled: for, one design for every kind to come, so that none
> revises another; the lane's exactness and one answer (G2, G6) in each; a
> kind's dense storage the C core's own type, its bits the C core's;
> against, more work for each kind than one type of doubles, and a boxed
> allocation for an exact complex number or quaternion. Recommended: as
> ruled. Ruled; from step 22.

**S267. The C core's complex numbers are a struct per floating-point type,
`d_complex_d` and its float and long double kin, whose one member is an
array of the two parts, real first, so that it has the layout of `T[2]`,
which C99's `_Complex T` and C++'s `std::complex<T>` share, and converts to
either by a copy; passed and returned by value, as C99's functions pass
theirs; in a header of its own in `c/math/`, made for the three families by
`tools/gen_c_math_float.py`. Its functions are the 22 of C99's
`<complex.h>`, `creal` to `catanh`, each in a `_c` form, a constant
expression from C++14, and a plain one, both built on the C core's real
functions and never on the C library's complex ones, with the real
`log1p`, `expm1`, `asinh`, `acosh` and `atanh` they need, which the C core
lacks (S203's review), each in both forms; conversions to and from
`_Complex` are given where the compiler has it, and nothing needs them.**
C11 makes `_Complex` optional, MSVC has none, and C++ has no such type, yet
the C core's headers compile as C++ too (the C++ ladder's 24 C headers); a
struct of an array has no padding between its parts, which a struct of two
members is not promised.
- Alternative: `_Complex`, C99's own type: optional in C11, absent in MSVC
  and in C++.
- Alternative: a struct of two members, `re` and `im`: the layout of `T[2]`
  on every ABI the lane meets, but not by the standard.
- Alternative: the plain forms on the C library's `cexp` and the rest:
  absent where `_Complex` is, and of uneven quality.

> **Review.** Ruled: for, one type in C99, C11 with or without `_Complex`,
> MSVC and C++, of the layout of both standard types; the plain and `_c`
> forms on the core's own real functions, so one algorithm everywhere;
> against, a struct, not the language's operators, in C. Chunk 37 names them
> `struct d_cplx_d` and its kin in `cplx.h` (S280). Recommended: as ruled.
> Ruled; step 22.

**S268. The complex arithmetic and functions are as accurate as the best
published algorithms, each bound in its header. A product's parts are each
an exact product less another, rounded once, Kahan's way, within 2 ulps of
each part (Jeannerod, Louvet and Muller), by fma where the target has it
and Dekker's split where it has not and in a constant expression; a
quotient is Baudin and Smith's robust division; the modulus a hypot scaled
by a power of two, as S256 scales the norm. `log` near the unit circle
takes log1p of the parts' squares formed exactly; `asin`, `acos` and their
hyperbolic kin are Hull, Fairgrieve and Tang's; `exp`, `sin`, `cos` and the
rest are formed of their real parts, scaled where a part would overflow; a
whole power is repeated squaring, any other exp(w log z). The branch cuts
and the principal values are C99's Annex G's, after Kahan: `sqrt` and `log`
cut along the negative real axis, continuous from above, the sign of a zero
imaginary part choosing the side, so that sqrt(-4 - 0i) is -2i; infinities
and NaNs as Annex G gives them, a value with an infinite part an infinity
whatever its other part. Each is checked against references of 50 digits
or more, made by mpmath in the step's tools.** This is the owner's "both"
of his sixth answer, built with the kind.
- Alternative: the textbook formulas, as many `std::complex`s compute: a
  part lost where products cancel, an overflow past about 1e154 in double
  where the result is in range, and infinities taken for NaNs.

> **Review.** Ruled: for, each part's error bounded and stated, the cuts and
> special values of the standards every user of C, C++, Julia and Common
> Lisp meets; against, the cost: an exact product is two to four operations
> more than a plain one, Dekker's split more still, and the x87's `long
> double` fma is the library's software. Chunk 37 forms the product without
> fma in every build and the quotient by exact pairs (S281), and the plain
> pow's general case as the `_c` one (S283). Recommended: as ruled. Ruled;
> step 22.

**S269. Complex numbers complete the C core's linear algebra, in step 22.
A real general matrix's eigenvalues are found by Francis's double-shift QR,
LAPACK's `dlahqr`, after balancing, LAPACK's `dgebal`, a diagonal
similarity by powers of two, and given as complex numbers, each pair
exactly conjugate and each real eigenvalue's imaginary part exactly +0;
its eigenvectors come from the real Schur form by back-substitution,
LAPACK's `dtrevc`, complex for a complex pair (S45's, S47's and S49's
recommendations). A complex matrix has its LU with partial pivoting,
determinant, inverse and solution; a Hermitian one its eigenvalues, real,
by Jacobi's method with complex rotations; any other its eigenvalues by
single-shift complex QR, LAPACK's `zlahqr`. The C core's norms scale as
S256 recommends, `d_vec_norm` and its `_c` form, and with them `normalize`
and the complex modulus; both kernel families, the C core's and the
generic, have an elimination of [A | B] beside `inverse` (S257's). The
generic kernels keep each product in its order, so that they hold for
quaternions; and `number_traits` tells a field's traits apart from an
order's, a magnitude to pivot by where a number has no `<`, taking S157's
and S158's recommendations, an exact integer root and a relative
tolerance, as the traits are written again. `linear_algebra`'s
`eigen_general` gives complex eigenvalues and eigenvectors beside the real
ones it gives now.**
- Alternative: complex QR of a real matrix: four times the work, pairs not
  exactly conjugate, and real eigenvalues with imaginary parts of rounding,
  which an inexact complex number keeps (S266).
- Alternative: LAPACK's two arrays of the parts, `wr` and `wi`: the same
  numbers, in two arrays where one type now holds them.

> **Review.** Ruled: for, LAPACK's algorithms for each kind of matrix, pairs
> exactly conjugate and real eigenvalues exactly real, so that an inexact
> complex number's zero imaginary part is one, and four flags cleared (S45,
> S47, S49, S256) with S257's and S157's and S158's recommendations;
> against, the C core's largest step yet, four new kernels beside the
> complex functions. Recommended: as ruled. Ruled; step 22.

**S270. `math::complex<T>`, of float, double and long double, from C++98,
is the C core's struct's face, as `math::value` is the value's: its
operators and functions are the C core's, the same bits on every compiler
and at every level (G6), constexpr where the `_c` forms are, from C++14. It
is made from a `std::complex<T>` implicitly and gives one by a member, not
implicitly, so that no expression of the two types is ambiguous; and it
has the `number_traits` of a field, so that `linear_algebra`'s vectors and
matrices of it compute.** The standard's type is no face of the core, as
its arithmetic is each standard library's own.
- Alternative: `std::complex<T>` itself: each library's arithmetic, other
  bits on each, and no constexpr before C++20.

> **Review.** Ruled: for, the core's bits through the face on every
> compiler, which `std::complex` cannot promise, and its constexpr from
> C++14; against, a second complex type beside the standard's, converted at
> the edges. Recommended: as ruled. Ruled; step 22.

**S271. The language's complex kind, in step 23, is a boxed kind of rank
900, past every real kind's and below the matrix's 1000, in the room S238
left, its parts, storage and exactness as S266 makes them. `im`, a
constant of the table, is its imaginary unit, `3 + 4im` by implied
multiplication; `complex(x, y)` is the number of those parts, as C11's
`CMPLX` makes the values a sum cannot, a signed zero or an infinity in one
part. `real`, `imag`, `conj` and `arg` are functions of the table, and
`abs` and the bars give the modulus. Every function of the table that has
a principal complex value takes a complex argument: `sqrt`, `exp`, `exp2`,
`log`, `log2`, `log10`, `pow` and `^`, the trigonometric and hyperbolic
functions and their inverses, `log1p`, `expm1`, `asinh`, `acosh` and
`atanh` joining the table; those of the real line's order, `cbrt`,
`floor`, `ceil`, `round`, `trunc`, `fmod`, `remainder`, `mod`, `atan2`,
`hypot` and the factorial, give a domain error of a non-real one. `=` and
`!=` compare the parts; `<`, `<=`, `>` and `>=` are a domain error of any
complex operand, as in Julia and Python. Each is exact wherever its value
is a Gaussian rational: (1 + 2im) (3 - im) is 5 + 5im and sqrt(2im) is
1 + im. In a floating-point type `im` is a NaN, as sqrt(-1) is there; and
the folds take `math::complex<T>` as a type of their own, each function its
complex form there. A complex number is written `3 + 4im`, bracketed where
its place asks, and reads back as a term of its value. `im`, `jm` and `km`
(S275) answer `not_yet` from step 22.** `i` would be lost as a variable's
name, and `A[i, j]` with it.
- Alternative: `i`, a constant of the table, never again a variable
  (S202); or a suffix, `2i`, which reads today as 2 times i (S197).
- Alternative: complex numbers ordered by their real parts, or
  lexicographically, as NumPy sorts them: an order no field has.
- Alternative: no fold in `math::complex<T>`, the value's alone, each
  number boxed.

> **Review.** Ruled: for, a complex number of every kind the tower has,
> exact Gaussian rationals among them; `im` costing one variable's name,
> where `i` would cost every index; the folds in `math::complex<T>` as fast
> as the real ones; the relations refusing what no field orders; against, an
> exact complex number is boxed and allocates, and a variable named `im` is
> lost. Recommended: as ruled. Ruled; step 23.

**S272. A real function of a real argument outside its real domain gives
its principal complex value as a value: sqrt(-4) is 2im, exactly, log(-1)
is pi im, and (-8) ^ (1/3) is 1 + 1.732...im, as MATLAB, Mathematica,
SymPy and Python's `**` give them; `cbrt(-8)` stays -2, the real root, and
in a floating-point type each is C's NaN, as now. An option of the
evaluation's context, `expr_context`, makes them domain errors instead, as
Julia's and C's are, for a computation meant to stay real; it comes with
the kind, in step 23.** The value's kind already follows its value, 1/3 a
rational and a product of matrices a matrix; so a complex result where no
real one exists is the tower's rule carried one kind further.
- Alternative: a domain error unless the argument is already complex, as
  Julia and C give it, `sqrt(complex(-4))` for 2im.

> **Review.** Ruled: for, every function answered where a value exists, as
> the systems the language is measured against answer; the tower's rule
> carried to complex values; and an option for a computation meant to stay
> real; against, where the option is off a real computation that strays
> negative goes complex silently, and `^` gives the principal root, 1 +
> 1.732...im for (-8) ^ (1/3), where `cbrt` gives -2. Recommended: as ruled,
> the option given with the kind's functions, in step 23's second part,
> chunk 41 (S324), which gave it, `expr_context`'s `real_results` (S340).
> Ruled 2026.11.04: S272 follows S340's ruling, the option on each
> function's evaluator, made in chunk 46 (S385). **Flag:** significant
> drawback, documented: a stray negative goes complex silently unless the
> option is set.

**S273. Matrices of complex numbers, in step 23, are dense where every
element is a floating-point number of one type, complex or real, the C
core's structs in a row-major array, as LAPACK lays them, and values
otherwise: S239's storages and three more. `A'` is the conjugate
transpose, as in MATLAB and Julia, the same as now for every real matrix,
and `transpose(A)` the transpose; `dot(a, b)` is the sum of conj(a_i) b_i,
as Julia's `dot`, NumPy's `vdot` and BLAS's `zdotc`, so that dot(a, a) is
norm(a) ^ 2; `norm` is of the moduli, scaled (S256), and `cross`
conjugates nothing. `det`, `inv`, `solve` and `trace` are S269's kernels'
or exact; `eig` of a Hermitian matrix is Jacobi's, real, of a real one
Francis's QR's, and of any other complex QR's. Eigenvalues ascend by real
part, then by imaginary part, as Julia sorts them, a conjugate pair side by
side, the negative first; a real spectrum is real, as now (S258), and a
spectrum with a complex pair complex throughout, each real eigenvalue's
imaginary part +0. An exact matrix's eigenvalues that are Gaussian
rationals are exact, `eig([0, -1; 1, 0])` being [-im, im]: d A's
characteristic polynomial is monic over the Gaussian integers, so each
such eigenvalue's parts are multiples of 1/d, and the nearest such
candidate of each eigenvalue found is tried exactly, as S258 tries the
rational ones; the rest are long doubles.**
- Alternative: `A'` the transpose, as now, and the conjugate transpose a
  function of its own, as NumPy's `.T` and `.conj().T` are: unlike
  MATLAB's and Julia's `A'`, which their users write for the adjoint.
- Alternative: `dot` unconjugated, NumPy's `dot`: dot(a, a) then not a
  norm's square.
- Alternative: eigenvalues unsorted, as LAPACK, NumPy and MATLAB give
  them: their order the algorithm's.

> **Review.** Ruled: for, the conjugate transpose and the conjugating `dot`
> of MATLAB, Julia and BLAS, the same as today's for every real matrix; each
> kind of matrix its own algorithm; eigenvalues in an order of their own, a
> real spectrum real; exact eigenvalues wherever they are Gaussian
> rationals; against, `A'` and `dot` of complex matrices differ from NumPy's
> `.T` and `dot`. Recommended: as ruled. Ruled; step 23, made in chunk 43
> (S348 to S352).

**S274. Simplification's variables have declared domains, in step 25 (4a),
widening S251's recommendation. A variable not declared is a complex
number, which commutes, so that no rewrite changes a value at any complex
binding, as each of today's rules already holds over the complex numbers;
simplification's guarantee is then of the complex numbers, not the real.
A declaration makes a variable real, positive or an integer, a rule true
only there then applying to it, sqrt(x ^ 2) being |x| of a real x and x of
a positive one; or a complex number; or a quaternion, or a matrix of a
shape where known, these two keeping their order among the factors as
their literals do (S251). The derivative of a function with no complex
derivative, `abs`, `conj`, `real`, `imag`, `arg` or `sign`, is
`not_differentiable` of a variable not declared, and the real derivative
of a real one, as SymPy's and Mathematica's derivatives take a symbol to
be complex.**
- Alternative: variables real unless declared, as the guarantee is
  written now: the derivative of |x| sign(x) with no declaration, but
  sqrt(x ^ 2) = |x| wrong at x = im once a rule of the reals is added.
- Alternative: declared matrix variables alone (S251's review).

> **Review.** Ruled: for, no rewrite ever wrong at a binding a value can
> have, and the real rules where a variable is declared so; S251's matrix
> variables one case of a general declaration; against, the derivative of
> `abs` of a real variable needs its declaration. Recommended: as ruled.
> Ruled; step 25.

**S275. Quaternions are a boxed kind of the value, in step 24: Hamilton's,
w + x im + y jm + z km with im jm = km, of rank 950, past the complex
kind's and below the matrix's; a complex operand is taken in with its `im`
the quaternion's first unit. Its parts, storage and exactness are as S266
makes them, dense as the C core's array of four in its order, [x, y, z,
w], the order of glTF, Eigen and GLM, so that a renderer takes one as it
is, and values otherwise. An exact quaternion whose jm and km parts are
exactly 0 is complex, and real where its im part is too; an inexact one
stays a quaternion. `jm` and `km` are constants beside `im`; `quat(w, x,
y, z)` makes one of its parts and `parts(q)` gives them; with `real`,
`conj`, `abs` and the bars, `inv`, `sign`, `exp`, `log`, `sqrt` and `^` of
a real exponent, each its principal value: log q = log|q| + u atan2(|v|,
w), v = |v| u its vector part, u = im where v is 0, so that log(-1) is pi
im, as a complex number's is. A non-real exponent is a domain error, as
its two orders give two values. For rotations, `rotate(q, v)` of a
3-vector, `slerp(p, q, t)`, the rotation matrix of a unit quaternion and
the quaternion of a rotation matrix; their names are settled in the
step.**
- Alternative: `i`, `j` and `k` as constants, never again variables
  (S202); or quaternions a C++ type alone, never a value, so that no term
  holds one.
- Alternative: the order [w, x, y, z], the mathematician's and Boost's: a
  copy for each call a renderer makes.

> **Review.** Ruled: for, quaternions of every kind the tower has, a complex
> number one of them; the layout renderers pass, so the C core's array goes
> to them as it is; principal values that agree with the complex ones;
> against, two more names, `jm` and `km`, lost to variables, and an exact
> quaternion boxed. Recommended: as ruled. Ruled; step 24.

**S276. The C core's quaternions, `quat.h`, keep their conventions and
grow, for the kind and for rendering, in step 24: Hamilton's product,
right-handed, a rotation active, v' = q v q*, as glTF's, Eigen's and GLM's
are, never JPL's; the array [x, y, z, w], 16 bytes in float, which a
renderer can align, upload and vectorize as it is. Added are the inverse,
the right and the left quotient, exp, log, sqrt and powers of a real
exponent; the scaled norm (S256) in `normalize`; a rotation matrix's
quaternion by Shepperd's method, the largest of four candidates; the
quaternion turning one vector to another, the antiparallel case among
them; nlerp, and slerp along the shorter arc, q or -q, accurate near angle
0. The product's four parts are each a compensated sum of four products,
as accurate as one computed in twice the precision and rounded once;
today's formulas stay as the `_fast` forms of the product, the rotation
and `normalize`, the last by a reciprocal square root, for a renderer's
inner loops, in the same layout and conventions (6e's rule, its use the
owner's). A function with a sine or a cosine has its `_c` form, as now,
and `quaternion.hpp`'s faces take each.** Today's callers of the product
and the rotation get other last bits, more accurate; the `_fast` forms give
today's.
- Alternative: today's formulas as the default, an accurate form beside
  them: a renderer's speed by default, but a part's error unbounded where
  products cancel.
- Alternative: dual quaternions now, for rigid motions and skinning: a
  pair of quaternions, left until a renderer asks (S278).

> **Review.** Ruled: for, the conventions of glTF, Eigen and GLM kept; each
> part of a product bounded by default; a renderer's speed by the `_fast`
> forms, of the same layout; the functions a renderer and the kind need,
> Shepperd's and a shorter-arc slerp among them; against, today's callers'
> bits change, and a compensated product is about three times the operations
> of today's. Recommended: as ruled. Ruled; step 24.

**S277. Quaternions do not commute, in step 24: p / q is p inv(q), the
quotient on the right, as MATLAB's `/` divides, and inv(q) p is written
so. Simplification keeps a quaternion's factors in their order, as a
matrix's (S251), and `im` among them in any product that holds `jm`, `km`
or a quaternion, since im jm is km and jm im is -km. A matrix of
quaternions has sums, products, the conjugate transpose `A'`, and `inv` and
`solve` by Gauss-Jordan in each product's order (S269's kernels); `eig`
gives its standard right eigenvalues, complex numbers of imaginary part 0
or more, those of the complex matrix of twice its size that represents it
(Zhang's), by S269's complex QR; and `det` is a domain error, as no
determinant of quaternion matrices is both multiplicative and the complex
one on complex matrices.**
- Alternative: `eig` of a quaternion matrix a domain error too: no answer
  where Zhang's gives one.
- Alternative: p / q the quotient on the left, inv(q) p, MATLAB's `\`.

> **Review.** Ruled: for, one quotient, the right one, as MATLAB's; the
> order of factors kept wherever it matters; Zhang's eigenvalues, the
> standard answer, on the complex QR step 22 brings; against, no
> determinant, as none is both multiplicative and the complex one.
> Recommended: as ruled. Ruled; step 24.

**S278. The other kinds the lane may need, each by S266's rules: intervals,
a boxed kind as the roadmap's design plans it, their endpoints of one
kind, exact ones exact and floating-point ones rounded outward, their step
the owner's to place; algebraic numbers, exact roots such as sqrt(2), so
that `eig` and a polynomial's roots are exact throughout, after Phase 8's
big integers, which their coefficients need; and dual quaternions, for
rigid motions and skinning, a C core type and a C++ face when a renderer
asks. Not kinds: octonions, which do not associate, so that a chain a b c
would have no one value, and which nothing planned needs; Clifford
algebras, one general kind harder to make accurate and fast than complex
numbers and quaternions are each; and dual numbers, as Phase 4's
derivatives are symbolic and exact.** Each waits in the roadmap's "Later"
until it is placed.
- Alternative: one kind of Clifford algebras for complex numbers,
  quaternions, dual numbers and the rest; or kinds made as uses ask, with
  no rules in common.

> **Review.** Ruled: for, every kind to come made by S266's rules, so that
> none needs a design of its own; the kinds no use asks for left out, each
> for a reason; against, intervals and algebraic numbers still without a
> step. Recommended: as ruled. Ruled; each placed when the owner places it.

### S279 to S290: chunk 37, 2026.10.27

The record: `_handoffs/math98_chunk37_2026.10.27/README.md`.

S279 to S290, chunk 37's, as `decisions.md` has them; each is reviewed
there, none flagged, and the 22 outstanding decisions of S1 to S290 come
first, by group and urgency, as chunk 36's made them (section 2.6). The
text below is `decisions_c37.md`, which `decisions_c37.py` reads.

**S279. Step 22 (3d) is made in three chunks, each delivered and checked
whole: chunk 37 the C core's complex numbers, `cplx.h`, with C99's 22
functions and the five real functions they need (S280 to S288); chunk 38
the linear algebra S269 names, Francis's double-shift QR after balancing,
the eigenvectors of the real Schur form, complex LU, inverse and solution,
Hermitian Jacobi and complex QR, the C core's norms scaled (S256) and its
elimination of [A | B] (S257), and `number_traits` written again (S157,
S158); chunk 39 `math::complex<T>` (S270), `im`, `jm` and `km` answering
`not_yet` (S271, S275), the stubs' step numbers made new (S265), and the
guard chunk 36's faults asked of `t_expr_face.cpp`'s empty matrix.** Each
part is a chunk the record can check whole, its references and its faults
with it; the step's flags, S45, S47, S49, S256, S257, S157 and S158, close
with chunk 38, which needs the complex type this chunk makes.
- Alternative: step 22 in one chunk, the C core's largest change yet in
  one delivery, its parts' faults and references checked together.

> **Review.** Taken: for, each part checked whole, its references and faults
> with it, and the step's flags closing in its second part, which needs the
> type the first makes; against, three deliveries for one step. Recommended:
> as taken. Taken.

**S280. The C core's complex numbers are `struct d_cplx_f`, `struct
d_cplx_d` and `struct d_cplx_ld`, in `c/math/cplx.h`; their functions are
`d_cplx_<S>_<fn>` and `d_cplx_<S>_<fn>_c`, `d_cplx_d_exp` and
`d_cplx_d_exp_c`, and in C++ also one name for every family,
`d_cplx_<fn>` and `d_cplx_<fn>_c`, chosen by the argument's type, as
`d_math_<fn>` is for the real functions. Each struct's one member is
`part[2]`, as S267 has it, and none is typedef'd, as the C guide asks.
Beside C99's 22 functions, `creal` to `catanh` as `real` to `atanh`, are
`make`, `neg`, `add`, `sub`, `mul`, `div` and `equal` of complex numbers,
and `add_real`, `sub_real`, `real_sub`, `mul_real`, `div_real` and
`real_div` of a complex number and a real one.** This refines S267's
`d_complex_d`: the C core's other types are named short, `d_vec_`,
`d_mat_`, `d_quat_`, and a header named `complex.h` would be found for
C99's `<complex.h>` by a build with `c/math/` on its include path.
- Alternative: S267's names, `d_complex_d` in a `complex.h`: a prefix
  longer than the core's others, and a header that can shadow the
  standard's.
- Alternative: `d_complex_d` in `cplx.h`: the type named apart from its
  header and its functions.

> **Review.** Taken: for, the core's short prefixes kept, no header that can
> shadow the standard's, and one C++ name a template calls for every family;
> against, a name other than S267's, as ruled. Recommended: as taken. Taken.

**S281. The complex arithmetic comes once, pure, a constant expression
from C++14, with no plain form. A product's part is a sum of two exact
products, each split by Dekker's method, its low parts kept and the sum
rounded once, in every build: never fma, which no constant expression has
before C++23 and the x87's long double has only in software. A quotient is
the conjugate product over the squared modulus, both carried as exact
pairs, the operands first scaled by powers of 2 so that nothing on the way
overflows or underflows, and then a residual's step: q of the high parts,
the residual u - q n formed exactly of Dekker's product of q and n's high
part, and q plus the residual over n, rounded once. Where an operand is
infinite, Annex G's recovery (G.5.1): an infinity times a nonzero number is
an infinity, and a nonzero number over a zero is one. An operation of a
complex and a real number takes the real as itself, not as x + 0i (G.5): z
x is a x + b x i, so that a zero's sign survives and no infinity meets a
zero.** This refines S268's arithmetic: its product, Kahan's, formed
without fma, and its quotient by exact pairs and a residual's step rather
than Baudin and Smith's robust division, whose correction step keeps an
error of its own. Each part of either is within a step of the correctly
rounded one; at every reference point, double's and long double's at their
full width among them, each is the correctly rounded one (section 4), and
a float or double part within half an ulp of the exact value and 2^-8
more, as rounding once gives it, which t_cplx.c measures. Without the
residual's step - the quotient over n's high part corrected by its low
part, as the first version of this chunk's was - a double part is
1.3745 ulps from the exact value at a reference point, and a long double
part two steps off at a point of long double's full width.
- Alternative: S268's as it reads, fma where the target has it and Dekker's
  split where it has not, and Baudin and Smith's division: other bits on
  other targets, and no constant expression with fma.

> **Review.** Taken: for, the same bits in every build that evaluates in the
> type and in a constant expression, each part within a step of the
> correctly rounded one and measured correctly rounded at every reference
> point, and Annex G's infinities; against, a product of several times the
> operations fma's takes, a quotient of more still, and no fast form for a
> renderer's inner loops until one asks for it (S266). Recommended: as
> taken, a `_fast` product by fma when a use asks. Taken.

**S282. Of C99's 22 functions, real, imag, conj and proj, exact, come once,
pure, with the arithmetic (S281); each of the other 18 comes twice, as
every real function of the C core. The `_c` form is one kernel of pure
arithmetic in long double, on the core's `_c` real functions, rounded once
to the family: a float or double part is then within a step of the
correctly rounded one, as long double carries 11 bits past double's. The
three families share each kernel, which a new qualifier of `math_common.h`,
`D_MATH_FN_KERNEL`, compiles once in each translation unit: constexpr from
C++14, as `D_MATH_FN` is, but never forced inline. The plain form is the
same algorithms in the family's own type, on the C library's real
functions, fast at run time, its bound wider by the library's errors and
its own roundings (the header gives both forms' bounds).** Forced inline
into every caller, as the core's other kernels are, the shared kernels
would be compiled into each: a translation unit calling each of the fifteen
unary double `_c` functions once, exp to atanh, compiles in 52.0 s to
580,801 bytes of text, where shared it takes 3.4 s and 100,564 bytes, and
one calling `pow_c` alone in 8.8 s, where shared it takes 4.5 (section 4).
Where long double is double, as with MSVC, the `_c` forms keep their
kernel's error, as the real ones do (S8).
- Alternative: a kernel per family in its own type: three times the text,
  and a float or double `_c` part with its kernel's error, not one
  rounding.
- Alternative: the shared kernels `D_MATH_FN`, forced inline as the rest
  of the core's are: the compile time and the size above.

> **Review.** Taken: for, one algorithm for each function, rounded once for
> float and double, compiled once in a unit; against, long double arithmetic
> in a float's `_c` form, slower than float's own, and where long double is
> double the `_c` forms' bounds are the kernel's. Recommended: as taken.
> Taken.

**S283. The plain `pow`'s general case, w not whole and z neither real nor
imaginary, is the `_c` kernel's, rounded to the family: w log z formed as
long double pairs, its exponential taken of the pair, the angle by a sum.
The plain form is fast where pow is exact by other means: z^0, a whole w by
repeated squaring, a real or an imaginary z by turns of its argument, and
0^w.** S266 makes each operation as accurate as the best algorithm for it,
and a plain form fast beside the `_c` one but no less right. C99's exp(w
log z), in the family's own type, carries |w log z| times log's error into
the angle: at t_cplx's 198 points of pow it puts 123 double results past
the plain bound, 12 of them by more than a million steps, the farthest
right to 6 digits; float's and long double's alike (section 4). No C
library function forms w log z closer.
- Alternative: C99's formula, as glibc's `cpow` computes: fast, and past
  the bound at most of the points.

> **Review.** Taken: for, the plain pow as accurate as the `_c` one;
> against, its general case at the `_c` kernel's speed. Recommended: as
> taken. Taken.

**S284. The values at the cuts, the zeros, the infinities and the NaNs are
Annex G's (G.6), as C17 corrects them by its DR 471: cacosh(±0 + i NaN) is
NaN + iπ/2, the sign C17 leaves open taken +, and ctanh(±0 + i∞) and
ctanh(±0 + i NaN) are ±0 + i NaN, where C11 gave NaN + i NaN for each.
Where Annex G specifies a zero's sign, every form gives it, a modulus being
|x| + 0, so that a -0 never passes for +0: csqrt's real part +0 on the cut,
cacos's and cacosh's real parts +0, ccosh(±∞ ± 0i)'s imaginary part a
zero, catanh(x ± i∞)'s real part a zero of x's sign. Where it leaves a sign
open, the sign is the algorithm's. No floating-point exception is promised,
as a constant expression cannot raise one; the plain forms raise what
their arithmetic and the C library raise.** C17's three values keep the
identities Annex G's others keep, cacosh(z) = ±i cacos(z) and ctanh(iy) =
i tan(y), which C11's broke.
- Alternative: Annex G as C11 prints it, the three values NaN + i NaN.

> **Review.** Taken: for, C17's values, every sign Annex G gives given;
> against, no floating-point exception, which Annex G specifies, for a
> program that tests the flags. Recommended: as taken. Taken.

**S285. Where C has `_Complex` (no `__STDC_NO_COMPLEX__`), `cplx.h`
converts each family's struct to and from it, `d_cplx_<S>_to_complex` and
`d_cplx_<S>_from_complex`, by a copy of the two parts, without including
`<complex.h>`; nothing in the header needs them. C++ has none: its
`std::complex<T>` is `math::complex<T>`'s to convert (S270, chunk 39).**
- Alternative: no conversion, a C program copying the parts itself; or
  `<complex.h>` included, which defines `I` and `complex` as macros.

> **Review.** Taken: for, a C program passes values to and from the standard
> type, and the header needs none of `<complex.h>`'s macros; against, none
> in C++ until `math::complex<T>` (chunk 39). Recommended: as taken. Taken.

**S286. The C core gains `log1p` and `expm1`, in `exponential.h`, and
`asinh`, `acosh` and `atanh`, in `hyperbolic.h`, each in the three families
and in both forms, as its other real functions: the `_c` form one long
double kernel carrying its reduced argument and its sums as exact pairs,
rounded once to the family, a constant expression from C++14; the plain
form the C library's, and below C++11, which has none of the five, the
`_c` form; with C++'s one names. Annex F's values at the ends: log1p(-1)
is -inf and below it NaN, expm1(-inf) is -1, acosh below 1 NaN, atanh(±1)
±inf. expm1's kernel takes the series of e^r - 1 with its first three
terms exact, r, r^2/2 and r^3/6 as pairs, and past ln 2 / 2 x = k ln 2 +
r, so that e^x - 1 = (2^k - 1) + 2^k (e^r - 1), two terms summed exactly;
the exponential's own core takes the same series, 1 + (e^r - 1).** S203's
review recommended the five for this step, as the complex functions need
them (S267). At 20,000 arguments each, the long double `_c` forms of the
five and of exp are within 0.5183 of an ulp of the exact values, where
glibc's long double functions reach 0.9417 for log1p, 1.0367 for exp and
from 2.2983 to 2.4921 for the others (section 4). The first kernels of this
chunk took e^r with its series' rounding in the whole tail, and expm1 as
2^k e^r - 1, which loses e^r's leading bits near e^x = 1: expm1 was 0.8737
of an ulp off and exp 0.5596 (section 3).
- Alternative: the complex functions on the C library's real five, as the
  plain forms are: no constant expression, and the library's errors in the
  `_c` forms.

> **Review.** Taken: for, the five functions in the C core in every family
> and both forms, the complex functions' `_c` forms on them, and exp's and
> expm1's long double kernels within half an ulp but where the exact value
> lies near a midpoint, closer than the C library's; against, a C++ program
> below C++11 gets the `_c` form's speed from the plain one, and exp's `_c`
> results move where its old kernel rounded wrongly, each to the correctly
> rounded value. Recommended: as taken; the five in the table in step 23
> (S203). Taken.

**S287. Where a target evaluates float and double wider than their types,
as a 32-bit x87 does (math_common.h's `D_MATH_FLT_EVAL_WIDE` and
`D_MATH_DBL_EVAL_WIDE`), `cplx.h`'s float and double families take their
exact steps in long double, which the target rounds in its own type: a
product's and a quotient's parts, and the plain kernels' |z|^2 - 1 and 1 -
x^2 - y^2, as long double pairs, each result rounded once to the family.
Elsewhere they take them in their own types.** Dekker's split and the
two-sum need each operation rounded to the family's type, which pure
arithmetic cannot force where the target evaluates it wider: a value keeps
the register's bits or loses them as the compiler spills it, and a cast
rounds only in an ISO mode. The first version of this chunk's arithmetic,
in the families' own types everywhere, was hundreds of steps off on the x87
at points of double's full width where the parts cancel (section 3).
- Alternative: one operation in each statement, on the x87 as elsewhere:
  exact in an ISO mode, where an assignment rounds, but each rounding
  twice, to 64 bits and then to 53, and in a GNU mode, where nothing need
  round, no exactness at all.

> **Review.** Taken: for, the arithmetic and the plain kernels' exact steps
> exact on the x87 in every mode, ISO and GNU, C and C++; against, long
> double operations in a float or double product there, and an x87's last
> bit of a product or a quotient that can differ from SSE's where the two
> round a hair apart, each within its bound. Recommended: as taken. Taken.

**S288. `cplx.h`'s harness, `t_cplx.c`, checks every function against
references made by mpmath, kept in the tree as generated headers,
`math_tests_cplx_refs.h`, and for the five real functions
`math_tests_real_refs.h`, both made by the record's `refs_c37.py`, whose
`--check` keeps them in step. Each value is computed at 400 bits and again
at doubled precision until two agree, as mpmath's error is in places
absolute and a part far below 1 needs more bits; each part is compared with
the family's correctly rounded one, in steps, against a bound for each
function, family and form, the header's. The points: exact in float, shared
by the families; each family's own at the ends of its range; every other
shared point widened to double's and to long double's full width, as a
point exact in float makes every product of two parts exact in double and
leaves the exact steps' low parts at 0; and for log, at each of those
widths, |z| just below 1 with both parts just below 1/√2, which a widened
point may pass. A product's reference is its exact value, whole, so that a
tie reads as a tie, and the product's and the quotient's float and double
parts are measured in ulps of the exact value, too, against half an ulp and
2^-8 more (S281). Beside them, Annex G's values in a table, each checked
with the symmetries G.6 gives it, a sign it leaves open accepted either
way; G.6's definitions of casin, catan, ccos, csin and ctan by their
hyperbolic kin on a grid of special and ordinary parts; exact values; the
paths no reference reaches, so that every line of `cplx.h` runs; in C
`_Complex`'s conversions, in C++ the one names, and from C++14 each `_c`
form a constant expression with the run time's bits. The five real
functions' long double `_c` forms, and exp's, whose core takes expm1's
series, are held to 0.53 of an ulp of the exact value at points of long
double's full width, expm1's among them where its reduction's k is 1 or 2.
The plain forms are held to their bounds in every build, not to one bit
pattern: on the x87 a plain form's values keep or lose their extra bits by
where the compiler inlines it, as S39 finds of the library's results. Seven
harnesses' `d_tests_same_ld` compare zeros' signs as booleans, as glibc's
`signbit` of a long double gives 1 for a folded constant and 512 at run
time.** The full-width points found what the float points could not: the
x87's inexact products (S287), the quotient's two steps (S281), and log |z|
wrong near |z| = 1 where both parts lie near 1/√2 (section 2); and mpmath's
`atanh` at a fixed 400 bits loses a real part below 2^-400, which settling
finds. The fault trial found what the points could not: a double quotient
without its residual's step within its step at every point, which the
half-ulp measure finds; the plain double log with b^2 - 1 one value right
at every point, as no widened point of double's width had both parts below
1/√2 where b^2 - 1 is not one value; expm1's kernel without r's low part
within its bound at every point, as few lay where k is 1 or 2; and exp's
without its series' low part, which no point of long double's full width
measured.
- Alternative: references from the C library, or from another kernel of
  the lane's: neither independent of what it checks; points exact in float
  alone, which never round a product in double or long double.

> **Review.** Taken: for, references independent of every kernel and of the
> C library, kept in the tree and made again by the record; points at every
> family's full width; every line of `cplx.h` run; the real kernels held to
> a fraction of an ulp; against, the plain forms' bits not checked across
> inlining contexts on the x87, as S39 leaves them, and a header of
> references grown to some 28,000 lines. Recommended: as taken. Taken.

**S289. The checks: as S264, against chunk 36's delivered tree, made on the
owner's tree of 2026.10.27 (S290): the generator's and the references'
`--check`; t_cplx's builds of the full tier, and the full tier of every math
test, as `math_common.h` reaches them all; the smoke tier, every other
test's lines as chunk 36's; the three harnesses under the sanitizers; faults
planted in `cplx.h`, `exponential.h` and `hyperbolic.h`, one at a time,
each caught by its harness in every build that can reach it, the host's
and the x87's paths apart; which lines the harnesses run, on the host and
on the x87; what the shared kernels save; the real kernels' errors to the
fraction of an ulp, beside glibc's; C99's pow formula at the references;
the includers of the four headers; the C and C++ ladders and the lints
against the tree before; t_cplx at each optimization level; the note's
examples; and what the last harnesses find in the first version's
headers.**
- Alternative: the full tier of the three harnesses alone: `math_common.h`
  changed, and every test includes it.

> **Review.** Taken: as S264, with the full tier of every math test, as the
> change reaches every one, the x87's paths among the faults and the lines
> run, and the measurements the decisions cite. Recommended: as taken.
> Taken.

**S290. Chunk 37 stands on the owner's tree of 2026.10.27, which he sent
during the chunk, with the lane's files laid over it, as chunk 36 stood on
his tree of 2026.10.23; the tree before the change is chunk 36's delivered
tree made again on it, from the one zip of 2026.10.28, which holds chunk
36's copies of the files chunk 37 changed. At his word ("let's make a ZIP
that does have everything we need. One ZIP."), the delivery is that zip's
successor: every file of the working tree his tree has not, or holds
otherwise, the lane's files, every record but his own, and the
repository's own directories his tree does not carry, so that it alone,
over his tree, gives the tree the record checked.** The files of his tree
of 2026.10.27 that the lane's replace are each dated before his tree of
2026.10.23, the two he changed his own way are his still, and the smoke
tier's lines on the tree are chunk 36's (section 4).
- Alternative: chunk 36's rule, the lane's files and the records from
  chunk 16, which the lane's repository and his tree of 2026.10.23 make
  checkable; neither is at hand in this chunk.

> **Review.** Taken: for, the delivery applies over the owner's newest tree,
> alone, with no earlier zip; against, a larger zip, and the comparisons
> with the lane's repository and its commits, which chunk 36's checks made,
> not made in this chunk. Recommended: as taken, those comparisons made
> again when the repository is at hand. Taken.

### S291 to S311: chunk 38, 2026.10.30

The record: `_handoffs/math98_chunk38_2026.10.30/README.md`.

S291 to S311, chunk 38's, as `decisions.md` has them; each is reviewed
there, S301, S306 and S311 flagged, all minor, and the 18 outstanding
decisions of S1 to S311 come first, by group and urgency, as chunk 37's
made them (section 2.6). The text below is `decisions_c38.md`, which
`decisions_c38.py` reads.

**S291. Step 22's second part is the C core's linear algebra of the
complex type and the general eigenproblem: the real path in `decomp.h`
beside its other factorizations, and complex matrices in a header of their
own, `c/math/cmat.h`, its functions `d_cmat_<S>_<fn>` and, where a root is
taken, `d_cmat_<S>_<fn>_c`, as `decomp.h`'s are; C++'s one names for every
family, `d_cmat_<fn>`, as `cplx.h` has them.** `cmat.h` includes
`decomp.h` for the real helpers it shares - the scaled norms, hypot, the
scaling by powers of two - and `cplx.h` for the arithmetic, so that a
program wanting real matrices alone includes neither complex header's
kernels.
- Alternative: the complex functions in `decomp.h`, `d_decomp_<S>_c...`: one
  header, but every includer of the real factorizations compiling the
  complex ones too.
- Alternative: the complex functions in `cplx.h`: the scalar header grown by
  matrices, against the core's one subject to a header.

> **Review.** Taken: for, a program of real matrices compiles no complex
> kernel, the complex header the real helpers once, one subject to a header;
> against, a third header for the eigenproblem's complex half. Recommended:
> as taken. Taken.

**S292. A general real matrix's eigenvalues and eigenvectors are LAPACK's
`dgeev` path, each part a public function in the three families and both
forms: `d_decomp_<S>_balance` (`dgebal`, job 'B') and `_balance_back`
(`dgebak`); `_hessenberg_q`, Householder reflectors with Q accumulated
(`dgehd2`, `dorghr`); `_schur`, Francis's implicit double-shift QR
(`dlahqr`: Ahues and Kressner's deflation, exceptional shifts at every tenth
step without a split, `dlanv2`'s standard blocks of two); `_schur_vectors`,
back-substitution through the quasi-triangular form with `dlaln2`'s small
solves and overflow guards (`dtrevc`, job 'B'); and the driver,
`d_decomp_<S>_eigen(a, n, values, vectors, work, perm, max_iter, tol)`, its
eigenvalues `struct d_cplx_<S>` and its eigenvectors complex, column j the
vector of value j.** Each part is the algorithm LAPACK's reference
implementation takes, so that a reader can check it line by line against a
standard, and each is reachable alone: a caller who wants the Schur form,
or no balancing, composes the parts. The values and vectors are the C
core's complex numbers (S267), as the owner ruled.
- Alternative: LAPACK's storage, `wr` and `wi` arrays and real vectors with
  a pair in two columns, as `dgeev` returns them: no complex type needed,
  every caller unpacking pairs.
- Alternative: the driver alone, the parts internal: a smaller API, no
  Schur form for a caller who wants one.

> **Review.** Taken: for, LAPACK's reference algorithms, each checkable
> against it and each public, the values and vectors the C core's complex
> numbers, as ruled; against, a complex result for a real matrix every
> caller takes, and n (n + 4) entries of scratch. Recommended: as taken.
> Taken.

**S293. The driver's eigenvalues are sorted ascending by real part, then
imaginary: a complex pair exactly conjugate, side by side, the negative
imaginary part first where no real eigenvalue shares the pair's real part,
a real eigenvalue's imaginary part exactly +0. Each eigenvector is of unit
2-norm, its entry of largest modulus (the first among equals, by the
rounded squares) real and positive; a real eigenvalue's vector real, its
imaginary parts +0; the second of a pair the first's conjugate, bit for
bit.** One order and one normalization make the answer a function of the
matrix alone, what the language's `eig` and the C++ face's `general_eigen`
need to compare and to print, where LAPACK's order is its Schur form's,
which a change of algorithm or of scaling may move.
- Alternative: LAPACK's order and normalization, unit vectors with the
  largest component real (`dgeev`), as NumPy and MATLAB give them: the
  standard's own answer, its order the iteration's.
- Alternative: vectors scaled to a largest entry of 1, as `dtrevc` leaves
  them: no root, a vector not of unit length.

> **Review.** Taken: for, one answer for a matrix, whatever the iteration's
> order or the matrix's scale, a pair's vectors conjugate bit for bit;
> against, LAPACK's order lost (a caller wanting the Schur form's calls the
> parts), and a pair whose real part a real eigenvalue shares sorted around
> it. Recommended: as taken. Taken.

**S294. The driver always balances, as `dgeev` with 'B' does; the parts
are public, so that a path without balancing is the parts composed without
it.** Balancing, a diagonal similarity by powers of two with the
permutations that isolate eigenvalues, is exact and makes a badly scaled
matrix's eigenvalues as accurate as its condition allows; on a few
matrices whose small entries are not noise it can worsen an eigenvector's
residual measured against the matrix as given, which is why MATLAB offers
`eig(A, 'nobalance')`. The harnesses measure each vector's residual against
the balanced matrix, of whose norm the backward error is a few epsilons,
as LAPACK's is.
- Alternative: a flag to the driver, MATLAB's 'nobalance': a parameter
  every caller passes for a case few meet.
- Alternative: no balancing, as before: a badly scaled matrix's small
  eigenvalues lost to its large entries' rounding.

> **Review.** Taken: for, a badly scaled matrix's eigenvalues as accurate as
> its condition allows, by an exact similarity, and the parts for a caller
> who wants none; against, the rare matrix whose small entries are not
> noise, where balancing worsens a vector's residual against the matrix as
> given, has no flag in the driver. Recommended: as taken, with a
> 'nobalance' option in the language when a step asks. Taken.

**S295. Before balancing, the driver scales the matrix by a power of two to
a largest magnitude in [1, 2), exactly, and its eigenvalues back, so that
2^k A's eigenvalues are 2^k times A's and its eigenvectors A's, bit for bit,
barring overflow and underflow (S48's property, kept on the new path); a
NaN or an infinity gives every eigenvalue and every vector's entry a NaN
and returns false; the zero matrix has zero eigenvalues and the identity's
columns. `_max_iter` bounds the double-shift steps in all; if it is spent,
the eigenvalues found are given, the rest the diagonal's entries, every
vector's entry a NaN, and the driver returns false. `_tol` is the
deflation's tolerance, the family's epsilon where smaller.** `dgeev`
scales only a matrix whose norm lies outside its safe range; scaling every
matrix costs n^2 exact products and keeps the 2^k property for all.
- Alternative: `dgeev`'s scaling, only past the safe range: the property
  only in range.
- Alternative: LAPACK's budget, 30 steps per eigenvalue: no budget the
  caller sets, unlike `d_decomp_<S>_eigenvalues` of chunk 13.

> **Review.** Taken: for, S48's 2^k property on the new path, the zero
> matrix and a non-finite matrix answered, a budget the caller sets;
> against, n^2 exact products per call. Recommended: as taken. Taken.

**S296. `d_decomp_<S>_eigenvalues` of chunk 13 is the driver's path, its
eigenvalues the driver's real parts, bit for bit, false where one is
complex, as before; and the C++ face's `general_eigen` takes the driver's
eigenvalues and eigenvectors, its vectors now from the Schur form (S49)
rather than inverse iteration. `d_decomp_<S>_hessenberg` and
`_eigenvector`, the old parts, stay as they were for their callers.** One
path for every caller, and S45's and S49's flags closed where the face
reaches them. A face vector's sign can differ from inverse iteration's: the
largest entry is positive in both, but a vector whose two largest entries
are equal in magnitude may take the other.
- Alternative: the old path kept beside the new one: two answers for one
  matrix.

> **Review.** Taken: for, one path, the old function's bits the driver's
> real parts, the face's vectors independent for a repeated eigenvalue;
> against, a face vector whose two largest entries tie in magnitude can
> change sign from chunk 37's. Recommended: as taken. Taken.

**S297. A complex matrix's LU, determinant, inverse and solution are
`zgetrf`'s and `zgesv`'s: each column's pivot its entry of largest |re| +
|im| at or below the diagonal, the first among equals, as `izamax` chooses
it; every product and quotient `cplx.h`'s, correctly rounded or within a
step; `d_cmat_<S>_determinant` the parity times U's diagonal,
`_inverse` Gauss-Jordan as `mat.h`'s `d_mat_<S>_inverse`, and `_solve`
the elimination of [A | B] with back-substitution, each column the bits of
`_lu` and `_lu_solve`. Needing no root, each comes once, a constant
expression from C++14.** A singular matrix is one with a pivot exactly 0, as
`zgesv` reports it: its determinant 0, its inverse and solution the zero
matrix, false.
- Alternative: the pivot of largest modulus: a root or a comparison of
  squares per entry, and not LAPACK's choice.

> **Review.** Taken: for, LAPACK's pivot, no root, constant expressions, a
> solution of LU's bits; against, a nearly singular matrix's inverse of huge
> entries with no warning, as `zgesv`'s, until a condition estimate (S257's
> `cond`). Recommended: as taken. Taken.

**S298. A Hermitian matrix's eigenvalues and eigenvectors are found by the
cyclic Jacobi method with complex rotations, `d_cmat_<S>_hermitian_eigen`
and its `_c` form, mirroring `decomp.h`'s `d_decomp_<S>_sym_eigen`: each
off-diagonal pair's phase u = a_pq / |a_pq| taken out by the unitary
diag(1, conj(u)), then the real rotation of the pair; the values ascending,
the vectors a unitary matrix's columns. A real symmetric matrix gets
`sym_eigen`'s values bit for bit.** Jacobi's method is accurate to each
eigenvalue's own scale on graded matrices, where tridiagonal QR is accurate
to the norm's, and is the method the real path already takes.
- Alternative: Householder tridiagonalization and implicit QL (`zheev`):
  faster for large orders, LAPACK's own, accurate to the norm.

> **Review.** Taken: for, each eigenvalue accurate to its own scale, the
> real path's values for a real symmetric matrix, constant expressions;
> against, Jacobi's sweeps take several times tridiagonal QR's operations at
> large orders. Recommended: as taken, with a tridiagonal path when a step
> asks for large Hermitian matrices. Taken.

**S299. A general complex matrix's eigenvalues and eigenvectors are
`zgeev`'s path, as S292's are `dgeev`'s: `d_cmat_<S>_balance` and
`_balance_back` (`zgebal`, `zgebak`), `_hessenberg_q` (`zgehd2`, each
reflector's beta real), `_schur`, single-shift complex QR (`zlahqr`: the
subdiagonal made real first and kept real, Wilkinson's shift, the root
nearer the corner, by `cplx.h`'s square root, exceptional shifts),
`_schur_vectors`, back-substitution with each divisor no smaller than ulp
times |t_kk| and x scaled down where the next step could overflow, and the
driver `d_cmat_<S>_eigen`, with S293's order, S294's balancing and S295's
scaling and budget.** A real matrix keeps the real path, whose pairs are
exactly conjugate; the complex path is for complex matrices.
- Alternative: `ztrevc`'s triangular solves by `zlatrs`, scaled column by
  column: more robust to growth than the per-entry guard, at more code.
- Alternative: every real matrix through the complex path: one path, its
  pairs conjugate only to rounding.

> **Review.** Taken: for, `zgeev`'s path, a real matrix's pairs exactly
> conjugate on its own path; against, the back-substitution's per-entry
> guard rather than `zlatrs`'s scaling, and a complex eigenproblem of order
> 3 as a constant expression takes clang past its default million steps of
> constant evaluation (`-fconstexpr-steps` raises it; the harness's is of
> order 2). Recommended: as taken. Taken.

**S300. The eigenvalue paths' kernels are compiled once in a translation
unit and never forced inline: `D_MATH_FN_KERNEL` where a kernel is a
constant expression from C++14, and a new qualifier,
`D_MATH_FN_KERNEL_RT`, for a kernel that takes the C library's root and is
not (`inline` in C++, `static` with the compiler's inline hint in C). In
`cmat.h`, `cplx.h`'s product and quotient are called through two such
kernels, `d_internal_cmat_<S>_mul` and `_div`, where the matrix kernels
would otherwise each take a copy of them at every call.** Forced inline,
every call would compile its own copy of each kernel it reaches, and the
complex kernels each a copy of `cplx.h`'s exact products at every use.
- Alternative: `D_MATH_FN`, forced inline, as the scalar functions are:
  the call's cost saved, the compile's multiplied.

> **Review.** Taken: for, each kernel compiled once in a unit, the complex
> products once; against, a call where a forced inline would have saved it,
> and a fifth qualifier. Recommended: as taken. Taken.

**S301. The C core's norms are scaled, as S256 recommended: `d_vec_<S>_norm`
and its `_c` form, `_distance`, `_normalize` and `_cos_angle` take the root
of the sum of squares where it lies in [sqrt(MIN / EPS), MAX], where no
square overflowed or lost its precision to underflow, and elsewhere scale
the vector by a power of two to a largest magnitude in [1, 2), take the
norm, and scale it back. In range every bit is as it was; at the ends the
norm is the one the type holds, exactly where it is exact. The language's
wrapper is gone: `linear_algebra`'s norms, unit vectors and angles, and the
language's, are the C core's.** A power of two commutes with rounding, so
the scaled path's sum is the unscaled one's, bit for bit, wherever both are
finite.
- Alternative: always scaled, as `dnrm2`'s running sum or Blue's
  algorithm: slower in range, its bits other than the plain sum's.
- Alternative: hypot chains: a root per entry.

> **Review.** Taken: for, S256's recommendation where it serves every face,
> every bit in range unchanged, the norms at the ends exact where they can
> be; against, `project` and `reject` still divide by an unscaled squared
> norm, an infinity or 0 at the range's ends. Recommended: as taken, with
> `project` and `reject` scaled the same way at the next step that touches
> `vec.h`. Ruled 2026.11.04: as recommended, at the next step that touches
> `vec.h`. Done in chunk 50 (S409): `project` and `reject` scaled at the
> range's ends, every bit in range kept.

**S302. `mat.h`'s `d_mat_<S>_solve(x, scratch, a, b, n, m)` is Gaussian
elimination of [A | B] with partial pivoting and back-substitution, as
`dgesv`: each column of B the bits of `decomp.h`'s `d_decomp_<S>_lu` and
`_lu_solve`, false and the zero matrix for a singular A. The generic
kernels' `solve` is the same elimination in the number type's own
arithmetic, and `linear_algebra`'s `solve(A, b)` and `solve(A, B)` take the
kernels' for every number type but an integer type - the C families'
LU's bits, an exact type's exact solution - where they took LU and required
a floating-point type. A singular A gives the zero solution, as `inverse`
gives the zero matrix.** S257's recommendation: an exact solve no longer
inv(A) B, at about a third of the products, and the C++ face's `solve` open
to values, rationals and quaternions. For the C families, a singular matrix's
solution was LU's quotients by 0, infinities and NaNs; it is now zero, as
the inverse's was.
- Alternative: Gauss-Jordan of [A | B], the inverse's elimination: the
  same exact answer, half again the products, and other bits than LU's.
- Alternative: Bareiss's fraction-free elimination for exact types: no
  quotient until the end, larger intermediate numbers.

> **Review.** Taken: for, S257's elimination in every family, an exact solve
> at about a third of the inverse's products, the face's `solve` open to
> every number type, a dense one LU's bits; against, a C family's singular
> solve now zeros, not LU's infinities and NaNs, a change for a caller who
> tested for those. Recommended: as taken; `cond` when a step asks (S257).
> Taken.

**S303. `number_traits` is written again (S157, S158): the field's traits,
`is_exact`, `is_integer`, `is_field`, `is_commutative`, `zero()` and
`one()`; the order's, `is_ordered` and `abs`; `magnitude(x)` of
`magnitude_type`, what generic code pivots by and scales a tolerance by,
|x| for an ordered type and, for one without an order, a nonnegative number
of an ordered type; `epsilon()`, the type's precision, and `epsilon(x)`,
the precision x carries, 0 for an exact value; `tolerance()`, four
epsilons, the relative tolerance of a comparison given none, 0 for an exact
type; and `tolerance(a, b)`, that times the larger magnitude with the larger
epsilon of the two, 0 where both are exact. An integer type's root is
exact, rounded down. The value classes' epsilon is their kind's, 0 for an
exact value, their tolerance for a pair 0 where both are exact.** The
tolerance relative and of the type, as S46 chose for the eigenvalues; the
traits ready for complex numbers and quaternions, whose magnitude is a
modulus and whose products do not commute.
- Alternative: Eigen's `NumTraits`, `dummy_precision()` 1e-12 for double
  and `isApprox` relative to the smaller norm: a looser default, one per
  type.
- Alternative: Julia's `isapprox`, rtol = sqrt(eps): eight digits of a
  double's sixteen.

> **Review.** Taken: for, S157's and S158's recommendations, the traits
> ready for complex numbers and quaternions, a tolerance of the type's own
> precision relative to scale; against, four epsilons is strict for the
> results of long computations compared with no tolerance, where a caller
> must give one. Recommended: as taken. Taken.

**S304. The C++ face's comparisons given no tolerance - `equals`,
`approx_equal`, `is_symmetric`, `is_identity`, `is_unit`,
`is_orthogonal` - take `internal::linalg_tolerance`: the type's
`tolerance(x, y)` of x, the entry of largest magnitude among both operands'
(1 among them where the comparison is with the identity or a unit's norm),
and y, the entry that carries the largest epsilon; a comparison given a
tolerance takes it as an absolute one, as before.** Relative to the
operands' scale by default, four epsilons of a floating-point type, 0 where
every entry is exact, and a type's author's choice where its traits make
one; an explicit tolerance keeps its meaning for every caller who passed
one.
- Alternative: an explicit tolerance relative too: one meaning, every
  caller's explicit tolerance changed.

> **Review.** Taken: for, a default relative to the operands, exact where
> they are, a type's own choice honored, every explicit tolerance unchanged;
> against, two meanings, relative by default and absolute when given.
> Recommended: as taken, a relative form by name when a caller asks. Taken.

**S305. The generic kernels keep each product in its order - a row
operation a product on the left, a quotient on the left as 1 / d times x
and on the right as x times 1 / d where products do not commute - and
pivot by `magnitude`, so that a quaternion matrix's inverse and solution are
right, and an inexact type's elimination pivots by size.** The face's tests
plant a quaternion type of exact rationals: A A^-1 and A^-1 A the identity,
A X = B, exactly; with either quotient's side wrong, each misses.
- Alternative: x / d everywhere, as before: right only where products
  commute.

> **Review.** Taken: for, quaternion matrices solved and inverted exactly,
> an inexact type pivoted by size; against, a quotient where products do not
> commute is two operations, 1 / d and a product. Recommended: as taken.
> Taken.

**S306. The Jacobi rotations' tangent - `d_decomp_<S>_sym_eigen`'s, the
one-sided Jacobi SVD's and `d_cmat_<S>_hermitian_eigen`'s - is 1 / (2
theta), the off-diagonal entry over the diagonal's difference, where theta
passes 2^(EMAX / 2) and theta^2 would overflow; below, as before, bit for
bit.** This chunk's constant expressions found it: at a threshold of 0 a
pair all but annihilated gives theta past the range's half, theta^2
overflows, at run time to t = 0, no rotation, and in a constant expression
to an error; the root equals 1 / (2 theta) within a part in 2^EMAX there.
- Alternative: the tangent by hypot of the difference and twice the entry:
  no overflow at all, other bits everywhere.
- Alternative: the matrix scaled by a power of two first, as the driver
  is: no entry past MAX / 2, its difference and twice its entries finite,
  but theta's ratio unchanged.

> **Review.** Taken: for, constant expressions at a threshold of 0, the
> rotation made where it was skipped, every other bit as before; against,
> the Jacobi functions take the matrix unscaled, so that an entry past MAX /
> 2 overflows twice itself, or two of opposite signs their difference, an
> error in a constant expression. Recommended: as taken, with the matrix
> scaled by a power of two first, as the driver is, at the next step that
> touches the Jacobi functions, the bits in range unchanged. Ruled
> 2026.11.04: as recommended, at the next step that touches the Jacobi
> functions. **Flag:** minor: no scaling before the Jacobi sweeps.

**S307. `t_decomp.c` and the new `t_cmat.c` check the paths against
references made by mpmath at 80 digits, `math_tests_eig_refs.h` and
`math_tests_cmat_refs.h` (`tools/refs_c38.py`): every matrix exact in
float, so that each family takes it exactly; each eigenvalue within 32
epsilons of the family times |A|_F times its condition number; each
eigenvector's residual within 32 epsilons of the balanced matrix's norm;
the Hessenberg and Schur forms' similarities and their Q and Z's
orthogonality within 8 n epsilons; a system's solution, inverse and
determinant within 2 n epsilons times its condition number of the exact
ones. `t_vec.c` checks the norms at the range's ends, `t_mat.c` the
solution written out bit for bit, and `t_number_traits_face.cpp` the
traits, the quaternions and the generic pivot.** The bounds are a few
times the largest measured in every build (section 4).
- Alternative: residuals alone, with no references: a backward-stable
  method's property, not its answer's accuracy.

> **Review.** Taken: for, every eigenvalue against an independent reference
> to its condition, every family on the same exact matrix, the parts checked
> alone and together, bounds a few times the measured; against, references
> for orders up to 12, and two headers of references. Recommended: as taken.
> Taken.

**S308. The C++ face's complex type, `math::complex<T>`, and with it the
language's complex eigenvalues (S258) and the C++ face's general eigen of a
matrix with a complex pair, come in chunk 39 as S279 planned; this chunk's
paths give them every value they need.** The face's general eigen gives a
real matrix's real eigenvalues as before, a pair still flagged there until
the face has a complex type to give it in.
- Alternative: the face's pairs as two rows of reals now: a shape withdrawn
  by S258's review.

> **Review.** Taken: as S279 planned; against, until chunk 39 the face flags
> a pair as before. Recommended: as taken. Taken; chunk 39 gives every pair
> as complex numbers (S317).

**S309. In `decomp.h`'s section of the general eigenproblem and `cmat.h`'s
sections of Hermitian and general matrices, gcc's `-Warray-bounds` is set
aside, `#pragma GCC diagnostic push` and `ignored`, and restored after each
section by `pop`; clang and other compilers are untouched.** gcc 13 at -O2,
compiling a caller whose matrix is a constant it can fold - a 2-by-2
rotation in `main`, the record's example - takes the kernels' loops for
n of 3 and more for reachable and warns of indices past the array: eight
false warnings, each an error under `-Werror`. Every index in these
sections lies within the caller's n by n, as the harnesses and the
sanitizers check.
- Alternative: the warnings left, a caller adding `-Wno-array-bounds`:
  every caller with `-Werror` stopped by the library's headers.
- Alternative: the kernels written so that gcc's value ranges can bound
  each index: a rewrite of LAPACK's loops for one compiler's analysis.

> **Review.** Taken: for, a caller's `-Werror` build of gcc at -O2 not
> stopped by false warnings from the library's headers; against, a caller
> who passes an array too small for its n loses gcc's warning inside these
> sections, where the sanitizers would still find it. Recommended: as taken.
> Taken.

**S310. The checks, as S289's: the generator's `--check` and the
references' `--check`; the full tier of every math test, as the change
reaches every harness through `decomp.h` and the face's traits; the smoke
tier against chunk 37's lines; the fault trial; the sanitizers, the lines
run, the ladders, the lints and the levels, as chunk 37's record ran them.**
- Alternative: the full tier of the four harnesses and the faces the
  change names: fewer builds, a header every test includes left unchecked
  elsewhere.

> **Review.** Taken: as S289, with the full tier of every math test, as the
> change reaches every one, and the faults of the new paths. Recommended: as
> taken. Taken.

**S311. The balancing scales the matrix in place, as `dgebal` and `zgebal`
do: each step divides a row and multiplies a column by a power of two, so
that the balanced matrix is D^-1 P^T A P D exactly unless an entry passes
below the least normal number on the way and loses bits, as LAPACK lets it;
`decomp.h`'s comment on the balancing says so, and the harnesses check it
exactly where no entry does and, across the range, to each entry's own
magnitude.** The guards that keep the factors within [MIN / EPS, EPS / MIN]
keep each shrunk row's or column's largest entry above 2 MIN / EPS, so that
an entry passing below the least normal is then below half an epsilon of
that largest; later steps can raise it, its row's largest at the end, with
few of its bits. The harnesses' guards found it, balancing matrices of
entries from 2^-(EMAX-4) to 2^(EMAX-4), in float and long double; LAPACK's
balancing, and MATLAB's and NumPy's through it, have the same property.
- Alternative: the factors found as now, each norm taken of the entries
  times the factors' ratios rather than of the entries scaled, and applied
  once at the end, each entry a_ij d_j / d_i rounded once: D^-1 P^T A P D
  exactly wherever it is representable, and at every matrix where no entry
  passes below the least normal the same factors and bits as now; a norm
  of products, and an exact scaling by 2^k, which the C core lacks, in
  both headers.
- Alternative: the factors bounded so that no entry can leave the normal
  range: matrices of a wide range balanced less, their eigenvalues less
  accurate.

> **Review.** Taken: for, LAPACK's algorithm line by line, as S292 chose,
> and the eigenvalues' accuracy against |A| unchanged; against, at matrices
> whose entries span most of the range, an entry of the balanced matrix can
> keep few of its bits and be its row's largest, so that the balanced matrix
> is not the exact similarity the earlier text claimed. Recommended: the
> first alternative, the factors applied once at the end, at the next step
> that touches the balancing: exact wherever representable, the same bits
> wherever no entry underflows. Ruled 2026.11.04: the first alternative, the
> factors applied once at the end, as recommended. **Flag:** minor: the
> balancing exact only where no entry passes below the least normal.

### S312 to S323: chunk 39, 2026.10.31

The record: `_handoffs/math98_chunk39_2026.10.31/README.md`.

S312 to S323, chunk 39's, as `decisions.md` has them; each is reviewed
there, S322 flagged, minor; S308's review notes its close; and the 19
outstanding decisions of S1 to S323 come first, by group and urgency, as
chunk 38's made them (section 2.6). The text below is `decisions_c39.md`,
which `decisions_c39.py` reads.

**S312. Step 22's third part is the C++ face of the C core's complex
numbers and what it completes, as S279 planned: `math::complex<T>` in a
header of its own, `math/complex.hpp`, with its `number_traits`; vectors
and matrices of it; `general_eigen`'s eigenvalues and eigenvectors as
complex numbers (S308); and in the language `im`, `jm` and `km` answering
`not_yet` (S271, S275), the stubs' new step numbers (S265) and the empty
matrix's guard (chunk 36).** `complex.hpp` stands beside `value.hpp` and
`quaternion.hpp` and includes `cplx.h` alone of the C core; linear algebra
names the type by a declaration, so that a program of real vectors
compiles no complex code.
- Alternative: the type in `value.hpp` or `number_traits.hpp`: every
  includer of the value or the traits compiling `cplx.h`'s 11,042 lines.

> **Review.** Taken: for, step 22's last part as S279 planned, and a header
> of its own, so that real code compiles no complex code; against, a third
> delivery for one step. Recommended: as taken. Taken.

**S313. Every operation of `math::complex<T>` is `cplx.h`'s: the
arithmetic, which comes once; `real`, `imag`, `conj` and `proj`, exact;
and `abs`, `arg` and C99's 16 other functions in their `_c` forms, at
every level and in every build, never the plain forms, so that a result
has the same bits on every compiler and target, in a constant expression
or not (G6). Each operation is a friend defined in the class, found by
argument-dependent lookup alone, as `math::value`'s operators are; `pow`
with a real operand is `pow` of it plus 0i, as C's `<tgmath.h>` takes it;
and there is no `_fast` form until a use asks for one (S266).** The real
`number_root` takes the C library's root below C++14 because the two are
the same bits, both correctly rounded; the complex plain forms are not:
their bounds in `cplx.h`'s banner run to 6 steps where the `_c` forms' run
from 1 to 4, and the C library's real functions beneath them are each
library's. The test finds, at its 2,000 points of each family, points
where the plain forms give other bits, and checks the face against the
`_c` form at every one.
- Alternative: the plain forms at run time and the `_c` forms in constant
  evaluation, as `std::is_constant_evaluated` allows from C++20: faster at
  run time, and two answers for one call.
- Alternative: function templates at namespace scope, found by
  `math::exp(z)` too: a qualified call as `std::exp(z)` is written, and
  every unqualified call in `djinterp::math` meeting templates that match
  any type.

> **Review.** Taken: for, one answer (G6) on every compiler, target and
> level, in a constant expression or not, which `std::complex` cannot give;
> the operations found as `math::value`'s are; against, the `_c` forms' long
> double kernels are slower than the plain forms on a fast C library, and a
> qualified `math::exp(z)` does not compile, a caller writing `exp(z)`. The
> alternatives give two answers, or templates that every unqualified call in
> the namespace meets. Recommended: as taken, a `_fast` form where a use
> measures the need (S266). Taken.

**S314. To `number_traits`, `math::complex<T>` is a commutative field,
inexact and unordered: its magnitude, a T, is |re| + |im|, LAPACK's
`cabs1`, by which `izamax` and `cmat.h` pivot, needing no root; its
epsilon T's, its tolerance four of them, and a pair's that times the
larger magnitude. It has no `abs` of its own type, no root and no
conversion through `double`, so that generic code that would order its
numbers, take their root or pass them through `double` - a vector's norm,
its length, an angle - is refused at compile time rather than computing a
sum of squares that is no norm.** It is no `number`, the C++20 concept,
which asks for `<`. `common_number` of it and T, either way round, or of
two, is the complex type at every level.
- Alternative: the modulus as the magnitude: a root for every pivot, and
  pivots other than `cmat.h`'s where the two order two entries differently,
  so other bits.
- Alternative: `abs` the modulus and `sqrt` the principal root in the
  traits: the generic norm compiling as the root of the sum of z_i^2, a
  complex number, silently no norm.

> **Review.** Taken: for, LAPACK's magnitude, so that the pivots and the
> bits are `cmat.h`'s, and no root per pivot; generic code that would
> compute a false norm refused at compile time; against, |re| + |im| is up
> to sqrt 2 times the modulus, so that a tolerance it scales is a little
> looser than the modulus would make it. Recommended: as taken. Taken.

**S315. Vectors and matrices of `math::complex<T>` take the generic
kernels, whose every operation is the type's own and so `cplx.h`'s. Their
determinant, for this type alone (`linalg_by_lu`), is LU with partial
pivoting by magnitude, `cmat.h`'s `d_cmat_lu` and `d_cmat_lu_det` step for
step, the sign of the row swaps 1 or 0 - 1, -1 + 0i, as the product's first
factor; their inverse and solution, Gauss-Jordan and the elimination of
[A | B] (S302, S305), are `cmat.h`'s step for step already. So the face's
determinant, inverse and solution of a complex matrix are `cmat.h`'s bit
for bit, and constant expressions from C++14.** The test checks the three
against `cmat.h`'s on random matrices of 1 to 5 rows, singular and tied
ones among them, in every build. Every other type the generic path takes
keeps Bareiss's determinant, exact where the type is.
- Alternative: a kernel family of its own, forwarding to `cmat.h` through
  the arrays' pointers reinterpreted: one source of the algorithm, but
  pointer arithmetic over the members of `complex<T>`, which the language
  does not define, no constant expression before C++20's transient
  allocations, and `cmat.h`'s 14,117 lines compiled by every includer of
  `linalg_common.hpp`.
- Alternative: Bareiss's determinant for complex matrices too: a row
  swapped in only at an exact zero and each step divided by the previous
  pivot, unstable in floating point, and other bits than the C core's.

> **Review.** Taken: for, `cmat.h`'s bits through the face, constant
> expressions from C++14, and no pointer arithmetic the language leaves
> undefined; against, the LU's steps written twice, in the C core and in the
> generic kernels, kept equal by the test's checks of every build rather
> than by one source. The alternatives cost a constant expression and
> defined behaviour, or the C core's bits and stability. Recommended: as
> taken; the parity checks are the guard, and the fault trial plants what
> would break them. Taken.

**S316. A tolerance given to the faces' comparisons - `matrix::equals`,
`is_identity` and `is_symmetric`, `vector::equals` and `is_unit`,
`approx_equal`, `is_invertible`, `is_singular` and `is_orthogonal` - is
of the traits' `magnitude_type`, as the kernels already take it, which for
every type that had one is the element type itself; `is_invertible` asks
that the determinant's magnitude pass it, |det| for a real type.** The
free `approx_equal`'s tolerance is no longer deduced, so that
`approx_equal(a, b, 1e-6)` of float vectors now compiles.
- Alternative: the element type, as before: a complex tolerance, whose
  magnitude the kernels would have to take, and no build for complex
  elements.

> **Review.** Taken: for, a tolerance of the type a magnitude is, so that
> complex and quaternion matrices compare, and `approx_equal` of float
> vectors with a double tolerance compiles; against, none for the types that
> had a tolerance, whose magnitude type is their own. Recommended: as taken.
> Taken.

**S317. `general_eigen<T, N>` gives every eigenvalue and eigenvector of a
real matrix as `math::complex<T>` (S308): `eigenvalues()` a
`vector<complex<T>, N>`, `eigenvectors()` a `matrix<complex<T>, N, N>`,
and `eigenvalue(i)` and `eigenvector(i)` of them, the driver's values and
vectors as `decomp.h` gives them, a pair's exactly conjugate, the real
parts `eigenvalues_qr`'s bit for bit. `converged()` is false only where
not every eigenvalue was found, every vector then NaNs, as the driver
gives them; `is_real()` says whether every eigenvalue is real.**
`eigenvalues_qr` keeps its real eigenvalues, a pair's real part twice and
flagged. Where the face gave the identity's columns for a matrix whose
vectors were not found, it gives the driver's NaNs, which no caller can
take for eigenvectors.
- Alternative: a second class, `complex_eigen`, beside the real one: two
  answers for one matrix, the real one's pair still flagged.
- Alternative: real eigenvalues and a vector of their imaginary parts, as
  LAPACK's `wr` and `wi`: the shape S258's review withdrew.
- Alternative: the identity's columns where no vector was found, as
  before: a matrix a caller who does not ask `converged()` takes for
  eigenvectors.

> **Review.** Taken: for, every eigenvalue and vector of a real matrix in
> one call, as S308 planned, the driver's answer whole, NaNs that no caller
> can take for eigenvectors; against, a caller of the real face now takes
> complex values, writing `.real()` where it wanted a T, and a caller who
> tested the identity's columns of a failed iteration finds NaNs.
> Recommended: as taken. Taken.

**S318. `im`, `jm` and `km` are entries of the language's constant table
from step 22, as S271 and S275 ask: the parser reads each as a constant,
never a variable, and every fold answers `not_yet` for each, its message
naming the step that gives it a value, "im: not yet (step 23, 3e)" and
"jm: not yet (step 24, 3f)", "km" as "jm".** An entry carries its message
in `not_yet`, empty for every constant with a value, and
`expr_constant_of` asks it first. No text binds `im` now and reads
otherwise when step 23 gives it a value.
- Alternative: the names unknown until their steps: `im` a variable now
  and a constant later, one text read two ways.
- Alternative: the names as functions of the table answering `not_yet`:
  `im` a call, not a constant, and `3 + 4im` no product.

> **Review.** Taken: for, the names held for their kinds from now, so that
> no text changes its reading at steps 23 and 24, S197's rule for a name the
> language reserves; against, a variable named `im`, `jm` or `km` is lost
> now rather than at its step. Recommended: as taken. Taken.

**S319. The stubs of Phase 4 name their steps as S265 numbered them:
`expr_derivative` and `expr_gradient` step 25 (4a), `expr_integrate` and
`expr_derivative_at` step 26 (4b), `expr_sum`, `expr_product`,
`expr_limit` and `expr_taylor` step 27 (4c), and `expr_antiderivative`
step 28 (4d), in their messages, their comments and the banners;
`t_expr_face.cpp` checks the new messages.**
- Alternative: the old numbers until Phase 4: messages naming steps the
  roadmap gives to other items.

> **Review.** Taken: for, messages that name the roadmap's steps; against,
> none. Recommended: as taken. Taken.

**S320. `t_expr_face.cpp`'s empty matrix asks first that `[]` parses,
before it reads the term the parse gave, as chunk 36's ten faults that
stopped the test by a signal asked (its section 5).** Chunk 36's F743,
`[]` of one empty element, planted again, fails a check (section 4).
- Alternative: as it was: each such fault caught by a signal, with no
  check saying where.

> **Review.** Taken: for, a fault that breaks `[]` fails a check that says
> where, rather than stopping the test; against, none. Recommended: as
> taken. Taken.

**S321. The type is `math::complex<T>`, as S270 names it, beside the
lane's `math::value`, `math::quaternion` and `linalg::vector` and `matrix`,
and the standard's `std::complex`, which it stands beside. Its parts are
read as `std::complex`'s are, `real()` and `imag()`, and set by `real(x)`
and `imag(y)`; `c_value()` lends the C core's struct, as `math::value`'s
lends the value; `to_std()` gives a `std::complex<T>`, by name at every
level, as C++98 has no explicit conversion operator; and a `complex<U>` of
another part type converts explicitly, part by part.** The C++ guide names
a project type `D` and PascalCase, `DComplex`, which no type of the lane's
C++ faces is.
- Alternative: `DComplex<T>`, as the guide asks: the guide's name, and one
  type of the lane's named apart from the others and from the standard's.
- Alternative: an explicit conversion operator to `std::complex<T>` from
  C++11 and the member below it: two spellings by level.

> **Review.** Ruled (S270): for, the name the owner gave, the lane's types'
> spelling and the standard's; against, the C++ guide's `D` and PascalCase,
> which no type of the lane's faces has, so that the lane's C++ types are
> named apart from the guide's rule. Renaming them all, `DValue`,
> `DQuaternion`, `DMatrix` and `DComplex`, is the owner's to ask for.
> Recommended: as ruled. Ruled.

**S322. The face's `dot` of complex vectors stays the bilinear sum of
a_i b_i, the matrix product's row by column and NumPy's `dot`; norms,
lengths, angles and unit vectors of complex vectors are refused at compile
time (S314); and `is_orthogonal` asks A^T A = I, an orthogonal complex
matrix, not a unitary one. A conjugating dot, the 2-norm of a complex
vector and the Frobenius norm of a complex matrix, and `is_unitary`, come
with step 23, as the language's `dot` conjugates and its `norm` takes a
complex matrix (S273).**
- Alternative: `dot` conjugating its first operand now, as Eigen's does
  and the inner product is written: the face's `dot` two different sums by
  type, and the product's row by column no longer `dot`.

> **Review.** Taken: for, one `dot` for every element type, the product's
> row by column; what would be wrong refused at compile time; against, no
> inner product and no 2-norm of complex vectors in the C++ face until step
> 23, a gap a user of Eigen would notice at once. Recommended: as taken now,
> and at step 23 the conjugating `vdot` (Eigen's `dot`, NumPy's `vdot`), the
> 2-norm and the Frobenius norm of complex vectors and matrices, and
> `is_unitary`, with the language's (S273), in step 23's third part, chunk
> 43 (S324). Taken in chunk 43: `vdot`, the norms, `frobenius_norm` and
> `is_unitary` (S354).

**S323. The checks: `t_complex_face.cpp`, new, at every level from C++98;
the full tier of every face test, as the traits, the kernels and the
faces' tolerances reach every one; the smoke tier against chunk 38's
lines; the fault trial; the sanitizers on the four tests the change names;
the lines run of the new code; the C++ ladder over the math lane; the
lints; and the levels, as chunk 38's record ran them. The C core is
unchanged, and its harnesses' full tier and the C ladder are not run
again.**
- Alternative: the full tier of the four tests the change names: fewer
  builds, and a header every face includes unchecked elsewhere.

> **Review.** Taken: for, every face test in every build, as the traits and
> the kernels reach each; the fault trial over the new code; against, the C
> core's harnesses not run again, which nothing in the chunk touches.
> Recommended: as taken. Taken.

### S324 to S334: chunk 40, 2026.10.31

The record: `_handoffs/math98_chunk40_2026.10.31/README.md`.

S324 to S334, chunk 40's, as `decisions.md` has them; each is reviewed
there, none flagged; S134's review notes its close (S326), and S203's,
S258's, S272's and S322's name the part of step 23 that brings them; and
the 18 outstanding decisions of S1 to S334 come first, by group and
urgency, as chunk 39's made them (section 2.6). The text below is
`decisions_c40.md`, which `decisions_c40.py` reads.

**S324. Step 23 is made in three chunks, each delivered and checked whole,
as S279 made step 22: chunk 40 the complex kind of the value and what a
number of it is in the language, its parts, arithmetic, modulus,
comparison, conversion and text, `im`'s value, `complex`, `real`, `imag`,
`conj` and `arg`, `z'`, the relations, the exact powers to a whole
exponent, and the holders every kind counts (S134); chunk 41 each
function's principal complex value (S271), the five functions chunk 37
gave the C core and `min`, `max` and `sign` joining the table (S203), a
real function's complex value outside its real domain with
`expr_context`'s option (S272), and the folds in `math::complex<T>`; chunk
42 matrices of complex numbers (S273), their storage, `A'`, `transpose`,
`dot`, the norms and `eig` complex and ordered (S258), and the C++ face's
conjugating dot, complex norms and `is_unitary` (S322).** Each part is a
chunk the record can check whole, its references and faults with it; what
a later part computes answers `not_yet` meanwhile, by the step's message
(S331).
- Alternative: step 23 in one chunk: the kind, thirty functions' complex
  values and matrices of complex numbers checked together, the largest
  change of the language yet in one delivery.

> **Review.** Taken: for, each part checked whole, its references and faults
> with it, and what a later part computes answering not_yet, never a value
> it would change; the holders atomic before the first kind that shares a
> box between folds; against, three deliveries for one step, and two chunks
> in which a complex number has no square root. Recommended: as taken.
> Taken. Chunk 41 made the second part in two chunks, the C core's five
> complex functions it lacks the second's (S335), so that the third part is
> chunk 43, which made it, so that step 23 is done (S353).

**S325. Whether a boxed value is exact is asked of each value: `value.h`'s
`struct d_math_boxed_kind` gains a last entry, `exact`, which a kind that
is a number, but not exact in all its values, sets, and which is asked
only of a number of a kind whose traits do not say `D_MATH_BOXED_EXACT`;
`NULL` means none of its values is exact. A table written before it, with
one initializer fewer, holds `NULL` there, as C and C++ initialize a member
no initializer names; the wide integer's, the matrix's and the harnesses'
tables name it.** A complex number of exact parts is exact and one of
doubles is not, by S266's one set of rules, and so will a quaternion and an
interval be: exactness is the value's, as its parts are, while one kind
keeps one table, as the matrix keeps one for every storage (S238).
- Alternative: two kinds, an exact complex number of rank 900 and an
  inexact one of 910, as the tower's inline kinds are exact or not by
  kind: no change to `value.h`, but two tables for each kind to come, and
  cross-kind cases within one kind of number.
- Alternative: `D_MATH_VALUE_FLAG_INEXACT` on every inexact complex number:
  a flag every result inherits, so that an exact result computed from one
  would be marked inexact.

> **Review.** Taken: for, exactness asked where it lives, of the value, so
> that one kind of complex numbers, quaternions or intervals keeps one
> table, as the matrix does for every storage (S238); a table written before
> the entry unchanged in meaning; against, a change to the C core's table,
> and an indirect call for each exactness asked of such a kind. The
> alternatives: two kinds for each number of S266's rules, and cases between
> them; or a flag that every result inherits. Recommended: as taken. Taken.

**S326. A box's holders are counted atomically by default, closing S134's
flag: `value.h`'s `d_math_boxed_hold` and `d_math_boxed_unhold` change the
`size_t` every box already has by GCC's `__atomic` builtins, which Clang
and the GCC-compatible compilers give at every language level, C89 and
C++98 among them, or by MSVC's interlocked functions; the increment
relaxed, the decrement with acquire and release, so that the last holder
sees every other's reads before it gives the box back, as `shared_ptr`'s
count is kept. Where a compiler has neither, the count is plain and
`D_MATH_BOXED_HOLDERS_ATOMIC` is 0. The knob `D_CFG_MATH_VALUE_ATOMIC_HOLDERS`,
in a new `config/c/math/cfg_value.h`, on by default, turns it off for a
program that never shares a value between threads; `cfg_value.h` resolves
which way the count is kept, and `value.h` reads the answer, as `dconfig`'s
localization rule asks. The wide integer, the matrix and the complex number
count through them; so will every kind to come.** The language's constant
`im` is one box, held by every fold that reads it: two threads evaluating
`3 + 4im` at once change its count together.
- Alternative: C11's `_Atomic size_t` and C++11's `std::atomic`: types of
  their own, which no struct C and C++ both read can hold, and none below
  C11 or C++11.
- Alternative: plain by default, atomic behind the knob: a data race,
  undefined behaviour, for every program that shares a value and does not
  know to set it.
- Alternative: a lock around each count: a lock for every copy of a value.

> **Review.** Taken: for, values shared between threads safe by default, at
> every language level the lane serves, with no type the C and C++ structs
> cannot both hold; the count's orders shared_ptr's; a knob for a
> single-threaded program; against, an atomic step on every copy, a few
> cycles uncontended, and a compiler with neither builtins nor MSVC's
> functions plain, which D_MATH_BOXED_HOLDERS_ATOMIC says. The alternatives:
> C11's and C++11's atomic types, which no struct both languages read can
> hold; plain by default, a data race for every program that shares a value
> unawares; a lock for every copy. Recommended: as taken. Taken.

**S327. The language's complex numbers are one boxed kind, "complex", of
rank 900, written in C++ from C++11 in `math/expr/complex.hpp`, beside the
matrix kind, as the language that makes them is (S237). Its parts are
exact, integers, rationals or wide integers, kept as two `math::value`s,
where both are given exact; and otherwise dense, a `math::complex<float>`,
`<double>` or `<long double>`, the C core's struct, in the wider of the
parts' floating-point types, each part its nearest there (S266); a part of
a boxed kind that is not exact, which no kind is yet, keeps both as values.
An exact complex number whose imaginary part comes to 0 is its real part,
of a real kind, as a rational of denominator 1 is an integer; an inexact
one keeps its parts, its zeros with their signs, never made real. The kind
is a number, its exactness its parts' (S325). In C++, `expr_complex(re,
im)` makes one as the language's `complex(x, y)` does, `expr_is_complex`
asks for one, `expr_real`, `expr_imag`, `expr_conj` and `expr_arg` are the
language's functions of a number, and `expr_complex_of` and
`expr_complex_to` go to and from a `math::complex<T>`.** A part of
`complex(1/3, 0.5)` is the double nearest 1/3, as the tower takes 1/3 +
0.5; `complex(3, 0)` is 3, and `complex(3.0, 0.0)` stays 3 + 0im.
- Alternative: the kind in C, `c/math/value_cplx.h`, beside the wide
  integer: a complex value for C programs, which have no language to make
  one with, and its exact parts' boxes held by hand.
- Alternative: dense parts always, an exact part its nearest long double:
  (1 + 2im)(3 - im) no longer exactly 5 + 5im, against S271.

> **Review.** Taken: for, the kind beside the matrix, in the language that
> makes it, its exact parts held as C++ objects hold their boxes (S237);
> exact numbers exact and floating-point ones the C core's struct, as S266
> rules; an exact number with no imaginary part a real one, the tower's
> simplest kind; against, an exact complex number allocates a box with two
> values, and C programs have no complex value. The alternatives: the kind
> in C, for callers who have no language to make one with, each part's box
> held by hand; dense parts always, exactness lost. Recommended: as taken.
> Taken.

**S328. Its arithmetic is the value's, through its table, with a complex
operand or a real one of any lower rank. Exact operands compute on the
parts by `value.h`'s exact operations, (a + bi)(c + di) as (ac - bd) +
(ad + bc)i and a quotient over c^2 + d^2, a part past the family G2's long
double, flagged, and the number then of long double parts; any other
operation computes in the widest floating-point type of its operands, each
exact part its nearest there, by `cplx.h`'s arithmetic through
`math::complex<T>`, a real operand taken as itself by the `_real` forms
(C99's G.5), so that (1 + 0im)(-1) is -1 - 0im. An exact quotient by an
exact 0 is a pole, as a real one is. -z negates both parts; |z|, `abs` and
the bars, is of exact parts the root of the exact sum of their squares,
exact where it is a rational square, |3 + 4im| being 5, and otherwise the
long double nearest the root of the long double nearest the sum, as
`norm([c, d])` is (S257); and of dense parts `cplx.h`'s `abs_c`. `arg` is
`cplx.h`'s `arg_c` in the parts' type, exact parts' long doubles.** The
kind's arithmetic is `cplx.h`'s one form, which has no plain one, and its
modulus and argument the `_c` forms, as `math::complex<T>`'s are (S313),
so that a value and the face give one answer.
- Alternative: a real operand taken as x + 0i, as the textbook writes it:
  (1 + 0im)(-1) is -1 + 0im, its zero's sign lost, and an infinite
  operand meeting a zero is a NaN.
- Alternative: the modulus of exact parts as `hypot` of their nearest long
  doubles: two roundings before the root, where the exact sum takes one.
- Alternative: the plain forms of `abs` and `arg`, as the language's real
  functions are (S194): bits that vary with the C library, as S313 found.

> **Review.** Taken: for, exact arithmetic exact, G2 where a part passes the
> family, as every exact kind has it; floating-point arithmetic cplx.h's, a
> real operand as itself, a zero's sign kept, as C99's G.5 and
> math::complex<T> keep it; the modulus of exact parts rounded once from the
> exact sum, the norm's answer; the modulus and argument of dense parts the
> _c forms', one answer with the face (S313); against, the language's real
> functions are the plain forms (S194), so abs(z) and hypot(real(z),
> imag(z)) of dense parts may differ in a last bit. The alternatives: a real
> operand as x + 0i, a sign lost; the modulus of rounded parts, two
> roundings; the plain forms, the C library's bits. Recommended: as taken.
> Taken.

**S329. A complex number equals a number whose parts are each equal to
its own, a real number's imaginary part being 0, in `value.h`'s exact
order, so that 1/2 + im equals 0.5 + 1.0im and 3 - 0.0im equals 3; it
stands in no other order with any number, a NaN part with itself too. It
converts to a float, a double or a long double where its imaginary part is
0, of either sign, as its real part's nearest, with the rounding's
direction found exactly; never to an integer or a rational, as no exact
complex number is real. The language's `=` and `!=` of a complex number
are by its parts; `<`, `<=`, `>` and `>=` a domain error (S271).** A
dense complex number of imaginary part 0 is a real number, and a fold in a
real type reads it as one, as Julia's `Float64(3.0 + 0.0im)` is 3.0.
- Alternative: no conversion at all, as Python's `float(3 + 0j)` is an
  error: a variable bound to 3 + 0.0im a NaN in a real type.
- Alternative: the real part always, as NumPy converts with a warning: an
  imaginary part dropped silently.

> **Review.** Taken: for, equality by the parts in the exact order, across
> kinds, as the real numbers compare; no order where no field has one; a
> complex number of imaginary part 0 a real one to a fold in a real type, as
> Julia converts; against, 3 + 0.0im converts to 3.0 though its kind is
> complex, which a reader of Python would not expect. The alternatives: no
> conversion, a variable bound to 3 + 0.0im a NaN in a real type; the real
> part always, an imaginary part dropped silently. Recommended: as taken.
> Taken.

**S330. The value layer writes a complex number `3 + 4im`: its real part,
then `+` or `-`, then its imaginary part's magnitude and `im`; exact parts
as integers or fractions, `1/2 - 3/4im`, floating-point parts in
`max_digits10` digits as a matrix's elements are, and a part that is no
finite number by name, `nan + inf*im`. The language's renderer writes it as
a sum, in brackets wherever it stands, as it writes a negative number or a
fraction, `(3 + 4 * im)`, `(1.5 - 0 * im)`, each part as a number with no
literal is written but bare, the least `d_math_imax` as the next one less
1; so that it reads back as a term of its value with implied
multiplication off, as it is by default (S196), or on.** S271 writes
`3 + 4im`, as a person types it with implied multiplication on; the
renderer knows no syntax, and spells the product as it spells every one.
An inexact number reads back as itself in its type, its signed zeros
among them, where a fold in `math::complex<T>` computes the sum (chunk 41).
- Alternative: `(3 + 4im)`: no text the default syntax reads.
- Alternative: `complex(3, 4)`, which reads back a part's -0 or infinity
  exactly as a value too: a call where every system writes a sum.

> **Review.** Taken: for, the value layer's text as every system writes a
> complex number; the renderer's a sum that reads back with implied
> multiplication off, as by default, or on, bracketed as a negative number
> is wherever it stands, its parts as numbers with no literal are written;
> against, the renderer's `4 * im` where S271 writes `4im`, and brackets at
> the top of a text where none are needed, as a negative number has them.
> The alternatives: `(3 + 4im)`, which the default syntax does not read;
> `complex(3, 4)`, a call where every system writes a sum. Recommended: as
> taken. Taken.

**S331. What step 23's later parts compute answers `not_yet` meanwhile, by
the fold, after an operand's error and `wrong_arity`, its message naming
the part: of the second, a function of a complex argument whose principal
complex value it gives, `sqrt`, the trigonometric and hyperbolic functions
and their inverses, `exp`, `exp2`, `log`, `log2` and `log10`, and a power
of a complex number but an exact one's to a whole exponent, which is
exact; of the third, a literal with a complex element, an operation of a
matrix and a complex number, linear algebra's functions of one, and
`complex`, `real`, `imag`, `conj` and `arg` of a matrix. Below the fold, the
same are a domain error: the function table's evaluators, the matrix
kind's arithmetic and blocks, linear algebra's functions, and
`number_traits<value>`'s root of a number of no real value; `expr_matrix`,
which answers an `expr_result`, answers `not_yet`. The functions of the
real line's order, `cbrt`, `floor`, `ceil`, `round`, `trunc`, `fmod`,
`remainder`, `mod`, `atan2`, `hypot` and the factorial, are a domain error
of a complex number now and after, as S271 rules.** A program of chunk 40
never meets a complex value computed one way now and another later: what
it cannot compute it says it cannot yet.
- Alternative: a domain error everywhere until then: an answer later parts
  would change, where `not_yet` says the step will give one.

> **Review.** Taken: for, no answer a later part would change: what chunk 40
> cannot compute answers not_yet with the part that will, after the errors
> that come first; below the fold a domain error, the one error a value has;
> the functions of the real line's order a domain error now, as S271 rules
> for good; against, two chunks in which sqrt(im) has no value, and the
> evaluators' and the kinds' domain errors, which their callers outside the
> fold see, become values in the later parts. Recommended: as taken. Taken.
> Chunk 43 keeps `number_traits<value>`'s real root for good (S355).

**S332. In a floating-point type every number is real: `im` is a NaN, as
sqrt(-1) is there (S271); `complex(x, y)` is x where y is 0, of either
sign, and a NaN otherwise; `real(x)` is x, `imag(x)` +0, `conj(x)` x, and
`arg(x)` carg(x + 0i), pi where x is negative or -0, a NaN for a NaN, and +0
otherwise, pi the constant table's in the type. As a value, `arg` of an
exact real number not negative is exactly 0, 0 among them, and of a
negative one pi in long double.** `arg(0)` is 0, as C99's `carg` (G.6),
Julia's `angle`, Python's `cmath.phase`, MATLAB's `angle` and Mathematica's
`Arg` give it.
- Alternative: `complex(x, y)` a NaN in a type whatever y is: `complex(x,
  0)` no longer x, as it is as a value.
- Alternative: `arg(0)` a domain error, as `atan2(0, 0)` is as a value:
  every system's 0 refused.

> **Review.** Taken: for, every number in a type real, as S271 rules for im;
> complex(x, 0) x in a type as it is as a value; arg(x) carg(x + 0i), as C99
> gives it; arg(0) 0, every system's; against, arg(0) is exactly 0 where
> atan2(0, 0) is a domain error as a value (S203's table). The alternatives:
> complex(x, y) a NaN whatever y is; arg(0) a domain error. Recommended: as
> taken. Taken.

**S333. A complex number's transpose, `z'`, is its conjugate, as `A'` is
the conjugate transpose (S273) and a number is a 1 by 1 matrix, as MATLAB's
and Julia's `'` are; a real number's is itself, as now. An exact complex
number to a whole power is exact, by the repeated squaring the real powers
take, (1 + im) ^ 2 being 2im and (1 + im) ^ (-2) -1/2im; `pow` is `^`.**
- Alternative: `z'` the number itself, a number its own transpose: `A'`
  of a 1 by 1 complex matrix then other than `A[1]'`.

> **Review.** Taken: for, z' the adjoint of a 1 by 1 matrix, as MATLAB's and
> Julia's ' are, so that A' of a complex matrix will agree with each
> element's; exact powers exact by the squaring the real ones take; against,
> z' differs from NumPy's transpose of a number. The alternative: z' the
> number, then other than A' of the matrix [z]. Recommended: as taken.
> Taken.

**S334. The lane's two configurations, `cfg_value.h` and the wide
integer's `cfg_value_wide.h`, which chunk 22 left out, join `dconfig.h`'s
list, as `dconfig`'s checklist asks of every new `cfg_*.h`.** Neither reads
another's knob, so they stand after the layers before them.
- Alternative: `dconfig.h` left as it was, the umbrella the owner's: the
  lane's knobs then unresolved by "resolve everything".

> **Review.** Taken: for, dconfig's checklist kept, the lane's knobs
> resolved by the umbrella with the rest; against, a line in a file the
> owner keeps, which other lanes' configurations are not yet in either. The
> alternative: the umbrella unchanged, the lane's knobs outside "resolve
> everything". Recommended: as taken. Taken.

### S335 to S341: chunk 41, 2026.11.01

The record: `_handoffs/math98_chunk41_2026.11.01/README.md`.

S335 to S341, chunk 41's, as `decisions.md` has them; each is reviewed
there, none flagged. S203's review says the eight are in the table and
`gamma` and `erf` wait for their kernels, still flagged for those; S272's
that the option is given, its drawback documented and still flagged;
S258's and S322's name chunk 43, and S324's notes S335's split. The 18
outstanding decisions of S1 to S341 come first, by group and urgency
(section 2.5). The text below is `decisions_c41.md`, which
`decisions_c41.py` reads.

**S335. Step 23's second part is made in two chunks, each delivered and
checked whole, as S324 made the step: chunk 41 each function's principal
complex value where `cplx.h` has its complex form, the eight functions of
S203 joining the table, a real function's complex value outside its real
domain with `expr_context`'s option (S272), and the folds in
`math::complex<T>`; and chunk 42 the C core's complex `exp2`, `log2`,
`log10`, `log1p` and `expm1`, which `cplx.h` lacks, in both forms and
every family, from the generator, with references of high precision and
bounds as chunk 37 made the others (S266, S268), the face's with them, and
their values in the language. The third part, matrices of complex
numbers, becomes chunk 43. Meanwhile those five answer `not_yet` where
their value is complex, of a complex argument, and `log2` and `log10` of a
negative number and `log1p` of one below -1, in the fold to a value and in
a complex type, naming the second part; below the fold, a domain error.**
Their real values, in their real domains, are the table's from chunk 41.
- Alternative: the five composed in the language from `cplx.h`'s long
  double kernels now, `log2(z)` as `log(z) / ln 2` rounded once: within a
  step for float and double but not the C core's, for neither C programs
  nor the face, and `log1p` and `expm1` near 0 needing kernels of their own;
  values that `cplx.h`'s forms would later change.
- Alternative: chunk 41 with the C core's five and the language's whole
  second part together, the generator, the references and the language
  checked in one delivery.

> **Review.** Taken: for, the C core's five complex functions made as its
> others were, with references and bounds, the face and C programs having
> them too; no value the language gives now that chunk 42 would change; each
> chunk checked whole; against, one more delivery before matrices of complex
> numbers, and meanwhile the five of a complex argument, `log2(-8)` among
> them, answer not_yet. The alternatives: values composed now that
> `cplx.h`'s forms would change, and C programs without them; or one larger
> delivery. Recommended: as taken. Taken.

**S336. A function's complex value is `math::complex<T>`'s, so `cplx.h`'s
`_c` form, as a value and in a complex type alike, as the kind's modulus
and argument already are (S328) and the face's every operation is (S313);
a value's computes in the widest floating-point type of its arguments, a
long double where every one is exact, dense and never made real (S266).
The language's real functions keep their plain forms (S194).** So that a
complex value of the language and `math::complex<T>` give the same bits,
and a fold in `math::complex<T>` the same as the value's of the same
arguments.
- Alternative: the plain forms for complex values, as the real functions
  take them: faster on a fast C library, and bits that differ from the
  face's and between libraries, `cplx.h`'s plain bounds running to 6 steps.
- Alternative: the plain forms in a complex type and the `_c` forms as
  values: two answers for one computation.

> **Review.** Taken: for, one answer (G6): a value, the face and a fold in a
> complex type agree bit for bit, on every compiler and level; the `_c`
> forms within a step for float and double; against, the `_c` forms slower
> than the plain forms on a fast C library, and a complex value's bits other
> than its real counterpart's plain ones where both exist. The alternatives:
> the plain forms, whose bits vary with the library and differ from the
> face's; or two answers for one computation. Recommended: as taken, a
> `_fast` form where a use asks for one (S266). Taken. Ruled 2026.11.04 (the
> owner's): the second alternative, the plain forms in a complex type and
> the `_c` forms as values, two answers for one computation, which the owner
> chose over one answer, the docs saying where a value and
> `math::complex<T>` may differ. Made in chunk 46 (S384).

**S337. A function of exact arguments is exact wherever its value is a
Gaussian rational: `sqrt(z)` and `z ^ (p/q)` where the principal `q`-th
root of `z` is one, `sqrt(2im)` being 1 + im, `sqrt(-4)` 2im and
`(-4) ^ (3/2)` -8im, then raised to `p` exactly; and `sign(z)` where `|z|`
is rational, `sign(3 + 4im)` being 3/5 + 4/5im. The root is found as a
Gaussian integer: with `z = N / D`, `N` a Gaussian integer and `D` whole,
`D` times a root of `z` is a root of `M = z D^q`, an algebraic integer, so
a Gaussian integer where it is a Gaussian rational; `M`'s principal root
is computed in long double, its parts rounded to whole numbers, and taken
where its `q`-th power is `M` exactly and it lies within 2^-10 of its
modulus of the principal root, each other root lying 2 sin(pi / q) of it
away. Where a term or a power passes the family, there is none. No other
function of a complex Gaussian rational is one (Lindemann and Weierstrass,
Gelfond and Schneider), so no other exact value is lost.**
- Alternative: `sqrt` alone exact, by `|z|`: re = sqrt((|z| + a) / 2) and
  im = sign(b) sqrt((|z| - a) / 2), each exactly: no exact root but the
  square root, `(-2 + 2im) ^ (1/3)` being 1 + im among those lost.
- Alternative: exactness for whole powers alone, as chunk 40 left it:
  `sqrt(-4)` a long double 2im.

> **Review.** Taken: for, every exact root and rational power the Gaussian
> rationals have, an estimate taken only where its power is verified
> exactly, so that no exact value is ever wrong; `sign` exact where the
> modulus is; against, a long double estimate and an exact power for each
> root, and roots whose terms pass the family dense. The alternatives: the
> square root alone exact; whole powers alone. Recommended: as taken. Taken.
> Ruled 2026.11.04: as taken.

**S338. `min` and `max` take any number of arguments from 1, as Julia's,
Python's and Mathematica's do; in a floating-point type they are IEEE
754-2019's `minimum` and `maximum`, a NaN where any argument is one and -0
below +0, as Julia's and NumPy's `minimum` are; as a value, the least or
the greatest argument itself, by `value.h`'s exact order, so that an exact
argument stays exact, `min(1/3, 0.5)` being 1/3 as in Mathematica and
SymPy, the first of equal ones but a -0 below a 0, a NaN where any is one,
every argument's flags kept; of a complex number a domain error. `sign(x)`
is 1 or -1 by its sign, a 0 or a NaN itself, as Julia's is, exact for an
exact x; and `sign(z)` of a complex `z` is `z / |z|`, 0 at 0 (S203).**
- Alternative: C's `fmin` and `fmax`, of two arguments, a NaN ignored, as
  MATLAB's `min` ignores one: a NaN that vanishes from a result.
- Alternative: the tower's kind for `min` and `max`, as Julia promotes
  `min(1//3, 0.5)` to 0.333...: an exact argument made inexact.

> **Review.** Taken: for, IEEE 754-2019's operations, a NaN never lost and
> -0 ordered; an exact argument kept exact (G2); `sign` Julia's, a zero's
> sign kept; against, a user of C's `fmin` or MATLAB's `min` meets a NaN
> where those drop it, and `min` of an exact and an inexact argument may be
> the exact one, not the tower's kind. The alternatives: a NaN dropped from
> a result; an exact argument made inexact. Recommended: as taken, a
> NaN-dropping form where a use asks for one. Taken. Ruled 2026.11.04: as
> taken, and the docs say how `min` and `max` differ from `fmin`; made in
> chunk 45 (S379).

**S339. The folds take `math::complex<T>` for T float, double and long
double, as S271 rules: `expr_evaluate<math::complex<T>>`. A real number
stays real there, as in the value: the fold carries with each number
whether it is real, so that a term whose every number is real has its
bits in T, plus +0i; an operator of real operands is T's, and of a complex
operand the face's, a real one taken as itself (G.5); a function of real
arguments is its real evaluator's, but where that is a NaN of arguments
that are not, its principal complex value at x + 0i (S272), so that
`sqrt(-4)` is 2i; of a complex argument its complex evaluator's.
`expr_function` gains an evaluator for each complex type, which every
standard function has, and a caller's function without one is its real
evaluator's of real arguments and a NaN otherwise; `expr_named_constant`
gains a value in each complex type, `im`'s 0 + 1i. In a complex type `x'`
is the conjugate (S333), `=` and `!=` compare the parts, `<`, `<=`, `>`,
`>=`, `mod` and `!` of a complex number are a NaN, and linear algebra's
functions of a number not real answer `not_yet`, the third part's.**
- Alternative: every number complex in the fold, as `std::complex`
  arithmetic is: `-4` there -4 - 0i, so that `sqrt(-4)` is -2i where the
  value's is 2im, and a real term's bits other than T's, `x ^ 2` by `cpow`.
- Alternative: no fold in a complex type, the value's alone (S271's
  alternative), each number boxed.

> **Review.** Taken: for, the fold S271 rules; a real term's bits the same
> in T and in `math::complex<T>`, and a complex value's the same as the
> value's, `sqrt(-4)` 2i in both; a caller's table working there unchanged,
> its functions of real arguments; against, a flag carried with each number,
> a real term's zeros +0 where the value has none, and an operation of a
> complex number and a real one keeping the zeros' signs C99's G.5 gives,
> not those of x + 0i. The alternatives: every number complex, as
> `std::complex` computes, two answers for `sqrt(-4)`; or no fold in a
> complex type. Recommended: as taken. Taken. Ruled 2026.11.04: as taken.

**S340. S272's option is `expr_context`'s first field, `real_results`,
false by default: where it is true, an operation of real operands whose
value is complex alone, `complex(x, y)` apart, is a domain error, of exact
and inexact operands alike, `sqrt(-4)` and `sqrt(-4.0)` both; and so are
`log2` and `log10` of a negative number and `log1p` of one below -1. The
fold applies it, after the function, as it alone has the context.** The
typed folds have no context: in a real type each such value is a NaN, as
C's is, and in a complex type the principal complex value.
- Alternative: the real function's own answer where the option is set, as
  chunk 40 gave it: a domain error of an exact argument and a NaN of an
  inexact one, two answers for one option.
- Alternative: the option on each function's evaluator, which would take
  the context: every evaluator's signature changed, the caller's tables'
  among them.

> **Review.** Taken: for, one answer for one option, a domain error whatever
> the operands' exactness, as S272 asked; the evaluators' signatures
> unchanged, the caller's tables' too; against, the typed folds, which have
> no context, answer as they always would: a NaN in a real type, the complex
> value in a complex one. The alternatives: two answers for one option;
> every evaluator's signature changed. Recommended: as taken. Taken. Ruled
> 2026.11.04 (the owner's): the second alternative, real_results an option
> on each function's evaluator, which takes the context, every evaluator's
> signature changed, the callers' tables' among them, S272 following it.
> Made in chunk 46 (S385).

**S341. A real argument on a branch cut is taken as x + 0i, its zero part
+0, so that its value is the one C99's Annex G gives on the cut's upper
side, as C, Python's `cmath`, NumPy and Scilab give it: `asin(2)` is pi/2 +
1.317...im, `acos(2)` -1.317...im, `atanh(2)` 0.549... + pi/2 im and
`acosh(1/2)` 1.047...im; `sqrt` and `log` of a negative number take the
upper side by every convention.** MATLAB takes the cuts of `asin` and
`acos` past 1 from below: its `asin(2)` is 1.5708 - 1.3170i, the
imaginary part the opposite of Scilab's, as Scilab's own comparison of
the two says.
- Alternative: MATLAB's side for `asin` and `acos` past 1: a real
  argument's value then no longer Annex G's at x + 0i, and a value of the
  language other than `math::complex<T>`'s at the same point.

> **Review.** Taken: for, Annex G's value at x + 0i, so that the language's
> value and `math::complex<T>`'s agree, as C, Python, NumPy and Scilab give
> them; against, a MATLAB user meets the other sign for `asin` and `acos`
> past 1. The alternative: MATLAB's side, a value of the language other than
> the face's at the same point. Recommended: as taken, documented. Taken.
> Ruled 2026.11.04: as taken, and the docs give the MATLAB difference; made
> in chunk 45 (S379).

### S342 to S347: chunk 42, 2026.11.02

The record: `_handoffs/math98_chunk42_2026.11.02/README.md`.

S342 to S347, chunk 42's, as `decisions.md` has them; each is reviewed
there, none flagged. The 18 outstanding decisions of S1 to S347 come
first, by group and urgency (section 2.5); S258 and S322 are due in chunk
43. The text below is `decisions_c42.md`, which `decisions_c42.py` reads.

**S342. `cplx.h` gains `exp2`, `log2`, `log10`, `log1p` and `expm1` of a
complex number, each one long double kernel of pure arithmetic, its parts
carried as pairs to past twice the value's precision and rounded once; and
both forms of every family are that kernel rounded to the family, the `_c`
form as every other's is, and the plain form too, as standard C's library
has none of the five to be faster with: C99 reserves the names `cexp2`,
`cexpm1`, `clog10`, `clog1p` and `clog2` for them, and glibc and the UCRT
give `clog10` alone, as an extension (S266).** `log2` and `log10` are log
|z| and arg z as pairs, times 1/ln b as a pair; `log1p`'s log |1 + z| is
half the logarithm of 1 + w, w = 2x + x^2 + y^2 summed exactly, where |1 +
z|^2 lies in [1/2, 2], and of the squares of 1 + x as a pair and y
elsewhere, its angle with 1 + x as a pair; `expm1`'s real part is (e^x - 1)
- 2 e^x sin^2(y/2) within ln 2 / 2 of the imaginary axis and e^x cos y - 1
past it (S345); `exp2`'s angle y ln 2 is reduced exactly (S344). Each part
within a step of the part correctly rounded, in every family and both forms,
at every reference point.
- Alternative: the textbook formulas in each family's type, log z / ln b,
  exp(z ln 2), log(1 + z), and expm1(x) cos y - 2 sin^2(y/2) beside e^x
  sin y: fast; a step or two off where a value already rounded is divided
  or multiplied by ln b; for `log1p`, many steps off near |1 + z| = 1 and
  every bit of a small z once 1 + z rounds to 1; every bit of `expm1`'s
  real part where it cancels near the curve e^x cos y = 1; and every bit
  of `exp2`'s angle past |y| of some 2^53, where y ln 2 is rounded.
- Alternative: plain forms of their own, in each family's type on the C
  library's real functions, falling back to the kernel where they would
  lose: faster for a C caller, a second set of algorithms per family to
  bound and test, and the cancellations to detect.

> **Review.** Taken: for, every family's parts within a step of the
> correctly rounded ones at every reference point, both forms one answer,
> independent of the C library; against, the plain forms no faster than the
> `_c` ones, and the pairs' kernels slower than C99's formulas would be. The
> alternatives: fast values wrong by many steps where they cancel, or every
> bit; or a second set of algorithms per family. Recommended: as taken, a
> `_fast` form where a use asks for one (S266). Taken.

**S343. On the real axis, where the value is real - every x for `exp2`
and `expm1`, x positive for `log2` and `log10`, x past -1 for `log1p` --
each of the five, in both forms and every family, is that family's real
function in the same form, the `_c` form's `_c` and the plain form's
plain, with y's zero; and on the axes elsewhere the kernel takes the real
function of |x| or |y| for `log2`'s and `log10`'s real part, so that a
power of the base gives its exponent exactly: `log2(-8)` is 3 +
4.532...i.** So that `log2(x + 0i)` is `log2(x)` bit for bit, in C, the face
and the language alike.
- Alternative: the kernel on the axis too, rounded to the family: one path,
  its float and double values there rounded twice, a step from the real
  function's at rare points, and the plain form's never the C library's.

> **Review.** Taken: for, a real argument's value one answer in C, the face
> and the language, the real function's; a power of the base exact; against,
> a test per family before the kernel, and the plain forms' values on the
> axis the C library's, as every plain real function's are. The alternative:
> values on the axis a step from the real function's at rare points.
> Recommended: as taken. Taken.

**S344. `exp2`'s angle, y ln 2, is reduced by pi/2 exactly for every y:
below 1 in magnitude as a pair, ln 2 held as the exponential holds it, the
angle then within pi/4; past it by Payne and Hanek's method, as
`elementary.h` reduces x, with 2 ln 2 / pi's digits in 2/pi's place - a
table of 704 digits of 24 bits, 16,896 bits, which the generator computes
from Machin's formula for pi and 2 atanh(1/3) for ln 2, both ends of their
errors agreeing at the cut.** So that `exp2(x + iy)` is within a step of its
value at every y a family has, 2^118 and 10^300 among them.
- Alternative: y ln 2 as a pair, ln 2 in two parts or three: exact to |y|
  of some 2^30 or 2^60, and past it every bit of the angle lost, as
  NumPy's and Julia's exp(z ln 2) lose it from 2^53.
- Alternative: `exp2(z)` as `pow(2, z)`, whose w log z carries ln 2 as its
  pair: the same limit.

> **Review.** Taken: for, every y, the reduction as exact as the core's
> sine's and cosine's; against, a second table of 2,112 bytes in `cplx.h`
> and a pass of Payne and Hanek's for every |y| of 1 or more. The
> alternatives: a limit on y past which the angle is noise. Recommended: as
> taken. Taken.

**S345. `expm1`'s real part, which vanishes on the curve e^x cos y = 1, is
formed of terms good to some 2^-(P+28) of themselves: e^x - 1 by the
exponential's series in pairs to its eighth term, e^x by the same on x
less k ln 2, ln 2 in three parts, and the sine and cosine of the reduced
angle by their series in pairs to their twelfth terms, written in
`cplx.h`'s section 3.1; `elementary.h`'s pairs round each term in long
double, some 2^-(P+3) of the value, and `power.h`'s exponential of a pair
is good to some 2^-(P+18).** Within some 2^-28 of the curve, relatively,
the real part keeps its precision; nearer, its error is some 2^-(P+28) of
its terms, as any fixed precision's is somewhere.
- Alternative: `elementary.h`'s and `power.h`'s pairs as they are: the real
  part far from its value within 2^-10 of the curve.
- Alternative: the terms in triple pairs, to some 2^-(2P+20): the curve met
  within 2^-60, for three times the work, at points a random argument
  meets one time in 2^28.

> **Review.** Taken: for, the real part within a step everywhere but within
> some 2^-28 of the curve, relatively, where any fixed precision ends
> somewhere; the series written once, in the five's section; against, two
> series in `cplx.h` beside `elementary.h`'s and `power.h`'s, and points
> nearer the curve than 2^-28 off by more. The alternatives: the core's
> pairs, far off within 2^-10; or triple pairs, three times the work.
> Recommended: as taken; triple pairs where a use meets the curve that
> closely. Taken.

**S346. Where a part is infinite or NaN, each of the five takes the value
of the C99 function it is a form of, as Annex G has none of its own: clog's
for `log2` and `log10`, the angle over ln b; clog's at 1 + z for `log1p`,
and -inf with y's zero at z = -1; cexp's for `exp2`, y ln 2 for y; and
cexp's less 1 for `expm1`, so that `expm1(-inf + iy)` is -1 and
`expm1(-0)` -0.** Each keeps conj(f(z)) = f(conj z), as Annex G's
functions do.
- Alternative: NaNs at every special part: values the limits give, such as
  `log2(-inf)` = inf + i pi / ln 2, lost.
- Alternative: tables of their own: no standard to follow, and values that
  would differ from the identities each function's definition states.

> **Review.** Taken: for, the limits' values, as Annex G's own functions
> give them, each the identity its definition states, checked entry by entry
> by `t_cplx.c`; against, none. The alternatives: NaNs, the limits' values
> lost; or tables with no standard. Recommended: as taken. Taken.

**S347. In the language, `exp2`, `log2`, `log10`, `log1p` and `expm1` of a
complex number, and `log2` and `log10` of a negative number and `log1p` of
one below -1, are their principal values, `math::complex<T>`'s and so
`cplx.h`'s `_c` forms in the tower's type (S336), dense, as none of theirs
at a Gaussian rational is one but a real value (S337); `expr_context`'s
`real_results` makes the latter a domain error (S340); and `not_yet` is
step 23's third part's alone.** `log2(-8)` is 3 + 4.532...im, its real part
3 exactly in long double.
- Alternative: an exact real part beside a dense imaginary one, 3 exact in
  3 + 4.532...im: a complex value of mixed exactness, which no other
  function gives, its sums with exact numbers exact in one part alone.

> **Review.** Taken: for, chunk 41's rules as they stand, one answer with
> the face, the `not_yet` gates gone; against, none past chunk 41's. The
> alternative: a complex value of mixed exactness. Recommended: as taken.
> Taken.

### S348 to S356: chunk 43, 2026.11.02

The record: `_handoffs/math98_chunk43_2026.11.02/README.md`.

S348 to S356, chunk 43's, as `decisions.md` has them; each is reviewed
there, S354 flagged, minor. The 17 outstanding decisions of S1 to S356 come
first, by group and urgency (section 2.5); S258 and S322 are done. The
text below is `decisions_c43.md`, which `decisions_c43.py` reads.

**S348. A matrix keeps a complex element as it is, as S239 keeps a real
one: where every element is a float, a double, a long double or a complex
number of such parts, densely, in the widest one's type, as complex numbers
-- math::complex<T>, the C core's struct, row by row - where any element
is complex, a real element x then held as x + 0i, as S273 rules; where any
element is exact or boxed, an exact complex number among them, as values,
each of its own kind. `[1, 2im]` is a matrix of values, exact; a matrix of
`exp(im)` and `sqrt(2)` is one of complex long doubles, its second element
read back as 1.414... + 0im.** So that no element is rounded by being put in
a matrix, and a dense complex matrix is the array cmat.h and LAPACK take.
- Alternative: every complex matrix as values, each element a box of its
  own: no layout the C core takes, each operation boxing every element.
- Alternative: a dense matrix of mixed real and complex elements, each kept
  real or complex: two arrays, or a tag per element, and no kernel takes
  either.

> **Review.** Taken: for, no element rounded by being put in a matrix; a
> dense complex matrix the array cmat.h, LAPACK and the face's
> `matrix<complex<T>>` take, so that the algebra reaches the C core without
> a copy into another layout; an exact complex matrix exact; against, a real
> element of a dense complex matrix costs a zero part, and one exact complex
> element among dense ones makes the matrix one of values, each element
> boxed. The alternatives: every complex matrix as values, each operation
> boxing every element; a mixed dense layout no kernel takes. Recommended:
> as taken. Taken.

**S349. The arithmetic of matrices of complex numbers is the kind's, as
S246 has it, a complex number among the scalars: A + B and A - B of one
shape, -A, A z, z A and A / z, A B. Dense, it computes in the widest type
of its operands, complex where either is, each element by math::complex's
operators, so cplx.h's, a real operand's element or a real scalar taken as
itself, as C99's G.5 and the language's numbers take it, so that a zero's
sign survives and an infinite real part meets no 0 times infinity; a
product's each element the sum from 0, in order, of its row's products
with its column's, as the C++ face's kernels sum a math::complex<T>
matrix's. On values, each element by the value's own arithmetic, exact for
exact elements.** A + z, a matrix plus a number, stays a domain error, as
for a real number (S246).
- Alternative: a real operand widened to x + 0i first, as NumPy and MATLAB
  promote it: two arithmetics for one operation of a real and a complex
  number, the matrix's and the number's, and NaNs where 0 meets an infinity.

> **Review.** Taken: for, one arithmetic for a complex number in a matrix
> and out of one, G.5's, so that `A * 2` and each element times 2 agree bit
> for bit, a zero's sign and an infinity kept; a product's bits the face's
> kernels'; against, a product's sum in order, as every naive product, its
> error growing with n, where an optimized BLAS's blocked or fused sums give
> other bits. The alternative: a real operand widened first, as NumPy
> computes, NaNs where an infinity meets 0. Recommended: as taken; a
> compensated product where a use asks for one. Taken.

**S350. `A'` is the conjugate transpose, as S273 rules, each complex
element conjugated, a real one as it is, a dense complex element's zero
part's sign flipped as cplx.h's conj flips it; `transpose(A)`, a new
function of the table, 52 now, is the transpose, and of a number the number
itself. `real`, `imag`, `conj` and `arg` of a matrix are its elements',
element by element, a matrix of the results kept as S348 keeps them, as
MATLAB's, NumPy's and Julia's are; `complex(A, B)` of two matrices of one
shape is the matrix of `complex(a_ij, b_ij)`, and of a matrix and a number
a domain error, as A + b is (S246).** The C++ form is `expr_transpose(A,
false)`; `expr_transpose(A)` stays `A'`.
- Alternative: `complex(A, b)` with b broadcast to every element, as MATLAB
  scales a scalar: a second rule of shapes beside S246's.
- Alternative: `abs` of a matrix element by element too: |A| is a norm in
  the language's notation (S246), and `norm` the language's.

> **Review.** Taken: for, MATLAB's and Julia's `'` and `transpose`, and the
> parts of a matrix element by element as every system gives them;
> `complex(A, B)` shaped as A + B is (S246); against, one more name in the
> function table, 52, and `A'` other than NumPy's `.T` (S273). The
> alternatives: `complex` broadcasting a number, a second rule of shapes;
> `abs` element by element, where |A| is the language's norm. Recommended:
> as taken. Taken.

**S351. Linear algebra's functions take matrices of complex numbers: in
the type of S257, the widest of the elements', of complex numbers where any
element is one, an exact matrix exactly. Dense, `det`, `inv` and `solve`
are cmat.h's - LU with partial pivoting by |re| + |im|, Gauss-Jordan and
the elimination of [A | B], as LAPACK's zgetrf, zgetri and zgesv - bit for
bit, a real operand of `solve` taken as x + 0i, cmat.h's arrays being
complex; `trace` the kernels' sum from 0. `norm` is of the moduli: dense,
the C core's scaled norm of the 2n parts, re and im in turn, which never
overflows where the norm does not; on values, the root of the exact sum of
re^2 + im^2, exact where it is a rational square. `dot(a, b)` is the sum
from 0, in order, of conj(a_i) b_i, as S273 rules; `cross` conjugates
nothing. Of numbers as scalars (S256): det, trace and eig z itself, inv 1 /
z, a real 1 taken as itself, norm |z|, dot(a, b) conj(a) b, solve(a, b) b /
a, in a complex type too.**
- Alternative: the face's generic kernels for dense det, inv and solve:
  the same bits (S315), cmat.h being the C core's, as decomp.h is the real
  matrices' path.
- Alternative: `dot` bilinear, NumPy's `dot`: dot(a, a) then not norm(a)^2
  (S273's own alternative).

> **Review.** Taken: for, cmat.h's bits, the C core's, in every family; an
> exact matrix exact; the norm that never overflows where the norm does not;
> the conjugating `dot` S273 rules, `dot(a, a)` the norm squared; against, a
> real operand of `solve` taken as x + 0i where the arithmetic keeps it real
> (S349), so that a zero's sign can differ from A \ b computed by hand, and
> no condition estimate beside `solve`, as for real matrices (S257). The
> alternatives: the face's generic kernels, the same bits; a bilinear `dot`.
> Recommended: as taken; `cond` when a step asks, for both. Taken.

**S352. `eig` of a matrix of complex numbers, and of a real one with a
complex pair, gives every eigenvalue (S258, S273). Dense: a real matrix's,
where each is real, as before, bit for bit; otherwise decomp.h's
d_decomp_eigen's, LAPACK's dgeev's path, a pair exactly conjugate, complex
throughout, a real eigenvalue's imaginary part +0; a Hermitian matrix's,
A^H = A exactly, by cmat.h's cyclic Jacobi method with complex rotations,
on A scaled exactly by a power of two, to its Frobenius norm times the
type's epsilon, real; any other complex matrix's by cmat.h's d_cmat_eigen,
zgeev's path, complex. An exact matrix's are found so in long double, each
made exact where it is a Gaussian rational: d A's characteristic
polynomial is monic over the Gaussian integers, d the least common multiple
of A's parts' denominators, so each such eigenvalue's parts are multiples
of 1/d, and the nearest such candidate of each eigenvalue found is tried
exactly, divided out as often as it is a root. Every spectrum is ordered by
real part, then by imaginary part, in value.h's exact order, as Julia sorts
them; a spectrum found real, or rational throughout, ascends as before.
`expr_context`'s `real_results` makes a complex spectrum of a real matrix a
domain error (S340). Each solver is given 30 QR steps a row in all, and at
least the 1000 every matrix was given since chunk 36, which a real matrix of
700 rows ran out of, a domain error; one that converged within 1000 takes
the same steps, so the same bits.** `eig([0, -1; 1, 0])` is `[-im, im]`,
exactly.
- Alternative: a domain error for a complex spectrum, as until now: no
  eigenvalue of a rotation.
- Alternative: S273's conjugate pair always side by side, the negative
  first, even where a real eigenvalue shares the pair's real part: a second
  key beside the parts, where the C core's and Julia's order is the parts'.

> **Review.** Taken: for, every eigenvalue a matrix has, as LAPACK's and
> Julia's `eigvals` give them, a Hermitian matrix's real and accurate to its
> norm's epsilon, an exact matrix's exact wherever it is a Gaussian
> rational, one order for every spectrum, and a budget of steps that grows
> with the matrix, as LAPACK's does; against, an exact matrix's other
> algebraic eigenvalues are long doubles, a dense real matrix's repeated or
> nearly repeated eigenvalue can come as a complex pair with tiny imaginary
> parts, as floating-point QR finds it, and so a domain error under
> `real_results`, and no eigenvectors yet. The alternatives: a domain error
> for a complex spectrum; a second key for pairs. Recommended: as taken;
> eigenvectors when a step asks for `eig`'s second output. Taken.

**S353. With matrices of complex numbers, step 23 is done: no node of the
language answers not_yet but the constants `jm` and `km`, step 24's (S275).
The folds' gates for what step 23's later parts would compute are removed,
the value fold's and the complex types' alike.**
- Alternative: the gates kept, empty, for parts to come: code that nothing
  reaches, which coverage counts.

> **Review.** Taken: for, no gate that nothing reaches, every line coverage
> counts run; against, none. The alternative: empty gates for parts already
> made. Recommended: as taken. Taken.

**S354. The C++ face gives vectors and matrices of math::complex<T> what
S322 promised: `vdot(a, b)`, the sum from 0 of conj(a_i) b_i, the
language's dot's bits, and of a real vector its `dot`; `norm`, `length`
and `norm_squared` of the moduli, a T, the C core's scaled norm and sum of
squares of the parts; `normalize`, each component over the norm, a real
divisor; `distance`, the norm of the difference; `frobenius_norm` of a
complex matrix, a T; and `is_unitary(m [, tol])`, m^H m the identity within
`is_identity`'s tolerance, of a real matrix `is_orthogonal`. `dot` stays
bilinear, and angles and the fluent members stay refused at compile time;
`dynamic_matrix` holds arithmetic elements alone, so takes none of this.**
- Alternative: `dot` conjugating for complex types, as Eigen's does: the
  face's `dot` two different sums by type (S322's own alternative).
- Alternative: the name `inner`: the mathematical one, where `vdot` is
  NumPy's and S322's review named it.

> **Review.** Taken: for, Eigen's and NumPy's vocabulary, the language's
> bits for the same sums, the norms in T as every library gives them; what
> would be wrong still refused at compile time; against, the face's `dot` of
> complex vectors is bilinear where the language's conjugates (S273, S322),
> so that the same name gives two sums across the two faces, and
> `dynamic_matrix` holds no complex element, nor a value or a quaternion, so
> the face has no runtime sized complex matrix, Eigen's `MatrixXcd`, a gap a
> user of Eigen would notice. The alternatives: `dot` conjugating, two sums
> by type in the face; the name `inner`. Recommended: as taken, and
> `dynamic_matrix` of every element type the fixed-size matrix takes,
> complex numbers, values and quaternions at once, at the next step that
> touches it. Ruled 2026.11.04: stands, as recommended, the owner ruling no
> other by the next window. Done in chunk 46 (S392): `dynamic_matrix` of
> complex numbers, values and inline values; quaternions apart, which S392
> flags.

**S355. `number_traits<value>::sqrt` keeps no real root for a negative or
complex number, a domain error, as S331 left it, the trait the ordered real
field's that the face's generic kernels take; and those kernels' norms and
distances sum a new function, `number_modulus_squared(x)`, |x|^2, Julia's
`abs2`: x * x for a number of a real value, so that every real norm keeps
its bits, and for a value's complex number |x * x|, which is |x|^2, exact
where x is exact, as the modulus of a Gaussian rational's square is a
rational (S337). So `norm`, `norm_squared`, `distance` and
`frobenius_norm` of the face's vectors and matrices of values holding
complex numbers are the language's: `norm` of [1 + im, 1 - im] is 2, where
since chunk 40 it summed the squares, 0, and gave 0.** A type whose numbers
are not all real overloads the function beside its traits, as value.hpp
does; a value's quaternion, at step 24, by the same identity, |q^2| =
|q|^2.
- Alternative: the principal complex root in the trait, as the language's
  `sqrt` gives it: a generic kernel's root of a negative quantity, an error
  of its arithmetic, then a complex value, and the norm of [1 + im, 1 - im]
  still 0.
- Alternative: a conjugate among value.h's operations, |x|^2 then x times
  conj(x): a change to the C core's value and to every boxed kind, for the
  same values.

> **Review.** Taken: for, the trait the ordered real field's that the face's
> generic kernels assume, a root of a negative quantity in them an error,
> not a complex value; every real norm's bits kept; a complex number's
> modulus squared exact where it is, by the value's own operations, with no
> new operation in the C core, and the same identity for quaternions;
> against, a product and a root for each complex element, and a dense
> complex element's |z^2| can differ in its last place from re^2 + im^2, the
> language's sum for a matrix of values; the face's `dot` of such vectors
> stays bilinear (S354). The alternatives: the principal complex root in the
> trait, the norm of [1 + im, 1 - im] still 0; a conjugate among value.h's
> operations, the C core's value and every boxed kind changed for the same
> values. Recommended: as taken. Taken.

**S356. The checks: `t_expr_complex_matrix_face.cpp`, new, its
eigenvalues' references mpmath's at 400 bits; the smoke tier against chunk
42's lines; the full tier of every face test, as the generic norms and the
language reach many; the fault trial over the new code, before the queue
and in it; the sanitizers on the tests the change names; the lines run,
each of the chunk's lines of each header it changed; the C++ ladder over
the math lane; the lints; and the levels. No C unit changed, so the C
harnesses' full tier and the C ladder are chunk 42's, run on the same C
files, as chunk 39's were (S323).**
- Alternative: every step, the C core's too: an hour and a half more, for
  files whose bytes chunk 42's run checked.

> **Review.** Taken: for, every changed file checked in each way that
> reaches it, the fault trial before the queue so that its gaps are closed
> in the code checked; against, the C core's harnesses not run again, which
> nothing in the chunk touches, step 1 showing its files unchanged.
> Recommended: as taken. Taken.

### S357 to S368: chunk 44, 2026.11.03

The record: `_handoffs/math98_chunk44_2026.11.03/README.md`.

S357 to S368, chunk 44's, as `decisions.md` has them; each is reviewed
there, S360 and S364 flagged, minor, and S368 flagged, not the
recommendation and significant until chunk 45 does it. The 20 outstanding
decisions of S1 to S368 come first, by group and urgency (section 2.4). The
text below is `decisions_c44.md`, which `decisions_c44.py` reads.

**S357. Step 24 (3f, quaternions) is made in three chunks, as S324 made
step 23: chunk 44, the C core's quaternions grown as S276 rules, and both
C++ faces taking each; chunk 45, the kind in the language as S275 rules --
`jm`, `km`, `quat(w, x, y, z)`, `parts` and the functions, dense as the C
core's array and exact as values - with S277's order of factors and its
right quotient; chunk 46, matrices of quaternions with S277's `A'`, `inv`,
`solve` and Zhang's eigenvalues, and `dynamic_matrix` of every element type
the fixed-size matrix takes (S354's recommendation).** Each chunk delivered
and checked as the others.
- Alternative: two chunks, the C core and the kind together: one chunk of
  twice this one's size, its checks run on more at once.
- Alternative: the kind first, on today's `quat.h`: the language's
  quaternions on the former formulas, then their bits changed.

> **Review.** Taken: for, three chunks each of a size the queue checks in a
> day, the C core first, so that the kind and the matrices are built on the
> bits they keep; the order S324 gave step 23; against, the step's last part
> two chunks away. The alternatives: two chunks, each twice the size; the
> kind first, on formulas then replaced. Recommended: as taken. Taken.

**S358. `quat.h`'s product, inverse and two quotients are sums of exact
products by Ogita, Rump and Oishi's Dot2, each part rounded once: for float
and double in double's pairs where double is evaluated in its own type, as
SSE evaluates it, a float's products exact there and a double's Dekker's;
for long double, and for float and double where double is evaluated wider,
as on a 32-bit x87, in long double's pairs. Each part is as accurate as its
sum computed in twice the pairs' precision and rounded once: within a step
but where the sum cancels, by some 16 u^2 times its terms' magnitudes. The
operands are scaled by powers of 2 where Dekker's products could overflow or
lose their low parts; an operand with an infinite or NaN part takes the
formula as written (`_fast`); an exact sum of products not all 0 is +0, as
IEEE's arithmetic makes it, and a sum of zero products their own sum in
order.** Measured: 0 steps from the correctly rounded part at every
reference but double's 8 at a sum cancelled to 2^-50 of its terms, within
the promise; on the x86-64 here 61 ns a double product and 96 ns a float's,
against 1.0 ns for `multiply_fast` and 431 ns for long double's.
- Alternative: every family in long double's pairs: float's and double's
  exact sum rounded once nearly always, but some 430 ns a product, the
  x87's, where SSE's double pairs take a seventh of it.
- Alternative: each family in its own type's pairs, as `cplx.h`'s
  arithmetic: float's at twice float's precision, where double's pairs give
  float more for the same cost.
- Alternative: an exact sum of each part, as an expansion: correctly rounded
  always, at several times Dot2's cost.

> **Review.** Taken: for, each part of the product, the inverse and the
> quotients as accurate as twice its pairs' precision, as S276 rules,
> measured 0 steps from the correctly rounded part at every reference but
> where a sum cancels to 2^-50 of its terms; float's and double's at SSE's
> speed in double's pairs, a float's products exact; long double's, and the
> x87's, in long double's pairs; the formula's IEEE values for an infinity
> or a NaN; against, a product costs 60 to 100 ns for float and double and
> 430 ns for long double, against 1.0 ns for `multiply_fast`, some 60 to 100
> times, where S276's review put it at three times the operations: without a
> fused multiply-add, which no constant expression has, each exact product
> is Dekker's 17 operations. The alternatives: every family in long double's
> pairs, exact but some 430 ns; each family in its own type's pairs, float's
> at twice float's precision; an exact sum, several times Dot2's cost.
> Recommended: as taken; the `_fast` forms for a renderer's inner loops.
> Taken.

**S359. The inverse and the two quotients of S277: `d_quat_*_inverse`, q* /
|q|^2; `divide(a, b)`, a b^-1, the quotient on the right; `left_divide(a,
b)`, a^-1 b, the quotient on the left; each part a numerator's pair over
|q|^2's, as S358's. 0's inverse is a NaN in each part, the formula's 0 / 0
and Eigen's; an infinite q's the zeros of its conjugate's signs, as 1 / inf
is 0; a NaN's NaNs. Where an operand is not finite or the divisor is 0, a
quotient is a times b's inverse, the product as written.**
- Alternative: 1 / 0 an infinity, as C99's Annex G makes a complex number's:
  a quaternion's infinity has no one direction to give it.
- Alternative: the names `rdivide` and `ldivide`, MATLAB's: `divide` reads
  as C's `/`, which is the right quotient.

> **Review.** Taken: for, the quotient on the right that S277 names `/`, and
> the left one, each part within a step; a zero's inverse NaN, as the
> formula and Eigen give it; an infinity's inverse 0, as 1 / inf is;
> against, no infinity for 1 / 0, where a complex number has Annex G's. The
> alternatives: 1 / 0 an infinity, with no one direction; MATLAB's names.
> Recommended: as taken. Taken.

**S360. The rotation is q v q^-1 for any q not 0: `rotate`, and
`to_matrix3` and `to_matrix4`, R / |q|^2, so that a q a few steps from unit
length names the rotation of q / |q|, as v' = q v q* does for a unit q (S276):
each part from pairs of exact products in long double, over |q|^2's pair,
within a step (measured 0 for float and double, 1 for long double). q = 0, or
a part not finite, gives NaNs; v = 0 is left as it is. `rotate_fast`,
`to_matrix3_fast` and `to_matrix4_fast` keep the former formulas, for a unit
q: S276's "the rotation" taken to include its matrices, which a renderer
uploads.** Some 900 ns a rotation, the kernel's long double, against 5 ns
for the `_fast` form, as the faces' `rotate` is the accurate one.
- Alternative: q v q* computed accurately: a q a few steps off unit length
  scales v by |q|^2, which q v q^-1 does not.
- Alternative: the rotation and its matrices in double's pairs for float
  and double, as the product (S358): some seven times faster, at more code.

> **Review.** Taken: for, the rotation that any q not 0 names, the rotation
> of q / |q|, each part within a step, so that rounding's few steps off unit
> length do not scale v; the former formulas kept as the `_fast` forms, the
> matrices among them, for a renderer; against, the accurate rotation costs
> some 900 ns in long double's pairs, against 5 ns for `rotate_fast`, and
> the faces' `rotate` is the accurate one; q = 0 gives NaNs where the former
> formula gave v. The alternatives: q v q* accurately, which a q off unit
> length scales; the rotation in double's pairs, as the product, seven times
> faster. Recommended: as taken, and the rotation and its matrices in
> double's pairs for float and double, as the product's (S358), at the next
> step that touches `quat.h`'s kernels. **Flag:** minor: the accurate
> rotation costs some 900 ns, near two hundred times the `_fast` form's.

**S361. The norm and normalization: `norm_c` and `norm`, the scaled norm as
a pair rounded once, +inf for an infinite part, NaN for a NaN, +0 for 0;
`normalize_c` and `normalize(out, q)`, each part over the norm's pair, q = 0
left as it is, an infinite part's quaternion the direction of its
infinities, the limit of q / |q|, a NaN's NaNs. `normalize` takes no norm of
the caller's now, as S276 puts the scaled norm in it; the `_c` and plain
forms give the same bits, the root correctly rounded either way.
`normalize_fast` is the reciprocal square root of the sum of squares, in the
family's type and unscaled.** Measured 0 steps in every family.
- Alternative: `normalize(out, q, norm)` as before: each caller's norm and
  its accuracy its own.

> **Review.** Taken: for, `normalize` scaled and within a step as S276
> rules, with no norm of the caller's to be wrong; the limits of q / |q|
> where a part is infinite; the two forms the same bits; `normalize_fast`
> the renderer's reciprocal root; against, the three-argument `normalize` is
> gone, so its callers change (the faces, here). The alternative: the
> caller's norm, of the caller's accuracy. Recommended: as taken. Taken.

**S362. Construction: `from_matrix3` and `from_matrix4` by Shepperd's
method, the largest of 4w^2, 4x^2, 4y^2 and 4z^2 - w first among equals,
then x, y, z - giving its part, the others the off-diagonal sums and
differences over 4 times it, then q or -q, whichever has w >= 0; a NaN or
infinite entry gives NaNs; the result not normalized after, so that an
orthogonal matrix's quaternion is within a step and a nearly orthogonal one's
as near unit length as the matrix is orthogonal. `from_two_vectors(a, b)`,
the shortest rotation turning a's direction into b's: (a x b, |a| |b| + a .
b) normalized, the sum taken as |a x b|^2 / (|a| |b| - a . b) where a . b <
0, so that it does not cancel; exactly opposed, the half turn about a x e_k,
e_k the unit vector along a's smallest part (the first among equals); a zero
vector, the identity. `nlerp(a, b, t)`, (1 - t) a + t b along the shorter
arc, normalized, exact at the ends before the root.**
- Alternative: Eigen's sign, w's as the branch leaves it: one rotation's
  quaternion either sign by which candidate was largest.
- Alternative: from_matrix normalized: a step more for every exact rotation
  matrix.
- Alternative: an opposed pair's axis from a fixed axis's cross product, as
  GLM's: none where a lies along that axis.

> **Review.** Taken: for, Shepperd's method, the stable one, with one sign
> for every rotation; a nearly orthogonal matrix's quaternion as near unit
> length as the matrix is orthogonal, which normalizing would hide; two
> vectors' quaternion without cancellation where they nearly oppose, and an
> axis for exactly opposed ones whatever a's direction; nlerp along the
> shorter arc; against, from_matrix of a matrix far from orthogonal gives a
> quaternion far from unit length, which the caller normalizes. The
> alternatives: Eigen's sign; a normalized from_matrix; GLM's fixed axis.
> Recommended: as taken. Taken.

**S363. `slerp` along the shorter arc, b or -b: theta = 2 atan2(|a - b|, |a
+ b|), accurate near 0 where acos(a . b) is not; at theta = 0 the line (1 -
t) a + t b; sin((1 - t) theta) / sin theta and sin(t theta) / sin theta
otherwise, each part the two products' sum. a at t = 0, b or -b at t = 1,
and a at every t between a and a, exactly. `slerp_c` is one long double
kernel, each part a pair, rounded once; the plain `slerp` the same algorithm
in the family's type on the C library's. Its parts are sums that may cancel,
so it is measured against its largest part: `slerp_c` within a step of it,
and 2 for long double (measured 0, 0 and 2), the plain one within 4
(measured 2).**
- Alternative: the former acos(a . b), and a renormalized line past a . b =
  0.9995: another curve near 0, and acos's error growing as 1 / theta there.

> **Review.** Taken: for, the angle accurate near 0, so that slerp is one
> curve at every angle; its ends exact; a at every t between a and a;
> against, slerp's parts are bounded against its largest part, not each its
> own, as they are sums that may cancel. The alternative: the former acos
> and line. Recommended: as taken. Taken.

**S364. exp, log, sqrt and real powers are the complex functions in q's
plane (S275): f(q) = Re f(w + r i) + u Im f(w + r i), r = |v|, u = v / r;
for v along im alone, y and z 0, `cplx.h`'s function of w + x i itself, bit
for bit, y and z as given, so that a complex number's value is the same as a
quaternion's. In the `_c` kernels r is a pair and f corrected to first order
for its low part; log and sqrt take q scaled by a power of 2 only outside the
safe range, as near |q| = 1 the scale's k ln 2 would cancel log |q|; sqrt's
vector part is v / (2 Re) where the real part is the larger, 2 Re Im = r
being exact, and log's v / w where r < w 2^-(P/2 + 2). A part of v that is 0
stays 0, whatever it multiplies, an infinity or a NaN among them; a NaN part
gives NaNs; an infinite part of v an infinite r along v's infinities. The
`_c` forms are within a step for float and double and 4 for long double
(measured 0, and 2 each), pow_c's of its largest part (measured 0, 1 and
3), as a whole power's parts, by `cplx.h`'s repeated squaring, may cancel
(S368); the plain forms, in the family's type with |v| rounded once, within
12, 4, 4 and 12 steps of the largest part at the references' |v| below 16
and |p| below 8 (measured 8, 3, 2 and 6), exp's and pow's error growing with
|v| and p, as the cosine of a rounded |v| does.** The faces take the `_c`
forms (S365).
- Alternative: the plain forms on the `_c` kernels' algorithm in long double
  on the C library's functions: as accurate as the `_c` forms, at long
  double's cost.
- Alternative: q^p for an integer p by repeated products: exact where the
  products are, but a power with two definitions.

> **Review.** Taken: for, the complex functions' principal values, a complex
> number's bit for bit, as S275 rules; the `_c` forms within a step for
> float and double, r carried as a pair so that a cosine of |v| near 0 is
> right; zero parts of v kept 0, so that the complex case's jm and km parts
> stay 0 whatever the plane's value; against, the plain forms take |v|
> rounded once, so that exp's and pow's errors, measured against the largest
> part, grow with |v| and with p: 8 and 6 steps at |v| below 16. The
> alternatives: the plain forms in long double on the C library's, as
> accurate as the `_c` forms at long double's cost; integer powers as
> products. Recommended: as taken for the `_c` forms, which the faces take;
> the plain forms carrying |v| as a pair, as the product's double pairs do,
> at the next step that touches `quat.h`'s kernels. **Flag:** minor: the
> plain exp's and pow's error grows with |v| and p.

**S365. The C++ faces, `math::quaternion<T>` and `linalg::quaternion<T>`,
take each: `operator*` accurate and `multiply_fast`; `inverse`, `operator/`,
the right quotient, and `left_divide`; `norm` the C core's (no longer vec.h's
norm of the four parts); `normalize` and `normalize_fast`; `rotate` and
`rotate_fast`; `to_matrix` (`to_matrix3`) and `to_matrix4`, and their `_fast`
forms; `from_matrix` of a 3x3 or a 4x4 matrix; `from_two_vectors`; `slerp`,
`slerp_fast` and `nlerp`; `exp`, `log`, `sqrt` and `pow`. The accurate forms
at every level: slerp, exp, log, sqrt and pow the C core's `_c` forms,
constant expressions from C++14, as `math::complex`'s functions are; what
takes a root alone by level, the same bits; `from_axis_angle` and
`slerp_fast` the C library's at run time and constant expressions from
C++20, as before. The generic path, any other T: the algebra in T's own
arithmetic by the same definitions, exact where T is; roots as `norm` takes
them; slerp, exp, log, sqrt and pow through double by the C core's double
`_c` forms; the `_fast` forms the former expressions.**
- Alternative: slerp, exp, log, sqrt and pow on the C library at run time,
  as from_axis_angle: faster, a few steps off and exp's growing with |v|.
- Alternative: the generic path's slerp in T's arithmetic with its sines
  through double, as before: its own formula, a different curve.

> **Review.** Taken: for, every function of S276 in both faces, the accurate
> forms by default and constant expressions from C++14, as math::complex's;
> the `_fast` forms for a renderer; the generic path by the same
> definitions, exact where its type is, and through double where a sine or
> an exponential is; against, the faces' slerp, exp, log, sqrt and pow take
> the `_c` forms at run time too, 1 to 2 microseconds a call; the faces'
> `norm` and `normalize` change their last bits. The alternatives: the C
> library's at run time; the generic slerp's former formula. Recommended: as
> taken. Taken.

**S366. `elementary.h`'s `round_pair`, which rounds a long double pair once
to float or double, gives an infinity for a finite pair past the family's
largest finite value, but for the midpoint above it with the sum below,
which rounds to the largest value. It gave the largest finite value wherever
the low part pointed back, as the midpoint test met infinity less infinity;
the quaternions' references found it, a left quotient past DBL_MAX coming
back DBL_MAX. Only values past a family's range change.**
- Alternative: each kernel testing for overflow before it rounds: the same
  test in every caller, and a caller that forgets it.

> **Review.** Taken: for, an infinity where the value is past the range,
> every family's round_pair right at its edge; the change only there;
> against, `elementary.h` changes in a quaternion chunk, so the C harnesses
> and the C ladder run again. The alternative: a test in every caller.
> Recommended: as taken. Taken.

**S367. The checks: `t_quat.c` against `math_tests_quat_refs.h`, which
`refs_c44.py` makes, mpmath's values at 400 bits settled at 640, an exact
dyadic value written exactly, so that one on a midpoint stays there, as 45
digits moved a product's off it, a false step; each function's, family's and
form's largest distance printed; the product held to Dot2's promise; the
`_fast` forms against the former formulas, bit for bit; identities; special
values, the range's end and midpoints among them; each C++ overload; and
the constant expressions against the run time's bits from C++14.
`t_quaternion_face.cpp` grown with the faces, each function the C core's
bit for bit in the form its level takes, and the generic path against the
C core. The fault trial over the new code, before the queue and in it, by
the C harness where double is evaluated in its own type and on the x87,
and by the face test at C++11, C++17 and C++20; the sanitizers; the lines
run; the C++ ladder; the lints; the levels. As C units changed, the queue
runs the C harnesses' full tier and the C ladder again (steps 4 and 5);
the render module's tests are built and run over the lane's math headers,
as the restructuring script will lay them; and the cost of each form is
measured (`timing_c44.c`).**
- Alternative: chunk 43's queue, without steps 4 and 5: the C files changed
  left to the smoke tier's one build each.

> **Review.** Taken: for, every accurate function measured against
> references made independently, at every family's range and width, each
> form's largest distance in the record; the references exact where the
> value is, so that no false step is counted; each path of the algebra,
> double's pairs and the x87's, and each level of the faces under the fault
> trial; the steps the C core's change needs, 4 and 5, run again; the render
> module over the lane's headers, as it will be; the cost of each form
> measured; against, a longer queue. The alternative: chunk 43's queue.
> Recommended: as taken. Taken.

**S368. `cplx.h`'s whole powers, z^n for n from -2^30 to 2^30 by repeated
squaring as chunk 42 made them, each product rounded to the family, or to
long double in the `_c` kernel, stay as they are in this chunk: within their
bounds of the larger part, but not of each part where one cancels - a cube
of w + r i, its imaginary part some 2^-10 of its modulus, comes back
hundreds of steps of that part off in long double, as the quaternions'
references found at random points, `cplx.h`'s own having none such. The
quaternions' `pow_c`, whose plane's power it is, is measured and bounded of
its largest part meanwhile, as slerp is (S363), and `cplx.h`'s banner says
so. Recommended: the `_c` kernel's repeated squaring carried in pairs, so
that each part is within its bound again, exact where the power is, and the
plain forms' whole powers by it, rounded once; at the next chunk, before
the language's quaternions, with references at such points
(`raw/pow_probe.txt` has this chunk's).**
- Alternative: the fix in this chunk: `cplx.h`'s kernel, its tests and its
  references changed beside `quat.h`, a second C unit's change in the
  quaternions' first part.
- Alternative: whole powers by exp(n log z), the general path, its angle
  carried in pairs: each part accurate, but a Gaussian integer's square no
  longer exact.

> **Review.** Taken: for, the quaternions' first part kept to `quat.h` and
> its faces, the finding recorded with its measure and its fix, the
> quaternions' bound stated as it holds; against, `cplx.h`'s whole powers
> keep, until the next chunk, an error of some steps of the larger part in a
> part that cancels, in both forms and every family, which its banner now
> states. The alternatives: the fix in this chunk; whole powers by exp(n log
> z). Recommended: the pair kernel, at the next chunk, before the language's
> quaternions. Taken in chunk 45, before the language's quaternions: the
> pair kernel, every part within its bound again (S370).

### S369 to S382: chunk 45, 2026.11.06

The record: `_handoffs/math98_chunk45_2026.11.06/README.md`.

S369 to S382, chunk 45's, as `decisions.md` has them; each is reviewed
there, S376 flagged, a significant drawback until 4a, and S370 flagged, a
significant drawback, its plain forms' cost. The owner's rulings of 11-03
and 11-04 are recorded in the reviews they rule; S336 and S340, his choice
of an alternative each review had set aside, are flagged until chunk 46
makes them, and S368, done, is flagged no more. The 23 outstanding
decisions of S1 to S382 come first in `decisions.md`, by group and urgency.
The text below is `decisions_c45.md`, which `decisions_c45.py` reads.

**S369. Step 24's second part, chunk 45, makes quaternions values of the
language, as S275 and S277 rule, and leaves matrices of them to its third,
chunk 46, as S357 planned. Until then a literal holding a quaternion, a
product or a quotient of a matrix and a quaternion, and linear algebra's
functions of a quaternion but `inv`, which S275 names, answer
`expr_status::not_yet`, each with its message - "a matrix of quaternions:
not yet (step 24, 3f, its third part)", "det of a quaternion: not yet (step
24, 3f, its third part)" - after an operand's error, as S331 had chunk
40's nodes answer; below the fold, `expr_matrix` answers an element that is
a quaternion `not_yet` too, and the matrix kind's arithmetic, a literal's
blocks and linear algebra's operands refuse one, a domain error. A sum of a
matrix and a quaternion stays a domain error, as a matrix plus a number is
(S246). `slerp`, a rotation's matrix and the quaternion of one come to the
language with chunk 46's matrices, as each takes or gives one.**
- Alternative: the third part's nodes a domain error now: an answer that
  chunk 46 would change.
- Alternative: matrices of quaternions in this chunk: a chunk of twice this
  one's size, its checks run on more at once.

> **Review.** Taken: for, no answer the third part would change: each node
> it computes answers not_yet with its message, after the errors that come
> first, as chunk 40's did (S331); a sum of a matrix and a quaternion a
> domain error now and after, as S246 has every matrix plus a number;
> against, two chunks in which [1, jm] has no value and det(jm) waits, and
> the matrix kind's refusals, which callers below the fold see, become
> values in chunk 46. The alternatives: a domain error that chunk 46 would
> change; a chunk of twice the size. Recommended: as taken. Taken.

**S370. `cplx.h`'s whole powers, z^n for n from -2^30 to 2^30, are carried
in long double's pairs, as S368 recommended: repeated squaring from the
exponent's highest bit, each product's parts sums of Dekker's exact products
by `power.h`'s pairs, the larger part rescaled into [1, 2) after each step
and its binade kept apart, so that no step overflows or underflows, the
binade clamped where the result is surely an infinity or 0; a negative n's
power inverted at the end, as pairs, the conjugate over the squared modulus;
each part then scaled and rounded once, into the subnormals as the
exponential scales its own. The plain forms take the same kernel, bit for
bit, as no C library has a faster whole power to the same bound. Each part
is within a step of the correctly rounded part where it is at least |n| 2^(4
- P) of the larger part, P long double's precision, and below that, where it
may cancel past what the pairs carry, within |n| 2^(3 - 2P) of the larger
part; a power exact in twice long double's precision comes exactly, and an
exact zero part takes the sign `d_cplx_ld_mul`'s and `d_cplx_ld_div`'s exact
products give. `quat.h`'s `pow_c`, whose plane's power it is, is bounded of
each part again: within a step for float and double and 4 for long double
(measured 0, and 2).** Measured at the 1,027 points of
`math_tests_cplx_pow_refs.h`: every part above its threshold within 0 steps
in every family, and the 61 long double parts below it within 0.031 of their
allowance; each whole power costs 461 to 1,690 ns a call from n = 2 to 100,
in both forms and in double and float alike, where chunk 44's plain forms
took 131 to 823 ns for double and 81 to 507 for float and its `_c` form 278
to 1,851: the plain forms 2.0 to 3.5 times as long for double and 3.3 to 6.2
for float, the `_c` form 0.9 to 1.7 times (`raw/timing.txt`).
- Alternative: whole powers by exp(n log z), the angle carried in pairs:
  each part accurate, but a Gaussian integer's square no longer exact.
- Alternative: the plain forms' whole powers squared in each family's own
  type, as chunk 42 made them, the `_c` form's alone in pairs: faster, but
  a part that cancels some steps of the larger part off in the plain forms,
  two bounds for one power.

> **Review.** Taken: for, every part of a whole power within its bound
> again, a part that cancels among them, in both forms and every family, as
> S368 recommended; a Gaussian integer's power exact still; one kernel for
> both forms, so that the plain form's whole power is the `_c` form's bit
> for bit; `quat.h`'s `pow_c` bounded of each part again; against, the plain
> forms' whole powers 2.0 to 3.5 times as long as chunk 44's for double and
> 3.3 to 6.2 for float, the `_c` form's 0.9 to 1.7 times; for float and
> double each part is the long double's conversion, as every `_c` form's is.
> The alternatives: exp(n log z), no Gaussian integer's power exact; the
> plain forms in their own type, two bounds for one power. Recommended: as
> taken, every part within its bound before a power is fast, as the
> roadmap's seventh principle has it; chunk 42's squaring in the family's
> own type, the faster, kept for a `_fast` form at step 40 (6e), where
> measured demand asks for one. **Flag:** significant drawback: the plain
> forms' whole powers cost 2.0 to 3.5 times chunk 44's time for double and
> 3.3 to 6.2 for float, n from 2 to 100.

**S371. A quaternion is a boxed kind of the value, `expr/quaternion.hpp`'s,
of rank 950: past the complex number's 900, so that in an operation of a
quaternion and a real or complex number the quaternion's table handles it,
and below the matrix's 1000, whose table handles a quaternion operand
(S275, S238). It holds its parts as values where all four are exact, or
where one is of a boxed kind that is not exact, and otherwise densely, as
`math::quaternion<T>` of the widest floating-point type among them, the C
core's array [x, y, z, w], each part its nearest there (S266). An exact
quaternion whose jm and km parts come to 0 is the complex number of its
other two, or the real number w where its im part is 0 too, as an exact
complex number of imaginary part 0 is real (S327); an inexact one keeps its
four parts, its zeros with their signs, as `quat(x, 0, 0, 0)` of a double x
does. The kind's exact entry says which values are exact (S325), and its
holders are counted atomically (S134): `jm` and `km` are a box each.**
- Alternative: dense always, in long double where the parts are exact:
  (1 + 2jm)(3 - km) no longer exactly 3 - 2im + 6jm - km.
- Alternative: an inexact quaternion of jm and km parts 0 made complex
  too: a dense value changing kind with its parts' values, its zeros'
  signs lost, an operation's kind no longer its operands'.

> **Review.** Taken: for, exact quaternions exact, as S266 rules for every
> kind, and dense ones the C core's array, so that the language's dense
> arithmetic is `quat.h`'s bits; an exact quaternion of jm and km parts 0
> the complex number it equals, as S275 rules, so that jm km is im; an
> inexact one its own four parts, its kind its operands', its zeros kept;
> against, a value of exact parts holds four values, four times a complex
> number's, and a boxed inexact part keeps the parts as values, computed by
> its kind's operations one by one. The alternatives: dense always, no exact
> quaternion; an inexact one made complex, its zeros' signs lost.
> Recommended: as taken. Taken.

**S372. A quaternion's arithmetic is the value's own, through the kind's
table. Exact parts compute exactly by `value.h`'s operations: Hamilton's
product, im jm = km, of sixteen products, and the quotient p / q = p
inv(q), the quotient on the right (S277), p q* over |q|^2, an exact 0
divisor a pole; a part past the family becomes G2's long double, flagged.
Otherwise an operation computes in the widest floating-point type of its
operands, each exact part its nearest there, by `quat.h`'s accurate forms
through `math::quaternion<T>`: the product and the quotient each part
rounded once from Dot2's sums (S358, S359), a sum each part rounded once. A
real operand is taken as itself, as C99's G.5 takes one beside a complex
number: a sum or a difference changes w alone, and a product or a quotient
by it scales each part, so that a zero part's sign survives; a complex
operand is taken with its im the first unit, a sum changing w and x alone;
a real or complex number over a quaternion is the full quotient. No
operator takes the quotient on the left: inv(q) p is written so, as S277
rules.**
- Alternative: `\` the quotient on the left, MATLAB's, as chunk 44's
  handoff had it: an operator S277 did not give, one more for the parser
  that S190's agreement is deciding.
- Alternative: a real or complex operand taken as a quaternion of +0 parts:
  one path for every operand, a zero part's sign lost, as G.5 avoids for
  complex numbers.
- Alternative: dense arithmetic by the `_fast` forms, the former formulas:
  some 60 times faster in double, each part within a few steps, not one.

> **Review.** Taken: for, exact arithmetic exact, the quotient on the right
> as S277 rules; dense arithmetic `quat.h`'s accurate forms, each part
> within a step; a real or complex operand taken as itself, so that a zero's
> sign survives as C99's G.5 keeps it for complex numbers; against, a dense
> product or quotient costs Dot2's pairs, some 60 ns in double where the
> former formula took 1, as S358 measured; the quotient on the left has no
> operator of its own. The alternatives: `\`, an operator S277 did not give;
> every operand a quaternion of +0 parts, the zeros' signs lost; the `_fast`
> forms, a few steps off. Recommended: as taken. Taken.

**S373. |q| is the norm: of exact parts the root of the exact sum of their
squares, exact where it is the square of a rational of the family, |1 + 2im
+ 2jm + 4km| being 5, and otherwise the long double nearest it; of dense
parts `quat.h`'s scaled norm, the root correctly rounded (S361). A
quaternion equals a number whose parts equal its own, a real or complex
number's missing parts 0, by `value.h`'s exact order, and stands in no
order otherwise, nor with a NaN part; the language's < <= > >= of one are a
domain error. It converts to a float, double or long double where its im,
jm and km parts are 0, of either sign, its w's nearest, with the rounding's
direction; no exact quaternion converts, as none is real. Its text is w,
then each other part's sign, magnitude and unit, 1 + 2im - 3jm + 4km, exact
parts as integers or fractions, floating-point ones in max_digits10 digits,
a part that is no finite number by its name, nan*jm; the language writes
one in brackets, (1 + 2 * im - 3 * jm + 4 * km), and reads it back as its
value.**
- Alternative: `abs` the norm squared, as some libraries' `norm` is: no
  root to round, but not |q|, which the bars mean.
- Alternative: an order of the parts taken in turn, for sorting: an order
  that no algebra of the quaternions keeps.

> **Review.** Taken: for, |q| the norm the bars mean, exact where rational,
> the C core's correctly rounded root where dense; equality by the parts, no
> order, as no field's order of the quaternions exists; conversion where the
> number is real, never of an exact quaternion; a text that reads as the
> language writes it; against, a quaternion is unordered with every number,
> so `min`, `max` and the order relations take none. The alternatives: the
> norm squared, not |q|; an order no algebra keeps. Recommended: as taken.
> Taken.

**S374. The language's functions of a quaternion are those S275 names.
`quat(w, x, y, z)` makes one of four real parts; `parts(q)` gives its parts
as the row [w, x, y, z], of a real or complex number too, a missing part a
0 of its parts' kind; `real` and `conj` give its real part and conjugate,
and `q'` its conjugate, as a complex number's z' is (S333); `abs` and the
bars its norm; `inv(q)` its inverse, exact for exact parts; `sign(q)` q /
|q|, exact where |q| is rational, a 0 itself; `exp`, `log` and `sqrt` the
complex functions in q's plane, `quat.h`'s `_c` forms in the tower's type,
dense, `sqrt` exact where the root is a quaternion of rationals; and q ^ r
of a real r exact by repeated squaring for a whole r, a negative one the
inverse's, and for a rational p/s where q's principal s-th root is a
quaternion of rationals, the Gaussian root in q's plane (S337) where |v| is
rational, and otherwise `quat.h`'s `pow_c`. Every other function of a
quaternion is a domain error - `imag` and `arg`, whose parts `parts`
gives, `complex(x, y)`, the functions of the real line's order, the
trigonometric and hyperbolic functions and their inverses, `exp2`, `log2`,
`log10`, `log1p` and `expm1` - and so is a quaternion exponent of any
base, and a complex one of a quaternion, whose two orders give two
values.**
- Alternative: `imag(q)` the vector part [x, y, z] and `arg(q)` its angle,
  atan2(|v|, w): functions that give a number of every other number giving
  a vector of a quaternion.
- Alternative: every function of the complex numbers in q's plane, as
  `exp`, `log` and `sqrt` are: `sin(q)`, `log2(q)` and the rest, past
  S275's list, each without `quat.h`'s kernel.
- Alternative: `parts(q)` a column: the language's vectors are rows, as
  its literal [a, b] is (S240).

> **Review.** Taken: for, the functions S275 names, each its principal
> value, the complex function in q's plane, `quat.h`'s bits; exact roots and
> powers wherever they are quaternions of rationals; every other function a
> domain error, so that no value is given where the language has no kernel
> or no one answer; against, `imag`, `arg`, the trigonometric functions and
> `log2` of a quaternion have no value, where some libraries give one, and a
> user writes `parts(q)` for the vector part. The alternatives: `imag` and
> `arg` giving a vector and an angle; every complex function in q's plane,
> without kernels; `parts` a column. Recommended: as taken. Taken.

**S375. `rotate(q, v)` is the 3-vector q v q^-1, as S360 makes the C
core's: v a vector of 1 by 3 or 3 by 1 of real numbers, the result of its
shape; any q not 0 names the rotation of q / |q|, a real one rotating
nothing; exact where q's parts and v's elements are, by q's rotation matrix
times v over |q|^2, each element a sum of exact products, and otherwise
`quat.h`'s accurate rotation in the widest type of the operands, each
element within a step (S360). An exact 0 names no rotation, a domain error,
as do a vector of other than three real numbers, a matrix for q and a
boolean.**
- Alternative: q made a unit first, its rotation then q v q*: an exact q
  not of unit norm no longer exact, its norm irrational.
- Alternative: a 0 q's rotation NaNs, as `quat.h`'s: a domain error says
  better what an exact 0 is.

> **Review.** Taken: for, the C core's rotation of any q not 0, exact where
> q and v are, of v's shape; an exact 0 a domain error; against, an inexact
> 0 gives the C core's NaNs, not an error, as its exactness is not known.
> The alternatives: q made a unit first, no exact rotation by a q of
> irrational norm; NaNs for an exact 0. Recommended: as taken. Taken.

**S376. Simplification keeps a quaternion's factors in their order, as S277
rules: a factor that holds one, `jm`, `km`, a call of `quat` or a quaternion
a fold made, keeps its place among the others that do, as a matrix's does
(S251); and in a product that holds a quaternion, a factor that holds a
complex number, `im`, a call of `complex` or a complex number a fold made,
keeps its place too, as im jm is km and jm im -km. Two terms are alike only
where such factors stand in one order, so that jm * km + km * jm stays and
jm * km + jm * km is 2 * jm * km; an exact quaternion, or an exact complex
number beside one, is never part of a coefficient. A real number commutes
with every factor, and so does a variable, taken for a real number, as S251
takes one for a number: 2 * jm * 3 is 6 * jm, and x * jm + jm * x is 2 * x *
jm.**
- Alternative: a variable keeping its place beside a quaternion: x * jm +
  jm * x kept as it is, where x is bound to a real number, the common case,
  as where it is bound to im.
- Alternative: every factor of a product that holds a quaternion keeping
  its place, numbers among them: 2 * jm * 3 no longer 6 * jm.

> **Review.** Taken: for, the order S277 rules: a quaternion's factors and a
> complex number beside them keep their places, so that no simplification
> changes a product of jm, km and im; numbers and variables still collected,
> as S251 collects them beside matrices; against, a variable is taken for a
> real number, so that x * jm + jm * x becomes 2 * x * jm, which bound to
> im, or to a quaternion, is another value, as S251's x * y + y * x is for
> matrices. The alternatives: variables keeping their places, x * jm + jm *
> x uncollected where x is real, the common case; every factor in its place,
> 2 * jm * 3 uncollected. Recommended: as taken, with S251's declared
> domains at step 25 (4a), a variable declared complex or a quaternion then
> keeping its place. **Flag:** significant drawback until 4a: a variable
> bound to a number that does not commute with a quaternion's factors gets
> another value from a simplified term.

**S377. In float, double and long double `jm` and `km` are NaNs, as `im`
is (S271); `quat(w, x, y, z)` is w where x, y and z are 0, of either sign,
and a NaN otherwise, as complex(x, y) is (S332); `parts` and `rotate`,
which give vectors, are NaNs, as `cross` is (S351); and a variable bound to
a quaternion is its w's nearest where its other parts are 0, and a NaN
otherwise. In math::complex<T> `jm` and `km` are NaNs in both parts, as no
complex number is a unit past `im`; `quat` of real arguments is w + xi
where y and z are 0, and a NaN in both parts otherwise; and a bound
quaternion is the complex number of its w and x where its jm and km parts
are 0. With real_results, `quat(w, x, y, z)` stands apart, as complex(x, y)
does (S340): a quaternion asked for by name is not a stray result.**
- Alternative: `quat` a NaN in every type: quat(2, 0, 0, 0), a real number
  written as a quaternion, no longer 2 there.
- Alternative: real_results refusing `quat` too: a context meant to keep
  results real refusing a quaternion asked for by name, where it allows
  complex(x, y).

> **Review.** Taken: for, every number in a real type real, as S271 rules
> for im, and quat's real or complex value where it has one, as complex(x,
> y)'s is (S332); a bound quaternion its nearest real or complex number
> where it is one; real_results leaving quat apart, as it leaves complex;
> against, a typed fold gives a NaN where a value is a quaternion, with no
> error to say why. The alternatives: quat a NaN in every type; real_results
> refusing a quaternion asked for by name. Recommended: as taken. Taken.

**S378. The owner's limit of 100 lines a function (2026.11.04) is kept by
every function chunk 45 wrote or changed: the value fold's application is a
function of its own, `expr_value_application`, which `expr_evaluate` calls;
a quaternion's power and the routing of a function past the real numbers are
functions of their own beside `expr_value_pow` and `expr_value_fn`; the
standard function table is built in three parts; `expr_matrix` fills its box
by `expr_matrix_fill_box`; two of the base's test functions it grew,
`t_expr_face.cpp`'s `d_tests_tables` and
`t_expr_complex_function_face.cpp`'s `d_tests_complex_fold`, are split; and
the new test's exact cases are a function of their own. The other functions
past the limit, 59 in the files chunk 45 changed, each the base's as it was,
and more in the lane's others, are left for a sweep of their own (section
6).**
- Alternative: every function of the lane's past the limit split now: a
  chunk's work of moves in headers this chunk does not otherwise touch.
- Alternative: the limit kept by new functions alone: those chunk 45 grew
  past it or further past it, `expr_evaluate` among them, longer still.

> **Review.** Taken: for, the owner's limit kept by every function the chunk
> wrote or changed, the five of the base's past it that the chunk changed,
> `expr_evaluate`, `expr_matrix` and the standard table among them, now
> within it; each split along a seam the function had, its behaviour
> unchanged, every test giving what it gave; against, 59 of the base's
> functions past the limit stay in the files the chunk changed, each as it
> was, more in the lane's others. The alternatives: every one split now, a
> chunk of moves; new functions alone within the limit. Recommended: as
> taken, the rest swept at step 35 (5g), the style debt, the lane's headers
> with the expression headers'. Taken.

**S379. The owner's rulings of 2026.11.03 and 2026.11.04 on the flagged
decisions (lane A's message of 11-04), each at the step the lane schedules,
are scheduled so: S336's and S340's, which take the alternatives their
reviews set aside - the plain forms in a complex type and the `_c` forms as
values, the docs saying where a value and `math::complex<T>` may differ;
and real_results an option each function's evaluator takes with the
context, every evaluator's signature changed - in chunk 46, before
matrices of quaternions, so that its evaluators are written once to S340's
signature; S338's and S341's docs in chunk 45, `function.hpp` saying how
`min` and `max` differ from C's `fmin` and `fmax` and where MATLAB takes
`asin`'s and `acos`'s cuts from the other side; S203's `gamma`, `lgamma`,
`erf` and `erfc` at step 36 (6a), their kernels with the correctly rounded
forms and their entries in the language with them; S251's declared domains
at step 25 (4a), as ruled, S376's variables among them; S190, S209 and S252
as Friday's agreement with lane F decides; and every other at the step its
review names, S15 at step 36.**
- Alternative: S336 and S340 in chunk 45: this chunk larger, and its three
  new entries written to the signature S340 then changes.
- Alternative: S336 and S340 at step 25 (4a), with the derivative's
  entries: the evaluators' signature changing after chunk 46's are written.

> **Review.** Taken: for, S336 and S340 made in the chunk after the one
> under way when they came, before chunk 46 writes evaluators that S340's
> signature would change, and apart from the quaternions' kind, so that each
> chunk's change is reviewed alone; the docs S338 and S341 ask for in this
> chunk, at no cost; S203's special functions placed with the correctly
> rounded forms, their kernels' method; against, chunk 45's three new
> entries change their signature again in chunk 46, and S336's two answers
> wait a chunk. The alternatives: both in chunk 45, a larger chunk; both at
> 4a, the signature changing after chunk 46. Recommended: as taken. Taken.

**S380. The checks: `t_expr_quaternion_face.cpp`, new, 71 checks: the kind,
exact arithmetic against the test's own quaternions of rationals at 3,000
random pairs, dense arithmetic at 1,500 pairs in each type `quat.h`'s bit
for bit, the functions and the rotation at 300 random quaternions in each
type, the rotation at 300 exact ones against the test's own rationals, the
language, the folds in each type, the renderer read back at 500 random
quaternions, simplification, the refusals, and every path no other check
reaches, so that each line chunk 45 wrote that a test can reach is run;
`t_cplx.c`'s whole powers against `math_tests_cplx_pow_refs.h`, 1,027 rows
by `refs_c45.py` from mpmath, at points where a part cancels, and their
zeros' signs at exact points; `t_expr_face.cpp` and
`t_expr_complex_function_face.cpp` checking `jm`, `km` and the table's 55;
the fault trial's 55 faults, each caught in every build that can reach it,
the four its first run missed closed by checks of their own; the full tier
of every face test and of the four harnesses that include `cplx.h`; the C
ladder on both trees; and chunk 44's other steps. Laying the base over the
owner's restructured tree, and the zip with its shims over it, wait for that
tree, which the lane does not have: `zip_check.txt` lays the zip over the
10.29 tree, the lane's old layout.**
- Alternative: chunk 44's queue as it stood, over the restructured tree:
  not to be had in this chunk.
- Alternative: the face tests' smoke tier alone: the language's headers
  changed under every one of them at every level unchecked.

> **Review.** Taken: for, every function and rule chunk 45 wrote checked
> against an oracle apart from it, the test's own quaternions of rationals
> and mpmath's references; each path of the kind, exact, dense in each type
> and of a boxed kind, under the fault trial; every face test at every level
> of the full tier, and every harness that includes `cplx.h`; against, the
> restructured tree's checks wait for that tree, and the zip is checked over
> the 10.29 tree, so the shims lane A makes for it are not exercised here.
> The alternatives: chunk 44's queue, not to be had; the smoke tier alone.
> Recommended: as taken, the restructured tree's steps run again when lane
> A's tree comes. Taken.

**S381. The delivery's lines end as the owner ruled on 2026.11.05: in CRLF
in every text file but those bash reads, which a carriage return would
break, in LF: the `.sh` files, and the two ladders' known-failures lists,
`ci/c_standards_known_failures.txt` and `ci/cpp_standards_known_failures.txt`,
which `check_c_standards.sh` and `check_cpp_standards.sh` read with grep
and comm. A file is text where it holds no NUL byte and reads as UTF-8.
The working tree keeps the 10.29 tree's LF; the lane's tools read either
ending, as Python's text files take CRLF as LF, and gcc and clang compile
either.**
- Alternative: the two lists in CRLF too, the ruling as written: each
  ladder then taking every listed failure for a new one, and every listed
  unit for one now passing.
- Alternative: the two scripts made to strip carriage returns: two of the
  owner's scripts changed for the sake of line endings.

> **Review.** Taken: for, the ruling's own reason, that bash refuses a
> carriage return, kept wherever bash reads a file, so that the delivered
> tree's ladders run as they do in the lane's; every other text file CRLF,
> as ruled; against, two files besides the `.sh` files in LF, an exception
> the ruling did not name, which the owner may prefer otherwise. The
> alternatives: the ladders broken by their lists' endings; two of the
> owner's scripts changed. Recommended: as taken, the two lists named in the
> note for the owner's word. Taken.

**S382. A quotient's zero part takes the sign the textbook formula gives
it, as `cplx.h` says of every operation and as `quat.h`'s quotient keeps
it: `d_cplx_<S>_div` in every family, `real_div` through it, and the whole
power's inverse divide the formula's zero by the squared modulus and keep
it as it is. S281's quotient took q less q n_lo / n for a zero too, which
makes a -0 +0 wherever the squared modulus's low part is not negative: 1 /
(-2 + 0i) was -0.5 + 0i, where the formula's (0 (-2) - 1 0) / 4 is -0. The
fault trial found it (step 8): the two faults planted in the whole power's
zeros escaped every build, and the checks written for them showed it.**
Three of the generator's templates change, the family's quotient, its wide
form and the kernel's, each returning a zero as it is; no part that is not 0
moves. `t_cplx.c` checks the zeros' signs of quotients exactly and of whole
powers at exact points against the family's own products and quotient: at 16
exact points, n from -6 to 6, in every family and both forms, no power off,
of 234 zero parts for n > 0 and 234 for n < 0, on the host and on the x87
(`raw/distances.txt`).
- Alternative: the quotient as it was, the docs saying where its zero is
  +0: a sign that follows how the squared modulus rounds.
- Alternative: every zero part of a quotient +0: one rule, but not the
  products' or `quat.h`'s, so that a complex number's quotient and a
  quaternion's of the same parts would differ.

> **Review.** Taken: for, the sign `cplx.h`'s text promises, the formula's,
> kept by the quotient as by every other operation and by `quat.h`'s
> quotient, so that a complex number's quotient and a quaternion's of the
> same parts agree; the whole power's inverse, which takes its zero as the
> division takes it, with it; a zero returned as it is in three templates,
> no part that is not 0 moved; against, a quotient's zero that was +0 is -0
> where the formula says so, a change a program that prints a quotient's
> parts may see, though no value compares otherwise. The alternatives: the
> old sign, documented, which follows the rounding of the squared modulus;
> +0 for every zero of a quotient, apart from the products' rule and
> `quat.h`'s. Recommended: as taken. Taken.

### S383 to S394: chunk 46, 2026.11.08

The record: `_handoffs/math98_chunk46_2026.11.08/README.md`.

**S383. Chunks 44 and 45 are laid into the owner's restructured tree, his
tree of 2026.11.07, by the lane itself, as he chose on 2026.11.07 ("Convert
44+45 here"), and the lane's C types are renamed `dj_` inside the same
conversion ("Inside this conversion"): `struct dj_cplx_f`, `dj_quat_d`,
`dj_math_imax` and the rest, as his tree names every other module's. From
this chunk the lane works in the restructured layout alone:
`inc/djinterp/c/modules/math/`, `inc/djinterp/cpp/modules/math/` with
`expr/` and `linear_algebra/` under it, the tests under
`tests/djinterp/c/modules/math/` and `tests/djinterp/cpp/modules/math/`,
the checks under `scripts/ci/`, the generator `scripts/gen_c_math_float.py`.
Each file the two chunks changed was merged three ways against the lane's
chunk 43, which his tree holds converted; the generator was converted and
makes the C core's generated headers as the tree holds them; no check of a
test changed but the names. The conversion's own edits: banners' tags and
paths as the tree's, the changed files' revised dates, columns realigned
where a name grew, lines past 80 broken, and `t_quat.c`'s
`d_tests_references` in two, each under his 100 lines.** The package he
asked for on 2026.11.07 and set aside the same evening ("Forget it, just
continue with the next chunk") is this chunk's zip instead, with chunk 46.
`t_interval_face`, which his tree does not build (`dj_math_umax` not
declared, lane C's report), builds and passes with it.
- Alternative: lane A converts, as it did chunks 41 to 43: the lane's tree
  and his a chunk apart again, and chunk 46 built over a layout he no
  longer has.
- Alternative: the rename left to a chunk of its own: every later chunk's
  merge crossing it.

> **Review.** Taken: for, the lane's tree and the owner's one tree again, in
> his layout and his names, so that chunk 46 and every later chunk is built
> and checked on the tree he has, with his own checks; the merge three ways
> against the lane's chunk 43, which his tree holds, so that his edits to
> the lane's files since are kept; no check changed, which the smoke tier's
> lines show, byte for byte chunk 45's; against, a conversion the lane had
> not made before, a hundred and more files touched in one step, and the
> records before it, with their tools, still name the old paths, so that an
> old record's tool runs on the new tree only through `refs_paths_c46.py` or
> by hand. The alternatives: lane A converting, a chunk behind again; the
> rename apart, every later merge crossing it. Recommended: as taken, the
> owner's own choice on 2026.11.07. Ruled 2026.11.07 (the owner's): convert
> here, the rename inside.

**S384. The owner's ruling S336 is made as he ruled it: `math::complex<T>`'s
functions by their own names, `abs`, `arg`, `exp`, `log`, `sqrt`, `pow`
and the rest, are `cplx.h`'s plain forms at run time, on the C library's
real functions, and its `_c` forms in a constant expression from C++20,
where `std::is_constant_evaluated` tells the two apart; the names ending
`_c`, `exp_c` and the rest, new, are the `_c` forms at every level,
constant expressions from C++14 as the functions were. The language's
values keep the `_c` forms, one value on every build; a fold in
`math::complex<T>` takes the plain ones, as a fold in a real type takes the
C library's. So a value and a fold in a complex type may differ, each
within its form's bound, as the README says where it describes both.**
`t_complex_face.cpp` holds each function by its name to the plain form and
by its `_c` name to the `_c` form, bit for bit, and counts the points where
the two differ. With clang on the x87 the plain forms' bits in float and
double are not held to, as S37 says of every float and double kernel there:
one function gives one argument other bits from one call's place to
another's.
- Alternative: the `_c` forms by the plain names from C++14 as constant
  expressions, the plain forms by names of their own: the ruling's
  two answers, but the fast form no longer the default.
- Alternative: below C++20 the names not constant expressions at all, so
  that a constant expression never gives other bits than the run time:
  C++14's and C++17's constant expressions of them lost. As made, below
  C++20 a name is not a constant expression and its `_c` name is.

> **Review.** Ruled: for, the ruling as the owner gave it: the C library's
> speed by the names a caller reaches for, and the `_c` forms, one answer on
> every build, by names of their own and in constant expressions;
> `std::is_constant_evaluated` keeps a C++20 constant expression the `_c`
> form's, so that a constant never depends on the library; against, two
> answers for one computation, a value's and `math::complex<T>`'s, each
> within its bound, which the ruling accepted; below C++20 a function by its
> own name is no constant expression, where chunk 39's was from C++14, a
> program that used one so must take the `_c` name; and the plain forms'
> bits vary with the C library and, with clang on the x87, with the call's
> place (S37). The alternatives: the `_c` forms by the plain names, the
> default then the slow form; no constant expressions below C++20 under
> either name. Recommended: as ruled and as made, the `_c` names keeping
> every constant expression chunk 39 gave. Ruled 2026.11.04 (S336); made as
> ruled.

**S385. The owner's ruling S340 is made as he ruled it: every evaluator of
the function table takes the fold's context, `Type (*)(const Type*,
std::size_t, const expr_context&)`, the caller's tables' evaluators among
them, and `expr_context` moves to `expr_common.hpp` so that the table can
name it. Each value evaluator that can give a complex answer of real
arguments holds its own answer real where the context asks
(`expr_real_only`), a domain error whatever the operands' exactness; the
fold no longer judges an answer after the call; the typed folds take a
context too, a NaN in a real type and, under `real_results`, a NaN in a
complex type where the arguments are real and the answer is not. A matrix
holding a quaternion counts as not real, as one holding a complex number
does. S272's drawback, the option on some evaluators and not others, is
gone with it.** Every evaluator's signature changes: a caller's own table
must add the parameter, a compile error until it does, as the ruling
accepted.
- Alternative: the old signature kept beside the new, a table taking
  either: two kinds of evaluator for good, to spare one edit.
- Alternative: the context reached through a thread's variable: no
  signature changed, and an evaluator's answer no longer a function of its
  arguments.

> **Review.** Ruled: for, the ruling as the owner gave it: one rule, every
> evaluator answering for its own result under the option, a function of its
> arguments and the context alone; the fold's judgment after the call, which
> could not see why an answer was complex, gone; the typed folds honoring
> the option too; S272's uneven option even; against, every evaluator's
> signature changed, a caller's own table a compile error until its
> evaluators take the parameter, which the ruling accepted, and one more
> argument passed on every call. The alternatives: both signatures for good;
> the context in a thread's variable. Recommended: as ruled and as made.
> Ruled 2026.11.04 (S340); made as ruled.

**S386. A matrix holds a quaternion as it holds a complex number (S277,
S243): densely, as `math::quaternion<T>` of the widest floating-point type
among its elements' parts, where every element is a float, a double, a long
double, or a complex number or a quaternion of such parts, three storages
after the complex ones, a real element x then x + 0im + 0jm + 0km and a
complex one x + y im + 0jm + 0km; as values, each element of its own kind,
where any is exact or of a boxed kind. A dense storage of quaternions wins
over a complex one of the same type, and a wider type over a narrower.
`expr_matrix` answers no element `not_yet`; no node of the language does
(S369's are all values now).**
- Alternative: every matrix holding a quaternion as values: no new
  storage, and each element's arithmetic through the value's tables, some
  tens of times slower for the matrices a rotation's user has.
- Alternative: a complex adjoint stored, 2n by 2n complex: the complex
  kernels reused, twice the memory, and no element's bits `quat.h`'s.

> **Review.** Taken: for, S277 as ruled and S243's pattern kept: a dense
> matrix of quaternions is contiguous `math::quaternion<T>`, `quat.h`'s
> array, its elements' bits the kind's; real and complex elements widen as
> the kind widens them, with +0 parts; exact matrices stay exact as values;
> against, three more storages for every function of the matrix kind to
> dispatch on, some 2,200 lines of `expr/matrix.hpp`, which is past 8,000
> now; a matrix of quaternions and real numbers stores each real one as four
> parts. The alternatives: values always, tens of times slower dense; a
> stored adjoint, twice the memory and no element's bits `quat.h`'s.
> Recommended: as taken; `expr/matrix.hpp` split by concern at the next step
> that touches it, which 4a's rewrite of the parser layer is not; minor.
> Taken.

**S387. A matrix of quaternions computes with the quaternion kind's own
arithmetic, element by element: `expr_quaternion_operand`, a quaternion and
the parts it holds, 1, 2 or 4, whose operators are the kind's dense
arithmetic, a real or complex operand taken as itself, as the kind takes it
(S372), so that each element of a sum, of a product's sum or of a scaling
is the kind's own operation on its two, bit for bit. The generic kernels
compute on these operands, their traits saying they do not commute (S269):
a product's element sums a_ik b_kj, in that order, from 0; `q * A` and `A *
q` are two matrices, `A / q` is `A inv(q)`, and `q / A` a domain error as
every number over a matrix is; `-A` negates each part; `A'` conjugates each
element and `transpose(A)` moves it; `A ^ n` is the product's power and
`A ^ 0` the identity of A's storage. Exact matrices compute as values, each
product in its order.** The kind's own dense binary operations were
rewritten on the same operands, bit for bit what they were
(`t_expr_quaternion_face.cpp`, unchanged, 71 of 71). Found with it: `A ^ 0`
of a dense complex matrix was the exact identity, not complex; it is the
identity of A's storage now.
- Alternative: every element a full quaternion in the kernels, a real one
  w + 0im + 0jm + 0km: simpler, but 2 * q then q's parts times 2 and a +0
  or -0 from the products with 0, other bits than the kind's own 2 * q.
- Alternative: `quat.h`'s Hamilton product on the adjoint's complex
  blocks: no element's bits the kind's.

> **Review.** Taken: for, one arithmetic for a quaternion wherever it
> stands: an element's sum, product or quotient is the kind's own, bit for
> bit, in a matrix or alone, a real or complex operand as itself, so that 2
> * A is each element doubled and nothing else; the generic kernels, already
> proved for values and complex numbers, keep each product's order (S269);
> the kind's own operations rewritten on the same operands with no bit
> changed; `A ^ 0` of a dense complex matrix found and fixed with it;
> against, an operand carries the count of its parts beside them, a branch
> in each product, some tens of percent slower than a bare Hamilton product;
> a matrix product is the kernels' triple loop, no blocking. The
> alternatives: every element a full quaternion, other bits than the kind's
> for a real or complex element; the adjoint's complex products, no
> element's bits the kind's. Recommended: as taken; speed with step 40's
> work, measured first. Taken.

**S388. The square algebra of a matrix of quaternions is the generic
kernels' on the kind's operands, each product in its order and each row
divided on the left, as A X = B asks (S269): `inv` by Gauss-Jordan with a
pivot of largest |w| + |x| + |y| + |z|, `solve(A, B)` by elimination of
[A | B], a singular matrix a pole; exact matrices on values whose traits
say they do not commute (`expr_matrix_skew`), so that an exact inverse is
exact; a matrix of values holding an inexact element in its widest type's
quaternions. `det` of a matrix holding a quaternion is a domain error: no
determinant of quaternion matrices is both multiplicative and the complex
one on complex matrices (Dieudonne's is real, Study's the adjoint's, a
square), and the language has one `det`. `trace` is the diagonal's sum;
`norm` the Frobenius norm of the moduli, scaled as every norm is; `dot(a,
b)` sums conj(a_i) b_i, in that order, from 0, as the complex one
conjugates its first (S273); `cross` conjugates nothing, each product in
its order.**
- Alternative: `det` as Study's determinant, det of the complex adjoint, or
  Dieudonne's, its root: each a real number that is not the complex
  determinant on a complex matrix, under the name that is.
- Alternative: `inv` and `solve` through the complex adjoint's LU: the
  complex kernels reused, and an exact matrix's inverse no longer exact
  without a path of its own.

> **Review.** Taken: for, A X = B solved as it is written, every product in
> its order and rows divided on the left, the one consistent convention with
> S269's kernels; exact matrices exact, by values that say they do not
> commute; a singular matrix a pole as every other is; `det` refused and not
> given a meaning the name does not have on complex matrices, where every
> system the language is measured against that has quaternion matrices
> either has no determinant of them or names Dieudonne's and Study's apart;
> `dot` conjugating its first as the complex one does; against, no
> determinant at all of a quaternion matrix, where a user may want Study's,
> a function of its own name a later step could give; Gauss-Jordan with a
> pivot by the sum of the parts' magnitudes, not the modulus, within a
> factor of 2 of it, as the complex kernel's is. The alternatives: `det` as
> Study's or Dieudonne's under the complex determinant's name; the adjoint's
> LU, an exact inverse lost. Recommended: as taken. Taken.

**S389. `eig` of a matrix of quaternions gives Zhang's standard right
eigenvalues: the n complex numbers of imaginary part 0 or more, one from
each class of similar right eigenvalues, as a 1 by n row, by real part and
then imaginary. They are those of the complex adjoint [A1, A2; -conj(A2),
conj(A1)], A = A1 + A2 jm, 2n by 2n, found as a complex matrix's are, in
A's type or exactly (S352, S353), which come in conjugate pairs; one of
each pair is given, the pairs matched nearest first in the plane folded
onto its upper half, the member of the larger imaginary part taken. A
triangular matrix's are its diagonal's, each element's own, w + |v| i,
exactly where |v| is rational, since rounding would part a repeated pair. A
Hermitian matrix's are real. A domain error where the adjoint's are not
each found, NaNs where A holds one.** Checked against references mpmath
computes at 50 digits, ten matrices (`math_tests_expr_qmatrix_refs.h`,
`refs_c46.py`): exact matrices within 2^-56 of each, double within 2^-44,
float within 2^-16, relative to the spectrum's largest.
- Alternative: left eigenvalues, A x = lambda x: no finite method finds
  them past n = 2, and they are no invariants of similarity.
- Alternative: every right eigenvalue's class given as a quaternion: an
  answer of infinitely many members written as one of them, where the
  standard one is the member every text picks.
- Alternative: a quaternion QR on A itself, no adjoint: half the work at
  large n, a kernel of its own to prove; the adjoint's is the complex
  path's, already bounded. Worth taking up with step 40's speed work.

> **Review.** Taken: for, the eigenvalues every text on quaternion matrices
> means, the invariants of similarity, complex numbers a caller can compare
> and sort, found by the complex path already bounded (S352, S353), exactly
> where the adjoint's are Gaussian rationals; a triangular matrix's exact, a
> repeated pair never parted by rounding; references from mpmath at 50
> digits held to the complex path's own bounds; against, the adjoint is 2n
> by 2n, so the work is some 8 times a complex matrix's of A's size and its
> memory 4 times; the pairing of the adjoint's 2n eigenvalues into n is by
> nearness, which a cluster of nearly equal eigenvalues may match across
> pairs, each answer then still within the cluster's width; no eigenvectors,
> as for complex matrices. The alternatives: left eigenvalues, which no
> finite method finds past n = 2; a class written as one quaternion; a
> quaternion QR, half the work, a kernel to prove. Recommended: as taken; a
> quaternion QR at step 40 (6e) where measured demand asks for one. Taken.

**S390. A quaternion is a scalar to linear algebra's functions, as a real
or complex number is (S356): `det(q)` a domain error, as a matrix's is
(S388); `inv(q)` 1 / q; `trace(q)` q; `norm(q)` |q|; `eig(q)` its standard
eigenvalue w + |v| i, exact where |v| is rational and otherwise in q's type,
`quat.h`'s norm of v; `dot(p, q)` conj(p) q; `transpose(q)` q and `q'`
conj(q); `solve(a, B)` of a number a is B / a where a is real or nothing
holds a quaternion, and otherwise a^-1 B, on the left, `quat.h`'s left
quotient where dense.**
- Alternative: `eig(q)` q itself, as `eig` of a real or complex number is:
  not a complex number, where every other eigenvalue the language gives of
  a quaternion matrix is, and other than `eig([q])`.
- Alternative: `det(q)` |q| or |q|^2, the 1 by 1 Dieudonne and Study
  determinants: S388's reason again.

> **Review.** Taken: for, every function of linear algebra answering a
> quaternion as it answers a 1 by 1 matrix of one, so that `eig(q)` and
> `eig([q])` agree, a complex number as every eigenvalue of a quaternion
> matrix is; `solve(q, B)` on the left, as `solve(A, B)` is; no node
> `not_yet`; against, `eig(q)` is not q, where `eig(z)` of a complex z is z,
> a difference a reader must learn, which the docs give with its reason: q's
> class has one complex member of imaginary part 0 or more, and that is the
> answer. The alternatives: `eig(q)` q itself, other than `eig([q])`;
> `det(q)` a modulus, S388's reason against it. Recommended: as taken.
> Taken.

**S391. `expr/rotation.hpp`, new, holds a quaternion's functions that give
or take a vector or a matrix, so that the matrix kind, which holds
quaternions now, is read after the quaternion kind: `parts` and `rotate`
move there from `quaternion.hpp`, unchanged, and three functions come to
the language, 58 in its table. `rotmat(q)` is the 3 by 3 matrix of the
rotation q names, R v = q v q^-1 for any q but 0: exact where q is, R |q|^2
over |q|^2, and otherwise `quat.h`'s `to_matrix3`. `rotquat(R)` is the unit
quaternion of a rotation's matrix, w >= 0, by Shepperd's method as `quat.h`
computes it, w's candidate first among equals: exact where R is and the
root is rational, otherwise `quat.h`'s `from_matrix3`; of a 4 by 4 matrix,
its upper left; R is taken for a rotation's and not tested for one, as
`quat.h` takes it. `slerp(p, q, t)` is the shorter arc between the
rotations p and q name, `quat.h`'s accurate slerp of the two normalized,
in the widest type among the three, long double where all are exact. An
exact 0 names no rotation, a domain error; a dense 0 gives NaNs; a t that
is not real, a matrix where a quaternion is asked, or a quaternion where a
matrix is, a domain error; in a typed fold each is a NaN, as `parts` is.**
- Alternative: MATLAB's names, `rotmat` and `quaternion(R, 'rotmat',
  'point')`, or its Robotics toolbox's `quat2rotm` and `rotm2quat`: the
  second pair says more, in a language whose other names (`quat`, `parts`,
  `rotate`) are single words.
- Alternative: `rotquat` testing R for a rotation's matrix within a
  tolerance, a domain error otherwise: a tolerance to choose, and an
  answer refused for a matrix a product of rotations has drifted to.
- Alternative: `slerp` exact where its result is, t = 0, 1 or 1/2 of
  suitable p and q: three special cases of a function whose answers are
  otherwise never exact.

> **Review.** Taken: for, the three functions S369 promised, each `quat.h`'s
> kernel bit for bit where dense and exact where the answer is,
> `rotquat(rotmat(q))` giving q's unit quaternion exactly for a q of
> rational modulus; the kinds read in an order with no cycle; R v = q v q^-1
> the one convention, `rotate`'s and MATLAB's 'point'; against, the names:
> `rotmat` is MATLAB's, `rotquat` is the lane's own, made to pair with it,
> where MATLAB's Robotics toolbox says `quat2rotm` and `rotm2quat` and SciPy
> `as_matrix` and `from_matrix`, so a user arriving from either must look
> them up; `rotquat` does not test its matrix for a rotation's, so a matrix
> that is none gives a quaternion that means nothing, silently, as
> `quat.h`'s kernel does; `slerp` is never exact. The alternatives:
> `quat2rotm` and `rotm2quat`; `rotquat` refusing within a tolerance;
> `slerp` exact at three special points. Recommended: as taken, the names
> for the owner's word before 0.8 fixes them. Ruled 2026.11.16: `rotmat` and
> `rotquat` kept, the docs listing MATLAB's and SciPy's equivalents, as
> recommended (9a, A); `rotquat` the quaternion of the nearest true
> rotation, Bar-Itzhack's, an error for a reflection or a matrix far from
> any rotation, an exact matrix that is exactly a rotation keeping its exact
> answer (9b, C); both before 0.8. Done in chunk 53 (S429): the docs list
> both, and `rotquat` gives the nearest rotation's quaternion, a domain
> error for a reflection or a matrix farther than 1/2 from any rotation.

**S392. `linalg::dynamic_matrix<T>` takes every element type the fixed-size
matrix takes, each that `number_traits` specializes (S354): float, double
and long double on the C core's kernels, any other arithmetic type through
double, and `math::complex<T>`, `math::value` and `math::inline_value` on
the generic path, each zero, one and tolerance the traits', each product in
its order. The Frobenius norm of a complex one is a T, the C core's scaled
norm of the parts, as the fixed-size matrix's is; a tolerance is of the
traits' magnitude type; a type without traits is refused at compile time.
Quaternions it does not take, where S354's review said all at once:
neither face's quaternion has sums or traits, and the fixed-size matrix
does not take them either; the language's matrices of quaternions compute
with the kind's own operands (S387).**
- Alternative: `math::quaternion<T>` given sums and `number_traits`, both
  faces' matrices then taking it: the face's quaternion made a number
  type, with a magnitude and a pivot rule to choose, for a matrix no user
  has asked of the faces; the language has it.
- Alternative: `dynamic_matrix` alone taking the language's operand type:
  an internal type of the language in the face.

> **Review.** Taken: for, the face's runtime-sized matrix of complex
> numbers, Eigen's `MatrixXcd`, the gap S354's review named, closed, and of
> values and inline values with it, each on the generic path the fixed-size
> matrix proved, the two faces' matrices now taking the same element types;
> a type without traits refused at compile time, not at link time or run
> time; against, S354's review said complex numbers, values and quaternions
> at once, and quaternions are not taken: the face's `math::quaternion<T>`
> has products and no sums or traits, so neither matrix of the faces can
> hold one, and a user who wants a matrix of quaternions has the language's
> and no C++ type. The alternatives: `math::quaternion<T>` made a number
> type, sums, traits, a magnitude and a pivot rule, both matrices then
> taking it; the language's operand type in the face. Recommended: the first
> alternative, with step 40 (6e) or sooner on demand, both faces' matrices
> at once; as taken until then. **Flag:** not the earlier review's
> recommendation in full: neither face's matrix holds quaternions; the
> language's does.

**S393. Chunk 46's checks are the restructured tree's own, the owner's
tools where he has one: `scripts/ci/check_math_tests.sh` for the tiers,
`scripts/ci/ladder.py` and `check_style.py` against their known lists in
place of the lane's ladders and the consolidation's style check, the
generator's `--check`; and the record's tools for the rest, each chunk
45's rewritten for the layout. The smoke tier runs on the snapshot and on
his tree, and each line is compared with chunk 45's run, byte for byte
where chunk 46 changed no check: the conversion's proof that it changed
none. The full tier runs every face test and every C harness, the C core
renamed under each. References made before the restructure are made again
by their tools and compared with the tree's, line for line, the banner's
tag and date and the realigned columns apart (`refs_paths_c46.py`).
`t_complex_face.cpp` does not hold the plain forms' bits in float or
double with clang on the x87 (S384, S37).**
- Alternative: the lane's own ladders and style check kept beside his:
  two verdicts on one tree.
- Alternative: the smoke tier alone after a conversion that changed no
  check: the rename under every harness at every level unchecked.

> **Review.** Taken: for, one verdict on one tree, the owner's tools', which
> his CI runs; the conversion proved by the smoke tier's lines against chunk
> 45's, byte for byte, and by every harness's full tier under the rename;
> old references remade by their own tools and compared line for line; the
> plain forms held to their bits wherever a compiler keeps them; against,
> the lane's own ladders, which built every unit at every level, are retired
> for his `ladder.py` and its known list, whose coverage is his to keep;
> clang's x87 is not held to the plain forms' bits in float and double
> (S37), a build the tier runs. The alternatives: both sets of checks, two
> verdicts; the smoke tier alone after the rename. Recommended: as taken.
> Taken.

**S394. The delivery is one cumulative zip over the owner's tree of
2026.11.07, the restructured layout: every file of the lane's working tree
that his tree has not or holds otherwise, the records with it, the note,
`MANIFEST.md` and `HANDOFF.md` beside its `djinterp/` folder, `decisions.md`
and `zip_check.txt` apart. It supersedes chunk 45's zip, which was over his
10.29 tree in the old layout, and needs no earlier one. Lines end in CRLF
but in the `.sh` files, LF, as bash asks (S381, his ruling of 2026.11.05);
`scripts/ci/`'s known lists, which his Python tools read, are CRLF as his
tree has them, where the old layout's two, read by grep and comm, were LF.
The lane commits nothing: the zip is files to copy.**
- Alternative: a zip of chunk 46's changed files alone: smaller, and not
  enough over his tree, which has chunks 41 to 43 and not 44 or 45.
- Alternative: the zip over his tree of 2026.10.27 as before: the old
  layout, which the lane has left (S383).

> **Review.** Taken: for, one zip over the tree the owner has, in his
> layout, enough alone; the records with it, which `decisions.md` and the
> record tools read; the endings as he ruled and as his tree has them;
> nothing committed, as he asked; against, the zip repeats every lane file
> his tree lacks or holds otherwise, the records among them, tens of
> megabytes for a chunk that changed 21 files; a tree older than 2026.11.07
> is not served. The alternatives: chunk 46's files alone, not enough over
> his tree; the old layout's base. Recommended: as taken. Taken.

### S395 and S396: chunk 47, 2026.11.13

The record: `_handoffs/math98_chunk47_2026.11.13/README.md`.

**S395. A term is compiled once for a type, the first of the evaluation's
fixes the owner chose on 2026.11.12 ("Speed, no API change"):
`expr_compile<T>(term)`, or `expr_compile<T>(term, names)`, gives an
`expr_program<T>`, in the new `expr/program.hpp`, for float, double or long
double or the `math::complex` of one, and the handle's `compile<T>()` gives
one too. Its steps are the typed fold's, in the order the fold computes,
each operand before its operation, left to right: a literal rounded once to
T, a constant's value in T, a variable's slot, an operator of the table's,
a function's evaluator, indexing's NaN. A run does them on a stack whose
greatest depth the compiler counted, in the caller's frame up to
`D_CFG_MATH_EXPR_PROGRAM_STACK` (32), the language's first knob, in
`config/cpp/modules/math/expr/cfg_expr.h`, which `dconfig.h` includes, and
on the heap past it. A run's answer is the fold's, bit for bit, and its
error the fold's first: the term's shape's when it is compiled, an unbound
variable's at a run of bindings, the first read in the fold's order. A run
takes T's own numbers in slot order, `math::value`s, a vector of them or
bindings, and a context, which each evaluator is given (S340). For it the
complex fold's operations take their operands where they lie, in an array
of the frame up to eight (`expr_operands`), where each built up to three
vectors; the fold is faster with it, its answers unchanged. No constant is
folded, and the value fold is not compiled.** Measured
(`bench_program_c47.cpp`, the median of seven timings, five terms of 8 to
40 steps): a run of doubles takes 2.9 to 11.4 times the same term written
in C++, where the typed fold takes 79 to 624 times, so a run is 26 to 55
times sooner than a fold; a run of bindings, which looks each name up,
takes 1.5 to 3.1 times a run of doubles; a run in `math::complex<double>`
is 8.1 to 15.2 times sooner than that fold; a compile, 992 to 5,874 ns,
pays for itself at the second run.
- Alternative: constants folded when the term is compiled, `2 * pi` one
  step of `2 * pi * x`: fewer steps, and a folded constant computed under
  the compiler's rounding mode and context, not the run's, so that a run
  would no longer be the fold wherever either differs. A literal is rounded
  from its text in any mode (`decimal.h`), so literals are made at compile
  time.
- Alternative: machine code, through LLVM as SymEngine's `lambdify` does,
  or a JIT: several times faster again on a long term; a dependency the
  framework has no other like, code generated for each platform, and none
  at all in a constant expression.
- Alternative: a tree of nodes specialized to each operation, ExprTk's
  design: a call through a pointer at every node, which the flat steps
  already make, and a node allocated for each.
- Alternative: the value fold compiled too: a value's arithmetic allocates
  and its errors are values, so a program of values would save the lookups
  alone; S396 makes the value's arithmetic itself faster instead.
- Alternative: a run of an array of `math::complex<T>`: a complex number
  cannot say it is real, and the fold's answers turn on it (S339), so a run
  in a complex type takes `math::value`s or bindings.

> **Review.** Taken: for, the 130 times a fold cost, as the evaluation
> measured it, cut to a few times the term written in C++, with nothing
> given up: each run's answer the fold's bit for bit and its error the
> fold's first, in all six types the fold takes, proved on 2,000 random
> terms in each against the fold itself and on 500,000 points of five terms
> by the benchmark; a compile pays for itself at the second run; a run of a
> real type allocates nothing, nor, with the complex fold's operations in
> place, a run of a complex one; the API only grows, as the owner chose.
> Against: a second way to evaluate a term in a type, `eval` once and
> `compile` for many, which a reader must learn; a run of bindings looks
> each name up, several times a run of numbers; past 32 levels of operands a
> run allocates; still no machine code, several times slower than
> SymEngine's LLVM back end would be on a long term; the names
> `expr_compile`, `expr_program` and `expr_variables` are prefixed, and will
> move with `math::expr::`. The alternatives: constants folded; machine
> code; a tree of specialized nodes; the value fold compiled; a run of
> `math::complex<T>`s. Recommended: as taken.

**S396. The value's binary arithmetic, the second of the speed fixes, sends
two inline numbers to their arithmetic before the checks that cannot hold
for them: two integers whose sum, difference or product a test of the
operands shows fits the word, at once, a product where each factor's
magnitude is below 2^(w/2 - 1); two exact numbers to the exact arithmetic;
two numbers of one inexact kind to that kind's arithmetic, past the checks
for errors, boxes, booleans and none. The floating-point tail is one helper,
which both paths call. `math::value`'s move assignment moves the value once,
where it moved it into a temporary and swapped. Each result is the general
path's, field by field.** `value_fast_c47.sh` compares chunk 46's `value.h`
with chunk 47's on 2,214,884 pairs in each of eight builds, and none
differs; the two faults planted in the shortcut, a product bound too large
and flags dropped, it catches. `t_value.c` gains the operands at 2^(w/2 - 1)
and three flagged values, each checked against its exact reference. Measured
(`bench_value_c47.sh`, chunk 46's header and chunk 47's built alike, the
median of three rounds): `math::value`'s sum of two doubles from 39.2 to
13.0 ns, 75 times a double's sum to 25; a product from 39.0 to 13.3; a sum
of two integers from 28.7 to 21.0; of two rationals, which no shortcut
takes, from 60.9 to 57.8.
- Alternative: the shortcut in the C++ face alone, `value.hpp` testing the
  kinds before it calls the C core: C callers left the long path, and the
  dispatch in two places.
- Alternative: a smaller value, a union of the fields where 64 bytes hold
  them all: copies halved; constant evaluation could not switch a union's
  member before C++20 (`value.h`'s banner), and every C caller's layout
  would change.
- Alternative: every integer product that fits taken at once, by the two
  words of `d_internal_rational_mul_wide`: two words multiplied for each
  product, where most factors are small; a product of large factors that
  fits takes the exact arithmetic, as before.

> **Review.** Taken: for, `math::value`'s arithmetic on the numbers it most
> often holds several times faster, in C and C++ alike, with every result
> the general path's field by field, proved by `value_fast_c47.sh` against
> chunk 46's header on two million pairs in eight builds, and by `t_value.c`
> against its exact reference with operands at the shortcut's bound; the
> faults a shortcut invites, a bound too large and flags lost, caught by the
> comparison. Against: three paths to read where one was, though each ends
> in the function the general path calls; an integer product of large
> factors that fits still takes the long way; the value stays 64 bytes, so
> copies still cost what they did. The alternatives: the shortcut in the C++
> face alone; a union; every product that fits by a wide multiply.
> Recommended: as taken.

### S397 to S403: chunk 48, 2026.11.14

The record: `_handoffs/math98_chunk48_2026.11.14/README.md`.

**S397. The run-time language keeps its names, flat in `djinterp::math`
with the `expr_` prefix, as S179 set them; the namespace `math::expr::`
the owner chose on 2026.11.12 is withdrawn at his word of 2026.11.14,
"flat in djinterp::math", which left the prefix to the lane's
recommendation "for fewer/no namespace levels below math".** A class and a
namespace cannot share a name in one scope, so `math::expr::` would have
moved the handle, `math::expr`; flat, the handle stays where it is. Without
the prefix eleven of the language's 91 public names would clash with
names `math` already has (`complex`, `quaternion` and `function`, class
templates or a function template of them; `constant`, `derivative`,
`gradient`, `sum` and `integrate`, the compile-time language's; `norm` and
`dot`, the quaternion's; `parse`, the handle's own), and nine would hide
`djinterp`'s inside `math` (`evaluate`, `render`, `substitute`, `atom_of`,
`sum`, `product`, `constant`, `parse` and `result`), found by a compiler
probe of every name with every math header included. Lane F was told on
2026.11.14 that `expr_op`, `expr_atom` and `expr_term` are final.
- Alternative: the prefix dropped, `math::term`, `math::evaluate`,
  `math::compile`: shorter names; the eleven clashes renamed each its own
  way (`complex_value`, `quaternion_value`, `named_function`,
  `constant_term`, `parse_term`, `result<T, error>` for `expr_result<T>`),
  so that `number()` would stand beside `constant_term()`; math's own calls
  of `djinterp`'s names qualified; and a user's `using namespace` of both
  namespaces ambiguous for `evaluate`, `render`, `substitute`, `sum` and
  `parse`.
- Alternative: `math::expr::`, the handle `math::expr::expression`: the
  names grouped, a level below `math`, which the owner ruled out.

> **Review.** Taken: for, the owner's word of 2026.11.14, flat in
> `djinterp::math`, kept to the letter, and the rest left to the lane's
> recommendation, "fewer or no namespace levels below math", met with none;
> the handle `math::expr` where it is; no name of the language's clashing
> with one of `math`'s or hiding one of `djinterp`'s, eleven and nine that
> the prefix's removal would bring, found by a compiler probe with every
> header read; lane F building on names that do not move, as it was told the
> same day; no break for any caller. Against: the prefix stays, `expr_` on
> 91 names, longer to write than a namespace's names would be inside it; the
> evaluation's third group, an API break before 0.8 or never, is now never,
> and with it the shorter spelling. The alternatives: the prefix dropped,
> eleven names renamed each its own way; `math::expr::`, which the owner
> ruled out. Recommended: as taken.

**S398. The framework has one quaternion: `math::quaternion<T>`, in
`quaternion.hpp`, takes linear algebra's names, its members `m_x`, `m_y`,
`m_z` and `m_w`, the default element type `double`, `identity()`,
`vector_part()`, `to_matrix3` and `to_matrix3_fast` for `to_matrix` and
`to_matrix_fast`, and its `from_axis_angle`, the C core's
`d_quat_from_axis_angle`; and `linear_algebra/quaternion.hpp`, which held a
second class over the same `quat.h`, names the one class and each of its
functions in `linalg` by using-declarations, so that
`linalg::from_axis_angle` and `math::from_axis_angle` are one function and
no call is ambiguous.** The second of the evaluation's groups of fixes the
owner chose on 2026.11.12, "one header per concept". Two classes made two of
every function, and a call such as `from_matrix(m)` of a `linalg::matrix`,
its own namespace searched by argument, found both and was ambiguous
wherever both headers were read, as in a translation unit of every header
(`expr/rotation.hpp`'s). The render module's code, which uses
`linalg::quaternion`, its `m_` members and its functions' names, compiles
and passes unchanged: each name it uses stands. What changes for
`math::quaternion`'s callers: its members are `m_x`, `m_y`, `m_z` and `m_w`,
as the C++ style guide names members, where they were `x`, `y`, `z` and
`w`; `to_matrix` is `to_matrix3`; and `from_axis_angle`, which normalized
the axis in the face and then took `d_quat_from_unit_axis_angle`, scales
once, axis sin(a/2) / |axis|, as `linalg`'s did, so that its parts may
differ from before in the last bit, each within a step, and a zero axis,
which named no rotation, gives the identity where it gave (0, 0, 0,
cos(a/2)), a quaternion that rotates nothing only by being divided by its
norm.
- Alternative: two classes, each call that is ambiguous written out by its
  namespace: no caller changes; two of every function to keep in step, the
  ambiguity left for every user who includes both and calls by argument,
  and two names for one concept, which the evaluation found.
- Alternative: the one class in `linalg`, `math` naming it: `linalg`'s
  callers untouched as now; the language's kinds, the complex face's
  neighbour and `math.hpp`'s entry point would reach a quaternion through
  linear algebra's namespace, where the module's numbers, `math::value` and
  `math::complex<T>`, stand in `math`.
- Alternative: `linear_algebra/quaternion.hpp` removed, its
  using-declarations in `quaternion.hpp`: no second header of the name;
  the render module's include of it, outside the lane's paths, broken.
- Alternative: `math`'s members `x`, `y`, `z` and `w` kept and `linalg`'s
  callers changed: no change for `math::quaternion`'s callers, the render
  module's code edited outside the lane's paths, and public members against
  the style guide's `m_`.

> **Review.** Taken: for, one quaternion where there were two, one of every
> function, and no call ambiguous by its argument's namespace, as a
> translation unit of every header found; the render module, the one user of
> `linalg`'s class outside math, compiling and passing unchanged, each name
> it uses standing; the members named as the style guide names them;
> `from_axis_angle` the C core's in both names, the axis scaled once and a
> zero axis the identity, a rotation, where `math`'s gave a quaternion that
> rotated nothing only once normalized. Against: `math::quaternion`'s
> callers change, its members `x`..`w` now `m_x`..`m_w`, `to_matrix` now
> `to_matrix3`, and `from_axis_angle`'s parts may differ in the last bit and
> are the identity for a zero axis; within the framework those callers are
> the lane's own, changed with it, and a caller outside the framework would
> change too, a break before 0.8, as the owner prefers breaks to be. The
> alternatives: two classes, the ambiguous calls written out; the one class
> in `linalg`; `math`'s members kept; the `linalg` header removed.
> Recommended: as taken.

**S399. `math.hpp` includes every header of the module, `expr/expr.hpp`,
`linear_algebra/linalg.hpp`, `linear_algebra/quaternion.hpp`,
`quaternion.hpp`, `functional.hpp` and `numeric_compare.hpp` among them, and
its banner begins with the entry points, where to start for each part; and
`geometry/geometry.hpp` includes `platonic.hpp`, the one geometry header it
did not.** A header that says it includes everything now does: before, a
user of `math.hpp` had no run-time language, no quaternion and no linear
algebra beyond what other headers happened to read. `math.hpp` compiles
from C++14, the compile-time core's floor, as it did; `expr/` is empty
below C++11. With every header read, a probe of `math.hpp` with `-Wshadow`
found the lane's own locals named as the compile-time core's variables
`x`, `z` and `r` in `quaternion.hpp` and three of `expr/`; they are
renamed (S403).
- Alternative: `math.hpp` as it was, its banner saying what it leaves out:
  no new compile cost for its users; one header named for the module that
  does not hold it, and the evaluation's finding left standing.
- Alternative: an umbrella for each part, `math.hpp` of the compile-time
  core alone: a user picks; three more names to learn for what one header
  can give, and the module's name on the narrowest of them.

> **Review.** Taken: for, `math.hpp` holding what its banner says, every
> header, with the entry points first, so that a user who includes it has
> the language, the quaternion and linear algebra; the floor unchanged,
> C++14; a header left out of `geometry.hpp`, `platonic.hpp`, read with the
> rest. Against: `math.hpp` costs more to compile, the language's 23 headers
> among what it reads; a user who wants one part still includes that part's
> header, as the banner and the README advise; reading the compile-time core
> beside the rest brings its namespace variables `x` to `phi` beside every
> function of `math`, whose locals of those names `-Wshadow` reports (S403).
> The alternatives: `math.hpp` as it was, its banner saying what it leaves
> out; an umbrella for each part. Recommended: as taken.

**S400. The language's headers whose names another math header had are
named for what they hold: `expr/complex.hpp` is `expr/complex_kind.hpp`,
`expr/quaternion.hpp` `expr/quaternion_kind.hpp`, `expr/matrix.hpp`
`expr/matrix_kind.hpp` (and `expr/matrix_function.hpp`, S401) and
`expr/series.hpp` `expr/summation.hpp`, beside `complex.hpp`,
`quaternion.hpp`, `linear_algebra/matrix.hpp` and `calculus/series.hpp`;
and `expr/function.hpp`, beside `function/function.hpp`, is split
(S401), its tables in `expr/function_table.hpp`. No two math headers share
a name but the quaternion's, the second its name in `linalg`. Each include
guard follows its file's name; no forwarding header stands at an old
name.** The evaluation of 2026.11.12 found the names twice
over, and the owner chose the fix with the group. The language's side is
renamed: its headers are the lane's, where `calculus/series.hpp` and
`function/function.hpp` are the compile-time core's, on the owner's punch
list (`scripts/ci/ladder_known.txt`), and the class headers are the
module's entry points. A copy of the zip over the owner's tree leaves the
five old files in place, as a copy deletes nothing: they are to be deleted,
and the note names them. `linear_algebra/quaternion.hpp` keeps its name
beside `quaternion.hpp`: it holds no code of its own, but names the one
class and its functions in `linalg` (S398), and the render module includes
it by that path.
- Alternative: the names kept, each folder saying which: no break, as the
  includes stand; two `complex.hpp` and two `matrix.hpp` to tell apart in
  every search, editor and message, the evaluation's finding left standing.
- Alternative: the other side renamed: the language's includes stand; the
  owner's compile-time core and punch list edited, and the class headers,
  each a concept's first name, renamed for the language's sake.
- Alternative: forwarding headers at the old names until 0.8: old includes
  build for a while; the two names stay, each a header, until a second
  break removes them.

> **Review.** Taken: for, each of the language's headers of a name no other
> math header has, each named for what it holds, the language's kinds
> `_kind` and its tables `function_table`, so that a search, an editor's
> list or a compiler's message names one file; the renames on the lane's
> side, its own headers, the owner's compile-time core and punch list
> untouched and the class headers keeping the concepts' first names; include
> guards that follow the names; the break before 0.8, as the owner chose for
> the names on 2026.11.14. Against: an include of an old path fails, a break
> for any caller outside the framework, the framework's own changed with it;
> a copy of the zip over the owner's tree deletes nothing, so the five old
> files stay until he deletes them, and an old file left beside its new one
> defines the same entities under another guard, which a translation unit
> reading both would find twice; the decisions before S400 name the headers
> as they were; `quaternion.hpp` stays a name of two headers, the second the
> `linalg` alias the render module includes. The alternatives: the names
> kept, the folders telling them apart; the other side renamed; forwarding
> headers until 0.8. Recommended: as taken.

**S401. `expr/function.hpp`, 4,363 lines, and `expr/matrix.hpp`, 7,979, are
split along their concepts: the tables (`expr/function_table.hpp`: the
entries, the table, linear algebra's and the quaternions' evaluators and the
standard tables), the arithmetic beyond `value.h`'s (`expr/arithmetic.hpp`:
the functions' evaluators in a type and as a value, and `^` and `mod`, which
the operators share) and the relations and logic (`expr/logic.hpp`), from
`function.hpp`; the matrix kind, its values, operators, transposes and
blocks (`expr/matrix_kind.hpp`) and linear algebra's functions of them,
det, inv, trace, norm, eig, dot, cross and solve
(`expr/matrix_function.hpp`), from `matrix.hpp`. Each section moves whole,
its own numbers its new
header's; each header includes what it uses and compiles alone; no
statement changes.** `function.hpp`'s sections depended in one direction
(the logic on two helpers of the arithmetic, the standard tables on every
other), and `matrix.hpp`'s functions on the kind and nothing after them,
so each cut follows a line the code already drew. `evaluate.hpp` includes
the arithmetic and the logic, which its operators use, and the table;
`expr/expr.hpp` includes all 23. The tests' outputs are the same, byte for
byte, before and after.
- Alternative: `matrix_kind.hpp`'s arithmetic, 3,150 lines, in a header of
  its own: no header past 3,300 lines; the kind's table calls the
  arithmetic, which needs the kind, so one would include the other at its
  end, or every user would need both, the kind alone an ODR trap.
- Alternative: each family's evaluators beside its kind, the complex
  functions' in `complex_kind.hpp`: one header per family; the table's
  arithmetic, which takes every kind in one function, cut across five
  headers, and a reader of one function sent to several.
- Alternative: the two headers left whole: no move; two headers of 4,363
  and 7,979 lines, the evaluation's finding left standing.

> **Review.** Taken: for, the two largest headers, 4,363 and 7,979 lines,
> cut along lines their code already drew, the largest header now the matrix
> kind's, 4,811; each concept a header: the tables, the arithmetic the
> operators share, the logic, the kind, its functions; each header compiling
> alone at C++11 and empty below; no statement changed in the move, every
> test's output the same byte for byte; `evaluate.hpp` reading the
> arithmetic and logic it uses, not reaching them through the table.
> Against: three headers more to find, which `expr_common.hpp`'s map and the
> README name; the matrix kind still 4,811 lines, its arithmetic in it, as
> the kind's table needs it; a caller who included `function.hpp` for an
> evaluator now includes the header that holds it, or the table, which
> includes all three. The alternatives: the kind's arithmetic apart; each
> family's evaluators beside its kind; the headers whole. Recommended: as
> taken.

**S402. `values.hpp`, the compile-time core's sampler, a near twin of
`value.hpp`'s name, keeps its name until step 30 (5b) ports it.** Renaming
it now would edit the owner's punch list, where `values.hpp` has its line,
and the roadmap's 5b, which names it; the port is the place where its name
can follow what it becomes.
- Alternative: renamed now, `sampling.hpp`: the near twin gone in this
  chunk; the owner's punch list and roadmap edited for a header the lane
  does not port until step 30.

> **Review.** Taken: for, the owner's punch list and roadmap unedited for a
> header the lane ports at step 30, where its name can follow what it
> becomes; the near twin documented in `math.hpp`'s map, `values.hpp`,
> sampling, and `value.hpp`, the generic value, each said. Against: until
> step 30 the two names stand side by side, the one near twin the evaluation
> named that this chunk leaves. The alternatives: renamed now. Recommended:
> as taken, the rename with step 30.

**S403. Locals of the lane's headers named as the compile-time core's
namespace variables `x`, `z` and `r` are renamed: nine `r`s in
`quaternion.hpp`'s functions are `out`, and three of `expr/`, in
`rotation.hpp`, `complex_kind.hpp` and `evaluate.hpp`, `entries`, `nearest`
and `values`, so that no header of the lane shadows them where `math.hpp`
reads both; the 26 such locals of `function/function.hpp` and of six
geometry headers stay, for step 29 (5a), which ports `expression.hpp`, to
end at their root.** `expression.hpp` defines `x`, `y`, `z`, `t`, `r`,
`rho`, `theta` and `phi` as variables of `djinterp::math`, so that every
local of that name in a function of `math` shadows one, a warning under
`-Wshadow`, which the face tests build with and the ladder does not: before
this chunk, `math.hpp` gave such warnings at 20 places, all in `function/`
and `geometry/`; reading every header, it gave them at 38, and gives them
at 26 now, those of `function/` and `geometry/`, `platonic.hpp`'s six among
them.
- Alternative: the 26 left renamed now too: `math.hpp` clean under
  `-Wshadow` in this chunk; edits across the owner's compile-time core and
  six geometry headers for a hazard whose cause is one header's
  namespace-scope names, which 5a can put in a namespace of their own.

> **Review.** Taken: for, no header of the lane's shadowing the compile-time
> core's variables wherever both are read, twelve locals renamed, each name
> saying what it holds; `math.hpp` under `-Wshadow` back to the hazard it
> had before this chunk, in `function/` and `geometry/` alone, where 20
> places were and 26 are, `platonic.hpp`'s six read since S399. Against: a
> user who includes `math.hpp` and builds with `-Wshadow` still sees 26
> warnings, none of them the lane's code; the cause, eight variables of
> `djinterp::math` named as the commonest locals, stands until step 29. The
> alternatives: the 26 renamed now too. Recommended: as taken; at step 29
> (5a), `expression.hpp`'s variables in a namespace of their own, which ends
> every such warning at once. Ruled 2026.11.16: as recommended (decision 10,
> A), at step 29. Done in chunk 53 (S430): the placeholders in
> `djinterp::math::variables`, `math.hpp` clean under `-Wshadow`.

### S404 to S408: chunk 49, 2026.11.14

The record: `_handoffs/math98_chunk49_2026.11.14/README.md`.

**S404. The C core's sums of products and of terms are compensated by
default: vec.h's `dot`, `cross` and `sum`, and mat.h's `multiply`,
`multiply_vector`, `trace` and `power` (by `multiply`), each a sum rounded
once from Ogita, Rump and Oishi's Dot2 or Sum2 - each product exact as a
pair by Dekker's product, the terms summed by two-sums, the errors summed
apart and added last - as accurate as the sum computed in twice the
precision and rounded once.** The pairs are double's where double is
evaluated in its own type, for the float and double families - a float's
products exact in one multiplication - and long double's otherwise and
for the long double family, as quat.h's products took them (S276). Where an
entry leaves the range where Dekker's products are exact, its operand is
scaled by a power of two and the sum scaled back, rounded once, a subnormal
result's midpoint decided by the low part; where an operand has a NaN or an
infinity, the sum is the formula as written; an exact zero is +0, as the
formula from +0 makes it, and a part of `cross` that is 0 because its two
products are equal is the formula's, of the sign the arithmetic gives it.
A product's entries are tested once for the range, and each entry summed
with no test of its own where both operands lie in it.
- Alternative: the formulas as they were, compensated forms under new names
  (`dot_accurate`, ...): no bits change and no cost; accuracy only where a
  caller knows to ask for it, against S276's choice for the quaternion's
  product.
- Alternative: Kahan's or Neumaier's compensated summation of the rounded
  products: cheaper, but each product's own rounding stays, so a dot is no
  better than its products where they cancel.
- Alternative: an exact accumulator (Kulisch's, or Rump's AccSum): every sum
  correctly rounded, at several times Dot2's cost and much more code.
- Alternative: products by fused multiply-add at run time: Dot2 at a
  fraction of the cost where the target has one, but no constant expression
  and not the same bits on every target.

> **Review.** Taken: for, every dot, sum, cross product, matrix product and
> trace of the C core as accurate as computed in twice the precision and
> rounded once - in this chunk's checks every float result and every sum
> and cross part correctly rounded, and every double and long double dot the
> rounding of a value within Dot2's bound, 86 in 100 of them correctly
> rounded where the formula as written rounds 14 or 15 in 100 so
> (`raw/compensated.txt`) - the same bits on every target where the family
> is evaluated in its own type, and a constant expression from C++14 as
> before. Against: the cost, measured for double at 6 times the formula's
> for a dot of 3 terms, 22 for 16 and 13 for 256, 11 for a 4 x 4 product and
> 12 for a cross product (`raw/timing.txt`); and every result of these
> kernels may change in its last bits, the tests written against the
> formulas moved to their `_fast` forms. The alternatives: the formulas kept
> as the defaults; Kahan's summation; an exact accumulator; fused
> multiply-adds. Recommended: as taken, as S276 took it for the quaternion's
> product: a default that is right, and a `_fast` form for a measured inner
> loop. Taken.

**S405. Each kernel S404 compensates keeps its formula as written, bit for
bit, under a `_fast` name: `dot_fast`, `cross_fast`, `sum_fast`,
`multiply_fast`, `multiply_vector_fast`, `power_fast`, `trace_fast` and
`solve_fast` (S406), in the C core, the kernels and both faces, fixed and
dynamic.** The norms keep their sums as written: `norm_squared` is
`dot_fast(a, a)`, as the norms S256 and S301 scaled use it, a sum of
squares, which cannot cancel, within (n - 1) u of its value relatively, and
every norm keeps its bits. `project`, `reject`, `cos_angle` and `angle`
divide the compensated `dot`, the part of them that can cancel; quat.h's
`slerp` takes its arc's sign from it. quat.h's `rotate_fast` and isect.h's
ray tests take `cross_fast` and `dot_fast`: the first is the quaternion's
fast form, as written by its contract, and the second tests within an
epsilon, in a renderer's inner loop, which the compensated kernels would
make several times slower for no answer it can use. The generic kernel,
for exact and other number types, has one formula, and its `_fast` forms
are it.
- Alternative: no `_fast` forms: one formula each, the compensated, and a
  caller who needs the old speed or bits writes the loop.
- Alternative: the norms compensated too: each norm within about half an
  ulp, at Dot2's cost in `normalize`, the renderer's commonest call, and
  every norm's bits changed for a gain of at most (n - 1) u.
- Alternative: isect.h's tests compensated: their products more accurate,
  their epsilon the same, and their answers in an inner loop several times
  slower.

> **Review.** Taken: for, every caller's old bits and speed one name away,
> the norms' bits and S256's and S301's guarantees untouched, and the
> kernels that divide a dot more accurate where the dot cancels; quat.h's
> fast rotation and isect.h's tests as fast as they were. Against: eight
> more names in the C core and the faces, and `norm_squared(a)` and `dot(a,
> a)` no longer the same bits. The alternatives: no `_fast` forms; the norms
> compensated; isect.h compensated. Recommended: as taken. Taken.

**S406. `d_mat_<S>_solve` refines its solution with extra precision, as
LAPACK's `dgerfsx` does (Demmel, Hida, Kahan, Li, Mukherjee and Riedy,
2006): each column first as the elimination gives it, A's LU factors kept
with their interchanges, then corrected from its residual b - A x,
computed by Dot2 with x carried as a pair, x and its tail, the correction
solved by the factors and added to the pair, until every entry's
correction lies below half its unit in the last place, or neither the
largest correction nor the largest relative to its entry halves (the last
not added), or `D_CFG_MATH_MAT_REFINE_STEPS` steps have run; each entry is
then x and its tail rounded once.** For a system well away from singular,
cond(A) u well below 1, each entry is the solution correctly rounded but
where it lies within some cond(A) u^2 of a midpoint. The signature takes
the factors' room, `_scratch` of n * n + 2 n entries and `_pivots` of n,
so that a call written for the old one fails to compile rather than
overrun a buffer of n * n; `d_mat_<S>_solve_fast` is the elimination alone,
the old signature and its bits. The faces' `solve(A, b)` and `solve(A, B)`
are the refined solve, `solve_fast` the elimination; the generic kernel's
solve is its elimination, which needs no refinement for an exact type and
has no wider sum to refine with for another, and the language's solves,
which take the generic kernel, call it as `solve_fast`, their bits as
they were.
- Alternative: the old signature, the pivots kept in `_scratch` as values
  of the family's type: no call to change, and a caller's buffer of n * n
  overrun without a word.
- Alternative: refinement without the tail, x alone: the classical
  refinement with an extra-precise residual, the solution to about an ulp,
  not to its rounding.
- Alternative: LAPACK's whole procedure, with its normwise and
  componentwise error bounds returned: what a caller needs to trust an
  ill-conditioned answer, and an interface the faces do not have.
- Alternative: the elimination in long double, rounded once: no refinement
  loop, and nothing gained for the long double family or where long double
  is double.

> **Review.** Taken: for, the solution of a system well away from singular
> correctly rounded entry by entry - every entry of the 65 reference
> systems in this chunk's checks, 281, the 64 well conditioned and the one
> that is not, where the elimination alone gave 41 (`raw/compensated.txt`)
> - by the algorithm LAPACK and its analysis stand behind, its old bits
> under `solve_fast`, and no old call compiling into an overrun. Against: a
> C caller of `d_mat_<S>_solve` must change its call; and the cost, measured
> for double at 10 times the elimination's for order 3 and 7 for order 8
> (`raw/timing.txt`). The alternatives: the old signature; no tail; LAPACK's
> error bounds; the elimination in long double. Recommended: as taken; error
> bounds, should a caller ask for them, in a form of their own. Taken.

**S407. The refinement's bound is a knob, `D_CFG_MATH_MAT_REFINE_STEPS`,
in a new configuration header, `cfg_mat.h`: 10 by default, LAPACK's
ITHRESH, 1 or more.** A system cond(A) u from singular gains about -log2
(cond(A) u) bits a step, so most converge in one or two; the bound matters
only where the corrections neither converge nor stop shrinking quickly.
- Alternative: 10, written into the kernel: one less header, and no way to
  trade the last steps for speed without editing the C core.
- Alternative: an argument of `solve`: per call, and one more argument on
  every call for what is almost never changed.

> **Review.** Taken: for, the configuration's convention, the knobs in
> `cfg_*.h`, one default a host can change without editing the C core;
> against, one more header. The alternatives: a constant in the kernel; an
> argument. Recommended: as taken. Taken.

**S408. The complex dense kernels stay as they were in this chunk:
cmat.h's `d_cmat_<S>_solve` unrefined, and the products of matrices of
`math::complex<T>` summed in order by the generic kernel, each product
compensated by cplx.h's multiplication.** The real kernels came first,
being what the faces, the geometry and the renderer call; the complex ones
need a Dot2 over pairs of real products for each part of each entry, and
cmat.h's solve the same refinement with a complex residual.
- Alternative: both now: every dense kernel compensated in one chunk, and
  this chunk's delivery later by the size of cmat.h's changes and their
  tests.

> **Review.** Taken: for, this chunk delivered on the kernels every face
> calls; against, `d_cmat_<S>_solve` and the language's complex matrices as
> accurate as before, a complex solve no longer the real one's equal in
> accuracy. The alternative: both in this chunk. Recommended: cmat.h's solve
> refined and the complex products compensated in the next chunk, before
> 0.8. Done in chunk 50 (S410 to S412): cmat.h's products compensated and
> its solve refined, the generic path's and the language's complex products
> with them.

### S409 to S413: chunk 50, 2026.11.15

The record: `_handoffs/math98_chunk50_2026.11.15/README.md`.

Chunk 50's decisions, S409 to S413: the second part of the accuracy the
owner scoped on 2026.11.14, the complex dense kernels and the scaled
projection his ruling of 2026.11.04 asked for. Chunk 49's, S404 to S408,
stand: his "continue" of 2026.11.15 followed its delivery.

**S409. `project` and `reject` scale their operands at the range's ends,
as S301 was ruled on 2026.11.04: in range - the squared norm d of the
vector projected onto from MIN/EPS to MAX, the compensated dot p 0 or its
ratio to d from MIN/EPS to MAX - each component p/d times the other's, as
before, every bit kept; for the zero vector, or an operand with a NaN or an
infinity, the formula as written; elsewhere each operand scaled by a power
of two, its largest entry to [1, 2), the scaled dot by Dot2 over the scaled
squared norm summed as `norm_squared` sums, each component that ratio times
the scaled entry, scaled back by the first operand's power.** Scaling by a
power of two commutes with rounding in the normal range, so a projection
whose result is in range is computed in range wherever its operands lie: a
vector onto itself at either end is itself, where the formula made it NaN
or 0.
- Alternative: one path, always scaled: simpler, and the in-range bits
  changed, against the ruling's "every bit in range kept".
- Alternative: the squared norm compensated too, by Sum2 of the squares: a
  more accurate ratio, and `norm_squared`'s bits, which S405 kept as
  written, no longer the projection's.
- Alternative: the ratio in long double: nothing gained where long double is
  double, nor for the long double family.

> **Review.** Taken: for, the ruling of 2026.11.04 carried out: every bit in
> range kept, t_vec.c's 200,000 random pairs per family still the formula's,
> and 2,000 pairs per family moved past the range by powers of two each
> giving the in-range bits moved, every one checked, where chunk 49's
> formula fails 4,449 of those checks (`raw/s301_probe.txt`); a vector onto
> itself at either end itself, where the formula made it NaN or 0. Against:
> two paths where one would serve, and the cost: a double projection of 4
> components in range 25.8 ns against chunk 49's 16.9, its largest entries
> found first, and past the range 330.3 (`raw/timing.txt`). The
> alternatives: one path, always scaled; the squared norm compensated; the
> ratio in long double. Recommended: as taken. Taken.

**S410. cmat.h's complex dense kernels are compensated and refined:
`d_cmat_<S>_multiply`, new, rounds each part of each entry once from Dot2's
pair - the real part's products re re and -im im, the imaginary part's re
im and im re, each exact as a pair - and `d_cmat_<S>_solve` refines its
solution as mat.h's does (S406), the residual's parts by Dot2 with the
solution carried as a pair, part by part, until every part's correction
lies below half its unit in the last place, neither the largest correction
nor the largest relative halves, or `D_CFG_MATH_MAT_REFINE_STEPS` steps
have run.** The sums go through an accumulator vec.h now keeps,
`dj_internal_vec_<S>_acc`, Dot2's running pair in the pairs the family
sums in, for callers whose terms are not strided arrays of one family:
added to by a product or a term, rounded once, scaled as the family's dot
scales; with C++ overloads of one name for the three families. As vec.h's
dot: operands past the range scaled by their largest part's power, a NaN or
an infinity the formula as written, every product 0 the constant's parts
plus 0, +0 for either zero. The solve takes the factors' room,
`(x, scratch, pivots, a, b, n, m)`, scratch n * n + 4 n, so that an old
call fails to compile rather than overrun; `d_cmat_<S>_multiply_fast` is
cplx.h's products summed in order, and `d_cmat_<S>_solve_fast` the old
elimination, its signature and bits.
- Alternative: each part as two real dots, re re and im im apart, then
  subtracted: vec.h's kernels as they are, two roundings a part, and the
  cancellation between the two dots unprotected.
- Alternative: Gauss's product of three multiplications: fewer products,
  and an error that grows where the parts differ in size.
- Alternative: the parts copied into real arrays, interleaved, and vec.h's
  dot called: the same bits (the checks show it), a copy per entry, and no
  form the generic path can call on `math::complex<T>`'s elements.
- Alternative: the old signature, the refinement inside n * n of scratch:
  no room for the factors, the pair and the correction.

> **Review.** Taken: for, every part of a complex dot within Dot2's bound of
> the exact sum - 120,000 parts checked exactly, in six builds - and the
> refined complex solve exact on all 562 parts of 65 reference systems, 64
> well conditioned, where the elimination gave 72 (`raw/compensated.txt`);
> the old bits under `_fast` names; and, cplx.h's products being themselves
> within 2 ulps, the compensated product the cheaper: a double 4 x 4 product
> 1,130.5 ns against `multiply_fast`'s 2,128.4. Against: a C caller of
> `d_cmat_<S>_solve` must change its call; the refined solve's cost, 3,211.6
> ns against 1,212.6 for a double system of order 3, 24,510.1 against
> 11,840.7 for order 8 (`raw/timing.txt`); and vec.h's internal API larger
> by an accumulator and its nine overloads. The alternatives: two real dots
> a part; Gauss's three products; the parts copied into real arrays; the old
> signature. Recommended: as taken. Taken.

**S411. The generic path's `math::complex<T>` follows cmat.h: its `dot`,
`sum`, products, `multiply_vector`, `trace` and `power` are compensated, and
its `solve` refined, each a C++ mirror of cmat.h's kernel on vec.h's
accumulator through its overloads, so that the faces' matrices of complex
numbers are cmat.h's bit for bit, from C++98 and as constant expressions
from C++14.** `linalg_sums<Type>` holds the sums: for every type the
formula as written, exact for an exact type; for `math::complex<T>`
cmat.h's Dot2, each part of a sum of terms Sum2 scaled as vec.h's sums
scale. `solve` is tag-dispatched by `linalg_by_lu`: for complex numbers LU
with partial pivoting kept, each column of B gathered into X's column in
the factors' row order, solved there and refined in place, the residual
taken in the factors' order, so that the C core's arguments suffice,
scratch n * n + 2 n; for every other type the elimination. Each keeps its
formula under its `_fast` name, `power_fast` now by `multiply_fast`.
- Alternative: the generic path calling cmat.h itself: its elements are
  `math::complex<T>`, not the C structs, so a reinterpreting cast, which no
  constant expression allows, or a copy into scratch of the C struct type,
  which the faces' scratch is not.
- Alternative: the generic path's complex sums as written: a face and the C
  core apart on the same matrix, against S315's parity.
- Alternative: a compensated generic path for every floating type: no other
  type of the generic path has a wider sum to compensate with; the exact
  ones need none.

> **Review.** Taken: for, the faces' complex matrices as accurate as
> cmat.h's and its bits exactly: t_complex_face.cpp holds the determinant,
> the inverse, both solves, the products, the cubes and the traces of 1,000
> random matrices per family to cmat.h's, a fifth of them of small integers
> with zero pivots and ties, and the refined solution, a product and a trace
> as constant expressions to the run time's (`raw/smoke.txt`); the scratch
> the faces already give. Against: some 650 lines of C++ mirroring cmat.h's
> kernels, which must stay in step with them - the parity checks hold them
> so - and the cost, the face's double 4 x 4 product 1,319.0 ns against
> `times_fast`'s 904.6 and its solve of order 3 2,936.5 against 1,059.0
> (`raw/timing.txt`). The alternatives: the generic path calling cmat.h; its
> sums as written; every floating type compensated. Recommended: as taken.
> Taken.

**S412. The language's dense complex products follow the kernels, as S349
asked: each element's parts rounded once from Dot2's pair, a real
operand's element taken as itself, its products with the other's parts
alone, each part then a real Dot2; the language's complex trace, the
kernels', is each part's Sum2.** Where an operand has a NaN or an infinity
the element is the sum from 0, in order, of math::complex's products, as
before.
- Alternative: the language's complex products as written, S349's
  sentence changed: the language's real products compensated (S404) and
  its complex ones not.
- Alternative: a real operand widened to x + 0i: the same values, its zero
  products adding nothing, and G.5's "taken as itself", the language's
  rule, no longer the code's.

> **Review.** Taken: for, the language's complex products as accurate as its
> real ones (S404) and S349's parity kept: a product of complex matrices the
> kernels', a real matrix's by a complex one each part the C core's real dot
> of a row with the column's parts, as t_expr_complex_matrix_face.cpp
> checks. Against: the products' bits differ from chunk 49's in their last
> places. The alternatives: the products as written; a real operand widened.
> Recommended: as taken. Taken.

**S413. The language's dense solves stay unrefined: a complex one by
cmat.h's `d_cmat_solve_fast`, its bits as they were, as the real one by
decomp.h's LU and the quaternions' and values' by the generic kernel's
elimination (S406).** This chunk changes no solve of the language.
- Alternative: the language's float, double and long double solves refined,
  the real by `d_mat_solve` and the complex by `d_cmat_solve`: `solve(A, B)`
  in the language as accurate as `la::solve`, at the refinement's cost, the
  language's tests' expected solutions changed.

> **Review.** Taken: for, this chunk's change kept to the kernels S408
> named, the language's solutions the bits its tests and users had. Against:
> the language's `solve(A, B)` of floats and doubles the elimination's,
> while `la::solve` of the same matrices is refined, each part correctly
> rounded where the system is well conditioned: the one place a user of the
> language gets less than the face gives. The alternative: the language's
> float, double and long double solves refined. Recommended: the
> alternative, the real solves by `d_mat_solve` and the complex by
> `d_cmat_solve`, with step 29 (5a) or the next chunk that touches the
> language's matrices, before 0.8. Ruled 2026.11.16: as recommended
> (decision 8, B): the language's float, double and long double solves
> refined, real and complex, before 0.8, with step 29. Done in chunk 53
> (S428): the real solves by `d_mat_solve`, the complex by `d_cmat_solve`.

### S414 to S423: chunk 51, 2026.11.16

The record: `_handoffs/math98_chunk51_2026.11.16/README.md`.

Chunk 51's decisions, S414 to S423: the first of the double family's
rewrite, under the owner's rulings of 2026.11.16 at 09:26 on the
decisions of 2026.11.15 - the double family on pairs of doubles, exp and
log first, each in three phases with proven bounds (1, B); the eight rules
(2, A); exp and log now, then 0.8's items, core v2 in December, then the
rest of the family (3, B); the x87 refused for the family's functions
(4, B); the lane's own algorithms, proven with Gappa and Sollya, where the
lane had recommended CORE-MATH's (5, B); a public pair type (6, A); and
kernels immune to fusing (7, B). His rulings on 0.8's items are in the
reviews they rule: S413 (8, B), S391 (9a, A; 9b, C) and S403 (10, A), each
"Ruled 2026.11.16", each outstanding until done before 0.8. Parse's M3
(11, A) is no decision of the lane's: math reads the opener's position from
the note in the next diagnostic record, as its note of 11-13c answered,
and its note of 11-16 says so. Chunk 50's decisions, S409 to S413,
stand: his rulings of 11-16 followed its delivery.

**S414. Every exact product is immune to a compiler that fuses a multiply
and an add (ruling 7, B): wherever the target has a fused multiply-add for
the family, an exact product takes its low part with an explicit one,
`fma(a, b, -high)`, exact, so the bits Dekker's product gives; where it has
none, Dekker's product, which no compiler can fuse there; and in a C++
constant evaluation Dekker's product too, an explicit fused multiply-add
being no constant expression for every compiler, the same bits.** Found
while preparing the decisions of 11-15: gcc fuses by default in its GNU
modes wherever the target has the instruction, every ARM64 and x86 with a
`-march` that has it, and a fused `split * a - a` breaks Veltkamp's split;
built with `gcc -std=gnu11 -O2 -march=x86-64-v3`, chunk 50's compensated
double dot was correctly rounded on 2,403 of 20,000 cancelling dots, its
worst error 6.4e13 ulps. `math_common.h`'s new section 5 says when the
builtin may be called - `D_MATH_{FLT,DBL,LDBL}_FMA` and `_FMA_OF`, from
gcc's `__FP_FAST_FMA` macros and x86's and ARM's own; in C, in C++ before
C++14, where no kernel is a constant expression, and from C++14 where
`D_MATH_CONSTANT_EVALUATED` keeps it out of a constant evaluation - and
`D_MATH_RUN_TIME()` guards each call. The four exact products the
generator writes take it - `vec.h`'s, `cplx.h`'s for each family,
`elementary.h`'s long double product and float square, and `dd.h`'s (S417)
-- and no error-free transform anywhere takes a product that is not exact.
Every generated header opens with clang's `#pragma float_control(push)` and
`#pragma STDC FP_CONTRACT OFF` and closes with the pop
(`D_MATH_PRAGMA_NO_CONTRACT`, clang from 11), so that clang fuses nothing in
them but under `-ffp-contract=fast`; gcc ignores the pragma, and there the
exact products are safe by the above and the plain formulas, the `_fast`
forms among them, follow the build (S415). `check_math_tests.sh` gains a
third tier, `fused`, which builds every test as gcc and clang fuse: GNU C11,
GNU C++14 and C++20 for the harnesses with a C++14 section, GNU C++17 for
the faces, with the host's fused multiply-add (`-march=x86-64-v3`).
- Alternative: fusing forbidden in the headers, by the standard pragma and
  gcc's `#pragma GCC optimize("fp-contract=off")`: gcc ignores the
  standard pragma, documents its own as not for production code, and stops
  inlining a function it marks, in every build.
- Alternative: the needs list's X3 as written, `-ffp-contract=off` asked of
  every user and a self-test to report fusing: a build that misses the note
  is silently wrong, and the flag stops harmless fusing in the user's own
  code.
- Alternative: Dekker's product everywhere, the build refused where it
  fuses: no macro says that gcc fuses - `__FP_FAST_FMA` says that the
  target can, which an ISO build with the instruction does not use - so
  the refusal would take every ARM64 build with the ones it must.

> **Review.** Taken: for, every compensated kernel's exact products exact in
> every build, fused or not, with nothing asked of a user's flags; where the
> target has the instruction, an exact product two operations instead of
> some seventeen; clang told not to fuse in the headers, so that its plain
> formulas keep their ISO bits but under -ffp-contract=fast. Against: two
> forms of every exact product, both to be tested - the fused tier tests
> the explicit one, every other tier and every constant evaluation Dekker's
> - and, where gcc fuses, the plain formulas' and the `_fast` forms' last
> bits following the build (S415). The alternatives: fusing forbidden in the
> headers, which gcc does not honour in a header without losing inlining;
> `-ffp-contract=off` asked of every user, silently wrong where missed;
> Dekker's everywhere with a refusal where the build fuses, which would
> refuse every ARM64 build. Recommended: as taken. Ruled 2026.11.16: as
> recommended (decision 7, B).

**S415. The double family's contract (ruling 2, A): one result, the
correctly rounded one, on every platform, under the ruling's eight rules --
no long double inside; no C library calls; no fused multiply-add made by
the compiler that changes a result, with a start-up self-test that says if
one did; plain double arithmetic, so no x87; no dependence on a C library
at all (wasm32); correctly rounded; within 2 to 3 times st_math's speed;
and C's standard answers outside the domain. The roadmap's principle 4 and
gate G6 say so.** It holds for each of the family's functions as its
rewrite lands, `exp_c` in this chunk (S416), and for `dd.h`'s exact sums and
products, exact in every build, its other results within their proven
bounds, fused or not (S417). The plain formulas - every kernel written
as its formula, the `_fast` forms among them - follow the build: where it
fuses, their last bits may differ from an ISO build's, as ruling 7 has it,
and `d_math_formulas_fuse()` says whether the translation unit's build
fuses (S419). The float and long double families keep their contract until
their turn, float from the double kernels (ruling 1); the plain double
forms, `d_math_d_exp` among them, call the C library until ruling 12, due
in early December.
- Alternative: correct rounding alone, without the speed and no-long-double
  rules (the ruling's B): the same bits eventually, at 10 to 100 times the
  speed simterp needs each tick, so that the terps keep st_math beside it.
- Alternative: the terps keep st_math (the ruling's C): two math libraries
  in one project, one whose pow is 120 ulps off.
- Alternative: the plain formulas held to one result too, fusing forbidden:
  not possible for gcc in a header (S414).

> **Review.** Taken: for, one answer on every platform and compiler, the
> best one, each function meeting the eight rules as its rewrite lands, so
> that the terps can retire st_math and share one library. Against: the
> largest block of work left, by the decisions' estimate 15 to 20 chunks; a
> 32-bit x87 build without SSE2 refused (S418); the plain formulas outside
> the contract where the build fuses; and until ruling 12 the plain double
> forms the C library's, a second answer beside each `_c` form wherever the
> library's is not correctly rounded. The alternatives: correct rounding
> without the speed and no-long-double rules; the terps keeping st_math.
> Recommended: as taken, and each plain double form the correctly rounded
> function once it is within about twice the C library's speed, as decision
> 12 recommends. Ruled 2026.11.16: as recommended (decision 2, A).

**S416. `d_math_d_exp_c` is the exponential correctly rounded, in double
arithmetic alone, by three phases each with a proven bound, the lane's own
algorithm (rulings 1, B, and 5, B): x reduced to (128 e + j) ln2/128 + r,
the integer nearest x 128/ln2 found by the shift 1.5 2^52, |k| < 2^18,
ln2/128 in three parts L1 + L2 + L3, L1 and L2 of 35 bits so that k L1
and k L2 are exact, r1 + r2 exact but for 2^-113.9 and |r1| < 0.00271;
then e^x = 2^e 2^(j/128) e^r, 2^(j/128) from a table of 128 pairs of
doubles, each within 2^-107 of its value. Phase 1, in doubles, q of degree
3 by Sollya and T r1 split so that its high product is exact: within
2^-63.5 relatively, tested against 2^-63, k = 0 by its own bound; phase 2,
in `dd.h`'s pairs, P of degree 6, its first two coefficients pairs:
within 2^-101.2 absolutely of a result below 2.01, tested against 2^-100;
phase 3, in fixed point, 16 limbs of 16 bits in `unsigned long`, so that
no 64-bit integer is needed: x exact, k ln2/128 from 240-bit digits, e^r by
its series, times 2^(j/128) from 224-bit digits, within 51.4 units of
2^-224, 2^-218.3 relatively, then rounded at 53 bits or at the least
subnormal. Each phase gives its result where the interval its bound allows
rounds one way, as Ziv's method has it.** The bounds are proven in the
record's `proofs/`: Sollya's `supnorm` for the polynomials, Gappa for each
phase's rounding errors from the code's own operations, every product a
compiler may fuse modelled both ways, and exact rational arithmetic for
phase 3. The hardest case known, Lefevre and Muller's
0x1.9e9cbbfd6080bp-31, lies 2^-110.6 relatively from a rounding boundary,
far outside phase 3's bound. NaN gives NaN, x past 6243314768165359/2^43
(709.78) +inf, x below -6554261109157969/2^43 (-745.13) +0, and
-2^-53 <= x < 2^-53 gives 1 + x; a result below the least normal is rounded
to the subnormals at each phase's test, from the pair with its error. A
constant expression from C++14 through all three phases. Measured
(`raw/timing.txt`, gcc -O2): 9.9 ns a call over [-745, 709.7] against the C
library's 6.7, 9.0 over [-1, 1] against 3.8; 57.8 ns where phase 2 decides;
3,950 ns at the hardest known argument, where phase 3 does; 22 to 27 times
faster than chunk 50's `exp_c` on the long double kernel; phase 1 decides
99.86% of arguments over each range and phase 2 the rest, of ten million
each, phase 3 none. The plain `d_math_d_exp` stays the C library's
(S415).
- Alternative: CORE-MATH's exp, ported with its proofs (decision 5's A, the
  lane's recommendation): proofs reviewed by specialists and the hard cases
  searched, and the licence notice in each ported file; the owner ruled
  the lane's own.
- Alternative: chunk 50's long double kernel with a rounding test and an
  exact fallback (decision 1's A): correctly rounded too, at 149 to 597
  ns a call, and slower where long double is 64 bits or 128 in software.
- Alternative: phase 3 straight after phase 1: one bound fewer to prove,
  and the 0.14% that phase 1 leaves would pay phase 3's 3.95 us, some 5.5
  ns a call on average, where phase 2's 57.8 ns costs some 0.08.
- Alternative: phase 3 in triple doubles: a third floating-point bound to
  prove, where fixed point's is counted exactly and needs no 64-bit
  integer.
- Alternative: a table of 256 pairs, or 64: a shorter phase 1 polynomial
  for twice the memory, or a longer one for half.

> **Review.** Taken: for, exp correctly rounded on every platform in double
> arithmetic alone, each phase's bound proven from the code's own
> operations, fused or not, the hardest case known far inside phase 3's
> bound; 9.9 ns a call, 1.5 times the C library's over the range and 2.4
> times over [-1, 1], and 22 to 27 times faster than chunk 50's `exp_c`; a
> constant expression from C++14 through all three phases; no 64-bit integer
> needed. Against: phase 3's 3.95 us at the arguments that need it, none of
> ten million random ones; a 2 KB table; proofs the lane wrote, which no
> specialist has reviewed, their risk held by Gappa's and Sollya's
> certificates, run again in this session (`raw/proofs.txt`), and by 2,398
> references with every phase checked alone; st_math's own speed not
> measured here, where the needs list puts its functions at 9 to 36 ns a
> call; the plain form the C library's until ruling 12. The alternatives:
> CORE-MATH's exp, ported; chunk 50's kernel with a rounding test; no phase
> 2; phase 3 in triple doubles; a table of another size. Recommended: as
> taken, now that its proofs are made and checked; an outside review of
> them, when one can be had. Ruled 2026.11.16 (the owner's): the lane's own
> algorithms, decision 5's B, where the lane had recommended CORE-MATH's, A.

**S417. `dd.h` is a public pair type (ruling 6, A): `struct dj_math_dd
{high, low}`, normalized, with `d_math_dd_of`, `_value`, `_neg`, the exact
`_two_sum`, `_fast_two_sum` and `_two_product`, and `_add_d` 2u^2, `_add`
3u^2, `_sub_d`, `_sub`, `_mul_d` 3u^2 (2u^2 fused) and `_mul` 7u^2 (5u^2
fused) by Joldes, Muller and Popescu (ACM TOMS 44(2), 2017) with their
proven bounds, and `_div_d` 12u^2, `_div` 38u^2 and `_sqrt` 32u^2, the
lane's, proven with Gappa; u^2 = 2^-106, relatively, where no operation
overflows or underflows.** The products become the paper's fused variants
where the compiler fuses, the bits following the build and the bounds
holding either way; the quotients and the square root are written so that
no inexact product reaches an error-free transform (S414). Every function
is pure double arithmetic, a constant expression from C++14; where double
is evaluated wider, it refuses to compile (S418). `t_dd.c`, new, checks
every result exactly in integers of 768 bits, 200,000 random arguments per
operation, edges and normalization: the largest errors, in u^2, `add_d`
1.571, `add` 1.743, `mul_d` 2.375 (fused 1.702), `mul` 3.605 (3.500),
`div_d` 2.333, `div` 6.118 (5.046), `sqrt` 2.808. Measured: `two_product`
2.1 ns, `add` 2.7, `mul` 2.3, `div` 6.3, `sqrt` 72.0, its square root
`elementary.h`'s correctly rounded `d_math_d_sqrt_c`.
- Alternative: the pairs kept internal (decision 6's B): no names to
  freeze, and every user who needs them writes their own.
- Alternative: every operation correctly rounded to the pair (decision 6's
  C): several times slower, for little a user gains.
- Alternative: the paper's own quotients with their published bounds:
  tighter on paper, with products whose errors an error-free transform
  takes, which a compiler that fuses breaks.

> **Review.** Taken: for, one tested pair type for every user, each
> operation within a bound proven for the code as written, fused or not, the
> exact sums and product exact in every build, constant expressions from
> C++14; `t_dd.c`'s exact checks found every error within its bound.
> Against: names frozen once 0.8 ships them; the quotients' bounds, 12 and
> 38 u^2, and the root's, 32, proven but loose - the worst errors found are
> 2.3, 6.1 and 2.8 u^2 - which a function that leans on them must budget
> for; `sqrt` 72 ns, its first step `d_math_d_sqrt_c`, the correctly rounded
> root in pure arithmetic. The alternatives: the pairs internal; every
> operation correctly rounded to the pair; the paper's quotients.
> Recommended: as taken; tighter quotients, and the root by the instruction
> at run time, where a function needs them. Ruled 2026.11.16: as recommended
> (decision 6, A).

**S418. The double family refuses, at compile time, a build whose double
arithmetic is not plain double (ruling 4, B): where `D_MATH_DBL_EVAL_WIDE`
and the compiler has the `error` attribute, each of the family's functions
begins with `D_MATH_DBL_REFUSE(x)`, a call, under `if (x == x)`, to a
function whose attribute stops the build with "double is evaluated wider
than double: build with -msse2 -mfpmath=sse for the double family's
correctly rounded functions"; so a translation unit that never calls one
builds, and the float and long double families build as before.** The
condition keeps a constant evaluation of a NaN from reaching the call and
clang from taking the function to have no constant value. `exp_c`, `dd.h`
and `d_math_self_test` refuse; `expm1_c`'s far path, which calls `exp_c`
elsewhere, takes the long double kernel there, chunk 50's, with its bound,
so that +inf and arguments past the overflow never reach the split, whose
k would not fit an int (found by the handoff's zip check, `t_exponential`
at `-O0` on the x87: `expm1(+inf)`); the harnesses leave the family out
with a note, and `t_constants_face.cpp` guards its double `exp_c`. Where the
compiler has no such attribute, the functions document the bound they lose.
S37 and S39 (step 39) stay, for the other families.
- Alternative: the x87 supported as now, the drift documented (decision
  4's A): silently wrong bits on those builds.
- Alternative: a slow path through memory for the x87 (decision 4's C):
  several times slower, and fragile where clang keeps extra bits through a
  store (S37).
- Alternative: `#error` in the headers where double is evaluated wider:
  every translation unit that includes `exponential.h` refused, its float
  users with the rest.

> **Review.** Taken: for, no silently wrong answer from the double family on
> any build: an x87 build that uses it stops, the fix in the message, and
> one that does not builds as before, the float and long double families
> with it. Against: a 32-bit x86 build must add two flags to use the double
> family, and a processor without SSE2 cannot; a compiler without the
> `error` attribute gets documentation, not a refusal; `expm1_c`'s far path
> keeps chunk 50's long double bound on the x87, not the family's. The
> alternatives: the drift documented; a slow path through memory; `#error`
> in the headers. Recommended: as taken. Ruled 2026.11.16: as recommended
> (decision 4, B).

**S419. `self_test.h`, new: `d_math_self_test()` gives 0 where the double
family's guarantees hold in the translation unit's build, as known results
show, and otherwise `D_MATH_SELF_TEST_PRODUCTS` (1u), an exact product or
sum that was not, `D_MATH_SELF_TEST_EXP` (2u), an exponential not
correctly rounded, or both: three exact products and their sums against
their parts, and `exp_c` at four arguments, one each phase decides, the
hardest known among them, each read through a volatile; and
`d_math_formulas_fuse()` says whether the build fuses a multiply and an add
written as two statements: (1 + 2^-30)(1 - 2^-30) - 1, its factors
volatile, is 0 unless fused.** A header library cannot see its build:
`-ffast-math`, a compiler that fuses where it should not, a target that
rounds otherwise. So a program that needs the same bits calls the self-test
once at start-up, in each translation unit whose build may differ, and
stops where it is not 0. `t_exponential.c` holds it to 0 in every build
of the tiers; it gives 3 under `-ffast-math`, by gcc and clang alike; and
`d_math_formulas_fuse` is true for gcc in its GNU modes with the
instruction and for clang under `-ffp-contract=fast`, false in ISO modes
and in clang's GNU modes, which the headers' pragma governs.
- Alternative: a test run on its own at start-up: C has no static
  constructor, and a header cannot run code no caller calls.
- Alternative: every function tested at many points: a longer start-up for
  little more, as an exact product or sum that fails shows the build, and
  exp's four points take each phase.
- Alternative: fusing told at compile time: no macro says that gcc fuses
  (S414).

> **Review.** Taken: for, a program that needs the same bits learns at
> start-up, in each translation unit, whether its build keeps them,
> `-ffast-math` among the builds caught; and whether the plain formulas
> fuse, which the harnesses read (S421). Against: the program must call it,
> as a header cannot run code by itself; and known results show only what
> they test, so that a build that breaks a path its three products and four
> exponentials do not take passes. The alternatives: a test run by itself,
> impossible in C; more points, a longer start-up; fusing told at compile
> time, which no macro says. Recommended: as taken, its points widened as
> each function lands, log's in chunk 52. Ruled 2026.11.16: as decisions 2
> and 7 have it, a start-up self-test, kept as a check.

**S420. `exp_c`'s table, 128 pairs, is one object, not a copy per call: in
C++ a class template's static constexpr member, in C a function-local
static; each entry an exact literal, an odd integer over a power of two, as
every level from C99 and C++98 reads it.** gcc copies a local constexpr
array onto the stack at each call; a template's static member has one
definition however many translation units include the header, at every
level from C++98 - constexpr from C++11, a const defined after the class
before it - without C++17's inline variables.
- Alternative: hexadecimal literals: C99's and C++17's, not C++98's to
  C++14's.
- Alternative: a namespace-scope constexpr array: one copy per translation
  unit before C++17's inline variables.

> **Review.** Taken: for, one table however many translation units include
> the header, read by a constant evaluation, exact as written at every
> language level. Against: a template written for one table, and literals as
> fractions where hexadecimal would read shorter. The alternatives:
> hexadecimal literals, not before C++17; a namespace-scope array, one per
> translation unit before C++17. Recommended: as taken. Taken.

**S421. Where a build fuses, the harnesses leave out the checks that a
function's bits are another computation's - a kernel against its formula
written out, the same formula in two orders, a constant evaluation against
the run time for an operation that is not exact - and say so, by
`d_math_formulas_fuse()`: the fusing switch of `t_mat.c`, `t_plane.c`,
`t_decomp.c`, `t_vec.c`, `t_cmat.c` and `t_compensated.c`, and `t_dd.c`'s
parity of its non-exact operations; every check against exact arithmetic,
or of an exact result, stays.** Such bits follow the build (S415): the last
fused tier before the switch, at `b4cdb4e`, failed 9 of 282 builds on
those checks alone; the optimization levels found one more in
`t_compensated.c`, that the formula overflows on MAX + MAX - MAX, which gcc
at `-O3` with the fused multiply-add, inlining the check, gives `dot_fast`
as MAX: it and its complex twin are left out where the build fuses; and the
fused tier found two in `t_decomp.c`'s C++14 section, the float and double
`eigen_c`'s constant evaluation against the run time, which the switch had
not reached.
- Alternative: closeness in place of identity there: a check every small
  error passes tests less than the checks against exact arithmetic beside
  it.
- Alternative: those harnesses built with `-ffp-contract=off` in the fused
  tier: the tier would then not test fusing in them at all.

> **Review.** Taken: for, the fused tier tests what a fusing build must keep
> - every exact result, and every result against exact arithmetic - and
> leaves out only what ruling 7 says follows the build, each harness saying
> so in its output. Against: a fusing build's plain formulas held to no
> bits, only to the checks against exact arithmetic where a harness has
> them. The alternatives: closeness checks; the fused tier unfused for those
> harnesses. Recommended: as taken. Taken.

**S422. `exp_c` is checked against references made by mpmath at 400 bits,
`math_tests_exp_refs.h`, made by `tools/refs_exp_c51.py` with its
`--check`: 2,398 arguments in four groups - 2,048 random over the range;
256 within 2^-66 of a midpoint; 37 within 2^-102 of one, found by
`tools/hard_exp_c51.c`'s window search in five binades, 2^-27, 2^-30,
2^-33, 2^-36 and 2^-39, of 49 it screened, and checked at 400 bits, the
known 0x1.9e9cbbfd6080bp-31 first; 57 at the
edges - each phase checked alone at every argument where it is defined,
its result within its bound of the reference, and phases 2 and 3 each
required to be reached.** On the references phase 1 decides 1,887 of the
1,888 random arguments it reduces, phase 2 252 of the 256 near a midpoint,
phase 3 all 37 hard ones and two of the edges, 2^-53 and the next double.
- Alternative: random arguments alone: none of ten million reached phase 3.
- Alternative: the published hard cases alone (Lefevre and Muller's
  lists): not in the workspace, where the window search found 37 in
  minutes.
- Alternative: every double: 2^64 arguments, out of reach; float's turn
  will be exhaustive.

> **Review.** Taken: for, every phase reached and checked alone at every
> argument, phase 3 on 37 arguments within 2^-102 of a midpoint, which
> random arguments never reach; the references made at 400 bits by mpmath,
> not by the code's method, and kept in step by `--check`. Against: 2,398
> arguments of 2^64, the hard ones from five binades near 2^-30 alone; the
> proofs, not the references, carry the guarantee. The alternatives: random
> arguments alone; the published lists; every double. Recommended: as taken,
> the window search widened to more binades as time allows. Taken.

**S423. The delivery is over the owner's tree of 2026.11.16 16:00, his
repository's `7de55b67`: the 33 files chunk 51 changed or made, and the
records, with `NOTE.md`, `MANIFEST.md` and `HANDOFF.md` beside the
`djinterp/` folder and `decisions.md` and `zip_check.txt` apart, as S394's
rule has it, "every file of the lane's working tree that his tree has not
or holds otherwise". It supersedes the cumulative zip over his tree of
2026.11.07: his tree now holds chunks 44 to 50 as he committed them, the
lane's files byte for byte as chunk 50 delivered them, and nothing is to be
deleted.** He gave the session his repository in place of the 11-07 zip and
chunk 50's, so the lane's scratch repository was made from it
(`tools/rebuild_scratch_c51b.sh`): chunk 51's commits rebased onto his 11-16
tree, the rebased change chunk 51's own blob for blob; and his
`dconfig.h`, `ladder_known.txt` and `style_known.txt`, which he merged with
other lanes' work, are his.
- Alternative: the cumulative zip over 2026.11.07, as chunks 46 to 50's:
  copied over his tree, it would put back the 11-07 forms of those three
  files, dropping cfg_sort, ostream, option_env and bitwisedom from
  `dconfig.h` and his 11-14 ladder lines.
- Alternative: every lane file, those his tree holds byte for byte among
  them: a larger zip, and nothing it changes.

> **Review.** Taken: for, a zip the owner copies over the tree he has, which
> changes the lane's 33 files and nothing of his or other lanes'; the
> scratch repository made from his repository, each base checked by its
> tree's hash and the rebased change chunk 51's own, blob for blob. Against:
> the zip is not enough over the 11-07 tree, should that tree be wanted
> again; the scratch repository keeps it as a tag, from which the cumulative
> zip can be made. The alternatives: cumulative over 11-07, which would put
> back three shared files; every lane file. Recommended: as taken. Taken.

### S424 to S427: chunk 52, 2026.11.20

The record: `_handoffs/math98_chunk52_2026.11.20/README.md`.

Chunk 52's decisions, S424 to S427: the second of the double family's
rewrite, under the owner's rulings of 2026.11.16 - exp and log first,
each in three phases with proven bounds, by the lane's own algorithms
(1, B; 3, B; 5, B), the x87 refused (4, B), kernels immune to fusing
(7, B). Chunk 51's decisions, S414 to S423, stand: the owner's "continue
for now" of 2026.11.20 at 06:59 followed its delivery. His question of
2026.11.19, whether to split math into modules, is on the owner's
decisions page; on 2026.11.19 he asked the lane to go on as it is.

**S424. `d_math_d_log_c` is the natural logarithm correctly rounded, in
double arithmetic alone, by three phases each with a proven bound, as
`exp_c` is (S416; rulings 1, B, and 5, B): x = m 2^e, m in [1, 2), from its
bits where the language can read them and by `float_order.h`'s binade
search elsewhere, a subnormal first lifted by 2^54; i, the integer nearest
128 (m - 1), picks c = 1 + i/128, and from i = 53, c > sqrt(2), m and c are
halved and e raised by 1; inv the double nearest 1/c, 1 at i = 0 and 128,
and L = -log(inv) from a table of 129 triples, its pair within 2^-107.07 of
L relatively; r = m inv - 1 exactly as a pair, |r| < 2^-8, by an exact
product, Sterbenz's lemma and Fast2Sum. Phase 1, in doubles: e LN2H exact,
LN2H of 42 bits, L's high part and r taken by three Fast2Sums, r^2/2 split
exactly, q of degree 5 by Sollya: within 2^-67.17 of log(x) relatively,
tested against 2^-64; phase 2, in `dd.h`'s pairs, P of degree 10, its
first five coefficients pairs, ln2 in three parts, the first two of 42
bits: within 2^-101.48, tested against 2^-100; phase 3, in `exp_c`'s
fixed point and with its operations: e ln2 from `exp_c`'s 240-bit digits
of ln2/128, |L| from 224-bit digits, log1p(r) by its series, the positive
parts and the negative ones apart: within 36.1 units of 2^-224, 2^-165.83
relatively, then rounded at 53 bits. Each phase gives its result where the
interval its bound allows rounds one way, as Ziv's method has it.** The
bounds are proven in the record's `proofs/`: Sollya's `supnorm` for the
polynomials, against their Taylor polynomials of degree 40; Gappa for
phases 1 and 2 in each of three cases - e not 0; e = 0 and i neither 0 nor
128, where L and log1p(r) may partly cancel; e = 0 and i 0 or 128, where r
= x - 1 - every value divided by |log(x)|, the magnitudes each case allows
proven by `log_consts.py` against mpmath at 600 bits, every product a
compiler may fuse modelled both ways; exact rational arithmetic for phase
3. `log_gappa.py` checks in exact arithmetic that each phase's test is
sound: the proven bound and the rounding of lo -+ err within err. The
hardest case known, Lefevre and Muller's 0x1.62a88613629b6p+678, lies
2^-118.03 relatively from a 54-bit number, far outside phase 3's bound.
NaN and +inf give themselves, +-0 gives -inf, a negative x NaN, and 1 +0.
A constant expression from C++14 through all three phases. The generator
makes the table and the digits from integers alone, by ln's series of
atanh, checking that each rounding is the same at both ends of an interval
that holds the exact value; `log_consts.py` checks them again against
mpmath. Measured (`raw/timing.txt`, gcc -O2): 18.2 ns a call over every
binade against the C library's 3.9, 19.5 over [1/2, 2] against 4.0, 16.6
near 1 against 3.5, 17.6 over [1, 1e6] against 3.8; 117 ns where phase 2
decides; 2,940 ns where phase 3 does, at 1 + 6 2^-52; 8.0 to 9.6 times
faster than the long double kernel's `log_c`; phase 1 decides 99.92% to
99.93% of ten million arguments in each of the four ranges, phase 2 the
rest, phase 3 none. The plain `d_math_d_log` stays the C library's
(S415).
- Alternative: inverses of 8 bits, so that r = m inv - 1 is one double,
  exact by one fused multiply-add or by a split of m at 45 bits, as
  CORE-MATH's logarithm has it: no exact product of two full doubles and no
  low part of r, perhaps 3 to 5 ns a call less, for a longer polynomial,
  |r| then up to 2^-7.2; a design and proofs of their own.
- Alternative: the long double kernel with a rounding test: correctly
  rounded too, at 148 to 175 ns a call, and slower where long double is 64
  bits or 128 in software.
- Alternative: phase 1 with r^2/2 rounded, not split: its rounding, 2^-61
  of log(x) where L and log1p(r) nearly cancel, is past the test's 2^-64.
- Alternative: a table of 256 triples: |r| < 2^-9 and q of degree 4, for a
  table of 6 KB.

> **Review.** Taken: for, the logarithm correctly rounded on every platform
> in double arithmetic alone, each phase's bound proven from the code's own
> operations in each of three cases, fused or not, and each phase's test
> shown sound in exact arithmetic; 8 to 10 times faster than the long double
> kernel's `log_c`; a constant expression from C++14 through all three
> phases; no 64-bit integer needed. Against: 18.2 ns a call, 4.7 times the C
> library's, where `exp_c` is 1.5 times: the exact product of m and inv,
> Dekker's where the target has no fused multiply-add, and phase 1's chain
> of three exact sums; phase 3's 2.9 us at the arguments that need it, none
> of forty million random ones; a 3 KB table; proofs the lane wrote, which
> no specialist has reviewed, their risk held by Gappa's and Sollya's
> certificates, run again in this session (`raw/proofs.txt`), and by 2,354
> references with every phase checked alone. The alternatives: inverses of 8
> bits, r one exact double; the long double kernel with a rounding test;
> phase 1 with r^2/2 rounded; a table of 256. Recommended: as taken, then
> inverses of 8 bits, so that r is exact in one double, before the plain
> double `log` follows ruling 12; an outside review of the proofs when one
> can be had. In chunk 54 (S432, S433) `log_c` became 1.70 to 1.85 times as
> fast, its results the same bits. **Flag:** `log_c` is still 1.95 to 3.03
> times the C library's time, past the twice that ruling 12's recommendation
> asks before the plain form becomes the correctly rounded one.

**S425. `log_c` is checked against references made by mpmath at 400 bits, as
`exp_c` is (S422): `math_tests_log_refs.h`, made by `tools/refs_log_c52.py`,
holds 2,048 random arguments of eight classes - every binade, the
subnormals among them, [1/2, 2], 1 +- tiny, 1 + k 2^-52 and 1 - k 2^-53,
random patterns, powers of 2 and their neighbours, the table's turns, [1,
1e6] --, 256 whose logarithm lies within 2^-66 of a rounding midpoint and
farther than 2^-99, which phase 1 cannot decide and phase 2 can, the hard
ones within 2^-99 and Lefevre and Muller's argument, and 46 edges.
`t_exponential.c` checks `log_c` exactly at each, and each phase alone: a
value phase 1 or 2 decides is the reference, and phase 3's is at every
point; phases 2 and 3 must each be reached. `self_test.h`'s
`d_math_self_test` takes the logarithm at four arguments, one for each of
phases 1 and 2 and two for phase 3, its new bit `D_MATH_SELF_TEST_LOG` 4: it
gives 7 under `-ffast-math`, by gcc and clang alike, and 0 in every tier.**
The hard ones come from `tools/hard_log_c52.c`, which takes every x = 1 + j
2^-52 and 1 - j 2^-53, j below 2^22, through log1p's first four terms in
128-bit integers: the place of a logarithm's fraction past its last bit
follows from j's square, which for j = 2^a o, o odd of a + 1 bits, falls on
a midpoint exactly, leaving the distance to j's cube, some j^3 2^(-51-b)/3
of an ulp, b the bits of j. Three lie within 2^-99: 1 - 2^-52 at 2^-105.58,
and 1 + 6 2^-52 and 1 - 12 2^-53 at 2^-100.42. 1 + 2^-52's logarithm lies
near a double, not a midpoint, and the same holds of Lefevre and Muller's
argument, so that phase 1 decides both. Phase 3 is reached at 7 of the 2,354
references; each of its values is checked at all.
- Alternative: random arguments alone: phase 3 never reached, and phase 2
  at one argument in fourteen hundred.
- Alternative: the window search of `hard_exp_c51.c`: it needs the place
  nearly linear across 2^26 arguments, as the exponential's is near 0, and
  the logarithm's near 1 is quadratic in j.

> **Review.** Taken: for, every phase reached and checked alone at every
> argument, phase 3 at seven of them, the hard ones found by a search whose
> screen mpmath checks; the references made at 400 bits by mpmath, not by
> the code's method, and kept in step by `--check`; the self-test's points
> widened to the logarithm, as S419's review recommended. Against: three
> arguments within 2^-99 of a midpoint, all within 2^-52 of 1; the proofs,
> not the references, carry the guarantee. The alternatives: random
> arguments alone; `hard_exp_c51.c`'s window search. Recommended: as taken,
> the search widened past 1's neighbourhood as time allows. Taken.

**S426. `log_c` refuses a build whose double arithmetic is not plain double,
as `exp_c` does (S418, ruling 4, B): it begins with `D_MATH_DBL_REFUSE`,
and the tests that call the double `log_c` leave it out where double is
evaluated wider, saying so: `t_exponential.c`'s random, edge, special,
overload and constant-evaluation checks of it, and `t_constants_face.cpp`'s
ln2 and ln10 against it.** Nothing else in the tree calls the double
`log_c`; the faces that use the `_c` forms - the expression language's,
the complex and quaternion faces, the constants - build and pass at
`-m32` on the x87, as they did. `tools/refusal_c52.sh` checks both ways: a
program that calls the double `exp_c` and `log_c`, a pair's sum and the
self-test is refused on the x87 with the message, by gcc, clang, g++ and
clang++ at C99, ISO strict C++98 and C++14, and builds and runs on SSE; one
that calls only the float and long double forms builds on both.
- Alternative: on the x87, `log_c` falls back to the long double kernel, as
  `expm1_c`'s far path does: no refusal for log, but two answers for one
  function, which ruling 4 refused.

> **Review.** Taken: for, no silently wrong logarithm from the double family
> on any build, the same rule as `exp_c`'s, and every test that calls it
> saying where it leaves it out. Against: an x87 build that calls the double
> `log_c` stops, where chunk 51's built with the long double kernel. The
> alternative: the long double kernel on the x87, two answers for one
> function. Recommended: as taken. Taken.

**S427. The delivery is over the owner's tree of 2026.11.16 16:00, his
repository's `7de55b67`, as chunk 51's was (S423), and holds chunks 51 and
52 together: every file of the lane's working tree that his tree has not
or holds otherwise, as S394's rule has it, with the records, `NOTE.md`,
`MANIFEST.md` and `HANDOFF.md` beside the `djinterp/` folder and
`decisions.md` and `zip_check.txt` apart.** The lane has had no word that
chunk 51's zip is in his repository; copied over the tree that holds it,
this zip changes only chunk 52's files, chunk 51's being byte for byte
what he has, and copied over his 11-16 tree, without it, it brings both.
- Alternative: chunk 52's files alone: smaller, and broken over the 11-16
  tree, where `exponential.h` would need chunk 51's `dd.h`.

> **Review.** Taken: for, one zip right over the tree the owner has, whether
> or not he has copied chunk 51's in: it changes chunk 52's files and, where
> his tree lacks them, chunk 51's, and nothing of his or other lanes'.
> Against: chunk 51's 36 files sent again. The alternative: chunk 52's files
> alone, broken over the 11-16 tree. Recommended: as taken. Taken.

### S428 to S431: chunk 53, 2026.11.20

The record: `_handoffs/math98_chunk53_2026.11.20/README.md`.

Chunk 53's decisions, S428 to S431: step 29 (5a), with 0.8's items as the
owner ruled them on 2026.11.16 - the language's dense solves refined
(ruling 8, B), the docs' equivalents of `rotmat` and `rotquat` (9a, A),
`rotquat` the nearest rotation's quaternion (9b, C), and `expression.hpp`'s
variables in a namespace of their own (10, A) - each before 0.8 on
2026.11.30. Chunk 52's decisions, S424 to S427, stand: the owner's
"continue" of 2026.11.20 at 14:23 followed its delivery.

**S428. The language's dense solves of real matrices, in float, double and
long double, are `mat.h`'s `d_mat_solve`, and of complex matrices
`cmat.h`'s `d_cmat_solve`: A's LU with partial pivoting, each of B's
columns solved by it and then refined with extra precision as LAPACK's
dgerfsx and zgerfsx refine - the residual by Dot2, the solution carried as
a pair, until the corrections converge or stop halving - each part then
rounded once, correctly rounded where A is far enough from singular, as
`la::solve`'s are (S406, S410), as ruled (8, B).** Before, the real solve
was `decomp.h`'s LU and its substitutions, and the complex
`d_cmat_solve_fast`, unrefined (S413). Exact solves keep the exact
elimination (S257) and matrices of quaternions the generic one; a singular
A is a pole, as it was. Measured (`raw/s428_solve.txt`), over 20,000
random 4 by 4 systems of doubles, entries uniform in [-1, 1], against the
exact solution in rational arithmetic: unrefined, 23.03% of the solution's
entries correctly rounded and 219 systems wholly; refined, every entry of
every system; at about nine times the time, 1,201 ns a solve against 127
(gcc -O2, the C core's kernels alone, the machine otherwise idle).
`t_expr_face.cpp`'s and `t_expr_complex_matrix_face.cpp`'s solves are
checked against `d_mat_solve`'s and `d_cmat_solve`'s bits.
- Alternative: the unrefined solve kept beside it as a second function of
  the language, `solve_fast`: speed for a user who asks, and one more name
  for 0.8 to fix, where an interpreter's user rarely feels a microsecond.

> **Review.** Taken: for, the language's dense solutions what the faces
> give, each part correctly rounded where the system is well conditioned --
> every entry of 20,000 random 4 by 4 systems, where the unrefined
> elimination rounded 23% correctly - with one kernel for the language and
> `la::solve`, so that one answer holds across them; a singular matrix a
> pole as before. Against: about nine times the unrefined solve's time on a
> small system, and the scratch of n^2 + 2n entries, 4n for a complex one;
> for an ill-conditioned system refinement stops where it no longer halves
> its corrections, so the gain is the conditioning's to allow. The
> alternative: `solve_fast` beside it. Recommended: as taken. Ruled
> 2026.11.16 (decision 8, B).

**S429. `rotquat(R)` is the quaternion of the rotation nearest R, as ruled
(9b, C): where R's elements are exact and R is exactly a rotation's matrix
-- R^T R = I and det(R) = 1 in exact arithmetic - Shepperd's method as
before, exact where the root of its largest candidate is rational and
otherwise `quat.h`'s `from_matrix3` in long double (S391); every other R,
dense or exact, by Bar-Itzhack's method (J. Guidance, Control and Dynamics
23(6), 2000): the eigenvector of the largest eigenvalue of the symmetric 4
by 4 matrix K that R's sums and differences make, K q = 3 q where R is q's
rotation, by `decomp.h`'s cyclic Jacobi to a tolerance of the type's
epsilon times K's norm, normalized, w >= 0, in the elements' widest
floating-point type, a long double where all are exact. A domain error
where det(R) is not above 0, a reflection or no rotation, and where R lies
farther than 1/2 from the rotation found, in the Frobenius norm, far from
any rotation (a rotation's matrix is of norm sqrt(3)): s I is taken for
|s - 1| up to 1/(2 sqrt(3)), about 0.289; NaNs for a NaN or infinite
element. The docs - the README, `rotation.hpp`'s banner, the function
table and `comparison.md` - list MATLAB's equivalents, `rotmat(q,
'point')` and `quaternion(R, 'rotmat', 'point')`, with the Robotics
toolbox's `quat2rotm` and `rotm2quat`, and SciPy's, `Rotation.as_matrix`
and `Rotation.from_matrix`, as ruled (9a, A).** Bar-Itzhack's eigenvector
is the Frobenius-nearest rotation's quaternion, the polar factor's, with
no SVD and no sign to fix; the gap between K's largest eigenvalue and the
next is 4 for an exact rotation, so the eigenvector is accurate to some
epsilons. A dense rotation's matrix now gives Bar-Itzhack's bits rather
than `from_matrix3`'s: `t_expr_quaternion_matrix_face.cpp` checks them
against the test's own computation bit for bit and within 32 epsilons of
the generating quaternion, in float, double and long double; a drifted
30-degree turn in double; an exact shear, [1, 1/1000, 0; 0, 1, 0; 0, 0,
1], in long double, the rotation by -atan(1/2000) about z within 8
epsilons; 1.1 I the identity's; reflections exact and dense, the zero
matrix, 2 I and 1.3 I domain errors; and a NaN NaNs.
- Alternative: a strict threshold, a few hundred epsilons: a matrix that
  is no rotation's refused early; and refused too where products of many
  rotations have drifted, the case the ruling's "far from any rotation"
  spares, against the lane's promise of 2026.11.15 of a generous one.
- Alternative: the polar factor by an SVD, then Shepperd's method: the
  same rotation by two kernels and a determinant's sign to correct, where
  K's eigenvector gives it in one.
- Alternative: Markley's method, SciPy's: the best-conditioned of four
  candidate columns, faster, and not the nearest rotation's in general.

> **Review.** Taken: for, `rotquat` meaningful for every matrix a user is
> likely to have - a rotation's, exact or rounded, or one drifted through
> many products - by the nearest rotation, the answer the ruling asked, in
> one kernel the C core has, its accuracy set by K's eigenvalue gap of 4; an
> exact rotation's exact answer kept; a reflection or a matrix far from any
> rotation an error instead of a quaternion that means nothing; MATLAB's and
> SciPy's names in the docs, so a user arriving from either finds it.
> Against: a dense rotation's quaternion no longer `from_matrix3`'s bits, so
> `rotquat` and `quat.h`'s kernel differ in the last bits where they agreed;
> a cyclic Jacobi on a 4 by 4 per call, dearer than Shepperd's few
> operations; 1/2 is the lane's choice of "far", generous as promised, and
> an exact matrix that is no rotation's but near one now gives a long
> double, not an error. The alternatives: a strict threshold; the polar
> factor by an SVD; Markley's method. Recommended: as taken. Ruled
> 2026.11.16 (decisions 9a, A, and 9b, C).

**S430. `expression.hpp`, the compile-time core, compiles from C++11 and is
empty below it (N1), its kernels the C core's (N3), its placeholders in a
namespace of their own (S403; ruling 10, A): step 29 (5a). From C++11 its
leaves, nodes and operators, `squared`, `abs` and `dot` are constant
expressions - each function one return, each return type spelled, a
variable's argument picked by a small recursive template where `std::get`
is not yet constexpr; `exp`, `sin`, `cos`, `tan` and `sqrt` are
`d_math_exp_c`, `d_math_sin_c`, `d_math_cos_c`, `d_math_tan_c` and
`d_math_sqrt_c` at compile time and at run time alike, constant expressions
from C++14, where the C core's are; `abs` is exact, of -0 +0; `x`, `y`,
`z`, `t`, `r`, `rho`, `theta` and `phi` are in `djinterp::math::variables`,
brought in by `using namespace djinterp::math::variables;`, and `var<I,
T>` stays in `djinterp::math`, from C++14.** The header's own kernels gave
(`raw/old_kernels.txt`) `sqrt(1e300)` as 7.89e269 and `sqrt(-1)` as 0;
`exp` up to 13,408 ulps from the C library's over two million points in
[-700, 700]; `sin` up to 1.2 times 10^7 ulps over [-100, 100] and a NaN at
1e22; and from C++20 a program had the C library's answer at run time and
theirs in a constant expression. Now one call has one answer at every
level: the double `exp` correctly rounded (S416), `sqrt` correctly rounded,
`sin`, `cos` and `tan` the C core's until 6a rewrites them. `math.hpp`,
which reads every header, gives no `-Wshadow` warning at C++14, 17 or 20,
where it gave 31 at 26 places, all of `function/` and `geometry/`. Its
users follow: `calculus/elementary.hpp`'s arctangent takes `d_math_sqrt_c`
where it took the header's `csqrt`, its log and atan its own until step 31;
the owner's standalone tests, `test.cpp`, `test_calc.cpp`,
`test_constants_values.cpp` and `test_functional.cpp`, bring the variables
in and pass at C++14, 17 and 20 (`test_functional.cpp` from C++17, as
before). The ladder passes `expression.hpp` at every rung, gcc and clang,
and names its line of the punch list to go; the eight units that read it
and fail at C++11 and below fail at their own code now, for steps 30, 31
and 33. On the x87 a program that calls the core's `exp` is refused at
compile time, as `exp_c` is (S418), where it was 13,408 ulps off. A new
face, `t_expression_face.cpp`, checks it at every rung: the nodes folded
at compile time from C++11, the placeholders' indices, a function of
`djinterp::math` naming locals `x` to `phi` under `-Wshadow -Werror`, the
five functions the C core's bits over twenty arguments from -745.25 to
1e300, `abs`, and from C++14 each function folded at compile time with the
run time's bits.
- Alternative: the C library at run time and the `_c` kernel in a
  constant expression, as before but on the C core's kernels: the library's
  speed, and two answers to one call wherever the two differ.
- Alternative: the placeholders kept in `djinterp::math` and the 26
  locals renamed: no user's code to change, and every future local of
  those names a warning again.

> **Review.** Taken: for, the compile-time core at the floor the owner ruled
> (N1), its nodes constant expressions from C++11, and one answer at every
> level for its five functions, each the C core's - a correctly rounded
> `exp` and `sqrt` where they were 13,408 ulps and 10^119 times off - with
> nothing of its own left to drift; its placeholders out of
> `djinterp::math`, so that `math.hpp` is clean under `-Wshadow` and stays
> so; the ladder's punch list one line shorter. Against: a user's code that
> named `x` to `phi` from `djinterp::math` needs one using-directive; the
> five functions are run-time calls at C++11, constant expressions only from
> C++14; the `_c` kernels' time where the C library's was taken at run time;
> on the x87 a program calling the core's `exp` no longer builds; the other
> compile-time headers still fail below C++14 until steps 30, 31 and 33. The
> alternatives: the library at run time and the kernel in a constant
> expression; the placeholders kept and the locals renamed. Recommended: as
> taken (N1, N3). Taken; the placeholders' namespace ruled 2026.11.16
> (decision 10, A).

**S431. The delivery is over the owner's tree of 2026.11.16 16:00,
`7de55b67`, as chunks 51's and 52's were (S423, S427), and holds chunks 51,
52 and 53 together: every file of the lane's working tree that his tree
has not or holds otherwise (S394), with the records, `NOTE.md`,
`MANIFEST.md` and `HANDOFF.md` beside the `djinterp/` folder and
`decisions.md` and `zip_check.txt` apart.** The lane has had no word that
chunk 51's or 52's zip is in his repository; copied over a tree that holds
them, this zip changes only chunk 53's files, and over his 11-16 tree it
brings all three. The ladder's punch list, his shared file, is not in it:
the note asks for `expression.hpp`'s line to go.
- Alternative: chunk 53's files alone: smaller, and broken over the 11-16
  tree, where the double `exp_c` the core now calls is chunk 51's.

> **Review.** Taken: for, one zip right over the tree the owner has, whether
> or not he has copied chunks 51 and 52 in; nothing of his or other lanes'
> changed, the punch list's line asked to go. Against: chunks 51's and 52's
> files sent a third and a second time. The alternative: chunk 53's files
> alone, broken over the 11-16 tree. Recommended: as taken. Taken.

### S432 to S434: chunk 54, 2026.11.21

The record: `_handoffs/math98_chunk54_2026.11.21/README.md`.

Chunk 54's decisions, S432 to S434: `log_c`'s speed, the next step of 6a
that S424's flag asks before the plain double `log` follows ruling 12 in
early December. Chunk 53's decisions, S428 to S431, stand: the owner's
"continue" of 2026.11.21 at 16:01 followed its delivery.

**S432. `log_c`'s reduction is cheaper and gives the same bits: the
table's 127 inverses that are not 1 are rounded to 26 significant bits,
within 2^-26 of 1/c, where they were doubles within 2^-53, and from i = 53
the table holds the inverse halved, so that m is never halved and e is
raised by an integer test; m inv is exact as a pair by Dekker's product
with m alone split, the inverse whole where its split would leave it so,
or by a fused multiply-add where the target has one; and every positive
normal x is taken in one test, x >= DBL_MIN and x <= DBL_MAX, the rest --
NaN, infinities, zeros, negatives, subnormals, the last lifted by 2^54 as
before - after it; log(1) is +0 as the phases give it, with no test of its
own.** Every bound chunk 52 proved holds again (S424): |r| below 2^-8 at
every i, at most 2^-8.011 from i = 1, as the slack between a c's
neighbourhood and 2^-8 takes an inverse's error of 2^-27; the table's pairs
within 2^-107.06 of -log(inv); each stored inverse of at most 26 bits, so
that the product's split of m alone is exact (proofs/log_consts.py). The
halving was the slowest part, a branch on i that a random argument's m
mispredicts half the time; with the product's two splits, it was
eleven of chunk 52's 20 ns a call. Measured (`raw/timing.txt`, `raw/
identical.txt`): with S433, 151 instructions a call where chunk 52's took
206 and the C library's log 66, and no mispredicted branch where chunk 52's
had half of one; 12.2 ns a call where chunk 52's took 22.2, the C
library's 5.3, over arguments from e^-690 to e^690 (gcc -O2), 1.70 to 1.85
times as fast as chunk 52's in each of nine builds and sets, 1.95 to 3.03
times the library's time, the machine otherwise idle, its times varying by
a fifth from run to run where their ratio to chunk 52's does not; and its
results chunk 52's, bit for bit, at 26,214,412 arguments, by gcc with and
without a fused multiply-add.
- Alternative: inverses of 8 bits, as S424's review named, so that r is one
  exact double: |r| then passes 2^-8, every polynomial and bound of phases 1
  to 3 to be made and proven again, for a saving of the low part's few
  operations, where 26 bits give the product's saving with every bound kept.
- Alternative: a table of 256, |r| below 2^-9: phase 1's polynomial of
  degree 5, at a table of 6 KB and every bound proven again.

> **Review.** Taken: for, the halving's mispredicted branch gone and the
> product's split on m alone, the two dearest parts of chunk 52's reduction;
> every bound of chunk 52's proofs kept, `log_consts.py` checking the new
> table against them, so that phases 2 and 3 stand as proven; the same bits
> as chunk 52's at 26 million arguments, with and without a fused
> multiply-add; one test for every ordinary argument. Against: the table's
> inverses no longer the doubles nearest 1/c, which a reader of the table
> may not expect; the subnormal path a separate call of the reduction. The
> alternatives: inverses of 8 bits; a table of 256. Recommended: as taken.
> Taken.

**S433. `log_c`'s first phase sums rh^2 q(rh), q of degree 6 with q(0) =
-1/2 (Sollya's, within 2^-65.05 of (log1p(r) - r)/r^2), where it split rh^2
exactly into a third exact sum; and it decides where the result is within
2^-90 of itself plus 11 2^-55 rh^2 of the logarithm, where it tested against
2^-64 of the result: its error is of the order of 2^-53 rh^2, from the
rounding of rh^2 q, and of 2^-93 of the result besides, so that the test
grows with rh as the error does.** Proven in each of S424's three cases
(proofs/): Gappa bounds the error's two parts, Z, of every part but rh^2
q's, within 2^-93.18 of |log(x)|, and W, of rh^2 q's roundings, within
2^-51.00 of it, checking as a rewriting that the computed sum less the exact
one is Z + rh^2 q W; exact arithmetic checks the test sound at both ends of
rh^2/|log(x)|'s interval in each case, the condition being linear in it.
Phase 1 is within 2^-66.45 of the logarithm where e is not 0 and 2^-59.99
at worst, where rh^2 is largest against it. Over two million arguments of
each set (`raw/identical.txt`), it leaves undecided 0.39% of those within
2^-7 of 1, more than chunk 52's 0.07%, the price of the cheaper sum; 0.01%
of those over [0.5, 2], where chunk 52's left 0.07%; and none from e^-690
to e^690, where chunk 52's left 0.0002%. `t_exponential.c`'s
near-midpoint references, which chunk 52's phase 1 left all to phase 2, it
decides 245 of 256, its error far below 2^-64 where rh is small. The rest of
the phase is chunk 52's: e LN2H and L's high part, then rh, by Fast2Sum, rl
(1 - rh), the low parts summed, rh^2 q added last. A test that chose its
room by whether e is 0, a first form, gcc compiled to a branch whichever
way it was written, mispredicted half the time over [0.5, 2] (cachegrind);
the uniform test has no branch.
- Alternative: chunk 52's phase 1 with S432's reduction: its bounds kept as
  proven, at some 15 ns a call, where S433 gives 10.
- Alternative: the room chosen by e's being 0, 2^-66 or 2^-60 of the result:
  a select that compilers make a branch of, and 1.2% undecided near 1.
- Alternative: rh^2 split exactly, as chunk 52 had it, near 1 too: 0.07%
  undecided there, at some 2 ns a call everywhere.

> **Review.** Taken: for, phase 1 some 4 ns cheaper, its test no longer a
> fixed fraction but the error's own shape, so that it decides far more
> where rh is small and as much as before where it is large, with no branch;
> proven by splitting its error in two and checking the test at both ends of
> the one quantity it grows with; the near-midpoint references now mostly
> decided in phase 1. Against: arguments within 2^-7 of 1 go to phase 2, 89
> ns, five times as often as before, 0.39% of them; a proof shape new to the
> lane's logarithm, Z and W bounded apart and joined in exact arithmetic,
> which no specialist has reviewed; the test's two constants, 2^-90 and 11
> 2^-55, chosen with some 9% to spare at its tightest. The alternatives:
> chunk 52's phase 1 kept; the room chosen by e; rh^2 split exactly.
> Recommended: as taken, with an outside review of the proofs when one can
> be had. Taken.

**S434. The delivery is over the owner's tree of 2026.11.16 16:00,
`7de55b67`, as chunks 51's to 53's were (S423, S427, S431), and holds chunks
51 to 54 together: every file of the lane's working tree that his tree has
not or holds otherwise (S394), with the records, `NOTE.md`, `MANIFEST.md`
and `HANDOFF.md` beside the `djinterp/` folder and `decisions.md` and
`zip_check.txt` apart.** The lane has had no word that any of chunks 51 to
53's zips is in his repository; copied over a tree that holds them, this
zip changes only chunk 54's files.
- Alternative: chunk 54's files alone: smaller, and broken over the 11-16
  tree, where `exponential.h`'s double family is chunk 51's.

> **Review.** Taken: for, one zip right over the tree the owner has, whether
> or not he has copied chunks 51 to 53 in; nothing of his or other lanes'
> changed. Against: chunks 51's to 53's files sent again. The alternative:
> chunk 54's files alone, broken over the 11-16 tree. Recommended: as taken.
> Taken.

### S435 to S440: chunk 55, 2026.11.21

The record: `_handoffs/math98_chunk55_2026.11.21/README.md`.

Chunk 55's decisions, S435 to S440: math's largest files split, as the
owner ruled on 2026.11.21 - no modules of math's own, and no file past
about 4,000 lines, each old name kept; the deeper modularization not now;
the other modules left to their own keepers. Chunk 54's decisions, S432
to S434, stand: the owner's "continue" of 2026.11.21 at 19:16 followed its
delivery. His question of 2026.11.19, whether to split math into
modules, is answered by the same ruling.

**S435. The generator aligns each parameter list it writes one parameter a
line, its names in one column one space past its longest type, as the
owner aligned `cmat.h`'s float family by hand on 2026.11.21; and every
generated header's banner carries the converted tag, `[c][math]`.** The
templates align their lists for the placeholders, so each family's type
and suffix shifted them: 1,548 lines of the twenty headers were out of
column, in every family, where the owner's edit fixed eight. A list holding
a comment, a function pointer or a default is left as it is. The tag is
the one `banner_tags.py` gives the hand-written files, which skips the
generated ones. Only whitespace and the tag change: `diff -w` shows the
tag lines alone. His edit dropped `cmat.h`'s last newline, which every
generated header has; it comes back.
- Alternative: his eight lines kept by hand in the generated file: the
  generator's `--check` failing from then on, and the other families
  still out of column.

> **Review.** Taken: for, the generator in step with the owner's edit and
> his column rule kept in every family, so that no generated list is out of
> column and `--check` passes again; the converted tag in every banner, as
> in the hand-written files. Against: 1,548 lines of whitespace changed in
> the generated headers, which a reader of a diff must pass over. The
> alternative: his lines by hand, the check failing. Recommended: as taken.
> Taken.

**S436. The eight largest generated headers - `decomp.h`, 25,732 lines;
`cmat.h`, 15,827; `cplx.h`, 13,242; `quat.h`, 8,809; `vec.h`, 7,959;
`coord.h`, 5,380; `mat.h`, 4,942; `exponential.h`, 4,670 - are written as
44 parts of 3,989 lines and fewer, each a header that stands alone, named
for its header and what it holds: a family's suffix (`decomp_f.h`), a part
of a family's section (`decomp_f_eigen_c.h`, `decomp_f_eigen.h`), the
kernels the families share (`quat_kernels.h`), the C++ names
(`decomp_cpp.h`). The header of each name keeps its banner and contents,
lists its parts and includes them in order.** The generator writes each
header's text as before and cuts it at section headings (`SPLITS`): a part
holds the header's own includes, the parts before it whose names it uses,
its slice inside the no-fusing pragma as the whole header was, and gcc's
`-Warray-bounds` set aside in the part that sets it aside and again in the
part a section continues into. So an includer of the header sees the same
code in the same order: preprocessed, each of the eight gives the same
text by gcc at C99, GNU C11 with a fused multiply-add and C++98 and C++14,
and by clang at C11, the pragmas apart, but for C++98's static assertions,
whose names carry their line. Every part compiles alone at six rungs,
warnings as errors. No includer changes. The proofs of `log_c` read
`exponential_d.h`, which holds section 3 whole.
- Alternative: parts by algorithm across the families (`decomp_lu.h`
  holding all three LUs): a reader's grouping, but the families' order
  changed, so the preprocessed text no longer the same, and `decomp.h`'s
  general eigensolver, 6,674 lines a family, still to be cut within a
  family.
- Alternative: the headers kept whole and the parts made by hand: the
  generator's one text lost.

> **Review.** Taken: for, no generated header past 3,989 lines, each part a
> header that stands alone and a reader's unit, a family or a part of one;
> the header of each name the same to every includer, the preprocessed text
> the same, which is the strongest check a move of code can have; one text
> kept in the generator, cut where its sections begin. Against: 44 more
> headers, each with the original's includes again; a reader looking for one
> algorithm across the families opens three parts, not one; the ladder has
> 44 more units to compile. The alternatives: parts by algorithm; parts by
> hand. Recommended: as taken. Taken.

**S437. `expr/matrix_kind.hpp`, 4,822 lines, is three headers that stand
alone - `matrix_kind_storage.hpp`, sections 1 to 3.1, the shape and the
kind's storage, entries and table, and making and reading values;
`matrix_kind_arithmetic.hpp`, section 3.2, 3,250 lines; and
`matrix_kind_transpose.hpp`, sections 3.3 and 3.4 - and `matrix_kind.hpp`
keeps its banner and contents and includes them in order.** The kind's
table names two functions section 3.2 defines, `expr_matrix_negated` and
`expr_matrix_operation`, so the storage part includes the arithmetic after
its own text, and each part compiles alone, from C++98 (empty) to C++20,
warnings as errors and `-Wshadow`. Preprocessed, the header gives the
same text, but for the namespaces closed and opened again between parts.
Each part's banner carries HOW TO USE, OPTIONS and CONFIGURATION. As
S400 and S401 ask of every header of `expr/`, `expr.hpp` includes the
three parts, and the face test includes each and checks its guard.
- Alternative: sections 1 to 3.2 in one part: 4,400 lines, past the
  ruling's limit.

> **Review.** Taken: for, the language's largest header in three of its own
> concepts, each standing alone, the same text to every includer but the
> namespaces reopened. Against: the storage part includes the arithmetic
> after it, since the kind's table names two of its functions: a part that
> pulls in the one after it, which a reader may not expect. The alternative:
> sections 1 to 3.2 together, past the limit. Recommended: as taken. Taken.

**S438. The face test `t_expr_face.cpp`, 11,685 lines, is one program in
eight parts of 3,054 lines and fewer, `t_expr_face_*.hpp` beside it - the
helpers, the types, parsing, the rewrites, three of matrices and the
handle - each including the one before it; the test keeps its banner, the
module's driver and main, and includes them in order.** It gives the same
checks, 520 from C++11 and 1 below, by g++ and clang++. The parts share
the test's anonymous namespace, reopened in each.
- Alternative: one test per part: eight programs, the runner's list and
  every tier's counts changed.

> **Review.** Taken: for, the face test in parts a reader can open, one
> program and the same checks, so that no tier's count moves. Against: parts
> that are not programs, each only one piece of the test, which compile only
> in it. The alternative: eight programs. Recommended: as taken. Taken.

**S439. The generator, `scripts/gen_c_math_float.py`, 32,498 lines, is a
package, `scripts/gen_c_math/`, of 26 modules of 3,741 lines and fewer --
`common` for what every header's text shares, one module for a header's
text or two (`decomp_text` and `decomp`, `cplx_text` and `cplx`, where one
module would pass the limit), `split` for S436's parts, `driver` for the
outputs and main - and the script is its entry point, its usage the
same.** Each top-level statement went, with the comments above it, to the
module of the section it stood in, and each module imports by name what it
uses of the others', in an order without a cycle. It writes the same
bytes: `--check` passes on the headers the single script wrote, and
written afresh they are the same files. The script writes no bytecode
into the package (`sys.dont_write_bytecode`).
- Alternative: the script kept whole: 32,000 lines, eight times the limit.

> **Review.** Taken: for, the generator in 26 modules, each a header's text
> or two, writing the same bytes, its usage the same; no bytecode written.
> Against: a module's names imported from the others by name, a list to keep
> when a template moves; the two halves of `decomp`'s and `cplx`'s texts cut
> where their sizes balance, not at a concept. The alternative: the script
> whole. Recommended: as taken. Taken.

**S440. The delivery is over the owner's main of 2026.11.21, `4c536f19`,
which holds chunk 51 as he committed it, with chunk 52's zip laid over it,
as the merge window asks of a zip cut before Monday's push: it carries the
files chunks 53 to 55 changed or made, and no record, the records staying
with the lane's own deliveries. Chunks 52 to 54 are laid three ways over
his tree's converted banners and the window's rewordings, and the lane's
decision texts are read with the window's rewordings (chunk 51's two lines)
and the lane-name check's (a line each of chunks 52's and 53's; a review's
line in the reviews), so that `decisions.md` adds no line the check
refuses; and the two files of chunks 52 and 53 his main had not yet,
`math_tests_log_refs.h` and `t_expression_face.cpp`, have their banner's
tag converted by his `banner_tags.py`, every zip being cut on converted
banners.** Chunk 52 lands on Monday as the window has it; this zip is for
Wednesday's window. The earlier records keep their words; the tool rewords
as it reads. The reference tools of chunks 51 and 52 write the old tag, so
their own `--check` differs from his main in that line;
`refs_tags_c55.sh` makes the references again and converts the tag
before it compares.
- Alternative: chunk 54's form, over the 11-16 tree with every chunk since
  51 and the records: refused by the window, which keeps the records out
  of main and lands chunk 52 first.

> **Review.** Taken: for, the zip the window can lay over Monday's main,
> chunk 52 landing first as it planned, no record in the tree; the decisions
> file passing the lane-name check with the earlier records' words kept.
> Against: the zip cut on a main with chunk 52's zip laid over it, not on
> the main Monday's push will name, so the window lays it three ways. The
> alternative: chunk 54's form, refused. Recommended: as taken. Taken.

### S441 to S446: chunk 56, 2026.11.22

The record: `_handoffs/math98_chunk56_2026.11.22/README.md`.

Chunk 56's decisions, S441 to S446: step 30 (5b), `function/function.hpp`,
`values.hpp` and `functional.hpp` from C++11, as the owner's N1 asks and as
chunk 53 brought `expression.hpp`. Chunk 55's decisions, S435 to S440, are
open: the owner's "continue for now" of 2026.11.22 at 20:40 went on while he
weighs them, with rulings 12 to 15. Nothing here rests on them but the
zip's base (S446) and the three documents both chunks change.

**S441. `function/function.hpp`, `values.hpp` and `functional.hpp` compile
at every level, each empty below C++11, as `expression.hpp` is (N1,
S430).** Each includes the framework's root first and, from C++11 only, the
standard headers and the core. Below C++11 the forms have nothing to stand
on: their nodes are `expression.hpp`'s, empty there, and the functional
subframework the bridge adapts to is empty below C++11 too. Each was on
the ladder's punch list at C++98 and C++11, failing at `<array>`'s warning
and at `std::index_sequence`; each now passes at every rung, plain and
testing, with g++ and clang++.
- Alternative: C++98 forms of the three: forms over a core that has none,
  where the owner's ruling is a C++11 floor for the expression engine and
  what stands on it (N1).

> **Review.** Taken: for, the three headers on the ladder at every rung,
> each as the core it stands on, one floor for the compile-time face, as
> ruled. Against: a C++98 program that includes them gets nothing, not an
> error saying why; `math.hpp` still includes the calculus headers, which
> fail below C++14, until steps 31 and 33. The alternative: C++98 forms over
> a core that has none. Recommended: as taken. Taken.

**S442. From C++11 each constexpr function of the three is one return, so
that each form is a constant expression wherever its operands' evaluation
is; the index sequences are re_std's; the traits' `_v` forms are from
C++14, where variable templates are; and `void_t` names a member of a
class template.** The relations with a scalar on one side, `when`,
`otherwise` and `level_set` built their result from a local; `pw_run`
chose its branch by `if`; `implicit_node`'s `contains` and `sign` named the
residual, and now hand it to a static helper, so that it is still
evaluated once. `std::index_sequence` is C++14's; re_std's is C++11's.
`D_DETECT_MARKER` defined each `_v` at every level; `D_DETECT_MARKER_V`
defines it where variable templates exist, as `is_coordinate_system_v`
already was. A bare alias `void_t` is not a substitution failure under
every compiler of C++11 (CWG 1558); through `make_void` it is. The
relations, the logic, implicit functions, level sets and the axis sets are
constant expressions from C++11; the forms over `std::tuple`, piecewise,
vector-valued and parametric, and `math_function`'s calls from a
`std::array` or a `std::tuple`, are by the standard from C++14, where
`std::get` and `std::array`'s `const` `operator[]` are constexpr, and from
C++11 with libstdc++, which marks them so, as both compilers' builds here
use it. The face test asserts the standard's levels.
- Alternative: a tuple of the lane's own, constant-evaluable at C++11:
  `pieces_type` and `components_type`, which are public, would change, for
  C++11 with a library other than libstdc++ alone.
- Alternative: `contains` and `sign` evaluating the expression twice in
  one return: each call twice the work.

> **Review.** Taken: for, each form a constant expression from the first
> level its operands allow, the residual evaluated once; the `_v` forms
> where the language has them; detection right under every compiler of
> C++11. Against: the forms over `std::tuple` constant expressions at C++11
> only with libstdc++, at run time with another library until C++14. The
> alternatives: a tuple of the lane's own, public types changing; the
> residual evaluated twice. Recommended: as taken. Taken.

**S443. `math_function` takes a coordinate system's point, a
`linalg::vector`, beside a scalar pack, a `std::array` and a
`std::tuple`.** Its banner said that a point, "just the system's
point_type, a std::array", evaluated directly; but every system's
`point_type` is a `linalg::vector`, as ruled on 2026.10.05, and the
scalar-pack overload took it as the one argument:
`function(cartesian<3>()).returns(x + y * z)(p)` failed to compile,
`variable_node` asking for more arguments. An overload explodes the vector
by its constexpr `operator[]`, and `first_is_container` keeps the scalar
pack from it. The call is a constant expression from C++14, where the
vector's constructors are.
- Alternative: the banner corrected alone: the builder's documented use
  still a compile error, a point taken apart by hand at each call.

> **Review.** Taken: for, the builder's documented use working, a point
> evaluated as the banner says, a constant expression from C++14. Against:
> one more overload and one more trait beside the array's and the tuple's.
> The alternative: the banner corrected alone. Recommended: as taken. Taken.

**S444. `sample_over<I>(e)` evaluates `e` at each of the interval's
members, `I::at(0)` to `I::at(I::size() - 1)`, where it evaluated at
`lower_bound + i * step`.** For an interval open below that took the bound,
which is no member, and lost the last member: (0, 4] gave 0, 1, 2 and 3,
where its members are 1 to 4; (0, 6] by 2 gave 0, 2 and 4, where they are
1, 3 and 5; (-3, 3) gave -3 to 1, where they are -2 to 2. A closed
interval's samples are as they were. `sample_over` is from C++14, where the
interval's `size()` and `at()` are constant expressions, the C core's
count being one there; `sample` is from C++11.
- Alternative: the first member and the step, `first() + i * step`: the
  same points, by a second rule beside the interval's own, which `at` is.

> **Review.** Taken: for, every sample a member and their count the
> interval's, by the interval's own rule; a closed interval's samples
> unchanged. Against: a change for an interval open below, which a caller
> relying on the bound would see; none in the tree does, the owner's
> standalone test's interval being closed. The alternative: the first member
> and the step. Recommended: as taken. Taken.

**S445. Three face tests: `t_function_face.cpp`, 22 checks from C++14 and
19 at C++11; `t_values_face.cpp`, 9 and 5; `t_functional_face.cpp`, 7; each
1 check below C++11, that the header reads and is empty. Each passes its
64 builds of the full tier and its 4 of the fused.** Each asserts at
compile time what is a constant expression at its level, as
`t_expression_face.cpp` does, and checks at run time what is not. The
bridge's test uses the functional subframework itself, `compose`, `pipe`,
`curry`, `flip` and `pipeline_from`, where `test_functional.cpp`, one of
the owner's standalone tests, which the runner does not build, used
stand-ins, saying that the subframework could not be compiled; it compiles
from C++11. The owner's standalone tests, `test.cpp`,
`test_functional.cpp`, `test_calc.cpp` and `test_constants_values.cpp`,
are left as they were, and pass as before.
- Alternative: one face test for the three: a failure in one header
  stopping the others' checks, where the runner has a test a header
  elsewhere.

> **Review.** Taken: for, each header checked at every rung, the bridge
> against the subframework itself, so that a change there which breaks the
> adapters shows. Against: the owner's standalone tests left beside the face
> tests, one of them saying what is no longer so. The alternative: one test
> for the three. Recommended: as taken. Taken.

**S446. Chunk 56's zip is cut over the owner's main of 2026.11.21 with
chunks 52's and 55's zips laid over it, as the merge window's relay of
2026.11.21 asks of a zip cut before the push that would land them, and
holds chunk 56's files alone. Today's note, which chunk 55's record began,
goes on in chunk 56's, and the day's last zip carries both chunks' zips in
place of the afternoon's.** Chunk 55's zip is for Wednesday's window, chunk
56's for Friday's: should Wednesday land chunk 55 other than as its zip,
chunk 56 is cut again over that commit by Thursday at 20:00. The ladder
now passes `function.hpp`, `functional.hpp` and `values.hpp` everywhere,
as it has `expression.hpp` since chunk 53: the note asks for their four
lines of `scripts/ci/ladder_known.txt`, his shared file, to go.
- Alternative: one zip of chunks 53 to 56 over main with chunk 52's laid
  over it, in place of chunk 55's: Wednesday's window given a zip that
  changes again before Friday's.

> **Review.** Taken: for, each window its own zip, Wednesday's unchanged;
> the punch list's lines named for the owner to drop. Against: chunk 56's
> zip right only over chunk 55's: laid alone over main, its three documents
> would bring chunk 55's text without its files. The alternative: one zip of
> chunks 53 to 56 in place of chunk 55's. Recommended: as taken. Taken.

### S447 to S449: chunk 57, 2026.11.23

The record: `_handoffs/math98_chunk57_2026.11.23/README.md`.

Chunk 57's decisions, S447 to S449: chunks 55's and 56's zips cut again on
the owner's push of 2026.11.23, `6ee9dce1`, which holds chunk 52 as he
committed it and gives the tree's prose one hyphen for a dash. No C or C++
token changed. Chunks 55's and 56's decisions are still open, with his
rulings 12 to 15.

**S447. Wednesday's zip is cut again over the owner's main of 2026.11.23,
`6ee9dce1`, and holds the files chunks 53 to 55 changed or made, 123 of
them, 83 new: one fewer than on 2026.11.21, since main now has
`math_tests_log_refs.h` as chunk 55 had it. Friday's zip is cut over that
main with Wednesday's zip laid over it, and holds chunk 56's 10 files, 3
new, its documents with this chunk's lines. Every file is laid three ways
over his push, his fix for double hyphens is run over the lane's files
before the cut, and his push's rewording of a line of chunk 52's, "is on
the owner's", is kept.** Both zips pass his merge window's gates, laid in
order over his push, and the lane-name check, each over the tree it goes
on.
- Alternative: the zips of 2026.11.21 and 2026.11.22 as sent, over his
  main of 2026.11.21: no longer the main the window starts from, and their
  double hyphens refused by its new gate.

> **Review.** Taken: for, each zip right over the main the window starts
> from, its gates run before it is sent. Against: two zips sent again, the
> same files but for their dashes and the banner chunk 52's landing changed.
> The alternative: the zips as sent, which the window stops. Recommended: as
> taken. Taken.

**S448. The generator's strings turn with the prose: a double hyphen in the
comment text it writes is one hyphen, a dash that ends or starts one of its
string lines too, since it rewraps some paragraphs and would join the two
halves into a double hyphen. Its 64 headers have the tokens of the headers
the owner's fix turns in place, each checked: 36 the same byte for byte, 28
differing in their comments' line breaks alone, where a rewrapped line is a
character shorter. Its `--check` holds on his main again, which the pass,
turning the headers and not the generator, had made fail in 19 headers.**
The tree's other strings and code keep their double hyphens, as the pass
asks; math's files have none left.
- Alternative: the generator as it was and the turned headers kept: its
  `--check` failing on main, and its next run bringing every double hyphen
  back into their comments.

> **Review.** Taken: for, the generator and its headers agreeing again, the
> gate passing on what it writes. Against: 28 headers' comments wrapped
> otherwise than his pass left them. The alternative: the generator as it
> was, which fails its check. Recommended: as taken. Taken.

**S449. Each zip holds its files at their paths under the tree's root,
`inc/`, `scripts/`, `tests/`, as the owner's `merge_window.py` reads a
lane's zip, and nothing beside them; chunks 55's and 56's put them under a
`djinterp/` folder, which the window reads as a top level that is not the
tree's root.** The notes, manifests and records go beside the zips, as
before (S440).
- Alternative: the `djinterp/` folder of chunks 55 and 56, unwrapped by
  hand before the window can read the zip.

> **Review.** Taken: for, the zip the window reads as it is, the gates run
> on exactly what is sent. Against: a layout unlike the lane's earlier zips.
> The alternative: the folder, unwrapped by hand. Recommended: as taken.
> Taken.

### S450 to S455: chunk 58, 2026.11.23

The record: `_handoffs/math98_chunk58_2026.11.23/README.md`.

Chunk 58's decisions, S450 to S455: step 31 (5c), the calculus headers
from C++11, as the owner's N1 asks and as chunks 53 and 56 brought the
compile-time core and the forms on it; and with it step 33 (5e), since
`math.hpp`, which includes them, then compiles at every rung. Chunks 55's
to 57's decisions are open, with the owner's rulings 12 to 15; nothing here
rests on them but the zip's base (S455).

**S450. The six calculus headers, `calculus.hpp`, `differentiation.hpp`,
`elementary.hpp`, `integration.hpp`, `sequence.hpp` and `series.hpp`,
compile at every level, each empty below C++11, as `expression.hpp` is
(N1, S430); `math.hpp`, which includes them, compiles at every level with
them, so that step 33 (5e) is done with step 31 (5c), step 32 (5d) still to
come.** Below C++11 the umbrella brings the constants alone. The ladder,
which failed all seven at C++98, strict C++98 and C++11, now passes them
at every rung, plain and testing, with g++ and clang++: their seven lines of
`scripts/ci/ladder_known.txt`, the owner's shared file, can go, and with the
four the notes since 11-20 ask for, the punch list has no math line.
- Alternative: steps 31 and 33 apart, `math.hpp` taken with step 33 in a
  chunk of its own: a chunk with no change of its own to make.

> **Review.** Taken: for, every calculus header and math.hpp at every rung,
> the punch list's last math lines free to go, step 33 taken where it costs
> nothing. Against: steps 31 and 33 done before 32, out of the roadmap's
> order. The alternative: math.hpp in a chunk of its own, with nothing of
> its own to change. Recommended: as taken. Taken.

**S451. A form is a constant expression from C++11 where it is one return:
each derivative's tree, the higher derivatives and the gradient, the 5-point
rule, the Taylor and Maclaurin evaluations, the arithmetic sequence and
sums, and the scalar helpers abs to degrees, factorial by halving its
product and gcd and lcm by Euclid's steps, each as deep as its type allows.
A form that loops as many times as an argument says (the trapezoid,
Simpson and Romberg rules and `integrate`, `sum`, `partial_sum`,
`geometric_sum`, the geometric, power and Fibonacci terms and the partial
sums that loop, `binomial`), reads a `std::array` (`horner`,
`numeric_partial`) or calls exp, sin, cos, tan, sqrt, log or atan is a
constant expression from C++14, and at C++11 the same code at run time.**
So every form keeps its bits at every level: a program compiled before and
after the step gives the same results, form by form, at C++14 and C++20,
where double is evaluated as double, log and atan apart (S453). The 5-point
rule's one return adds its points in the loop's order, from 0.0, and gives
the loop's bits at run time, fused or not; on a 32-bit x87 its sum is kept
wider, as the loop's was not. `numeric_partial` reads its point through
`std::array`'s const operator[], a constant expression from C++14, where its
copy and `+=` needed C++17's.
- Alternative: every loop a recursion, so that every form is a constant
  expression from C++11: a recursion as deep as an argument at run time,
  which a large count overflows, or a pairwise sum, whose bits differ from
  the loop's.
- Alternative: every form from C++14 and C++11 given the declarations only:
  less than C++11 can express, against N1.

> **Review.** Taken: for, each form a constant expression wherever C++11 can
> make it one in one return, and every form's bits kept at every level.
> Against: the quadrature rules, the sums and the sequences' loops constant
> expressions only from C++14. The alternatives: recursion for every loop,
> deep or reordered; or every form from C++14, less than N1 asks.
> Recommended: as taken. Taken.

**S452. Each derivative rule takes a tag, `wrt<I>` (with respect to
argument I), as its first parameter, and its recursive calls name no
template argument, so that each rule's type, its one return's
(`-> decltype`), is deduced from its arguments alone; `derivative<I>(e)`
calls the rules and is the spelling callers use, as before.** GCC
substituted an explicit template argument into each rule's trailing return
type before deducing the rest, and so into the rules that type named, past
900 levels, where the rules named `derivative<_I>(...)`. A rule for a new
node is the overload `derivative(wrt<I>, const node&)` in the node's
namespace, found by argument-dependent lookup as the old overloads were.
- Alternative: a trait, one specialization per node with its type and its
  builder: no lookup to go wrong, but a new node's rule a specialization of
  an internal template.
- Alternative: the rules as they were, with deduced return types: C++14
  only, against N1.

> **Review.** Taken: for, the rules' types deduced from their arguments,
> which GCC needs at C++11, callers' spelling unchanged, a new node's rule
> still found by argument-dependent lookup. Against: a tag, wrt<I>, added to
> the public names, which a new node's rule must take. The alternatives: a
> trait of internal specializations; or the deduced return types, C++14
> only. Recommended: as taken. Taken.

**S453. `log` and `atan` are the C core's kernels, `d_math_log_c` and
`d_math_atan_c`, at compile time and at run time alike, as `exp`, `sin`,
`cos`, `tan` and `sqrt` are since step 29 (N3, S430): this header's own
kernels and its dispatch to the C library go.** The own logarithm divided
its argument by e as many times as its size asked, and at 20,000 arguments
spread over the doubles it was not correctly rounded at 6,972, a third, the
worst normal one 1.73 ulps off and a subnormal 8.2e11; the own arctangent
was not at 10,395 of 20,000, the worst 4.25 ulps off; and from C++20 the
run time took the C library's, a second answer. Now the logarithm is the
double family's correctly rounded one and the arctangent the C core's,
within half an ulp at each of the 40,000, one answer at every level. On a
32-bit x87, where double is evaluated wider, `log` refuses to compile, as
`exp` does (S418, S426). The header's constants are `constants::of<double>`'s,
one spelling at every level, where the variable templates are C++14's.
- Alternative: the own kernels kept to step 36 (6a): two answers across
  levels a step longer, and a logarithm whose answer at the smallest
  subnormal has 13 of its 53 bits right.

> **Review.** Taken: for, one answer at every level, the logarithm correctly
> rounded where the own one missed a third of its arguments and had 13 bits
> of 53 right at the smallest subnormal, the arctangent within half an ulp
> where the own one was 4.25 ulps off. Against: log refused on a 32-bit x87,
> as exp is. The alternative: the own kernels to step 36, wrong and
> two-answered a step longer. Recommended: as taken. Taken.

**S454. A face test, `t_calculus_face.cpp`, checks the six headers at every
rung: 1 check below C++11, 22 at C++11 and 26 from C++14, 18 and 21 on the
32-bit x87, where the checks of `exp` and `log` and of the 5-point rule's
bits are left out; 64 of 64 builds of the full tier and 4 of 4 of the fused
tier pass.** It checks each constant expression of S451 at its level, the
derivatives of the transcendentals bit for bit as the C core's kernels
composed by their rules, `log` and `atan` bit for bit `d_math_log_c` and
`d_math_atan_c`, the 5-point rule's bits against the loop's at 200
intervals, and each form at compile time against its run time's bits from
C++14. The old program `test_calc.cpp`, which no runner builds, passes its
45 checks on the new headers at C++14, 17 and 20, by g++ and clang++, and
stays as it is.
- Alternative: `test_calc.cpp` brought into the runner: a program of
  approximate checks, from C++14 alone, with no check below it.

> **Review.** Taken: for, the six headers checked at every rung, their
> constant expressions at their levels, the kernels bit for bit, the old
> program's checks passing on the new headers. Against: on the x87 the
> checks of exp, log and the 5-point rule's bits left out. The alternative:
> the old program in the runner, approximate and from C++14 alone.
> Recommended: as taken. Taken.

**S455. Chunk 58's zip is cut over the owner's main of 2026.11.23,
`6ee9dce1`, with Wednesday's and Friday's zips laid over it, in that order,
and holds chunk 58's files alone, at the tree's root (S449).** It goes after
Friday's zip, in the window after it: its three documents carry chunks 53
to 57's text. Should a window land chunks 53 to 56 other than as their zips,
chunk 58 is cut again over that commit.
- Alternative: one zip of chunks 56 to 58 for Friday's window, in place of
  Friday's: a zip already gated and given out changed two days before its
  window.

> **Review.** Taken: for, Friday's zip unchanged, chunk 58's own zip after
> it, the gates run over the three in order. Against: chunk 58 right only
> over Wednesday's and Friday's zips laid in order. The alternative:
> Friday's zip changed to carry chunk 58. Recommended: as taken. Taken.

### S456 to S462: chunk 59, 2026.11.23

The record: `_handoffs/math98_chunk59_2026.11.23/README.md`.

Chunk 59's decisions, S456 to S462: step 32 (5d), `reify`, a compile-time
expression made a term of the run-time language, so that it can be
printed, simplified and evaluated by name, and differentiated where the
language differentiates. Chunks 55's to 58's decisions stand by the owner's
"continue" of 2026.11.23 at 19:54; his rulings 12 to 15 are still to come.

**S456. `reify(e)`, in `expr/reify.hpp`, makes a compile-time expression a
term of the run-time language, `expr_term`, node for node in its shape,
nothing rewritten: its constants numbers, its variables names, + - * / and
unary - the language's operators, exp, sin, cos, tan, sqrt, abs, log, atan
and dot calls of the functions of those names, the relations, and, or and
not the language's, a piecewise a call of piecewise and a vector a vector
literal, and from C++14 constants.hpp's rational an exact fraction. What
the language has no term for, a parametric or an implicit function, is
refused at compile time, with a message. The header is empty below C++11;
`math.hpp` includes it, and `expr/expr.hpp` does not.** The term prints by
`expr_render`, simplifies by `expr_simplify`, substitutes and evaluates by
name, and `math::expr` wraps it. Step 32 is done with it: the run-time
derivative the step names is step 25's (4a), whose `expr_derivative` will
take the term when it comes; until then a compile-time derivative,
`derivative<I>(e)`, is reified and simplified (S458, S461).
- Alternative: a member, `e.reify()`, on every node: each node's header
  then includes the language's, which the compile-time core does not need.
- Alternative: the compile-time core rebuilt on the parse module's static
  terms (`expression_static.hpp`), reified by its own `reify<OpId, Atom>`:
  another lane's module under the core, for one bridge.

> **Review.** Taken: for, a compile-time expression printed, simplified,
> substituted and evaluated by name in the language, its shape kept, what
> has no term refused at compile time with a message; the compile-time core
> free of the language, the language's handle free of the core. Against: the
> run-time derivative the step names waits for step 25 (4a); until then a
> compile-time derivative is reified. The alternatives: a member on every
> node, each node's header pulling in the language; or the core rebuilt on
> the parse module's static terms. Recommended: as taken. Taken.

**S457. Variables are named by index: x, y, z and t by `reify(e)`, as
`math::variables` names indices 0 to 3, an expression that reads a later
index refused at compile time; or the names given, in index order, by
`reify(e, "r", "theta", ...)`, as many as the expression reads at least,
checked at compile time.** A name is anything a `std::string` is made
from. A name the constant table holds, pi, e, im, jm, km, true or false,
evaluates as the variable it is in the term, but its text reads back as the
constant.
- Alternative: a run-time list of names, with an error where it is short:
  every call's result an `expr_result`, for a check the compiler can make.
- Alternative: x4, x5 and so on for indices past 3: names the caller never
  wrote.

> **Review.** Taken: for, math::variables' names by default, any names
> given, both checked at compile time, no result to unwrap. Against: a name
> the constant table holds reads back from the text as the constant. The
> alternatives: a run-time list with an error; invented names past index 3.
> Recommended: as taken. Taken.

**S458. A constant whose value is an integer, of magnitude below 2^53 and
not -0, is that exact integer, as the language reads 2 from its text; any
other float, double or long double is its value in its type; an integer
type's constant is exact, a bool the constant true or false, and a vec3 a
vector literal.** So `expr_simplify` collects a reified derivative's
coefficients, the rules' 0.0 and 1.0 exact as it needs them: `1 * sin(x) +
x * (cos(x) * 1)` simplifies to `sin(x) + x * cos(x)`. A term's text reads
back as the same term's value, and in a typed fold the integer converts to
the same double. -0, and integers from 2^53 on, keep their double, whose
sign or size an exact integer would not keep.
- Alternative: every floating constant its double: the simplifier, which
  collects exact coefficients only, leaves a derivative as the rules built
  it.

> **Review.** Taken: for, a derivative's coefficients exact, so that the
> simplifier collects them; the term's text reading back as the term's
> value; the same double in a typed fold. Against: an integral double's kind
> exact in the term, where C++ held a double; -0 and integers from 2^53 keep
> their double. The alternative: every floating constant its double, a
> derivative left as the rules built it. Recommended: as taken. Taken.

**S459. reify rewrites nothing, and refuses what it cannot carry: a
piecewise's last branch is the call's otherwise, its condition dropped, as
the compile-time face evaluates it, never testing it; and arithmetic in an
integer type, whose division truncates and whose overflow wraps in C++, as
the language's exact integers do not, is refused at compile time, with the
message to make a constant a double.** A last branch that is `otherwise(d)`
loses nothing; one that is `when(c, d)` is the otherwise it is in C++.
- Alternative: the last branch's condition kept: a NaN in the term where
  C++ gives the branch's value.
- Alternative: integer arithmetic reified as the language's: `constant(1) /
  constant(2)` 0 in C++ and 1/2 in the term.

> **Review.** Taken: for, the term the expression's value wherever C++ gives
> one, nothing rewritten, integer arithmetic refused where C++ and the
> language differ. Against: a last branch's condition, never tested in C++,
> not in the term's text. The alternatives: the condition kept, a NaN where
> C++ gives a value; integer arithmetic reified, 0 in C++ and 1/2 in the
> term. Recommended: as taken. Taken.

**S460. A reified term's value is the language's: in double its
arithmetic, sqrt and abs give the expression's value bit for bit, and its
exp, sin, cos, tan, log and atan are the language's plain functions, which
ruling 12 decides, where the compile-time face's are the C core's kernels
(N3).** At 20,000 arguments each, the term's value and the expression's
differ at 18 of exp's, 35 of sin's, 26 of cos's, 56 of tan's, 1 of log's
and 3 of atan's, by one ulp, and at each the C core's kernel gives the
correctly rounded value and the C library's plain function does not;
arithmetic, sqrt and abs differ at none.
- Alternative: the language's functions the C core's kernels for reified
  terms alone: two answers in one language, before ruling 12 gives one.

> **Review.** Taken: for, one language, whose functions are its own; the
> arithmetic, sqrt and abs bit for bit the expression's. Against: exp, sin,
> cos, tan, log and atan the C library's plain functions, one ulp from the C
> core's kernels at 1 to 56 of 20,000 arguments each, and at each the kernel
> the correctly rounded value. The alternative: the C core's kernels for
> reified terms alone, two answers in one language. Recommended: as taken,
> the plain functions made the correctly rounded kernels as ruling 12
> decides. **Flag:** until ruling 12, a reified term's transcendentals can
> differ from the expression's in the last bit, the expression's being the
> right one.

**S461. A face test, `t_reify_face.cpp`, checks reify at every rung: 1
check below C++11, 19 at C++11 and 21 from C++14, 17 and 19 on the 32-bit
x87, where the bit-for-bit checks of the arithmetic and the checks of exp
and log are left out; 64 of 64 builds of the full tier and 4 of 4 of the
fused tier pass.** It checks each leaf's term and kind, and the names by
index and given; each node's text, which reads back to the same value at 40
points; the values at 200 points, the arithmetic's bit for bit, the
functions' within an ulp of the C core's kernels, a relation 1 or 0 and a
piecewise its branch's; a compile-time derivative reified, as the rules
built it and simplified, `sin(x) + x * cos(x)` and `6 * x + 2`, its value
the derivative's bit for bit; `math::expr` over a reified term; and from
C++14 the rational's exact fraction.
- Alternative: reify's checks in `t_expr_face.cpp`, the language's face
  test: a test that needs no compile-time core made to include it.

> **Review.** Taken: for, reify checked at every rung: each leaf, kind and
> name, each node's text read back, the values bit for bit where the
> language computes as C++ does, a derivative reified and simplified, the
> rational. Against: on the x87 the arithmetic's bit-for-bit checks and
> exp's and log's left out. The alternative: the checks in the language's
> face test, which needs no compile-time core. Recommended: as taken. Taken.

**S462. Chunk 59's zip is cut over the owner's main of 2026.11.23,
`6ee9dce1`, with Wednesday's, Friday's and chunk 58's zips laid over it, in
that order, and holds chunk 59's files alone, at the tree's root (S449,
S455).** It goes after chunk 58's zip. Among its files, `math.hpp`, whose
banner chunk 58 left saying that the umbrella compiled from C++14, says
that it compiles at every level.
- Alternative: chunks 58 and 59 in one zip in place of chunk 58's: a zip
  already gated and given out, changed.

> **Review.** Taken: for, the zips sent unchanged, chunk 59's own after
> chunk 58's, the gates run over the four in order; math.hpp's banner right.
> Against: chunk 59 right only over the three zips laid in order. The
> alternative: chunk 58's zip changed to carry chunk 59. Recommended: as
> taken. Taken.

### S463 to S468: chunk 60, 2026.11.23

The record: `_handoffs/math98_chunk60_2026.11.23/README.md`.

Chunk 60's decisions, S463 to S468: step 34 (5f), the opt-in lazy type, for
linear algebra first, as the owner's decision 2 asks: eager by default, and
laziness an explicit opt-in from C++11 whose bits are the eager ones
(principle 6). Chunk 59's decisions stand by the owner's "continue" of
2026.11.23 at 21:32; his rulings 12 to 15 are still to come.

**S463. The lazy type is `linear_algebra/lazy.hpp`, from C++11 and empty
below it: `lazy(x)` wraps a vector, a matrix or a dynamic_matrix, and the
operators on what it returns build an expression and compute nothing: + and
- of two expressions of one shape, unary -, a scalar times an expression on
either side, an expression of a vector divided by a scalar, as vector's /
is, `hadamard`, the entry-wise product, the matrix product of a matrix by a
matrix or by a vector, and `transpose`. `eval(e)`, or `e.eval()`, computes
it into a new value of the type eager arithmetic would give, and
`assign(dst, e)`, or `e.assign_to(dst)`, into dst, each element once, with
no temporary for each step. `linalg.hpp` includes it.** Nothing is lazy
unless `lazy` is written: eager arithmetic is as it was, and an expression
of lazy operands is a type of its own until it is evaluated.
- Alternative: every operator of the types lazy, as Eigen's are: against
  decision 2, and a change to what every caller's `auto` holds.
- Alternative: a lazy wrapper holding each operand by copy: a copy of each
  dynamic matrix, for an economy of copies.

> **Review.** Taken: for, laziness where it is asked for and nowhere else,
> eager arithmetic unchanged, each element once with no temporary a step,
> the results of the types eager arithmetic gives. Against: a second
> spelling of the same arithmetic, `lazy(a) + lazy(b)` beside `a + b`. The
> alternatives: every operator lazy, against decision 2; operands held by
> copy, a copy of each dynamic matrix. Recommended: as taken. Taken.

**S464. A lazy element is computed by the same operations, in the same
order, as the eager loops compute it, so that its bits are the eager ones:
a sum, a difference, a negation, a scaling and a division element by
element, an entry-wise product likewise; and a matrix product and a
transpose by the eager operations themselves, once, before the elements,
each product's entry the compensated dot of S404. Where the build may fuse a
multiply and an add, gcc and clang on a target with a fused multiply-add,
each product of the element loop is fenced, so that the next sum takes it
rounded, as the eager loops, each storing its result, always do; where
float and double are evaluated wider than their types, a 32-bit x87, every
element is fenced, rounded to its type.** With the fence taken out of a
build that fuses, gnu++17 with `-march=x86-64-v3`, gcc's lazy elements
differ from its eager ones at 54,931 of 262,000, clang's at none, its
default fusing within one expression only (`raw/measure/fence_out.txt`);
with it, at none. The fence costs a store and a load a product where the
target fuses, and nothing elsewhere.
On clang's x87 the eager fixed-size operations themselves keep the wider
value across their steps, as S37 documents, and so differ from the lazy
ones there, which round each step as gcc's eager ones do.
- Alternative: a matrix product's entries computed in the element loop,
  each the dot of a row and a column of operands that may be expressions
  themselves: those recomputed for every entry.
- Alternative: no fence, the lazy bits following the build where it fuses
  (S415): against principle 6, a lazy result another number than the eager
  one in the same build.

> **Review.** Taken: for, the eager bits by construction, element by
> element, products and transposes the eager operations' own; the fence
> measured needed, gcc's lazy elements otherwise a fifth of them other
> numbers in a fusing build. Against: the fence a store and a load a product
> where the target fuses, which keeps the loop from vectorizing there; on
> clang's x87 the eager fixed-size steps, not the lazy ones, keep the wider
> value (S37). The alternatives: products entry by entry in the loop,
> operands recomputed; no fence, two answers in one build. Recommended: as
> taken. Taken.

**S465. The leaves hold their operands by reference, so an expression lives
no longer than they do, and `lazy` of a temporary is refused at compile
time; both operands of + - and a product are lazy, a concrete operand
wrapped explicitly, `lazy(b)`, and a scalar is any arithmetic type,
converted to the element type as eager arithmetic converts it. Shapes are
checked as eager arithmetic checks them: by the types for the fixed sizes,
by assert for dynamic_matrix, a product's and a transpose's shape taken from
their operands before anything is computed.**
- Alternative: a concrete operand lifted to a leaf on its own: a
  temporary's reference kept by an expression an `auto` holds past its full
  expression.

> **Review.** Taken: for, no copy of an operand, no dangling temporary,
> nothing lazy unasked, shapes checked as eagerly as eager arithmetic checks
> them. Against: an expression's lifetime its operands', as with any
> reference. The alternative: concrete operands lifted on their own, a
> temporary's reference kept past its expression. Recommended: as taken.
> Taken.

**S466. `assign(dst, e)` writes each element of e into dst in place, dst
taking e's shape, as an eager assignment would: where e reads dst, it reads
it only at the element being written, element-wise, or before the first is
written, in a product or a transpose, so that no temporary is needed.**
`assign(a, lazy(a) * k + lazy(b))` and `assign(m, lazy(m) * lazy(n) +
lazy(m) * 0.5)` give the eager `a * k + b` and `m * n + m * 0.5`.
- Alternative: every assignment through a temporary, then copied: an
  allocation for each, which assign exists to save.

> **Review.** Taken: for, assignment in place without a temporary, safe
> where the expression reads the destination, the destination taking its
> shape. Against: none measured. The alternative: every assignment through a
> temporary. Recommended: as taken. Taken.

**S467. A face test, `t_lazy_face.cpp`, checks the lazy type at every rung:
1 check below C++11, 12 from C++11, 7 on clang's 32-bit x87, where the
fixed-size checks are left out (S37, S464); 64 of 64 builds of the full tier
and 4 of 4 of the fused tier pass.** It checks the types and shapes, and at
300 draws each element of vectors of double, float, long double and complex
numbers, fixed-size matrices with products and a transpose, and dynamic
matrices with products, a transpose and assign, against the eager
arithmetic, bit for bit.
- Alternative: the lazy checks in each type's own face test: the opt-in
  type spread over tests that need no C++11.

> **Review.** Taken: for, the lazy type checked at every rung against the
> eager arithmetic, bit for bit, in each element type and shape. Against: on
> clang's x87 the fixed-size checks left out, as the eager side differs
> there. The alternative: the checks spread over each type's own test.
> Recommended: as taken. Taken.

**S468. Chunk 60's zip is cut over the owner's main of 2026.11.23,
`6ee9dce1`, with Wednesday's, Friday's and chunks 58's and 59's zips laid
over it, in that order, and holds chunk 60's files alone, at the tree's
root (S449, S462).** It goes after chunk 59's zip.
- Alternative: chunks 59 and 60 in one zip in place of chunk 59's: a zip
  already gated and given out, changed.

> **Review.** Taken: for, the zips sent unchanged, chunk 60's own after
> chunk 59's, the gates run over the five in order. Against: chunk 60 right
> only over the four zips laid in order. The alternative: chunk 59's zip
> changed to carry chunk 60. Recommended: as taken. Taken.
