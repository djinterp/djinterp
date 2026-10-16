/*******************************************************************************
* djinterp [core]                 node_container_traits_tests_classification.cpp
*
*
* path:      /tests/djinterp/core/container/traits/node_container_traits_tests_classification.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "node_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// VI.  Combined classification
// ===========================================================================
//   Section V leaves the shape predicates overlapping; this section resolves
// the overlap into a single answer by a fixed order, and republishes the whole
// header as one flat snapshot.  The snapshot adds no logic, so the test that
// matters most is the agreement test: every member must equal the trait it
// mirrors.


/*
tests_node_container_shape_enum
  Verifies the shape enumeration itself.
  Tests the following:
  - it is a SCOPED enumeration with std::uint8_t as its declared underlying
    type, both of which the header pins deliberately
  - the five enumerators carry the values the header assigns, and are pairwise
    distinct
  - unknown is zero, so a value-initialised shape denotes "not classified"
  - a shape is usable as a non-type template argument, which is what lets
    node_container_shape_of derive from integral_constant
*/
bool
tests_node_container_shape_enum()
{
    bool ok = true;

    // scoped, and pinned to an explicit underlying type
    D_NC_STATIC_CHECK(std::is_enum<DNodeContainerShape>::value == true);
    D_NC_STATIC_CHECK(
        std::is_convertible<DNodeContainerShape, int>::value == false);
    D_NC_STATIC_CHECK(
        std::is_same<std::underlying_type<DNodeContainerShape>::type,
                     std::uint8_t>::value);

    // the explicitly assigned values
    D_NC_CHECK(static_cast<int>(DNodeContainerShape::unknown) == 0);
    D_NC_CHECK(static_cast<int>(DNodeContainerShape::tree)    == 1);
    D_NC_CHECK(static_cast<int>(DNodeContainerShape::list)    == 2);
    D_NC_CHECK(static_cast<int>(DNodeContainerShape::graph)   == 3);
    D_NC_CHECK(static_cast<int>(DNodeContainerShape::generic) == 4);

    // a value-initialised shape is unknown
    D_NC_CHECK(DNodeContainerShape{} == DNodeContainerShape::unknown);

    // pairwise distinct
    D_NC_CHECK(DNodeContainerShape::unknown != DNodeContainerShape::tree);
    D_NC_CHECK(DNodeContainerShape::unknown != DNodeContainerShape::list);
    D_NC_CHECK(DNodeContainerShape::unknown != DNodeContainerShape::graph);
    D_NC_CHECK(DNodeContainerShape::unknown != DNodeContainerShape::generic);
    D_NC_CHECK(DNodeContainerShape::tree    != DNodeContainerShape::list);
    D_NC_CHECK(DNodeContainerShape::tree    != DNodeContainerShape::graph);
    D_NC_CHECK(DNodeContainerShape::tree    != DNodeContainerShape::generic);
    D_NC_CHECK(DNodeContainerShape::list    != DNodeContainerShape::graph);
    D_NC_CHECK(DNodeContainerShape::list    != DNodeContainerShape::generic);
    D_NC_CHECK(DNodeContainerShape::graph   != DNodeContainerShape::generic);

    // usable as a non-type template argument
    D_NC_STATIC_CHECK(
        std::integral_constant<DNodeContainerShape,
                               DNodeContainerShape::graph>::value
            == DNodeContainerShape::graph);

    return ok;
}

