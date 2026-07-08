/******************************************************************************
* djinterp [test]                                 pipeline_tests_accessors.cpp
*
* Accessor / status / iteration tests (section iv):
*   test_accessors -- size, empty, has_error, error_code, data, operator[],
*                     and the two to_vector overloads (const& and &&).
*   test_iteration -- begin/end range-for support.
******************************************************************************/

#include <vector>
#include <utility>
#include "./pipeline_tests.hpp"


NS_DJINTERP
NS_TESTING


using pi = function_pipeline<int>;


void test_accessors(test::test_handler& _h)
{
    pi p = pi::from({ 10, 20, 30 });

    test::record_assertion(_h, p.size() == 3u && !p.empty(),
        "accessors: size and empty report the element count");

    test::record_assertion(_h, !p.has_error() && p.error_code() == 0,
        "accessors: a normal pipeline has no error and code 0");

    // data() exposes the underlying vector
    test::record_assertion(_h,
        p.data() == std::vector<int>({ 10, 20, 30 }),
        "accessors: data() exposes the underlying vector");

    // operator[]
    test::record_assertion(_h,
        p[0] == 10 && p[1] == 20 && p[2] == 30,
        "accessors: operator[] indexes elements");

    // empty pipeline
    pi e = pi::from(std::vector<int>());
    test::record_assertion(_h, e.empty() && e.size() == 0u,
        "accessors: an empty pipeline reports size 0");

    // to_vector() const& copies (leaves the source intact)
    std::vector<int> copy = p.to_vector();
    test::record_assertion(_h,
        copy == std::vector<int>({ 10, 20, 30 }) && p.size() == 3u,
        "accessors: to_vector() const& returns a copy, source intact");

    // to_vector() && moves out
    std::vector<int> moved = pi::from({ 7, 8 }).to_vector();
    test::record_assertion(_h,
        moved == std::vector<int>({ 7, 8 }),
        "accessors: to_vector() && moves the data out of a temporary");

    return;
}


void test_iteration(test::test_handler& _h)
{
    pi p = pi::from({ 1, 2, 3, 4 });

    // range-for via begin()/end()
    long sum = 0;
    for (int x : p)
    {
        sum += x;
    }
    test::record_assertion(_h, sum == 10,
        "iterate: range-for over begin()/end() visits every element");

    // begin == end for an empty pipeline
    pi e = pi::from(std::vector<int>());
    test::record_assertion(_h, e.begin() == e.end(),
        "iterate: begin() == end() for an empty pipeline");

    return;
}


NS_END  // testing
NS_END  // djinterp
