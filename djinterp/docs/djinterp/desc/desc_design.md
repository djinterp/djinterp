# desc: describing anything, facts first and text last

Lane E's design note, 2026-11-04, for week 2's core and the interfaces agreed
on Friday 11-13. It builds on desc.txt (the owner's idea and the first C++
sketch) and on what the tree already has: the stream's templates
(`c/util/ostream_template.h`), the environment printer
(`env/env_printer.h`) and option_env with its registry.

## What desc is

A module that describes any object in words, at any length, for three uses
from one source: a log line, `--help`, and a debugger's view. "This is a C++
tree map; it stores std::string keys mapped to 64-bit signed hash key values.
This one contains 42 elements, has no max size, and uses a custom allocator
and a custom comparator." The library does the work: every mechanical fact is
derived, and only intent is written by hand.

## The rule: facts first, text last

A describer never writes a sentence. It emits **facts**, and a renderer turns
facts into text, JSON, markdown, CSV or key=value lines. The environment
printer is already this for one type -- its report is sections of entries,
each a name, a label and a value -- so desc generalises its report into a
stream of facts, and every type gets every format the printer has.

### A fact

```c
struct dj_desc_fact
{
    const char*             key;         // "element_count": a path part
    const char*             label;       // "elements": for prose; may be null
    struct dj_ostream_value value;       // the stream's tagged value
    uint16_t                layer;       // enum dj_desc_layer
    uint8_t                 importance;  // 0 the headline .. 3 the detail
    uint8_t                 truth;       // enum dj_desc_truth
    uint32_t                depth;       // nesting, for contents
};
```

- **Layers**, which a reader selects: identity, type, state, intent, policy,
  health, provenance, contents, cost.
- **Importance** orders facts within a layer, so a one-line description is
  the facts of importance 0 and the sentence adds 1.
- **Truth** says what kind of fact it is, so a careful description can label
  it: *guaranteed* (the standard requires it), *implementation* (true of
  this library's or this compiler's choice: "tree map"), *build* (fixed by
  configuration: `D_INTERNAL_EVENT_QUEUE`), *run time* (an option, with its
  source), *observed* (state now: "42 elements").

`struct dj_ostream_value` is the stream's own value (text, integers, reals,
booleans, none), so a fact needs no conversion to be written by a template.

## Describers

Common Lisp's `describe` has the shape: `describe` calls `describe-object`,
each type supplies its own, and nesting, depth limits and cycle detection come
from the caller, provided each describer handles exactly one level.

- **C**: a describer is a function over the object and a sink of facts,
  registered for a type by name; C describers read the descriptors C already
  has (`dj_type_info16`, a kv schema's rows, an option_env row), so most are
  short. Each lives next to its type, in that type's own unit, so it is
  renamed with the type and a program that never describes links none.
- **C++**: `desc::describer<T>` is a class template, specialised beside the
  type, deriving types (`numeric_limits` gives "64-bit signed"), policies
  (an allocator compared with `std::allocator` gives "custom") and state
  (`size()`). Intent is the one hand-written part:

```cpp
template<>
struct desc::role<hash_key>
{
    typedef std::int64_t underlying;
    static const char* name() { return "hash key"; }
};
```

- **Grammar** is the renderer's, not a describer's: "a"/"an", "1 element"
  against "42 elements", lists joined with "and", digit grouping (1,234) --
  in prose only: JSON gets 1234, since numbers for machines never follow the
  locale.

## Rendering

Facts are rendered by the stream's templates through the printer's machine:
the five presets, `select` rules over `layer.key` paths, the `show_*`
switches. Prose is one more preset whose templates join facts into sentences
with the grammar helpers. Week 2 moves the printer's report onto facts; the
printer's own presets keep printing the environment as they do now, so its
harnesses are the regression test of the change.

## desc's own settings, as option_env rows

desc is a djin, `desc`, so `DJINTERP_DESC_LEVEL`, `--desc.level` and a
script all work with no code of desc's own, each with its knob:

| row | kind | values |
|---|---|---|
| `level` | choice | name, line, sentence, detailed, everything |
| `layers` | flags | identity, type, state, intent, policy, health, provenance, contents, cost |
| `format` | choice | the printer's presets and prose |
| `depth`, `items`, `width` | int ranges | the limits on nesting, a container's items, a line |
| `preset` | choice | help, log, debug: a level, layers and format together |

## Cost

- A program that never describes anything pays nothing: C describers are
  separate units, C++ describers templates, and a knob,
  `D_CFG_DESC_ENABLED`, removes desc from a build.
- The first sentence is the same for every instance of a type, so it is built
  once per type (at compile time where C++ allows); a call reads state only.
- No allocation: facts go to a caller's sink; the renderer writes to a stream.

## Three uses from one source

- **Logging**: a log record carries desc facts, rendered as a line for people
  and as JSON for machines (`docs/djinterp/log/log_design.md`).
- **Help**: `--help` is built from schema rows and describers, so it cannot
  disagree with the code.
- **Debugging**: desc can be called from a debugger now; later the same
  describers can generate natvis files and gdb pretty-printers.

## To agree by Friday 11-13

1. The fact record's members, and whether `importance` and `truth` share a
   byte.
2. The layer list, and whether a module may add layers of its own (proposed:
   no; a module adds keys, not layers).
3. Where a describer is registered: by type name in a registry beside the
   djin registry (proposed), or found by the C++ type alone.
4. Which stable modules get describers first (proposed: memory, filesys,
   option, test, db and net, the order of week 3's adoption).
