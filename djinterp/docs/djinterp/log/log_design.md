# The event and log contract

The log's design, agreed Friday 11-13: what a log record is, how it is
written, and how it relates to events, message codes and desc. It records
the owner's rulings of 11-12 and 11-17, cited by the log's decision numbers.
The code lands on Wednesday 11-18 (decisions 27 and 80): `c/log/log.h`,
`src/djinterp/c/log/log.c` and `log_writers.c`,
its knobs in `config/c/log/cfg_log.h`, and its suite, `c_log_tests`. Every feature is
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

## The event key (decision 21 (c))

A record's key is the hash of its code's full name ("net.connect_timeout"),
FNV-1a 64 (`d_event_key_of_name`), so C and C++, two runs and two
processes agree, with no registration. Uniqueness is checked, not hoped
for: CI hashes every name in the codes manifest (below) and fails on a
collision, and a debug build checks each name as it registers.

| Knob | Default | |
|---|---|---|
| `D_CFG_EVENT_KEY_CHECK` | 1 | 0 never, 1 in debug builds, 2 always (`cfg_event_common.h`) |

`d_log_code_key` gives a code's key; `d_log_module_add` refuses a table
whose key collides with a registered one's, where the check is built, and a
table with a number twice, always.

Events and codes share one key space. An event's name is declared beside
its type, the same string in C and C++ (the event core's decision 15:
`D_EVENT(DOnResize, "ui.resize", int, int)`,
`D_EVENT_DECLARE(on_resize, "ui.resize", int, int)`), and hashed the same
way. A record is an occurrence of its code's event, so a name means one
thing, with one payload, whichever declared it: the manifest lists events'
names as well as codes', and CI's collision check covers both (decision
64). The debug check sees only registered codes, since events are not
registered. A program's own hash (`D_CFG_EVENT_KEY_HASH`) and the option
`event.key_check` come with the event system's djin, in week 3. CI's check
of the tree's own names is not optional: it is the tree's guarantee.

## Levels (decision 22)

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

writes, in text,

```text
2026-11-13T15:04:05.123Z error net.connect_timeout: connecting to db1 took over 500 ms host=db1 timeout_ms=500
```

`D_LOG` needs variadic macros (C99, C++11); `d_log_emit(code, level, facts,
count)` is the same in one call, for C++98 and for a subscriber that finds
its code at run time (`d_log_code_find(module, size, number)`). A record
with nested facts is opened, filled through its sink, and closed:

