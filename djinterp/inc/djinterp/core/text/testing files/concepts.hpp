/******************************************************************************
* djinterp [meta]                                                 concepts.hpp
*
* djinterp concepts header:
*   This header provides C++20 concept definitions that parallel the type
* traits in type_traits.hpp. It includes:
*   - standard library concept re-exports
*   - custom concept definition macros (parallel to the D_TYPE_TRAIT_HAS_*
*     family in trait_detect.hpp)
*   - fundamental and composite type concepts
*   - type property concepts (cv-qualification, triviality, lifetime)
*   - tuple introspection concepts
*   - class definition rule concepts (Rule of Zero / Three / Five)
*   - container, allocator, and template concepts
*   - logical, invocable, size/numeric, and parameter-pack concepts
* STRUCTURE:
*   The header is laid out to mirror type_traits.hpp:
*     0.  Concept definition macros (parallels the D_TYPE_TRAIT_HAS_* detection
*         macros, which now live in trait_detect.hpp - formerly type_traits.hpp
*         section 0.3). Sits at file scope so the macros are namespace-
*         agnostic; the concepts they emit are intended to be instantiated
*         inside whatever namespace the macro is invoked in (typically the
*         djinterp namespace below).
*     I.  Standard library concept re-exports (parallels section I portable
*         standard-library traits).
*    II.  Custom djinterp concepts (parallels section III custom traits).
* REQUIREMENTS:
*   This header requires C++20 or later. It uses env.h and env_cpp_features.h
* (pulled in transitively via djinterp.hpp) for feature detection to verify
* concept support is available.
* INDEPENDENCE:
*   This header is designed to be completely independent of type_traits.hpp.
* Code may choose to use either traits-based or concept-based constraints;
* the two are parallel facilities, not layered.
*
*
* path:      /inc/djinterp/core/meta/concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2024.03.21
******************************************************************************/

#ifndef DJINTERP_META_CONCEPTS_
#define DJINTERP_META_CONCEPTS_ 1

// std
#include <concepts>
#include <memory>
#include <tuple>
#include <type_traits>
// djinterp
#include "../djinterp.hpp"


// require C++20 for concepts
#if !D_ENV_LANG_IS_CPP20_OR_HIGHER
    #error "concepts.hpp requires C++20 or later"
#endif

// verify compiler support for the concepts language feature
#if !D_ENV_CPP_FEATURE_LANG_CONCEPTS
    #error "concepts.hpp requires compiler support for concepts (__cpp_concepts)"
#endif


// =============================================================================
// 0.   CONCEPT DEFINITION MACROS
// =============================================================================
// Parallels the D_TYPE_TRAIT_HAS_* family, which now lives in trait_detect.hpp
// (formerly the D_TRAIT_HAS_* family in type_traits.hpp section 0.3). Each
// macro below emits a concept definition (not a trait struct), one per member
// of that family, so a caller can express the same detection either as a trait
// or as a concept.
//
// Macros sit at file scope (intentionally - the C++ preprocessor has no
// concept of namespaces), but the concepts they emit are intended to be
// instantiated inside whatever namespace the macro is invoked in (typically
// the djinterp namespace below).
//
// Family overview:
//   - D_CONCEPT_HAS_METHOD                 : detects T.M() - no args.
//   - D_CONCEPT_HAS_METHOD_ARGS            : detects T.M(args...).
//   - D_CONCEPT_HAS_METHOD_TYPED           : detects T.M() returning exactly RET.
//   - D_CONCEPT_HAS_METHOD_ARGS_TYPED      : detects T.M(args...) returning RET.
//   - D_CONCEPT_HAS_METHOD_CONVERTIBLE     : detects T.M() returning a type
//                                            CONVERTIBLE to RET.
//   - D_CONCEPT_HAS_METHOD_ARGS_CONVERTIBLE: detects T.M(args...) returning a
//                                            type CONVERTIBLE to RET.
//   - D_CONCEPT_HAS_TYPE                   : detects nested type T::TYPE_NAME.
//   - D_CONCEPT_HAS_STATIC_MEMBER          : detects static member T::MEMBER.
//   - D_CONCEPT_HAS_BINARY_OP              : detects `T OP T` (const operands).
//   - D_CONCEPT_HAS_UNARY_OP               : detects prefix `OP T`.

// D_CONCEPT_HAS_METHOD
//   macro: emits a concept that is satisfied when `_Type` has a callable
// member named METHOD_NAME taking no arguments. Concept analog of
// D_TYPE_TRAIT_HAS_METHOD. Renamed from D_CONCEPT_DETECT_METHOD.
#define D_CONCEPT_HAS_METHOD(CONCEPT_NAME, METHOD_NAME)                       \
    template<typename _Type>                                                  \
    concept CONCEPT_NAME = requires(_Type& _t) {                              \
        _t.METHOD_NAME();                                                     \
    };

