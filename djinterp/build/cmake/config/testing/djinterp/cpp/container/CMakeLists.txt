# ============================================================================
# djinterp container test subtree  (config aggregate)
#
# The build only recurses into config dirs that carry a CMakeLists.txt (DTest
# guide, Section 6.4).  This one-line aggregate lets the parent core/ recursion
# descend into the container binding suite under traits/.  Merge - don't
# duplicate - if this dir already carries a CMakeLists for other container suites.
# ============================================================================

djinterp_add_test_subtree(container_tests)
