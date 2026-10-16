/*******************************************************************************
* djinterp [core]               concurrency_strategy_tags_tests_tag_identity.cpp
*
*
* path:      /tests/djinterp/core/sync/concurrency_strategy_tags_tests_tag_identity.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]        concurrency_strategy_tags_tests_tag_identity.cpp
//   Section I (b): what makes the seven tags SEVEN.
//
//   Emptiness is what a tag costs; DISTINCTNESS is what it buys.  Because the
// tags carry no state, two of them could differ only in name - and a name is
// exactly what the trait system dispatches on.  This section pins that down:
// no two tags are the same type, none derives from or converts to another,
// each selects its own overload, and each keys its own specialization.  It
// also draws the line the header's hybrid_strategy_tag comment implies: a type
// that INHERITS two tags is not a tag, which is precisely why `hybrid` is a
// first-class seventh tag rather than a composition of the other six.

// djinterp
#include "concurrency_strategy_tags_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // not_same / not_base_of / not_convertible
    //   trait: the three ways two tags could fail to be distinct, each as a
    // two-parameter predicate the pair sweep below can apply.
    template<typename _A,
             typename _B>
    struct not_same
    {
        static D_CONSTEXPR bool value = !std::is_same<_A, _B>::value;
    };

    template<typename _A,
             typename _B>
    struct not_base_of
    {
        static D_CONSTEXPR bool value = !std::is_base_of<_A, _B>::value;
    };

    template<typename _A,
             typename _B>
    struct not_convertible
    {
        static D_CONSTEXPR bool value = !std::is_convertible<_A, _B>::value;
    };

    // both_ways
    //   function: the relation holds in BOTH directions for the pair, so a
    // sweep over the 21 unordered pairs covers all 42 ordered ones.
    template<template<typename, typename> class _Rel,
             typename                            _A,
             typename                            _B>
    D_CONSTEXPR bool
    both_ways()
    {
        return ( _Rel<_A, _B>::value &&
                 _Rel<_B, _A>::value );
    }

    // every_distinct_pair
    //   function: applies the relation to every one of the 21 unordered pairs
    // of distinct tags (identical pairs are excluded - is_base_of<A, A> and
    // is_convertible<A, A> are TRUE for a class, and rightly so).
    template<template<typename, typename> class _Rel>
    D_CONSTEXPR bool
    every_distinct_pair()
    {
        return ( both_ways<_Rel, none_strategy_tag, locked_strategy_tag>()   &&
                 both_ways<_Rel, none_strategy_tag, cow_strategy_tag>()      &&
                 both_ways<_Rel, none_strategy_tag, rcu_strategy_tag>()      &&
                 both_ways<_Rel, none_strategy_tag, atomic_strategy_tag>()   &&
                 both_ways<_Rel, none_strategy_tag, hazard_strategy_tag>()   &&
                 both_ways<_Rel, none_strategy_tag, hybrid_strategy_tag>()   &&

                 both_ways<_Rel, locked_strategy_tag, cow_strategy_tag>()    &&
                 both_ways<_Rel, locked_strategy_tag, rcu_strategy_tag>()    &&
                 both_ways<_Rel, locked_strategy_tag, atomic_strategy_tag>() &&
                 both_ways<_Rel, locked_strategy_tag, hazard_strategy_tag>() &&
                 both_ways<_Rel, locked_strategy_tag, hybrid_strategy_tag>() &&

                 both_ways<_Rel, cow_strategy_tag, rcu_strategy_tag>()       &&
                 both_ways<_Rel, cow_strategy_tag, atomic_strategy_tag>()    &&
                 both_ways<_Rel, cow_strategy_tag, hazard_strategy_tag>()    &&
                 both_ways<_Rel, cow_strategy_tag, hybrid_strategy_tag>()    &&

                 both_ways<_Rel, rcu_strategy_tag, atomic_strategy_tag>()    &&
                 both_ways<_Rel, rcu_strategy_tag, hazard_strategy_tag>()    &&
                 both_ways<_Rel, rcu_strategy_tag, hybrid_strategy_tag>()    &&

                 both_ways<_Rel, atomic_strategy_tag, hazard_strategy_tag>() &&
                 both_ways<_Rel, atomic_strategy_tag, hybrid_strategy_tag>() &&

                 both_ways<_Rel, hazard_strategy_tag, hybrid_strategy_tag>() );
    }

    // derived_cow
    //   type: a type that IS-A cow strategy.  The alternative to the alias
    // protocol: inherit the tag instead of naming it.
    struct derived_cow : cow_strategy_tag
    {};

    // locked_holder
    //   type: a type carrying the locked tag as a base and one data member -
    // the empty-base-optimization case the container layer depends on.
    struct locked_holder : locked_strategy_tag
    {
        int value;
    };

    // locked_and_cow
    //   type: "locked metadata around a cow payload", spelled STRUCTURALLY -
    // by inheriting both tags.  It is empty, and it derives from both; but it
    // is not itself a tag, and it cannot be dispatched on (either base is an
    // equally good match).  hybrid_strategy_tag exists so that this shape is
    // never needed.
    struct locked_and_cow : locked_strategy_tag,
                            cow_strategy_tag
    {};
}

/*
tests_tags_are_pairwise_distinct
  no two of the seven tags are the same type.
  Tests the following:
  - std::is_same is false for all 21 unordered pairs
  - so a trait keyed on a tag can never confuse two strategies
*/
bool
tests_tags_are_pairwise_distinct()
{
    D_CONSTEXPR bool distinct = every_distinct_pair<not_same>();

    static_assert(distinct, "the seven strategy tags must be seven types");

    return distinct;
}

