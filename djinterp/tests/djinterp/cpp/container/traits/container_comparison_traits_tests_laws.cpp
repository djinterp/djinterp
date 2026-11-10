/*******************************************************************************
* djinterp [cpp]                      container_comparison_traits_tests_laws.cpp
*
*
* path:      /tests/djinterp/cpp/container/traits/container_comparison_traits_tests_laws.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/
// std
#include <array>
#include <list>
#include <set>
#include <vector>
// djinterp
#include "container_comparison_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                BUILD-TIME PINS                                           ///
///////////////////////////////////////////////////////////////////////////////
//   The laws whose regression should stop the line.  Every other check in this
// suite is a statement about a particular PAIR of types; these are statements
// about the ORDER, and if the order is broken then every pairwise verdict is
// meaningless - so there is nothing worth reporting and a dead build is the
// honest outcome.

// reflexivity: nothing can refine itself more than equally
D_TEST_STATIC(refinement_of<DRuntimeSeq, DRuntimeSeq>::value ==
              order_relation::equivalent);

// the meet's unit
D_TEST_STATIC(order_relation_meet(order_relation::equivalent,
                                  order_relation::greater) ==
              order_relation::greater);

// the meet's zero
D_TEST_STATIC(order_relation_meet(order_relation::incomparable,
                                  order_relation::greater) ==
              order_relation::incomparable);

// opposing directions do NOT average out
D_TEST_STATIC(order_relation_meet(order_relation::less,
                                  order_relation::greater) ==
              order_relation::incomparable);


/*
tests_law_reflexivity
  REFLEXIVITY.  Every type is realization-equal to itself, on every axis and
  therefore on the refinement.

  Trivial to state, easy to break, and impossible to notice when broken: a
  relation that returned `greater` for T against T would still give sensible-
  looking answers for every DISTINCT pair, and would only surface as a subtle
  wrongness in whatever downstream code assumed the order was reflexive.

  Tests the following:
  - refinement_of<T, T> is `equivalent`, and realization_equal<T, T> is true, for
    every fixture in the suite
  - each of the four AXES is reflexive too, separately - so a failure is
    attributable to the axis that broke it rather than to the meet
  - and over the whole hostile zoo, which is the only way to state it as a
    property of the trait rather than a list of instances
*/
bool tests_law_reflexivity()
{
    bool ok = true;

    #define D_CCM_REFLEXIVE(...)                                              \
        D_CCM_REL(order_relation::equivalent,                                 \
                  (discipline_relation<__VA_ARGS__, __VA_ARGS__>::value));                  \
        D_CCM_REL(order_relation::equivalent,                                 \
                  (lifetime_relation<__VA_ARGS__, __VA_ARGS__>::value));                    \
        D_CCM_REL(order_relation::equivalent,                                 \
                  (DIterabilityRelation<__VA_ARGS__, __VA_ARGS__>::value));                 \
        D_CCM_REL(order_relation::equivalent,                                 \
                  (storage_relation<__VA_ARGS__, __VA_ARGS__>::value));                     \
        D_CCM_REL(order_relation::equivalent,                                 \
                  (refinement_of<__VA_ARGS__, __VA_ARGS__>::value));                        \
        D_CCM_CHECK((realization_equal<__VA_ARGS__, __VA_ARGS__>::value));                  \
        D_CCM_CHECK((realization_comparable<__VA_ARGS__, __VA_ARGS__>::value))

    D_CCM_REFLEXIVE(DRuntimeSeq);
    D_CCM_REFLEXIVE(DCompileSeq);
    D_CCM_REFLEXIVE(DCompileConstSeq);
    D_CCM_REFLEXIVE(DConstSeq);
    D_CCM_REFLEXIVE(DNotIterable);
    D_CCM_REFLEXIVE(DNotAContainer);
    D_CCM_REFLEXIVE(DUniqueSeq);
    D_CCM_REFLEXIVE(DBoundedBag);
    D_CCM_REFLEXIVE(DUnboundedBag);
    D_CCM_REFLEXIVE(DSortedSeq);
    D_CCM_REFLEXIVE(DCapacitySeq);
    D_CCM_REFLEXIVE(DKeyedSeq);
    D_CCM_REFLEXIVE(DNestedSeq);
    D_CCM_REFLEXIVE(DHierarchicalUnique);
    D_CCM_REFLEXIVE(static_seq);
    D_CCM_REFLEXIVE(dynamic_seq);
    D_CCM_REFLEXIVE(DHybridSeq);
    D_CCM_REFLEXIVE(int);
    D_CCM_REFLEXIVE(void);
    D_CCM_REFLEXIVE(std_vector);
    D_CCM_REFLEXIVE(std_array);
    D_CCM_REFLEXIVE(std_set);

    #undef D_CCM_REFLEXIVE

    return ok;
}