/*
tests_node_container_shape_of
  Verifies the shape each unambiguous fixture resolves to.
  Tests the following:
  - the four named shapes, each on a fixture that lights exactly one predicate
  - generic: a node container with the neutral entry_point() and no domain
    alias, which is the shape the production node_container base presents
  - unknown: everything that fails the identity requirement, including types
    carrying an entry marker but no node_type, and types carrying node_type but
    no entry marker
  - the internal implementation agrees with the published trait
*/
bool
tests_node_container_shape_of()
{
    bool ok = true;

    // the four named shapes
    D_NC_CHECK(node_container_shape_of<nct_tree>::value
                   == DNodeContainerShape::tree);
    D_NC_CHECK(node_container_shape_of<nct_list>::value
                   == DNodeContainerShape::list);
    D_NC_CHECK(node_container_shape_of<nct_slist>::value
                   == DNodeContainerShape::list);
    D_NC_CHECK(node_container_shape_of<nct_graph>::value
                   == DNodeContainerShape::graph);

    // the neutral base
    D_NC_CHECK(node_container_shape_of<nct_generic>::value
                   == DNodeContainerShape::generic);
    D_NC_CHECK(node_container_shape_of<nct_const_release>::value
                   == DNodeContainerShape::generic);
    D_NC_CHECK(node_container_shape_of<nct_unknown_policy_container>::value
                   == DNodeContainerShape::generic);

    // the ownership fixtures keep their shapes
    D_NC_CHECK(node_container_shape_of<nct_unique_container>::value
                   == DNodeContainerShape::tree);
    D_NC_CHECK(node_container_shape_of<nct_shared_container>::value
                   == DNodeContainerShape::graph);
    D_NC_CHECK(node_container_shape_of<nct_non_owning_container>::value
                   == DNodeContainerShape::list);

    // unclassified: identity fails on one side or the other
    D_NC_CHECK(node_container_shape_of<nct_node_only>::value
                   == DNodeContainerShape::unknown);
    D_NC_CHECK(node_container_shape_of<nct_root_only>::value
                   == DNodeContainerShape::unknown);
    D_NC_CHECK(node_container_shape_of<nct_head_only_no_node>::value
                   == DNodeContainerShape::unknown);
    D_NC_CHECK(node_container_shape_of<nct_entries_only_no_node>::value
                   == DNodeContainerShape::unknown);
    D_NC_CHECK(node_container_shape_of<nct_tail_no_head>::value
                   == DNodeContainerShape::unknown);
    D_NC_CHECK(node_container_shape_of<nct_bare>::value
                   == DNodeContainerShape::unknown);
    D_NC_CHECK(node_container_shape_of<int>::value
                   == DNodeContainerShape::unknown);
    D_NC_CHECK(node_container_shape_of<void>::value
                   == DNodeContainerShape::unknown);
    D_NC_CHECK(node_container_shape_of<nonesuch>::value
                   == DNodeContainerShape::unknown);

    // the internal implementation agrees with the published trait
    D_NC_CHECK(internal::shape_impl<nct_tree>::value
                   == node_container_shape_of<nct_tree>::value);
    D_NC_CHECK(internal::shape_impl<nct_generic>::value
                   == node_container_shape_of<nct_generic>::value);
    D_NC_CHECK(internal::shape_impl<int>::value
                   == node_container_shape_of<int>::value);

    return ok;
}

/*
tests_node_container_shape_precedence
  Verifies the ORDER in which the overlapping predicates are consulted.  A
  container may light several at once - nothing forbids root() beside head() -
  and the classifier resolves that by testing tree, then list, then graph, then
  the bare identity.  These fixtures occupy the overlapping cells so the order
  is pinned rather than assumed.
  Tests the following:
  - tree beats list, tree beats graph, list beats graph
  - the maximal fixture, which lights all four predicates, still resolves to
    tree
  - generic is reached only when NO shape predicate holds, so it cannot mask a
    named shape
  - unknown is reached only when identity itself fails
  - in every case the resolved shape is one the type actually satisfies, so the
    ordering never invents a classification
*/
bool
tests_node_container_shape_precedence()
{
    bool ok = true;

    // tree beats list
    D_NC_CHECK(   ( is_tree_shaped_container<nct_tree_and_list>::value
                        == true )
               && ( is_list_shaped_container<nct_tree_and_list>::value
                        == true )
               && ( node_container_shape_of<nct_tree_and_list>::value
                        == DNodeContainerShape::tree ));

    // tree beats graph
    D_NC_CHECK(   ( is_tree_shaped_container<nct_tree_and_graph>::value
                        == true )
               && ( is_graph_shaped_container<nct_tree_and_graph>::value
                        == true )
               && ( node_container_shape_of<nct_tree_and_graph>::value
                        == DNodeContainerShape::tree ));

    // list beats graph
    D_NC_CHECK(   ( is_list_shaped_container<nct_list_and_graph>::value
                        == true )
               && ( is_graph_shaped_container<nct_list_and_graph>::value
                        == true )
               && ( node_container_shape_of<nct_list_and_graph>::value
                        == DNodeContainerShape::list ));

    // all four at once still resolves to tree
    D_NC_CHECK(   ( nc_shape_predicate_count<nct_all_shapes>() == 4 )
               && ( node_container_shape_of<nct_all_shapes>::value
                        == DNodeContainerShape::tree ));

    // generic is reached only when no shape predicate holds
    D_NC_CHECK(   ( nc_shape_predicate_count<nct_generic>() == 0 )
               && ( is_node_container<nct_generic>::value   == true )
               && ( node_container_shape_of<nct_generic>::value
                        == DNodeContainerShape::generic ));

    // unknown only when identity fails
    D_NC_CHECK(   ( is_node_container<nct_root_only>::value == false )
               && ( node_container_shape_of<nct_root_only>::value
                        == DNodeContainerShape::unknown ));

    // the resolved shape is always one the type genuinely satisfies
    D_NC_CHECK(!(  node_container_shape_of<nct_all_shapes>::value
                       == DNodeContainerShape::tree )
               || is_tree_shaped_container<nct_all_shapes>::value);
    D_NC_CHECK(!(  node_container_shape_of<nct_list_and_graph>::value
                       == DNodeContainerShape::list )
               || is_list_shaped_container<nct_list_and_graph>::value);
    D_NC_CHECK(!(  node_container_shape_of<nct_graph>::value
                       == DNodeContainerShape::graph )
               || is_graph_shaped_container<nct_graph>::value);

    return ok;
}

