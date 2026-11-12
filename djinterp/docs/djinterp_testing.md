# djinterp Testing (DTest) — Authoring Guide

This is the single reference for how test suites in djinterp are written, built,
run, and delivered. It covers the goal we are aiming for, the anatomy of a suite
(its four files and where each lives), the two-faced suite header, the section
translation units, the thin runner, the `CMakeLists.txt` wiring, the form a
finished suite is handed over in, the reporting pipeline that turns a suite into
console output and a PDF, and a reference to the framework modules and how to
use them.

DTest has two faces, one per language, and both are frozen as of
**2026-11-06** (Section 13). Sections 1 to 11 describe the **C++ face**, whose
authoring surface is `test_defaults.hpp` and which needs C++17. Section 12
describes the **C face**, `c/modules/test/test_suite.h`, which is plain C99,
compiles at every C and C++ level, and is what a suite uses below C++17.
Section 14 is the migration guide from the older forms, with the converter
(`scripts/dtest/dtest_convert.py`).

---

## 1. The goal — what we want a test suite to be

A DTest suite is a **declarative description of what is being tested, kept as
data**, paired with **test bodies that assert against the real code**. Three
principles drive the design:

1. **The suite is a spec, and the spec lives in the header.** Every suite header
   exposes a single `module_spec` provider — a nested `module → block → test`
   tree in which *every node carries a name and a descriptor*. That spec is the
   one authoritative description of the suite. It is rich enough to render a
   detailed, well-written report on its own.

2. **Runners are thin and do nothing but choose options.** A runner is a
   `main()` that builds a `test_option_set`, hands the spec to `run_module`, and
   returns its exit code. It contains no per-test logic, no report formatting,
   and no templating. Swapping a suite to a different output format is a
   one-line options change in the runner.

3. **All templating lives in one place — `test_defaults.hpp`.** How a report is
   projected onto the console, a PDF, or the parseable `|...|` table is decided
   by `run_module` / `drive_report` inside `test_defaults.hpp`. Change the
   format
   there and every suite inherits it. Runners never re-implement any of it.

The mechanism that makes (1) and (2) coexist in one header is `DTEST_SPEC_MODE`
(Section 3). The pipeline that realizes (3) is described in Section 8.

---

## 2. Anatomy of a suite — four artifacts, four locations

Take a component `something` whose header is
`inc/djinterp/foo/bar/something.hpp`. The slug **`djinterp/foo/bar`** is the key
that repeats verbatim under every root. A suite for it is exactly four kinds of
file:

| Artifact | Path | Role |
| --- | --- | --- |
| **Header under test** | `inc/djinterp/foo/bar/something.hpp` | the code being tested (not part of the suite) |
| **Suite header** | `tests/djinterp/foo/bar/something_tests.hpp` | declarations + fixtures + **the spec provider** |
| **Section TUs** | `tests/djinterp/foo/bar/something_tests_<section>.cpp` | the test bodies, one file per section |
| **Runner** | `build/cmake/config/testing/djinterp/foo/bar/something_tests_runner.cpp` | `main()` — options + `run_module` |
| **CMakeLists** | `build/cmake/config/testing/djinterp/foo/bar/CMakeLists.txt` | wires the exe |

Two things that trip people up:

- **The runner lives in the config tree, not in `tests/`.** `tests/` holds only
  the suite header and the section TUs. The `main()` sits next to its
  `CMakeLists.txt`.
- **The config path is prefixed with `testing/`.** `inc/`, `src/`, and `tests/`
  are not; the config tree is `config/testing/djinterp/...`. That `testing/`
  segment is what lands the built exe under
  `bin/<arch>-<Config>/testing/djinterp/...`.

Linking the runner (`main()`) with the section TUs (which *define* the `tests_*`
bodies) produces the test executable. The suite header is included by both, in
its two different faces.

---

## 3. The suite header — the two faces of `DTEST_SPEC_MODE`

The suite header is included by two different kinds of consumer, and it presents
a different face to each, selected by whether the includer has
`#define DTEST_SPEC_MODE` before the `#include`:

- **Normal mode** (the section TUs — no macro): the header pulls in the
  *header under test* and exposes the **fixtures** and the **check macro** the
  bodies need. The section TUs then define the `tests_*` bodies.
- **Spec mode** (the runner — macro defined): the header pulls in
  `djinterp/cpp/modules/test/test_defaults.hpp` instead (for `module_spec` and
  `run_module`)
  and exposes the **`<shortname>_spec()` provider**. No fixtures are compiled.

The `bool tests_*();` **declarations are visible in both modes** — the section
TUs define them, and the spec provider references them by address.

### 3.1 The three-part structure

Every migrated header follows the same three-part shape. Here is the skeleton
(comments show the invariants):

```cpp
#ifndef DJINTERP_TESTS_SOMETHING_TESTS_
#define DJINTERP_TESTS_SOMETHING_TESTS_ 1

// std
#include <cstdio>
// ... whatever std headers the fixtures need ...

// -- (part 1) mode-gated includes ------------------------------------------
// djinterp core is ALWAYS first and unconditional (NS_*, D_* qualifiers,
// language gates), so both faces have it.
#include <djinterp/cpp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "something.hpp"                       // the header under test (normal)
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/cpp/modules/test/test_defaults.hpp"     // module_spec + run_module (spec)
#endif


NS_DJINTERP
NS_TESTING

// dt names the entities under test (djinterp::test).  Declared UNCONDITIONALLY,
// because the spec provider (spec mode) needs dt::module_spec too.
namespace dt = ::djinterp::test;


// A self-contained check macro: evaluate once, print the failing expression and
// its location, return false.  Unique two letters per suite (D_ST_ here) so
// co-compiled suites never collide.  Left OUTSIDE the fixture guard because it
// depends on nothing from the header under test.
#define D_ST_CHECK(...)                                                       \
    /* do/while + printf, or a routed helper function; returns the condition */

#ifndef DTEST_SPEC_MODE  // (part 1 cont.) fixtures — normal mode only
// ... fixtures that mention the header-under-test's types live HERE, because in
// spec mode those types are not included ...
#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- (part 2) declarations — visible in BOTH modes -------------------------
// One flat list, grouped by section with a banner per section.  The section
// banner names the .cpp that defines the group.

// I.  <SECTION NAME>   (something_tests_<section>.cpp)
bool tests_something_alpha();
bool tests_something_beta();
// ...


// -- (part 3) the spec provider — spec mode only ---------------------------
#ifdef DTEST_SPEC_MODE
inline dt::module_spec
something_spec()
{
    return dt::module_spec{
        "something",
        "<one-paragraph module descriptor: what this module is and does>",
        {
            dt::block_spec{
                "<block-name>",
                "<what this group of tests covers>",
                {
                    { "tests_something_alpha",
                      "<what tests_something_alpha actually asserts>",
                      &tests_something_alpha },
                    { "tests_something_beta",
                      "<what tests_something_beta actually asserts>",
                      &tests_something_beta },
                }
            },
            // ... one block_spec per section ...
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp

#endif  // DJINTERP_TESTS_SOMETHING_TESTS_
```

The three edits that turn a legacy header into this shape:

1. **Mode-gate the include**, put `djinterp/cpp/djinterp.hpp` first and
   unconditional, add `namespace dt = ::djinterp::test;` unconditionally, and
   open `#ifndef DTEST_SPEC_MODE` before the fixtures.
2. **Close `#endif  // !DTEST_SPEC_MODE  (fixtures)`** immediately before the
   first `bool tests_*();` declaration.
3. **Insert the `#ifdef DTEST_SPEC_MODE ... provider ... #endif`** block
   immediately before the closing `NS_END  // testing`.

### 3.2 Where the fixture guard opens

The guard opens **after** anything that is self-contained and **before**
anything that touches the header under test:

- The **check macro** and any pure helper functions it routes through (plain
  `printf` / `strcmp`, no framework types) stay **unconditional**, above the
  guard. The runner never uses them, but leaving them out of the guard is
  harmless and keeps them simple.
- **Fixtures that reference the header-under-test's types** (probes, sample
  builders, type aliases like `using tt = test::test_timer<...>;`) must be
  **inside** the guard — in spec mode the header under test is not included, so
  those names do not exist.

### 3.3 The check macro convention

