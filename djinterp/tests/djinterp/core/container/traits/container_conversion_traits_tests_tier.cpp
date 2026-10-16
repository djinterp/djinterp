/*******************************************************************************
* djinterp [testing]                  container_conversion_traits_tests_tier.cpp
*
*   Section I - the conversion_tier vocabulary: the enumeration's severity
* ordering (view coarsest-work to structural, with none off the scale) and the
* stable spellings conversion_tier_name emits for diagnostics.
*
*
* path:      /tests/djinterp/core/container/traits/container_conversion_traits_tests_tier.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_conversion_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_conversion_tier_vocabulary
//   the enum values, their declared severity order, and the name mapping.
bool
tests_conversion_tier_vocabulary()
{
    bool _ccv_ok = true;

    // -- names (the stable, agent-facing spellings) --
    D_CCV_CHECK( ccv_cstr_eq(conversion_tier_name(conversion_tier::view),
                             "view") );
    D_CCV_CHECK( ccv_cstr_eq(conversion_tier_name(conversion_tier::constructive),
                             "constructive") );
    D_CCV_CHECK( ccv_cstr_eq(conversion_tier_name(conversion_tier::lossy),
                             "lossy") );
    D_CCV_CHECK( ccv_cstr_eq(conversion_tier_name(conversion_tier::structural),
                             "structural") );
    D_CCV_CHECK( ccv_cstr_eq(conversion_tier_name(conversion_tier::none),
                             "none") );

    // a spelling that must NOT collide with a neighbour (guards a mis-ordered
    // ternary in conversion_tier_name)
    D_CCV_CHECK( !ccv_cstr_eq(conversion_tier_name(conversion_tier::view),
                              "constructive") );
    D_CCV_CHECK( !ccv_cstr_eq(conversion_tier_name(conversion_tier::none),
                              "structural") );

    // -- severity order (the enum is declared coarsest-work to greatest, then
    //    `none` off the scale; the file header asserts this ordering) --
    D_CCV_CHECK( static_cast<int>(conversion_tier::view)
               < static_cast<int>(conversion_tier::constructive) );
    D_CCV_CHECK( static_cast<int>(conversion_tier::constructive)
               < static_cast<int>(conversion_tier::lossy) );
    D_CCV_CHECK( static_cast<int>(conversion_tier::lossy)
               < static_cast<int>(conversion_tier::structural) );
    D_CCV_CHECK( static_cast<int>(conversion_tier::structural)
               < static_cast<int>(conversion_tier::none) );

    return _ccv_ok;
}


NS_END  // testing
NS_END  // djinterp
