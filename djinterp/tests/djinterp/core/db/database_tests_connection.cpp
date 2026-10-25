/*******************************************************************************
* djinterp [core]                                  database_tests_connection.cpp
*
*
* path:      /tests/djinterp/core/db/database_tests_connection.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.10.21
*******************************************************************************/
#include "../../../../inc/djinterp/env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// djinterp
// tests for db/database.hpp section VIII (connection CRTP base)
#include "database_tests.hpp"

NS_DJINTERP
namespace testing
{
    /*
    tests_connection_initial_state
      Verifies the base default constructor's initial state.
      Tests the following:
      - the initial state is disconnected and no transaction is in progress
      - auto-commit defaults to true
      - the default config carries connection_config's own defaults
    */
    bool
    tests_connection_initial_state()
    {
        conn_probe probe;
        mock_conn mock(&probe);
        const connection<mock_conn>& conn = mock;

        D_DB_CHECK(conn.get_state() == connection_state::disconnected);
        D_DB_CHECK(conn.in_transaction() == false);
        D_DB_CHECK(conn.get_auto_commit() == true);
        D_DB_CHECK(conn.get_config().host == "localhost");
        D_DB_CHECK(conn.get_config().verify_ssl == true);

        return true;
    }

    /*
    tests_connection_lifecycle
      Verifies the connection-management forwarders and their return values.
      Tests the following:
      - connect/disconnect/reconnect each forward once
      - is_connected/ping forward and return the helper's values (both truthy
        and falsy)
    */
    bool
    tests_connection_lifecycle()
    {
        conn_probe probe;
        mock_conn mock(&probe);
        connection<mock_conn>& conn = mock;

        conn.connect();
        conn.disconnect();
        conn.reconnect();
        D_DB_CHECK(probe.connect_calls == 1);
        D_DB_CHECK(probe.disconnect_calls == 1);
        D_DB_CHECK(probe.reconnect_calls == 1);

        D_DB_CHECK(conn.is_connected() == true);
        D_DB_CHECK(conn.ping() == true);
        D_DB_CHECK(probe.is_connected_calls == 1);
        D_DB_CHECK(probe.ping_calls == 1);

        probe.is_connected_ret = false;
        probe.ping_ret         = false;
        D_DB_CHECK(conn.is_connected() == false);
        D_DB_CHECK(conn.ping() == false);

        return true;
    }

    /*
    tests_connection_execution
      Verifies the query-execution forwarders pass the query and return the
      helper's values.
      Tests the following:
      - execute_query/execute_update/execute forward the query string
      - each returns the helper's sentinel/result
    */
    bool
    tests_connection_execution()
    {
        conn_probe probe;
        mock_conn mock(&probe);
        connection<mock_conn>& conn = mock;

        D_DB_CHECK(conn.execute_query("q1") == k_conn_query_val);
        D_DB_CHECK(probe.last_query == "q1");

        D_DB_CHECK(conn.execute_update("q2") == k_conn_update_val);
        D_DB_CHECK(probe.last_query == "q2");

        D_DB_CHECK(conn.execute("q3") == true);
        D_DB_CHECK(probe.last_query == "q3");

        return true;
    }

    /*
    tests_connection_prepare
      Verifies the prepared-statement forwarder.
      Tests the following:
      - prepare forwards the query and returns the helper's result
    */
    bool
    tests_connection_prepare()
    {
        conn_probe probe;
        mock_conn mock(&probe);
        connection<mock_conn>& conn = mock;

        D_DB_CHECK(conn.prepare("SELECT 1") == k_conn_prepare_val);
        D_DB_CHECK(probe.last_query == "SELECT 1");

        return true;
    }