// D_CONCEPT_HAS_METHOD_ARGS
//   macro: emits a concept that is satisfied when `_Type` has a callable
// member named METHOD_NAME taking an argument of the given type. Concept
// analog of D_TYPE_TRAIT_HAS_METHOD_ARGS. Renamed from
// D_CONCEPT_DETECT_METHOD_ARGS.
//
//   Like the trait's EXPR_METHOD_ARGS probe, the argument is a SINGLE type
// (the call is `M(declval<ARG>())`); this is the shared contract of the
// three _ARGS_ variants below. (Previously this emitted a spurious pack
// expansion `declval<...>()...` that failed to compile at all.)
#define D_CONCEPT_HAS_METHOD_ARGS(CONCEPT_NAME, METHOD_NAME, ...)             \
    template<typename _Type>                                                  \
    concept CONCEPT_NAME = requires(_Type& _t) {                              \
        _t.METHOD_NAME(std::declval<__VA_ARGS__>());                          \
    };

// D_CONCEPT_HAS_METHOD_TYPED
//   macro: emits a concept that is satisfied when `_Type` has a callable
// member named METHOD_NAME returning exactly RET. Concept analog of
// D_TYPE_TRAIT_HAS_METHOD_TYPED. Renamed from D_CONCEPT_DETECT_METHOD_RETURNS.
#define D_CONCEPT_HAS_METHOD_TYPED(CONCEPT_NAME, METHOD_NAME, RET)            \
    template<typename _Type>                                                  \
    concept CONCEPT_NAME = requires(_Type& _t) {                              \
        { _t.METHOD_NAME() } -> std::same_as<RET>;                            \
    };

// D_CONCEPT_HAS_METHOD_ARGS_TYPED
//   macro: emits a concept that is satisfied when `_Type` has a callable
// member named METHOD_NAME taking the given argument types and returning
// exactly RET. Renamed from D_CONCEPT_DETECT_METHOD_ARGS_RETURNS.
#define D_CONCEPT_HAS_METHOD_ARGS_TYPED(CONCEPT_NAME, METHOD_NAME, RET, ...)  \
    template<typename _Type>                                                  \
    concept CONCEPT_NAME = requires(_Type& _t) {                              \
        { _t.METHOD_NAME(std::declval<__VA_ARGS__>()) }                       \
            -> std::same_as<RET>;                                             \
    };

// D_CONCEPT_HAS_METHOD_CONVERTIBLE
//   macro: emits a concept that is satisfied when `_Type` has a callable
// member named METHOD_NAME (no args) whose return type is CONVERTIBLE to RET -
// the looser sibling of D_CONCEPT_HAS_METHOD_TYPED (e.g. a size() returning
// unsigned where size_t is wanted). Concept analog of
// D_TYPE_TRAIT_HAS_METHOD_CONVERTIBLE.
#define D_CONCEPT_HAS_METHOD_CONVERTIBLE(CONCEPT_NAME, METHOD_NAME, RET)      \
    template<typename _Type>                                                  \
    concept CONCEPT_NAME = requires(_Type& _t) {                              \
        { _t.METHOD_NAME() } -> std::convertible_to<RET>;                     \
    };

// D_CONCEPT_HAS_METHOD_ARGS_CONVERTIBLE
//   macro: emits a concept that is satisfied when `_Type` has a callable
// member named METHOD_NAME taking the given argument types and returning a
// type CONVERTIBLE to RET. The args-taking sibling of
// D_CONCEPT_HAS_METHOD_CONVERTIBLE; concept analog of
// D_TYPE_TRAIT_HAS_METHOD_CONVERTIBLE with arguments.
#define D_CONCEPT_HAS_METHOD_ARGS_CONVERTIBLE(CONCEPT_NAME, METHOD_NAME,      \
                                              RET, ...)                       \
    template<typename _Type>                                                  \
    concept CONCEPT_NAME = requires(_Type& _t) {                              \
        { _t.METHOD_NAME(std::declval<__VA_ARGS__>()) }                       \
            -> std::convertible_to<RET>;                                      \
    };

// D_CONCEPT_HAS_TYPE
//   macro: emits a concept that is satisfied when `_Type` has a nested
// type alias named TYPE_NAME. Concept analog of D_TYPE_TRAIT_HAS_TYPE.
// Renamed from D_CONCEPT_DETECT_TYPE.
#define D_CONCEPT_HAS_TYPE(CONCEPT_NAME, TYPE_NAME)                           \
    template<typename _Type>                                                  \
    concept CONCEPT_NAME = requires {                                         \
        typename _Type::TYPE_NAME;                                            \
    };

