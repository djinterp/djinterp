/*******************************************************************************
* djinterp [testing]                 container_traits_tests_constexpr_traits.cpp
*
*   Section III - the constexpr-evaluated shape traits: has_constexpr_size /
* _max_size / _min_size (is `T{}.size()` etc. a constant expression?),
* has_fixed_size (size()==max_size()), has_bounded_capacity (max_size() <
* SIZE_MAX/2), has_bounded_minimum (min_size() > 0), and the interval-validity
* traits.
*
*   Because "is T{}.size() a constant expression?" is standard/stdlib-dependent
* for std:: containers (std::vector is constexpr-sized in C++20, not C++17), the
* deterministic branch coverage rests on the custom fixtures, whose explicit
* constexpr members fix the answer on any toolchain.  std::array (always
* constexpr-sized) and std::deque (never) anchor the two stable std:: ends; a
* couple of C++20-specific std readings are noted where included.
*
*
* path:      /tests/djinterp/core/container/traits/container_traits_tests_constexpr_traits.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_constexpr_size_traits
//   has_constexpr_size / has_constexpr_max_size / has_constexpr_min_size.
bool
tests_constexpr_size_traits()
{
    bool _ct_ok = true;

    // constexpr size(): the fixtures all expose one; std::array does; std::deque
    // does not (not constexpr-constructible)
    D_CT_CHECK( (has_constexpr_size<empty_ct>::value) );
    D_CT_CHECK( (has_constexpr_size<fixed4>::value) );
    D_CT_CHECK( (has_constexpr_size<min5>::value) );
    D_CT_CHECK( (has_constexpr_size<unbounded_huge>::value) );
    D_CT_CHECK( (has_constexpr_size<std::array<int, 4>>::value) );
    D_CT_CHECK( (!has_constexpr_size<std::deque<int>>::value) );
    D_CT_CHECK( (!has_constexpr_size<std::list<int>>::value) );
    D_CT_CHECK( (!has_constexpr_size<int>::value) );          // no size()
    D_CT_CHECK( (has_constexpr_size<std::vector<int>>::value) );  // C++20: vector is constexpr-sized

    // constexpr max_size(): fixtures WITH a max_size(); not empty_ct / min5
    D_CT_CHECK( (has_constexpr_max_size<fixed4>::value) );
    D_CT_CHECK( (has_constexpr_max_size<bounded_cap>::value) );
    D_CT_CHECK( (has_constexpr_max_size<unbounded_huge>::value) );
    D_CT_CHECK( (has_constexpr_max_size<std::array<int, 4>>::value) );
    D_CT_CHECK( (!has_constexpr_max_size<empty_ct>::value) );   // no max_size()
    D_CT_CHECK( (!has_constexpr_max_size<min5>::value) );       // no max_size()
    D_CT_CHECK( (!has_constexpr_max_size<std::deque<int>>::value) );

    // constexpr min_size(): only the min5 fixture (no std container has min_size)
    D_CT_CHECK( (has_constexpr_min_size<min5>::value) );
    D_CT_CHECK( (!has_constexpr_min_size<fixed4>::value) );
    D_CT_CHECK( (!has_constexpr_min_size<std::array<int, 4>>::value) );
    D_CT_CHECK( (!has_constexpr_min_size<std::vector<int>>::value) );

    return _ct_ok;
}


// tests_size_shape_traits
//   has_fixed_size / has_bounded_capacity / has_bounded_minimum.
bool
tests_size_shape_traits()
{
    bool _ct_ok = true;

    // fixed size: size() == max_size() (both constexpr)
    D_CT_CHECK( (has_fixed_size<std::array<int, 4>>::value) );  // 4 == 4
    D_CT_CHECK( (has_fixed_size<fixed4>::value) );              // 4 == 4
    D_CT_CHECK( (!has_fixed_size<bounded_cap>::value) );        // 0 != 100
    D_CT_CHECK( (!has_fixed_size<unbounded_huge>::value) );     // 0 != SIZE_MAX
    D_CT_CHECK( (!has_fixed_size<empty_ct>::value) );           // no max_size()
    D_CT_CHECK( (!has_fixed_size<std::vector<int>>::value) );   // 0 != max_size
    D_CT_CHECK( (!has_fixed_size<std::deque<int>>::value) );

    // bounded capacity: max_size() < SIZE_MAX/2
    D_CT_CHECK( (has_bounded_capacity<std::array<int, 4>>::value) );  // 4
    D_CT_CHECK( (has_bounded_capacity<fixed4>::value) );             // 4
    D_CT_CHECK( (has_bounded_capacity<bounded_cap>::value) );        // 100
    D_CT_CHECK( (has_bounded_capacity<alloc_bounded>::value) );      // 100
    D_CT_CHECK( (!has_bounded_capacity<unbounded_huge>::value) );    // SIZE_MAX
    D_CT_CHECK( (!has_bounded_capacity<empty_ct>::value) );          // no max_size()
    D_CT_CHECK( (!has_bounded_capacity<std::deque<int>>::value) );

    // bounded minimum: min_size() > 0
    D_CT_CHECK( (has_bounded_minimum<min5>::value) );
    D_CT_CHECK( (!has_bounded_minimum<fixed4>::value) );            // no min_size()
    D_CT_CHECK( (!has_bounded_minimum<std::vector<int>>::value) );
    D_CT_CHECK( (!has_bounded_minimum<std::array<int, 4>>::value) );

    return _ct_ok;
}


// tests_interval_validity
//   has_valid_size / depth / multiplicity_interval - a declared alias that is
//   NOT a math interval is rejected (int is no interval), and absent aliases
//   are false.  (The positive branch needs math/interval, unavailable here.)
bool
tests_interval_validity()
{
    bool _ct_ok = true;

    // intervals_bogus declares the aliases, but they are `int` - not intervals,
    // so the validity check rejects them
    D_CT_CHECK( (!has_valid_size_interval<intervals_bogus>::value) );
    D_CT_CHECK( (!has_valid_depth_interval<intervals_bogus>::value) );
    D_CT_CHECK( (!has_valid_multiplicity_interval<intervals_bogus>::value) );

    // no interval alias at all -> false
    D_CT_CHECK( (!has_valid_size_interval<std::vector<int>>::value) );
    D_CT_CHECK( (!has_valid_depth_interval<std::array<int, 4>>::value) );
    D_CT_CHECK( (!has_valid_multiplicity_interval<std::set<int>>::value) );
    D_CT_CHECK( (!has_valid_size_interval<int>::value) );

    return _ct_ok;
}


NS_END  // testing
NS_END  // djinterp
