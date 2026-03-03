/******************************************************************************
* djinterp [core]                                                 concepts.hpp
*
* djinterp concepts header:
*   This header provides C++20 concept definitions that parallel the type
* traits in type_traits.hpp. It includes:
*   - standard library concept re-exports
*   - custom concept definition macros
*   - tuple introspection concepts
*   - class definition rule concepts (Rule of Zero/Three/Five)
*   - container/allocator concepts
*
*   REQUIREMENTS:
*   This header requires C++20 or later. It uses `env.h` and `cpp_features.h` 
* for feature detection to verify concept support is available.
*
*   INDEPENDENCE:
*   This header is designed to be completely independent of type_traits.hpp.
* Code can choose to use either traits-based or concept-based constraints.
*
*
* path:      \inc\meta\concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2024.03.21
******************************************************************************/

#ifndef DJINTERP_CONCEPTS_
#define DJINTERP_CONCEPTS_ 1

#include <concepts>
#include <type_traits>
#include <memory>
#include <tuple>
#include ".\djinterp.h"
#include "..\cpp_features.h"


// require C++20 for concepts
#if !D_ENV_LANG_IS_CPP20_OR_HIGHER
    #error "concepts.hpp requires C++20 or later"
#endif

// verify concept support
#if !D_ENV_CPP_FEATURE_LANG_CONCEPTS
    #error "concepts.hpp requires compiler support for concepts (__cpp_concepts)"
#endif


// =============================================================================
// 0.   STANDARD LIBRARY CONCEPTS
// =============================================================================
// Re-export standard library concepts for convenience. These are the concept
// equivalents of many standard type traits.

NS_DJINTERP

// -----------------------------------------------------------------------------
// 0.1  Core language concepts
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
// 0.2  Comparison concepts
// -----------------------------------------------------------------------------

using std::equality_comparable;
using std::equality_comparable_with;
using std::totally_ordered;
using std::totally_ordered_with;

// -----------------------------------------------------------------------------
// 0.3  Object concepts
// -----------------------------------------------------------------------------

using std::movable;
using std::copyable;
using std::semiregular;
using std::regular;

// -----------------------------------------------------------------------------
// 0.4  Callable concepts
// -----------------------------------------------------------------------------

using std::invocable;
using std::regular_invocable;
using std::predicate;
using std::relation;
using std::equivalence_relation;
using std::strict_weak_order;


// =============================================================================
// I.   CONCEPT DEFINITION MACROS
// =============================================================================
// These macros provide convenient ways to define concepts with common patterns.

}   // namespace djinterp (close for macros)

// D_CONCEPT_DETECT_METHOD
//   macro: creates a concept that detects if T has METHOD()
#define D_CONCEPT_DETECT_METHOD(CONCEPT_NAME, METHOD_NAME)                      \
    template<typename _Type>                                                    \
    concept CONCEPT_NAME = requires(_Type& t) {                                 \
        t.METHOD_NAME();                                                        \
    };

// D_CONCEPT_DETECT_METHOD_ARGS
//   macro: creates a concept that detects if T has METHOD(args...)
#define D_CONCEPT_DETECT_METHOD_ARGS(CONCEPT_NAME, METHOD_NAME, ...)            \
    template<typename _Type>                                                    \
    concept CONCEPT_NAME = requires(_Type& t) {                                 \
        t.METHOD_NAME(std::declval<__VA_ARGS__>()...);                          \
    };

// D_CONCEPT_DETECT_METHOD_RETURNS
//   macro: creates a concept that detects if T has METHOD() returning RET
#define D_CONCEPT_DETECT_METHOD_RETURNS(CONCEPT_NAME, METHOD_NAME, RET)         \
    template<typename _Type>                                                    \
    concept CONCEPT_NAME = requires(_Type& t) {                                 \
        { t.METHOD_NAME() } -> std::same_as<RET>;                               \
    };

// D_CONCEPT_DETECT_METHOD_ARGS_RETURNS
//   macro: creates a concept that detects if T has METHOD(args...) returning RET
#define D_CONCEPT_DETECT_METHOD_ARGS_RETURNS(CONCEPT_NAME, METHOD_NAME, RET, ...)\
    template<typename _Type>                                                    \
    concept CONCEPT_NAME = requires(_Type& t) {                                 \
        { t.METHOD_NAME(std::declval<__VA_ARGS__>()...) } -> std::same_as<RET>; \
    };

