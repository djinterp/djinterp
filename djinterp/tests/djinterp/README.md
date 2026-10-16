# djinterp — document / pdf / test-output consolidation

This tree is the result of a multi-round refactor of the document, PDF and
DTest-output subframeworks. `MIGRATION.md` is the full step-by-step record (20
sections); this file is the map.

---

## Apply this first

**`expression_deduction_fix.patch`** (100 lines, against your `expression.hpp`).

It is a **blocker**, not an improvement. Your `expression.hpp` defines
`annotated_expression` as a plain alias through a member template, which is a
non-deduced context — so `render_document`, `renumber`, `outline` and `emit` are
all uncallable, and the entire layout consolidation is inert until it lands. The
headers compile; the *calls* do not. Confirmed against the real header and fixed
minimally (make it a class deriving from the cofree specialisation, plus one
child-hole retype). See §16 of MIGRATION.md.

Everything else in this delivery is a net improvement that can land on its own
schedule. This one gates the layout work.

---

## What changed, at a glance

| | count | |
|---|---|---|
| **authored** (new headers) | 14 | the renderers, the bridges, the templating module, the DTest content/layout/packaging headers |
| **relocated / edited** | 31 | moved under `core/util/{document,pdf}/`, include paths + guards rewritten |
| **deleted** | 8 | see below |

**Deleted:** `test_report_runner_table.hpp` (dead fork, colliding guard) ·
`test_render_pdf_table.hpp` (882-line painter, capability absorbed) ·
`cofree_recursive.hpp` (merged into your `cofree.hpp`) · `test_pack.hpp`
(enum-tag hop, superseded by the packaging bridge) · `test_output.hpp` /
`test_output_config.hpp` / `test_render.hpp` / `test_render_pdf.hpp` (the dormant
emit stack). `test_emit.hpp` is not in this tree — delete it in yours.

---

## The shape of the result

**One semantic sink, five dialects.** `document_renderer` is the write-only
interface. Realisations: `plain_document_renderer` (monospace, pre-existing),
`markup_document_renderer<Policy>` → `xml_` / `html_`, `markdown_document_renderer`,
and two PDF backends (`pdf_document_renderer` on pdf_template,
`pdf_canvas_document_renderer` on pdf_canvas). `document_dialect.hpp` maps a
`document_format` to the right one; `render_to_string` / `render_table_model_to_string`
are the one-shots.

**One format enum.** `document_format` is the single selector; DTest's old
`doc_format` and `test_doc_type` are now aliases. `document_format.hpp` is the
light C++11 value header; `document_format_policy.hpp` holds the C++17 factory.

**Templating is renderer-agnostic.** `document_template.hpp` — declarative
skeleton, `{token}` interpolation, `tmpl_repeat` for sequences — renders to all
five dialects. This is what `pdf_template` *should* have been once layout moved
to `pdf_canvas`.

**Tables have one engine.** `pdf_canvas::table` gained `cell_format` (per-cell
fill / colour / align) and `band()`, so the hand-painted DTest table retired.
`table_model_render.hpp` bridges the container/table model (merges, grouped
headers) to any dialect.

**DTest reports are one walk.** `test_layout.hpp` folds a `test_report` into a
`layout_doc` and renders it through the interpreter's single pass — replacing two
hand-written traversals. `test_report_content.hpp` (the emitters),
`test_report_document.hpp` (the page design as a template),
`test_report_pdf.hpp` (the PDF entry point), `test_packaging.hpp` (bundle +
output_config). `test_layout_suite.hpp` is the DTest module that runs the demos
that used to live inside the library headers.

---

## verify/

Every claim in MIGRATION.md was compiled and run. `verify/verify.sh` reproduces
it (25 header checks + 13 drivers). `verify/stubs/` now holds the **real**
functional and expression headers, not inferences — only `monad.hpp`,
`foldable.hpp` and `parse.hpp`'s `result.hpp` remain shimmed, because those were
never provided. `verify/STUBS.md` is the reconciliation record.

With the patch applied, the full layout stack compiles **and runs** at C++20
against the real framework (drive13 renders a numbered report). Build outside the
tree (`BUILD=…`) if your checkout is on a noexec mount.

---

## Known state / next steps

1. **The C++20 floor is accidental.** It comes from unconditionally-`constexpr`
   destructors in `maybe`/`result`. A `D_CONSTEXPR_DTOR` macro (constexpr on
   C++20+, empty below) removes it. The functional stack's *structural* floor is
   C++14 (generic lambdas in `annotated_cata`) and no library can lower that.

2. **`pdf_template` teardown, not yet done.** With `document_template.hpp`
   landed, `pdf_template.hpp` should reduce to a thin face or be deleted — which
   orphans `pdf_printer.hpp`, `pdf_template_traits.hpp` and `pdf_options.hpp`.
   Left undone deliberately: four headers is more than should be removed without
   a compile pass behind the removal.

3. **`pdf_document_renderer` vs `pdf_canvas_document_renderer`.** Both ship. Once
   (2) is decided the canvas one is the survivor (better metrics, real tables);
   the open question is whether `text_template` token binding moves onto canvas
   or is dropped (it is now covered by `document_template.hpp` at the document
   layer regardless).

4. **restd migration** is viable for the document/PDF side (mostly
   string/vector/function/optional) but needs `restd::function` / `restd::optional`
   first. Best treated as its own project starting with the least-entangled
   layer (pdf). Leave the functional stack where it is.

5. **Minor, unaddressed:** the `metadata_concepts.hpp` / `metadata_traits.hpp`
   doubled-`core/` include (out of scope, flagged §7); the `table_options` name
   collision between `container/table/table_options.hpp` (no such type) and
   `pdf_canvas.hpp` (defines it) (§6).
