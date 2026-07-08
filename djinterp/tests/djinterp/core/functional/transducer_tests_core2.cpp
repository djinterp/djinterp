#include "transducer_tests.hpp"

// std
#include <vector>


NS_DJINTERP
NS_TESTING


#if DJINTERP_TEST_TRANSDUCER_ENABLED

/*
test_core2_take_while
  Verifies take_while forwards values while the predicate holds, then
  short-circuits at the first failure.
  Tests the following:
  - take_while(under(4)) over {1,2,3,4,5,1} yields {1,2,3} -- stops at 4 and
    does NOT resume for the trailing 1
*/
bool
test_core2_take_while(
)
{
    std::vector<int> in = { 1, 2, 3, 4, 5, 1 };
    auto xform = transducers::take_while(under(4));
    std::vector<int> out = transduce_into_vector<int>(xform, in);

    D_INTERNAL_TRD_CHECK(out.size() == 3);
    D_INTERNAL_TRD_CHECK(out[0] == 1);
    D_INTERNAL_TRD_CHECK(out[1] == 2);
    D_INTERNAL_TRD_CHECK(out[2] == 3);

    return true;
}


/*
test_core2_take_while_none
  Verifies take_while yields nothing when the first value already fails.
  Tests the following:
  - take_while(under(0)) over {1,2,3} yields an empty result
*/
bool
test_core2_take_while_none(
)
{
    std::vector<int> in = { 1, 2, 3 };
    auto xform = transducers::take_while(under(0));
    std::vector<int> out = transduce_into_vector<int>(xform, in);

    D_INTERNAL_TRD_CHECK(out.empty());

    return true;
}


/*
test_core2_drop_while
  Verifies drop_while drops the initial run satisfying the predicate, then
  forwards every subsequent value without re-testing.
  Tests the following:
  - drop_while(under(3)) over {1,2,3,1,4} yields {3,1,4} -- once 3 fails the
    predicate, the later 1 is still forwarded
*/
bool
test_core2_drop_while(
)
{
    std::vector<int> in = { 1, 2, 3, 1, 4 };
    auto xform = transducers::drop_while(under(3));
    std::vector<int> out = transduce_into_vector<int>(xform, in);

    D_INTERNAL_TRD_CHECK(out.size() == 3);
    D_INTERNAL_TRD_CHECK(out[0] == 3);
    D_INTERNAL_TRD_CHECK(out[1] == 1);
    D_INTERNAL_TRD_CHECK(out[2] == 4);

    return true;
}


/*
test_core2_drop_while_all
  Verifies drop_while drops everything when the predicate never fails.
  Tests the following:
  - drop_while(under(100)) over {1,2,3} yields an empty result
*/
bool
test_core2_drop_while_all(
)
{
    std::vector<int> in = { 1, 2, 3 };
    auto xform = transducers::drop_while(under(100));
    std::vector<int> out = transduce_into_vector<int>(xform, in);

    D_INTERNAL_TRD_CHECK(out.empty());

    return true;
}


/*
test_core2_distinct
  Verifies distinct<int>() forwards each value only the first time it is seen.
  Tests the following:
  - distinct over {1,1,2,3,2,1} yields {1,2,3} in first-seen order
*/
bool
test_core2_distinct(
)
{
    std::vector<int> in = { 1, 1, 2, 3, 2, 1 };
    auto xform = transducers::distinct<int>();
    std::vector<int> out = transduce_into_vector<int>(xform, in);

    D_INTERNAL_TRD_CHECK(out.size() == 3);
    D_INTERNAL_TRD_CHECK(out[0] == 1);
    D_INTERNAL_TRD_CHECK(out[1] == 2);
    D_INTERNAL_TRD_CHECK(out[2] == 3);

    return true;
}


/*
test_core2_tap
  Verifies tap forwards every value unchanged while invoking the side effect
  once per value.
  Tests the following:
  - tap(counter) over {5,6,7} forwards {5,6,7} unchanged
  - the side effect ran exactly three times
*/
bool
test_core2_tap(
)
{
    int hits = 0;
    std::vector<int> in = { 5, 6, 7 };
    auto xform = transducers::tap(counter(&hits));
    std::vector<int> out = transduce_into_vector<int>(xform, in);

    D_INTERNAL_TRD_CHECK(out.size() == 3);
    D_INTERNAL_TRD_CHECK(out[0] == 5);
    D_INTERNAL_TRD_CHECK(out[2] == 7);
    D_INTERNAL_TRD_CHECK(hits == 3);

    return true;
}


/*
test_core2_flat_map
  Verifies flat_map expands each value into a sequence and forwards each
  element individually.
  Tests the following:
  - flat_map(expand) over {1,2} yields {1,1,2,2}
*/
bool
test_core2_flat_map(
)
{
    std::vector<int> in = { 1, 2 };
    auto xform = transducers::flat_map(expand());
    std::vector<int> out = transduce_into_vector<int>(xform, in);

    D_INTERNAL_TRD_CHECK(out.size() == 4);
    D_INTERNAL_TRD_CHECK(out[0] == 1);
    D_INTERNAL_TRD_CHECK(out[1] == 1);
    D_INTERNAL_TRD_CHECK(out[2] == 2);
    D_INTERNAL_TRD_CHECK(out[3] == 2);

    return true;
}


/*
test_core2_flat_map_short_circuit
  Verifies flat_map honours mid-expansion termination: when composed before a
  take, it stops emitting expanded elements once the downstream signals done.
  Tests the following:
  - flat_map(expand) | take(3) over {1,2,3}: expansion is {1,1,2,2,3,3};
    taking 3 yields {1,1,2}
*/
bool
test_core2_flat_map_short_circuit(
)
{
    std::vector<int> in = { 1, 2, 3 };
    auto xform = transducers::flat_map(expand()) | transducers::take(3);
    std::vector<int> out = transduce_into_vector<int>(xform, in);

    D_INTERNAL_TRD_CHECK(out.size() == 3);
    D_INTERNAL_TRD_CHECK(out[0] == 1);
    D_INTERNAL_TRD_CHECK(out[1] == 1);
    D_INTERNAL_TRD_CHECK(out[2] == 2);

    return true;
}


/*
run_core2_tests
  Aggregates the take_while / drop_while / distinct / tap / flat_map tests.
*/
bool
run_core2_tests(
)
{
    return ( test_core2_take_while()            &&
             test_core2_take_while_none()       &&
             test_core2_drop_while()            &&
             test_core2_drop_while_all()        &&
             test_core2_distinct()              &&
             test_core2_tap()                   &&
             test_core2_flat_map()              &&
             test_core2_flat_map_short_circuit() );
}

#else  // !DJINTERP_TEST_TRANSDUCER_ENABLED

bool
run_core2_tests(
)
{
    return true;
}

#endif  // DJINTERP_TEST_TRANSDUCER_ENABLED


NS_END  // testing
NS_END  // djinterp