// D_CONCEPT_DETECT_TYPE
//   macro: creates a concept that detects if T has nested type TYPE_NAME
#define D_CONCEPT_DETECT_TYPE(CONCEPT_NAME, TYPE_NAME)                          \
    template<typename _Type>                                                    \
    concept CONCEPT_NAME = requires {                                           \
        typename _Type::TYPE_NAME;                                              \
    };

// D_CONCEPT_DETECT_STATIC
//   macro: creates a concept that detects if T has static member MEMBER
#define D_CONCEPT_DETECT_STATIC(CONCEPT_NAME, MEMBER)                           \
    template<typename _Type>                                                    \
    concept CONCEPT_NAME = requires {                                           \
        _Type::MEMBER;                                                          \
    };

// =============================================================================
// II.  FUNDAMENTAL TYPE CONCEPTS
// =============================================================================
// Basic type classification concepts that parallel standard type traits.

// -----------------------------------------------------------------------------
// 2.1  Void and null concepts
// -----------------------------------------------------------------------------

// is_void_c
//   concept: satisfied if T is void (cv-qualified)
template<typename _Type>
concept is_void_c = std::is_void_v<_Type>;

// is_null_pointer_c
//   concept: satisfied if T is std::nullptr_t
template<typename _Type>
concept is_null_pointer_c = std::is_null_pointer_v<_Type>;

// nonvoid
//   concept: satisfied if T is not void
template<typename _Type>
concept nonvoid = !std::is_void_v<_Type>;

// -----------------------------------------------------------------------------
// 2.2  Array concepts
// -----------------------------------------------------------------------------

// is_array_c
//   concept: satisfied if T is an array type
template<typename _Type>
concept is_array_c = std::is_array_v<_Type>;

// bounded_array
//   concept: satisfied if T is a bounded array (T[N])
template<typename _Type>
concept bounded_array = std::is_bounded_array_v<_Type>;

// unbounded_array
//   concept: satisfied if T is an unbounded array (T[])
template<typename _Type>
concept unbounded_array = std::is_unbounded_array_v<_Type>;

// -----------------------------------------------------------------------------
// 2.3  Enum concepts
// -----------------------------------------------------------------------------

// is_enum_c
//   concept: satisfied if T is an enumeration type
template<typename _Type>
concept is_enum_c = std::is_enum_v<_Type>;

// scoped_enum
//   concept: satisfied if T is a scoped enumeration (enum class)
template<typename _Type>
concept scoped_enum = std::is_scoped_enum_v<_Type>;

// unscoped_enum
//   concept: satisfied if T is an unscoped enumeration
template<typename _Type>
concept unscoped_enum = std::is_enum_v<_Type> && !std::is_scoped_enum_v<_Type>;

// -----------------------------------------------------------------------------
// 2.4  Pointer and reference concepts
// -----------------------------------------------------------------------------

// is_pointer_c
//   concept: satisfied if T is a pointer type
template<typename _Type>
concept is_pointer_c = std::is_pointer_v<_Type>;

// is_member_pointer_c
//   concept: satisfied if T is a pointer to member
template<typename _Type>
concept is_member_pointer_c = std::is_member_pointer_v<_Type>;

// is_lvalue_reference_c
//   concept: satisfied if T is an lvalue reference
template<typename _Type>
concept is_lvalue_reference_c = std::is_lvalue_reference_v<_Type>;

// is_rvalue_reference_c
//   concept: satisfied if T is an rvalue reference
template<typename _Type>
concept is_rvalue_reference_c = std::is_rvalue_reference_v<_Type>;

// is_reference_c
//   concept: satisfied if T is a reference (lvalue or rvalue)
template<typename _Type>
concept is_reference_c = std::is_reference_v<_Type>;

// -----------------------------------------------------------------------------
// 2.5  Class and function concepts
// -----------------------------------------------------------------------------

// is_class_c
//   concept: satisfied if T is a class type
template<typename _Type>
concept is_class_c = std::is_class_v<_Type>;

// is_union_c
//   concept: satisfied if T is a union type
template<typename _Type>
concept is_union_c = std::is_union_v<_Type>;

// is_function_c
//   concept: satisfied if T is a function type
template<typename _Type>
concept is_function_c = std::is_function_v<_Type>;


// =============================================================================
// III. COMPOSITE TYPE CONCEPTS
// =============================================================================

