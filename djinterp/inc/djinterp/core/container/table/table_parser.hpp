/******************************************************************************
* djinterp [container]                                         table_parser.hpp
*
*   The TEXT DSL -- the front end that reads a table written as a grid, and the
* PARSE leg of the prism (ch-parsing.tex).  Its opposite leg, render, lives on the
* model (table_model.hpp), and the two are inverse on the recognised language:
*
*       parse(render(m)) = m                (a model survives being written down)
*       render(parse(s)) = s                for every s the grammar accepts
*
*   That pair is the whole point.  table_builder folds C++ types into a
* table_model; this folds TEXT into the same table_model; so anything expressible
* in one front end is expressible in the other, not by parallel maintenance but
* because both name one carrier and the text leg is invertible.
*
*   ALIGNMENT CARRIES THE SPANS.  A merged cell is not announced in the text -- it
* is SEEN.  Its span is read from where the delimiters fall:
*
*         |     |   something   |  something else |     <- 3 cells
*         | foo | bar  | alpha  | beta  | gamma   |     <- 5 cells
*         |---------------------------------------|     <- the header separator
*         | 1   | 2    | 3      | 4     | 5       |
*
* "something" spans bar and alpha because it COVERS their columns.  So the parse
* is: collect every line's delimiter offsets; take the finest line's offsets as
* the column BOUNDARIES; then a cell running from one delimiter to the next spans
* however many boundaries it covers.  A cell that covers no interior boundary is
* atomic; one that covers k of them is a merge of k+1 columns, registered in the
* model's cover Gamma at its anchor -- the lex-least position it covers.
*
*   THE SEPARATOR names the header block: the rows above it are header levels
* (which stack, outermost first, and may group -- an outer label spanning the
* finer ones beneath), the rows below are the body.  With no separator the whole
* grid is body.
*
*   STRICTNESS is the option surface's (table_options.hpp).  A row whose cells do
* not cover the boundaries exactly is an error under `exact`, tolerated under
* truncate / pad / lenient, in the direction each names.
*
*   PORTABILITY:
*   C++11 baseline.  Built on parse.hpp's parse_state / parse_result / parse_error
* vocabulary (which lives in djinterp::parse, hence the parse:: qualification
* throughout), so a failure reports the offset and a message like any other parse
* in the framework.
*
*
* path:      /inc/djinterp/core/container/table/table_parser.hpp
* link(s):   ch-parsing.tex
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.15
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    parse_grid_options          (the dialect's knobs)
II.   grid_line                   (one scanned line: its delimiter offsets)
III.  scanning                    (text -> grid_lines; separator detection)
IV.   boundaries                  (the column boundaries; span_of)
V.    parse_table                 (grid -> table_model_value)
VI.   round_trip                  (the prism laws, as checkable predicates)
*/

#ifndef DJINTERP_CONTAINER_TABLE_PARSER_
#define DJINTERP_CONTAINER_TABLE_PARSER_ 1

// std
#include <cstddef>
#include <string>
#include <vector>
// djinterp
#include "../../djinterp.hpp"        // NS_*, D_CONSTEXPR, D_NODISCARD
#include "../../../parse/parse.hpp"      // parse_state, parse_result, parse_error,
                                      // parse_status + DParseStatus* codes
#include "./table_model.hpp"          // table_model_value, render, render_options
#include "./table_options.hpp"        // the strictness vocabulary
#include "../../../config/core/container/table/cfg_table.h"   // the dialect + gates


NS_DJINTERP


// ===========================================================================
// I.   parse_grid_options
// ===========================================================================

// parse_grid_options
//   struct: the text dialect's knobs -- the runtime face of the policy surface.
// The option pack fixes these at compile time (table_options.hpp); this carries
// the same grades for text whose strictness is only known when it is read.
struct parse_grid_options
{
    char             delimiter;       // the cell delimiter
    char             separator_fill;  // the fill that marks a separator line
    table_strictness cell_count;      // how strictly a row must cover the columns
    trim_policy      trim;            // whether cell text is trimmed
    bool             skip_blank;      // ignore blank lines between rows

