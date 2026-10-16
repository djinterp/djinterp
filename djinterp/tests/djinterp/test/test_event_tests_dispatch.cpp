/*******************************************************************************
* djinterp [test]                                  test_event_tests_dispatch.cpp
*
*   Section IV of the test_event suite: the alphabet's actual PROMISE.
*
*   Sections I-III pin the tags as TYPES.  But a tag exists to be dispatched
* over, and test_event.hpp's whole claim is that "the type IS the registration":
* no enrolment step, and a custom tag "binds and fires through exactly the same
* handler API" as a built-in.  Type-level assertions cannot show that.  These
* tests do: they bind listeners to the tags through the event subsystem the
* header is declared against (event_dispatcher, which test_handler.hpp wraps as
* on() / fire()), fire each one, and assert the DECLARED payload is the payload
* that actually arrives.
*
*   This is also the only place a payload mismatch between a tag and a real fire
* site could be caught, since a listener whose parameters disagree with the tag's
* payload_type simply will not bind.
*
*   The listener's return is part of the contract too: a void-returning callable
* is an always-pass, and one returning verdict::consume halts propagation - so
* both are exercised.
*
*
* path:      /tests/djinterp/test/test_event_tests_dispatch.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <cstddef>
#include <cstring>
#include <string>
// djinterp
#include "test_event_tests.hpp"


// a custom tag, declared exactly as a user would, to prove it dispatches on the
// same terms as the built-ins
namespace myapp
{
    D_TEST_EVENT(on_custom_dispatch);
    D_TEST_EVENT_X(on_custom_retry, int);
}


NS_DJINTERP
NS_TESTING


// tests_event_dispatch_binds_and_fires_a_node_tag
//   the base case: bind to on_test_failed, fire with the node, and the listener
// runs exactly once with the pointer it was given.
bool
tests_event_dispatch_binds_and_fires_a_node_tag()
{
    ::djinterp::event_dispatcher _d;
    const dt::basic_test         _node = make_node();
    const log_ptr                _log  = make_log();

    _d.bind<dt::on_test_failed>(
        [_log](const dt::basic_test* _n)
        {
            ++_log->calls;
            _log->node = _n;
        });

    const ::djinterp::dispatch_result _r = _d.fire<dt::on_test_failed>(&_node);

    D_TE_CHECK(_r.invoked == 1u);
    D_TE_CHECK(_log->calls == 1);
    D_TE_CHECK(_log->node == &_node);      // the very node, not a copy

    return true;
}


// tests_event_dispatch_delivers_a_usable_node
//   a pointer payload is only worth anything if the node behind it is READABLE
// at the listener.  This is the payload convention's whole point: the listener
// reads status / type id / metadata straight off the node.
bool
tests_event_dispatch_delivers_a_usable_node()
{
    ::djinterp::event_dispatcher _d;
    const dt::basic_test         _node = make_node();
    const log_ptr                _log  = make_log();

    _d.bind<dt::on_test_start>(
        [_log](const dt::basic_test* _n)
        {
            ++_log->calls;
            _log->a      = static_cast<std::size_t>(_n->type_id());
            _log->text_1 = _n->metadata().get("name");
        });

    (void) _d.fire<dt::on_test_start>(&_node);

    D_TE_CHECK(_log->calls == 1);
    D_TE_CHECK(_log->a == 7u);                       // the fixture's type id
    D_TE_CHECK(_log->text_1 == k_node_name());       // its metadata, read live

    return true;
}


// tests_event_dispatch_empty_payload_tag_fires_with_no_arguments
//   the empty summand: D_EVENT_EMPTY gives it std::tuple<>, so the fire site
// supplies nothing and the listener takes nothing.  This is exactly how
// test_handler drives it (fire<on_session_start>()).
bool
tests_event_dispatch_empty_payload_tag_fires_with_no_arguments()
{
    ::djinterp::event_dispatcher _d;
    const log_ptr                _log = make_log();

    _d.bind<dt::on_session_start>(
        [_log]()
        {
            ++_log->calls;
        });

    const ::djinterp::dispatch_result _r = _d.fire<dt::on_session_start>();

    D_TE_CHECK(_r.invoked == 1u);
    D_TE_CHECK(_log->calls == 1);

    return true;
}


// tests_event_dispatch_two_count_payload_arrives_intact
//   on_session_end carries (passed, failed) - and IN THAT ORDER.  Two same-typed
// arguments are exactly where a transposition would hide, so distinct values are
// used.
bool
tests_event_dispatch_two_count_payload_arrives_intact()
{
    ::djinterp::event_dispatcher _d;
    const log_ptr                _log = make_log();

    _d.bind<dt::on_session_end>(
        [_log](std::size_t _passed, std::size_t _failed)
        {
            ++_log->calls;
            _log->a = _passed;
            _log->b = _failed;
        });

    (void) _d.fire<dt::on_session_end>(std::size_t(41), std::size_t(2));

    D_TE_CHECK(_log->calls == 1);
    D_TE_CHECK(_log->a == 41u);      // passed
    D_TE_CHECK(_log->b == 2u);       // failed - not transposed

    return true;
}


// tests_event_dispatch_status_change_payload_arrives_intact
//   the three-element payload (node, before, after).  Again the two same-typed
// statuses must not swap.
bool
tests_event_dispatch_status_change_payload_arrives_intact()
{
    ::djinterp::event_dispatcher _d;
    const dt::basic_test         _node = make_node();
    const log_ptr                _log  = make_log();

    _d.bind<dt::on_status_change>(
        [_log](const dt::basic_test* _n,
               dt::test_status       _before,
               dt::test_status       _after)
        {
            ++_log->calls;
            _log->node   = _n;
            _log->before = _before;
            _log->after  = _after;
        });

    (void) _d.fire<dt::on_status_change>(&_node,
                                         dt::test_status::pending,
                                         dt::test_status::passed);

    D_TE_CHECK(_log->calls == 1);
    D_TE_CHECK(_log->node == &_node);
    D_TE_CHECK(_log->before == dt::test_status::pending);
    D_TE_CHECK(_log->after  == dt::test_status::passed);

    return true;
}


// tests_event_dispatch_error_payload_carries_the_message
//   on_test_error is (node, message).  The header notes the message "may be
// null", so both a real string and nullptr are fired.
bool
tests_event_dispatch_error_payload_carries_the_message()
{
    ::djinterp::event_dispatcher _d;
    const dt::basic_test         _node = make_node();
    const log_ptr                _log  = make_log();

    _d.bind<dt::on_test_error>(
        [_log](const dt::basic_test* _n, const char* _msg)
        {
            ++_log->calls;
            _log->node   = _n;
            _log->text_1 = (_msg != nullptr) ? std::string(_msg)
                                             : std::string("<null>");
        });

    (void) _d.fire<dt::on_test_error>(&_node, "segfault in leaf");

    D_TE_CHECK(_log->calls == 1);
    D_TE_CHECK(_log->node == &_node);
    D_TE_CHECK(_log->text_1 == "segfault in leaf");

    // the documented null-message case must dispatch just as happily
    (void) _d.fire<dt::on_test_error>(&_node, static_cast<const char*>(nullptr));

    D_TE_CHECK(_log->calls == 2);
    D_TE_CHECK(_log->text_1 == "<null>");

    return true;
}


// tests_event_dispatch_listener_threw_payload_arrives_intact
//   the diagnostic summand: (event_name, what), two C-strings and no node.
bool
tests_event_dispatch_listener_threw_payload_arrives_intact()
{
    ::djinterp::event_dispatcher _d;
    const log_ptr                _log = make_log();

    _d.bind<dt::on_listener_threw>(
        [_log](const char* _event, const char* _what)
        {
            ++_log->calls;
            _log->text_1 = _event;
            _log->text_2 = _what;
        });

    (void) _d.fire<dt::on_listener_threw>(dt::on_test_failed::name(),
                                          "std::bad_alloc");

    D_TE_CHECK(_log->calls == 1);
    D_TE_CHECK(_log->text_1 == "on_test_failed");   // a tag's own name(), routed
    D_TE_CHECK(_log->text_2 == "std::bad_alloc");

    return true;
}


// tests_event_dispatch_custom_tag_binds_exactly_like_a_builtin
//   THE open-alphabet claim: a D_TEST_EVENT tag declared in a user namespace is
// bound and fired through the same API, with no enrolment step of any kind - and
// a D_TEST_EVENT_X tag carries its extra domain through as well.
bool
tests_event_dispatch_custom_tag_binds_exactly_like_a_builtin()
{
    ::djinterp::event_dispatcher _d;
    const dt::basic_test         _node = make_node();
    const log_ptr                _log  = make_log();

    _d.bind<myapp::on_custom_dispatch>(
        [_log](const dt::basic_test* _n)
        {
            ++_log->calls;
            _log->node = _n;
        });

    _d.bind<myapp::on_custom_retry>(
        [_log](const dt::basic_test* _n, int _attempt)
        {
            ++_log->calls;
            _log->node = _n;
            _log->a    = static_cast<std::size_t>(_attempt);
        });

    const ::djinterp::dispatch_result _r1 =
        _d.fire<myapp::on_custom_dispatch>(&_node);

    D_TE_CHECK(_r1.invoked == 1u);
    D_TE_CHECK(_log->node == &_node);

    const ::djinterp::dispatch_result _r2 =
        _d.fire<myapp::on_custom_retry>(&_node, 3);

    D_TE_CHECK(_r2.invoked == 1u);
    D_TE_CHECK(_log->calls == 2);
    D_TE_CHECK(_log->a == 3u);

    return true;
}


// tests_event_dispatch_void_listener_is_an_always_pass
//   a void-returning callable is accepted and treated as an always-pass, so the
// dispatch outcome is `pass` and propagation is not consumed.
bool
tests_event_dispatch_void_listener_is_an_always_pass()
{
    ::djinterp::event_dispatcher _d;
    const dt::basic_test         _node = make_node();
    const log_ptr                _log  = make_log();

    _d.bind<dt::on_test_passed>(
        [_log](const dt::basic_test*)
        {
            ++_log->calls;               // returns void
        });

    const ::djinterp::dispatch_result _r = _d.fire<dt::on_test_passed>(&_node);

    D_TE_CHECK(_r.invoked == 1u);
    D_TE_CHECK(_r.outcome == ::djinterp::verdict::pass);
    D_TE_CHECK(!_r.consumed());
    D_TE_CHECK(!::djinterp::consumed(_r.outcome));

    return true;
}


// tests_event_dispatch_consume_verdict_halts_propagation
//   a listener returning verdict::consume is the left zero of handler
// sequencing: the dispatch reports `consume` and the listener bound AFTER it does
// not run.  The tags carry no opinion about this - but they must not obstruct it.
bool
tests_event_dispatch_consume_verdict_halts_propagation()
{
    ::djinterp::event_dispatcher _d;
    const dt::basic_test         _node   = make_node();
    const log_ptr                _first  = make_log();
    const log_ptr                _second = make_log();

    _d.bind<dt::on_test_failed>(
        [_first](const dt::basic_test*) -> ::djinterp::verdict
        {
            ++_first->calls;
            return ::djinterp::verdict::consume;
        });

    _d.bind<dt::on_test_failed>(
        [_second](const dt::basic_test*)
        {
            ++_second->calls;            // must NOT run
        });

    const ::djinterp::dispatch_result _r = _d.fire<dt::on_test_failed>(&_node);

    D_TE_CHECK(_first->calls == 1);
    D_TE_CHECK(_second->calls == 0);     // propagation halted
    D_TE_CHECK(_r.consumed());
    D_TE_CHECK(_r.outcome == ::djinterp::verdict::consume);
    D_TE_CHECK(_r.invoked == 1u);

    return true;
}


// tests_event_dispatch_every_bound_listener_is_invoked
//   several listeners on one tag all run (absent a consume), and the dispatch
// result counts them.
bool
tests_event_dispatch_every_bound_listener_is_invoked()
{
    ::djinterp::event_dispatcher _d;
    const dt::basic_test         _node = make_node();
    const log_ptr                _log  = make_log();

    _d.bind<dt::on_test_skipped>([_log](const dt::basic_test*) { ++_log->calls; });
    _d.bind<dt::on_test_skipped>([_log](const dt::basic_test*) { ++_log->calls; });
    _d.bind<dt::on_test_skipped>([_log](const dt::basic_test*) { ++_log->calls; });

    D_TE_CHECK(_d.has_handlers_for<dt::on_test_skipped>());
    D_TE_CHECK(_d.handler_count_for<dt::on_test_skipped>() == 3u);

    const ::djinterp::dispatch_result _r = _d.fire<dt::on_test_skipped>(&_node);

    D_TE_CHECK(_r.invoked == 3u);
    D_TE_CHECK(_log->calls == 3);

    return true;
}


// tests_event_dispatch_tags_are_independent_channels
//   THE TYPE IS THE REGISTRATION - so binding to one tag must not catch another,
// even when the two share an identical payload shape.  This is the assertion that
// would fail if two tags ever collapsed onto one type.
bool
tests_event_dispatch_tags_are_independent_channels()
{
    ::djinterp::event_dispatcher _d;
    const dt::basic_test         _node   = make_node();
    const log_ptr                _failed = make_log();
    const log_ptr                _passed = make_log();
    const log_ptr                _custom = make_log();

    _d.bind<dt::on_test_failed>([_failed](const dt::basic_test*) { ++_failed->calls; });
    _d.bind<dt::on_test_passed>([_passed](const dt::basic_test*) { ++_passed->calls; });
    _d.bind<myapp::on_custom_dispatch>([_custom](const dt::basic_test*) { ++_custom->calls; });

    // all three share the SAME payload type...
    D_TE_CHECK((std::is_same<payload_of<dt::on_test_failed>,
                             payload_of<myapp::on_custom_dispatch> >::value));

    // ...but firing one reaches only its own listener
    (void) _d.fire<dt::on_test_failed>(&_node);

    D_TE_CHECK(_failed->calls == 1);
    D_TE_CHECK(_passed->calls == 0);
    D_TE_CHECK(_custom->calls == 0);

    (void) _d.fire<myapp::on_custom_dispatch>(&_node);

    D_TE_CHECK(_failed->calls == 1);
    D_TE_CHECK(_passed->calls == 0);
    D_TE_CHECK(_custom->calls == 1);

    return true;
}


// tests_event_dispatch_unbound_tag_invokes_nothing
//   firing a tag nobody listens for is a no-op, not an error.  The alphabet is
// open: a tag may exist with no listeners at all.
bool
tests_event_dispatch_unbound_tag_invokes_nothing()
{
    ::djinterp::event_dispatcher _d;
    const dt::basic_test         _node = make_node();

    D_TE_CHECK(!_d.has_handlers_for<dt::on_module_start>());
    D_TE_CHECK(_d.handler_count_for<dt::on_module_start>() == 0u);

    const ::djinterp::dispatch_result _r = _d.fire<dt::on_module_start>(&_node);

    D_TE_CHECK(_r.invoked == 0u);
    D_TE_CHECK(!_r.consumed());

    return true;
}


// tests_event_dispatch_unbind_removes_the_listener
//   bind returns a handler_id, and unbinding by that id takes the listener off
// the tag's channel - the registration is retractable even though it is a type.
bool
tests_event_dispatch_unbind_removes_the_listener()
{
    ::djinterp::event_dispatcher _d;
    const dt::basic_test         _node = make_node();
    const log_ptr                _log  = make_log();

    const ::djinterp::handler_id _id = _d.bind<dt::on_module_end>(
        [_log](const dt::basic_test*) { ++_log->calls; });

    D_TE_CHECK(_d.contains(_id));
    D_TE_CHECK(_d.handler_count_for<dt::on_module_end>() == 1u);

    (void) _d.fire<dt::on_module_end>(&_node);
    D_TE_CHECK(_log->calls == 1);

    D_TE_CHECK(_d.unbind(_id));
    D_TE_CHECK(!_d.contains(_id));
    D_TE_CHECK(_d.handler_count_for<dt::on_module_end>() == 0u);

    const ::djinterp::dispatch_result _r = _d.fire<dt::on_module_end>(&_node);

    D_TE_CHECK(_r.invoked == 0u);
    D_TE_CHECK(_log->calls == 1);       // unchanged: the listener is gone

    return true;
}


NS_END   // testing
NS_END   // djinterp
