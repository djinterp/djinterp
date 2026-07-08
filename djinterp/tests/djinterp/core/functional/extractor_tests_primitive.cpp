#include "extractor_tests.hpp"

// std
#include <tuple>
#include <utility>


NS_DJINTERP
NS_TESTING


/*
test_primitive_identity
  Verifies identity returns its source unchanged.
  Tests the following:
  - identity<int>() yields the input value
  - identity over a struct yields an equal struct (by field)
*/
bool
test_primitive_identity(
)
{
    D_INTERNAL_EXT_CHECK(extractors::identity<int>()(42) == 42);

    person p = { 1, 30, 10 };
    D_INTERNAL_EXT_CHECK(extractors::identity<person>()(p).id == 1);
    D_INTERNAL_EXT_CHECK(extractors::identity<person>()(p).age == 30);
    D_INTERNAL_EXT_CHECK(extractors::identity<person>()(p).dept == 10);

    return true;
}


/*
test_primitive_constant
  Verifies constant ignores its source and always yields the stored value.
  Tests the following:
  - the same constant extractor returns its value for different sources
  - the source type is irrelevant (applied to int and to person)
*/
bool
test_primitive_constant(
)
{
    D_INTERNAL_EXT_CHECK(extractors::constant(7)(0) == 7);
    D_INTERNAL_EXT_CHECK(extractors::constant(7)(999) == 7);

    person p = { 1, 30, 10 };
    D_INTERNAL_EXT_CHECK(extractors::constant(-1)(p) == -1);

    return true;
}


/*
test_primitive_from_function_functor
  Verifies from_function lifts a unary functor into an extractor.
  Tests the following:
  - the lifted functor extracts the expected feature
*/
bool
test_primitive_from_function_functor(
)
{
    person p = { 1, 30, 10 };
    D_INTERNAL_EXT_CHECK(extractors::from_function(get_age())(p) == 30);

    return true;
}


/*
test_primitive_from_function_pointer
  Confirms from_function accepts a plain free function (pointer), not only a
  functor.
  Tests the following:
  - a function-pointer extractor yields the same result as the functor form
*/
bool
test_primitive_from_function_pointer(
)
{
    person p = { 1, 30, 10 };
    D_INTERNAL_EXT_CHECK(extractors::from_function(&age_of)(p) == 30);

    return true;
}


/*
test_primitive_from_member
  Verifies from_member reads a pointer-to-data-member.
  Tests the following:
  - each of the three members is read correctly through its own extractor
*/
bool
test_primitive_from_member(
)
{
    person p = { 5, 33, 99 };

    D_INTERNAL_EXT_CHECK(extractors::from_member(&person::id)(p) == 5);
    D_INTERNAL_EXT_CHECK(extractors::from_member(&person::age)(p) == 33);
    D_INTERNAL_EXT_CHECK(extractors::from_member(&person::dept)(p) == 99);

    return true;
}


/*
test_primitive_from_index
  Verifies from_index reads the N-th element of a tuple / pair via std::get.
  Tests the following:
  - from_index<N> selects the right element of a std::tuple
  - it also works on a std::pair
*/
bool
test_primitive_from_index(
)
{
    std::tuple<int, int, int> t(10, 20, 30);
    D_INTERNAL_EXT_CHECK(extractors::from_index<0>()(t) == 10);
    D_INTERNAL_EXT_CHECK(extractors::from_index<1>()(t) == 20);
    D_INTERNAL_EXT_CHECK(extractors::from_index<2>()(t) == 30);

    std::pair<int, int> pr(101, 202);
    D_INTERNAL_EXT_CHECK(extractors::from_index<0>()(pr) == 101);
    D_INTERNAL_EXT_CHECK(extractors::from_index<1>()(pr) == 202);

    return true;
}


/*
test_primitive_constexpr
  Confirms the primitive extractors are usable in constant expressions, as
  the module advertises.
  Tests the following:
  - identity and constant evaluate at compile time
  - a composed (then_extract) primitive also evaluates at compile time
  Guarded to C++14+, where relaxed constexpr makes the compile-time
  evaluation of these calls portable across compilers; the runtime path
  already covers the same behaviour below C++14.
*/
bool
test_primitive_constexpr(
)
{
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    constexpr int a = extractors::identity<int>()(5);
    static_assert(a == 5, "identity is constexpr");

    constexpr int b = extractors::constant(9)(0);
    static_assert(b == 9, "constant is constexpr");

    constexpr int c =
        extractors::then_extract(extractors::identity<int>(),
                                 extractors::constant(3))(123);
    static_assert(c == 3, "then_extract(identity, constant) is constexpr");
#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

    return true;
}


/*
run_primitive_tests
  Aggregates every primitive-section test.
  Tests the following:
  - all identity / constant / from_function / from_member / from_index /
    constexpr tests pass
*/
bool
run_primitive_tests(
)
{
    return ( test_primitive_identity()              &&
             test_primitive_constant()              &&
             test_primitive_from_function_functor() &&
             test_primitive_from_function_pointer() &&
             test_primitive_from_member()           &&
             test_primitive_from_index()            &&
             test_primitive_constexpr() );
}


NS_END  // testing
NS_END  // djinterp
