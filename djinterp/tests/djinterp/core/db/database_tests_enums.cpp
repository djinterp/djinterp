/*******************************************************************************
* djinterp [core]                                       database_tests_enums.cpp
*
*
* path:      /tests/djinterp/core/db/database_tests_enums.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.10.23
*******************************************************************************/
#include "../../../../inc/djinterp/env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// djinterp
// tests for db/database.hpp section II (enumerations + type aliases)
#include "database_tests.hpp"

NS_DJINTERP
namespace testing
{
    /*
    tests_database_type_enum
      Verifies database_type's fixed underlying type, its exact enumerator
      values, and that distinct vendors compare unequal.
      Tests the following:
      - underlying type is re_std::uint16_t
      - unknown is 0 and each vendor has its documented value
      - representative enumerators are mutually distinct
    */
    bool
    tests_database_type_enum()
    {
        D_DB_CHECK((std::is_same<std::underlying_type_t<database_type>,
                                 re_std::uint16_t>::value));

        D_DB_CHECK(static_cast<re_std::uint16_t>(database_type::unknown)    == 0x00);
        D_DB_CHECK(static_cast<re_std::uint16_t>(database_type::mariadb)    == 0x01);
        D_DB_CHECK(static_cast<re_std::uint16_t>(database_type::mysql)      == 0x02);
        D_DB_CHECK(static_cast<re_std::uint16_t>(database_type::postgresql) == 0x03);
        D_DB_CHECK(static_cast<re_std::uint16_t>(database_type::sqlite)     == 0x04);
        D_DB_CHECK(static_cast<re_std::uint16_t>(database_type::mongodb)    == 0x05);
        D_DB_CHECK(static_cast<re_std::uint16_t>(database_type::redis)      == 0x06);
        D_DB_CHECK(static_cast<re_std::uint16_t>(database_type::arangodb)   == 0x07);
        D_DB_CHECK(static_cast<re_std::uint16_t>(database_type::oracle)     == 0x08);
        D_DB_CHECK(static_cast<re_std::uint16_t>(database_type::mssql)      == 0x09);
        D_DB_CHECK(static_cast<re_std::uint16_t>(database_type::db2)        == 0x0A);
        D_DB_CHECK(static_cast<re_std::uint16_t>(database_type::firebase)   == 0x0B);
        D_DB_CHECK(static_cast<re_std::uint16_t>(database_type::cassandra)  == 0x0C);
        D_DB_CHECK(static_cast<re_std::uint16_t>(database_type::couchdb)    == 0x0D);
        D_DB_CHECK(static_cast<re_std::uint16_t>(database_type::neo4j)      == 0x0E);

        D_DB_CHECK(database_type::mysql != database_type::mariadb);
        D_DB_CHECK(database_type::unknown != database_type::sqlite);

        return true;
    }

    /*
    tests_isolation_level_enum
      Verifies the isolation_level enumerators are present and distinct.
      Tests the following:
      - all five levels are mutually distinct
      - default_level is separable from the four SQL-standard levels
    */
    bool
    tests_isolation_level_enum()
    {
        D_DB_CHECK(isolation_level::read_uncommitted != isolation_level::read_committed);
        D_DB_CHECK(isolation_level::read_committed   != isolation_level::repeatable_read);
        D_DB_CHECK(isolation_level::repeatable_read  != isolation_level::serializable);
        D_DB_CHECK(isolation_level::serializable     != isolation_level::default_level);
        D_DB_CHECK(isolation_level::read_uncommitted != isolation_level::default_level);

        isolation_level level = isolation_level::serializable;
        D_DB_CHECK(level == isolation_level::serializable);

        return true;
    }

    /*
    tests_connection_state_enum
      Verifies the connection_state lifecycle enumerators are distinct.
      Tests the following:
      - disconnected, connecting, connected, executing, error, closed differ
    */
    bool
    tests_connection_state_enum()
    {
        D_DB_CHECK(connection_state::disconnected != connection_state::connecting);
        D_DB_CHECK(connection_state::connecting   != connection_state::connected);
        D_DB_CHECK(connection_state::connected    != connection_state::executing);
        D_DB_CHECK(connection_state::executing    != connection_state::error);
        D_DB_CHECK(connection_state::error        != connection_state::closed);
        D_DB_CHECK(connection_state::disconnected != connection_state::closed);

        return true;
    }

