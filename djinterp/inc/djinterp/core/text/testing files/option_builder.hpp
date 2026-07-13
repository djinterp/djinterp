/******************************************************************************
* djinterp [option]                                         option_builder.hpp
*
*   The flat-schema-to-option_set pipeline.  Takes a flat typename pack
* of key carriers + opaque args (with optional passthroughs sitting
* between chunks of length _N), and yields an option_set<...>.
*
*   PIPELINE:
*     1. `partition_wrap_except` (meta/dtuple_wrap_partition.hpp)
*        groups the flat schema into chunks of exactly _N consecutive
*        non-passthrough types, wrapping each chunk in
*        `option_partition_wrap` (which extracts ::value from the
*        first slot as the option's key NTTP, then keeps the
*        remaining slots as the option's opaque args).  Passthroughs
*        (types satisfying `is_passthrough`) sit at chunk boundaries
*        unwrapped.
*
*     2. `tuple_to_option_set` lifts the resulting std::tuple<...>
*        into an option_set<...>.
*
*     3. option_set's own strict contract (`is_option_v` on every
*        entry of the flattened tuple, after expansion through
*        ::expanded_t) validates the final result.
*
*   THE FIRST SLOT IS A KEY CARRIER:
*   By the option_partition_wrap contract, the first slot of every
* chunk must expose a static constexpr ::value member.  The framework
* provides `key_carrier<auto>` (option_factory.hpp) for this; any
* user-defined type with the same shape works just as well.
*
*   THE REMAINING SLOTS ARE OPAQUE:
*   The framework imposes nothing on the args that follow the key.
* They are stored verbatim in the option<>.  Any meaning - default
* values, descriptions, verifiers, etc. - is the domain of the
* schema's owner, not of the options subframework.
*
*
* path:      /inc/djinterp/core/option/option_builder.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.27
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    option_partition_wrap       (typename-only wrapper for partition)
II.   tuple_to_option_set         (tuple-of-entries -> option_set)
III.  option_set_from_flat_t      (end-to-end pipeline alias)
*/

#ifndef DJINTERP_OPTION_BUILDER_
#define DJINTERP_OPTION_BUILDER_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
// djinterp
#include "../djinterp.hpp"
#include "../meta/tuple_partition.hpp"
#include "../meta/passthrough.hpp"
#include "./option.hpp"
#include "./option_set.hpp"


NS_DJINTERP

// ===========================================================================
// I.   option_partition_wrap
// ===========================================================================

NS_INTERNAL

    // has_value_member
    //   helper: SFINAE probe for a static constexpr ::value
    // member.  Used to validate the first slot of every
    // partition chunk before forwarding to option<>.
    template<typename _Type,
             typename = void>
    struct has_value_member : std::false_type
    {};

    template<typename _Type>
    struct has_value_member<_Type, std::void_t<decltype(_Type::value)>>
        : std::true_type
    {};


    // option_partition_wrap_impl
    //   helper: rebuild as option<_Key::value, _Rest...>.
    // Errors with a clear diagnostic if the first slot does
    // not expose a static constexpr ::value member.  No tag
    // semantics are imposed on _Rest...; the engine simply
    // hands them to option<> verbatim.
    template<typename... _Args>
    struct option_partition_wrap_impl;

    template<typename    _Key,
             typename... _Rest>
    struct option_partition_wrap_impl<_Key, _Rest...>
    {
        static_assert(has_value_member<_Key>::value,
            "option_partition_wrap: the first slot of every "
            "chunk in a flat schema must be a key carrier - a "
            "type exposing a static constexpr ::value member.  "
            "Use `key_carrier<auto>` (option_factory.hpp) or "
            "any user-defined type with the same shape.");

        using type = option<_Key::value, _Rest...>;
    };

NS_END  // internal


// option_partition_wrap
//   alias: typename-only wrapper for `partition_wrap_except`.
// Treats its first parameter as a key carrier (must expose
// ::value), the rest as opaque option args, and yields
// option<_Key::value, _Rest...>.
//
//   This is the wrapper passed to partition_wrap_except's
// _Wrap slot, NOT a tag-detection facility.  The framework
// never inspects what's stored in _Rest...
template<typename... _Args>
using option_partition_wrap = typename internal::option_partition_wrap_impl<_Args...>::type;


// ===========================================================================
// II.  tuple_to_option_set
// ===========================================================================

// tuple_to_option_set
//   trait: lift std::tuple<_Entries...> into
// option_set<_Entries...>.
//
//   The tuple may contain a mix of option<>s and passthrough
// markers; option_set itself runs the strict-option contract
// after expanding every entry through ::expanded_t (so
// passthroughs with `using expanded_t = std::tuple<>` flatten
// to nothing and pass the contract trivially).
template<typename _Tuple>
struct tuple_to_option_set;

template<typename... _Entries>
struct tuple_to_option_set<std::tuple<_Entries...>>
{
    using type = option_set<_Entries...>;
};

template<typename _Tuple>
using tuple_to_option_set_t =
    typename tuple_to_option_set<_Tuple>::type;


// ===========================================================================
// III. option_set_from_flat_t
// ===========================================================================

// option_set_from_flat_t
//   alias: end-to-end pipeline.  Partitions _Schema... into
// chunks of exactly _N consecutive non-passthrough types,
// wraps each chunk in an option<>, leaves passthroughs at
// their original positions, and lifts the result into an
// option_set.
//
//   _Schema can be a flat typename pack OR a std::tuple<...>
// (the underlying partition trait accepts both via partial
// specialization).
//
//   Slot semantics are entirely the schema author's domain.
// The framework guarantees only that:
//     - chunks of length _N become option<> instantiations,
//       with the first slot's ::value used as the key NTTP;
//     - passthroughs survive the partition unchanged;
//     - the resulting option_set's contract (is_option_v on
//       every flattened entry) is enforced.
//
// Example (_N = 3 in the schema author's project; the slots
// after the key carry user-defined semantics):
//
//   using my_options = option_set_from_flat_t<3,
//       key_carrier<my_enum::a>, slot_def<5>,    desc<"A">,
//       key_carrier<my_enum::b>, slot_def<true>, desc<"B">>;
//
//   // == option_set<
//   //        option<my_enum::a, slot_def<5>,    desc<"A">>,
//   //        option<my_enum::b, slot_def<true>, desc<"B">>>
//
//   `slot_def<>` and `desc<>` here are USER-DEFINED in the
// schema's owning project; the framework treats them as
// opaque storage and never looks at them.
template<std::size_t _N,
         typename... _Schema>
using option_set_from_flat_t =
    tuple_to_option_set_t<
        partition_wrap_except_t<
            option_partition_wrap,
            _N,
            is_passthrough,
            _Schema...
        >
    >;


NS_END  // djinterp


#endif  // DJINTERP_OPTION_BUILDER_
