/******************************************************************************
* djinterp [test]                       event_dispatcher_tests_concepts.cpp
*
*   Section VII -- CONCEPT CONSTRAINTS (C++20+).  Exercises the dispatcher
* concepts layered over event_dispatcher_traits.  Core: is_event_dispatcher_type
* / event_dispatcher_type / non_event_dispatcher_type and the process-capability
* refinements processing_/draining_event_dispatcher_type.  Typed capability:
* event_dispatcher_bindable_to (which additionally requires handler_for, so a
* non-handler such as int is rejected even though the bare structural has_bind
* would accept it), event_dispatcher_fireable_for, event_dispatcher_queueable_for.
* Composite: firing_/queueing_/full_event_dispatcher_for, conjunctions of the
* above.
*
*   Each test body is gated on D_ENV_CPP_FEATURE_LANG_CONCEPTS.  When concepts
* are unavailable (pre-C++20) the bodies compile to a trivial pass so the suite
* builds and runs uniformly across standards; the declarations in the header
* are unconditional.
*
*
* path:      /tests/djinterp/core/event/event_dispatcher/event_dispatcher_tests_concepts.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.30
******************************************************************************/

// djinterp
#include "event_dispatcher_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_concepts_core
bool
tests_concepts_core()
{
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    bool ok = true;

    // the facade satisfies the core dispatcher concepts...
    ok = D_ED_CHECK(is_event_dispatcher_type<event_dispatcher>) && ok;
    ok = D_ED_CHECK(event_dispatcher_type<event_dispatcher>) && ok;
    // ...including through cv/ref-qualified spellings.
    ok = D_ED_CHECK(event_dispatcher_type<const event_dispatcher&>) && ok;

    // a plain type is not a dispatcher.
    ok = D_ED_CHECK(non_event_dispatcher_type<not_disp>) && ok;
    ok = D_ED_CHECK(!event_dispatcher_type<not_disp>) && ok;

    // process-capability refinements.
    ok = D_ED_CHECK(processing_event_dispatcher_type<event_dispatcher>) && ok;
    ok = D_ED_CHECK(draining_event_dispatcher_type<event_dispatcher>) && ok;
    ok = D_ED_CHECK(!processing_event_dispatcher_type<not_disp>) && ok;

    return ok;
#else
    // concepts unavailable on this standard; trivially pass.
    return true;
#endif
}


// tests_concepts_typed_capability
bool
tests_concepts_typed_capability()
{
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    bool ok = true;

    // bindable_to: a compatible handler for the event binds...
    ok = D_ED_CHECK((event_dispatcher_bindable_to<event_dispatcher, summing, ev_int>)) && ok;
    // ...but bindable_to also requires handler_for, so a non-handler (int) is
    // rejected even though the bare structural has_bind would accept it.
    ok = D_ED_CHECK((!event_dispatcher_bindable_to<event_dispatcher, int, ev_int>)) && ok;

    // fireable / queueable for the event with matching args.
    ok = D_ED_CHECK((event_dispatcher_fireable_for<event_dispatcher, ev_int, int>)) && ok;
    ok = D_ED_CHECK((event_dispatcher_queueable_for<event_dispatcher, ev_int, int>)) && ok;

    // none of these hold for a non-dispatcher.
    ok = D_ED_CHECK((!event_dispatcher_bindable_to<not_disp, summing, ev_int>)) && ok;
    ok = D_ED_CHECK((!event_dispatcher_fireable_for<not_disp, ev_int, int>)) && ok;
    ok = D_ED_CHECK((!event_dispatcher_queueable_for<not_disp, ev_int, int>)) && ok;

    return ok;
#else
    return true;
#endif
}


// tests_concepts_composite
bool
tests_concepts_composite()
{
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    bool ok = true;

    // the facade can bind a handler for ev_int and both fire and queue it.
    ok = D_ED_CHECK((firing_event_dispatcher_for<event_dispatcher, summing, ev_int, int>)) && ok;
    ok = D_ED_CHECK((queueing_event_dispatcher_for<event_dispatcher, summing, ev_int, int>)) && ok;
    ok = D_ED_CHECK((full_event_dispatcher_for<event_dispatcher, summing, ev_int, int>)) && ok;

    // if the callable is not a handler for the event, the bindable conjunct
    // fails and so do all three composites.
    ok = D_ED_CHECK((!firing_event_dispatcher_for<event_dispatcher, int, ev_int, int>)) && ok;
    ok = D_ED_CHECK((!queueing_event_dispatcher_for<event_dispatcher, int, ev_int, int>)) && ok;
    ok = D_ED_CHECK((!full_event_dispatcher_for<event_dispatcher, int, ev_int, int>)) && ok;

    return ok;
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
