# =============================================================================
# testing/djinterp/cpp/util/document/
#   AGGREGATE only - recurses into the child directories that carry a
# CMakeLists (document_bundle/) and publishes all_djinterp_cpp_util_document
# over every test exe at or below here. The parent aggregate recurses
# straight through this file.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.23
# =============================================================================

djinterp_add_test_subtree(all_djinterp_cpp_util_document)
