/*******************************************************************************
* djinterp [core]                                  database_tests_result_set.cpp
*
*
* path:      /tests/djinterp/core/db/database_tests_result_set.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.10.23
*******************************************************************************/
#include "../../../../inc/djinterp/env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// djinterp
// tests for db/database.hpp section V (result_set CRTP base)
#include "database_tests.hpp"

NS_DJINTERP
namespace testing
{
    /*
    tests_result_set_navigation
      Verifies the base forwards each cursor-navigation call to the helper
      and returns the helper's result.
      Tests the following:
      - next/previous/first/last forward and return the helper's value
      - each is dispatched exactly once per call
    */
    bool
    tests_result_set_navigation()
    {
        rs_probe probe;
        mock_result_set mock(&probe);
        result_set<mock_result_set>& rs = mock;

        D_DB_CHECK(rs.next()     == true);
        D_DB_CHECK(rs.previous() == false);
        D_DB_CHECK(rs.first()    == true);
        D_DB_CHECK(rs.last()     == false);

        D_DB_CHECK(probe.next_calls == 1);
        D_DB_CHECK(probe.previous_calls == 1);
        D_DB_CHECK(probe.first_calls == 1);
        D_DB_CHECK(probe.last_calls == 1);

        return true;
    }

    /*
    tests_result_set_absolute_sfinae
      Verifies the SFINAE-gated absolute() is enabled only when the helper
      provides absolute(), and forwards its argument when present.
      Tests the following:
      - the gate is ON for a helper with absolute(), OFF for one without
      - absolute() forwards the row argument and returns the helper's value
    */
    bool
    tests_result_set_absolute_sfinae()
    {
        static_assert(rs_has_absolute<result_set<mock_result_set>>::value,
                      "absolute() must be enabled for mock_result_set");
        static_assert(!rs_has_absolute<result_set<rs_no_absolute>>::value,
                      "absolute() must be disabled for rs_no_absolute");

        rs_probe probe;
        mock_result_set mock(&probe);
        result_set<mock_result_set>& rs = mock;

        D_DB_CHECK(rs.absolute(7) == true);
        D_DB_CHECK(probe.absolute_calls == 1);
        D_DB_CHECK(probe.last_row == 7);

        return true;
    }

    /*
    tests_result_set_metadata
      Verifies the metadata accessors forward (and, where applicable,
      forward their index/name argument).
      Tests the following:
      - column_count/row_count/current_row return the helper's values
      - column_name/column_type forward the index argument
      - column_index forwards the name argument
    */
    bool
    tests_result_set_metadata()
    {
        rs_probe probe;
        mock_result_set mock(&probe);
        const result_set<mock_result_set>& rs = mock;

        D_DB_CHECK(rs.column_count() == k_rs_column_count);

        D_DB_CHECK(rs.column_name(3) == "3");
        D_DB_CHECK(probe.last_index == 3);

        D_DB_CHECK(rs.column_type(2) == k_rs_column_type);
        D_DB_CHECK(probe.last_index == 2);

        D_DB_CHECK(rs.column_index("abcd") == 4);
        D_DB_CHECK(probe.last_name == "abcd");

        D_DB_CHECK(rs.row_count() == k_rs_row_count);
        D_DB_CHECK(rs.current_row() == k_rs_current_row);

        return true;
    }

    /*
    tests_result_set_access_by_index
      Verifies the by-index data accessors forward the index and return the
      helper's typed values.
      Tests the following:
      - get_value/is_null forward the index and return the helper's result
      - get_bool/get_int/get_long/get_double/get_string/get_binary forward
    */
    bool
    tests_result_set_access_by_index()
    {
        rs_probe probe;
        mock_result_set mock(&probe);
        const result_set<mock_result_set>& rs = mock;

        value v = rs.get_value(std::size_t{5});
        D_DB_CHECK(std::holds_alternative<re_std::int64_t>(v));
        D_DB_CHECK(std::get<re_std::int64_t>(v) == 5);
        D_DB_CHECK(probe.last_index == 5);

        D_DB_CHECK(rs.is_null(std::size_t{2}) == true);
        D_DB_CHECK(probe.last_index == 2);

        auto b = rs.get_bool(std::size_t{0});
        D_DB_CHECK(b.has_value() && b.value() == true);

        auto i = rs.get_int(std::size_t{1});
        D_DB_CHECK(i.has_value() && i.value() == k_rs_int_by_index);

        auto l = rs.get_long(std::size_t{1});
        D_DB_CHECK(l.has_value() && l.value() == k_rs_long);

        auto d = rs.get_double(std::size_t{1});
        D_DB_CHECK(d.has_value() && d.value() == k_rs_double);

        auto s = rs.get_string(std::size_t{1});
        D_DB_CHECK(s.has_value() && s.value() == "by_index");

        auto bin = rs.get_binary(std::size_t{1});
        D_DB_CHECK(bin.has_value());
        D_DB_CHECK(bin.value().size() == 3);
        D_DB_CHECK(bin.value()[0] == 1);

        return true;
    }

