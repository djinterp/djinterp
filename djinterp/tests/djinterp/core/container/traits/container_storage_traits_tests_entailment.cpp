/*******************************************************************************
* djinterp [testing]               container_storage_traits_tests_entailment.cpp
*
*   Section IV - static_storage_implies_constexpr_size, the per-type form of the
* formal entailment site(c) = stat => stage(|c|) = c: an inline (static) footprint
* forces a compile-time-expressible SIZE.  It holds iff the siting has no static
* component OR the size is compile-time - so it is vacuously true for dynamic /
* unknown containers and, for SHAPE-inferred static/hybrid containers, true BY
* CONSTRUCTION (the same extent / tuple_size that marks static storage also makes
* the size compile-time).
*
*   The edge the header's "true for every type" note quietly assumes away: an
* OPT-IN can break it.  optin_static_no_size declares static storage yet carries
* no compile-time-size signal, so its size lifetime is runtime and the entailment
* FAILS - the one type here for which this trait is false.
*
*
* path:      /tests/djinterp/core/container/traits/container_storage_traits_tests_entailment.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_storage_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_static_storage_implies_constexpr_size
//   the siting<->size coherence check, true everywhere EXCEPT an opt-in that
//   declares static storage without a compile-time size.
bool
tests_static_storage_implies_constexpr_size()
{
    bool _stg_ok = true;

    // shape-inferred static/hybrid: the static signal IS a compile-time-size
    // signal, so coherence holds by construction
    D_STG_CHECK( (static_storage_implies_constexpr_size<std::array<int, 4>>::value) );
    D_STG_CHECK( (static_storage_implies_constexpr_size<std::pair<int, int>>::value) );
    D_STG_CHECK( (static_storage_implies_constexpr_size<std::tuple<int, char>>::value) );
    D_STG_CHECK( (static_storage_implies_constexpr_size<extent_only>::value) );
    D_STG_CHECK( (static_storage_implies_constexpr_size<hybrid_shape>::value) );
    D_STG_CHECK( (static_storage_implies_constexpr_size<optin_hybrid>::value) );   // hybrid, but extent keeps size compile-time

    // dynamic / unknown: vacuously true (no static component to constrain)
    D_STG_CHECK( (static_storage_implies_constexpr_size<std::vector<int>>::value) );
    D_STG_CHECK( (static_storage_implies_constexpr_size<std::set<int>>::value) );
    D_STG_CHECK( (static_storage_implies_constexpr_size<std::string>::value) );
    D_STG_CHECK( (static_storage_implies_constexpr_size<reserve_only>::value) );
    D_STG_CHECK( (static_storage_implies_constexpr_size<optin_dynamic_override>::value) );
    D_STG_CHECK( (static_storage_implies_constexpr_size<int>::value) );
    D_STG_CHECK( (static_storage_implies_constexpr_size<no_signal>::value) );

    // THE edge: opt-in static siting with a runtime-only size -> coherence FAILS
    D_STG_CHECK( (!static_storage_implies_constexpr_size<optin_static_no_size>::value) );

    return _stg_ok;
}


NS_END  // testing
NS_END  // djinterp
