/*******************************************************************************
* djinterp [core]                                              lookup_traits.hpp
*
* Row classification and column access traits for lookup tables.
*
*   This header is the trait surface for the lookup family.  It exposes
* the minimum surface area a lookup container needs to operate over a
* row: classification (singular vs tuple), key column resolution, value
* column resolution, and per-column type and value extraction.  All row
* introspection is layered over the std::tuple protocol via the
* dtuple.hpp traits; no member-named protocol is recognized here.
*
*   ROW SHAPES:
*   Exactly two are recognized.
*
*   - TUPLE-LIKE - Row is a std::tuple specialization (detected via
*                  is_tuple).  Columns are accessed positionally via
*                  std::get / tuple_type_at_value.  Multi-column rows
*                  carry a key column (default index 0) and, if the
*                  arity allows, a value column (default index 1).
*                  Columns beyond the value column are invisible to
*                  lookup_traits - they belong to higher layers.
*
*   - SINGULAR   - any non-std::tuple row.  The row IS the key; there
*                  is no value column.  Common shapes: bare keys
*                  (`std::string`, an enum) for flag-only sets.
*
*   USER STRUCTS WITH MULTIPLE FIELDS:
*   The intent is that consumers wanting multi-column rows use
* `std::tuple<...>` directly.  A custom struct can participate by
* specializing the std::tuple protocol for it (std::tuple_size,
* std::tuple_element, std::get) - the trait surface here will pick it
* up if `is_tuple<R>` is taught to recognize it.
*
*   KEY / VALUE COLUMN OVERRIDE (TUPLE ROWS ONLY):
*   Key column defaults to 0; value column defaults to 1.  Override by
* exposing static constexpr `key_column` / `value_column` members on
* the row type, or by specializing `lookup_row_key_column<R>` /
* `lookup_row_value_column<R>`.  For singular rows these knobs are
* meaningless and ignored.
*
*   WHAT LIVES ABOVE:
*   Anything that uses columns beyond key/value (descriptions, default
* values, bounds, sort priority, etc.) is the responsibility of the
* caller's row type and the option_pair_traits / option_set_traits
* layer.  This header sees only key, value, and the row's tuple arity.
* Database-style secondary indices over arbitrary columns are provided
* by table_index.hpp, which uses lookup_row_column_type_t for general
* column access.
*
*   RELATIONSHIP TO util/lookup.hpp:
*   Orthogonal.  util/lookup.hpp searches a PACK of entries by key
* (find_by_key, find_by_pred, the sorted binary-search family); this
* header resolves the COLUMNS WITHIN a single row.  A lookup container
* composes the two: lookup_traits says where the key column is and what
* type it has, and util/lookup.hpp finds the row carrying a given key.
* Neither header includes the other.
*
*   COLUMN ACCESS PRIMITIVE:
*   Tuple-row column typing delegates to dtuple::tuple_type_at, which is
* itself the tuple-facing wrapper over type_traits::pack_element (the
* shared indexed-pack-access idiom).  This header depends only on the
* tuple-facing accessor; it never indexes a raw pack directly.
*
*
* path:      /inc/djinterp/core/util/lookup/lookup_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.23
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Row Classification
      ------------------
      1.    is_tuple_row                    (+ _v)
      2.    is_singular_row                 (+ _v)
      3.    lookup_row_arity                (+ _v)

II.   Column Index Customization
      --------------------------
      1.    has_key_column_member           (+ _v)
      2.    has_value_column_member         (+ _v)
      3.    lookup_row_key_column           (+ _v)
      4.    lookup_row_value_column         (+ _v)

III.  Value Column Presence
      ---------------------
      1.    has_value_column                (+ _v)

IV.   Column Type Resolution
      ----------------------
      1.    lookup_row_column_type          (+ _t)
      2.    lookup_row_key_type             (+ _t)
      3.    lookup_row_value_type           (+ _t)

V.    Column Extraction (runtime)
      ---------------------------
      1.    lookup_key_of
      2.    lookup_value_of
      3.    lookup_column_of

VI.   Convenience _v / _t Aliases
      ---------------------------
*/

#ifndef DJINTERP_UTIL_LOOKUP_LOOKUP_TRAITS_HPP
#define DJINTERP_UTIL_LOOKUP_LOOKUP_TRAITS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "../../meta/dtuple.hpp"


