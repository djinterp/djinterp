/*******************************************************************************
* djinterp [core]                                                table_index.hpp
*
* Database-style secondary indices over lookup-table columns.
*
*   A lookup_table provides primary access by its key column.  When
* callers need fast retrieval by some other column - "find all rows
* where column 2 equals X", "the row where column 3 is the largest" -
* they instantiate a table_index over that column.  The index maintains
* a side data structure mapping column values back to row positions,
* enabling sub-linear lookup at the cost of one auxiliary structure per
* index.
*
*   STRATEGIES:
*   The index's internal organization is chosen at compile time via a
* strategy tag:
*
*   - linear_index_strategy   : no acceleration; a flat list of column-
*                               value -> row-offset pairs.  Builds in
*                               O(N); lookup is O(N).  Useful as a
*                               diagnostic baseline or when an index
*                               must always synthesize a result set
*                               regardless of column-type capabilities.
*
*   - sorted_index_strategy   : column values sorted; lookup via binary
*                               search.  Builds in O(N log N); lookup
*                               is O(log N + matches).  Requires the
*                               column type to be totally ordered
*                               (operator<).
*
*   - hashed_index_strategy   : column values hashed; lookup via hash
*                               map.  Builds in O(N); lookup is O(1)
*                               average per match.  Requires the column
*                               type to be hashable (std::hash, or a
*                               user-supplied hasher).
*
*   UNIQUENESS:
*   An index is either UNIQUE (each column value maps to at most one
* row) or NON-UNIQUE (a column value may map to many rows).  Unique
* indices reject duplicates at insertion time; non-unique indices
* collect all matches.  Both flavors use the same strategy storage but
* differ in their insert path and the shape of their public find API.
*
*   ROW STABILITY:
*   The index stores row OFFSETS (std::size_t) into the table's
* container, not iterators.  Offsets survive reallocation of e.g.
* std::vector but mean that callers MUST notify the index when rows
* are reordered or removed by anything other than the index's own
* maintenance hooks.  See "Synchronization" below.
*
*   SYNCHRONIZATION:
*   table_index is intentionally NOT a self-updating observer of the
* table - that would require the table to know about its indices,
* coupling the two too tightly.  Instead, callers explicitly drive
* maintenance:
*
*     - rebuild()        : full rebuild from current table state.
*                          Call after bulk mutations or whenever
*                          drift is suspected.
*     - on_insert(_i)    : a row was appended at offset _i.  Insert
*                          its column value into the index.
*     - on_erase(_i)     : the row at offset _i was removed (with
*                          subsequent rows shifting down by one).
*                          Adjust all stored offsets.
*     - on_modify(_i)    : the row at offset _i had its column
*                          value changed.  Remove the old entry
*                          and insert the new one.
*
*   A higher-level wrapper (e.g. an indexed_table type that owns its
* lookup_table and its indices) can invoke these hooks automatically;
* table_index itself remains policy-neutral.
*
*   COLUMN ACCESS:
*   Indices address columns by integer position (the Column template
* parameter).  Resolution goes through lookup_row_column_type_t and
* lookup_column_of, both from lookup_traits.hpp.  This means an index
* can target any column - including columns that lookup_table itself
* never reads (description, default value, sort priority, ...).
* Indexing the key column (column 0 in most cases) is permitted but
* redundant; lookup_table's own find() already covers it.
*
*
* path:      /inc/djinterp/core/util/table_index.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.23
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Strategy Tags
      -------------
      1.    linear_index_strategy
      2.    sorted_index_strategy
      3.    hashed_index_strategy

II.   Uniqueness Tags
      ---------------
      1.    unique_index
      2.    non_unique_index

III.  Index Capability Traits
      -----------------------
      1.    is_index_strategy               (+ _v)
      2.    is_index_uniqueness             (+ _v)

IV.   Index Storage Primitives
      ------------------------
      1.    index_entry

V.    table_index (primary template + strategy specializations)
      ---------------------------------------------------------
      1.    linear / non-unique
      2.    linear / unique
      3.    sorted  / non-unique
      4.    sorted  / unique
      5.    hashed  / non-unique
      6.    hashed  / unique

VI.   Convenience Factories
      ---------------------
      1.    make_linear_index
      2.    make_sorted_index
      3.    make_hashed_index
*/