// D_CONCEPT_HAS_STATIC_MEMBER
//   macro: emits a concept that is satisfied when `_Type` has a static
// member named MEMBER (of any kind). Concept analog of
// D_TYPE_TRAIT_HAS_STATIC_MEMBER. Renamed from D_CONCEPT_DETECT_STATIC.
#define D_CONCEPT_HAS_STATIC_MEMBER(CONCEPT_NAME, MEMBER)                     \
    template<typename _Type>                                                  \
    concept CONCEPT_NAME = requires {                                         \
        _Type::MEMBER;                                                        \
    };

// D_CONCEPT_HAS_BINARY_OP
//   macro: emits a concept that is satisfied when two (const) `_Type` operands
// support the binary operator OP (e.g. +, ==, <). Operands are `const _Type&`,
// matching the trait probe, so the concept is not defeated by const operands.
// Concept analog of D_TYPE_TRAIT_HAS_BINARY_OP.
#define D_CONCEPT_HAS_BINARY_OP(CONCEPT_NAME, OP)                             \
    template<typename _Type>                                                  \
    concept CONCEPT_NAME = requires(const _Type& _a, const _Type& _b) {       \
        _a OP _b;                                                             \
    };

// D_CONCEPT_HAS_UNARY_OP
//   macro: emits a concept that is satisfied when a `_Type` operand supports
// the prefix unary operator OP (e.g. -, !, *, ++). Concept analog of
// D_TYPE_TRAIT_HAS_UNARY_OP.
#define D_CONCEPT_HAS_UNARY_OP(CONCEPT_NAME, OP)                              \
    template<typename _Type>                                                  \
    concept CONCEPT_NAME = requires(_Type& _t) {                              \
        OP _t;                                                                \
    };


NS_DJINTERP


// =============================================================================
// I.   STANDARD LIBRARY CONCEPT RE-EXPORTS
// =============================================================================
// Re-exports of standard library concepts for convenience. These are the
// concept equivalents of many standard type traits and parallel the portable
// standard-library traits exposed in type_traits.hpp section I.

// -----------------------------------------------------------------------------
// I.1  Core language concepts
// -----------------------------------------------------------------------------

using std::same_as;
using std::derived_from;
using std::convertible_to;
using std::common_reference_with;
using std::common_with;
using std::integral;
using std::signed_integral;
using std::unsigned_integral;
using std::floating_point;
using std::assignable_from;
using std::swappable;
using std::swappable_with;
using std::destructible;
using std::constructible_from;
using std::default_initializable;
using std::move_constructible;
using std::copy_constructible;

// -----------------------------------------------------------------------------
// I.2  Comparison concepts
// -----------------------------------------------------------------------------

using std::equality_comparable;
using std::equality_comparable_with;
using std::totally_ordered;
using std::totally_ordered_with;

// -----------------------------------------------------------------------------
// I.3  Object concepts
// -----------------------------------------------------------------------------

using std::movable;
using std::copyable;
using std::semiregular;
using std::regular;

// -----------------------------------------------------------------------------
// I.4  Callable concepts
// -----------------------------------------------------------------------------

using std::invocable;
using std::regular_invocable;
using std::predicate;
using std::relation;
using std::equivalence_relation;
using std::strict_weak_order;


// =============================================================================
// II.  CUSTOM DJINTERP CONCEPTS
// =============================================================================
// Custom concepts to supplement the standard library concepts above. Parallel
// to the custom traits in type_traits.hpp section III; where a custom trait
// `is_X` exists there, an analogous concept (`is_X_c` or a natural-language
// equivalent) lives here.

// -----------------------------------------------------------------------------
// II.1  Fundamental type concepts
// -----------------------------------------------------------------------------

// II.1.a  Void and null concepts

// is_void_c
//   concept: satisfied if `_Type` is (cv-qualified) void.
template<typename _Type>
concept is_void_c = std::is_void_v<_Type>;

// is_null_pointer_c
//   concept: satisfied if `_Type` is std::nullptr_t.
template<typename _Type>
concept is_null_pointer_c = std::is_null_pointer_v<_Type>;

// nonvoid
//   concept: satisfied if `_Type` is not void.
// Parallels djinterp::is_nonvoid.
template<typename _Type>
concept nonvoid = !std::is_void_v<_Type>;

// II.1.b  Array concepts

// is_array_c
//   concept: satisfied if `_Type` is an array type.
template<typename _Type>
concept is_array_c = std::is_array_v<_Type>;

