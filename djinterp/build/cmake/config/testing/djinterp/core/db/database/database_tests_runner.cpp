// std
#include <cstdio>
// djinterp
#include "database_tests.hpp"

namespace
{
    int g_passed = 0;
    int g_failed = 0;

    void run(const char* _name, bool (*_test)())
    {
        const bool ok = _test();
        std::printf("  [%s] %s\n", ok ? "PASS" : "FAIL", _name);
        if (ok) { ++g_passed; } else { ++g_failed; }
    }
}

// D_DB_ prefix is unique to the database suite so co-compiled suites never
// collide on the macro name.
#define D_DB_RUN(_fn)   run(#_fn, &::djinterp::testing::_fn)

int
main()
{
    std::printf("database.hpp unit tests\n");

    // -- enumerations + type aliases --------------------------------------
    D_DB_RUN(tests_database_type_enum);
    D_DB_RUN(tests_isolation_level_enum);
    D_DB_RUN(tests_connection_state_enum);
    D_DB_RUN(tests_field_type_enum);
    D_DB_RUN(tests_store_direction_enum);
    D_DB_RUN(tests_value_variant);
    D_DB_RUN(tests_type_aliases);

    // -- exception hierarchy ----------------------------------------------
    D_DB_RUN(tests_exception_base);
    D_DB_RUN(tests_exception_what_and_code);
    D_DB_RUN(tests_connection_exception);
    D_DB_RUN(tests_query_exception);
    D_DB_RUN(tests_transaction_exception);
    D_DB_RUN(tests_type_conversion_exception);
    D_DB_RUN(tests_exception_polymorphism);

    // -- configuration structs --------------------------------------------
    D_DB_RUN(tests_connection_config_defaults);
    D_DB_RUN(tests_connection_config_mutation);
    D_DB_RUN(tests_pool_config_defaults);
    D_DB_RUN(tests_pool_config_mutation);

    // -- vendor metadata --------------------------------------------------
    D_DB_RUN(tests_vendor_info_defaults);
    D_DB_RUN(tests_vendor_traits_primary_statics);
    D_DB_RUN(tests_vendor_traits_get_info);
    D_DB_RUN(tests_vendor_traits_make_default_config);

    // -- result_set -------------------------------------------------------
    D_DB_RUN(tests_result_set_navigation);
    D_DB_RUN(tests_result_set_absolute_sfinae);
    D_DB_RUN(tests_result_set_metadata);
    D_DB_RUN(tests_result_set_access_by_index);
    D_DB_RUN(tests_result_set_access_by_name);
    D_DB_RUN(tests_result_set_convenience);
    D_DB_RUN(tests_result_set_move);

    // -- statement --------------------------------------------------------
    D_DB_RUN(tests_statement_bind_by_index);
    D_DB_RUN(tests_statement_bind_by_name);
    D_DB_RUN(tests_statement_bind_by_name_sfinae);
    D_DB_RUN(tests_statement_execution);
    D_DB_RUN(tests_statement_metadata);
    D_DB_RUN(tests_statement_move);

    // -- transaction ------------------------------------------------------
    D_DB_RUN(tests_transaction_begin_commit);
    D_DB_RUN(tests_transaction_begin_isolation);
    D_DB_RUN(tests_transaction_rollback);
    D_DB_RUN(tests_transaction_dtor_rollback);
    D_DB_RUN(tests_transaction_inactive_throws);
    D_DB_RUN(tests_transaction_move_ctor);
    D_DB_RUN(tests_transaction_move_assign);
    D_DB_RUN(tests_transaction_savepoints);
    D_DB_RUN(tests_transaction_savepoint_sfinae);
    D_DB_RUN(tests_transaction_accessors);

    // -- connection -------------------------------------------------------
    D_DB_RUN(tests_connection_initial_state);
    D_DB_RUN(tests_connection_lifecycle);
    D_DB_RUN(tests_connection_execution);
    D_DB_RUN(tests_connection_prepare);
    D_DB_RUN(tests_connection_transactions);
    D_DB_RUN(tests_connection_isolation_sfinae);
    D_DB_RUN(tests_connection_savepoints);
    D_DB_RUN(tests_connection_metadata);
    D_DB_RUN(tests_connection_config_and_autocommit);
    D_DB_RUN(tests_connection_error_handling);
    D_DB_RUN(tests_connection_move);
    D_DB_RUN(tests_connection_absent_sfinae);

    // -- connection_pool / pooled_connection ------------------------------
    D_DB_RUN(tests_pool_construction_preallocate);
    D_DB_RUN(tests_pool_acquire_from_idle);
    D_DB_RUN(tests_pool_acquire_create_new);
    D_DB_RUN(tests_pool_acquire_exhausted);
    D_DB_RUN(tests_pool_acquire_validate_recreate);
    D_DB_RUN(tests_pool_release_basic);
    D_DB_RUN(tests_pool_release_null_and_notfound);
    D_DB_RUN(tests_pool_release_validate_drop);
    D_DB_RUN(tests_pool_release_idle_cap);
    D_DB_RUN(tests_pool_clear);
    D_DB_RUN(tests_pool_resize_grow_shrink);
    D_DB_RUN(tests_pool_counters_and_config);
    D_DB_RUN(tests_pool_create_connection_failure);
    D_DB_RUN(tests_pool_validate_connection_branches);
    D_DB_RUN(tests_pool_cleanup_idle);
    D_DB_RUN(tests_pooled_connection_accessors);
    D_DB_RUN(tests_pooled_connection_move);
    D_DB_RUN(tests_pooled_connection_release_idempotent);

    std::printf("passed: %d   failed: %d\n", g_passed, g_failed);
    return (g_failed == 0) ? 0 : 1;
}

#undef D_DB_RUN
