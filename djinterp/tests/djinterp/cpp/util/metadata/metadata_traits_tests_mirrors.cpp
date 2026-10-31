/*******************************************************************************
* djinterp [cpp]                               metadata_traits_tests_mirrors.cpp
*
*
* path:      /tests/djinterp/cpp/util/metadata/metadata_traits_tests_mirrors.cpp
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
bool tests_metadata_type_shapes()
{
    // each shape by its own trait
    D_MT_CHECK(has_metadata_type_data_member<DMdtMember>::value);
    D_MT_CHECK(has_metadata_type_method<DMdtMethod>::value);
    D_MT_CHECK(has_metadata_type_nested_type<DMdtNested>::value);

    // and not by the other two
    D_MT_CHECK(!has_metadata_type_method<DMdtMember>::value);
    D_MT_CHECK(!has_metadata_type_nested_type<DMdtMember>::value);
    D_MT_CHECK(!has_metadata_type_data_member<DMdtMethod>::value);
    D_MT_CHECK(!has_metadata_type_nested_type<DMdtMethod>::value);
    D_MT_CHECK(!has_metadata_type_data_member<DMdtNested>::value);
    D_MT_CHECK(!has_metadata_type_method<DMdtNested>::value);

    // the unifier over them
    D_MT_CHECK(has_metadata_type<DMdtMember>::value);
    D_MT_CHECK(has_metadata_type<DMdtMethod>::value);
    D_MT_CHECK(has_metadata_type<DMdtNested>::value);
    D_MT_CHECK(has_metadata_type<DMdAllNames>::value);

    // absence
    D_MT_CHECK(!has_metadata_type<DMdNone>::value);
    D_MT_CHECK(!has_metadata_type<DMdSimilar>::value);

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
bool tests_metadata_container_type_shapes()
{
    // each shape by its own trait
    D_MT_CHECK(
     has_metadata_container_type_data_member<DMdcMember>::value);
    D_MT_CHECK(
     has_metadata_container_type_method<DMdcMethod>::value);
    D_MT_CHECK(
     has_metadata_container_type_nested_type<DMdcNested>::value);

    // and not by the other two
    D_MT_CHECK(
     !has_metadata_container_type_method<DMdcMember>::value);
    D_MT_CHECK(
     !has_metadata_container_type_nested_type<DMdcMember>::value);
    D_MT_CHECK(
     !has_metadata_container_type_data_member<DMdcMethod>::value);
    D_MT_CHECK(
     !has_metadata_container_type_nested_type<DMdcMethod>::value);
    D_MT_CHECK(
     !has_metadata_container_type_data_member<DMdcNested>::value);
    D_MT_CHECK(
     !has_metadata_container_type_method<DMdcNested>::value);

    // the unifier over them
    D_MT_CHECK(has_metadata_container_type<DMdcMember>::value);
    D_MT_CHECK(has_metadata_container_type<DMdcMethod>::value);
    D_MT_CHECK(has_metadata_container_type<DMdcNested>::value);
    D_MT_CHECK(has_metadata_container_type<DMdAllNames>::value);

    // absence
    D_MT_CHECK(!has_metadata_container_type<DMdNone>::value);
    D_MT_CHECK(!has_metadata_container_type<DMdSimilar>::value);

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
bool tests_metadata_name_independence()
{
    // `metadata` alone implies neither of the others, in every shape
    D_MT_CHECK(has_metadata<DMdMember>::value &&
               !has_metadata_type<DMdMember>::value &&
               !has_metadata_container_type<DMdMember>::value);
    D_MT_CHECK(has_metadata<DMdMethod>::value &&
               !has_metadata_type<DMdMethod>::value &&
               !has_metadata_container_type<DMdMethod>::value);
    D_MT_CHECK(has_metadata<DMdNested>::value &&
               !has_metadata_type<DMdNested>::value &&
               !has_metadata_container_type<DMdNested>::value);

    // `metadata_type` alone implies neither of the others
    D_MT_CHECK(has_metadata_type<DMdtMember>::value &&
               !has_metadata<DMdtMember>::value &&
               !has_metadata_container_type<DMdtMember>::value);
    D_MT_CHECK(has_metadata_type<DMdtMethod>::value &&
               !has_metadata<DMdtMethod>::value &&
               !has_metadata_container_type<DMdtMethod>::value);
    D_MT_CHECK(has_metadata_type<DMdtNested>::value &&
               !has_metadata<DMdtNested>::value &&
               !has_metadata_container_type<DMdtNested>::value);

    // `metadata_container_type` alone implies neither of the others
    D_MT_CHECK(has_metadata_container_type<DMdcMember>::value &&
               !has_metadata<DMdcMember>::value &&
               !has_metadata_type<DMdcMember>::value);
    D_MT_CHECK(has_metadata_container_type<DMdcMethod>::value &&
               !has_metadata<DMdcMethod>::value &&
               !has_metadata_type<DMdcMethod>::value);
    D_MT_CHECK(has_metadata_container_type<DMdcNested>::value &&
               !has_metadata<DMdcNested>::value &&
               !has_metadata_type<DMdcNested>::value);

    // all three together, which is what a protocol-conforming type looks
    // like and the only fixture where more than one name is present
    D_MT_CHECK(has_metadata<DMdAllNames>::value &&
               has_metadata_type<DMdAllNames>::value &&
               has_metadata_container_type<DMdAllNames>::value);

    return true;
}


NS_END  // testing
NS_END  // djinterp
