/*******************************************************************************
* djinterp [cpp]                                                 cpp_named17.hpp
*
* C++17 named requirement type traits:
*   Extends the C++14 named requirement traits with:
*   - inline constexpr variable templates (ODR-safe across translation units)
*   - Compound requirement traits using std::conjunction / std::disjunction
*   - Trivially-qualified variants (is_trivially_*)
*
* ADDITIONS OVER C++14:
*   - inline constexpr _v helpers (ODR-safe)
*   - is_trivially_default_constructible<T>
*   - is_trivially_copy_constructible<T>
*   - is_trivially_move_constructible<T>
*   - is_trivially_copy_assignable<T>
*   - is_trivially_move_assignable<T>
*   - is_trivially_destructible<T>
*   - is_fully_constructible<T>     (compound: default + copy + move)
*   - is_fully_assignable<T>        (compound: copy + move)
*   - is_fully_copyable<T>          (compound: copy construct + copy assign)
*   - is_fully_movable<T>           (compound: move construct + move assign)
*   - is_value_type<T>              (compound: all six named requirements)
*
* FEATURE DEPENDENCIES:
*   D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES  - inline constexpr
*   D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES - move trait availability
*
*
* path:      /inc/djinterp/cpp/meta/stl/cpp_named17.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.27
*                                                            revised: 2026.11.04
*******************************************************************************/

#ifndef DJINTERP_META_STL_CPP_NAMED17_HPP
#define DJINTERP_META_STL_CPP_NAMED17_HPP 1

// djinterp
#include "../../djinterp.hpp"  // framework root


// only meaningful in C++ mode
#ifndef __cplusplus
    #error "cpp_named17.hpp can only be used in C++ compilation mode"
#endif

// include C++14 layer
#include "cpp_named14.hpp"

// C++17 additions are active for C++17 and all later standards
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <type_traits>


NS_DJINTERP
D_NAMESPACE(stl)

// =========================================================================
// I.   TRIVIALLY-QUALIFIED VARIANTS (C++17)
// =========================================================================

// is_trivially_default_constructible
//   trait: evaluates to true if T is trivially default-constructible
// (i.e., default construction performs no non-trivial operations).
template<typename T>
struct is_trivially_default_constructible
{
    static constexpr bool value =
        std::is_trivially_default_constructible<T>::value;
};

// is_trivially_copy_constructible
//   trait: evaluates to true if T is trivially copy-constructible.
template<typename T>
struct is_trivially_copy_constructible
{
    static constexpr bool value =
        std::is_trivially_copy_constructible<T>::value;
};

// is_trivially_copy_assignable
//   trait: evaluates to true if T is trivially copy-assignable.
template<typename T>
struct is_trivially_copy_assignable
{
    static constexpr bool value =
        std::is_trivially_copy_assignable<T>::value;
};

// is_trivially_destructible
//   trait: evaluates to true if T is trivially destructible.
template<typename T>
struct is_trivially_destructible
{
    static constexpr bool value =
        std::is_trivially_destructible<T>::value;
};

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

// is_trivially_move_constructible
//   trait: evaluates to true if T is trivially move-constructible.
template<typename T>
struct is_trivially_move_constructible
{
    static constexpr bool value =
        std::is_trivially_move_constructible<T>::value;
};

// is_trivially_move_assignable
//   trait: evaluates to true if T is trivially move-assignable.
template<typename T>
struct is_trivially_move_assignable
{
    static constexpr bool value =
        std::is_trivially_move_assignable<T>::value;
};

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES


// =========================================================================
// II.  COMPOUND REQUIREMENT TRAITS (C++17)
// =========================================================================

// is_fully_copyable
//   trait: evaluates to true if T satisfies both CopyConstructible and
// CopyAssignable.
template<typename T>
struct is_fully_copyable
{
    static constexpr bool value =
        ( std::is_copy_constructible<T>::value &&
          std::is_copy_assignable<T>::value );
};

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

// is_fully_movable
//   trait: evaluates to true if T satisfies both MoveConstructible and
// MoveAssignable.
template<typename T>
struct is_fully_movable
{
    static constexpr bool value =
        ( std::is_move_constructible<T>::value &&
          std::is_move_assignable<T>::value );
};

