/*******************************************************************************
* djinterp [cpp]                   container_comparison_traits_tests_profile.cpp
*
*
* path:      /tests/djinterp/cpp/container/traits/container_comparison_traits_tests_profile.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <array>
#include <type_traits>
#include <vector>
// djinterp
#include "container_comparison_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_profile_members
  DComparisonProfile - the eight members it exposes.
  Tests the following:
  - the three CONTENT members: the two native rungs and the rung they share
  - the four AXIS members: discipline, lifetime, iterability, storage
  - the one VERDICT member: refinement
  - every member is a static constexpr, so the whole profile is usable at compile
    time - each is used as a template argument, which is the only way to prove it
  - the profile is an EMPTY type: it carries answers, not storage
*/
bool tests_profile_members()
{
    bool ok = true;

    using p = DComparisonProfile<DCompileSeq, DRuntimeSeq>;

    // content
    D_CCM_LEVEL(content_level::seq, p::content_left);
    D_CCM_LEVEL(content_level::seq, p::content_right);
    D_CCM_LEVEL(content_level::seq, p::content_shared);

    // axes
    D_CCM_REL(order_relation::equivalent, p::discipline_order);
    D_CCM_REL(order_relation::greater,    p::lifetime_order);
    D_CCM_REL(order_relation::greater,    p::iterability_order);
    D_CCM_REL(order_relation::equivalent, p::storage_order);

    // verdict
    D_CCM_REL(order_relation::greater, p::refinement);

    // a profile whose content rungs DIFFER, so content_shared is doing work
    using q = DComparisonProfile<DNestedSeq, DUniqueSeq>;

    D_CCM_LEVEL(content_level::str, q::content_left);
    D_CCM_LEVEL(content_level::set, q::content_right);
    D_CCM_LEVEL(content_level::set, q::content_shared);   // the COARSER

    // every member is a constant expression
    D_CCM_CHECK((std::integral_constant<content_level,
                                        p::content_shared>::value ==
                 content_level::seq));
    D_CCM_CHECK((std::integral_constant<order_relation,
                                        p::refinement>::value ==
                 order_relation::greater));

    // and it costs nothing
    D_CCM_CHECK(std::is_empty<p>::value);
    D_CCM_CHECK(std::is_empty<q>::value);

    return ok;
}

/*
tests_profile_agrees_with_axes
  The profile must be a faithful restatement of its axes - a convenience that can
  disagree with what it summarizes is worse than no convenience.
  Tests the following:
  - all eight members agree with their standalone sources, over every fixture pair
    the suite has
  - content_shared is exactly content_level_coarser(content_left, content_right)
  - refinement is exactly the three nested meets of the four axis members
*/
bool tests_profile_agrees_with_axes()
{
    bool ok = true;

    #define D_CCM_PROFILE_AGREES(LEFT, RIGHT)                                 \
        D_CCM_CHECK(                                                          \
            (DComparisonProfile<LEFT, RIGHT>::content_left) ==                \
            (native_content_level_of<LEFT>::value));                          \
        D_CCM_CHECK(                                                          \
            (DComparisonProfile<LEFT, RIGHT>::content_right) ==               \
            (native_content_level_of<RIGHT>::value));                         \
        D_CCM_CHECK(                                                          \
            (DComparisonProfile<LEFT, RIGHT>::content_shared) ==              \
            content_level_coarser(                                            \
                (native_content_level_of<LEFT>::value),                       \
                (native_content_level_of<RIGHT>::value)));                    \
        D_CCM_CHECK(                                                          \
            (DComparisonProfile<LEFT, RIGHT>::discipline_order) ==            \
            (discipline_relation<LEFT, RIGHT>::value));                       \
        D_CCM_CHECK(                                                          \
            (DComparisonProfile<LEFT, RIGHT>::lifetime_order) ==              \
            (lifetime_relation<LEFT, RIGHT>::value));                         \
        D_CCM_CHECK(                                                          \
            (DComparisonProfile<LEFT, RIGHT>::iterability_order) ==           \
            (DIterabilityRelation<LEFT, RIGHT>::value));                      \
        D_CCM_CHECK(                                                          \
            (DComparisonProfile<LEFT, RIGHT>::storage_order) ==               \
            (storage_relation<LEFT, RIGHT>::value));                          \
        D_CCM_CHECK(                                                          \
            (DComparisonProfile<LEFT, RIGHT>::refinement) ==                  \
            order_relation_meet(                                              \
                order_relation_meet(                                          \
                    order_relation_meet(                                      \
                        (discipline_relation<LEFT, RIGHT>::value),            \
                        (lifetime_relation<LEFT, RIGHT>::value)),             \
                    (DIterabilityRelation<LEFT, RIGHT>::value)),              \
                (storage_relation<LEFT, RIGHT>::value)));                     \
        D_CCM_CHECK((refinement_of<LEFT, RIGHT>::value) ==                    \
                    (DComparisonProfile<LEFT, RIGHT>::refinement))

    D_CCM_PROFILE_AGREES(DRuntimeSeq,       DRuntimeSeq);
    D_CCM_PROFILE_AGREES(DCompileSeq,       DRuntimeSeq);
    D_CCM_PROFILE_AGREES(DCompileConstSeq, DRuntimeSeq);
    D_CCM_PROFILE_AGREES(DConstSeq,         DRuntimeSeq);
    D_CCM_PROFILE_AGREES(DNotIterable,      DRuntimeSeq);
    D_CCM_PROFILE_AGREES(DUniqueSeq,        DRuntimeSeq);
    D_CCM_PROFILE_AGREES(DBoundedBag,       DUnboundedBag);
    D_CCM_PROFILE_AGREES(DSortedSeq,        DCapacitySeq);
    D_CCM_PROFILE_AGREES(DKeyedSeq,         DRuntimeSeq);
    D_CCM_PROFILE_AGREES(DNestedSeq,        DRuntimeSeq);
    D_CCM_PROFILE_AGREES(DHierarchicalUnique, DUniqueSeq);
    D_CCM_PROFILE_AGREES(static_seq,        dynamic_seq);
    D_CCM_PROFILE_AGREES(DHybridSeq,        DRuntimeSeq);
    D_CCM_PROFILE_AGREES(DNotAContainer,   DNotIterable);
    D_CCM_PROFILE_AGREES(int,               std_vector);
    D_CCM_PROFILE_AGREES(std_array, std_vector);

    // it cleans its arguments, so the cv-ref forms agree with the bare ones
    D_CCM_PROFILE_AGREES(const DRuntimeSeq&, DCompileSeq&&);

    #undef D_CCM_PROFILE_AGREES

    return ok;
}

