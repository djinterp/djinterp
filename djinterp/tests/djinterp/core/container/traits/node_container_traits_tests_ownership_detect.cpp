/*******************************************************************************
* djinterp [core]               node_container_traits_tests_ownership_detect.cpp
*
*
* path:      /tests/djinterp/core/container/traits/node_container_traits_tests_ownership_detect.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "node_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// I.   Ownership policy detection
// ===========================================================================
//   Three structural probes for the ownership half of a node container's
// interface.  Two ask for a nested alias; the third asks for a static constant
// of an EXACT type, which is a stricter question than the others and is tested
// as such.


/*
tests_has_ownership_policy
  Verifies the probe for a nested ownership_policy alias.
  Tests the following:
  - every fixture that names a policy is detected, whether the policy is
    complete or merely forward-declared - the probe asks for the alias, not for
    the policy's contents
  - the djinterp policies, visible here only as forward declarations, are
    detected exactly as the local complete ones are
  - a container with no policy is not detected, and neither is a non-container
  - the entry_storage alias does not stand in for the policy alias
*/
bool
tests_has_ownership_policy()
{
    bool ok = true;

    // local policies, complete
    D_NC_CHECK(has_ownership_policy<nct_with_owning_policy>::value == true);
    D_NC_CHECK(has_ownership_policy<nct_with_non_owning_policy>::value
                   == true);
    D_NC_CHECK(has_ownership_policy<nct_with_int_owns_policy>::value == true);
    D_NC_CHECK(has_ownership_policy<nct_with_no_owns_policy>::value  == true);

    // an INCOMPLETE policy is still a named alias
    D_NC_CHECK(has_ownership_policy<nct_with_incomplete_policy>::value
                   == true);

    // the djinterp policies, forward-declared only
    D_NC_CHECK(has_ownership_policy<nct_unique_container>::value     == true);
    D_NC_CHECK(has_ownership_policy<nct_shared_container>::value     == true);
    D_NC_CHECK(has_ownership_policy<nct_non_owning_container>::value == true);
    D_NC_CHECK(has_ownership_policy<nct_tree>::value                 == true);

    // containers without a policy
    D_NC_CHECK(has_ownership_policy<nct_list>::value    == false);
    D_NC_CHECK(has_ownership_policy<nct_graph>::value   == false);
    D_NC_CHECK(has_ownership_policy<nct_generic>::value == false);

    // the sibling alias is not a substitute
    D_NC_CHECK(has_ownership_policy<nct_entry_storage_only>::value == false);

    // non-containers
    D_NC_CHECK(has_ownership_policy<nct_bare>::value == false);
    D_NC_CHECK(has_ownership_policy<int>::value      == false);
    D_NC_CHECK(has_ownership_policy<void>::value     == false);
    D_NC_CHECK(has_ownership_policy<nonesuch>::value == false);

    return ok;
}

/*
tests_has_entry_storage_type
  Verifies the probe for a nested entry_storage alias - the type in which a
  container actually holds its entry reference.
  Tests the following:
  - the alias is detected in isolation and alongside the rest of the interface
  - it is independent of the policy alias in both directions: a type may carry
    one without the other
  - non-containers are not detected
*/
bool
tests_has_entry_storage_type()
{
    bool ok = true;

    // present
    D_NC_CHECK(has_entry_storage_type<nct_entry_storage_only>::value == true);
    D_NC_CHECK(has_entry_storage_type<nct_tree>::value               == true);

    // absent
    D_NC_CHECK(has_entry_storage_type<nct_list>::value    == false);
    D_NC_CHECK(has_entry_storage_type<nct_graph>::value   == false);
    D_NC_CHECK(has_entry_storage_type<nct_generic>::value == false);
    D_NC_CHECK(has_entry_storage_type<nct_bare>::value    == false);
    D_NC_CHECK(has_entry_storage_type<int>::value         == false);
    D_NC_CHECK(has_entry_storage_type<void>::value        == false);

    // independent of the policy alias, in both directions
    D_NC_CHECK(   ( has_entry_storage_type<nct_entry_storage_only>::value
                        == true )
               && ( has_ownership_policy<nct_entry_storage_only>::value
                        == false ));
    D_NC_CHECK(   ( has_entry_storage_type<nct_with_owning_policy>::value
                        == false )
               && ( has_ownership_policy<nct_with_owning_policy>::value
                        == true ));
    D_NC_CHECK(   ( has_entry_storage_type<nct_tree>::value == true )
               && ( has_ownership_policy<nct_tree>::value   == true ));

    return ok;
}

