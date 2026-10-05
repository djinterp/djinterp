# parse / parsegen foundation — steps 1 through 6

## Install

Unzip at the **repository root** (the directory containing `inc/`, `src/`,
`tests/` and `build/`). Every path in the archive is relative to that root.

```bash
cd <repo-root>
unzip -o vparse.zip
build/cmake/config/testing/djinterp/parsegen/foundation/run_foundation_tests.sh
```

It builds and runs two suites against the same library, and expects
`passed: 18   failed: 0` from the C++ tests and `passed: 5   failed: 0` from the
C tests. Add `--matrix` to run both under every configuration knob. The consolidated vparse builds as
`parsegen/vparse/vparse-AGENTS.md` describes, expecting `passed: 14   failed: 0`.

The archive carries three things: this foundation, the consolidated vparse, and
five framework files with fixes or registrations applied. **Eight existing files
are replaced:**

| file | why |
|---|---|
| `inc/djinterp/djinterp.hpp` | `NS_DJINTERP` fix; the `djinterp::functional` alias |
| `inc/djinterp/env/env.h` | `env_c_lib.h` include order |
| `inc/djinterp/parse/parser/parser.hpp` | `parser_expr` return-type deduction |
| `inc/djinterp/config/dconfig.h` | registers `cfg_parse.h` and `cfg_parsegen.h` |
| `inc/djinterp/config/cfg_testing.h` | the parse machine trace's test-build default |
| `inc/djinterp/parsegen/parsegen.hpp` | the C / C++ umbrella split |
| `inc/djinterp/parsegen/vparse/machine.hpp` | consolidated vparse, on the real root |
| `inc/djinterp/parsegen/vparse/peg.hpp` | consolidated vparse |

If your copies of the first five have changed since the snapshot this was built
against, merge the changes rather than overwriting.

**Delete by hand, since a zip cannot remove files:** `inc/djinterp/parse/vparse/`.
Its six headers are an older generation of vparse that nothing references;
`parsegen/vparse/` supersedes them.

**If you unzipped an earlier version of this archive,** also delete the C
layer's old locations: this version moves it into `c/` (see Layout), and a
build that globs `src/djinterp/parse/*.c` would otherwise compile both copies
and fail on duplicate symbols. None of these files is in the original tree, so
the command removes nothing of yours:

```bash
cd <repo-root>
rm -f inc/djinterp/parse/{charset,diagnostic,machine,pool,program,storage}.h \
      src/djinterp/parse/{charset,diagnostic,machine,pool,program,storage}.c \
      inc/djinterp/parsegen/{parsegen,feature,registry,grammar,analysis}.h \
      src/djinterp/parsegen/{feature,registry,grammar,analysis}.c
```

### CMake

`build/cmake/config/testing/djinterp/parsegen/foundation/CMakeLists.txt` is a
plain-CMake leaf. Wire it with `add_subdirectory()` wherever your other test
leaves are added. It builds the library and both test executables, and registers both with
CTest. It does not use `djinterp_add_test_executable`, because that
helper's source was not available; swapping to it is mechanical.

### Framework fixes, and one open issue

Building against your real headers, rather than stand-ins, surfaced four
framework issues. The three files above fix them, so nothing here needs a build
flag or a force-include to compile:

- **`D_FRAMEWORK_NAME` was used but never defined.** The C root defines
  `D_FRAMEWORK_NAME`; the C++ root's `NS_DJINTERP` expanded through the other
  name, so every `NS_DJINTERP` opened a namespace literally called
  `D_FRAMEWORK_NAME`. It now uses `D_FRAMEWORK_NAME`.
- **`functional` was never declared.** `parse.hpp`, `core/functional` and
  `core/event` spell the functional API `functional::` (32 uses in 7 files), and
  only the old sandbox stand-in declared the alias. `djinterp.hpp` now declares
  `djinterp::functional` — inside `djinterp`, as the stand-in did, so it adds no
  name to the global namespace.
- **`env.h` included `c/env_c_lib.h` too early,** before the headers whose macros
  its `#if` tests read. It now comes after the detection headers and before
  `env_build.h`, the order `env.h`'s own banner documents.
- **`parser_expr` used a trailing `decltype` return** that g++ evaluates while
  the derived class is still incomplete, so no `parser<R, E>` instantiated under
  g++. It now deduces at the point of use — your own fix from the corrected copy,
  which had not carried over when `parser.hpp` moved to the real root.

