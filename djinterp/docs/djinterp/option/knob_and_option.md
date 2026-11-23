# The knob-and-option pattern

Proposed on 2026-11-04 and agreed on Friday 11-13 by the modules it touches:
the option core, the CLI kit, parse, net and the build. Where the owner has
ruled since, the decision is cited by its module and number. The interfaces
named here exist in the tree: `c/option/option_env.h`, `c/option/option_env_registry.h`
and `cpp/option/option_env.hpp`. `env/env_printer.c` is the reference
implementation.

## What it is for

The owner's standing rule: wherever it makes sense, give the user both a
compile-time knob that sets the default and a run-time option that overrides
it, with the most sensible default. The pattern makes that one thing to do,
the same way in every module, in four parts:

1. the knob's value becomes the default of an option_env row;
2. a run-time binding overrides it, from a source whose standing is known;
3. the row says where its value came from;
4. a module reads it on a hot path for the cost of a global.

## 1. The knob becomes a row's default

The knob stays where knobs live: `config/<area>/cfg_<module>.h`, an `#ifndef`
default and an `#error` in the file's VALIDATION section, as
`config/env/cfg_env_printer.h` does.

The module's schema row is declared with the `_KNOB` form of its row macro:

```c
D_OPTION_ENV_INT_RANGE_KNOB("timeout_ms", D_CFG_NET_TIMEOUT_MS, 1, 3600000,
                            "how long a connection may take, in ms")
```

The macro evaluates the knob once, for the default, and stringises its name
into the row's `knob` member. The forms are `D_OPTION_ENV_BOOL_KNOB`,
`_INT_KNOB`, `_INT_RANGE_KNOB`, `_CHOICE_KNOB` and `_FLAGS_KNOB`; a REAL or
TEXT form is added when a module first needs one.

- **A range row repeats the knob's validation**, so a value the build refuses
  is refused at run time with the same bounds (`INVALID_ARGUMENT`).
- **A value ruled but not built** is listed in the row's `unbuilt` names
  (`D_OPTION_ENV_ROW`): binding it fails with `POLICY_REJECTED`, by name. It is
  never accepted and ignored: a setting that changes nothing is a silent lie.
- **A row with no knob** is a run-time option alone; that is fine where a
  compile-time default would mean nothing.
- **A value with a name, outside the range**: an INT row may name values.
  Text gives one by its name, ignoring case, or by its number; it is written
  back by its name, and `d_option_env_text` gives the name; it is taken even
  outside the range, which still holds for every other value. The knob may
  be a named value too. Parse's depth guard is the first:

  ```c
  static const struct dj_option_env_name D_PARSE_EXPR_DEPTHS[] =
  {
      { "unbounded", D_PARSE_EXPR_DEPTH_UNBOUNDED }
  };

  D_OPTION_ENV_INT_RANGE_KNOB_NAMED("expr_max_depth",
                                    D_CFG_PARSE_EXPR_MAX_DEPTH, 1, 65535,
                                    D_PARSE_EXPR_DEPTHS,
                                    "1 to 65535, or unbounded")
  ```

  `D_OPTION_ENV_INT_RANGE_NAMED` is the same with a plain default. An INT's
  `unbuilt` names are refused by name, as a choice's are.

## 2. A run-time option overrides it

A module's rows are declared in its **djin**: a frame named after the module's
slug, registered in the program's registry before the program starts its
threads. The djin's frame can be static and initialised before `main`, as
`env_printer.c`'s is, so no first call can race another:

```c
struct dj_option_result d_net_register(struct dj_option_env_registry* _registry);
// d_env_printer_register is the model: null registers in the program's
```

Every value is a **binding from a source**, in ascending standing:

| source | who binds from it |
|---|---|
| `DEFAULT` | nobody: what an unbound row reads, the knob's or the schema's |
| `PROGRAM` | the program, in code: `d_option_env_put`, `set_int` and the rest |
| `FILE` | a configuration file or sheet |
| `ENVIRONMENT` | the process's environment |
| `COMMAND_LINE` | the command line |
| `OVERRIDE` | forced: a test, a scoped override |

