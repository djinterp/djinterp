/******************************************************************************
* djinterp [option]                                             option_set.hpp
*
*   The option_set<> core type plus the structural machinery needed to
* CONSTRUCT one safely.
*
*   STRICTNESS CHANGE (2026.05.27):
*   Previously, option_set accepted any "keyed" type (one exposing
* ::key and ::key_type).  The new contract is stricter: every entry,
* after expansion through ::expanded_t, must be an option<...> per
* is_option_v.  Multi-expanders (types exposing ::expanded_t) must
* therefore expand to a tuple of option<>s - or to an empty tuple,
* which is the right shape for passthrough markers (see
* meta/passthrough.hpp and option_passthrough.hpp).
*
*   The loose `is_keyed` contract has been retired entirely.  If a
* user wants to use a custom option-like type with option_set, that
* type must satisfy is_option_v (either by specializing the trait or,
* more idiomatically, by being some option<K, ...> specialization).
*
*   What this header provides:
*     - expand_option      : the per-entry ::expanded_t customization
*                            point (preserved from earlier design).
*     - flatten_tuples_t   : type-level tuple_cat.
*     - run_set_checks     : the all-options + uniformity + uniqueness
*                            static_asserts.
*     - option_set         : the user-facing type.
*
*   Queries OVER an instantiated set (is_option_set, key_type, contains,
* find) and their C++20 concept analogs now live in this header too
* (folded in from the former option_set_traits.hpp / option_set_concepts.hpp).
* The concepts - and the one constrained query that needs a requires-clause
* (option_set_key_type) - compile only where the toolchain supports
* concepts; the rest of the header remains usable down to its prior
* standard level.
*
*   This header depends on option.hpp (for option<> and is_option_v) and
* util/lookup (for value_pack_unique / contains_key / find_by_key).  It
* does NOT depend on option_factory.hpp - it speaks only in terms of
* already-constructed option<> instantiations.
*
*
* path:      /inc/djinterp/core/option/option_set.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.25
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    expand_option              (structural per-entry expansion)
II.   flatten helpers            (tuple_cat at the type level)
III.  set checks                 (all-options + uniformity + uniqueness)
IV.   option_set                 (type-level pack form)
V.    field marker + values      (field<>, value-carrying option_set)
VI.   queries                    (is_option_set, key_type, contains, find)
VII.  concepts                   (C++20 analogs)
*/

#ifndef DJINTERP_OPTION_SET_
#define DJINTERP_OPTION_SET_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"
#include "../util/lookup/lookup.hpp"    // contains_key, find_by_key
#include "./option.hpp"                 // option<>, is_option_v


NS_DJINTERP

// ===========================================================================
// I.   expand_option
// ===========================================================================
// expand_option
//   trait: yields std::tuple<...> for ONE user-supplied entry.
//   Default specialization: an entry is its own expansion
// (wrapped in a single-element tuple).  An entry opts into
// multi-expansion by exposing a nested `::expanded_t =
// std::tuple<...>` alias.  This is the ONLY customization
// point - detection is structural, so adding a new
// multi-expanding type requires NO specialization here.
//
//   POST-EXPANSION CONTRACT:
//   The flattened union of every entry's expand_option_t MUST
// be a tuple of option<>s.  Multi-expanders that yield non-
// option types are rejected by run_set_checks below.  An empty
// tuple is acceptable (passthrough markers use this).
//
// Example multi-expander (passthrough, contributes nothing):
//   struct my_section_marker : passthrough_marker
//   {
//       using expanded_t = std::tuple<>;
//   };
template<typename _Entry,
         typename = void>
struct expand_option
{
    using type = std::tuple<_Entry>;
};

template<typename _Entry>
struct expand_option<_Entry, std::void_t<typename _Entry::expanded_t>>
{
    using type = typename _Entry::expanded_t;
};

template<typename _Entry>
using expand_option_t = typename expand_option<_Entry>::type;


// ===========================================================================
// II.  flatten helpers
// ===========================================================================

// flatten_tuples_t
//   trait: tuple_cat-style flattening at the type level.
template<typename... _Tuples>
using flatten_tuples_t = decltype(std::tuple_cat(std::declval<_Tuples>()...));


// ===========================================================================
// III. set checks
// ===========================================================================