// is_arithmetic_c
//   concept: satisfied if T is an arithmetic type
template<typename _Type>
concept is_arithmetic_c = std::is_arithmetic_v<_Type>;

// is_fundamental_c
//   concept: satisfied if T is a fundamental type
template<typename _Type>
concept is_fundamental_c = std::is_fundamental_v<_Type>;

// is_scalar_c
//   concept: satisfied if T is a scalar type
template<typename _Type>
concept is_scalar_c = std::is_scalar_v<_Type>;

// is_object_c
//   concept: satisfied if T is an object type
template<typename _Type>
concept is_object_c = std::is_object_v<_Type>;

// is_compound_c
//   concept: satisfied if T is a compound type
template<typename _Type>
concept is_compound_c = std::is_compound_v<_Type>;


// =============================================================================
// IV.  TYPE PROPERTY CONCEPTS
// =============================================================================

// -----------------------------------------------------------------------------
// 4.1  CV-qualification concepts
// -----------------------------------------------------------------------------

// is_const_c
//   concept: satisfied if T is const-qualified
template<typename _Type>
concept is_const_c = std::is_const_v<_Type>;

// is_volatile_c
//   concept: satisfied if T is volatile-qualified
template<typename _Type>
concept is_volatile_c = std::is_volatile_v<_Type>;

// -----------------------------------------------------------------------------
// 4.2  Triviality concepts
// -----------------------------------------------------------------------------

// is_trivial_c
//   concept: satisfied if T is trivial
template<typename _Type>
concept is_trivial_c = std::is_trivial_v<_Type>;

// is_trivially_copyable_c
//   concept: satisfied if T is trivially copyable
template<typename _Type>
concept is_trivially_copyable_c = std::is_trivially_copyable_v<_Type>;

// is_standard_layout_c
//   concept: satisfied if T has standard layout
template<typename _Type>
concept is_standard_layout_c = std::is_standard_layout_v<_Type>;

// pod_type
//   concept: satisfied if T is a POD type (trivial + standard layout)
template<typename _Type>
concept pod_type = std::is_trivial_v<_Type> && std::is_standard_layout_v<_Type>;

// -----------------------------------------------------------------------------
// 4.3  Lifetime concepts
// -----------------------------------------------------------------------------

// is_empty_c
//   concept: satisfied if T is an empty class
template<typename _Type>
concept is_empty_c = std::is_empty_v<_Type>;

// is_polymorphic_c
//   concept: satisfied if T is polymorphic (has virtual functions)
template<typename _Type>
concept is_polymorphic_c = std::is_polymorphic_v<_Type>;

// is_abstract_c
//   concept: satisfied if T is abstract
template<typename _Type>
concept is_abstract_c = std::is_abstract_v<_Type>;

// is_final_c
//   concept: satisfied if T is final
template<typename _Type>
concept is_final_c = std::is_final_v<_Type>;

// is_aggregate_c
//   concept: satisfied if T is an aggregate
template<typename _Type>
concept is_aggregate_c = std::is_aggregate_v<_Type>;


// =============================================================================
// V.   TUPLE CONCEPTS
// =============================================================================
// Concepts for working with std::tuple and tuple-like types.

NS_INTERNAL
    // is_tuple_impl
    template<typename _Type>
    struct is_tuple_impl : std::false_type
    {};

    template<typename... _Types>
    struct is_tuple_impl<std::tuple<_Types...>> : std::true_type
    {};

    // is_tuple_homogeneous_impl
    template<typename _Tuple>
    struct is_tuple_homogeneous_impl : std::false_type
    {};

    template<typename _Type>
    struct is_tuple_homogeneous_impl<std::tuple<_Type>> : std::true_type
    {};

    template<typename _Type, 
             typename _Type2, 
             typename... _Types>
    struct is_tuple_homogeneous_impl<std::tuple<_Type, _Type2, _Types...>>
        : std::bool_constant<
            std::is_same_v<_Type, _Type2> &&
            is_tuple_homogeneous_impl<std::tuple<_Type2, _Types...>>::value>
    {};
NS_END  // internal

// is_tuple_c
//   concept: satisfied if T is a std::tuple specialization
template<typename _Type>
concept is_tuple_c = internal::is_tuple_impl<std::remove_cv_t<_Type>>::value;