/*
tests_refinement_composition
  refinement_of - the product over the four type-decidable axes.  The verdict a
  caller actually reads.

  Tests the following, one row per way the meet can land:
  - ONE axis moves, the rest agree            -> that axis's verdict wins
  - TWO axes move the same way                -> that direction survives
  - TWO axes move in OPPOSITE directions      -> incomparable
  - ONE axis is incomparable                  -> incomparable, whatever the rest say
  - every axis agrees                         -> equivalent

  The opposing-directions row is the one that matters.  A product order says "L
  refines R" only if L stands at least as high on EVERY axis; an axis that says
  `less` and an axis that says `greater` mean neither refines the other, and any
  fold that averaged, summed or majority-voted would say otherwise.
*/
bool tests_refinement_composition()
{
    bool ok = true;

    // every axis agrees
    D_CCM_DUAL(order_relation::equivalent, DRuntimeSeq, DRuntimeSeq);
    D_CCM_DUAL(order_relation::equivalent, DRuntimeSeq, DNestedSeq);

    // ONE axis moves: discipline
    D_CCM_DUAL(order_relation::greater, DCapacitySeq, DRuntimeSeq);
    D_CCM_DUAL(order_relation::greater, DSortedSeq,   DRuntimeSeq);
    D_CCM_DUAL(order_relation::greater, DKeyedSeq,    DRuntimeSeq);
    D_CCM_DUAL(order_relation::greater, DUniqueSeq,   DRuntimeSeq);

    // TWO axes move the SAME way: DCompileSeq is more static AND more capable
    // (the stage bit feeds both the lifetime axis and the iterability lattice)
    D_CCM_REL(order_relation::greater,
              (lifetime_relation<DCompileSeq, DRuntimeSeq>::value));
    D_CCM_REL(order_relation::greater,
              (DIterabilityRelation<DCompileSeq, DRuntimeSeq>::value));
    D_CCM_DUAL(order_relation::greater, DCompileSeq, DRuntimeSeq);

    // ONE axis is INCOMPARABLE: discipline
    D_CCM_DUAL(order_relation::incomparable, DSortedSeq, DCapacitySeq);
    D_CCM_DUAL(order_relation::incomparable, DUniqueSeq, DSortedSeq);

    // ONE axis is INCOMPARABLE: iterability (the mixed kinds)
    D_CCM_DUAL(order_relation::incomparable, DCompileConstSeq, DRuntimeSeq);

    // ONE axis is INCOMPARABLE: storage
    D_CCM_DUAL(order_relation::incomparable, static_seq, dynamic_seq);
    D_CCM_DUAL(order_relation::incomparable, static_seq, DRuntimeSeq);

    // ...and the storage veto really is the storage veto: static_seq and
    // DRuntimeSeq agree on discipline, lifetime AND iterability
    D_CCM_REL(order_relation::equivalent,
              (discipline_relation<static_seq, DRuntimeSeq>::value));
    D_CCM_REL(order_relation::equivalent,
              (lifetime_relation<static_seq, DRuntimeSeq>::value));
    D_CCM_REL(order_relation::equivalent,
              (DIterabilityRelation<static_seq, DRuntimeSeq>::value));
    D_CCM_REL(order_relation::incomparable,
              (storage_relation<static_seq, DRuntimeSeq>::value));

    // OPPOSING DIRECTIONS.  DConstSeq is discipline-equal to DCapacitySeq's base
    // and less capable; give it the capacity flag and it is greater on discipline
    // and less on iterability at once
    D_CCM_REL(order_relation::greater,
              (discipline_relation<DCapacitySeq, DRuntimeSeq>::value));
    D_CCM_REL(order_relation::less,
              (DIterabilityRelation<DConstSeq, DRuntimeSeq>::value));
    D_CCM_DUAL(order_relation::less, DConstSeq, DRuntimeSeq);

    return ok;
}

