# The djinterp program

`djinterp` is the djinterp language's command line. In 0.8, the preview of
11-30, it is the djins' and their settings': it lists the djins the program
registers, describes them, and reads and writes their settings. The
interpreter, with `djinterp -c 'LINE'`, `djinterp FILE` and standard input,
comes in 0.9 ([DESIGN.md](../../interp/DESIGN.md), sections 1 to 5).

It is a program on the command-line kit, so it has everything the kit's
grammar gives every program: `--help`, `--version`, `-q`, `-v`, `--color`,
`help`, `completion`, and the settings every program reads
([GRAMMAR.md](../cli/GRAMMAR.md), section 6).

---

## Commands

```
djinterp list                         the djins, a line each: name, version,
                                      summary; then the frames no djin names
djinterp describe DJIN [COMMAND]      a djin, or one of its commands, as
                                      text, or JSON (--format=json)
djinterp get [--show-origin] [NAME]...
                                      settings as statements that load back,
                                      every one without a NAME; where each
                                      value came from
djinterp set [--section=PROGRAM] NAME=VALUE...
                                      settings written to the user's file
```

The program registers every core djin before it runs: parse's, desc's, the
environment printer's and the log's. With the kit's own, `cli`, they are
what 0.8 lists, each as "settings only", since none has commands yet.

```
$ djinterp list
parse        -  settings only
desc         -  settings only
env_printer  -  settings only
log          -  settings only
cli          -  settings only
$ djinterp get cli.unknown_settings cli.plugin_strict_version
cli.unknown_settings = typos
cli.plugin_strict_version = true
$ djinterp set cli.unknown_settings=all
$ djinterp set --section=vparse cli.unknown_settings=none
$ djinterp get --show-origin cli.unknown_settings
cli.unknown_settings = all  (file)
    default    typos   (D_CFG_CLI_UNKNOWN_SETTINGS)
```

After the two `set`s, the user's file holds a line every program reads and
one for vparse alone:

```
cli.unknown_settings = all

[vparse]
cli.unknown_settings = none
```

- **`get`** writes statements the file reads back, so its output can be
  pasted into the file; an unset text is a comment, `# plugin_path is
  unset`. With `--show-origin`, each value's source and the bindings under
  it, down to the default and its knob.
- **`set`** checks each setting as the file will bind it, so a name no djin
  has, or a value the setting does not take, is refused before anything is
  written, and the run ends with 2. It writes the file whole, each setting
  under its canonical name: among every program's lines, or with
  `--section=PROGRAM` in that program's. A line kept after the new one that
  sets the same name for a program that reads it is said by its line,
  since that program reads it last. The headers are kept even in a run that
  reads the file as one part.
- **`describe`** writes a djin's description, or one command's; for a frame
  no djin names, its settings. `--format=json` writes
  `"djinterp.describe/1.0"`. `help --format=json` adds two members to the
  kit's description, `djins` and `reserved` (DESIGN.md, 6.4).

Where the user's file is, how the file, the environment and the command
line stand, and how its sections are read: the kit's grammar, section 6.

---

## Not yet

- **A djin's commands.** `djinterp DJIN COMMAND [ARGUMENT]...` runs them
  from the first djin with commands of its own, in 0.9. Its names are kept
  now: no djin may be named `list`, `describe`, `get`, `set`, `help`,
  `completion`, `settings`, `cli` or `language`.
- **Another program's settings.** A djin `djinterp` does not register is
  one it cannot check, so `set` refuses its settings:

  ```
  $ djinterp set bitwisedom.keys=paths
  djinterp: error: no djin is named 'bitwisedom'
  ```

  Such lines are written by hand, in the program's section.
- **Loadable djins,** `djinterp-DJIN` modules found as the kit finds a
  command's module (DESIGN.md, 6.5).
- **`--show-origin`'s file and line.** 0.8 shows the source's word.

---

## Building

The program's leaf builds it with the kit, the djin records and the core
djins' sources, as C99, and runs its tests:

```
cmake -S build/cmake/config/djinterp/tools/djinterp -B build/djinterp
cmake --build build/djinterp
ctest --test-dir build/djinterp
```

`DJINTERP_PROGRAM_BUILD_TESTS` (on by default) builds the tests. Each runs
the program as a user would, through the kit's `d_cli_add_test`, with a
home of its own, so that no user's settings file is read or written: the
`set` runs write theirs, and the runs after them read it. The djin records'
describers have a DTest suite of their own, `djinterp_djin_desc`.

---

## Files

| file | what it holds |
|---|---|
| `src/djinterp/tools/djinterp/djinterp_main.c` | `main`: the parser, the program and the kit's run of it |
| `src/djinterp/tools/djinterp/djinterp_commands.h`, `.c` | the four commands, the core djins registered, and the members `help --format=json` adds |
| `inc/djinterp/c/modules/interp/djin.h`, `djin_desc.h` | the djin records, the set the program keeps them in, and their text and JSON describers |
| `build/cmake/config/djinterp/tools/djinterp/` | the leaf: the program and its tests |
| `tests/djinterp/c/modules/interp/djin_desc_tests.c` | the describers' suite |
