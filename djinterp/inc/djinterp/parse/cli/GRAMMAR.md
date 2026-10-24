# The djinterp command line

One grammar for every djinterp program -- `bitwisedom`, `vparse`, and the
ones after them -- written so that the same command line means the same
thing in sh, bash, zsh, fish, cmd.exe and PowerShell, on Linux, the BSDs,
macOS and Windows. The argument parser (`parse/cli/arg_parser/`) reads it;
the CLI kit beside this file (`parse/cli/`, the `d_cli_` library and its
C++ face) installs what every program shares, so that a program only
declares its own commands.

It is built from three sources and departs from them only where they
disagree with each other or with a shell:

- POSIX.1-2017, XBD 12.1 and 12.2: the utility argument syntax and its
  fourteen guidelines -- short options, `--`, `-` for a standard stream;
- the GNU coding standards: long options, `--help` and `--version`, the
  shape of a diagnostic, and the exit status of a program that answers a
  question (grep, diff, cmp);
- the Microsoft C runtime's rules for splitting a command line into `argv`,
  which every Windows program built with any toolchain is given.

Sections 1 to 8 are the rules; section 9 is the grammar itself, in ABNF;
sections 10 and 11 are the grammars of `bitwisedom` and `vparse`.

---

## 1. The shape of a command line

```
program [global-option]... command [option | operand]... [-- operand...]
```

- **R1.** A program does one thing per run, named by its **command**: a
  lowercase verb, `gen`, `check`, `run`. A command may have subcommands,
  one level deep and no more: `bitwisedom example list`.
- **R2.** **Options may come anywhere after the program's name** -- before
  the command, after it, between operands -- as GNU programs take them.
  A global option (section 6) is accepted before the command or after it.
- **R3.** **`--` ends the options.** Every argument after it is an operand,
  even one that starts with `-`. `--` itself is not an operand. The command
  comes before it: `bitwisedom -- gen` is a usage error, not `gen`.
- **R4.** Operands are files, names and values, in the order the command's
  usage gives them. Their meaning never depends on where the options fall.

A program with no command is a usage error (section 8): it prints its
usage and the list of commands to standard error and exits 2. `PROGRAM
--help` prints the whole help to standard output and exits 0.

## 2. Options

| form | example | means |
|---|---|---|
| `--name` | `--quiet` | a flag |
| `--name=VALUE` | `--level=c11` | an option and its value |
| `--name VALUE` | `--level c11` | the same, when the option takes a value |
| `-x` | `-q` | a short flag |
| `-xyz` | `-qv` | several short flags at once |
| `-xVALUE`, `-x VALUE` | `-ofile.h`, `-o file.h` | a short option and its value |
| `--no-name` | `--no-banner` | a flag that is on by default, turned off |

- **R5.** A **long name** is lowercase ASCII letters, digits and single
  hyphens between them, starting with a letter: `--self-test`,
  `--int-suffix`. Long names are never abbreviated: `--verb` is not
  `--verbose`, so a script written today runs the same after an option is
  added tomorrow. A name that does not exist is refused, and the nearest
  one that does is suggested.
- **R6.** A **short name** is one ASCII letter, upper or lower case, and
  never a digit -- so `-1`, `-0x10` and `-.5` are always words: negative
  numbers, as an operand or as the value of the option before them.
- **R7.** An option's value is **required or absent, never optional** when
  written as a separate argument. An option whose value may be left out
  takes it only after `=`: `--color` means `--color=always`, and in
  `--color never` the word `never` is an operand. **`--name=` is the empty
  value**, as getopt_long reads it -- `--note=` clears a note -- and never
  takes the argument after it.
- **R8.** An option given twice, where it takes one value, **takes the
  last**: `--level=c99 --level=c11` is c11, as GNU programs do, so that a
  script may append an override to a command line it was given. An option
  that collects values says so in its help, and takes every one in order,
  **one each time it is given**: `-D A -D B`; in `-D A B`, `B` is an
  operand.
- **R9.** Options are case-sensitive. Commands, enumerated values and
  long names are lowercase.
- **R10.** On Windows, `/?` is `--help`, as Windows users type it. No other
  option has a `/` form: on every system `/x` is a path.

## 3. Operands

- **R11.** **`-` is a standard stream**: standard input where the command
  reads, standard output where it writes. A file whose name is `-` is
  written `./-`.
