/*******************************************************************************
* djinterp [core]                                           option_generator.hpp
*
*   The one-statement authoring front-end for option_set: a flat interleaved
* NTTP stream of keys and values is parsed ONCE, at compile time, and emitted
* as a POPULATED option_set instance.  It is a construction layer on top of the
* engine (alongside option_builder / option_compose) and adds NO core machinery
* - the result is an ordinary option_set carrying field<T> slots.
*
*     using namespace djinterp;
*     //   width -> 1024, height -> 768, visible (UNARY - next token is a key),
*     //   name  -> "box"
*     auto cfg = make_option_set<
*         setting::width,  1024,
*         setting::height, 768,
*         setting::visible,                       // presence-only (no value)
*         setting::name,   fixed_string("box") >();
*     cfg.set<setting::width>(1280);              // mutable, exact-typed
*     // decltype(cfg) == option_set<
*     //     option<setting::width,  field<int>>,
*     //     option<setting::height, field<int>>,
*     //     unary_option<setting::visible>,
*     //     option<setting::name,   field<fixed_string<4>>> >
*
*   STRUCTURAL UNARY DETECTION.  Keys are values of the (single) key enum;
* everything else is a value, distinguished by type (decltype(arg) == KeyType).
* A key followed by a non-key is valued; a key followed by another key, or by
* the end of the stream, is UNARY.  No external "which keys are flags" predicate
* is maintained - the absence of a following value IS the signal.
*
*   The emitted option_set is the value-carrying face (option_set.hpp): field<T>
* options become typed runtime slots, unary keys become unit slots
* (contains<>-only).  key_type is inferred from each option's key.
*
*   Keys must be TYPE-distinguishable from values (the usual case: values are
* never of the key enum type).  If a value can itself be a key-enum value,
* position is ambiguous and the flat form is not applicable - use the explicit
* option<> spelling there.
*
*   C++20 (auto-NTTP pack, requires-distinguished partial specializations, the
* class-type NTTPs a string value is authored from); self-suppresses below it.
*
*
* path:      /inc/djinterp/core/option/option_generator.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.25
*                                                            revised: 2026.09.30
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    SPEC CARRIERS            (kv_spec / k_spec - one per parsed token)
      ------------------------------------------------------------------

II.   STREAM PARSE             (flat NTTP stream -> tuple of specs)
      -------------------------------------------------------------

III.  EMITTER                  (specs -> populated option_set)
      --------------------------------------------------------

IV.   PUBLIC FRONT-END         (option_generator, option_set_t,
      ---------------------------------------------------------

      make_option_set)
*/

#ifndef DJINTERP_OPTION_OPTION_GENERATOR_HPP
#define DJINTERP_OPTION_OPTION_GENERATOR_HPP 1

// djinterp
#include "../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: this header's floor

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "./option.hpp"        // option<>
#include "./option_set.hpp"    // option_set<> (value-carrying), field<>, unit, unary_option<>


#if D_ENV_LANG_IS_CPP20_OR_HIGHER


NS_DJINTERP