/*
tests_node_container_shape_companions
  Verifies the `_v` companion of node_container_shape_of.
  Tests the following:
  - the companion carries the enum, not a bool
  - it agrees with the trait across every shape
  - it is usable in a constant expression
  - gated on variable-template support
*/
bool
tests_node_container_shape_companions()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_NC_STATIC_CHECK(node_container_shape_of_v<nct_tree>
                          == DNodeContainerShape::tree);
    D_NC_STATIC_CHECK(node_container_shape_of_v<int>
                          == DNodeContainerShape::unknown);

    D_NC_CHECK(node_container_shape_of_v<nct_tree>
                   == node_container_shape_of<nct_tree>::value);
    D_NC_CHECK(node_container_shape_of_v<nct_list>
                   == node_container_shape_of<nct_list>::value);
    D_NC_CHECK(node_container_shape_of_v<nct_graph>
                   == node_container_shape_of<nct_graph>::value);
    D_NC_CHECK(node_container_shape_of_v<nct_generic>
                   == node_container_shape_of<nct_generic>::value);
    D_NC_CHECK(node_container_shape_of_v<nct_bare>
                   == node_container_shape_of<nct_bare>::value);

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}

/*
tests_node_container_class_identity
  Verifies the identity and shape members of the snapshot.
  Tests the following:
  - is_node_container and shape are reported for each silhouette
  - the four shape flags are reported alongside the resolved shape, so a caller
    can see both the winner and the overlap
  - the flags and the shape stay consistent on an overlapping fixture: several
    flags hold while one shape is named
*/
bool
tests_node_container_class_identity()
{
    bool ok = true;

    // tree
    D_NC_CHECK(node_container_class<nct_tree>::is_node_container == true);
    D_NC_CHECK(node_container_class<nct_tree>::shape
                   == DNodeContainerShape::tree);
    D_NC_CHECK(node_container_class<nct_tree>::is_tree_shaped   == true);
    D_NC_CHECK(node_container_class<nct_tree>::is_list_shaped   == false);
    D_NC_CHECK(node_container_class<nct_tree>::is_doubly_linked == false);
    D_NC_CHECK(node_container_class<nct_tree>::is_graph_shaped  == false);

    // doubly linked list
    D_NC_CHECK(node_container_class<nct_list>::shape
                   == DNodeContainerShape::list);
    D_NC_CHECK(node_container_class<nct_list>::is_list_shaped   == true);
    D_NC_CHECK(node_container_class<nct_list>::is_doubly_linked == true);
    D_NC_CHECK(node_container_class<nct_list>::is_tree_shaped   == false);

    // singly linked list
    D_NC_CHECK(node_container_class<nct_slist>::is_list_shaped   == true);
    D_NC_CHECK(node_container_class<nct_slist>::is_doubly_linked == false);

    // graph
    D_NC_CHECK(node_container_class<nct_graph>::shape
                   == DNodeContainerShape::graph);
    D_NC_CHECK(node_container_class<nct_graph>::is_graph_shaped == true);

    // the neutral base
    D_NC_CHECK(node_container_class<nct_generic>::is_node_container == true);
    D_NC_CHECK(node_container_class<nct_generic>::shape
                   == DNodeContainerShape::generic);
    D_NC_CHECK(node_container_class<nct_generic>::is_tree_shaped  == false);
    D_NC_CHECK(node_container_class<nct_generic>::is_list_shaped  == false);
    D_NC_CHECK(node_container_class<nct_generic>::is_graph_shaped == false);

    // not a container
    D_NC_CHECK(node_container_class<nct_bare>::is_node_container == false);
    D_NC_CHECK(node_container_class<nct_bare>::shape
                   == DNodeContainerShape::unknown);

    // an overlapping fixture: several flags, one shape
    D_NC_CHECK(   ( node_container_class<nct_all_shapes>::is_tree_shaped
                        == true )
               && ( node_container_class<nct_all_shapes>::is_list_shaped
                        == true )
               && ( node_container_class<nct_all_shapes>::is_doubly_linked
                        == true )
               && ( node_container_class<nct_all_shapes>::is_graph_shaped
                        == true )
               && ( node_container_class<nct_all_shapes>::shape
                        == DNodeContainerShape::tree ));

    return ok;
}

