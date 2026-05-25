/******************************************************************************
* djinterp [options]                                          option_traits.hpp
*
*   Tagless structural detection, form classification, and pack
* normalization for the options machinery.
*
*   STRUCTURAL CLASSIFIERS:
*   Five purely structural detectors over arbitrary types:
*
*     is_option              - is exactly option<_K, _V>
*     is_option_list         - is exactly option_list<...>
*     is_option_entry_like   - has .key, .value, key_type, value_type
*     is_option_set_like     - has key_type, mapped_type, value_type,
*                              find(_K), contains(_K)
*     is_option_container_like - iterable, value_type is entry-like,
*                              not set-like
*
*   No tags, no markers, no registration.  A type is what it structurally
* presents itself as.
*
*   FORM CLASSIFICATION:
*   `option_form` is a six-position enum classifying any type by its most
* specific recognized shape.  The priority lattice is:
*
*     1. option_list_form          (canonical pack -> flatten)
*     2. option_canonical_form     (canonical entry -> passthrough)
*     3. option_set_form           (set-like -> runtime carrier hint)
*     4. option_container_form     (container-of-entries -> runtime carrier hint)
*     5. option_entry_form         (entry-like -> extract types)
*     6. bare_key_form             (anything else -> claim next slot)
*
*   The bare_key_form is the negative-detection fallback that makes
* `K, V, K, V` syntax work without any key-tag concept.
*
*   NORMALIZER:
*   `normalize_options_t<_Options...>` walks any pack of mixed wire
* formats and produces a single `option_list<option<K,V>...>`.  It is
* the only entry point most users need.
*
*   QUERY TRAITS:
*   Recursive walks over option_list:
*
*     option_list_contains  - is a key present?
*     option_list_lookup    - get value type for a key (with default)
*     option_list_keys      - the option_list with values stripped
*
* DEPENDENCIES:
*   djinterp.hpp       - D_CONSTEXPR, void_t, clean_t, namespaces
*   type_traits.hpp    - portable type_traits
*   options.hpp        - option, option_list, list manipulation
*
*
* path:      /inc/djinterp/options/option_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.30
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Structural Classifiers
      1. is_option
      2. is_option_list
      3. is_iterable_for_options     (file-local helper)
      4. is_option_entry_like
      5. is_option_set_like
      6. is_option_container_like
II.   Form Classification
      1. option_form                 (enum)
      2. classify_option_form
III.  Normalizer
      1. normalize_options
IV.   Query Traits
      1. option_list_contains
      2. option_list_lookup
      3. option_list_keys
V.    Convenience _v / _t Aliases
*/

#ifndef DJINTERP_OPTION_TRAITS_
#define DJINTERP_OPTION_TRAITS_ 1

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"
#include "../meta/type_traits.hpp"
#include "./options.hpp"


NS_DJINTERP


// ===========================================================================
// I.   Structural Classifiers
// ===========================================================================
// All purely structural - no tags, no registration.  Each
// classifier asks one yes/no question about a type's shape.

// ---------------------------------------------------------------------------
// 1. is_option
// ---------------------------------------------------------------------------

// is_option
//   trait: true iff _Type is exactly option<_K, _V>.
template<typename _Type>
struct is_option : std::false_type
{};

template<typename _Key,
         typename _Value>
struct is_option<option<_Key, _Value>> : std::true_type
{};


// ---------------------------------------------------------------------------
// 2. is_option_list
// ---------------------------------------------------------------------------

// is_option_list
//   trait: true iff _Type is exactly option_list<...>.
template<typename _Type>
struct is_option_list : std::false_type
{};

template<typename... _Options>
struct is_option_list<option_list<_Options...>> : std::true_type
{};


// ---------------------------------------------------------------------------
// 3. is_iterable_for_options  (file-local helper)
// ---------------------------------------------------------------------------
// Purely structural check: does _Type expose .begin() and
// .end() callable on an lvalue?  This is the file-local
// iteration predicate the container-form classifier consults.
//
//   This helper is intentionally narrow - the framework's
// general-purpose iterable trait lives elsewhere; option_traits
// rolls its own to avoid a circular header dependency.

