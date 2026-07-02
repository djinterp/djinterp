/******************************************************************************
* djinterp [container]                             container_comparison_traits.hpp
*
*   The comparison of two container TYPES: where they stand, axis by axis, on the
* content, discipline, and realization scales - their comparison profile.  A
* profile is deliberately a vector, not a scalar: two containers a permutation
* apart and two an allocator apart are incomparably different, not more-or-less
* different, and this module keeps them on separate axes.
*
*   Three groups of axis, following the formal model:
*
*     CONTENT   an equivalence hierarchy - structural, sequential, multiset, set,
*               nested finest-to-coarsest.  A type carries a NATIVE level lambda
*               (a set compares at =set, a sequence at =seq, a nested container at
*               =str); two types compare at the COARSER of their natives.  The
*               native level is a type-level fact and is what this module reports.
*               Deciding the actual =L between two container VALUES is a value-
*               level operation (it reads contents) and is left to a companion.
*
*     DISCIPLINE the overlay strength order - one discipline may be at least as
*               restrictive as another (guaranteeing at least as much), refine it
*               either way, or be incomparable.  Read straight off overlay_subsumes.
*
*     REALIZATION three content-blind preorders.  LIFETIME (a two-point total
*               order, compile-time the more static end).  ITERABILITY (the five-
*               element capability lattice: not-iterable at the bottom, then the
*               stage x mode kinds, which reduce to a product of compile-reachable
*               and non-const-reachable).  STORAGE (an equivalence: same siting).
*
*   The per-axis relations combine into one PRODUCT refinement: a type refines
* another only if it stands at least as high on every type-decidable axis at once
* - stronger discipline, and every realization at least as capable.  Mutual
* refinement is agreement; a mixed verdict is incomparability.  The landmarks of
* the introduction sit on this order: EQUAL is mutual refinement on discipline and
* realization (value and realization agree, up to content and identity), and
* EQUIVALENT-ON-S is mutual refinement on the axes of S.
*
*   DIRECTION.  A relation is read of the LEFT operand against the right:
* `greater` is Left the stronger / finer / more-capable / more-static, `less` the
* reverse, `equivalent` agreement, `incomparable` a genuine mismatch.  An
* equivalence axis (storage) yields only `equivalent` or `incomparable`, never an
* ordering.
*
*
* path:      /inc/djinterp/core/container/traits/container_comparison_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.30
******************************************************************************/

#ifndef DJINTERP_CONTAINER_COMPARISON_TRAITS_
#define DJINTERP_CONTAINER_COMPARISON_TRAITS_ 1

// std
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"                       // clean_t, NS_*, feature macros
#include "../../meta/lifetime.hpp"                  // lifetime, is_compile_time
#include "../../meta/storage.hpp"                   // storage_duration, storage_of
#include "../../meta/multiplicity.hpp"              // multiplicity_kind
#include "./constexpr_container_traits.hpp"         // container_lifetime
#include "./iterable_container_traits.hpp"          // is_iterable_container, iteration_mode_of
#include "./container_overlay_traits.hpp"           // overlay_of, overlay_subsumes
#include "../structure/hierarchical_container_traits.hpp" // is_hierarchical_container


NS_DJINTERP


// ===========================================================================
// I.   Comparison relations
// ===========================================================================

// order_relation
//   enum: the standing of a left operand against a right, on one axis.  On a
// preorder axis all four arise; on an equivalence axis only equivalent and
// incomparable do.
enum class order_relation
{
    equivalent,     // the two agree on this axis
    less,           // left is the weaker / coarser / less-capable
    greater,        // left is the stronger / finer / more-capable
    incomparable    // each stands somewhere the other does not
};

// order_relation_name
//   function: a stable spelling, for diagnostics and agent-facing summaries.
constexpr const char*
order_relation_name(order_relation _r) noexcept
{
    return ( _r == order_relation::equivalent   ? "equivalent"
           : _r == order_relation::less         ? "less"
           : _r == order_relation::greater      ? "greater"
           :                                      "incomparable" );
}

