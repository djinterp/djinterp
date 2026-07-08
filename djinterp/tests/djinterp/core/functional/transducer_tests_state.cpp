#include "transducer_tests.hpp"


NS_DJINTERP
NS_TESTING


#if DJINTERP_TEST_TRANSDUCER_ENABLED

/*
test_state_reducing_basic
  Verifies reducing_state holds and exposes a mutable accumulator and starts
  not-done.
  Tests the following:
  - a freshly constructed state is not done
  - accumulator() returns the seeded value and is mutable
  - the const accumulator() overload observes the mutation
*/
bool
test_state_reducing_basic(
)
{
    reducing_state<int> st(10);

    D_INTERNAL_TRD_CHECK(!st.is_done());
    D_INTERNAL_TRD_CHECK(st.accumulator() == 10);

    st.accumulator() += 5;
    const reducing_state<int>& cst = st;
    D_INTERNAL_TRD_CHECK(cst.accumulator() == 15);

    return true;
}


/*
test_state_mark_done
  Verifies mark_done flips the termination flag.
  Tests the following:
  - is_done is false until mark_done, true after
*/
bool
test_state_mark_done(
)
{
    reducing_state<int> st(0);

    D_INTERNAL_TRD_CHECK(!st.is_done());
    st.mark_done();
    D_INTERNAL_TRD_CHECK(st.is_done());

    return true;
}


/*
test_state_reduced_wrapper
  Verifies the reduced<_Acc> convenience wrapper stores and returns its value
  via the const-lvalue accessor.
  Tests the following:
  - value() on a const reduced returns the wrapped accumulator
*/
bool
test_state_reduced_wrapper(
)
{
    const reduced<int> r(42);

    D_INTERNAL_TRD_CHECK(r.value() == 42);

    return true;
}


/*
test_state_reduced_move
  Verifies the rvalue value() overload moves the wrapped accumulator out.
  Tests the following:
  - value() on an rvalue reduced yields the stored value
*/
bool
test_state_reduced_move(
)
{
    int moved = reduced<int>(7).value();

    D_INTERNAL_TRD_CHECK(moved == 7);

    return true;
}


/*
run_state_tests
  Aggregates every reduced / reducing_state test.
*/
bool
run_state_tests(
)
{
    return ( test_state_reducing_basic()  &&
             test_state_mark_done()        &&
             test_state_reduced_wrapper()  &&
             test_state_reduced_move() );
}

#else  // !DJINTERP_TEST_TRANSDUCER_ENABLED

// C++11 stub: the module is suppressed below C++14; report success.
bool
run_state_tests(
)
{
    return true;
}

#endif  // DJINTERP_TEST_TRANSDUCER_ENABLED


NS_END  // testing
NS_END  // djinterp
