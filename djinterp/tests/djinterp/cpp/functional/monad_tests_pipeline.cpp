/*******************************************************************************
* djinterp [test]                                       monad_tests_pipeline.cpp
*
*   Tests for monad.hpp Sections III and IV: the pipeline combinators
* bind_with / map_with / then_with and the operator| that applies them.
* Covers each combinator's success and short-circuit behavior through
* the pipe, multi-stage chaining, type-changing pipelines, the
* equivalence of the pipe form with the direct call, and the SFINAE
* guard that keeps operator| from firing on non-monad left-hand sides.
*
*
* path:      /tests/djinterp/cpp/functional/monad_tests_pipeline.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./monad_tests.hpp"


NS_DJINTERP
NS_TESTING



namespace {

    // a type that overloads operator| itself, to confirm monad.hpp's
    // operator| does not hijack unrelated pipe expressions (its SFINAE
    // guard requires a monad LHS).
    struct DPipedFlag
    {
        int value;
    };

    inline
    DPipedFlag operator|(
        const DPipedFlag& _lhs,
        const DPipedFlag& _rhs
    )
    {
        return DPipedFlag{ _lhs.value + _rhs.value };
    }

}  // namespace


/*
monad_tests_pipeline
  Exercises monad.hpp Sections III + IV (combinators, operator|).
  Tests the following:
  - m | bind_with(arrow) threads a some value through the arrow
  - bind_with short-circuits on a none left-hand side
  - m | map_with(transform) maps the inner value
  - map_with short-circuits on a none left-hand side
  - m1 | then_with(m2) sequences and discards the first value
  - then_with short-circuits when the first monad is none
  - a multi-stage pipeline (bind | map | bind) composes correctly
  - a pipeline can change the inner type
  - the pipe form equals the equivalent direct monad_bind / monad_map
  - operator| does not interfere with an unrelated user operator|
*/
bool monad_tests_pipeline()
{
    bool verdict = true;

    DUnitTestTally tally;

    // bind_with: success
    run_unit_test(verdict,
        tally,
        "operator| with bind_with threads a some value",
        [&]()
        {
            auto m = some(5) | ::djinterp::bind_with(arrow_inc());

            record_assertion(verdict,
                (m.is_some() && m.value() == 6),
                "some(5) | bind_with(inc) is some(6)");
        });

    // bind_with: none short-circuits
    run_unit_test(verdict,
        tally,
        "operator| with bind_with short-circuits on none",
        [&]()
        {
            auto m = DTestMaybe<int>::none()
                   | ::djinterp::bind_with(arrow_inc());

            record_assertion(verdict,
                (!m.is_some()),
                "none | bind_with(inc) is none");
        });

    // map_with: success
    run_unit_test(verdict,
        tally,
        "operator| with map_with maps the inner value",
        [&]()
        {
            auto m = some(5) | ::djinterp::map_with(plain_double());

            record_assertion(verdict,
                (m.is_some() && m.value() == 10),
                "some(5) | map_with(double) is some(10)");
        });

    // map_with: none short-circuits
    run_unit_test(verdict,
        tally,
        "operator| with map_with short-circuits on none",
        [&]()
        {
            auto m = DTestMaybe<int>::none()
                   | ::djinterp::map_with(plain_double());

            record_assertion(verdict,
                (!m.is_some()),
                "none | map_with(double) is none");
        });

    // then_with: success
    run_unit_test(verdict,
        tally,
        "operator| with then_with sequences and discards the first",
        [&]()
        {
            auto m = some(1) | ::djinterp::then_with(some(42));

            record_assertion(verdict,
                (m.is_some() && m.value() == 42),
                "some(1) | then_with(some(42)) is some(42)");
        });

    // then_with: none short-circuits
    run_unit_test(verdict,
        tally,
        "operator| with then_with short-circuits on a none first",
        [&]()
        {
            auto m = DTestMaybe<int>::none()
                   | ::djinterp::then_with(some(42));

            record_assertion(verdict,
                (!m.is_some()),
                "none | then_with(some(42)) is none");
        });

    // multi-stage pipeline
    run_unit_test(verdict,
        tally,
        "a multi-stage pipeline composes left to right",
        [&]()
        {
            // some(5) -> inc -> some(6) -> *2 -> some(12) -> DTimesTen
            //                                              -> some(120)
            auto m = some(5)
                   | ::djinterp::bind_with(arrow_inc())
                   | ::djinterp::map_with(plain_double())
                   | ::djinterp::bind_with(DArrowTimesTen());

            record_assertion(verdict,
                (m.is_some() && m.value() == 120),
                "bind|map|bind pipeline yields some(120)");
        });

    // pipeline that fails mid-way
    run_unit_test(verdict,
        tally,
        "a pipeline propagates a mid-stage failure",
        [&]()
        {
            auto m = some(5)
                   | ::djinterp::bind_with(arrow_inc())
                   | ::djinterp::bind_with(DArrowToNone())
                   | ::djinterp::map_with(plain_double());

            record_assertion(verdict,
                (!m.is_some()),
                "a mid-pipeline none propagates to the result");
        });

    // type-changing pipeline
    run_unit_test(verdict,
        tally,
        "a pipeline can change the inner type",
        [&]()
        {
            auto m = some(41)
                   | ::djinterp::bind_with(arrow_inc())
                   | ::djinterp::map_with(DPlainToString());

            const bool type_ok =
                std::is_same<decltype(m), DTestMaybe<std::string> >::value;

            record_assertion(verdict,
                (type_ok && m.is_some() && m.value() == "42"),
                "bind|map pipeline yields some(\"42\") : test_maybe<string>");
        });

    // pipe form equals direct call
    run_unit_test(verdict,
        tally,
        "the pipe form equals the equivalent direct call",
        [&]()
        {
            auto piped  = some(5) | ::djinterp::bind_with(arrow_inc());
            auto direct = ::djinterp::monad_bind(some(5), arrow_inc());

            auto piped_map  = some(5) | ::djinterp::map_with(plain_double());
            auto direct_map = ::djinterp::monad_map(some(5), plain_double());

            record_assertion(verdict,
                (piped == direct && piped_map == direct_map),
                "pipe form matches monad_bind / monad_map directly");
        });

    // operator| does not hijack unrelated pipes
    run_unit_test(verdict,
        tally,
        "operator| leaves unrelated user operator| intact",
        [&]()
        {
            // DPipedFlag is not a monad; its own operator| must win.
            DPipedFlag r = DPipedFlag{ 2 } | DPipedFlag{ 3 };

            record_assertion(verdict,
                (r.value == 5),
                "user-defined operator| on a non-monad is unaffected");
        });

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
