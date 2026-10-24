# The djinterp command-line kit

`parse/cli/` is what every djinterp program shares on its command line: the
grammar in [GRAMMAR.md](GRAMMAR.md), installed over the argument parser
(`arg_parser/`) so that a program declares only its own commands. A program
made with the kit has, without writing them, `-h`/`--help`, `--version`,
`-q`, `-v`, `--color`, the `help` and `completion` commands, help laid out
as GNU programs lay it out, failures in the GNU shape on standard error, the
grammar's exit statuses, and -- for a command that is a window -- a module
loaded only when that command runs.

The kit is C99 (`c/`, the `d_cli_` library), with a C++ face beside it
(`cli_program.hpp`, `cli_io.hpp`, `cli_plugin.hpp`) that adds no state of its
own: a `kit::program` *is* the parser and holds the kit's C record.

---

## Files

| file | what it holds |
|---|---|
| `c/cli_program.h` | the program: making one, its commands and their work, the run and its halves, the reports |
| `c/cli_format.h` | help's layout, as the parser's formatter |
| `c/cli_io.h` | the standard streams, terminals, colour, and files read and written whole |
| `c/cli_plugin.h` | the plugin interface, finding and loading a command's module, both sides of a plugin |
| `c/cli.h` | every C header of the kit, and the parser's |
| `cli_program.hpp`, `cli_io.hpp`, `cli_plugin.hpp` | the C++ face of each |
| `cli.hpp` | every C++ header of the kit, and the parser's face |
| `../../config/parse/cli/cfg_cli.h` | the kit's knobs |
| `../../env/parse/cli/env_cli.h` | what the platform has: consoles, modules, the program's own file |
| `src/djinterp/parse/cli/c/cli_system.c` | everything the kit asks of the system; the only source that includes a system header |

---

## A program in C

```c
#include <stdbool.h>
#include <stdio.h>
#include "djinterp/parse/cli/c/cli.h"

// `hello say NAME`: the command's work, given the program and the command
static int
say(struct d_cli_program* _program,
    struct d_arg_command* _command,
    void*                 _context)
{
    size_t                          count = 0u;
    const struct d_pack_text* const name  = d_arg_option_values(
        d_arg_command_option(_command, "NAME"), &count);

    (void)_context;
    printf("hello %.*s\n", (int)name[0].length, name[0].data);
    d_cli_note(_program, 1, "said hello");      // shown at -v

    return D_CLI_EXIT_OK;
}

int
main(int argc, char** argv)
{
    struct d_arg_parser  parser;
    struct d_cli_program program;

    // the kit first: the rule that an option given twice takes its last
    // value is a default, and reaches only what is defined after it. Any
    // of the making that fails is kept, and the run says it
    (void)d_arg_parser_init(&parser, "say hello", "hello");
    (void)d_cli_program_init(&program, &parser, "hello", "1.0",
                             D_CLI_PROGRAM_COMMANDS);

    struct d_arg_command* const command = d_cli_program_add_command(
        &program, NULL, "say", "say hello to NAME", say, NULL, NULL);
    struct d_arg_option* const  name    = (command != NULL)
        ? d_arg_command_add_option(command, "NAME", "who to greet")
        : NULL;

    if (name != NULL)
    {
        d_arg_option_set_required(name, true);
    }

    const int status = d_cli_program_run(&program, argc,
                                         (const char* const*)argv);

    d_cli_program_release(&program);
    d_arg_parser_release(&parser);

    return status;
}
```

`d_cli_program_run` is the whole of `main`: the parse; the ending a parse
calls for -- help, a version or a completion script to standard output and
0, a failure to standard error and 2 -- and otherwise the work of the
command the arguments reached, whose status the program exits with. A
program with something to do between the two calls `d_cli_program_parse`,
which returns `true` when the run is over, and then
`d_cli_program_dispatch`.

`main` checks nothing it makes. A definition that fails -- the parser's,
or the kit's, for want of memory or a name used twice -- is kept, by the
parser or in the program's `status`, and the run ends in trouble and says
why, `hello: error: out of memory`, rather than run a program short of a
command.