/*
tests_law_direction
  THE DIRECTION CONTRACT.  "A relation is read of the LEFT operand against the
  right."  Swapping the operands must therefore INVERT an ordering and preserve
  everything else:

      refinement_of<A, B> == greater      <=>   refinement_of<B, A> == less
      refinement_of<A, B> == equivalent   <=>   refinement_of<B, A> == equivalent
      refinement_of<A, B> == incomparable <=>   refinement_of<B, A> == incomparable

  This is the law a ternary chain breaks with a single transposed operand, and it
  is the law that every INDIVIDUAL cell test passes anyway: a chain that reported
  `greater` in both directions would look right in every one-sided check ever
  written.

  Tests the following:
  - the duality holds for the refinement, over every fixture pair
  - it holds for each of the four axes SEPARATELY, so a failure names the axis
  - the equivalence axis (storage) is trivially self-dual - it has no ordering to
    invert - and is checked anyway, because "trivially" is how these things get
    left out
*/
bool tests_law_direction()
{
    bool ok = true;

    #define D_CCM_DUAL_AXES(LEFT, RIGHT)                                      \
        D_CCM_CHECK((discipline_relation<RIGHT, LEFT>::value) ==              \
                    ccm_dual(discipline_relation<LEFT, RIGHT>::value));       \
        D_CCM_CHECK((lifetime_relation<RIGHT, LEFT>::value) ==                \
                    ccm_dual(lifetime_relation<LEFT, RIGHT>::value));         \
        D_CCM_CHECK((DIterabilityRelation<RIGHT, LEFT>::value) ==             \
                    ccm_dual(DIterabilityRelation<LEFT, RIGHT>::value));      \
        D_CCM_CHECK((storage_relation<RIGHT, LEFT>::value) ==                 \
                    ccm_dual(storage_relation<LEFT, RIGHT>::value));          \
        D_CCM_CHECK((refinement_of<RIGHT, LEFT>::value) ==                    \
                    ccm_dual(refinement_of<LEFT, RIGHT>::value))

    // ordered pairs
    D_CCM_DUAL_AXES(DCompileSeq,  DRuntimeSeq);
    D_CCM_DUAL_AXES(DCapacitySeq, DRuntimeSeq);
    D_CCM_DUAL_AXES(DUniqueSeq,   DBoundedBag);
    D_CCM_DUAL_AXES(DBoundedBag,  DRuntimeSeq);
    D_CCM_DUAL_AXES(DConstSeq,    DRuntimeSeq);
    D_CCM_DUAL_AXES(DNotIterable, DRuntimeSeq);
    D_CCM_DUAL_AXES(DKeyedSeq,    DRuntimeSeq);
    D_CCM_DUAL_AXES(DSortedSeq,   DRuntimeSeq);

    // equivalent pairs
    D_CCM_DUAL_AXES(DRuntimeSeq,  DNestedSeq);
    D_CCM_DUAL_AXES(DRuntimeSeq,  DRuntimeSeq);

    // incomparable pairs
    D_CCM_DUAL_AXES(DSortedSeq,        DCapacitySeq);
    D_CCM_DUAL_AXES(DCompileConstSeq, DRuntimeSeq);
    D_CCM_DUAL_AXES(static_seq,        dynamic_seq);
    D_CCM_DUAL_AXES(DUniqueSeq,        DSortedSeq);
    D_CCM_DUAL_AXES(DNotAContainer,   DRuntimeSeq);

    // the finding, which obeys the law while being wrong - a broken order and a
    // broken RELATION are different failures, and this suite catches both
    D_CCM_DUAL_AXES(DNotAContainer, DNotIterable);

    // the standard library
    D_CCM_DUAL_AXES(std_array, std_vector);
    D_CCM_DUAL_AXES(std_set,      std_vector);
    D_CCM_DUAL_AXES(int,                std_vector);

    #undef D_CCM_DUAL_AXES

    // ccm_dual is itself an involution, which is what makes the above a law and
    // not a coincidence
    D_CCM_CHECK(ccm_dual(ccm_dual(order_relation::greater)) ==
                order_relation::greater);
    D_CCM_CHECK(ccm_dual(ccm_dual(order_relation::less)) ==
                order_relation::less);
    D_CCM_CHECK(ccm_dual(order_relation::equivalent) ==
                order_relation::equivalent);
    D_CCM_CHECK(ccm_dual(order_relation::incomparable) ==
                order_relation::incomparable);

    return ok;
}