- **R12.** An operand that starts with `-` follows `--`, or is written so
  that it does not -- `./-file`.
- **R13.** An operand that names something **other than a file** says so
  with a prefix and a colon, as git, docker and scp do: `example:uart_cr`
  is the built-in example named `uart_cr`. A prefix is two letters or more,
  so a Windows drive (`C:`) is never one; a file whose name is such a word
  is written `./example:x`.
- **R14.** **Response files**, `@FILE`, are read only by a program that
  says so in its help: each argument in FILE is put in the `@FILE`'s place,
  split as the platform's own tools split them -- the POSIX shell's quoting
  where GCC reads them, the C runtime's where cl reads them. PowerShell
  reads an unquoted `@` as splatting: there, write `'@FILE'`.

## 4. Values

| kind | written | notes |
|---|---|---|
| integer | `42`, `0x2A`, `0o52`, `0b101010`, `1_000` | decimal unless prefixed; a leading zero is not octal (`010` is ten); `-` before a negative one |
| boolean | `true` `false`, `yes` `no`, `on` `off`, `1` `0` | any case |
| enumeration | `c11`, `cpp17`, `twos_complement` | one of the words the help lists, which are the words the program's files use |
| range | `4..6`, `6..4`, `5` | inclusive, either way round; one number is one bit |
| setting | `KEY=VALUE` | `grid.lines=false`; keys are dotted paths |
| list | the option repeated: `-D A -D B` | a comma-separated list is accepted where the help says so, never required |
| path | as the platform writes it | `/` is accepted on Windows as well as `\` |
| text | as the shell quotes it | see section 5 |

- **R15.** **Text is UTF-8.** On Windows the program reads its true command
  line, `GetCommandLineW()`, so every argument arrives whole whatever the
  console's code page, and is converted to UTF-8; paths are converted back
  where a file is opened. A command that is a module is given its
  arguments by the program, already UTF-8, and never reads the command
  line again.
- **R16.** A value is never interpreted by locale: `1.5` is one and a half
  under every locale, as the argument parser reads it.

## 5. What a shell does to an argument

The grammar's own syntax is written only in characters that no shell
gives a meaning to where the grammar puts them:

```
A-Z a-z 0-9   -   _   .   /   :   +   ,   and   =   after the first character
```

Everything else is the user's to quote, and an argument that holds them --
an expression, a macro's body, a grammar given inline -- is quoted, or read
from a file or from standard input instead. Every command that takes such
text has a way to read it from a file or `-`.

| character | sh, bash | zsh | fish | cmd.exe | PowerShell |
|---|---|---|---|---|---|
| space, tab | separate | separate | separate | separate | separate |
| `'` `"` | quote | quote | quote | `"` quotes | quote |
| `\` | escapes | escapes | escapes | literal | literal |
| `` ` `` | command substitution | command substitution | literal | literal | escapes |
| `$` | expands | expands | expands | literal | expands |
| `%` | literal | literal | literal | `%NAME%` expands | literal |
| `!` | history, interactively | history, interactively | literal | expands with delayed expansion | literal |
| `^` | literal | glob with `EXTENDED_GLOB` | literal | escapes | literal |
| `*` `?` `[ ]` | glob | glob, and an error when nothing matches | `*` globs; `?` too, unless `qmark-noglob` | literal | literal |
| `{ }` | brace expansion | brace expansion | brace expansion | literal | script block |
| `( )` | subshell | subshell, glob qualifier | command substitution | grouping | subexpression |
| `<` `>` `\|` `&` `;` | redirect, pipe, background, separate | the same | the same | the same | the same (`<` reserved) |
| `#` at a word's start | comment | comment | comment | literal | comment |
| `~` at a word's start | home | home | home | literal | home, in a path |
| `=` at a word's start | literal | `=cmd` expands to a path | literal | separates, in a batch file | literal |
| `@` at a word's start | literal | literal | literal | literal | splatting |
| `,` | literal | literal | literal | separates, in a batch file | makes an array: the parts become separate arguments |
| `--%` | literal | literal | literal | literal | stops parsing |

Two consequences shape the rest:

- **Lists are repeated options** (R8), because PowerShell splits `a,b`.
- **`--` reaches the program** from every shell named here, PowerShell
  included, as `git checkout -- file` shows daily. A PowerShell *script*
  (`.ps1`) standing in for a program consumes it: the programs are
  executables, and their installers put no script in front of them.

