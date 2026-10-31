/*******************************************************************************
* djinterp [cpp]                        node_container_traits_tests_identity.cpp
*
*
* path:      /tests/djinterp/cpp/container/traits/node_container_traits_tests_identity.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "node_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// V.   Node container identity
// ===========================================================================
//   The structural minimum - a node_type alias AND some entry reference - and
// the four shape predicates layered on it.  The shapes are NOT a partition:
// they are independent conjunctions over the same base, so a type may answer
// to several at once.  Resolving that overlap is section VI's job, not theirs.


/*
tests_is_node_container
  Verifies the structural minimum.
  Tests the following:
  - every realistic silhouette qualifies: tree, list, singly linked list,
    graph, and the topology-neutral base
  - BOTH halves are required.  A type with node_type and no entry reference
    does not qualify, and neither does one with an entry reference and no
    node_type - the two isolators fail on opposite sides
  - the five members that are not entry references cannot supply the second
    half: has_root, tail, entry_count, has_entry and release_entry each leave
    an otherwise-complete fixture short
  - the trait equals its definition, term by term
  - non-containers do not qualify, and void does not hard-error
*/
bool tests_is_node_container()
{
    bool ok = true;

    // the realistic silhouettes
    D_NC_CHECK(is_node_container<DNctTree>::value    == true);
    D_NC_CHECK(is_node_container<DNctList>::value    == true);
    D_NC_CHECK(is_node_container<DNctSlist>::value   == true);
    D_NC_CHECK(is_node_container<DNctGraph>::value   == true);
    D_NC_CHECK(is_node_container<DNctGeneric>::value == true);
    D_NC_CHECK(is_node_container<DNctUniqueContainer>::value     == true);
    D_NC_CHECK(is_node_container<DNctSharedContainer>::value     == true);
    D_NC_CHECK(is_node_container<DNctNonOwningContainer>::value == true);

    // each half alone is not enough
    D_NC_CHECK(is_node_container<DNctNodeOnly>::value == false);
    D_NC_CHECK(is_node_container<DNctRootOnly>::value == false);
    D_NC_CHECK(is_node_container<DNctHeadOnlyNoNode>::value        == false);
    D_NC_CHECK(is_node_container<DNctEntriesOnlyNoNode>::value     == false);
    D_NC_CHECK(is_node_container<DNctEntryPointOnlyNoNode>::value == false);

    // ... and the two isolators fail on opposite sides
    D_NC_CHECK(   ( has_node_type<DNctNodeOnly>::value       == true )
               && ( has_any_entry_point<DNctNodeOnly>::value == false ));
    D_NC_CHECK(   ( has_node_type<DNctRootOnly>::value       == false )
               && ( has_any_entry_point<DNctRootOnly>::value == true ));

    // the five non-entry members cannot complete the pair
    D_NC_CHECK(is_node_container<nct_has_root_no_root>::value         == false);
    D_NC_CHECK(is_node_container<DNctTailNoHead>::value             == false);
    D_NC_CHECK(is_node_container<DNctEntryCountNoEntries>::value   == false);
    D_NC_CHECK(is_node_container<nct_has_entry_no_entry_point>::value == false);
    D_NC_CHECK(is_node_container<DNctReleaseNoEntry>::value         == false);

    // an alias naming void or nonesuch still satisfies the first half
    D_NC_CHECK(is_node_container<nct_void_node_type>::value     == true);
    D_NC_CHECK(is_node_container<nct_nonesuch_node_type>::value == true);

    // non-containers
    D_NC_CHECK(is_node_container<DNctBare>::value == false);
    D_NC_CHECK(is_node_container<DNctNode>::value == false);
    D_NC_CHECK(is_node_container<int>::value      == false);
    D_NC_CHECK(is_node_container<void>::value     == false);
    D_NC_CHECK(is_node_container<int*>::value     == false);
    D_NC_CHECK(is_node_container<nonesuch>::value == false);

    // term-by-term agreement with the definition
    D_NC_CHECK(is_node_container<DNctTree>::value
                   == (    has_node_type<DNctTree>::value
                        && has_any_entry_point<DNctTree>::value ));
    D_NC_CHECK(is_node_container<DNctNodeOnly>::value
                   == (    has_node_type<DNctNodeOnly>::value
                        && has_any_entry_point<DNctNodeOnly>::value ));
    D_NC_CHECK(is_node_container<DNctRootOnly>::value
                   == (    has_node_type<DNctRootOnly>::value
                        && has_any_entry_point<DNctRootOnly>::value ));

    return ok;
}

