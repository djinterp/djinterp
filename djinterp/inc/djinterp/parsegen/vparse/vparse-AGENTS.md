# vparse — agent guide

`djinterp::parsegen::vparse`. A parser generator: given a grammar `G`, produce a
parser `P` by *building* it — never learning it — so `P` is correct by
construction. The whole thing runs on one core idea, and if you internalize only
one thing, make it this:

> **The machine is operator-agnostic.** There is no central `switch` on opcode
> anywhere. Execution dispatches through a per-family registry (`op_set`) that
> maps an opcode to a `std::function<void(machine&)>`. Behavior is *registered*,
> not branched. Adding capability = adding a handler, never editing a dispatch
> loop.

Everything below serves that idea.

---

## 1. Orientation in 30 seconds

Three layers, cleanly separated:

| layer         | what it is                                    | where                 |
|---------------|-----------------------------------------------|-----------------------|
| **substrate** | run-state + the registry; no opcode meaning   | `machine.hpp`         |
| **operators** | families that register handlers into an `op_set` | `peg`, `gen`, `notation` |
| **driver**    | a family's `run`/fetch loop over its program  | `peg::run`, `gen::generate` |

Four operator families share the substrate, each with its **own private opcode
space** (so `peg::CHAR`, `gen::G_PREAMBLE` and `lr::SHIFT` are all `0` and
coexist — they live in different `op_set`s and never collide), plus a second
frontend:

- **`peg`** — a PEG virtual machine (recursive-descent, PC-driven). Runs a flat
  instruction stream against input. This is what actually recognizes text.
- **`gen`** — the generator. Walks a `ruleset` and *emits* a `peg::program`.
- **`notation`** — the grammar-of-grammars. Parses grammar *text* into a
  `ruleset`. It is itself powered by `gen` compiling a seed grammar (metacircular
  — see §4).
- **`lr`** — a shift/reduce family: its own opcodes (`SHIFT`, `REDUCE`, `ACCEPT`,
  `ERROR`), table format and table-driven driver. Runs left-recursive grammars
  the PEG family cannot. Tables are **supplied**, not yet generated.
- **`ebnf`** — a second frontend, not an operator family: hand-written recursive
  descent over EBNF text (grouping, `?`, string literals, `.`, `#` comments)
  compiling straight to a `peg::program`. Overlaps `notation` — see §9.

---

## 2. File map

```
inc/djinterp/parsegen/vparse/
  machine.hpp    substrate: struct machine, voperator, op_set (def/find/name)
  ruleset.hpp    grammar model: term/rule/ruleset + builders lit/cls/ref/any_ch
  peg.hpp        PEG family: opcodes, instr, program, capture, run(), make_ops()
  gen.hpp        generator family: directives, gen_instr, plan/generate/compile()
  notation.hpp   grammar language: meta_grammar/read_ruleset/parse_grammar()
  adapter.hpp    carrier bridge: as_parser(program) -> parser<std::string,char>
  lr.hpp         LR family: opcodes, table/tokens/result, run(), make_ops()
  ebnf.hpp       EBNF frontend: parse() text -> rules, compile() -> program

src/djinterp/parsegen/vparse/
  peg.cpp gen.cpp notation.cpp adapter.cpp lr.cpp ebnf.cpp   (implementations)

tests/djinterp/parsegen/vparse/
  vparse_tests.hpp              shared harness + fixtures
  vparse_tests_captures.cpp     backtrack-safe capture unwinding; traced run
  vparse_tests_generate.cpp     gen::compile parity (anchor, multi-rule)
  vparse_tests_notation.cpp     grammar text -> ruleset -> parser
  vparse_tests_rebase.cpp       real carrier: handle + combinator composition
  vparse_tests_lr.cpp           LR family: left recursion, trace, step limit
  vparse_tests_ebnf.cpp         EBNF frontend: arithmetic, grouping, errors

build/cmake/config/testing/djinterp/parsegen/vparse/
  vparse_tests_runner.cpp       main(): runs each tests_* section, prints PASS/FAIL
  CMakeLists.txt                djinterp_add_test_executable leaf
```

Headers are declaration-only; the banners carry design intent (read them). The
whole subsystem depends on the framework through **one contract only**:
`parser<R,E>`, `parse_state`, `parse_result` from `djinterp::parse`. Keep it that
way — it is what makes vparse portable.