NS_DJINTERP


// ===========================================================================
// I.   Row Classification
// ===========================================================================

// ---------------------------------------------------------------------------
// 1. is_tuple_row
// ---------------------------------------------------------------------------

// is_tuple_row
//   trait: true iff Row is a std::tuple specialization.  Delegates to
// `is_tuple` from type_traits.hpp.  Tuple rows are the only multi-
// column shape recognized by lookup_traits.
template<typename Row>
struct is_tuple_row
    : is_tuple<clean_t<Row>>
{};


// ---------------------------------------------------------------------------
// 2. is_singular_row
// ---------------------------------------------------------------------------

// is_singular_row
//   trait: true iff Row is NOT a tuple.  In this case the row IS the
// key and there is no value column.  Used for flag-only sets and bare-
// key tables.
template<typename Row>
struct is_singular_row
    : std::integral_constant<bool, !is_tuple_row<Row>::value>
{};


// ---------------------------------------------------------------------------
// 3. lookup_row_arity
// ---------------------------------------------------------------------------

// lookup_row_arity
//   trait: number of columns in Row.
//   - singular rows: 1.
//   - tuple rows:    std::tuple_size<Row>::value.
template<typename Row,
         bool     IsTuple = is_tuple_row<Row>::value>
struct lookup_row_arity
    : std::integral_constant<std::size_t, 1>
{};

template<typename Row>
struct lookup_row_arity<Row, true>
    : std::integral_constant<std::size_t,
                             std::tuple_size<clean_t<Row>>::value>
{};


// ===========================================================================
// II.  Column Index Customization
// ===========================================================================

// ---------------------------------------------------------------------------
// 1. has_key_column_member
// ---------------------------------------------------------------------------

// has_key_column_member
//   trait: true iff Row exposes a static constexpr std::size_t
// key_column member.  Tuple rows can override the default key column
// (0) via this mechanism.  Singular rows ignore it.
NS_INTERNAL

    template<typename Row,
             typename = void>
    struct has_key_column_member_helper : std::false_type
    {};

    template<typename Row>
    struct has_key_column_member_helper<Row,
        D_VOID_T<decltype(Row::key_column)>>
        : std::true_type
    {};

NS_END  // internal

template<typename Row>
struct has_key_column_member
    : internal::has_key_column_member_helper<clean_t<Row>>
{};


// ---------------------------------------------------------------------------
// 2. has_value_column_member
// ---------------------------------------------------------------------------

// has_value_column_member
//   trait: true iff Row exposes a static constexpr std::size_t
// value_column member.  Tuple rows can override the default value
// column (1) via this mechanism.  Singular rows ignore it.
NS_INTERNAL

    template<typename Row,
             typename = void>
    struct has_value_column_member_helper : std::false_type
    {};

    template<typename Row>
    struct has_value_column_member_helper<Row,
        D_VOID_T<decltype(Row::value_column)>>
        : std::true_type
    {};

NS_END  // internal

template<typename Row>
struct has_value_column_member
    : internal::has_value_column_member_helper<clean_t<Row>>
{};


// ---------------------------------------------------------------------------
// 3. lookup_row_key_column
// ---------------------------------------------------------------------------

// lookup_row_key_column
//   trait: tuple index of the key column in Row.
//   - defaults to 0.
//   - picks up `static constexpr std::size_t key_column = N;` if
//     present on the row type.
//   - may be specialized for types whose source cannot be modified.
//
//   Meaningful only for tuple rows.  Querying it on singular rows
// yields 0; harmless but not useful.
template<typename Row,
         bool     HasMember = has_key_column_member<Row>::value>
struct lookup_row_key_column
    : std::integral_constant<std::size_t, 0>
{};

template<typename Row>
struct lookup_row_key_column<Row, true>
    : std::integral_constant<std::size_t,
                             clean_t<Row>::key_column>
{};


// ---------------------------------------------------------------------------
// 4. lookup_row_value_column
// ---------------------------------------------------------------------------

// lookup_row_value_column
//   trait: tuple index of the value column in Row.
//   - defaults to 1.
//   - picks up `static constexpr std::size_t value_column = N;` if
//     present on the row type.
//   - may be specialized for types whose source cannot be modified.
//
//   Meaningful only for tuple rows with arity > value_column.  See
// has_value_column.
template<typename Row,
         bool     HasMember = has_value_column_member<Row>::value>