**Still open, by your choice:** the C-side attribute macros in
`env/c/env_attributes.h` select C23 `[[...]]` syntax under `-std=c11`, which
`-Wpedantic` reports. The script leaves `-Wpedantic` off by default;
`PEDANTIC=1` turns it on. A fix ships *beside* this archive rather than in it,
so adopting it stays a separate decision: `env_attributes.h` gates the C23 probe
on `__STDC_VERSION__ > 201710L` at all six sites. That takes a pedantic C11 or
C17 build of this foundation from 62 warnings to 0, leaves every attribute in
effect (a discarded `D_NODISCARD` result and a `D_DEPRECATED` call still warn),
and changes nothing in C++.

## Layout

Every subsystem X owns `X/`: its C++ lives at `X/`, its C in `X/c/`, and `X/c/`
mirrors X's layout beneath it. The framework root follows the same rule, and its
C half, `c/`, is the C foundation -- which is why an optional subsystem never
puts anything there: a C user who takes djinterp without parse or parsegen must
not carry them. Preprocessor-only code (`env/`, `config/`) is language-neutral
and exempt.

| | C | C++ |
|---|---|---|
| parse | `inc/djinterp/parse/c/*.h`, `src/djinterp/parse/c/*.c` | `inc/djinterp/parse/*.hpp` |
| parsegen | `inc/djinterp/parsegen/c/*.h`, `src/djinterp/parsegen/c/*.c` | `inc/djinterp/parsegen/*.hpp` |
| tests | `tests/djinterp/parse/c/`, `tests/djinterp/parsegen/c/` | `tests/djinterp/parse/`, `tests/djinterp/parsegen/` |

A test's directory follows the language it is *written* in, not the API it
calls: the C++ suite exercises the C API through C++, so it stays outside `c/`.

A C++ face reaches its C layer through `./c/` -- `diagnostic.hpp` includes
`./c/diagnostic.h` -- the way `djinterp.hpp` reaches `c/djinterp.h`. And a bare
`c/` directly under a subsystem means only that subsystem's C half. A language
as a *subject* sits under a role directory instead: `parse/parsers/c/` (a parser
of C) already does, and a C export target would go in `parsegen/backends/c/`,
never `parsegen/c/`.

Config stays at the subframework level (`config/parse/`, `config/parsegen/`),
since one knob file configures both halves. Both files are registered in
`dconfig.h`, and the one knob that differs in a test build, the machine trace,
takes its test default from `cfg_testing.h`, as the config README requires. It
defers to `D_CFG_PARSE_ALL` there, so relocating it changed nothing except one
case: a testing build that sets `D_CFG_NO_TESTING_PRESET` now gets the trace
off, as suppressing the preset is documented to do.

Applying the rule to existing code is deferred, but needs no further decision:
`c/test/` moves to `test/c/`; the C headers in `parse/parsers/{bnf,abnf,ebnf}/`
move to `parse/c/parsers/...`; `jit/` becomes `jit/c/` if the rule is held
strictly; and `env/c/` and `env/cpp/`, which hold detection *of* C and C++
features, move under a role directory such as `env/lang/`, so that `c/` keeps
one meaning.

## Files

