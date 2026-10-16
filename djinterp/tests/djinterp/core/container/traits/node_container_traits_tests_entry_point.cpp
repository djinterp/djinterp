/*******************************************************************************
* djinterp [core]                    node_container_traits_tests_entry_point.cpp
*
*
* path:      /tests/djinterp/core/container/traits/node_container_traits_tests_entry_point.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "node_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// II.  Entry point detection
// ===========================================================================
//   Nine method probes and the disjunction that decides whether a type exposes
// an entry reference at all.  Only FOUR of the nine feed that disjunction -
// entry_point, root, head and entries - because only those name a root; the
// other five (has_root, tail, entry_count, has_entry, release_entry) describe
// or manipulate an entry set that must already exist.  Each test therefore
// checks its probe both in isolation and, where relevant, against the
// disjunction it does or does not join.


/*
tests_has_entry_point_method
  Verifies the probe for the topology-neutral entry_point() accessor.
  Tests the following:
  - the containers that expose it are detected, whatever their shape
  - a container that reaches its nodes only through a domain-specific alias -
    a bare head() or entries() - is not detected
  - the probe stands alone: a type with entry_point() and nothing else is
    detected even though it is not a node container
*/
bool
tests_has_entry_point_method()
{
    bool ok = true;

    // present
    D_NC_CHECK(has_entry_point_method<nct_tree>::value       == true);
    D_NC_CHECK(has_entry_point_method<nct_list>::value       == true);
    D_NC_CHECK(has_entry_point_method<nct_generic>::value    == true);
    D_NC_CHECK(has_entry_point_method<nct_all_shapes>::value == true);
    D_NC_CHECK(has_entry_point_method<nct_unknown_policy_container>::value
                   == true);

    // domain alias only, no generic accessor
    D_NC_CHECK(has_entry_point_method<nct_slist>::value == false);
    D_NC_CHECK(has_entry_point_method<nct_graph>::value == false);
    D_NC_CHECK(has_entry_point_method<nct_unique_container>::value == false);

    // in isolation, without the node_type half of container identity
    D_NC_CHECK(has_entry_point_method<nct_entry_point_only_no_node>::value
                   == true);

    // absent
    D_NC_CHECK(has_entry_point_method<nct_bare>::value      == false);
    D_NC_CHECK(has_entry_point_method<nct_node_only>::value == false);
    D_NC_CHECK(has_entry_point_method<int>::value           == false);
    D_NC_CHECK(has_entry_point_method<void>::value          == false);
    D_NC_CHECK(has_entry_point_method<nonesuch>::value      == false);

    return ok;
}

/*
tests_has_root_method
  Verifies the probe for root() - the single distinguished entry that marks a
  tree-shaped container.
  Tests the following:
  - the tree fixtures are detected
  - lists and graphs are not: their entry reference has a different name
  - root() in isolation is detected, without node_type
  - has_root() does NOT stand in for root(): the interrogative is a different
    member, and a type carrying only has_root() reads dark
*/
bool
tests_has_root_method()
{
    bool ok = true;

    // present
    D_NC_CHECK(has_root_method<nct_tree>::value             == true);
    D_NC_CHECK(has_root_method<nct_unique_container>::value == true);
    D_NC_CHECK(has_root_method<nct_tree_and_list>::value    == true);
    D_NC_CHECK(has_root_method<nct_tree_and_graph>::value   == true);
    D_NC_CHECK(has_root_method<nct_all_shapes>::value       == true);
    D_NC_CHECK(has_root_method<nct_root_only>::value        == true);

    // other shapes
    D_NC_CHECK(has_root_method<nct_list>::value    == false);
    D_NC_CHECK(has_root_method<nct_slist>::value   == false);
    D_NC_CHECK(has_root_method<nct_graph>::value   == false);
    D_NC_CHECK(has_root_method<nct_generic>::value == false);

    // the interrogative is not the accessor
    D_NC_CHECK(has_root_method<nct_has_root_no_root>::value == false);

    // absent
    D_NC_CHECK(has_root_method<nct_bare>::value == false);
    D_NC_CHECK(has_root_method<int>::value      == false);
    D_NC_CHECK(has_root_method<void>::value     == false);

    return ok;
}