/*
tests_refinement_non_container
  THE FINDING, ESCAPING.

  tests_discipline_non_container shows that a non-container out-disciplines every
  container, because multiplicity_kind_rank ranks `unknown` below `unique`.  It
  also shows what usually rescues the verdict: a non-container cannot be traversed,
  so the iterability axis says `less`, and meet(greater, less) is incomparable.

  Usually.

  Pit a non-container against a container that is ALSO not traversable, and there
  is nothing left to rescue it:

      discipline   greater      (the bogus one)
      lifetime     equivalent   (both runtime)
      iterability  equivalent   (both at the bottom of the lattice)
      storage      equivalent   (both unknown)
      ---------------------------------------------
      refinement   GREATER

  So

      refinement_of<int, DNotIterable>::value          ==  greater
      refinement_of<void, DNotIterable>::value         ==  greater
      realization_comparable<void, DNotIterable>       ==  true

  The module's central trait reports that `void` is a REFINEMENT of a container -
  "stronger discipline, and every realization at least as capable" - and that the
  two are ORDERED, with void the stronger.

  A caller that dispatched on realization_comparable to decide whether a
  substitution is licensed would be told yes.

  Tests the following:
  - the verdict, on the fixtures and on void and int
  - the axis-by-axis reason, so the finding is attributable
  - the CONTENT axis, which is the one thing that does say something sensible:
    content_shared is `none`, because a non-container has no rung.  It is not in
    the refinement, so it cannot veto - but a caller reading the whole PROFILE has
    the information, and a caller reading only refinement_of does not
*/
bool tests_refinement_non_container()
{
    bool ok = true;

    // *** THE VERDICT ***
    D_CCM_REL(order_relation::greater,
              (refinement_of<DNotAContainer, DNotIterable>::value));
    D_CCM_REL(order_relation::greater,
              (refinement_of<int, DNotIterable>::value));
    D_CCM_REL(order_relation::greater,
              (refinement_of<void, DNotIterable>::value));

    // ...and the module says the two are ORDERED
    D_CCM_CHECK((realization_comparable<void, DNotIterable>::value));
    D_CCM_CHECK((realization_comparable<int, DNotIterable>::value));
    D_CCM_CHECK((!realization_equal<int, DNotIterable>::value));

    // the direction, backwards: the CONTAINER is the weaker
    D_CCM_REL(order_relation::less,
              (refinement_of<DNotIterable, int>::value));

    // THE AXIS-BY-AXIS REASON
    D_CCM_REL(order_relation::greater,
              (discipline_relation<DNotAContainer, DNotIterable>::value));
    D_CCM_REL(order_relation::equivalent,
              (lifetime_relation<DNotAContainer, DNotIterable>::value));
    D_CCM_REL(order_relation::equivalent,
              (DIterabilityRelation<DNotAContainer, DNotIterable>::value));
    D_CCM_REL(order_relation::equivalent,
              (storage_relation<DNotAContainer, DNotIterable>::value));

    // nothing is left to veto the bogus `greater`
    D_CCM_CHECK(order_relation_meet(
                    order_relation_meet(
                        order_relation_meet(order_relation::greater,
                                            order_relation::equivalent),
                        order_relation::equivalent),
                    order_relation::equivalent) == order_relation::greater);

    // THE ONE THING THAT DOES SAY SO: the content axis, which is not in the fold
    using p = DComparisonProfile<DNotAContainer, DNotIterable>;

    D_CCM_LEVEL(content_level::none, p::content_left);
    D_CCM_LEVEL(content_level::seq,  p::content_right);
    D_CCM_LEVEL(content_level::none, p::content_shared);

    // a caller reading content_shared knows.  A caller reading refinement_of does
    // not - and refinement_of is the trait the header names as "the verdict"
    D_CCM_CHECK(p::content_shared == content_level::none);
    D_CCM_CHECK(p::refinement == order_relation::greater);

    return ok;
}

