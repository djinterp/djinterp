# The djinterp interpreter and its command language: design

Lane G, 2026-11-04, amended 2026-11-09. For agreement with lanes F
(parse), E (options, the registry, desc) and H (dawk) by Friday 11-06, as
the brief asks; the owner rules what they do not settle. Lane F agreed on
11-05; section 0 says what that settled. Release numbers are the
renumbered ones: 0.8 on 11-30 carries the CLI preview with djins, 0.9 on
12-30 the interpreter and the language.

Read with `inc/djinterp/cpp/modules/parse/grammar/djinterp.peg` (the
normative grammar), `inc/djinterp/cpp/modules/interp/spine.hpp` (the spine
that runs a subset of it today, parse's `functional_cli.hpp` until 11-09),
`docs/djinterp/parse/expression_core_v2.md` (lane F's design, whose section
17.2 is this note's other half), the cli registers' rulings
(`docs/djinterp_decisions.md`, Part C) and `desc.txt`.


## 0. Agreed since 11-04

With lane F, on 11-05 (`G_to_F_2026-11-05.md` and F's reply; core v2's
design, 17.2), checked on F's delivery of 11-06:

- **Core v2 carries the command language.** F1 to F6 of section 8 are in
  F's C core: non-associative comparisons refused at the second operator,
  with a note at the first (`'==' cannot follow '<' without brackets`); the
  command as a custom role on a word in operand position; tokens of the
  interpreter's own classes, pulled from its source, which is told the
  position and the mode; spans that `d_parse_expr_locate` turns into the
  line and the column (in code points) that the kit's `d_cli_report_at`
  takes; a term in a post-order arena; and the renderer. F7 is G's patch of
  11-09 to `djinterp.peg` and its companions (section 4), which F takes in
  week 3.
- **A command's node.** `cmd(verb, arg1, ..., argN)`: the verb first, each
  word a leaf of its token's class whose span is the word as written and
  whose value indexes the interpreter's table of words with the quoting
  taken out; a block an apply node in its place. `cmd` is an operator only
  the role builds -- core v2 allows one -- defined as a flattening infix
  spelled `""` at 90, so that the renderer writes `ls -l | grep x` back.
- **Blocks.** A block, `{ Sequence }`, is read by core v2's sub-parse (F's
  week 2) with `}` among the classes that end it. A raw block -- awk's -- is
  read through a hook's own mode (core v2's F19): the hook takes `{` in the
  parse's mode, the body in its own (a bit other than 1, the parse's), and
  `}` in the parse's again. Core v2's stop option (F17) is the other way to
  read a block's sequence; G chooses between them when blocks are built.
- **G's own choices, from F's design.** `and` and `or` are built flattened,
  as `|` and `;` are: the meaning is the same, short-circuit runs over the
  list, and a long predicate stays shallow under the depth guard. The source
  reads a newline as `;` in operator position and as nothing in operand
  position, and drops a separator before a closing bracket, a closing brace
  or the end (P1). The evaluator walks the arena with its own stack -- lazy
  `and` and `or`, a pipeline that stops at its first failure -- so F5 needs
  nothing past the term's layout; strict folds serve `describe` and
  rendering.
- **Not in the language:** an implied `and` between two predicates, and
  `if ... then ... else`; F's design said both, and is corrected.
- **The command language's table and suites** live in the interpreter's
  module (`tests/djinterp/cpp/modules/interp/`); core v2's suite keeps its
  own command-line-like cases.

Lanes E and H have not answered this note yet; sections 6 and 7, and E1 to
E6 and H1 to H3, stand as written until they do.


## 1. What is decided here

1. Where the interpreter lives, and what moves into it (section 2).
2. The spine: reader, grammar, fold, each mapped onto expression core v2
   (section 3), and what lane G needs core v2 to give (section 8, F1 to F7).
3. The grammar's open defects, found reading `djinterp.peg` against the
   reader and the kit's rulings, each with a proposed fix (section 4).