// bounded_array
//   concept: satisfied if `_Type` is a bounded array (T[N]).
template<typename _Type>
concept bounded_array = std::is_bounded_array_v<_Type>;

// unbounded_array
//   concept: satisfied if `_Type` is an unbounded array (T[]).
template<typename _Type>
concept unbounded_array = std::is_unbounded_array_v<_Type>;

// II.1.c  Enum concepts

// is_enum_c
//   concept: satisfied if `_Type` is an enumeration type.
template<typename _Type>
concept is_enum_c = std::is_enum_v<_Type>;

// scoped_enum
//   concept: satisfied if `_Type` is a scoped enumeration (enum class).
//template<typename _Type>
//concept scoped_enum = std::is_scoped_enum_v<_Type>;

// unscoped_enum
//   concept: satisfied if `_Type` is an unscoped enumeration.
//template<typename _Type>
//concept unscoped_enum = std::is_enum_v<_Type> && !std::is_scoped_enum_v<_Type>;

// II.1.d  Pointer and reference concepts

// is_pointer_c
//   concept: satisfied if `_Type` is a pointer type.
template<typename _Type>
concept is_pointer_c = std::is_pointer_v<_Type>;

// is_member_pointer_c
//   concept: satisfied if `_Type` is a pointer-to-member.
template<typename _Type>
concept is_member_pointer_c = std::is_member_pointer_v<_Type>;

// is_lvalue_reference_c
//   concept: satisfied if `_Type` is an lvalue reference.
template<typename _Type>
concept is_lvalue_reference_c = std::is_lvalue_reference_v<_Type>;

// is_rvalue_reference_c
//   concept: satisfied if `_Type` is an rvalue reference.
template<typename _Type>
concept is_rvalue_reference_c = std::is_rvalue_reference_v<_Type>;

// is_reference_c
//   concept: satisfied if `_Type` is a reference (lvalue or rvalue).
template<typename _Type>
concept is_reference_c = std::is_reference_v<_Type>;

// II.1.e  Class and function concepts

// is_class_c
//   concept: satisfied if `_Type` is a class type.
template<typename _Type>
concept is_class_c = std::is_class_v<_Type>;

// is_union_c
//   concept: satisfied if `_Type` is a union type.
template<typename _Type>
concept is_union_c = std::is_union_v<_Type>;

// is_function_c
//   concept: satisfied if `_Type` is a function type.
template<typename _Type>
concept is_function_c = std::is_function_v<_Type>;


// -----------------------------------------------------------------------------
// II.2  Composite type concepts
// -----------------------------------------------------------------------------

// is_arithmetic_c
//   concept: satisfied if `_Type` is an arithmetic type.
template<typename _Type>
concept is_arithmetic_c = std::is_arithmetic_v<_Type>;

// is_fundamental_c
//   concept: satisfied if `_Type` is a fundamental type.
template<typename _Type>
concept is_fundamental_c = std::is_fundamental_v<_Type>;

// is_scalar_c
//   concept: satisfied if `_Type` is a scalar type.
template<typename _Type>
concept is_scalar_c = std::is_scalar_v<_Type>;

// is_object_c
//   concept: satisfied if `_Type` is an object type.
template<typename _Type>
concept is_object_c = std::is_object_v<_Type>;

// is_compound_c
//   concept: satisfied if `_Type` is a compound type.
template<typename _Type>
concept is_compound_c = std::is_compound_v<_Type>;


// -----------------------------------------------------------------------------
// II.3  Type property concepts
// -----------------------------------------------------------------------------

// II.3.a  CV-qualification concepts

// is_const_c
//   concept: satisfied if `_Type` is const-qualified.
template<typename _Type>
concept is_const_c = std::is_const_v<_Type>;

// is_volatile_c
//   concept: satisfied if `_Type` is volatile-qualified.
template<typename _Type>
concept is_volatile_c = std::is_volatile_v<_Type>;

// II.3.b  Triviality concepts

// is_trivial_c
//   concept: satisfied if `_Type` is trivial.
template<typename _Type>
concept is_trivial_c = std::is_trivial_v<_Type>;

// is_trivially_copyable_c
//   concept: satisfied if `_Type` is trivially copyable.
template<typename _Type>
concept is_trivially_copyable_c = std::is_trivially_copyable_v<_Type>;

// is_standard_layout_c
//   concept: satisfied if `_Type` has standard layout.
template<typename _Type>
concept is_standard_layout_c = std::is_standard_layout_v<_Type>;

// pod_type
//   concept: satisfied if `_Type` is a POD type (trivial + standard layout).
template<typename _Type>
concept pod_type = std::is_trivial_v<_Type> && std::is_standard_layout_v<_Type>;

