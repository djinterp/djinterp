#include "filter_tests.hpp"

// std
#include <array>
#include <list>
#include <map>
#include <type_traits>
#include <vector>


NS_DJINTERP
NS_TESTING


// ---- compile-time assertions (the traits are a compile-time contract) ----

// low-level member detection: std::vector exposes the full surface; int none.
static_assert( has_begin<std::vector<int> >::value,         "vector has begin");
static_assert( has_end<std::vector<int> >::value,           "vector has end");
static_assert( has_value_type<std::vector<int> >::value,    "vector value_type");
static_assert( has_push_back<std::vector<int> >::value,     "vector push_back");
static_assert( has_insert<std::vector<int> >::value,        "vector insert");
static_assert( has_size<std::vector<int> >::value,          "vector size");
static_assert( has_empty<std::vector<int> >::value,         "vector empty");
static_assert( has_iterator<std::vector<int> >::value,      "vector iterator");
static_assert( has_const_iterator<std::vector<int> >::value,"vector const_it");

static_assert( !has_begin<int>::value,        "int no begin");
static_assert( !has_value_type<int>::value,   "int no value_type");
static_assert( !has_push_back<int>::value,    "int no push_back");
static_assert( !has_iterator<int>::value,     "int no iterator");

// std::array is iterable and has value_type, but has neither push_back nor
// insert, so it is NOT output-capable and therefore NOT filterable.
static_assert( is_iterable<std::array<int, 3> >::value,        "array iterable");
static_assert( has_value_type<std::array<int, 3> >::value,     "array value_type");
static_assert( !is_output_capable<std::array<int, 3> >::value, "array !output");
static_assert( !is_filterable<std::array<int, 3> >::value,     "array !filterable");

// composite contract on the standard containers.
static_assert( is_filterable<std::vector<int> >::value,            "vector filterable");
static_assert( is_filterable<std::list<int> >::value,              "list filterable");
static_assert( is_filterable<std::map<int, int> >::value,          "map filterable");
static_assert( !is_filterable<int>::value,                         "int !filterable");

// cv / ref qualifiers are stripped before the composite check.
static_assert( is_filterable<const std::vector<int>& >::value, "const-ref filterable");

// native .filter() detection.
static_assert( has_filter_method<native_filter_type>::value, "native has filter()");
static_assert( !has_filter_method<std::vector<int> >::value, "vector no filter()");

// value-type extraction.
static_assert(
    std::is_same<filterable_value_t<std::vector<int> >, int>::value,
    "filterable_value_t<vector<int>> == int");


/*
test_filterable_member_detection
  Exercises the low-level member detectors at run time.
  Tests the following:
  - a std::vector reports begin/end/value_type/push_back/insert/size/empty/
    iterator/const_iterator; a scalar reports none of them
*/
bool
test_filterable_member_detection(
)
{
    D_INTERNAL_FLT_CHECK((has_begin<std::vector<int> >::value));
    D_INTERNAL_FLT_CHECK((has_end<std::vector<int> >::value));
    D_INTERNAL_FLT_CHECK((has_value_type<std::vector<int> >::value));
    D_INTERNAL_FLT_CHECK((has_push_back<std::vector<int> >::value));
    D_INTERNAL_FLT_CHECK((has_insert<std::vector<int> >::value));
    D_INTERNAL_FLT_CHECK((has_size<std::vector<int> >::value));
    D_INTERNAL_FLT_CHECK((has_empty<std::vector<int> >::value));
    D_INTERNAL_FLT_CHECK((has_iterator<std::vector<int> >::value));
    D_INTERNAL_FLT_CHECK((has_const_iterator<std::vector<int> >::value));

    D_INTERNAL_FLT_CHECK((!has_begin<int>::value));
    D_INTERNAL_FLT_CHECK((!has_value_type<int>::value));
    D_INTERNAL_FLT_CHECK((!has_push_back<int>::value));
    D_INTERNAL_FLT_CHECK((!has_iterator<int>::value));

    return true;
}


/*
test_filterable_iterable
  Exercises the is_iterable composite.
  Tests the following:
  - vector / list / array are iterable; int is not
*/
bool
test_filterable_iterable(
)
{
    D_INTERNAL_FLT_CHECK((is_iterable<std::vector<int> >::value));
    D_INTERNAL_FLT_CHECK((is_iterable<std::list<int> >::value));
    D_INTERNAL_FLT_CHECK((is_iterable<std::array<int, 3> >::value));
    D_INTERNAL_FLT_CHECK((!is_iterable<int>::value));

    return true;
}


/*
test_filterable_output_capable
  Exercises the is_output_capable composite.
  Tests the following:
  - vector / list (push_back) and map (insert) are output-capable
  - std::array is not (no push_back, no insert)
*/
bool
test_filterable_output_capable(
)
{
    D_INTERNAL_FLT_CHECK((is_output_capable<std::vector<int> >::value));
    D_INTERNAL_FLT_CHECK((is_output_capable<std::list<int> >::value));
    D_INTERNAL_FLT_CHECK((is_output_capable<std::map<int, int> >::value));
    D_INTERNAL_FLT_CHECK((!is_output_capable<std::array<int, 3> >::value));

    return true;
}


/*
test_filterable_composite
  Exercises the top-level is_filterable contract, including the array edge.
  Tests the following:
  - vector / list / map are filterable
  - std::array is iterable with a value_type but not output-capable, so it is
    NOT filterable
  - a scalar is not filterable
  - cv/ref qualifiers are stripped before the check
*/
bool
test_filterable_composite(
)
{
    D_INTERNAL_FLT_CHECK((is_filterable<std::vector<int> >::value));
    D_INTERNAL_FLT_CHECK((is_filterable<std::list<int> >::value));
    D_INTERNAL_FLT_CHECK((is_filterable<std::map<int, int> >::value));
    D_INTERNAL_FLT_CHECK((!is_filterable<std::array<int, 3> >::value));
    D_INTERNAL_FLT_CHECK((!is_filterable<int>::value));
    D_INTERNAL_FLT_CHECK((is_filterable<const std::vector<int>&>::value));

    return true;
}


/*
test_filterable_native_method
  Exercises has_filter_method.
  Tests the following:
  - the native_filter_type fixture (a .filter(pred) member) is detected
  - a plain std::vector is not
*/
bool
test_filterable_native_method(
)
{
    D_INTERNAL_FLT_CHECK((has_filter_method<native_filter_type>::value));
    D_INTERNAL_FLT_CHECK((!has_filter_method<std::vector<int> >::value));

    return true;
}


/*
test_filterable_value_type
  Exercises filterable_value_t.
  Tests the following:
  - the alias recovers the element type of a filterable container
*/
bool
test_filterable_value_type(
)
{
    D_INTERNAL_FLT_CHECK(
        (std::is_same<filterable_value_t<std::vector<int> >, int>::value));
    D_INTERNAL_FLT_CHECK(
        (std::is_same<filterable_value_t<std::list<char> >, char>::value));

    return true;
}


/*
run_filterable_tests
  Aggregates every folded filterable-container-traits test.
  Tests the following:
  - all member-detection / iterable / output-capable / composite /
    native-method / value-type tests pass
*/
bool
run_filterable_tests(
)
{
    return ( test_filterable_member_detection() &&
             test_filterable_iterable()          &&
             test_filterable_output_capable()    &&
             test_filterable_composite()         &&
             test_filterable_native_method()     &&
             test_filterable_value_type() );
}


NS_END  // testing
NS_END  // djinterp