// tuple_like
//   concept: satisfied if T is tuple-like (has std::tuple_size and std::get)
template<typename _Type>
concept tuple_like = requires 
{
    typename std::tuple_size<std::remove_cvref_t<_Type>>::type;

    requires std::derived_from<
        std::tuple_size<std::remove_cvref_t<_Type>>,
        std::integral_constant<std::size_t, std::tuple_size_v<std::remove_cvref_t<_Type>>>
    >;
};

// homogeneous_tuple
//   concept: satisfied if T is a tuple where all elements have the same type
template<typename _Type>
concept homogeneous_tuple =
    ( is_tuple_c<_Type> &&
      internal::is_tuple_homogeneous_impl<std::remove_cv_t<_Type>>::value );

// empty_tuple
//   concept: satisfied if T is an empty tuple
template<typename _Type>
concept empty_tuple =
    ( is_tuple_c<_Type> &&
      (std::tuple_size_v<std::remove_cv_t<_Type>> == 0) );

// nonempty_tuple
//   concept: satisfied if T is a non-empty tuple
template<typename _Type>
concept nonempty_tuple =
    ( is_tuple_c<_Type> &&
      (std::tuple_size_v<std::remove_cv_t<_Type>> > 0) );

// single_element_tuple
//   concept: satisfied if T is a tuple with exactly one element
template<typename _Type>
concept single_element_tuple =
    ( is_tuple_c<_Type> &&
      (std::tuple_size_v<std::remove_cv_t<_Type>> == 1) );


// =============================================================================
// VI.  CLASS DEFINITION RULE CONCEPTS
// =============================================================================
// Concepts for detecting Rule of Zero/Three/Five compliance.

// follows_rule_of_zero_c
//   concept: satisfied if T follows the Rule of Zero (all special members trivial)
template<typename _Type>
concept follows_rule_of_zero_c =
    ( std::is_trivially_copy_constructible_v<_Type> &&
      std::is_trivially_move_constructible_v<_Type> &&
      std::is_trivially_copy_assignable_v<_Type>    &&
      std::is_trivially_move_assignable_v<_Type>    &&
      std::is_trivially_destructible_v<_Type> );

// follows_rule_of_three_c
//   concept: satisfied if T follows the Rule of Three
//   (copy constructor, copy assignment, destructor all defined)
template<typename _Type>
concept follows_rule_of_three_c =
    ( std::is_copy_constructible_v<_Type> &&
      std::is_copy_assignable_v<_Type>    &&
      std::is_destructible_v<_Type> );

// follows_rule_of_five_c
//   concept: satisfied if T follows the Rule of Five
//   (copy/move constructors, copy/move assignment, destructor all defined)
template<typename _Type>
concept follows_rule_of_five_c =
    ( std::is_copy_constructible_v<_Type> &&
      std::is_move_constructible_v<_Type> &&
      std::is_copy_assignable_v<_Type>    &&
      std::is_move_assignable_v<_Type>    &&
      std::is_destructible_v<_Type> );


// =============================================================================
// VII. CONTAINER AND ALLOCATOR CONCEPTS
// =============================================================================

// has_value_type
//   concept: satisfied if T has a value_type member type
template<typename _Type>
concept has_value_type = requires 
{
    typename _Type::value_type;
};

// has_size_type
//   concept: satisfied if T has a size_type member type
template<typename _Type>
concept has_size_type = requires 
{
    typename _Type::size_type;
};

// has_iterator
//   concept: satisfied if T has an iterator member type
template<typename _Type>
concept has_iterator = requires 
{
    typename _Type::iterator;
};

// has_const_iterator
//   concept: satisfied if T has a const_iterator member type
template<typename _Type>
concept has_const_iterator = requires 
{
    typename _Type::const_iterator;
};

// sizeable
//   concept: satisfied if T has size_type and size() returning convertible to size_t
template<typename _Type>
concept sizeable = requires(const _Type& t) 
{
    typename _Type::size_type;
    { t.size() } -> std::convertible_to<std::size_t>;
    requires std::convertible_to<typename _Type::size_type, std::size_t>;
};

// has_max_size_c
//   concept: satisfied if T has size_type and a max_size static member
template<typename _Type>
concept has_max_size_c = requires 
{
    typename _Type::size_type;
    { _Type::max_size } -> std::convertible_to<typename _Type::size_type>;
};

// allocator_c
//   concept: satisfied if T is an allocator (has allocate/deallocate)
template<typename _Type>
concept allocator_c = requires(_Type alloc, std::size_t n) 
{
    typename std::allocator_traits<_Type>::value_type;
    { std::allocator_traits<_Type>::allocate(alloc, n) };
    { std::allocator_traits<_Type>::deallocate(
        alloc,
        std::declval<typename std::allocator_traits<_Type>::pointer>(),
        n) };
};