/*
tests_node_container_class_ownership
  Verifies the ownership members of the snapshot.
  Tests the following:
  - has_ownership, ownership, is_owning and the two model flags are reported
    for each ownership fixture
  - the snapshot carries the completeness consequence through unchanged: a
    container naming a forward-declared policy reports its model but is_owning
    false
  - a container owning under an unrecognised policy reports the mirror case
*/
bool
tests_node_container_class_ownership()
{
    bool ok = true;

    // a recognised but incomplete policy
    D_NC_CHECK(node_container_class<nct_unique_container>::has_ownership
                   == true);
    D_NC_CHECK(node_container_class<nct_unique_container>::ownership
                   == DOwnershipModel::unique);
    D_NC_CHECK(node_container_class<nct_unique_container>::is_unique_owning
                   == true);
    D_NC_CHECK(node_container_class<nct_unique_container>::is_shared_owning
                   == false);
    D_NC_CHECK(node_container_class<nct_unique_container>::is_owning
                   == false);

    D_NC_CHECK(node_container_class<nct_shared_container>::ownership
                   == DOwnershipModel::shared);
    D_NC_CHECK(node_container_class<nct_shared_container>::is_shared_owning
                   == true);

    D_NC_CHECK(node_container_class<nct_non_owning_container>::ownership
                   == DOwnershipModel::non_owning);
    D_NC_CHECK(node_container_class<nct_non_owning_container>::is_owning
                   == false);

    // an unrecognised but complete owning policy - the mirror case
    D_NC_CHECK(node_container_class<
                   nct_unknown_policy_container>::has_ownership == true);
    D_NC_CHECK(node_container_class<nct_unknown_policy_container>::ownership
                   == DOwnershipModel::unknown);
    D_NC_CHECK(node_container_class<nct_unknown_policy_container>::is_owning
                   == true);
    D_NC_CHECK(node_container_class<
                   nct_unknown_policy_container>::is_unique_owning == false);
    D_NC_CHECK(node_container_class<
                   nct_unknown_policy_container>::is_shared_owning == false);

    // no policy at all
    D_NC_CHECK(node_container_class<nct_list>::has_ownership == false);
    D_NC_CHECK(node_container_class<nct_list>::ownership
                   == DOwnershipModel::non_owning);
    D_NC_CHECK(node_container_class<nct_list>::is_owning == false);

    return ok;
}

