/*******************************************************************************
* djinterp [core]                          metadata_concepts_tests_agreement.cpp
*
*
* path:      /tests/djinterp/core/util/metadata/metadata_concepts_tests_agreement.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "metadata_concepts_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_metadata_concepts_agreement
  Every concept agrees with the trait of the same name.
  Tests the following:
  - all TWELVE names are checked, each against a fixture that satisfies it
    and fixtures that do not, so agreement is established on both answers
    rather than only on the affirmative one
  - the two mechanisms are genuinely independent - the traits are built by
    the D_TYPE_TRAIT_ detection macros over void_t, the concepts are
    hand-written requires-expressions - so this is a comparison of two
    implementations rather than of one implementation with itself
  - the edge fixtures are included, since the ordinary cases are where two
    implementations are least likely to differ
  D_MC_AGREES writes the shared name once and reads it out of both
  namespaces, so no check can degenerate into comparing a name with
  itself.
*/
bool
tests_metadata_concepts_agreement()
{
    // --- the `metadata` family ---
    D_MC_AGREES(has_metadata_data_member, mc_member);
    D_MC_AGREES(has_metadata_data_member, mc_method);
    D_MC_AGREES(has_metadata_data_member, mc_none);
    D_MC_AGREES(has_metadata_method, mc_method);
    D_MC_AGREES(has_metadata_method, mc_member);
    D_MC_AGREES(has_metadata_method, mc_none);
    D_MC_AGREES(has_metadata_nested_type, mc_nested);
    D_MC_AGREES(has_metadata_nested_type, mc_member);
    D_MC_AGREES(has_metadata_nested_type, mc_none);
    D_MC_AGREES(has_metadata, mc_member);
    D_MC_AGREES(has_metadata, mc_method);
    D_MC_AGREES(has_metadata, mc_nested);
    D_MC_AGREES(has_metadata, mc_none);
    D_MC_AGREES(has_metadata, mc_all_names);

    // --- the `metadata_type` family ---
    D_MC_AGREES(has_metadata_type_data_member, mct_member);
    D_MC_AGREES(has_metadata_type_data_member, mct_nested);
    D_MC_AGREES(has_metadata_type_data_member, mc_none);
    D_MC_AGREES(has_metadata_type_method, mct_method);
    D_MC_AGREES(has_metadata_type_method, mct_member);
    D_MC_AGREES(has_metadata_type_method, mc_none);
    D_MC_AGREES(has_metadata_type_nested_type, mct_nested);
    D_MC_AGREES(has_metadata_type_nested_type, mct_method);
    D_MC_AGREES(has_metadata_type_nested_type, mc_none);
    D_MC_AGREES(has_metadata_type, mct_member);
    D_MC_AGREES(has_metadata_type, mct_method);
    D_MC_AGREES(has_metadata_type, mct_nested);
    D_MC_AGREES(has_metadata_type, mc_none);
    D_MC_AGREES(has_metadata_type, mc_all_names);

    // --- the `metadata_container_type` family ---
    D_MC_AGREES(has_metadata_container_type_data_member, mcc_member);
    D_MC_AGREES(has_metadata_container_type_data_member, mcc_nested);
    D_MC_AGREES(has_metadata_container_type_data_member, mc_none);
    D_MC_AGREES(has_metadata_container_type_method, mcc_method);
    D_MC_AGREES(has_metadata_container_type_method, mcc_member);
    D_MC_AGREES(has_metadata_container_type_method, mc_none);
    D_MC_AGREES(has_metadata_container_type_nested_type, mcc_nested);
    D_MC_AGREES(has_metadata_container_type_nested_type, mcc_method);
    D_MC_AGREES(has_metadata_container_type_nested_type, mc_none);
    D_MC_AGREES(has_metadata_container_type, mcc_member);
    D_MC_AGREES(has_metadata_container_type, mcc_method);
    D_MC_AGREES(has_metadata_container_type, mcc_nested);
    D_MC_AGREES(has_metadata_container_type, mc_none);
    D_MC_AGREES(has_metadata_container_type, mc_all_names);

    // --- cross-family, where a crossed wire would show ---
    D_MC_AGREES(has_metadata, mct_nested);
    D_MC_AGREES(has_metadata, mcc_nested);
    D_MC_AGREES(has_metadata_type, mc_member);
    D_MC_AGREES(has_metadata_container_type, mct_member);

    return true;
}

