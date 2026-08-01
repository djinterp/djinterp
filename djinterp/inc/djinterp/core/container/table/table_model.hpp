/******************************************************************************
* djinterp [container]                                          table_model.hpp
*
*   THE MEETING POINT.  A table has two declaration front ends -- table_builder
* (C++ types) and table_parser (text) -- and the requirement that anything
* expressible in one be expressible in the other, identically.  That is not a
* convention to be maintained by hand: parse and compose are OPPOSITE LEGS OF ONE
* PRISM (ch-parsing.tex), and a prism needs a carrier both legs speak about.  This
* header is that carrier.  Both front ends produce a table_model; neither knows
* the other exists.
*
*                 types  --fold-->  [ table_model ]  <--parse--  text
*                                      |      |
*                             realize  |      |  render (compose)
*                                      v      v
*                                 container   text
*
*   THE MODEL IS THE FORMAL TRIPLE T = (T_, I_T, Gamma) (containers.tex), in two
* incarnations that answer the same questions:
*     - table_model<...>       a TYPE: the shape (I_T + the cell types) and the
*                              layout (Gamma) the builder folds, plus the option
*                              packs.  Pure compile time; no storage.
*     - table_model_value<...> a VALUE: the same three layers as data -- cells,
*                              a runtime_layout, and the headers / open key-value
*                              metadata of table_metadata.  What the parser fills.
*
*   RENDER IS THE COMPOSE LEG, and it is here rather than in the parser because it
* is a property of the MODEL: a table knows how to write itself down.  With parse
* as its inverse, the round-trip laws (parse . render = id; render . parse = id on
* the language) are statable, and the two front ends are provably the same DSL.
*
*   ALIGNMENT CARRIES THE SPANS.  In the text surface a merged cell is not marked;
* it is SEEN -- its span is read from where the delimiters fall:
*
*         |     |   something   |  something else |     <- 3 cells
*         | foo | bar  | alpha  | beta  | gamma   |     <- 5 cells
*
* "something" spans bar and alpha because it covers their columns.  Render is
* therefore column-aligned by construction: it computes a width per column and
* gives a merged cell the exact width its span would have occupied, so the
* delimiters line up and the span survives the round trip.
*
*   PORTABILITY:
*   C++11 for the type-level model; the value model and render use std::string /
* std::vector (runtime).  The option surface is C++17 (as table_options).
*
*
* path:      /inc/djinterp/core/container/table/table_model.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.14
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    table_model                (the compile-time model)
II.   model_of                   (table_builder -> table_model)
III.  table_model_value          (the runtime model)
IV.   render                     (the compose leg: model -> canonical text)
V.    detection traits
VI.   concepts                   (C++20 analogs)
*/

#ifndef DJINTERP_CONTAINER_TABLE_MODEL_
#define DJINTERP_CONTAINER_TABLE_MODEL_ 1

// std
#include <cstddef>
#include <string>
#include <type_traits>
#include <vector>
// djinterp
#include "../../djinterp.hpp"                    // NS_*, D_CONSTEXPR, clean_t, D_ENV_*
#include "./table_shape.hpp"                      // table_shape, realization_of
#include "./table_layout.hpp"                     // layout, runtime_layout, region_value
#include "./table_options.hpp"                    // the policy vocabulary (+ surface)
#include "../metadata/table_metadata.hpp"         // table_metadata: headers + metadata
#include "../../../config/core/container/table/cfg_table.h"   // the dialect + gates


NS_DJINTERP


// ===========================================================================
// I.   table_model
// ===========================================================================

// table_model
//   type: the compile-time model -- the shape (the domain I_T and the cell types)
// and the layout (the cover Gamma), with the option packs that configure how the
// declaration was read and what the realized container must be.  This is what a
// table_builder IS, once its declarators are folded; keeping it a separate type
// lets the parser name the same thing.
//
//   _Shape       : a table_shape<Height, Cols...>
//   _Layout      : a layout<Regions...> (trivial_layout = the ordinary table)
//   _TableOpts   : the table policy set   (an option_set; C++17)
//   _AxisOpts    : the container axis set (an option_set; C++17)
template<typename _Shape,
         typename _Layout    = trivial_layout,
         typename _TableOpts = void,
         typename _AxisOpts  = void>