// =============================================================================
// VIII. TEMPLATE CONCEPTS
// =============================================================================

// has_nested_template_type
//   concept: satisfied if T has a nested template called 'type'
template<typename _Type>
concept has_nested_template_type_c = requires 
{
    typename _Type::template type<int>;
};

// has_variadic_constructor_c
//   concept: satisfied if T can be constructed from T (copy/move)
template<typename _Type>
concept has_variadic_constructor_c = requires 
{
    _Type(std::declval<_Type>());
};

// template_parameter_base_of
//   concept: satisfied if T::value_type is a base of T
template<typename _Type>
concept template_parameter_base_of = requires 
{
    typename _Type::value_type;

    requires std::is_base_of_v<typename _Type::value_type, _Type>;
};


// =============================================================================
// IX.  LOGICAL CONCEPTS
// =============================================================================
// Concepts for logical operations on type predicates.

// all_of
//   concept: satisfied if all type predicates are true
template<typename... _Bs>
concept all_of = (... && _Bs::value);

// any_of
//   concept: satisfied if any type predicate is true
template<typename... _Bs>
concept any_of = (... || _Bs::value);

// none_of
//   concept: satisfied if no type predicate is true
template<typename... _Bs>
concept none_of = !(... || _Bs::value);


// =============================================================================
// X.   INVOCABLE CONCEPTS
// =============================================================================
// Extended invocable concepts beyond std::invocable.

// invocable_r
//   concept: satisfied if F is invocable with Args and returns convertible to R
template<typename    _Ret, 
         typename    _Fn, 
         typename... _Args>
concept invocable_r = std::invocable<_Fn, _Args...> &&
    ( std::is_void_v<_Ret> || 
      std::convertible_to<std::invoke_result_t<_Fn, _Args...>, _Ret> );

// nothrow_invocable
//   concept: satisfied if F is nothrow invocable with Args
template<typename    _Fn, 
         typename... _Args>
concept nothrow_invocable = 
    ( std::invocable<_Fn, _Args...> &&
      std::is_nothrow_invocable_v<_Fn, _Args...> );

// nothrow_invocable_r
//   concept: satisfied if F is nothrow invocable with Args, returning R
template<typename    _Ret, 
         typename    _Fn, 
         typename... _Args>
concept nothrow_invocable_r = 
    ( invocable_r<_Ret, _Fn, _Args...> &&
      std::is_nothrow_invocable_r_v<_Ret, _Fn, _Args...> );


// =============================================================================
// XI.  SIZE AND NUMERIC CONCEPTS
// =============================================================================

// valid_size_type
//   concept: satisfied if T is valid as a size type (unsigned arithmetic)
template<typename _Type>
concept valid_size_type = 
    ( std::is_unsigned_v<_Type> && 
      std::is_arithmetic_v<_Type> );

// nonzero_size
//   concept: satisfied if N is nonzero
template<std::size_t _N>
concept nonzero_size = (_N != 0);

// zero_size
//   concept: satisfied if N is zero
template<std::size_t _N>
concept zero_size = (_N == 0);


// =============================================================================
// XII. PARAMETER PACK CONCEPTS
// =============================================================================

// single_type
//   concept: satisfied if exactly one type is provided
template<typename... _Types>
concept single_type = (sizeof...(_Types) == 1);

// empty_pack
//   concept: satisfied if no types are provided
template<typename... _Types>
concept empty_pack = (sizeof...(_Types) == 0);

// nonempty_pack
//   concept: satisfied if at least one type is provided
template<typename... _Types>
concept nonempty_pack = (sizeof...(_Types) > 0);

// all_same
//   concept: satisfied if all types in pack are the same
template<typename _First, typename... _Rest>
concept all_same = (std::same_as<_First, _Rest> && ...);

// all_convertible_to
//   concept: satisfied if all types are convertible to Target
template<typename _Target, typename... _Types>
concept all_convertible_to = (std::convertible_to<_Types, _Target> && ...);

// all_derived_from
//   concept: satisfied if all types are derived from Base
template<typename _Base, typename... _Types>
concept all_derived_from = (std::derived_from<_Types, _Base> && ...);


NS_END  // djinterp


#endif  // DJINTERP_CONCEPTS_
