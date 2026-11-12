# The djinterp interpreter and its command language: design

Written 2026-11-04, amended 2026-11-09, 2026-11-11 and 2026-11-13. Agreed with the
designs it depends on: expression core v2's, the options' and the
registry's, and dawk's. The owner rules what they do not settle.

Release numbers are the renumbered ones:

- **0.8, on 11-30:** the CLI preview, with djins.
- **0.9, on 12-30:** the interpreter and the language.

Read it with:

- `inc/djinterp/cpp/modules/parse/grammar/djinterp.peg`, the normative
  grammar;
- `inc/djinterp/cpp/modules/interp/spine.hpp`, the spine that runs a subset
  of the grammar today;
- `docs/djinterp/parse/expression_core_v2.md`, core v2's design, whose
  section 17.2 is this note's other half;
- `docs/djinterp/option/knob_and_option.md`, the pattern every setting
  follows;
- the cli registers' rulings in `docs/djinterp_decisions.md`, Part C;
- `desc.txt`.


## 0. Agreed since 11-04

### Expression core v2

Agreed on 11-05 (core v2's design, 17.2) and checked on the core delivered
11-06; the scanner, the sub-parse and the caller's scratch followed on
11-11.

- **Core v2 carries the command language.** Its C core gives what section 8
  asked of it:
  - non-associative comparisons, refused at the second operator with a note
    at the first (`'==' cannot follow '<' without brackets`);
  - the command, as a custom role on a word in operand position;
  - tokens of the interpreter's own classes, pulled from its source, which
    is told the position and the mode;
  - spans that `d_parse_expr_locate` turns into the line and the column (in
    code points) that the kit's `d_cli_report_at` takes;
  - a term in a post-order arena;
  - the renderer.

  The grammar's fixes of section 4 are in `djinterp.peg` and its companions
  since 11-11.
- **A command's node** is `cmd(verb, arg1, ..., argN)`:
  - the verb first;
  - each word a leaf of its token's class, whose span is the word as written
    and whose value indexes the interpreter's table of words, with the
    quoting taken out;
  - a block an apply node in its place.

  `cmd` is an operator only the role builds, which core v2 allows. It is a
  flattening infix spelled `""` at 90, so that the renderer writes
  `ls -l | grep x` back.
- **Blocks.**
  - A block, `{ Sequence }`, is read by core v2's sub-parse, with `}` among
    the classes that end it.
  - A raw block, awk's, is read through a hook's own mode. The hook takes `{`
    in the parse's mode, the body in its own (a bit other than 1, the
    parse's), and `}` in the parse's again.
  - Where awk's block ends is dawk's to find (section 7).
  - Core v2's stop option is the other way to read a block's sequence. The
    choice between them is made when blocks are built.
- **The reader keeps its own tokens.** Core v2's scanner takes the caller's
  class numbers and fills the same token record. It does not consult the
  position or the mode it is told, and the command language's tokens depend
  on both (the option shapes, the word parts, P1 to P8). So the interpreter's
  reader fills `dj_parse_expr_token` itself.
- **Choices made here, from core v2's design:**
  - `and` and `or` are built flattened, as `|` and `;` are. The meaning is
    the same, short-circuit runs over the list, and a long predicate stays
    shallow under the depth guard.
  - The source reads a newline as `;` in operator position and as nothing
    in operand position. It drops a separator before a closing bracket, a
    closing brace or the end (P1).
  - The evaluator walks the arena with its own frames (3.3). Strict folds
    serve `describe` and rendering.
- **Not in the language:** an implied `and` between two predicates, and
  `if ... then ... else`. Core v2's design said both, and is corrected.
- **The command language's table and suites** live in the interpreter's
  module (`tests/djinterp/cpp/modules/interp/`). Core v2's suite keeps its
  own command-line-like cases.

### The options and the registry

Agreed on 11-10, against `knob_and_option.md`, `c/option/option_env.h` and
`c/option/option_env_registry.h`. Section 6 is written to them.

### dawk

Agreed on 11-10, against `tools/dawk/` (`dinterp.h`, `dsource.h`,
`dhost.h`, `dvalue.h`) and `c/util/dregex.h`. Sections 5 and 7 are written
to them.


## 1. What is decided here

1. Where the interpreter lives, and what moves into it (section 2).
2. The spine: reader, grammar and fold, each mapped onto expression core v2
   (section 3).
3. The grammar's defects, found reading `djinterp.peg` against the reader and
   the kit's rulings, each with its fix (section 4).
4. What a value is, and what `|`, `;`, `and`, `or`, `not` and the
   comparisons do with one (section 5).
5. What a djin is, how it reaches the registry, and the 0.8 preview's
   commands (section 6).
6. Where awk meets the language (section 7).
7. What the interpreter depends on, and where each stands (section 8).

Nothing here changes a stable API.

- The interpreter's library is preview in 0.9.
- It is C++ only until a C core gives it the C face the parity law asks for
  (the roadmap's ruling 13).
- The records a djin is made of are C from the start, so that a djin can be
  written in C or C++ (section 6.2).


## 2. The module

**Decision 1. The interpreter is a module of its own, `interp`.**

| | |
|---|---|
| headers | `inc/djinterp/cpp/modules/interp/` |
| sources | `src/djinterp/cpp/modules/interp/` |
| suites | `tests/djinterp/cpp/modules/interp/` |
| namespace | `djinterp::interp` |
| whole-module docs | `docs/djinterp/interp/` (ruling rs c) |

Later its C core goes in `c/modules/interp/`, as parse has both.

- **The spine moved there on 11-10**, as `interp/spine.hpp`. It was parse's
  `functional_cli.hpp`, which is deleted.
  - Its namespace is `djinterp::interp`, opened by `D_KEYWORD_INTERP` and
    `NS_INTERP`.
  - The header defines those two until the root's keyword list
    (`djinterp.h`) names them, on 11-13. `D_KEYWORD_CLI` is the root's, and
    stays.
  - Its two variable templates take `D_CONSTEXPR_INLINE_VAR`, as `cpp/cli`
    does.
  - Its suite, `tests/djinterp/cpp/modules/interp/spine_tests.cpp`, holds
    what it does, with its leaf in
    `build/cmake/config/testing/djinterp/cpp/modules/interp/`.
- **The program, `djinterp`, is a tool on the kit**, like bitwisedom and
  vparse:
  - `src/djinterp/tools/djinterp/` holds its `main` and its built-in djins;
  - its CMake leaf is `build/cmake/config/djinterp/tools/djinterp/`.

  The doubled name is the one cost. `tools/interp` would name the library,
  not the program.

**Alternatives considered:**

- Keeping the interpreter inside parse. Against: parse's core is stable in
  0.8, while the interpreter is preview until a C core, so one module would
  carry two tiers.
- Putting it under `tools/`. Against: the library is meant to be used by
  programs other than `djinterp`, dawk among them.


## 3. The spine on expression core v2

The spine has the right shape -- read, parse, fold -- and runs a subset of
the language: commands joined by `|`. The interpreter keeps the shape and
replaces each leg's insides.

### 3.1 Reading

Two readers, as today:

- `read_argv`, for a command line the shell has already split;
- `read_line`, for a script or an interactive line.

They differ in one way that matters: **a word the shell split is one token,
whatever its characters.** Only its shape (`--name`, `-x`, `--`, a number, a
keyword, an operator) decides its kind. A line is lexed by the grammar's
section I.

**Decision 2. Every argument token keeps its text as written.** The grammar
decides where a command's arguments end: at an operator, a keyword, a
bracket or the end. The command's own parser decides what they mean.

- The spine's `build_invocation` goes. It splits `-xvf` into three flags and
  `--k=v` into a binding without knowing the command's options.
- But `-ofile` is one option with a value to a command whose `-o` takes one,
  and only that command's parser knows.
- So a command is handed its words, as `argv`, and parses them with the kit
  (`dj_cli_program` over a `dj_arg_parser`). Then:
  - a command reads the same typed in a script as typed at a shell;
  - its help and its errors are the kit's;
  - the rulings on the kit's grammar (R1 to R20, cli-taken 13 to 85) hold
    inside the language unchanged.

This is cli-taken 51's principle -- one reading of a command line, the
parser's -- carried into the language.

### 3.2 The grammar

The PEG's syntactic grammar (section II) is a precedence cascade:

| level | operators |
|---|---|
| 10 | `;` |
| 20 | `\|` |
| 30 | `or` |
| 40 | `and` |
| 50 | `not`, prefix |
| 60 | the comparisons, non-associative |

Primaries sit above them. That is a precedence climber's table, and core v2
is one. The interpreter gives core v2:

- the operator table above, as core v2's signature;
- a **primary hook**: the command sub-parser (verb, then arguments until a
  token that cannot be one), and `( Sequence )`;
- the token stream of 3.1, as core v2's input alphabet. Each token carries
  its span in the source, so that a diagnostic points at the word.

It takes back a term: core v2's tree, whose leaves are commands and literals
and whose inner nodes are the operators. `command_pipeline`, the spine's
stand-in for the free-monad term, goes, and the fold walks core v2's term
instead (3.3).

### 3.3 The fold

The interpreter is a fold over the term, through `functional`'s `result` and
`maybe` (promise 5: "the interpreter folds its pipelines through
functional's result and maybe"). Each node's rule is in section 5.

- **Evaluation walks the arena** with `d_parse_expr_walk` and its own
  frames, one per node entered.
  - `enter` is given the node's parent, and the evaluator's frame for the
    parent says what came before.
  - `enter` answers `D_PARSE_EXPR_WALK_SKIP` for an operand that need not
    run: under an `and` whose earlier operand was false, under an `or`
    whose earlier operand was true, and for a pipeline's later stages once
    a stage has failed. So `f1 | b ; d` still runs `d`, and
    short-circuiting needs nothing of core v2 past the term's layout.
  - `D_PARSE_EXPR_WALK_STOP` ends the whole walk. The interpreter uses it
    only to abandon a script.
- **Commands are resolved through the registry** (section 6), not the
  spine's `command_registry` of first-match names. A djin's commands are the
  registry's.

### 3.4 Two faces

Promise 6 asks that each djin command be tested both as a call and in the
language. The C++ library exposes the fold's steps as calls:

- `interp::parse(text)`;
- `interp::evaluate(term, context)`;
- `interp::run(djin, command, argv, input)`.

The suites hold every command run through the language to the same command
run as a call.


## 4. The grammar: what `djinterp.peg` got wrong

Found reading the grammar against the reader, the kit's rulings and the
commands that exist. The fixes are in the grammar since 11-11. The numbers
are this note's.

| # | Defect | Example | Fix |
|---|---|---|---|
| P1 | The sequence's note says a newline separates pipelines, but `Skip` discards `Newline` before the syntactic grammar sees it. | `a` newline `b` is `a b`: one command with an operand. | A `NEWLINE` token, read as `;`. `Skip` loses `Newline`. |
| P2 | `--name=` cannot be lexed: `OptionValue` needs a character, and `=` is in no token. | `--note=` is a lexical error; R7 and cli-taken 33 make it the empty value. | `LONG_OPTION <- "--" OptionName ("=" OptionValue?)?`. |
| P3 | `-1` is a `SHORT_OPTION` (tried first), so `NUMBER`'s `"-"?` never matches, and `-1.5` lexes as `-1` and `.5`. | `size > -1` compares to the command `-1`. | A word that is wholly a number is a `NUMBER`, tried before `SHORT_OPTION`. In argument position both are handed to the command as written (decision 2), so `ls -1` is unchanged. |
| P4 | A token in word shape has no end check, so a word splits where a token ends. | `2file` is `2` and `file`; `-x.txt` is `-x` and `.txt`. | `!Joins` after `NUMBER` and `SHORT_OPTION`; otherwise the whole is a `WORD`. |
| P5 | `=` cannot be in a word. | `bitwisedom set s.json gen.level=c11`, `make CC=gcc` and `dd if=x` cannot be written. | A word takes `=`. `COMPARE` and the keywords are tokens only standing alone, as `and` already was. |
| P6 | `#` begins a comment anywhere. | `C#` and `http://h/p#frag` are cut. | A comment begins only where a token could: at the start of input or after a blank, as `sh` reads it. |
| P7 | A string has no escape for its own quote, and quoted and bare parts do not join into one word, which the reader already does. | `"say \"hi\""`; `--name="a b"c`. | `sh`'s rules. Single quotes are literal; in double quotes, `\"` and `\\`. A word is a run of bare and quoted parts. The reader and the grammar then agree. |
| P8 | A command takes no expression as an argument, so a predicate cannot be handed to a command that applies it. | `ls \| where size > 100` parses as `ls \| ((where size) > 100)`: the comparison is the stage, and `where` never sees it. | A block operand, `{ Sequence }`, evaluated by the command that takes it, once per item or once, as it says: `ls \| where { size > 100 }`. It is also awk's `pattern { action }` shape (section 7). |

**Amended 11-09, when the fixes were written.** Reading the grammar again to
write them changed two rows.

- **P1.** Inside brackets, a newline is read as it is outside, as `sh`
  reads it: `(a` newline `b)` is two commands. One rule is simpler, and a
  pipeline or predicate already continues on the next line after its
  operator. A separator may end a sequence, inside brackets too.
- **P6.** The grammar already begins a comment only where a token could,
  and the reader has no comments at all. The one word a comment cut is a
  quoted part before `#` (`"x"#y`), which P7's joined words make whole. So
  P6 is a note in the grammar, not a change to a rule.

The fixes also make `{` and `}` metacharacters, as `(` and `)` are, for P8.
The grammar was run, as written, by a small PEG interpreter over 81 cases.

**Decision 3. Precedence stays as the PEG has it**: `|` binds looser than
`and` and `or`, so `test -f x and test -r x | report` hands the predicate's
answer to `report`.

- `sh` binds `&&` looser than `|`. But the language's predicates are values,
  not exit statuses alone, and a predicate is most often a stage of its own.
- The keywords stay reserved at word boundaries: a command or operand named
  `and` is quoted.

P8 is a new construct, not a fix.

- **The alternative** is commands that filter by running a predicate command
  per item -- find's model -- with no block syntax.
- **Recommended: the block.** The same construct carries awk's shape and the
  language's own `where`, `each` and `sort-by`. A predicate command run per
  item cannot see the item it is run for without a convention outside the
  grammar.


## 5. Values and what the operators do

**Decision 4. A value is a tagged value.** It is one of:

- nothing;
- a truth value;
- an integer, 64-bit signed;
- a real, a double;
- text, UTF-8;
- bytes;
- a list;
- a record, with text keys in insertion order;
- a command's status alone.

dawk's `dvalue` moves into the library as "the tagged value and hash array"
(`desc.txt`'s table), and the interpreter's value is that module's, not a
second one.

- Its header comes on 11-13 and its code on 12-04. It carries the tags
  above: a list, integers apart from reals, a truth value, bytes apart from
  text, and keys kept in insertion order.
- A status needs no tag: the djin call carries it beside the value (6.2).
- awk's own rules stay in dawk, over the tags awk uses.
- Until the header lands, the interpreter's suites use a stand-in with the
  same tags.

| Construct | Meaning |
|---|---|
| `a ; b` | Run `a`, then `b`, whatever `a` came to; the sequence comes to `b`'s value and status, as `sh` does. |
| `a \| b` | `b` takes `a`'s value as its input. The first stage that fails ends the pipeline with its error (functional's `result`). The seed of a pipeline is nothing, or standard input's text where the program was given some. |
| `a and b` / `a or b` | Short-circuit on `a`'s truth: a truth value as it is; nothing, zero, an empty text, list or record false; a status true when it is 0. The construct comes to the value that decided it. |
| `not a` | The truth value opposite to `a`'s truth. |
| `a == b`, `!=`, `<`, `<=`, `>`, `>=` | On two numbers, numerically. On two texts, by code point. A number and a text that reads wholly as a number, numerically. Anything else: `==` and `!=` by structure, and the rest an error. Never chained (the grammar's rule). |
| `a =~ b` | `b` is a pattern, in `c/util/dregex`'s POSIX ERE; true when it matches anywhere in `a`'s text. |
| a command | Run with its words and the input value; it comes to its result value and status (6.3). |
| a literal | Its value: a number, or a text. A bare word is a command (PEG note 4). |

`=~` in 0.9:

- **It is ERE alone.** BRE comes with dsed, after 0.9.
- **It has no case-insensitive form.** Folding ASCII only would be half a
  feature over UTF-8 text. A later release may add one as a new member of
  the regex module's options.

The status the program exits with is the last construct's, by GRAMMAR
section 8: 0 done and yes, 1 done and no, 2 trouble. A predicate that comes
to false is 1, not an error.


## 6. Djins

### 6.1 What a djin is

A djin is a named, versioned unit of djinterp that a program can list,
describe, configure and run. It registers:

- **commands:** verbs, each with its options as the kit defines them;
- **settings:** its rows in the program's registry, each with its type,
  default and accepted values, and where its value came from;
- **descriptions:** desc facts.

dawk is one: awk is its command, and the awk machine's variables are its
settings. bitwisedom's model could be another.

### 6.2 The records

C records, as the kit's plugin records are, so that a djin is C or C++.
The same djin can later be a loadable module through the kit's plugin ABI
(cli-taken 13 to 15, 40).

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
    // its settings: its frame in the program's registry, named as the djin
    struct dj_option_env*         settings;
    // its description: desc facts, through desc's sink (11-13; void* until)
    int (*describe)(const struct dj_djin*, void* _facts);
};
```

- **The settings are an option_env frame.** The registry has no command
  face, so commands stay the djin's records, and the settings are its frame:
  - the frame's name is the djin's;
  - it is static and initialised before `main`;
  - `d_djin_register` registers it with `d_option_env_registry_add` at
    start, before the program has threads.
- **`dj_djin_call`** carries:
  - the program, for reports, `-q` and `-v`;
  - the command reached;
  - the input value, and a place for the output value;
  - the status.

  Its exact shape follows the tagged value's header (11-13).

**The set** (`c/modules/interp/djin.h`, in the tree since 11-11). A program
keeps its djins in a `dj_djin_set` over its registry, in room it gives, with
the names it keeps for its own commands. `d_djin_register` refuses, and
changes nothing, when:

- the record is out of shape: another ABI, a record smaller than the
  build's, a name that is not a lowercase identifier, a command that is not
  a lowercase word or is there twice, a summary of more than one line, or a
  frame named other than the djin;
- the name is one of the program's own commands (`list`, `describe`,
  `get`, `set`, and the kit's `help` and `completion`);
- a djin of the name is in the set, or another frame of it is in the
  registry;
- the set is full, or the registry refuses the frame.

A frame already in the registry, as a module's static frame may be, is
taken as it is. `d_djin_write_list` and `d_djin_write_description` write
the text `list` and `describe` print, through a sink: a line a djin, in
columns, and a registry frame no djin names after them, as "settings
only"; and a djin's commands and its settings, with the full name, kind,
default, knob and help of each.

### 6.3 Through the registry

**Decision 5. Djins reach the program only through the program's registry.**

- **At start, the program**:
  - registers its built-in djins;
  - reads the user's settings file;
  - imports the environment;
  - and takes the command line.

  Standing decides which binding holds, not the order of reading. The
  standing rises through `DEFAULT`, `PROGRAM`, `FILE`, `ENVIRONMENT`,
  `COMMAND_LINE` and `OVERRIDE`, and a binding remembers its source.
- **A setting's full name** is `djin[.scope].variable`. On the command line
  it is `--djin.variable=VALUE`, `--set djin.variable=VALUE`, or
  `--djin.flag` and `--no-djin.flag`.
  - The kit offers the registry only long options it does not define whose
    names hold a `.`, plus `--set` and `--no-X.Y`. So the registry never
    takes an option the kit defines, and a dotted name is never abbreviated.
  - A name that names nothing is a usage error, with the registry's
    nearest name as its "did you mean".
- **In the environment**, a setting is `DJINTERP_` and its full name,
  upper-cased, with `.` as `_`: `DJINTERP_NET_TIMEOUT_MS`.
  - A program built on the kit also imports its own name's prefix
    (`BITWISEDOM_`), after `DJINTERP_`, so the program's own name wins.
  - A program's own rows sit on the registry's root, so they read
    `BITWISEDOM_KEYS`.
- **The user's settings file** is `$XDG_CONFIG_HOME/djinterp/settings`, or
  else `~/.config/djinterp/settings`, and `%APPDATA%\djinterp\settings` on
  Windows. Its bindings are `FILE`'s.
  - A program may read a file of its own after it, and the program's file
    wins.
  - A statement that fails is reported by its line, and reading goes on at
    the next.
  - 0.8 has no system or project file. They come in 0.9, once a row can
    refuse a source, since a project file is the one an untrusted checkout
    could carry.

So `--help` (from desc, in December), `describe`, `get` and `set` all read
one source, and cannot disagree with the code (`desc.txt`, "three uses from
one source").

### 6.4 The 0.8 preview's command line

```
djinterp list                        the djins, a line each: name, version,
                                     summary
djinterp describe DJIN [COMMAND]     desc's facts, as text, or JSON
                                     (--format=json)
djinterp get [--show-origin] NAME... settings' values, as statements that
                                     load back; where each came from
djinterp set NAME=VALUE...           settings written to the user's file
djinterp DJIN COMMAND [ARGUMENT]...  a djin's command, run
djinterp help, completion, --version the kit's
```

Each djin is a command group, as `git`'s subcommands are. A djin named as a
built-in command is refused at registration, by name.

In the preview of 11-13, `describe` takes a djin alone, as text; `COMMAND`
and `--format=json` come with desc's sink. With no djin of the program's
own yet, `list` names the registry's frames, the kit's `cli` among them, as
"settings only".

- **`get`** prints statements, `net.timeout_ms = 5000`, one a line, with
  `d_option_env_format_quoted`. So its output can be pasted into a settings
  file.
  - An unset text prints as a comment, `# plugin_path is unset`, which loads
    as nothing.
  - With `--show-origin`, each value is followed by its source and the
    bindings under it:

    ```text
    net.timeout_ms = 5000  (command line)
        under it   10000   (file)
        default    30000   (D_CFG_NET_TIMEOUT_MS)
    ```

    0.8 shows the source's word. When a binding carries its origin (the
    file and line, or the variable's name), 0.9 shows that too.
- **`set`** writes each statement to the user's settings file, with
  `d_option_env_format_quoted`. Its bindings are `FILE`'s on the next run.
  - A one-run override is `--set NAME=VALUE`.
  - In the 0.9 interpreter's prompt, `set` binds from `COMMAND_LINE` for the
    session instead.
- **`get` and `set` for a djin with a file of its own.** They take the
  registry's names and, for a djin whose settings come from a file of its
  own, fall back to that file's raw path for anything the registry does not
  list. That is bitwisedom's rule (cli-taken 16), made the language's.

The 0.9 interpreter adds `djinterp -c 'LINE'`, `djinterp FILE` for scripts,
and standard input.

### 6.5 Loadable djins

Not in 0.8. In 0.9:

- A djin may be a module the kit loads, `djinterp-DJIN`, found beside the
  program or along its search list (cli-taken 14,
  `D_CFG_CLI_PLUGIN_SEARCH`). Its plugin record carries a `dj_djin`.
- A module of another version is refused, or loaded with a warning, as the
  program says (cli-taken 15, `D_CFG_CLI_PLUGIN_STRICT_VERSION`).
- `list` then names where each came from.

**The search list is a setting that decides what code runs**, so its rows
refuse the environment when `D_CFG_CLI_PLUGIN_VARIABLE` is 0. A row names
the sources it refuses, and a binding from one fails by name.


## 7. awk

`desc.txt` settles the boundary: the djinterp language is native, and awk is
a dialect over a shared core. **They mix at module granularity**, never
inside an expression, so that awk's string-and-number duality does not leak
into the language's types.

- **awk is a djin, `awk`, in djinterp's registry.** dawk the tool keeps a
  registry of its own, `dawk`, whose environment prefix is `DAWK_`.
  - Its command is `run`, which takes a program -- text, or a file -- and
    its operands.
  - Its settings are the awk machine's own, spelled as rows: `awk.fs`,
    `awk.ofs`, `awk.rs` and `awk.ors`.
  - They map into the program once, at the start of a run, as `-v` assigns.
    A program's `FS` never writes back to its row: the FS bug of last time
    came from mixing the two.
- **In a pipeline, awk takes its input value as records.**
  - A text goes to dawk whole, and awk's `RS` splits it. So paragraphs
    (`RS = ""`) and a regular-expression `RS` work.
  - A list goes through a `dj_awk_source`, one record per element.
  - Output comes back as text, through dawk's output sink, each record
    ended by `ORS`.

  So `ls | awk run '{ print $1 }' | count` works on values the language
  already has.
- **A block handed to awk's `run` is an awk program**, read by dawk, not by
  the language's grammar. This is Racket's `#lang` boundary, at the size of
  a block.
  - When the reader meets `{` after `#lang awk`, it hands dawk the text
    after the brace.
  - dawk's block-end scanner finds the closing brace, and gives back its
    offset, or an error at the place awk's lexer reports it.
  - The reader gives core v2 that span as the block's raw text. The
    interpreter counts no braces in awk.

**Decision 6. The interpreter calls dawk only through its implemented hooks**:

- `dinterp.h`, with `d_awk_interp_new` and `d_awk_interp_run`;
- `dsource.h`, for records;
- `dhost.h`, for host functions;
- `dvalue.h`'s extern values.

`desc.txt` names these as the extension points the new engine keeps.

- They freeze with the command language on 12-11. After that, a member may
  be added, but not changed.
- The output sink, text input that keeps `RS`, and the block-end scanner
  join them by 12-04.


## 8. What the interpreter depends on

| On | What | Where it stands |
|---|---|---|
| expression core v2 | non-associative levels, a primary hook, a client token alphabet, diagnostics at the character, the term in an arena, the renderer | in the C core since 11-06 |
| expression core v2 | the walk's skip, for `and`, `or` and a pipeline's first failure | in the C core since 11-06 (3.3) |
| expression core v2 | the sub-parse, for blocks | in the C core since 11-11 |
| the grammar | P1 to P8 in `djinterp.peg`, its ABNF and its EBNF | in the tree since 11-11 |
| option_env and the registry | rows, frames, sources and standing; `take_option`; `format_quoted`; text knobs | in the tree since 11-10 |
| option_env and the registry | rows that refuse a source | 11-13 |
| option_env and the registry | a binding's origin, for `--show-origin` | 0.9, at the earliest |
| desc | its fact sink, for a djin's `describe` | 11-13 |
| desc | `--help` from desc facts, laid out by the kit's help formatter, so the kit's rulings on help (84, 57, 65, 66) hold in one place | December |
| the tagged value | its header, with the tags of decision 4 | 11-13; code 12-04 |
| `c/util/dregex` | POSIX ERE, for `=~` | in the tree |
| dawk | the hooks of decision 6, frozen; the output sink, RS-keeping text input and the block-end scanner | frozen 12-11; additions by 12-04 |
| the root | `D_KEYWORD_INTERP` and `NS_INTERP` in `djinterp.h` | 11-13 |
| the build | `D_CORE_API`, the core's export switch, for the kit's shared core (cli-taken 13) | proposed; settled by 11-13 |


## 9. Schedule

| When | What |
|---|---|
| Fri 11-06 | This note agreed or amended with the designs it depends on. *Core v2 agreed 11-05; the options and dawk 11-10.* |
| Mon 11-09 | The grammar's fixes; the spine moved to `interp`. |
| Wed 11-11 | The djin records (6.2): the set, registration into the registry, and the text that `list` and `describe` print. |
| Fri 11-13 | The `djinterp` program on the records (`src/djinterp/tools/djinterp/`, its leaf `build/cmake/config/djinterp/tools/djinterp/`): `list`, `describe DJIN`, `get [--show-origin] [NAME]...` and `set NAME=VALUE...`. The kit's settings on the knob-and-option pattern (`tools/cli/c/cli_settings.h`): the user's file, both imports, and the command line's settings. |
| Weeks 2 and 3 | The 0.8 preview (6.4) whole: `describe DJIN COMMAND` and `--format=json` on desc's sink, and a djin's commands run. The interpreter's skeleton on core v2 -- sequences and pipelines -- behind it. |
| Week 4 | The preview's suites and docs; this note's December half, refined. |
| December | Predicates, blocks and values; djins through the interpreter; awk run by dawk; the language frozen 12-11; suites, docs and the man page by 12-18. |

If checkpoint 2 (11-15) finds the interpreter a week behind, the cut is the
roadmap's trigger 1: the preview shrinks to this note and a skeleton.
