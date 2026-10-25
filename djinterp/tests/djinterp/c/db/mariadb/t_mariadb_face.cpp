/*******************************************************************************
* djinterp [c]                                                t_mariadb_face.cpp
*
*   The C++ face of the MariaDB and MySQL layer, mariadb_face.hpp, compiled as
* C++ at every level from ISO strict C++98 to C++26 and driven against a
* server. The layer's harness, t_mariadb.c, tests what each function does;
* this tests what the face adds -- ownership, adopting a client connection,
* string and list and text conversion, statements and rows, the guards, the
* static members, from C++11 moves and the f forms, and the forms each later
* level adds where the compiler has them: std::optional text and deduced
* guards (C++17), face::connection, std::span columns, UTF-8 strings and
* formats checked at compile time (C++20), std::expected results (C++23) --
* and samples every module, each member being generated from its C
* prototype, its arguments in the C order. The summary names the level forms
* the build had.
*   The server is the one t_mariadb.c reaches, by the same settings
* (DJINTERP_TEST_MARIADB_HOST, _PORT, _USER, _PASSWORD, _SOCKET); the user
* needs every privilege on databases named d_tests_mariadb_%. Without a
* server only the static checks run, unless DJINTERP_TEST_MARIADB_REQUIRED
* is 1. Over a layer built without a client it checks that the face reports
* what the stubs do. Below the 64-bit floor the face compiles to nothing, and
* so does this test. Defining D_TESTS_MARIADB_FACE_MISUSE as 1 to 4 makes it
* misuse the face in a way that must not compile (see
* d_tests_mariadb_face_variadic).
*
*   Build: compile the layer's C sources as C, this file as C++, and link them
* with the client -- ci/check_db_mariadb.sh does, at every level.
*
*
* path:      /tests/djinterp/c/db/mariadb/t_mariadb_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.14
*                                                            revised: 2026.10.23
*******************************************************************************/
// std
#include <cstdio>   // std::printf
#include <cstdlib>  // std::getenv, std::strtoul
#include <cstring>  // std::strcmp
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "../../../../../inc/djinterp/c/db/mariadb/mariadb_face.hpp"  // under
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
constinit face::mariadb g_unopened;
#endif  // C++20 constinit