struct table_model
{
    static_assert(is_table_shape<_Shape>::value,
        "table_model: _Shape must be a table_shape.");

    static_assert(is_layout<_Layout>::value,
        "table_model: _Layout must be a layout<> (the cover Gamma).");

    // --- the three layers ---

    using shape_type  = _Shape;
    using layout_type = _Layout;

    using table_options_type = _TableOpts;
    using axis_options_type  = _AxisOpts;

    // --- the domain I_T ---

    static D_CONSTEXPR std::size_t rank   = _Shape::rank;
    static D_CONSTEXPR std::size_t width  = _Shape::width;
    static D_CONSTEXPR std::size_t height = _Shape::height;

    static D_CONSTEXPR bool static_height = _Shape::static_height;

    // --- the cell types ---

    static D_CONSTEXPR bool is_homogeneous = _Shape::is_homogeneous;

    using element_type = typename _Shape::element_type;
    using columns_type = typename _Shape::columns_type;

    // column_type_t -- the cell type of column _C (tau_c).
    template<std::size_t _C>
    using column_type_t = typename _Shape::template column_type_t<_C>;

    // --- the cover Gamma ---

    static D_CONSTEXPR bool        has_merges  = _Layout::has_merges;
    static D_CONSTEXPR std::size_t merge_count = _Layout::merge_count;

    // owner_of -- the layout cell owning atomic position (_R, _C).  Read its
    // anchor for the layout-aware access T[i] = T[anchor(cell_T(i))].
    template<std::size_t _R,
             std::size_t _C>
    using owner_of = typename _Layout::template owner_of<_R, _C>;

    // --- realization ---

    // realization -- the container family the shape backs onto.
    static D_CONSTEXPR realization_kind realization =
        realization_of<_Shape>::value;

    // --- conformance ---

    // conforms -- the cover is valid for the domain (every merge in bounds, no
    // two overlapping).
    static D_CONSTEXPR bool conforms =
        layout_valid<_Layout, height, width>::value;

    static_assert(conforms,
        "table_model: the layout is not a valid cover of the shape -- a merge "
        "runs past the extents, or two merges overlap.");
};


// ===========================================================================
// II.  model_of
// ===========================================================================

// model_of
//   trait: the table_model a table_builder folds to.  The builder's declarator
// surface and the model are deliberately different types -- one is the DSL, the
// other the carrier -- and this is the arrow between them.  Any front end that
// can name a shape and a layout can produce a model the same way.
//
// Usage:
//   using t = table_builder<columns<int,int>, row<a, b>>;
//   using m = model_of_t<t>;      // == table_model<table_shape<1,int,int>, ...>
template<typename _Builder,
         typename = void>
struct model_of;

// model_of (a builder with the C++17 option surface)
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

template<typename _Builder>
struct model_of<_Builder, void_t<typename _Builder::shape_type>>
{
    using type = table_model<typename _Builder::shape_type,
                             typename _Builder::layout_type,
                             typename _Builder::table_options_type,
                             typename _Builder::axis_options_type>;
};

#else

template<typename _Builder>
struct model_of<_Builder, void_t<typename _Builder::shape_type>>
{
    using type = table_model<typename _Builder::shape_type,
                             typename _Builder::layout_type>;
};

#endif

// model_of_t
//   type: shorthand for model_of<_Builder>::type.
template<typename _Builder>
using model_of_t = typename model_of<_Builder>::type;


// ===========================================================================
// III. table_model_value
// ===========================================================================

// table_model_value
//   class: the runtime model -- the same three layers as VALUES.  The atomic
// cells sit in a flat store of height x width; the cover is a runtime_layout;
// the headers and the open key-value metadata are a table_metadata.  This is what
// the text front end fills and what render writes back out.
//
//   Layout-aware access is the formal rule, honoured here: at(r, c) reads the
// ANCHOR of the cell owning (r, c), so every position a merge covers reports the
// merge's one value (T[i] = T[anchor(cell_T(i))]).  raw_at reaches past the
// overlay to the atomic store.
//
//   _Cell     : the cell type (std::string for the text surface -- the parser's
//               natural output before any typing).
//   _Metadata : the metadata / header carrier (a table_metadata).
template<typename _Cell     = std::string,
         typename _Metadata = table_metadata<>>
