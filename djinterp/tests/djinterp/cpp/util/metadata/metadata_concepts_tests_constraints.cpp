/*******************************************************************************
* djinterp [cpp]                         metadata_concepts_tests_constraints.cpp
*
*
* path:      /tests/djinterp/cpp/util/metadata/metadata_concepts_tests_constraints.cpp
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
bool tests_metadata_concepts_constrained_use()
{
    // the constrained function accepts each shape
    D_MC_CHECK(mc_accepts(DMcMember{}) == 7);
    D_MC_CHECK(mc_accepts(DMcMethod{}) == 7);
    D_MC_CHECK(mc_accepts(DMcNested{}) == 7);
    D_MC_CHECK(mc_accepts(DMcAllNames{}) == 7);

    // admissibility is decided by the constraint, not by the body
    D_MC_CHECK(mc_is_admissible<DMcMember>::value);
    D_MC_CHECK(mc_is_admissible<DMcMethod>::value);
    D_MC_CHECK(mc_is_admissible<DMcNested>::value);
    D_MC_CHECK(mc_is_admissible<DMcCallableMember>::value);

    // ... and a non-conforming type is genuinely rejected
    D_MC_CHECK(!mc_is_admissible<DMcNone>::value);
    D_MC_CHECK(!mc_is_admissible<DMcNonconstMethod>::value);
    D_MC_CHECK(!mc_is_admissible<int>::value);
    D_MC_CHECK(!mc_is_admissible<DMctNested>::value);

    // admissibility tracks the concept exactly
    D_MC_CHECK(mc_is_admissible<DMcMember>::value ==
               concepts::has_metadata<DMcMember>);
    D_MC_CHECK(mc_is_admissible<DMcNone>::value ==
               concepts::has_metadata<DMcNone>);

    // the constrained call is usable in a constant expression
    D_MC_CHECK((std::integral_constant<int,
                                       mc_accepts(DMcMember{})
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
bool tests_metadata_concepts_subsumption()
{
    // the atomic overload wins where its constraint is satisfied
    D_MC_CHECK(mc_pick(DMcMember{}) == 2);
    D_MC_CHECK(mc_pick(DMcCallableMember{}) == 2);

    // ... and the disjunction takes the cases it cannot
    D_MC_CHECK(mc_pick(DMcMethod{}) == 1);
    D_MC_CHECK(mc_pick(DMcNested{}) == 1);

    // both overloads are genuinely reachable, so neither result above is
    // simply one candidate always being chosen
    D_MC_CHECK(mc_pick(DMcMember{}) != mc_pick(DMcNested{}));

    // the selection agrees with the constraint that drove it
    D_MC_CHECK((mc_pick(DMcMember{}) == 2) ==
               concepts::has_metadata_data_member<DMcMember>);
    D_MC_CHECK((mc_pick(DMcNested{}) == 2) ==
               concepts::has_metadata_data_member<DMcNested>);

    // resolution happens at compile time, as constraint checking must
    D_MC_CHECK((std::integral_constant<int,
                                       mc_pick(DMcMember{})
                                       >::value == 2));
    D_MC_CHECK((std::integral_constant<int,
                                       mc_pick(DMcNested{})
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
bool tests_metadata_concepts_reexports()
{
    // the same alias, reached through either namespace
    D_MC_CHECK((std::is_same_v<concepts::metadata_t<DMcMember>,
                               djinterp::metadata_t<DMcMember>>));
    D_MC_CHECK(
     (std::is_same_v<concepts::metadata_type_t<DMctNested>,
                     djinterp::metadata_type_t<DMctNested>>));
    D_MC_CHECK(
     (std::is_same_v<concepts::metadata_container_type_t<DMccNested>,
                     djinterp::metadata_container_type_t<DMccNested>>));

    // each resolves through every shape
    D_MC_CHECK(
     (std::is_same_v<concepts::metadata_t<DMcMember>, DMcPayload>));
    D_MC_CHECK(
     (std::is_same_v<concepts::metadata_t<DMcMethod>, DMcPayload>));
    D_MC_CHECK(
     (std::is_same_v<concepts::metadata_t<DMcNested>, DMcPayload>));

    // the mirrored families, including the holder-versus-payload
    // distinction that a crossed re-export would lose
    D_MC_CHECK(
     (std::is_same_v<concepts::metadata_type_t<DMcAllNames>,
                     DMcPayload>));
    D_MC_CHECK(
     (std::is_same_v<concepts::metadata_container_type_t<DMcAllNames>,
                     DMcHolder>));
    D_MC_CHECK(
     (!std::is_same_v<concepts::metadata_container_type_t<DMcAllNames>,
                      concepts::metadata_type_t<DMcAllNames>>));

    // qualifiers are stripped, as on the djinterp-namespace originals
    D_MC_CHECK(
     (std::is_same_v<concepts::metadata_t<const DMcMember&>,
                     DMcPayload>));

    return true;
}


NS_END  // testing
NS_END  // djinterp