---

## 3. Public API cheat-sheet

**Build a grammar in code** (`ruleset.hpp`) — first rule is the start symbol;
`star` = zero-or-more of that term; `cap` = capture tag (0 = none):

```cpp
term lit(char c, int cap=0);                          // literal char
term cls(const std::string& set, bool star=false, int cap=0);  // [class]
term ref(const std::string& name, bool star=false, int cap=0); // rule reference
term any_ch(int cap=0);                               // any one char  (T_ANY)

struct rule    { std::string name; std::vector<std::vector<term>> alts; };  // alts = ordered choice
struct ruleset { std::vector<rule> rules; };          // rules.front() = start
```

**Compile a grammar to a parser** — the common path:

```cpp
namespace notation { ruleset parse_grammar(const std::string& text, bool* ok=nullptr); }
namespace gen      { peg::program compile(const ruleset& g); }
parser<std::string,char> as_parser(const peg::program& prog);   // adapter.hpp
```

**Lower-level PEG surface** (`peg.hpp`) — hand-author or inspect programs:

```cpp
enum { CHAR, ANY, CHOICE, JUMP, CALL, RETURN, COMMIT, FAIL, MATCH, SET, MARK, CAP };
struct instr { int op; int arg=-1; char ch=0; std::string set; };  // arg = jump target OR capture tag
using program = std::vector<instr>;
struct capture { int tag; int start; int end; };      // half-open input span

op_set make_ops();
bool   run(parse_state<char>& st, const program& prog, const op_set& ops,
           std::vector<capture>* caps=nullptr);        // advances st.offset on match
bool   run(machine& m, parse_state<char>& st, const program& prog,
           const op_set& ops,
           std::vector<capture>* caps=nullptr);        // same, on a caller's machine
std::string fmt(const instr& i);                       // disassembly
```

The machine overload is the observable one: it honours `m.step_limit` (a run
that hits it fails with `m.error == "step limit"` and leaves `st.offset` put),
and when `m.trace` is set it appends one line per dispatch, written *before*
the operator runs:

```
ip=12 off=3 calls=2 back=1 marks=0 caps=0 | CHAR '+'
```

`m.steps` counts dispatches, so a matched run leaves `m.steps + 1` lines (the
last is the `MATCH`). `m.ext` is cleared on return. The four-argument `run`
delegates to it with a fresh machine.

**Generator internals** (`gen.hpp`) — if you need the plan, not just the program:

```cpp
enum { G_PREAMBLE, G_RULE_BEGIN, G_RULE_END, G_ALT_CHOICE, G_ALT_COMMIT,
       G_EMIT_CHAR, G_EMIT_SET, G_EMIT_REF, G_EMIT_ANY, G_EMIT_MARK, G_EMIT_CAP,
       G_STAR_BEGIN, G_STAR_END, G_FINISH };
gen_program  plan(const ruleset& g);                   // ruleset -> directives
peg::program generate(const gen_program& p, const op_set& ops);  // directives -> program
peg::program compile(const ruleset& g);                // plan + generate, one call
```

**LR family** (`lr.hpp`) — a supplied table, run over `(kind, value)` tokens:

```cpp
op_set     lr::make_ops();
lr::result lr::run(machine& m, const lr::table& t, const lr::tokens& toks, const op_set& ops);
```

**EBNF frontend** (`ebnf.hpp`) — never throws; failures come back through `ok`:

```cpp
std::pair<ebnf::rules, std::string> ebnf::parse(const std::string& text, bool* ok=nullptr, std::string* error=nullptr);
peg::program ebnf::compile(const ebnf::rules& r, const std::string& start, bool* ok=nullptr, std::string* error=nullptr);
```

### Canonical example (the anchor)

```cpp
using namespace djinterp::parsegen::vparse;
namespace pr = djinterp::parse;

ruleset g = notation::parse_grammar("S = [0-9]+ ;");   // grammar text -> ruleset
pr::parser<std::string,char> p = as_parser(gen::compile(g));   // -> real parser<R,E>

std::string s = "12345";
pr::parse_state<char> st(s.data(), s.size(), 0);
auto r = p(st);           // r.ok() == true, r.value() == "12345"
```

