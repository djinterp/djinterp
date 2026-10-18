/*******************************************************************************
* djinterp [c]                                               t_postgres_face.cpp
*
*   The C++ face of the PostgreSQL layer, postgres_face.hpp, compiled as C++
* at every level from ISO strict C++98 to C++26 and driven against a server.
* The layer's harness, t_postgres.c, tests what each function does; this
* tests what the face adds -- ownership, a failed connection's reason,
* string and list and text conversion, statements and rows, the guards, the
* static members, from C++11 moves and the f forms, and the forms each later
* level adds where the compiler has them: std::optional text and deduced
* guards (C++17), face::connection, std::span columns, UTF-8 strings and
* formats checked at compile time (C++20), std::expected results (C++23) --
* and samples every module, each member being generated from its C
* prototype, its arguments in the C order. The summary names the level forms
* the build had.
*   The server is the one t_postgres.c reaches, by the same settings
* (DJINTERP_TEST_POSTGRES_HOST, _PORT, _USER, _PASSWORD, _DATABASE). The
* test works in a schema of its own, and drops it. Without a server only the
* static checks run, unless DJINTERP_TEST_POSTGRES_REQUIRED is 1. Over a
* layer built without libpq it checks that the face reports what the stubs
* do. Below the 64-bit floor the face compiles to nothing, and so does this
* test. Defining D_TESTS_POSTGRES_FACE_MISUSE as 1 to 4 makes it misuse the
* face in a way that must not compile (see d_tests_postgres_face_variadic).
*
*   Build: compile the layer's C sources as C, this file as C++, and link them
* with libpq -- ci/check_db_postgres.sh does, at every level.
*
*
* path:      /tests/djinterp/c/db/postgres/t_postgres_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.14
*                                                            revised: 2026.10.16
*******************************************************************************/
// std
#include <cstdio>   // std::printf
#include <cstdlib>  // std::getenv, std::strtoul
#include <cstring>  // std::strcmp, std::strlen, std::strstr
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "../../../../../inc/djinterp/c/db/postgres/postgres_face.hpp"  // under
                                                                         // test
// re_std
#include "../../../../../inc/re_std/cstdint/cstdint.hpp"  // re_std::int64_t,
                                                         // INT64_MAX


#if defined(INT64_MAX)

namespace face = djinterp::face;

namespace
{

int g_checks   = 0;
int g_failures = 0;

#if ( (D_ENV_LANG_IS_CPP20_OR_HIGHER) &&                                      \
      (D_ENV_CPP_FEATURE_LANG_CONSTINIT) )
// g_unopened
//   a face at namespace scope, constant-initialized: no constructor runs at
// start-up, since an empty face is made at compile time.
constinit face::postgres g_unopened;
#endif  // C++20 constinit

/*
d_tests_postgres_face_expect
  Counts one check, and prints it when it fails.
*/
void
d_tests_postgres_face_expect(
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
        std::printf("  FAIL line %d: %s\n", _line, _what);
    }

    return;
}

/*
d_tests_postgres_face_setting
  An environment variable, or a default when it is unset or empty.
*/
const char*
d_tests_postgres_face_setting(
    const char* _name,
    const char* _default
)
{
    const char* const value = std::getenv(_name);

    return ( (value) && (value[0] != '\0') ) ? value : _default;
}

/*
d_tests_postgres_face_same
  Whether a C string is the text expected; NULL is no text.
*/
bool
d_tests_postgres_face_same(
    const char* _text,
    const char* _expected
)
{
    return ( (_text) &&
             (std::strcmp(_text, _expected) == 0) );
}

/*
d_tests_postgres_face_number
  A number's digits, by hand: snprintf is not C++98's, and sprintf is
deprecated on Apple's platforms.
*/
std::string
d_tests_postgres_face_number(
    unsigned long _number
)
{
    std::string digits;

    // the digits, last first
    do
    {
        digits.insert(digits.begin(),
                      static_cast<char>('0' + (_number % 10ul)));
        _number /= 10ul;
    } while (_number != 0ul);

    return digits;
}

}  // namespace

// D_TESTS_POSTGRES_FACE_CHECK
//   macro: records one check with its source text and line, which only the
// preprocessor can supply.
#define D_TESTS_POSTGRES_FACE_CHECK(_cond)                                    \
    d_tests_postgres_face_expect((_cond), #_cond, __LINE__)

// D_TESTS_POSTGRES_FACE_FORMS
//   constant: the forms of the levels above C++11 this build has, for the
// summary; empty when it has none.
#define D_TESTS_POSTGRES_FACE_FORMS                                           \
    D_TESTS_POSTGRES_FACE_FORM_OPTIONAL D_TESTS_POSTGRES_FACE_FORM_CONCEPTS   \
    D_TESTS_POSTGRES_FACE_FORM_SPAN D_TESTS_POSTGRES_FACE_FORM_CHAR8          \
    D_TESTS_POSTGRES_FACE_FORM_CHECK D_TESTS_POSTGRES_FACE_FORM_EXPECTED      \
    D_TESTS_POSTGRES_FACE_FORM_DELETED

#if D_INTERNAL_DB_FACE_OPTIONAL
    #define D_TESTS_POSTGRES_FACE_FORM_OPTIONAL " optional"
#else
    #define D_TESTS_POSTGRES_FACE_FORM_OPTIONAL ""
#endif
#if D_INTERNAL_DB_FACE_CONCEPTS
    #define D_TESTS_POSTGRES_FACE_FORM_CONCEPTS " concepts"
#else
    #define D_TESTS_POSTGRES_FACE_FORM_CONCEPTS ""
#endif
#if D_INTERNAL_DB_FACE_SPAN
    #define D_TESTS_POSTGRES_FACE_FORM_SPAN " span"
#else
    #define D_TESTS_POSTGRES_FACE_FORM_SPAN ""
#endif
#if D_INTERNAL_DB_FACE_CHAR8
    #define D_TESTS_POSTGRES_FACE_FORM_CHAR8 " char8_t"
#else
    #define D_TESTS_POSTGRES_FACE_FORM_CHAR8 ""
#endif
#if D_INTERNAL_DB_FACE_FORMAT_CHECK
    #define D_TESTS_POSTGRES_FACE_FORM_CHECK " checked-formats"
#else
    #define D_TESTS_POSTGRES_FACE_FORM_CHECK ""
#endif
#if D_INTERNAL_DB_FACE_EXPECTED
    #define D_TESTS_POSTGRES_FACE_FORM_EXPECTED " expected"
#else
    #define D_TESTS_POSTGRES_FACE_FORM_EXPECTED ""
#endif
#if ( (D_ENV_LANG_IS_CPP26_OR_HIGHER) &&                                      \
      (D_ENV_CPP_FEATURE_LANG_DELETED_FUNCTION) )
    #define D_TESTS_POSTGRES_FACE_FORM_DELETED " deleted-reasons"
#else
    #define D_TESTS_POSTGRES_FACE_FORM_DELETED ""
#endif

