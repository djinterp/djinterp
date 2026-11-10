/*******************************************************************************
* djinterp [cpp]                                     curry_tests_combinators.cpp
*
*
* path:      /tests/djinterp/cpp/functional/curry_tests_combinators.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.31
*                                                            revised: 2026.11.09
*******************************************************************************/
// std
#include <string>
// djinterp
// djinterp [test] -- curry.hpp Section VI (constant-valued combinators)
#include "./curry_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
test_identity
  Exercises the identity function object (DIdentityFnHelper).
  Tests the following:
  - returns its argument value unchanged
  - preserves the lvalue category: the result is an lvalue reference bound to
    the original object (assignable, same address)
  - is value-category- and type-agnostic (works for non-arithmetic types)
*/
bool test_identity()
{
    bool verdict = true;

    // value passthrough
    record_assertion(verdict, (identity(42) == 42),
                           "identity: returns its argument value");

    // lvalue reference preservation: assignable and same address
    int x = 5;
    identity(x) = 10;
    record_assertion(verdict, (x == 10),
                           "identity: returns an assignable lvalue reference");
    record_assertion(verdict, (&identity(x) == &x),
                           "identity: result aliases the original object");

    // non-arithmetic type
    std::string s = "hi";
    record_assertion(verdict, (identity(s) == "hi"),
                           "identity: works for non-arithmetic types");

    return verdict;
}


/*
test_always_constant
  Exercises always() and its constant() alias (DAlwaysHelper).
  Tests the following:
  - a nullary call returns the stored value
  - the stored value is returned regardless of the number, type, or value of
    the arguments supplied
  - constant() is a behavioural alias of always()
  - the stored value type is independent of the argument types
*/
bool test_always_constant()
{
    bool verdict = true;

    auto five = always(5);

    // nullary
    record_assertion(verdict, (five() == 5),
                           "always: nullary call returns stored value");

    // arguments of varying count and type are ignored
    record_assertion(verdict, (five(1, 2, std::string("x")) == 5),
                           "always: ignores all supplied arguments");

    // constant alias
    record_assertion(verdict, (constant(7)(1, 2, 3) == 7),
                           "constant: alias of always");

    // stored value type independent of arg types
    auto greet = always(std::string("hello"));
    record_assertion(verdict, (greet(0, 1.5) == "hello"),
                           "always: stored value type is independent of args");

    return verdict;
}


/*
test_never
  Exercises the never predicate primitive (DNeverHelper).
  Tests the following:
  - returns false for a nullary call
  - returns false regardless of the number, type, or value of arguments
*/
bool test_never()
{
    bool verdict = true;

    record_assertion(verdict, (never() == false),
                           "never: nullary call returns false");
    record_assertion(verdict,
                           (never(1, std::string("x"), 2.0) == false),
                           "never: returns false for any arguments");

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