Each suite defines exactly one check macro with a **unique two-letter suffix**
(`D_TK_`, `D_TP_`, `D_TO_`, `D_RT_`, `D_TR_`, `D_OC_`, `D_TT_`, `D_TE_`, …) so
several suites can be compiled into one TU set without macro collisions. Two
equivalent shapes are in use:

```cpp
// (a) pure macro — a self-contained do/while that returns false on failure
#define D_ST_CHECK(_cond)                                                     \
    do {                                                                      \
        if (!(_cond)) {                                                       \
            std::printf("    check failed: %s\n    at: %s:%d\n",              \
                        #_cond, __FILE__, __LINE__);                          \
            return false;                                                     \
        }                                                                     \
    } while (0)

// (b) routed through a helper — variadic so a top-level comma in a trait
//     expression passes through whole
inline bool
something_check(bool _cond, const char* _expr, const char* _file, int _line)
{
    if (!_cond) std::printf("    [FAIL] %s:%d: %s\n", _file, _line, _expr);
    return _cond;
}
#define D_ST_CHECK(...)                                                       \
    ::djinterp::testing::something_check(                                     \
        (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__)
```

Use the variadic `...` form whenever a checked expression can contain a
top-level comma (e.g. `std::is_same<A, B>::value`).

### 3.4 The spec data model

The provider builds a `module_spec` out of three aggregate types (defined in
`test_defaults.hpp`):

```cpp
struct test_spec   { const char* name; const char* descriptor; test_fn_ptr fn; };
struct block_spec  { const char* name; const char* descriptor; std::vector<test_spec>  tests;  };
struct module_spec { const char* name; const char* descriptor; std::vector<block_spec> blocks; };
```

- A **block** corresponds one-to-one to a **section TU**. Its name is a short
  slug (`"construction"`, `"dispatch"`, `"verdict"`); its descriptor is a
  sentence describing the group.
- A **test** row is `{ "tests_name", "descriptor", &tests_name }`. The `name`
  string should match the function; the descriptor is what the test *verifies*;
  the pointer is the address of the declaration above.

**Naming the provider.** The shortname is the component minus its `test_`
prefix: `test_pack → pack_spec()`, `test_object → object_spec()`,
`test_output → output_spec()`. (`test_kind` is the one exception that kept the
full name `test_kind_spec()`, matching its original runner.)

**Conditional blocks.** If some declarations are gated behind a feature macro,
the matching block or rows must be gated behind the *same* macro so the
address-of references stay in step with the declarations on every build. For
example `test_traits`' concept block is wrapped in
`#if D_ENV_CPP_FEATURE_LANG_CONCEPTS ... #endif`, exactly as its concept-layer
declarations are.

### 3.5 Writing good descriptors

Descriptors are the metadata that makes reports detailed and well-written, so
they must be **accurate to what the test actually checks**, in this order of
preference:

1. **Author `/* Verifies … */` blocks.** Some bodies carry a comment block above
   each test stating what it verifies; lift that sentence directly (this is how
   `session`, `timer`, `counter`, `tree` were written).
2. **The test's first assertion.** Where there is no comment, read the first
   `D_xx_CHECK(...)` in the body and describe what it asserts (this is how
   `pack`, `object`, `defaults`, `event`, `traits`, and the `output/` cluster
   were written).
3. **The function name.** Names in this codebase are highly descriptive and are
   a reliable last resort, but confirm against the body where you can.

Keep each descriptor to one clause; it becomes a single table cell.

### 3.6 The special case — a header that is its own authoring surface

`test_defaults.hpp` is both a header under test *and* the surface that supplies
`module_spec` / `run_module`. Its suite (`test_defaults_tests.hpp`) therefore
does **not** swap the include between modes — `test_defaults.hpp` is included in
both. Only the check helpers are gated. Every other suite swaps normally.

---

## 4. The section translation units

The bodies live in `tests/djinterp/foo/bar/something_tests_<section>.cpp`, one
file per section (matching the header's blocks). A section TU:

- Includes the suite header in **normal mode** (it does **not**
  `#define DTEST_SPEC_MODE`), which gives it the header under test, the
  fixtures, and the check macro.
- Opens `NS_DJINTERP NS_TESTING` and **defines** the `tests_*` functions
  declared in the header, flat in `djinterp::testing`.
- Each `tests_*` returns `bool` — `true` for pass. It runs its assertions
  through the suite's `D_xx_CHECK` macro (which early-returns `false` on the
  first failure) and returns `true` at the end.

```cpp
// tests/djinterp/foo/bar/something_tests_construction.cpp
#include "something_tests.hpp"     // normal mode: fixtures + macro + header under test

NS_DJINTERP
NS_TESTING

bool tests_something_alpha()
{
    something s;
    D_ST_CHECK(s.ready());
    D_ST_CHECK(s.count() == 0u);
    return true;
}
// ... the rest of this section's bodies ...

NS_END
NS_END
```

The header's spec provider and the section TUs stay in step by construction: the
provider references every declaration by address, and each declaration is
defined in exactly one section TU.

---

## 5. The runner