namespace
{

/*
d_tests_postgres_face_static
  The members that need no connection, in both forms where a member
returns text.
*/
void
d_tests_postgres_face_static(
    bool _stubs
)
{
    face::text quoted;
    char       spelled[6] = "?";

    std::printf("static members\n");

    D_TESTS_POSTGRES_FACE_CHECK(
        (face::postgres::client_version_number() == 0) == _stubs);
    D_TESTS_POSTGRES_FACE_CHECK(
        face::postgres::result_sqlstate(D_POSTGRES_SQLSTATE_UNDEFINED_TABLE) ==
        "42P01");
    D_TESTS_POSTGRES_FACE_CHECK(face::postgres::result_sqlstate(0) ==
                                "00000");
    D_TESTS_POSTGRES_FACE_CHECK(
        face::postgres::result_sqlstate(D_DB_NOT_FOUND).empty());
    D_TESTS_POSTGRES_FACE_CHECK(
        face::postgres::result_from_sqlstate(std::string("23505")) ==
        D_POSTGRES_SQLSTATE_UNIQUE_VIOLATION);
    D_TESTS_POSTGRES_FACE_CHECK(
        face::postgres::result_message(D_POSTGRES_SQLSTATE_UNDEFINED_TABLE) !=
        D_NULLPTR);
    D_TESTS_POSTGRES_FACE_CHECK(
        face::postgres::quote_identifier("a\"b") ==
        std::string((_stubs) ? "" : "\"a\"\"b\""));

    // the exact forms: the SQLSTATE into the caller's buffer, and the
    // layer's text owned by the caller's text, null where the layer gave
    // none
    D_TESTS_POSTGRES_FACE_CHECK(
        (face::postgres::result_sqlstate(D_POSTGRES_SQLSTATE_UNDEFINED_TABLE,
                                         spelled)) &&
        (d_tests_postgres_face_same(spelled, "42P01")));
    D_TESTS_POSTGRES_FACE_CHECK(
        (!face::postgres::result_sqlstate(D_DB_NOT_FOUND, spelled)) &&
        (d_tests_postgres_face_same(spelled, "")));
    face::postgres::quote_identifier("a\"b", quoted);
    D_TESTS_POSTGRES_FACE_CHECK(
        (_stubs) ? (quoted.is_null())
                 : (d_tests_postgres_face_same(quoted.c_str(),
                                               "\"a\"\"b\"")));
    face::postgres::quote_identifier(0, quoted);
    D_TESTS_POSTGRES_FACE_CHECK(quoted.is_null());

    return;
}

/*
d_tests_postgres_face_ownership
  Connecting, failing to with a reason, connecting with keywords beside a
string, swapping, giving up and replacing a connection; from C++11, moving
it; from C++20, a face constant-initialized.
*/
void
d_tests_postgres_face_ownership(
    const struct d_postgres_options& _where
)
{
    struct d_postgres_options nowhere = _where;
    face::postgres            db;
    face::postgres            other;
    face::text                why;

    std::printf("ownership\n");

#if ( (D_ENV_LANG_IS_CPP20_OR_HIGHER) &&                                      \
      (D_ENV_CPP_FEATURE_LANG_CONSTINIT) )
    D_TESTS_POSTGRES_FACE_CHECK(!g_unopened.is_open());
#endif  // C++20 constinit

    nowhere.host = "127.0.0.1";
    nowhere.port = 1u;
    D_TESTS_POSTGRES_FACE_CHECK(!db.is_open());
    D_TESTS_POSTGRES_FACE_CHECK(db.connect(&_where) == 0);
    D_TESTS_POSTGRES_FACE_CHECK(db.is_open());

    struct pg_conn* const first = db.handle();

    // a failed connect leaves the connection as it was, and says why
    D_TESTS_POSTGRES_FACE_CHECK(db.connect(&nowhere, why) ==
                                D_POSTGRES_SQLSTATE_CANNOT_CONNECT);
    D_TESTS_POSTGRES_FACE_CHECK( (!why.is_null()) &&
                                 (std::strlen(why.c_str()) > 0u) );
    D_TESTS_POSTGRES_FACE_CHECK(db.handle() == first);
    D_TESTS_POSTGRES_FACE_CHECK(db.connect_string("port=1 host=127.0.0.1",
                                                  why) != 0);
    D_TESTS_POSTGRES_FACE_CHECK(db.handle() == first);
    D_TESTS_POSTGRES_FACE_CHECK(db.connect_string("a=b=c", why) ==
                                D_DB_INVALID_ARGUMENT);

    // libpq's keywords beside a string: as pointers and a count, as
    // arrays, and from C++20 as spans, which must be the same size; a
    // keyword libpq does not know leaves the connection as it was
    {
        char port[16] = "";

        std::snprintf(port, sizeof(port), "%u", _where.port);

        const char* const keywords[5] = { "host", "port", "user",
                                          "password", "dbname" };
        const char* const values[5]   = { _where.host, port, _where.user,
                                          _where.password, _where.database };
        const char* const unknown[1]  = { "no_such_option" };
        const char* const nothing[1]  = { "x" };
        face::postgres    keyed;

        D_TESTS_POSTGRES_FACE_CHECK(keyed.connect_with(0, keywords, values) ==
                                    0);
        D_TESTS_POSTGRES_FACE_CHECK(keyed.is_open());
        D_TESTS_POSTGRES_FACE_CHECK(
            keyed.connect_with("application_name=d_tests_face",
                               keywords,
                               values,
                               5u,
                               why) == 0);
        D_TESTS_POSTGRES_FACE_CHECK(why.is_null());

        struct pg_conn* const held = keyed.handle();

        D_TESTS_POSTGRES_FACE_CHECK(keyed.connect_with(0, unknown, nothing,
                                                       why) ==
                                    D_DB_INVALID_ARGUMENT);
        D_TESTS_POSTGRES_FACE_CHECK(
            (!why.is_null()) &&
            (std::strstr(why.c_str(), "no_such_option") != 0));
        D_TESTS_POSTGRES_FACE_CHECK(keyed.handle() == held);
        D_TESTS_POSTGRES_FACE_CHECK(keyed.connect_with(0, unknown, nothing,
                                                       1u) ==
                                    D_DB_INVALID_ARGUMENT);

#if D_INTERNAL_DB_FACE_SPAN
        D_TESTS_POSTGRES_FACE_CHECK(
            keyed.connect_with(0,
                               std::span<const char* const>(keywords),
                               std::span<const char* const>(values)) == 0);
        D_TESTS_POSTGRES_FACE_CHECK(
            keyed.connect_with(0,
                               std::span<const char* const>(keywords),
                               std::span<const char* const>(values, 4u),
                               why) == D_DB_INVALID_ARGUMENT);
        D_TESTS_POSTGRES_FACE_CHECK(keyed.is_open());
#endif  // D_INTERNAL_DB_FACE_SPAN
    }
    D_TESTS_POSTGRES_FACE_CHECK(db.ping() == 0);
    other.swap(db);
    D_TESTS_POSTGRES_FACE_CHECK( (!db.is_open()) &&
                                 (other.handle() == first) );
    swap(db, other);

    struct pg_conn* const raw = db.release();

    D_TESTS_POSTGRES_FACE_CHECK( (raw == first) && (!db.is_open()) );

    face::postgres adopted(raw);