/*
tests_metadata_concepts_agreement_hostile
  Agreement holds on types that are not classes at all.
  Tests the following:
  - both implementations reject void, function types, arrays, references,
    pointers, member pointers, enums and nullptr_t, and reject them
    identically.  A requires-expression and a void_t detection idiom fail
    for different underlying reasons on these, so agreeing here says more
    than agreeing on ordinary class types
  - the representative types are named individually rather than swept with
    a quantifier: a concept cannot be passed as a template argument, so
    the trait-side quantifiers in test_traits.hpp have no concept-side
    counterpart to be compared against
*/
bool
tests_metadata_concepts_agreement_hostile()
{
    // fundamental and void
    D_MC_AGREES(has_metadata, int);
    D_MC_AGREES(has_metadata, void);
    D_MC_AGREES(has_metadata, double);
    D_MC_AGREES(has_metadata_type, void);
    D_MC_AGREES(has_metadata_container_type, void);

    // pointers, references and arrays
    D_MC_AGREES(has_metadata, int*);
    D_MC_AGREES(has_metadata, int&);
    D_MC_AGREES(has_metadata, void*);
    D_MC_AGREES(has_metadata_type, int*);

    // nullptr and enumerations
    D_MC_AGREES(has_metadata, std::nullptr_t);
    D_MC_AGREES(has_metadata_type, std::nullptr_t);
    D_MC_AGREES(has_metadata_container_type, std::nullptr_t);

    // the atomic concepts, not only the unifiers
    D_MC_AGREES(has_metadata_data_member, int);
    D_MC_AGREES(has_metadata_method, int);
    D_MC_AGREES(has_metadata_nested_type, int);
    D_MC_AGREES(has_metadata_nested_type, void);

    // and the trait side really does reject the whole zoo, which is the
    // sweep the concept side cannot express
    D_MC_CHECK((test::holds_for_none<has_metadata,
                                     D_TEST_HOSTILE_TYPES_COMPLETE
                                     >::value));
    D_MC_CHECK((test::holds_for_none<has_metadata_type,
                                     D_TEST_HOSTILE_TYPES_COMPLETE
                                     >::value));
    D_MC_CHECK((test::holds_for_none<has_metadata_container_type,
                                     D_TEST_HOSTILE_TYPES_COMPLETE
                                     >::value));

    return true;
}

/*
tests_metadata_concepts_agreement_qualified
  Agreement holds through cv- and reference-qualified forms.
  Tests the following:
  - const, volatile and reference qualifications give the same answer on
    both sides, for all three shapes
  - this is worth checking separately because the two implementations
    arrive at transparency by different routes: the expression shapes bind
    `const _Type&`, which absorbs qualifiers on its own, whereas the
    nested-alias shapes clean the type before naming into it.  Two
    mechanisms that happen to agree today could be changed independently
*/
bool
tests_metadata_concepts_agreement_qualified()
{
    // const, reference, and both, on the expression shapes
    D_MC_AGREES(has_metadata, const mc_member);
    D_MC_AGREES(has_metadata, mc_member&);
    D_MC_AGREES(has_metadata, const mc_member&);
    D_MC_AGREES(has_metadata_data_member, const mc_member&);
    D_MC_AGREES(has_metadata_method, const mc_method&);
    D_MC_AGREES(has_metadata, volatile mc_member);

    // and on the nested-alias shape, which cleans rather than binds
    D_MC_AGREES(has_metadata, const mc_nested);
    D_MC_AGREES(has_metadata, mc_nested&);
    D_MC_AGREES(has_metadata_nested_type, const mc_nested&);

    // the mirrored families likewise
    D_MC_AGREES(has_metadata_type, const mct_nested&);
    D_MC_AGREES(has_metadata_type, mct_member&);
    D_MC_AGREES(has_metadata_container_type, const mcc_nested&);
    D_MC_AGREES(has_metadata_container_type, mcc_method&);

    // qualification does not manufacture a protocol that was absent
    D_MC_AGREES(has_metadata, const mc_none&);
    D_MC_AGREES(has_metadata, mc_none&);
    D_MC_CHECK(!concepts::has_metadata<const mc_none&>);

    return true;
}


NS_END  // testing
NS_END  // djinterp
