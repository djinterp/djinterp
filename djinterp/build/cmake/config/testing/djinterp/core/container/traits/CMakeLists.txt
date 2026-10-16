# =============================================================================
# testing/djinterp/core/container/traits/  ::  AGGREGATE (AGENTS-cmake R4)
#
#   This directory holds 25 per-component leaves (container_traits,
# flat_container_traits, container_memory_strategy, ...), so its only job is to
# recurse.  It previously called djinterp_add_test_executable() directly, which
# made it a LEAF - and because a leaf never calls djinterp_add_test_subtree(),
# CMake never descended into any of the child recipes.  Every traits suite below
# here was silently unbuilt, and the one exe defined here collided by name with
# the stale copy under core/memory/container_memory_strategy/.
#
#   The binding suite now lives in its own child directory beside its siblings
# (see container_memory_strategy/CMakeLists.txt).
# =============================================================================

djinterp_add_test_subtree(all_djinterp_core_container_traits)
