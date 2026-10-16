/*******************************************************************************
* djinterp [core]                                      database_tests_vendor.cpp
*
*
* path:      /tests/djinterp/core/db/database_tests_vendor.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// tests for db/database.hpp section IV.b (vendor_info / vendor_traits)
#include "database_tests.hpp"

NS_DJINTERP
namespace testing
{
    /*
    tests_vendor_info_defaults
      Verifies vendor_info's default constructor and field assignability.
      Tests the following:
      - type defaults to unknown; the three flags default false; port 0
      - the string fields default empty
      - fields are assignable and read back
    */
    bool
    tests_vendor_info_defaults()
    {
        vendor_info info;

        D_DB_CHECK(info.type == database_type::unknown);
        D_DB_CHECK(info.default_port == 0);
        D_DB_CHECK(info.is_relational == false);
        D_DB_CHECK(info.is_embedded == false);
        D_DB_CHECK(info.supports_ssl == false);
        D_DB_CHECK(info.name.empty());
        D_DB_CHECK(info.display_name.empty());

        info.type          = database_type::postgresql;
        info.name          = "postgresql";
        info.display_name  = "PostgreSQL";
        info.default_port  = 5432;
        info.is_relational = true;
        info.supports_ssl  = true;

        D_DB_CHECK(info.type == database_type::postgresql);
        D_DB_CHECK(info.name == "postgresql");
        D_DB_CHECK(info.display_name == "PostgreSQL");
        D_DB_CHECK(info.default_port == 5432);
        D_DB_CHECK(info.is_relational == true);
        D_DB_CHECK(info.is_embedded == false);
        D_DB_CHECK(info.supports_ssl == true);

        return true;
    }

    /*
    tests_vendor_traits_primary_statics
      Verifies the unspecialized vendor_traits primary template's static
      metadata (the safe defaults for an unknown vendor).
      Tests the following:
      - native_handle_type is void* and db_type echoes the template argument
      - default_port is 0 and the name strings are the "unknown" defaults
      - the three capability flags are false
    */
    bool
    tests_vendor_traits_primary_statics()
    {
        using traits = vendor_traits<database_type::unknown>;

        D_DB_CHECK((std::is_same<traits::native_handle_type, void*>::value));

        D_DB_CHECK(traits::db_type == database_type::unknown);
        D_DB_CHECK(traits::default_port == 0);
        D_DB_CHECK(std::string(traits::name) == "unknown");
        D_DB_CHECK(std::string(traits::display_name) == "Unknown Database");
        D_DB_CHECK(traits::is_relational == false);
        D_DB_CHECK(traits::is_embedded == false);
        D_DB_CHECK(traits::supports_ssl == false);

        // db_type forwards the template argument for any (unspecialized) vendor
        D_DB_CHECK(vendor_traits<database_type::sqlite>::db_type
                       == database_type::sqlite);
        D_DB_CHECK(vendor_traits<database_type::mysql>::db_type
                       == database_type::mysql);

        return true;
    }

    /*
    tests_vendor_traits_get_info
      Verifies vendor_traits::get_info() populates a vendor_info from the
      static metadata.
      Tests the following:
      - the returned struct mirrors every static field
      - db_type (and hence info.type) follows the template argument
    */
    bool
    tests_vendor_traits_get_info()
    {
        vendor_info info = vendor_traits<database_type::unknown>::get_info();

        D_DB_CHECK(info.type == database_type::unknown);
        D_DB_CHECK(info.name == "unknown");
        D_DB_CHECK(info.display_name == "Unknown Database");
        D_DB_CHECK(info.default_port == 0);
        D_DB_CHECK(info.is_relational == false);
        D_DB_CHECK(info.is_embedded == false);
        D_DB_CHECK(info.supports_ssl == false);

        // the type field tracks the template argument through get_info()
        vendor_info sqlite_info =
            vendor_traits<database_type::sqlite>::get_info();
        D_DB_CHECK(sqlite_info.type == database_type::sqlite);
        D_DB_CHECK(sqlite_info.name == "unknown");   // still primary defaults

        return true;
    }

    /*
    tests_vendor_traits_make_default_config
      Verifies vendor_traits::make_default_config() yields a config with
      vendor-appropriate host/port defaults.
      Tests the following:
      - host is "localhost"
      - port equals the traits' default_port (0 for the primary template)
      - the remaining config fields keep connection_config's own defaults
    */
    bool
    tests_vendor_traits_make_default_config()
    {
        connection_config config =
            vendor_traits<database_type::unknown>::make_default_config();

        D_DB_CHECK(config.host == "localhost");
        D_DB_CHECK(config.port == vendor_traits<database_type::unknown>::default_port);
        D_DB_CHECK(config.port == 0);

        // untouched fields retain connection_config defaults
        D_DB_CHECK(config.verify_ssl == true);
        D_DB_CHECK(config.connect_timeout == std::chrono::seconds(30));

        return true;
    }

}   // namespace testing
NS_END  // djinterp
