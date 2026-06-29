// std
#include <cstdio>
// djinterp
#include "test_counter_tests.hpp"


namespace
{
    int g_passed = 0;
    int g_failed = 0;

    // run
    //   invokes one test function, prints its PASS/FAIL line, and tallies the
    // outcome into the file-local counters.
    void run(
        const char* _name,
        bool       (*_test)()
    )
    {
        const bool ok = _test();

        std::printf("  [%s] %s\n",
                    ok ? "PASS" : "FAIL",
                    _name);

        if (ok)
        {
            ++g_passed;
        }
        else
        {
            ++g_failed;
        }

        return;
    }
}


// D_TC_RUN
//   macro: runs a named test from djinterp::testing through run(), passing
// both the stringified name and a pointer to the function.
#define D_TC_RUN(_fn)   run(#_fn, &::djinterp::testing::_fn)


int main()
{
    std::printf("test_counter.hpp unit tests\n");

    // construction
    D_TC_RUN(tests_test_counter_default_ctor);
    D_TC_RUN(tests_test_counter_value_ctor);
    D_TC_RUN(tests_test_counter_value_bounds_ctor);
    D_TC_RUN(tests_test_counter_value_bounds_handler_ctor);

    // operations
    D_TC_RUN(tests_test_counter_increment_within_bounds);
    D_TC_RUN(tests_test_counter_increment_exact_max_no_limit);
    D_TC_RUN(tests_test_counter_increment_overshoot_clamps);
    D_TC_RUN(tests_test_counter_increment_at_max_noop);
    D_TC_RUN(tests_test_counter_increment_zero_noop);
    D_TC_RUN(tests_test_counter_decrement_within_bounds);
    D_TC_RUN(tests_test_counter_decrement_undershoot_clamps);
    D_TC_RUN(tests_test_counter_decrement_at_min_noop);
    D_TC_RUN(tests_test_counter_decrement_zero_noop);
    D_TC_RUN(tests_test_counter_reset_fires_on_reset);
    D_TC_RUN(tests_test_counter_reset_all_leaf);
    D_TC_RUN(tests_test_counter_reset_all_recurses);

    // accessors
    D_TC_RUN(tests_test_counter_value);
    D_TC_RUN(tests_test_counter_initial);
    D_TC_RUN(tests_test_counter_min);
    D_TC_RUN(tests_test_counter_max);
    D_TC_RUN(tests_test_counter_at_min);
    D_TC_RUN(tests_test_counter_at_max);

    // children (owning)
    D_TC_RUN(tests_test_counter_add_child);
    D_TC_RUN(tests_test_counter_add_child_with_bounds);
    D_TC_RUN(tests_test_counter_multiple_children);
    D_TC_RUN(tests_test_counter_child_access);
    D_TC_RUN(tests_test_counter_child_access_const);
    D_TC_RUN(tests_test_counter_child_count);
    D_TC_RUN(tests_test_counter_children_independent);
    D_TC_RUN(tests_test_counter_copy_deep_copies_children);

    // children (non-owning / observed)
    D_TC_RUN(tests_test_counter_observe);
    D_TC_RUN(tests_test_counter_observe_multiple);
    D_TC_RUN(tests_test_counter_observed_out_of_range);
    D_TC_RUN(tests_test_counter_observed_live_view);
    D_TC_RUN(tests_test_counter_observed_count);
    D_TC_RUN(tests_test_counter_observe_non_owning);

    // events
    D_TC_RUN(tests_test_counter_event_tag_names);
    D_TC_RUN(tests_test_counter_event_tag_args);
    D_TC_RUN(tests_test_counter_no_handler_noop);
    D_TC_RUN(tests_test_counter_handler_accessor);
    D_TC_RUN(tests_test_counter_set_handler_attach_detach);
    D_TC_RUN(tests_test_counter_set_handler_swaps);

    std::printf("passed: %d   failed: %d\n",
                g_passed,
                g_failed);

    return (g_failed == 0) ? 0 : 1;
}

#undef D_TC_RUN
