#*****************************************************************************
# djinterp [test]                                                CMakeLists.txt
#
#   AGGREGATE for the `pool` subsystem (AGENTS-cmake R4).
#
#   pool is the one exception to the flat memory layout: pool.hpp,
# pool_allocator.hpp and pool_traits.hpp live in inc/djinterp/core/memory/pool,
# their suite headers in tests/djinterp/cpp/memory/pool, and so their config
# recipes sit one level down here - pool/, pool_allocator/ and the pool_all
# aggregate exe.  Everything else on the memory axis is flat and its recipes
# are direct children of core/memory/.
#
#   This directory therefore RECURSES and defines nothing.  It previously held
# a leaf recipe for pool_tests plus a pool_tests_runner.cpp, both keyed to the
# flat tests tree; because a leaf never calls djinterp_add_test_subtree, the
# three real recipes below it were unreachable, and its pool_tests target
# collided by name with pool/pool/.  The runner it owned is superseded by
# pool/pool_tests_runner.cpp and should be deleted.
#
#
# path:      /build/cmake/config/testing/djinterp/core/memory/pool/
#            CMakeLists.txt
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.22
#*****************************************************************************

djinterp_add_test_subtree(all_djinterp_core_memory_pool)
