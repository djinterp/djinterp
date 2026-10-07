# djinterp decisions: the register and the approved programme, merged

Two decision files merged into one on 2026.10.04, each entry with its status
as of that day (branch `register/partials`, the step's handoff in
`_handoffs/register_2026.10.04/`):

- **Part A**, the approved programme: the owner's answers of 2026.09.30,
  items 1.1 to 5.2 and the rulings on specific files (`DECISIONS.md`).
- **Part B**, the register: the owner's style-conformance decisions #1 to
  #99, as copied into the tree on 2026.09.28
  (`research/djinterp_decisions_merged1_copy.md`). The canonical register,
  outside the tree, runs past #120; numbers from #100 on are not in this
  file, and their statuses are not known here.

Both parts are reproduced whole. What is added: this front matter, Part A's
status column brought up to date with its section 7, and a status line
opening each of Part B's decisions. Section numbers inside each part are
that part's own.

## Contents

1. Status at a glance
2. The rulings of 2026.10.04
3. What is still open for the owner
4. Found, not decided
5. Part A: the approved programme
6. Part B: the register, #1 to #99

## 1. Status at a glance

**Part A, the approved programme**

| Status | Items |
|---|---|
| done | 1.1 to 1.10; 2.1, 2.2, 2.3, 2.4, 2.6, 2.8; 3.1, 3.2, 3.3, 3.4; 4.1, 4.2, 4.3, 4.6, 4.8, 4.9; 5.1, 5.2; the event module; the TLS engine |
| in part | 2.5 (six nightly jobs never run; msp430-gcc), 3.6 (30 headers still empty below C++11), 4.4 (the root's transitional include), 4.5 (its second step), 4.7 (re_std's 46-header porting list), the `test_standalone` ruling (two files wait on a net branch) |
| not started | 3.5 (three questions open) |
| n/a, ongoing, standing | 2.7; 2.9; DTest |

**Part B, the register**

| Status | Decisions | Count |
|---|---|---|
| settled 2026.10.04 | #2, #3, #4, #7, #8, #9, #10, #11, #12, #13, #14, #15, #16, #19, #20, #21, #22, #23, #24, #25, #26, #27, #28, #29, #30, #31, #33, #34, #35, #36, #37, #38, #39, #40, #41, #42, #43, #59, #64, #93 | 40 |
| settled by the downporting | #17, #18, #32, #58, #65, #66, #72, #73, #78, #83, #88, #98 | 12 |
| settled earlier | #1, #5, #45, #71, #96 | 5 |
| other sessions' | #44, #54, #55, #56, #57, #60, #74, #75, #79, #81 | 10 |
| open; the tree already does what it recommends | #95, #99 | 2 |
| open, unchanged | #6, #46, #47, #48, #49, #50, #51, #52, #53, #61, #62, #63, #67, #68, #69, #70, #76, #77, #80, #82, #84, #85, #86, #87, #89, #90, #91, #92, #94, #97 | 30 |

"Settled by the downporting" means the C99 and C++98 work of 2026.09.28 to
2026.10.03 carried the decision out or made it moot; "settled 2026.10.04"
means the owner's rulings of that day, carried out on `register/partials`.
Of the 32 still open, 2 need only a yes: the tree already does what
they recommend.

## 2. The rulings of 2026.10.04

| Decision | Ruling |
|---|---|
| #9 | rename `D_CFG_APPLE_UNIVERSAL` into the `D_CFG_ENV_*` family |
| #16 | B: the rule once in the C guide, and a note in the files where it bites |
| #33 | a: `[c]`; b: allow `/** */` contract blocks; c: omit `@return`; d: decimal; e: corresponding header; f: CamelCase, "first letter", the typo; g: only `#ifdef` / `#ifndef`; h: allowed under an exception for "improves readability", as long as it looks as neat as possible; i: fix it; j: "it's fine" (left as it is) |
| #38 | B: write our own portable version of the checked scanf |
| #64 | B: keep the vendor attributes opt-in, as shipped |
| #93 | a to d, all A: `D_CFG_IS_ON` / `D_CFG_IS_OFF` and validation; move the four configs into `config/`; retire the eight stale copies; list all twelve in `dconfig.h` |
| side finding | "fix the invalid switch values": an invalid value reaches the switch's own `#error` |

The second batch, the same day:

| Decision | Ruling |
|---|---|
| #2 | get rid of unnecessary includes |
| #3 | opt-in |
| #4 | `env_apple.h`, consistent with #2 and #3 |
| #7 | redefine it |
| #8 | an opt-in switch |
| #9, #10 | rename |
| #11 | if the error space being built is pertinent to `d_strerror_r` (more error numbers once `derror.h` is complete), do nothing; if not, delegate. It is not: delegated |
| #12 | ASCII only |
| #13 | update the size |
| #14 | the precondition, plus a debug-build assert |
| #15 to #18, #20 | following the recommendation |
| #19 | keep as is |
| #21 to #32 | fix all of them |

The third batch, the same day:

| Decision | Ruling |
|---|---|
| #34 | define `_WIN32_WINNT` (in the build) |
| #35 | drop it |
| #36 | require both |
| #37, #42, #43 | following the recommendation |
| #39 | if dio's file functionality is redundant with c/fs, remove it; if not, follow the recommendation. Stream positioning was: removed |
| #40 | include `<linux/version.h>` |
| #41 | an explicit switch |
| #44, #54 to #60 | being done in the net session |
| #46 to #53 | ruled; to be carried out after this batch's files are returned |

## 3. What is still open for the owner

From the approved programme and its rounds:

1. **Decision 3.5's three questions**: the tag's name; whether the
   forward-declaration header is generated or written by hand; whether the
   change lands at once or module by module
   (`_handoffs/round3_2026.10.02/results_c.md`, section 9).
2. **A 32-bit rung for the ladder** (round 3, question 4; its cost is in
   `results_c.md`, section 7).
3. **`d_filter_operation_free`** frees what an operation owns but not the
   struct: rename it, or document that the caller frees the struct.
4. **FINDINGS Q2**: FreeBSD's `SIG_ATOMIC_MAX` on powerpc64 and wasi-libc's
   `WINT_MAX` are wrong upstream: draft reports, and/or hold platform limits
   too with those two as known exceptions.
5. **FINDINGS Q4**: a download step for the real msp430-gcc in the nightly,
   or the stand-ins.
6. **`mediator_handler_for`**: round 2's merging agent's rename stands unless
   overruled.
7. **The C guide's formatting**: 245 whitespace-only lines, and missing
   blank lines after lists that make paragraphs render inside the list
   above them. A formatting-only commit would fix it.
8. **`env.c`**: retire it, or always compile its compiler-information
   printer. Outside debug builds it holds nothing but the `<stdio.h>` it
   now includes, and its debug build does not compile (it calls two
   constants as functions).

From the register: every entry Part B marks open, and anything numbered #100
or later in the canonical register.

Not decisions, but the owner's to do: push, so the first CI run tests the
ladders' baselines, and dispatch the six nightly portability jobs that have
never run.

## 4. Found, not decided

- `D_ENV_CRT_MSVC` is defined only in simulated-environment builds, so
  `d_strftime_s`'s MSVC branch (#27) and `dtime.h`'s MSVC `timespec_get`
  branch never run on MSVC; `D_ENV_COMPILER_MSVC_FAMILY` is the detected
  fact.
- The eight older database configs force `*_CUSTOM` to 1 with `#undef` when
  a detection result is pre-defined, overriding an explicit 0, and still
  number their sections in Roman numerals.
