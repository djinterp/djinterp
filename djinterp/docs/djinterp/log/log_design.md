# The event and log contract

Lane E's design, for the agreement of Friday 11-13: what a log record is,
how it is written, and how it relates to events, message codes and desc. It
records the owner's rulings of 11-12, E21 to E27; the log's code lands on
Monday 11-16, built on what Friday settles (E27 (b)). Every feature is
customizable: a knob (`D_CFG_*`, fixed when compiling) says what a build
has, and an option (a row of the `log` djin, set from a file, the
environment or the command line) says what a run does, with sensible
defaults for both.

## A log record is an event, a message code and desc facts

```text
record = event occurrence      (c/event: a dj_event_key and its payload)
       + message code          (the codes registry)
       + desc facts            (docs/djinterp/desc/desc_design.md)
       + time, severity, origin
```

- **The event** is what happened, as the event system already names it: a
  `dj_event_key` and its occurrence's payload. Logging is then one more
  subscriber: a log writer is a handler on the log's dispatcher, and
  anything that listens for events can listen for log records with no
  second mechanism.
- **The message code** says what kind of thing happened, stably, for
  machines: a module, a number, a name, a class, a grade and a one-line
  template ("net.connect_timeout": "connecting to {host} took over
  {timeout_ms} ms"). Codes are the contract: text may be reworded, a code's
  meaning may not.
- **The facts** are the record's data, as desc emits them: each a
  `struct dj_desc_fact` (`c/desc/desc.h`). The message's template names
  facts, so the text is written last, from facts, like every other desc
  output.

## The event key (E21 (c))

A record's key is the hash of its code's full name ("net.connect_timeout"),
FNV-1a 64 (`d_event_key_of_name`), so C and C++, two runs and two
processes agree, with no registration. Uniqueness is checked, not hoped
for: CI hashes every name in the codes manifest (below) and fails on a
collision, and a debug build checks each name as it registers.

| Knob | Default | |
|---|---|---|
| `D_CFG_EVENT_KEY_HASH` | FNV-1a, 64 bits | the hash; a program's own allowed |
| `D_CFG_EVENT_KEY_CHECK` | 1 | 0 never, 1 in debug builds, 2 always |

The option `event.key_check` turns the check off or on where it is built.
CI's check of the tree's own names is not optional: it is the tree's
guarantee.

## Levels (E22)

Every level is a name and a severity on OpenTelemetry's scale of 1 to 24,
so a threshold is one comparison, and any level maps onto OpenTelemetry and
syslog. The seven built in:

| Level | trace | debug | info | notice | warning | error | fatal |
|---|---|---|---|---|---|---|---|
| Severity | 1 | 5 | 9 | 10 | 13 | 17 | 21 |

`D_CFG_LOG_LEVELS` says how much of this a build has, a spectrum from none
to levels added while the program runs:

- **0, no levels.** Every record is written; level arguments and checks
  compile away.
- **1, the seven** (the default).
- **2, the seven and a program's own, fixed when compiling:**
  `D_CFG_LOG_LEVELS_EXTRA` lists name and severity pairs, `(audit, 14)`.
- **3, a run-time registry:** levels may also be added while the program
  runs, `d_log_level_add("audit", 14)`, or from settings,
  `log.levels="audit=14,security=19"`.

`D_CFG_LOG_LEVEL_MIN` compiles out every record below a level, at no cost,
since the cheapest level check is none. The options `log.level` (a name or a
number) and `log.<module>.level` set the threshold for the run, and per
module.

## Writing a record

```c
D_LOG(net, connect_timeout, D_LOG_FACT_TEXT("host", host),
                            D_LOG_FACT_INT("timeout_ms", ms));
```

- The level check comes first and costs a cached read
  (`dj_option_env_cache`, the knob-and-option pattern's hot path): a record
  below the threshold builds nothing.
- Facts are collected into a fixed array on the caller's stack, through a
  `struct dj_desc_sink` writing to desc's collecting machine (a
  `struct dj_desc_machine` whose `fact` keeps them); nothing allocates on
  the logging path. So a type's description can add its facts to a record,
  as it adds them to `--help`.
- The record goes to the log's writer: a machine with begin, record, end
  and flush, so a program can replace it. Text, JSON lines and OTLP/JSON are
  built in; syslog, journald or a network exporter are a program's writers.
- Numbers for machines never follow the locale; prose may group digits.

## The record's members: OpenTelemetry's log data model (E24 (b))

A record uses the data model's names: Timestamp, SeverityText,
SeverityNumber, Body and Attributes, the facts going in Attributes, the
code and its module among them as `code` and `module`. Each concept
djinterp lacked is a feature of its own: a knob builds it, an option turns
it on, and where it needs something from outside, a machine supplies it, so
a program can replace any part.

| Feature | Knob, default | Option, default | Machine |
|---|---|---|---|
| Severity number | always built | `log.severity_number`, on | E22's levels |
| Instrumentation scope: the module | `D_CFG_LOG_SCOPE`, 1 | `log.scope`, on | none: the module is known where the record is made |
| Resource: program, host, process, platform | `D_CFG_LOG_RESOURCE`, 1 | `log.resource`: off in JSON lines, where it would repeat on every line; on in OTLP, once per batch. `log.resource.<key>` adds or overrides one. | a resource provider; the default reads the environment report and the program's name |
| Trace and span ids, trace flags | `D_CFG_LOG_CONTEXT`, 0, since djinterp makes no traces | `log.context`, on when built | a context provider, which a tracing library supplies |
| Observed timestamp | `D_CFG_LOG_OBSERVED_TIME`, 1 | `log.observed_time`, off: it equals the timestamp unless a record waits | the clock |
| Dropped attributes count | with E25's cap | `log.dropped_count`, on | none |
| Timestamps | always built | `log.time`: UTC or local, and its precision | the clock: a test fixes it |
| Formats | `D_CFG_LOG_FORMATS`, all three | `log.format`: text (default), json, otlp | a writer |

The OTLP/JSON details -- how fields are spelt, how ids and 64-bit numbers
are written -- are checked against the specification when its writer is
built.

## Nested facts (E25 (b))

A record carries nested facts from 0.8: a fact's depth
(`dj_desc_fact.depth`) opens an object in JSON and a dotted key in text, so
a type's description goes into a record unchanged, and the JSON's shape
does not change in 0.9. The writer still writes once, front to back.

| Knob | Default | Option |
|---|---|---|
| `D_CFG_LOG_FACTS_MAX` | 32 | `log.facts_max`, up to the knob |
| `D_CFG_LOG_DEPTH_MAX` | 4 | `log.depth_max`, up to the knob |

The knobs are the stack's room, so an option cannot raise a cap past its
knob. Facts past a cap are counted in a final `truncated` fact (the
dropped attributes count). `log.nesting` chooses nested objects, or dotted
keys for tools that want flat attributes.

## Message codes: tables and a manifest (E23 (c))

- **One table per module**, beside the module, owned by the lane that owns
  the module and compiled out with it: each row a number, a name, a class,
  a grade, a template and the facts it names.
- A code's full name is `module.name`; its number is unique in its module;
  the pair never changes meaning once released.
- **A manifest CI generates and checks**: every code's module, number, name,
  class and grade. CI fails on a duplicate number, a hash collision (E21),
  or a released code whose meaning changed against the committed manifest,
  and the docs' list is made from it. Lane E writes the generator and the
  job; it runs in lane A's CI. The generator also runs on a program's own
  tables, so a program gets the same guarantee.
- The status codes the modules return today (`D_OPTION_STATUS_*`,
  `D_OSTREAM_*`, `errno`) map onto codes, so an error reported to a caller
  and an error logged are the same thing in two places.
- A module's own code names (`parse.expr.unclosed`) are its own; the first
  are lane F's: domain 4 of parse's diagnostics, its ten codes.

| Knob | Default | |
|---|---|---|
| `D_CFG_LOG_CODES` | 1 | 1 builds code names and templates; 0 keeps numbers only ("net:207") |
| `D_CFG_<MODULE>_LOG_CODES` | as `D_CFG_LOG_CODES` | the same, per module |

The option `log.code_style` writes a code by name, number or both, in text.

### Classes, sides and grades

Lane C's table of container error conditions (from the retired
`c/container/derror.h`, 11-04) gives the registry three things more, which
every module's codes share:

- **A class** for each code, from one vocabulary across modules:
  `argument.null`, `position.index`, `quantity.count`, `span.inverted`,
  `resource.memory`, `handle.stale`, `integrity.invariant` and the rest. A
  reader can then treat every module's out-of-range index alike.
- **A side**, a standard fact for a value or a range against the valid
  interval: any of below (B), at its lower edge (L), within (W), at its upper
  edge (U) and above (A). An index past the end is `position.index {A}`; a
  range overlapping the start is `position.range {B,L,W}`.
- **A grade**, what the event did, which sets the log's default level: ok,
  note, caveat, soft (an expected miss: not found, empty), rejected (nothing
  changed), failed (tried, and could not), broken (an invariant does not
  hold). The table's effect marks become facts: `effect` (no-op, partial)
  and `retryable`.

**The syntax family**, for bad input text, which lane C's table has no
class for: five classes, each graded rejected, for every parser in the tree
to share. A module's own code says which fault it is; the class says what
kind:

- `syntax.unexpected`: a token where the grammar has none;
- `syntax.missing`: something required is absent;
- `syntax.unbalanced`: a pair not matched;
- `syntax.ambiguous`: an order the grammar will not choose;
- `syntax.arity`: the wrong number of operands.

Lane F maps parse's seven syntax codes onto them once they join the
vocabulary.

