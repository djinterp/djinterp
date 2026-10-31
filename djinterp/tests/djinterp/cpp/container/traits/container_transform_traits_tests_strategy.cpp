/*******************************************************************************
* djinterp [testing]               container_transform_traits_tests_strategy.cpp
*
*   Section II - the mapping strategy, the strongest realization a type's
* iterability grants.  Priority: native (its own transform()) first, then a
* source-only container maps externally, then a transformable one by its iterator
* strength - indexed (contiguous: data() + random access), bidirectional, or
* forward; unsupported for a non-source.
*
*   The ordering matters and is checked: DNativeBox is random-access yet resolves
* to native (native wins); std::array has data() and random access yet resolves to
* external (the source-only test precedes the indexed test); std::deque is
* random-access but has NO data(), so it is bidirectional, not indexed; and
* std::unordered_set is the one standard container whose transformable iterator is
* forward-only.
*
*
* path:      /tests/djinterp/cpp/container/traits/container_transform_traits_tests_strategy.cpp
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
bool tests_transform_strategy_name()
{
    bool _tx_ok = true;

    D_TX_CHECK( (tx_cstr_eq(transform_strategy_name(DTransformStrategy::native),        "native")) );
    D_TX_CHECK( (tx_cstr_eq(transform_strategy_name(DTransformStrategy::indexed),       "indexed")) );
    D_TX_CHECK( (tx_cstr_eq(transform_strategy_name(DTransformStrategy::bidirectional), "bidirectional")) );
    D_TX_CHECK( (tx_cstr_eq(transform_strategy_name(DTransformStrategy::forward),       "forward")) );
    D_TX_CHECK( (tx_cstr_eq(transform_strategy_name(DTransformStrategy::external),      "external")) );
    D_TX_CHECK( (tx_cstr_eq(transform_strategy_name(DTransformStrategy::unsupported),   "unsupported")) );

    D_TX_CHECK( (!tx_cstr_eq(transform_strategy_name(DTransformStrategy::indexed),
                             "bidirectional")) );

    return _tx_ok;
}


// tests_container_transform_strategy
//   the resolved strategy across every branch (and every priority tie).
bool tests_container_transform_strategy()
{
    bool _tx_ok = true;

    // native: a transform() member wins even over random-access iteration
    D_TX_CHECK( (DContainerTransformStrategy<DNativeBox>::value == DTransformStrategy::native) );

    // indexed: contiguous store (data() + random access) AND transformable
    D_TX_CHECK( (DContainerTransformStrategy<std::vector<int>>::value == DTransformStrategy::indexed) );
    D_TX_CHECK( (DContainerTransformStrategy<std::string>::value      == DTransformStrategy::indexed) );

    // bidirectional: transformable, bidirectional, but NOT contiguous
    D_TX_CHECK( (DContainerTransformStrategy<std::deque<int>>::value  == DTransformStrategy::bidirectional) );  // RA but no data()
    D_TX_CHECK( (DContainerTransformStrategy<std::list<int>>::value   == DTransformStrategy::bidirectional) );
    D_TX_CHECK( (DContainerTransformStrategy<std::set<int>>::value    == DTransformStrategy::bidirectional) );
    D_TX_CHECK( (DContainerTransformStrategy<std::map<int, int>>::value == DTransformStrategy::bidirectional) );
    D_TX_CHECK( (DContainerTransformStrategy<Box<int>>::value         == DTransformStrategy::bidirectional) );

    // forward: transformable but forward-only iterator
    D_TX_CHECK( (DContainerTransformStrategy<std::unordered_set<int>>::value == DTransformStrategy::forward) );

    // external: a source that cannot build (even array, though contiguous + RA)
    D_TX_CHECK( (DContainerTransformStrategy<std::forward_list<int>>::value == DTransformStrategy::external) );
    D_TX_CHECK( (DContainerTransformStrategy<std::array<int, 4>>::value     == DTransformStrategy::external) );
    D_TX_CHECK( (DContainerTransformStrategy<DReadOnlySeq>::value          == DTransformStrategy::external) );

    // unsupported: not even a source
    D_TX_CHECK( (DContainerTransformStrategy<int>::value == DTransformStrategy::unsupported) );

    return _tx_ok;
}


NS_END  // testing
NS_END  // djinterp
