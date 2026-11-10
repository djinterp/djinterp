/*******************************************************************************
* djinterp [cpp]                                     filter_tests_filterable.cpp
*
*
* path:      /tests/djinterp/cpp/functional/filter_tests_filterable.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.11.09
*******************************************************************************/
// std
#include <array>
#include <list>
#include <map>
#include <type_traits>
#include <vector>
// djinterp
#include "filter_tests.hpp"


NS_DJINTERP
NS_TESTING


// ---- compile-time assertions (the traits are a compile-time contract) ----

// DPushBackOnly / DInsertOnly
//   struct: fixtures offering one insertion each, so each of
// is_output_capable's two detections is seen alone.
struct DPushBackOnly
{
    typedef int value_type;

    void push_back(int)
    {
        return;
    }
};

struct DInsertOnly
{
    typedef int  value_type;
    typedef int* iterator;

    iterator end()
    {
        return nullptr;
    }

    iterator insert(iterator _at, int)
    {
        return _at;
    }
};

// the members the composite reads: value_type (meta's has_value_type) and
// either insertion; a scalar has none of them.
static_assert( has_value_type<std::vector<int> >::value,  "vector value_type");
static_assert( !has_value_type<int>::value,               "int no value_type");
static_assert( is_output_capable<DPushBackOnly>::value,   "push_back alone");
static_assert( is_output_capable<DInsertOnly>::value,     "insert alone");
static_assert( !is_output_capable<int>::value,            "int no insertion");

// std::array is iterable and has value_type, but has neither push_back nor
// insert, so it is NOT output-capable and therefore NOT filterable.
static_assert( internal::is_filter_iterable<std::array<int, 3> >::value,
               "array iterable");
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
  Exercises the members the composite reads, at run time.
  Tests the following:
  - std::vector has a value_type; a scalar has none
  - push_back alone, and insert(iterator, value) alone, each make a type
    output-capable; a scalar is not
  (begin/end, size, empty, push_back and insert as general container questions
  are the container traits' and are tested in their suites.)
*/
bool test_filterable_member_detection()
{
    D_INTERNAL_FLT_CHECK((has_value_type<std::vector<int> >::value));
    D_INTERNAL_FLT_CHECK((!has_value_type<int>::value));
    D_INTERNAL_FLT_CHECK((is_output_capable<DPushBackOnly>::value));
    D_INTERNAL_FLT_CHECK((is_output_capable<DInsertOnly>::value));
    D_INTERNAL_FLT_CHECK((!is_output_capable<int>::value));

    return true;
}


/*
test_filterable_iterable
  Exercises the iteration is_filterable reads (internal::is_filter_iterable;
  the public is_iterable is container/iterator/iterator_traits.hpp's).
  Tests the following:
  - vector / list / array are iterable; int is not
  - a type with value_type and push_back but no begin/end is not iterable,
    and so not filterable
*/
bool test_filterable_iterable()
{
    D_INTERNAL_FLT_CHECK(
        (internal::is_filter_iterable<std::vector<int> >::value));
    D_INTERNAL_FLT_CHECK(
        (internal::is_filter_iterable<std::list<int> >::value));
    D_INTERNAL_FLT_CHECK(
        (internal::is_filter_iterable<std::array<int, 3> >::value));
    D_INTERNAL_FLT_CHECK(
        (!internal::is_filter_iterable<int>::value));
    D_INTERNAL_FLT_CHECK(
        (!internal::is_filter_iterable<DPushBackOnly>::value));
    D_INTERNAL_FLT_CHECK((!is_filterable<DPushBackOnly>::value));

    return true;
}


/*
test_filterable_output_capable
  Exercises the is_output_capable composite.
  Tests the following:
  - vector / list (push_back) and map (insert) are output-capable
  - std::array is not (no push_back, no insert)
*/
bool test_filterable_output_capable()
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
bool test_filterable_composite()
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
bool test_filterable_native_method()
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
bool test_filterable_value_type()
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
bool run_filterable_tests()
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
