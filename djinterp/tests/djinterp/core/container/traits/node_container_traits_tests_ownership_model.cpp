/*******************************************************************************
* djinterp [core]                node_container_traits_tests_ownership_model.cpp
*
*
* path:      /tests/djinterp/core/container/traits/node_container_traits_tests_ownership_model.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "node_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// IV.  Ownership model classification
// ===========================================================================
//   Two independent questions wearing similar names.  ownership_model_of asks
// WHICH policy a container names, by type identity, and needs only the name.
// is_owning_container asks WHETHER that policy owns, by reading a member, and
// needs the complete type.  A forward-declared policy answers the first and
// not the second, which is the situation this header creates for itself by
// forward-declaring djinterp's three policies rather than including them.


/*
tests_ownership_model_enum
  Verifies the ownership enumeration itself.
  Tests the following:
  - it is a SCOPED enumeration: it does not implicitly convert to int, so a
    model cannot be mistaken for a count
  - its underlying type is std::uint8_t, as declared - the header pins this
    rather than leaving it to the implementation, so the choice is part of the
    contract
  - the four enumerators carry the values the header assigns them, and are
    pairwise distinct
  - a model is usable as a non-type template argument, which is what lets
    ownership_model_of derive from integral_constant
*/
bool
tests_ownership_model_enum()
{
    bool ok = true;

    // scoped, and pinned to an explicit underlying type
    D_NC_STATIC_CHECK(std::is_enum<DOwnershipModel>::value == true);
    D_NC_STATIC_CHECK(
        std::is_convertible<DOwnershipModel, int>::value == false);
    D_NC_STATIC_CHECK(
        std::is_same<std::underlying_type<DOwnershipModel>::type,
                     std::uint8_t>::value);

    // the explicitly assigned values
    D_NC_CHECK(static_cast<int>(DOwnershipModel::non_owning) == 0);
    D_NC_CHECK(static_cast<int>(DOwnershipModel::unique)     == 1);
    D_NC_CHECK(static_cast<int>(DOwnershipModel::shared)     == 2);
    D_NC_CHECK(static_cast<int>(DOwnershipModel::unknown)    == 3);

    // pairwise distinct
    D_NC_CHECK(DOwnershipModel::non_owning != DOwnershipModel::unique);
    D_NC_CHECK(DOwnershipModel::non_owning != DOwnershipModel::shared);
    D_NC_CHECK(DOwnershipModel::non_owning != DOwnershipModel::unknown);
    D_NC_CHECK(DOwnershipModel::unique     != DOwnershipModel::shared);
    D_NC_CHECK(DOwnershipModel::unique     != DOwnershipModel::unknown);
    D_NC_CHECK(DOwnershipModel::shared     != DOwnershipModel::unknown);

    // usable as a non-type template argument
    D_NC_STATIC_CHECK(
        std::integral_constant<DOwnershipModel,
                               DOwnershipModel::shared>::value
            == DOwnershipModel::shared);

    return ok;
}

/*
tests_ownership_model_of
  Verifies the classification of a named policy by type identity.
  Tests the following:
  - each of djinterp's three policies is matched to its model, using only the
    forward declarations this header provides - identity needs no definition
  - a policy that is none of the three reads unknown, even when it is complete
    and does own, so unknown means "unrecognised", not "unowned"
  - an INCOMPLETE unrecognised policy also reads unknown, by the same route
  - the model is a property of the policy alias alone and is independent of a
    container's shape
*/
bool
tests_ownership_model_of()
{
    bool ok = true;

    // the three recognised policies
    D_NC_CHECK(ownership_model_of<nct_unique_container>::value
                   == DOwnershipModel::unique);
    D_NC_CHECK(ownership_model_of<nct_tree>::value
                   == DOwnershipModel::unique);
    D_NC_CHECK(ownership_model_of<nct_shared_container>::value
                   == DOwnershipModel::shared);
    D_NC_CHECK(ownership_model_of<nct_non_owning_container>::value
                   == DOwnershipModel::non_owning);

    // recognised is not the same as owning
    D_NC_CHECK(ownership_model_of<nct_unknown_policy_container>::value
                   == DOwnershipModel::unknown);
    D_NC_CHECK(ownership_model_of<nct_with_owning_policy>::value
                   == DOwnershipModel::unknown);
    D_NC_CHECK(ownership_model_of<nct_with_non_owning_policy>::value
                   == DOwnershipModel::unknown);
    D_NC_CHECK(ownership_model_of<nct_with_no_owns_policy>::value
                   == DOwnershipModel::unknown);

    // an incomplete policy can still be classified by identity
    D_NC_CHECK(ownership_model_of<nct_with_incomplete_policy>::value
                   == DOwnershipModel::unknown);

    // shape is immaterial: the same policy on a tree and on a graph
    D_NC_CHECK(ownership_model_of<nct_unique_container>::value
                   == ownership_model_of<nct_tree>::value);
    D_NC_CHECK(   ( is_tree_shaped_container<nct_unique_container>::value
                        == true )
               && ( is_graph_shaped_container<nct_shared_container>::value
                        == true )
               && ( ownership_model_of<nct_unique_container>::value
                        != ownership_model_of<nct_shared_container>::value ));

    return ok;
}