Conditions the table marks undetectable (a comparator that is not a strict
weak order, a variadic count) get no code: they belong in a module's
documentation.

### Each grade's default level (E26 (b))

| Grade | ok | note | caveat | soft | rejected | failed | broken |
|---|---|---|---|---|---|---|---|
| Level | debug | info | notice | debug | warning | error | fatal |

A reader can tell a caller's mistake, which changed nothing, from the
library's failure, and successes stay out of info. `D_CFG_LOG_GRADE_LEVELS`
replaces the whole map when compiling; one option per grade changes it for
a run, `log.grade_rejected=error`, and the same with a module's scope,
`log.net.grade_rejected`.

## The log's settings, as option_env rows

`log` is a djin, so every setting has a knob, an environment variable, a
command-line option and a provenance:

| Row | Kind | Meaning |
|---|---|---|
| `level`, `<module>.level` | choice or int | the threshold: a level's name or a severity |
| `levels` | text | levels added at run time, with `D_CFG_LOG_LEVELS 3` |
| `format` | choice | text, json, otlp |
| `destination` | text | stderr, a file, none |
| `code_style` | choice | name, number, both |
| `facts_max`, `depth_max` | int ranges | the caps, up to their knobs |
| `nesting` | choice | nested, dotted |
| `grade_<grade>`, `<module>.grade_<grade>` | choice | a grade's level |
| `severity_number`, `scope`, `resource`, `context`, `observed_time`, `dropped_count` | bool | E24's features, where built |
| `resource.<key>` | text | a resource attribute added or overridden |
| `time` | text | UTC or local, and the precision |

## When it lands

- **Friday 11-13**: this contract, agreed.
- **Monday 11-16** (E27 (b)): the log's header, its writers and its suite,
  with desc's text and JSON machines; lanes adopt it from Wednesday 11-18.
- **Week 3**: the codes generator and its CI job, with lane A; the modules'
  code tables, each lane its own.

## Settled

1. A record's event key: the name's hash, checked (E21 (c)), above.
2. The levels: a spectrum from none to added at run time, the seven built in
   (E22), above.
3. The record's members: OpenTelemetry's names, each missing concept an
   opt-in feature (E24 (b)), above.
4. Nested facts: from 0.8, with a cap (E25 (b)), above.
5. The codes registry: tables per module, and a manifest CI checks
   (E23 (c)), above.
6. The class vocabulary, from lane C's table with the syntax family, which
   lane F agreed (11-11); each grade's default level, revised (E26 (b)),
   above. Lane C reports container errors with the codes this settles.
