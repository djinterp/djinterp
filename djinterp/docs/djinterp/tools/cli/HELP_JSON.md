# The description: `djinterp.help/1.0`

A program on the command-line kit describes itself as JSON:

```sh
PROGRAM help --format=json            # the whole program
PROGRAM help COMMAND --format=json    # from a command down
```

The description is what a program's manual and wiki pages are built from:
the renderer reads it, with a CommonMark prose file per page, and checks
the prose's code spans against it. This page is the description's schema,
and the rules the code spans are checked by. `cli_help.h` writes it;
`d_cli_help_json` gives it to a program that has no `help` command.

## Versions

The first member, `format`, names the shape: `"djinterp.help/1.0"`,
MAJOR.MINOR, as `describe --format=json` names its own
(`"djinterp.describe/1.0"`).

- A member added is a minor step. A reader passes over a member it does not
  know.
- A member removed, or a member whose meaning changes, is a major step. A
  reader refuses a major version it does not know, rather than render a
  wrong page.

## The description

An object. The kit writes these members, in this order:

| Member | Value |
|---|---|
| `format` | `"djinterp.help/1.0"` |
| `program` | the program's name, as diagnostics begin: `"djinterp"` |
| `version` | its version, as `--version` prints it, or `null` |
| `command` | the command described, a **command** (below): the root, or the one `help COMMAND` named |
| `settings` | every setting the program reads, each a **setting**: the root's first, then each djin's, in the registry's order |
| `variables` | the environment variables the kit reads besides the settings', each a **variable** |
| `prefixes` | the prefixes the settings' variables begin with: `"DJINTERP_"`, then the program's own (`"BITWISEDOM_"`), unless that is the same |
| `exit_statuses` | what each exit status means, each `{ "status": 0, "meaning": "..." }`, as the root's help says it |

A program then adds members of its own, through its hook
(`d_cli_program_set_help_json`). A reader knows these three:

| Member | Value |
|---|---|
| `djins` | the program's djins, each a **djin** (below), in the order `list` gives them |
| `reserved` | the names a djin's page may not take: the program's commands, and its pages in sections 5 and 7 (`settings`, `cli`, `language` for `djinterp`) |
| `sections` | a page's own sections, each a **section** (below) |

`djinterp` writes `djins` and `reserved`. No program writes `sections` yet.

### A command

| Member | Value |
|---|---|
| `name` | its name: the root's is the program's |
| `path` | its full name, as a user types it: `"djinterp set"` |
| `summary` | what help says of it, or left out |
| `usage` | its own usage line, where it has one |
| `footer` | its footer, where it has one |
| `aliases` | its other names, where it has any |
| `options` | its options, each an **option**: its own, in the order they were made, then each option group's |
| `arguments` | its positionals, in order, each an **argument** |
| `commands` | its subcommands, each a **command**, down |

What help hides is left out: an option or a subcommand in the group named
`""`, and a disabled subcommand or option group.

### An option

| Member | Value |
|---|---|
| `names` | its names as typed: `["-o", "--output"]` |
| `value` | the placeholder of the value it takes, `"FILE"`; left out for a flag |
| `optional` | `true` where a flag's long name may take a value after `=` (`--color[=WHEN]`) |
| `summary` | what help says of it |
| `group` | the heading help lists it under, where it is not the command's own |
| `default` | its default, as help shows it, where it has one |
| `env` | the environment variable it is read from, where it has one |
| `choices` | the values its validators list, where they list any |
| `required` | whether it must be given |
| `standard` | whether it is one every program on the kit has: a command's `-h`, `--version`, `-q`, `-v`, `--color` (GRAMMAR.md, section 6). A page can refer to `djinterp-cli(7)` for these. |

### An argument

| Member | Value |
|---|---|
| `name` | its name: `"SHEET"` |
| `summary` | what help says of it |
| `min`, `max` | how many values it takes; `max` is `null` for no limit |
| `required` | whether it must be given |
| `default` | its default, where it has one |
| `choices` | the values its validators list, where they list any |

### A setting