class table_model_value
{
public:
    // --- member types ---

    using cell_type     = _Cell;
    using metadata_type = _Metadata;
    using size_type     = std::size_t;
    using store_type    = std::vector<_Cell>;

    // --- construction ---

    table_model_value()
        : m_rows(0),
          m_cols(0),
          m_cells(),
          m_layout(),
          m_metadata()
    {}

    // an atomic table of _rows x _cols, cells value-initialized.
    table_model_value(
        size_type _rows,
        size_type _cols
    )
        : m_rows(_rows),
          m_cols(_cols),
          m_cells(_rows * _cols),
          m_layout(),
          m_metadata()
    {}

    // --- the domain I_T ---

    D_NODISCARD size_type rows() const D_NOEXCEPT
    {
        return m_rows;
    }

    D_NODISCARD size_type cols() const D_NOEXCEPT
    {
        return m_cols;
    }

    // row_count / column_count -- the names table_metadata::conforms_to reads.
    D_NODISCARD size_type row_count() const D_NOEXCEPT
    {
        return m_rows;
    }

    D_NODISCARD size_type column_count() const D_NOEXCEPT
    {
        return m_cols;
    }

    D_NODISCARD size_type size() const D_NOEXCEPT
    {
        return m_cells.size();
    }

    D_NODISCARD bool empty() const D_NOEXCEPT
    {
        return m_cells.empty();
    }

    // rank -- 2 for this rectangular row/column model.
    D_NODISCARD static D_CONSTEXPR size_type rank() D_NOEXCEPT
    {
        return 2;
    }

    // resize -- re-shape the atomic table, preserving nothing.
    void resize(
        size_type _rows,
        size_type _cols
    )
    {
        m_rows = _rows;
        m_cols = _cols;
        m_cells.assign(_rows * _cols, _Cell());

        return;
    }

    // --- cells ---

    // at -- the value at (_r, _c), read through the overlay: a position a merge
    // covers reports the merge's value, held at its anchor.
    D_NODISCARD const _Cell& at(
        size_type _r,
        size_type _c
    ) const
    {
        const region_value _owner = m_layout.owner_of(_r, _c);

        return raw_at(_owner.anchor_row(), _owner.anchor_col());
    }

    // raw_at -- the atomic store, past the overlay.
    D_NODISCARD const _Cell& raw_at(
        size_type _r,
        size_type _c
    ) const
    {
        return m_cells[index_of(_r, _c)];
    }

    D_NODISCARD _Cell& raw_at(
        size_type _r,
        size_type _c
    )
    {
        return m_cells[index_of(_r, _c)];
    }

    // set -- write the atomic cell at (_r, _c).
    void set(
        size_type _r,
        size_type _c,
        _Cell     _value
    )
    {
        m_cells[index_of(_r, _c)] = static_cast<_Cell&&>(_value);

        return;
    }

    // --- the cover Gamma ---

    // merge -- declare a merged cell spanning _rows x _cols from (_r, _c), whose
    // one value is _value (held at the anchor, which is (_r, _c)).
    void merge(
        size_type _r,
        size_type _c,
        size_type _rows,
        size_type _cols,
        _Cell     _value
    )
    {
        m_layout.add_merge(_r, _c, _rows, _cols);
        set(_r, _c, static_cast<_Cell&&>(_value));

        return;
    }

    D_NODISCARD const runtime_layout& layout() const D_NOEXCEPT
    {
        return m_layout;
    }

    D_NODISCARD runtime_layout& layout() D_NOEXCEPT
    {
        return m_layout;
    }

    D_NODISCARD bool has_merges() const D_NOEXCEPT
    {
        return m_layout.has_merges();
    }

    // --- headers / metadata ---

    D_NODISCARD const _Metadata& metadata() const D_NOEXCEPT
    {
        return m_metadata;
    }

    D_NODISCARD _Metadata& metadata() D_NOEXCEPT
    {
        return m_metadata;
    }

    // --- conformance ---