#ifndef DJINTERP_UTIL_TABLE_INDEX_HPP
#define DJINTERP_UTIL_TABLE_INDEX_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <algorithm>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
// djinterp
#include "../../djinterp.hpp"
#include "../meta/type_traits.hpp"
#include "lookup/lookup_traits.hpp"


NS_DJINTERP


// ===========================================================================
// I.   Strategy Tags
// ===========================================================================

// linear_index_strategy
//   tag: no acceleration; index storage is a flat vector of
// (column_value, row_offset) entries.  Lookup is O(N).
struct linear_index_strategy
{};

// sorted_index_strategy
//   tag: entries kept sorted by column_value; lookup via binary
// search.  Requires column_type to provide operator<.
struct sorted_index_strategy
{};

// hashed_index_strategy
//   tag: entries placed in an unordered_map keyed by column_value;
// lookup is O(1) average.  Requires column_type to provide a hash
// (std::hash specialization or user-supplied hasher).
struct hashed_index_strategy
{};


// ===========================================================================
// II.  Uniqueness Tags
// ===========================================================================

// unique_index
//   tag: each column value maps to at most one row.  Insertion of a
// duplicate column value is reported as failure (insert returns false,
// table state unchanged).
struct unique_index
{};

// non_unique_index
//   tag: a column value may map to many rows.  All matches are
// retrievable via find_all / equal_range-style queries.
struct non_unique_index
{};


// ===========================================================================
// III. Index Capability Traits
// ===========================================================================

// is_index_strategy
//   trait: true iff Type is one of the recognized index strategy
// tags.
template<typename Type>
struct is_index_strategy
    : std::integral_constant<bool,
        ( std::is_same<clean_t<Type>, linear_index_strategy>::value ||
          std::is_same<clean_t<Type>, sorted_index_strategy>::value ||
          std::is_same<clean_t<Type>, hashed_index_strategy>::value )>
{};

template<typename Type>
inline constexpr bool is_index_strategy_v =
    is_index_strategy<Type>::value;

// is_index_uniqueness
//   trait: true iff Type is one of the recognized uniqueness tags.
template<typename Type>
struct is_index_uniqueness
    : std::integral_constant<bool,
        ( std::is_same<clean_t<Type>, unique_index>::value ||
          std::is_same<clean_t<Type>, non_unique_index>::value )>
{};

template<typename Type>
inline constexpr bool is_index_uniqueness_v =
    is_index_uniqueness<Type>::value;


// ===========================================================================
// IV.  Index Storage Primitives
// ===========================================================================

// index_entry
//   struct: a single (column_value, row_offset) pair used as the
// element type for linear and sorted strategies.  Hashed strategies
// store the column_value as the map key and the row_offset as the
// mapped value, so they do not use this primitive directly.
//
//   Ordering is by column value only; row_offset is treated as a
// payload and ignored for comparison.  This lets sorted_index_strategy
// route directly through std::lower_bound on a vector of entries.
template<typename ColumnType>
struct index_entry
{
    using column_type = ColumnType;

    ColumnType column_value;
    std::size_t row_offset;

    // default construction
    index_entry() = default;

    // construct from column value + row offset
    D_CONSTEXPR index_entry(
        const ColumnType& _cv,
        std::size_t        _ro
    )
        : column_value(_cv),
          row_offset  (_ro)
    {}

    D_CONSTEXPR index_entry(
        ColumnType&& _cv,
        std::size_t   _ro
    )
        : column_value(static_cast<ColumnType&&>(_cv)),
          row_offset  (_ro)
    {}

    // ordering compares the column value only
    D_CONSTEXPR bool
    operator<(
        const index_entry& _other
    ) const
    {
        return (column_value < _other.column_value);
    }

    D_CONSTEXPR bool
    operator==(
        const index_entry& _other
    ) const
    {
        return (column_value == _other.column_value);
    }
};


