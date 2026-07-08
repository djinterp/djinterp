#include "filter_tests.hpp"

// std
#include <list>
#include <vector>


NS_DJINTERP
NS_TESTING


/*
test_builder_take_skip
  Verifies the take/skip family on the fluent builder.
  Tests the following:
  - take_first / take_last keep the leading / trailing run
  - skip_first / skip_last drop the leading / trailing run
*/
bool
test_builder_take_skip(
)
{
    std::vector<int> in = { 1, 2, 3, 4, 5 };

    D_INTERNAL_FLT_CHECK(
        filter_builder<int>::build().take_first(2).apply(in).count() == 2);
    D_INTERNAL_FLT_CHECK(
        filter_builder<int>::build().take_first(2).apply(in).elements()[1]
        == 2);

    D_INTERNAL_FLT_CHECK(
        filter_builder<int>::build().take_last(2).apply(in).elements()[0]
        == 4);

    D_INTERNAL_FLT_CHECK(
        filter_builder<int>::build().skip_first(2).apply(in).elements()[0]
        == 3);

    D_INTERNAL_FLT_CHECK(
        filter_builder<int>::build().skip_last(2).apply(in).count() == 3);

    return true;
}


/*
test_builder_aliases
  Verifies the head/tail/init/rest convenience aliases.
  Tests the following:
  - head keeps the first element; tail the last
  - init drops the last; rest drops the first
*/
bool
test_builder_aliases(
)
{
    std::vector<int> in = { 10, 20, 30, 40 };

    filter_result<int> h = filter_builder<int>::build().head().apply(in);
    D_INTERNAL_FLT_CHECK(h.count() == 1 && h.elements()[0] == 10);

    filter_result<int> t = filter_builder<int>::build().tail().apply(in);
    D_INTERNAL_FLT_CHECK(t.count() == 1 && t.elements()[0] == 40);

    filter_result<int> i = filter_builder<int>::build().init().apply(in);
    D_INTERNAL_FLT_CHECK(i.count() == 3 && i.elements()[2] == 30);

    filter_result<int> r = filter_builder<int>::build().rest().apply(in);
    D_INTERNAL_FLT_CHECK(r.count() == 3 && r.elements()[0] == 20);

    return true;
}


/*
test_builder_nth_range_slice
  Verifies take_nth, range, and slice on the builder.
  Tests the following:
  - take_nth(2) keeps every other element
  - range(1,3) keeps the half-open span
  - slice(0,5,2) keeps stepped indices
*/
bool
test_builder_nth_range_slice(
)
{
    std::vector<int> in = { 0, 1, 2, 3, 4 };

    filter_result<int> nth =
        filter_builder<int>::build().take_nth(2).apply(in);
    D_INTERNAL_FLT_CHECK(nth.count() == 3);
    D_INTERNAL_FLT_CHECK(nth.elements()[0] == 0);
    D_INTERNAL_FLT_CHECK(nth.elements()[2] == 4);

    filter_result<int> rng =
        filter_builder<int>::build().range(1, 3).apply(in);
    D_INTERNAL_FLT_CHECK(rng.count() == 2);
    D_INTERNAL_FLT_CHECK(rng.elements()[0] == 1);
    D_INTERNAL_FLT_CHECK(rng.elements()[1] == 2);

    filter_result<int> sl =
        filter_builder<int>::build().slice(0, 5, 2).apply(in);
    D_INTERNAL_FLT_CHECK(sl.count() == 3);
    D_INTERNAL_FLT_CHECK(sl.elements()[1] == 2);

    return true;
}


/*
test_builder_where_variants
  Verifies where and where_not on the builder.
  Tests the following:
  - where(is_even) keeps evens; where_not(is_even) keeps odds
*/
bool
test_builder_where_variants(
)
{
    std::vector<int> in = { 1, 2, 3, 4, 5 };

    filter_result<int> ev =
        filter_builder<int>::build().where(is_even()).apply(in);
    D_INTERNAL_FLT_CHECK(ev.count() == 2);
    D_INTERNAL_FLT_CHECK(ev.elements()[0] == 2);
    D_INTERNAL_FLT_CHECK(ev.elements()[1] == 4);

    filter_result<int> od =
        filter_builder<int>::build().where_not(is_even()).apply(in);
    D_INTERNAL_FLT_CHECK(od.count() == 3);
    D_INTERNAL_FLT_CHECK(od.elements()[0] == 1);

    return true;
}


