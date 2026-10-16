/*******************************************************************************
* djinterp [core]               container_comparison_traits_tests_discipline.cpp
*
*
* path:      /tests/djinterp/core/container/traits/container_comparison_traits_tests_discipline.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <set>
#include <vector>
// djinterp
#include "container_comparison_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_discipline_equivalent
  discipline_relation - mutual subsumption is discipline-EQUALITY.
  Tests the following:
  - a type against itself
  - two DIFFERENT types with the same overlay: nested_seq wears exactly
    runtime_seq's restrictions (nesting is structural and carries none), so their
    disciplines are equal even though their content rungs are not
  - equivalence really is MUTUAL subsumption, checked against overlay_subsumes
    directly rather than assumed
*/
bool
tests_discipline_equivalent()
{
    bool ok = true;

    D_CCM_REL(order_relation::equivalent,
              (discipline_relation<runtime_seq, runtime_seq>::value));
    D_CCM_REL(order_relation::equivalent,
              (discipline_relation<unique_seq, unique_seq>::value));
    D_CCM_REL(order_relation::equivalent,
              (discipline_relation<sorted_seq, sorted_seq>::value));

    // different types, same overlay
    D_CCM_REL(order_relation::equivalent,
              (discipline_relation<nested_seq, runtime_seq>::value));
    D_CCM_REL(order_relation::equivalent,
              (discipline_relation<runtime_seq, nested_seq>::value));

    // it IS mutual subsumption, restated
    D_CCM_CHECK(overlay_subsumes(overlay_of<nested_seq>::value(),
                                 overlay_of<runtime_seq>::value()));
    D_CCM_CHECK(overlay_subsumes(overlay_of<runtime_seq>::value(),
                                 overlay_of<nested_seq>::value()));

    // the two multiset kinds are NOT discipline-equal, though they share a content
    // rung: bounded_bag's multiplicity is strictly tighter
    D_CCM_REL(order_relation::greater,
              (discipline_relation<bounded_bag, unbounded_bag>::value));

    // A SECOND FINDING.  An UNBOUNDED MULTISET and a SEQUENCE are DISCIPLINE-EQUAL.
    //
    //   multiplicity_kind_rank collapses them:
    //
    //       sequence            -> 2
    //       unbounded_multiset  -> 2
    //
    // because both have m = inf.  But the two kinds are not the same restriction:
    // a multiset has m = inf UNDER A GENUINE EQUIVALENCE, and a sequence has no
    // equivalence at all (identity by default).  The rank measures the multiplicity
    // BOUND and erases whether there is an equivalence to be bounded.
    //
    //   So a multiset's equivalence relation - the whole thing that makes it a
    // multiset rather than a list - contributes NOTHING to the discipline order,
    // and std::multiset comes out discipline-equal to std::vector.  A caller
    // dispatching on the discipline to decide whether a substitution preserves
    // semantics would be told the two are interchangeable.
    //
    //   The rank is doing two jobs - ordering the bounds AND separating the kinds -
    // and it can only do one.  A second component (has_equivalence) in
    // overlay_subsumes would separate them; a rank alone cannot.
    D_CCM_CHECK(multiplicity_kind_rank(multiplicity_kind::sequence) ==
                multiplicity_kind_rank(multiplicity_kind::unbounded_multiset));
    D_CCM_CHECK(overlay_of<unbounded_bag>::mult !=
                overlay_of<runtime_seq>::mult);
    D_CCM_REL(order_relation::equivalent,
              (discipline_relation<unbounded_bag, runtime_seq>::value));
    D_CCM_REL(order_relation::equivalent,
              (discipline_relation<runtime_seq, unbounded_bag>::value));

    // ...and their CONTENT rungs do differ, so the information is not lost - it is
    // simply not on the discipline axis
    D_CCM_CHECK((native_content_level_of<unbounded_bag>::value ==
                 content_level::bag));
    D_CCM_CHECK((native_content_level_of<runtime_seq>::value ==
                 content_level::seq));

    return ok;
}

