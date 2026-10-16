/*******************************************************************************
* djinterp [core]                      node_container_traits_tests_node_type.cpp
*
*
* path:      /tests/djinterp/core/container/traits/node_container_traits_tests_node_type.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "node_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// III. Node type extraction
// ===========================================================================
//   One probe and one extractor.  The extractor is total - it answers for
// every type - because it falls back to nonesuch rather than failing to
// instantiate, which is what lets node_container_class name a node member for
// any argument at all.


/*
tests_has_node_type
  Verifies the probe for a nested node_type alias - one half of the identity
  requirement.
  Tests the following:
  - every fixture that declares the alias is detected, whatever the alias names
  - an alias naming void is detected: the probe asks whether the name exists,
    not whether it names an object type
  - a type that exposes entry references but no alias is NOT detected, which is
    what keeps the identity requirement a conjunction
  - non-containers are not detected
  - the `_v` companion agrees with the trait and is usable in a constant
    expression
*/
bool
tests_has_node_type()
{
    bool ok = true;

    // present, across every shape
    D_NC_CHECK(has_node_type<nct_tree>::value    == true);
    D_NC_CHECK(has_node_type<nct_list>::value    == true);
    D_NC_CHECK(has_node_type<nct_slist>::value   == true);
    D_NC_CHECK(has_node_type<nct_graph>::value   == true);
    D_NC_CHECK(has_node_type<nct_generic>::value == true);

    // present, but naming something unusual
    D_NC_CHECK(has_node_type<nct_void_node_type>::value     == true);
    D_NC_CHECK(has_node_type<nct_const_node_type>::value    == true);
    D_NC_CHECK(has_node_type<nct_nonesuch_node_type>::value == true);

    // present without any entry reference
    D_NC_CHECK(has_node_type<nct_node_only>::value == true);

    // entry references without the alias
    D_NC_CHECK(has_node_type<nct_root_only>::value                 == false);
    D_NC_CHECK(has_node_type<nct_head_only_no_node>::value         == false);
    D_NC_CHECK(has_node_type<nct_entries_only_no_node>::value      == false);
    D_NC_CHECK(has_node_type<nct_entry_point_only_no_node>::value  == false);

    // non-containers
    D_NC_CHECK(has_node_type<nct_bare>::value == false);
    D_NC_CHECK(has_node_type<nct_node>::value == false);
    D_NC_CHECK(has_node_type<int>::value      == false);
    D_NC_CHECK(has_node_type<void>::value     == false);
    D_NC_CHECK(has_node_type<nonesuch>::value == false);

    // the `_v` companion.  Section III publishes only this one bool trait, so
    // its companion is checked here rather than in a section of its own.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_NC_STATIC_CHECK(has_node_type_v<nct_tree> == true);
    D_NC_STATIC_CHECK(has_node_type_v<int>      == false);

    D_NC_CHECK(has_node_type_v<nct_tree>
                   == has_node_type<nct_tree>::value);
    D_NC_CHECK(has_node_type_v<nct_node_only>
                   == has_node_type<nct_node_only>::value);
    D_NC_CHECK(has_node_type_v<nct_bare>
                   == has_node_type<nct_bare>::value);

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}

/*
tests_node_type_of_t
  Verifies the extractor on types that HAVE the alias.
  Tests the following:
  - the alias is reproduced exactly, for every shape fixture
  - the extracted type is not itself cleaned: a cv-qualified node_type survives
    qualification intact, since clean_t is applied to the container before the
    alias is read and never to the alias's result
  - an alias naming void extracts void
  - extraction is stable across every spelling of the container, because the
    probe that gates it uses clean_t
*/
bool
tests_node_type_of_t()
{
    bool ok = true;

    // the alias is reproduced exactly
    D_NC_STATIC_CHECK(std::is_same<node_type_of_t<nct_tree>,
                                   nct_node>::value);
    D_NC_STATIC_CHECK(std::is_same<node_type_of_t<nct_list>,
                                   nct_node>::value);
    D_NC_STATIC_CHECK(std::is_same<node_type_of_t<nct_graph>,
                                   nct_node>::value);
    D_NC_STATIC_CHECK(std::is_same<node_type_of_t<nct_generic>,
                                   nct_node>::value);

    // the extracted type keeps its own qualification
    D_NC_STATIC_CHECK(std::is_same<node_type_of_t<nct_const_node_type>,
                                   const nct_node>::value);
    D_NC_STATIC_CHECK(!std::is_same<node_type_of_t<nct_const_node_type>,
                                    nct_node>::value);

    // an alias may name void
    D_NC_STATIC_CHECK(std::is_same<node_type_of_t<nct_void_node_type>,
                                   void>::value);

    // stable across every spelling of the container
    D_NC_STATIC_CHECK(std::is_same<node_type_of_t<const nct_tree>,
                                   nct_node>::value);
    D_NC_STATIC_CHECK(std::is_same<node_type_of_t<volatile nct_tree>,
                                   nct_node>::value);
    D_NC_STATIC_CHECK(std::is_same<node_type_of_t<const volatile nct_tree>,
                                   nct_node>::value);
    D_NC_STATIC_CHECK(std::is_same<node_type_of_t<nct_tree&>,
                                   nct_node>::value);
    D_NC_STATIC_CHECK(std::is_same<node_type_of_t<const nct_tree&>,
                                   nct_node>::value);
    D_NC_STATIC_CHECK(std::is_same<node_type_of_t<nct_tree&&>,
                                   nct_node>::value);

    // the same conclusions, readable at run time so a failure names itself
    D_NC_CHECK(std::is_same<node_type_of_t<nct_tree>, nct_node>::value);
    D_NC_CHECK(std::is_same<node_type_of_t<nct_const_node_type>,
                            const nct_node>::value);
    D_NC_CHECK(std::is_same<node_type_of_t<nct_void_node_type>,
                            void>::value);

    return ok;
}