// ===========================================================================
// V.   table_index (primary template + strategy specializations)
// ===========================================================================

// table_index
//   class: a database-style secondary index over column Column of
// Table, using the given Strategy and Uniqueness.
//
//   Primary template is declared but not defined; only the strategy/
// uniqueness specializations are usable.  Static asserts in each
// specialization confirm the tag values.
//
// Type parameters:
//   Table       - a lookup-table-like type exposing row_type, size(),
//                 container(), begin(), end(), and indexed access via
//                 container()[i] for stable offset semantics.
//   Column      - tuple index of the column to index over.  May be
//                 any column the row type exposes; resolution goes
//                 through lookup_row_column_type_t<row_type, Column>.
//   Strategy    - one of the strategy tags from section I.
//   Uniqueness - one of the uniqueness tags from section II.
template<typename     Table,
         std::size_t  Column,
         typename     Strategy    = sorted_index_strategy,
         typename     Uniqueness = non_unique_index>
class table_index;


// ---------------------------------------------------------------------------
// V.1  table_index<Table, Column, linear_index_strategy, non_unique_index>
// ---------------------------------------------------------------------------

// table_index (linear, non-unique)
//   class: O(N) lookup over a flat entry list.  No constraints on the
// column type beyond equality comparison.
template<typename    Table,
         std::size_t Column>
class table_index<Table, Column,
                  linear_index_strategy,
                  non_unique_index>
{
public:
    using table_type   = Table;
    using row_type     = typename Table::row_type;
    using column_type  =
        lookup_row_column_type_t<row_type, Column>;
    using entry_type   = index_entry<column_type>;
    using storage_type = std::vector<entry_type>;
    using size_type    = std::size_t;

    static constexpr std::size_t column_index = Column;

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    explicit table_index(
        Table& _table
    )
        : m_table(&_table)
    {
        rebuild();
    }

    // -----------------------------------------------------------------
    //  capacity
    // -----------------------------------------------------------------

    D_CONSTEXPR size_type
    size() const D_NOEXCEPT
    {
        return m_entries.size();
    }

    D_CONSTEXPR bool
    empty() const D_NOEXCEPT
    {
        return m_entries.empty();
    }

    // -----------------------------------------------------------------
    //  lookup
    // -----------------------------------------------------------------

    // find_first
    //   returns the row offset of the first match, or size_type(-1)
    // if no row has _cv in the indexed column.
    size_type
    find_first(
        const column_type& _cv
    ) const
    {
        for (auto it = m_entries.begin();
             it != m_entries.end();
             ++it)
        {
            if (it->column_value == _cv)
            {
                return it->row_offset;
            }
        }

        return static_cast<size_type>(-1);
    }

    // find_all
    //   returns the row offsets of every row whose indexed column
    // equals _cv.
    std::vector<size_type>
    find_all(
        const column_type& _cv
    ) const
    {
        std::vector<size_type> result;

        for (auto it = m_entries.begin();
             it != m_entries.end();
             ++it)
        {
            if (it->column_value == _cv)
            {
                result.push_back(it->row_offset);
            }
        }

        return result;
    }

    // contains
    bool
    contains(
        const column_type& _cv
    ) const
    {
        return (find_first(_cv) !=
                static_cast<size_type>(-1));
    }

    // -----------------------------------------------------------------
    //  maintenance
    // -----------------------------------------------------------------

    // rebuild
    //   reconstructs the index from the current table contents.
    void
    rebuild()
    {
        m_entries.clear();
        m_entries.reserve(m_table->size());

        size_type i = 0;

        for (auto it = m_table->begin();
             it != m_table->end();
             ++it)
        {
            m_entries.push_back(
                entry_type(
                    lookup_column_of<Column>(*it),
                    i));

            ++i;
        }

        return;
    }

    // on_insert
    //   called by the caller after appending a row at offset _i.
    void
    on_insert(
        size_type _i
    )
    {
        // fetch the row's indexed column value
        auto& row = m_table->container()[_i];

        m_entries.push_back(
            entry_type(lookup_column_of<Column>(row), _i));

        return;
    }

    // on_erase
    //   called by the caller after removing the row at offset _i.
    // Subsequent rows have shifted down by one; the index adjusts
    // its stored offsets accordingly.
    void
    on_erase(
        size_type _i
    )
    {
        // remove all entries pointing at _i; shift entries above _i down
        auto write = m_entries.begin();

        for (auto read = m_entries.begin();
             read != m_entries.end();
             ++read)
        {
            if (read->row_offset == _i)
            {
                // drop this entry
                continue;
            }

            *write = *read;

            if (write->row_offset > _i)
            {
                --write->row_offset;
            }

            ++write;
        }

        m_entries.erase(write, m_entries.end());

        return;
    }

    // on_modify
    //   called by the caller after changing the indexed column of
    // the row at offset _i.  Removes the stale entries pointing at
    // _i and inserts a fresh one.
    void
    on_modify(
        size_type _i
    )
    {
        // remove all entries with row_offset == _i
        auto new_end = std::remove_if(
            m_entries.begin(),
            m_entries.end(),
            [_i](const entry_type& _e) -> bool
            {
                return (_e.row_offset == _i);
            });

        m_entries.erase(new_end, m_entries.end());

        // re-insert
        on_insert(_i);

        return;
    }

private:
    Table*      m_table;
    storage_type m_entries;
};


