# =============================================================================
# testing/djinterp/parse/
#   Parse-layer aggregate: recurses into every parse leaf beneath it (cli,
# parse, vparse), which no parent reached before this file: a directory
# without a CMakeLists.txt is skipped by djinterp_add_test_subtree.
#
#     - all_djinterp_parse : the subtree group target.
# =============================================================================
djinterp_add_test_subtree(all_djinterp_parse)