NS_INTERNAL

    // ===================================================================
    // I.   SPEC CARRIERS
    // ===================================================================
    //   One spec per parsed token.  Each yields the option<> it becomes and
    // the runtime value to seed its slot, so the emitter is a trivial pack-map
    // and the parse is the only place that reasons about key/value structure.

    // kv_spec
    //   carrier: a key bound to a value.  option<key, field<decltype V> >,
    // seeded with V.
    template<auto Key,
             auto Value>
    struct kv_spec
    {
        using option_t =
            option<Key, field<std::remove_cvref_t<decltype(Value)> > >;

        static D_CONSTEXPR auto
        initial()
        {
            return Value;
        }
    };

    // k_spec
    //   carrier: a presence-only key.  unary_option<key> (a unit slot), seeded
    // with unit{}.
    template<auto Key>
    struct k_spec
    {
        using option_t = unary_option<Key>;

        static D_CONSTEXPR unit
        initial()
        {
            return unit{};
        }
    };


    // first_arg_type
    //   trait: the (decayed) type of the first NTTP - the inferred key type.
    template<auto First,
             auto... Rest>
    struct first_arg_type
    {
        using type = std::remove_cvref_t<decltype(First)>;
    };


    // ===================================================================
    // II.  STREAM PARSE
    // ===================================================================
    //   Accumulate specs into a std::tuple.  Three productions, made mutually
    // exclusive by requires-clauses: (key, value), (key, key) -> unary, and a
    // trailing lone key -> unary.  A key whose type is not KeyType matches no
    // production (a hard error), which enforces key uniformity.

    template<typename KeyType,
             typename Acc,
             auto...  Args>
    struct parse_stream;

    // exhausted
    template<typename    KeyType,
             typename... Specs>
    struct parse_stream<KeyType, std::tuple<Specs...> >
    {
        using type = std::tuple<Specs...>;
    };

    // trailing lone key -> unary
    template<typename    KeyType,
             typename... Specs,
             auto        Key>
    struct parse_stream<KeyType, std::tuple<Specs...>, Key>
    {
        using type = std::tuple<Specs..., k_spec<Key> >;
    };

    // key followed by a non-key value -> valued
    template<typename    KeyType,
             typename... Specs,
             auto        Key,
             auto        Value,
             auto...     Rest>
        requires ( std::is_same_v<std::remove_cvref_t<decltype(Key)>,   KeyType> &&
                  !std::is_same_v<std::remove_cvref_t<decltype(Value)>, KeyType> )
    struct parse_stream<KeyType, std::tuple<Specs...>, Key, Value, Rest...>
    {
        using type = typename parse_stream<
            KeyType, std::tuple<Specs..., kv_spec<Key, Value> >, Rest...>::type;
    };

    // key followed by another key -> the first is unary
    template<typename    KeyType,
             typename... Specs,
             auto        Key,
             auto        Next,
             auto...     Rest>
        requires ( std::is_same_v<std::remove_cvref_t<decltype(Key)>,  KeyType> &&
                   std::is_same_v<std::remove_cvref_t<decltype(Next)>, KeyType> )
    struct parse_stream<KeyType, std::tuple<Specs...>, Key, Next, Rest...>
    {
        using type = typename parse_stream<
            KeyType, std::tuple<Specs..., k_spec<Key> >, Next, Rest...>::type;
    };


    // ===================================================================
    // III. EMITTER
    // ===================================================================

    // emit_set
    //   trait: specs -> option_set< each spec's option_t > plus make(), which
    // constructs the set from the specs' initial values (in slot order,
    // matching option_set's values-constructor).
    template<typename Specs>
    struct emit_set;

    template<typename... Specs>
    struct emit_set<std::tuple<Specs...> >
    {
        using type = option_set<typename Specs::option_t...>;

        static D_CONSTEXPR type
        make()
        {
            return type( Specs::initial()... );
        }
    };

NS_END  // internal


// ===========================================================================
// IV.  PUBLIC FRONT-END
// ===========================================================================

// option_generator
//   class: parse a flat key/value NTTP stream once and expose the resulting
// option_set type and a constructed instance.  Args is the interleaved stream
// (key, value, key, value, unary_key, ...).
template<auto... Args>
struct option_generator
{
    static_assert(sizeof...(Args) > 0,
        "option_generator: the stream needs at least one key so the key type "
        "can be inferred from the first argument.");

    using key_type =
        typename internal::first_arg_type<Args...>::type;

    using specs =
        typename internal::parse_stream<key_type, std::tuple<>, Args...>::type;

    // type: the populated option_set's TYPE - option_set< option<key,
    // field<T>>..., unary_option<unary_key>... >.
    using type = typename internal::emit_set<specs>::type;

    static D_CONSTEXPR type
    make()
    {
        return internal::emit_set<specs>::make();
    }
};


// option_set_t
//   type: the option_set a flat stream produces (the type face).
template<auto... Args>
using option_set_t = typename option_generator<Args...>::type;

// make_option_set
//   function: build the option_set instance, seeded with the stream's values.
template<auto... Args>
D_NODISCARD D_CONSTEXPR auto
make_option_set()
{
    return option_generator<Args...>::make();
}


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_OPTION_OPTION_GENERATOR_HPP
