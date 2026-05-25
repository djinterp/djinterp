/******************************************************************************
* djinterp [options]                                           options_bridge.hpp
*
*   Bridges between the three compile-time option representations:
*
*     1. `with_options_pack<KEY, VALUE, KEY, VALUE, ...>`
*          - flat KEY,VALUE,... type pack (table_common.hpp).
*          - used by `table<>`, `lookup_table<>`, `database_table<>`,
*            and any other host class accepting `_Options...`.
*
*     2. `option_list<option<K1,V1>, option<K2,V2>, ...>`
*          - canonical type-level list (options.hpp / option_traits.hpp).
*          - the normalized internal form that everything else lowers to.
*
*     3. `static_option_set<option_pair<K1,V1>, ...>` (constexpr instance)
*       / `static_options<K1{}, V1, K2{}, V2, ...>`  (auto-NTTP alias)
*          - value-level constexpr container (static_option_set.hpp).
*
*   The three forms carry the same information.  This header provides
* the conversions so a host class doesn't care which form a user supplies:
*
*     to_option_list_t<X>           — converts X to option_list<...>.
*                                      Works for any of the three forms.
*
*     to_with_options_pack_t<X>     — converts X to with_options_pack<...>.
*                                      Useful when host code wants the
*                                      flat KEY,VALUE pack form.
*
*     static_option_set_to_option_list_t<S>
*                                   — direct lowering for the static form.
*
*     option_list_to_static_option_set_t<L>
*                                   — reverse direction: produces a type
*                                      whose default-constructed instance
*                                      has the values from the option_list.
*                                      Note: only well-formed when every
*                                      value type in the list is default-
*                                      constructible.
*
*   These conversions are purely type-level; they do not move or copy
* runtime state.  For a `static_option_set` instance whose values you
* want to access at runtime, use the instance directly — the
* conversions here exist so host classes can pick the trait surface
* they're most comfortable with.
*
* DEPENDENCIES:
*   djinterp.hpp           - namespaces
*   options.hpp            - option, option_list
*   option_traits.hpp      - normalize_options_t (the canonical normalizer)
*   option_pair.hpp        - option_pair (for static→list)
*   static_option_set.hpp  - static_option_set, static_options
*   table_common.hpp       - with_options_pack (the flat-pack host form)
*
*
* path:      /inc/djinterp/core/options/options_bridge.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.23
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    static_option_set ↔ option_list
II.   option_list ↔ with_options_pack
III.  Universal to_option_list_t entry point
IV.   make_static_from_pack helper
*/

#ifndef DJINTERP_OPTIONS_BRIDGE_
#define DJINTERP_OPTIONS_BRIDGE_ 1

// std
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"
#include "./option_pair.hpp"
#include "./options.hpp"
#include "./option_traits.hpp"
#include "./static_option_set.hpp"


NS_DJINTERP


// ===========================================================================
// I.   static_option_set ↔ option_list
// ===========================================================================

NS_INTERNAL

    // entry_to_option
    //   trait: converts a single entry type (option_pair or
    // verified_option_pair) into a canonical `option<K, V>`.
    // The verifier column, if present, is dropped at the type
    // level — it's instance-state, not type-state.
    template<typename _Entry>
    struct entry_to_option
    {
        using type = option<typename _Entry::key_type,
                            typename _Entry::value_type>;
    };

    template<typename _Entry>
    using entry_to_option_t = typename entry_to_option<_Entry>::type;

NS_END  // internal


// static_option_set_to_option_list
//   trait: lowers a `static_option_set<E1, E2, ...>` (or any tuple of
// entries) into an `option_list<option<K1,V1>, option<K2,V2>, ...>`.
template<typename _StaticSet>
struct static_option_set_to_option_list;

template<typename... _Entries>
struct static_option_set_to_option_list<
    static_option_set<_Entries...>>
{
    using type = option_list<
        internal::entry_to_option_t<_Entries>...>;
};

template<typename _StaticSet>
using static_option_set_to_option_list_t =
    typename static_option_set_to_option_list<_StaticSet>::type;


// option_list_to_static_option_set
//   trait: lifts an `option_list<option<K1,V1>, ...>` into a
// `static_option_set<option_pair<K1,V1>, ...>` type.  Default-
// constructing an instance of the resulting type yields a
// static_option_set with default-constructed values for each key
// — only well-formed if every value type in the list is default-
// constructible.  For carrying actual values you'd construct the
// static_option_set directly via `make_static_option_set`.
template<typename _List>
struct option_list_to_static_option_set;

template<typename... _Options>
struct option_list_to_static_option_set<
    option_list<_Options...>>
{
    using type = static_option_set<
        option_pair<typename _Options::key_type,
                    typename _Options::value_type>...>;
};

template<typename _List>
using option_list_to_static_option_set_t =
    typename option_list_to_static_option_set<_List>::type;


// ===========================================================================
// II.  option_list ↔ with_options_pack
// ===========================================================================
//   `with_options_pack<KEY, VALUE, KEY, VALUE, ...>` is the flat-pack
// host form used by `table<>` and friends.  Its public surface
// (`option_t<Key, Default>`, `has_option<Key>::value`) is what host
// classes already consult; conversion to/from `option_list` is rare
// at the call site but useful inside trait machinery.