## 6. What every program has

The kit installs these; a program's own options never reuse their names.

| option | does |
|---|---|
| `-h`, `--help` | the help of the program or of the command it follows, to standard output; exit 0 |
| `--version` | `PROGRAM VERSION` on the first line, as GNU programs print it; with `--verbose`, the build too; exit 0 |
| `-q`, `--quiet` | only errors on standard error |
| `-v`, `--verbose` | more on standard error; given twice, more again |
| `--color[=WHEN]`, `--colour[=WHEN]` | `auto` (the default), `always` or `never` |

| command | does |
|---|---|
| `help [COMMAND]` | the same as `[COMMAND] --help` |
| `completion SHELL` | a completion script for `bash`, `zsh`, `fish` or `powershell`, to standard output |
| `gui [ARGUMENT]...` | the program's window, where it has one (section 7) |

**Help** is wrapped to the terminal's width when standard output is a
terminal, and to 80 columns otherwise, so that what a script captures does
not depend on the window it ran in.

**Colour.** `auto` colours a stream only when it is a terminal, the
terminal is not `TERM=dumb`, and `NO_COLOR` is unset or empty
(no-color.org); on Windows, only where the console takes ANSI sequences,
which the program asks for. `--color=always` colours in every case,
`NO_COLOR` notwithstanding.

## 7. The window

A program with a window has it as a **plugin**: a module loaded only by
`PROGRAM gui`, so the program itself carries none of it -- no window
system, no GPU, no fonts -- and a system with no display can install the
program alone.

- `PROGRAM gui [ARGUMENT]...` loads the module and gives it every argument
  after `gui`. The module reads them with the same parser and the same
  grammar, so `PROGRAM gui --help` is the window's own help, and an option
  the window shares with a command -- a sheet to open, a grammar to run --
  is spelled as the command spells it.
- The module is `PROGRAM-gui` with the platform's suffix for a loadable
  module -- `bitwisedom-gui.dll`, `bitwisedom-gui.so` -- and is looked for
  in order: the directories in `PROGRAM_PLUGIN_PATH` (the program's name
  upper-cased, `-` as `_`), separated as `PATH` is; the program's own
  directory; and `../lib/PROGRAM/` from it, where a POSIX installation
  puts it. A program file that carries a tag after the program's name --
  `bitwisedom-linux64`, `bitwisedom-win64.exe`, as a build's `bin/`
  directory names them so that every platform's builds can share it -- has
  its module carry the same tag, `bitwisedom-gui-linux64.so`, and that name
  is looked for first in each directory.
- A module belongs to one program at one version. One made for another
  program, or for another version, is refused by name, with both versions,
  and the program exits 2.
- The window exits as a command does: 0 when it closed normally.

## 8. Output, diagnostics and exit status

**Streams.**

- **R17.** **Standard output holds the result and nothing else**: the
  header, the listing, the JSON. Everything said *about* the work --
  errors, warnings, notes, progress -- goes to standard error.
- **R18.** Text is UTF-8, its lines ended by LF on every system. On a
  Windows console it is written through the console's own wide interface,
  so it reads correctly whatever the code page; redirected, it is UTF-8
  bytes.
- **R19.** A command whose result has structure offers
  **`--format=json`** beside its text. The JSON is documented with the
  command, and only grows: a key once written keeps its meaning.
- **R20.** A file the program writes is **written whole or not at all**:
  into a temporary file beside it, then renamed over it. `-o -` writes
  standard output.
- **R21.** A program never prompts, and never reads the terminal unless a
  command reads standard input and none was redirected -- then it reads
  the terminal, as `cat` does.

**Diagnostics** take the GNU shape:

```
PROGRAM: error: MESSAGE
PROGRAM: warning: MESSAGE
FILE:LINE:COLUMN: error: MESSAGE
FILE:LINE: warning: MESSAGE
```

`PROGRAM` is the program's own name, not the path it was run by. A message
starts in lower case and has no full stop at its end. Where it is about a
place in a file, the place comes first, as compilers write it, which every
editor and IDE turns into a link. With colour, the severity is coloured as
GCC colours it.

A usage error is one line and a pointer:

```
bitwisedom: error: gen: SHEET is required
Try 'bitwisedom gen --help' for more information.
```