// order_relation_meet
//   function: the product-order combination of two axis relations.  Agreement is
// the unit; a shared direction is preserved; opposing directions - or any
// incomparability - collapse to incomparable.  This is how the per-axis verdicts
// fold into the single refinement relation.
constexpr order_relation
order_relation_meet(order_relation _a, order_relation _b) noexcept
{
    return ( _a == order_relation::incomparable
          || _b == order_relation::incomparable )  ? order_relation::incomparable
         : ( _a == order_relation::equivalent )     ? _b
         : ( _b == order_relation::equivalent )     ? _a
         : ( _a == _b )                             ? _a   // both less, or both greater
         :                                            order_relation::incomparable;
}


// ===========================================================================
// II.  Content: the native level lambda
// ===========================================================================

// content_level
//   enum: a rung of the content hierarchy, coarsest to finest.  A type's native
// rung is fixed by its discipline, exactly as the frontier equalities nest.
enum class content_level
{
    none,       // not a container / indeterminate
    set,        // support only            (=set)  - a set's native rung
    bag,        // counts                  (=bag)  - a multiset's native rung
    seq,        // leaf order              (=seq)  - a sequence's native rung
    str         // shape and values        (=str)  - a nested container's native rung
};

// content_level_name
constexpr const char*
content_level_name(content_level _l) noexcept
{
    return ( _l == content_level::none ? "none"
           : _l == content_level::set  ? "set"
           : _l == content_level::bag  ? "bag"
           : _l == content_level::seq  ? "seq"
           :                             "str" );
}

// content_level_rank
//   function: fineness as an integer (none coarsest, str finest); the enum is
// declared in that order, so the cast is the rank.
constexpr int
content_level_rank(content_level _l) noexcept
{
    return static_cast<int>(_l);
}

// content_level_coarser
//   function: the coarser of two rungs - the rung at which two types compare, per
// the formal ``coarser of the two native levels''.
constexpr content_level
content_level_coarser(content_level _a, content_level _b) noexcept
{
    return ( content_level_rank(_a) <= content_level_rank(_b) ) ? _a : _b;
}

NS_INTERNAL

    // native_content_level_helper
    //   helper: the native rung from the structure and multiplicity verdicts.  A
    // nested container compares structurally; otherwise the multiplicity kind
    // fixes the rung - a sequence at seq, a multiset at bag, a unique discipline
    // at set.
    constexpr content_level
    native_content_level_helper(bool _hierarchical, multiplicity_kind _mk) noexcept
    {
        return _hierarchical
                   ? content_level::str
             : ( _mk == multiplicity_kind::sequence )
                   ? content_level::seq
             : ( _mk == multiplicity_kind::bounded_multiset
              || _mk == multiplicity_kind::unbounded_multiset )
                   ? content_level::bag
             : ( _mk == multiplicity_kind::unique )
                   ? content_level::set
             :     content_level::none;
    }

NS_END  // internal

// native_content_level_of
//   trait: the content rung at which a type natively compares.
template<typename _Type>
struct native_content_level_of
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr content_level value =
        internal::native_content_level_helper(
            is_hierarchical_container<clean_type>::value,
            overlay_of<clean_type>::mult );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr content_level native_content_level_of_v =
        native_content_level_of<_Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr content_level native_content_level_of_v =
        native_content_level_of<_Type>::value;
#endif


// ===========================================================================
// III. Discipline: the overlay strength order
// ===========================================================================

// discipline_relation
//   trait: how two types' disciplines compare under the overlay strength order.
// `greater` is the left more restrictive (it wears every restriction the right
// does, and more); mutual containment is discipline-equality.
template<typename _Left,
         typename _Right>
