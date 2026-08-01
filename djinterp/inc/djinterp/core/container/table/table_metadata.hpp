/******************************************************************************
* djinterp [container]                                      table_metadata.hpp
*
*   The table's metadata, derived from container_metadata.  A table carries the
* same open key-value metadata any container may (title, name, description, date,
* provenance -- inherited wholesale from container_metadata), and, in addition,
* two things a flat collection has no notion of: COLUMN HEADERS and ROW HEADERS.
*
*   HEADERS CONFORM TO THE DIMENSIONS.  Unlike a free key-value pair, a header set
* must line up with the table: the column headers span the columns, the row
* headers span the rows.  This module makes that conformance CHECKABLE without
* dictating how a header set is stored.
*
*   MULTIPLE, POSSIBLY GROUPED, HEADERS.  A table may carry more than one level of
* header on a dimension -- a stack of header rows above the columns, or header
* columns beside the rows -- and a level may GROUP finer ones:
*
*         ... |    coordinate    | ...        (an outer level: one label,
*         --------------------------              spanning three columns)
*         ... |  x  |  y  |  z   | ...        (the finer level: three labels)
*
*   The grouping is a SPAN: an outer label covers several finer positions.
*
*   ABSTRACT BY CONSTRUCTION.  A header set may be stored in any conceivable way,
* so table_metadata takes the column-header and row-header types as OPEN template
* parameters -- it never inspects their internals.  Conformance is a CUSTOMIZATION
* POINT: a header type opts in by providing header_extent(h) (how many table
* positions it spans); table_metadata then verifies that against the table's
* dimension, and simply cannot-and-won't object when a header type does not opt
* in.  A versatile DEFAULT representation (header_stack: a stack of levels, each a
* run of labelled, spannable cells) is provided as a convenience and models the
* grouped example above -- but it is only a default; any type serves.
*
*   PORTABILITY:
*   C++11 baseline, as container_metadata.
*
*
* path:      /inc/djinterp/core/container/table/table_metadata.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.07
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Default header representation   (header_cell / header_level / header_stack)
II.   header_extent                   (the conformance customization point)
III.  table_metadata (class)
IV.   is_table_metadata (detection trait)
*/

#ifndef DJINTERP_TABLE_METADATA_
#define DJINTERP_TABLE_METADATA_ 1

// std
#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../djinterp.hpp"           // NS_*, D_NODISCARD, clean_t, void_t
#include "./container_metadata.hpp"      // container_metadata (base) + metadata_carrier


NS_DJINTERP


// ===========================================================================
// I.   Default header representation
// ===========================================================================
//   A convenient, versatile default a caller MAY use for header sets -- not a
// requirement.  A header is a STACK of LEVELS (outermost first); a level is a run
// of CELLS; a cell is a label plus a SPAN, the number of finer positions it
// covers.  A simple single-row header is one level of unit-span cells; a grouped
// header (the "coordinate" over x, y, z example) is an outer level whose cell
// spans the finer level beneath it.  The label type is open.

// header_cell
//   struct: one header label and the number of table positions it spans (1 for
// an ungrouped cell; more for a group covering several finer positions).
template<typename _Label>
struct header_cell
{
    _Label      label;
    std::size_t span;

    header_cell()
        : label(),
          span(1)
    {}

    explicit header_cell(_Label _label, std::size_t _span = 1)
        : label(static_cast<_Label&&>(_label)),
          span(_span)
    {}
};

// header_level
//   type: one level of a header -- a run of cells covering the dimension once.
template<typename _Label>
using header_level = std::vector<header_cell<_Label>>;

// header_stack
//   type: a stack of header levels, outermost first.  Its size is the NUMBER of
// headers on the dimension; each level covers the dimension (their spans sum to
// the extent).  This is the default column-/row-header type of table_metadata.
template<typename _Label>
using header_stack = std::vector<header_level<_Label>>;