/*
tests_has_root_member_method
  Verifies the probe for has_root() - the interrogative that reports whether a
  tree's root is populated.
  Tests the following:
  - the fixture that declares it is detected, with or without root() alongside
  - root() does not stand in for has_root(), so the two probes are independent
  - the probe does not feed the entry disjunction: a type carrying has_root()
    alone exposes no entry reference
*/
bool
tests_has_root_member_method()
{
    bool ok = true;

    // present
    D_NC_CHECK(has_root_member_method<nct_tree>::value == true);
    D_NC_CHECK(has_root_member_method<nct_has_root_no_root>::value == true);

    // absent
    D_NC_CHECK(has_root_member_method<nct_unique_container>::value == false);
    D_NC_CHECK(has_root_member_method<nct_root_only>::value        == false);
    D_NC_CHECK(has_root_member_method<nct_list>::value             == false);
    D_NC_CHECK(has_root_member_method<nct_all_shapes>::value       == false);
    D_NC_CHECK(has_root_member_method<nct_bare>::value             == false);
    D_NC_CHECK(has_root_member_method<int>::value                  == false);

    // independent of root(), in both directions
    D_NC_CHECK(   ( has_root_member_method<nct_has_root_no_root>::value
                        == true )
               && ( has_root_method<nct_has_root_no_root>::value == false ));
    D_NC_CHECK(   ( has_root_member_method<nct_root_only>::value == false )
               && ( has_root_method<nct_root_only>::value        == true ));

    // and it is not an entry reference
    D_NC_CHECK(has_any_entry_point<nct_has_root_no_root>::value == false);

    return ok;
}

/*
tests_has_head_method
  Verifies the probe for head() - the entry that marks a list-shaped
  container.
  Tests the following:
  - both list fixtures are detected, singly and doubly linked alike
  - trees and graphs are not
  - head() in isolation is detected, without node_type
  - tail() does NOT stand in for head()
*/
bool
tests_has_head_method()
{
    bool ok = true;

    // present
    D_NC_CHECK(has_head_method<nct_list>::value              == true);
    D_NC_CHECK(has_head_method<nct_slist>::value             == true);
    D_NC_CHECK(has_head_method<nct_non_owning_container>::value == true);
    D_NC_CHECK(has_head_method<nct_tree_and_list>::value     == true);
    D_NC_CHECK(has_head_method<nct_list_and_graph>::value    == true);
    D_NC_CHECK(has_head_method<nct_all_shapes>::value        == true);
    D_NC_CHECK(has_head_method<nct_head_only_no_node>::value == true);

    // other shapes
    D_NC_CHECK(has_head_method<nct_tree>::value    == false);
    D_NC_CHECK(has_head_method<nct_graph>::value   == false);
    D_NC_CHECK(has_head_method<nct_generic>::value == false);

    // tail is not head
    D_NC_CHECK(has_head_method<nct_tail_no_head>::value == false);

    // absent
    D_NC_CHECK(has_head_method<nct_bare>::value == false);
    D_NC_CHECK(has_head_method<int>::value      == false);
    D_NC_CHECK(has_head_method<void>::value     == false);

    return ok;
}