// II.3.c  Lifetime / structure concepts

// is_empty_c
//   concept: satisfied if `_Type` is an empty class.
template<typename _Type>
concept is_empty_c = std::is_empty_v<_Type>;

// is_polymorphic_c
//   concept: satisfied if `_Type` is polymorphic (has virtual functions).
template<typename _Type>
concept is_polymorphic_c = std::is_polymorphic_v<_Type>;

// is_abstract_c
//   concept: satisfied if `_Type` is abstract.
template<typename _Type>
concept is_abstract_c = std::is_abstract_v<_Type>;

// is_final_c
//   concept: satisfied if `_Type` is final.
template<typename _Type>
concept is_final_c = std::is_final_v<_Type>;

// is_aggregate_c
//   concept: satisfied if `_Type` is an aggregate.
template<typename _Type>
concept is_aggregate_c = std::is_aggregate_v<_Type>;


// -----------------------------------------------------------------------------
// II.4  Tuple concepts
// -----------------------------------------------------------------------------
// Self-contained tuple support; concept-side companion to the tuple utilities
// in dtuple.hpp. The internal helpers below are intentionally local to keep
// concepts.hpp independent of type_traits.hpp and dtuple.hpp.

NS_INTERNAL
    // is_tuple_impl
    //   trait: detects std::tuple specializations (concept-local helper).
    template<typename _Type>
    struct is_tuple_impl : std::false_type
    {};

    template<typename... _Types>
    struct is_tuple_impl<std::tuple<_Types...>> : std::true_type
    {};

    // is_tuple_homogeneous_impl
    //   trait: detects tuples whose elements are all the same type
    // (concept-local helper).
    template<typename _Tuple>
    struct is_tuple_homogeneous_impl : std::false_type
    {};

    template<typename _Type>
    struct is_tuple_homogeneous_impl<std::tuple<_Type>> : std::true_type
    {};

    template<typename    _Type,
             typename    _Type2,
             typename... _Types>
    struct is_tuple_homogeneous_impl<std::tuple<_Type, _Type2, _Types...>>
        : std::bool_constant<
            std::is_same_v<_Type, _Type2> &&
            is_tuple_homogeneous_impl<std::tuple<_Type2, _Types...>>::value>
    {};
NS_END  // internal

// is_tuple_c
//   concept: satisfied if `_Type` is a std::tuple specialization
// (cv-qualifiers stripped).
template<typename _Type>
concept is_tuple_c = internal::is_tuple_impl<std::remove_cv_t<_Type>>::value;

// tuple_like
//   concept: satisfied if `_Type` is tuple-like (has std::tuple_size and
// std::get specializations).
template<typename _Type>
concept tuple_like = requires
{
    typename std::tuple_size<std::remove_cvref_t<_Type>>::type;

    requires std::derived_from<
        std::tuple_size<std::remove_cvref_t<_Type>>,
        std::integral_constant<std::size_t,
                               std::tuple_size_v<std::remove_cvref_t<_Type>>>
    >;
};

// homogeneous_tuple
//   concept: satisfied if `_Type` is a tuple where all elements have the
// same type.
template<typename _Type>
concept homogeneous_tuple =
    ( is_tuple_c<_Type> &&
      internal::is_tuple_homogeneous_impl<std::remove_cv_t<_Type>>::value );

// empty_tuple
//   concept: satisfied if `_Type` is an empty tuple.
template<typename _Type>
concept empty_tuple =
    ( is_tuple_c<_Type> &&
      (std::tuple_size_v<std::remove_cv_t<_Type>> == 0) );

// nonempty_tuple
//   concept: satisfied if `_Type` is a non-empty tuple.
template<typename _Type>
concept nonempty_tuple =
    ( is_tuple_c<_Type> &&
      (std::tuple_size_v<std::remove_cv_t<_Type>> > 0) );

// single_element_tuple
//   concept: satisfied if `_Type` is a tuple with exactly one element.
template<typename _Type>
concept single_element_tuple =
    ( is_tuple_c<_Type> &&
      (std::tuple_size_v<std::remove_cv_t<_Type>> == 1) );


// -----------------------------------------------------------------------------
// II.5  Class definition rule concepts
// -----------------------------------------------------------------------------
// Parallels djinterp::follows_rule_of_{zero,three,five} in type_traits.hpp.

// follows_rule_of_zero_c
//   concept: satisfied if `_Type` follows the Rule of Zero (all five
// special members are trivially implemented).
template<typename _Type>
concept follows_rule_of_zero_c =
    ( std::is_trivially_copy_constructible_v<_Type> &&
      std::is_trivially_move_constructible_v<_Type> &&
      std::is_trivially_copy_assignable_v<_Type>    &&
      std::is_trivially_move_assignable_v<_Type>    &&
      std::is_trivially_destructible_v<_Type> );

