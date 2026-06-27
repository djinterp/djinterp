// djinterp
#include "test_timer_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_test_timer_event_tag_names
  Verifies each nested event tag reports its documented name.
  Tests the following:
  - on_start / on_stop / on_expire / on_reset name() return their literals
*/
bool
tests_test_timer_event_tag_names()
{
    reset_clock();

    D_TT_CHECK(std::strcmp(tt::on_start::name(),
                           "test_timer::on_start")  == 0);
    D_TT_CHECK(std::strcmp(tt::on_stop::name(),
                           "test_timer::on_stop")   == 0);
    D_TT_CHECK(std::strcmp(tt::on_expire::name(),
                           "test_timer::on_expire") == 0);
    D_TT_CHECK(std::strcmp(tt::on_reset::name(),
                           "test_timer::on_reset")  == 0);

    return true;
}


/*
tests_test_timer_event_tag_args
  Verifies the argument tuple of each event tag (a compile-time contract).
  Tests the following:
  - on_start / on_expire carry no arguments
  - on_stop / on_reset each carry a single rep_type count
*/
bool
tests_test_timer_event_tag_args()
{
    reset_clock();

    static_assert(
        std::is_same<tt::on_start::args_type, std::tuple<>>::value,
        "on_start carries no arguments");

    static_assert(
        std::is_same<tt::on_expire::args_type, std::tuple<>>::value,
        "on_expire carries no arguments");

    static_assert(
        std::is_same<tt::on_stop::args_type, std::tuple<tt::rep_type>>::value,
        "on_stop carries one rep_type");

    static_assert(
        std::is_same<tt::on_reset::args_type, std::tuple<tt::rep_type>>::value,
        "on_reset carries one rep_type");

    return true;
}


/*
tests_test_timer_no_handler_noop
  Verifies the zero-overhead path: with no dispatcher, every operation runs
  without error and dispatches nothing.
  Tests the following:
  - start / stop / reset / reset_all complete on a handler-less timer
  - the handler stays null and final state is consistent
*/
bool
tests_test_timer_no_handler_noop()
{
    reset_clock();

    tt t;

    t.start();
    test_clock::advance(msec(35));
    t.stop();
    t.reset();
    t.reset_all();

    D_TT_CHECK(t.handler() == nullptr);
    D_TT_CHECK(t.elapsed() == msec(0));
    D_TT_CHECK(t.running() == false);

    return true;
}


/*
tests_test_timer_handler_accessor
  Verifies handler() reports the attached dispatcher.
  Tests the following:
  - a timer built with a dispatcher returns it
  - a default timer returns nullptr
*/
bool
tests_test_timer_handler_accessor()
{
    reset_clock();

    event_dispatcher eh;

    tt with_handler(&eh);
    tt without;

    D_TT_CHECK(with_handler.handler() == &eh);
    D_TT_CHECK(without.handler()      == nullptr);

    return true;
}


/*
tests_test_timer_set_handler_attach_detach
  Verifies set_handler() attaches and detaches dispatch at runtime.
  Tests the following:
  - attaching a dispatcher causes subsequent operations to fire events
  - detaching (nullptr) silences subsequent operations
*/
bool
tests_test_timer_set_handler_attach_detach()
{
    reset_clock();

    event_dispatcher eh;
    event_log        log;
    bind_log(eh, log);

    tt t;

    D_TT_CHECK(t.handler() == nullptr);

    t.set_handler(&eh);

    D_TT_CHECK(t.handler() == &eh);

    t.start();

    D_TT_CHECK(log.starts == 1);

    t.set_handler(nullptr);

    D_TT_CHECK(t.handler() == nullptr);

    t.stop();

    D_TT_CHECK(log.stops == 0);

    return true;
}


/*
tests_test_timer_set_handler_swaps
  Verifies set_handler() can redirect dispatch from one dispatcher to another.
  Tests the following:
  - events fire to the first dispatcher before the swap
  - after the swap, events fire to the second and not the first
*/
bool
tests_test_timer_set_handler_swaps()
{
    reset_clock();

    event_dispatcher eh1;
    event_dispatcher eh2;
    event_log        log1;
    event_log        log2;
    bind_log(eh1, log1);
    bind_log(eh2, log2);

    tt t(&eh1);

    t.start();

    D_TT_CHECK(log1.starts == 1);
    D_TT_CHECK(log2.starts == 0);

    t.stop();

    t.set_handler(&eh2);

    t.start();

    D_TT_CHECK(log2.starts == 1);
    D_TT_CHECK(log1.starts == 1);

    return true;
}


NS_END  // testing
NS_END  // djinterp