A program of one command gives the root work of its own with
`d_cli_program_set_main`, and leaves `D_CLI_PROGRAM_COMMANDS` out.

---

## A program in C++

```cpp
#include <string>
#include "djinterp/parse/cli/cli.hpp"

namespace kit = djinterp::parse::cli;

int
main(int argc, char** argv)
{
    kit::program program("hello",
                         "1.0",
                         "say hello",
                         kit::program_flags::commands);
    std::string  name;

    kit::command say = program.add_command(
        "say",
        "say hello to NAME",
        [&name](kit::program& _program, kit::command)
        {
            (void)kit::write_file("-", "hello " + name + "\n");
            _program.note(1, "said hello");

            return kit::exit_status::ok;
        });

    say.add_option("NAME", name, "who to greet").required();

    return program.run(argc, argv);
}
```

A command's work is any function object -- a function, a functor, a lambda
-- called as `work(program, command)`, returning an `int` or an
`exit_status`. The program keeps a copy and frees it with itself. The work
runs behind the face's guard: an exception it lets out is said as the
program's error -- its `what()`, or `out of memory` -- and the run is
trouble, 2.

Every definition CLI11's `App` makes is made on a `kit::program` as on any
parser, since the program is one.

---

## What a command's work says

| call | writes | returns |
|---|---|---|
| `d_cli_report(program, D_CLI_SEVERITY_ERROR, ...)` | `PROGRAM: error: MESSAGE` | -- |
| `d_cli_report_at(program, severity, file, line, column, ...)` | `FILE:LINE:COLUMN: warning: MESSAGE` | -- |
| `d_cli_note(program, level, ...)` | `PROGRAM: note: MESSAGE`, at `-v` for level 1, `-vv` for 2 | -- |
| `d_cli_usage_error(program, command, ...)` | `PROGRAM: error: COMMAND: MESSAGE` and `Try 'PROGRAM COMMAND --help' ...` | 2 |
| `d_cli_file_error(program, "read", path, errno)` | `PROGRAM: error: cannot read 'PATH': REASON` | 2 |

A message is printf's format, starting in lower case, with no full stop.
`-q` leaves only errors. Each report is composed whole and written in one
write; one there is no memory to compose is composed in a room of its own
on the stack, and cut there if it must be, so that the want of memory
shortens a diagnostic and never loses it. The C++ face has the same calls
on the program, each with the message given whole: a `%` in it is written
as it is.

Results go to standard output, everything said about them to standard
error. Help, a version or a script the output refuses -- a full disk -- is
trouble, said: `hello: error: cannot write standard output: No space left
on device`. `d_cli_write_file` writes a file whole or not at all -- `-` is
standard output -- and through a symbolic link to the file it leads to,
the link kept; a link it cannot follow, for any reason but that it leads
nowhere, fails the write rather than be replaced. `d_cli_read_file` reads
a file whole -- `-` is standard input.

---

## A command that is a window

The program declares the command, and nothing of the window:

```cpp
program.add_plugin("gui", "open the window");
```

`hello gui ARGUMENT...` then finds `hello-gui` with the platform's suffix for
a module -- along `HELLO_PLUGIN_PATH`, then beside the program, then in
`../lib/hello/` from it -- loads it, checks that it is `hello`'s at `1.0`,
and runs it with every argument after `gui`. A module not there, or not
this program's, is said, with where it was looked for or why it was
refused, and the run is 2. A module loaded stays loaded until the process
ends.

The module is a program of its own, made from the host that runs it, so
that `hello gui --help` is the window's help and `hello -v gui` is verbose
in the window too:

```cpp
#include "djinterp/parse/cli/cli.hpp"

namespace kit = djinterp::parse::cli;

static int
run_window(const d_cli_plugin_host* _host,
           int                      _argc,
           const char* const*       _argv)
{
    kit::program window(*_host, "open the window");

    window.set_main([](kit::program&, kit::command)
                    {
                        // the window's own loop
                        return kit::exit_status::ok;
                    });

    return window.run(_argc, _argv);
}

D_CLI_PLUGIN("hello", "1.0", "gui", "open the window", run_window)
```