/*
d_tests_mariadb_face_expect
  Counts one check, and prints it when it fails.
*/
void
d_tests_mariadb_face_expect(
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
d_tests_mariadb_face_setting
  An environment variable, or a default when it is unset or empty.
*/
const char*
d_tests_mariadb_face_setting(
    const char* _name,
    const char* _default
)
{
    const char* const value = std::getenv(_name);

    return ( (value) && (value[0] != '\0') ) ? value : _default;
}

/*
d_tests_mariadb_face_same
  Whether a C string is the text expected; NULL is no text.
*/
bool
d_tests_mariadb_face_same(
    const char* _text,
    const char* _expected
)
{
    return ( (_text) &&
             (std::strcmp(_text, _expected) == 0) );
}

/*
d_tests_mariadb_face_number
  A number's digits, by hand: snprintf is not C++98's, and sprintf is
deprecated on Apple's platforms.
*/
std::string
d_tests_mariadb_face_number(
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

// D_TESTS_MARIADB_FACE_CHECK
//   macro: records one check with its source text and line, which only the
// preprocessor can supply.
#define D_TESTS_MARIADB_FACE_CHECK(_cond)                                     \
    d_tests_mariadb_face_expect((_cond), #_cond, __LINE__)

// D_TESTS_MARIADB_FACE_FORMS
//   constant: the forms of the levels above C++11 this build has, for the
// summary; empty when it has none.
#define D_TESTS_MARIADB_FACE_FORMS                                            \
    D_TESTS_MARIADB_FACE_FORM_OPTIONAL D_TESTS_MARIADB_FACE_FORM_CONCEPTS     \
    D_TESTS_MARIADB_FACE_FORM_SPAN D_TESTS_MARIADB_FACE_FORM_CHAR8            \
    D_TESTS_MARIADB_FACE_FORM_CHECK D_TESTS_MARIADB_FACE_FORM_EXPECTED        \
    D_TESTS_MARIADB_FACE_FORM_DELETED

#if D_INTERNAL_DB_FACE_OPTIONAL
    #define D_TESTS_MARIADB_FACE_FORM_OPTIONAL " optional"
#else
    #define D_TESTS_MARIADB_FACE_FORM_OPTIONAL ""
#endif
#if D_INTERNAL_DB_FACE_CONCEPTS
    #define D_TESTS_MARIADB_FACE_FORM_CONCEPTS " concepts"
#else
    #define D_TESTS_MARIADB_FACE_FORM_CONCEPTS ""
#endif
#if D_INTERNAL_DB_FACE_SPAN
    #define D_TESTS_MARIADB_FACE_FORM_SPAN " span"
#else
    #define D_TESTS_MARIADB_FACE_FORM_SPAN ""
#endif
#if D_INTERNAL_DB_FACE_CHAR8
    #define D_TESTS_MARIADB_FACE_FORM_CHAR8 " char8_t"
#else
    #define D_TESTS_MARIADB_FACE_FORM_CHAR8 ""
#endif
#if D_INTERNAL_DB_FACE_FORMAT_CHECK
    #define D_TESTS_MARIADB_FACE_FORM_CHECK " checked-formats"
#else
    #define D_TESTS_MARIADB_FACE_FORM_CHECK ""
#endif
#if D_INTERNAL_DB_FACE_EXPECTED
    #define D_TESTS_MARIADB_FACE_FORM_EXPECTED " expected"
#else
    #define D_TESTS_MARIADB_FACE_FORM_EXPECTED ""
#endif
#if ( (D_ENV_LANG_IS_CPP26_OR_HIGHER) &&                                      \
      (D_ENV_CPP_FEATURE_LANG_DELETED_FUNCTION) )
    #define D_TESTS_MARIADB_FACE_FORM_DELETED " deleted-reasons"
#else
    #define D_TESTS_MARIADB_FACE_FORM_DELETED ""
#endif

namespace
{

/*
d_tests_mariadb_face_static
  The members that need no connection, in both forms where a member
returns text.
*/
void
d_tests_mariadb_face_static(
    bool _stubs
)
{
    face::text quoted;

    std::printf("static members\n");

    D_TESTS_MARIADB_FACE_CHECK(
        (std::strcmp(face::mariadb::client_version(), "") == 0) == _stubs);
    D_TESTS_MARIADB_FACE_CHECK(
        (face::mariadb::client_version_number() == 0ul) == _stubs);
    D_TESTS_MARIADB_FACE_CHECK(
        face::mariadb::quote_identifier("a`b") ==
        std::string((_stubs) ? "" : "`a``b`"));
    D_TESTS_MARIADB_FACE_CHECK(face::mariadb::quote_identifier(0).empty());

    // the exact form: the layer's text, owned by the caller's text, which is
    // null where the layer gave none
    face::mariadb::quote_identifier("a`b", quoted);
    D_TESTS_MARIADB_FACE_CHECK(
        (_stubs) ? (quoted.is_null())
                 : (d_tests_mariadb_face_same(quoted.c_str(), "`a``b`")));
    face::mariadb::quote_identifier(0, quoted);
    D_TESTS_MARIADB_FACE_CHECK(quoted.is_null());

    D_TESTS_MARIADB_FACE_CHECK(
        face::mariadb::result_message(D_DB_NOT_FOUND) != D_NULLPTR);

    return;
}

/*
d_tests_mariadb_face_ownership
  Connecting, failing to connect, swapping, giving up and replacing a
handle; adopting a client connection, not owned; from C++11, moving; from
C++20, a face constant-initialized.
*/
void
d_tests_mariadb_face_ownership(
    const struct d_mariadb_options& _where
)
{
    struct d_mariadb_options nowhere = _where;
    face::mariadb            db;
    face::mariadb            other;
    face::mariadb            borrowed;

    std::printf("ownership\n");

#if ( (D_ENV_LANG_IS_CPP20_OR_HIGHER) &&                                      \
      (D_ENV_CPP_FEATURE_LANG_CONSTINIT) )
    D_TESTS_MARIADB_FACE_CHECK(!g_unopened.is_open());
#endif  // C++20 constinit

    nowhere.port        = 1u;
    nowhere.unix_socket = D_NULLPTR;
    nowhere.host        = "127.0.0.1";
    D_TESTS_MARIADB_FACE_CHECK(!db.is_open());
    D_TESTS_MARIADB_FACE_CHECK(db.connect(&_where) == 0);
    D_TESTS_MARIADB_FACE_CHECK(db.is_open());

    struct d_mariadb* const first = db.handle();

    // a failed connect leaves the handle as it was
    D_TESTS_MARIADB_FACE_CHECK(db.connect(&nowhere) != 0);
    D_TESTS_MARIADB_FACE_CHECK(db.handle() == first);
    D_TESTS_MARIADB_FACE_CHECK(db.ping() == 0);

    // settings, as a pointer and a count, and as an array
    {
        const d_mariadb_setting settings[] =
        {
            { D_MARIADB_SETTING_INIT_COMMAND, 0ul, "SET @d_face = 3",
              D_NULLPTR },
            { D_MARIADB_SETTING_READ_TIMEOUT, 30ul, D_NULLPTR, D_NULLPTR }
        };
        const d_mariadb_setting wrong = { D_MARIADB_SETTING_INIT_COMMAND, 0ul,
                                          D_NULLPTR, D_NULLPTR };
        face::mariadb           set;
        re_std::int64_t         value = 0;

        D_TESTS_MARIADB_FACE_CHECK(set.connect_with(&_where, settings) == 0);
        D_TESTS_MARIADB_FACE_CHECK(
            (set.query_int64(value, "SELECT @d_face") == 0) && (value == 3));
        D_TESTS_MARIADB_FACE_CHECK(
            set.connect_with(&_where, &wrong, 1u) == D_DB_INVALID_ARGUMENT);
        D_TESTS_MARIADB_FACE_CHECK(set.is_open());

        // a reason, where the connect fails, and none where it does not
        face::text reason;

        D_TESTS_MARIADB_FACE_CHECK(
            set.connect_with(&nowhere, settings, reason) != 0);
        D_TESTS_MARIADB_FACE_CHECK(!reason.is_null());
        D_TESTS_MARIADB_FACE_CHECK(
            set.connect_with(&_where, settings, 2u, reason) == 0);
        D_TESTS_MARIADB_FACE_CHECK(reason.is_null());
        D_TESTS_MARIADB_FACE_CHECK(set.connect_with(&_where, settings, 1u) ==
                                   0);

        std::string status;

        D_TESTS_MARIADB_FACE_CHECK( (set.server_status(status) == 0) &&
                                    (status.find("Uptime: ") == 0u) );
        D_TESTS_MARIADB_FACE_CHECK(
            set.change_user(_where.user, _where.password, D_NULLPTR) == 0);
        D_TESTS_MARIADB_FACE_CHECK(
            set.set_next_isolation_level(
                D_MARIADB_ISOLATION_READ_COMMITTED) == 0);
    }
    other.swap(db);
    D_TESTS_MARIADB_FACE_CHECK( (!db.is_open()) &&
                                (other.handle() == first) );
    swap(db, other);
    D_TESTS_MARIADB_FACE_CHECK(db.handle() == first);

    // a client connection, wrapped and not owned: closing the wrapper
    // leaves it open
    D_TESTS_MARIADB_FACE_CHECK(borrowed.adopt(db.native(), false) == 0);
    D_TESTS_MARIADB_FACE_CHECK(borrowed.connection_id() ==
                               db.connection_id());
    D_TESTS_MARIADB_FACE_CHECK(borrowed.close() == 0);
    D_TESTS_MARIADB_FACE_CHECK(db.ping() == 0);

    struct d_mariadb* const raw = db.release();

    D_TESTS_MARIADB_FACE_CHECK( (raw == first) && (!db.is_open()) );

    face::mariadb adopted(raw);

    D_TESTS_MARIADB_FACE_CHECK(adopted.exec("SELECT 1") == 0);

#if D_MOVE_ENABLED
    face::mariadb moved(static_cast<face::mariadb&&>(adopted));

    D_TESTS_MARIADB_FACE_CHECK( (!adopted.is_open()) &&
                                (moved.handle() == raw) );
    D_TESTS_MARIADB_FACE_CHECK(static_cast<bool>(moved));
    adopted = static_cast<face::mariadb&&>(moved);
    D_TESTS_MARIADB_FACE_CHECK(adopted.handle() == raw);
#endif  // D_MOVE_ENABLED

    adopted.reset();
    D_TESTS_MARIADB_FACE_CHECK(!adopted.is_open());
    D_TESTS_MARIADB_FACE_CHECK(adopted.exec("SELECT 1") ==
                               D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_FACE_CHECK(!adopted.has_table(0, "anything"));

    return;
}

/*
d_tests_mariadb_face_conversions
  string_arg from a C string, a std::string and NULL; a column array; a
name list and a vector; a column list; text, null and not, and a
std::string; the numbers and booleans a member writes.
*/
void
d_tests_mariadb_face_conversions(
    face::mariadb&     _db,
    const std::string& _schema
)
{
    static const struct d_db_column people[] =
    {
        { "id",   D_DB_TYPE_BIG_INTEGER,
          D_DB_COLUMN_PRIMARY_KEY | D_DB_COLUMN_AUTO_INCREMENT,
          0, 0u, 0u, 0, 0u },
        { "name", D_DB_TYPE_STRING, D_DB_COLUMN_NOT_NULL,
          "'?'", 40u, 0u, 0, 0u }
    };
    face::name_list          names;
    std::vector<std::string> copied;
    face::column_list        columns;
    face::text               value;
    std::string              plain("unchanged");
    re_std::int64_t          count  = -1;
    bool                     exists = false;

    std::printf("conversions\n");

    D_TESTS_MARIADB_FACE_CHECK(_db.create_table(_schema, "people", people,
                                                0u) == 0);
    D_TESTS_MARIADB_FACE_CHECK(
        _db.create_table_sql(0, std::string("notes"),
                             "id INT PRIMARY KEY, body TEXT",
                             D_DB_IF_NOT_EXISTS) == 0);
    D_TESTS_MARIADB_FACE_CHECK(_db.has_table(0, "notes"));
    D_TESTS_MARIADB_FACE_CHECK(
        (_db.has_table_s(_schema, "people", exists) == 0) && (exists));
    D_TESTS_MARIADB_FACE_CHECK(_db.list_tables(0, names) == 0);
    D_TESTS_MARIADB_FACE_CHECK( (names.size() == 2u)                &&
                                (d_tests_mariadb_face_same(names[0],
                                                           "notes")) &&
                                (names.contains("people")) );
    D_TESTS_MARIADB_FACE_CHECK(_db.list_tables(_schema, copied) == 0);
    D_TESTS_MARIADB_FACE_CHECK( (copied.size() == 2u) &&
                                (copied[1] == "people") );
    D_TESTS_MARIADB_FACE_CHECK(_db.list_columns(0, "people", columns) == 0);
    D_TESTS_MARIADB_FACE_CHECK( (columns.size() == 2u)                  &&
                                (columns.find("id"))                    &&
                                (columns.find("id")->primary_key == 1)  &&
                                (columns[1].not_null) );
    D_TESTS_MARIADB_FACE_CHECK(
        _db.exec("INSERT INTO notes VALUES (1, NULL), (2, 'x')") == 0);
    D_TESTS_MARIADB_FACE_CHECK(
        (_db.query_text(value, "SELECT body FROM notes WHERE id = 1") == 0) &&
        (value.is_null()));
    D_TESTS_MARIADB_FACE_CHECK(
        (_db.query_text(plain, "SELECT body FROM notes WHERE id = 2") == 0) &&
        (plain == "x"));
    D_TESTS_MARIADB_FACE_CHECK(
        (_db.query_int64(count, "SELECT COUNT(*) FROM notes") == 0) &&
        (count == 2));
    D_TESTS_MARIADB_FACE_CHECK(
        (_db.count_rows(0, "people", count) == 0) && (count == 0));
    D_TESTS_MARIADB_FACE_CHECK(
        _db.quote_literal("it's") == std::string("'it\\'s'"));
    _db.quote_literal("it's", value);
    D_TESTS_MARIADB_FACE_CHECK(
        d_tests_mariadb_face_same(value.c_str(), "'it\\'s'"));

    return;
}

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

/*
d_tests_mariadb_face_cpp17
  C++17's forms: text as a std::optional<std::string>, empty for SQL NULL
and for a failure; and guards that deduce their face. Over the notes
d_tests_mariadb_face_conversions made: 1 NULL, 2 'x'.
*/
void
d_tests_mariadb_face_cpp17(
    face::mariadb&     _db,
    const std::string& _schema
)
{
    std::printf("C++17 forms\n");

#if D_INTERNAL_DB_FACE_OPTIONAL
    std::optional<std::string> body(std::string("stale"));
    std::optional<std::string> database;

    D_TESTS_MARIADB_FACE_CHECK(
        _db.query_text(body, "SELECT body FROM notes WHERE id = 1") == 0);
    D_TESTS_MARIADB_FACE_CHECK(!body.has_value());
    D_TESTS_MARIADB_FACE_CHECK(
        _db.query_text(body, "SELECT body FROM notes WHERE id = 2") == 0);
    D_TESTS_MARIADB_FACE_CHECK( (body.has_value()) && (*body == "x") );
    D_TESTS_MARIADB_FACE_CHECK(
        _db.query_text(body, "SELECT body FROM notes WHERE id = 9") ==
        D_DB_NOT_FOUND);
    D_TESTS_MARIADB_FACE_CHECK(!body.has_value());
    D_TESTS_MARIADB_FACE_CHECK(
        (_db.query_textf(body, "SELECT body FROM notes WHERE id = %d", 2) ==
         0) &&
        (body == std::string("x")));
    D_TESTS_MARIADB_FACE_CHECK(
        (_db.current_database(database) == 0) && (database == _schema));
#else
    (void)_schema;
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

#if D_ENV_CPP_FEATURE_LANG_DEDUCTION_GUIDES
    {
        face::transaction work(_db);

        D_TESTS_MARIADB_FACE_CHECK(work.is_active());

        {
            face::savepoint step(_db, "step");

            D_TESTS_MARIADB_FACE_CHECK( (step.is_active()) &&
                                        (step.release() == 0) );
        }

        D_TESTS_MARIADB_FACE_CHECK(work.rollback() == 0);
    }

    D_TESTS_MARIADB_FACE_CHECK(!_db.in_transaction());
#endif  // D_ENV_CPP_FEATURE_LANG_DEDUCTION_GUIDES

    return;
}

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#if D_INTERNAL_DB_FACE_CONCEPTS

/*
d_tests_mariadb_face_generic
  Code written for any face, through face::connection: a transaction begun
and rolled back, its guard deduced.
*/
template<face::connection Database>
bool
d_tests_mariadb_face_generic(
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
d_tests_mariadb_face_cpp20
  C++20's forms: the face as a face::connection; a column array as a
std::span, or anything that makes one; UTF-8 strings; and a format read at
compile time.
*/
void
d_tests_mariadb_face_cpp20(
    face::mariadb&     _db,
    const std::string& _schema
)
{
    static const struct d_db_column columns[] =
    {
        { "id", D_DB_TYPE_BIG_INTEGER, D_DB_COLUMN_PRIMARY_KEY,
          0, 0u, 0u, 0, 0u }
    };
    re_std::int64_t count = -1;

    std::printf("C++20 forms\n");

#if D_INTERNAL_DB_FACE_CONCEPTS
    static_assert(face::connection<face::mariadb>,
                  "face::mariadb is a face::connection");
    D_TESTS_MARIADB_FACE_CHECK(d_tests_mariadb_face_generic(_db));
#endif  // D_INTERNAL_DB_FACE_CONCEPTS

#if D_INTERNAL_DB_FACE_SPAN
    const std::vector<struct d_db_column> held(columns, columns + 1);

    D_TESTS_MARIADB_FACE_CHECK(
        _db.create_table(0, "spanned", std::span<const d_db_column>(columns),
                         0u) == 0);
    D_TESTS_MARIADB_FACE_CHECK(_db.create_table(0, "held", held, 0u) == 0);
#else
    D_TESTS_MARIADB_FACE_CHECK(
        _db.create_table(0, "spanned", columns, 0u) == 0);
    D_TESTS_MARIADB_FACE_CHECK(_db.create_table(0, "held", columns, 0u) ==
                               0);
#endif  // D_INTERNAL_DB_FACE_SPAN

#if D_INTERNAL_DB_FACE_CHAR8
    const std::u8string name(u8"held");

    D_TESTS_MARIADB_FACE_CHECK(_db.has_table(0, u8"spanned"));
    D_TESTS_MARIADB_FACE_CHECK(_db.has_table(0, name));
#endif  // D_INTERNAL_DB_FACE_CHAR8

#if D_INTERNAL_DB_FACE_FORMAT_CHECK
    // a format checked at compile time is a pointer to the literal
    constexpr face::mariadb::format<const char*, const char*> both(
        "SELECT COUNT(*) FROM %T");

    static_assert(sizeof(both) == sizeof(const char*),
                  "a checked format is its pointer");
    D_TESTS_MARIADB_FACE_CHECK(
        d_tests_mariadb_face_same(both.c_str(), "SELECT COUNT(*) FROM %T"));
    D_TESTS_MARIADB_FACE_CHECK(
        (_db.query_int64f(count, both, _schema.c_str(), "spanned") == 0) &&
        (count == 0));
#else
    (void)_schema;
#endif  // D_INTERNAL_DB_FACE_FORMAT_CHECK

    // the same calls compile at every level from C++11; from C++20 each
    // literal is read against its arguments first
    D_TESTS_MARIADB_FACE_CHECK(
        (_db.query_int64f(count, "SELECT COUNT(*) FROM %I", "held") == 0) &&
        (count == 0));
    D_TESTS_MARIADB_FACE_CHECK(
        (_db.query_int64f(count, "SELECT %lld + %d",
                          static_cast<long long>(40), 2) == 0) &&
        (count == 42));

    return;
}

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#if D_INTERNAL_DB_FACE_EXPECTED

/*
d_tests_mariadb_face_cpp23
  C++23's forms: a result written through a parameter, returned instead,
as a std::expected that holds the value or the layer's result. Over the
notes and people d_tests_mariadb_face_conversions made.
*/
void
d_tests_mariadb_face_cpp23(
    face::mariadb&     _db,
    const std::string& _schema
)
{
    std::printf("C++23 forms\n");

    const auto count      = _db.query_int64("SELECT COUNT(*) FROM notes");
    const auto missing    = _db.query_text(
                                "SELECT body FROM notes WHERE id = 9");
    const auto null_body  = _db.query_text(
                                "SELECT body FROM notes WHERE id = 1");
    const auto body       = _db.query_textf(
                                "SELECT body FROM notes WHERE id = %d", 2);
    const auto tables     = _db.list_tables(0);
    const auto people     = _db.list_columns(0, "people");
    const auto exists     = _db.has_table_s(0, "people");
    const auto database   = _db.current_database();
    const auto autocommit = _db.get_autocommit();
    const auto nothing    = face::mariadb().query_int64("SELECT 1");

    D_TESTS_MARIADB_FACE_CHECK( (count.has_value()) && (*count == 2) );
    D_TESTS_MARIADB_FACE_CHECK( (!missing.has_value()) &&
                                (missing.error() == D_DB_NOT_FOUND) );
    D_TESTS_MARIADB_FACE_CHECK( (null_body.has_value()) &&
                                (null_body->is_null()) );
    D_TESTS_MARIADB_FACE_CHECK( (body.has_value()) &&
                                (d_tests_mariadb_face_same(body->c_str(),
                                                           "x")) );
    D_TESTS_MARIADB_FACE_CHECK( (tables.has_value()) &&
                                (tables->contains("notes")) );
    D_TESTS_MARIADB_FACE_CHECK( (people.has_value()) &&
                                (people->find("name")) );
    D_TESTS_MARIADB_FACE_CHECK( (exists.has_value()) && (*exists) );
    D_TESTS_MARIADB_FACE_CHECK(
        (database.has_value()) &&
        (d_tests_mariadb_face_same(database->c_str(), _schema.c_str())));
    D_TESTS_MARIADB_FACE_CHECK( (autocommit.has_value()) && (*autocommit) );
    D_TESTS_MARIADB_FACE_CHECK( (!nothing.has_value()) &&
                                (nothing.error() == D_DB_INVALID_ARGUMENT) );

    return;
}

#endif  // D_INTERNAL_DB_FACE_EXPECTED

/*
d_tests_mariadb_face_statements
  Rows and statements: a statement prepared into a face, its parameters
counted and bound -- text from a std::string too -- run for a count and for
rows; rows read by place and by name, their values converted; a statement
finalized by hand, by its destructor, and on a failed prepare, which leaves
it empty; from C++11 one moved and swapped, and the f form; from C++23 the
statement, the rows, a column's place and a conversion returned.
*/
void
d_tests_mariadb_face_statements(
    face::mariadb& _db
)
{
    face::mariadb_statement insert;
    face::mariadb_statement select;
    face::rows              found;
    re_std::int64_t         changes = -1;
    re_std::int64_t         number  = 0;
    double                  real    = 0.0;
    bool                    truth   = false;
    std::size_t             index   = 99u;

    std::printf("statements and rows\n");

    D_TESTS_MARIADB_FACE_CHECK(
        _db.exec("CREATE TABLE kept (id INTEGER, flag TINYINT(1), "
                 "ratio DOUBLE, body TEXT, raw BLOB)") == 0);
    D_TESTS_MARIADB_FACE_CHECK(!insert.is_prepared());
    D_TESTS_MARIADB_FACE_CHECK(
        _db.prepare(insert, "INSERT INTO kept VALUES (?, ?, ?, ?, ?)") == 0);
    D_TESTS_MARIADB_FACE_CHECK( (insert.is_prepared()) &&
                                (insert.handle() != 0) );
    D_TESTS_MARIADB_FACE_CHECK(insert.statement_connection() == _db.handle());
    D_TESTS_MARIADB_FACE_CHECK(insert.parameter_count() == 5u);
    D_TESTS_MARIADB_FACE_CHECK(insert.bind_int64(1u, 7) == 0);
    D_TESTS_MARIADB_FACE_CHECK(insert.bind_bool(2u, true) == 0);
    D_TESTS_MARIADB_FACE_CHECK(insert.bind_double(3u, 0.5) == 0);
    D_TESTS_MARIADB_FACE_CHECK(insert.bind_text(4u, std::string("it's")) ==
                               0);
    D_TESTS_MARIADB_FACE_CHECK(insert.bind_blob(5u, "\0\1", 2u) == 0);
    D_TESTS_MARIADB_FACE_CHECK(
        (insert.execute_update(changes) == 0) && (changes == 1));
    D_TESTS_MARIADB_FACE_CHECK(insert.bind_int64(6u, 1) == 2034);
    D_TESTS_MARIADB_FACE_CHECK(insert.clear_bindings() == 0);
    D_TESTS_MARIADB_FACE_CHECK(insert.bind_null(1u) == 0);
    D_TESTS_MARIADB_FACE_CHECK(insert.bind_text(4u, 0) == 0);
    D_TESTS_MARIADB_FACE_CHECK(insert.execute_query(found) == 0);
    D_TESTS_MARIADB_FACE_CHECK( (found.column_count() == 0u) &&
                                (found.empty()) );

    // rows, by place and by name, converted
    D_TESTS_MARIADB_FACE_CHECK(
        _db.prepare(select, "SELECT id, flag, ratio, body, raw FROM kept "
                            "WHERE id IS NOT NULL") == 0);
    D_TESTS_MARIADB_FACE_CHECK(select.execute_query(found) == 0);
    D_TESTS_MARIADB_FACE_CHECK( (found.row_count() == 1u)    &&
                                (found.column_count() == 5u) &&
                                (!found.empty()) );
    D_TESTS_MARIADB_FACE_CHECK(
        (d_tests_mariadb_face_same(found.column(3).name, "body")) &&
        (found.column(1).type == D_DB_TYPE_BOOLEAN) &&
        (d_tests_mariadb_face_same(found.column(1).type_name,
                                 "TINYINT")));
    D_TESTS_MARIADB_FACE_CHECK(
        (face::value_int64(found.value(0u, 0u), number) == 0) &&
        (number == 7));
    D_TESTS_MARIADB_FACE_CHECK(
        (face::value_bool(found.value(0u, 1u), truth) == 0) && (truth));
    D_TESTS_MARIADB_FACE_CHECK(
        (face::value_double(found.value(0u, 2u), real) == 0) &&
        (real == 0.5));
    D_TESTS_MARIADB_FACE_CHECK(
        face::value_int64(found.value(0u, 3u), number) == D_DB_MISMATCH);
    D_TESTS_MARIADB_FACE_CHECK(
        d_tests_mariadb_face_same(found.value(0u, 3u)->as.text, "it's"));
    D_TESTS_MARIADB_FACE_CHECK( (found.value(0u, 4u)->type ==
                                 D_DB_VALUE_BLOB) &&
                                (found.value(0u, 4u)->size == 2u) );
    D_TESTS_MARIADB_FACE_CHECK(found.value(1u, 0u) == 0);
    D_TESTS_MARIADB_FACE_CHECK(
        (found.column_index("RATIO", index) == 0) && (index == 2u));
    D_TESTS_MARIADB_FACE_CHECK(found.column_index("nobody", index) ==
                               D_DB_NOT_FOUND);
    found.clear();
    D_TESTS_MARIADB_FACE_CHECK( (found.row_count() == 0u) &&
                                (found.core().columns == 0) );

    // the connection's own counts and rows
    D_TESTS_MARIADB_FACE_CHECK(
        (_db.exec_changes(changes, "UPDATE kept SET flag = 0") == 0) &&
        (changes == 2));
    D_TESTS_MARIADB_FACE_CHECK(
        _db.query_rows(found, "SELECT count(*) AS n FROM kept") == 0);
    D_TESTS_MARIADB_FACE_CHECK( (found.row_count() == 1u) &&
                                (found.value(0u, 0u)->as.integer == 2) );

    // finalized by hand, and emptied by a prepare that fails
    D_TESTS_MARIADB_FACE_CHECK(select.finalize() == 0);
    D_TESTS_MARIADB_FACE_CHECK(!select.is_prepared());
    D_TESTS_MARIADB_FACE_CHECK(_db.prepare(select, "SELECT 1") == 0);
    D_TESTS_MARIADB_FACE_CHECK(_db.prepare(select, "SELEC 1") ==
                               1064);
    D_TESTS_MARIADB_FACE_CHECK(!select.is_prepared());

    // handed over, and taken back
    D_TESTS_MARIADB_FACE_CHECK(_db.prepare(select, "SELECT 2") == 0);
    {
        struct d_mariadb_statement* const raw = select.release();

        D_TESTS_MARIADB_FACE_CHECK( (raw != 0) && (!select.is_prepared()) );
        select.reset(raw);
        D_TESTS_MARIADB_FACE_CHECK(select.handle() == raw);
    }
    insert.swap(select);
    D_TESTS_MARIADB_FACE_CHECK( (insert.parameter_count() == 0u) &&
                                (select.parameter_count() == 5u) );
    swap(insert, select);
    D_TESTS_MARIADB_FACE_CHECK(insert.parameter_count() == 5u);

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    {
        face::mariadb_statement moved(
            static_cast<face::mariadb_statement&&>(insert));

        D_TESTS_MARIADB_FACE_CHECK( (moved) && (!insert) );
        insert = static_cast<face::mariadb_statement&&>(moved);
        D_TESTS_MARIADB_FACE_CHECK( (insert) && (!moved) );

        face::rows taken(static_cast<face::rows&&>(found));

        D_TESTS_MARIADB_FACE_CHECK( (taken.row_count() == 1u) &&
                                    (found.row_count() == 0u) );
    }
    D_TESTS_MARIADB_FACE_CHECK(
        _db.preparef(select, "SELECT id FROM %I WHERE body = %Q", "kept",
                     "it's") == 0);
    D_TESTS_MARIADB_FACE_CHECK(
        (select.execute_update(changes) == 0) && (changes == 1));
    D_TESTS_MARIADB_FACE_CHECK(
        _db.query_rowsf(found, "SELECT %d + %d", 2, 3) == 0);
    D_TESTS_MARIADB_FACE_CHECK(found.value(0u, 0u)->as.integer == 5);
    D_TESTS_MARIADB_FACE_CHECK(
        (_db.execf_changes(changes, "DELETE FROM kept WHERE id IS %s",
                           "NULL") == 0) &&
        (changes == 1));
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_INTERNAL_DB_FACE_EXPECTED
    {
        auto       prepared = _db.prepare("SELECT id, body FROM kept");
        const auto refused  = _db.prepare("SELEC");
        const auto all      = _db.query_rows("SELECT id FROM kept");

        D_TESTS_MARIADB_FACE_CHECK( (prepared.has_value()) &&
                                    (prepared->is_prepared()) );
        D_TESTS_MARIADB_FACE_CHECK( (!refused.has_value()) &&
                                    (refused.error() == 1064) );
        D_TESTS_MARIADB_FACE_CHECK( (all.has_value()) &&
                                    (all->row_count() == 1u) );

        // nothing more without a statement
        if (prepared)
        {
            const auto rows_of = prepared->execute_query();
            const auto count   = prepared->execute_update();

            D_TESTS_MARIADB_FACE_CHECK( (rows_of.has_value()) &&
                                        (rows_of->row_count() == 1u) );
            D_TESTS_MARIADB_FACE_CHECK( (count.has_value()) &&
                                        (*count == 1) );
            D_TESTS_MARIADB_FACE_CHECK(
                (rows_of) &&
                (rows_of->column_index("body") == std::size_t(1u)));
            D_TESTS_MARIADB_FACE_CHECK(
                (rows_of) &&
                (face::value_int64(rows_of->value(0u, 0u)) ==
                 re_std::int64_t(7)));
            D_TESTS_MARIADB_FACE_CHECK(
                (rows_of) &&
                (face::value_double(rows_of->value(0u, 1u)).error() ==
                 D_DB_MISMATCH));
        }
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    D_TESTS_MARIADB_FACE_CHECK(_db.exec("DROP TABLE kept") == 0);

    return;
}

/*
d_tests_mariadb_face_cursors
  Rows a few at a time through the statement face: pages no longer than asked
for, and a short one ending the run; the connection the run's until then, and a
fetch after the end refused; a run closed part-way; from C++23 a page returned.
*/
void
d_tests_mariadb_face_cursors(
    face::mariadb& _db
)
{
    face::mariadb_statement select;
    face::rows              page;

    std::printf("rows a few at a time\n");

    D_TESTS_MARIADB_FACE_CHECK(
        _db.prepare(select,
                    "SELECT 1 UNION ALL SELECT 2 "
                    "UNION ALL SELECT 3") == 0);
    D_TESTS_MARIADB_FACE_CHECK(select.open_cursor() == 0);
    D_TESTS_MARIADB_FACE_CHECK(select.fetch_rows(2u, page) == 0);
    D_TESTS_MARIADB_FACE_CHECK( (page.row_count() == 2u) &&
                                (page.column_count() == 1u) );
    D_TESTS_MARIADB_FACE_CHECK(_db.exec("SELECT 1") == 2014);
    D_TESTS_MARIADB_FACE_CHECK(select.fetch_rows(2u, page) == 0);
    D_TESTS_MARIADB_FACE_CHECK( (page.row_count() == 1u) &&
                                (page.value(0u, 0u)->as.integer == 3) );
    D_TESTS_MARIADB_FACE_CHECK(select.fetch_rows(2u, page) ==
                               D_DB_INVALID_ARGUMENT);
    D_TESTS_MARIADB_FACE_CHECK(_db.exec("SELECT 1") == 0);

    // closed part-way, and again
    D_TESTS_MARIADB_FACE_CHECK(select.open_cursor() == 0);
    D_TESTS_MARIADB_FACE_CHECK(select.fetch_rows(1u, page) == 0);
    D_TESTS_MARIADB_FACE_CHECK(select.close_cursor() == 0);
    D_TESTS_MARIADB_FACE_CHECK(_db.exec("SELECT 1") == 0);
    D_TESTS_MARIADB_FACE_CHECK(select.close_cursor() == 0);

#if D_INTERNAL_DB_FACE_EXPECTED
    // a page returned: three rows fill the first exactly, and the next is
    // empty, its columns still described
    D_TESTS_MARIADB_FACE_CHECK(select.open_cursor() == 0);
    {
        const auto full = select.fetch_rows(3u);
        const auto rest = select.fetch_rows(3u);

        D_TESTS_MARIADB_FACE_CHECK( (full.has_value()) &&
                                    (full->row_count() == 3u) );
        D_TESTS_MARIADB_FACE_CHECK( (rest.has_value())           &&
                                    (rest->row_count() == 0u)    &&
                                    (rest->column_count() == 1u) );
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    D_TESTS_MARIADB_FACE_CHECK(select.finalize() == 0);

    return;
}

/*
d_tests_mariadb_face_modules
  A member or two of each module beyond the ones above: servers and
errors, connections, databases, schema, settings -- the session's changes,
as a list and as strings, among them -- and maintenance.
*/
void
d_tests_mariadb_face_modules(
    face::mariadb&     _db,
    const std::string& _schema
)
{
    face::text  current;
    std::string charset;
    unsigned    seconds = 0u;
    bool        ok      = false;

    std::printf("modules\n");

    D_TESTS_MARIADB_FACE_CHECK(_db.server_flavor() !=
                               D_MARIADB_FLAVOR_UNKNOWN);
    D_TESTS_MARIADB_FACE_CHECK(_db.server_version_number() >= 50700ul);
    D_TESTS_MARIADB_FACE_CHECK(_db.exec("SELECT * FROM nope") != 0);
    D_TESTS_MARIADB_FACE_CHECK( (_db.error_code() == 1146u) &&
                                (_db.error_message()) &&
                                (d_tests_mariadb_face_same(_db.sqlstate(),
                                                           "42S02")) );
    D_TESTS_MARIADB_FACE_CHECK(
        (_db.current_database(current) == 0) &&
        (d_tests_mariadb_face_same(current.c_str(), _schema.c_str())));
    D_TESTS_MARIADB_FACE_CHECK(_db.has_database(_schema));
    D_TESTS_MARIADB_FACE_CHECK(
        _db.create_index(0, "notes_body", "notes", "body(10)", 0u) == 0);
    D_TESTS_MARIADB_FACE_CHECK(_db.has_index(0, "notes_body", "notes"));
    D_TESTS_MARIADB_FACE_CHECK(_db.set_lock_wait_timeout(7u) == 0);
    D_TESTS_MARIADB_FACE_CHECK(
        (_db.get_lock_wait_timeout(seconds) == 0) && (seconds == 7u));
    D_TESTS_MARIADB_FACE_CHECK(
        (_db.get_charset(charset) == 0) && (charset == "utf8mb4"));
    D_TESTS_MARIADB_FACE_CHECK(
        (_db.check_table(0, "notes", ok) == 0) && (ok));
    D_TESTS_MARIADB_FACE_CHECK(_db.analyze_table(0, "notes") == 0);

    // the session's changes, where the server and the client track them
    if (_db.supports(D_MARIADB_FEATURE_SESSION_TRACKING))
    {
        face::name_list          entries;
        std::vector<std::string> strings(1u, "kept?");

        D_TESTS_MARIADB_FACE_CHECK(
            _db.exec("SET SESSION time_zone = '+01:00'") == 0);
        D_TESTS_MARIADB_FACE_CHECK(
            (_db.session_track(D_MARIADB_TRACK_SYSTEM_VARIABLES,
                               entries) == 0) &&
            (entries.size() == 2u) &&
            (d_tests_mariadb_face_same(entries[1u], "+01:00")));
        D_TESTS_MARIADB_FACE_CHECK(
            (_db.session_track(D_MARIADB_TRACK_SCHEMA, strings) == 0) &&
            (strings.empty()));
    }
    else
    {
        std::vector<std::string> strings(1u, "kept?");

        D_TESTS_MARIADB_FACE_CHECK(
            _db.session_track(D_MARIADB_TRACK_SCHEMA, strings) ==
            D_DB_UNSUPPORTED);
    }

    return;
}

/*
d_tests_mariadb_face_guards
  The transaction guard: committed, and rolled back when its scope ends; the
savepoint guard: released, rolled back by hand, and rolled back when its
scope ends, named by a literal or by a string that outlives it.
*/
void
d_tests_mariadb_face_guards(
    face::mariadb& _db
)
{
    const std::string dropped_name("dropped");
    re_std::int64_t   count = 0;

    std::printf("guards\n");

    D_TESTS_MARIADB_FACE_CHECK(
        _db.exec("CREATE TABLE g (v INT) ENGINE=InnoDB") == 0);

    {
        face::transaction<face::mariadb> work(_db);

        D_TESTS_MARIADB_FACE_CHECK( (work.result() == 0) &&
                                    (_db.in_transaction()) );
        D_TESTS_MARIADB_FACE_CHECK(_db.exec("INSERT INTO g VALUES (1)") ==
                                   0);
        D_TESTS_MARIADB_FACE_CHECK(work.commit() == 0);
        D_TESTS_MARIADB_FACE_CHECK(!work.is_active());
    }

    {
        face::transaction<face::mariadb> work(
            _db, D_MARIADB_TRANSACTION_CONSISTENT_SNAPSHOT);

        D_TESTS_MARIADB_FACE_CHECK(_db.exec("INSERT INTO g VALUES (2)") ==
                                   0);
    }

    D_TESTS_MARIADB_FACE_CHECK(!_db.in_transaction());

    {
        face::transaction<face::mariadb> work(_db);

        {
            face::savepoint<face::mariadb> kept(_db, "kept");

            D_TESTS_MARIADB_FACE_CHECK(
                _db.exec("INSERT INTO g VALUES (4)") == 0);
            D_TESTS_MARIADB_FACE_CHECK(kept.release() == 0);
        }

        {
            face::savepoint<face::mariadb> dropped(_db, dropped_name);

            D_TESTS_MARIADB_FACE_CHECK(
                _db.exec("INSERT INTO g VALUES (8)") == 0);
        }

        {
            face::savepoint<face::mariadb> undone(_db, "undone");

            D_TESTS_MARIADB_FACE_CHECK(
                _db.exec("INSERT INTO g VALUES (16)") == 0);
            D_TESTS_MARIADB_FACE_CHECK(undone.rollback() == 0);
        }

        D_TESTS_MARIADB_FACE_CHECK(work.commit() == 0);
    }

    D_TESTS_MARIADB_FACE_CHECK(
        (_db.query_int64(count, "SELECT SUM(v) FROM g") == 0) &&
        (count == 1 + 4));

    return;
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

/*
d_tests_mariadb_face_variadic
  The f forms, from C++11, in the layer's conversions: values quoted into
SQL, and read back through each kind of result; and a format built at run
time, through face::runtime_format.
*/
void
d_tests_mariadb_face_variadic(
    face::mariadb&     _db,
    const std::string& _schema
)
{
    const std::string built("SELECT COUNT(*) FROM f");
    re_std::int64_t   count = 0;
    face::text        value;
    std::string       plain;

    std::printf("f forms\n");

    D_TESTS_MARIADB_FACE_CHECK(
        _db.execf("CREATE TABLE %I (n INT, t TEXT)", "f") == 0);
    D_TESTS_MARIADB_FACE_CHECK(
        _db.execf("INSERT INTO %T VALUES (%d, %Q), (%lld, %Q)",
                  _schema.c_str(), "f", 1, "it's",
                  static_cast<long long>(2), nullptr) == 0);
    D_TESTS_MARIADB_FACE_CHECK(
        (_db.query_int64f(count, "SELECT SUM(n) FROM %I", "f") == 0) &&
        (count == 3));
    D_TESTS_MARIADB_FACE_CHECK(
        (_db.query_textf(value, "SELECT t FROM f WHERE n = %d", 1) == 0) &&
        (d_tests_mariadb_face_same(value.c_str(), "it's")));
    D_TESTS_MARIADB_FACE_CHECK(
        (_db.query_textf(plain, "SELECT t FROM f WHERE n = %u", 2u) == 0) &&
        (plain.empty()));

    D_TESTS_MARIADB_FACE_CHECK(
        (_db.query_int64f(count, face::runtime_format(built)) == 0) &&
        (count == 2));
    D_TESTS_MARIADB_FACE_CHECK(
        (_db.query_textf(value, face::runtime_format("SELECT %Q"), "rt") ==
         0) &&
        (d_tests_mariadb_face_same(value.c_str(), "rt")));

#if defined(D_TESTS_MARIADB_FACE_MISUSE)
    // must not compile, each refused by the face, in its own words where it
    // has them (ci/check_db_mariadb.sh runs each from the level that refuses
    // it, and expects those words):
    //   1  a std::string through C's `...` (C++11): "an f form takes
    //      numbers"
    //   2  a literal format its argument does not match (C++20):
    //      "the_format_argument_does_not_match_its_conversion"
    //   3  a std::string as a format (C++11), which goes through
    //      runtime_format: no form takes it
    //   4  a guard made and dropped at once, which begins and ends at once
    //      (C++17): its warning, made an error
    #if (D_TESTS_MARIADB_FACE_MISUSE == 1)
        D_TESTS_MARIADB_FACE_CHECK(_db.execf("SELECT %Q", _schema) == 0);
    #elif (D_TESTS_MARIADB_FACE_MISUSE == 2)
        D_TESTS_MARIADB_FACE_CHECK(_db.execf("SELECT %d", "two") == 0);
    #elif (D_TESTS_MARIADB_FACE_MISUSE == 3)
        D_TESTS_MARIADB_FACE_CHECK(_db.execf(built) == 0);
    #elif (D_TESTS_MARIADB_FACE_MISUSE == 4)
        face::transaction<face::mariadb>{_db};
    #endif
#endif  // defined(D_TESTS_MARIADB_FACE_MISUSE)

    return;
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

/*
d_tests_mariadb_face_session
  A database of the run's own, made, worked in, and dropped.
*/
void
d_tests_mariadb_face_session(
    const struct d_mariadb_options& _where
)
{
    face::mariadb db;

    D_TESTS_MARIADB_FACE_CHECK(db.connect(&_where) == 0);

    const std::string schema = "d_tests_mariadb_face_" +
                               d_tests_mariadb_face_number(
                                   db.connection_id());

    D_TESTS_MARIADB_FACE_CHECK(
        db.create_database(schema, D_DB_IF_NOT_EXISTS) == 0);
    D_TESTS_MARIADB_FACE_CHECK(db.use_database(schema) == 0);
    d_tests_mariadb_face_conversions(db, schema);
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    d_tests_mariadb_face_cpp17(db, schema);
#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    d_tests_mariadb_face_cpp20(db, schema);
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER
#if D_INTERNAL_DB_FACE_EXPECTED
    d_tests_mariadb_face_cpp23(db, schema);
#endif  // D_INTERNAL_DB_FACE_EXPECTED
    d_tests_mariadb_face_statements(db);
    d_tests_mariadb_face_cursors(db);
    d_tests_mariadb_face_modules(db, schema);
    d_tests_mariadb_face_guards(db);
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    d_tests_mariadb_face_variadic(db, schema);
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER
    D_TESTS_MARIADB_FACE_CHECK(db.drop_database(schema, 0u) == 0);

    return;
}

/*
d_tests_mariadb_face_stubs
  The face over the stubs: nothing connects, nothing is listed, no guard
begins, no text, statement or rows come, in any level's forms.
*/
void
d_tests_mariadb_face_stubs(
    const struct d_mariadb_options& _where
)
{
    face::mariadb            db;
    std::vector<std::string> names(3u, "kept?");
    face::text               value;
    face::mariadb_statement  statement;
    face::rows               found;

    std::printf("over the stubs\n");

    D_TESTS_MARIADB_FACE_CHECK(db.connect(&_where) == D_DB_UNSUPPORTED);
    D_TESTS_MARIADB_FACE_CHECK(!db.is_open());
    D_TESTS_MARIADB_FACE_CHECK(db.connect_with(&_where, D_NULLPTR, 0u) ==
                               D_DB_UNSUPPORTED);
    D_TESTS_MARIADB_FACE_CHECK(!db.is_open());
    D_TESTS_MARIADB_FACE_CHECK(db.connect_with(&_where, D_NULLPTR, 0u, value) ==
                               D_DB_UNSUPPORTED);
    D_TESTS_MARIADB_FACE_CHECK(value.is_null());
    D_TESTS_MARIADB_FACE_CHECK(db.list_databases(names) == D_DB_UNSUPPORTED);
    D_TESTS_MARIADB_FACE_CHECK(names.empty());
    D_TESTS_MARIADB_FACE_CHECK(db.query_text(value, "SELECT 1") ==
                               D_DB_UNSUPPORTED);
    D_TESTS_MARIADB_FACE_CHECK(value.is_null());
    D_TESTS_MARIADB_FACE_CHECK(db.prepare(statement, "SELECT 1") ==
                               D_DB_UNSUPPORTED);
    D_TESTS_MARIADB_FACE_CHECK(!statement.is_prepared());
    D_TESTS_MARIADB_FACE_CHECK(statement.open_cursor() == D_DB_UNSUPPORTED);
    D_TESTS_MARIADB_FACE_CHECK(statement.fetch_rows(1u, found) ==
                               D_DB_UNSUPPORTED);
    D_TESTS_MARIADB_FACE_CHECK(statement.close_cursor() == D_DB_UNSUPPORTED);
    D_TESTS_MARIADB_FACE_CHECK(db.query_rows(found, "SELECT 1") ==
                               D_DB_UNSUPPORTED);
    D_TESTS_MARIADB_FACE_CHECK(found.empty());

#if D_INTERNAL_DB_FACE_OPTIONAL
    std::optional<std::string> maybe(std::string("kept?"));

    D_TESTS_MARIADB_FACE_CHECK(
        (db.query_text(maybe, "SELECT 1") == D_DB_UNSUPPORTED) &&
        (!maybe.has_value()));
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

#if D_INTERNAL_DB_FACE_EXPECTED
    const auto none = db.list_tables(0);

    D_TESTS_MARIADB_FACE_CHECK( (!none.has_value()) &&
                                (none.error() == D_DB_UNSUPPORTED) );
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    {
        face::transaction<face::mariadb> work(db);

        D_TESTS_MARIADB_FACE_CHECK( (work.result() == D_DB_UNSUPPORTED) &&
                                    (!work.is_active()) );
    }

    D_TESTS_MARIADB_FACE_CHECK(db.close() == D_DB_OK);

    return;
}

}  // namespace

/*
main
  The static members; then, over a client, a session where a server
answers, or, over the stubs, what the stubs report.
*/
int
main(
    void
)
{
    const bool stubs    = (face::mariadb::client_version_number() == 0ul);
    const bool required = (std::strcmp(
        d_tests_mariadb_face_setting("DJINTERP_TEST_MARIADB_REQUIRED", "0"),
        "1") == 0);
    const struct d_mariadb_options where =
    {
        d_tests_mariadb_face_setting("DJINTERP_TEST_MARIADB_HOST",
                                     "127.0.0.1"),
        d_tests_mariadb_face_setting("DJINTERP_TEST_MARIADB_USER", "root"),
        d_tests_mariadb_face_setting("DJINTERP_TEST_MARIADB_PASSWORD", 0),
        0,
        static_cast<unsigned>(std::strtoul(
            d_tests_mariadb_face_setting("DJINTERP_TEST_MARIADB_PORT",
                                         "3306"),
            0,
            10)),
        d_tests_mariadb_face_setting("DJINTERP_TEST_MARIADB_SOCKET", 0)
    };

    d_tests_mariadb_face_static(stubs);

    // the stubs, or a session where a server answers
    if (stubs)
    {
        d_tests_mariadb_face_stubs(where);
    }
    else
    {
        face::mariadb probe;

        // a server to reach, or the static checks alone
        if (probe.connect(&where) == 0)
        {
            probe.close();
            d_tests_mariadb_face_ownership(where);
            d_tests_mariadb_face_session(where);
        }
        else
        {
            std::printf("  no server reached; the static checks only\n");
            D_TESTS_MARIADB_FACE_CHECK(!required);
        }
    }

    std::printf("%d checks, %d failed (%s%s, as C++ %ld; forms:%s)\n",
                g_checks,
                g_failures,
                (stubs) ? "built without a client" : "client ",
                face::mariadb::client_version(),
                static_cast<long>(__cplusplus),
                (sizeof(D_TESTS_MARIADB_FACE_FORMS) > 1u)
                    ? D_TESTS_MARIADB_FACE_FORMS
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
    std::printf("the MariaDB face is absent below the 64-bit floor\n");

    return 0;
}

#endif  // defined(INT64_MAX)