/*
test_builder_at_variants
  Verifies at and at_indices, including out-of-range handling.
  Tests the following:
  - at(2) selects the single element at index 2
  - at_indices keeps requested in-range indices and drops out-of-range ones
*/
bool
test_builder_at_variants(
)
{
    std::vector<int> in = { 5, 6, 7, 8 };

    filter_result<int> one = filter_builder<int>::build().at(2).apply(in);
    D_INTERNAL_FLT_CHECK(one.count() == 1);
    D_INTERNAL_FLT_CHECK(one.elements()[0] == 7);

    std::vector<std::size_t> picks = { 0, 3, 99 };  // 99 out of range
    filter_result<int> many =
        filter_builder<int>::build().at_indices(picks).apply(in);
    D_INTERNAL_FLT_CHECK(many.count() == 2);
    D_INTERNAL_FLT_CHECK(many.elements()[0] == 5);
    D_INTERNAL_FLT_CHECK(many.elements()[1] == 8);

    filter_result<int> oob = filter_builder<int>::build().at(99).apply(in);
    D_INTERNAL_FLT_CHECK(oob.count() == 0);

    return true;
}


/*
test_builder_distinct_reverse
  Verifies distinct (default and custom equality) and reverse on the builder.
  Tests the following:
  - distinct() drops repeats by operator==
  - distinct(mod3_eq) collapses values sharing a residue class (first wins)
  - reverse flips order
*/
bool
test_builder_distinct_reverse(
)
{
    std::vector<int> dup = { 1, 1, 2, 3, 2 };
    filter_result<int> d = filter_builder<int>::build().distinct().apply(dup);
    D_INTERNAL_FLT_CHECK(d.count() == 3);
    D_INTERNAL_FLT_CHECK(d.elements()[0] == 1);
    D_INTERNAL_FLT_CHECK(d.elements()[1] == 2);
    D_INTERNAL_FLT_CHECK(d.elements()[2] == 3);

    // mod3: 1, then 4 (==1 mod 3, dropped), then 2 -> {1, 2}
    std::vector<int> mod = { 1, 4, 2 };
    filter_result<int> dm =
        filter_builder<int>::build().distinct(mod3_eq()).apply(mod);
    D_INTERNAL_FLT_CHECK(dm.count() == 2);
    D_INTERNAL_FLT_CHECK(dm.elements()[0] == 1);
    D_INTERNAL_FLT_CHECK(dm.elements()[1] == 2);

    std::vector<int> seq = { 1, 2, 3 };
    filter_result<int> rv = filter_builder<int>::build().reverse().apply(seq);
    D_INTERNAL_FLT_CHECK(rv.elements()[0] == 3);
    D_INTERNAL_FLT_CHECK(rv.elements()[2] == 1);

    return true;
}


/*
test_builder_chained
  Verifies multi-stage fluent chaining preserves left-to-right semantics.
  Tests the following:
  - skip_first(1).where(is_even).take_first(1) over {1,2,3,4,6}:
    after skip -> {2,3,4,6}; evens -> {2,4,6}; take 1 -> {2}
*/
bool
test_builder_chained(
)
{
    std::vector<int> in = { 1, 2, 3, 4, 6 };

    filter_result<int> r = filter_builder<int>::build()
                               .skip_first(1)
                               .where(is_even())
                               .take_first(1)
                               .apply(in);

    D_INTERNAL_FLT_CHECK(r.count() == 1);
    D_INTERNAL_FLT_CHECK(r.elements()[0] == 2);

    return true;
}