The runner is a thin `main()` in the config tree. It defines
`DTEST_SPEC_MODE` (selecting the header's provider face), includes the suite
header, configures a `test_option_set`, and hands the spec to `run_module`:

```cpp
// build/cmake/config/testing/djinterp/foo/bar/something_tests_runner.cpp
#define DTEST_SPEC_MODE            // suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "something_tests.hpp"     // relative to the tests dir via INCLUDES

int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("something_tests.pdf"));

    return dt::run_module(
        tt::something_spec(),
        "something.hpp unit tests",                 // title
        "<one-line subtitle for the report>",       // subtitle
        opts);
}
```

Notes:

- **`dt` = `::djinterp::test`, `tt` = `::djinterp::testing`.** The spec provider
  is in `djinterp::testing`; the option types and `run_module` are in
  `djinterp::test`.
- **The runner defines `DTEST_SPEC_MODE` itself** (in the `.cpp`), not through
  CMake. CMake needs no `DEFINES` for this.
- **Options only.** The runner never formats a report or names anything the
  pipeline can name for it; all of that is inside `run_module`.
- The header include path (`"something_tests.hpp"` bare) resolves through the
  `INCLUDES` the CMakeLists adds (Section 6). For suites under an extra
  directory level (e.g. `.../test/output/`), the relative `../` count in the
  runner's include changes accordingly, but the shape is identical.

`run_module` returns `0` when the module passes clean and non-zero otherwise, so
`return run_module(...)` is the whole of `main`.

---

## 6. The `CMakeLists.txt`

The config tree mirrors `inc/` / `src/` / `tests/`, and the root config
publishes cache variables you must always use instead of hand-rolled paths:
`DJINTERP_INC_ROOT` (`inc/`), `DJINTERP_TESTS_ROOT` (`tests/`),
`DJINTERP_CONFIG_ROOT`, `DJINTERP_BIN_ROOT`.

### 6.1 The leaf recipe (header-only module, no `src/`)

```cmake
set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/foo/bar")

file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/something_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(something_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/something_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/foo/bar"    # bare header-under-test include
        "${_tests_dir}")                           # bare suite-header include
```

### 6.2 Why exactly those two `INCLUDES`

`djinterp_add_test_executable` adds **only** `inc/` to the target. The suite
header's two faces need two more roots on top of that:

| Include, and where it appears | Resolved by |
| --- | --- |
| `#include "something_tests.hpp"` — bare, **both** faces | `tests/djinterp/foo/bar` |
| `#include "something.hpp"` — bare, **normal** face (section TUs) | `inc/djinterp/foo/bar` |
| `#include "djinterp/cpp/djinterp.hpp"` and `"djinterp/cpp/modules/test/test_defaults.hpp"` — full-from-inc, **spec** face (the runner) | the `inc/` root the helper already adds |

So the **section TUs** (normal mode) are the reason `inc/djinterp/foo/bar` is
needed; the **runner** (spec mode) needs only the helper's `inc/` root. Both
directories listed above cover both faces. The `_main.cpp` filter drops any
stale `tests/…_main.cpp` so two `main()`s never collide at link.

### 6.3 Sources: runner explicit, sections globbed

The **runner** is listed explicitly from *this* directory
(`${CMAKE_CURRENT_SOURCE_DIR}/…_runner.cpp`) because it lives in the config
tree; the **section TUs** are globbed from the `tests/` directory. Together they
link into the exe. The exe lands at
`bin/<arch>-<Config>/testing/djinterp/foo/bar/something_tests` — computed from
where this CMakeLists sits; never set output directories by hand.

### 6.4 Aggregation and clusters

- To make a group target at some level, drop a one-line aggregate:
  `djinterp_add_test_subtree(<globally_unique_name>)`. It recurses into child
  dirs that have a `CMakeLists.txt` and depends on every test exe at or below
  it. Composition is automatic (a leaf is a member of every aggregate above it).
- A directory may be a leaf and an aggregate at once. If a directory already
  carries a CMakeLists for a sibling suite (e.g. several suites share
  `output/`), do **not** add a second file — **merge** the new
  `djinterp_add_test_executable(...)` call into the existing one (a multi-exe
  cluster). Recursion only enters directories that have a `CMakeLists.txt`, so
  the parent must have an aggregate for the build to descend.
- That merge rule is for suites **already in the tree**. A suite arriving as a
  delivery takes the leaf-directory form instead, because a zip cannot merge
  into a file it cannot see — see Section 7.2.

The helper links `djinterp_c_runtime`, defines `D_TESTING=1`, builds C++20/C17
`/permissive- /Zc:preprocessor` under MSVC, and registers CTest
(`ctest -R something_tests`).

---

## 7. Delivering a suite — the zip and its tree

A finished suite is handed over as **a single zip whose root is the repository
root**. Unzipping it at the top of a checkout puts every file exactly where
Section 2 says it belongs — no path fixing, no per-file copying, no guessing at
which half of the tree a given file was meant for. Nothing in the archive is
nested under a project folder, a date stamp, or the component name.

### 7.1 The tree

For a component `something` under the slug `djinterp/foo/bar`:

```
<zip root>
----build
--------cmake
------------config
----------------testing
--------------------djinterp
------------------------foo
----------------------------bar
--------------------------------something
------------------------------------CMakeLists.txt
------------------------------------something_tests_runner.cpp
----tests
--------djinterp
------------foo
----------------bar
--------------------something_tests.hpp
--------------------something_tests_<section>.cpp     (one per block)
```

Two rules the tree encodes:

- **The `tests/` side is flat under the slug.** The suite header and every
  section TU sit directly in `tests/djinterp/foo/bar/`, beside any sibling
  suite's files. They are told apart by their `something_tests_` stem, which is
  exactly what the CMakeLists glob keys on — which is why the stem has to be the
  component name and nothing else.
- **The config side gets one leaf directory per component.** The runner and its
  `CMakeLists.txt` live together in `.../bar/something/`, one level below the
  slug. Each component therefore ships a self-contained pair that can be dropped
  in, built, moved, or deleted without touching another suite's wiring.

### 7.2 Why a leaf directory per component, when 6.4 says merge

Section 6.4 tells you to merge a new `djinterp_add_test_executable(...)` call
into a CMakeLists that already exists rather than adding a second file. That is
the right rule **inside the tree**, where you can see the file you are merging
into. It is the wrong rule for a delivery, for a mechanical reason: a zip
overwrites. An archive that shipped `.../bar/CMakeLists.txt` would silently
replace whatever wiring already sat there, and neither side would find out until
some unrelated suite stopped building.

So a delivery never writes a file it does not own. Giving each component its own
leaf directory makes ownership total — every path in the zip is new, and
unzipping cannot destroy anything. Merging into a shared cluster afterwards is a
local decision you can make with the existing file in front of you; collapsing
the leaves back into one CMakeLists is a copy of three
`djinterp_add_test_executable` calls and a delete.

### 7.3 What the zip deliberately leaves out

- **Parent aggregates.** No `CMakeLists.txt` at `.../foo/bar/` or `.../foo/`,
  for the overwrite reason above. Recursion only enters directories that have
  one (6.4), so **if the parent has no CMakeLists, nothing below it is reached**
  — add the one-line `djinterp_add_test_subtree(<unique_name>)` yourself. A
  delivery that omits an aggregate should say so out loud rather than leave it
  to be discovered by a suite that never runs.
- **The header under test.** A suite ships test artifacts only. The module is
  the thing being tested, not part of the deliverable.
- **Build output**, scratch mains, and anything under `bin/`.

### 7.4 Broken dependencies are reported, never patched

If the header under test does not compile as shipped — a stale macro name, a
dangling include, a claim the code cannot honour — the suite is still delivered,
and the fix is **recorded in the suite header's preamble** under a `BUILD
PREREQUISITE` heading, with the exact substitution needed. The delivery does not
edit the module under test and does not quietly work around it.

The reason is that a test suite is evidence, and evidence that silently repaired
its own subject proves nothing. A suite whose preamble names the two-token fix
it needs is a suite you can trust; one that shipped green against a header the
author had patched is a suite you cannot. The same applies to behaviour the
tests found and did not like: pin it, name it in the descriptor, and say in the
preamble that the assertion is expected to invert when the module is fixed.

### 7.5 Cross-slug includes

Section 6.2 lists exactly two `INCLUDES` because the ordinary suite reaches
exactly two places. A suite that includes a header from **another** slug needs
that slug's `inc/` directory listed as well — most often a composition test,
asserting that the module does or does not satisfy a protocol defined elsewhere
in the tree:

```cmake
INCLUDES
    "${DJINTERP_INC_ROOT}/djinterp/foo/bar"      # the header under test
    "${DJINTERP_INC_ROOT}/djinterp/other/slug"   # a header it is checked on
    "${_tests_dir}")
```

Worth a comment in the CMakeLists saying which test needs the extra root, since
it is the first thing to delete if that test is ever dropped.

### 7.6 Before zipping

All mechanical, all cheap, and each one catches a class of failure that
otherwise surfaces on someone else's machine:

| Check | Catches |
| --- | --- |
| Both faces compile (`-Wall -Wextra -Wpedantic -Werror`) | a guard that only closes in one mode |
| Provider ↔ declarations ↔ definitions parity | a test declared and never written, or written and never listed |
| Block names match section filenames | a block renamed on one side only |
| Balanced preprocessor guards per header | the fixture guard left open |
| Glob matches only its own component's sections | a sibling suite's TUs pulled into the wrong exe |
| No tabs, no trailing whitespace, nothing over 79 columns | style-gate failures after handoff |
| Unzip into a scratch checkout | any path in the archive that would overwrite |

The spec provider can be compiled and **linked against the real section TUs**
even where the full framework is unavailable, by standing in a minimal
`test_spec` / `block_spec` / `module_spec` triple. That is worth doing: linking
is what proves every `&tests_x` in the provider resolves to a definition, which
is the parity check above in its strongest form.

---

## 8. The reporting pipeline — how a spec becomes output

Everything a runner triggers happens inside `test_defaults.hpp`. This is the one
place to change formatting.

### 8.1 Entry points

```cpp
// options-driven (what the runners use)
int run_module(const module_spec&, const char* title, const char* subtitle,
               const test_option_set& opts);
int run_suite (const char* suite_name, const char* descriptor,
               const std::vector<module_spec>&, const char* title,
               const test_option_set& opts);

// convenience overloads (title/subtitle + a bare PDF path)
int run_module(const module_spec&, const char* title, const char* subtitle,
               const char* pdf);
int run_suite (..., const char* pdf);
```

All four funnel through **`drive_report(report_builder&, const module_spec&)`**,
which walks the spec, runs each test's predicate once, and records the outcome
into a `report_builder` (open a unit, run each check, close the unit). The
builder then renders to whatever the options requested (console always; a PDF
when `document == pdf`) and `run_module` returns the exit code.

### 8.2 The parseable `|...|` table

`test_defaults.hpp` also owns a Markdown-pipe view of the results, so a report
can be re-parsed by tooling. It is the one place the `|...|` grammar is defined:

```cpp
std::string render_spec_pipe_table  (const module_spec&);   // RESULT | # | TEST | DESCRIPTION
std::string render_report_pipe_table(const test_report&);   // MODULE | UNIT | RESULT | CHECK
```

- `render_spec_pipe_table` runs each predicate to fill its `RESULT` cell and is
  what the entry points emit — once per module, under a greppable caption, via
  the shared `internal::emit_spec_pipe_table`.
- `render_report_pipe_table` sources the same idea from an
  already-populated `test_report`, so it runs **nothing**; use it when you hold
  a finished report.

Both are plain string assembly over `<string>`/`<vector>` (no dependency on the
table subframework), and their output is exactly what `parse_table_text<>()`
reads back. Change the columns or spacing here and every suite's table changes.

### 8.3 The report model (the data the pipeline builds)

`test_report.hpp` defines the tree the builder fills and the verdict logic:

```
test_report      (title, subtitle, description, modules[])   → a run verdict
  report_module  (name, units[])                             → a module verdict
    report_unit  (name, checks[], verdict, elapsed_ns)       → a unit verdict
      report_check (description, status)                     → a leaf
