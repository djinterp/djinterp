# =============================================================================
# testing/
#   Top of the test tree.  Aggregate over EVERY test exe in the project,
# regardless of which sub-domain (djinterp, etc.) it belongs to.  Today
# there's only one sub-domain, but the slot is here for when there are more.
# =============================================================================

djinterp_add_test_subtree(all_tests)