NS_INTERNAL

    // are_all_options
    //   helper: true iff every type in the pack satisfies
    // is_option_v.  This is the strict contract option_set
    // enforces post-expansion.
    template<typename...>
    struct are_all_options : std::true_type
    {};

    template<typename    _First,
             typename... _Rest>
    struct are_all_options<_First, _Rest...>
        : std::integral_constant<bool,
            ( is_option_v<_First> &&
              are_all_options<_Rest...>::value )>
    {};

    // all_same_type
    //   helper: every type in the pack is identical.
    template<typename...>
    struct all_same_type : std::true_type
    {};

    template<typename _First>
    struct all_same_type<_First> : std::true_type
    {};

    template<typename    _First,
             typename    _Second,
             typename... _Rest>
    struct all_same_type<_First, _Second, _Rest...>
        : std::integral_constant<bool,
            ( std::is_same<_First, _Second>::value &&
              all_same_type<_Second, _Rest...>::value )>
    {};

    // run_set_checks
    //   helper: instantiated by option_set on its flat
    // (post-expansion) tuple to fire the strict-option +
    // uniformity + uniqueness static_asserts.  Exposes
    // ::value so callers can force instantiation by depending
    // on it.
    template<typename _Tuple>
    struct run_set_checks;

    // empty flat tuple (e.g. an option_set of passthroughs only)
    template<>
    struct run_set_checks<std::tuple<>>
    {
        static D_CONSTEXPR bool value = true;
    };

    template<typename    _First,
             typename... _Rest>
    struct run_set_checks<std::tuple<_First, _Rest...>>
    {
        static_assert(are_all_options<_First, _Rest...>::value,
            "option_set: every entry (after expansion through "
            "::expanded_t) must be an option<...> instantiation "
            "per is_option_v.  The previous structural "
            "'is_keyed' contract is no longer accepted.  Custom "
            "option-like types must satisfy is_option_v - see "
            "option.hpp.  Multi-expanders must produce a "
            "std::tuple of option<>s (or an empty tuple for "
            "passthrough markers).");

        static_assert(all_same_type<typename _First::key_type,
                                    typename _Rest::key_type...>::value,
            "option_set: all options (after expansion) must "
            "share the same key_type.  Use a single enum / "
            "class / scope for every key in the set.");

        static_assert(value_pack_unique<_First::key,
                                        _Rest::key...>::value,
            "option_set: all keys (after expansion) must be "
            "unique.  Multi-expanding entries (those exposing "
            "::expanded_t) emit ALL of their inner keys - any "
            "collision with a directly-declared key, or among "
            "expansions, is caught here.");

        static D_CONSTEXPR bool value = true;
    };

NS_END  // internal


// ===========================================================================
// IV.  option_set
// ===========================================================================

NS_INTERNAL

template<typename... _Entries>
struct option_set_pack
{
private:
    // 1. expand each entry per the structural ::expanded_t
    //    convention.
    // 2. flatten the per-entry tuples into one normalized
    //    tuple.
    using flat_tuple = flatten_tuples_t<expand_option_t<_Entries>...>;

    // 3. force the checks to fire by depending on ::value.
    //    Accessing the member instantiates run_set_checks,
    //    which runs the nested static_asserts.
    static_assert(internal::run_set_checks<flat_tuple>::value,
        "internal: run_set_checks did not return true (see "
        "preceding assertion for the real diagnostic).");

public:
    // size
    //   value: number of options AFTER expansion.
    static D_CONSTEXPR std::size_t size =
        std::tuple_size<flat_tuple>::value;

    // empty
    static D_CONSTEXPR bool empty = (size == 0);

    // flat_options_t
    //   type: the normalized std::tuple<...> of expanded
    // option<>s.
    using flat_options_t = flat_tuple;

    // option_at
    //   type: positional access into the flat list.
    template<std::size_t _I>
    using option_at = std::tuple_element_t<_I, flat_tuple>;
};

NS_END  // internal


// ===========================================================================
// V.   FIELD MARKER + value-carrying option_set
// ===========================================================================
//
//   option_set is BOTH faces of a configuration in ONE type:
//     - a pure TYPE-LEVEL SCHEMA when its options carry compile-time payloads
//       (val_t<V>, the node-sugar surface): used by compose / override /
//       lowering.  Its derived values_type is then all-unit, so the type is
//       pure - no instance carries storage.
//     - a POPULATED, mutable, constexpr-capable INSTANCE when its options carry
//       a field<T> marker: each field<T> option contributes one runtime slot of
//       type T, addressed by key through get<> / set<>.
//   Both faces share the type-level engine above (internal::option_set_pack):
//   the contract, flat_options_t, and key->slot resolution are reused, never
//   re-derived.  The value-carrying face is the construction target of
//   option_generator.hpp (the flat one-statement authoring front-end).

