#include "extractor_tests.hpp"

// std
#include <tuple>


NS_DJINTERP
NS_TESTING


/*
test_combinators_then_extract
  Verifies then_extract composes two extractors: outer applied to inner's
  result.
  Tests the following:
  - then_extract(get_age, times_two) yields 2 * age
*/
bool
test_combinators_then_extract(
)
{
    person p = { 1, 21, 10 };
    D_INTERNAL_EXT_CHECK(
        extractors::then_extract(get_age(), times_two())(p) == 42);

    return true;
}


/*
test_combinators_fanout2
  Verifies binary fanout applies two extractors to one source and tuples the
  results.
  Tests the following:
  - fanout(get_age, get_dept) yields (age, dept) in order
*/
bool
test_combinators_fanout2(
)
{
    person p = { 1, 30, 10 };
    std::tuple<int, int> t = extractors::fanout(get_age(), get_dept())(p);

    D_INTERNAL_EXT_CHECK(std::get<0>(t) == 30);
    D_INTERNAL_EXT_CHECK(std::get<1>(t) == 10);

    return true;
}


/*
test_combinators_fanout3
  Verifies ternary fanout tuples three extracted features.
  Tests the following:
  - fanout(get_id, get_age, get_dept) yields (id, age, dept) in order
*/
bool
test_combinators_fanout3(
)
{
    person p = { 7, 30, 10 };
    std::tuple<int, int, int> t =
        extractors::fanout(get_id(), get_age(), get_dept())(p);

    D_INTERNAL_EXT_CHECK(std::get<0>(t) == 7);
    D_INTERNAL_EXT_CHECK(std::get<1>(t) == 30);
    D_INTERNAL_EXT_CHECK(std::get<2>(t) == 10);

    return true;
}


/*
test_combinators_mapped
  Verifies mapped post-transforms an extractor's output. Operationally the
  same as then_extract; the test confirms the mapped factory wiring.
  Tests the following:
  - mapped(get_age, times_two) yields 2 * age
*/
bool
test_combinators_mapped(
)
{
    person p = { 1, 16, 10 };
    D_INTERNAL_EXT_CHECK(extractors::mapped(get_age(), times_two())(p) == 32);

    return true;
}


/*
test_combinators_filtered_pass
  Verifies filtered gates on the EXTRACTED value and yields just(value) when
  the predicate passes.
  Tests the following:
  - filtered(get_age, is_adult) on an adult yields a present maybe holding
    the age
*/
bool
test_combinators_filtered_pass(
)
{
    person adult = { 1, 30, 10 };
    maybe<int> m = extractors::filtered(get_age(), is_adult())(adult);

    D_INTERNAL_EXT_CHECK(m.has_value());
    D_INTERNAL_EXT_CHECK(m.value() == 30);

    return true;
}


/*
test_combinators_filtered_fail
  Verifies filtered yields nothing when the value-side predicate fails.
  Tests the following:
  - filtered(get_age, is_adult) on a minor yields an empty maybe
*/
bool
test_combinators_filtered_fail(
)
{
    person minor = { 2, 15, 20 };
    maybe<int> m = extractors::filtered(get_age(), is_adult())(minor);

    D_INTERNAL_EXT_CHECK(m.is_nothing());
    D_INTERNAL_EXT_CHECK(m.value_or(-1) == -1);

    return true;
}


/*
test_combinators_guarded
  Verifies guarded gates on the SOURCE before extraction: just(extract) when
  the guard passes, nothing otherwise.
  Tests the following:
  - guarded(get_age, dept_nonzero) yields the age when dept != 0
  - it yields nothing when dept == 0 (guard fails on the source)
*/
bool
test_combinators_guarded(
)
{
    person ok  = { 1, 30, 10 };   // dept != 0 -> guard passes
    person bad = { 2, 40,  0 };   // dept == 0 -> guard fails

    maybe<int> m_ok = extractors::guarded(get_age(), dept_nonzero())(ok);
    D_INTERNAL_EXT_CHECK(m_ok.has_value());
    D_INTERNAL_EXT_CHECK(m_ok.value() == 30);

    maybe<int> m_bad = extractors::guarded(get_age(), dept_nonzero())(bad);
    D_INTERNAL_EXT_CHECK(m_bad.is_nothing());

    return true;
}


/*
test_combinators_defaulted_present
  Verifies defaulted passes the inner maybe's value through when present.
  Tests the following:
  - defaulted(filtered(get_age, is_adult), -1) on an adult yields the age
*/
bool
test_combinators_defaulted_present(
)
{
    person adult = { 1, 30, 10 };
    int v = extractors::defaulted(
                extractors::filtered(get_age(), is_adult()), -1)(adult);

    D_INTERNAL_EXT_CHECK(v == 30);

    return true;
}


/*
test_combinators_defaulted_absent
  Verifies defaulted substitutes the stored default when the inner maybe is
  nothing, lifting a partial extractor back to a total one.
  Tests the following:
  - defaulted(filtered(get_age, is_adult), -1) on a minor yields -1
*/
bool
test_combinators_defaulted_absent(
)
{
    person minor = { 2, 15, 20 };
    int v = extractors::defaulted(
                extractors::filtered(get_age(), is_adult()), -1)(minor);

    D_INTERNAL_EXT_CHECK(v == -1);

    return true;
}


/*
test_combinators_try_extract_success
  Verifies try_extract wraps a non-throwing extraction as just(value).
  Tests the following:
  - try_extract(checked_age) on a valid person yields the age
*/
bool
test_combinators_try_extract_success(
)
{
    person p = { 1, 30, 10 };
    maybe<int> m = extractors::try_extract(checked_age())(p);

    D_INTERNAL_EXT_CHECK(m.has_value());
    D_INTERNAL_EXT_CHECK(m.value() == 30);

    return true;
}


/*
test_combinators_try_extract_throws
  Verifies try_extract captures an exception as nothing.
  Tests the following:
  - try_extract(checked_age) on a person that triggers a throw yields an
    empty maybe rather than propagating the exception
*/
bool
test_combinators_try_extract_throws(
)
{
    person bad = { 1, -5, 10 };   // negative age -> checked_age throws
    maybe<int> m = extractors::try_extract(checked_age())(bad);

    D_INTERNAL_EXT_CHECK(m.is_nothing());

    return true;
}


/*
run_combinators_tests
  Aggregates every combinator-section test.
  Tests the following:
  - all then_extract / fanout / mapped / filtered / guarded / defaulted /
    try_extract tests pass
*/
bool
run_combinators_tests(
)
{
    return ( test_combinators_then_extract()         &&
             test_combinators_fanout2()              &&
             test_combinators_fanout3()              &&
             test_combinators_mapped()               &&
             test_combinators_filtered_pass()        &&
             test_combinators_filtered_fail()        &&
             test_combinators_guarded()              &&
             test_combinators_defaulted_present()    &&
             test_combinators_defaulted_absent()     &&
             test_combinators_try_extract_success()  &&
             test_combinators_try_extract_throws() );
}


NS_END  // testing
NS_END  // djinterp
