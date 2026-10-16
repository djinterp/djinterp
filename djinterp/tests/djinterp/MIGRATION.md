# djinterp — document / pdf relocation + consolidation

40 files. Drop `core/` and `test/` over `inc/djinterp/`.

---

## 1. Relocation

```
inc/djinterp/core/
  functional/cofree_recursive.hpp        <- shipped under document/ in the zip;
                                            its banner and its ./functor,
                                            ./recursion, ./cofree includes only
                                            resolve in functional/
  text/                                   unchanged (printer, writer, font,
                                            text_align, text_template,
                                            text_pattern, markup_string_template,
                                            section, xml/)
  util/document/                          document*, layout*, report_*
  util/document/templates/                the renderers + the templates
  util/pdf/                               pdf*
```

`layout*.hpp` and `report_*.hpp` are **flat** in `core/util/document/` — their own
banners declare that path, and their `../../djinterp.hpp` /
`../../../parse/parse.hpp` pairs only resolve at that depth. The zip's
`document/layout/` folder was packaging, not intent.

33 include paths and 34 banner `path:` / relative-to notes rewritten. Full
before/after table was in the previous manifest; the short version:

- `core/text/pdf/` → `core/util/pdf/` is depth-neutral, so only the five sibling
  edges moved (`../font.hpp`, `../writer.hpp`, `../text_template.hpp`,
  `../text_align.hpp`, `../../util/color/color.hpp`) plus the one cross-edge into
  `templates/`.
- `core/text/*.hpp` → `core/util/document/*.hpp` is one level deeper, so
  `../djinterp.hpp` → `../../djinterp.hpp`, `../util/X` → `../X`,
  `./printer.hpp` → `../../text/printer.hpp`.
- DTest: 7 edges across `test_output.hpp`, `test_render_pdf.hpp`,
  `test_render_pdf_table.hpp`.

### Guards and tags

All moved files: `DJINTERP_TEXT_*` → `DJINTERP_UTIL_*` (and the
`DJINTERP_UTILITY_DOCUMENT_BUNDLE_` outlier → `DJINTERP_UTIL_DOCUMENT_BUNDLE_`),
banner tag `djinterp [text]` → `djinterp [utility]`. All 31 guards verified
unique — the previous set would have collided with any stale copy left under
`core/text/` during the migration.

---

## 2. Format enum collapse

Three enums named one concept: `document_format` (7 values),
`doc_format` (5, test_output.hpp), `test_doc_type` (4, test_options.hpp).
`test_output_config.hpp` existed partly to switch between them, and warned in its
own banner that a `static_cast` would mis-map — a bug class manufactured by the
duplication.

**`document_format` is now the single selector.** Both others are aliases:

```cpp
using test_doc_type = ::djinterp::document_format;   // test_options.hpp
using doc_format    = ::djinterp::document_format;   // test_output.hpp
```

`document_format.hpp` was **split** so this is affordable:

| header | floor | contents | deps |
|---|---|---|---|
| `document_format.hpp` | C++11 | the enum, `format_name`, `format_extension`, `format_from_name`, `format_is_markup`, `format_is_binary` | `djinterp.hpp` only |
| `document_format_policy.hpp` *(new)* | C++17 | `make_print_policy`, `make_boxed_document` | `document.hpp` |

The split is the point: `test_options.hpp` must compile at the C++11 floor and
must not drag a serialiser (or the `djinterp::document<>` class template, which
would shadow-clash with its own `document(opts)` accessor) into every TU that
merely configures a format. Nothing was including `document_format.hpp` before,
so moving the factory out breaks no caller.

### Call-site changes

- **One spelling changed: `test_doc_type::txt` → `::text`.** 2 sites in
  `test_options.hpp`, 1 in `test_pack.hpp`, 1 in `test_output_config.hpp`, 2
  comments. `xml` / `html` / `pdf` are unchanged, so a runner that only ever
  selected PDF is untouched. `"txt"` still parses via `format_from_name`.
- `format_extension(doc_format)` deleted from `test_output.hpp`; the canonical
  one is found by ordinary lookup into `djinterp`, where it forms an overload set
  with output_packaging's `format_extension(format_id)`. Distinct scoped enums,
  so no ambiguity.
- `pack_doc_ext_helper` (test_pack.hpp) now delegates to `format_extension` and
  strips the dot, instead of carrying its own switch.
- `to_doc_format` (test_output_config.hpp) is now an identity, kept so call sites
  need no edit. Candidate for removal.
- **`markdown` / `tex` / `wiki` became selectable for free.** The emit layer
  already rendered markdown; `test_doc_type` simply had no member to ask for it.
- The two `render_*_bytes` switches in `test_output.hpp` already fuse
  `case doc_format::text:` with `default:`, so the wider enum needs no edit there.

The **packaging** bridges (`to_pack_mode` / `to_codec_id` / `to_format_id`) are
untouched. Those really are two independently-ordered enum sets, and the
`tar_gz` / `gz` hazard the header documents is exactly why they're switches.
Collapsing them needs `output_packaging.hpp`, which wasn't in the file set.

---

## 3. New renderers

`document_renderer` had one realisation for text and one for PDF. The string
formats DTest emits (markdown, xml, html) had no renderer, which is why
`test_render.hpp` carries `escaping_sink<Policy>` and a per-format
`report_layout` instead. Three new headers close that:

### `templates/markup_document_renderer.hpp`

`markup_document_renderer<_Policy>` + `xml_document_renderer` /
`html_document_renderer`. Reuses `markup_string_template`'s escape policies
whole — no new escaping.

- **Tags raw, values escaped.** Element and attribute *names* come from the
  profile; every string from a template (text, cells, attribute *values*) goes
  through `_Policy::escape`. Same contract `escaping_sink` held, lifted into the
  renderer so a caller can't get it wrong by pairing a markup layout with a plain
  sink.
