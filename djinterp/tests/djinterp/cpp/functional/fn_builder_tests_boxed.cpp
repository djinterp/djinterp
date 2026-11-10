/*******************************************************************************
* djinterp [test]                                     fn_builder_tests_boxed.cpp
*
* Type-erasure tests (section IV): DBoxedFnBuilder and box_builder.
*
*   test_boxed -- erase a typed builder into a single concrete
*                 DBoxedFnBuilder type (via direct construction and via
*                 box_builder), then execute it; including a type-changing
*                 chain and the operator() container overload.
*
*
* path:      /tests/djinterp/cpp/functional/fn_builder_tests_boxed.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.11.09
*******************************************************************************/

// std
#include <list>
#include <string>
#include <vector>
// djinterp
#include "./fn_builder_tests.hpp"


NS_DJINTERP
NS_TESTING


bool test_boxed()
{
    bool verdict = true;

    std::vector<int> in = { 1, 2, 3, 4 };

    // direct construction from a same-type chain (int -> int)
    auto                      typed = DFnBuilder<int>::create()
                                          .filter(fn_is_even{})
                                          .map(DFnDouble{});
    DBoxedFnBuilder<int, int> boxed(typed);
    record_assertion(verdict,
        boxed.execute(in) == std::vector<int>({ 4, 8 }),
        "boxed: direct construction erases a same-type chain");

    // box_builder deduces input/output from the typed builder
    auto deduced = box_builder(typed);
    record_assertion(verdict,
        deduced.execute(in) == std::vector<int>({ 4, 8 }),
        "boxed: box_builder deduces input/output and erases the chain");

    // chain() exposes the composed functor for direct invocation
    record_assertion(verdict,
        typed.chain()(in) == std::vector<int>({ 4, 8 }),
        "boxed: chain() exposes the composed chain functor directly");

    // type-changing chain (int -> std::string)
    auto typed_str = DFnBuilder<int>::create().map(fn_to_string{});
    DBoxedFnBuilder<int, std::string> boxed_str = box_builder(typed_str);
    record_assertion(verdict,
        boxed_str.execute(in)
            == std::vector<std::string>({ "1", "2", "3", "4" }),
        "boxed: erases a type-changing chain");

    // operator() container overload converts then runs
    std::list<int> lin = { 2, 4 };
    record_assertion(verdict,
        deduced(lin) == std::vector<int>({ 4, 8 }),
        "boxed: operator() accepts a generic container");

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
