/******************************************************************************
* djinterp [options]                                    option_set_traits.hpp
*
* SFINAE-based traits for option_set containers and options-configurable
* target types.
*   Provides compile-time detection and classification of option_set
* interface compliance, value homogeneity queries, and target-level
* configurability.  Entry-level traits (has_key, has_value, etc.) are
* defined in option_pair_traits.hpp and re-exported here via the include.
*
*   All detection is purely structural: no tag types are required.
* Container types declare capabilities through method and alias presence.
* Target types declare configurability through method presence -
* apply_options, configure, set_option, to_options, parse_args, etc.
*
*   Uses the void_t / detector SFINAE idiom from type_traits.hpp.
*
*
* TABLE OF CONTENTS
* =================
* I.      CONTAINER-LEVEL EXPRESSION ALIASES
*
* II.     CONTAINER-LEVEL has_* TRAITS
*
* III.    COMPOSITE CONTAINER TRAITS
*         1.  is_option_set               (minimum contract)
*         2.  is_iterable_option_set      (begin + end)
*         3.  is_searchable_option_set    (find + contains)
*         4.  is_static_option_set        (has backing_array)
*         5.  is_dynamic_option_set       (no backing_array)
*         6.  is_sorted_option_set        (has binary search)
*
* IV.     VALUE HOMOGENEITY TRAITS
*         1.  has_homogeneous_values      (mapped_type is not any-like)
*         2.  has_heterogeneous_values    (mapped_type is any-like)
*         3.  has_mapped_type_of          (mapped_type is exactly _V)
*         4.  all_values_are_arithmetic   (mapped arithmetic)
*         5.  all_values_are_string       (mapped string-like)
*         6.  all_values_are_boolean      (mapped bool)
*
* V.      TARGET-LEVEL EXPRESSION ALIASES
*         A.  unparameterized (single _Type)
*             1.  to_options_method_expr
*             2.  get_options_method_expr
*             3.  help_string_method_expr
*             4.  parse_args_method_expr
*             5.  parse_option_method_expr
*             6.  parse_config_method_expr
*         B.  parameterized (multi-type)
*             7.  apply_options_method_expr<_Type, _OptionSet>
*             8.  configure_method_expr<_Type, _OptionSet>
*             9.  set_option_method_expr<_Type, _Key, _Value>
*         C.  default options
*             10. default_options_static_call_expr
*             11. default_options_static_member_expr
*             12. default_options_const_method_expr
*             13. default_options_method_expr
*
* VI.     TARGET-LEVEL has_* TRAITS
*         (unparameterized, parameterized, default options)
*
* VII.    CONFIGURE STRATEGY
*         1.  configure_strategy    (enum)
*         2.  option_configure_strategy   (trait)
*
* VIII.   COMPOSITE TARGET TRAITS
*         1.  is_options_configurable     (can accept options)
*         2.  is_option_producing         (can export options)
*         3.  is_option_parseable         (can parse CLI/config)
*         4.  is_option_describable       (has help_string)
*         5.  has_default_options         (any default path)
*         6.  has_runtime_default_options (const or mutable method)
*         7.  has_constexpr_default_options (static path)
*         8.  is_option_set_constructible (ctor from option_set)
*         9.  is_option_list_constructible (ctor from init list)
*         10. is_options_constructible    (either ctor path)
*
* IX.     STATIC OPTION SET CLASS IDENTITY
*         1.  is_static_option_set_class  (identity check for the
*                                          static_option_set<E...>
*                                          class template — distinct
*                                          from the structural
*                                          is_static_option_set above)
*         2.  has_compile_time_keys       (structural: exposes typed
*                                          get<Key>() / contains<Key>)
*         3.  is_any_option_set           (aggregate over structural
*                                          dynamic + static class
*                                          + compile-time-keyed)
*
* X.      STATIC OPTION SET CONSTRUCTIBILITY
*         1.  is_static_option_set_constructible
*                                          (ctor from specific static set)
*         2.  is_any_static_option_set_constructible
*                                          (template ctor over any
*                                          static_option_set)
*         3.  has_from_static_options_factory
*                                          (static factory method)
*         4.  has_apply_static_options_method
*                                          (instance apply method)
*         5.  is_static_default_constructible
*                                          (all entries default-ctible)
*         6.  has_verifier_entries         (any verified_option_pair)
*         7.  is_options_constructible_any (dynamic or static aggregate)
*
* XI.     CONVENIENCE ALIASES
*
*
* path:      /inc/options/option_set_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.06
******************************************************************************/

#ifndef DJINTERP_OPTION_SET_TRAITS_
#define DJINTERP_OPTION_SET_TRAITS_ 1

// std
#include <cstddef>
#include <initializer_list>
#include <string>
#include <type_traits>
// djinterp
#include "../djinterp.hpp"
#include "../meta/type_traits.hpp"
#include "./option_pair_traits.hpp"

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #include <string_view>
#endif


NS_DJINTERP

// forward-declare option_set for constructibility traits.
// Full definition is in option_set.hpp (included before this
// header by the options.hpp umbrella).
template<typename _Key,
         typename _Value,
         bool     _Sorted>
class option_set;

// forward-declare static_option_set for class-identity and
// constructibility traits (sections IX and X).  Full definition
// is in static_option_set.hpp.
template<typename... _Entries>
class static_option_set;

// forward-declare verified_option_pair for the verifier-entry
// detection trait (section X).  Full definition is in
// option_pair.hpp.
template<typename _Key,
         typename _Value,
         typename _Verifier>
struct verified_option_pair;


///////////////////////////////////////////////////////////////////////////////
///        I.    CONTAINER-LEVEL EXPRESSION ALIASES                         ///
///////////////////////////////////////////////////////////////////////////////
// Detects whether a container type exposes the interface
// expected of an option_set: nested type aliases, lookup
// methods, iteration, and optional features.  Each expression
// is probed in a void_t context.