/*
tests_ownership_model_absent_policy
  Verifies what the classifier answers for a type that names NO policy - the
  primary template's branch.
  Tests the following:
  - the answer is non_owning, not unknown.  A type with no ownership story at
    all is therefore reported as positively non-owning, which is
    indistinguishable from a container that deliberately names
    non_owning_policy: only has_ownership_policy separates the two
  - this holds for containers, for scalars and for void alike, so the model is
    total but not discriminating at its default
  - the internal implementation's two forms agree with the published trait, and
    its bool parameter is what selects between them
*/
bool
tests_ownership_model_absent_policy()
{
    bool ok = true;

    // containers with no policy
    D_NC_CHECK(ownership_model_of<nct_list>::value
                   == DOwnershipModel::non_owning);
    D_NC_CHECK(ownership_model_of<nct_graph>::value
                   == DOwnershipModel::non_owning);
    D_NC_CHECK(ownership_model_of<nct_generic>::value
                   == DOwnershipModel::non_owning);

    // and types that are not containers at all
    D_NC_CHECK(ownership_model_of<nct_bare>::value
                   == DOwnershipModel::non_owning);
    D_NC_CHECK(ownership_model_of<int>::value
                   == DOwnershipModel::non_owning);
    D_NC_CHECK(ownership_model_of<void>::value
                   == DOwnershipModel::non_owning);
    D_NC_CHECK(ownership_model_of<nonesuch>::value
                   == DOwnershipModel::non_owning);

    // the default and the deliberate answer are the same value; only the
    // policy probe distinguishes them
    D_NC_CHECK(ownership_model_of<int>::value
                   == ownership_model_of<nct_non_owning_container>::value);
    D_NC_CHECK(   ( has_ownership_policy<int>::value == false )
               && ( has_ownership_policy<nct_non_owning_container>::value
                        == true ));

    // the internal implementation agrees with the published trait
    D_NC_CHECK(internal::ownership_model_impl<nct_list>::value
                   == ownership_model_of<nct_list>::value);
    D_NC_CHECK(internal::ownership_model_impl<nct_tree>::value
                   == ownership_model_of<nct_tree>::value);

    // and its bool parameter selects the branch
    D_NC_CHECK(internal::ownership_model_impl<nct_tree, false>::value
                   == DOwnershipModel::non_owning);
    D_NC_CHECK(internal::ownership_model_impl<nct_tree, true>::value
                   == DOwnershipModel::unique);

    return ok;
}

/*
tests_is_owning_container
  Verifies the predicate that reads the policy's own `owns` member.
  Tests the following:
  - a complete policy whose owns is true qualifies; one whose owns is false
    does not
  - a policy with NO owns member does not qualify, and neither does one whose
    owns is a non-static data member - neither is nameable as a constant
  - the read is a CONTEXTUAL conversion to bool, not a type match: an int-
    valued owns of 2 qualifies.  This is looser than has_entry_owns_constant,
    which demands exactly const bool, so the two constants in this header are
    checked to different standards
  - a type with no policy at all does not qualify
*/
bool
tests_is_owning_container()
{
    bool ok = true;

    // reads the member
    D_NC_CHECK(is_owning_container<nct_with_owning_policy>::value == true);
    D_NC_CHECK(is_owning_container<nct_with_non_owning_policy>::value
                   == false);

    // contextual conversion, not a type match
    D_NC_CHECK(is_owning_container<nct_with_int_owns_policy>::value == true);

    // not nameable as a constant
    D_NC_CHECK(is_owning_container<nct_with_no_owns_policy>::value == false);
    D_NC_CHECK(is_owning_container<nct_with_nonstatic_owns_policy>::value
                   == false);

    // no policy at all
    D_NC_CHECK(is_owning_container<nct_list>::value    == false);
    D_NC_CHECK(is_owning_container<nct_generic>::value == false);
    D_NC_CHECK(is_owning_container<nct_bare>::value    == false);
    D_NC_CHECK(is_owning_container<int>::value         == false);
    D_NC_CHECK(is_owning_container<void>::value        == false);
    D_NC_CHECK(is_owning_container<nonesuch>::value    == false);

    // a container may own without being recognised
    D_NC_CHECK(is_owning_container<nct_unknown_policy_container>::value
                   == true);

    // the two constants in this header are checked to different standards:
    // owns converts, entry_owns must match exactly
    D_NC_CHECK(   ( is_owning_container<nct_with_int_owns_policy>::value
                        == true )
               && ( has_entry_owns_constant<nct_entry_owns_int>::value
                        == false ));

    return ok;
}