/*
tests_has_entry_owns_constant
  Verifies the probe for a static entry_owns constant.  Unlike the two alias
  probes, this one compares decltype against `const bool` EXACTLY, so it is a
  type test rather than a convertibility test.
  Tests the following:
  - `static constexpr bool` is detected - constexpr implies const, so this is
    the production spelling
  - `static const bool` is detected: the same declared type by another route
  - `static bool`, without const, is NOT detected
  - `static constexpr int` is NOT detected: const, but the wrong type, which is
    the strictness that separates this probe from is_owning_container's
    contextual conversion
  - a non-static data member is NOT detected: decltype names its declared type,
    a bare bool
  - a function named entry_owns is NOT detected
  - the constant is independent of the policy alias
*/
bool
tests_has_entry_owns_constant()
{
    bool ok = true;

    // the two spellings whose declared type is const bool
    D_NC_CHECK(has_entry_owns_constant<nct_entry_owns_constexpr>::value
                   == true);
    D_NC_CHECK(has_entry_owns_constant<nct_entry_owns_const>::value == true);
    D_NC_CHECK(has_entry_owns_constant<nct_tree>::value             == true);

    // const missing
    D_NC_CHECK(has_entry_owns_constant<nct_entry_owns_plain>::value == false);

    // const present, type wrong
    D_NC_CHECK(has_entry_owns_constant<nct_entry_owns_int>::value == false);

    // not a constant at all
    D_NC_CHECK(has_entry_owns_constant<nct_entry_owns_nonstatic>::value
                   == false);
    D_NC_CHECK(has_entry_owns_constant<nct_entry_owns_method>::value
                   == false);

    // absent
    D_NC_CHECK(has_entry_owns_constant<nct_list>::value    == false);
    D_NC_CHECK(has_entry_owns_constant<nct_generic>::value == false);
    D_NC_CHECK(has_entry_owns_constant<nct_bare>::value    == false);
    D_NC_CHECK(has_entry_owns_constant<int>::value         == false);
    D_NC_CHECK(has_entry_owns_constant<void>::value        == false);
    D_NC_CHECK(has_entry_owns_constant<nonesuch>::value    == false);

    // independent of the policy alias: the constant summarises the policy but
    // is not derived from it
    D_NC_CHECK(   ( has_entry_owns_constant<nct_with_owning_policy>::value
                        == false )
               && ( has_ownership_policy<nct_with_owning_policy>::value
                        == true ));
    D_NC_CHECK(   ( has_entry_owns_constant<nct_entry_owns_constexpr>::value
                        == true )
               && ( has_ownership_policy<nct_entry_owns_constexpr>::value
                        == false ));

    return ok;
}

/*
tests_ownership_detect_cv_ref
  Verifies that all three section I probes honour the clean_t contract: each
  writes clean_t<_Type> before asking its question, so cv-qualification and
  reference-ness are stripped and every spelling of a type answers alike.
  Tests the following:
  - all three probes, across a positive and a negative carrier each, in all
    seven cv- and reference-qualified spellings
  - the volatile spellings in particular, since these are the ones the section
    II probes get wrong - the divergence is not a property of the header as a
    whole, only of how section II phrases its question
*/
bool
tests_ownership_detect_cv_ref()
{
    bool ok = true;

    D_NC_CV_REF_AGREE(has_ownership_policy, nct_tree,             true);
    D_NC_CV_REF_AGREE(has_ownership_policy, nct_unique_container, true);
    D_NC_CV_REF_AGREE(has_ownership_policy, nct_list,             false);
    D_NC_CV_REF_AGREE(has_ownership_policy, nct_bare,             false);

    D_NC_CV_REF_AGREE(has_entry_storage_type, nct_tree,     true);
    D_NC_CV_REF_AGREE(has_entry_storage_type, nct_generic,  false);

    D_NC_CV_REF_AGREE(has_entry_owns_constant, nct_tree,             true);
    D_NC_CV_REF_AGREE(has_entry_owns_constant, nct_entry_owns_const, true);
    D_NC_CV_REF_AGREE(has_entry_owns_constant, nct_entry_owns_int,   false);
    D_NC_CV_REF_AGREE(has_entry_owns_constant, nct_bare,             false);

    return ok;
}

/*
tests_ownership_detect_companions
  Verifies the `_v` companions of the section I probes.
  Tests the following:
  - each companion agrees with its trait's ::value on a positive and a negative
    carrier
  - the companions are usable in a constant expression
  - the body is gated on variable-template support, since below C++14 the
    engine emits nothing and ::value remains the portable spelling
*/
bool
tests_ownership_detect_companions()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_NC_STATIC_CHECK(has_ownership_policy_v<nct_tree>      == true);
    D_NC_STATIC_CHECK(has_entry_owns_constant_v<nct_tree>   == true);

    D_NC_CHECK(has_ownership_policy_v<nct_tree>
                   == has_ownership_policy<nct_tree>::value);
    D_NC_CHECK(has_ownership_policy_v<nct_list>
                   == has_ownership_policy<nct_list>::value);

    D_NC_CHECK(has_entry_storage_type_v<nct_tree>
                   == has_entry_storage_type<nct_tree>::value);
    D_NC_CHECK(has_entry_storage_type_v<nct_list>
                   == has_entry_storage_type<nct_list>::value);

    D_NC_CHECK(has_entry_owns_constant_v<nct_entry_owns_constexpr>
                   == has_entry_owns_constant<
                          nct_entry_owns_constexpr>::value);
    D_NC_CHECK(has_entry_owns_constant_v<nct_entry_owns_int>
                   == has_entry_owns_constant<nct_entry_owns_int>::value);

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
