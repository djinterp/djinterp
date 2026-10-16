/*******************************************************************************
* djinterp [test]                                    test_event_tests_macros.cpp
*
*   Section III of the test_event suite: the D_TEST_EVENT family.
*
*   These macros are the header's OPEN-ALPHABET promise: a user declares a
* custom test event in one line and it binds and fires through exactly the same
* API as the built-ins, because the type IS the registration.  Three claims are
* made about them, and each is pinned here:
*
*     1. SHAPE.  D_TEST_EVENT carries the node ("matching the built-in lifecycle
*        shape"); D_TEST_EVENT_MSG carries node + message ("matching the
*        on_test_error shape"); D_TEST_EVENT_X carries the node followed by
*        caller-specified extra domains.  Those are not merely "similar" shapes -
*        the tests assert they are the SAME TYPES as the built-ins' payloads.
*     2. NAMESPACE-AGNOSTICISM.  The macros refer to the node by its fully
*        qualified name, so a custom tag may be declared in the user's own
*        namespace, at global scope, or alongside the built-ins in
*        djinterp::test.  All three are exercised below - a tag declared in each.
*     3. IDENTITY.  A custom tag is a NEW summand: distinct from every built-in
*        and from every other custom tag, even when the payload shape is
*        identical.
*
*   The macros expand to a struct with no trailing semicolon (the caller
* supplies it), which is why each declaration below ends in `;`.
*
*
* path:      /tests/djinterp/test/test_event_tests_macros.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <cstring>
#include <tuple>
#include <type_traits>
// djinterp
#include "test_event_tests.hpp"


// ---------------------------------------------------------------------------
//   CUSTOM TAGS, DECLARED IN THREE DIFFERENT SCOPES
//
//   This is the namespace-agnosticism claim, made structurally: if the macros
// did not fully qualify the node type, at most one of these three would compile.
// ---------------------------------------------------------------------------

// (a) a user's own namespace, outside djinterp entirely - the documented case
namespace myproj
{
    D_TEST_EVENT(on_flaky_retry);              // node
    D_TEST_EVENT_MSG(on_soft_warning);         // node + message
    D_TEST_EVENT_X(on_retry, int);             // node + one extra domain
    D_TEST_EVENT_X(on_budget, int, double);    // node + two extra domains
}

// (b) the global namespace
D_TEST_EVENT(on_global_probe);

// (c) alongside the built-ins, in djinterp::test itself
NS_DJINTERP
NS_TEST
    D_TEST_EVENT(on_alongside_builtin);
NS_END
NS_END


NS_DJINTERP
NS_TESTING


// tests_event_d_test_event_declares_a_node_tag
//   the plain form: payload is exactly (const basic_test*).
bool
tests_event_d_test_event_declares_a_node_tag()
{
    D_TE_CHECK((std::is_same<payload_of<myproj::on_flaky_retry>,
                             std::tuple<const dt::basic_test*> >::value));
    D_TE_CHECK(::djinterp::event_traits<myproj::on_flaky_retry>::arity == 1u);

    return true;
}


// tests_event_d_test_event_matches_the_builtin_lifecycle_shape
//   the header says D_TEST_EVENT matches "the built-in lifecycle shape".  Not
// approximately - the payload type is the SAME type as the node tags'.  So a
// listener written against on_test_failed can be bound to a custom tag unchanged.
bool
tests_event_d_test_event_matches_the_builtin_lifecycle_shape()
{
    D_TE_CHECK((std::is_same<payload_of<myproj::on_flaky_retry>,
                             payload_of<dt::on_test_failed> >::value));
    D_TE_CHECK((std::is_same<payload_of<myproj::on_flaky_retry>,
                             node_payload>::value));

    return true;
}


// tests_event_d_test_event_msg_declares_a_node_plus_message_tag
bool
tests_event_d_test_event_msg_declares_a_node_plus_message_tag()
{
    D_TE_CHECK((std::is_same<
                    payload_of<myproj::on_soft_warning>,
                    std::tuple<const dt::basic_test*, const char*> >::value));
    D_TE_CHECK(::djinterp::event_traits<myproj::on_soft_warning>::arity == 2u);

    return true;
}


// tests_event_d_test_event_msg_matches_the_on_test_error_shape
//   the second documented equivalence, likewise asserted as type identity.
bool
tests_event_d_test_event_msg_matches_the_on_test_error_shape()
{
    D_TE_CHECK((std::is_same<payload_of<myproj::on_soft_warning>,
                             payload_of<dt::on_test_error> >::value));

    return true;
}


// tests_event_d_test_event_x_appends_one_extra_domain
//   the node comes FIRST, the caller's domains after - order matters, since the
// fire site supplies them positionally.
bool
tests_event_d_test_event_x_appends_one_extra_domain()
{
    D_TE_CHECK((std::is_same<
                    payload_of<myproj::on_retry>,
                    std::tuple<const dt::basic_test*, int> >::value));
    D_TE_CHECK(::djinterp::event_traits<myproj::on_retry>::arity == 2u);

    // the node is element 0, the extra domain element 1
    using p0 = typename std::tuple_element<0, payload_of<myproj::on_retry> >::type;
    using p1 = typename std::tuple_element<1, payload_of<myproj::on_retry> >::type;

    D_TE_CHECK((std::is_same<p0, const dt::basic_test*>::value));
    D_TE_CHECK((std::is_same<p1, int>::value));

    return true;
}


// tests_event_d_test_event_x_appends_several_extra_domains
//   the variadic tail really is variadic.
bool
tests_event_d_test_event_x_appends_several_extra_domains()
{
    D_TE_CHECK((std::is_same<
                    payload_of<myproj::on_budget>,
                    std::tuple<const dt::basic_test*, int, double> >::value));
    D_TE_CHECK(::djinterp::event_traits<myproj::on_budget>::arity == 3u);

    return true;
}


// tests_event_custom_tags_name_their_own_identifiers
//   D_TEST_EVENT forwards to D_EVENT, which stringifies the tag name - so the
// name survives the extra macro hop rather than becoming "_name" or the
// expansion text.
bool
tests_event_custom_tags_name_their_own_identifiers()
{
    D_TE_CHECK(std::strcmp(myproj::on_flaky_retry::name(),  "on_flaky_retry")  == 0);
    D_TE_CHECK(std::strcmp(myproj::on_soft_warning::name(), "on_soft_warning") == 0);
    D_TE_CHECK(std::strcmp(myproj::on_retry::name(),        "on_retry")        == 0);
    D_TE_CHECK(std::strcmp(myproj::on_budget::name(),       "on_budget")       == 0);
    D_TE_CHECK(std::strcmp(::on_global_probe::name(),       "on_global_probe") == 0);
    D_TE_CHECK(std::strcmp(dt::on_alongside_builtin::name(),
                           "on_alongside_builtin") == 0);

    // the name is the IDENTIFIER, not the qualified name - the same identifier
    // declared in two scopes yields two types with the same name() string
    D_TE_CHECK(std::strcmp(myproj::on_flaky_retry::name(),
                           "myproj::on_flaky_retry") != 0);

    return true;
}


// tests_event_custom_tags_are_distinct_from_builtins
//   a custom tag with the built-in payload shape is still its OWN channel:
// binding to it must not catch on_test_failed, and vice versa.  That is only
// true because they are different types.
bool
tests_event_custom_tags_are_distinct_from_builtins()
{
    D_TE_CHECK(!(std::is_same<myproj::on_flaky_retry, dt::on_test_failed>::value));
    D_TE_CHECK(!(std::is_same<myproj::on_flaky_retry, dt::on_test_passed>::value));
    D_TE_CHECK(!(std::is_same<myproj::on_soft_warning, dt::on_test_error>::value));
    D_TE_CHECK(!(std::is_same<dt::on_alongside_builtin, dt::on_test_start>::value));

    // ...even though the payloads are identical types
    D_TE_CHECK((std::is_same<payload_of<myproj::on_flaky_retry>,
                             payload_of<dt::on_test_failed> >::value));

    return true;
}


// tests_event_custom_tags_are_distinct_from_each_other
bool
tests_event_custom_tags_are_distinct_from_each_other()
{
    D_TE_CHECK(!(std::is_same<myproj::on_flaky_retry, myproj::on_soft_warning>::value));
    D_TE_CHECK(!(std::is_same<myproj::on_retry,       myproj::on_budget>::value));
    D_TE_CHECK(!(std::is_same<myproj::on_flaky_retry, ::on_global_probe>::value));
    D_TE_CHECK(!(std::is_same<::on_global_probe, dt::on_alongside_builtin>::value));

    // same macro, same payload, three scopes -> three distinct types
    D_TE_CHECK((std::is_same<payload_of<myproj::on_flaky_retry>,
                             payload_of< ::on_global_probe> >::value));
    D_TE_CHECK((std::is_same<payload_of< ::on_global_probe>,
                             payload_of<dt::on_alongside_builtin> >::value));

    return true;
}


// tests_event_macros_are_namespace_agnostic
//   the claim made structurally: tags declared in a user namespace, at global
// scope, and inside djinterp::test all resolve their node type correctly and are
// all well-formed events.  (If the macros did not fully qualify basic_test, the
// global-scope and myproj declarations at the top of this file would not have
// compiled at all - so reaching this line is already most of the proof.)
bool
tests_event_macros_are_namespace_agnostic()
{
    D_TE_CHECK((std::is_same<payload_of<myproj::on_flaky_retry>,      node_payload>::value));
    D_TE_CHECK((std::is_same<payload_of< ::on_global_probe>,          node_payload>::value));
    D_TE_CHECK((std::is_same<payload_of<dt::on_alongside_builtin>,    node_payload>::value));

    return true;
}


// tests_event_custom_tags_satisfy_event_traits
//   a macro-declared tag is a first-class summand: the subsystem accepts it on
// exactly the same terms as a built-in.
bool
tests_event_custom_tags_satisfy_event_traits()
{
    D_TE_CHECK(::djinterp::event_traits<myproj::on_flaky_retry>::has_name);
    D_TE_CHECK(::djinterp::event_traits<myproj::on_flaky_retry>::has_args);
    D_TE_CHECK(::djinterp::event_traits<myproj::on_budget>::arity == 3u);
    D_TE_CHECK(::djinterp::event_traits< ::on_global_probe>::has_name);

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    static_assert(::djinterp::is_event<myproj::on_flaky_retry>,
                  "a D_TEST_EVENT tag must satisfy is_event");
    static_assert(::djinterp::argument_event_type<myproj::on_budget>,
                  "a D_TEST_EVENT_X tag carries arguments");
    static_assert(::djinterp::event_of_arity<myproj::on_budget, 3>,
                  "on_budget has arity 3");

#endif

    return true;
}


NS_END   // testing
NS_END   // djinterp
