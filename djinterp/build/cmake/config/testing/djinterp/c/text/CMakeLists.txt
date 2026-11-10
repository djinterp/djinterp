# =============================================================================
# testing/djinterp/c/text/
#   AGGREGATE only -- recurses into the child directories that carry a
# CMakeLists (ostream_template/, text_template/, text_template_overflow/ and
# str_interp/) and publishes all_djinterp_c_text over every test exe at or
# below here. The parent aggregate recurses straight through this file.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.09
# =============================================================================

djinterp_add_test_subtree(all_djinterp_c_text)
