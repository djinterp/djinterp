/*******************************************************************************
* djinterp [core]                                         table_model_render.hpp
*
*   The join between the TABLE subframework's model and the DOCUMENT
* subframework's dialects: stream a table_model_value through any
* document_renderer, so one table renders as monospace, Markdown, XML, HTML or
* PDF without a per-dialect table engine.
*
*   WHY THIS EXISTS.  Both halves already had a table.  container/table carries
* the richer MODEL -- a merge cover (regions, anchors, spans) and multi-level
* GROUPED headers on both dimensions -- and one text renderer that round-trips
* with table_parser.  util/document carries the richer DIALECT set -- plain,
* Markdown, XML, HTML, PDF -- behind one semantic sink, but a flat model
* (document_table: a column list and string rows, no merges, no row headers).
* Neither is wrong; they answer different questions.  This header lets the
* richer model reach the wider dialect set, which is what makes the several
* independent monospace table engines around the codebase redundant rather than
* merely similar.
*
*   DUCK-TYPED ON THE MODEL, DELIBERATELY.  Model is a template parameter and
* this header includes nothing from container/table.  A conforming model
* supplies rows() / cols(), raw_at(r, c), layout().owner_of(r, c) returning a
* region with anchor_row / anchor_col / rows / cols, and metadata() with the
* has_*_headers / *_headers accessors.  table_model_value satisfies it; so does
* anything shaped like it.  The document subframework therefore does not take a
* dependency on the table container family (which is C++17 and pulls the option
* pack), and a text-only build pays for neither.  This is the same decoupling
* table_metadata::conforms_to already uses -- any type with the accessors works.
*
*   SPANS ARE A DIALECT QUESTION, so they are a POLICY (span_policy):
*     hint  -- a layout cell emits ONCE, at its anchor, carrying colspan /
*              rowspan hints; covered positions are skipped entirely.  Correct
*              for HTML, which expresses merges natively.  A dialect that
*              ignores the hints sees a short row.
*     fill  -- one cell per atomic position: the anchor carries the value, the
*              positions it covers render empty.  Correct for monospace and
*              Markdown, where a row must keep its column count for the grid to
*              line up.  The merge reads visually but is not marked.
*   table_render_defaults(format) picks the right one, so a caller that does not
* want to think about it does not have to.
*
*   GROUPED HEADERS meet a protocol that has ONE header row (table_column per
* column).  Two honest answers, and no third:
*     join  -- fold a column's levels into its header text ("coordinate / x").
*              Lossless in reading, works in every dialect.  The default.
*     rows  -- emit the coarser levels as leading body rows carrying colspan.
*              Shows the grouping structurally; in markup they land in the body
*              rather than the header block, which is valid but not ideal.
*   Extending document_renderer with a header_row verb would beat both; that is
* a protocol change and deliberately not made here.
*
*   PORTABILITY:
*   C++11 baseline (matches document_renderer).
*
*
* path:      /inc/djinterp/core/util/document/templates/table_model_render.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.23
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    POLICIES                      (span_policy / header_level_policy)
      -----------------------------------------------------------------

II.   table_render_options          (+ table_render_defaults)
      -------------------------------------------------------

III.  render_table_model            (the traversal)
      ---------------------------------------------

IV.   ONE-SHOT CONVENIENCE         (see table_model_dialect.hpp)
      ----------------------------------------------------------
*/

#ifndef DJINTERP_UTIL_DOCUMENT_TEMPLATES_TABLE_MODEL_RENDER_HPP
#define DJINTERP_UTIL_DOCUMENT_TEMPLATES_TABLE_MODEL_RENDER_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <string>
#include <vector>
// djinterp
#include "../../../../djinterp.hpp"      // NS_*, D_NODISCARD, D_NOEXCEPT
#include "../document_format.hpp"     // document_format, format_is_markup
#include "./document_attributes.hpp"  // doc_attributes, doc_attr_*, text_alignment
#include "./document_renderer.hpp"    // document_renderer


NS_DJINTERP


// ===========================================================================
// I.   POLICIES
// ===========================================================================

// span_policy
//   enum: how a model's merge cover reaches the renderer.  See the header note
// -- `hint` is right for a dialect that expresses merges, `fill` for one whose
// grid depends on every row carrying every column.
enum class span_policy
{
    hint,
    fill
};


// header_level_policy
//   enum: how the coarser (grouping) header levels reach a protocol that has
// one header row.  `join` folds them into the column's header text; `rows`
// emits them as leading body rows carrying colspan.
enum class header_level_policy
{
    join,
    rows
};


// ===========================================================================
// II.  table_render_options
// ===========================================================================

// table_render_options
//   struct: how a model is streamed.  Defaults are the safe, every-dialect
// choices; table_render_defaults(format) sharpens them per dialect.
struct table_render_options
{
    // spans
    //   how the merge cover is expressed.
    span_policy         spans;

    // header_levels
    //   how grouped header levels are expressed.
    header_level_policy header_levels;

