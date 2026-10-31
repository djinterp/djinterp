/*******************************************************************************
* djinterp [cpp]                                   passthrough_tests_concept.cpp
*
*
* path:      /tests/djinterp/cpp/meta/passthrough_tests_concept.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "passthrough_tests.hpp"


NS_DJINTERP
NS_TESTING

#if defined(__cpp_concepts)

/*
tests_concept_accepts_passthroughs
  The Passthrough concept is satisfied by every kind of passthrough.
  Tests the following:
  - a direct inheritor satisfies it
  - a transitive (grandchild) inheritor satisfies it
  - a private inheritor satisfies it
  - a multiple-inheritance passthrough and the marker itself satisfy it
*/
bool tests_concept_accepts_passthroughs()
{
    D_PT_CHECK(Passthrough<DPtDirect>);
    D_PT_CHECK(Passthrough<DPtChild>);
    D_PT_CHECK(Passthrough<DPtPrivate>);
    D_PT_CHECK(Passthrough<DPtMulti>);
    D_PT_CHECK(Passthrough<DPassthroughMarker>);

    // and through a cv-ref spelling
    D_PT_CHECK(Passthrough<const DPtDirect&>);

    return true;
}

/*
tests_concept_rejects_non_passthroughs
  The Passthrough concept is not satisfied by non-passthroughs.
  Tests the following:
  - a plain struct does not satisfy it
  - an unrelated-base struct does not satisfy it
  - a fundamental type and void do not satisfy it
  - a pointer to a passthrough does not satisfy it
*/
bool tests_concept_rejects_non_passthroughs()
{
    D_PT_CHECK(!Passthrough<plain>);
    D_PT_CHECK(!Passthrough<DDerivesUnrelated>);
    D_PT_CHECK(!Passthrough<DUnrelatedBase>);
    D_PT_CHECK(!Passthrough<int>);
    D_PT_CHECK(!Passthrough<void>);
    D_PT_CHECK(!Passthrough<DPtDirect*>);

    return true;
}

/*
tests_concept_agrees_with_trait
  The concept and the trait are the same predicate.
  Tests the following:
  - Passthrough<T> equals is_passthrough<T>::value on passthroughs
  - and on non-passthroughs
  - the agreement holds through cv-ref spellings
  - it holds on the marker-as-its-own-base corner
*/
bool tests_concept_agrees_with_trait()
{
    D_PT_CHECK(Passthrough<DPtDirect> == is_passthrough<DPtDirect>::value);
    D_PT_CHECK(Passthrough<DPtPrivate> == is_passthrough<DPtPrivate>::value);
    D_PT_CHECK(Passthrough<plain> == is_passthrough<plain>::value);
    D_PT_CHECK(Passthrough<int> == is_passthrough<int>::value);
    D_PT_CHECK(Passthrough<void> == is_passthrough<void>::value);

    // cv-ref spelling
    D_PT_CHECK(Passthrough<const DPtChild&> ==
               is_passthrough<const DPtChild&>::value);

    // marker as its own base
    D_PT_CHECK(Passthrough<DPassthroughMarker> ==
               is_passthrough<DPassthroughMarker>::value);

    return true;
}

/*
tests_concept_constrains_overload_resolution
  The concept works as a constraint, not merely as a bool.
  Tests the following:
  - a passthrough selects the Passthrough-constrained overload
  - an inherited/private passthrough does too
  - a non-passthrough selects the unconstrained fallback
  - a fundamental selects the fallback
*/
bool tests_concept_constrains_overload_resolution()
{
    DPtDirect         direct;
    DPtPrivate        priv;
    plain             ordinary;
    DUnrelatedBase    unrelated;

    D_PT_CHECK(std::string(taken_by(direct)) == "passthrough");
    D_PT_CHECK(std::string(taken_by(priv)) == "passthrough");

    D_PT_CHECK(std::string(taken_by(ordinary)) == "generic");
    D_PT_CHECK(std::string(taken_by(unrelated)) == "generic");
    D_PT_CHECK(std::string(taken_by(42)) == "generic");

    return true;
}

#endif  // __cpp_concepts

NS_END  // testing
NS_END  // djinterp
