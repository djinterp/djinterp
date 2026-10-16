/*******************************************************************************
* djinterp [testing]               container_overlay_traits_tests_overlay_of.cpp
*
*   Section IV - overlay_of, the overlay a container actually wears, assembled
* from the per-axis verdicts (mu from multiplicity; gamma / delta from
* boundedness; eta / sigma local).  Every restriction member is checked across a
* representative spread, value() is confirmed to materialise them faithfully,
* is_order_blind_overlay is verified (only the sigma-bearing sorted_seq is NOT
* order-blind), and wears_overlay is exercised as the conformance test - a type
* wears a target overlay iff its own overlay is at least as strong.
*
*   The synthesis is what these pin down: array is capacity-bounded (a fixed
* extent) but wears no multiplicity; an interval carrier (domain_bounded_seq) is
* a bounded, domain-bounded set; and a sequence of pairs is keyed yet otherwise
* unrestricted.
*
*
* path:      /tests/djinterp/core/container/traits/container_overlay_traits_tests_overlay_of.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_overlay_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_overlay_of
//   the five borne restrictions across a representative spread, plus value().
bool
tests_overlay_of()
{
    bool _ovl_ok = true;

    // -- vector<int>: an unrestricted sequence --
    {
        using O = overlay_of<std::vector<int>>;
        D_OVL_CHECK( (O::mult == multiplicity_kind::sequence) );
        D_OVL_CHECK( (!O::keyed && !O::sorted && !O::capacity_bounded && !O::domain_bounded) );
    }

    // -- set<int>: unique, nothing else --
    {
        using O = overlay_of<std::set<int>>;
        D_OVL_CHECK( (O::mult == multiplicity_kind::unique) );
        D_OVL_CHECK( (!O::keyed) );
        D_OVL_CHECK( (!O::capacity_bounded && !O::domain_bounded && !O::sorted) );
    }

    // -- map<int,int>: unique AND keyed --
    {
        using O = overlay_of<std::map<int, int>>;
        D_OVL_CHECK( (O::mult == multiplicity_kind::unique) );
        D_OVL_CHECK( (O::keyed) );
    }

    // -- multiset<int>: unbounded_multiset, unkeyed --
    {
        using O = overlay_of<std::multiset<int>>;
        D_OVL_CHECK( (O::mult == multiplicity_kind::unbounded_multiset) );
        D_OVL_CHECK( (!O::keyed) );
    }

    // -- array<int,4>: capacity-bounded, but a sequence (no mu, no domain) --
    {
        using O = overlay_of<std::array<int, 4>>;
        D_OVL_CHECK( (O::mult == multiplicity_kind::sequence) );
        D_OVL_CHECK( (O::capacity_bounded) );
        D_OVL_CHECK( (!O::domain_bounded && !O::keyed && !O::sorted) );
    }

    // -- vector<pair>: keyed, yet an unrestricted sequence otherwise --
    {
        using O = overlay_of<std::vector<std::pair<int, int>>>;
        D_OVL_CHECK( (O::mult == multiplicity_kind::sequence) );
        D_OVL_CHECK( (O::keyed) );
        D_OVL_CHECK( (!O::capacity_bounded && !O::sorted) );
    }

    // -- sorted_seq: the sigma opt-in shows up here --
    {
        using O = overlay_of<sorted_seq>;
        D_OVL_CHECK( (O::sorted) );
        D_OVL_CHECK( (O::mult == multiplicity_kind::sequence) );
        D_OVL_CHECK( (!O::keyed) );
    }

    // -- domain_bounded_seq: an interval carrier -> unique + capacity + domain --
    {
        using O = overlay_of<domain_bounded_seq>;
        D_OVL_CHECK( (O::mult == multiplicity_kind::unique) );
        D_OVL_CHECK( (O::capacity_bounded) );
        D_OVL_CHECK( (O::domain_bounded) );
        D_OVL_CHECK( (!O::keyed && !O::sorted) );
    }

    // -- capacity_bounded_seq: gamma via capacity()/!reserve, a sequence --
    {
        using O = overlay_of<capacity_bounded_seq>;
        D_OVL_CHECK( (O::mult == multiplicity_kind::sequence) );
        D_OVL_CHECK( (O::capacity_bounded) );
        D_OVL_CHECK( (!O::domain_bounded) );
    }

    // -- bounded_multiset_seq: the bounded_multiset kind --
    {
        using O = overlay_of<bounded_multiset_seq>;
        D_OVL_CHECK( (O::mult == multiplicity_kind::bounded_multiset) );
        D_OVL_CHECK( (!O::keyed) );
    }

    // -- int: a non-container wears nothing (unknown multiplicity) --
    {
        using O = overlay_of<int>;
        D_OVL_CHECK( (O::mult == multiplicity_kind::unknown) );
        D_OVL_CHECK( (!O::keyed && !O::sorted && !O::capacity_bounded && !O::domain_bounded) );
    }

    // value() materialises the members faithfully
    {
        constexpr overlay v = overlay_of<std::map<int, int>>::value();
        D_OVL_CHECK( (v.mult == overlay_of<std::map<int, int>>::mult) );
        D_OVL_CHECK( (v.keyed == overlay_of<std::map<int, int>>::keyed) );
        D_OVL_CHECK( (v.capacity_bounded == overlay_of<std::map<int, int>>::capacity_bounded) );
    }
    {
        constexpr overlay v = overlay_of<domain_bounded_seq>::value();
        D_OVL_CHECK( (v.domain_bounded) );
        D_OVL_CHECK( (v.capacity_bounded) );
    }

    return _ovl_ok;
}


// tests_is_order_blind_overlay
//   true iff the type's overlay bears no sigma - false ONLY for sorted_seq here.
bool
tests_is_order_blind_overlay()
{
    bool _ovl_ok = true;

    // order-blind: the canonical overlays and every non-sorted type
    D_OVL_CHECK( (is_order_blind_overlay<std::vector<int>>::value) );
    D_OVL_CHECK( (is_order_blind_overlay<std::set<int>>::value) );
    D_OVL_CHECK( (is_order_blind_overlay<std::map<int, int>>::value) );
    D_OVL_CHECK( (is_order_blind_overlay<std::multiset<int>>::value) );
    D_OVL_CHECK( (is_order_blind_overlay<std::array<int, 4>>::value) );
    D_OVL_CHECK( (is_order_blind_overlay<domain_bounded_seq>::value) );
    D_OVL_CHECK( (is_order_blind_overlay<int>::value) );

    // NOT order-blind: the sigma opt-in
    D_OVL_CHECK( (!is_order_blind_overlay<sorted_seq>::value) );

    return _ovl_ok;
}


// tests_wears_overlay
//   conformance: a type wears a target iff its overlay is at least as strong.
bool
tests_wears_overlay()
{
    bool _ovl_ok = true;

    constexpr overlay target_top = overlay_top();
    constexpr overlay target_set{ multiplicity_kind::unique, false, false, false, false };
    constexpr overlay target_map{ multiplicity_kind::unique, true,  false, false, false };
    constexpr overlay target_cap{ multiplicity_kind::sequence, false, false, true, false };  // capacity-bounded sequence
    constexpr overlay target_dom{ multiplicity_kind::unique, false, false, true, true };     // bounded + domain set

    // everything wears the empty (top) overlay
    D_OVL_CHECK( (wears_overlay<std::set<int>>(target_top)) );
    D_OVL_CHECK( (wears_overlay<std::vector<int>>(target_top)) );
    D_OVL_CHECK( (wears_overlay<std::multiset<int>>(target_top)) );

    // the set overlay: worn by unique containers, not by sequences or multisets
    D_OVL_CHECK( (wears_overlay<std::set<int>>(target_set)) );
    D_OVL_CHECK( (wears_overlay<std::map<int, int>>(target_set)) );   // a map is a unique container
    D_OVL_CHECK( (!wears_overlay<std::vector<int>>(target_set)) );    // a sequence is not unique
    D_OVL_CHECK( (!wears_overlay<std::multiset<int>>(target_set)) );  // a multiset is not unique

    // the map overlay: needs keyed too - a set does not wear it
    D_OVL_CHECK( (wears_overlay<std::map<int, int>>(target_map)) );
    D_OVL_CHECK( (!wears_overlay<std::set<int>>(target_map)) );

    // the capacity-bounded sequence overlay: array wears it, vector does not
    D_OVL_CHECK( (wears_overlay<std::array<int, 4>>(target_cap)) );
    D_OVL_CHECK( (!wears_overlay<std::vector<int>>(target_cap)) );

    // the bounded+domain set overlay: the interval carrier wears it
    D_OVL_CHECK( (wears_overlay<domain_bounded_seq>(target_dom)) );
    D_OVL_CHECK( (!wears_overlay<std::set<int>>(target_dom)) );       // a plain set is not domain-bounded

    return _ovl_ok;
}


NS_END  // testing
NS_END  // djinterp
