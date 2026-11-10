# =============================================================================
# testing/djinterp/cpp/util/pdf/
#   AGGREGATE only -- recurses into the child directories that carry a
# CMakeLists (pdf_stream/) and publishes all_djinterp_cpp_util_pdf over every
# test exe at or below here. The parent aggregate recurses straight through
# this file.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.09
# =============================================================================

djinterp_add_test_subtree(all_djinterp_cpp_util_pdf)
