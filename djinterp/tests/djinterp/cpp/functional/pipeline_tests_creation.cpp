/*******************************************************************************
* djinterp [test]                                    pipeline_tests_creation.cpp
*
* Pipeline creation tests (section i):
*   test_creation -- default ctor, from(container/vector&&/init-list/raw
*                    array), of(variadic), and error().
*
*
* path:      /tests/djinterp/cpp/functional/pipeline_tests_creation.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.11.09
*******************************************************************************/

// std
#include <list>
#include <utility>
#include <vector>
// djinterp
#include "./pipeline_tests.hpp"


NS_DJINTERP
NS_TESTING


using pi = DFunctionPipeline<int>;


bool test_creation()
{
    bool verdict = true;

    // default constructor -> empty, no error
    pi def;
    record_assertion(verdict,
        def.size() == 0 && def.empty() && !def.has_error(),
        "create: default constructor is empty and error-free");

    // from(container) -- generic lvalue container (std::list)
    std::list<int> lst = { 1, 2, 3 };
    pi from_list = pi::from(lst);
    record_assertion(verdict,
        from_list.to_vector() == std::vector<int>({ 1, 2, 3 }),
        "create: from(container) copies a generic container");

    // from(container) -- lvalue vector
    std::vector<int> vec = { 4, 5 };
    pi from_vec = pi::from(vec);
    record_assertion(verdict,
        from_vec.to_vector() == std::vector<int>({ 4, 5 }),
        "create: from(container) copies an lvalue vector");

    // from(vector&&) -- move overload
    pi from_moved = pi::from(std::vector<int>({ 6, 7, 8 }));
    record_assertion(verdict,
        from_moved.to_vector() == std::vector<int>({ 6, 7, 8 }),
        "create: from(vector&&) moves a vector in");

    // from(initializer_list)
    pi from_init = pi::from({ 9, 10 });
    record_assertion(verdict,
        from_init.to_vector() == std::vector<int>({ 9, 10 }),
        "create: from(initializer_list)");

    // from(raw array, count)
    const int arr[] = { 11, 12, 13 };
    pi from_arr = pi::from(arr, 3);
    record_assertion(verdict,
        from_arr.to_vector() == std::vector<int>({ 11, 12, 13 }),
        "create: from(pointer, count) copies a raw array");

    // of(variadic)
    pi from_of = pi::of(20, 21, 22, 23);
    record_assertion(verdict,
        from_of.to_vector() == std::vector<int>({ 20, 21, 22, 23 }),
        "create: of(variadic) builds from arguments");

    // error()
    pi err = pi::error(42);
    record_assertion(verdict,
        err.has_error() && err.error_code() == 42 && err.empty(),
        "create: error() yields an empty errored pipeline with the code");

    // error() default code
    pi err_default = pi::error();
    record_assertion(verdict,
        err_default.has_error() && err_default.error_code() == -1,
        "create: error() default code is -1");

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
