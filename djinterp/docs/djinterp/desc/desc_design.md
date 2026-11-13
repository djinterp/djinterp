# desc: describing anything, in prose and in facts

Lane E's design, for the agreement of Friday 11-13 (E29): the APIs of all
three of desc's layers -- the units, the facts and the macro layer -- as
they are in the tree, and the C++ face over them. It records the owner's
rulings of 11-12: E20 (describer forms and machines), E28 (d) (one authored
form), E30 (a) (op arrays), E31 (c) (a declaring macro per family), E32 (b)
(dmacro's tools), E33 (c) (where a describer is found) and E34 (a) (the
order of adoption). It builds on desc.txt (the owner's idea and the first
C++ sketch) and on what the tree already has: the stream's value and
templates (`c/text/ostream_template.h`), the environment printer and
option_env with its registry.

## What desc is

A module that describes any object in words, at any length, for three uses
from one source: a log line, `--help`, and a debugger's view. The library
does the work: every mechanical fact is derived from what the type already
knows, and only intent is written by hand.

## Two engines, one authored form

desc has two engines, both built by default (`D_CFG_DESC_UNITS`,
`D_CFG_DESC_FACTS`):

- **Units** write prose for people: a type's table of fill-in-the-blank
  parts, chosen when the type is compiled and by a plan read from the
  object's values, then written once, front to back. This is the default
  for a description a person reads.
- **Facts** are typed values for machines: key, label, value, layer,
  importance and truth, into a sink, and from the sink to a machine that
  makes text, JSON, a log record's attributes, or lane G's help.

A type is authored once (E28 (d)). It gives desc:

- a **field list**: each value's key, label, layer, importance and truth;
- one **reader**, which reads a field by its index in that list;
- a **plan**, which reads the values that choose units and grammar;
- **units**, which name the fields they say.

Prose comes from the units, and facts from the field list, through the
same reader, so the two cannot disagree; a field nobody asked for is never
read. A hand-written fact describer is needed only for nested contents,
such as a container's elements.

| File | What it holds |
|---|---|
| `inc/djinterp/c/desc/desc.h` | facts, machines, sinks, both describer forms |
| `inc/djinterp/c/desc/desc_units.h` | rings, slots, op arrays, units, fields, the type |
| `inc/djinterp/c/desc/desc_options.h` | desc's djin, `desc` |
| `inc/djinterp/c/desc/desc_macro.h` | the macro layer |
| `inc/djinterp/cpp/desc/desc_units.hpp` | the C++ face |
| `inc/djinterp/config/c/desc/cfg_desc.h` | the knobs |
| `src/djinterp/c/desc/desc_units.c` | the units engine, and the derived facts |
| `src/djinterp/c/desc/desc_options.c` | the djin |

## A type's description

```c
struct dj_desc_type
{
    const char*                 name;         // an identifier: its options'
                                              // scope, desc.<name>.ring
    const char*                 noun;         // its name in prose
    const struct dj_desc_unit*  units;        // null when none are built
    size_t                      unit_count;
    const struct dj_desc_field* fields;       // null when facts are not built
    size_t                      field_count;
    fn_desc_plan                plan;         // may be null: no bits
    fn_desc_read                read;         // may be null: no fields
    fn_desc_describer           contents;     // nested contents; may be null
};
```

It is a const object beside the type's own unit, named
`d_desc_type_<name>`, so `D_DESC_OF(name)` finds it, and a program that
never describes the type links none of it. `D_DESC_UNITS_TABLE(table)` and
`D_DESC_FIELDS_TABLE(table)` give a table's pointer and count, or null and
0 where the knobs leave that engine out, so the tables may sit inside
`#if D_CFG_DESC_UNITS` and `#if D_CFG_DESC_FACTS`.

```c
typedef uint32_t (*fn_desc_plan)(const void* _object, uint32_t _ring);
typedef struct dj_ostream_value (*fn_desc_read)(const void* _object,
                                                uint32_t    _field);
```

The value is lane D's `struct dj_ostream_value` (text, integers, reals,
booleans, none), which units and facts share, so a value needs no
conversion to be written by a template.

## 1. The units (`desc_units.h`)