```c
struct dj_log_record record;

if (d_log_open(&record, &d_log_code_net_connect_timeout, 0u))
{
    d_desc_emit(d_log_sink(&record), "host", NULL, d_ostream_value_text(h),
                D_DESC_LAYER_STATE, 0u, D_DESC_TRUTH_OBSERVED);
    d_desc_facts(D_DESC_OF(socket), &sock, d_log_sink(&record));
    d_log_close(&record);
}
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
  and flush, so a program can replace it (`d_log_set_writer`). Text, JSON
  lines and OTLP/JSON are built in; syslog, journald or a network exporter
  are a program's writers.
- The writer writes to a stream: standard error, a file appended to, or
  none (`log.destination`), or a program's own (`d_log_set_stream`). The
  stream is buffered and flushed at each record's end, so a record that fits
  is one write. With `D_CFG_LOG_THREADSAFE` (1) a lock around the writer and
  its stream keeps records whole. The lock and the first cache of settings
  are each made once (`d_call_once`), so threads that log first at the same
  moment are safe; call `d_log_init` before the program starts its threads,
  after its settings are read, and change no setting while they log (decision
  63).
- A record takes its time when it opens, when the event happened, and its
  observed time when it is written. No lock is held while a record is open,
  so a record may stay open across calls: an error whose note comes after
  it is one record, with the note a nested fact.
- The body is the code's template written from the record's facts by the
  stream's template engine; a field the template names that the record lacks
  is empty. A body past `D_CFG_LOG_BODY_MAX` is cut where a character
  starts.
- Numbers for machines never follow the locale; prose may group digits.

## The record's members: OpenTelemetry's log data model (decision 24 (b))

A record uses the data model's names: Timestamp, SeverityText,
SeverityNumber, Body and Attributes, the facts going in Attributes, the
code and its module among them as `code` and `module`. Each concept
djinterp lacked is a feature of its own: a knob builds it, an option turns
it on, and where it needs something from outside, a machine supplies it, so
a program can replace any part.

| Feature | Knob, default | Option, default | Machine |
|---|---|---|---|
| Severity number | always built | `log.severity_number`, on | decision 22's levels |
| Instrumentation scope: the module | `D_CFG_LOG_SCOPE`, 1 | `log.scope`, on | none: the module is known where the record is made |
| Resource: program, host, process, platform | `D_CFG_LOG_RESOURCE`, 1 | `log.resource`: off in JSON lines, where it would repeat on every line; on in OTLP, once per batch. `log.resource.<key>` adds or overrides one. | a resource provider; the default reads the environment report and the program's name |
| Trace and span ids, trace flags | `D_CFG_LOG_CONTEXT`, 0, since djinterp makes no traces | `log.context`, on when built | a context provider, which a tracing library supplies |
| Observed timestamp | `D_CFG_LOG_OBSERVED_TIME`, 1 | `log.observed_time`, off: the record's time is when it opened, the observed time when it was written | the clock |
| Dropped attributes count | with decision 25's cap | `log.dropped_count`, on | none |
| Timestamps | always built | `log.time`: UTC or local, and its precision | the clock: a test fixes it |
| Formats | `D_CFG_LOG_FORMATS`, all three | `log.format`: text (default), json, otlp | a writer |

OTLP/JSON follows the specification's JSON encoding (OTLP's "JSON
Protobuf Encoding", and protobuf's JSON mapping): one
ExportLogsServiceRequest a line, field names in lowerCamelCase, a 64-bit
integer (`timeUnixNano`, `intValue`) as a decimal string, an enum
(`severityNumber`) as its number, a double's NaN and infinities as
`"NaN"`, `"Infinity"` and `"-Infinity"`, and a nested fact as a
`kvlistValue`. JSON lines use the data model's names in the same case:
`timestamp` (RFC 3339), `observedTimestamp`, `severityText`,
`severityNumber`, `body`, `attributes`, `droppedAttributesCount`, `scope`,
`resource`.

Built but not yet a program's to fill: the resource holds `service.name`
(`log.service`, else `unknown_service`) and `process.pid`;
`log.resource.<key>` and a resource provider, and the context provider for
trace and span ids, come with the first program that needs them. The
observed time differs from the record's when a record is held open, or
waits in a queue.

## Nested facts (decision 25 (b))

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

## Message codes: tables and a manifest (decision 23 (c))

- **One table per enum of codes**, beside the enum, owned with the module
  and compiled out with it (decision 77): `net` and `db` each have one, and
  a submodule gets its own only when it has an enum of its own, as
  `db.vendor` does (decision 75). Each row is a number, a name, a class, a
  grade, a template and the facts it names. A table is a macro of two
  parameters, a row each, in the enum's header or in `<module>_codes.h`:

  ```c
  #define D_NET_LOG_CODES(X, m)                                          \
      X(m, 207, connect_timeout, RESOURCE_BUDGET, FAILED,               \
        "connecting to {host} took over {timeout_ms} ms",               \
        (host, timeout_ms))
  D_LOG_CODES_DECLARE(net, D_NET_LOG_CODES)        // in the log's unit
  D_LOG_CODES_DEFINE(net, "net", D_NET_LOG_CODES)  // in one source
  ```

  The table's `#define` is inert text and needs no include, so a module's
  header can hold it without depending on the log. `D_LOG_CODES_DECLARE`
  and `_DEFINE` need `log.h`, so they go in the unit that logs for the
  module (parse's subscriber, for instance), built only when the log is.
  The generator checks each row against the enum beside it: a constant
  whose name ends in the row's name, upper-cased, with the row's number.

  Class and grade are checked by the compiler. A template that does not
  compile is written as it is; `d_log_module_check` compiles each code's
  template against its facts and names the first that fails, for a module's
  tests to call. A module registers its table
  with `d_log_module_add` before settings are read, which also makes it a
  scope of the log's djin: `log.net.level`, with a dotted module's '.' as
  '_' (`log.parse_expr.level`).
- A code's full name is `module.name`; its number is unique in its module;
  the pair never changes meaning once released. A number is the value the
  module's enum gives the code, sign and all, so a record's number is the
  number the caller holds: db's codes are negative, so that they never meet
  a vendor's, and are written `db:-6` (decision 76).
- **A manifest CI generates and checks**: every code's module, number, name,
  class and grade, and every event's name, since events and codes share one
  key space (decision 64). CI fails on a duplicate number, a hash collision
  (decision 21), a name that is both an event and a code, or a released
  code whose meaning changed against the committed manifest, and the docs'
  list is made from it. The generator and its job come with the log; the
  job runs in the tree's CI. The generator also runs on a program's own
  tables, so a program gets the same guarantee.
- **A module the log is built on never logs** (decision 67): the containers,
  the stream, filesys, sync and the option core return their codes, and
  their callers log them, finding each by its number with
  `d_log_code_find`, and adding the facts only the caller knows. The
  module's table still sits beside its enum, as inert text. A module may
  log from inside - a broken invariant where it happens - only under a
  knob of its own, `D_CFG_<MODULE>_LOG`, 0 by default, and a run then
  chooses its records with `log.<module>.level`. Parse, above the log,
  logs through a subscriber unit of its own.
- **A vendor's own codes** are mapped to a small table of kinds beside an
  enum of them, `db.vendor.<kind>` (connection, auth, constraint, syntax,
  not found, busy, data, unsupported, internal), so a reader filters by
  class across vendors; the vendor's product, code and message stay as
  facts under OpenTelemetry's names (`db.system.name`,
  `db.response.status_code`). The mapping is a classifier per vendor that
  a program may replace, and the message, which can carry a user's data,
  may be left out by a knob and an option of the module's (decision 75).
