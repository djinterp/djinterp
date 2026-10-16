/*******************************************************************************
* djinterp [testing]               container_overlay_traits_tests_vocabulary.cpp
*
*   Section I - the restriction vocabulary: the scope and kind enums, their
* stable spellings, and scope_of_restriction, which records what each restriction
* inspects.  The load-bearing fact is that only `sorted` is sequence-level (and
* only `keyed` is static); every other restriction is bag-level - which is what
* makes an overlay without sigma order-blind.
*
*
* path:      /tests/djinterp/core/container/traits/container_overlay_traits_tests_vocabulary.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_overlay_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_restriction_scope_name
//   the stable spellings of restriction_scope.
bool
tests_restriction_scope_name()
{
    bool _ovl_ok = true;

    D_OVL_CHECK( (ovl_cstr_eq(restriction_scope_name(restriction_scope::static_scope),   "static")) );
    D_OVL_CHECK( (ovl_cstr_eq(restriction_scope_name(restriction_scope::bag_level),       "bag_level")) );
    D_OVL_CHECK( (ovl_cstr_eq(restriction_scope_name(restriction_scope::sequence_level),  "sequence_level")) );

    D_OVL_CHECK( (!ovl_cstr_eq(restriction_scope_name(restriction_scope::bag_level),
                               "sequence_level")) );

    return _ovl_ok;
}


// tests_restriction_kind_name
//   the stable spellings of restriction_kind.
bool
tests_restriction_kind_name()
{
    bool _ovl_ok = true;

    D_OVL_CHECK( (ovl_cstr_eq(restriction_kind_name(restriction_kind::multiplicity), "multiplicity")) );
    D_OVL_CHECK( (ovl_cstr_eq(restriction_kind_name(restriction_kind::capacity),     "capacity")) );
    D_OVL_CHECK( (ovl_cstr_eq(restriction_kind_name(restriction_kind::domain),       "domain")) );
    D_OVL_CHECK( (ovl_cstr_eq(restriction_kind_name(restriction_kind::sorted),       "sorted")) );
    D_OVL_CHECK( (ovl_cstr_eq(restriction_kind_name(restriction_kind::keyed),        "keyed")) );

    D_OVL_CHECK( (!ovl_cstr_eq(restriction_kind_name(restriction_kind::domain),
                               "capacity")) );

    return _ovl_ok;
}


// tests_scope_of_restriction
//   the scope each restriction kind carries - keyed is static, sorted is
//   sequence-level, and the mu / gamma / delta restrictions are all bag-level.
bool
tests_scope_of_restriction()
{
    bool _ovl_ok = true;

    // static: only keyed
    D_OVL_CHECK( (scope_of_restriction(restriction_kind::keyed)
                     == restriction_scope::static_scope) );

    // sequence-level: only sorted
    D_OVL_CHECK( (scope_of_restriction(restriction_kind::sorted)
                     == restriction_scope::sequence_level) );

    // bag-level: multiplicity, capacity, domain
    D_OVL_CHECK( (scope_of_restriction(restriction_kind::multiplicity)
                     == restriction_scope::bag_level) );
    D_OVL_CHECK( (scope_of_restriction(restriction_kind::capacity)
                     == restriction_scope::bag_level) );
    D_OVL_CHECK( (scope_of_restriction(restriction_kind::domain)
                     == restriction_scope::bag_level) );

    return _ovl_ok;
}


NS_END  // testing
NS_END  // djinterp
