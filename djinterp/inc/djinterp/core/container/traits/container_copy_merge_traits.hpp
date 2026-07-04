/******************************************************************************
* djinterp [container]                          container_copy_merge_traits.hpp
*
*   COPY and MERGE - two of the operations FOUNDED on comparison (the spec,
* Operations founded on comparison), the siblings of the conversion already in
* container_conversion_traits.hpp.  Where conversion carries one container type
* to another and is classified by the content level it preserves, copy and merge
* are the identity-preserving and the combining operations, and this header says
* how each stands at the type level.
*
*   COPY.  A copy of c is an independent container EQUAL to c in content, in
* discipline, and in realization, differing from it only in IDENTITY - a fresh
* object holding the same value.  Copy therefore preserves every axis of the
* anatomy (the one thing it does not preserve is identity), a guarantee the spec
* states and this header records as the note trait copy_preserves_all_axes.  At
* the type level the capability is copy-constructibility of a container type;
* is_copyable_container asks exactly that.
*
*   MERGE.  A merge combines two containers AT THEIR SHARED DISCIPLINE: two
* sequences CONCATENATE, two sets/bags take a UNION, two maps take a KEYED
* MERGE, and disciplines that do not agree do not merge.  merge_kind_of reads the
* two operands' disciplines and names the combining operation, with three
* provisos from the spec attached:
*     - ELEMENTS must be compatible (same or convertible) or there is no merge;
*     - a KEYED target may raise a key CONFLICT (two entries at one key where the
*       result admits each key once) - merge_has_key_conflict;
*     - a bounded result may OVERFLOW its capacity, since a merge only grows -
*       merge_may_overflow (the capacity proviso).
* The shared-discipline result type is taken as the left operand, the common
* convention where the two disciplines agree (merge_result_type).
*
*   DISCIPLINE, LOCALLY.  Classifying an operand as sequence / set / multiset /
* map / multimap is the overlay reading of container_overlay_traits.hpp.  To keep
* this module self-contained - free of the container_traits.hpp include graph the
* dedicated axis headers sit on - the few signals merge needs are detected here
* with their own structural probes, mirroring exactly the tells those axes use: a
* nested key_type is the duplicate-EQUIVALENCE mark (set / map / ... versus a
* plain sequence), a value_type that is a pair is the KEYED mark (map / multimap),
* and a single-element insert returning a `.second` is the UNIQUE mark (set / map
* versus multiset / multimap) - the same insert-signature split the multiplicity
* axis draws.  Boundedness of the result mirrors the boundedness axis in the same
* way (a static extent, a tuple_size, static interval bounds, or a fixed
* capacity() without reserve()).  This is the local-signal idiom the overlay
* header itself uses for its keyed / sorted hooks; the verdicts agree with the
* axis traits on the familiar containers.
*
*   PORTABILITY:
*   C++11 baseline; `_v` companions degrade with the language, as elsewhere.
*
*   CONTENTS:
*     I.   Operand prerequisites          container-hood + element compatibility
*     II.  Copy                           is_copyable_container, preservation note
*     III. Discipline signals             the local overlay reading (per operand)
*     IV.  Merge kind                     the combining operation of a pair
*     V.   Merge provisos                 result type, key conflict, overflow
*     VI.  Aggregate snapshot
*
*
* path:      /inc/djinterp/core/container/traits/container_copy_merge_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.03
******************************************************************************/

#ifndef DJINTERP_CONTAINER_COPY_MERGE_TRAITS_
#define DJINTERP_CONTAINER_COPY_MERGE_TRAITS_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"                       // clean_t, NS_*, feature macros
#include "../../meta/trait_detect.hpp"              // D_VOID_T, D_TYPE_TRAIT_* macros
#include "./element_relation_traits.hpp"            // elements_same / elements_convertible
#include "./iterable_container_traits.hpp"          // is_iterable_container (container guard)