**Exit status.** Three values, as grep, diff and cmp give them:

| status | means | for example |
|---|---|---|
| 0 | **done**: it did what was asked, and the answer, if one was asked, is yes | a header written; an input that matched |
| 1 | **done, no**: it did what was asked, and the answer is no | a sheet with errors (the header still written, without them); an input the grammar refuses |
| 2 | **trouble**: it could not do what was asked | an unknown option; a missing operand; a file that cannot be read or written; a sheet or a grammar that is not one; a plugin that is missing |

Help and version are 0 -- when they were written: an output that refuses
them, as a full disk does, is trouble, and said, as `cannot write standard
output: REASON`. So is running out of memory, whatever was asked: the
program says `out of memory`, and never what the want of memory made look
true -- a command that is not there, a module not installed. A program
killed by a signal leaves the status the shell gives it, 128 and the
signal's number; one that catches `SIGINT`
to remove a temporary file raises it again afterwards, so the shell still
sees the interrupt. A command documents what it answers; a command that
answers nothing never exits 1.

## 9. The grammar

In ABNF (RFC 5234), with RFC 7405's `%s` for a case-sensitive string. The
command line is its arguments, each ended by NUL: `argv` as `execve`
passes it, `/proc/PID/cmdline` as Linux shows it, `xargs -0` as it reads;
on Windows, `GetCommandLineW()` split by the C runtime's rules (the
argument parser's `d_arg_parser_parse_wide`), each in UTF-8.

```abnf
; ------------------------------------------------------------ the line
command-line   = program END *( argument END )
program        = *OCTET-NN                ; argv[0]; its file name, less
                                          ; any .exe, is PROGRAM
END            = %x00
OCTET-NN       = %x01-FF

; ------------------------------------------------------ one argument
; an argument is the first of these it matches
argument       = end-of-options
               / long-option
               / std-stream
               / short-options
               / response-file            ; where the program reads them
               / windows-help             ; on Windows alone
               / word

end-of-options = %s"--"
long-option    = %s"--" long-name [ "=" value ]
long-name      = LOWER *( [ "-" ] ( LOWER / DIGIT ) )
std-stream     = "-"
short-options  = "-" LETTER *OCTET-NN     ; read by R22
response-file  = "@" 1*OCTET-NN
windows-help   = %s"/?"
word           = *OCTET-NN                ; anything else: a command, an
                                          ; operand, an option's value
value          = *OCTET-NN

LOWER          = %x61-7A                  ; a-z
LETTER         = %x41-5A / %x61-7A        ; A-Z a-z
DIGIT          = %x30-39

; ------------------------------------------- the words of the grammar
command-name   = LOWER *( [ "-" ] ( LOWER / DIGIT ) )
enum-word      = LOWER *( LOWER / DIGIT / "_" )
boolean        = "true" / "false" / "yes" / "no" / "on" / "off"
               / "1" / "0"                ; ABNF strings: any case
integer        = [ "-" ] ( decimal / hex / octal / binary )
decimal        = DIGIT *( [ "_" ] DIGIT )
hex            = "0x" HEXDIG *( [ "_" ] HEXDIG )
octal          = "0o" %x30-37 *( [ "_" ] %x30-37 )
binary         = "0b" BIT *( [ "_" ] BIT )
range          = integer [ ".." integer ]
setting        = key "=" value
key            = enum-word *( "." enum-word )
named-operand  = prefix ":" 1*OCTET-NN    ; R13
prefix         = 2*LOWER
when           = "auto" / "always" / "never"
```

The arguments then form a sequence, read left to right against the
definitions of the command reached so far -- its options, and its parents'
global ones:

- **R22.** `-` and a letter starts **short options**, read a letter at a
  time: a flag's letter is followed by the next letter or by nothing; the
  letter of an option that takes a value is followed by its value -- the
  rest of the argument, or, when nothing is left, the next argument whole.
- **R23.** A long option that takes a value and has no `=VALUE` takes the
  next argument whole as its value, whatever it looks like: `--offset -3`,
  `--out --` (a file named `--`).
- **R24.** A word that is not an option's value is the **next command**
  while the command reached so far has subcommands and none has been
  given, and otherwise the **next operand**. A word that is neither --
  no such command, no room for another operand -- is a usage error, and the
  nearest command is suggested.