NS_INTERNAL

    // ----------------------------------------------------------------
    // nested type aliases
    // ----------------------------------------------------------------

    // key_type_alias_expr
    //   detector: expression alias for ::key_type.
    template<typename _Type>
    using key_type_alias_expr = typename _Type::key_type;

    // mapped_type_alias_expr
    //   detector: expression alias for ::mapped_type.
    template<typename _Type>
    using mapped_type_alias_expr = typename _Type::mapped_type;

    // value_type_alias_expr
    //   detector: expression alias for ::value_type
    // (the pair type in option_set).
    template<typename _Type>
    using value_type_alias_expr = typename _Type::value_type;

    // extent_expr
    //   detector: expression alias for ::extent static member.
    template<typename _Type>
    using extent_expr = decltype(_Type::extent);

    // ----------------------------------------------------------------
    // lookup methods
    // ----------------------------------------------------------------

    // find_method_expr
    //   detector: expression alias for .find(key).
    template<typename _Type>
    using find_method_expr = decltype(
        std::declval<const _Type&>().find(
            std::declval<
                const typename _Type::key_type&>()));

    // contains_method_expr
    //   detector: expression alias for .contains(key).
    template<typename _Type>
    using contains_method_expr = decltype(
        std::declval<const _Type&>().contains(
            std::declval<
                const typename _Type::key_type&>()));

    // value_or_method_expr
    //   detector: expression alias for
    // .value_or(key, fallback).
    template<typename _Type>
    using value_or_method_expr = decltype(
        std::declval<const _Type&>().value_or(
            std::declval<
                const typename _Type::key_type&>(),
            std::declval<
                const typename _Type::mapped_type&>()));

    // ----------------------------------------------------------------
    // capacity methods
    // ----------------------------------------------------------------

    // size_method_expr
    //   detector: expression alias for .size().
    template<typename _Type>
    using size_method_expr =
        decltype(std::declval<const _Type&>().size());

    // empty_method_expr
    //   detector: expression alias for .empty().
    template<typename _Type>
    using empty_method_expr =
        decltype(std::declval<const _Type&>().empty());

    // ----------------------------------------------------------------
    // iteration methods
    // ----------------------------------------------------------------

    // begin_method_expr
    //   detector: expression alias for .begin().
    template<typename _Type>
    using begin_method_expr =
        decltype(std::declval<const _Type&>().begin());

    // end_method_expr
    //   detector: expression alias for .end().
    template<typename _Type>
    using end_method_expr =
        decltype(std::declval<const _Type&>().end());

    // ----------------------------------------------------------------
    // optional features
    // ----------------------------------------------------------------

    // backing_array_method_expr
    //   detector: expression alias for .backing_array().
    // Present only on static-extent option_sets.
    template<typename _Type>
    using backing_array_method_expr =
        decltype(std::declval<const _Type&>().backing_array());

    // count_if_method_expr
    //   detector: expression alias for .count_if(pred).
    template<typename _Type>
    using count_if_method_expr = decltype(
        std::declval<const _Type&>().count_if(
            std::declval<bool(*)(
                const typename _Type::value_type&)>()));

    // data_method_expr
    //   detector: expression alias for .data() -> pointer.
    template<typename _Type>
    using data_method_expr =
        decltype(std::declval<const _Type&>().data());

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///        II.   CONTAINER-LEVEL has_* TRAITS                               ///
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------
// nested type aliases
// ----------------------------------------------------------------

// has_key_type_alias
//   trait: detects whether _Type has a ::key_type nested type.
template<typename _Type>
struct has_key_type_alias
{
    static constexpr bool value =
        is_detected<internal::key_type_alias_expr, _Type>::value;
};

// has_mapped_type_alias
//   trait: detects whether _Type has a ::mapped_type nested type.
template<typename _Type>
struct has_mapped_type_alias
{
    static constexpr bool value =
        is_detected<internal::mapped_type_alias_expr, _Type>::value;
};

// has_value_type_alias
//   trait: detects whether _Type has a ::value_type nested type.
template<typename _Type>
struct has_value_type_alias
{
    static constexpr bool value =
        is_detected<
            internal::value_type_alias_expr, _Type>::value;
};

// has_extent
//   trait: detects whether _Type has a ::extent static member.
template<typename _Type>
struct has_extent
{
    static constexpr bool value =
        is_detected<internal::extent_expr, _Type>::value;
};

// ----------------------------------------------------------------
// lookup methods
// ----------------------------------------------------------------

// has_find_method
//   trait: detects whether _Type has a .find(key) method.
template<typename _Type>
struct has_find_method
{
    static constexpr bool value =
        is_detected<internal::find_method_expr, _Type>::value;
};

// has_contains_method
//   trait: detects whether _Type has a .contains(key) method.
template<typename _Type>
struct has_contains_method
{
    static constexpr bool value =
        is_detected<
            internal::contains_method_expr, _Type>::value;
};

// has_value_or_method
//   trait: detects whether _Type has a .value_or(key, fallback)
// method.
template<typename _Type>
struct has_value_or_method
{
    static constexpr bool value =
        is_detected<
            internal::value_or_method_expr, _Type>::value;
};

// ----------------------------------------------------------------
// capacity methods
// ----------------------------------------------------------------

// has_size_accessor
//   trait: detects whether _Type has a .size() method.
template<typename _Type>
struct has_size_accessor
{
    static constexpr bool value =
        is_detected<internal::size_method_expr, _Type>::value;
};

// has_empty_method
//   trait: detects whether _Type has an .empty() method.
template<typename _Type>
struct has_empty_method
{
    static constexpr bool value =
        is_detected<internal::empty_method_expr, _Type>::value;
};

// ----------------------------------------------------------------
// iteration methods
// ----------------------------------------------------------------

// has_begin_method
//   trait: detects whether _Type has a .begin() method.
template<typename _Type>
struct has_begin_method
{
    static constexpr bool value =
        is_detected<internal::begin_method_expr, _Type>::value;
};

// has_end_method
//   trait: detects whether _Type has an .end() method.
template<typename _Type>
struct has_end_method
{
    static constexpr bool value =
        is_detected<internal::end_method_expr, _Type>::value;
};

// ----------------------------------------------------------------
// optional features
// ----------------------------------------------------------------

// has_backing_array
//   trait: detects whether _Type has a .backing_array() method.
template<typename _Type>
struct has_backing_array
{
    static constexpr bool value =
        is_detected<
            internal::backing_array_method_expr, _Type>::value;
};

// has_count_if_method
//   trait: detects whether _Type has a .count_if(pred) method.
template<typename _Type>
struct has_count_if_method
{
    static constexpr bool value =
        is_detected<
            internal::count_if_method_expr, _Type>::value;
};

// has_data_method
//   trait: detects whether _Type has a .data() method.
template<typename _Type>
struct has_data_method
{
    static constexpr bool value =
        is_detected<internal::data_method_expr, _Type>::value;
};


///////////////////////////////////////////////////////////////////////////////
///        III.  COMPOSITE CONTAINER TRAITS                                 ///
///////////////////////////////////////////////////////////////////////////////

// is_option_set
//   trait: true when _Type satisfies the minimum option_set
// container contract: exposes ::key_type, supports keyed
// lookup via .find and .contains, reports size via .size,
// and is iterable via .begin/.end.
template<typename _Type>
struct is_option_set
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool value =
        ( has_key_type_alias<clean_type>::value    &&
          has_find_method<clean_type>::value       &&
          has_contains_method<clean_type>::value   &&
          has_size_accessor<clean_type>::value       &&
          has_begin_method<clean_type>::value      &&
          has_end_method<clean_type>::value );
};

// is_iterable_option_set
//   trait: true when _Type supports range-based iteration
// via .begin() and .end().
template<typename _Type>
struct is_iterable_option_set
{
    static constexpr bool value =
        ( has_begin_method<_Type>::value &&
          has_end_method<_Type>::value );
};

// is_searchable_option_set
//   trait: true when _Type supports keyed lookup via .find()
// and .contains().
template<typename _Type>
struct is_searchable_option_set
{
    static constexpr bool value =
        ( has_find_method<_Type>::value &&
          has_contains_method<_Type>::value );
};