/*
tests_node_container_class_entry
  Verifies the entry point members of the snapshot.

  Note the naming trap this section exists to pin: the member `has_entry`
  reports has_any_entry_point - whether the container holds ANY root - while
  the trait has_has_entry_method reports whether it declares a has_entry()
  METHOD.  The two are unrelated, and a container may have either without the
  other.

  Tests the following:
  - the seven entry members are reported for each silhouette
  - has_entry follows the disjunction and NOT the method probe, witnessed on a
    fixture where the two disagree in each direction
  - has_root, has_head, has_tail, has_entries, has_entry_count and has_release
    each follow their own probe
*/
bool
tests_node_container_class_entry()
{
    bool ok = true;

    // tree
    D_NC_CHECK(node_container_class<nct_tree>::has_entry       == true);
    D_NC_CHECK(node_container_class<nct_tree>::has_root        == true);
    D_NC_CHECK(node_container_class<nct_tree>::has_head        == false);
    D_NC_CHECK(node_container_class<nct_tree>::has_tail        == false);
    D_NC_CHECK(node_container_class<nct_tree>::has_entries     == false);
    D_NC_CHECK(node_container_class<nct_tree>::has_entry_count == false);
    D_NC_CHECK(node_container_class<nct_tree>::has_release     == true);

    // doubly linked list
    D_NC_CHECK(node_container_class<nct_list>::has_head    == true);
    D_NC_CHECK(node_container_class<nct_list>::has_tail    == true);
    D_NC_CHECK(node_container_class<nct_list>::has_root    == false);
    D_NC_CHECK(node_container_class<nct_list>::has_release == false);

    // graph
    D_NC_CHECK(node_container_class<nct_graph>::has_entries     == true);
    D_NC_CHECK(node_container_class<nct_graph>::has_entry_count == true);
    D_NC_CHECK(node_container_class<nct_graph>::has_head        == false);

    // not a container
    D_NC_CHECK(node_container_class<nct_bare>::has_entry   == false);
    D_NC_CHECK(node_container_class<nct_bare>::has_root    == false);
    D_NC_CHECK(node_container_class<nct_bare>::has_release == false);

    // the naming trap: has_entry is the DISJUNCTION, not the method probe
    D_NC_CHECK(node_container_class<nct_tree>::has_entry
                   == has_any_entry_point<nct_tree>::value);

    // ... a container with the method but no root reports has_entry false
    D_NC_CHECK(   ( has_has_entry_method<
                        nct_has_entry_no_entry_point>::value == true )
               && ( node_container_class<
                        nct_has_entry_no_entry_point>::has_entry == false ));

    // ... and one with a root but no method reports has_entry true
    D_NC_CHECK(   ( has_has_entry_method<nct_list>::value == false )
               && ( node_container_class<nct_list>::has_entry == true ));

    return ok;
}

/*
tests_node_container_class_node
  Verifies the node type members of the snapshot.
  Tests the following:
  - has_node reports the alias probe, and node names the extracted type
  - the extracted type keeps its own qualification
  - for a type with no alias, node is nonesuch and has_node is false, so the
    pair together is unambiguous even where node alone is not
  - a container declaring node_type = nonesuch reports has_node true with the
    same node type, which is the one case the pair cannot separate by the type
    alone
*/
bool
tests_node_container_class_node()
{
    bool ok = true;

    // present
    D_NC_CHECK(node_container_class<nct_tree>::has_node == true);
    D_NC_STATIC_CHECK(std::is_same<node_container_class<nct_tree>::node,
                                   nct_node>::value);
    D_NC_STATIC_CHECK(std::is_same<node_container_class<nct_graph>::node,
                                   nct_node>::value);

    // qualification survives extraction
    D_NC_STATIC_CHECK(
        std::is_same<node_container_class<nct_const_node_type>::node,
                     const nct_node>::value);

    // an alias naming void
    D_NC_STATIC_CHECK(
        std::is_same<node_container_class<nct_void_node_type>::node,
                     void>::value);

    // absent: the pair is unambiguous
    D_NC_CHECK(node_container_class<nct_bare>::has_node == false);
    D_NC_STATIC_CHECK(std::is_same<node_container_class<nct_bare>::node,
                                   nonesuch>::value);
    D_NC_CHECK(node_container_class<int>::has_node == false);
    D_NC_STATIC_CHECK(std::is_same<node_container_class<int>::node,
                                   nonesuch>::value);

    // the one case the type alone cannot separate
    D_NC_CHECK(node_container_class<nct_nonesuch_node_type>::has_node == true);
    D_NC_STATIC_CHECK(
        std::is_same<node_container_class<nct_nonesuch_node_type>::node,
                     node_container_class<nct_bare>::node>::value);

    // agreement with the standalone extractor
    D_NC_STATIC_CHECK(std::is_same<node_container_class<nct_tree>::node,
                                   node_type_of_t<nct_tree>>::value);
    D_NC_STATIC_CHECK(std::is_same<node_container_class<int>::node,
                                   node_type_of_t<int>>::value);

    return ok;
}

