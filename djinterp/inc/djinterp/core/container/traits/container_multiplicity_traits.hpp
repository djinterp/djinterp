/******************************************************************************
* djinterp [container]                        container_multiplicity_traits.hpp
*
*   SFINAE structural traits for the MULTIPLICITY axis - the per-class occurrence
* bound a container imposes (the spec, Multiplicity; vocabulary in
* meta/multiplicity.hpp).  The verdict combines a duplicate-EQUIVALENCE signal
* with a UNIQUENESS signal, falling back to the comparator-less default.
*
*   DETECTION.
*     1. key_type          the equivalence-E tell - a keyed / associative
*                          container carries a duplicate-equivalence.  Absent, the
*                          container is comparator-less: identity default, the
*                          SEQUENCE kind (m = inf, copies by position).
*     2. unique insert     among keyed containers, the single-element insert
*                          returns a pair<iterator,bool> for UNIQUE semantics (the
*                          bool reports whether it was inserted) and a plain
*                          iterator for MULTISET semantics.  Probing `.second` on
*                          the insert result splits set/map (unique, m = 1) from
*                          multiset/multimap (m = inf).
*     3. interval bounds   static lower_bound / upper_bound mark a closed-interval
*                          carrier, whose values are distinct by construction -
*                          UNIQUE (m = 1), matching the spec's interval row.
*     4. opt-in bound      a static `multiplicity` constant states the numeric m
*                          directly, the authoritative override - the way to
*                          express a BOUNDED multiset (1 < m < inf), which has no
*                          structural tell.
*
*   The opt-in bound wins where present; otherwise an interval is unique, a
* comparator-less container is a sequence, and a keyed container is unique or an
* unbounded multiset by its insert signature.  The axis is orthogonal to the
* other intrinsic axes.
*
*   PORTABILITY:
*   C++11 baseline; `_v` companions degrade with the language as the rest do.
*
*
* path:      /inc/djinterp/core/container/traits/container_multiplicity_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.30
******************************************************************************/

#ifndef DJINTERP_CONTAINER_MULTIPLICITY_TRAITS_
#define DJINTERP_CONTAINER_MULTIPLICITY_TRAITS_ 1

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"           // clean_t, NS_*, feature macros
#include "../../meta/trait_detect.hpp"  // D_TYPE_TRAIT_* detection macros, D_VOID_T
#include "../../meta/multiplicity.hpp"  // multiplicity_kind + bounds vocabulary
#include "./container_traits.hpp"       // multiplicity_kind + bounds vocabulary


NS_DJINTERP


// ===========================================================================
// I.   Structural signals
// ===========================================================================

// is_countable_container
//   trait: the container guard for this axis - a value_type (the class type) AND
// a const-callable size().
D_TYPE_TRAIT_TRUE(is_countable_container,
    typename clean_t<_Type>::value_type,
    decltype(std::declval<const clean_t<_Type>&>().size()))

// has_interval_bounds_signal
//   trait: detects static `lower_bound` AND `upper_bound` - a closed-interval
// carrier (distinct values -> unique multiplicity).
D_TYPE_TRAIT_TRUE(has_interval_bounds_signal,
    decltype(clean_t<_Type>::lower_bound),
    decltype(clean_t<_Type>::upper_bound))

// has_unique_insert
//   trait: detects that the single-element insert returns a type with a `.second`
// (a pair<iterator,bool>) - the mark of UNIQUE associative semantics.  A multiset
// returns a plain iterator (no `.second`); a sequence has no value-only insert at
// all.  Both leave this false.
template<typename _Type,
         typename = void>
struct has_unique_insert : std::false_type
{};

template<typename _Type>
struct has_unique_insert<_Type,
    D_VOID_T<decltype(
        std::declval<clean_t<_Type>&>().insert(
            std::declval<const typename clean_t<_Type>::value_type&>()).second )>>
    : std::true_type
{};

D_TYPE_TRAIT_VALUE_BOOL(has_unique_insert)


NS_INTERNAL

    // multiplicity_member_helper
    //   helper: read the opt-in static `multiplicity` bound, reporting presence
    // separately so an absent member is distinguishable from a declared inf.
    template<typename _Type,
             typename = void>
    struct multiplicity_member_helper
    {
        static constexpr bool        present = false;
        static constexpr std::size_t value   = unbounded_multiplicity;
    };

    template<typename _Type>
    struct multiplicity_member_helper<_Type,
        D_VOID_T<decltype(clean_t<_Type>::multiplicity)>>
    {
        static constexpr bool        present = true;
        static constexpr std::size_t value   =
            static_cast<std::size_t>(clean_t<_Type>::multiplicity);
    };

NS_END  // internal

// has_multiplicity_bound
//   trait: detects the opt-in static `multiplicity` constant.
template<typename _Type>
struct has_multiplicity_bound
    : std::integral_constant<bool,
          internal::multiplicity_member_helper<clean_t<_Type>>::present>
{};

D_TYPE_TRAIT_VALUE_BOOL(has_multiplicity_bound)


// ===========================================================================
// II.  Verdict
// ===========================================================================

// multiplicity_kind_of
//   trait: the container's multiplicity kind.  Precedence: a non-container is
// unknown; an opt-in bound is authoritative; an interval is unique; a comparator-
// less container is a sequence; a keyed container is unique or an unbounded
// multiset by its insert signature.
template<typename _Type>
struct multiplicity_kind_of
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr multiplicity_kind value =
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

    using type = std::integral_constant<multiplicity_kind, value>;
};

template<typename _Type>
using multiplicity_kind_of_t = typename multiplicity_kind_of<_Type>::type;

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr multiplicity_kind multiplicity_kind_of_v =
        multiplicity_kind_of<_Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr multiplicity_kind multiplicity_kind_of_v =
        multiplicity_kind_of<_Type>::value;
#endif

// multiplicity_bound_of
//   trait: the numeric bound m.  An opt-in member is reported verbatim; otherwise
// it follows from the kind (unique -> 1, sequence / unbounded_multiset -> inf).
template<typename _Type>
struct multiplicity_bound_of
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr std::size_t value =
        ( internal::multiplicity_member_helper<clean_type>::present )
              ? internal::multiplicity_member_helper<clean_type>::value
              : multiplicity_bound_of_kind(
                    multiplicity_kind_of<clean_type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr std::size_t multiplicity_bound_of_v =
        multiplicity_bound_of<_Type>::value;
#endif


// ===========================================================================
// III. Classification predicates
// ===========================================================================

// is_sequence_container
//   trait: true for the comparator-less kind (identity default, m = inf).
template<typename _Type>
struct is_sequence_container
    : std::integral_constant<bool,
          is_sequence_kind(multiplicity_kind_of<clean_t<_Type>>::value)>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_sequence_container)

// is_unique_container
//   trait: true for set semantics (m = 1).
template<typename _Type>
struct is_unique_container
    : std::integral_constant<bool,
          is_unique_kind(multiplicity_kind_of<clean_t<_Type>>::value)>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_unique_container)

// is_multiset_container
//   trait: true for either multiset kind (a genuine equivalence, m > 1).
template<typename _Type>
struct is_multiset_container
    : std::integral_constant<bool,
          is_multiset_kind(multiplicity_kind_of<clean_t<_Type>>::value)>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_multiset_container)


// ===========================================================================
// IV.  Aggregate snapshot
// ===========================================================================

template<typename _Type>
struct multiplicity_container_class
{
private:
    using clean_type = clean_t<_Type>;

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
    static constexpr multiplicity_kind kind =
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


#endif  // DJINTERP_CONTAINER_MULTIPLICITY_TRAITS_