    D_TESTS_POSTGRES_FACE_CHECK(adopted.exec("SELECT 1") == 0);
    D_TESTS_POSTGRES_FACE_CHECK(adopted.backend_pid() > 0);

#if D_MOVE_ENABLED
    face::postgres moved(static_cast<face::postgres&&>(adopted));

    D_TESTS_POSTGRES_FACE_CHECK( (!adopted.is_open()) &&
                                 (moved.handle() == raw) );
    D_TESTS_POSTGRES_FACE_CHECK(static_cast<bool>(moved));
    adopted = static_cast<face::postgres&&>(moved);
    D_TESTS_POSTGRES_FACE_CHECK(adopted.handle() == raw);
#endif  // D_MOVE_ENABLED

    adopted.reset();
    D_TESTS_POSTGRES_FACE_CHECK(!adopted.is_open());
    D_TESTS_POSTGRES_FACE_CHECK(adopted.close() == 0);
    D_TESTS_POSTGRES_FACE_CHECK(adopted.exec("SELECT 1") ==
                                D_DB_INVALID_ARGUMENT);
    D_TESTS_POSTGRES_FACE_CHECK(!adopted.has_table(0, "anything"));

    return;
}

/*
d_tests_postgres_face_conversions
  string_arg from a C string, a std::string and NULL; a column array; a
name list and a vector; a column list; text, null and not, and a
std::string; the numbers and booleans a member writes.
*/
void
d_tests_postgres_face_conversions(
    face::postgres&    _db,
    const std::string& _schema
)
{
    static const struct d_db_column people[] =
    {
        { "id",   D_DB_TYPE_BIG_INTEGER,
          D_DB_COLUMN_PRIMARY_KEY | D_DB_COLUMN_AUTO_INCREMENT, 0, 0u, 0u, 0 },
        { "name", D_DB_TYPE_STRING, D_DB_COLUMN_NOT_NULL, "'?'", 40u, 0u, 0 }
    };
    face::name_list          names;
    std::vector<std::string> copied;
    face::column_list        columns;
    face::text               value;
    std::string              plain("unchanged");
    re_std::int64_t          count  = -1;
    bool                     exists = false;

    std::printf("conversions\n");

    D_TESTS_POSTGRES_FACE_CHECK(_db.create_table(_schema, "people", people,
                                                 0u) == 0);
    D_TESTS_POSTGRES_FACE_CHECK(
        _db.create_table_sql(0, std::string("notes"),
                             "id integer PRIMARY KEY, body text",
                             D_DB_IF_NOT_EXISTS) == 0);
    D_TESTS_POSTGRES_FACE_CHECK(_db.has_table(0, "notes"));
    D_TESTS_POSTGRES_FACE_CHECK(
        (_db.has_table_s(_schema, "people", exists) == 0) && (exists));
    D_TESTS_POSTGRES_FACE_CHECK(_db.list_tables(0, names) == 0);
    D_TESTS_POSTGRES_FACE_CHECK( (names.size() == 2u)                 &&
                                 (d_tests_postgres_face_same(names[0],
                                                             "notes")) &&
                                 (names.contains("people")) );
    D_TESTS_POSTGRES_FACE_CHECK(_db.list_tables(_schema, copied) == 0);
    D_TESTS_POSTGRES_FACE_CHECK( (copied.size() == 2u) &&
                                 (copied[1] == "people") );
    D_TESTS_POSTGRES_FACE_CHECK(_db.list_columns(0, "people", columns) == 0);
    D_TESTS_POSTGRES_FACE_CHECK( (columns.size() == 2u)                  &&
                                 (columns.find("id"))                    &&
                                 (columns.find("id")->primary_key == 1)  &&
                                 (columns[1].not_null) );
    D_TESTS_POSTGRES_FACE_CHECK(
        _db.exec("INSERT INTO notes VALUES (1, NULL), (2, 'x')") == 0);
    D_TESTS_POSTGRES_FACE_CHECK(
        (_db.query_text(value, "SELECT body FROM notes WHERE id = 1") == 0) &&
        (value.is_null()));
    D_TESTS_POSTGRES_FACE_CHECK(
        (_db.query_text(plain, "SELECT body FROM notes WHERE id = 2") == 0) &&
        (plain == "x"));
    D_TESTS_POSTGRES_FACE_CHECK(
        (_db.query_int64(count, "SELECT count(*) FROM notes") == 0) &&
        (count == 2));
    D_TESTS_POSTGRES_FACE_CHECK(
        (_db.count_rows(0, "people", count) == 0) && (count == 0));
    D_TESTS_POSTGRES_FACE_CHECK(_db.quote_literal("it's") ==
                                std::string("'it''s'"));
    _db.quote_literal("it's", value);
    D_TESTS_POSTGRES_FACE_CHECK(
        d_tests_postgres_face_same(value.c_str(), "'it''s'"));

    return;
}

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

/*
d_tests_postgres_face_cpp17
  C++17's forms: text as a std::optional<std::string>, empty for SQL NULL
and for a failure; and guards that deduce their face. Over the notes
d_tests_postgres_face_conversions made: 1 NULL, 2 'x'.
*/
void
d_tests_postgres_face_cpp17(
    face::postgres&    _db,
    const std::string& _schema
)
{
    std::printf("C++17 forms\n");

#if D_INTERNAL_DB_FACE_OPTIONAL
    std::optional<std::string> body(std::string("stale"));
    std::optional<std::string> schema;

    D_TESTS_POSTGRES_FACE_CHECK(
        _db.query_text(body, "SELECT body FROM notes WHERE id = 1") == 0);
    D_TESTS_POSTGRES_FACE_CHECK(!body.has_value());
    D_TESTS_POSTGRES_FACE_CHECK(
        _db.query_text(body, "SELECT body FROM notes WHERE id = 2") == 0);
    D_TESTS_POSTGRES_FACE_CHECK( (body.has_value()) && (*body == "x") );
    D_TESTS_POSTGRES_FACE_CHECK(
        _db.query_text(body, "SELECT body FROM notes WHERE id = 9") ==
        D_DB_NOT_FOUND);
    D_TESTS_POSTGRES_FACE_CHECK(!body.has_value());
    D_TESTS_POSTGRES_FACE_CHECK(
        (_db.query_textf(body, "SELECT body FROM notes WHERE id = %d", 2) ==
         0) &&
        (body == std::string("x")));
    D_TESTS_POSTGRES_FACE_CHECK(
        (_db.current_schema(schema) == 0) && (schema == _schema));
#else
    (void)_schema;
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

#if D_ENV_CPP_FEATURE_LANG_DEDUCTION_GUIDES
    {
        face::transaction work(_db);

        D_TESTS_POSTGRES_FACE_CHECK(work.is_active());

        {
            face::savepoint step(_db, "step");

            D_TESTS_POSTGRES_FACE_CHECK( (step.is_active()) &&
                                         (step.release() == 0) );
        }

        D_TESTS_POSTGRES_FACE_CHECK(work.rollback() == 0);
    }

    D_TESTS_POSTGRES_FACE_CHECK(!_db.in_transaction());
#endif  // D_ENV_CPP_FEATURE_LANG_DEDUCTION_GUIDES

    return;
}

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#if D_INTERNAL_DB_FACE_CONCEPTS