```

A **verdict** (`report_verdict`: `empty`, `passed`, `failed`, `pending`) folds
up the tree with a fixed precedence (`failure > pending > passed`; all-`skipped`
reads as `passed`; a zero total is `empty`). `report_pass_rate` / `report_ratio`
format the summary line, and `expand_report_file_name` builds the output path
from a pattern (`{module}`, `{title}`, `{index}`, `{ext}`, date) with
filesystem-sanitised components.

---

## 9. Options reference (`test_option_set`)

A runner starts from `default_test_options()` and sets what it needs. The keys
seen in use:

| Key | Member | Meaning |
| --- | --- | --- |
| `test_option::document` | `document` | output document type (`test_doc_type`: `text`, `markdown`, `html`, `xml`, `pdf`, ...) |
| `test_option::output_file` | `output_path` | output path (a `std::string`) |
| `test_option::show` | `show` | what the console lists (`DTestShow`) |
| `test_option::pack` | `pack` | packaging mode (`DTestOutputPack`: none / compress / archive) |
| `test_option::compressor` | `compressor` | codec for compress mode (`test_compressor`) |
| `test_option::compress_opts` | `compress_opts` | codec options (level, ...) |
| `test_option::archive_format` | `archive_format` | container for archive mode (`test_archive_format`) |
| `test_option::archive_opts` | `archive_opts` | archive options |
| `test_option::split` | `split` | one document for the run, or one per module (`DTestOutputSplit`) |

`test_option_set` has **one shape at every level** the C++ face compiles at: a
struct with a named member per aspect, and keyed access to the same members.
Both spellings compile everywhere, so no fixture needs an `#if`:

```cpp
dt::test_option_set o = dt::default_test_options();
o.set<dt::test_option::document>(dt::test_doc_type::pdf);   // keyed
o.output_path = "suite.pdf";                                // named member
bool c = o.get<dt::test_option::color>();                   // keyed read
```

Until 2026-11-05 the set was an `option_set<>` at C++20 and a plain struct
below, which the C++ guide rules out ("no level changes what a shared type
is"); fixtures carried a face-neutral setter with an `#if` on
`D_ENV_LANG_IS_CPP20_OR_HIGHER`. Those setters still compile and may simply
call `set<>()`; new code writes `set<>()` or the member directly. The framework
itself reads options through the free accessors (`document(o)`,
`output_path(o)`, ...).

### 9.1 Packaging the report

With `D_TEST_REPORT_ENABLE_ARCHIVE` defined, `report_builder` packages the
documents it renders as `pack` says. Without it, every document is written
loose.

- **`none`**: each document is written as it is, under its own name.
- **`compress`**: each document is compressed on its own with `compressor`,
  tuned by `compress_opts`. It is written where its loose file would have gone,
  with the codec's suffix appended: `suite.pdf` becomes `suite.pdf.gz`. Under
  `store` the name stays as it is.
- **`archive`**: every document goes into one container in `archive_format`,
  each entry under its own name. An explicit `output_file` names the
  container; otherwise it is `report` plus the format's extension.

A container's entries are stored or compressed as `archive_opts` says:
`store_only` (1 stored, 0 compressed), `level`, and the method in `zip.method`
or `sevenzip.method`. A stored PDF can be searched inside the container
without a codec. A compressed one is much smaller, since DTest's PDFs are not
compressed within.

**Who decides.** The runtime option always wins. Where a runner leaves one
alone, `default_test_options()` holds the build's knob, which falls back to
the library's own knob, which falls back to the default below (the owner's
ruling of 2026-11-09: complete configurability when the user decides,
sensible defaults when not). A build that sets only the library's knobs
packages DTest's reports the same way as everything else. One that sets
DTest's packages the reports one way and everything else another.

| Runtime option | DTest knob (`cfg_test_options.h`) | Library knob | Default |
| --- | --- | --- | --- |
| `pack` | `D_CFG_TEST_OUTPUT_PACK` | `D_CFG_OUTPUT_PACK` | `none` |
| `compressor` | `D_CFG_TEST_OUTPUT_CODEC` | `D_CFG_OUTPUT_CODEC` | `gzip` |
| `archive_format` | `D_CFG_TEST_OUTPUT_FORMAT` | `D_CFG_OUTPUT_FORMAT` | `zip` |
| `archive_opts.store_only` | `D_CFG_TEST_OUTPUT_STORE_ONLY` | `D_CFG_ARCHIVE_STORE_ONLY` | compressed |
| `archive_opts.level` | `D_CFG_TEST_OUTPUT_LEVEL` | `D_CFG_ARCHIVE_LEVEL` | the codec's default |
| `archive_opts.zip.method` | | `D_CFG_ARCHIVE_ZIP_METHOD` | deflate where the build has it, else stored |
| `archive_opts.sevenzip.method` | | `D_CFG_ARCHIVE_SEVENZIP_METHOD` | LZMA2 |

The library's knobs live in `config/cpp/util/output/cfg_output_packaging.h`
and `config/c/util/archive/cfg_archive.h`. Each knob is a number:
- the codec is a `dj_codec_id` (0 store, 1 deflate, 2 zlib, 3 gzip, 4 bzip2,
  5 xz, 6 zstd, 7 lz4, 8 brotli);
- the container is a `dj_format_id` (0 zip, 1 tar, 2 gz, 3 tar.gz, 4 7z,
  5 rar). `test_archive_format` orders gz and tar.gz the other way round, and
  `cfg_test_options.h` converts;
- DTest's two entry knobs take -1, the default, to leave `archive_opts` unset.

A value out of range stops the build with an `#error` that names the knob.
For example, `-DD_CFG_TEST_OUTPUT_STORE_ONLY=1` stores DTest's reports in
their containers and leaves everything else compressed.

---

## 10. Framework module reference

The modules below are the pieces a suite is built on. They live under
`inc/djinterp/cpp/modules/test/` (the reporting/output group under
`inc/djinterp/cpp/modules/test/output/`), with the event substrate under
`inc/djinterp/cpp/event/`.

### 10.1 Foundation

- **`test_common.hpp`** — the base types every other module builds on:
  `test_type_id` (the kind id), `test_callable_id` (1-based, `k_no_callable = 0`
  sentinel), the `test_status` enum (`passed` / `failed` / `skipped` / `error` /
  `pending`), the `test_event` record, and the `D_TEST_CONSTEXPR` /
  `D_TEST_STATIC_CONSTEXPR` language-level macros.

- **`test_kind.hpp`** — the ranked **test-kind** record at the centre of the
  six-kind taxonomy (`test_suite > test_module > test_block > unit_test >
  test_fn > assert`). Provides `make_test_kind`, a flat kind-set wrapper with
  compile-time `contains` dispatch, and the resolved queries `find_kind`,
  `rank_of`, `is_leaf`, `name_of`, `can_be_child_of`.

- **`test_object.hpp`** — the unified **node**: a `test_object<>` carrying a
  type
  id, an evaluated result, a `test_status`, and a `test_metadata` key/value
  store. Aliases `basic_test` (the default instantiation) and `tagged_test`, and
  the factories `make_test` / `make_interior`.

### 10.2 Structure and run context

- **`test_tree.hpp`** — the default test **container**: a six-kind set paired
  with a node forest whose implied *conjunctive root* anchors every top-level
  root. `add_root` (unconstrained) and rank-checked `append_child` build the
  forest; `count_by_status`, `all_passed`, and `any_failed` roll it up. A
  `validate_rank == false` instantiation compiles the rank/leaf rules out.

