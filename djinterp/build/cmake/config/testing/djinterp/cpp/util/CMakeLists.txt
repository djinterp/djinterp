# =============================================================================
# testing/djinterp/core/util/
#   AGGREGATE only (GENERATED) — recurses into the child leaf/cluster directories
# (archive, archive_options, compress, compress_options, sort, counter, timer)
# and publishes all_djinterp_core_util over every test exe at or below here.
# Composes upward automatically: the parent core/ aggregate recurses straight
# through this file.  Directories that carry no CMakeLists (color/, metadata/)
# are simply skipped, so a component that has no runner + section-TU pair yet is
# not built.
#
#   This file closes the gap that previously left the whole util/ axis
# unreachable: core/'s aggregate only descends into children that carry a
# CMakeLists, so without this one archive/compress/sort/... were never entered.
# =============================================================================

djinterp_add_test_subtree(all_djinterp_core_util)
