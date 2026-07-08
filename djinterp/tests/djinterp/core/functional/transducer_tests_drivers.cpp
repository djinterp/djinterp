#include "transducer_tests.hpp"

// std
#include <vector>


NS_DJINTERP
NS_TESTING


#if DJINTERP_TEST_TRANSDUCER_ENABLED

/*
test_drivers_transduce
  Verifies the general transduce driver: applies a transducer to an explicit
  reducer and folds a container from an explicit init.
  Tests the following:
  - transduce(filter(is_even), sum-reducer, 0, {1,2,3,4}) accumulates 2+4 = 6
*/
bool
test_drivers_transduce(
)
{
    auto sum_reducer = [](reducing_state<int>& _st, const int& _v)
    {
        _st.accumulator() += _v;
    };

    std::vector<int> in = { 1, 2, 3, 4 };
    int result = transduce(transducers::filter(is_even()),
                           sum_reducer, 0, in);

    D_INTERNAL_TRD_CHECK(result == 6);

    return true;
}


/*
test_drivers_into_vector
  Verifies transduce_into_vector collects survivors into a vector.
  Tests the following:
  - map(square) | filter(is_even) over {1,2,3,4} -> squares {1,4,9,16},
    evens {4,16}
*/
bool
test_drivers_into_vector(
)
{
    std::vector<int> in = { 1, 2, 3, 4 };
    auto xform = transducers::map(square()) | transducers::filter(is_even());
    std::vector<int> out = transduce_into_vector<int>(xform, in);

    D_INTERNAL_TRD_CHECK(out.size() == 2);
    D_INTERNAL_TRD_CHECK(out[0] == 4);
    D_INTERNAL_TRD_CHECK(out[1] == 16);

    return true;
}


/*
test_drivers_into_vector_empty
  Verifies transduce_into_vector over an empty source yields an empty vector.
  Tests the following:
  - any transducer over {} yields {}
*/
bool
test_drivers_into_vector_empty(
)
{
    std::vector<int> in;
    auto xform = transducers::map(square());
    std::vector<int> out = transduce_into_vector<int>(xform, in);

    D_INTERNAL_TRD_CHECK(out.empty());

    return true;
}


/*
test_drivers_into_accumulator
  Verifies transduce_into_accumulator adapts an accumulator-protocol sink
  (input_type / step / finalize) to the transducer protocol.
  Tests the following:
  - filter(is_even) over {1,2,3,4,5,6} into a summing accumulator finalizes
    2+4+6 = 12
*/
bool
test_drivers_into_accumulator(
)
{
    std::vector<int> in = { 1, 2, 3, 4, 5, 6 };
    int total = transduce_into_accumulator(
        transducers::filter(is_even()), int_sum_acc(), in);

    D_INTERNAL_TRD_CHECK(total == 12);

    return true;
}


/*
test_drivers_producer_to_consumer
  Verifies transduce_producer_to_consumer pulls from a producer, runs values
  through the transducer, and pushes survivors to a consumer.
  Tests the following:
  - a range_producer(5) yields 0..4; map(plus_one) shifts to 1..5; the
    consumer collects {1,2,3,4,5}
*/
bool
test_drivers_producer_to_consumer(
)
{
    range_producer prod(5);
    std::vector<int> collected;
    auto consumer = [&collected](const int& _v)
    {
        collected.push_back(_v);
    };

    auto xform = transducers::map(plus_one());
    transduce_producer_to_consumer(xform, prod, consumer);

    D_INTERNAL_TRD_CHECK(collected.size() == 5);
    D_INTERNAL_TRD_CHECK(collected[0] == 1);
    D_INTERNAL_TRD_CHECK(collected[4] == 5);

    return true;
}


/*
test_drivers_short_circuit_stops_source
  Verifies a short-circuiting transducer (take) stops the driver from pulling
  the whole producer: the producer is not exhausted past what take needs.
  Tests the following:
  - range_producer(100) through take(3): consumer receives exactly {0,1,2}
  - the producer's read position advances only as far as needed to satisfy
    take plus the one final pull that observes done (i.e. far short of 100)
*/
bool
test_drivers_short_circuit_stops_source(
)
{
    range_producer prod(100);
    std::vector<int> collected;
    auto consumer = [&collected](const int& _v)
    {
        collected.push_back(_v);
    };

    auto xform = transducers::take(3);
    transduce_producer_to_consumer(xform, prod, consumer);

    D_INTERNAL_TRD_CHECK(collected.size() == 3);
    D_INTERNAL_TRD_CHECK(collected[0] == 0);
    D_INTERNAL_TRD_CHECK(collected[1] == 1);
    D_INTERNAL_TRD_CHECK(collected[2] == 2);

    // take(3) marks done on the third forward, so the driver stops before
    // draining the producer: far fewer than 100 values were pulled.
    D_INTERNAL_TRD_CHECK(prod.pos < 100);

    return true;
}


/*
run_drivers_tests
  Aggregates the transduce / into_vector / into_accumulator /
  producer_to_consumer / short-circuit tests.
*/
bool
run_drivers_tests(
)
{
    return ( test_drivers_transduce()                  &&
             test_drivers_into_vector()                &&
             test_drivers_into_vector_empty()          &&
             test_drivers_into_accumulator()           &&
             test_drivers_producer_to_consumer()       &&
             test_drivers_short_circuit_stops_source() );
}

#else  // !DJINTERP_TEST_TRANSDUCER_ENABLED

bool
run_drivers_tests(
)
{
    return true;
}

#endif  // DJINTERP_TEST_TRANSDUCER_ENABLED


NS_END  // testing
NS_END  // djinterp
