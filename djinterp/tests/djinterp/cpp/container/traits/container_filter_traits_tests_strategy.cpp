/*******************************************************************************
* djinterp [testing]                  container_filter_traits_tests_strategy.cpp
*
*   Section II - the DFilterStrategy vocabulary and DContainerFilterStrategy,
* the strongest selection realization a type's iterability grants.  The
* precedence is exercised end to end: a non-source is `unsupported`; a source
* with its own primitive is `native`; a source that cannot build is `external`;
* and a filterable one is `indexed` (contiguous + random-access),
* `bidirectional`, or `forward` by the strongest category its iterator meets.
*
*   Two discriminations the standard containers make sharp:
*     - `indexed` requires BOTH data() and a random-access iterator, so a
*       std::deque (random-access, but no data()) is `bidirectional`, not
*       `indexed` - contiguity, not just index-ability, is the line.
*     - a std::unordered_set is forward-only, so it is `forward` even though it
*       is fully filterable.
*   `native` is confirmed to win ahead of `indexed` (DNativeFilterable), while a
*   wrong-signature filter() does not (DNativeWrongSig is `indexed`).
*
*
* path:      /tests/djinterp/cpp/container/traits/container_filter_traits_tests_strategy.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_filter_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_filter_strategy_name
//   the stable spellings, with a couple of non-collision guards.
bool tests_filter_strategy_name()
{
    bool _cf_ok = true;

    D_CF_CHECK( (cf_cstr_eq(filter_strategy_name(DFilterStrategy::native),        "native")) );
    D_CF_CHECK( (cf_cstr_eq(filter_strategy_name(DFilterStrategy::indexed),       "indexed")) );
    D_CF_CHECK( (cf_cstr_eq(filter_strategy_name(DFilterStrategy::bidirectional), "bidirectional")) );
    D_CF_CHECK( (cf_cstr_eq(filter_strategy_name(DFilterStrategy::forward),       "forward")) );
    D_CF_CHECK( (cf_cstr_eq(filter_strategy_name(DFilterStrategy::external),      "external")) );
    D_CF_CHECK( (cf_cstr_eq(filter_strategy_name(DFilterStrategy::unsupported),   "unsupported")) );

    D_CF_CHECK( (!cf_cstr_eq(filter_strategy_name(DFilterStrategy::forward),
                             "bidirectional")) );
    D_CF_CHECK( (!cf_cstr_eq(filter_strategy_name(DFilterStrategy::unsupported),
                             "external")) );

    return _cf_ok;
}


// tests_container_filter_strategy
//   the realization verdict across every branch.
bool tests_container_filter_strategy()
{
    bool _cf_ok = true;

    // native: the type's own primitive wins ahead of any iteration strategy
    D_CF_CHECK( (DContainerFilterStrategy<DNativeFilterable>::value
                     == DFilterStrategy::native) );

    // indexed: contiguous (data()) AND random-access
    D_CF_CHECK( (DContainerFilterStrategy<std::vector<int>>::value
                     == DFilterStrategy::indexed) );
    D_CF_CHECK( (DContainerFilterStrategy<std::string>::value
                     == DFilterStrategy::indexed) );
    D_CF_CHECK( (DContainerFilterStrategy<DSortedSeq>::value
                     == DFilterStrategy::indexed) );
    // a wrong-signature filter() does NOT trigger native -> falls through to indexed
    D_CF_CHECK( (DContainerFilterStrategy<DNativeWrongSig>::value
                     == DFilterStrategy::indexed) );

    // bidirectional: random-access WITHOUT data() (deque), or genuinely
    // bidirectional (list, and the tree-backed associatives)
    D_CF_CHECK( (DContainerFilterStrategy<std::deque<int>>::value
                     == DFilterStrategy::bidirectional) );
    D_CF_CHECK( (DContainerFilterStrategy<std::list<int>>::value
                     == DFilterStrategy::bidirectional) );
    D_CF_CHECK( (DContainerFilterStrategy<std::set<int>>::value
                     == DFilterStrategy::bidirectional) );
    D_CF_CHECK( (DContainerFilterStrategy<std::multiset<int>>::value
                     == DFilterStrategy::bidirectional) );
    D_CF_CHECK( (DContainerFilterStrategy<std::map<int, int>>::value
                     == DFilterStrategy::bidirectional) );

    // forward: a filterable but forward-only container
    D_CF_CHECK( (DContainerFilterStrategy<std::unordered_set<int>>::value
                     == DFilterStrategy::forward) );

    // external: a source that cannot build its own result
    D_CF_CHECK( (DContainerFilterStrategy<std::forward_list<int>>::value
                     == DFilterStrategy::external) );
    D_CF_CHECK( (DContainerFilterStrategy<std::array<int, 4>>::value
                     == DFilterStrategy::external) );

    // unsupported: not even a source
    D_CF_CHECK( (DContainerFilterStrategy<int>::value
                     == DFilterStrategy::unsupported) );
    D_CF_CHECK( (DContainerFilterStrategy<value_only>::value
                     == DFilterStrategy::unsupported) );
    D_CF_CHECK( (DContainerFilterStrategy<DIterableNoValue>::value
                     == DFilterStrategy::unsupported) );

    return _cf_ok;
}


NS_END  // testing
NS_END  // djinterp
