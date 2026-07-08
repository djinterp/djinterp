/******************************************************************************
* djinterp [test]                                   pipeline_tests_factory.cpp
*
* Convenience factory tests (section II):
*   test_factory -- the free pipeline_from(container) and
*                   pipeline_from(pointer, count), including element-type
*                   deduction.
******************************************************************************/

#include <vector>
#include <list>
#include <type_traits>
#include "./pipeline_tests.hpp"


NS_DJINTERP
NS_TESTING


void test_factory(test::test_handler& _h)
{
    // pipeline_from(container) deduces the element type
    std::vector<int> vec = { 1, 2, 3 };
    auto             p = pipeline_from(vec);
    test::record_assertion(_h,
        is_pipeline<decltype(p)>::value
        && std::is_same<pipeline_value_type_t<decltype(p)>, int>::value,
        "factory: pipeline_from(container) deduces a pipeline of the "
        "element type");
    test::record_assertion(_h,
        p.to_vector() == std::vector<int>({ 1, 2, 3 }),
        "factory: pipeline_from(container) copies the elements");

    // works on a generic container
    std::list<int> lst = { 4, 5 };
    test::record_assertion(_h,
        pipeline_from(lst).to_vector() == std::vector<int>({ 4, 5 }),
        "factory: pipeline_from works on any iterable container");

    // pipeline_from(pointer, count)
    const int arr[] = { 7, 8, 9 };
    auto      pa = pipeline_from(arr, 3);
    test::record_assertion(_h,
        is_pipeline<decltype(pa)>::value
        && pa.to_vector() == std::vector<int>({ 7, 8, 9 }),
        "factory: pipeline_from(pointer, count) copies a raw array");

    return;
}


NS_END  // testing
NS_END  // djinterp
