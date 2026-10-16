/*******************************************************************************
* djinterp [testing]                  container_overlay_traits_tests_algebra.cpp
*
*   Section III - the overlay descriptor and its meet-semilattice algebra:
* overlay_top (the empty overlay worn by everything), is_order_blind (bears no
* sigma), overlay_subsumes (the strength order: _a <= _b iff [_a] is contained in
* [_b], i.e. _a is the MORE restrictive), and overlay_meet (the union of
* restrictions - the tighter multiplicity, the disjunction of flags).
*
*   The strength order is checked on all fronts: reflexivity, top as the weakest
* (everything subsumes it, it subsumes only itself), multiplicity tightness
* (unique subsumes multiset, not the reverse), and each flag (a bounded, keyed,
* sorted, or domain-bounded overlay subsumes its unrestricted counterpart, never
* the reverse).  The meet is checked for the tighter-multiplicity + flag-union
* result and for top acting as its identity.
*
*
* path:      /tests/djinterp/core/container/traits/container_overlay_traits_tests_algebra.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_overlay_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// representative overlays (field order: mult, keyed, sorted, capacity, domain)
namespace
{
    constexpr overlay ovl_top   = overlay_top();
    constexpr overlay ovl_set{  multiplicity_kind::unique,             false, false, false, false };
    constexpr overlay ovl_bset{ multiplicity_kind::unique,             false, false, true,  false };  // bounded set
    constexpr overlay ovl_map{  multiplicity_kind::unique,             true,  false, false, false };
    constexpr overlay ovl_mset{ multiplicity_kind::unbounded_multiset, false, false, false, false };
    constexpr overlay ovl_srt{  multiplicity_kind::sequence,           false, true,  false, false };  // sorted sequence
    constexpr overlay ovl_dom{  multiplicity_kind::unique,             false, false, true,  true  };  // bounded + domain (interval)
}


// tests_overlay_top
//   the empty overlay: a vacuous (sequence) multiplicity and no flags.
bool
tests_overlay_top()
{
    bool _ovl_ok = true;

    constexpr overlay t = overlay_top();
    D_OVL_CHECK( (t.mult == multiplicity_kind::sequence) );
    D_OVL_CHECK( (!t.keyed) );
    D_OVL_CHECK( (!t.sorted) );
    D_OVL_CHECK( (!t.capacity_bounded) );
    D_OVL_CHECK( (!t.domain_bounded) );

    return _ovl_ok;
}


// tests_is_order_blind
//   true iff the overlay bears no sigma (is not sorted).
bool
tests_is_order_blind()
{
    bool _ovl_ok = true;

    D_OVL_CHECK( (is_order_blind(ovl_top)) );
    D_OVL_CHECK( (is_order_blind(ovl_set)) );
    D_OVL_CHECK( (is_order_blind(ovl_dom)) );          // bounded + domain, but not sorted
    D_OVL_CHECK( (!is_order_blind(ovl_srt)) );         // sorted -> NOT order-blind

    return _ovl_ok;
}


// tests_overlay_subsumes
//   the strength order _a <= _b (_a is the more restrictive).
bool
tests_overlay_subsumes()
{
    bool _ovl_ok = true;

    // reflexive
    D_OVL_CHECK( (overlay_subsumes(ovl_set, ovl_set)) );
    D_OVL_CHECK( (overlay_subsumes(ovl_top, ovl_top)) );

    // top is the weakest: everything subsumes it
    D_OVL_CHECK( (overlay_subsumes(ovl_set,  ovl_top)) );
    D_OVL_CHECK( (overlay_subsumes(ovl_mset, ovl_top)) );
    D_OVL_CHECK( (overlay_subsumes(ovl_map,  ovl_top)) );
    D_OVL_CHECK( (overlay_subsumes(ovl_srt,  ovl_top)) );
    // ... and top subsumes nothing stronger
    D_OVL_CHECK( (!overlay_subsumes(ovl_top, ovl_set)) );
    D_OVL_CHECK( (!overlay_subsumes(ovl_top, ovl_srt)) );

    // multiplicity tightness: unique subsumes multiset, not the reverse
    D_OVL_CHECK( (overlay_subsumes(ovl_set,  ovl_mset)) );
    D_OVL_CHECK( (!overlay_subsumes(ovl_mset, ovl_set)) );

    // the capacity flag: a bounded set subsumes a set, not the reverse
    D_OVL_CHECK( (overlay_subsumes(ovl_bset, ovl_set)) );
    D_OVL_CHECK( (!overlay_subsumes(ovl_set, ovl_bset)) );

    // the keyed flag: a map subsumes a set, not the reverse
    D_OVL_CHECK( (overlay_subsumes(ovl_map, ovl_set)) );
    D_OVL_CHECK( (!overlay_subsumes(ovl_set, ovl_map)) );

    // the sorted flag: a sorted overlay subsumes top, top does not subsume it
    D_OVL_CHECK( (overlay_subsumes(ovl_srt, ovl_top)) );
    D_OVL_CHECK( (!overlay_subsumes(ovl_top, ovl_srt)) );

    // the domain flag: bounded+domain subsumes bounded, not the reverse
    D_OVL_CHECK( (overlay_subsumes(ovl_dom, ovl_bset)) );
    D_OVL_CHECK( (!overlay_subsumes(ovl_bset, ovl_dom)) );

    return _ovl_ok;
}


// tests_overlay_meet
//   the composition: tighter multiplicity, disjunction of flags.
bool
tests_overlay_meet()
{
    bool _ovl_ok = true;

    // meet(set, multiset): tighter multiplicity wins -> unique, no flags
    {
        constexpr overlay m = overlay_meet(ovl_set, ovl_mset);
        D_OVL_CHECK( (m.mult == multiplicity_kind::unique) );
        D_OVL_CHECK( (!m.keyed && !m.sorted && !m.capacity_bounded && !m.domain_bounded) );
    }

    // meet(set, map): equal multiplicity, keyed flag unions in
    {
        constexpr overlay m = overlay_meet(ovl_set, ovl_map);
        D_OVL_CHECK( (m.mult == multiplicity_kind::unique) );
        D_OVL_CHECK( (m.keyed) );
        D_OVL_CHECK( (!m.sorted && !m.capacity_bounded && !m.domain_bounded) );
    }

    // meet(bounded set, sorted sequence): tighter mult (unique) + union of flags
    {
        constexpr overlay m = overlay_meet(ovl_bset, ovl_srt);
        D_OVL_CHECK( (m.mult == multiplicity_kind::unique) );   // unique (rank 0) < sequence (rank 2)
        D_OVL_CHECK( (m.sorted) );                              // from the sorted operand
        D_OVL_CHECK( (m.capacity_bounded) );                   // from the bounded operand
        D_OVL_CHECK( (!m.keyed && !m.domain_bounded) );
    }

    // top is the identity: meet(x, top) == x
    {
        constexpr overlay m = overlay_meet(ovl_set, ovl_top);
        D_OVL_CHECK( (m.mult == ovl_set.mult) );
        D_OVL_CHECK( (m.keyed == ovl_set.keyed) );
        D_OVL_CHECK( (m.capacity_bounded == ovl_set.capacity_bounded) );
    }

    // commutative in value (multiplicity picks the tighter regardless of order)
    {
        constexpr overlay ab = overlay_meet(ovl_set, ovl_mset);
        constexpr overlay ba = overlay_meet(ovl_mset, ovl_set);
        D_OVL_CHECK( (ab.mult == ba.mult) );
        D_OVL_CHECK( (ab.keyed == ba.keyed) );
    }

    return _ovl_ok;
}


NS_END  // testing
NS_END  // djinterp