    //   Every field is READ from the resolved config: the characters so render
    // and parse cannot drift apart, and the grades so the runtime dialect starts
    // where the compile-time defaults do.  One knob, one home.
    parse_grid_options()
        : delimiter(D_INTERNAL_TABLE_DELIMITER),
          separator_fill(D_INTERNAL_TABLE_SEPARATOR_FILL),
          cell_count(default_cell_count),
          trim(default_trim),
          skip_blank(true)
    {}
};


// ===========================================================================
// II.  grid_line
// ===========================================================================

// grid_line
//   struct: one scanned line of the grid -- where its delimiters fell and what
// lay between them.  The delimiter OFFSETS are the load-bearing part: they are
// what the spans are read from.
struct grid_line
{
    std::vector<std::size_t> delimiters;   // the delimiter column offsets
    std::vector<std::string> cells;        // the text between them
    std::size_t              line_offset;  // where the line began in the input
    bool                     is_separator; // a rule line, not a row

    grid_line()
        : delimiters(),
          cells(),
          line_offset(0),
          is_separator(false)
    {}

    // cell_count -- the cells the line carries (one fewer than its delimiters).
    D_NODISCARD std::size_t cell_count() const
    {
        return cells.size();
    }
};


// ===========================================================================
// III. scanning
// ===========================================================================

NS_INTERNAL

    // is_space
    //   function: the trimmable characters of the text surface.
    inline bool
    grid_is_space(char _c)
    {
        return ( (_c == ' ')  || (_c == '\t') ||
                 (_c == '\r') || (_c == '\n') );
    }

    // grid_trim
    //   function: _text without its leading and trailing whitespace.
    inline std::string
    grid_trim(const std::string& _text)
    {
        std::size_t _b = 0;
        std::size_t _e = _text.size();

        while ((_b < _e) && grid_is_space(_text[_b]))
        {
            ++_b;
        }

        while ((_e > _b) && grid_is_space(_text[_e - 1]))
        {
            --_e;
        }

        return _text.substr(_b, _e - _b);
    }

    // looks_like_separator
    //   function: whether a line is a RULE -- delimiters and fill only, with at
    // least one fill character.  This is what marks the rows above it as headers.
    inline bool
    looks_like_separator(
        const std::string&       _line,
        const parse_grid_options& _opts)
    {
        bool _saw_fill = false;

        for (std::size_t _i = 0; _i < _line.size(); ++_i)
        {
            const char _c = _line[_i];

            if (_c == _opts.separator_fill)
            {
                _saw_fill = true;

                continue;
            }

            // anything that is not a delimiter or trimmable is real content
            if ((_c != _opts.delimiter) && (!grid_is_space(_c)))
            {
                return false;
            }
        }

        return _saw_fill;
    }

NS_END  // internal

// scan_line
//   function: one line of the grid -- its delimiter offsets and the text between
// them.  The offsets are recorded RELATIVE TO THE LINE, which is the coordinate
// the spans are read in.
D_NODISCARD inline grid_line
scan_line(
    const std::string&        _line,
    std::size_t               _line_offset,
    const parse_grid_options& _opts = parse_grid_options())
{
    grid_line _g;

    _g.line_offset  = _line_offset;
    _g.is_separator = internal::looks_like_separator(_line, _opts);

    // record every delimiter's column
    for (std::size_t _i = 0; _i < _line.size(); ++_i)
    {
        if (_line[_i] == _opts.delimiter)
        {
            _g.delimiters.push_back(_i);
        }
    }

    // the text between consecutive delimiters is a cell
    for (std::size_t _i = 0; (_i + 1) < _g.delimiters.size(); ++_i)
    {
        const std::size_t _b = _g.delimiters[_i] + 1;
        const std::size_t _e = _g.delimiters[_i + 1];

        std::string _text = _line.substr(_b, _e - _b);

        if (_opts.trim == trim_policy::trim)
        {
            _text = internal::grid_trim(_text);
        }

        _g.cells.push_back(_text);
    }

    return _g;
}

