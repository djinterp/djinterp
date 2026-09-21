# parse / parsegen foundation — steps 1 through 6

## Install

Unzip at the **repository root** (the directory containing `inc/`, `src/`,
`tests/` and `build/`). Every path in the archive is relative to that root.

```bash
cd <repo-root>
unzip -o vparse.zip
build/cmake/config/testing/djinterp/parsegen/foundation/run_foundation_tests.sh
```

Expected: `passed: 18   failed: 0`. Add `--matrix` to also build and run
under every configuration knob.

**Exactly one existing file is replaced:** `inc/djinterp/parsegen/parsegen.hpp`.
Its only dependent is `inc/djinterp/parsegen/vparse/machine.hpp`, and the old
umbrella included `../core/djinterp.hpp`, which does not exist in the snapshot
this was verified against — so the replacement, which includes the real
`../djinterp.hpp` and defines `NS_PARSEGEN` identically, is strictly an
improvement for that dependent. Everything else in the archive is a new file.

Nothing in the existing `vparse` tree is modified or removed. The old `peg`,
`gen`, `notation` and `adapter` keep building exactly as before; moving them
onto this foundation is the port, which is separate work.

### CMake

`build/cmake/config/testing/djinterp/parsegen/foundation/CMakeLists.txt` is a
plain-CMake leaf. Wire it with `add_subdirectory()` wherever your other test
leaves are added. It does not use `djinterp_add_test_executable`, because that
helper's source was not available; swapping to it is mechanical.

### Three pre-existing framework issues

Building against your real root headers, rather than stand-ins, surfaced three
issues in the framework itself. Neither is in this foundation and both affect
existing code; each is worked around in the script and the CMake leaf so the
drop-in builds, and each has a one-line real fix. The zip deliberately does
**not** patch your root headers: your working copy may differ from the
snapshot this was verified against, and overwriting a root header is what a
drop-in must not do.

#### `D_KEYWORD_FRAMEWORK_NAME` is used but never defined

The C root defines `D_FRAMEWORK_NAME djinterp`. The C++ root's `NS_DJINTERP` is
`D_NAMESPACE(D_KEYWORD_FRAMEWORK_NAME)`, and nothing defines
`D_KEYWORD_FRAMEWORK_NAME` — it looks like a rename that landed in one root and
not the other. So in this snapshot every `NS_DJINTERP` opens a namespace
literally named `D_KEYWORD_FRAMEWORK_NAME`, which affects every C++ header in
the tree that uses it. Only `core/fs/file_path.hpp` works around it, with its
own `NS_DJINTERP`.

Workaround: `-DD_KEYWORD_FRAMEWORK_NAME=djinterp` (CMake option
`DJINTERP_FOUNDATION_NAME_FIX`). Safe even where a header also defines it,
since an identical redefinition is allowed. **Real fix:** in `djinterp.hpp`,
either define `D_KEYWORD_FRAMEWORK_NAME` or have `NS_DJINTERP` use
`D_FRAMEWORK_NAME`.

#### The env include-order issue

Both the script and the CMake leaf also force-include `djinterp/env/env_os.h`.
In this snapshot `env/c/env_c_lib.h` uses `D_ENV_IS_OS_POSIX_LIKE_OR_ANDROID` in an
`#if` before `env_os.h`, which defines it, is included — so the framework root
does not preprocess on its own, with or without anything here. Force-including
`env_os.h` first fixes it and is harmless where unneeded. **The real fix is one
`#include` in `env_c_lib.h`**; then drop the workaround (the CMake option is
`DJINTERP_FOUNDATION_ENV_FIX`).

C foundation with a zero-overhead C++ face. Verified: clean under
`-Wall -Wextra -Wpedantic` on C99/C11/C17, C++20 consumer linking against a
**C archive** (`gcc -c` → `ar` → `g++`), 18/18 tests pass, and all fourteen
knob configurations build and pass including
`-DD_CFG_PARSE_ALL=0 -DD_CFG_PARSEGEN_ALL=0` (no allocator anywhere, no
formatting, no transport, no trace).

#### C builds get C23 attribute syntax under `-std=c11`