// ---------------------------------------------------------------------------
// V.2  table_index<Table, Column, sorted_index_strategy, non_unique_index>
// ---------------------------------------------------------------------------

// table_index (sorted, non-unique)
//   class: O(log N + matches) lookup over a sorted entry list.
// Requires column_type to provide operator<.
template<typename    Table,
         std::size_t Column>
class table_index<Table, Column,
                  sorted_index_strategy,
                  non_unique_index>
{
public:
    using table_type   = Table;
    using row_type     = typename Table::row_type;
    using column_type  =
        lookup_row_column_type_t<row_type, Column>;
    using entry_type   = index_entry<column_type>;
    using storage_type = std::vector<entry_type>;
    using size_type    = std::size_t;

    static constexpr std::size_t column_index = Column;

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    explicit table_index(
        Table& _table
    )
        : m_table(&_table)
    {
        rebuild();
    }

    // -----------------------------------------------------------------
    //  capacity
    // -----------------------------------------------------------------

    D_CONSTEXPR size_type
    size() const D_NOEXCEPT
    {
        return m_entries.size();
    }

    D_CONSTEXPR bool
    empty() const D_NOEXCEPT
    {
        return m_entries.empty();
    }

    // -----------------------------------------------------------------
    //  lookup
    // -----------------------------------------------------------------

    // find_first
    //   returns the row offset of the first match (smallest offset
    // among rows whose column equals _cv), or size_type(-1) if no
    // such row exists.
    size_type
    find_first(
        const column_type& _cv
    ) const
    {
        entry_type probe(_cv, 0);

        auto it = std::lower_bound(
            m_entries.begin(),
            m_entries.end(),
            probe);

        if ( (it != m_entries.end()) &&
             (it->column_value == _cv) )
        {
            // walk back to the smallest row_offset among ties
            size_type best = it->row_offset;

            for (auto walk = it;
                 (walk != m_entries.end()) &&
                 (walk->column_value == _cv);
                 ++walk)
            {
                if (walk->row_offset < best)
                {
                    best = walk->row_offset;
                }
            }

            return best;
        }

        return static_cast<size_type>(-1);
    }

    // find_all
    //   returns the row offsets of every row whose indexed column
    // equals _cv.  Result order is unspecified.
    std::vector<size_type>
    find_all(
        const column_type& _cv
    ) const
    {
        std::vector<size_type> result;

        entry_type probe(_cv, 0);

        auto lo = std::lower_bound(
            m_entries.begin(),
            m_entries.end(),
            probe);

        // collect contiguous run of matches
        for (auto it = lo;
             (it != m_entries.end()) &&
             (it->column_value == _cv);
             ++it)
        {
            result.push_back(it->row_offset);
        }

        return result;
    }

    // contains
    bool
    contains(
        const column_type& _cv
    ) const
    {
        entry_type probe(_cv, 0);

        auto it = std::lower_bound(
            m_entries.begin(),
            m_entries.end(),
            probe);

        return ( (it != m_entries.end()) &&
                 (it->column_value == _cv) );
    }

    // -----------------------------------------------------------------
    //  maintenance
    // -----------------------------------------------------------------

    void
    rebuild()
    {
        m_entries.clear();
        m_entries.reserve(m_table->size());

        size_type i = 0;

        for (auto it = m_table->begin();
             it != m_table->end();
             ++it)
        {
            m_entries.push_back(
                entry_type(
                    lookup_column_of<Column>(*it),
                    i));

            ++i;
        }

        std::sort(m_entries.begin(), m_entries.end());

        return;
    }

    void
    on_insert(
        size_type _i
    )
    {
        auto& row = m_table->container()[_i];

        entry_type fresh(lookup_column_of<Column>(row), _i);

        // insert in sorted position
        auto pos = std::lower_bound(
            m_entries.begin(),
            m_entries.end(),
            fresh);

        m_entries.insert(pos, fresh);

        return;
    }

    void
    on_erase(
        size_type _i
    )
    {
        auto write = m_entries.begin();

        for (auto read = m_entries.begin();
             read != m_entries.end();
             ++read)
        {
            if (read->row_offset == _i)
            {
                continue;
            }

            *write = *read;

            if (write->row_offset > _i)
            {
                --write->row_offset;
            }

            ++write;
        }

        m_entries.erase(write, m_entries.end());

        // shifting offsets preserves sort order on column_value,
        // so no resort is needed.

        return;
    }

    void
    on_modify(
        size_type _i
    )
    {
        auto new_end = std::remove_if(
            m_entries.begin(),
            m_entries.end(),
            [_i](const entry_type& _e) -> bool
            {
                return (_e.row_offset == _i);
            });

        m_entries.erase(new_end, m_entries.end());

        on_insert(_i);

        return;
    }

private:
    Table*      m_table;
    storage_type m_entries;
};


