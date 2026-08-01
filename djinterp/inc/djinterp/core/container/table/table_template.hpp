/******************************************************************************
* djinterp [container]                                       table_template.hpp
*
*   The TEMPLATING layer over a table model -- what turns a table written once
* into a table that can be filled many times.  Two independent features, both
* model -> model, so they compose with the parse and compose legs rather than
* complicating either:
*
*     1. CELL INTERPOLATION.  A cell's text may carry {key} placeholders; a
*        resolver says what each becomes.  This is not re-implemented here -- it
*        IS interpolate.hpp (brace_scanner / resolver / sink), applied per cell.
*        A MISS LEAVES THE PLACEHOLDER UNTOUCHED, which is what makes PARTIAL
*        interpolation well-defined there and here alike: a table may be filled
*        in passes, each resolver supplying what it knows.
*
*     2. MULTI-CELL PLACEHOLDERS.  A cell whose text is `name...` stands for a
*        WHOLE RUN of cells, not one:
*
*            | headers... |||||        <- one placeholder, five cells
*
*        Expanding it replaces the spanning cell with the cells its binding
*        names.  The interesting case is the one the sketch asks about: what if
*        the binding does not fit the span one-to-one?  That is not a new
*        question -- it is the placeholder_fit policy (table_options.hpp),
*        already graded:
*
*            exact     a mismatch is an error       (5 cells need 5 values)
*            truncate  a surplus is dropped         (7 values fill 5 cells)
*            pad       a shortfall is filled        (4 values fill 5, last empty)
*            lenient   either is accepted
*
*   BOTH ARE MODEL -> MODEL.  Neither knows whether the model came from
* table_builder or table_parser, which is the point of having one carrier: a
* template declared in types and a template read from text expand identically.
*
*   ITERATION.  The sketch's `#` sigil marks a cell as iterated.  Its detection
* is here (iteration_of / is_iterated); what an iterated cell EXPANDS to is a
* binding question, so it routes through the same multi-cell expansion -- an
* iterated cell is a run whose values a resolver names.
*
*   PORTABILITY:
*   C++17 (interpolate.hpp is C++17 -- std::string_view backs its piece views).
*
*
* path:      /inc/djinterp/core/container/table/table_template.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.15
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    template_syntax             (the markers: ellipsis, iteration sigil)
II.   placeholder detection       (is_multi_placeholder / multi_placeholder_key
                                   / is_iterated / iteration_of)
III.  multi_bindings              (key -> a run of values)
IV.   expand_multi_placeholders   (the run expansion; placeholder_fit applied)
V.    interpolate_cells           ({key} per cell, via interpolate.hpp)
*/

#ifndef DJINTERP_CONTAINER_TABLE_TEMPLATE_
#define DJINTERP_CONTAINER_TABLE_TEMPLATE_ 1

// std
#include <cstddef>
#include <string>
#include <utility>
#include <vector>
// djinterp
#include "../../djinterp.hpp"                  // NS_*, D_CONSTEXPR, D_NODISCARD
#include "./table_model.hpp"                    // table_model_value
#include "./table_options.hpp"                  // table_strictness (placeholder_fit)
#include "../../../config/core/container/table/cfg_table.h"   // the markers


#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <string_view>
// djinterp
#include "../../../parse/parse.hpp"             // parse_result / parse_error
#include "../../functional/interpolate.hpp"      // brace_scanner, interp_string_sink,
                                                 // interpolate_into (flat in djinterp)


NS_DJINTERP


// ===========================================================================
// I.   template_syntax
// ===========================================================================

// template_syntax
//   struct: the markers the text surface spells a template with.  Kept as data
// rather than baked in, so a dialect that collides with the defaults (a table of
// C++ ellipses, say) can move them.
struct template_syntax
{
    std::string ellipsis;         // marks a MULTI-cell placeholder: "headers..."
    char        iteration_sigil;  // marks an ITERATED cell: "bar#"

    //   Read from cfg_table.h so a dialect that collides with the defaults moves
    // them in ONE place rather than in each module that spells them.
    template_syntax()
        : ellipsis(D_INTERNAL_TABLE_ELLIPSIS),
          iteration_sigil(D_INTERNAL_TABLE_ITERATION_SIGIL)
    {}
};


// ===========================================================================
// II.  placeholder detection
// ===========================================================================

