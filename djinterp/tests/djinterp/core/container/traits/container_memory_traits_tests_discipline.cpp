/*******************************************************************************
* djinterp [testing]                container_memory_traits_tests_discipline.cpp
*
*   Section II - container_memory_discipline, the resolution engine, exercised
* across all five disciplines and every step of the precedence:
*     1. explicit memory_strategy alias      -> discipline_of<alias>
*     2. allocator_type                       -> its declared discipline, else the
*                                                plain-allocator INDIVIDUAL default
*     3. inline aggregate (extent/tuple_size) -> none
*     4. bare out-of-line shape (reserve())   -> individual
*     5. otherwise                            -> unknown
*   The precedence itself is pinned: an alias that resolves to `unknown` still
* wins over an allocator that would give individual (alias_unknown_container),
* and an allocator wins over a static-storage signal (alloc_beats_static).  Every
* standard allocator-aware container lands on `individual` because a plain
* std::allocator advertises no discipline (discipline_of -> unknown -> the step-2
* default); std::array, allocator-free, is `none` via its inline signal.
*
*
* path:      /tests/djinterp/core/container/traits/container_memory_traits_tests_discipline.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_memory_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_container_memory_discipline
//   the resolved discipline across every branch and every enum value.
bool
tests_container_memory_discipline()
{
    bool _cm_ok = true;

    // -- individual: the plain-allocator default (step 2, discipline_of -> unknown) --
    D_CM_CHECK( (container_memory_discipline<std::vector<int>>::value        == memory_discipline::individual) );
    D_CM_CHECK( (container_memory_discipline<std::deque<int>>::value         == memory_discipline::individual) );
    D_CM_CHECK( (container_memory_discipline<std::list<int>>::value          == memory_discipline::individual) );
    D_CM_CHECK( (container_memory_discipline<std::forward_list<int>>::value  == memory_discipline::individual) );
    D_CM_CHECK( (container_memory_discipline<std::set<int>>::value           == memory_discipline::individual) );
    D_CM_CHECK( (container_memory_discipline<std::multiset<int>>::value      == memory_discipline::individual) );
    D_CM_CHECK( (container_memory_discipline<std::unordered_set<int>>::value == memory_discipline::individual) );
    D_CM_CHECK( (container_memory_discipline<std::map<int, int>>::value      == memory_discipline::individual) );
    D_CM_CHECK( (container_memory_discipline<std::string>::value            == memory_discipline::individual) );
    // individual via the bare out-of-line shape (step 4: reserve(), no allocator)
    D_CM_CHECK( (container_memory_discipline<reserve_only>::value            == memory_discipline::individual) );
    // individual via an explicit heap strategy (step 1 -> discipline_of -> individual)
    D_CM_CHECK( (container_memory_discipline<inconsistent_individual_static>::value == memory_discipline::individual) );
    // individual via allocator precedence over a static signal (step 2 beats step 3)
    D_CM_CHECK( (container_memory_discipline<alloc_beats_static>::value      == memory_discipline::individual) );

    // -- none: an inline aggregate (step 3) or a descriptive strategy (step 1) --
    D_CM_CHECK( (container_memory_discipline<std::array<int, 4>>::value      == memory_discipline::none) );  // tuple_size signal
    D_CM_CHECK( (container_memory_discipline<extent_aggregate>::value        == memory_discipline::none) );  // extent signal
    D_CM_CHECK( (container_memory_discipline<descriptive_via_strategy>::value == memory_discipline::none) );  // strategy allocates nothing

    // -- pooled: an explicit alias, or an allocator that declares the discipline --
    D_CM_CHECK( (container_memory_discipline<pooled_via_strategy>::value     == memory_discipline::pooled) );   // step 1
    D_CM_CHECK( (container_memory_discipline<pooled_via_allocator>::value    == memory_discipline::pooled) );   // step 2 declared

    // -- arena: an explicit alias to a monotone strategy --
    D_CM_CHECK( (container_memory_discipline<arena_via_strategy>::value      == memory_discipline::arena) );

    // -- unknown: not resolvable --
    D_CM_CHECK( (container_memory_discipline<int>::value                     == memory_discipline::unknown) );  // step 5
    // alias precedence: the alias resolves to unknown and wins over the allocator
    D_CM_CHECK( (container_memory_discipline<alias_unknown_container>::value == memory_discipline::unknown) );

    return _cm_ok;
}


// tests_container_memory_discipline_companions
//   the ::type carrier and the `_v` value agree with ::value.
bool
tests_container_memory_discipline_companions()
{
    bool _cm_ok = true;

    // the ::type carrier exposes the same value
    D_CM_CHECK( (container_memory_discipline_t<std::vector<int>>::value == memory_discipline::individual) );
    D_CM_CHECK( (container_memory_discipline_t<std::array<int, 4>>::value == memory_discipline::none) );
    D_CM_CHECK( (container_memory_discipline_t<pooled_via_strategy>::value == memory_discipline::pooled) );
    D_CM_CHECK( (container_memory_discipline_t<int>::value == memory_discipline::unknown) );

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // the _v companion mirrors ::value
    D_CM_CHECK( (container_memory_discipline_v<std::vector<int>>
                     == container_memory_discipline<std::vector<int>>::value) );
    D_CM_CHECK( (container_memory_discipline_v<arena_via_strategy>  == memory_discipline::arena) );
    D_CM_CHECK( (container_memory_discipline_v<pooled_via_allocator> == memory_discipline::pooled) );
    D_CM_CHECK( (container_memory_discipline_v<std::array<int, 4>>  == memory_discipline::none) );
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return _cm_ok;
}


NS_END  // testing
NS_END  // djinterp