/*
tests_law_transitivity
  TRANSITIVITY.  If A refines B and B refines C, then A refines C.

  The refinement is a product of preorders, so transitivity is inherited - but
  "inherited" is an argument, not a test, and it is inherited only if each axis is
  itself transitive.  The discipline axis is the one that could fail: it is a
  conjunction over a multiplicity RANK and four flags, and a rank comparison is
  transitive while a badly written flag clause need not be.

  Tests the following:
  - a three-link DISCIPLINE chain: DUniqueSeq (mu rank 0) > DBoundedBag (1) >
    DRuntimeSeq (2), all other axes held equal.  Each link is `greater`, and so is
    the composite
  - the chain composes through the REFINEMENT, not merely through the axis
  - a chain that mixes axes: DUniqueSeq > DRuntimeSeq on discipline, and
    DCompileSeq > DRuntimeSeq on lifetime+iterability.  The two are incomparable
    with each other, which is what a product order is for - transitivity does not
    mean everything is ordered
  - transitivity of EQUIVALENCE: realization_equal is transitive, so a chain of
    equal types is equal end to end
*/
bool tests_law_transitivity()
{
    bool ok = true;

    // the discipline chain, link by link
    D_CCM_REL(order_relation::greater,
              (refinement_of<DUniqueSeq, DBoundedBag>::value));
    D_CCM_REL(order_relation::greater,
              (refinement_of<DBoundedBag, DRuntimeSeq>::value));

    // ...and the composite
    D_CCM_REL(order_relation::greater,
              (refinement_of<DUniqueSeq, DRuntimeSeq>::value));

    // stated as the law
    D_CCM_CHECK(!((refinement_of<DUniqueSeq, DBoundedBag>::value ==
                   order_relation::greater) &&
                  (refinement_of<DBoundedBag, DRuntimeSeq>::value ==
                   order_relation::greater)) ||
                (refinement_of<DUniqueSeq, DRuntimeSeq>::value ==
                 order_relation::greater));

    // a longer chain, adding a flag at the top: DCapacitySeq bears a restriction
    // DRuntimeSeq does not, and DUniqueSeq's multiplicity is tighter than both -
    // but DUniqueSeq does NOT bear the capacity flag, so it does not subsume
    // DCapacitySeq.  Transitivity holds; totality does not, and must not
    D_CCM_REL(order_relation::greater,
              (refinement_of<DCapacitySeq, DRuntimeSeq>::value));
    D_CCM_REL(order_relation::greater,
              (refinement_of<DUniqueSeq, DRuntimeSeq>::value));
    D_CCM_REL(order_relation::incomparable,
              (refinement_of<DUniqueSeq, DCapacitySeq>::value));

    // transitivity of the ORDER, on the iterability lattice: (T,T) > (F,T) > (F,F)
    D_CCM_REL(order_relation::greater,
              (DIterabilityRelation<DCompileSeq, DRuntimeSeq>::value));
    D_CCM_REL(order_relation::greater,
              (DIterabilityRelation<DRuntimeSeq, DConstSeq>::value));
    D_CCM_REL(order_relation::greater,
              (DIterabilityRelation<DCompileSeq, DConstSeq>::value));

    // ...and through the bottom: (T,T) > (F,T) > bottom
    D_CCM_REL(order_relation::greater,
              (DIterabilityRelation<DRuntimeSeq, DNotIterable>::value));
    D_CCM_REL(order_relation::greater,
              (DIterabilityRelation<DCompileSeq, DNotIterable>::value));

    // transitivity of EQUIVALENCE
    D_CCM_CHECK((realization_equal<DRuntimeSeq, DNestedSeq>::value));
    D_CCM_CHECK((realization_equal<DNestedSeq, DRuntimeSeq>::value));
    D_CCM_CHECK((realization_equal<DRuntimeSeq, DRuntimeSeq>::value));

    return ok;
}