NS_DJINTERP


// ===========================================================================
// I.   Operand prerequisites
// ===========================================================================
//   Both operations rest on the operand being a container, and merge on the two
// operands' elements being combinable.  Container-hood is is_iterable_container
// (a container one can read); element compatibility is same-type or convertible
// in either direction - enough that the survivors can inhabit one result.

// merge_elements_compatible
//   trait: the two operands are containers whose elements are the same, or are
// convertible one to the other - the element prerequisite of a merge.
template<typename _Left,
         typename _Right>
struct merge_elements_compatible
    : std::integral_constant<bool,
            is_iterable_container<clean_t<_Left>>::value
         && is_iterable_container<clean_t<_Right>>::value
         && (    elements_same_type<_Left, _Right>::value
              || elements_convertible<_Left, _Right>::value
              || elements_convertible<_Right, _Left>::value )>
{};

// merge_elements_compatible_v  (two-param, so the `_v` is written by hand)
#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Left,
             typename _Right>
    inline constexpr bool merge_elements_compatible_v =
        merge_elements_compatible<_Left, _Right>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Left,
             typename _Right>
    constexpr bool merge_elements_compatible_v =
        merge_elements_compatible<_Left, _Right>::value;
#endif


// ===========================================================================
// II.  Copy
// ===========================================================================
//   A copy yields an independent container equal in content, discipline, and
// realization, differing only in identity.  The capability is container copy-
// construction; the preservation of every axis is a fact the spec supplies.

// is_copyable_container
//   trait: the type is a container that can be copy-constructed - a copy of a
// value is producible.
template<typename _Type>
struct is_copyable_container
    : std::integral_constant<bool,
            is_iterable_container<clean_t<_Type>>::value
         && std::is_copy_constructible<clean_t<_Type>>::value>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_copyable_container)

// copy_preserves_all_axes
//   trait: the note (always true) that a copy preserves every axis of the
// anatomy - content, discipline, and realization alike - altering only identity.
// It is the type-level record of the spec's guarantee, not a detected property.
template<typename _Type>
struct copy_preserves_all_axes : std::true_type
{};

D_TYPE_TRAIT_VALUE_BOOL(copy_preserves_all_axes)


// ===========================================================================
// III. Discipline signals  (the local overlay reading)
// ===========================================================================

NS_INTERNAL

    // has_equivalence_helper
    //   helper: a nested key_type - the duplicate-EQUIVALENCE mark distinguishing
    // an associative discipline (set / multiset / map / multimap) from a plain
    // comparator-less sequence.
    D_TYPE_TRAIT_TRUE(has_equivalence_helper,
        typename clean_t<_Type>::key_type)

    // has_keyed_value_helper
    //   helper: the value_type is a pair (first_type / second_type) - the KEYED
    // mark (map / multimap), mirroring is_keyed_container of the overlay axis.
    D_TYPE_TRAIT_TRUE(has_keyed_value_helper,
        typename clean_t<_Type>::value_type::first_type,
        typename clean_t<_Type>::value_type::second_type)

    // has_unique_insert_helper
    //   helper: a single-element insert whose result carries a `.second` (a
    // pair<iterator,bool>) - the UNIQUE mark (set / map) as against the plain
    // iterator a multiset / multimap returns, the multiplicity axis's own split.
    template<typename _Type,
             typename = void>
    struct has_unique_insert_helper : std::false_type
    {};

    template<typename _Type>
    struct has_unique_insert_helper<_Type,
        D_VOID_T<decltype(
            std::declval<clean_t<_Type>&>().insert(
                std::declval<const typename clean_t<_Type>::value_type&>()).second)>>
        : std::true_type
    {};

    // --- boundedness signals (mirroring bounded_container_traits) ---

    // has_extent_helper
    //   helper: a static `extent` constant - the fixed-capacity convention.
    D_TYPE_TRAIT_TRUE(has_extent_helper,
        decltype(clean_t<_Type>::extent))

    // has_tuple_size_helper
    //   helper: a std::tuple_size specialization - a fixed-size aggregate.
    D_TYPE_TRAIT_TRUE(has_tuple_size_helper,
        decltype(std::tuple_size<clean_t<_Type>>::value))

    // has_static_bounds_helper
    //   helper: static lower_bound / upper_bound - a finite closed-interval domain.
    D_TYPE_TRAIT_TRUE(has_static_bounds_helper,
        decltype(clean_t<_Type>::lower_bound),
        decltype(clean_t<_Type>::upper_bound))

    // has_capacity_helper
    //   helper: a const-callable capacity() accessor.
    D_TYPE_TRAIT_TRUE(has_capacity_helper,
        decltype(std::declval<const clean_t<_Type>&>().capacity()))

    // has_reserve_helper
    //   helper: a reserve(size) call - the growability anti-signal that keeps a
    // capacity() from meaning a FIXED bound.
    D_TYPE_TRAIT_TRUE(has_reserve_helper,
        decltype(std::declval<clean_t<_Type>&>().reserve(std::declval<std::size_t>())))