// make_header_level
//   function: a single level of unit-span cells from a list of labels -- the
// common simple (ungrouped) header.
template<typename _Label>
D_NODISCARD header_level<_Label>
make_header_level(std::initializer_list<_Label> _labels)
{
    header_level<_Label> _level;
    _level.reserve(_labels.size());

    for (const _Label& _l : _labels)
    {
        _level.push_back(header_cell<_Label>(_l, 1));
    }

    return _level;
}


// ===========================================================================
// II.  header_extent (the conformance customization point)
// ===========================================================================
//   header_extent(h) reports how many table positions a header set h spans, so
// table_metadata can check it against the table's dimension.  A header type opts
// into conformance checking by making header_extent(h) well-formed -- either
// through one of the default overloads below (for the default representation) or
// through its own overload found by ADL.  A type that provides none is treated as
// opaque: its conformance simply is not checked.

// header_extent (a level)
//   the positions a single level covers: the sum of its cells' spans.
template<typename _Label>
D_NODISCARD std::size_t
header_extent(const header_level<_Label>& _level)
{
    std::size_t _n = 0;

    for (const header_cell<_Label>& _cell : _level)
    {
        _n += _cell.span;
    }

    return _n;
}

// header_extent (a stack)
//   the positions the stack covers: the extent of its finest (innermost) level,
// which addresses the dimension one-to-one; an empty stack spans nothing.
template<typename _Label>
D_NODISCARD std::size_t
header_extent(const header_stack<_Label>& _stack)
{
    if (_stack.empty())
    {
        return 0;
    }

    return header_extent(_stack.back());
}

NS_INTERNAL

    // has_header_extent
    //   trait: whether header_extent(h) is well-formed for _Headers -- i.e.
    // whether the header type opts into conformance checking.
    template<typename _Headers,
             typename = void>
    struct has_header_extent : std::false_type
    {};

    template<typename _Headers>
    struct has_header_extent<_Headers,
        void_t<decltype(header_extent(std::declval<const _Headers&>()))>>
        : std::true_type
    {};

NS_END  // internal


// ===========================================================================
// III. table_metadata (class)
// ===========================================================================

// table_metadata
//   class: a table's metadata -- container_metadata's open key-value entries plus
// a column-header set and a row-header set of OPEN types.  It exposes, sets, and
// checks the conformance of the headers without inspecting their internals.
template<typename _Key           = std::string,
         typename _Value         = std::string,
         typename _ColumnHeaders = header_stack<std::string>,
         typename _RowHeaders    = header_stack<std::string>,
         typename _Store         = std::vector<std::pair<_Key, _Value>>>
