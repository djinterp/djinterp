/******************************************************************************
* djinterp [container]                     observational_comparison_traits.hpp
*
*   SFINAE structural traits for OBSERVATIONAL comparison - through which of a
* container's interface operations two containers can be told apart.  Where
* container_comparison_traits.hpp answers the ABSTRACT question ("do these have
* equal content / discipline / realization", a matter of fact whether or not
* anyone can check it), this header answers the OBSERVATIONAL one: given only
* the operations a type's interface exposes, at what content level can a pair
* actually be DECIDED equal.  It is "through which operations", not "are they
* equal".
*
*   The interface I of a container is its inspection surface - size(), a const
* iteration, positional at(i)/operator[](i), a membership test contains(x), a
* per-class count(x).  Each basic interface is COMPLETE for exactly one content
* level (it decides that level's equality and every coarser one):
*
*     { at(.) , shape }   ->  =str      (positional access into a nested shape)
*     { const iteration } ->  =seq      (the ordered frontier is readable)
*     { count(.) }        ->  =bag      (per-class multiplicities are readable)
*     { contains(.) }     ->  =set      (membership is readable)
*     { size } alone      ->  size      (equal size only; coarser than all four)
*
*   THE SHARED-INTERFACE PRINCIPLE.  To compare c and c' one may use only the
* operations BOTH expose; the decidable comparison is complete for the finest
* level that shared interface resolves, and no finer.  So an iterable container
* compared with an opaque (non-iterable) one falls from =seq to whatever else is
* shared - contains -> =set, else size, else identity.  An abstract equality the
* shared interface cannot reach is not false, merely unobservable as things
* stand; conversion (container_conversion_traits.hpp) is what widens the shared
* interface.  observational_level_of<C> reports the finest a single type
* resolves; shared_observational_level<A,B> takes the coarser of the two - the
* principle in one operation.
*
*   RELATION TO THE ABSTRACT LEVELS.  observational_level's set/bag/seq/str name
* and rank the identically-named content_level values of
* container_comparison_traits.hpp, with an extra coarsest content step, size,
* below them (equal-size, which no content_level names).  The two enums are kept
* distinct - and this header takes no #include on the abstract-comparison header
* - because the axes are distinct: a std::set's NATIVE content level is set, yet
* its interface (via iteration) OBSERVATIONALLY resolves seq, since iterating a
* set yields its canonical order.  That gap is the whole subject here.
*
*   STAGE OF A COMPARISON (section V).  Each access carries a Lifetime stage; the
* stage of a comparison is the latest of the accesses it uses (compile-time only
* when BOTH are), which is the lattice meet of meta/lifetime.hpp - see the note
* there.  The compile-time-iterability of an operand is proxied here by a static
* size signal (so std::array reads as compile-stage and std::vector as runtime);
* the finer container_lifetime lives in constexpr_container_traits.hpp.
*
*   PORTABILITY:
*   C++11 baseline.  contains(x) is a C++20 member for the standard associative
* containers; the probe is generic and simply reads false for them pre-C++20,
* which does not change any observational_level_of (an iterable container
* already resolves seq, finer than set).  The `_v` companions degrade with the
* language as the rest of the trait family's do.
*
*   CONTENTS:
*     I.   Interface-operation probes         size / indexed / count / contains
*     II.  observational_level                the resolvable levels + rank/coarser
*     III. observational_level_of<C>          finest a type's interface resolves
*     IV.  shared_observational_level<A,B>     the shared-interface principle
*     V.   Stage of a comparison              the lifetime the accesses carry
*
*
* path:      /inc/djinterp/core/container/traits/observational_comparison_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.02
******************************************************************************/

#ifndef DJINTERP_OBSERVATIONAL_COMPARISON_TRAITS_
#define DJINTERP_OBSERVATIONAL_COMPARISON_TRAITS_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"                            // clean_t, NS_*, D_ENV_*
#include "../../meta/trait_detect.hpp"                   // D_TYPE_TRAIT_* macros
#include "../../meta/lifetime.hpp"                       // lifetime, lifetime_meet, is_compile_time
#include "./iterable_container_traits.hpp"               // is_iterable_container (const iteration)
#include "./element_relation_traits.hpp"                 // element_type_of_t (count/contains argument)
#include "../structure/hierarchical_container_traits.hpp" // is_hierarchical_container (shape, for =str)


NS_DJINTERP


// ===========================================================================
// I.   Interface-operation probes
// ===========================================================================
//   One SFINAE probe per inspection operation of the interface I.  All strip
// cv-ref via clean_t and read on a CONST lvalue - observation leaves the
// container invariant, so a const-callable operation is what counts.

// has_size_member
//   trait: detects a const-callable size() - the equal-size interface.
D_TYPE_TRAIT_TRUE(has_size_member,
    decltype(std::declval<const clean_t<_Type>&>().size()))

// has_subscript_index
//   trait: detects positional operator[](size_t) on a const lvalue.  A map's
// operator[] is non-const (it inserts) and so is NOT matched here.
D_TYPE_TRAIT_TRUE(has_subscript_index,
    decltype(std::declval<const clean_t<_Type>&>()[std::declval<std::size_t>()]))

