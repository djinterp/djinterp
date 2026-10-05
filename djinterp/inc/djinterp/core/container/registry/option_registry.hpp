/*******************************************************************************
* djinterp [core]                                            option_registry.hpp
*
*   The OPTION <-> REGISTRY bridge. Not "a registry can be built from options"
* (though it can) but options as a MUTATION LANGUAGE over an existing
* registry:
* a single option, an option (sub)set, a runtime key/value, or a pair all
* drive
* the same primitive.  The governing idea --
*
*       AN OPTION IS SHORTHAND FOR (lookup by key) + (set value in its
*     column).
*
* An option<Key, ...> names WHERE (its key selects the row via the registry's
* key lookup) and WHAT (its value, placed in that row's value column -- the
* column ValueProj designates, never the key column).  Applying an option is
* therefore exactly `locate(key)` then `set_value(value)`, inserting the entry
* when the key is absent so an option ESTABLISHES a setting rather than merely
* overwriting one.
*
*   THE SEVERAL FORMS (all reduce to that one primitive):
*     apply_option(reg, key, value)          runtime key + value.
*     apply_option(reg, kv_pair)             a runtime pair.
*     apply_option(reg, option<K, arg>{}) a compile-time option; its key
*   becomes the
*                                     runtime lookup key and arg0::value the
*                                     runtime value (unary / field options
*                                   with
*                                     no compile-time value degrade to
*                                   key-only,
*                                     ensuring the entry is present).
*     apply_all(reg, opt, ...)        a pack of options / pairs, in order.
*     apply_all(reg, option_set) a whole set / subset: each option applied
*                                     -- compile-time values everywhere, plus,
*                                     under C++20, the set instance's runtime
*                                     field<T> values read through get<>.
*     reg << opt      / reg <<= opt   the stream-style idiom for the same, so
*                                     options "flow into" a registry and
*                                   chain.
*
*   COLUMN SAFETY, FOR FREE. The value is always written through
* set()/update(),
* which target the value column and hold the key const -- so an option can
* never
* rewrite a key. For a WHOLE-RECORD registry (value projection = whole_record,
* a tuple whose columns share one record), place_at<J> directs a value into
* column J of the located row THROUGH THE UPDATE FOLD: land it in a value
* column
* and it commits; land it on the key column and the rekey strategy (reject /
* reposition) catches it.  No placement bypasses the guard.
*
*   DOMAIN. apply_option()/the operators target a registry with a DISTINCT
* value column
* (kv rows or projected-key records -- Reg::value_disjoint); there "set the
* value" is well defined.  For a whole-record registry, drive edits with
* place_at<J> (an explicit column) or the registry's own update() family.
*
*   REUSE.  This header adds no lookup or mutation of its own: it is a thin
* materialisation + dispatch layer over registry.hpp (contains / index_of /
* set / insert / update) and reads options through option.hpp / option_set.hpp
* (::key, args, flat_options_t, option_field_t, get<>).
*
*   PORTABILITY:
*   C++17 for the compile-time-value and runtime key/value forms.  Reading a
* value-carrying option_set INSTANCE's runtime field<T> slots needs
* option_set's
* get<>, which is C++20; below that, such options apply as key-only.
*
*
* path:      /inc/djinterp/core/container/registry/option_registry.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.11
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    materialisation traits   (is_kv_pair, option_key_v, option_ct_value)
      --------------------------------------------------------------------

II.   apply outcome            (option_apply_status / option_apply_result)
      --------------------------------------------------------------------

III.  apply_option -- primitive  (key+value / pair / option / key-only)
      -----------------------------------------------------------------

IV.   apply_all -- sets/packs    (option pack, option_set fold)
      ---------------------------------------------------------

V.    place_at<J>              (whole-record column-directed placement)
      -----------------------------------------------------------------

VI.   operators                (reg << opt, reg <<= opt)
      --------------------------------------------------
*/

#ifndef DJINTERP_CONTAINER_REGISTRY_OPTION_REGISTRY_HPP
#define DJINTERP_CONTAINER_REGISTRY_OPTION_REGISTRY_HPP 1

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
#include "../../../djinterp.hpp"          // NS_*, D_NODISCARD, clean_t, feature gates
#include "./registry.hpp"              // registry, is_registry, kv_pair
#include "../../option/option.hpp"     // option<>, is_option_v
#include "../../option/option_set.hpp" // option_set<>, option_field_t, unit, field<>


NS_DJINTERP


