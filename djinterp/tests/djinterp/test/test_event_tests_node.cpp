/*******************************************************************************
* djinterp [test]                                      test_event_tests_node.cpp
*
*   Section I of the test_event suite: the node forward declarations.
*
*   This section is small but it guards the header's structural reason for
* existing.  test_event.hpp deliberately does NOT include test_object.hpp: it
* forward-declares the test_object template and spells `basic_test` as a pointer
* payload, so the alphabet stays a LEAF of the include graph (test_handler.hpp
* and test_builder.hpp both include it first, before the node type is complete).
* The tests below pin both halves of that:
*
*     - the alias resolves to the right node type, and agrees with the
*       re-declaration test_object.hpp makes (the header claims this is a "legal
*       redeclaration" - it only is if the two spellings name the same type);
*     - a tag's payload really is a POINTER, and D_EVENT really does accept an
*       INCOMPLETE type - demonstrated against a type that is never defined at
*       all, which is the strongest form of the claim.
*
*
* path:      /tests/djinterp/test/test_event_tests_node.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <cstdint>
#include <tuple>
#include <type_traits>
// djinterp
#include "test_event_tests.hpp"


// ---------------------------------------------------------------------------
//   THE INCOMPLETE-TYPE PROBE
//
//   `incomplete_probe` is declared and NEVER defined.  Declaring an event over
// a pointer to it must still compile and must still yield a well-formed payload
// tuple - which is precisely the property that lets test_event.hpp name
// basic_test before test_object.hpp completes it.  If D_EVENT ever started
// requiring a complete type (by, say, storing the payload by value), this
// translation unit would stop compiling.
// ---------------------------------------------------------------------------
namespace
{
    struct incomplete_probe;          // declared, deliberately never defined
}

D_EVENT(on_incomplete_probe, const incomplete_probe*);

static_assert(
    std::is_same<
        typename ::djinterp::event_traits<on_incomplete_probe>::payload_type,
        std::tuple<const incomplete_probe*> >::value,
    "D_EVENT must accept an incomplete payload type (pointer payload)");


NS_DJINTERP
NS_TESTING


// tests_event_basic_test_is_the_default_test_object
//   the alias spelled in test_event.hpp - test_object<std::uint8_t,
// test_metadata> - is what basic_test resolves to.
bool
tests_event_basic_test_is_the_default_test_object()
{
    D_TE_CHECK((std::is_same<
                    dt::basic_test,
                    dt::test_object<std::uint8_t, dt::test_metadata> >::value));

    return true;
}


// tests_event_basic_test_agrees_with_test_object_realias
//   test_event.hpp spells the alias EXPLICITLY (it cannot see test_object.hpp's
// default template arguments); test_object.hpp then re-aliases the same name as
// test_object<>.  The header calls that "a legal redeclaration" - which holds
// only if both spellings name one type.  This is that assertion, and it is why
// including both headers in one TU (as this one does) is well-formed.
bool
tests_event_basic_test_agrees_with_test_object_realias()
{
    D_TE_CHECK((std::is_same<dt::basic_test, dt::test_object<> >::value));

    return true;
}


// tests_event_basic_test_status_type_is_uint8
//   the node's status domain is the 8-bit one the alias pins.
bool
tests_event_basic_test_status_type_is_uint8()
{
    D_TE_CHECK((std::is_same<dt::basic_test::status_type,
                             std::uint8_t>::value));

    return true;
}


// tests_event_basic_test_metadata_type_is_test_metadata
//   and its metadata CONTAINER is test_common.hpp's basic_metadata<> alias - the
// type test_event.hpp names in the alias without ever completing it.  (Note the
// node's `metadata_type` is the container's value_type, not the container; the
// alias pins `metadata_container_type`.)
bool
tests_event_basic_test_metadata_type_is_test_metadata()
{
    D_TE_CHECK((std::is_same<dt::basic_test::metadata_container_type,
                             dt::test_metadata>::value));
    D_TE_CHECK((std::is_same<dt::test_metadata,
                             dt::basic_metadata<> >::value));

    return true;
}


// tests_event_node_payload_is_a_pointer_not_a_value
//   the payload convention: every node-scoped tag carries the node BY POINTER.
// A by-value payload would force the alphabet to include test_object.hpp and
// would drag the node's whole definition into every listener.
bool
tests_event_node_payload_is_a_pointer_not_a_value()
{
    using first = typename std::tuple_element<0, node_payload>::type;

    D_TE_CHECK((std::is_same<payload_of<dt::on_test_failed>,
                             node_payload>::value));
    D_TE_CHECK(std::is_pointer<first>::value);
    D_TE_CHECK((std::is_same<first, const dt::basic_test*>::value));

    // pointer-to-const: a listener observes the node, it does not mutate it
    D_TE_CHECK(std::is_const<
                   typename std::remove_pointer<first>::type>::value);
    D_TE_CHECK(sizeof(first) == sizeof(void*));

    return true;
}


// tests_event_tags_work_against_an_incomplete_node_type
//   the structural claim, proven at file scope above against a type that is
// never defined: D_EVENT accepts an incomplete pointee and still yields a
// well-formed payload.  This is what lets test_event.hpp sit BELOW
// test_object.hpp in the include chain.  (The static_assert at the top of this
// TU is the real test - it fails the build, not the run; this case reports it.)
bool
tests_event_tags_work_against_an_incomplete_node_type()
{
    D_TE_CHECK((std::is_same<
                    payload_of<on_incomplete_probe>,
                    std::tuple<const incomplete_probe*> >::value));

    // ...and the tag is a perfectly ordinary event despite the incomplete type
    D_TE_CHECK(::djinterp::event_traits<on_incomplete_probe>::arity == 1u);
    D_TE_CHECK(::djinterp::event_traits<on_incomplete_probe>::has_name);

    return true;
}


// tests_event_forward_declaration_accepts_the_option_pack
//   test_event.hpp forward-declares test_object with a trailing option pack
// (<_StatusType, _MetadataContainer, _Options...>) and names it with TWO
// arguments, leaving the pack empty.  If the forward declaration's parameter
// list ever drifted from test_object.hpp's primary template, that two-argument
// spelling would stop matching - so exercising it, and a non-empty pack, pins
// the shape.
bool
tests_event_forward_declaration_accepts_the_option_pack()
{
    // the empty-pack spelling test_event.hpp itself uses
    D_TE_CHECK((std::is_same<
                    dt::test_object<std::uint8_t, dt::test_metadata>,
                    dt::basic_test>::value));

    // a DIFFERENT instantiation is a different type: the parameters are real,
    // not decoration
    D_TE_CHECK(!(std::is_same<
                     dt::test_object<std::uint16_t, dt::test_metadata>,
                     dt::basic_test>::value));

    return true;
}


NS_END   // testing
NS_END   // djinterp