struct lookup_row_value_column
    : std::integral_constant<std::size_t, 1>
{};

template<typename Row>
struct lookup_row_value_column<Row, true>
    : std::integral_constant<std::size_t,
                             clean_t<Row>::value_column>
{};


// ===========================================================================
// III. Value Column Presence
// ===========================================================================

// has_value_column
//   trait: true iff Row is a tuple with arity strictly greater than
// lookup_row_value_column<Row>.  Singular rows always yield false.
// This is the gate consumers should check before accessing value-
// related traits and extractors.
template<typename Row,
         bool     IsTuple = is_tuple_row<Row>::value>
struct has_value_column : std::false_type
{};

template<typename Row>
struct has_value_column<Row, true>
    : std::integral_constant<bool,
        ( lookup_row_arity<Row>::value >
          lookup_row_value_column<Row>::value )>
{};


// ===========================================================================
// IV.  Column Type Resolution
// ===========================================================================

// ---------------------------------------------------------------------------
// 1. lookup_row_column_type
// ---------------------------------------------------------------------------

// lookup_row_column_type
//   trait: type of the I-th column of Row.
//   - singular rows: only I == 0 is valid; type is clean_t<Row>.
//   - tuple rows:    delegates to dtuple::tuple_type_at<I, Row>.
//
//   Provided primarily so table_index can address arbitrary columns;
// lookup itself only ever resolves the key and value columns.
template<typename    Row,
         std::size_t I,
         bool        IsTuple = is_tuple_row<Row>::value>
struct lookup_row_column_type;

template<typename Row>
struct lookup_row_column_type<Row, 0, false>
{
    using type = clean_t<Row>;
};

template<typename    Row,
         std::size_t I>
struct lookup_row_column_type<Row, I, true>
{
    static_assert(
        (I < std::tuple_size<clean_t<Row>>::value),
        "Non-type parameter `I` exceeds tuple arity of `Row`.");

    using type =
        tuple_type_at_t<I, clean_t<Row>>;
};

// lookup_row_column_type_t
//   type: convenience alias for lookup_row_column_type<...>::type.
template<typename    Row,
         std::size_t I>
using lookup_row_column_type_t =
    typename lookup_row_column_type<Row, I>::type;


// ---------------------------------------------------------------------------
// 2. lookup_row_key_type
// ---------------------------------------------------------------------------

// lookup_row_key_type
//   trait: type of the key column in Row.
//   - singular rows: clean_t<Row> (the row itself).
//   - tuple rows:    column type at lookup_row_key_column<Row>.
template<typename Row>
struct lookup_row_key_type
{
    using type =
        lookup_row_column_type_t<Row,
                                 lookup_row_key_column<Row>::value>;
};

// lookup_row_key_type_t
//   type: convenience alias for lookup_row_key_type<...>::type.
template<typename Row>
using lookup_row_key_type_t =
    typename lookup_row_key_type<Row>::type;


// ---------------------------------------------------------------------------
// 3. lookup_row_value_type
// ---------------------------------------------------------------------------

// lookup_row_value_type
//   trait: type of the value column in Row.  Only defined when
// has_value_column<Row>::value is true.  Consumers must gate on
// has_value_column before naming this trait.
template<typename Row,
         bool     HasValue = has_value_column<Row>::value>
struct lookup_row_value_type;

template<typename Row>
struct lookup_row_value_type<Row, true>
{
    using type =
        lookup_row_column_type_t<Row,
                                 lookup_row_value_column<Row>::value>;
};

// lookup_row_value_type_t
//   type: convenience alias for lookup_row_value_type<...>::type.
template<typename Row>
using lookup_row_value_type_t =
    typename lookup_row_value_type<Row>::type;


// ===========================================================================
// V.   Column Extraction (runtime)
// ===========================================================================

// ---------------------------------------------------------------------------
// 1. lookup_key_of
// ---------------------------------------------------------------------------