// follows_rule_of_three_c
//   concept: satisfied if `_Type` follows the Rule of Three (copy
// constructor, copy assignment, destructor all defined).
template<typename _Type>
concept follows_rule_of_three_c =
    ( std::is_copy_constructible_v<_Type> &&
      std::is_copy_assignable_v<_Type>    &&
      std::is_destructible_v<_Type> );

// follows_rule_of_five_c
//   concept: satisfied if `_Type` follows the Rule of Five (copy/move
// constructors, copy/move assignment, destructor all defined).
template<typename _Type>
concept follows_rule_of_five_c =
    ( std::is_copy_constructible_v<_Type> &&
      std::is_move_constructible_v<_Type> &&
      std::is_copy_assignable_v<_Type>    &&
      std::is_move_assignable_v<_Type>    &&
      std::is_destructible_v<_Type> );


// -----------------------------------------------------------------------------
// II.6  Container and allocator concepts
// -----------------------------------------------------------------------------
// Parallels djinterp::is_sized, ::has_max_size, ::is_allocator, ::is_bounded
// in type_traits.hpp.

// has_value_type_c
//   concept: satisfied if `_Type` has a value_type member type.
template<typename _Type>
concept has_value_type_c = requires
{
    typename _Type::value_type;
};

// has_size_type_c
//   concept: satisfied if `_Type` has a size_type member type.
template<typename _Type>
concept has_size_type_c = requires
{
    typename _Type::size_type;
};

// has_iterator
//   concept: satisfied if `_Type` has an iterator member type.
template<typename _Type>
concept has_iterator = requires
{
    typename _Type::iterator;
};

// has_const_iterator
//   concept: satisfied if `_Type` has a const_iterator member type.
template<typename _Type>
concept has_const_iterator = requires
{
    typename _Type::const_iterator;
};

// sizeable
//   concept: satisfied if `_Type` has a size_type alias and a size()
// returning a type convertible to std::size_t.
// Parallels djinterp::is_sized.
template<typename _Type>
concept sizeable = requires(const _Type& _t)
{
    typename _Type::size_type;
    { _t.size() } -> std::convertible_to<std::size_t>;
    requires std::convertible_to<typename _Type::size_type, std::size_t>;
};

// has_max_size_c
//   concept: satisfied if `_Type` has a size_type alias and a max_size
// static member convertible to it.
// Parallels djinterp::has_max_size.
template<typename _Type>
concept has_max_size_c = requires
{
    typename _Type::size_type;
    { _Type::max_size } -> std::convertible_to<typename _Type::size_type>;
};

// bounded_c
//   concept: satisfied if `_Type` satisfies the unary concept-like predicate
// `_Concept` and also exposes a max_size member, indicating a bounded
// capacity. Parallels djinterp::is_bounded<_Type, _Trait>.
//
//   Note: because concepts are not first-class template arguments, the
// `_Concept` parameter is taken as a unary trait template (any unary
// `template<typename> class` exposing `::value`).
template<typename                    _Type,
         template<typename> typename _Concept>
concept bounded_c = _Concept<_Type>::value && has_max_size_c<_Type>;

// allocator_c
//   concept: satisfied if `_Type` is an allocator (has allocate/deallocate
// and a value_type accessible via std::allocator_traits).
// Parallels djinterp::is_allocator.
template<typename _Type>
concept allocator_c = requires(_Type _alloc, std::size_t _n)
{
    typename std::allocator_traits<_Type>::value_type;
    { std::allocator_traits<_Type>::allocate(_alloc, _n) };
    { std::allocator_traits<_Type>::deallocate(
        _alloc,
        std::declval<typename std::allocator_traits<_Type>::pointer>(),
        _n) };
};


// -----------------------------------------------------------------------------
// II.7  Template concepts
// -----------------------------------------------------------------------------
// Parallels djinterp::has_nested_template_type, ::has_variadic_constructor,
// ::is_template, ::is_template_with_args, ::is_template_parameter_base_of in
// type_traits.hpp.

NS_INTERNAL
    // is_template_impl
    //   trait: detects empty class-template instantiations
    // (concept-local helper).
    template<typename>
    struct is_template_impl : std::false_type
    {};

    template<template<typename...> typename _Tpl>
    struct is_template_impl<_Tpl<>> : std::true_type
    {};

    // is_template_with_args_impl
    //   trait: detects class-template instantiations with one or more
    // arguments (concept-local helper).
    template<typename>
    struct is_template_with_args_impl : std::false_type
    {};

    template<template<typename...> typename _Tpl,
             typename...                    _Args>
    struct is_template_with_args_impl<_Tpl<_Args...>> : std::true_type
    {};