/*
tests_has_tail_method
  Verifies the probe for tail() - the second end of a doubly linked list.
  Tests the following:
  - the doubly linked fixture is detected; the singly linked one is not
  - tail() is detected in isolation, but does NOT feed the entry disjunction:
    a back reference is not a root, so a type carrying tail() alone exposes no
    entry point and is not a node container
*/
bool
tests_has_tail_method()
{
    bool ok = true;

    // present
    D_NC_CHECK(has_tail_method<nct_list>::value         == true);
    D_NC_CHECK(has_tail_method<nct_all_shapes>::value   == true);
    D_NC_CHECK(has_tail_method<nct_tail_no_head>::value == true);

    // absent
    D_NC_CHECK(has_tail_method<nct_slist>::value           == false);
    D_NC_CHECK(has_tail_method<nct_tree>::value            == false);
    D_NC_CHECK(has_tail_method<nct_graph>::value           == false);
    D_NC_CHECK(has_tail_method<nct_list_and_graph>::value  == false);
    D_NC_CHECK(has_tail_method<nct_bare>::value            == false);
    D_NC_CHECK(has_tail_method<int>::value                 == false);

    // a back reference is not a root
    D_NC_CHECK(   ( has_tail_method<nct_tail_no_head>::value      == true )
               && ( has_any_entry_point<nct_tail_no_head>::value  == false )
               && ( is_node_container<nct_tail_no_head>::value    == false ));

    return ok;
}

/*
tests_has_entries_method
  Verifies the probe for entries() - the entry SET rho that marks a graph-
  shaped container, where a node may be reached by several references.
  Tests the following:
  - the graph fixtures are detected
  - trees and lists are not
  - entries() in isolation is detected, without node_type
  - entry_count() does NOT stand in for entries()
*/
bool
tests_has_entries_method()
{
    bool ok = true;

    // present
    D_NC_CHECK(has_entries_method<nct_graph>::value             == true);
    D_NC_CHECK(has_entries_method<nct_shared_container>::value  == true);
    D_NC_CHECK(has_entries_method<nct_list_and_graph>::value    == true);
    D_NC_CHECK(has_entries_method<nct_tree_and_graph>::value    == true);
    D_NC_CHECK(has_entries_method<nct_all_shapes>::value        == true);
    D_NC_CHECK(has_entries_method<nct_entries_only_no_node>::value == true);

    // other shapes
    D_NC_CHECK(has_entries_method<nct_tree>::value    == false);
    D_NC_CHECK(has_entries_method<nct_list>::value    == false);
    D_NC_CHECK(has_entries_method<nct_generic>::value == false);

    // the cardinality is not the set
    D_NC_CHECK(has_entries_method<nct_entry_count_no_entries>::value
                   == false);

    // absent
    D_NC_CHECK(has_entries_method<nct_bare>::value == false);
    D_NC_CHECK(has_entries_method<int>::value      == false);
    D_NC_CHECK(has_entries_method<void>::value     == false);

    return ok;
}

/*
tests_has_entry_count_method
  Verifies the probe for entry_count() - the cardinality of the entry set.
  Tests the following:
  - the graph fixture is detected, and so is the fixture that carries the
    count without the set
  - entries() does not imply entry_count(): the two probes are independent
  - the count does NOT feed the entry disjunction - knowing how many roots
    there are is not the same as holding one
*/
bool
tests_has_entry_count_method()
{
    bool ok = true;

    // present
    D_NC_CHECK(has_entry_count_method<nct_graph>::value == true);
    D_NC_CHECK(has_entry_count_method<nct_entry_count_no_entries>::value
                   == true);

    // absent
    D_NC_CHECK(has_entry_count_method<nct_tree>::value          == false);
    D_NC_CHECK(has_entry_count_method<nct_list>::value          == false);
    D_NC_CHECK(has_entry_count_method<nct_all_shapes>::value    == false);
    D_NC_CHECK(has_entry_count_method<nct_shared_container>::value == false);
    D_NC_CHECK(has_entry_count_method<nct_bare>::value          == false);
    D_NC_CHECK(has_entry_count_method<int>::value               == false);

    // independent of entries(), in both directions
    D_NC_CHECK(   ( has_entry_count_method<nct_shared_container>::value
                        == false )
               && ( has_entries_method<nct_shared_container>::value
                        == true ));
    D_NC_CHECK(   ( has_entry_count_method<nct_entry_count_no_entries>::value
                        == true )
               && ( has_entries_method<nct_entry_count_no_entries>::value
                        == false ));

    // the count is not a root
    D_NC_CHECK(has_any_entry_point<nct_entry_count_no_entries>::value
                   == false);

    return ok;
}

