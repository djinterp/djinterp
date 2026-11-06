# The event and log contract

Lane E's design note, 2026-11-04, for week 2 and the interfaces agreed on
Friday 11-13: what a log record is, how it is written, and how it relates to
events, message codes and desc.

## A log record is an event, a message code and desc facts

```text
record = event occurrence      (c/event: a dj_event_key and its payload)
       + message code          (the status codes registry, v1)
       + desc facts            (docs/djinterp/desc/desc_design.md)
       + time, level, origin   (where and when)
```

- **The event** is what happened, as the event system already names it: a
  `dj_event_key` -- for a log record, the key of its message code's name,
  content-derived (`d_event_key_of_name`, FNV-1a 64), so C and C++ and two
  runs agree -- and its occurrence's payload. Logging is then one more
  subscriber: a sink is a handler on the log's dispatcher, and anything that
  listens for events can listen for log records with no second mechanism.
- **The message code** says what kind of thing happened, stably, for
  machines: a registry of codes, each a module, a number, a name, a level and
  a one-line template ("net.connect_timeout": "connecting to {host} took over
  {timeout_ms} ms"). Codes are the contract: text may be reworded, a code's
  meaning may not.
- **The facts** are the record's data, as desc emits them: key, value, layer,
  importance. The message's template names facts, so the text is rendered
  last, from facts, like every other desc output.

## Writing a record

```c
D_LOG(net, connect_timeout, D_LOG_FACT_TEXT("host", host),
                            D_LOG_FACT_INT("timeout_ms", ms));
```

- The level check comes first and costs a cached read
  (`d_option_env_cached`, the knob-and-option pattern's hot path): a record
  below the level in force builds nothing.
- Facts are collected into a fixed array on the caller's stack; nothing
  allocates on the logging path.
- The record is dispatched to the log's sinks, each a stream with a format:
  text for people (one line: time, level, code, message), JSON for machines
  (one object per line: `time`, `level`, `code`, `module`, `message` and
  every fact by key), or any template of the printer's language.
- Numbers for machines never follow the locale; prose may group digits.

## The log's settings, as option_env rows

`log` is a djin, so every setting has a knob, an environment variable, a
command-line option and a provenance:

| row | kind | meaning |
|---|---|---|
| `level` | choice | trace, debug, info, notice, warning, error, fatal |
| `format` | choice | text, json, or a template |
| `destination` | text | stderr, a file, none |
| `modules` | text | per-module levels: rules over module names, `net=debug, -db` |
| `<module>.level` | choice | a module's level, as a scope of the log's djin |

`D_CFG_LOG_LEVEL` sets the default; `D_CFG_LOG_COMPILED_LEVEL` removes calls
below it from the build entirely, as the cheapest level check is none.

## Message codes: the registry, v1

- One table per module, generated or hand-written beside the module, each
  row: number, name, level, template, the facts it names.
- A code's full name is `module.name`; its number is unique in its module;
  the pair never changes meaning once released.
- The status codes the modules return today (`D_OPTION_STATUS_*`,
  `D_OSTREAM_*`, `errno`) map onto codes, so an error reported to a caller
  and an error logged are the same thing in two places.
- A module's own code names (`parse.expr.unclosed`) are its own; the first
  is lane F's: domain 4 of parse's diagnostics, its ten codes.

### Classes, sides and grades

Lane C's table of container error conditions (from the retired
`c/container/derror.h`, 11-04) gives the registry three things more, which
every module's codes can share:

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

Conditions the table marks undetectable (a comparator that is not a strict
weak order, a variadic count) get no code: they belong in a module's
documentation.

## To agree by Friday 11-13

1. A record's event key: the content hash of the code's full name
   (proposed), or a registered handle; this is the event system's open
   question 1, and the log's answer should be the event system's.
2. The level names, and whether `notice` is one.
3. The JSON record's member names (proposed above).
4. Whether a log record may carry a nested object's facts (desc's contents
   layer) or only flat facts in 0.8 (proposed: flat in 0.8, nested in 0.9).
5. The code registry's form: a C table per module (proposed) or one
   generated file for the tree.
6. The class vocabulary, from lane C's table, and each grade's default log
   level (proposed: note and ok as info, caveat as notice, soft as debug,
   rejected and failed as error, broken as fatal); lane C reports container
   errors with the codes this settles.