- **R25.** After `--`, every argument is an operand (R3); `@FILE` and `/?`
  are read as themselves there.

The standard options of section 6, in the same notation:

```abnf
standard-option = help-option / version-option / quiet-option
                / verbose-option / colour-option
help-option     = %s"-h" / %s"--help"
version-option  = %s"--version"
quiet-option    = %s"-q" / %s"--quiet"
verbose-option  = %s"-v" / %s"--verbose"
colour-option   = ( %s"--color" / %s"--colour" ) [ "=" when ]
standard-command = %s"help" [ END command-name ]
                 / %s"completion" END shell
                 / %s"gui" *( END argument )
shell           = %s"bash" / %s"zsh" / %s"fish" / %s"powershell"
```

## 10. bitwisedom

```
bitwisedom [global-option]... COMMAND ...
```

A **SHEET** operand is any of the three shapes of JSON (`docs/bitwisedom/
json.md`): a sheet file, a bitfield or an export; `-` for standard input;
or `example:NAME` for a built-in example (R13). A **ROW** is a symbol's
macro name (`UART_CR_EN`), its own name where only one row has it (`EN`),
or a group's name -- the rule a sheet's expressions name rows by.

### Reading a sheet

| command | does | answers |
|---|---|---|
| `gen SHEET` | writes the header: C or C++, macros or constants, at a language level | 1 when the sheet has errors (the header is written without them) |
| `check SHEET...` | lists every issue of every sheet; writes nothing else | 1 when a sheet has errors |
| `show SHEET` | every row as it resolves: macro name, bits, value, the integer C knows it by | -- |
| `decode SHEET VALUE...` | the symbols a value holds: each bit set, each field's value and the name it has | 1 when a value sets bits no symbol names |
| `eval SHEET EXPRESSION...` | evaluates C expressions over the sheet's symbols, every way | 1 when an expression does not evaluate |
| `convert SHEET` | writes the sheet in another shape: `--to sheet`, `bitfield` or `export` | -- |
| `examples` | lists the built-in examples, by shelf | -- |

`gen`'s options are `bitwisedom-gen`'s, unchanged:

| option | |
|---|---|
| `-o, --out=FILE` | the header's file; standard output by default |
| `--self-test=FILE` | a C or C++ file that compiles only if every macro has the sheet's value |
| `--bitfield=FILE`, `--export=FILE` | the sheet as a bitfield, or everything about it, as well |
| `--lang=c\|cpp`, `--form=macros\|constants`, `--style=djinterp\|plain` | |
| `--level=LEVEL` | `c99` `c11` `c17` `c23` `cpp98` `cpp11` `cpp14` `cpp17` `cpp20` `cpp23` |
| `--word=8\|16\|32\|64` | a wide sheet's array words |
| `--date=YYYY.MM.DD` | the banner's date; today's by default, so a reproducible build gives it |

### Making and changing a sheet

| command | does |
|---|---|
| `new [--width=N] [--title=T] [--prefix=P] [--suffix=S]` | a new sheet, as File > New makes one |
| `import FILE...` | a sheet from C: each `#define`, read as the preprocessor reads it, a row |
| `add SHEET NAME DEFINITION` | a row, defined as a bitfield's row is |
| `change SHEET ROW [DEFINITION]` | a row: its definition, the parts of the one it has, or anything else the inspector shows |
| `duplicate SHEET ROW...` | rows, each copied just after itself, named as it is with `_COPY` after |
| `remove SHEET ROW...` | rows, and everything in them |
| `rename SHEET ROW NAME` | a row; every expression that names it follows |
| `move SHEET ROW WHERE` | `--up`, `--down`, `--before=ROW`, `--after=ROW` or `--into=GROUP` |
| `sort SHEET [KEY]` | each group's rows in the order KEY sorts them, made the order the header follows; by default, the order the window shows |
| `set SHEET KEY=VALUE...` | settings, by the paths the window's Options show |
| `get SHEET [KEY]...` | settings, every one without a KEY |
| `op SHEET OPERATION [OPERAND]` | previews an operation -- a shift, a rotation, a byte swap, arithmetic -- over rows; `--apply` applies it |

