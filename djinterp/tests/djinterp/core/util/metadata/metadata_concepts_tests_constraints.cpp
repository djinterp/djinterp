/*******************************************************************************
* djinterp [core]                        metadata_concepts_tests_constraints.cpp
*
*
* path:      /tests/djinterp/core/util/metadata/metadata_concepts_tests_constraints.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "metadata_concepts_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_metadata_concepts_constrained_use
  The concepts work where concepts are meant to be used.
  Tests the following:
  - a function template constrained on the unifying concept accepts a type
    satisfying any of the three shapes, and REJECTS one that satisfies
    none.  The rejection is stated through a detection probe rather than
    by writing a call that fails to compile, so the negative case is an
    assertion the runner can report
  - constraint checking happens during overload resolution, before the
    body is instantiated, so admissibility is a property of the type alone
  - the results are available in a constant expression, which is what lets
    a constrained call be used in a template argument
  A concept evaluated only in a bool context has not been exercised as a
  concept; these checks put them on a signature.
*/
bool
tests_metadata_concepts_constrained_use()
{
    // the constrained function accepts each shape
    D_MC_CHECK(mc_accepts(mc_member{}) == 7);
    D_MC_CHECK(mc_accepts(mc_method{}) == 7);
    D_MC_CHECK(mc_accepts(mc_nested{}) == 7);
    D_MC_CHECK(mc_accepts(mc_all_names{}) == 7);

    // admissibility is decided by the constraint, not by the body
    D_MC_CHECK(mc_is_admissible<mc_member>::value);
    D_MC_CHECK(mc_is_admissible<mc_method>::value);
    D_MC_CHECK(mc_is_admissible<mc_nested>::value);
    D_MC_CHECK(mc_is_admissible<mc_callable_member>::value);

    // ... and a non-conforming type is genuinely rejected
    D_MC_CHECK(!mc_is_admissible<mc_none>::value);
    D_MC_CHECK(!mc_is_admissible<mc_nonconst_method>::value);
    D_MC_CHECK(!mc_is_admissible<int>::value);
    D_MC_CHECK(!mc_is_admissible<mct_nested>::value);

    // admissibility tracks the concept exactly
    D_MC_CHECK(mc_is_admissible<mc_member>::value ==
               concepts::has_metadata<mc_member>);
    D_MC_CHECK(mc_is_admissible<mc_none>::value ==
               concepts::has_metadata<mc_none>);

    // the constrained call is usable in a constant expression
    D_MC_CHECK((std::integral_constant<int,
                                       mc_accepts(mc_member{})
                                       >::value == 7));

    return true;
}

/*
tests_metadata_concepts_subsumption
  The atomic concepts are more constrained than the disjunction over them.
  Tests the following:
  - given two overloads, one constrained on has_metadata_data_member and
    one on has_metadata, a type satisfying the atomic concept selects the
    atomic overload.  That requires the compiler to NORMALISE both
    constraints and find that the atomic one subsumes the disjunction; if
    it did not, both candidates would be equally viable and the call would
    be ambiguous, so this compiling at all is half the result
  - a type satisfying the unifier through some OTHER shape falls to the
    disjunction overload, which shows the ordering is doing real work
    rather than one overload simply always winning
  - the callable-member fixture, which satisfies two atomic concepts at
    once, still resolves - the partial order is over the constraints, not
    over how many happen to hold
  This is the property a plain bool cannot provide, and so the reason for
  this module to exist beside metadata_traits.hpp rather than as an alias
  for it.
*/
bool
tests_metadata_concepts_subsumption()
{
    // the atomic overload wins where its constraint is satisfied
    D_MC_CHECK(mc_pick(mc_member{}) == 2);
    D_MC_CHECK(mc_pick(mc_callable_member{}) == 2);

    // ... and the disjunction takes the cases it cannot
    D_MC_CHECK(mc_pick(mc_method{}) == 1);
    D_MC_CHECK(mc_pick(mc_nested{}) == 1);

    // both overloads are genuinely reachable, so neither result above is
    // simply one candidate always being chosen
    D_MC_CHECK(mc_pick(mc_member{}) != mc_pick(mc_nested{}));

    // the selection agrees with the constraint that drove it
    D_MC_CHECK((mc_pick(mc_member{}) == 2) ==
               concepts::has_metadata_data_member<mc_member>);
    D_MC_CHECK((mc_pick(mc_nested{}) == 2) ==
               concepts::has_metadata_data_member<mc_nested>);

    // resolution happens at compile time, as constraint checking must
    D_MC_CHECK((std::integral_constant<int,
                                       mc_pick(mc_member{})
                                       >::value == 2));
    D_MC_CHECK((std::integral_constant<int,
                                       mc_pick(mc_nested{})
                                       >::value == 1));

    return true;
}

/*
tests_metadata_concepts_reexports
  The `_t` extraction aliases re-exported into the concepts namespace.
  Tests the following:
  - concepts::metadata_t and djinterp::metadata_t name the SAME alias, not
    a second definition that happens to agree.  The module re-exports them
    with using-declarations precisely so that a caller working in the
    concepts namespace need not switch namespaces to extract a type
  - all three aliases are re-exported, and each still resolves through
    every shape
  - they extract the correct types for a protocol-conforming type, with
    the container family yielding the holder type rather than the payload
  Extraction has no concept form of its own - a type alias is not a
  predicate - so these re-exports are the whole of the module's extraction
  surface, and a missing one would only show up at a caller.
*/
bool
tests_metadata_concepts_reexports()
{
    // the same alias, reached through either namespace
    D_MC_CHECK((std::is_same_v<concepts::metadata_t<mc_member>,
                               djinterp::metadata_t<mc_member>>));
    D_MC_CHECK(
     (std::is_same_v<concepts::metadata_type_t<mct_nested>,
                     djinterp::metadata_type_t<mct_nested>>));
    D_MC_CHECK(
     (std::is_same_v<concepts::metadata_container_type_t<mcc_nested>,
                     djinterp::metadata_container_type_t<mcc_nested>>));

    // each resolves through every shape
    D_MC_CHECK(
     (std::is_same_v<concepts::metadata_t<mc_member>, mc_payload>));
    D_MC_CHECK(
     (std::is_same_v<concepts::metadata_t<mc_method>, mc_payload>));
    D_MC_CHECK(
     (std::is_same_v<concepts::metadata_t<mc_nested>, mc_payload>));

    // the mirrored families, including the holder-versus-payload
    // distinction that a crossed re-export would lose
    D_MC_CHECK(
     (std::is_same_v<concepts::metadata_type_t<mc_all_names>,
                     mc_payload>));
    D_MC_CHECK(
     (std::is_same_v<concepts::metadata_container_type_t<mc_all_names>,
                     mc_holder>));
    D_MC_CHECK(
     (!std::is_same_v<concepts::metadata_container_type_t<mc_all_names>,
                      concepts::metadata_type_t<mc_all_names>>));

    // qualifiers are stripped, as on the djinterp-namespace originals
    D_MC_CHECK(
     (std::is_same_v<concepts::metadata_t<const mc_member&>,
                     mc_payload>));

    return true;
}


NS_END  // testing
NS_END  // djinterp
