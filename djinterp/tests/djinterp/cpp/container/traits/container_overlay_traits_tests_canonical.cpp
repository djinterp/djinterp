/*******************************************************************************
* djinterp [testing]                container_overlay_traits_tests_canonical.cpp
*
*   Section V - the canonical overlays, the named 2x2 of keyed? against
* repeatable?  classify_canonical maps (multiplicity, keyed) to the named overlay:
* unique -> set/map, a multiset kind -> multiset/multimap, and anything else
* (a sequence - even a keyed one - or an unknown) -> none.  canonical_overlay_of
* applies it to a type.
*
*   The edges: BOTH multiset kinds (bounded and unbounded) classify as
* multiset/multimap, and a keyed SEQUENCE (a vector of pairs) is `none`, not a
* map - the canonical base is a multiplicity distinction, and a sequence bears no
* multiplicity cap regardless of its value shape.
*
*
* path:      /tests/djinterp/cpp/container/traits/container_overlay_traits_tests_canonical.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_overlay_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_canonical_overlay_name
//   the stable spellings of canonical_overlay.
bool tests_canonical_overlay_name()
{
    bool _ovl_ok = true;

    D_OVL_CHECK( (ovl_cstr_eq(canonical_overlay_name(DCanonicalOverlay::none),     "none")) );
    D_OVL_CHECK( (ovl_cstr_eq(canonical_overlay_name(DCanonicalOverlay::set),      "set")) );
    D_OVL_CHECK( (ovl_cstr_eq(canonical_overlay_name(DCanonicalOverlay::multiset), "multiset")) );
    D_OVL_CHECK( (ovl_cstr_eq(canonical_overlay_name(DCanonicalOverlay::map),      "map")) );
    D_OVL_CHECK( (ovl_cstr_eq(canonical_overlay_name(DCanonicalOverlay::multimap), "multimap")) );

    D_OVL_CHECK( (!ovl_cstr_eq(canonical_overlay_name(DCanonicalOverlay::set),
                               "multiset")) );

    return _ovl_ok;
}


// tests_classify_canonical
//   the (multiplicity, keyed) -> named overlay mapping, every branch.
bool tests_classify_canonical()
{
    bool _ovl_ok = true;

    // unique -> set / map
    D_OVL_CHECK( (classify_canonical(DMultiplicityKind::unique, false) == DCanonicalOverlay::set) );
    D_OVL_CHECK( (classify_canonical(DMultiplicityKind::unique, true)  == DCanonicalOverlay::map) );

    // multiset kinds -> multiset / multimap (BOTH bounded and unbounded)
    D_OVL_CHECK( (classify_canonical(DMultiplicityKind::unbounded_multiset, false) == DCanonicalOverlay::multiset) );
    D_OVL_CHECK( (classify_canonical(DMultiplicityKind::unbounded_multiset, true)  == DCanonicalOverlay::multimap) );
    D_OVL_CHECK( (classify_canonical(DMultiplicityKind::bounded_multiset,   false) == DCanonicalOverlay::multiset) );
    D_OVL_CHECK( (classify_canonical(DMultiplicityKind::bounded_multiset,   true)  == DCanonicalOverlay::multimap) );

    // sequence -> none, even when keyed (no multiplicity cap)
    D_OVL_CHECK( (classify_canonical(DMultiplicityKind::sequence, false) == DCanonicalOverlay::none) );
    D_OVL_CHECK( (classify_canonical(DMultiplicityKind::sequence, true)  == DCanonicalOverlay::none) );

    // unknown -> none
    D_OVL_CHECK( (classify_canonical(DMultiplicityKind::unknown, false) == DCanonicalOverlay::none) );
    D_OVL_CHECK( (classify_canonical(DMultiplicityKind::unknown, true)  == DCanonicalOverlay::none) );

    return _ovl_ok;
}


// tests_canonical_overlay_of
//   the named overlay a type wears, across every kind, plus the _t / _v companions.
bool tests_canonical_overlay_of()
{
    bool _ovl_ok = true;

    // set: unkeyed unique (associative or interval carrier)
    D_OVL_CHECK( (canonical_overlay_of<std::set<int>>::value           == DCanonicalOverlay::set) );
    D_OVL_CHECK( (canonical_overlay_of<std::unordered_set<int>>::value == DCanonicalOverlay::set) );
    D_OVL_CHECK( (canonical_overlay_of<DDomainBoundedSeq>::value      == DCanonicalOverlay::set) );

    // map: keyed unique
    D_OVL_CHECK( (canonical_overlay_of<std::map<int, int>>::value == DCanonicalOverlay::map) );

    // multiset: unkeyed repeatable (both multiset kinds)
    D_OVL_CHECK( (canonical_overlay_of<std::multiset<int>>::value           == DCanonicalOverlay::multiset) );
    D_OVL_CHECK( (canonical_overlay_of<std::unordered_multiset<int>>::value == DCanonicalOverlay::multiset) );
    D_OVL_CHECK( (canonical_overlay_of<DBoundedMultisetSeq>::value         == DCanonicalOverlay::multiset) );

    // multimap: keyed repeatable (both multiset kinds)
    D_OVL_CHECK( (canonical_overlay_of<std::multimap<int, int>>::value == DCanonicalOverlay::multimap) );
    D_OVL_CHECK( (canonical_overlay_of<DBoundedMultimapSeq>::value    == DCanonicalOverlay::multimap) );

    // none: sequences (incl. a KEYED sequence), unknowns, and non-containers
    D_OVL_CHECK( (canonical_overlay_of<std::vector<int>>::value  == DCanonicalOverlay::none) );
    D_OVL_CHECK( (canonical_overlay_of<std::array<int, 4>>::value == DCanonicalOverlay::none) );
    D_OVL_CHECK( (canonical_overlay_of<std::vector<std::pair<int, int>>>::value == DCanonicalOverlay::none) );  // keyed sequence
    D_OVL_CHECK( (canonical_overlay_of<DSortedSeq>::value        == DCanonicalOverlay::none) );
    D_OVL_CHECK( (canonical_overlay_of<std::forward_list<int>>::value == DCanonicalOverlay::none) );  // unknown mult
    D_OVL_CHECK( (canonical_overlay_of<int>::value              == DCanonicalOverlay::none) );

    // ::type carrier
    D_OVL_CHECK( (canonical_overlay_of_t<std::set<int>>::value  == DCanonicalOverlay::set) );
    D_OVL_CHECK( (canonical_overlay_of_t<std::multimap<int, int>>::value == DCanonicalOverlay::multimap) );

    return _ovl_ok;
}


NS_END  // testing
NS_END  // djinterp
