/******************************************************************************
* djinterp [core]                                              cpp_named23.h
*
* C++23 named requirement type traits:
*   Extends the C++20 named requirement traits with C++23 refinements:
*   - Expression-based concept definitions using `requires` expressions
*     for direct semantic validation (not just type trait wrappers)
*   - Nothrow compound concepts
*   - Static assertion diagnostic helpers
*
* ADDITIONS OVER C++20:
*   - concepts::explicit_default_constructible  (requires-expression based)
*   - concepts::explicit_copy_constructible     (requires-expression based)
*   - concepts::explicit_move_constructible     (requires-expression based)
*   - concepts::explicit_copy_assignable        (requires-expression based)
*   - concepts::explicit_move_assignable        (requires-expression based)
*   - concepts::explicit_destructible           (requires-expression based)
*   - concepts::nothrow_fully_copyable          (compound nothrow)
*   - concepts::nothrow_fully_movable           (compound nothrow)
*   - concepts::nothrow_value_type              (compound nothrow)
*   - D_STATIC_ASSERT_VALUE_TYPE(_T) diagnostic macro
*   - ImplicitLifetimeType        (is_implicit_lifetime)
*   - concepts::implicit_lifetime_type
*   - D_STATIC_ASSERT_TRIVIALLY_COPYABLE, D_STATIC_ASSERT_STANDARD_LAYOUT,
*     D_STATIC_ASSERT_POD diagnostic macros
*   - LayoutMapping              (is_layout_mapping)        [gated __cpp_lib_mdspan]
*   - LayoutMappingPolicy        (is_layout_mapping_policy) [gated __cpp_lib_mdspan]
*   - AccessorPolicy             (is_accessor_policy)       [gated __cpp_lib_mdspan]
*   - Range expression-based concepts                       [gated __cpp_lib_ranges]
*   - Concurrency expression-based concepts
*
* NOTE ON EXPLICIT CONCEPTS:
*   The `explicit_*` concepts use `requires` expressions to directly test
*   the syntactic operations, rather than delegating to std::is_*_v traits.
*   This can catch subtle cases where trait intrinsics disagree with actual
*   expression validity and provides clearer subsumption ordering.
*
* FEATURE DEPENDENCIES:
*   D_ENV_CPP_FEATURE_LANG_CONCEPTS             - concept availability
*   D_ENV_CPP_FEATURE_LANG_IF_CONSTEVAL         - if consteval
*
* path:      \inc\cpp_named23.h
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.27
******************************************************************************/

#ifndef DJINTERP_CPP_NAMED23_
#define DJINTERP_CPP_NAMED23_ 1

// require env.h to be included first
#ifndef DJINTERP_ENVIRONMENT_
    #error "cpp_named23.h requires env.h to be included first"
#endif

// only meaningful in C++ mode
#ifndef __cplusplus
    #error "cpp_named23.h can only be used in C++ compilation mode"
#endif

// include C++20 layer
#include "cpp_named20.h"

// C++23 additions are active for C++23 and all later standards
#if D_ENV_LANG_IS_CPP23_OR_HIGHER

#include <type_traits>
#include <concepts>
#include <iterator>
#include <random>
#include <ios>
#include <chrono>


NS_DJINTERP
NS_TRAITS

// =========================================================================
// I.   EXPRESSION-BASED CONCEPTS (C++23)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

D_NAMESPACE(concepts)

    // explicit_default_constructible
    //   concept: constrains _T to types where `_T()` and `_T{}`
    // are well-formed expressions (direct expression validation).
    template<typename _T>
    concept explicit_default_constructible = requires
    {
        _T();
        _T{};
    };

    // explicit_destructible
    //   concept: constrains _T to types where the destructor can be
    // explicitly invoked on a valid object.
    template<typename _T>
    concept explicit_destructible = requires(_T _obj)
    {
        _obj.~_T();
    };

    // explicit_copy_constructible
    //   concept: constrains _T to types where copy construction from
    // a const lvalue reference is a well-formed expression.
    template<typename _T>
    concept explicit_copy_constructible = requires(const _T _src)
    {
        _T(_src);
    };

    // explicit_move_constructible
    //   concept: constrains _T to types where move construction from
    // an rvalue reference is a well-formed expression.
    template<typename _T>
    concept explicit_move_constructible = requires
    {
        _T(std::declval<_T&&>());
    };

    // explicit_copy_assignable
    //   concept: constrains _T to types where copy assignment from a
    // const lvalue reference is a well-formed expression.
    template<typename _T>
    concept explicit_copy_assignable = requires(_T       _dst,
                                                const _T _src)
    {
        { _dst = _src } -> std::same_as<_T&>;
    };

    // explicit_move_assignable
    //   concept: constrains _T to types where move assignment from an
    // rvalue reference is a well-formed expression.
    template<typename _T>
    concept explicit_move_assignable = requires(_T _dst)
    {
        { _dst = std::declval<_T&&>() } -> std::same_as<_T&>;
    };


    // =====================================================================
    // II.  NOTHROW COMPOUND CONCEPTS (C++23)
    // =====================================================================

    // nothrow_fully_copyable
    //   concept: constrains _T to types satisfying both
    // NothrowCopyConstructible and NothrowCopyAssignable.
    template<typename _T>
    concept nothrow_fully_copyable =
        ( nothrow_copy_constructible<_T> &&
          nothrow_copy_assignable<_T> );

    // nothrow_fully_movable
    //   concept: constrains _T to types satisfying both
    // NothrowMoveConstructible and NothrowMoveAssignable.
    template<typename _T>
    concept nothrow_fully_movable =
        ( nothrow_move_constructible<_T> &&
          nothrow_move_assignable<_T> );

    // nothrow_value_type
    //   concept: constrains _T to types satisfying all six core named
    // requirements with nothrow guarantees.
    template<typename _T>
    concept nothrow_value_type =
        ( nothrow_default_constructible<_T> &&
          nothrow_copy_constructible<_T>    &&
          nothrow_move_constructible<_T>    &&
          nothrow_copy_assignable<_T>       &&
          nothrow_move_assignable<_T>       &&
          nothrow_destructible<_T> );


    // =====================================================================
    // III. EXPRESSION-BASED COMPARISON CONCEPTS (C++23)
    // =====================================================================

    // explicit_equality_comparable
    //   concept: constrains _T using requires-expressions to directly
    // test == and != with bool-convertible results.
    template<typename _T>
    concept explicit_equality_comparable = requires(const _T _a,
                                                    const _T _b)
    {
        { _a == _b } -> std::convertible_to<bool>;
        { _a != _b } -> std::convertible_to<bool>;
        { _b == _a } -> std::convertible_to<bool>;
        { _b != _a } -> std::convertible_to<bool>;
    };

    // explicit_less_than_comparable
    //   concept: constrains _T using requires-expressions to directly
    // test all four relational operators.
    template<typename _T>
    concept explicit_less_than_comparable = requires(const _T _a,
                                                     const _T _b)
    {
        { _a <  _b } -> std::convertible_to<bool>;
        { _a >  _b } -> std::convertible_to<bool>;
        { _a <= _b } -> std::convertible_to<bool>;
        { _a >= _b } -> std::convertible_to<bool>;
    };

    // nothrow_swappable
    //   concept: constrains _T to types that are swappable without
    // throwing exceptions.
    template<typename _T>
    concept nothrow_swappable =
        ( swappable<_T> &&
          std::is_nothrow_swappable_v<_T> );

    // strict_weak_order
    //   concept: constrains _F to types that structurally model a
    // strict weak ordering over _T. Uses std::strict_weak_order.
    template<typename _F,
             typename _T>
    concept strict_weak_order =
        std::strict_weak_order<_F, _T, _T>;

