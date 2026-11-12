/*******************************************************************************
* djinterp [cpp][test]                               test_event_tests_macros.cpp
*
*   Section III of the DTestEvent suite: the D_TEST_EVENT family.
*
*   These macros are the header's open-alphabet promise: a user declares a
* custom test event in one line and it binds and fires through exactly the same
* API as the built-ins, because the type is the registration.  Three claims are
* made about them, and each is pinned here:
*
*     1. Shape.  D_TEST_EVENT carries the node ("matching the built-in lifecycle
*        shape"); D_TEST_EVENT_MSG carries node + message ("matching the
*        DOnTestError shape"); D_TEST_EVENT_X carries the node followed by
*        caller-specified extra domains.  Those are not merely "similar" shapes -
*        the tests assert they are the same types as the built-ins' payloads.
*     2. Namespace-agnosticism.  The macros refer to the node by its fully
*        qualified name, so a custom tag may be declared in the user's own
*        namespace, at global scope, or alongside the built-ins in
*        djinterp::test.  All three are exercised below - a tag declared in each.
*     3. Identity.  A custom tag is a new summand: distinct from every built-in
*        and from every other custom tag, even when the payload shape is
*        identical.
*
*   The macros expand to a struct with no trailing semicolon (the caller
* supplies it), which is why each declaration below ends in `;`.
*
*
* path:      /tests/djinterp/cpp/modules/test/test_event_tests_macros.cpp
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
    D_TEST_EVENT(DOnFlakyRetry);              // node
    D_TEST_EVENT_MSG(DOnSoftWarning);         // node + message
    D_TEST_EVENT_X(DOnRetry, int);             // node + one extra domain
    D_TEST_EVENT_X(DOnBudget, int, double);    // node + two extra domains
}

// (b) the global namespace
D_TEST_EVENT(DOnGlobalProbe);

// (c) alongside the built-ins, in djinterp::test itself
NS_DJINTERP
NS_TEST
    D_TEST_EVENT(DOnAlongsideBuiltin);
NS_END
NS_END


NS_DJINTERP
NS_TESTING


// tests_event_d_test_event_declares_a_node_tag
//   the plain form: payload is exactly (const basic_test*).
bool tests_event_d_test_event_declares_a_node_tag()
{
    D_TE_CHECK((std::is_same<payload_of<myproj::DOnFlakyRetry>,
                             std::tuple<const dt::basic_test*> >::value));
    D_TE_CHECK(::djinterp::event_traits<myproj::DOnFlakyRetry>::arity == 1u);

    return true;
}


// tests_event_d_test_event_matches_the_builtin_lifecycle_shape
//   the header says D_TEST_EVENT matches "the built-in lifecycle shape".  Not
// approximately - the payload type is the SAME type as the node tags'.  So a
// listener written against DOnTestFailed can be bound to a custom tag unchanged.
bool tests_event_d_test_event_matches_the_builtin_lifecycle_shape()
{
    D_TE_CHECK((std::is_same<payload_of<myproj::DOnFlakyRetry>,
                             payload_of<dt::DOnTestFailed> >::value));
    D_TE_CHECK((std::is_same<payload_of<myproj::DOnFlakyRetry>,
                             DNodePayload>::value));

    return true;
}


// tests_event_d_test_event_msg_declares_a_node_plus_message_tag
bool tests_event_d_test_event_msg_declares_a_node_plus_message_tag()
{
    D_TE_CHECK((std::is_same<
                    payload_of<myproj::DOnSoftWarning>,
                    std::tuple<const dt::basic_test*, const char*> >::value));
    D_TE_CHECK(::djinterp::event_traits<myproj::DOnSoftWarning>::arity == 2u);

    return true;
}


// tests_event_d_test_event_msg_matches_the_on_test_error_shape
//   the second documented equivalence, likewise asserted as type identity.
bool tests_event_d_test_event_msg_matches_the_on_test_error_shape()
{
    D_TE_CHECK((std::is_same<payload_of<myproj::DOnSoftWarning>,
                             payload_of<dt::DOnTestError> >::value));

    return true;
}


// tests_event_d_test_event_x_appends_one_extra_domain
//   the node comes FIRST, the caller's domains after - order matters, since the
// fire site supplies them positionally.
bool tests_event_d_test_event_x_appends_one_extra_domain()
{
    D_TE_CHECK((std::is_same<
                    payload_of<myproj::DOnRetry>,
                    std::tuple<const dt::basic_test*, int> >::value));
    D_TE_CHECK(::djinterp::event_traits<myproj::DOnRetry>::arity == 2u);

    // the node is element 0, the extra domain element 1
    using p0 = typename std::tuple_element<0, payload_of<myproj::DOnRetry> >::type;
    using p1 = typename std::tuple_element<1, payload_of<myproj::DOnRetry> >::type;

    D_TE_CHECK((std::is_same<p0, const dt::basic_test*>::value));
    D_TE_CHECK((std::is_same<p1, int>::value));

    return true;
}


// tests_event_d_test_event_x_appends_several_extra_domains
//   the variadic tail really is variadic.
bool tests_event_d_test_event_x_appends_several_extra_domains()
{
    D_TE_CHECK((std::is_same<
                    payload_of<myproj::DOnBudget>,
                    std::tuple<const dt::basic_test*, int, double> >::value));
    D_TE_CHECK(::djinterp::event_traits<myproj::DOnBudget>::arity == 3u);

    return true;
}


// tests_event_custom_tags_name_their_own_identifiers
//   D_TEST_EVENT forwards to D_EVENT, which stringifies the tag name - so the
// name survives the extra macro hop rather than becoming "_name" or the
// expansion text.
bool tests_event_custom_tags_name_their_own_identifiers()
{
    D_TE_CHECK(std::strcmp(myproj::DOnFlakyRetry::name(),  "DOnFlakyRetry")  == 0);
    D_TE_CHECK(std::strcmp(myproj::DOnSoftWarning::name(), "DOnSoftWarning") == 0);
    D_TE_CHECK(std::strcmp(myproj::DOnRetry::name(),       "DOnRetry")       == 0);
    D_TE_CHECK(std::strcmp(myproj::DOnBudget::name(),      "DOnBudget")      == 0);
    D_TE_CHECK(std::strcmp(::DOnGlobalProbe::name(),       "DOnGlobalProbe") == 0);
    D_TE_CHECK(std::strcmp(dt::DOnAlongsideBuiltin::name(),
                           "DOnAlongsideBuiltin") == 0);

    // the name is the IDENTIFIER, not the qualified name - the same identifier
    // declared in two scopes yields two types with the same name() string
    D_TE_CHECK(std::strcmp(myproj::DOnFlakyRetry::name(),
                           "myproj::DOnFlakyRetry") != 0);

    return true;
}


// tests_event_custom_tags_are_distinct_from_builtins
//   a custom tag with the built-in payload shape is still its OWN channel:
// binding to it must not catch DOnTestFailed, and vice versa.  That is only
// true because they are different types.
bool tests_event_custom_tags_are_distinct_from_builtins()
{
    D_TE_CHECK(!(std::is_same<myproj::DOnFlakyRetry, dt::DOnTestFailed>::value));
    D_TE_CHECK(!(std::is_same<myproj::DOnFlakyRetry, dt::DOnTestPassed>::value));
    D_TE_CHECK(!(std::is_same<myproj::DOnSoftWarning, dt::DOnTestError>::value));
    D_TE_CHECK(!(std::is_same<dt::DOnAlongsideBuiltin, dt::DOnTestStart>::value));

    // ...even though the payloads are identical types
    D_TE_CHECK((std::is_same<payload_of<myproj::DOnFlakyRetry>,
                             payload_of<dt::DOnTestFailed> >::value));

    return true;
}


// tests_event_custom_tags_are_distinct_from_each_other
bool tests_event_custom_tags_are_distinct_from_each_other()
{
    D_TE_CHECK(!(std::is_same<myproj::DOnFlakyRetry, myproj::DOnSoftWarning>::value));
    D_TE_CHECK(!(std::is_same<myproj::DOnRetry,       myproj::DOnBudget>::value));
    D_TE_CHECK(!(std::is_same<myproj::DOnFlakyRetry, ::DOnGlobalProbe>::value));
    D_TE_CHECK(!(std::is_same<::DOnGlobalProbe, dt::DOnAlongsideBuiltin>::value));

    // same macro, same payload, three scopes -> three distinct types
    D_TE_CHECK((std::is_same<payload_of<myproj::DOnFlakyRetry>,
                             payload_of< ::DOnGlobalProbe> >::value));
    D_TE_CHECK((std::is_same<payload_of< ::DOnGlobalProbe>,
                             payload_of<dt::DOnAlongsideBuiltin> >::value));

    return true;
}


// tests_event_macros_are_namespace_agnostic
//   the claim made structurally: tags declared in a user namespace, at global
// scope, and inside djinterp::test all resolve their node type correctly and are
// all well-formed events.  (If the macros did not fully qualify basic_test, the
// global-scope and myproj declarations at the top of this file would not have
// compiled at all - so reaching this line is already most of the proof.)
bool tests_event_macros_are_namespace_agnostic()
{
    D_TE_CHECK((std::is_same<payload_of<myproj::DOnFlakyRetry>,      DNodePayload>::value));
    D_TE_CHECK((std::is_same<payload_of< ::DOnGlobalProbe>,          DNodePayload>::value));
    D_TE_CHECK((std::is_same<payload_of<dt::DOnAlongsideBuiltin>,    DNodePayload>::value));

    return true;
}


// tests_event_custom_tags_satisfy_event_traits
//   a macro-declared tag is a first-class summand: the subsystem accepts it on
// exactly the same terms as a built-in.
bool tests_event_custom_tags_satisfy_event_traits()
{
    D_TE_CHECK(::djinterp::event_traits<myproj::DOnFlakyRetry>::has_name);
    D_TE_CHECK(::djinterp::event_traits<myproj::DOnFlakyRetry>::has_args);
    D_TE_CHECK(::djinterp::event_traits<myproj::DOnBudget>::arity == 3u);
    D_TE_CHECK(::djinterp::event_traits< ::DOnGlobalProbe>::has_name);

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    static_assert(::djinterp::is_event<myproj::DOnFlakyRetry>,
                  "a D_TEST_EVENT tag must satisfy is_event");
    static_assert(::djinterp::argument_event_type<myproj::DOnBudget>,
                  "a D_TEST_EVENT_X tag carries arguments");
    static_assert(::djinterp::event_of_arity<myproj::DOnBudget, 3>,
                  "on_budget has arity 3");

#endif

    return true;
}


NS_END   // testing
NS_END   // djinterp
