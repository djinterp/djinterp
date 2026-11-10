# The knob-and-option pattern

Lane E's proposal, 2026-11-04, to agree by Friday 11-13 with lanes G and H
(and lane A, which asked for it). The interfaces named here exist in the tree
as of this delivery: `c/option/option_env.h`, `c/option/option_env_registry.h`
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
TEXT form is added when a lane first needs one.

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
  be a named value too. Lane F's depth guard is the first:

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
  `DJINTERP_NET_TIMEOUT_MS=5000`;
- the command line: `d_option_env_registry_take_option(reg, argc, argv, &i,
  &result)` takes `--net.timeout_ms=5000`, `--set net.timeout_ms=5000`,
  `--net.verbose` and `--no-net.verbose`; a misspelling returns -1, and
  `d_option_env_registry_suggest` gives the name meant;
- a test: `djinterp::DOptionEnvOverride` in C++ forces a value from `OVERRIDE`
  for its scope's life and gives it back; in C, `d_option_env_set_value` with
  `D_OPTION_ENV_SOURCE_OVERRIDE`, then `d_option_env_unset_from`.

## 3. The row says where its value came from

- `d_option_env_get` gives the value in force with its `source` (`DEFAULT`
  when unbound) and the binding frame's `depth`.
- `d_option_env_layers` lists every binding that bears on a row in a scope,
  the one in force first: what the CLI's `get --show-origin` shows.
- `d_option_env_var_at(env, var)->knob` names the knob when the default is
  one.

A proposed rendering for lane G's `get`:

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

## To agree by Friday 11-13

1. **Djin names** are module slugs: `net`, `cli`, `env_printer`; full names
   are `djin[.scope].variable`.
2. **The environment prefix.** Proposed: the registry root's name, upper-cased,
   then `_`: the program's registry is "djinterp", so `DJINTERP_`; a tool's
   own registry, `d_option_env_registry_init(&registry, "dawk")`, `DAWK_`.
3. **The CLI's `set`.** Proposed: it binds from `COMMAND_LINE`, so a setting
   typed at the prompt holds over a file and the environment, and a test's
   `OVERRIDE` still holds over it.
4. **Threads.** Settings are written before the program starts its threads;
   `d_option_env_cached` is not synchronised, so a read concurrent with a
   write is a race. Proposed for 0.9: publish the generation with
   release/acquire where C11 atomics exist (c/sync), and keep "configure,
   then run" as the rule until then.
5. **Lane G**: `get`, `set` and `--show-origin`'s text. **Lane H**: the knobs
   in net #10, #12, E1, E3 and E9, each mapped to a row.
6. **A per-call options struct.** A module that takes its settings per call,
   as expression core v2 does (`dj_parse_expr_options`,
   `docs/djinterp/parse/expression_core_v2.md`, section 11), keeps the struct,
   and a member left 0 means the setting in force: the module's djin row,
   read through its cache, whose default is the knob. A program that sets
   nothing then still answers to `DJINTERP_PARSE_EXPR_MAX_DEPTH`, and a
   knob that compiles the djin out leaves 0 meaning the knob.
