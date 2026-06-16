/******************************************************************************
* djinterp [meta]                                                  carrier.hpp
*
* Carriers - the dual-domain substrate:
*   This header introduces the substrate that lets a single combinator body run
* over runtime values, compile-time non-type values (NTTPs), and types alike.
* It is the foundation of the "two-lift" resolution.  C++ has three disjoint
* template parameter sorts (types, NTTPs, templates) with no kind spanning more
* than one, so a higher-order combinator written for the type world and the same
* combinator for the NTTP world are otherwise different, non-interacting code.
*
*   A *carrier* dissolves that split by collapsing both worlds toward ordinary
* objects:
*   - type_t<T> / type_c<T>   carry a *type*                as an empty object.
*   - val_t<V> / val<V>       carry a compile-time NTTP value as an empty object.
*   Runtime values are already objects and need no carrier.  Results are
* recovered structurally with decltype(x)::type (type domain) or
* decltype(x)::value (value domain), or via the carrier_type_t / carrier_value_v
* helpers below.
*
*   Because a carrier is a value, a forward-and-apply combinator (compose, curry,
* predicate, ...) written constexpr over carriers runs unchanged in all three
* domains.  Leaf operations stay domain-specific by design (add_pointer on a
* type, *2 on a number are genuinely different operations); only the spine
* unifies.
*
* TWO TIERS
*   type_t<T> (the type-carrier *type*) and the carrier-detection traits are
* C++11-clean and belong to the detection/trait floor.  The carrier *objects*
* (type_c, val_t, val) and the value-recovery helper require auto NTTPs and
* inline variables and are therefore gated to the C++17 lifted-functional floor.
* C++20 additionally exposes the concept faces (TypeCarrier / ValueCarrier /
* Carrier) when concepts are available.
*
* TRAITS / CONCEPTS PROVIDED
*   is_type_carrier<T>    - is T a type_t<...> specialization?  (C++11)
*   is_value_carrier<T>   - is T a val_t<...> specialization?   (C++17; else false)
*   is_carrier<T>         - is T either kind of carrier?        (C++11)
*   carrier_type_t<C>     - recover the carried type   (C::type).
*   carrier_value_v<C>    - recover the carried value  (C::value; C++17).
*   Carrier/TypeCarrier/  - C++20 concept faces mirroring the traits
*   ValueCarrier            (PascalCase per project convention).
*
* RELATED
*   The protocol concepts that *consume* carriers (Reducer, Transducer,
* UnfoldStep) live with the structural callable detectors in
* core/functional/structural_traits.hpp; the Carrier concept itself lives here.
*
* NOTE (Q2 extension point)
*   An optional `constant<V>` with overloaded constexpr operators (the Hana
* technique) could let a single runtime leaf such as [](auto x){ return x*2; }
* also drive value carriers, shrinking the value-domain leaf layer.  It is purely
* additive over val_t and is intentionally NOT defined here pending the Q2
* decision in the roadmap; the locked design (D5) keeps leaves domain-specific.
*
* path:      /inc/djinterp/core/meta/carrier.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.05
******************************************************************************/

#ifndef DJINTERP_META_CARRIER_
#define DJINTERP_META_CARRIER_ 1

// std
#include <type_traits>
// djinterp
#include "../djinterp.hpp"
#include "./trait_detect.hpp"   // D_TYPE_TRAIT_VALUE_BOOL (the canonical _v emitter)


NS_DJINTERP


// ================================================================
//  type carrier  (type_t / type_c)
// ================================================================

// type_t
//   type: carries a type as an ordinary (empty) object so a
// type-domain leaf or combinator can be invoked as `f(type_c<T>)`
// and yield another carrier.  Recover the carried type with
// `decltype(expr)::type` or `carrier_type_t<...>`.  C++11-clean.
template<typename _Type>
struct type_t
{
    using type = _Type;
};

// carrier_type_t
//   type: recovers the type carried by a type_t (or any carrier
// exposing a nested `type`).  cv-ref on the carrier is ignored.
template<typename _Carrier>
using carrier_type_t = typename clean_t<_Carrier>::type;


// ================================================================
//  carrier detection  (is_type_carrier / is_value_carrier / is_carrier)
// ================================================================