    // level_separator
    //   what joins a column's header levels under header_level_policy::join.
    std::string         level_separator;

    // emit_row_headers
    //   whether the row headers (when the model carries them) are prepended as
    // a leading column.  Its column header is empty -- the corner cell.
    bool                emit_row_headers;

    // column_aligns
    //   per-column alignment, applied to the column header and inherited by
    // its cells.  Empty leaves alignment unhinted, which is what a model that
    // does not carry presentation should do.  Indexed by DATA column, not
    // counting a prepended row-header column.
    std::vector<text_alignment> column_aligns;

    // table_render_options (default)
    //   fill spans, joined header levels, row headers emitted.
    table_render_options()
        : spans           (span_policy::fill),
          header_levels   (header_level_policy::join),
          level_separator (" / "),
          emit_row_headers(true),
          column_aligns   ()
    {}
};


// table_render_defaults
//   function: the options that suit _format.  A markup dialect expresses merges
// natively, so it takes hinted spans; every other dialect needs a full grid, so
// it takes filled ones.  Everything else is the shared default.
D_NODISCARD inline table_render_options
table_render_defaults(
    document_format _format
)
{
    table_render_options _opts;

    // xml / html carry colspan / rowspan; the rest cannot
    _opts.spans = format_is_markup(_format) ? span_policy::hint
                                            : span_policy::fill;

    return _opts;
}


// ===========================================================================
// III. render_table_model
// ===========================================================================

NS_INTERNAL

    // header_label_at_helper
    //   helper: the label covering position _pos in one header level, where a
    // cell's span covers several positions.  Empty when the level is short.
    template<typename Level>
    D_NODISCARD inline std::string
    header_label_at_helper(
        const Level& _level,
        std::size_t   _pos
    )
    {
        std::size_t _seen = 0;
        std::size_t _i    = 0;

        // walk the run, accumulating spans until _pos is covered
        for (_i = 0; _i < _level.size(); ++_i)
        {
            const std::size_t _span = (_level[_i].span > std::size_t(0))
                                          ? _level[_i].span
                                          : std::size_t(1);

            if (_pos < (_seen + _span))
            {
                return std::string(_level[_i].label);
            }

            _seen += _span;
        }

        return std::string();
    }

    // joined_header_helper
    //   helper: every level's label for position _pos, outermost first, joined
    // by _sep.  Empty levels contribute nothing, so an ungrouped header yields
    // just its own label.
    template<typename Stack>
    D_NODISCARD inline std::string
    joined_header_helper(
        const Stack&      _stack,
        std::size_t        _pos,
        const std::string& _sep
    )
    {
        std::string _out;
        std::size_t _l = 0;

        for (_l = 0; _l < _stack.size(); ++_l)
        {
            const std::string _label =
                header_label_at_helper(_stack[_l], _pos);

            // a level with nothing over this position adds no separator either
            if (_label.empty())
            {
                continue;
            }

            if (!_out.empty())
            {
                _out += _sep;
            }

            _out += _label;
        }

        return _out;
    }

    // align_attrs_helper
    //   helper: a hint bag carrying _index's alignment, or an empty bag when
    // the caller supplied none.
    D_NODISCARD inline doc_attributes
    align_attrs_helper(
        const std::vector<text_alignment>& _aligns,
        std::size_t                        _index
    )
    {
        doc_attributes _attrs;

        // an unhinted column stays unhinted -- the model carries no presentation
        if (_index < _aligns.size())
        {
            _attrs.set(doc_attr_align, align_to_string(_aligns[_index]));
        }

        return _attrs;
    }

NS_END  // internal