struct discipline_relation
{
private:
    static constexpr bool left_subsumes_right =
        overlay_subsumes( overlay_of<clean_t<_Left>>::value(),
                          overlay_of<clean_t<_Right>>::value() );
    static constexpr bool right_subsumes_left =
        overlay_subsumes( overlay_of<clean_t<_Right>>::value(),
                          overlay_of<clean_t<_Left>>::value() );

public:
    static constexpr order_relation value =
        ( left_subsumes_right && right_subsumes_left ) ? order_relation::equivalent
      : ( left_subsumes_right )                        ? order_relation::greater
      : ( right_subsumes_left )                        ? order_relation::less
      :                                                  order_relation::incomparable;
};


// ===========================================================================
// IV.  Realization preorders
// ===========================================================================

// ---------------------------------------------------------------------------
//  Lifetime (a two-point total order, compile-time the more static end)
// ---------------------------------------------------------------------------

NS_INTERNAL

    // is_compile_stage_helper
    //   helper: whether a type's defining data are fixed by compile time - the
    // stage the Lifetime axis reports, taken as the more static (higher) end.
    template<typename _Type>
    struct is_compile_stage_helper
        : std::integral_constant<bool,
              is_compile_time( container_lifetime<clean_t<_Type>>::value )>
    {};

NS_END  // internal

// lifetime_relation
//   trait: how two types stand on staticity.  A total order - never incomparable
// - with the compile-time stage the stronger (greater).
template<typename _Left,
         typename _Right>
struct lifetime_relation
{
private:
    static constexpr bool left_static  =
        internal::is_compile_stage_helper<_Left>::value;
    static constexpr bool right_static =
        internal::is_compile_stage_helper<_Right>::value;

public:
    static constexpr order_relation value =
        ( left_static == right_static ) ? order_relation::equivalent
      : ( left_static )                 ? order_relation::greater
      :                                   order_relation::less;
};

// ---------------------------------------------------------------------------
//  Iterability (the five-element capability lattice)
// ---------------------------------------------------------------------------

NS_INTERNAL

    // iterability_relation_helper
    //   helper: the capability lattice as a product.  Not-iterable is the bottom;
    // among iterables the order is the product of compile-reachable traversal and
    // non-const-reachable traversal, so the two mixed kinds - compile/const and
    // runtime/non-const - are incomparable, as the lattice has them.
    constexpr order_relation
    iterability_relation_helper(bool _iter_l, bool _stage_l, bool _mode_l,
                                bool _iter_r, bool _stage_r, bool _mode_r) noexcept
    {
        return ( !_iter_l && !_iter_r )                 ? order_relation::equivalent
             : ( !_iter_r )                             ? order_relation::greater
             : ( !_iter_l )                             ? order_relation::less
             : ( _stage_l == _stage_r
              && _mode_l  == _mode_r )                  ? order_relation::equivalent
             : ( _stage_l >= _stage_r
              && _mode_l  >= _mode_r )                  ? order_relation::greater
             : ( _stage_l <= _stage_r
              && _mode_l  <= _mode_r )                  ? order_relation::less
             :                                            order_relation::incomparable;
    }

    // non_const_reachable_helper
    //   helper: whether a type admits a non-const (settable) traversal.
    template<typename _Type>
    struct non_const_reachable_helper
        : std::integral_constant<bool,
              iteration_mode_of<clean_t<_Type>>::value
                  == iteration_mode::non_const>
    {};

NS_END  // internal

// iterability_relation
//   trait: how two types compare on traversal capability, in the five-element
// lattice.  The compile-reachable bit is read from the Lifetime axis, per the
// formal entailment that a compile-time-fixed container traverses at compile time.
template<typename _Left,
         typename _Right>
struct iterability_relation
{
private:
    using L = clean_t<_Left>;
    using R = clean_t<_Right>;

public:
    static constexpr order_relation value =
        internal::iterability_relation_helper(
            is_iterable_container<L>::value,
            is_iterable_container<L>::value
                && internal::is_compile_stage_helper<L>::value,
            internal::non_const_reachable_helper<L>::value,
            is_iterable_container<R>::value,
            is_iterable_container<R>::value
                && internal::is_compile_stage_helper<R>::value,
            internal::non_const_reachable_helper<R>::value );
};

