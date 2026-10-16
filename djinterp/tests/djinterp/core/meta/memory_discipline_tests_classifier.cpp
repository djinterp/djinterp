/*******************************************************************************
* djinterp [core]                         memory_discipline_tests_classifier.cpp
*
*
* path:      /tests/djinterp/core/meta/memory_discipline_tests_classifier.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <type_traits>
// djinterp
#include "memory_discipline_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_memory_discipline_classify_unknown_none
  the first two classifier gates: not-a-strategy, and descriptive-only
  Tests the following:
  - a type without strategy_storage_kind classifies unknown
  - a type with the marker but no allocate surface classifies none
*/
bool
tests_memory_discipline_classify_unknown_none()
{
    D_MD_CHECK(discipline_of<md_unknown>::value == memory_discipline::unknown);
    D_MD_CHECK(discipline_of<md_none>::value == memory_discipline::none);

    return true;
}

/*
tests_memory_discipline_classify_arena
  an allocating strategy with no individual release classifies arena
  Tests the following:
  - a strategy with allocate but no supports_individual_release constant is arena
  - a strategy with allocate and supports_individual_release = false is arena
  - (both the absent and the explicit-false release paths reach arena)
*/
bool
tests_memory_discipline_classify_arena()
{
    D_MD_CHECK(discipline_of<md_arena_absent>::value == memory_discipline::arena);
    D_MD_CHECK(discipline_of<md_arena_false>::value == memory_discipline::arena);

    return true;
}

/*
tests_memory_discipline_classify_pooled_individual
  an individual-release strategy splits on pointer stability
  Tests the following:
  - allocate + individual-release + pointer_stable = true classifies pooled
  - allocate + individual-release + no pointer_stable constant classifies individual
  - allocate + individual-release + pointer_stable = false classifies individual
  - (both the absent and the explicit-false stability paths reach individual)
*/
bool
tests_memory_discipline_classify_pooled_individual()
{
    D_MD_CHECK(discipline_of<md_pooled>::value == memory_discipline::pooled);
    D_MD_CHECK(discipline_of<md_individual_absent>::value == memory_discipline::individual);
    D_MD_CHECK(discipline_of<md_individual_false>::value == memory_discipline::individual);

    return true;
}

/*
tests_memory_discipline_classify_allocate_isolation
  either allocate shape counts as "allocates", so neither is misread as none
  Tests the following:
  - a byte-only allocate(size_t, size_t) strategy is NOT none (classifies arena)
  - an element-only allocate(size_t) strategy is NOT none (classifies arena)
  - a strategy with NEITHER allocate shape IS none
  - (the byte and element branches of the allocate gate are each decisive)
*/
bool
tests_memory_discipline_classify_allocate_isolation()
{
    // byte-allocate alone -> allocates -> (no release) arena
    D_MD_CHECK(discipline_of<md_byte_alloc>::value == memory_discipline::arena);
    D_MD_CHECK(discipline_of<md_byte_alloc>::value != memory_discipline::none);

    // element-allocate alone -> allocates -> (no release) arena
    D_MD_CHECK(discipline_of<md_arena_absent>::value == memory_discipline::arena);

    // neither allocate shape -> none
    D_MD_CHECK(discipline_of<md_none>::value == memory_discipline::none);

    return true;
}

/*
tests_memory_discipline_classify_aliases_and_v
  the _t carrier and _v value track discipline_of, and cv/ref is stripped
  Tests the following:
  - discipline_of_t<T> is the carrier for the classified value
  - discipline_of strips cv/ref (const T& classifies as T does)
  - discipline_of_t<T>::value equals discipline_of<T>::value
  - discipline_of_v<T> equals discipline_of<T>::value (where the companion exists)
*/
bool
tests_memory_discipline_classify_aliases_and_v()
{
    // _t is the matching carrier
    D_MD_CHECK((std::is_same<discipline_of_t<md_pooled>, pooled_discipline>::value));
    D_MD_CHECK((std::is_same<discipline_of_t<md_none>, none_discipline>::value));
    D_MD_CHECK((std::is_same<discipline_of_t<md_arena_absent>, arena_discipline>::value));

    // cv/ref stripping in the classifier
    D_MD_CHECK(discipline_of<const md_pooled&>::value == memory_discipline::pooled);
    D_MD_CHECK(discipline_of<md_individual_absent&>::value == memory_discipline::individual);

    // _t carrier value agrees with ::value
    D_MD_CHECK(discipline_of_t<md_pooled>::value == discipline_of<md_pooled>::value);

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES || D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // the _v companion, where the language provides variable templates
    D_MD_CHECK(discipline_of_v<md_unknown> == memory_discipline::unknown);
    D_MD_CHECK(discipline_of_v<md_none> == memory_discipline::none);
    D_MD_CHECK(discipline_of_v<md_arena_absent> == memory_discipline::arena);
    D_MD_CHECK(discipline_of_v<md_pooled> == memory_discipline::pooled);
    D_MD_CHECK(discipline_of_v<md_individual_absent> == memory_discipline::individual);
#endif

    return true;
}

NS_END  // testing
NS_END  // djinterp
