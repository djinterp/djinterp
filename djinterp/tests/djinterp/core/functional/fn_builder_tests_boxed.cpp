/******************************************************************************
* djinterp [test]                                    fn_builder_tests_boxed.cpp
*
* Type-erasure tests (section IV): boxed_fn_builder and box_builder.
*
*   test_boxed -- erase a typed builder into a single concrete
*                 boxed_fn_builder type (via direct construction and via
*                 box_builder), then execute it; including a type-changing
*                 chain and the operator() container overload.
******************************************************************************/

#include <vector>
#include <list>
#include <string>
#include "./fn_builder_tests.hpp"


NS_DJINTERP
NS_TESTING


void test_boxed(test::test_handler& _h)
{
    std::vector<int> in = { 1, 2, 3, 4 };

    // direct construction from a same-type chain (int -> int)
    auto                      typed = fn_builder<int>::create()
                                          .filter(fn_is_even{})
                                          .map(fn_double{});
    boxed_fn_builder<int, int> boxed(typed);
    test::record_assertion(_h,
        boxed.execute(in) == std::vector<int>({ 4, 8 }),
        "boxed: direct construction erases a same-type chain");

    // box_builder deduces input/output from the typed builder
    auto deduced = box_builder(typed);
    test::record_assertion(_h,
        deduced.execute(in) == std::vector<int>({ 4, 8 }),
        "boxed: box_builder deduces input/output and erases the chain");

    // chain() exposes the composed functor for direct invocation
    test::record_assertion(_h,
        typed.chain()(in) == std::vector<int>({ 4, 8 }),
        "boxed: chain() exposes the composed chain functor directly");

    // type-changing chain (int -> std::string)
    auto typed_str = fn_builder<int>::create().map(fn_to_string{});
    boxed_fn_builder<int, std::string> boxed_str = box_builder(typed_str);
    test::record_assertion(_h,
        boxed_str.execute(in)
            == std::vector<std::string>({ "1", "2", "3", "4" }),
        "boxed: erases a type-changing chain");

    // operator() container overload converts then runs
    std::list<int> lin = { 2, 4 };
    test::record_assertion(_h,
        deduced(lin) == std::vector<int>({ 4, 8 }),
        "boxed: operator() accepts a generic container");

    return;
}


NS_END  // testing
NS_END  // djinterp
