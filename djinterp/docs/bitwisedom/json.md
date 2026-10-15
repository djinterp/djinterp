# bitwis(e)dom's JSON

bitwis(e)dom reads three shapes of JSON, told apart by their `format`, and
writes all three:

| `format` | what it is | written by |
|---|---|---|
| `bitwisedom.sheet` | the window's own file: the bitfield and every setting the window keeps with it -- the grid, its colours, its widths -- to open as it was left | File > Save |
| `bitwisedom.bitfield` | the bitfield alone, as a person would write it: its width, its affixes and its rows, each in as few keys as say what it is | File > Export the bitfield as JSON; `bitwisedom-gen --bitfield` |
| `bitwisedom.export` | everything about it: the sheet whole, every row as it resolves -- its macro name, bits, value, the integer C knows it by, its error -- the issues, and the header | File > Export everything as JSON (Ctrl+Shift+E); `bitwisedom-gen --export` |

Any of the three opens with File > Open, dropped on the window, or pasted
with File > New sheet from the clipboard; `bitwisedom-gen` reads any of them
in place of a sheet file. A sheet file opens as itself. A bitfield or an
export becomes a new sheet beside the file it came from, named for it
(`uart_cr.bitfield.json` gives `uart_cr.bitwisedom.json`), and not yet saved.
An object that names no `format` is read as a bitfield, so the smallest
bitfield is a width and its rows:

```json
{ "width": 8, "rows": [ { "name": "READY", "bit": 0 } ] }
```

Numbers are JSON integers, or strings in any base C has -- `"0x0F00"`,
`"0b101"`, `"017"`, `"-3"` -- of any width. A number past 2^53, which a reader
that holds numbers as doubles would round, is written as a string of its
digits.

---

## A bitfield

```json
{
  "format": "bitwisedom.bitfield",
  "version": 1,
  "title": "uart_cr",
  "width": 32,
  "prefix": "UART_CR_",
  "rows": [
    { "group": "Enables", "note": "Switches for the UART's two halves.",
      "rows": [
        { "name": "EN", "bit": 0, "note": "enables the UART" },
        { "name": "TXEN", "bit": 1 }
      ] },
    { "group": "Format",
      "rows": [
        { "name": "PARITY", "bits": "4..5", "note": "the parity mode",
          "values": { "PARITY_NONE": 0, "PARITY_EVEN": 2,
                      "PARITY_ODD": { "number": 3, "note": "odd parity" } } },
        { "name": "DIV", "lsb": 16, "width": 12 },
        { "name": "DIV_115200", "field": "DIV", "number": 54 }
      ] },
    { "name": "DEFAULT", "expr": "EN | TXEN | PARITY_NONE" },
    { "name": "MAGIC", "value": "0x0F00" },
    { "name": "IS_ENABLED", "params": "x", "body": "(((x) & {EN}) != 0)" }
  ],
  "generator": { "file_name": "uart_cr.h" }
}
```

### The bitfield's keys

| key | is | if left out |
|---|---|---|
| `format` | `"bitwisedom.bitfield"` | read as one |
| `version` | `1` | 1 |
| `title` | the sheet's name | `"untitled"` |
| `width` | its bits, 1 to 65,536; past 64 the header keeps it in arrays of words (README, *Sheets wider than 64 bits*) | -- it must be there |
| `prefix`, `suffix` | the start and the end of every macro name | none |
| `trim_both`, `trim_left`, `trim_right` | comma-separated exact tokens removed repeatedly from both, leading or trailing ends of each node name *before* joining prefixes | none |
| `numbering` | `"lsb0"` or `"msb0"`: how the window numbers the bits (the header always counts from the least significant) | `"lsb0"` |
| `msb_left` | whether the grid draws the most significant bit at the left | `true` |
| `byte_order` | `"little"` or `"big"`: how the window numbers bytes | `"little"` |
| `probe` | the probe's value, a number of any width | 0 |
| `environment` | `#define`s the macros are evaluated in, which the header does not write | none |
| `rows` | its groups and symbols, in order | none |
| `segments` | the grid's named runs of columns, each `{"name", "bits", "collapsed", "colour"}` | one every few bits, as a new sheet has |
| `generator` | the header's settings, as a sheet file holds them -- `file_name`, `level` (`"c99"` ... `"cpp23"`), `form` (`"macros"`, `"constants"`), `style` (`"djinterp"`, `"plain"`), `radix`, `int_suffix`, `word`, `templates` and the rest; only those that differ from a new sheet's | a new sheet's |

### Rows

A row is a **group** -- `{"group": NAME, "rows": [...]}` -- or a **symbol**,
`{"name": NAME}` and one definition:

