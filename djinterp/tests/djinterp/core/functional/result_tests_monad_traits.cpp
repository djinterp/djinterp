/******************************************************************************
* djinterp [test]                                result_tests_monad_traits.cpp
*
* Tests for the monad_traits<result<_T, _E>> specialization defined in
* result.hpp section V. The generic monad protocol (monad_bind / map_with /
* ... in monad.hpp) is covered by the monad module's own suite; here we test
* only the specialization's own surface as declared in result.hpp:
*   value_type, error_type, rebind, is_specialized, unit, and bind.
******************************************************************************/

#include <string>
#include <type_traits>
#include "./result_tests.hpp"


NS_DJINTERP
NS_TESTING


using ri = result<int, std::string>;


// compile-time surface of the specialization
static_assert(monad_traits<ri>::is_specialized::value,
    "monad_traits<result>: is_specialized is true_type");
static_assert(std::is_same<monad_traits<ri>::value_type, int>::value,
    "monad_traits<result>: value_type is the success type");
static_assert(std::is_same<monad_traits<ri>::error_type, std::string>::value,
    "monad_traits<result>: error_type is the error type");
static_assert(
    std::is_same<
        monad_traits<ri>::template rebind<std::string>,
        result<std::string, std::string>>::value,
    "monad_traits<result>: rebind<U> yields result<U, E>");


void test_monad_traits(test::test_handler& _h)
{
    using traits = monad_traits<ri>;

    // unit lifts a value into an ok result
    ri u = traits::unit(5);
    test::record_assertion(_h, u.is_ok() && u.value() == 5,
        "monad_traits: unit lifts a value into ok");

    // bind threads an ok value through a result-returning function
    ri okv = ok<int, std::string>(8);
    ri bound = traits::bind(okv, fn_safe_halve());
    test::record_assertion(_h, bound.is_ok() && bound.value() == 4,
        "monad_traits: bind threads an ok value");

    // bind on err short-circuits, preserving the error
    ri errv = err<int, std::string>("e");
    ri bound_err = traits::bind(errv, fn_safe_halve());
    test::record_assertion(_h, bound_err.is_err() && bound_err.error() == "e",
        "monad_traits: bind on err short-circuits");

    // bind where the inner function yields err
    ri odd = ok<int, std::string>(7);
    ri bound_inner = traits::bind(odd, fn_safe_halve());
    test::record_assertion(_h,
        bound_inner.is_err() && bound_inner.error() == "odd",
        "monad_traits: bind propagates an inner err");

    return;
}


NS_END  // testing
NS_END  // djinterp
