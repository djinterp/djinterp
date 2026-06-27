// djinterp
#include "test_counter_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_test_counter_event_tag_names
  Verifies each nested event tag reports its documented name.
  Tests the following:
  - on_increment / on_decrement / on_limit / on_reset name() return their
    literals
*/
bool
tests_test_counter_event_tag_names()
{
    D_TC_CHECK(std::strcmp(tc::on_increment::name(),
                           "test_counter::on_increment") == 0);
    D_TC_CHECK(std::strcmp(tc::on_decrement::name(),
                           "test_counter::on_decrement") == 0);
    D_TC_CHECK(std::strcmp(tc::on_limit::name(),
                           "test_counter::on_limit")     == 0);
    D_TC_CHECK(std::strcmp(tc::on_reset::name(),
                           "test_counter::on_reset")     == 0);

    return true;
}


/*
tests_test_counter_event_tag_args
  Verifies the argument tuple of each event tag (a compile-time contract).
  Tests the following:
  - on_increment / on_decrement each carry (old, new) value pairs
  - on_limit / on_reset each carry a single value
*/
bool
tests_test_counter_event_tag_args()
{
    static_assert(
        std::is_same<tc::on_increment::args_type,
                     std::tuple<tc::value_type, tc::value_type>>::value,
        "on_increment carries (old, new)");

    static_assert(
        std::is_same<tc::on_decrement::args_type,
                     std::tuple<tc::value_type, tc::value_type>>::value,
        "on_decrement carries (old, new)");

    static_assert(
        std::is_same<tc::on_limit::args_type,
                     std::tuple<tc::value_type>>::value,
        "on_limit carries (clamped_value)");

    static_assert(
        std::is_same<tc::on_reset::args_type,
                     std::tuple<tc::value_type>>::value,
        "on_reset carries (old)");

    return true;
}


/*
tests_test_counter_no_handler_noop
  Verifies the zero-overhead path: with no dispatcher, every operation runs
  without error and dispatches nothing.
  Tests the following:
  - increment / decrement (including a clamp) / reset / reset_all complete on a
    handler-less counter
  - the handler stays null and final state is consistent
*/
bool
tests_test_counter_no_handler_noop()
{
    tc c(0, 0, 100);

    c.increment(5);
    c.decrement(2);
    c.increment(200);
    c.reset();
    c.reset_all();

    D_TC_CHECK(c.handler() == nullptr);
    D_TC_CHECK(c.value()   == 0);
    D_TC_CHECK(c.at_min()  == true);

    return true;
}


/*
tests_test_counter_handler_accessor
  Verifies handler() reports the attached dispatcher.
  Tests the following:
  - a counter built with a dispatcher returns it
  - a default counter returns nullptr
*/
bool
tests_test_counter_handler_accessor()
{
    event_dispatcher eh;

    tc with_handler(0, 0, 100, &eh);
    tc without;

    D_TC_CHECK(with_handler.handler() == &eh);
    D_TC_CHECK(without.handler()      == nullptr);

    return true;
}


/*
tests_test_counter_set_handler_attach_detach
  Verifies set_handler() attaches and detaches dispatch at runtime.
  Tests the following:
  - attaching a dispatcher causes subsequent operations to fire events
  - detaching (nullptr) silences subsequent operations
*/
bool
tests_test_counter_set_handler_attach_detach()
{
    event_dispatcher eh;
    event_log        log;
    bind_log(eh, log);

    tc c(0, 0, 100);

    D_TC_CHECK(c.handler() == nullptr);

    c.set_handler(&eh);

    D_TC_CHECK(c.handler() == &eh);

    c.increment(5);

    D_TC_CHECK(log.increments == 1);

    c.set_handler(nullptr);

    D_TC_CHECK(c.handler() == nullptr);

    c.increment(5);

    D_TC_CHECK(log.increments == 1);

    return true;
}


/*
tests_test_counter_set_handler_swaps
  Verifies set_handler() can redirect dispatch from one dispatcher to another.
  Tests the following:
  - events fire to the first dispatcher before the swap
  - after the swap, events fire to the second and not the first
*/
bool
tests_test_counter_set_handler_swaps()
{
    event_dispatcher eh1;
    event_dispatcher eh2;
    event_log        log1;
    event_log        log2;
    bind_log(eh1, log1);
    bind_log(eh2, log2);

    tc c(0, 0, 100, &eh1);

    c.increment(5);

    D_TC_CHECK(log1.increments == 1);
    D_TC_CHECK(log2.increments == 0);

    c.set_handler(&eh2);

    c.increment(5);

    D_TC_CHECK(log2.increments == 1);
    D_TC_CHECK(log1.increments == 1);

    return true;
}


NS_END  // testing
NS_END  // djinterp