/*
tests_is_tree_shaped_container
  Verifies the tree predicate - a node container with a single root entry.
  Tests the following:
  - the tree fixtures qualify, including every admitted root() shape
  - lists, graphs and the neutral base do not
  - root() alone is not enough: without node_type the base requirement fails,
    so the predicate is a genuine conjunction rather than an alias for the
    root probe
  - has_root() does not substitute for root()
  - the predicate equals its definition
*/
bool tests_is_tree_shaped_container()
{
    bool ok = true;

    // positive
    D_NC_CHECK(is_tree_shaped_container<DNctTree>::value             == true);
    D_NC_CHECK(is_tree_shaped_container<DNctUniqueContainer>::value == true);
    D_NC_CHECK(is_tree_shaped_container<DNctTreeAndList>::value    == true);
    D_NC_CHECK(is_tree_shaped_container<DNctTreeAndGraph>::value   == true);
    D_NC_CHECK(is_tree_shaped_container<DNctAllShapes>::value       == true);

    // other shapes
    D_NC_CHECK(is_tree_shaped_container<DNctList>::value    == false);
    D_NC_CHECK(is_tree_shaped_container<DNctSlist>::value   == false);
    D_NC_CHECK(is_tree_shaped_container<DNctGraph>::value   == false);
    D_NC_CHECK(is_tree_shaped_container<DNctGeneric>::value == false);

    // the base requirement is real
    D_NC_CHECK(   ( has_root_method<DNctRootOnly>::value       == true )
               && ( is_tree_shaped_container<DNctRootOnly>::value
                        == false ));

    // the interrogative does not substitute
    D_NC_CHECK(is_tree_shaped_container<nct_has_root_no_root>::value
                   == false);

    // non-containers
    D_NC_CHECK(is_tree_shaped_container<DNctBare>::value == false);
    D_NC_CHECK(is_tree_shaped_container<int>::value      == false);
    D_NC_CHECK(is_tree_shaped_container<void>::value     == false);

    // agreement with the definition
    D_NC_CHECK(is_tree_shaped_container<DNctTree>::value
                   == (    is_node_container<DNctTree>::value
                        && has_root_method<DNctTree>::value ));
    D_NC_CHECK(is_tree_shaped_container<DNctRootOnly>::value
                   == (    is_node_container<DNctRootOnly>::value
                        && has_root_method<DNctRootOnly>::value ));

    return ok;
}

/*
tests_is_list_shaped_container
  Verifies the list predicate - a node container with a head entry.
  Tests the following:
  - both list fixtures qualify, singly and doubly linked alike, since only
    head() is required
  - trees, graphs and the neutral base do not
  - tail() without head() does not qualify
  - head() without node_type does not qualify
  - the predicate equals its definition
*/
bool tests_is_list_shaped_container()
{
    bool ok = true;

    // positive - head() is the whole requirement
    D_NC_CHECK(is_list_shaped_container<DNctList>::value           == true);
    D_NC_CHECK(is_list_shaped_container<DNctSlist>::value          == true);
    D_NC_CHECK(is_list_shaped_container<DNctTreeAndList>::value  == true);
    D_NC_CHECK(is_list_shaped_container<DNctListAndGraph>::value == true);
    D_NC_CHECK(is_list_shaped_container<DNctAllShapes>::value     == true);
    D_NC_CHECK(is_list_shaped_container<DNctNonOwningContainer>::value
                   == true);

    // other shapes
    D_NC_CHECK(is_list_shaped_container<DNctTree>::value    == false);
    D_NC_CHECK(is_list_shaped_container<DNctGraph>::value   == false);
    D_NC_CHECK(is_list_shaped_container<DNctGeneric>::value == false);

    // tail alone is not a head
    D_NC_CHECK(is_list_shaped_container<DNctTailNoHead>::value == false);

    // the base requirement is real
    D_NC_CHECK(   ( has_head_method<DNctHeadOnlyNoNode>::value == true )
               && ( is_list_shaped_container<DNctHeadOnlyNoNode>::value
                        == false ));

    // non-containers
    D_NC_CHECK(is_list_shaped_container<DNctBare>::value == false);
    D_NC_CHECK(is_list_shaped_container<int>::value      == false);
    D_NC_CHECK(is_list_shaped_container<void>::value     == false);

    // agreement with the definition
    D_NC_CHECK(is_list_shaped_container<DNctSlist>::value
                   == (    is_node_container<DNctSlist>::value
                        && has_head_method<DNctSlist>::value ));
    D_NC_CHECK(is_list_shaped_container<DNctTailNoHead>::value
                   == (    is_node_container<DNctTailNoHead>::value
                        && has_head_method<DNctTailNoHead>::value ));

    return ok;
}

