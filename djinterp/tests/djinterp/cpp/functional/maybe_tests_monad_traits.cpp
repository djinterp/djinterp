/*******************************************************************************
* djinterp [test]                                   maybe_tests_monad_traits.cpp
*
* Tests for the monad_traits<maybe<_T>> specialization defined in maybe.hpp
* section V.  The generic monad protocol (free bind/map_with/etc. in
* monad.hpp) is covered by the monad module's own suite; here we test only the
* specialization's own surface as declared in maybe.hpp:
*   value_type, rebind, is_specialized, unit, and bind.
*
*
* path:      /tests/djinterp/cpp/functional/maybe_tests_monad_traits.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.01
*                                                            revised: 2026.11.09
*******************************************************************************/

// std
#include <string>
#include <type_traits>
// djinterp
#include "./maybe_tests.hpp"


NS_DJINTERP
NS_TESTING


// compile-time surface of the specialization
static_assert(monad_traits<DMaybe<int>>::is_specialized::value,
    "monad_traits<maybe>: is_specialized is true_type");
static_assert(
    std::is_same<monad_traits<DMaybe<int>>::value_type, int>::value,
    "monad_traits<maybe>: value_type is the contained type");
static_assert(
    std::is_same<
        monad_traits<DMaybe<int>>::template rebind<std::string>,
        DMaybe<std::string>>::value,
    "monad_traits<maybe>: rebind<U> yields maybe<U>");


bool test_monad_traits()
{
    bool verdict = true;

    using traits = monad_traits<DMaybe<int>>;

    // unit lifts a value into a maybe (equivalent to just)
    DMaybe<int> u = traits::unit(5);
    record_assertion(verdict, u.has_value() && u.value() == 5,
        "monad_traits: unit lifts a value into just");

    // bind threads a present value through a maybe-returning function
    DMaybe<int> source(8);
    DMaybe<int> bound = traits::bind(source, DFnHalfIfEven());
    record_assertion(verdict, bound.has_value() && bound.value() == 4,
        "monad_traits: bind threads a present value");

    // bind on empty short-circuits
    DMaybe<int> empty;
    DMaybe<int> bound_empty = traits::bind(empty, DFnHalfIfEven());
    record_assertion(verdict, !bound_empty.has_value(),
        "monad_traits: bind on empty yields empty");

    // bind where the function yields nothing
    DMaybe<int> odd(7);
    DMaybe<int> bound_nothing = traits::bind(odd, DFnHalfIfEven());
    record_assertion(verdict, !bound_nothing.has_value(),
        "monad_traits: bind propagates an inner nothing");

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
