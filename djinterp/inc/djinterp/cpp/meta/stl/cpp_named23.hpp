/*******************************************************************************
* djinterp [cpp]                                                 cpp_named23.hpp
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
*   - D_STATIC_ASSERT_VALUE_TYPE(T) diagnostic macro
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
*
* path:      /inc/djinterp/cpp/meta/stl/cpp_named23.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.27
*                                                            revised: 2026.11.04
*******************************************************************************/

#ifndef DJINTERP_META_STL_CPP_NAMED23_HPP
#define DJINTERP_META_STL_CPP_NAMED23_HPP 1

// djinterp
#include "../../djinterp.hpp"  // framework root


// only meaningful in C++ mode
#ifndef __cplusplus
    #error "cpp_named23.hpp can only be used in C++ compilation mode"
#endif

// include C++20 layer
#include "cpp_named20.hpp"

// C++23 additions are active for C++23 and all later standards
#if D_ENV_LANG_IS_CPP23_OR_HIGHER

// std
#include <chrono>
#include <concepts>
#include <ios>
#include <iterator>
#include <random>
#include <type_traits>
// the feature-tested header, at file scope: a standard header inside a
// namespace declares the library in it
#if D_ENV_CPP_FEATURE_STL_MDSPAN
    #include <mdspan>
#endif  // D_ENV_CPP_FEATURE_STL_MDSPAN


NS_DJINTERP
D_NAMESPACE(stl)

// =========================================================================
// I.   EXPRESSION-BASED CONCEPTS (C++23)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

D_NAMESPACE(concepts)

    // explicit_default_constructible
    //   concept: constrains T to types where `T()` and `T{}`
    // are well-formed expressions (direct expression validation).
    template<typename T>
    concept explicit_default_constructible = requires
    {
        T();
        T{};
    };

    // explicit_destructible
    //   concept: constrains T to types where the destructor can be
    // explicitly invoked on a valid object.
    template<typename T>
    concept explicit_destructible = requires(T _obj)
    {
        _obj.~T();
    };

    // explicit_copy_constructible
    //   concept: constrains T to types where copy construction from
    // a const lvalue reference is a well-formed expression.
    template<typename T>
    concept explicit_copy_constructible = requires(const T _src)
    {
        T(_src);
    };

    // explicit_move_constructible
    //   concept: constrains T to types where move construction from
    // an rvalue reference is a well-formed expression.
    template<typename T>
    concept explicit_move_constructible = requires
    {
        T(std::declval<T&&>());
    };

    // explicit_copy_assignable
    //   concept: constrains T to types where copy assignment from a
    // const lvalue reference is a well-formed expression.
    template<typename T>
    concept explicit_copy_assignable = requires(T       _dst,
                                                const T _src)
    {
        { _dst = _src } -> std::same_as<T&>;
    };

    // explicit_move_assignable
    //   concept: constrains T to types where move assignment from an
    // rvalue reference is a well-formed expression.
    template<typename T>
    concept explicit_move_assignable = requires(T _dst)
    {
        { _dst = std::declval<T&&>() } -> std::same_as<T&>;
    };


    // =====================================================================
    // II.  NOTHROW COMPOUND CONCEPTS (C++23)
    // =====================================================================

    // nothrow_fully_copyable
    //   concept: constrains T to types satisfying both
    // NothrowCopyConstructible and NothrowCopyAssignable.
    template<typename T>
    concept nothrow_fully_copyable =
        ( nothrow_copy_constructible<T> &&
          nothrow_copy_assignable<T> );

    // nothrow_fully_movable
    //   concept: constrains T to types satisfying both
    // NothrowMoveConstructible and NothrowMoveAssignable.
    template<typename T>
    concept nothrow_fully_movable =
        ( nothrow_move_constructible<T> &&
          nothrow_move_assignable<T> );

    // nothrow_value_type
    //   concept: constrains T to types satisfying all six core named
    // requirements with nothrow guarantees.
    template<typename T>
    concept nothrow_value_type =
        ( nothrow_default_constructible<T> &&
          nothrow_copy_constructible<T>    &&
          nothrow_move_constructible<T>    &&
          nothrow_copy_assignable<T>       &&
          nothrow_move_assignable<T>       &&
          nothrow_destructible<T> );


    // =====================================================================
    // III. EXPRESSION-BASED COMPARISON CONCEPTS (C++23)
    // =====================================================================

    // explicit_equality_comparable
    //   concept: constrains T using requires-expressions to directly
    // test == and != with bool-convertible results.
    template<typename T>
    concept explicit_equality_comparable = requires(const T _a,
                                                    const T _b)
    {
        { _a == _b } -> std::convertible_to<bool>;
        { _a != _b } -> std::convertible_to<bool>;
        { _b == _a } -> std::convertible_to<bool>;
        { _b != _a } -> std::convertible_to<bool>;
    };

    // explicit_less_than_comparable
    //   concept: constrains T using requires-expressions to directly
    // test all four relational operators.
    template<typename T>
    concept explicit_less_than_comparable = requires(const T _a,
                                                     const T _b)
    {
        { _a <  _b } -> std::convertible_to<bool>;
        { _a >  _b } -> std::convertible_to<bool>;
        { _a <= _b } -> std::convertible_to<bool>;
        { _a >= _b } -> std::convertible_to<bool>;
    };

    // nothrow_swappable
    //   concept: constrains T to types that are swappable without
    // throwing exceptions.
    template<typename T>
    concept nothrow_swappable =
        ( swappable<T> &&
          std::is_nothrow_swappable_v<T> );

    // strict_weak_order
    //   concept: constrains F to types that structurally model a
    // strict weak ordering over T. Uses std::strict_weak_order.
    template<typename F,
             typename T>
    concept strict_weak_order =
        std::strict_weak_order<F, T, T>;

NS_END  // concepts

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// =========================================================================
// III. STATIC ASSERTION DIAGNOSTIC HELPERS (C++23)
// =========================================================================