/*
tests_has_has_entry_method
  Verifies the probe for has_entry() - the interrogative for the generic
  entry point.
  Tests the following:
  - the fixtures that declare it are detected
  - entry_point() does not imply has_entry(), nor the reverse
  - the interrogative does not feed the entry disjunction
*/
bool
tests_has_has_entry_method()
{
    bool ok = true;

    // present
    D_NC_CHECK(has_has_entry_method<nct_tree>::value    == true);
    D_NC_CHECK(has_has_entry_method<nct_generic>::value == true);
    D_NC_CHECK(has_has_entry_method<nct_has_entry_no_entry_point>::value
                   == true);

    // absent
    D_NC_CHECK(has_has_entry_method<nct_list>::value       == false);
    D_NC_CHECK(has_has_entry_method<nct_graph>::value      == false);
    D_NC_CHECK(has_has_entry_method<nct_all_shapes>::value == false);
    D_NC_CHECK(has_has_entry_method<nct_bare>::value       == false);
    D_NC_CHECK(has_has_entry_method<int>::value            == false);

    // independent of entry_point(), in both directions
    D_NC_CHECK(   ( has_has_entry_method<nct_list>::value        == false )
               && ( has_entry_point_method<nct_list>::value      == true ));
    D_NC_CHECK(   ( has_has_entry_method<
                        nct_has_entry_no_entry_point>::value     == true )
               && ( has_entry_point_method<
                        nct_has_entry_no_entry_point>::value     == false ));

    // the interrogative is not a root
    D_NC_CHECK(has_any_entry_point<nct_has_entry_no_entry_point>::value
                   == false);

    return ok;
}

/*
tests_has_release_entry_method
  Verifies the probe for release_entry() - the hatch that relinquishes
  ownership of the entry.  This is the ONLY section II probe phrased on a
  modifiable object, since releasing mutates.
  Tests the following:
  - the fixtures that declare it are detected, whether or not the declaration
    is itself const-qualified
  - the release hatch does not feed the entry disjunction
  - it is independent of the ownership traits: a fixture may release without
    naming a policy at all
*/
bool
tests_has_release_entry_method()
{
    bool ok = true;

    // present
    D_NC_CHECK(has_release_entry_method<nct_tree>::value            == true);
    D_NC_CHECK(has_release_entry_method<nct_generic>::value         == true);
    D_NC_CHECK(has_release_entry_method<nct_release_no_entry>::value == true);
    D_NC_CHECK(has_release_entry_method<nct_const_release>::value   == true);

    // absent
    D_NC_CHECK(has_release_entry_method<nct_list>::value       == false);
    D_NC_CHECK(has_release_entry_method<nct_graph>::value      == false);
    D_NC_CHECK(has_release_entry_method<nct_all_shapes>::value == false);
    D_NC_CHECK(has_release_entry_method<nct_bare>::value       == false);
    D_NC_CHECK(has_release_entry_method<int>::value            == false);
    D_NC_CHECK(has_release_entry_method<void>::value           == false);

    // releasing is not a root
    D_NC_CHECK(   ( has_release_entry_method<nct_release_no_entry>::value
                        == true )
               && ( has_any_entry_point<nct_release_no_entry>::value
                        == false ));

    // independent of the ownership traits
    D_NC_CHECK(   ( has_release_entry_method<nct_generic>::value == true )
               && ( has_ownership_policy<nct_generic>::value     == false ));

    return ok;
}