NS_INTERNAL

    // option_list_to_pack_unfold
    //   trait: unfolds an option_list<option<K,V>...> into a flat
    // type pack KEY, VALUE, KEY, VALUE, ... wrapped in
    // with_options_pack<...>.
    //
    // Implementation: a single partial specialization on
    // option_list<option<K,V>...> is enough because the inner
    // pack of option<...> can be expanded with two index
    // accesses each (key_type, value_type), which is what the
    // expansion below does.

    // build_pack_helper
    //   helper: takes an option_list and a partial flat-pack
    // accumulator; on each step it peels one option<K,V>, appends
    // K and V to the accumulator, and recurses.
    template<typename _List,
             typename... _Accum>
    struct build_pack_helper;

    // base case: list is empty, emit with_options_pack<accum...>
    template<typename... _Accum>
    struct build_pack_helper<option_list<>, _Accum...>
    {
        // forward-declared in table_common.hpp; uses ADL/lookup
        // at the use site.  We deliberately do not include
        // table_common.hpp here to avoid pulling the entire
        // table surface — callers that want this conversion
        // should include both.
        //
        // Result is delivered as a parameter pack so callers can
        // re-wrap it however they need (with_options_pack<...>,
        // a custom alias, etc.).  See `apply_pack_to` below.
        template<template<typename...> class _PackTemplate>
        using apply = _PackTemplate<_Accum...>;
    };

    // step: peel one option<K,V> from the list head
    template<typename    _Key,
             typename    _Value,
             typename... _Rest,
             typename... _Accum>
    struct build_pack_helper<
        option_list<option<_Key, _Value>, _Rest...>,
        _Accum...>
        : build_pack_helper<
            option_list<_Rest...>,
            _Accum..., _Key, _Value>
    {};

NS_END  // internal


// option_list_to_flat_pack_apply
//   trait: applies the unfolded flat KEY,VALUE,... pack to a
// user-supplied template.  Letting the caller name the wrapper
// avoids a hard dependency on `with_options_pack` (which lives
// in table_common.hpp and may not be desired by everyone).
//
// Example:
//   using as_pack =
//       option_list_to_flat_pack_apply_t<
//           my_list,
//           with_options_pack>;
template<typename                       _List,
         template<typename...> class    _PackTemplate>
struct option_list_to_flat_pack_apply
{
    using type =
        typename internal::build_pack_helper<_List>::
            template apply<_PackTemplate>;
};

template<typename                       _List,
         template<typename...> class    _PackTemplate>
using option_list_to_flat_pack_apply_t =
    typename option_list_to_flat_pack_apply<_List, _PackTemplate>::type;


// ===========================================================================
// III. Universal to_option_list_t entry point
// ===========================================================================

// to_option_list
//   trait: converts any of the recognized option carriers into a
// canonical `option_list<option<K,V>...>`.  Dispatches on the
// input shape:
//
//     - option_list<...>                 — identity
//     - static_option_set<...>           — drop verifiers, project
//                                          each entry to option<K,V>
//     - other forms (option_pair, ad-hoc
//       set-like, raw entry types)      — falls through to
//                                          normalize_options_t<X>
//
//   This is the recommended entry point for host code that wants
// the canonical form regardless of which wire format the user
// supplied.
template<typename _Input,
         typename = void>
struct to_option_list
{
    // fallback: feed through the canonical normalizer.
    using type = normalize_options_t<_Input>;
};

// identity case: already an option_list
template<typename... _Options>
struct to_option_list<option_list<_Options...>, void>
{
    using type = option_list<_Options...>;
};

// static_option_set case
template<typename... _Entries>
struct to_option_list<static_option_set<_Entries...>, void>
{
    using type =
        static_option_set_to_option_list_t<
            static_option_set<_Entries...>>;
};

template<typename _Input>
using to_option_list_t = typename to_option_list<_Input>::type;


// ===========================================================================
// IV.  make_static_from_pack helper
// ===========================================================================
//   Convenience runtime helper: takes a `with_options_pack<...>`-shaped
// flat KEY,VALUE,... type pack and produces a default-constructed
// `static_option_set` value with the matching entry types.  Only
// well-formed when every value type in the pack is default-
// constructible.

NS_INTERNAL

    // pack_to_static_set
    //   trait: walks a flat KEY,VALUE,KEY,VALUE,... pack and
    // accumulates into a static_option_set<option_pair<K,V>...>.
    template<typename _Acc,
             typename... _FlatPack>
    struct pack_to_static_set;

    // base case
    template<typename... _Entries>
    struct pack_to_static_set<static_option_set<_Entries...>>
    {
        using type = static_option_set<_Entries...>;
    };

    // step
    template<typename    _Key,
             typename    _Value,
             typename... _Rest,
             typename... _Entries>
    struct pack_to_static_set<
        static_option_set<_Entries...>,
        _Key, _Value, _Rest...>
        : pack_to_static_set<
            static_option_set<
                _Entries...,
                option_pair<_Key, _Value>>,
            _Rest...>
    {};

NS_END  // internal


// flat_pack_to_static_option_set
//   trait: lowers a flat KEY,VALUE,KEY,VALUE,... type pack into a
// `static_option_set<option_pair<K,V>...>` type.  The pack length
// must be even.
template<typename... _FlatPack>
struct flat_pack_to_static_option_set
{
    static_assert((sizeof...(_FlatPack) % 2) == 0,
                  "flat_pack_to_static_option_set: pack length "
                  "must be a multiple of 2 (KEY, VALUE, ...).");

    using type = typename internal::pack_to_static_set<
        static_option_set<>,
        _FlatPack...>::type;
};

template<typename... _FlatPack>
using flat_pack_to_static_option_set_t =
    typename flat_pack_to_static_option_set<_FlatPack...>::type;


NS_END  // djinterp


#endif  // DJINTERP_OPTIONS_BRIDGE_
