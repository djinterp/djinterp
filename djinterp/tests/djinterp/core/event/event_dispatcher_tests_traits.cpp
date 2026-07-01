/******************************************************************************
* djinterp [test]                          event_dispatcher_tests_traits.cpp
*
*   Sections V-VI -- STRUCTURAL DETECTION HELPERS and EVENT DISPATCHER TRAITS.
* event_dispatcher_traits performs compile-time structural detection of the
* facade interface.  Covers the real event_dispatcher (every has_* flag and the
* composite is_event_dispatcher hold); a negative type (none hold); a
* duck-typed struct exposing the ten non-template operations with the detected
* signatures (is_event_dispatcher holds even though it is unrelated to
* event_dispatcher -- the check is structural, not nominal); clean_t
* normalization (cv/ref-qualified spellings detect identically); and the typed
* detection traits event_dispatcher_has_bind / _has_fire / _has_queue, which
* hold for the real dispatcher and fail for a non-dispatcher.
*
*   These tests are deliberately standard-agnostic: the traits are plain
* SFINAE and behave identically across C++11..C++23.  (The concepts layered on
* top of them is covered separately, under its C++20 gate.)
*
*
* path:      /tests/djinterp/core/event/event_dispatcher/event_dispatcher_tests_traits.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.30
******************************************************************************/

// djinterp
#include "event_dispatcher_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_traits_real_dispatcher
bool
tests_traits_real_dispatcher()
{
    bool ok = true;

    typedef event_dispatcher_traits<event_dispatcher> T;

    // every individual operation is detected.
    ok = D_ED_CHECK(T::has_unbind) && ok;
    ok = D_ED_CHECK(T::has_enable) && ok;
    ok = D_ED_CHECK(T::has_disable) && ok;
    ok = D_ED_CHECK(T::has_is_enabled) && ok;
    ok = D_ED_CHECK(T::has_contains) && ok;
    ok = D_ED_CHECK(T::has_handler_count) && ok;
    ok = D_ED_CHECK(T::has_enabled_count) && ok;
    ok = D_ED_CHECK(T::has_pending_events) && ok;
    ok = D_ED_CHECK(T::has_process) && ok;
    ok = D_ED_CHECK(T::has_process_all) && ok;

    // and the composite holds.
    ok = D_ED_CHECK(T::is_event_dispatcher) && ok;

    return ok;
}


// tests_traits_negative
bool
tests_traits_negative()
{
    bool ok = true;

    typedef event_dispatcher_traits<not_disp> T;

    // a plain type satisfies none of the interface.
    ok = D_ED_CHECK(!T::has_unbind) && ok;
    ok = D_ED_CHECK(!T::has_process) && ok;
    ok = D_ED_CHECK(!T::has_pending_events) && ok;
    ok = D_ED_CHECK(!T::is_event_dispatcher) && ok;

    return ok;
}


// tests_traits_duck_typed_structural
bool
tests_traits_duck_typed_structural()
{
    bool ok = true;

    // a duck-typed struct exposing the ten facade operations satisfies the
    // structural interface, proving the detection is structural, not nominal.
    typedef event_dispatcher_traits<duck> T;

    ok = D_ED_CHECK(T::has_unbind) && ok;
    ok = D_ED_CHECK(T::has_is_enabled) && ok;
    ok = D_ED_CHECK(T::has_handler_count) && ok;
    ok = D_ED_CHECK(T::has_process) && ok;
    ok = D_ED_CHECK(T::has_process_all) && ok;
    ok = D_ED_CHECK(T::is_event_dispatcher) && ok;

    return ok;
}


// tests_traits_clean_t_normalization
bool
tests_traits_clean_t_normalization()
{
    bool ok = true;

    // cv/ref-qualified spellings normalize through clean_t and detect like the
    // bare type.
    ok = D_ED_CHECK(event_dispatcher_traits<event_dispatcher&>::is_event_dispatcher) && ok;
    ok = D_ED_CHECK(event_dispatcher_traits<const event_dispatcher&>::is_event_dispatcher) && ok;
    ok = D_ED_CHECK(event_dispatcher_traits<event_dispatcher&&>::is_event_dispatcher) && ok;
    ok = D_ED_CHECK(event_dispatcher_traits<const event_dispatcher>::is_event_dispatcher) && ok;

    return ok;
}


// tests_traits_typed_bind_fire_queue
bool
tests_traits_typed_bind_fire_queue()
{
    bool ok = true;

    // typed structural detection holds for the real dispatcher...
    ok = D_ED_CHECK((event_dispatcher_has_bind<event_dispatcher, ev_int, summing>::value)) && ok;
    ok = D_ED_CHECK((event_dispatcher_has_fire<event_dispatcher, ev_int, int>::value)) && ok;
    ok = D_ED_CHECK((event_dispatcher_has_queue<event_dispatcher, ev_int, int>::value)) && ok;

    // ...and fails for a type lacking the typed operations.
    ok = D_ED_CHECK(!(event_dispatcher_has_bind<not_disp, ev_int, summing>::value)) && ok;
    ok = D_ED_CHECK(!(event_dispatcher_has_fire<not_disp, ev_int, int>::value)) && ok;
    ok = D_ED_CHECK(!(event_dispatcher_has_queue<not_disp, ev_int, int>::value)) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