### Rings

A ring says how much is said, and each includes the ones inside it:

| Ring | Says | Example |
|---|---|---|
| `D_DESC_RING_NAME` | the noun and what completes it, with no article | shelf |
| `D_DESC_RING_LINE` | its condition and what it has | a shelf with 3 books |
| `D_DESC_RING_SENTENCE` | its policies, and a sentence more | an ordered shelf of books, with 3 books. Its first book is Emma. |
| `D_DESC_RING_DETAILED` | what takes a walk to know, and its limits | an 8-slot ordered shelf of books, with 3 books. ... It holds at most 8 books. |

`D_DESC_RING_OPTION` asks for what `desc.ring` says; a ring past the last
built is the last built; a unit whose ring is `D_DESC_RING_NEVER` is never
shown.

### Layers

A unit is on one of desc.h's nine layers -- identity, type, state, intent,
policy, health, provenance, contents, cost -- so units and facts are
selected the same way. A reader hides the layers it does not want; the
noun, which names the type, is always written.

### Slots, in a sentence's order

A unit goes in a slot, and the slots are in the order a person says the
parts:

| Slot | Holds | Example |
|---|---|---|
| CONDITION | how it is now | empty, full |
| SIZE | its size | fixed-size, 8-slot |
| QUALITY | its qualities | immutable, dusty |
| SHAPE | its arrangement | ordered |
| ORIGIN | its origin, its style | Windows-style |
| NOUN | what it is, always written | shelf |
| COMPLEMENT | what completes the noun | of books, mapping int to text |
| WITH | what it has | with 3 books and a label |
| REMARK | a sentence after | Its first book is Emma. |

Adjectives of one kind are coordinate and take commas between them;
adjectives of different kinds take none: "a full dusty, old shelf".
Complements take commas; the units of WITH join with ", " and a last
" and ", after ", with" when there are complements; each remark is a
sentence.

### Op arrays and units (E30 (a))

A unit's text is an array of ops the compiler lays out, so nothing is
parsed or compiled at run time, and a field named by a misspelt constant
does not compile. Units that say the same thing share one array.

```c
static const struct dj_desc_op OPS_BOOKS[] =
    { D_DESC_F(SHELF_BOOKS), D_DESC_T(" book"), D_DESC_PL, D_DESC_END };
```

| Op | Writes |
|---|---|
| `D_DESC_T(text)` | a literal |
| `D_DESC_F(field)` | a field's value, read when the op is reached |
| `D_DESC_PL` | "s", unless the last number written was 1 |
| `D_DESC_WORDS(one, many)` | the first after a 1, else the second |
| `D_DESC_END` | ends the array |

A unit points at its array (24 bytes on x86-64):

```c
struct dj_desc_unit
{
    const struct dj_desc_op* ops;      // shared; ends with D_DESC_END
    uint32_t                 on;       // plan bits it needs
    uint32_t                 off;      // plan bits it must not have
    uint8_t                  ring, layer, slot, article;
};

#if D_DESC_BUILT(LINE, STATE)
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_STATE, D_DESC_SLOT_WITH,
                0u, SHELF_EMPTY, OPS_BOOKS),
#endif
```

A unit shows when its ring is in force, its layer is wanted, and the plan
has every bit of `on` and none of `off`. `D_DESC_BUILT(ring, layer)`, with
the names unprefixed, tells `#if` whether the build has a ring and a layer,
so a row the knobs leave out is not compiled and its text is not in the
binary. (desc.h's layers are enum constants, which `#if` cannot read; the
names map to numbers in `desc_units.h`, and a misspelt one is reported by
`-Wundef`.)

### Write once, front to back

A description is made in two steps, and nothing written is ever changed:

1. **Decide**, from values only: the ring and layers in force, the plan's
   bits, how many units show in each slot, and which unit leads.
2. **Write** each slot's units in order, each after its separator, which the
   counts already decided. The article is chosen from the lead unit's first
   word or number ("an 8-slot", "an empty"; `D_DESC_UNIT_ARTICLE` gives a
   unit its own: "a unique"), and that field is read once for both; a
   capital, asked for with `D_DESC_CAPITAL`, changes the first letter on
   its way out.