// has_at_index
//   trait: detects a const-callable at(size_t).  Probed with size_t per the
// positional reading; a keyed at(key) with an integral key (e.g. a map) will
// also match, an approximation a single structural probe does not separate.
// It is harmless to the level: such a container is invariably also iterable and
// so resolves seq regardless.
D_TYPE_TRAIT_TRUE(has_at_index,
    decltype(std::declval<const clean_t<_Type>&>().at(std::declval<std::size_t>())))

// has_indexed_access
//   trait: positional access is available - operator[] OR at with a size_t.
// Supplies the =str interface when the container also has shape, and otherwise
// a positional (seq) read.
template<typename _Type>
struct has_indexed_access
    : std::integral_constant<bool,
          (    has_subscript_index<clean_t<_Type>>::value
            || has_at_index<clean_t<_Type>>::value )>
{};

D_TYPE_TRAIT_VALUE_BOOL(has_indexed_access)

// has_count_member
//   trait: detects a const-callable count(x) taking an element - the per-class
// (bag) interface.  x is drawn from element_type_of_t, which for a set or
// multiset is the key; a keyed container whose key_type differs from value_type
// (a map) is not matched by the value-typed probe, an honest limitation of a
// value-shaped probe (maps carry their keyed discipline through the overlay
// machinery, not here).
D_TYPE_TRAIT_TRUE(has_count_member,
    decltype(std::declval<const clean_t<_Type>&>().count(
        std::declval<element_type_of_t<_Type>>())))

// has_contains_member
//   trait: detects a const-callable contains(x) taking an element - the
// membership (set) interface.  A C++20 member for the standard containers;
// generic here, so false pre-C++20 without disturbing any level.
D_TYPE_TRAIT_TRUE(has_contains_member,
    decltype(std::declval<const clean_t<_Type>&>().contains(
        std::declval<element_type_of_t<_Type>>())))


// ===========================================================================
// II.  The observational level
// ===========================================================================

// observational_level
//   enum: the finest content level an interface can DECIDE.  A refinement
// order, coarsest to finest: none < size < set < bag < seq < str.  The four
// finest name the content_level values of container_comparison_traits.hpp; size
// is the extra coarsest content step (equal-size only) that observation adds.
enum class observational_level
{
    none,   // nothing shared but identity / type
    size,   // equal size only (coarser than every content level)
    set,    // membership readable        - contains(.) -> =set
    bag,    // per-class counts readable  - count(.)    -> =bag
    seq,    // ordered frontier readable  - iteration   -> =seq
    str     // positional shape readable  - at(.)+shape -> =str
};

// observational_level_name
//   function: a stable spelling, for diagnostics and agent-facing summaries.
constexpr const char*
observational_level_name(observational_level _l) noexcept
{
    return ( _l == observational_level::none ? "none"
           : _l == observational_level::size ? "size"
           : _l == observational_level::set  ? "set"
           : _l == observational_level::bag  ? "bag"
           : _l == observational_level::seq  ? "seq"
           :                                   "str" );
}

// observational_level_rank
//   function: the refinement rank (none=0 < size=1 < set=2 < bag=3 < seq=4 <
// str=5), the linear order the level preorder collapses to.
constexpr int
observational_level_rank(observational_level _l) noexcept
{
    return ( _l == observational_level::none ? 0
           : _l == observational_level::size ? 1
           : _l == observational_level::set  ? 2
           : _l == observational_level::bag  ? 3
           : _l == observational_level::seq  ? 4
           :                                   5 );
}

// observational_level_coarser
//   function: the coarser (lower-rank) of two levels - the level a shared
// interface resolves, and the meet of the refinement order.
constexpr observational_level
observational_level_coarser(
    observational_level _a,
    observational_level _b
) noexcept
{
    return ( observational_level_rank(_a) <= observational_level_rank(_b) ) ? _a : _b;
}

// observational_level_finer
//   function: the finer (higher-rank) of two levels - the join of the
// refinement order.
constexpr observational_level
observational_level_finer(
    observational_level _a,
    observational_level _b
) noexcept
{
    return ( observational_level_rank(_a) >= observational_level_rank(_b) ) ? _a : _b;
}

// observational_level_subsumes
//   function: true iff level _a resolves at least as finely as _b (rank order).
constexpr bool
observational_level_subsumes(
    observational_level _a,
    observational_level _b
) noexcept
{
    return ( observational_level_rank(_a) >= observational_level_rank(_b) );
}


// ===========================================================================
// III. The level a single type's interface resolves
// ===========================================================================