- **`test_session.hpp`** — the top-level **run context**: an owned tree,
  per-status counters, a timer, a lifecycle state machine
  (`run`/`pause`/`resume`/`finish`/`reset` with legal transitions and no-ops),
  the handler-driven walk (`run(handler)` / `run(handler, tree)`), and the
  verdict decision.

### 10.3 Instruments

- **`test_timer.hpp`** — a `util::timer` wrapped with an optional event-dispatch
  layer, over a caller-supplied clock. Owns child timers by value and can
  observe external ones non-owningly; fires `on_start` / `on_stop` /
  `on_expire` / `on_reset` when a dispatcher is attached, and is zero-overhead
  when none is.

- **`test_counter.hpp`** — the same shape for a `util::counter`: bounded value
  with clamping, `increment` / `decrement` / `reset`, matching events
  (`on_increment` / `on_decrement` / `on_limit` / `on_reset`), owned and
  observed children.

### 10.4 Events

- **`test_event.hpp`** — the DTest event **alphabet**: twelve built-in tags
  (session / module / test lifecycle, terminal statuses, error, status-change,
  listener-threw), each with a compile-time payload contract and a `name()`, and
  the `D_TEST_EVENT` / `D_TEST_EVENT_MSG` / `D_TEST_EVENT_X` macros for
  declaring
  custom tags in any namespace.
- **`djinterp/cpp/event/` — the substrate**: `event_dispatcher`
  (bind / fire — the object tags plug into), plus `event_common`,
  `event_handler`, `event_registry`, and `event_table`. A listener bound to a
  tag is invoked with that tag's payload; a `consume` verdict halts propagation;
  distinct tags are independent channels.

### 10.5 Options, authoring, and running

- **`test_options.hpp`** — the option system: `test_option_set`, the
  `test_option` keys, and `default_test_options()` (Section 9).
- **`test_defaults.hpp`** — the **authoring surface** and the pipeline. Supplies
  `module_spec` / `block_spec` / `test_spec`, the kind constants and
  `default_test_kinds`, the node factories, `build_enriched_tree` (lowers a spec
  into the six-kind tree), `drive_report`, the `run_module` / `run_suite` entry
  points, and the `|...|` table view (Section 8). **This is the one file to edit
  to change how reports are produced.**
- **`test_builder.hpp`** — the imperative `report_builder` that `drive_report`
  drives (`module`, `open_unit`, `check`, `close_unit`, `set_title`, `use_pdf`,
  `finish`). Suites do not use it directly; the pipeline does.
- **`test_context.hpp`** — the run **focus**: a `test_context` cursor over the
  report/module/unit/check being described, with narrowing accessors. The report
  renderer borrows it to resolve scope-dependent tokens.
- **`test_callable.hpp`**, **`test_container.hpp`**, **`test_handler.hpp`**,
  **`test_runner.hpp`** — the deferred-callable binding, the container
  abstractions, the listener/handler wiring, and the runner harness that the
  higher-level surface composes. (Each already ships a `*_spec()` suite in the
  DTEST_SPEC_MODE idiom.)

### 10.6 Reporting and output

- **`test_report.hpp`** — the report **data model** and verdict logic
  (Section 8.3).
- **`output/test_report_document.hpp`**, **`output/test_report_content.hpp`**,
  **`output/test_report_pdf.hpp`**, **`output/test_layout.hpp`** and
  **`output/test_layout_suite.hpp`** — the report's document, its content, the
  PDF rendering and the page layout, which `report_builder` drives.
- **`output/test_packaging.hpp`** — packaging a finished report: compression
  and archiving through `util/output/output_packaging.hpp`.
- The earlier output layer (`test_render.hpp`, `test_render_pdf.hpp`,
  `test_output.hpp`, `test_output_config.hpp`, `test_zip_store.hpp`) was
  retired by the owner on 2026-10 (`be9080d3`); `test_emit.hpp`, which was
  built on it, and the suites and leaves of all six were retired on 2026-11-05.

### 10.7 Meta-testing and packaging

- **`test_traits.hpp`** — the **trait-testing toolkit** the suites lean on for
  compile-time checks: probe declarators (type / expression / noexcept /
  constexpr), the readings (`is_nothrow_probe`, value-category, `is_valid`), the
  non-short-circuiting type-set quantifiers (`count_holds`,
  `holds_for_all/any/none`), the bool-trait shape check (`is_bool_trait`), the
  cv-ref agreement matrix, a fixture "zoo" of hostile types, build-time pins,
  and
  a C++20 concept layer.
- **`test_pack.hpp`** — the bridge from a runtime packaging request (the
  `pack` / `compressor` / `archive_format` knobs) to the matching compile-time
  compress/archive facade call: the output-naming helpers, availability routers,
  the internal enum→tag dispatch, and `pack_report`.

### 10.8 Concurrency

- **`inc/djinterp/cpp/modules/test/sync/`** — the concurrency subframework:
  `test_thread`, `test_race`, `test_deadlock`, `test_stress`,
  `test_invariant`, `test_threadsafe`, `test_concurrent`, and the `test_sync`
  façade over them. Each has a suite in `tests/djinterp/cpp/modules/test/sync/`
  and a leaf of its own, and `sync_tests` runs them together. The headers
  are C++11 and up, with parts that need C++17 or C++20; their suites, like
  every C++ suite, are C++17 and up. They are framework internals, not part
  of the frozen surface of Section 13.

---

## 11. Authoring checklist

To add or migrate a suite:

1. **Suite header** (`tests/djinterp/foo/bar/something_tests.hpp`):
   - `djinterp/cpp/djinterp.hpp` first and unconditional.
   - `#ifndef DTEST_SPEC_MODE` → the header under test; `#ifdef DTEST_SPEC_MODE`
     → `djinterp/cpp/modules/test/test_defaults.hpp`. (Exception: a header that
     is its own
     authoring surface is included in both.)
   - `namespace dt = ::djinterp::test;` unconditional.
   - `#ifndef DTEST_SPEC_MODE` around the fixtures; the self-contained check
     macro stays above it.
   - `bool tests_*();` declarations, grouped by section, visible in both modes.
   - `#ifdef DTEST_SPEC_MODE` → `inline dt::module_spec <shortname>_spec()` with
     one `block_spec` per section and a body-accurate descriptor on every node;
     gate any conditional block on the same macro as its declarations.
2. **Section TUs** (`…_tests_<section>.cpp`): normal-mode include, define the
   `tests_*` bodies flat in `djinterp::testing`, one file per block.
3. **Runner** (`config/testing/djinterp/foo/bar/something_tests_runner.cpp`):
   `#define DTEST_SPEC_MODE`, include the suite header, build `opts`, call
   `run_module(tt::<shortname>_spec(), title, subtitle, opts)`.
4. **CMakeLists** (same config dir): the leaf recipe — glob the section TUs,
   list the runner, `INCLUDES = inc/djinterp/foo/bar + tests/djinterp/foo/bar`.
   If the directory already has a CMakeLists, merge the call into it — unless
   this is a delivery, in which case it gets its own leaf directory (Section
   7.2).
5. **Delivery** (Section 7): a single zip rooted at the repository root; the
   runner and its CMakeLists in their own leaf directory
   `config/testing/djinterp/foo/bar/something/`; no parent aggregates, no
   header under test; any `BUILD PREREQUISITE` recorded in the suite header's
   preamble rather than patched into the module.

Invariants worth checking before you call it done (all mechanical):

- **Provider ↔ declarations parity** — every `&tests_x` in the provider has a
  matching `bool tests_x();` declaration, and vice versa.
- **Balanced braces/parens** in the header.
- **Balanced preprocessor guards**, including any nested feature `#if`s.
- **Runner is options-only** — no `report_builder`, no per-test calls, no
  formatting.
- **Block names match section filenames**, so the report and the tree agree.
- **The glob matches only its own component's sections** — check it against a
  directory that holds sibling suites.
- **Nothing in the zip overwrites** — unzip into a scratch checkout first.

---

## 12. The C face — writing a C suite