    /*
    tests_connection_transactions
      Verifies the transaction forwarders and the in_transaction flag.
      Tests the following:
      - begin_transaction forwards and sets in_transaction true
      - commit forwards and clears in_transaction
      - rollback forwards and clears in_transaction
    */
    bool
    tests_connection_transactions()
    {
        conn_probe probe;
        mock_conn mock(&probe);
        connection<mock_conn>& conn = mock;

        conn.begin_transaction();
        D_DB_CHECK(probe.begin_calls == 1);
        D_DB_CHECK(conn.in_transaction() == true);

        conn.commit();
        D_DB_CHECK(probe.commit_calls == 1);
        D_DB_CHECK(conn.in_transaction() == false);

        conn.begin_transaction();
        D_DB_CHECK(probe.begin_calls == 2);
        D_DB_CHECK(conn.in_transaction() == true);

        conn.rollback();
        D_DB_CHECK(probe.rollback_calls == 1);
        D_DB_CHECK(conn.in_transaction() == false);

        return true;
    }

    /*
    tests_connection_isolation_sfinae
      Verifies the SFINAE-gated isolation operations forward when the helper
      provides them.
      Tests the following:
      - the begin(iso)/get/set isolation gates are enabled for mock_conn
      - begin_transaction(isolation) forwards the level and sets the flag
      - get_isolation_level/set_isolation_level forward
    */
    bool
    tests_connection_isolation_sfinae()
    {
        static_assert(conn_has_begin_iso<connection<mock_conn>>::value,
                      "begin_transaction(iso) must be enabled for mock_conn");
        static_assert(conn_has_get_iso<connection<mock_conn>>::value,
                      "get_isolation_level must be enabled for mock_conn");
        static_assert(conn_has_set_iso<connection<mock_conn>>::value,
                      "set_isolation_level must be enabled for mock_conn");

        conn_probe probe;
        mock_conn mock(&probe);
        connection<mock_conn>& conn = mock;

        conn.begin_transaction(isolation_level::serializable);
        D_DB_CHECK(probe.begin_iso_calls == 1);
        D_DB_CHECK(probe.last_set_iso == isolation_level::serializable);
        D_DB_CHECK(conn.in_transaction() == true);

        D_DB_CHECK(conn.get_isolation_level() == isolation_level::repeatable_read);
        D_DB_CHECK(probe.get_iso_calls == 1);

        conn.set_isolation_level(isolation_level::read_committed);
        D_DB_CHECK(probe.set_iso_calls == 1);
        D_DB_CHECK(probe.last_set_iso == isolation_level::read_committed);

        conn.commit();

        return true;
    }

    /*
    tests_connection_savepoints
      Verifies the SFINAE-gated savepoint forwarders pass their names.
      Tests the following:
      - the savepoint gate is enabled for mock_conn
      - create/rollback_to/release savepoint each forward once with the name
    */
    bool
    tests_connection_savepoints()
    {
        static_assert(conn_has_savepoint<connection<mock_conn>>::value,
                      "savepoints must be enabled for mock_conn");

        conn_probe probe;
        mock_conn mock(&probe);
        connection<mock_conn>& conn = mock;

        conn.create_savepoint("s1");
        D_DB_CHECK(probe.savepoint_calls == 1);
        D_DB_CHECK(probe.last_savepoint == "s1");

        conn.rollback_to_savepoint("s2");
        D_DB_CHECK(probe.rollback_sp_calls == 1);
        D_DB_CHECK(probe.last_savepoint == "s2");

        conn.release_savepoint("s3");
        D_DB_CHECK(probe.release_sp_calls == 1);
        D_DB_CHECK(probe.last_savepoint == "s3");

        return true;
    }

    /*
    tests_connection_metadata
      Verifies the metadata forwarders and the state accessor.
      Tests the following:
      - database type/name and server version forward
      - get_state returns the base state; insert-id/affected-rows forward
    */
    bool
    tests_connection_metadata()
    {
        conn_probe probe;
        mock_conn mock(&probe);
        const connection<mock_conn>& conn = mock;

        D_DB_CHECK(conn.get_database_type() == database_type::sqlite);
        D_DB_CHECK(conn.get_database_name() == "mockdb");
        D_DB_CHECK(conn.get_server_version() == "9.9.9");
        D_DB_CHECK(conn.get_state() == connection_state::disconnected);
        D_DB_CHECK(conn.get_last_insert_id() == k_conn_insert_id);
        D_DB_CHECK(conn.get_affected_rows() == k_conn_affected);

        return true;
    }