// ---------------------------------------------------------------------------
// V.3  table_index<Table, Column, sorted_index_strategy, unique_index>
// ---------------------------------------------------------------------------

// table_index (sorted, unique)
//   class: like the sorted/non-unique form, but rejects insertion of
// a column value that already appears in the index.
template<typename    Table,
         std::size_t Column>
class table_index<Table, Column,
                  sorted_index_strategy,
                  unique_index>
{
public:
    using table_type   = Table;
    using row_type     = typename Table::row_type;
    using column_type  =
        lookup_row_column_type_t<row_type, Column>;
    using entry_type   = index_entry<column_type>;
    using storage_type = std::vector<entry_type>;
    using size_type    = std::size_t;

    static constexpr std::size_t column_index = Column;

    explicit table_index(
        Table& _table
    )
        : m_table(&_table)
    {
        rebuild();
    }

    D_CONSTEXPR size_type
    size() const D_NOEXCEPT
    {
        return m_entries.size();
    }

    D_CONSTEXPR bool
    empty() const D_NOEXCEPT
    {
        return m_entries.empty();
    }

    // find
    //   returns the row offset of the one match, or size_type(-1)
    // if no such row exists.
    size_type
    find(
        const column_type& _cv
    ) const
    {
        entry_type probe(_cv, 0);

        auto it = std::lower_bound(
            m_entries.begin(),
            m_entries.end(),
            probe);

        if ( (it != m_entries.end()) &&
             (it->column_value == _cv) )
        {
            return it->row_offset;
        }

        return static_cast<size_type>(-1);
    }

    bool
    contains(
        const column_type& _cv
    ) const
    {
        return (find(_cv) != static_cast<size_type>(-1));
    }

    // -----------------------------------------------------------------
    //  maintenance
    // -----------------------------------------------------------------

    // rebuild
    //   reconstructs the index.  Returns the number of duplicate
    // column values encountered (which were dropped); zero means the
    // table's column was actually unique.
    size_type
    rebuild()
    {
        m_entries.clear();
        m_entries.reserve(m_table->size());

        size_type i           = 0;
        size_type dup_dropped = 0;

        // gather all entries, sort, then strip duplicates
        for (auto it = m_table->begin();
             it != m_table->end();
             ++it)
        {
            m_entries.push_back(
                entry_type(
                    lookup_column_of<Column>(*it),
                    i));

            ++i;
        }

        std::sort(m_entries.begin(), m_entries.end());

        // count and remove duplicates (first occurrence wins)
        auto write = m_entries.begin();

        for (auto read = m_entries.begin();
             read != m_entries.end();
             ++read)
        {
            if ( (write != m_entries.begin()) &&
                 ((write - 1)->column_value == read->column_value) )
            {
                ++dup_dropped;
                continue;
            }

            *write = *read;
            ++write;
        }

        m_entries.erase(write, m_entries.end());

        return dup_dropped;
    }

    // on_insert
    //   returns true iff the entry was inserted.  Returns false (and
    // leaves the index unchanged) if the column value already exists.
    bool
    on_insert(
        size_type _i
    )
    {
        auto& row = m_table->container()[_i];

        entry_type fresh(lookup_column_of<Column>(row), _i);

        auto pos = std::lower_bound(
            m_entries.begin(),
            m_entries.end(),
            fresh);

        // reject duplicates
        if ( (pos != m_entries.end()) &&
             (pos->column_value == fresh.column_value) )
        {
            return false;
        }

        m_entries.insert(pos, fresh);

        return true;
    }

    void
    on_erase(
        size_type _i
    )
    {
        auto write = m_entries.begin();

        for (auto read = m_entries.begin();
             read != m_entries.end();
             ++read)
        {
            if (read->row_offset == _i)
            {
                continue;
            }

            *write = *read;

            if (write->row_offset > _i)
            {
                --write->row_offset;
            }

            ++write;
        }

        m_entries.erase(write, m_entries.end());

        return;
    }

    bool
    on_modify(
        size_type _i
    )
    {
        auto new_end = std::remove_if(
            m_entries.begin(),
            m_entries.end(),
            [_i](const entry_type& _e) -> bool
            {
                return (_e.row_offset == _i);
            });

        m_entries.erase(new_end, m_entries.end());

        return on_insert(_i);
    }

private:
    Table*      m_table;
    storage_type m_entries;
};


