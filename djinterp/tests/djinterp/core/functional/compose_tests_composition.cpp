/******************************************************************************
* djinterp [test]                              compose_tests_composition.cpp
*
*   Tests for compose.hpp Section II: the binary factories compose,
* pipe, and compose_transformer.  Verifies application order (math vs
* pipe), type-changing chains, the const& and rvalue call overloads,
* the .first()/.second() introspection accessors, and constexpr
* evaluation of a composed chain.
******************************************************************************/

#include "compose_tests.hpp"


NS_DJINTERP
NS_TESTING

using ::djinterp::test::test_handler;
using ::djinterp::test::unit_test_tally;
using ::djinterp::test::run_unit_test;
using ::djinterp::test::record_assertion;


namespace {

    // constexpr witness: compose must fold at compile time when its
    // operands are constexpr-callable.  add_one(double(5)) == 11.
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    D_CONSTEXPR int k_constexpr_compose =
        ::djinterp::compose(add_one(), doubler())(5);
    static_assert(k_constexpr_compose == 11,
                  "compose must evaluate in a constexpr context");

    D_CONSTEXPR int k_constexpr_pipe =
        ::djinterp::pipe(add_one(), doubler())(5);
    static_assert(k_constexpr_pipe == 12,
                  "pipe must evaluate in a constexpr context");
#endif

}  // namespace


/*
compose_tests_composition
  Exercises compose.hpp Section II (compose / pipe / compose_transformer).
  Tests the following:
  - compose is math-order: compose(f, g)(x) == f(g(x))
  - pipe is pipe-order: pipe(f, g)(x) == g(f(x))
  - compose_transformer matches pipe order
  - composing across changing types (int -> string -> length)
  - the const-lvalue and rvalue operator() overloads both work
  - .first()/.second() expose the stored operands in execution order
  - a composed chain folds at compile time (constexpr, C++14+)
*/
void
compose_tests_composition(
    test_handler& _handler
)
{
    unit_test_tally tally;

    // compose: math order f(g(x))
    run_unit_test(
        _handler,
        tally,
        "compose applies in math order f(g(x))",
        [&]()
        {
            // add_one(double(5)) = add_one(10) = 11
            record_assertion(
                _handler,
                (::djinterp::compose(add_one(), doubler())(5) == 11),
                "compose(add_one, doubler)(5) == 11");

            // double(add_one(5)) would be 12; confirm we did NOT get that
            record_assertion(
                _handler,
                (::djinterp::compose(add_one(), doubler())(5) != 12),
                "compose order is not reversed");
        });

    // pipe: pipe order g(f(x))
    run_unit_test(
        _handler,
        tally,
        "pipe applies in pipe order g(f(x))",
        [&]()
        {
            // double(add_one(5)) = double(6) = 12
            record_assertion(
                _handler,
                (::djinterp::pipe(add_one(), doubler())(5) == 12),
                "pipe(add_one, doubler)(5) == 12");
        });

    // compose_transformer: same as pipe order
    run_unit_test(
        _handler,
        tally,
        "compose_transformer matches pipe order",
        [&]()
        {
            record_assertion(
                _handler,
                (::djinterp::compose_transformer(add_one(), doubler())(5)
                     == 12),
                "compose_transformer(add_one, doubler)(5) == 12");

            // pipe and compose_transformer must agree on the same inputs
            record_assertion(
                _handler,
                (::djinterp::compose_transformer(add_one(), doubler())(7)
                     == ::djinterp::pipe(add_one(), doubler())(7)),
                "compose_transformer agrees with pipe");
        });

    // type-changing composition: int -> string -> length
    run_unit_test(
        _handler,
        tally,
        "compose threads changing types",
        [&]()
        {
            // length_of(to_string(12345)) = length_of("12345") = 5
            record_assertion(
                _handler,
                (::djinterp::compose(length_of(), to_string_fn())(12345)
                     == 5),
                "length_of . to_string (12345) == 5");

            // boundary: single-digit input yields length 1
            record_assertion(
                _handler,
                (::djinterp::compose(length_of(), to_string_fn())(0) == 1),
                "length_of . to_string (0) == 1");
        });

    // both operator() overloads (const lvalue input and rvalue input)
    run_unit_test(
        _handler,
        tally,
        "compose handles lvalue and rvalue inputs",
        [&]()
        {
            auto c           = ::djinterp::compose(add_one(), doubler());
            const int lvalue = 5;

            // const-lvalue overload
            record_assertion(
                _handler,
                (c(lvalue) == 11),
                "compose accepts a const lvalue input");

            // rvalue overload
            record_assertion(
                _handler,
                (c(5) == 11),
                "compose accepts an rvalue input");
        });

    // introspection accessors expose operands in execution order
    run_unit_test(
        _handler,
        tally,
        "compose exposes .first()/.second() in execution order",
        [&]()
        {
            // compose(add_one, doubler) applies doubler first, add_one
            // second.  first() is the first-applied operand (doubler),
            // second() is the second-applied operand (add_one).
            auto c = ::djinterp::compose(add_one(), doubler());

            record_assertion(
                _handler,
                (c.first()(5) == 10),
                "compose .first() is the inner (first-applied) operand");

            record_assertion(
                _handler,
                (c.second()(5) == 6),
                "compose .second() is the outer (second-applied) operand");
        });

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    // constexpr foldability (validated at compile time above; surfaced
    // here as a runtime row).
    run_unit_test(
        _handler,
        tally,
        "compose/pipe fold in a constexpr context",
        [&]()
        {
            record_assertion(
                _handler,
                (k_constexpr_compose == 11),
                "constexpr compose chain == 11");

            record_assertion(
                _handler,
                (k_constexpr_pipe == 12),
                "constexpr pipe chain == 12");
        });
#endif

    return;
}


NS_END  // testing
NS_END  // djinterp