A command that changes a sheet **writes it back where it was read**,
whole or not at all (R20), in the shape it was read in; `-o FILE` writes it
elsewhere and leaves the original as it was; a sheet read from standard
input is written to standard output; `-n, --dry-run` writes nothing and
says what would change. Every issue the change brings is said, at the
sheet. **A change that would give the sheet an error it does not have** --
an expression naming a row that is gone, a value whose range is no longer
one -- **is refused**, nothing written, and the command exits 2; `-f,
--force` writes it anyway. The window makes any change and shows its
issues, since it can undo one; a command line cannot. A warning -- bits
two rows share, a number its field cannot hold -- never stops a change.
`op --apply` writes the sheet, which is its result; the table it previews
goes to standard error then, with `-v` or `-n`.

`add`'s DEFINITION is a bitfield row's keys as options, one definition
each, refused as the bitfield refuses a row with two or none; `change`
takes one, to replace the row's, or none:

| definition | options |
|---|---|
| bit | `--bit=N` |
| range | `--bits=RANGE`, or `--lsb=N [--width=N]` |
| value in a range | `--field=ROW --number=N [--encoding=E] [--bias=N]` |
| literal | `--value=N` |
| expression | `--expr=TEXT` |
| macro | `--body=TEXT [--params=TEXT] [--callable]` |
| group | `--group` |

and any of `--note=TEXT`, `--short=TEXT`, `--prefix=TEXT`,
`--suffix=TEXT`, `--colour=COLOUR` (`#RRGGBB`, `#RRGGBBAA`, `auto` or
`none`), `--emit` or `--no-emit`, `--radix=WORD`, `--int-suffix=WORD`,
`--digits=N`, and for a group or a range `--isolate`, `--name-children`
and `--exclusive`, each with its `--no-` form; `add` takes `--in=GROUP`
too. `change` given a part alone -- `--width`, `--number`, `--encoding`,
`--bias`, `--params`, `--callable` -- changes that part of the definition
the row has, and keeps the rest of it; whatever it is not given, it keeps.
An expression or a macro's body, which the shells would take apart, may be
read from a file instead, or from `-`: `--expr-file=FILE`,
`--body-file=FILE`.

### The window

`bitwisedom gui [SHEET]...` opens the window on the sheets named, or where
it was left. A SHEET is read as the commands read one -- `example:NAME`, or
a file the window opens, a header to import among them -- except `-`,
since a window cannot read standard input; one that cannot be opened is
refused before the window opens, and the run is 2. Its own options are in
`bitwisedom gui --help`.

### Compatibility

`bitwisedom-gen ARGUMENTS` is `bitwisedom gen ARGUMENTS` for the arguments
`bitwisedom-gen` took, and its exit status is the same; `--example NAME` is
`example:NAME`, `--list-examples` is `bitwisedom examples`, and
`--save-example NAME FILE` is `bitwisedom convert example:NAME -o FILE`.
It is kept so that build scripts written for it run unchanged.

## 11. vparse

```
vparse [global-option]... COMMAND ...
```

A **GRAMMAR** is a file in vparse's notation, EBNF (`--ebnf`) or Ford's PEG
notation (`--peg`), or `-`. An **INPUT** is a file, `-`, or the text of
`--text=TEXT`.

| command | does | answers |
|---|---|---|
| `run GRAMMAR [INPUT]` | reads the grammar, builds its parser and runs it over the input; prints the result, and any part asked for with `--show=PART` | 1 when the input does not match |
| `check GRAMMAR` | reads and builds the grammar; says which family it runs on, which table, and what the analysis found | 1 when it cannot be built |
| `show GRAMMAR [INPUT] PART` | one part of a run: `summary`, `grammar`, `reading`, `rules`, `plan`, `program`, `matches`, `heat` or `trace`, as the workbench's tabs show them | -- |
| `diff RUN RUN` | two saved runs, part by part, as the workbench compares them | 1 when they differ |
| `runs [DIR]` | the saved runs under DIR | -- |
| `gui [ARGUMENT]...` | the workbench | -- |

The options that choose the parser are the workbench's, spelled the same
in `run`, `check`, `show` and `gui`: `--family=auto|peg|lr|earley`,
`--table=auto|lalr|lr|slr`, `--lookahead=N` (`-k N`), `--memoize`,
`--tokens=RULE`, `--skip=RULE`, `--steps=N`, `--no-trace`. A run saved with
`--save=DIR` is the folder the workbench saves, and opens there.