| path | what |
|---|---|
| `inc/djinterp/config/parse/cfg_parse.h` | every `D_CFG_PARSE_*` knob and its `D_INTERNAL_PARSE_*` derived value |
| `inc/djinterp/parse/c/diagnostic.h` | C: `d_parse_span`, `d_parse_diagnostic`, `d_parse_diag_sink` |
| `inc/djinterp/parse/c/machine.h` | C: `d_parse_machine`, `d_parse_voperator`, `d_parse_op_set` |
| `inc/djinterp/parse/c/charset.h` | C: `d_parse_charset`, the canonical 256-bit class |
| `inc/djinterp/parse/c/pool.h` | C: `d_parse_pool`, the operand intern pool |
| `inc/djinterp/parse/c/program.h` | C: `d_parse_instr`, `d_parse_program`, verify/hash/transport |
| `inc/djinterp/parse/c/storage.h` | C: `d_parse_grow`, the one growth policy |
| `inc/djinterp/parse/substrate.hpp` | C++: the substrate's one guarded `NS_PARSE` |
| `tests/djinterp/parse/c/parse_c_tests.{h,c}` | C tests: diagnostics, machine, program |
| `tests/djinterp/parsegen/c/parsegen_c_tests.{h,c}` | C tests: grammar, analysis and routing |
| `.../parsegen/foundation/foundation_c_tests_runner.c` | the C suite's entry point |
| `inc/djinterp/parsegen/vparse/` | the consolidated vparse; see its `vparse-AGENTS.md` |
| `inc/djinterp/parsegen/c/feature.h` | C: `d_parsegen_features`, the capability vocabulary |
| `inc/djinterp/parsegen/c/registry.h` | C: `d_parsegen_stage`, `d_parsegen_registry`, selection |
| `inc/djinterp/parsegen/c/grammar.h` | C: `d_parsegen_node`, `d_parsegen_grammar`, the neutral model |
| `inc/djinterp/parsegen/c/analysis.h` | C: `d_parsegen_facts` — nullability, first sets, left recursion |
| `inc/djinterp/config/parsegen/cfg_parsegen.h` | parsegen's own knobs and derived values |
| `inc/djinterp/parse/diagnostic.hpp` | C++: `span`, `diagnostics`, `fixed_diagnostics<N,M>`, views, iteration |
| `inc/djinterp/parse/machine.hpp` | C++: `machine`, `op_set`, `fixed_op_set<N>`, the `def()` family |
| `inc/djinterp/parse/charset.hpp` | C++: `charset`, constexpr membership |
| `inc/djinterp/parse/pool.hpp` | C++: `pool`, `fixed_pool<B,E>` |
| `inc/djinterp/parse/program.hpp` | C++: `instr` (alias), `program`, `fixed_program<N,B,E>` |
| `inc/djinterp/parsegen/registry.hpp` | C++: `feature`, `feature_set`, `stage`, `registry` |
| `inc/djinterp/parsegen/grammar.hpp` | C++: `node` (alias), `grammar`, `fixed_grammar<…>` |
| `inc/djinterp/parsegen/analysis.hpp` | C++: `facts`, `fixed_facts<N,R>`, the named queries |
| `inc/djinterp/parsegen/c/parsegen.h` | C umbrella: subsystem keyword, generator diagnostic domains |
| `inc/djinterp/parsegen/parsegen.hpp` | C++ face: `NS_PARSEGEN` and nothing else |
| `src/djinterp/parse/c/{diagnostic,machine,charset,pool,program,storage}.c` | definitions |
| `src/djinterp/parsegen/c/{feature,registry,grammar,analysis}.c` | definitions |
| `tests/djinterp/parse/parse_substrate_tests.{hpp,cpp}` | 3 sections |
| `tests/djinterp/parse/parse_program_tests.cpp` | 3 sections |
| `tests/djinterp/parsegen/parsegen_registry_tests.cpp` | 3 sections |
| `tests/djinterp/parsegen/parsegen_grammar_tests.cpp` | 5 sections |
| `tests/djinterp/parsegen/parsegen_analysis_tests.cpp` | 4 sections |
| `build/cmake/config/testing/djinterp/parsegen/foundation/` | runner, CMake leaf, build script |

## The namespace rule, as applied

`djinterp::parse` — usable without generating anything: the carrier, the
execution substrate, the diagnostic channel. `djinterp::parsegen` — flat,
a sibling of `parse`, everything that *produces* a parser.

The machine landed in `parse` because a hand-authored program run through
it needs no generator, and because the dependency has to point that way:
`parsegen` depends on `parse`, never the reverse. C prefixes mirror the
namespaces (`d_parse_*`, `d_parsegen_*`).

## What step 1 actually removed

`input`, `sp`, `input_type`, `result_type`, and `std::string error`.

The first three made every family a byte-at-a-time recognizer over one
contiguous string — a token-stream family had nowhere to put its cursor and
a generator paid for one it couldn't use. Both now live behind `ext`, where
all other family-private state already was.

`error` was a one-slot diagnostics facility, which is why step 2 subsumes
it. `d_parse_machine_fail` halts *and* reports into the shared sink, so a
failure from any stage lands in the same ordered report.

## Two things that changed from the plan

**`std::function` → function pointer + context.** The old `voperator` was
`std::function<void(machine&)>`: an allocation per stateful handler and an
indirect call through a type-erased wrapper, and not expressible in C. It is
now `void (*)(d_parse_machine*, void*)`.

Ergonomics are preserved by generating the trampoline as a template. The
C++ registration

```cpp
ops.def(OP_TICK, "TICK", [](machine& m){ m.extension<counter>()->ticks++; });
```

compiles to, verbatim, what a hand-written C operator compiles to:

```
movq 24(%rdi), %rax
addl $1, (%rax)
ret
```

