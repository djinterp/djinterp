# vparse

> **Where this file is, and what its paths mean (lane F, 2026-11-05).**
> This is the tool's README as it stood on 2026.10.27, restored after the
> restructure of 2026-11-03 left it out of the tree. Ruling rs c put a doc
> about a whole module in `docs/djinterp/<module>/`, and the build names it
> here (`VPARSE_README`), where `tests_cli_json` holds the command line's
> JSON to its tables. It is kept as written; read its paths through this
> table.
>
> | the README says | the tree has |
> |---|---|
> | `src/djinterp/tools/vparse/README.md`, "this file" | `docs/djinterp/tools/vparse/README.md` |
> | `build/cmake/djinterp/tools/vparse/` | `build/cmake/config/djinterp/tools/vparse/` |
> | `build/ui/vparse/` | `build/cmake/config/ui/vparse/` |
> | `build/cmake/djinterp/tools/cli/DjinterpCli.cmake` | `build/cmake/config/djinterp/tools/cli/DjinterpCli.cmake` |
> | `src/djinterp/parse/cli/arg_parser/c/` | `src/djinterp/c/modules/parse/cli/arg_parser/` |
> | `inc/`, `src/`, `tests/djinterp/parsegen/vparse` | the same under `djinterp/cpp/modules/parsegen/vparse` |
>
> `vparse-AGENTS.md`, the library's guide, and the kit's `README.md` and
> `GRAMMAR.md` it points to are not in the tree either.

vparse is djinterp's parser generator as a program and a window over one
model.

- **`vparse`**, the command line: judge a grammar, write out its program,
  match inputs against it, print any stage of a run as text or JSON, and save
  runs and run them again; measure a grammar over many inputs, make inputs
  from the grammar itself, and run two grammars over the same inputs to find
  where they differ; and have a program call its rules through a memo. It
  needs nothing but djinterp: no uxoxo, no Dear ImGui, no display.