A module and its program meet at two C records and one function
(`cli_plugin.h`): nothing else crosses, so a module built with another
toolchain's runtime still runs. Its run function must not let an exception
out; running a `kit::program`, whose work is guarded, is the usual way to be
sure.

---

## Building

With CMake, include the kit's module; it makes `djinterp::arg_parser` and
`djinterp::cli`, static and position-independent, and the function that
makes a module:

```cmake
include(<djinterp root>/build/cmake/djinterp/parse/cli/DjinterpCli.cmake)

add_executable(hello main.cpp)
target_link_libraries(hello PRIVATE djinterp::cli)

d_cli_add_plugin(hello-gui SOURCES window.cpp)
```

`d_cli_add_plugin` makes a `MODULE` library with no `lib` prefix, linking
the kit, whose one exported symbol is its entry point. Place it as the
program looks for it.

Without CMake, compile `DJINTERP_ARG_PARSER_SOURCES` and
`DJINTERP_CLI_SOURCES` -- listed in
`build/cmake/djinterp/parse/cli/DjinterpCliSources.cmake` -- as C99, with
`inc/` on the include path, and link the dynamic loader where the platform
keeps it apart (`-ldl` on older Linux).

---

## Knobs

`config/parse/cli/cfg_cli.h`; each is `#ifndef`-guarded, so `-D` wins. A
feature knob removes code, never a structure's member, so units built with
different settings still agree on every layout.

| knob | default | what it does |
|---|---|---|
| `D_CFG_CLI_ALL` | unset | the default of every feature knob without a value of its own |
| `D_CFG_CLI_PLUGINS` | 1 | the plugin loader, where the platform has modules; without it a plugin command says so and exits 2 |
| `D_CFG_CLI_COLOUR` | 1 | colour on a terminal, as `--color`, `NO_COLOR` and `TERM` allow |
| `D_CFG_CLI_FORMATTER` | 1 | help in the GNU layout; without it, the parser's own |
| `D_CFG_CLI_CONSOLE_WIDE` | 1 | a Windows console written through `WriteConsoleW` |
| `D_CFG_CLI_HELP_WIDTH` | 80 | the columns help takes off a terminal |
| `D_CFG_CLI_HELP_WIDTH_MAX` | 100 | the most columns help takes on a terminal |
| `D_CFG_CLI_HELP_COLUMN_MAX` | 30 | the furthest right a description starts |

---

## Tests

The suites are under `tests/djinterp/parse/cli/`: the C suites in `c/`,
the C++ suite beside them, and the probe modules the plugin suites load in
`c/probe/`. Beside the C suites is the allocation sweep,
`c/cli_sweep_tests.c`: each of its cases -- a program made, a command line
run to its end, a file written or read, a module found or loaded -- run
once as it is, then once for each allocation it makes with that one
refused, every run held to leaving nothing live and to doing all the clean
run did or ending in trouble and saying `out of memory`. It wraps `malloc`
and its kin at link time, so it is built where the linker has `--wrap`.
Their leaf, `build/cmake/config/testing/djinterp/parse/cli/cli/`, builds
them two ways, each holding every run to the count of sections
`expected_sections.txt` gives it:

```
cmake -S build/cmake/config/testing/djinterp/parse/cli/cli -B out
cmake --build out
ctest --test-dir out

build/cmake/config/testing/djinterp/parse/cli/cli/run_cli_tests.sh [--sanitize]
```

The script takes `CC`, `CXX` and `AR`, and for a Windows cross build
`EXE_SUFFIX=.exe` and a `RUN_PREFIX` that runs a Windows program, such as
`env LC_ALL=C.UTF-8 wine`. A run is given the directory the probes are in,
or looks beside itself.

A case's expected output is in its table. With `D_TESTS_CLI_DUMP` set in
the environment, a C run prints what each case came to as the table entry
that would hold it -- for a change to the kit's words made on purpose, to
be read, checked, and put in the table. With `D_TESTS_CLI_SWEEP_SHOW` set,
the sweep prints everything both runs wrote for each run that fell short.