/*
tests_entry_point_method_shapes
  Verifies WHAT the method probes admit, using root() as the representative.
  The question they ask is "does `x.root()` compile on a suitably qualified x",
  which is narrower than "declares a member named root" in some directions and
  wider in others.
  Tests the following:
  - a void return counts: the probe names the expression's type and never
    constrains it
  - a private member does NOT count - access control participates in the probe
  - a required parameter breaks the nullary call; a DEFAULTED one does not
  - a data member of pointer-to-node type does not count, since it cannot be
    called
  - a data member of POINTER-TO-FUNCTION type DOES count, since `x.root()`
    calls through it - the probe tests callability, not member-function-ness
  - a static member function counts, being callable through an object
  - an INHERITED root() counts: the traits are structural, so where the member
    was declared is immaterial
  - overload sets and member templates resolve normally
  - the admitted shapes propagate: each promotes its fixture all the way to
    tree-shaped
*/
bool
tests_entry_point_method_shapes()
{
    bool ok = true;

    // admitted
    D_NC_CHECK(has_root_method<nct_void_root>::value        == true);
    D_NC_CHECK(has_root_method<nct_default_arg_root>::value == true);
    D_NC_CHECK(has_root_method<nct_fnptr_root>::value       == true);
    D_NC_CHECK(has_root_method<nct_static_root>::value      == true);
    D_NC_CHECK(has_root_method<nct_inherited_root>::value   == true);
    D_NC_CHECK(has_root_method<nct_overloaded_root>::value  == true);
    D_NC_CHECK(has_root_method<nct_template_root>::value    == true);

    // rejected
    D_NC_CHECK(has_root_method<nct_private_root>::value     == false);
    D_NC_CHECK(has_root_method<nct_arg_root>::value         == false);
    D_NC_CHECK(has_root_method<nct_data_member_root>::value == false);
    D_NC_CHECK(has_root_method<nct_nonconst_root>::value    == false);

    // the distinction between the two data members is callability alone
    D_NC_CHECK(   ( has_root_method<nct_fnptr_root>::value       == true )
               && ( has_root_method<nct_data_member_root>::value == false ));

    // the distinction between the two parameter lists is the nullary call
    D_NC_CHECK(   ( has_root_method<nct_default_arg_root>::value == true )
               && ( has_root_method<nct_arg_root>::value         == false ));

    // and every admitted shape carries through to the classification
    D_NC_CHECK(is_tree_shaped_container<nct_void_root>::value        == true);
    D_NC_CHECK(is_tree_shaped_container<nct_default_arg_root>::value == true);
    D_NC_CHECK(is_tree_shaped_container<nct_fnptr_root>::value       == true);
    D_NC_CHECK(is_tree_shaped_container<nct_static_root>::value      == true);
    D_NC_CHECK(is_tree_shaped_container<nct_inherited_root>::value   == true);
    D_NC_CHECK(is_tree_shaped_container<nct_overloaded_root>::value  == true);
    D_NC_CHECK(is_tree_shaped_container<nct_template_root>::value    == true);

    D_NC_CHECK(is_node_container<nct_private_root>::value     == false);
    D_NC_CHECK(is_node_container<nct_arg_root>::value         == false);
    D_NC_CHECK(is_node_container<nct_data_member_root>::value == false);

    return ok;
}