- Six parse and parsegen headers carry 25 lettered fourth-level ToC entries,
  now against the guide (#33d).
- `net/tls/tls.hpp` stops at its own `#error` without the OpenSSL headers;
  net's.
- `D_TIME_HAS_STRPTIME` reads 1 wherever dtime is POSIX, even where glibc
  hides `strptime` (only `_DEFAULT_SOURCE`, or gnu11, without
  `_XOPEN_SOURCE`), and `dtime.c` then does not compile; the C guide's
  feature-test rule avoids it.
- `d_string_replace` and `d_string_replace_cstr` break on a string replaced
  with itself, as #23's three did.
- `env_archive.h` includes `<unistd.h>` or `<io.h>` only for
  `env_archive.c`'s path probe; the helpers and their includes could move
  into the `.c` file.
- `dstring.c` still classifies with the locale-dependent `isspace` (#12
  covered `string_fn`).
- `d_usleep` draws two `-Wsign-conversion` warnings where `long` is 32 bits
  (`-m32`, MinGW).

---

## Part A. The approved programme: items 1.1 to 5.2

The owner's answers of 2026.09.30 (`DECISIONS.md`, titled "Decisions -- the owner's answers, 2026.09.30"), whole. The status column of section 4 is brought to 2026.10.04; section 7 is new. Section numbers are this part's own; numbering inside is that of `cpp98_open_issues.md`.

Every decision the owner has given for the work this handoff carries, with
the action it approves. Numbering is that of `cpp98_open_issues.md`, beside
this file. Answers are quoted verbatim; "approved action" is what the answer
commits the next agent to.

The bar for every choice, in the owner's words: **"we seek the BEST solution,
not the cheapest, quickest or easiest."**

### 1. The owner's message of 2026.09.30, 09:05

> For 1.1-1.10: follow your recommendation
> 2.1-2.6 + 2.8-2.9: follow your recommendation; 2.7 which header? keep as is
> for now
> 3 = all done I presume
> 4.1 -4.2 follow your recommendation
> 4.3 remove the ; in NS_END, and for no_discard gate it appropriately for both
> C, C++, the C or C++ language version, the right compiler, and anything else.
> Make the defintion all in one place, or in as few places as possible
> 4.4 lets name it dmarco.h and dmacro_token.h for the macros, fix
> `D_NOEXCEPT_IF` as per your suggestion, and move them into /core/meta
> 4.5 either gate it or use the solution you recommended; whatever works first
> and adds the most functionality across all versions second
> 4.6 id like the user to be able to use long long if they'd like to; do
> whatever allows this. if they are in strict mode then they should know
> better, correct?
> 4.7 what is your recommendation on this?
> 4.8 so its fixed?
> 4.9 update the docs, yes
>
> 5.1 yes we MUST do a!!!
> 5.2 clean it up

### 2. Follow-up answers, same morning

Asked through the question tool; answers verbatim.

| Question | Answer |
|---|---|
| Section 3 was six open decisions, none implemented. Did "3 = all done" mean adopt the recommendations? | **Adopt all six (Recommended)** |
| The C test runner (default/test, test_block, test_module, test_handler, test_printer, test_session, the `dtest` umbrella) has never compiled; about 100 names it uses were removed by the rewrite. How should it get to zero? | **"ignore dtest for now, that is being rewritten"** |
| The four event sources implement the retired pre-core API; the tier-0 core's ~75 functions have no C implementation. How to resolve? | **Implement the core in C (Recommended)** |
| `ftp_interop_test.c` needs the TLS lane's `ssl_openssl.h`; `test_standalone_min.c` needs `tests/djinterp/c/fs/file_tests_sa.h`. Can you upload them? | **I'll upload them** — then uploaded `ssl_openssl.h` and `ssl_openssl.c`, and wrote: **"retire test_standalone\*.\*"** |

### 3. What was said back to the owner (uncontested)

These went to the owner in writing before the follow-up questions; the owner
answered the questions without correcting any of them, so treat them as
agreed.

- **2.7, "which header?":** `dstdint.h` (`inc/djinterp/c/re_std/dstdint.h`,
  1,571 lines). Kept as is: no split, no generator.
- **4.4, "dmarco.h":** read as **`dmacro.h`**. The variadic machinery stays in
  `c/dmacro.h`; the token macros (`D_CONCAT`, `D_STRINGIFY`, …) move to a new
  **`c/dmacro_token.h`**, free of variadic macros, which the root includes
  instead of all of `dmacro.h`. They stay in the C layer because C code uses
  them. **The C++ type utilities** (`void_t`, `clean`, `repeat_type`,
  `resolve_self`, now in `djinterp.hpp`) **move to `core/meta`**.
- **4.6:** "Correct. `long long` stays available wherever the compiler has it:
  every C99+ build, every C++11+ build, and default-mode C++98 as the
  extension. Only strict ISO C++98 (`-DD_CFG_ENV_ISO_STRICT=1`) lacks it,
  because ISO C++98 has no `long long`; opting into strict is opting out of
  it. The framework's own code uses `int64_t` where it means 64 bits and gates
  the rest on `D_ENV_HAS_LONG_LONG`."
- **4.7, the recommendation:** do it right after 4.4 and 2.2 —
  `re_std/cstdint` moves onto `dstdint.h` with using-declarations; each of the
  72 headers swaps its C++11-only std includes for re_std's own genuine C++98
  implementations (never aliases of std's C++11 types); `is_convertible` gets
  its `sizeof`-based C++98 branch; anything that cannot exist at C++98 (e.g.
  concepts) is omitted, not degraded; the 4.1 ladder measures each step.
  "Unless you say otherwise, I'll schedule it that way." — not objected to.
- **4.8, "so its fixed?":** **No, not yet.** It was prototyped in a scratch
  copy (8 of 8 checks right on GCC and Clang; the prototype is
  `research/sfinae98_trait_prototype.cpp`); `core/meta/trait_detect.hpp` in
  the tree is unchanged. Approved; still to apply, with its parity test.

### 4. The approved action, item by item

Status column: **done** (in this tree), **todo**, **n/a**.

#### 4.1 `dstdint` (section 1 of the issues doc) — all "follow your recommendation"

| # | Approved action | Status |
|---|---|---|
| 1.1 | (b): gate the fast-type format descriptors on the platform backend; supply withheld `INT_FASTn_MIN/MAX`, `UINT_FASTn_MAX` from the same descriptors; add the **converse** presence rule to dstdint.c for every family | **done** (`dstdint_2026.10.02/START-HERE.md`) |
| 1.2 | (b): key the fast-type rule on the `<stdint.h>` in effect (`__CLANG_STDINT_H`, `_GCC_STDINT_H`; otherwise that library's `<inttypes.h>` via `__has_include`, else omit the fast formats). **Caution:** some distributions move Clang's guard (see `research/clang_stdint_guard_openmandriva.patch`) | **done** (`dstdint_2026.10.02/START-HERE.md`) |
| 1.3 | (b): new env fact `D_ENV_ARCH_POINTER_BITS` in `env_arch.h`; env fills `D_ENV_ARCH_BITS` where it is 0 today; dstdint's `<limits.h>` path keys `intptr_t` on the new fact | **done** (`dstdint_2026.10.02/START-HERE.md`) |
| 1.4 | (b): dstdint.h declares the C names only; C++ names come from re_std's cstdint, which includes dstdint.h and declares them by using-declarations, no C++11 floor | **done** (`dstdint_2026.10.02/START-HERE.md`) |
| 1.5 | (a)+(c): `1 ? 1 :` / `0 ? 0 :` prefixes for IS_SHORT; Clang's `__<F>_FMT<c>__` as an oracle in dstdint.c's C++11 cross modes; msp430-gcc in CI (2.5) | **done** (`dstdint_2026.10.02/START-HERE.md`) |
| 1.6 | (a)+(b): exact-type checks for every limit/constant macro (C++ overload ranking, C11 `_Generic`, C99 size/sign); exact extrema of `wchar_t`, `wint_t`, `sig_atomic_t` via `uintmax_t` | **done** (`dstdint_2026.10.02/START-HERE.md`) |
| 1.7 | (a)+(b)+all extras: fail on skips unless `ALLOW_SKIP=1`; assert the configuration count; `${VAR-…}`; workflow runs `bash ci/check_dstdint.sh`; exec bit committed; negative-compile tests | **done** (`dstdint_2026.10.02/START-HERE.md`) |
| 1.8 | (a): `D_CFG_IS_INT_LITERAL` in cfg_common.h, used in **every** enumerated knob's validation, framework-wide | **done** (`dstdint_2026.10.02/START-HERE.md`) |
| 1.9 | (a): 64-bit family on the `<limits.h>` path only with proof of width, else absent | **done** (`dstdint_2026.10.02/START-HERE.md`) |
| 1.10 | (a): undefine the `__STDC_*_MACROS` dstdint.h itself defined | **done** (`dstdint_2026.10.02/START-HERE.md`) |

#### 4.2 `dstdint` design questions (section 2)

| # | Approved action | Status |
|---|---|---|
| 2.1 | (c): fast types on the djinterp backend where identity is known — GCC `__INT_FASTn_TYPE__`; for Clang a per-C-library table (glibc, Apple, BSDs, MSVC runtime, bionic, newlib) keyed on each library's macros, **every row verified against that library's real header** (sources fetched so far: `research/dstdint_upstream_stdint/`); absent when unknown | **done**, with WASI's row; FINDINGS Q1 (c) and Emscripten's fast types (`9c57d79`) in round 3 |
| 2.2 | (a): `re_std::uint32_t` etc. — rewrite the ~1,610 `std::` uses in the 161 headers (`re_std::` inside namespace djinterp, `djinterp::re_std::` outside) | **done** for re_std (`4f61cd3`) and every lane (round 3, merged by `3f05d69`); math, net, dawk and DTest, worked on in other sessions, keep theirs |
| 2.3 | (a): one scripted pass replacing the ~330 `<stdint.h>`/`<cstdint>`/`<inttypes.h>` includes (house include-comment style, per-file relative paths; C++ through re_std's cstdint) **plus a CI lint** that fails on any direct include outside dstdint.h and re_std's wrappers | **done**: every lane swept, and `tools/check_stdint_includes.py` runs in CI as `stdint-lint`, its known list the 126 files of math, net, dawk and DTest |
| 2.4 | (b): keep the pragma for declarations; wrap the framework's own 64-bit literal uses in `D_LONG_LONG_DIAG_PUSH`/`POP` (defined in the root, 4.6); a default-mode CI row with no `-Wno-long-long`; a declaration-only row for dstdint.h | **done**; the 32-bit hole closed in round 3 (question 1 (a), the 64-bit floor) |
| 2.5 | all: simulated old-library and no-predefines cases in PR CI; nightly portability workflow with macOS, Windows (MSVC, clang-cl, MinGW-w64), Alpine (musl), CentOS 7, cross GCC (arm-none-eabi, avr, msp430), optional BSD VMs | **done**, as far as one Linux machine reaches: six nightly jobs have never run (FINDINGS Q5); msp430-gcc is not in Ubuntu (Q4, open) |
| 2.6 | (a)+(c): report upstream (draft only — nobody here can file it) and document; keep the relaxed checks | **done**: no report needed, fixed upstream in Clang 21 |
| 2.7 | **keep as is for now** | n/a |
| 2.8 | `dconfig_readme.md` entries for `D_CFG_ENV_ISO_STRICT` and `D_CFG_STDINT_BACKEND`; style guides gain "include dstdint.h, never `<stdint.h>`" (with 2.3's lint) | **done** in round 3 (F1, F2); the project's guides and `dconfig_readme.md` carry it |
| 2.9 | commit as a series after the P1 fixes | ongoing |

#### 4.3 C++98 plan decisions (section 3) — "Adopt all six"

| # | Approved action | Status |
|---|---|---|
| 3.1 | `D_NOEXCEPT` empty at C++98 | **done** (`3f67b68`) |
| 3.2 | a named predicate (`is_valid()` style) at every level, `explicit operator bool` added from C++11 | **done** (`3f67b68`) |
| 3.3 | `D_DELETED_FN` at C++98: private, never-defined declaration — declared private at every level | **done** (`3f67b68`) |
| 3.4 | struct-scoped enums at every level; a fixed-width integer where an enum's storage is part of a layout | **done** (round 3, lane 2: the C++98-floor modules' eleven enums) |
| 3.5 | (c)+(d)+(a): one inline-namespace tag separating C++98 from C++11-and-up (members added above C++11 must be ODR-benign — a review rule); MSVC `#pragma detect_mismatch`; policy documented: build a program's C++ at one standard, the C API is the supported mixing boundary; a forward-declaration header | **not started**: round 3's inventory and proposal (`round3_2026.10.02/results_c.md` section 9); three questions open (section 3 above) |
| 3.6 | adopt the floors — C++98: root, env, the C layer, re_std, `core/fs`, `core/sync`, `core/meta`, `core/cli` (+ util, text, memory, container if the ladder allows); C++11: functional, event, paradigm, test, net, parse, parsegen, ui, render; C++17: option, db. Revisit the four with ladder numbers | **in part**: floors adopted, and the owner's D4 ruling put util, text, memory and container at C++11 where C++98 cannot be reached; 30 of the 62 headers of `core/{fs,sync,meta,cli}` are still empty below C++11 (`core/fs` is fully down) |

#### 4.4 C++98 implementation (section 4)

| # | Approved action | Status |
|---|---|---|
| 4.1 | the ladder, built first | **done** (`fd764fd`); the whole ladder matches the baseline (2026.10.04) |
| 4.2 | (a): define the macro kit (`D_CONSTEXPR_CPP14`, `D_NULLPTR`, …) once in the root; delete the ~99 private copies; lint against redefinition | **done** (`8abb386`; the lint `6f1208b`, in CI) |
| 4.3 | **`NS_END` is `}`** (no `;`). **`D_NODISCARD`: one definition, in one place (or as few as possible), gated on language (C/C++), language version, compiler, and anything else relevant.** Content per the recommendation: C23 and C++17 `[[nodiscard]]`; below that the vendor attribute only where it behaves the same (Clang `warn_unused_result`, silenced by a `(void)` cast), GCC ≥ 11 in C via `__extension__ [[nodiscard]]` (the C99 step's spelling), nothing where the behaviour would differ. Today it is spelled three times — `c/djinterp.h` (a C and a C++ branch) and `env/c/env_attributes.h` (a C++ block near line 258, a C block near 559) — and whichever header is included first wins. Both C++ spellings emit `[[nodiscard]]` below C++17 whenever `__has_cpp_attribute(nodiscard)` says yes (g++ does, even at C++98), and the `env_attributes.h` C++ block falls back to GCC's `warn_unused_result`, which a `(void)` cast cannot silence | **done** (`bf636e0`, `6a7b024`, `b4db405`) |
| 4.4 | `c/dmacro_token.h` (token macros, no variadics; the root includes it instead of `dmacro.h`); one-argument `D_NOEXCEPT_IF`; type utilities to `core/meta`. Prototype: `research/proto98_patches/` (made against merged1; its `util/macro/concat.h` becomes `c/dmacro_token.h`) | **done** (`5ecc193`, `9f7dd73`, `3f67b68`), but for the root's transitional include of `core/meta/type_utility.hpp` |
| 4.5 | "whatever works first and adds the most functionality across all versions second": `D_ENV_PP_HAS_VARIADIC_MACROS` driven by the strict knob; dual-form list macros with `D_UNPAREN` (Boost.PP's extra-expansion workaround inside it for MSVC's traditional preprocessor); variadic-by-nature families — first gate them out under strict (works), then add preprocessor sequences `(a)(b)(c)` for the user-facing families C++98-floor modules need (functionality) | **first step done** (`a6d47ea`, `0e792d0`); the second (sequences, `D_UNPAREN`) waits for a family that needs it |
| 4.6 | users keep `long long` wherever the compiler offers it; strict C++98 excludes it by definition. Framework: `int64_t`/`intmax_t` where width is meant; gate the rest on `D_ENV_HAS_LONG_LONG`, declarations wrapped in `D_LONG_LONG_DIAG_PUSH`/`POP` (defined once, in the root) | **done** (`26bcd13`, `3683e56`) |
| 4.7 | as recommended in §3 above, after 4.4 and 2.2 | **first step done** (`f0c3818`); the 46 re_std headers empty below C++11 wait on "port when needed" |
| 4.8 | apply the prototyped engine to `trait_detect.hpp`: `sizeof` detection at C++98, the `requires` expression at C++20 with the trait defined from the concept, and a parity test between the two | **done** (`aea09ab`) |
| 4.9 | update the docs: the C++ guide's "targets C++20" becomes a C++98 floor with levels; "`static_assert` unconditional" becomes `D_STATIC_ASSERT`; the decisions register's C++ target row and #83 (a copy of the merged1 register is in `research/`; the owner's canonical register is outside this tree, so write an addendum) | **done** (section 6) |

#### 4.5 C framework (section 5)

| # | Approved action | Status |
|---|---|---|
| 5.1 | **(a), and it MUST reach zero**: triage by root cause, burn the C99/C11 failures down. 38 → 28 in this session; the 28 are classified in `known_failures.txt`. The 13 C-runner units are excluded **by the owner's ruling** ("ignore dtest for now, that is being rewritten") | **done**, to the owner's ruling: the C failures left are DTest's 13 units |
| 5.2 | make the tree `-Wundef`-clean, then add `-Werror=undef` to CI (after 1.8) | **done** (`205ed83`): `-Werror=undef` in both ladders |

#### 4.6 Rulings on specific files

| Subject | Ruling | Status |
|---|---|---|
| The C test runner / DTest (`test/c/default/*`, `test_handler`, `test_printer`, `test_session`, `test_tree`, the `dtest` umbrella) | **ignore — being rewritten.** Do not port, reconstruct, retire or edit | standing (DTest is worked on in other sessions) |
| `test_standalone*.*` | **retire**: `inc/djinterp/test/c/test_standalone.h`, `src/djinterp/test/c/test_standalone.c`, `src/djinterp/test/c/test_standalone_min.c`, `inc/djinterp/config/core/defaults/test/test_standalone_defaults.h`, `_handoffs/net_foundation/test_standalone_standin.c` | **in part**: three of five retired (`7392591`); `test_standalone.h` and net's stand-in wait on branch `net/retire-test-standalone` (`c5d3f18`), net's to merge |
| Event module | **implement the tier-0 core in C**; the four pre-core sources then retire | **done** (`cc48a5a`) |
| TLS engine | uploaded; **added** (`8ed6d70`) | **done** |

### 5. Standing rulings from earlier steps (still in force)

- Stale copies left behind by moves are **retired**, to
  `_retired/<step>/` with a README entry: old path, successor, reason
  (ruling of 2026.09.29; `derror.h` was retired "for now" under it).
- The test framework's union is named `data`; `dmemory.h` is one header in
  the memory module (2026.09.29).
- C side: **C99 floor**; MSVC's default C mode supported; detection through
  `env/`, never raw `__STDC_VERSION__`.
- C++ side: floor lowered to **C++98**; the default mode is everything the
  compiler supports; strict ISO C++98 via `-DD_CFG_ENV_ISO_STRICT=1`; list
  macros accept both forms and only strict mode requires the parenthesized
  one.
- re_std: preprocessor identifiers keep `RESTD`, everything else `re_std`;
  namespace `djinterp::re_std` (nested); **`NS_END` is `}` not `};`**;
  C++98-capable stubs are genuine implementations, never aliases of std's
  C++11 types; "omit, don't degrade".
- Decisions 7 (config cycle cut) and 8 (`D_NODISCARD`) from the net lane.

### 6. Addendum, 2026.10.01: the C++98 programme after lane 1 (decision 4.9)

The owner's rulings since this register was written, in order:

| When | Ruling | Applied |
|---|---|---|
| 09.30 | "break the c++98 portion into multiple other agents (4)"; the C lane stays here | `_handoffs/cpp98_2026.09.30/`, four briefs |
| 09.30 | the C function bodies: "do the other functions body" | functional, pdf, option_diff; option_diff's two-call protocol is option (a) -- a NULL buffer measures, a short one fails alone |
| 09.30 | "ignore the test files for now" | no harness files added; checks run throwaway |
| 09.30 | "lets do test headers. comment out where you can" | 95 undefined declarations and the 8 macros calling them commented out; `d_assert_standalone` and `d_test_counter_reset` kept for the net suites |
| 10.01 | lane 1: compile now, port to C++98 later | a unit above its level compiles to nothing below it, never `#error` |
| 10.01 | lane 1: `env_printer.hpp` is the owner's (`print.hpp`) | left failing on the ladder |
| 10.01 | lane 1: `archive.h` and `compress.h` lose their `#error` | `22bfc3b` |
| 10.01 | "merge and finish Lane 1 ourselves" | merged at `b6df539`; finished on master (below) |
| 10.01 | lane 4's fourth pass, forwarded | merged at `8bbd185`; four test units left: two waiting on lane 3's document module, one on a document API that exists nowhere, one lane 4's own |
| 10.01 | lane 3's handoff, forwarded: "retire `matrix_iterator.hpp`" | merged at `2b7dae6`; retired to `_retired/cpp98_merge_2026.10.01/` with `section.hpp` (lane 3's 5.1) |
| 10.01 | "make linked_node::null_link() return -1" | `size_type(-1)` for an index link, as documented and as `is_null_link` tests |
| 10.01 | lane 2, merged with lane 1, forwarded | merged at `fc4906c`; all four lanes are on master |

Finishing lane 1 on master: the C headers' C++ faces are included from
their modules' floors (`9f042ff`); `core/event`'s 22 extra semicolons are gone
(`2879cc2`, lane 4's files); the root's second derivation of
`D_INTERNAL_QUAL_TESTING` is gone (`647c4bc`); and the macro redefinition lint
runs in CI with its known list (`6f1208b`, 4.2). Its three exemptions are
part of the rule from now on: names defined only under `config/` (the cascade
defines one knob in several files on purpose), the two roots read as one
header (each defines the shared kit for its own language), and the generated
`c/util/macro/` families (configuration picks one variant).

The docs (4.9): the C++ guide's "targets C++20" is now a C++98 floor with
levels -- decision 3.6's module floors, the empty-below-floor rule, the level
tests, and the root's kit in one table -- and its "static_assert is available
unconditionally" is now `D_STATIC_ASSERT`. The C guide's "targets C11" is now
the C99 floor this register already ruled (section 5), with the same
`D_STATIC_ASSERT`. Both were delivered as replacement copies of the project's
`style_guide_cpp.md` and `style_guide_c.md`; the project files themselves are
the owner's to swap.

Still open from lane 1, and why: the ladder's known-failures baseline (4.1)
needs one full CI run on master -- about 31,000 compiles, not feasible on the
merge session's single processor; `-Werror=undef` in the C++ ladder waits for
lanes 2-4's sites (`results_root.md`); 4.5's second step waits for a family
that needs it. `results_root.md` lists five choices lane 1 made that the owner
may want to revisit.

## 7. Addendum, 2026.10.02 to 2026.10.04

Rounds 2 and 3 ran the programme's remaining items; their rulings and
results are in `_handoffs/cpp98_round2_2026.10.01/README.md`,
`_handoffs/round3_2026.10.02/README.md` and
`_handoffs/round3_2026.10.02/merge_2026.10.03/HANDOFF.md`, and the statuses
in section 4 above take them in. On 2026.10.04 the owner then ruled on the
register's decisions that the downporting had negated only in part (Part B).
Each was carried out on branch `register/partials`, one change per commit,
measured before and after (`_handoffs/register_2026.10.04/README.md`):

| Ruling of 2026.10.04 | Commit |
|---|---|
| #9: rename to `D_CFG_ENV_APPLE_UNIVERSAL`, in a config of its own | `269aed9` |
| found: the root's `ssize_t` block tests `D_ENV_OS_USING_WINDOWS64` with `defined()` | `fa11f39` |
| #93c (A): retire the eight stale database configs | `7607cbb` |
| #93b (A): the four database configs move into `config/` | `a9d5811` |
| #93a (A): `D_CFG_IS_ON` / `D_CFG_IS_OFF`, and the switches validated | `8c9679e` |
| #93d (A): `dconfig.h` lists the twelve database configs | `a975b27` |
| #16 (B): feature-test macros come from the build; the C guide's rule and three notes | `f028dde` |
| #64 (B): vendor attributes stay opt-in, and the header says so; #59 closes with it | `c54ed08` |
| #33, a to i: both guides; j as is | `476c0ae` |
| #38 (B): the checked scanf family is the framework's own | `ccf0af6` |
| side finding: an invalid switch value reaches the switch's own `#error` | `7b8c3bb` |
| #2: module headers stop including detection they don't use | `958aad8` |
| #3: per-platform env headers stay opt-in, and `env.h` says so | `cde4687` |
| #4: `env_apple.h` includes `<Availability.h>` itself | `7f5777a` |
| #7: `D_ENV_LANG_USING_C` means compiling as C | `56cd674` |
| #8: `<unistd.h>` is opt-in for POSIX detection | `ea03a90` |
| #10: the internal detection macros carry `ENV_` | `6cbe581` |
| #31: `env_archive.h` stops including `<archive.h>` on a promise | `9ada003` |
| #12: `string_fn` is ASCII only (two lines wrapped in `5922876`) | `a042abc` |
| #30: `d_strcasestr_index` compares embedded NULs | `e5b1fb8` |
| #11: `d_strerror_r` asks the platform | `c787f83` |
| #13: tokenizing keeps the size with the text | `77ec2c5` |
| #22: dstring's clamps cannot wrap | `a8a474f` |
| #23: a string can be appended, inserted or assigned to itself | `8c0eb5d` |
| #24: a split of delimiters only returns no array | `bb5ee58` |
| #21: the index macros go through `d_index_is_valid` | `6c433bb` |
| #28: `d_timespec_cmp` orders NULL first on both sides | `9812551` |
| #14: `d_timespec_to_ns` asserts its precondition in debug builds | `6c97fcd` |
| #29: the pre-2015 MSVC `timespec` shim is gone | `eb8e488` |
| #27: `strftime` everywhere; `timespec_get` by `TIME_UTC` | `68a4826` |
| found: the portable `d_strptime` declares `matched` where C allows it | `331f514` |
| #26: the Windows clocks keep their frequency and their carry | `8e81b88` |
| #15: `d_nanosleep`'s fallback: `thrd_sleep`, `sleep()`, or `ENOSYS` | `27da03a` |
| #25: `timegm` where it is declared, and a fallback that normalizes | `b773090` |
| #20: `-Wundef` joins both guides' flag tables | `0accfda` |
| #35: a store app no longer reads as desktop too | `86272bd` |
| #36: C++/WinRT needs Windows and its own header | `c3b980d` |
| #37: Windows on ARM under MinGW, and Cygwin's bitness | `0216af1` |
| #34: the build defines `_WIN32_WINNT` (both guides) | `484007e` |
| #39: dio drops the stream positioning c/fs already provides | `a28c1ef` |
| #40: `env_linux.h` includes `<linux/version.h>` | `2df9eab` |
| #41: musl is the build's word, `D_CFG_ENV_LINUX_MUSL` | `8b9ba91` |
| #42: `env_bsd.h` includes each variant's version header | `aea72dc` |
| #43: visionOS reads as visionOS | `5cb980e` |

Each was measured before and after on every unit that reaches what it
changed; the whole ladders on the final tree are in the step's README.

---

## Part B. The register: decisions #1 to #99

The owner's register, as copied into this tree on 2026.09.28 (`research/djinterp_decisions_merged1_copy.md`, titled "djinterp style-conformance work: decisions and handoff"), whole and in its own order. Each decision now opens with its status as of 2026.10.04; nothing else in it has changed, and its section numbers are its own. The canonical register, outside this tree, runs past #120: numbers from #100 on are not here.

Handoff notes for an agent continuing the style-guide conformance pass over the
djinterp C/C++ framework. It records the conventions the work follows, the
decisions already made, every decision still open (each with advantages,
disadvantages, and a recommendation, the form the project owner asks for), the
outstanding to-dos, and which files are done.

As of 2026-09-26. Decisions 71–84, the addition to #33, and the done mark on
the first to-do in section 4 were merged in 2026-09-27 from relay 93.
Decisions 85–88, the updates to #6, #20, #59 and #64, the banner addition to
##33, and the `env/c` entries in sections 4 and 5 came from the `env/c` pass
the same day, as did #1's resolution for the archive and compression headers,
and decisions 89–91 with the `env/cpp` entries in section 5, and 92–97 with
the `env/db` entries.
On 2026-09-28 the owner set the floors at C99 and C++11 (#5, section 2).
The C99 session's tree was merged into the owner's tree the same day. The
updates to #5, #15, #29, #33, #58, #64, #65 and #93, decisions 98 and 99,
and the to-dos marked 2026-09-28 come from that merge.

---

### 1. Context

- **Project:** djinterp, a C/C++ framework. Style guides:
  `style_guide_c.md` and `style_guide_cpp.md`. The C guide still says C11;
  #33 lists the edits the C99 floor needs.
- **Target languages:** **C99** for C (#5, resolved 2026-09-28) and
  **C++11** for C++. Both override the guides, which say C11 and C++20.
- **Nothing is in production yet.** Removed or renamed headers need no
  compatibility shims.
- **Decision format:** when raising open decisions, give each one its
  advantages, disadvantages, and a recommendation.

#### 1.1 Conventions every converted file follows

- **Banner.** A fixed 80-column template: `/` plus 79 `*` opening, 79 `*`
  plus `/` closing, filename ending on column 80 of the title line, `path:`,
  `link(s):` and `author(s):` values starting on column 14, and `created:` /
  `revised:` values ending on column 80. The title tag is the file's top-level
  directory (`[c]`, `[env]`, `[config]`, `[net]`, ...).
- **Include guards.** Derived mechanically from the path:
  `DJINTERP_` + subsystem prefix + remaining directories + stem + `_H`, with
  `#define GUARD 1` and `#endif  // GUARD`.
- **Numbering.** Sections are `// N.  NAME` between 80-column `//===` rules;
  subsections are `// N.M    Name` followed by an 80-column `//---` rule and
  zero blank lines; items are a `// N.M.K` marker line, then `// NAME`, then
  `//   category: description`. Identifier tables use fourth-level
  `// N.M.K.J` entries. Two blank lines follow a section's description; one
  blank line separates items.
- **Tables of contents.** Required, and generated from the body's markers,
  so the two always agree.
- **Includes.** Grouped under category comments (`// std`, `// djinterp`,
  `// platform`, `// openssl`, ...), each with a trailing summary naming what
  is used from it, aligned where practical. Include what you use: a header that
  uses a macro includes the header that defines it, not a header that happens
  to pull it in.
- **Env headers.**
  - Every detection macro is `#ifndef`-guarded, so builds can override it.
  - Each `#endif` closing an `#ifdef`/`#ifndef` names its symbol.
  - Multi-condition `#if`s put one parenthesized condition per line, with the
    backslash at column 80.
  - C linkage uses `#if D_ENV_LANG_USING_CPP` + `extern "C" {`: env headers sit
    below `djinterp.h`, so `D_EXTERN_C_BEGIN` is unavailable to them.
- **Function comments.** Doxygen contracts on header declarations
  (`@brief`, `@param[in|out|in,out]`, `@return` except for `void`). Mechanism
  prose on definitions, in a `/*` block that opens with the function's name
  and carries no contract tags.
- **Verification.** Every conversion was checked for identical behavior.
  - Preprocessor macro state (`-dM`) and token streams (`-E`) are compared
    between the old and new versions across targets (`clang -target ...`),
    languages, and configurations.
  - Stand-ins replace unavailable headers.
  - Strict builds use `-Wall -Wextra -Werror -pedantic -Wshadow -Wconversion`,
    plus `-Wreserved-identifier -Wdocumentation` on clang.

---

### 2. Resolved decisions

| Decision | Outcome |
|---|---|
| C++ target | C++11, overriding the C++ guide's C++20; confirmed 2026-09-28 |
| 5. C floor | C99 (2026-09-28), not the recommended C11. A C build below C99 stops at an `#error` in `djinterp.h`; MSVC's default C mode counts as C99 from Visual Studio 2015 on (`D_ENV_LANG_C_INFERRED`). Done |
| Compatibility shims | None needed: not in production |
| `web` directories | Renamed: `/env/web` to `/env/net`, `/djinterp/web` to `/djinterp/net` |
| 45. `env_web.h` duplication | Dissolved, see below. Done |
| TLS backend tables | Two axes, see below. Done |
| Include-what-you-use in `env_pop.h` / `env_tls.h` / `env_net.h` | Fixed: `env_pop.h` and `env_tls.h` include `../env.h` directly, and `env_net.h`'s include comment now names what it uses. Done |
| 96. `env_mysql_common.h` needs `env.h` | Fixed in its conversion (2026-09-27): it includes `env.h` itself, so automatic MySQL and MariaDB detection compiles in any include order. Done |
| 1. Where the env headers live (archive and compression) | `env/util/archive/` and `env/util/compress/` (2026-09-27). The converted copies moved there; the flat and nested copies are deleted, and every includer points at the new paths. `env_pdf.h` is not yet affected |

> **Status, 2026.10.04:** every row still stands, and #1's other half (`env_apple.h`, `env_net.h`) is settled too.

**How `env_web.h` was dissolved:**
- Its transport-layer macros already duplicated `env_net.h`'s exactly, so
  they were dropped; `getaddrinfo` detection and the byte-order helpers moved
  into `env_net.h`.
- Its six-library TLS detection moved to `env_tls.h`.
- Its zlib/brotli/zstd flags were dropped in favor of
  `D_ENV_COMPRESSION_HAVE_*`.
- `D_ENV_WEB_CAN_NETWORK` became `D_ENV_NET_CAN_TCP`, and `D_ENV_WEB_CAN_TLS`
  became `D_ENV_TLS_AVAILABLE`.
- `env_curl.h` now builds on `env_net.h` and gates on `D_ENV_NET_CAN_TCP`.

**The two TLS axes:**
- `D_ENV_TLS_BACKEND` is the library, using `env_web.h`'s seven-value table
  (0–6): none, OpenSSL family, mbedTLS, GnuTLS, wolfSSL, Schannel, Secure
  Transport. It keeps `env_web.h`'s preference order.
- `D_ENV_TLS_OPENSSL_VARIANT` (with its `_NAME`) says which OpenSSL-family
  member is present: none, OpenSSL, LibreSSL, BoringSSL.

---

### 3. Open decisions

Numbers are stable: they are how the owner refers to decisions. The numbers
run 1 to 99, with #5 and #45 resolved (section 2); #60 was raised with #44 but
not numbered at the time. 71–84 came from relay 93's merge work, 85–88 from
the `env/c` pass, 89–91 from `env/cpp`, 92–97 from `env/db`, and 98–99 from
the C99 merge.

#### Design decisions

**1. Where the env headers live.**
The module headers include `env/compress/env_compress.h` and
`env/archive/env_archive.h`, and `env_apple.h` sits in `env/os/`. But the four
library headers' own banners, and the guards derived from them, say they sit
directly in `env/`. The web-to-net rename already put the net headers in
`env/net/`. The same question applies to `env_net.h` itself, whose banner
calls it the `net/` layer's.

> **Status, 2026.10.04:** settled earlier: every env header lives in a
> subdirectory, its banner and guard following its path; `env_apple.h` and
> `env_net.h` match theirs.

- *Advantages (subdirectories):* matches how everything already includes them,
  and how `env/c/`, `env/cpp/`, `env/os/`, `env/ui/` and `env/net/` are
  organized.
- *Disadvantages:* four banners, guards and relative includes to update, plus
  the guard checks in `env_compress_link.h`.
- *Alternative (flat):* no churn, but the module headers' include paths are
  then wrong.
- **Recommendation:** subdirectories.
- **Resolved 2026-09-27** for the archive and compression headers:
  `env/util/archive/` and `env/util/compress/` (section 2).

**2. Module headers pulling in env detection.**
`compress.h`, `archive_common.h` and `pdf_primitives.h` include detection
headers they don't use themselves.

> **Status, 2026.10.04:** settled 2026.10.04: "get rid of unnecessary includes":
> `compress.h`, `archive_common.h` and `pdf_primitives.h` stop including
> detection they don't use; a file that reads a `D_ENV_*` fact includes its env
> header itself (`958aad8`).

- *Advantages (keep):* detection arrives with the module that needs it, and
  nothing else pays for it.
- *Disadvantages:* the guide's include-what-you-use rule, as written, forbids
  it.
- **Recommendation:** keep it, and add one sentence to the guide: a module's
  public header may include its env detection header for its consumers.

**3. Should `env.h` include per-OS headers like `env_apple.h`?**
The old banner said it did; the current `env.h` doesn't.

> **Status, 2026.10.04:** settled 2026.10.04: "opt-in": `env.h` includes no
> per-OS header, and its banner says so (`cde4687`).

- *Advantages (auto-include):* convenient.
- *Disadvantages:* every file pays for about 240 macros.
- **Recommendation:** opt-in, which matches the load-on-demand approach of #2.

**4. Where `<Availability.h>` gets included.**
Without it, a macOS 13 build reports "macOS (Legacy)" and every SDK-gated
feature as 0. `env_ios.h` has the same dependency and inherits whatever is
decided here, since it includes `env_apple.h`.

> **Status, 2026.10.04:** settled 2026.10.04: `env_apple.h` includes
> `<Availability.h>` itself, on Apple, consistent with #2 and #3 (`7f5777a`); it
> had worked only through `<unistd.h>`.

- *Option `env_apple.h`:* loads it only with Apple detection.
- *Option `env_os.h`:* puts it in every Apple translation unit.
- **Recommendation:** `env_apple.h`, consistent with #2 and #3.

**5. C floor: C99 or C11.**
The guide says C11, but `djinterp.h` still calls C99 its "tentative floor"
and keeps pre-C11 fallbacks.

> **Status, 2026.10.04:** settled earlier: C99, the owner's ruling of 2026.09.28
> (section 2 of this part).

- *Advantages (C11):* matches the guide and the C++11 decision, gives real
  `static_assert`, and lets the fallback code go.
- *Disadvantages:* drops very old toolchains, which is all C99 buys.
- **Recommendation:** C11.
- **Resolved 2026-09-28:** C99, against the recommendation. The C99
  session's work, merged the same day, builds to it: every C unit is
  checked with GCC and Clang at `-std=c99` and `-std=c11` under
  `-pedantic-errors` (`ci/check_c_standards.sh`). The pre-C11 fallbacks
  stay, and #15 and #29 were re-read against the new floor.

**6. Compiler extensions outside the env layer.**
`djinterp.h`'s qualifier kit spells `__forceinline` and
`__attribute__((always_inline))` directly; the guide reserves those for the
env layer.

> **Status, 2026.10.04:** open, unchanged: `djinterp.h` still spells the vendor
> forms itself.

- *Advantages (move them into `env_compiler.h`):* conformance, and all vendor
  spellings in one place.
- *Disadvantages:* touches the root header. A guide exception would avoid that.
- **Recommendation:** move them, keeping public names such as `D_INLINE` where
  they are.
- **Update (2026-09-27, `env/c` pass):** `env_vendor_attributes.h`, now
  converted, is the env layer's home for vendor attribute spellings, and its
  banner names `D_INLINE`, `D_NOINLINE` and `D_RESTRICT` as left to
  `djinterp.h` on purpose. If the spellings move, that header is a closer fit
  than `env_compiler.h`, and its banner's list changes with them.

**7. What `D_ENV_LANG_USING_C` means.**
It is 1 under C++ too, and `D_ENV_LANG_C_STANDARD` reads as C90 there, so
`env_printer.hpp` reports "Using C: yes" in C++ builds.

> **Status, 2026.10.04:** settled 2026.10.04: "redefine it":
> `D_ENV_LANG_USING_C` means compiling as C, 0 under C++ (`56cd674`).

- *Advantages (redefine it as "compiling as C"):* the name becomes true.
- *Disadvantages:* the printer is the only known user, but the tree may have
  others.
- **Recommendation:** redefine it, after a grep of the tree.

**8. `<unistd.h>` in every translation unit.**
The POSIX detection fix includes it unconditionally.

> **Status, 2026.10.04:** settled 2026.10.04: "opt in switch":
> `D_CFG_ENV_POSIX_UNISTD`, default 0, decides whether `env_posix.h` includes
> `<unistd.h>` (`ea03a90`). At 0 the XSI version and the `_POSIX_*` options read
> 0 unless a unit includes `<unistd.h>` first; `env.c` now includes `<stdio.h>`,
> which it had reached through `<unistd.h>`.

- *Advantages (an opt-out switch):* a file could avoid its declarations.
- *Disadvantages:* opting out brings back the include-order bug.
- **Recommendation:** no switch unless a name collision actually shows up.

**9. Switch naming.**
`D_CFG_APPLE_UNIVERSAL` and `D_ENV_PRINTER_INCLUDE_*` break the `D_CFG_ENV_*`
convention.

> **Status, 2026.10.04:** settled 2026.10.04: renamed
> `D_CFG_ENV_APPLE_UNIVERSAL`, in its own config,
> `config/core/env/os/cfg_env_apple.h`, and read as 0 or 1 (`269aed9`); the old
> `defined()` test turned it on even at 0. `D_ENV_PRINTER_INCLUDE_*` left with
> `env_printer.hpp`.

- *Advantages (rename):* consistency. Defining the new name from the old
  whenever only the old is set would keep existing `-D` flags working, but
  with nothing in production the old names can simply go.
- *Disadvantages:* leaving them costs nothing now, but stays inconsistent.
- **Recommendation:** rename.

**10. Internal macro names.**
`D_INTERNAL_COMPRESSION_PROBE`, `_ARCHIVE_PROBE`, `_PDF_PROBE` and
`D_INTERNAL_ACCESS` lack the `ENV_` segment, and `D_INTERNAL_ACCESS` is
generic enough to collide.

> **Status, 2026.10.04:** settled 2026.10.04: "rename": the four named, and four
> siblings in the same headers, carry `D_INTERNAL_ENV_` (`6cbe581`).

- *Advantages (rename):* conformance, and less collision risk.
- *Disadvantages:* the implementation files, not yet seen, may use them.
- **Recommendation:** rename once those `.c` files are available.

#### Bug and behavior decisions (string, time, printer)

**11. `d_strerror_r` knows only three error numbers.**

> **Status, 2026.10.04:** settled 2026.10.04: delegated: the error space being
> built defines the framework's own codes, not errno values, so it is not
> pertinent; d_strerror_r asks the platform (strerror_r, glibc's GNU variant,
> strerror_s), with the table as fallback (`c787f83`).

- *Advantages (delegate to `strerror_r`, or `strerror_s` on Windows, with the
  table as fallback):* real messages.
- *Disadvantages:* the text varies by platform, and glibc has two
  incompatible `strerror_r` variants to steer between.
- **Recommendation:** delegate.

**12. Locale-dependent character handling in `string_fn`.**
The alpha, alnum and whitespace validators, and the case-insensitive and
case-conversion functions, follow the C locale through `tolower`/`isalpha`.

> **Status, 2026.10.04:** settled 2026.10.04: "ascii only": string_fn's classes
> and case folding (`a042abc`, wrapped in `5922876`).

- *Advantages (ASCII-only):* deterministic across machines.
- *Disadvantages:* 8-bit letters are no longer handled.
- **Recommendation:** ASCII-only across `string_fn`.

**13. `d_string_tokenize` leaves `size` stale.**
It writes NULs into the text without updating the size.

> **Status, 2026.10.04:** settled 2026.10.04: "update size": the size follows
> the text after the first call (`77ec2c5`).

- *Advantages (set `size` to the first token's length):* size and text stay
  consistent.
- *Alternative (document the string as consumed):* no code change, but a
  wrong size is left behind.
- **Recommendation:** update `size`.

**14. `d_timespec_to_ns` overflow.**

> **Status, 2026.10.04:** settled 2026.10.04: the precondition, plus a
> debug-build assert (`6c97fcd`).

- *Advantages (keep the documented precondition):* costs nothing, and ±292
  years is ample.
- *Alternative (saturate):* never undefined behavior, but hides nonsense
  inputs.
- **Recommendation:** the precondition, plus a debug-build assert.

**15. The fallback `d_nanosleep` calls POSIX `sleep()`,** exactly where POSIX
is missing.

> **Status, 2026.10.04:** settled 2026.10.04: thrd_sleep where C11 threads
> exist, POSIX sleep() where only it does, else ENOSYS (`27da03a`).

- *Advantages (C11 `thrd_sleep` where threads exist, failing with `ENOSYS`
  otherwise):* compiles everywhere and reports honestly.
- *Alternative (`#error`):* forces the decision at port time.
- **Recommendation:** `thrd_sleep` with the `ENOSYS` fallback.
- **Update (2026-09-28, #5):** at a C99 floor, `thrd_sleep` is a tier, not
  the baseline: it applies where the translation unit is C11 or later and
  `<threads.h>` exists (`__STDC_NO_THREADS__` undefined), and every other
  build without POSIX takes the `ENOSYS` path. The recommendation stands.

**16. Where glibc's feature-test macros come from (`dtime.c`).**

> **Status, 2026.10.04:** settled 2026.10.04: (B) the rule is in the C guide:
> feature-test macros come from the build; where the C library hides POSIX in a
> strict mode (glibc, musl), every build defines `_XOPEN_SOURCE=700`, and
> elsewhere it stays undefined. `dtime.c`, `dmutex.h` and `c/fs/file.h` carry a
> note (`f028dde`); "following your recommendation", reaffirmed in the second
> batch.

- *Advantages (build flags, `-D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE`):*
  respect the corresponding-header-first rule, and keep reserved names out of
  source.
- *Disadvantages:* every build must set them.
- *Alternative (define them at the top of `dtime.c`):* self-contained, but
  breaks both rules.
- **Recommendation:** build flags, noted in `dtime.c`'s banner.

**17. `env_printer.hpp`'s `FILE*` overloads** print a much smaller report than
the templates.

> **Status, 2026.10.04:** settled by the downporting: `env_printer.hpp` was
> retired (`58add73`, the owner's ruling in round 2).

- *Advantages (forward them to the templates through a small `FILE*`
  target):* one report everywhere.
- *Disadvantages:* a small adapter to write.
- **Recommendation:** forward.

**18. The six-parameter helper in `env_printer.hpp`.**

> **Status, 2026.10.04:** settled by the downporting: `env_printer.hpp` was
> retired (`58add73`, the owner's ruling in round 2).

- *Advantages (a feature-descriptor struct):* meets the five-parameter limit,
  and turns the C++ feature list into a table.
- *Disadvantages:* a restructure of the printer's feature list.
- **Recommendation:** the struct.

**19. Two long functions in `dstring`.**
`d_string_split` is 113 lines and `d_string_replace_all_cstr` 97, against the
guide's 60.

> **Status, 2026.10.04:** settled 2026.10.04: "keep as is".

- *Advantages (split into helpers):* conforms, and is easier to test. The
  existing differential harness guards the change.
- *Disadvantages:* churn in working code.
- **Recommendation:** split.

**20. Add `-Wundef` to the guide's warning flags.**
Several bugs found were undefined macros silently reading as 0 in `#if`.

> **Status, 2026.10.04:** settled 2026.10.04: both guides' flag tables carry
> `-Wundef` (`0accfda`); the fallout was cleared in the downporting (`205ed83`).

- *Advantages:* catches that whole class of bug.
- *Disadvantages:* some initial warnings to clear in the env headers.
- **Recommendation:** add it, and clear the fallout.
- **Update (2026-09-27, `env/c` pass):** on GCC 13 and clang 18, `env.h`
  draws three `-Wundef` warnings in a C11 build and one in C++17.
  `env_lang.h`'s `D_DELETE` and `env_long_long.h` both read
  `D_ENV_LANG_IS_CPP11_OR_HIGHER`, which `env_lang.h` defines only in C++
  builds; defining the C++ predicates as 0 in C builds would clear both in one
  place. The third, in both languages, is a real bug: #88.
  `env_vendor_attributes.h` draws none.

#### Bug fixes (21–32)

Each changes behavior only where the current behavior is wrong. For every one:
- *Advantages:* correct behavior.
- *Disadvantages:* code that depends on the bug changes, and nothing is in
  production.
- **Recommendation:** fix all of them.

21. `D_IS_VALID_INDEX` negates the index, which is undefined behavior for the
    most negative value. It and its two siblings should be defined through
    `d_index_is_valid`.
    *Status, 2026.10.04: settled 2026.10.04: the three macros call
    d_index_is_valid (`6c433bb`).*
22. `dstring`: a `SIZE_MAX` count overflows in substr, erase, replace and
    replace_cstr. Clamp it to the remaining length, as `std::string` does with
    `npos`.
    *Status, 2026.10.04: settled 2026.10.04: clamped against what is left, so
    SIZE_MAX cannot wrap (`a8a474f`).*
23. `dstring`: appending, inserting or copying a string into itself hits an
    overlapping `memcpy`. Handle the overlap.
    *Status, 2026.10.04: settled 2026.10.04: append, insert and assign take the
    string itself; the _s forms keep their precondition (`8c0eb5d`).*
24. `d_string_split` returns a non-NULL array with a count of 0 for input that
    is only delimiters. Return NULL.
    *Status, 2026.10.04: settled 2026.10.04: no token, no array: *_tokens is
    NULL (`bb5ee58`).*
25. `D_TIME_HAS_TIMEGM` is 0 on Linux and macOS, so both use the fallback,
    which, unlike `timegm`, doesn't normalize the `struct tm`.
    *Status, 2026.10.04: settled 2026.10.04: timegm wherever `<time.h>` declares
    it under the build's feature-test macros, and a fallback that normalizes
    (`b773090`).*
26. Windows: the clock functions reset their frequency cache on every call,
    and the CPU-time clocks drop a carry.
    *Status, 2026.10.04: settled 2026.10.04: the frequency is cached, and the
    CPU times add as 64-bit counts (`8e81b88`); compile-checked with MinGW-w64
    only.*
27. `d_strftime_s`'s MSVC branch calls a function that doesn't exist, behind a
    macro nothing defines. Delete the branch, and replace `D_ENV_MSC_VER` with
    the env layer's compiler-version macros.
    *Status, 2026.10.04: settled 2026.10.04: the strftime_s branch is gone, and
    timespec_get is detected by TIME_UTC instead of a compiler version
    (`68a4826`).*
28. `d_timespec_cmp` returns -1 when either argument alone is NULL. Make NULL
    order first on both sides, so the ordering is safe for sorting.
    *Status, 2026.10.04: settled 2026.10.04: NULL orders first on both sides
    (`9812551`).*
29. The `_TIMESPEC_DEFINED` shim for MSVC before 2015 defines a reserved name.
    Delete it; it is below the C11 floor anyway if #5 is taken.
    *Update (2026-09-28, #5):* the floor is C99, and the shim is below that
    too: it serves `_MSC_VER < 1900`, and the root accepts MSVC's C mode only
    from Visual Studio 2015 (1900) on. Still delete it.
    *Status, 2026.10.04: settled 2026.10.04: the pre-2015 MSVC shim is gone
    (`eb8e488`).*
30. `d_strcasestr_index` misreads embedded NULs. Compare with the
    length-aware `d_strncasecmp_n` instead.
    *Status, 2026.10.04: settled 2026.10.04: compares through d_strncasecmp_n,
    embedded NULs included (`e5b1fb8`).*
31. `env_archive.h`'s stray `#include <archive.h>` fires only if the build
    pre-defines the detection macro. Delete it.
    *Status, 2026.10.04: settled 2026.10.04: the stray include is gone
    (`9ada003`).*
32. `env_printer.hpp`: `#ifdef __STDC_HOSTED__` should be `#if` (done in the
    converted file). `print_env` should also call the C++11 feature list when
    features are enabled, or the list should go.
    *Status, 2026.10.04: settled by the downporting: `env_printer.hpp` was
    retired (`58add73`, the owner's ruling in round 2).*

#### Style guide contradictions

**33. Contradictions in the guides.** Both guides contain these; each pick
below would be applied to both.

> **Status, 2026.10.04:** settled 2026.10.04: a to i as ruled (`476c0ae`): a,
> the /c/ examples are `[c]`; b, a declaration's `/** */` contract is a
> permitted block comment; c, a void contract omits `@return`; d, the fourth
> level is decimal; e, a source's own header is its corresponding header; f,
> CamelCase template parameters aligned on the first letter, and the typo fixed;
> g, only an `#endif` closing `#ifdef` or `#ifndef` names its symbol; h, a
> return type too long to pad may take its own line where that improves
> readability, kept as neat as the padded form; i, the example is a sample
> module, `c/foo/bar`. j, the banner spacing, stays as it is ("it's fine"). The
> language floor was settled earlier (programme 4.9).

- *Advantages of resolving:* one rule per question, so conversions stop
  depending on which rule was read.
- *Disadvantages:* none beyond the editing.
- **Recommendation:**
  - **Banner tag.** The examples say `[core]` for files under `/c/`, but the
    tag rule says `[c]`. Fix the examples.
  - **Comment rules.** "Block comments only for banner and ToC" and "no
    comments above individual declarations" conflict with the `/** */`
    contract rule. The contract rule should win.
  - **Void functions.** One rule says "document the return as 'none.'";
    another says omit `@return` entirely. Omit it, which is what
    `-Wdocumentation` expects.
  - **Fourth-level ToC entries.** Letters (`a.`) versus decimal (`#.#.#.#`).
    Use decimal, which is what is implemented; drop the letter rule, including
    the `a.` in the C++ guide.
  - **`djinterp.c`'s own header.** "framework root" versus "corresponding
    header" as the include comment. The corresponding-header rule should win
    for a file's own header.
  - **C++ template example.** It uses `_Iterator`, `_ConstIterator` and
    `_DifferenceType`, which the reserved-identifier rule forbids. Rename them,
    and fix the "guideI" typo in both guides.
  - **`#endif` comments** *(added 2026-09-26)*. The preprocessor section says
    every `#endif` names its controlling symbol, while Conditional-Block
    Closures requires it only for `#ifdef`/`#ifndef` and exempts expression
    `#if`s. Keep the Closures rule, which is what every file follows, and
    reword the preprocessor bullet.
  - **Declaration layout exception** *(added 2026-09-26; see #54)*. When
    padding a declaration would pass column 80, allow the return type on its
    own line.
  - **Example include guard** *(added 2026-09-27, relay 93)*. The C guide's
    example guard is `PROJECT_HEADER_FILE_`, which the guard derivation in the
    same guide forbids and which no header in the tree could legally carry.
    Replace it with a derived one, such as `DJINTERP_C_PROJECT_HEADER_FILE_H`.
    The C++ guide has no such example.
  - **Banner spacing** *(added 2026-09-27, `env/c` pass)*. Both guides'
    banner templates put one blank `*` line before `path:`; all 1,880 header
    banners under `inc/djinterp` have two. Change the templates to two, which
    is what every file carries.
  - **Language floor** *(added 2026-09-28, #5)*. The C guide's Language
    Standard section says C11, and its Assertions section says to spell a
    static assertion `static_assert` from `<assert.h>`, which C99 lacks.
    Make the target C99 and the spelling `D_STATIC_ASSERT`, which the root
    defines at every level. The C++ guide's C++20 becomes C++11.

#### Windows, dio and Linux headers (34–41)

**34. Windows target-version detection depends on include order,** as on
Apple. On MSVC with nothing included first, the header reports "Windows
(Unspecified)" and every version-gated feature (CNG, SRW locks, and so on)
as 0. Defining `_WIN32_WINNT=0x0A00` gives "Windows 10+" with all of them 1.

> **Status, 2026.10.04:** settled 2026.10.04: "define `_WIN32_WINNT`", in the
> build: both guides and `env_windows.h`'s banner say so, and the header never
> sets one (`484007e`).

- *Option (include `<sdkddkver.h>` in the header):* fixes it, but that header
  sets a default `_WIN32_WINNT`, so a file that defines its own afterwards gets
  a redefinition.
- *Option (define `_WIN32_WINNT` in the build):* standard Windows practice;
  the header documents the dependency.
- **Recommendation:** define `_WIN32_WINNT` in the build.

**35. Store apps report both UWP and desktop.** The desktop test counts
`WINAPI_FAMILY_PC_APP`, which current SDKs make equal to `WINAPI_FAMILY_APP`.

> **Status, 2026.10.04:** settled 2026.10.04: "drop it": only
> WINAPI_FAMILY_DESKTOP_APP counts as desktop (`86272bd`).

- *Advantages (drop it from the desktop test):* store apps classify
  correctly.
- *Disadvantages:* none known.
- **Recommendation:** drop it.

**36. `D_ENV_WIN_HAS_CPPWINRT` is 1 on Linux** in any C++20 file that
includes `<version>`, because it keys on `__cpp_lib_coroutine`.

> **Status, 2026.10.04:** settled 2026.10.04: "require both": a Windows C++
> build with `winrt/base.h` (`c3b980d`).

- *Advantages (require Windows plus C++/WinRT's own header):* correct.
- *Disadvantages:* none known.
- **Recommendation:** require both.

**37. Two targets are misdetected.** MinGW on ARM64 reports not-ARM, because
`_M_ARM64` is MSVC-only. 64-bit Cygwin reports neither 64- nor 32-bit, though
the same header treats Cygwin as Windows.

> **Status, 2026.10.04:** settled 2026.10.04: the GNU spellings for Windows on
> ARM; Cygwin counts as Windows for bitness (`0216af1`).

- *Advantages (add the GNU spellings):* both targets detect correctly.
- *Disadvantages:* someone must decide whether Cygwin counts as Windows for
  bitness.
- **Recommendation:** add the GNU spellings, and decide the Cygwin question.

**38. `d_sscanf_s`, `d_vsscanf_s` and `d_fscanf_s` crash on correctly written
calls.** `D_STUDIO_HAS_SCANF_S` is defined nowhere, and is probably a typo for
`STDIO`. So they always use plain `vsscanf`/`vfscanf`, which don't consume the
buffer size that follows each `%s`, `%c` or `%[`. Reproduced: a correct
`"%7s %d"` call segfaults, writing through the size as a pointer.

> **Status, 2026.10.04:** settled 2026.10.04: (B) the checked scanf family is
> the framework's own and the same everywhere: each directive goes to the
> library's plain `vsscanf` or `vfscanf`, and the engine walks the arguments and
> holds `%c`, `%s` and `%[` to a size_t size (`ccf0af6`). The typo
> `D_STUDIO_HAS_SCANF_S` is gone.

- *Advantages (detect support properly, via `__STDC_LIB_EXT1__` or `_MSC_VER`,
  and make the `_s` scanf wrappers a compile error where neither exists):*
  correct, and fails loudly where unsupported.
- *Alternative (emulate the convention in the fallback):* means parsing the
  format string yourself.
- **Recommendation:** proper detection plus the compile error. Each contract
  carries a warning for now.

**39. `dio.c` calls `d_file_tell_stream`/`d_file_seek_stream` without
including the header that declares them.** It gets them only through
`dio.h`'s `fs/dfile.h`, yet `dio.h`'s own note says the seek functions live in
`fs/file_seek.h`. Neither file has been seen.

> **Status, 2026.10.04:** settled 2026.10.04: dio's stream positioning, which
> only wrapped c/fs's, is removed, and with it dio's use of c/fs; the rest of
> dio has no c/fs counterpart and stays (`a28c1ef`).

- *Advantages (include the declaring header directly):* include-what-you-use,
  and no silent breakage if `dio.h` changes.
- *Disadvantages:* needs `fs/` to confirm which header declares them.
- **Recommendation:** include it directly once `fs/` is available.

**40. `env_linux.h`: `<linux/version.h>` is never included,** so every
kernel-version gate reads 0 unless the file included it first. Including it
first flips io_uring, openat2, clone3 and the rest from 0 to 1.

> **Status, 2026.10.04:** settled 2026.10.04: `env_linux.h` includes
> `<linux/version.h>` behind __has_include (`2df9eab`).

- *Advantages (include it behind `__has_include`):* order-independent.
  Unlike Windows' `<sdkddkver.h>`, it only defines macros and sets no defaults,
  so there is no redefinition hazard.
- *Disadvantages:* none known.
- **Recommendation:** include it.

**41. musl detection keys on a `__MUSL__` the build must define,** since musl
deliberately has no identifying macro. A double-underscore name is reserved,
so the project's own guide forbids defining it.

> **Status, 2026.10.04:** settled 2026.10.04: `D_CFG_ENV_LINUX_MUSL`, in
> `cfg_env_linux.h`, replaces __MUSL__ (`8b9ba91`).

- *Option (guess by elimination: no glibc, no uClibc, no Bionic):* mislabels
  every other C library.
- *Option (an explicit switch, `D_CFG_ENV_LINUX_MUSL`):* honest, and conforms.
- **Recommendation:** the explicit switch.

#### BSD and Apple (42–43)

**42. `env_bsd.h`: version detection needs `<sys/param.h>`, which nothing
includes.** Measured effect of knowing the version: nothing changes on FreeBSD
(clang's predefined major version suffices) or DragonFly; one flag on OpenBSD
(`mimmutable`); five on NetBSD (DTrace, `explicit_bzero`, `getentropy`,
`reallocarray`, `sysctlbyname`).

> **Status, 2026.10.04:** settled 2026.10.04: `<osreldate.h>` on FreeBSD,
> `<sys/param.h>` on the others, behind __has_include (`aea72dc`).

- *Advantages (include per variant, behind `__has_include`):* order-independent,
  consistent with #4 and #40. `<osreldate.h>` on FreeBSD defines only
  `__FreeBSD_version`.
- *Disadvantages:* on the others, `<sys/param.h>` also defines `MIN`, `MAX`,
  `nitems`, `howmany` and friends in every file that includes `env_bsd.h`.
- **Recommendation:** `<osreldate.h>` on FreeBSD, `<sys/param.h>` on the
  others.

**43. visionOS detection in `env_apple.h` rests on unverified `TARGET_OS_*`
values.**

> **Status, 2026.10.04:** settled 2026.10.04: visionOS by TARGET_OS_VISION or
> __is_target_os(xros), kept out of the macOS and iOS fallbacks; checked against
> Apple's own `<TargetConditionals.h>` with clang 18, not yet with a visionOS
> SDK (`5cb980e`).

- With stand-in values (`TARGET_OS_IPHONE=1`, `TARGET_OS_VISION=1`,
  `TARGET_OS_IOS=0`), the fallback iOS branch fires, so visionOS also reports
  as iOS. That branch is an `#elif` that excludes tvOS and watchOS but not
  visionOS, and it runs whenever `TARGET_OS_IOS` is 0, not only when it is
  undefined.
- With clang 18's own predefined values (`IPHONE=0`, no `TARGET_OS_VISION`),
  visionOS reports as macOS.
- Apple's real `TargetConditionals.h` has not been checked.
- *Advantages (fix it):* correct classification whatever the SDK. The fix: run
  the fallback only when `TARGET_OS_IOS` is undefined, exclude visionOS, and
  detect visionOS from the compiler too (`__is_target_os(xros)`).
- *Disadvantages:* needs confirming on a real visionOS SDK.
- **Recommendation:** fix, then verify on a real SDK.

#### TLS (44, 60)

**44. `D_ENV_TLS_OPENSSL_AT_LEAST` reports TLS 1.3 for OpenSSL 1.1.0a–1.1.0l.**
It always packs its threshold the 3.x way (`0xMNN00PP0`). But 1.x packs
versions as `0xMNNFFPPS`, with a patch *letter* where 3.x has the patch
number, so `(1, 1, 1)` becomes `0x10100010`, which is 1.1.0a. Reproduced with
real version numbers. The 1.1.0 gates (hostname validation, implicit init) are
unaffected.

> **Status, 2026.10.04:** other sessions': net's: the owner's rulings of
> 2026.10.04 are being carried out in the net session.

- *Option (a), fix the helper to pack by release line (3.x packing from 3.0 on,
  `(major << 28) | (minor << 20) | (patch << 12)` before):*
  - *Advantages:* every `AT_LEAST` call becomes right. Checked: 1.1.0l
    (`0x101000cf`) fails `(1, 1, 1)`, and 1.1.1 (`0x1010100f`) meets it.
  - *Disadvantages:* `(1, x, y)` changes meaning for 1.x, but the current
    meaning is wrong.
- *Option (b), patch only the TLS 1.3 gate against `0x10101000L`:*
  - *Advantages:* minimal.
  - *Disadvantages:* the helper stays wrong for others, and it adds a magic
    number.
- *Option (c), leave it documented:*
  - *Advantages:* no work, and 1.1.0 has been end-of-life since 2019.
  - *Disadvantages:* silent wrongness is what the env layer exists to prevent.
- **Recommendation:** (a).

**60. Library-aware TLS capability flags** *(raised with #44)*.
`D_ENV_TLS_HAS_TLS1_3`, `_HOSTNAME_VALIDATION` and `_IMPLICIT_INIT` describe
the OpenSSL family only, and read 0 for the other five libraries even where
they can.

> **Status, 2026.10.04:** other sessions': net's: the owner's rulings of
> 2026.10.04 are being carried out in the net session.

- *Option (a), keep them OpenSSL-only (as now documented):*
  - *Advantages:* accurate for what they claim.
  - *Disadvantages:* users of the other libraries always get "no", and must
    special-case the backend.
- *Option (b), make each flag describe the selected library:*
  - *Advantages:* answers what callers ask, and fits the two-axis design.
  - *Disadvantages:* several answers are not version-gated.
    - mbedTLS's TLS 1.3 depends on `MBEDTLS_SSL_PROTO_TLS1_3`, and wolfSSL's on
      `WOLFSSL_TLS13`.
    - Schannel's depends on the Windows version at runtime.
    - Secure Transport never supported TLS 1.3.
    - It also needs each library's version/config header, and "implicit init"
      doesn't translate (wolfSSL requires `wolfSSL_Init()`).
- *Option (c), per-library flags plus a roll-up for the selected library,
  reporting runtime-dependent cases as 0 alongside a runtime query:*
  - *Advantages:* explicit, testable, and honest about compile-time limits.
  - *Disadvantages:* the most macros to maintain.
- **Recommendation:** (c), starting with TLS 1.3. Hostname validation is
  cheap to extend. Rename implicit init to
  `D_ENV_TLS_OPENSSL_HAS_IMPLICIT_INIT`. Verify the library version facts
  against each library's documentation when implementing.

#### Qt (46–49)

**46. `env_qt.h` defines Qt's own `QT_VERSION_CHECK`** when no Qt header has.
Verified: included before `<QtGlobal>`, both compilers warn "`QT_VERSION_CHECK`
redefined" (an error under `-Werror`), because Qt's replacement list is spelled
without spaces. With no Qt at all, `#ifdef QT_VERSION_CHECK` wrongly says Qt is
present.

> **Status, 2026.10.04:** open, unchanged: nothing in the tree addresses it yet.

- *Advantages (replace it with a prefixed `D_ENV_QT_VERSION_CHECK`, parameters
  parenthesized, never defining Qt's name):* no clash in either include order,
  no false Qt signal, and safe with expression arguments.
- *Disadvantages:* code using `QT_VERSION_CHECK` without Qt must switch names.
- **Recommendation:** replace it.

**47. Qt headers must be included before `env_qt.h`,** or Qt reads as absent
even when linked.

> **Status, 2026.10.04:** open, unchanged: nothing in the tree addresses it yet.

- *Advantages (probe the version header automatically, with an opt-out
  switch, as curl and TLS do):* order-independent, and consistent with #4,
  #40 and #42.
- *Disadvantages:*
  - Qt headers are C++-only, so C files can't probe.
  - Only Qt 6.5+ has a small standalone version header, believed to be
    `<QtCore/qtversion.h>`; verify.
  - Older Qt needs the heavy `qglobal.h`.
  - Only the version benefits, since module flags come from the build.
- **Recommendation:** probe on Qt 6.5+ in C++ only, with an opt-out; keep the
  documented requirement for older Qt.

**48. `env_qt.h` has no `#ifndef` guards,** unlike every other env module, so
detection can't be overridden.

> **Status, 2026.10.04:** open, unchanged: nothing in the tree addresses it yet.

- *Advantages (guard every macro):* consistency, and lets cross-builds and
  tests pin a Qt configuration. Costs nothing when unused.
- *Disadvantages:* the grouped definitions (such as the series flags set
  together per branch) need restructuring, or overriding `D_ENV_QT_VER`
  wouldn't update `D_ENV_QT_IS_QT6`.
- **Recommendation:** guard every macro, and derive the grouped ones from
  `D_ENV_QT_VER`.

**49. `D_ENV_QT_HAS_GUI` reports 1 for any Qt build unless `QT_NO_GUI`** (a
Qt 4-era macro) is defined. Verified: a Qt 6 console application linking only
QtCore reports `HAS_GUI=1`.

> **Status, 2026.10.04:** open, unchanged: nothing in the tree addresses it yet.

- *Advantages (key on `QT_GUI_LIB`, like every other module):* accurate and
  consistent, and Qt 4's qmake defines it too.
- *Disadvantages:* builds not using qmake/CMake Qt targets define no `_LIB`
  macros, so GUI reads as absent. All the other module flags already behave
  that way.
- **Recommendation:** switch to `QT_GUI_LIB`.

#### Curses (50–53)

The original `env_curses.h` did not compile: it used OS predicates that don't
exist. The converted file compiles, using the `env_os.h` constants
(`D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX` / `_MACOS`, and the BSD block test).
Section III's four guidance blocks are now one `#if`/`#elif` chain; before, on
Linux, whose identifier is in the Unix block, both the Linux and "other Unix"
blocks fired and the System V answer won.

**50. Where the Linux/macOS/BSD OS predicates live.**

> **Status, 2026.10.04:** open, unchanged: nothing in the tree addresses it yet.

- *Advantages (add `D_ENV_IS_OS_LINUX`, `_MACOS`, `_BSD` to `env_os.h` beside
  `_WINDOWS`):* reads naturally, and any other file in the tree that assumed
  these names compiles too.
- *Disadvantages:* a larger `env_os.h` API, and semantic choices to make: does
  Linux include Android, and does macOS include Mac Catalyst?
- **Recommendation:** add them, then switch `env_curses.h` back to them.

**51. Narrow ncurses is reported as wide.** The header tests
`defined(NCURSES_WIDECHAR)`, but ncurses always defines it, as 1 or 0.
Verified with real ncurses 6.4: plain `<curses.h>` reports `NCURSESW`, with
the wide feature flag set.

> **Status, 2026.10.04:** open, unchanged: nothing in the tree addresses it yet.

- *Advantages (test the value):* correct type and feature flags.
- *Disadvantages:* none, beyond anything relying on the wrong answer.
- **Recommendation:** fix.

**52. `env_curses.h` discards overrides.** Detection un-defines a pre-defined
`D_ENV_CURSES_TYPE`/`_FEATURES`; verified, a pre-defined BSD type became
`NCURSESW`. `D_ENV_CURSES_NAME`/`_AVAILABLE` are unguarded, so pre-defining
them gives a redefinition warning.

> **Status, 2026.10.04:** open, unchanged: nothing in the tree addresses it yet.

- *Advantages (guard every macro):* the same override behavior as the rest of
  the env layer.
- *Disadvantages:* the detection chain needs restructuring.
- **Recommendation:** guard every macro, as in #48.

**53. PDCursesMod is identified by `PDC_WIDE`.** That macro marks a
wide-character build, which classic PDCurses has too, so a wide PDCurses
reports PDCursesMod and a narrow PDCursesMod reports PDCurses.

> **Status, 2026.10.04:** open, unchanged: nothing in the tree addresses it yet.

- *Advantages (use the version instead, believed to be 4.x for PDCursesMod
  and 3.x for classic PDCurses, with wide support as a feature flag):* correct
  naming and features.
- *Disadvantages:* the version claim needs confirming against PDCursesMod's
  `curses.h`.
- **Recommendation:** fix, after that check.

#### STARTTLS module and framework (54–59)

Files: `env/net/env_starttls.h`, `config/net/starttls/cfg_starttls.h`,
`net/starttls/starttls.h`, `src/.../net/starttls/starttls.c`. The audit found
them in very good shape.
- **Style:** banners, guards, tables of contents and structure all conform.
  All 87 definitions pass every mechanical rule checked.
- **Build:** with real OpenSSL 3.0.13 and the two framework macros of #58 and
  #59 supplied, `starttls.c` builds with zero warnings under the full flag set
  (GCC/clang, C11/C17/C2x), and `starttls.h` is clean for C++11 to C++23.
- **Exports:** all 27 declared functions are defined, and nothing else is
  exported.

**54. `starttls.h` declaration layout.** All 27 declarations put the return
type on its own line; the guide's rule 3 wants it padded on the same line, with
names aligned down a block. With `D_NODISCARD enum d_starttls_status` taking 35
columns, 23 of 27 would pass column 80, and moving `D_NODISCARD` to its own
line still leaves 15.

> **Status, 2026.10.04:** other sessions': net's: the owner's rulings of
> 2026.10.04 are being carried out in the net session.

- *Advantages (convert):* literal conformance, matching every other header.
- *Disadvantages:* trades rule 3 for another rule, either line length or
  aligned parameters.
- **Recommendation:** keep this layout, and add the exception to the guide
  (see #33).

**55. `cfg_starttls.h` writes nested directives hash-first (`#   define`).**
It's the only file in the project that does, with 30 directives, against
thousands indent-first. The guide never states the rule, but all its examples
indent first.

> **Status, 2026.10.04:** other sessions': net's: the owner's rulings of
> 2026.10.04 are being carried out in the net session.

- *Advantages (convert to `    #define`):* consistency.
- *Disadvantages:* none; it's mechanical.
- **Recommendation:** convert.

**56. `cfg_starttls.h` promises "the module never tests a `D_CFG_*` or
`D_ENV_*` symbol itself", and `starttls.c` breaks it.** It tests
`D_ENV_STARTTLS_HAS_SYS_IOCTL_H`/`_FILIO_H` in its include block, and reads
`D_ENV_TLS_OPENSSL_VARIANT_NAME` from `env_tls.h`, which it doesn't include
itself. The `D_CFG_` static assertions are documented and intentional.

> **Status, 2026.10.04:** other sessions': net's: the owner's rulings of
> 2026.10.04 are being carried out in the net session.

- *Option (derive `D_INTERNAL_` values for the three in section 5):*
  - *Advantages:* the promise holds, and the module reads a single layer.
  - *Disadvantages:* three more derived macros.
- *Option (loosen the wording):*
  - *Advantages:* no code change.
  - *Disadvantages:* a weaker rule, and a transitive include left in place.
- **Recommendation:** derive them.

**57. Two functions in `starttls.c` exceed 60 lines:**
`d_starttls_send_linef` (63) and `d_starttls_upgrade` (65). Both have natural
seams: the heap re-format path in the first, and the state checks that open
the second.

> **Status, 2026.10.04:** other sessions': net's: the owner's rulings of
> 2026.10.04 are being carried out in the net session.

- *Advantages (extract a helper each):* meets the guide's limit with small,
  nameable helpers.
- *Disadvantages:* a little more indirection in two readable, linear
  functions.
- *Alternative:* keep them, and state the reason in each mechanism comment, as
  the guide permits.
- **Recommendation:** extract; the overshoot is small and the seams are real.

**58. `D_NODISCARD` in `djinterp.h` breaks `-pedantic` builds (framework
bug).** It picks `[[nodiscard]]` whenever `__has_c_attribute` or
`__has_cpp_attribute` says yes, and GCC and clang say yes in C11, C17 and
C++11 as an extension. So `-pedantic` fails: "`[[]]` attributes are a C23
extension", and in C++11 "nodiscard is a C++17 extension". Every C file using
it before C23 fails the warning-clean baseline; `starttls.h` is the first user
from C.

> **Status, 2026.10.04:** settled by the downporting: one `D_NODISCARD`, in
> `env_attributes.h`, gated on language and level (`6a7b024`, `b4db405`;
> programme 4.3). The net session carries on from this ruling for net's headers.

- *Advantages (use the standard spelling only from C23 and C++17, falling
  back to `__attribute__((warn_unused_result))` on GCC/clang):* clean
  everywhere, with the same effect.
- *Disadvantages:* MSVC in C++14 mode loses the attribute, which it barely
  supports there.
- **Recommendation:** fix.
- **Update:** superseded by #65. `env_attributes.h`, the framework's attribute
  layer, has the same flaw in almost every attribute, and `djinterp.h`'s copy
  of `D_NODISCARD` goes away under #64.
- **Update (2026-09-28, C99 merge):** the C half is done. `djinterp.h` picks
  `[[nodiscard]]` in C only at C23 and no longer consults
  `__has_c_attribute`. The C++ half is open: at C++11 and C++14,
  `__has_cpp_attribute(nodiscard)` still selects the spelling `-pedantic`
  calls a C++17 extension.

**59. `D_FORMAT_PRINTF` is undefined in every framework header seen**
(`djinterp.h` original and converted, `djinterp.hpp`, `dmacro.h`), so
`starttls.h` doesn't compile in C or C++. If the owner's current `djinterp.h`
defines it, obtain that version and re-verify.

> **Status, 2026.10.04:** settled 2026.10.04: closed with #64: `D_FORMAT_PRINTF`
> stays in `env_vendor_attributes.h`, which a user includes. STARTTLS is not in
> this tree, and nothing uses the macro. The net session carries on from this
> ruling for net's headers.

- *Advantages (add it to `djinterp.h` beside `D_NODISCARD`, pre-definable, as
  `__attribute__((format(printf, f, a)))` on GCC/clang and empty elsewhere):*
  compile-time checking of every `d_starttls_send_linef` call's arguments.
- *Disadvantages:* no MSVC equivalent in that position (SAL's
  `_Printf_format_string_` attaches to the parameter).
- **Recommendation:** add it.
- **Update (2026-09-27, `env/c` pass):** it already exists.
  `env_vendor_attributes.h` defines `D_FORMAT_PRINTF` (and `D_FORMAT_SCANF`)
  exactly as recommended: pre-definable, `__attribute__((format(printf, f,
  a)))` on GCC and Clang, empty elsewhere. Restoring that header's include
  under #64 supplies it. Nothing should be added to `djinterp.h`: a second
  definition is what the guide's one-definition rule for macros forbids. The
  recommendation becomes: take #64.

#### JIT (61–63)

`env_jit.h` was converted and verified identical to its original across 18
targets (every architecture it names, plus Apple, iOS, Windows, OpenBSD and
wasm). Its detection is correct on all of them. These three came up.

**61. `env_jit.h`'s JIT prohibition depends on include order.**
`D_ENV_JIT_PROHIBITED` reads `env_ios.h`'s `D_ENV_MOBILE_NO_JIT`, which
`env_jit.h` doesn't include. Verified: on iOS, where App Store rules forbid JIT,
it reports `PROHIBITED=0` and `CAN_ALLOCATE_EXEC=1` unless `env_ios.h` was
included first; with it, 1 and 0.

> **Status, 2026.10.04:** open, unchanged: nothing in the tree addresses it yet.

- *Option (a), include `env_ios.h` from `env_jit.h` on mobile Apple targets
  (`D_ENV_OS_ID == D_ENV_OS_FLAG_IOS`):*
  - *Advantages:* order-independent, with one source for the policy.
  - *Disadvantages:* JIT users on those targets also get `env_ios.h`'s
    macros, and `env_apple.h`'s.
- *Option (b), derive the prohibition from `D_ENV_OS_ID` directly:*
  - *Advantages:* no new dependency.
  - *Disadvantages:* a second copy of the policy that can drift from
    `env_ios.h`.
- **Recommendation:** (a).

**62. `D_ENV_JIT_IS_APPLE_FAMILY` calls itself an internal helper,** but
lacks the `D_INTERNAL_` prefix the guide asks of such macros.

> **Status, 2026.10.04:** open, unchanged: nothing in the tree addresses it yet.

- *Option (a), rename to `D_INTERNAL_ENV_JIT_IS_APPLE_FAMILY`:*
  - *Advantages:* conforms, and signals it isn't public API.
  - *Disadvantages:* it is `#ifndef`-guarded, so it is also an override point,
    and renaming changes that override's name.
- *Option (b), keep it public and describe it as a feature flag:*
  - *Advantages:* keeps a useful override, for example simulating an Apple
    target in tests.
  - *Disadvantages:* one more public macro.
- **Recommendation:** (b).

**63. Mac Catalyst inherits the iOS JIT prohibition.** `env_ios.h`'s
`D_ENV_MOBILE_NO_JIT` keys on `D_ENV_APPLE_IS_IOS`, which is 1 for Mac
Catalyst. Verified: Catalyst reports `PROHIBITED=1` and
`CAN_ALLOCATE_EXEC=0`. But Catalyst apps are macOS apps, which may use
MAP_JIT with the allow-jit entitlement. `D_ENV_MOBILE_NO_DLOPEN` uses the same
condition, so it likely has the same problem.

> **Status, 2026.10.04:** open, unchanged: nothing in the tree addresses it yet.

- *Advantages (exclude Mac Catalyst from both flags):* correct JIT and
  `dlopen` availability for Catalyst apps.
- *Disadvantages:* whether a given app holds the entitlement is a runtime
  fact, as for any macOS app.
- **Recommendation:** exclude Catalyst from both, after confirming the
  entitlement facts.

#### Attributes (64–67)

`env_attributes.h` (`/inc/djinterp/env/c/`) was converted and verified
identical to its original in the documented include order. That was 144
comparisons of macro state and use-site expansions, across GCC, clang, and
clang targeting MSVC, C11 to C2x and C++11 to C++2b, with overrides. One
deliberate change: it now includes `env.h` itself. Included alone before, it
silently read every language test as 0; for example, `D_DELETE` expanded to
nothing in C++17.

**64. Restore `djinterp.h`'s includes of `env_attributes.h` and
`env_vendor_attributes.h`.** An earlier pass removed them, along with
`<stdint.h>`, judging them unused when `env_attributes.h` was not yet
available and the test tree held an empty stand-in. That was a mistake. In the
original, `env_attributes.h` came first and supplied the attribute kit to
every file through the framework root, so `djinterp.h`'s own `D_NODISCARD`
was a fallback that never ran. Now that fallback is the live definition, and
the other twelve attributes no longer arrive through the root.

> **Status, 2026.10.04:** settled 2026.10.04: (B) the root carries
> `env_attributes.h`, with the one `D_NODISCARD` (`6a7b024`);
> `env_vendor_attributes.h` stays opt-in and says so (`c54ed08`).

- *Advantages (restore both, and delete `djinterp.h`'s own `D_NODISCARD`):*
  the root again provides the attribute kit, as designed, and `D_NODISCARD`
  has a single definition. The guide forbids two macros with one name and
  different definitions.
- *Disadvantages:* every file pays for the attribute headers again, as it did
  originally. `env_vendor_attributes.h` has not been seen.
- **Recommendation:** restore both, delete `djinterp.h`'s `D_NODISCARD`, and
  send `env_vendor_attributes.h` for its own pass.
- **Update (2026-09-27, `env/c` pass):** `env_vendor_attributes.h` has had its
  pass (see 85–88). Nothing in the tree includes either attribute header, and
  none of the vendor header's 30 macros is defined anywhere else outside
  `_retired/`, so restoring it adds no duplicate definitions. Nothing in the
  tree uses its macros yet; STARTTLS's `D_FORMAT_PRINTF` (#59) would be the
  first. Like `env_attributes.h`, it now includes `env.h` itself. #87 bears on
  this decision: restored, both headers reach every file.
- **Update (2026-09-28, C99 merge):** the C99 session's tree restores both
  includes and `<stdint.h>` in `djinterp.h`, but keeps `djinterp.h`'s own
  `D_NODISCARD`. The merge left those lines to this decision. Instead,
  `datomic.h` and `dmutex.c` include `env_vendor_attributes.h` for
  `D_ALIGNED` and `D_THREAD_LOCAL`, as include-what-you-use asks whichever
  way this goes.

**65. `env_attributes.h` fails `-pedantic` in most language modes**
(generalizes #58). Compiling each attribute in every mode with GCC and clang
under `-Wall -Wextra -Werror -pedantic`, 48 of 176 uses fail.

> **Status, 2026.10.04:** settled by the downporting: no C++ `[[...]]` spelling
> below the level that guarantees it; `D_CARRIES_DEPENDENCY` leaves GCC out;
> `D_ASSUME`'s C++23 form needs `__has_cpp_attribute(assume)` (`b4db405`).

- In C11/C17 and C++11–17, the `__has_*_attribute` "early support" probe
  picks `[[...]]` spellings that `-pedantic` rejects there. This hits
  `D_NORETURN`, `D_DEPRECATED(_MSG)`, `D_FALLTHROUGH`, `D_NODISCARD(_MSG)`,
  `D_MAYBE_UNUSED` and `D_LIKELY`. For `D_NORETURN` in C11/C17, the correct
  `_Noreturn` sits in the fallback tier but is never reached.
- `D_CARRIES_DEPENDENCY` warns in every C++ mode on GCC, which ignores the
  attribute.
- `D_ASSUME` in C++2b fails on clang 18, which doesn't know `[[assume]]`: the
  standard tier assumes compiler support.
- *Advantages (use the standard spelling only where the language standard
  guarantees it, and drop the probe tier; expand `D_CARRIES_DEPENDENCY` to
  nothing on GCC; in C++23 also require `__has_cpp_attribute(assume)`):*
  warning-clean under the guide's flags in every mode.
- *Disadvantages:* compilers that accept the attributes early get the vendor
  spelling in older modes instead, which has the same effect on GCC and clang.
- **Recommendation:** fix.
- **Update (2026-09-28, C99 merge):** the C half is done. `env_attributes.h`
  no longer has a `__has_c_attribute` tier, so C11 and C17 reach
  `_Noreturn`, and C gets `[[...]]` spellings only at C23. Still open: the
  C++ probe tier, `D_CARRIES_DEPENDENCY` on GCC, and `D_ASSUME` at C++23.

**66. `D_DELETE` is defined in both `env_lang.h` and `env_attributes.h`,**
with the same logic. `env_lang.h`'s definition always wins, now that
`env_attributes.h` includes `env.h`, so the second is dead code.

> **Status, 2026.10.04:** settled by the downporting: `D_DELETE` is
> `env_lang.h`'s alone (`734547f`).

- *Advantages (delete it from `env_attributes.h`):* one definition. It isn't
  an attribute, and `env_lang.h`'s "C++ helpers" is a natural home.
- *Disadvantages:* none.
- **Recommendation:** delete it from `env_attributes.h`.

**67. `D_ASSUME`'s portable fallback evaluates its argument.** It is
`((void)(expr))`, while every other tier (`[[assume]]`, MSVC's `__assume`,
clang's `__builtin_assume`) never evaluates it, so side effects run on some
compilers and not others.

> **Status, 2026.10.04:** open, unchanged: nothing in the tree addresses it yet.

- *Advantages (an unevaluated fallback, such as
  `((void)sizeof((expr) ? 1 : 0))`):* the same semantics everywhere, and the
  expression is still type-checked.
- *Disadvantages:* none of note. A plain `((void)0)` would also work, but
  drops the type check.
- **Recommendation:** the unevaluated `sizeof` form.

#### C runtime flags (68–70)

`env_c_lib.h` (`/inc/djinterp/env/c/`, an internal component of `env.h`) was
converted and verified identical to its original: 276 comparisons across ten
targets, C11 to C2x and C++11 to C++20, freestanding builds, SIMD options, and
overrides. One deliberate change: its single raw `__cplusplus >= 201103L` test
(for C++ atomics) now uses `D_ENV_LANG_IS_CPP11_OR_HIGHER`. That is identical on
GCC and clang, and corrects MSVC without `/Zc:__cplusplus`, which
reports 199711. The real file's network and memory flags match what the stand-ins used
in testing `env_net.h`, `env_jit.h` and the STARTTLS module assumed, and
`env_net.h` gives the same answers when driven by it.

**68. `env_c_lib.h`'s hosted gate never gates.** The whole file sits inside
`#ifdef __STDC_HOSTED__`, the same construct as #32. GCC and clang define
`__STDC_HOSTED__` as 0 in freestanding builds, so the test is always true.
Verified: under `-ffreestanding`, 40 of 53 flags still report library
features, such as `fork`, sockets and `mmap`.

> **Status, 2026.10.04:** open, unchanged: nothing in the tree addresses it yet.

- *Option (a), gate only the library-dependent flags on
  `#if __STDC_HOSTED__`, defining them 0 otherwise:* leave ungated what a
  freestanding implementation still provides: the freestanding headers
  (`<stdint.h>`, `<stdbool.h>`, `<stdalign.h>`), SIMD intrinsics (a compiler
  feature), and VLAs (a language feature).
  - *Advantages:* honest answers in freestanding builds.
  - *Disadvantages:* each of the 53 flags must be sorted into one group or the
    other.
- *Option (b), `#if __STDC_HOSTED__` around the whole block:*
  - *Advantages:* minimal.
  - *Disadvantages:* wrongly zeroes the freestanding headers and intrinsics, and
    leaves every flag undefined unless an `#else` defines them.
- **Recommendation:** (a).

**69. The C-runtime flags depend on the C standard, so they are wrong in
C++.** On the same Linux target, 13 flags are 1 in C11 and 0 in C++17. Some
zeros are right: VLAs, `<complex.h>`, `<tgmath.h>` and C11 threads are
C-only. Others are wrong:

> **Status, 2026.10.04:** open, unchanged: nothing in the tree addresses it yet.

- `STDINT_H`, `INTTYPES_H`, `SNPRINTF`, `FENV_H` and `UCHAR_H`: all in C++
  since C++11.
- `ALIGNED_ALLOC` and `TIMESPEC_GET`: in C++ since C++17.
- `STDBOOL_H` and `STDALIGN_H`: compatibility headers C++ still accepts.

The banner says these flags describe the runtime for both languages. The root
cause is the one in #7: the C-standard macros read as C90 in C++.
- *Advantages (in C++, gate each flag on the C++ standard that adopted that C
  library feature: C++11 for C99's, C++17 for C11's; keep the C-only language
  facilities at 0):* correct flags for C++ files, matching the banner.
- *Disadvantages:* each flag needs its own C++ condition, and the deprecated
  compatibility headers need a call.
- **Recommendation:** fix, together with #7.

**70. `D_ENV_C_HAS_SSE` reads 0 on MSVC x64 while `D_ENV_C_HAS_SSE2` reads
1.** MSVC is believed to define neither `__SSE__` nor `__SSE2__`, and defines
`_M_IX86_FP` only for 32-bit x86. The SSE2 test has an explicit x64 clause;
the SSE test doesn't. Simulated with MSVC's macro set: SSE=0, SSE2=1, though
SSE2 implies SSE.

> **Status, 2026.10.04:** open, unchanged: nothing in the tree addresses it yet.

- *Advantages (add the x64 clause to the SSE test, since x64 guarantees both):*
  consistent and correct.
- *Disadvantages:* none.
- **Recommendation:** fix, after confirming MSVC's macros.

---

#### Include guards and collisions (71–72)

**71. Include guards with a repeated segment.**
507 of 1,790 guards (28%) repeat a segment, `_TEST_TEST_` most often. Both
guides say "Do not merge repeated segments ... A guard that looks redundant is
still correct." Collapsing every repeat produces 10 collisions. Collapsing only
where a file's stem begins with its directory's name, never where it equals it,
still produces 7. No rule that looks at one path alone avoids collisions.

> **Status, 2026.10.04:** settled earlier: the guides keep the rule and say why
> ("Why there are no exceptions"), before the downporting's rounds.

- *Advantages (collapse, keeping the full form only where it would collide):*
  shorter guards across 28% of headers.
- *Disadvantages:* a guard stops being a function of its own path. Adding a
  file can rename another file's guard, `derive_guards.py` needs a whole-tree
  pass to verify any single guard, and collisions go from impossible to
  prevented by checking.
- *Alternative (keep the rule):* no churn; guards stay checkable one file at a
  time. The repetition stays.
- **Recommendation:** keep the rule. The repetition is cosmetic, and
  path-independence is what makes a guard checkable.

**72. Retiring the five mergeable collisions.**
Five of #71's ten collisions can go with no consumer affected, proven on a
scratch copy. `net/curl.hpp` is a true duplicate of `net/curl/curl.hpp` and
nothing uses it. The four flat `parse/expression_*.hpp` are a closed island:
the only include among them is the flat `render` pulling in the flat `ops`,
and nothing outside uses any of the four. The nested `parse/expression/` set is
the live one, used by `layout.hpp`. The flat set is built on
`math/expression.hpp`, the nested on `parse/expression/expression.hpp`, and
those two share 5.6% of their tokens.

> **Status, 2026.10.04:** settled by the downporting: the four flat
> `parse/expression_*.hpp` copies were retired (`a094959`); `net/curl.hpp` is
> net's and no longer in the tree.

- *Advantages (retire all five):* removes dead code and half of #71's
  collisions, whichever way #71 goes. On the scratch copy: no new unresolved
  includes, and every nested file compiles exactly as before.
- *Disadvantages:* settles the expression fork for `parse/expression/`.
- **Recommendation:** retire them to `_retired/`. `math/expression.hpp` is not
  orphaned by this: eight `math/` headers include it as a sibling.

#### Layout and structure (73–77)

**73. The two pool stacks.**
`core/memory/pool.hpp` and `core/memory/pool/pool.hpp`, and the matching
`pool_allocator.hpp` pair, are two independent implementations, 18–33%
token-similar, each with live consumers. The flat pair has 4 and 2, among them
`memory.hpp` and `memory_strategy.hpp`; the nested pair has 7 and 1, among them
three `file_tree_*` headers and the pool tests. Ruling 5 records pool and
pool_allocator as already migrated nested-over-flat.

> **Status, 2026.10.04:** settled by the downporting: the other way from this
> entry's pick: the owner's ruling of 2026.09.30 made the C-backed memory
> generation canonical, and the nested pool stack was retired (`f5fa25f`).

- *Advantages (converge on nested):* one implementation, and ruling 5 becomes
  true.
- *Disadvantages:* the flat stack's consumers move to an API that differs in
  most of its tokens: porting, not renaming.
- *Alternative (keep both):* no porting; ruling 5's record stays wrong, and two
  of #71's collisions stay.
- **Recommendation:** converge on nested, after diffing the two APIs to make
  sure nested covers what `memory.hpp` uses; correct ruling 5 until then.

**74. Where the network modules' C headers live.**
Five of six modules put C headers directly under `net/` (`net/ssl/ssl.h`, and
likewise `pop`, `imap`, `ssh`, `ftp`). SMTP alone uses `c/net/smtp/`, with its
C++ wrapper at `net/smtp.hpp`. `net/` also holds C++ headers: `web.hpp`,
`tls.hpp`, `tcp.hpp`, `curl.hpp`, `reactor.hpp`, `smtp.hpp`.

> **Status, 2026.10.04:** other sessions': net's, worked on in other sessions;
> not audited here.

- *Advantages (`net/`):* matches five modules as delivered; no moves.
- *Disadvantages:* one directory mixing both languages, unlike the rest of the
  tree, which keeps C under `c/`.
- *Alternative (`c/net/`):* C under `c/net/`, C++ under `net/`, as SMTP does;
  five modules' files, guards and includes to move.
- **Recommendation:** `c/net/`. Five moves now costs less than a second
  convention.

**75. How network modules register their configuration.**
Three patterns. SSL and POP add their `cfg_*.h` to `dconfig.h`. SSH's
`cfg_ssh.h` is included directly by six SSH files. IMAP, SMTP and FTP have no
configuration header.

> **Status, 2026.10.04:** other sessions': net's, worked on in other sessions;
> not audited here.

- *Advantages (everything through `dconfig.h`):* one list of every module's
  knobs.
- *Disadvantages:* every translation unit that includes `dconfig.h` pays for
  every module's configuration.
- *Alternative (direct, per module):* only a module's users pay for it.
- **Recommendation:** direct, as SSH does, which is the load-on-demand
  reasoning of #2 and #3; move SSL and POP off `dconfig.h`.

**76. The subsystem table: `tools`, `ui`, `render`.**
Both guides' tables still list `3d` and have no rows for `tools` or `ui`. The
tree has dawk under `tools/` (its user ruling R-28), a 17-header `ui/`, and
`3d/` renamed to `render/` (ruling 22). Guard derivation already yields
`DJINTERP_TOOLS_*`, `DJINTERP_UI_*` and `DJINTERP_RENDER_*`, because an
unlisted top-level directory falls through to its own name; the banner tags
and macro prefixes are unspecified.

> **Status, 2026.10.04:** open, unchanged: the subsystem table still has /3d and
> no tools, ui or render row.

- *Advantages (add the rows):* the guide describes the tree.
- *Disadvantages:* none beyond the editing.
- **Recommendation:** add `tools` and `ui`, rename `3d` to `render`: prefixes
  `TOOLS_`, `UI_`, `RENDER_`; tags `[tools]`, `[ui]`, `[render]` — what the
  tree already carries.

**77. `render/render3d.hpp`'s name.**
Ruling 22 renamed the directory and left the file's name, so the umbrella reads
`render/render3d.hpp`.

> **Status, 2026.10.04:** open, unchanged: `render/render3d.hpp` keeps its name.

- *Advantages (rename to `render/render.hpp`):* reads naturally, and matches
  `net/net.hpp` and `ui/ui.hpp`.
- *Disadvantages:* one more rename, and its includers to update.
- **Recommendation:** rename.

#### Coordination between sessions (78–82)

**78. Who owns `djinterp.hpp`.**
Five bundles delivered `djinterp.hpp` in relay 93, each written against its own
view of the file, and four needed repair: one renamed a macro to a name nothing
defined, and three dropped declarations another session had added —
`namespace functional = ::djinterp;`, which five vparse headers need, and
`NS_UI` / `D_KEYWORD_USER_INTERFACE`, which the 17 `ui/` headers need. Each
drop was caught only by recompiling those subsystems.

> **Status, 2026.10.04:** settled by the downporting: in practice, by the
> rounds' lane rule: one lane owns the root and the others send requests. The
> growth check was never built.

- *Advantages (one owning session; the others send additions, not whole
  files):* the file stops churning.
- *Disadvantages:* every session that needs a keyword or namespace waits on
  one.
- *Alternative (a check in `verify/` that `djinterp.hpp`'s declarations only
  ever grow):* sessions stay independent, and a drop is caught at merge rather
  than at compile.
- **Recommendation:** the growth check. It enforces the property that matters
  without routing every change through one session.

**79. The two `d_test_counter` designs.**
Relay 92 defined `struct d_test_counter` from its call sites: 48 files use its
four fields directly, 690 times. Four files, chiefly `test_printer.c` and
`test_standalone.c`, call an API that exists nowhere — `d_test_counter_init`,
`_increment`, `_get`, `_reset`, `_free`, `_add`, with
`D_TEST_COUNTER_ASSERT_STD`, `D_TEST_COUNTER_TEST_STD` and
`D_TEST_COUNT_TOTAL` / `_PASSED` / `_FAILED` — 43 uses; two files use both.
`config/test/cfg_test_counter.h` exists, so the API was planned.

> **Status, 2026.10.04:** other sessions': DTest's, worked on in other sessions;
> not audited here.

- *Advantages (write the API over the struct):* its constants read as
  selectors over the same four fields, so both forms can stand, and the four
  files compile unchanged.
- *Disadvantages:* two ways to do one thing.
- *Alternative (rewrite the 43 uses to the field form):* one way; four files
  to edit.
- **Recommendation:** the field form. 690 uses to 43 have already chosen it.

**80. Two `/parse/` foundations.**
The parse and vparse sessions each built a `/parse/` substrate. No file,
identifier or macro collides; the concepts do — among them three unconnected
diagnostic channels. The parse handoff's `ISSUES.md` A1 sets out the options
and recommends a joint architecture.

> **Status, 2026.10.04:** open, unchanged: parse and vparse both still exist.

- **Recommendation:** decide it as ISSUES A1, where it is argued in full. It
  appears here so this register is complete.

**81. The quarantined flat C test sources.**
`_retired/relay91_flat_test_c/` holds 17 sources quarantined in relay 91 for
sitting at `test/c/`. Ruling 21 has since moved the C test framework there, so
that reason is gone. Nine differed from their nested counterparts in code at
the time; the live copies have since absorbed two rounds of include repair, so
all 17 differ now.

> **Status, 2026.10.04:** other sessions': DTest's, worked on in other sessions;
> not audited here.

- *Advantages (harvest the nine):* recovers any work the nested versions never
  got.
- *Disadvantages:* a per-file diff against the repaired copies; the
  `test_object.c` there predates ruling 13 and must not be taken.
- *Alternative (discard):* nothing to do, at the risk of lost work.
- **Recommendation:** harvest the nine, file by file.

**82. The deferred relay-91 headers.**
`_retired/relay91_deferred_headers/` holds 11 headers reverted in relay 91:
they deleted the framework-name macro, reordered `env/env.h`'s includes, and
put a Cyrillic letter in a guard. They also carried real improvements, among
them a table of contents in `env_arch.h`, include summaries in `registry.h`
and a Roman-to-decimal ToC in `c/djinterp.h`.

> **Status, 2026.10.04:** open, unchanged: `_retired/relay91_deferred_headers/`
> is not in this tree; nothing to act on here.

- *Advantages (harvest):* recovers those improvements.
- *Disadvantages:* this session has since rewritten several of the same files,
  so much of it may already be done.
- **Recommendation:** compare each with its current version first, and take
  only what isn't already there.

#### C++11 (83)

**83. The C++11 path for post-C++11 library features.**
The C++ guide now targets C++11, with later standards only behind an env-layer
check and never as the only implementation. About 40 `core/` headers use
`std::void_t`, `std::enable_if_t`, `_t` / `_v` aliases,
`std::optional` / `variant` / `string_view`, or `if constexpr`. At C++11,
`ui/` builds 1 of 17 headers, `render/` 0 of 5 and vparse 3 of 8, all failing
in that shared code. `re_std` already backports `void_t` and `enable_if_t`.

> **Status, 2026.10.04:** settled by the downporting: superseded: the C++ floor
> is C++98 with levels (programme 3.6, 4.9), and re_std supplies the library
> types.

- *Option (`re_std` always, for library types):* one spelling at every
  standard, and no env-layer branch at each use.
- *Option (`std` behind an env-layer check, `re_std` below it):* the
  platform's own types where available; two code paths per use, both needing
  tests.
- **Recommendation:** `re_std` always for library types. Language features
  such as `if constexpr` can't be backported, so those alone take the
  env-layer branch.

#### Guide gaps (84)

**84. Where a file-local `static` function's contract goes.**
The guides say where a function's contract goes when a header declares it, but
a file-local `static` function has no header declaration, and neither guide
covers it. The parse session's `style-guides.patch` proposes: above its first
declaration in the source; and when that first declaration is the definition,
one comment block carrying both the contract and the mechanism.

> **Status, 2026.10.04:** open, unchanged: neither guide says where a file-local
> static function's contract goes.

- *Advantages:* closes a gap every source file with helpers runs into.
- *Disadvantages:* none beyond the editing.
- **Recommendation:** adopt it.

#### `env/c`: vendor attributes and `long long` (85–88)

`env_long_long.h` and `env_vendor_attributes.h` (`/inc/djinterp/env/c/`) were
converted and verified identical to their originals in the documented include
order. That was 2,904 comparisons of macro state and use-site expansions:
- **Compilers and targets:** GCC 13 and clang 18, targeting Linux on x86-64,
  x86, AArch64 and RISC-V; MSVC on x64, x86 and ARM64; MinGW; macOS; and iOS.
- **Languages:** GNU89 and C99 to C23, C++98 to C++23.
- **Configurations:** simulated compilers and language standards, and
  overrides.

A strict sweep compiled each macro in every mode, including every usage
example in the header's comments: 600 builds per version, with no pass/fail
difference between them.
- **One deliberate change:** `env_vendor_attributes.h` now includes `env.h`
  itself, as `env_attributes.h` does. Included alone before, on a GCC build
  for Linux, 25 of its 30 macros expanded to nothing and the other five fell
  back to their portable forms. `D_THREAD_LOCAL` was among the 25, so a
  would-be thread-local variable silently became an ordinary global.
- **Corrected banners:** `env_long_long.h`'s said `env.h` includes it;
  `env_lang.h` does, at its end. The vendor header's said it reads
  `D_ENV_OS_*`; it reads raw `_WIN32`.
- **Renamed:** the file-local helper `D_INTERNAL_GCC_COMPAT_` is now
  `D_INTERNAL_ENV_GCC_COMPAT`, #10's form. It is still `#undef`'d at the end of
  the header, so nothing outside it can see either name, whichever way #10
  goes.
- **Strict flags:** every macro compiles in C99 to C23 and C++11 to C++23 on
  both compilers, except `D_UNREACHABLE` (#85). C++98 fails at the header's
  variadic macros, below the C++11 target.

**85. `D_UNREACHABLE`'s C++23 tier needs `<utility>`, which nothing includes.**
The tier expands to `std::unreachable()`, and the header leaves including
`<utility>` to each user. A C++23 translation unit that uses the macro without
it fails on clang 18 ("use of undeclared identifier 'std'"), and compiles once
`<utility>` is included. GCC 13 escapes only because it reports `__cplusplus`
as 202100L, below `env_lang.h`'s 202302L threshold.

> **Status, 2026.10.04:** open, unchanged: documented: `D_UNREACHABLE` still
> does not include `<utility>`.

- *Advantages (include `<utility>` from the header, in the C++23 branch
  only):* the macro works wherever it is used, and keeps libstdc++'s
  debug-build check: its `std::unreachable()` traps under
  `_GLIBCXX_ASSERTIONS` and asserts under `_GLIBCXX_DEBUG`.
- *Disadvantages:* every C++23 file that includes the header pulls in
  `<utility>`, whether or not it uses the macro.
- *Alternative (put `__builtin_unreachable()` and `__assume(0)` ahead of the
  standard tier):* no include, but libstdc++'s check is lost.
- **Recommendation:** include `<utility>` in the C++23 branch.

**86. `D_WEAK` is `__declspec(selectany)` on MSVC, which rejects it on
functions.**
MSVC accepts selectany only on data with external linkage. clang's MSVC mode
enforces the same rule: with the header's MSVC branch simulated there, `D_WEAK`
on a function fails ("'selectany' can only be applied to data items with
external linkage"). Even on data, selectany means "any one of several identical
definitions", not "a strong definition wins". The same test fails
`__declspec(naked)` on x64. That limit is documented and unavoidable, since
MSVC has no x64 inline assembly to write a naked body in.

> **Status, 2026.10.04:** open, unchanged: nothing in the tree addresses it yet.

- *Advantages (expand `D_WEAK` to nothing on MSVC, as for an unknown
  compiler):* uses compile everywhere.
- *Disadvantages:* MSVC loses selectany's data-only approximation, and a
  replaced default then shows up, if at all, as a duplicate symbol at link
  time.
- *Alternative (leave `D_WEAK` undefined on MSVC):* each use fails at compile
  time, naming the macro. This breaks the header's rule that every macro is
  always defined.
- **Recommendation:** expand to nothing on MSVC.

**87. Attribute spellings follow a simulated compiler.**
`env_vendor_attributes.h` and `env_attributes.h` choose spellings from
`D_ENV_COMPILER_*`, which `cfg_env.h` lets a build simulate. On GCC, with
compiler detection off (`D_CFG_ENV_CUSTOM=0x04`) and
`D_ENV_DETECTED_COMPILER_MSVC` defined, `D_ALIGNED(16)` expands to
`__declspec(align(16))`, which GCC rejects. With #64 taken, both headers reach
every file, so a simulated-compiler build fails at the first attribute with an
MSVC spelling.

> **Status, 2026.10.04:** open, unchanged: nothing in the tree addresses it yet.

- *Advantages (key the spellings on the real compiler's predefines,
  `__GNUC__`, `__clang__` and `_MSC_VER`):* the spellings always match the
  toolchain doing the compiling, and simulation keeps working for everything
  else.
- *Disadvantages:* the attribute headers stop following simulation, which they
  can't compile under anyway, and the env layer gains a second way of naming
  compilers.
- **Recommendation:** real predefines, in both attribute headers.

**88. `env_build.h` tests a switch that doesn't exist (bug).**
It reads `#if D_CFG_ENV_BUILD_IS_ENABLED`, but `cfg_env.h` defines
`D_CFG_ENV_BUILD_ENABLED`, as it did before relay 91's config rename. The
unknown name reads as 0, so automatic Debug/Release detection never runs. With
no `D_ENV_DETECTED_BUILD_*` override, none of `D_ENV_BUILD_DEBUG`,
`D_ENV_BUILD_RELEASE` or `D_ENV_BUILD_TYPE` is defined, with or without
`-DNDEBUG`. The effects:

> **Status, 2026.10.04:** settled by the downporting: `env_build.h` reads the
> switch `cfg_env.h` defines (`07dae62`).

- `env_compress_link.h`'s debug-library block and `env_printer.hpp`'s build
  report are silently skipped.
- `env.c`'s `D_DEBUG` printer fails to compile on the missing
  `D_ENV_BUILD_TYPE`, among other errors in that unconverted block.
- `env_build.h`'s banner names the wrong switch too, and `env_tests_sa_cfg.c`
  uses the same wrong name six times.

`-Wundef` (#20) reports it at once.
- *Advantages (use `D_CFG_ENV_BUILD_ENABLED` in `env_build.h`, its banner and
  the test):* build detection works, and the name matches the other
  `D_CFG_ENV_*_ENABLED` switches.
- *Disadvantages:* code that checks these macros sees them for the first time;
  nothing is in production.
- **Recommendation:** fix.

#### `env/cpp`: feature detection (89–91)

`env_cpp_features.h` (`/inc/djinterp/env/cpp/`) was converted and verified
identical to its original. Every feature macro's definition matched exactly,
and the 17 aggregates matched term for term and in value, in 41
configurations: GCC 13 and clang 18 in C++98 to C++26, with the header alone
or after `<bits/stdc++.h>` or `<version>`, and as C.
- **Layout:** each of the 160 features is now one item. The flag and `_VAL`
  share one `#ifdef`, whose `#endif` names the feature-test macro, and
  `_NAME`, `_DESC` and `_VERS` follow. The file went from 4,995 lines to
  2,978.
- **Corrected:** the banner promised four macros per feature and listed five.
- **Left long:** one `_DESC` string literal runs past column 80, as the guide
  allows for string literals.

**89. The library flags depend on what was included first.**
The `__cpp_lib_*` macros come from `<version>` and from each library header,
and `env_cpp_features.h` includes neither. With GCC 13 in C++23, the header
alone sets 0 of its 84 library flags; after `<bits/stdc++.h>` it sets
55.

> **Status, 2026.10.04:** open, unchanged: nothing in the tree addresses it yet.

- *Advantages (include `<version>` when `__has_include(<version>)` finds it):*
  the flags describe the library whatever the translation unit included
  first.
- *Disadvantages:* every C++ file that includes the header pulls in
  `<version>`, which #85 weighs for `<utility>` too.
- **Recommendation:** include `<version>` when it exists.

**90. The feature flags can't be overridden.**
The env conventions (1.1) `#ifndef`-guard every detection macro so a build can
override it. `env_cpp_features.h` guards none of its 160 flags, so
pre-defining one draws a redefinition warning, an error under `-Werror`.

> **Status, 2026.10.04:** open, unchanged: nothing in the tree addresses it yet.

- *Advantages (guard each flag):* consistent with the rest of env, and a
  build or test can switch a feature off.
- *Disadvantages:* 160 more guards in a file that is mostly data.
- *Alternative (also guard each `_VAL`):* every macro becomes overridable, at
  twice the guards.
- **Recommendation:** guard the flags only.
- **Also in `env/db` (2026-09-27):** the database headers share the gap.
  `env_arangodb.h` guards 4 of its 98 `D_ENV_ARANGO_HAS_*` flags, and
  its siblings look alike. The same recommendation applies.

`env_cpp98.h` was converted and verified identical to its original in 245
configurations:
- **Compilers and targets:** GCC 13 and clang 18 in C++98 to C++23 and as C,
  plus clang targeting MSVC (with and without exceptions), Android, AVR and
  macOS.
- **Switches:** RTTI off, exceptions off, `-ffreestanding`, `__AVR__`, and
  overrides.

All 38 macros took both values across those runs.
- **Notes folded in:** its notes section now lives in the banner, the section
  description and the four special-cased items.
- **Removed:** a commented-out `env.h` check that named a guard which no longer
  exists.
- **Corrected banner:** it no longer asks for `env.h` first, since the header
  reads only compiler predefines. It also names `env_cpp_features.h` correctly.

**91. `env_cpp98.h` assumes a hosted library.**
Apart from `<locale>` and the RTTI and exception cases, its flags report every
header present in any C++ build, and it never reads `__STDC_HOSTED__`. With
GCC 13 and `-ffreestanding`, `__STDC_HOSTED__` is 0 while
`D_ENV_CPP98_HAS_VECTOR` and `D_ENV_CPP98_HAS_IOSTREAM` stay 1, though a
freestanding implementation needn't provide either. `__has_include(<vector>)`
is 1 there too, since the hosted headers are still on disk.

> **Status, 2026.10.04:** open, unchanged: nothing in the tree addresses it yet.

- *Advantages (gate the hosted-only headers on `__STDC_HOSTED__`, keeping
  `<new>`, `<typeinfo>`, `<exception>` and `<limits>`, which C++98 requires
  even of freestanding implementations):* the flags match what the standard
  guarantees, which is the header's stated purpose for embedded builds.
- *Disadvantages:* a firmware build that uses `-ffreestanding` with a hosted
  toolchain loses flags for headers that do exist.
- **Recommendation:** gate on `__STDC_HOSTED__`.

#### `env/db`: database detection (92–97)

`env_arangodb.h` (`/inc/djinterp/env/db/arangodb/`) was converted and verified
identical to its original in 55 configurations. They covered:
- **Compilers and languages:** GCC 13 and clang 18, as C11, C17, and C++11 to
  C++20.
- **Detection:** not detected; detected manually by version, by series and
  with Enterprise; detected automatically through a stub velocypack header; and
  detected from the version string alone.
- **Overrides:** pre-defined flags.

The four C builds with the header setting on stop at its `#error` in both
versions. 94 of its 147 object-like macros took both values across the runs.
- **Layout:** the Roman sections became 20 decimal ones, each item numbered.
  The old section I, only a pointer to the config file, moved into the banner.
  The version-comparison macros and the release-series group gained briefs,
  and boolean `#if`s and composite definitions are in the house form.
- **Corrected:** the vendor section named `cfg_env.h` for a setting that
  lives in `cfg_env_arangodb.h`. Section XX documented a
  `D_ENV_ARANGO_DEPRECATED_VST` that was never defined; the macro is
  `D_ENV_ARANGO_VST_DEPRECATED`.

`env_cassandra.h` and `env_cassandra_config.h`
(`/inc/djinterp/env/db/cassandra/`) were converted and verified identical to
their originals in 104 configurations. They covered both headers under GCC 13
and clang 18, as C11, C17, C++17 and C++20, across 13 scenarios:
- **Driver:** detected through a stub `cassandra.h`, with and without a
  version suffix, or supplied as a manual version.
- **Server:** each way of configuring the target server version.
- **Editions:** DSE and Astra.
- **Overrides:** including a pre-defined `D_ENV_CASSANDRA_DETECTED`, which
  skips the consumer layer.

108 of their 133 object-like macros took both values across the runs.
- **Layout:** 18 decimal sections. The driver and server version machinery and
  the release-series group gained briefs, and the consumer compatibility layer
  is one item. The config's section III note became a documented item for
  `D_CFG_ENV_CASS_SERVER_VERSION`, which deliberately has no default.
- **Corrected:** section 0 named `cfg_env.h` for a setting in
  `env_cassandra_config.h`. The consumer layer's description named
  `cassandra_traits.hpp`, which doesn't exist; `cassandra.hpp` is the only
  consumer.
- **Not moved:** `env_cassandra_config.h` stays in `env/db/cassandra/`
  pending #93.

`env_db2.h` and `env_db2_config.h` (`/inc/djinterp/env/db/db2/`) were
converted and verified identical to their originals in 104 configurations.
They covered both headers under GCC 13 and clang 18, as C11, C17, C++17 and
C++20, across 13 scenarios:
- **Client:** detected through a stub `sqlcli1.h`, with and without
  `SQLCLI_VER`, or supplied manually with or without a version.
- **Server:** each way of configuring the target server version.
- **Platform:** z/OS and Db2 for i.
- **Overrides:** pre-defined flags.

74 of their 100 object-like macros took both values across the runs.
- **Layout:** 18 decimal sections, laid out like `env_cassandra.h`. The client
  and server version machinery and the release-series group gained briefs,
  and the consumer layer is two items, the public version and the feature
  aliases. In the config, the server-version note became a documented item,
  and the platform-family values and `D_CFG_ENV_DB2_PLATFORM` gained briefs.
- **Corrected:** section 0 named `cfg_env.h` for a setting in
  `env_db2_config.h`. The consumer layer's description named `db2_traits.hpp`,
  which doesn't exist; `db2.hpp` and `db2_table.hpp` are the consumers.
- **Not moved:** `env_db2_config.h` stays in `env/db/db2/` pending #93.

`env_dynamodb.h` and `env_dynamodb_config.h` (`/inc/djinterp/env/db/dynamodb/`)
were converted and verified identical to their originals in 88
configurations. They covered both headers under GCC 13 and clang 18, as C11,
C17, C++17 and C++20, across 11 scenarios:
- **SDK:** detected through stub headers, with and without its version header
  and the DAX client, or supplied manually.
- **Target:** DynamoDB Local, set both ways.
- **Opt-outs and overrides:** capability opt-outs, and overrides including the
  consumer-layer skip.

In the six C builds with the SDK setting on, both versions stop at the same
`#error`. 60 of their 104 object-like macros took both values across the runs.
- **Layout:** 17 decimal sections, laid out like `env_cassandra.h`. The vendor
  section is two items, the SDK header and the optional DAX client. The
  capability-profile opt-outs are one item, and the consumer layer is one
  item. In the config, the deployment-target values and
  `D_CFG_ENV_DYNAMODB_TARGET` gained briefs.
- **Corrected, where the documentation disagreed with the code:**
  - Section 0 named `cfg_env.h` for a setting in `env_dynamodb_config.h`.
  - The consumer layer named `dynamodb_traits.hpp`, which doesn't exist;
    `dynamodb.hpp` and `dynamodb_table.hpp` are the consumers.
  - `D_ENV_DDB_DETECTED`'s comment said a configured target or a capability
    profile counts as detection; only the SDK or `D_CFG_ENV_USING_DYNAMODB`
    does.
  - The capability section promised a Local opt-in that doesn't exist (#95).
- **Not moved:** `env_dynamodb_config.h` stays in `env/db/dynamodb/` pending
  #93.

`env_mariadb.h` (`/inc/djinterp/env/db/mariadb/`) was converted and verified
identical to its original in 88 configurations. They covered GCC 13 and
clang 18, as C11, C17, C++17 and C++20, with the header alone and after
`env.h`, across 11 scenarios:
- **Automatic detection:** through stub client headers for MariaDB 10.6.11,
  for 11.4.2 with only its base-version string, and for Oracle MySQL, which
  isn't MariaDB.
- **Manual detection:** by version and by series.
- **Overrides:** pre-defined flags.

Automatic detection without `env.h` first fails alike in both versions (#96).
119 of its 169 object-like macros took both values across the runs.
- **Layout:** 17 decimal sections. There is no vendor section, since
  `env_mysql_common.h` includes the client headers for the MySQL family. The
  release IDs, the detection chain, the comparisons, the release series and
  the LTS flag gained items and briefs. The three alias groups shared with
  MySQL became items.
- **Not touched:** `cfg_env_mariadb.h`, which lives in `config/` (#93).

`env_mysql.h` and `env_mysql_common.h` (`/inc/djinterp/env/db/mysql/`) were
converted and verified. The runs covered GCC 13 and clang 18, as C11, C17,
C++17 and C++20, plus clang targeting MSVC. Ten scenarios were run through
`env_mysql.h`, `env_mariadb.h` and the common header:
- **Automatic detection:** through stub MySQL 8.0.36 and 9.1 headers, a
  MariaDB header under either setting, and the X DevAPI header.
- **Manual detection:** by version and by series.
- **Overrides:** pre-defined flags.

The results:
- **After `env.h`:** all 150 configurations are identical.
- **Standalone:** the new headers behave exactly as the old ones did after
  `env.h`, in all 150. The old common header failed standalone in 75 of them,
  every run with a client header in scope. That one deliberate change is the
  fix for #96.

225 of the 402 object-like macros took both values across the runs.
- **Layout:** `env_mysql_common.h` has 10 sections. The four vendor-inclusion
  blocks are items, and section IV's lettered subsections A to H became
  5.1 to 5.8. The `#else` that zeroes every flag without a client library
  stays. `env_mysql.h` has 18 sections, like `env_mariadb.h`.
- **Documented for the first time:** sections V to XIX of `env_mysql.h`, about
  120 flags, had no comments at all. Each section now has a group item naming
  its capabilities, and the flags whose logic goes beyond a version gate have
  their own briefs.
- **Corrected:** section 0 named `cfg_env.h` for settings in
  `cfg_env_mariadb.h` and `cfg_env_mysql.h`.

`env_mongodb.h` (`/inc/djinterp/env/db/mongodb/`) was converted and verified
identical to its original in 88 configurations. They covered GCC 13 and clang
18, as C11, C17, C++17 and C++20, with the header alone and after `env.h`,
across 11 scenarios:
- **Driver:** detected through stub headers, with libbson and the version
  string and without, or supplied manually.
- **Server:** each way of configuring the target server version.
- **Editions:** Enterprise and Atlas.
- **Overrides:** pre-defined flags.

146 of its 176 object-like macros took both values across the runs.
- **Layout:** 19 decimal sections, laid out like `env_cassandra.h`. The vendor
  section is two items, libmongoc and mongocxx. The driver, libbson and server
  version machinery, the comparisons and the release-series group gained
  items and briefs.
- **Corrected:** section 0 named `cfg_env.h` for a setting in
  `cfg_env_mongodb.h`. The banner said server-gated features "default to the
  minimum version the driver supports" when no server version is configured;
  the server version is then 0, so they read 0.
- **Not touched:** `cfg_env_mongodb.h`, which lives in `config/` (#93).

`env_oracle.h` (`/inc/djinterp/env/db/oracle/`) was converted and verified
identical to its original in 112 configurations. They covered GCC 13 and
clang 18, as C11, C17, C++17 and C++20, with the header alone and after
`env.h`, across 14 scenarios:
- **Automatic detection:** through stub OCI headers for 19.3 with OCCI, 12.2
  with legacy numbering, and 21 with a major version only; also from
  `ORACLE_VERSION` alone.
- **Manual detection:** versions, including a legacy one, and release flags.
- **Editions and options:** Express, Standard Edition 2, and licensed
  options.
- **Overrides:** pre-defined flags.

108 of its 178 object-like macros took both values across the runs.
- **Layout:** 21 decimal sections. The vendor section is two items, OCI and
  OCCI. The release IDs, the `D_ENV_ORA_AT_LEAST` shorthand, the detection
  chain, the comparisons, the release series and the LTS flag gained items
  and briefs. The edition flags, which had none, are one documented item, and
  the licensed options their own subsection.
- **Corrected:** section 0 named `cfg_env.h` for a setting in
  `cfg_env_oracle.h`. The edition section called its defaults "the most
  permissive assumption (Enterprise, all options disabled)"; it now says
  plainly that the edition defaults to Enterprise and each option to 0.
- **Kept as found:** the mangled implicit-results flag (#97).
- **Not touched:** `cfg_env_oracle.h`, which lives in `config/` (#93).

`env_postgres.h` (`/inc/djinterp/env/db/postgres/`) was converted and verified
identical to its original in 80 configurations. They covered GCC 13 and
clang 18, as C11, C17, C++17 and C++20, with the header alone and after
`env.h`, across 10 scenarios:
- **Automatic detection:** through stub libpq headers for 16.2 with libpqxx,
  and for 9.6.24 with legacy numbering.
- **Manual detection:** versions and release flags.
- **Extensions:** PostGIS and pgvector.
- **Overrides:** pre-defined flags.

83 of its 183 object-like macros took both values across the runs.
- **Layout:** 20 decimal sections. The vendor section is two items, libpq and
  libpqxx. The release IDs, the detection chain and the release-series group
  gained items and briefs.
- **Corrected:**
  - Section 0 named `cfg_env.h` for a setting in `cfg_env_postgres.h`.
  - The first comparison item was headed `D_ENV_PG_VERSION_AT_LEAST`, and
    described two- and three-argument forms. The macro is
    `D_ENV_PG_VERSION_AT_LEAST_NUM`, and it takes one raw `PG_VERSION_NUM`
    value.
  - `D_ENV_PG_HAS_SSL`'s brief claimed it detects whether libpq was built with
    SSL, and a comment claimed a version check. It is 1 unless pre-defined:
    `USE_SSL` or `USE_OPENSSL` confirm SSL support, and otherwise it is
    assumed.
- **Not touched:** `cfg_env_postgres.h`, which lives in `config/` (#93).
- **Earlier headers amended:** the same pass found a gap in the earlier
  conversions. Single-line multi-condition `#if`s had been left in their old
  form: one in `env_mongodb.h`, two in `env_mysql_common.h` and two in
  `env_oracle.h`. They are now in the house form, and all three headers were
  re-verified.

`env_redis.h` and `env_redis_config.h` (`/inc/djinterp/env/db/redis/`) were
converted and verified identical to their originals in 208 configurations.
They covered both headers under GCC 13 and clang 18, as C11, C17, C++17 and
C++20, standalone and after `env.h`, across 13 scenarios:
- **Client:** detected through stub hiredis headers (1.2.0 with its SSL
  header and SONAME, and 0.14.1 flat without SSL), or supplied manually.
- **Server:** each way of configuring the target server version.
- **Distributions:** Valkey, and Enterprise, Cloud and Stack together.
- **Overrides:** pre-defined flags.

100 of their 136 object-like macros took both values across the runs.
- **Layout:** 18 decimal sections, laid out like `env_cassandra.h`. The vendor
  section is two items, the hiredis header and its optional SSL header. The
  client and server version machinery, the comparisons and the
  release-series group gained items and briefs, and the consumer layer is two
  items, the public version and the feature aliases. The config's
  server-version note became a documented item.
- **Corrected:**
  - Section 0 named `cfg_env.h` for a setting in `env_redis_config.h`.
  - The foundational data-type item named `ZSETS` for
    `D_ENV_REDIS_HAS_SORTED_SETS`.
  - The consumer layer named `redis_traits.hpp`, which doesn't exist;
    `redis.hpp` and `redis_table.hpp` are the consumers.
- **Not moved:** `env_redis_config.h` stays in `env/db/redis/` pending #93.

`env_sqlite.h` (`/inc/djinterp/env/db/sqlite/`) was converted and verified
identical to its original in 96 configurations. They covered GCC 13 and
clang 18, as C11, C17, C++17 and C++20, standalone and after `env.h`, across
12 scenarios:
- **Automatic detection:** through stub `sqlite3.h` headers for 3.46.0 and
  3.7.11.
- **Manual detection:** versions and release flags.
- **Build options:** threading modes, FTS3 and FTS5, JSON1 on 3.30, R*Tree,
  sessions, the codec, and omitted features.
- **Overrides:** pre-defined flags.

48 of its 154 object-like macros took both values across the runs; many
flags follow build options only some of which were sampled.
- **Layout:** 19 decimal sections. The library-header section is one item.
  The release IDs and the detection chain gained items and briefs, and the
  comparisons, which already had briefs, are numbered, with the range macro
  in the house form. The threading and JSON section descriptions were
  rewritten to keep their lists readable.
- **Corrected:** section 0 named `cfg_env.h` for a setting in
  `cfg_env_sqlite.h`.
- **Not touched:** `cfg_env_sqlite.h`, which lives in `config/` (#93).

`env_db.h` (`/inc/djinterp/env/db/`) was converted and verified identical to
its original in 99 configurations: GCC 13 and clang 18, as C11, C17 and
C++17, across 33 scenarios. There was one scenario per branch of the automatic
and manual detection chains, plus a versioned MariaDB, and all 16 databases
were identified across the runs.
- **Layout:** 6 decimal sections. The identifier, category and feature bits
  are group items. The detection chain is one item, and the category tests,
  queries, version comparisons and feature combinations are documented. The
  original ran 246 lines past column 80, almost all in the feature-bit lists
  of the detection chain; those lists are reflowed, a whitespace-only change
  that was token-checked.
- **Also changed:** the manual chain now opens with
  `#if defined(D_ENV_DB_DETECTED_MARIADB)` instead of `#ifdef`. The two are
  equivalent, and the `#endif` closing an `#elif` chain shouldn't name one
  database.
- **Corrected:** the banner said the header includes `env.h`; it includes only
  `cfg_env_db.h`.
- **Kept as found:** the DynamoDB branch of #94.

**92. The database headers define nothing when the database isn't detected.**
Each `env/db` header puts its version comparisons and feature flags under
`#if D_ENV_<DB>_DETECTED`, with nothing in the `#else`.

> **Status, 2026.10.04:** open, unchanged: nothing in the tree addresses it yet.

- **The two exceptions:** `env_cassandra.h` and `env_dynamodb.h` have an
  `#else`, but it only zeroes the detected flag of the consumer-facing
  vocabulary each publishes (`D_ENV_CASSANDRA_*`, `D_ENV_DYNAMODB_*`). Their
  `D_ENV_CASS_*` and `D_ENV_DDB_*` feature macros still vanish.
- **The effect in `env_arangodb.h`:** 129 of its 148 macros disappear without
  ArangoDB. `#if D_ENV_ARANGO_HAS_INDEX_TTL` then silently reads 0, and
  `return D_ENV_ARANGO_HAS_INDEX_TTL;` doesn't compile.
- *Advantages (give every macro a defined 0 when the database isn't detected,
  for instance by defining the comparison macros as 0 and moving the flags out
  from under the `#if`):* code can use the flags in C and C++ expressions, and
  `-Wundef` (#20) stays quiet.
- *Disadvantages:* flags not built from the comparisons, such as the edition
  flags, each need their own handling, header by header.
- **Recommendation:** define them all, as each database header is converted.

**93. The database configs predate `cfg_common.h`.**

> **Status, 2026.10.04:** settled 2026.10.04: (a to d, all A) switches read with
> `D_CFG_IS_ON` and `D_CFG_IS_OFF`, and validated (`8c9679e`); the four configs
> moved into `config/` (`a9d5811`); the eight stale copies retired (`7607cbb`);
> `dconfig.h` lists all twelve (`a975b27`).

- **The empty-setting failure:** the config README has modules test settings
  with `D_CFG_IS_ON`, so that an empty definition reads as off. None of the
  eight `cfg_env_*.h` under `config/core/env/db/` includes `cfg_common.h`,
  which provides it. So the headers test `#if (D_CFG_ENV_USING_ARANGODB == 1)`
  instead, and `-DD_CFG_ENV_USING_ARANGODB=` fails with "operator '==' has no
  left operand".
- **Misplaced settings:** four databases keep their settings in
  `env/db/<db>/env_<db>_config.h` rather than in `config/`: cassandra, db2,
  dynamodb and redis.
- *Advantages (include `cfg_common.h` from each db config, test with
  `D_CFG_IS_ON`, and move the four into
  `config/core/env/db/<db>/cfg_env_<db>.h`):* the family follows the config
  subframework like the rest of env.
- *Disadvantages:* all eight configs change, and the four moves need their
  includers updated.
- **Recommendation:** do it, as each database header is converted.
- **Update (2026-09-28, C99 merge):** the first part is done: all eight
  configs, and `cfg_env.h`, now include `cfg_common.h` in place of the
  conditional `dconfig.h`. Still open: testing with `D_CFG_IS_ON`, and
  moving the four configs that live under `env/db/`.

**94. `env_db.h` never identifies DynamoDB (bug).**
Its DynamoDB branch requires `defined(D_ENV_DYNAMODB_DETECTED)`, but
`env_dynamodb.h` includes `env_db.h` before it defines that flag. With the SDK
detected through `env_dynamodb.h`, `D_ENV_DB_ID` reads `0x0000` ("Unknown");
only a standalone `env_db.h` with the flag pre-defined reports DynamoDB
(`0x4000`). The flag is also defined as 0 when DynamoDB isn't detected, so
`defined()` would misfire once the order were fixed. No other database's
branch has this pattern.

> **Status, 2026.10.04:** open, unchanged: nothing in the tree addresses it yet.

- *Advantages (include `env_db.h` at the end of `env_dynamodb.h`, and test the
  flag's value rather than `defined()`):* `D_ENV_DB_ID` and `D_ENV_DB_NAME`
  report DynamoDB.
- *Disadvantages:* both headers change. `env_db.h`'s answer still depends on
  which database header a translation unit includes first, as it does today.
- **Recommendation:** fix it when `env_db.h` is converted.

**95. The documented DynamoDB Local opt-in doesn't exist.**
`env_dynamodb.h` described a `D_ENV_DYNAMODB_LOCAL_HAS_<CAP>` that would turn
a capability on for DynamoDB Local, but nothing reads it: on Local, the
cloud-only capabilities are always 0. The description now says what the code
does.

> **Status, 2026.10.04:** open; the tree already does what it recommends:
> DynamoDB Local is left out, and the description says what the code does.

- *Advantages (implement it, as opt-in constants beside the opt-outs):* a
  project testing against Local can declare what its Local build emulates.
- *Disadvantages:* about 16 more switches, for a development-only target.
- **Recommendation:** leave it out. Guarding the capability flags (#90) would
  cover the rare need to force one on.

**96. `env_mysql_common.h` needs `env.h` but doesn't include it (bug).**
Its platform section tests `D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)` and
`D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)`, which `env.h` defines. Once a MySQL-family
client header is in scope, a translation unit that didn't include `env.h`
first fails with "missing binary operator before token '('", in C and C++.
Verified through `D_CFG_ENV_USING_MARIADB` with MariaDB and Oracle MySQL stub
headers; `env_mysql.h` shares the code.

> **Status, 2026.10.04:** settled earlier: fixed in its conversion, 2026.09.27
> (section 2 of this part).

- *Advantages (include `../../env.h` from `env_mysql_common.h`):* automatic
  detection compiles whatever the include order.
- *Disadvantages:* none beyond the include.
- **Recommendation:** fix it when `env_mysql_common.h` is converted.
- **Resolved 2026-09-27:** fixed in that conversion (section 2).

**97. `env_oracle.h`'s implicit-results flag has a mangled name (bug).**
It defines `D_ENV_ORA_HAS_OCI_helperICIT_RESULTS`, evidently "IMPLICIT" after
a find-and-replace of "IMPL" with "helper". `oracle.hpp`, its only consumer,
tests `D_ENV_ORA_HAS_OCI_IMPLICIT_RESULTS`, which nothing defines. Its
implicit-results support (`has_oci_implicit_results`,
`get_implicit_results()`) is therefore compiled out on every Oracle version.
No other name mangled this way turned up under `inc/`.

> **Status, 2026.10.04:** open, unchanged: nothing in the tree addresses it yet.

- *Advantages (rename it to `D_ENV_ORA_HAS_OCI_IMPLICIT_RESULTS`):*
  `oracle.hpp` gets the feature on Oracle 12.1 and later, as intended.
- *Disadvantages:* that code in `oracle.hpp` has never been compiled, so it
  needs a build against a 12.1 or later OCI.
- **Recommendation:** rename it, then build `oracle.hpp` against such an OCI.

#### The C99 merge (98–99)

**98. The C99 session's dedupe and layout commit (0002).**
The C99 session's series carries one commit the merge did not take. It moves
the C test framework from `test/c` back to `c/test`, because the banners in
its snapshot said `c/test`; ruling 21 settled that the other way, and this
tree's banners and sources say `test/c`. It also deletes duplicate headers and
repoints their includers: `c/dmemory.h` in favor of `c/memory/dmemory.h` (33
includes here), `test_config.h` for `cfg_test.h`, `container_config.h` for
`cfg_container.h`, and `database_common.hpp` for `database.hpp`.

> **Status, 2026.10.04:** settled by the downporting: `dmemory.h` is one header
> in the memory module; the stale c/test sources, `database_common.hpp` and
> `container_config.h` were retired. `test_config.h`, beside `cfg_test.h`, is
> DTest's.

- The two `dmemory.h` copies are not duplicates: `memory/dmemory.h` adds
  `<errno.h>` and `d_memdup`. Repointing is a merge of the two, not a
  deletion.
- `src/djinterp/c/test/` still holds 20 stale copies of the test sources in
  `src/djinterp/test/c/`. They missed the named-union change, so nothing
  should build them.
- *Advantages (take the dedupe, one duplicate pair at a time, keeping this
  tree's `test/c` layout):* one copy of each header, and the includers
  checked pair by pair.
- *Disadvantages:* each pair needs its includers found and repointed first;
  the math session counted four deletions in 0002 that would break a build
  (`test_config.h`, `tree_iterator.hpp`, `database_common.hpp`,
  `type_info_cpp.h`).
- *Alternative (apply 0002 as written):* the C99 session's exact layout, at
  the cost of reversing ruling 21.
- **Recommendation:** keep `test/c`, and dedupe pair by pair, starting with
  `dmemory.h` (merge the two, then repoint) and the 20 stale test sources.

**99. `D_THREAD_LOCAL` can now be undefined.**
The C99 merge brought the C99 session's change to `env_vendor_attributes.h`:
where the compiler has no thread-local storage, `D_THREAD_LOCAL` is left
undefined instead of expanding to nothing, and the new
`D_THREAD_LOCAL_AVAILABLE` says which. An empty expansion silently gave a
per-thread variable ordinary static storage, shared by every thread; the sync
module now tests the flag. Until now every macro in the header was always
defined, the rule #86 cites.

> **Status, 2026.10.04:** open; the tree already does what it recommends:
> `D_THREAD_LOCAL` stays undefined where unavailable, beside
> `D_THREAD_LOCAL_AVAILABLE`.

- *Advantages (keep it):* a would-be thread-local variable fails to compile
  where it would otherwise be a silent data race.
- *Disadvantages:* one exception to the always-defined rule, now stated in the
  banner.
- *Alternative (restore the empty fallback, keep the flag):* the rule holds,
  and correctness again depends on every user checking the flag.
- **Recommendation:** keep it. #86 is a different trade: an empty `D_WEAK`
  fails loudly, as a duplicate symbol at link time, where an empty
  `D_THREAD_LOCAL` fails silently.

### 4. To-dos (not decisions)

> *As of 2026.09.28; not re-audited.*

**In the owner's tree:**
- ~~Delete `env_web.h`. Move the net env headers to `env/net/`, and rename
  `djinterp/web` to `djinterp/net`, updating any includes of `env/web/...`.~~
  **Done in relay 93.** `env_web.h` is retired to `_retired/relay93_env_web/`
  after confirming nothing outside `env/web/` used any `D_ENV_WEB_*` macro;
  `env/web/` and `djinterp/web/` are gone, and no include names either.
  Nothing else in the converted outputs refers to the old locations. The
  name mapping for any code still using `D_ENV_WEB_*`:

  | Old `D_ENV_WEB_*` name | New name |
  |---|---|
  | Transport flags (`HAS_SOCKETS`, `SOCKET_BACKEND`, header probes, `HAS_IPV6`, `BYTE_ORDER`, ...) | `D_ENV_NET_` + same suffix |
  | `HAS_GETADDRINFO`, `IS_LITTLE_ENDIAN`, `IS_BIG_ENDIAN`, `NEEDS_BYTE_SWAP` | `D_ENV_NET_` + same suffix |
  | `HAS_OPENSSL`, `HAS_MBEDTLS`, ... `HAS_SECURETRANSPORT`, `HAS_TLS` | `D_ENV_TLS_` + same suffix |
  | `TLS_BACKEND`, `TLS_BACKEND_NAME`, `TLS_BACKEND_*` | `D_ENV_TLS_BACKEND`, `_NAME`, `_*` |
  | `CAN_TLS` / `CAN_NETWORK` | `D_ENV_TLS_AVAILABLE` / `D_ENV_NET_CAN_TCP` |
  | `HAS_ZLIB`, `HAS_BROTLI`, `HAS_ZSTD` | `D_ENV_COMPRESSION_HAVE_*` |

- Rename the definition of `print_compiler_info` to `d_env_print_compiler_info`.
- Add `env_compress.h` to `compress.hpp`, and `env_pdf.h` to the C++ PDF
  headers, so C++ keeps detection.
- Grep the tree for the old include-guard names, and for files that relied on
  includes removed from `string_fn.h`, `dtime.h`, `djinterp.h` (`<stdint.h>`,
  plus the two attribute headers of #64) and the env headers.
- `env_attributes.h`: its `D_LIKELY` / `D_UNLIKELY` note points to
  `D_EXPECT_TRUE` / `D_EXPECT_FALSE` "(if provided elsewhere)". They are
  provided, by `env_vendor_attributes.h`; name it.

**Found in the 2026-09-28 merge:**
- Two files have CRLF line endings, against ruling 15:
  `c/container/derror.h` and `config/c/util/dmacro_config.h`.
- `c/container/derror.h` is a draft: no banner, tab indentation, and
  nothing includes it.
- Stray files in `src/`: `uxoxo_style_guide_cpp.md`,
  `djinterp/core/util/core archive compress.zip` and
  `djinterp/c/test/test_metadata.zip`.
- The sync module, taken whole from the C99 session, has not had a style
  pass. It typedefs its structs (`d_atomic_llong`, `d_mutex_t`, ...) and
  `dmutex.c` uses `///` section banners.
- 313 quoted includes resolve nowhere. The merge fixed the dawk and
  `circular_array_mtx.h` ones; `dregex.c` and `dcheck.c` still name dawk
  headers that do not exist.

**Files needed from the owner:**
- `print.hpp`'s path, so `env_printer.hpp` can include it.
- `env_archive.c`, so its PATH-probe helpers can move out of the header.
- `compress_common.c`, to write per-function contracts for the fallible
  functions. Several return values are unstated, such as what
  `d_codec_id_from_name`'s `int` means, and the knob setters' results.
- The current `djinterp.h`, if it defines `D_FORMAT_PRINTF` (#59).
- The `fs/` headers (#39) and the implementation files behind #10.
- ~~`env_vendor_attributes.h` (#64).~~ **Received and converted 2026-09-27**
  (`env/c` pass).

**STARTTLS fixes with no choice involved, awaiting the go-ahead:**
- `env_starttls.h`: include `./env_net.h` directly; its include comment
  should stop claiming `D_ENV_OS_ID`, which it never uses. Its banner says it
  "includes both itself".
- `env_starttls.h`: the banner's `[4]` reference is one column right of
  `[1]`–`[3]`.
- `starttls.c`: add a `// openssl` category comment to the OpenSSL include
  group.

**Facts to verify before implementing:**
- #47: `<QtCore/qtversion.h>` is Qt 6.5+.
- #53: PDCursesMod's version numbering.
- #60: each TLS library's TLS 1.3 facts.
- #43: visionOS `TARGET_OS_*` values on a real SDK.
- #63: Mac Catalyst apps may JIT with the allow-jit entitlement.
- #70: MSVC's predefined SSE macros on x64.

---

### 5. File status

> *As of 2026.09.28; not re-audited.*

Converted files so far; each was verified behavior-identical to its original.

- **Framework root:** `djinterp.h`, `djinterp.c`, `djinterp.hpp`.
- **Core env headers:** `env.h`, `env_lang.h`, `env_posix.h`, `env_arch.h`,
  `env_os.h`, `env_compiler.h`, `env_build.h`.
- **Library detection:** `env_compress.h`, `env_archive.h`, `env_pdf.h`,
  `env_compress_link.h`. The archive and compression headers now live in
  `env/util/archive/` and `env/util/compress/`.
- **OS headers:** `env_apple.h`, `env_windows.h`, `env_linux.h`, `env_bsd.h`,
  `env_ios.h`.
- **Net layer (`env/net/`):** `env_net.h`, `env_tls.h`, `env_curl.h`, and
  `env_pop.h` (audited, then fixed per the include-what-you-use item).
- **UI (`env/ui/`):** `env_qt.h`, `env_curses.h`.
- **JIT (`env/jit/`):** `env_jit.h`.
- **Attributes and C runtime (`env/c/`):** `env_attributes.h`, `env_c_lib.h`,
  `env_long_long.h`, `env_vendor_attributes.h`.
- **C++ features (`env/cpp/`):** `env_cpp_features.h`, `env_cpp98.h`.
- **Databases (`env/db/`):** `env_arangodb.h`, `env_cassandra.h`,
  `env_cassandra_config.h`, `env_db2.h`, `env_db2_config.h`, `env_dynamodb.h`,
  `env_dynamodb_config.h`, `env_mariadb.h`, `env_mongodb.h`, `env_mysql.h`,
  `env_mysql_common.h`, `env_oracle.h`, `env_postgres.h`, `env_redis.h`,
  `env_redis_config.h`, `env_sqlite.h`, `env_db.h`.
- **Finishing pass (2026-09-27):** 15 headers converted in earlier sessions
  did not yet follow the house form for multi-condition `#if`s: `env_arch.h`,
  `env_build.h`, `env_compiler.h`, `env_lang.h`, `env_os.h`, `env_pdf.h`,
  `env_posix.h`, `net/env_imap.h`, the five `os/` headers, and both `ui/`
  headers. All of their multi-condition `#if`s and boolean definitions now use
  it, with backslashes on column 80. `env_linux.h`'s
  `D_ENV_LINUX_GLIBC_AT_LEAST` was formatted by hand.
  - **Verification:** all 15 were checked identical to their previous versions,
    macro state and preprocessed output, in 170 comparisons. These covered
    native GCC and clang, as C and C++, and clang targeting AArch64 and RISC-V
    Linux, MSVC on x64 and x86, MinGW, macOS, iOS, Android, FreeBSD, NetBSD
    and OpenBSD.
  - **Left long:** `env_printer.hpp`'s seven lines past column 80, arguments
    aligned under a call whose feature names are long generated identifiers,
    which the guide exempts.
  - **Result:** every header under `inc/djinterp/env/` is now converted.
- **Printer:** `env_printer.hpp`.
- **Modules:** `dstring.h`/`.c`, `dtime.h`/`.c`, `string_fn.h`/`.c`,
  `dio.h`/`.c`, and `compress_common.h` (structural pass; contracts await
  `compress_common.c`).
- **Partial:** `compress.h`, `compress_common.h` and `pdf_primitives.h` got a
  load-order change only.
- **Audited, not changed:** the STARTTLS module (four files, see #54–#59).

**Still to do:**
- `compress.h` (the C notation face), `archive_common.h` and `archive.h`, and
  `pdf_primitives.h`: full passes.
