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
bool
tests_concept_accepts_passthroughs()
{
    D_PT_CHECK(Passthrough<pt_direct>);
    D_PT_CHECK(Passthrough<pt_child>);
    D_PT_CHECK(Passthrough<pt_private>);
    D_PT_CHECK(Passthrough<pt_multi>);
    D_PT_CHECK(Passthrough<passthrough_marker>);

    // and through a cv-ref spelling
    D_PT_CHECK(Passthrough<const pt_direct&>);

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
bool
tests_concept_rejects_non_passthroughs()
{
    D_PT_CHECK(!Passthrough<plain>);
    D_PT_CHECK(!Passthrough<derives_unrelated>);
    D_PT_CHECK(!Passthrough<unrelated_base>);
    D_PT_CHECK(!Passthrough<int>);
    D_PT_CHECK(!Passthrough<void>);
    D_PT_CHECK(!Passthrough<pt_direct*>);

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
bool
tests_concept_agrees_with_trait()
{
    D_PT_CHECK(Passthrough<pt_direct> == is_passthrough<pt_direct>::value);
    D_PT_CHECK(Passthrough<pt_private> == is_passthrough<pt_private>::value);
    D_PT_CHECK(Passthrough<plain> == is_passthrough<plain>::value);
    D_PT_CHECK(Passthrough<int> == is_passthrough<int>::value);
    D_PT_CHECK(Passthrough<void> == is_passthrough<void>::value);

    // cv-ref spelling
    D_PT_CHECK(Passthrough<const pt_child&> ==
               is_passthrough<const pt_child&>::value);

    // marker as its own base
    D_PT_CHECK(Passthrough<passthrough_marker> ==
               is_passthrough<passthrough_marker>::value);

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
bool
tests_concept_constrains_overload_resolution()
{
    pt_direct         direct;
    pt_private        priv;
    plain             ordinary;
    unrelated_base    unrelated;

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