// ===========================================================================
// I.   materialisation traits
// ===========================================================================
//   Reading an option down to a runtime (key, value): the key is the option's
// NTTP key; the value is, by the framework's own convention (container_options'
// option_enum_value), the first arg's static ::value.  Args are opaque, so this
// is a POLICY -- specialise option_ct_value for an arg shape that carries its
// value differently.

// is_kv_pair
//   trait: true iff Type is a kv_pair<K, V> (the runtime pair form).
template<typename Type>
struct is_kv_pair : std::false_type
{};

// is_kv_pair<kv_pair<Key, Value>>
//   trait: the `kv_pair<Key, Value>` case; it reports true.
template<typename Key,
         typename Value>
struct is_kv_pair<kv_pair<Key, Value>> : std::true_type
{};

template<typename Type>
inline constexpr bool is_kv_pair_v = is_kv_pair<clean_t<Type>>::value;


// option_key_v
//   value: an option's key as a runtime value (its NTTP key). Usable as the
// registry lookup key when the registry's key_type accepts the option
// key_type.
template<typename Option>
inline constexpr auto option_key_v = clean_t<Option>::key;


// option_ct_value
//   trait: the compile-time value an option carries, if any. ::has is true
// when the option's first arg exposes a static ::value (an integral_constant,
// a val_t<>, an enum-valued constant); ::value is then that value. A unary
// option (no args) or a field<T> option (runtime value, no ::value) yields
// ::has == false -- the caller then treats the option as key-only.
NS_INTERNAL

    template<typename Option,
             typename = void>
    struct option_ct_value_impl
    {
        static constexpr bool has = false;
    };

    template<typename Option>
    struct option_ct_value_impl<
        Option,
        void_t<decltype(std::tuple_element_t<0, typename Option::args_type>::value)>>
    {
        static constexpr bool has   = true;
        static constexpr auto value =
            std::tuple_element_t<0, typename Option::args_type>::value;
    };

NS_END  // internal

template<typename Option>
struct option_ct_value : internal::option_ct_value_impl<clean_t<Option>>
{};


// ===========================================================================
// II.  apply outcome
// ===========================================================================

// option_apply_status
//   enum: how applying one option resolved.
//     updated -- the key was present; its value column was written.
//     inserted -- the key was absent; a new entry was established.
//     located -- key-only application; the key was present (nothing written).
//     absent -- the key was absent and could not be established (no runtime
//                 value, or the record is not constructible from key + value).
enum class option_apply_status
{
    updated,
    inserted,
    located,
    absent
};

// option_apply_result
//   struct: the status plus the entry's index afterwards (npos when absent).
template<typename SizeType = std::size_t>
struct option_apply_result
{
    option_apply_status status;
    SizeType            index;
};


// ===========================================================================
// III. apply_option -- the primitive
// ===========================================================================

// apply_option (key, value)
//   THE primitive: locate _key in _reg and place _value in its value column
// (set(), which targets ValueProj and holds the key const). When _key is
// absent and _insert_if_absent, establish the entry -- but only when the
// record is constructible from key + value (kv rows, key/value tuples); a
// wider record cannot be synthesised from a key/value alone and is left for
// update()/insert() of a full record. Targets a distinct-value-column
// registry.
template<typename Reg,
         typename Key,
         typename Value>
option_apply_result<typename Reg::size_type>
apply_option(
    Reg&         _reg,
    const Key&   _key,
    const Value& _value,
    bool          _insert_if_absent = true
)
{
    static_assert(is_registry_v<Reg>,
        "apply: first argument must be a registry.");
    static_assert(Reg::value_disjoint,
        "apply_option(key, value): the registry must have a distinct value column "
        "(a kv or projected-key registry).  For a whole-record registry, "
        "direct a column with place_at<J> or drive update() directly.");

    using key_type    = typename Reg::key_type;
    using record_type = typename Reg::record_type;
    using size_type   = typename Reg::size_type;

    const key_type k = static_cast<key_type>(_key);

    // present -> write the value column in place (key-const)
    if (_reg.contains(k))
    {
        _reg.set(k, static_cast<typename Reg::mapped_type>(_value));

        return option_apply_result<size_type>{
            option_apply_status::updated, _reg.index_of(k) };
    }

    // absent -> establish the entry when the record is (key, value)-shaped
    if (_insert_if_absent)
    {
        if constexpr (std::is_constructible<record_type, key_type, Value>::value)
        {
            const size_type pos = _reg.insert(k, _value);

            return option_apply_result<size_type>{
                option_apply_status::inserted, pos };
        }
    }

    return option_apply_result<size_type>{
        option_apply_status::absent, _reg.npos };
}