/*
tests_tags_are_identical_to_themselves
  the sanity check that keeps the negative sweeps honest.
  Tests the following:
  - each tag IS the same type as itself, and IS convertible to itself
  - so the pairwise negatives above are not passing vacuously (a broken
    is_same would fail here first)
*/
bool
tests_tags_are_identical_to_themselves()
{
    D_CONSTEXPR bool reflexive =
        ( std::is_same<none_strategy_tag, none_strategy_tag>::value          &&
          std::is_same<locked_strategy_tag, locked_strategy_tag>::value      &&
          std::is_same<cow_strategy_tag, cow_strategy_tag>::value            &&
          std::is_same<rcu_strategy_tag, rcu_strategy_tag>::value            &&
          std::is_same<atomic_strategy_tag, atomic_strategy_tag>::value      &&
          std::is_same<hazard_strategy_tag, hazard_strategy_tag>::value      &&
          std::is_same<hybrid_strategy_tag, hybrid_strategy_tag>::value      &&
          std::is_convertible<cow_strategy_tag, cow_strategy_tag>::value     &&
          std::is_base_of<hybrid_strategy_tag, hybrid_strategy_tag>::value );

    static_assert(reflexive, "a tag must be identical to itself");

    return reflexive;
}

/*
tests_tags_are_unrelated_by_inheritance
  the seven are peers - a flat set, not a hierarchy.
  Tests the following:
  - std::is_base_of is false for all 42 ordered pairs of distinct tags
  - so no tag can be silently substituted for another through a base
    conversion, and no dispatch can be hijacked by an inheritance edge
*/
bool
tests_tags_are_unrelated_by_inheritance()
{
    D_CONSTEXPR bool unrelated = every_distinct_pair<not_base_of>();

    static_assert(unrelated, "the strategy tags must form a flat set");

    return unrelated;
}

/*
tests_tags_do_not_interconvert
  no tag converts to any other.
  Tests the following:
  - std::is_convertible is false for all 42 ordered pairs of distinct tags
  - the tags are empty, but emptiness does not make them interchangeable: an
    added converting constructor would, and this is what would catch it
*/
bool
tests_tags_do_not_interconvert()
{
    D_CONSTEXPR bool no_conversion = every_distinct_pair<not_convertible>();

    static_assert(no_conversion, "no strategy tag may convert to another");

    return no_conversion;
}

/*
tests_tag_dispatch_selects_the_matching_overload
  overload resolution alone picks the right arm - the tags' first job.
  Tests the following:
  - each of the seven tags selects its own overload of dispatch_on
  - the seven results are seven distinct values, so no two tags share an arm
*/
bool
tests_tag_dispatch_selects_the_matching_overload()
{
    const bool selected =
        ( (dispatch_on(none_strategy_tag())   == 0) &&
          (dispatch_on(locked_strategy_tag()) == 1) &&
          (dispatch_on(cow_strategy_tag())    == 2) &&
          (dispatch_on(rcu_strategy_tag())    == 3) &&
          (dispatch_on(atomic_strategy_tag()) == 4) &&
          (dispatch_on(hazard_strategy_tag()) == 5) &&
          (dispatch_on(hybrid_strategy_tag()) == 6) );

    return selected;
}