// lookup_key_of
//   function: extracts the key column from a row instance.
//   - singular rows: returns _row by const reference.
//   - tuple rows:    returns std::get<key_column>(_row).
NS_INTERNAL

    template<typename Row>
    D_CONSTEXPR const clean_t<Row>&
    lookup_key_of_dispatch(
        const Row&     _row,
        std::false_type /*is_tuple*/
    )
    {
        return _row;
    }

    template<typename Row>
    D_CONSTEXPR auto
    lookup_key_of_dispatch(
        const Row&    _row,
        std::true_type /*is_tuple*/
    ) -> decltype(
        std::get<lookup_row_key_column<Row>::value>(_row))
    {
        return std::get<lookup_row_key_column<Row>::value>(_row);
    }

NS_END  // internal

template<typename Row>
D_CONSTEXPR auto
lookup_key_of(
    const Row& _row
) -> decltype(internal::lookup_key_of_dispatch(
        _row,
        std::integral_constant<bool,
                               is_tuple_row<Row>::value>{}))
{
    return internal::lookup_key_of_dispatch(
        _row,
        std::integral_constant<bool,
                               is_tuple_row<Row>::value>{});
}


// ---------------------------------------------------------------------------
// 2. lookup_value_of
// ---------------------------------------------------------------------------

// lookup_value_of
//   function: extracts the value column from a row instance.  Only
// well-formed when has_value_column<Row>::value is true; consumers
// must gate on that trait before calling.
template<typename Row>
D_CONSTEXPR auto
lookup_value_of(
    const Row& _row
) -> decltype(
    std::get<lookup_row_value_column<Row>::value>(_row))
{
    static_assert(has_value_column<Row>::value,
                  "lookup_value_of requires Row to expose a value "
                  "column; check has_value_column<Row>::value first "
                  "or use lookup_key_of for singular rows.");

    return std::get<lookup_row_value_column<Row>::value>(_row);
}


// ---------------------------------------------------------------------------
// 3. lookup_column_of
// ---------------------------------------------------------------------------

// lookup_column_of
//   function: extracts the I-th column from a row instance.  Used
// primarily by table_index to address arbitrary columns; lookup itself
// only ever calls lookup_key_of / lookup_value_of.
//
//   For singular rows only I == 0 is well-formed (returns the row).
// For tuple rows I must satisfy I < std::tuple_size<Row>::value.
NS_INTERNAL

    template<std::size_t I,
             typename    Row>
    D_CONSTEXPR const clean_t<Row>&
    lookup_column_of_dispatch(
        const Row&     _row,
        std::false_type /*is_tuple*/
    )
    {
        static_assert((I == 0),
                      "Non-type parameter `I` must be 0 for "
                      "singular (non-tuple) rows.");

        return _row;
    }

    template<std::size_t I,
             typename    Row>
    D_CONSTEXPR auto
    lookup_column_of_dispatch(
        const Row&    _row,
        std::true_type /*is_tuple*/
    ) -> decltype(std::get<I>(_row))
    {
        return std::get<I>(_row);
    }

NS_END  // internal

template<std::size_t I,
         typename    Row>
D_CONSTEXPR auto
lookup_column_of(
    const Row& _row
) -> decltype(internal::lookup_column_of_dispatch<I>(
        _row,
        std::integral_constant<bool,
                               is_tuple_row<Row>::value>{}))
{
    return internal::lookup_column_of_dispatch<I>(
        _row,
        std::integral_constant<bool,
                               is_tuple_row<Row>::value>{});
}


// ===========================================================================
// VI.  Convenience _v / _t Aliases
// ===========================================================================

template<typename Row>
inline constexpr bool is_tuple_row_v =
    is_tuple_row<Row>::value;

template<typename Row>
inline constexpr bool is_singular_row_v =
    is_singular_row<Row>::value;

template<typename Row>
inline constexpr std::size_t lookup_row_arity_v =
    lookup_row_arity<Row>::value;

template<typename Row>
inline constexpr bool has_key_column_member_v =
    has_key_column_member<Row>::value;

template<typename Row>
inline constexpr bool has_value_column_member_v =
    has_value_column_member<Row>::value;

template<typename Row>
inline constexpr std::size_t lookup_row_key_column_v =
    lookup_row_key_column<Row>::value;

template<typename Row>
inline constexpr std::size_t lookup_row_value_column_v =
    lookup_row_value_column<Row>::value;

template<typename Row>
inline constexpr bool has_value_column_v =
    has_value_column<Row>::value;


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_UTIL_LOOKUP_LOOKUP_TRAITS_HPP
