// djinterp [test] : mediator_tests_classic.cpp
//   The classic, virtual-dispatch layer (section III): colleague_id, the
// abstract mediator_base / colleague_base, and the dynamic concrete_mediator.
// A single concrete colleague fixture records what it receives and exposes the
// protected send()/mediator() accessors so the routing can be observed.

// std
#include <cstddef>
#include <type_traits>
#include <vector>
// djinterp
#include "mediator_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // test_colleague : a concrete colleague_base that logs deliveries and
    // surfaces the protected members for inspection.
    struct test_colleague : colleague_base
    {
        std::vector<int> received;        // payload of every delivered event
        colleague_id     last_sender = static_cast<colleague_id>(-1);

        void
        on_event(
            colleague_id       _sender,
            const compat::any& _event
        ) override
        {
            last_sender = _sender;
            received.push_back(_event.get<int>());

            return;
        }

        // do_send : drives the protected send() with an int payload.
        void
        do_send(
            int _value
        )
        {
            this->send(compat::any(_value));

            return;
        }

        // bound_mediator : surfaces the protected mediator() accessor.
        mediator_base*
        bound_mediator() const
        {
            return this->mediator();
        }
    };
}


/*
tests_colleague_id_type
  Verifies the colleague_id alias.
  Tests the following:
  - colleague_id is std::size_t
*/
bool
tests_colleague_id_type()
{
    static_assert(std::is_same<colleague_id, std::size_t>::value,
                  "colleague_id is std::size_t");

    return std::is_same<colleague_id, std::size_t>::value;
}

/*
tests_colleague_base_defaults
  Verifies the default-constructed colleague_base state and set_mediator.
  Tests the following:
  - a fresh colleague reports id() == 0 and a null bound mediator
  - set_mediator(m, id) records both the pointer and the id
*/
bool
tests_colleague_base_defaults()
{
    concrete_mediator med;
    test_colleague    c;

    bool ok = true;

    ok = ok && (c.id() == 0);
    ok = ok && (c.bound_mediator() == nullptr);

    c.set_mediator(&med, 7);
    ok = ok && (c.id() == 7);
    ok = ok && (c.bound_mediator() == &med);

    return ok;
}

/*
tests_concrete_mediator_add
  Verifies add_colleague id assignment, mediator binding, and the count.
  Tests the following:
  - ids are handed out sequentially from zero
  - each colleague is bound to the registering mediator
  - colleague_count reflects the number added
*/
bool
tests_concrete_mediator_add()
{
    concrete_mediator med;
    test_colleague    a, b, c;

    colleague_id ida = med.add_colleague(&a);
    colleague_id idb = med.add_colleague(&b);
    colleague_id idc = med.add_colleague(&c);

    bool ok = true;

    ok = ok && (ida == 0 && idb == 1 && idc == 2);
    ok = ok && (a.id() == 0 && b.id() == 1 && c.id() == 2);
    ok = ok && (a.bound_mediator() == &med);
    ok = ok && (b.bound_mediator() == &med);
    ok = ok && (med.colleague_count() == 3);

    return ok;
}

/*
tests_concrete_mediator_broadcast
  Verifies receive() broadcasts to every colleague except the sender.
  Tests the following:
  - a colleague's send() routes through the mediator to the others
  - the originating colleague does not receive its own event
  - recipients observe the correct payload and the sender's id
*/
bool
tests_concrete_mediator_broadcast()
{
    concrete_mediator med;
    test_colleague    a, b, c;

    colleague_id ida = med.add_colleague(&a);
    med.add_colleague(&b);
    med.add_colleague(&c);

    a.do_send(42);

    bool ok = true;

    ok = ok && (a.received.empty());
    ok = ok && (b.received.size() == 1 && b.received[0] == 42);
    ok = ok && (c.received.size() == 1 && c.received[0] == 42);
    ok = ok && (b.last_sender == ida);
    ok = ok && (c.last_sender == ida);

    return ok;
}

/*
tests_concrete_mediator_remove
  Verifies remove_colleague drops a participant and stops its delivery.
  Tests the following:
  - the count decreases after removal
  - a removed colleague no longer receives broadcasts
  - the remaining colleagues still receive
*/
bool
tests_concrete_mediator_remove()
{
    concrete_mediator med;
    test_colleague    a, b, c;

    med.add_colleague(&a);
    colleague_id idb = med.add_colleague(&b);
    med.add_colleague(&c);

    med.remove_colleague(idb);

    bool ok = (med.colleague_count() == 2);

    a.do_send(9);
    ok = ok && (b.received.empty());
    ok = ok && (c.received.size() == 1 && c.received[0] == 9);

    return ok;
}

/*
tests_colleague_send_no_mediator
  Verifies send() is a safe no-op when the colleague is unbound.
  Tests the following:
  - an unbound colleague reports a null mediator
  - send() neither routes anywhere nor throws
*/
bool
tests_colleague_send_no_mediator()
{
    test_colleague solo;

    bool ok = (solo.bound_mediator() == nullptr);

    solo.do_send(123);        // must be a no-op, not a crash
    ok = ok && (solo.received.empty());

    return ok;
}


NS_END  // testing
NS_END  // djinterp