/*
tests_law_content_excluded
  CONTENT IS NOT IN THE REFINEMENT - and that is deliberate.

  The header is explicit: the content =L between two container VALUES reads their
  contents and is a value-level operation.  What a TYPE fixes is the RUNG at which
  such a comparison could happen, and the module reports the rung and folds only
  the four type-decidable axes into the verdict.

  So two types can differ on the content axis and still be REALIZATION-EQUAL, and
  the suite must show it rather than assert it - because the alternative reading
  ("obviously a nested container is a refinement of a flat one") is the natural
  one, and it is wrong.

  DNestedSeq is the fixture that makes the point: nesting is a structural fact and
  wears NO overlay restriction, so its discipline, lifetime, iterability and
  storage are DRuntimeSeq's, exactly.  The two differ on content and on nothing
  else in the fold.

  Tests the following:
  - the two content rungs differ - str against seq
  - all four fold axes agree
  - so the refinement is `equivalent` and realization_equal is TRUE
  - and the profile still REPORTS the difference, in content_left, content_right
    and content_shared.  The information is not lost; it is simply not folded
  - the same for DHierarchicalUnique against DUniqueSeq, which differ on content
    (str against set) AND agree on every fold axis
*/
bool tests_law_content_excluded()
{
    bool ok = true;

    // the content rungs differ
    D_CCM_LEVEL(content_level::str, native_content_level_of<DNestedSeq>::value);
    D_CCM_LEVEL(content_level::seq, native_content_level_of<DRuntimeSeq>::value);
    D_CCM_CHECK((native_content_level_of<DNestedSeq>::value !=
                 native_content_level_of<DRuntimeSeq>::value));

    // every FOLD axis agrees
    D_CCM_REL(order_relation::equivalent,
              (discipline_relation<DNestedSeq, DRuntimeSeq>::value));
    D_CCM_REL(order_relation::equivalent,
              (lifetime_relation<DNestedSeq, DRuntimeSeq>::value));
    D_CCM_REL(order_relation::equivalent,
              (DIterabilityRelation<DNestedSeq, DRuntimeSeq>::value));
    D_CCM_REL(order_relation::equivalent,
              (storage_relation<DNestedSeq, DRuntimeSeq>::value));

    // ...so the verdict is EQUIVALENT
    D_CCM_REL(order_relation::equivalent,
              (refinement_of<DNestedSeq, DRuntimeSeq>::value));
    D_CCM_CHECK((realization_equal<DNestedSeq, DRuntimeSeq>::value));

    // and the information is REPORTED, not lost
    using p = DComparisonProfile<DNestedSeq, DRuntimeSeq>;

    D_CCM_LEVEL(content_level::str, p::content_left);
    D_CCM_LEVEL(content_level::seq, p::content_right);
    D_CCM_LEVEL(content_level::seq, p::content_shared);   // the coarser
    D_CCM_REL(order_relation::equivalent, p::refinement);

    // the same, one rung further apart: str against set
    D_CCM_LEVEL(content_level::str,
                native_content_level_of<DHierarchicalUnique>::value);
    D_CCM_LEVEL(content_level::set,
                native_content_level_of<DUniqueSeq>::value);
    D_CCM_REL(order_relation::equivalent,
              (refinement_of<DHierarchicalUnique, DUniqueSeq>::value));
    D_CCM_CHECK((realization_equal<DHierarchicalUnique, DUniqueSeq>::value));

    using q = DComparisonProfile<DHierarchicalUnique, DUniqueSeq>;

    D_CCM_LEVEL(content_level::set, q::content_shared);
    D_CCM_REL(order_relation::equivalent, q::refinement);

    // and the real thing: a vector of vectors is realization-equal to a vector
    D_CCM_LEVEL(content_level::str,
                native_content_level_of<std_vec_vec>::value);
    D_CCM_LEVEL(content_level::seq,
                native_content_level_of<std_vector>::value);
    D_CCM_CHECK((realization_equal<std_vec_vec,
                                   std_vector>::value));

    return ok;
}

