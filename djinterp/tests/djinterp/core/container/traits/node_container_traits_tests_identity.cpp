/*******************************************************************************
* djinterp [core]                       node_container_traits_tests_identity.cpp
*
*
* path:      /tests/djinterp/core/container/traits/node_container_traits_tests_identity.cpp
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
bool
tests_is_node_container()
{
    bool ok = true;

    // the realistic silhouettes
    D_NC_CHECK(is_node_container<nct_tree>::value    == true);
    D_NC_CHECK(is_node_container<nct_list>::value    == true);
    D_NC_CHECK(is_node_container<nct_slist>::value   == true);
    D_NC_CHECK(is_node_container<nct_graph>::value   == true);
    D_NC_CHECK(is_node_container<nct_generic>::value == true);
    D_NC_CHECK(is_node_container<nct_unique_container>::value     == true);
    D_NC_CHECK(is_node_container<nct_shared_container>::value     == true);
    D_NC_CHECK(is_node_container<nct_non_owning_container>::value == true);

    // each half alone is not enough
    D_NC_CHECK(is_node_container<nct_node_only>::value == false);
    D_NC_CHECK(is_node_container<nct_root_only>::value == false);
    D_NC_CHECK(is_node_container<nct_head_only_no_node>::value        == false);
    D_NC_CHECK(is_node_container<nct_entries_only_no_node>::value     == false);
    D_NC_CHECK(is_node_container<nct_entry_point_only_no_node>::value == false);

    // ... and the two isolators fail on opposite sides
    D_NC_CHECK(   ( has_node_type<nct_node_only>::value       == true )
               && ( has_any_entry_point<nct_node_only>::value == false ));
    D_NC_CHECK(   ( has_node_type<nct_root_only>::value       == false )
               && ( has_any_entry_point<nct_root_only>::value == true ));

    // the five non-entry members cannot complete the pair
    D_NC_CHECK(is_node_container<nct_has_root_no_root>::value         == false);
    D_NC_CHECK(is_node_container<nct_tail_no_head>::value             == false);
    D_NC_CHECK(is_node_container<nct_entry_count_no_entries>::value   == false);
    D_NC_CHECK(is_node_container<nct_has_entry_no_entry_point>::value == false);
    D_NC_CHECK(is_node_container<nct_release_no_entry>::value         == false);

    // an alias naming void or nonesuch still satisfies the first half
    D_NC_CHECK(is_node_container<nct_void_node_type>::value     == true);
    D_NC_CHECK(is_node_container<nct_nonesuch_node_type>::value == true);

    // non-containers
    D_NC_CHECK(is_node_container<nct_bare>::value == false);
    D_NC_CHECK(is_node_container<nct_node>::value == false);
    D_NC_CHECK(is_node_container<int>::value      == false);
    D_NC_CHECK(is_node_container<void>::value     == false);
    D_NC_CHECK(is_node_container<int*>::value     == false);
    D_NC_CHECK(is_node_container<nonesuch>::value == false);

    // term-by-term agreement with the definition
    D_NC_CHECK(is_node_container<nct_tree>::value
                   == (    has_node_type<nct_tree>::value
                        && has_any_entry_point<nct_tree>::value ));
    D_NC_CHECK(is_node_container<nct_node_only>::value
                   == (    has_node_type<nct_node_only>::value
                        && has_any_entry_point<nct_node_only>::value ));
    D_NC_CHECK(is_node_container<nct_root_only>::value
                   == (    has_node_type<nct_root_only>::value
                        && has_any_entry_point<nct_root_only>::value ));

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
bool
tests_is_tree_shaped_container()
{
    bool ok = true;

    // positive
    D_NC_CHECK(is_tree_shaped_container<nct_tree>::value             == true);
    D_NC_CHECK(is_tree_shaped_container<nct_unique_container>::value == true);
    D_NC_CHECK(is_tree_shaped_container<nct_tree_and_list>::value    == true);
    D_NC_CHECK(is_tree_shaped_container<nct_tree_and_graph>::value   == true);
    D_NC_CHECK(is_tree_shaped_container<nct_all_shapes>::value       == true);

    // other shapes
    D_NC_CHECK(is_tree_shaped_container<nct_list>::value    == false);
    D_NC_CHECK(is_tree_shaped_container<nct_slist>::value   == false);
    D_NC_CHECK(is_tree_shaped_container<nct_graph>::value   == false);
    D_NC_CHECK(is_tree_shaped_container<nct_generic>::value == false);

    // the base requirement is real
    D_NC_CHECK(   ( has_root_method<nct_root_only>::value       == true )
               && ( is_tree_shaped_container<nct_root_only>::value
                        == false ));

    // the interrogative does not substitute
    D_NC_CHECK(is_tree_shaped_container<nct_has_root_no_root>::value
                   == false);

    // non-containers
    D_NC_CHECK(is_tree_shaped_container<nct_bare>::value == false);
    D_NC_CHECK(is_tree_shaped_container<int>::value      == false);
    D_NC_CHECK(is_tree_shaped_container<void>::value     == false);

    // agreement with the definition
    D_NC_CHECK(is_tree_shaped_container<nct_tree>::value
                   == (    is_node_container<nct_tree>::value
                        && has_root_method<nct_tree>::value ));
    D_NC_CHECK(is_tree_shaped_container<nct_root_only>::value
                   == (    is_node_container<nct_root_only>::value
                        && has_root_method<nct_root_only>::value ));

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
bool
tests_is_list_shaped_container()
{
    bool ok = true;

    // positive - head() is the whole requirement
    D_NC_CHECK(is_list_shaped_container<nct_list>::value           == true);
    D_NC_CHECK(is_list_shaped_container<nct_slist>::value          == true);
    D_NC_CHECK(is_list_shaped_container<nct_tree_and_list>::value  == true);
    D_NC_CHECK(is_list_shaped_container<nct_list_and_graph>::value == true);
    D_NC_CHECK(is_list_shaped_container<nct_all_shapes>::value     == true);
    D_NC_CHECK(is_list_shaped_container<nct_non_owning_container>::value
                   == true);

    // other shapes
    D_NC_CHECK(is_list_shaped_container<nct_tree>::value    == false);
    D_NC_CHECK(is_list_shaped_container<nct_graph>::value   == false);
    D_NC_CHECK(is_list_shaped_container<nct_generic>::value == false);

    // tail alone is not a head
    D_NC_CHECK(is_list_shaped_container<nct_tail_no_head>::value == false);

    // the base requirement is real
    D_NC_CHECK(   ( has_head_method<nct_head_only_no_node>::value == true )
               && ( is_list_shaped_container<nct_head_only_no_node>::value
                        == false ));

    // non-containers
    D_NC_CHECK(is_list_shaped_container<nct_bare>::value == false);
    D_NC_CHECK(is_list_shaped_container<int>::value      == false);
    D_NC_CHECK(is_list_shaped_container<void>::value     == false);

    // agreement with the definition
    D_NC_CHECK(is_list_shaped_container<nct_slist>::value
                   == (    is_node_container<nct_slist>::value
                        && has_head_method<nct_slist>::value ));
    D_NC_CHECK(is_list_shaped_container<nct_tail_no_head>::value
                   == (    is_node_container<nct_tail_no_head>::value
                        && has_head_method<nct_tail_no_head>::value ));

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
bool
tests_is_doubly_linked_container()
{
    bool ok = true;

    // positive
    D_NC_CHECK(is_doubly_linked_container<nct_list>::value       == true);
    D_NC_CHECK(is_doubly_linked_container<nct_all_shapes>::value == true);

    // head without tail
    D_NC_CHECK(is_doubly_linked_container<nct_slist>::value == false);
    D_NC_CHECK(is_doubly_linked_container<nct_tree_and_list>::value
                   == false);
    D_NC_CHECK(is_doubly_linked_container<nct_list_and_graph>::value
                   == false);

    // tail without head
    D_NC_CHECK(is_doubly_linked_container<nct_tail_no_head>::value == false);

    // ... and the decisive case: a genuine node container, with a tail, that
    // has no head.  It satisfies identity and has_tail, so a trait layered on
    // is_node_container would call it doubly linked; the real one is layered
    // on is_list_shaped_container, which fails first
    D_NC_CHECK(   ( is_node_container<nct_rooted_tail_no_head>::value
                        == true )
               && ( has_tail_method<nct_rooted_tail_no_head>::value
                        == true )
               && ( is_list_shaped_container<nct_rooted_tail_no_head>::value
                        == false )
               && ( is_doubly_linked_container<nct_rooted_tail_no_head>::value
                        == false ));

    // other shapes
    D_NC_CHECK(is_doubly_linked_container<nct_tree>::value  == false);
    D_NC_CHECK(is_doubly_linked_container<nct_graph>::value == false);
    D_NC_CHECK(is_doubly_linked_container<nct_bare>::value  == false);
    D_NC_CHECK(is_doubly_linked_container<int>::value       == false);

    // it implies list-shaped, never the reverse
    D_NC_CHECK(   ( is_doubly_linked_container<nct_list>::value == true )
               && ( is_list_shaped_container<nct_list>::value   == true ));
    D_NC_CHECK(   ( is_list_shaped_container<nct_slist>::value    == true )
               && ( is_doubly_linked_container<nct_slist>::value  == false ));

    // agreement with the definition
    D_NC_CHECK(is_doubly_linked_container<nct_list>::value
                   == (    is_list_shaped_container<nct_list>::value
                        && has_tail_method<nct_list>::value ));
    D_NC_CHECK(is_doubly_linked_container<nct_tail_no_head>::value
                   == (    is_list_shaped_container<nct_tail_no_head>::value
                        && has_tail_method<nct_tail_no_head>::value ));

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
bool
tests_is_graph_shaped_container()
{
    bool ok = true;

    // positive
    D_NC_CHECK(is_graph_shaped_container<nct_graph>::value            == true);
    D_NC_CHECK(is_graph_shaped_container<nct_shared_container>::value == true);
    D_NC_CHECK(is_graph_shaped_container<nct_list_and_graph>::value   == true);
    D_NC_CHECK(is_graph_shaped_container<nct_tree_and_graph>::value   == true);
    D_NC_CHECK(is_graph_shaped_container<nct_all_shapes>::value       == true);

    // other shapes
    D_NC_CHECK(is_graph_shaped_container<nct_tree>::value    == false);
    D_NC_CHECK(is_graph_shaped_container<nct_list>::value    == false);
    D_NC_CHECK(is_graph_shaped_container<nct_generic>::value == false);

    // the cardinality is not the set
    D_NC_CHECK(is_graph_shaped_container<nct_entry_count_no_entries>::value
                   == false);

    // the base requirement is real
    D_NC_CHECK(   ( has_entries_method<nct_entries_only_no_node>::value
                        == true )
               && ( is_graph_shaped_container<
                        nct_entries_only_no_node>::value == false ));

    // non-containers
    D_NC_CHECK(is_graph_shaped_container<nct_bare>::value == false);
    D_NC_CHECK(is_graph_shaped_container<int>::value      == false);
    D_NC_CHECK(is_graph_shaped_container<void>::value     == false);

    // agreement with the definition
    D_NC_CHECK(is_graph_shaped_container<nct_graph>::value
                   == (    is_node_container<nct_graph>::value
                        && has_entries_method<nct_graph>::value ));
    D_NC_CHECK(is_graph_shaped_container<nct_entries_only_no_node>::value
                   == (    is_node_container<
                               nct_entries_only_no_node>::value
                        && has_entries_method<
                               nct_entries_only_no_node>::value ));

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
bool
tests_identity_composition_shape()
{
    bool ok = true;

    // tree = base AND root
    D_NC_CHECK(is_tree_shaped_container<nct_all_shapes>::value
                   == (    is_node_container<nct_all_shapes>::value
                        && has_root_method<nct_all_shapes>::value ));
    D_NC_CHECK(is_tree_shaped_container<nct_graph>::value
                   == (    is_node_container<nct_graph>::value
                        && has_root_method<nct_graph>::value ));

    // list = base AND head
    D_NC_CHECK(is_list_shaped_container<nct_all_shapes>::value
                   == (    is_node_container<nct_all_shapes>::value
                        && has_head_method<nct_all_shapes>::value ));
    D_NC_CHECK(is_list_shaped_container<nct_tree>::value
                   == (    is_node_container<nct_tree>::value
                        && has_head_method<nct_tree>::value ));

    // doubly linked = list AND tail
    D_NC_CHECK(is_doubly_linked_container<nct_all_shapes>::value
                   == (    is_list_shaped_container<nct_all_shapes>::value
                        && has_tail_method<nct_all_shapes>::value ));
    D_NC_CHECK(is_doubly_linked_container<nct_slist>::value
                   == (    is_list_shaped_container<nct_slist>::value
                        && has_tail_method<nct_slist>::value ));
    D_NC_CHECK(is_doubly_linked_container<nct_rooted_tail_no_head>::value
                   == (    is_list_shaped_container<
                               nct_rooted_tail_no_head>::value
                        && has_tail_method<
                               nct_rooted_tail_no_head>::value ));

    // graph = base AND entries
    D_NC_CHECK(is_graph_shaped_container<nct_all_shapes>::value
                   == (    is_node_container<nct_all_shapes>::value
                        && has_entries_method<nct_all_shapes>::value ));
    D_NC_CHECK(is_graph_shaped_container<nct_list>::value
                   == (    is_node_container<nct_list>::value
                        && has_entries_method<nct_list>::value ));

    // every shape implies the base
    D_NC_CHECK(!is_tree_shaped_container<nct_tree>::value
                   || is_node_container<nct_tree>::value);
    D_NC_CHECK(!is_list_shaped_container<nct_list>::value
                   || is_node_container<nct_list>::value);
    D_NC_CHECK(!is_graph_shaped_container<nct_graph>::value
                   || is_node_container<nct_graph>::value);
    D_NC_CHECK(!is_doubly_linked_container<nct_list>::value
                   || is_node_container<nct_list>::value);

    // and a type that fails the base has no shape at all, however many
    // markers it carries
    D_NC_CHECK(   ( has_root_method<nct_root_only>::value    == true )
               && ( nc_shape_predicate_count<nct_root_only>() == 0 ));
    D_NC_CHECK(   ( has_head_method<nct_head_only_no_node>::value == true )
               && ( nc_shape_predicate_count<nct_head_only_no_node>()
                        == 0 ));
    D_NC_CHECK(   ( has_entries_method<nct_entries_only_no_node>::value
                        == true )
               && ( nc_shape_predicate_count<nct_entries_only_no_node>()
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
bool
tests_identity_overlap()
{
    bool ok = true;

    // the unambiguous shapes
    D_NC_CHECK(nc_shape_predicate_count<nct_tree>()    == 1);
    D_NC_CHECK(nc_shape_predicate_count<nct_slist>()   == 1);
    D_NC_CHECK(nc_shape_predicate_count<nct_graph>()   == 1);
    D_NC_CHECK(nc_shape_predicate_count<nct_generic>() == 0);

    // a doubly linked list is list-shaped AND doubly linked
    D_NC_CHECK(nc_shape_predicate_count<nct_list>() == 2);
    D_NC_CHECK(   ( is_list_shaped_container<nct_list>::value   == true )
               && ( is_doubly_linked_container<nct_list>::value == true ));

    // the overlapping fixtures
    D_NC_CHECK(nc_shape_predicate_count<nct_tree_and_list>() == 2);
    D_NC_CHECK(   ( is_tree_shaped_container<nct_tree_and_list>::value
                        == true )
               && ( is_list_shaped_container<nct_tree_and_list>::value
                        == true ));

    D_NC_CHECK(nc_shape_predicate_count<nct_list_and_graph>() == 2);
    D_NC_CHECK(   ( is_list_shaped_container<nct_list_and_graph>::value
                        == true )
               && ( is_graph_shaped_container<nct_list_and_graph>::value
                        == true ));

    D_NC_CHECK(nc_shape_predicate_count<nct_tree_and_graph>() == 2);
    D_NC_CHECK(   ( is_tree_shaped_container<nct_tree_and_graph>::value
                        == true )
               && ( is_graph_shaped_container<nct_tree_and_graph>::value
                        == true ));

    // the maximal fixture
    D_NC_CHECK(nc_shape_predicate_count<nct_all_shapes>() == 4);

    // and nothing at all for a non-container
    D_NC_CHECK(nc_shape_predicate_count<nct_bare>() == 0);
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
bool
tests_node_container_cv_ref_divergence()
{
    bool ok = true;

    // --- consequence 1: volatile collapses identity ---
    D_NC_CHECK(is_node_container<nct_tree>::value                  == true);
    D_NC_CHECK(is_node_container<volatile nct_tree>::value         == false);
    D_NC_CHECK(is_node_container<const volatile nct_tree>::value   == false);
    D_NC_CHECK(is_tree_shaped_container<volatile nct_tree>::value  == false);
    D_NC_CHECK(is_list_shaped_container<volatile nct_list>::value  == false);
    D_NC_CHECK(is_graph_shaped_container<volatile nct_graph>::value == false);

    // ... while the clean_t sections are unmoved by the same qualification
    D_NC_CHECK(has_node_type<volatile nct_tree>::value          == true);
    D_NC_CHECK(has_ownership_policy<volatile nct_tree>::value    == true);
    D_NC_CHECK(has_entry_owns_constant<volatile nct_tree>::value == true);
    D_NC_CHECK(ownership_model_of<volatile nct_tree>::value
                   == DOwnershipModel::unique);
    D_NC_STATIC_CHECK(std::is_same<node_type_of_t<volatile nct_tree>,
                                   nct_node>::value);

    // --- consequence 2: a reference is MORE permissive than the bare type ---
    D_NC_CHECK(has_root_method<nct_nonconst_root>::value       == false);
    D_NC_CHECK(has_root_method<nct_nonconst_root&>::value      == true);
    D_NC_CHECK(has_root_method<nct_nonconst_root&&>::value     == true);
    D_NC_CHECK(has_root_method<const nct_nonconst_root&>::value == false);

    // ... and it carries all the way up to identity and shape
    D_NC_CHECK(is_node_container<nct_nonconst_root>::value       == false);
    D_NC_CHECK(is_node_container<nct_nonconst_root&>::value      == true);
    D_NC_CHECK(is_tree_shaped_container<nct_nonconst_root>::value  == false);
    D_NC_CHECK(is_tree_shaped_container<nct_nonconst_root&>::value == true);

    // ... while the clean_t sections answer the same for both spellings
    D_NC_CHECK(has_node_type<nct_nonconst_root>::value
                   == has_node_type<nct_nonconst_root&>::value);

    // --- the well-behaved case, for contrast ---
    D_NC_CONST_PROBE_AGREE(is_node_container,        nct_tree,  true);
    D_NC_CONST_PROBE_AGREE(is_tree_shaped_container, nct_tree,  true);
    D_NC_CONST_PROBE_AGREE(is_list_shaped_container, nct_list,  true);
    D_NC_CONST_PROBE_AGREE(is_doubly_linked_container, nct_list, true);
    D_NC_CONST_PROBE_AGREE(is_graph_shaped_container, nct_graph, true);
    D_NC_CONST_PROBE_AGREE(is_node_container,        nct_bare,  false);

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
bool
tests_identity_companions()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_NC_STATIC_CHECK(is_node_container_v<nct_tree>         == true);
    D_NC_STATIC_CHECK(is_graph_shaped_container_v<nct_tree> == false);

    D_NC_CHECK(is_node_container_v<nct_tree>
                   == is_node_container<nct_tree>::value);
    D_NC_CHECK(is_node_container_v<nct_bare>
                   == is_node_container<nct_bare>::value);

    D_NC_CHECK(is_tree_shaped_container_v<nct_tree>
                   == is_tree_shaped_container<nct_tree>::value);
    D_NC_CHECK(is_tree_shaped_container_v<nct_list>
                   == is_tree_shaped_container<nct_list>::value);

    D_NC_CHECK(is_list_shaped_container_v<nct_slist>
                   == is_list_shaped_container<nct_slist>::value);
    D_NC_CHECK(is_list_shaped_container_v<nct_tree>
                   == is_list_shaped_container<nct_tree>::value);

    D_NC_CHECK(is_doubly_linked_container_v<nct_list>
                   == is_doubly_linked_container<nct_list>::value);
    D_NC_CHECK(is_doubly_linked_container_v<nct_slist>
                   == is_doubly_linked_container<nct_slist>::value);

    D_NC_CHECK(is_graph_shaped_container_v<nct_graph>
                   == is_graph_shaped_container<nct_graph>::value);
    D_NC_CHECK(is_graph_shaped_container_v<nct_list>
                   == is_graph_shaped_container<nct_list>::value);

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
