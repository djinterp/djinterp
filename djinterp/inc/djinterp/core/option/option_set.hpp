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
V.    runtime map + dispatch     (option_set<Key,Value> + public option_set)
VI.   queries                    (is_option_set, key_type, contains, find)
VII.  concepts                   (C++20 analogs)
*/

#ifndef DJINTERP_OPTION_SET_
#define DJINTERP_OPTION_SET_ 1

// std
#include <cstddef>
#include <tuple>
#include <vector>
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
// V.   runtime key->value map  +  public option_set dispatch
// ===========================================================================
//
//   The pack form above (internal::option_set_pack) is the COMPILE-TIME,
// type-level option_set: a variadic list of option<> types exposing
// size / empty / flat_options_t / option_at.
//
//   Below, the PUBLIC option_set name dispatches by shape:
//     - option_set<_Key, _Value> where _Key is a runtime key (enum or
//       integral, not an option<>)  ->  runtime key->value map
//       (insert / insert_or_assign / at / find / end / erase / ...).
//     - everything else (any arity, or a 2-arg pack of option<>s)  ->
//       the type-level pack form.
//
//   VERSION PORTABILITY: the runtime map's own code is C++11-grade - it
// keys on a plain typename, with no auto NTTP, variable templates, or fold
// expressions of its own.  The header overall is C++17 (the option<> /
// pack forms use an auto NTTP), but nothing in the runtime map adds a
// C++17-only requirement; extracted on its own it compiles to C++11.

NS_INTERNAL

    // os_is_runtime_key
    //   trait: true iff _Type is a runtime key (enum or integral) and not an
    // option<>.  Routes a two-arg option_set<_Key,_Value> to the runtime
    // map rather than the type-level pack.
    template<typename _Type>
    struct os_is_runtime_key
        : std::integral_constant<bool,
            ( ( std::is_enum<_Type>::value || 
                std::is_integral<_Type>::value ) &&
              ( !is_option_v<_Type> ) )>
    {};


    // option_set_map
    //   the runtime key->value map base.  An ordered-vector map: linear
    // find, stable storage, value semantics.  The element type exposes
    // public `.key` / `.value` members so callers can write `it->value`.
    template<typename _Key,
             typename _Value>
    struct option_set_map
    {
        // entry: stored key/value pair.
        struct entry
        {
            _Key   key;
            _Value value;

            entry() : key(), value() 
            {}

            entry(
                const _Key&   _k, 
                const _Value& _v
            ) : key(_k), value(_v)
            {}

            entry(
                const _Key& _k,
                _Value&&    _v
            ) : key(_k), value(static_cast<_Value&&>(_v)) 
            {}
        };

        typedef _Key                                        key_type;
        typedef _Value                                      mapped_type;
        typedef entry                                       value_type;
        typedef std::size_t                                 size_type;
        typedef typename std::vector<entry>::iterator       iterator;
        typedef typename std::vector<entry>::const_iterator const_iterator;

        // ---- capacity ----
        size_type size()  const D_NOEXCEPT { return m_entries.size(); }
        bool      empty() const D_NOEXCEPT { return m_entries.empty(); }
        void      clear()                  { m_entries.clear(); }

        // ---- iterators ----
        iterator       begin() D_NOEXCEPT       { return m_entries.begin(); }
        iterator       end()   D_NOEXCEPT       { return m_entries.end(); }
        const_iterator begin() const D_NOEXCEPT { return m_entries.begin(); }
        const_iterator end()   const D_NOEXCEPT { return m_entries.end(); }

        // ---- lookup ----
        iterator find(const _Key& _key)
        {
            iterator it = m_entries.begin();
            for (; it != m_entries.end(); ++it)
            {
                if (it->key == _key) { return it; }
            }
            return m_entries.end();
        }
        const_iterator find(const _Key& _key) const
        {
            const_iterator it = m_entries.begin();
            for (; it != m_entries.end(); ++it)
            {
                if (it->key == _key) { return it; }
            }
            return m_entries.end();
        }
        bool contains(const _Key& _key) const
        {
            return find(_key) != end();
        }

        // at: reference to the mapped value for _key.  Precondition: the
        //   key is present (documented UB-if-absent contract, matching the
        //   test option helpers).
        _Value& at(const _Key& _key)
        {
            return find(_key)->value;
        }
        const _Value& at(const _Key& _key) const
        {
            return find(_key)->value;
        }

        // ---- modifiers ----
        // insert: adds (key,value) if absent.  Returns true if inserted,
        //   false if the key already existed (no overwrite).
        bool insert(const _Key& _key, const _Value& _value)
        {
            if (contains(_key)) { return false; }
            m_entries.push_back(entry(_key, _value));
            return true;
        }
        bool insert(const _Key& _key, _Value&& _value)
        {
            if (contains(_key)) { return false; }
            m_entries.push_back(entry(_key, static_cast<_Value&&>(_value)));
            return true;
        }

        // insert_or_assign: sets the value for _key, overwriting if present.
        //   Returns true if a new key was inserted, false if an existing
        // value was overwritten (parity with std::map::insert_or_assign).
        bool insert_or_assign(const _Key& _key, const _Value& _value)
        {
            iterator it = find(_key);
            if (it != end()) { it->value = _value; return false; }
            m_entries.push_back(entry(_key, _value));
            return true;
        }
        bool insert_or_assign(const _Key& _key, _Value&& _value)
        {
            iterator it = find(_key);
            if (it != end())
            {
                it->value = static_cast<_Value&&>(_value);
                return false;
            }
            m_entries.push_back(entry(_key, static_cast<_Value&&>(_value)));
            return true;
        }

        // erase: removes _key if present; returns true if removed.
        bool erase(const _Key& _key)
        {
            iterator it = find(_key);
            if (it == end()) { return false; }
            m_entries.erase(it);
            return true;
        }

    private:
        std::vector<entry> m_entries;
    };

NS_END  // internal


// option_set  (public primary: the type-level pack form)
//   Inherits the full type-level API from internal::option_set_pack for
// every instantiation EXCEPT the two-arg runtime-keyed case specialized
// below.
template<typename... _Entries>
struct option_set
    : internal::option_set_pack<_Entries...>
{};

// option_set<_Key, _Value>  (two-arg dispatch)
//   For exactly two type arguments, route by the nature of _Key: a runtime
// key (enum / integral, not an option<>) selects the runtime map; anything
// else (e.g. a 2-entry pack of option<>s) keeps the type-level pack.
template<typename _A,
         typename _B>
struct option_set<_A, _B>
    : std::conditional<
          internal::os_is_runtime_key<_A>::value,
          internal::option_set_map<_A, _B>,
          internal::option_set_pack<_A, _B>
      >::type
{};


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