NS_END  // concepts

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// =========================================================================
// III. STATIC ASSERTION DIAGNOSTIC HELPERS (C++23)
// =========================================================================

// D_STATIC_ASSERT_DEFAULT_CONSTRUCTIBLE
//   macro: static_assert that _T is default-constructible with a
// descriptive error message.
#define D_STATIC_ASSERT_DEFAULT_CONSTRUCTIBLE(_T)                          \
    static_assert(                                                         \
        std::is_default_constructible_v<_T>,                               \
        "Type " #_T " does not satisfy DefaultConstructible")

// D_STATIC_ASSERT_COPY_CONSTRUCTIBLE
//   macro: static_assert that _T is copy-constructible with a
// descriptive error message.
#define D_STATIC_ASSERT_COPY_CONSTRUCTIBLE(_T)                             \
    static_assert(                                                         \
        std::is_copy_constructible_v<_T>,                                  \
        "Type " #_T " does not satisfy CopyConstructible")

// D_STATIC_ASSERT_MOVE_CONSTRUCTIBLE
//   macro: static_assert that _T is move-constructible with a
// descriptive error message.
#define D_STATIC_ASSERT_MOVE_CONSTRUCTIBLE(_T)                             \
    static_assert(                                                         \
        std::is_move_constructible_v<_T>,                                  \
        "Type " #_T " does not satisfy MoveConstructible")

// D_STATIC_ASSERT_COPY_ASSIGNABLE
//   macro: static_assert that _T is copy-assignable with a
// descriptive error message.
#define D_STATIC_ASSERT_COPY_ASSIGNABLE(_T)                                \
    static_assert(                                                         \
        std::is_copy_assignable_v<_T>,                                     \
        "Type " #_T " does not satisfy CopyAssignable")

// D_STATIC_ASSERT_MOVE_ASSIGNABLE
//   macro: static_assert that _T is move-assignable with a
// descriptive error message.
#define D_STATIC_ASSERT_MOVE_ASSIGNABLE(_T)                                \
    static_assert(                                                         \
        std::is_move_assignable_v<_T>,                                     \
        "Type " #_T " does not satisfy MoveAssignable")

// D_STATIC_ASSERT_DESTRUCTIBLE
//   macro: static_assert that _T is destructible with a descriptive
// error message.
#define D_STATIC_ASSERT_DESTRUCTIBLE(_T)                                   \
    static_assert(                                                         \
        std::is_destructible_v<_T>,                                        \
        "Type " #_T " does not satisfy Destructible")

// D_STATIC_ASSERT_VALUE_TYPE
//   macro: static_assert that _T satisfies all six core named
// requirements. Produces individual error messages for each failed
// requirement.
#define D_STATIC_ASSERT_VALUE_TYPE(_T)                                     \
    D_STATIC_ASSERT_DEFAULT_CONSTRUCTIBLE(_T);                             \
    D_STATIC_ASSERT_COPY_CONSTRUCTIBLE(_T);                                \
    D_STATIC_ASSERT_MOVE_CONSTRUCTIBLE(_T);                                \
    D_STATIC_ASSERT_COPY_ASSIGNABLE(_T);                                   \
    D_STATIC_ASSERT_MOVE_ASSIGNABLE(_T);                                   \
    D_STATIC_ASSERT_DESTRUCTIBLE(_T)


// =========================================================================
// IV.  IMPLICIT LIFETIME TYPE TRAIT (C++23)
// =========================================================================

// is_implicit_lifetime
//   trait: evaluates to true if _T is an implicit-lifetime type (a type
// whose lifetime can begin without explicit constructor invocation).
// Implicit-lifetime types include: scalar types, implicit-lifetime
// class types, arrays of such types, and cv-qualified versions thereof.
#if defined(__cpp_lib_is_implicit_lifetime)
template<typename _T>
struct is_implicit_lifetime
{
    static constexpr bool value =
        std::is_implicit_lifetime_v<_T>;
};
#else
// fallback: conservative approximation when std::is_implicit_lifetime
// is not available. This approximation covers scalar types, trivial
// types, and aggregates of such types but may produce false negatives
// for some implicit-lifetime class types.
template<typename _T>
struct is_implicit_lifetime
{
    static constexpr bool value =
        ( std::is_scalar_v<_T>            ||
          std::is_array_v<_T>             ||
          std::is_trivially_copyable_v<_T> );
};
#endif

// is_implicit_lifetime_v
//   value: shorthand for is_implicit_lifetime<_T>::value.
template<typename _T>
inline constexpr bool is_implicit_lifetime_v =
    is_implicit_lifetime<_T>::value;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

D_NAMESPACE(concepts)

    // implicit_lifetime_type
    //   concept: constrains _T to implicit-lifetime types whose
    // lifetime can begin without explicit constructor invocation.
    template<typename _T>
    concept implicit_lifetime_type =
        is_implicit_lifetime_v<_T>;

NS_END  // concepts

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// =========================================================================
// V.   TYPE CLASSIFICATION STATIC ASSERTION MACROS (C++23)
// =========================================================================

// D_STATIC_ASSERT_TRIVIALLY_COPYABLE
//   macro: static_assert that _T is trivially copyable with a
// descriptive error message.
#define D_STATIC_ASSERT_TRIVIALLY_COPYABLE(_T)                             \
    static_assert(                                                         \
        std::is_trivially_copyable_v<_T>,                                  \
        "Type " #_T " does not satisfy TriviallyCopyable")

// D_STATIC_ASSERT_STANDARD_LAYOUT
//   macro: static_assert that _T is standard-layout with a descriptive
// error message.
#define D_STATIC_ASSERT_STANDARD_LAYOUT(_T)                                \
    static_assert(                                                         \
        std::is_standard_layout_v<_T>,                                     \
        "Type " #_T " does not satisfy StandardLayoutType")

// D_STATIC_ASSERT_POD
//   macro: static_assert that _T is a POD type with a descriptive
// error message.
// note: PODType is deprecated in C++20.
#define D_STATIC_ASSERT_POD(_T)                                            \
    D_STATIC_ASSERT_TRIVIALLY_COPYABLE(_T);                                \
    D_STATIC_ASSERT_STANDARD_LAYOUT(_T);                                   \
    static_assert(                                                         \
        std::is_trivial_v<_T>,                                             \
        "Type " #_T " does not satisfy TrivialType (required for POD)")


// =========================================================================
// VI.  CONTAINER EXPRESSION-BASED CONCEPTS (C++23)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

D_NAMESPACE(concepts)

    // explicit_container
    //   concept: constrains _C using requires-expressions to directly
    // test Container operations and nested types.
    template<typename _C>
    concept explicit_container = requires(const _C _c, _C _m)
    {
        typename _C::value_type;
        typename _C::reference;
        typename _C::const_reference;
        typename _C::iterator;
        typename _C::const_iterator;
        typename _C::difference_type;
        typename _C::size_type;
        { _c.begin()    } -> std::input_or_output_iterator;
        { _c.end()      } -> std::sentinel_for<decltype(_c.begin())>;
        { _c.cbegin()   } -> std::input_or_output_iterator;
        { _c.cend()     } -> std::sentinel_for<decltype(_c.cbegin())>;
        { _c.size()     } -> std::convertible_to<typename _C::size_type>;
        { _c.max_size() } -> std::convertible_to<typename _C::size_type>;
        { _c.empty()    } -> std::convertible_to<bool>;
    };

    // explicit_reversible_container
    //   concept: constrains _C to types satisfying the
    // ReversibleContainer requirements via requires-expression.
    template<typename _C>
    concept explicit_reversible_container =
        ( explicit_container<_C> &&
          requires(const _C _c)
          {
              typename _C::reverse_iterator;
              typename _C::const_reverse_iterator;
              { _c.rbegin()  } -> std::input_or_output_iterator;
              { _c.rend()    } -> std::sentinel_for<decltype(_c.rbegin())>;
              { _c.crbegin() } -> std::input_or_output_iterator;
              { _c.crend()   } -> std::sentinel_for<decltype(_c.crbegin())>;
          } );

    // explicit_allocator_aware_container
    //   concept: constrains _C to types satisfying the
    // AllocatorAwareContainer requirements via requires-expression.
    template<typename _C>
    concept explicit_allocator_aware_container =
        ( explicit_container<_C> &&
          requires(const _C _c)
          {
              typename _C::allocator_type;
              { _c.get_allocator() }
                  -> std::same_as<typename _C::allocator_type>;
          } );

    // explicit_contiguous_container
    //   concept: constrains _C to types satisfying the
    // ContiguousContainer requirements via requires-expression.
    template<typename _C>
    concept explicit_contiguous_container =
        ( explicit_container<_C> &&
          requires(const _C _c)
          {
              { _c.data() }
                  -> std::same_as<const typename _C::value_type*>;
          } );

    // explicit_associative_container
    //   concept: constrains _C to types satisfying the
    // AssociativeContainer requirements via requires-expression.
    template<typename _C>
    concept explicit_associative_container =
        ( explicit_container<_C> &&
          requires(const _C _c,
                   const typename _C::key_type& _k)
          {
              typename _C::key_type;
              typename _C::key_compare;
              typename _C::value_compare;
              _c.find(_k);
              { _c.count(_k) }
                  -> std::convertible_to<typename _C::size_type>;
              _c.lower_bound(_k);
              _c.upper_bound(_k);
              _c.equal_range(_k);
          } );

    // explicit_unordered_associative_container
    //   concept: constrains _C to types satisfying the
    // UnorderedAssociativeContainer requirements via
    // requires-expression.
    template<typename _C>
    concept explicit_unordered_associative_container =
        ( explicit_container<_C> &&
          requires(const _C _c)
          {
              typename _C::hasher;
              typename _C::key_equal;
              _c.hash_function();
              _c.key_eq();
              { _c.bucket_count() }
                  -> std::convertible_to<typename _C::size_type>;
              _c.load_factor();
              _c.max_load_factor();
          } );

NS_END  // concepts

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// =========================================================================
// VII. CONTAINER STATIC ASSERTION MACROS (C++23)
// =========================================================================

// D_STATIC_ASSERT_CONTAINER
//   macro: static_assert that _C satisfies the Container named
// requirement.
#define D_STATIC_ASSERT_CONTAINER(_C)                                      \
    static_assert(                                                         \
        djinterp::traits::is_container_v<_C>,                              \
        "Type " #_C " does not satisfy Container")

// D_STATIC_ASSERT_REVERSIBLE_CONTAINER
//   macro: static_assert that _C satisfies the ReversibleContainer
// named requirement.
#define D_STATIC_ASSERT_REVERSIBLE_CONTAINER(_C)                           \
    D_STATIC_ASSERT_CONTAINER(_C);                                         \
    static_assert(                                                         \
        djinterp::traits::is_reversible_container_v<_C>,                   \
        "Type " #_C " does not satisfy ReversibleContainer")

// D_STATIC_ASSERT_SEQUENCE_CONTAINER
//   macro: static_assert that _C satisfies the SequenceContainer
// named requirement.
#define D_STATIC_ASSERT_SEQUENCE_CONTAINER(_C)                             \
    D_STATIC_ASSERT_CONTAINER(_C);                                         \
    static_assert(                                                         \
        djinterp::traits::is_sequence_container_v<_C>,                     \
        "Type " #_C " does not satisfy SequenceContainer")

// D_STATIC_ASSERT_ASSOCIATIVE_CONTAINER
//   macro: static_assert that _C satisfies the AssociativeContainer
// named requirement.
#define D_STATIC_ASSERT_ASSOCIATIVE_CONTAINER(_C)                          \
    D_STATIC_ASSERT_CONTAINER(_C);                                         \
    static_assert(                                                         \
        djinterp::traits::is_associative_container_v<_C>,                  \
        "Type " #_C " does not satisfy AssociativeContainer")

// D_STATIC_ASSERT_UNORDERED_ASSOCIATIVE_CONTAINER
//   macro: static_assert that _C satisfies the
// UnorderedAssociativeContainer named requirement.
#define D_STATIC_ASSERT_UNORDERED_ASSOCIATIVE_CONTAINER(_C)                \
    D_STATIC_ASSERT_CONTAINER(_C);                                         \
    static_assert(                                                         \
        djinterp::traits::is_unordered_associative_container_v<_C>,        \
        "Type " #_C " does not satisfy UnorderedAssociativeContainer")


// =========================================================================
// VIII. CONTAINER ELEMENT EXPRESSION-BASED CONCEPTS (C++23)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

D_NAMESPACE(concepts)

    // explicit_default_insertable
    //   concept: constrains _T to types that are DefaultInsertable
    // using allocator _A, verified via requires-expression on
    // allocator_traits::construct.
    template<typename _T,
             typename _A = std::allocator<_T>>
    concept explicit_default_insertable = requires(_A _a, _T* _p)
    {
        std::allocator_traits<_A>::construct(_a, _p);
    };

    // explicit_copy_insertable
    //   concept: constrains _T to types that are CopyInsertable
    // using allocator _A, verified via requires-expression.
    template<typename _T,
             typename _A = std::allocator<_T>>
    concept explicit_copy_insertable = requires(_A       _a,
                                                _T*      _p,
                                                const _T& _v)
    {
        std::allocator_traits<_A>::construct(_a, _p, _v);
    };

    // explicit_move_insertable
    //   concept: constrains _T to types that are MoveInsertable
    // using allocator _A, verified via requires-expression.
    template<typename _T,
             typename _A = std::allocator<_T>>
    concept explicit_move_insertable = requires(_A  _a,
                                                _T* _p,
                                                _T  _rv)
    {
        std::allocator_traits<_A>::construct(
            _a, _p, static_cast<_T&&>(_rv));
    };

    // explicit_erasable
    //   concept: constrains _T to types that are Erasable using
    // allocator _A, verified via requires-expression on
    // allocator_traits::destroy.
    template<typename _T,
             typename _A = std::allocator<_T>>
    concept explicit_erasable = requires(_A _a, _T* _p)
    {
        std::allocator_traits<_A>::destroy(_a, _p);
    };

NS_END  // concepts

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// =========================================================================
// IX.  CONTAINER ELEMENT STATIC ASSERTION MACROS (C++23)
// =========================================================================

// D_STATIC_ASSERT_DEFAULT_INSERTABLE
//   macro: static_assert that _T is DefaultInsertable with
// allocator _A (defaults to std::allocator<_T>).
#define D_STATIC_ASSERT_DEFAULT_INSERTABLE(_T, ...)                        \
    static_assert(                                                         \
        djinterp::traits::is_default_insertable_v<_T __VA_OPT__(,)        \
            __VA_ARGS__>,                                                  \
        "Type " #_T " does not satisfy DefaultInsertable")

// D_STATIC_ASSERT_COPY_INSERTABLE
//   macro: static_assert that _T is CopyInsertable with
// allocator _A (defaults to std::allocator<_T>).
#define D_STATIC_ASSERT_COPY_INSERTABLE(_T, ...)                           \
    static_assert(                                                         \
        djinterp::traits::is_copy_insertable_v<_T __VA_OPT__(,)           \
            __VA_ARGS__>,                                                  \
        "Type " #_T " does not satisfy CopyInsertable")

// D_STATIC_ASSERT_MOVE_INSERTABLE
//   macro: static_assert that _T is MoveInsertable with
// allocator _A (defaults to std::allocator<_T>).
#define D_STATIC_ASSERT_MOVE_INSERTABLE(_T, ...)                           \
    static_assert(                                                         \
        djinterp::traits::is_move_insertable_v<_T __VA_OPT__(,)           \
            __VA_ARGS__>,                                                  \
        "Type " #_T " does not satisfy MoveInsertable")

// D_STATIC_ASSERT_ERASABLE
//   macro: static_assert that _T is Erasable with allocator _A
// (defaults to std::allocator<_T>).
#define D_STATIC_ASSERT_ERASABLE(_T, ...)                                  \
    static_assert(                                                         \
        djinterp::traits::is_erasable_v<_T __VA_OPT__(,)                  \
            __VA_ARGS__>,                                                  \
        "Type " #_T " does not satisfy Erasable")


// =========================================================================
// X.   ITERATOR CONCEPTS (C++23)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

D_NAMESPACE(concepts)

    // constexpr_iterator
    //   concept: constrains _I to types satisfying the
    // ConstexprIterator named requirement: a LegacyIterator whose
    // operations can be evaluated in a constant expression.
    // note: detection checks that _I is a literal type (trivially
    // destructible or constexpr destructible) and satisfies
    // std::semiregular (constexpr copy/move/default-constructible).
    // Full constexpr-evaluability of all operations cannot be verified
    // at compile time without attempting actual constant evaluation.
    template<typename _I>
    concept constexpr_iterator =
        ( legacy_iterator<_I>          &&
          std::semiregular<_I>         &&
          std::is_trivially_destructible_v<_I> );

    // explicit_legacy_input_iterator
    //   concept: constrains _I using requires-expressions to directly
    // test LegacyInputIterator operations.
    template<typename _I>
    concept explicit_legacy_input_iterator = requires(_I       _it,
                                                      const _I _cit)
    {
        { *_it }     -> std::common_reference_with<
                            typename std::iterator_traits<_I>::reference>;
        { ++_it }    -> std::same_as<_I&>;
        { _cit == _cit } -> std::convertible_to<bool>;
        { _cit != _cit } -> std::convertible_to<bool>;
        *_it++;
    };

    // explicit_legacy_forward_iterator
    //   concept: constrains _I using requires-expressions to directly
    // test LegacyForwardIterator operations.
    template<typename _I>
    concept explicit_legacy_forward_iterator =
        ( explicit_legacy_input_iterator<_I> &&
          std::default_initializable<_I>     &&
          requires(_I _it)
          {
              { _it++ } -> std::convertible_to<const _I&>;
              { *_it++ } -> std::common_reference_with<
                                typename std::iterator_traits<_I>::reference>;
          } );

    // explicit_legacy_bidirectional_iterator
    //   concept: constrains _I using requires-expressions to directly
    // test LegacyBidirectionalIterator operations.
    template<typename _I>
    concept explicit_legacy_bidirectional_iterator =
        ( explicit_legacy_forward_iterator<_I> &&
          requires(_I _it)
          {
              { --_it }  -> std::same_as<_I&>;
              { _it-- }  -> std::convertible_to<const _I&>;
              { *_it-- } -> std::common_reference_with<
                                typename std::iterator_traits<_I>::reference>;
          } );

    // explicit_legacy_random_access_iterator
    //   concept: constrains _I using requires-expressions to directly
    // test LegacyRandomAccessIterator operations.
    template<typename _I>
    concept explicit_legacy_random_access_iterator =
        ( explicit_legacy_bidirectional_iterator<_I> &&
          std::totally_ordered<_I>                   &&
          requires(_I       _it,
                   const _I _cit,
                   typename std::iterator_traits<_I>::difference_type _n)
          {
              { _it  += _n }    -> std::same_as<_I&>;
              { _it  -= _n }    -> std::same_as<_I&>;
              { _cit +  _n }    -> std::same_as<_I>;
              { _n   +  _cit }  -> std::same_as<_I>;
              { _cit -  _n }    -> std::same_as<_I>;
              { _cit -  _cit }  -> std::same_as<
                  typename std::iterator_traits<_I>::difference_type>;
              { _cit[_n] }      -> std::common_reference_with<
                  typename std::iterator_traits<_I>::reference>;
          } );

NS_END  // concepts

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// =========================================================================
// XI.  ITERATOR STATIC ASSERTION MACROS (C++23)
// =========================================================================

// D_STATIC_ASSERT_LEGACY_ITERATOR
//   macro: static_assert that _I satisfies the LegacyIterator
// named requirement.
#define D_STATIC_ASSERT_LEGACY_ITERATOR(_I)                                \
    static_assert(                                                         \
        djinterp::traits::is_legacy_iterator_v<_I>,                        \
        "Type " #_I " does not satisfy LegacyIterator")

// D_STATIC_ASSERT_LEGACY_INPUT_ITERATOR
//   macro: static_assert that _I satisfies the
// LegacyInputIterator named requirement.
#define D_STATIC_ASSERT_LEGACY_INPUT_ITERATOR(_I)                          \
    D_STATIC_ASSERT_LEGACY_ITERATOR(_I);                                   \
    static_assert(                                                         \
        djinterp::traits::is_legacy_input_iterator_v<_I>,                  \
        "Type " #_I " does not satisfy LegacyInputIterator")

// D_STATIC_ASSERT_LEGACY_FORWARD_ITERATOR
//   macro: static_assert that _I satisfies the
// LegacyForwardIterator named requirement.
#define D_STATIC_ASSERT_LEGACY_FORWARD_ITERATOR(_I)                        \
    D_STATIC_ASSERT_LEGACY_INPUT_ITERATOR(_I);                             \
    static_assert(                                                         \
        djinterp::traits::is_legacy_forward_iterator_v<_I>,                \
        "Type " #_I " does not satisfy LegacyForwardIterator")

// D_STATIC_ASSERT_LEGACY_BIDIRECTIONAL_ITERATOR
//   macro: static_assert that _I satisfies the
// LegacyBidirectionalIterator named requirement.
#define D_STATIC_ASSERT_LEGACY_BIDIRECTIONAL_ITERATOR(_I)                  \
    D_STATIC_ASSERT_LEGACY_FORWARD_ITERATOR(_I);                           \
    static_assert(                                                         \
        djinterp::traits::is_legacy_bidirectional_iterator_v<_I>,          \
        "Type " #_I " does not satisfy LegacyBidirectionalIterator")

// D_STATIC_ASSERT_LEGACY_RANDOM_ACCESS_ITERATOR
//   macro: static_assert that _I satisfies the
// LegacyRandomAccessIterator named requirement.
#define D_STATIC_ASSERT_LEGACY_RANDOM_ACCESS_ITERATOR(_I)                  \
    D_STATIC_ASSERT_LEGACY_BIDIRECTIONAL_ITERATOR(_I);                     \
    static_assert(                                                         \
        djinterp::traits::is_legacy_random_access_iterator_v<_I>,          \
        "Type " #_I " does not satisfy LegacyRandomAccessIterator")


// =========================================================================
// XII. STREAM I/O AND RANDOM EXPRESSION-BASED CONCEPTS (C++23)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

D_NAMESPACE(concepts)

    // explicit_unformatted_input_stream
    //   concept: constrains _S using requires-expressions to directly
    // test unformatted input operations.
    template<typename _S>
    concept explicit_unformatted_input_stream =
        requires(_S _s,
                 typename _S::char_type* _buf,
                 std::streamsize _n)
    {
        { _s.get()       } -> std::convertible_to<typename _S::int_type>;
        { _s.read(_buf, _n) } -> std::same_as<_S&>;
        { _s.gcount()    } -> std::convertible_to<std::streamsize>;
    };

    // explicit_unformatted_output_stream
    //   concept: constrains _S using requires-expressions to directly
    // test unformatted output operations.
    template<typename _S>
    concept explicit_unformatted_output_stream =
        requires(_S _s,
                 typename _S::char_type _ch,
                 const typename _S::char_type* _buf,
                 std::streamsize _n)
    {
        { _s.put(_ch)         } -> std::same_as<_S&>;
        { _s.write(_buf, _n)  } -> std::same_as<_S&>;
    };

    // explicit_uniform_random_bit_generator
    //   concept: constrains _G using requires-expressions to directly
    // test UniformRandomBitGenerator operations with return type
    // constraints.
    template<typename _G>
    concept explicit_uniform_random_bit_generator =
        ( std::unsigned_integral<typename _G::result_type> &&
          requires(_G _g)
          {
              { _g()     } -> std::same_as<typename _G::result_type>;
              { _G::min() } -> std::same_as<typename _G::result_type>;
              { _G::max() } -> std::same_as<typename _G::result_type>;
          } );

    // explicit_random_number_engine
    //   concept: constrains _E using requires-expressions to directly
    // test RandomNumberEngine operations.
    template<typename _E>
    concept explicit_random_number_engine =
        ( explicit_uniform_random_bit_generator<_E> &&
          requires(_E _e,
                   typename _E::result_type _val,
                   unsigned long long _z)
          {
              _e.seed();
              _e.seed(_val);
              _e.discard(_z);
              { _e == _e } -> std::convertible_to<bool>;
              { _e != _e } -> std::convertible_to<bool>;
          } );

    // explicit_random_number_distribution
    //   concept: constrains _D using requires-expressions to directly
    // test RandomNumberDistribution operations.
    template<typename _D>
    concept explicit_random_number_distribution =
        requires(_D _d, const _D _cd, std::mt19937 _gen)
    {
        typename _D::result_type;
        typename _D::param_type;
        { _d(_gen)   } -> std::same_as<typename _D::result_type>;
        { _cd.param() } -> std::same_as<typename _D::param_type>;
        { _cd.min()  } -> std::same_as<typename _D::result_type>;
        { _cd.max()  } -> std::same_as<typename _D::result_type>;
        _d.reset();
    };

NS_END  // concepts

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// =========================================================================
// XIII. STREAM I/O AND RANDOM STATIC ASSERTION MACROS (C++23)
// =========================================================================

// D_STATIC_ASSERT_UNFORMATTED_INPUT_STREAM
//   macro: static_assert that _S supports unformatted input.
#define D_STATIC_ASSERT_UNFORMATTED_INPUT_STREAM(_S)                       \
    static_assert(                                                         \
        djinterp::traits::is_unformatted_input_stream_v<_S>,               \
        "Type " #_S " does not support UnformattedInputFunction")

// D_STATIC_ASSERT_FORMATTED_INPUT_STREAM
//   macro: static_assert that _S supports formatted input.
#define D_STATIC_ASSERT_FORMATTED_INPUT_STREAM(_S)                         \
    static_assert(                                                         \
        djinterp::traits::is_formatted_input_stream_v<_S>,                 \
        "Type " #_S " does not support FormattedInputFunction")

// D_STATIC_ASSERT_UNFORMATTED_OUTPUT_STREAM
//   macro: static_assert that _S supports unformatted output.
#define D_STATIC_ASSERT_UNFORMATTED_OUTPUT_STREAM(_S)                      \
    static_assert(                                                         \
        djinterp::traits::is_unformatted_output_stream_v<_S>,              \
        "Type " #_S " does not support UnformattedOutputFunction")

// D_STATIC_ASSERT_FORMATTED_OUTPUT_STREAM
//   macro: static_assert that _S supports formatted output.
#define D_STATIC_ASSERT_FORMATTED_OUTPUT_STREAM(_S)                        \
    static_assert(                                                         \
        djinterp::traits::is_formatted_output_stream_v<_S>,                \
        "Type " #_S " does not support FormattedOutputFunction")

// D_STATIC_ASSERT_UNIFORM_RANDOM_BIT_GENERATOR
//   macro: static_assert that _G satisfies
// UniformRandomBitGenerator.
#define D_STATIC_ASSERT_UNIFORM_RANDOM_BIT_GENERATOR(_G)                   \
    static_assert(                                                         \
        djinterp::traits::is_uniform_random_bit_generator_v<_G>,           \
        "Type " #_G " does not satisfy UniformRandomBitGenerator")

// D_STATIC_ASSERT_RANDOM_NUMBER_ENGINE
//   macro: static_assert that _E satisfies RandomNumberEngine.
#define D_STATIC_ASSERT_RANDOM_NUMBER_ENGINE(_E)                           \
    D_STATIC_ASSERT_UNIFORM_RANDOM_BIT_GENERATOR(_E);                     \
    static_assert(                                                         \
        djinterp::traits::is_random_number_engine_v<_E>,                   \
        "Type " #_E " does not satisfy RandomNumberEngine")

// D_STATIC_ASSERT_RANDOM_NUMBER_DISTRIBUTION
//   macro: static_assert that _D satisfies
// RandomNumberDistribution.
#define D_STATIC_ASSERT_RANDOM_NUMBER_DISTRIBUTION(_D)                     \
    static_assert(                                                         \
        djinterp::traits::is_random_number_distribution_v<_D>,             \
        "Type " #_D " does not satisfy RandomNumberDistribution")


// =========================================================================
// XIV. CONCURRENCY EXPRESSION-BASED CONCEPTS (C++23)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

D_NAMESPACE(concepts)

    // explicit_basic_lockable
    //   concept: constrains _M using requires-expressions to directly
    // test BasicLockable operations.
    template<typename _M>
    concept explicit_basic_lockable = requires(_M _m)
    {
        _m.lock();
        _m.unlock();
    };

    // explicit_lockable
    //   concept: constrains _M using requires-expressions to directly
    // test Lockable operations.
    template<typename _M>
    concept explicit_lockable =
        ( explicit_basic_lockable<_M> &&
          requires(_M _m)
          {
              { _m.try_lock() } -> std::convertible_to<bool>;
          } );

    // explicit_timed_lockable
    //   concept: constrains _M using requires-expressions to directly
    // test TimedLockable operations.
    template<typename _M>
    concept explicit_timed_lockable =
        ( explicit_lockable<_M> &&
          requires(_M                                  _m,
                   std::chrono::milliseconds           _dur,
                   std::chrono::steady_clock::time_point _tp)
          {
              { _m.try_lock_for(_dur) }   -> std::convertible_to<bool>;
              { _m.try_lock_until(_tp) }  -> std::convertible_to<bool>;
          } );

    // explicit_shared_lockable
    //   concept: constrains _M using requires-expressions to directly
    // test SharedLockable operations.
    template<typename _M>
    concept explicit_shared_lockable = requires(_M _m)
    {
        _m.lock_shared();
        _m.unlock_shared();
        { _m.try_lock_shared() } -> std::convertible_to<bool>;
    };

    // explicit_shared_timed_lockable
    //   concept: constrains _M using requires-expressions to directly
    // test SharedTimedLockable operations.
    template<typename _M>
    concept explicit_shared_timed_lockable =
        ( explicit_shared_lockable<_M> &&
          requires(_M                                  _m,
                   std::chrono::milliseconds           _dur,
                   std::chrono::steady_clock::time_point _tp)
          {
              { _m.try_lock_shared_for(_dur) }
                  -> std::convertible_to<bool>;
              { _m.try_lock_shared_until(_tp) }
                  -> std::convertible_to<bool>;
          } );

NS_END  // concepts

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// =========================================================================
// XV.  CONCURRENCY STATIC ASSERTION MACROS (C++23)
// =========================================================================

// D_STATIC_ASSERT_BASIC_LOCKABLE
//   macro: static_assert that _M satisfies BasicLockable.
#define D_STATIC_ASSERT_BASIC_LOCKABLE(_M)                                 \
    static_assert(                                                         \
        djinterp::traits::is_basic_lockable_v<_M>,                         \
        "Type " #_M " does not satisfy BasicLockable")

// D_STATIC_ASSERT_LOCKABLE
//   macro: static_assert that _M satisfies Lockable.
#define D_STATIC_ASSERT_LOCKABLE(_M)                                       \
    D_STATIC_ASSERT_BASIC_LOCKABLE(_M);                                    \
    static_assert(                                                         \
        djinterp::traits::is_lockable_v<_M>,                               \
        "Type " #_M " does not satisfy Lockable")

// D_STATIC_ASSERT_TIMED_LOCKABLE
//   macro: static_assert that _M satisfies TimedLockable.
#define D_STATIC_ASSERT_TIMED_LOCKABLE(_M)                                 \
    D_STATIC_ASSERT_LOCKABLE(_M);                                          \
    static_assert(                                                         \
        djinterp::traits::is_timed_lockable_v<_M>,                         \
        "Type " #_M " does not satisfy TimedLockable")

// D_STATIC_ASSERT_MUTEX
//   macro: static_assert that _M satisfies Mutex.
#define D_STATIC_ASSERT_MUTEX(_M)                                          \
    D_STATIC_ASSERT_LOCKABLE(_M);                                          \
    static_assert(                                                         \
        djinterp::traits::is_mutex_v<_M>,                                  \
        "Type " #_M " does not satisfy Mutex")

// D_STATIC_ASSERT_SHARED_MUTEX
//   macro: static_assert that _M satisfies SharedMutex.
#define D_STATIC_ASSERT_SHARED_MUTEX(_M)                                   \
    D_STATIC_ASSERT_MUTEX(_M);                                             \
    static_assert(                                                         \
        djinterp::traits::is_shared_mutex_v<_M>,                           \
        "Type " #_M " does not satisfy SharedMutex")


// =========================================================================
// XVI. RANGE EXPRESSION-BASED CONCEPTS (C++23)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
#if defined(__cpp_lib_ranges)

D_NAMESPACE(concepts)

    // explicit_range
    //   concept: constrains _R using requires-expressions to directly
    // test range operations with return-type constraints.
    template<typename _R>
    concept explicit_range = requires(_R _r)
    {
        { std::ranges::begin(_r) } -> std::input_or_output_iterator;
        { std::ranges::end(_r) }
            -> std::sentinel_for<decltype(std::ranges::begin(_r))>;
    };

    // explicit_range_adaptor_closure_object
    //   concept: constrains _A using requires-expressions to directly
    // test pipe syntax (range | _A) producing a valid range.
    template<typename _A>
    concept explicit_range_adaptor_closure_object =
        requires(_A _a, std::ranges::iota_view<int, int> _r)
    {
        { _r | _a } -> std::ranges::range;
    };

NS_END  // concepts

#endif  // __cpp_lib_ranges
#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// =========================================================================
// XVII. MULTIDIMENSIONAL VIEW CUSTOMIZATION TRAITS (C++23)
// =========================================================================

#if defined(__cpp_lib_mdspan)

#include <mdspan>

NS_INTERNAL

    // has_layout_mapping_ops
    //   trait: detects LayoutMapping members: extents(),
    // required_span_size(), is_always_unique(), is_always_exhaustive(),
    // is_always_strided(), is_unique(), is_exhaustive(), is_strided(),
    // and operator()(indices...).
    template<typename _M,
             typename = void>
    struct has_layout_mapping_ops
    {
        static constexpr bool value = false;
    };

    template<typename _M>
    struct has_layout_mapping_ops<_M,
        decltype(static_cast<void>(
            std::declval<const _M&>().extents(),
            std::declval<const _M&>().required_span_size(),
            static_cast<bool>(_M::is_always_unique()),
            static_cast<bool>(_M::is_always_exhaustive()),
            static_cast<bool>(_M::is_always_strided()),
            static_cast<bool>(std::declval<const _M&>().is_unique()),
            static_cast<bool>(std::declval<const _M&>().is_exhaustive()),
            static_cast<bool>(std::declval<const _M&>().is_strided())
        ))>
    {
        static constexpr bool value = true;
    };

    // has_layout_mapping_policy_type
    //   trait: detects if _L has a nested mapping<_E> template that
    // is itself a layout mapping.
    template<typename _L,
             typename _E,
             typename = void>
    struct has_layout_mapping_policy_type
    {
        static constexpr bool value = false;
    };

    template<typename _L,
             typename _E>
    struct has_layout_mapping_policy_type<_L, _E,
        decltype(static_cast<void>(
            std::declval<typename _L::template mapping<_E>>()
        ))>
    {
        static constexpr bool value =
            has_layout_mapping_ops<
                typename _L::template mapping<_E>>::value;
    };

    // has_accessor_policy_ops
    //   trait: detects AccessorPolicy members: element_type,
    // reference (nested types), data_handle_type, offset(h, i),
    // access(h, i).
    template<typename _A,
             typename = void>
    struct has_accessor_policy_ops
    {
        static constexpr bool value = false;
    };

    template<typename _A>
    struct has_accessor_policy_ops<_A,
        decltype(static_cast<void>(
            std::declval<typename _A::element_type>(),
            std::declval<typename _A::reference>(),
            std::declval<typename _A::data_handle_type>(),
            std::declval<const _A&>().offset(
                std::declval<typename _A::data_handle_type>(),
                std::size_t(0)),
            std::declval<const _A&>().access(
                std::declval<typename _A::data_handle_type>(),
                std::size_t(0))
        ))>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// is_layout_mapping
//   trait: evaluates to true if _M satisfies the structural
// requirements of the LayoutMapping named requirement: has
// extents(), required_span_size(), uniqueness/exhaustiveness/
// stridedness static and member queries.
template<typename _M>
struct is_layout_mapping
{
    static constexpr bool value =
        internal::has_layout_mapping_ops<_M>::value;
};

// is_layout_mapping_policy
//   trait: evaluates to true if _L satisfies the
// LayoutMappingPolicy named requirement: has a nested
// mapping<_E> template that is itself a LayoutMapping.
// default: _E = std::extents<std::size_t, std::dynamic_extent>.
template<typename _L,
         typename _E = std::extents<std::size_t, std::dynamic_extent>>
struct is_layout_mapping_policy
{
    static constexpr bool value =
        internal::has_layout_mapping_policy_type<_L, _E>::value;
};

// is_accessor_policy
//   trait: evaluates to true if _A satisfies the structural
// requirements of the AccessorPolicy named requirement: has
// element_type, reference, data_handle_type nested types, and
// offset(h, i), access(h, i) members.
template<typename _A>
struct is_accessor_policy
{
    static constexpr bool value =
        ( internal::has_accessor_policy_ops<_A>::value &&
          std::is_copy_constructible_v<_A>            &&
          std::is_move_constructible_v<_A>            &&
          std::is_copy_assignable_v<_A>               &&
          std::is_move_assignable_v<_A> );
};

// is_layout_mapping_v (inline constexpr)
//   value: ODR-safe shorthand for is_layout_mapping<_M>::value.
template<typename _M>
inline constexpr bool is_layout_mapping_v =
    is_layout_mapping<_M>::value;

// is_layout_mapping_policy_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_layout_mapping_policy<_L, _E>::value.
template<typename _L,
         typename _E = std::extents<std::size_t, std::dynamic_extent>>
inline constexpr bool is_layout_mapping_policy_v =
    is_layout_mapping_policy<_L, _E>::value;

// is_accessor_policy_v (inline constexpr)
//   value: ODR-safe shorthand for is_accessor_policy<_A>::value.
template<typename _A>
inline constexpr bool is_accessor_policy_v =
    is_accessor_policy<_A>::value;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

D_NAMESPACE(concepts)

    // layout_mapping
    //   concept: constrains _M to types satisfying the
    // LayoutMapping named requirement.
    template<typename _M>
    concept layout_mapping =
        is_layout_mapping_v<_M>;

    // layout_mapping_policy
    //   concept: constrains _L to types satisfying the
    // LayoutMappingPolicy named requirement for extents _E.
    template<typename _L,
             typename _E = std::extents<std::size_t,
                                        std::dynamic_extent>>
    concept layout_mapping_policy =
        is_layout_mapping_policy_v<_L, _E>;

    // accessor_policy
    //   concept: constrains _A to types satisfying the
    // AccessorPolicy named requirement.
    template<typename _A>
    concept accessor_policy =
        is_accessor_policy_v<_A>;

    // explicit_layout_mapping
    //   concept: constrains _M using requires-expressions to directly
    // test LayoutMapping operations with return-type constraints.
    template<typename _M>
    concept explicit_layout_mapping = requires(const _M _m)
    {
        { _m.extents() };
        { _m.required_span_size() }
            -> std::convertible_to<typename decltype(
                   _m.extents())::size_type>;
        { _M::is_always_unique() }     -> std::convertible_to<bool>;
        { _M::is_always_exhaustive() } -> std::convertible_to<bool>;
        { _M::is_always_strided() }    -> std::convertible_to<bool>;
        { _m.is_unique() }             -> std::convertible_to<bool>;
        { _m.is_exhaustive() }         -> std::convertible_to<bool>;
        { _m.is_strided() }            -> std::convertible_to<bool>;
    };

    // explicit_accessor_policy
    //   concept: constrains _A using requires-expressions to directly
    // test AccessorPolicy operations with type constraints.
    template<typename _A>
    concept explicit_accessor_policy = requires(const _A               _a,
                                                typename _A::data_handle_type _h,
                                                std::size_t                   _i)
    {
        typename _A::element_type;
        typename _A::reference;
        typename _A::data_handle_type;
        { _a.offset(_h, _i) }
            -> std::same_as<typename _A::data_handle_type>;
        { _a.access(_h, _i) }
            -> std::same_as<typename _A::reference>;
    };

NS_END  // concepts

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// =========================================================================
// XVIII. MDSPAN STATIC ASSERTION MACROS (C++23)
// =========================================================================

// D_STATIC_ASSERT_LAYOUT_MAPPING
//   macro: static_assert that _M satisfies LayoutMapping.
#define D_STATIC_ASSERT_LAYOUT_MAPPING(_M)                                 \
    static_assert(                                                         \
        djinterp::traits::is_layout_mapping_v<_M>,                         \
        "Type " #_M " does not satisfy LayoutMapping")

// D_STATIC_ASSERT_LAYOUT_MAPPING_POLICY
//   macro: static_assert that _L satisfies LayoutMappingPolicy.
#define D_STATIC_ASSERT_LAYOUT_MAPPING_POLICY(_L)                          \
    static_assert(                                                         \
        djinterp::traits::is_layout_mapping_policy_v<_L>,                  \
        "Type " #_L " does not satisfy LayoutMappingPolicy")

// D_STATIC_ASSERT_ACCESSOR_POLICY
//   macro: static_assert that _A satisfies AccessorPolicy.
#define D_STATIC_ASSERT_ACCESSOR_POLICY(_A)                                \
    static_assert(                                                         \
        djinterp::traits::is_accessor_policy_v<_A>,                        \
        "Type " #_A " does not satisfy AccessorPolicy")

#endif  // __cpp_lib_mdspan


// =========================================================================
// XIX. MISCELLANEOUS EXPRESSION-BASED CONCEPTS (C++23)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

D_NAMESPACE(concepts)

    // explicit_clock_type
    //   concept: constrains _C using requires-expressions to directly
    // test Clock operations with return-type constraints.
    template<typename _C>
    concept explicit_clock_type = requires
    {
        typename _C::rep;
        typename _C::period;
        typename _C::duration;
        typename _C::time_point;
        { _C::is_steady } -> std::convertible_to<bool>;
        { _C::now() }     -> std::same_as<typename _C::time_point>;
    };

    // explicit_char_traits_type
    //   concept: constrains _Tr using requires-expressions to directly
    // test CharTraits operations.
    template<typename _Tr>
    concept explicit_char_traits_type =
        requires(typename _Tr::char_type                _ch,
                 typename _Tr::int_type                 _i,
                 const typename _Tr::char_type*         _p)
    {
        typename _Tr::char_type;
        typename _Tr::int_type;
        typename _Tr::off_type;
        typename _Tr::pos_type;
        typename _Tr::state_type;
        { _Tr::eq(_ch, _ch) }        -> std::convertible_to<bool>;
        { _Tr::lt(_ch, _ch) }        -> std::convertible_to<bool>;
        { _Tr::length(_p) }          -> std::convertible_to<std::size_t>;
        { _Tr::to_char_type(_i) }    -> std::same_as<typename _Tr::char_type>;
        { _Tr::to_int_type(_ch) }    -> std::same_as<typename _Tr::int_type>;
        { _Tr::eof() }               -> std::same_as<typename _Tr::int_type>;
        { _Tr::not_eof(_i) }         -> std::same_as<typename _Tr::int_type>;
    };

    // explicit_bitmask_type
    //   concept: constrains _B using requires-expressions to directly
    // test BitmaskType bitwise operations.
    template<typename _B>
    concept explicit_bitmask_type = requires(_B _a, _B _b)
    {
        { _a & _b }  -> std::convertible_to<_B>;
        { _a | _b }  -> std::convertible_to<_B>;
        { _a ^ _b }  -> std::convertible_to<_B>;
        { ~_a }      -> std::convertible_to<_B>;
        { _a &= _b } -> std::same_as<_B&>;
        { _a |= _b } -> std::same_as<_B&>;
        { _a ^= _b } -> std::same_as<_B&>;
    };

NS_END  // concepts

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// =========================================================================
// XX.  MISCELLANEOUS STATIC ASSERTION MACROS (C++23)
// =========================================================================

// D_STATIC_ASSERT_CLOCK
//   macro: static_assert that _C satisfies Clock.
#define D_STATIC_ASSERT_CLOCK(_C)                                          \
    static_assert(                                                         \
        djinterp::traits::is_clock_v<_C>,                                  \
        "Type " #_C " does not satisfy Clock")

// D_STATIC_ASSERT_TRIVIAL_CLOCK
//   macro: static_assert that _C satisfies TrivialClock.
#define D_STATIC_ASSERT_TRIVIAL_CLOCK(_C)                                  \
    D_STATIC_ASSERT_CLOCK(_C);                                             \
    static_assert(                                                         \
        djinterp::traits::is_trivial_clock_v<_C>,                          \
        "Type " #_C " does not satisfy TrivialClock")

// D_STATIC_ASSERT_CHAR_TRAITS
//   macro: static_assert that _Tr satisfies CharTraits.
#define D_STATIC_ASSERT_CHAR_TRAITS(_Tr)                                   \
    static_assert(                                                         \
        djinterp::traits::is_char_traits_v<_Tr>,                           \
        "Type " #_Tr " does not satisfy CharTraits")

// D_STATIC_ASSERT_BITMASK_TYPE
//   macro: static_assert that _B satisfies BitmaskType.
#define D_STATIC_ASSERT_BITMASK_TYPE(_B)                                   \
    static_assert(                                                         \
        djinterp::traits::is_bitmask_type_v<_B>,                           \
        "Type " #_B " does not satisfy BitmaskType")

// D_STATIC_ASSERT_REGEX_TRAITS
//   macro: static_assert that _Tr satisfies RegexTraits.
#define D_STATIC_ASSERT_REGEX_TRAITS(_Tr)                                  \
    static_assert(                                                         \
        djinterp::traits::is_regex_traits_v<_Tr>,                          \
        "Type " #_Tr " does not satisfy RegexTraits")


NS_END  // traits
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP23_OR_HIGHER


#endif  // DJINTERP_CPP_NAMED23_