/*
tests_is_owning_container_completeness
  Verifies that is_owning_container requires a COMPLETE policy, and pins the
  consequence for this suite.

  Two fixtures differing only in whether their policy is defined must answer
  differently - that part is true in any translation unit.  The second half is
  configuration-specific: this suite does not include node_container.hpp, so
  djinterp's three policies are incomplete here and is_owning_container reads
  false for every container that names one, even where the model says unique or
  shared.  Including node_container.hpp anywhere in a program that also uses
  these traits would flip that answer in the including TU only, instantiating
  the same trait two ways.  If this test starts failing, the include graph has
  changed and that hazard is live.

  Tests the following:
  - a complete owning policy qualifies; an incomplete one does not
  - the two are indistinguishable to has_ownership_policy and to
    ownership_model_of, so the difference is visible only here
  - the djinterp policies behave as incomplete types in this suite
  - ownership_model_of still classifies them, since identity needs no
    definition
*/
bool
tests_is_owning_container_completeness()
{
    bool ok = true;

    // complete versus incomplete, all else equal
    D_NC_CHECK(is_owning_container<nct_with_owning_policy>::value     == true);
    D_NC_CHECK(is_owning_container<nct_with_incomplete_policy>::value
                   == false);

    // the alias probe cannot tell them apart
    D_NC_CHECK(has_ownership_policy<nct_with_owning_policy>::value
                   == has_ownership_policy<nct_with_incomplete_policy>::value);

    // nor can the model, since neither policy is one of the three names
    D_NC_CHECK(ownership_model_of<nct_with_owning_policy>::value
                   == ownership_model_of<nct_with_incomplete_policy>::value);

    // in THIS suite the djinterp policies are incomplete
    D_NC_CHECK(is_owning_container<nct_unique_container>::value == false);
    D_NC_CHECK(is_owning_container<nct_shared_container>::value == false);
    D_NC_CHECK(is_owning_container<nct_tree>::value             == false);

    // ... while identity still classifies them
    D_NC_CHECK(   ( ownership_model_of<nct_unique_container>::value
                        == DOwnershipModel::unique )
               && ( is_owning_container<nct_unique_container>::value
                        == false ));
    D_NC_CHECK(   ( ownership_model_of<nct_shared_container>::value
                        == DOwnershipModel::shared )
               && ( is_owning_container<nct_shared_container>::value
                        == false ));

    // the non-owning policy agrees either way, so it is not a witness
    D_NC_CHECK(is_owning_container<nct_non_owning_container>::value == false);

    return ok;
}

/*
tests_is_unique_owning_container
  Verifies the predicate for the exclusive-ownership model.
  Tests the following:
  - the containers naming unique_owning_policy qualify
  - no other model does, including unknown and non_owning
  - the predicate equals its definition - the model IS unique
  - it does not consult is_owning_container, so it holds even where the policy
    is too incomplete to read
*/
bool
tests_is_unique_owning_container()
{
    bool ok = true;

    // positive
    D_NC_CHECK(is_unique_owning_container<nct_unique_container>::value
                   == true);
    D_NC_CHECK(is_unique_owning_container<nct_tree>::value == true);

    // every other model
    D_NC_CHECK(is_unique_owning_container<nct_shared_container>::value
                   == false);
    D_NC_CHECK(is_unique_owning_container<nct_non_owning_container>::value
                   == false);
    D_NC_CHECK(is_unique_owning_container<nct_unknown_policy_container>::value
                   == false);
    D_NC_CHECK(is_unique_owning_container<nct_list>::value == false);
    D_NC_CHECK(is_unique_owning_container<int>::value      == false);
    D_NC_CHECK(is_unique_owning_container<void>::value     == false);

    // agreement with the definition
    D_NC_CHECK(is_unique_owning_container<nct_tree>::value
                   == ( ownership_model_of<nct_tree>::value
                            == DOwnershipModel::unique ));
    D_NC_CHECK(is_unique_owning_container<nct_list>::value
                   == ( ownership_model_of<nct_list>::value
                            == DOwnershipModel::unique ));

    // independent of the completeness the owning read needs
    D_NC_CHECK(   ( is_unique_owning_container<nct_tree>::value == true )
               && ( is_owning_container<nct_tree>::value        == false ));

    return ok;
}