### Describing

```c
int d_desc_say(const struct dj_desc_type* type, const void* object,
               uint32_t ring, uint32_t layers, uint32_t flags,
               fn_desc_put put, void* context);
```

The text goes to desc.h's `fn_desc_put`, only ever appended to. It returns
0 when written, 1 when the put asked to stop (the put is not called again),
and -1 for a null argument or a build without units. desc gives two puts:
`d_desc_put_buffer`, into a caller's `struct dj_desc_buffer`, which keeps
what fits and a NUL after it and stops when full; and `d_desc_put_file`,
into a `FILE*`.

## 2. The facts (`desc.h`)

### A fact

```c
struct dj_desc_fact
{
    const char*             key;         // "element_count": a path part
    const char*             label;       // "elements": for prose; may be null
    struct dj_ostream_value value;       // the stream's tagged value
    uint32_t                depth;       // nesting, from the sink
    uint16_t                layer;       // enum dj_desc_layer
    uint8_t                 importance;  // 0 the headline .. 3 the detail
    uint8_t                 truth;       // enum dj_desc_truth
};
```

- **Layers**, which a reader selects: the nine above. A module adds keys,
  not layers.
- **Importance** orders facts within a layer, so a one-line description is
  the facts of importance 0, and the detail is 3.
- **Truth** says what kind of fact it is: *guaranteed* (the standard
  requires it), *implementation* (this library's or compiler's choice),
  *build* (fixed by a knob), *run time* (an option, with its source),
  *observed* (the state now).

### The machine (E20)

The work is a machine's: `struct dj_desc_machine` is a table of six
operations, each of which may be null and is then skipped:

```c
struct dj_desc_machine
{
    fn_desc_begin begin;  // a description starts
    fn_desc_fact  fact;   // one fact the sink wanted
    fn_desc_enter enter;  // a level opens: its key, label and depth
    fn_desc_leave leave;  // the level closes
    fn_desc_text  text;   // words from a text describer
    fn_desc_end   end;    // the description is over, stopped or not
};
```

Each operation that returns int returns 0 to go on and anything else to
stop. desc's own machines -- text, JSON, a collecting one, a log record's --
are built on this table, and a program writes any other. A table is
usually a static const, shared by every sink that writes to that kind of
machine.

### The sink

A describer emits into a `struct dj_desc_sink`, which the caller owns,
usually on its stack. The sink holds the machine and its context, and what
the reader wants: a mask of layers, the least important fact wanted, and a
depth limit. The helpers are inline:

- `d_desc_sink_init` sets a sink to want everything, eight levels deep; with
  no machine, it wants nothing;
- `d_desc_wants` lets a describer skip the work of a fact nobody asked for;
- `d_desc_emit` sends a fact when the sink wants it, at the sink's depth,
  and returns 1 once the sink has stopped. A fact the sink does not want is
  skipped, and emitting goes on;
- `d_desc_text` gives a text describer's words to the machine's `text`, or,
  to a machine without one, as one fact: `text`, on the intent layer, at
  importance 0;
- `d_desc_enter` opens a named level (a JSON machine writes `"key": {`
  there), and refuses when the sink does not want contents, has reached its
  limit or has stopped, so a describer that meets an object again ends;
  `d_desc_leave` closes it;
- `d_desc_run` and `d_desc_run_text` describe an object in either form:
  begin, the describer, then end, which is always called, so a machine can
  close what begin opened.

### Derived facts

```c
int d_desc_facts(const struct dj_desc_type* type, const void* object,
                 struct dj_desc_sink* sink);
int d_desc_run_facts(struct dj_desc_sink* sink,
                     const struct dj_desc_type* type, const void* object);
```

`d_desc_facts` emits `type`, the type's noun, on the type layer; then each
field the sink wants, in the list's order, read only then; then the type's
nested contents, through its hand-written `contents` describer, which opens
its own level. `d_desc_run_facts` adds the machine's begin and end.

### Describer forms (E20)

A type may give any of these, and every reader works with every one:

- **A field list and units** (the default, E28 (d)): facts and prose from
  one authored form.