// is_multi_placeholder
//   function: whether _text names a RUN of cells rather than one -- it ends with
// the ellipsis.  `headers...` is the run; `headers` is an ordinary cell.
D_NODISCARD inline bool
is_multi_placeholder(
    const std::string&     _text,
    const template_syntax& _syntax = template_syntax())
{
    const std::size_t _n = _syntax.ellipsis.size();

    // a bare ellipsis names no key, so the text must be strictly longer
    if (_text.size() <= _n)
    {
        return false;
    }

    return (_text.compare(_text.size() - _n, _n, _syntax.ellipsis) == 0);
}

// multi_placeholder_key
//   function: the key a run placeholder names -- its text without the ellipsis.
D_NODISCARD inline std::string
multi_placeholder_key(
    const std::string&     _text,
    const template_syntax& _syntax = template_syntax())
{
    if (!is_multi_placeholder(_text, _syntax))
    {
        return _text;
    }

    return _text.substr(0, _text.size() - _syntax.ellipsis.size());
}

// is_iterated
//   function: whether _text carries the iteration sigil, at either end -- the
// sketch writes both `bar#` and `#gamma`, so neither position is privileged.
D_NODISCARD inline bool
is_iterated(
    const std::string&     _text,
    const template_syntax& _syntax = template_syntax())
{
    if (_text.size() < 2)
    {
        return false;
    }

    return ( (_text[_text.size() - 1] == _syntax.iteration_sigil) ||
             (_text[0] == _syntax.iteration_sigil) );
}

// iteration_of
//   function: the base name an iterated cell carries -- its text without the
// sigil.  `bar#` and `#bar` both name bar.
D_NODISCARD inline std::string
iteration_of(
    const std::string&     _text,
    const template_syntax& _syntax = template_syntax())
{
    if (!is_iterated(_text, _syntax))
    {
        return _text;
    }

    // strip whichever end carries the sigil
    if (_text[_text.size() - 1] == _syntax.iteration_sigil)
    {
        return _text.substr(0, _text.size() - 1);
    }

    return _text.substr(1);
}


// ===========================================================================
// III. multi_bindings
// ===========================================================================

// multi_bindings
//   class: what a run placeholder resolves to -- a key bound to a RUN of values
// (as opposed to interpolate.hpp's resolvers, which bind a key to ONE value).
// The distinction is the whole reason this layer exists: `{name}` fills a cell,
// `name...` fills a span.
class multi_bindings
{
public:
    using run_type   = std::vector<std::string>;
    using entry_type = std::pair<std::string, run_type>;
    using store_type = std::vector<entry_type>;

    multi_bindings()
        : m_entries()
    {}

    // set -- bind _key to _run, overwriting any existing binding.
    void set(
        const std::string& _key,
        run_type           _run)
    {
        for (entry_type& _e : m_entries)
        {
            if (_e.first == _key)
            {
                _e.second = static_cast<run_type&&>(_run);

                return;
            }
        }

        m_entries.push_back(entry_type(_key, static_cast<run_type&&>(_run)));

        return;
    }

    // find -- the run bound to _key, or null when unbound.  A miss is not an
    // error: an unresolved placeholder is left standing, exactly as interpolate's
    // resolvers leave an unresolved {key}, so a table may be filled in passes.
    D_NODISCARD const run_type* find(const std::string& _key) const
    {
        for (const entry_type& _e : m_entries)
        {
            if (_e.first == _key)
            {
                return &_e.second;
            }
        }

        return nullptr;
    }

    D_NODISCARD bool contains(const std::string& _key) const
    {
        return (find(_key) != nullptr);
    }

    D_NODISCARD std::size_t size() const
    {
        return m_entries.size();
    }

private:
    store_type m_entries;
};


// ===========================================================================
// IV.  expand_multi_placeholders
// ===========================================================================

