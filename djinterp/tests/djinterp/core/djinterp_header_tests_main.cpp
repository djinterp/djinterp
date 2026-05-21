/******************************************************************************
* djinterp [test]                            djinterp_header_tests_main.cpp
*
*   Test runner (harness, NOT a test section).
*
*   This translation unit owns main(). It calls every bool-returning test
* group declared in djinterp_header_tests.hpp, prints a PASS/FAIL line per
* group, and returns a non-zero exit status if any group fails so the build /
* CI can detect failure.  The compile-time guarantees inside each group are
* already enforced by static_assert at translation time; the runtime tally
* here exists so a human (or CI) sees an explicit, per-group result.
******************************************************************************/

#include <cstdio>

#include "./djinterp_header_tests.hpp"


namespace
{

    // test_entry
    //   type: a (name, function-pointer) pair for one test group.
    struct test_entry
    {
        const char* name;
        bool      (*fn)();
    };

    // run_all
    //   harness: invokes every group, prints a result line each, and reports
    // how many failed via the out-parameter.  Returns the total count run.
    int
    run_all(
        int& _failed_out
    )
    {
        using namespace djinterp::test;

        const test_entry entries[] =
        {
            { "tests_keyword_macros",    &tests_keyword_macros    },
            { "tests_namespace_macros",  &tests_namespace_macros  },
            { "tests_constexpr_macros",  &tests_constexpr_macros  },
            { "tests_noexcept_macro",    &tests_noexcept_macro    },
            { "tests_void_t",            &tests_void_t            },
            { "tests_abs_value",         &tests_abs_value         },
            { "tests_abs_value_v",       &tests_abs_value_v       },
            { "tests_abs_value_to_size_t", &tests_abs_value_to_size_t },
            { "tests_clean",             &tests_clean             },
            { "tests_constexpr_swap",    &tests_constexpr_swap    },
            { "tests_repeat",            &tests_repeat            },
            { "tests_self_and_is_self",  &tests_self_and_is_self  },
            { "tests_resolve_self",      &tests_resolve_self      },
        };

        const int count  = static_cast<int>(sizeof(entries)
                                             / sizeof(entries[0]));
        int       failed = 0;

        for (int i = 0; i < count; ++i)
        {
            const bool ok = entries[i].fn();

            if (!ok)
            {
                ++failed;
            }

            std::printf("[ %s ] %s\n",
                        (ok ? "PASS" : "FAIL"),
                        entries[i].name);
        }

        _failed_out = failed;

        return count;
    }

}  // anonymous namespace


int
main()
{
    int failed = 0;
    int total  = run_all(failed);

    std::printf("----------------------------------------\n");
    std::printf("%d/%d groups passed.\n",
                (total - failed),
                total);

    return (failed == 0) ? 0 : 1;
}
