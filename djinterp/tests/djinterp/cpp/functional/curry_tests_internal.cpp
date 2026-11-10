/*******************************************************************************
* djinterp [cpp]                                        curry_tests_internal.cpp
*
*
* path:      /tests/djinterp/cpp/functional/curry_tests_internal.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.31
*                                                            revised: 2026.11.09
*******************************************************************************/
// std
#include <tuple>
#include <type_traits>
// djinterp
// djinterp [test] -- curry.hpp Section I (internal machinery)
#include "./curry_tests.hpp"


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
bool test_internal_machinery()
{
    bool verdict = true;

    // ---- make_index_seq ----
    const bool seq3_ok =
        std::is_same<internal::make_index_seq<3>,
                     internal::index_seq<0, 1, 2> >::value;
    record_assertion(verdict, seq3_ok,
                           "make_index_seq<3> == index_seq<0,1,2>");

    const bool seq1_ok =
        std::is_same<internal::make_index_seq<1>,
                     internal::index_seq<0> >::value;
    record_assertion(verdict, seq1_ok,
                           "make_index_seq<1> == index_seq<0>");

    const bool seq0_ok =
        std::is_same<internal::make_index_seq<0>,
                     internal::index_seq<> >::value;
    record_assertion(verdict, seq0_ok,
                           "make_index_seq<0> == index_seq<>");

    // ---- apply_tuple ----
    const int applied_nullary =
        internal::apply_tuple(DNullarySeven{}, std::tuple<>{});
    record_assertion(verdict, (applied_nullary == 7),
                           "apply_tuple over empty tuple invokes nullary");

    const int applied_unary =
        internal::apply_tuple(DEchoInt{}, std::make_tuple(5));
    record_assertion(verdict, (applied_unary == 5),
                           "apply_tuple over 1-tuple forwards single arg");

    const int applied_ternary =
        internal::apply_tuple(add3{}, std::make_tuple(1, 2, 3));
    record_assertion(verdict, (applied_ternary == 6),
                           "apply_tuple over 3-tuple expands all args");

    const int applied_ordered =
        internal::apply_tuple(DDigits3{}, std::make_tuple(1, 2, 3));
    record_assertion(verdict, (applied_ordered == 123),
                           "apply_tuple preserves argument order");

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