// scan_grid
//   function: the input's lines, scanned.  Blank lines are skipped when the
// dialect says so; every other line becomes a grid_line, separator or not.
D_NODISCARD inline std::vector<grid_line>
scan_grid(
    parse::parse_state<char>&        _state,
    const parse_grid_options& _opts = parse_grid_options())
{
    std::vector<grid_line> _lines;

    // walk the residual Σ*, a line at a time, advancing the state as we go
    while (!_state.at_end())
    {
        const std::size_t _begin = _state.offset;

        std::string _line;

        while ((!_state.at_end()) && (*_state.current() != '\n'))
        {
            _line.push_back(*_state.current());
            _state.advance(1);
        }

        // consume the newline itself
        if (!_state.at_end())
        {
            _state.advance(1);
        }

        const std::string _trimmed = internal::grid_trim(_line);

        // a blank line carries no row
        if (_trimmed.empty())
        {
            if (_opts.skip_blank)
            {
                continue;
            }
        }

        const grid_line _g = scan_line(_line, _begin, _opts);

        // a line with no delimiters is not part of the grid
        if (_g.delimiters.size() < 2)
        {
            continue;
        }

        _lines.push_back(_g);
    }

    return _lines;
}


// ===========================================================================
// IV.  boundaries
// ===========================================================================

// column_boundaries
//   function: the grid's column boundaries -- the delimiter offsets of its FINEST
// line, the one that resolves the most columns.  Every other line's cells are read
// against these: a cell that covers interior boundaries is a merge of the columns
// they separate.
//
//   The finest line is the right reference because a merge can only ever be
// COARSER than the atomic grid; no line can resolve more columns than the atomic
// table has.  Separator lines are ignored -- a rule resolves nothing.
D_NODISCARD inline std::vector<std::size_t>
column_boundaries(const std::vector<grid_line>& _lines)
{
    std::vector<std::size_t> _best;

    for (std::size_t _i = 0; _i < _lines.size(); ++_i)
    {
        if (_lines[_i].is_separator)
        {
            continue;
        }

        if (_lines[_i].delimiters.size() > _best.size())
        {
            _best = _lines[_i].delimiters;
        }
    }

    return _best;
}

// span_of
//   function: how many columns a cell running from delimiter offset _begin to
// _end covers -- one, plus every INTERIOR boundary it swallows.  This is the
// whole span inference: an atomic cell covers no interior boundary; a cell
// covering k of them merges k+1 columns.
D_NODISCARD inline std::size_t
span_of(
    const std::vector<std::size_t>& _boundaries,
    std::size_t                     _begin,
    std::size_t                     _end)
{
    std::size_t _covered = 0;

    for (std::size_t _i = 0; _i < _boundaries.size(); ++_i)
    {
        // strictly interior to the cell: a boundary the cell has swallowed
        if ((_boundaries[_i] > _begin) && (_boundaries[_i] < _end))
        {
            ++_covered;
        }
    }

    return (_covered + 1);
}

// grid_width
//   function: the atomic column count the boundaries resolve.
D_NODISCARD inline std::size_t
grid_width(const std::vector<std::size_t>& _boundaries)
{
    return (_boundaries.size() > 0) ? (_boundaries.size() - 1) : 0;
}


// ===========================================================================
// V.   parse_table
// ===========================================================================

NS_INTERNAL

    // row_span_ok
    //   function: whether a row covering _covered of _required columns is
    // admissible under _s -- the runtime twin of table_builder's row_width_ok, so
    // the two front ends enforce one rule.
    inline bool
    row_span_ok(
        table_strictness _s,
        std::size_t      _covered,
        std::size_t      _required)
    {
        return ( (_s == table_strictness::exact)    ? (_covered == _required)
               : (_s == table_strictness::truncate) ? (_covered >= _required)
               : (_s == table_strictness::pad)      ? (_covered <= _required)
               :                                      true );
    }

    // fill_row
    //   function: lay one scanned line's cells onto row _r of the model, reading
    // each cell's span off the boundaries and registering a merge for any cell
    // that covers more than one column.  Returns the columns the row covered.
    template<typename _Cell,
             typename _Metadata>
    inline std::size_t
    fill_row(
        table_model_value<_Cell, _Metadata>& _model,
        const grid_line&                     _line,
        const std::vector<std::size_t>&      _boundaries,
        std::size_t                          _r)
    {
        const std::size_t _width = grid_width(_boundaries);

        std::size_t _c = 0;

        for (std::size_t _i = 0; _i < _line.cells.size(); ++_i)
        {
            if (_c >= _width)
            {
                break;
            }

            const std::size_t _span = span_of(_boundaries,
                                              _line.delimiters[_i],
                                              _line.delimiters[_i + 1]);

            // never let a span run past the grid
            const std::size_t _fit =
                ((_c + _span) > _width) ? (_width - _c) : _span;

            // a cell covering more than one column IS a merge; it is written once,
            // at its anchor, and the covered positions defer to it
            if (_fit > 1)
            {
                _model.merge(_r, _c, 1, _fit, _line.cells[_i]);
            }
            else
            {
                _model.set(_r, _c, _line.cells[_i]);
            }

            _c += _fit;
        }

        return _c;
    }

    // header_level_of
    //   function: one scanned line as a header LEVEL -- each cell a labelled,
    // spannable header_cell, its span read off the boundaries exactly as a body
    // cell's is.  This is what makes a grouped header ("coordinate" over x, y, z)
    // survive the round trip.
    inline header_level<std::string>
    header_level_of(
        const grid_line&                _line,
        const std::vector<std::size_t>& _boundaries)
    {
        header_level<std::string> _level;

        const std::size_t _width = grid_width(_boundaries);

        std::size_t _c = 0;

        for (std::size_t _i = 0; _i < _line.cells.size(); ++_i)
        {
            if (_c >= _width)
            {
                break;
            }

            const std::size_t _span = span_of(_boundaries,
                                              _line.delimiters[_i],
                                              _line.delimiters[_i + 1]);

            const std::size_t _fit =
                ((_c + _span) > _width) ? (_width - _c) : _span;

            _level.push_back(header_cell<std::string>(_line.cells[_i], _fit));

            _c += _fit;
        }

        return _level;
    }

