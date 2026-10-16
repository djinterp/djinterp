/*******************************************************************************
* djinterp [core]                         container_metadata_tests_detection.cpp
*
*
* path:      /tests/djinterp/core/container/container_metadata_tests_detection.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "container_metadata_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_container_metadata_detection
  is_container_metadata as a structural trait.
  Tests the following:
  - it holds for the template's own specialisations, across payload types
  - it is STRUCTURAL rather than nominal: cm_duck satisfies it without any
    relationship to the template, because it exposes the same four names.
    That is a design decision with consequences - anything shaped alike is
    accepted - so it is stated rather than left implicit
  - ordinary class types and the whole hostile type zoo are rejected, and
    the quantifier instantiates every cell rather than short-circuiting
  - the `_v` companion agrees with `::value`
  - qualification is NOT transparent here: a const-qualified type is still
    detected, since cv-qualifiers are ignored inside a nested-name-
    specifier, but a REFERENCE to a container_metadata is not, because the
    trait names into its argument without cleaning it first.  The metadata
    protocol traits next door do clean, so the two disagree on reference
    forms; that inconsistency is recorded here rather than judged
*/
bool
tests_container_metadata_detection()
{
    // the template's own specialisations
    D_CM_CHECK(is_container_metadata<cm_meta>::value);
    D_CM_CHECK(
     (is_container_metadata<container_metadata<cm_box,
                                               cm_counted>>::value));
    D_CM_CHECK(
     (is_container_metadata<container_metadata<cm_box,
                                               cm_box>>::value));

    // structural, not nominal: an unrelated type of the same shape passes
    D_CM_CHECK(is_container_metadata<cm_duck>::value);

    // ordinary types do not
    D_CM_CHECK(!is_container_metadata<cm_box>::value);
    D_CM_CHECK(!is_container_metadata<cm_note>::value);
    D_CM_CHECK(!is_container_metadata<int>::value);

    // nor does anything in the hostile zoo
    D_CM_CHECK((test::holds_for_none<is_container_metadata,
                                     D_TEST_HOSTILE_TYPES_COMPLETE
                                     >::value));
    D_CM_CHECK(
     (test::is_bool_trait<is_container_metadata<cm_meta>>::value));
    D_CM_CHECK(
     (test::is_bool_trait<is_container_metadata<int>>::value));

    // the `_v` companion tracks the trait
    D_CM_CHECK(is_container_metadata_v<cm_meta> ==
               is_container_metadata<cm_meta>::value);
    D_CM_CHECK(is_container_metadata_v<cm_duck> ==
               is_container_metadata<cm_duck>::value);
    D_CM_CHECK(is_container_metadata_v<int> ==
               is_container_metadata<int>::value);

    // const is transparent, references are not
    D_CM_CHECK(is_container_metadata<const cm_meta>::value);
    D_CM_CHECK(!is_container_metadata<cm_meta&>::value);
    D_CM_CHECK(!is_container_metadata<const cm_meta&>::value);

    // ... whereas the metadata protocol traits DO clean their argument,
    // so the two families answer differently about reference forms
    D_CM_CHECK(has_metadata<cm_protocol_type&>::value);
    D_CM_CHECK(has_metadata<cm_protocol_type>::value ==
               has_metadata<cm_protocol_type&>::value);
    D_CM_CHECK(is_container_metadata<cm_meta>::value !=
               is_container_metadata<cm_meta&>::value);

    return true;
}

/*
tests_container_metadata_detection_requirements
  Each of the trait's four requirements is load-bearing.
  Tests the following:
  - removing the container_type alias, the payload_type alias,
    get_container(), or payload() each drops the type from the trait,
    one at a time, from a fixture that is otherwise complete.  A
    requirement that had been dropped from the trait would show up as one
    of these near-misses being wrongly accepted
  - the accessors must be callable on a CONST object.  cm_nonconst_
    accessors has all four names spelled correctly and is still rejected,
    purely because its two accessors are not const-qualified.  This is the
    sharpest of the set, and the one most likely to bite an author writing
    a type to satisfy the trait deliberately
  - the complete fixture is accepted, so the rejections above are the
    result of the single removal in each case and not of some property
    shared by all five
*/
bool
tests_container_metadata_detection_requirements()
{
    // the complete duck type is accepted
    D_CM_CHECK(is_container_metadata<cm_duck>::value);

    // each single removal is fatal
    D_CM_CHECK(!is_container_metadata<cm_no_container_type>::value);
    D_CM_CHECK(!is_container_metadata<cm_no_payload_type>::value);
    D_CM_CHECK(!is_container_metadata<cm_no_get_container>::value);
    D_CM_CHECK(!is_container_metadata<cm_no_payload_fn>::value);

    // and const-callability of the accessors is required, even with
    // every name present and correctly spelled
    D_CM_CHECK(!is_container_metadata<cm_nonconst_accessors>::value);

    // the near-misses really do carry the parts they were meant to keep,
    // so each rejection above is attributable to the one thing removed
    D_CM_CHECK(
     (std::is_same<cm_no_container_type::payload_type,
                   cm_note>::value));
    D_CM_CHECK(
     (std::is_same<cm_no_payload_type::container_type,
                   cm_box>::value));
    D_CM_CHECK(
     (std::is_same<cm_no_get_container::container_type,
                   cm_box>::value));
    D_CM_CHECK(
     (std::is_same<cm_no_payload_fn::payload_type, cm_note>::value));
    D_CM_CHECK(
     (std::is_same<cm_nonconst_accessors::container_type,
                   cm_box>::value));

    // the `_v` companion agrees on the rejections too
    D_CM_CHECK(!is_container_metadata_v<cm_nonconst_accessors>);
    D_CM_CHECK(is_container_metadata_v<cm_duck>);

    return true;
}


NS_END  // testing
NS_END  // djinterp
