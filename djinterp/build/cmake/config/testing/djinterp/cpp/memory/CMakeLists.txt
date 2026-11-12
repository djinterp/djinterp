# ============================================================================
# djinterp memory test subtree  (config aggregate)
#
# The build only recurses into a config directory that carries a CMakeLists.txt
# (DTest guide, Section 6.4).  This one-line aggregate makes the memory/ level a
# group target and recurses into every leaf folder below it (today, memory/,
# the suite of the C++ faces over the memory kernels), so the whole axis builds
# from here.  The group is all_djinterp_cpp_memory; memory_tests is the leaf's.
# ============================================================================

djinterp_add_test_subtree(all_djinterp_cpp_memory)