| kind | its definition | for example |
|---|---|---|
| bit | `bit`: its number | `{"name": "EN", "bit": 0}` |
| range | `bits`: `"lowest..highest"` (either way round), `"5"` for one bit, or `[lowest, highest]`; or `lsb` and `width` (1 if left out). `values` names its values | `{"name": "MODE", "bits": "4..6", "values": {"SLOW": 1, "FAST": 2}}` |
| value in a range | `number`, with `field`, the range by its name or its macro name -- or with `bits` of its own; `encoding` and `bias` | `{"name": "GAIN_DOWN", "field": "GAIN", "number": -1, "encoding": "twos_complement"}` |
| literal | `value`: a number, which may be negative and is then written as the number it is | `{"name": "MAGIC", "value": "0x0F00"}` |
| expression | `expr`: other symbols, by their names or macro names, with C's operators | `{"name": "ALL", "expr": "EN \| MODE"}` |
| macro | `body`, with `{NAME}` for any symbol's macro name; `params`; `callable` to write one with no parameters as `NAME()` | `{"name": "IS_ON", "params": "x", "body": "(((x) & {EN}) != 0)"}` |

A range's **`values`** are an object, each member a value in it, made a row
after it in the same group and named by its key -- its own name, so
`"FAST"` is `FAST` with the prefixes around it, not `MODE_FAST`. A member is
the value's number, or an object with its `number` and any of `encoding`,
`bias` and the keys below. The encodings are `"unsigned"`, `"twos_complement"`,
`"ones_complement"`, `"sign_magnitude"` and `"offset_binary"` (with `bias`).

Every row may have:

| key | is |
|---|---|
| `note` | its brief, in the header and the inspector |
| `short` | a short form of its name, for the grid when the name does not fit |
| `prefix`, `suffix` | its own affixes |
| `colour` | `"#RRGGBB"` or `"#RRGGBBAA"` |
| `emit` | `false` to leave it out of the header |
| `radix` | `"hex"`, `"dec"`, `"oct"`, `"bin"` or `"shift"`; `"inherit"` (the default) takes its group's |
| `int_suffix` | `"none"`, `"u"`, `"ul"` or `"ull"`; `"inherit"` (the default) takes its group's |
| `digits` | the fewest digits its literal is written with |

and a group, besides:

| key | is |
|---|---|
| `exclusive` | `false` lets its bits and ranges share bits; otherwise two claiming one bit collide |
| `isolate` | `true` for *own names only*: its rows take none of the affixes of the groups around it |
| `name_children` | `true` prepends this group's local name (after trimming) to each child's, cascading for nested groups; default `false` |
| `collapsed` | `true` to show it closed |

### Reading one

What a bitfield does not say keeps its default. A key a row does not have is
passed over and noted, with where it was, on the Issues page and from
`bitwisedom-gen`:

```
rows[5] (TOP): "nmae" is not a key it has, and was passed over
```

What it says wrongly is refused, with where, and nothing is made of the
file:

```
rows[0].rows[0] (BAD): two definitions, "bit" and "expr": a row has one
rows[1].rows[2] (DIV_115200): its "field", DIVISOR, names no row: unknown name 'DIVISOR'
width: a bitfield's width is a number of bits, 1 to 65536
```

What is wrong with the bitfield itself -- two bits that collide, a value
that does not fit its range, an expression's unknown name -- is the sheet's
business, and is on the Issues page as it would be for a sheet.

[`bitfield.schema.json`](bitfield.schema.json) is a JSON Schema (draft
2020-12) of the bitfield, for an editor that checks JSON as it is typed. It
is stricter than the reader: it refuses a key the reader would pass over,
so that a misspelt one shows at once.

### Writing one

A bitfield is written with every key the defaults would give left out: a
bit is `{"name": "EN", "bit": 0}`. A range takes the values that follow it in
its group as its `values`; a value elsewhere names its range by its macro
name. The grid's and the colours' settings stay in the sheet file; the
generator's that differ from a new sheet's are kept. Read back, a bitfield
writes the same header as the sheet it was written from.

### How it is laid out

All three shapes are written four spaces to an indent, an array of numbers
or strings on one line, and any other array or object on one line where the
line -- a hundred columns, the comma after it included -- has room for it,
and a member or an item to a line where not. A bitfield's row is a line,
note and all, unless its note is long:

```json
        {
            "group": "Control",
            "prefix": "CTL_",
            "rows": [
                { "name": "EN", "bit": 0, "note": "on" },
                {
                    "name": "MODE",
                    "bits": "4..6",
                    "values": { "SLOW": 1, "FAST": { "number": 2, "note": "fast" } }
                },
```

so a bitfield of a thousand rows is about a thousand lines, and a diff of
two says which rows changed. The reader takes any layout.
[`tools/bitfield_json.py`](../tools/bitfield_json.py) lays JSON out the same
way, for a script that writes bitfields: what it writes is what
`bitwisedom-gen --bitfield` writes back of it, byte for byte.

### Two large ones

`res/djinterp/tools/bitwisedom/examples/large` holds two bitfields made by scripts, which the window has
on File > Examples > Large and `bitwisedom-gen --example` has by name:

| file | what it is | made by |
|---|---|---|
| [`win32_hresults.bitfield.json`](../examples/large/win32_hresults.bitfield.json) | every HRESULT MinGW-w64's `winerror.h` defines, 1,376 codes, each a literal in a closed group for its facility, under the fields that make them: 1,424 rows | [`tools/winerror_to_bitfield.py`](../tools/winerror_to_bitfield.py), from that header |
| [`x86_cpuid.bitfield.json`](../examples/large/x86_cpuid.bitfield.json) | ten of CPUID's feature registers end to end, 320 bits: 212 flags, a 5-bit field, and x86-64's microarchitecture levels as expressions over them, the probe set to `x86-64-v3` | [`tools/x86_cpuid_bitfield.py`](../tools/x86_cpuid_bitfield.py), from its own table |

