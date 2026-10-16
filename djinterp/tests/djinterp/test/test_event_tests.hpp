/*******************************************************************************
* djinterp [test]                                           test_event_tests.hpp
*
*   Unit-test suite for test_event.hpp - the DTest event ALPHABET.  This header
* is DECLARATIONS + SHARED FIXTURES only; every test body lives in one of the
* four section translation units:
*
*     test_event_tests_node.cpp       I.   node forward declarations
*                                          (basic_test; the incomplete-type
*                                          contract that keeps this header a
*                                          leaf of the include graph)
*     test_event_tests_alphabet.cpp   II.  the twelve built-in tags: payloads,
*                                          names, distinctness, traits, concepts
*     test_event_tests_macros.cpp     III. D_TEST_EVENT / _MSG / _X
*     test_event_tests_dispatch.cpp   the alphabet's actual PROMISE: a tag -
*                                          built-in or custom - binds and fires
*                                          through the event subsystem with the
*                                          payload it declares
*
*   Every test is a `bool tests_*()` sitting FLAT in djinterp::testing.
*
*   WHAT IS BEING TESTED, EXACTLY:
*   test_event.hpp declares no functions - it is an alphabet.  So "coverage"
* here means every declared TYPE is pinned (payload tuple, name, identity,
* distinctness), every MACRO is expanded and pinned, and the whole alphabet is
* shown to satisfy the event subsystem it is declared against.  The payload
* shapes are the load-bearing part: they must match the framework's fire sites
* (test_handler.hpp fires on_session_start / on_session_end / the node tags), so
* the payload assertions below are what would catch a tag drifting away from its
* caller.
*
*   NAMESPACE NOTE (a real discrepancy, pinned deliberately):
*   test_event.hpp's banner says the built-in tags live "in the
* `djinterp::test::events` sub-namespace".  They do not - the file opens
* NS_DJINTERP / NS_TEST and nothing else, so they are in `djinterp::test`.  There
* is no `events` namespace anywhere in the header.  These tests pin the CODE
* (djinterp::test); if the banner is the intended truth, the header must gain the
* namespace and these tests move with it.
*
*
* path:      /tests/djinterp/test/test_event_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_TEST_EVENT_TESTS_
#define DJINTERP_TESTS_TEST_EVENT_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>
// djinterp
//   The core headers use the FULL-FROM-INC form, so they resolve against
// ${DJINTERP_INC_ROOT} (which djinterp_add_test_executable always supplies) with
// no extra include dirs.  The two test headers are bare, resolved by the leaf's
// INCLUDES (inc/djinterp/test).  djinterp.hpp must come first: event_dispatcher
// #errors out unless DJINTERP_ is already defined.
#include "djinterp/djinterp.hpp"                // NS_*, D_* qualifiers, language gates
#ifndef DTEST_SPEC_MODE
#include "djinterp/core/event/event_dispatcher.hpp"  // event_dispatcher: bind / fire - the
                                                     //   substrate the tags plug into
#include "../../../inc/djinterp/test/test_event.hpp"        // THE HEADER UNDER TEST: basic_test + the 12 tags + the macros
#include "../../../inc/djinterp/test/test_object.hpp"       // completes basic_test, so fixtures can build real nodes
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"           // spec mode: module_spec + run_module
#endif


NS_DJINTERP
NS_TESTING


// dt
//   alias: the namespace under test.  verdict / event_traits / event_dispatcher
// live one level up in ::djinterp and resolve unqualified from here.
namespace dt = ::djinterp::test;


// D_TE_CHECK
//   macro: assert inside a test body; on failure it reports the expression and
// its location and returns false.  Unique letters (TE = test_event) so
// co-compiled suites never collide on the macro name.
#define D_TE_CHECK(_cond)                                                      \
    do                                                                         \
    {                                                                          \
        if (!(_cond))                                                          \
        {                                                                      \
            std::printf("        check failed:  %s\n"                          \
                        "        at:            %s:%d\n",                      \
                        #_cond, __FILE__, __LINE__);                           \
            return false;                                                      \
        }                                                                      \
    }                                                                          \
    while (0)


#ifndef DTEST_SPEC_MODE  // fixtures: normal (section-file) mode only (they depend on event_traits)


///////////////////////////////////////////////////////////////////////////////
///                FIXTURES                                                  ///
///////////////////////////////////////////////////////////////////////////////

// k_node_name
//   the metadata name planted in the fixture node, so a listener that receives
// a `const basic_test*` can be shown to have received a USABLE node and not
// merely a non-null address.
inline const char*
k_node_name()
{
    return "the fixture node";
}


// make_node
//   a real basic_test carrying a type id and a metadata name.  The lifecycle
// tags all carry `const basic_test*`, so this is the payload every node-scoped
// dispatch test fires with.
D_NODISCARD inline dt::basic_test
make_node()
{
    dt::basic_test _n(static_cast<dt::test_type_id>(7));

    _n.metadata().set("name", k_node_name());

    return _n;
}


// payload_of
//   alias: the payload tuple the event subsystem derives for a tag.  Every
// payload assertion in the suite goes through event_traits (rather than reading
// _Event::payload_type directly), so it tests the tag AS THE SUBSYSTEM SEES IT -
// which is the only thing that matters at a fire site.
template<typename _Event>
using payload_of = typename ::djinterp::event_traits<_Event>::payload_type;


// node_payload
//   the payload shape shared by every node-scoped lifecycle tag.
using node_payload = std::tuple<const dt::basic_test*>;


// call_log
//   a shared, copyable record of what a bound listener saw.  Listeners are
// stored type-erased by the dispatcher, so the observable has to outlive the
// lambda by shared ownership.
struct call_log
{
    int                       calls   = 0;
    const dt::basic_test*     node    = nullptr;
    std::size_t               a       = 0;
    std::size_t               b       = 0;
    dt::test_status           before  = dt::test_status::pending;
    dt::test_status           after   = dt::test_status::pending;
    std::string               text_1;
    std::string               text_2;
};

using log_ptr = std::shared_ptr<call_log>;

D_NODISCARD inline log_ptr
make_log()
{
    return std::make_shared<call_log>();
}


#endif  // !DTEST_SPEC_MODE  (fixtures)


///////////////////////////////////////////////////////////////////////////////
///                I.   NODE FORWARD DECLARATIONS  (…_tests_node.cpp)        ///
///////////////////////////////////////////////////////////////////////////////

bool tests_event_basic_test_is_the_default_test_object();
bool tests_event_basic_test_agrees_with_test_object_realias();
bool tests_event_basic_test_status_type_is_uint8();
bool tests_event_basic_test_metadata_type_is_test_metadata();
bool tests_event_node_payload_is_a_pointer_not_a_value();
bool tests_event_tags_work_against_an_incomplete_node_type();
bool tests_event_forward_declaration_accepts_the_option_pack();


///////////////////////////////////////////////////////////////////////////////
///                II.  BUILT-IN ALPHABET  (…_tests_alphabet.cpp)            ///
///////////////////////////////////////////////////////////////////////////////

bool tests_event_on_session_start_has_empty_payload();
bool tests_event_on_session_end_carries_two_counts();
bool tests_event_module_tags_carry_the_node();
bool tests_event_test_lifecycle_tags_carry_the_node();
bool tests_event_terminal_status_tags_carry_the_node();
bool tests_event_on_test_error_carries_node_and_message();
bool tests_event_on_status_change_carries_node_and_both_statuses();
bool tests_event_on_listener_threw_carries_two_c_strings();
bool tests_event_every_tag_names_its_own_identifier();
bool tests_event_every_tag_is_a_distinct_type();
bool tests_event_every_tag_satisfies_event_traits();
bool tests_event_arity_agrees_with_payload_size();
bool tests_event_node_tags_all_share_one_payload_shape();
bool tests_event_has_args_partitions_the_alphabet();
bool tests_event_tags_satisfy_the_event_concepts();


///////////////////////////////////////////////////////////////////////////////
///                III. CUSTOM EVENT MACROS  (…_tests_macros.cpp)            ///
///////////////////////////////////////////////////////////////////////////////

bool tests_event_d_test_event_declares_a_node_tag();
bool tests_event_d_test_event_matches_the_builtin_lifecycle_shape();
bool tests_event_d_test_event_msg_declares_a_node_plus_message_tag();
bool tests_event_d_test_event_msg_matches_the_on_test_error_shape();
bool tests_event_d_test_event_x_appends_one_extra_domain();
bool tests_event_d_test_event_x_appends_several_extra_domains();
bool tests_event_custom_tags_name_their_own_identifiers();
bool tests_event_custom_tags_are_distinct_from_builtins();
bool tests_event_custom_tags_are_distinct_from_each_other();
bool tests_event_macros_are_namespace_agnostic();
bool tests_event_custom_tags_satisfy_event_traits();


///////////////////////////////////////////////////////////////////////////////
///                IV.  DISPATCH ROUND TRIP  (…_tests_dispatch.cpp)          ///
///////////////////////////////////////////////////////////////////////////////

bool tests_event_dispatch_binds_and_fires_a_node_tag();
bool tests_event_dispatch_delivers_a_usable_node();
bool tests_event_dispatch_empty_payload_tag_fires_with_no_arguments();
bool tests_event_dispatch_two_count_payload_arrives_intact();
bool tests_event_dispatch_status_change_payload_arrives_intact();
bool tests_event_dispatch_error_payload_carries_the_message();
bool tests_event_dispatch_listener_threw_payload_arrives_intact();
bool tests_event_dispatch_custom_tag_binds_exactly_like_a_builtin();
bool tests_event_dispatch_void_listener_is_an_always_pass();
bool tests_event_dispatch_consume_verdict_halts_propagation();
bool tests_event_dispatch_every_bound_listener_is_invoked();
bool tests_event_dispatch_tags_are_independent_channels();
bool tests_event_dispatch_unbound_tag_invokes_nothing();
bool tests_event_dispatch_unbind_removes_the_listener();


#ifdef DTEST_SPEC_MODE

// =========================================================================
//  suite spec provider (spec mode)
//    Descriptors are drawn from what each test actually asserts.
// =========================================================================

inline dt::module_spec
event_spec()
{
    return dt::module_spec{
        "test_event",
        "test_event.hpp - the DTest event ALPHABET: the forward-declared node "
        "type, the twelve built-in tags (payloads, names, distinctness, traits, "
        "concepts), the D_TEST_EVENT custom-tag macros, and the dispatch round "
        "trip that proves each tag binds and fires with its declared payload.",
        {
            dt::block_spec{
                "node",
                "The node forward declaration and the incomplete-type contract "
                "that keeps this header a leaf of the include graph.",
                {
                    { "tests_event_basic_test_is_the_default_test_object",     "basic_test is the default test_object instantiation", &tests_event_basic_test_is_the_default_test_object },
                    { "tests_event_basic_test_agrees_with_test_object_realias","the forward-declared basic_test agrees with the test_object re-alias", &tests_event_basic_test_agrees_with_test_object_realias },
                    { "tests_event_basic_test_status_type_is_uint8",          "basic_test's status_type is uint8", &tests_event_basic_test_status_type_is_uint8 },
                    { "tests_event_basic_test_metadata_type_is_test_metadata","basic_test's metadata container is test_metadata", &tests_event_basic_test_metadata_type_is_test_metadata },
                    { "tests_event_node_payload_is_a_pointer_not_a_value",    "a node tag's payload is a pointer to the node, not a value", &tests_event_node_payload_is_a_pointer_not_a_value },
                    { "tests_event_tags_work_against_an_incomplete_node_type","the tags are usable against an incomplete node type (the leaf-of-include-graph contract)", &tests_event_tags_work_against_an_incomplete_node_type },
                    { "tests_event_forward_declaration_accepts_the_option_pack","the node forward declaration accepts the option pack", &tests_event_forward_declaration_accepts_the_option_pack },
                }
            },
            dt::block_spec{
                "alphabet",
                "The twelve built-in tags: payload shapes, names, distinctness, "
                "and the event_traits / concept surface.",
                {
                    { "tests_event_on_session_start_has_empty_payload",              "on_session_start carries an empty payload", &tests_event_on_session_start_has_empty_payload },
                    { "tests_event_on_session_end_carries_two_counts",               "on_session_end carries two count arguments", &tests_event_on_session_end_carries_two_counts },
                    { "tests_event_module_tags_carry_the_node",                      "the module lifecycle tags carry the node pointer", &tests_event_module_tags_carry_the_node },
                    { "tests_event_test_lifecycle_tags_carry_the_node",              "the test lifecycle tags carry the node pointer", &tests_event_test_lifecycle_tags_carry_the_node },
                    { "tests_event_terminal_status_tags_carry_the_node",             "the terminal-status tags carry the node pointer", &tests_event_terminal_status_tags_carry_the_node },
                    { "tests_event_on_test_error_carries_node_and_message",          "on_test_error carries the node plus an error message", &tests_event_on_test_error_carries_node_and_message },
                    { "tests_event_on_status_change_carries_node_and_both_statuses", "on_status_change carries the node and both the before and after statuses", &tests_event_on_status_change_carries_node_and_both_statuses },
                    { "tests_event_on_listener_threw_carries_two_c_strings",         "on_listener_threw carries two C-string arguments", &tests_event_on_listener_threw_carries_two_c_strings },
                    { "tests_event_every_tag_names_its_own_identifier",              "every tag reports its own identifier via name()", &tests_event_every_tag_names_its_own_identifier },
                    { "tests_event_every_tag_is_a_distinct_type",                    "the alphabet has twelve distinct tag types", &tests_event_every_tag_is_a_distinct_type },
                    { "tests_event_every_tag_satisfies_event_traits",                "every tag satisfies the event_traits contract (has a name)", &tests_event_every_tag_satisfies_event_traits },
                    { "tests_event_arity_agrees_with_payload_size",                  "each tag's arity agrees with its payload tuple size", &tests_event_arity_agrees_with_payload_size },
                    { "tests_event_node_tags_all_share_one_payload_shape",           "all node-scoped tags share the one node-pointer payload shape", &tests_event_node_tags_all_share_one_payload_shape },
                    { "tests_event_has_args_partitions_the_alphabet",                "has_args partitions the alphabet into empty and payload-carrying tags", &tests_event_has_args_partitions_the_alphabet },
                    { "tests_event_tags_satisfy_the_event_concepts",                 "the tags satisfy the event concepts, where concepts are available", &tests_event_tags_satisfy_the_event_concepts },
                }
            },
            dt::block_spec{
                "macros",
                "The custom-event macros: D_TEST_EVENT / _MSG / _X.",
                {
                    { "tests_event_d_test_event_declares_a_node_tag",             "D_TEST_EVENT declares a node-scoped tag", &tests_event_d_test_event_declares_a_node_tag },
                    { "tests_event_d_test_event_matches_the_builtin_lifecycle_shape","a D_TEST_EVENT tag matches the built-in lifecycle payload shape", &tests_event_d_test_event_matches_the_builtin_lifecycle_shape },
                    { "tests_event_d_test_event_msg_declares_a_node_plus_message_tag","D_TEST_EVENT_MSG declares a node-plus-message tag", &tests_event_d_test_event_msg_declares_a_node_plus_message_tag },
                    { "tests_event_d_test_event_msg_matches_the_on_test_error_shape","a D_TEST_EVENT_MSG tag matches the on_test_error payload shape", &tests_event_d_test_event_msg_matches_the_on_test_error_shape },
                    { "tests_event_d_test_event_x_appends_one_extra_domain",      "D_TEST_EVENT_X appends one extra payload domain", &tests_event_d_test_event_x_appends_one_extra_domain },
                    { "tests_event_d_test_event_x_appends_several_extra_domains", "D_TEST_EVENT_X appends several extra payload domains", &tests_event_d_test_event_x_appends_several_extra_domains },
                    { "tests_event_custom_tags_name_their_own_identifiers",       "custom tags report their own identifiers via name()", &tests_event_custom_tags_name_their_own_identifiers },
                    { "tests_event_custom_tags_are_distinct_from_builtins",       "custom tags are distinct types from the built-ins", &tests_event_custom_tags_are_distinct_from_builtins },
                    { "tests_event_custom_tags_are_distinct_from_each_other",     "distinct custom tags are distinct types", &tests_event_custom_tags_are_distinct_from_each_other },
                    { "tests_event_macros_are_namespace_agnostic",               "the event macros work in any namespace", &tests_event_macros_are_namespace_agnostic },
                    { "tests_event_custom_tags_satisfy_event_traits",            "custom tags satisfy the event_traits contract", &tests_event_custom_tags_satisfy_event_traits },
                }
            },
            dt::block_spec{
                "dispatch",
                "The dispatch round trip: every tag - built-in or custom - binds "
                "and fires through the subsystem with its declared payload.",
                {
                    { "tests_event_dispatch_binds_and_fires_a_node_tag",            "a node tag binds and fires through the dispatcher", &tests_event_dispatch_binds_and_fires_a_node_tag },
                    { "tests_event_dispatch_delivers_a_usable_node",                "the listener receives a usable node, not merely a non-null pointer", &tests_event_dispatch_delivers_a_usable_node },
                    { "tests_event_dispatch_empty_payload_tag_fires_with_no_arguments","an empty-payload tag fires its listener with no arguments", &tests_event_dispatch_empty_payload_tag_fires_with_no_arguments },
                    { "tests_event_dispatch_two_count_payload_arrives_intact",      "a two-count payload arrives at the listener intact", &tests_event_dispatch_two_count_payload_arrives_intact },
                    { "tests_event_dispatch_status_change_payload_arrives_intact",  "the status-change payload (node + both statuses) arrives intact", &tests_event_dispatch_status_change_payload_arrives_intact },
                    { "tests_event_dispatch_error_payload_carries_the_message",     "the error payload carries its message to the listener", &tests_event_dispatch_error_payload_carries_the_message },
                    { "tests_event_dispatch_listener_threw_payload_arrives_intact", "the listener-threw payload (two C-strings) arrives intact", &tests_event_dispatch_listener_threw_payload_arrives_intact },
                    { "tests_event_dispatch_custom_tag_binds_exactly_like_a_builtin","a custom tag binds and fires exactly like a built-in", &tests_event_dispatch_custom_tag_binds_exactly_like_a_builtin },
                    { "tests_event_dispatch_void_listener_is_an_always_pass",       "a void-returning listener is treated as an always-pass", &tests_event_dispatch_void_listener_is_an_always_pass },
                    { "tests_event_dispatch_consume_verdict_halts_propagation",     "a consume verdict from a listener halts further propagation", &tests_event_dispatch_consume_verdict_halts_propagation },
                    { "tests_event_dispatch_every_bound_listener_is_invoked",       "every listener bound to a tag is invoked on a fire", &tests_event_dispatch_every_bound_listener_is_invoked },
                    { "tests_event_dispatch_tags_are_independent_channels",         "distinct tags are independent channels - firing one does not invoke another's listeners", &tests_event_dispatch_tags_are_independent_channels },
                    { "tests_event_dispatch_unbound_tag_invokes_nothing",          "firing an unbound tag invokes nothing", &tests_event_dispatch_unbound_tag_invokes_nothing },
                    { "tests_event_dispatch_unbind_removes_the_listener",          "unbind removes a previously-bound listener", &tests_event_dispatch_unbind_removes_the_listener },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END   // testing
NS_END   // djinterp

#endif   // DJINTERP_TESTS_TEST_EVENT_TESTS_