// field
//   marker: as an option's first arg, field<T> declares "this key's slot stores
// a runtime T".  Sibling of carrier.hpp's val_t<V> (a compile-time value);
// field<T> carries a runtime value's TYPE.
template<typename _Type>
struct field
{
    using type = _Type;
};

// unit
//   type: the empty slot for a key that contributes no runtime value - a
// presence-only (unary) key, or a compile-time val_t<> schema option.  get<> /
// set<> reject a unit slot, directing the caller to contains<>.
struct unit
{
};

// unary_option
//   type: a presence-only flag - sugar for option<_Key, field<unit> >.  Keeps
// the option<->slot 1:1 mapping (an empty unit slot); query it with
// contains<_Key>().
template<auto _Key>
using unary_option = option<_Key, field<unit> >;


NS_INTERNAL

    // option_field
    //   trait: the runtime field type bound to an option - its first arg read
    // as field<T> (-> T); unit otherwise (a val_t<> schema option, a unary
    // option<K>, or any non-field arg).  THE defaults / value seam: re-point at
    // extract_default_t (option_set_compare.hpp) to source explicit defaults.
    template<typename _Opt,
             typename = void>
    struct option_field
    {
        using type = unit;
    };

    template<auto        _Key,
             typename    _Type,
             typename... _Rest>
    struct option_field<option<_Key, field<_Type>, _Rest...>, void>
    {
        using type = _Type;
    };

    // store_values
    //   trait: tuple<option...> -> tuple<option_field<option>::type...> over the
    // normalized (post-expansion) flat option list.  All-unit for a pure schema.
    template<typename _Flat>
    struct store_values;

    template<typename... _Opts>
    struct store_values<std::tuple<_Opts...> >
    {
        using type = std::tuple<typename option_field<_Opts>::type...>;
    };

    // os_slot
    //   trait: key -> slot index over a flat option tuple, via find_by_key (the
    // same primitive option_set_find uses) - not a re-rolled scan.
    template<auto     _Key,
             typename _Flat>
    struct os_slot;

    template<auto        _Key,
             typename... _Opts>
    struct os_slot<_Key, std::tuple<_Opts...> >
    {
        static D_CONSTEXPR bool        found = find_by_key<_Key, _Opts...>::found;
        static D_CONSTEXPR std::size_t index = find_by_key<_Key, _Opts...>::index;
    };

NS_END  // internal


// option_field_t
//   type: the runtime field type bound to an option (its value's type); unit
// when the option carries no runtime value.
template<typename _Opt>
using option_field_t = typename internal::option_field<_Opt>::type;


