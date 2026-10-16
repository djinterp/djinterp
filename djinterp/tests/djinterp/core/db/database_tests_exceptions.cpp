/*******************************************************************************
* djinterp [core]                                  database_tests_exceptions.cpp
*
*
* path:      /tests/djinterp/core/db/database_tests_exceptions.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <cstring>
#include <stdexcept>
// djinterp
// tests for db/database.hpp section III (exception hierarchy)
#include "database_tests.hpp"

NS_DJINTERP
namespace testing
{
    /*
    tests_exception_base
      Verifies the base exception's two constructors and its std::
      runtime_error lineage.
      Tests the following:
      - the std::string constructor preserves the message
      - the const char* constructor preserves the message
      - exception is-a std::runtime_error (catchable as such)
    */
    bool
    tests_exception_base()
    {
        exception from_string(std::string("string message"));
        D_DB_CHECK(std::strcmp(from_string.what(), "string message") == 0);

        exception from_cstr("cstr message");
        D_DB_CHECK(std::strcmp(from_cstr.what(), "cstr message") == 0);

        D_DB_CHECK((std::is_base_of<std::runtime_error, exception>::value));

        bool caught = false;

        try
        {
            throw exception("as runtime_error");
        }
        catch (const std::runtime_error& _e)
        {
            caught = true;
            D_DB_CHECK(std::strcmp(_e.what(), "as runtime_error") == 0);
        }

        D_DB_CHECK(caught);

        return true;
    }

    /*
    tests_exception_what_and_code
      Verifies what() and error_code() accessors on the base exception.
      Tests the following:
      - what() returns the stored message for both constructors
      - error_code() defaults to 0
      - what() is stable across repeated calls
    */
    bool
    tests_exception_what_and_code()
    {
        exception ex("payload");

        D_DB_CHECK(std::strcmp(ex.what(), "payload") == 0);
        D_DB_CHECK(ex.error_code() == 0);

        // stable across calls (same backing string)
        const char* first  = ex.what();
        const char* second = ex.what();
        D_DB_CHECK(std::strcmp(first, second) == 0);

        exception empty("");
        D_DB_CHECK(std::strcmp(empty.what(), "") == 0);
        D_DB_CHECK(empty.error_code() == 0);

        return true;
    }

    /*
    tests_connection_exception
      Verifies connection_exception inherits the base constructors and type.
      Tests the following:
      - constructs from a message and reports it via what()
      - is-a exception and is-a std::runtime_error
    */
    bool
    tests_connection_exception()
    {
        connection_exception ex("connect failed");

        D_DB_CHECK(std::strcmp(ex.what(), "connect failed") == 0);
        D_DB_CHECK(ex.error_code() == 0);
        D_DB_CHECK((std::is_base_of<exception, connection_exception>::value));

        bool caught = false;

        try
        {
            throw connection_exception("boom");
        }
        catch (const exception& _e)
        {
            caught = true;
            D_DB_CHECK(std::strcmp(_e.what(), "boom") == 0);
        }

        D_DB_CHECK(caught);

        return true;
    }

    /*
    tests_query_exception
      Verifies query_exception inherits the base constructors and type.
      Tests the following:
      - constructs from a message and reports it via what()
      - is-a exception
    */
    bool
    tests_query_exception()
    {
        query_exception ex("syntax error");

        D_DB_CHECK(std::strcmp(ex.what(), "syntax error") == 0);
        D_DB_CHECK((std::is_base_of<exception, query_exception>::value));

        bool caught = false;

        try
        {
            throw query_exception("bad sql");
        }
        catch (const exception& _e)
        {
            caught = true;
            D_DB_CHECK(std::strcmp(_e.what(), "bad sql") == 0);
        }

        D_DB_CHECK(caught);

        return true;
    }

    /*
    tests_transaction_exception
      Verifies transaction_exception inherits the base constructors and type.
      Tests the following:
      - constructs from a message and reports it via what()
      - is-a exception
    */
    bool
    tests_transaction_exception()
    {
        transaction_exception ex("txn aborted");

        D_DB_CHECK(std::strcmp(ex.what(), "txn aborted") == 0);
        D_DB_CHECK((std::is_base_of<exception, transaction_exception>::value));

        return true;
    }

    /*
    tests_type_conversion_exception
      Verifies type_conversion_exception inherits the base constructors/type.
      Tests the following:
      - constructs from a message and reports it via what()
      - is-a exception
    */
    bool
    tests_type_conversion_exception()
    {
        type_conversion_exception ex("cannot convert");

        D_DB_CHECK(std::strcmp(ex.what(), "cannot convert") == 0);
        D_DB_CHECK((std::is_base_of<exception,
                                    type_conversion_exception>::value));

        return true;
    }

    /*
    tests_exception_polymorphism
      Verifies each derived exception is catchable as the common base and
      that sibling types are not conflated.
      Tests the following:
      - every derived type is caught by a catch(const exception&)
      - a connection_exception is not caught by catch(const query_exception&)
    */
    bool
    tests_exception_polymorphism()
    {
        int caught_as_base = 0;

        try { throw connection_exception("a"); }
        catch (const exception&) { ++caught_as_base; }

        try { throw query_exception("b"); }
        catch (const exception&) { ++caught_as_base; }

        try { throw transaction_exception("c"); }
        catch (const exception&) { ++caught_as_base; }

        try { throw type_conversion_exception("d"); }
        catch (const exception&) { ++caught_as_base; }

        D_DB_CHECK(caught_as_base == 4);

        // sibling types must remain distinct
        bool wrong_sibling_caught = false;
        bool right_base_caught    = false;

        try
        {
            throw connection_exception("e");
        }
        catch (const query_exception&)
        {
            wrong_sibling_caught = true;
        }
        catch (const exception&)
        {
            right_base_caught = true;
        }

        D_DB_CHECK(!wrong_sibling_caught);
        D_DB_CHECK(right_base_caught);

        return true;
    }

}   // namespace testing
NS_END  // djinterp