/*
tests_is_doubly_linked_container
  Verifies the doubly linked predicate - list-shaped AND carrying tail().
  This is the only shape predicate layered on another shape rather than on the
  base, so it is the one place where the shape lattice has two levels.
  Tests the following:
  - the doubly linked fixture qualifies; the singly linked one does not
  - tail() without head() does not qualify, because list-shaped fails first -
    so a back reference can never on its own make a container doubly linked
  - it implies list-shaped, and never the reverse
  - the predicate equals its definition
*/
bool tests_is_doubly_linked_container()
{
    bool ok = true;

    // positive
    D_NC_CHECK(is_doubly_linked_container<DNctList>::value       == true);
    D_NC_CHECK(is_doubly_linked_container<DNctAllShapes>::value == true);

    // head without tail
    D_NC_CHECK(is_doubly_linked_container<DNctSlist>::value == false);
    D_NC_CHECK(is_doubly_linked_container<DNctTreeAndList>::value
                   == false);
    D_NC_CHECK(is_doubly_linked_container<DNctListAndGraph>::value
                   == false);

    // tail without head
    D_NC_CHECK(is_doubly_linked_container<DNctTailNoHead>::value == false);

    // ... and the decisive case: a genuine node container, with a tail, that
    // has no head.  It satisfies identity and has_tail, so a trait layered on
    // is_node_container would call it doubly linked; the real one is layered
    // on is_list_shaped_container, which fails first
    D_NC_CHECK(   ( is_node_container<DNctRootedTailNoHead>::value
                        == true )
               && ( has_tail_method<DNctRootedTailNoHead>::value
                        == true )
               && ( is_list_shaped_container<DNctRootedTailNoHead>::value
                        == false )
               && ( is_doubly_linked_container<DNctRootedTailNoHead>::value
                        == false ));

    // other shapes
    D_NC_CHECK(is_doubly_linked_container<DNctTree>::value  == false);
    D_NC_CHECK(is_doubly_linked_container<DNctGraph>::value == false);
    D_NC_CHECK(is_doubly_linked_container<DNctBare>::value  == false);
    D_NC_CHECK(is_doubly_linked_container<int>::value       == false);

    // it implies list-shaped, never the reverse
    D_NC_CHECK(   ( is_doubly_linked_container<DNctList>::value == true )
               && ( is_list_shaped_container<DNctList>::value   == true ));
    D_NC_CHECK(   ( is_list_shaped_container<DNctSlist>::value    == true )
               && ( is_doubly_linked_container<DNctSlist>::value  == false ));

    // agreement with the definition
    D_NC_CHECK(is_doubly_linked_container<DNctList>::value
                   == (    is_list_shaped_container<DNctList>::value
                        && has_tail_method<DNctList>::value ));
    D_NC_CHECK(is_doubly_linked_container<DNctTailNoHead>::value
                   == (    is_list_shaped_container<DNctTailNoHead>::value
                        && has_tail_method<DNctTailNoHead>::value ));

    return ok;
}

