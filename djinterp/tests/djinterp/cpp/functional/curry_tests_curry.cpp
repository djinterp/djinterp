/*******************************************************************************
* djinterp [cpp]                                           curry_tests_curry.cpp
*
*
* path:      /tests/djinterp/cpp/functional/curry_tests_curry.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.31
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
// djinterp [test] -- curry.hpp Section III (curry factories)
#include "./curry_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
test_curry_factory
  Exercises the auto-arity curry() factory and every branch of its
  DCurryHelper accumulator.
  Tests the following:
  - single-argument chaining c(1)(2)(3): two extension steps then an
    invocation (the dispatch false_type then true_type paths)
  - immediate invocation of a unary callable (single-arg dispatch true_type)
  - the nullary path: curry of a zero-arg callable invoked with ()
  - all-at-once grouped invocation c(1, 2, 3) (call_multi true_type)
  - mixed grouping c(1)(2, 3) and c(1, 2)(3)
  - partial application reused across multiple completions
  - higher-arity extension on a 4-ary callable, where two arguments are
    insufficient and force the call_multi false_type branch before a later
    completion
*/
bool test_curry_factory()
{
    bool verdict = true;

    add3 a3;
    auto c = curry(a3);

    // single-argument chaining: extend, extend, invoke
    record_assertion(verdict, (c(1)(2)(3) == 6),
                           "curry: c(1)(2)(3) chains to 6");

    // grouped, all at once: multi-arg invoke
    record_assertion(verdict, (c(1, 2, 3) == 6),
                           "curry: c(1, 2, 3) invokes at once");

    // mixed groupings
    record_assertion(verdict, (c(1, 2)(3) == 6),
                           "curry: c(1, 2)(3)");
    record_assertion(verdict, (c(1)(2, 3) == 6),
                           "curry: c(1)(2, 3)");

    // partial application reused
    auto partial = c(1);
    record_assertion(verdict, (partial(2)(3) == 6),
                           "curry: stored partial completes via singles");
    record_assertion(verdict, (partial(2, 3) == 6),
                           "curry: stored partial completes via group");

    // unary callable: immediate invocation
    record_assertion(verdict, (curry(DEchoInt{})(9) == 9),
                           "curry: unary callable invokes immediately");

    // nullary callable: invocation with no arguments
    record_assertion(verdict, (curry(DNullarySeven{})() == 7),
                           "curry: nullary callable invokes with ()");

    // higher-arity extension (4-ary): two args extend, two more invoke
    auto c4 = curry(DAdd4{});
    record_assertion(verdict, (c4(1, 2)(3, 4) == 10),
                           "curry: 4-ary extends on a pair then invokes");
    record_assertion(verdict, (c4(1)(2)(3)(4) == 10),
                           "curry: 4-ary full single-arg chain");
    record_assertion(verdict, (c4(1, 2)(3)(4) == 10),
                           "curry: 4-ary group then singles");
    record_assertion(verdict, (c4(1)(2, 3, 4) == 10),
                           "curry: 4-ary single then triple group");

    return verdict;
}


/*
test_curry_n_factory
  Exercises the explicit-arity curry_n<N>() factory, both the counting
  curry_n_helper and its zero-arity terminal specialization.
  Tests the following:
  - single-argument chaining through arities 3 -> 2 -> 1 -> 0
  - the terminal value obtained by copy-initialization (operator _R) and by
    an explicit trailing () (terminal operator())
  - grouped argument forms cn(1, 2)(3) and cn(1, 2, 3)
  - curry_n<1> over a unary callable
  - curry_n<0> over a nullary callable, via both () and copy-initialization
  - curry_n<2> over a binary callable, single and grouped
  NOTE: a fully-saturated curry_n result is the terminal helper, not the
  underlying value; its value is taken by binding to the result type or by a
  trailing (). It is deliberately NOT compared directly against a literal,
  since the templated conversion operator is not deduced in that context.
*/
bool test_curry_n_factory()
{
    bool verdict = true;

    add3 a3;
    auto cn = curry_n<3>(a3);

    // single-arg chain, terminal value via copy-initialization
    const int chained = cn(1)(2)(3);
    record_assertion(verdict, (chained == 6),
                           "curry_n<3>: single-arg chain (implicit conv)");

    // single-arg chain, terminal value via explicit ()
    record_assertion(verdict, (cn(1)(2)(3)() == 6),
                           "curry_n<3>: single-arg chain (explicit ())");

    // grouped forms
    const int grouped_pair = cn(1, 2)(3);
    record_assertion(verdict, (grouped_pair == 6),
                           "curry_n<3>: cn(1, 2)(3)");
    const int grouped_all = cn(1, 2, 3);
    record_assertion(verdict, (grouped_all == 6),
                           "curry_n<3>: cn(1, 2, 3)");

    // curry_n<1> over a unary callable
    const int unary_n = curry_n<1>(DEchoInt{})(12);
    record_assertion(verdict, (unary_n == 12),
                           "curry_n<1>: unary terminal (implicit conv)");
    record_assertion(verdict, (curry_n<1>(DEchoInt{})(13)() == 13),
                           "curry_n<1>: unary terminal (explicit ())");

    // curry_n<0> over a nullary callable
    record_assertion(verdict, (curry_n<0>(DNullarySeven{})() == 7),
                           "curry_n<0>: nullary terminal (explicit ())");
    const int zero_n = curry_n<0>(DNullarySeven{});
    record_assertion(verdict, (zero_n == 7),
                           "curry_n<0>: nullary terminal (implicit conv)");

    // curry_n<2> over a binary callable
    const int binary_singles = curry_n<2>(add2{})(3)(4);
    record_assertion(verdict, (binary_singles == 7),
                           "curry_n<2>: binary via singles");
    record_assertion(verdict, (curry_n<2>(add2{})(3, 4)() == 7),
                           "curry_n<2>: binary via group");

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