`inc/djinterp/c/modules/test/test_suite.h` (with `src/.../test_suite.c`) is
DTest's C face: a suite is a tree of **modules, blocks and tests**, a test is a
run of **checks**, and the report is the same as the C++ face's in the part
tooling reads — a line per test, a closing summary, and per module the `|...|`
results table with C++'s caption and columns (8.2). It is C99, includes nothing
of the C++ framework, and compiles as C at C99 to C23 and as C++ at every level
from strict C++98. Use it for every C suite, and for a C++ suite that must run
below C++17, where `test_defaults.hpp` is empty.

At C++98 and C++03, GCC and Clang take variadic macros as an extension, so
the framework defines them there (`D_TEST_NOTE` here, and `D_ALLOC_SIZE` and
`D_NONNULL` in `env/c/env_vendor_attributes.h`). The extension is only a
warning, but `-pedantic-errors` makes it an error, and no macro tells the
headers that flag is set. A C++98 or C++03 build under `-pedantic-errors`
therefore adds `-Wno-variadic-macros`.

It is not thread-safe: a suite is built and run by one thread, and one suite
runs at a time in a process (a test that starts threads joins them first).

### 12.1 The four files

As in Section 2, keyed by the slug; C names, `d_tests_` functions:

| Artifact | Path |
| --- | --- |
| Suite header | `tests/djinterp/c/foo/bar_tests.h` |
| Section TUs | `tests/djinterp/c/foo/bar_tests_<section>.c` |
| Runner | `build/cmake/config/testing/djinterp/c/foo/bar/bar_tests_runner.c` |
| CMakeLists | `build/cmake/config/testing/djinterp/c/foo/bar/CMakeLists.txt` |

The working examples are DTest's own suite (`tests/djinterp/c/modules/test/
test_suite_tests*`) and the root's (`tests/djinterp/c/{djinterp,dstring,
dtime,string_fn,dio}_tests*`), each with its leaf.

### 12.2 The spec form (the default)

The suite header has the two faces of Section 3, selected by the same
`DTEST_SPEC_MODE`. Both faces declare the tests; the normal face also includes
the header under test; the spec face defines the tables and the provider:

```c
#include "../../../inc/djinterp/c/modules/test/test_suite.h"
#ifndef DTEST_SPEC_MODE
    #include "../../../inc/djinterp/c/foo/bar.h"   // the header under test
#endif

bool d_tests_bar_parses(void);
bool d_tests_bar_rejects_null(void);

#ifdef DTEST_SPEC_MODE
static const struct dj_test_case_spec d_tests_bar_parse_rows[] =
{
    { "d_tests_bar_parses", "a well-formed input parses", d_tests_bar_parses },
    { "d_tests_bar_rejects_null", "NULL is EINVAL", d_tests_bar_rejects_null }
};
static const struct dj_test_block_spec d_tests_bar_blocks[] =
{
    { "parse", "parsing", d_tests_bar_parse_rows,
      sizeof(d_tests_bar_parse_rows) / sizeof(d_tests_bar_parse_rows[0]) }
};
static const struct dj_test_module_spec d_tests_bar_module =
{
    "bar", "bar.h: what it is", d_tests_bar_blocks,
    sizeof(d_tests_bar_blocks) / sizeof(d_tests_bar_blocks[0])
};
D_STATIC_INLINE const struct dj_test_module_spec* d_tests_bar_spec(void)
{
    return &d_tests_bar_module;
}
#endif
```

A test is a `bool (void)` that checks with `D_TEST_REQUIRE(expr)` — which
records the check, with its text, file and line, and returns `false` from the
test when it fails — and returns `true` at its end. `D_TEST_SKIP("why")` skips
it, and `D_TEST_NOTE("got %d", n)` attaches a note (12.5). A test that makes no
require counts its return as its one check, which is
exactly a C++ spec test's `bool`, so **a C++ suite's test functions can be
listed in a C spec table** and run at every C++ level.

The runner chooses options and nothing else:

```c
#define DTEST_SPEC_MODE
#include "bar_tests.h"

int main(int _argc, char* _argv[])
{
    const struct dj_test_module_spec* modules[1];

    modules[0] = d_tests_bar_spec();

    return d_test_run_modules(_argc, _argv, "bar.h unit tests", modules, 1);
}
```

`d_test_run_modules` reads `--failures`, `--tests` (the default), `--checks`,
`--no-table`, `--filter=TEXT` (a module/block/test path) and `--help`, runs
every selected test once, prints the report and returns the exit code: **0**
every test passed or was skipped, **1** a test failed or made no check (or no
test ran at all), **2** the suite could not record a node or check, or the
command line was refused.

### 12.3 The imperative form

For a suite that builds its cases in loops, or must evaluate in an order the
runner controls (the filesys suite stands up a scratch directory first), a
section is a `void d_tests_<module>_<section>(struct dj_test_suite*)` that opens
nodes and checks against the test opened last:

```c
d_test_suite_module(s, "file_open");
d_test_suite_block(s, "d_file_open_stream");
d_test_suite_test(s, "NULL path -> NULL, EINVAL");
D_TEST_CHECK(s, d_file_open_stream(NULL, "r") == NULL);
D_TEST_CHECK_MSG(s, "errno", errno == EINVAL, "errno should be EINVAL");
```

`D_TEST_CHECK` returns the check's result, so a section can stop on it.
`d_test_suite_describe` gives the node opened last a descriptor;
`d_test_suite_note` attaches a note to it (12.5); `d_test_suite_skip` skips the
open test. The runner creates the suite, applies
the parsed options (`d_test_suite_options_init`, `_parse`, `d_test_suite_
set_options`), calls the sections, and returns `d_test_suite_run`. A spec and
imperative sections can share one suite (`d_test_suite_add_module`).

### 12.4 The CMake leaf

```cmake
set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/foo")
file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/bar_tests*.c")

djinterp_add_test_executable(c_bar_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/bar_tests_runner.c"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/foo/bar.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
    INCLUDES
        "${_tests_dir}")