/*
tests_discipline_ordered
  discipline_relation - `greater` is the LEFT more restrictive: it wears every
  restriction the right does, and more.  There are two ways to be more
  restrictive, and there is a lever for each.
  Tests the following:
  - a TIGHTER MULTIPLICITY: unique (rank 0) subsumes bounded_multiset (rank 1)
    subsumes sequence (rank 2).  A three-link chain, which is what makes
    transitivity checkable at all
  - a SUPERSET OF FLAGS at the same multiplicity: sorted_seq, capacity_seq and
    keyed_seq each wear one restriction runtime_seq does not, and each is therefore
    strictly greater than it
  - `less` is the same relation read backwards - checked at every site, because a
    ternary chain that got the direction wrong would still get every INDIVIDUAL
    verdict "right" in isolation
*/
bool
tests_discipline_ordered()
{
    bool ok = true;

    #define D_CCM_DISC(EXPECTED, LEFT, RIGHT)                                 \
        D_CCM_REL((EXPECTED), (discipline_relation<LEFT, RIGHT>::value));     \
        D_CCM_REL(ccm_dual(EXPECTED), (discipline_relation<RIGHT, LEFT>::value))

    // a tighter multiplicity: the chain unique < bag < sequence, by rank
    D_CCM_DISC(order_relation::greater, unique_seq,   bounded_bag);
    D_CCM_DISC(order_relation::greater, bounded_bag,  runtime_seq);
    D_CCM_DISC(order_relation::greater, unique_seq,   runtime_seq);
    D_CCM_DISC(order_relation::greater, unique_seq,   unbounded_bag);

    // a superset of flags, at the same multiplicity
    D_CCM_DISC(order_relation::greater, sorted_seq,   runtime_seq);
    D_CCM_DISC(order_relation::greater, capacity_seq, runtime_seq);
    D_CCM_DISC(order_relation::greater, keyed_seq,    runtime_seq);

    #undef D_CCM_DISC

    // the flags really are what does it
    D_CCM_CHECK(overlay_of<sorted_seq>::sorted);
    D_CCM_CHECK(!overlay_of<runtime_seq>::sorted);
    D_CCM_CHECK(overlay_of<capacity_seq>::capacity_bounded);
    D_CCM_CHECK(!overlay_of<runtime_seq>::capacity_bounded);
    D_CCM_CHECK(overlay_of<keyed_seq>::keyed);
    D_CCM_CHECK(!overlay_of<runtime_seq>::keyed);

    // ...and the multiplicity ranks really are what does the other
    D_CCM_CHECK(multiplicity_kind_rank(overlay_of<unique_seq>::mult) <
                multiplicity_kind_rank(overlay_of<bounded_bag>::mult));
    D_CCM_CHECK(multiplicity_kind_rank(overlay_of<bounded_bag>::mult) <
                multiplicity_kind_rank(overlay_of<runtime_seq>::mult));

    return ok;
}

/*
tests_discipline_incomparable
  discipline_relation - `incomparable` is a genuine mismatch: each type wears a
  restriction the other does not, so neither overlay contains the other.

  This is the outcome that makes the discipline a PREORDER rather than a chain,
  and it is the outcome most easily lost: a relation implemented as a rank
  comparison, or as a bit-count, would order every pair of overlays and would
  never report it.

  Tests the following:
  - two DIFFERENT FLAGS at the same multiplicity: sorted_seq is sorted and not
    capacity-bounded; capacity_seq is the reverse.  Neither subsumes the other
  - a TIGHTER MULTIPLICITY against a FLAG: unique_seq is tighter on mu and bears no
    sorted invariant; sorted_seq is looser on mu and does.  Neither subsumes the
    other, and this is the case a rank-only comparison would call `greater`
  - three-way: keyed against sorted, keyed against capacity
*/
bool
tests_discipline_incomparable()
{
    bool ok = true;

    #define D_CCM_DISC_IC(LEFT, RIGHT)                                        \
        D_CCM_REL(order_relation::incomparable,                               \
                  (discipline_relation<LEFT, RIGHT>::value));                 \
        D_CCM_REL(order_relation::incomparable,                               \
                  (discipline_relation<RIGHT, LEFT>::value))

    // different flags, same multiplicity
    D_CCM_DISC_IC(sorted_seq,   capacity_seq);
    D_CCM_DISC_IC(sorted_seq,   keyed_seq);
    D_CCM_DISC_IC(capacity_seq, keyed_seq);

    // a tighter multiplicity against a flag the other lacks
    D_CCM_DISC_IC(unique_seq, sorted_seq);
    D_CCM_DISC_IC(unique_seq, capacity_seq);
    D_CCM_DISC_IC(unique_seq, keyed_seq);
    D_CCM_DISC_IC(bounded_bag, sorted_seq);

    #undef D_CCM_DISC_IC

    // ...and it really is a two-sided failure of subsumption, not a one-sided one
    D_CCM_CHECK(!overlay_subsumes(overlay_of<unique_seq>::value(),
                                  overlay_of<sorted_seq>::value()));
    D_CCM_CHECK(!overlay_subsumes(overlay_of<sorted_seq>::value(),
                                  overlay_of<unique_seq>::value()));

    return ok;
}

