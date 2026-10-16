/*******************************************************************************
* djinterp [core]                                      database_tests_config.cpp
*
*
* path:      /tests/djinterp/core/db/database_tests_config.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <chrono>
// djinterp
// tests for db/database.hpp section IV (connection_config / pool_config)
#include "database_tests.hpp"

NS_DJINTERP
namespace testing
{
    /*
    tests_connection_config_defaults
      Verifies every field initialized by connection_config's default
      constructor.
      Tests the following:
      - host/port and the three timeouts default as documented
      - the boolean flags and max_packet_size default as documented
      - the optional and string/map fields default empty/unset
    */
    bool
    tests_connection_config_defaults()
    {
        connection_config config;

        D_DB_CHECK(config.host == "localhost");
        D_DB_CHECK(config.port == 0);

        D_DB_CHECK(config.connect_timeout == std::chrono::seconds(30));
        D_DB_CHECK(config.read_timeout    == std::chrono::seconds(30));
        D_DB_CHECK(config.write_timeout   == std::chrono::seconds(30));

        D_DB_CHECK(config.auto_reconnect  == false);
        D_DB_CHECK(config.enable_ssl      == false);
        D_DB_CHECK(config.verify_ssl      == true);
        D_DB_CHECK(config.max_packet_size == static_cast<std::size_t>(16 * 1024 * 1024));

        // value-initialized (non-explicitly-set) members
        D_DB_CHECK(config.database.empty());
        D_DB_CHECK(config.username.empty());
        D_DB_CHECK(config.password.empty());

        D_DB_CHECK(!config.charset.has_value());
        D_DB_CHECK(!config.schema.has_value());
        D_DB_CHECK(!config.ssl_cert.has_value());
        D_DB_CHECK(!config.ssl_key.has_value());
        D_DB_CHECK(!config.ssl_ca.has_value());

        D_DB_CHECK(config.custom_options.empty());

        return true;
    }

    /*
    tests_connection_config_mutation
      Verifies fields can be assigned and read back, including the optionals
      and the custom-options map.
      Tests the following:
      - scalar/string fields round-trip assigned values
      - optional fields transition unset -> set and hold their payload
      - custom_options stores arbitrary key/value pairs
    */
    bool
    tests_connection_config_mutation()
    {
        connection_config config;

        config.host            = "db.example.com";
        config.port            = 5432;
        config.database        = "app";
        config.username        = "admin";
        config.password        = "secret";
        config.auto_reconnect  = true;
        config.enable_ssl      = true;
        config.verify_ssl      = false;
        config.max_packet_size = 4096;
        config.connect_timeout = std::chrono::milliseconds(500);

        D_DB_CHECK(config.host == "db.example.com");
        D_DB_CHECK(config.port == 5432);
        D_DB_CHECK(config.database == "app");
        D_DB_CHECK(config.username == "admin");
        D_DB_CHECK(config.password == "secret");
        D_DB_CHECK(config.auto_reconnect == true);
        D_DB_CHECK(config.enable_ssl == true);
        D_DB_CHECK(config.verify_ssl == false);
        D_DB_CHECK(config.max_packet_size == 4096);
        D_DB_CHECK(config.connect_timeout == std::chrono::milliseconds(500));

        config.charset = std::string("utf8mb4");
        config.schema  = std::string("public");
        D_DB_CHECK(config.charset.has_value());
        D_DB_CHECK(config.charset.value() == "utf8mb4");
        D_DB_CHECK(config.schema.value() == "public");

        config.custom_options["application_name"] = "djinterp";
        config.custom_options["sslmode"]          = "require";
        D_DB_CHECK(config.custom_options.size() == 2);
        D_DB_CHECK(config.custom_options.at("sslmode") == "require");

        return true;
    }

    /*
    tests_pool_config_defaults
      Verifies every field initialized by pool_config's default constructor.
      Tests the following:
      - the three sizing fields default as documented
      - the three timeout fields default as documented
      - validation flags default false and validation_query defaults empty
    */
    bool
    tests_pool_config_defaults()
    {
        pool_config config;

        D_DB_CHECK(config.min_connections       == 1);
        D_DB_CHECK(config.max_connections       == 10);
        D_DB_CHECK(config.connection_queue_size == 100);

        D_DB_CHECK(config.connection_timeout == std::chrono::seconds(30));
        D_DB_CHECK(config.idle_timeout       == std::chrono::minutes(10));
        D_DB_CHECK(config.max_lifetime       == std::chrono::hours(1));

        D_DB_CHECK(config.validate_on_acquire == false);
        D_DB_CHECK(config.validate_on_return  == false);

        D_DB_CHECK(config.validation_query.empty());

        return true;
    }

    /*
    tests_pool_config_mutation
      Verifies pool_config fields can be assigned and read back.
      Tests the following:
      - sizing and timeout fields round-trip assigned values
      - validation flags and validation_query round-trip assigned values
    */
    bool
    tests_pool_config_mutation()
    {
        pool_config config;

        config.min_connections       = 4;
        config.max_connections       = 32;
        config.connection_queue_size = 256;
        config.idle_timeout          = std::chrono::seconds(90);
        config.validate_on_acquire   = true;
        config.validate_on_return    = true;
        config.validation_query      = "SELECT 1";

        D_DB_CHECK(config.min_connections == 4);
        D_DB_CHECK(config.max_connections == 32);
        D_DB_CHECK(config.connection_queue_size == 256);
        D_DB_CHECK(config.idle_timeout == std::chrono::seconds(90));
        D_DB_CHECK(config.validate_on_acquire == true);
        D_DB_CHECK(config.validate_on_return == true);
        D_DB_CHECK(config.validation_query == "SELECT 1");

        return true;
    }

}   // namespace testing
NS_END  // djinterp
