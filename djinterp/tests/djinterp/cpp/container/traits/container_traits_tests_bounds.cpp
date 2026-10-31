/*******************************************************************************
* djinterp [testing]                           container_traits_tests_bounds.cpp
*
*   Section VI - interval-aware bounds.  is_lower_bounded (a valid size_interval
* OR a positive min_size()); is_upper_bounded (a valid size_interval OR a C/std
* array OR a fixed size OR a bounded capacity); is_unbounded (neither); and
* is_interval_bounded (either), which is the exact complement of is_unbounded.
*
*   Deterministic coverage comes from the fixtures: only min5 is lower-bounded;
* the fixed / bounded-capacity / array fixtures are upper-bounded; DEmptyCt and
* DUnboundedHuge are unbounded.  std::array is upper-bounded; std::deque (not
* constexpr-sized) is unbounded on any standard; std::vector reads as
* upper-bounded under C++20 (constexpr max_size below SIZE_MAX/2).
*
*
* path:      /tests/djinterp/cpp/container/traits/container_traits_tests_bounds.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_is_lower_bounded
//   a positive min_size() (or a valid size_interval).
bool tests_is_lower_bounded()
{
    bool _ct_ok = true;

    D_CT_CHECK( (is_lower_bounded<DMin5>::value) );

    // no min_size / no valid interval -> not lower-bounded
    D_CT_CHECK( (!is_lower_bounded<DFixed4>::value) );
    D_CT_CHECK( (!is_lower_bounded<DBoundedCap>::value) );
    D_CT_CHECK( (!is_lower_bounded<DIntervalsBogus>::value) );   // bogus interval is rejected
    D_CT_CHECK( (!is_lower_bounded<std::array<int, 4>>::value) );
    D_CT_CHECK( (!is_lower_bounded<std::vector<int>>::value) );
    D_CT_CHECK( (!is_lower_bounded<int>::value) );

    return _ct_ok;
}


// tests_is_upper_bounded
//   an array, a fixed size, or a bounded capacity (or a valid size_interval).
bool tests_is_upper_bounded()
{
    bool _ct_ok = true;

    // arrays and fixed / bounded-capacity fixtures
    D_CT_CHECK( (is_upper_bounded<std::array<int, 4>>::value) );
    D_CT_CHECK( (is_upper_bounded<int[4]>::value) );
    D_CT_CHECK( (is_upper_bounded<DFixed4>::value) );
    D_CT_CHECK( (is_upper_bounded<DBoundedCap>::value) );
    D_CT_CHECK( (is_upper_bounded<DAllocBounded>::value) );

    // no upper signal -> not upper-bounded
    D_CT_CHECK( (!is_upper_bounded<DMin5>::value) );             // lower only
    D_CT_CHECK( (!is_upper_bounded<DEmptyCt>::value) );         // no max_size()
    D_CT_CHECK( (!is_upper_bounded<DUnboundedHuge>::value) );   // capacity not bounded
    D_CT_CHECK( (!is_upper_bounded<std::deque<int>>::value) );  // not constexpr-sized
    D_CT_CHECK( (!is_upper_bounded<int>::value) );

    return _ct_ok;
}


// tests_is_unbounded_and_interval_bounded
//   the two are exact complements over every type.
bool tests_is_unbounded_and_interval_bounded()
{
    bool _ct_ok = true;

    // unbounded: no lower and no upper signal
    D_CT_CHECK( (is_unbounded<DEmptyCt>::value) );
    D_CT_CHECK( (is_unbounded<DUnboundedHuge>::value) );
    D_CT_CHECK( (is_unbounded<std::deque<int>>::value) );
    D_CT_CHECK( (is_unbounded<std::list<int>>::value) );
    D_CT_CHECK( (is_unbounded<int>::value) );

    // NOT unbounded
    D_CT_CHECK( (!is_unbounded<DMin5>::value) );                 // lower
    D_CT_CHECK( (!is_unbounded<std::array<int, 4>>::value) );   // upper
    D_CT_CHECK( (!is_unbounded<DBoundedCap>::value) );          // upper

    // is_interval_bounded is the exact complement of is_unbounded
    D_CT_CHECK( (is_interval_bounded<DMin5>::value) );
    D_CT_CHECK( (is_interval_bounded<std::array<int, 4>>::value) );
    D_CT_CHECK( (is_interval_bounded<DBoundedCap>::value) );
    D_CT_CHECK( (!is_interval_bounded<DEmptyCt>::value) );
    D_CT_CHECK( (!is_interval_bounded<DUnboundedHuge>::value) );
    D_CT_CHECK( (!is_interval_bounded<std::deque<int>>::value) );
    D_CT_CHECK( (!is_interval_bounded<int>::value) );

    // spot-check the complement relation directly
    D_CT_CHECK( (is_interval_bounded<std::array<int, 4>>::value != is_unbounded<std::array<int, 4>>::value) );
    D_CT_CHECK( (is_interval_bounded<std::deque<int>>::value    != is_unbounded<std::deque<int>>::value) );

    return _ct_ok;
}


NS_END  // testing
NS_END  // djinterp
