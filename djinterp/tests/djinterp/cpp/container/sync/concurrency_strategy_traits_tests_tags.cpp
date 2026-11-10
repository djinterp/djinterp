/*******************************************************************************
* djinterp [test]                     concurrency_strategy_traits_tests_tags.cpp
*
*   Section I/II - the strategy enum and the concurrency_strategy_tag alias
* detector.  Defines the "tags" block declared in the suite header.
*
*
* path:      /tests/djinterp/cpp/container/sync/concurrency_strategy_traits_tests_tags.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/

// FLOOR, FOR NOW: below C++17, concurrency_strategy_traits.hpp's floor,
// this unit is empty, as that header is.
#include "../../../../../inc/djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

#include "concurrency_strategy_traits_tests.hpp"

NS_DJINTERP
NS_TESTING

// tests_tags_enum_values
//   Verifies the DConcurrencyStrategy enumerators carry their documented
//   integer values, so the tag_to_strategy table and any serialisation agree
//   on the ordering.
//   Tests the following:
//   - none / locked / cow / rcu / atomic / hazard / hybrid == 0 .. 6
bool tests_tags_enum_values()
{
    D_CS_CHECK(static_cast<int>(DConcurrencyStrategy::none)   == 0);
    D_CS_CHECK(static_cast<int>(DConcurrencyStrategy::locked) == 1);
    D_CS_CHECK(static_cast<int>(DConcurrencyStrategy::cow)    == 2);
    D_CS_CHECK(static_cast<int>(DConcurrencyStrategy::rcu)    == 3);
    D_CS_CHECK(static_cast<int>(DConcurrencyStrategy::atomic) == 4);
    D_CS_CHECK(static_cast<int>(DConcurrencyStrategy::hazard) == 5);
    D_CS_CHECK(static_cast<int>(DConcurrencyStrategy::hybrid) == 6);
    return true;
}

// tests_tags_has_strategy_tag
//   Verifies has_concurrency_strategy_tag_v detects the presence of a
//   concurrency_strategy_tag alias and nothing else.
//   Tests the following:
//   - true for every tagged fixture (including the none tag)
//   - false for tag-less structural fixtures and the empty type
bool tests_tags_has_strategy_tag()
{
    D_CS_CHECK(has_concurrency_strategy_tag_v<DTagNone>);
    D_CS_CHECK(has_concurrency_strategy_tag_v<DTagLocked>);
    D_CS_CHECK(has_concurrency_strategy_tag_v<DTagCow>);
    D_CS_CHECK(has_concurrency_strategy_tag_v<DTagRcu>);
    D_CS_CHECK(has_concurrency_strategy_tag_v<DTagAtomic>);
    D_CS_CHECK(has_concurrency_strategy_tag_v<DTagHazard>);
    D_CS_CHECK(has_concurrency_strategy_tag_v<DTagHybrid>);

    D_CS_CHECK(!has_concurrency_strategy_tag_v<DStPlain>);
    D_CS_CHECK(!has_concurrency_strategy_tag_v<DStLockedRw>);
    D_CS_CHECK(!has_concurrency_strategy_tag_v<DStCowState>);
    D_CS_CHECK(!has_concurrency_strategy_tag_v<st_atomic_value>);
    return true;
}

NS_END  // testing
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