Each script writes the platform check beside its bitfield --
`tests/platform/win32_hresults.c`, every code and facility against the
`winerror.h` a Windows build has; `tests/platform/x86_cpuid.c`, every flag
against GCC's and Clang's `<cpuid.h>` -- and the tests read each file and
write it back as a bitfield, which must give the same bytes.

---

## An export

```json
{
  "format": "bitwisedom.export",
  "version": 1,
  "title": "uart_cr",
  "layout": { "width": 32, "numbering": "lsb0", "msb_left": true,
              "byte_order": "little" },
  "symbols": [
    { "id": 1, "kind": "group", "name": "Enables", "path": [],
      "note": "Switches for the UART's two halves.", "bits": "bits 0..3" },
    { "id": 2, "kind": "bit", "name": "EN", "macro": "UART_CR_EN",
      "path": ["Enables"], "note": "enables the UART",
      "lsb": 0, "msb": 0, "width": 1,
      "bits": "bit 0", "value": "0x1", "integer": "1" },
    { "id": 9, "kind": "value", "name": "PARITY_EVEN",
      "macro": "UART_CR_PARITY_EVEN", "path": ["Format"],
      "lsb": 4, "msb": 5, "width": 2, "bits": "bit 5", "value": "0x20",
      "integer": "32", "number": 2, "encoding": "unsigned" }
  ],
  "issues": [],
  "header": { "file_name": "uart_cr.h", "level": "c99", "form": "macros",
              "macros": 35, "skipped": 0, "notes": [], "text": "..." },
  "sheet": { "format": "bitwisedom.sheet", "...": "the sheet file, whole" }
}
```

| key | is |
|---|---|
| `layout` | `width`, `numbering`, `msb_left` and `byte_order`; and, for a sheet wider than 64 bits, `words`: the header's array words' `bits`, their `count`, and the prefix of the array `macros` |
| `symbols` | every row, groups too, in the tree's order |
| `issues` | each with its `level` (`"error"`, `"warning"`, `"note"`), the `symbol` it is about by macro name (`null` for the sheet's own), and its `message` |
| `header` | what the generator wrote: its `file_name`, `level`, `form`, how many `macros`, how many symbols `skipped` for their errors, its `notes`, and its `text` |
| `sheet` | the sheet, as its file holds it: what an export is read back as |

A row of `symbols` has:

| key | is |
|---|---|
| `id` | its number in the sheet, which other rows' `field` use |
| `kind` | `"group"`, `"bit"`, `"range"`, `"value"`, `"literal"`, `"expression"` or `"macro"` |
| `name`, `macro` | its own name, and its macro name (not for a group) |
| `path` | the names of the groups it is in, outermost first |
| `note`, `emit` | its brief; `emit` only when `false` |
| `lsb`, `msb`, `width` | its bits' span: a bit's, a range's, a value's range's |
| `bits` | its bits as runs, for a person: `"bits 4..5"`, `"bits 0, 3 and 70"` |
| `value` | its bits, in hex, of any width: `"0x30"` |
| `integer` | the integer C knows it by, in decimal: its value, or -- a negative literal -- the number; in a sheet wider than 64 bits, a bit's or range's place, the bits a value's field holds, a literal's number. None for an expression of a wide sheet, nor a literal past bit 63 |
| `number`, `encoding`, `bias` | a value's number and how its range holds it |
| `negative` | a literal written as the negative number it is |
| `expr` | an expression's text |
| `params`, `body`, `ctype`, `expansion` | a macro's parameters and body, the C type of what it evaluates to, and its name fully expanded |
| `error` | why it is left out of the header |


## Nested ranges and symbolic expressions

A range can own nested ranges, bits, and expressions by using a `rows`
array. Every nested bit or range must lie entirely within its parent's bit
interval (both endpoints inclusive). Nested ranges and their containing
ranges intentionally overlap; non-contained sibling intersections still
produce a warning. `name_children: true` prepends that range's name to
all descendants, and defaults to false for existing sheets.

For example, a range called `D_ERR_` may contain a range `ARRAY_`, which
contains `INDEX_` and then an expression `OOB` with generated name
`D_ERR_ARRAY_INDEX_OOB`. A symbol's `expr` can name other symbols and
uses C integer operators and precedence. A backslash-newline continuation
in an expression is accepted, and the generator emits a C `#define`
containing the symbolic references.

`transparent: true` on a symbol or group disables its colored cell fill;
this is separate from `colour` being unspecified (automatic palette).
The **New** command creates a 32-bit sheet with a full-width range named
`root`, ready to receive nested rows. **Shift+drag** on the grid selects
a rectangle of cells and rows without editing their bit definitions.
Binary values in the inspector and grid tooltip are ungrouped.
