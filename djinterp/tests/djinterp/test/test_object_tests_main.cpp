// std
#include <cstdio>
// djinterp
#include "test_object_tests.hpp"


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


// D_TO_RUN
//   macro: runs ::djinterp::testing::<fn> under its own stringized name.
#define D_TO_RUN(_fn)   run(#_fn, &::djinterp::testing::_fn)


int
main()
{
    std::printf("test_object.hpp unit tests\n");
    std::printf("--------------------------\n");

    // test_metadata
    D_TO_RUN(tests_test_metadata_types);
    D_TO_RUN(tests_test_metadata_set);
    D_TO_RUN(tests_test_metadata_get);
    D_TO_RUN(tests_test_metadata_contains);

    // aliases / status constants / storage / traits
    D_TO_RUN(tests_object_aliases);
    D_TO_RUN(tests_object_status_constants);
    D_TO_RUN(tests_object_storage);
    D_TO_RUN(tests_object_traits);

    // construction
    D_TO_RUN(tests_object_ctor_default);
    D_TO_RUN(tests_object_ctor_type_id);
    D_TO_RUN(tests_object_ctor_type_result);
    D_TO_RUN(tests_object_ctor_metadata);
    D_TO_RUN(tests_object_ctor_noexcept);

    // read-only query surface
    D_TO_RUN(tests_object_bool_conversion);
    D_TO_RUN(tests_object_status_result);
    D_TO_RUN(tests_object_passed_failed);
    D_TO_RUN(tests_object_type_id);
    D_TO_RUN(tests_object_callable_query);

    // mutation
    D_TO_RUN(tests_object_evaluate);
    D_TO_RUN(tests_object_skip);
    D_TO_RUN(tests_object_set_status);
    D_TO_RUN(tests_object_set_type_id);
    D_TO_RUN(tests_object_set_callable_id);
    D_TO_RUN(tests_object_mutation_sequence);

    // metadata access
    D_TO_RUN(tests_object_metadata_accessor);
    D_TO_RUN(tests_object_set_metadata_copy);
    D_TO_RUN(tests_object_set_metadata_move);
    D_TO_RUN(tests_object_metadata_noexcept);

    // convenience aliases
    D_TO_RUN(tests_basic_test);
    D_TO_RUN(tests_tagged_test);

    // factory functions
    D_TO_RUN(tests_make_test);
    D_TO_RUN(tests_make_interior);
    D_TO_RUN(tests_make_interior_named);

    std::printf("--------------------------\n");
    std::printf("passed: %d   failed: %d\n", g_passed, g_failed);

    return (g_failed == 0) ? 0 : 1;
}

#undef D_TO_RUN
