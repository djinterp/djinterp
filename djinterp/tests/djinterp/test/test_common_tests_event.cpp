// std
#include <cstddef>
#include <cstring>
#include <limits>
#include <type_traits>
// djinterp
#include "test_common_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace ::djinterp::test;


///////////////////////////////////////////////////////////////////////////////
///   compile-time invariants -- III. EVENT SYSTEM                            ///
///////////////////////////////////////////////////////////////////////////////

// -- test_event_id : exactly std::size_t --------------------------------------
static_assert(std::is_same<test_event_id, std::size_t>::value,
              "test_event_id must be std::size_t");
static_assert(std::is_integral<test_event_id>::value,
              "test_event_id must be integral");
static_assert(std::is_unsigned<test_event_id>::value,
              "test_event_id must be unsigned");
static_assert(std::numeric_limits<test_event_id>::max()
                  == std::numeric_limits<std::size_t>::max(),
              "test_event_id must span the whole std::size_t range");

// -- test_event : member layout and value semantics ---------------------------
static_assert(std::is_same<decltype(test_event::event), test_event_id>::value,
              "test_event::event must be test_event_id");
static_assert(std::is_same<decltype(test_event::status), test_status>::value,
              "test_event::status must be test_status");
static_assert(std::is_same<decltype(test_event::message), const char*>::value,
              "test_event::message must be const char*");

static_assert(std::is_standard_layout<test_event>::value,
              "test_event must be standard-layout");
static_assert(std::is_trivially_copyable<test_event>::value,
              "test_event must stay trivially copyable despite its user ctor");
static_assert(std::is_trivially_destructible<test_event>::value,
              "test_event must be trivially destructible");
static_assert(!std::is_default_constructible<test_event>::value,
              "the user-declared ctor must suppress a default ctor");
static_assert(std::is_copy_constructible<test_event>::value,
              "test_event must be copy-constructible");
static_assert(std::is_move_constructible<test_event>::value,
              "test_event must be move-constructible");

// the constructor is noexcept in both its 2-arg and 3-arg forms
static_assert(std::is_nothrow_constructible<test_event,
                  test_event_id, test_status>::value,
              "2-arg test_event ctor must be noexcept");
static_assert(std::is_nothrow_constructible<test_event,
                  test_event_id, test_status, const char*>::value,
              "3-arg test_event ctor must be noexcept");

// the constructor is usable in a constant expression (constexpr ctor)
namespace
{
    constexpr test_event k_ce_default(7, test_status::pending);
    constexpr test_event k_ce_message(9, test_status::error, "x");
}
static_assert(k_ce_default.event   == 7,                   "");
static_assert(k_ce_default.status  == test_status::pending, "");
static_assert(k_ce_default.message == nullptr,
              "omitted message must default to nullptr at compile time");
static_assert(k_ce_message.message != nullptr,
              "explicit message must be retained at compile time");

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
static_assert(!std::is_aggregate<test_event>::value,
              "the user-declared ctor must make test_event a non-aggregate");
#endif


///////////////////////////////////////////////////////////////////////////////
///   runtime tests -- III. EVENT SYSTEM                                      ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_test_event_id
  Verifies the event-stream sequence type.
  Tests the following:
  - exact identity with std::size_t and unsigned-integral classification
  - capacity spans the full std::size_t range
  - representative sequence numbers round-trip intact
  - unsigned wrap-around at the maximum is well defined
*/
bool
tests_test_event_id()
{
    bool ok = true;

    // identity & classification
    ok = D_TC_CHECK(std::is_same<test_event_id, std::size_t>::value) && ok;
    ok = D_TC_CHECK(std::is_integral<test_event_id>::value)          && ok;
    ok = D_TC_CHECK(std::is_unsigned<test_event_id>::value)          && ok;

    // capacity
    ok = D_TC_CHECK(std::numeric_limits<test_event_id>::max()
                        == std::numeric_limits<std::size_t>::max()) && ok;

    // representative values round-trip
    {
        test_event_id first = 0;
        test_event_id mid   = 1000;
        test_event_id last  = std::numeric_limits<test_event_id>::max();

        ok = D_TC_CHECK(first == 0u)    && ok;
        ok = D_TC_CHECK(mid   == 1000u) && ok;
        ok = D_TC_CHECK(first <  mid)   && ok;
        ok = D_TC_CHECK(mid   <  last)  && ok;
    }

    // unsigned arithmetic wraps cleanly (well-defined, not UB)
    {
        test_event_id last = std::numeric_limits<test_event_id>::max();
        ok = D_TC_CHECK(static_cast<test_event_id>(last + 1u) == 0u) && ok;
    }

    return ok;
}