/*
tests_has_any_entry_point
  Verifies the disjunction that decides whether a type holds a root at all.
  Tests the following:
  - each of the FOUR contributing probes promotes on its own: entry_point,
    root, head, entries
  - none of the other five does: has_root, tail, entry_count, has_entry and
    release_entry each leave the disjunction dark, because none of them names
    a root
  - the trait equals its definition, term by term, across carriers that light
    different subsets
  - it does not require node_type, so it can hold for a type that is not a
    node container
*/
bool
tests_has_any_entry_point()
{
    bool ok = true;

    // each contributing probe promotes alone
    D_NC_CHECK(has_any_entry_point<nct_entry_point_only_no_node>::value
                   == true);
    D_NC_CHECK(has_any_entry_point<nct_root_only>::value            == true);
    D_NC_CHECK(has_any_entry_point<nct_head_only_no_node>::value    == true);
    D_NC_CHECK(has_any_entry_point<nct_entries_only_no_node>::value == true);

    // the five non-contributors leave it dark
    D_NC_CHECK(has_any_entry_point<nct_has_root_no_root>::value == false);
    D_NC_CHECK(has_any_entry_point<nct_tail_no_head>::value     == false);
    D_NC_CHECK(has_any_entry_point<nct_entry_count_no_entries>::value
                   == false);
    D_NC_CHECK(has_any_entry_point<nct_has_entry_no_entry_point>::value
                   == false);
    D_NC_CHECK(has_any_entry_point<nct_release_no_entry>::value == false);

    // the realistic shapes all hold a root
    D_NC_CHECK(has_any_entry_point<nct_tree>::value    == true);
    D_NC_CHECK(has_any_entry_point<nct_list>::value    == true);
    D_NC_CHECK(has_any_entry_point<nct_slist>::value   == true);
    D_NC_CHECK(has_any_entry_point<nct_graph>::value   == true);
    D_NC_CHECK(has_any_entry_point<nct_generic>::value == true);

    // and the empty ones do not
    D_NC_CHECK(has_any_entry_point<nct_bare>::value      == false);
    D_NC_CHECK(has_any_entry_point<nct_node_only>::value == false);
    D_NC_CHECK(has_any_entry_point<int>::value           == false);
    D_NC_CHECK(has_any_entry_point<void>::value          == false);
    D_NC_CHECK(has_any_entry_point<nonesuch>::value      == false);

    // term-by-term agreement with the definition
    D_NC_CHECK(has_any_entry_point<nct_tree>::value
                   == (    has_entry_point_method<nct_tree>::value
                        || has_root_method<nct_tree>::value
                        || has_head_method<nct_tree>::value
                        || has_entries_method<nct_tree>::value ));
    D_NC_CHECK(has_any_entry_point<nct_graph>::value
                   == (    has_entry_point_method<nct_graph>::value
                        || has_root_method<nct_graph>::value
                        || has_head_method<nct_graph>::value
                        || has_entries_method<nct_graph>::value ));
    D_NC_CHECK(has_any_entry_point<nct_tail_no_head>::value
                   == (    has_entry_point_method<nct_tail_no_head>::value
                        || has_root_method<nct_tail_no_head>::value
                        || has_head_method<nct_tail_no_head>::value
                        || has_entries_method<nct_tail_no_head>::value ));

    // a root without a node_type is still a root
    D_NC_CHECK(   ( has_any_entry_point<nct_root_only>::value == true )
               && ( has_node_type<nct_root_only>::value       == false ));

    return ok;
}