4. What a value is, and what `|`, `;`, `and`, `or`, `not` and the
   comparisons do with one (section 5).
5. What a djin is, how it reaches the registry, and the 0.8 preview's
   commands (section 6), and what lane G needs of lane E (E1 to E6).
6. Where awk meets the language (section 7), and what lane G needs of lane
   H (H1 to H3).

Nothing here changes a stable API. The interpreter's library is preview in
0.9 and C++ only until a C core gives it the C face the parity law asks for
(the roadmap's ruling 13); the records a djin is made of are C from the start,
so that a djin can be written in C or C++ (section 6.2).


## 2. The module

**G-D1. The interpreter is a module of its own, `interp`.** Its headers in
`inc/djinterp/cpp/modules/interp/`, its sources in
`src/djinterp/cpp/modules/interp/`, its suites in
`tests/djinterp/cpp/modules/interp/`, its namespace `djinterp::interp`, and
its whole-module docs in `docs/djinterp/interp/` (ruling rs c). Later its C
core in `c/modules/interp/`, as parse has both.

- `functional_cli.hpp` moves there as `interp/spine.hpp` (agreed with lane F
  in the brief; done 11-09). Nothing includes it, so the move breaks no
  unit. Its namespace becomes `djinterp::interp`, opened by
  `D_KEYWORD_INTERP` and `NS_INTERP`, which the header defines until the
  root's keyword list (`djinterp.h`) names them; `D_KEYWORD_CLI` is the
  root's, and stays. Its two variable templates take
  `D_CONSTEXPR_INLINE_VAR`, as `cpp/cli` now does. Its suite,
  `tests/djinterp/cpp/modules/interp/spine_tests.cpp`, holds what it does
  as it moved.
- The program, `djinterp`, is a tool on the kit like bitwisedom and vparse:
  `src/djinterp/tools/djinterp/` for its `main` and its built-in djins, its
  CMake leaf in `build/cmake/config/djinterp/tools/djinterp/`. The doubled
  name is the one cost; `tools/interp` would name the library, not the
  program. Lane A, which owns `build/`, confirms the leaf.

Alternatives considered: keeping the interpreter inside parse (against: parse
is lane F's and its core is stable in 0.8, while the interpreter is preview
until a C core; one module would carry two tiers); putting it under
`tools/` (against: the library is meant to be used by programs other than
`djinterp`, dawk among them).


## 3. The spine on expression core v2

`functional_cli.hpp` has the right shape -- read, parse, fold -- and runs a
subset of the language: commands joined by `|`. The interpreter keeps the
shape and replaces each leg's insides.

### 3.1 Reading

Two readers, as today: `read_argv` for a command line the shell has already
split, `read_line` for a script or an interactive line. They differ in one
way that matters: **a word the shell split is one token whatever its
characters**; only its shape (`--name`, `-x`, `--`, a number, a keyword, an
operator) decides its kind. A line is lexed by the grammar's section I.

**G-D2. Every argument token keeps its text as written.** The grammar
decides where a command's arguments end -- at an operator, a keyword, a
bracket or the end -- and the command's own parser decides what they mean.
`functional_cli.hpp`'s `build_invocation`, which splits `-xvf` into three
flags and `--k=v` into a binding without knowing the command's options,
goes: `-ofile` is one option with a value to a command whose `-o` takes one,
and only that command's parser knows. A command is handed its words, as
`argv`, and parses them with the kit (`dj_cli_program` over a
`dj_arg_parser`), so a command reads the same typed in a script as typed at
a shell, its help and its errors are the kit's, and the rulings on the
kit's grammar (R1 to R20, cli-taken 13 to 85) hold inside the language
unchanged. This is cli-taken 51's principle -- one reading of a command
line, the parser's -- carried into the language.

### 3.2 The grammar

The PEG's syntactic grammar (section II) is a precedence cascade: `;` 10,
`|` 20, `or` 30, `and` 40, `not` 50 (prefix), the comparisons 60
(non-associative), primaries above. That is exactly a precedence climber's
table, and core v2 is to be one (lane F's week 1). The interpreter gives
core v2:

- the operator table above, as core v2's signature;
- a **primary hook**: the command sub-parser (verb, then arguments until a
  token that cannot be one), and `( Sequence )`;
- the token stream of 3.1 as core v2's input alphabet, each token with its
  span in the source (line, column, byte offset, length), so that a
  diagnostic points at the word.

and takes back a term: core v2's tree, whose leaves are commands and
literals and whose inner nodes are the operators. `command_pipeline`, the
spine's stand-in for the free-monad term, goes; the fold walks core v2's
term instead (3.3).

### 3.3 The fold

The interpreter is a fold over the term, through `functional`'s `result`
and `maybe` (promise 5, "the interpreter folds its pipelines through
functional's result and maybe"). Each node's rule is in section 5. Commands
are resolved through the registry (section 6), not the spine's
`command_registry` of first-match names: a djin's commands are the
registry's rows.

### 3.4 Two faces

Promise 6 asks that each djin command be tested both as a call and in the
language. The C++ library exposes the fold's steps as calls --
`interp::parse(text)`, `interp::evaluate(term, context)`,
`interp::run(djin, command, argv, input)` -- and the suites hold every
command run through the language to the same command run as a call.


## 4. The grammar: what `djinterp.peg` gets wrong today

Found reading the grammar against the reader, the kit's rulings and the
commands that exist. Each is a change to a file lane F owns; the fixes are
proposals, to agree on Friday. Numbers are this note's.

| # | Defect | Example | Proposed fix |
|---|---|---|---|
| P1 | The sequence's note says a newline separates pipelines, but `Skip` discards `Newline` before the syntactic grammar sees it. | `a` newline `b` is `a b`: one command with an operand. | A `NEWLINE` token outside brackets, read as `;`; inside `( )`, a blank. `Skip` loses `Newline`. |
| P2 | `--name=` cannot be lexed: `OptionValue` needs a character, and `=` is in no token. | `--note=` is a lexical error; R7 and cli-taken 33 make it the empty value. | `LONG_OPTION <- "--" OptionName ("=" OptionValue?)?`. |
| P3 | `-1` is a `SHORT_OPTION` (tried first), so `NUMBER`'s `"-"?` never matches, and `-1.5` lexes as `-1` and `.5`. | `size > -1` compares to the command `-1`. | A word that is wholly a number is a `NUMBER`: `NUMBER <- "-"? Digit+ ("." Digit+)? !WordChar`, tried before `SHORT_OPTION`. In argument position both are handed to the command as written (G-D2), so `ls -1` is unchanged. |
| P4 | A token in word shape has no end check, so a word splits where a token ends. | `2file` is `2` and `file`; `-x.txt` is `-x` and `.txt`. | `!WordChar` after `NUMBER` and `SHORT_OPTION`; otherwise the whole is a `WORD`. |
| P5 | `=` cannot be in a word. | `bitwisedom set s.json gen.level=c11`, `make CC=gcc`, `dd if=x` cannot be written. | `WordChar` takes `=`; `COMPARE` and the keywords are tokens only standing alone, followed by a blank, a bracket or the end, as `and` already is. |
| P6 | `#` begins a comment anywhere. | `C#`, `http://h/p#frag` are cut. | A comment begins only where a token could: at the start of input or after a blank, as `sh` reads it. |
| P7 | A string has no escape for its own quote, and quoted and bare parts do not join into one word, which the reader already does. | `"say \"hi\""`; `--name="a b"c`. | `sh`'s: single quotes literal; in double quotes `\"` and `\\`; a word is a run of bare and quoted parts. The reader and the grammar then agree. |
| P8 | A command takes no expression as an argument, so a predicate cannot be handed to a command that applies it. | `ls \| where size > 100` parses as `ls \| ((where size) > 100)`: the comparison is the stage, and `where` never sees it. | A block operand, `{ Sequence }`, evaluated by the command that takes it, once per item or once, as it says: `ls \| where { size > 100 }`. It is also awk's `pattern { action }` shape (section 7). |

**Amended 11-09, with the patch** (`G_2026.11.09_djinterp_grammar.patch`,
for lane F). Reading the grammar again to write it changed two rows. P1:
inside brackets a newline is read as it is outside, as `sh` reads it --
`(a` newline `b)` is two commands -- rather than as a blank, since one rule
is simpler and a pipeline or predicate already continues on the next line
after its operator; a separator may end a sequence, inside brackets too.
P6: the grammar already begins a comment only where a token could, and the
reader has no comments at all; the one word a comment cut is a quoted part
before `#` (`"x"#y`), which P7's joined words make whole, so P6 is a note in
the grammar, not a change to a rule. The patch also makes `{` and `}`
metacharacters, as `(` and `)` are, for P8. The patched grammar was run, as
written, by a small PEG interpreter over 81 cases.

**G-D3. Precedence stays as the PEG has it**: `|` binds looser than `and`
and `or`, so `test -f x and test -r x | report` hands the predicate's answer
to `report`. `sh` binds `&&` looser than `|`; the language's predicates are
values, not exit statuses alone, and a predicate is most often a stage of
its own. The keywords stay reserved at word boundaries: a command or operand
named `and` is quoted.

P8 is a new construct, not a fix; the alternative is commands that filter
by running a predicate command per item -- find's model -- with no block
syntax. Recommended: the block, because the same construct carries awk's
shape and the language's own `where`, `each` and `sort-by`, and a predicate
command per item cannot see the item it is run for without a convention
outside the grammar.


## 5. Values and what the operators do

**G-D4. A value is a tagged value**: nothing, a truth value, an integer
(64-bit signed), a real (double), text (UTF-8), bytes, a list, a record
(text keys, in insertion order), or a command's status alone. dawk's
`dvalue` is to be moved into the library as "the tagged value and hash
array" (`desc.txt`'s table); the interpreter's value is that module's, not
a second one (H1). Until it lands, the interpreter's suites use a stand-in
with the same tags.

| Construct | Meaning |
|---|---|
| `a ; b` | Run `a`, then `b`, whatever `a` came to; the sequence comes to `b`'s value and status, as `sh` does. |
| `a \| b` | `b` takes `a`'s value as its input; the first stage that fails ends the pipeline with its error (functional's `result`). The seed of a pipeline is nothing, or standard input's text where the program was given some. |
| `a and b` / `a or b` | Short-circuit on `a`'s truth: a truth value as it is; nothing, zero, an empty text, list or record false; a status true when it is 0. The construct comes to the value that decided it. |
| `not a` | The truth value opposite to `a`'s truth. |
| `a == b`, `!=`, `<`, `<=`, `>`, `>=` | On two numbers, numerically; on two texts, by code point; a number and a text that reads wholly as a number, numerically; anything else, `==` and `!=` by structure and the rest an error. Never chained (the grammar's rule). |
| `a =~ b` | `b` a pattern (the common regex module's ERE, H2); true when it matches anywhere in `a`'s text. |
| a command | Run with its words and the input value; it comes to its result value and status (section 6.3). |
| a literal | Its value: a number, or a text. A bare word is a command (PEG note 4). |

The status the program exits with is the last construct's status, by
GRAMMAR section 8: 0 done and yes, 1 done and no, 2 trouble. A predicate
that comes to false is 1, not an error.


## 6. Djins

### 6.1 What a djin is

A djin is a named, versioned unit of djinterp that a program can list,
describe, configure and run: it **registers commands** (verbs, each with its
options as the kit defines them), **settings** (rows in lane E's registry:
type, default, accepted values, where each value came from), and
**descriptions** (desc facts, lane E's describer kit). dawk is one, awk its
commands and the awk machine's variables its settings; bitwisedom's model
could be another.

### 6.2 The records

C records, as the kit's plugin records are, so that a djin is C or C++ and
the same djin can later be a loadable module through the kit's plugin ABI
(cli-taken 13 to 15, 40). A sketch, to agree with lane E:

```c
// a command a djin registers
struct dj_djin_command
{
    uint32_t    size;          // the record grows, by size, as the kit's do
    const char* name;          // the verb, a lowercase word (R1, cli-taken 66)
    const char* summary;       // a line, as the command list shows it
    // its options and operands, defined through the kit
    int       (*define)(struct dj_cli_program*, struct dj_arg_command*);
    // its work: the parsed command, the input value, the output value
    int       (*run)(struct dj_djin_call* _call);
};

// a djin
struct dj_djin
{
    uint32_t                      abi;
    uint32_t                      size;
    const char*                   name;      // `awk`
    const char*                   version;
    const char*                   summary;
    const struct dj_djin_command* commands;
    size_t                        command_count;
    // its settings: lane E's rows, under the djin's name
    const void*                   settings;  // E's row type, by E1
    size_t                        setting_count;
    // its description: desc facts, by E3
    int (*describe)(const struct dj_djin*, void* _facts);
};
```

`dj_djin_call` carries the program (for reports, `-q`, `-v`), the command
reached, the input value and a place for the output value. Its exact shape
waits on the tagged value (H1).

### 6.3 Through the registry

**G-D5. Djins reach the program only through lane E's registry.** The
program registers its built-in djins at start; the registry holds each
djin's settings as rows named `DJIN.KEY`, read from their sources in the
registry's order (build default, settings file, environment, command line)
and remembering which source each value came from; and the program builds a
kit command group per djin from its command records. So `--help` (lane E's,
from desc in December), `describe`, `get` and `set` all read one source and
cannot disagree with the code (`desc.txt`, "three uses from one source").

### 6.4 The 0.8 preview's command line

```
djinterp list                        the djins, a line each: name, version,
                                     summary
djinterp describe DJIN [COMMAND]     desc's facts, as text, or JSON
                                     (--format=json)
djinterp get DJIN.KEY...             settings' values; with -v, where each
                                     came from
djinterp set DJIN.KEY=VALUE...       settings written to the user's file
djinterp DJIN COMMAND [ARGUMENT]...  a djin's command, run
djinterp help, completion, --version the kit's
```

Each djin is a command group, as `git`'s subcommands are; a djin named as a
built-in command is refused at registration, by name. `get` and `set` take
the registry's names and, for a djin whose settings come from a file of its
own, fall back to that file's raw path for anything the registry does not
list -- bitwisedom's rule (cli-taken 16), made the language's. The 0.9
interpreter adds `djinterp -c 'LINE'` and `djinterp FILE` for scripts, and
standard input.

### 6.5 Loadable djins

Not in 0.8. In 0.9, a djin may be a module the kit loads, `djinterp-DJIN`
beside the program or along its search list (cli-taken 14,
`D_CFG_CLI_PLUGIN_SEARCH`), whose plugin record carries a `dj_djin`; a
module of another version is refused, or loaded with a warning, as the
program says (cli-taken 15, `D_CFG_CLI_PLUGIN_STRICT_VERSION`). `list` then
names where each came from.


## 7. awk

`desc.txt` settles the boundary: the djinterp language is native, awk is a
dialect over a shared core, and **they mix at module granularity**, never
inside an expression, so that awk's string-and-number duality does not leak
into the language's types.

- **awk is a djin, dawk's**, whose `run` command takes a program -- text, or
  a file -- and its operands, and whose settings are the awk machine's own,
  spelled as rows (`awk.fs`, `awk.ofs`, `awk.rs`), which awk's `FS`, `OFS`
  and `RS` map onto (the FS bug of last time came from mixing the two).
- In a pipeline, awk takes its input value as records -- a text as lines, a
  list as one record each -- and comes to its output as text, so
  `ls | awk run '{ print $1 }' | count` works on values the language
  already has.
- A block (P8) handed to awk's `run` is an awk program, read by dawk, not by
  the language's grammar: the block's text is kept, raw, between its braces,
  and the djin parses it. This is Racket's `#lang` boundary, at the size of
  a block.

**G-D6. The interpreter calls dawk only through its host hooks**
(`d_awk_host`: record reading, field splitting, patterns, statements), which
`desc.txt` names as the extension points the new engine keeps.


## 8. Requests

To lane F, for core v2's design and the grammar:

- **F1.** Non-associative operators at a level, with a diagnostic at the
  second operator for `a == b == c`.
- **F2.** A primary hook the client supplies: the command sub-parser.
- **F3.** A client-defined token alphabet: core v2 parses tokens, each with a
  kind and a span, not characters only.
- **F4.** Diagnostics at the character, from those spans: line, column, and
  the token's extent, so that `djinterp: error: 1:12: ...` and an editor's
  link point at the word.
- **F5.** Folds over the term with short-circuiting (`and`, `or`, and a
  pipeline's first failure), through functional's `result`.
- **F6.** Rendering a term back to canonical text, for `describe` of a
  command line and for diagnostics that quote it.
- **F7.** P1 to P8 into `djinterp.peg` (and its ABNF and EBNF companions),
  as agreed; the command grammar in week 3 follows them.

*11-09:* F1 to F6 are in F's delivery of 11-06 (section 0); F7 is G's
patch of 11-09, for F to take in week 3.

To lane E, for the registry (frozen 11-13) and desc:

- **E1.** The registry's row type, and rows named `DJIN.KEY` with a djin's
  name as their namespace.
- **E2.** The pattern for a knob that sets a default and a run-time option
  that overrides it -- the environment variable's name and the command-line
  option's spelling -- which the kit's rulings 14, 15, 16 and 25 wait on:
  this delivery has their knobs and their C settings, and maps the options
  onto E's pattern once it is agreed.
- **E3.** desc's fact sink, for a djin's `describe`.
- **E4.** Sources and provenance: build default, settings file (where, and
  in what format), environment, command line, in that order.
- **E5.** `--help` from desc (December) takes the kit's layout and rulings
  (84: `@FILE` on every page; 57: what a command's status answers).
- **E6.** The registry's command face (December): the records of 6.2, or
  E's equivalent.

To lane H, for dawk:

- **H1.** The tagged value and hash array moved, not copied, into the
  library (`desc.txt`'s table), before the interpreter's week 2 of December.
- **H2.** `c/util/dregex` as the only regex, with BRE beside ERE (`desc.txt`),
  for `=~`.
- **H3.** dawk's `d_awk_host` hooks frozen with the command language
  (12-11), and the awk djin of section 7.

To lane A:

- **A1.** The core's export macro for the kit's SHARED_CORE build
  (cli-taken 13): until it is agreed, the kit's CMake module exports every
  function of the shared core through CMake's list on Windows.
- **A2.** The program's CMake leaf,
  `build/cmake/config/djinterp/tools/djinterp/`.


## 9. Schedule

| When | What |
|---|---|
| Fri 11-06 | This note agreed or amended with F, E and H. *F agreed 11-05; E and H to come.* |
| Mon 11-09 | The grammar patch for F; the spine moved to `interp`. |
| Wed 11-11 | The djin records (6.2) and the program's `list` and `describe`, on a stand-in for E's registry. |
| Fri 11-13 | E's registry and option pattern; F's core v2 code. The kit's options on E's pattern. |
| Weeks 2 and 3 | The 0.8 preview (6.4): `list`, `describe`, `get`, `set`, a djin's commands run; the interpreter's skeleton on core v2 -- sequences and pipelines -- behind it. |
| Week 4 | The preview's suites and docs; this note's December half, refined. |
| December | Predicates, blocks, values; djins through the interpreter; awk run by dawk (H); the language frozen 12-11; suites, docs and the man page by 12-18. |

The cut, if checkpoint 2 (11-15) finds the lane a week behind, is the
roadmap's trigger 1: the preview shrinks to this note and a skeleton.