NS_END  // internal

// merge_discipline
//   enum: the combining-relevant discipline of a single operand - the overlay it
// wears, read locally.  A non-container is `unknown`; a comparator-less container
// is a `sequence`; the associative disciplines split by keyed? and unique?.
enum class merge_discipline
{
    unknown,    // not a container
    sequence,   // comparator-less (concatenates)
    set,        // unkeyed, unique       (set union)
    multiset,   // unkeyed, repeatable   (multiset union)
    map,        // keyed, unique keys    (keyed merge)
    multimap    // keyed, repeatable keys(keyed merge)
};

// merge_discipline_name
//   function: a stable spelling, for diagnostics and agent-facing summaries.
constexpr const char*
merge_discipline_name(merge_discipline _d) noexcept
{
    return ( _d == merge_discipline::unknown  ? "unknown"
           : _d == merge_discipline::sequence ? "sequence"
           : _d == merge_discipline::set      ? "set"
           : _d == merge_discipline::multiset ? "multiset"
           : _d == merge_discipline::map      ? "map"
           :                                    "multimap" );
}

// discipline_is_sequence / discipline_is_bag / discipline_is_keyed
//   function: the three merge families - a comparator-less sequence, an unkeyed
// bag (set or multiset), and a keyed map (map or multimap).
constexpr bool
discipline_is_sequence(merge_discipline _d) noexcept
{
    return ( _d == merge_discipline::sequence );
}

constexpr bool
discipline_is_bag(merge_discipline _d) noexcept
{
    return (    _d == merge_discipline::set
             || _d == merge_discipline::multiset );
}

constexpr bool
discipline_is_keyed(merge_discipline _d) noexcept
{
    return (    _d == merge_discipline::map
             || _d == merge_discipline::multimap );
}

// merge_discipline_of
//   trait: the discipline a single operand wears, from the local signals.  Not a
// container -> unknown; a pair value_type -> map / multimap by uniqueness; else
// an equivalence (key_type) -> set / multiset by uniqueness; else a sequence.
template<typename _Type>
struct merge_discipline_of
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr merge_discipline value =
        ( !is_iterable_container<clean_type>::value )
              ? merge_discipline::unknown
      : (  internal::has_keyed_value_helper<clean_type>::value )
              ? (  internal::has_unique_insert_helper<clean_type>::value
                       ? merge_discipline::map
                       : merge_discipline::multimap )
      : (  internal::has_equivalence_helper<clean_type>::value )
              ? (  internal::has_unique_insert_helper<clean_type>::value
                       ? merge_discipline::set
                       : merge_discipline::multiset )
      :         merge_discipline::sequence;

    using type = std::integral_constant<merge_discipline, value>;
};