NS_END  // internal

// parse_table
//   function: text -> the model.  The PARSE leg: scan the grid, take the finest
// line's delimiters as the column boundaries, read every other line's spans
// against them, and fill a table_model_value -- the same carrier table_builder
// folds a type declaration into.
//
//   The rows above a separator become the column-header stack (outermost first);
// the rows below become the body.  A cell spanning several columns becomes a
// merge in the model's cover, anchored where it starts.
//
// Example:
//   parse::parse_state<char> st(text.data(), text.size());
//   parse::parse_result<table_model_value<>> r = parse_table(st);
//   if (r.ok()) { const auto& m = r.value(); ... }
template<typename _Cell     = std::string,
         typename _Metadata = table_metadata<>>
D_NODISCARD parse::parse_result<table_model_value<_Cell, _Metadata>>
parse_table(
    parse::parse_state<char>&        _state,
    const parse_grid_options& _opts = parse_grid_options())
{
    using model_type  = table_model_value<_Cell, _Metadata>;
    using result_type = parse::parse_result<model_type>;

    const std::size_t _origin = _state.offset;

    const std::vector<grid_line> _lines = scan_grid(_state, _opts);

    // a grid needs at least one delimited line
    if (_lines.empty())
    {
        return result_type::make_error(
            parse::DParseStatusMalformed, _origin,
            "parse_table: no delimited line found -- a table is a grid of rows "
            "bounded by the cell delimiter.");
    }

    const std::vector<std::size_t> _bounds = column_boundaries(_lines);
    const std::size_t              _width  = grid_width(_bounds);

    if (_width == 0)
    {
        return result_type::make_error(
            parse::DParseStatusMalformed, _origin,
            "parse_table: the grid resolves no columns -- a row needs at least "
            "two delimiters to bound one cell.");
    }

    // the separator splits the header block from the body
    std::size_t _sep = _lines.size();

    for (std::size_t _i = 0; _i < _lines.size(); ++_i)
    {
        if (_lines[_i].is_separator)
        {
            _sep = _i;

            break;
        }
    }

    const bool _has_headers = (_sep < _lines.size());

    // the body is everything after the separator, or the whole grid without one
    std::vector<const grid_line*> _body;

    for (std::size_t _i = (_has_headers ? (_sep + 1) : 0);
         _i < _lines.size();
         ++_i)
    {
        // a later rule line is not a row
        if (_lines[_i].is_separator)
        {
            continue;
        }

        _body.push_back(&_lines[_i]);
    }

    model_type _model(_body.size(), _width);

    // 1. the body: every row's spans read off the boundaries
    for (std::size_t _r = 0; _r < _body.size(); ++_r)
    {
        const std::size_t _covered =
            internal::fill_row(_model, *_body[_r], _bounds, _r);

        // the cell_count policy, applied
        if (!internal::row_span_ok(_opts.cell_count, _covered, _width))
        {
            return result_type::make_error(
                parse::DParseStatusMalformed, _body[_r]->line_offset,
                "parse_table: a row does not cover the table's columns.  Its "
                "cells' spans must total the grid's width; relax this with a "
                "cell_count policy (truncate / pad / lenient).");
        }
    }

    // 2. the header block: each line above the separator is a level, outermost
    //    first, its groupings read the same way the body's merges are
    if (_has_headers && (_sep > 0))
    {
        header_stack<std::string> _stack;

        for (std::size_t _i = 0; _i < _sep; ++_i)
        {
            _stack.push_back(internal::header_level_of(*(&_lines[_i]), _bounds));
        }

        _model.metadata().set_column_headers(_stack);
    }

    return result_type::make_ok(_model);
}