// is_static_option_set
//   trait: true when _Type satisfies the option_set contract
// and exposes .backing_array(), indicating static-extent
// (compile-time-sized) storage.
template<typename _Type>
struct is_static_option_set
{
    static constexpr bool value =
        ( is_option_set<_Type>::value &&
          has_backing_array<_Type>::value );
};

// is_dynamic_option_set
//   trait: true when _Type satisfies the option_set contract
// but does NOT expose .backing_array(), indicating heap-
// backed dynamic-extent storage.
template<typename _Type>
struct is_dynamic_option_set
{
    static constexpr bool value =
        ( is_option_set<_Type>::value &&
          !has_backing_array<_Type>::value );
};

// is_sorted_option_set
//   trait: true when _Type satisfies the option_set contract
// and its key type supports operator<, which is a
// prerequisite for the binary-search lookup path.
// Note: this is a necessary condition, not a guarantee
// that the instance was constructed with _Sorted=true.
// Runtime sorted state is not detectable at compile time
// without a tag; this trait confirms sortability.
template<typename _Type>
struct is_sorted_option_set
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool value =
        ( is_option_set<clean_type>::value        &&
          has_key_type_alias<clean_type>::value    &&
          is_detected<internal::less_than_expr,
              typename clean_type::key_type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///        IV.   VALUE HOMOGENEITY TRAITS                                   ///
///////////////////////////////////////////////////////////////////////////////
// Classifies an option_set based on whether its mapped value
// type is a concrete type (homogeneous) or a type-erased
// container (heterogeneous).  Useful for choosing between
// compile-time-dispatched and runtime-dispatched value access.

NS_INTERNAL

    // mapped_type_or_void
    //   helper: resolves to the mapped_type of _Type when
    // present, or void when absent.
    template<typename _Type,
             bool = is_detected<mapped_type_alias_expr,
                                _Type>::value>
    struct mapped_type_or_void
    {
        using type = void;
    };

    template<typename _Type>
    struct mapped_type_or_void<_Type, true>
    {
        using type = typename _Type::mapped_type;
    };

NS_END  // internal

// has_homogeneous_values
//   trait: true when _Type exposes a ::mapped_type that is NOT
// an any-like type-erased container.  When true, every entry
// in the set holds a value of the same concrete type and
// typed access is safe without runtime dispatch.
template<typename _Type>
struct has_homogeneous_values
{
private:
    using mapped =
        typename internal::mapped_type_or_void<_Type>::type;

public:
    static constexpr bool value =
        ( has_mapped_type_alias<_Type>::value &&
          !is_detected<internal::any_holds_expr,
                       mapped>::value );
};

// has_heterogeneous_values
//   trait: true when _Type exposes a ::mapped_type that IS an
// any-like type-erased container.  When true, individual
// entries may hold different runtime types and typed access
// requires holds<T>() / get<T>() dispatch.
template<typename _Type>
struct has_heterogeneous_values
{
private:
    using mapped =
        typename internal::mapped_type_or_void<_Type>::type;

public:
    static constexpr bool value =
        ( has_mapped_type_alias<_Type>::value &&
          is_detected<internal::any_holds_expr,
                      mapped>::value );
};

// has_mapped_type_of
//   trait: true when _Type has a ::mapped_type that is exactly
// _V (after stripping cv-qualifiers).  Useful for checking
// whether an option_set stores a specific value type.
//
// Example:
//   has_mapped_type_of<my_set, int>::value
template<typename _Type,
         typename _V>
struct has_mapped_type_of
{
private:
    using mapped =
        typename internal::mapped_type_or_void<_Type>::type;

public:
    static constexpr bool value =
        ( has_mapped_type_alias<_Type>::value &&
          std::is_same<
              typename std::remove_cv<mapped>::type,
              typename std::remove_cv<_V>::type>::value );
};

// all_values_are_arithmetic
//   trait: true when _Type is a homogeneous option_set whose
// mapped_type is an arithmetic type (integral or floating-
// point).  This is a compile-time property of the set's
// declared type, not a runtime scan.
template<typename _Type>
struct all_values_are_arithmetic
{
private:
    using mapped =
        typename internal::mapped_type_or_void<_Type>::type;

public:
    static constexpr bool value =
        ( has_homogeneous_values<_Type>::value &&
          std::is_arithmetic<mapped>::value );
};

// all_values_are_string
//   trait: true when _Type is a homogeneous option_set whose
// mapped_type is string-like (std::string, const char*,
// std::string_view).
template<typename _Type>
struct all_values_are_string
{
private:
    using mapped =
        typename internal::mapped_type_or_void<_Type>::type;

public:
    static constexpr bool value =
        ( has_homogeneous_values<_Type>::value &&
          internal::string_like_check<mapped>::value );
};

// all_values_are_boolean
//   trait: true when _Type is a homogeneous option_set whose
// mapped_type is bool.
template<typename _Type>
struct all_values_are_boolean
{
private:
    using mapped =
        typename internal::mapped_type_or_void<_Type>::type;

public:
    static constexpr bool value =
        ( has_homogeneous_values<_Type>::value &&
          std::is_same<mapped, bool>::value );
};

// all_values_are_enum
//   trait: true when _Type is a homogeneous option_set whose
// mapped_type is an enumeration type.
template<typename _Type>
struct all_values_are_enum
{
private:
    using mapped =
        typename internal::mapped_type_or_void<_Type>::type;

public:
    static constexpr bool value =
        ( has_homogeneous_values<_Type>::value &&
          std::is_enum<mapped>::value );
};

// all_values_are_integral
//   trait: true when _Type is a homogeneous option_set whose
// mapped_type is an integral type (but not bool).
template<typename _Type>
struct all_values_are_integral
{
private:
    using mapped =
        typename internal::mapped_type_or_void<_Type>::type;

public:
    static constexpr bool value =
        ( has_homogeneous_values<_Type>::value        &&
          std::is_integral<mapped>::value           &&
          !std::is_same<mapped, bool>::value );
};

// all_values_are_floating
//   trait: true when _Type is a homogeneous option_set whose
// mapped_type is a floating-point type.
template<typename _Type>
struct all_values_are_floating
{
private:
    using mapped =
        typename internal::mapped_type_or_void<_Type>::type;

public:
    static constexpr bool value =
        ( has_homogeneous_values<_Type>::value &&
          std::is_floating_point<mapped>::value );
};


///////////////////////////////////////////////////////////////////////////////
///        V.    TARGET-LEVEL EXPRESSION ALIASES                            ///
///////////////////////////////////////////////////////////////////////////////
// Detects whether an arbitrary type _Type exposes methods for
// accepting, exporting, or parsing options.  No assumption
// is made about _Type - it may be a container, a service, a
// configuration object, or any user-defined type.
//
// Expression aliases are split into two groups:
//   A. Unparameterized - method signature is fully determined
//      by _Type alone (fixed argument types or no arguments).
//   B. Parameterized - the method signature depends on one or
//      more additional type parameters (e.g. the option_set
//      type, or key and value types).

NS_INTERNAL

    // ----------------------------------------------------------------
    // A.  unparameterized (single _Type)
    // ----------------------------------------------------------------

    // to_options_method_expr
    //   detector: expression alias for .to_options() const.
    template<typename _Type>
    using to_options_method_expr = decltype(
        std::declval<const _Type&>().to_options());

    // get_options_method_expr
    //   detector: expression alias for .get_options() const.
    template<typename _Type>
    using get_options_method_expr = decltype(
        std::declval<const _Type&>().get_options());

    // help_string_method_expr
    //   detector: expression alias for .help_string() const.
    template<typename _Type>
    using help_string_method_expr = decltype(
        std::declval<const _Type&>().help_string());

    // parse_args_method_expr
    //   detector: expression alias for
    // .parse_args(int, const char* const[]).
    template<typename _Type>
    using parse_args_method_expr = decltype(
        std::declval<_Type&>().parse_args(
            std::declval<int>(),
            std::declval<const char* const*>()));

    // parse_option_method_expr
    //   detector: expression alias for
    // .parse_option(const std::string&).
    template<typename _Type>
    using parse_option_method_expr = decltype(
        std::declval<_Type&>().parse_option(
            std::declval<const std::string&>()));

    // parse_config_method_expr
    //   detector: expression alias for
    // .parse_config(const std::string&).
    template<typename _Type>
    using parse_config_method_expr = decltype(
        std::declval<_Type&>().parse_config(
            std::declval<const std::string&>()));

    // ----------------------------------------------------------------
    // C.  default options
    // ----------------------------------------------------------------
    // Three access paths for default option values:
    //   compile-time:  T::default_options()   (static method)
    //                  T::default_options      (static data member)
    //   runtime const: t.default_options()    (const instance method)
    //   runtime:       t.default_options()    (mutable instance method)

    // default_options_static_call_expr
    //   detector: expression alias for T::default_options()
    // as a nullary static method.  Covers the compile-time
    // path - the return value is typically constexpr.
    template<typename _Type>
    using default_options_static_call_expr =
        decltype(_Type::default_options());

    // default_options_static_member_expr
    //   detector: expression alias for T::default_options as
    // a static data member (e.g. static constexpr option_set).
    // Also matches static methods (yielding function type), so
    // downstream traits filter via !is_function.
    template<typename _Type>
    using default_options_static_member_expr =
        decltype(_Type::default_options);

    // default_options_const_method_expr
    //   detector: expression alias for .default_options() const.
    // Runtime const path - callable on a const reference.
    template<typename _Type>
    using default_options_const_method_expr = decltype(
        std::declval<const _Type&>().default_options());

    // default_options_method_expr
    //   detector: expression alias for .default_options() on
    // a mutable reference.  Runtime mutable path.
    template<typename _Type>
    using default_options_method_expr = decltype(
        std::declval<_Type&>().default_options());

    // ----------------------------------------------------------------
    // B.  parameterized (multi-type)
    // ----------------------------------------------------------------
    // These aliases accept additional type parameters beyond _Type.
    // Use with is_detected<alias, _Type, _Extra...>.

    // apply_options_method_expr
    //   detector: expression alias for
    // .apply_options(const _OptionSet&).
    template<typename _Type,
             typename _OptionSet>
    using apply_options_method_expr = decltype(
        std::declval<_Type&>().apply_options(
            std::declval<const _OptionSet&>()));

    // configure_method_expr
    //   detector: expression alias for
    // .configure(const _OptionSet&).
    template<typename _Type,
             typename _OptionSet>
    using configure_method_expr = decltype(
        std::declval<_Type&>().configure(
            std::declval<const _OptionSet&>()));

    // set_option_method_expr
    //   detector: expression alias for
    // .set_option(const _Key&, const _Value&).
    template<typename _Type,
             typename _Key,
             typename _Value>
    using set_option_method_expr = decltype(
        std::declval<_Type&>().set_option(
            std::declval<const _Key&>(),
            std::declval<const _Value&>()));

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///        VI.   TARGET-LEVEL has_* TRAITS                                  ///
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------
// unparameterized
// ----------------------------------------------------------------

// has_to_options_method
//   trait: detects .to_options() const on _Type.
template<typename _Type>
struct has_to_options_method
{
    static constexpr bool value =
        is_detected<internal::to_options_method_expr,
                    _Type>::value;
};

// has_get_options_method
//   trait: detects .get_options() const on _Type.
template<typename _Type>
struct has_get_options_method
{
    static constexpr bool value =
        is_detected<internal::get_options_method_expr,
                    _Type>::value;
};

// has_help_string_method
//   trait: detects .help_string() const on _Type.
template<typename _Type>
struct has_help_string_method
{
    static constexpr bool value =
        is_detected<internal::help_string_method_expr,
                    _Type>::value;
};

// has_parse_args_method
//   trait: detects .parse_args(int, const char* const*)
// on _Type.
template<typename _Type>
struct has_parse_args_method
{
    static constexpr bool value =
        is_detected<internal::parse_args_method_expr,
                    _Type>::value;
};

// has_parse_option_method
//   trait: detects .parse_option(const std::string&) on _Type.
template<typename _Type>
struct has_parse_option_method
{
    static constexpr bool value =
        is_detected<internal::parse_option_method_expr,
                    _Type>::value;
};

// has_parse_config_method
//   trait: detects .parse_config(const std::string&) on _Type.
template<typename _Type>
struct has_parse_config_method
{
    static constexpr bool value =
        is_detected<internal::parse_config_method_expr,
                    _Type>::value;
};

// ----------------------------------------------------------------
// default options
// ----------------------------------------------------------------

// has_static_default_options_call
//   trait: detects T::default_options() as a nullary static
// method.  This is the compile-time path - the result is
// typically constexpr-evaluable.
template<typename _Type>
struct has_static_default_options_call
{
    static constexpr bool value =
        is_detected<
            internal::default_options_static_call_expr,
            _Type>::value;
};

// has_static_default_options_member
//   trait: detects T::default_options as a static data member
// (not a static method).  Covers the pattern:
//   static constexpr option_set<K,V> default_options = { ... };

NS_INTERNAL

    // default_options_member_is_data
    //   helper: when the member exists, checks that it is
    // not a function type (which would indicate a static
    // method rather than a data member).
    template<typename _Type,
             bool = is_detected<
                 default_options_static_member_expr,
                 _Type>::value>
    struct default_options_member_is_data
    {
        static constexpr bool value = false;
    };

    template<typename _Type>
    struct default_options_member_is_data<_Type, true>
    {
        static constexpr bool value =
            !std::is_function<
                default_options_static_member_expr<_Type>
            >::value;
    };

NS_END  // internal

template<typename _Type>
struct has_static_default_options_member
{
    static constexpr bool value =
        internal::default_options_member_is_data<_Type>::value;
};

// has_static_default_options
//   trait: true when _Type exposes default options via a
// compile-time path - either a static method
// T::default_options() or a static constexpr data member
// T::default_options.
template<typename _Type>
struct has_static_default_options
{
    static constexpr bool value =
        ( has_static_default_options_call<_Type>::value ||
          has_static_default_options_member<_Type>::value );
};

// has_const_default_options_method
//   trait: detects .default_options() const on _Type.
// Runtime const path - callable on any const reference
// or const instance without mutation.
template<typename _Type>
struct has_const_default_options_method
{
    static constexpr bool value =
        is_detected<
            internal::default_options_const_method_expr,
            _Type>::value;
};

// has_default_options_method
//   trait: detects .default_options() on a mutable _Type&.
// Runtime mutable path - may compute or lazily initialize
// the defaults.
template<typename _Type>
struct has_default_options_method
{
    static constexpr bool value =
        is_detected<
            internal::default_options_method_expr,
            _Type>::value;
};

// ----------------------------------------------------------------
// parameterized
// ----------------------------------------------------------------

// has_apply_options_method
//   trait: detects .apply_options(const _OptionSet&) on _Type.
template<typename _Type,
         typename _OptionSet>
struct has_apply_options_method
{
    static constexpr bool value =
        is_detected<internal::apply_options_method_expr,
                    _Type, _OptionSet>::value;
};

// has_configure_method
//   trait: detects .configure(const _OptionSet&) on _Type.
template<typename _Type,
         typename _OptionSet>
struct has_configure_method
{
    static constexpr bool value =
        is_detected<internal::configure_method_expr,
                    _Type, _OptionSet>::value;
};

// has_set_option_method
//   trait: detects .set_option(const _Key&, const _Value&)
// on _Type.
template<typename _Type,
         typename _Key,
         typename _Value>
struct has_set_option_method
{
    static constexpr bool value =
        is_detected<internal::set_option_method_expr,
                    _Type, _Key, _Value>::value;
};


///////////////////////////////////////////////////////////////////////////////
///        VII.  CONFIGURE STRATEGY                                         ///
///////////////////////////////////////////////////////////////////////////////

// configure_strategy
//   enum: identifies the dispatch path used by apply_options.
enum class configure_strategy : unsigned
{
    native_apply     = 0,
    native_configure = 1,
    per_option       = 2,
    unsupported      = 3
};

NS_INTERNAL

    // option_configure_strategy_helper
    //   helper: resolves native paths in priority order.
    template<typename _Type,
             typename _OptionSet,
             bool     _HasApply =
                 is_detected<apply_options_method_expr,
                             _Type, _OptionSet>::value,
             bool     _HasConfigure =
                 is_detected<configure_method_expr,
                             _Type, _OptionSet>::value>
    struct option_configure_strategy_helper
    {
        static constexpr configure_strategy value =
            configure_strategy::unsupported;
    };

    // specialization: native apply path
    template<typename _Type,
             typename _OptionSet,
             bool     _HasConfigure>
    struct option_configure_strategy_helper<
        _Type, _OptionSet, true, _HasConfigure>
    {
        static constexpr configure_strategy value =
            configure_strategy::native_apply;
    };

    // specialization: native configure path
    template<typename _Type,
             typename _OptionSet>
    struct option_configure_strategy_helper<
        _Type, _OptionSet, false, true>
    {
        static constexpr configure_strategy value =
            configure_strategy::native_configure;
    };

    // per_option_strategy_check
    //   helper: detects the per-option path when the
    // option_set's entry type exposes .key and .value
    // and _Type has a matching set_option method.
    // Falls back to unsupported when entry types cannot
    // be deduced.
    template<typename _Type,
             typename _OptionSet,
             bool = ( has_value_type_alias<_OptionSet>::value &&
                      has_begin_method<_OptionSet>::value     &&
                      has_end_method<_OptionSet>::value )>
    struct per_option_strategy_check
    {
        static constexpr bool value = false;
    };

    template<typename _Type,
             typename _OptionSet>
    struct per_option_strategy_check<_Type, _OptionSet, true>
    {
    private:
        using entry_type = typename _OptionSet::value_type;

    public:
        static constexpr bool value =
            ( has_key<entry_type>::value   &&
              has_value<entry_type>::value &&
              is_detected<set_option_method_expr,
                          _Type,
                          option_key_t<entry_type>,
                          option_value_t<entry_type>>::value );
    };

NS_END  // internal

// option_configure_strategy
//   trait: resolves the best configure strategy for applying
// _OptionSet to _Type.  Checks native paths first, then falls
// back to per-option iteration.
template<typename _Type,
         typename _OptionSet>
struct option_configure_strategy
{
private:
    static constexpr configure_strategy native =
        internal::option_configure_strategy_helper<
            _Type, _OptionSet>::value;

public:
    static constexpr configure_strategy value =
        (native != configure_strategy::unsupported)
            ? native
            : ( internal::per_option_strategy_check<
                    _Type, _OptionSet>::value
                ? configure_strategy::per_option
                : configure_strategy::unsupported );
};


///////////////////////////////////////////////////////////////////////////////
///        VIII. COMPOSITE TARGET TRAITS                                    ///
///////////////////////////////////////////////////////////////////////////////

// is_options_configurable
//   trait: true when _Type can be configured from _OptionSet
// via at least one strategy path (apply_options, configure,
// or per-option set_option).
template<typename _Type,
         typename _OptionSet>
struct is_options_configurable
{
    static constexpr bool value =
        ( option_configure_strategy<
              _Type, _OptionSet>::value !=
          configure_strategy::unsupported );
};

// is_option_producing
//   trait: true when _Type can export its current configuration
// as an option collection via to_options() or get_options().
template<typename _Type>
struct is_option_producing
{
    static constexpr bool value =
        ( has_to_options_method<_Type>::value ||
          has_get_options_method<_Type>::value );
};

// is_option_parseable
//   trait: true when _Type can accept configuration from string
// sources - CLI arguments, individual option strings, or
// config file contents.
template<typename _Type>
struct is_option_parseable
{
    static constexpr bool value =
        ( has_parse_args_method<_Type>::value   ||
          has_parse_option_method<_Type>::value ||
          has_parse_config_method<_Type>::value );
};

// is_option_describable
//   trait: true when _Type can produce help/usage text
// describing its available options.
template<typename _Type>
struct is_option_describable
{
    static constexpr bool value =
        has_help_string_method<_Type>::value;
};

// has_default_options
//   trait: true when _Type exposes default options via any
// path - compile-time (static method or constexpr member),
// runtime const, or runtime mutable.
template<typename _Type>
struct has_default_options
{
    static constexpr bool value =
        ( has_static_default_options<_Type>::value       ||
          has_const_default_options_method<_Type>::value  ||
          has_default_options_method<_Type>::value );
};

// has_runtime_default_options
//   trait: true when _Type exposes default options via a
// runtime path (const or mutable instance method), as
// opposed to the compile-time static path.
template<typename _Type>
struct has_runtime_default_options
{
    static constexpr bool value =
        ( has_const_default_options_method<_Type>::value ||
          has_default_options_method<_Type>::value );
};

// has_constexpr_default_options
//   trait: true when _Type exposes default options via a
// compile-time path (static method or static constexpr
// data member).  Alias for has_static_default_options,
// provided for naming symmetry with
// has_runtime_default_options.
template<typename _Type>
struct has_constexpr_default_options
{
    static constexpr bool value =
        has_static_default_options<_Type>::value;
};

// is_option_set_constructible
//   trait: true when _Type can be constructed from an
// option_set<_Key, _Value>.  Detects constructors of the
// form T(option_set<K,V>) or T(const option_set<K,V>&).
template<typename _Type,
         typename _Key,
         typename _Value>
struct is_option_set_constructible
{
    static constexpr bool value =
        std::is_constructible<
            _Type,
            const option_set<_Key, _Value>&>::value;
};

// is_option_list_constructible
//   trait: true when _Type can be constructed from an
// initializer_list of option_pair<_Key, _Value>.  Detects
// constructors of the form
// T(std::initializer_list<option_pair<K,V>>).
template<typename _Type,
         typename _Key,
         typename _Value>
struct is_option_list_constructible
{
    static constexpr bool value =
        std::is_constructible<
            _Type,
            std::initializer_list<
                option_pair<_Key, _Value>>>::value;
};

// is_options_constructible
//   trait: true when _Type can be constructed from options
// via either an option_set or an initializer_list of
// option_pairs.  This is the aggregate constructibility
// query - use when the construction path does not matter,
// only whether option-based construction is possible.
template<typename _Type,
         typename _Key,
         typename _Value>
struct is_options_constructible
{
    static constexpr bool value =
        ( is_option_set_constructible<
              _Type, _Key, _Value>::value ||
          is_option_list_constructible<
              _Type, _Key, _Value>::value );
};


///////////////////////////////////////////////////////////////////////////////
///        IX.   STATIC OPTION SET CLASS IDENTITY                           ///
///////////////////////////////////////////////////////////////////////////////
// Identity and structural detection for the `static_option_set<E...>`
// class template introduced in static_option_set.hpp.
//
// NAMING NOTE:
//   Section III defines `is_static_option_set<T>` as a STRUCTURAL
// check — true when T satisfies the option_set contract AND exposes
// `.backing_array()` (i.e. compile-time-sized storage).  That trait
// is unchanged here.
//
//   This section adds an IDENTITY trait, `is_static_option_set_class<T>`,
// which is true only for instantiations of the specific
// `static_option_set<E...>` class template.  Pattern matches the
// existing `is_table` (structural) / `is_fundamental_table` (identity)
// convention used elsewhere in the framework.
//
//   `has_compile_time_keys<T>` is a separate structural probe for the
// type-keyed lookup surface (`template<K> get<K>()` and
// `template<K> contains<K>::value`) that the static class exposes.
// User-defined types that mimic this surface qualify without inheriting
// from `static_option_set`.
//
//   `is_any_option_set<T>` is the aggregate that recognizes a type
// regardless of which form it takes.

// is_static_option_set_class
//   trait: true iff `_Type` is a `static_option_set<_Entries...>`
// instantiation after cv-ref stripping.  Identity check, not
// structural — see has_compile_time_keys for the structural form.
template<typename _Type>
struct is_static_option_set_class : std::false_type
{};

template<typename... _Entries>
struct is_static_option_set_class<static_option_set<_Entries...>>
    : std::true_type
{};

template<typename _Type>
struct is_static_option_set_class<const _Type>
    : is_static_option_set_class<_Type>
{};

template<typename _Type>
struct is_static_option_set_class<_Type&>
    : is_static_option_set_class<_Type>
{};

template<typename _Type>
struct is_static_option_set_class<_Type&&>
    : is_static_option_set_class<_Type>
{};


NS_INTERNAL

    // probe_key_unlikely_to_exist
    //   helper: a dummy key type used to probe whether a class
    // exposes the typed lookup surface.  Template-name lookup
    // succeeds before substitution, so absence of this specific
    // key in any real container does not foil the probe.
    struct probe_key_unlikely_to_exist {};

    // typed_get_expr
    //   detector: expression alias for `template<K> get<K>()`,
    // probed with the sentinel key type.
    template<typename _Type>
    using typed_get_expr =
        decltype(
            std::declval<const _Type&>().template get<
                probe_key_unlikely_to_exist>());

    // typed_contains_expr
    //   detector: expression alias for the value of a typed
    // `template<K> contains<K>::value` nested trait, probed with
    // the sentinel key type.
    template<typename _Type>
    using typed_contains_expr =
        decltype(
            _Type::template contains<
                probe_key_unlikely_to_exist>::value);

NS_END  // internal

// has_compile_time_keys
//   trait: structural detection — true if `_Type` exposes the
// compile-time type-keyed lookup surface (`template<K> get<K>()`
// plus `template<K> contains<K>::value`).
template<typename _Type>
struct has_compile_time_keys
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool value =
        ( is_detected<internal::typed_get_expr,
                      clean_type>::value &&
          is_detected<internal::typed_contains_expr,
                      clean_type>::value );
};

