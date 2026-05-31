// djinterp [test] -- curry.hpp Section I (internal machinery)
#include "./curry_tests.hpp"

#include <tuple>
#include <type_traits>


NS_DJINTERP
NS_TESTING


/*
test_internal_machinery
  Exercises the C++11 index-sequence fallback and the std::apply stand-in
  that underpin every curry invocation.
  Tests the following:
  - make_index_seq<N> builds index_seq<0, 1, ..., N-1> for N > 0
  - make_index_seq<0> degenerates to the empty index_seq<>
  - apply_tuple invokes a callable with a zero-element tuple (nullary path)
  - apply_tuple invokes a callable with a one-element tuple
  - apply_tuple expands a multi-element tuple in order
  - apply_tuple preserves argument ordering (not merely the sum)
*/
void
test_internal_machinery(
    test::test_handler& _h
)
{
    // ---- make_index_seq ----
    const bool seq3_ok =
        std::is_same<internal::make_index_seq<3>,
                     internal::index_seq<0, 1, 2> >::value;
    test::record_assertion(_h, seq3_ok,
                           "make_index_seq<3> == index_seq<0,1,2>");

    const bool seq1_ok =
        std::is_same<internal::make_index_seq<1>,
                     internal::index_seq<0> >::value;
    test::record_assertion(_h, seq1_ok,
                           "make_index_seq<1> == index_seq<0>");

    const bool seq0_ok =
        std::is_same<internal::make_index_seq<0>,
                     internal::index_seq<> >::value;
    test::record_assertion(_h, seq0_ok,
                           "make_index_seq<0> == index_seq<>");

    // ---- apply_tuple ----
    const int applied_nullary =
        internal::apply_tuple(nullary_seven{}, std::tuple<>{});
    test::record_assertion(_h, (applied_nullary == 7),
                           "apply_tuple over empty tuple invokes nullary");

    const int applied_unary =
        internal::apply_tuple(echo_int{}, std::make_tuple(5));
    test::record_assertion(_h, (applied_unary == 5),
                           "apply_tuple over 1-tuple forwards single arg");

    const int applied_ternary =
        internal::apply_tuple(add3{}, std::make_tuple(1, 2, 3));
    test::record_assertion(_h, (applied_ternary == 6),
                           "apply_tuple over 3-tuple expands all args");

    const int applied_ordered =
        internal::apply_tuple(digits3{}, std::make_tuple(1, 2, 3));
    test::record_assertion(_h, (applied_ordered == 123),
                           "apply_tuple preserves argument order");

    return;
}


NS_END  // testing
NS_END  // djinterp