/*
tests_tag_dispatch_accepts_a_derived_tag
  a type that INHERITS a tag dispatches as that tag.
  Tests the following:
  - derived_cow, which derives from cow_strategy_tag, selects the cow overload
    through the derived-to-base conversion
  - the conversion is one-way: cow_strategy_tag is not a derived_cow
  - so a container may carry its strategy by inheritance as well as by alias
*/
bool
tests_tag_dispatch_accepts_a_derived_tag()
{
    const bool dispatches_as_cow = (dispatch_on(derived_cow()) == 2);

    D_CONSTEXPR bool one_way =
        ( std::is_base_of<cow_strategy_tag, derived_cow>::value      &&
          std::is_convertible<derived_cow, cow_strategy_tag>::value  &&
          !std::is_convertible<cow_strategy_tag, derived_cow>::value );

    static_assert(one_way, "the derived-to-base conversion must be one-way");

    return (dispatches_as_cow && one_way);
}

/*
tests_tags_key_template_specializations
  a tag can key a template specialization - the tags' second job, and the one
  concurrency_strategy_traits.hpp actually performs.
  Tests the following:
  - strategy_tag_index resolves to a distinct value for each of the seven
  - the primary template still answers for a type that is not a tag
  - so a trait may specialize on a tag exhaustively AND keep a default
*/
bool
tests_tags_key_template_specializations()
{
    D_CONSTEXPR bool keyed =
        ( (strategy_tag_index<none_strategy_tag>::value   == 0) &&
          (strategy_tag_index<locked_strategy_tag>::value == 1) &&
          (strategy_tag_index<cow_strategy_tag>::value    == 2) &&
          (strategy_tag_index<rcu_strategy_tag>::value    == 3) &&
          (strategy_tag_index<atomic_strategy_tag>::value == 4) &&
          (strategy_tag_index<hazard_strategy_tag>::value == 5) &&
          (strategy_tag_index<hybrid_strategy_tag>::value == 6) );

    D_CONSTEXPR bool primary_answers =
        ( (strategy_tag_index<int>::value           == k_not_a_tag) &&
          (strategy_tag_index<mock_untagged>::value == k_not_a_tag) );

    static_assert(keyed, "each tag must key its own specialization");

    return (keyed && primary_answers);
}

/*
tests_the_header_declares_exactly_seven_tags
  the set is closed: seven tags, no more and no fewer.
  Tests the following:
  - is_strategy_tag holds for each of the seven
  - their indices are exactly 0..6 - a bijection onto the seven slots, so none
    is duplicated and none is missing
  - k_strategy_tag_count agrees
  An eighth tag added to the header without being added here would leave this
  test passing but the suite incomplete; an eighth tag added HERE without the
  header would not compile.  The pair of them is the coverage guarantee.
*/
bool
tests_the_header_declares_exactly_seven_tags()
{
    D_CONSTEXPR bool all_are_tags =
        ( is_strategy_tag<none_strategy_tag>::value   &&
          is_strategy_tag<locked_strategy_tag>::value &&
          is_strategy_tag<cow_strategy_tag>::value    &&
          is_strategy_tag<rcu_strategy_tag>::value    &&
          is_strategy_tag<atomic_strategy_tag>::value &&
          is_strategy_tag<hazard_strategy_tag>::value &&
          is_strategy_tag<hybrid_strategy_tag>::value );

    // the seven indices, summed: 0+1+2+3+4+5+6 == 21 only if each slot is hit
    // exactly once (they are already known to be in 0..6 and pairwise
    // distinct by the specialization test above)
    D_CONSTEXPR int index_sum =
        ( strategy_tag_index<none_strategy_tag>::value   +
          strategy_tag_index<locked_strategy_tag>::value +
          strategy_tag_index<cow_strategy_tag>::value    +
          strategy_tag_index<rcu_strategy_tag>::value    +
          strategy_tag_index<atomic_strategy_tag>::value +
          strategy_tag_index<hazard_strategy_tag>::value +
          strategy_tag_index<hybrid_strategy_tag>::value );

    D_CONSTEXPR bool closed =
        ( all_are_tags               &&
          (index_sum == 21)          &&
          (k_strategy_tag_count == 7) );

    static_assert(closed, "the header must declare exactly seven tags");

    return closed;
}

