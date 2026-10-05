/*******************************************************************************
* djinterp [core]                              container_multiplicity_traits.hpp
*
*   SFINAE structural traits for the MULTIPLICITY axis - the per-class
* occurrence
* bound a container imposes (the spec, Multiplicity; vocabulary in
* meta/multiplicity.hpp).  The verdict combines a duplicate-EQUIVALENCE signal
* with a UNIQUENESS signal, falling back to the comparator-less default.
*
*   DETECTION.
*     1. key_type          the equivalence-E tell - a keyed / associative
*                          container carries a duplicate-equivalence. Absent,
*                        the
*                          container is comparator-less: identity default, the
*                          SEQUENCE kind (m = inf, copies by position).
*     2. unique insert     among keyed containers, the single-element insert
*                          returns a pair<iterator,bool> for UNIQUE semantics
*                        (the
*                          bool reports whether it was inserted) and a plain
*                          iterator for MULTISET semantics. Probing `.second`
*                        on
*                          the insert result splits set/map (unique, m = 1)
*                        from
*                          multiset/multimap (m = inf).
*     3. interval bounds static lower_bound / upper_bound mark a
*   closed-interval
*                          carrier, whose values are distinct by construction
*                        -
*                          UNIQUE (m = 1), matching the spec's interval row.
*     4. opt-in bound a static `multiplicity` constant states the numeric m
*                          directly, the authoritative override - the way to
*                          express a BOUNDED multiset (1 < m < inf), which has
*                        no
*                          structural tell.
*
*   The opt-in bound wins where present; otherwise an interval is unique, a
* comparator-less container is a sequence, and a keyed container is unique or
* an
* unbounded multiset by its insert signature.  The axis is orthogonal to the
* other intrinsic axes.
*
*   PORTABILITY:
*   C++11 baseline; `_v` companions degrade with the language as the rest do.
*
*            container_multiplicity_traits.hpp
*
*
* path:      /inc/djinterp/core/container/traits/container_multiplicity_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.30
*                                                            revised: 2026.10.03
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_TRAITS_CONTAINER_MULTIPLICITY_TRAITS_HPP
#define DJINTERP_CONTAINER_TRAITS_CONTAINER_MULTIPLICITY_TRAITS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../../djinterp.hpp"            // clean_t, NS_*, feature macros
#include "../../meta/trait_detect.hpp"  // D_TYPE_TRAIT_* detection macros, D_VOID_T
#include "../../meta/member_types.hpp"  // has_key_type (canonical member-type detection)
#include "../../meta/multiplicity.hpp"   // multiplicity_kind + bounds vocabulary


NS_DJINTERP


// ===========================================================================
// I.   Structural signals
// ===========================================================================

// is_countable_container
//   trait: the container guard for this axis - a value_type (the class type)
// AND a const-callable size().
D_TYPE_TRAIT_TRUE(is_countable_container,
    typename clean_t<Type>::value_type,
    decltype(std::declval<const clean_t<Type>&>().size()))

// has_key_type
//   Owned by meta/member_types.hpp (included above) and re-exported through
// it; the local duplicate was removed to end the ODR conflict.

// has_interval_bounds_signal
//   trait: detects static `lower_bound` AND `upper_bound` - a closed-interval
// carrier (distinct values -> unique multiplicity).
D_TYPE_TRAIT_TRUE(has_interval_bounds_signal,
    decltype(clean_t<Type>::lower_bound),
    decltype(clean_t<Type>::upper_bound))

// has_unique_insert
//   trait: detects that the single-element insert returns a type with a
// `.second` (a pair<iterator,bool>) - the mark of UNIQUE associative
// semantics. A multiset returns a plain iterator (no `.second`); a sequence
// has no value-only insert at all. Both leave this false.
template<typename Type,
         typename = void>
struct has_unique_insert : std::false_type
{};

// has_unique_insert specialization
//   helper: the detected case -- selected when `insert(value_type)` returns
// something with a `.second` member. That pair-shaped return is how a
// uniqueness-enforcing container reports whether the insert actually happened,
// so its presence is the signal that duplicates are rejected.
template<typename Type>
struct has_unique_insert<Type,
    D_VOID_T<decltype(
        std::declval<clean_t<Type>&>().insert(
            std::declval<const typename clean_t<Type>::value_type&>()).second )>>
    : std::true_type
{};

D_TYPE_TRAIT_VALUE_BOOL(has_unique_insert)


NS_INTERNAL

    // multiplicity_member_helper
    //   helper: read the opt-in static `multiplicity` bound, reporting
    // presence separately so an absent member is distinguishable from a
    // declared inf.
    template<typename Type,
             typename = void>
    struct multiplicity_member_helper
    {
        static constexpr bool        present = false;
        static constexpr std::size_t value   = unbounded_multiplicity;
    };

    // multiplicity_member_helper specialization
    //   helper: the detected case -- selected when the type declares the
    // static member `multiplicity`.
    template<typename Type>
    struct multiplicity_member_helper<Type,
        D_VOID_T<decltype(clean_t<Type>::multiplicity)>>
    {
        static constexpr bool        present = true;
        static constexpr std::size_t value   =
            static_cast<std::size_t>(clean_t<Type>::multiplicity);
    };

NS_END  // internal

// has_multiplicity_bound
//   trait: detects the opt-in static `multiplicity` constant.
template<typename Type>
struct has_multiplicity_bound
    : std::integral_constant<bool,
          internal::multiplicity_member_helper<clean_t<Type>>::present>
{};

D_TYPE_TRAIT_VALUE_BOOL(has_multiplicity_bound)


// ===========================================================================
// II.  Verdict
// ===========================================================================

// multiplicity_kind_of
//   trait: the container's multiplicity kind. Precedence: a non-container is
// unknown; an opt-in bound is authoritative; an interval is unique; a
// comparator- less container is a sequence; a keyed container is unique or an
// unbounded multiset by its insert signature.
template<typename Type>
struct multiplicity_kind_of
{
private:
    using clean_type = clean_t<Type>;

public:
    static constexpr multiplicity_kind::value value =
        ( !is_countable_container<clean_type>::value )
              ? multiplicity_kind::unknown
      : (  internal::multiplicity_member_helper<clean_type>::present )
              ? make_multiplicity_kind(
                    true, internal::multiplicity_member_helper<clean_type>::value )
      : (  has_interval_bounds_signal<clean_type>::value )
              ? multiplicity_kind::unique
      : ( !has_key_type<clean_type>::value )
              ? multiplicity_kind::sequence
      : (  has_unique_insert<clean_type>::value )
              ? multiplicity_kind::unique
      :         multiplicity_kind::unbounded_multiset;

    using type = std::integral_constant<multiplicity_kind::value, value>;
};

// multiplicity_kind_of_t / multiplicity_kind_of_v
//   type / value: the carrier and, where the language permits, the value
// companion of multiplicity_kind_of.
template<typename Type>
using multiplicity_kind_of_t = typename multiplicity_kind_of<Type>::type;

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename Type>
    inline constexpr multiplicity_kind::value multiplicity_kind_of_v =
        multiplicity_kind_of<Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr multiplicity_kind::value multiplicity_kind_of_v =
        multiplicity_kind_of<Type>::value;
#endif

// multiplicity_bound_of
//   trait: the numeric bound m. An opt-in member is reported verbatim;
// otherwise it follows from the kind (unique -> 1, sequence /
// unbounded_multiset -> inf).
template<typename Type>
struct multiplicity_bound_of
{
private:
    using clean_type = clean_t<Type>;

public:
    static constexpr std::size_t value =
        ( internal::multiplicity_member_helper<clean_type>::present )
              ? internal::multiplicity_member_helper<clean_type>::value
              : multiplicity_bound_of_kind(
                    multiplicity_kind_of<clean_type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr std::size_t multiplicity_bound_of_v =
        multiplicity_bound_of<Type>::value;
#endif


// ===========================================================================
// III. Classification predicates
// ===========================================================================

// is_sequence_container
//   trait: true for the comparator-less kind (identity default, m = inf).
template<typename Type>
struct is_sequence_container
    : std::integral_constant<bool,
          is_sequence_kind(multiplicity_kind_of<clean_t<Type>>::value)>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_sequence_container)

// is_unique_container
//   trait: true for set semantics (m = 1).
template<typename Type>
struct is_unique_container
    : std::integral_constant<bool,
          is_unique_kind(multiplicity_kind_of<clean_t<Type>>::value)>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_unique_container)

// is_multiset_container
//   trait: true for either multiset kind (a genuine equivalence, m > 1).
template<typename Type>
struct is_multiset_container
    : std::integral_constant<bool,
          is_multiset_kind(multiplicity_kind_of<clean_t<Type>>::value)>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_multiset_container)


// ===========================================================================
// IV.  Aggregate snapshot
// ===========================================================================

// multiplicity_container_class
//   trait: one-shot snapshot of the multiplicity axis for `Type`. Gathers
// every
// signal and verdict this header computes into a single instantiation, so
// a caller that needs several of them pays for detection once rather than per
// query.
template<typename Type>
struct multiplicity_container_class
{
private:
    using clean_type = clean_t<Type>;

public:
    // signals
    static constexpr bool has_equivalence =
        has_key_type<clean_type>::value;
    static constexpr bool unique_insert =
        has_unique_insert<clean_type>::value;
    static constexpr bool interval_domain =
        has_interval_bounds_signal<clean_type>::value;
    static constexpr bool opt_in_bound =
        has_multiplicity_bound<clean_type>::value;

    // verdict
    static constexpr multiplicity_kind::value kind =
        multiplicity_kind_of<clean_type>::value;
    static constexpr std::size_t       bound =
        multiplicity_bound_of<clean_type>::value;
    static constexpr const char*       kind_name =
        multiplicity_kind_name(kind);

    // shorthands
    static constexpr bool is_sequence =
        is_sequence_kind(kind);
    static constexpr bool is_unique =
        is_unique_kind(kind);
    static constexpr bool is_multiset =
        is_multiset_kind(kind);
};


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_TRAITS_CONTAINER_MULTIPLICITY_TRAITS_HPP