- The status codes the modules return today (`D_OPTION_STATUS_*`,
  `D_OSTREAM_*`, `errno`) map onto codes, so an error reported to a caller
  and an error logged are the same thing in two places.
- A module's own code names (`parse.expr.unclosed`) are its own; the first
  are parse's: domain 4 of its diagnostics, its ten codes.

| Knob | Default | |
|---|---|---|
| `D_CFG_LOG_CODES` | 1 | 1 builds code names and templates; 0 keeps numbers only ("net:207") |
| `D_CFG_<MODULE>_LOG_CODES` | as `D_CFG_LOG_CODES` | the same, per module |

The option `log.code_style` writes a code by name, number or both, in text.

### Classes, sides and grades

The containers' table of error conditions (from the retired
`c/container/derror.h`, 11-04) gives the registry three things more, which
every module's codes share:

- **A class** for each code, from one vocabulary across modules:
  `argument.null`, `position.index`, `quantity.count`, `span.inverted`,
  `resource.memory`, `handle.stale`, `integrity.invariant` and the rest
  (`D_LOG_CLASSES` in log.h lists them all). A reader can then treat every
  module's out-of-range index alike. Ruled on 11-17: `constraint.violated`,
  for a construct that parsed and that a language's check refuses
  (decision 57); `quantity.depth`, `resource.budget`,
  `operation.unsupported`, `io.failed`, `lifecycle.closed` and the
  `format.*` family, as one set (decision 58); `lifecycle.frozen`, for a
  change refused by an object that is still in use (decision 66); and
  `io.timed_out`, `io.refused`, `io.unreachable`, `io.reset` and
  `io.would_block`, for a peer's conditions (decision 78). Ruled on 11-17
  as one set, for what a host or a server refuses or cuts short (decision
  81): `io.interrupted`, a call a signal cut short (its codes are soft:
  the caller tries again); `resource.in_use`, an address or a resource
  another holds; `access.denied`, a caller who is known but not allowed;
  and `access.unauthenticated`, a caller whose identity was not accepted.
  A class is part of a code's meaning, fixed when it compiles; a missing
  one is proposed and ruled before the table that needs it. A program's
  own tables may add classes of their own, `D_CFG_LOG_CLASSES_EXTRA`, after
  the vocabulary.
- **A side**, a standard fact for a value or a range against the valid
  interval: any of below (B), at its lower edge (L), within (W), at its upper
  edge (U) and above (A). An index past the end is `position.index {A}`; a
  range overlapping the start is `position.range {B,L,W}`.
- **A grade**, what the event did, which sets the log's default level: ok,
  note, caveat, soft (an expected miss: not found, empty), rejected (nothing
  changed), failed (tried, and could not), broken (an invariant does not
  hold). The table's effect marks become facts: `effect` (no-op, partial)
  and `retryable`.

