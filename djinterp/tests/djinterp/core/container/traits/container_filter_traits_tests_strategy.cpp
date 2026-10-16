/*******************************************************************************
* djinterp [testing]                  container_filter_traits_tests_strategy.cpp
*
*   Section II - the filter_strategy vocabulary and container_filter_strategy,
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
*   `native` is confirmed to win ahead of `indexed` (native_filterable), while a
*   wrong-signature filter() does not (native_wrong_sig is `indexed`).
*
*
* path:      /tests/djinterp/core/container/traits/container_filter_traits_tests_strategy.cpp
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
bool
tests_filter_strategy_name()
{
    bool _cf_ok = true;

    D_CF_CHECK( (cf_cstr_eq(filter_strategy_name(filter_strategy::native),        "native")) );
    D_CF_CHECK( (cf_cstr_eq(filter_strategy_name(filter_strategy::indexed),       "indexed")) );
    D_CF_CHECK( (cf_cstr_eq(filter_strategy_name(filter_strategy::bidirectional), "bidirectional")) );
    D_CF_CHECK( (cf_cstr_eq(filter_strategy_name(filter_strategy::forward),       "forward")) );
    D_CF_CHECK( (cf_cstr_eq(filter_strategy_name(filter_strategy::external),      "external")) );
    D_CF_CHECK( (cf_cstr_eq(filter_strategy_name(filter_strategy::unsupported),   "unsupported")) );

    D_CF_CHECK( (!cf_cstr_eq(filter_strategy_name(filter_strategy::forward),
                             "bidirectional")) );
    D_CF_CHECK( (!cf_cstr_eq(filter_strategy_name(filter_strategy::unsupported),
                             "external")) );

    return _cf_ok;
}


// tests_container_filter_strategy
//   the realization verdict across every branch.
bool
tests_container_filter_strategy()
{
    bool _cf_ok = true;

    // native: the type's own primitive wins ahead of any iteration strategy
    D_CF_CHECK( (container_filter_strategy<native_filterable>::value
                     == filter_strategy::native) );

    // indexed: contiguous (data()) AND random-access
    D_CF_CHECK( (container_filter_strategy<std::vector<int>>::value
                     == filter_strategy::indexed) );
    D_CF_CHECK( (container_filter_strategy<std::string>::value
                     == filter_strategy::indexed) );
    D_CF_CHECK( (container_filter_strategy<sorted_seq>::value
                     == filter_strategy::indexed) );
    // a wrong-signature filter() does NOT trigger native -> falls through to indexed
    D_CF_CHECK( (container_filter_strategy<native_wrong_sig>::value
                     == filter_strategy::indexed) );

    // bidirectional: random-access WITHOUT data() (deque), or genuinely
    // bidirectional (list, and the tree-backed associatives)
    D_CF_CHECK( (container_filter_strategy<std::deque<int>>::value
                     == filter_strategy::bidirectional) );
    D_CF_CHECK( (container_filter_strategy<std::list<int>>::value
                     == filter_strategy::bidirectional) );
    D_CF_CHECK( (container_filter_strategy<std::set<int>>::value
                     == filter_strategy::bidirectional) );
    D_CF_CHECK( (container_filter_strategy<std::multiset<int>>::value
                     == filter_strategy::bidirectional) );
    D_CF_CHECK( (container_filter_strategy<std::map<int, int>>::value
                     == filter_strategy::bidirectional) );

    // forward: a filterable but forward-only container
    D_CF_CHECK( (container_filter_strategy<std::unordered_set<int>>::value
                     == filter_strategy::forward) );

    // external: a source that cannot build its own result
    D_CF_CHECK( (container_filter_strategy<std::forward_list<int>>::value
                     == filter_strategy::external) );
    D_CF_CHECK( (container_filter_strategy<std::array<int, 4>>::value
                     == filter_strategy::external) );

    // unsupported: not even a source
    D_CF_CHECK( (container_filter_strategy<int>::value
                     == filter_strategy::unsupported) );
    D_CF_CHECK( (container_filter_strategy<value_only>::value
                     == filter_strategy::unsupported) );
    D_CF_CHECK( (container_filter_strategy<iterable_no_value>::value
                     == filter_strategy::unsupported) );

    return _cf_ok;
}


NS_END  // testing
NS_END  // djinterp