/*
d_tests_postgres_face_generic
  Code written for any face, through face::connection: a transaction begun
and rolled back, its guard deduced.
*/
template<face::connection Database>
bool
d_tests_postgres_face_generic(
    Database& _db
)
{
    face::transaction work(_db);

    return ( (work.is_active())      &&
             (_db.in_transaction())  &&
             (work.rollback() == 0)  &&
             (!_db.in_transaction()) );
}

#endif  // D_INTERNAL_DB_FACE_CONCEPTS

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

/*
d_tests_postgres_face_cpp20
  C++20's forms: the face as a face::connection; a column array as a
std::span, or anything that makes one; UTF-8 strings; and a format read at
compile time.
*/
void
d_tests_postgres_face_cpp20(
    face::postgres&    _db,
    const std::string& _schema
)
{
    static const struct d_db_column columns[] =
    {
        { "id", D_DB_TYPE_BIG_INTEGER, D_DB_COLUMN_PRIMARY_KEY, 0, 0u, 0u, 0 }
    };
    re_std::int64_t count = -1;

    std::printf("C++20 forms\n");

#if D_INTERNAL_DB_FACE_CONCEPTS
    static_assert(face::connection<face::postgres>,
                  "face::postgres is a face::connection");
    D_TESTS_POSTGRES_FACE_CHECK(d_tests_postgres_face_generic(_db));
#endif  // D_INTERNAL_DB_FACE_CONCEPTS

#if D_INTERNAL_DB_FACE_SPAN
    const std::vector<struct d_db_column> held(columns, columns + 1);

    D_TESTS_POSTGRES_FACE_CHECK(
        _db.create_table(0, "spanned", std::span<const d_db_column>(columns),
                         0u) == 0);
    D_TESTS_POSTGRES_FACE_CHECK(_db.create_table(0, "held", held, 0u) == 0);
#else
    D_TESTS_POSTGRES_FACE_CHECK(
        _db.create_table(0, "spanned", columns, 0u) == 0);
    D_TESTS_POSTGRES_FACE_CHECK(_db.create_table(0, "held", columns, 0u) ==
                                0);
#endif  // D_INTERNAL_DB_FACE_SPAN

#if D_INTERNAL_DB_FACE_CHAR8
    const std::u8string name(u8"held");

    D_TESTS_POSTGRES_FACE_CHECK(_db.has_table(0, u8"spanned"));
    D_TESTS_POSTGRES_FACE_CHECK(_db.has_table(0, name));
#endif  // D_INTERNAL_DB_FACE_CHAR8

#if D_INTERNAL_DB_FACE_FORMAT_CHECK
    // a format checked at compile time is a pointer to the literal
    constexpr face::postgres::format<const char*, const char*> both(
        "SELECT count(*) FROM %T");

    static_assert(sizeof(both) == sizeof(const char*),
                  "a checked format is its pointer");
    D_TESTS_POSTGRES_FACE_CHECK(
        d_tests_postgres_face_same(both.c_str(), "SELECT count(*) FROM %T"));
    D_TESTS_POSTGRES_FACE_CHECK(
        (_db.query_int64f(count, both, _schema.c_str(), "spanned") == 0) &&
        (count == 0));
#else
    (void)_schema;
#endif  // D_INTERNAL_DB_FACE_FORMAT_CHECK

    // the same calls compile at every level from C++11; from C++20 each
    // literal is read against its arguments first
    D_TESTS_POSTGRES_FACE_CHECK(
        (_db.query_int64f(count, "SELECT count(*) FROM %I", "held") == 0) &&
        (count == 0));
    D_TESTS_POSTGRES_FACE_CHECK(
        (_db.query_int64f(count, "SELECT %lld + %d",
                          static_cast<long long>(40), 2) == 0) &&
        (count == 42));

    return;
}

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#if D_INTERNAL_DB_FACE_EXPECTED

/*
d_tests_postgres_face_cpp23
  C++23's forms: a result written through a parameter, returned instead,
as a std::expected that holds the value or the layer's result. Over the
notes and people d_tests_postgres_face_conversions made.
*/
void
d_tests_postgres_face_cpp23(
    face::postgres&    _db,
    const std::string& _schema
)
{
    std::printf("C++23 forms\n");

    const auto count     = _db.query_int64("SELECT count(*) FROM notes");
    const auto missing   = _db.query_text(
                               "SELECT body FROM notes WHERE id = 9");
    const auto null_body = _db.query_text(
                               "SELECT body FROM notes WHERE id = 1");
    const auto body      = _db.query_textf(
                               "SELECT body FROM notes WHERE id = %d", 2);
    const auto truth     = _db.query_bool("SELECT 2 > 1");
    const auto tables    = _db.list_tables(0);
    const auto people    = _db.list_columns(0, "people");
    const auto exists    = _db.has_table_s(0, "people");
    const auto schema    = _db.current_schema();
    const auto nothing   = face::postgres().query_int64("SELECT 1");

    D_TESTS_POSTGRES_FACE_CHECK( (count.has_value()) && (*count == 2) );
    D_TESTS_POSTGRES_FACE_CHECK( (!missing.has_value()) &&
                                 (missing.error() == D_DB_NOT_FOUND) );
    D_TESTS_POSTGRES_FACE_CHECK( (null_body.has_value()) &&
                                 (null_body->is_null()) );
    D_TESTS_POSTGRES_FACE_CHECK( (body.has_value()) &&
                                 (d_tests_postgres_face_same(body->c_str(),
                                                             "x")) );
    D_TESTS_POSTGRES_FACE_CHECK( (truth.has_value()) && (*truth) );
    D_TESTS_POSTGRES_FACE_CHECK( (tables.has_value()) &&
                                 (tables->contains("notes")) );
    D_TESTS_POSTGRES_FACE_CHECK( (people.has_value()) &&
                                 (people->find("name")) );
    D_TESTS_POSTGRES_FACE_CHECK( (exists.has_value()) && (*exists) );
    D_TESTS_POSTGRES_FACE_CHECK(
        (schema.has_value()) &&
        (d_tests_postgres_face_same(schema->c_str(), _schema.c_str())));
    D_TESTS_POSTGRES_FACE_CHECK( (!nothing.has_value()) &&
                                 (nothing.error() == D_DB_INVALID_ARGUMENT) );

    return;
}

#endif  // D_INTERNAL_DB_FACE_EXPECTED