/*
tests_entry_point_probe_qualification
  Verifies the qualification contract of section II, which differs from the
  clean_t contract the rest of the header keeps.  These probes name the type
  directly - `const _Type&` for the observers, `_Type&` for release_entry -
  rather than stripping it first, so the answer depends on the spelling.
  Tests the following:
  - the observers hold for the five non-volatile spellings and go dark for both
    volatile ones, since no fixture declares a const volatile member
  - release_entry, phrased on a modifiable object, additionally goes dark for
    every const spelling when the member is non-const
  - a CONST-qualified release_entry follows the observer pattern instead, so
    the mutable phrasing is a lower bound rather than a requirement of
    non-constness
  - the disjunction inherits whichever pattern its contributors have
*/
bool
tests_entry_point_probe_qualification()
{
    bool ok = true;

    // the observers: const-probed
    D_NC_CONST_PROBE_AGREE(has_entry_point_method, nct_generic, true);
    D_NC_CONST_PROBE_AGREE(has_root_method,        nct_tree,    true);
    D_NC_CONST_PROBE_AGREE(has_root_member_method, nct_tree,    true);
    D_NC_CONST_PROBE_AGREE(has_head_method,        nct_list,    true);
    D_NC_CONST_PROBE_AGREE(has_tail_method,        nct_list,    true);
    D_NC_CONST_PROBE_AGREE(has_entries_method,     nct_graph,   true);
    D_NC_CONST_PROBE_AGREE(has_entry_count_method, nct_graph,   true);
    D_NC_CONST_PROBE_AGREE(has_has_entry_method,   nct_generic, true);

    // negative carriers answer false in every spelling
    D_NC_CONST_PROBE_AGREE(has_root_method,    nct_list, false);
    D_NC_CONST_PROBE_AGREE(has_entries_method, nct_list, false);

    // release_entry: mutable-probed, and the member is non-const
    D_NC_MUTABLE_PROBE_AGREE(has_release_entry_method, nct_tree,    true);
    D_NC_MUTABLE_PROBE_AGREE(has_release_entry_method, nct_generic, true);
    D_NC_MUTABLE_PROBE_AGREE(has_release_entry_method, nct_list,    false);

    // a const-qualified release_entry is callable on a const object too, so it
    // follows the observer pattern despite the mutable phrasing
    D_NC_CONST_PROBE_AGREE(has_release_entry_method, nct_const_release, true);

    // the disjunction inherits the observer pattern
    D_NC_CONST_PROBE_AGREE(has_any_entry_point, nct_tree,  true);
    D_NC_CONST_PROBE_AGREE(has_any_entry_point, nct_graph, true);
    D_NC_CONST_PROBE_AGREE(has_any_entry_point, nct_bare,  false);

    return ok;
}

/*
tests_entry_point_companions
  Verifies the `_v` companions of the section II probes.
  Tests the following:
  - each of the nine method companions agrees with its trait's ::value
  - the disjunction's companion agrees likewise
  - the companions are usable in a constant expression
  - gated on variable-template support, as in section I
*/
bool
tests_entry_point_companions()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_NC_STATIC_CHECK(has_root_method_v<nct_tree>       == true);
    D_NC_STATIC_CHECK(has_any_entry_point_v<nct_bare>   == false);

    D_NC_CHECK(has_entry_point_method_v<nct_generic>
                   == has_entry_point_method<nct_generic>::value);
    D_NC_CHECK(has_root_method_v<nct_tree>
                   == has_root_method<nct_tree>::value);
    D_NC_CHECK(has_root_member_method_v<nct_tree>
                   == has_root_member_method<nct_tree>::value);
    D_NC_CHECK(has_head_method_v<nct_list>
                   == has_head_method<nct_list>::value);
    D_NC_CHECK(has_tail_method_v<nct_list>
                   == has_tail_method<nct_list>::value);
    D_NC_CHECK(has_entries_method_v<nct_graph>
                   == has_entries_method<nct_graph>::value);
    D_NC_CHECK(has_entry_count_method_v<nct_graph>
                   == has_entry_count_method<nct_graph>::value);
    D_NC_CHECK(has_has_entry_method_v<nct_generic>
                   == has_has_entry_method<nct_generic>::value);
    D_NC_CHECK(has_release_entry_method_v<nct_tree>
                   == has_release_entry_method<nct_tree>::value);
    D_NC_CHECK(has_any_entry_point_v<nct_tree>
                   == has_any_entry_point<nct_tree>::value);

    // negative carriers
    D_NC_CHECK(has_entry_point_method_v<nct_graph>   == false);
    D_NC_CHECK(has_root_method_v<nct_list>           == false);
    D_NC_CHECK(has_head_method_v<nct_tree>           == false);
    D_NC_CHECK(has_tail_method_v<nct_slist>          == false);
    D_NC_CHECK(has_entries_method_v<nct_tree>        == false);
    D_NC_CHECK(has_entry_count_method_v<nct_tree>    == false);
    D_NC_CHECK(has_has_entry_method_v<nct_list>      == false);
    D_NC_CHECK(has_release_entry_method_v<nct_list>  == false);
    D_NC_CHECK(has_any_entry_point_v<nct_node_only>  == false);

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