/*
tests_test_event
  Verifies the immutable-by-convention event record.
  Tests the following:
  - member types match the declared types
  - the full 3-argument constructor stores each field verbatim
  - the 2-argument form defaults message to nullptr (default-argument path)
  - message is stored by pointer identity (the string is referenced, not copied)
  - nullptr and the empty string "" are distinct message states
  - brace- and paren-initialization agree
  - copies are independent: mutating a copy leaves the source untouched
  - public members are reassignable after construction
  - extreme field values round-trip intact
  - the constructor is usable in a constant expression
*/
bool
tests_test_event()
{
    bool ok = true;

    // member types
    ok = D_TC_CHECK(std::is_same<decltype(test_event::event),
                                 test_event_id>::value)              && ok;
    ok = D_TC_CHECK(std::is_same<decltype(test_event::status),
                                 test_status>::value)                && ok;
    ok = D_TC_CHECK(std::is_same<decltype(test_event::message),
                                 const char*>::value)                && ok;

    // full 3-argument construction stores each field (runtime-forced inputs so
    // the constructor body is genuinely executed, not folded away)
    {
        volatile test_event_id v_id = 314159;
        test_event_id          id   = v_id;
        const char*            msg  = "diagnostic message";

        test_event e(id, test_status::failed, msg);

        ok = D_TC_CHECK(e.event == id)                  && ok;
        ok = D_TC_CHECK(e.status == test_status::failed) && ok;
        ok = D_TC_CHECK(e.message == msg)                && ok;  // pointer kept
        ok = D_TC_CHECK(std::strcmp(e.message, "diagnostic message") == 0)
             && ok;
    }

    // 2-argument construction: message defaults to nullptr
    {
        volatile test_event_id v_id = 271828;
        test_event_id          id   = v_id;

        test_event e(id, test_status::passed);

        ok = D_TC_CHECK(e.event == id)                   && ok;
        ok = D_TC_CHECK(e.status == test_status::passed)  && ok;
        ok = D_TC_CHECK(e.message == nullptr)             && ok;
    }

    // nullptr message vs. empty string "" are distinct states
    {
        test_event none(1, test_status::skipped);
        test_event empty(1, test_status::skipped, "");

        ok = D_TC_CHECK(none.message == nullptr)   && ok;
        ok = D_TC_CHECK(empty.message != nullptr)  && ok;
        ok = D_TC_CHECK(empty.message[0] == '\0')  && ok;
    }

    // paren- and brace-initialization produce equal records
    {
        test_event a(2, test_status::error, "boom");
        test_event b{2, test_status::error, "boom"};

        ok = D_TC_CHECK(a.event == b.event)     && ok;
        ok = D_TC_CHECK(a.status == b.status)   && ok;
        ok = D_TC_CHECK(a.message == b.message) && ok;
    }

    // copies are deep-enough to be independent (trivially copyable value type)
    {
        test_event source(10, test_status::pending, "orig");
        test_event copy(source);          // copy construction
        test_event assigned(0, test_status::passed);
        assigned = source;                // copy assignment

        ok = D_TC_CHECK(copy.event == 10u)                  && ok;
        ok = D_TC_CHECK(copy.status == test_status::pending) && ok;
        ok = D_TC_CHECK(copy.message == source.message)      && ok;
        ok = D_TC_CHECK(assigned.event == 10u)               && ok;
        ok = D_TC_CHECK(assigned.status == test_status::pending) && ok;

        // mutating the copy must not disturb the source
        copy.event   = 999;
        copy.status  = test_status::error;
        ok = D_TC_CHECK(copy.event == 999u)                  && ok;
        ok = D_TC_CHECK(source.event == 10u)                 && ok;
        ok = D_TC_CHECK(source.status == test_status::pending) && ok;
    }

    // public members are reassignable after construction
    {
        test_event e(0, test_status::passed);

        e.event   = 77;
        e.status  = test_status::error;
        e.message = "updated";

        ok = D_TC_CHECK(e.event == 77u)                          && ok;
        ok = D_TC_CHECK(e.status == test_status::error)           && ok;
        ok = D_TC_CHECK(std::strcmp(e.message, "updated") == 0)   && ok;

        e.message = nullptr;
        ok = D_TC_CHECK(e.message == nullptr)                     && ok;
    }

    // extreme field values round-trip
    {
        test_event_id huge = std::numeric_limits<test_event_id>::max();
        test_event    e(huge, test_status::error, "max");

        ok = D_TC_CHECK(e.event == huge)                && ok;
        ok = D_TC_CHECK(e.status == test_status::error)  && ok;
        ok = D_TC_CHECK(std::strcmp(e.message, "max") == 0) && ok;
    }

    // usable in a constant expression
    {
        D_CONSTEXPR test_event ce(5, test_status::pending);
        ok = D_TC_CHECK(ce.event == 5u)                   && ok;
        ok = D_TC_CHECK(ce.status == test_status::pending) && ok;
        ok = D_TC_CHECK(ce.message == nullptr)             && ok;
    }

    return ok;
}


NS_END  // testing
NS_END  // djinterp
