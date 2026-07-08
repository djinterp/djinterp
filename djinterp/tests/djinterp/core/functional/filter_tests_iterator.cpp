#include "filter_tests.hpp"

// std
#include <vector>


NS_DJINTERP
NS_TESTING


/*
test_iterator_traversal
  Verifies the lazy iterator yields the filtered elements in order and tracks
  remaining count.
  Tests the following:
  - over {1,2,3,4,5} with a where(is_even) chain, the iterator yields 2 then
    4, with has_next / remaining behaving correctly at each step
*/
bool
test_iterator_traversal(
)
{
    std::vector<int> in = { 1, 2, 3, 4, 5 };
    filter_chain<int> chain =
        filter_builder<int>::build().where(is_even()).build_chain();

    filter_iterator<int> it(in, chain);

    D_INTERNAL_FLT_CHECK(it.has_next());
    D_INTERNAL_FLT_CHECK(it.remaining() == 2);
    D_INTERNAL_FLT_CHECK(it.next() == 2);
    D_INTERNAL_FLT_CHECK(it.remaining() == 1);
    D_INTERNAL_FLT_CHECK(it.has_next());
    D_INTERNAL_FLT_CHECK(it.next() == 4);
    D_INTERNAL_FLT_CHECK(!it.has_next());
    D_INTERNAL_FLT_CHECK(it.remaining() == 0);

    return true;
}


/*
test_iterator_reset
  Verifies reset returns the iterator to the start so it can be replayed.
  Tests the following:
  - after exhausting the iterator, reset restores has_next and re-yields the
    first filtered element
*/
bool
test_iterator_reset(
)
{
    std::vector<int> in = { 2, 4, 6 };
    filter_chain<int> chain =
        filter_builder<int>::build().where(is_even()).build_chain();

    filter_iterator<int> it(in, chain);

    D_INTERNAL_FLT_CHECK(it.next() == 2);
    D_INTERNAL_FLT_CHECK(it.next() == 4);
    D_INTERNAL_FLT_CHECK(it.next() == 6);
    D_INTERNAL_FLT_CHECK(!it.has_next());

    it.reset();
    D_INTERNAL_FLT_CHECK(it.has_next());
    D_INTERNAL_FLT_CHECK(it.remaining() == 3);
    D_INTERNAL_FLT_CHECK(it.next() == 2);

    return true;
}


/*
test_iterator_empty
  Verifies the iterator over a chain that selects nothing reports no next.
  Tests the following:
  - a where(gt3) chain over all-small input yields an iterator with no next
    and zero remaining
*/
bool
test_iterator_empty(
)
{
    std::vector<int> in = { 1, 2, 3 };
    filter_chain<int> chain =
        filter_builder<int>::build().where(gt3()).build_chain();

    filter_iterator<int> it(in, chain);

    D_INTERNAL_FLT_CHECK(!it.has_next());
    D_INTERNAL_FLT_CHECK(it.remaining() == 0);

    return true;
}


/*
run_iterator_tests
  Aggregates every iterator test.
  Tests the following:
  - all traversal / reset / empty tests pass
*/
bool
run_iterator_tests(
)
{
    return ( test_iterator_traversal() &&
             test_iterator_reset()      &&
             test_iterator_empty() );
}


NS_END  // testing
NS_END  // djinterp