    // conforms -- the cover is valid for the domain AND the headers span the
    // dimensions (table_metadata's own check, read through row_count /
    // column_count).
    D_NODISCARD bool conforms() const
    {
        return ( m_layout.valid(m_rows, m_cols) &&
                 m_metadata.conforms_to(*this) );
    }

private:
    // index_of -- row-major flattening of the atomic domain.
    D_NODISCARD size_type index_of(
        size_type _r,
        size_type _c
    ) const D_NOEXCEPT
    {
        return ((_r * m_cols) + _c);
    }

    size_type      m_rows;
    size_type      m_cols;
    store_type     m_cells;      // the atomic cells, row-major
    runtime_layout m_layout;     // the cover Gamma
    _Metadata      m_metadata;   // headers + open key-value entries
};


// ===========================================================================
// IV.  render
// ===========================================================================
//   The COMPOSE leg: a model, written down.  Alignment is the point -- a merged
// cell is recognised in text by covering its columns' delimiters, so render gives
// each column a width and a merged cell exactly the width its span occupied.  The
// delimiters then line up and a parser can read the spans straight back off them.
//
//   A singleton in column c renders as  "| " + pad(text, w[c]) + " ", so it costs
// w[c] + 3 characters including its left delimiter.  A cell spanning n columns
// from c0 must cost the same as the n singletons it replaces:
//
//       W + 3  =  sum(w[c0..c0+n-1]) + 3n     =>    W = sum(w) + 3(n-1)
//
// which is merged_width below.  That identity is what keeps the grid square.

// render_options
//   struct: the knobs of the text surface.  Defaults match the sketch's dialect:
// a pipe delimiter, a space of padding either side, a dashed separator under the
// header block.
struct render_options
{
    char        delimiter;       // the cell delimiter
    char        pad;             // the padding character
    char        separator_fill;  // the header/body separator's fill
    bool        emit_separator;  // write a separator when headers are present
    std::size_t min_width;       // the least width any column may render at

    //   The characters come from cfg_table.h, not from a literal here: render and
    // parse must agree on them or the round trip cannot close, so they are
    // configured ONCE and read by both.
    render_options()
        : delimiter(D_INTERNAL_TABLE_DELIMITER),
          pad(D_INTERNAL_TABLE_PAD),
          separator_fill(D_INTERNAL_TABLE_SEPARATOR_FILL),
          emit_separator(true),
          min_width(1)
    {}
};

NS_INTERNAL

    // cell_text
    //   function: a cell's text.  A std::string cell is already text; any other
    // cell type opts in by being convertible to one (the text surface's cells are
    // strings by construction, so this is the identity in practice).
    inline const std::string&
    cell_text(const std::string& _c)
    {
        return _c;
    }

    // pad_to
    //   function: _text padded to _width with _fill (never truncated: a cell
    // wider than its column widens the column instead, see column_widths).
    inline std::string
    pad_to(
        const std::string& _text,
        std::size_t        _width,
        char               _fill)
    {
        std::string _out = _text;

        // grow to the column's width; an over-long cell is left intact
        while (_out.size() < _width)
        {
            _out.push_back(_fill);
        }

        return _out;
    }

    // merged_width
    //   function: the field width a cell spanning _span columns from _c0 must
    // render at to occupy exactly what the singletons it replaces would have.
    inline std::size_t
    merged_width(
        const std::vector<std::size_t>& _w,
        std::size_t                     _c0,
        std::size_t                     _span)
    {
        std::size_t _total = 0;

        for (std::size_t _i = 0; _i < _span; ++_i)
        {
            _total += _w[_c0 + _i];
        }

        // the interior delimiters and their padding become part of the field
        return (_total + (3 * (_span - 1)));
    }

NS_END  // internal

// is_header_stack
//   trait: whether a header type is the DEFAULT representation table_metadata
// ships, with TEXT labels (a stack of levels of labelled, spannable cells).
// Header types are OPEN there -- a caller may store a header set any way at all,
// under any label type -- so render, which must read labels and spans to place
// them on the grid, can only lay out a representation it understands: a stack,
// whose labels are already text.  This is the same stance table_metadata takes
// with header_extent: a type that does not opt in is treated as opaque, and is
// simply not rendered rather than being second-guessed.
//
//   Narrow on purpose.  A header_stack<some_other_label> IS a stack, but render
// has no way to turn its labels into text, so admitting it here would trade a
// clean "not rendered" for a hard compile error at the first pad_to.
template<typename _Type>
struct is_header_stack : std::false_type
{};