/*
tests_is_graph_shaped_container
  Verifies the graph predicate - a node container exposing the entry SET rho.
  Tests the following:
  - the graph fixtures qualify
  - trees, lists and the neutral base do not
  - entry_count() without entries() does not qualify: the cardinality of rho
    is not rho
  - entries() without node_type does not qualify
  - the predicate equals its definition
*/
bool tests_is_graph_shaped_container()
{
    bool ok = true;

    // positive
    D_NC_CHECK(is_graph_shaped_container<DNctGraph>::value            == true);
    D_NC_CHECK(is_graph_shaped_container<DNctSharedContainer>::value == true);
    D_NC_CHECK(is_graph_shaped_container<DNctListAndGraph>::value   == true);
    D_NC_CHECK(is_graph_shaped_container<DNctTreeAndGraph>::value   == true);
    D_NC_CHECK(is_graph_shaped_container<DNctAllShapes>::value       == true);

    // other shapes
    D_NC_CHECK(is_graph_shaped_container<DNctTree>::value    == false);
    D_NC_CHECK(is_graph_shaped_container<DNctList>::value    == false);
    D_NC_CHECK(is_graph_shaped_container<DNctGeneric>::value == false);

    // the cardinality is not the set
    D_NC_CHECK(is_graph_shaped_container<DNctEntryCountNoEntries>::value
                   == false);

    // the base requirement is real
    D_NC_CHECK(   ( has_entries_method<DNctEntriesOnlyNoNode>::value
                        == true )
               && ( is_graph_shaped_container<
                        DNctEntriesOnlyNoNode>::value == false ));

    // non-containers
    D_NC_CHECK(is_graph_shaped_container<DNctBare>::value == false);
    D_NC_CHECK(is_graph_shaped_container<int>::value      == false);
    D_NC_CHECK(is_graph_shaped_container<void>::value     == false);

    // agreement with the definition
    D_NC_CHECK(is_graph_shaped_container<DNctGraph>::value
                   == (    is_node_container<DNctGraph>::value
                        && has_entries_method<DNctGraph>::value ));
    D_NC_CHECK(is_graph_shaped_container<DNctEntriesOnlyNoNode>::value
                   == (    is_node_container<
                               DNctEntriesOnlyNoNode>::value
                        && has_entries_method<
                               DNctEntriesOnlyNoNode>::value ));

    return ok;
}

/*
tests_identity_composition_shape
  Verifies the SHAPE of the composition, not merely its per-type answers: each
  predicate is its base conjoined with exactly one marker.
  Tests the following:
  - all four identities hold across carriers that light different subsets, so a
    dropped, duplicated or swapped conjunct cannot pass
  - every shape predicate implies is_node_container, without exception
  - no shape predicate holds for a type that fails the base, however many
    markers it carries
*/
bool tests_identity_composition_shape()
{
    bool ok = true;

    // tree = base AND root
    D_NC_CHECK(is_tree_shaped_container<DNctAllShapes>::value
                   == (    is_node_container<DNctAllShapes>::value
                        && has_root_method<DNctAllShapes>::value ));
    D_NC_CHECK(is_tree_shaped_container<DNctGraph>::value
                   == (    is_node_container<DNctGraph>::value
                        && has_root_method<DNctGraph>::value ));

    // list = base AND head
    D_NC_CHECK(is_list_shaped_container<DNctAllShapes>::value
                   == (    is_node_container<DNctAllShapes>::value
                        && has_head_method<DNctAllShapes>::value ));
    D_NC_CHECK(is_list_shaped_container<DNctTree>::value
                   == (    is_node_container<DNctTree>::value
                        && has_head_method<DNctTree>::value ));

    // doubly linked = list AND tail
    D_NC_CHECK(is_doubly_linked_container<DNctAllShapes>::value
                   == (    is_list_shaped_container<DNctAllShapes>::value
                        && has_tail_method<DNctAllShapes>::value ));
    D_NC_CHECK(is_doubly_linked_container<DNctSlist>::value
                   == (    is_list_shaped_container<DNctSlist>::value
                        && has_tail_method<DNctSlist>::value ));
    D_NC_CHECK(is_doubly_linked_container<DNctRootedTailNoHead>::value
                   == (    is_list_shaped_container<
                               DNctRootedTailNoHead>::value
                        && has_tail_method<
                               DNctRootedTailNoHead>::value ));

    // graph = base AND entries
    D_NC_CHECK(is_graph_shaped_container<DNctAllShapes>::value
                   == (    is_node_container<DNctAllShapes>::value
                        && has_entries_method<DNctAllShapes>::value ));
    D_NC_CHECK(is_graph_shaped_container<DNctList>::value
                   == (    is_node_container<DNctList>::value
                        && has_entries_method<DNctList>::value ));

    // every shape implies the base
    D_NC_CHECK(!is_tree_shaped_container<DNctTree>::value
                   || is_node_container<DNctTree>::value);
    D_NC_CHECK(!is_list_shaped_container<DNctList>::value
                   || is_node_container<DNctList>::value);
    D_NC_CHECK(!is_graph_shaped_container<DNctGraph>::value
                   || is_node_container<DNctGraph>::value);
    D_NC_CHECK(!is_doubly_linked_container<DNctList>::value
                   || is_node_container<DNctList>::value);

    // and a type that fails the base has no shape at all, however many
    // markers it carries
    D_NC_CHECK(   ( has_root_method<DNctRootOnly>::value    == true )
               && ( nc_shape_predicate_count<DNctRootOnly>() == 0 ));
    D_NC_CHECK(   ( has_head_method<DNctHeadOnlyNoNode>::value == true )
               && ( nc_shape_predicate_count<DNctHeadOnlyNoNode>()
                        == 0 ));
    D_NC_CHECK(   ( has_entries_method<DNctEntriesOnlyNoNode>::value
                        == true )
               && ( nc_shape_predicate_count<DNctEntriesOnlyNoNode>()
                        == 0 ));

    return ok;
}