- **One class, two dialects.** The difference is a `markup_profile` — element
  vocabulary plus how presentation is spelled. `markup_profile_for<_Policy>`
  picks the default; pass a profile to retarget at DocBook/TEI/a house schema
  without touching the file.
- **Streams.** `begin_table` emits `<table>` at once, the first `table_column`
  opens `<thead>`, the first `begin_row` closes it. No row storage, unlike the
  plain renderer which must buffer to compute widths.
- Hints: column `align` merges into that column's cells; under `use_css_style`
  (HTML) `style`→`class`, `locator`→`id`, `colspan`→`colspan`, and
  align/color/background/font/size/bold/italic/width collapse into one inline
  `style`; otherwise (XML) each hint is an attribute of the same name.

### `templates/markdown_document_renderer.hpp`

GFM: ATX headings, `-`/`N.` lists at nesting indent, `---` rules, and pipe tables
whose **delimiter row carries each column's alignment** — the one hint Markdown
can spell structurally. `bold`/`italic` become emphasis runs; the rest drops.

Escaping is positional, not total: escape what changes meaning *mid-line*
(`\` `` ` `` `*` `_` `[` `]` `<`) plus `|` and newline→`<br>` inside cells. The
line-start-only characters (`-` `#` `.` `+`) are left alone, because every string
handed to the renderer is placed after a block marker. Escaping the full
CommonMark set turns prose into `a well\-formed sentence\.`.

### `templates/document_dialect.hpp`

`make_document_dialect(document_format)` → a `document_dialect` owning the
renderer plus the `str()` / `clear()` accessors bound while the concrete type is
still known (`document_renderer` deliberately has no `str()` — a PDF back end has
no string to give).

`render_to_string(format, template, frame)` renders any template
(`render(document_renderer&) const`) in any served dialect, wrapped in
`begin_document`/`end_document` so the result stands alone — the generalisation
of each template's `to_string()`, which is hard-wired to plain.

`pdf` returns an **invalid** dialect on purpose: `pdf_document_renderer` pulls in
the whole `pdf.hpp` tree, and this header is included by text-only builds. Check
`format_is_binary(fmt)` and construct it yourself — the same opt-in split
`document_renderer.hpp` / `pdf_document_renderer.hpp` already keep. `tex` and
`wiki` are unserved until someone writes them.

---

## 4. Verification

The new headers were compiled and run against **stub** implementations of the
five headers not in the file set (`djinterp.hpp` macros, `container_metadata`,
`text_align`, and the two escape policies) with `g++ -std=c++17 -Wall -Wextra
-Wpedantic` — clean, no warnings. A driver rendered a real `document_table` and
a block sampler (headings, emphasis, key-values, rules, vertical space, nested
lists, page breaks) through all four dialects.

Programmatic checks on the output: XML parses under `xml.dom.minidom`; HTML tag
stack balances; the nested `<ol>` sits inside its parent `<li>`.

Two bugs were found and fixed this way, both HTML-only:

1. `vertical_space` and `page_break` emitted `<div ...>` with no closing tag —
   an unclosed `<div>` swallows the rest of the document. `void_element` now
   distinguishes three cases: XML self-closes everything, HTML leaves genuine
   void elements (`<hr>`) open, and everything else gets `<div></div>`.
2. A nested list was emitted as a **sibling** of its parent item
   (`<ul><li>a</li><ol>…</ol></ul>`), which is invalid. `list_item` now leaves the
   item open and `close_open_item` folds the two cases together — an unnested
   item still closes on one line, a nested one closes after its child list.

**This is not a compile against the real tree.** Anything touching
`container_metadata`'s exact surface, the real `D_` macros, or `document_table` /
`title_page` internals still needs a real build. One deliberate concession to
that: `plain_attribute_text` spells the standard `doc_attr_*` vocabulary **by
name** rather than walking the bag, so this header depends only on
`find` / `contains` and not on `container_metadata`'s iterator value shape. A
dialect-specific key is emitted by listing it in
`markup_profile::extra_attribute_keys`.

---

## 5. Open items