NS_INTERNAL

    template<typename _Type,
             typename = void>
    struct is_iterable_for_options : std::false_type
    {};

    template<typename _Type>
    struct is_iterable_for_options<_Type, void_t<
        decltype(std::declval<_Type&>().begin()),
        decltype(std::declval<_Type&>().end())
    >> : std::true_type
    {};

NS_END  // internal


// ---------------------------------------------------------------------------
// 4. is_option_entry_like
// ---------------------------------------------------------------------------

// is_option_entry_like
//   trait: true iff _Type exposes the entry surface:
//     - nested key_type
//     - nested value_type
//     - .key   member (any access path)
//     - .value member (any access path)
//
//   option_pair<K,V> satisfies this.  User-defined entry types
// satisfy it automatically by structural conformance.
template<typename _Type,
         typename = void>
struct is_option_entry_like : std::false_type
{};

template<typename _Type>
struct is_option_entry_like<_Type, void_t<
    typename clean_t<_Type>::key_type,
    typename clean_t<_Type>::value_type,
    decltype(std::declval<clean_t<_Type>&>().key),
    decltype(std::declval<clean_t<_Type>&>().value)
>> : std::true_type
{};


// ---------------------------------------------------------------------------
// 5. is_option_set_like
// ---------------------------------------------------------------------------

// is_option_set_like
//   trait: true iff _Type exposes the set surface:
//     - nested key_type, mapped_type, value_type
//     - find(const key_type&) callable on a const lvalue
//     - contains(const key_type&) callable on a const lvalue
//
//   option_set<K,V> satisfies this.  std::map and std::unordered_map
// also satisfy it (in C++20+ for contains; pre-C++20 they do not
// expose contains and so are classified as container_like below).
template<typename _Type,
         typename = void>
struct is_option_set_like : std::false_type
{};

template<typename _Type>
struct is_option_set_like<_Type, void_t<
    typename clean_t<_Type>::key_type,
    typename clean_t<_Type>::mapped_type,
    typename clean_t<_Type>::value_type,
    decltype(std::declval<const clean_t<_Type>&>().find(
        std::declval<const typename clean_t<_Type>::key_type&>())),
    decltype(std::declval<const clean_t<_Type>&>().contains(
        std::declval<const typename clean_t<_Type>::key_type&>()))
>> : std::true_type
{};


// ---------------------------------------------------------------------------
// 6. is_option_container_like
// ---------------------------------------------------------------------------

// is_option_container_like
//   trait: true iff _Type is iterable and its value_type is
// entry-like, but it is NOT set-like (no find/contains).
//
//   This catches std::vector<option_pair<K,V>>, plain arrays
// of entries (when wrapped in a span-like view), and similar
// flat sequences-of-entries.
NS_INTERNAL

    template<typename _Type,
             typename = void>
    struct is_option_container_like_helper : std::false_type
    {};

    template<typename _Type>
    struct is_option_container_like_helper<_Type, std::enable_if_t<
        ( is_iterable_for_options<_Type>::value  &&
          !is_option_set_like<_Type>::value      &&
          is_option_entry_like<
              typename clean_t<_Type>::value_type>::value )
    >> : std::true_type
    {};

NS_END  // internal

template<typename _Type>
struct is_option_container_like
    : internal::is_option_container_like_helper<_Type>
{};


// ===========================================================================
// II.  Form Classification
// ===========================================================================

// option_form
//   enum: classifies any type by its most-specific recognized
// shape.  Priority is highest at the top.
//
//   The bare_key_form is the negative-detection fallback that
// catches anything not matching the other five.  This is what
// makes the K,V,K,V wire format work tag-free.
enum class option_form
{
    // most specific - exact framework types
    option_list_form,
    option_canonical_form,

    // structural detection forms (high to low specificity)
    option_set_form,
    option_container_form,
    option_entry_form,

    // negative-detection fallback
    bare_key_form
};


