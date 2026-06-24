// std
#include <cstdio>
// djinterp
#include "test_kind_tests.hpp"


namespace
{
    int g_passed = 0;
    int g_failed = 0;

    // run
    //   helper: invokes one test, prints a per-test verdict, and tallies it.
    void
    run(
        const char* _name,
        bool      (*_test)()
    )
    {
        const bool ok = _test();

        std::printf("  [%s] %s\n", ok ? "PASS" : "FAIL", _name);

        if (ok)
        {
            ++g_passed;
        }
        else
        {
            ++g_failed;
        }
    }
}  // namespace


// D_TK_RUN
//   macro: runs ::djinterp::testing::<fn> under its own stringized name.
#define D_TK_RUN(_fn)   run(#_fn, &::djinterp::testing::_fn)


int
main()
{
    std::printf("test_kind.hpp unit tests\n");
    std::printf("------------------------\n");

    // I. test kind record
    D_TK_RUN(tests_test_kind_aggregate);
    D_TK_RUN(tests_test_kind_members);
    D_TK_RUN(tests_test_kind_traits);
    D_TK_RUN(tests_test_kind_values);

    // II. factory function
    D_TK_RUN(tests_make_test_kind_basic);
    D_TK_RUN(tests_make_test_kind_default_arg);
    D_TK_RUN(tests_make_test_kind_constexpr);
    D_TK_RUN(tests_make_test_kind_noexcept);

    // III. test kind set -- construction / aliases
    D_TK_RUN(tests_set_aliases);
    D_TK_RUN(tests_set_default_ctor);
    D_TK_RUN(tests_set_copy_ctor);
    D_TK_RUN(tests_set_move_ctor);
    D_TK_RUN(tests_set_underlying);
    D_TK_RUN(tests_set_ctor_noexcept);

    // III. test kind set -- forwarded surface + iteration
    D_TK_RUN(tests_set_size_empty);
    D_TK_RUN(tests_set_clear);
    D_TK_RUN(tests_set_insert);
    D_TK_RUN(tests_set_erase);
    D_TK_RUN(tests_set_find);
    D_TK_RUN(tests_set_iteration);

    // III. test kind set -- contains + dispatch
    D_TK_RUN(tests_set_contains_native);
    D_TK_RUN(tests_set_contains_fallback);
    D_TK_RUN(tests_set_contains_detection);

    // IV. resolved queries
    D_TK_RUN(tests_find_kind);
    D_TK_RUN(tests_rank_of);
    D_TK_RUN(tests_is_leaf);
    D_TK_RUN(tests_is_interior);
    D_TK_RUN(tests_name_of);
    D_TK_RUN(tests_default_options);
    D_TK_RUN(tests_can_be_child_of);
    D_TK_RUN(tests_queries_compose);

    // V. structural detection
    D_TK_RUN(tests_is_test_kind_set);
    D_TK_RUN(tests_is_test_kind_set_cvref);
    D_TK_RUN(tests_is_test_kind_set_variable);

    std::printf("------------------------\n");
    std::printf("passed: %d   failed: %d\n", g_passed, g_failed);

    return (g_failed == 0) ? 0 : 1;
}

#undef D_TK_RUN
