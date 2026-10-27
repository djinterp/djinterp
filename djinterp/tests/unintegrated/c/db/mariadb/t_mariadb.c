/*******************************************************************************
* djinterp [c]                                                       t_mariadb.c
*
*   Conformance harness for djinterp's MariaDB/MySQL convenience layer: every
* function in c/db/mariadb/mariadb_*.h, its success paths, its documented
* failures and its argument checks, against a live server -- MariaDB from
* 10.3 or MySQL from 5.7 -- through whichever client it is linked with. Where
* the answer depends on the server, the check asks the server's flavor and
* version and expects the matching answer, so one harness passes on every
* server the layer supports.
*   Each d_mariadb_supports answer is also checked against the server itself,
* by running SQL that needs the feature: a version table that drifts from the
* truth fails here, not in a program.
*   The tests follow the headers, section by section. Each makes a database of
* its own, named after the run and the test, connects with it current, and
* drops it at the end, so a failure belongs to the function it names and no
* run sees another's objects.
*
*   Server: DJINTERP_TEST_MARIADB_HOST (default 127.0.0.1), _PORT (3306),
* _USER (root), _PASSWORD (none) and _SOCKET (none) say where; the user needs
* every privilege on databases named d_tests_mariadb_%. The harness fails,
* saying why, when it cannot connect.
*
*   Build (from the repo root), at any language level from C99, against
* Connector/C or libmysqlclient -- or let ci/check_db_mariadb.sh, which runs
* it across compilers, standards, clients and servers:
*     cc -std=c99 -Wall -Wextra -Werror -Iinc                                 \
*        src/djinterp/c/db/db_*.c src/djinterp/c/db/mariadb/mariadb_*.c       \
*        tests/unintegrated/c/db/mariadb/t_mariadb.c -lmariadb -o t_mariadb
*
*
* path:      /tests/unintegrated/c/db/mariadb/t_mariadb.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.26
*******************************************************************************/
// _WIN32_WINNT
//   knob (Windows'): Vista's API at least, for WSAPoll, where nothing asked
// for more; it precedes every include.
#if ( (defined(_WIN32)) &&                                                    \
      (!defined(_WIN32_WINNT)) )
    #define _WIN32_WINNT 0x0600
#endif  // _WIN32
// WIN32_LEAN_AND_MEAN, NOMINMAX
//   knobs (Windows'): <windows.h>, which <winsock2.h> and the clients' own
// headers include, without its rarer headers -- rpcndr.h among them, which
// makes `small` a macro -- and without its min and max macros.
#if defined(_WIN32)
    #if !defined(WIN32_LEAN_AND_MEAN)
        #define WIN32_LEAN_AND_MEAN 1
    #endif  // WIN32_LEAN_AND_MEAN
    #if !defined(NOMINMAX)
        #define NOMINMAX 1
    #endif  // NOMINMAX
#endif  // _WIN32
// D_CFG_ENV_USING_MARIADB
//   knob (env's): on, since the harness reads mysql.h itself -- the client's
// own codes, and its flavor -- as a program asking for env's detection does;
// the layer turns it on for its own sources alone.
#ifndef D_CFG_ENV_USING_MARIADB
    #define D_CFG_ENV_USING_MARIADB 1
#endif  // D_CFG_ENV_USING_MARIADB

// std
#include <limits.h>  // INT_MAX, UINT_MAX
#include <locale.h>  // setlocale, LC_NUMERIC
#include <stdio.h>   // printf, snprintf
#include <stdlib.h>  // free, getenv, malloc, strtoul
#include <string.h>  // memcmp, memcpy, memset, strcmp, strlen, strstr
#include <time.h>    // time
#if defined(_WIN32)
    #include <winsock2.h>      // WSAPoll, struct pollfd, SOCKET
    #if defined(_MSC_VER)
        #pragma comment(lib, "ws2_32.lib")  // WSAPoll
    #endif  // _MSC_VER
#else
    #include <arpa/inet.h>     // htonl, ntohs
    #include <netinet/in.h>    // struct sockaddr_in, INADDR_LOOPBACK
    #include <poll.h>          // poll, struct pollfd
    #include <sys/socket.h>    // socket, bind, listen, getsockname
    #include <unistd.h>        // close
#endif
// djinterp
#include "../../../../../inc/djinterp/c/db/mariadb/mariadb.h"  // unit under test
#include "../../../../../inc/djinterp/env/db/mariadb/env_mariadb.h"  // mysql.h
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // int64_t, INT64_MAX


//==============================================================================
// HARNESS
//==============================================================================

// g_checks, g_failures
//   counters: the checks run, and those that failed.
static int g_checks   = 0;
static int g_failures = 0;

// g_options
//   setting: where the server is, from the environment.
static struct d_mariadb_options g_options =
{
    "127.0.0.1", "root", NULL, NULL, 3306u, NULL
};

// g_run
//   setting: the prefix of every database the run makes, d_tests_mariadb_
// and the setup connection's id, which no other connection has at once.
static char g_run[40] = "";

// g_plain
//   setting: no TLS, which the connections the tests of calls that do not
// block make are given where the client cannot carry TLS so
// (d_tests_mariadb_without_blocking); g_plain_count is 1 while they run so,
// and 0 otherwise.
static const struct d_mariadb_setting g_plain =
{
    D_MARIADB_SETTING_SSL_MODE, (unsigned long)D_MARIADB_SSL_DISABLED, NULL,
    NULL
};
static size_t                         g_plain_count = 0u;

#if defined(_WIN32)

// d_tests_mariadb_socket
//   type: a socket, as the platform's poll takes it.
typedef SOCKET d_tests_mariadb_socket;

/*
d_tests_mariadb_poll
  WSAPoll, Windows' poll.
*/
static int
d_tests_mariadb_poll(
    struct pollfd* _fds,
    unsigned       _count,
    int            _timeout
)
{
    return WSAPoll(_fds, (ULONG)_count, _timeout);
}

#else

// d_tests_mariadb_socket
//   type: a socket, as the platform's poll takes it.
typedef int d_tests_mariadb_socket;

/*
d_tests_mariadb_poll
  poll(2).
*/
static int
d_tests_mariadb_poll(
    struct pollfd* _fds,
    unsigned       _count,
    int            _timeout
)
{
    return poll(_fds, (nfds_t)_count, _timeout);
}

#endif  // _WIN32

/*
d_tests_mariadb_expect
  Counts one check, and prints it when it fails.
*/
static void
d_tests_mariadb_expect(
    bool        _ok,
    const char* _what,
    int         _line
)
{
    ++g_checks;

    // report a failure with what was being checked, and where
    if (!_ok)
    {
        ++g_failures;
        printf("  FAIL line %d: %s\n", _line, _what);
    }

    return;
}

// D_TESTS_MARIADB_CHECK
//   macro: records one check with its source text and line, which only the
// preprocessor can supply.
#define D_TESTS_MARIADB_CHECK(_cond)                                          \
    d_tests_mariadb_expect((_cond), #_cond, __LINE__)

/*
d_tests_mariadb_expect_result
  Counts one check of a result, and prints the server's words when the result
is not the one expected.
*/
static void
d_tests_mariadb_expect_result(
    struct d_mariadb* _db,
    int               _result,
    int               _expected,
    const char*       _what,
    int               _line
)
{
    d_tests_mariadb_expect(_result == _expected, _what, _line);

    // the result, and what the connection says of it
    if (_result != _expected)
    {
        printf("    got %d, expected %d: %s\n",
               _result,
               _expected,
               (_result > 0) ? d_mariadb_error_message(_db)
                             : d_mariadb_result_message(_result));
    }

    return;
}

// D_TESTS_MARIADB_RESULT
//   macro: records a check that a call returned a given result, with the
// call's own text and line, which only the preprocessor can supply.
#define D_TESTS_MARIADB_RESULT(_db, _call, _expected)                         \
    d_tests_mariadb_expect_result((_db),                                       \
                                  (_call),                                     \
                                  (_expected),                                 \
                                  #_call " == " #_expected,                    \
                                  __LINE__)

// D_TESTS_MARIADB_OK
//   macro: records a check that a call succeeded; a macro, as
// D_TESTS_MARIADB_RESULT is, for the call's text and line.
#define D_TESTS_MARIADB_OK(_db, _call)                                        \
    D_TESTS_MARIADB_RESULT((_db), (_call), 0)

//==============================================================================
// FIXTURES
//==============================================================================

// d_tests_mariadb_feature_probe
//   struct: a feature, and SQL that runs only where the server has it; the
// table names are the test's own.
struct d_tests_mariadb_feature_probe
{
    enum d_mariadb_feature feature;  // the feature
    const char*            sql;      // SQL that needs it, as a script
};

// d_tests_mariadb_probes
//   table: SQL for the features whose syntax the server either takes or
// refuses -- Galera's by its variable, which a server built with it has --;
// the features a server parses and ignores are probed by what they do, in
// d_tests_mariadb_features. What a plugin or privileges decide -- roles, the
// X Plugin, Group Replication -- is the release's alone.
static const struct d_tests_mariadb_feature_probe
d_tests_mariadb_probes[] =
{
    { D_MARIADB_FEATURE_CTE,
      "WITH x AS (SELECT 1 AS a) SELECT a FROM x" },
    { D_MARIADB_FEATURE_WINDOW_FUNCTIONS,
      "SELECT ROW_NUMBER() OVER () FROM (SELECT 1 AS a) AS t" },
    { D_MARIADB_FEATURE_JSON,
      "CREATE TABLE p_json (a JSON)" },
    { D_MARIADB_FEATURE_RENAME_COLUMN,
      "CREATE TABLE p_rename (a INT); "
      "ALTER TABLE p_rename RENAME COLUMN a TO b" },
    { D_MARIADB_FEATURE_INVISIBLE_COLUMNS,
      "CREATE TABLE p_invisible (a INT, b INT INVISIBLE)" },
    { D_MARIADB_FEATURE_EXPRESSION_DEFAULTS,
      "CREATE TABLE p_default (a DOUBLE DEFAULT (RAND()))" },
    { D_MARIADB_FEATURE_RETURNING,
      "CREATE TABLE p_returning (a INT); "
      "INSERT INTO p_returning VALUES (1) RETURNING a" },
    { D_MARIADB_FEATURE_SEQUENCES,
      "CREATE SEQUENCE p_sequence" },
    { D_MARIADB_FEATURE_SYSTEM_VERSIONING,
      "CREATE TABLE p_versioned (a INT) WITH SYSTEM VERSIONING" },
    { D_MARIADB_FEATURE_INET6,
      "CREATE TABLE p_inet6 (a INET6)" },
    { D_MARIADB_FEATURE_UUID,
      "CREATE TABLE p_uuid (a UUID)" },
    { D_MARIADB_FEATURE_BINARY_JSON,
      "SELECT CAST('[1]' AS JSON)" },
    { D_MARIADB_FEATURE_LATERAL_DERIVED,
      "SELECT x.a FROM (SELECT 1 AS a) AS t, "
      "LATERAL (SELECT t.a AS a) AS x" },
    { D_MARIADB_FEATURE_GALERA,
      "SELECT @@wsrep_on" }
};

// d_tests_mariadb_every_type
//   table: one column of every d_db_type NONE excepted, which the servers
// refuse, the first the key.
static const struct d_db_column
d_tests_mariadb_every_type[] =
{
    { "id",       D_DB_TYPE_BIG_INTEGER,
      D_DB_COLUMN_PRIMARY_KEY | D_DB_COLUMN_AUTO_INCREMENT,
      NULL,       0u,  0u, NULL, 0u },
    { "flag",     D_DB_TYPE_BOOLEAN,        D_DB_COLUMN_NOT_NULL,
      "TRUE",     0u,  0u, NULL, 0u },
    { "count",    D_DB_TYPE_INTEGER,        0u, "0",    0u,  0u, NULL, 0u },
    { "ratio",    D_DB_TYPE_FLOATING_POINT, 0u, NULL,   0u,  0u, NULL, 0u },
    { "price",    D_DB_TYPE_DECIMAL,        0u, "1.50", 10u, 2u, NULL, 0u },
    { "plain",    D_DB_TYPE_DECIMAL,        0u, NULL,   0u,  0u, NULL, 0u },
    { "name",     D_DB_TYPE_STRING,
      D_DB_COLUMN_NOT_NULL | D_DB_COLUMN_UNIQUE,
      "'none'",   80u, 0u, NULL, 0u },
    { "note",     D_DB_TYPE_STRING,         0u, NULL,   0u,  0u, NULL, 0u },
    { "tag",      D_DB_TYPE_STRING,         D_DB_COLUMN_UNIQUE,
      NULL,       0u,  0u, NULL, 0u },
    { "blob",     D_DB_TYPE_BINARY,         0u, NULL,   0u,  0u, NULL, 0u },
    { "digest",   D_DB_TYPE_BINARY,         0u, NULL,   32u, 0u, NULL, 0u },
    { "born",     D_DB_TYPE_DATE,           0u, NULL,   0u,  0u, NULL, 0u },
    { "alarm",    D_DB_TYPE_TIME,           0u, NULL,   3u,  0u, NULL, 0u },
    { "seen",     D_DB_TYPE_DATETIME,       0u, NULL,   0u,  0u, NULL, 0u },
    { "stamp",    D_DB_TYPE_TIMESTAMP,      0u, NULL,   6u,  0u, NULL, 0u },
    { "doc",      D_DB_TYPE_JSON,           0u, NULL,   0u,  0u, NULL, 0u },
    { "page",     D_DB_TYPE_XML,            0u, NULL,   0u,  0u, NULL, 0u },
    { "uuid",     D_DB_TYPE_UUID,           0u, NULL,   0u,  0u, NULL, 0u },
    { "own",      D_DB_TYPE_NONE,           0u, NULL,   0u,  0u, "SMALLINT",
      0u }
};

// d_tests_mariadb_every_type_sql
//   table: each column of d_tests_mariadb_every_type as the servers report
// its type: MariaDB, MySQL before 8.0.19 (which wrote integers' display
// widths), and MySQL since.
static const char* const
d_tests_mariadb_every_type_sql[][3] =
{
    { "bigint(20)",    "bigint(20)",    "bigint"        },
    { "tinyint(1)",    "tinyint(1)",    "tinyint(1)"    },
    { "int(11)",       "int(11)",       "int"           },
    { "double",        "double",        "double"        },
    { "decimal(10,2)", "decimal(10,2)", "decimal(10,2)" },
    { "decimal(10,0)", "decimal(10,0)", "decimal(10,0)" },
    { "varchar(80)",   "varchar(80)",   "varchar(80)"   },
    { "longtext",      "longtext",      "longtext"      },
    { "varchar(255)",  "varchar(255)",  "varchar(255)"  },
    { "longblob",      "longblob",      "longblob"      },
    { "varbinary(32)", "varbinary(32)", "varbinary(32)" },
    { "date",          "date",          "date"          },
    { "time(3)",       "time(3)",       "time(3)"       },
    { "datetime",      "datetime",      "datetime"      },
    { "timestamp(6)",  "timestamp(6)",  "timestamp(6)"  },
    { "longtext",      "json",          "json"          },
    { "longtext",      "longtext",      "longtext"      },
    { "char(36)",      "char(36)",      "char(36)"      },
    { "smallint(6)",   "smallint(6)",   "smallint"      }
};

// d_tests_mariadb_composite
//   table: a two-column key, the second column listed first.
static const struct d_db_column
d_tests_mariadb_composite[] =
{
    { "region", D_DB_TYPE_STRING,  D_DB_COLUMN_PRIMARY_KEY, NULL, 8u, 0u,
      NULL, 0u },
    { "code",   D_DB_TYPE_INTEGER, D_DB_COLUMN_PRIMARY_KEY, NULL, 0u, 0u,
      NULL, 0u },
    { "label",  D_DB_TYPE_STRING,  0u,                      NULL, 0u, 0u,
      NULL, 0u }
};

// d_tests_mariadb_ordered
//   table: a composite key whose places order it b, a: otherwise than the
// table.
static const struct d_db_column
d_tests_mariadb_ordered[] =
{
    { "a", D_DB_TYPE_INTEGER, D_DB_COLUMN_PRIMARY_KEY, NULL, 0u, 0u, NULL, 2u },
    { "b", D_DB_TYPE_INTEGER, D_DB_COLUMN_PRIMARY_KEY, NULL, 0u, 0u, NULL, 1u },
    { "c", D_DB_TYPE_INTEGER, 0u,                      NULL, 0u, 0u, NULL, 0u }
};

// d_tests_mariadb_refused
//   table: column sets the layer refuses before the server sees them, each
// with its length: NONE without type_sql, a type outside the enumeration, an
// unknown flag, an empty name, AUTO_INCREMENT off the key, and on a
// composite key.
static const struct d_db_column
d_tests_mariadb_refused[][2] =
{
    { { "a", D_DB_TYPE_NONE,     0u,    NULL, 0u, 0u, NULL, 0u } },
    { { "a", (enum d_db_type)99, 0u,    NULL, 0u, 0u, NULL, 0u } },
    { { "a", D_DB_TYPE_INTEGER,  0x80u, NULL, 0u, 0u, NULL, 0u } },
    { { "",  D_DB_TYPE_INTEGER,  0u,    NULL, 0u, 0u, NULL, 0u } },
    { { "a", D_DB_TYPE_INTEGER,  D_DB_COLUMN_AUTO_INCREMENT, NULL, 0u, 0u,
        NULL, 0u } },
    { { "a", D_DB_TYPE_INTEGER,
        D_DB_COLUMN_PRIMARY_KEY | D_DB_COLUMN_AUTO_INCREMENT, NULL, 0u, 0u,
        NULL, 0u },
      { "b", D_DB_TYPE_INTEGER,  D_DB_COLUMN_PRIMARY_KEY, NULL, 0u, 0u,
        NULL, 0u } }
};

// d_tests_mariadb_defaults_sql
//   setting: a table whose columns have every kind of default, to read back.
static const char* const d_tests_mariadb_defaults_sql =
    "id INT PRIMARY KEY, a VARCHAR(10) DEFAULT 'abc', b INT DEFAULT 5, "
    "c DATETIME DEFAULT CURRENT_TIMESTAMP, d VARBINARY(4) DEFAULT 'xy', "
    "e DECIMAL(5,2) DEFAULT 1.50, f VARCHAR(10) DEFAULT NULL, "
    "g VARCHAR(10) NOT NULL, h ENUM('x','y') DEFAULT 'y', "
    "i BIT(3) DEFAULT b'101', j DATE DEFAULT '2020-01-02', "
    "l VARCHAR(10) DEFAULT 'it''s', n VARCHAR(10) DEFAULT 'NULL', "
    "o BINARY(2) DEFAULT 0x0102, q CHAR(3) DEFAULT '', "
    "r FLOAT DEFAULT -0.5, s VARCHAR(20) DEFAULT 'back\\\\slash'";

// d_tests_mariadb_default_reads
//   table: a column of d_tests_mariadb_defaults_sql, a query its default
// fills in for %s, and what the query must read: the default's own value,
// or, for bits and bytes, their number.
static const char* const
d_tests_mariadb_default_reads[][3] =
{
    { "a", "SELECT %s",       "abc"        },
    { "b", "SELECT %s",       "5"          },
    { "d", "SELECT %s",       "xy"         },
    { "e", "SELECT %s",       "1.50"       },
    { "h", "SELECT %s",       "y"          },
    { "i", "SELECT %s + 0",   "5"          },
    { "j", "SELECT %s",       "2020-01-02" },
    { "l", "SELECT %s",       "it's"       },
    { "n", "SELECT %s",       "NULL"       },
    { "o", "SELECT HEX(%s)",  "0102"       },
    { "q", "SELECT %s",       ""           },
    { "r", "SELECT %s",       "-0.5"       },
    { "s", "SELECT %s",       "back\\slash" }
};

/*
d_tests_mariadb_setting
  An environment variable, or a default when it is unset or empty.
*/
static const char*
d_tests_mariadb_setting(
    const char* _name,
    const char* _default
)
{
    const char* const value = getenv(_name);

    return ( (value) && (value[0] != '\0') ) ? value : _default;
}

/*
d_tests_mariadb_configure
  g_options from the environment.
*/
static void
d_tests_mariadb_configure(
    void
)
{
    g_options.host        = d_tests_mariadb_setting(
                                "DJINTERP_TEST_MARIADB_HOST", "127.0.0.1");
    g_options.user        = d_tests_mariadb_setting(
                                "DJINTERP_TEST_MARIADB_USER", "root");
    g_options.password    = d_tests_mariadb_setting(
                                "DJINTERP_TEST_MARIADB_PASSWORD", NULL);
    g_options.unix_socket = d_tests_mariadb_setting(
                                "DJINTERP_TEST_MARIADB_SOCKET", NULL);
    g_options.port        = (unsigned)strtoul(
        d_tests_mariadb_setting("DJINTERP_TEST_MARIADB_PORT", "3306"),
        NULL,
        10);

    return;
}

/*
d_tests_mariadb_connect
  A connection as the run's settings say, with a database current or none;
without TLS while g_plain_count says so.
*/
static struct d_mariadb*
d_tests_mariadb_connect(
    const char* _database
)
{
    struct d_mariadb*        db      = NULL;
    struct d_mariadb_options options = g_options;

    options.database = _database;

    // a connection, or a reason
    if (d_mariadb_connect_with(&options,
                               &g_plain,
                               g_plain_count,
                               &db,
                               NULL) != 0)
    {
        printf("  cannot connect to %s:%u as %s\n",
               options.host,
               options.port,
               options.user);
    }

    return db;
}

/*
d_tests_mariadb_native
  The client's own connection, not yet connected, as a program that adopts
one makes it: told, with Connector/C from 3.4 -- which otherwise insists on
TLS and checks the certificate -- what the layer's PREFERRED tells it, so
that it reaches the servers the layer's own connections reach.
*/
static MYSQL*
d_tests_mariadb_native(
    void
)
{
    MYSQL* const native = mysql_init(NULL);

#if defined(MARIADB_PACKAGE_VERSION_ID)
    #if (MARIADB_PACKAGE_VERSION_ID >= 30400)
        const my_bool on  = 1;
        const my_bool off = 0;

        // TLS where the server offers it, the certificate unchecked
        if (native)
        {
            (void)mysql_options(native, MYSQL_OPT_SSL_ENFORCE, &on);
            (void)mysql_options(native,
                                MYSQL_OPT_SSL_VERIFY_SERVER_CERT,
                                &off);
        }
    #endif  // MARIADB_PACKAGE_VERSION_ID >= 30400
#endif  // MARIADB_PACKAGE_VERSION_ID

    return native;
}

/*
d_tests_mariadb_schema_name
  The database a test works in: the run's prefix and the test's word.
*/
static void
d_tests_mariadb_schema_name(
    char*       _out,
    size_t      _size,
    const char* _word
)
{
    snprintf(_out, _size, "%s_%s", g_run, _word);

    return;
}

/*
d_tests_mariadb_begin
  A fresh database for one test, and a connection with it current.
*/
static struct d_mariadb*
d_tests_mariadb_begin(
    const char* _word,
    char*       _schema,
    size_t      _size
)
{
    struct d_mariadb* setup = d_tests_mariadb_connect(NULL);

    d_tests_mariadb_schema_name(_schema, _size, _word);

    // a database of the test's own, empty
    if (setup)
    {
        D_TESTS_MARIADB_OK(setup,
                           d_mariadb_drop_database(setup,
                                                   _schema,
                                                   D_DB_IF_EXISTS));
        D_TESTS_MARIADB_OK(setup,
                           d_mariadb_create_database(setup, _schema, 0u));
        d_mariadb_close(setup);
    }

    return d_tests_mariadb_connect(_schema);
}

/*
d_tests_mariadb_end
  Closes the test's connection and drops its database.
*/
static void
d_tests_mariadb_end(
    struct d_mariadb* _db,
    const char*       _schema
)
{
    struct d_mariadb* setup = NULL;

    d_mariadb_close(_db);
    setup = d_tests_mariadb_connect(NULL);

    // the database goes, whatever the test left in it
    if (setup)
    {
        D_TESTS_MARIADB_OK(setup,
                           d_mariadb_drop_database(setup,
                                                   _schema,
                                                   D_DB_IF_EXISTS));
        d_mariadb_close(setup);
    }

    return;
}

/*
d_tests_mariadb_int
  A single integer from SQL, or INT64_MIN when there is none.
*/
static int64_t
d_tests_mariadb_int(
    struct d_mariadb* _db,
    const char*       _sql
)
{
    int64_t value = INT64_MIN;

    // the value, or the sentinel
    if (d_mariadb_query_int64(_db, &value, _sql) != 0)
    {
        return INT64_MIN;
    }

    return value;
}

/*
d_tests_mariadb_text_is
  Whether a query's single value is the text expected, NULL for NULL.
*/
static bool
d_tests_mariadb_text_is(
    struct d_mariadb* _db,
    const char*       _sql,
    const char*       _expected
)
{
    char*      text   = NULL;
    const int  result = d_mariadb_query_text(_db, &text, _sql);
    const bool same   = (result == 0) &&
                        ( (text)
                              ? ( (_expected) &&
                                  (strcmp(text, _expected) == 0) )
                              : (!_expected) );

    // say what came instead
    if (!same)
    {
        printf("    %s: got %s, expected %s\n",
               _sql,
               (result != 0) ? "a failure" : (text) ? text : "NULL",
               (_expected) ? _expected : "NULL");
    }

    d_db_free(text);

    return same;
}

/*
d_tests_mariadb_status
  A session status's value, as SHOW SESSION STATUS words it, copied into
`_out`; "" when there is none.
*/
static void
d_tests_mariadb_status(
    struct d_mariadb* _db,
    const char*       _name,
    char*             _out,
    size_t            _size
)
{
    struct d_db_rows rows = { 0u, 0u, NULL, NULL };

    _out[0] = '\0';

    // the second column of the one row, when there is one
    if (d_mariadb_query_rowsf(_db,
                              &rows,
                              "SHOW SESSION STATUS LIKE %Q",
                              _name) == 0)
    {
        const struct d_db_value* const value = d_db_rows_value(&rows, 0u, 1u);

        if ( (value) &&
             (value->type == D_DB_VALUE_TEXT) )
        {
            (void)snprintf(_out, _size, "%s", value->as.text);
        }

        d_db_rows_free(&rows);
    }

    return;
}

/*
d_tests_mariadb_server_tls
  Whether the server offers TLS: have_ssl, where the server still has it, and
otherwise a MySQL that does, as every MySQL from 8.0 does by default.
*/
static bool
d_tests_mariadb_server_tls(
    struct d_mariadb* _db
)
{
    struct d_db_rows rows    = { 0u, 0u, NULL, NULL };
    bool             offered = true;

    // YES, or DISABLED and NO; no row where the variable is gone
    if (d_mariadb_query_rows(_db,
                             &rows,
                             "SHOW SESSION VARIABLES LIKE 'have_ssl'") == 0)
    {
        const struct d_db_value* const value = d_db_rows_value(&rows, 0u, 1u);

        if ( (value) &&
             (value->type == D_DB_VALUE_TEXT) )
        {
            offered = (strcmp(value->as.text, "YES") == 0);
        }

        d_db_rows_free(&rows);
    }

    return offered;
}

/*
d_tests_mariadb_has_name
  Whether a name list holds a name, exactly.
*/
static bool
d_tests_mariadb_has_name(
    const struct d_db_name_list* _names,
    const char*                  _name
)
{
    for (size_t i = 0u; i < _names->count; ++i)
    {
        if (strcmp(_names->names[i], _name) == 0)
        {
            return true;
        }
    }

    return false;
}

/*
d_tests_mariadb_sorted
  Whether a name list is in byte order, as the layer promises.
*/
static bool
d_tests_mariadb_sorted(
    const struct d_db_name_list* _names
)
{
    for (size_t i = 1u; i < _names->count; ++i)
    {
        if (strcmp(_names->names[i - 1u], _names->names[i]) >= 0)
        {
            return false;
        }
    }

    return (_names->names[_names->count] == NULL);
}

/*
d_tests_mariadb_find_column
  A column of a list by name, or NULL.
*/
static const struct d_db_column_info*
d_tests_mariadb_find_column(
    const struct d_db_column_list* _columns,
    const char*                    _name
)
{
    for (size_t i = 0u; i < _columns->count; ++i)
    {
        if (strcmp(_columns->columns[i].name, _name) == 0)
        {
            return &_columns->columns[i];
        }
    }

    return NULL;
}

/*
d_tests_mariadb_same_page
  Whether a page of rows read a few at a time holds the columns of the rows
read all at once, and `_count` of their rows from `_first`, value for value.
*/
static bool
d_tests_mariadb_same_page(
    const struct d_db_rows* _page,
    const struct d_db_rows* _all,
    size_t                  _first,
    size_t                  _count
)
{
    // the same columns, and as many rows as expected
    if ( (_page->column_count != _all->column_count) ||
         (_page->row_count != _count)                ||
         ((_first + _count) > _all->row_count) )
    {
        return false;
    }

    for (size_t c = 0u; c < _all->column_count; ++c)
    {
        // a column named or typed otherwise
        if ( (strcmp(_page->columns[c].name, _all->columns[c].name) != 0) ||
             (_page->columns[c].type != _all->columns[c].type)            ||
             (strcmp(_page->columns[c].type_name,
                     _all->columns[c].type_name) != 0) )
        {
            return false;
        }
    }

    // the same values, row by row
    for (size_t i = 0u; i < (_count * _all->column_count); ++i)
    {
        const struct d_db_value* const got  = &_page->values[i];
        const struct d_db_value* const want =
            &_all->values[(_first * _all->column_count) + i];
        bool                           same = ( (got->type == want->type) &&
                                                (got->size == want->size) );

        // the value itself, as its type holds it
        if (same)
        {
            switch (got->type)
            {
                case D_DB_VALUE_INTEGER:
                    same = (got->as.integer == want->as.integer);

                    break;

                case D_DB_VALUE_REAL:
                    same = (memcmp(&got->as.real,
                                   &want->as.real,
                                   sizeof(double)) == 0);

                    break;

                case D_DB_VALUE_TEXT:
                case D_DB_VALUE_BLOB:
                    same = (memcmp(got->as.blob, want->as.blob, got->size) ==
                            0);

                    break;

                default:
                    break;
            }
        }

        // the first difference ends the comparison
        if (!same)
        {
            return false;
        }
    }

    return true;
}

/*
d_tests_mariadb_is_mariadb
  Whether the connection reached MariaDB.
*/
static bool
d_tests_mariadb_is_mariadb(
    const struct d_mariadb* _db
)
{
    return (d_mariadb_server_flavor(_db) == D_MARIADB_FLAVOR_MARIADB);
}

/*
d_tests_mariadb_mysql_at_least
  Whether the connection reached MySQL at a version or later.
*/
static bool
d_tests_mariadb_mysql_at_least(
    const struct d_mariadb* _db,
    unsigned                _major,
    unsigned                _minor,
    unsigned                _patch
)
{
    return d_mariadb_server_at_least(_db,
                                     D_MARIADB_FLAVOR_MYSQL,
                                     _major,
                                     _minor,
                                     _patch);
}

/*
d_tests_mariadb_case_blind
  Whether the server matches table names without case: lower_case_table_names
other than 0, as on Windows and macOS.
*/
static bool
d_tests_mariadb_case_blind(
    struct d_mariadb* _db
)
{
    return (d_tests_mariadb_int(_db, "SELECT @@lower_case_table_names") != 0);
}

/*
d_tests_mariadb_works
  Whether a script runs, on a connection of its own in a database of its
own, so that a probe's failure touches nothing else.
*/
static bool
d_tests_mariadb_works(
    const char* _sql
)
{
    char              schema[96] = "";
    struct d_mariadb* db         = d_tests_mariadb_begin("probe",
                                                         schema,
                                                         sizeof(schema));
    const bool        works      = ( (db) &&
                                     (d_mariadb_exec_script(db, _sql) == 0) );

    d_tests_mariadb_end(db, schema);

    return works;
}

/*
d_tests_mariadb_client_has
  Whether the client the harness is built with has the call a feature of the
client's needs, read from its own header: Connector/C from 3.0 has the reset
and session tracking, and no query attributes; libmysqlclient has the reset
from 5.7.3, session tracking from 5.7.4 and query attributes from 8.0.23.
*/
static bool
d_tests_mariadb_client_has(
    enum d_mariadb_feature _feature
)
{
#if defined(MARIADB_PACKAGE_VERSION_ID)
    const unsigned long reset = (MARIADB_PACKAGE_VERSION_ID >= 30000) ? 1ul
                                                                       : 0ul;
    const unsigned long track = reset;
    const unsigned long tags  = 0ul;
#else
    const unsigned long reset = (MYSQL_VERSION_ID >= 50703) ? 1ul : 0ul;
    const unsigned long track = (MYSQL_VERSION_ID >= 50704) ? 1ul : 0ul;
    const unsigned long tags  = (MYSQL_VERSION_ID >= 80023) ? 1ul : 0ul;
#endif  // MARIADB_PACKAGE_VERSION_ID

    switch (_feature)
    {
        case D_MARIADB_FEATURE_RESET_CONNECTION:
            return (reset != 0ul);

        case D_MARIADB_FEATURE_SESSION_TRACKING:
            return (track != 0ul);

        case D_MARIADB_FEATURE_QUERY_ATTRIBUTES:
            return (tags != 0ul);

        default:
            break;
    }

    return true;
}

/*
d_tests_mariadb_tracked
  Whether the session's changes of a kind, as the layer reads them, are the
`_count` entries given, in their order; says what came instead when not.
*/
static bool
d_tests_mariadb_tracked(
    struct d_mariadb*        _db,
    enum d_mariadb_track     _kind,
    size_t                   _count,
    const char* const* const _expected
)
{
    struct d_db_name_list entries = { 0u, NULL };
    const int             result  = d_mariadb_session_track(_db,
                                                            _kind,
                                                            &entries);
    bool                  same    = ( (result == 0)              &&
                                      (entries.count == _count)  &&
                                      (entries.names)            &&
                                      (entries.names[_count] == NULL) );

    for (size_t i = 0u; (same) && (i < _count); ++i)
    {
        same = (strcmp(entries.names[i], _expected[i]) == 0);
    }

    // say what came instead
    if (!same)
    {
        printf("    kind %d:", (int)_kind);

        if (result != 0)
        {
            printf(" a failure, %d", result);
        }

        for (size_t i = 0u; (result == 0) && (i < entries.count); ++i)
        {
            printf(" <%s>", entries.names[i]);
        }

        printf(" (expected %lu)\n", (unsigned long)_count);
    }

    d_db_name_list_free(&entries);

    return same;
}

//==============================================================================
// db_common.h
//==============================================================================

/*
d_tests_mariadb_db_common
  db_common.h's vocabulary, as this layer meets it.
  Tests the following:
  - the status codes are 0 and distinct negatives, INVALID_ARGUMENT and
    MISMATCH among them, each with a message, and any other value without
    one
  - the release functions accept NULL and empty lists
*/
static void
d_tests_mariadb_db_common(
    void
)
{
    struct d_db_name_list   names   = { 0u, NULL };
    struct d_db_column_list columns = { 0u, NULL };

    printf("db_common\n");

    D_TESTS_MARIADB_CHECK(D_DB_OK == 0);
    D_TESTS_MARIADB_CHECK(D_DB_INVALID_ARGUMENT < 0);
    D_TESTS_MARIADB_CHECK(D_DB_INVALID_ARGUMENT != D_DB_UNSUPPORTED);
    D_TESTS_MARIADB_CHECK(D_DB_INVALID_ARGUMENT != D_DB_EXISTS);
    D_TESTS_MARIADB_CHECK(D_DB_INVALID_ARGUMENT != D_DB_NOT_FOUND);
    D_TESTS_MARIADB_CHECK(
        strcmp(d_db_status_message(D_DB_OK), "success") == 0);
    D_TESTS_MARIADB_CHECK(
        strcmp(d_db_status_message(D_DB_INVALID_ARGUMENT),
               "invalid argument") == 0);
    D_TESTS_MARIADB_CHECK(d_db_status_message(D_DB_UNSUPPORTED) != NULL);
    D_TESTS_MARIADB_CHECK(d_db_status_message(D_DB_EXISTS) != NULL);
    D_TESTS_MARIADB_CHECK(d_db_status_message(D_DB_NOT_FOUND) != NULL);
    D_TESTS_MARIADB_CHECK(D_DB_MISMATCH < 0);
    D_TESTS_MARIADB_CHECK(D_DB_MISMATCH != D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_CHECK(d_db_status_message(D_DB_MISMATCH) != NULL);
    D_TESTS_MARIADB_CHECK(d_db_status_message(D_DB_WAIT) != NULL);
    D_TESTS_MARIADB_CHECK(d_db_status_message(D_DB_WAIT - 1) == NULL);
    D_TESTS_MARIADB_CHECK(d_db_status_message(1146) == NULL);

    d_db_name_list_free(NULL);
    d_db_column_list_free(NULL);
    d_db_name_list_free(&names);
    d_db_column_list_free(&columns);
    d_db_free(NULL);
    D_TESTS_MARIADB_CHECK( (names.count == 0u) && (names.names == NULL) );
    D_TESTS_MARIADB_CHECK( (columns.count == 0u) &&
                           (columns.columns == NULL) );

    return;
}

//==============================================================================
// mariadb_common.h
//==============================================================================

/*
d_tests_mariadb_client
  The client library.
  Tests the following:
  - its version string and number agree with the library loaded
  - it is MariaDB's exactly when the header env found was
*/
static void
d_tests_mariadb_client(
    void
)
{
    printf("mariadb_common: client (%s, %s)\n",
           d_mariadb_client_version(),
           (d_mariadb_client_is_mariadb()) ? "MariaDB's" : "Oracle's");

    D_TESTS_MARIADB_CHECK(strlen(d_mariadb_client_version()) > 0u);
    D_TESTS_MARIADB_CHECK(
        strcmp(d_mariadb_client_version(), mysql_get_client_info()) == 0);
    D_TESTS_MARIADB_CHECK(
        d_mariadb_client_version_number() == mysql_get_client_version());
    D_TESTS_MARIADB_CHECK(d_mariadb_client_version_number() > 0ul);
    D_TESTS_MARIADB_CHECK(d_mariadb_client_is_mariadb() ==
                          (D_ENV_MYSQL_COMMON_IS_MARIADB != 0));

    return;
}

/*
d_tests_mariadb_versions
  The server's flavor and version.
  Tests the following:
  - a known flavor, at or above the layer's floor for it
  - the number agrees with the version string, past MariaDB's "5.5.5-"
  - NULL answers UNKNOWN, NULL and 0
*/
static void
d_tests_mariadb_versions(
    void
)
{
    struct d_mariadb* const db = d_tests_mariadb_connect(NULL);

    printf("mariadb_common: versions (%s)\n",
           (db) ? d_mariadb_server_version(db) : "no server");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_MARIADB_CHECK(db != NULL);

        return;
    }

    const enum d_mariadb_flavor flavor      = d_mariadb_server_flavor(db);
    const unsigned long         version     =
        d_mariadb_server_version_number(db);
    const char* const           text        = d_mariadb_server_version(db);
    char                        written[32] = "";

    D_TESTS_MARIADB_CHECK(flavor != D_MARIADB_FLAVOR_UNKNOWN);
    D_TESTS_MARIADB_CHECK(
        (flavor == D_MARIADB_FLAVOR_MARIADB)
            ? (version >= D_MARIADB_FLOOR_MARIADB_SERVER_ID)
            : (version >= D_MARIADB_FLOOR_MYSQL_SERVER_ID));
    D_TESTS_MARIADB_CHECK( (text) && (strlen(text) > 0u) );
    D_TESTS_MARIADB_CHECK(
        (flavor == D_MARIADB_FLAVOR_MARIADB) ==
        ( (text) && (strstr(text, "MariaDB") != NULL) ));

    // the number, written out, begins the string, or follows MariaDB's 5.5.5-
    snprintf(written,
             sizeof(written),
             "%lu.%lu.%lu",
             version / 10000ul,
             (version / 100ul) % 100ul,
             version % 100ul);
    D_TESTS_MARIADB_CHECK(
        (text) &&
        ( (strncmp(text, written, strlen(written)) == 0) ||
          (strncmp(text + 6, written, strlen(written)) == 0) ));

    D_TESTS_MARIADB_CHECK(d_mariadb_server_flavor(NULL) ==
                          D_MARIADB_FLAVOR_UNKNOWN);
    D_TESTS_MARIADB_CHECK(d_mariadb_server_version(NULL) == NULL);
    D_TESTS_MARIADB_CHECK(d_mariadb_server_version_number(NULL) == 0ul);

    d_mariadb_close(db);

    return;
}

/*
d_tests_mariadb_version_comparisons
  server_at_least.
  Tests the following:
  - each side of the exact version, part by part
  - parts no release has, past what an encoded number holds
  - the other flavor, UNKNOWN, and NULL answer false
*/
static void
d_tests_mariadb_version_comparisons(
    void
)
{
    struct d_mariadb* const     db      = d_tests_mariadb_connect(NULL);
    const enum d_mariadb_flavor flavor  = d_mariadb_server_flavor(db);
    const enum d_mariadb_flavor other   =
        (flavor == D_MARIADB_FLAVOR_MARIADB) ? D_MARIADB_FLAVOR_MYSQL
                                             : D_MARIADB_FLAVOR_MARIADB;
    const unsigned long         version = d_mariadb_server_version_number(db);
    const unsigned              major   = (unsigned)(version / 10000ul);
    const unsigned              minor   = (unsigned)((version / 100ul) % 100ul);
    const unsigned              patch   = (unsigned)(version % 100ul);

    printf("mariadb_common: version comparisons\n");

    D_TESTS_MARIADB_CHECK(
        d_mariadb_server_at_least(db, flavor, major, minor, patch));
    D_TESTS_MARIADB_CHECK(
        d_mariadb_server_at_least(db, flavor, major, minor, 0u));
    D_TESTS_MARIADB_CHECK(d_mariadb_server_at_least(db, flavor, 5u, 0u, 0u));
    D_TESTS_MARIADB_CHECK(
        !d_mariadb_server_at_least(db, flavor, major, minor, patch + 1u));
    D_TESTS_MARIADB_CHECK(
        !d_mariadb_server_at_least(db, flavor, major, minor + 1u, 0u));
    D_TESTS_MARIADB_CHECK(
        !d_mariadb_server_at_least(db, flavor, major + 1u, 0u, 0u));
    D_TESTS_MARIADB_CHECK(
        !d_mariadb_server_at_least(db, flavor, UINT_MAX, 0u, 0u));
    D_TESTS_MARIADB_CHECK(
        !d_mariadb_server_at_least(db, flavor, major, UINT_MAX, 0u));
    D_TESTS_MARIADB_CHECK(!d_mariadb_server_at_least(db, other, 5u, 0u, 0u));
    D_TESTS_MARIADB_CHECK(
        !d_mariadb_server_at_least(db, D_MARIADB_FLAVOR_UNKNOWN, 0u, 0u, 0u));
    D_TESTS_MARIADB_CHECK(
        !d_mariadb_server_at_least(NULL, flavor, 0u, 0u, 0u));

    d_mariadb_close(db);

    return;
}

/*
d_tests_mariadb_windows_build
  Whether the server was built for Windows, as @@version_compile_os says
("Win64"): MariaDB's Windows builds leave Galera out.
*/
static bool
d_tests_mariadb_windows_build(
    struct d_mariadb* _db
)
{
    char*      os      = NULL;
    const bool windows = (d_mariadb_query_text(
                              _db,
                              &os,
                              "SELECT @@version_compile_os") == 0) &&
                         (os) &&
                         (strncmp(os, "Win", 3u) == 0);

    d_db_free(os);

    return windows;
}

/*
d_tests_mariadb_features
  The feature table, against the server's syntax.
  Tests the following:
  - every feature whose syntax a server takes or refuses: d_mariadb_supports
    agrees with SQL that needs it -- Galera, which it answers by release,
    excepted on a server built for Windows, which has no wsrep_on
  - a value outside the enumeration, or a NULL handle, is no feature
*/
static void
d_tests_mariadb_features(
    void
)
{
    const size_t            count = sizeof(d_tests_mariadb_probes) /
                                    sizeof(d_tests_mariadb_probes[0]);
    struct d_mariadb* const db    = d_tests_mariadb_connect(NULL);

    printf("mariadb_common: features\n");

    for (size_t i = 0u; (db) && (i < count); ++i)
    {
        const struct d_tests_mariadb_feature_probe* const probe =
            &d_tests_mariadb_probes[i];
        const bool claimed = d_mariadb_supports(db, probe->feature);
        const bool works   = d_tests_mariadb_works(probe->sql);
        const bool unbuilt = (probe->feature == D_MARIADB_FEATURE_GALERA) &&
                             (claimed)                                    &&
                             (!works)                                     &&
                             (d_tests_mariadb_windows_build(db));

        // say which feature disagreed, beyond the line
        if ( (claimed != works) &&
             (!unbuilt) )
        {
            printf("  feature %d: supports says %d, SQL says %d (%s)\n",
                   (int)probe->feature,
                   (int)claimed,
                   (int)works,
                   probe->sql);
        }

        if (unbuilt)
        {
            printf("  Galera: in the release, left out of this Windows "
                   "build\n");
        }

        D_TESTS_MARIADB_CHECK( (claimed == works) ||
                               (unbuilt) );
    }

    D_TESTS_MARIADB_CHECK(
        !d_mariadb_supports(db, (enum d_mariadb_feature)999));
    D_TESTS_MARIADB_CHECK(
        !d_mariadb_supports(db, (enum d_mariadb_feature)-1));
    D_TESTS_MARIADB_CHECK(!d_mariadb_supports(NULL, D_MARIADB_FEATURE_CTE));

    d_mariadb_close(db);

    return;
}

/*
d_tests_mariadb_feature_behaviors
  The features a server may parse and ignore, by what they do.
  Tests the following:
  - CHECK constraints: supported exactly where the server refuses a row that
    breaks one
  - descending indexes: supported exactly where the server keeps an index
    descending
*/
static void
d_tests_mariadb_feature_behaviors(
    void
)
{
    char              schema[96] = "";
    struct d_mariadb* db         = d_tests_mariadb_begin("behaviors",
                                                         schema,
                                                         sizeof(schema));
    const bool        made       = (d_mariadb_exec(
        db,
        "CREATE TABLE f_check (a INT, CHECK (a > 0))") == 0);
    const bool        enforced   = ( (made) &&
                                     (d_mariadb_exec(
                                          db,
                                          "INSERT INTO f_check VALUES (0)") !=
                                      0) );

    printf("mariadb_common: features by behavior\n");

    D_TESTS_MARIADB_CHECK(made);
    D_TESTS_MARIADB_CHECK(
        d_mariadb_supports(db, D_MARIADB_FEATURE_CHECK_CONSTRAINTS) ==
        enforced);
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec(db,
                                      "CREATE TABLE f_desc "
                                      "(a INT, KEY k (a DESC))"));
    D_TESTS_MARIADB_CHECK(
        d_mariadb_supports(db, D_MARIADB_FEATURE_DESCENDING_INDEXES) ==
        (d_tests_mariadb_int(
             db,
             "SELECT COUNT(*) FROM information_schema.STATISTICS "
             "WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'f_desc' "
             "AND COLLATION = 'D'") == 1));

    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_messages
  Result messages.
  Tests the following:
  - the layer's own codes have db_common.h's messages, and an unknown
    negative value is "unknown result"
  - the three client numbers the layer returns itself have their own words
  - any other number points to d_mariadb_error_message
*/
static void
d_tests_mariadb_messages(
    void
)
{
    printf("mariadb_common: result messages\n");

    D_TESTS_MARIADB_CHECK(
        strcmp(d_mariadb_result_message(0), "success") == 0);
    D_TESTS_MARIADB_CHECK(
        strcmp(d_mariadb_result_message(D_DB_NOT_FOUND),
               d_db_status_message(D_DB_NOT_FOUND)) == 0);
    D_TESTS_MARIADB_CHECK(
        strcmp(d_mariadb_result_message(D_DB_INVALID_ARGUMENT),
               "invalid argument") == 0);
    D_TESTS_MARIADB_CHECK(
        strcmp(d_mariadb_result_message(-77), "unknown result") == 0);
    D_TESTS_MARIADB_CHECK(
        strcmp(d_mariadb_result_message(D_DB_MISMATCH),
               d_db_status_message(D_DB_MISMATCH)) == 0);
    D_TESTS_MARIADB_CHECK(
        strcmp(d_mariadb_result_message(2008), "out of memory") == 0);
    D_TESTS_MARIADB_CHECK(
        strstr(d_mariadb_result_message(2034), "no parameter"));
    D_TESTS_MARIADB_CHECK(
        strstr(d_mariadb_result_message(2000), "without an error number"));
    D_TESTS_MARIADB_CHECK(
        strstr(d_mariadb_result_message(1146), "d_mariadb_error_message"));

    return;
}

/*
d_tests_mariadb_errors
  The client's handle, and its record of the last error.
  Tests the following:
  - native hands back the client's MYSQL*, usable with the client's calls
  - a failed statement's number, words and SQLSTATE; a success clears them
  - a result of the layer's own leaves them as they were
  - NULL answers NULL, 0 and ""
*/
static void
d_tests_mariadb_errors(
    void
)
{
    struct d_mariadb* const db = d_tests_mariadb_connect(NULL);

    printf("mariadb_common: the client's handle and errors\n");

    // nothing more without a server
    if (!db)
    {
        D_TESTS_MARIADB_CHECK(db != NULL);

        return;
    }

    MYSQL* const native = (MYSQL*)d_mariadb_native(db);

    D_TESTS_MARIADB_CHECK(native != NULL);
    D_TESTS_MARIADB_CHECK( (native) && (mysql_ping(native) == 0) );

    const int failed = d_mariadb_exec(db, "SELECT no_column");

    D_TESTS_MARIADB_CHECK(failed == 1054);
    D_TESTS_MARIADB_CHECK(d_mariadb_error_code(db) == (unsigned)failed);
    D_TESTS_MARIADB_CHECK(strstr(d_mariadb_error_message(db), "no_column"));
    D_TESTS_MARIADB_CHECK(strcmp(d_mariadb_sqlstate(db), "42S22") == 0);

    // the layer's own refusal leaves the record alone
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_exec(db, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_CHECK(d_mariadb_error_code(db) != 0u);

    // a success clears it
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "SELECT 1"));
    D_TESTS_MARIADB_CHECK(d_mariadb_error_code(db) == 0u);
    D_TESTS_MARIADB_CHECK(strcmp(d_mariadb_error_message(db), "") == 0);
    D_TESTS_MARIADB_CHECK(strcmp(d_mariadb_sqlstate(db), "00000") == 0);

    D_TESTS_MARIADB_CHECK(d_mariadb_native(NULL) == NULL);
    D_TESTS_MARIADB_CHECK(d_mariadb_error_code(NULL) == 0u);
    D_TESTS_MARIADB_CHECK(strcmp(d_mariadb_error_message(NULL), "") == 0);
    D_TESTS_MARIADB_CHECK(strcmp(d_mariadb_sqlstate(NULL), "") == 0);

    d_mariadb_close(db);

    return;
}

//==============================================================================
// mariadb_connection.h
//==============================================================================

/*
d_tests_mariadb_connecting
  Connecting.
  Tests the following:
  - a connection with the run's settings, in the layer's character set
  - a wrong password, no server there and an unknown database each fail with
    the client's number, leaving NULL
  - a NULL handle to fill
*/
static void
d_tests_mariadb_connecting(
    void
)
{
    struct d_mariadb*        db      = NULL;
    struct d_mariadb_options options = g_options;
    char*                    charset = NULL;
    int                      result  = 0;

    printf("mariadb_connection: connecting\n");

    D_TESTS_MARIADB_OK(NULL, d_mariadb_connect(&g_options, &db));
    D_TESTS_MARIADB_CHECK(db != NULL);
    D_TESTS_MARIADB_OK(db, d_mariadb_get_charset(db, &charset));
    D_TESTS_MARIADB_CHECK( (charset) &&
                           (strcmp(charset, D_CFG_MARIADB_CHARSET) == 0) );
    d_db_free(charset);
    D_TESTS_MARIADB_CHECK(
        d_tests_mariadb_text_is(db,
                                "SELECT @@character_set_client",
                                D_CFG_MARIADB_CHARSET));
    D_TESTS_MARIADB_OK(db, d_mariadb_close(db));

    // a wrong password
    options.password = "certainly not the password";
    db               = (struct d_mariadb*)&options;
    result           = d_mariadb_connect(&options, &db);
    D_TESTS_MARIADB_CHECK( (result == 1045) || (result == 1698) );
    D_TESTS_MARIADB_CHECK(db == NULL);

    // no server there: nothing listens on port 1
    options             = g_options;
    options.host        = "127.0.0.1";
    options.port        = 1u;
    options.unix_socket = NULL;
    result              = d_mariadb_connect(&options, &db);
    D_TESTS_MARIADB_CHECK( (result == 2002) || (result == 2003) );
    D_TESTS_MARIADB_CHECK(db == NULL);

    // an unknown database
    options          = g_options;
    options.database = "d_tests_mariadb_no_such_database";
    D_TESTS_MARIADB_RESULT(NULL, d_mariadb_connect(&options, &db), 1049);
    D_TESTS_MARIADB_CHECK(db == NULL);
    D_TESTS_MARIADB_RESULT(NULL,
                           d_mariadb_connect(&g_options, NULL),
                           D_DB_INVALID_ARGUMENT);

    return;
}

/*
d_tests_mariadb_connecting_with
  Connecting with client settings.
  Tests the following:
  - no settings connects as d_mariadb_connect does
  - timeouts, compression, an init command, a connect attribute (where the
    server's performance_schema shows it) and a native option each take
  - a character set a setting names -- CHARSET, or the client's option as
    a native one -- replaces the layer's: the session's with every pairing,
    where the handshake loses it too, and after a reset
  - an init command that fails fails the connect with the server's number,
    and its reason in the server's words; a password the server refuses, in
    its words too; no reason where nothing failed
  - CLIENT_FOUND_ROWS makes an UPDATE count the rows it matched
  - each TLS mode, as the client and the server can have it: a mode that
    insists on TLS is refused with 2026 by a server without it, a reason
    given whenever the connect fails
  - settings of no name, without their text or value, past their range, the
    refused flag, and NULL arguments; every failure leaves NULL, and a
    refusal of the layer's own its db_common.h words
*/
static void
d_tests_mariadb_connecting_with(
    void
)
{
    struct d_mariadb*       db      = NULL;
    struct d_mariadb* const probe   = d_tests_mariadb_connect(NULL);
    const bool              tls     = ( (probe) &&
                                        (d_tests_mariadb_server_tls(probe)) );
    const bool              shown   =
        ( (probe) &&
          (d_tests_mariadb_int(probe, "SELECT @@performance_schema") == 1) );
    const bool              maria   = d_mariadb_client_is_mariadb();
    char                    text[128] = "";
    char*                   message = NULL;
    unsigned int            seconds = 7u;
    int                     result  = 0;

    printf("mariadb_connection: connecting with settings\n");

    d_mariadb_close(probe);

    // no settings at all, and no reason, since nothing failed
    message = (char*)&g_options;
    D_TESTS_MARIADB_OK(NULL,
                       d_mariadb_connect_with(&g_options, NULL, 0u, &db,
                                              &message));
    D_TESTS_MARIADB_CHECK(db != NULL);
    D_TESTS_MARIADB_CHECK(message == NULL);
    D_TESTS_MARIADB_CHECK(
        d_tests_mariadb_text_is(db,
                                "SELECT @@character_set_client",
                                D_CFG_MARIADB_CHARSET));
    d_mariadb_close(db);

    // settings that take, each seen from the session where it can be
    {
        const struct d_mariadb_setting settings[] =
        {
            { D_MARIADB_SETTING_CONNECT_TIMEOUT, 5ul, NULL, NULL },
            { D_MARIADB_SETTING_READ_TIMEOUT, 30ul, NULL, NULL },
            { D_MARIADB_SETTING_WRITE_TIMEOUT, 30ul, NULL, NULL },
            { D_MARIADB_SETTING_COMPRESS, 1ul, NULL, NULL },
            { D_MARIADB_SETTING_LOCAL_INFILE, 0ul, NULL, NULL },
            { D_MARIADB_SETTING_INIT_COMMAND, 0ul, "SET @d_init = 42", NULL },
            { D_MARIADB_SETTING_CONNECT_ATTRIBUTE, 0ul, "d_test", "yes" },
            { D_MARIADB_SETTING_NATIVE, (unsigned long)MYSQL_INIT_COMMAND,
              NULL, "SET @d_native = 7" },
            { D_MARIADB_SETTING_NATIVE,
              (unsigned long)MYSQL_OPT_CONNECT_TIMEOUT, NULL, &seconds }
        };

        D_TESTS_MARIADB_OK(NULL,
                           d_mariadb_connect_with(
                               &g_options,
                               settings,
                               sizeof(settings) / sizeof(settings[0]),
                               &db,
                               NULL));
        D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT @d_init") == 42);
        D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT @d_native") ==
                              7);
        d_tests_mariadb_status(db, "Compression", text, sizeof(text));
        D_TESTS_MARIADB_CHECK(strcmp(text, "ON") == 0);

        if (shown)
        {
            D_TESTS_MARIADB_CHECK(
                d_tests_mariadb_text_is(
                    db,
                    "SELECT ATTR_VALUE "
                    "FROM performance_schema.session_connect_attrs "
                    "WHERE PROCESSLIST_ID = CONNECTION_ID() "
                    "AND ATTR_NAME = 'd_test'",
                    "yes"));
        }

        d_mariadb_close(db);
    }

    // a character set a setting names, in place of the layer's -- the
    // layer's own setting, or the client's option as a native one: the
    // session's with every pairing, where the handshake loses it too, and
    // what a reset returns to
    {
        const struct d_mariadb_setting latin1[] =
        {
            { D_MARIADB_SETTING_CHARSET, 0ul, "latin1", NULL },
            { D_MARIADB_SETTING_NATIVE, (unsigned long)MYSQL_SET_CHARSET_NAME,
              NULL, "latin1" }
        };

        for (size_t i = 0u; i < (sizeof(latin1) / sizeof(latin1[0])); ++i)
        {
            D_TESTS_MARIADB_OK(NULL,
                               d_mariadb_connect_with(&g_options,
                                                      &latin1[i],
                                                      1u,
                                                      &db,
                                                      NULL));
            D_TESTS_MARIADB_CHECK(
                d_tests_mariadb_text_is(db,
                                        "SELECT @@character_set_client",
                                        "latin1"));
            D_TESTS_MARIADB_CHECK(
                d_tests_mariadb_text_is(db,
                                        "SELECT @@character_set_connection",
                                        "latin1"));

            if (d_mariadb_supports(db, D_MARIADB_FEATURE_RESET_CONNECTION))
            {
                D_TESTS_MARIADB_OK(db, d_mariadb_reset_connection(db));
                D_TESTS_MARIADB_CHECK(
                    d_tests_mariadb_text_is(db,
                                            "SELECT @@character_set_client",
                                            "latin1"));
            }

            d_mariadb_close(db);
        }
    }

    // an init command that fails
    {
        const struct d_mariadb_setting failing =
        {
            D_MARIADB_SETTING_INIT_COMMAND, 0ul,
            "SELECT * FROM mysql.d_tests_no_such_table", NULL
        };

        db = (struct d_mariadb*)&g_options;
        D_TESTS_MARIADB_RESULT(NULL,
                               d_mariadb_connect_with(&g_options, &failing, 1u,
                                                      &db, &message),
                               1146);
        D_TESTS_MARIADB_CHECK(db == NULL);

        // the server's words, which the closed connection no longer has
        D_TESTS_MARIADB_CHECK( (message != NULL) &&
                               (strstr(message, "d_tests_no_such_table") !=
                                NULL) );
        d_db_free(message);
    }

    // a password the server refuses, in its own words
    {
        struct d_mariadb_options refused = g_options;

        refused.password = "d_tests_not_the_password";
        db               = (struct d_mariadb*)&g_options;
        result           = d_mariadb_connect_with(&refused, NULL, 0u, &db,
                                                  &message);
        D_TESTS_MARIADB_CHECK( (result == 1045) || (result == 1698) );
        D_TESTS_MARIADB_CHECK(db == NULL);
        D_TESTS_MARIADB_CHECK( (message != NULL) &&
                               (strstr(message, "Access denied") != NULL) );
        d_db_free(message);
    }

    // rows matched, not changed, in a database of the test's own
    {
        const struct d_mariadb_setting found =
        {
            D_MARIADB_SETTING_CLIENT_FLAGS, (unsigned long)CLIENT_FOUND_ROWS,
            NULL, NULL
        };
        char                     schema[96] = "";
        struct d_mariadb*        setup      = NULL;
        struct d_mariadb_options options    = g_options;

        setup            = d_tests_mariadb_begin("found",
                                                 schema,
                                                 sizeof(schema));
        options.database = schema;
        D_TESTS_MARIADB_OK(NULL,
                           d_mariadb_connect_with(&options, &found, 1u, &db,
                                                  NULL));
        D_TESTS_MARIADB_OK(db,
                           d_mariadb_exec(db,
                                          "CREATE TEMPORARY TABLE f (v INT)"));
        D_TESTS_MARIADB_OK(db,
                           d_mariadb_exec(db, "INSERT INTO f VALUES (1), (1)"));
        D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "UPDATE f SET v = 1"));
        D_TESTS_MARIADB_CHECK(d_mariadb_changes(db) == 2);
        d_mariadb_close(db);
        d_tests_mariadb_end(setup, schema);
    }

    // the TLS modes
    for (unsigned long mode = (unsigned long)D_MARIADB_SSL_PREFERRED;
         mode <= (unsigned long)D_MARIADB_SSL_VERIFY_IDENTITY;
         ++mode)
    {
        const struct d_mariadb_setting setting =
        {
            D_MARIADB_SETTING_SSL_MODE, mode, NULL, NULL
        };

        db     = (struct d_mariadb*)&g_options;
        result = d_mariadb_connect_with(&g_options, &setting, 1u, &db,
                                        &message);

        switch (mode)
        {
            case D_MARIADB_SSL_PREFERRED:
                D_TESTS_MARIADB_RESULT(NULL, result, 0);
                break;

            case D_MARIADB_SSL_DISABLED:
                D_TESTS_MARIADB_RESULT(NULL, result, 0);
                d_tests_mariadb_status(db, "Ssl_cipher", text, sizeof(text));
                D_TESTS_MARIADB_CHECK(text[0] == '\0');
                break;

            case D_MARIADB_SSL_REQUIRED:
                D_TESTS_MARIADB_RESULT(NULL, result, (tls) ? 0 : 2026);

                if (tls)
                {
                    d_tests_mariadb_status(db,
                                           "Ssl_cipher",
                                           text,
                                           sizeof(text));
                    D_TESTS_MARIADB_CHECK(text[0] != '\0');
                }

                break;

            case D_MARIADB_SSL_VERIFY_CA:
                // no authority given, which the server's certificate needs
                D_TESTS_MARIADB_CHECK(
                    (maria) ? (result == D_DB_UNSUPPORTED) : (result != 0));
                break;

            default:
                // Connector/C from 3.4 may verify a certificate no authority
                // signed by the fingerprint a MariaDB server sends, its
                // zero-configuration TLS: then TLS, verified
                if ( (maria)                                 &&
                     (mysql_get_client_version() >= 30400ul) &&
                     (result == 0) )
                {
                    d_tests_mariadb_status(db,
                                           "Ssl_cipher",
                                           text,
                                           sizeof(text));
                    D_TESTS_MARIADB_CHECK(text[0] != '\0');
                }
                else
                {
                    D_TESTS_MARIADB_CHECK(result != 0);
                }

                break;
        }

        D_TESTS_MARIADB_CHECK( (result == 0) == (db != NULL) );
        D_TESTS_MARIADB_CHECK( (result == 0) == (message == NULL) );
        d_db_free(message);
        d_mariadb_close((result == 0) ? db : NULL);
    }

    // settings that cannot be right
    {
        const struct d_mariadb_setting wrong[] =
        {
            { (enum d_mariadb_setting_name)99, 0ul, NULL, NULL },
            { D_MARIADB_SETTING_INIT_COMMAND, 0ul, NULL, NULL },
            { D_MARIADB_SETTING_SSL_CA, 0ul, NULL, NULL },
            { D_MARIADB_SETTING_CONNECT_ATTRIBUTE, 0ul, "d_test", NULL },
            { D_MARIADB_SETTING_CONNECT_ATTRIBUTE, 0ul, NULL, "yes" },
            { D_MARIADB_SETTING_SSL_MODE, 5ul, NULL, NULL },
            { D_MARIADB_SETTING_CLIENT_FLAGS,
              (unsigned long)CLIENT_MULTI_STATEMENTS, NULL, NULL },
            { D_MARIADB_SETTING_CHARSET, 0ul, NULL, NULL },
            { D_MARIADB_SETTING_CHARSET, 0ul, "", NULL }
        };

        for (size_t i = 0u; i < (sizeof(wrong) / sizeof(wrong[0])); ++i)
        {
            db = (struct d_mariadb*)&g_options;
            D_TESTS_MARIADB_RESULT(NULL,
                                   d_mariadb_connect_with(&g_options,
                                                          &wrong[i], 1u, &db,
                                                          &message),
                                   D_DB_INVALID_ARGUMENT);
            D_TESTS_MARIADB_CHECK(db == NULL);

            // the layer's words for a refusal of its own
            D_TESTS_MARIADB_CHECK( (message != NULL) &&
                                   (strcmp(message, d_db_status_message(
                                               D_DB_INVALID_ARGUMENT)) == 0) );
            d_db_free(message);
        }
    }

#if (ULONG_MAX > UINT_MAX)
    {
        const struct d_mariadb_setting forever =
        {
            D_MARIADB_SETTING_READ_TIMEOUT, (unsigned long)UINT_MAX + 1ul,
            NULL, NULL
        };

        D_TESTS_MARIADB_RESULT(NULL,
                               d_mariadb_connect_with(&g_options, &forever, 1u,
                                                      &db, NULL),
                               D_DB_INVALID_ARGUMENT);
    }
#endif  // ULONG_MAX > UINT_MAX

    D_TESTS_MARIADB_RESULT(NULL,
                           d_mariadb_connect_with(&g_options, NULL, 1u, &db,
                                                  NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_CHECK(db == NULL);

    // no handle to write to: the reason is cleared all the same
    message = (char*)&g_options;
    D_TESTS_MARIADB_RESULT(NULL,
                           d_mariadb_connect_with(&g_options, NULL, 0u, NULL,
                                                  &message),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_CHECK(message == NULL);

    return;
}

/*
d_tests_mariadb_client_options
  The settings not every client has an option for.
  Tests the following:
  - the largest packet: a prepared statement's values past it, refused by
    libmysqlclient (2020) with the connection kept, sent by Connector/C; a
    value read past it loses the connection (2020, or 2013); 0 leaves the
    client's own, which reads it
  - an authentication plugin to answer with first, which the server's own
    then replaces
  - the TLS versions, where the server offers TLS: the one allowed is the
    one used; a list libmysqlclient cannot read, 2000; without TLS, a mode
    that insists on it is still 2026
  - TLS 1.3's cipher suites, with libmysqlclient from 8.0.16: the one allowed
    is the one used; D_DB_UNSUPPORTED with any other client
  - the clear-text password switch, taken by every client
  - none without its text
*/
static void
d_tests_mariadb_client_options(
    void
)
{
    struct d_mariadb*       db      = NULL;
    struct d_mariadb* const probe   = d_tests_mariadb_connect(NULL);
    const bool              tls     = ( (probe) &&
                                        (d_tests_mariadb_server_tls(probe)) );
    const bool              maria   = d_mariadb_client_is_mariadb();
    const bool              suites  = ( (!maria) &&
                                        (mysql_get_client_version() >=
                                         80016ul) );
    char                    text[128] = "";
    char*                   value   = NULL;
    int                     result  = 0;

    printf("mariadb_connection: options of the clients'\n");

    d_mariadb_close(probe);

    // the largest packet, past which the connection is lost
    {
        const struct d_mariadb_setting capped =
        {
            D_MARIADB_SETTING_MAX_PACKET, 1024ul * 1024ul, NULL, NULL
        };
        const struct d_mariadb_setting own =
        {
            D_MARIADB_SETTING_MAX_PACKET, 0ul, NULL, NULL
        };

        D_TESTS_MARIADB_OK(NULL,
                           d_mariadb_connect_with(&g_options, &capped, 1u, &db,
                                                  NULL));

        // a prepared statement's values past it: refused by libmysqlclient,
        // the connection kept; sent by Connector/C
        {
            struct d_mariadb_statement* statement = NULL;
            struct d_db_rows            rows      = { 0u, 0u, NULL, NULL };
            char* const                 big       = (char*)malloc(2097153u);

            D_TESTS_MARIADB_CHECK(big != NULL);

            if (big)
            {
                memset(big, 'z', 2097152u);
                big[2097152u] = '\0';
                D_TESTS_MARIADB_OK(db,
                                   d_mariadb_prepare(db,
                                                     &statement,
                                                     "SELECT LENGTH(?)"));
                D_TESTS_MARIADB_OK(db,
                                   d_mariadb_bind_text(statement, 1u, big));
                result = d_mariadb_execute_query(statement, &rows);
                D_TESTS_MARIADB_RESULT(db, result, (maria) ? 0 : 2020);

                if (result == 0)
                {
                    d_db_rows_free(&rows);
                }

                D_TESTS_MARIADB_OK(db, d_mariadb_finalize(statement));
                D_TESTS_MARIADB_OK(db, d_mariadb_ping(db));
                free(big);
            }
        }

        result = d_mariadb_query_text(db,
                                      &value,
                                      "SELECT REPEAT('x', 2097152)");
        D_TESTS_MARIADB_CHECK( (result == 2020) || (result == 2013) );
        D_TESTS_MARIADB_CHECK(d_mariadb_ping(db) != 0);
        d_mariadb_close(db);

        D_TESTS_MARIADB_OK(NULL,
                           d_mariadb_connect_with(&g_options, &own, 1u, &db,
                                                  NULL));
        D_TESTS_MARIADB_OK(db,
                           d_mariadb_query_text(db,
                                                &value,
                                                "SELECT REPEAT('x', 2097152)"));
        D_TESTS_MARIADB_CHECK( (value != NULL) &&
                               (strlen(value) == 2097152u) );
        d_db_free(value);
        value = NULL;
        d_mariadb_close(db);
    }

    // a plugin to answer with first, and the clear-text switch
    {
        const struct d_mariadb_setting settings[] =
        {
            { D_MARIADB_SETTING_DEFAULT_AUTH, 0ul, "mysql_native_password",
              NULL },
            { D_MARIADB_SETTING_CLEARTEXT_PLUGIN, 1ul, NULL, NULL }
        };

        D_TESTS_MARIADB_OK(NULL,
                           d_mariadb_connect_with(
                               &g_options,
                               settings,
                               sizeof(settings) / sizeof(settings[0]),
                               &db,
                               NULL));
        D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT 1") == 1);
        d_mariadb_close(db);
    }

    // the TLS versions
    {
        const struct d_mariadb_setting settings[] =
        {
            { D_MARIADB_SETTING_SSL_MODE, (unsigned long)D_MARIADB_SSL_REQUIRED,
              NULL, NULL },
            { D_MARIADB_SETTING_TLS_VERSION, 0ul, "TLSv1.2", NULL }
        };
        const struct d_mariadb_setting unreadable[] =
        {
            { D_MARIADB_SETTING_SSL_MODE, (unsigned long)D_MARIADB_SSL_REQUIRED,
              NULL, NULL },
            { D_MARIADB_SETTING_TLS_VERSION, 0ul, "TLSv9", NULL }
        };

        result = d_mariadb_connect_with(&g_options, settings, 2u, &db, NULL);
        D_TESTS_MARIADB_RESULT(NULL, result, (tls) ? 0 : 2026);

        if (result == 0)
        {
            d_tests_mariadb_status(db, "Ssl_version", text, sizeof(text));
            D_TESTS_MARIADB_CHECK(strcmp(text, "TLSv1.2") == 0);
            d_mariadb_close(db);
        }

        if (!maria)
        {
            D_TESTS_MARIADB_RESULT(NULL,
                                   d_mariadb_connect_with(&g_options,
                                                          unreadable, 2u, &db,
                                                          NULL),
                                   2000);
            D_TESTS_MARIADB_CHECK(db == NULL);
        }
    }

    // TLS 1.3's cipher suites
    {
        const struct d_mariadb_setting settings[] =
        {
            { D_MARIADB_SETTING_SSL_MODE, (unsigned long)D_MARIADB_SSL_REQUIRED,
              NULL, NULL },
            { D_MARIADB_SETTING_TLS_VERSION, 0ul, "TLSv1.3", NULL },
            { D_MARIADB_SETTING_TLS_CIPHERSUITES, 0ul,
              "TLS_AES_128_GCM_SHA256", NULL }
        };

        result = d_mariadb_connect_with(&g_options, settings, 3u, &db, NULL);

        if (!suites)
        {
            D_TESTS_MARIADB_RESULT(NULL, result, D_DB_UNSUPPORTED);
        }
        else
        {
            D_TESTS_MARIADB_RESULT(NULL, result, (tls) ? 0 : 2026);

            if (result == 0)
            {
                d_tests_mariadb_status(db, "Ssl_cipher", text, sizeof(text));
                D_TESTS_MARIADB_CHECK(strcmp(text,
                                             "TLS_AES_128_GCM_SHA256") == 0);
                d_mariadb_close(db);
            }
        }
    }

    // none without its text
    {
        const struct d_mariadb_setting wrong[] =
        {
            { D_MARIADB_SETTING_DEFAULT_AUTH, 0ul, NULL, NULL },
            { D_MARIADB_SETTING_TLS_VERSION, 0ul, NULL, NULL },
            { D_MARIADB_SETTING_TLS_CIPHERSUITES, 0ul, NULL, NULL }
        };

        for (size_t i = 0u; i < (sizeof(wrong) / sizeof(wrong[0])); ++i)
        {
            D_TESTS_MARIADB_RESULT(NULL,
                                   d_mariadb_connect_with(&g_options,
                                                          &wrong[i], 1u, &db,
                                                          NULL),
                                   D_DB_INVALID_ARGUMENT);
        }
    }

    return;
}

/*
d_tests_mariadb_options_read_back
  Options read back.
  Tests the following:
  - a setting the connect applied reads back as it was set: the connect
    timeout, an unsigned int; the init command, a text, with Connector/C
  - an option the client cannot read back, and a client before
    mysql_get_option, are D_DB_UNSUPPORTED
  - NULL arguments are D_DB_INVALID_ARGUMENT
*/
static void
d_tests_mariadb_options_read_back(
    void
)
{
    const struct d_mariadb_setting settings[] =
    {
        { D_MARIADB_SETTING_CONNECT_TIMEOUT, 7ul, NULL, NULL }
    };
    struct d_mariadb* db      = NULL;
    unsigned int      seconds = 0u;
    const bool        reads   = ( (!d_mariadb_client_is_mariadb()) ||
                                  (mysql_get_client_version() >= 30000ul) );

    printf("mariadb_connection: options read back\n");

    D_TESTS_MARIADB_RESULT(NULL,
                           d_mariadb_connect_with(&g_options, settings, 1u,
                                                  &db, NULL),
                           0);

    // nothing more without a connection
    if (!db)
    {
        return;
    }

    // a setting the connect applied, where the client reads options back
    if (reads)
    {
        D_TESTS_MARIADB_OK(db, d_mariadb_get_option(db,
                                                    MYSQL_OPT_CONNECT_TIMEOUT,
                                                    &seconds));
        D_TESTS_MARIADB_CHECK(seconds == 7u);
        D_TESTS_MARIADB_RESULT(db, d_mariadb_get_option(db, -1, &seconds),
                               D_DB_UNSUPPORTED);
    }
    else
    {
        D_TESTS_MARIADB_RESULT(db,
                               d_mariadb_get_option(db,
                                                    MYSQL_OPT_CONNECT_TIMEOUT,
                                                    &seconds),
                               D_DB_UNSUPPORTED);
    }

    // arguments
    D_TESTS_MARIADB_RESULT(NULL,
                           d_mariadb_get_option(NULL,
                                                MYSQL_OPT_CONNECT_TIMEOUT,
                                                &seconds),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_get_option(db,
                                                MYSQL_OPT_CONNECT_TIMEOUT,
                                                NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT 7") == 7);

    d_mariadb_close(db);

    return;
}

/*
d_tests_mariadb_server_public_key
  The server's RSA public key, for MySQL's caching_sha2_password without TLS.
  Tests the following:
  - against a server with caching_sha2_password, an account of its own, its
    cache cleared before each connect, without TLS: libmysqlclient, not
    told it may ask for the key, is refused (2061); Connector/C asks
  - told it may ask, each connects (libmysqlclient before 8.0.3 refuses the
    setting: D_DB_UNSUPPORTED)
  - given the key in a file, each connects without asking
  - Connector/C, which always asks, refuses being told not to
  - a server without caching_sha2_password -- MariaDB -- is skipped
*/
static void
d_tests_mariadb_server_public_key(
    void
)
{
    struct d_mariadb* const        db       = d_tests_mariadb_connect(NULL);
    const char* const              path     = "d_tests_mariadb_rsa.pem";
    const bool                     maria    = d_mariadb_client_is_mariadb();
    const bool                     can_ask  = ( (maria) ||
                                                (mysql_get_client_version() >=
                                                 80003ul) );
    char                           key[2048] = "";
    struct d_mariadb_options       options  = g_options;
    struct d_mariadb*              rsa      = NULL;
    const struct d_mariadb_setting plain[]  =
    {
        { D_MARIADB_SETTING_SSL_MODE, D_MARIADB_SSL_DISABLED, NULL, NULL }
    };
    const struct d_mariadb_setting asked[]  =
    {
        { D_MARIADB_SETTING_SSL_MODE, D_MARIADB_SSL_DISABLED, NULL, NULL },
        { D_MARIADB_SETTING_GET_SERVER_PUBLIC_KEY, 1ul, NULL, NULL }
    };
    const struct d_mariadb_setting filed[]  =
    {
        { D_MARIADB_SETTING_SSL_MODE, D_MARIADB_SSL_DISABLED, NULL, NULL },
        { D_MARIADB_SETTING_SERVER_PUBLIC_KEY, 0ul, path, NULL }
    };
    const struct d_mariadb_setting quiet[]  =
    {
        { D_MARIADB_SETTING_GET_SERVER_PUBLIC_KEY, 0ul, NULL, NULL }
    };

    printf("mariadb_connection: the server's public key\n");

    // nothing more without a connection
    if (!db)
    {
        return;
    }

    // MySQL's caching_sha2_password only
    if (d_tests_mariadb_int(db,
                            "SELECT COUNT(*) FROM information_schema.PLUGINS "
                            "WHERE PLUGIN_NAME = 'caching_sha2_password' "
                            "AND PLUGIN_STATUS = 'ACTIVE'") != 1)
    {
        printf("  skipped: the server has no caching_sha2_password\n");
        d_mariadb_close(db);

        return;
    }

    (void)d_mariadb_exec(db, "DROP USER IF EXISTS 'd_tests_rsa'@'%'");
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec(db,
                                      "CREATE USER 'd_tests_rsa'@'%' "
                                      "IDENTIFIED WITH caching_sha2_password "
                                      "BY 'rsa'"));
    d_tests_mariadb_status(db,
                           "Caching_sha2_password_rsa_public_key",
                           key,
                           sizeof(key));

    FILE* const file = fopen(path, "wb");

    D_TESTS_MARIADB_CHECK( (strstr(key, "BEGIN PUBLIC KEY") != NULL) &&
                           (file != NULL)                            &&
                           (fputs(key, file) >= 0) );

    if (file)
    {
        (void)fclose(file);
    }

    options.user     = "d_tests_rsa";
    options.password = "rsa";
    options.database = NULL;

    // without TLS, nothing said of the key
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "FLUSH PRIVILEGES"));
    D_TESTS_MARIADB_RESULT(NULL,
                           d_mariadb_connect_with(&options, plain, 1u, &rsa,
                                                  NULL),
                           (maria) ? 0 : 2061);
    d_mariadb_close(rsa);
    rsa = NULL;

    // told it may ask for the key
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "FLUSH PRIVILEGES"));
    D_TESTS_MARIADB_RESULT(NULL,
                           d_mariadb_connect_with(&options, asked, 2u, &rsa,
                                                  NULL),
                           (can_ask) ? 0 : D_DB_UNSUPPORTED);
    D_TESTS_MARIADB_CHECK( (!can_ask) ||
                           ( (rsa != NULL) &&
                             (d_tests_mariadb_int(rsa, "SELECT 7") == 7) ) );
    d_mariadb_close(rsa);
    rsa = NULL;

    // given it in a file
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "FLUSH PRIVILEGES"));
    D_TESTS_MARIADB_RESULT(NULL,
                           d_mariadb_connect_with(&options, filed, 2u, &rsa,
                                                  NULL),
                           0);
    D_TESTS_MARIADB_CHECK( (rsa != NULL) &&
                           (d_tests_mariadb_int(rsa, "SELECT 7") == 7) );
    d_mariadb_close(rsa);
    rsa = NULL;

    // told not to ask: Connector/C cannot keep from it
    D_TESTS_MARIADB_RESULT(NULL,
                           d_mariadb_connect_with(&options, quiet, 1u, &rsa,
                                                  NULL),
                           (maria) ? D_DB_UNSUPPORTED : 0);
    d_mariadb_close(rsa);

    (void)d_mariadb_exec(db, "DROP USER IF EXISTS 'd_tests_rsa'@'%'");
    (void)remove(path);
    d_mariadb_close(db);

    return;
}

/*
d_tests_mariadb_adopting
  Adopting and closing.
  Tests the following:
  - adopt wraps a client connection as it is, learning its server: unowned,
    it outlives the handle; owned, it goes with it
  - NULL connections and handles; close accepts NULL
*/
static void
d_tests_mariadb_adopting(
    void
)
{
    struct d_mariadb* db     = NULL;
    MYSQL* const      native = d_tests_mariadb_native();

    printf("mariadb_connection: adopting and closing\n");

    D_TESTS_MARIADB_CHECK(
        (native) &&
        (mysql_real_connect(native,
                            g_options.host,
                            g_options.user,
                            g_options.password,
                            NULL,
                            g_options.port,
                            g_options.unix_socket,
                            0ul) != NULL));
    D_TESTS_MARIADB_OK(NULL, d_mariadb_adopt(native, false, &db));
    D_TESTS_MARIADB_CHECK(d_mariadb_native(db) == (void*)native);
    D_TESTS_MARIADB_CHECK(d_mariadb_server_flavor(db) !=
                          D_MARIADB_FLAVOR_UNKNOWN);
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "SELECT 1"));
    D_TESTS_MARIADB_OK(db, d_mariadb_close(db));
    D_TESTS_MARIADB_CHECK( (native) && (mysql_ping(native) == 0) );
    D_TESTS_MARIADB_OK(NULL, d_mariadb_adopt(native, true, &db));
    D_TESTS_MARIADB_OK(db, d_mariadb_close(db));

    // NULL arguments
    db = (struct d_mariadb*)&g_options;
    D_TESTS_MARIADB_RESULT(NULL,
                           d_mariadb_adopt(NULL, true, &db),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_CHECK(db == NULL);
    D_TESTS_MARIADB_RESULT(NULL,
                           d_mariadb_adopt(&g_options, true, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_OK(NULL, d_mariadb_close(NULL));

    return;
}

/*
d_tests_mariadb_changing_user
  Changing user.
  Tests the following:
  - the user signs in again with a database current: the session is new, its
    variables and temporary tables gone, the character set kept
  - a statement prepared before can only be finalized after
  - a wrong password is 1045, after which a MariaDB server keeps the old
    session and a MySQL server has closed the connection
  - a NULL handle
*/
static void
d_tests_mariadb_changing_user(
    void
)
{
    char                        schema[96] = "";
    struct d_mariadb*           db         = NULL;
    struct d_mariadb_statement* statement  = NULL;
    int64_t                     changes    = 0;

    printf("mariadb_connection: changing user\n");

    db = d_tests_mariadb_begin("change", schema, sizeof(schema));

    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "SET @d_kept = 1"));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec(db, "CREATE TEMPORARY TABLE c (v INT)"));
    D_TESTS_MARIADB_OK(db, d_mariadb_prepare(db, &statement, "SELECT 1"));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_change_user(db,
                                             g_options.user,
                                             g_options.password,
                                             schema));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_text_is(db, "SELECT DATABASE()",
                                                  schema));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_text_is(db, "SELECT @d_kept", NULL));
    D_TESTS_MARIADB_CHECK(!d_mariadb_has_table(db, NULL, "c"));
    D_TESTS_MARIADB_CHECK(
        d_tests_mariadb_text_is(db,
                                "SELECT @@character_set_client",
                                D_CFG_MARIADB_CHARSET));
    D_TESTS_MARIADB_CHECK(d_mariadb_execute_update(statement, &changes) != 0);
    D_TESTS_MARIADB_OK(db, d_mariadb_finalize(statement));

    // into no database at all
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_change_user(db,
                                             g_options.user,
                                             g_options.password,
                                             NULL));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_text_is(db, "SELECT DATABASE()",
                                                  NULL));

    // a wrong password
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_change_user(db,
                                                 g_options.user,
                                                 "certainly not the password",
                                                 NULL),
                           1045);
    D_TESTS_MARIADB_CHECK( (d_tests_mariadb_is_mariadb(db))
                               ? (d_mariadb_ping(db) == 0)
                               : (d_mariadb_ping(db) != 0) );

    D_TESTS_MARIADB_RESULT(NULL,
                           d_mariadb_change_user(NULL, "u", "p", NULL),
                           D_DB_INVALID_ARGUMENT);

    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_resetting
  Resetting the session.
  Tests the following:
  - the reset is supported exactly where the server and the client have it
  - where it is, the session is fresh: its transaction rolled back, its user
    variables, temporary tables and session variables gone; the database is
    kept, and the character set is the client's, the one it began with or
    another set since
  - a statement prepared before can only be finalized after
  - the init commands the connect ran run again, in their order
  - where it is not, D_DB_UNSUPPORTED, the session as it was
  - a NULL handle
*/
static void
d_tests_mariadb_resetting(
    void
)
{
    char                        schema[96] = "";
    struct d_mariadb*           db         = NULL;
    struct d_mariadb_statement* statement  = NULL;
    int64_t                     changes    = 0;
    bool                        can        = false;

    printf("mariadb_connection: resetting the session\n");

    db  = d_tests_mariadb_begin("reset", schema, sizeof(schema));
    can = d_mariadb_supports(db, D_MARIADB_FEATURE_RESET_CONNECTION);

    D_TESTS_MARIADB_CHECK(
        can ==
        ( (d_tests_mariadb_client_has(D_MARIADB_FEATURE_RESET_CONNECTION)) &&
          ( (d_mariadb_server_at_least(db,
                                       D_MARIADB_FLAVOR_MARIADB,
                                       10u, 2u, 4u)) ||
            (d_tests_mariadb_mysql_at_least(db, 5u, 7u, 3u)) ) ));

    // a session with something of each kind to lose
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec(db,
                                      "CREATE TABLE r_rows (v INT) "
                                      "ENGINE = InnoDB"));
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "SET @d_reset = 1"));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec(db, "SET SESSION time_zone = '+03:00'"));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec(db,
                                      "CREATE TEMPORARY TABLE r_temp (v INT)"));
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "START TRANSACTION"));
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "INSERT INTO r_rows VALUES (1)"));
    D_TESTS_MARIADB_OK(db, d_mariadb_prepare(db, &statement, "SELECT 1"));

    if (can)
    {
        D_TESTS_MARIADB_OK(db, d_mariadb_reset_connection(db));
        D_TESTS_MARIADB_CHECK(!d_mariadb_in_transaction(db));
        D_TESTS_MARIADB_CHECK(
            d_tests_mariadb_int(db, "SELECT COUNT(*) FROM r_rows") == 0);
        D_TESTS_MARIADB_CHECK(
            d_tests_mariadb_text_is(db, "SELECT @d_reset", NULL));
        D_TESTS_MARIADB_CHECK(
            d_tests_mariadb_int(db,
                                "SELECT @@session.time_zone = "
                                "@@global.time_zone") == 1);
        D_TESTS_MARIADB_RESULT(db,
                               d_mariadb_exec(db, "SELECT v FROM r_temp"),
                               1146);
        D_TESTS_MARIADB_CHECK(
            d_tests_mariadb_text_is(db, "SELECT DATABASE()", schema));
        D_TESTS_MARIADB_CHECK(
            d_tests_mariadb_text_is(db,
                                    "SELECT @@character_set_client",
                                    D_CFG_MARIADB_CHARSET));
        D_TESTS_MARIADB_CHECK(
            d_tests_mariadb_text_is(db,
                                    "SELECT @@character_set_results",
                                    D_CFG_MARIADB_CHARSET));
        D_TESTS_MARIADB_CHECK(
            d_mariadb_execute_update(statement, &changes) != 0);

        // a character set set since the session began
        D_TESTS_MARIADB_OK(db, d_mariadb_set_charset(db, "latin1"));
        D_TESTS_MARIADB_OK(db, d_mariadb_reset_connection(db));
        D_TESTS_MARIADB_CHECK(
            d_tests_mariadb_text_is(db,
                                    "SELECT @@character_set_client",
                                    "latin1"));
        D_TESTS_MARIADB_CHECK(
            d_tests_mariadb_text_is(db,
                                    "SELECT @@character_set_connection",
                                    "latin1"));

        // the connect's init commands, run again in their order
        {
            struct d_mariadb*              init        = NULL;
            struct d_mariadb_options       options     = g_options;
            const struct d_mariadb_setting settings[2] =
            {
                { D_MARIADB_SETTING_INIT_COMMAND, 0ul,
                  "SET @d_first = 42", NULL },
                { D_MARIADB_SETTING_INIT_COMMAND, 0ul,
                  "SET @d_second = @d_first + 1", NULL }
            };

            options.database = schema;

            D_TESTS_MARIADB_OK(NULL,
                               d_mariadb_connect_with(&options,
                                                      settings,
                                                      2u,
                                                      &init,
                                                      NULL));
            D_TESTS_MARIADB_CHECK(
                d_tests_mariadb_int(init, "SELECT @d_second") == 43);
            D_TESTS_MARIADB_OK(init,
                               d_mariadb_exec(init,
                                              "SET @d_first = 7, "
                                              "@d_second = 8"));
            D_TESTS_MARIADB_OK(init, d_mariadb_reset_connection(init));
            D_TESTS_MARIADB_CHECK(
                d_tests_mariadb_int(init, "SELECT @d_second") == 43);
            d_mariadb_close(init);
        }
    }
    else
    {
        D_TESTS_MARIADB_RESULT(db,
                               d_mariadb_reset_connection(db),
                               D_DB_UNSUPPORTED);
        D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT @d_reset") == 1);
        D_TESTS_MARIADB_CHECK(d_mariadb_in_transaction(db));
        D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "ROLLBACK"));
    }

    D_TESTS_MARIADB_OK(db, d_mariadb_finalize(statement));
    D_TESTS_MARIADB_RESULT(NULL,
                           d_mariadb_reset_connection(NULL),
                           D_DB_INVALID_ARGUMENT);

    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_liveness
  Ping, the connection's id and the server's status.
  Tests the following:
  - a live connection pings; one the server has killed does not
  - the id is CONNECTION_ID()'s
  - the status is mysql_stat's summary
  - NULL arguments
*/
static void
d_tests_mariadb_liveness(
    void
)
{
    struct d_mariadb* const db    = d_tests_mariadb_connect(NULL);
    struct d_mariadb* const other = d_tests_mariadb_connect(NULL);

    printf("mariadb_connection: liveness\n");

    D_TESTS_MARIADB_OK(db, d_mariadb_ping(db));
    D_TESTS_MARIADB_CHECK(
        (int64_t)d_mariadb_connection_id(db) ==
        d_tests_mariadb_int(db, "SELECT CONNECTION_ID()"));
    D_TESTS_MARIADB_CHECK(d_mariadb_connection_id(db) !=
                          d_mariadb_connection_id(other));

    // the server's summary of itself
    {
        char* status = NULL;

        D_TESTS_MARIADB_OK(db, d_mariadb_server_status(db, &status));
        D_TESTS_MARIADB_CHECK( (status) &&
                               (strncmp(status, "Uptime: ", 8u) == 0) &&
                               (strstr(status, "Threads: ") != NULL) );
        d_db_free(status);
        status = (char*)&g_options;
        D_TESTS_MARIADB_RESULT(NULL,
                               d_mariadb_server_status(NULL, &status),
                               D_DB_INVALID_ARGUMENT);
        D_TESTS_MARIADB_CHECK(status == (char*)&g_options);
        D_TESTS_MARIADB_RESULT(db,
                               d_mariadb_server_status(db, NULL),
                               D_DB_INVALID_ARGUMENT);
    }

    // killed from another connection
    D_TESTS_MARIADB_OK(other,
                       d_mariadb_execf(other,
                                       "KILL %lu",
                                       d_mariadb_connection_id(db)));
    D_TESTS_MARIADB_CHECK(d_mariadb_ping(db) > 0);

    {
        char* status = (char*)&g_options;

        D_TESTS_MARIADB_CHECK(d_mariadb_server_status(db, &status) > 0);
        D_TESTS_MARIADB_CHECK(status == (char*)&g_options);
    }

    D_TESTS_MARIADB_RESULT(NULL, d_mariadb_ping(NULL), D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_CHECK(d_mariadb_connection_id(NULL) == 0ul);

    d_mariadb_close(db);
    d_mariadb_close(other);

    return;
}

/*
d_tests_mariadb_rows_changed
  The last statement's insert id, row count and warnings.
  Tests the following:
  - a multi-row INSERT reports its first id and its row count
  - an UPDATE counts the rows it changed, not those it matched
  - a statement that returns rows counts them; one that fails is -1
  - a warning is counted
  - NULL answers 0
*/
static void
d_tests_mariadb_rows_changed(
    void
)
{
    char              schema[96] = "";
    struct d_mariadb* db         = d_tests_mariadb_begin("changes",
                                                         schema,
                                                         sizeof(schema));

    printf("mariadb_connection: rows changed\n");

    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec(db,
                                      "CREATE TABLE r (id INT AUTO_INCREMENT "
                                      "PRIMARY KEY, v INT)"));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec(db,
                                      "INSERT INTO r (v) "
                                      "VALUES (1), (2), (3)"));
    D_TESTS_MARIADB_CHECK(d_mariadb_last_insert_id(db) == 1u);
    D_TESTS_MARIADB_CHECK(d_mariadb_changes(db) == 3);
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec(db, "UPDATE r SET v = 2 WHERE v >= 2"));
    D_TESTS_MARIADB_CHECK(d_mariadb_changes(db) == 1);
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "INSERT INTO r (v) VALUES (9)"));
    D_TESTS_MARIADB_CHECK(d_mariadb_last_insert_id(db) == 4u);
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "SELECT * FROM r"));
    D_TESTS_MARIADB_CHECK(d_mariadb_changes(db) == 4);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_exec(db, "SELECT * FROM none"), 1146);
    D_TESTS_MARIADB_CHECK(d_mariadb_changes(db) == -1);
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "SELECT 1 / 0"));
    D_TESTS_MARIADB_CHECK(d_mariadb_warning_count(db) == 1u);

    D_TESTS_MARIADB_CHECK(d_mariadb_last_insert_id(NULL) == 0u);
    D_TESTS_MARIADB_CHECK(d_mariadb_changes(NULL) == 0);
    D_TESTS_MARIADB_CHECK(d_mariadb_warning_count(NULL) == 0u);

    d_tests_mariadb_end(db, schema);

    return;
}

//==============================================================================
// mariadb_query.h
//==============================================================================

/*
d_tests_mariadb_statements
  Running statements.
  Tests the following:
  - exec runs one statement and refuses two, with the server's syntax error
  - rows a statement returns are read, so the next statement runs
  - every result of a CALL is read
  - execf quotes values and names
*/
static void
d_tests_mariadb_statements(
    void
)
{
    char              schema[96] = "";
    struct d_mariadb* db         = d_tests_mariadb_begin("statements",
                                                         schema,
                                                         sizeof(schema));

    printf("mariadb_query: statements\n");

    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "CREATE TABLE s (a INT)"));
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_exec(db, "SELECT 1; SELECT 2"),
                           1064);
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "SELECT * FROM s"));
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "INSERT INTO s VALUES (1)"));

    // a procedure answering twice, and the status after
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec(db,
                                      "CREATE PROCEDURE twice() "
                                      "BEGIN SELECT 1; SELECT 2; END"));
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "CALL twice()"));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT 7") == 7);

    // formatted
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_execf(db,
                                       "CREATE TABLE %I (%I TEXT)",
                                       "odd`name",
                                       "odd col"));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_execf(db,
                                       "INSERT INTO %T VALUES (%Q), (%Q)",
                                       schema,
                                       "odd`name",
                                       "it's \\ \"quoted\"",
                                       (const char*)NULL));
    D_TESTS_MARIADB_CHECK(
        d_tests_mariadb_int(db,
                            "SELECT COUNT(*) FROM `odd``name` "
                            "WHERE `odd col` IS NULL") == 1);
    D_TESTS_MARIADB_CHECK(
        d_tests_mariadb_text_is(db,
                                "SELECT `odd col` FROM `odd``name` "
                                "WHERE `odd col` IS NOT NULL",
                                "it's \\ \"quoted\""));

    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_scripts
  Running scripts.
  Tests the following:
  - exec_script runs several statements
  - a failure stops it, those before it having run, and its error still
    reads after; then exec refuses two statements again
*/
static void
d_tests_mariadb_scripts(
    void
)
{
    char              schema[96] = "";
    struct d_mariadb* db         = d_tests_mariadb_begin("scripts",
                                                         schema,
                                                         sizeof(schema));

    printf("mariadb_query: scripts\n");

    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec_script(db,
                                             "CREATE TABLE s (a INT); "
                                             "INSERT INTO s VALUES (2); "
                                             "INSERT INTO s VALUES (3)"));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT COUNT(*) FROM s") ==
                          2);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_exec_script(db,
                                                 "INSERT INTO s VALUES (4); "
                                                 "INSERT INTO none VALUES (5); "
                                                 "INSERT INTO s VALUES (6)"),
                           1146);
    D_TESTS_MARIADB_CHECK(d_mariadb_error_code(db) == 1146u);
    D_TESTS_MARIADB_CHECK(strstr(d_mariadb_error_message(db), "none"));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT COUNT(*) FROM s") ==
                          3);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_exec(db, "SELECT 1; SELECT 2"),
                           1064);
    D_TESTS_MARIADB_OK(db, d_mariadb_exec_script(db, "SELECT 1; SELECT 2"));
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_exec(db, "SELECT 1; SELECT 2"),
                           1064);

    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_integers
  Single integers.
  Tests the following:
  - integers read exactly, to both ends of int64_t; a fraction truncates
    toward zero; NULL reads as 0
  - text that is not a number, and a number past int64_t, are
    D_DB_INVALID_ARGUMENT, the value untouched
  - no row, and no result at all, are D_DB_NOT_FOUND, the value untouched
  - the `f` form formats its arguments
*/
static void
d_tests_mariadb_integers(
    void
)
{
    struct d_mariadb* const db     = d_tests_mariadb_connect(NULL);
    int64_t                 number = 0;

    printf("mariadb_query: integers\n");

    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT 42") == 42);
    D_TESTS_MARIADB_CHECK(
        d_tests_mariadb_int(db, "SELECT 9223372036854775807") == INT64_MAX);
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_query_int64(db,
                                             &number,
                                             "SELECT -9223372036854775808"));
    D_TESTS_MARIADB_CHECK(number == INT64_MIN);
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT 3.7") == 3);
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT -3.7") == -3);
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT 1e3") == 1000);
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT NULL") == 0);

    // refusals, the value untouched
    number = 17;
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_query_int64(db, &number, "SELECT 'abc'"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_query_int64(db,
                                                 &number,
                                                 "SELECT 18446744073709551615"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_query_int64(db,
                                                 &number,
                                                 "SELECT 1 FROM DUAL "
                                                 "WHERE FALSE"),
                           D_DB_NOT_FOUND);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_query_int64(db, &number, "SET @x = 1"),
                           D_DB_NOT_FOUND);
    D_TESTS_MARIADB_CHECK(number == 17);

    D_TESTS_MARIADB_OK(db,
                       d_mariadb_query_int64f(db,
                                              &number,
                                              "SELECT %d + %lld + %u",
                                              1,
                                              2LL,
                                              3u));
    D_TESTS_MARIADB_CHECK(number == 6);

    d_mariadb_close(db);

    return;
}

/*
d_tests_mariadb_doubles
  Single doubles.
  Tests the following:
  - doubles round as the server wrote them; NULL reads as 0; text that is
    not a number is D_DB_INVALID_ARGUMENT, the value untouched
  - formatted and read the same under a locale whose point is a comma
*/
static void
d_tests_mariadb_doubles(
    void
)
{
    struct d_mariadb* const db     = d_tests_mariadb_connect(NULL);
    int64_t                 number = 0;
    double                  real   = 2.0;

    printf("mariadb_query: doubles\n");

    D_TESTS_MARIADB_OK(db, d_mariadb_query_double(db, &real, "SELECT 1.5"));
    D_TESTS_MARIADB_CHECK(real == 1.5);
    D_TESTS_MARIADB_OK(db, d_mariadb_query_double(db, &real, "SELECT 0.1e0"));
    D_TESTS_MARIADB_CHECK(real == 0.1);
    D_TESTS_MARIADB_OK(db, d_mariadb_query_double(db, &real, "SELECT NULL"));
    D_TESTS_MARIADB_CHECK(real == 0.0);
    real = 2.0;
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_query_double(db, &real, "SELECT 'x'"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_CHECK(real == 2.0);
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_query_doublef(db,
                                               &real,
                                               "SELECT %g * 2",
                                               0.125));
    D_TESTS_MARIADB_CHECK(real == 0.25);

    // under a locale whose point is a comma, where one is installed
    if ( (setlocale(LC_NUMERIC, "de_DE.UTF-8")) ||
         (setlocale(LC_NUMERIC, "de_DE"))       ||
         (setlocale(LC_NUMERIC, "German")) )
    {
        D_TESTS_MARIADB_OK(db,
                           d_mariadb_query_double(db, &real, "SELECT 2.5"));
        D_TESTS_MARIADB_CHECK(real == 2.5);
        D_TESTS_MARIADB_OK(db,
                           d_mariadb_query_doublef(db,
                                                   &real,
                                                   "SELECT %g + %f",
                                                   0.25,
                                                   0.5));
        D_TESTS_MARIADB_CHECK(real == 0.75);
        D_TESTS_MARIADB_OK(db,
                           d_mariadb_query_int64(db, &number, "SELECT 9.9"));
        D_TESTS_MARIADB_CHECK(number == 9);
        setlocale(LC_NUMERIC, "C");
    }
    else
    {
        printf("  (no locale with a comma for its point; skipped)\n");
    }

    d_mariadb_close(db);

    return;
}

/*
d_tests_mariadb_text
  Single values as text.
  Tests the following:
  - text in the connection's character set, whole
  - NULL reads as a NULL string, with success
  - a value written with %Q reads back as it was, quotes, backslashes and all
  - no row is D_DB_NOT_FOUND, the value untouched
*/
static void
d_tests_mariadb_text(
    void
)
{
    struct d_mariadb* const db       = d_tests_mariadb_connect(NULL);
    const char* const       awkward  = "it's \\'; DROP TABLE x; -- \"\n\r%_";
    char*                   text     = NULL;
    char                    sentinel = '\0';

    printf("mariadb_query: text\n");

    D_TESTS_MARIADB_CHECK(
        d_tests_mariadb_text_is(db,
                                "SELECT 'h\xc3\xa9llo \xf0\x9f\x98\x80'",
                                "h\xc3\xa9llo \xf0\x9f\x98\x80"));
    D_TESTS_MARIADB_OK(db, d_mariadb_query_text(db, &text, "SELECT NULL"));
    D_TESTS_MARIADB_CHECK(text == NULL);
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_query_textf(db, &text, "SELECT %Q", awkward));
    D_TESTS_MARIADB_CHECK( (text) && (strcmp(text, awkward) == 0) );
    d_db_free(text);
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_query_textf(db,
                                             &text,
                                             "SELECT %Q",
                                             (const char*)NULL));
    D_TESTS_MARIADB_CHECK(text == NULL);

    text = &sentinel;
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_query_text(db,
                                                &text,
                                                "SELECT 'x' FROM DUAL "
                                                "WHERE FALSE"),
                           D_DB_NOT_FOUND);
    D_TESTS_MARIADB_CHECK(text == &sentinel);

    d_mariadb_close(db);

    return;
}

/*
d_tests_mariadb_changes_counted
  Running statements and counting the rows they changed.
  Tests the following:
  - an INSERT counts its rows; an UPDATE the rows it changed, not those it
    matched; a DELETE its rows
  - a SELECT counts the rows it returned; a CREATE TABLE straight after an
    UPDATE counts none
  - a failure writes no count; two statements are the server's syntax error
  - the `f` form formats
*/
static void
d_tests_mariadb_changes_counted(
    void
)
{
    char              schema[96] = "";
    struct d_mariadb* db         = d_tests_mariadb_begin("counted",
                                                         schema,
                                                         sizeof(schema));
    int64_t           changes    = -1;

    printf("mariadb_query: rows changed\n");

    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec(db,
                                      "CREATE TABLE r (id INT "
                                      "AUTO_INCREMENT PRIMARY KEY, v INT)"));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec_changes(db,
                                              &changes,
                                              "INSERT INTO r (v) "
                                              "VALUES (1), (2), (3)"));
    D_TESTS_MARIADB_CHECK(changes == 3);
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec_changes(db,
                                              &changes,
                                              "UPDATE r SET v = 2 "
                                              "WHERE v >= 2"));
    D_TESTS_MARIADB_CHECK(changes == 1);
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec_changes(db, &changes,
                                              "SELECT * FROM r"));
    D_TESTS_MARIADB_CHECK(changes == 3);
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec(db, "UPDATE r SET v = v + 10"));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec_changes(db, &changes,
                                              "CREATE TABLE later (a INT)"));
    D_TESTS_MARIADB_CHECK(changes == 0);
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_execf_changes(db,
                                               &changes,
                                               "DELETE FROM %I WHERE v > %d",
                                               "r",
                                               11));
    D_TESTS_MARIADB_CHECK(changes == 2);

    // failures write no count
    changes = -1;
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_exec_changes(db,
                                                  &changes,
                                                  "INSERT INTO none "
                                                  "VALUES (1)"),
                           1146);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_exec_changes(db,
                                                  &changes,
                                                  "SELECT 1; SELECT 2"),
                           1064);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_execf_changes(db,
                                                   &changes,
                                                   "SELECT %I",
                                                   (const char*)NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_CHECK(changes == -1);
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT COUNT(*) FROM r")
                          == 1);

    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_rows_values
  The values a query returns.
  Tests the following:
  - integers to both ends of int64_t, and an unsigned BIGINT past them as
    TEXT; a DOUBLE to the last bit; a FLOAT; a DECIMAL as TEXT; text;
    bytes with NULs and BIT values as BLOBs; a date as TEXT; NULL
  - TINYINT(1) is a BOOLEAN column of INTEGER values
  - empty text and empty bytes point at a terminator
  - columns are named by alias, or by the expression as written
  - no rows: the columns alone; no columns: empty rows
  - every result of a CALL is read, the first kept, and the connection
    runs its next statement
  - a failure leaves the rows alone; two statements are the server's syntax
    error
  - values read the same under a locale whose point is a comma
*/
static void
d_tests_mariadb_rows_values(
    void
)
{
    char              schema[96] = "";
    struct d_mariadb* db         = d_tests_mariadb_begin("rows",
                                                         schema,
                                                         sizeof(schema));
    struct d_db_rows  rows       = { 0u, 0u, NULL, NULL };
    struct d_db_rows  marker     = { 7u, 7u, NULL, NULL };

    printf("mariadb_query: rows\n");

    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec(db,
                                      "CREATE TABLE v (id INT PRIMARY KEY, "
                                      "flag TINYINT(1), big BIGINT, "
                                      "ubig BIGINT UNSIGNED, d DOUBLE, "
                                      "f FLOAT, n DECIMAL(10,2), "
                                      "t VARCHAR(20), b VARBINARY(8), "
                                      "bits BIT(3), day DATE)"));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec(db,
                                      "INSERT INTO v VALUES "
                                      "(1, 1, -9223372036854775808, "
                                      "18446744073709551615, "
                                      "0.1e0 + 0.2e0, 0.25, 12.34, 'it''s', "
                                      "x'00ff00', b'101', '2024-05-06'), "
                                      "(2, 0, 9223372036854775807, 5, NULL, "
                                      "NULL, NULL, '', x'', NULL, NULL)"));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_query_rows(db,
                                            &rows,
                                            "SELECT * FROM v ORDER BY id"));
    D_TESTS_MARIADB_CHECK( (rows.column_count == 11u) &&
                           (rows.row_count == 2u) );

    // the values of each row
    if ( (rows.column_count == 11u) &&
         (rows.row_count == 2u) )
    {
        const struct d_db_value* const one = &rows.values[0];
        const struct d_db_value* const two = &rows.values[11];

        D_TESTS_MARIADB_CHECK(rows.columns[1].type == D_DB_TYPE_BOOLEAN);
        D_TESTS_MARIADB_CHECK( (one[1].type == D_DB_VALUE_INTEGER) &&
                               (one[1].as.integer == 1)            &&
                               (two[1].as.integer == 0) );
        D_TESTS_MARIADB_CHECK( (one[2].type == D_DB_VALUE_INTEGER) &&
                               (one[2].as.integer == INT64_MIN)    &&
                               (two[2].as.integer == INT64_MAX) );
        D_TESTS_MARIADB_CHECK(
            (one[3].type == D_DB_VALUE_TEXT) &&
            (strcmp(one[3].as.text, "18446744073709551615") == 0));
        D_TESTS_MARIADB_CHECK( (two[3].type == D_DB_VALUE_INTEGER) &&
                               (two[3].as.integer == 5) );
        D_TESTS_MARIADB_CHECK( (one[4].type == D_DB_VALUE_REAL) &&
                               (one[4].as.real == 0.30000000000000004) );
        D_TESTS_MARIADB_CHECK( (one[5].type == D_DB_VALUE_REAL) &&
                               (one[5].as.real == 0.25) );
        D_TESTS_MARIADB_CHECK( (one[6].type == D_DB_VALUE_TEXT) &&
                               (strcmp(one[6].as.text, "12.34") == 0) );
        D_TESTS_MARIADB_CHECK( (one[7].type == D_DB_VALUE_TEXT) &&
                               (strcmp(one[7].as.text, "it's") == 0) );
        D_TESTS_MARIADB_CHECK(
            (one[8].type == D_DB_VALUE_BLOB) &&
            (one[8].size == 3u)              &&
            (memcmp(one[8].as.blob, "\0\xff\0", 3u) == 0));
        D_TESTS_MARIADB_CHECK(
            (one[9].type == D_DB_VALUE_BLOB) &&
            (one[9].size == 1u)              &&
            (((const unsigned char*)one[9].as.blob)[0] == 5u));
        D_TESTS_MARIADB_CHECK( (one[10].type == D_DB_VALUE_TEXT) &&
                               (strcmp(one[10].as.text, "2024-05-06") == 0) );
        D_TESTS_MARIADB_CHECK( (two[4].type == D_DB_VALUE_NULL) &&
                               (two[9].type == D_DB_VALUE_NULL) );
        D_TESTS_MARIADB_CHECK( (two[7].type == D_DB_VALUE_TEXT) &&
                               (two[7].size == 0u)              &&
                               (two[7].as.text[0] == '\0') );
        D_TESTS_MARIADB_CHECK( (two[8].type == D_DB_VALUE_BLOB) &&
                               (two[8].size == 0u)              &&
                               (two[8].as.blob) );
    }

    d_db_rows_free(&rows);

    // names: an alias, an expression as written
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_query_rows(db,
                                            &rows,
                                            "SELECT id AS ident, 1 + 1 "
                                            "FROM v WHERE id = 1"));
    D_TESTS_MARIADB_CHECK( (rows.column_count == 2u)                     &&
                           (strcmp(rows.columns[0].name, "ident") == 0)  &&
                           (strcmp(rows.columns[1].name, "1 + 1") == 0)  &&
                           (rows.values[1].as.integer == 2) );
    d_db_rows_free(&rows);

    // no rows, and no columns
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_query_rows(db,
                                            &rows,
                                            "SELECT id, t FROM v WHERE 0"));
    D_TESTS_MARIADB_CHECK( (rows.column_count == 2u) &&
                           (rows.row_count == 0u)    &&
                           (rows.values == NULL) );
    d_db_rows_free(&rows);
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_query_rows(db,
                                            &rows,
                                            "UPDATE v SET t = 'x' "
                                            "WHERE id = 2"));
    D_TESTS_MARIADB_CHECK( (rows.column_count == 0u) &&
                           (rows.row_count == 0u)    &&
                           (rows.columns == NULL) );
    D_TESTS_MARIADB_CHECK(
        d_tests_mariadb_text_is(db, "SELECT t FROM v WHERE id = 2", "x"));
    d_db_rows_free(&rows);

    // a procedure answering twice: the first answer, then the next statement
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec(db,
                                      "CREATE PROCEDURE twice() "
                                      "BEGIN SELECT 1 AS a; "
                                      "SELECT 2 AS b, 3 AS c; END"));
    D_TESTS_MARIADB_OK(db, d_mariadb_query_rows(db, &rows, "CALL twice()"));
    D_TESTS_MARIADB_CHECK( (rows.column_count == 1u) &&
                           (rows.row_count == 1u)    &&
                           (rows.values[0].as.integer == 1) );
    d_db_rows_free(&rows);
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT 7") == 7);

    // failures
    rows = marker;
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_query_rows(db, &rows,
                                                "SELECT nope FROM v"),
                           1054);
    D_TESTS_MARIADB_CHECK(strstr(d_mariadb_error_message(db), "nope"));
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_query_rows(db, &rows,
                                                "SELECT 1; SELECT 2"),
                           1064);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_query_rowsf(db, &rows, "SELECT %x", 1),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_CHECK( (rows.column_count == 7u) &&
                           (rows.row_count == 7u) );

    // formatted, and under a comma for the point where one is installed
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_query_rowsf(db,
                                             &rows,
                                             "SELECT t FROM %T WHERE t = %Q",
                                             schema,
                                             "v",
                                             "it's"));
    D_TESTS_MARIADB_CHECK(rows.row_count == 1u);
    d_db_rows_free(&rows);

    if ( (setlocale(LC_NUMERIC, "de_DE.UTF-8")) ||
         (setlocale(LC_NUMERIC, "de_DE"))       ||
         (setlocale(LC_NUMERIC, "German")) )
    {
        D_TESTS_MARIADB_OK(db,
                           d_mariadb_query_rows(db, &rows,
                                                "SELECT d FROM v "
                                                "WHERE id = 1"));
        D_TESTS_MARIADB_CHECK( (rows.row_count == 1u) &&
                               (rows.values[0].as.real ==
                                0.30000000000000004) );
        d_db_rows_free(&rows);
        setlocale(LC_NUMERIC, "C");
    }

    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_rows_types
  The types the columns of a query's rows are read as.
  Tests the following:
  - every type mariadb_query.h tabulates, with its name as DDL writes it,
    UNSIGNED included, and each TEXT and BLOB by its size
  - a string column is binary by its character set
  - NULL, a GEOMETRY and JSON, which is MySQL's own type and MariaDB's
    LONGTEXT
*/
static void
d_tests_mariadb_rows_types(
    void
)
{
    static const struct
    {
        const char*    declared;  // the column's type, in DDL
        enum d_db_type type;      // what it reads as
        const char*    name;      // and its name
    } expected[] =
    {
        { "TINYINT(1)",         D_DB_TYPE_BOOLEAN,        "TINYINT" },
        { "TINYINT",            D_DB_TYPE_INTEGER,        "TINYINT" },
        { "SMALLINT UNSIGNED",  D_DB_TYPE_INTEGER,        "SMALLINT UNSIGNED" },
        { "MEDIUMINT",          D_DB_TYPE_INTEGER,        "MEDIUMINT" },
        { "INT",                D_DB_TYPE_INTEGER,        "INT" },
        { "INT UNSIGNED",       D_DB_TYPE_BIG_INTEGER,    "INT UNSIGNED" },
        { "BIGINT",             D_DB_TYPE_BIG_INTEGER,    "BIGINT" },
        { "BIGINT UNSIGNED",    D_DB_TYPE_BIG_INTEGER,    "BIGINT UNSIGNED" },
        { "YEAR",               D_DB_TYPE_INTEGER,        "YEAR" },
        { "FLOAT",              D_DB_TYPE_FLOATING_POINT, "FLOAT" },
        { "DOUBLE",             D_DB_TYPE_FLOATING_POINT, "DOUBLE" },
        { "DECIMAL(8,3)",       D_DB_TYPE_DECIMAL,        "DECIMAL" },
        { "CHAR(3)",            D_DB_TYPE_STRING,         "CHAR" },
        { "VARCHAR(9)",         D_DB_TYPE_STRING,         "VARCHAR" },
        { "TINYTEXT",           D_DB_TYPE_STRING,         "TINYTEXT" },
        { "TEXT",               D_DB_TYPE_STRING,         "TEXT" },
        { "MEDIUMTEXT",         D_DB_TYPE_STRING,         "MEDIUMTEXT" },
        { "LONGTEXT",           D_DB_TYPE_STRING,         "LONGTEXT" },
        { "TEXT CHARACTER SET latin1", D_DB_TYPE_STRING,  "TEXT" },
        { "ENUM('a', 'b')",     D_DB_TYPE_STRING,         "ENUM" },
        { "SET('x', 'y')",      D_DB_TYPE_STRING,         "SET" },
        { "BINARY(3)",          D_DB_TYPE_BINARY,         "BINARY" },
        { "VARBINARY(9)",       D_DB_TYPE_BINARY,         "VARBINARY" },
        { "VARCHAR(9) CHARACTER SET binary",
                                D_DB_TYPE_BINARY,         "VARBINARY" },
        { "TINYBLOB",           D_DB_TYPE_BINARY,         "TINYBLOB" },
        { "BLOB",               D_DB_TYPE_BINARY,         "BLOB" },
        { "MEDIUMBLOB",         D_DB_TYPE_BINARY,         "MEDIUMBLOB" },
        { "LONGBLOB",           D_DB_TYPE_BINARY,         "LONGBLOB" },
        { "BIT(4)",             D_DB_TYPE_BINARY,         "BIT" },
        { "DATE",               D_DB_TYPE_DATE,           "DATE" },
        { "TIME",               D_DB_TYPE_TIME,           "TIME" },
        { "DATETIME(3)",        D_DB_TYPE_DATETIME,       "DATETIME" },
        { "TIMESTAMP NULL",     D_DB_TYPE_TIMESTAMP,      "TIMESTAMP" },
        { "GEOMETRY",           D_DB_TYPE_NONE,           "GEOMETRY" }
    };
    const size_t      count      = sizeof(expected) / sizeof(expected[0]);
    char              schema[96] = "";
    struct d_mariadb* db         = d_tests_mariadb_begin("types",
                                                         schema,
                                                         sizeof(schema));
    struct d_db_rows  rows       = { 0u, 0u, NULL, NULL };

    printf("mariadb_query: column types\n");

    for (size_t i = 0u; i < count; ++i)
    {
        D_TESTS_MARIADB_OK(db,
                           d_mariadb_execf(db,
                                           "CREATE TABLE t%d (c %s)",
                                           (int)i,
                                           expected[i].declared));
        D_TESTS_MARIADB_OK(db,
                           d_mariadb_query_rowsf(db,
                                                 &rows,
                                                 "SELECT c FROM t%d",
                                                 (int)i));

        const bool typed = ( (rows.column_count == 1u)                    &&
                             (rows.columns[0].type == expected[i].type)   &&
                             (strcmp(rows.columns[0].type_name,
                                     expected[i].name) == 0) );

        // say which column was misread, beyond the line
        if (!typed)
        {
            printf("    %s read as %s (%s)\n",
                   expected[i].declared,
                   (rows.column_count == 1u)
                       ? d_db_type_name(rows.columns[0].type)
                       : "nothing",
                   (rows.column_count == 1u) ? rows.columns[0].type_name
                                             : "");
        }

        D_TESTS_MARIADB_CHECK(typed);
        d_db_rows_free(&rows);
    }

    // NULL, and JSON by flavor
    D_TESTS_MARIADB_OK(db, d_mariadb_query_rows(db, &rows, "SELECT NULL"));
    D_TESTS_MARIADB_CHECK( (rows.column_count == 1u)                    &&
                           (rows.columns[0].type == D_DB_TYPE_NONE)     &&
                           (strcmp(rows.columns[0].type_name, "NULL") == 0) &&
                           (rows.values[0].type == D_DB_VALUE_NULL) );
    d_db_rows_free(&rows);
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "CREATE TABLE j (c JSON)"));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec(db,
                                      "INSERT INTO j VALUES ('{\"a\": 1}')"));
    D_TESTS_MARIADB_OK(db, d_mariadb_query_rows(db, &rows, "SELECT c FROM j"));
    D_TESTS_MARIADB_CHECK( (rows.column_count == 1u) &&
                           (rows.row_count == 1u)    &&
                           (rows.values[0].type == D_DB_VALUE_TEXT) );
    D_TESTS_MARIADB_CHECK(
        (d_tests_mariadb_is_mariadb(db))
            ? ( (rows.columns[0].type == D_DB_TYPE_STRING) &&
                (strcmp(rows.columns[0].type_name, "LONGTEXT") == 0) )
            : ( (rows.columns[0].type == D_DB_TYPE_JSON) &&
                (strcmp(rows.columns[0].type_name, "JSON") == 0) ));
    d_db_rows_free(&rows);

    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_quoting_names
  Quoting names.
  Tests the following:
  - a name in backquotes, a backquote doubled, an empty one too
  - NULL gives NULL
*/
static void
d_tests_mariadb_quoting_names(
    void
)
{
    char* quoted = d_mariadb_quote_identifier("plain");

    printf("mariadb_query: quoting names\n");

    D_TESTS_MARIADB_CHECK( (quoted) && (strcmp(quoted, "`plain`") == 0) );
    d_db_free(quoted);
    quoted = d_mariadb_quote_identifier("a`b``c");
    D_TESTS_MARIADB_CHECK( (quoted) && (strcmp(quoted, "`a``b````c`") == 0) );
    d_db_free(quoted);
    quoted = d_mariadb_quote_identifier("");
    D_TESTS_MARIADB_CHECK( (quoted) && (strcmp(quoted, "``") == 0) );
    d_db_free(quoted);
    D_TESTS_MARIADB_CHECK(d_mariadb_quote_identifier(NULL) == NULL);

    return;
}

/*
d_tests_mariadb_quoting_values
  Quoting values.
  Tests the following:
  - a value the server reads back as it was, with backslash escapes and,
    under NO_BACKSLASH_ESCAPES, with quotes doubled instead; %Q likewise
  - NULL becomes the keyword; a NULL handle gives NULL
*/
static void
d_tests_mariadb_quoting_values(
    void
)
{
    struct d_mariadb* const db      = d_tests_mariadb_connect(NULL);
    const char* const       awkward = "a\\b'c\"d\n%";
    char*                   quoted  = NULL;
    char*                   text    = NULL;

    printf("mariadb_query: quoting values\n");

    // as each SQL mode reads them
    for (int mode = 0; mode < 2; ++mode)
    {
        D_TESTS_MARIADB_OK(db,
                           d_mariadb_set_sql_mode(
                               db,
                               (mode) ? "NO_BACKSLASH_ESCAPES" : ""));
        quoted = d_mariadb_quote_literal(db, awkward);
        D_TESTS_MARIADB_CHECK( (quoted) &&
                               ( (mode) ? (strstr(quoted, "''") != NULL)
                                        : (strstr(quoted, "\\'") != NULL) ) );
        D_TESTS_MARIADB_OK(db,
                           d_mariadb_query_textf(db,
                                                 &text,
                                                 "SELECT %s",
                                                 quoted));
        D_TESTS_MARIADB_CHECK( (text) && (strcmp(text, awkward) == 0) );
        d_db_free(text);
        d_db_free(quoted);
        text = NULL;
        D_TESTS_MARIADB_OK(db,
                           d_mariadb_query_textf(db,
                                                 &text,
                                                 "SELECT %Q",
                                                 awkward));
        D_TESTS_MARIADB_CHECK( (text) && (strcmp(text, awkward) == 0) );
        d_db_free(text);
        text = NULL;
    }

    quoted = d_mariadb_quote_literal(db, NULL);
    D_TESTS_MARIADB_CHECK( (quoted) && (strcmp(quoted, "NULL") == 0) );
    d_db_free(quoted);
    D_TESTS_MARIADB_CHECK(d_mariadb_quote_literal(NULL, "x") == NULL);

    d_mariadb_close(db);

    return;
}

/*
d_tests_mariadb_query_arguments
  Argument checks for running, counting and reading.
  Tests the following:
  - NULL handles, SQL, formats, counts, outputs and rows
  - a format the layer cannot read, and a value SQL cannot spell
*/
static void
d_tests_mariadb_query_arguments(
    void
)
{
    struct d_mariadb* const db     = d_tests_mariadb_connect(NULL);
    int64_t                 number = 0;
    double                  real   = 0.0;
    char*                   text   = NULL;
    const double            zero   = 0.0;
    struct d_db_rows        rows   = { 0u, 0u, NULL, NULL };

    printf("mariadb_query: arguments\n");

    D_TESTS_MARIADB_RESULT(db, d_mariadb_exec(NULL, "SELECT 1"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_exec(db, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_execf(NULL, "SELECT 1"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_execf(db, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_execf(db, "SELECT %x", 1),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_execf(db, "SELECT %g", zero / zero),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_execf(db, "SELECT %I",
                                               (const char*)NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_exec_script(NULL, "SELECT 1"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_exec_script(db, NULL),
                           D_DB_INVALID_ARGUMENT);

    D_TESTS_MARIADB_RESULT(db, d_mariadb_query_int64(NULL, &number, "SELECT 1"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_query_int64(db, NULL, "SELECT 1"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_query_int64(db, &number, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_query_int64f(db, &number, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_query_double(db, &real, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_query_double(db, NULL, "SELECT 1"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_query_doublef(NULL, &real, "SELECT 1"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_query_text(db, &text, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_query_text(db, NULL, "SELECT 1"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_query_textf(NULL, &text, "SELECT 1"),
                           D_DB_INVALID_ARGUMENT);

    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_exec_changes(NULL, &number, "SELECT 1"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_exec_changes(db, NULL, "SELECT 1"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_exec_changes(db, &number, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_execf_changes(NULL, &number, "SELECT 1"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_execf_changes(db, NULL, "SELECT 1"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_execf_changes(db, &number, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_query_rows(NULL, &rows, "SELECT 1"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_query_rows(db, NULL, "SELECT 1"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_query_rows(db, &rows, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_query_rowsf(NULL, &rows, "SELECT 1"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_query_rowsf(db, NULL, "SELECT 1"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_query_rowsf(db, &rows, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_CHECK( (rows.columns == NULL) &&
                           (rows.values == NULL) );

    d_mariadb_close(db);

    return;
}

/*
d_tests_mariadb_query_attributes
  Query attributes.
  Tests the following:
  - they are supported exactly where the server and the client have them
  - where they are, an attribute set goes with the next query, which drops
    it, read back with mysql_query_attribute_string() where the server has
    the query_attributes component -- which the test installs where it can,
    and removes again: setting a name again replaces its value; several go
    together; clearing drops them; a prepared statement's execution neither
    carries nor drops them; a reset drops them; an empty value is one
  - where they are not, D_DB_UNSUPPORTED, and clearing succeeds
  - NULL arguments and an empty name
*/
static void
d_tests_mariadb_query_attributes(
    void
)
{
    static const char* const read_a =
        "SELECT mysql_query_attribute_string('d_a')";
    struct d_mariadb* const  db     = d_tests_mariadb_connect(NULL);
    const bool               can    =
        d_mariadb_supports(db, D_MARIADB_FEATURE_QUERY_ATTRIBUTES);

    printf("mariadb_query: query attributes\n");

    D_TESTS_MARIADB_CHECK(
        can ==
        ( (d_tests_mariadb_client_has(D_MARIADB_FEATURE_QUERY_ATTRIBUTES)) &&
          (d_tests_mariadb_mysql_at_least(db, 8u, 0u, 23u)) ));

    if (can)
    {
        const bool had      = (d_tests_mariadb_int(
                                   db,
                                   "SELECT COUNT(*) FROM mysql.component "
                                   "WHERE component_urn = "
                                   "'file://component_query_attributes'") ==
                               1);
        const bool made     = ( (!had) &&
                                (d_mariadb_exec(
                                     db,
                                     "INSTALL COMPONENT "
                                     "'file://component_query_attributes'") ==
                                 0) );
        const bool readable = ( (had) ||
                                (made) );

        // say why the values cannot be read back, where they cannot
        if (!readable)
        {
            printf("    the server has no query_attributes component: the "
                   "values go unread\n");
        }

        // one attribute, for one query
        D_TESTS_MARIADB_OK(db, d_mariadb_set_query_attribute(db, "d_a", "hi"));
        D_TESTS_MARIADB_CHECK( (!readable) ||
                               (d_tests_mariadb_text_is(db, read_a, "hi")) );
        D_TESTS_MARIADB_CHECK( (!readable) ||
                               (d_tests_mariadb_text_is(db, read_a, NULL)) );

        // a value replaced, and two together
        D_TESTS_MARIADB_OK(db, d_mariadb_set_query_attribute(db, "d_a", "one"));
        D_TESTS_MARIADB_OK(db, d_mariadb_set_query_attribute(db, "d_a", "two"));
        D_TESTS_MARIADB_CHECK( (!readable) ||
                               (d_tests_mariadb_text_is(db, read_a, "two")) );
        D_TESTS_MARIADB_OK(db, d_mariadb_set_query_attribute(db, "d_a", "x"));
        D_TESTS_MARIADB_OK(db, d_mariadb_set_query_attribute(db, "d_b", "y"));
        D_TESTS_MARIADB_CHECK(
            (!readable) ||
            (d_tests_mariadb_text_is(
                 db,
                 "SELECT CONCAT(mysql_query_attribute_string('d_a'), "
                 "mysql_query_attribute_string('d_b'))",
                 "xy")) );

        // cleared
        D_TESTS_MARIADB_OK(db, d_mariadb_set_query_attribute(db, "d_a", "z"));
        D_TESTS_MARIADB_OK(db, d_mariadb_clear_query_attributes(db));
        D_TESTS_MARIADB_CHECK( (!readable) ||
                               (d_tests_mariadb_text_is(db, read_a, NULL)) );

        // a prepared statement neither carries them nor drops them
        {
            struct d_mariadb_statement* statement = NULL;
            struct d_db_rows            rows      = { 0u, 0u, NULL, NULL };

            D_TESTS_MARIADB_OK(db,
                               d_mariadb_set_query_attribute(db, "d_a", "k"));
            D_TESTS_MARIADB_OK(db,
                               d_mariadb_prepare(db,
                                                 &statement,
                                                 (readable) ? read_a
                                                            : "SELECT NULL"));
            D_TESTS_MARIADB_OK(db, d_mariadb_execute_query(statement, &rows));
            D_TESTS_MARIADB_CHECK(
                (d_db_rows_value(&rows, 0u, 0u)) &&
                (d_db_rows_value(&rows, 0u, 0u)->type == D_DB_VALUE_NULL));
            d_db_rows_free(&rows);
            D_TESTS_MARIADB_OK(db, d_mariadb_finalize(statement));
            D_TESTS_MARIADB_CHECK( (!readable) ||
                                   (d_tests_mariadb_text_is(db, read_a, "k")) );
        }

        // an empty value, and a reset
        D_TESTS_MARIADB_OK(db, d_mariadb_set_query_attribute(db, "d_a", ""));
        D_TESTS_MARIADB_CHECK( (!readable) ||
                               (d_tests_mariadb_text_is(db, read_a, "")) );
        D_TESTS_MARIADB_OK(db, d_mariadb_set_query_attribute(db, "d_a", "r"));
        D_TESTS_MARIADB_OK(db, d_mariadb_reset_connection(db));
        D_TESTS_MARIADB_CHECK( (!readable) ||
                               (d_tests_mariadb_text_is(db, read_a, NULL)) );

        // the component goes again, where the test brought it
        if (made)
        {
            D_TESTS_MARIADB_OK(db,
                               d_mariadb_exec(
                                   db,
                                   "UNINSTALL COMPONENT "
                                   "'file://component_query_attributes'"));
        }
    }
    else
    {
        D_TESTS_MARIADB_RESULT(db,
                               d_mariadb_set_query_attribute(db, "d_a", "x"),
                               D_DB_UNSUPPORTED);
        D_TESTS_MARIADB_OK(db, d_mariadb_clear_query_attributes(db));
    }

    D_TESTS_MARIADB_RESULT(NULL,
                           d_mariadb_set_query_attribute(NULL, "d_a", "x"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_set_query_attribute(db, NULL, "x"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_set_query_attribute(db, "", "x"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_set_query_attribute(db, "d_a", NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(NULL,
                           d_mariadb_clear_query_attributes(NULL),
                           D_DB_INVALID_ARGUMENT);

    d_mariadb_close(db);

    return;
}

//==============================================================================
// mariadb_statement.h
//==============================================================================

/*
d_tests_mariadb_preparing
  Preparing, describing and finalizing statements.
  Tests the following:
  - parameters counted by their `?`; the statement knows its connection
  - a statement the server will not prepare is its error, with its message
    still readable after, and leaves NULL; the next statement runs, and the
    message is gone; two statements are the server's syntax error
  - preparef formats with the connection's quoting
  - finalize accepts NULL; a statement finalized after its connection closed
    is only freed
*/
static void
d_tests_mariadb_preparing(
    void
)
{
    char                        schema[96] = "";
    struct d_mariadb*           db         = d_tests_mariadb_begin("prepare",
                                                                   schema,
                                                                   sizeof(
                                                                     schema));
    struct d_mariadb*           other      = d_tests_mariadb_connect(schema);
    struct d_mariadb_statement* statement  = NULL;

    printf("mariadb_statement: preparing\n");

    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec(db, "CREATE TABLE p (a INT, b TEXT)"));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_prepare(db,
                                         &statement,
                                         "INSERT INTO p VALUES (?, ?)"));
    D_TESTS_MARIADB_CHECK(d_mariadb_parameter_count(statement) == 2u);
    D_TESTS_MARIADB_CHECK(d_mariadb_statement_connection(statement) == db);
    D_TESTS_MARIADB_OK(db, d_mariadb_finalize(statement));
    D_TESTS_MARIADB_OK(db, d_mariadb_prepare(db, &statement, "SELECT 1"));
    D_TESTS_MARIADB_CHECK(d_mariadb_parameter_count(statement) == 0u);
    D_TESTS_MARIADB_OK(db, d_mariadb_finalize(statement));

    // refusals: the message reads, then the next statement clears it
    statement = (struct d_mariadb_statement*)&g_options;
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_prepare(db, &statement, "SELEC bad"),
                           1064);
    D_TESTS_MARIADB_CHECK(statement == NULL);
    D_TESTS_MARIADB_CHECK(d_mariadb_error_code(db) == 1064u);
    D_TESTS_MARIADB_CHECK(strstr(d_mariadb_error_message(db), "SELEC"));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT 7") == 7);
    D_TESTS_MARIADB_CHECK(d_mariadb_error_code(db) == 0u);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_prepare(db, &statement,
                                             "SELECT * FROM none"),
                           1146);
    D_TESTS_MARIADB_CHECK(strstr(d_mariadb_error_message(db), "none"));
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_prepare(db, &statement,
                                             "SELECT 1; SELECT 2"),
                           1064);
    D_TESTS_MARIADB_CHECK(statement == NULL);

    // formatted
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_preparef(db,
                                          &statement,
                                          "SELECT %I FROM %T WHERE b = %Q "
                                          "AND a = ?",
                                          "a",
                                          schema,
                                          "p",
                                          "it's"));
    D_TESTS_MARIADB_CHECK(d_mariadb_parameter_count(statement) == 1u);
    D_TESTS_MARIADB_OK(db, d_mariadb_finalize(statement));
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_preparef(db, &statement, "SELECT %x", 1),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_CHECK(statement == NULL);
    D_TESTS_MARIADB_OK(NULL, d_mariadb_finalize(NULL));

    // finalized after its connection: only freed
    D_TESTS_MARIADB_OK(other, d_mariadb_prepare(other, &statement,
                                                "SELECT ?"));
    d_mariadb_close(other);
    D_TESTS_MARIADB_OK(NULL, d_mariadb_finalize(statement));

    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_binding
  Giving parameters values.
  Tests the following:
  - each bind stores its value: NULL, a boolean as 1 or 0, the extreme
    integers, a DOUBLE to the last bit, text, bytes with NULs
  - text and bytes are copied: changing the caller's buffer after the bind
    changes nothing
  - NULL text and NULL bytes bind NULL; a pointer to no bytes binds the
    empty string of bytes
  - a parameter numbered 0 or past the last is 2034, which has words of its
    own; clearing makes every parameter NULL again
*/
static void
d_tests_mariadb_binding(
    void
)
{
    char                        schema[96] = "";
    struct d_mariadb*           db         = d_tests_mariadb_begin("binding",
                                                                   schema,
                                                                   sizeof(
                                                                     schema));
    struct d_mariadb_statement* statement  = NULL;
    struct d_db_rows            rows       = { 0u, 0u, NULL, NULL };
    char                        text[]     = "it's";
    unsigned char               bytes[]    = { 0x00, 0x01, 0x00 };
    int64_t                     changes    = 0;

    printf("mariadb_statement: binding\n");

    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec(db,
                                      "CREATE TABLE v (k INT "
                                      "AUTO_INCREMENT PRIMARY KEY, n INT, "
                                      "t TINYINT(1), f TINYINT(1), "
                                      "lo BIGINT, hi BIGINT, r DOUBLE, "
                                      "s TEXT, b BLOB)"));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_prepare(db,
                                         &statement,
                                         "INSERT INTO v (n, t, f, lo, hi, r, "
                                         "s, b) VALUES "
                                         "(?, ?, ?, ?, ?, ?, ?, ?)"));
    D_TESTS_MARIADB_OK(db, d_mariadb_bind_null(statement, 1u));
    D_TESTS_MARIADB_OK(db, d_mariadb_bind_bool(statement, 2u, true));
    D_TESTS_MARIADB_OK(db, d_mariadb_bind_bool(statement, 3u, false));
    D_TESTS_MARIADB_OK(db, d_mariadb_bind_int64(statement, 4u, INT64_MIN));
    D_TESTS_MARIADB_OK(db, d_mariadb_bind_int64(statement, 5u, INT64_MAX));
    D_TESTS_MARIADB_OK(db, d_mariadb_bind_double(statement, 6u,
                                                 0.30000000000000004));
    D_TESTS_MARIADB_OK(db, d_mariadb_bind_text(statement, 7u, text));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_bind_blob(statement, 8u, bytes,
                                           sizeof(bytes)));

    // the copies outlive the caller's buffers
    text[0]  = 'X';
    bytes[1] = 0xffu;
    D_TESTS_MARIADB_OK(db, d_mariadb_execute_update(statement, &changes));
    D_TESTS_MARIADB_CHECK(changes == 1);

    // NULL text and bytes, and no bytes
    D_TESTS_MARIADB_OK(db, d_mariadb_bind_text(statement, 7u, NULL));
    D_TESTS_MARIADB_OK(db, d_mariadb_bind_blob(statement, 8u, bytes, 0u));
    D_TESTS_MARIADB_OK(db, d_mariadb_bind_blob(statement, 6u, NULL, 2u));
    D_TESTS_MARIADB_OK(db, d_mariadb_execute_update(statement, NULL));

    // cleared: NULL throughout
    D_TESTS_MARIADB_OK(db, d_mariadb_clear_bindings(statement));
    D_TESTS_MARIADB_OK(db, d_mariadb_execute_update(statement, NULL));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_query_rows(db,
                                            &rows,
                                            "SELECT n, t, f, lo, hi, r, s, b "
                                            "FROM v ORDER BY k"));
    D_TESTS_MARIADB_CHECK( (rows.column_count == 8u) &&
                           (rows.row_count == 3u) );

    // the rows: every kind, then NULLs and no bytes, then NULLs throughout
    if ( (rows.column_count == 8u) &&
         (rows.row_count == 3u) )
    {
        const struct d_db_value* const v = rows.values;

        D_TESTS_MARIADB_CHECK(v[0].type == D_DB_VALUE_NULL);
        D_TESTS_MARIADB_CHECK( (v[1].as.integer == 1) &&
                               (v[2].as.integer == 0) );
        D_TESTS_MARIADB_CHECK( (v[3].as.integer == INT64_MIN) &&
                               (v[4].as.integer == INT64_MAX) );
        D_TESTS_MARIADB_CHECK( (v[5].type == D_DB_VALUE_REAL) &&
                               (v[5].as.real == 0.30000000000000004) );
        D_TESTS_MARIADB_CHECK( (v[6].type == D_DB_VALUE_TEXT) &&
                               (strcmp(v[6].as.text, "it's") == 0) );
        D_TESTS_MARIADB_CHECK( (v[7].type == D_DB_VALUE_BLOB) &&
                               (v[7].size == 3u)              &&
                               (memcmp(v[7].as.blob, "\0\1\0", 3u) == 0) );
        D_TESTS_MARIADB_CHECK( (v[8 + 5].type == D_DB_VALUE_NULL) &&
                               (v[8 + 6].type == D_DB_VALUE_NULL) );
        D_TESTS_MARIADB_CHECK( (v[8 + 7].type == D_DB_VALUE_BLOB) &&
                               (v[8 + 7].size == 0u) );
        D_TESTS_MARIADB_CHECK( (v[16 + 1].type == D_DB_VALUE_NULL) &&
                               (v[16 + 4].type == D_DB_VALUE_NULL) &&
                               (v[16 + 7].type == D_DB_VALUE_NULL) );
    }

    d_db_rows_free(&rows);

    // numbers the statement does not have
    D_TESTS_MARIADB_RESULT(db, d_mariadb_bind_null(statement, 0u), 2034);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_bind_int64(statement, 9u, 1), 2034);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_bind_bool(statement, 9u, true),
                           2034);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_bind_double(statement, 9u, 1.0),
                           2034);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_bind_text(statement, 0u, "x"), 2034);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_bind_blob(statement, 9u, "x", 1u),
                           2034);
    D_TESTS_MARIADB_CHECK(
        strcmp(d_mariadb_result_message(2034),
               "the statement has no parameter of that number") == 0);
    D_TESTS_MARIADB_OK(db, d_mariadb_finalize(statement));

    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_running
  Running statements, once and again.
  Tests the following:
  - execute_update counts an INSERT's rows, and an UPDATE the rows it
    changed; values stay bound between runs
  - a run that fails is the server's error, writes no count, and leaves the
    statement ready: the next run, rebound, succeeds
  - execute_update counts a SELECT's rows; execute_query returns them, the
    same on a second run; text longer than any value before it reads whole
  - execute_query of a statement without rows gives empty rows, and runs it
  - every result of a CALL is read, the first kept, and the connection runs
    its next statement
*/
static void
d_tests_mariadb_running(
    void
)
{
    char                        schema[96] = "";
    struct d_mariadb*           db         = d_tests_mariadb_begin("running",
                                                                   schema,
                                                                   sizeof(
                                                                     schema));
    struct d_mariadb_statement* insert     = NULL;
    struct d_mariadb_statement* select     = NULL;
    struct d_db_rows            rows       = { 0u, 0u, NULL, NULL };
    int64_t                     changes    = -1;
    static char                 longest[70001] = "";

    printf("mariadb_statement: running\n");

    memset(longest, 'x', sizeof(longest) - 1u);
    longest[sizeof(longest) - 1u] = '\0';

    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec(db,
                                      "CREATE TABLE k (id INT PRIMARY KEY, "
                                      "v MEDIUMTEXT)"));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_prepare(db, &insert,
                                         "INSERT INTO k VALUES (?, ?)"));
    D_TESTS_MARIADB_OK(db, d_mariadb_bind_int64(insert, 1u, 1));
    D_TESTS_MARIADB_OK(db, d_mariadb_bind_text(insert, 2u, "one"));
    D_TESTS_MARIADB_OK(db, d_mariadb_execute_update(insert, &changes));
    D_TESTS_MARIADB_CHECK(changes == 1);

    // the same values again: the key collides
    changes = -1;
    D_TESTS_MARIADB_RESULT(db, d_mariadb_execute_update(insert, &changes),
                           1062);
    D_TESTS_MARIADB_CHECK(changes == -1);
    D_TESTS_MARIADB_CHECK(strstr(d_mariadb_error_message(db), "Duplicate"));
    D_TESTS_MARIADB_OK(db, d_mariadb_bind_int64(insert, 1u, 2));
    D_TESTS_MARIADB_OK(db, d_mariadb_bind_text(insert, 2u, longest));
    D_TESTS_MARIADB_OK(db, d_mariadb_execute_update(insert, &changes));
    D_TESTS_MARIADB_CHECK(changes == 1);

    // an UPDATE counts what it changed
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_prepare(db, &select,
                                         "UPDATE k SET v = ? WHERE id >= ?"));
    D_TESTS_MARIADB_OK(db, d_mariadb_bind_text(select, 1u, "one"));
    D_TESTS_MARIADB_OK(db, d_mariadb_bind_int64(select, 2u, 1));
    D_TESTS_MARIADB_OK(db, d_mariadb_execute_update(select, &changes));
    D_TESTS_MARIADB_CHECK(changes == 1);
    D_TESTS_MARIADB_OK(db, d_mariadb_bind_text(select, 1u, longest));
    D_TESTS_MARIADB_OK(db, d_mariadb_bind_int64(select, 2u, 2));
    D_TESTS_MARIADB_OK(db, d_mariadb_execute_update(select, &changes));
    D_TESTS_MARIADB_CHECK(changes == 1);
    D_TESTS_MARIADB_OK(db, d_mariadb_finalize(select));

    // a SELECT, counted, then collected twice
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_prepare(db, &select,
                                         "SELECT id, v FROM k WHERE id >= ? "
                                         "ORDER BY id"));
    D_TESTS_MARIADB_OK(db, d_mariadb_bind_int64(select, 1u, 1));
    D_TESTS_MARIADB_OK(db, d_mariadb_execute_update(select, &changes));
    D_TESTS_MARIADB_CHECK(changes == 2);

    for (int run = 0; run < 2; ++run)
    {
        D_TESTS_MARIADB_OK(db, d_mariadb_execute_query(select, &rows));
        D_TESTS_MARIADB_CHECK( (rows.column_count == 2u)                  &&
                               (rows.row_count == 2u)                     &&
                               (rows.values[0].as.integer == 1)           &&
                               (strcmp(rows.values[1].as.text, "one") == 0) &&
                               (rows.values[3].size ==
                                sizeof(longest) - 1u)                     &&
                               (strcmp(rows.values[3].as.text,
                                       longest) == 0) );
        d_db_rows_free(&rows);
    }

    D_TESTS_MARIADB_OK(db, d_mariadb_finalize(select));

    // a statement without rows
    D_TESTS_MARIADB_OK(db, d_mariadb_bind_int64(insert, 1u, 3));
    D_TESTS_MARIADB_OK(db, d_mariadb_execute_query(insert, &rows));
    D_TESTS_MARIADB_CHECK( (rows.column_count == 0u) &&
                           (rows.row_count == 0u) );
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT COUNT(*) FROM k")
                          == 3);
    d_db_rows_free(&rows);
    D_TESTS_MARIADB_OK(db, d_mariadb_finalize(insert));

    // a procedure answering twice
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec(db,
                                      "CREATE PROCEDURE twice(IN x INT) "
                                      "BEGIN SELECT x AS a; "
                                      "SELECT 2 AS b, 3 AS c; END"));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_prepare(db, &select, "CALL twice(?)"));
    D_TESTS_MARIADB_OK(db, d_mariadb_bind_int64(select, 1u, 5));
    D_TESTS_MARIADB_OK(db, d_mariadb_execute_query(select, &rows));
    D_TESTS_MARIADB_CHECK( (rows.column_count == 1u) &&
                           (rows.row_count == 1u)    &&
                           (rows.values[0].as.integer == 5) );
    d_db_rows_free(&rows);
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT 7") == 7);
    D_TESTS_MARIADB_OK(db, d_mariadb_execute_update(select, NULL));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT 8") == 8);
    D_TESTS_MARIADB_OK(db, d_mariadb_finalize(select));

    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_cursors
  Rows a few at a time.
  Tests the following:
  - each fetch returns the columns, named and typed as execute_query's are,
    and the next rows, no more than asked for, each value as execute_query
    reads it -- a value longer than its column's first buffer whole, and the
    values after it too
  - a short page ends the run; a fetch after is D_DB_INVALID_ARGUMENT, and
    the connection runs its next statement
  - while a run is open, everything that would send the server anything --
    a query, a script, a prepare, another statement's run, a ping, a change
    of database or character set -- is 2014, having sent nothing; values
    bound meanwhile serve the next run, which the run itself goes on without
  - rows that fill their last page exactly end with an empty page, which
    still describes the columns; a query finding no rows describes them too
  - a statement without rows runs on opening, holds the connection until its
    first fetch, which ends the run with no columns
  - running the statement again part-way through a run ends it and begins
    another; close_cursor ends one part-way, and again does nothing
  - another statement finalized part-way through a run is released, and the
    server told once the connection is free; the statement's own finalize
    ends its run
  - a CALL's every result is read at the run's end, or when it is closed
    part-way, and the connection runs its next statement
  - a row failing part-way through a run is the server's error, which the
    connection still describes; the run has ended, and the connection runs
    its next statement
*/
static void
d_tests_mariadb_cursors(
    void
)
{
    char                        schema[96] = "";
    struct d_mariadb*           db         = d_tests_mariadb_begin("cursors",
                                                                   schema,
                                                                   sizeof(
                                                                     schema));
    struct d_mariadb_statement* select     = NULL;
    struct d_mariadb_statement* other      = NULL;
    struct d_mariadb_statement* insert     = NULL;
    struct d_db_rows            all        = { 0u, 0u, NULL, NULL };
    struct d_db_rows            rows       = { 0u, 0u, NULL, NULL };
    struct d_db_rows            marker     = { 7u, 7u, NULL, NULL };
    int64_t                     changes    = -1;
    char                        before[32] = "";
    char                        after[32]  = "";
    static char                 longest[70001] = "";

    printf("mariadb_statement: rows a few at a time\n");

    memset(longest, 'x', sizeof(longest) - 1u);
    longest[sizeof(longest) - 1u] = '\0';

    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec(db,
                                      "CREATE TABLE k (id INT PRIMARY KEY, "
                                      "v MEDIUMTEXT, r DOUBLE, "
                                      "b VARBINARY(16), d DECIMAL(10,2), "
                                      "t DATETIME)"));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec(db,
                                      "INSERT INTO k VALUES "
                                      "(1, 'a', 1.5, x'00ff', 1.25, "
                                      "'2020-01-02 03:04:05'), "
                                      "(2, NULL, NULL, NULL, NULL, NULL), "
                                      "(4, 'it''s', 0, x'', -2.50, "
                                      "'1999-12-31 23:59:59'), "
                                      "(5, '', 1e300, x'61', 0.00, "
                                      "'2000-02-29 00:00:00')"));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_prepare(db, &insert,
                                         "INSERT INTO k (id, v) "
                                         "VALUES (?, ?)"));
    D_TESTS_MARIADB_OK(db, d_mariadb_bind_int64(insert, 1u, 3));
    D_TESTS_MARIADB_OK(db, d_mariadb_bind_text(insert, 2u, longest));
    D_TESTS_MARIADB_OK(db, d_mariadb_execute_update(insert, NULL));
    D_TESTS_MARIADB_OK(db, d_mariadb_finalize(insert));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_prepare(db, &select,
                                         "SELECT id, v, r, b, d, t FROM k "
                                         "WHERE id >= ? ORDER BY id"));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_prepare(db, &other,
                                         "SELECT COUNT(*) FROM k"));
    D_TESTS_MARIADB_OK(db, d_mariadb_bind_int64(select, 1u, 1));

    // the rows all at once, to hold the pages to
    D_TESTS_MARIADB_OK(db, d_mariadb_execute_query(select, &all));
    D_TESTS_MARIADB_CHECK( (all.column_count == 6u) &&
                           (all.row_count == 5u)    &&
                           (all.values[13].size == sizeof(longest) - 1u) );

    // pages of two: 1 and 2, 3 and 4, then 5 alone, which ends the run
    D_TESTS_MARIADB_OK(db, d_mariadb_open_cursor(select));
    D_TESTS_MARIADB_OK(db, d_mariadb_fetch_rows(select, 2u, &rows));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_same_page(&rows, &all, 0u, 2u));
    d_db_rows_free(&rows);
    D_TESTS_MARIADB_OK(db, d_mariadb_fetch_rows(select, 2u, &rows));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_same_page(&rows, &all, 2u, 2u));
    d_db_rows_free(&rows);
    D_TESTS_MARIADB_OK(db, d_mariadb_fetch_rows(select, 2u, &rows));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_same_page(&rows, &all, 4u, 1u));
    d_db_rows_free(&rows);
    rows = marker;
    D_TESTS_MARIADB_RESULT(db, d_mariadb_fetch_rows(select, 2u, &rows),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_CHECK( (rows.column_count == 7u) &&
                           (rows.row_count == 7u) );
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT 7") == 7);

    // part-way through, the connection is the run's
    D_TESTS_MARIADB_OK(db, d_mariadb_open_cursor(select));
    D_TESTS_MARIADB_OK(db, d_mariadb_fetch_rows(select, 1u, &rows));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_same_page(&rows, &all, 0u, 1u));
    d_db_rows_free(&rows);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_exec(db, "SELECT 1"), 2014);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_exec_script(db, "SELECT 1; SELECT 2"),
                           2014);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_execute_update(other, &changes),
                           2014);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_execute_query(other, &rows), 2014);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_open_cursor(other), 2014);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_fetch_rows(other, 1u, &rows),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_close_cursor(other), 0);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_ping(db), 2014);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_use_database(db, schema), 2014);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_set_charset(db, "utf8mb4"), 2014);
    insert = (struct d_mariadb_statement*)&g_options;
    D_TESTS_MARIADB_RESULT(db, d_mariadb_prepare(db, &insert, "SELECT 1"),
                           2014);
    D_TESTS_MARIADB_CHECK(insert == NULL);
    D_TESTS_MARIADB_CHECK(changes == -1);
    D_TESTS_MARIADB_CHECK(
        strstr(d_mariadb_result_message(2014), "holds the connection") !=
        NULL);
    D_TESTS_MARIADB_OK(db, d_mariadb_bind_int64(select, 1u, 4));
    D_TESTS_MARIADB_OK(db, d_mariadb_fetch_rows(select, 9u, &rows));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_same_page(&rows, &all, 1u, 4u));
    d_db_rows_free(&rows);
    D_TESTS_MARIADB_OK(db, d_mariadb_execute_update(other, &changes));
    D_TESTS_MARIADB_CHECK(changes == 1);

    // the value bound part-way serves the next run: from 4, two rows filling
    // a page exactly, and then an empty page, which ends the run
    D_TESTS_MARIADB_OK(db, d_mariadb_open_cursor(select));
    D_TESTS_MARIADB_OK(db, d_mariadb_fetch_rows(select, 2u, &rows));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_same_page(&rows, &all, 3u, 2u));
    d_db_rows_free(&rows);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_exec(db, "SELECT 1"), 2014);
    D_TESTS_MARIADB_OK(db, d_mariadb_fetch_rows(select, 2u, &rows));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_same_page(&rows, &all, 5u, 0u));
    D_TESTS_MARIADB_CHECK( (rows.columns != NULL) &&
                           (rows.values == NULL) );
    d_db_rows_free(&rows);
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT 7") == 7);

    // no row found: the columns alone
    D_TESTS_MARIADB_OK(db, d_mariadb_bind_int64(select, 1u, 9));
    D_TESTS_MARIADB_OK(db, d_mariadb_open_cursor(select));
    D_TESTS_MARIADB_OK(db, d_mariadb_fetch_rows(select, 4u, &rows));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_same_page(&rows, &all, 5u, 0u));
    d_db_rows_free(&rows);

    // a statement without rows runs on opening, and holds the connection
    // until its first fetch
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_prepare(db, &insert,
                                         "INSERT INTO k (id) VALUES (?)"));
    D_TESTS_MARIADB_OK(db, d_mariadb_bind_int64(insert, 1u, 6));
    D_TESTS_MARIADB_OK(db, d_mariadb_open_cursor(insert));
    D_TESTS_MARIADB_RESULT(db, d_mariadb_exec(db, "SELECT 1"), 2014);
    rows = marker;
    D_TESTS_MARIADB_OK(db, d_mariadb_fetch_rows(insert, 3u, &rows));
    D_TESTS_MARIADB_CHECK( (rows.column_count == 0u) &&
                           (rows.row_count == 0u)    &&
                           (rows.columns == NULL) );
    D_TESTS_MARIADB_RESULT(db, d_mariadb_fetch_rows(insert, 3u, &rows),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT COUNT(*) FROM k")
                          == 6);
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "DELETE FROM k WHERE id = 6"));
    D_TESTS_MARIADB_OK(db, d_mariadb_finalize(insert));

    // running the statement again part-way: its run ends, another begins
    D_TESTS_MARIADB_OK(db, d_mariadb_bind_int64(select, 1u, 1));
    D_TESTS_MARIADB_OK(db, d_mariadb_open_cursor(select));
    D_TESTS_MARIADB_OK(db, d_mariadb_fetch_rows(select, 2u, &rows));
    d_db_rows_free(&rows);
    D_TESTS_MARIADB_OK(db, d_mariadb_execute_query(select, &rows));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_same_page(&rows, &all, 0u, 5u));
    d_db_rows_free(&rows);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_fetch_rows(select, 2u, &rows),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_OK(db, d_mariadb_open_cursor(select));
    D_TESTS_MARIADB_OK(db, d_mariadb_fetch_rows(select, 1u, &rows));
    d_db_rows_free(&rows);
    D_TESTS_MARIADB_OK(db, d_mariadb_open_cursor(select));
    D_TESTS_MARIADB_OK(db, d_mariadb_fetch_rows(select, 2u, &rows));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_same_page(&rows, &all, 0u, 2u));
    d_db_rows_free(&rows);

    // close_cursor part-way, and again
    D_TESTS_MARIADB_OK(db, d_mariadb_close_cursor(select));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT 7") == 7);
    D_TESTS_MARIADB_OK(db, d_mariadb_close_cursor(select));
    D_TESTS_MARIADB_RESULT(db, d_mariadb_fetch_rows(select, 1u, &rows),
                           D_DB_INVALID_ARGUMENT);

    // another statement finalized part-way: the server is told once the
    // connection is free
    d_tests_mariadb_status(db, "Com_stmt_close", before, sizeof(before));
    D_TESTS_MARIADB_OK(db, d_mariadb_open_cursor(select));
    D_TESTS_MARIADB_OK(db, d_mariadb_fetch_rows(select, 1u, &rows));
    d_db_rows_free(&rows);
    D_TESTS_MARIADB_OK(db, d_mariadb_finalize(other));
    D_TESTS_MARIADB_OK(db, d_mariadb_fetch_rows(select, 9u, &rows));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_same_page(&rows, &all, 1u, 4u));
    d_db_rows_free(&rows);
    d_tests_mariadb_status(db, "Com_stmt_close", after, sizeof(after));
    D_TESTS_MARIADB_CHECK(strtoul(after, NULL, 10) ==
                          strtoul(before, NULL, 10) + 1ul);

    // the statement's own finalize ends its run
    D_TESTS_MARIADB_OK(db, d_mariadb_open_cursor(select));
    D_TESTS_MARIADB_OK(db, d_mariadb_fetch_rows(select, 1u, &rows));
    d_db_rows_free(&rows);
    D_TESTS_MARIADB_OK(db, d_mariadb_finalize(select));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT 7") == 7);

    // a procedure answering twice, read to its end, and closed part-way
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec(db,
                                      "CREATE PROCEDURE twice(IN x INT) "
                                      "BEGIN SELECT id FROM k WHERE id >= x "
                                      "ORDER BY id; "
                                      "SELECT 2 AS b, 3 AS c; END"));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_prepare(db, &select, "CALL twice(?)"));
    D_TESTS_MARIADB_OK(db, d_mariadb_bind_int64(select, 1u, 1));
    D_TESTS_MARIADB_OK(db, d_mariadb_open_cursor(select));
    D_TESTS_MARIADB_OK(db, d_mariadb_fetch_rows(select, 4u, &rows));
    D_TESTS_MARIADB_CHECK( (rows.column_count == 1u) &&
                           (rows.row_count == 4u) );
    d_db_rows_free(&rows);
    D_TESTS_MARIADB_OK(db, d_mariadb_fetch_rows(select, 4u, &rows));
    D_TESTS_MARIADB_CHECK( (rows.row_count == 1u) &&
                           (rows.values[0].as.integer == 5) );
    d_db_rows_free(&rows);
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT 7") == 7);
    D_TESTS_MARIADB_OK(db, d_mariadb_open_cursor(select));
    D_TESTS_MARIADB_OK(db, d_mariadb_fetch_rows(select, 1u, &rows));
    d_db_rows_free(&rows);
    D_TESTS_MARIADB_OK(db, d_mariadb_close_cursor(select));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT 8") == 8);
    D_TESTS_MARIADB_OK(db, d_mariadb_open_cursor(select));
    D_TESTS_MARIADB_OK(db, d_mariadb_execute_update(select, NULL));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT 9") == 9);
    D_TESTS_MARIADB_OK(db, d_mariadb_finalize(select));

    // a row failing part-way: the third's subquery finds three rows
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_prepare(db, &select,
                                         "SELECT id, IF(id < 3, id, "
                                         "(SELECT k2.id FROM k AS k2 "
                                         "WHERE k2.id >= k.id)) FROM k "
                                         "ORDER BY id"));
    D_TESTS_MARIADB_OK(db, d_mariadb_open_cursor(select));
    D_TESTS_MARIADB_OK(db, d_mariadb_fetch_rows(select, 2u, &rows));
    D_TESTS_MARIADB_CHECK( (rows.row_count == 2u) &&
                           (rows.values[3].as.integer == 2) );
    d_db_rows_free(&rows);
    rows = marker;
    D_TESTS_MARIADB_RESULT(db, d_mariadb_fetch_rows(select, 2u, &rows),
                           1242);
    D_TESTS_MARIADB_CHECK( (rows.column_count == 7u) &&
                           (rows.row_count == 7u) );
    D_TESTS_MARIADB_CHECK(strstr(d_mariadb_error_message(db), "1 row") !=
                          NULL);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_fetch_rows(select, 2u, &rows),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT 7") == 7);
    D_TESTS_MARIADB_OK(db, d_mariadb_finalize(select));

    d_db_rows_free(&all);
    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_cursor_closing
  A run open when its connection closes.
  Tests the following:
  - closing the connection ends the run; the statement then has no
    connection, refuses to run with D_DB_INVALID_ARGUMENT, closes no cursor,
    and finalizes; so does one finalized while the run held the connection
  - a connection the handle does not own is handed back with the run read to
    its end, so that the program's next query runs
*/
static void
d_tests_mariadb_cursor_closing(
    void
)
{
    struct d_mariadb*           db       = d_tests_mariadb_connect(NULL);
    struct d_mariadb*           adopted  = NULL;
    struct d_mariadb_statement* running  = NULL;
    struct d_mariadb_statement* retiring = NULL;
    struct d_db_rows            rows     = { 0u, 0u, NULL, NULL };
    int64_t                     changes  = 0;
    MYSQL* const                native   = d_tests_mariadb_native();

    printf("mariadb_statement: a run open at closing\n");

    D_TESTS_MARIADB_OK(db,
                       d_mariadb_prepare(db, &running,
                                         "SELECT seq FROM (SELECT 1 AS seq "
                                         "UNION ALL SELECT 2 UNION ALL "
                                         "SELECT 3) AS t ORDER BY seq"));
    D_TESTS_MARIADB_OK(db, d_mariadb_prepare(db, &retiring, "SELECT 1"));
    D_TESTS_MARIADB_OK(db, d_mariadb_open_cursor(running));
    D_TESTS_MARIADB_OK(db, d_mariadb_fetch_rows(running, 1u, &rows));
    d_db_rows_free(&rows);
    D_TESTS_MARIADB_OK(db, d_mariadb_finalize(retiring));
    D_TESTS_MARIADB_OK(db, d_mariadb_close(db));
    D_TESTS_MARIADB_CHECK(d_mariadb_statement_connection(running) == NULL);
    D_TESTS_MARIADB_RESULT(NULL, d_mariadb_fetch_rows(running, 1u, &rows),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(NULL, d_mariadb_open_cursor(running),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(NULL, d_mariadb_execute_query(running, &rows),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(NULL, d_mariadb_execute_update(running, &changes),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_OK(NULL, d_mariadb_close_cursor(running));
    D_TESTS_MARIADB_OK(NULL, d_mariadb_finalize(running));

    // a connection the program keeps
    D_TESTS_MARIADB_CHECK(
        (native) &&
        (mysql_real_connect(native,
                            g_options.host,
                            g_options.user,
                            g_options.password,
                            NULL,
                            g_options.port,
                            g_options.unix_socket,
                            0ul) != NULL));
    D_TESTS_MARIADB_OK(NULL, d_mariadb_adopt(native, false, &adopted));
    D_TESTS_MARIADB_OK(adopted,
                       d_mariadb_prepare(adopted, &running,
                                         "SELECT seq FROM (SELECT 1 AS seq "
                                         "UNION ALL SELECT 2 UNION ALL "
                                         "SELECT 3) AS t ORDER BY seq"));
    D_TESTS_MARIADB_OK(adopted, d_mariadb_open_cursor(running));
    D_TESTS_MARIADB_OK(adopted, d_mariadb_fetch_rows(running, 1u, &rows));
    d_db_rows_free(&rows);
    D_TESTS_MARIADB_OK(adopted, d_mariadb_close(adopted));
    D_TESTS_MARIADB_CHECK( (native) &&
                           (mysql_query(native, "SELECT 1") == 0) );

    // the program's query answered, then the statement told the server
    if (native)
    {
        MYSQL_RES* const result = mysql_store_result(native);

        D_TESTS_MARIADB_CHECK(result != NULL);

        if (result)
        {
            mysql_free_result(result);
        }
    }

    D_TESTS_MARIADB_OK(NULL, d_mariadb_finalize(running));
    D_TESTS_MARIADB_CHECK( (native) && (mysql_ping(native) == 0) );

    if (native)
    {
        mysql_close(native);
    }

    return;
}

/*
d_tests_mariadb_statement_arguments
  Argument checks for statements.
  Tests the following:
  - a NULL connection, statement pointer, text or format is
    D_DB_INVALID_ARGUMENT, the statement left NULL where there is one
  - every bind, clear and run refuses a NULL statement, as execute_query
    refuses NULL rows; the describers answer 0 or NULL
  - the cursor calls refuse a NULL statement, and fetch_rows NULL rows, none
    asked for, or no run open, the rows untouched
*/
static void
d_tests_mariadb_statement_arguments(
    void
)
{
    struct d_mariadb* const     db        = d_tests_mariadb_connect(NULL);
    struct d_mariadb_statement* statement = NULL;
    struct d_db_rows            rows      = { 0u, 0u, NULL, NULL };
    struct d_db_rows            marker    = { 7u, 7u, NULL, NULL };
    int64_t                     changes   = 0;

    printf("mariadb_statement: arguments\n");

    statement = (struct d_mariadb_statement*)&g_options;
    D_TESTS_MARIADB_RESULT(db, d_mariadb_prepare(NULL, &statement,
                                                 "SELECT 1"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_CHECK(statement == NULL);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_prepare(db, NULL, "SELECT 1"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_prepare(db, &statement, NULL),
                           D_DB_INVALID_ARGUMENT);
    statement = (struct d_mariadb_statement*)&g_options;
    D_TESTS_MARIADB_RESULT(db, d_mariadb_preparef(NULL, &statement,
                                                  "SELECT 1"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_CHECK(statement == NULL);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_preparef(db, NULL, "SELECT 1"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_preparef(db, &statement, NULL),
                           D_DB_INVALID_ARGUMENT);

    D_TESTS_MARIADB_CHECK(d_mariadb_parameter_count(NULL) == 0u);
    D_TESTS_MARIADB_CHECK(d_mariadb_statement_connection(NULL) == NULL);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_bind_null(NULL, 1u),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_bind_bool(NULL, 1u, true),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_bind_int64(NULL, 1u, 1),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_bind_double(NULL, 1u, 1.0),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_bind_text(NULL, 1u, "x"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_bind_blob(NULL, 1u, "x", 1u),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_clear_bindings(NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_execute_update(NULL, &changes),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_execute_query(NULL, &rows),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_open_cursor(NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_fetch_rows(NULL, 1u, &rows),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_close_cursor(NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_OK(db, d_mariadb_prepare(db, &statement, "SELECT 1"));
    D_TESTS_MARIADB_RESULT(db, d_mariadb_execute_query(statement, NULL),
                           D_DB_INVALID_ARGUMENT);
    rows = marker;
    D_TESTS_MARIADB_RESULT(db, d_mariadb_fetch_rows(statement, 1u, &rows),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_OK(db, d_mariadb_open_cursor(statement));
    D_TESTS_MARIADB_RESULT(db, d_mariadb_fetch_rows(statement, 0u, &rows),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_fetch_rows(statement, 1u, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_CHECK( (rows.column_count == 7u) &&
                           (rows.row_count == 7u) );
    D_TESTS_MARIADB_OK(db, d_mariadb_close_cursor(statement));
    D_TESTS_MARIADB_OK(db, d_mariadb_finalize(statement));

    d_mariadb_close(db);

    return;
}

//==============================================================================
// mariadb_database.h
//==============================================================================

/*
d_tests_mariadb_databases
  Databases.
  Tests the following:
  - has, create, create again (1007), IF NOT EXISTS, drop, drop again (1008),
    IF EXISTS
  - a created database takes the layer's character set
  - the list holds it and the server's own, sorted
*/
static void
d_tests_mariadb_databases(
    void
)
{
    struct d_mariadb* const db       = d_tests_mariadb_connect(NULL);
    struct d_db_name_list   names    = { 0u, NULL };
    char                    name[96] = "";
    bool                    exists   = true;

    printf("mariadb_database: databases\n");

    d_tests_mariadb_schema_name(name, sizeof(name), "dbs");
    D_TESTS_MARIADB_OK(db, d_mariadb_drop_database(db, name, D_DB_IF_EXISTS));
    D_TESTS_MARIADB_OK(db, d_mariadb_has_database_s(db, name, &exists));
    D_TESTS_MARIADB_CHECK(!exists);
    D_TESTS_MARIADB_OK(db, d_mariadb_create_database(db, name, 0u));
    D_TESTS_MARIADB_CHECK(d_mariadb_has_database(db, name));
    D_TESTS_MARIADB_RESULT(db, d_mariadb_create_database(db, name, 0u), 1007);
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_create_database(db,
                                                 name,
                                                 D_DB_IF_NOT_EXISTS));

    // the layer's character set, unless the build left it to the server
    if (D_CFG_MARIADB_CHARSET[0] != '\0')
    {
        char* charset = NULL;

        D_TESTS_MARIADB_OK(db,
                           d_mariadb_query_textf(
                               db,
                               &charset,
                               "SELECT DEFAULT_CHARACTER_SET_NAME FROM "
                               "information_schema.SCHEMATA "
                               "WHERE SCHEMA_NAME = %Q",
                               name));
        D_TESTS_MARIADB_CHECK( (charset) &&
                               (strcmp(charset, D_CFG_MARIADB_CHARSET) == 0) );
        d_db_free(charset);
    }

    // listed, among the server's own, in order
    D_TESTS_MARIADB_OK(db, d_mariadb_list_databases(db, &names));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_has_name(&names, name));
    D_TESTS_MARIADB_CHECK(
        d_tests_mariadb_has_name(&names, "information_schema"));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_sorted(&names));
    d_db_name_list_free(&names);

    // dropped
    D_TESTS_MARIADB_OK(db, d_mariadb_drop_database(db, name, 0u));
    D_TESTS_MARIADB_CHECK(!d_mariadb_has_database(db, name));
    D_TESTS_MARIADB_RESULT(db, d_mariadb_drop_database(db, name, 0u), 1008);
    D_TESTS_MARIADB_OK(db, d_mariadb_drop_database(db, name, D_DB_IF_EXISTS));

    d_mariadb_close(db);

    return;
}

/*
d_tests_mariadb_current_database
  The current database.
  Tests the following:
  - names match as the server matches them
  - use_database and current_database; none current is D_DB_NOT_FOUND, as is
    a current one dropped; one not there is 1049
*/
static void
d_tests_mariadb_current_database(
    void
)
{
    char              name[96]  = "";
    struct d_mariadb* db        = d_tests_mariadb_begin("current",
                                                        name,
                                                        sizeof(name));
    struct d_mariadb* elsewhere = d_tests_mariadb_connect(NULL);
    char              upper[96] = "";
    char*             current   = NULL;

    printf("mariadb_database: the current database\n");

    // by case, where the server matches by case
    for (size_t i = 0u; name[i] != '\0'; ++i)
    {
        upper[i] = ( (name[i] >= 'a') && (name[i] <= 'z') )
                       ? (char)(name[i] - 'a' + 'A')
                       : name[i];
    }

    D_TESTS_MARIADB_CHECK(d_mariadb_has_database(db, upper) ==
                          d_tests_mariadb_case_blind(db));

    // chosen and named
    D_TESTS_MARIADB_RESULT(elsewhere,
                           d_mariadb_current_database(elsewhere, &current),
                           D_DB_NOT_FOUND);
    D_TESTS_MARIADB_OK(elsewhere, d_mariadb_use_database(elsewhere, name));
    D_TESTS_MARIADB_OK(elsewhere,
                       d_mariadb_current_database(elsewhere, &current));
    D_TESTS_MARIADB_CHECK( (current) && (strcmp(current, name) == 0) );
    d_db_free(current);
    current = NULL;
    D_TESTS_MARIADB_RESULT(elsewhere,
                           d_mariadb_use_database(elsewhere,
                                                  "d_tests_mariadb_no_such"),
                           1049);

    // dropped while current
    D_TESTS_MARIADB_OK(elsewhere, d_mariadb_drop_database(elsewhere, name, 0u));
    D_TESTS_MARIADB_RESULT(elsewhere,
                           d_mariadb_current_database(elsewhere, &current),
                           D_DB_NOT_FOUND);

    d_mariadb_close(elsewhere);
    d_tests_mariadb_end(db, name);

    return;
}

/*
d_tests_mariadb_database_arguments
  Argument checks for databases.
  Tests the following:
  - NULL handles, NULL and empty names, NULL outputs
  - flags a function does not take
*/
static void
d_tests_mariadb_database_arguments(
    void
)
{
    struct d_mariadb* const db      = d_tests_mariadb_connect(NULL);
    struct d_db_name_list   names   = { 0u, NULL };
    bool                    exists  = true;
    char*                   current = NULL;

    printf("mariadb_database: arguments\n");

    D_TESTS_MARIADB_CHECK(!d_mariadb_has_database(NULL, "mysql"));
    D_TESTS_MARIADB_CHECK(!d_mariadb_has_database(db, NULL));
    D_TESTS_MARIADB_RESULT(db, d_mariadb_has_database_s(db, "", &exists),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_CHECK(!exists);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_has_database_s(db, "mysql", NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_create_database(NULL, "x", 0u),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_create_database(db, "", 0u),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_create_database(db, "x", D_DB_IF_EXISTS),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_drop_database(db, NULL, 0u),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_drop_database(db,
                                                   "x",
                                                   D_DB_IF_NOT_EXISTS),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_list_databases(NULL, &names),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_list_databases(db, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_use_database(db, ""),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_use_database(NULL, "mysql"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_current_database(db, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_current_database(NULL, &current),
                           D_DB_INVALID_ARGUMENT);

    d_mariadb_close(db);

    return;
}

//==============================================================================
// mariadb_schema.h -- tables
//==============================================================================

/*
d_tests_mariadb_table_existence
  Whether a table exists.
  Tests the following:
  - a base table is one; a view and a temporary table are not
  - in the current database, a named one, and one that does not exist
  - names match as the server matches them: two tables whose names differ
    only by case are two, where the server matches by case
*/
static void
d_tests_mariadb_table_existence(
    void
)
{
    char              schema[96] = "";
    struct d_mariadb* db         = d_tests_mariadb_begin("existence",
                                                         schema,
                                                         sizeof(schema));
    struct d_mariadb* elsewhere  = d_tests_mariadb_connect(NULL);
    bool              exists     = false;

    printf("mariadb_schema: table existence\n");

    D_TESTS_MARIADB_CHECK(!d_mariadb_has_table(db, NULL, "t"));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_create_table_sql(db,
                                                  NULL,
                                                  "t",
                                                  "id INT PRIMARY KEY",
                                                  0u));
    D_TESTS_MARIADB_CHECK(d_mariadb_has_table(db, NULL, "t"));
    D_TESTS_MARIADB_OK(db, d_mariadb_has_table_s(db, NULL, "t", &exists));
    D_TESTS_MARIADB_CHECK(exists);
    D_TESTS_MARIADB_CHECK(d_mariadb_has_table(elsewhere, schema, "t"));
    D_TESTS_MARIADB_CHECK(!d_mariadb_has_table(elsewhere, NULL, "t"));
    D_TESTS_MARIADB_CHECK(
        !d_mariadb_has_table(db, "d_tests_mariadb_no_such", "t"));

    // a view and a temporary table are not tables
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "CREATE VIEW v AS SELECT 1"));
    D_TESTS_MARIADB_CHECK(!d_mariadb_has_table(db, NULL, "v"));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec(db,
                                      "CREATE TEMPORARY TABLE tmp (a INT)"));
    D_TESTS_MARIADB_CHECK(!d_mariadb_has_table(db, NULL, "tmp"));

    // case
    if (!d_tests_mariadb_case_blind(db))
    {
        D_TESTS_MARIADB_CHECK(!d_mariadb_has_table(db, NULL, "T"));
        D_TESTS_MARIADB_OK(db,
                           d_mariadb_exec(db, "CREATE TABLE Mixed (a INT)"));
        D_TESTS_MARIADB_OK(db,
                           d_mariadb_exec(db, "CREATE TABLE mixed (b INT)"));
        D_TESTS_MARIADB_CHECK(d_mariadb_has_table(db, NULL, "Mixed"));
        D_TESTS_MARIADB_CHECK(d_mariadb_has_table(db, NULL, "mixed"));
        D_TESTS_MARIADB_CHECK(!d_mariadb_has_table(db, NULL, "MIXED"));
    }
    else
    {
        D_TESTS_MARIADB_CHECK(d_mariadb_has_table(db, NULL, "T"));
    }

    d_mariadb_close(elsewhere);
    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_table_creation
  Creating tables.
  Tests the following:
  - create again is 1050, and IF NOT EXISTS a success
  - a table with no database current and none named is 1046; one named is
    made there
*/
static void
d_tests_mariadb_table_creation(
    void
)
{
    char              schema[96] = "";
    struct d_mariadb* db         = d_tests_mariadb_begin("creation",
                                                         schema,
                                                         sizeof(schema));
    struct d_mariadb* elsewhere  = d_tests_mariadb_connect(NULL);

    printf("mariadb_schema: table creation\n");

    D_TESTS_MARIADB_OK(db,
                       d_mariadb_create_table_sql(db, NULL, "t", "a INT", 0u));
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_create_table_sql(db,
                                                      NULL,
                                                      "t",
                                                      "a INT",
                                                      0u),
                           1050);
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_create_table_sql(db,
                                                  NULL,
                                                  "t",
                                                  "different INT",
                                                  D_DB_IF_NOT_EXISTS));
    D_TESTS_MARIADB_CHECK(!d_mariadb_has_column(db, NULL, "t", "different"));

    // no database current
    D_TESTS_MARIADB_RESULT(elsewhere,
                           d_mariadb_create_table_sql(elsewhere,
                                                      NULL,
                                                      "u",
                                                      "a INT",
                                                      0u),
                           1046);
    D_TESTS_MARIADB_OK(elsewhere,
                       d_mariadb_create_table_sql(elsewhere,
                                                  schema,
                                                  "u",
                                                  "a INT",
                                                  0u));
    D_TESTS_MARIADB_CHECK(d_mariadb_has_table(db, NULL, "u"));

    d_mariadb_close(elsewhere);
    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_table_renames
  Renaming and dropping tables.
  Tests the following:
  - rename moves a table; a missing one is 1146, a taken name 1050
  - drop of a missing table is 1051, IF EXISTS a success
*/
static void
d_tests_mariadb_table_renames(
    void
)
{
    char              schema[96] = "";
    struct d_mariadb* db         = d_tests_mariadb_begin("renames",
                                                         schema,
                                                         sizeof(schema));
    struct d_mariadb* elsewhere  = d_tests_mariadb_connect(NULL);

    printf("mariadb_schema: table renames and drops\n");

    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec_script(db,
                                             "CREATE TABLE t (a INT); "
                                             "CREATE TABLE u (a INT)"));
    D_TESTS_MARIADB_OK(db, d_mariadb_rename_table(db, NULL, "t", "t2"));
    D_TESTS_MARIADB_CHECK(!d_mariadb_has_table(db, NULL, "t"));
    D_TESTS_MARIADB_CHECK(d_mariadb_has_table(db, NULL, "t2"));
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_rename_table(db, NULL, "t", "t3"),
                           1146);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_rename_table(elsewhere, schema, "t2", "u"),
                           1050);

    D_TESTS_MARIADB_OK(db, d_mariadb_drop_table(db, NULL, "t2", 0u));
    D_TESTS_MARIADB_CHECK(!d_mariadb_has_table(db, NULL, "t2"));
    D_TESTS_MARIADB_RESULT(db, d_mariadb_drop_table(db, NULL, "t2", 0u), 1051);
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_drop_table(db, NULL, "t2", D_DB_IF_EXISTS));
    D_TESTS_MARIADB_OK(elsewhere,
                       d_mariadb_drop_table(elsewhere, schema, "u", 0u));
    D_TESTS_MARIADB_CHECK(!d_mariadb_has_table(db, NULL, "u"));

    d_mariadb_close(elsewhere);
    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_table_rows
  Counting and emptying.
  Tests the following:
  - count_rows counts; a missing table is 1146, the count untouched
  - truncate empties a table and restarts its AUTO_INCREMENT
*/
static void
d_tests_mariadb_table_rows(
    void
)
{
    char              schema[96] = "";
    struct d_mariadb* db         = d_tests_mariadb_begin("rows",
                                                         schema,
                                                         sizeof(schema));
    int64_t           count      = -1;

    printf("mariadb_schema: table rows\n");

    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec_script(db,
                                             "CREATE TABLE r (id INT "
                                             "AUTO_INCREMENT PRIMARY KEY, "
                                             "v INT); "
                                             "INSERT INTO r (v) "
                                             "VALUES (1), (2), (3)"));
    D_TESTS_MARIADB_OK(db, d_mariadb_count_rows(db, NULL, "r", &count));
    D_TESTS_MARIADB_CHECK(count == 3);
    D_TESTS_MARIADB_OK(db, d_mariadb_count_rows(db, schema, "r", &count));
    D_TESTS_MARIADB_CHECK(count == 3);
    count = -1;
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_count_rows(db, NULL, "none", &count),
                           1146);
    D_TESTS_MARIADB_CHECK(count == -1);

    D_TESTS_MARIADB_OK(db, d_mariadb_truncate_table(db, NULL, "r"));
    D_TESTS_MARIADB_OK(db, d_mariadb_count_rows(db, NULL, "r", &count));
    D_TESTS_MARIADB_CHECK(count == 0);
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "INSERT INTO r (v) VALUES (4)"));
    D_TESTS_MARIADB_CHECK(d_mariadb_last_insert_id(db) == 1u);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_truncate_table(db, NULL, "none"),
                           1146);

    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_table_listing
  Listing tables.
  Tests the following:
  - base tables only, in byte order, from the current database or a named
    one; a database that does not exist lists nothing
*/
static void
d_tests_mariadb_table_listing(
    void
)
{
    char                  schema[96] = "";
    struct d_mariadb*     db         = d_tests_mariadb_begin("listing",
                                                             schema,
                                                             sizeof(schema));
    struct d_db_name_list names      = { 0u, NULL };

    printf("mariadb_schema: table listing\n");

    D_TESTS_MARIADB_OK(db, d_mariadb_list_tables(db, NULL, &names));
    D_TESTS_MARIADB_CHECK( (names.count == 0u) && (names.names != NULL) &&
                           (names.names[0] == NULL) );
    d_db_name_list_free(&names);

    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec_script(db,
                                             "CREATE TABLE b (a INT); "
                                             "CREATE TABLE a (a INT); "
                                             "CREATE TABLE B2 (a INT); "
                                             "CREATE VIEW c AS SELECT 1"));
    D_TESTS_MARIADB_OK(db, d_mariadb_list_tables(db, NULL, &names));
    D_TESTS_MARIADB_CHECK(names.count == 3u);
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_has_name(&names, "a"));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_has_name(&names, "b"));
    D_TESTS_MARIADB_CHECK(!d_tests_mariadb_has_name(&names, "c"));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_sorted(&names));
    D_TESTS_MARIADB_CHECK( (names.count == 3u) &&
                           ( (d_tests_mariadb_case_blind(db)) ||
                             (strcmp(names.names[0], "B2") == 0) ) );
    d_db_name_list_free(&names);

    D_TESTS_MARIADB_OK(db, d_mariadb_list_tables(db, schema, &names));
    D_TESTS_MARIADB_CHECK(names.count == 3u);
    d_db_name_list_free(&names);
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_list_tables(db,
                                             "d_tests_mariadb_no_such",
                                             &names));
    D_TESTS_MARIADB_CHECK(names.count == 0u);
    d_db_name_list_free(&names);

    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_table_arguments
  Argument checks for tables.
  Tests the following:
  - NULL handles, empty schemas, NULL and empty names, NULL outputs
  - flags a function does not take
*/
static void
d_tests_mariadb_table_arguments(
    void
)
{
    struct d_mariadb* const db     = d_tests_mariadb_connect(NULL);
    struct d_db_name_list   names  = { 0u, NULL };
    int64_t                 count  = 0;
    bool                    exists = true;

    printf("mariadb_schema: table arguments\n");

    D_TESTS_MARIADB_CHECK(!d_mariadb_has_table(NULL, NULL, "t"));
    D_TESTS_MARIADB_RESULT(db, d_mariadb_has_table_s(db, "", "t", &exists),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_CHECK(!exists);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_has_table_s(db, NULL, "", &exists),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_has_table_s(db, NULL, "t", NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_drop_table(db, NULL, "t",
                                                D_DB_IF_NOT_EXISTS),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_rename_table(db, NULL, "t", ""),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_truncate_table(db, "", "t"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_count_rows(db, NULL, "t", NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_count_rows(NULL, NULL, "t", &count),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_list_tables(db, "", &names),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_list_tables(db, NULL, NULL),
                           D_DB_INVALID_ARGUMENT);

    d_mariadb_close(db);

    return;
}

/*
d_tests_mariadb_creation_arguments
  Argument checks for creating tables.
  Tests the following:
  - no columns, a NULL column array, an empty definition, a NULL handle
  - flags CREATE TABLE does not take
*/
static void
d_tests_mariadb_creation_arguments(
    void
)
{
    struct d_mariadb* const db = d_tests_mariadb_connect(NULL);

    printf("mariadb_schema: table creation arguments\n");

    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_create_table(db, NULL, "t",
                                                  d_tests_mariadb_composite,
                                                  0u, 0u),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_create_table(db, NULL, "t", NULL, 1u, 0u),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_create_table(db, NULL, "t",
                                                  d_tests_mariadb_composite,
                                                  3u, D_DB_IF_EXISTS),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_create_table_sql(db, NULL, "t", "", 0u),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_create_table_sql(NULL, NULL, "t",
                                                      "a INT", 0u),
                           D_DB_INVALID_ARGUMENT);

    d_mariadb_close(db);

    return;
}

//==============================================================================
// mariadb_schema.h -- described tables and columns
//==============================================================================

/*
d_tests_mariadb_type_expected
  The type a server reports for a column of d_tests_mariadb_every_type.
*/
static const char*
d_tests_mariadb_type_expected(
    const struct d_mariadb* _db,
    size_t                  _column
)
{
    return (d_tests_mariadb_is_mariadb(_db))
               ? d_tests_mariadb_every_type_sql[_column][0]
           : (d_tests_mariadb_mysql_at_least(_db, 8u, 0u, 19u))
               ? d_tests_mariadb_every_type_sql[_column][2]
               : d_tests_mariadb_every_type_sql[_column][1];
}

/*
d_tests_mariadb_make_every
  A database of the test's own holding d_tests_mariadb_every_type's table,
"every", and a connection with it current.
*/
static struct d_mariadb*
d_tests_mariadb_make_every(
    const char* _word,
    char*       _schema,
    size_t      _size
)
{
    struct d_mariadb* db = d_tests_mariadb_begin(_word, _schema, _size);

    D_TESTS_MARIADB_OK(db,
                       d_mariadb_create_table(
                           db,
                           NULL,
                           "every",
                           d_tests_mariadb_every_type,
                           sizeof(d_tests_mariadb_every_type) /
                               sizeof(d_tests_mariadb_every_type[0]),
                           0u));

    return db;
}

/*
d_tests_mariadb_described_types
  A table made from column descriptions, every type among them.
  Tests the following:
  - each column, in order, declared as the mapping says, as the server
    reports it
*/
static void
d_tests_mariadb_described_types(
    void
)
{
    const size_t            count      = sizeof(d_tests_mariadb_every_type) /
                                         sizeof(d_tests_mariadb_every_type[0]);
    char                    schema[96] = "";
    struct d_mariadb*       db         = d_tests_mariadb_make_every(
                                             "types",
                                             schema,
                                             sizeof(schema));
    struct d_db_column_list columns    = { 0u, NULL };

    printf("mariadb_schema: described types\n");

    D_TESTS_MARIADB_OK(db, d_mariadb_list_columns(db, NULL, "every", &columns));
    D_TESTS_MARIADB_CHECK(columns.count == count);

    for (size_t i = 0u; (i < count) && (i < columns.count); ++i)
    {
        const char* const expected = d_tests_mariadb_type_expected(db, i);

        // the column, in order, with its type
        D_TESTS_MARIADB_CHECK(
            strcmp(columns.columns[i].name,
                   d_tests_mariadb_every_type[i].name) == 0);

        if (strcmp(columns.columns[i].type, expected) != 0)
        {
            printf("    column %s: %s, expected %s\n",
                   columns.columns[i].name,
                   columns.columns[i].type,
                   expected);
        }

        D_TESTS_MARIADB_CHECK(strcmp(columns.columns[i].type, expected) == 0);
    }

    d_db_column_list_free(&columns);
    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_described_attributes
  The attributes of a table made from column descriptions.
  Tests the following:
  - NOT NULL, the key, AUTO_INCREMENT and the defaults hold
  - a nullable TIMESTAMP gets no default of the server's own
  - a row of defaults inserts, its key drawn; the unique column refuses a
    second
*/
static void
d_tests_mariadb_described_attributes(
    void
)
{
    char                    schema[96] = "";
    struct d_mariadb*       db         = d_tests_mariadb_make_every(
                                             "attributes",
                                             schema,
                                             sizeof(schema));
    struct d_db_column_list columns    = { 0u, NULL };

    printf("mariadb_schema: described attributes\n");

    D_TESTS_MARIADB_OK(db, d_mariadb_list_columns(db, NULL, "every", &columns));

    // the key, NOT NULL and the defaults, column by column
    if (columns.count > 14u)
    {
        D_TESTS_MARIADB_CHECK(columns.columns[0].primary_key == 1);
        D_TESTS_MARIADB_CHECK(columns.columns[0].not_null);
        D_TESTS_MARIADB_CHECK(columns.columns[1].not_null);
        D_TESTS_MARIADB_CHECK(!columns.columns[2].not_null);
        D_TESTS_MARIADB_CHECK( (columns.columns[2].default_sql) &&
                               (strcmp(columns.columns[2].default_sql,
                                       "0") == 0) );
        D_TESTS_MARIADB_CHECK( (columns.columns[4].default_sql) &&
                               (strcmp(columns.columns[4].default_sql,
                                       "1.50") == 0) );
        D_TESTS_MARIADB_CHECK( (columns.columns[6].default_sql) &&
                               (strcmp(columns.columns[6].default_sql,
                                       "'none'") == 0) );
        D_TESTS_MARIADB_CHECK(columns.columns[3].default_sql == NULL);
        D_TESTS_MARIADB_CHECK(columns.columns[14].default_sql == NULL);
        D_TESTS_MARIADB_CHECK(!columns.columns[14].not_null);
    }

    d_db_column_list_free(&columns);

    // a row of defaults, its key drawn, and the unique column refusing twice
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec(db, "INSERT INTO every () VALUES ()"));
    D_TESTS_MARIADB_CHECK(d_mariadb_last_insert_id(db) == 1u);
    D_TESTS_MARIADB_CHECK(
        d_tests_mariadb_text_is(db, "SELECT name FROM every", "none"));
    D_TESTS_MARIADB_CHECK(
        d_tests_mariadb_text_is(db, "SELECT stamp FROM every", NULL));
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_exec(db, "INSERT INTO every () VALUES ()"),
                           1062);
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec(db,
                                      "INSERT INTO every (name, uuid, doc) "
                                      "VALUES ('second', UUID(), "
                                      "'{\"a\": 1}')"));

    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_described_keys
  Keys from column descriptions.
  Tests the following:
  - a composite key, each column its place in it, in the table's order
  - key places order the key otherwise than the table; places that repeat,
    skip, number a column off the key, or leave a key column out are
    D_DB_INVALID_ARGUMENT, and make nothing
  - AUTO_INCREMENT on a sole key draws values
*/
static void
d_tests_mariadb_described_keys(
    void
)
{
    char                    schema[96] = "";
    struct d_mariadb*       db         = d_tests_mariadb_begin("keys",
                                                               schema,
                                                               sizeof(schema));
    struct d_db_column_list columns    = { 0u, NULL };

    printf("mariadb_schema: described keys\n");

    D_TESTS_MARIADB_OK(db,
                       d_mariadb_create_table(db,
                                              NULL,
                                              "composite",
                                              d_tests_mariadb_composite,
                                              3u,
                                              0u));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_list_columns(db, NULL, "composite", &columns));
    D_TESTS_MARIADB_CHECK(columns.count == 3u);

    if (columns.count == 3u)
    {
        D_TESTS_MARIADB_CHECK(strcmp(columns.columns[0].name, "region") == 0);
        D_TESTS_MARIADB_CHECK(columns.columns[0].primary_key == 1);
        D_TESTS_MARIADB_CHECK(columns.columns[1].primary_key == 2);
        D_TESTS_MARIADB_CHECK(columns.columns[2].primary_key == 0);
        D_TESTS_MARIADB_CHECK(
            strcmp(columns.columns[0].type, "varchar(8)") == 0);
        D_TESTS_MARIADB_CHECK(columns.columns[0].not_null);
        D_TESTS_MARIADB_CHECK(
            strcmp(columns.columns[2].type, "longtext") == 0);
    }

    d_db_column_list_free(&columns);
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec(db,
                                      "INSERT INTO composite (region, code) "
                                      "VALUES ('eu', 1), ('us', 1)"));
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_exec(db,
                                          "INSERT INTO composite "
                                          "(region, code) VALUES ('eu', 1)"),
                           1062);


    // a key in the order its places give, not the table's
    struct d_db_column bad[3] =
    {
        d_tests_mariadb_ordered[0],
        d_tests_mariadb_ordered[1],
        d_tests_mariadb_ordered[2]
    };

    D_TESTS_MARIADB_OK(db,
                       d_mariadb_create_table(db,
                                              NULL,
                                              "ordered",
                                              d_tests_mariadb_ordered,
                                              3u,
                                              0u));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_list_columns(db,
                                              NULL,
                                              "ordered",
                                              &columns));

    const struct d_db_column_info* const first  =
        d_tests_mariadb_find_column(&columns, "a");
    const struct d_db_column_info* const second =
        d_tests_mariadb_find_column(&columns, "b");

    D_TESTS_MARIADB_CHECK( (first) && (first->primary_key == 2) );
    D_TESTS_MARIADB_CHECK( (second) && (second->primary_key == 1) );
    d_db_column_list_free(&columns);

    // places that repeat, skip, number a column off the key, or leave one
    // key column out
    bad[1].key_place = 2u;
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_create_table(db,
                                                  NULL,
                                                  "unordered",
                                                  bad,
                                                  3u,
                                                  0u),
                           D_DB_INVALID_ARGUMENT);
    bad[1].key_place = 3u;
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_create_table(db,
                                                  NULL,
                                                  "unordered",
                                                  bad,
                                                  3u,
                                                  0u),
                           D_DB_INVALID_ARGUMENT);
    bad[1].key_place = 0u;
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_create_table(db,
                                                  NULL,
                                                  "unordered",
                                                  bad,
                                                  3u,
                                                  0u),
                           D_DB_INVALID_ARGUMENT);
    bad[1].key_place = 1u;
    bad[2].key_place = 3u;
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_create_table(db,
                                                  NULL,
                                                  "unordered",
                                                  bad,
                                                  3u,
                                                  0u),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_CHECK(!d_mariadb_has_table(db, NULL, "unordered"));

    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_described_refusals
  Column descriptions the layer refuses, before the server sees them.
  Tests the following:
  - each set of d_tests_mariadb_refused is D_DB_INVALID_ARGUMENT, and makes
    nothing
  - NONE with the server's own type is no refusal
*/
static void
d_tests_mariadb_described_refusals(
    void
)
{
    const size_t       count      = sizeof(d_tests_mariadb_refused) /
                                    sizeof(d_tests_mariadb_refused[0]);
    char               schema[96] = "";
    struct d_mariadb*  db         = d_tests_mariadb_begin("refusals",
                                                          schema,
                                                          sizeof(schema));
    struct d_db_column own        = d_tests_mariadb_refused[0][0];

    printf("mariadb_schema: described refusals\n");

    for (size_t i = 0u; i < count; ++i)
    {
        D_TESTS_MARIADB_RESULT(db,
                               d_mariadb_create_table(
                                   db,
                                   NULL,
                                   "t",
                                   d_tests_mariadb_refused[i],
                                   (d_tests_mariadb_refused[i][1].name) ? 2u
                                                                        : 1u,
                                   0u),
                               D_DB_INVALID_ARGUMENT);
    }

    D_TESTS_MARIADB_CHECK(!d_mariadb_has_table(db, NULL, "t"));
    own.type_sql = "MEDIUMINT UNSIGNED";
    D_TESTS_MARIADB_OK(db, d_mariadb_create_table(db, NULL, "t", &own, 1u, 0u));

    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_read_default
  Whether a column's default, put into a query as SQL, reads as expected;
says what it read when not.
*/
static bool
d_tests_mariadb_read_default(
    struct d_mariadb*              _db,
    const struct d_db_column_list* _columns,
    const char* const              _read[3]
)
{
    const struct d_db_column_info* const column =
        d_tests_mariadb_find_column(_columns, _read[0]);
    char*                                text   = NULL;
    const bool                           same   =
        ( (column)              &&
          (column->default_sql) &&
          (d_mariadb_query_textf(_db,
                                 &text,
                                 _read[1],
                                 column->default_sql) == 0) &&
          (text) &&
          (strcmp(text, _read[2]) == 0) );

    // what the default was, and what it read
    if (!same)
    {
        printf("    default of %s: %s reads %s, expected %s\n",
               _read[0],
               ( (column) && (column->default_sql) ) ? column->default_sql
                                                     : "none",
               (text) ? text : "NULL",
               _read[2]);
    }

    d_db_free(text);

    return same;
}

/*
d_tests_mariadb_defaults
  Literal defaults read back as SQL.
  Tests the following:
  - every literal default list_columns reports is SQL that reads as the
    stored default: strings, quotes, backslashes and an empty one among
    them, numbers, a date, an ENUM value, bits and bytes
*/
static void
d_tests_mariadb_defaults(
    void
)
{
    const size_t            count      =
        sizeof(d_tests_mariadb_default_reads) /
        sizeof(d_tests_mariadb_default_reads[0]);
    char                    schema[96] = "";
    struct d_mariadb*       db         = d_tests_mariadb_begin("defaults",
                                                               schema,
                                                               sizeof(schema));
    struct d_db_column_list columns    = { 0u, NULL };

    printf("mariadb_schema: defaults as SQL\n");

    D_TESTS_MARIADB_OK(db,
                       d_mariadb_create_table_sql(db,
                                                  NULL,
                                                  "d",
                                                  d_tests_mariadb_defaults_sql,
                                                  0u));
    D_TESTS_MARIADB_OK(db, d_mariadb_list_columns(db, NULL, "d", &columns));

    for (size_t i = 0u; i < count; ++i)
    {
        D_TESTS_MARIADB_CHECK(
            d_tests_mariadb_read_default(db,
                                         &columns,
                                         d_tests_mariadb_default_reads[i]));
    }

    d_db_column_list_free(&columns);
    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_default_kinds
  Defaults that are not literals.
  Tests the following:
  - CURRENT_TIMESTAMP reads as itself, in the server's spelling
  - a default of NULL, and none, read as NULL
*/
static void
d_tests_mariadb_default_kinds(
    void
)
{
    char                           schema[96] = "";
    struct d_mariadb*              db         = d_tests_mariadb_begin(
                                                    "kinds",
                                                    schema,
                                                    sizeof(schema));
    struct d_db_column_list        columns    = { 0u, NULL };
    const struct d_db_column_info* column     = NULL;

    printf("mariadb_schema: kinds of default\n");

    D_TESTS_MARIADB_OK(db,
                       d_mariadb_create_table_sql(db,
                                                  NULL,
                                                  "d",
                                                  d_tests_mariadb_defaults_sql,
                                                  0u));
    D_TESTS_MARIADB_OK(db, d_mariadb_list_columns(db, NULL, "d", &columns));

    column = d_tests_mariadb_find_column(&columns, "c");
    D_TESTS_MARIADB_CHECK( (column) && (column->default_sql) &&
                           ( (strncmp(column->default_sql,
                                      "CURRENT_TIMESTAMP", 17u) == 0) ||
                             (strncmp(column->default_sql,
                                      "current_timestamp", 17u) == 0) ) );
    column = d_tests_mariadb_find_column(&columns, "f");
    D_TESTS_MARIADB_CHECK( (column) && (!column->default_sql) );
    column = d_tests_mariadb_find_column(&columns, "g");
    D_TESTS_MARIADB_CHECK( (column) && (!column->default_sql) &&
                           (column->not_null) );
    column = d_tests_mariadb_find_column(&columns, "id");
    D_TESTS_MARIADB_CHECK( (column) && (!column->default_sql) &&
                           (column->primary_key == 1) );

    d_db_column_list_free(&columns);
    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_column_existence
  Whether a column exists.
  Tests the following:
  - a column is found by any case; a missing column or table is not
  - a view's columns are columns too
*/
static void
d_tests_mariadb_column_existence(
    void
)
{
    char              schema[96] = "";
    struct d_mariadb* db         = d_tests_mariadb_begin("colexist",
                                                         schema,
                                                         sizeof(schema));
    bool              exists     = false;

    printf("mariadb_schema: column existence\n");

    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec_script(db,
                                             "CREATE TABLE t (id INT, Name "
                                             "TEXT); CREATE VIEW v AS SELECT "
                                             "id AS vid FROM t"));
    D_TESTS_MARIADB_CHECK(d_mariadb_has_column(db, NULL, "t", "id"));
    D_TESTS_MARIADB_CHECK(d_mariadb_has_column(db, NULL, "t", "ID"));
    D_TESTS_MARIADB_CHECK(d_mariadb_has_column(db, NULL, "t", "name"));
    D_TESTS_MARIADB_CHECK(d_mariadb_has_column(db, schema, "t", "Name"));
    D_TESTS_MARIADB_CHECK(!d_mariadb_has_column(db, NULL, "t", "other"));
    D_TESTS_MARIADB_CHECK(!d_mariadb_has_column(db, NULL, "none", "id"));
    D_TESTS_MARIADB_CHECK(d_mariadb_has_column(db, NULL, "v", "vid"));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_has_column_s(db, NULL, "t", "id", &exists));
    D_TESTS_MARIADB_CHECK(exists);

    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_column_additions
  Adding columns.
  Tests the following:
  - a column added from a description, its default filling the rows there,
    and one from SQL; a taken name is 1060, a missing table 1146
  - a key column with AUTO_INCREMENT added to a table without a key numbers
    the rows; AUTO_INCREMENT off the key is refused
*/
static void
d_tests_mariadb_column_additions(
    void
)
{
    char                     schema[96] = "";
    struct d_mariadb*        db         = d_tests_mariadb_begin(
                                              "additions",
                                              schema,
                                              sizeof(schema));
    const struct d_db_column note       = { "note", D_DB_TYPE_STRING, 0u,
                                            "'-'", 20u, 0u, NULL, 0u };
    const struct d_db_column serial     = { "serial", D_DB_TYPE_INTEGER,
                                            D_DB_COLUMN_PRIMARY_KEY |
                                            D_DB_COLUMN_AUTO_INCREMENT,
                                            NULL, 0u, 0u, NULL, 0u };

    printf("mariadb_schema: column additions\n");

    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec_script(db,
                                             "CREATE TABLE t (a INT); "
                                             "INSERT INTO t VALUES (1), (2)"));
    D_TESTS_MARIADB_OK(db, d_mariadb_add_column(db, NULL, "t", &note));
    D_TESTS_MARIADB_CHECK(d_mariadb_has_column(db, NULL, "t", "note"));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_text_is(db,
                                                  "SELECT note FROM t LIMIT 1",
                                                  "-"));
    D_TESTS_MARIADB_RESULT(db, d_mariadb_add_column(db, NULL, "t", &note),
                           1060);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_add_column(db, NULL, "none", &note),
                           1146);
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_add_column_sql(db,
                                                schema,
                                                "t",
                                                "extra DOUBLE NOT NULL "
                                                "DEFAULT 0.5"));
    D_TESTS_MARIADB_CHECK(d_mariadb_has_column(db, NULL, "t", "extra"));
    D_TESTS_MARIADB_OK(db, d_mariadb_add_column(db, NULL, "t", &serial));
    D_TESTS_MARIADB_CHECK(
        d_tests_mariadb_int(db, "SELECT MAX(serial) FROM t") == 2);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_add_column(db,
                                                NULL,
                                                "t",
                                                &d_tests_mariadb_refused[4][0]),
                           D_DB_INVALID_ARGUMENT);

    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_column_removals
  Renaming and dropping columns.
  Tests the following:
  - rename where the server has RENAME COLUMN, D_DB_UNSUPPORTED where not;
    a missing column is 1054
  - drop; a missing column is 1091, the last column 1090
*/
static void
d_tests_mariadb_column_removals(
    void
)
{
    char              schema[96] = "";
    struct d_mariadb* db         = d_tests_mariadb_begin("removals",
                                                         schema,
                                                         sizeof(schema));

    printf("mariadb_schema: column renames and drops\n");

    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec_script(db,
                                             "CREATE TABLE t (a INT, "
                                             "note TEXT, extra INT); "
                                             "CREATE TABLE one (x INT)"));

    // renamed, where the server can
    if (d_mariadb_supports(db, D_MARIADB_FEATURE_RENAME_COLUMN))
    {
        D_TESTS_MARIADB_OK(db,
                           d_mariadb_rename_column(db, NULL, "t", "note",
                                                   "remark"));
        D_TESTS_MARIADB_CHECK(!d_mariadb_has_column(db, NULL, "t", "note"));
        D_TESTS_MARIADB_CHECK(d_mariadb_has_column(db, NULL, "t", "remark"));
        D_TESTS_MARIADB_RESULT(db,
                               d_mariadb_rename_column(db, NULL, "t", "note",
                                                       "x"),
                               1054);
    }
    else
    {
        D_TESTS_MARIADB_RESULT(db,
                               d_mariadb_rename_column(db, NULL, "t", "note",
                                                       "remark"),
                               D_DB_UNSUPPORTED);
    }

    // dropped, to the last
    D_TESTS_MARIADB_OK(db, d_mariadb_drop_column(db, NULL, "t", "extra"));
    D_TESTS_MARIADB_CHECK(!d_mariadb_has_column(db, NULL, "t", "extra"));
    D_TESTS_MARIADB_RESULT(db, d_mariadb_drop_column(db, NULL, "t", "extra"),
                           1091);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_drop_column(db, NULL, "one", "x"),
                           1090);

    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_column_redefinitions
  Redefining columns.
  Tests the following:
  - a column given a new type and default keeps its values, and loses the
    NOT NULL left out
  - the key column redefined with the key flag stays the sole key, adding
    none, and AUTO_INCREMENT then numbers new rows; the unique flag adds no
    index
  - a missing column is 1054, a missing table 1146; AUTO_INCREMENT off the
    key is refused
*/
static void
d_tests_mariadb_column_redefinitions(
    void
)
{
    char                     schema[96] = "";
    struct d_mariadb*        db         = d_tests_mariadb_begin("redefine",
                                                                schema,
                                                                sizeof(schema));
    const struct d_db_column wide       = { "a", D_DB_TYPE_BIG_INTEGER, 0u,
                                            "7", 0u, 0u, NULL, 0u };
    const struct d_db_column key        = { "id", D_DB_TYPE_BIG_INTEGER,
                                            D_DB_COLUMN_PRIMARY_KEY |
                                            D_DB_COLUMN_AUTO_INCREMENT |
                                            D_DB_COLUMN_NOT_NULL,
                                            NULL, 0u, 0u, NULL, 0u };
    const struct d_db_column note       = { "note", D_DB_TYPE_STRING,
                                            D_DB_COLUMN_UNIQUE, NULL, 40u, 0u,
                                            NULL, 0u };
    const struct d_db_column none       = { "none", D_DB_TYPE_INTEGER, 0u,
                                            NULL, 0u, 0u, NULL, 0u };
    struct d_db_column_list  columns    = { 0u, NULL };

    printf("mariadb_schema: column redefinitions\n");

    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec_script(db,
                                             "CREATE TABLE t (id INT "
                                             "PRIMARY KEY, a INT NOT NULL "
                                             "DEFAULT 1, note VARCHAR(10)); "
                                             "INSERT INTO t VALUES "
                                             "(1, 2, 'x')"));

    // a new type and default, the value kept, NOT NULL gone
    D_TESTS_MARIADB_OK(db, d_mariadb_modify_column(db, NULL, "t", &wide));
    D_TESTS_MARIADB_OK(db, d_mariadb_list_columns(db, NULL, "t", &columns));
    D_TESTS_MARIADB_CHECK( (columns.count == 3u) &&
                           (strncmp(columns.columns[1].type, "bigint", 6u)
                            == 0) &&
                           (!columns.columns[1].not_null) &&
                           (columns.columns[1].default_sql) &&
                           (strcmp(columns.columns[1].default_sql, "7")
                            == 0) );
    d_db_column_list_free(&columns);
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT a FROM t") == 2);

    // the key stays the key, and numbers what comes next
    D_TESTS_MARIADB_OK(db, d_mariadb_modify_column(db, schema, "t", &key));
    D_TESTS_MARIADB_OK(db, d_mariadb_list_columns(db, NULL, "t", &columns));
    D_TESTS_MARIADB_CHECK( (columns.count == 3u) &&
                           (columns.columns[0].primary_key == 1) &&
                           (columns.columns[1].primary_key == 0) &&
                           (strncmp(columns.columns[0].type, "bigint", 6u)
                            == 0) );
    d_db_column_list_free(&columns);
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "INSERT INTO t (a) VALUES (3)"));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT MAX(id) FROM t")
                          == 2);

    // UNIQUE describes the column, adding no index
    D_TESTS_MARIADB_OK(db, d_mariadb_modify_column(db, NULL, "t", &note));
    D_TESTS_MARIADB_CHECK(
        d_tests_mariadb_int(db,
                            "SELECT COUNT(*) FROM information_schema."
                            "STATISTICS WHERE TABLE_SCHEMA = DATABASE() "
                            "AND TABLE_NAME = 't' AND COLUMN_NAME = 'note'")
        == 0);

    // what is not there, and what is refused
    D_TESTS_MARIADB_RESULT(db, d_mariadb_modify_column(db, NULL, "t", &none),
                           1054);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_modify_column(db, NULL, "gone", &wide),
                           1146);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_modify_column(
                               db,
                               NULL,
                               "t",
                               &d_tests_mariadb_refused[4][0]),
                           D_DB_INVALID_ARGUMENT);

    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_column_listing
  Describing columns.
  Tests the following:
  - a table that does not exist is D_DB_NOT_FOUND
  - two tables whose names differ only by case are described apart, where
    the server matches by case
  - a view is described by its columns
  - generated columns are told apart, a DEFAULT_GENERATED default not
*/
static void
d_tests_mariadb_column_listing(
    void
)
{
    char                    schema[96] = "";
    struct d_mariadb*       db         = d_tests_mariadb_begin("collist",
                                                               schema,
                                                               sizeof(schema));
    struct d_db_column_list columns    = { 0u, NULL };

    printf("mariadb_schema: column listing\n");

    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_list_columns(db, NULL, "none", &columns),
                           D_DB_NOT_FOUND);
    D_TESTS_MARIADB_CHECK( (columns.count == 0u) &&
                           (columns.columns == NULL) );

    if (!d_tests_mariadb_case_blind(db))
    {
        D_TESTS_MARIADB_OK(db,
                           d_mariadb_exec_script(
                               db,
                               "CREATE TABLE Pair (a INT PRIMARY KEY, b INT); "
                               "CREATE TABLE pair (c INT PRIMARY KEY)"));
        D_TESTS_MARIADB_OK(db,
                           d_mariadb_list_columns(db, NULL, "Pair", &columns));
        D_TESTS_MARIADB_CHECK( (columns.count == 2u) &&
                               (strcmp(columns.columns[0].name, "a") == 0) &&
                               (columns.columns[0].primary_key == 1) &&
                               (columns.columns[1].primary_key == 0) );
        d_db_column_list_free(&columns);
        D_TESTS_MARIADB_OK(db,
                           d_mariadb_list_columns(db, schema, "pair",
                                                  &columns));
        D_TESTS_MARIADB_CHECK( (columns.count == 1u) &&
                               (strcmp(columns.columns[0].name, "c") == 0) );
        d_db_column_list_free(&columns);
        D_TESTS_MARIADB_RESULT(db,
                               d_mariadb_list_columns(db, NULL, "PAIR",
                                                      &columns),
                               D_DB_NOT_FOUND);
    }

    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec(db,
                                      "CREATE VIEW v AS SELECT 1 AS one, "
                                      "'x' AS two"));
    D_TESTS_MARIADB_OK(db, d_mariadb_list_columns(db, NULL, "v", &columns));
    D_TESTS_MARIADB_CHECK( (columns.count == 2u) &&
                           (strcmp(columns.columns[1].name, "two") == 0) );
    d_db_column_list_free(&columns);

    // generated columns, virtual and stored; a default MySQL marks
    // DEFAULT_GENERATED is not one
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec(db,
                                      "CREATE TABLE gen (a INT, b INT AS "
                                      "(a * 2) VIRTUAL, c INT AS (a + 1) "
                                      "STORED, d TIMESTAMP NULL DEFAULT "
                                      "CURRENT_TIMESTAMP)"));
    D_TESTS_MARIADB_OK(db, d_mariadb_list_columns(db, NULL, "gen", &columns));
    D_TESTS_MARIADB_CHECK( (columns.count == 4u) &&
                           (!columns.columns[0].generated) &&
                           (columns.columns[1].generated) &&
                           (columns.columns[2].generated) &&
                           (!columns.columns[3].generated) &&
                           (!columns.columns[1].default_sql) );
    d_db_column_list_free(&columns);

    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_column_arguments
  Argument checks for columns.
  Tests the following:
  - NULL handles, empty schemas, NULL and empty names, NULL outputs and
    descriptions
*/
static void
d_tests_mariadb_column_arguments(
    void
)
{
    struct d_mariadb* const  db      = d_tests_mariadb_connect(NULL);
    struct d_db_column_list  columns = { 0u, NULL };
    const struct d_db_column column  = { "a", D_DB_TYPE_INTEGER, 0u, NULL, 0u,
                                         0u, NULL, 0u };
    bool                     exists  = true;

    printf("mariadb_schema: column arguments\n");

    D_TESTS_MARIADB_CHECK(!d_mariadb_has_column(NULL, NULL, "t", "a"));
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_has_column_s(db, NULL, NULL, "a", &exists),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_CHECK(!exists);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_has_column_s(db, NULL, "t", "", &exists),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_add_column(db, NULL, "t", NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_add_column(db, "", "t", &column),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_add_column_sql(db, NULL, "t", ""),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_modify_column(db, NULL, "t", NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_modify_column(db, NULL, "", &column),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_modify_column(NULL, NULL, "t", &column),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_rename_column(db, NULL, "t", "a", ""),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_rename_column(NULL, NULL, "t", "a", "b"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_drop_column(db, NULL, "t", NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_list_columns(db, NULL, "t", NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_list_columns(db, NULL, "", &columns),
                           D_DB_INVALID_ARGUMENT);

    d_mariadb_close(db);

    return;
}

//==============================================================================
// mariadb_schema.h -- indexes, views and triggers
//==============================================================================

/*
d_tests_mariadb_make_indexed
  A database of the test's own holding a table, t, of two rows, with two
indexes beside its key: ix_bc on two columns, and ix_c, unique, on a
prefix.
*/
static struct d_mariadb*
d_tests_mariadb_make_indexed(
    const char* _word,
    char*       _schema,
    size_t      _size
)
{
    struct d_mariadb* db = d_tests_mariadb_begin(_word, _schema, _size);

    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec_script(db,
                                             "CREATE TABLE t (id INT PRIMARY "
                                             "KEY, b INT, c VARCHAR(20)); "
                                             "INSERT INTO t VALUES (1, 1, 'x'),"
                                             " (2, 1, 'y')"));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_create_index(db, NULL, "ix_bc", "t", "b, c",
                                              0u));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_create_index(db, _schema, "ix_c", "t",
                                              "c(10)", D_DB_UNIQUE));

    return db;
}

/*
d_tests_mariadb_indexes
  Finding and listing indexes.
  Tests the following:
  - an index is found by any case, the primary key as PRIMARY; a missing
    index or table is not
  - listed once each, PRIMARY left out, in byte order
*/
static void
d_tests_mariadb_indexes(
    void
)
{
    char                  schema[96] = "";
    struct d_mariadb*     db         = d_tests_mariadb_make_indexed(
                                           "indexes",
                                           schema,
                                           sizeof(schema));
    struct d_db_name_list names      = { 0u, NULL };
    bool                  exists     = false;

    printf("mariadb_schema: indexes\n");

    D_TESTS_MARIADB_CHECK(d_mariadb_has_index(db, NULL, "ix_bc", "t"));
    D_TESTS_MARIADB_CHECK(d_mariadb_has_index(db, NULL, "IX_BC", "t"));
    D_TESTS_MARIADB_CHECK(d_mariadb_has_index(db, NULL, "PRIMARY", "t"));
    D_TESTS_MARIADB_CHECK(!d_mariadb_has_index(db, NULL, "ix_none", "t"));
    D_TESTS_MARIADB_CHECK(!d_mariadb_has_index(db, NULL, "ix_bc", "none"));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_has_index_s(db, NULL, "ix_c", "t", &exists));
    D_TESTS_MARIADB_CHECK(exists);
    D_TESTS_MARIADB_OK(db, d_mariadb_list_indexes(db, NULL, "t", &names));
    D_TESTS_MARIADB_CHECK( (names.count == 2u) &&
                           (strcmp(names.names[0], "ix_bc") == 0) &&
                           (strcmp(names.names[1], "ix_c") == 0) );
    d_db_name_list_free(&names);

    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_index_changes
  Creating and dropping indexes again.
  Tests the following:
  - IF NOT EXISTS and IF EXISTS on both servers, MySQL's by asking first
  - a taken name is 1061, a unique index over duplicates 1062, a missing
    index 1091, a missing table 1146
*/
static void
d_tests_mariadb_index_changes(
    void
)
{
    char                  schema[96] = "";
    struct d_mariadb*     db         = d_tests_mariadb_make_indexed(
                                           "ixchanges",
                                           schema,
                                           sizeof(schema));
    struct d_db_name_list names      = { 0u, NULL };

    printf("mariadb_schema: index changes\n");

    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_create_index(db, NULL, "ix_bc", "t", "c",
                                                  0u),
                           1061);
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_create_index(db, NULL, "ix_bc", "t", "c",
                                              D_DB_IF_NOT_EXISTS));
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_create_index(db, NULL, "ix_b", "t", "b",
                                                  D_DB_UNIQUE),
                           1062);
    D_TESTS_MARIADB_CHECK(!d_mariadb_has_index(db, NULL, "ix_b", "t"));
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_create_index(db, NULL, "ix_x", "none",
                                                  "a", 0u),
                           1146);

    D_TESTS_MARIADB_OK(db, d_mariadb_drop_index(db, NULL, "ix_bc", "t", 0u));
    D_TESTS_MARIADB_CHECK(!d_mariadb_has_index(db, NULL, "ix_bc", "t"));
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_drop_index(db, NULL, "ix_bc", "t", 0u),
                           1091);
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_drop_index(db, NULL, "ix_bc", "t",
                                            D_DB_IF_EXISTS));
    D_TESTS_MARIADB_OK(db, d_mariadb_list_indexes(db, NULL, "t", &names));
    D_TESTS_MARIADB_CHECK( (names.count == 1u) &&
                           (strcmp(names.names[0], "ix_c") == 0) );
    d_db_name_list_free(&names);

    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_make_viewed
  A database of the test's own holding a table, t, of two rows, and two
views: v_two over t, and v_one, made in the database named.
*/
static struct d_mariadb*
d_tests_mariadb_make_viewed(
    const char* _word,
    char*       _schema,
    size_t      _size
)
{
    struct d_mariadb* db = d_tests_mariadb_begin(_word, _schema, _size);

    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec_script(db,
                                             "CREATE TABLE t (a INT); "
                                             "INSERT INTO t VALUES (1), (2)"));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_create_view(db, NULL, "v_two",
                                             "SELECT a * 2 AS b FROM t", 0u));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_create_view(db, _schema, "v_one",
                                             "SELECT 1 AS one", 0u));

    return db;
}

/*
d_tests_mariadb_views
  Finding and listing views.
  Tests the following:
  - a view is found and queried; a table is not a view
  - listed apart from tables, in byte order
*/
static void
d_tests_mariadb_views(
    void
)
{
    char                  schema[96] = "";
    struct d_mariadb*     db         = d_tests_mariadb_make_viewed(
                                           "views",
                                           schema,
                                           sizeof(schema));
    struct d_db_name_list names      = { 0u, NULL };
    bool                  exists     = false;

    printf("mariadb_schema: views\n");

    D_TESTS_MARIADB_CHECK(d_mariadb_has_view(db, NULL, "v_two"));
    D_TESTS_MARIADB_CHECK(!d_mariadb_has_view(db, NULL, "t"));
    D_TESTS_MARIADB_OK(db, d_mariadb_has_view_s(db, schema, "v_one", &exists));
    D_TESTS_MARIADB_CHECK(exists);
    D_TESTS_MARIADB_CHECK(
        d_tests_mariadb_int(db, "SELECT SUM(b) FROM v_two") == 6);
    D_TESTS_MARIADB_OK(db, d_mariadb_list_views(db, NULL, &names));
    D_TESTS_MARIADB_CHECK( (names.count == 2u) &&
                           (strcmp(names.names[0], "v_one") == 0) &&
                           (strcmp(names.names[1], "v_two") == 0) );
    d_db_name_list_free(&names);

    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_view_changes
  Creating and dropping views again.
  Tests the following:
  - IF NOT EXISTS: a success over a view of the name, whose query stays, and
    over a table of the name, as MariaDB's own clause has it, which leaves
    the table
  - a taken name is 1050; a missing view 4092 or 1051; IF EXISTS
*/
static void
d_tests_mariadb_view_changes(
    void
)
{
    char              schema[96] = "";
    struct d_mariadb* db         = d_tests_mariadb_make_viewed("vchanges",
                                                               schema,
                                                               sizeof(schema));
    int               result     = 0;

    printf("mariadb_schema: view changes\n");

    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_create_view(db, NULL, "v_two",
                                                 "SELECT 2", 0u),
                           1050);
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_create_view(db, NULL, "v_two", "SELECT 2",
                                             D_DB_IF_NOT_EXISTS));
    D_TESTS_MARIADB_CHECK(
        d_tests_mariadb_int(db, "SELECT SUM(b) FROM v_two") == 6);
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_create_view(db, NULL, "t", "SELECT 2",
                                             D_DB_IF_NOT_EXISTS));
    D_TESTS_MARIADB_CHECK(d_mariadb_has_table(db, NULL, "t"));
    D_TESTS_MARIADB_CHECK(!d_mariadb_has_view(db, NULL, "t"));

    D_TESTS_MARIADB_OK(db, d_mariadb_drop_view(db, NULL, "v_two", 0u));
    D_TESTS_MARIADB_CHECK(!d_mariadb_has_view(db, NULL, "v_two"));
    result = d_mariadb_drop_view(db, NULL, "v_two", 0u);
    D_TESTS_MARIADB_CHECK( (result == 4092) || (result == 1051) );
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_drop_view(db, NULL, "v_two", D_DB_IF_EXISTS));

    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_make_triggered
  A database of the test's own holding two tables, t and u, and a trigger on
each: tr_add, which adds one to what t is given, and tr_u, made in the
database named.
*/
static struct d_mariadb*
d_tests_mariadb_make_triggered(
    const char* _word,
    char*       _schema,
    size_t      _size
)
{
    struct d_mariadb* db = d_tests_mariadb_begin(_word, _schema, _size);

    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec_script(db,
                                             "CREATE TABLE t (a INT); "
                                             "CREATE TABLE u (a INT)"));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_create_trigger(db, NULL, "tr_add",
                                                "BEFORE INSERT ON t FOR EACH "
                                                "ROW SET NEW.a = NEW.a + 1",
                                                0u));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_create_trigger(db, _schema, "tr_u",
                                                "BEFORE INSERT ON u FOR EACH "
                                                "ROW SET NEW.a = 0", 0u));

    return db;
}

/*
d_tests_mariadb_triggers
  Running, finding and listing triggers.
  Tests the following:
  - a trigger runs; is found; is listed for the database and for its table
  - names match as the server matches them: by case on MariaDB and MySQL
    5.7, never on MySQL 8
*/
static void
d_tests_mariadb_triggers(
    void
)
{
    char                  schema[96] = "";
    struct d_mariadb*     db         = d_tests_mariadb_make_triggered(
                                           "triggers",
                                           schema,
                                           sizeof(schema));
    struct d_db_name_list names      = { 0u, NULL };
    bool                  exists     = false;

    printf("mariadb_schema: triggers\n");

    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "INSERT INTO t VALUES (1)"));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT a FROM t") == 2);
    D_TESTS_MARIADB_CHECK(d_mariadb_has_trigger(db, NULL, "tr_add"));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_has_trigger_s(db, schema, "tr_u", &exists));
    D_TESTS_MARIADB_CHECK(exists);
    D_TESTS_MARIADB_CHECK(!d_mariadb_has_trigger(db, NULL, "tr_none"));
    D_TESTS_MARIADB_CHECK(
        d_mariadb_has_trigger(db, NULL, "TR_ADD") ==
        ( (d_tests_mariadb_mysql_at_least(db, 8u, 0u, 0u)) ||
          (d_tests_mariadb_case_blind(db)) ));

    D_TESTS_MARIADB_OK(db, d_mariadb_list_triggers(db, NULL, NULL, &names));
    D_TESTS_MARIADB_CHECK( (names.count == 2u) &&
                           (strcmp(names.names[0], "tr_add") == 0) &&
                           (strcmp(names.names[1], "tr_u") == 0) );
    d_db_name_list_free(&names);
    D_TESTS_MARIADB_OK(db, d_mariadb_list_triggers(db, NULL, "u", &names));
    D_TESTS_MARIADB_CHECK( (names.count == 1u) &&
                           (strcmp(names.names[0], "tr_u") == 0) );
    d_db_name_list_free(&names);
    D_TESTS_MARIADB_OK(db, d_mariadb_list_triggers(db, NULL, "none", &names));
    D_TESTS_MARIADB_CHECK(names.count == 0u);
    d_db_name_list_free(&names);

    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_trigger_changes
  Creating and dropping triggers again.
  Tests the following:
  - IF NOT EXISTS on every server, the older MySQLs by asking first
  - a taken name is 1359, a missing trigger 1360; IF EXISTS
*/
static void
d_tests_mariadb_trigger_changes(
    void
)
{
    char              schema[96] = "";
    struct d_mariadb* db         = d_tests_mariadb_make_triggered(
                                       "trchanges",
                                       schema,
                                       sizeof(schema));

    printf("mariadb_schema: trigger changes\n");

    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_create_trigger(db, NULL, "tr_add",
                                                    "AFTER INSERT ON t FOR "
                                                    "EACH ROW SET @x = 1",
                                                    0u),
                           1359);
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_create_trigger(db, NULL, "tr_add",
                                                "AFTER INSERT ON t FOR EACH "
                                                "ROW SET @x = 1",
                                                D_DB_IF_NOT_EXISTS));
    D_TESTS_MARIADB_OK(db, d_mariadb_drop_trigger(db, NULL, "tr_add", 0u));
    D_TESTS_MARIADB_CHECK(!d_mariadb_has_trigger(db, NULL, "tr_add"));
    D_TESTS_MARIADB_RESULT(db, d_mariadb_drop_trigger(db, NULL, "tr_add", 0u),
                           1360);
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_drop_trigger(db, NULL, "tr_add",
                                              D_DB_IF_EXISTS));

    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_index_arguments
  Argument checks for indexes.
  Tests the following:
  - NULL handles, NULL and empty names and key parts, NULL outputs
  - flags a function does not take
*/
static void
d_tests_mariadb_index_arguments(
    void
)
{
    struct d_mariadb* const db     = d_tests_mariadb_connect(NULL);
    struct d_db_name_list   names  = { 0u, NULL };
    bool                    exists = true;

    printf("mariadb_schema: index arguments\n");

    D_TESTS_MARIADB_RESULT(db, d_mariadb_has_index_s(db, NULL, "i", NULL,
                                                     &exists),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_CHECK(!exists);
    D_TESTS_MARIADB_CHECK(!d_mariadb_has_index(NULL, NULL, "i", "t"));
    D_TESTS_MARIADB_RESULT(db, d_mariadb_create_index(db, NULL, "i", "t", "",
                                                      0u),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_create_index(db, NULL, "i", "t", "a",
                                                      D_DB_IF_EXISTS),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_create_index(db, NULL, "", "t", "a",
                                                      0u),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_drop_index(db, NULL, "i", NULL, 0u),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_drop_index(db, NULL, "i", "t",
                                                    D_DB_UNIQUE),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_list_indexes(db, NULL, NULL, &names),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_list_indexes(db, NULL, "t", NULL),
                           D_DB_INVALID_ARGUMENT);

    d_mariadb_close(db);

    return;
}

/*
d_tests_mariadb_view_trigger_arguments
  Argument checks for views and triggers.
  Tests the following:
  - NULL handles, empty schemas, NULL and empty names and SQL, NULL outputs
  - flags a function does not take
*/
static void
d_tests_mariadb_view_trigger_arguments(
    void
)
{
    struct d_mariadb* const db     = d_tests_mariadb_connect(NULL);
    struct d_db_name_list   names  = { 0u, NULL };
    bool                    exists = true;

    printf("mariadb_schema: view and trigger arguments\n");

    D_TESTS_MARIADB_RESULT(db, d_mariadb_has_view_s(db, "", "v", &exists),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_create_view(db, NULL, "v", NULL, 0u),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_create_view(db, NULL, "v",
                                                     "SELECT 1",
                                                     D_DB_UNIQUE),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_drop_view(NULL, NULL, "v", 0u),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_list_views(db, NULL, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_has_trigger_s(db, NULL, NULL,
                                                       &exists),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_create_trigger(db, NULL, "tr", "",
                                                        0u),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_create_trigger(db, NULL, "tr",
                                                        "x", D_DB_IF_EXISTS),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_drop_trigger(db, NULL, "",
                                                      D_DB_IF_EXISTS),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_list_triggers(db, NULL, "", &names),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_list_triggers(NULL, NULL, NULL,
                                                       &names),
                           D_DB_INVALID_ARGUMENT);

    d_mariadb_close(db);

    return;
}

//==============================================================================
// mariadb_transaction.h
//==============================================================================

/*
d_tests_mariadb_transactions
  Beginning and ending transactions.
  Tests the following:
  - a rollback undoes, a commit keeps; in_transaction follows the server
  - a commit or rollback with none open succeeds
*/
static void
d_tests_mariadb_transactions(
    void
)
{
    char              schema[96] = "";
    struct d_mariadb* db         = d_tests_mariadb_begin("transactions",
                                                         schema,
                                                         sizeof(schema));

    printf("mariadb_transaction: transactions\n");

    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "CREATE TABLE t (a INT)"));
    D_TESTS_MARIADB_CHECK(!d_mariadb_in_transaction(db));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_begin_transaction(
                           db,
                           D_MARIADB_TRANSACTION_READ_WRITE));
    D_TESTS_MARIADB_CHECK(d_mariadb_in_transaction(db));
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "INSERT INTO t VALUES (1)"));
    D_TESTS_MARIADB_OK(db, d_mariadb_rollback(db));
    D_TESTS_MARIADB_CHECK(!d_mariadb_in_transaction(db));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT COUNT(*) FROM t") ==
                          0);
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_begin_transaction(
                           db,
                           D_MARIADB_TRANSACTION_READ_WRITE));
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "INSERT INTO t VALUES (1)"));
    D_TESTS_MARIADB_OK(db, d_mariadb_commit(db));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT COUNT(*) FROM t") ==
                          1);

    // nothing open
    D_TESTS_MARIADB_OK(db, d_mariadb_commit(db));
    D_TESTS_MARIADB_OK(db, d_mariadb_rollback(db));

    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_transaction_modes
  The other ways to begin one.
  Tests the following:
  - READ ONLY refuses a write with 1792; CONSISTENT SNAPSHOT begins one
  - a mode outside the enumeration, and NULL handles
*/
static void
d_tests_mariadb_transaction_modes(
    void
)
{
    char              schema[96] = "";
    struct d_mariadb* db         = d_tests_mariadb_begin("modes",
                                                         schema,
                                                         sizeof(schema));

    printf("mariadb_transaction: transaction modes\n");

    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "CREATE TABLE t (a INT)"));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_begin_transaction(
                           db,
                           D_MARIADB_TRANSACTION_READ_ONLY));
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_exec(db, "INSERT INTO t VALUES (2)"),
                           1792);
    D_TESTS_MARIADB_OK(db, d_mariadb_rollback(db));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_begin_transaction(
                           db,
                           D_MARIADB_TRANSACTION_CONSISTENT_SNAPSHOT));
    D_TESTS_MARIADB_CHECK(d_mariadb_in_transaction(db));
    D_TESTS_MARIADB_OK(db, d_mariadb_commit(db));

    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_begin_transaction(
                               db,
                               (enum d_mariadb_transaction_mode)3),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_begin_transaction(
                               NULL,
                               D_MARIADB_TRANSACTION_READ_WRITE),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_commit(NULL), D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_rollback(NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_CHECK(!d_mariadb_in_transaction(NULL));

    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_savepoints
  Savepoints.
  Tests the following:
  - rolling back to one undoes what followed and keeps it; releasing forgets
    it, and a name not set is 1305
  - a name is quoted, odd characters and all
  - NULL and empty names
*/
static void
d_tests_mariadb_savepoints(
    void
)
{
    char              schema[96] = "";
    struct d_mariadb* db         = d_tests_mariadb_begin("savepoints",
                                                         schema,
                                                         sizeof(schema));

    printf("mariadb_transaction: savepoints\n");

    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "CREATE TABLE t (a INT)"));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_begin_transaction(
                           db,
                           D_MARIADB_TRANSACTION_READ_WRITE));
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "INSERT INTO t VALUES (1)"));
    D_TESTS_MARIADB_OK(db, d_mariadb_create_savepoint(db, "first point"));
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "INSERT INTO t VALUES (2)"));
    D_TESTS_MARIADB_OK(db, d_mariadb_rollback_to_savepoint(db, "first point"));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT COUNT(*) FROM t") ==
                          1);
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "INSERT INTO t VALUES (3)"));
    D_TESTS_MARIADB_OK(db, d_mariadb_rollback_to_savepoint(db, "first point"));
    D_TESTS_MARIADB_OK(db, d_mariadb_release_savepoint(db, "first point"));
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_rollback_to_savepoint(db, "first point"),
                           1305);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_release_savepoint(db, "none"), 1305);
    D_TESTS_MARIADB_OK(db, d_mariadb_commit(db));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT COUNT(*) FROM t") ==
                          1);

    D_TESTS_MARIADB_RESULT(db, d_mariadb_create_savepoint(db, ""),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_release_savepoint(db, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_rollback_to_savepoint(NULL, "a"),
                           D_DB_INVALID_ARGUMENT);

    d_tests_mariadb_end(db, schema);

    return;
}

//==============================================================================
// mariadb_settings.h
//==============================================================================

/*
d_tests_mariadb_autocommit
  Autocommit.
  Tests the following:
  - on by default; off leaves a statement's transaction open; on again
    commits it
  - NULL handles and outputs
*/
static void
d_tests_mariadb_autocommit(
    void
)
{
    char              schema[96] = "";
    struct d_mariadb* db         = d_tests_mariadb_begin("autocommit",
                                                         schema,
                                                         sizeof(schema));
    bool              enabled    = false;

    printf("mariadb_settings: autocommit\n");

    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "CREATE TABLE t (a INT)"));
    D_TESTS_MARIADB_OK(db, d_mariadb_get_autocommit(db, &enabled));
    D_TESTS_MARIADB_CHECK(enabled);
    D_TESTS_MARIADB_OK(db, d_mariadb_set_autocommit(db, false));
    D_TESTS_MARIADB_OK(db, d_mariadb_get_autocommit(db, &enabled));
    D_TESTS_MARIADB_CHECK(!enabled);
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "INSERT INTO t VALUES (1)"));
    D_TESTS_MARIADB_CHECK(d_mariadb_in_transaction(db));
    D_TESTS_MARIADB_OK(db, d_mariadb_set_autocommit(db, true));
    D_TESTS_MARIADB_CHECK(!d_mariadb_in_transaction(db));
    D_TESTS_MARIADB_OK(db, d_mariadb_get_autocommit(db, &enabled));
    D_TESTS_MARIADB_CHECK(enabled);

    D_TESTS_MARIADB_RESULT(db, d_mariadb_set_autocommit(NULL, true),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_get_autocommit(db, NULL),
                           D_DB_INVALID_ARGUMENT);

    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_isolation_and_waits
  Isolation levels and lock waits.
  Tests the following:
  - every isolation level is set and read back
  - a lock wait is set and read back; 0 and past the ceiling are refused
  - values outside the enumeration, NULL handles and outputs
*/
static void
d_tests_mariadb_isolation_and_waits(
    void
)
{
    struct d_mariadb* const  db      = d_tests_mariadb_connect(NULL);
    enum d_mariadb_isolation level   = D_MARIADB_ISOLATION_SERIALIZABLE;
    unsigned                 seconds = 0u;

    printf("mariadb_settings: isolation and lock waits\n");

    for (int i = D_MARIADB_ISOLATION_READ_UNCOMMITTED;
         i <= D_MARIADB_ISOLATION_SERIALIZABLE;
         ++i)
    {
        D_TESTS_MARIADB_OK(db,
                           d_mariadb_set_isolation_level(
                               db,
                               (enum d_mariadb_isolation)i));
        D_TESTS_MARIADB_OK(db, d_mariadb_get_isolation_level(db, &level));
        D_TESTS_MARIADB_CHECK(level == (enum d_mariadb_isolation)i);
    }

    D_TESTS_MARIADB_OK(db, d_mariadb_set_lock_wait_timeout(db, 7u));
    D_TESTS_MARIADB_OK(db, d_mariadb_get_lock_wait_timeout(db, &seconds));
    D_TESTS_MARIADB_CHECK(seconds == 7u);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_set_lock_wait_timeout(db, 0u),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_set_lock_wait_timeout(db, 1073741825u),
                           D_DB_INVALID_ARGUMENT);

    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_set_isolation_level(
                               db,
                               (enum d_mariadb_isolation)4),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_get_isolation_level(db, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_get_lock_wait_timeout(NULL, &seconds),
                           D_DB_INVALID_ARGUMENT);

    d_mariadb_close(db);

    return;
}

/*
d_tests_mariadb_next_isolation
  The next transaction's isolation level.
  Tests the following:
  - READ COMMITTED for the next transaction sees another's commit between two
    reads; the transaction after it is the session's, REPEATABLE READ, again
  - inside a transaction the server refuses it with 1568
  - a value outside the enumeration, a NULL handle
*/
static void
d_tests_mariadb_next_isolation(
    void
)
{
    char                     schema[96] = "";
    struct d_mariadb*        db         = d_tests_mariadb_begin("isolate",
                                                                schema,
                                                                sizeof(schema));
    struct d_mariadb* const  other      = d_tests_mariadb_connect(schema);

    printf("mariadb_settings: the next transaction's isolation\n");

    D_TESTS_MARIADB_OK(db,
                       d_mariadb_set_isolation_level(
                           db,
                           D_MARIADB_ISOLATION_REPEATABLE_READ));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec(db,
                                      "CREATE TABLE i (v INT) ENGINE=InnoDB"));
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "INSERT INTO i VALUES (1)"));

    // READ COMMITTED for this one transaction
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_set_next_isolation_level(
                           db,
                           D_MARIADB_ISOLATION_READ_COMMITTED));
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_begin_transaction(
                           db,
                           D_MARIADB_TRANSACTION_READ_WRITE));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT v FROM i") == 1);
    D_TESTS_MARIADB_OK(other, d_mariadb_exec(other, "UPDATE i SET v = 2"));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT v FROM i") == 2);

    // refused inside a transaction
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_set_next_isolation_level(
                               db,
                               D_MARIADB_ISOLATION_SERIALIZABLE),
                           1568);
    D_TESTS_MARIADB_OK(db, d_mariadb_commit(db));

    // the session's level again
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_begin_transaction(
                           db,
                           D_MARIADB_TRANSACTION_READ_WRITE));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT v FROM i") == 2);
    D_TESTS_MARIADB_OK(other, d_mariadb_exec(other, "UPDATE i SET v = 3"));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT v FROM i") == 2);
    D_TESTS_MARIADB_OK(db, d_mariadb_commit(db));

    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_set_next_isolation_level(
                               db,
                               (enum d_mariadb_isolation)4),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(NULL,
                           d_mariadb_set_next_isolation_level(
                               NULL,
                               D_MARIADB_ISOLATION_READ_COMMITTED),
                           D_DB_INVALID_ARGUMENT);

    d_mariadb_close(other);
    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_session_settings
  The character set, time zone and SQL mode.
  Tests the following:
  - each is set and read back; the character set's reaches the session too
  - an unknown character set is 2019, an unknown zone 1298, an unknown mode
    1231
  - NULL arguments
*/
static void
d_tests_mariadb_session_settings(
    void
)
{
    struct d_mariadb* const db   = d_tests_mariadb_connect(NULL);
    char*                   text = NULL;

    printf("mariadb_settings: session settings\n");

    D_TESTS_MARIADB_OK(db, d_mariadb_set_charset(db, "latin1"));
    D_TESTS_MARIADB_OK(db, d_mariadb_get_charset(db, &text));
    D_TESTS_MARIADB_CHECK( (text) && (strcmp(text, "latin1") == 0) );
    d_db_free(text);
    text = NULL;
    D_TESTS_MARIADB_CHECK(
        d_tests_mariadb_text_is(db, "SELECT @@character_set_client",
                                "latin1"));
    D_TESTS_MARIADB_OK(db, d_mariadb_set_charset(db, "utf8mb4"));
    D_TESTS_MARIADB_RESULT(db, d_mariadb_set_charset(db, "no_such_charset"),
                           2019);

    D_TESTS_MARIADB_OK(db, d_mariadb_set_time_zone(db, "+02:00"));
    D_TESTS_MARIADB_OK(db, d_mariadb_get_time_zone(db, &text));
    D_TESTS_MARIADB_CHECK( (text) && (strcmp(text, "+02:00") == 0) );
    d_db_free(text);
    text = NULL;
    D_TESTS_MARIADB_RESULT(db, d_mariadb_set_time_zone(db, "Mars/Base"),
                           1298);

    D_TESTS_MARIADB_OK(db, d_mariadb_set_sql_mode(db, ""));
    D_TESTS_MARIADB_OK(db, d_mariadb_get_sql_mode(db, &text));
    D_TESTS_MARIADB_CHECK( (text) && (strcmp(text, "") == 0) );
    d_db_free(text);
    text = NULL;
    D_TESTS_MARIADB_OK(db, d_mariadb_set_sql_mode(db, "ANSI_QUOTES"));
    D_TESTS_MARIADB_OK(db, d_mariadb_get_sql_mode(db, &text));
    D_TESTS_MARIADB_CHECK( (text) && (strstr(text, "ANSI_QUOTES") != NULL) );
    d_db_free(text);
    text = NULL;
    D_TESTS_MARIADB_RESULT(db, d_mariadb_set_sql_mode(db, "NO_SUCH_MODE"),
                           1231);

    D_TESTS_MARIADB_RESULT(db, d_mariadb_set_charset(db, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_get_charset(db, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_set_time_zone(db, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_get_time_zone(NULL, &text),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_set_sql_mode(NULL, ""),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_get_sql_mode(db, NULL),
                           D_DB_INVALID_ARGUMENT);

    d_mariadb_close(db);

    return;
}

/*
d_tests_mariadb_variables
  Any session variable.
  Tests the following:
  - set from SQL and read back as text
  - an unknown variable is 1193, both ways
  - names that are not a variable's shape are refused before the server
    sees them; NULL values and outputs too
*/
static void
d_tests_mariadb_variables(
    void
)
{
    struct d_mariadb* const  db      = d_tests_mariadb_connect(NULL);
    char*                    text    = NULL;
    static const char* const unfit[] =
    {
        "", "1a", "a b", "a;b", "@@a", "a.b", "a-b", "a`b"
    };

    printf("mariadb_settings: any variable\n");

    D_TESTS_MARIADB_OK(db, d_mariadb_set_variable(db, "wait_timeout", "1234"));
    D_TESTS_MARIADB_OK(db, d_mariadb_get_variable(db, "wait_timeout", &text));
    D_TESTS_MARIADB_CHECK( (text) && (strcmp(text, "1234") == 0) );
    d_db_free(text);
    text = NULL;
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_set_variable(db, "sql_mode", "'TRADITIONAL'"));
    D_TESTS_MARIADB_OK(db, d_mariadb_get_variable(db, "sql_mode", &text));
    D_TESTS_MARIADB_CHECK( (text) && (strstr(text, "STRICT") != NULL) );
    d_db_free(text);
    text = NULL;
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_set_variable(db, "no_such_variable", "1"),
                           1193);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_get_variable(db, "no_such_variable",
                                                  &text),
                           1193);

    for (size_t i = 0u; i < sizeof(unfit) / sizeof(unfit[0]); ++i)
    {
        D_TESTS_MARIADB_RESULT(db, d_mariadb_set_variable(db, unfit[i], "1"),
                               D_DB_INVALID_ARGUMENT);
        D_TESTS_MARIADB_RESULT(db, d_mariadb_get_variable(db, unfit[i], &text),
                               D_DB_INVALID_ARGUMENT);
    }

    D_TESTS_MARIADB_RESULT(db, d_mariadb_set_variable(db, NULL, "1"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_set_variable(db, "wait_timeout", NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_get_variable(db, "wait_timeout", NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_get_variable(NULL, "wait_timeout",
                                                      &text),
                           D_DB_INVALID_ARGUMENT);

    d_mariadb_close(db);

    return;
}

/*
d_tests_mariadb_session_tracking
  The session's changes, as the server reports them.
  Tests the following:
  - tracking is supported exactly where the server and the client have it
  - where it is: a variable the session asks for is reported with its value,
    by a statement and by a prepared one; the current database, changed by
    USE, and by the client's call through libmysqlclient, which reads its
    reply; the state, and the transaction's, once the session asks for them
  - nothing for a statement that changes nothing tracked, one that returns
    rows, one that fails, a ping, a prepare -- through either client
  - where it is not, D_DB_UNSUPPORTED, the list untouched
  - NULL arguments, and a kind outside the enumeration
*/
static void
d_tests_mariadb_session_tracking(
    void
)
{
    static const char* const zone_2[]  = { "time_zone", "+02:00" };
    static const char* const zone_3[]  = { "time_zone", "+03:00" };
    static const char* const zone_6[]  = { "time_zone", "+06:00" };
    static const char* const changed[] = { "1" };
    static const char* const named[]   = { "information_schema" };
    static const char* const begun[]   = { "T_______" };
    static const char* const ended[]   = { "________" };
    static const char* const opening[] = { "START TRANSACTION READ ONLY;" };
    static const char* const nothing[] = { "" };
    char                     schema[96] = "";
    struct d_mariadb*        db         = NULL;
    struct d_db_name_list    entries    = { 7u, NULL };
    bool                     can        = false;

    printf("mariadb_settings: the session's changes\n");

    db  = d_tests_mariadb_begin("tracking", schema, sizeof(schema));
    can = d_mariadb_supports(db, D_MARIADB_FEATURE_SESSION_TRACKING);

    D_TESTS_MARIADB_CHECK(
        can ==
        ( (d_tests_mariadb_client_has(D_MARIADB_FEATURE_SESSION_TRACKING)) &&
          ( (d_mariadb_server_at_least(db,
                                       D_MARIADB_FLAVOR_MARIADB,
                                       10u, 2u, 2u)) ||
            (d_tests_mariadb_mysql_at_least(db, 5u, 7u, 4u)) ) ));

    if (can)
    {
        const char* const          own[1]    = { schema };
        struct d_mariadb_statement* statement = NULL;
        int64_t                     changes   = 0;

        // a variable the session asks for, with its value
        D_TESTS_MARIADB_OK(db,
                           d_mariadb_exec(db,
                                          "SET SESSION "
                                          "session_track_system_variables = "
                                          "'time_zone'"));
        D_TESTS_MARIADB_OK(db,
                           d_mariadb_exec(db,
                                          "SET SESSION time_zone = '+02:00'"));
        D_TESTS_MARIADB_CHECK(
            d_tests_mariadb_tracked(db, D_MARIADB_TRACK_SYSTEM_VARIABLES,
                                    2u, zone_2));
        D_TESTS_MARIADB_CHECK(
            d_tests_mariadb_tracked(db, D_MARIADB_TRACK_SCHEMA, 0u, NULL));

        // nothing tracked changed; rows; a failure; a ping
        D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "SET @d_tracked = 1"));
        D_TESTS_MARIADB_CHECK(
            d_tests_mariadb_tracked(db, D_MARIADB_TRACK_SYSTEM_VARIABLES,
                                    0u, NULL));
        D_TESTS_MARIADB_OK(db,
                           d_mariadb_exec(db,
                                          "SET SESSION time_zone = '+03:00'"));
        D_TESTS_MARIADB_CHECK(
            d_tests_mariadb_tracked(db, D_MARIADB_TRACK_SYSTEM_VARIABLES,
                                    2u, zone_3));
        D_TESTS_MARIADB_CHECK(d_tests_mariadb_int(db, "SELECT 1") == 1);
        D_TESTS_MARIADB_CHECK(
            d_tests_mariadb_tracked(db, D_MARIADB_TRACK_SYSTEM_VARIABLES,
                                    0u, NULL));
        D_TESTS_MARIADB_OK(db,
                           d_mariadb_exec(db,
                                          "SET SESSION time_zone = '+04:00'"));
        D_TESTS_MARIADB_CHECK(d_mariadb_exec(db, "SELEC 1") != 0);
        D_TESTS_MARIADB_CHECK(
            d_tests_mariadb_tracked(db, D_MARIADB_TRACK_SYSTEM_VARIABLES,
                                    0u, NULL));
        D_TESTS_MARIADB_OK(db,
                           d_mariadb_exec(db,
                                          "SET SESSION time_zone = '+05:00'"));
        D_TESTS_MARIADB_OK(db, d_mariadb_ping(db));
        D_TESTS_MARIADB_CHECK(
            d_tests_mariadb_tracked(db, D_MARIADB_TRACK_SYSTEM_VARIABLES,
                                    0u, NULL));

        // a prepared statement: its prepare reports nothing, its run the
        // change
        D_TESTS_MARIADB_OK(db,
                           d_mariadb_prepare(db,
                                             &statement,
                                             "SET SESSION time_zone = "
                                             "'+06:00'"));
        D_TESTS_MARIADB_CHECK(
            d_tests_mariadb_tracked(db, D_MARIADB_TRACK_SYSTEM_VARIABLES,
                                    0u, NULL));
        D_TESTS_MARIADB_OK(db, d_mariadb_execute_update(statement, &changes));
        D_TESTS_MARIADB_CHECK(
            d_tests_mariadb_tracked(db, D_MARIADB_TRACK_SYSTEM_VARIABLES,
                                    2u, zone_6));
        D_TESTS_MARIADB_OK(db, d_mariadb_finalize(statement));

        // the current database, by the client's call -- whose reply
        // Connector/C does not read -- and by USE
        D_TESTS_MARIADB_OK(db,
                           d_mariadb_use_database(db, "information_schema"));
        D_TESTS_MARIADB_CHECK(
            d_tests_mariadb_tracked(db,
                                    D_MARIADB_TRACK_SCHEMA,
                                    (d_mariadb_client_is_mariadb()) ? 0u : 1u,
                                    named));
        D_TESTS_MARIADB_OK(db, d_mariadb_execf(db, "USE %I", schema));
        D_TESTS_MARIADB_CHECK(
            d_tests_mariadb_tracked(db, D_MARIADB_TRACK_SCHEMA, 1u, own));

        // the state, once asked for
        D_TESTS_MARIADB_OK(db,
                           d_mariadb_exec(db,
                                          "SET SESSION "
                                          "session_track_state_change = ON"));
        D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "SET @d_tracked = 2"));
        D_TESTS_MARIADB_CHECK(
            d_tests_mariadb_tracked(db, D_MARIADB_TRACK_STATE_CHANGE,
                                    1u, changed));

        // the transaction's, where the server has it: MySQL from 5.7.8,
        // MariaDB from 10.3.1
        if ( (d_mariadb_server_at_least(db,
                                        D_MARIADB_FLAVOR_MARIADB,
                                        10u, 3u, 1u)) ||
             (d_tests_mariadb_mysql_at_least(db, 5u, 7u, 8u)) )
        {
            D_TESTS_MARIADB_OK(db,
                               d_mariadb_exec(db,
                                              "SET SESSION "
                                              "session_track_transaction_info "
                                              "= 'CHARACTERISTICS'"));
            D_TESTS_MARIADB_OK(db,
                               d_mariadb_exec(db,
                                              "START TRANSACTION READ ONLY"));
            D_TESTS_MARIADB_CHECK(
                d_tests_mariadb_tracked(db, D_MARIADB_TRACK_TRANSACTION_STATE,
                                        1u, begun));
            D_TESTS_MARIADB_CHECK(
                d_tests_mariadb_tracked(
                    db, D_MARIADB_TRACK_TRANSACTION_CHARACTERISTICS,
                    1u, opening));
            D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "COMMIT"));
            D_TESTS_MARIADB_CHECK(
                d_tests_mariadb_tracked(db, D_MARIADB_TRACK_TRANSACTION_STATE,
                                        1u, ended));
            D_TESTS_MARIADB_CHECK(
                d_tests_mariadb_tracked(
                    db, D_MARIADB_TRACK_TRANSACTION_CHARACTERISTICS,
                    1u, nothing));
        }

        D_TESTS_MARIADB_RESULT(db,
                               d_mariadb_session_track(
                                   db,
                                   (enum d_mariadb_track)99,
                                   &entries),
                               D_DB_INVALID_ARGUMENT);
    }
    else
    {
        D_TESTS_MARIADB_RESULT(db,
                               d_mariadb_session_track(db,
                                                       D_MARIADB_TRACK_SCHEMA,
                                                       &entries),
                               D_DB_UNSUPPORTED);
    }

    D_TESTS_MARIADB_CHECK( (entries.count == 7u) &&
                           (entries.names == NULL) );
    D_TESTS_MARIADB_RESULT(NULL,
                           d_mariadb_session_track(NULL,
                                                   D_MARIADB_TRACK_SCHEMA,
                                                   &entries),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_session_track(db,
                                                   D_MARIADB_TRACK_SCHEMA,
                                                   NULL),
                           D_DB_INVALID_ARGUMENT);

    d_tests_mariadb_end(db, schema);

    return;
}

//==============================================================================
// mariadb_maintenance.h
//==============================================================================

/*
d_tests_mariadb_make_kept
  A database of the test's own holding InnoDB tables m and m2 of the same
rows, m3 of others, a view mv over m, and mem, a MEMORY table.
*/
static struct d_mariadb*
d_tests_mariadb_make_kept(
    const char* _word,
    char*       _schema,
    size_t      _size
)
{
    struct d_mariadb* db = d_tests_mariadb_begin(_word, _schema, _size);

    D_TESTS_MARIADB_OK(db,
                       d_mariadb_exec_script(db,
                                             "CREATE TABLE m (a INT PRIMARY "
                                             "KEY, b TEXT) ENGINE = InnoDB; "
                                             "INSERT INTO m VALUES (1, 'x'), "
                                             "(2, 'y'); "
                                             "CREATE TABLE m2 LIKE m; "
                                             "INSERT INTO m2 SELECT * FROM m; "
                                             "CREATE TABLE m3 LIKE m; "
                                             "INSERT INTO m3 VALUES (1, 'z'); "
                                             "CREATE VIEW mv AS SELECT * FROM "
                                             "m; "
                                             "CREATE TABLE mem (a INT) "
                                             "ENGINE = MEMORY"));

    return db;
}

/*
d_tests_mariadb_upkeep
  Analyze, optimize, check and checksum.
  Tests the following:
  - each succeeds on an InnoDB table, in the current database or a named
    one; the table is sound
  - tables holding the same rows share a checksum, and others do not
*/
static void
d_tests_mariadb_upkeep(
    void
)
{
    char              schema[96] = "";
    struct d_mariadb* db         = d_tests_mariadb_make_kept("upkeep",
                                                             schema,
                                                             sizeof(schema));
    bool              ok         = false;
    uint64_t          first      = 0u;
    uint64_t          second     = 1u;
    uint64_t          third      = 0u;

    printf("mariadb_maintenance: upkeep\n");

    D_TESTS_MARIADB_OK(db, d_mariadb_analyze_table(db, NULL, "m"));
    D_TESTS_MARIADB_OK(db, d_mariadb_optimize_table(db, schema, "m"));
    D_TESTS_MARIADB_OK(db, d_mariadb_check_table(db, NULL, "m", &ok));
    D_TESTS_MARIADB_CHECK(ok);
    D_TESTS_MARIADB_OK(db, d_mariadb_checksum_table(db, NULL, "m", &first));
    D_TESTS_MARIADB_OK(db, d_mariadb_checksum_table(db, NULL, "m2", &second));
    D_TESTS_MARIADB_OK(db, d_mariadb_checksum_table(db, NULL, "m3", &third));
    D_TESTS_MARIADB_CHECK(first == second);
    D_TESTS_MARIADB_CHECK(first != third);

    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_upkeep_refusals
  Where upkeep cannot be done.
  Tests the following:
  - a missing table, and a view, are D_DB_NOT_FOUND, the answers untouched
  - an engine without the operation is D_DB_UNSUPPORTED; CHECKSUM works on
    any
  - NULL arguments
*/
static void
d_tests_mariadb_upkeep_refusals(
    void
)
{
    char              schema[96] = "";
    struct d_mariadb* db         = d_tests_mariadb_make_kept("refused",
                                                             schema,
                                                             sizeof(schema));
    bool              ok         = true;
    uint64_t          checksum   = 7u;

    printf("mariadb_maintenance: upkeep refusals\n");

    D_TESTS_MARIADB_RESULT(db, d_mariadb_analyze_table(db, NULL, "none"),
                           D_DB_NOT_FOUND);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_optimize_table(db, NULL, "none"),
                           D_DB_NOT_FOUND);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_check_table(db, NULL, "none", &ok),
                           D_DB_NOT_FOUND);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_check_table(db, NULL, "mv", &ok),
                           D_DB_NOT_FOUND);
    D_TESTS_MARIADB_RESULT(db,
                           d_mariadb_checksum_table(db, NULL, "mv", &checksum),
                           D_DB_NOT_FOUND);
    D_TESTS_MARIADB_CHECK( (ok) && (checksum == 7u) );

    // an engine without them
    D_TESTS_MARIADB_RESULT(db, d_mariadb_analyze_table(db, NULL, "mem"),
                           D_DB_UNSUPPORTED);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_optimize_table(db, NULL, "mem"),
                           D_DB_UNSUPPORTED);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_check_table(db, NULL, "mem", &ok),
                           D_DB_UNSUPPORTED);
    D_TESTS_MARIADB_OK(db,
                       d_mariadb_checksum_table(db, NULL, "mem", &checksum));
    D_TESTS_MARIADB_CHECK(checksum == 0u);

    // arguments
    D_TESTS_MARIADB_RESULT(db, d_mariadb_check_table(db, NULL, "m", NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_checksum_table(db, NULL, "m", NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_analyze_table(NULL, NULL, "m"),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_optimize_table(db, "", "m"),
                           D_DB_INVALID_ARGUMENT);

    d_tests_mariadb_end(db, schema);

    return;
}

//==============================================================================
// mariadb_connection.h and mariadb_query.h -- without blocking
//==============================================================================

// D_TESTS_MARIADB_ROUNDS
//   constant: the most waits a call that does not block is driven through
// before the harness gives up on it, so that one that never finishes fails
// rather than hangs.
#define D_TESTS_MARIADB_ROUNDS 100000

/*
d_tests_mariadb_async
  Whether the client makes calls that do not block: MariaDB's, and
libmysqlclient from 8.0.16; and whether it makes all of them, as MariaDB's
does.
*/
static bool
d_tests_mariadb_async(
    bool _all
)
{
    return (d_mariadb_client_is_mariadb())
               ? true
               : ( (!_all) &&
                   (d_mariadb_client_version_number() >= 80016ul) );
}

/*
d_tests_mariadb_wait
  Waits as a pending call says, with poll(2), or WSAPoll on Windows, and
answers what came, as d_db_wait; trouble on the socket reads as ready for
what was asked, and no socket as everything having come, for the call to
find out.
*/
static unsigned
d_tests_mariadb_wait(
    const struct d_db_pending* _pending
)
{
    const unsigned ready   = (unsigned)D_DB_WAIT_READ |
                             (unsigned)D_DB_WAIT_WRITE;
    const int      timeout = ((_pending->events & D_DB_WAIT_TIMEOUT) != 0u)
                                 ? ( (_pending->timeout_ms > (unsigned)INT_MAX)
                                         ? INT_MAX
                                         : (int)_pending->timeout_ms )
                                 : -1;
    struct pollfd  fd;

    // no socket to wait on
    if (_pending->socket == D_DB_SOCKET_NONE)
    {
        return _pending->events & ready;
    }

    memset(&fd, 0, sizeof(fd));
    fd.fd     = (d_tests_mariadb_socket)_pending->socket;
    fd.events = (short)( ( ((_pending->events & D_DB_WAIT_READ) != 0u)
                               ? POLLIN
                               : 0 ) |
                         ( ((_pending->events & D_DB_WAIT_WRITE) != 0u)
                               ? POLLOUT
                               : 0 ) );

    const int came = d_tests_mariadb_poll(&fd, 1u, timeout);

    // the deadline first
    if (came == 0)
    {
        return (unsigned)D_DB_WAIT_TIMEOUT;
    }

    // trouble on the socket is the call's to find
    if ( (came < 0) ||
         ((fd.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) )
    {
        return _pending->events & ready;
    }

    return ( ((fd.revents & POLLIN) != 0)  ? (unsigned)D_DB_WAIT_READ  : 0u ) |
           ( ((fd.revents & POLLOUT) != 0) ? (unsigned)D_DB_WAIT_WRITE : 0u );
}

/*
d_tests_mariadb_drive_connect
  Continues a connect until it is done; the answer is the last
continuation's.
*/
static int
d_tests_mariadb_drive_connect(
    int                  _started,
    struct d_mariadb**   _db,
    struct d_db_pending* _pending,
    char**               _message
)
{
    int result = _started;

    for (int waits = 0;
         (result == D_DB_WAIT) && (waits < D_TESTS_MARIADB_ROUNDS);
         ++waits)
    {
        d_db_free((_message) ? *_message : NULL);
        result = d_mariadb_connect_continue(_db,
                                            d_tests_mariadb_wait(_pending),
                                            _pending,
                                            _message);
    }

    return result;
}

/*
d_tests_mariadb_drive_send
  Continues a send until it is done.
*/
static int
d_tests_mariadb_drive_send(
    int                  _started,
    struct d_mariadb*    _db,
    struct d_db_pending* _pending
)
{
    int result = _started;

    for (int waits = 0;
         (result == D_DB_WAIT) && (waits < D_TESTS_MARIADB_ROUNDS);
         ++waits)
    {
        result = d_mariadb_send_query_continue(_db,
                                               d_tests_mariadb_wait(_pending),
                                               _pending);
    }

    return result;
}

/*
d_tests_mariadb_drive_next
  Continues a result's read until it is done.
*/
static int
d_tests_mariadb_drive_next(
    int                  _started,
    struct d_mariadb*    _db,
    struct d_db_pending* _pending,
    struct d_db_rows*    _rows,
    int64_t*             _changes
)
{
    int result = _started;

    for (int waits = 0;
         (result == D_DB_WAIT) && (waits < D_TESTS_MARIADB_ROUNDS);
         ++waits)
    {
        result = d_mariadb_next_result_continue(
                     _db,
                     d_tests_mariadb_wait(_pending),
                     _pending,
                     _rows,
                     _changes);
    }

    return result;
}

/*
d_tests_mariadb_next
  The next result of SQL sent, read without blocking from start to end.
*/
static int
d_tests_mariadb_next(
    struct d_mariadb* _db,
    struct d_db_rows* _rows,
    int64_t*          _changes
)
{
    struct d_db_pending pending;

    return d_tests_mariadb_drive_next(d_mariadb_next_result_start(_db,
                                                                  &pending,
                                                                  _rows,
                                                                  _changes),
                                      _db,
                                      &pending,
                                      _rows,
                                      _changes);
}

/*
d_tests_mariadb_send
  SQL sent without blocking, from start to end.
*/
static int
d_tests_mariadb_send(
    struct d_mariadb* _db,
    const char*       _sql
)
{
    struct d_db_pending pending;

    return d_tests_mariadb_drive_send(d_mariadb_send_query_start(_db,
                                                                 _sql,
                                                                 &pending),
                                      _db,
                                      &pending);
}

/*
d_tests_mariadb_first_int
  A result's first value as an integer, or INT64_MIN.
*/
static int64_t
d_tests_mariadb_first_int(
    const struct d_db_rows* _rows
)
{
    int64_t value = INT64_MIN;

    // the value, or the sentinel
    if (d_db_value_int64(d_db_rows_value(_rows, 0u, 0u), &value) != 0)
    {
        return INT64_MIN;
    }

    return value;
}

/*
d_tests_mariadb_unpended
  Whether a pending call was left as a finished one leaves it.
*/
static bool
d_tests_mariadb_unpended(
    const struct d_db_pending* _pending
)
{
    return ( (_pending->socket == D_DB_SOCKET_NONE) &&
             (_pending->events == 0u)               &&
             (_pending->timeout_ms == 0u)           &&
             (_pending->deadline == 0.0)            &&
             (_pending->state == 0u) );
}

/*
d_tests_mariadb_plain_first
  A test's one setting, behind g_plain while the run's connections go
without TLS: `_two` filled, the first to pass answered, how many in
`_count` -- called before the connect, which must not read the count in the
same expression.
*/
static const struct d_mariadb_setting*
d_tests_mariadb_plain_first(
    struct d_mariadb_setting*       _two,
    const struct d_mariadb_setting* _one,
    size_t*                         _count
)
{
    _two[0] = g_plain;
    _two[1] = *_one;
    *_count = 1u + g_plain_count;

    return &_two[1u - g_plain_count];
}

/*
d_tests_mariadb_connect_nonblocking
  d_mariadb_connect_start, _connect_with_start and _connect_continue, each
connection without TLS while g_plain_count says so.
  Tests the following:
  - with a client that can, a start is D_DB_WAIT, the handle in hand, the
    socket to wait on -- or 0, where every answer came at once; driven
    through a poll loop, it connects, set up as a blocking connect: the
    character set the layer names, the server described; with one that
    cannot, D_DB_UNSUPPORTED, no handle
  - settings apply: an init command has run; a character set a setting
    names is the session's
  - a wrong password is 1045 with the client's reason, the handle NULL
  - a connect abandoned is closed with d_mariadb_close
  - a server that never answers is 2013 once the connect timeout passes:
    MariaDB's client keeps the timeout, and the layer keeps libmysqlclient's
  - NULL arguments are D_DB_INVALID_ARGUMENT
*/
static void
d_tests_mariadb_connect_nonblocking(
    void
)
{
    struct d_mariadb*              db       = NULL;
    char*                          reason   = NULL;
    struct d_db_pending            pending;
    struct d_mariadb_options       options  = g_options;
    const struct d_mariadb_setting init[1]  =
    {
        { D_MARIADB_SETTING_INIT_COMMAND, 0ul, "SET @d_tests_nb = 7", NULL }
    };
    struct d_mariadb_setting       two[2];
    size_t                         count    = 0u;
    const struct d_mariadb_setting* given   = NULL;
    int64_t                        number   = 0;

    printf("mariadb_connection: connecting without blocking\n");

    memset(&pending, 0x5a, sizeof(pending));

    // a client that cannot
    if (!d_tests_mariadb_async(false))
    {
        D_TESTS_MARIADB_RESULT(NULL,
                               d_mariadb_connect_start(&g_options,
                                                       &db,
                                                       &pending),
                               D_DB_UNSUPPORTED);
        D_TESTS_MARIADB_CHECK( (db == NULL) &&
                               (d_tests_mariadb_unpended(&pending)) );

        return;
    }

    const int started = d_mariadb_connect_with_start(&g_options,
                                                     &g_plain,
                                                     g_plain_count,
                                                     &db,
                                                     &pending,
                                                     NULL);

    // under way, the handle in hand and a socket to wait on; or, where
    // every answer came at once, made
    D_TESTS_MARIADB_CHECK( (db != NULL) &&
                           ( (started == 0) ||
                             ( (started == D_DB_WAIT) &&
                               (pending.socket != D_DB_SOCKET_NONE) &&
                               ((pending.events &
                                 ((unsigned)D_DB_WAIT_READ |
                                  (unsigned)D_DB_WAIT_WRITE)) != 0u) ) ) );
    D_TESTS_MARIADB_OK(db, d_tests_mariadb_drive_connect(started,
                                                         &db,
                                                         &pending,
                                                         &reason));
    D_TESTS_MARIADB_CHECK( (db != NULL) &&
                           (reason == NULL) &&
                           (d_tests_mariadb_unpended(&pending)) );

    // set up as a blocking connect sets it up
    if (db)
    {
        char* charset = NULL;

        D_TESTS_MARIADB_OK(db, d_mariadb_ping(db));
        D_TESTS_MARIADB_CHECK(d_mariadb_server_version_number(db) > 0ul);
        D_TESTS_MARIADB_OK(db, d_mariadb_query_text(
                                   db,
                                   &charset,
                                   "SELECT @@character_set_client"));
        D_TESTS_MARIADB_CHECK( (charset) &&
                               (strcmp(charset, D_CFG_MARIADB_CHARSET) ==
                                0) );
        d_db_free(charset);
    }

    d_mariadb_close(db);
    db = NULL;

    // the settings, an init command among them
    given = d_tests_mariadb_plain_first(two, init, &count);
    D_TESTS_MARIADB_OK(NULL, d_tests_mariadb_drive_connect(
                                 d_mariadb_connect_with_start(&g_options,
                                                              given,
                                                              count,
                                                              &db,
                                                              &pending,
                                                              NULL),
                                 &db,
                                 &pending,
                                 NULL));
    D_TESTS_MARIADB_CHECK( (db) &&
                           (d_mariadb_query_int64(db,
                                                  &number,
                                                  "SELECT @d_tests_nb") ==
                            0) &&
                           (number == 7) );
    d_mariadb_close(db);
    db = NULL;

    // a character set a setting names, the session's with every pairing
    {
        const struct d_mariadb_setting latin1 =
        {
            D_MARIADB_SETTING_CHARSET, 0ul, "latin1", NULL
        };

        given = d_tests_mariadb_plain_first(two, &latin1, &count);
        D_TESTS_MARIADB_OK(NULL, d_tests_mariadb_drive_connect(
                                     d_mariadb_connect_with_start(&g_options,
                                                                  given,
                                                                  count,
                                                                  &db,
                                                                  &pending,
                                                                  NULL),
                                     &db,
                                     &pending,
                                     NULL));
        D_TESTS_MARIADB_CHECK( (db) &&
                               (d_tests_mariadb_text_is(
                                    db,
                                    "SELECT @@character_set_client",
                                    "latin1")) );
        d_mariadb_close(db);
        db = NULL;
    }

    // a wrong password
    options.password = "certainly not the password";
    D_TESTS_MARIADB_RESULT(NULL,
                           d_tests_mariadb_drive_connect(
                               d_mariadb_connect_with_start(&options,
                                                            &g_plain,
                                                            g_plain_count,
                                                            &db,
                                                            &pending,
                                                            &reason),
                               &db,
                               &pending,
                               &reason),
                           1045);
    D_TESTS_MARIADB_CHECK( (db == NULL) &&
                           (reason) &&
                           (strstr(reason, "denied") != NULL) &&
                           (d_tests_mariadb_unpended(&pending)) );
    d_db_free(reason);
    reason = NULL;

    // abandoned while being made, or made at once
    D_TESTS_MARIADB_CHECK( (d_mariadb_connect_with_start(&g_options,
                                                         &g_plain,
                                                         g_plain_count,
                                                         &db,
                                                         &pending,
                                                         NULL) <= 0) &&
                           (d_mariadb_close(db) == 0) );
    db = NULL;

#if !defined(_WIN32)
    // a server that never answers, within the connect timeout: MariaDB's
    // client keeps it, and the layer keeps libmysqlclient's
    {
        struct sockaddr_in             address;
        socklen_t                      length  = (socklen_t)sizeof(address);
        const int                      mute    = socket(AF_INET,
                                                        SOCK_STREAM,
                                                        0);
        const struct d_mariadb_setting brief[1] =
        {
            { D_MARIADB_SETTING_CONNECT_TIMEOUT, 2ul, NULL, NULL }
        };

        memset(&address, 0, sizeof(address));
        address.sin_family      = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        if ( (mute >= 0) &&
             (bind(mute, (struct sockaddr*)&address, length) == 0) &&
             (listen(mute, 4) == 0) &&
             (getsockname(mute, (struct sockaddr*)&address, &length) ==
              0) )
        {
            const time_t began = time(NULL);

            given            = d_tests_mariadb_plain_first(two,
                                                           brief,
                                                           &count);
            options          = g_options;
            options.host     = "127.0.0.1";
            options.port     = (unsigned)ntohs(address.sin_port);
            D_TESTS_MARIADB_RESULT(
                NULL,
                d_tests_mariadb_drive_connect(
                    d_mariadb_connect_with_start(&options,
                                                 given,
                                                 count,
                                                 &db,
                                                 &pending,
                                                 &reason),
                    &db,
                    &pending,
                    &reason),
                2013);
            D_TESTS_MARIADB_CHECK( (db == NULL) &&
                                   (reason) &&
                                   (time(NULL) - began >= 1) &&
                                   (time(NULL) - began <= 4) );
            d_db_free(reason);
            reason = NULL;
        }

        if (mute >= 0)
        {
            (void)close(mute);
        }
    }
#endif  // !_WIN32

    // arguments
    D_TESTS_MARIADB_RESULT(NULL,
                           d_mariadb_connect_start(&g_options, NULL, &pending),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(NULL,
                           d_mariadb_connect_start(&g_options, &db, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(NULL,
                           d_mariadb_connect_with_start(&g_options,
                                                        NULL,
                                                        1u,
                                                        &db,
                                                        &pending,
                                                        NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(NULL,
                           d_mariadb_connect_continue(NULL,
                                                      0u,
                                                      &pending,
                                                      NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(NULL,
                           d_mariadb_connect_continue(&db,
                                                      0u,
                                                      &pending,
                                                      NULL),
                           D_DB_INVALID_ARGUMENT);

    return;
}

/*
d_tests_mariadb_sent_nonblocking
  d_mariadb_send_query_start, _continue, d_mariadb_next_result_start and
_continue.
  Tests the following:
  - a statement sent gives its result -- empty rows and a count for one
    without rows, rows for a query -- then D_DB_NOT_FOUND, again
  - a procedure's several results come one a call, its status last
  - a result still to come is D_DB_WAIT, the socket to read; meanwhile every
    other command is 2014, a second send among them; an answer the server
    takes half a second over is waited for, never waited in, over TLS too
  - the first statement's failure is the first result, then nothing; a
    failure among a procedure's results ends them
  - SQL larger than the socket takes at once is sent as it takes it
  - nothing sent is D_DB_NOT_FOUND at once; NULL arguments are
    D_DB_INVALID_ARGUMENT; a client that cannot is D_DB_UNSUPPORTED
*/
static void
d_tests_mariadb_sent_nonblocking(
    void
)
{
    char                schema[80] = "";
    struct d_mariadb*   db         = d_tests_mariadb_begin("sentnb",
                                                           schema,
                                                           sizeof(schema));
    struct d_db_rows    rows;
    struct d_db_pending pending;
    int64_t             changes    = -1;
    const size_t        large      = (size_t)4u * 1024u * 1024u;
    char* const         sql        = (char*)malloc(large + 64u);

    printf("mariadb_query: statements sent without blocking\n");

    memset(&rows, 0, sizeof(rows));

    // nothing more without a server, or memory
    if ( (!db) ||
         (!sql) )
    {
        D_TESTS_MARIADB_CHECK( (db) && (sql) );
        free(sql);
        d_tests_mariadb_end(db, schema);

        return;
    }

    // a client that cannot
    if (!d_tests_mariadb_async(false))
    {
        D_TESTS_MARIADB_RESULT(db,
                               d_mariadb_send_query_start(db,
                                                          "SELECT 1",
                                                          &pending),
                               D_DB_UNSUPPORTED);
        free(sql);
        d_tests_mariadb_end(db, schema);

        return;
    }

    D_TESTS_MARIADB_RESULT(db, d_tests_mariadb_next(db, &rows, &changes),
                           D_DB_NOT_FOUND);
    D_TESTS_MARIADB_OK(db, d_tests_mariadb_send(db, "CREATE TABLE t (a int)"));
    D_TESTS_MARIADB_OK(db, d_tests_mariadb_next(db, &rows, &changes));
    D_TESTS_MARIADB_CHECK(rows.column_count == 0u);
    d_db_rows_free(&rows);
    D_TESTS_MARIADB_RESULT(db, d_tests_mariadb_next(db, &rows, &changes),
                           D_DB_NOT_FOUND);
    D_TESTS_MARIADB_RESULT(db, d_tests_mariadb_next(db, &rows, &changes),
                           D_DB_NOT_FOUND);
    D_TESTS_MARIADB_OK(db, d_tests_mariadb_send(
                               db,
                               "INSERT INTO t VALUES (1), (2)"));
    D_TESTS_MARIADB_OK(db, d_tests_mariadb_next(db, &rows, &changes));
    D_TESTS_MARIADB_CHECK( (rows.column_count == 0u) && (changes == 2) );
    d_db_rows_free(&rows);
    D_TESTS_MARIADB_RESULT(db, d_tests_mariadb_next(db, &rows, &changes),
                           D_DB_NOT_FOUND);
    D_TESTS_MARIADB_OK(db, d_tests_mariadb_send(db,
                                                "SELECT a FROM t ORDER BY a"));
    D_TESTS_MARIADB_OK(db, d_tests_mariadb_next(db, &rows, &changes));
    D_TESTS_MARIADB_CHECK( (rows.row_count == 2u) &&
                           (rows.column_count == 1u) &&
                           (strcmp(rows.columns[0].name, "a") == 0) &&
                           (d_tests_mariadb_first_int(&rows) == 1) );
    d_db_rows_free(&rows);
    D_TESTS_MARIADB_RESULT(db, d_tests_mariadb_next(db, &rows, &changes),
                           D_DB_NOT_FOUND);

    // a procedure's results, its status last
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(
                               db,
                               "CREATE PROCEDURE p() BEGIN SELECT 1 AS a; "
                               "SELECT 'x' AS s; END"));
    D_TESTS_MARIADB_OK(db, d_tests_mariadb_send(db, "CALL p()"));
    D_TESTS_MARIADB_OK(db, d_tests_mariadb_next(db, &rows, NULL));
    D_TESTS_MARIADB_CHECK( (rows.column_count == 1u) &&
                           (strcmp(rows.columns[0].name, "a") == 0) );
    d_db_rows_free(&rows);
    D_TESTS_MARIADB_OK(db, d_tests_mariadb_next(db, &rows, NULL));
    D_TESTS_MARIADB_CHECK( (rows.column_count == 1u) &&
                           (strcmp(rows.columns[0].name, "s") == 0) );
    d_db_rows_free(&rows);
    D_TESTS_MARIADB_OK(db, d_tests_mariadb_next(db, &rows, NULL));
    D_TESTS_MARIADB_CHECK(rows.column_count == 0u);
    d_db_rows_free(&rows);
    D_TESTS_MARIADB_RESULT(db, d_tests_mariadb_next(db, &rows, NULL),
                           D_DB_NOT_FOUND);

    // a result still to come, the connection held meanwhile
    D_TESTS_MARIADB_OK(db, d_tests_mariadb_send(db, "SELECT SLEEP(0.2), 7"));
    D_TESTS_MARIADB_RESULT(db, d_mariadb_exec(db, "SELECT 1"),
                           2014);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_send_query_start(db,
                                                          "SELECT 1",
                                                          &pending),
                           2014);
    D_TESTS_MARIADB_OK(db, d_tests_mariadb_next(db, &rows, NULL));
    D_TESTS_MARIADB_CHECK( (rows.row_count == 1u) &&
                           (rows.column_count == 2u) );
    d_db_rows_free(&rows);
    D_TESTS_MARIADB_RESULT(db, d_tests_mariadb_next(db, &rows, NULL),
                           D_DB_NOT_FOUND);
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "SELECT 1"));

    // an answer still to come is waited for, never waited in: MariaDB's
    // client's result, and libmysqlclient's send, which reads the answer
    int waited = d_mariadb_send_query_start(db,
                                            "SELECT SLEEP(0.5) AS s",
                                            &pending);

    D_TESTS_MARIADB_CHECK( (d_mariadb_client_is_mariadb()) ||
                           (waited == D_DB_WAIT) );
    D_TESTS_MARIADB_OK(db, d_tests_mariadb_drive_send(waited, db, &pending));
    waited = d_mariadb_next_result_start(db, &pending, &rows, NULL);
    D_TESTS_MARIADB_CHECK( (!d_mariadb_client_is_mariadb()) ||
                           (waited == D_DB_WAIT) );
    D_TESTS_MARIADB_OK(db, d_tests_mariadb_drive_next(waited,
                                                      db,
                                                      &pending,
                                                      &rows,
                                                      NULL));
    D_TESTS_MARIADB_CHECK(rows.row_count == 1u);
    d_db_rows_free(&rows);
    D_TESTS_MARIADB_RESULT(db, d_tests_mariadb_next(db, &rows, NULL),
                           D_DB_NOT_FOUND);

    // the first statement's failure, then nothing
    D_TESTS_MARIADB_OK(db, d_tests_mariadb_send(db, "SELECT * FROM no_table"));
    D_TESTS_MARIADB_RESULT(db, d_tests_mariadb_next(db, &rows, NULL), 1146);
    D_TESTS_MARIADB_RESULT(db, d_tests_mariadb_next(db, &rows, NULL),
                           D_DB_NOT_FOUND);

    // a failure among a procedure's results
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(
                               db,
                               "CREATE PROCEDURE q() BEGIN SELECT 1 AS a; "
                               "SELECT * FROM no_table; END"));
    D_TESTS_MARIADB_OK(db, d_tests_mariadb_send(db, "CALL q()"));
    D_TESTS_MARIADB_OK(db, d_tests_mariadb_next(db, &rows, NULL));
    d_db_rows_free(&rows);
    D_TESTS_MARIADB_RESULT(db, d_tests_mariadb_next(db, &rows, NULL), 1146);
    D_TESTS_MARIADB_RESULT(db, d_tests_mariadb_next(db, &rows, NULL),
                           D_DB_NOT_FOUND);
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "SELECT 1"));

    // more than the socket takes at once
    memcpy(sql, "SELECT LENGTH('", 15u);
    memset(sql + 15, 'y', large);
    memcpy(sql + 15 + large, "')", 3u);
    D_TESTS_MARIADB_OK(db, d_tests_mariadb_send(db, sql));
    D_TESTS_MARIADB_OK(db, d_tests_mariadb_next(db, &rows, NULL));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_first_int(&rows) == (int64_t)large);
    d_db_rows_free(&rows);
    D_TESTS_MARIADB_RESULT(db, d_tests_mariadb_next(db, &rows, NULL),
                           D_DB_NOT_FOUND);

    // arguments
    D_TESTS_MARIADB_RESULT(NULL, d_mariadb_send_query_start(NULL,
                                                            "SELECT 1",
                                                            &pending),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(NULL, d_mariadb_send_query_start(db,
                                                            NULL,
                                                            &pending),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(NULL, d_mariadb_send_query_start(db,
                                                            "SELECT 1",
                                                            NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(NULL, d_mariadb_send_query_continue(db,
                                                               0u,
                                                               &pending),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(NULL, d_mariadb_next_result_start(db,
                                                             &pending,
                                                             NULL,
                                                             NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(NULL, d_mariadb_next_result_continue(db,
                                                                0u,
                                                                &pending,
                                                                &rows,
                                                                NULL),
                           D_DB_INVALID_ARGUMENT);
    free(sql);
    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_drive_prepare
  Continues a prepare until it is done.
*/
static int
d_tests_mariadb_drive_prepare(
    int                         _started,
    struct d_mariadb_statement* _statement,
    struct d_db_pending*        _pending
)
{
    int result = _started;

    for (int waits = 0;
         (result == D_DB_WAIT) && (waits < D_TESTS_MARIADB_ROUNDS);
         ++waits)
    {
        result = d_mariadb_prepare_continue(_statement,
                                            d_tests_mariadb_wait(_pending),
                                            _pending);
    }

    return result;
}

/*
d_tests_mariadb_prepare_nb
  A statement prepared without blocking, from start to end; a statement a
failed prepare left is finalized, and NULL.
*/
static int
d_tests_mariadb_prepare_nb(
    struct d_mariadb*            _db,
    struct d_mariadb_statement** _statement,
    const char*                  _sql
)
{
    struct d_db_pending pending;
    const int           started = d_mariadb_prepare_start(_db,
                                                          _statement,
                                                          _sql,
                                                          &pending);
    const int           result  = d_tests_mariadb_drive_prepare(started,
                                                                *_statement,
                                                                &pending);

    // what a failed prepare left
    if (result != 0)
    {
        d_mariadb_finalize(*_statement);
        *_statement = NULL;
    }

    return result;
}

/*
d_tests_mariadb_update_nb
  A statement run without blocking, its count read, from start to end.
*/
static int
d_tests_mariadb_update_nb(
    struct d_mariadb_statement* _statement,
    int64_t*                    _changes
)
{
    struct d_db_pending pending;
    int                 result = d_mariadb_execute_start(_statement,
                                                         &pending);

    for (int waits = 0;
         (result == D_DB_WAIT) && (waits < D_TESTS_MARIADB_ROUNDS);
         ++waits)
    {
        result = d_mariadb_execute_update_continue(
                     _statement,
                     d_tests_mariadb_wait(&pending),
                     &pending,
                     _changes);
    }

    return result;
}

/*
d_tests_mariadb_query_nb
  A statement run without blocking, its rows read, from start to end.
*/
static int
d_tests_mariadb_query_nb(
    struct d_mariadb_statement* _statement,
    struct d_db_rows*           _rows
)
{
    struct d_db_pending pending;
    int                 result = d_mariadb_execute_start(_statement,
                                                         &pending);

    for (int waits = 0;
         (result == D_DB_WAIT) && (waits < D_TESTS_MARIADB_ROUNDS);
         ++waits)
    {
        result = d_mariadb_execute_query_continue(
                     _statement,
                     d_tests_mariadb_wait(&pending),
                     &pending,
                     _rows);
    }

    return result;
}

/*
d_tests_mariadb_open_nb
  A run opened without blocking, from start to end.
*/
static int
d_tests_mariadb_open_nb(
    struct d_mariadb_statement* _statement
)
{
    struct d_db_pending pending;
    int                 result = d_mariadb_open_cursor_start(_statement,
                                                             &pending);

    for (int waits = 0;
         (result == D_DB_WAIT) && (waits < D_TESTS_MARIADB_ROUNDS);
         ++waits)
    {
        result = d_mariadb_open_cursor_continue(
                     _statement,
                     d_tests_mariadb_wait(&pending),
                     &pending);
    }

    return result;
}

/*
d_tests_mariadb_page_nb
  A page fetched without blocking, from start to end.
*/
static int
d_tests_mariadb_page_nb(
    struct d_mariadb_statement* _statement,
    size_t                      _max,
    struct d_db_rows*           _rows
)
{
    struct d_db_pending pending;
    int                 result = d_mariadb_fetch_rows_start(_statement,
                                                            _max,
                                                            &pending,
                                                            _rows);

    for (int waits = 0;
         (result == D_DB_WAIT) && (waits < D_TESTS_MARIADB_ROUNDS);
         ++waits)
    {
        result = d_mariadb_fetch_rows_continue(
                     _statement,
                     d_tests_mariadb_wait(&pending),
                     &pending,
                     _rows);
    }

    return result;
}

/*
d_tests_mariadb_statements_nonblocking
  d_mariadb_prepare_start and _continue, d_mariadb_execute_start and its
continuations, d_mariadb_open_cursor_start, _fetch_rows_start and theirs.
  Tests the following:
  - with libmysqlclient, each start is D_DB_UNSUPPORTED, no statement made
  - a prepare driven through a poll loop compiles the statement, its
    parameters counted -- after the multi-statement mode a failed script
    left on, too; one the server refuses is its error, the statement, where
    the start made one, unprepared -- each run D_DB_INVALID_ARGUMENT -- and
    finalized
  - a run sent is D_DB_WAIT; read for its count, it gives the rows it
    changed; for its rows, those the blocking run gives; a failure is the
    server's number; a procedure's further results are read away
  - while a run is under way the connection is held: a statement, a bind on
    the statement, its blocking run, another start, all 2014; the other
    continuation, once one has chosen, D_DB_INVALID_ARGUMENT
  - a run opened without blocking gives its rows a page at a time, as the
    blocking run does, the last page short; the connection is then free;
    while it is open, a start on it is 2014
  - a statement finalized while its run is under way abandons it: the
    connection then 2014 until closed
  - NULL arguments, and continuations with nothing under way, are
    D_DB_INVALID_ARGUMENT
*/
static void
d_tests_mariadb_statements_nonblocking(
    void
)
{
    char                        schema[80] = "";
    struct d_mariadb*           db         = d_tests_mariadb_begin(
                                                 "stmtnb",
                                                 schema,
                                                 sizeof(schema));
    struct d_mariadb_statement* statement  = NULL;
    struct d_mariadb_statement* other      = NULL;
    struct d_db_pending         pending;
    struct d_db_rows            rows;
    struct d_db_rows            all;
    int64_t                     changes    = -1;

    printf("mariadb_statement: without blocking\n");

    memset(&rows, 0, sizeof(rows));
    memset(&all, 0, sizeof(all));

    // nothing more without a server
    if (!db)
    {
        return;
    }

    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db,
                                          "CREATE TABLE t (a INT PRIMARY KEY, "
                                          "b VARCHAR(20))"));

    // a client without them
    if (!d_mariadb_client_is_mariadb())
    {
        D_TESTS_MARIADB_RESULT(db, d_mariadb_prepare_start(db,
                                                           &statement,
                                                           "SELECT 1",
                                                           &pending),
                               D_DB_UNSUPPORTED);
        D_TESTS_MARIADB_CHECK( (statement == NULL) &&
                               (d_tests_mariadb_unpended(&pending)) );
        D_TESTS_MARIADB_OK(db, d_mariadb_prepare(db, &statement, "SELECT 1"));
        D_TESTS_MARIADB_RESULT(db, d_mariadb_execute_start(statement,
                                                           &pending),
                               D_DB_UNSUPPORTED);
        D_TESTS_MARIADB_RESULT(db, d_mariadb_open_cursor_start(statement,
                                                               &pending),
                               D_DB_UNSUPPORTED);
        D_TESTS_MARIADB_RESULT(db, d_mariadb_execute_update_continue(
                                       statement,
                                       0u,
                                       &pending,
                                       NULL),
                               D_DB_UNSUPPORTED);
        D_TESTS_MARIADB_OK(db, d_mariadb_execute_update(statement, NULL));
        d_mariadb_finalize(statement);
        d_tests_mariadb_end(db, schema);

        return;
    }

    // prepared, after the mode a failed script left on
    D_TESTS_MARIADB_RESULT(db, d_mariadb_exec_script(db,
                                                     "SELECT 1; "
                                                     "SELECT * FROM none"),
                           1146);

    int started = d_mariadb_prepare_start(db,
                                          &statement,
                                          "INSERT INTO t VALUES (?, ?)",
                                          &pending);

    D_TESTS_MARIADB_CHECK( ( (started == D_DB_WAIT) ||
                             (started == 0) ) &&
                           (statement != NULL) );
    D_TESTS_MARIADB_OK(db, d_tests_mariadb_drive_prepare(started,
                                                         statement,
                                                         &pending));
    D_TESTS_MARIADB_CHECK( (d_mariadb_parameter_count(statement) == 2u) &&
                           (d_tests_mariadb_unpended(&pending)) );
    D_TESTS_MARIADB_RESULT(db, d_mariadb_exec(db, "SELECT 1; SELECT 2"),
                           1064);

    // runs read for their counts
    for (int64_t i = 1; i <= 5; ++i)
    {
        char text[16] = "";

        (void)snprintf(text, sizeof(text), "row %d", (int)i);
        D_TESTS_MARIADB_OK(db, d_mariadb_bind_int64(statement, 1u, i));
        D_TESTS_MARIADB_OK(db, d_mariadb_bind_text(statement, 2u, text));
        D_TESTS_MARIADB_OK(db, d_tests_mariadb_update_nb(statement,
                                                         &changes));
        D_TESTS_MARIADB_CHECK(changes == 1);
    }

    // a failure is the server's
    D_TESTS_MARIADB_OK(db, d_mariadb_bind_int64(statement, 1u, 3));
    D_TESTS_MARIADB_RESULT(db, d_tests_mariadb_update_nb(statement, &changes),
                           1062);
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "SELECT 1"));
    d_mariadb_finalize(statement);
    statement = NULL;

    // runs read for their rows, as the blocking run reads them
    D_TESTS_MARIADB_OK(db, d_mariadb_prepare(db,
                                             &other,
                                             "SELECT a, b FROM t WHERE a > ? "
                                             "ORDER BY a"));
    D_TESTS_MARIADB_OK(db, d_mariadb_bind_int64(other, 1u, 1));
    D_TESTS_MARIADB_OK(db, d_mariadb_execute_query(other, &all));
    D_TESTS_MARIADB_OK(db, d_tests_mariadb_prepare_nb(
                               db,
                               &statement,
                               "SELECT a, b FROM t WHERE a > ? ORDER BY a"));
    D_TESTS_MARIADB_CHECK(statement != NULL);
    D_TESTS_MARIADB_OK(db, d_mariadb_bind_int64(statement, 1u, 1));
    D_TESTS_MARIADB_OK(db, d_tests_mariadb_query_nb(statement, &rows));
    D_TESTS_MARIADB_CHECK( (all.row_count == 4u) &&
                           (d_tests_mariadb_same_page(&rows, &all, 0u, 4u)) );
    d_db_rows_free(&rows);

    // a page at a time, as the blocking run reads them
    D_TESTS_MARIADB_OK(db, d_tests_mariadb_open_nb(statement));
    D_TESTS_MARIADB_RESULT(db, d_mariadb_exec(db, "SELECT 1"), 2014);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_execute_start(other, &pending),
                           2014);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_open_cursor_start(statement,
                                                           &pending),
                           2014);

    for (size_t first = 0u; first < 4u; first += 3u)
    {
        const size_t expected = (first == 0u) ? 3u : 1u;

        D_TESTS_MARIADB_OK(db, d_tests_mariadb_page_nb(statement, 3u, &rows));
        D_TESTS_MARIADB_CHECK(
            (rows.row_count == expected) &&
            (d_tests_mariadb_same_page(&rows, &all, first, expected)) );
        d_db_rows_free(&rows);
    }

    D_TESTS_MARIADB_RESULT(db, d_mariadb_fetch_rows_start(statement,
                                                          3u,
                                                          &pending,
                                                          &rows),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "SELECT 1"));
    d_db_rows_free(&all);
    d_mariadb_finalize(other);
    other = NULL;

    // the connection held while a run is under way
    D_TESTS_MARIADB_OK(db, d_mariadb_prepare(db,
                                             &other,
                                             "SELECT SLEEP(0.3) AS s, ? AS v"));
    D_TESTS_MARIADB_OK(db, d_mariadb_bind_int64(other, 1u, 42));
    D_TESTS_MARIADB_RESULT(db, d_mariadb_execute_start(other, &pending),
                           D_DB_WAIT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_exec(db, "SELECT 1"), 2014);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_bind_int64(other, 1u, 7), 2014);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_clear_bindings(other), 2014);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_execute_update(other, NULL), 2014);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_execute_start(statement, &pending),
                           2014);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_execute_query_continue(other,
                                                                0u,
                                                                &pending,
                                                                &rows),
                           D_DB_WAIT);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_execute_update_continue(other,
                                                                 0u,
                                                                 &pending,
                                                                 NULL),
                           D_DB_INVALID_ARGUMENT);

    started = D_DB_WAIT;

    for (int waits = 0;
         (started == D_DB_WAIT) && (waits < D_TESTS_MARIADB_ROUNDS);
         ++waits)
    {
        started = d_mariadb_execute_query_continue(
                      other,
                      d_tests_mariadb_wait(&pending),
                      &pending,
                      &rows);
    }

    D_TESTS_MARIADB_OK(db, started);
    D_TESTS_MARIADB_CHECK( (rows.row_count == 1u) &&
                           (rows.column_count == 2u) );

    {
        int64_t value = 0;

        D_TESTS_MARIADB_CHECK(
            (d_db_value_int64(d_db_rows_value(&rows, 0u, 1u), &value) == 0) &&
            (value == 42) );
    }

    d_db_rows_free(&rows);
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "SELECT 1"));
    d_mariadb_finalize(other);
    other = NULL;

    // a procedure's further results, read away
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db,
                                          "CREATE PROCEDURE p() BEGIN "
                                          "SELECT 1 AS x; SELECT 2 AS y; "
                                          "END"));
    D_TESTS_MARIADB_OK(db, d_tests_mariadb_prepare_nb(db, &other, "CALL p()"));
    D_TESTS_MARIADB_OK(db, d_tests_mariadb_query_nb(other, &rows));
    D_TESTS_MARIADB_CHECK( (rows.row_count == 1u) &&
                           (strcmp(rows.columns[0].name, "x") == 0) );
    d_db_rows_free(&rows);
    D_TESTS_MARIADB_OK(db, d_tests_mariadb_update_nb(other, &changes));
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "SELECT 1"));
    d_mariadb_finalize(other);
    other = NULL;

    // a statement the server refuses
    started = d_mariadb_prepare_start(db, &other, "SELEC 1", &pending);

    if (started == D_DB_WAIT)
    {
        D_TESTS_MARIADB_RESULT(db, d_tests_mariadb_drive_prepare(started,
                                                                 other,
                                                                 &pending),
                               1064);
        D_TESTS_MARIADB_CHECK(strstr(d_mariadb_error_message(db), "SELEC"));
        D_TESTS_MARIADB_RESULT(db, d_mariadb_execute_update(other, NULL),
                               D_DB_INVALID_ARGUMENT);
        D_TESTS_MARIADB_RESULT(db, d_mariadb_execute_start(other, &pending),
                               D_DB_INVALID_ARGUMENT);
        D_TESTS_MARIADB_CHECK(d_mariadb_parameter_count(other) == 0u);
        D_TESTS_MARIADB_OK(db, d_mariadb_finalize(other));
    }
    else
    {
        D_TESTS_MARIADB_CHECK( (started == 1064) &&
                               (other == NULL) );
    }

    other = NULL;
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "SELECT 1"));

    // arguments, and continuations with nothing under way
    D_TESTS_MARIADB_RESULT(NULL, d_mariadb_prepare_start(NULL,
                                                         &other,
                                                         "SELECT 1",
                                                         &pending),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(NULL, d_mariadb_prepare_start(db,
                                                         NULL,
                                                         "SELECT 1",
                                                         &pending),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(NULL, d_mariadb_prepare_start(db,
                                                         &other,
                                                         NULL,
                                                         &pending),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(NULL, d_mariadb_prepare_start(db,
                                                         &other,
                                                         "SELECT 1",
                                                         NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(NULL, d_mariadb_prepare_continue(statement,
                                                            0u,
                                                            &pending),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(NULL, d_mariadb_execute_start(NULL, &pending),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(NULL, d_mariadb_execute_start(statement, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(NULL, d_mariadb_execute_update_continue(statement,
                                                                   0u,
                                                                   &pending,
                                                                   NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(NULL, d_mariadb_execute_query_continue(statement,
                                                                  0u,
                                                                  &pending,
                                                                  &rows),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(NULL, d_mariadb_open_cursor_continue(statement,
                                                                0u,
                                                                &pending),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(NULL, d_mariadb_fetch_rows_start(statement,
                                                            0u,
                                                            &pending,
                                                            &rows),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(NULL, d_mariadb_fetch_rows_continue(statement,
                                                               0u,
                                                               &pending,
                                                               &rows),
                           D_DB_INVALID_ARGUMENT);

    // finalized under way: the run abandoned, the connection good only for
    // closing
    D_TESTS_MARIADB_OK(db, d_mariadb_prepare(db,
                                             &other,
                                             "SELECT SLEEP(0.3)"));
    D_TESTS_MARIADB_RESULT(db, d_mariadb_execute_start(other, &pending),
                           D_DB_WAIT);
    D_TESTS_MARIADB_OK(db, d_mariadb_finalize(other));
    D_TESTS_MARIADB_RESULT(db, d_mariadb_exec(db, "SELECT 1"), 2014);
    D_TESTS_MARIADB_RESULT(db, d_mariadb_execute_start(statement, &pending),
                           2014);
    d_mariadb_finalize(statement);
    d_tests_mariadb_end(db, schema);

    return;
}

/*
d_tests_mariadb_drive_on
  Continues a ping, a reset or a change of user until it is done.
*/
static int
d_tests_mariadb_drive_on(
    int                  _started,
    struct d_mariadb*    _db,
    struct d_db_pending* _pending,
    int                (*_continue)(struct d_mariadb*,
                                    unsigned,
                                    struct d_db_pending*)
)
{
    int result = _started;

    for (int waits = 0;
         (result == D_DB_WAIT) && (waits < D_TESTS_MARIADB_ROUNDS);
         ++waits)
    {
        result = _continue(_db, d_tests_mariadb_wait(_pending), _pending);
    }

    return result;
}

/*
d_tests_mariadb_session_nonblocking
  d_mariadb_ping_start, _reset_connection_start and _change_user_start, and
their continuations.
  Tests the following:
  - with MariaDB's client, each finishes through a poll loop as its blocking
    form does: a ping answers; a reset ends the session's state -- a user
    variable gone -- and runs the connect's init commands again; a change
    of user to the same user signs in again, the database named current
  - while one is under way the connection is held: a statement is 2014,
    where a reset's answer has yet to come when it starts
  - with libmysqlclient, each is D_DB_UNSUPPORTED
  - NULL arguments, and continuations with nothing under way, are
    D_DB_INVALID_ARGUMENT
*/
static void
d_tests_mariadb_session_nonblocking(
    void
)
{
    char                           schema[80] = "";
    struct d_mariadb*              holder     = d_tests_mariadb_begin(
                                                    "sessnb",
                                                    schema,
                                                    sizeof(schema));
    struct d_mariadb*              db         = NULL;
    struct d_db_pending            pending;
    const struct d_mariadb_setting init[1]    =
    {
        { D_MARIADB_SETTING_INIT_COMMAND, 0ul, "SET @d_tests_init = 5", NULL }
    };
    struct d_mariadb_setting       two[2];
    size_t                         count      = 0u;
    int64_t                        number     = 0;
    char*                          current    = NULL;

    printf("mariadb_connection: the session without blocking\n");

    const struct d_mariadb_setting* given = d_tests_mariadb_plain_first(
                                                two,
                                                init,
                                                &count);

    D_TESTS_MARIADB_OK(NULL, d_mariadb_connect_with(&g_options,
                                                    given,
                                                    count,
                                                    &db,
                                                    NULL));

    // nothing more without a server
    if ( (!db) ||
         (!holder) )
    {
        d_mariadb_close(db);
        d_tests_mariadb_end(holder, schema);

        return;
    }

    // a client that makes none of them
    if (!d_tests_mariadb_async(true))
    {
        D_TESTS_MARIADB_RESULT(db, d_mariadb_ping_start(db, &pending),
                               D_DB_UNSUPPORTED);
        D_TESTS_MARIADB_RESULT(db,
                               d_mariadb_reset_connection_start(db, &pending),
                               D_DB_UNSUPPORTED);
        D_TESTS_MARIADB_RESULT(db,
                               d_mariadb_change_user_start(db,
                                                           g_options.user,
                                                           g_options.password,
                                                           NULL,
                                                           &pending),
                               D_DB_UNSUPPORTED);
        D_TESTS_MARIADB_CHECK(d_tests_mariadb_unpended(&pending));
        D_TESTS_MARIADB_OK(db, d_mariadb_ping(db));
        d_mariadb_close(db);
        d_tests_mariadb_end(holder, schema);

        return;
    }

    D_TESTS_MARIADB_OK(db, d_tests_mariadb_drive_on(
                               d_mariadb_ping_start(db, &pending),
                               db,
                               &pending,
                               d_mariadb_ping_continue));

    // a reset, the init command run again
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "SET @d_tests_init = 9, "
                                              "@d_tests_gone = 1"));
    const int reset = d_mariadb_reset_connection_start(db, &pending);

    // held while under way, where the answer has yet to come
    if (reset == D_DB_WAIT)
    {
        D_TESTS_MARIADB_RESULT(db, d_mariadb_exec(db, "SELECT 1"), 2014);
    }

    D_TESTS_MARIADB_OK(db, d_tests_mariadb_drive_on(
                               reset,
                               db,
                               &pending,
                               d_mariadb_reset_connection_continue));
    D_TESTS_MARIADB_CHECK( (d_mariadb_query_int64(db,
                                                  &number,
                                                  "SELECT @d_tests_init") ==
                            0) &&
                           (number == 5) );
    D_TESTS_MARIADB_CHECK( (d_mariadb_query_int64(db,
                                                  &number,
                                                  "SELECT @d_tests_gone") ==
                            0) &&
                           (number == 0) );

    // a change of user, to the same user, the test's database named
    D_TESTS_MARIADB_OK(db, d_tests_mariadb_drive_on(
                               d_mariadb_change_user_start(
                                   db,
                                   g_options.user,
                                   g_options.password,
                                   schema,
                                   &pending),
                               db,
                               &pending,
                               d_mariadb_change_user_continue));
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_unpended(&pending));
    D_TESTS_MARIADB_OK(db, d_mariadb_query_text(db,
                                                &current,
                                                "SELECT DATABASE()"));
    D_TESTS_MARIADB_CHECK( (current) &&
                           (strcmp(current, schema) == 0) );
    d_db_free(current);

    // arguments, and continuations with nothing under way
    D_TESTS_MARIADB_RESULT(NULL, d_mariadb_ping_start(NULL, &pending),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(NULL, d_mariadb_ping_start(db, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(NULL, d_mariadb_ping_continue(db, 0u, &pending),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(NULL,
                           d_mariadb_reset_connection_continue(db,
                                                               0u,
                                                               &pending),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(NULL,
                           d_mariadb_change_user_continue(db, 0u, &pending),
                           D_DB_INVALID_ARGUMENT);
    d_mariadb_close(db);
    d_tests_mariadb_end(holder, schema);

    return;
}

/*
d_tests_mariadb_sleeping
  Starts sending a long sleep without blocking, and waits, from another
connection, until the server runs it; the send is left as it started, for
d_tests_mariadb_stopped to finish: MariaDB's client has sent it, and
libmysqlclient, whose send waits for the first statement's answer, has it
on its way.
*/
static bool
d_tests_mariadb_sleeping(
    struct d_mariadb*    _db,
    struct d_mariadb*    _watcher,
    struct d_db_pending* _pending,
    int*                 _sent
)
{
    char sql[200] = "";

    *_sent = d_mariadb_send_query_start(_db,
                                        "SELECT SLEEP(30) AS slept",
                                        _pending);

    // the statement on its way
    if ( (*_sent != 0) &&
         (*_sent != D_DB_WAIT) )
    {
        return false;
    }

    (void)snprintf(sql,
                   sizeof(sql),
                   "SELECT COUNT(*) FROM information_schema.PROCESSLIST "
                   "WHERE ID = %lu AND INFO LIKE '%%SLEEP(30)%%'",
                   d_mariadb_connection_id(_db));

    for (int i = 0; i < 250; ++i)
    {
        int64_t count = 0;

        if ( (d_mariadb_query_int64(_watcher, &count, sql) == 0) &&
             (count == 1) )
        {
            return true;
        }

        (void)d_mariadb_exec(_watcher, "DO SLEEP(0.02)");
    }

    return false;
}

/*
d_tests_mariadb_stopped
  Finishes a cancelled sleep's send and reads its result: the server's 1317,
or SLEEP's own 1 for a sleep it interrupted, soon either way; then nothing,
the connection free.
*/
static void
d_tests_mariadb_stopped(
    struct d_mariadb*    _db,
    time_t               _began,
    int                  _sent,
    struct d_db_pending* _pending
)
{
    struct d_db_rows rows;

    D_TESTS_MARIADB_OK(_db, d_tests_mariadb_drive_send(_sent, _db, _pending));

    const int result = d_tests_mariadb_next(_db, &rows, NULL);

    D_TESTS_MARIADB_CHECK( (result == 1317) ||
                           ( (result == 0) &&
                             (d_tests_mariadb_first_int(&rows) == 1) ) );
    D_TESTS_MARIADB_CHECK(time(NULL) - _began < 10);

    if (result == 0)
    {
        d_db_rows_free(&rows);
        D_TESTS_MARIADB_RESULT(_db, d_tests_mariadb_next(_db, &rows, NULL),
                               D_DB_NOT_FOUND);
    }

    D_TESTS_MARIADB_OK(_db, d_mariadb_exec(_db, "SELECT 1"));

    return;
}

// D_TESTS_MARIADB_TLS_LIBRARY
//   macro: 1 where the client names its TLS library at run time
// (mariadb_get_infov's MARIADB_TLS_LIBRARY): Connector/C from 3.0.
#if defined(MARIADB_PACKAGE_VERSION_ID)
    #if (MARIADB_PACKAGE_VERSION_ID >= 30000)
        #define D_TESTS_MARIADB_TLS_LIBRARY 1
    #endif
#endif
#if !defined(D_TESTS_MARIADB_TLS_LIBRARY)
    #define D_TESTS_MARIADB_TLS_LIBRARY 0
#endif

#if (D_TESTS_MARIADB_TLS_LIBRARY == 1)

/*
d_tests_mariadb_tls_expected
  The harness's own reading of whether the client carries TLS without
blocking: Connector/C before 3.4.10 whose TLS library is Schannel does not,
nor one before 3.0.4 with any TLS library.
*/
static bool
d_tests_mariadb_tls_expected(
    void
)
{
    const unsigned long version = mysql_get_client_version();
    const char*         library = "";

    (void)mariadb_get_infov(NULL, MARIADB_TLS_LIBRARY, &library);

    return ( (version >= 30410ul)                         ||
             ( (library) &&
               (strcmp(library, "Off") == 0) )            ||
             ( (version >= 30004ul) &&
               ( (!library) ||
                 (strcmp(library, "Schannel") != 0) ) ) );
}

#else

/*
d_tests_mariadb_tls_expected
  A client without Schannel carries TLS where it makes calls that do not
block at all.
*/
static bool
d_tests_mariadb_tls_expected(
    void
)
{
    return d_tests_mariadb_async(false);
}

#endif  // D_TESTS_MARIADB_TLS_LIBRARY

/*
d_tests_mariadb_tls_under
  Whether a connection runs under TLS, as the cipher it agreed says.
*/
static bool
d_tests_mariadb_tls_under(
    struct d_mariadb* _db
)
{
    return (mysql_get_ssl_cipher((MYSQL*)d_mariadb_native(_db)) != NULL);
}

/*
d_tests_mariadb_tls_refusals
  Every start on a connection under TLS, where the client cannot carry it
without blocking: each D_DB_UNSUPPORTED, nothing pending, nothing held -- the
connection still answers a blocking call after each.
*/
static void
d_tests_mariadb_tls_refusals(
    struct d_mariadb* _db
)
{
    struct d_mariadb_statement* statement = NULL;
    struct d_mariadb_statement* started   = NULL;
    struct d_mariadb*           canceller = NULL;
    char*                       reason    = NULL;
    struct d_db_pending         pending;
    struct d_db_rows            rows;

    memset(&rows, 0, sizeof(rows));
    D_TESTS_MARIADB_RESULT(_db, d_mariadb_ping_start(_db, &pending),
                           D_DB_UNSUPPORTED);
    D_TESTS_MARIADB_RESULT(_db,
                           d_mariadb_reset_connection_start(_db, &pending),
                           D_DB_UNSUPPORTED);
    D_TESTS_MARIADB_RESULT(_db,
                           d_mariadb_change_user_start(_db,
                                                       g_options.user,
                                                       g_options.password,
                                                       NULL,
                                                       &pending),
                           D_DB_UNSUPPORTED);
    D_TESTS_MARIADB_RESULT(_db,
                           d_mariadb_send_query_start(_db,
                                                      "SELECT 1",
                                                      &pending),
                           D_DB_UNSUPPORTED);
    D_TESTS_MARIADB_RESULT(_db,
                           d_mariadb_next_result_start(_db,
                                                       &pending,
                                                       &rows,
                                                       NULL),
                           D_DB_UNSUPPORTED);
    D_TESTS_MARIADB_RESULT(_db,
                           d_mariadb_prepare_start(_db,
                                                   &started,
                                                   "SELECT 1",
                                                   &pending),
                           D_DB_UNSUPPORTED);
    D_TESTS_MARIADB_CHECK( (started == NULL) &&
                           (d_tests_mariadb_unpended(&pending)) );
    D_TESTS_MARIADB_OK(_db, d_mariadb_ping(_db));

    // a statement prepared by blocking: its runs and its cursor's pages
    D_TESTS_MARIADB_OK(_db, d_mariadb_prepare(_db,
                                              &statement,
                                              "SELECT 1 UNION SELECT 2"));
    D_TESTS_MARIADB_RESULT(_db, d_mariadb_execute_start(statement, &pending),
                           D_DB_UNSUPPORTED);
    D_TESTS_MARIADB_RESULT(_db, d_mariadb_open_cursor_start(statement,
                                                            &pending),
                           D_DB_UNSUPPORTED);
    D_TESTS_MARIADB_OK(_db, d_mariadb_open_cursor(statement));
    D_TESTS_MARIADB_RESULT(_db, d_mariadb_fetch_rows_start(statement,
                                                           1u,
                                                           &pending,
                                                           &rows),
                           D_DB_UNSUPPORTED);
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_unpended(&pending));
    D_TESTS_MARIADB_OK(_db, d_mariadb_close_cursor(statement));
    D_TESTS_MARIADB_OK(_db, d_mariadb_finalize(statement));

    // a cancel, which would need TLS for its own connection too
    D_TESTS_MARIADB_RESULT(_db, d_mariadb_cancel_start(_db,
                                                       &canceller,
                                                       &pending,
                                                       &reason),
                           D_DB_UNSUPPORTED);
    D_TESTS_MARIADB_CHECK( (canceller == NULL) &&
                           (reason) &&
                           (strstr(reason, "Schannel") != NULL) );
    d_db_free(reason);
    D_TESTS_MARIADB_OK(_db, d_mariadb_exec(_db, "SELECT 1"));

    return;
}

/*
d_tests_mariadb_tls_nonblocking
  d_mariadb_client_nonblocking_tls, and the starts' refusal of TLS where it
is false.
  Tests the following:
  - the answer is the harness's own reading of the client: Connector/C
    before 3.4.10 whose TLS library is Schannel cannot, nor one before 3.0.4;
    any other client with calls that do not block can
  - where it can, a connection under TLS where the server offers it sends
    and reads without blocking
  - where it cannot: a connect's start that would ask for TLS -- the
    layer's default -- is D_DB_UNSUPPORTED, no handle, nothing pending, the
    reason given; one with D_MARIADB_SSL_DISABLED connects, without TLS,
    sends and reads, and cancels, its own connection without TLS; on a
    connection under TLS every start is D_DB_UNSUPPORTED -- a ping, a reset,
    a change of user, SQL sent, its results, a prepare, a run, a cursor, its
    pages, a cancel with its reason -- and the connection carries on by
    blocking
*/
static void
d_tests_mariadb_tls_nonblocking(
    void
)
{
    const bool          can       = d_mariadb_client_nonblocking_tls();
    struct d_mariadb*   db        = NULL;
    struct d_mariadb*   plain     = NULL;
    struct d_mariadb*   canceller = NULL;
    char*               reason    = NULL;
    struct d_db_pending pending;
    struct d_db_rows    rows;

    printf("mariadb_common: calls that do not block, over TLS (%s)\n",
           (can) ? "carried" : "refused");

    memset(&rows, 0, sizeof(rows));
    D_TESTS_MARIADB_CHECK(can == d_tests_mariadb_tls_expected());

    // a client with no calls that do not block has nothing to refuse
    if (!d_tests_mariadb_async(false))
    {
        D_TESTS_MARIADB_CHECK(!can);

        return;
    }

    db = d_tests_mariadb_connect(NULL);

    // nothing more without a server
    if (!db)
    {
        D_TESTS_MARIADB_CHECK(db != NULL);

        return;
    }

    printf("  %s\n", (d_tests_mariadb_tls_under(db)) ? "under TLS"
                                                     : "the server has no TLS");

    // carried: SQL sent and read over the connection as it is
    if (can)
    {
        D_TESTS_MARIADB_OK(db, d_tests_mariadb_send(db, "SELECT 1"));
        D_TESTS_MARIADB_OK(db, d_tests_mariadb_next(db, &rows, NULL));
        d_db_rows_free(&rows);
        D_TESTS_MARIADB_RESULT(db, d_tests_mariadb_next(db, &rows, NULL),
                               D_DB_NOT_FOUND);
        d_mariadb_close(db);

        return;
    }

    // refused: a connect's start that would ask for TLS
    D_TESTS_MARIADB_RESULT(NULL,
                           d_mariadb_connect_with_start(&g_options,
                                                        NULL,
                                                        0u,
                                                        &plain,
                                                        &pending,
                                                        &reason),
                           D_DB_UNSUPPORTED);
    D_TESTS_MARIADB_CHECK( (plain == NULL) &&
                           (d_tests_mariadb_unpended(&pending)) &&
                           (reason) &&
                           (strstr(reason, "D_MARIADB_SSL_DISABLED") != NULL) );
    d_db_free(reason);
    reason = NULL;
    D_TESTS_MARIADB_RESULT(NULL,
                           d_mariadb_connect_start(&g_options,
                                                   &plain,
                                                   &pending),
                           D_DB_UNSUPPORTED);
    D_TESTS_MARIADB_CHECK(plain == NULL);

    // without TLS: made, sent and read, and cancelled, without blocking
    D_TESTS_MARIADB_OK(NULL, d_tests_mariadb_drive_connect(
                                 d_mariadb_connect_with_start(&g_options,
                                                              &g_plain,
                                                              1u,
                                                              &plain,
                                                              &pending,
                                                              NULL),
                                 &plain,
                                 &pending,
                                 NULL));

    if (plain)
    {
        int result = 0;

        D_TESTS_MARIADB_CHECK(!d_tests_mariadb_tls_under(plain));
        D_TESTS_MARIADB_OK(plain, d_tests_mariadb_send(plain, "SELECT 1"));
        D_TESTS_MARIADB_OK(plain, d_tests_mariadb_next(plain, &rows, NULL));
        d_db_rows_free(&rows);
        D_TESTS_MARIADB_RESULT(plain, d_tests_mariadb_next(plain, &rows, NULL),
                               D_DB_NOT_FOUND);
        result = d_mariadb_cancel_start(plain, &canceller, &pending, &reason);

        for (int waits = 0;
             (result == D_DB_WAIT) && (waits < D_TESTS_MARIADB_ROUNDS);
             ++waits)
        {
            result = d_mariadb_cancel_continue(&canceller,
                                               d_tests_mariadb_wait(&pending),
                                               &pending,
                                               &reason);
        }

        D_TESTS_MARIADB_OK(plain, result);
        D_TESTS_MARIADB_CHECK( (canceller == NULL) &&
                               (reason == NULL) );
        d_db_free(reason);
        d_mariadb_close(plain);
    }

    // a connection under TLS: every start refused
    if (d_tests_mariadb_tls_under(db))
    {
        d_tests_mariadb_tls_refusals(db);
    }

    d_mariadb_close(db);

    return;
}

/*
d_tests_mariadb_cancels
  d_mariadb_cancel, _cancel_start and _cancel_continue.
  Tests the following:
  - a request made while the server runs a long sleep stops it, soon
  - without blocking, the same, the canceller closed and NULLed once done
  - a request with nothing running is made, and changes nothing
  - a request abandoned is closed with d_mariadb_close
  - NULL arguments are D_DB_INVALID_ARGUMENT
*/
static void
d_tests_mariadb_cancels(
    void
)
{
    struct d_mariadb*   db        = d_tests_mariadb_connect(NULL);
    struct d_mariadb*   watcher   = d_tests_mariadb_connect(NULL);
    struct d_mariadb*   canceller = NULL;
    char*               reason    = NULL;
    struct d_db_pending pending;

    printf("mariadb_connection: cancelling\n");

    // nothing more without a server, or a client that can send without
    // blocking
    if ( (!db)      ||
         (!watcher) ||
         (!d_tests_mariadb_async(false)) )
    {
        D_TESTS_MARIADB_CHECK( (db) && (watcher) );
        d_mariadb_close(db);
        d_mariadb_close(watcher);

        return;
    }

    time_t began = time(NULL);
    int    sent  = 0;

    D_TESTS_MARIADB_CHECK(d_tests_mariadb_sleeping(db,
                                                   watcher,
                                                   &pending,
                                                   &sent));
    D_TESTS_MARIADB_OK(watcher, d_mariadb_cancel(db, &reason));
    D_TESTS_MARIADB_CHECK(reason == NULL);
    d_tests_mariadb_stopped(db, began, sent, &pending);
    D_TESTS_MARIADB_OK(watcher, d_mariadb_cancel(db, NULL));
    D_TESTS_MARIADB_OK(db, d_mariadb_exec(db, "SELECT 1"));

    // without blocking
    struct d_db_pending sending;

    began = time(NULL);
    D_TESTS_MARIADB_CHECK(d_tests_mariadb_sleeping(db,
                                                   watcher,
                                                   &sending,
                                                   &sent));

    int result = d_mariadb_cancel_start(db, &canceller, &pending, &reason);

    // under way, or done at once where every answer came at once (a loaded
    // machine, with libmysqlclient)
    D_TESTS_MARIADB_CHECK( ( (result == D_DB_WAIT) &&
                             (canceller != NULL) ) ||
                           ( (result == 0) &&
                             (canceller == NULL) ) );

    for (int waits = 0;
         (result == D_DB_WAIT) && (waits < D_TESTS_MARIADB_ROUNDS);
         ++waits)
    {
        result = d_mariadb_cancel_continue(&canceller,
                                           d_tests_mariadb_wait(&pending),
                                           &pending,
                                           &reason);
    }

    D_TESTS_MARIADB_OK(watcher, result);
    D_TESTS_MARIADB_CHECK( (canceller == NULL) &&
                           (reason == NULL) &&
                           (d_tests_mariadb_unpended(&pending)) );
    d_tests_mariadb_stopped(db, began, sent, &sending);

    // abandoned under way, or done at once
    result = d_mariadb_cancel_start(db, &canceller, &pending, NULL);
    D_TESTS_MARIADB_CHECK( ( (result == D_DB_WAIT) &&
                             (canceller != NULL) ) ||
                           ( (result == 0) &&
                             (canceller == NULL) ) );
    D_TESTS_MARIADB_CHECK(d_mariadb_close(canceller) == 0);
    canceller = NULL;

    // arguments
    D_TESTS_MARIADB_RESULT(NULL, d_mariadb_cancel(NULL, NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(NULL, d_mariadb_cancel_start(NULL,
                                                        &canceller,
                                                        &pending,
                                                        NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(NULL, d_mariadb_cancel_start(db,
                                                        NULL,
                                                        &pending,
                                                        NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(NULL, d_mariadb_cancel_continue(NULL,
                                                           0u,
                                                           &pending,
                                                           NULL),
                           D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_RESULT(NULL, d_mariadb_cancel_continue(&canceller,
                                                           0u,
                                                           &pending,
                                                           NULL),
                           D_DB_INVALID_ARGUMENT);
    d_mariadb_close(db);
    d_mariadb_close(watcher);

    return;
}

/*
d_tests_mariadb_without_blocking
  The tests of calls that do not block, over connections without TLS where
the client cannot carry TLS so (d_tests_mariadb_tls_nonblocking tests that
refusal), and as the run's other connections otherwise.
*/
static void
d_tests_mariadb_without_blocking(
    void
)
{
    d_tests_mariadb_tls_nonblocking();
    g_plain_count = ( (d_tests_mariadb_async(false)) &&
                      (!d_mariadb_client_nonblocking_tls()) )
                        ? 1u
                        : 0u;

    if (g_plain_count != 0u)
    {
        printf("  (the tests that follow connect without TLS)\n");
    }

    d_tests_mariadb_connect_nonblocking();
    d_tests_mariadb_sent_nonblocking();
    d_tests_mariadb_statements_nonblocking();
    d_tests_mariadb_session_nonblocking();
    d_tests_mariadb_cancels();
    g_plain_count = 0u;

    return;
}


//==============================================================================
// RUNNER
//==============================================================================

// d_tests_mariadb_all
//   table: every test, in the headers' order.
static void (* const d_tests_mariadb_all[])(void) =
{
    d_tests_mariadb_db_common,
    d_tests_mariadb_client,
    d_tests_mariadb_versions,
    d_tests_mariadb_version_comparisons,
    d_tests_mariadb_features,
    d_tests_mariadb_feature_behaviors,
    d_tests_mariadb_messages,
    d_tests_mariadb_errors,
    d_tests_mariadb_connecting,
    d_tests_mariadb_connecting_with,
    d_tests_mariadb_client_options,
    d_tests_mariadb_options_read_back,
    d_tests_mariadb_server_public_key,
    d_tests_mariadb_adopting,
    d_tests_mariadb_changing_user,
    d_tests_mariadb_resetting,
    d_tests_mariadb_liveness,
    d_tests_mariadb_without_blocking,
    d_tests_mariadb_rows_changed,
    d_tests_mariadb_statements,
    d_tests_mariadb_scripts,
    d_tests_mariadb_integers,
    d_tests_mariadb_doubles,
    d_tests_mariadb_text,
    d_tests_mariadb_changes_counted,
    d_tests_mariadb_rows_values,
    d_tests_mariadb_rows_types,
    d_tests_mariadb_quoting_names,
    d_tests_mariadb_quoting_values,
    d_tests_mariadb_query_arguments,
    d_tests_mariadb_query_attributes,
    d_tests_mariadb_preparing,
    d_tests_mariadb_binding,
    d_tests_mariadb_running,
    d_tests_mariadb_cursors,
    d_tests_mariadb_cursor_closing,
    d_tests_mariadb_statement_arguments,
    d_tests_mariadb_databases,
    d_tests_mariadb_current_database,
    d_tests_mariadb_database_arguments,
    d_tests_mariadb_table_existence,
    d_tests_mariadb_table_creation,
    d_tests_mariadb_table_renames,
    d_tests_mariadb_table_rows,
    d_tests_mariadb_table_listing,
    d_tests_mariadb_table_arguments,
    d_tests_mariadb_creation_arguments,
    d_tests_mariadb_described_types,
    d_tests_mariadb_described_attributes,
    d_tests_mariadb_described_keys,
    d_tests_mariadb_described_refusals,
    d_tests_mariadb_defaults,
    d_tests_mariadb_default_kinds,
    d_tests_mariadb_column_existence,
    d_tests_mariadb_column_additions,
    d_tests_mariadb_column_removals,
    d_tests_mariadb_column_redefinitions,
    d_tests_mariadb_column_listing,
    d_tests_mariadb_column_arguments,
    d_tests_mariadb_indexes,
    d_tests_mariadb_index_changes,
    d_tests_mariadb_views,
    d_tests_mariadb_view_changes,
    d_tests_mariadb_triggers,
    d_tests_mariadb_trigger_changes,
    d_tests_mariadb_index_arguments,
    d_tests_mariadb_view_trigger_arguments,
    d_tests_mariadb_transactions,
    d_tests_mariadb_transaction_modes,
    d_tests_mariadb_savepoints,
    d_tests_mariadb_autocommit,
    d_tests_mariadb_isolation_and_waits,
    d_tests_mariadb_next_isolation,
    d_tests_mariadb_session_settings,
    d_tests_mariadb_variables,
    d_tests_mariadb_session_tracking,
    d_tests_mariadb_upkeep,
    d_tests_mariadb_upkeep_refusals
};

/*
main
  Connects once to name the run, runs every test and reports; the exit
status is 0 only if every check passed. Without a server to reach, it says
how to name one, and fails.
*/
int
main(
    void
)
{
    const size_t      count = sizeof(d_tests_mariadb_all) /
                              sizeof(d_tests_mariadb_all[0]);
    struct d_mariadb* setup = NULL;

    d_tests_mariadb_configure();
    setup = d_tests_mariadb_connect(NULL);

    // a server to test against
    if (!setup)
    {
        printf("no server: set DJINTERP_TEST_MARIADB_HOST, _PORT, _USER, "
               "_PASSWORD or _SOCKET\n");

        return 1;
    }

    snprintf(g_run,
             sizeof(g_run),
             "d_tests_mariadb_%lu",
             d_mariadb_connection_id(setup));
    printf("server %s; client %s\n",
           d_mariadb_server_version(setup),
           d_mariadb_client_version());
    d_mariadb_close(setup);

    for (size_t i = 0u; i < count; ++i)
    {
        d_tests_mariadb_all[i]();
    }

    printf("%d checks, %d failed\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
