/******************************************************************************
* djinterp [test]                                  monad_tests_operations.cpp
*
*   Tests for monad.hpp Section II core operations: monad_unit,
* monad_bind, monad_map, and monad_join.  Covers the success path and
* the short-circuit (none) path for each, type-changing maps, the
* functor identity and composition laws for map, and the left/right
* identity monad laws for unit/bind.
******************************************************************************/
#include "./monad_tests.hpp"


NS_DJINTERP
NS_TESTING

using ::djinterp::test::test_handler;
using ::djinterp::test::unit_test_tally;
using ::djinterp::test::run_unit_test;
using ::djinterp::test::record_assertion;

/*
monad_tests_operations
  Exercises monad.hpp Section II core operations.
  Tests the following:
  - monad_unit lifts a value (explicit monad type argument)
  - monad_bind on some applies the arrow; on none short-circuits
  - monad_bind chains and propagates a mid-chain failure
  - monad_map on some applies the transform; on none short-circuits
  - monad_map changes the inner type (int -> string)
  - monad_map obeys the functor identity law (map id == id)
  - monad_map obeys the functor composition law
  - monad_join flattens some(some(x)); none and some(none) stay empty
  - left identity:  bind(unit(a), f) == f(a)
  - right identity: bind(m, unit) == m
*/
void
monad_tests_operations(
    test_handler& _handler
)
{
    unit_test_tally tally;

    // monad_unit
    run_unit_test(
        _handler,
        tally,
        "monad_unit lifts a value into the named monad",
        [&]()
        {
            auto m = ::djinterp::monad_unit<test_maybe<int> >(5);

            record_assertion(
                _handler,
                (m.is_some() && m.value() == 5),
                "monad_unit<test_maybe<int>>(5) is some(5)");
        });

    // monad_bind on some
    run_unit_test(
        _handler,
        tally,
        "monad_bind applies the arrow on a some value",
        [&]()
        {
            auto m = ::djinterp::monad_bind(some(5), arrow_inc());

            record_assertion(
                _handler,
                (m.is_some() && m.value() == 6),
                "bind(some(5), inc) is some(6)");
        });

    // monad_bind on none
    run_unit_test(
        _handler,
        tally,
        "monad_bind short-circuits on a none value",
        [&]()
        {
            auto m = ::djinterp::monad_bind(test_maybe<int>::none(),
                                            arrow_inc());

            record_assertion(
                _handler,
                (!m.is_some()),
                "bind(none, inc) is none");
        });

    // monad_bind chains with a mid-chain failure
    run_unit_test(
        _handler,
        tally,
        "monad_bind propagates a failure introduced mid-chain",
        [&]()
        {
            // some(5) -> inc -> some(6) -> to_none -> none -> inc -> none
            auto m = ::djinterp::monad_bind(
                ::djinterp::monad_bind(
                    ::djinterp::monad_bind(some(5), arrow_inc()),
                    arrow_to_none()),
                arrow_inc());

            record_assertion(
                _handler,
                (!m.is_some()),
                "a none introduced mid-chain propagates to the end");
        });

    // monad_map on some
    run_unit_test(
        _handler,
        tally,
        "monad_map applies the transform on a some value",
        [&]()
        {
            auto m = ::djinterp::monad_map(some(5), plain_double());

            record_assertion(
                _handler,
                (m.is_some() && m.value() == 10),
                "map(some(5), double) is some(10)");
        });

    // monad_map on none
    run_unit_test(
        _handler,
        tally,
        "monad_map short-circuits on a none value",
        [&]()
        {
            auto m = ::djinterp::monad_map(test_maybe<int>::none(),
                                           plain_double());

            record_assertion(
                _handler,
                (!m.is_some()),
                "map(none, double) is none");
        });

    // monad_map changes the inner type
    run_unit_test(
        _handler,
        tally,
        "monad_map can change the inner type",
        [&]()
        {
            auto m = ::djinterp::monad_map(some(42), plain_to_string());

            const bool type_ok =
                std::is_same<decltype(m), test_maybe<std::string> >::value;

            record_assertion(
                _handler,
                (type_ok && m.is_some() && m.value() == "42"),
                "map(some(42), to_string) is some(\"42\") : test_maybe<string>");
        });

    // functor identity law: map(m, id) == m
    run_unit_test(
        _handler,
        tally,
        "monad_map obeys the functor identity law",
        [&]()
        {
            struct identity
            {
                int operator()(const int& _x) const { return _x; }
            };

            auto m = ::djinterp::monad_map(some(9), identity());

            record_assertion(
                _handler,
                (m == some(9)),
                "map(some(9), id) == some(9)");
        });

    // functor composition law: map(map(m, f), g) == map(m, g . f)
    run_unit_test(
        _handler,
        tally,
        "monad_map obeys the functor composition law",
        [&]()
        {
            struct add_one
            {
                int operator()(const int& _x) const { return _x + 1; }
            };
            struct times_three
            {
                int operator()(const int& _x) const { return _x * 3; }
            };
            struct composed
            {
                int operator()(const int& _x) const { return (_x + 1) * 3; }
            };

            auto lhs = ::djinterp::monad_map(
                ::djinterp::monad_map(some(4), add_one()), times_three());
            auto rhs = ::djinterp::monad_map(some(4), composed());

            record_assertion(
                _handler,
                (lhs == rhs && lhs.value() == 15),
                "map(map(m, f), g) == map(m, g.f)");
        });

    // monad_join: some(some(x))
    run_unit_test(
        _handler,
        tally,
        "monad_join flattens a nested some",
        [&]()
        {
            test_maybe<test_maybe<int> > nested =
                some(test_maybe<int>(9));
            auto m = ::djinterp::monad_join(nested);

            record_assertion(
                _handler,
                (m.is_some() && m.value() == 9),
                "join(some(some(9))) is some(9)");
        });

    // monad_join: outer none
    run_unit_test(
        _handler,
        tally,
        "monad_join keeps an outer none empty",
        [&]()
        {
            test_maybe<test_maybe<int> > outer_none =
                test_maybe<test_maybe<int> >::none();
            auto m = ::djinterp::monad_join(outer_none);

            record_assertion(
                _handler,
                (!m.is_some()),
                "join(none) is none");
        });

    // monad_join: inner none
    run_unit_test(
        _handler,
        tally,
        "monad_join propagates an inner none",
        [&]()
        {
            test_maybe<test_maybe<int> > some_none =
                some(test_maybe<int>::none());
            auto m = ::djinterp::monad_join(some_none);

            record_assertion(
                _handler,
                (!m.is_some()),
                "join(some(none)) is none");
        });

    // left identity law: bind(unit(a), f) == f(a)
    run_unit_test(
        _handler,
        tally,
        "monad obeys the left identity law",
        [&]()
        {
            auto lhs = ::djinterp::monad_bind(
                ::djinterp::monad_unit<test_maybe<int> >(5), arrow_inc());
            auto rhs = arrow_inc()(5);

            record_assertion(
                _handler,
                (lhs == rhs),
                "bind(unit(5), inc) == inc(5)");
        });

    // right identity law: bind(m, unit) == m
    run_unit_test(
        _handler,
        tally,
        "monad obeys the right identity law",
        [&]()
        {
            struct unit_arrow
            {
                test_maybe<int> operator()(const int& _x) const
                {
                    return ::djinterp::monad_unit<test_maybe<int> >(_x);
                }
            };

            auto m   = some(5);
            auto lhs = ::djinterp::monad_bind(m, unit_arrow());

            record_assertion(
                _handler,
                (lhs == m),
                "bind(some(5), unit) == some(5)");
        });

    return;
}


NS_END  // testing
NS_END  // djinterp