/*
tests_axis_coupling
  THE AXES ARE NOT INDEPENDENT, and the product order assumes they are.

  The iterability lattice's compile-reachable bit is read from
  internal::is_compile_stage_helper.  So is the entire lifetime axis.  It is the
  SAME BIT:

      lifetime_relation      reads  is_compile_stage_helper<L> vs <R>
      DIterabilityRelation   reads  is_compile_stage_helper<L> vs <R>   (the stage
                                    component of the product)

  The header states the reason - "the compile-reachable bit is read from the
  Lifetime axis, per the formal entailment that a compile-time-fixed container
  traverses at compile time" - and the entailment is sound.  But the CONSEQUENCE is
  that the refinement DOUBLE-COUNTS staticity: a type that is more static is
  necessarily at least as high on iterability's stage component, so the two axes
  can never disagree ABOUT STATICITY, and the meet of the two can never be
  incomparable FOR THAT REASON.

  That is not a bug.  It is a structural fact worth pinning, because it means:
    - the four "independent" axes are really 3.5
    - a refinement that is `greater` on lifetime alone is `greater` on iterability
      too, unless the MODE bit disagrees.  So `greater` on lifetime and `less` on
      iterability is IMPOSSIBLE - the only iterability verdict that can veto a
      lifetime `greater` is `incomparable`, and it does so through the mode bit
    - and a future change that gave iterability its own compile-reachability
      signal (a constexpr_begin(), say, on a runtime-lifetime container) would
      DECOUPLE them, and the refinement would start reporting orderings it does not
      report today

  Tests the following:
  - the two axes read the same bit, checked directly
  - lifetime `greater` and iterability `less` never co-occur, over the fixture set
  - the ONE way iterability vetoes a lifetime ordering is through the mode bit, and
    the verdict is `incomparable`, not `less`
*/
bool tests_axis_coupling()
{
    bool ok = true;

    // the same bit, read twice
    #define D_CCM_SAME_BIT(TYPE)                                              \
        D_CCM_CHECK(is_compile_time(container_lifetime_of<TYPE>::value) ==       \
                    (::djinterp::internal::is_compile_stage_helper<TYPE>      \
                         ::value))

    D_CCM_SAME_BIT(DRuntimeSeq);
    D_CCM_SAME_BIT(DCompileSeq);
    D_CCM_SAME_BIT(DCompileConstSeq);
    D_CCM_SAME_BIT(DConstSeq);
    D_CCM_SAME_BIT(DNotAContainer);

    #undef D_CCM_SAME_BIT

    // lifetime `greater` + iterability `less` is IMPOSSIBLE.  Stated over every
    // pair the suite has: whenever lifetime orders one way, iterability never
    // orders the other
    #define D_CCM_NO_STAGE_CONFLICT(LEFT, RIGHT)                              \
        D_CCM_CHECK(!((lifetime_relation<LEFT, RIGHT>::value ==               \
                       order_relation::greater) &&                            \
                      (DIterabilityRelation<LEFT, RIGHT>::value ==            \
                       order_relation::less)));                               \
        D_CCM_CHECK(!((lifetime_relation<LEFT, RIGHT>::value ==               \
                       order_relation::less) &&                               \
                      (DIterabilityRelation<LEFT, RIGHT>::value ==            \
                       order_relation::greater)))

    D_CCM_NO_STAGE_CONFLICT(DCompileSeq,       DRuntimeSeq);
    D_CCM_NO_STAGE_CONFLICT(DCompileConstSeq, DRuntimeSeq);
    D_CCM_NO_STAGE_CONFLICT(DCompileConstSeq, DConstSeq);
    D_CCM_NO_STAGE_CONFLICT(DCompileSeq,       DConstSeq);
    D_CCM_NO_STAGE_CONFLICT(DRuntimeSeq,       DConstSeq);
    D_CCM_NO_STAGE_CONFLICT(DCompileSeq,       DNotIterable);
    D_CCM_NO_STAGE_CONFLICT(DNotAContainer,   DCompileSeq);

    #undef D_CCM_NO_STAGE_CONFLICT

    // the ONE way iterability vetoes a lifetime ordering: through the MODE bit,
    // and the verdict is `incomparable`
    D_CCM_REL(order_relation::greater,
              (lifetime_relation<DCompileConstSeq, DRuntimeSeq>::value));
    D_CCM_REL(order_relation::incomparable,
              (DIterabilityRelation<DCompileConstSeq, DRuntimeSeq>::value));
    D_CCM_REL(order_relation::incomparable,
              (refinement_of<DCompileConstSeq, DRuntimeSeq>::value));

    // ...and when the mode bits AGREE, the two axes agree too, and the meet is a
    // no-op
    D_CCM_REL(order_relation::greater,
              (lifetime_relation<DCompileSeq, DRuntimeSeq>::value));
    D_CCM_REL(order_relation::greater,
              (DIterabilityRelation<DCompileSeq, DRuntimeSeq>::value));
    D_CCM_CHECK(order_relation_meet(
                    lifetime_relation<DCompileSeq, DRuntimeSeq>::value,
                    DIterabilityRelation<DCompileSeq, DRuntimeSeq>::value) ==
                order_relation::greater);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