- **A fact describer**, `fn_desc_describer`: the only form that can
  describe nested contents, with `d_desc_enter` and `d_desc_leave`.
- **A text describer**, `fn_desc_text_describer`, which writes words
  through a put; `d_desc_run_text` gives them to any machine.

## 3. The macro layer (`desc_macro.h`)

The preprocessor makes a description, as a string literal, of what a type
and the knobs fix; no code runs, so it is a constant: a const array, a
`static_assert`'s message, a `#pragma message`, or a unit's text, so the
run-time units and the macro layer share words.

- **`#if` words**, which need nothing but `#if` and `D_CONCAT`, and so build
  in every language, C++98 included. `D_DESC_MACRO_BUILD` is desc's own
  build in sentences: "Units are built to the detailed ring, in every layer;
  facts are derived. Articles are on, and options are read at run time."
  (`D_CFG_DESC_AT_BUILD 1` prints it while `desc_units.c` compiles.) A word
  that a knob alone decides belongs here (E32).
- **The kit**, built where the language has variadic macros (C99, C++11),
  on dmacro.h's tools (E32 (b)): `D_CONCAT`, `D_IIF`, `D_CHECK` and
  `D_PROBE`. The sequence joins are desc's own.

```c
D_DESC_MACRO_SENTENCE(condition, size, quality, shape, origin, noun,
                      complements, remarks)
D_DESC_MACRO_SENTENCE((a, "full"), (a, "fixed-size"), , (an, "ordered"), ,
                      (a, "tree"), ("of 8 keys"), ("It is balanced."))
  -> "a full fixed-size ordered tree of 8 keys. It is balanced."
D_DESC_MACRO_NAME((a, "radix tree"), ("mapping int to text"))
  -> "radix tree mapping int to text"
D_DESC_MACRO_PLURAL(n)      "" for 1, "s" else; n a plain number
D_DESC_MACRO_IS_ZERO(n), D_DESC_MACRO_IS_ONE(n)    1 or 0
D_DESC_MACRO_WHEN(bit, words)                      words when bit is 1
```

An adjective part holds one or more words, `(a, "dusty")(an, "old")`; the
determiner, "a" or "an", is the first word's own, and is left out with
`D_CFG_DESC_ARTICLES 0`.

**A declaring macro per container family (E31 (c)).** A container family
declares its type and its description from one argument list, so nothing
is written twice:
`D_BRT_DECLARE(route_table, uint64_t, route, 0, (immutable)(ordered))`.
The families' lanes (C, H) write these on the kit in week 3, with the
two-macro form -- the type's own macro and its `D_DESC_MACRO_` twin --
underneath, for a user who wants no typedef.

## The C++ face (`desc_units.hpp`)

From C++98. A type `T` gives its description as `units<T>`, a
specialization whose `type()` returns its `struct dj_desc_type`, and its
name in prose as `type_name<T>` (`D_DESC_TYPE_NAME`), which a container's
units use to name its parameters: "a fixed-size bag of int". Describing a
type that gives neither fails to compile.

- `pick<Keep, Literal>` keeps a literal only for a type that keeps it, so
  a literal a type rules out is never referenced and is not in the binary;
  `D_DESC_RING_IF(keep, ring)` puts the unit that would say it in no ring.
- `say(object, put, context, ring, layers, flags)`, `describe(object, ...)`
  into a `std::string`, and `facts(object, sink)`.

## Knobs (`cfg_desc.h`)

| Knob | Default | Builds |
|---|---|---|
| `D_CFG_DESC_ENABLED` | 1 | desc at all |
| `D_CFG_DESC_UNITS` | 1 | the units engine, and a type's units |
| `D_CFG_DESC_FACTS` | 1 | the derived facts, and a type's field list |
| `D_CFG_DESC_RING_MAX` | 3 | the outermost ring built |
| `D_CFG_DESC_RING` | 1 | the ring when nobody says; `desc.ring`'s default |
| `D_CFG_DESC_LAYERS_BUILT` | 0x1FF | the layers whose units are built |
| `D_CFG_DESC_LAYERS` | 0x1FF | the layers shown when nobody says |
| `D_CFG_DESC_ARTICLES` | 1 | "a" and "an", and the code that chooses them |
| `D_CFG_DESC_OPTIONS` | 1 | desc's djin |
| `D_CFG_DESC_SCOPES` | 16 | the types that may have options of their own |
| `D_CFG_DESC_BINDINGS` | 64 | the djin's bindings |
| `D_CFG_DESC_MACRO` | 1 | the macro layer |
| `D_CFG_DESC_AT_BUILD` | 0 | desc's build sentence, printed while compiling |