// observational_level_of
//   trait: the FINEST level the type's interface resolves.  A cascade from
// finest to coarsest, which is the max over the completeness levels of the
// operations present (a finer operation subsumes the coarser: reading the
// sequence also reads the bag, the set, the size).  Indexed access into a
// hierarchical shape resolves str; iteration or a flat positional read resolves
// seq; then count -> bag, contains -> set, size -> size, else none.
template<typename _Type>
struct observational_level_of
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr observational_level value =
        (    has_indexed_access<clean_type>::value
          && is_hierarchical_container<clean_type>::value )
              ? observational_level::str
      : (    is_iterable_container<clean_type>::value
          || has_indexed_access<clean_type>::value )
              ? observational_level::seq
      : (  has_count_member<clean_type>::value )
              ? observational_level::bag
      : (  has_contains_member<clean_type>::value )
              ? observational_level::set
      : (  has_size_member<clean_type>::value )
              ? observational_level::size
      :         observational_level::none;

    using type = std::integral_constant<observational_level, value>;
};

// observational_level_of_t / observational_level_of_v
template<typename _Type>
using observational_level_of_t = typename observational_level_of<_Type>::type;

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr observational_level observational_level_of_v =
        observational_level_of<_Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr observational_level observational_level_of_v =
        observational_level_of<_Type>::value;
#endif


// ===========================================================================
// IV.  The shared-interface principle
// ===========================================================================

// shared_observational_level
//   trait: the level at which a PAIR can be decided - the coarser of the two
// types' resolved levels, using only what both interfaces share.  This is the
// principle: comparison is bounded by the access the two hold in common.
template<typename _Left,
         typename _Right>
struct shared_observational_level
{
    static constexpr observational_level value =
        observational_level_coarser(
            observational_level_of<clean_t<_Left>>::value,
            observational_level_of<clean_t<_Right>>::value );

    using type = std::integral_constant<observational_level, value>;
};

// shared_observational_level_t / shared_observational_level_v
template<typename _Left,
         typename _Right>
using shared_observational_level_t =
    typename shared_observational_level<_Left, _Right>::type;

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Left,
             typename _Right>
    inline constexpr observational_level shared_observational_level_v =
        shared_observational_level<_Left, _Right>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Left,
             typename _Right>
    constexpr observational_level shared_observational_level_v =
        shared_observational_level<_Left, _Right>::value;
#endif


// ===========================================================================
// V.   Stage of a comparison
// ===========================================================================
//   A comparison carries the latest stage of the accesses it uses (meta/
// lifetime.hpp: the meet composes stages so that a composite is compile-time
// only when every part is).  Each operand's access stage is proxied by a static
// size signal - the compile-time-iterability tell std::array carries and
// std::vector does not; the finer container_lifetime is in
// constexpr_container_traits.hpp.

NS_INTERNAL

    // has_static_size_helper
    //   helper: detects a std::tuple_size<T>::value - a compile-time size, the
    // proxy for compile-time observability used below.
    D_TYPE_TRAIT_TRUE(has_static_size_helper,
        decltype(std::tuple_size<clean_t<_Type>>::value))

NS_END  // internal

// observed_access_stage_of
//   trait: the Lifetime stage at which a type's contents are observable -
// lifetime::both (compile-time and runtime) when it carries a static size,
// lifetime::runtime otherwise.
template<typename _Type>
struct observed_access_stage_of
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr lifetime value =
        ( internal::has_static_size_helper<clean_type>::value )
              ? lifetime::both
      :         lifetime::runtime;

    using type = std::integral_constant<lifetime, value>;
};

// observed_access_stage_of_t / observed_access_stage_of_v
template<typename _Type>
using observed_access_stage_of_t = typename observed_access_stage_of<_Type>::type;

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr lifetime observed_access_stage_of_v =
        observed_access_stage_of<_Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr lifetime observed_access_stage_of_v =
        observed_access_stage_of<_Type>::value;
#endif

// observational_stage_of
//   trait: the stage a comparison of the two types carries - the meet of their
// access stages (compile-time only when both operands are compile-time
// observable).
template<typename _Left,
         typename _Right>
struct observational_stage_of
{
    static constexpr lifetime value =
        lifetime_meet(
            observed_access_stage_of<clean_t<_Left>>::value,
            observed_access_stage_of<clean_t<_Right>>::value );

    using type = std::integral_constant<lifetime, value>;
};

// observational_stage_of_t / observational_stage_of_v
template<typename _Left,
         typename _Right>
using observational_stage_of_t = typename observational_stage_of<_Left, _Right>::type;

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Left,
             typename _Right>
    inline constexpr lifetime observational_stage_of_v =
        observational_stage_of<_Left, _Right>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Left,
             typename _Right>
    constexpr lifetime observational_stage_of_v =
        observational_stage_of<_Left, _Right>::value;
#endif

// is_compile_time_comparison
//   trait: true iff a comparison of the two types is settled in constant
// evaluation - both operands compile-time observable.
template<typename _Left,
         typename _Right>
struct is_compile_time_comparison
    : std::integral_constant<bool,
          is_compile_time(observational_stage_of<_Left, _Right>::value)>
{};

// is_compile_time_comparison_v
#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Left,
             typename _Right>
    inline constexpr bool is_compile_time_comparison_v =
        is_compile_time_comparison<_Left, _Right>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Left,
             typename _Right>
    constexpr bool is_compile_time_comparison_v =
        is_compile_time_comparison<_Left, _Right>::value;
#endif


NS_END  // djinterp

#endif  // DJINTERP_OBSERVATIONAL_COMPARISON_TRAITS_