`env/c/env_attributes.h` resolves the C-side `D_NODISCARD` in four steps; step 2
probes `__has_c_attribute(nodiscard)`, which GCC answers "yes" even in C11 mode
because it accepts `[[...]]` as an extension. So C11 builds get `[[nodiscard]]`,
and `-Wpedantic` flags every use as C2X syntax. This is a warning, not an
error, and it applies to your existing C code as much as to this foundation.

Workaround: the script leaves `-Wpedantic` off by default, matching what your
own build evidently does; `PEDANTIC=1` turns it on. **Real fix:** delete step 2
— step 1 already covers C23 and step 3 covers GCC and Clang before it.

## Files

| path | what |
|---|---|
| `inc/djinterp/config/parse/cfg_parse.h` | every `D_CFG_PARSE_*` knob and its `D_INTERNAL_PARSE_*` derived value |
| `inc/djinterp/parse/diagnostic.h` | C: `d_parse_span`, `d_parse_diagnostic`, `d_parse_diag_sink` |
| `inc/djinterp/parse/machine.h` | C: `d_parse_machine`, `d_parse_voperator`, `d_parse_op_set` |
| `inc/djinterp/parse/charset.h` | C: `d_parse_charset`, the canonical 256-bit class |
| `inc/djinterp/parse/pool.h` | C: `d_parse_pool`, the operand intern pool |
| `inc/djinterp/parse/program.h` | C: `d_parse_instr`, `d_parse_program`, verify/hash/transport |
| `inc/djinterp/parse/storage.h` | C: `d_parse_grow`, the one growth policy |
| `inc/djinterp/parsegen/feature.h` | C: `d_parsegen_features`, the capability vocabulary |
| `inc/djinterp/parsegen/registry.h` | C: `d_parsegen_stage`, `d_parsegen_registry`, selection |
| `inc/djinterp/parsegen/grammar.h` | C: `d_parsegen_node`, `d_parsegen_grammar`, the neutral model |
| `inc/djinterp/parsegen/analysis.h` | C: `d_parsegen_facts` — nullability, first sets, left recursion |
| `inc/djinterp/config/parsegen/cfg_parsegen.h` | parsegen's own knobs and derived values |
| `inc/djinterp/parse/diagnostic.hpp` | C++: `span`, `diagnostics`, `fixed_diagnostics<N,M>`, views, iteration |
| `inc/djinterp/parse/machine.hpp` | C++: `machine`, `op_set`, `fixed_op_set<N>`, the `def()` family |
| `inc/djinterp/parse/charset.hpp` | C++: `charset`, constexpr membership |
| `inc/djinterp/parse/pool.hpp` | C++: `pool`, `fixed_pool<B,E>` |
| `inc/djinterp/parse/program.hpp` | C++: `instr` (alias), `program`, `fixed_program<N,B,E>` |
| `inc/djinterp/parsegen/registry.hpp` | C++: `feature`, `feature_set`, `stage`, `registry` |
| `inc/djinterp/parsegen/grammar.hpp` | C++: `node` (alias), `grammar`, `fixed_grammar<…>` |
| `inc/djinterp/parsegen/analysis.hpp` | C++: `facts`, `fixed_facts<N,R>`, the named queries |
| `inc/djinterp/parsegen/parsegen.h` | C umbrella: subsystem keyword, generator diagnostic domains |
| `inc/djinterp/parsegen/parsegen.hpp` | C++ face: `NS_PARSEGEN` and nothing else |
| `src/djinterp/parse/{diagnostic,machine,charset,pool,program,storage}.c` | definitions |
| `src/djinterp/parsegen/{feature,registry,grammar,analysis}.c` | definitions |
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
tail. That is now `d_parse_grow` in `parse/storage.h`, and `op_set`, both pool
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
built by the included script, and run. That is what surfaced the two framework
issues described under Install. The guarded `NS_PARSE` / `D_KEYWORD_PARSE`
spellings were also confirmed token-identical to `parse.hpp`'s, which is what
makes their redefinition benign.

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

for c in $ROOT/src/djinterp/parse/*.c $ROOT/src/djinterp/parsegen/*.c; do
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
