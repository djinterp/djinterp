# bitwis(e)dom

A bitfield manager -- `bitwisedom` in code, on disk and on the command line. A **sheet** is a word of 1 to 64 bits -- or wider, to 65,536, held
in C as an array of words -- and a tree of **symbols**, each one row: a name
mapped to a value, shown across the bits it sets, and written out as a C or
C++ macro. Groups collapse like the nodes of a
tree, runs of columns collapse to one, and long names abbreviate to fit. An
operation -- a shift, a rotation, a byte swap, arithmetic -- is previewed as
the bits it moves and the collisions it makes before it is applied.

It reads C as well as writing it: a header, or code pasted from one, becomes
a sheet, its macros evaluated as C evaluates them -- expanded in a define
environment, typed, and checked for what C would do wrong.

It is built on djinterp (the model's integers and bit functions come from
`re_std`, and C is read with its lexer), with its window drawn by uxoxo over
Dear ImGui. Building it is described in [BUILD.md](BUILD.md).

![The uart_cr example: PARITY selected, a shift previewed](screenshots/operations-preview.png)

---

## The sheet

A sheet has a **width** (1 to 64 bits, the widest a C integer literal can be;
or wider, to 65,536 -- [below](#sheets-wider-than-64-bits)), a **name prefix
and suffix** that every macro name takes, a **bit numbering**
(LSB 0 or MSB 0, for the screen; the generated header always counts from the
least significant bit, as C does), a **byte numbering** for byte ranges and
labels, and which side the most significant bit is drawn on.

Rows are **groups** or **symbols**. A group holds rows, carries a prefix and a
suffix of its own, and can be collapsed. A **range** symbol can also hold
other ranges, bits, expressions and groups. Its children are indented in the
tree, and contained ranges may overlap their parents completely (inclusive
boundaries), while partial overlaps remain flagged. New documents start with
an editable full-width range named `root`. A symbol can use **No fill** in
its inspector to remain transparent instead of receiving an automatic or
custom fill; the grid's binary displays have no digit separators. Hold
**Shift** and drag across bit cells to select a rectangle of rows and bits;
ordinary drags continue editing the current row. Each containing group or
range may opt in to prepending its own name to descendant names. A range or
group that holds children can be collapsed. A group whose bits are *exclusive*
warns when two of its bit or range symbols claim the same bit; one that is
*own names only* ignores the affixes of the groups around it. A collapsed
group's rows are neither drawn nor built, in the tree or the grid, so a
sheet of thousands of rows in closed groups is as quick to work in as a
small one; the HRESULT example, 1,424 rows, arrives with its facilities
closed. **Expand all** and **Collapse all**, under the Symbols panel's add
buttons and on the View menu, open or close every group at once, in the
tree and the grid together; a group's right-click menu does the same for
the group and the groups within it. Whether a group is open is kept with
the sheet, and is not a step to undo.

A symbol's value comes from one of six definitions:

| kind | defined by | written, by default, as |
|---|---|---|
| bit | one bit | `#define D_EN 0x00000001u` |
| range | a lowest bit and a width (or a byte range) | `#define D_MODE 0x00000070u` |
| value in a range | a number, placed in a range in one of five encodings | `#define D_MODE_FAST 0x00000020u` |
| literal | a number, positive or negative | `#define D_DEFAULT 0x1234u`, `(-3)` |
| expression | other symbols, with C's operators | `#define D_RW (D_READ \| D_WRITE)` |
| macro | parameters and a body, written as given | `#define D_IS_SET(x) (((x) & D_EN) != 0)` |

**Names** are built from affixes: the sheet's prefix, each enclosing group's,
outermost first, then the row's own; the suffixes close in reverse order.
`UART_CR_` + `PARITY` gives `UART_CR_PARITY`. A name the result makes invalid
in C, or reserved to the implementation (`_Capital`, `double__underscore`), is
reported.

**Literals** are written in a radix -- hex, decimal, octal, binary, or
*shifts* (`(1u << 3)`, `(0x7u << 4)`) -- with an integer suffix (`u`, `ul`,
`ull`, or none), set for the sheet and overridden by any group or row.

### Negative and positive

A value in a range is a number in one of five encodings, each with its range
shown beside it:

| encoding | in 4 bits, -3 is | holds |
|---|---|---|
| unsigned | (does not fit) | 0..15 |
| two's complement | `1101` | -8..7 |
| ones' complement | `1100` | -7..7 |
| sign-magnitude | `1011` | -7..7 |
| offset binary (bias 8) | `0101` | -8..7 |

A number that does not fit is a warning, not an error: it still has bits --
the ones it wraps to -- and they are shown. A literal may be negative, and is
then written as the number it is: `#define D_ONES (-1)`.

### Expressions

C's integer operators at C's precedence -- `~ - + !`, `* / %`, `+ -`,
`<< >>`, `&`, `^`, `|` -- over literals in any base C has, the names of other
symbols (local or full), parentheses, and `BIT(n)`, `MASK(w)` and
`GENMASK(high, low)`. Arithmetic is unsigned and 64 bits wide. An expression
keeps its form in the header, every name replaced by its macro name; a cycle
is reported as the chain it is (`cycle: LA -> LB -> LA`). The expression
editor accepts multiline input and C backslash-newline continuations, so
`D_ERR_ARRAY_INDEX_OOB` may be defined as
`D_CODE_ARRAY_FIELD | (D_CODE_ARRAY_IDX_LO | D_CODE_ARRAY_IDX_HI)`.
Expression evaluation uses 64-bit integer arithmetic, even for wider sheets.

### Sheets wider than 64 bits

A sheet may be wider than any C integer -- a configuration space, a CPU's
feature bits, a bitmap of hundreds of flags -- up to 65,536 bits. Its values
are sets of bits as wide as the sheet; the grid, the operations, the probe
and the files work on them as they do on a narrow sheet's, and the value
column names a set by its runs (`bits 0..15`, `bits 3, 70 and 128..131`). Its
expressions are taken over sets of bits: the bitwise operators, `+`, `-` and
negation within the sheet's width, literals of any width, and `BIT(n)` past
bit 63; a product or a shift needs one side an integer, and a quotient two.

No C integer holds such a sheet, so its header keeps it in an **array of
words** -- 32 bits by default, or 8, 16 or 64 (the Macros page's *array
words*) -- lowest first, bit *n* being bit *n* % 32 of word *n* / 32. The
header begins with what works on such an array, named with the sheet's
prefix: `_WORD_BITS` and `_WORDS`, and `_WORD(n)`, `_MASK(n)`, `_TEST(a, n)`,
`_SET(a, n)` and `_CLEAR(a, n)` for a bit by its number. Then each symbol is
what names a place in the array:

| kind | in a wide sheet's header |
|---|---|
| bit, range | its place: the number of its (lowest) bit, `#define D_CMD_IO 32` |
| value in a range | the bits its field holds, as its range's `_GET` reads them: two's complement -1 in 8 bits is `0xFFu` |
| literal | the number, while an integer holds it; past bit 63, an array's initializer |
| expression | an array's initializer, `{ 0x00000000u, 0x00000007u, ... }` |

A wide sheet has **templates of its own**, on the Templates page in place of
a narrow sheet's: a range's `_WIDTH`, its `_WORD` and the `_SHIFT` and
`_MASK` of its bits there, and `_GET(a)` and `_SET(a, v)`, which read and
write the range in an array -- word by word, where it crosses from one word
to the next; a bit's `_WORD`, `_MASK` and `_IS_SET(a)`, and a value's
`_IS(a)`, to switch on. A range wider than an integer has no `_GET` or
`_SET`, and the notes say so. In C++'s constants form, places are `int`s,
values as wide as their fields, and sets arrays of the words' fixed-width
type: `inline constexpr std::uint32_t D_ENABLE[D_WORDS] = { ... };`.

The `pci_config_header` example is one: a PCI function's 64 bytes of
configuration header, sixteen 32-bit registers, each field and bit at its
byte offset times eight -- `linux/pci_regs.h`'s offsets and masks, which the
tests compare it with on Linux. `x86_cpuid`, on the Large shelf, is
another: ten of CPUID's feature registers end to end, so word *n* of its
array is the *n*th register as CPUID returns it, and `CPUID_TEST(regs,
CPUID_AVX2)` asks the processor's answer whether it has AVX2; x86-64's
microarchitecture levels are expressions over the flags, and so arrays.

![pci_config_header: 512 bits, read from byte 0, and its header's array macros](screenshots/wide-sheet.png)

---

## The grid

Each row is a symbol; each column a bit. A cell is filled where the row's
value has the bit set. A range's or a value's field is tinted underneath, so a
value's zeros still show where it lives; a range's name, or a value's number,
runs across its span. A group's row shows the union of everything in it.

![rv32i_itype in the light theme: signed immediates, a negate previewed](screenshots/light-theme-signed-fields.png)

**Clicking** a name selects its row (Ctrl adds, Shift extends); clicking a
group's arrow opens or closes it. Clicking a cell edits the row's bit by its
kind: a literal flips the bit, a bit moves to it, a value flips it within its
field, and a range stretches to take it in. **Dragging** across a row's cells
sets a range exactly, or paints a literal's bits. Rows can be dragged within
the tree to move them.

**The probe** is a scratch value in the grid's top row: click its cells or
type a value, and the operations can act on it too.

**The usage row** at the bottom counts, for each bit, how many bit and range
symbols claim it -- red where two in an exclusive group do.

### Grid lines

Six kinds, each with its own switch, thickness, colour and style (solid,
dashed or dotted), and a master switch on **G**:

* between bits,
* every *n* bits (*n* from 2 to 32: 4 for nibbles, 8 for bytes, 3 for octal),
* between rows,
* above groups,
* at the edges of column segments,
* around the grid.

Cell width and height, the gap between cells and their rounding are on the
same page.

![posix_mode: octal values, major lines every three bits](screenshots/grid-lines-octal.png)

### Collapsing and abbreviating

**Column segments** name runs of columns: the sheet's own (Columns tab, or
over a selected range), and automatic ones by nibble, byte or half-word. Click
a segment's heading to collapse it to one narrow column, which shows each
row's bits there as hex, a dot, a count, or nothing. Clicking an automatic
segment makes it one of the sheet's, collapsed.

Runs of bits no visible row uses can be **abbreviated** to one `..` column.

**Names** that do not fit their space are shortened, in order: the row's short
name, if it has one; its initials (`PARITY_EVEN` is `PE`); as much of it as
fits, then `..`. The same applies to a range's name across its cells and to
segment headings.

### Colour

The **Theme** menu holds the window's two choices of colour, and they are the
window's: kept in its settings file, not in the sheet, so every sheet opens
in them.

* **The theme**, dark or light, sets the window's surfaces and the grid's
  empty cells and lines.
* **The colour scheme** -- vivid, pastel, earth, neon, or colour-blind safe
  (Okabe and Ito's seven colours, which stay apart for the common
  colour-vision deficiencies) -- sets the palette rows take their colours
  from, the accent, and the roles: collision, probe, selection, hover, the
  preview's outline. Earth warms the surfaces, neon cools and deepens them.
  Each scheme has a dark and a light version.

Each row has a colour -- its own, or one from the palette -- and set bits are
coloured by their row, their group, their kind, or the accent. A sheet's
colours are **automatic** until it sets one in the Colours tab -- a palette
of its own, a saturation, an accent, a grid line's colour -- and what it sets
it keeps, in either theme and every scheme. **Use the scheme's colours**
(the Colours tab, or the Theme menu) gives them all back.

### Names, menus and widths

**Renaming** happens where the name is: double-click a name in the tree or
the grid, press **F2**, or choose **Rename** from the row's menu, and type
over it. Enter renames; Escape, or a click elsewhere with nothing typed,
gives up. The inspector's name field renames as well, once its edit is
done. Whatever named the row follows it -- an expression's names, a macro's
body where it names the macro, a macro's `{NAME}` placeholders -- found with
djinterp's lexer, so a string, a comment or a macro's parameter that only
shares the spelling is left alone. A rename is one step of undo. A name that
would not be a C identifier once the affixes are added, or that is another
symbol's, is refused, and the status line says why. A segment is renamed the
same way, by a double-click on its heading or from its menu.

**Right-click** opens a menu for what is under the mouse: a row, in the tree
or the grid (rename, copy as code, paste code after it or into it, add,
duplicate, delete, move, put its value in the probe, show its issues); a
segment's heading (rename, collapse or expand, remove); or the sheet (add,
paste code, copy the whole sheet as code).

**Columns resize** by their edges, which light up under the mouse. Drag the
edge after the name or value column, or double-click it to fit the column to
its text; drag any boundary in the ruler of bit numbers to widen or narrow
every bit's cell at once, or double-click one to put the cells back. A sheet
just imported has its name column fitted to its names. The grid's widths are
the sheet's, and are saved with it.

**The grid fills its panel.** The width it has beyond what its columns need
goes to the bits' cells, widened alike, so a 32-bit sheet in a wide window
has wide cells and no empty space beside them; the cells' width is then the
least they take, and a sheet too wide for the window scrolls. The Grid lines
tab's **Room to spare goes to** gives it to the name column instead, or to
nothing -- and then the rows end where the columns do.

**Under a resting mouse**, what is squished, hidden or abbreviated says what
it is: a name in the grid gives its full macro name, what it is and its
brief; a value, itself in hex, decimal, signed and binary; a band, its bits
and what a click does; the ruler, the bit or the run of bits below it; a
collapsed or abbreviated column, the bits it holds. Anywhere in the window, a
label cut off by its column ends in `...` and shows itself whole, as does a
value longer than its field, a combo's choice, a check box's or a tree row's
name past the panel's edge.

The columns of every table -- the inspector's, the issues', an operation's
-- resize by their edges too, and the window remembers their widths, the
sizes of its panels, how they are shown, the theme and the colour scheme,
from one run to the next, in a settings file of the user's:
`%APPDATA%\bitwisedom\bitwisedom.ini` on Windows, and
`$XDG_CONFIG_HOME/bitwisedom/bitwisedom.ini` (by default
`~/.config/bitwisedom/`) elsewhere.

### Panels

The symbols, the grid and the inspector lie flush, divided by hairlines --
drag one to size the panels either side of it. The **View** menu hides and
shows the symbols panel and the inspector.

The tabbed panel below the grid can be shown five ways, from the buttons at
the end of its tabs or **View > Bottom panel**:

| | |
|---|---|
| docked | below the grid, sized by the hairline above it |
| minimized | to a strip of its tabs; a tab pressed brings it back at that page |
| minimized until the mouse is over it | a strip of tabs, and a rest of the mouse on the strip opens the page over the grid, until the mouse leaves both |
| floating | in a window of its own, moved and sized where it is wanted, and kept there |
| hidden | not at all; **Ctrl+J** brings it back as it was, and hides it again |

![win32_hresult with the panel minimized until the mouse is over it, opened over the grid](screenshots/panel-peek.png)

---

## Operations

Select rows (a group stands for every symbol in it), pick an operation and an
operand, and the Operations page previews it:

* the **flow view** -- the distribution of the sheet's bits before and after,
  with a curve from each moved bit to where it lands, a cross on each that
  falls off the end and a mark on each zero filled in;
* the targets' **values** before and after, the bits lost and gained, and how
  each definition will be rewritten;
* the **collisions** before and after;
* the result outlined in the grid itself.

Shift left and right, arithmetic shift, rotate left and right, complement,
and, or, xor, and-not, add, subtract, multiply, negate, swap bytes, reverse
bits, and set, clear or toggle one bit. **Apply** keeps each definition's kind
where the result fits it: a shifted bit is still a bit, a range moved without
wrapping is still a range, a value moved within its field is still a value,
and an expression is wrapped in the operation (`(EN | TXEN) << 1`) -- unless
its operands moved with it, when it is left as written. Otherwise the symbol
becomes a literal of its new bits. Every applied operation can be undone.

---

## Macros

The Macros page shows the header as it will be written, coloured with the
grid's colours, and writes it beside the sheet file (**Export**, Ctrl+E) or
to the clipboard (**Copy**).

![The Macros page: shifts, no banner, no briefs](screenshots/macros.png)

* **Language level**: C99, C11, C17, C23, C++98 to C++23. The level decides
  what may be written: a binary literal needs C23 or C++14, `ull` needs a
  `long long`; what a level cannot honour is replaced by the nearest it can,
  and a note says so.
* **Form**: macros, or -- for C++ -- constants (`inline constexpr` from
  C++17, `constexpr` from C++11, `const` before), optionally in a namespace.
* **Style**: *djinterp* follows the house style guide -- the banner at its
  fixed geometry, a table of contents, numbered sections for groups, a
  two-line brief over every macro, aligned values, an include guard derived
  from the file name, lines kept within 80 columns -- or *plain*.

**Templates** are macros written beside every symbol of the kinds they apply
to. The defaults give every range `_SHIFT`, `_WIDTH`, `_GET(x)` and
`_SET(x, v)`; `_BIT`, `_IS_SET(x)` and `_IS(x)` are there to switch on. Add
your own on the Templates page, with these placeholders in the name, the
parameters and the body:

| placeholder | is |
|---|---|
| `{NAME}` | the symbol's macro name |
| `{LOCAL}` | its own name |
| `{VALUE}` | its value, as a literal |
| `{MASK}` | its mask (a value's: its field's macro name) |
| `{SHIFT}`, `{WIDTH}`, `{BIT}` | its lowest bit, its width, its bit |
| `{PREFIX}`, `{SUFFIX}` | the affixes its name took |

A [wide sheet](#sheets-wider-than-64-bits)'s templates have more:

| placeholder | is |
|---|---|
| `{WORD}`, `{WORD_SHIFT}`, `{WORD_MASK}` | the word a bit or range begins in, where in it, and its bits there |
| `{WORD_BITS}` | a word's width |
| `{FIELD}` | a value's range's macro name |
| `{GET}`, `{PUT}` | the range read from an array named `a`, and `v` written into it |
| `{BITSET}` | the prefix of the array macros |

No parameters makes an object-like macro. A **macro row** is the same thing
for one macro: parameters and a body, with `{NAME}` for any symbol's macro
name -- and, with no parameters, *function-like* to write `NAME()`. A body
over several lines is written in the style guide's layout: a backslash at
column 80 on every line but the last, and parameters that do not fit one
line one to a line, aligned under the first. A literal can be given a least
number of digits, so `0x0A00` stays four.

Symbols in error stay out of the header, and the notes say which.

---

## Code in and out

![The attached code imported: its macros evaluated, what C would do wrong in the issues](screenshots/import-define-environment.png)

**File > Open sheet, bitfield or header** opens a sheet, or reads a header or
a source -- anything that is not JSON -- into a new one, saved beside it
under its name; JSON is read as the [JSON](#json) section says. A file
**dropped on the window** is opened the same way. **File > New sheet from the
clipboard** makes a sheet of what was copied, code or JSON, and **Paste code** (Ctrl+Shift+V, or a row's menu) merges it into the
open sheet: after the selected row, or into a selected group. A name the
sheet already has takes the pasted definition. A pasted name keeps the macro
name it had: where it lands, the affixes it would take are taken off its own
name -- `UART_CR_EXTRA` pasted into `uart_cr` is the row `EXTRA` -- and one
without them is noted, since it will take them.

Code is read as a preprocessor reads it -- by djinterp's lexer, at the sheet's
language and level, its conditionals followed -- and each #define in a
branch taken becomes a row:

| written | becomes |
|---|---|
| `0x0A00`, `42u`, `(-3)` | a literal |
| `(1u << 3)` | a bit |
| `(0xFu << 4)` | a range |
| anything else: `D_A \| D_BIT(3)`, `F(x)` | a macro, verbatim |

-- but a literal, a bit or a range only when the header would write it back
exactly as it was written, digits, case and suffix and all; anything else
stays a macro, written as it was, its lines and their indentation kept. The
comment above a definition becomes its note. A rule of `=` begins a group,
titled by the heading under it, or else named for what its symbols' names
share. An include guard, or `#pragma once`, becomes the generator's setting.
What was left out -- declarations, code -- the Issues page lists.

**Copy as code** (Ctrl+Shift+C, or a row's menu) puts the selected rows on
the clipboard as their #defines, without the header's banner, contents and
guard: ready to paste into a file of one's own. **Export** writes the whole
header.

### The define environment

A macro row has the value its name has in C. The name is expanded as the
preprocessor expands it -- hide sets, `#` and `##`, `__VA_ARGS__` and
`__VA_OPT__` -- in an environment of definitions: the standard headers'
(`<stdint.h>`, `<limits.h>`, `<stdbool.h>`), the sheet's own
**environment** (on the Macros page: #defines from elsewhere -- another
header, a compiler's `-D` options -- that the sheet's macros use and its
header does not write), and every row of the sheet. What it expands to is
then evaluated with C's types: each literal's own, casts and the fixed-width
typedefs, the integer promotions and the usual arithmetic conversions, at
the sheet's language level and **data model** (LP64 for Linux and macOS,
LLP64 for Windows, ILP32). The inspector shows a macro's value, its C type,
and what it expands to; in the grid, an object-like macro with a value is
drawn as any value is, and one that takes arguments shows its signature
across the bits.

What C would get wrong is reported. Undefined behaviour -- a negative or
too-wide shift, signed overflow, division by zero -- is a warning, traced
through the macros it came out of (`D_BYTE_BIT_R(3, 0): shift count -4 is
negative`), and leaves the macro no value; implementation-defined behaviour
is a note. So are a macro called with the wrong number of arguments; a name
nothing defines, with the one meant when one is near (`did you mean
D_CODE_ARRAY_INDEX_VALID?`); an `|` that adds no bit; a `?:` whose arms
convert to their common type, so that a cast in one arm does not choose the
result's type; and a body that is not in parentheses.

---

## Issues

The Issues page lists everything the sheet's check found, by row: names that
are not C identifiers, reserved names, names defined twice, bits outside the
width, numbers an encoding cannot hold, unknown names and cycles in
expressions, overlaps in exclusive groups, and what the define environment
found in the macros (above). What the last import or paste had to say is
listed after them. Click a symbol to select it; right-click it for its
menu.

---

## Files

A sheet is saved as JSON (`*.bitwisedom.json`): its fields, its rows as a tree,
its segments, and its grid, colour and generator settings, so it opens looking
as it was left. Enumerations are words and colours `#RRGGBBAA`; values, of any
width, are 0x-prefixed strings, which no JSON reader rounds. A missing key keeps its
default, so older files open; a newer version is refused with the reason. A
colour the sheet leaves automatic is not written; a file from before the
theme was the window's wrote every colour, and reads with its defaults
automatic again and its own choices kept.

The examples are in `res/djinterp/tools/bitwisedom/examples/`, and on the File menu; the two large ones
are bitfields, in `res/djinterp/tools/bitwisedom/examples/large`.

### JSON

A sheet file is the window's own. For other programs -- a script that makes
bitfields from a datasheet, a generator for another language, a review of
what changed -- there are two more shapes of JSON, both read wherever a
sheet is (File > Open, a drop, the clipboard, `bitwisedom-gen`):

* **A bitfield** (`*.bitfield.json`, File > Export the bitfield as JSON, or
  Copy the bitfield as JSON) is the bitfield alone, as a person would write
  it -- its width, its affixes, and its rows, each in as few keys as say
  what it is:

  ```json
  { "title": "dev", "width": 16, "prefix": "D_", "rows": [
      { "name": "EN", "bit": 0, "note": "on" },
      { "name": "MODE", "bits": "4..6", "values": { "SLOW": 1, "FAST": 2 } },
      { "name": "ALL", "expr": "EN | MODE" } ] }
  ```

  A key a row does not have is passed over and noted with where it was; a
  row that says two things, or nothing, is refused with its path --
  `rows[1] (MODE): two definitions, "bit" and "bits": a row has one`. Written
  from a sheet and read back, it writes the same header.
* **An export** (`*.export.json`, File > Export everything as JSON,
  Ctrl+Shift+E) is everything: the sheet whole, the layout, every row as it
  resolves -- its macro name, its bits, its value in hex, the integer C knows
  it by, its error -- the issues, and the header's text. Read back, it is the
  sheet it holds.

All three are written a row to a line where the row fits in a hundred
columns, so a bitfield of a thousand rows is about a thousand lines, and a
diff of two shows which rows changed; `tools/bitfield_json.py` lays out
JSON the same way, for a script that writes bitfields.
[json.md](json.md) describes both key by key, and
[bitfield.schema.json](bitfield.schema.json) is a JSON Schema of the
bitfield for an editor that checks JSON as it is typed. The two large
examples, in `res/djinterp/tools/bitwisedom/examples/large`, are bitfields made by scripts -- one of
1,424 rows, one of 320 bits -- to read as examples of the shape.

---

## Examples

File > Examples holds twenty-one sheets, on shelves, each with its size
beside it. They vary in width (one byte to sixty-four, and past it), in
density (every bit named, or a few scattered through the word), in length
(a dozen rows to fourteen hundred), and in sign: some fields are unsigned,
some two's complement or offset binary, and some values negative.

![File > Examples > Linux: three sheets, with their sizes](screenshots/examples-menu.png)

| shelf | example | bits | what it shows |
|---|---|---|---|
| Hardware | `uart_cr` | 32 | a peripheral's control register: enables, ranges and their named values |
| | `usb_request_type` | 8 | one byte, every bit in a field: a USB request's direction, type and recipient, and the standard requests |
| | `lm75_temperature` | 16 | a signed field: nine bits of two's complement at the top of the word, -55 C to +125 C in half degrees |
| | `x86_eflags` | 32 | a sparse word: flags around a two-bit privilege level, a bit that always reads as one, ten reserved bits collapsed |
| | `x86_64_pte` | 64 | every bit named: flags at both ends, a 40-bit frame number collapsed to one column, protection keys and NX |
| | `pci_config_header` | 512 | wider than any integer: a PCI function's configuration header, sixteen registers, its header's arrays checked against `linux/pci_regs.h` |
| Formats | `rv32i_itype` | 32 | an instruction: opcode, registers and a 12-bit two's complement immediate |
| | `ieee754_single` | 32 | a float: the exponent in offset binary (bias 127), and 1, pi, `FLT_MAX`, the infinities and a NaN, bit for bit |
| | `rgb565` | 16 | a dense pixel: three channels, every level and colour drawn in the colour it is |
| POSIX | `posix_mode` | 16 | a file's permission bits, its setuid, setgid and sticky bits, and the usual modes |
| Windows | `win32_hresult` | 32 | severity, flags, facility and code, the codes built from them as `winerror.h` builds them |
| | `win32_window_style` | 32 | the high half all styles, two of them under two names, and the combinations |
| macOS | `macos_macho_flags` | 32 | a dense word of flags: 29 of a Mach-O header's 32 bits |
| | `macos_cpu_type` | 32 | a signed `int`: the ABI's bits over the family, and `CPU_TYPE_ANY` as -1 |
| Linux | `linux_ioctl` | 32 | an ioctl number's four fields, the `_IO` family as macros, commands old and new |
| | `linux_epoll_events` | 32 | sparse: events in the low bits, modes in the top four, nothing between |
| | `linux_capabilities` | 64 | 41 capabilities in eight bytes, each with its number as a `_BIT` macro |
| FreeBSD | `freebsd_file_flags` | 32 | the owner's half and the superuser's half of a file's flags |
| | `freebsd_kevent` | 16 | a `short`'s negative filters, -1 to -15, beside the flags' unsigned bits |
| Large | `win32_hresults` | 32 | every HRESULT `winerror.h` defines: 1,376 codes, a closed group for each of 22 facilities, under the fields that make them -- every facility a value of the facility field, the DirectX ones with the X bit beside it |
| | `x86_cpuid` | 320 | ten of CPUID's feature registers end to end: 212 flags, the MPX address-width adjust as a field among them, and x86-64-v1 to v4 as sets of them; the probe is a v3 processor, and the grid reads from bit 0, the registers in the order of the rows |

![The freebsd_kevent example: negative filters, each drawn as its two's complement bits](screenshots/example-kevent.png)

The two on the Large shelf are bitfields, in `res/djinterp/tools/bitwisedom/examples/large`, which the
build embeds, and which scripts in `tools/` made: `winerror_to_bitfield.py`
reads MinGW-w64's `winerror.h` (public domain) for every HRESULT, and
`x86_cpuid_bitfield.py` writes CPUID's flags from its own table, after the
Intel SDM and AMD's APM. A closed group's row shows the union of its
codes, so each facility's bits read at a glance before it is opened.

![win32_hresults: twenty facilities closed, WIN32 and DXGI open, E_ACCESSDENIED selected -- DXGI's codes with the X bit set](screenshots/example-hresults.png)

The platform examples are checked against the platform: where the build
targets Windows, macOS, Linux or FreeBSD, each of that platform's examples
is generated and compiled beside the system's own headers, and each value
the system also defines is compared at compile time (`tests/platform/`) --
on Windows, all 1,376 HRESULTs and their facilities among them. Where GCC
or Clang targets x86, `x86_cpuid`'s flags are compared with `<cpuid.h>`'s
masks, under every name either compiler gives them; Clang's `bit_PKU` is
passed over, since Clang's header has had it at bit 2, UMIP's. At C++98,
which has no `long long`, the 64-bit examples' constants are `unsigned
long`s, which hold them only where `long` has 64 bits; the generator says
so, on the Macros page and from `bitwisedom-gen`, and the C++98 header tests
skip them on Windows and 32-bit targets.

---

## The command line

`bitwisedom-gen` is the generator without the window, for build scripts:

```
bitwisedom-gen <sheet.json | --example NAME> [options]
  -o, --out FILE        write the header to FILE (default: stdout)
  --self-test FILE      write a compile-time self-test of the header
  --bitfield FILE       write the sheet as a bitfield (JSON)
  --export FILE         write everything about it (JSON)
  --lang c|cpp          --form macros|constants
  --level c99|c11|c17|c23|cpp98|cpp11|cpp14|cpp17|cpp20|cpp23
  --style djinterp|plain
  --word 8|16|32|64     a wide sheet's array words, in bits
  --date YYYY.MM.DD     the banner's date (default: today's)
  --check               report the sheet's issues; write nothing
bitwisedom-gen --save-example NAME FILE
bitwisedom-gen --list-examples
```

The sheet may be any of the three shapes of JSON -- a sheet file, a
bitfield or an export -- so a bitfield written by a script goes straight to
a header. Asked for `--bitfield` or `--export` and not for `-o`, it writes no
header to stdout. It exits 0 when it did what it was asked, 1 when the sheet
has errors (the header is still written, without them), and 2 for a usage or
file problem.
The self-test is a C or C++ file that includes the header and does not compile
unless every macro has the value the sheet gives it, and every range's `_GET`
and `_SET` agree with its named values. A wide sheet's arrays are not
constants C can check as it compiles, so its self-test checks them as it
runs: every array against the words the sheet gives it, every bit set,
tested and cleared through the array macros, and every field written and
read back, alone and among bits all set. It returns 0 only if each check
holds, and names any that does not.

---

## Keys

| key | does |
|---|---|
| Ctrl+Z, Ctrl+Y (or Ctrl+Shift+Z) | undo, redo |
| Ctrl+O, Ctrl+S, Ctrl+E | open, save, export the header |
| Ctrl+Shift+E | export everything as JSON |
| Ctrl+B, Ctrl+G | add a bit, add a group |
| Ctrl+D, Del | duplicate, delete the selected rows |
| Alt+Up, Alt+Down | move the selected row |
| F2 | rename the selected row |
| Ctrl+Shift+C, Ctrl+Shift+V | copy the selection as code, paste code |
| Ctrl+J | hide the bottom panel, and bring it back |
| Ctrl+Shift+Plus, Ctrl+Shift+Minus (or Ctrl with the keypad's + and -) | expand every group, collapse every group |
| G | grid lines on and off |

| with the mouse | does |
|---|---|
| double-click a name | renames it, in the tree or the grid |
| double-click a segment's heading | renames the segment |
| right-click | the menu for the row, segment or sheet under the mouse |
| drag, double-click a column's edge | resizes the column, fits it to its text |
| drag, double-click a ruler boundary | sizes every bit's cell, puts them back |
| drop a file | opens it: a sheet, or a header read as code |

Edits that arrive as a stream -- a drag, typing -- undo as one step.