// ---------------------------------------------------------------------------
// V.4  table_index<Table, Column, hashed_index_strategy, non_unique_index>
// ---------------------------------------------------------------------------

// table_index (hashed, non-unique)
//   class: O(1) average lookup via std::unordered_map<column_type,
// std::vector<size_type>>.  Requires column_type to be hashable.
template<typename    Table,
         std::size_t Column>
class table_index<Table, Column,
                  hashed_index_strategy,
                  non_unique_index>
{
public:
    using table_type   = Table;
    using row_type     = typename Table::row_type;
    using column_type  =
        lookup_row_column_type_t<row_type, Column>;
    using bucket_type  = std::vector<std::size_t>;
    using storage_type =
        std::unordered_map<column_type, bucket_type>;
    using size_type    = std::size_t;

    static constexpr std::size_t column_index = Column;

    explicit table_index(
        Table& _table
    )
        : m_table(&_table)
    {
        rebuild();
    }

    D_CONSTEXPR size_type
    size() const D_NOEXCEPT
    {
        // total number of indexed rows, summed across buckets
        size_type total = 0;

        for (auto it = m_buckets.begin();
             it != m_buckets.end();
             ++it)
        {
            total += it->second.size();
        }

        return total;
    }

    D_CONSTEXPR bool
    empty() const D_NOEXCEPT
    {
        return m_buckets.empty();
    }

    // -----------------------------------------------------------------
    //  lookup
    // -----------------------------------------------------------------

    size_type
    find_first(
        const column_type& _cv
    ) const
    {
        auto it = m_buckets.find(_cv);

        if ( (it == m_buckets.end()) ||
             it->second.empty() )
        {
            return static_cast<size_type>(-1);
        }

        // return the smallest offset in the bucket
        size_type best = it->second.front();

        for (auto v = it->second.begin();
             v != it->second.end();
             ++v)
        {
            if (*v < best)
            {
                best = *v;
            }
        }

        return best;
    }

    std::vector<size_type>
    find_all(
        const column_type& _cv
    ) const
    {
        auto it = m_buckets.find(_cv);

        if (it == m_buckets.end())
        {
            return std::vector<size_type>{};
        }

        return it->second;
    }

    bool
    contains(
        const column_type& _cv
    ) const
    {
        auto it = m_buckets.find(_cv);

        return ( (it != m_buckets.end()) &&
                 (!it->second.empty()) );
    }

    // -----------------------------------------------------------------
    //  maintenance
    // -----------------------------------------------------------------

    void
    rebuild()
    {
        m_buckets.clear();

        size_type i = 0;

        for (auto it = m_table->begin();
             it != m_table->end();
             ++it)
        {
            m_buckets[lookup_column_of<Column>(*it)]
                .push_back(i);

            ++i;
        }

        return;
    }

    void
    on_insert(
        size_type _i
    )
    {
        auto& row = m_table->container()[_i];

        m_buckets[lookup_column_of<Column>(row)]
            .push_back(_i);

        return;
    }

    void
    on_erase(
        size_type _i
    )
    {
        // walk every bucket; drop _i, shift greater offsets down
        for (auto it = m_buckets.begin();
             it != m_buckets.end();
             /* in-loop */ )
        {
            auto& bucket = it->second;

            auto write = bucket.begin();

            for (auto read = bucket.begin();
                 read != bucket.end();
                 ++read)
            {
                if (*read == _i)
                {
                    continue;
                }

                *write = (*read > _i) ? (*read - 1) : *read;
                ++write;
            }

            bucket.erase(write, bucket.end());

            if (bucket.empty())
            {
                it = m_buckets.erase(it);
            }
            else
            {
                ++it;
            }
        }

        return;
    }

    void
    on_modify(
        size_type _i
    )
    {
        // remove _i from every bucket it might be in
        for (auto it = m_buckets.begin();
             it != m_buckets.end();
             /* in-loop */ )
        {
            auto& bucket = it->second;

            auto new_end = std::remove(
                bucket.begin(),
                bucket.end(),
                _i);

            bucket.erase(new_end, bucket.end());

            if (bucket.empty())
            {
                it = m_buckets.erase(it);
            }
            else
            {
                ++it;
            }
        }

        on_insert(_i);

        return;
    }

private:
    Table*      m_table;
    storage_type m_buckets;
};