#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// option_set  (public: the type-level engine PLUS the value-carrying face)
//   Inherits the contract / size / flat_options_t / option_at from
// internal::option_set_pack and adds a values_type derived from the options'
// field<T> markers (all-unit, hence storage-free, for a pure schema), a
// values-constructor, and key-addressed get<> / set<> / contains<>.  key_type
// is inferred per option from its key (option<Key,...>::key_type ==
// decltype(Key)); the head option's key_type is available via
// option_set_key_type (Section VI).
template<typename... _Entries>
struct option_set
    : internal::option_set_pack<_Entries...>
{
private:
    using base = internal::option_set_pack<_Entries...>;

    // slot_of: key -> slot index, via find_by_key.  Clamped to 0 when absent so
    // the friendly static_assert in get/set is the first diagnostic.
    template<auto _Key>
    static D_CONSTEXPR std::size_t slot_of =
        ( internal::os_slot<_Key, typename base::flat_options_t>::found
              ? internal::os_slot<_Key, typename base::flat_options_t>::index
              : std::size_t(0) );

public:
    // values_type: one slot per option (field<T> -> T, otherwise unit).
    using values_type =
        typename internal::store_values<typename base::flat_options_t>::type;

    // option_set ()
    //   constructor: default - every field value-initialized (a schema's
    // all-unit values cost nothing).
    D_CONSTEXPR option_set() = default;

    // option_set (values)
    //   constructor: seed the fields in slot order.  Guarded to the non-empty
    // set so it does not collide with the default constructor.  This is the
    // construct-with-values path option_generator drives; it assumes the
    // entries ARE the flat options (no multi-expanders), as generated instances
    // are.
    D_CONSTEXPR explicit option_set(
        option_field_t<_Entries>... _values
    )
        requires (sizeof...(_Entries) > 0)
        : m_values{ static_cast<option_field_t<_Entries>&&>(_values)... }
    {}

    // contains
    //   function: whether _Key is one of the set's keys (compile time).
    template<auto _Key>
    static D_CONSTEXPR bool
    contains()
    D_NOEXCEPT
    {
        return internal::os_slot<_Key, typename base::flat_options_t>::found;
    }

    // get
    //   function: a reference to the field bound to _Key, at its exact declared
    // type.  Rejected for a unit slot (a unary or compile-time-only option).
    template<auto _Key>
    D_NODISCARD D_CONSTEXPR auto&
    get()
    {
        static_assert(contains<_Key>(),
            "option_set::get: _Key is not one of this set's keys.");
        static_assert(
            !std::is_same<std::tuple_element_t<slot_of<_Key>, values_type>,
                          unit>::value,
            "option_set::get: _Key carries no runtime value (a unary flag or a "
            "compile-time val_t<> option); query it with contains<_Key>().");

        return std::get<slot_of<_Key> >(m_values);
    }

    template<auto _Key>
    D_NODISCARD D_CONSTEXPR const auto&
    get() const
    {
        static_assert(contains<_Key>(),
            "option_set::get: _Key is not one of this set's keys.");
        static_assert(
            !std::is_same<std::tuple_element_t<slot_of<_Key>, values_type>,
                          unit>::value,
            "option_set::get: _Key carries no runtime value (a unary flag or a "
            "compile-time val_t<> option); query it with contains<_Key>().");

        return std::get<slot_of<_Key> >(m_values);
    }

    // set
    //   function: assign the field bound to _Key.  Rejected for a unit slot.
    template<auto     _Key,
             typename _Value>
    D_CONSTEXPR void
    set(
        const _Value& _value
    )
    {
        static_assert(contains<_Key>(),
            "option_set::set: _Key is not one of this set's keys.");
        static_assert(
            !std::is_same<std::tuple_element_t<slot_of<_Key>, values_type>,
                          unit>::value,
            "option_set::set: _Key carries no runtime value.");

        std::get<slot_of<_Key> >(m_values) = _value;

        return;
    }

    // values
    //   accessor: the underlying slot tuple.
    D_NODISCARD D_CONSTEXPR const values_type&
    values() const
    D_NOEXCEPT
    {
        return m_values;
    }

private:
    values_type m_values{};
};

#else  // pre-C++20: type-level-only option_set (the value-carrying face needs
       // the requires-guarded constructor and is unavailable here)

template<typename... _Entries>
struct option_set
    : internal::option_set_pack<_Entries...>
{};

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER


// ===========================================================================
// VI.  queries
// ===========================================================================
//
//   Trait machinery OVER an instantiated option_set<> (folded in from the
// former option_set_traits.hpp).  The set-CONSTRUCTION machinery above
// (expand_option, run_set_checks) is the contract option_set enforces;
// these are for querying an already-built set.  Comparison / equality /
// value-extraction traits remain in option_set_compare.hpp.

// is_option_set
//   trait: true iff _Type is some option_set<...> specialization.
template<typename _Type>
struct is_option_set : std::false_type
{};

template<typename... _Options>
struct is_option_set<option_set<_Options...>> : std::true_type
{};

template<typename _Type>
inline constexpr bool is_option_set_v = is_option_set<clean_t<_Type>>::value;


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS
// option_set_key_type
//   trait: extracts the key_type of a non-empty option_set from the head
// of its normalized option tuple.  Uses a constrained partial
// specialization (requires-clause), so it is available only where the
// toolchain supports concepts; the rest of this section is unconstrained.
template<typename _Set>
struct option_set_key_type;

template<typename... _Options>
    requires (option_set<_Options...>::size > 0)
struct option_set_key_type<option_set<_Options...>>
{
private:
    using head_option =
        typename option_set<_Options...>::template option_at<0>;
public:
    using type = typename head_option::key_type;
};