// ---------------------------------------------------------------------------
//  Storage (an equivalence: same siting)
// ---------------------------------------------------------------------------

// storage_relation
//   trait: whether two types share a storage class.  An equivalence axis -
// content is invariant under siting - so the verdict is only equivalent or
// incomparable, never an ordering.
template<typename _Left,
         typename _Right>
struct storage_relation
{
    static constexpr order_relation value =
        ( storage_of<clean_t<_Left>>::value
              == storage_of<clean_t<_Right>>::value )
            ? order_relation::equivalent
            : order_relation::incomparable;
};


// ===========================================================================
// V.   The comparison profile
// ===========================================================================

// comparison_profile
//   trait: the assembled comparison of two types - the native content rungs and
// the rung they share, the discipline order, and the three realization preorders,
// together with the product refinement over the type-decidable axes (discipline
// and realization).  The content =L itself reads values and is not decided here;
// the shared rung bounds the level at which it could be.
template<typename _Left,
         typename _Right>
struct comparison_profile
{
private:
    using L = clean_t<_Left>;
    using R = clean_t<_Right>;

public:
    // content (native rungs; the shared rung is the coarser)
    static constexpr content_level content_left =
        native_content_level_of<L>::value;
    static constexpr content_level content_right =
        native_content_level_of<R>::value;
    static constexpr content_level content_shared =
        content_level_coarser(content_left, content_right);

    // discipline + realization
    static constexpr order_relation discipline_order =
        discipline_relation<L, R>::value;
    static constexpr order_relation lifetime_order =
        lifetime_relation<L, R>::value;
    static constexpr order_relation iterability_order =
        iterability_relation<L, R>::value;
    static constexpr order_relation storage_order =
        storage_relation<L, R>::value;

    // the product refinement over the type-decidable axes
    static constexpr order_relation refinement =
        order_relation_meet(
            order_relation_meet(
                order_relation_meet(discipline_order, lifetime_order),
                iterability_order),
            storage_order );
};

// refinement_of
//   trait: the product refinement relation alone - the standing of two types over
// discipline and realization together.
template<typename _Left,
         typename _Right>
struct refinement_of
{
    static constexpr order_relation value =
        comparison_profile<_Left, _Right>::refinement;
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Left, typename _Right>
    inline constexpr order_relation refinement_of_v =
        refinement_of<_Left, _Right>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Left, typename _Right>
    constexpr order_relation refinement_of_v =
        refinement_of<_Left, _Right>::value;
#endif

// realization_equal
//   trait: true iff two types agree on discipline and every realization axis -
// the type-decidable half of ``equal''.  Full equality adds agreement of the
// contents at the shared level, a value-level fact.
template<typename _Left,
         typename _Right>
struct realization_equal
    : std::integral_constant<bool,
          refinement_of<_Left, _Right>::value == order_relation::equivalent>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Left, typename _Right>
    inline constexpr bool realization_equal_v =
        realization_equal<_Left, _Right>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Left, typename _Right>
    constexpr bool realization_equal_v =
        realization_equal<_Left, _Right>::value;
#endif

// realization_comparable
//   trait: true iff two types are ordered (or equal) on the refinement order -
// i.e. not incomparable.  When false, the two differ in ways that make neither a
// refinement of the other (a permutation against an allocator difference, say).
template<typename _Left,
         typename _Right>
struct realization_comparable
    : std::integral_constant<bool,
          refinement_of<_Left, _Right>::value != order_relation::incomparable>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Left, typename _Right>
    inline constexpr bool realization_comparable_v =
        realization_comparable<_Left, _Right>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Left, typename _Right>
    constexpr bool realization_comparable_v =
        realization_comparable<_Left, _Right>::value;
#endif


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_COMPARISON_TRAITS_