// is_any_option_set
//   trait: aggregate — true if `_Type` is a dynamic option_set-like
// type (section III), a `static_option_set<...>` class instance,
// or any type exposing the compile-time-keyed lookup surface.
//
//   Used by `option_layers` to dispatch lookup paths per layer.
template<typename _Type>
struct is_any_option_set
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool value =
        ( is_option_set<clean_type>::value              ||
          is_static_option_set_class<clean_type>::value ||
          has_compile_time_keys<clean_type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///        X.    STATIC OPTION SET CONSTRUCTIBILITY                         ///
///////////////////////////////////////////////////////////////////////////////
// Constructibility traits for the static-options surface, mirroring
// the dynamic-side `is_option_set_constructible` / `is_options_constructible`
// traits in section VIII.
//
// STRUCTURAL DIFFERENCE FROM THE DYNAMIC TRAITS:
//   The dynamic option-set constructibility traits parameterise on
// (_Type, _Key, _Value) because `option_set<K, V>` is homogeneous.
// `static_option_set<Entries...>` is heterogeneous — each entry may
// carry a different value type — so the static traits parameterise
// on (_Type, _StaticSet) instead.

// is_static_option_set_constructible
//   trait: true when _Type can be constructed from a const
// reference to the specific _StaticSet instance type.
template<typename _Type,
         typename _StaticSet>
struct is_static_option_set_constructible
{
    static constexpr bool value =
        std::is_constructible<
            _Type,
            const _StaticSet&>::value;
};


NS_INTERNAL

    // probe_static_set
    //   helper: an empty static_option_set used to probe whether a
    // type accepts ANY static_option_set instantiation via a
    // template constructor.
    using probe_static_set = static_option_set<>;

    // template_static_ctor_expr
    //   detector: expression alias probing for constructibility
    // from the sentinel empty static set.
    template<typename _Type>
    using template_static_ctor_expr =
        decltype(_Type(std::declval<const probe_static_set&>()));

NS_END  // internal

// is_any_static_option_set_constructible
//   trait: true when _Type has a (template) constructor accepting
// any static_option_set instantiation.  Heuristic — types
// constraining on a non-empty entry pack will report false here.
template<typename _Type>
struct is_any_static_option_set_constructible
{
    static constexpr bool value =
        is_detected<internal::template_static_ctor_expr,
                    _Type>::value;
};


NS_INTERNAL

    // from_static_options_call_expr
    //   detector: expression alias for
    // `_Type::from_static_options(static_set)` returning something
    // convertible to _Type.
    template<typename _Type,
             typename _StaticSet>
    using from_static_options_call_expr =
        decltype(_Type::from_static_options(
            std::declval<const _StaticSet&>()));

    // apply_static_options_method_expr
    //   detector: expression alias for
    // `instance.apply_static_options(static_set)`.
    template<typename _Type,
             typename _StaticSet>
    using apply_static_options_method_expr =
        decltype(std::declval<_Type&>().apply_static_options(
            std::declval<const _StaticSet&>()));

    // from_static_return_type
    //   helper: safely resolves the return type of
    // `_Type::from_static_options(...)`.  Falls back to void
    // when the call expression doesn't exist, so the convertibility
    // check in has_from_static_options_factory below has a
    // well-formed type to feed std::is_convertible.
    template<typename _Type,
             typename _StaticSet,
             bool     _Detected>
    struct from_static_return_type
    {
        using type = void;
    };

    template<typename _Type,
             typename _StaticSet>
    struct from_static_return_type<_Type, _StaticSet, true>
    {
        using type = from_static_options_call_expr<
            _Type, _StaticSet>;
    };

NS_END  // internal

// has_from_static_options_factory
//   trait: true when _Type exposes
// `static _Type from_static_options(const _StaticSet&)` (or any
// signature whose return type is convertible to _Type).
template<typename _Type,
         typename _StaticSet>
struct has_from_static_options_factory
{
private:
    static constexpr bool detected =
        is_detected<internal::from_static_options_call_expr,
                    _Type, _StaticSet>::value;

    using return_type =
        typename internal::from_static_return_type<
            _Type, _StaticSet, detected>::type;

public:
    static constexpr bool value =
        ( detected &&
          std::is_convertible<return_type, _Type>::value );
};

// has_apply_static_options_method
//   trait: true when _Type exposes an instance method
// `apply_static_options(const _StaticSet&)`.
template<typename _Type,
         typename _StaticSet>
struct has_apply_static_options_method
{
    static constexpr bool value =
        is_detected<internal::apply_static_options_method_expr,
                    _Type, _StaticSet>::value;
};


NS_INTERNAL

    // is_verified_entry
    //   helper trait: true iff _Entry is a verified_option_pair<...>.
    template<typename _Entry>
    struct is_verified_entry : std::false_type
    {};

    template<typename _K,
             typename _V,
             typename _F>
    struct is_verified_entry<verified_option_pair<_K, _V, _F>>
        : std::true_type
    {};

    // bool_or_fold
    //   helper: OR-fold over a pack of bool NTTPs.
    template<bool... _Bs>
    struct bool_or_fold;

    template<>
    struct bool_or_fold<> : std::false_type
    {};

    template<bool _First,
             bool... _Rest>
    struct bool_or_fold<_First, _Rest...>
        : std::integral_constant<bool,
            _First || bool_or_fold<_Rest...>::value>
    {};

    // bool_and_fold
    //   helper: AND-fold over a pack of bool NTTPs.
    template<bool... _Bs>
    struct bool_and_fold;

    template<>
    struct bool_and_fold<> : std::true_type
    {};

    template<bool _First,
             bool... _Rest>
    struct bool_and_fold<_First, _Rest...>
        : std::integral_constant<bool,
            _First && bool_and_fold<_Rest...>::value>
    {};

NS_END  // internal

// is_static_default_constructible
//   trait: true when every entry in _StaticSet is itself
// default-constructible.  Specialized only on the literal
// static_option_set<...> class template — other compile-time-keyed
// types are not covered by this trait.
template<typename _StaticSet>
struct is_static_default_constructible;

template<typename... _Entries>
struct is_static_default_constructible<
    static_option_set<_Entries...>>
    : std::integral_constant<bool,
        internal::bool_and_fold<
            std::is_default_constructible<_Entries>::value...>::value>
{};

// has_verifier_entries
//   trait: true when _StaticSet contains at least one
// verified_option_pair entry.  Specialized only on the literal
// static_option_set<...> class template.
template<typename _StaticSet>
struct has_verifier_entries;

template<typename... _Entries>
struct has_verifier_entries<
    static_option_set<_Entries...>>
    : std::integral_constant<bool,
        internal::bool_or_fold<
            internal::is_verified_entry<_Entries>::value...>::value>
{};

// is_options_constructible_any
//   trait: aggregate constructibility query — true when _Type can
// be constructed via any recognized options path:
//   - from a dynamic option_set<_Key, _Value>
//   - from an initializer_list<option_pair<_Key, _Value>>
//   - from the static set _StaticSet
//   - from any static_option_set via template ctor
template<typename _Type,
         typename _Key,
         typename _Value,
         typename _StaticSet>
struct is_options_constructible_any
{
    static constexpr bool value =
        ( is_option_set_constructible<
              _Type, _Key, _Value>::value          ||
          is_option_list_constructible<
              _Type, _Key, _Value>::value          ||
          is_static_option_set_constructible<
              _Type, _StaticSet>::value            ||
          is_any_static_option_set_constructible<
              _Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///        XI.   CONVENIENCE ALIASES                                        ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // ================================================================
    // container-level _v aliases
    // ================================================================

    template<typename _Type>
    constexpr bool has_key_type_alias_v =
        has_key_type_alias<_Type>::value;

    template<typename _Type>
    constexpr bool has_mapped_type_alias_v =
        has_mapped_type_alias<_Type>::value;

    template<typename _Type>
    constexpr bool has_value_type_alias_v =
        has_value_type_alias<_Type>::value;

    template<typename _Type>
    constexpr bool has_extent_v = has_extent<_Type>::value;

    template<typename _Type>
    constexpr bool has_find_method_v =
        has_find_method<_Type>::value;

    template<typename _Type>
    constexpr bool has_contains_method_v =
        has_contains_method<_Type>::value;

    template<typename _Type>
    constexpr bool has_value_or_method_v =
        has_value_or_method<_Type>::value;

    template<typename _Type>
    constexpr bool has_size_accessor_v =
        has_size_accessor<_Type>::value;

    template<typename _Type>
    constexpr bool has_empty_method_v =
        has_empty_method<_Type>::value;

    template<typename _Type>
    constexpr bool has_begin_method_v =
        has_begin_method<_Type>::value;

    template<typename _Type>
    constexpr bool has_end_method_v =
        has_end_method<_Type>::value;

    template<typename _Type>
    constexpr bool has_backing_array_v =
        has_backing_array<_Type>::value;

    template<typename _Type>
    constexpr bool has_count_if_method_v =
        has_count_if_method<_Type>::value;

    template<typename _Type>
    constexpr bool has_data_method_v =
        has_data_method<_Type>::value;

    // ================================================================
    // composite container _v aliases
    // ================================================================

    template<typename _Type>
    constexpr bool is_option_set_v =
        is_option_set<_Type>::value;

    template<typename _Type>
    constexpr bool is_iterable_option_set_v =
        is_iterable_option_set<_Type>::value;

    template<typename _Type>
    constexpr bool is_searchable_option_set_v =
        is_searchable_option_set<_Type>::value;

    template<typename _Type>
    constexpr bool is_static_option_set_v =
        is_static_option_set<_Type>::value;

    template<typename _Type>
    constexpr bool is_dynamic_option_set_v =
        is_dynamic_option_set<_Type>::value;

    template<typename _Type>
    constexpr bool is_sorted_option_set_v =
        is_sorted_option_set<_Type>::value;

    // ================================================================
    // value homogeneity _v aliases
    // ================================================================

    template<typename _Type>
    constexpr bool has_homogeneous_values_v =
        has_homogeneous_values<_Type>::value;

    template<typename _Type>
    constexpr bool has_heterogeneous_values_v =
        has_heterogeneous_values<_Type>::value;

    template<typename _Type, typename _V>
    constexpr bool has_mapped_type_of_v =
        has_mapped_type_of<_Type, _V>::value;

    template<typename _Type>
    constexpr bool all_values_are_arithmetic_v =
        all_values_are_arithmetic<_Type>::value;

    template<typename _Type>
    constexpr bool all_values_are_string_v =
        all_values_are_string<_Type>::value;

    template<typename _Type>
    constexpr bool all_values_are_boolean_v =
        all_values_are_boolean<_Type>::value;

    template<typename _Type>
    constexpr bool all_values_are_enum_v =
        all_values_are_enum<_Type>::value;

    template<typename _Type>
    constexpr bool all_values_are_integral_v =
        all_values_are_integral<_Type>::value;

    template<typename _Type>
    constexpr bool all_values_are_floating_v =
        all_values_are_floating<_Type>::value;

    // ================================================================
    // target-level has_* _v aliases
    // ================================================================

    template<typename _Type>
    constexpr bool has_to_options_method_v =
        has_to_options_method<_Type>::value;

    template<typename _Type>
    constexpr bool has_get_options_method_v =
        has_get_options_method<_Type>::value;

    template<typename _Type>
    constexpr bool has_help_string_method_v =
        has_help_string_method<_Type>::value;

    template<typename _Type>
    constexpr bool has_parse_args_method_v =
        has_parse_args_method<_Type>::value;

    template<typename _Type>
    constexpr bool has_parse_option_method_v =
        has_parse_option_method<_Type>::value;

    template<typename _Type>
    constexpr bool has_parse_config_method_v =
        has_parse_config_method<_Type>::value;

    template<typename _Type, typename _OptionSet>
    constexpr bool has_apply_options_method_v =
        has_apply_options_method<_Type, _OptionSet>::value;

    template<typename _Type, typename _OptionSet>
    constexpr bool has_configure_method_v =
        has_configure_method<_Type, _OptionSet>::value;

    template<typename _Type, typename _Key, typename _Value>
    constexpr bool has_set_option_method_v =
        has_set_option_method<_Type, _Key, _Value>::value;

    // ================================================================
    // strategy _v alias
    // ================================================================

    template<typename _Type, typename _OptionSet>
    constexpr configure_strategy
        option_configure_strategy_v =
            option_configure_strategy<_Type, _OptionSet>::value;

    // ================================================================
    // composite target _v aliases
    // ================================================================

    template<typename _Type, typename _OptionSet>
    constexpr bool is_options_configurable_v =
        is_options_configurable<_Type, _OptionSet>::value;

    template<typename _Type>
    constexpr bool is_option_producing_v =
        is_option_producing<_Type>::value;

    template<typename _Type>
    constexpr bool is_option_parseable_v =
        is_option_parseable<_Type>::value;

    template<typename _Type>
    constexpr bool is_option_describable_v =
        is_option_describable<_Type>::value;

    // ================================================================
    // default options _v aliases
    // ================================================================

    template<typename _Type>
    constexpr bool has_static_default_options_call_v =
        has_static_default_options_call<_Type>::value;

    template<typename _Type>
    constexpr bool has_static_default_options_member_v =
        has_static_default_options_member<_Type>::value;

    template<typename _Type>
    constexpr bool has_static_default_options_v =
        has_static_default_options<_Type>::value;

    template<typename _Type>
    constexpr bool has_const_default_options_method_v =
        has_const_default_options_method<_Type>::value;

    template<typename _Type>
    constexpr bool has_default_options_method_v =
        has_default_options_method<_Type>::value;

    template<typename _Type>
    constexpr bool has_default_options_v =
        has_default_options<_Type>::value;

    template<typename _Type>
    constexpr bool has_runtime_default_options_v =
        has_runtime_default_options<_Type>::value;

    template<typename _Type>
    constexpr bool has_constexpr_default_options_v =
        has_constexpr_default_options<_Type>::value;

    // ================================================================
    // constructibility _v aliases
    // ================================================================

    template<typename _Type, typename _Key, typename _Value>
    constexpr bool is_option_set_constructible_v =
        is_option_set_constructible<_Type, _Key, _Value>::value;

    template<typename _Type, typename _Key, typename _Value>
    constexpr bool is_option_list_constructible_v =
        is_option_list_constructible<_Type, _Key, _Value>::value;

    template<typename _Type, typename _Key, typename _Value>
    constexpr bool is_options_constructible_v =
        is_options_constructible<_Type, _Key, _Value>::value;

    // ================================================================
    // static option set class identity _v aliases  (section IX)
    // ================================================================

    template<typename _Type>
    constexpr bool is_static_option_set_class_v =
        is_static_option_set_class<_Type>::value;

    template<typename _Type>
    constexpr bool has_compile_time_keys_v =
        has_compile_time_keys<_Type>::value;

    template<typename _Type>
    constexpr bool is_any_option_set_v =
        is_any_option_set<_Type>::value;

    // ================================================================
    // static option set constructibility _v aliases  (section X)
    // ================================================================

    template<typename _Type, typename _StaticSet>
    constexpr bool is_static_option_set_constructible_v =
        is_static_option_set_constructible<_Type, _StaticSet>::value;

    template<typename _Type>
    constexpr bool is_any_static_option_set_constructible_v =
        is_any_static_option_set_constructible<_Type>::value;

    template<typename _Type, typename _StaticSet>
    constexpr bool has_from_static_options_factory_v =
        has_from_static_options_factory<_Type, _StaticSet>::value;

    template<typename _Type, typename _StaticSet>
    constexpr bool has_apply_static_options_method_v =
        has_apply_static_options_method<_Type, _StaticSet>::value;

    template<typename _StaticSet>
    constexpr bool is_static_default_constructible_v =
        is_static_default_constructible<_StaticSet>::value;

    template<typename _StaticSet>
    constexpr bool has_verifier_entries_v =
        has_verifier_entries<_StaticSet>::value;

    template<typename _Type,
             typename _Key,
             typename _Value,
             typename _StaticSet>
    constexpr bool is_options_constructible_any_v =
        is_options_constructible_any<
            _Type, _Key, _Value, _StaticSet>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // djinterp


#endif  // DJINTERP_OPTION_SET_TRAITS_