`S = [0-9]+ ;` compiles to an 11-instruction program byte-identical to the
hand-written equivalent — that byte-identity is the parity guarantee the
`generate` tests defend.

---

## 4. The pipeline (and why it's metacircular)

```
grammar text ──notation::parse_grammar──▶ ruleset ──gen::compile──▶ peg::program ──as_parser──▶ parser<std::string,char>
                       │
   internally parse_grammar is itself:
       notation::meta_grammar()  ──gen::compile──▶  program            (the grammar-of-grammars, compiled by gen)
       peg::run(program, text)   ──▶  captures
       notation::read_ruleset(captures, text)  ──▶  ruleset            (the "reader")
```

The meta-grammar is authored as an ordinary `ruleset` (the irreducible seed) and
compiled by the *same* `gen` used for user grammars. So the front-end that turns
text into grammars is built by the generator it feeds. `read_ruleset` (captures →
ruleset) is currently host C++ — closing that (expressing the reader in the
notation itself) is the self-hosting item on the roadmap.

Notation syntax the meta-grammar accepts: `Name = alt | alt ;`, terms are `'x'`
(literal, needs `ANY`/`T_ANY`), `[class]`, `Ref`, postfix `*` and `+`
(`x+` desugars to `x x*`), sequence by juxtaposition. Capture tags emitted:
`NAME, SETBODY, REF, STAR, PLUS, LITCH, BAR`.

---

## 5. Invariants — do not break these

1. **No central dispatch.** `machine` has no opcode `switch`; the driver
   dispatches only via `op_set::find`. New behavior is a new handler in a
   family's `make_ops()`, never a branch in the substrate.
2. **Opcode spaces are family-private.** Never share opcode integers across
   families or assume an opcode has global meaning. `peg` and `gen` both start at
   `0` on purpose.
3. **`|` is *ordered* choice (PEG, not CFG).** First alternative that matches
   wins and commits; there is no ambiguity and no backtracking across a `COMMIT`.
   Don't "fix" this toward CFG semantics.
4. **No left recursion.** A rule referencing itself leftmost will not terminate.
   (Left recursion is what the `lr` family is for; its table is currently
   supplied by hand.)
5. **Captures are backtrack-safe.** `CHOICE` snapshots the mark-stack and
   capture-stack depths; `FAIL` unwinds to them. Any new backtracking opcode must
   preserve this snapshot/unwind discipline or captures will leak across dead
   alternatives.
6. **`gen::compile` output is a *whole-input* recognizer.** The compiled program
   carries an end-of-input gate (`CALL start` + `!.` test + `MATCH`), so it
   matches only if it consumes the entire input. See §7 for the consequence.
7. **Carrier contract only.** vparse uses `parser<R,E>` / `parse_state` /
   `parse_result` and nothing else from the framework.

---

## 6. Build & test

The subsystem builds with the project's CMake helper
(`djinterp_add_test_executable`, which adds `inc/` + the test leaf and defines
`D_TESTING=1`). The equivalent raw invocation — useful for a quick agent loop —
mirrors exactly the include dirs that helper sets:

```bash
g++ -std=c++20 -DD_TESTING=1 -O2 -Wall -Wextra \
  -Iinc -Iinc/djinterp/parsegen/vparse -Itests/djinterp/parsegen/vparse \
  src/djinterp/parsegen/vparse/peg.cpp \
  src/djinterp/parsegen/vparse/gen.cpp \
  src/djinterp/parsegen/vparse/notation.cpp \
  src/djinterp/parsegen/vparse/adapter.cpp \
  src/djinterp/parsegen/vparse/lr.cpp \
  src/djinterp/parsegen/vparse/ebnf.cpp \
  tests/djinterp/parsegen/vparse/vparse_tests_captures.cpp \
  tests/djinterp/parsegen/vparse/vparse_tests_generate.cpp \
  tests/djinterp/parsegen/vparse/vparse_tests_notation.cpp \
  tests/djinterp/parsegen/vparse/vparse_tests_rebase.cpp \
  tests/djinterp/parsegen/vparse/vparse_tests_lr.cpp \
  tests/djinterp/parsegen/vparse/vparse_tests_ebnf.cpp \
  build/cmake/config/testing/djinterp/parsegen/vparse/vparse_tests_runner.cpp \
  -o vparse_tests && ./vparse_tests
```