NS_INTERNAL

    // is_type_carrier_raw
    //   trait: primary template (failure case); cv-ref is already
    // stripped by the public face below.
    template<typename _Type>
    struct is_type_carrier_raw : std::false_type
    {};

    // is_type_carrier_raw (success case)
    //   trait: succeeds for a type_t<...> specialization.
    template<typename _Type>
    struct is_type_carrier_raw<type_t<_Type>> : std::true_type
    {};

    // is_value_carrier_raw
    //   trait: primary template (failure case).  Declared here, at the
    // C++11 floor, so is_value_carrier is well-formed even when the
    // value-carrier object layer (C++17) is guarded away; the val_t
    // success specialization is added inside the C++17 block below.
    template<typename _Type>
    struct is_value_carrier_raw : std::false_type
    {};

NS_END  // internal

// is_type_carrier
//   trait: detects whether _Type is a type-domain carrier, i.e. a
// specialization of type_t.  Specialization-based (not tagless), so
// detection is exact.  C++11-clean.
template<typename _Type>
struct is_type_carrier
    : internal::is_type_carrier_raw<clean_t<_Type>>
{};

// is_value_carrier
//   trait: detects whether _Type is a value-domain carrier, i.e. a
// specialization of val_t.  Always false at the C++11 floor (val_t is
// a C++17 facility).  Specialization-based, so detection is exact.
template<typename _Type>
struct is_value_carrier
    : internal::is_value_carrier_raw<clean_t<_Type>>
{};

// is_carrier
//   trait: detects whether _Type is either kind of carrier (type or
// value).  C++11-clean.
template<typename _Type>
struct is_carrier
    : std::integral_constant<bool,
        ( is_type_carrier<_Type>::value  ||
          is_value_carrier<_Type>::value )>
{};

// is_type_carrier_v / is_value_carrier_v / is_carrier_v
//   value: the `_v` companions, emitted via the canonical trait_detect
// macro so they auto-degrade with the language (inline variable on
// C++17+, plain variable template on C++14, absent on C++11 - where the
// `::value` member is still available).
D_TYPE_TRAIT_VALUE_BOOL(is_type_carrier)
D_TYPE_TRAIT_VALUE_BOOL(is_value_carrier)
D_TYPE_TRAIT_VALUE_BOOL(is_carrier)


// ================================================================
//  value carrier  (val_t / val)  +  carrier objects
// ================================================================
//   The value-domain carrier and ALL carrier objects (including the
// type-domain object type_c) require auto NTTPs and inline variables,
// so the whole object/value layer is gated to the C++17 lifted-
// functional floor.  At the C++11 detection floor only the type-carrier
// *type* (type_t) and the carrier-detection traits exist.

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    // type_c
    //   value: the canonical type-carrier object for _Type; pass it
    // into a type-domain leaf/combinator, e.g. `f(type_c<int>)`.
    template<typename _Type>
    inline constexpr type_t<_Type> type_c{};

    // val_t
    //   type: carries a compile-time NTTP value as an ordinary (empty)
    // object.  Recover it with `decltype(expr)::value` or
    // `carrier_value_v<...>`.
    template<auto _Value>
    struct val_t
    {
        static constexpr auto value = _Value;
    };

    // val
    //   value: the canonical value-carrier object for _Value; pass it
    // into a value-domain leaf/combinator, e.g. `f(val<10>)`.
    template<auto _Value>
    inline constexpr val_t<_Value> val{};

    // carrier_value_v
    //   value: recovers the value carried by a val_t (or any carrier
    // exposing a nested static `value`).  cv-ref on the carrier is
    // ignored.
    template<typename _Carrier>
    inline constexpr auto carrier_value_v = clean_t<_Carrier>::value;

    NS_INTERNAL

        // is_value_carrier_raw (success case)
        //   trait: succeeds for a val_t<...> specialization.  Completes
        // the primary declared at the C++11 floor above.
        template<auto _Value>
        struct is_value_carrier_raw<val_t<_Value>> : std::true_type
        {};

    NS_END  // internal

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


// ================================================================
//  concept faces
// ================================================================
//   C++20 concept faces mirroring the detection traits, for use in
// constrained combinators/drivers.  Guarded so the C++11 trait floor
// is unaffected.

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // TypeCarrier
    //   concept: satisfied by a type-domain carrier (type_t<...>).
    // PascalCase per the project's concept naming convention (cf.
    // passthrough.hpp's Passthrough), paralleling is_type_carrier_v.
    template<typename _Type>
    concept TypeCarrier = is_type_carrier<_Type>::value;

    // ValueCarrier
    //   concept: satisfied by a value-domain carrier (val_t<...>).
    template<typename _Type>
    concept ValueCarrier = is_value_carrier<_Type>::value;

    // Carrier
    //   concept: satisfied by either kind of carrier.
    template<typename _Type>
    concept Carrier = is_carrier<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_META_CARRIER_
