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
* path:      /tests/djinterp/core/container/traits/container_overlay_traits_tests_canonical.cpp
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
bool
tests_canonical_overlay_name()
{
    bool _ovl_ok = true;

    D_OVL_CHECK( (ovl_cstr_eq(canonical_overlay_name(canonical_overlay::none),     "none")) );
    D_OVL_CHECK( (ovl_cstr_eq(canonical_overlay_name(canonical_overlay::set),      "set")) );
    D_OVL_CHECK( (ovl_cstr_eq(canonical_overlay_name(canonical_overlay::multiset), "multiset")) );
    D_OVL_CHECK( (ovl_cstr_eq(canonical_overlay_name(canonical_overlay::map),      "map")) );
    D_OVL_CHECK( (ovl_cstr_eq(canonical_overlay_name(canonical_overlay::multimap), "multimap")) );

    D_OVL_CHECK( (!ovl_cstr_eq(canonical_overlay_name(canonical_overlay::set),
                               "multiset")) );

    return _ovl_ok;
}


// tests_classify_canonical
//   the (multiplicity, keyed) -> named overlay mapping, every branch.
bool
tests_classify_canonical()
{
    bool _ovl_ok = true;

    // unique -> set / map
    D_OVL_CHECK( (classify_canonical(multiplicity_kind::unique, false) == canonical_overlay::set) );
    D_OVL_CHECK( (classify_canonical(multiplicity_kind::unique, true)  == canonical_overlay::map) );

    // multiset kinds -> multiset / multimap (BOTH bounded and unbounded)
    D_OVL_CHECK( (classify_canonical(multiplicity_kind::unbounded_multiset, false) == canonical_overlay::multiset) );
    D_OVL_CHECK( (classify_canonical(multiplicity_kind::unbounded_multiset, true)  == canonical_overlay::multimap) );
    D_OVL_CHECK( (classify_canonical(multiplicity_kind::bounded_multiset,   false) == canonical_overlay::multiset) );
    D_OVL_CHECK( (classify_canonical(multiplicity_kind::bounded_multiset,   true)  == canonical_overlay::multimap) );

    // sequence -> none, even when keyed (no multiplicity cap)
    D_OVL_CHECK( (classify_canonical(multiplicity_kind::sequence, false) == canonical_overlay::none) );
    D_OVL_CHECK( (classify_canonical(multiplicity_kind::sequence, true)  == canonical_overlay::none) );

    // unknown -> none
    D_OVL_CHECK( (classify_canonical(multiplicity_kind::unknown, false) == canonical_overlay::none) );
    D_OVL_CHECK( (classify_canonical(multiplicity_kind::unknown, true)  == canonical_overlay::none) );

    return _ovl_ok;
}


// tests_canonical_overlay_of
//   the named overlay a type wears, across every kind, plus the _t / _v companions.
bool
tests_canonical_overlay_of()
{
    bool _ovl_ok = true;

    // set: unkeyed unique (associative or interval carrier)
    D_OVL_CHECK( (canonical_overlay_of<std::set<int>>::value           == canonical_overlay::set) );
    D_OVL_CHECK( (canonical_overlay_of<std::unordered_set<int>>::value == canonical_overlay::set) );
    D_OVL_CHECK( (canonical_overlay_of<domain_bounded_seq>::value      == canonical_overlay::set) );

    // map: keyed unique
    D_OVL_CHECK( (canonical_overlay_of<std::map<int, int>>::value == canonical_overlay::map) );

    // multiset: unkeyed repeatable (both multiset kinds)
    D_OVL_CHECK( (canonical_overlay_of<std::multiset<int>>::value           == canonical_overlay::multiset) );
    D_OVL_CHECK( (canonical_overlay_of<std::unordered_multiset<int>>::value == canonical_overlay::multiset) );
    D_OVL_CHECK( (canonical_overlay_of<bounded_multiset_seq>::value         == canonical_overlay::multiset) );

    // multimap: keyed repeatable (both multiset kinds)
    D_OVL_CHECK( (canonical_overlay_of<std::multimap<int, int>>::value == canonical_overlay::multimap) );
    D_OVL_CHECK( (canonical_overlay_of<bounded_multimap_seq>::value    == canonical_overlay::multimap) );

    // none: sequences (incl. a KEYED sequence), unknowns, and non-containers
    D_OVL_CHECK( (canonical_overlay_of<std::vector<int>>::value  == canonical_overlay::none) );
    D_OVL_CHECK( (canonical_overlay_of<std::array<int, 4>>::value == canonical_overlay::none) );
    D_OVL_CHECK( (canonical_overlay_of<std::vector<std::pair<int, int>>>::value == canonical_overlay::none) );  // keyed sequence
    D_OVL_CHECK( (canonical_overlay_of<sorted_seq>::value        == canonical_overlay::none) );
    D_OVL_CHECK( (canonical_overlay_of<std::forward_list<int>>::value == canonical_overlay::none) );  // unknown mult
    D_OVL_CHECK( (canonical_overlay_of<int>::value              == canonical_overlay::none) );

    // ::type carrier
    D_OVL_CHECK( (canonical_overlay_of_t<std::set<int>>::value  == canonical_overlay::set) );
    D_OVL_CHECK( (canonical_overlay_of_t<std::multimap<int, int>>::value == canonical_overlay::multimap) );

    return _ovl_ok;
}


NS_END  // testing
NS_END  // djinterp