Three registration shapes: `def` (captureless callable, no context stored),
`def_state<&fn>` (free function plus family state), `def_object` (a functor
the caller owns), plus `def_raw` for the plain C form.

**`unordered_map` → dense table.** An opcode space is family-private and
dense from zero by construction, so `find` is a bounds check and a load.
This is the interpreter-baseline work from the earlier plan, done here
because the substrate was being rewritten anyway.

## Seams left open deliberately

- **`d_parse_op_set_covers`** — asks whether one registry implements every
  opcode another defines. This is the gate for a second reading of one
  opcode space (interpreter vs. JIT vs. source emitter); wiring it into a
  test turns a silently-lost backend into a build failure.
- **Diagnostic domains** partition the code space the way `op_set`
  partitions an opcode space. `parse` owns below 64, `parsegen` numbers its
  stages from 64, applications from 1024. Adding a frontend means claiming a
  domain, not touching anything existing.
- **`d_parse_diagnostic` is a 32-byte POD holding an arena offset**, not a
  pointer — so a sink is memcpy-able, hashable and serializable whole, which
  is what the later `(grammar, token_set, profile) → program` caching needs.
- **`D_PARSE_DIAG_FLAG_CONTINUATION`** for "error here / note: declared
  there" chains, which analysis will want as soon as it reports a left-
  recursive cycle.
- **`reserved` fields** in the diagnostic and the registry, so a field can
  be added without moving anything.

## Migration for existing vparse code

1. `parsegen/vparse/machine.hpp` is superseded by `parse/machine.hpp`;
   delete it. `djinterp::parsegen::machine` → `djinterp::parse::machine`.
2. `op_set::def(code, name, fn)` → same call, but the lambda must capture
   nothing (use `def_object` or `def_state` if it does).
3. `ops.find(code)` returns `const op*`; handlers now take
   `(d_parse_machine*, void*)` at the ABI and `machine&` through `def`.
4. A family's private state moves from the old `machine` fields into its own
   struct behind `ext` — `peg` already did this with `parse_state<char>`.
5. `m.error = "..."` → `m.fail(domain, code, span, "...")`.
6. Drivers call `ops.dispatch(m, code)`, which charges the step and reports
   an unregistered opcode, replacing the hand-rolled fetch/find/call.

## Step 3: the IR is a POD plus side tables

```c
struct d_parse_instr { uint16_t op; uint16_t flags; int32_t a; int32_t b; };
```

Twelve bytes, no padding, no pointers. Measured: `instr=12 charset=32
pool_entry=8 op_set=24 machine=40 diagnostic=32 program=72`.

The `std::string set` that used to sit inside an instruction blocked four
things at once. All four now work and each has a test:

- **hashing** — `program::digest()` is a cache key over family, entry,
  instructions and pool, deliberately *not* over the memory image, so it
  ignores capacity, ownership flags and padding. Verifying a program sets
  annotation flags and does not change its digest.
- **transport** — `write`/`read` in a defined little-endian layout, not a
  memory image. A program written on one machine reads on another. Read
  treats the buffer as hostile: counts checked against bytes present, every
  pool entry checked to lie inside the blob it indexes, blobs re-interned
  rather than copied wholesale.
- **C handoff** — no fix-up pass at any boundary.
- **compile time** — `make_instr` is constexpr and the assertion is a
  constant evaluation rather than a trait, so it proves the property instead
  of approximating it.

### Classes resolve to bits, not text

A class operand is a 32-byte bitmap, interned. That makes membership one
shifted load, and it makes canonicalization automatic: `[0-9]` and
`[0123456789]` produce identical bytes and therefore share one pool entry.
Rendering folds runs back into ranges, so a disassembly is comparable across
builds.

It also carries what the optimizer will ask for later. `contiguous()` answers
"can this be two compares instead of a table load" — the biggest per-class
codegen win. `count()` gives density. `disjoint()` is the first-set condition
an ordered choice must satisfy before it may become a dispatch.

### The family identifier makes invariant 2 checkable

An `op_set` now records *which* private opcode space it reads, and a program
records which one it is written in. `d_parse_program_verify` compares them.
Invariant 2 was a rule people follow; it is now one the build enforces —
which starts mattering the moment a second family exists.

Two registries over one space (an interpreter and a code generator) share a
family and differ only in what their handlers do. That is the JIT arrangement
from earlier, expressible now.

### Verification annotates

`verify` already walks every branch, so it marks each landing site with
`D_PARSE_INSTR_FLAG_TARGET`. A code generator needs exactly that to know
where to place labels and would otherwise scan for it. The disassembler shows
it as a `:`:

```
   0  JUMP       -> 3
   1  SET        [0-9A-Fa-f]
   2  ANY
   3: HALT
```

### One disassembler, any family

Mnemonics come from the registry, operand meanings from a shape table the
family declares (`shape(operand::target, operand::name)`). So there is no
`fmt()` per family — which matters because the second copy is always the one
that drifts.

The shape table is also what lets *this* level verify operands without
knowing a single opcode: it checks branch targets are in range and pool
indices resolve, for any family, forever.

### One API gap closed while here

`def` rejected a plain free function (not an empty type), which is the shape
most C++ handlers take. Added `def<&fn>(code, name)` — the function is a
template argument, so the shim is a direct call.

## Step 4, and a correction to how it was framed

I filed this as "`registry<T>` lifted from `op_set`". Having written three
containers, that was half wrong. `op_set` is keyed by a **dense integer**; the
stage registries the pipeline needs are keyed by **name** and queried by
**capability**. Those are not the same structure, and merging them would have
made the dispatch path worse to serve a resemblance. So step 4 is two things:
the duplication that was actually there, and the registry that was actually
needed.

### What genuinely repeated: the growth policy

Four containers held a fixed-or-owned array and each had written the same
twelve lines — refuse if not ours, double until it fits, realloc, zero the
tail. That is now `d_parse_grow` in `parse/c/storage.h`, and `op_set`, both pool
arrays, the program, and the registry all call it.

**What is deliberately not extracted: the containers.** A generic buffer
carrying an element size turns every access into a runtime multiply against a
loaded stride, and `d_parse_op_set_find` is the innermost thing in the system
— it has to stay `&set->ops[code]`. The containers keep typed fields and share
only the algorithm, which is where the duplication was.

Zeroing the tail became part of the contract rather than one caller's
afterthought. `op_set` depended on it (an undefined opcode must read as a hole)
and nobody else had to think about it again.

### What was actually needed: capability-keyed stage selection

`d_parsegen_registry` holds frontends, passes, families and backends in one
table. Each stage declares three masks — `provides`, `needs`, `rejects` — and
selection is:

```c
(features & stage->rejects) == 0  &&  (features & stage->needs) == stage->needs
```

Two instructions. The point is what it makes cheap: **adding the LR family is a
row of declarations, not an edit to whatever was choosing between the existing
families.** The test asserts exactly that — it selects, fails, registers `lr`,
selects again, and nothing in between was touched. This is the `op_set` idea one
level up.

Invariant 4 ("no left recursion") stops being a comment and becomes
`.rejects = D_PARSEGEN_LEFT_RECURSION` on the PEG family.

### The vocabulary, and the bit that matters

`ORDERED_CHOICE` and `UNORDERED_CHOICE` are separate bits and **neither is a
default**. PEG's `|` commits; BNF's does not. Collapsing them would silently
change which language a grammar denotes and show up as a wrong parse rather
than an error. A pass may lower unordered to ordered where first sets are
disjoint — `charset::disjoint` from step 3 is that proof obligation, already
available.

### Diagnostics are the product

A failed selection returning NULL is not an answer. This is:

```
error: no stage accepts a grammar using UNORDERED_CHOICE|LEFT_RECURSION
  note: peg: rejects UNORDERED_CHOICE|LEFT_RECURSION
  note: lr: needs TOKEN_STREAM
```

Notes carry `D_PARSE_DIAG_FLAG_CONTINUATION` from step 2, which is what the
flag was for. A failed `find` lists what *is* registered, since the next thing
the caller needs is the spelling it should have used.

### One bug the knob matrix found

Two configurations failed with `D_CFG_PARSE_DIAG_FORMAT=0`, because
`registry.c` and `program.c` had `#if`-guarded pairs of messages — a detailed
one when the sink's printf entry point existed and a vague one otherwise. But
that knob controls whether the **sink** offers a varargs emitter; it should not
change what a stage can say about itself. Both now compose locally with
`snprintf` and emit once. Six preprocessor branches gone and the diagnostics
are identical in every configuration.

## Step 5: the neutral grammar

The old `ruleset` had a structural ceiling worth naming: a rule as a list of
alternatives each of which is a list of terms is **two levels**, so it cannot
express `a (b | c)* d`. EBNF and ABNF both have grouping. So a rule body is an
expression, and an expression has children.

By the same reasoning as step 3, it is a flat node arena with index references
rather than a pointer graph — 28 bytes per node, no pointers, so it copies,
hashes and serialises for the same reasons the instruction stream does.

