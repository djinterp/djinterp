# =============================================================================
# testing/djinterp/core/event/
#   Aggregate over the whole event subsystem: the five per-header leaves
# (event_common, event_handler, event_table, event_registry, event_dispatcher)
# plus event_tests_all (the single-exe union runner).  AGGREGATE role only -
# each component gets its own leaf subdirectory holding its runner + CMakeLists;
# this recurses into them and publishes all_events over the lot.
#
#   Composes upward automatically - the parent all_djinterp_core aggregate (and
# all_djinterp / all_tests above it) recurse straight through here, so every
# event leaf is reachable as a member of all of them.
#
#   To add another event component's suite: drop a sibling leaf directory next
# to these (its runner + a leaf CMakeLists); the recursion finds it with no
# edit to this file.
# =============================================================================

djinterp_add_test_subtree(all_events)
