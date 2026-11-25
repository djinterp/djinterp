# ============================================================================
# djinterp container/tree test subtree  (config aggregate)
#
# The build only recurses into config dirs that carry a CMakeLists.txt. This
# aggregate reaches the tree floor's leaves: file/, nary/, radix/ and path/.
# ============================================================================

djinterp_add_test_subtree(container_tree_tests)