NS_END  // internal

// has_nested_template_type_c
//   concept: satisfied if `_Type` has a nested template alias named `type`.
// Parallels djinterp::has_nested_template_type.
template<typename _Type>
concept has_nested_template_type_c = requires
{
    typename _Type::template type<int>;
};

// has_variadic_constructor_c
//   concept: satisfied if `_Type` can be constructed from itself.
// Parallels djinterp::has_variadic_constructor.
template<typename _Type>
concept has_variadic_constructor_c = requires
{
    _Type(std::declval<_Type>());
};

// template_parameter_base_of
//   concept: satisfied if `_Type::value_type` is a base of `_Type`.
// Parallels djinterp::is_template_parameter_base_of.
template<typename _Type>
concept template_parameter_base_of = requires
{
    typename _Type::value_type;

    requires std::is_base_of_v<typename _Type::value_type, _Type>;
};

// is_template_c
//   concept: satisfied if `_Type` is a class-template instantiation with no
// arguments (e.g. Foo<>). Parallels djinterp::is_template.
template<typename _Type>
concept is_template_c = internal::is_template_impl<_Type>::value;

// is_template_with_args_c
//   concept: satisfied if `_Type` is a class-template instantiation with
// one or more arguments. Parallels djinterp::is_template_with_args.
template<typename _Type>
concept is_template_with_args_c =
    internal::is_template_with_args_impl<_Type>::value;


// -----------------------------------------------------------------------------
// II.8  Logical concepts
// -----------------------------------------------------------------------------
// Parallels djinterp::conjunction, ::disjunction, ::negation, and
// ::exclusive_disjunction in type_traits.hpp.

// all_of
//   concept: satisfied if every type predicate in `_Bs` is true.
template<typename... _Bs>
concept AllOf = (... && _Bs::value);

// any_of
//   concept: satisfied if at least one type predicate in `_Bs` is true.
template<typename... _Bs>
concept AnyOf = (... || _Bs::value);

// none_of
//   concept: satisfied if no type predicate in `_Bs` is true.
template<typename... _Bs>
concept NoneOf = !(... || _Bs::value);

// exactly_one_of
//   concept: satisfied if exactly one type predicate in `_Bs` is true.
// A clean "one-hot" counterpart to all_of / any_of / none_of.
//
//   Note: this is NOT the same as xor_of below.  exactly_one_of has
// "one-hot" semantics (the count of true predicates is exactly 1);
// xor_of has cumulative pairwise XOR semantics.  For e.g. <T, F, F, F>
// exactly_one_of is true but xor_of is false; for <T, F> they agree
// (both true); for <T, T, T> both are false but for different reasons.
template<typename... _Bs>
concept exactly_one_of = (((_Bs::value ? 1U : 0U) + ...) == 1U);

NS_INTERNAL
    // xor_of_impl
    //   trait: cumulative pairwise XOR (concept-local helper).  Mirrors
    // djinterp::exclusive_disjunction's recurrence:
    //   - 0 args  -> false_type
    //   - 1 arg   -> the predicate itself (takes its bool value)
    //   - 2 args  -> _B1::value != _B2::value
    //   - 3+ args -> (_B1::value != _B2::value) AND xor_of<_Bs...>
    template<typename...>
    struct xor_of_impl : std::false_type
    {};

    template<typename _B1>
    struct xor_of_impl<_B1> : _B1
    {};

    template<typename _B1,
             typename _B2>
    struct xor_of_impl<_B1, _B2>
        : std::bool_constant<bool(_B1::value) != bool(_B2::value)>
    {};

    template<typename    _B1,
             typename    _B2,
             typename... _Bs>
    struct xor_of_impl<_B1, _B2, _Bs...>
        : std::bool_constant<
            (bool(_B1::value) != bool(_B2::value)) &&
            xor_of_impl<_Bs...>::value>
    {};
NS_END  // internal

// xor_of
//   concept: satisfied by the cumulative pairwise XOR of the type
// predicates in `_Bs`.  Parallels djinterp::exclusive_disjunction.
// See the note on exactly_one_of above for how the two differ.
template<typename... _Bs>
concept xor_of = internal::xor_of_impl<_Bs...>::value;


// -----------------------------------------------------------------------------
// II.9  Invocable concepts
// -----------------------------------------------------------------------------
// Parallels djinterp::is_invocable_r, ::is_nothrow_invocable, and
// ::is_nothrow_invocable_r in type_traits.hpp.

