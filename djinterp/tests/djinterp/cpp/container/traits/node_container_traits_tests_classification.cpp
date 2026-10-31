/*******************************************************************************
* djinterp [cpp]                  node_container_traits_tests_classification.cpp
*
*
* path:      /tests/djinterp/cpp/container/traits/node_container_traits_tests_classification.cpp
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
bool tests_node_container_shape_enum()
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
    alias, which is the shape the production DNodeContainer base presents
  - unknown: everything that fails the identity requirement, including types
    carrying an entry marker but no node_type, and types carrying node_type but
    no entry marker
  - the internal implementation agrees with the published trait
*/
bool tests_node_container_shape_of()
{
    bool ok = true;

    // the four named shapes
    D_NC_CHECK(node_container_shape_of<DNctTree>::value
                   == DNodeContainerShape::tree);
    D_NC_CHECK(node_container_shape_of<DNctList>::value
                   == DNodeContainerShape::list);
    D_NC_CHECK(node_container_shape_of<DNctSlist>::value
                   == DNodeContainerShape::list);
    D_NC_CHECK(node_container_shape_of<DNctGraph>::value
                   == DNodeContainerShape::graph);

    // the neutral base
    D_NC_CHECK(node_container_shape_of<DNctGeneric>::value
                   == DNodeContainerShape::generic);
    D_NC_CHECK(node_container_shape_of<DNctConstRelease>::value
                   == DNodeContainerShape::generic);
    D_NC_CHECK(node_container_shape_of<DNctUnknownPolicyContainer>::value
                   == DNodeContainerShape::generic);

    // the ownership fixtures keep their shapes
    D_NC_CHECK(node_container_shape_of<DNctUniqueContainer>::value
                   == DNodeContainerShape::tree);
    D_NC_CHECK(node_container_shape_of<DNctSharedContainer>::value
                   == DNodeContainerShape::graph);
    D_NC_CHECK(node_container_shape_of<DNctNonOwningContainer>::value
                   == DNodeContainerShape::list);

    // unclassified: identity fails on one side or the other
    D_NC_CHECK(node_container_shape_of<DNctNodeOnly>::value
                   == DNodeContainerShape::unknown);
    D_NC_CHECK(node_container_shape_of<DNctRootOnly>::value
                   == DNodeContainerShape::unknown);
    D_NC_CHECK(node_container_shape_of<DNctHeadOnlyNoNode>::value
                   == DNodeContainerShape::unknown);
    D_NC_CHECK(node_container_shape_of<DNctEntriesOnlyNoNode>::value
                   == DNodeContainerShape::unknown);
    D_NC_CHECK(node_container_shape_of<DNctTailNoHead>::value
                   == DNodeContainerShape::unknown);
    D_NC_CHECK(node_container_shape_of<DNctBare>::value
                   == DNodeContainerShape::unknown);
    D_NC_CHECK(node_container_shape_of<int>::value
                   == DNodeContainerShape::unknown);
    D_NC_CHECK(node_container_shape_of<void>::value
                   == DNodeContainerShape::unknown);
    D_NC_CHECK(node_container_shape_of<nonesuch>::value
                   == DNodeContainerShape::unknown);

    // the internal implementation agrees with the published trait
    D_NC_CHECK(internal::shape_impl<DNctTree>::value
                   == node_container_shape_of<DNctTree>::value);
    D_NC_CHECK(internal::shape_impl<DNctGeneric>::value
                   == node_container_shape_of<DNctGeneric>::value);
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
bool tests_node_container_shape_precedence()
{
    bool ok = true;

    // tree beats list
    D_NC_CHECK(   ( is_tree_shaped_container<DNctTreeAndList>::value
                        == true )
               && ( is_list_shaped_container<DNctTreeAndList>::value
                        == true )
               && ( node_container_shape_of<DNctTreeAndList>::value
                        == DNodeContainerShape::tree ));

    // tree beats graph
    D_NC_CHECK(   ( is_tree_shaped_container<DNctTreeAndGraph>::value
                        == true )
               && ( is_graph_shaped_container<DNctTreeAndGraph>::value
                        == true )
               && ( node_container_shape_of<DNctTreeAndGraph>::value
                        == DNodeContainerShape::tree ));

    // list beats graph
    D_NC_CHECK(   ( is_list_shaped_container<DNctListAndGraph>::value
                        == true )
               && ( is_graph_shaped_container<DNctListAndGraph>::value
                        == true )
               && ( node_container_shape_of<DNctListAndGraph>::value
                        == DNodeContainerShape::list ));

    // all four at once still resolves to tree
    D_NC_CHECK(   ( nc_shape_predicate_count<DNctAllShapes>() == 4 )
               && ( node_container_shape_of<DNctAllShapes>::value
                        == DNodeContainerShape::tree ));

    // generic is reached only when no shape predicate holds
    D_NC_CHECK(   ( nc_shape_predicate_count<DNctGeneric>() == 0 )
               && ( is_node_container<DNctGeneric>::value   == true )
               && ( node_container_shape_of<DNctGeneric>::value
                        == DNodeContainerShape::generic ));

    // unknown only when identity fails
    D_NC_CHECK(   ( is_node_container<DNctRootOnly>::value == false )
               && ( node_container_shape_of<DNctRootOnly>::value
                        == DNodeContainerShape::unknown ));

    // the resolved shape is always one the type genuinely satisfies
    D_NC_CHECK(!(  node_container_shape_of<DNctAllShapes>::value
                       == DNodeContainerShape::tree )
               || is_tree_shaped_container<DNctAllShapes>::value);
    D_NC_CHECK(!(  node_container_shape_of<DNctListAndGraph>::value
                       == DNodeContainerShape::list )
               || is_list_shaped_container<DNctListAndGraph>::value);
    D_NC_CHECK(!(  node_container_shape_of<DNctGraph>::value
                       == DNodeContainerShape::graph )
               || is_graph_shaped_container<DNctGraph>::value);

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
bool tests_node_container_shape_companions()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_NC_STATIC_CHECK(node_container_shape_of_v<DNctTree>
                          == DNodeContainerShape::tree);
    D_NC_STATIC_CHECK(node_container_shape_of_v<int>
                          == DNodeContainerShape::unknown);

    D_NC_CHECK(node_container_shape_of_v<DNctTree>
                   == node_container_shape_of<DNctTree>::value);
    D_NC_CHECK(node_container_shape_of_v<DNctList>
                   == node_container_shape_of<DNctList>::value);
    D_NC_CHECK(node_container_shape_of_v<DNctGraph>
                   == node_container_shape_of<DNctGraph>::value);
    D_NC_CHECK(node_container_shape_of_v<DNctGeneric>
                   == node_container_shape_of<DNctGeneric>::value);
    D_NC_CHECK(node_container_shape_of_v<DNctBare>
                   == node_container_shape_of<DNctBare>::value);

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
bool tests_node_container_class_identity()
{
    bool ok = true;

    // tree
    D_NC_CHECK(DNodeContainerClass<DNctTree>::is_node_container == true);
    D_NC_CHECK(DNodeContainerClass<DNctTree>::shape
                   == DNodeContainerShape::tree);
    D_NC_CHECK(DNodeContainerClass<DNctTree>::is_tree_shaped   == true);
    D_NC_CHECK(DNodeContainerClass<DNctTree>::is_list_shaped   == false);
    D_NC_CHECK(DNodeContainerClass<DNctTree>::is_doubly_linked == false);
    D_NC_CHECK(DNodeContainerClass<DNctTree>::is_graph_shaped  == false);

    // doubly linked list
    D_NC_CHECK(DNodeContainerClass<DNctList>::shape
                   == DNodeContainerShape::list);
    D_NC_CHECK(DNodeContainerClass<DNctList>::is_list_shaped   == true);
    D_NC_CHECK(DNodeContainerClass<DNctList>::is_doubly_linked == true);
    D_NC_CHECK(DNodeContainerClass<DNctList>::is_tree_shaped   == false);

    // singly linked list
    D_NC_CHECK(DNodeContainerClass<DNctSlist>::is_list_shaped   == true);
    D_NC_CHECK(DNodeContainerClass<DNctSlist>::is_doubly_linked == false);

    // graph
    D_NC_CHECK(DNodeContainerClass<DNctGraph>::shape
                   == DNodeContainerShape::graph);
    D_NC_CHECK(DNodeContainerClass<DNctGraph>::is_graph_shaped == true);

    // the neutral base
    D_NC_CHECK(DNodeContainerClass<DNctGeneric>::is_node_container == true);
    D_NC_CHECK(DNodeContainerClass<DNctGeneric>::shape
                   == DNodeContainerShape::generic);
    D_NC_CHECK(DNodeContainerClass<DNctGeneric>::is_tree_shaped  == false);
    D_NC_CHECK(DNodeContainerClass<DNctGeneric>::is_list_shaped  == false);
    D_NC_CHECK(DNodeContainerClass<DNctGeneric>::is_graph_shaped == false);

    // not a container
    D_NC_CHECK(DNodeContainerClass<DNctBare>::is_node_container == false);
    D_NC_CHECK(DNodeContainerClass<DNctBare>::shape
                   == DNodeContainerShape::unknown);

    // an overlapping fixture: several flags, one shape
    D_NC_CHECK(   ( DNodeContainerClass<DNctAllShapes>::is_tree_shaped
                        == true )
               && ( DNodeContainerClass<DNctAllShapes>::is_list_shaped
                        == true )
               && ( DNodeContainerClass<DNctAllShapes>::is_doubly_linked
                        == true )
               && ( DNodeContainerClass<DNctAllShapes>::is_graph_shaped
                        == true )
               && ( DNodeContainerClass<DNctAllShapes>::shape
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
bool tests_node_container_class_ownership()
{
    bool ok = true;

    // a recognised but incomplete policy
    D_NC_CHECK(DNodeContainerClass<DNctUniqueContainer>::has_ownership
                   == true);
    D_NC_CHECK(DNodeContainerClass<DNctUniqueContainer>::ownership
                   == DOwnershipModel::unique);
    D_NC_CHECK(DNodeContainerClass<DNctUniqueContainer>::is_unique_owning
                   == true);
    D_NC_CHECK(DNodeContainerClass<DNctUniqueContainer>::is_shared_owning
                   == false);
    D_NC_CHECK(DNodeContainerClass<DNctUniqueContainer>::is_owning
                   == false);

    D_NC_CHECK(DNodeContainerClass<DNctSharedContainer>::ownership
                   == DOwnershipModel::shared);
    D_NC_CHECK(DNodeContainerClass<DNctSharedContainer>::is_shared_owning
                   == true);

    D_NC_CHECK(DNodeContainerClass<DNctNonOwningContainer>::ownership
                   == DOwnershipModel::non_owning);
    D_NC_CHECK(DNodeContainerClass<DNctNonOwningContainer>::is_owning
                   == false);

    // an unrecognised but complete owning policy - the mirror case
    D_NC_CHECK(DNodeContainerClass<
                   DNctUnknownPolicyContainer>::has_ownership == true);
    D_NC_CHECK(DNodeContainerClass<DNctUnknownPolicyContainer>::ownership
                   == DOwnershipModel::unknown);
    D_NC_CHECK(DNodeContainerClass<DNctUnknownPolicyContainer>::is_owning
                   == true);
    D_NC_CHECK(DNodeContainerClass<
                   DNctUnknownPolicyContainer>::is_unique_owning == false);
    D_NC_CHECK(DNodeContainerClass<
                   DNctUnknownPolicyContainer>::is_shared_owning == false);

    // no policy at all
    D_NC_CHECK(DNodeContainerClass<DNctList>::has_ownership == false);
    D_NC_CHECK(DNodeContainerClass<DNctList>::ownership
                   == DOwnershipModel::non_owning);
    D_NC_CHECK(DNodeContainerClass<DNctList>::is_owning == false);

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
bool tests_node_container_class_entry()
{
    bool ok = true;

    // tree
    D_NC_CHECK(DNodeContainerClass<DNctTree>::has_entry       == true);
    D_NC_CHECK(DNodeContainerClass<DNctTree>::has_root        == true);
    D_NC_CHECK(DNodeContainerClass<DNctTree>::has_head        == false);
    D_NC_CHECK(DNodeContainerClass<DNctTree>::has_tail        == false);
    D_NC_CHECK(DNodeContainerClass<DNctTree>::has_entries     == false);
    D_NC_CHECK(DNodeContainerClass<DNctTree>::has_entry_count == false);
    D_NC_CHECK(DNodeContainerClass<DNctTree>::has_release     == true);

    // doubly linked list
    D_NC_CHECK(DNodeContainerClass<DNctList>::has_head    == true);
    D_NC_CHECK(DNodeContainerClass<DNctList>::has_tail    == true);
    D_NC_CHECK(DNodeContainerClass<DNctList>::has_root    == false);
    D_NC_CHECK(DNodeContainerClass<DNctList>::has_release == false);

    // graph
    D_NC_CHECK(DNodeContainerClass<DNctGraph>::has_entries     == true);
    D_NC_CHECK(DNodeContainerClass<DNctGraph>::has_entry_count == true);
    D_NC_CHECK(DNodeContainerClass<DNctGraph>::has_head        == false);

    // not a container
    D_NC_CHECK(DNodeContainerClass<DNctBare>::has_entry   == false);
    D_NC_CHECK(DNodeContainerClass<DNctBare>::has_root    == false);
    D_NC_CHECK(DNodeContainerClass<DNctBare>::has_release == false);

    // the naming trap: has_entry is the DISJUNCTION, not the method probe
    D_NC_CHECK(DNodeContainerClass<DNctTree>::has_entry
                   == has_any_entry_point<DNctTree>::value);

    // ... a container with the method but no root reports has_entry false
    D_NC_CHECK(   ( has_has_entry_method<
                        nct_has_entry_no_entry_point>::value == true )
               && ( DNodeContainerClass<
                        nct_has_entry_no_entry_point>::has_entry == false ));

    // ... and one with a root but no method reports has_entry true
    D_NC_CHECK(   ( has_has_entry_method<DNctList>::value == false )
               && ( DNodeContainerClass<DNctList>::has_entry == true ));

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
bool tests_node_container_class_node()
{
    bool ok = true;

    // present
    D_NC_CHECK(DNodeContainerClass<DNctTree>::has_node == true);
    D_NC_STATIC_CHECK(std::is_same<DNodeContainerClass<DNctTree>::node,
                                   DNctNode>::value);
    D_NC_STATIC_CHECK(std::is_same<DNodeContainerClass<DNctGraph>::node,
                                   DNctNode>::value);

    // qualification survives extraction
    D_NC_STATIC_CHECK(
        std::is_same<DNodeContainerClass<nct_const_node_type>::node,
                     const DNctNode>::value);

    // an alias naming void
    D_NC_STATIC_CHECK(
        std::is_same<DNodeContainerClass<nct_void_node_type>::node,
                     void>::value);

    // absent: the pair is unambiguous
    D_NC_CHECK(DNodeContainerClass<DNctBare>::has_node == false);
    D_NC_STATIC_CHECK(std::is_same<DNodeContainerClass<DNctBare>::node,
                                   nonesuch>::value);
    D_NC_CHECK(DNodeContainerClass<int>::has_node == false);
    D_NC_STATIC_CHECK(std::is_same<DNodeContainerClass<int>::node,
                                   nonesuch>::value);

    // the one case the type alone cannot separate
    D_NC_CHECK(DNodeContainerClass<nct_nonesuch_node_type>::has_node == true);
    D_NC_STATIC_CHECK(
        std::is_same<DNodeContainerClass<nct_nonesuch_node_type>::node,
                     DNodeContainerClass<DNctBare>::node>::value);

    // agreement with the standalone extractor
    D_NC_STATIC_CHECK(std::is_same<DNodeContainerClass<DNctTree>::node,
                                   node_type_of_t<DNctTree>>::value);
    D_NC_STATIC_CHECK(std::is_same<DNodeContainerClass<int>::node,
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
bool tests_node_container_class_agreement()
{
    bool ok = true;

    // --- identity and shape, on a tree ---
    D_NC_CHECK(DNodeContainerClass<DNctTree>::is_node_container
                   == is_node_container<DNctTree>::value);
    D_NC_CHECK(DNodeContainerClass<DNctTree>::shape
                   == node_container_shape_of<DNctTree>::value);
    D_NC_CHECK(DNodeContainerClass<DNctTree>::is_tree_shaped
                   == is_tree_shaped_container<DNctTree>::value);
    D_NC_CHECK(DNodeContainerClass<DNctTree>::is_list_shaped
                   == is_list_shaped_container<DNctTree>::value);
    D_NC_CHECK(DNodeContainerClass<DNctTree>::is_doubly_linked
                   == is_doubly_linked_container<DNctTree>::value);
    D_NC_CHECK(DNodeContainerClass<DNctTree>::is_graph_shaped
                   == is_graph_shaped_container<DNctTree>::value);

    // --- identity and shape, on a doubly linked list ---
    D_NC_CHECK(DNodeContainerClass<DNctList>::shape
                   == node_container_shape_of<DNctList>::value);
    D_NC_CHECK(DNodeContainerClass<DNctList>::is_tree_shaped
                   == is_tree_shaped_container<DNctList>::value);
    D_NC_CHECK(DNodeContainerClass<DNctList>::is_list_shaped
                   == is_list_shaped_container<DNctList>::value);
    D_NC_CHECK(DNodeContainerClass<DNctList>::is_doubly_linked
                   == is_doubly_linked_container<DNctList>::value);
    D_NC_CHECK(DNodeContainerClass<DNctList>::is_graph_shaped
                   == is_graph_shaped_container<DNctList>::value);

    // --- ownership, on a recognised policy ---
    D_NC_CHECK(DNodeContainerClass<DNctUniqueContainer>::has_ownership
                   == has_ownership_policy<DNctUniqueContainer>::value);
    D_NC_CHECK(DNodeContainerClass<DNctUniqueContainer>::ownership
                   == ownership_model_of<DNctUniqueContainer>::value);
    D_NC_CHECK(DNodeContainerClass<DNctUniqueContainer>::is_owning
                   == is_owning_container<DNctUniqueContainer>::value);
    D_NC_CHECK(DNodeContainerClass<DNctUniqueContainer>::is_unique_owning
                   == is_unique_owning_container<
                          DNctUniqueContainer>::value);
    D_NC_CHECK(DNodeContainerClass<DNctUniqueContainer>::is_shared_owning
                   == is_shared_owning_container<
                          DNctUniqueContainer>::value);

    // --- ownership, on an unrecognised complete policy ---
    D_NC_CHECK(DNodeContainerClass<DNctUnknownPolicyContainer>::ownership
                   == ownership_model_of<
                          DNctUnknownPolicyContainer>::value);
    D_NC_CHECK(DNodeContainerClass<DNctUnknownPolicyContainer>::is_owning
                   == is_owning_container<
                          DNctUnknownPolicyContainer>::value);

    // --- entry, on a tree ---
    D_NC_CHECK(DNodeContainerClass<DNctTree>::has_entry
                   == has_any_entry_point<DNctTree>::value);
    D_NC_CHECK(DNodeContainerClass<DNctTree>::has_root
                   == has_root_method<DNctTree>::value);
    D_NC_CHECK(DNodeContainerClass<DNctTree>::has_head
                   == has_head_method<DNctTree>::value);
    D_NC_CHECK(DNodeContainerClass<DNctTree>::has_tail
                   == has_tail_method<DNctTree>::value);
    D_NC_CHECK(DNodeContainerClass<DNctTree>::has_entries
                   == has_entries_method<DNctTree>::value);
    D_NC_CHECK(DNodeContainerClass<DNctTree>::has_entry_count
                   == has_entry_count_method<DNctTree>::value);
    D_NC_CHECK(DNodeContainerClass<DNctTree>::has_release
                   == has_release_entry_method<DNctTree>::value);

    // --- entry, on a graph, where a different subset is lit ---
    D_NC_CHECK(DNodeContainerClass<DNctGraph>::has_entry
                   == has_any_entry_point<DNctGraph>::value);
    D_NC_CHECK(DNodeContainerClass<DNctGraph>::has_root
                   == has_root_method<DNctGraph>::value);
    D_NC_CHECK(DNodeContainerClass<DNctGraph>::has_head
                   == has_head_method<DNctGraph>::value);
    D_NC_CHECK(DNodeContainerClass<DNctGraph>::has_tail
                   == has_tail_method<DNctGraph>::value);
    D_NC_CHECK(DNodeContainerClass<DNctGraph>::has_entries
                   == has_entries_method<DNctGraph>::value);
    D_NC_CHECK(DNodeContainerClass<DNctGraph>::has_entry_count
                   == has_entry_count_method<DNctGraph>::value);
    D_NC_CHECK(DNodeContainerClass<DNctGraph>::has_release
                   == has_release_entry_method<DNctGraph>::value);

    // --- node ---
    D_NC_CHECK(DNodeContainerClass<DNctTree>::has_node
                   == has_node_type<DNctTree>::value);
    D_NC_CHECK(DNodeContainerClass<DNctBare>::has_node
                   == has_node_type<DNctBare>::value);

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
bool tests_node_container_class_edge_types()
{
    bool ok = true;

    // void: the whole aggregate must instantiate
    D_NC_CHECK(DNodeContainerClass<void>::is_node_container == false);
    D_NC_CHECK(DNodeContainerClass<void>::shape
                   == DNodeContainerShape::unknown);
    D_NC_CHECK(DNodeContainerClass<void>::has_entry    == false);
    D_NC_CHECK(DNodeContainerClass<void>::has_node     == false);
    D_NC_CHECK(DNodeContainerClass<void>::has_ownership == false);
    D_NC_CHECK(DNodeContainerClass<void>::is_owning    == false);
    D_NC_CHECK(DNodeContainerClass<void>::ownership
                   == DOwnershipModel::non_owning);
    D_NC_STATIC_CHECK(std::is_same<DNodeContainerClass<void>::node,
                                   nonesuch>::value);

    // scalars, pointers, arrays, functions, nonesuch
    D_NC_CHECK(DNodeContainerClass<int>::shape
                   == DNodeContainerShape::unknown);
    D_NC_CHECK(DNodeContainerClass<int*>::shape
                   == DNodeContainerShape::unknown);
    D_NC_CHECK(DNodeContainerClass<int[4]>::shape
                   == DNodeContainerShape::unknown);
    D_NC_CHECK(DNodeContainerClass<void(int)>::shape
                   == DNodeContainerShape::unknown);
    D_NC_CHECK(DNodeContainerClass<nonesuch>::shape
                   == DNodeContainerShape::unknown);
    D_NC_CHECK(DNodeContainerClass<DNctNode>::shape
                   == DNodeContainerShape::unknown);

    // the default ownership answer, for every one of them
    D_NC_CHECK(DNodeContainerClass<int>::ownership
                   == DOwnershipModel::non_owning);
    D_NC_CHECK(DNodeContainerClass<int*>::ownership
                   == DOwnershipModel::non_owning);
    D_NC_CHECK(DNodeContainerClass<nonesuch>::ownership
                   == DOwnershipModel::non_owning);

    // the near-misses manufacture nothing
    D_NC_CHECK(DNodeContainerClass<DNctPrivateRoot>::is_node_container
                   == false);
    D_NC_CHECK(DNodeContainerClass<DNctArgRoot>::is_node_container
                   == false);
    D_NC_CHECK(DNodeContainerClass<DNctDataMemberRoot>::is_node_container
                   == false);
    D_NC_CHECK(DNodeContainerClass<DNctNonconstRoot>::is_node_container
                   == false);
    D_NC_CHECK(DNodeContainerClass<DNctTailNoHead>::is_node_container
                   == false);
    D_NC_CHECK(DNodeContainerClass<DNctEntryCountNoEntries>::shape
                   == DNodeContainerShape::unknown);

    // ... while the admitted method shapes do classify
    D_NC_CHECK(DNodeContainerClass<DNctFnptrRoot>::shape
                   == DNodeContainerShape::tree);
    D_NC_CHECK(DNodeContainerClass<DNctInheritedRoot>::shape
                   == DNodeContainerShape::tree);
    D_NC_CHECK(DNodeContainerClass<DNctVoidRoot>::shape
                   == DNodeContainerShape::tree);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