```

The glob takes `bar_tests.c` as well as `bar_tests_*.c`: a suite converted
from the standalone form keeps its aggregator there (14.2). Until
`djinterp_c_runtime` carries `test_suite.c` (requested of lane A on
2026-11-06), each leaf lists it. Prefix C targets with `c_`: target names are
global, and the C++ suite of the same header takes the bare name.

### 12.5 Notes, not prints

A suite does not print. What a check cannot carry — the value a failed check
got, the build a module ran on — goes in a note, which the report prints
beneath its node, through the suite's stream:

```c
if (!D_TEST_CHECK(s, count == 3))
{
    d_test_suite_note(s, "got %d", count);
}
```

`d_test_suite_note(suite, format, ...)` (or `_vnote` with a `va_list`) attaches
a printf-formatted note to the node opened last: a test, or a module or block
when no test is open. In a spec test, `D_TEST_NOTE(format, ...)` attaches one
to the test; at ISO C++98, which has no variadic macros, the macro is absent
and a test calls `d_test_suite_note_current(format, ...)`. A note's text may
run to several lines. The formats are checked as printf's are
(`D_FORMAT_PRINTF`).

The report prints every note at `--checks`. Otherwise it prints a test's
notes when the test fails, and a module's or block's when a test beneath it
fails, so a passing run stays as short as it was. A note is kept whatever the
options, since the verdict that decides it comes later: note beside a check,
not in a loop of thousands.

A module's build profile is the case for a heading's note: env's suite notes
on its module the report `env_printer.h` collects for the unit,
`d_env_report_here()`, as `section.name=value` lines, the printer's keyvalue
form (`tests/djinterp/c/env_tests_profile.c`), so a failure in any env test
comes with the build it failed on, and two builds' profiles compare line by
line. A program prints the same with
`d_env_print_file(stdout, "style=keyvalue; show_title=0; sections=all")`.

---

## 13. DTest's frozen API (2026-11-06)

From 2026-11-06 every lane writes its suites against what this section lists.
A frozen name is not removed or changed in meaning before 1.0.0; additions are
allowed, and a change that would break a suite goes to the owner as a
decision with its migration first.

**C face** (`c/modules/test/test_suite.h`), all of it: the types
`dj_test_suite` (opaque), `dj_test_summary`, `dj_test_suite_options`,
`fn_test_case`, `dj_test_case_spec`, `dj_test_block_spec`,
`dj_test_module_spec`; the constants `D_TEST_SUITE_SHOW_FAILURES` / `_TESTS` /
`_CHECKS` and `D_TEST_SUITE_EXIT_PASSED` / `_FAILED` / `_ERROR`; the functions
`d_test_suite_new`, `_free`, `_options_init`, `_options_parse`,
`_set_options`, `_module`, `_block`, `_test`, `_describe`, `_note` and
`_vnote` (added 2026-11-07), `_check`, `_check_msg`, `_skip`, `_add_module`,
`_run`, `d_test_run_modules`; the macros `D_TEST_CHECK`, `D_TEST_CHECK_MSG`,
`D_TEST_REQUIRE`, `D_TEST_SKIP`, `D_TEST_NOTE` (added 2026-11-07); the
NULL-safe `d_test_suite_str_equal`; the
command-line options of 12.2 and the exit codes; and the report's results
table. (`d_test_suite_require`, `d_test_suite_skip_current` and
`d_test_suite_note_current` exist for the macros and are not for direct use,
but for `d_test_suite_note_current` at ISO C++98, where `D_TEST_NOTE` is
absent.)

**C++ face** (`cpp/modules/test/test_defaults.hpp`, C++17 and up): the suite
layout of Sections 2 to 7 and its `DTEST_SPEC_MODE` idiom; `test_spec`,
`block_spec`, `module_spec`; `run_module` and `run_suite` in both their
overloads; `test_option_set` in its one shape, the `test_option` keys,
`get<>`/`set<>`, the members of Section 9 and `default_test_options()`; the
two `|...|` tables of 8.2; and the exit code `run_module` returns.

**Shared**: the status values of `c/modules/test/test_common.h`
(`D_TEST_STATUS_*`, with C++'s `test_status` over the same values), and the
counters of 13.1. The field-form tally that stood here until 2026-11-09
(`dj_test_counter` with four `size_t` fields, `d_test_counter_reset` and
`_add`, style #79) is replaced by the owner's ruling of that date; 13.1 says
how to move.

**Not frozen**, and not to be written against: the classic C tree
(`default/test.h`, `test_block.h`, `test_module.h`, `assert.h`,
`test_handler`, `test_session`, `test_printer`, `test_tree`, the `dtest`
umbrella), which does not compile yet and is being ported (B5: its sources
stay, its old `_sa` suites are gone, and its suites are written against the C
face when it is ported); the C++ framework's internals
(`test_tree`, `test_kind`, `test_session`, the report model and the output
headers), which may change under the surface above, including `test_session`'s
status counts, which it now keeps in a tally (13.1).

**Retired** (the owner's ruling of 2026-11-06, register 4.6): the standalone
framework, `test_standalone.h` and `.c`, its defaults header and
`d_assert_standalone`. Nothing runs a `_tests_sa*` suite any more; one still in
the tree is converted (14.2) before its module's tests count.

### 13.1 The counters (the owner's rulings of 2026-11-09 and 11-10)

DTest's counters know nothing of what they count. A tally holds one test
counter per key, and the caller chooses the keys. A test counter is the
generic counter of `c/util/counter` or `cpp/util/counter` plus a step; a
test timer is the generic timer of `c/util/timer` or `cpp/util/timer`.
Neither knows about events: both are used by hand, and events reach them
only through the tie-in modules below.

**C** (`c/modules/test/test_counter.h`, `test_timer.h`):

- `struct dj_test_counter` is `{ counter, step }`: a `struct dj_counter`
  and what one occurrence moves it by. `d_test_counter_init` /
  `_init_with`, `_record` (one occurrence, one step), the five operations
  (`_increment`, `_decrement`, `_compare`, `_merge`, `_reset`), `_value`,
  `_step` and `_as_counter`.
- `struct dj_test_tally` keeps up to `D_TEST_TALLY_CAPACITY` test counters
  by `dj_test_key`, a `uint64_t` the caller chooses: `d_test_tally_init`,
  `_track`, `_find`, `_record`, `_increment`, `_count`, `_overflow`,
  `_reset`, `_merge`, `_clear`. A zero-filled tally is usable.
- `struct dj_test_timer` is `{ timer }` with the timer's operations and
  `d_test_timer_elapsed_ms`.

**C++** (`cpp/modules/test/test_counter.hpp`, C++17 and up;
`test_timer.hpp`, C++11 and up):

- `test_counter<V>` is a `djinterp::counter<V>` plus a step and `record()`;
  `test_tally<V>` keeps one per key type (any type; one with a static
  `name()` lends it to reports), with no capacity, and has `record<Key>()`.
- `DTestTimer<Clock, Duration>` is a `djinterp::timer` plus `elapsed_ms()`.

**Events, through the tie-ins (the owner's ruling of 2026-11-10).** Each
counter and timer family has one event module, separate from everything
else, that couples a handler to a counter or timer and hands the handler
off. The caller binds it wherever handlers are bound; no counter, timer or
DTest runner includes a tie-in.

| Tie-in                                      | What it hands off                                                       |
|---------------------------------------------|-------------------------------------------------------------------------|
| `c/util/counter/counter_event.h`            | a `dj_event_step` moving any counter (`struct dj_counter_ref`)          |
| `c/util/timer/timer_event.h`                | a `dj_event_step` starting, stopping or moving any timer                |
| `cpp/util/counter/counter_event.hpp`        | `counter_handler<Counter>`, for any counter type                        |
| `cpp/util/timer/timer_event.hpp`            | `timer_handler<Timer>`, for any timer type                              |
| `c/modules/test/test_counter_event.h`       | a step recording a tally key; a test counter's `counter_event.h` binding |
| `c/modules/test/test_timer_event.h`         | a start and a stop binding on a test timer                              |
| `cpp/modules/test/test_counter_event.hpp`   | `tally_handler<Key, Tally>`; a test counter's `counter_handler`         |
| `cpp/modules/test/test_timer_event.hpp`     | a start and a stop `timer_handler` on a test timer                      |

The C tie-ins work on every kind of counter or timer through one interface:
`counter.h` and `timer.h` define a table of operations
(`struct dj_counter_vtable`, `struct dj_timer_vtable`) and a reference that
pairs one with a counter or timer (`struct dj_counter_ref`,
`struct dj_timer_ref`); `d_counter_ref`, `d_counter_atomic_ref`,
`d_counter_mutex_ref` and the timer three make them, and the
`d_counter_ref_*` / `d_timer_ref_*` functions use them.

```c
struct dj_test_tally_event failures;

d_test_tally_event_init(&failures,
                        &tally,
                        MY_TESTS_FAILED);
d_event_dispatcher_bind(&dispatcher,
                        KEY_TEST_FAILED,
                        d_test_tally_event_step(&failures),
                        NULL,
                        &id);
