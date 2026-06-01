/******************************************************************************
* djinterp [test]                                 maybe_tests_monad_traits.cpp
*
* Tests for the monad_traits<maybe<_T>> specialization defined in maybe.hpp
* section V.  The generic monad protocol (free bind/map_with/etc. in
* monad.hpp) is covered by the monad module's own suite; here we test only the
* specialization's own surface as declared in maybe.hpp:
*   value_type, rebind, is_specialized, unit, and bind.
******************************************************************************/

#include <string>
#include <type_traits>
#include "./maybe_tests.hpp"


NS_DJINTERP
NS_TESTING


// compile-time surface of the specialization
static_assert(monad_traits<maybe<int>>::is_specialized::value,
    "monad_traits<maybe>: is_specialized is true_type");
static_assert(
    std::is_same<monad_traits<maybe<int>>::value_type, int>::value,
    "monad_traits<maybe>: value_type is the contained type");
static_assert(
    std::is_same<
        monad_traits<maybe<int>>::template rebind<std::string>,
        maybe<std::string>>::value,
    "monad_traits<maybe>: rebind<U> yields maybe<U>");


void test_monad_traits(test::test_handler& _h)
{
    using traits = monad_traits<maybe<int>>;

    // unit lifts a value into a maybe (equivalent to just)
    maybe<int> u = traits::unit(5);
    test::record_assertion(_h, u.has_value() && u.value() == 5,
        "monad_traits: unit lifts a value into just");

    // bind threads a present value through a maybe-returning function
    maybe<int> source(8);
    maybe<int> bound = traits::bind(source, fn_half_if_even());
    test::record_assertion(_h, bound.has_value() && bound.value() == 4,
        "monad_traits: bind threads a present value");

    // bind on empty short-circuits
    maybe<int> empty;
    maybe<int> bound_empty = traits::bind(empty, fn_half_if_even());
    test::record_assertion(_h, !bound_empty.has_value(),
        "monad_traits: bind on empty yields empty");

    // bind where the function yields nothing
    maybe<int> odd(7);
    maybe<int> bound_nothing = traits::bind(odd, fn_half_if_even());
    test::record_assertion(_h, !bound_nothing.has_value(),
        "monad_traits: bind propagates an inner nothing");

    return;
}


NS_END  // testing
NS_END  // djinterp