| Member | Value |
|---|---|
| `name` | its full name: `"cli.plugin_search"`, or `"plugin_path"` on the root |
| `djin` | the djin it belongs to; left out on the root |
| `kind` | `"bool"`, `"int"`, `"real"`, `"text"`, `"choice"` or `"flags"` |
| `default` | its default, as the kind holds it: a truth value, a number, text or `null`; a choice by its name; flags as a list of names, the whole value's one name where it has one (`["all"]`), else each bit's |
| `knob` | the knob its default is, where it is one: `"D_CFG_CLI_PLUGIN_SEARCH"` |
| `summary` | what it is for |
| `alias` | its older name, where it has one |
| `choices` | a choice's or flags' names |
| `names` | an int's named values, each `{ "name": ..., "value": ... }` |
| `unbuilt` | the names ruled but not built in this build |
| `min`, `max` | an int's range, where it has one |
| `refuses` | the sources it refuses, by name: `"file"`, `"environment"`, `"command line"` |
| `env` | the variables it is read from, the shared prefix's first; left out where it refuses the environment |

### A variable

| Member | Value |
|---|---|
| `name` | its name: `"NO_COLOR"` |
| `system` | `"posix"` or `"windows"` where only one reads it; left out for both |
| `summary` | what it does |

A setting's variables are in its `env`, not here.

### A djin

| Member | Value |
|---|---|
| `name` | its name: `"awk"` |
| `version` | its version, for a djin with a record |
| `summary` | what it is, for a djin with a record |
| `commands` | its commands, each `{ "name": ..., "summary": ... }`, for a djin with a record |
| `settings_only` | `true` for a frame of settings with no record (`list` says "settings only"); its settings are in `settings` |

### A section

A page's own section, for prose the description carries itself.

| Member | Value |
|---|---|
| `title` | its heading |
| `id` | its anchor in the wiki, by which other pages refer to it: a lower-case name. By default, the title in lower case, with a dash for each run of other characters. |
| `after` | the standard section it follows (mdoc(7)'s order, from NAME and SYNOPSIS to SEE ALSO). By default, DESCRIPTION. Sections with the same `after` keep their order in the list. |
| `text` | its text, CommonMark |

The renderer refuses, naming the line, what mdoc cannot express.

## Code spans

A page's prose names the program's parts in code spans: `` `--output` ``,
`` `cli.plugin_search` ``. The renderer checks each against the
description, so a page cannot name an option the program does not have.
How it reads them is a setting of the renderer's, with a knob for its
default, an entry in a page's manifest, and an option for a run:

| Mode | A code span is read |
|---|---|
| `forms` (the default) | by its form, as the table below gives it |
| `markup` | by its mark, Pandoc's attribute: `` `--output`{.opt} `` |
| `options` | as an option where it begins with `-`; otherwise plain |
| `off` | never: every span is plain |

`forms` and `markup` can be on together, so that a mark settles what a
form leaves open. A form and a mark give the same kinds:

| A code span that is... | Kind | Checked against |
|---|---|---|
| begun with `-`: `-o`, `--output`, `--output=FILE` | `opt` | the command's `options` and its parents'; a value after `=` as `arg` or `val` |
| upper case with one of `prefixes`: `DJINTERP_CLI_PLUGIN_SEARCH` | `env` | the `variables`, and each setting's `env` |
| dotted lower case: `cli.plugin_search` | `set` | the `settings`, when its first part names one of `djins`; otherwise plain, with a warning when it is near a known name |
| `NAME=VALUE`, its name a setting's | `set` | the name; the value too, where the setting has `choices`, `names` or a range |
| an undotted name equal to a root setting's | `set` | exact match only |
| an undotted name equal to a djin's | `djin` | `djins`, exact match only |
| the program's name, alone or before a command path (`djinterp set`), or a command's exact name | `cmd` | the `command` tree |
| a path: begun with `/`, `./`, `../`, `~/` or `$NAME/` | `file` | nothing |
| any other upper case: `DJIN`, `FILE` | `arg` | nothing; a warning only when it is near an argument's `name` or an option's `value` but is neither |
| anything else | plain code | nothing |

Two kinds come only from a mark:

- `val`, a value an option or a setting takes: `` `json`{.val} ``. It is
  checked against the `choices` the description gives. A form gives it only
  after an option's or a setting's `=`.
- `lit`, never checked: what only looks like an option or a setting, such
  as `` `--` `` or `` `-1` ``.

A span two forms claim -- an undotted name that is a command and a root
setting both -- is rendered plain, with a warning, for a mark to settle.

The nine kinds, and the mdoc macro each is rendered with:

| Kind | What | mdoc |
|---|---|---|
| `opt` | an option | `Fl` |
| `set` | a setting | `Va` |
| `env` | an environment variable | `Ev` |
| `cmd` | a command | `Cm` |
| `file` | a path | `Pa` |
| `arg` | an argument's or a value's placeholder | `Ar` |
| `djin` | a djin | `Va` |
| `val` | a value an option or a setting takes | `Cm` |
| `lit` | text never checked | `Li` |
