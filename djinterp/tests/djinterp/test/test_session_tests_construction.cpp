// djinterp
#include "test_session_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_session_default_state
  Verifies a default-constructed session starts idle.
  Tests the following:
  - state() == session_state::idle
  - is_idle() true; is_running() / is_paused() / is_finished() false
*/
bool
tests_session_default_state()
{
    session_type s;

    return ( (s.state() == session_state::idle) &&
             (s.is_idle()     == true)          &&
             (s.is_running()  == false)         &&
             (s.is_paused()   == false)         &&
             (s.is_finished() == false) );
}

/*
tests_session_default_counters_zero
  Verifies every status counter (and the total) starts at zero.
  Tests the following:
  - passed / failed / skipped / pending / errors values are 0
  - total() == 0
*/
bool
tests_session_default_counters_zero()
{
    session_type s;

    return ( (s.passed().value()  == 0) &&
             (s.failed().value()  == 0) &&
             (s.skipped().value() == 0) &&
             (s.pending().value() == 0) &&
             (s.errors().value()  == 0) &&
             (s.total()           == 0) );
}

/*
tests_session_default_tree_and_timer
  Verifies the owned tree is empty and the timer is idle on construction.
  Tests the following:
  - tree() is empty (size 0)
  - timer() is not running
  - elapsed() count is 0
*/
bool
tests_session_default_tree_and_timer()
{
    session_type s;

    return ( (s.tree().size()        == 0)     &&
             (s.timer().running()    == false) &&
             (s.elapsed().count()    == 0) );
}

/*
tests_session_type_aliases
  Verifies the public type aliases resolve as documented.
  Tests the following:
  - element_type / underlying_type / tree_type
  - counter_type (int64 counter) / timer_type / size_type
*/
bool
tests_session_type_aliases()
{
    static_assert(
        std::is_same<session_type::element_type, basic_test>::value,
        "element_type should be the element type");

    static_assert(
        std::is_same<session_type::underlying_type,
                     nary_tree<basic_test>>::value,
        "underlying_type should be the backing forest type");

    static_assert(
        std::is_same<session_type::tree_type,
                     test_tree<basic_test, nary_tree<basic_test>>>::value,
        "tree_type should be test_tree<_Element, _Underlying>");

    static_assert(
        std::is_same<session_type::counter_type,
                     ::djinterp::test::test_counter<std::int64_t>>::value,
        "counter_type should be test_counter<std::int64_t>");

    static_assert(
        std::is_same<session_type::timer_type,
                     ::djinterp::test::test_timer<>>::value,
        "timer_type should be test_timer<>");

    static_assert(
        std::is_same<session_type::size_type, std::size_t>::value,
        "size_type should be std::size_t");

    return true;
}


NS_END  // testing
NS_END  // djinterp