    /*
    tests_field_type_enum
      Verifies the field_type enumerators are present and distinct.
      Tests the following:
      - the numeric, temporal, and structured field kinds differ
      - null and custom bracket the enumeration and are distinct
    */
    bool
    tests_field_type_enum()
    {
        D_DB_CHECK(field_type::null           != field_type::boolean);
        D_DB_CHECK(field_type::integer        != field_type::big_integer);
        D_DB_CHECK(field_type::floating_point != field_type::decimal);
        D_DB_CHECK(field_type::string         != field_type::binary);
        D_DB_CHECK(field_type::date           != field_type::time);
        D_DB_CHECK(field_type::datetime       != field_type::timestamp);
        D_DB_CHECK(field_type::json           != field_type::xml);
        D_DB_CHECK(field_type::uuid           != field_type::array);
        D_DB_CHECK(field_type::null           != field_type::custom);

        return true;
    }

    /*
    tests_store_direction_enum
      Verifies the render-fold store_direction enumerators are distinct.
      Tests the following:
      - store and retrieve are the two, mutually-distinct directions
    */
    bool
    tests_store_direction_enum()
    {
        D_DB_CHECK(store_direction::store != store_direction::retrieve);

        store_direction dir = store_direction::retrieve;
        D_DB_CHECK(dir == store_direction::retrieve);

        return true;
    }

    /*
    tests_value_variant
      Exercises the universal `value` variant across every alternative.
      Tests the following:
      - a default-constructed value holds std::monostate (index 0)
      - each alternative stores and retrieves its payload
      - the active index tracks the assigned alternative
    */
    bool
    tests_value_variant()
    {
        value v;
        D_DB_CHECK(v.index() == 0);
        D_DB_CHECK(std::holds_alternative<std::monostate>(v));

        v = true;
        D_DB_CHECK(std::holds_alternative<bool>(v));
        D_DB_CHECK(std::get<bool>(v) == true);

        v = static_cast<re_std::int32_t>(-7);
        D_DB_CHECK(std::holds_alternative<re_std::int32_t>(v));
        D_DB_CHECK(std::get<re_std::int32_t>(v) == -7);

        v = static_cast<re_std::int64_t>(9000000000LL);
        D_DB_CHECK(std::holds_alternative<re_std::int64_t>(v));
        D_DB_CHECK(std::get<re_std::int64_t>(v) == 9000000000LL);

        v = 3.14;
        D_DB_CHECK(std::holds_alternative<double>(v));
        D_DB_CHECK(std::get<double>(v) == 3.14);

        v = std::string("hello");
        D_DB_CHECK(std::holds_alternative<std::string>(v));
        D_DB_CHECK(std::get<std::string>(v) == "hello");

        v = std::vector<re_std::uint8_t>{ 0xDE, 0xAD, 0xBE, 0xEF };
        D_DB_CHECK(std::holds_alternative<std::vector<re_std::uint8_t>>(v));
        D_DB_CHECK(std::get<std::vector<re_std::uint8_t>>(v).size() == 4);
        D_DB_CHECK(std::get<std::vector<re_std::uint8_t>>(v)[0] == 0xDE);

        const auto now = std::chrono::system_clock::now();
        v = now;
        D_DB_CHECK(std::holds_alternative<
                       std::chrono::system_clock::time_point>(v));
        D_DB_CHECK(std::get<std::chrono::system_clock::time_point>(v) == now);

        return true;
    }

    /*
    tests_type_aliases
      Exercises the parameter_map, row, and result_rows aliases.
      Tests the following:
      - parameter_map and row are string-keyed value maps that insert/lookup
      - result_rows is a growable vector of rows preserving contents
    */
    bool
    tests_type_aliases()
    {
        parameter_map params;
        params["id"]   = static_cast<re_std::int32_t>(42);
        params["name"] = std::string("widget");

        D_DB_CHECK(params.size() == 2);
        D_DB_CHECK(std::get<re_std::int32_t>(params.at("id")) == 42);
        D_DB_CHECK(std::get<std::string>(params.at("name")) == "widget");

        row r;
        r["flag"] = true;
        D_DB_CHECK(std::get<bool>(r.at("flag")) == true);
        D_DB_CHECK(r.count("missing") == 0);

        result_rows rows;
        rows.push_back(r);
        rows.push_back(row{});
        D_DB_CHECK(rows.size() == 2);
        D_DB_CHECK(std::get<bool>(rows[0].at("flag")) == true);
        D_DB_CHECK(rows[1].empty());

        return true;
    }

}   // namespace testing
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