Expected: `passed: 15   failed: 0`. Discipline for any change: **everything
compiles + all 15 pass** before moving on. When editing a `.cpp`, rebuild from a
removed binary (`rm -f vparse_tests`) so a stale executable never masks a failed
build.

---

## 7. Gotchas (things that have actually bitten)

- **Whole-input gate vs. composition.** Because `gen::compile` emits a whole-input
  recognizer (invariant 6), feeding its output to a real combinator like
  `many`/`sep_by` as a *sub*-parser fails on the first call — the sub-parser
  rejects the trailing input it should leave for the next combinator. To compose,
  build a **non-gated prefix program** directly via `peg::program` + `as_parser`
  (the `rebase_compose` test does exactly this: a bare `SET/CHOICE/SET/COMMIT/
  MATCH` digit-run, driven by real `sep_by(_, literal(' '))`). The clean fix on
  the roadmap is a `compile_prefix` that omits the EOF gate.
- **`as_parser` semantics.** The produced handle runs from `st.offset`; on match
  it advances `offset` and returns the consumed span as the `std::string` value;
  on failure it leaves `offset` put and returns `parse_error(DParseStatusFailure)`.
  The `op_set` is built once and captured in the handle.

### Built against the real framework — no stand-ins

vparse builds against your real root headers. The sandbox stand-in
`core/djinterp.hpp` is retired; `machine.hpp` includes `../../djinterp.hpp`.
That needed four framework fixes, which are in the tree:

- `NS_DJINTERP` expands through `D_FRAMEWORK_NAME`, which the C root defines;
  it previously named `D_KEYWORD_FRAMEWORK_NAME`, which nothing defined.
- `djinterp::functional` is declared in `djinterp.hpp`: `parse.hpp`,
  `core/functional` and `core/event` spell the functional API that way, and only
  the stand-in used to declare it.
- `env.h` includes `c/env_c_lib.h` after the detection headers and before
  `env_build.h`, the order its banner documents.
- `parser_expr` in `parser.hpp` deduces its return type at the point of use; the
  trailing `decltype` it used is evaluated by g++ on an incomplete type.

The `monad_map` `traits::` fix was already present. Open framework item, not in
the parse path: `maybe` has no Alternative or Traversable instance, though both
protocol headers document one.

---

## 8. Extending vparse

- **Add a PEG opcode:** add the enum value in `peg.hpp`; register a handler in
  `peg::make_ops()` (`peg.cpp`) that reads/writes the family state (the
  `parse_state` threaded through `machine.ext`); extend `fmt()` for disassembly.
  No substrate change.
- **Add a notation operator** (e.g. `?`): teach `meta_grammar()` to emit a new
  capture tag, handle that tag in `read_ruleset()`, and lower it to
  terms/opcodes. No change to `peg` or the substrate.
- **Add a whole new family** (this is the LR path): a new private opcode enum +
  `make_ops()` + a `run`/driver, all sharing `machine` and `op_set`. `gen` would
  target the new family's program (e.g. an `lr::table`) instead of `peg`.

---

## 9. Roadmap

- **LR table generation** — the `lr` driver exists; what remains is `gen`
  building an `lr::table` from a grammar (LR(0), SLR or LALR) instead of it being
  supplied by hand.
- **One notation** — `ebnf` and `notation` overlap. Extending the metacircular
  meta-grammar to grouping and `?` would retire `ebnf`'s hand-written parser.
- **The port** — moving vparse onto the `parse` / `parsegen` foundation (the
  program IR, the operator registry, the neutral grammar), where `ebnf`'s tree
  maps directly onto the neutral grammar.
  The foundation keeps its C in `parse/c/` and `parsegen/c/`, with the C++
  faces beside them; the layout rule is in `parsegen/FOUNDATION.md`.
- **Full self-hosting** — express `read_ruleset` (captures → ruleset) in the
  notation itself, so the front-end is entirely bootstrapped.
- **`compile_prefix`** — a non-EOF-gated compile so generated parsers compose
  cleanly as sub-parsers under the combinator algebra.