class table_metadata
    : public container_metadata<_Key, _Value, _Store>
{
public:
    using base_type           = container_metadata<_Key, _Value, _Store>;
    using column_headers_type = _ColumnHeaders;
    using row_headers_type    = _RowHeaders;

    // --- construction ---

    table_metadata()
        : base_type(),
          m_column_headers(),
          m_row_headers(),
          m_has_column_headers(false),
          m_has_row_headers(false)
    {}

    // --- column headers ---

    D_NODISCARD bool has_column_headers() const noexcept
    {
        return m_has_column_headers;
    }

    D_NODISCARD const _ColumnHeaders& column_headers() const noexcept
    {
        return m_column_headers;
    }

    D_NODISCARD _ColumnHeaders& column_headers() noexcept
    {
        return m_column_headers;
    }

    // set_column_headers -- adopt a column-header set (of any type).
    void set_column_headers(_ColumnHeaders _headers)
    {
        m_column_headers     = static_cast<_ColumnHeaders&&>(_headers);
        m_has_column_headers = true;

        return;
    }

    void clear_column_headers()
    {
        m_column_headers     = _ColumnHeaders();
        m_has_column_headers = false;

        return;
    }

    // --- row headers ---

    D_NODISCARD bool has_row_headers() const noexcept
    {
        return m_has_row_headers;
    }

    D_NODISCARD const _RowHeaders& row_headers() const noexcept
    {
        return m_row_headers;
    }

    D_NODISCARD _RowHeaders& row_headers() noexcept
    {
        return m_row_headers;
    }

    // set_row_headers -- adopt a row-header set (of any type).
    void set_row_headers(_RowHeaders _headers)
    {
        m_row_headers     = static_cast<_RowHeaders&&>(_headers);
        m_has_row_headers = true;

        return;
    }

    void clear_row_headers()
    {
        m_row_headers     = _RowHeaders();
        m_has_row_headers = false;

        return;
    }

    // --- dimension conformance ---

    // conforms_columns
    //   whether the column headers line up with _column_count columns.  Vacuously
    // true when no column headers are set, or when the header type is opaque (it
    // does not opt into header_extent); otherwise the spanned extent must equal
    // the column count.
    D_NODISCARD bool conforms_columns(std::size_t _column_count) const
    {
        return conforms_dimension<_ColumnHeaders>(
            m_column_headers, m_has_column_headers, _column_count);
    }

    // conforms_rows
    //   the row-dimension counterpart of conforms_columns.
    D_NODISCARD bool conforms_rows(std::size_t _row_count) const
    {
        return conforms_dimension<_RowHeaders>(
            m_row_headers, m_has_row_headers, _row_count);
    }

    // conforms_to
    //   whether both header sets conform to a table's dimensions, read through its
    // column_count() / row_count() accessors.  Decoupled from any concrete table
    // type: any type exposing those two accessors works.
    template<typename _Table>
    D_NODISCARD bool conforms_to(const _Table& _table) const
    {
        return ( conforms_columns(
                     static_cast<std::size_t>(_table.column_count())) &&
                 conforms_rows(
                     static_cast<std::size_t>(_table.row_count())) );
    }

private:
    // conforms_dimension -- dispatch on whether the header type opts into the
    // header_extent customization point.
    template<typename _Headers>
    static bool conforms_dimension(
        const _Headers& _headers,
        bool            _present,
        std::size_t     _dimension)
    {
        // an unset header set trivially conforms
        if (!_present)
        {
            return true;
        }

        return conforms_extent(
            _headers, _dimension, internal::has_header_extent<_Headers>{});
    }

    // conforms_extent (checkable) -- the spanned extent must equal the dimension.
    template<typename _Headers>
    static bool conforms_extent(
        const _Headers& _headers,
        std::size_t     _dimension,
        std::true_type  /*has_extent*/)
    {
        return (header_extent(_headers) == _dimension);
    }

    // conforms_extent (opaque) -- no extent to read, so conformance is not
    // checked here; it is the header type's own contract.
    template<typename _Headers>
    static bool conforms_extent(
        const _Headers& /*_headers*/,
        std::size_t     /*_dimension*/,
        std::false_type /*no_extent*/)
    {
        return true;
    }

    _ColumnHeaders m_column_headers;
    _RowHeaders    m_row_headers;
    bool           m_has_column_headers;
    bool           m_has_row_headers;
};


// ===========================================================================
// IV.  is_table_metadata (detection trait)
// ===========================================================================

// is_table_metadata
//   trait: true when _Type (after stripping cv/ref) is a specialization of
// table_metadata.
NS_INTERNAL

    template<typename _Type>
    struct is_table_metadata_impl : std::false_type
    {};

    template<typename _K,
             typename _V,
             typename _C,
             typename _R,
             typename _S>
    struct is_table_metadata_impl<table_metadata<_K, _V, _C, _R, _S>>
        : std::true_type
    {};

NS_END  // internal

template<typename _Type>
struct is_table_metadata
    : internal::is_table_metadata_impl<clean_t<_Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _Type>
inline constexpr bool is_table_metadata_v =
    is_table_metadata<_Type>::value;
#endif


NS_END  // djinterp


#endif  // DJINTERP_TABLE_METADATA_