/*
test_builder_apply_container
  Verifies the container-accepting apply overload converts a non-vector
  range into the working vector first.
  Tests the following:
  - applying a builder to a std::list yields the same result as on a vector
*/
bool
test_builder_apply_container(
)
{
    std::list<int> lst;
    lst.push_back(1);
    lst.push_back(2);
    lst.push_back(3);
    lst.push_back(4);

    filter_result<int> r =
        filter_builder<int>::build().where(is_even()).apply(lst);

    D_INTERNAL_FLT_CHECK(r.count() == 2);
    D_INTERNAL_FLT_CHECK(r.elements()[0] == 2);
    D_INTERNAL_FLT_CHECK(r.elements()[1] == 4);

    return true;
}


/*
test_builder_match_queries
  Verifies the any/all/none/count match query methods.
  Tests the following:
  - any_match is true when at least one passes; none_match its complement
  - all_match is true only when every element passes
  - count_matches reports the surviving count
*/
bool
test_builder_match_queries(
)
{
    std::vector<int> mixed   = { 1, 2, 3, 4 };
    std::vector<int> alleven = { 2, 4, 6 };

    D_INTERNAL_FLT_CHECK(
        filter_builder<int>::build().where(is_even()).any_match(mixed));
    D_INTERNAL_FLT_CHECK(
        !filter_builder<int>::build().where(is_even()).none_match(mixed));
    D_INTERNAL_FLT_CHECK(
        !filter_builder<int>::build().where(is_even()).all_match(mixed));

    D_INTERNAL_FLT_CHECK(
        filter_builder<int>::build().where(is_even()).all_match(alleven));
    D_INTERNAL_FLT_CHECK(
        filter_builder<int>::build().where(is_even()).count_matches(mixed)
        == 2);

    D_INTERNAL_FLT_CHECK(
        filter_builder<int>::build().where(gt3()).none_match(
            std::vector<int>{ 1, 2, 3 }));

    return true;
}


/*
test_builder_build_chain
  Verifies both build_chain overloads (lvalue const& and rvalue &&) extract a
  usable chain.
  Tests the following:
  - an lvalue builder's build_chain() const& yields a chain that applies
    correctly
  - an rvalue builder's build_chain() && yields an equivalent chain
*/
bool
test_builder_build_chain(
)
{
    std::vector<int> in = { 1, 2, 3, 4 };

    filter_builder<int> b = filter_builder<int>::build().where(is_even());
    filter_chain<int>   c_lvalue = b.build_chain();          // const& overload
    D_INTERNAL_FLT_CHECK(c_lvalue.apply(in).count() == 2);

    filter_chain<int> c_rvalue =
        filter_builder<int>::build().where(is_even()).build_chain();  // &&
    D_INTERNAL_FLT_CHECK(c_rvalue.apply(in).count() == 2);

    return true;
}


/*
test_builder_empty_input
  Verifies builder operations are well-behaved on an empty input.
  Tests the following:
  - a where filter over an empty vector yields an empty result
  - all_match over an empty input is true (0 == 0), none_match is true
*/
bool
test_builder_empty_input(
)
{
    std::vector<int> empty;

    filter_result<int> r =
        filter_builder<int>::build().where(is_even()).apply(empty);
    D_INTERNAL_FLT_CHECK(r.count() == 0);
    D_INTERNAL_FLT_CHECK(r.empty());

    D_INTERNAL_FLT_CHECK(
        filter_builder<int>::build().where(is_even()).all_match(empty));
    D_INTERNAL_FLT_CHECK(
        filter_builder<int>::build().where(is_even()).none_match(empty));

    return true;
}


/*
run_builder_tests
  Aggregates every builder test.
  Tests the following:
  - all take/skip/alias/nth-range-slice/where/at/distinct-reverse/chained/
    container-apply/match-query/build-chain/empty-input tests pass
*/
bool
run_builder_tests(
)
{
    return ( test_builder_take_skip()         &&
             test_builder_aliases()            &&
             test_builder_nth_range_slice()    &&
             test_builder_where_variants()     &&
             test_builder_at_variants()        &&
             test_builder_distinct_reverse()   &&
             test_builder_chained()            &&
             test_builder_apply_container()    &&
             test_builder_match_queries()      &&
             test_builder_build_chain()        &&
             test_builder_empty_input() );
}


NS_END  // testing
NS_END  // djinterp