// D_STATIC_ASSERT_DEFAULT_CONSTRUCTIBLE
//   macro: static_assert that T is default-constructible with a
// descriptive error message.
#define D_STATIC_ASSERT_DEFAULT_CONSTRUCTIBLE(T)                          \
    static_assert(                                                         \
        std::is_default_constructible_v<T>,                               \
        "Type " #T " does not satisfy DefaultConstructible")

// D_STATIC_ASSERT_COPY_CONSTRUCTIBLE
//   macro: static_assert that T is copy-constructible with a
// descriptive error message.
#define D_STATIC_ASSERT_COPY_CONSTRUCTIBLE(T)                             \
    static_assert(                                                         \
        std::is_copy_constructible_v<T>,                                  \
        "Type " #T " does not satisfy CopyConstructible")

// D_STATIC_ASSERT_MOVE_CONSTRUCTIBLE
//   macro: static_assert that T is move-constructible with a
// descriptive error message.
#define D_STATIC_ASSERT_MOVE_CONSTRUCTIBLE(T)                             \
    static_assert(                                                         \
        std::is_move_constructible_v<T>,                                  \
        "Type " #T " does not satisfy MoveConstructible")

// D_STATIC_ASSERT_COPY_ASSIGNABLE
//   macro: static_assert that T is copy-assignable with a
// descriptive error message.
#define D_STATIC_ASSERT_COPY_ASSIGNABLE(T)                                \
    static_assert(                                                         \
        std::is_copy_assignable_v<T>,                                     \
        "Type " #T " does not satisfy CopyAssignable")

// D_STATIC_ASSERT_MOVE_ASSIGNABLE
//   macro: static_assert that T is move-assignable with a
// descriptive error message.
#define D_STATIC_ASSERT_MOVE_ASSIGNABLE(T)                                \
    static_assert(                                                         \
        std::is_move_assignable_v<T>,                                     \
        "Type " #T " does not satisfy MoveAssignable")

// D_STATIC_ASSERT_DESTRUCTIBLE
//   macro: static_assert that T is destructible with a descriptive
// error message.
#define D_STATIC_ASSERT_DESTRUCTIBLE(T)                                   \
    static_assert(                                                         \
        std::is_destructible_v<T>,                                        \
        "Type " #T " does not satisfy Destructible")

// D_STATIC_ASSERT_VALUE_TYPE
//   macro: static_assert that T satisfies all six core named
// requirements. Produces individual error messages for each failed
// requirement.
#define D_STATIC_ASSERT_VALUE_TYPE(T)                                     \
    D_STATIC_ASSERT_DEFAULT_CONSTRUCTIBLE(T);                             \
    D_STATIC_ASSERT_COPY_CONSTRUCTIBLE(T);                                \
    D_STATIC_ASSERT_MOVE_CONSTRUCTIBLE(T);                                \
    D_STATIC_ASSERT_COPY_ASSIGNABLE(T);                                   \
    D_STATIC_ASSERT_MOVE_ASSIGNABLE(T);                                   \
    D_STATIC_ASSERT_DESTRUCTIBLE(T)


// =========================================================================
// IV.  IMPLICIT LIFETIME TYPE TRAIT (C++23)
// =========================================================================

// is_implicit_lifetime
//   trait: evaluates to true if T is an implicit-lifetime type (a type
// whose lifetime can begin without explicit constructor invocation).
// Implicit-lifetime types include: scalar types, implicit-lifetime
// class types, arrays of such types, and cv-qualified versions thereof.
// env has no D_ENV_CPP_FEATURE_STL_ macro for this library feature yet, so
// its SD-6 macro is tested directly
#if defined(__cpp_lib_is_implicit_lifetime)
template<typename T>
struct is_implicit_lifetime
{
    static constexpr bool value =
        std::is_implicit_lifetime_v<T>;
};
#else
// fallback: conservative approximation when std::is_implicit_lifetime
// is not available. This approximation covers scalar types, trivial
// types, and aggregates of such types but may produce false negatives
// for some implicit-lifetime class types.
template<typename T>
struct is_implicit_lifetime
{
    static constexpr bool value =
        ( std::is_scalar_v<T>            ||
          std::is_array_v<T>             ||
          std::is_trivially_copyable_v<T> );
};
#endif

// is_implicit_lifetime_v
//   value: shorthand for is_implicit_lifetime<T>::value.
template<typename T>
inline constexpr bool is_implicit_lifetime_v =
    is_implicit_lifetime<T>::value;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

D_NAMESPACE(concepts)

    // implicit_lifetime_type
    //   concept: constrains T to implicit-lifetime types whose
    // lifetime can begin without explicit constructor invocation.
    template<typename T>
    concept implicit_lifetime_type =
        is_implicit_lifetime_v<T>;

NS_END  // concepts

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// =========================================================================
// V.   TYPE CLASSIFICATION STATIC ASSERTION MACROS (C++23)
// =========================================================================

// D_STATIC_ASSERT_TRIVIALLY_COPYABLE
//   macro: static_assert that T is trivially copyable with a
// descriptive error message.
#define D_STATIC_ASSERT_TRIVIALLY_COPYABLE(T)                             \
    static_assert(                                                         \
        std::is_trivially_copyable_v<T>,                                  \
        "Type " #T " does not satisfy TriviallyCopyable")

// D_STATIC_ASSERT_STANDARD_LAYOUT
//   macro: static_assert that T is standard-layout with a descriptive
// error message.
#define D_STATIC_ASSERT_STANDARD_LAYOUT(T)                                \
    static_assert(                                                         \
        std::is_standard_layout_v<T>,                                     \
        "Type " #T " does not satisfy StandardLayoutType")

// D_STATIC_ASSERT_POD
//   macro: static_assert that T is a POD type with a descriptive
// error message.
// note: PODType is deprecated in C++20.
#define D_STATIC_ASSERT_POD(T)                                            \
    D_STATIC_ASSERT_TRIVIALLY_COPYABLE(T);                                \
    D_STATIC_ASSERT_STANDARD_LAYOUT(T);                                   \
    static_assert(                                                         \
        std::is_trivial_v<T>,                                             \
        "Type " #T " does not satisfy TrivialType (required for POD)")


// =========================================================================
// VI.  CONTAINER EXPRESSION-BASED CONCEPTS (C++23)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

D_NAMESPACE(concepts)

    // explicit_container
    //   concept: constrains C using requires-expressions to directly
    // test Container operations and nested types.
    template<typename C>
    concept explicit_container = requires(const C _c, C _m)
    {
        typename C::value_type;
        typename C::reference;
        typename C::const_reference;
        typename C::iterator;
        typename C::const_iterator;
        typename C::difference_type;
        typename C::size_type;
        { _c.begin()    } -> std::input_or_output_iterator;
        { _c.end()      } -> std::sentinel_for<decltype(_c.begin())>;
        { _c.cbegin()   } -> std::input_or_output_iterator;
        { _c.cend()     } -> std::sentinel_for<decltype(_c.cbegin())>;
        { _c.size()     } -> std::convertible_to<typename C::size_type>;
        { _c.max_size() } -> std::convertible_to<typename C::size_type>;
        { _c.empty()    } -> std::convertible_to<bool>;
    };

    // explicit_reversible_container
    //   concept: constrains C to types satisfying the
    // ReversibleContainer requirements via requires-expression.
    template<typename C>
    concept explicit_reversible_container =
        ( explicit_container<C> &&
          requires(const C _c)
          {
              typename C::reverse_iterator;
              typename C::const_reverse_iterator;
              { _c.rbegin()  } -> std::input_or_output_iterator;
              { _c.rend()    } -> std::sentinel_for<decltype(_c.rbegin())>;
              { _c.crbegin() } -> std::input_or_output_iterator;
              { _c.crend()   } -> std::sentinel_for<decltype(_c.crbegin())>;
          } );

    // explicit_allocator_aware_container
    //   concept: constrains C to types satisfying the
    // AllocatorAwareContainer requirements via requires-expression.
    template<typename C>
    concept explicit_allocator_aware_container =
        ( explicit_container<C> &&
          requires(const C _c)
          {
              typename C::allocator_type;
              { _c.get_allocator() }
                  -> std::same_as<typename C::allocator_type>;
          } );

    // explicit_contiguous_container
    //   concept: constrains C to types satisfying the
    // ContiguousContainer requirements via requires-expression.
    template<typename C>
    concept explicit_contiguous_container =
        ( explicit_container<C> &&
          requires(const C _c)
          {
              { _c.data() }
                  -> std::same_as<const typename C::value_type*>;
          } );

    // explicit_associative_container
    //   concept: constrains C to types satisfying the
    // AssociativeContainer requirements via requires-expression.
    template<typename C>
    concept explicit_associative_container =
        ( explicit_container<C> &&
          requires(const C _c,
                   const typename C::key_type& _k)
          {
              typename C::key_type;
              typename C::key_compare;
              typename C::value_compare;
              _c.find(_k);
              { _c.count(_k) }
                  -> std::convertible_to<typename C::size_type>;
              _c.lower_bound(_k);
              _c.upper_bound(_k);
              _c.equal_range(_k);
          } );

    // explicit_unordered_associative_container
    //   concept: constrains C to types satisfying the
    // UnorderedAssociativeContainer requirements via
    // requires-expression.
    template<typename C>
    concept explicit_unordered_associative_container =
        ( explicit_container<C> &&
          requires(const C _c)
          {
              typename C::hasher;
              typename C::key_equal;
              _c.hash_function();
              _c.key_eq();
              { _c.bucket_count() }
                  -> std::convertible_to<typename C::size_type>;
              _c.load_factor();
              _c.max_load_factor();
          } );

NS_END  // concepts

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// =========================================================================
// VII. CONTAINER STATIC ASSERTION MACROS (C++23)
// =========================================================================

// D_STATIC_ASSERT_CONTAINER
//   macro: static_assert that C satisfies the Container named
// requirement.
#define D_STATIC_ASSERT_CONTAINER(C)                                      \
    static_assert(                                                         \
        djinterp::re_std::is_container_v<C>,                              \
        "Type " #C " does not satisfy Container")

// D_STATIC_ASSERT_REVERSIBLE_CONTAINER
//   macro: static_assert that C satisfies the ReversibleContainer
// named requirement.
#define D_STATIC_ASSERT_REVERSIBLE_CONTAINER(C)                           \
    D_STATIC_ASSERT_CONTAINER(C);                                         \
    static_assert(                                                         \
        djinterp::re_std::is_reversible_container_v<C>,                   \
        "Type " #C " does not satisfy ReversibleContainer")

// D_STATIC_ASSERT_SEQUENCE_CONTAINER
//   macro: static_assert that C satisfies the SequenceContainer
// named requirement.
#define D_STATIC_ASSERT_SEQUENCE_CONTAINER(C)                             \
    D_STATIC_ASSERT_CONTAINER(C);                                         \
    static_assert(                                                         \
        djinterp::re_std::is_sequence_container_v<C>,                     \
        "Type " #C " does not satisfy SequenceContainer")

// D_STATIC_ASSERT_ASSOCIATIVE_CONTAINER
//   macro: static_assert that C satisfies the AssociativeContainer
// named requirement.
#define D_STATIC_ASSERT_ASSOCIATIVE_CONTAINER(C)                          \
    D_STATIC_ASSERT_CONTAINER(C);                                         \
    static_assert(                                                         \
        djinterp::re_std::is_associative_container_v<C>,                  \
        "Type " #C " does not satisfy AssociativeContainer")

// D_STATIC_ASSERT_UNORDERED_ASSOCIATIVE_CONTAINER
//   macro: static_assert that C satisfies the
// UnorderedAssociativeContainer named requirement.
#define D_STATIC_ASSERT_UNORDERED_ASSOCIATIVE_CONTAINER(C)                \
    D_STATIC_ASSERT_CONTAINER(C);                                         \
    static_assert(                                                         \
        djinterp::re_std::is_unordered_associative_container_v<C>,        \
        "Type " #C " does not satisfy UnorderedAssociativeContainer")


// =========================================================================
// VIII. CONTAINER ELEMENT EXPRESSION-BASED CONCEPTS (C++23)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

D_NAMESPACE(concepts)

    // explicit_default_insertable
    //   concept: constrains T to types that are DefaultInsertable
    // using allocator A, verified via requires-expression on
    // allocator_traits::construct.
    template<typename T,
             typename A = std::allocator<T>>
    concept explicit_default_insertable = requires(A _a, T* _p)
    {
        std::allocator_traits<A>::construct(_a, _p);
    };

    // explicit_copy_insertable
    //   concept: constrains T to types that are CopyInsertable
    // using allocator A, verified via requires-expression.
    template<typename T,
             typename A = std::allocator<T>>
    concept explicit_copy_insertable = requires(A       _a,
                                                T*      _p,
                                                const T& _v)
    {
        std::allocator_traits<A>::construct(_a, _p, _v);
    };

    // explicit_move_insertable
    //   concept: constrains T to types that are MoveInsertable
    // using allocator A, verified via requires-expression.
    template<typename T,
             typename A = std::allocator<T>>
    concept explicit_move_insertable = requires(A  _a,
                                                T* _p,
                                                T  _rv)
    {
        std::allocator_traits<A>::construct(
            _a, _p, static_cast<T&&>(_rv));
    };

    // explicit_erasable
    //   concept: constrains T to types that are Erasable using
    // allocator A, verified via requires-expression on
    // allocator_traits::destroy.
    template<typename T,
             typename A = std::allocator<T>>
    concept explicit_erasable = requires(A _a, T* _p)
    {
        std::allocator_traits<A>::destroy(_a, _p);
    };

NS_END  // concepts

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// =========================================================================
// IX.  CONTAINER ELEMENT STATIC ASSERTION MACROS (C++23)
// =========================================================================

// D_STATIC_ASSERT_DEFAULT_INSERTABLE
//   macro: static_assert that T is DefaultInsertable with
// allocator A (defaults to std::allocator<T>).
#define D_STATIC_ASSERT_DEFAULT_INSERTABLE(T, ...)                        \
    static_assert(                                                         \
        djinterp::re_std::is_default_insertable_v<T __VA_OPT__(,)        \
            __VA_ARGS__>,                                                  \
        "Type " #T " does not satisfy DefaultInsertable")

// D_STATIC_ASSERT_COPY_INSERTABLE
//   macro: static_assert that T is CopyInsertable with
// allocator A (defaults to std::allocator<T>).
#define D_STATIC_ASSERT_COPY_INSERTABLE(T, ...)                           \
    static_assert(                                                         \
        djinterp::re_std::is_copy_insertable_v<T __VA_OPT__(,)           \
            __VA_ARGS__>,                                                  \
        "Type " #T " does not satisfy CopyInsertable")

// D_STATIC_ASSERT_MOVE_INSERTABLE
//   macro: static_assert that T is MoveInsertable with
// allocator A (defaults to std::allocator<T>).
#define D_STATIC_ASSERT_MOVE_INSERTABLE(T, ...)                           \
    static_assert(                                                         \
        djinterp::re_std::is_move_insertable_v<T __VA_OPT__(,)           \
            __VA_ARGS__>,                                                  \
        "Type " #T " does not satisfy MoveInsertable")

// D_STATIC_ASSERT_ERASABLE
//   macro: static_assert that T is Erasable with allocator A
// (defaults to std::allocator<T>).
#define D_STATIC_ASSERT_ERASABLE(T, ...)                                  \
    static_assert(                                                         \
        djinterp::re_std::is_erasable_v<T __VA_OPT__(,)                  \
            __VA_ARGS__>,                                                  \
        "Type " #T " does not satisfy Erasable")


// =========================================================================
// X.   ITERATOR CONCEPTS (C++23)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

D_NAMESPACE(concepts)

    // constexpr_iterator
    //   concept: constrains I to types satisfying the
    // ConstexprIterator named requirement: a LegacyIterator whose
    // operations can be evaluated in a constant expression.
    // note: detection checks that I is a literal type (trivially
    // destructible or constexpr destructible) and satisfies
    // std::semiregular (constexpr copy/move/default-constructible).
    // Full constexpr-evaluability of all operations cannot be verified
    // at compile time without attempting actual constant evaluation.
    template<typename I>
    concept constexpr_iterator =
        ( legacy_iterator<I>          &&
          std::semiregular<I>         &&
          std::is_trivially_destructible_v<I> );

    // explicit_legacy_input_iterator
    //   concept: constrains I using requires-expressions to directly
    // test LegacyInputIterator operations.
    template<typename I>
    concept explicit_legacy_input_iterator = requires(I       _it,
                                                      const I _cit)
    {
        { *_it }     -> std::common_reference_with<
                            typename std::iterator_traits<I>::reference>;
        { ++_it }    -> std::same_as<I&>;
        { _cit == _cit } -> std::convertible_to<bool>;
        { _cit != _cit } -> std::convertible_to<bool>;
        *_it++;
    };

    // explicit_legacy_forward_iterator
    //   concept: constrains I using requires-expressions to directly
    // test LegacyForwardIterator operations.
    template<typename I>
    concept explicit_legacy_forward_iterator =
        ( explicit_legacy_input_iterator<I> &&
          std::default_initializable<I>     &&
          requires(I _it)
          {
              { _it++ } -> std::convertible_to<const I&>;
              { *_it++ } -> std::common_reference_with<
                                typename std::iterator_traits<I>::reference>;
          } );

    // explicit_legacy_bidirectional_iterator
    //   concept: constrains I using requires-expressions to directly
    // test LegacyBidirectionalIterator operations.
    template<typename I>
    concept explicit_legacy_bidirectional_iterator =
        ( explicit_legacy_forward_iterator<I> &&
          requires(I _it)
          {
              { --_it }  -> std::same_as<I&>;
              { _it-- }  -> std::convertible_to<const I&>;
              { *_it-- } -> std::common_reference_with<
                                typename std::iterator_traits<I>::reference>;
          } );

    // explicit_legacy_random_access_iterator
    //   concept: constrains I using requires-expressions to directly
    // test LegacyRandomAccessIterator operations.
    template<typename I>
    concept explicit_legacy_random_access_iterator =
        ( explicit_legacy_bidirectional_iterator<I> &&
          std::totally_ordered<I>                   &&
          requires(I       _it,
                   const I _cit,
                   typename std::iterator_traits<I>::difference_type _n)
          {
              { _it  += _n }    -> std::same_as<I&>;
              { _it  -= _n }    -> std::same_as<I&>;
              { _cit +  _n }    -> std::same_as<I>;
              { _n   +  _cit }  -> std::same_as<I>;
              { _cit -  _n }    -> std::same_as<I>;
              { _cit -  _cit }  -> std::same_as<
                  typename std::iterator_traits<I>::difference_type>;
              { _cit[_n] }      -> std::common_reference_with<
                  typename std::iterator_traits<I>::reference>;
          } );

NS_END  // concepts

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// =========================================================================
// XI.  ITERATOR STATIC ASSERTION MACROS (C++23)
// =========================================================================

// D_STATIC_ASSERT_LEGACY_ITERATOR
//   macro: static_assert that I satisfies the LegacyIterator
// named requirement.
#define D_STATIC_ASSERT_LEGACY_ITERATOR(I)                                \
    static_assert(                                                         \
        djinterp::re_std::is_legacy_iterator_v<I>,                        \
        "Type " #I " does not satisfy LegacyIterator")

// D_STATIC_ASSERT_LEGACY_INPUT_ITERATOR
//   macro: static_assert that I satisfies the
// LegacyInputIterator named requirement.
#define D_STATIC_ASSERT_LEGACY_INPUT_ITERATOR(I)                          \
    D_STATIC_ASSERT_LEGACY_ITERATOR(I);                                   \
    static_assert(                                                         \
        djinterp::re_std::is_legacy_input_iterator_v<I>,                  \
        "Type " #I " does not satisfy LegacyInputIterator")

// D_STATIC_ASSERT_LEGACY_FORWARD_ITERATOR
//   macro: static_assert that I satisfies the
// LegacyForwardIterator named requirement.
#define D_STATIC_ASSERT_LEGACY_FORWARD_ITERATOR(I)                        \
    D_STATIC_ASSERT_LEGACY_INPUT_ITERATOR(I);                             \
    static_assert(                                                         \
        djinterp::re_std::is_legacy_forward_iterator_v<I>,                \
        "Type " #I " does not satisfy LegacyForwardIterator")

// D_STATIC_ASSERT_LEGACY_BIDIRECTIONAL_ITERATOR
//   macro: static_assert that I satisfies the
// LegacyBidirectionalIterator named requirement.
#define D_STATIC_ASSERT_LEGACY_BIDIRECTIONAL_ITERATOR(I)                  \
    D_STATIC_ASSERT_LEGACY_FORWARD_ITERATOR(I);                           \
    static_assert(                                                         \
        djinterp::re_std::is_legacy_bidirectional_iterator_v<I>,          \
        "Type " #I " does not satisfy LegacyBidirectionalIterator")

// D_STATIC_ASSERT_LEGACY_RANDOM_ACCESS_ITERATOR
//   macro: static_assert that I satisfies the
// LegacyRandomAccessIterator named requirement.
#define D_STATIC_ASSERT_LEGACY_RANDOM_ACCESS_ITERATOR(I)                  \
    D_STATIC_ASSERT_LEGACY_BIDIRECTIONAL_ITERATOR(I);                     \
    static_assert(                                                         \
        djinterp::re_std::is_legacy_random_access_iterator_v<I>,          \
        "Type " #I " does not satisfy LegacyRandomAccessIterator")


// =========================================================================
// XII. STREAM I/O AND RANDOM EXPRESSION-BASED CONCEPTS (C++23)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

D_NAMESPACE(concepts)

    // explicit_unformatted_input_stream
    //   concept: constrains S using requires-expressions to directly
    // test unformatted input operations.
    template<typename S>
    concept explicit_unformatted_input_stream =
        requires(S _s,
                 typename S::char_type* _buf,
                 std::streamsize _n)
    {
        { _s.get()       } -> std::convertible_to<typename S::int_type>;
        { _s.read(_buf, _n) } -> std::same_as<S&>;
        { _s.gcount()    } -> std::convertible_to<std::streamsize>;
    };

    // explicit_unformatted_output_stream
    //   concept: constrains S using requires-expressions to directly
    // test unformatted output operations.
    template<typename S>
    concept explicit_unformatted_output_stream =
        requires(S _s,
                 typename S::char_type _ch,
                 const typename S::char_type* _buf,
                 std::streamsize _n)
    {
        { _s.put(_ch)         } -> std::same_as<S&>;
        { _s.write(_buf, _n)  } -> std::same_as<S&>;
    };

    // explicit_uniform_random_bit_generator
    //   concept: constrains G using requires-expressions to directly
    // test UniformRandomBitGenerator operations with return type
    // constraints.
    template<typename G>
    concept explicit_uniform_random_bit_generator =
        ( std::unsigned_integral<typename G::result_type> &&
          requires(G _g)
          {
              { _g()     } -> std::same_as<typename G::result_type>;
              { G::min() } -> std::same_as<typename G::result_type>;
              { G::max() } -> std::same_as<typename G::result_type>;
          } );

    // explicit_random_number_engine
    //   concept: constrains E using requires-expressions to directly
    // test RandomNumberEngine operations.
    template<typename E>
    concept explicit_random_number_engine =
        ( explicit_uniform_random_bit_generator<E> &&
          requires(E _e,
                   typename E::result_type _val,
                   unsigned long long _z)
          {
              _e.seed();
              _e.seed(_val);
              _e.discard(_z);
              { _e == _e } -> std::convertible_to<bool>;
              { _e != _e } -> std::convertible_to<bool>;
          } );

    // explicit_random_number_distribution
    //   concept: constrains D using requires-expressions to directly
    // test RandomNumberDistribution operations.
    template<typename D>
    concept explicit_random_number_distribution =
        requires(D _d, const D _cd, std::mt19937 _gen)
    {
        typename D::result_type;
        typename D::param_type;
        { _d(_gen)   } -> std::same_as<typename D::result_type>;
        { _cd.param() } -> std::same_as<typename D::param_type>;
        { _cd.min()  } -> std::same_as<typename D::result_type>;
        { _cd.max()  } -> std::same_as<typename D::result_type>;
        _d.reset();
    };

NS_END  // concepts

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// =========================================================================
// XIII. STREAM I/O AND RANDOM STATIC ASSERTION MACROS (C++23)
// =========================================================================

// D_STATIC_ASSERT_UNFORMATTED_INPUT_STREAM
//   macro: static_assert that S supports unformatted input.
#define D_STATIC_ASSERT_UNFORMATTED_INPUT_STREAM(S)                       \
    static_assert(                                                         \
        djinterp::re_std::is_unformatted_input_stream_v<S>,               \
        "Type " #S " does not support UnformattedInputFunction")

// D_STATIC_ASSERT_FORMATTED_INPUT_STREAM
//   macro: static_assert that S supports formatted input.
#define D_STATIC_ASSERT_FORMATTED_INPUT_STREAM(S)                         \
    static_assert(                                                         \
        djinterp::re_std::is_formatted_input_stream_v<S>,                 \
        "Type " #S " does not support FormattedInputFunction")

// D_STATIC_ASSERT_UNFORMATTED_OUTPUT_STREAM
//   macro: static_assert that S supports unformatted output.
#define D_STATIC_ASSERT_UNFORMATTED_OUTPUT_STREAM(S)                      \
    static_assert(                                                         \
        djinterp::re_std::is_unformatted_output_stream_v<S>,              \
        "Type " #S " does not support UnformattedOutputFunction")

// D_STATIC_ASSERT_FORMATTED_OUTPUT_STREAM
//   macro: static_assert that S supports formatted output.
#define D_STATIC_ASSERT_FORMATTED_OUTPUT_STREAM(S)                        \
    static_assert(                                                         \
        djinterp::re_std::is_formatted_output_stream_v<S>,                \
        "Type " #S " does not support FormattedOutputFunction")

// D_STATIC_ASSERT_UNIFORM_RANDOM_BIT_GENERATOR
//   macro: static_assert that G satisfies
// UniformRandomBitGenerator.
#define D_STATIC_ASSERT_UNIFORM_RANDOM_BIT_GENERATOR(G)                   \
    static_assert(                                                         \
        djinterp::re_std::is_uniform_random_bit_generator_v<G>,           \
        "Type " #G " does not satisfy UniformRandomBitGenerator")

// D_STATIC_ASSERT_RANDOM_NUMBER_ENGINE
//   macro: static_assert that E satisfies RandomNumberEngine.
#define D_STATIC_ASSERT_RANDOM_NUMBER_ENGINE(E)                           \
    D_STATIC_ASSERT_UNIFORM_RANDOM_BIT_GENERATOR(E);                     \
    static_assert(                                                         \
        djinterp::re_std::is_random_number_engine_v<E>,                   \
        "Type " #E " does not satisfy RandomNumberEngine")

// D_STATIC_ASSERT_RANDOM_NUMBER_DISTRIBUTION
//   macro: static_assert that D satisfies
// RandomNumberDistribution.
#define D_STATIC_ASSERT_RANDOM_NUMBER_DISTRIBUTION(D)                     \
    static_assert(                                                         \
        djinterp::re_std::is_random_number_distribution_v<D>,             \
        "Type " #D " does not satisfy RandomNumberDistribution")


// =========================================================================
// XIV. CONCURRENCY EXPRESSION-BASED CONCEPTS (C++23)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

D_NAMESPACE(concepts)

    // explicit_basic_lockable
    //   concept: constrains M using requires-expressions to directly
    // test BasicLockable operations.
    template<typename M>
    concept explicit_basic_lockable = requires(M _m)
    {
        _m.lock();
        _m.unlock();
    };

    // explicit_lockable
    //   concept: constrains M using requires-expressions to directly
    // test Lockable operations.
    template<typename M>
    concept explicit_lockable =
        ( explicit_basic_lockable<M> &&
          requires(M _m)
          {
              { _m.try_lock() } -> std::convertible_to<bool>;
          } );

    // explicit_timed_lockable
    //   concept: constrains M using requires-expressions to directly
    // test TimedLockable operations.
    template<typename M>
    concept explicit_timed_lockable =
        ( explicit_lockable<M> &&
          requires(M                                  _m,
                   std::chrono::milliseconds           _dur,
                   std::chrono::steady_clock::time_point _tp)
          {
              { _m.try_lock_for(_dur) }   -> std::convertible_to<bool>;
              { _m.try_lock_until(_tp) }  -> std::convertible_to<bool>;
          } );

    // explicit_shared_lockable
    //   concept: constrains M using requires-expressions to directly
    // test SharedLockable operations.
    template<typename M>
    concept explicit_shared_lockable = requires(M _m)
    {
        _m.lock_shared();
        _m.unlock_shared();
        { _m.try_lock_shared() } -> std::convertible_to<bool>;
    };

    // explicit_shared_timed_lockable
    //   concept: constrains M using requires-expressions to directly
    // test SharedTimedLockable operations.
    template<typename M>
    concept explicit_shared_timed_lockable =
        ( explicit_shared_lockable<M> &&
          requires(M                                  _m,
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
//   macro: static_assert that M satisfies BasicLockable.
#define D_STATIC_ASSERT_BASIC_LOCKABLE(M)                                 \
    static_assert(                                                         \
        djinterp::re_std::is_basic_lockable_v<M>,                         \
        "Type " #M " does not satisfy BasicLockable")

// D_STATIC_ASSERT_LOCKABLE
//   macro: static_assert that M satisfies Lockable.
#define D_STATIC_ASSERT_LOCKABLE(M)                                       \
    D_STATIC_ASSERT_BASIC_LOCKABLE(M);                                    \
    static_assert(                                                         \
        djinterp::re_std::is_lockable_v<M>,                               \
        "Type " #M " does not satisfy Lockable")

// D_STATIC_ASSERT_TIMED_LOCKABLE
//   macro: static_assert that M satisfies TimedLockable.
#define D_STATIC_ASSERT_TIMED_LOCKABLE(M)                                 \
    D_STATIC_ASSERT_LOCKABLE(M);                                          \
    static_assert(                                                         \
        djinterp::re_std::is_timed_lockable_v<M>,                         \
        "Type " #M " does not satisfy TimedLockable")

// D_STATIC_ASSERT_MUTEX
//   macro: static_assert that M satisfies Mutex.
#define D_STATIC_ASSERT_MUTEX(M)                                          \
    D_STATIC_ASSERT_LOCKABLE(M);                                          \
    static_assert(                                                         \
        djinterp::re_std::is_mutex_v<M>,                                  \
        "Type " #M " does not satisfy Mutex")

// D_STATIC_ASSERT_SHARED_MUTEX
//   macro: static_assert that M satisfies SharedMutex.
#define D_STATIC_ASSERT_SHARED_MUTEX(M)                                   \
    D_STATIC_ASSERT_MUTEX(M);                                             \
    static_assert(                                                         \
        djinterp::re_std::is_shared_mutex_v<M>,                           \
        "Type " #M " does not satisfy SharedMutex")


// =========================================================================
// XVI. RANGE EXPRESSION-BASED CONCEPTS (C++23)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
#if D_ENV_CPP_FEATURE_STL_RANGES

D_NAMESPACE(concepts)

    // explicit_range
    //   concept: constrains R using requires-expressions to directly
    // test range operations with return-type constraints.
    template<typename R>
    concept explicit_range = requires(R _r)
    {
        { std::ranges::begin(_r) } -> std::input_or_output_iterator;
        { std::ranges::end(_r) }
            -> std::sentinel_for<decltype(std::ranges::begin(_r))>;
    };

    // explicit_range_adaptor_closure_object
    //   concept: constrains A using requires-expressions to directly
    // test pipe syntax (range | A) producing a valid range.
    template<typename A>
    concept explicit_range_adaptor_closure_object =
        requires(A _a, std::ranges::iota_view<int, int> _r)
    {
        { _r | _a } -> std::ranges::range;
    };

NS_END  // concepts

#endif  // D_ENV_CPP_FEATURE_STL_RANGES
#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// =========================================================================
// XVII. MULTIDIMENSIONAL VIEW CUSTOMIZATION TRAITS (C++23)
// =========================================================================

#if D_ENV_CPP_FEATURE_STL_MDSPAN

NS_INTERNAL

    // has_layout_mapping_ops
    //   trait: detects LayoutMapping members: extents(),
    // required_span_size(), is_always_unique(), is_always_exhaustive(),
    // is_always_strided(), is_unique(), is_exhaustive(), is_strided(),
    // and operator()(indices...).
    template<typename M,
             typename = void>
    struct has_layout_mapping_ops
    {
        static constexpr bool value = false;
    };

    template<typename M>
    struct has_layout_mapping_ops<M,
        decltype(static_cast<void>(
            std::declval<const M&>().extents(),
            std::declval<const M&>().required_span_size(),
            static_cast<bool>(M::is_always_unique()),
            static_cast<bool>(M::is_always_exhaustive()),
            static_cast<bool>(M::is_always_strided()),
            static_cast<bool>(std::declval<const M&>().is_unique()),
            static_cast<bool>(std::declval<const M&>().is_exhaustive()),
            static_cast<bool>(std::declval<const M&>().is_strided())
        ))>
    {
        static constexpr bool value = true;
    };

    // has_layout_mapping_policy_type
    //   trait: detects if L has a nested mapping<E> template that
    // is itself a layout mapping.
    template<typename L,
             typename E,
             typename = void>
    struct has_layout_mapping_policy_type
    {
        static constexpr bool value = false;
    };

    template<typename L,
             typename E>
    struct has_layout_mapping_policy_type<L, E,
        decltype(static_cast<void>(
            std::declval<typename L::template mapping<E>>()
        ))>
    {
        static constexpr bool value =
            has_layout_mapping_ops<
                typename L::template mapping<E>>::value;
    };

    // has_accessor_policy_ops
    //   trait: detects AccessorPolicy members: element_type,
    // reference (nested types), data_handle_type, offset(h, i),
    // access(h, i).
    template<typename A,
             typename = void>
    struct has_accessor_policy_ops
    {
        static constexpr bool value = false;
    };

    template<typename A>
    struct has_accessor_policy_ops<A,
        decltype(static_cast<void>(
            std::declval<typename A::element_type>(),
            std::declval<typename A::reference>(),
            std::declval<typename A::data_handle_type>(),
            std::declval<const A&>().offset(
                std::declval<typename A::data_handle_type>(),
                std::size_t(0)),
            std::declval<const A&>().access(
                std::declval<typename A::data_handle_type>(),
                std::size_t(0))
        ))>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// is_layout_mapping
//   trait: evaluates to true if M satisfies the structural
// requirements of the LayoutMapping named requirement: has
// extents(), required_span_size(), uniqueness/exhaustiveness/
// stridedness static and member queries.
template<typename M>
struct is_layout_mapping
{
    static constexpr bool value =
        internal::has_layout_mapping_ops<M>::value;
};

// is_layout_mapping_policy
//   trait: evaluates to true if L satisfies the
// LayoutMappingPolicy named requirement: has a nested
// mapping<E> template that is itself a LayoutMapping.
// default: E = std::extents<std::size_t, std::dynamic_extent>.
template<typename L,
         typename E = std::extents<std::size_t, std::dynamic_extent>>
struct is_layout_mapping_policy
{
    static constexpr bool value =
        internal::has_layout_mapping_policy_type<L, E>::value;
};

// is_accessor_policy
//   trait: evaluates to true if A satisfies the structural
// requirements of the AccessorPolicy named requirement: has
// element_type, reference, data_handle_type nested types, and
// offset(h, i), access(h, i) members.
template<typename A>
struct is_accessor_policy
{
    static constexpr bool value =
        ( internal::has_accessor_policy_ops<A>::value &&
          std::is_copy_constructible_v<A>            &&
          std::is_move_constructible_v<A>            &&
          std::is_copy_assignable_v<A>               &&
          std::is_move_assignable_v<A> );
};

// is_layout_mapping_v (inline constexpr)
//   value: ODR-safe shorthand for is_layout_mapping<M>::value.
template<typename M>
inline constexpr bool is_layout_mapping_v =
    is_layout_mapping<M>::value;

// is_layout_mapping_policy_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_layout_mapping_policy<L, E>::value.
template<typename L,
         typename E = std::extents<std::size_t, std::dynamic_extent>>
inline constexpr bool is_layout_mapping_policy_v =
    is_layout_mapping_policy<L, E>::value;

// is_accessor_policy_v (inline constexpr)
//   value: ODR-safe shorthand for is_accessor_policy<A>::value.
template<typename A>
inline constexpr bool is_accessor_policy_v =
    is_accessor_policy<A>::value;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

D_NAMESPACE(concepts)

    // layout_mapping
    //   concept: constrains M to types satisfying the
    // LayoutMapping named requirement.
    template<typename M>
    concept layout_mapping =
        is_layout_mapping_v<M>;

    // layout_mapping_policy
    //   concept: constrains L to types satisfying the
    // LayoutMappingPolicy named requirement for extents E.
    template<typename L,
             typename E = std::extents<std::size_t,
                                        std::dynamic_extent>>
    concept layout_mapping_policy =
        is_layout_mapping_policy_v<L, E>;

    // accessor_policy
    //   concept: constrains A to types satisfying the
    // AccessorPolicy named requirement.
    template<typename A>
    concept accessor_policy =
        is_accessor_policy_v<A>;

    // explicit_layout_mapping
    //   concept: constrains M using requires-expressions to directly
    // test LayoutMapping operations with return-type constraints.
    template<typename M>
    concept explicit_layout_mapping = requires(const M _m)
    {
        { _m.extents() };
        { _m.required_span_size() }
            -> std::convertible_to<typename decltype(
                   _m.extents())::size_type>;
        { M::is_always_unique() }     -> std::convertible_to<bool>;
        { M::is_always_exhaustive() } -> std::convertible_to<bool>;
        { M::is_always_strided() }    -> std::convertible_to<bool>;
        { _m.is_unique() }             -> std::convertible_to<bool>;
        { _m.is_exhaustive() }         -> std::convertible_to<bool>;
        { _m.is_strided() }            -> std::convertible_to<bool>;
    };

    // explicit_accessor_policy
    //   concept: constrains A using requires-expressions to directly
    // test AccessorPolicy operations with type constraints.
    template<typename A>
    concept explicit_accessor_policy = requires(const A               _a,
                                                typename A::data_handle_type _h,
                                                std::size_t                   _i)
    {
        typename A::element_type;
        typename A::reference;
        typename A::data_handle_type;
        { _a.offset(_h, _i) }
            -> std::same_as<typename A::data_handle_type>;
        { _a.access(_h, _i) }
            -> std::same_as<typename A::reference>;
    };

NS_END  // concepts

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// =========================================================================
// XVIII. MDSPAN STATIC ASSERTION MACROS (C++23)
// =========================================================================

// D_STATIC_ASSERT_LAYOUT_MAPPING
//   macro: static_assert that M satisfies LayoutMapping.
#define D_STATIC_ASSERT_LAYOUT_MAPPING(M)                                 \
    static_assert(                                                         \
        djinterp::re_std::is_layout_mapping_v<M>,                         \
        "Type " #M " does not satisfy LayoutMapping")

// D_STATIC_ASSERT_LAYOUT_MAPPING_POLICY
//   macro: static_assert that L satisfies LayoutMappingPolicy.
#define D_STATIC_ASSERT_LAYOUT_MAPPING_POLICY(L)                          \
    static_assert(                                                         \
        djinterp::re_std::is_layout_mapping_policy_v<L>,                  \
        "Type " #L " does not satisfy LayoutMappingPolicy")

// D_STATIC_ASSERT_ACCESSOR_POLICY
//   macro: static_assert that A satisfies AccessorPolicy.
#define D_STATIC_ASSERT_ACCESSOR_POLICY(A)                                \
    static_assert(                                                         \
        djinterp::re_std::is_accessor_policy_v<A>,                        \
        "Type " #A " does not satisfy AccessorPolicy")

#endif  // D_ENV_CPP_FEATURE_STL_MDSPAN


// =========================================================================
// XIX. MISCELLANEOUS EXPRESSION-BASED CONCEPTS (C++23)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

D_NAMESPACE(concepts)

    // explicit_clock_type
    //   concept: constrains C using requires-expressions to directly
    // test Clock operations with return-type constraints.
    template<typename C>
    concept explicit_clock_type = requires
    {
        typename C::rep;
        typename C::period;
        typename C::duration;
        typename C::time_point;
        { C::is_steady } -> std::convertible_to<bool>;
        { C::now() }     -> std::same_as<typename C::time_point>;
    };

    // explicit_char_traits_type
    //   concept: constrains Tr using requires-expressions to directly
    // test CharTraits operations.
    template<typename Tr>
    concept explicit_char_traits_type =
        requires(typename Tr::char_type                _ch,
                 typename Tr::int_type                 _i,
                 const typename Tr::char_type*         _p)
    {
        typename Tr::char_type;
        typename Tr::int_type;
        typename Tr::off_type;
        typename Tr::pos_type;
        typename Tr::state_type;
        { Tr::eq(_ch, _ch) }        -> std::convertible_to<bool>;
        { Tr::lt(_ch, _ch) }        -> std::convertible_to<bool>;
        { Tr::length(_p) }          -> std::convertible_to<std::size_t>;
        { Tr::to_char_type(_i) }    -> std::same_as<typename Tr::char_type>;
        { Tr::to_int_type(_ch) }    -> std::same_as<typename Tr::int_type>;
        { Tr::eof() }               -> std::same_as<typename Tr::int_type>;
        { Tr::not_eof(_i) }         -> std::same_as<typename Tr::int_type>;
    };

    // explicit_bitmask_type
    //   concept: constrains B using requires-expressions to directly
    // test BitmaskType bitwise operations.
    template<typename B>
    concept explicit_bitmask_type = requires(B _a, B _b)
    {
        { _a & _b }  -> std::convertible_to<B>;
        { _a | _b }  -> std::convertible_to<B>;
        { _a ^ _b }  -> std::convertible_to<B>;
        { ~_a }      -> std::convertible_to<B>;
        { _a &= _b } -> std::same_as<B&>;
        { _a |= _b } -> std::same_as<B&>;
        { _a ^= _b } -> std::same_as<B&>;
    };

NS_END  // concepts

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// =========================================================================
// XX.  MISCELLANEOUS STATIC ASSERTION MACROS (C++23)
// =========================================================================

// D_STATIC_ASSERT_CLOCK
//   macro: static_assert that C satisfies Clock.
#define D_STATIC_ASSERT_CLOCK(C)                                          \
    static_assert(                                                         \
        djinterp::re_std::is_clock_v<C>,                                  \
        "Type " #C " does not satisfy Clock")

// D_STATIC_ASSERT_TRIVIAL_CLOCK
//   macro: static_assert that C satisfies TrivialClock.
#define D_STATIC_ASSERT_TRIVIAL_CLOCK(C)                                  \
    D_STATIC_ASSERT_CLOCK(C);                                             \
    static_assert(                                                         \
        djinterp::re_std::is_trivial_clock_v<C>,                          \
        "Type " #C " does not satisfy TrivialClock")

// D_STATIC_ASSERT_CHAR_TRAITS
//   macro: static_assert that Tr satisfies CharTraits.
#define D_STATIC_ASSERT_CHAR_TRAITS(Tr)                                   \
    static_assert(                                                         \
        djinterp::re_std::is_char_traits_v<Tr>,                           \
        "Type " #Tr " does not satisfy CharTraits")

// D_STATIC_ASSERT_BITMASK_TYPE
//   macro: static_assert that B satisfies BitmaskType.
#define D_STATIC_ASSERT_BITMASK_TYPE(B)                                   \
    static_assert(                                                         \
        djinterp::re_std::is_bitmask_type_v<B>,                           \
        "Type " #B " does not satisfy BitmaskType")

// D_STATIC_ASSERT_REGEX_TRAITS
//   macro: static_assert that Tr satisfies RegexTraits.
#define D_STATIC_ASSERT_REGEX_TRAITS(Tr)                                  \
    static_assert(                                                         \
        djinterp::re_std::is_regex_traits_v<Tr>,                          \
        "Type " #Tr " does not satisfy RegexTraits")


NS_END  // stl
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP23_OR_HIGHER


#endif  // DJINTERP_META_STL_CPP_NAMED23_HPP
