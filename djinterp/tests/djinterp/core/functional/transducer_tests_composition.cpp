#include "transducer_tests.hpp"

// std
#include <vector>


NS_DJINTERP
NS_TESTING


#if DJINTERP_TEST_TRANSDUCER_ENABLED

/*
test_composition_compose2
  Verifies compose(t1, t2) applies t1 first, then t2 (t1 outer, closer to the
  source; t2 closer to the sink).
  Tests the following:
  - compose(map(square), filter(is_even)) over {1,2,3} squares to {1,4,9}
    then keeps evens -> {4}
*/
bool
test_composition_compose2(
)
{
    std::vector<int> in = { 1, 2, 3 };
    auto xform = compose(transducers::map(square()),
                         transducers::filter(is_even()));
    std::vector<int> out = transduce_into_vector<int>(xform, in);

    D_INTERNAL_TRD_CHECK(out.size() == 1);
    D_INTERNAL_TRD_CHECK(out[0] == 4);

    return true;
}


/*
test_composition_compose_variadic
  Verifies the variadic compose folds left across three or more stages.
  Tests the following:
  - compose(map(plus_one), map(square), take(2)) over {1,2,3,4}:
    +1 -> {2,3,4,5}; square -> {4,9,16,25}; take 2 -> {4,9}
*/
bool
test_composition_compose_variadic(
)
{
    std::vector<int> in = { 1, 2, 3, 4 };
    auto xform = compose(transducers::map(plus_one()),
                         transducers::map(square()),
                         transducers::take(2));
    std::vector<int> out = transduce_into_vector<int>(xform, in);

    D_INTERNAL_TRD_CHECK(out.size() == 2);
    D_INTERNAL_TRD_CHECK(out[0] == 4);
    D_INTERNAL_TRD_CHECK(out[1] == 9);

    return true;
}


/*
test_composition_pipe_operator
  Verifies operator| composes two transducers equivalently to compose.
  Tests the following:
  - map(square) | filter(is_even) over {1,2,3} yields {4}
*/
bool
test_composition_pipe_operator(
)
{
    std::vector<int> in = { 1, 2, 3 };
    auto xform = transducers::map(square()) | transducers::filter(is_even());
    std::vector<int> out = transduce_into_vector<int>(xform, in);

    D_INTERNAL_TRD_CHECK(out.size() == 1);
    D_INTERNAL_TRD_CHECK(out[0] == 4);

    return true;
}


/*
test_composition_pipe_lvalue
  Verifies operator| accepts named (lvalue) transducer operands, not just
  temporaries. This guards the forwarding-constructor self-type fix that lets
  helpers be copied during composition.
  Tests the following:
  - building m and f as named locals, then m | f, composes correctly
*/
bool
test_composition_pipe_lvalue(
)
{
    std::vector<int> in = { 1, 2, 3, 4 };
    auto m = transducers::map(plus_one());
    auto f = transducers::filter(is_even());
    auto xform = m | f;
    std::vector<int> out = transduce_into_vector<int>(xform, in);

    // +1 -> {2,3,4,5}; evens -> {2,4}
    D_INTERNAL_TRD_CHECK(out.size() == 2);
    D_INTERNAL_TRD_CHECK(out[0] == 2);
    D_INTERNAL_TRD_CHECK(out[1] == 4);

    return true;
}


/*
test_composition_order
  Verifies the documented ordering: a | b means a sees values first. Using two
  maps whose composition is non-commutative pins the direction.
  Tests the following:
  - (map(plus_one) | map(square)) over {2} gives square(plus_one(2)) =
    square(3) = 9
  - (map(square) | map(plus_one)) over {2} gives plus_one(square(2)) =
    plus_one(4) = 5
*/
bool
test_composition_order(
)
{
    std::vector<int> in = { 2 };

    auto a = transducers::map(plus_one()) | transducers::map(square());
    std::vector<int> oa = transduce_into_vector<int>(a, in);
    D_INTERNAL_TRD_CHECK(oa.size() == 1);
    D_INTERNAL_TRD_CHECK(oa[0] == 9);

    auto b = transducers::map(square()) | transducers::map(plus_one());
    std::vector<int> ob = transduce_into_vector<int>(b, in);
    D_INTERNAL_TRD_CHECK(ob.size() == 1);
    D_INTERNAL_TRD_CHECK(ob[0] == 5);

    return true;
}


/*
test_composition_into_reducer
  Verifies into_reducer applies a transducer to a downstream reducer to yield
  a usable reducer, which can then be driven by hand.
  Tests the following:
  - applying map(square) to a summing reducer and folding {1,2,3} by hand
    accumulates 1+4+9 = 14
*/
bool
test_composition_into_reducer(
)
{
    auto downstream = [](reducing_state<int>& _st, const int& _v)
    {
        _st.accumulator() += _v;
    };

    auto wrapped = into_reducer(transducers::map(square()), downstream);

    reducing_state<int> st(0);
    std::vector<int> in = { 1, 2, 3 };
    for (int v : in)
    {
        wrapped(st, v);
    }

    D_INTERNAL_TRD_CHECK(st.accumulator() == 14);

    return true;
}


/*
run_composition_tests
  Aggregates the compose / operator| / ordering / into_reducer tests.
*/
bool
run_composition_tests(
)
{
    return ( test_composition_compose2()         &&
             test_composition_compose_variadic()  &&
             test_composition_pipe_operator()     &&
             test_composition_pipe_lvalue()       &&
             test_composition_order()             &&
             test_composition_into_reducer() );
}

#else  // !DJINTERP_TEST_TRANSDUCER_ENABLED

bool
run_composition_tests(
)
{
    return true;
}

#endif  // DJINTERP_TEST_TRANSDUCER_ENABLED


NS_END  // testing
NS_END  // djinterp
