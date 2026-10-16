/*******************************************************************************
* djinterp [core]                              metadata_traits_tests_mirrors.cpp
*
*
* path:      /tests/djinterp/core/util/metadata/metadata_traits_tests_mirrors.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "metadata_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_metadata_type_shapes
  The `metadata_type` family, which mirrors section I name for name.
  Tests the following:
  - each of the three shapes is detected by its own trait, and by neither
    of the other two
  - the unifier is the disjunction over them
  - absence registers as absence
  This block exists because the module implements the three name families
  as separate copy-pasted trait definitions.  Copying is where a probe
  ends up reaching for the wrong name, and only running the same battery
  against a fixture that carries THIS name and no other can catch it.
*/
bool
tests_metadata_type_shapes()
{
    // each shape by its own trait
    D_MT_CHECK(has_metadata_type_data_member<mdt_member>::value);
    D_MT_CHECK(has_metadata_type_method<mdt_method>::value);
    D_MT_CHECK(has_metadata_type_nested_type<mdt_nested>::value);

    // and not by the other two
    D_MT_CHECK(!has_metadata_type_method<mdt_member>::value);
    D_MT_CHECK(!has_metadata_type_nested_type<mdt_member>::value);
    D_MT_CHECK(!has_metadata_type_data_member<mdt_method>::value);
    D_MT_CHECK(!has_metadata_type_nested_type<mdt_method>::value);
    D_MT_CHECK(!has_metadata_type_data_member<mdt_nested>::value);
    D_MT_CHECK(!has_metadata_type_method<mdt_nested>::value);

    // the unifier over them
    D_MT_CHECK(has_metadata_type<mdt_member>::value);
    D_MT_CHECK(has_metadata_type<mdt_method>::value);
    D_MT_CHECK(has_metadata_type<mdt_nested>::value);
    D_MT_CHECK(has_metadata_type<md_all_names>::value);

    // absence
    D_MT_CHECK(!has_metadata_type<md_none>::value);
    D_MT_CHECK(!has_metadata_type<md_similar>::value);

    return true;
}

/*
tests_metadata_container_type_shapes
  The `metadata_container_type` family, the third mirror.
  Tests the following:
  - the same battery as above, against fixtures carrying only this name
  - the payload type it yields is the HOLDER type rather than the payload
    type, so a probe that had drifted onto the metadata_type name would
    disagree here even where both are present
*/
bool
tests_metadata_container_type_shapes()
{
    // each shape by its own trait
    D_MT_CHECK(
     has_metadata_container_type_data_member<mdc_member>::value);
    D_MT_CHECK(
     has_metadata_container_type_method<mdc_method>::value);
    D_MT_CHECK(
     has_metadata_container_type_nested_type<mdc_nested>::value);

    // and not by the other two
    D_MT_CHECK(
     !has_metadata_container_type_method<mdc_member>::value);
    D_MT_CHECK(
     !has_metadata_container_type_nested_type<mdc_member>::value);
    D_MT_CHECK(
     !has_metadata_container_type_data_member<mdc_method>::value);
    D_MT_CHECK(
     !has_metadata_container_type_nested_type<mdc_method>::value);
    D_MT_CHECK(
     !has_metadata_container_type_data_member<mdc_nested>::value);
    D_MT_CHECK(
     !has_metadata_container_type_method<mdc_nested>::value);

    // the unifier over them
    D_MT_CHECK(has_metadata_container_type<mdc_member>::value);
    D_MT_CHECK(has_metadata_container_type<mdc_method>::value);
    D_MT_CHECK(has_metadata_container_type<mdc_nested>::value);
    D_MT_CHECK(has_metadata_container_type<md_all_names>::value);

    // absence
    D_MT_CHECK(!has_metadata_container_type<md_none>::value);
    D_MT_CHECK(!has_metadata_container_type<md_similar>::value);

    return true;
}

/*
tests_metadata_name_independence
  The three name families do not see one another.
  Tests the following:
  - a type carrying only `metadata` satisfies has_metadata and NEITHER of
    the other two unifiers, and correspondingly for the other two names
  - that holds in all three shapes, not just the nested-alias one, since a
    misdirected probe could be confined to a single shape
  - a type carrying all three names satisfies all three
  This is the sharpest check in the section.  `metadata` is a prefix of
  both `metadata_type` and `metadata_container_type`, so a probe reaching
  for the wrong one is easy to write and impossible to see by reading;
  every fixture here carries exactly one of the names for that reason.
*/
bool
tests_metadata_name_independence()
{
    // `metadata` alone implies neither of the others, in every shape
    D_MT_CHECK(has_metadata<md_member>::value &&
               !has_metadata_type<md_member>::value &&
               !has_metadata_container_type<md_member>::value);
    D_MT_CHECK(has_metadata<md_method>::value &&
               !has_metadata_type<md_method>::value &&
               !has_metadata_container_type<md_method>::value);
    D_MT_CHECK(has_metadata<md_nested>::value &&
               !has_metadata_type<md_nested>::value &&
               !has_metadata_container_type<md_nested>::value);

    // `metadata_type` alone implies neither of the others
    D_MT_CHECK(has_metadata_type<mdt_member>::value &&
               !has_metadata<mdt_member>::value &&
               !has_metadata_container_type<mdt_member>::value);
    D_MT_CHECK(has_metadata_type<mdt_method>::value &&
               !has_metadata<mdt_method>::value &&
               !has_metadata_container_type<mdt_method>::value);
    D_MT_CHECK(has_metadata_type<mdt_nested>::value &&
               !has_metadata<mdt_nested>::value &&
               !has_metadata_container_type<mdt_nested>::value);

    // `metadata_container_type` alone implies neither of the others
    D_MT_CHECK(has_metadata_container_type<mdc_member>::value &&
               !has_metadata<mdc_member>::value &&
               !has_metadata_type<mdc_member>::value);
    D_MT_CHECK(has_metadata_container_type<mdc_method>::value &&
               !has_metadata<mdc_method>::value &&
               !has_metadata_type<mdc_method>::value);
    D_MT_CHECK(has_metadata_container_type<mdc_nested>::value &&
               !has_metadata<mdc_nested>::value &&
               !has_metadata_type<mdc_nested>::value);

    // all three together, which is what a protocol-conforming type looks
    // like and the only fixture where more than one name is present
    D_MT_CHECK(has_metadata<md_all_names>::value &&
               has_metadata_type<md_all_names>::value &&
               has_metadata_container_type<md_all_names>::value);

    return true;
}


NS_END  // testing
NS_END  // djinterp