### Ordered and unordered choice are different node kinds

Not one kind with a flag, and emphatically not one kind with a default. PEG's
`/` commits to the first alternative that matches; BNF's `|` does not.
Collapsing them changes which language a grammar denotes, and the damage
surfaces as a wrong parse rather than an error.

Having no neutral `choice` node means a frontend **cannot build one without
saying which it meant**. The C++ builder has `ordered()` and `unordered()` and
no `choice()`. That is the enforcement; a comment would not have been.

The canonical rendering keeps the distinction visible: `/` for ordered, `|` for
unordered.

### Repetition carries bounds

`{min, max}`, not a star flag. `?` is {0,1}, `*` is {0,∞}, `+` is {1,∞}, ABNF's
`3*5DIGIT` is {3,5} — one representation for all of them, so adding ABNF costs
a frontend and no change below. Anything that is not one of the three classic
forms sets `BOUNDED_REPEAT`, so a family that can only loop knows it has been
handed a count. A reversed range is refused, not silently normalised.

### Capabilities accumulate as you build

Every construction folds in what its kind implies, from the same descriptor
table that drives verification and rendering. A frontend **cannot emit a
construct and forget to declare it** — a grammar containing an unordered choice
reports `UNORDERED_CHOICE` whether its frontend meant to or not. That removes
the entire class of "the frontend said PEG but emitted a CFG" bug.

One deliberate exception: `LEFT_RECURSION` is **derived**, not syntactic. The
indirect case needs nullability. Building a self-referencing rule declares
nothing and resolution does not either; analysis fills it in. The test asserts
that non-obvious boundary.

### It feeds the registry directly

`grammar.uses()` is exactly what `registry.select()` matches a family against.
The test builds a grammar, selects a family successfully, adds a host action,
and watches the same family stop accepting — with `HOST_ACTION` named in the
diagnostic.

```
Expr = Term ( ( '+' / '-' ) Term )* ;
Term = [0-9]{1,3} ;
Sign = '+' | '-' ;
uses: ORDERED_CHOICE|UNORDERED_CHOICE|BOUNDED_REPEAT|CHARACTER_CLASS
```

### Two bugs the knob matrix found

**A missing fixed-storage form.** Every other container had one; the grammar
did not, so `-DD_CFG_PARSE_ALL=0` left no way to build a grammar at all. Now
`fixed_grammar<Nodes, Rules, PoolBytes, PoolEntries>`.

**A config-doctrine violation.** `grammar.c` gated its heap form on
`D_INTERNAL_PARSE_HEAP`, but a grammar carries an operand pool, so it also
needs the *pool's* gate. With `POOL_HEAP=0` and the aggregate still on, it
referenced a function that did not exist. That conjunction is exactly what the
"resolution lives in `cfg_*.h`" rule exists to prevent being re-derived
per-module — so parsegen now has its own `cfg_parsegen.h` resolving
`D_INTERNAL_PARSEGEN_GRAMMAR_HEAP` once.

## Verified against the real config layer

Everything before this point was compiled against sandbox stand-ins for
`cfg_common.h`, `c/djinterp.h` and `djinterp.hpp`. Checking every macro the
deliverable uses against the real headers found one genuine bug, which the
stand-in had hidden.

**The real `D_CFG_IS_BOOL` token-pastes the fully-expanded knob** against
`D_INTERNAL_CFG_LIT1_` / `_LIT0_`, so it accepts only a knob that expands to the
single token `0` or `1` (or empty). `D_CFG_PARSE_MACHINE_TRACE` defaulted to
`D_CFG_NORM(D_CFG_TESTING)`, which expands to `(0 + 0)`, and pasting an
identifier against `(` is a hard preprocessing error:

```
cfg_common.h:112: error: pasting "D_INTERNAL_CFG_LIT1_" and "(" does not give
a valid preprocessing token
```

That fired in the **default** configuration, so every translation unit
including any parse header would have failed on first compile in the real
tree. Fixed with the framework's own idiom — test with `#if`, then define a
literal. The config layer in the sandbox is now the real one, and the full
suite and knob matrix pass against it, including the empty-flag case
(`-DD_CFG_PARSE_MACHINE_TRACE=`) the config README requires to read as off.

