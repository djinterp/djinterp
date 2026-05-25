/******************************************************************************
* djinterp [options]                                static_option_set_traits.hpp
*
*   Trait integration for `static_option_set` and the auto-NTTP
* `static_options<...>` form.  Both resolve to the same underlying
* `static_option_set<_Entries...>` type, so one detection trait
* covers both spellings.
*
*   This header extends the existing option-set classification
* surface from `option_set_traits.hpp` rather than replacing any
* piece of it:
*
*   - `is_static_option_set<T>`     - identity detection for the new shape.
*   - `is_any_option_set<T>`        - aggregate: either dynamic
*                                     (option_set_like) or static
*                                     (static_option_set).
*   - `has_compile_time_keys<T>`    - true iff lookup is type-keyed
*                                     (`get<Key>()`), which is the
*                                     distinguishing structural signal
*                                     between static and dynamic forms.
*
*   The aggregate trait is what `option_layers` (next module) uses to
* decide which lookup path applies per layer.  Existing classification
* code in `option_set_concepts.hpp` keeps working unchanged.
*
*
* path:      /inc/djinterp/core/options/static_option_set_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.23
******************************************************************************/

#ifndef DJINTERP_STATIC_OPTION_SET_TRAITS_
#define DJINTERP_STATIC_OPTION_SET_TRAITS_ 1

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "../djinterp.hpp"
#include "../meta/type_traits.hpp"
#include "./option_set_traits.hpp"
#include "./static_option_set.hpp"


NS_DJINTERP


// ===========================================================================
// I.   Identity detection
// ===========================================================================

// is_static_option_set
//   trait: true iff `_Type` is a `static_option_set<_Entries...>`
// instantiation (after cv-ref stripping).  Identity check, not a
// structural probe — the structural form would be the
// `has_compile_time_keys<T>` test below.
template<typename _Type>
struct is_static_option_set : std::false_type
{};

template<typename... _Entries>
struct is_static_option_set<static_option_set<_Entries...>> : std::true_type
{};

// Re-route cv-ref qualified inputs through the unqualified probe.
// This is intentionally a separate partial specialization so that
// `is_static_option_set<const static_option_set<...>&>` also
// resolves correctly.
template<typename _Type>
struct is_static_option_set<const _Type>
    : is_static_option_set<_Type>
{};

template<typename _Type>
struct is_static_option_set<_Type&>
    : is_static_option_set<_Type>
{};

template<typename _Type>
struct is_static_option_set<_Type&&>
    : is_static_option_set<_Type>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool is_static_option_set_v =
        is_static_option_set<_Type>::value;
#endif


// ===========================================================================
// II.  Structural compile-time-keyed detection
// ===========================================================================
//   Where `is_static_option_set` is an identity check, the
// structural form detects ANY type exposing the compile-time
// type-keyed lookup surface — `template<typename K> get()` plus
// a `template<typename K> contains` boolean member.  Any future
// alternative implementation of the static shape (e.g. a tag-keyed
// constexpr array) would qualify without needing to inherit from
// or pretend to be a `static_option_set`.

NS_INTERNAL

    // has_typed_get
    //   helper: true if `_Type` has a `template<typename> get<K>()`
    // const member function callable with no arguments.  We probe
    // with a dummy key type that the static set is unlikely to
    // contain; the trait probes for the FUNCTION TEMPLATE rather
    // than a specific instantiation, so absence of the key for the
    // probe type isn't a problem (since template-name lookup
    // succeeds before substitution).
    struct probe_key_unlikely_to_exist {};

    template<typename _Type,
             typename = void>
    struct has_typed_get : std::false_type
    {};

    template<typename _Type>
    struct has_typed_get<_Type,
        void_t<decltype(
            std::declval<const _Type&>().template get<
                probe_key_unlikely_to_exist>())>>
        : std::true_type
    {};

    // has_typed_contains
    //   helper: true if `_Type` has a `template<typename> contains`
    // nested type carrying a `::value`.
    template<typename _Type,
             typename = void>
    struct has_typed_contains : std::false_type
    {};

    template<typename _Type>
    struct has_typed_contains<_Type,
        void_t<decltype(
            _Type::template contains<
                probe_key_unlikely_to_exist>::value)>>
        : std::true_type
    {};

NS_END  // internal

// has_compile_time_keys
//   trait: structural detection — true if `_Type` exposes the
// compile-time type-keyed lookup surface (`get<K>()` +
// `contains<K>::value`).  This is the distinguishing signal
// between the static and dynamic forms of option sets.
template<typename _Type>
struct has_compile_time_keys
    : std::integral_constant<bool,
        ( internal::has_typed_get<clean_t<_Type>>::value &&
          internal::has_typed_contains<clean_t<_Type>>::value )>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool has_compile_time_keys_v =
        has_compile_time_keys<_Type>::value;
#endif


// ===========================================================================
// III. Aggregate trait
// ===========================================================================

// is_any_option_set
//   trait: true if `_Type` is either a dynamic option_set-like
// type (per the existing `is_option_set_like` from
// `option_set_traits.hpp`) or a static_option_set / a structural
// equivalent (`has_compile_time_keys`).
//
//   This is the trait `option_layers` consults when deciding which
// lookup path applies for each layer.
template<typename _Type>
struct is_any_option_set
    : std::integral_constant<bool,
        ( is_option_set_like<clean_t<_Type>>::value ||
          is_static_option_set<clean_t<_Type>>::value ||
          has_compile_time_keys<clean_t<_Type>>::value )>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool is_any_option_set_v =
        is_any_option_set<_Type>::value;
#endif


// ===========================================================================
// IV.  Concept companions  (C++20)
// ===========================================================================

#if defined(__cpp_concepts) && (__cpp_concepts >= 201907L)

    // static_option_set_type
    //   concept: constrains `static_option_set<...>` instantiations.
    template<typename _Type>
    concept static_option_set_type =
        is_static_option_set<_Type>::value;

    // compile_time_keyed_option_set
    //   concept: constrains types exposing the compile-time
    // type-keyed lookup surface.
    template<typename _Type>
    concept compile_time_keyed_option_set =
        has_compile_time_keys<_Type>::value;

    // any_option_set_type
    //   concept: constrains types that are either dynamic or static
    // option-set shapes.
    template<typename _Type>
    concept any_option_set_type =
        is_any_option_set<_Type>::value;

#endif  // __cpp_concepts


NS_END  // djinterp


#endif  // DJINTERP_STATIC_OPTION_SET_TRAITS_
