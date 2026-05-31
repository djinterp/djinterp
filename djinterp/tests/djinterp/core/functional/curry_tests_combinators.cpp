// djinterp [test] -- curry.hpp Section VI (constant-valued combinators)
#include "./curry_tests.hpp"

#include <string>


NS_DJINTERP
NS_TESTING


/*
test_identity
  Exercises the identity function object (identity_fn_helper).
  Tests the following:
  - returns its argument value unchanged
  - preserves the lvalue category: the result is an lvalue reference bound to
    the original object (assignable, same address)
  - is value-category- and type-agnostic (works for non-arithmetic types)
*/
void
test_identity(
    test::test_handler& _h
)
{
    // value passthrough
    test::record_assertion(_h, (identity(42) == 42),
                           "identity: returns its argument value");

    // lvalue reference preservation: assignable and same address
    int x = 5;
    identity(x) = 10;
    test::record_assertion(_h, (x == 10),
                           "identity: returns an assignable lvalue reference");
    test::record_assertion(_h, (&identity(x) == &x),
                           "identity: result aliases the original object");

    // non-arithmetic type
    std::string s = "hi";
    test::record_assertion(_h, (identity(s) == "hi"),
                           "identity: works for non-arithmetic types");

    return;
}


/*
test_always_constant
  Exercises always() and its constant() alias (always_helper).
  Tests the following:
  - a nullary call returns the stored value
  - the stored value is returned regardless of the number, type, or value of
    the arguments supplied
  - constant() is a behavioural alias of always()
  - the stored value type is independent of the argument types
*/
void
test_always_constant(
    test::test_handler& _h
)
{
    auto five = always(5);

    // nullary
    test::record_assertion(_h, (five() == 5),
                           "always: nullary call returns stored value");

    // arguments of varying count and type are ignored
    test::record_assertion(_h, (five(1, 2, std::string("x")) == 5),
                           "always: ignores all supplied arguments");

    // constant alias
    test::record_assertion(_h, (constant(7)(1, 2, 3) == 7),
                           "constant: alias of always");

    // stored value type independent of arg types
    auto greet = always(std::string("hello"));
    test::record_assertion(_h, (greet(0, 1.5) == "hello"),
                           "always: stored value type is independent of args");

    return;
}


/*
test_never
  Exercises the never predicate primitive (never_helper).
  Tests the following:
  - returns false for a nullary call
  - returns false regardless of the number, type, or value of arguments
*/
void
test_never(
    test::test_handler& _h
)
{
    test::record_assertion(_h, (never() == false),
                           "never: nullary call returns false");
    test::record_assertion(_h,
                           (never(1, std::string("x"), 2.0) == false),
                           "never: returns false for any arguments");

    return;
}


NS_END  // testing
NS_END  // djinterp