/*
tests_discipline_non_container
  THE FINDING.  A non-container has a STRONGER DISCIPLINE than std::set.

      discipline_relation<int, std_set>::value   ==   greater
      discipline_relation<void, std_vector>      ==   greater
      discipline_relation<not_a_container, runtime_seq> ==  greater

  THE MECHANISM.  overlay_subsumes reads its multiplicity component as

      multiplicity_kind_rank(_a.mult) <= multiplicity_kind_rank(_b.mult)

  i.e. "a's multiplicity is at least as TIGHT".  And multiplicity_kind_rank is

      unknown            -> -1
      unique             ->  0
      bounded_multiset   ->  1
      sequence           ->  2
      unbounded_multiset ->  2

  `unknown` is ranked BELOW `unique`, which is to say TIGHTER THAN THE TIGHTEST.
  But `unknown` does not mean "m = 0" or "an even stronger equivalence" - it means
  "not a container / indeterminate", and it is not on the multiplicity scale at
  all.  Placing it at the bottom of a scale that reads low as tight makes every
  non-container subsume every container on that component; and since a
  non-container also bears none of the four flag-restrictions, and the flag clauses
  are vacuous when the RIGHT operand bears none either, the whole subsumption goes
  through.

  THE FIX is one of three, and they are not equivalent:
    (a) rank `unknown` ABOVE sequence, making it the loosest.  Cheapest, and wrong
        in the other direction - it would make a container subsume a non-container
    (b) have overlay_subsumes return false whenever either multiplicity is unknown
        and the other is not.  `unknown` is then INCOMPARABLE with every real
        multiplicity, which is what it means
    (c) gate discipline_relation on containerhood before consulting the overlay

  (b) is the honest one: the defect is in the RANK's pretence that `unknown` is a
  point on the scale, and subsumption is the only place it matters.

  Tests the following:
  - the relation, on the fixtures and on the standard library
  - the mechanism, component by component, so the finding is attributable and not
    merely observed
  - the direction: the CONTAINER is `less` than the non-container, which is the
    same statement read backwards and no less wrong
  - what it does NOT break: the refinement usually rescues it, because a
    non-container cannot be traversed and the iterability axis says `less`, and
    meet(greater, less) is incomparable.  Usually.  See
    tests_refinement_non_container for the case where it does not
*/
bool
tests_discipline_non_container()
{
    bool ok = true;

    // THE FINDING
    D_CCM_REL(order_relation::greater,
              (discipline_relation<not_a_container, runtime_seq>::value));
    D_CCM_REL(order_relation::greater,
              (discipline_relation<int, std_set>::value));
    D_CCM_REL(order_relation::greater,
              (discipline_relation<void, std_vector>::value));
    D_CCM_REL(order_relation::greater,
              (discipline_relation<fixtures::empty, unique_seq>::value));

    // ...and the same statement backwards: the CONTAINER is the weaker
    D_CCM_REL(order_relation::less,
              (discipline_relation<std_set, int>::value));
    D_CCM_REL(order_relation::less,
              (discipline_relation<runtime_seq, not_a_container>::value));

    // THE MECHANISM, component by component
    D_CCM_CHECK(overlay_of<not_a_container>::mult == multiplicity_kind::unknown);
    D_CCM_CHECK(multiplicity_kind_rank(multiplicity_kind::unknown) == -1);
    D_CCM_CHECK(multiplicity_kind_rank(multiplicity_kind::unique) == 0);
    D_CCM_CHECK(multiplicity_kind_rank(multiplicity_kind::unknown) <
                multiplicity_kind_rank(multiplicity_kind::unique));

    // the non-container bears no flags, so every flag clause is vacuous
    D_CCM_CHECK(!overlay_of<not_a_container>::keyed);
    D_CCM_CHECK(!overlay_of<not_a_container>::sorted);
    D_CCM_CHECK(!overlay_of<not_a_container>::capacity_bounded);
    D_CCM_CHECK(!overlay_of<not_a_container>::domain_bounded);

    // ...so the subsumption goes through, one way and not the other
    D_CCM_CHECK(overlay_subsumes(overlay_of<not_a_container>::value(),
                                 overlay_of<runtime_seq>::value()));
    D_CCM_CHECK(!overlay_subsumes(overlay_of<runtime_seq>::value(),
                                  overlay_of<not_a_container>::value()));

    // TWO non-containers are of course discipline-equal, which is the one row of
    // the finding that is not embarrassing
    D_CCM_REL(order_relation::equivalent,
              (discipline_relation<int, void>::value));
    D_CCM_REL(order_relation::equivalent,
              (discipline_relation<not_a_container, fixtures::empty>::value));

    // WHAT SAVES IT, usually: a non-container cannot be traversed
    D_CCM_REL(order_relation::less,
              (iterability_relation<not_a_container, runtime_seq>::value));
    D_CCM_REL(order_relation::incomparable,
              (refinement_of<not_a_container, runtime_seq>::value));

    return ok;
}


NS_END  // testing
NS_END  // djinterp