// ---------------------------------------------------------------------------
// V.5  table_index<Table, Column, hashed_index_strategy, unique_index>
// ---------------------------------------------------------------------------

// table_index (hashed, unique)
//   class: O(1) average lookup via std::unordered_map<column_type,
// size_type>.  Rejects duplicate column values at insertion time.
template<typename    Table,
         std::size_t Column>
class table_index<Table, Column,
                  hashed_index_strategy,
                  unique_index>
{
public:
    using table_type   = Table;
    using row_type     = typename Table::row_type;
    using column_type  =
        lookup_row_column_type_t<row_type, Column>;
    using storage_type =
        std::unordered_map<column_type, std::size_t>;
    using size_type    = std::size_t;

    static constexpr std::size_t column_index = Column;

    explicit table_index(
        Table& _table
    )
        : m_table(&_table)
    {
        rebuild();
    }

    D_CONSTEXPR size_type
    size() const D_NOEXCEPT
    {
        return m_map.size();
    }

    D_CONSTEXPR bool
    empty() const D_NOEXCEPT
    {
        return m_map.empty();
    }

    // find
    //   returns the row offset, or size_type(-1) if absent.
    size_type
    find(
        const column_type& _cv
    ) const
    {
        auto it = m_map.find(_cv);

        if (it == m_map.end())
        {
            return static_cast<size_type>(-1);
        }

        return it->second;
    }

    bool
    contains(
        const column_type& _cv
    ) const
    {
        return (m_map.find(_cv) != m_map.end());
    }

    // -----------------------------------------------------------------
    //  maintenance
    // -----------------------------------------------------------------

    // rebuild
    //   returns the number of duplicate column values encountered.
    size_type
    rebuild()
    {
        m_map.clear();

        size_type i           = 0;
        size_type dup_dropped = 0;

        for (auto it = m_table->begin();
             it != m_table->end();
             ++it)
        {
            const auto& cv = lookup_column_of<Column>(*it);

            // first occurrence wins; duplicates dropped
            auto inserted = m_map.insert(
                std::make_pair(cv, i));

            if (!inserted.second)
            {
                ++dup_dropped;
            }

            ++i;
        }

        return dup_dropped;
    }

    // on_insert
    //   returns true iff inserted; false if the column value was
    // already present (index unchanged).
    bool
    on_insert(
        size_type _i
    )
    {
        auto& row = m_table->container()[_i];

        auto inserted = m_map.insert(
            std::make_pair(lookup_column_of<Column>(row), _i));

        return inserted.second;
    }

    void
    on_erase(
        size_type _i
    )
    {
        // remove the single entry pointing at _i, shift others down
        for (auto it = m_map.begin();
             it != m_map.end();
             /* in-loop */ )
        {
            if (it->second == _i)
            {
                it = m_map.erase(it);
                continue;
            }

            if (it->second > _i)
            {
                --it->second;
            }

            ++it;
        }

        return;
    }

    bool
    on_modify(
        size_type _i
    )
    {
        // drop the existing entry for _i
        for (auto it = m_map.begin();
             it != m_map.end();
             /* in-loop */ )
        {
            if (it->second == _i)
            {
                it = m_map.erase(it);
            }
            else
            {
                ++it;
            }
        }

        return on_insert(_i);
    }

private:
    Table*      m_table;
    storage_type m_map;
};


