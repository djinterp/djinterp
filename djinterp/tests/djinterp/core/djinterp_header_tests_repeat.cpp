/******************************************************************************
* djinterp [test]                          djinterp_header_tests_repeat.cpp
*
*   Section III.v tests: repeat / repeat_t (compile-time tuple-of-N builder)
* and, through it, the internal repeat_type_helper recursion.
******************************************************************************/

#include "./djinterp_header_tests.hpp"


namespace
{

    // repeat_fixture
    //   type: a user class type used to confirm repeat works on user types,
    // not only fundamentals.
    struct repeat_fixture
    {};

}  // anonymous namespace


NS_DJINTERP
NS_TEST


/*
tests_repeat
  Verifies the repeat trait and its repeat_t alias build a std::tuple holding
  _Type repeated _NumTimes times, driving the internal repeat_type_helper
  recursion through its general and base (_N == 0) cases.
  Tests the following:
  - repeat<T, 0> yields std::tuple<> (the base-case / empty branch)
  - repeat<T, 1>, repeat<T, 3>, repeat<T, 5> yield tuples of the right arity
  - every element of the produced tuple is exactly _Type
  - tuple arity is confirmed independently via std::tuple_size
  - element identity is confirmed via std::tuple_element
  - the builder works on a fundamental type, a user class type, and the
    framework's own self marker type
  - both repeat<>::type (the struct) and repeat_t (the alias) are exercised
*/
bool
tests_repeat()
{
    bool ok = true;

    // zero count -> empty tuple (base case / conditional false branch).
    static_assert(std::is_same<repeat_t<int, 0>, std::tuple<>>::value,
                  "repeat_t<int, 0> must be std::tuple<>.");
    static_assert(std::tuple_size<repeat_t<int, 0>>::value == 0,
                  "repeat_t<int, 0> must have arity 0.");

    // single element.
    static_assert(std::is_same<repeat_t<int, 1>, std::tuple<int>>::value,
                  "repeat_t<int, 1> must be std::tuple<int>.");

    // several elements.
    static_assert(std::is_same<repeat_t<int, 3>,
                               std::tuple<int, int, int>>::value,
                  "repeat_t<int, 3> must be std::tuple<int, int, int>.");
    static_assert(std::is_same<repeat_t<char, 5>,
                               std::tuple<char, char, char, char, char>>::value,
                  "repeat_t<char, 5> must be a 5-tuple of char.");

    // arity confirmed independently of the is_same checks above.
    static_assert(std::tuple_size<repeat_t<int, 3>>::value == 3,
                  "repeat_t<int, 3> must have arity 3.");
    static_assert(std::tuple_size<repeat_t<char, 5>>::value == 5,
                  "repeat_t<char, 5> must have arity 5.");

    // element identity confirmed via tuple_element.
    static_assert(std::is_same<std::tuple_element<0, repeat_t<int, 3>>::type,
                               int>::value,
                  "repeat_t<int, 3> element 0 must be int.");
    static_assert(std::is_same<std::tuple_element<2, repeat_t<int, 3>>::type,
                               int>::value,
                  "repeat_t<int, 3> element 2 must be int.");

    // user class type.
    static_assert(std::is_same<repeat_t<repeat_fixture, 2>,
                               std::tuple<repeat_fixture,
                                          repeat_fixture>>::value,
                  "repeat_t must repeat a user class type.");

    // the framework's own self marker as the repeated type.
    static_assert(std::is_same<repeat_t<self, 3>,
                               std::tuple<self, self, self>>::value,
                  "repeat_t<self, 3> must be a 3-tuple of self.");

    // exercise the struct form directly (not just the alias).
    static_assert(std::is_same<repeat<int, 3>::type,
                               std::tuple<int, int, int>>::value,
                  "repeat<>::type must match repeat_t.");
    static_assert(std::is_same<repeat<int, 0>::type, std::tuple<>>::value,
                  "repeat<int, 0>::type must be std::tuple<>.");

    // runtime mirror.
    ok = ok && std::is_same<repeat_t<int, 0>, std::tuple<>>::value;
    ok = ok && (std::tuple_size<repeat_t<int, 3>>::value == 3);
    ok = ok && std::is_same<repeat_t<int, 3>,
                            std::tuple<int, int, int>>::value;
    ok = ok && std::is_same<repeat_t<self, 3>,
                            std::tuple<self, self, self>>::value;

    return ok;
}


NS_END  // test
NS_END  // djinterp
