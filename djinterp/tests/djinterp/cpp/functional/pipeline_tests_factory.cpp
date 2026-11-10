/*******************************************************************************
* djinterp [test]                                     pipeline_tests_factory.cpp
*
* Convenience factory tests (section II):
*   test_factory -- the free pipeline_from(container) and
*                   pipeline_from(pointer, count), including element-type
*                   deduction.
*
*
* path:      /tests/djinterp/cpp/functional/pipeline_tests_factory.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.11.09
*******************************************************************************/

// std
#include <list>
#include <type_traits>
#include <vector>
// djinterp
#include "./pipeline_tests.hpp"


NS_DJINTERP
NS_TESTING


bool test_factory()
{
    bool verdict = true;

    // pipeline_from(container) deduces the element type
    std::vector<int> vec = { 1, 2, 3 };
    auto             p = pipeline_from(vec);
    record_assertion(verdict,
        is_pipeline<decltype(p)>::value
        && std::is_same<pipeline_value_type_t<decltype(p)>, int>::value,
        "factory: pipeline_from(container) deduces a pipeline of the "
        "element type");
    record_assertion(verdict,
        p.to_vector() == std::vector<int>({ 1, 2, 3 }),
        "factory: pipeline_from(container) copies the elements");

    // works on a generic container
    std::list<int> lst = { 4, 5 };
    record_assertion(verdict,
        pipeline_from(lst).to_vector() == std::vector<int>({ 4, 5 }),
        "factory: pipeline_from works on any iterable container");

    // pipeline_from(pointer, count)
    const int arr[] = { 7, 8, 9 };
    auto      pa = pipeline_from(arr, 3);
    record_assertion(verdict,
        is_pipeline<decltype(pa)>::value
        && pa.to_vector() == std::vector<int>({ 7, 8, 9 }),
        "factory: pipeline_from(pointer, count) copies a raw array");

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
