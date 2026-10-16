/*******************************************************************************
* djinterp [testing]                  container_storage_traits_tests_signals.cpp
*
*   Section I - the two raw shape signals: has_static_storage_signal (a member
* `extent` OR a std::tuple_size specialisation) and has_dynamic_storage_signal
* (an allocator_type OR a reserve()).  Each is a disjunction, so both disjuncts
* are exercised in isolation: the static signal via tuple_size (array / pair /
* tuple) and via extent (extent_only); the dynamic signal via an allocator
* (std::set - allocator, no reserve) and via reserve alone (reserve_only).
*
*   These are the STRUCTURAL signals, read independently of any opt-in - which is
* why optin_dynamic_override (a static `extent`, no allocator/reserve) raises the
* STATIC signal and not the dynamic one, even though its resolved siting is
* dynamic by opt-in (Section II).
*
*
* path:      /tests/djinterp/core/container/traits/container_storage_traits_tests_signals.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_storage_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_has_static_storage_signal
//   an inline compile-time-fixed aggregate: extent OR tuple_size.
bool
tests_has_static_storage_signal()
{
    bool _stg_ok = true;

    // tuple_size route
    D_STG_CHECK( (has_static_storage_signal<std::array<int, 4>>::value) );
    D_STG_CHECK( (has_static_storage_signal<std::pair<int, int>>::value) );
    D_STG_CHECK( (has_static_storage_signal<std::tuple<int, char>>::value) );
    // extent route
    D_STG_CHECK( (has_static_storage_signal<extent_only>::value) );
    D_STG_CHECK( (has_static_storage_signal<hybrid_shape>::value) );
    // present independently of the opt-in (these also carry an extent)
    D_STG_CHECK( (has_static_storage_signal<optin_dynamic_override>::value) );
    D_STG_CHECK( (has_static_storage_signal<optin_hybrid>::value) );

    // allocator-backed / growable containers raise no static signal
    D_STG_CHECK( (!has_static_storage_signal<std::vector<int>>::value) );
    D_STG_CHECK( (!has_static_storage_signal<std::set<int>>::value) );
    D_STG_CHECK( (!has_static_storage_signal<std::string>::value) );
    D_STG_CHECK( (!has_static_storage_signal<reserve_only>::value) );
    D_STG_CHECK( (!has_static_storage_signal<optin_static_no_size>::value) );  // opt-in only, no shape
    D_STG_CHECK( (!has_static_storage_signal<int>::value) );
    D_STG_CHECK( (!has_static_storage_signal<no_signal>::value) );

    return _stg_ok;
}


// tests_has_dynamic_storage_signal
//   out-of-line storage: an allocator_type OR a reserve().
bool
tests_has_dynamic_storage_signal()
{
    bool _stg_ok = true;

    // allocator route (std::set has an allocator but no reserve)
    D_STG_CHECK( (has_dynamic_storage_signal<std::set<int>>::value) );
    D_STG_CHECK( (has_dynamic_storage_signal<std::list<int>>::value) );
    D_STG_CHECK( (has_dynamic_storage_signal<std::map<int, int>>::value) );
    // allocator + reserve
    D_STG_CHECK( (has_dynamic_storage_signal<std::vector<int>>::value) );
    D_STG_CHECK( (has_dynamic_storage_signal<std::string>::value) );
    // reserve route alone (no allocator)
    D_STG_CHECK( (has_dynamic_storage_signal<reserve_only>::value) );
    D_STG_CHECK( (has_dynamic_storage_signal<hybrid_shape>::value) );

    // inline aggregates raise no dynamic signal
    D_STG_CHECK( (!has_dynamic_storage_signal<std::array<int, 4>>::value) );
    D_STG_CHECK( (!has_dynamic_storage_signal<std::pair<int, int>>::value) );
    D_STG_CHECK( (!has_dynamic_storage_signal<extent_only>::value) );
    // opt-in siting does NOT synthesise a dynamic SIGNAL (these carry only extent)
    D_STG_CHECK( (!has_dynamic_storage_signal<optin_dynamic_override>::value) );
    D_STG_CHECK( (!has_dynamic_storage_signal<optin_hybrid>::value) );
    D_STG_CHECK( (!has_dynamic_storage_signal<optin_static_no_size>::value) );
    D_STG_CHECK( (!has_dynamic_storage_signal<int>::value) );
    D_STG_CHECK( (!has_dynamic_storage_signal<no_signal>::value) );

    return _stg_ok;
}


NS_END  // testing
NS_END  // djinterp
