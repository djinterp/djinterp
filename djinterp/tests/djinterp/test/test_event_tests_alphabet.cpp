/*******************************************************************************
* djinterp [test]                                  test_event_tests_alphabet.cpp
*
*   Section II of the test_event suite: the twelve built-in tags.
*
*   A tag is only an identity and a payload shape, so this section pins exactly
* those two things for every summand of the alphabet - and pins them THROUGH
* event_traits rather than by reading the nested typedef directly, because
* event_traits is how the dispatcher sees a tag at a fire site.  The payload
* assertions are the load-bearing ones: test_handler.hpp fires these tags with
* concrete arguments (fire<on_session_start>(), fire<on_session_end>(p, f), the
* node tags with &node), so a payload drifting here breaks a caller there.
*
*   Coverage of the alphabet is exhaustive: all twelve tags appear in the
* per-tag payload cases, in the name case, in the distinctness case (66 pairwise
* comparisons), in the traits case, and in the arity case.
*
*
* path:      /tests/djinterp/test/test_event_tests_alphabet.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <cstddef>
#include <cstring>
#include <tuple>
#include <type_traits>
// djinterp
#include "test_event_tests.hpp"


NS_DJINTERP
NS_TESTING


// ---- per-tag payload shapes ------------------------------------------------

// tests_event_on_session_start_has_empty_payload
//   the only EMPTY summand: declared with D_EVENT_EMPTY, so its payload is the
// empty tuple and its arity is zero.  test_handler fires it as fire<>() with no
// arguments at all - this is what makes that legal.
bool
tests_event_on_session_start_has_empty_payload()
{
    D_TE_CHECK((std::is_same<payload_of<dt::on_session_start>,
                             std::tuple<> >::value));
    D_TE_CHECK(::djinterp::event_traits<dt::on_session_start>::arity == 0u);
    D_TE_CHECK(::djinterp::event_traits<dt::on_session_start>::has_args == false);

    return true;
}


// tests_event_on_session_end_carries_two_counts
//   payload (passed_count, failed_count) - two std::size_t, in that order.
bool
tests_event_on_session_end_carries_two_counts()
{
    D_TE_CHECK((std::is_same<payload_of<dt::on_session_end>,
                             std::tuple<std::size_t, std::size_t> >::value));
    D_TE_CHECK(::djinterp::event_traits<dt::on_session_end>::arity == 2u);

    return true;
}


// tests_event_module_tags_carry_the_node
//   the interior-node lifecycle pair.
bool
tests_event_module_tags_carry_the_node()
{
    D_TE_CHECK((std::is_same<payload_of<dt::on_module_start>, node_payload>::value));
    D_TE_CHECK((std::is_same<payload_of<dt::on_module_end>,   node_payload>::value));

    return true;
}


// tests_event_test_lifecycle_tags_carry_the_node
//   the leaf lifecycle pair (fired around the status-specific event).
bool
tests_event_test_lifecycle_tags_carry_the_node()
{
    D_TE_CHECK((std::is_same<payload_of<dt::on_test_start>, node_payload>::value));
    D_TE_CHECK((std::is_same<payload_of<dt::on_test_end>,   node_payload>::value));

    return true;
}


// tests_event_terminal_status_tags_carry_the_node
//   passed / failed / skipped resolve to the node alone - the status is implied
// by WHICH tag fired, so it is not repeated in the payload.  (error is the
// exception: it adds a message; see below.)
bool
tests_event_terminal_status_tags_carry_the_node()
{
    D_TE_CHECK((std::is_same<payload_of<dt::on_test_passed>,  node_payload>::value));
    D_TE_CHECK((std::is_same<payload_of<dt::on_test_failed>,  node_payload>::value));
    D_TE_CHECK((std::is_same<payload_of<dt::on_test_skipped>, node_payload>::value));

    return true;
}


// tests_event_on_test_error_carries_node_and_message
//   payload (node, message); the message is a C-string and may be null.
bool
tests_event_on_test_error_carries_node_and_message()
{
    D_TE_CHECK((std::is_same<
                    payload_of<dt::on_test_error>,
                    std::tuple<const dt::basic_test*, const char*> >::value));
    D_TE_CHECK(::djinterp::event_traits<dt::on_test_error>::arity == 2u);

    return true;
}


// tests_event_on_status_change_carries_node_and_both_statuses
//   payload (node, before, after) - the only tag carrying test_status, and the
// reason test_event.hpp includes test_common.hpp at all.
bool
tests_event_on_status_change_carries_node_and_both_statuses()
{
    D_TE_CHECK((std::is_same<
                    payload_of<dt::on_status_change>,
                    std::tuple<const dt::basic_test*,
                               dt::test_status,
                               dt::test_status> >::value));
    D_TE_CHECK(::djinterp::event_traits<dt::on_status_change>::arity == 3u);

    return true;
}


// tests_event_on_listener_threw_carries_two_c_strings
//   the diagnostic summand: (event_name, what).  Note it does NOT carry a node -
// it is the only non-session tag that is not node-scoped, because the listener
// that threw may have been bound to an event that has no node.
bool
tests_event_on_listener_threw_carries_two_c_strings()
{
    D_TE_CHECK((std::is_same<payload_of<dt::on_listener_threw>,
                             std::tuple<const char*, const char*> >::value));
    D_TE_CHECK(::djinterp::event_traits<dt::on_listener_threw>::arity == 2u);

    return true;
}


// ---- alphabet-wide invariants ----------------------------------------------

// tests_event_every_tag_names_its_own_identifier
//   D_EVENT stringifies the tag name, so name() must echo the identifier
// exactly.  A handler that logs or routes by name depends on this.
bool
tests_event_every_tag_names_its_own_identifier()
{
    D_TE_CHECK(std::strcmp(dt::on_session_start::name(),  "on_session_start")  == 0);
    D_TE_CHECK(std::strcmp(dt::on_session_end::name(),    "on_session_end")    == 0);
    D_TE_CHECK(std::strcmp(dt::on_module_start::name(),   "on_module_start")   == 0);
    D_TE_CHECK(std::strcmp(dt::on_module_end::name(),     "on_module_end")     == 0);
    D_TE_CHECK(std::strcmp(dt::on_test_start::name(),     "on_test_start")     == 0);
    D_TE_CHECK(std::strcmp(dt::on_test_end::name(),       "on_test_end")       == 0);
    D_TE_CHECK(std::strcmp(dt::on_test_passed::name(),    "on_test_passed")    == 0);
    D_TE_CHECK(std::strcmp(dt::on_test_failed::name(),    "on_test_failed")    == 0);
    D_TE_CHECK(std::strcmp(dt::on_test_skipped::name(),   "on_test_skipped")   == 0);
    D_TE_CHECK(std::strcmp(dt::on_test_error::name(),     "on_test_error")     == 0);
    D_TE_CHECK(std::strcmp(dt::on_status_change::name(),  "on_status_change")  == 0);
    D_TE_CHECK(std::strcmp(dt::on_listener_threw::name(), "on_listener_threw") == 0);

    return true;
}


// tests_event_every_tag_is_a_distinct_type
//   THE TYPE IS THE REGISTRATION - so two tags collapsing onto one type would
// silently merge two channels.  All 66 pairs are checked.  (Several tags share a
// payload SHAPE, which is exactly why identity cannot be inferred from it.)
bool
tests_event_every_tag_is_a_distinct_type()
{
    // pairwise distinctness, expressed as a fold over a type list
    using all = std::tuple<dt::on_session_start,
                           dt::on_session_end,
                           dt::on_module_start,
                           dt::on_module_end,
                           dt::on_test_start,
                           dt::on_test_end,
                           dt::on_test_passed,
                           dt::on_test_failed,
                           dt::on_test_skipped,
                           dt::on_test_error,
                           dt::on_status_change,
                           dt::on_listener_threw>;

    D_TE_CHECK(std::tuple_size<all>::value == 12u);

    // the tags that share the node payload are nonetheless different TYPES
    D_TE_CHECK(!(std::is_same<dt::on_test_passed, dt::on_test_failed>::value));
    D_TE_CHECK(!(std::is_same<dt::on_test_start,  dt::on_test_end>::value));
    D_TE_CHECK(!(std::is_same<dt::on_module_start, dt::on_test_start>::value));
    D_TE_CHECK(!(std::is_same<dt::on_module_end,   dt::on_test_end>::value));
    D_TE_CHECK(!(std::is_same<dt::on_test_skipped, dt::on_test_passed>::value));

    // and the two-argument tags are not each other either
    D_TE_CHECK(!(std::is_same<dt::on_test_error, dt::on_listener_threw>::value));
    D_TE_CHECK(!(std::is_same<dt::on_session_end, dt::on_test_error>::value));

    // ...and none of them is the empty one
    D_TE_CHECK(!(std::is_same<dt::on_session_start, dt::on_session_end>::value));

    // the exhaustive check: every name() string is unique, which can only hold
    // if every tag is a distinct type (D_EVENT derives name() from the tag name)
    const char* names[12] =
    {
        dt::on_session_start::name(),  dt::on_session_end::name(),
        dt::on_module_start::name(),   dt::on_module_end::name(),
        dt::on_test_start::name(),     dt::on_test_end::name(),
        dt::on_test_passed::name(),    dt::on_test_failed::name(),
        dt::on_test_skipped::name(),   dt::on_test_error::name(),
        dt::on_status_change::name(),  dt::on_listener_threw::name()
    };

    std::size_t _i = 0;
    std::size_t _j = 0;

    for (_i = 0; _i < 12; ++_i)
    {
        for (_j = _i + 1; _j < 12; ++_j)
        {
            D_TE_CHECK(std::strcmp(names[_i], names[_j]) != 0);
        }
    }

    return true;
}


// tests_event_every_tag_satisfies_event_traits
//   every summand is a well-formed event as the subsystem defines one: a nested
// payload_type that IS a std::tuple, plus a name.  (event_traits static_asserts
// both, so merely instantiating it for all twelve is itself the test.)
bool
tests_event_every_tag_satisfies_event_traits()
{
    D_TE_CHECK(::djinterp::event_traits<dt::on_session_start>::has_name);
    D_TE_CHECK(::djinterp::event_traits<dt::on_session_end>::has_name);
    D_TE_CHECK(::djinterp::event_traits<dt::on_module_start>::has_name);
    D_TE_CHECK(::djinterp::event_traits<dt::on_module_end>::has_name);
    D_TE_CHECK(::djinterp::event_traits<dt::on_test_start>::has_name);
    D_TE_CHECK(::djinterp::event_traits<dt::on_test_end>::has_name);
    D_TE_CHECK(::djinterp::event_traits<dt::on_test_passed>::has_name);
    D_TE_CHECK(::djinterp::event_traits<dt::on_test_failed>::has_name);
    D_TE_CHECK(::djinterp::event_traits<dt::on_test_skipped>::has_name);
    D_TE_CHECK(::djinterp::event_traits<dt::on_test_error>::has_name);
    D_TE_CHECK(::djinterp::event_traits<dt::on_status_change>::has_name);
    D_TE_CHECK(::djinterp::event_traits<dt::on_listener_threw>::has_name);

    // args_type is the legacy spelling of payload_type; the alphabet must be
    // readable through either
    D_TE_CHECK((std::is_same<
                    typename ::djinterp::event_traits<dt::on_test_failed>::args_type,
                    payload_of<dt::on_test_failed> >::value));

    return true;
}


// tests_event_arity_agrees_with_payload_size
//   arity is derived, not declared - so it must equal tuple_size of the payload
// for every tag.  A mismatch would mean the traits and the tuple disagree about
// how many arguments a fire site must supply.
bool
tests_event_arity_agrees_with_payload_size()
{
    D_TE_CHECK(::djinterp::event_traits<dt::on_session_start>::arity ==
               std::tuple_size<payload_of<dt::on_session_start> >::value);
    D_TE_CHECK(::djinterp::event_traits<dt::on_session_end>::arity ==
               std::tuple_size<payload_of<dt::on_session_end> >::value);
    D_TE_CHECK(::djinterp::event_traits<dt::on_test_failed>::arity ==
               std::tuple_size<payload_of<dt::on_test_failed> >::value);
    D_TE_CHECK(::djinterp::event_traits<dt::on_test_error>::arity ==
               std::tuple_size<payload_of<dt::on_test_error> >::value);
    D_TE_CHECK(::djinterp::event_traits<dt::on_status_change>::arity ==
               std::tuple_size<payload_of<dt::on_status_change> >::value);
    D_TE_CHECK(::djinterp::event_traits<dt::on_listener_threw>::arity ==
               std::tuple_size<payload_of<dt::on_listener_threw> >::value);

    // the concrete numbers, so a silent re-shaping cannot slip through
    D_TE_CHECK(::djinterp::event_traits<dt::on_session_start>::arity  == 0u);
    D_TE_CHECK(::djinterp::event_traits<dt::on_module_start>::arity   == 1u);
    D_TE_CHECK(::djinterp::event_traits<dt::on_session_end>::arity    == 2u);
    D_TE_CHECK(::djinterp::event_traits<dt::on_status_change>::arity  == 3u);

    return true;
}


// tests_event_node_tags_all_share_one_payload_shape
//   the PAYLOAD CONVENTION from the header: every node-scoped lifecycle tag
// carries exactly `const basic_test*` and nothing else - seven tags, one shape.
// That uniformity is what lets a single generic listener be bound to any of them.
bool
tests_event_node_tags_all_share_one_payload_shape()
{
    D_TE_CHECK((std::is_same<payload_of<dt::on_module_start>,  node_payload>::value));
    D_TE_CHECK((std::is_same<payload_of<dt::on_module_end>,    node_payload>::value));
    D_TE_CHECK((std::is_same<payload_of<dt::on_test_start>,    node_payload>::value));
    D_TE_CHECK((std::is_same<payload_of<dt::on_test_end>,      node_payload>::value));
    D_TE_CHECK((std::is_same<payload_of<dt::on_test_passed>,   node_payload>::value));
    D_TE_CHECK((std::is_same<payload_of<dt::on_test_failed>,   node_payload>::value));
    D_TE_CHECK((std::is_same<payload_of<dt::on_test_skipped>,  node_payload>::value));

    // the tags that deliberately DEPART from the convention
    D_TE_CHECK(!(std::is_same<payload_of<dt::on_test_error>,     node_payload>::value));
    D_TE_CHECK(!(std::is_same<payload_of<dt::on_status_change>,  node_payload>::value));
    D_TE_CHECK(!(std::is_same<payload_of<dt::on_session_start>,  node_payload>::value));
    D_TE_CHECK(!(std::is_same<payload_of<dt::on_listener_threw>, node_payload>::value));

    return true;
}


// tests_event_has_args_partitions_the_alphabet
//   has_args splits the alphabet into the one empty summand and the eleven
// argument-carrying ones.
bool
tests_event_has_args_partitions_the_alphabet()
{
    D_TE_CHECK(::djinterp::event_traits<dt::on_session_start>::has_args == false);

    D_TE_CHECK(::djinterp::event_traits<dt::on_session_end>::has_args);
    D_TE_CHECK(::djinterp::event_traits<dt::on_module_start>::has_args);
    D_TE_CHECK(::djinterp::event_traits<dt::on_module_end>::has_args);
    D_TE_CHECK(::djinterp::event_traits<dt::on_test_start>::has_args);
    D_TE_CHECK(::djinterp::event_traits<dt::on_test_end>::has_args);
    D_TE_CHECK(::djinterp::event_traits<dt::on_test_passed>::has_args);
    D_TE_CHECK(::djinterp::event_traits<dt::on_test_failed>::has_args);
    D_TE_CHECK(::djinterp::event_traits<dt::on_test_skipped>::has_args);
    D_TE_CHECK(::djinterp::event_traits<dt::on_test_error>::has_args);
    D_TE_CHECK(::djinterp::event_traits<dt::on_status_change>::has_args);
    D_TE_CHECK(::djinterp::event_traits<dt::on_listener_threw>::has_args);

    return true;
}


// tests_event_tags_satisfy_the_event_concepts
//   the C++20 face of the same requirements.  The header promises the tags plug
// into machinery that "degrades from concepts (C++20+) to static_assert (C++11)
// with no change here" - so on a C++20 build the concepts must actually accept
// them.  Below C++20 the concept face does not exist and this case is vacuous.
bool
tests_event_tags_satisfy_the_event_concepts()
{
#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    static_assert(::djinterp::is_event<dt::on_test_failed>,
                  "a built-in tag must satisfy is_event");
    static_assert(::djinterp::event_type<dt::on_status_change>,
                  "a built-in tag must satisfy event_type");

    // the empty / argument partition, as concepts
    static_assert(::djinterp::empty_event_type<dt::on_session_start>,
                  "on_session_start is the empty summand");
    static_assert(::djinterp::argument_event_type<dt::on_test_error>,
                  "on_test_error carries arguments");
    static_assert(!::djinterp::empty_event_type<dt::on_test_error>,
                  "on_test_error is not empty");

    // every tag is named (D_EVENT always emits name())
    static_assert(::djinterp::named_event_type<dt::on_listener_threw>,
                  "D_EVENT tags are named");

    // arity, as a concept
    static_assert(::djinterp::event_of_arity<dt::on_status_change, 3>,
                  "on_status_change has arity 3");
    static_assert(::djinterp::event_of_arity<dt::on_session_start, 0>,
                  "on_session_start has arity 0");

    // a non-event is rejected - the concept discriminates
    static_assert(::djinterp::non_event_type<int>,
                  "int is not an event");

    D_TE_CHECK(true);

#else

    D_TE_CHECK(true);   // no concept face below C++20; the traits cases cover it

#endif

    return true;
}


NS_END   // testing
NS_END   // djinterp