template<typename _Type>
using merge_discipline_of_t = typename merge_discipline_of<_Type>::type;

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr merge_discipline merge_discipline_of_v =
        merge_discipline_of<_Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr merge_discipline merge_discipline_of_v =
        merge_discipline_of<_Type>::value;
#endif


// ===========================================================================
// IV.  Merge kind
// ===========================================================================

// merge_kind
//   enum: the combining operation of a pair, chosen at the shared discipline.
// `incompatible` covers both a discipline mismatch and incompatible elements.
enum class merge_kind
{
    concatenation,   // two sequences, appended
    set_union,       // two sets, deduplicating union
    multiset_union,  // an unkeyed union that keeps repeats (a multiset present)
    keyed_merge,     // two maps, merged by key
    incompatible     // disciplines disagree, or elements do not combine
};

// merge_kind_name
//   function: a stable spelling, for diagnostics and agent-facing summaries.
constexpr const char*
merge_kind_name(merge_kind _k) noexcept
{
    return ( _k == merge_kind::concatenation  ? "concatenation"
           : _k == merge_kind::set_union      ? "set_union"
           : _k == merge_kind::multiset_union ? "multiset_union"
           : _k == merge_kind::keyed_merge    ? "keyed_merge"
           :                                    "incompatible" );
}

// merge_kind_from_disciplines
//   function: the combining operation of two disciplines.  Two sequences
// concatenate; two unkeyed bags union (a set_union only when BOTH are unique,
// else a multiset_union, since the union may repeat); two keyed maps merge by
// key; anything across the three families is incompatible.
constexpr merge_kind
merge_kind_from_disciplines(
    merge_discipline _a,
    merge_discipline _b
) noexcept
{
    return ( discipline_is_sequence(_a) && discipline_is_sequence(_b) )
               ? merge_kind::concatenation
         : ( discipline_is_bag(_a) && discipline_is_bag(_b) )
               ? (    ( _a == merge_discipline::set )
                   && ( _b == merge_discipline::set )
                          ? merge_kind::set_union
                          : merge_kind::multiset_union )
         : ( discipline_is_keyed(_a) && discipline_is_keyed(_b) )
               ? merge_kind::keyed_merge
         :         merge_kind::incompatible;
}

// merge_kind_of
//   trait: the combining operation of a Left / Right pair.  Incompatible elements
// admit no merge; otherwise the two disciplines decide it.
template<typename _Left,
         typename _Right>
struct merge_kind_of
{
private:
    using left_type  = clean_t<_Left>;
    using right_type = clean_t<_Right>;

public:
    static constexpr merge_kind value =
        ( !merge_elements_compatible<left_type, right_type>::value )
              ? merge_kind::incompatible
      :         merge_kind_from_disciplines(
                    merge_discipline_of<left_type>::value,
                    merge_discipline_of<right_type>::value );

    using type = std::integral_constant<merge_kind, value>;
};

template<typename _Left,
         typename _Right>
using merge_kind_of_t = typename merge_kind_of<_Left, _Right>::type;

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Left,
             typename _Right>
    inline constexpr merge_kind merge_kind_of_v =
        merge_kind_of<_Left, _Right>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Left,
             typename _Right>
    constexpr merge_kind merge_kind_of_v =
        merge_kind_of<_Left, _Right>::value;
#endif

// is_mergeable
//   trait: a merge of the pair is defined (the kind is not incompatible).
template<typename _Left,
         typename _Right>
struct is_mergeable
    : std::integral_constant<bool,
          merge_kind_of<_Left, _Right>::value != merge_kind::incompatible>
{};

// is_mergeable_v  (two-param, so the `_v` is written by hand)
#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Left,
             typename _Right>
    inline constexpr bool is_mergeable_v =
        is_mergeable<_Left, _Right>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Left,
             typename _Right>
    constexpr bool is_mergeable_v =
        is_mergeable<_Left, _Right>::value;