/*
tests_is_shared_owning_container
  Verifies the predicate for the reference-counted model - the sharing that,
  in the monograph's terms, admits a node reached by several references and so
  a DAG or general graph rather than a tree.
  Tests the following:
  - the container naming shared_owning_policy qualifies
  - no other model does
  - the predicate equals its definition
  - shared and unique are mutually exclusive, and both are false for every
    unrecognised or absent policy
*/
bool
tests_is_shared_owning_container()
{
    bool ok = true;

    // positive
    D_NC_CHECK(is_shared_owning_container<nct_shared_container>::value
                   == true);

    // every other model
    D_NC_CHECK(is_shared_owning_container<nct_unique_container>::value
                   == false);
    D_NC_CHECK(is_shared_owning_container<nct_tree>::value == false);
    D_NC_CHECK(is_shared_owning_container<nct_non_owning_container>::value
                   == false);
    D_NC_CHECK(is_shared_owning_container<nct_unknown_policy_container>::value
                   == false);
    D_NC_CHECK(is_shared_owning_container<nct_graph>::value == false);
    D_NC_CHECK(is_shared_owning_container<int>::value       == false);
    D_NC_CHECK(is_shared_owning_container<void>::value      == false);

    // agreement with the definition
    D_NC_CHECK(is_shared_owning_container<nct_shared_container>::value
                   == ( ownership_model_of<nct_shared_container>::value
                            == DOwnershipModel::shared ));

    // mutually exclusive with unique
    D_NC_CHECK(!(   is_shared_owning_container<nct_shared_container>::value
                 && is_unique_owning_container<nct_shared_container>::value ));
    D_NC_CHECK(!(   is_shared_owning_container<nct_tree>::value
                 && is_unique_owning_container<nct_tree>::value ));
    D_NC_CHECK(   ( is_shared_owning_container<nct_list>::value == false )
               && ( is_unique_owning_container<nct_list>::value == false ));

    return ok;
}

/*
tests_ownership_model_orthogonality
  Verifies that the identity question and the ownership question are genuinely
  independent, by exhibiting a fixture for each combination that can exist.
  Tests the following:
  - recognised model, cannot read owns    - the djinterp policies here
  - unrecognised model, owns true         - a complete local owning policy
  - unrecognised model, owns false        - a complete local non-owning policy
  - default model, no policy at all       - a container with no ownership story
  - the model never determines the owning answer and vice versa
*/
bool
tests_ownership_model_orthogonality()
{
    bool ok = true;

    // recognised, unreadable
    D_NC_CHECK(   ( ownership_model_of<nct_unique_container>::value
                        == DOwnershipModel::unique )
               && ( is_owning_container<nct_unique_container>::value
                        == false ));

    // unrecognised, owning
    D_NC_CHECK(   ( ownership_model_of<nct_with_owning_policy>::value
                        == DOwnershipModel::unknown )
               && ( is_owning_container<nct_with_owning_policy>::value
                        == true ));

    // unrecognised, not owning
    D_NC_CHECK(   ( ownership_model_of<nct_with_non_owning_policy>::value
                        == DOwnershipModel::unknown )
               && ( is_owning_container<nct_with_non_owning_policy>::value
                        == false ));

    // recognised as non-owning, and indeed not owning
    D_NC_CHECK(   ( ownership_model_of<nct_non_owning_container>::value
                        == DOwnershipModel::non_owning )
               && ( is_owning_container<nct_non_owning_container>::value
                        == false ));

    // no policy at all
    D_NC_CHECK(   ( ownership_model_of<nct_list>::value
                        == DOwnershipModel::non_owning )
               && ( is_owning_container<nct_list>::value == false )
               && ( has_ownership_policy<nct_list>::value == false ));

    // one model spans both owning answers, and one owning answer spans two
    // models - so neither determines the other
    D_NC_CHECK(   ( ownership_model_of<nct_with_owning_policy>::value
                        == ownership_model_of<
                               nct_with_non_owning_policy>::value )
               && ( is_owning_container<nct_with_owning_policy>::value
                        != is_owning_container<
                               nct_with_non_owning_policy>::value ));
    D_NC_CHECK(   ( is_owning_container<nct_unique_container>::value
                        == is_owning_container<
                               nct_non_owning_container>::value )
               && ( ownership_model_of<nct_unique_container>::value
                        != ownership_model_of<
                               nct_non_owning_container>::value ));

    return ok;
}

