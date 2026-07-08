#include "transducer_tests.hpp"

// std
#include <vector>


NS_DJINTERP
NS_TESTING


#if DJINTERP_TEST_TRANSDUCER_ENABLED

/*
test_core_map
  Verifies map transforms each value before forwarding.
  Tests the following:
  - map(square) over {1,2,3} into a vector yields {1,4,9}
*/
bool
test_core_map(
)
{
    std::vector<int> in = { 1, 2, 3 };
    auto xform = transducers::map(square());
    std::vector<int> out = transduce_into_vector<int>(xform, in);

    D_INTERNAL_TRD_CHECK(out.size() == 3);
    D_INTERNAL_TRD_CHECK(out[0] == 1);
    D_INTERNAL_TRD_CHECK(out[1] == 4);
    D_INTERNAL_TRD_CHECK(out[2] == 9);

    return true;
}


/*
test_core_filter
  Verifies filter forwards only values satisfying the predicate.
  Tests the following:
  - filter(is_even) over {1,2,3,4,5} yields {2,4}
*/
bool
test_core_filter(
)
{
    std::vector<int> in = { 1, 2, 3, 4, 5 };
    auto xform = transducers::filter(is_even());
    std::vector<int> out = transduce_into_vector<int>(xform, in);

    D_INTERNAL_TRD_CHECK(out.size() == 2);
    D_INTERNAL_TRD_CHECK(out[0] == 2);
    D_INTERNAL_TRD_CHECK(out[1] == 4);

    return true;
}


/*
test_core_filter_not
  Verifies filter_not forwards only values FAILING the predicate.
  Tests the following:
  - filter_not(is_even) over {1,2,3,4,5} yields the odds {1,3,5}
*/
bool
test_core_filter_not(
)
{
    std::vector<int> in = { 1, 2, 3, 4, 5 };
    auto xform = transducers::filter_not(is_even());
    std::vector<int> out = transduce_into_vector<int>(xform, in);

    D_INTERNAL_TRD_CHECK(out.size() == 3);
    D_INTERNAL_TRD_CHECK(out[0] == 1);
    D_INTERNAL_TRD_CHECK(out[1] == 3);
    D_INTERNAL_TRD_CHECK(out[2] == 5);

    return true;
}


/*
test_core_take
  Verifies take forwards at most n values then short-circuits.
  Tests the following:
  - take(2) over {10,20,30,40} yields {10,20}
*/
bool
test_core_take(
)
{
    std::vector<int> in = { 10, 20, 30, 40 };
    auto xform = transducers::take(2);
    std::vector<int> out = transduce_into_vector<int>(xform, in);

    D_INTERNAL_TRD_CHECK(out.size() == 2);
    D_INTERNAL_TRD_CHECK(out[0] == 10);
    D_INTERNAL_TRD_CHECK(out[1] == 20);

    return true;
}


/*
test_core_take_zero
  Verifies take(0) forwards nothing and immediately signals done.
  Tests the following:
  - take(0) over a non-empty input yields an empty result
*/
bool
test_core_take_zero(
)
{
    std::vector<int> in = { 1, 2, 3 };
    auto xform = transducers::take(0);
    std::vector<int> out = transduce_into_vector<int>(xform, in);

    D_INTERNAL_TRD_CHECK(out.empty());

    return true;
}


/*
test_core_take_more_than_size
  Verifies take(n) with n past the input size forwards everything without
  error.
  Tests the following:
  - take(100) over {1,2,3} yields {1,2,3}
*/
bool
test_core_take_more_than_size(
)
{
    std::vector<int> in = { 1, 2, 3 };
    auto xform = transducers::take(100);
    std::vector<int> out = transduce_into_vector<int>(xform, in);

    D_INTERNAL_TRD_CHECK(out.size() == 3);
    D_INTERNAL_TRD_CHECK(out[2] == 3);

    return true;
}


/*
test_core_drop
  Verifies drop silently skips the first n values and forwards the rest.
  Tests the following:
  - drop(2) over {1,2,3,4,5} yields {3,4,5}
*/
bool
test_core_drop(
)
{
    std::vector<int> in = { 1, 2, 3, 4, 5 };
    auto xform = transducers::drop(2);
    std::vector<int> out = transduce_into_vector<int>(xform, in);

    D_INTERNAL_TRD_CHECK(out.size() == 3);
    D_INTERNAL_TRD_CHECK(out[0] == 3);
    D_INTERNAL_TRD_CHECK(out[2] == 5);

    return true;
}


/*
test_core_drop_all
  Verifies drop(n) with n >= size forwards nothing.
  Tests the following:
  - drop(100) over {1,2,3} yields an empty result
*/
bool
test_core_drop_all(
)
{
    std::vector<int> in = { 1, 2, 3 };
    auto xform = transducers::drop(100);
    std::vector<int> out = transduce_into_vector<int>(xform, in);

    D_INTERNAL_TRD_CHECK(out.empty());

    return true;
}


/*
run_core_tests
  Aggregates the map / filter / filter_not / take / drop tests.
*/
bool
run_core_tests(
)
{
    return ( test_core_map()                 &&
             test_core_filter()              &&
             test_core_filter_not()          &&
             test_core_take()                &&
             test_core_take_zero()           &&
             test_core_take_more_than_size() &&
             test_core_drop()                &&
             test_core_drop_all() );
}

#else  // !DJINTERP_TEST_TRANSDUCER_ENABLED

bool
run_core_tests(
)
{
    return true;
}

#endif  // DJINTERP_TEST_TRANSDUCER_ENABLED


NS_END  // testing
NS_END  // djinterp
