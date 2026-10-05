/*******************************************************************************
* djinterp [core]                                                   concepts.hpp
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
*   The concepts need C++20 and the compiler's support for them, which it
* checks through env.h and env_cpp_features.h (via djinterp.hpp). At any other
* level the header compiles to D_CONCEPT_FROM_TRAIT alone, defined as nothing,
* so a module can include it and write that macro unconditionally; the
* concepts are absent there and their traits remain.
* INDEPENDENCE:
*   This header is designed to be completely independent of type_traits.hpp.
* Code may choose to use either traits-based or concept-based constraints;
* the two are parallel facilities, not layered.
*
*
* path:      /inc/djinterp/core/meta/concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2024.03.21
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_META_CONCEPTS_HPP
#define DJINTERP_META_CONCEPTS_HPP 1

// djinterp
#include "../../djinterp.hpp"


// D_CONCEPT_FROM_TRAIT
//   macro: defines the concept CONCEPT_NAME from a variable-template trait,
//       template<typename Type> concept CONCEPT_NAME = TRAIT_V<Type>;
// so that a concept is the PascalCase face of its trait. Defined at every
// level: below C++20, or where the compiler lacks concepts, it expands to
// nothing, so the concept is absent there and the trait remains -- a module
// may write it unconditionally.
#if ( (D_ENV_LANG_IS_CPP20_OR_HIGHER) &&                                      \
      (D_ENV_CPP_FEATURE_LANG_CONCEPTS) )
    #define D_CONCEPT_FROM_TRAIT(CONCEPT_NAME, TRAIT_V)                       \
        template<typename Type>                                               \
        concept CONCEPT_NAME = TRAIT_V<Type>;
#else
    #define D_CONCEPT_FROM_TRAIT(CONCEPT_NAME, TRAIT_V)
#endif

// everything below is C++20 with compiler support for concepts; at any other
// level this header defines D_CONCEPT_FROM_TRAIT, as nothing, and no concept
#if ( (D_ENV_LANG_IS_CPP20_OR_HIGHER) &&                                      \
      (D_ENV_CPP_FEATURE_LANG_CONCEPTS) )

// std
#include <concepts>     // std::same_as, std::convertible_to, ...
#include <memory>       // std::allocator_traits, std::pointer_traits
#include <tuple>        // std::tuple_size, std::tuple_element
#include <type_traits>  // std::is_*, std::remove_cvref_t, ...


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
//   macro: emits a concept that is satisfied when `Type` has a callable
// member named METHOD_NAME taking no arguments. Concept analog of
// D_TYPE_TRAIT_HAS_METHOD. Renamed from D_CONCEPT_DETECT_METHOD.
#define D_CONCEPT_HAS_METHOD(CONCEPT_NAME, METHOD_NAME)                       \
    template<typename Type>                                                  \
    concept CONCEPT_NAME = requires(Type& _t) {                              \
        _t.METHOD_NAME();                                                     \
    };

// D_CONCEPT_HAS_METHOD_ARGS
//   macro: emits a concept that is satisfied when `Type` has a callable
// member named METHOD_NAME taking an argument of the given type. Concept
// analog of D_TYPE_TRAIT_HAS_METHOD_ARGS. Renamed from
// D_CONCEPT_DETECT_METHOD_ARGS.
//
//   Like the trait's EXPR_METHOD_ARGS probe, the argument is a SINGLE type
// (the call is `M(declval<ARG>())`); this is the shared contract of the
// three _ARGS_ variants below. (Previously this emitted a spurious pack
// expansion `declval<...>()...` that failed to compile at all.)
#define D_CONCEPT_HAS_METHOD_ARGS(CONCEPT_NAME, METHOD_NAME, ...)             \
    template<typename Type>                                                  \
    concept CONCEPT_NAME = requires(Type& _t) {                              \
        _t.METHOD_NAME(std::declval<__VA_ARGS__>());                          \
    };

// D_CONCEPT_HAS_METHOD_TYPED
//   macro: emits a concept that is satisfied when `Type` has a callable
// member named METHOD_NAME returning exactly RET. Concept analog of
// D_TYPE_TRAIT_HAS_METHOD_TYPED. Renamed from D_CONCEPT_DETECT_METHOD_RETURNS.
#define D_CONCEPT_HAS_METHOD_TYPED(CONCEPT_NAME, METHOD_NAME, RET)            \
    template<typename Type>                                                  \
    concept CONCEPT_NAME = requires(Type& _t) {                              \
        { _t.METHOD_NAME() } -> std::same_as<RET>;                            \
    };

// D_CONCEPT_HAS_METHOD_ARGS_TYPED
//   macro: emits a concept that is satisfied when `Type` has a callable
// member named METHOD_NAME taking the given argument types and returning
// exactly RET. Renamed from D_CONCEPT_DETECT_METHOD_ARGS_RETURNS.
#define D_CONCEPT_HAS_METHOD_ARGS_TYPED(CONCEPT_NAME, METHOD_NAME, RET, ...)  \
    template<typename Type>                                                  \
    concept CONCEPT_NAME = requires(Type& _t) {                              \
        { _t.METHOD_NAME(std::declval<__VA_ARGS__>()) }                       \
            -> std::same_as<RET>;                                             \
    };

// D_CONCEPT_HAS_METHOD_CONVERTIBLE
//   macro: emits a concept that is satisfied when `Type` has a callable
// member named METHOD_NAME (no args) whose return type is CONVERTIBLE to RET -
// the looser sibling of D_CONCEPT_HAS_METHOD_TYPED (e.g. a size() returning
// unsigned where size_t is wanted). Concept analog of
// D_TYPE_TRAIT_HAS_METHOD_CONVERTIBLE.
#define D_CONCEPT_HAS_METHOD_CONVERTIBLE(CONCEPT_NAME, METHOD_NAME, RET)      \
    template<typename Type>                                                  \
    concept CONCEPT_NAME = requires(Type& _t) {                              \
        { _t.METHOD_NAME() } -> std::convertible_to<RET>;                     \
    };

// D_CONCEPT_HAS_METHOD_ARGS_CONVERTIBLE
//   macro: emits a concept that is satisfied when `Type` has a callable
// member named METHOD_NAME taking the given argument types and returning a
// type CONVERTIBLE to RET. The args-taking sibling of
// D_CONCEPT_HAS_METHOD_CONVERTIBLE; concept analog of
// D_TYPE_TRAIT_HAS_METHOD_CONVERTIBLE with arguments.
#define D_CONCEPT_HAS_METHOD_ARGS_CONVERTIBLE(CONCEPT_NAME, METHOD_NAME,      \
                                              RET, ...)                       \
    template<typename Type>                                                  \
    concept CONCEPT_NAME = requires(Type& _t) {                              \
        { _t.METHOD_NAME(std::declval<__VA_ARGS__>()) }                       \
            -> std::convertible_to<RET>;                                      \
    };

// D_CONCEPT_HAS_TYPE
//   macro: emits a concept that is satisfied when `Type` has a nested
// type alias named TYPE_NAME. Concept analog of D_TYPE_TRAIT_HAS_TYPE.
// Renamed from D_CONCEPT_DETECT_TYPE.
#define D_CONCEPT_HAS_TYPE(CONCEPT_NAME, TYPE_NAME)                           \
    template<typename Type>                                                  \
    concept CONCEPT_NAME = requires {                                         \
        typename Type::TYPE_NAME;                                            \
    };

// D_CONCEPT_HAS_STATIC_MEMBER
//   macro: emits a concept that is satisfied when `Type` has a static
// member named MEMBER (of any kind). Concept analog of
// D_TYPE_TRAIT_HAS_STATIC_MEMBER. Renamed from D_CONCEPT_DETECT_STATIC.
#define D_CONCEPT_HAS_STATIC_MEMBER(CONCEPT_NAME, MEMBER)                     \
    template<typename Type>                                                  \
    concept CONCEPT_NAME = requires {                                         \
        Type::MEMBER;                                                        \
    };

// D_CONCEPT_HAS_BINARY_OP
//   macro: emits a concept that is satisfied when two (const) `Type` operands
// support the binary operator OP (e.g. +, ==, <). Operands are `const Type&`,
// matching the trait probe, so the concept is not defeated by const operands.
// Concept analog of D_TYPE_TRAIT_HAS_BINARY_OP.
#define D_CONCEPT_HAS_BINARY_OP(CONCEPT_NAME, OP)                             \
    template<typename Type>                                                  \
    concept CONCEPT_NAME = requires(const Type& _a, const Type& _b) {       \
        _a OP _b;                                                             \
    };

// D_CONCEPT_HAS_UNARY_OP
//   macro: emits a concept that is satisfied when a `Type` operand supports
// the prefix unary operator OP (e.g. -, !, *, ++). Concept analog of
// D_TYPE_TRAIT_HAS_UNARY_OP.
#define D_CONCEPT_HAS_UNARY_OP(CONCEPT_NAME, OP)                              \
    template<typename Type>                                                  \
    concept CONCEPT_NAME = requires(Type& _t) {                              \
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
//   concept: satisfied if `Type` is (cv-qualified) void.
template<typename Type>
concept is_void_c = std::is_void_v<Type>;

// is_null_pointer_c
//   concept: satisfied if `Type` is std::nullptr_t.
template<typename Type>
concept is_null_pointer_c = std::is_null_pointer_v<Type>;

// nonvoid
//   concept: satisfied if `Type` is not void.
// Parallels djinterp::is_nonvoid.
template<typename Type>
concept nonvoid = !std::is_void_v<Type>;

// II.1.b  Array concepts

// is_array_c
//   concept: satisfied if `Type` is an array type.
template<typename Type>
concept is_array_c = std::is_array_v<Type>;

// bounded_array
//   concept: satisfied if `Type` is a bounded array (T[N]).
template<typename Type>
concept bounded_array = std::is_bounded_array_v<Type>;

// unbounded_array
//   concept: satisfied if `Type` is an unbounded array (T[]).
template<typename Type>
concept unbounded_array = std::is_unbounded_array_v<Type>;

// II.1.c  Enum concepts

// is_enum_c
//   concept: satisfied if `Type` is an enumeration type.
template<typename Type>
concept is_enum_c = std::is_enum_v<Type>;

// scoped_enum
//   concept: satisfied if `Type` is a scoped enumeration (enum class).
//template<typename Type>
//concept scoped_enum = std::is_scoped_enum_v<Type>;

// unscoped_enum
//   concept: satisfied if `Type` is an unscoped enumeration.
//template<typename Type>
//concept unscoped_enum = std::is_enum_v<Type> && !std::is_scoped_enum_v<Type>;

// II.1.d  Pointer and reference concepts

// is_pointer_c
//   concept: satisfied if `Type` is a pointer type.
template<typename Type>
concept is_pointer_c = std::is_pointer_v<Type>;

// is_member_pointer_c
//   concept: satisfied if `Type` is a pointer-to-member.
template<typename Type>
concept is_member_pointer_c = std::is_member_pointer_v<Type>;

// is_lvalue_reference_c
//   concept: satisfied if `Type` is an lvalue reference.
template<typename Type>
concept is_lvalue_reference_c = std::is_lvalue_reference_v<Type>;

// is_rvalue_reference_c
//   concept: satisfied if `Type` is an rvalue reference.
template<typename Type>
concept is_rvalue_reference_c = std::is_rvalue_reference_v<Type>;

// is_reference_c
//   concept: satisfied if `Type` is a reference (lvalue or rvalue).
template<typename Type>
concept is_reference_c = std::is_reference_v<Type>;

// II.1.e  Class and function concepts

// is_class_c
//   concept: satisfied if `Type` is a class type.
template<typename Type>
concept is_class_c = std::is_class_v<Type>;

// is_union_c
//   concept: satisfied if `Type` is a union type.
template<typename Type>
concept is_union_c = std::is_union_v<Type>;

// is_function_c
//   concept: satisfied if `Type` is a function type.
template<typename Type>
concept is_function_c = std::is_function_v<Type>;


// -----------------------------------------------------------------------------
// II.2  Composite type concepts
// -----------------------------------------------------------------------------

// is_arithmetic_c
//   concept: satisfied if `Type` is an arithmetic type.
template<typename Type>
concept is_arithmetic_c = std::is_arithmetic_v<Type>;

// is_fundamental_c
//   concept: satisfied if `Type` is a fundamental type.
template<typename Type>
concept is_fundamental_c = std::is_fundamental_v<Type>;

// is_scalar_c
//   concept: satisfied if `Type` is a scalar type.
template<typename Type>
concept is_scalar_c = std::is_scalar_v<Type>;

// is_object_c
//   concept: satisfied if `Type` is an object type.
template<typename Type>
concept is_object_c = std::is_object_v<Type>;

// is_compound_c
//   concept: satisfied if `Type` is a compound type.
template<typename Type>
concept is_compound_c = std::is_compound_v<Type>;


// -----------------------------------------------------------------------------
// II.3  Type property concepts
// -----------------------------------------------------------------------------

// II.3.a  CV-qualification concepts

// is_const_c
//   concept: satisfied if `Type` is const-qualified.
template<typename Type>
concept is_const_c = std::is_const_v<Type>;

// is_volatile_c
//   concept: satisfied if `Type` is volatile-qualified.
template<typename Type>
concept is_volatile_c = std::is_volatile_v<Type>;

// II.3.b  Triviality concepts

// is_trivial_c
//   concept: satisfied if `Type` is trivial.
template<typename Type>
concept is_trivial_c = std::is_trivial_v<Type>;

// is_trivially_copyable_c
//   concept: satisfied if `Type` is trivially copyable.
template<typename Type>
concept is_trivially_copyable_c = std::is_trivially_copyable_v<Type>;

// is_standard_layout_c
//   concept: satisfied if `Type` has standard layout.
template<typename Type>
concept is_standard_layout_c = std::is_standard_layout_v<Type>;

// pod_type
//   concept: satisfied if `Type` is a POD type (trivial + standard layout).
template<typename Type>
concept pod_type = std::is_trivial_v<Type> && std::is_standard_layout_v<Type>;

// II.3.c  Lifetime / structure concepts

// is_empty_c
//   concept: satisfied if `Type` is an empty class.
template<typename Type>
concept is_empty_c = std::is_empty_v<Type>;

// is_polymorphic_c
//   concept: satisfied if `Type` is polymorphic (has virtual functions).
template<typename Type>
concept is_polymorphic_c = std::is_polymorphic_v<Type>;

// is_abstract_c
//   concept: satisfied if `Type` is abstract.
template<typename Type>
concept is_abstract_c = std::is_abstract_v<Type>;

// is_final_c
//   concept: satisfied if `Type` is final.
template<typename Type>
concept is_final_c = std::is_final_v<Type>;

// is_aggregate_c
//   concept: satisfied if `Type` is an aggregate.
template<typename Type>
concept is_aggregate_c = std::is_aggregate_v<Type>;


// -----------------------------------------------------------------------------
// II.4  Tuple concepts
// -----------------------------------------------------------------------------
// Self-contained tuple support; concept-side companion to the tuple utilities
// in dtuple.hpp. The internal helpers below are intentionally local to keep
// concepts.hpp independent of type_traits.hpp and dtuple.hpp.

NS_INTERNAL
    // is_tuple_impl
    //   trait: detects std::tuple specializations (concept-local helper).
    template<typename Type>
    struct is_tuple_impl : std::false_type
    {};

    template<typename... Types>
    struct is_tuple_impl<std::tuple<Types...>> : std::true_type
    {};

    // is_tuple_homogeneous_impl
    //   trait: detects tuples whose elements are all the same type
    // (concept-local helper).
    template<typename Tuple>
    struct is_tuple_homogeneous_impl : std::false_type
    {};

    template<typename Type>
    struct is_tuple_homogeneous_impl<std::tuple<Type>> : std::true_type
    {};

    template<typename    Type,
             typename    Type2,
             typename... Types>
    struct is_tuple_homogeneous_impl<std::tuple<Type, Type2, Types...>>
        : std::bool_constant<
            std::is_same_v<Type, Type2> &&
            is_tuple_homogeneous_impl<std::tuple<Type2, Types...>>::value>
    {};
NS_END  // internal

// is_tuple_c
//   concept: satisfied if `Type` is a std::tuple specialization
// (cv-qualifiers stripped).
template<typename Type>
concept is_tuple_c = internal::is_tuple_impl<std::remove_cv_t<Type>>::value;

// tuple_like
//   concept: satisfied if `Type` is tuple-like (has std::tuple_size and
// std::get specializations).
template<typename Type>
concept tuple_like = requires
{
    typename std::tuple_size<std::remove_cvref_t<Type>>::type;

    requires std::derived_from<
        std::tuple_size<std::remove_cvref_t<Type>>,
        std::integral_constant<std::size_t,
                               std::tuple_size_v<std::remove_cvref_t<Type>>>
    >;
};

// homogeneous_tuple
//   concept: satisfied if `Type` is a tuple where all elements have the
// same type.
template<typename Type>
concept homogeneous_tuple =
    ( is_tuple_c<Type> &&
      internal::is_tuple_homogeneous_impl<std::remove_cv_t<Type>>::value );

// empty_tuple
//   concept: satisfied if `Type` is an empty tuple.
template<typename Type>
concept empty_tuple =
    ( is_tuple_c<Type> &&
      (std::tuple_size_v<std::remove_cv_t<Type>> == 0) );

// nonempty_tuple
//   concept: satisfied if `Type` is a non-empty tuple.
template<typename Type>
concept nonempty_tuple =
    ( is_tuple_c<Type> &&
      (std::tuple_size_v<std::remove_cv_t<Type>> > 0) );

// single_element_tuple
//   concept: satisfied if `Type` is a tuple with exactly one element.
template<typename Type>
concept single_element_tuple =
    ( is_tuple_c<Type> &&
      (std::tuple_size_v<std::remove_cv_t<Type>> == 1) );


// -----------------------------------------------------------------------------
// II.5  Class definition rule concepts
// -----------------------------------------------------------------------------
// Parallels djinterp::follows_rule_of_{zero,three,five} in type_traits.hpp.

// follows_rule_of_zero_c
//   concept: satisfied if `Type` follows the Rule of Zero (all five
// special members are trivially implemented).
template<typename Type>
concept follows_rule_of_zero_c =
    ( std::is_trivially_copy_constructible_v<Type> &&
      std::is_trivially_move_constructible_v<Type> &&
      std::is_trivially_copy_assignable_v<Type>    &&
      std::is_trivially_move_assignable_v<Type>    &&
      std::is_trivially_destructible_v<Type> );

// follows_rule_of_three_c
//   concept: satisfied if `Type` follows the Rule of Three (copy
// constructor, copy assignment, destructor all defined).
template<typename Type>
concept follows_rule_of_three_c =
    ( std::is_copy_constructible_v<Type> &&
      std::is_copy_assignable_v<Type>    &&
      std::is_destructible_v<Type> );

// follows_rule_of_five_c
//   concept: satisfied if `Type` follows the Rule of Five (copy/move
// constructors, copy/move assignment, destructor all defined).
template<typename Type>
concept follows_rule_of_five_c =
    ( std::is_copy_constructible_v<Type> &&
      std::is_move_constructible_v<Type> &&
      std::is_copy_assignable_v<Type>    &&
      std::is_move_assignable_v<Type>    &&
      std::is_destructible_v<Type> );


// -----------------------------------------------------------------------------
// II.6  Container and allocator concepts
// -----------------------------------------------------------------------------
// Parallels djinterp::is_sized, ::has_max_size, ::is_allocator, ::is_bounded
// in type_traits.hpp.

// has_value_type_c
//   concept: satisfied if `Type` has a value_type member type.
template<typename Type>
concept has_value_type_c = requires
{
    typename Type::value_type;
};

// has_size_type_c
//   concept: satisfied if `Type` has a size_type member type.
template<typename Type>
concept has_size_type_c = requires
{
    typename Type::size_type;
};

// has_iterator
//   concept: satisfied if `Type` has an iterator member type.
template<typename Type>
concept has_iterator = requires
{
    typename Type::iterator;
};

// has_const_iterator
//   concept: satisfied if `Type` has a const_iterator member type.
template<typename Type>
concept has_const_iterator = requires
{
    typename Type::const_iterator;
};

// sizeable
//   concept: satisfied if `Type` has a size_type alias and a size()
// returning a type convertible to std::size_t.
// Parallels djinterp::is_sized.
template<typename Type>
concept sizeable = requires(const Type& _t)
{
    typename Type::size_type;
    { _t.size() } -> std::convertible_to<std::size_t>;
    requires std::convertible_to<typename Type::size_type, std::size_t>;
};

// has_max_size_c
//   concept: satisfied if `Type` has a size_type alias and a max_size
// static member convertible to it.
// Parallels djinterp::has_max_size.
template<typename Type>
concept has_max_size_c = requires
{
    typename Type::size_type;
    { Type::max_size } -> std::convertible_to<typename Type::size_type>;
};

// bounded_c
//   concept: satisfied if `Type` satisfies the unary concept-like predicate
// `Concept` and also exposes a max_size member, indicating a bounded
// capacity. Parallels djinterp::is_bounded<Type, _Trait>.
//
//   Note: because concepts are not first-class template arguments, the
// `Concept` parameter is taken as a unary trait template (any unary
// `template<typename> class` exposing `::value`).
template<typename                    Type,
         template<typename> typename Concept>
concept bounded_c = Concept<Type>::value && has_max_size_c<Type>;

// allocator_c
//   concept: satisfied if `Type` is an allocator (has allocate/deallocate
// and a value_type accessible via std::allocator_traits).
// Parallels djinterp::is_allocator.
template<typename Type>
concept allocator_c = requires(Type _alloc, std::size_t _n)
{
    typename std::allocator_traits<Type>::value_type;
    { std::allocator_traits<Type>::allocate(_alloc, _n) };
    { std::allocator_traits<Type>::deallocate(
        _alloc,
        std::declval<typename std::allocator_traits<Type>::pointer>(),
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

    template<template<typename...> typename Tpl>
    struct is_template_impl<Tpl<>> : std::true_type
    {};

    // is_template_with_args_impl
    //   trait: detects class-template instantiations with one or more
    // arguments (concept-local helper).
    template<typename>
    struct is_template_with_args_impl : std::false_type
    {};

    template<template<typename...> typename Tpl,
             typename...                    Args>
    struct is_template_with_args_impl<Tpl<Args...>> : std::true_type
    {};
NS_END  // internal

// has_nested_template_type_c
//   concept: satisfied if `Type` has a nested template alias named `type`.
// Parallels djinterp::has_nested_template_type.
template<typename Type>
concept has_nested_template_type_c = requires
{
    typename Type::template type<int>;
};

// has_variadic_constructor_c
//   concept: satisfied if `Type` can be constructed from itself.
// Parallels djinterp::has_variadic_constructor.
template<typename Type>
concept has_variadic_constructor_c = requires
{
    Type(std::declval<Type>());
};

// template_parameter_base_of
//   concept: satisfied if `Type::value_type` is a base of `Type`.
// Parallels djinterp::is_template_parameter_base_of.
template<typename Type>
concept template_parameter_base_of = requires
{
    typename Type::value_type;

    requires std::is_base_of_v<typename Type::value_type, Type>;
};

// is_template_c
//   concept: satisfied if `Type` is a class-template instantiation with no
// arguments (e.g. Foo<>). Parallels djinterp::is_template.
template<typename Type>
concept is_template_c = internal::is_template_impl<Type>::value;

// is_template_with_args_c
//   concept: satisfied if `Type` is a class-template instantiation with
// one or more arguments. Parallels djinterp::is_template_with_args.
template<typename Type>
concept is_template_with_args_c =
    internal::is_template_with_args_impl<Type>::value;


// -----------------------------------------------------------------------------
// II.8  Logical concepts
// -----------------------------------------------------------------------------
// Parallels djinterp::conjunction, ::disjunction, ::negation, and
// ::exclusive_disjunction in type_traits.hpp.

// all_of
//   concept: satisfied if every type predicate in `Bs` is true.
template<typename... Bs>
concept AllOf = (... && Bs::value);

// any_of
//   concept: satisfied if at least one type predicate in `Bs` is true.
template<typename... Bs>
concept AnyOf = (... || Bs::value);

// none_of
//   concept: satisfied if no type predicate in `Bs` is true.
template<typename... Bs>
concept NoneOf = !(... || Bs::value);

// exactly_one_of
//   concept: satisfied if exactly one type predicate in `Bs` is true.
// A clean "one-hot" counterpart to all_of / any_of / none_of.
//
//   Note: this is NOT the same as xor_of below.  exactly_one_of has
// "one-hot" semantics (the count of true predicates is exactly 1);
// xor_of has cumulative pairwise XOR semantics.  For e.g. <T, F, F, F>
// exactly_one_of is true but xor_of is false; for <T, F> they agree
// (both true); for <T, T, T> both are false but for different reasons.
template<typename... Bs>
concept exactly_one_of = (((Bs::value ? 1U : 0U) + ...) == 1U);

NS_INTERNAL
    // xor_of_impl
    //   trait: cumulative pairwise XOR (concept-local helper).  Mirrors
    // djinterp::exclusive_disjunction's recurrence:
    //   - 0 args  -> false_type
    //   - 1 arg   -> the predicate itself (takes its bool value)
    //   - 2 args  -> B1::value != B2::value
    //   - 3+ args -> (B1::value != B2::value) AND xor_of<Bs...>
    template<typename...>
    struct xor_of_impl : std::false_type
    {};

    template<typename B1>
    struct xor_of_impl<B1> : B1
    {};

    template<typename B1,
             typename B2>
    struct xor_of_impl<B1, B2>
        : std::bool_constant<bool(B1::value) != bool(B2::value)>
    {};

    template<typename    B1,
             typename    B2,
             typename... Bs>
    struct xor_of_impl<B1, B2, Bs...>
        : std::bool_constant<
            (bool(B1::value) != bool(B2::value)) &&
            xor_of_impl<Bs...>::value>
    {};
NS_END  // internal

// xor_of
//   concept: satisfied by the cumulative pairwise XOR of the type
// predicates in `Bs`.  Parallels djinterp::exclusive_disjunction.
// See the note on exactly_one_of above for how the two differ.
template<typename... Bs>
concept xor_of = internal::xor_of_impl<Bs...>::value;


// -----------------------------------------------------------------------------
// II.9  Invocable concepts
// -----------------------------------------------------------------------------
// Parallels djinterp::is_invocable_r, ::is_nothrow_invocable, and
// ::is_nothrow_invocable_r in type_traits.hpp.

// invocable_r
//   concept: satisfied if `Fn` is invocable with `Args...` and the result
// is convertible to `Ret`. Parallels djinterp::is_invocable_r.
template<typename    Ret,
         typename    Fn,
         typename... Args>
concept invocable_r =
    ( std::invocable<Fn, Args...> &&
      ( std::is_void_v<Ret> ||
        std::convertible_to<std::invoke_result_t<Fn, Args...>, Ret> ) );

// nothrow_invocable
//   concept: satisfied if `Fn` is invocable with `Args...` without
// throwing. Parallels djinterp::is_nothrow_invocable.
template<typename    Fn,
         typename... Args>
concept nothrow_invocable =
    ( std::invocable<Fn, Args...> &&
      std::is_nothrow_invocable_v<Fn, Args...> );

// nothrow_invocable_r
//   concept: satisfied if `Fn` is invocable with `Args...` without
// throwing and the result is convertible to `Ret`.
// Parallels djinterp::is_nothrow_invocable_r.
template<typename    Ret,
         typename    Fn,
         typename... Args>
concept nothrow_invocable_r =
    ( invocable_r<Ret, Fn, Args...> &&
      std::is_nothrow_invocable_r_v<Ret, Fn, Args...> );


// -----------------------------------------------------------------------------
// II.10  Size and numeric concepts
// -----------------------------------------------------------------------------
// Parallels djinterp::is_valid_size_type, ::is_nonzero, ::is_zero in
// type_traits.hpp.

// valid_size_type
//   concept: satisfied if `Type` is valid as a size type (unsigned
// arithmetic). Parallels djinterp::is_valid_size_type.
template<typename Type>
concept valid_size_type =
    ( std::is_unsigned_v<Type> &&
      std::is_arithmetic_v<Type> );

// nonzero_size
//   concept: satisfied if `N` is nonzero. Parallels djinterp::is_nonzero.
template<std::size_t N>
concept nonzero_size = (N != 0);

// zero_size
//   concept: satisfied if `N` is zero. Parallels djinterp::is_zero.
template<std::size_t N>
concept zero_size = (N == 0);


// -----------------------------------------------------------------------------
// II.11  Parameter pack concepts
// -----------------------------------------------------------------------------
// Parallels djinterp::is_single_arg, ::is_single_type_arg, and
// ::are_all_nonvoid in type_traits.hpp.

// single_type
//   concept: satisfied if exactly one type is provided.
// Parallels djinterp::is_single_arg (without the `::type` extraction).
template<typename... Types>
concept single_type = (sizeof...(Types) == 1);

// empty_pack
//   concept: satisfied if no types are provided.
template<typename... Types>
concept empty_pack = (sizeof...(Types) == 0);

// nonempty_pack
//   concept: satisfied if at least one type is provided.
template<typename... Types>
concept nonempty_pack = (sizeof...(Types) > 0);

// all_same
//   concept: satisfied if all types in the pack are the same.
template<typename    First,
         typename... Rest>
concept all_same = (std::same_as<First, Rest> && ...);

// all_convertible_to
//   concept: satisfied if all types are convertible to `Target`.
template<typename    Target,
         typename... Types>
concept all_convertible_to = (std::convertible_to<Types, Target> && ...);

// all_derived_from
//   concept: satisfied if all types are derived from `Base`.
template<typename    Base,
         typename... Types>
concept all_derived_from = (std::derived_from<Types, Base> && ...);

// single_type_arg
//   concept: satisfied if `Types` contains exactly one element of type
// `Type`. Parallels djinterp::is_single_type_arg.
template<typename    Type,
         typename... Types>
concept single_type_arg =
    ( single_type<Types...> &&
      (std::same_as<Type, Types> && ...) );

// single_tuple_arg
//   concept: satisfied if `Types` contains exactly one element and
// that element is a std::tuple specialization.
// Parallels djinterp::is_single_tuple_arg.
//
//   The fold expression is well-formed for sizeof...(Types) == 0
// (empty fold over `&&` is true) but the leading size check
// short-circuits that case to false, matching the trait's empty-pack
// behaviour.
template<typename... Types>
concept single_tuple_arg =
    ( single_type<Types...> &&
      (is_tuple_c<Types> && ...) );

// nonvoid_pack
//   concept: satisfied if every type in `Types` is non-void.
// Parallels djinterp::are_all_nonvoid.
template<typename... Types>
concept nonvoid_pack = ((!std::is_void_v<Types>) && ...);


NS_END  // djinterp


#endif  // C++20 with concepts

#endif  // DJINTERP_META_CONCEPTS_HPP