/*
tests_identity_overlap
  Verifies that the shape predicates OVERLAP rather than partition.  Nothing in
  the header forbids a container from exposing root() and head() together, and
  several predicates then hold at once; it is node_container_shape_of that
  picks one, which is why the two must be tested separately.
  Tests the following:
  - the single-shape fixtures light exactly one predicate, except the doubly
    linked list which lights two by construction (list and doubly linked)
  - each overlapping fixture lights the pair it was built to light
  - the maximal fixture lights all four
  - a type that fails the base lights none
*/
bool tests_identity_overlap()
{
    bool ok = true;

    // the unambiguous shapes
    D_NC_CHECK(nc_shape_predicate_count<DNctTree>()    == 1);
    D_NC_CHECK(nc_shape_predicate_count<DNctSlist>()   == 1);
    D_NC_CHECK(nc_shape_predicate_count<DNctGraph>()   == 1);
    D_NC_CHECK(nc_shape_predicate_count<DNctGeneric>() == 0);

    // a doubly linked list is list-shaped AND doubly linked
    D_NC_CHECK(nc_shape_predicate_count<DNctList>() == 2);
    D_NC_CHECK(   ( is_list_shaped_container<DNctList>::value   == true )
               && ( is_doubly_linked_container<DNctList>::value == true ));

    // the overlapping fixtures
    D_NC_CHECK(nc_shape_predicate_count<DNctTreeAndList>() == 2);
    D_NC_CHECK(   ( is_tree_shaped_container<DNctTreeAndList>::value
                        == true )
               && ( is_list_shaped_container<DNctTreeAndList>::value
                        == true ));

    D_NC_CHECK(nc_shape_predicate_count<DNctListAndGraph>() == 2);
    D_NC_CHECK(   ( is_list_shaped_container<DNctListAndGraph>::value
                        == true )
               && ( is_graph_shaped_container<DNctListAndGraph>::value
                        == true ));

    D_NC_CHECK(nc_shape_predicate_count<DNctTreeAndGraph>() == 2);
    D_NC_CHECK(   ( is_tree_shaped_container<DNctTreeAndGraph>::value
                        == true )
               && ( is_graph_shaped_container<DNctTreeAndGraph>::value
                        == true ));

    // the maximal fixture
    D_NC_CHECK(nc_shape_predicate_count<DNctAllShapes>() == 4);

    // and nothing at all for a non-container
    D_NC_CHECK(nc_shape_predicate_count<DNctBare>() == 0);
    D_NC_CHECK(nc_shape_predicate_count<int>()      == 0);
    D_NC_CHECK(nc_shape_predicate_count<void>()     == 0);

    return ok;
}