    /*
    tests_result_set_access_by_name
      Verifies the by-name data accessors dispatch to the name overloads,
      forward the name, and return the helper's typed values.
      Tests the following:
      - get_value/is_null resolve the name overload and forward the name
      - the typed get_* by-name accessors forward and return distinctly
    */
    bool
    tests_result_set_access_by_name()
    {
        rs_probe probe;
        mock_result_set mock(&probe);
        const result_set<mock_result_set>& rs = mock;

        value v = rs.get_value(std::string("col"));
        D_DB_CHECK(std::holds_alternative<std::string>(v));
        D_DB_CHECK(std::get<std::string>(v) == "col");
        D_DB_CHECK(probe.last_name == "col");

        D_DB_CHECK(rs.is_null(std::string("x")) == false);
        D_DB_CHECK(probe.last_name == "x");

        auto b = rs.get_bool(std::string("y"));
        D_DB_CHECK(b.has_value() && b.value() == false);

        auto i = rs.get_int(std::string("abc"));
        D_DB_CHECK(i.has_value() && i.value() == 3);

        auto l = rs.get_long(std::string("abcd"));
        D_DB_CHECK(l.has_value() && l.value() == 4);

        auto d = rs.get_double(std::string("ab"));
        D_DB_CHECK(d.has_value() && d.value() == 2.0);

        auto s = rs.get_string(std::string("k"));
        D_DB_CHECK(s.has_value() && s.value() == "name:k");

        auto bin = rs.get_binary(std::string("z"));
        D_DB_CHECK(bin.has_value());
        D_DB_CHECK(bin.value().size() == 2);
        D_DB_CHECK(bin.value()[0] == 9);

        return true;
    }

    /*
    tests_result_set_convenience
      Verifies the row-materialization and close forwarding.
      Tests the following:
      - get_current_row forwards and returns the helper's row
      - get_all_rows forwards and returns the helper's rows
      - close forwards to the helper
    */
    bool
    tests_result_set_convenience()
    {
        rs_probe probe;
        mock_result_set mock(&probe);
        result_set<mock_result_set>& rs = mock;

        row current = rs.get_current_row();
        D_DB_CHECK(probe.get_current_row_calls == 1);
        D_DB_CHECK(current.count("k") == 1);
        D_DB_CHECK(std::get<re_std::int32_t>(current.at("k")) == k_rs_int_by_index);

        result_rows all = rs.get_all_rows();
        D_DB_CHECK(probe.get_all_rows_calls == 1);
        D_DB_CHECK(all.size() == 2);

        D_DB_CHECK(probe.closed == false);
        rs.close();
        D_DB_CHECK(probe.closed == true);

        return true;
    }

    /*
    tests_result_set_move
      Verifies the base's (defaulted) move operations leave a usable object
      that still forwards. result_set is non-copyable and move-only.
      Tests the following:
      - result_set is not copy-constructible or copy-assignable
      - a move-constructed helper still forwards through the base
      - a move-assigned helper still forwards through the base
    */
    bool
    tests_result_set_move()
    {
        static_assert(!std::is_copy_constructible<
                          result_set<mock_result_set>>::value,
                      "result_set must be non-copyable");
        static_assert(!std::is_copy_assignable<
                          result_set<mock_result_set>>::value,
                      "result_set must be non-copy-assignable");

        // move construction
        rs_probe probe;
        mock_result_set a(&probe);
        mock_result_set b(std::move(a));
        result_set<mock_result_set>& rb = b;
        D_DB_CHECK(rb.next() == true);
        D_DB_CHECK(probe.next_calls == 1);

        // move assignment (route the probe of a2 into b2)
        rs_probe probe1;
        rs_probe probe2;
        mock_result_set a2(&probe1);
        mock_result_set b2(&probe2);
        b2 = std::move(a2);
        result_set<mock_result_set>& rb2 = b2;
        D_DB_CHECK(rb2.first() == true);
        D_DB_CHECK(probe1.first_calls == 1);
        D_DB_CHECK(probe2.first_calls == 0);

        return true;
    }

}   // namespace testing
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