// invocable_r
//   concept: satisfied if `_Fn` is invocable with `_Args...` and the result
// is convertible to `_Ret`. Parallels djinterp::is_invocable_r.
template<typename    _Ret,
         typename    _Fn,
         typename... _Args>
concept invocable_r =
    ( std::invocable<_Fn, _Args...> &&
      ( std::is_void_v<_Ret> ||
        std::convertible_to<std::invoke_result_t<_Fn, _Args...>, _Ret> ) );

// nothrow_invocable
//   concept: satisfied if `_Fn` is invocable with `_Args...` without
// throwing. Parallels djinterp::is_nothrow_invocable.
template<typename    _Fn,
         typename... _Args>
concept nothrow_invocable =
    ( std::invocable<_Fn, _Args...> &&
      std::is_nothrow_invocable_v<_Fn, _Args...> );

// nothrow_invocable_r
//   concept: satisfied if `_Fn` is invocable with `_Args...` without
// throwing and the result is convertible to `_Ret`.
// Parallels djinterp::is_nothrow_invocable_r.
template<typename    _Ret,
         typename    _Fn,
         typename... _Args>
concept nothrow_invocable_r =
    ( invocable_r<_Ret, _Fn, _Args...> &&
      std::is_nothrow_invocable_r_v<_Ret, _Fn, _Args...> );


// -----------------------------------------------------------------------------
// II.10  Size and numeric concepts
// -----------------------------------------------------------------------------
// Parallels djinterp::is_valid_size_type, ::is_nonzero, ::is_zero in
// type_traits.hpp.

// valid_size_type
//   concept: satisfied if `_Type` is valid as a size type (unsigned
// arithmetic). Parallels djinterp::is_valid_size_type.
template<typename _Type>
concept valid_size_type =
    ( std::is_unsigned_v<_Type> &&
      std::is_arithmetic_v<_Type> );

// nonzero_size
//   concept: satisfied if `_N` is nonzero. Parallels djinterp::is_nonzero.
template<std::size_t _N>
concept nonzero_size = (_N != 0);

// zero_size
//   concept: satisfied if `_N` is zero. Parallels djinterp::is_zero.
template<std::size_t _N>
concept zero_size = (_N == 0);


// -----------------------------------------------------------------------------
// II.11  Parameter pack concepts
// -----------------------------------------------------------------------------
// Parallels djinterp::is_single_arg, ::is_single_type_arg, and
// ::are_all_nonvoid in type_traits.hpp.

// single_type
//   concept: satisfied if exactly one type is provided.
// Parallels djinterp::is_single_arg (without the `::type` extraction).
template<typename... _Types>
concept single_type = (sizeof...(_Types) == 1);

// empty_pack
//   concept: satisfied if no types are provided.
template<typename... _Types>
concept empty_pack = (sizeof...(_Types) == 0);

// nonempty_pack
//   concept: satisfied if at least one type is provided.
template<typename... _Types>
concept nonempty_pack = (sizeof...(_Types) > 0);

// all_same
//   concept: satisfied if all types in the pack are the same.
template<typename    _First,
         typename... _Rest>
concept all_same = (std::same_as<_First, _Rest> && ...);

// all_convertible_to
//   concept: satisfied if all types are convertible to `_Target`.
template<typename    _Target,
         typename... _Types>
concept all_convertible_to = (std::convertible_to<_Types, _Target> && ...);

// all_derived_from
//   concept: satisfied if all types are derived from `_Base`.
template<typename    _Base,
         typename... _Types>
concept all_derived_from = (std::derived_from<_Types, _Base> && ...);

// single_type_arg
//   concept: satisfied if `_Types` contains exactly one element of type
// `_Type`. Parallels djinterp::is_single_type_arg.
template<typename    _Type,
         typename... _Types>
concept single_type_arg =
    ( single_type<_Types...> &&
      (std::same_as<_Type, _Types> && ...) );

// single_tuple_arg
//   concept: satisfied if `_Types` contains exactly one element and
// that element is a std::tuple specialization.
// Parallels djinterp::is_single_tuple_arg.
//
//   The fold expression is well-formed for sizeof...(_Types) == 0
// (empty fold over `&&` is true) but the leading size check
// short-circuits that case to false, matching the trait's empty-pack
// behaviour.
template<typename... _Types>
concept single_tuple_arg =
    ( single_type<_Types...> &&
      (is_tuple_c<_Types> && ...) );

// nonvoid_pack
//   concept: satisfied if every type in `_Types` is non-void.
// Parallels djinterp::are_all_nonvoid.
template<typename... _Types>
concept nonvoid_pack = ((!std::is_void_v<_Types>) && ...);


NS_END  // djinterp


#endif  // DJINTERP_META_CONCEPTS_