**Update, at packaging time:** the real `c/djinterp.h` and `djinterp.hpp` turned
out to assemble in the sandbox once the env include order is corrected, so the
packaged foundation is verified against **your real root headers with no
stand-ins at all** — dropped into a fresh copy of the tree with one `unzip`,
built by the included script, and run. That is what surfaced the framework
issues described under Install. The guarded `NS_PARSE` / `D_KEYWORD_PARSE`
spellings were also confirmed token-identical to `parse.hpp`'s, which is what
makes their redefinition benign.

## Framework fixes: verification

Every header in the tree was compiled standalone, before and after the
`NS_DJINTERP` and include-order fixes: **0 regressions, 102 headers fixed** —
almost all C++ (core/container 32, core/functional 18, core/util 14, core/text 11,
and others), broken because `NS_DJINTERP` opened the wrong namespace. The one
header that changed from pass to fail, `env/c/env_c_lib.h` compiled on its own,
also fails in the original tree: it is a fragment reachable only through
`env.h`, which passes before and after.

That sweep placed `env_c_lib.h` after `env_build.h`; the final placement is just
before it, as `env.h`'s banner documents. The two headers are independent in
both directions — neither uses a macro the other defines — so the result carries
over.

With the `functional` alias declared, `parse.hpp` compiles against the real root
for the first time, as do `parser/combinators.hpp`, `core/functional/recursion.hpp`
and `core/functional/polynomial.hpp`. Three other `functional::` users still fail,
each for a reason unrelated to the alias that predates these changes:
`parse/functional_cli.hpp` (`NS_CLI` undefined), `core/functional/free.hpp`
(`djinterp::free` not found as a template), and `core/event/event_registry.hpp`
(an `#error` requiring `djinterp.h` first).

`NS_EXCEPTION` has the first bug's twin — `D_KEYWORD_EXCEPTION` is defined
nowhere — but it has no users and no evident intended name, so it is untouched.

## Diagnostic codes

Every parsegen diagnostic now carries a real code. Each stage owns its code
space, declared beside it, as a family owns its opcode space:
`d_parsegen_grammar_diag` (7 codes), `d_parsegen_analysis_diag` (6),
`d_parsegen_registry_diag` (3). Codes are append-only.

The registry has its own domain, `D_PARSEGEN_DIAG_DOMAIN_REGISTRY`. It had been
reporting under FRONTEND for `find` and FAMILY for `select`, whatever kind of
stage was asked for.

**Identity is the pair, never the code alone.** Codes are numbered from zero
within each domain, so code 3 in one domain and code 3 in another are unrelated
conditions. `d_parse_diag_find(sink, domain, code, from)` (C) and
`diagnostic_view::is(domain, code)` / `diagnostics::contains(domain, code)`
(C++) make comparing the pair the easy path.

Tests now identify diagnostics by that pair. Text is checked only where the
message carries information the test is about — which rule, which capability,
which candidate — never for wording.

## NS_PARSE umbrella

`parse/substrate.hpp` holds the substrate's one guarded `NS_PARSE`, replacing
copies in `diagnostic.hpp` and `charset.hpp`. It mirrors `parsegen.hpp`: an
umbrella's C++ face holds the namespace macro and nothing else. The spelling is
token-identical to `parse.hpp`'s, so the two can be included in either order.

## Step 6: analysis, and the loop it closes

Analysis derives what nobody wrote down: nullability and first sets per node
and per rule, reachability, productivity, and left recursion.

### Left recursion is a fact, not an error

This is the piece that makes the whole arrangement pay. Analysis **discovers**
left recursion and sets `LEFT_RECURSION` on the grammar. It does not complain
about it — it emits a note. Whether it matters is the family's business,
answered by a registry query.

The routing test is the end-to-end proof. The same grammar, the same query:

```
before analysis  →  peg     (syntax alone looks fine)
analysis runs    →  LEFT_RECURSION discovered
after analysis   →  lr      (peg now rejects it)
```