// is_fully_constructible
//   trait: evaluates to true if T satisfies DefaultConstructible,
// CopyConstructible, and MoveConstructible.
template<typename T>
struct is_fully_constructible
{
    static constexpr bool value =
        ( std::is_default_constructible<T>::value &&
          std::is_copy_constructible<T>::value    &&
          std::is_move_constructible<T>::value );
};

// is_fully_assignable
//   trait: evaluates to true if T satisfies both CopyAssignable and
// MoveAssignable.
template<typename T>
struct is_fully_assignable
{
    static constexpr bool value =
        ( std::is_copy_assignable<T>::value &&
          std::is_move_assignable<T>::value );
};

// is_value_type
//   trait: evaluates to true if T satisfies all six core named
// requirements: DefaultConstructible, MoveConstructible,
// CopyConstructible, MoveAssignable, CopyAssignable, and Destructible.
template<typename T>
struct is_value_type
{
    static constexpr bool value =
        ( std::is_default_constructible<T>::value &&
          std::is_copy_constructible<T>::value    &&
          std::is_move_constructible<T>::value    &&
          std::is_copy_assignable<T>::value        &&
          std::is_move_assignable<T>::value        &&
          std::is_destructible<T>::value );
};

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES


// =========================================================================
// III. INLINE CONSTEXPR VARIABLE TEMPLATES (C++17)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES

// --- trivially-qualified _v helpers ---

// is_trivially_default_constructible_v
//   value: shorthand for is_trivially_default_constructible<T>::value.
template<typename T>
inline constexpr bool is_trivially_default_constructible_v =
    is_trivially_default_constructible<T>::value;

// is_trivially_copy_constructible_v
//   value: shorthand for is_trivially_copy_constructible<T>::value.
template<typename T>
inline constexpr bool is_trivially_copy_constructible_v =
    is_trivially_copy_constructible<T>::value;

// is_trivially_copy_assignable_v
//   value: shorthand for is_trivially_copy_assignable<T>::value.
template<typename T>
inline constexpr bool is_trivially_copy_assignable_v =
    is_trivially_copy_assignable<T>::value;

// is_trivially_destructible_v
//   value: shorthand for is_trivially_destructible<T>::value.
template<typename T>
inline constexpr bool is_trivially_destructible_v =
    is_trivially_destructible<T>::value;

// --- compound _v helpers ---

// is_fully_copyable_v
//   value: shorthand for is_fully_copyable<T>::value.
template<typename T>
inline constexpr bool is_fully_copyable_v =
    is_fully_copyable<T>::value;

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

// is_trivially_move_constructible_v
//   value: shorthand for is_trivially_move_constructible<T>::value.
template<typename T>
inline constexpr bool is_trivially_move_constructible_v =
    is_trivially_move_constructible<T>::value;

// is_trivially_move_assignable_v
//   value: shorthand for is_trivially_move_assignable<T>::value.
template<typename T>
inline constexpr bool is_trivially_move_assignable_v =
    is_trivially_move_assignable<T>::value;

// is_fully_movable_v
//   value: shorthand for is_fully_movable<T>::value.
template<typename T>
inline constexpr bool is_fully_movable_v =
    is_fully_movable<T>::value;

// is_fully_constructible_v
//   value: shorthand for is_fully_constructible<T>::value.
template<typename T>
inline constexpr bool is_fully_constructible_v =
    is_fully_constructible<T>::value;

// is_fully_assignable_v
//   value: shorthand for is_fully_assignable<T>::value.
template<typename T>
inline constexpr bool is_fully_assignable_v =
    is_fully_assignable<T>::value;

// is_value_type_v
//   value: shorthand for is_value_type<T>::value.
template<typename T>
inline constexpr bool is_value_type_v =
    is_value_type<T>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES


// --- type classification inline constexpr _v helpers ---


// --- comparison, callable, and predicate inline constexpr _v helpers ---

#endif  // D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES


// =========================================================================
// IV.  STD-BACKED SWAPPABLE TRAITS (C++17)
// =========================================================================

// is_nothrow_swappable
//   trait: evaluates to true if T is swappable without throwing
// exceptions.
template<typename T>
struct is_nothrow_swappable
{
    static constexpr bool value =
        std::is_nothrow_swappable<T>::value;
};

// is_swappable_with
//   trait: evaluates to true if T and U are mutually swappable.
template<typename T,
         typename U>
