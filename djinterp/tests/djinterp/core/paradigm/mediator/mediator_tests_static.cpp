// djinterp [test] : mediator_tests_static.cpp
//   The CRTP static mediator (section VII): a compile-time colleague set with
// no virtual dispatch.  The fixture is a two-colleague UI: a button and a
// label, both static_colleagues of a ui_mediator whose route() simply
// broadcasts to everyone but the sender.  The static mediator's broadcast_impl
// calls a single-argument on_event(const _Event&) on every colleague, so both
// colleague types provide on_event(const ping&).

// std
#include <cstddef>
#include <vector>
// djinterp
#include "mediator_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    class ui_mediator;      // forward: colleagues are CRTP-parameterised on it

    // button : a static colleague that records the pings it is handed and can
    // originate a ping through the mediator via the protected send().
    struct button : static_colleague<button, ui_mediator>
    {
        std::vector<int> got;

        void on_event(const ping& _e) { got.push_back(_e.n); }

        void press(int _n) { this->send(ping{ _n }); }
    };

    // label : a second, structurally-identical static colleague.
    struct label : static_colleague<label, ui_mediator>
    {
        std::vector<int> got;

        void on_event(const ping& _e) { got.push_back(_e.n); }

        void nudge(int _n) { this->send(ping{ _n }); }
    };

    // ui_mediator : the concrete mediator over the fixed {button, label} set.
    // route() is the required hook that static_colleague::send() calls; here it
    // fans the event out to every colleague except the originator.
    class ui_mediator : public static_mediator<ui_mediator, button, label>
    {
    public:
        template<typename _Sender,
                 typename _Event>
        void
        route(
            _Sender&      _sender,
            const _Event& _event
        )
        {
            this->broadcast_except(_sender, _event);

            return;
        }
    };
}


/*
tests_static_colleague_defaults
  Verifies a default-constructed static colleague has no bound mediator.
  Tests the following:
  - mediator() is null before registration
*/
bool
tests_static_colleague_defaults()
{
    button b;

    return (b.mediator() == nullptr);
}

/*
tests_static_mediator_register
  Verifies register_colleague binds each colleague to the mediator.
  Tests the following:
  - after registration each colleague's mediator() points at the mediator
*/
bool
tests_static_mediator_register()
{
    ui_mediator med;
    button      b;
    label       l;

    med.register_colleague<0>(&b);
    med.register_colleague<1>(&l);

    bool ok = true;

    ok = ok && (b.mediator() == &med);
    ok = ok && (l.mediator() == &med);

    return ok;
}

/*
tests_static_mediator_get_colleague
  Verifies get_colleague returns the registered pointer for each index.
  Tests the following:
  - get_colleague<0> / get_colleague<1> return the registered addresses
*/
bool
tests_static_mediator_get_colleague()
{
    ui_mediator med;
    button      b;
    label       l;

    med.register_colleague<0>(&b);
    med.register_colleague<1>(&l);

    bool ok = true;

    ok = ok && (med.get_colleague<0>() == &b);
    ok = ok && (med.get_colleague<1>() == &l);

    return ok;
}

/*
tests_static_mediator_broadcast
  Verifies a send from one colleague reaches the others but not the sender.
  Tests the following:
  - button.press routes a ping to the label
  - the originating button does not receive its own ping
*/
bool
tests_static_mediator_broadcast()
{
    ui_mediator med;
    button      b;
    label       l;

    med.register_colleague<0>(&b);
    med.register_colleague<1>(&l);

    b.press(5);

    bool ok = true;

    ok = ok && (b.got.empty());
    ok = ok && (l.got.size() == 1 && l.got[0] == 5);

    return ok;
}

/*
tests_static_mediator_broadcast_reverse
  Verifies the routing is symmetric: the other colleague may originate too.
  Tests the following:
  - label.nudge routes a ping to the button
  - the originating label does not receive its own ping
*/
bool
tests_static_mediator_broadcast_reverse()
{
    ui_mediator med;
    button      b;
    label       l;

    med.register_colleague<0>(&b);
    med.register_colleague<1>(&l);

    l.nudge(11);

    bool ok = true;

    ok = ok && (l.got.empty());
    ok = ok && (b.got.size() == 1 && b.got[0] == 11);

    return ok;
}

/*
tests_static_colleague_send_no_mediator
  Verifies send() is a safe no-op when the colleague is unregistered.
  Tests the following:
  - an unregistered colleague has a null mediator
  - press() routes nowhere and does not crash
  - no colleague receives anything
*/
bool
tests_static_colleague_send_no_mediator()
{
    button solo;        // never registered

    bool ok = (solo.mediator() == nullptr);

    solo.press(9);      // must be a no-op
    ok = ok && (solo.got.empty());

    return ok;
}


NS_END  // testing
NS_END  // djinterp
