/*******************************************************************************
* djinterp [core]                                   database_tests_statement.cpp
*
*
* path:      /tests/djinterp/core/db/database_tests_statement.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.10.23
*******************************************************************************/
#include "../../../../inc/djinterp/env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// djinterp
// tests for db/database.hpp section VI (statement CRTP base)
#include "database_tests.hpp"

NS_DJINTERP
namespace testing
{
    /*
    tests_statement_bind_by_index
      Verifies each by-index bind forwards its index and value to the helper.
      Tests the following:
      - bind_null/bind_bool/bind_int/bind_long forward index (+ value)
      - bind_double/bind_string/bind_binary forward index (+ value)
    */
    bool
    tests_statement_bind_by_index()
    {
        stmt_probe probe;
        stmt_idx mock(&probe);
        statement<stmt_idx>& st = mock;

        st.bind_null(0);
        D_DB_CHECK(probe.last_method == "null");
        D_DB_CHECK(probe.last_index == 0);

        st.bind_bool(1, true);
        D_DB_CHECK(probe.last_method == "bool");
        D_DB_CHECK(probe.last_index == 1);
        D_DB_CHECK(probe.v_bool == true);

        st.bind_int(2, 42);
        D_DB_CHECK(probe.last_method == "int");
        D_DB_CHECK(probe.last_index == 2);
        D_DB_CHECK(probe.v_int == 42);

        st.bind_long(3, 9000000000LL);
        D_DB_CHECK(probe.last_method == "long");
        D_DB_CHECK(probe.last_index == 3);
        D_DB_CHECK(probe.v_long == 9000000000LL);

        st.bind_double(4, 1.5);
        D_DB_CHECK(probe.last_method == "double");
        D_DB_CHECK(probe.last_index == 4);
        D_DB_CHECK(probe.v_double == 1.5);

        st.bind_string(5, "hello");
        D_DB_CHECK(probe.last_method == "string");
        D_DB_CHECK(probe.last_index == 5);
        D_DB_CHECK(probe.v_string == "hello");

        st.bind_binary(6, std::vector<re_std::uint8_t>{ 0xAB, 0xCD });
        D_DB_CHECK(probe.last_method == "binary");
        D_DB_CHECK(probe.last_index == 6);
        D_DB_CHECK(probe.v_binary.size() == 2);
        D_DB_CHECK(probe.v_binary[1] == 0xCD);

        return true;
    }

    /*
    tests_statement_bind_by_name
      Verifies each by-name bind dispatches to the SFINAE-gated overload and
      forwards its name and value to the helper.
      Tests the following:
      - bind_null/bind_bool/bind_int/bind_long forward name (+ value)
      - bind_double/bind_string/bind_binary forward name (+ value)
    */
    bool
    tests_statement_bind_by_name()
    {
        stmt_probe probe;
        stmt_name mock(&probe);
        statement<stmt_name>& st = mock;

        st.bind_null(std::string("n0"));
        D_DB_CHECK(probe.last_method == "null");
        D_DB_CHECK(probe.last_name == "n0");

        st.bind_bool(std::string("n1"), true);
        D_DB_CHECK(probe.last_method == "bool");
        D_DB_CHECK(probe.last_name == "n1");
        D_DB_CHECK(probe.v_bool == true);

        st.bind_int(std::string("n2"), 7);
        D_DB_CHECK(probe.last_method == "int");
        D_DB_CHECK(probe.last_name == "n2");
        D_DB_CHECK(probe.v_int == 7);

        st.bind_long(std::string("n3"), 123456789LL);
        D_DB_CHECK(probe.last_method == "long");
        D_DB_CHECK(probe.last_name == "n3");
        D_DB_CHECK(probe.v_long == 123456789LL);

        st.bind_double(std::string("n4"), 2.75);
        D_DB_CHECK(probe.last_method == "double");
        D_DB_CHECK(probe.last_name == "n4");
        D_DB_CHECK(probe.v_double == 2.75);

        st.bind_string(std::string("n5"), std::string("world"));
        D_DB_CHECK(probe.last_method == "string");
        D_DB_CHECK(probe.last_name == "n5");
        D_DB_CHECK(probe.v_string == "world");

        st.bind_binary(std::string("n6"),
                       std::vector<re_std::uint8_t>{ 1, 2, 3, 4 });
        D_DB_CHECK(probe.last_method == "binary");
        D_DB_CHECK(probe.last_name == "n6");
        D_DB_CHECK(probe.v_binary.size() == 4);
        D_DB_CHECK(probe.v_binary[3] == 4);

        return true;
    }