NS_INTERNAL

    // run_fits
    //   function: whether a run of _have values may fill a span of _want cells
    // under _s.  The same grading, in the same direction, as every other count
    // check in the DSL (table_builder's row_width_ok, table_parser's row_span_ok)
    // -- one rule, three call sites.
    inline bool
    run_fits(
        table_strictness _s,
        std::size_t      _have,
        std::size_t      _want)
    {
        return ( (_s == table_strictness::exact)    ? (_have == _want)
               : (_s == table_strictness::truncate) ? (_have >= _want)
               : (_s == table_strictness::pad)      ? (_have <= _want)
               :                                      true );
    }

    // expand_header_stack (a readable stack)
    //   function: expand the run placeholders in a header stack.  This is the
    // sketch's PRIMARY case -- `| headers... |||||` is a HEADER row, not a body
    // row -- so a run in a header level must resolve exactly as one in the body
    // does.  A spanning cell becomes the span's worth of unit cells, which leaves
    // the level's EXTENT unchanged and so keeps table_metadata's own conformance
    // check (header_extent == the column count) true across the expansion.
    template<typename _Stack>
    inline bool
    expand_header_stack(
        _Stack&                _stack,
        const multi_bindings&  _bindings,
        table_strictness       _fit,
        const template_syntax& _syntax,
        std::string&           _key_out,
        std::true_type)
    {
        for (std::size_t _l = 0; _l < _stack.size(); ++_l)
        {
            typename _Stack::value_type _level;

            for (std::size_t _i = 0; _i < _stack[_l].size(); ++_i)
            {
                const std::string& _label = _stack[_l][_i].label;

                const std::size_t _span =
                    (_stack[_l][_i].span > 0) ? _stack[_l][_i].span : 1;

                const bool _is_run = is_multi_placeholder(_label, _syntax);

                const multi_bindings::run_type* _run =
                    _is_run
                        ? _bindings.find(multi_placeholder_key(_label, _syntax))
                        : nullptr;

                // a miss (or a plain label) is carried across untouched
                if (_run == nullptr)
                {
                    _level.push_back(_stack[_l][_i]);

                    continue;
                }

                if (!run_fits(_fit, _run->size(), _span))
                {
                    _key_out = multi_placeholder_key(_label, _syntax);

                    return false;
                }

                // the run resolves into the level: one unit cell per column of
                // the span, padded with empties where the run runs out
                for (std::size_t _j = 0; _j < _span; ++_j)
                {
                    const std::string _value =
                        (_j < _run->size()) ? (*_run)[_j] : std::string();

                    _level.push_back(
                        header_cell<std::string>(_value, 1));
                }
            }

            _stack[_l] = _level;
        }

        return true;
    }

    // expand_header_stack (an opaque stack)
    //   function: a header representation this layer cannot read is left alone --
    // the same stance table_metadata takes with header_extent, and render with
    // is_header_stack.
    template<typename _Stack>
    inline bool
    expand_header_stack(
        _Stack&                /*_stack*/,
        const multi_bindings&  /*_bindings*/,
        table_strictness       /*_fit*/,
        const template_syntax& /*_syntax*/,
        std::string&           /*_key_out*/,
        std::false_type)
    {
        return true;
    }

NS_END  // internal

// expand_multi_placeholders
//   function: replace every run placeholder with the cells its binding names.
// A cell reading `headers...` and spanning five columns becomes those five
// cells, filled from the run bound to `headers`; the merge that carried it is
// gone, because the run resolves INTO the atomic grid.
//
//   The fit is the placeholder_fit policy, applied: a run longer than the span is
// truncated, a shorter one padded, an exact match demanded -- or any of it
// tolerated -- exactly as the option grades name.  A placeholder whose key is
// unbound is LEFT STANDING (and keeps its span), so a template may be expanded in
// passes, each binding set supplying what it knows.
//
// Example (the sketch's own question):
//   | headers... |||||          five cells, one placeholder
//   bindings: headers -> [a, b, c, d]     (four values, five cells)
//     exact     -> error
//     pad       -> | a | b | c | d |   |
//     truncate  -> error (a shortfall is not a surplus)
//     lenient   -> | a | b | c | d |   |
template<typename _Cell,
         typename _Metadata>
