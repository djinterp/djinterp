/*******************************************************************************
* djinterp [core]                           metadata_traits_tests_extraction.cpp
*
*
* path:      /tests/djinterp/core/util/metadata/metadata_traits_tests_extraction.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "metadata_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_metadata_extraction
  The `_t` aliases yield the type behind whichever shape is present.
  Tests the following:
  - each alias resolves through all three shapes to the same answer, so a
    caller need not know how the queried type spelled its protocol
  - cv- and reference-qualifiers are stripped: a member declared
    `const md_payload&` extracts as a bare md_payload, which is what makes
    the result usable as a value type rather than as whatever the author
    of the queried class happened to declare
  - the three aliases extract independently, and a protocol-conforming
    type carrying all three names yields the right type for each - the
    holder type for the container name, the payload type for the other
    two, so a crossed wire between the families would be visible here
*/
bool
tests_metadata_extraction()
{
    // `metadata`, through each shape
    D_MT_CHECK(std::is_same_v<metadata_t<md_member>, md_payload>);
    D_MT_CHECK(std::is_same_v<metadata_t<md_method>, md_payload>);
    D_MT_CHECK(std::is_same_v<metadata_t<md_nested>, md_payload>);

    // `metadata_type`, through each shape
    D_MT_CHECK(
     std::is_same_v<metadata_type_t<mdt_member>, md_payload>);
    D_MT_CHECK(
     std::is_same_v<metadata_type_t<mdt_method>, md_payload>);
    D_MT_CHECK(
     std::is_same_v<metadata_type_t<mdt_nested>, md_payload>);

    // `metadata_container_type`, through each shape
    D_MT_CHECK(
     std::is_same_v<metadata_container_type_t<mdc_member>, md_holder>);
    D_MT_CHECK(
     std::is_same_v<metadata_container_type_t<mdc_method>, md_holder>);
    D_MT_CHECK(
     std::is_same_v<metadata_container_type_t<mdc_nested>, md_holder>);

    // qualifiers are stripped from the extracted type
    D_MT_CHECK(std::is_same_v<metadata_t<md_ref_member>, md_payload>);
    D_MT_CHECK(
     !std::is_reference_v<metadata_t<md_ref_member>>);
    D_MT_CHECK(!std::is_const_v<metadata_t<md_ref_member>>);

    // a protocol-conforming type: three names, three correct answers
    D_MT_CHECK(std::is_same_v<metadata_t<md_all_names>, md_payload>);
    D_MT_CHECK(
     std::is_same_v<metadata_type_t<md_all_names>, md_payload>);
    D_MT_CHECK(
     std::is_same_v<metadata_container_type_t<md_all_names>,
                    md_holder>);

    // ... and the container name really does yield the OTHER type, which
    // is the check that fails if two families were ever crossed
    D_MT_CHECK(
     !std::is_same_v<metadata_container_type_t<md_all_names>,
                     metadata_type_t<md_all_names>>);

    return true;
}

/*
tests_metadata_extraction_dispatch
  Which shape wins when more than one is present.
  Tests the following:
  - the documented precedence is nested alias, then data member, then
    member function.  Only the second boundary is observable: a class
    cannot carry a nested type and a member of the same name, since the
    member name space forbids it, so no fixture can put the alias in
    competition with anything.  The member-versus-method boundary CAN be
    reached, through a data member of callable type
  - for that case the data member wins: extraction yields the member's own
    type, not the return type of calling it.  Those are different types
    here by construction, so the check distinguishes them
  This is what makes the module's four-way bool dispatch load-bearing
  rather than ceremonial, despite its comment describing the shapes as
  mutually exclusive.
*/
bool
tests_metadata_extraction_dispatch()
{
    // the fixture really does satisfy both competing shapes
    D_MT_CHECK(has_metadata_data_member<md_callable_member>::value &&
               has_metadata_method<md_callable_member>::value);

    // data member wins: the member's own type, not the call's result
    D_MT_CHECK(
     std::is_same_v<metadata_t<md_callable_member>, md_callable>);
    D_MT_CHECK(
     !std::is_same_v<metadata_t<md_callable_member>, md_payload>);

    // the two candidate answers are genuinely different types, so the
    // check above could have failed
    D_MT_CHECK(!std::is_same_v<md_callable, md_payload>);
    D_MT_CHECK(
     std::is_same_v<decltype(std::declval<const md_callable&>()()),
                    md_payload>);

    // where only one shape is present there is nothing to arbitrate, and
    // each still resolves to the same payload type
    D_MT_CHECK(std::is_same_v<metadata_t<md_member>, md_payload>);
    D_MT_CHECK(std::is_same_v<metadata_t<md_method>, md_payload>);

    return true;
}

/*
tests_metadata_extraction_absent
  Extraction when the queried type has no such shape.
  Tests the following:
  - the module documents extraction as ill-formed when no shape is
    present.  The primary helper template simply has no `type` member, so
    the failure is a SUBSTITUTION failure rather than a hard error, and a
    caller can therefore detect it instead of being stopped by it
  - that is stated with a detection probe rather than by triggering the
    error, so the negative case is an assertion here and not a comment
  - the probe reports true wherever extraction genuinely works, so it is
    not vacuously false
  - each alias fails independently: a type carrying one name can be
    queried for the other two and will fail for exactly those
*/
bool
tests_metadata_extraction_absent()
{
    // no shape at all: every alias is a substitution failure
    D_MT_CHECK(!mt_extracts_metadata<md_none>::value);
    D_MT_CHECK(!mt_extracts_metadata_type<md_none>::value);
    D_MT_CHECK(!mt_extracts_container_type<md_none>::value);

    // non-class types likewise
    D_MT_CHECK(!mt_extracts_metadata<int>::value);
    D_MT_CHECK(!mt_extracts_metadata<void>::value);
    D_MT_CHECK(!mt_extracts_metadata<md_similar>::value);

    // the probe is not vacuously false - it reports true where the shape
    // really is present
    D_MT_CHECK(mt_extracts_metadata<md_member>::value);
    D_MT_CHECK(mt_extracts_metadata<md_method>::value);
    D_MT_CHECK(mt_extracts_metadata<md_nested>::value);
    D_MT_CHECK(mt_extracts_metadata_type<mdt_nested>::value);
    D_MT_CHECK(mt_extracts_container_type<mdc_nested>::value);

    // and each alias fails on its own account: carrying one name does
    // not make the other two extractable
    D_MT_CHECK(mt_extracts_metadata<md_member>::value &&
               !mt_extracts_metadata_type<md_member>::value &&
               !mt_extracts_container_type<md_member>::value);
    D_MT_CHECK(mt_extracts_metadata_type<mdt_nested>::value &&
               !mt_extracts_metadata<mdt_nested>::value &&
               !mt_extracts_container_type<mdt_nested>::value);

    // a fully conforming type extracts on all three
    D_MT_CHECK(mt_extracts_metadata<md_all_names>::value &&
               mt_extracts_metadata_type<md_all_names>::value &&
               mt_extracts_container_type<md_all_names>::value);

    return true;
}


NS_END  // testing
NS_END  // djinterp