- **`vparse gui`**, the window, the workbench on uxoxo and Dear ImGui -- a
  module the command line loads only when `gui` is run, built outside
  djinterp (`build/ui/vparse`, ruling 0.3 and the kit's GRAMMAR, section
  7): write a grammar (in vparse's notation or in EBNF) and an input, press
  Run, and
  inspect every stage: how the grammar was read, the rules it became, the
  generator's plan, the program, the rules that matched, where the parser
  spent its work, and the machine's trace step by step. Every run is kept for
  the session, any two runs can be compared part by part, and runs can be
  saved to folders and loaded back.

The command line and the window drive the same model, so a part `vparse` prints is the part the window
shows, row for row, and a run either one saves the other loads. The model
drives djinterp's own vparse, `djinterp::parsegen::vparse`, through its
session: a grammar is built and run only once the session has accepted it,
and refused in the session's words when it has not.

It is one of djinterp's tools, laid out as bitwisedom is: the program in
djinterp's tree, its window beside it under `inc/ui`, `src/ui`, `tests/ui`
and `build/ui`.

## Where it is

```
inc/djinterp/tools/vparse/          the model's headers
    run.hpp                         a run: its request, its stages, its parts
    store.hpp                       saved runs: folders, the session index,
                                    files named in UTF-8
    heat.hpp                        the heat map: where a run spent its work
src/djinterp/tools/vparse/          the model
    run.cpp  store.cpp  heat.cpp
    cli/                            the command line
        cli.hpp  cli.cpp            the program as a function: the command
                                    tree, and a run of it
        commands.hpp  commands.cpp  the subcommands: their options, and what
                                    each one over a grammar or a saved run
                                    does
        measures.cpp                bench, corpus and compare: a grammar over
                                    many inputs
        common.hpp  common.cpp      what the subcommands share: reading their
                                    options and their files, and ending
        report.hpp  report.cpp      what is printed: aligned text, and JSON
        main.cpp                    the program `vparse`
    README.md                       this file
tests/djinterp/tools/vparse/
    tool_tests.hpp                  what the suites share: grammars, helpers,
                                    and the loop that runs a table of sections
    workbench_core_tests.cpp        the model
    cli_tests.cpp                   the command line, run in-process
build/cmake/djinterp/tools/vparse/  the build: CMakeLists.txt, CMakePresets.json,
                                    cmake/, tests/, and cl_standin/
inc/ui/vparse/                      the window's headers
    view.hpp                        the window's model, its face as a uxoxo
                                    template, and the update that applies an
                                    event
    elements.hpp                    the workbench's own elements: listing,
                                    tape, heat
    host.hpp                        the window, behind one interface
src/ui/vparse/                      the window
    view.cpp  elements.cpp          the face and the elements
    elements_imgui.cpp              their Dear ImGui renderers, and the theme
    host_win32.cpp                  Win32 and Direct3D 11
    host_glfw.cpp                   GLFW and OpenGL 3
    host_none.cpp                   no window
    plugin.cpp                      the module `vparse gui` loads
tests/ui/vparse/
    workbench_tests.cpp             the window's model and face, as values
    workbench_imgui_tests.cpp       the face drawn on Dear ImGui, no window
build/ui/vparse/                    the window's build: CMakeLists.txt,
                                    CMakePresets.json, cmake/
bin/tools/vparse/                   what the builds make
```

The namespaces are `vparse::workbench` for the model and the window, and
`vparse::cli` for the command line. The library it drives is djinterp's, in
`inc/`, `src/` and `tests/djinterp/parsegen/vparse`; its guide is
`inc/djinterp/parsegen/vparse/vparse-AGENTS.md`, and the session is
`session.hpp` there. The command line is made on djinterp's command-line
kit, `tools/cli` (its `README.md` and `GRAMMAR.md`), over djinterp's
argument parser, `parse/cli/arg_parser`, CLI11's design in C; the window,
a module, reads its arguments through the same kit, as a program the
command line made.

## Before the first build

- **djinterp** is this tree. The build finds djinterp's root from where it
  sits, five folders below it, and needs no setting. It needs the argument
  parser's sources, `src/djinterp/parse/cli/arg_parser/c/`, which are still
  being written, and the command-line kit's, `src/djinterp/tools/cli/c/`,
  with the kit's CMake module, `build/cmake/djinterp/tools/cli/
  DjinterpCli.cmake`, which makes it: the command line uses the parser's
  validators, its completion scripts and its response files, through the
  kit. The build tests the parser with its own suites when its test leaf is
  there; the kit's suites are run from the kit's own leaf, which names its
  testing libraries as this build names the programs' and so is not added
  here.
- **uxoxo** is needed for the window alone, and only the window's build,
  `build/ui/vparse`, looks for it: the tree of 2026.10.16 or later, two
  directories above djinterp's root -- as `uxoxo/uxoxo-project` or `uxoxo`
  -- or at `UXOXO_ROOT`, with its Dear ImGui under `inc/imgui` (uxoxo
  carries 1.92.8 there, laid out as Dear ImGui's repository is). uxoxo
  builds itself, from its `build/cmake/CMakeLists.txt`; the window's build
  adds that as a subproject. Configure checks for the build, the fifteen
  uxoxo headers the window includes and Dear ImGui, and names whatever is
  missing. djinterp's own build of vparse names no uxoxo.
- **Stale files.** A tree that was only ever unzipped over still holds the
  parse and parsegen C layer from before it moved into `c/`, which defines
  every function its successor does. This build never compiles those files,
  and configure warns while they are there, naming them. The retire scripts
  in djinterp's `_retired/` take them out of the way.

## Building

There are two builds, as bitwisedom has: djinterp's, of vparse and its
command line, which needs nothing but djinterp; and the window's, which
builds djinterp's within it and adds uxoxo and the module `vparse gui`
loads.

From a **Developer Command Prompt** on Windows (Ninja needs `cl` or
`clang-cl` on `PATH`):

```bat
cd /d <djinterp>\build\cmake\djinterp\tools\vparse
cmake --preset msvc
cmake --build --preset msvc-release
ctest --preset msvc-release
```

Off Windows:

```sh
cd <djinterp>/build/cmake/djinterp/tools/vparse
cmake --preset clang-strict
cmake --build --preset clang-strict
ctest --preset clang-strict
```

The window too, from its own build -- the same presets, uxoxo found two
directories above djinterp's root or named:

```sh
cd <djinterp>/build/ui/vparse
cmake --preset clang-strict [-DUXOXO_ROOT=/path/to/uxoxo]
cmake --build --preset clang-strict
ctest --preset clang-strict
```

| preset | generator | compiler |
|---|---|---|
| `msvc` | Ninja Multi-Config | `cl` |
| `clang-cl` | Ninja Multi-Config | `clang-cl` |
| `vs2026` | Visual Studio 18 2026 | MSVC, for the IDE, with no command prompt; the generator needs CMake 4.2 |
| `vs2026-clangcl` | Visual Studio 18 2026 | ClangCL toolset |
| `vs2022`, `vs2022-clangcl` | Visual Studio 17 2022 | the same two, for Visual Studio 2022 |
| `strict` | Ninja Multi-Config | `clang-cl`, warnings as errors |
| `gcc`, `clang` | Ninja Multi-Config | off Windows: Linux, WSL, CI |
| `clang-strict` | Ninja Multi-Config | Clang, warnings as errors |
| `mingw-cross` | Ninja Multi-Config | MinGW-w64 from Linux, for 64-bit Windows: `vparse.exe`, static, and, from the window's build, the module `vparse-gui-win64.dll`, the window Win32's. The tests are Windows programs, so they are off; with Wine, `-DVPARSE_BUILD_TESTS=ON` (the window's build: `-DVPARSE_UI_BUILD_TESTS=ON`) and `-DCMAKE_CROSSCOMPILING_EMULATOR=<wine64>` run them |

Each has `-debug` and `-release` build presets and, but for `mingw-cross`,
test presets (`strict` and `clang-strict` have one each, Release). A preset
builds under `out/<preset>` beside the presets, which `.gitignore` keeps out
of the repository.

Without presets:

```sh
cmake -S build/cmake/djinterp/tools/vparse -B <build>
cmake -S build/ui/vparse -B <build> [-DUXOXO_ROOT=<uxoxo>]
cmake --build <build> --config Release
ctest --test-dir <build> -C Release
```

A build stops at the first target that fails; `-- -k 0` on the build line
shows every failure at once, which is what a first build with a new compiler
wants.

### What it makes

The binaries go to `<djinterp>/bin/tools/vparse/`, named with the platform
they are for (`vparse-linux64`, `vparse-gui-win64.dll`, ...), and Debug's
under `debug/` there, as bitwisedom's do. Every build for one platform writes
the same files, so the last one built is the one there; `vparse gui` loads
the module beside the program, named with the program's tag:

| target | build | what |
|---|---|---|
| `djinterp::c_base` | both | the C root's `dstdint.c` and `sink_common.c` |
| `djinterp::parse_foundation` | both | djinterp's parse/parsegen C foundation, compiled as C at C99 |
| `djinterp::arg_parser` | both | djinterp's argument parser, at C99 |
| `djinterp::cli` | both | the command-line kit, at C99 |
| `djinterp::vparse` | both | djinterp's vparse: the PEG machine, the generator, both frontends, the LR family and the session |
| `vparse::workbench_core` | both | the model: runs, the store, the heat map. No uxoxo, no Dear ImGui |
| `vparse::cli` | both | the command line: its subcommands, over the model |
| `vparse` | both | the program `vparse` |
| `vparse::workbench_face` | the window's | the window's model and face, and the workbench's elements |
| `vparse::workbench_imgui` | the window's | the elements' Dear ImGui renderers, and the theme |
| `vparse-gui` | the window's | the module `vparse gui` loads |

uxoxo's own targets (`uxoxo::uxoxo`, `uxoxo::imgui`, and the backends it
finds) come from its build. `--version` prints the version of the build's
`project()` line, 0.3.0, and the module is built at the same: a module of
another version is refused by name.

### Settings

| variable | default | what |
|---|---|---|
| `DJINTERP_ROOT` | the tree the build is in | djinterp's root; set it only to build another tree's |
| `VPARSE_BUILD_CLI` | `ON` | `vparse`, the command line, and its suite |
| `VPARSE_BUILD_TESTS` | `ON` | the suites |
| `VPARSE_BUILD_DJINTERP_TESTS` | `ON` | djinterp's foundation, argument-parser and vparse suites too, from their own test leaves |
| `VPARSE_WARNINGS_AS_ERRORS` | `OFF` | `-Werror`, `/WX` |
| `VPARSE_CXX_STANDARD` | `20` | djinterp's vparse and its foundation's C++: 11, 14, 17, 20 or 23 |
| `VPARSE_TOOL_CXX_STANDARD` | `20` | the model and the command line: 17, 20 or 23 |
| `UXOXO_CXX_STANDARD` | `20` | uxoxo and the window, in the window's build: 17, 20 or 23 |
| `UXOXO_ROOT` | two directories above djinterp's root | the window's build: uxoxo's project root |
| `VPARSE_UI_HOST` | `auto` | the window's build: `win32`, `glfw` or `none`; see below |
| `VPARSE_UI_BUILD_TESTS` | `ON` | the window's build: its tests, and vparse's with them |
| `VPARSE_UI_BUILD_UXOXO_TESTS` | `OFF` | the window's build: uxoxo's own platform smoke test |

The roots take backslashes, so a path pasted from Explorer works, and a
wrong one fails at configure time with the variable to set. The model's floor
is C++17, as the window's is uxoxo's: uxoxo's headers include djinterp's free
monad, which is empty below it.

### The window

`VPARSE_UI_HOST` picks the Dear ImGui backends the module opens its window
with, and one of `src/ui/vparse/host_*.cpp` is compiled:

| host | backends | default |
|---|---|---|
| `win32` | Win32 and Direct3D 11, a WARP device where there is no hardware one | on Windows |
| `glfw` | GLFW 3.3 or later and OpenGL 3.3 | elsewhere, where uxoxo's build finds GLFW (`libglfw3-dev`, `brew install glfw`, `vcpkg install glfw3`) |
| `none` | no window: `vparse gui` reads its command line, then says there is no window and points at the commands | where neither can be built |

`vparse` is a console program: started from a terminal, it prints there,
the window's help and errors among it. Started from Explorer, `vparse gui`
closes the console Windows opened for it as its window opens.

### Flags

Code this build owns, the tool's and djinterp's, is built at its level with
the conformance flags djinterp's ladder uses (`-pedantic-errors
-Werror=undef`, `-D_XOPEN_SOURCE=700` on POSIX), `-Wall -Wextra -Wshadow
-Wcast-qual`, and, for the tool's own sources, the rest of the style guide's
baseline: `-Wconversion`, and under Clang `-Wreserved-identifier` and
`-Wdocumentation`; `/W4` under `cl`. Under a cl-style driver every unit that
reads djinterp's headers is given the same reading flags, uxoxo's included:
`/utf-8`, `/Zc:__cplusplus`, `/permissive-`, `/EHsc`, and for `cl` itself
`/Zc:preprocessor`. Dear ImGui is given `/utf-8` alone.

**Whose headers are judged.** The tool includes its own headers by a path
relative to the file, as djinterp's tree does, and djinterp's, uxoxo's and
Dear ImGui's through their include directories, which this build passes as
system ones. GCC and Clang judge a header by how it was found, so the tool's
own headers are held to its warnings and its dependencies' are not. `cl`
judges a header by where it is, and the model's three headers sit in
djinterp's `inc/`, so under `cl` they are read as external, at
`/external:W0`: the GCC and Clang builds are the ones that check them.

**`<windows.h>`.** This build defines no Windows macro. `wingdi.h` defines
`ERROR`, and djinterp's LR family has an opcode called `ERROR`, so a unit that
includes `<windows.h>` and then `lr.hpp` does not compile. None does today:
`host_win32.cpp` includes no vparse header, and the command line's `main.cpp`
includes `<io.h>` and `<fcntl.h>` alone. The cure is the opcode's name, in
djinterp: branch `vparse/lr-reject` renames it `REJECT`.

## Tests

`ctest` runs every suite a build makes, the program, and djinterp's suites
for the code the tool stands on, at every level the compiler can select;
the window's build runs vparse's with its own.

| test | suite | expected |
|---|---|---|
| `vparse_workbench_core_tests` | the model: refusals, parity with the session, a failure, reading under the reader's own limit, the heat map, the store, the memo, the profile | `passed: 8` |
| `vparse_cli_tests` | the command line, run in-process: help, check, compile, match, run, saving and replaying, a folder named beyond ASCII, refusals, what the kit gives it (`-q` anywhere, `@FILE`, an output that takes nothing), bench, corpus, compare, the memo and how its values are folded, every JSON document held to this README's tables, what `run` and `corpus` were given, completion | `passed: 17` |
| `vparse_runs` | `vparse run` over the window's example grammar | exits 0 |
| `vparse_workbench_tests` | the window's build: the window's model and face, as values: the opening run, events, the face | `passed: 3` |
| `vparse_workbench_imgui_tests` | the window's build: the face drawn on uxoxo's Dear ImGui platform in frames with no window: no Dear ImGui error and no ID conflict anywhere the pointer goes, and clicks that post the right events | `passed: 3` |
| `vparse_gui_help`, `vparse_gui_version`, `vparse_gui_unread` | the window's build: `vparse gui --help`, `--version -v` and a grammar it cannot read -- the module found beside the program, loaded and run, with no display | its usage, and 0; its build, `window:` and `imgui:` among it, and 0; `cannot read`, and 2 -- the status and the words held together, by the kit's `d_cli_add_test` |
| `djinterp_foundation_tests_cxx11` ... `_cxx23` | djinterp's parse/parsegen foundation, C++ | `passed: 56` at C++11 and 14, `57` at 17, `58` at 20 and 23 |
| `djinterp_foundation_c_tests_c99` ... `_c23` | the same foundation, from C | `passed: 12` |
| `vparse_tests_cxx11` ... `_cxx23` | djinterp's vparse: the machine, the generator, both frontends, the session, the memo and the profile, the corpus | `passed: 32` |
| `djinterp_arg_parser_c_tests_c99` ... `_c23` | djinterp's argument parser, from its own leaf | the counts its `expected_sections.txt` gives |

That is 30 tests under GCC or Clang in djinterp's build, and 35 in the
window's -- 34 where Dear ImGui has no null backend
(`backends/imgui_impl_null.cpp`), which the ImGui suite needs. `cl`, whose
lowest C++ level is C++14 and which has no C23, gets 21 and 26 -- the
argument parser's sweep left out, its linker having no `--wrap`; `clang-cl`
leaves C++11 out the same way, and C23 where CMake has no level for it.
Each suite must print exactly its count, so a section that silently stopped
being compiled fails rather than reading as a smaller pass.

djinterp's suites come from its own test leaves, under
`build/cmake/config/testing/djinterp/`, added as they are. They build their
libraries a second time, on purpose: the suites need the testing
configuration, `D_TESTING=1` changes the layout of `struct d_parse_machine`,
and a library and everything linking it must agree on that. The libraries
the programs link are the production ones; the two never meet.

## The command line

```sh
bin/tools/vparse/vparse-linux64 [OPTION]... COMMAND [ARGUMENT]...
```

It is made on djinterp's command-line kit, and its grammar is the kit's
`GRAMMAR.md`, section 11. It has what every djinterp program has (section
6 there): `-h` and `--help`, after
the program or after a command; `--version`, and with `-v` how the program
was built; `-q`, `-v` and `--color`, before the command or after it; and the
`help` and `completion` commands. Its help is laid out as GNU programs lay
theirs out, and each command's help ends with what its exit status answers.

| command | does | ends with |
|---|---|---|
| `check GRAMMAR` | reads the grammar and judges it, as the session does: its rules and the size of its program, or, as an error, why it was refused and where its reading stopped; a grammar that loops over an empty input is said to | 0 accepted, 1 refused |
| `compile GRAMMAR [-o FILE]` | writes the session's listing of the program, an instruction a line | 0 built, 2 refused |
| `match GRAMMAR INPUT...` | runs the grammar over each input and says how each went, and how many matched | 0 every input matched, 1 one did not, 2 refused |
| `run GRAMMAR [INPUT]` | runs the grammar over the input, or over nothing, and prints the run's headline and the parts `--part` names | 0 matched, 1 not, 2 refused |
| `replay FOLDER` | runs a saved run again, and prints it as `run` does | as `run` |
| `saved [--root=DIR]` | lists the runs saved under the root, newest first | 0 |
| `bench GRAMMAR [INPUT...]` | runs the grammar over each input, and over its corpus where no input is given or `--walks` is: each run counted once and timed apart, the fastest of `--times`; prints the totals, the inputs and the rules | 0, 2 refused |
| `corpus GRAMMAR [-o DIR]` | makes inputs from the grammar, random walks of its rules and near-misses of them, and lists them or writes each to a file of its own, `0001-walk.txt` | 0, 2 refused |
| `compare FIRST SECOND [INPUT...]` | runs both grammars' programs over the inputs, and over the first's corpus where no input is given or `--walks` is, and names each input they say different things of | 0 agree, 1 differ, 2 refused |
| `help [COMMAND]` | the same as `[COMMAND] --help` | 0 |
| `completion SHELL` | a completion script for `bash`, `zsh`, `fish` or `powershell` | 0 |

| option | on | effect |
|---|---|---|
| `GRAMMAR` | all but `replay`, `saved`, `compare` | the grammar: a file, or `-` for standard input |
| `FIRST SECOND` | `compare` | the grammars: files, or `-` for one of them; the first is the one compared against, and its corpus is the one run |
| `-f, --frontend=notation\|ebnf`, `--ebnf` | the same | the frontend, both grammars' for `compare`; by default `ebnf` for a file whose name ends in `.ebnf`, in any case, and the notation otherwise |
| `INPUT` | `match`, `run`, `bench`, `compare` | the input: a file, or `-` for standard input, which one argument may name |
| `-t`, `--text` | the same | each input argument is the input itself, not a file name |
| `-p, --part=PART` | `run`, `replay` | the parts to print, in order, the option given again or the parts separated by commas: `summary` (the default), `grammar`, `reading`, `reading_trace`, `rules`, `plan`, `program`, `matches`, `heat`, `profile`, `trace`, or `all` |
| `--step-limit=STEPS` | `check`, `match`, `run`, `bench`, `compare` | stop the machine's run over an input after N steps (default 100000); the grammar is read under a limit of its own, which grows with it |
| `--memoize=RULES` | `match`, `run`, `bench`, and `compare`'s second program | the rules the program calls through the memo: `none` (the default), `all`, `auto` -- the rules a run without the memo says it pays for, over the whole corpus for `bench` and `compare` -- or rule names, the option given again or the names separated by commas: `--memoize=Sum --memoize=Prod` is `--memoize=Sum,Prod`. A name after names is added to them, and anything else replaces what came before it, so that `--memoize=none` at the end turns the memo off |
| `--walks=N`, `--depth=N`, `--length=N`, `--repeats=N`, `--misses=N`, `--seed=N` | `bench`, `corpus`, `compare` | how a corpus is made: walks (default 50), each going N rules deep (8) or N bytes long (256) before it takes each rule's shortest way out, a `*` or `+` repeated at most N times (3), N near-misses a walk (1), from a seed (1): one seed is one corpus, on every system |
| `--times=N` | `bench` | runs of each input timed, the fastest kept (default 3); steps are counted once, since they repeat exactly |
| `-o FILE` | `compile` | write the program to the file, whole or not at all: into a new file beside it, renamed over it; `-` is standard output |
| `-o DIR` | `corpus` | write each input to a file of its own in the folder, made if it is not there, each file whole or not at all |
| `--no-trace` | `match`, `run` | do not record the trace: `match` then never runs a failed input again to say where it stopped, and `run` has no frontier, heat, matches or trace to print |
| `--format=text\|json` | all | text, for reading, or one JSON document, for a program |
| `-q`, `--quiet` | the program, anywhere | only errors on standard error, as every djinterp program; and what the status says is left out of what is printed: `check`, `match` and `compare` print nothing, `run` and `replay` print the parts without the headline |
| `--save`, `--root=DIR` | `match`, `run` | save each run, as the window saves one, in a new session folder under the root (default `vparse_runs`) |
| `@FILE` | anywhere | the arguments the file holds, split at white space and quoted as GCC reads a response file (as the C runtime does on Windows); not after `--` |

Every failure is said as the kit says one, on standard error, and ends with
2: `vparse: error: MESSAGE`. A command line that cannot be read is a usage
error, the command's name before the message and a pointer to its help
after it:

```
vparse: error: match: standard input can be read once: - may stand for the grammar or for one input
Try 'vparse match --help' for more information.
```

A file that cannot be read or written says which and why --
`vparse: error: cannot read 'expr.txt': No such file or directory` -- and an
output that takes nothing says so: `cannot write standard output: No space
left on device`. A grammar the session refuses is said after its name, in
the words of the run's headline:

```
vparse: error: expr.txt: the program could not be built: rule not defined: Atom
```

and, where its reader stopped at a place, after the place, as a compiler
says one and an editor turns into a link -- the place taken out of the
words, which the JSON's `error` and the run's `result` still hold whole:

```
expr.txt:2:12: error: the grammar could not be read: the reader stopped, inside Grammar > Rule, expecting ';' or '|'
```

The EBNF reader says what it expected and not where, so its refusal has no
place: `vparse: error: expr.ebnf: the grammar could not be read: expected
';'`.

`check`, `compile`, `match` and the measures then print nothing more but,
with `--format json`, their JSON; `run` and `replay` print the run all the
same, since its parts show how far it got. A refused grammar is trouble, 2,
to every command but `check`, whose answer it is: 1. Every command reads every file it
is given before it runs, and writes its output only once the output is
whole, so a command line that fails prints the failure and nothing else.

```sh
vparse check expr.txt
vparse match expr.txt --text 1+2 '(3*4)-5' 6/
vparse run expr.txt input.txt -p rules,program,heat
vparse run expr.ebnf --text '2*(3+4)' -p all --format json
vparse match expr.txt corpus/*.txt --save --root runs
vparse replay runs/session-20261016-101500/run-003 -p trace
vparse run expr.txt input.txt -p summary,profile --memoize auto
vparse corpus expr.txt --walks 200 -o corpus
vparse bench expr.txt corpus/*.txt --memoize auto --format json
vparse compare expr.txt expr_factored.txt --walks 200
vparse compare expr.txt expr.txt --memoize all
vparse completion bash > vparse.bash
```

A run is counted as it runs, traced or not: its `profile` part has a row a
rule -- its calls, the calls that repeated one at the same position and the
steps those ran, the memo's answers, and its steps, arrivals and revisits, the
heat map's counts -- and the summary says how deep each stack went and what
the memo did. A corpus input that ends in `-walk.txt` was derived from the
grammar's rules, and one that ends in `-miss.txt` is a walk with a character
deleted, inserted or replaced; derived is not accepted, since ordered choice
can refuse what the rules derive.

### JSON

`--format=json` prints one JSON document, whatever the command, ended by a
newline: an object whose members come in the order the tables below give.
A document only grows (the grammar's R19): a key, once written, keeps its
meaning, and a new key comes after the ones there. The suite holds every
table here to what the command writes, key for key and in order
(`tests_cli_json`), so a key the code adds and the README does not is a
failing test.

JSON is UTF-8: a byte that belongs to no UTF-8 sequence, which a grammar or
an input may hold, is written as the visible escape `\xHH` the listings use
for control characters. A count is a whole number; a time, `ms`, is
milliseconds with three decimals. A grammar's or an input's *name* is the
one text prints: a file's name as given, `standard input` for `-`, and an
input given as text quoted and clipped, `'12+3'`; its *argument* is the
command line's own word.

#### `check`

| key | value |
|---|---|
| `grammar` | the grammar's name |
| `frontend` | `notation` or `ebnf`, the frontend that read it |
| `read` | whether the grammar was read |
| `accepted` | whether the session accepted it, and its program was built |
| `error` | the run's headline when it was refused -- "The grammar could not be read: ..." -- and `""` when it was not |
| `rules` | the rules' names, in order: those read, refused or not |
| `instructions` | the program's size, 0 when none was built |
| `stops_at_step_limit` | whether the grammar, accepted, runs to the step limit over an empty input, as a left-recursive one does |

#### `match`

| key | value |
|---|---|
| `grammar` | the grammar's name |
| `frontend` | the frontend that read it |
| `accepted` | whether the grammar was accepted |
| `error` | the headline of the run that found it refused, or `""` |
| `inputs` | an object an input, in order (below); none when the grammar was refused |
| `matched` | how many inputs matched |
| `total` | how many inputs were given |

#### `match.inputs[]`

| key | value |
|---|---|
| `input` | the input's argument: a file's name, `-`, or the text itself |
| `as_text` | whether the input was given as text, `-t` |
| `matched` | whether the grammar matched it whole |
| `consumed` | how many bytes the run read |
| `steps` | the machine's steps |
| `headline` | the run's headline: "Matched all 4 characters in 37 steps." or where it stopped |
| `saved` | the run's folder, with `--save`, or `""` |

#### `run`

| key | value |
|---|---|
| `grammar` | the grammar's name |
| `input` | the input's name: a file's, or a text quoted and clipped, `'12+3'`; `""` for a run over nothing |
| `frontend` | the frontend that read the grammar |
| `headline` | the run's headline |
| `read` | whether the grammar was read |
| `built` | whether its program was built |
| `matched` | whether the program matched the input whole |
| `consumed` | how many bytes the run read |
| `steps` | the machine's steps |
| `saved` | the run's folder, with `--save`, or `""` |
| `parts` | a member a part `-p` named, in the order named, keyed by the part's name (below) |
| `argument` | the input's argument, as `match.inputs[]`'s `input` is: a file's name, `-`, or the text itself; `""` for a run over nothing (since 2026.10.26) |
| `as_text` | whether the input was given as text, `-t` (since 2026.10.26) |

#### `replay`

| key | value |
|---|---|
| `folder` | the saved run's folder, as given |
| `frontend` | as `run`'s, and every key after it up to `parts`: a replay has no `argument` |
| `headline` | |
| `read` | |
| `built` | |
| `matched` | |
| `consumed` | |
| `steps` | |
| `saved` | `""`: a replay saves nothing |
| `parts` | |

#### `run.parts.PART`

| key | value |
|---|---|
| `title` | the part's title, as its tab shows it: `Summary`, `Rules` |
| `columns` | the listing's column headings |
| `rows` | its rows, each an array of its cells, as text prints them |
| `note` | what an empty listing says, or `""` |

#### `saved`

| key | value |
|---|---|
| `root` | the folder looked under, as given |
| `runs` | an object a run, newest first (below) |

#### `saved.runs[]`

| key | value |
|---|---|
| `folder` | the run's folder |
| `session` | its session folder's name |
| `run` | its number in the session |
| `frontend` | the frontend that read its grammar |
| `when` | when it ran, `YYYY-MM-DD HH:MM:SS` |
| `result` | its headline |

#### `bench`

| key | value |
|---|---|
| `grammar` | the grammar's name |
| `frontend` | the frontend that read it |
| `accepted` | `true`: a refused grammar writes a refusal (below) |
| `error` | `""` |
| `memoize` | the memo's setting, as `--memoize` folded it: `none`, `all`, `auto`, or names |
| `memoized` | the rules the program called through the memo |
| `inputs` | how many inputs were run: those given, then the corpus's |
| `matched` | how many matched |
| `unfinished` | how many ran to the step limit |
| `steps` | the steps of every run, each run counted once |
| `unmemoized_steps` | for `auto`, the steps the inputs took without the memo, which chose it; `null` otherwise |
| `revisits` | the times a rule was entered again at a position it had been entered at |
| `peaks` | the stacks' deepest, over every run (below) |
| `memo` | what the memo did, over every run (below) |
| `ms` | the time of every run, each the fastest of `times` |
| `times` | how many times each input was timed, `--times` |
| `rules` | an object a rule, in the program's order (below) |
| `runs` | an object an input, in order (below) |

#### `bench.peaks`

| key | value |
|---|---|
| `calls` | the deepest call stack |
| `back` | the most backtrack points |
| `marks` | the most marks |
| `caps` | the most captures open |

#### `bench.memo`

| key | value |
|---|---|
| `entries` | the results the memo kept |
| `hits` | the calls it answered |
| `misses` | the calls it did not |

#### `bench.rules[]`

| key | value |
|---|---|
| `rule` | the rule's name |
| `calls` | its calls |
| `repeats` | its calls at a position it had been called at before |
| `repeat_steps` | the steps those repeats ran |
| `hits` | its calls the memo answered |
| `steps` | the steps run inside it |
| `arrivals` | the times the machine arrived in it, as the heat map counts |
| `revisits` | its arrivals at a position it had arrived at before |

#### `bench.runs[]`

| key | value |
|---|---|
| `input` | the input's argument, or a corpus input's text |
| `kind` | `given`, `walk` or `miss` |
| `matched` | whether the program matched it whole |
| `consumed` | how many bytes the run read |
| `steps` | the machine's steps |
| `finished` | whether the run ended within the step limit |
| `revisits` | its revisits |
| `ms` | its time, the fastest of `times` |

#### `corpus`

| key | value |
|---|---|
| `grammar` | the grammar's name |
| `accepted` | `true`: a refused grammar writes a refusal (below) |
| `error` | `""` |
| `walks` | `--walks` |
| `depth` | `--depth` |
| `seed` | `--seed` |
| `inputs` | an object an input, in order (below) |
| `length` | `--length` (since 2026.10.26) |
| `repeats` | `--repeats` (since 2026.10.26) |
| `misses` | `--misses` (since 2026.10.26) |

#### `corpus.inputs[]`

| key | value |
|---|---|
| `text` | the input |
| `kind` | `walk`, derived from the grammar's rules, or `miss`, a walk with a character deleted, inserted or replaced |
| `file` | the file `-o` wrote it to, or `""` |

#### `compare`

| key | value |
|---|---|
| `first` | the first grammar's name, the one compared against |
| `second` | the second's |
| `first_frontend` | the frontend that read the first |
| `second_frontend` | the frontend that read the second |
| `accepted` | `true`: a refused grammar writes a refusal (below) |
| `error` | `""` |
| `memoize` | the second program's memo setting |
| `memoized` | the rules the second program called through the memo |
| `inputs` | how many inputs both programs ran |
| `matched` | how many both matched |
| `agreed` | whether they said the same of every input |
| `differences` | an object an input they did not, in order (below) |
| `first_steps` | the first program's steps over every input |
| `second_steps` | the second's |

#### `compare.differences[]`

| key | value |
|---|---|
| `input` | the input's argument, or a corpus input's text |
| `kind` | `given`, `walk` or `miss` |
| `first` | what the first program said of it (below) |
| `second` | what the second said, the same keys |

#### `compare.differences[].first`

| key | value |
|---|---|
| `matched` | whether the program matched the input whole |
| `consumed` | how many bytes it read |
| `steps` | its steps |
| `finished` | whether it ended within the step limit |

#### `refusal`

What `bench`, `corpus` and `compare` write for a grammar the session
refused, beside the error; `check`, `match`, `run` and `replay` write their
own documents, `accepted` or `built` false.

| key | value |
|---|---|
| `grammar` | the refused grammar's name -- for `compare`, the first refused |
| `frontend` | the frontend that read it |
| `accepted` | `false` |
| `error` | the headline of the run that found it refused |

## The window

```sh
vparse gui [OPTION]... [GRAMMAR] [INPUT]
```

The module `vparse gui` loads, found beside the program (the kit's GRAMMAR,
section 7). Its operands and the options it shares with the commands are
spelled as they spell them; a grammar or an input it cannot read is refused,
as the commands refuse one, before the window opens.

| option | effect |
|---|---|
| `GRAMMAR` | start with this grammar; by default the workbench's example |
| `INPUT` | start with this input |
| `-f, --frontend=notation\|ebnf`, `--ebnf` | the frontend, decided as the commands decide it |
| `--root=DIR` | save runs under DIR (default `vparse_runs`) |
| `--load=DIR` | load a saved run's folder at the start, and run it again |
| `--no-run` | do not run the grammar the window starts with |
| `--font=FILE` | a TrueType font (default: a system monospace font) |
| `--font-size=PX` | font size in pixels, 4 to 256 (default 15) |
| `--size=WIDTHxHEIGHT` | window size (default 1440x900) |
| `--frames=N` | draw N frames, and close |
| `--screenshot=FILE` | write the last frame `--frames` draws to FILE, as a PPM |
| `--version` | the version; with `-v`, how the module was built, the window and the Dear ImGui among it |
| `--help` | the list |

`vparse_gui`, the window's program of its own until 2026.10.26, is retired,
and with it its `-g` and `-i`, its `--completion` -- the program's
`completion` command covers the program -- and the `--print` and `--part`
it kept only to say where they went.

## The screen

The toolbar holds **Run** (also Ctrl+Enter, from anywhere), the front end
(notation or EBNF), whether to record the trace, and the step limit of the
run over the input.

The left column has the grammar and input editors, the list of runs (newest
first, green if the run matched, red if not), and the file controls.

The right side shows the selected run: a sentence saying what happened, the
**tape**, and a tab for each part of the run.

The tape is the text as a strip of character cells. The rules that matched
are drawn beneath it as nested spans, one layer per depth. The highlighted
cell is where the machine was at the current trace step; move it with the
slider, the arrow buttons, the arrow keys while the pointer is on the tape,
or by clicking a character, which jumps to the first step that reached it.
When a run fails, the furthest point the parser reached is framed in red, the
text after it is greyed, and the run opens at the step where it failed there.
When the grammar itself cannot be read, the tape shows the grammar instead,
with the reader's failure marked the same way.

| tab | what it shows |
|---|---|
| Summary | the result, sizes, step counts, timings, where the run came from |
| Grammar | the grammar, line by line |
| Reading | notation: the grammar's tape with what each piece was read as, the reader's captures, and the reader's own trace. EBNF: the syntax tree |
| Rules | the rules, as the front end understood them |
| Plan | the generator's directives (notation only; EBNF compiles directly) |
| Program | the PEG program; the instruction at the current step is highlighted |
| Matches | the rules that matched, as a tree over the input; after a failure, what had matched when the parser stopped |
| Heat | where the parser spent its work: arrivals (the separate times it came to a place, which is re-parsing) or steps, at each input position and rule, up to the current step, with the call stack drawn over it. Click a cell to jump to its first step. Hot spots lists the cells, most arrived at first, and Profile what the run counted rule by rule: its calls, the calls that repeated one, the memo's answers, and its steps, arrivals and revisits |
| Trace | every step: address, instruction, offset, next character, call and backtrack depths. Click a row to move the tape there |

The tabs are the parts `vparse run --part` prints.

**Comparing runs.** Click **vs** beside another run to compare it with the
selected one. Each tab then shows both runs side by side, and rows that only
one of them has are washed red, with a count above each side. Program rows
are compared by what they mean, not where they sit: calls by the rule they
call, branches by rule and offset, so adding an alternative marks the new
instructions and not every address after them. Traces are shown side by side
without marking. On the Heat tab the two maps share one colour scale, so a
grammar that re-parses less shows up paler.

**Files.** **Save run** writes the selected run, **Save all runs** every run,
and **save each run** saves each run as it finishes, one folder per session,
the same folders `vparse --save` makes:

```
<root>/session-YYYYMMDD-HHMMSS/
    index.tsv          one line per saved run: id, time, front end, result
    run-007/
        run.txt        id, time, front end, trace, step-limit and memo
                       settings, result
        grammar.txt    the grammar, byte for byte
        input.txt      the input, byte for byte
        summary.txt  reading.txt  reading_trace.txt  rules.txt
        plan.txt  program.txt  matches.txt  heat.txt  profile.txt  trace.txt
        heat.svg       the whole run's heat map, with a failed run's frontier
```

The part files are aligned text, for reading and for diffing runs with other
tools; a part `vparse run` prints alone is that file's text. **Saved runs**
lists everything under the root; clicking one loads it: the grammar, input
and settings are read back and run again, which reproduces every other part
exactly, since a run is deterministic.

## How it is put together

- **The model** (`run`, `store`, `heat`) runs the stages and describes them.
  A grammar that reads is offered to djinterp's `session`, which accepts it
  or refuses it in its own words; an accepted grammar is then built here with
  every stage kept. A program here of another length than the session's is a
  fault the run reports, and the suites hold the two to the same
  disassembly.
  The notation grammar is read the way `notation::parse_grammar` reads it,
  under the same step limit, `notation::read_limit`, but on a traced machine,
  so the reading can be inspected like any other run; a reading that stops
  untraced is read again, traced, so that it can always say where. The
  matched-rule tree, the failure frontier and the heat map are recovered by
  replaying the trace; every pass is also counted as it runs, so a run's
  profile needs no trace. Where the request names a memo, the program is
  made to call those rules through it once the session has accepted the
  grammar; the trace readers see a `MEMO` the memo answered for what it was,
  and replay the spans its rule's first match closed, so a memoized run's
  matches are the plain run's.
- **The command line** (`cli/`) is a tree of subcommands on djinterp's
  argument parser. Each subcommand reads its files, runs the model's
  `execute()`, the function the window's Run button calls, and prints the
  model's own listings, as text or as JSON (`report`). A trace is recorded
  only for what it explains: `match` runs each input untraced and a failed
  one again, traced, to say where it stopped; `check` and `compile` never
  trace. The measures (`measures.cpp`) build a grammar once, through the
  same `execute()`, and run the library's machine over the inputs
  themselves, untraced, counting each run and timing it apart.
- **The window's face** (`ui/vparse/view`, `ui/vparse/elements`) is a uxoxo
  template over
  the model: one shell with holes for the toolbar, editors, runs, files and
  detail, closed each frame with uxoxo's closing pass. `update` applies an
  event to the model. The workbench's own elements, **listing**, **tape** and
  **heat**, are uxoxo element modules; their attributes `key` and `target`
  mean what they mean on every uxoxo element.
- **The renderers** (`elements_imgui.cpp`) draw those elements on uxoxo's
  Dear ImGui platform, registered through `set_renderer`, with the theme.
- **The host** (`host_*.cpp`) opens the window, runs a frame, and reads one
  back for `--screenshot`; `plugin.cpp`, the module's run function, reads
  its command line through the kit and is the loop between them.

## Limits

- A notation literal is one character, `'a'`, so EBNF's `"ab"` is `'a' 'b'`
  there. Notation character classes take only letters, digits and `-`, so
  `[+-]` does not read; write `'+' | '-'`. EBNF classes are wider.
- PEG has no left recursion: a left-recursive grammar loads, and a run over it
  loops until the step limit stops it, which the command line and the
  window both say.
- Both frontends read a grammar as PEG: `|` is ordered choice, so
  `S = "a" | "ab"` (in EBNF) never matches `ab`, and `S = A* A` never
  matches, since the repetition takes every `A`.
- The trace keeps every step in memory. Runs of a few hundred thousand steps
  are fine; the step limit bounds a run over an input, and a traced reading
  of a grammar takes about fifteen steps a byte of it: a grammar of 5,000
  rules is two million steps and some 360 MB traced, and a tenth of a second
  and 34 MB with the trace off.
- `--memoize auto` counts a run without the memo before it chooses, so the
  choice costs what the run it saves would have: over `nested.txt` at depth
  7, ten seconds for a run of 183 steps.
- A profile costs a run about half again its time; the trace costs far
  more. The memo keeps every result to the end of the run, in pages of 256
  positions for each rule it is used for.
- The heat map gives each input character a column up to 160 of them; longer
  inputs share columns, each covering a range of characters.
- EBNF is read by a hand-written parser rather than the machine, so its
  reading has no trace.
- Comparing very long listings (over four million row pairs, once the common
  start and end are set aside) falls back to comparing row by row.

## What was verified

On 2026.10.27, later, `@FILE` listed by the kit among the root's options,
and the stand-in for `cl` configuring as `cl` does, with the cli lane's drop
of 2026.10.27c:

| | result |
|---|---|
| djinterp's vparse build, GCC, warnings as errors; Clang; MinGW-w64 under Wine; the window's build, GCC and MinGW-w64 | 30, 3, 3, 35 and 8; no warning in the tool |
| the window's build through `cl_standin/`, djinterp's suites in | configured as `cl` configures it, the argument parser's sweep left out; no warning; 22 of 26, the parser's four C++ suites faulting under Wine |

On 2026.10.27, a refused grammar said with the place its reader stopped
first, over the same djinterp with the cli lane's drop of 2026.10.27:

| | result |
|---|---|
| djinterp's vparse build, GCC 13.3, Debug, warnings as errors; Clang 18.1; MinGW-w64 under Wine | no warning in the tool; 30 of 30, 3 of 3 and 3 of 3, the model's suite 8 of 8 and the command line's 17 of 17 |
| the window's build, GCC under a virtual display; MinGW-w64 under Wine | 35 of 35; 8 of 8 |
| three faults planted in a copy of the tree: the place never put first; the sentence keeping its place; the column one off | each caught, by the command line's suite, and the last two by the model's too |

On 2026.10.26, later, `run`'s JSON ending with the input's argument and
`corpus`'s with the settings it lacked, over the same djinterp with the cli
lane's drop of 2026.10.26e:

| | result |
|---|---|
| djinterp's vparse build, GCC 13.3, Debug, warnings as errors; Clang 18.1; MinGW-w64 under Wine | no warning in the tool; 30 of 30, 3 of 3 and 3 of 3, the command line's suite 17 of 17 |
| the window's build, GCC under a virtual display; MinGW-w64 under Wine | 35 of 35; 8 of 8 |
| three faults planted in a copy of the command line: `run`'s last two members left out; `corpus`'s `repeats` and `misses` swapped; the README's `argument` row dropped | each caught by the command line's suite |

On 2026.10.26, again, the window out of djinterp's tree -- the module
`vparse gui` loads, built by `build/ui/vparse` -- over the owner's djinterp
of 2026.10.23 with the cli lane's drop of 2026.10.26c, and uxoxo of
2026.10.25:

| | result |
|---|---|
| djinterp's vparse build, GCC 13.3, Linux, Debug, warnings as errors | no warning in the tool; 30 of 30. The same four in the argument parser's own test leaf |
| the same under Clang 18.1, djinterp's suites left out | no warning; 3 of 3 |
| the same under MinGW-w64 GCC 13, run under Wine 9.0 in `C.UTF-8` | no warning; 3 of 3 |
| the window's build, GCC, Debug, warnings as errors, GLFW and OpenGL 3 on a virtual X display with Mesa | no warning in the tool; 35 of 35: the 30, the window's two suites, and `vparse gui`'s help, its version and a grammar it cannot read, each held to its status |
| the window's build, MinGW-w64 GCC 13, Win32 and Direct3D 11, under Wine | no warning; 8 of 8 |
| the window's build as CMake configures it for `cl` 19.51, through `cl_standin/`, run under Wine in `C.UTF-8` | CMake reports MSVC 19.51.36260.0 and takes `cl`'s branches; every unit of the window's and uxoxo's read as djinterp's are; no warning; 8 of 8, the module found and loaded. With djinterp's suites, 22 of 27: the argument parser's sweep does not link there, and its C++ suites fault on their first instructions (`cl_standin/README.md`) |
| `vparse gui GRAMMAR INPUT --frames 4 --screenshot FILE`, on GLFW and on Win32 under Wine | each opens on the grammar and the input, runs them and captures its frame |
| three faults planted in a copy of the tree: a refusal ending 0, its error still said; no `window:` line; the module's command misnamed | each caught by the runs of `vparse gui`: the first, which a pattern alone passes, by its status |

On 2026.10.26, the command line on djinterp's command-line kit and spelled
as its grammar's section 11 has it, over the owner's djinterp of 2026.10.23
with the cli lane's drop of 2026.10.26b, and uxoxo of 2026.10.25:

| | result |
|---|---|
| GCC 13.3, Linux, Debug, warnings as errors | no warning in the tool; 30 of 30. Four warnings in the argument parser's own test leaf (`arg_argv_tests.c`, line 5000), not the tool's |
| Clang 18.1, Linux, Debug, warnings as errors, the command line alone, djinterp's suites left out | no warning; 3 of 3 |
| the window, GLFW and OpenGL 3, GCC, Debug | 33 of 33 |
| MinGW-w64 GCC 13, the command line alone, run under Wine 9.0 in `C.UTF-8` | no warning; 3 of 3, the command line's suite among them; `--version -v` gives the build, and a grammar named beyond ASCII is read |
| seven faults planted in a copy of the command line: a write that fails passed over, a refused grammar not said, `-q` never read, `@FILE` not read (2026.10.25); `--memoize`'s values not folded, `check` answering 2, `auto` not read as a word (2026.10.26); a key dropped from a JSON table, two keys swapped in one, a key the code adds, a key the code renames (2026.10.26b) | each caught by the command line's suite |

On 2026.10.17, against djinterp of 2026.10.15 with this tool, djinterp's
vparse of 2026.10.17 and the argument parser of 2026.10.15 in it, and uxoxo
of 2026.10.16 with its Dear ImGui 1.92.8:

| | result |
|---|---|
| GCC 13.3, Linux, Release and Debug, warnings as errors | no warning; 24 of 24 each |
| Clang 18.1, Linux, Release and Debug, warnings as errors | no warning; 24 of 24 each |
| `VPARSE_CXX_STANDARD` 11 with the tool and uxoxo at 17, under Clang; 23 with 23 and 23, under GCC | no warning; 24 of 24 each |
| the command line alone: `-DVPARSE_BUILD_GUI=OFF`, no uxoxo, Clang | no warning; 21 of 21 |
| djinterp's vparse leaf alone, in plain CMake, GCC and Clang | 32 of 32 at C++11, 14, 17, 20 and 23 |
| MinGW-w64 GCC 13, 64-bit Windows (`mingw-cross`), run under Wine 9.0 in `C.UTF-8` | no warning in the tool or in djinterp's foundation and vparse; the argument parser's `printf` formats warn there (below), so the build was run without `-Werror`. 20 of 24: the tool's six and djinterp's foundation and vparse suites pass, and the argument parser's own suite fails at each of its four levels (below) |
| the whole build as CMake configures it for `cl` 19.51, through `cl_standin/`, run under Wine in `C.UTF-8` | CMake reports MSVC 19.51.36260.0 and takes `cl`'s branches; no warning; 17 of 20, the argument parser's three failing as under MinGW-w64 |
| the window: GLFW and OpenGL 3 on a virtual X display with Mesa, and Win32 and Direct3D 11 under Wine, built by MinGW-w64 and through the stand-in | each opens with a grammar and an input given by name, runs them and captures its frame; the faces match but for the times |
| the face, in the ImGui suite | with each part's tab brought forward in turn, the Profile listing and the reading's trace are drawn; with only pages brought forward, as before, neither was, each being a tab within a page |
| thirty-one deliberate faults: nineteen in djinterp's vparse (the memo, the profile, the corpus, the two small fixes), six in the model, six in the command line | every one caught: the nineteen by djinterp's vparse suite, two of them by the model's too; the model's six by the model's suite, four of them by the command line's too; the command line's six by its suite |

**Wine and names beyond ASCII.** Wine keeps a Windows file name on the Unix
side in the locale's encoding, so in the POSIX locale it makes no file or
folder whose name is beyond ASCII: `CreateDirectoryW` fails with error 2
for `runs_` and a u with a diaeresis alone. `tests_cli_store_names` fails
there for that reason, and passes in `C.UTF-8`, where the suites above were
run.

**The argument parser on `msvcrt.dll`.** Its suite fails 9 of its 295
sections at each level under Wine, 6 with `LC_ALL=C.UTF-8`, which its own
notes ask Wine for. The six are its path checks, on one case: `tree/dir/`, a
directory named with a trailing slash, which `msvcrt.dll`'s `_wstat64` does
not find and the Universal CRT does. Its notes verify it with llvm-mingw,
whose runtime is the Universal CRT; MinGW-w64 GCC and the stand-in link
`msvcrt.dll`. And MinGW-w64 GCC checks `printf` formats against
`msvcrt.dll`'s, which have no `%z`: the parser's completion scripts and
messages use `%zu`, and warn (`-Wformat`), so a `mingw-cross` build with
`-DVPARSE_WARNINGS_AS_ERRORS=ON` stops in the parser. The parser is another
agent's, and is left as it is.

**Not verified: `cl` itself.** No MSVC toolchain was available, so `cl`'s
own parser, its `/W4` over the tool, and the MSVC C++ library are untested;
the first build on Windows is the one that proves them.
`cl_standin/README.md` has what the stand-in shows and what it cannot.