/*
tests_node_type_of_t_fallback
  Verifies the extractor on types that do NOT have the alias, where the
  internal helper's primary template supplies nonesuch.
  Tests the following:
  - every type without the alias extracts nonesuch: containers missing it,
    scalars, void, and nonesuch itself
  - the helper's two specialisations are selected on the value of
    has_node_type, so the helper agrees with node_type_of_t by construction
  - nonesuch is AMBIGUOUS as an answer: a container that declares
    node_type = nonesuch extracts the same type as one declaring nothing, and
    only has_node_type separates them.  Callers that need to distinguish the
    two must ask the probe, not compare the extracted type
*/
bool
tests_node_type_of_t_fallback()
{
    bool ok = true;

    // no alias, so nonesuch
    D_NC_STATIC_CHECK(std::is_same<node_type_of_t<nct_bare>,
                                   nonesuch>::value);
    D_NC_STATIC_CHECK(std::is_same<node_type_of_t<nct_root_only>,
                                   nonesuch>::value);
    D_NC_STATIC_CHECK(std::is_same<node_type_of_t<int>, nonesuch>::value);
    D_NC_STATIC_CHECK(std::is_same<node_type_of_t<void>, nonesuch>::value);
    D_NC_STATIC_CHECK(std::is_same<node_type_of_t<nonesuch>,
                                   nonesuch>::value);
    D_NC_STATIC_CHECK(std::is_same<node_type_of_t<int*>, nonesuch>::value);

    // the extractor is total: even a function type answers
    D_NC_STATIC_CHECK(std::is_same<node_type_of_t<void(int)>,
                                   nonesuch>::value);

    // the internal helper agrees with the published alias
    D_NC_STATIC_CHECK(
        std::is_same<internal::node_type_helper<nct_tree>::type,
                     node_type_of_t<nct_tree>>::value);
    D_NC_STATIC_CHECK(
        std::is_same<internal::node_type_helper<nct_bare>::type,
                     node_type_of_t<nct_bare>>::value);

    // and its two specialisations are selected on the probe's value
    D_NC_STATIC_CHECK(
        std::is_same<internal::node_type_helper<nct_tree, true>::type,
                     nct_node>::value);
    D_NC_STATIC_CHECK(
        std::is_same<internal::node_type_helper<nct_tree, false>::type,
                     nonesuch>::value);

    // the ambiguity: declared-as-nonesuch is indistinguishable by type alone
    D_NC_STATIC_CHECK(
        std::is_same<node_type_of_t<nct_nonesuch_node_type>,
                     node_type_of_t<nct_bare>>::value);
    D_NC_CHECK(   ( has_node_type<nct_nonesuch_node_type>::value == true )
               && ( has_node_type<nct_bare>::value               == false ));

    // run-time readable forms
    D_NC_CHECK(std::is_same<node_type_of_t<nct_bare>, nonesuch>::value);
    D_NC_CHECK(std::is_same<node_type_of_t<void>, nonesuch>::value);

    return ok;
}

/*
tests_node_type_cv_ref
  Verifies that the section III probe honours the clean_t contract.
  Tests the following:
  - has_node_type answers identically for all seven cv- and reference-qualified
    spellings, on a positive and a negative carrier
  - the volatile spellings in particular, which is where section II diverges -
    a container's node_type is still findable through a volatile handle even
    though its accessors are not
*/
bool
tests_node_type_cv_ref()
{
    bool ok = true;

    D_NC_CV_REF_AGREE(has_node_type, nct_tree,      true);
    D_NC_CV_REF_AGREE(has_node_type, nct_graph,     true);
    D_NC_CV_REF_AGREE(has_node_type, nct_node_only, true);
    D_NC_CV_REF_AGREE(has_node_type, nct_bare,      false);
    D_NC_CV_REF_AGREE(has_node_type, nct_root_only, false);
    D_NC_CV_REF_AGREE(has_node_type, int,           false);

    // the contrast with section II, on one carrier: the alias survives a
    // volatile handle, the accessor does not
    D_NC_CHECK(   ( has_node_type<volatile nct_tree>::value   == true )
               && ( has_root_method<volatile nct_tree>::value == false ));

    return ok;
}


NS_END  // testing
NS_END  // djinterp