**The syntax family**, for bad input text, which the containers' table has
no class for: five classes, each graded rejected, for every parser in the tree
to share. A module's own code says which fault it is; the class says what
kind:

- `syntax.unexpected`: a token where the grammar has none;
- `syntax.missing`: something required is absent;
- `syntax.unbalanced`: a pair not matched;
- `syntax.ambiguous`: an order the grammar will not choose;
- `syntax.arity`: the wrong number of operands.

Parse maps its seven syntax codes onto them.

Conditions the table marks undetectable (a comparator that is not a strict
weak order, a variadic count) get no code: they belong in a module's
documentation.

### Each grade's default level (decision 26 (b))

| Grade | ok | note | caveat | soft | rejected | failed | broken |
|---|---|---|---|---|---|---|---|
| Level | debug | info | notice | debug | warning | error | fatal |

A reader can tell a caller's mistake, which changed nothing, from the
library's failure, and successes stay out of info.
`D_CFG_LOG_GRADE_LEVEL(grade)` replaces the whole map when compiling, a
macro so that a record's default level is a constant where it is written,
and `D_CFG_LOG_LEVEL_MIN` can compile it out; one option per grade changes
it for a run, `log.grade_rejected=error`, and the same with a module's
scope, `log.net.grade_rejected`.

## The log's settings, as option_env rows

`log` is a djin, so every setting has a knob, an environment variable, a
command-line option and a provenance:

| Row | Kind | Meaning |
|---|---|---|
| `level`, `<module>.level` | int, named | the threshold: a level's name or a severity, 1 to 24 |
| `levels` | text | levels added at run time, `audit=14,security=19`, with `D_CFG_LOG_LEVELS 3`; named by their severity in `level` |
| `format` | choice | text, json, otlp; a writer not built refused by name |
| `destination` | text | stderr (the default), none, or a file's path |
| `code_style` | choice | name, number, both |
| `facts_max`, `depth_max` | int ranges | the caps, up to their knobs |
| `nesting` | choice | nested, dotted |
| `grade_<grade>`, `<module>.grade_<grade>` | int, named | a grade's level |
| `severity_number`, `scope`, `resource`, `context`, `observed_time`, `dropped_count` | bool | decision 24's features, where built |
| `time` | choice | utc, utc_ms (the default), utc_us, utc_ns, local, local_ms, local_us, local_ns |
| `json_integers` | choice | number (the default), string: decision 48 (d) |
| `json_nonfinite` | choice | null (the default), string: decision 49 (c) |
| `real_digits` | int, 0 to 17 | 0, the default: a real whole, the shortest text that reads back; 1 to 17 rounds each real to that many significant digits first, shorter but lossy (decision 51) |
| `service` | text | the resource's `service.name` |

Every row's default is its knob (`cfg_log.h`), and every row is per module
as well, through the module's scope.

## When it lands

- **Friday 11-13**: this contract, agreed.
- **Wednesday 11-18** (decisions 27 and 80): the log's header, its writers
  and its suite, with desc's text, JSON and collecting machines; then the
  lock and the first cache made once, a record's time when it opens,
  `d_log_module_check`, signed numbers, the classes of 11-17 and
  `log.real_digits`. Modules may adopt it from then.
- **Wednesday 11-18 or Friday 11-20**: events named beside their types
  (the event core's decision 15), with DTest's names, in the window DTest's
  half lands.
- **Week 3**: the codes generator and its CI job; the modules' code tables,
  each module its own.

## Settled

1. A record's event key: the name's hash, checked (decision 21 (c)), above.
2. The levels: a spectrum from none to added at run time, the seven built in
   (decision 22), above.
3. The record's members: OpenTelemetry's names, each missing concept an
   opt-in feature (decision 24 (b)), above.
4. Nested facts: from 0.8, with a cap (decision 25 (b)), above.
5. The codes registry: tables per module, and a manifest CI checks
   (decision 23 (c)), above.
6. The class vocabulary, from the containers' table with the syntax family,
   which parse agreed (11-11), and the classes of 11-17; each grade's default
   level, revised (decision 26 (b)), above. The containers report their
   errors with the codes this settles.
7. Code tables: one per enum of codes; numbers signed; modules below the
   log return codes and their callers log them; vendors' codes as kinds
   (decisions 64 and 67, 75 to 77), above.