With the machines on Monday 11-16 come `D_CFG_DESC_STYLE` (which engine
`describe` uses when nobody says), `D_CFG_DESC_TEXT`, `D_CFG_DESC_FORM`
and `D_CFG_DESC_MACHINES` (E20, E28); with the name registry in week 3,
`D_CFG_DESC_REGISTRY` (E33).

## Options: desc's djin (`desc_options.h`)

desc is a djin, `desc`, so a settings file, `DJINTERP_DESC_RING` and
`--desc.ring` all work through the registry, each variable's default its
knob:

| Variable | Kind | Values |
|---|---|---|
| `ring` | choice | name, line, sentence, detailed |
| `layers` | flags | identity, type, state, intent, policy, health, provenance, contents, cost, all, none; joined by '\|', and '+' or '-' to edit the value in force |
| `<type>.ring` | choice | as `ring`, for one type |

A type declared to desc with `d_desc_declare` is a scope of the djin, so
`desc.shelf.ring=detailed`, from a file or the command line, sets its ring
alone; declare a type before reading the settings that name it. (The
environment's import reads unscoped variables only, as option_env's import
does for every djin.) `d_desc_register(NULL)` registers the
djin in the program's registry. Without the djin (`D_CFG_DESC_OPTIONS 0`,
or below the option core's 64-bit floor) the knobs decide alone.

With the machines on Monday come `style`, `form` and `format`, per run and
per type (E20, E28).

## Where a describer is found (E33 (c))

At compile time by default: `D_DESC_OF(name)` in C, `units<T>` in C++. No
start-up cost, and a missing description fails to compile. A program that
wants describing by name -- a debugger, lane G's interpreter -- opts into a
name registry in week 3: `D_CFG_DESC_REGISTRY` 0 none, 1 the types a
program registers (the default), 2 every described type.

## Cost

- A program that never describes anything pays nothing: a type's tables
  live beside it, the engine is two units, and `D_CFG_DESC_ENABLED 0`
  removes desc from a build.
- A ring, a layer or articles left out by the knobs leaves out their code
  and their text.
- No allocation, but the C++ face's `describe`, which returns a
  `std::string`: text goes to a caller's put, and facts to a caller's sink.

## Three uses from one source

- **Logging**: a log record carries desc facts, written as a line for
  people and as JSON for machines (`docs/djinterp/log/log_design.md`).
- **Help**: `--help` is built from schema rows and descriptions, so it
  cannot disagree with the code.
- **Debugging**: desc can be called from a debugger now; later the same
  descriptions can generate natvis files and gdb pretty-printers.

## Order of adoption (E34 (a))

Week 3's describers follow the roadmap's order: memory, filesys, option,
test, db, net.

## When each part lands

- **Friday 11-13**: desc.h's core with its machines; the units engine and
  its derived facts; desc's djin; the macro layer; the C++ face; their
  suites (`c_desc_tests`: 17 tests, 121 checks; the C++ face's harness,
  `tests/unintegrated/cpp/desc/t_desc_units.cpp`, from C++98).
- **Monday 11-16**: desc's text and JSON machines, with the log's code;
  the `style`, `form` and `format` options and their knobs.
- **Week 3**: the name registry (E33); describers in E34's order; the
  declaring macros per family, with lanes C and H (E31).

## Settled

1. The fact record's members: as `desc.h` has them, a byte each for
   importance and truth, since sharing one would not make the record
   smaller (64 bytes either way on x86-64). Lane G takes the sink and the
   machine table for its djins' `describe` (its E-8).
2. The layers: desc.h's nine, and no others; a module adds keys.
3. Where a describer is found: E33 (c), above.
4. Which modules get describers first: E34 (a), above.