/*
tests_realization_equal
  realization_equal - true iff the refinement is `equivalent`: the two types agree
  on discipline and on every realization axis.  The TYPE-DECIDABLE half of
  ``equal''; full equality adds agreement of the contents at the shared rung,
  which is a value-level fact and is not decided here.
  Tests the following:
  - a type against itself
  - two different types whose axes all agree - DNestedSeq and DRuntimeSeq, which
    differ on the CONTENT axis alone and are therefore realization-equal despite
    being obviously different containers.  That is correct, and it is the thing
    the trait's name most invites a reader to get wrong
  - types differing on ANY axis are not realization-equal
  - it is exactly `refinement == equivalent`, restated
*/
bool tests_realization_equal()
{
    bool ok = true;

    // reflexive
    D_CCM_CHECK((realization_equal<DRuntimeSeq, DRuntimeSeq>::value));
    D_CCM_CHECK((realization_equal<DCompileSeq, DCompileSeq>::value));
    D_CCM_CHECK((realization_equal<DNotAContainer, DNotAContainer>::value));
    D_CCM_CHECK((realization_equal<int, int>::value));

    // different TYPES, same realization - the content axis is not consulted
    D_CCM_CHECK((realization_equal<DNestedSeq, DRuntimeSeq>::value));
    D_CCM_CHECK((realization_equal<DRuntimeSeq, DNestedSeq>::value));

    // ...and they really are different on content
    D_CCM_CHECK((native_content_level_of<DNestedSeq>::value !=
                 native_content_level_of<DRuntimeSeq>::value));

    // any axis moving breaks it
    D_CCM_CHECK((!realization_equal<DCompileSeq, DRuntimeSeq>::value));
    D_CCM_CHECK((!realization_equal<DCapacitySeq, DRuntimeSeq>::value));
    D_CCM_CHECK((!realization_equal<DConstSeq, DRuntimeSeq>::value));
    D_CCM_CHECK((!realization_equal<static_seq, DRuntimeSeq>::value));
    D_CCM_CHECK((!realization_equal<DSortedSeq, DCapacitySeq>::value));

    // it IS `refinement == equivalent`
    #define D_CCM_EQ_IS_REF(LEFT, RIGHT)                                      \
        D_CCM_CHECK((realization_equal<LEFT, RIGHT>::value) ==                \
                    ((refinement_of<LEFT, RIGHT>::value) ==                   \
                     order_relation::equivalent))

    D_CCM_EQ_IS_REF(DRuntimeSeq, DRuntimeSeq);
    D_CCM_EQ_IS_REF(DCompileSeq, DRuntimeSeq);
    D_CCM_EQ_IS_REF(DSortedSeq,  DCapacitySeq);
    D_CCM_EQ_IS_REF(DNestedSeq,  DRuntimeSeq);
    D_CCM_EQ_IS_REF(static_seq,  dynamic_seq);
    D_CCM_EQ_IS_REF(int,         std_vector);

    #undef D_CCM_EQ_IS_REF

    return ok;
}