```

**Moving from the field form.** Each old field becomes a key the suite
defines, and each operation the tally's:

| Field form (before 2026-11-09)            | Now                                                      |
|-------------------------------------------|----------------------------------------------------------|
| `struct dj_test_counter c = {0};`         | `struct dj_test_tally t = {0};` (or `d_test_tally_init`) |
| `c.tests_total++`                         | `d_test_tally_increment(&t, MY_TESTS_RUN, 1)`            |
| `c.assertions_passed` (read)              | `d_test_tally_count(&t, MY_ASSERTIONS_PASSED)`           |
| `d_test_counter_add(&dest, &src)`         | `d_test_tally_merge(&dest, &src)`                        |
| `d_test_counter_reset(&c)`                | `d_test_tally_reset(&t)`                                 |

`MY_TESTS_RUN` and the like are `dj_test_key` values the suite chooses; a
suite that fires events binds `test_counter_event.h`'s handlers instead of
counting by hand. `d_test_counter_reset` keeps its name, on one test counter. A C-face
suite needs none of this, since its report already counts tests and checks;
the old fields were the standalone tier's.

**Settings, in three layers, each optional.** A build that sets nothing gets
unbounded counters that step by one and a tally of 16 that refuses a 17th
key.

1. *Knobs* (`config/c/modules/test/cfg_test_counter.h`):

   | Knob                          | Default | Meaning                                                 |
   |-------------------------------|---------|---------------------------------------------------------|
   | `D_CFG_TEST_TALLY_CAPACITY`   | 16      | a C tally's storage, 1 to 4096                          |
   | `D_CFG_TEST_TALLY_WHEN_FULL`  | 0       | a full C tally refuses a key (0) or overflows (1)       |
   | `D_CFG_TEST_COUNTER_STEP`     | 1       | how far one occurrence moves a counter; not 0           |
   | `D_CFG_TEST_COUNTER_FLOOR`    | 0       | hold counters at or above zero (1)                      |
   | `D_CFG_TEST_COUNTER_MACHINE`  | 0       | compile the option_env schema of layer 3 (1)            |

2. *Run-time options*: C's `struct dj_test_counter_options { step,
   has_floor }` and `struct dj_test_tally_options { limit, when_full,
   counter }`, whose `_options_init` functions read the knobs, passed to
   `d_test_counter_init_with` and `d_test_tally_init`. A tally's `limit` may
   be lower than its capacity, never higher. C++'s
   `test_counter_options<V> { step, has_floor }` starts at the knobs and is
   taken by `test_counter`'s and `test_tally`'s constructors.
3. *The abstract machine*, opt-in with `D_CFG_TEST_COUNTER_MACHINE`:
   `d_test_counter_env_vars()` is a schema for `c/option/option_env.h`
   (`tally_limit`, `tally_when_full` = `refuse` | `overflow`,
   `counter_step`, `counter_floor`) whose defaults are the knobs, and
   `d_test_tally_options_from_env` reads a frame into tally options, so text,
   a script, the environment or the command line can set them.

Under `D_TEST_TALLY_FULL_OVERFLOW`, a key a full tally cannot track is
counted in the tally's overflow counter (`d_test_tally_overflow`), and merge,
reset and clear carry it; totals stay whole, and the overflow says how much
was not told apart. `test_session`'s own status counts step by one and are
floored whatever the knobs say: they count tests.

---

## 14. Migration guide

Three older forms are in the tree. Each converts to the C face of Section 12;
`scripts/dtest/dtest_convert.py --from FORM` does the mechanical part and marks
what it cannot decide with `DTEST-CONVERT:` comments, which `--report` lists.
A converted file is done when no mark is left and its suite passes.

```
cd <repository root>
# a dry run first: it reports what it would change, and changes nothing
python3 scripts/dtest/dtest_convert.py --from sa --report x_tests_sa*.[ch]
python3 scripts/dtest/dtest_convert.py --from sa --in-place --final \
    x_tests_sa*.[ch]
```

### 14.1 The `dtest_*` bridge (`--from bridge`)

The filesys suite was written against a C ABI named `dtest_*` that no tree
held, with a stand-in of its own. The face is that ABI under the C guide's
names, so the conversion is a rename: `dtest_suite` → `struct dj_test_suite`,
`dtest_create` / `_destroy` → `d_test_suite_new` / `_free`, `dtest_module` /
`_block` / `_test` → `d_test_suite_module` / `_block` / `_test`,
`dtest_assert_expr` → `d_test_suite_check`, `dtest_run` →
`d_test_suite_run`, `DTEST_SHOW_ALL` / `_FAILURES_ONLY` →
`D_TEST_SUITE_SHOW_TESTS` / `_FAILURES`; the stand-in header's include becomes
`test_suite.h`. `dtest_use_console` is marked: set `show` in the options
instead. The filesys suite was converted this way on 2026-11-05 (286 tests);
its stand-in and its C++ runner are retired, and its runner is C.

### 14.2 The standalone suites (`--from sa`)

The standalone tier is retired (13), so a `*_tests_sa*` suite builds only once
converted. They come in two styles, and the converter handles both:

- **Counter style**: `bool f(struct dj_test_counter* _counter)` with
  `d_assert_standalone(cond, name, msg, _counter)`. The parameter becomes
  `struct dj_test_suite* _suite`, each assertion
  `D_TEST_CHECK_MSG(_suite, name, cond, msg)`, and the counter bookkeeping
  (`_counter->tests_total++`, a snapshot of `tests_passed`, a summary
  printer) goes.
- **Tree style**: `struct dj_test_object* f(void)` building a group with
  `d_test_object_new_interior` and leaves with `D_ASSERT_TRUE` / `_FALSE` /
  `_NULL` / `_NOT_NULL` / `_EQUAL` / `_STR_EQUAL`. The function becomes
  `bool f(struct dj_test_suite* _suite)`, each leaf a `D_TEST_CHECK_MSG`, and
  a child group stored into its parent a call over the same suite. Each
  group's variable goes with its declaration, its `if (!group)` guard and its
  return, whatever it is named: a guard left behind would read a pointer
  nothing sets.

In both, a function that checks opens a `d_test_suite_test` of its name, one
that only calls others a `d_test_suite_block`; the tier's own progress lines
(`--- Testing X ---`, `[PASS]` / `[FAIL]`, `[SECTION]`) go, since the report
says the same, and the flag they printed becomes what the section returns. A
local the conversion leaves unread goes with its assignments; one set from a
call keeps the call as a `(void)` statement. Diagnostic prints (a detected
value, say) stay.

`--final` then gives the suite its final names: `d_tests_sa_` becomes
`d_tests_`, and `x_tests_sa*.{c,h}` become `x_tests*.{c,h}`, in the code, the
includes, the guards and the banners. A file whose new name a C-face suite
already has is left in place and reported. What is left by hand:

- **Merging into a C-face suite.** Where the module has one, the converted
  header's body goes into its header (lane B put it in a third section,
  outside spec mode, with the entry points declared in the spec section, where
  the runner sees them), and a function name a register test already has
  takes a suffix (`_cases`).
- **The runner and the leaf.** The runner of 12.3 creates the suite, adds the
  spec if there is one, and calls each entry point: a function no other one
  calls. A module without a spec opens its module in the runner
  (`d_test_suite_module`). The leaf is 12.4's.
- **What the old tier hid.** It compiled without most warnings and ran no
  check of its own, so expect stale API names, a test that checks nothing
  (the face counts it as failed), and placeholders. A section that only prints
  TODO is not a test: drop it, with what only it fed.

Lane B converted its fifteen suites this way on 2026-11-07; `docs/` has no
per-file record, but each suite's runner lists its entry points.

### 14.3 The programs in `tests/unintegrated` (`--from program`)

Each `t_*.c` there is a program that prints its failures and returns 0 when
none failed. `--from program` renames its `main` and adds a spec test that
requires the program's result to be 0, and prints the spec row to paste into
the suite header. That integrates the program as one test without touching its
checks; split it into tests of its own when its module's suite is written.

### 14.4 C++ suites

A C++ suite in the spec idiom of Sections 2 to 7 needs no change. One that used
the face-neutral option setters may drop the `#if` (Section 9). A C++ suite
that must also run below C++17 lists its test functions in a C spec table
(12.2) — they are `bool ()` already — and gets a second runner over
`d_test_run_modules`.

### 14.5 The `dj_tests_section` tables (`--from section`)

Lanes F and G wrote suites as tables of `struct dj_tests_section { name,
run }` (`tests/.../modules/parse/tests_section.h`) walked by a runner of
their own. `--from section` turns each table into the C face's spec form in
place:

- `struct dj_tests_section X[] = { { "name", fn }, ... }` becomes
  `struct dj_test_case_spec X[]` with `{ "name", "", fn }` rows. The
  functions are `bool (void)` in both, so they are not touched.
- The table keeps its name, its linkage and its `#if`s: rows built under a
  language level or a configuration move with their conditions.
- Beside each table goes a block spec, `X_block`, named after the table, and
  an `extern` for each in the header that declared the table.
- `tests_section.h`'s include becomes `test_suite.h`'s.
- Each table gets one `DTEST-CONVERT:` mark asking for its rows' descriptors
  (3.5), since the tables have none, and each block one for its own.
- A runner over the old tables walks `.run` and `.name`. It is marked, not
  converted. The converter prints, on standard error, a runner over every
  non-static block it converted: one module of those blocks, run by
  `d_test_run_modules`. Merge it with the suite's own setup and fill in
  `MODULE` and `TITLE`.

C++ tables (`cli_tests.cpp`, say) convert the same way and then compile
against the C face as C++, which is the form for a C++ suite below C++17
(14.4).

```
python3 scripts/dtest/dtest_convert.py --from section --report \
    tests/djinterp/c/modules/parse/*.c
python3 scripts/dtest/dtest_convert.py --from section --in-place \
    tests/djinterp/c/modules/parse/*.[ch]
```
