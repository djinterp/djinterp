# =============================================================================
# djinterp [test] --
# build/cmake/config/testing/djinterp/cpp/modules/net/CMakeLists.txt
#
#  Aggregate for cpp/modules/net's programs: recurses into each leaf. The
# programs are net's C++ tests as they stand, mains over checks of their own,
# each built at C++98 and C++20 and run by CTest on its exit status.
#
#    - all_djinterp_cpp_modules_net : the subtree group target.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.20
# =============================================================================
djinterp_add_test_subtree(all_djinterp_cpp_modules_net)