A binding yields only to one of higher standing, so the order in which a
program reads its file, its environment and its command line cannot change
the outcome. Among bindings of one standing, the innermost frame's holds, and
in it a scoped binding before a global one.

By full name -- `djin.variable` or `djin.scope.variable` -- through the
registry:

- a file: `d_option_env_registry_load(reg, text, D_OPTION_ENV_SOURCE_FILE,
  &offset)`, lines of `net.timeout_ms = 5000`, `#` comments;
- the environment: `d_option_env_registry_import(reg, "DJINTERP_")`, so
  `DJINTERP_NET_TIMEOUT_MS=5000`, and a scope's as its unscoped name is
  made, `DJINTERP_DESC_SHELF_RING=4` for `desc.shelf.ring`
  (`D_CFG_OPTION_ENV_SCOPED_IMPORT`, 1). Two settings that would make one
  name are refused when the second is declared, scope or variable, so an
  import never reads a name two ways (option_env's decision 47). Each name
  is read through a reader a program may replace, per frame or per
  registry (`d_option_env_set_getenv`, `d_option_env_registry_set_getenv`):
  `getenv` by default, and a build for a host with no environment of its
  own names the host's in `D_CFG_OPTION_ENV_GETENV`. A value is copied into
  the import's own room, `D_CFG_OPTION_ENV_VALUE_MAX` bytes; a longer one is
  refused and reported, never cut short (option_env's decision 83);
- the command line: `d_option_env_registry_take_option(reg, argc, argv, &i,
  &result)` takes `--net.timeout_ms=5000`, `--set net.timeout_ms=5000`,
  `--net.verbose` and `--no-net.verbose`; a misspelling returns -1, and
  `d_option_env_registry_suggest` gives the name meant;
- a test: `djinterp::DOptionEnvOverride` in C++ forces a value from `OVERRIDE`
  for its scope's life and gives it back; in C, `d_option_env_set_value` with
  `D_OPTION_ENV_SOURCE_OVERRIDE`, then `d_option_env_unset_from`.

A **row may refuse sources** it should not trust. A binding from a refused
source fails by name, with `D_OPTION_STATUS_POLICY_REJECTED`, as an unbuilt
value does; the value is untouched, and `import` reports the refusal and
reads the rest. A row whose value chooses what code a privileged program
loads must not take its caller's word, and its caller controls more than
the environment: it writes the command line, and it picks the settings
file, which the program finds through `XDG_CONFIG_HOME`, `HOME` or
`APPDATA`. So such a row refuses all three, `D_OPTION_ENV_REFUSES_CALLER`,
and only the program's own code, and a test's override, set it (option_env's
decision 69):

```c
D_OPTION_ENV_ROW_REFUSING(D_OPTION_ENV_KIND_TEXT, "plugin_search", NULL, 0u,
                          NULL, 0u, D_CFG_CLI_PLUGIN_SEARCH, 0, INT64_MIN,
                          INT64_MAX, 0.0, NULL, "D_CFG_CLI_PLUGIN_SEARCH",
                          "where modules are loaded from",
                          (D_CFG_CLI_PLUGIN_VARIABLE != 0)
                              ? 0u
                              : D_OPTION_ENV_REFUSES_CALLER)
```

The row is still declared, so `get`, `describe` and `--help` show the
setting, and say why a value was refused. A program whose builder trusts one
of those sources -- a file in `/etc` that only an administrator writes --
defines its own `D_OPTION_ENV_REFUSES_CALLER` before including `option_env.h`.
No run-time option relaxes a refusal: it could be set from the very sources
it guards.

The refused sources are `D_OPTION_ENV_REFUSES` bits, joined by `|`; any source
but `DEFAULT` may be refused, and a row that names another is not declared.
Use them where trust decides, not for taste: a setting a user may want from
the environment should not refuse it. `d_option_env_refuses(env, scope, var,
source)` reads a row's refusals as binding does, so help can say a source is
not taken before anything is bound.

## 3. The row says where its value came from

- `d_option_env_get` gives the value in force with its `source` (`DEFAULT`
  when unbound) and the binding frame's `depth`.
- `d_option_env_layers` lists every binding that bears on a row in a scope,
  the one in force first, and the default last: what the CLI's
  `get --show-origin` shows. The default's layer names its knob (`knob`),
  so a row nothing binds has one layer, the default, in force.
- `d_option_env_var_at(env, var)->knob` names the knob when the default is
  one.
- `d_option_env_check_from` (and, by full name, the registry's
  `d_option_env_registry_check_from`) checks a value as binding it from a
  source would, by the same rules, and writes it as a settings file needs
  it, binding nothing. Name and value come apart, as a shell gives them, so
  a value is never split again; an optional record, `dj_option_env_checked`,
  says which frame, scope and variable the name resolved, so a writer of
  settings names the setting canonically, and why a value was refused
  (option_env's decision 72). `d_option_env_registry_check` takes
  `NAME=VALUE` over it.
- `d_option_env_import_each` (and `d_option_env_registry_import_each`)
  imports as `d_option_env_import` does, telling a report of each value it
  could not bind: a record, `dj_option_env_report`, with the variable as
  read, its text, the setting's full name ("cli.plugin_strict_version"),
  the frame, scope and variable, the status and the reason. It grows by its
  `size` member, so members can be added after the freeze (option_env's
  decision 71). The stock report, `d_option_env_report_print`, writes
  "DJINTERP_X=maybe: not a bool" to a C stream, control bytes escaped; the
  plain imports use it with `D_CFG_OPTION_ENV_IMPORT_REPORT 1`, and say
  nothing by default, since a library writes to a program's error stream
  only when asked (option_env's decision 38).
- A **reason** says which rule refused a value, finer than its status: a
  name, a kind, a refused source, a value not built, a range, a choice, text
  the kind does not read, room. `d_option_env_reason_of` gives it for any
  status binding answered, and `d_option_env_reason_name` its stock words
  (option_env's decision 70). A registry's check adds one, the frame: a
  dotted name whose first part names no djin and that the root does not
  take, `net.timeout` with no djin `net`, which the stock report words "no
  djin named net". The check's record names the frame the name addressed,
  found or not (option_env's decision 84).
- `d_option_env_describe(env, sink, flags)` (`option_env_desc.h`) writes a
  frame's settings as desc facts, a level each: value, source, default,
  knob, help and refused sources, and on request each setting's layers,
  what it accepts and each scope's settings. It is the one reading of the
  provenance: describe, help and `--show-origin` cannot disagree
  (option_env's decision 73). It lives in a unit of its own, above desc.

A rendering for the kit's `get`:

```text
net.timeout_ms = 5000   (command line)
    under it      10000 (file)
    default       30000 (D_CFG_NET_TIMEOUT_MS)
```

## 4. A module reads it cheaply on a hot path

```c
static struct dj_option_env_cache timeout = D_OPTION_ENV_CACHE_INIT;

const int64_t ms = d_option_env_cached(&net_djin,
                                       D_OPTION_ENV_GLOBAL,
                                       net_var_base + D_NET_TIMEOUT_MS,
                                       &timeout)->integer;
```

The cache reads the frame again only when the chain's generation moved: any
binding, declaration or new parent anywhere in the chain moves it. Measured on
this machine (GCC 13, `-O2`, x86-64; a djin under the registry root, one
binding; each figure includes the loop's own cost):

| read | per read |
|---|---|
| `d_option_env_int` | 12.5 ns |
| `d_option_env_cached` | 2.5 ns |
| a plain `static int64_t` | 2.4 ns |

A module that need not see a change mid-operation can instead read its rows
once where it starts one -- a connection's open -- into its own fields.

A row's id is the schema's first id, from `d_option_env_declare_all` (a
schema's ids are consecutive), plus the row's place. It stays the same once
the djin is registered: a frame with children cannot be given another
parent.

## A module's checklist

1. The knob, in its cfg header, with its validation.
2. The row, with the `_KNOB` macro and the same range; its unbuilt values.
3. One line of help in the row: desc and `--help` render it.
4. The djin: static, named after the module's slug, and
   `d_<module>_register(registry)`.
5. A hot path reads through a cache.
6. Tests: the default from the knob (a build with `-D`), each source, the
   standing, a forced value, a misspelling's suggestion.

## Agreed on Friday 11-13

Each point says which module agreed to it: the CLI kit on 11-10, parse on
11-11 and 11-13, the build and net on 11-12.

1. **Djin names** are module slugs: `net`, `cli`, `env_printer`; full names
   are `djin[.scope].variable`.
   - Agreed by the kit (its own rows go in `cli`) and by parse (`parse`).
2. **The environment prefix.** Proposed: the registry root's name, upper-cased,
   then `_`: the program's registry is "djinterp", so `DJINTERP_`; a tool's
   own registry, `d_option_env_registry_init(&registry, "dawk")`, `DAWK_`.
   - Agreed by parse: its rows answer to `DJINTERP_PARSE_*`, and its
     suite checks `DJINTERP_PARSE_EXPR_MAX_ERRORS`.
   - Agreed by the kit for `djinterp`. A program of the kit imports
     `DJINTERP_` and then its own prefix, as `BITWISEDOM_`: for a setting
     both name, the program's wins, since the second import's binding
     replaces the first's; a setting only `DJINTERP_` names still binds.
     The kit's probe checks all three.
3. **The CLI's `set`.** Proposed: it binds from `COMMAND_LINE`, so a setting
   typed at the prompt holds over a file and the environment, and a test's
   `OVERRIDE` still holds over it.
   - Agreed by the kit, by where `set` is typed:
     - at the 0.9 interpreter's prompt, it binds from `COMMAND_LINE`;
     - in 0.8, `djinterp set DJIN.KEY=VALUE` is a run of its own, and a
       binding would die with it, so it writes the statement to the user's
       settings file, whose bindings are `FILE`'s on the next run;
     - a one-run override is `--set NAME=VALUE`, which every program of
       the kit takes through `d_option_env_registry_take_option`.
   - The settings file is `$XDG_CONFIG_HOME/djinterp/settings` (else
     `~/.config/djinterp/settings`), and `%APPDATA%\djinterp\settings` on
     Windows. The program reads it at start, after its djins register, with
     `d_option_env_registry_load`; a statement that fails is reported by its
     line, and the rest is read on. A program may read a file of its own
     after it; both are `FILE`, so its own wins. 0.8 has no system or
     project file: a project file is one an untrusted checkout could carry,
     so it waits for rows that refuse it (section 2).
4. **Threads.** Settings are written before the program starts its threads;
   `d_option_env_cached` is not synchronised, so a read concurrent with a
   write is a race. Proposed for 0.9: publish the generation with
   release/acquire where C11 atomics exist (c/sync), and keep "configure,
   then run" as the rule until then.
   - Agreed by the kit: the program configures and then runs; 0.8 has no
     threads of its own.
   - Agreed by parse: `settings.h` says to register before a program
     starts its threads.
5. **The kit**: `get`, `set` and `--show-origin`'s text. **net**: the knobs
   it listed for 0.8, each mapped to a row.
   - Agreed by the kit:
     - `get` and `set` write their values with
       `d_option_env_format_quoted`, so what they write loads back;
     - a TEXT that is unset is written by `get` as a comment,
       `# plugin_path is unset`, so its output still loads;
     - `--show-origin` shows the source's word, for the binding in force
       and then the one under it; the file and line come in 0.9, once
       bindings carry them.
6. **A per-call options struct.** A module that takes its settings per call,
   as expression core v2 does (`dj_parse_expr_options`,
   `docs/djinterp/parse/expression_core_v2.md`, section 11), keeps the struct,
   and a member left 0 means the setting in force: the module's djin row,
   read through its cache, whose default is the knob. A program that sets
   nothing then still answers to `DJINTERP_PARSE_EXPR_MAX_DEPTH`, and a
   knob that compiles the djin out leaves 0 meaning the knob.
   - Agreed by parse. Its rows, in a djin named `parse`:
     - `expr_max_depth`: `D_OPTION_ENV_INT_RANGE_KNOB_NAMED`, 1 to 65535,
       or `unbounded`;
     - `expr_max_errors`: `D_OPTION_ENV_INT_RANGE_KNOB`, 1 to 65535;
     - `expr_error_order`: `D_OPTION_ENV_CHOICE_KNOB` over `leftmost` (1)
       and `structure_first` (2), its knob defaulting to `leftmost`;
     - none of the three refuses a source (section 2): none chooses code to
       load.
