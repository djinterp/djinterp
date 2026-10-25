# =============================================================================
# testing/djinterp/test/sync/
#   AGGREGATE only -- recurses into the child leaf directories and publishes
# all_djinterp_test_sync over every test exe at or below here, so that the parent's
# recursion reaches them (the owner's ruling of 2026.10.23 on the leaves left
# unreached).
# =============================================================================

djinterp_add_test_subtree(all_djinterp_test_sync)