/*
tests_ownership_model_cv_ref
  Verifies that section IV honours the clean_t contract throughout - both the
  identity classifier and the member read strip qualification before asking.
  Tests the following:
  - ownership_model_of answers alike for all seven spellings, across every
    model
  - is_owning_container and the two model predicates do likewise
  - the volatile spellings included, which is where section II diverges
*/
bool
tests_ownership_model_cv_ref()
{
    bool ok = true;

    D_NC_CV_REF_AGREE(ownership_model_of, nct_unique_container,
                      DOwnershipModel::unique);
    D_NC_CV_REF_AGREE(ownership_model_of, nct_shared_container,
                      DOwnershipModel::shared);
    D_NC_CV_REF_AGREE(ownership_model_of, nct_non_owning_container,
                      DOwnershipModel::non_owning);
    D_NC_CV_REF_AGREE(ownership_model_of, nct_with_owning_policy,
                      DOwnershipModel::unknown);
    D_NC_CV_REF_AGREE(ownership_model_of, nct_bare,
                      DOwnershipModel::non_owning);

    D_NC_CV_REF_AGREE(is_owning_container, nct_with_owning_policy, true);
    D_NC_CV_REF_AGREE(is_owning_container, nct_with_int_owns_policy, true);
    D_NC_CV_REF_AGREE(is_owning_container, nct_with_non_owning_policy, false);
    D_NC_CV_REF_AGREE(is_owning_container, nct_unique_container, false);

    D_NC_CV_REF_AGREE(is_unique_owning_container, nct_unique_container, true);
    D_NC_CV_REF_AGREE(is_unique_owning_container, nct_shared_container, false);
    D_NC_CV_REF_AGREE(is_shared_owning_container, nct_shared_container, true);
    D_NC_CV_REF_AGREE(is_shared_owning_container, nct_unique_container, false);

    return ok;
}

/*
tests_ownership_model_companions
  Verifies the `_v` companions of the section IV traits.
  Tests the following:
  - each companion agrees with its trait's ::value
  - the model companion carries the enum, not a bool
  - the companions are usable in a constant expression
  - gated on variable-template support
*/
bool
tests_ownership_model_companions()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_NC_STATIC_CHECK(ownership_model_of_v<nct_unique_container>
                          == DOwnershipModel::unique);
    D_NC_STATIC_CHECK(is_owning_container_v<nct_with_owning_policy> == true);

    D_NC_CHECK(ownership_model_of_v<nct_unique_container>
                   == ownership_model_of<nct_unique_container>::value);
    D_NC_CHECK(ownership_model_of_v<nct_shared_container>
                   == ownership_model_of<nct_shared_container>::value);
    D_NC_CHECK(ownership_model_of_v<int>
                   == ownership_model_of<int>::value);

    D_NC_CHECK(is_owning_container_v<nct_with_owning_policy>
                   == is_owning_container<nct_with_owning_policy>::value);
    D_NC_CHECK(is_owning_container_v<nct_list>
                   == is_owning_container<nct_list>::value);

    D_NC_CHECK(is_unique_owning_container_v<nct_unique_container>
                   == is_unique_owning_container<
                          nct_unique_container>::value);
    D_NC_CHECK(is_unique_owning_container_v<nct_list>
                   == is_unique_owning_container<nct_list>::value);

    D_NC_CHECK(is_shared_owning_container_v<nct_shared_container>
                   == is_shared_owning_container<
                          nct_shared_container>::value);
    D_NC_CHECK(is_shared_owning_container_v<nct_list>
                   == is_shared_owning_container<nct_list>::value);

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