// parse_table_text
//   function: parse_table over a std::string -- the convenience face, for when
// there is no residual to thread.
template<typename _Cell     = std::string,
         typename _Metadata = table_metadata<>>
D_NODISCARD parse::parse_result<table_model_value<_Cell, _Metadata>>
parse_table_text(
    const std::string&        _text,
    const parse_grid_options& _opts = parse_grid_options())
{
    parse::parse_state<char> _state(_text.data(), _text.size());

    return parse_table<_Cell, _Metadata>(_state, _opts);
}


// ===========================================================================
// VI.  round_trip
// ===========================================================================
//   The prism laws, as things a caller (or a test) can actually check.  They are
// the reason the two front ends are one DSL rather than two that happen to agree.

// renders_to_same_text
//   function: render . parse = id, at _text -- the law that says the text surface
// loses nothing.  True when _text is canonical (as render writes it); a grid that
// is merely EQUIVALENT (differently padded) parses to the same model but renders
// to the canonical spelling, which is the point of a canonical form.
D_NODISCARD inline bool
renders_to_same_text(
    const std::string&        _text,
    const parse_grid_options& _opts = parse_grid_options())
{
    parse::parse_result<table_model_value<>> _r = parse_table_text<>(_text, _opts);

    if (!_r.ok())
    {
        return false;
    }

    return (render(_r.value()) == _text);
}

// models_equal
//   function: whether two runtime models agree -- same domain, same cover, and
// the same value at every position READ THROUGH the overlay (so two models that
// differ only in the dead storage under a merge still agree, as they should:
// a covered position has no value of its own).
template<typename _Cell,
         typename _Metadata>
D_NODISCARD bool
models_equal(
    const table_model_value<_Cell, _Metadata>& _a,
    const table_model_value<_Cell, _Metadata>& _b)
{
    if ((_a.rows() != _b.rows()) || (_a.cols() != _b.cols()))
    {
        return false;
    }

    if (_a.layout().merge_count() != _b.layout().merge_count())
    {
        return false;
    }

    for (std::size_t _r = 0; _r < _a.rows(); ++_r)
    {
        for (std::size_t _c = 0; _c < _a.cols(); ++_c)
        {
            // the cover must agree position by position ...
            const region_value _oa = _a.layout().owner_of(_r, _c);
            const region_value _ob = _b.layout().owner_of(_r, _c);

            if ( (_oa.row0 != _ob.row0) || (_oa.col0 != _ob.col0) ||
                 (_oa.rows != _ob.rows) || (_oa.cols != _ob.cols) )
            {
                return false;
            }

            // ... and so must the value each position reports
            if (!(_a.at(_r, _c) == _b.at(_r, _c)))
            {
                return false;
            }
        }
    }

    return true;
}

// survives_round_trip
//   function: parse . render = id, at _model -- the law that says a model can be
// written down and read back unchanged.  The direction that matters for the two
// front ends: a model table_builder folds from types renders to text that parses
// back to the same model.
template<typename _Cell,
         typename _Metadata>
D_NODISCARD bool
survives_round_trip(
    const table_model_value<_Cell, _Metadata>& _model,
    const parse_grid_options&                  _opts = parse_grid_options())
{
    const std::string _text = render(_model);

    parse::parse_state<char> _state(_text.data(), _text.size());

    parse::parse_result<table_model_value<_Cell, _Metadata>> _r =
        parse_table<_Cell, _Metadata>(_state, _opts);

    if (!_r.ok())
    {
        return false;
    }

    return models_equal(_model, _r.value());
}


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_TABLE_PARSER_