    /*
    tests_statement_bind_by_name_sfinae
      Verifies the by-name bind family is enabled only when the helper
      exposes a single string-taking bind overload.
      Tests the following:
      - the gate is ON for a name-only helper (stmt_name)
      - the gate is OFF for an index-only helper (stmt_idx)
    */
    bool
    tests_statement_bind_by_name_sfinae()
    {
        static_assert(stmt_has_name_bind<statement<stmt_name>>::value,
                      "by-name binds must be enabled for stmt_name");
        static_assert(!stmt_has_name_bind<statement<stmt_idx>>::value,
                      "by-name binds must be disabled for stmt_idx");

        return true;
    }

    /*
    tests_statement_execution
      Verifies the three execution entry points forward and return the
      helper's results.
      Tests the following:
      - execute_query/execute_update/execute forward and return sentinels
      - each is dispatched exactly once
    */
    bool
    tests_statement_execution()
    {
        stmt_probe probe;
        stmt_idx mock(&probe);
        statement<stmt_idx>& st = mock;

        D_DB_CHECK(st.execute_query() == k_stmt_query_val);
        D_DB_CHECK(st.execute_update() == k_stmt_update_val);
        D_DB_CHECK(st.execute() == true);

        D_DB_CHECK(probe.execute_query_calls == 1);
        D_DB_CHECK(probe.execute_update_calls == 1);
        D_DB_CHECK(probe.execute_calls == 1);

        return true;
    }

    /*
    tests_statement_metadata
      Verifies parameter_count and the lifecycle forwarders.
      Tests the following:
      - parameter_count returns the helper's value
      - clear_parameters/reset/close each forward once
    */
    bool
    tests_statement_metadata()
    {
        stmt_probe probe;
        stmt_idx mock(&probe);
        statement<stmt_idx>& st = mock;

        D_DB_CHECK(st.parameter_count() == k_stmt_param_count);

        st.clear_parameters();
        st.reset();
        st.close();

        D_DB_CHECK(probe.clear_calls == 1);
        D_DB_CHECK(probe.reset_calls == 1);
        D_DB_CHECK(probe.close_calls == 1);

        return true;
    }

    /*
    tests_statement_move
      Verifies statement is move-only and that a moved helper still forwards.
      Tests the following:
      - statement is not copy-constructible or copy-assignable
      - a move-constructed helper still forwards through the base
      - a move-assigned helper still forwards through the base
    */
    bool
    tests_statement_move()
    {
        static_assert(!std::is_copy_constructible<statement<stmt_idx>>::value,
                      "statement must be non-copyable");
        static_assert(!std::is_copy_assignable<statement<stmt_idx>>::value,
                      "statement must be non-copy-assignable");

        stmt_probe probe;
        stmt_idx a(&probe);
        stmt_idx b(std::move(a));
        statement<stmt_idx>& sb = b;
        D_DB_CHECK(sb.execute() == true);
        D_DB_CHECK(probe.execute_calls == 1);

        stmt_probe probe1;
        stmt_probe probe2;
        stmt_idx a2(&probe1);
        stmt_idx b2(&probe2);
        b2 = std::move(a2);
        statement<stmt_idx>& sb2 = b2;
        sb2.reset();
        D_DB_CHECK(probe1.reset_calls == 1);
        D_DB_CHECK(probe2.reset_calls == 0);

        return true;
    }

}   // namespace testing
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