template<typename _AllocCell,
         typename _AllocLevel>
struct is_header_stack<
    std::vector<std::vector<header_cell<std::string>, _AllocCell>, _AllocLevel>>
    : std::true_type
{};

NS_INTERNAL

    // apply_header_widths (a readable stack)
    //   function: let the header labels claim their columns.  A header level is a
    // row of the grid, so its labels constrain the column widths exactly as body
    // cells do -- a unit-span label widens its own column, and an over-long
    // spanning label widens the last column of its span.
    template<typename _Stack>
    inline void
    apply_header_widths(
        const _Stack&             _stack,
        std::vector<std::size_t>& _w,
        std::true_type)
    {
        // 1. unit-span labels must fit their own column
        for (std::size_t _l = 0; _l < _stack.size(); ++_l)
        {
            std::size_t _c = 0;

            for (std::size_t _i = 0; _i < _stack[_l].size(); ++_i)
            {
                const std::size_t _span =
                    (_stack[_l][_i].span > 0) ? _stack[_l][_i].span : 1;

                if ((_c + _span) > _w.size())
                {
                    break;
                }

                const std::size_t _len = _stack[_l][_i].label.size();

                if ((_span == 1) && (_len > _w[_c]))
                {
                    _w[_c] = _len;
                }

                _c += _span;
            }
        }

        // 2. spanning labels must fit the span they cover; the deficit widens the
        //    span's last column, as for a merged body cell
        for (std::size_t _l = 0; _l < _stack.size(); ++_l)
        {
            std::size_t _c = 0;

            for (std::size_t _i = 0; _i < _stack[_l].size(); ++_i)
            {
                const std::size_t _span =
                    (_stack[_l][_i].span > 0) ? _stack[_l][_i].span : 1;

                if ((_c + _span) > _w.size())
                {
                    break;
                }

                if (_span > 1)
                {
                    const std::size_t _need = _stack[_l][_i].label.size();
                    const std::size_t _have = merged_width(_w, _c, _span);

                    if (_need > _have)
                    {
                        _w[_c + _span - 1] += (_need - _have);
                    }
                }

                _c += _span;
            }
        }

        return;
    }

    // apply_header_widths (an opaque stack)
    //   function: a header representation render cannot read contributes no
    // width -- it is the header type's own business, per table_metadata.
    template<typename _Stack>
    inline void
    apply_header_widths(
        const _Stack&             /*_stack*/,
        std::vector<std::size_t>& /*_w*/,
        std::false_type)
    {
        return;
    }

NS_END  // internal

// column_widths
//   function: the render width of each column -- the widest SINGLETON cell in it
// (a merged cell constrains no single column), floored at _opts.min_width, then
// widened where a merged cell or a header label would otherwise overflow the span
// it covers.  The widening lands on the span's last column, which is deterministic
// and keeps every earlier column at its natural width.
template<typename _Cell,
         typename _Metadata>
D_NODISCARD std::vector<std::size_t>
column_widths(
    const table_model_value<_Cell, _Metadata>& _model,
    const render_options&                      _opts = render_options())
{
    const std::size_t _rows = _model.rows();
    const std::size_t _cols = _model.cols();

    std::vector<std::size_t> _w(_cols, _opts.min_width);

    // 1. every singleton cell must fit its own column
    for (std::size_t _r = 0; _r < _rows; ++_r)
    {
        for (std::size_t _c = 0; _c < _cols; ++_c)
        {
            const region_value _owner = _model.layout().owner_of(_r, _c);

            // a covered position contributes nothing to a single column's width
            if (_owner.is_merge())
            {
                continue;
            }

            const std::size_t _len =
                internal::cell_text(_model.raw_at(_r, _c)).size();

            if (_len > _w[_c])
            {
                _w[_c] = _len;
            }
        }
    }

    // 2. the headers are rows of the grid too, so their labels claim columns on
    //    the same terms (when the header representation is one render can read)
    if (_model.metadata().has_column_headers())
    {
        using headers_t = typename _Metadata::column_headers_type;

        internal::apply_header_widths(
            _model.metadata().column_headers(), _w,
            is_header_stack<headers_t>{});
    }

    // 3. a merged cell must fit the span it covers; the deficit widens the span's
    //    last column, so the identity W = sum(w) + 3(n-1) still holds
    const runtime_layout::merge_store& _merges = _model.layout().merges();

    for (std::size_t _i = 0; _i < _merges.size(); ++_i)
    {
        const region_value& _m = _merges[_i];

        const std::size_t _need =
            internal::cell_text(_model.raw_at(_m.row0, _m.col0)).size();

        const std::size_t _have =
            internal::merged_width(_w, _m.col0, _m.cols);

        if (_need > _have)
        {
            _w[_m.col0 + _m.cols - 1] += (_need - _have);
        }
    }

    return _w;
}

