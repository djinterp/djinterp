/*******************************************************************************
* djinterp [test]                                  fn_builder_tests_creation.cpp
*
* Builder creation tests (sections i / III):
*   test_creation -- create(), make_builder(), the identity seed chain, the
*                    chain_type alias, and reuse of a composed builder across
*                    multiple inputs (composition is built once, executed many).
*
*
* path:      /tests/djinterp/cpp/functional/fn_builder_tests_creation.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <type_traits>
#include <vector>
// djinterp
#include "./fn_builder_tests.hpp"


NS_DJINTERP
NS_TESTING


void test_creation(
    test::test_handler& _h
)
{
    // create() seeds an identity builder: execute returns the input unchanged
    auto             b = DFnBuilder<int>::create();
    std::vector<int> in = { 1, 2, 3 };
    test::record_assertion(_h, b.execute(in) == in,
        "create: identity builder returns its input unchanged");

    // make_builder<T>() is equivalent to DFnBuilder<T>::create()
    auto mb = make_builder<int>();
    test::record_assertion(_h, mb.execute(in) == in,
        "create: make_builder<T> seeds an identity builder");

    // identity over an empty input -> empty
    test::record_assertion(_h,
        DFnBuilder<int>::create().execute(std::vector<int>()).empty(),
        "create: identity over empty input is empty");

    // chain_type alias is the seed's DIdentityChain
    test::record_assertion(_h,
        std::is_same<decltype(b)::chain_type,
                     internal::DIdentityChain<int> >::value,
        "create: chain_type names the identity seed chain");

    // a composed builder is reusable across multiple inputs
    auto doubler = DFnBuilder<int>::create().map(DFnDouble{});
    std::vector<int> a = { 1, 2 };
    std::vector<int> c = { 10 };
    bool reusable = doubler.execute(a) == std::vector<int>({ 2, 4 })
                 && doubler.execute(c) == std::vector<int>({ 20 });
    test::record_assertion(_h, reusable,
        "create: a composed builder can be executed on many inputs");

    return;
}


NS_END  // testing
NS_END  // djinterp