// apply_key (key-only)
//   ensure _key is present; when absent and permitted, establish it with a
// value-initialised value (requires a default value and a (key, value)-shaped
// record). The meaning of applying a unary / presence option.
template<typename Reg,
         typename Key>
option_apply_result<typename Reg::size_type>
apply_key(
    Reg&       _reg,
    const Key& _key,
    bool        _insert_if_absent = true
)
{
    static_assert(is_registry_v<Reg>,
        "apply_key: first argument must be a registry.");

    using key_type    = typename Reg::key_type;
    using record_type = typename Reg::record_type;
    using mapped_type = typename Reg::mapped_type;
    using size_type   = typename Reg::size_type;

    const key_type k = static_cast<key_type>(_key);

    if (_reg.contains(k))
    {
        return option_apply_result<size_type>{
            option_apply_status::located, _reg.index_of(k) };
    }

    if (_insert_if_absent)
    {
        if constexpr (std::is_constructible<record_type,
                                            key_type, mapped_type>::value &&
                      std::is_default_constructible<mapped_type>::value)
        {
            const size_type pos = _reg.insert(k, mapped_type{});

            return option_apply_result<size_type>{
                option_apply_status::inserted, pos };
        }
    }

    return option_apply_result<size_type>{
        option_apply_status::absent, _reg.npos };
}


// apply_option (kv_pair)
//   the runtime pair form -- unpack and place.
template<typename Reg,
         typename Key,
         typename Value>
option_apply_result<typename Reg::size_type>
apply_option(
    Reg&                       _reg,
    const kv_pair<Key, Value>& _pair,
    bool                        _insert_if_absent = true
)
{
    return apply_option(_reg, _pair.m_key, _pair.m_value, _insert_if_absent);
}


// apply_option (option)
//   the compile-time option form. Materialise: key = option key, value =
// arg0::value when present; otherwise apply key-only (unary / field option).
template<typename    Reg,
         auto        Key,
         typename... Args>
option_apply_result<typename Reg::size_type>
apply_option(
    Reg&                  _reg,
    option<Key, Args...> /*_option*/,
    bool                   _insert_if_absent = true
)
{
    using opt_type = option<Key, Args...>;

    if constexpr (option_ct_value<opt_type>::has)
    {
        return apply_option(_reg, Key, option_ct_value<opt_type>::value,
                     _insert_if_absent);
    }
    else
    {
        // no compile-time value: a presence / runtime-field option -> key-only
        return apply_key(_reg, Key, _insert_if_absent);
    }
}


// ===========================================================================
// IV.  apply_all -- sets / packs
// ===========================================================================

NS_INTERNAL

    // apply_one_from_set
    //   apply the flat option Opt drawn from a set instance _set: its
    // compile-time value when it has one; else, under C++20, its runtime field
    // value via get<>; else key-only.
    template<typename Reg,
             typename Opt,
             typename Set>
    void
    apply_one_from_set(
        Reg&       _reg,
        const Set& _set,
        bool        _insert_if_absent
    )
    {
        constexpr auto key = Opt::key;

        if constexpr (option_ct_value<Opt>::has)
        {
            (void)_set;
            djinterp::apply_option(_reg, key, option_ct_value<Opt>::value,
                            _insert_if_absent);
        }
        else if constexpr (!std::is_same<option_field_t<Opt>, unit>::value)
        {
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
            // value-carrying instance: read the runtime slot addressed by key
            djinterp::apply_option(_reg, key, _set.template get<key>(),
                            _insert_if_absent);
#else
            // pre-C++20: no runtime value surface -> ensure presence only
            (void)_set;
            djinterp::apply_key(_reg, key, _insert_if_absent);
#endif
        }
        else
        {
            (void)_set;
            djinterp::apply_key(_reg, key, _insert_if_absent);
        }
    }

    // apply_set_impl
    //   fold apply_one_from_set over the set's flat option positions.
    template<typename Reg,
             typename Set,
             std::size_t... I>
    void
    apply_set_impl(
        Reg&       _reg,
        const Set& _set,
        bool        _insert_if_absent,
        std::index_sequence<I...> /*seq*/
    )
    {
        ( apply_one_from_set<
              Reg,
              typename Set::template option_at<I>,
              Set>(_reg, _set, _insert_if_absent),
          ... );
    }

NS_END  // internal


// apply_all (option_set)
//   apply a whole set / subset: every option placed under its key. Returns the
// registry for chaining.
template<typename    Reg,
         typename... Options>