D_NODISCARD parse::parse_result<table_model_value<_Cell, _Metadata>>
expand_multi_placeholders(
    const table_model_value<_Cell, _Metadata>& _model,
    const multi_bindings&                      _bindings,
    table_strictness                           _fit    = default_placeholder_fit,
    const template_syntax&                     _syntax = template_syntax())
{
    using model_type  = table_model_value<_Cell, _Metadata>;
    using result_type = parse::parse_result<model_type>;

    model_type _out(_model.rows(), _model.cols());

    // the metadata rides across, and its HEADERS are expanded too: the sketch's
    // `| headers... |||||` is a header row, so a run must resolve there exactly as
    // it does in the body.  The open key-value entries are untouched.
    _out.metadata() = _model.metadata();

    if (_out.metadata().has_column_headers())
    {
        using headers_t = typename _Metadata::column_headers_type;

        std::string _bad_key;

        const bool _ok = internal::expand_header_stack(
            _out.metadata().column_headers(), _bindings, _fit, _syntax, _bad_key,
            is_header_stack<headers_t>{});

        if (!_ok)
        {
            return result_type::make_error(
                parse::DParseStatusMalformed, 0,
                "expand_multi_placeholders: the run bound to '" + _bad_key +
                "' does not fit the header cells it must fill.  Relax this with "
                "a placeholder_fit policy -- truncate (drop a surplus), pad "
                "(fill a shortfall), or lenient (either).");
        }
    }

    for (std::size_t _r = 0; _r < _model.rows(); ++_r)
    {
        std::size_t _c = 0;

        while (_c < _model.cols())
        {
            const region_value _owner = _model.layout().owner_of(_r, _c);

            const std::size_t _span = _owner.cols;

            // a cell whose anchor lies in an earlier row is covered from above;
            // it is not this row's to write
            if (_owner.anchor_row() != _r)
            {
                _c += _span;

                continue;
            }

            const _Cell& _text = _model.raw_at(_owner.anchor_row(),
                                               _owner.anchor_col());

            const bool _is_run = is_multi_placeholder(_text, _syntax);

            const std::string _key =
                _is_run ? multi_placeholder_key(_text, _syntax) : std::string();

            const multi_bindings::run_type* _run =
                _is_run ? _bindings.find(_key) : nullptr;

            // an unbound (or non-) placeholder is carried across verbatim, span
            // and all -- a miss leaves the cell untouched
            if (_run == nullptr)
            {
                if (_span > 1)
                {
                    _out.merge(_r, _c, _owner.rows, _span, _text);
                }
                else
                {
                    _out.set(_r, _c, _text);
                }

                _c += _span;

                continue;
            }

            // the fit, graded
            if (!internal::run_fits(_fit, _run->size(), _span))
            {
                return result_type::make_error(
                    parse::DParseStatusMalformed, 0,
                    "expand_multi_placeholders: the run bound to '" + _key +
                    "' does not fit the cells it must fill.  Relax this with a "
                    "placeholder_fit policy -- truncate (drop a surplus), pad "
                    "(fill a shortfall), or lenient (either).");
            }

            // the run resolves INTO the grid: one atomic cell per column of the
            // span, padded with empties where the run runs out
            for (std::size_t _i = 0; _i < _span; ++_i)
            {
                const _Cell _value =
                    (_i < _run->size()) ? (*_run)[_i] : _Cell();

                _out.set(_r, _c + _i, _value);
            }

            _c += _span;
        }
    }

    return result_type::make_ok(_out);
}


// ===========================================================================
// V.   interpolate_cells
// ===========================================================================

// interpolate_cells
//   function: resolve the {key} placeholders in every cell.  Not a
// re-implementation: each cell's text is handed to interpolate.hpp's engine
// (brace_scanner over the cell, the caller's resolver, a string sink), so the
// table surface inherits that module's whole vocabulary -- inline bindings, a
// callable lookup, chained frames, predicate-gated frames, recursive expansion --
// for free, and behaves identically to every other interpolated text in the
// framework.  In particular a MISS LEAVES THE PLACEHOLDER STANDING, so a table
// may be filled in passes.
//
//   The cover is preserved: a merged cell's one value is interpolated once, at
// its anchor, which is the only place it lives.
//
// Example:
//   auto r = interpolate_cells(m, bindings<char>({{"who", "world"}}));
//   // a cell reading "hello {who}" now reads "hello world"
template<typename _Cell,
         typename _Metadata,
         typename _Resolver>
D_NODISCARD table_model_value<_Cell, _Metadata>
interpolate_cells(
    const table_model_value<_Cell, _Metadata>& _model,
    const _Resolver&                           _resolver)
{
    table_model_value<_Cell, _Metadata> _out(_model.rows(), _model.cols());

    _out.metadata() = _model.metadata();

    // carry the cover across: interpolation rewrites text, never structure
    const runtime_layout::merge_store& _merges = _model.layout().merges();

    for (std::size_t _i = 0; _i < _merges.size(); ++_i)
    {
        const region_value& _m = _merges[_i];

        _out.layout().add_merge(_m.row0, _m.col0, _m.rows, _m.cols);
    }

    // every atomic cell's text through the engine
    for (std::size_t _r = 0; _r < _model.rows(); ++_r)
    {
        for (std::size_t _c = 0; _c < _model.cols(); ++_c)
        {
            const _Cell& _text = _model.raw_at(_r, _c);

            std::string _rendered;

            interp_string_sink<char> _sink(_rendered);

            interpolate_into(
                _sink,
                brace_scanner<char>(std::string_view(_text)),
                _resolver);

            _out.set(_r, _c, _rendered);
        }
    }

    return _out;
}


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


#endif  // DJINTERP_CONTAINER_TABLE_TEMPLATE_