/*
tests_node_container_class_agreement
  Verifies that the snapshot is a faithful mirror: every member equals the
  standalone trait it republishes, checked across carriers that occupy
  different cells of the classification.
  Tests the following:
  - the identity and five shape members against their traits
  - the five ownership members against theirs
  - the seven entry members against theirs
  - the node member against the extractor
  - the check is repeated on a second, differently shaped carrier so a member
    wired to the wrong trait cannot pass by coincidence
*/
bool
tests_node_container_class_agreement()
{
    bool ok = true;

    // --- identity and shape, on a tree ---
    D_NC_CHECK(node_container_class<nct_tree>::is_node_container
                   == is_node_container<nct_tree>::value);
    D_NC_CHECK(node_container_class<nct_tree>::shape
                   == node_container_shape_of<nct_tree>::value);
    D_NC_CHECK(node_container_class<nct_tree>::is_tree_shaped
                   == is_tree_shaped_container<nct_tree>::value);
    D_NC_CHECK(node_container_class<nct_tree>::is_list_shaped
                   == is_list_shaped_container<nct_tree>::value);
    D_NC_CHECK(node_container_class<nct_tree>::is_doubly_linked
                   == is_doubly_linked_container<nct_tree>::value);
    D_NC_CHECK(node_container_class<nct_tree>::is_graph_shaped
                   == is_graph_shaped_container<nct_tree>::value);

    // --- identity and shape, on a doubly linked list ---
    D_NC_CHECK(node_container_class<nct_list>::shape
                   == node_container_shape_of<nct_list>::value);
    D_NC_CHECK(node_container_class<nct_list>::is_tree_shaped
                   == is_tree_shaped_container<nct_list>::value);
    D_NC_CHECK(node_container_class<nct_list>::is_list_shaped
                   == is_list_shaped_container<nct_list>::value);
    D_NC_CHECK(node_container_class<nct_list>::is_doubly_linked
                   == is_doubly_linked_container<nct_list>::value);
    D_NC_CHECK(node_container_class<nct_list>::is_graph_shaped
                   == is_graph_shaped_container<nct_list>::value);

    // --- ownership, on a recognised policy ---
    D_NC_CHECK(node_container_class<nct_unique_container>::has_ownership
                   == has_ownership_policy<nct_unique_container>::value);
    D_NC_CHECK(node_container_class<nct_unique_container>::ownership
                   == ownership_model_of<nct_unique_container>::value);
    D_NC_CHECK(node_container_class<nct_unique_container>::is_owning
                   == is_owning_container<nct_unique_container>::value);
    D_NC_CHECK(node_container_class<nct_unique_container>::is_unique_owning
                   == is_unique_owning_container<
                          nct_unique_container>::value);
    D_NC_CHECK(node_container_class<nct_unique_container>::is_shared_owning
                   == is_shared_owning_container<
                          nct_unique_container>::value);

    // --- ownership, on an unrecognised complete policy ---
    D_NC_CHECK(node_container_class<nct_unknown_policy_container>::ownership
                   == ownership_model_of<
                          nct_unknown_policy_container>::value);
    D_NC_CHECK(node_container_class<nct_unknown_policy_container>::is_owning
                   == is_owning_container<
                          nct_unknown_policy_container>::value);

    // --- entry, on a tree ---
    D_NC_CHECK(node_container_class<nct_tree>::has_entry
                   == has_any_entry_point<nct_tree>::value);
    D_NC_CHECK(node_container_class<nct_tree>::has_root
                   == has_root_method<nct_tree>::value);
    D_NC_CHECK(node_container_class<nct_tree>::has_head
                   == has_head_method<nct_tree>::value);
    D_NC_CHECK(node_container_class<nct_tree>::has_tail
                   == has_tail_method<nct_tree>::value);
    D_NC_CHECK(node_container_class<nct_tree>::has_entries
                   == has_entries_method<nct_tree>::value);
    D_NC_CHECK(node_container_class<nct_tree>::has_entry_count
                   == has_entry_count_method<nct_tree>::value);
    D_NC_CHECK(node_container_class<nct_tree>::has_release
                   == has_release_entry_method<nct_tree>::value);

    // --- entry, on a graph, where a different subset is lit ---
    D_NC_CHECK(node_container_class<nct_graph>::has_entry
                   == has_any_entry_point<nct_graph>::value);
    D_NC_CHECK(node_container_class<nct_graph>::has_root
                   == has_root_method<nct_graph>::value);
    D_NC_CHECK(node_container_class<nct_graph>::has_head
                   == has_head_method<nct_graph>::value);
    D_NC_CHECK(node_container_class<nct_graph>::has_tail
                   == has_tail_method<nct_graph>::value);
    D_NC_CHECK(node_container_class<nct_graph>::has_entries
                   == has_entries_method<nct_graph>::value);
    D_NC_CHECK(node_container_class<nct_graph>::has_entry_count
                   == has_entry_count_method<nct_graph>::value);
    D_NC_CHECK(node_container_class<nct_graph>::has_release
                   == has_release_entry_method<nct_graph>::value);

    // --- node ---
    D_NC_CHECK(node_container_class<nct_tree>::has_node
                   == has_node_type<nct_tree>::value);
    D_NC_CHECK(node_container_class<nct_bare>::has_node
                   == has_node_type<nct_bare>::value);

    return ok;
}

