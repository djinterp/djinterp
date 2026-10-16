/*******************************************************************************
* djinterp [testing]           container_overlay_traits_tests_classification.cpp
*
*   Section VI - overlay_container_class, the assembled overlay snapshot (the
* five borne restrictions, the order-blind bit, and the canonical overlay with
* its name), checked member-for-member against the standalone traits across a
* representative of every canonical kind - including the sorted (not order-blind)
* case and the interval carrier (a bounded, domain-bounded set).  The `_v`
* variable-template companions (is_keyed_container_v, is_order_blind_overlay_v,
* canonical_overlay_of_v) are verified here too, gated as the header emits them.
*
*
* path:      /tests/djinterp/core/container/traits/container_overlay_traits_tests_classification.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_overlay_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_overlay_container_class
//   the assembled snapshot, one representative per canonical kind (and route).
bool
tests_overlay_container_class()
{
    bool _ovl_ok = true;

    // -- vector<int>: unrestricted sequence, canonical none --
    {
        using C = overlay_container_class<std::vector<int>>;
        D_OVL_CHECK( (C::mult == multiplicity_kind::sequence) );
        D_OVL_CHECK( (!C::keyed && !C::sorted && !C::capacity_bounded && !C::domain_bounded) );
        D_OVL_CHECK( (C::order_blind) );
        D_OVL_CHECK( (C::canonical == canonical_overlay::none) );
        D_OVL_CHECK( (ovl_cstr_eq(C::canonical_name, "none")) );
    }

    // -- set<int>: canonical set --
    {
        using C = overlay_container_class<std::set<int>>;
        D_OVL_CHECK( (C::mult == multiplicity_kind::unique) );
        D_OVL_CHECK( (!C::keyed) );
        D_OVL_CHECK( (C::order_blind) );
        D_OVL_CHECK( (C::canonical == canonical_overlay::set) );
        D_OVL_CHECK( (ovl_cstr_eq(C::canonical_name, "set")) );
    }

    // -- map<int,int>: canonical map (keyed) --
    {
        using C = overlay_container_class<std::map<int, int>>;
        D_OVL_CHECK( (C::mult == multiplicity_kind::unique) );
        D_OVL_CHECK( (C::keyed) );
        D_OVL_CHECK( (C::canonical == canonical_overlay::map) );
        D_OVL_CHECK( (ovl_cstr_eq(C::canonical_name, "map")) );
    }

    // -- multiset<int>: canonical multiset --
    {
        using C = overlay_container_class<std::multiset<int>>;
        D_OVL_CHECK( (C::mult == multiplicity_kind::unbounded_multiset) );
        D_OVL_CHECK( (!C::keyed) );
        D_OVL_CHECK( (C::canonical == canonical_overlay::multiset) );
        D_OVL_CHECK( (ovl_cstr_eq(C::canonical_name, "multiset")) );
    }

    // -- multimap<int,int>: canonical multimap --
    {
        using C = overlay_container_class<std::multimap<int, int>>;
        D_OVL_CHECK( (C::keyed) );
        D_OVL_CHECK( (C::canonical == canonical_overlay::multimap) );
        D_OVL_CHECK( (ovl_cstr_eq(C::canonical_name, "multimap")) );
    }

    // -- array<int,4>: capacity-bounded sequence --
    {
        using C = overlay_container_class<std::array<int, 4>>;
        D_OVL_CHECK( (C::capacity_bounded) );
        D_OVL_CHECK( (!C::domain_bounded) );
        D_OVL_CHECK( (C::order_blind) );
        D_OVL_CHECK( (C::canonical == canonical_overlay::none) );
    }

    // -- sorted_seq: the NOT-order-blind case --
    {
        using C = overlay_container_class<sorted_seq>;
        D_OVL_CHECK( (C::sorted) );
        D_OVL_CHECK( (!C::order_blind) );
        D_OVL_CHECK( (C::canonical == canonical_overlay::none) );
    }

    // -- domain_bounded_seq: bounded + domain-bounded set --
    {
        using C = overlay_container_class<domain_bounded_seq>;
        D_OVL_CHECK( (C::mult == multiplicity_kind::unique) );
        D_OVL_CHECK( (C::capacity_bounded) );
        D_OVL_CHECK( (C::domain_bounded) );
        D_OVL_CHECK( (C::canonical == canonical_overlay::set) );
    }

    // -- bounded_multiset_seq: the bounded_multiset kind, canonical multiset --
    {
        using C = overlay_container_class<bounded_multiset_seq>;
        D_OVL_CHECK( (C::mult == multiplicity_kind::bounded_multiset) );
        D_OVL_CHECK( (C::canonical == canonical_overlay::multiset) );
    }

    // -- int: a non-container --
    {
        using C = overlay_container_class<int>;
        D_OVL_CHECK( (C::mult == multiplicity_kind::unknown) );
        D_OVL_CHECK( (!C::keyed && !C::sorted && !C::capacity_bounded && !C::domain_bounded) );
        D_OVL_CHECK( (C::order_blind) );
        D_OVL_CHECK( (C::canonical == canonical_overlay::none) );
        D_OVL_CHECK( (ovl_cstr_eq(C::canonical_name, "none")) );
    }

    return _ovl_ok;
}


// tests_overlay_variable_templates
//   the `_v` companions agree with their `::value` forms.  Gated on variable-
//   template support exactly as the header emits them.
bool
tests_overlay_variable_templates()
{
    bool _ovl_ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_keyed_container_v
    D_OVL_CHECK( (is_keyed_container_v<std::map<int, int>>
                     == is_keyed_container<std::map<int, int>>::value) );
    D_OVL_CHECK( (is_keyed_container_v<std::map<int, int>>) );
    D_OVL_CHECK( (!is_keyed_container_v<std::set<int>>) );

    // is_order_blind_overlay_v
    D_OVL_CHECK( (is_order_blind_overlay_v<std::vector<int>>
                     == is_order_blind_overlay<std::vector<int>>::value) );
    D_OVL_CHECK( (is_order_blind_overlay_v<std::set<int>>) );
    D_OVL_CHECK( (!is_order_blind_overlay_v<sorted_seq>) );

    // canonical_overlay_of_v
    D_OVL_CHECK( (canonical_overlay_of_v<std::set<int>>
                     == canonical_overlay_of<std::set<int>>::value) );
    D_OVL_CHECK( (canonical_overlay_of_v<std::map<int, int>>   == canonical_overlay::map) );
    D_OVL_CHECK( (canonical_overlay_of_v<std::multiset<int>>   == canonical_overlay::multiset) );
    D_OVL_CHECK( (canonical_overlay_of_v<bounded_multimap_seq> == canonical_overlay::multimap) );
    D_OVL_CHECK( (canonical_overlay_of_v<std::vector<int>>     == canonical_overlay::none) );

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return _ovl_ok;
}


NS_END  // testing
NS_END  // djinterp
