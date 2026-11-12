# =============================================================================
# testing/djinterp/cpp/text/
#   AGGREGATE only -- recurses into the child directories that carry a
# CMakeLists (document_templates/, font/ and writer_stream/) and publishes
# all_djinterp_cpp_text over every test exe at or below here. The parent
# aggregate recurses straight through this file.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.10
# =============================================================================

djinterp_add_test_subtree(all_djinterp_cpp_text)