1. **`parse.hpp` location.** `layout_parse.hpp` keeps `../../../parse/parse.hpp`
   → `inc/djinterp/parse/parse.hpp`, matching its banner ("beside core rather
   than under it"). If it's at `core/parse/`, that's a one-line change.
2. **`templates/` subdirectory** kept, per the four template banners; the
   `report_*` / `layout_*` headers assumed flat siblings, which is what the six
   `./templates/…` rewrites fix.
3. **Not done, from the earlier analysis:** the `pdf_canvas` / `pdf_template`
   merge (two flow engines, the metric-accurate one with the real paginating
   table is still orphaned); folding `test_render_pdf_table.hpp`'s ruled/shaded
   painter into the PDF layer; retiring `report_layout` / `pdf_layout` in favour
   of `layout_doc` + one walk; consolidating the three packaging paths.
   The renderers above are the prerequisite for the second and third of those.
4. `test_render.hpp`'s `escaping_sink` and the xml/html `report_layout` literals
   are now redundant with `xml_document_renderer` / `html_document_renderer`, but
   are **left in place** — retiring them is the single-walk change, not this one.

---

## 6. Table bridge (`container/table` ↔ `util/document`)

Both halves already had a table, answering different questions:

| | model | dialects |
|---|---|---|
| `container/table` | merge cover (regions, anchors, spans), multi-level **grouped** headers on **both** dimensions, round-trips with `table_parser` | one — monospace, via `table_model::render` |
| `util/document` | `document_table`: flat column list, string rows, no merges, no row headers | five — plain, Markdown, XML, HTML, PDF |

`table_model_value` is a strict superset of `document_table`'s model, and
`table_model::render` is the **fifth** monospace table engine in the codebase
(after `plain_document_renderer`'s, `pdf_document_renderer` forwarding to it,
`pdf_canvas::table`, and `test_render_pdf_table`'s painter).

### `templates/table_model_render.hpp` *(new)*

`render_table_model(model, renderer, opts)` streams a table model through any
`document_renderer`.

**Duck-typed on the model.** `_Model` is a template parameter and the header
includes nothing from `container/table` — so `util/document` takes no dependency
on the table container family (C++17, pulls the option pack), and a text-only
build pays for neither. A conforming model needs `rows()`/`cols()`,
`raw_at(r,c)`, `layout().owner_of(r,c)` (with `anchor_row`/`anchor_col`/`rows`/
`cols`), and `metadata()` with the header accessors. Same decoupling
`table_metadata::conforms_to` already uses.

**Spans are a dialect question, so they're a policy.** `span_policy::hint`
emits a layout cell once at its anchor with `colspan`/`rowspan` and skips
covered positions — correct for HTML. `span_policy::fill` emits one cell per
atomic position, blank where covered — correct for monospace and Markdown,
where the grid needs its column count. `table_render_defaults(format)` picks.

**Grouped headers** meet a protocol with one header row. Two honest answers:
`join` folds a column's levels into its header text (`"coordinate / x"`, the
default, works everywhere) or `rows` emits coarser levels as leading body rows
carrying colspan. A `header_row` verb on `document_renderer` would beat both;
that's a protocol change and isn't made here.

### `templates/table_model_dialect.hpp` *(new)*

`render_table_model_to_string(model, format)` — the one-shot. Separate because
it needs `document_dialect.hpp` (every renderer) and the bridge needs none.

### Supporting changes

- `doc_attr_rowspan` added to the hint vocabulary (`doc_attr_colspan` already
  existed); the markup renderer emits both as real HTML attributes and as XML
  attributes.

### Verification

Ran against the **real** `runtime_layout` and `table_metadata<>` (both compile
clean against the stubs), with a 2×2 merge and two-level grouped column headers
plus row headers. `layout().valid()` and `metadata().conforms_to(model)` both
pass. HTML output was parsed back and the span grid checked programmatically:
4 rows, every row 5 columns wide after expanding colspan/rowspan, no overlapping
occupancy. The merge renders as `colspan="2" rowspan="2"` in HTML and as a
visually-merged block in monospace.

---

## 7. Bug found: `container_metadata.hpp` path

`document_attributes.hpp` included `container/container_metadata.hpp`. It is at
`container/metadata/container_metadata.hpp` — `table_metadata.hpp` (banner path
`core/container/metadata/table_metadata.hpp`) includes it as
`./container_metadata.hpp`, which is authoritative.

This was broken *before* the relocation too: the original include was
`"../../container/container_metadata.hpp"` from a file declaring itself at
`core/text/document/templates/`, which resolves to
`core/text/container/container_metadata.hpp` — nonsense. The relocation
preserved the breakage; having `table_metadata.hpp` in hand exposed it.

Fixed to `../../../container/metadata/container_metadata.hpp`.

Unrelated but adjacent, and **not** fixed since those files are out of scope:
`core/util/metadata/metadata_concepts.hpp` and `metadata_traits.hpp` both include
`"../../core/meta/type_traits.hpp"`, which from `core/util/metadata/` resolves to
`core/core/meta/type_traits.hpp` — the `core/` is doubled.

---

## 8. Naming papercut

`core/container/table/table_options.hpp` does **not** define a type called
`table_options` (it defines `table_opt_key` and the option aliases).
`core/util/pdf/pdf_canvas.hpp` **does** define `djinterp::table_options`, a
struct of column widths, padding, grid colour. No ODR collision today, but the
header name and the type name point at different things in the same namespace.
Worth renaming one of them before they meet.

---

## 9. Q2-2: one walk (`test_layout.hpp` + `test_report_content.hpp`)

`report_layout` (test_render.hpp) and `pdf_layout` (test_render_pdf.hpp) are both
seven-slot structs — `run_open / module_open / unit_open / check_line /
unit_close / module_close / run_close` — i.e. a **hardcoded four-level document
with the construct set frozen into struct fields**, walked by two structurally
identical hand-written traversals. `test_render_pdf.hpp`'s own banner called them
"a candidate to factor into one visitor-driven walk."

Both are now one fold to a term plus the interpreter's single fold over it.

### DTest defines no new constructs

That is the load-bearing decision. A test report *is* a cover, a table of
contents, and nested numbered sections holding content — which is exactly the set
`report_dialect.hpp` already names (`body` / `cover` / `clearpage` / `toc` /
`section`, with content as a `body_ref` leaf). So `test_layout.hpp` registers
nothing and lowers nothing; it builds a `report_op` term with the dialect's own
combinators and hands it to the dialect's own signature.

```
test_report   -> body{ cover, clearpage, toc, clearpage, sections... }
report_module -> section(module name) { content(module summary), units }
report_unit   -> section(unit name)   { content(assertion table) }
report_check  -> a ROW of that table
```

Numbering and the TOC fall out of the interpreter's passes — modules number 1, 2,
3 and their units 1.1, 1.2. The flat `report_layout` could not express that at
all: one literal per *level*, no notion of depth.

### The split, and why

| header | contents | verifiable here? |
|---|---|---|
| `test_report_content.hpp` | the content resolver's body: assertion tables, module and run summaries, status/style vocabulary, the key scheme | **yes** — plain `document_renderer` calls over the report model, no expression/cofree/interpreter |
| `test_layout.hpp` | the fold to `layout_doc<report_op>`, the metadata bag, the resolver, `render_test_report` | **no** — needs `expression.hpp`, `cofree.hpp`, `maybe.hpp`, `operator_signature`, none of which are in the file set |

The line is drawn so the part a maintainer actually edits (all the DTest-specific
detail) is the part that is cheap to verify.

### Verification

`test_report_content.hpp` compiled and run against a stub report model under
`-Wall -Wextra -Wpedantic`, emitting a run summary, a module summary and a
three-row assertion table (pass / fail / skipped-with-no-expression) into plain,
Markdown and HTML from **one** emission. Per-dialect escaping confirmed correct:
`<core> &` renders raw in text, backslash-escaped in Markdown,
`&lt;core&gt; &amp;` in HTML. Status style names land as HTML classes. The
unbound-key predicate returns false for a key the report does not bind.

`test_layout.hpp` is **unverified**. It uses only the published surface of
`layout.hpp` / `layout_interpret.hpp` / `report_dialect.hpp` — all of which are in
hand — and follows `report_dialect.hpp`'s `demo_render` pattern for the term
construction and the resolver. But the layer beneath (`expression`, `cofree`,
`annotated_cata`, `maybe`, `operator_signature`) was never available, so nothing
below `render_document` has been exercised. Expect it to need a real build.

### Not yet cut over

`test_render.hpp` and `test_render_pdf.hpp` are **kept and annotated** as
superseded, not deleted: `test_output.hpp` still calls `render_report_bytes` /
`render_report_pdf_bytes`, and rewiring it is the next step. Once it moves,
`report_layout`, `pdf_layout`, `escaping_sink`, the four `layout_*()` literal
tables, both traversals, and `test_render_pdf.hpp`'s `brace_escape` all go.

### Known nit

Status style names are dotted (`"verdict.pass"`), which is shared vocabulary with
`title_page.hpp` and the PDF `register_style` registry. As an HTML `class` the
value is legal but needs escaping to use as a CSS *selector*. Renaming would
break the PDF side; flagged rather than changed.

---

## 10. Q1-A: the PDF layer, measured

The whole PDF stack turned out to be compilable here — `pdf_primitives` through
`pdf_canvas`, `pdf_template` and `pdf_document_renderer` — needing only stubs for
`color.hpp`, `color_rgb.hpp` and `meta/type_traits.hpp` (the real `font.hpp` and
`env_pdf.h` were in the file set). So this round is verified rather than argued.

### `pdf_canvas` works, and was orphaned

Driving it directly produced a valid 2.9 KB PDF — `%PDF-1.4`, 8 objects, font
resources, xref, `%%EOF` — including a gridded table. Nothing in the codebase
included it.

### `pdf/pdf_canvas_document_renderer.hpp` *(new)*

A **second** `document_renderer` on the PDF side, backed by `pdf_canvas` instead
of `pdf_template`. Neither engine is a superset of the other:

| | `pdf_template` | `pdf_canvas` |
|---|---|---|
| measurement | fixed character cells | `pdf_metrics` — true Core-14 advances |
| tables | none (grid primitive absent) | real, gridded, paginating |
| tokens | `text_template` substitution | none |
| styles | named registry | none (registry added by this renderer) |

`pdf_document_renderer` drives the first, so a template's table falls back to a
Courier text block. This one drives the second, so the same template's table
becomes an actual ruled grid.

Tables **buffer** here, deliberately: `pdf_canvas::table` resolves column widths
across the whole grid and paginates over it, so it takes every row at once while
the renderer protocol streams. That's the one piece of state a streaming dialect
wouldn't hold — the price of a real grid.

### Head-to-head measurement

Same emitters (`emit_run_summary`, `resolve_report_content`), same report, two
renderers, PDF content streams inspected:

| | rectangles | strokes | text shows | fonts |
|---|---|---|---|---|
| canvas-backed | **21** | **20** | 25 | Helvetica, Helvetica-Bold, Courier |
| template-backed | 0 | 0 | 10 | Helvetica-Bold, Courier |

Zero rectangles and zero strokes is the monospace-block fallback, exactly as the
earlier analysis claimed. This is the concrete form of the Q1-A argument.

### A bridge, not the merge

The two engines still duplicate a cursor, page advancement, pagination and
alignment. That duplication is real and worth removing — but removing it changes
the PDF layer, whereas putting a second renderer on the better-measured engine
does not. After this header both engines have exactly one consumer apiece and the
same interface in front of them, which is the position from which the merge is a
refactor rather than a rewrite. Not done; deliberately.

### Also still open

`test_render_pdf_table.hpp`'s ruled-and-shaded painter remains a richer PDF-only
face than either renderer: it carries per-cell fills, pass/fail chips, zebra
striping and fail tints, which `pdf_canvas`'s single-style grid cannot express.
Folding its capability into `pdf_canvas::table` (per-cell fill and stroke) would
let it retire and would give every dialect the good table. That is the next PDF
piece.

---

## 11. Per-cell table styling in `pdf_canvas`

The capability `test_render_pdf_table.hpp` was built around — per-cell fills and
colours, which "the foundation's single-fill table() helper cannot express" (its
own banner) — is now in the foundation.

### `pdf_canvas.hpp` — additive change

New `cell_format` (opt-in `has_*` flags for fill, text colour, alignment, font)
and a `cell_format_fn cells` callback on `table_options`:

```cpp
using cell_format_fn = std::function<cell_format(std::size_t row,
                                                 std::size_t col)>;
```

A callback rather than a parallel grid, so a zebra rule stays `(row % 2) == 0`
and nothing has to be materialised. The row index is the caller's own: the
header is row 0, data rows keep their index in the supplied grid.

`draw_table_row` now paints an optional per-cell fill *under* the rule, refines
the row style by whichever overrides are present, and places text by measured
width (`text_width`) so per-cell centre/right alignment is exact rather than
left-only.

**Verified additive**: the pre-existing driver produces a byte-identical 2903-byte
PDF before and after. A null `cells` is the old path exactly.

### `pdf_canvas_document_renderer` — wired through

Per-cell hint bags now buffer alongside the cell text and become a `cell_format`
at `end_table`. Resolution order, weakest first: zebra stripe → registered
`style` (colour / font / alignment) → `background` → `color` → `align`. Plus
`set_zebra(fill)` / `clear_zebra()`.

### Measured

`emit_unit_table` unchanged, five checks across four statuses, zebra on:

```
filled rects: 11    stroked rects: 30
distinct colours: 7
  0 0 0                          text
  0 0.5 0.15                     verdict.pass
  0.45 0.45 0.45                 verdict.skip
  0.6 0.3 0                      verdict.error
  0.8 0.05 0.05                  verdict.fail
  0.921569 …                     header fill (235)
  0.956863 0.960784 0.972549     zebra stripe
```

11 fills = 1 header + 10 striped cells; 30 strokes = 6 rows × 5 columns. The
four verdict colours come from `emit_unit_table`'s existing `doc_attr_style`
hints — no DTest change was needed.

### What this does and does not retire

Of `test_render_pdf_table.hpp`'s 19 functions, the table half — `fill`,
`stroke`, `glyphs`, `cell_text`, `header_row`, `test_table`, `room`, `new_page`,
`measure_h`, `test_level_cols` — is now covered by `pdf_canvas::table` plus the
renderer, including the pagination-with-header-redraw that `pdf_canvas` already
had.

What remains is **page design**, not table capability: `band`, `meta_block`,
`meta_row`, `section_label`, `title_block` — the slate palette, dark suite
bands, metadata blocks and per-test cards. That belongs in a document template
alongside `title_page.hpp`, not in a painter. One cosmetic gap: its pass/fail
**chips** are inset filled rects behind the text, where a `cell_format` fill
covers the whole cell.

So the file cannot be deleted yet, but it is no longer carrying a capability the
foundation lacks — which was the actual argument for its existence.

### Regression

All nine drivers accumulated across these rounds rebuild and run clean under
`-Wall -Wextra`; the renderer compiles warning-free under `-Wpedantic` as well.

---

## 12. The page design, as a template

`test_render_pdf_table.hpp` held two different things. The table engine moved to
`pdf_canvas` (§11). This closes the other half.

### `pdf_canvas::band()` *(new primitive)*

A full-content-width filled strip with text flowed inside it — the
flow-relative counterpart of `text_box`, which places at an absolute rect and
only strokes. The fill must be painted *before* the text, so the height is
measured first (wrap to the inner width, count lines) rather than discovered by
flowing: that is precisely why this cannot just be `paragraph()` with a colour.
Paginates as a unit, so a band never splits across pages.

Byte-identical regression again: the pre-existing driver still emits 2903 bytes.

`pdf_canvas_document_renderer` now funnels every block override through a
`flow_block` helper — a `background` hint becomes a band, everything else an
ordinary paragraph.

### `test/output/test_report_document.hpp` *(new)*

The page design as a **document template**: title block, run summary, a banded
section per module, a card per unit, each card carrying its assertion table.

The painter could only ever emit PDF, because it spoke in fills and glyph
placements. The same design written as `document_renderer` calls renders in
every dialect. A band is a `heading` carrying a `background` hint; a card is a
heading plus a table. The palette is **data** (`report_palette`, "#RRGGBB"
tokens, defaulting to the painter's slate), so restyling never touches the
composition and a colourless dialect simply never reads it.

### Measured — one composition, three media

Two modules × two units × three checks, `page_break_per_module` on:

| | result |
|---|---|
| PDF | 2 pages, 30 filled rects, 80 stroked rects, **2** slate suite-band fills |
| HTML | 4 `<table>`, 2 banded `<h2>` (#2F3A4A), 4 banded `<h3>` (#EEF1F5), 1 page-break div |
| text | setext underlines, rules, aligned figure block |

The page break landed once (between the two modules, never before the first),
the bands landed once per module, and the tables came through as real `<table>`
in HTML and as ruled grids in PDF — from a single call sequence.

### `test_render_pdf_table.hpp` can now go

Its table capability is in `pdf_canvas`; its composition is in
`test_report_document.hpp`. Two caveats before deleting:

1. **Chips are cosmetically different.** The painter drew an inset filled rect
   behind the status text; a `cell_format` fill covers the whole cell. If the
   inset look matters, `cell_format` needs an inset field.
2. **Geometry will not be pixel-identical.** The painter hard-coded the design
   reference's margins and card metrics; the template delegates geometry to the
   renderer. Same document, not the same millimetres.

Neither is a capability gap. `test_render_pdf.hpp` and `test_render.hpp` remain
the real blocker on deletion, since `test_output.hpp` still calls into them.

### Regression

All ten drivers rebuild and run clean under `-Wall -Wextra`; the renderer and
canvas compile warning-free under `-Wpedantic`.

---

## 13. `cofree_recursive` merged — and the layout stack now verified

### Mechanical

`core/functional/cofree_recursive.hpp` **deleted** (its sections V and VI are now
cofree.hpp's). `layout.hpp`'s include rerouted to `../../functional/cofree.hpp`;
three prose references updated. Nothing else referenced it.

### The chain that unblocked

Having the real `cofree.hpp` made the whole layout stack compilable. Working
outward, stubbing only what was genuinely absent:

1. `functor.hpp` / `comonad.hpp` / `recursion.hpp` — the three typeclass
   registries cofree.hpp names. Their contracts are fully determined by how
   cofree.hpp uses them, so the stubs are inferred, not invented.
2. **The real `cofree.hpp` compiles, and runs its own documented example to the
   documented values**: `extract` → 3, `functor_map` → 6, `annotated_cata` sum
   → 6, depth → 3. That validates `cata` / `project` / `env_layer` semantically,
   not just syntactically.
3. `expression.hpp` / `expression_ops.hpp`, `maybe` / `semigroup` / `monoid` —
   inferred the same way, from their consumers.
4. **The entire real layout stack then compiled**: `layout.hpp`,
   `layout_build.hpp`, `layout_spec.hpp`, `layout_interpret.hpp`,
   `report_dialect.hpp`.
5. `report_dialect.hpp`'s own `demo_render()` **runs**, producing a cover, a
   form-feed page break, a numbered table of contents (1, 1.1, 2), numbered
   sections and a rendered table — i.e. `render_document`'s four passes
   (number → outline → emit → run) all work.

### `test_layout.hpp` is now verified

The one piece nothing had compiled **compiles and runs correctly, first try**:

```
Nightly / build 4412 / Author: teer      <- cover
[page break]
Contents: 1 Summary, 2 meta, 2.1 trait_detect
1  Summary        (=== underline)
2  meta           (=== underline)
2.1  trait_detect (--- underline)
RESULT | # | ASSERTION     | EXPECTED | ACTUAL
```

The numbering and the table of contents are the point: `2.1` is a depth the flat
seven-slot `report_layout` could not express at all. Across dialects, from the
same fold: HTML (4 `<table>`, 2 page-break divs, TOC entries), Markdown, and PDF
(3 pages, 60 stroked cells) via the canvas renderer.

**Q2-2 is now verified rather than argued.**

### A real constraint discovered

`render_document`, `renumber`, `outline` and `emit` all take
`const layout_doc<_OpId, _Atom>&` with **no default on `_Atom`**. So `_Atom` must
be deducible from that parameter — which means `annotated_expression` cannot be a
plain alias for `cofree<binder::template layer, Ann>`: an alias through a member
template is a **non-deduced context**, and every generic entry point in
`layout_interpret.hpp` would fail to match. `report_dialect.hpp`'s own demo is the
first thing that breaks.

Two further constraints fall out of the same exercise:

- The layer's child hole must be typed as the **node**
  (`shared_ptr<layout_doc<Op,Atom>>`), not as the raw
  `shared_ptr<cofree<F,Ann>>` — `layout.hpp`'s `apply_node` boxes the former and
  `layout_interpret.hpp`'s `renumber_children` binds a reference to it.
- `operator_signature::describe(op)` returns a **raw pointer**
  (`const layout_descriptor*`, null when unregistered), not a `maybe`.

If the real `expression.hpp` already satisfies these, nothing to do. If it
defines `annotated_expression` as a plain alias, the layout stack does not
compile today and this is why.

### Caveat, stated plainly

The layout stack was compiled against **inferred** `expression.hpp`,
`functor.hpp`, `comonad.hpp`, `recursion.hpp`, `maybe/semigroup/monoid`. What is
verified is that the real headers are internally consistent and that
`test_layout.hpp` uses their published API correctly. What is *not* verified is
that my inferred interfaces match the real ones — where they differ, the real
build will say so. The constraints above are the places that difference is most
likely to bite.

### Regression

All **14** drivers accumulated across these rounds build and run clean.

---

## 14. Demos out of library headers; the surfaces claim verified

With the layout stack compilable (§13), two things that had only ever been
*argued* became testable — and one Q3 item became doable.

### The four-surfaces claim holds

`report_surfaces.hpp` asserts that four declaration front ends — combinator,
cursor, aggregate spec, and block-DSL parser — build the *same* term. Stubbing
`parse.hpp` (contract fully determined by `layout_parse.hpp`'s use:
`parse_state<char>` with `advance` / `at_end` / `current` / `offset`, and
`parse_result<T>` with `ok` / `value` / `error` / `make_error`) let the parser
surface compile, and the check **passes**: all four render identically.

### `test/output/test_layout_suite.hpp` *(new)* — the demos, now run

`report_dialect.hpp`'s DEMO section (86 lines) and `report_surfaces.hpp`'s
EQUIVALENCE DEMO (160 lines) moved into a DTest module.

The cut is where it should be: **sections I–III of `report_surfaces.hpp` stayed**
— the surfaces themselves are library — and only the assertions *about* them
moved. A library header that ships its own assertions cannot be exercised by the
harness: the code was compiled and never run, so an interpreter regression would
have shipped silently.

The module runs 8 checks through `report_builder` / `D_CHECK_EQ`:

```
module layout
  unit surfaces  -- the four declaration front ends build one term
    [PASS] surfaces_equivalent()
  unit interpreter  -- number -> outline -> emit -> run, end to end
    [PASS] contains_helper(_rendered, "Contents")
    [PASS] contains_helper(_rendered, "1  Introduction")
    [PASS] contains_helper(_rendered, "1.1  Motivation")
    [PASS] contains_helper(_rendered, "2  Results")
    [PASS] contains_helper(_rendered, "nightly build")
    [PASS] contains_helper(_rendered, "Assertions")
    [PASS] count_helper(_rendered, '\f')

8 passed, 0 failed
```

`1.1  Motivation` is the check worth having: that depth is exactly what the flat
per-level layout literals could not express.

`report_dialect.hpp` 467 → 390 lines; `report_surfaces.hpp` 483 → 336. Both
still compile; each carries a pointer to the module's new home.

### Guard doubling fixed

`DJINTERP_TEST_TEST_*` → `DJINTERP_TEST_*` across `test_output.hpp`,
`test_output_config.hpp`, `test_render.hpp`, `test_render_pdf.hpp`. All **47**
guards in the deliverable are now distinct.

### `functional.md` updated

Notes that `cofree_recursive.hpp` merged into `cofree.hpp`, and documents what
that brought with it: `env_layer<B, F, X>`, the
`recursive_traits<cofree<F, B>>` registration, and `annotated_cata<R>` — the
binary-algebra fold that is the engine under the layout interpreter.

### Regression

14 drivers pass. (`drive12` / `drive15`, which called `demo_render()` and
`report_surfaces_equivalent()` directly, are retired — `drive16` runs both
through the module.)

---

## 15. Deletions, and a runnable harness

### `test_render_pdf_table.hpp` deleted (882 lines)

`test_report_runner.hpp` — the **live** path — called exactly one thing from it,
`render_report_pdf_bytes_table`, at two sites. That is now provided by
`test/output/test_report_pdf.hpp` (new): the pairing of `test_report_document.hpp`
with the canvas-backed renderer. Signature preserved, so both call sites are
unchanged; the runner needed only an include swap.

Its `test_render_pdf.hpp` include was also **vestigial** — the runner referenced
nothing from it — and is gone. `test_render_pdf.hpp` and `test_render.hpp` are
now reachable only from `test_output.hpp`.

### Not deleted, and why

`test_render.hpp` and `test_render_pdf.hpp` still hold `report_layout`,
`pdf_layout`, `escaping_sink`, the four `layout_*()` literal tables, both
traversals and `brace_escape` — all superseded. They survive because
`test_output.hpp` still calls `render_report_bytes` / `render_report_pdf_bytes`
against them, and rewiring that is the cutover of the **dormant** stack
(`test_emit` → `test_output_config` → `test_output`, reachable from nothing).
Whether that stack is the intended future or should be deleted outright is a
decision, not a refactor.

### `verify/` — the harness

Every claim in this document was verified by compiling and running the real
headers against stubs of the ones never in the file set. That verification now
ships:

```
verify/verify.sh      build + run everything   (BUILD=... to relocate artefacts)
verify/verify.sh -q   results only
verify/verify.sh syntax   header syntax pass only
verify/stubs/         the inferred headers, at their canonical paths
verify/drivers/       13 behavioural drivers
verify/STUBS.md       what each stub was inferred to satisfy
```

Current state: **25 header syntax checks + 13 drivers, 38 ok, 0 failed**
(`drive4` skipped — it needs the `container/table` headers).

The harness assembles a tree (shipped headers, stubs overlaid) because the real
headers reach siblings *relatively*, so an `-I` path cannot substitute for a
missing neighbour. It builds outside the source tree by default, since a
checkout may sit on a noexec mount.

**The point is the swap.** Drop a real header into `verify/stubs/` at the same
path and re-run; what breaks is where my inference of that interface was wrong.
`STUBS.md` leads with the three constraints most likely to bite — chiefly that
`annotated_expression` cannot be a plain alias through a member template, or
every generic entry point in `layout_interpret.hpp` becomes uncallable.

---

## 16. The real headers, swapped in — one confirmed defect

With the real `expression.hpp`, `functor.hpp`, `recursion.hpp`, `comonad.hpp`,
`semigroup.hpp`, `monoid.hpp`, `maybe.hpp` and `parse.hpp` in hand, the swap the
harness was built for was run. Result: **two of the three constraints in
STUBS.md are real defects, and they are coupled.**

### Constraint 1 — confirmed. `annotated_expression` is a non-deduced context

`expression.hpp` defines it as a plain alias:

```cpp
template<typename _OpId, typename _Atom, typename _Annotation>
using annotated_expression =
    cofree<internal::expr_carrier<_OpId, _Atom>::template layer, _Annotation>;
```

An alias through a member template is a non-deduced context
([temp.deduct.type]/5). Ten-line reproduction:

```cpp
template<typename _OpId, typename _Atom>
void takes_annotated(const annotated_expression<_OpId, _Atom, std::string>&) {}

annotated_expression<op, int, std::string> node;
takes_annotated(node);      // error: template argument deduction failed
```

**Every generic entry point over an annotated term is uncallable** —
`render_document`, `renumber`, `outline`, `emit`. The headers compile (templates
are not instantiated); the *calls* do not. That is why a header syntax pass was
never going to catch it, and why `test_layout.hpp` failed the moment it was
built against the real tree.

### Constraint 2 — confirmed, and only visible once 1 is fixed

Deriving `annotated_expression` from the cofree specialisation restores
deduction, and immediately exposes the second: `expr_carrier<Op,Atom>::layer<X>`
types the child hole as `shared_ptr<cofree<F,Ann>>`, while `layout.hpp`'s
`apply_node` boxes `shared_ptr<annotated_expression<...>>` and
`layout_interpret.hpp`'s `renumber_children` binds a reference to it.

### Constraint 3 — not a defect

`describe` already returns a raw pointer. The stub matched.

### The fix

`expression_deduction_fix.patch` (100 lines, against your `expression.hpp`) does
both halves: makes `annotated_expression` a class deriving from the cofree
specialisation, and adds an `expr_child` retype so the carrier's child hole names
the node. With it applied, the layout stack builds past both defects; the only
remaining failure in the sandbox is a deficiency of the *stub* `foldable.hpp`
(`foldable_value_type_t` returning `void`, which `monoid.hpp`'s `mconcat` needs),
not of your tree.

**This is the single most valuable thing the harness produced**, and it needed
instantiation to find — no amount of header-level compilation would have.

## 17. Deletions (dormant stack)

Per your call, the dormant emit stack is gone:

- `test_output.hpp` — the emit layer, reachable only via `test_output_config`
- `test_output_config.hpp` — the enum bridge, whose document-format arm had
  already collapsed to an identity
- `test_render.hpp` — `report_layout`, `escaping_sink`, the four `layout_*()`
  literal tables, `render_report`
- `test_render_pdf.hpp` — `pdf_layout`, the second traversal, `brace_escape`

`test_emit.hpp` is not in this tree; delete it in yours. Every remaining mention
of these four is prose in a banner, not an `#include`.

**That closes Q2-3 and Q2-7** — the three ad-hoc sinks and the duplicated
brace-escaping went with them. `drive8` (the canvas-vs-template head-to-head)
is retired with them.

Deleted across this migration: `test_report_runner_table.hpp`,
`test_render_pdf_table.hpp`, `cofree_recursive.hpp`, and the four above.

---

## 18. `document_template.hpp` — templating, generalised out of PDF

`pdf_template` conflated four jobs: flow layout, token substitution, a named
style registry, and an element list. Layout now belongs to `pdf_canvas`, which
measures with real font metrics and owns the cursor — and once layout leaves,
**nothing remaining is PDF-specific**. Substitution, styles-by-name and an
element list are document concerns.

So the rewrite is not a better PDF templating module. It is a *document*
templating module that renders to PDF among others, and the PDF layer is left
with exactly one layout engine instead of two.

### What a template is

An element sequence. Every text field is a `text_template`, so `{title}` and
`{modules.name}` interpolate at render time. An element carries a **style name**,
emitted as a `style` hint — a dialect with a registry (PDF's `canvas_style`,
HTML's `class`) resolves it, one without drops it. That is the whole of how one
skeleton serves five dialects.

### What makes it a template rather than a form

`tmpl_repeat` binds a sequence by name and emits its children once per item,
each item's fields scoped under the loop variable (`{modules.name}`). Nested
repeats nest their scopes. Without repetition a "template" is only a document
with holes — a table of N rows, a section per module and a card per test all
become one declaration rather than a hand-written loop.

`tmpl_table` is the same mechanism specialised: columns declared once serve both
the header and every row's cell.

`tmpl_slot` is the escape hatch — a named hole a caller fills with an emitter,
for content a declarative form cannot express (a chart, a bespoke table). It
keeps the template declarative without growing a case per irregular block.

### What it deliberately does not do

No cursor, no pagination, no measurement — it emits semantic calls. And it names
styles rather than owning a registry; the renderer binds them. Both were
pdf_template's, and both were exactly why it could serve one back end.

### Verified

One skeleton — heading, key-value, rule, a repeat over modules each containing a
table over checks — rendered to plain text, HTML and PDF from a single
declaration. Tokens bound, nested scopes resolved, per-column alignment carried
through. `drive17`.

---

## 19. Packaging consolidated

Three paths did render -> name -> compress/archive -> sink. `test/output/
test_packaging.hpp` (new) re-homes the live one onto the designed one.

### The bridge stays a switch, permanently

`output_packaging.hpp` says it outright and it is worth restating: DTest's
`test_archive_format` orders `{ zip, tar, tar_gz, gz, ... }` and the facade's
`format_id` orders `{ zip, tar, gz, tar_gz, ... }` — **`tar_gz` and `gz` are
transposed**. A `static_cast` between them compiles cleanly and writes the wrong
container. Neither side can move: the facades own `codec_id` / `format_id`
because `compress.cpp` / `archive.cpp` are C++98 and dispatch on them, and DTest
owns its typed enums because they are part of an `option_set` schema. So three
explicit switches, and they earn their keep.

One enum *did* collapse — the document format — so that arm of the old bridge is
gone entirely. What remains is only the packaging axis.

### `bundle_report` / `emit_report_to_disk` / `emit_report_to_buffer`

Documents are collected lazily (a producer per format, so only the selected ones
render), extensions come from `document_format.hpp`'s single table, and the
result goes through `document_bundle` + `output_config` — which brings a naming
policy, both pack modes and a sink abstraction the hand-rolled paths lacked.

The producers stay the caller's, which is what keeps the packaging layer free of
the PDF engine.

`to_output_config` takes the four knobs as values rather than an option_set, so
this header need not know the accessor spellings.

### Deleted: `test_pack.hpp`

Its enum -> tag hop is exactly what the bridge above does, with a bundle behind
it instead of a bare facade call.

### NOT deleted: `test_zip_store.hpp`

Corrected mid-round. Its dependency-free stored-ZIP writer is the fallback when
no backend zip writer is built in — a capability `output_packaging` does **not**
have, since it dispatches *to* the facade rather than around it. That is a real
capability, not a duplicate path, and deleting it would have been a regression.
The runner's archive block keeps it and now carries a note pointing new work at
`test_packaging.hpp`.

---

## 20. Verification refreshed against the real headers

The harness stubs for the functional and expression layers were **replaced with
the real headers** you supplied (functor, recursion, comonad, semigroup, monoid,
maybe, cofree, expression, expression_ops). Only three shims remain, and each is
a header that was never provided rather than an inference: `monad.hpp` and
`foldable.hpp` (supplying the exact names `functor.hpp` / `monoid.hpp`
reference), and `parse.hpp`'s `result.hpp`.

With `expression_deduction_fix.patch` applied, the outcome is no longer
"compiles against inferred interfaces" but:

**The full layout stack compiles AND runs at C++20 against your real functional
framework.** `layout.hpp`, `layout_interpret.hpp`, `report_dialect.hpp` build;
`test_layout.hpp` renders a numbered report end-to-end (cover, page break,
`Contents`, `1 Summary`, `2 meta`, `2.1 trait_detect`, ruled assertion table) —
using the real `mappend` / `mconcat` from your `semigroup.hpp` / `monoid.hpp`,
the real `annotated_cata` from `cofree.hpp`, the real precedence-climbing
machinery in `expression.hpp`.

`2.1` is the depth the flat `report_layout` could not express, produced by the
interpreter's numbering pass over the real cofree fold. That is the whole
consolidation, working on the actual framework rather than a model of it.

This retires the largest caveat that has stood since round 4. The remaining
shims (`monad.hpp`, `foldable.hpp`, `result.hpp`) are the only interfaces still
unverified, and they are peripheral — the layout stack touches `foldable` only
through `monoid.hpp`'s `mconcat`, which is exercised and correct.