/*
tests_tags_support_empty_base_optimization
  inheriting a tag costs nothing.
  Tests the following:
  - a class deriving from locked_strategy_tag and holding one int is the size
    of that int - the empty base was optimized away
  - the base is still there for is_base_of and for dispatch
  Not a static_assert: the empty-base optimization is an ABI guarantee rather
  than a language one, so a platform that failed to apply it should FAIL this
  test, not fail to build the suite.
*/
bool
tests_tags_support_empty_base_optimization()
{
    const bool costs_nothing = (sizeof(locked_holder) == sizeof(int));

    D_CONSTEXPR bool base_survives =
        ( std::is_base_of<locked_strategy_tag, locked_holder>::value &&
          std::is_empty<locked_strategy_tag>::value );

    const bool still_dispatches = (dispatch_on(locked_holder()) == 1);

    return ( costs_nothing  &&
             base_survives  &&
             still_dispatches );
}

/*
tests_tags_compose_by_multiple_inheritance
  two tags CAN be inherited at once - and the result is not a tag.
  Tests the following:
  - locked_and_cow derives from both locked_strategy_tag and cow_strategy_tag
  - it is still an empty type (empty bases, no members)
  - it is NOT one of the seven, and it is NOT hybrid_strategy_tag
  - dispatch_on(locked_and_cow()) is deliberately NOT called: both base
    conversions are equally good, so the call would be ambiguous.  That
    ambiguity is the reason the header gives `hybrid` a tag of its own.
*/
bool
tests_tags_compose_by_multiple_inheritance()
{
    D_CONSTEXPR bool derives_from_both =
        ( std::is_base_of<locked_strategy_tag, locked_and_cow>::value &&
          std::is_base_of<cow_strategy_tag, locked_and_cow>::value );

    D_CONSTEXPR bool still_empty = std::is_empty<locked_and_cow>::value;

    D_CONSTEXPR bool not_itself_a_tag =
        ( !is_strategy_tag<locked_and_cow>::value &&
          !std::is_same<locked_and_cow, hybrid_strategy_tag>::value );

    static_assert(derives_from_both && still_empty,
                  "two empty tags must compose into an empty type");

    return ( derives_from_both &&
             still_empty       &&
             not_itself_a_tag );
}

/*
tests_hybrid_is_a_peer_not_a_composite
  hybrid_strategy_tag is a seventh NAME, not a structural union of the others.
  Tests the following:
  - it derives from no other tag (nor any other tag from it)
  - it is empty and one byte, exactly like its six peers
  - it dispatches unambiguously, which a composed type could not
  - so "combines two or more strategies" is a claim about what the tag MEANS,
    not about how it is built
*/
bool
tests_hybrid_is_a_peer_not_a_composite()
{
    D_CONSTEXPR bool no_base =
        ( !std::is_base_of<none_strategy_tag, hybrid_strategy_tag>::value   &&
          !std::is_base_of<locked_strategy_tag, hybrid_strategy_tag>::value &&
          !std::is_base_of<cow_strategy_tag, hybrid_strategy_tag>::value    &&
          !std::is_base_of<rcu_strategy_tag, hybrid_strategy_tag>::value    &&
          !std::is_base_of<atomic_strategy_tag, hybrid_strategy_tag>::value &&
          !std::is_base_of<hazard_strategy_tag, hybrid_strategy_tag>::value );

    D_CONSTEXPR bool shaped_like_its_peers =
        ( std::is_empty<hybrid_strategy_tag>::value &&
          (sizeof(hybrid_strategy_tag) == 1) );

    const bool dispatches_unambiguously =
        (dispatch_on(hybrid_strategy_tag()) == 6);

    static_assert(no_base, "hybrid must not derive from another tag");

    return ( no_base                  &&
             shaped_like_its_peers    &&
             dispatches_unambiguously );
}

/*
tests_the_tag_predicate_rejects_non_tags
  the seven, and nothing else.
  Tests the following:
  - is_strategy_tag is false for a fundamental type, for void, for a pointer
    and a reference TO a tag, for an untagged class, and for a composition of
    two tags
  - so "is one of the header's tags" is a strictly narrower question than "is
    empty" or "is a class"
*/
bool
tests_the_tag_predicate_rejects_non_tags()
{
    D_CONSTEXPR bool rejected =
        ( !is_strategy_tag<int>::value                 &&
          !is_strategy_tag<void>::value                &&
          !is_strategy_tag<std::size_t>::value         &&
          !is_strategy_tag<none_strategy_tag*>::value  &&
          !is_strategy_tag<cow_strategy_tag&>::value   &&
          !is_strategy_tag<const rcu_strategy_tag>::value &&
          !is_strategy_tag<mock_untagged>::value       &&
          !is_strategy_tag<locked_and_cow>::value      &&
          !is_strategy_tag<derived_cow>::value );

    static_assert(rejected, "only the seven tags may satisfy is_strategy_tag");

    return rejected;
}


NS_END  // testing
NS_END  // djinterp
