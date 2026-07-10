// djinterp [test] : mediator_tests_traits.cpp
//   Feature gates (section I) and the SFINAE protocol-detection traits
// (section II): the internal has_*_method probes, the is_mediator_like /
// is_colleague_like classifiers (and their _v forms), and the
// mediator_capability aggregate.
//
//   Notable, verified facts about the trait definitions: they probe SINGLE-
// argument protocol methods, whereas the classic colleague_base/mediator_base
// use two-argument signatures, and event_bus::subscribe is templated on the
// event type — so those framework types deliberately do NOT satisfy the
// narrow protocol probes.  The tests pin those truths down.

// std
#include <functional>
#include <type_traits>
// djinterp
#include "mediator_tests.hpp"


NS_DJINTERP
NS_TESTING

// shorthand for the internal trait namespace exercised throughout this TU.
namespace di = ::djinterp::internal;

namespace
{
    // ---- protocol probe types: each carries exactly the members needed to
    // ---- satisfy (or miss) a single trait ----

    // mp_none : no mediator protocol surface.
    struct mp_none
    {
    };

    // mp_notify : notify(const any&) only.
    struct mp_notify
    {
        void notify(const compat::any&) {}
    };

    // mp_subscribe : subscribe(function<void(const any&)>) only.
    struct mp_subscribe
    {
        void subscribe(std::function<void(const compat::any&)>) {}
    };

    // mp_set_mediator : single-argument set_mediator(ptr) only.
    struct mp_set_mediator
    {
        void set_mediator(void*) {}
    };

    // mp_on_event : single-argument on_event(const any&) only.
    struct mp_on_event
    {
        void on_event(const compat::any&) {}
    };

    // mp_all : the full narrow protocol surface.
    struct mp_all
    {
        void notify(const compat::any&) {}
        void subscribe(std::function<void(const compat::any&)>) {}
        void set_mediator(void*) {}
        void on_event(const compat::any&) {}
    };
}


/*
tests_feature_gates
  Verifies the configuration macros track the detected language level.
  Tests the following:
  - D_MEDIATOR_HAS_IF_CONSTEXPR mirrors the C++17 gate
  - D_MEDIATOR_HAS_CONCEPTS mirrors the C++20 gate
  - D_MEDIATOR_HAS_STRING_VIEW / D_MEDIATOR_HAS_VARIANT mirror the C++17 gate
  - D_MEDIATOR_DEFAULT_MAX_COLLEAGUES defaults to 64
*/
bool
tests_feature_gates()
{
    bool ok = true;

    ok = ok && (D_MEDIATOR_HAS_IF_CONSTEXPR ==
                (D_ENV_LANG_IS_CPP17_OR_HIGHER ? 1 : 0));
    ok = ok && (D_MEDIATOR_HAS_CONCEPTS ==
                (D_ENV_LANG_IS_CPP20_OR_HIGHER ? 1 : 0));
    ok = ok && (D_MEDIATOR_HAS_STRING_VIEW ==
                (D_ENV_LANG_IS_CPP17_OR_HIGHER ? 1 : 0));
    ok = ok && (D_MEDIATOR_HAS_VARIANT ==
                (D_ENV_LANG_IS_CPP17_OR_HIGHER ? 1 : 0));
    ok = ok && (D_MEDIATOR_DEFAULT_MAX_COLLEAGUES == 64);

    return ok;
}

/*
tests_has_notify_method
  Verifies internal::has_notify_method (detects notify(const any&)).
  Tests the following:
  - true for a type exposing notify(const any&) (mp_notify, event_bus)
  - false for a type without it (mp_none, int)
*/
bool
tests_has_notify_method()
{
    static_assert( di::has_notify_method<mp_notify>::value, "notify present");
    static_assert(!di::has_notify_method<mp_none>::value,   "notify absent");

    bool ok = true;

    ok = ok && ( di::has_notify_method<mp_notify>::value);
    ok = ok && ( di::has_notify_method<event_bus>::value);
    ok = ok && (!di::has_notify_method<mp_none>::value);
    ok = ok && (!di::has_notify_method<int>::value);

    return ok;
}

/*
tests_has_subscribe_method
  Verifies internal::has_subscribe_method (detects subscribe(function<...>)).
  Tests the following:
  - true for a type with the exact subscribe signature (mp_subscribe)
  - false for mp_none
  - false for event_bus, whose subscribe is templated on the event type and so
    cannot be called with a bare std::function (edge case)
*/
bool
tests_has_subscribe_method()
{
    static_assert( di::has_subscribe_method<mp_subscribe>::value, "subscribe present");
    static_assert(!di::has_subscribe_method<mp_none>::value,      "subscribe absent");

    bool ok = true;

    ok = ok && ( di::has_subscribe_method<mp_subscribe>::value);
    ok = ok && (!di::has_subscribe_method<mp_none>::value);
    ok = ok && (!di::has_subscribe_method<event_bus>::value);

    return ok;
}

