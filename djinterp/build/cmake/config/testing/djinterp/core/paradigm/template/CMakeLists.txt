# =============================================================================
# testing/djinterp/core/paradigm/template/
#   AGGREGATE only — recurses into the template leaf directory and publishes
# all_djinterp_core_paradigm_template over the exe(s) at or below here.  Composes
# upward: paradigm/CMakeLists.txt recurses straight through this file.
# =============================================================================

djinterp_add_test_subtree(all_djinterp_core_paradigm_template)