/*
tests_node_container_class_edge_types
  Verifies that the snapshot instantiates and answers sensibly for types that
  are not containers at all - the case an aggregate is most likely to break on,
  since it touches every trait in the header at once.
  Tests the following:
  - void instantiates: no probe forms a reference to it outside the immediate
    context, so every member reads as the empty answer
  - scalars, pointers, raw arrays, function types and nonesuch likewise
  - the near-miss fixtures manufacture nothing: a snapshot never reports a
    capability the individual traits declined
  - the ownership members report the default model rather than unknown for
    every one of these, which is the same quirk section IV records
*/
bool
tests_node_container_class_edge_types()
{
    bool ok = true;

    // void: the whole aggregate must instantiate
    D_NC_CHECK(node_container_class<void>::is_node_container == false);
    D_NC_CHECK(node_container_class<void>::shape
                   == DNodeContainerShape::unknown);
    D_NC_CHECK(node_container_class<void>::has_entry    == false);
    D_NC_CHECK(node_container_class<void>::has_node     == false);
    D_NC_CHECK(node_container_class<void>::has_ownership == false);
    D_NC_CHECK(node_container_class<void>::is_owning    == false);
    D_NC_CHECK(node_container_class<void>::ownership
                   == DOwnershipModel::non_owning);
    D_NC_STATIC_CHECK(std::is_same<node_container_class<void>::node,
                                   nonesuch>::value);

    // scalars, pointers, arrays, functions, nonesuch
    D_NC_CHECK(node_container_class<int>::shape
                   == DNodeContainerShape::unknown);
    D_NC_CHECK(node_container_class<int*>::shape
                   == DNodeContainerShape::unknown);
    D_NC_CHECK(node_container_class<int[4]>::shape
                   == DNodeContainerShape::unknown);
    D_NC_CHECK(node_container_class<void(int)>::shape
                   == DNodeContainerShape::unknown);
    D_NC_CHECK(node_container_class<nonesuch>::shape
                   == DNodeContainerShape::unknown);
    D_NC_CHECK(node_container_class<nct_node>::shape
                   == DNodeContainerShape::unknown);

    // the default ownership answer, for every one of them
    D_NC_CHECK(node_container_class<int>::ownership
                   == DOwnershipModel::non_owning);
    D_NC_CHECK(node_container_class<int*>::ownership
                   == DOwnershipModel::non_owning);
    D_NC_CHECK(node_container_class<nonesuch>::ownership
                   == DOwnershipModel::non_owning);

    // the near-misses manufacture nothing
    D_NC_CHECK(node_container_class<nct_private_root>::is_node_container
                   == false);
    D_NC_CHECK(node_container_class<nct_arg_root>::is_node_container
                   == false);
    D_NC_CHECK(node_container_class<nct_data_member_root>::is_node_container
                   == false);
    D_NC_CHECK(node_container_class<nct_nonconst_root>::is_node_container
                   == false);
    D_NC_CHECK(node_container_class<nct_tail_no_head>::is_node_container
                   == false);
    D_NC_CHECK(node_container_class<nct_entry_count_no_entries>::shape
                   == DNodeContainerShape::unknown);

    // ... while the admitted method shapes do classify
    D_NC_CHECK(node_container_class<nct_fnptr_root>::shape
                   == DNodeContainerShape::tree);
    D_NC_CHECK(node_container_class<nct_inherited_root>::shape
                   == DNodeContainerShape::tree);
    D_NC_CHECK(node_container_class<nct_void_root>::shape
                   == DNodeContainerShape::tree);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
