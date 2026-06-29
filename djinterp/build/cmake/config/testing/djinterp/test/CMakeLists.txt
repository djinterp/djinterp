# =============================================================================
# testing/djinterp/test/
#   Aggregate over the DTest framework's own self-tests.  AGGREGATE role only:
# each framework header gets its own leaf subdirectory (test_common/,
# test_object/, test_kind/, ...), and this recurses into them and publishes
# all_djinterp_test over the lot.
#
#   Composes upward automatically - the parent testing/djinterp aggregate
# (all_djinterp) and the root testing aggregate (all_tests) recurse straight
# through here, so each leaf is reachable as a member of all three.
#
#   To add another header's suite: drop a sibling leaf directory next to these
# (e.g. test_event/) with its own runner + CMakeLists; the recursion finds it
# with no edit to this file.
# =============================================================================

djinterp_add_test_subtree(all_djinterp_test)