// classify_option_form
//   trait: classifies _Type into an option_form.
//
//   Priority is enforced by short-circuit evaluation: the
// first classifier that matches wins.  A type satisfying both
// is_option_set_like and is_option_entry_like is classified
// as set (more specific surface).
template<typename _Type>
struct classify_option_form
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr option_form value =
          ( is_option_list<clean_type>::value )
              ? option_form::option_list_form
        : ( is_option<clean_type>::value )
              ? option_form::option_canonical_form
        : ( is_option_set_like<clean_type>::value )
              ? option_form::option_set_form
        : ( is_option_container_like<clean_type>::value )
              ? option_form::option_container_form
        : ( is_option_entry_like<clean_type>::value )
              ? option_form::option_entry_form
              : option_form::bare_key_form;
};


// ===========================================================================
// III. Normalizer
// ===========================================================================

// Recursive type-level state machine that consumes a pack of
// mixed wire formats and produces option_list<option<K,V>...>.
//   The dispatcher is a separate template specialized on
// option_form so each form's behavior is one isolated rule.

NS_INTERNAL

    // forward declaration of the public recursive helper
    template<typename... _Pack>
    struct normalize_helper;

    // ----------------------------------------------------------------
    // dispatcher - one specialization per form
    // ----------------------------------------------------------------

    template<option_form _Form,
             typename... _Pack>
    struct normalize_dispatch;

    // option_list_form: flatten the inner pack and recurse
    template<typename... _Inner,
             typename... _Rest>
    struct normalize_dispatch<option_form::option_list_form,
                              option_list<_Inner...>,
                              _Rest...>
    {
        using type = typename normalize_helper<_Inner..., _Rest...>::type;
    };

    // option_canonical_form: passthrough
    template<typename    _Key,
             typename    _Value,
             typename... _Rest>
    struct normalize_dispatch<option_form::option_canonical_form,
                              option<_Key, _Value>,
                              _Rest...>
    {
        using type = option_list_prepend_t<
            option<_Key, _Value>,
            typename normalize_helper<_Rest...>::type>;
    };

    // option_set_form: record runtime carrier hint
    template<typename    _Head,
             typename... _Rest>
    struct normalize_dispatch<option_form::option_set_form,
                              _Head,
                              _Rest...>
    {
        using type = option_list_prepend_t<
            option<runtime_options_carrier_key, clean_t<_Head>>,
            typename normalize_helper<_Rest...>::type>;
    };

    // option_container_form: same treatment as set_form
    template<typename    _Head,
             typename... _Rest>
    struct normalize_dispatch<option_form::option_container_form,
                              _Head,
                              _Rest...>
    {
        using type = option_list_prepend_t<
            option<runtime_options_carrier_key, clean_t<_Head>>,
            typename normalize_helper<_Rest...>::type>;
    };

    // option_entry_form: extract key_type and value_type
    template<typename    _Head,
             typename... _Rest>
    struct normalize_dispatch<option_form::option_entry_form,
                              _Head,
                              _Rest...>
    {
        using type = option_list_prepend_t<
            option<typename clean_t<_Head>::key_type,
                   typename clean_t<_Head>::value_type>,
            typename normalize_helper<_Rest...>::type>;
    };

    // bare_key_form (paired): claim next slot as value
    template<typename    _Key,
             typename    _Value,
             typename... _Rest>
    struct normalize_dispatch<option_form::bare_key_form,
                              _Key,
                              _Value,
                              _Rest...>
    {
        using type = option_list_prepend_t<
            option<_Key, _Value>,
            typename normalize_helper<_Rest...>::type>;
    };

    // bare_key_form (lone): trailing flag-style key
    template<typename _Key>
    struct normalize_dispatch<option_form::bare_key_form, _Key>
    {
        using type = option_list<option<_Key, std::true_type>>;
    };

    // ----------------------------------------------------------------
    // public recursive helper
    // ----------------------------------------------------------------

    // empty pack -> empty list
    template<typename... _Pack>
    struct normalize_helper
    {
        using type = option_list<>;
    };

    // non-empty pack -> classify head and dispatch
    template<typename    _Head,
             typename... _Rest>
    struct normalize_helper<_Head, _Rest...>
    {
    private:
        using clean_head = clean_t<_Head>;

    public:
        using type = typename normalize_dispatch<
            classify_option_form<clean_head>::value,
            clean_head,
            _Rest...>::type;
    };