struct is_swappable_with
{
    static constexpr bool value =
        std::is_swappable_with<T, U>::value;
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES

// is_nothrow_swappable_v (inline constexpr)
//   value: ODR-safe shorthand for is_nothrow_swappable<T>::value.
template<typename T>
inline constexpr bool is_nothrow_swappable_v =
    is_nothrow_swappable<T>::value;

// is_swappable_with_v (inline constexpr)
//   value: ODR-safe shorthand for is_swappable_with<T, U>::value.
template<typename T,
         typename U>
inline constexpr bool is_swappable_with_v =
    is_swappable_with<T, U>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES


// =========================================================================
// V.   STD-BACKED INVOCABLE / CALLABLE TRAITS (C++17)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES

// --- container inline constexpr _v helpers ---

#endif  // D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES


// =========================================================================
// VI.  CONTIGUOUS CONTAINER TRAIT (C++17)
// =========================================================================

NS_INTERNAL

    // has_contiguous_ops
    //   trait: detects ContiguousContainer member: data() returning a
    // pointer to value_type.
    template<typename C,
             typename = void>
    struct has_contiguous_ops
    {
        static constexpr bool value = false;
    };

    template<typename C>
    struct has_contiguous_ops<C,
        decltype(static_cast<void>(
            static_cast<const typename C::value_type*>(
                std::declval<const C&>().data())
        ))>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// is_contiguous_container
//   trait: evaluates to true if C satisfies the structural
// requirements of the ContiguousContainer named requirement: is a
// Container with a data() member returning a pointer to contiguous
// storage.
// note: introduced in C++17. Does not verify contiguous_iterator
// concept on iterators (that requires C++20 concepts).
template<typename C>
struct is_contiguous_container
{
    static constexpr bool value =
        ( is_container<C>::value &&
          internal::has_contiguous_ops<C>::value );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES

// is_contiguous_container_v (inline constexpr)
//   value: ODR-safe shorthand for is_contiguous_container<C>::value.
template<typename C>
inline constexpr bool is_contiguous_container_v =
    is_contiguous_container<C>::value;

// --- container element inline constexpr _v helpers ---

// --- iterator inline constexpr _v helpers ---

#endif  // D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES


// =========================================================================
// VII. LEGACY CONTIGUOUS ITERATOR TRAIT (C++17)
// =========================================================================

NS_INTERNAL

    // is_contiguous_iterator_tag
    //   trait: detects if iterator_traits<I>::iterator_category is
    // or derives from contiguous_iterator_tag. Since
    // std::contiguous_iterator_tag is formally C++20, this SFINAE
    // check gracefully returns false on implementations that do not
    // provide it.
    template<typename I,
             typename = void>
    struct is_contiguous_iterator_tag
    {
        static constexpr bool value = false;
    };

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    template<typename I>
    struct is_contiguous_iterator_tag<I,
        typename std::enable_if<
            std::is_base_of<
                std::contiguous_iterator_tag,
                typename std::iterator_traits<I>::iterator_category
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };
#endif

NS_END  // internal

// is_legacy_contiguous_iterator
//   trait: evaluates to true if I satisfies the
// LegacyContiguousIterator named requirement:
// LegacyRandomAccessIterator whose elements are contiguous in memory.
// note: pointer types are always contiguous. For class-type iterators,
// detection relies on contiguous_iterator_tag (C++20). See
// cpp_named20.hpp for std::contiguous_iterator-backed version.
template<typename I>
struct is_legacy_contiguous_iterator
{
    static constexpr bool value =
        ( is_legacy_random_access_iterator<I>::value &&
          ( std::is_pointer<I>::value ||
            internal::is_contiguous_iterator_tag<I>::value ) );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES

// is_legacy_contiguous_iterator_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_legacy_contiguous_iterator<I>::value.
template<typename I>
inline constexpr bool is_legacy_contiguous_iterator_v =
    is_legacy_contiguous_iterator<I>::value;

// --- stream I/O inline constexpr _v helpers ---

// --- random number generation inline constexpr _v helpers ---

// --- concurrency inline constexpr _v helpers ---

// --- type trait meta-trait inline constexpr _v helpers ---

// --- clock, chartraits, misc inline constexpr _v helpers ---

#endif  // D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES


NS_END  // stl
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


#endif  // DJINTERP_META_STL_CPP_NAMED17_HPP