Nothing in between was told either family exists. Invariant 4 of the old
design ("no left recursion — a rule referencing itself leftmost will not
terminate") has gone from a warning in a comment to a capability the system
routes on.

```
Expr = Expr '+' Term / Term ;
Term = [0-9]+ ;
Unused = 'x' ;

syntax says:   ORDERED_CHOICE|CHARACTER_CLASS
analysis adds: LEFT_RECURSION
so it reports: ORDERED_CHOICE|LEFT_RECURSION|CHARACTER_CLASS

note: rule 'Expr' is directly left-recursive
warning: rule 'Unused' is never reached from the start symbol
```

Health findings are graded by who can act on them: a rule that can never match
is an **error** (no family could run it); an unreachable rule is a **warning**
(dead weight a pass may drop); left recursion is a **note** (a fact for
selection).

### First sets are sound over-approximations

Never smaller than the truth, which is what makes them safe to optimise
against — skipping an alternative whose first set excludes the next symbol can
never skip one that would have matched. Three transfer functions have their
reasoning recorded in the source because each is a place a careless reading
gets it wrong:

- A **predicate** consumes nothing, so it is nullable *and* contributes no
  symbol. `!'_' [a-z]` has a first set of exactly the 26 letters — the test
  asserts that, because giving `&A` the first set of `A` would be sound but
  needlessly loose.
- A **sequence** keeps absorbing first sets while its members are nullable,
  which is the only reason nullability has to be computed alongside rather
  than after.
- A **repeat** is nullable when its minimum is zero, whatever the child says.

### The named queries carry the rule that gets forgotten

`disjoint()` answers whether a choice can be told apart by one symbol — the
precondition both for lowering an unordered choice to an ordered one and for
compiling either as a jump table. It includes the nullability check, which is
the half a caller working from first sets alone would miss: **a nullable
alternative matches whatever comes next**, so nothing can be dispatched past
it however tidy the first sets look. One place, not every optimiser.

### Left recursion behind a nullable prefix

`N = ' '? N 'z'` is left-recursive, and a naive leftmost-child check misses it.
The left-call walk keeps going through members while what precedes them can
match empty, and it propagates through predicates (`A = &A x` recurses too).
Both are tested.

### One soundness bug caught in review

The left-call traversal used a fixed `int32_t stack[64]` and silently dropped
pushes when full. A choice with more than 64 alternatives would have pushed
past it and **under-reported** left recursion — the one direction that is not
safe to be wrong in. The stack is now sized by node count and lives in the
facts' scratch, alongside `work` and `mark`, so a no-allocator build supplies
it like everything else.

`node_facts` is 36 bytes, `rule_facts` 40.

## Umbrella split

`parsegen.h` carries the keyword and the domain enum; `parsegen.hpp` adds
`NS_PARSEGEN` and stops. A namespace macro is the only part of an umbrella
that cannot be written in C, so it is the only thing above the `.h`. Verified
from a pure C translation unit (`gcc -std=c11 -Wpedantic`) and a C++ one.

The same shape applies to any future umbrella: if a header's C++-only content
is one macro, that macro is the `.hpp` and everything else is the `.h`.

## Two style calls worth confirming

**No `typedef` on structs or enums.** The style guide says don't hide
whether a type is a struct; `jit.h` typedefs everything
(`typedef struct d_jit_buffer {...} d_jit_buffer;`). I followed the guide.
The cost is nil on the C++ side, which sees the tag names directly.

**One config file for the subframework** rather than `cfg_diagnostic.h` +
`cfg_machine.h`, per the "don't over-fragment; group config per
subframework" gotcha in the config README.

## Build by hand

What the script does, for reference:

```bash
ROOT=<repo-root>
FIX="-include djinterp/env/env_os.h"
INC="-I$ROOT/inc -I$ROOT/tests/djinterp/parse -I$ROOT/tests/djinterp/parsegen"

for c in $ROOT/src/djinterp/parse/c/*.c $ROOT/src/djinterp/parsegen/c/*.c; do
  gcc -std=c11 -O2 -Wall -Wextra -Wpedantic -DD_TESTING=1 $FIX $INC \
      -c "$c" -o "$(basename "${c%.c}").o"
done
ar rcs libdjinterp_parse_foundation.a *.o

g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -DD_TESTING=1 $FIX $INC \
    $ROOT/tests/djinterp/parse/*.cpp $ROOT/tests/djinterp/parsegen/*.cpp \
    $ROOT/build/cmake/config/testing/djinterp/parsegen/foundation/foundation_tests_runner.cpp \
    libdjinterp_parse_foundation.a -o foundation_tests && ./foundation_tests
```

`D_TESTING` must be the same for the library and the tests: it turns on the
machine's trace hook, which changes the layout of `struct d_parse_machine`.

## Open: a grammar has no fixed-storage initializer in C

Writing the C tests found one gap in the C API. Facts, pools, programs,
registries, operator sets and sinks all have an initializer that binds
caller-supplied storage; a grammar does not, so C code binds its fields by hand
(`parsegen_c_tests.c` does, in one helper) -- the same fields `fixed_grammar`
binds in C++. A `d_parsegen_grammar_init_fixed` would close it and let
`fixed_grammar` use it too.

