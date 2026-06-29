/******************************************************************************
* djinterp [test]                            event_common_tests_index_apply.cpp
*
*   Section I -- INDEX SEQUENCE POLYFILL + tuple-apply utilities.  Covers the
* internal compile-time integer sequence (index_sequence / make_index_sequence)
* and the runtime tuple-apply helpers (apply_impl / apply_tuple) that stand in
* for std::apply on C++11/14.  Sequence width is measured with the seq_size
* helper from the test header; apply is exercised across arities 0..3 with both
* the deducing wrapper and a directly supplied index sequence, and the computed
* values are checked.
*
*   The callables here are explicitly typed (no generic lambdas) so the suite
* stays C++11-clean.
*
*
* path:      /tests/djinterp/core/event/event_common/event_common_tests_index_apply.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

// djinterp
#include "event_common_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_index_sequence
bool
tests_index_sequence()
{
    bool ok = true;

    // a literal index_sequence carries exactly the indices written into it.
    ok = D_EC_CHECK(
        (seq_size< internal::index_sequence<> >::value == 0u)
    ) && ok;
    ok = D_EC_CHECK(
        (seq_size< internal::index_sequence<0> >::value == 1u)
    ) && ok;
    ok = D_EC_CHECK(
        (seq_size< internal::index_sequence<9, 8, 7> >::value == 3u)
    ) && ok;

    return ok;
}


// tests_make_index_sequence
bool
tests_make_index_sequence()
{
    bool ok = true;

    // make_index_sequence<N> builds a sequence of exactly N indices.
    ok = D_EC_CHECK(
        (seq_size< internal::make_index_sequence<0> >::value == 0u)
    ) && ok;
    ok = D_EC_CHECK(
        (seq_size< internal::make_index_sequence<1> >::value == 1u)
    ) && ok;
    ok = D_EC_CHECK(
        (seq_size< internal::make_index_sequence<5> >::value == 5u)
    ) && ok;

    return ok;
}


// tests_apply_impl_direct
bool
tests_apply_impl_direct()
{
    bool ok = true;

    // apply_impl invokes the callable with the tuple elements selected by an
    // explicitly supplied index sequence.
    std::tuple<int, int, int> t3(1, 2, 3);

    struct sum3
    {
        int operator()(int _a, int _b, int _c) const
        {
            return (_a + _b + _c);
        }
    };

    const int r = internal::apply_impl(
        sum3(), t3, internal::make_index_sequence<3>{});

    ok = D_EC_CHECK(r == 6) && ok;

    return ok;
}


// tests_apply_tuple_arities
bool
tests_apply_tuple_arities()
{
    bool ok = true;

    // apply_tuple deduces the index sequence from the tuple size, so it must
    // work for an empty tuple and for non-empty tuples alike.  (apply_tuple
    // binds the tuple by non-const lvalue reference, hence the named tuples.)
    std::tuple<>              t0;
    std::tuple<int>           t1(7);
    std::tuple<int, int>      t2(3, 4);
    std::tuple<int, int, int> t3(1, 2, 3);

    struct k42
    {
        int operator()() const { return 42; }
    };
    struct inc
    {
        int operator()(int _a) const { return (_a + 1); }
    };
    struct add2
    {
        int operator()(int _a, int _b) const { return (_a + _b); }
    };
    struct add3
    {
        int operator()(int _a, int _b, int _c) const
        {
            return (_a + _b + _c);
        }
    };

    ok = D_EC_CHECK(internal::apply_tuple(k42(),  t0) == 42) && ok;
    ok = D_EC_CHECK(internal::apply_tuple(inc(),  t1) == 8)  && ok;
    ok = D_EC_CHECK(internal::apply_tuple(add2(), t2) == 7)  && ok;
    ok = D_EC_CHECK(internal::apply_tuple(add3(), t3) == 6)  && ok;

    return ok;
}


// tests_apply_tuple_values
bool
tests_apply_tuple_values()
{
    bool ok = true;

    // the elements must reach the callable in order and unmodified: a
    // difference is order-sensitive and so pins down the argument routing.
    std::tuple<int, int, int> t(10, 3, 1);

    struct diff
    {
        int operator()(int _a, int _b, int _c) const
        {
            return ((_a - _b) - _c);
        }
    };

    ok = D_EC_CHECK(internal::apply_tuple(diff(), t) == 6) && ok;

    // a heterogeneous payload is forwarded with its element types intact.
    std::tuple<int, double> td(2, 0.5);

    struct scale
    {
        double operator()(int _a, double _b) const
        {
            return (_a * _b);
        }
    };

    ok = D_EC_CHECK(internal::apply_tuple(scale(), td) == 1.0) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