// render_table_model
//   function: stream _model through _renderer as one table, honouring the
// model's merge cover and headers per _opts.  The renderer decides realisation;
// this function decides only what the semantic call sequence is.
//
// Usage:
//   html_document_renderer _r;
//   render_table_model(_model, _r, table_render_defaults(document_format::html));
template<typename Model>
inline void
render_table_model(
    const Model&               _model,
    document_renderer&          _renderer,
    const table_render_options& _opts = table_render_options()
)
{
    const std::size_t _rows = static_cast<std::size_t>(_model.rows());
    const std::size_t _cols = static_cast<std::size_t>(_model.cols());

    const bool _row_heads = ( _opts.emit_row_headers &&
                              _model.metadata().has_row_headers() );

    std::size_t _r = 0;
    std::size_t _c = 0;

    _renderer.begin_table(doc_attributes());

    // --- column headers ------------------------------------------------------
    //   The finest level addresses the columns one-to-one; the coarser levels
    // group it.  Under `join` every level folds into the header text here;
    // under `rows` only the finest is a header and the rest become body rows.

    if (_model.metadata().has_column_headers())
    {
        // the corner cell above a row-header column carries no label
        if (_row_heads)
        {
            _renderer.table_column(std::string(), doc_attributes());
        }

        for (_c = 0; _c < _cols; ++_c)
        {
            const doc_attributes _attrs =
                internal::align_attrs_helper(_opts.column_aligns, _c);

            if (_opts.header_levels == header_level_policy::join)
            {
                _renderer.table_column(
                    internal::joined_header_helper(
                        _model.metadata().column_headers(),
                        _c,
                        _opts.level_separator),
                    _attrs);
            }
            else
            {
                const std::size_t _levels =
                    _model.metadata().column_headers().size();

                // the finest level is the one that addresses columns
                _renderer.table_column(
                    (_levels > std::size_t(0))
                        ? internal::header_label_at_helper(
                              _model.metadata().column_headers()[_levels - 1],
                              _c)
                        : std::string(),
                    _attrs);
            }
        }
    }

    // --- grouping levels as leading rows -------------------------------------

    if ( (_opts.header_levels == header_level_policy::rows) &&
         _model.metadata().has_column_headers() &&
         (_model.metadata().column_headers().size() > std::size_t(1)) )
    {
        const std::size_t _levels = _model.metadata().column_headers().size();
        std::size_t       _l      = 0;

        // every level ABOVE the finest, outermost first
        for (_l = 0; (_l + std::size_t(1)) < _levels; ++_l)
        {
            _renderer.begin_row(doc_attributes());

            // keep the row-header column aligned with the body
            if (_row_heads)
            {
                _renderer.cell(std::string(), doc_attributes());
            }

            std::size_t _pos = 0;
            std::size_t _k   = 0;

            for (_k = 0;
                 (_k < _model.metadata().column_headers()[_l].size()) &&
                 (_pos < _cols);
                 ++_k)
            {
                const std::size_t _span =
                    (_model.metadata().column_headers()[_l][_k].span >
                     std::size_t(0))
                        ? _model.metadata().column_headers()[_l][_k].span
                        : std::size_t(1);

                doc_attributes _attrs;

                // a grouping label spans the finer positions beneath it
                if (_span > std::size_t(1))
                {
                    _attrs.set(doc_attr_colspan, std::to_string(_span));
                }

                _renderer.cell(
                    std::string(
                        _model.metadata().column_headers()[_l][_k].label),
                    _attrs);

                // under `fill` the covered positions still need their cells
                if (_opts.spans == span_policy::fill)
                {
                    std::size_t _f = 1;

                    for (_f = 1; _f < _span; ++_f)
                    {
                        _renderer.cell(std::string(), doc_attributes());
                    }
                }

                _pos += _span;
            }

            _renderer.end_row();
        }
    }

    // --- body ----------------------------------------------------------------

    for (_r = 0; _r < _rows; ++_r)
    {
        _renderer.begin_row(doc_attributes());

        // the row's own header, as a leading cell
        if (_row_heads)
        {
            _renderer.cell(
                internal::joined_header_helper(
                    _model.metadata().row_headers(),
                    _r,
                    _opts.level_separator),
                doc_attributes());
        }

        for (_c = 0; _c < _cols; ++_c)
        {
            // the layout cell owning this position; a singleton owns itself
            const std::size_t _ar =
                static_cast<std::size_t>(
                    _model.layout().owner_of(_r, _c).anchor_row());
            const std::size_t _ac =
                static_cast<std::size_t>(
                    _model.layout().owner_of(_r, _c).anchor_col());

            const bool _is_anchor = ( (_ar == _r) &&
                                      (_ac == _c) );

            doc_attributes _attrs =
                internal::align_attrs_helper(_opts.column_aligns, _c);

            if (_opts.spans == span_policy::hint)
            {
                // a covered position was already emitted, at its anchor
                if (!_is_anchor)
                {
                    continue;
                }

                const std::size_t _sr =
                    static_cast<std::size_t>(
                        _model.layout().owner_of(_r, _c).rows);
                const std::size_t _sc =
                    static_cast<std::size_t>(
                        _model.layout().owner_of(_r, _c).cols);

                if (_sc > std::size_t(1))
                {
                    _attrs.set(doc_attr_colspan, std::to_string(_sc));
                }

                if (_sr > std::size_t(1))
                {
                    _attrs.set(doc_attr_rowspan, std::to_string(_sr));
                }

                _renderer.cell(std::string(_model.raw_at(_ar, _ac)), _attrs);

                continue;
            }

            // fill: the anchor carries the value, covered positions are blank,
            // so the grid keeps its column count
            _renderer.cell(
                _is_anchor ? std::string(_model.raw_at(_r, _c))
                           : std::string(),
                _attrs);
        }

        _renderer.end_row();
    }

    _renderer.end_table();

    return;
}


// ===========================================================================
// IV.  ONE-SHOT CONVENIENCE
// ===========================================================================
//   `render_table_model_to_string(model, format)` -- pick the dialect, apply
// the per-format defaults, render, hand back the bytes -- lives in
// table_model_dialect.hpp, not here.  It needs document_dialect.hpp, which
// includes every renderer; this header deliberately needs none of them, so a
// caller that already holds a renderer pays for nothing extra.  Same split as
// document_format.hpp / document_format_policy.hpp.


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_UTIL_DOCUMENT_TEMPLATES_TABLE_MODEL_RENDER_HPP
