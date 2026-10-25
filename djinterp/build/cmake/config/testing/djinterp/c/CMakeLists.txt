# =============================================================================
# testing/djinterp/c/
#   C-layer aggregate: recurses into every C subsystem leaf beneath it (today
# filesys), which no parent reached before this file: a directory without a
# CMakeLists.txt is skipped by djinterp_add_test_subtree.
#
#     - all_djinterp_c : the subtree group target.
# =============================================================================
djinterp_add_test_subtree(all_djinterp_c)