    /*
    tests_connection_config_and_autocommit
      Verifies the config accessor and the auto-commit forward/flag pairing.
      Tests the following:
      - get_config returns the (default) config
      - set_auto_commit forwards and updates the cached flag both ways
    */
    bool
    tests_connection_config_and_autocommit()
    {
        conn_probe probe;
        mock_conn mock(&probe);
        connection<mock_conn>& conn = mock;

        D_DB_CHECK(conn.get_config().host == "localhost");
        D_DB_CHECK(conn.get_auto_commit() == true);

        conn.set_auto_commit(false);
        D_DB_CHECK(probe.set_autocommit_calls == 1);
        D_DB_CHECK(probe.last_autocommit == false);
        D_DB_CHECK(conn.get_auto_commit() == false);

        conn.set_auto_commit(true);
        D_DB_CHECK(conn.get_auto_commit() == true);

        return true;
    }

    /*
    tests_connection_error_handling
      Verifies the error-reporting forwarders.
      Tests the following:
      - get_last_error and get_last_error_code forward the helper's values
    */
    bool
    tests_connection_error_handling()
    {
        conn_probe probe;
        mock_conn mock(&probe);
        const connection<mock_conn>& conn = mock;

        D_DB_CHECK(conn.get_last_error() == "no error");
        D_DB_CHECK(conn.get_last_error_code() == k_conn_error_code);

        return true;
    }

    /*
    tests_connection_move
      Verifies connection is move-only and a moved helper still forwards.
      Tests the following:
      - connection is not copy-constructible or copy-assignable
      - a move-constructed helper still forwards through the base
      - a move-assigned helper still forwards through the base
    */
    bool
    tests_connection_move()
    {
        static_assert(!std::is_copy_constructible<connection<mock_conn>>::value,
                      "connection must be non-copyable");
        static_assert(!std::is_copy_assignable<connection<mock_conn>>::value,
                      "connection must be non-copy-assignable");

        conn_probe probe;
        mock_conn a(&probe);
        mock_conn b(std::move(a));
        connection<mock_conn>& cb = b;
        cb.connect();
        D_DB_CHECK(probe.connect_calls == 1);

        conn_probe probe1;
        conn_probe probe2;
        mock_conn a2(&probe1);
        mock_conn b2(&probe2);
        b2 = std::move(a2);
        connection<mock_conn>& cb2 = b2;
        cb2.disconnect();
        D_DB_CHECK(probe1.disconnect_calls == 1);
        D_DB_CHECK(probe2.disconnect_calls == 0);

        return true;
    }

    /*
    tests_connection_absent_sfinae
      Verifies the optional operations are disabled when the helper omits
      them, while the always-present no-arg begin path still works.
      Tests the following:
      - begin(iso)/get/set isolation and savepoint gates are OFF for a
        minimal helper
      - the non-template begin_transaction() still forwards and sets the flag
    */
    bool
    tests_connection_absent_sfinae()
    {
        static_assert(!conn_has_begin_iso<connection<mock_conn_minimal>>::value,
                      "begin_transaction(iso) must be disabled");
        static_assert(!conn_has_get_iso<connection<mock_conn_minimal>>::value,
                      "get_isolation_level must be disabled");
        static_assert(!conn_has_set_iso<connection<mock_conn_minimal>>::value,
                      "set_isolation_level must be disabled");
        static_assert(!conn_has_savepoint<connection<mock_conn_minimal>>::value,
                      "savepoints must be disabled");

        mock_conn_minimal mock;
        connection<mock_conn_minimal>& conn = mock;
        conn.begin_transaction();
        D_DB_CHECK(conn.in_transaction() == true);

        return true;
    }

}   // namespace testing
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