/*
tests_has_set_mediator_method
  Verifies internal::has_set_mediator_method (detects set_mediator(ptr)).
  Tests the following:
  - true for a single-argument set_mediator(ptr) (mp_set_mediator)
  - false for mp_none
  - false for colleague_base, whose set_mediator takes two arguments (edge case)
*/
bool
tests_has_set_mediator_method()
{
    static_assert( di::has_set_mediator_method<mp_set_mediator>::value, "present");
    static_assert(!di::has_set_mediator_method<mp_none>::value,         "absent");

    bool ok = true;

    ok = ok && ( di::has_set_mediator_method<mp_set_mediator>::value);
    ok = ok && (!di::has_set_mediator_method<mp_none>::value);
    ok = ok && (!di::has_set_mediator_method<colleague_base>::value);

    return ok;
}

/*
tests_has_on_event_method
  Verifies internal::has_on_event_method (detects on_event(const any&)).
  Tests the following:
  - true for a single-argument on_event(const any&) (mp_on_event)
  - false for mp_none
  - false for colleague_base, whose on_event takes two arguments (edge case)
*/
bool
tests_has_on_event_method()
{
    static_assert( di::has_on_event_method<mp_on_event>::value, "present");
    static_assert(!di::has_on_event_method<mp_none>::value,     "absent");

    bool ok = true;

    ok = ok && ( di::has_on_event_method<mp_on_event>::value);
    ok = ok && (!di::has_on_event_method<mp_none>::value);
    ok = ok && (!di::has_on_event_method<colleague_base>::value);

    return ok;
}

/*
tests_is_mediator_like
  Verifies is_mediator_like (true iff notify-capable).
  Tests the following:
  - true for a notify-capable type (mp_notify, event_bus)
  - false otherwise (mp_none, int)
*/
bool
tests_is_mediator_like()
{
    static_assert( is_mediator_like<mp_notify>::value, "mediator-like");
    static_assert(!is_mediator_like<mp_none>::value,   "not mediator-like");

    bool ok = true;

    ok = ok && ( is_mediator_like<mp_notify>::value);
    ok = ok && ( is_mediator_like<event_bus>::value);
    ok = ok && (!is_mediator_like<mp_none>::value);
    ok = ok && (!is_mediator_like<int>::value);

    return ok;
}

/*
tests_is_colleague_like
  Verifies is_colleague_like (true iff it can set_mediator OR on_event) and its
  _v form.
  Tests the following:
  - true via set_mediator (mp_set_mediator)
  - true via on_event (mp_on_event)
  - false when neither is present (mp_none, int)
  - the _v alias mirrors the trait's ::value
*/
bool
tests_is_colleague_like()
{
    static_assert( is_colleague_like<mp_set_mediator>::value, "colleague-like");
    static_assert(!is_colleague_like<mp_none>::value,         "not colleague-like");

    bool ok = true;

    ok = ok && ( is_colleague_like<mp_set_mediator>::value);
    ok = ok && ( is_colleague_like<mp_on_event>::value);
    ok = ok && (!is_colleague_like<mp_none>::value);
    ok = ok && (!is_colleague_like<int>::value);

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    ok = ok && (is_colleague_like_v<mp_on_event> ==
                is_colleague_like<mp_on_event>::value);
    ok = ok && (is_mediator_like_v<mp_notify> ==
                is_mediator_like<mp_notify>::value);
    ok = ok && (is_colleague_like_v<mp_none> == false);
#endif

    return ok;
}

/*
tests_mediator_capability
  Verifies the mediator_capability aggregate across a fully-conforming type and
  a bare type, driving every member true and false.
  Tests the following:
  - mp_all: every capability flag true (has_notify/subscribe/set_mediator/
    on_event, is_mediator, is_colleague)
  - mp_none: every capability flag false
*/
bool
tests_mediator_capability()
{
    using all  = mediator_capability<mp_all>;
    using none = mediator_capability<mp_none>;

    bool ok = true;

    ok = ok && ( all::has_notify);
    ok = ok && ( all::has_subscribe);
    ok = ok && ( all::has_set_mediator);
    ok = ok && ( all::has_on_event);
    ok = ok && ( all::is_mediator);
    ok = ok && ( all::is_colleague);

    ok = ok && (!none::has_notify);
    ok = ok && (!none::has_subscribe);
    ok = ok && (!none::has_set_mediator);
    ok = ok && (!none::has_on_event);
    ok = ok && (!none::is_mediator);
    ok = ok && (!none::is_colleague);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
