/******************************************************************************
* djinterp [option]                                        option_generator.hpp
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
* TABLE OF CONTENTS
* =================
* I.    SPEC CARRIERS            (kv_spec / k_spec - one per parsed token)
* II.   STREAM PARSE             (flat NTTP stream -> tuple of specs)
* III.  EMITTER                  (specs -> populated option_set)
* IV.   PUBLIC FRONT-END         (option_generator, option_set_t,
*                                 make_option_set)
*
*
* path:      /inc/djinterp/core/option/option_generator.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.25
******************************************************************************/

#ifndef DJINTERP_OPTION_GENERATOR_
#define DJINTERP_OPTION_GENERATOR_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"
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
    template<auto _Key,
             auto _Value>
    struct kv_spec
    {
        using option_t =
            option<_Key, field<std::remove_cvref_t<decltype(_Value)> > >;

        static D_CONSTEXPR auto
        initial()
        {
            return _Value;
        }
    };

    // k_spec
    //   carrier: a presence-only key.  unary_option<key> (a unit slot), seeded
    // with unit{}.
    template<auto _Key>
    struct k_spec
    {
        using option_t = unary_option<_Key>;

        static D_CONSTEXPR unit
        initial()
        {
            return unit{};
        }
    };


    // first_arg_type
    //   trait: the (decayed) type of the first NTTP - the inferred key type.
    template<auto _First,
             auto... _Rest>
    struct first_arg_type
    {
        using type = std::remove_cvref_t<decltype(_First)>;
    };


    // ===================================================================
    // II.  STREAM PARSE
    // ===================================================================
    //   Accumulate specs into a std::tuple.  Three productions, made mutually
    // exclusive by requires-clauses: (key, value), (key, key) -> unary, and a
    // trailing lone key -> unary.  A key whose type is not _KeyType matches no
    // production (a hard error), which enforces key uniformity.

    template<typename _KeyType,
             typename _Acc,
             auto...  _Args>
    struct parse_stream;

    // exhausted
    template<typename    _KeyType,
             typename... _Specs>
    struct parse_stream<_KeyType, std::tuple<_Specs...> >
    {
        using type = std::tuple<_Specs...>;
    };

    // trailing lone key -> unary
    template<typename    _KeyType,
             typename... _Specs,
             auto        _Key>
    struct parse_stream<_KeyType, std::tuple<_Specs...>, _Key>
    {
        using type = std::tuple<_Specs..., k_spec<_Key> >;
    };

    // key followed by a non-key value -> valued
    template<typename    _KeyType,
             typename... _Specs,
             auto        _Key,
             auto        _Value,
             auto...     _Rest>
        requires ( std::is_same_v<std::remove_cvref_t<decltype(_Key)>,   _KeyType> &&
                  !std::is_same_v<std::remove_cvref_t<decltype(_Value)>, _KeyType> )
    struct parse_stream<_KeyType, std::tuple<_Specs...>, _Key, _Value, _Rest...>
    {
        using type = typename parse_stream<
            _KeyType, std::tuple<_Specs..., kv_spec<_Key, _Value> >, _Rest...>::type;
    };

    // key followed by another key -> the first is unary
    template<typename    _KeyType,
             typename... _Specs,
             auto        _Key,
             auto        _Next,
             auto...     _Rest>
        requires ( std::is_same_v<std::remove_cvref_t<decltype(_Key)>,  _KeyType> &&
                   std::is_same_v<std::remove_cvref_t<decltype(_Next)>, _KeyType> )
    struct parse_stream<_KeyType, std::tuple<_Specs...>, _Key, _Next, _Rest...>
    {
        using type = typename parse_stream<
            _KeyType, std::tuple<_Specs..., k_spec<_Key> >, _Next, _Rest...>::type;
    };


    // ===================================================================
    // III. EMITTER
    // ===================================================================

    // emit_set
    //   trait: specs -> option_set< each spec's option_t > plus make(), which
    // constructs the set from the specs' initial values (in slot order,
    // matching option_set's values-constructor).
    template<typename _Specs>
    struct emit_set;

    template<typename... _Specs>
    struct emit_set<std::tuple<_Specs...> >
    {
        using type = option_set<typename _Specs::option_t...>;

        static D_CONSTEXPR type
        make()
        {
            return type( _Specs::initial()... );
        }
    };

NS_END  // internal


// ===========================================================================
// IV.  PUBLIC FRONT-END
// ===========================================================================

// option_generator
//   class: parse a flat key/value NTTP stream once and expose the resulting
// option_set type and a constructed instance.  _Args is the interleaved stream
// (key, value, key, value, unary_key, ...).
template<auto... _Args>
struct option_generator
{
    static_assert(sizeof...(_Args) > 0,
        "option_generator: the stream needs at least one key so the key type "
        "can be inferred from the first argument.");

    using key_type =
        typename internal::first_arg_type<_Args...>::type;

    using specs =
        typename internal::parse_stream<key_type, std::tuple<>, _Args...>::type;

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
template<auto... _Args>
using option_set_t = typename option_generator<_Args...>::type;

// make_option_set
//   function: build the option_set instance, seeded with the stream's values.
template<auto... _Args>
D_NODISCARD D_CONSTEXPR auto
make_option_set()
{
    return option_generator<_Args...>::make();
}


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER


#endif  // DJINTERP_OPTION_GENERATOR_