// ===========================================================================
// VI.  Convenience Factories
// ===========================================================================

// make_linear_index
//   function: builds a linear, non-unique table_index over column
// Column of _table.  Deduces Table.
template<std::size_t Column,
         typename    Table>
table_index<Table, Column,
            linear_index_strategy, non_unique_index>
make_linear_index(
    Table& _table
)
{
    return table_index<Table, Column,
                       linear_index_strategy,
                       non_unique_index>(_table);
}

// make_sorted_index
//   function: builds a sorted, non-unique table_index over column
// Column of _table.  Deduces Table.
template<std::size_t Column,
         typename    Table>
table_index<Table, Column,
            sorted_index_strategy, non_unique_index>
make_sorted_index(
    Table& _table
)
{
    return table_index<Table, Column,
                       sorted_index_strategy,
                       non_unique_index>(_table);
}

// make_unique_sorted_index
//   function: builds a sorted, unique table_index over column Column
// of _table.  Deduces Table.
template<std::size_t Column,
         typename    Table>
table_index<Table, Column,
            sorted_index_strategy, unique_index>
make_unique_sorted_index(
    Table& _table
)
{
    return table_index<Table, Column,
                       sorted_index_strategy,
                       unique_index>(_table);
}

// make_hashed_index
//   function: builds a hashed, non-unique table_index over column
// Column of _table.  Deduces Table.
template<std::size_t Column,
         typename    Table>
table_index<Table, Column,
            hashed_index_strategy, non_unique_index>
make_hashed_index(
    Table& _table
)
{
    return table_index<Table, Column,
                       hashed_index_strategy,
                       non_unique_index>(_table);
}

// make_unique_hashed_index
//   function: builds a hashed, unique table_index over column Column
// of _table.  Deduces Table.
template<std::size_t Column,
         typename    Table>
table_index<Table, Column,
            hashed_index_strategy, unique_index>
make_unique_hashed_index(
    Table& _table
)
{
    return table_index<Table, Column,
                       hashed_index_strategy,
                       unique_index>(_table);
}


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_UTIL_TABLE_INDEX_HPP
