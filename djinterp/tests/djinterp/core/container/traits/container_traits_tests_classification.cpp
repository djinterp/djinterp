/*******************************************************************************
* djinterp [testing]                   container_traits_tests_classification.cpp
*
*   Section XVI - container_class, the combined snapshot uniting this header's
* own axes with the re-exported ones.  These checks assert, member-for-member,
* the axes this header OWNS (lifetime via is_compile_time, the interval-aware
* bounds, the legacy storage split, uniqueness/multiplicity, and underlying vs
* fundamental) across a representative spread, plus has_reverse_iteration (an
* rbegin/rend fact) as a witness that the re-exported members are wired through.
* The remaining re-exported members (immutable/mutable/runtime, the iteration and
* ordering and sorted axes) are validated by their own trait suites.
*
*   Highlights: std::array is upper-bounded static-storage; std::set enforces
* uniqueness and is unbounded; empty_ct is simultaneously a compile-time container
* AND dynamic storage (the axes are independent); adaptor is underlying, not
* fundamental; and std::forward_list is neither static nor dynamic storage and
* has no reverse iteration.  A couple of std::vector bounds are C++20 readings.
*
*
* path:      /tests/djinterp/core/container/traits/container_traits_tests_classification.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_container_class
//   the aggregate, one representative per shape (owned members + a reverse witness).
bool
tests_container_class()
{
    bool _ct_ok = true;

    // -- std::array<int,4>: upper-bounded, static storage, fundamental --
    {
        using C = container_class<std::array<int, 4>>;
        D_CT_CHECK( (!C::is_compile_time) );
        D_CT_CHECK( (C::is_upper_bounded && !C::is_lower_bounded) );
        D_CT_CHECK( (!C::is_unbounded && C::is_bounded) );
        D_CT_CHECK( (C::is_static_storage && !C::is_dynamic_storage) );
        D_CT_CHECK( (!C::enforces_uniqueness && C::allows_duplicates) );
        D_CT_CHECK( (C::is_fundamental && !C::is_underlying) );
        D_CT_CHECK( (!C::has_size_interval && !C::has_multiplicity_interval && !C::has_bounded_multiplicity) );
        D_CT_CHECK( (C::has_reverse_iteration) );
    }

    // -- std::vector<int>: dynamic storage, fundamental (bounds are C++20) --
    {
        using C = container_class<std::vector<int>>;
        D_CT_CHECK( (!C::is_compile_time) );
        D_CT_CHECK( (!C::is_static_storage && C::is_dynamic_storage) );
        D_CT_CHECK( (C::is_fundamental && !C::is_underlying) );
        D_CT_CHECK( (!C::enforces_uniqueness && C::allows_duplicates) );
        D_CT_CHECK( (C::has_reverse_iteration) );
        D_CT_CHECK( (C::is_upper_bounded && !C::is_unbounded) );   // C++20: constexpr max_size < SIZE_MAX/2
    }

    // -- std::set<int>: enforces uniqueness, unbounded, dynamic --
    {
        using C = container_class<std::set<int>>;
        D_CT_CHECK( (C::enforces_uniqueness && !C::allows_duplicates) );
        D_CT_CHECK( (C::is_unbounded && !C::is_bounded) );
        D_CT_CHECK( (!C::is_static_storage && C::is_dynamic_storage) );
        D_CT_CHECK( (C::is_fundamental && !C::is_underlying) );
        D_CT_CHECK( (C::has_reverse_iteration) );
    }

    // -- empty_ct: compile-time container AND dynamic storage (axes independent) --
    {
        using C = container_class<empty_ct>;
        D_CT_CHECK( (C::is_compile_time) );
        D_CT_CHECK( (C::is_dynamic_storage && !C::is_static_storage) );
        D_CT_CHECK( (C::is_unbounded) );
        D_CT_CHECK( (C::is_fundamental) );
        D_CT_CHECK( (C::allows_duplicates && !C::enforces_uniqueness) );
        D_CT_CHECK( (!C::has_reverse_iteration) );
    }

    // -- adaptor: underlying, NOT fundamental --
    {
        using C = container_class<adaptor>;
        D_CT_CHECK( (C::is_underlying && !C::is_fundamental) );
        D_CT_CHECK( (C::is_dynamic_storage) );
        D_CT_CHECK( (C::is_unbounded) );
        D_CT_CHECK( (C::allows_duplicates) );
    }

    // -- std::forward_list<int>: neither static nor dynamic; no reverse iteration --
    {
        using C = container_class<std::forward_list<int>>;
        D_CT_CHECK( (!C::is_static_storage && !C::is_dynamic_storage) );  // no size()
        D_CT_CHECK( (!C::has_reverse_iteration) );
        D_CT_CHECK( (C::is_unbounded) );
        D_CT_CHECK( (C::is_fundamental) );
        D_CT_CHECK( (C::allows_duplicates && !C::enforces_uniqueness) );
    }

    return _ct_ok;
}


// tests_container_variable_templates
//   the `_v` companions agree with their `::value` forms.  Gated on inline-
//   variable support exactly as the header emits them.
bool
tests_container_variable_templates()
{
    bool _ct_ok = true;

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES

    // method-detection companions
    D_CT_CHECK( (has_size_accessor_v<std::vector<int>> == has_size_accessor<std::vector<int>>::value) );
    D_CT_CHECK( (has_data_method_v<std::vector<int>>) );
    D_CT_CHECK( (!has_size_accessor_v<std::forward_list<int>>) );

    // constexpr / shape companions
    D_CT_CHECK( (has_constexpr_size_v<std::array<int, 4>> == has_constexpr_size<std::array<int, 4>>::value) );
    D_CT_CHECK( (has_fixed_size_v<fixed4>) );
    D_CT_CHECK( (has_bounded_capacity_v<bounded_cap>) );
    D_CT_CHECK( (!has_bounded_minimum_v<fixed4>) );

    // array companions
    D_CT_CHECK( (is_c_array_v<int[4]> == is_c_array<int[4]>::value) );
    D_CT_CHECK( (is_std_array_v<std::array<int, 4>>) );

    // lifetime companions
    D_CT_CHECK( (is_compile_time_container_v<empty_ct> == is_compile_time_container<empty_ct>::value) );
    D_CT_CHECK( (is_not_compile_time_container_v<std::vector<int>>) );

    // bounds companions
    D_CT_CHECK( (is_lower_bounded_v<min5>) );
    D_CT_CHECK( (is_upper_bounded_v<std::array<int, 4>>) );
    D_CT_CHECK( (is_unbounded_v<std::deque<int>> == is_unbounded<std::deque<int>>::value) );
    D_CT_CHECK( (is_interval_bounded_v<std::array<int, 4>>) );

    // storage companions
    D_CT_CHECK( (is_static_storage_v<fixed4> == is_static_storage<fixed4>::value) );
    D_CT_CHECK( (is_dynamic_storage_v<std::vector<int>>) );

    // uniqueness / multiplicity companions
    D_CT_CHECK( (enforces_uniqueness_v<std::set<int>>) );
    D_CT_CHECK( (allows_duplicates_v<std::vector<int>>) );
    D_CT_CHECK( (has_bounded_multiplicity_v<mult_accessors>) );

    // underlying companions
    D_CT_CHECK( (is_underlying_container_v<adaptor>) );
    D_CT_CHECK( (is_fundamental_container_v<std::vector<int>>) );

    // interval-validity companion
    D_CT_CHECK( (!has_valid_size_interval_v<intervals_bogus>) );

#endif  // D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES

    return _ct_ok;
}


NS_END  // testing
NS_END  // djinterp
