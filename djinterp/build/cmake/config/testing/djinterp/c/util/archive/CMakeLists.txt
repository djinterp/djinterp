# =============================================================================
# testing/djinterp/c/util/archive/
#   AGGREGATE only -- recurses into the child directories that carry a
# CMakeLists (archive_kernel/) and publishes all_djinterp_c_util_archive over
# every test exe at or below here. The parent aggregate recurses straight
# through this file.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.11
# =============================================================================

djinterp_add_test_subtree(all_djinterp_c_util_archive)