/*
d_tests_postgres_face_statements
  Rows and statements: a statement prepared into a face, its parameters
counted and bound -- text from a std::string too -- run for a count and for
rows; rows read by place and by name, their values converted; a statement
finalized by hand, by its destructor, and on a failed prepare, which leaves
it empty; from C++11 one moved and swapped, and the f form; from C++23 the
statement, the rows, a column's place and a conversion returned.
*/
void
d_tests_postgres_face_statements(
    face::postgres& _db
)
{
    face::postgres_statement insert;
    face::postgres_statement select;
    face::rows               found;
    re_std::int64_t          changes = -1;
    re_std::int64_t          number  = 0;
    double                   real    = 0.0;
    bool                     truth   = false;
    std::size_t              index   = 99u;

    std::printf("statements and rows\n");

    D_TESTS_POSTGRES_FACE_CHECK(
        _db.exec("CREATE TABLE kept (id INTEGER, flag boolean, "
                 "ratio double precision, body TEXT, raw bytea)") == 0);
    D_TESTS_POSTGRES_FACE_CHECK(!insert.is_prepared());
    D_TESTS_POSTGRES_FACE_CHECK(
        _db.prepare(insert,
                    "INSERT INTO kept VALUES ($1, $2, $3, $4, $5)") == 0);
    D_TESTS_POSTGRES_FACE_CHECK( (insert.is_prepared()) &&
                                 (insert.handle() != 0) );
    D_TESTS_POSTGRES_FACE_CHECK(insert.statement_connection() == _db.handle());
    D_TESTS_POSTGRES_FACE_CHECK(insert.parameter_count() == 5u);
    D_TESTS_POSTGRES_FACE_CHECK(insert.bind_int64(1u, 7) == 0);
    D_TESTS_POSTGRES_FACE_CHECK(insert.bind_bool(2u, true) == 0);
    D_TESTS_POSTGRES_FACE_CHECK(insert.bind_double(3u, 0.5) == 0);
    D_TESTS_POSTGRES_FACE_CHECK(insert.bind_text(4u, std::string("it's")) ==
                                0);
    D_TESTS_POSTGRES_FACE_CHECK(insert.bind_blob(5u, "\0\1", 2u) == 0);
    D_TESTS_POSTGRES_FACE_CHECK(
        (insert.execute_update(changes) == 0) && (changes == 1));
    D_TESTS_POSTGRES_FACE_CHECK(insert.bind_int64(6u, 1) ==
                                D_POSTGRES_SQLSTATE_UNDEFINED_PARAMETER);
    D_TESTS_POSTGRES_FACE_CHECK(insert.clear_bindings() == 0);
    D_TESTS_POSTGRES_FACE_CHECK(insert.bind_null(1u) == 0);
    D_TESTS_POSTGRES_FACE_CHECK(insert.bind_text(4u, 0) == 0);
    D_TESTS_POSTGRES_FACE_CHECK(insert.execute_query(found) == 0);
    D_TESTS_POSTGRES_FACE_CHECK( (found.column_count() == 0u) &&
                                 (found.empty()) );

    // rows, by place and by name, converted
    D_TESTS_POSTGRES_FACE_CHECK(
        _db.prepare(select, "SELECT id, flag, ratio, body, raw FROM kept "
                            "WHERE id IS NOT NULL") == 0);
    D_TESTS_POSTGRES_FACE_CHECK(select.execute_query(found) == 0);
    D_TESTS_POSTGRES_FACE_CHECK( (found.row_count() == 1u)    &&
                                 (found.column_count() == 5u) &&
                                 (!found.empty()) );
    D_TESTS_POSTGRES_FACE_CHECK(
        (d_tests_postgres_face_same(found.column(3).name, "body")) &&
        (found.column(1).type == D_DB_TYPE_BOOLEAN) &&
        (d_tests_postgres_face_same(found.column(1).type_name,
                                 "boolean")));
    D_TESTS_POSTGRES_FACE_CHECK(
        (face::value_int64(found.value(0u, 0u), number) == 0) &&
        (number == 7));
    D_TESTS_POSTGRES_FACE_CHECK(
        (face::value_bool(found.value(0u, 1u), truth) == 0) && (truth));
    D_TESTS_POSTGRES_FACE_CHECK(
        (face::value_double(found.value(0u, 2u), real) == 0) &&
        (real == 0.5));
    D_TESTS_POSTGRES_FACE_CHECK(
        face::value_int64(found.value(0u, 3u), number) == D_DB_MISMATCH);
    D_TESTS_POSTGRES_FACE_CHECK(
        d_tests_postgres_face_same(found.value(0u, 3u)->as.text, "it's"));
    D_TESTS_POSTGRES_FACE_CHECK( (found.value(0u, 4u)->type ==
                                  D_DB_VALUE_BLOB) &&
                                 (found.value(0u, 4u)->size == 2u) );
    D_TESTS_POSTGRES_FACE_CHECK(found.value(1u, 0u) == 0);
    D_TESTS_POSTGRES_FACE_CHECK(
        (found.column_index("RATIO", index) == 0) && (index == 2u));
    D_TESTS_POSTGRES_FACE_CHECK(found.column_index("nobody", index) ==
                                D_DB_NOT_FOUND);
    found.clear();
    D_TESTS_POSTGRES_FACE_CHECK( (found.row_count() == 0u) &&
                                 (found.core().columns == 0) );

    // the connection's own counts and rows
    D_TESTS_POSTGRES_FACE_CHECK(
        (_db.exec_changes(changes, "UPDATE kept SET flag = false") == 0) &&
        (changes == 2));
    D_TESTS_POSTGRES_FACE_CHECK(
        _db.query_rows(found, "SELECT count(*) AS n FROM kept") == 0);
    D_TESTS_POSTGRES_FACE_CHECK( (found.row_count() == 1u) &&
                                 (found.value(0u, 0u)->as.integer == 2) );

    // finalized by hand, and emptied by a prepare that fails
    D_TESTS_POSTGRES_FACE_CHECK(select.finalize() == 0);
    D_TESTS_POSTGRES_FACE_CHECK(!select.is_prepared());
    D_TESTS_POSTGRES_FACE_CHECK(_db.prepare(select, "SELECT 1") == 0);
    D_TESTS_POSTGRES_FACE_CHECK(_db.prepare(select, "SELEC 1") ==
                                D_POSTGRES_SQLSTATE_SYNTAX_ERROR);
    D_TESTS_POSTGRES_FACE_CHECK(!select.is_prepared());

    // handed over, and taken back
    D_TESTS_POSTGRES_FACE_CHECK(_db.prepare(select, "SELECT 2") == 0);
    {
        struct d_postgres_statement* const raw = select.release();

        D_TESTS_POSTGRES_FACE_CHECK( (raw != 0) && (!select.is_prepared()) );
        select.reset(raw);
        D_TESTS_POSTGRES_FACE_CHECK(select.handle() == raw);
    }
    insert.swap(select);
    D_TESTS_POSTGRES_FACE_CHECK( (insert.parameter_count() == 0u) &&
                                 (select.parameter_count() == 5u) );
    swap(insert, select);
    D_TESTS_POSTGRES_FACE_CHECK(insert.parameter_count() == 5u);

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    {
        face::postgres_statement moved(
            static_cast<face::postgres_statement&&>(insert));

        D_TESTS_POSTGRES_FACE_CHECK( (moved) && (!insert) );
        insert = static_cast<face::postgres_statement&&>(moved);
        D_TESTS_POSTGRES_FACE_CHECK( (insert) && (!moved) );

        face::rows taken(static_cast<face::rows&&>(found));

        D_TESTS_POSTGRES_FACE_CHECK( (taken.row_count() == 1u) &&
                                     (found.row_count() == 0u) );
    }
    D_TESTS_POSTGRES_FACE_CHECK(
        _db.preparef(select, "SELECT id FROM %I WHERE body = %Q", "kept",
                     "it's") == 0);
    D_TESTS_POSTGRES_FACE_CHECK(
        (select.execute_update(changes) == 0) && (changes == 1));
    D_TESTS_POSTGRES_FACE_CHECK(
        _db.query_rowsf(found, "SELECT %d + %d", 2, 3) == 0);
    D_TESTS_POSTGRES_FACE_CHECK(found.value(0u, 0u)->as.integer == 5);
    D_TESTS_POSTGRES_FACE_CHECK(
        (_db.execf_changes(changes, "DELETE FROM kept WHERE id IS %s",
                           "NULL") == 0) &&
        (changes == 1));
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_INTERNAL_DB_FACE_EXPECTED
    {
        auto       prepared = _db.prepare("SELECT id, body FROM kept");
        const auto refused  = _db.prepare("SELEC");
        const auto all      = _db.query_rows("SELECT id FROM kept");

        D_TESTS_POSTGRES_FACE_CHECK( (prepared.has_value()) &&
                                     (prepared->is_prepared()) );
        D_TESTS_POSTGRES_FACE_CHECK( (!refused.has_value()) &&
                                     (refused.error() ==
                                      D_POSTGRES_SQLSTATE_SYNTAX_ERROR) );
        D_TESTS_POSTGRES_FACE_CHECK( (all.has_value()) &&
                                     (all->row_count() == 1u) );

        // nothing more without a statement
        if (prepared)
        {
            const auto rows_of = prepared->execute_query();
            const auto count   = prepared->execute_update();

            D_TESTS_POSTGRES_FACE_CHECK( (rows_of.has_value()) &&
                                         (rows_of->row_count() == 1u) );
            D_TESTS_POSTGRES_FACE_CHECK( (count.has_value()) &&
                                         (*count == 1) );
            D_TESTS_POSTGRES_FACE_CHECK(
                (rows_of) &&
                (rows_of->column_index("body") == std::size_t(1u)));
            D_TESTS_POSTGRES_FACE_CHECK(
                (rows_of) &&
                (face::value_int64(rows_of->value(0u, 0u)) ==
                 re_std::int64_t(7)));
            D_TESTS_POSTGRES_FACE_CHECK(
                (rows_of) &&
                (face::value_double(rows_of->value(0u, 1u)).error() ==
                 D_DB_MISMATCH));
        }
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    D_TESTS_POSTGRES_FACE_CHECK(_db.exec("DROP TABLE kept") == 0);

    return;
}

/*
d_tests_postgres_face_modules
  A member or two of each module beyond the ones above: servers and
errors, databases and schemas, schema objects, settings and maintenance.
*/
void
d_tests_postgres_face_modules(
    face::postgres&    _db,
    const std::string& _schema
)
{
    face::text      current;
    std::string     path;
    unsigned        milliseconds = 0u;
    re_std::int64_t number       = 0;

    std::printf("modules\n");

    D_TESTS_POSTGRES_FACE_CHECK(_db.server_version_number() >=
                                D_POSTGRES_FLOOR_SERVER_VERSION_NUM);
    D_TESTS_POSTGRES_FACE_CHECK(_db.server_at_least(10u, 0u));
    D_TESTS_POSTGRES_FACE_CHECK(_db.exec("SELECT * FROM nope") ==
                                D_POSTGRES_SQLSTATE_UNDEFINED_TABLE);
    D_TESTS_POSTGRES_FACE_CHECK( (_db.error_message()) &&
                                 (std::strstr(_db.error_message(), "nope")) );
    D_TESTS_POSTGRES_FACE_CHECK(_db.current_database() != D_NULLPTR);
    D_TESTS_POSTGRES_FACE_CHECK(
        (_db.current_schema(current) == 0) &&
        (d_tests_postgres_face_same(current.c_str(), _schema.c_str())));
    D_TESTS_POSTGRES_FACE_CHECK(_db.has_schema(_schema));
    D_TESTS_POSTGRES_FACE_CHECK(
        _db.create_index(0, "notes_body", "notes", "body", 0u) == 0);
    D_TESTS_POSTGRES_FACE_CHECK(_db.has_index(0, "notes_body"));
    D_TESTS_POSTGRES_FACE_CHECK(_db.create_sequence(0, "s", 0u) == 0);
    D_TESTS_POSTGRES_FACE_CHECK(
        (_db.next_value(0, "s", number) == 0) && (number == 1));
    D_TESTS_POSTGRES_FACE_CHECK(_db.set_statement_timeout(7000u) == 0);
    D_TESTS_POSTGRES_FACE_CHECK(
        (_db.get_statement_timeout(milliseconds) == 0) &&
        (milliseconds == 7000u));
    D_TESTS_POSTGRES_FACE_CHECK(
        (_db.get_search_path(path) == 0) &&
        (path.find(_schema) != std::string::npos));
    D_TESTS_POSTGRES_FACE_CHECK(
        (_db.table_size(0, "notes", number) == 0) && (number > 0));
    D_TESTS_POSTGRES_FACE_CHECK(
        (_db.last_value(number) == 0) && (number == 1));
    D_TESTS_POSTGRES_FACE_CHECK( (_db.parameter_status("server_version")) &&
                                 (std::strcmp(_db.parameter_status(
                                                  "server_version"),
                                              _db.server_version()) == 0) );

    // values apart from the SQL
    {
        const char* const values[] = { "20", "22" };
        face::rows        sum;

        D_TESTS_POSTGRES_FACE_CHECK(
            _db.query_rows_params(sum, "SELECT $1::int + $2::int", values,
                                  2u) == 0);
        D_TESTS_POSTGRES_FACE_CHECK(
            (face::value_int64(sum.value(0u, 0u), number) == 0) &&
            (number == 42));
    }

    // COPY in, a notification to itself, and a statement sent
    {
        const std::string row("7\tcopied\n");
        std::string       channel;
        std::string       payload;
        int               pid = 0;
        face::rows        sent;

        D_TESTS_POSTGRES_FACE_CHECK(
            _db.copy_in_start("COPY notes (id, body) FROM STDIN") == 0);
        D_TESTS_POSTGRES_FACE_CHECK(
            _db.copy_in_data(row.data(), row.size()) == 0);
        D_TESTS_POSTGRES_FACE_CHECK(
            (_db.copy_in_end(D_NULLPTR, number) == 0) && (number == 1));
        D_TESTS_POSTGRES_FACE_CHECK(_db.listen("face_channel") == 0);
        D_TESTS_POSTGRES_FACE_CHECK(_db.notify("face_channel", "hi") == 0);
        D_TESTS_POSTGRES_FACE_CHECK(
            (_db.next_notification(channel, payload, pid) == 0) &&
            (channel == "face_channel") && (payload == "hi") &&
            (pid == _db.backend_pid()));
        D_TESTS_POSTGRES_FACE_CHECK(_db.unlisten(D_NULLPTR) == 0);
        D_TESTS_POSTGRES_FACE_CHECK(_db.send_query("SELECT 5") == 0);
        D_TESTS_POSTGRES_FACE_CHECK(_db.next_result(sent, number) == 0);
        D_TESTS_POSTGRES_FACE_CHECK(_db.next_result(sent, number) ==
                                    D_DB_NOT_FOUND);
        D_TESTS_POSTGRES_FACE_CHECK(!_db.is_busy());
        D_TESTS_POSTGRES_FACE_CHECK(_db.consume_input() == 0);
    }

    return;
}

/*
d_tests_postgres_face_guards
  The transaction guard: committed, rolled back when its scope ends, and one
whose transaction failed; the savepoint guard: released, rolled back by
hand, and rolled back when its scope ends, named by a literal or by a
string that outlives it.
*/
void
d_tests_postgres_face_guards(
    face::postgres& _db
)
{
    const std::string dropped_name("dropped");
    re_std::int64_t   count = 0;

    std::printf("guards\n");

    D_TESTS_POSTGRES_FACE_CHECK(_db.exec("CREATE TABLE g (v integer)") == 0);

    {
        face::transaction<face::postgres> work(_db);

        D_TESTS_POSTGRES_FACE_CHECK( (work.result() == 0) &&
                                     (_db.in_transaction()) );
        D_TESTS_POSTGRES_FACE_CHECK(_db.exec("INSERT INTO g VALUES (1)") ==
                                    0);
        D_TESTS_POSTGRES_FACE_CHECK(work.commit() == 0);
        D_TESTS_POSTGRES_FACE_CHECK(!work.is_active());
    }

    {
        face::transaction<face::postgres> work(
            _db, D_POSTGRES_TRANSACTION_SERIALIZABLE);

        D_TESTS_POSTGRES_FACE_CHECK(_db.exec("INSERT INTO g VALUES (2)") ==
                                    0);
    }

    D_TESTS_POSTGRES_FACE_CHECK(!_db.in_transaction());

    {
        face::transaction<face::postgres> work(_db);

        D_TESTS_POSTGRES_FACE_CHECK(_db.exec("SELECT 1 / 0") != 0);
        D_TESTS_POSTGRES_FACE_CHECK(_db.in_failed_transaction());
        D_TESTS_POSTGRES_FACE_CHECK(_db.transaction_status() ==
                                    D_POSTGRES_STATE_IN_FAILED_TRANSACTION);
        D_TESTS_POSTGRES_FACE_CHECK(
            work.commit() == D_POSTGRES_SQLSTATE_IN_FAILED_SQL_TRANSACTION);
        D_TESTS_POSTGRES_FACE_CHECK( (!work.is_active()) &&
                                     (!_db.in_transaction()) );
    }

    {
        face::transaction<face::postgres> work(_db);

        {
            face::savepoint<face::postgres> kept(_db, "kept");

            D_TESTS_POSTGRES_FACE_CHECK(
                _db.exec("INSERT INTO g VALUES (4)") == 0);
            D_TESTS_POSTGRES_FACE_CHECK(kept.release() == 0);
        }

        {
            face::savepoint<face::postgres> dropped(_db, dropped_name);

            D_TESTS_POSTGRES_FACE_CHECK(
                _db.exec("INSERT INTO g VALUES (8)") == 0);
        }

        {
            face::savepoint<face::postgres> undone(_db, "undone");

            D_TESTS_POSTGRES_FACE_CHECK(_db.exec("SELECT 1 / 0") != 0);
            D_TESTS_POSTGRES_FACE_CHECK(undone.rollback() == 0);
            D_TESTS_POSTGRES_FACE_CHECK(!_db.in_failed_transaction());
        }

        D_TESTS_POSTGRES_FACE_CHECK(work.commit() == 0);
    }

    D_TESTS_POSTGRES_FACE_CHECK(
        (_db.query_int64(count, "SELECT sum(v) FROM g") == 0) &&
        (count == 1 + 4));

    return;
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

/*
d_tests_postgres_face_variadic
  The f forms, from C++11, in the layer's conversions: values quoted into
SQL, and read back through each kind of result; and a format built at run
time, through face::runtime_format.
*/
void
d_tests_postgres_face_variadic(
    face::postgres&    _db,
    const std::string& _schema
)
{
    const std::string built("SELECT count(*) FROM f");
    re_std::int64_t   count   = 0;
    re_std::int64_t   changed = 0;
    face::text        value;
    std::string       plain;

    std::printf("f forms\n");

    D_TESTS_POSTGRES_FACE_CHECK(
        _db.execf("CREATE TABLE %I (n integer, t text)", "f") == 0);
    D_TESTS_POSTGRES_FACE_CHECK(
        (_db.execf_changes(changed,
                           "INSERT INTO %T VALUES (%d, %Q), (%lld, %Q)",
                           _schema.c_str(), "f", 1, "it's",
                           static_cast<long long>(2), nullptr) == 0) &&
        (changed == 2));
    D_TESTS_POSTGRES_FACE_CHECK(
        (_db.query_int64f(count, "SELECT sum(n) FROM %I", "f") == 0) &&
        (count == 3));
    D_TESTS_POSTGRES_FACE_CHECK(
        (_db.query_textf(value, "SELECT t FROM f WHERE n = %d", 1) == 0) &&
        (d_tests_postgres_face_same(value.c_str(), "it's")));
    D_TESTS_POSTGRES_FACE_CHECK(
        (_db.query_textf(plain, "SELECT t FROM f WHERE n = %u", 2u) == 0) &&
        (plain.empty()));

    D_TESTS_POSTGRES_FACE_CHECK(
        (_db.query_int64f(count, face::runtime_format(built)) == 0) &&
        (count == 2));
    D_TESTS_POSTGRES_FACE_CHECK(
        (_db.query_textf(value, face::runtime_format("SELECT %Q"), "rt") ==
         0) &&
        (d_tests_postgres_face_same(value.c_str(), "rt")));

#if defined(D_TESTS_POSTGRES_FACE_MISUSE)
    // must not compile, each refused by the face, in its own words where it
    // has them (ci/check_db_postgres.sh runs each from the level that
    // refuses it, and expects those words):
    //   1  a std::string through C's `...` (C++11): "an f form takes
    //      numbers"
    //   2  a literal format its argument does not match (C++20):
    //      "the_format_argument_does_not_match_its_conversion"
    //   3  a std::string as a format (C++11), which goes through
    //      runtime_format: no form takes it
    //   4  a guard made and dropped at once, which begins and ends at once
    //      (C++17): its warning, made an error
    #if (D_TESTS_POSTGRES_FACE_MISUSE == 1)
        D_TESTS_POSTGRES_FACE_CHECK(_db.execf("SELECT %Q", _schema) == 0);
    #elif (D_TESTS_POSTGRES_FACE_MISUSE == 2)
        D_TESTS_POSTGRES_FACE_CHECK(_db.execf("SELECT %d", "two") == 0);
    #elif (D_TESTS_POSTGRES_FACE_MISUSE == 3)
        D_TESTS_POSTGRES_FACE_CHECK(_db.execf(built) == 0);
    #elif (D_TESTS_POSTGRES_FACE_MISUSE == 4)
        face::transaction<face::postgres>{_db};
    #endif
#endif  // defined(D_TESTS_POSTGRES_FACE_MISUSE)

    return;
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

/*
d_tests_postgres_face_session
  A schema of the run's own, made, worked in through the search path, and
dropped with what it holds.
*/
void
d_tests_postgres_face_session(
    const struct d_postgres_options& _where
)
{
    face::postgres db;

    D_TESTS_POSTGRES_FACE_CHECK(db.connect(&_where) == 0);

    const std::string schema = "d_tests_postgres_face_" +
                               d_tests_postgres_face_number(
                                   static_cast<unsigned long>(
                                       db.backend_pid()));

    D_TESTS_POSTGRES_FACE_CHECK(db.create_schema(schema, 0u) == 0);
    D_TESTS_POSTGRES_FACE_CHECK(db.set_search_path(schema) == 0);
    d_tests_postgres_face_conversions(db, schema);
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    d_tests_postgres_face_cpp17(db, schema);
#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    d_tests_postgres_face_cpp20(db, schema);
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER
#if D_INTERNAL_DB_FACE_EXPECTED
    d_tests_postgres_face_cpp23(db, schema);
#endif  // D_INTERNAL_DB_FACE_EXPECTED
    d_tests_postgres_face_statements(db);
    d_tests_postgres_face_modules(db, schema);
    d_tests_postgres_face_guards(db);
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    d_tests_postgres_face_variadic(db, schema);
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER
    D_TESTS_POSTGRES_FACE_CHECK(
        db.drop_schema(schema, D_POSTGRES_CASCADE) == 0);

    return;
}

/*
d_tests_postgres_face_stubs
  The face over the stubs: nothing connects, nothing is listed, no guard
begins, no text, statement or rows come, in any level's forms.
*/
void
d_tests_postgres_face_stubs(
    const struct d_postgres_options& _where
)
{
    face::postgres           db;
    std::vector<std::string> names(3u, "kept?");
    face::text               value;
    face::postgres_statement statement;
    face::rows               found;

    std::printf("over the stubs\n");

    D_TESTS_POSTGRES_FACE_CHECK(db.connect(&_where) == D_DB_UNSUPPORTED);
    D_TESTS_POSTGRES_FACE_CHECK(db.connect_string("dbname=x", value) ==
                                D_DB_UNSUPPORTED);
    {
        const char* const keywords[1] = { "application_name" };
        const char* const values[1]   = { "d_tests" };

        D_TESTS_POSTGRES_FACE_CHECK(db.connect_with("dbname=x",
                                                    keywords,
                                                    values,
                                                    value) ==
                                    D_DB_UNSUPPORTED);
    }
    D_TESTS_POSTGRES_FACE_CHECK( (!db.is_open()) && (value.is_null()) );
    D_TESTS_POSTGRES_FACE_CHECK(db.list_databases(names) ==
                                D_DB_UNSUPPORTED);
    D_TESTS_POSTGRES_FACE_CHECK(names.empty());
    D_TESTS_POSTGRES_FACE_CHECK(db.prepare(statement, "SELECT 1") ==
                                D_DB_UNSUPPORTED);
    D_TESTS_POSTGRES_FACE_CHECK(!statement.is_prepared());
    D_TESTS_POSTGRES_FACE_CHECK(db.query_rows(found, "SELECT 1") ==
                                D_DB_UNSUPPORTED);
    D_TESTS_POSTGRES_FACE_CHECK(found.empty());
    D_TESTS_POSTGRES_FACE_CHECK(
        db.query_rows_params(found, "SELECT 1", D_NULLPTR, 0u) ==
        D_DB_UNSUPPORTED);
    D_TESTS_POSTGRES_FACE_CHECK(db.send_query("SELECT 1") ==
                                D_DB_UNSUPPORTED);
    D_TESTS_POSTGRES_FACE_CHECK(db.parameter_status("TimeZone") ==
                                D_NULLPTR);

#if D_INTERNAL_DB_FACE_OPTIONAL
    std::optional<std::string> maybe(std::string("kept?"));

    D_TESTS_POSTGRES_FACE_CHECK(
        (db.query_text(maybe, "SELECT 1") == D_DB_UNSUPPORTED) &&
        (!maybe.has_value()));
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

#if D_INTERNAL_DB_FACE_EXPECTED
    const auto none = db.list_tables(0);

    D_TESTS_POSTGRES_FACE_CHECK( (!none.has_value()) &&
                                 (none.error() == D_DB_UNSUPPORTED) );
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    {
        face::transaction<face::postgres> work(db);

        D_TESTS_POSTGRES_FACE_CHECK( (work.result() == D_DB_UNSUPPORTED) &&
                                     (!work.is_active()) );
    }

    D_TESTS_POSTGRES_FACE_CHECK(db.close() == D_DB_OK);

    return;
}

}  // namespace

/*
main
  The static members; then, over libpq, a session where a server answers,
or, over the stubs -- whose libpq version is 0 -- what the stubs report.
*/
int
main(
    void
)
{
    const bool stubs    = (face::postgres::client_version_number() == 0);
    const bool required = (std::strcmp(
        d_tests_postgres_face_setting("DJINTERP_TEST_POSTGRES_REQUIRED", "0"),
        "1") == 0);
    const struct d_postgres_options where =
    {
        d_tests_postgres_face_setting("DJINTERP_TEST_POSTGRES_HOST",
                                      "127.0.0.1"),
        d_tests_postgres_face_setting("DJINTERP_TEST_POSTGRES_USER",
                                      "postgres"),
        d_tests_postgres_face_setting("DJINTERP_TEST_POSTGRES_PASSWORD", 0),
        d_tests_postgres_face_setting("DJINTERP_TEST_POSTGRES_DATABASE",
                                      "postgres"),
        static_cast<unsigned>(std::strtoul(
            d_tests_postgres_face_setting("DJINTERP_TEST_POSTGRES_PORT",
                                          "5432"),
            0,
            10)),
        0
    };

    d_tests_postgres_face_static(stubs);

    // the stubs, or a session where a server answers
    if (stubs)
    {
        d_tests_postgres_face_stubs(where);
    }
    else
    {
        face::postgres probe;

        // a server to reach, or the static checks alone
        if (probe.connect(&where) == 0)
        {
            probe.close();
            d_tests_postgres_face_ownership(where);
            d_tests_postgres_face_session(where);
        }
        else
        {
            std::printf("  no server reached; the static checks only\n");
            D_TESTS_POSTGRES_FACE_CHECK(!required);
        }
    }

    std::printf("%d checks, %d failed (%s %d, as C++ %ld; forms:%s)\n",
                g_checks,
                g_failures,
                (stubs) ? "built without libpq:" : "libpq",
                face::postgres::client_version_number(),
                static_cast<long>(__cplusplus),
                (sizeof(D_TESTS_POSTGRES_FACE_FORMS) > 1u)
                    ? D_TESTS_POSTGRES_FACE_FORMS
                    : " C++11's and below");

    return (g_failures == 0) ? 0 : 1;
}

#else

/*
main
  Below the 64-bit floor the face is absent, and there is nothing to test.
*/
int
main(
    void
)
{
    std::printf("the PostgreSQL face is absent below the 64-bit floor\n");

    return 0;
}

#endif  // defined(INT64_MAX)