NS_END  // internal


// normalize_options
//   trait: the public entry point.  Normalizes any pack of
// mixed wire formats into option_list<option<K,V>...>.
template<typename... _Options>
struct normalize_options
{
    using type =
        typename internal::normalize_helper<_Options...>::type;
};

// normalize_options_t
//   type: convenience alias for normalize_options<...>::type.
template<typename... _Options>
using normalize_options_t =
    typename normalize_options<_Options...>::type;


// ===========================================================================
// IV.  Query Traits
// ===========================================================================
// All queries operate on a normalized option_list.

// ---------------------------------------------------------------------------
// 1. option_list_contains
// ---------------------------------------------------------------------------

// option_list_contains
//   trait: true iff _List contains an option whose key_type
// matches _Key (using std::is_same).
template<typename _List,
         typename _Key>
struct option_list_contains : std::false_type
{};

template<typename    _Key,
         typename    _Head,
         typename... _Tail>
struct option_list_contains<option_list<_Head, _Tail...>, _Key>
{
    static constexpr bool value =
        ( std::is_same<typename _Head::key_type, _Key>::value ||
          option_list_contains<option_list<_Tail...>,
                               _Key>::value );
};


// ---------------------------------------------------------------------------
// 2. option_list_lookup
// ---------------------------------------------------------------------------

// option_list_lookup
//   trait: yields the value_type of the option whose key
// matches _Key.  Falls back to _Default if absent.
template<typename _List,
         typename _Key,
         typename _Default = void>
struct option_list_lookup
{
    using type = _Default;
};

template<typename    _Key,
         typename    _Default,
         typename    _Head,
         typename... _Tail>
struct option_list_lookup<option_list<_Head, _Tail...>,
                          _Key,
                          _Default>
{
    using type = std::conditional_t<
        std::is_same<typename _Head::key_type, _Key>::value,
        typename _Head::value_type,
        typename option_list_lookup<option_list<_Tail...>,
                                    _Key,
                                    _Default>::type>;
};

// option_list_lookup_t
//   type: convenience alias for option_list_lookup<...>::type.
template<typename _List,
         typename _Key,
         typename _Default = void>
using option_list_lookup_t = typename option_list_lookup<_List, _Key, _Default>::type;


// ---------------------------------------------------------------------------
// 3. option_list_keys
// ---------------------------------------------------------------------------

// option_list_keys
//   trait: produces an option_list whose entries' value_types
// have been replaced by void.  Useful when only key presence
// matters and downstream code wants a fixed shape.
NS_INTERNAL

    template<typename _Option>
    struct strip_value
    {
        using type = option<typename _Option::key_type, void>;
    };

NS_END  // internal

template<typename _List>
struct option_list_keys;

template<typename... _Options>
struct option_list_keys<option_list<_Options...>>
{
    using type = option_list<
        typename internal::strip_value<_Options>::type...>;
};

// option_list_keys_t
//   type: convenience alias for option_list_keys<...>::type.
template<typename _List>
using option_list_keys_t = typename option_list_keys<_List>::type;


// ===========================================================================
// V.   Convenience _v Aliases
// ===========================================================================

template<typename _Type>
inline constexpr bool is_option_v = is_option<_Type>::value;

template<typename _Type>
inline constexpr bool is_option_list_v = is_option_list<_Type>::value;

template<typename _Type>
inline constexpr bool is_option_entry_like_v = is_option_entry_like<_Type>::value;

template<typename _Type>
inline constexpr bool is_option_set_like_v = is_option_set_like<_Type>::value;

template<typename _Type>
inline constexpr bool is_option_container_like_v = is_option_container_like<_Type>::value;

template<typename _Type>
inline constexpr option_form classify_option_form_v = classify_option_form<_Type>::value;

template<typename _List,
         typename _Key>
inline constexpr bool option_list_contains_v = option_list_contains<_List, _Key>::value;


NS_END  // djinterp


#endif  // DJINTERP_OPTION_TRAITS_
