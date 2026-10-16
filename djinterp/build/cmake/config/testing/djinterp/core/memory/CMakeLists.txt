# ============================================================================
# djinterp memory test subtree  (config aggregate)
#
# The build only recurses into a config directory that carries a CMakeLists.txt
# (DTest guide, Section 6.4).  This one-line aggregate makes the memory/ level a
# group target and recurses into every per-module leaf folder below it, so the
# whole axis builds from here.  Folders without a CMakeLists (e.g. pool/ until it
# gains a suite) are simply skipped.
# ============================================================================

djinterp_add_test_subtree(memory_tests)