Reg&
apply_all(
    Reg&                         _reg,
    const option_set<Options...>& _set,
    bool                          _insert_if_absent = true
)
{
    static_assert(is_registry_v<Reg>,
        "apply_all: first argument must be a registry.");

    internal::apply_set_impl(
        _reg, _set, _insert_if_absent,
        std::make_index_sequence<option_set<Options...>::size>{});

    return _reg;
}


// apply_all (pack)
//   apply a pack of options / pairs in order. Returns the registry for
// chaining. (An option_set argument routes to the overload above, which is
// more specialised.)
template<typename    Reg,
         typename... Items>
Reg&
apply_all(
    Reg&          _reg,
    const Items&... _items
)
{
    static_assert(is_registry_v<Reg>,
        "apply_all: first argument must be a registry.");

    ( (void)apply_option(_reg, _items), ... );

    return _reg;
}


// ===========================================================================
// V.   place_at<J>
// ===========================================================================

// place_at
//   direct _value into COLUMN J of the row keyed by _key, for a whole-record
// registry (value projection = whole_record). Routed through update(), so the
// key stays guarded: a value column commits; the key column trips the rekey
// strategy (Policy, defaulting to the registry's). For a
// distinct-value-column registry, "the value column" is already the target --
// use apply_option() instead.
template<std::size_t J,
         typename    Policy = void,
         typename    Reg,
         typename    Key,
         typename    Value>
update_result
place_at(
    Reg&         _reg,
    const Key&   _key,
    const Value& _value
)
{
    static_assert(is_registry_v<Reg>,
        "place_at: first argument must be a registry.");
    static_assert(!Reg::value_disjoint,
        "place_at<J>: intended for a whole-record registry (value projection = "
        "whole_record), where a value shares the record's tuple with the key.  "
        "For a distinct value column, use apply_option().");

    using key_type    = typename Reg::key_type;
    using record_type = typename Reg::record_type;

    // default the rekey policy to the registry's when none is named
    using policy = std::conditional_t<std::is_same<Policy, void>::value,
                                      typename Reg::rekey_policy,
                                      Policy>;

    return _reg.template update<policy>(
        static_cast<key_type>(_key),
        [&_value](record_type& _rec)
        {
            std::get<J>(_rec) = _value;
        });
}


// ===========================================================================
// VI.  operators   (stream-style apply idiom)
// ===========================================================================
//   `reg << thing` applies thing to reg and returns reg, so options flow in and
// chain: reg << opt_a << opt_b << a_subset.  `reg <<= thing` is the compound
// form.  Both mutate reg.  Constrained to a registry LHS so the operators never
// intrude on unrelated types.

// reg << option
template<typename Reg,
         auto     Key,
         typename... Args,
         typename std::enable_if<is_registry_v<Reg>, int>::type = 0>
Reg&
operator<<(
    Reg&                  _reg,
    option<Key, Args...> _option
)
{
    apply_option(_reg, _option);

    return _reg;
}

// reg << kv_pair
template<typename Reg,
         typename Key,
         typename Value,
         typename std::enable_if<is_registry_v<Reg>, int>::type = 0>
Reg&
operator<<(
    Reg&                       _reg,
    const kv_pair<Key, Value>& _pair
)
{
    apply_option(_reg, _pair);

    return _reg;
}

// reg << option_set
template<typename Reg,
         typename... Options,
         typename std::enable_if<is_registry_v<Reg>, int>::type = 0>
Reg&
operator<<(
    Reg&                         _reg,
    const option_set<Options...>& _set
)
{
    apply_all(_reg, _set);

    return _reg;
}

// reg <<= option
template<typename Reg,
         auto     Key,
         typename... Args,
         typename std::enable_if<is_registry_v<Reg>, int>::type = 0>
Reg&
operator<<=(
    Reg&                  _reg,
    option<Key, Args...> _option
)
{
    apply_option(_reg, _option);

    return _reg;
}

// reg <<= kv_pair
template<typename Reg,
         typename Key,
         typename Value,
         typename std::enable_if<is_registry_v<Reg>, int>::type = 0>
Reg&
operator<<=(
    Reg&                       _reg,
    const kv_pair<Key, Value>& _pair
)
{
    apply_option(_reg, _pair);

    return _reg;
}

// reg <<= option_set
template<typename Reg,
         typename... Options,
         typename std::enable_if<is_registry_v<Reg>, int>::type = 0>
Reg&
operator<<=(
    Reg&                         _reg,
    const option_set<Options...>& _set
)
{
    apply_all(_reg, _set);

    return _reg;
}


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_REGISTRY_OPTION_REGISTRY_HPP
