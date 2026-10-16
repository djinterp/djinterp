/*******************************************************************************
* djinterp [testing]               container_transform_traits_tests_strategy.cpp
*
*   Section II - the mapping strategy, the strongest realization a type's
* iterability grants.  Priority: native (its own transform()) first, then a
* source-only container maps externally, then a transformable one by its iterator
* strength - indexed (contiguous: data() + random access), bidirectional, or
* forward; unsupported for a non-source.
*
*   The ordering matters and is checked: native_box is random-access yet resolves
* to native (native wins); std::array has data() and random access yet resolves to
* external (the source-only test precedes the indexed test); std::deque is
* random-access but has NO data(), so it is bidirectional, not indexed; and
* std::unordered_set is the one standard container whose transformable iterator is
* forward-only.
*
*
* path:      /tests/djinterp/core/container/traits/container_transform_traits_tests_strategy.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_transform_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_transform_strategy_name
//   the stable spellings.
bool
tests_transform_strategy_name()
{
    bool _tx_ok = true;

    D_TX_CHECK( (tx_cstr_eq(transform_strategy_name(transform_strategy::native),        "native")) );
    D_TX_CHECK( (tx_cstr_eq(transform_strategy_name(transform_strategy::indexed),       "indexed")) );
    D_TX_CHECK( (tx_cstr_eq(transform_strategy_name(transform_strategy::bidirectional), "bidirectional")) );
    D_TX_CHECK( (tx_cstr_eq(transform_strategy_name(transform_strategy::forward),       "forward")) );
    D_TX_CHECK( (tx_cstr_eq(transform_strategy_name(transform_strategy::external),      "external")) );
    D_TX_CHECK( (tx_cstr_eq(transform_strategy_name(transform_strategy::unsupported),   "unsupported")) );

    D_TX_CHECK( (!tx_cstr_eq(transform_strategy_name(transform_strategy::indexed),
                             "bidirectional")) );

    return _tx_ok;
}


// tests_container_transform_strategy
//   the resolved strategy across every branch (and every priority tie).
bool
tests_container_transform_strategy()
{
    bool _tx_ok = true;

    // native: a transform() member wins even over random-access iteration
    D_TX_CHECK( (container_transform_strategy<native_box>::value == transform_strategy::native) );

    // indexed: contiguous store (data() + random access) AND transformable
    D_TX_CHECK( (container_transform_strategy<std::vector<int>>::value == transform_strategy::indexed) );
    D_TX_CHECK( (container_transform_strategy<std::string>::value      == transform_strategy::indexed) );

    // bidirectional: transformable, bidirectional, but NOT contiguous
    D_TX_CHECK( (container_transform_strategy<std::deque<int>>::value  == transform_strategy::bidirectional) );  // RA but no data()
    D_TX_CHECK( (container_transform_strategy<std::list<int>>::value   == transform_strategy::bidirectional) );
    D_TX_CHECK( (container_transform_strategy<std::set<int>>::value    == transform_strategy::bidirectional) );
    D_TX_CHECK( (container_transform_strategy<std::map<int, int>>::value == transform_strategy::bidirectional) );
    D_TX_CHECK( (container_transform_strategy<Box<int>>::value         == transform_strategy::bidirectional) );

    // forward: transformable but forward-only iterator
    D_TX_CHECK( (container_transform_strategy<std::unordered_set<int>>::value == transform_strategy::forward) );

    // external: a source that cannot build (even array, though contiguous + RA)
    D_TX_CHECK( (container_transform_strategy<std::forward_list<int>>::value == transform_strategy::external) );
    D_TX_CHECK( (container_transform_strategy<std::array<int, 4>>::value     == transform_strategy::external) );
    D_TX_CHECK( (container_transform_strategy<read_only_seq>::value          == transform_strategy::external) );

    // unsupported: not even a source
    D_TX_CHECK( (container_transform_strategy<int>::value == transform_strategy::unsupported) );

    return _tx_ok;
}


NS_END  // testing
NS_END  // djinterp