template<typename _Set>
using option_set_key_type_t = typename option_set_key_type<_Set>::type;
#endif  // C++20 (constrained option_set_key_type)


// option_set_contains
//   trait: true iff the set has an option with key _Key.  Walks the
// flat (post-expansion) tuple.
template<typename _Set, auto _Key>
struct option_set_contains;

template<typename... _Options, auto _Key>
struct option_set_contains<option_set<_Options...>, _Key>
{
private:
    using flat = typename option_set<_Options...>::flat_options_t;

    template<typename _Tuple>
    struct apply;

    template<typename... _Opts>
    struct apply<std::tuple<_Opts...>>
        : std::integral_constant<bool,
            contains_key<_Key, _Opts...>::value>
    {};

public:
    static constexpr bool value = apply<flat>::value;
};

template<typename _Set, auto _Key>
inline constexpr bool option_set_contains_v =
    option_set_contains<_Set, _Key>::value;


// option_set_find
//   trait: yields the option with key _Key, or lookup_not_found.
template<typename _Set, auto _Key>
struct option_set_find;

template<typename... _Options, auto _Key>
struct option_set_find<option_set<_Options...>, _Key>
{
private:
    using flat = typename option_set<_Options...>::flat_options_t;

    template<typename _Tuple>
    struct apply;

    template<typename... _Opts>
    struct apply<std::tuple<_Opts...>>
    {
        using type  = find_by_key_t<_Key, _Opts...>;
        static constexpr bool        found = find_by_key<_Key, _Opts...>::found;
        static constexpr std::size_t index = find_by_key<_Key, _Opts...>::index;
    };

public:
    using type = typename apply<flat>::type;

    static constexpr bool        found = apply<flat>::found;
    static constexpr std::size_t index = apply<flat>::index;
};

template<typename _Set, auto _Key>
using option_set_find_t = typename option_set_find<_Set, _Key>::type;


// ===========================================================================
// VII. concepts   (C++20 analogs)
// ===========================================================================
//
//   Concept analogs of the query machinery above (folded in from the
// former option_set_concepts.hpp), compiled only where the toolchain
// provides concepts.  Pre-C++20 they are simply absent and the traits
// remain the portable path.

#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

// Keyed
//   concept: satisfied iff _Type exposes the keyed shape - a nested
// ::key_type alias and a static ::key member.  This is a standalone
// shape check: the old loose is_keyed_v contract has been retired, and
// option_set itself now requires the stricter is_option_v.  Keyed
// remains useful for constraining your own helpers against the bare key
// shape.
template<typename _Type>
concept Keyed = requires
{
    typename _Type::key_type;
    _Type::key;
};

// OptionSet
//   concept: satisfied iff _Type is some option_set<...> specialization.
// Parallels is_option_set_v.
template<typename _Type>
concept OptionSet = is_option_set_v<_Type>;

// OptionSetContains
//   concept: satisfied iff _Set is an option_set that contains the key
// _Key.  Parameterized over the NTTP key for use in requires-clauses.
// Parallels option_set_contains_v.
//
// Example:
//   template<typename _Set>
//     requires OptionSetContains<_Set, cli::verbose>
//   void enable_verbosity();
template<typename _Set,
         auto     _Key>
concept OptionSetContains =
    OptionSet<_Set> &&
    requires
    {
        requires option_set_contains_v<_Set, _Key>;
    };


// OptionSetFindable
//   concept: satisfied iff _Set is an option_set and the find trait
// reports `found` for _Key.  Functionally identical to
// OptionSetContains, but speaks in find vocabulary - useful where a
// downstream constraint wants a paired find_t<> alias to be meaningful.
template<typename _Set, 
         auto     _Key>
concept OptionSetFindable =
    OptionSet<_Set> &&
    requires
    {
        requires option_set_find<_Set, _Key>::found;
    };


// OptionSetNonEmpty
//   concept: satisfied iff _Set is a non-empty option_set.  Pairs
// naturally with option_set_key_type_t (which requires non-emptiness for
// its single-key-type extraction).
template<typename _Set>
concept OptionSetNonEmpty = OptionSet<_Set> &&
    requires
    {
        requires (_Set::size > 0);
    };

#endif  // C++20 concepts available


NS_END  // djinterp


#endif  // DJINTERP_OPTION_SET_