# =============================================================================
# djinterp [test] --
# build/cmake/config/testing/djinterp/c/modules/db/CMakeLists.txt
#
#   Aggregate for c/modules/db's suites: recurses into each vendor's leaf
# (sqlite, mariadb, postgres, mongodb), every one in DTest's C face.
#
#     - all_djinterp_c_modules_db : the subtree group target.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.10
# =============================================================================
djinterp_add_test_subtree(all_djinterp_c_modules_db)