#endif


// ===========================================================================
// V.   Merge provisos
// ===========================================================================

// merge_result_type
//   trait: the shared-discipline result type of a merge.  Where the disciplines
// agree the result wears that discipline, taken here as the LEFT operand (the
// common convention); the alias is meaningful only when the merge is defined.
template<typename _Left,
         typename _Right>
struct merge_result_type
{
    using type = clean_t<_Left>;
};

template<typename _Left,
         typename _Right>
using merge_result_type_t = typename merge_result_type<_Left, _Right>::type;

// merge_has_key_conflict
//   trait: a keyed merge whose result admits each key at most once (a map on
// either side) may raise a key CONFLICT - two entries at one key.  A merge of
// multimaps, whose keys may repeat, raises none.
template<typename _Left,
         typename _Right>
struct merge_has_key_conflict
    : std::integral_constant<bool,
            ( merge_kind_of<_Left, _Right>::value == merge_kind::keyed_merge )
         && (    merge_discipline_of<clean_t<_Left>>::value
                     == merge_discipline::map
              || merge_discipline_of<clean_t<_Right>>::value
                     == merge_discipline::map )>
{};

// merge_has_key_conflict_v  (two-param, so the `_v` is written by hand)
#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Left,
             typename _Right>
    inline constexpr bool merge_has_key_conflict_v =
        merge_has_key_conflict<_Left, _Right>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Left,
             typename _Right>
    constexpr bool merge_has_key_conflict_v =
        merge_has_key_conflict<_Left, _Right>::value;
#endif

// merge_may_overflow
//   trait: the capacity proviso - a defined merge whose RESULT type is capacity-
// bounded may overflow, since a merge only grows the container.  The result type
// (the left operand) decides it, mirroring bounded_container_traits.
template<typename _Left,
         typename _Right>
struct merge_may_overflow
{
private:
    using result_type = merge_result_type_t<_Left, _Right>;

public:
    static constexpr bool value =
        (    is_mergeable<_Left, _Right>::value
          && (    internal::has_extent_helper<result_type>::value
               || internal::has_tuple_size_helper<result_type>::value
               || internal::has_static_bounds_helper<result_type>::value
               || (    internal::has_capacity_helper<result_type>::value
                    && !internal::has_reserve_helper<result_type>::value ) ) );
};

// merge_may_overflow_v  (two-param, so the `_v` is written by hand)
#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Left,
             typename _Right>
    inline constexpr bool merge_may_overflow_v =
        merge_may_overflow<_Left, _Right>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Left,
             typename _Right>
    constexpr bool merge_may_overflow_v =
        merge_may_overflow<_Left, _Right>::value;
#endif


// ===========================================================================
// VI.  Aggregate snapshot
// ===========================================================================

// merge_class
//   trait: the assembled reading of a Left / Right merge - the two disciplines,
// the combining kind, whether the elements combine, whether a key conflict is
// possible, and whether the bounded result may overflow.
template<typename _Left,
         typename _Right>
struct merge_class
{
private:
    using left_type  = clean_t<_Left>;
    using right_type = clean_t<_Right>;

public:
    static constexpr merge_discipline left_discipline =
        merge_discipline_of<left_type>::value;
    static constexpr merge_discipline right_discipline =
        merge_discipline_of<right_type>::value;
    static constexpr merge_kind kind =
        merge_kind_of<left_type, right_type>::value;
    static constexpr const char* kind_name =
        merge_kind_name(kind);
    static constexpr bool elements_ok =
        merge_elements_compatible<left_type, right_type>::value;
    static constexpr bool mergeable =
        is_mergeable<left_type, right_type>::value;
    static constexpr bool key_conflict =
        merge_has_key_conflict<left_type, right_type>::value;
    static constexpr bool may_overflow =
        merge_may_overflow<left_type, right_type>::value;
};


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_COPY_MERGE_TRAITS_