/*
tests_node_container_cv_ref_divergence
  Verifies - and pins - the fact that this header does NOT answer cv- and
  reference-qualified types uniformly, because its sections phrase their
  questions differently.  Sections I, III and IV write clean_t<_Type> and strip
  everything; section II writes `const _Type&` and strips nothing.  The
  composites inherit section II's phrasing, so the divergence surfaces at
  is_node_container and everything above it.

  Two consequences are recorded here, both surprising:

    1. A VOLATILE container is not a node container.  The accessors are not
       declared const volatile, so the section II probes go dark and identity
       collapses, even though the node_type alias is still perfectly findable.

    2. A REFERENCE is more permissive than the bare type.  `const _Type&` with
       _Type = X& collapses to X&, dropping the const, so a container whose
       root() is not const-qualified reads dark as X and lit as X& - the
       opposite of the usual expectation that qualification only removes
       capability.

  Tests the following:
  - both consequences, on dedicated fixtures
  - the contrast with sections I, III and IV on the same carriers, so the
    divergence is localised to section II rather than blamed on the header
    as a whole
  - the well-behaved case: for a container whose accessors ARE const-qualified,
    the five non-volatile spellings do agree
*/
bool tests_node_container_cv_ref_divergence()
{
    bool ok = true;

    // --- consequence 1: volatile collapses identity ---
    D_NC_CHECK(is_node_container<DNctTree>::value                  == true);
    D_NC_CHECK(is_node_container<volatile DNctTree>::value         == false);
    D_NC_CHECK(is_node_container<const volatile DNctTree>::value   == false);
    D_NC_CHECK(is_tree_shaped_container<volatile DNctTree>::value  == false);
    D_NC_CHECK(is_list_shaped_container<volatile DNctList>::value  == false);
    D_NC_CHECK(is_graph_shaped_container<volatile DNctGraph>::value == false);

    // ... while the clean_t sections are unmoved by the same qualification
    D_NC_CHECK(has_node_type<volatile DNctTree>::value          == true);
    D_NC_CHECK(has_ownership_policy<volatile DNctTree>::value    == true);
    D_NC_CHECK(has_entry_owns_constant<volatile DNctTree>::value == true);
    D_NC_CHECK(ownership_model_of<volatile DNctTree>::value
                   == DOwnershipModel::unique);
    D_NC_STATIC_CHECK(std::is_same<node_type_of_t<volatile DNctTree>,
                                   DNctNode>::value);

    // --- consequence 2: a reference is MORE permissive than the bare type ---
    D_NC_CHECK(has_root_method<DNctNonconstRoot>::value       == false);
    D_NC_CHECK(has_root_method<DNctNonconstRoot&>::value      == true);
    D_NC_CHECK(has_root_method<DNctNonconstRoot&&>::value     == true);
    D_NC_CHECK(has_root_method<const DNctNonconstRoot&>::value == false);

    // ... and it carries all the way up to identity and shape
    D_NC_CHECK(is_node_container<DNctNonconstRoot>::value       == false);
    D_NC_CHECK(is_node_container<DNctNonconstRoot&>::value      == true);
    D_NC_CHECK(is_tree_shaped_container<DNctNonconstRoot>::value  == false);
    D_NC_CHECK(is_tree_shaped_container<DNctNonconstRoot&>::value == true);

    // ... while the clean_t sections answer the same for both spellings
    D_NC_CHECK(has_node_type<DNctNonconstRoot>::value
                   == has_node_type<DNctNonconstRoot&>::value);

    // --- the well-behaved case, for contrast ---
    D_NC_CONST_PROBE_AGREE(is_node_container,        DNctTree,  true);
    D_NC_CONST_PROBE_AGREE(is_tree_shaped_container, DNctTree,  true);
    D_NC_CONST_PROBE_AGREE(is_list_shaped_container, DNctList,  true);
    D_NC_CONST_PROBE_AGREE(is_doubly_linked_container, DNctList, true);
    D_NC_CONST_PROBE_AGREE(is_graph_shaped_container, DNctGraph, true);
    D_NC_CONST_PROBE_AGREE(is_node_container,        DNctBare,  false);

    return ok;
}

/*
tests_identity_companions
  Verifies the `_v` companions of the section V traits.
  Tests the following:
  - each of the five companions agrees with its trait's ::value, on a positive
    and a negative carrier
  - the companions are usable in a constant expression
  - gated on variable-template support
*/
bool tests_identity_companions()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_NC_STATIC_CHECK(is_node_container_v<DNctTree>         == true);
    D_NC_STATIC_CHECK(is_graph_shaped_container_v<DNctTree> == false);

    D_NC_CHECK(is_node_container_v<DNctTree>
                   == is_node_container<DNctTree>::value);
    D_NC_CHECK(is_node_container_v<DNctBare>
                   == is_node_container<DNctBare>::value);

    D_NC_CHECK(is_tree_shaped_container_v<DNctTree>
                   == is_tree_shaped_container<DNctTree>::value);
    D_NC_CHECK(is_tree_shaped_container_v<DNctList>
                   == is_tree_shaped_container<DNctList>::value);

    D_NC_CHECK(is_list_shaped_container_v<DNctSlist>
                   == is_list_shaped_container<DNctSlist>::value);
    D_NC_CHECK(is_list_shaped_container_v<DNctTree>
                   == is_list_shaped_container<DNctTree>::value);

    D_NC_CHECK(is_doubly_linked_container_v<DNctList>
                   == is_doubly_linked_container<DNctList>::value);
    D_NC_CHECK(is_doubly_linked_container_v<DNctSlist>
                   == is_doubly_linked_container<DNctSlist>::value);

    D_NC_CHECK(is_graph_shaped_container_v<DNctGraph>
                   == is_graph_shaped_container<DNctGraph>::value);
    D_NC_CHECK(is_graph_shaped_container_v<DNctList>
                   == is_graph_shaped_container<DNctList>::value);

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