/*
tests_realization_comparable
  realization_comparable - true iff the two are ORDERED (or equal) on the
  refinement: not incomparable.
  Tests the following:
  - equivalent, less and greater are all comparable
  - incomparable is not
  - it is exactly `refinement != incomparable`, restated
  - realization_equal IMPLIES realization_comparable, and not conversely - the two
    are a nested pair, and a caller who used them interchangeably would be
    substituting a strictly weaker container where an equal one was required
*/
bool tests_realization_comparable()
{
    bool ok = true;

    // ordered, in each of the three ordered ways
    D_CCM_CHECK((realization_comparable<DRuntimeSeq, DRuntimeSeq>::value));
    D_CCM_CHECK((realization_comparable<DCompileSeq, DRuntimeSeq>::value));
    D_CCM_CHECK((realization_comparable<DRuntimeSeq, DCompileSeq>::value));
    D_CCM_CHECK((realization_comparable<DCapacitySeq, DRuntimeSeq>::value));

    // NOT ordered
    D_CCM_CHECK((!realization_comparable<DSortedSeq, DCapacitySeq>::value));
    D_CCM_CHECK((!realization_comparable<DCompileConstSeq,
                                         DRuntimeSeq>::value));
    D_CCM_CHECK((!realization_comparable<static_seq, dynamic_seq>::value));
    D_CCM_CHECK((!realization_comparable<DNotAContainer, DRuntimeSeq>::value));

    // it IS `refinement != incomparable`
    #define D_CCM_CMP_IS_REF(LEFT, RIGHT)                                     \
        D_CCM_CHECK((realization_comparable<LEFT, RIGHT>::value) ==           \
                    ((refinement_of<LEFT, RIGHT>::value) !=                   \
                     order_relation::incomparable));                          \
        /* equal implies comparable, never the reverse */                     \
        D_CCM_CHECK(!(realization_equal<LEFT, RIGHT>::value) ||               \
                    (realization_comparable<LEFT, RIGHT>::value))

    D_CCM_CMP_IS_REF(DRuntimeSeq,       DRuntimeSeq);
    D_CCM_CMP_IS_REF(DCompileSeq,       DRuntimeSeq);
    D_CCM_CMP_IS_REF(DCompileConstSeq, DRuntimeSeq);
    D_CCM_CMP_IS_REF(DSortedSeq,        DCapacitySeq);
    D_CCM_CMP_IS_REF(static_seq,        dynamic_seq);
    D_CCM_CMP_IS_REF(DNestedSeq,        DRuntimeSeq);
    D_CCM_CMP_IS_REF(DNotAContainer,   DNotIterable);
    D_CCM_CMP_IS_REF(int,               std_vector);

    #undef D_CCM_CMP_IS_REF

    // comparable does NOT imply equal - the strictly-ordered pair
    D_CCM_CHECK((realization_comparable<DCompileSeq, DRuntimeSeq>::value) &&
                (!realization_equal<DCompileSeq, DRuntimeSeq>::value));

    return ok;
}

/*
tests_profile_value_companions
  The `_v` companions of section V.
  Tests the following:
  - refinement_of_v, realization_equal_v and realization_comparable_v each carry
    the same value as their trait
  - refinement_of_v is typed order_relation, not bool - it cannot be folded into a
    boolean expression by accident, which is precisely the mistake
    realization_comparable exists to prevent
*/
bool tests_profile_value_companions()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_CCM_CHECK((refinement_of_v<DCompileSeq, DRuntimeSeq>) ==
                (refinement_of<DCompileSeq, DRuntimeSeq>::value));
    D_CCM_CHECK((refinement_of_v<DSortedSeq, DCapacitySeq>) ==
                (refinement_of<DSortedSeq, DCapacitySeq>::value));
    D_CCM_CHECK((refinement_of_v<DRuntimeSeq, DRuntimeSeq>) ==
                (refinement_of<DRuntimeSeq, DRuntimeSeq>::value));

    D_CCM_REL(order_relation::greater,
              (refinement_of_v<DCompileSeq, DRuntimeSeq>));
    D_CCM_REL(order_relation::incomparable,
              (refinement_of_v<DSortedSeq, DCapacitySeq>));
    D_CCM_REL(order_relation::equivalent,
              (refinement_of_v<DNestedSeq, DRuntimeSeq>));

    D_CCM_CHECK((realization_equal_v<DNestedSeq, DRuntimeSeq>) ==
                (realization_equal<DNestedSeq, DRuntimeSeq>::value));
    D_CCM_CHECK((realization_equal_v<DCompileSeq, DRuntimeSeq>) ==
                (realization_equal<DCompileSeq, DRuntimeSeq>::value));

    D_CCM_CHECK((realization_comparable_v<DCompileSeq, DRuntimeSeq>) ==
                (realization_comparable<DCompileSeq, DRuntimeSeq>::value));
    D_CCM_CHECK((realization_comparable_v<static_seq, dynamic_seq>) ==
                (realization_comparable<static_seq, dynamic_seq>::value));

    // the shorthands really are the values
    D_CCM_CHECK((realization_equal_v<DRuntimeSeq, DNestedSeq>));
    D_CCM_CHECK((!realization_comparable_v<static_seq, dynamic_seq>));

    // refinement_of_v is an order_relation, the other two are bools
    D_CCM_CHECK((std::is_same<
                    decltype(refinement_of_v<DCompileSeq, DRuntimeSeq>),
                    const order_relation>::value));
    D_CCM_CHECK((std::is_same<
                    decltype(realization_equal_v<DCompileSeq, DRuntimeSeq>),
                    const bool>::value));

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