NS_INTERNAL

    // render_row
    //   function: one row of the grid.  Walks the row's LAYOUT cells (not its
    // atomic positions): a cell is written once, at its anchor, at the width its
    // span occupies; the positions it covers are skipped, which is exactly what
    // makes the delimiters fall where the span can be read back.
    //
    //   A cell whose anchor lies in an EARLIER row (a vertical merge covering
    // this one) has no value to write here, so the span renders blank -- the
    // value lives at the anchor, per T[i] = T[anchor(cell_T(i))].
    template<typename _Cell,
             typename _Metadata>
    inline std::string
    render_row(
        const table_model_value<_Cell, _Metadata>& _model,
        const std::vector<std::size_t>&            _w,
        std::size_t                                _r,
        const render_options&                      _opts)
    {
        std::string _line;

        std::size_t _c = 0;

        while (_c < _model.cols())
        {
            const region_value _owner = _model.layout().owner_of(_r, _c);

            const std::size_t _span  = _owner.cols;
            const std::size_t _field = internal::merged_width(_w, _c, _span);

            // the value is written at its anchor row; a covered row renders blank
            const bool _at_anchor = (_owner.anchor_row() == _r);

            const std::string _text =
                _at_anchor
                    ? internal::cell_text(
                          _model.raw_at(_owner.anchor_row(), _owner.anchor_col()))
                    : std::string();

            _line.push_back(_opts.delimiter);
            _line.push_back(_opts.pad);
            _line.append(internal::pad_to(_text, _field, _opts.pad));
            _line.push_back(_opts.pad);

            _c += _span;
        }

        _line.push_back(_opts.delimiter);

        return _line;
    }

    // render_separator
    //   function: the header/body separator -- the line that says "the rows above
    // are headers".  Spans the full grid width so it cannot be mistaken for a row.
    inline std::string
    render_separator(
        const std::vector<std::size_t>& _w,
        const render_options&           _opts)
    {
        std::size_t _inner = 0;

        // the separator matches the grid's full inner width
        for (std::size_t _i = 0; _i < _w.size(); ++_i)
        {
            _inner += (_w[_i] + 3);
        }

        // less the leading delimiter, which is written separately
        _inner = (_inner > 0) ? (_inner - 1) : 0;

        std::string _line;

        _line.push_back(_opts.delimiter);
        _line.append(_inner, _opts.separator_fill);
        _line.push_back(_opts.delimiter);

        return _line;
    }

    // render_header_level
    //   function: one level of a header stack, rendered as a row.  A header cell
    // carries its own span, so it lands on the grid exactly as a merged body cell
    // does -- the same alignment identity, the same readability back.
    template<typename _Label>
    inline std::string
    render_header_level(
        const header_level<_Label>&     _level,
        const std::vector<std::size_t>& _w,
        const render_options&           _opts)
    {
        std::string _line;

        std::size_t _c = 0;

        for (std::size_t _i = 0; _i < _level.size(); ++_i)
        {
            const std::size_t _span =
                (_level[_i].span > 0) ? _level[_i].span : 1;

            // a header level may not overrun the grid
            if ((_c + _span) > _w.size())
            {
                break;
            }

            const std::size_t _field = internal::merged_width(_w, _c, _span);

            _line.push_back(_opts.delimiter);
            _line.push_back(_opts.pad);
            _line.append(internal::pad_to(_level[_i].label, _field, _opts.pad));
            _line.push_back(_opts.pad);

            _c += _span;
        }

        _line.push_back(_opts.delimiter);

        return _line;
    }

    // render_header_levels (a readable stack)
    //   function: every level of a header stack, outermost first, each as a row.
    template<typename _Stack>
    inline std::string
    render_header_levels(
        const _Stack&                   _stack,
        const std::vector<std::size_t>& _w,
        const render_options&           _opts,
        std::true_type)
    {
        std::string _out;

        for (std::size_t _l = 0; _l < _stack.size(); ++_l)
        {
            _out.append(render_header_level(_stack[_l], _w, _opts));
            _out.push_back('\n');
        }

        // the separator closes the header block and marks the rows above as
        // headers -- the signal the text surface reads them back by
        if (_opts.emit_separator && (!_stack.empty()))
        {
            _out.append(render_separator(_w, _opts));
            _out.push_back('\n');
        }

        return _out;
    }

    // render_header_levels (an opaque stack)
    //   function: a header representation render cannot read writes nothing.
    template<typename _Stack>
    inline std::string
    render_header_levels(
        const _Stack&                   /*_stack*/,
        const std::vector<std::size_t>& /*_w*/,
        const render_options&           /*_opts*/,
        std::false_type)
    {
        return std::string();
    }

