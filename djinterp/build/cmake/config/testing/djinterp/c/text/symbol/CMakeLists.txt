# =============================================================================
# testing/djinterp/c/text/symbol/
#   AGGREGATE only - recurses into the child directories that carry a
# CMakeLists (ascii/ and emoji/) and publishes all_djinterp_c_text_symbol over
# every test exe at or below here. The parent aggregate recurses straight
# through this file.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.23
# =============================================================================

djinterp_add_test_subtree(all_djinterp_c_text_symbol)
