// djinterp [test] -- curry.hpp Section III (curry factories)
#include "./curry_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
test_curry_factory
  Exercises the auto-arity curry() factory and every branch of its
  curry_helper accumulator.
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
void
test_curry_factory(
    test::test_handler& _h
)
{
    add3 a3;
    auto c = curry(a3);

    // single-argument chaining: extend, extend, invoke
    test::record_assertion(_h, (c(1)(2)(3) == 6),
                           "curry: c(1)(2)(3) chains to 6");

    // grouped, all at once: multi-arg invoke
    test::record_assertion(_h, (c(1, 2, 3) == 6),
                           "curry: c(1, 2, 3) invokes at once");

    // mixed groupings
    test::record_assertion(_h, (c(1, 2)(3) == 6),
                           "curry: c(1, 2)(3)");
    test::record_assertion(_h, (c(1)(2, 3) == 6),
                           "curry: c(1)(2, 3)");

    // partial application reused
    auto partial = c(1);
    test::record_assertion(_h, (partial(2)(3) == 6),
                           "curry: stored partial completes via singles");
    test::record_assertion(_h, (partial(2, 3) == 6),
                           "curry: stored partial completes via group");

    // unary callable: immediate invocation
    test::record_assertion(_h, (curry(echo_int{})(9) == 9),
                           "curry: unary callable invokes immediately");

    // nullary callable: invocation with no arguments
    test::record_assertion(_h, (curry(nullary_seven{})() == 7),
                           "curry: nullary callable invokes with ()");

    // higher-arity extension (4-ary): two args extend, two more invoke
    auto c4 = curry(add4{});
    test::record_assertion(_h, (c4(1, 2)(3, 4) == 10),
                           "curry: 4-ary extends on a pair then invokes");
    test::record_assertion(_h, (c4(1)(2)(3)(4) == 10),
                           "curry: 4-ary full single-arg chain");
    test::record_assertion(_h, (c4(1, 2)(3)(4) == 10),
                           "curry: 4-ary group then singles");
    test::record_assertion(_h, (c4(1)(2, 3, 4) == 10),
                           "curry: 4-ary single then triple group");

    return;
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
void
test_curry_n_factory(
    test::test_handler& _h
)
{
    add3 a3;
    auto cn = curry_n<3>(a3);

    // single-arg chain, terminal value via copy-initialization
    const int chained = cn(1)(2)(3);
    test::record_assertion(_h, (chained == 6),
                           "curry_n<3>: single-arg chain (implicit conv)");

    // single-arg chain, terminal value via explicit ()
    test::record_assertion(_h, (cn(1)(2)(3)() == 6),
                           "curry_n<3>: single-arg chain (explicit ())");

    // grouped forms
    const int grouped_pair = cn(1, 2)(3);
    test::record_assertion(_h, (grouped_pair == 6),
                           "curry_n<3>: cn(1, 2)(3)");
    const int grouped_all = cn(1, 2, 3);
    test::record_assertion(_h, (grouped_all == 6),
                           "curry_n<3>: cn(1, 2, 3)");

    // curry_n<1> over a unary callable
    const int unary_n = curry_n<1>(echo_int{})(12);
    test::record_assertion(_h, (unary_n == 12),
                           "curry_n<1>: unary terminal (implicit conv)");
    test::record_assertion(_h, (curry_n<1>(echo_int{})(13)() == 13),
                           "curry_n<1>: unary terminal (explicit ())");

    // curry_n<0> over a nullary callable
    test::record_assertion(_h, (curry_n<0>(nullary_seven{})() == 7),
                           "curry_n<0>: nullary terminal (explicit ())");
    const int zero_n = curry_n<0>(nullary_seven{});
    test::record_assertion(_h, (zero_n == 7),
                           "curry_n<0>: nullary terminal (implicit conv)");

    // curry_n<2> over a binary callable
    const int binary_singles = curry_n<2>(add2{})(3)(4);
    test::record_assertion(_h, (binary_singles == 7),
                           "curry_n<2>: binary via singles");
    test::record_assertion(_h, (curry_n<2>(add2{})(3, 4)() == 7),
                           "curry_n<2>: binary via group");

    return;
}


NS_END  // testing
NS_END  // djinterp