NS_END  // internal

// render
//   function: the model as canonical text -- the compose leg of the prism.  The
// column headers stack above a separator, the body follows, and every cell falls
// at the column its span covers, so parse can read the shape, the contents, and
// the cover straight back off the delimiters.
//
// Example (a 3-column table whose first row merges columns 1-2):
//   | a | bc     |
//   | d | e | f  |
template<typename _Cell,
         typename _Metadata>
D_NODISCARD std::string
render(
    const table_model_value<_Cell, _Metadata>& _model,
    const render_options&                      _opts = render_options())
{
    const std::vector<std::size_t> _w = column_widths(_model, _opts);

    std::string _out;

    // 1. the column headers, outermost level first, then the separator
    if (_model.metadata().has_column_headers())
    {
        using headers_t = typename _Metadata::column_headers_type;

        _out.append(internal::render_header_levels(
            _model.metadata().column_headers(), _w, _opts,
            is_header_stack<headers_t>{}));
    }

    // 2. the body
    for (std::size_t _r = 0; _r < _model.rows(); ++_r)
    {
        _out.append(internal::render_row(_model, _w, _r, _opts));
        _out.push_back('\n');
    }

    return _out;
}


// ===========================================================================
// V.   detection traits
// ===========================================================================

NS_INTERNAL

    template<typename _Type>
    struct is_table_model_impl : std::false_type
    {};

    template<typename _S,
             typename _L,
             typename _T,
             typename _A>
    struct is_table_model_impl<table_model<_S, _L, _T, _A>> : std::true_type
    {};

    template<typename _Type>
    struct is_table_model_value_impl : std::false_type
    {};

    template<typename _C,
             typename _M>
    struct is_table_model_value_impl<table_model_value<_C, _M>> : std::true_type
    {};

NS_END  // internal

// is_table_model
//   trait: true iff _Type (after stripping cv/ref) is a table_model.
template<typename _Type>
struct is_table_model : internal::is_table_model_impl<clean_t<_Type>>
{};

// is_table_model_value
//   trait: true iff _Type (after stripping cv/ref) is a table_model_value.
template<typename _Type>
struct is_table_model_value
    : internal::is_table_model_value_impl<clean_t<_Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename _Type>
D_CONSTEXPR bool is_table_model_v = is_table_model<_Type>::value;

template<typename _Type>
D_CONSTEXPR bool is_table_model_value_v = is_table_model_value<_Type>::value;
#endif


// ===========================================================================
// VI.  concepts   (C++20 analogs)
// ===========================================================================

#if D_INTERNAL_TABLE_CONCEPTS

// TableModel
//   concept: satisfied iff _Type is a table_model (the compile-time carrier).
template<typename _Type>
concept TableModel = is_table_model_v<_Type>;

// TableModelValue
//   concept: satisfied iff _Type is a table_model_value (the runtime carrier).
template<typename _Type>
concept TableModelValue = is_table_model_value_v<_Type>;

#endif  // D_INTERNAL_TABLE_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_TABLE_MODEL_
