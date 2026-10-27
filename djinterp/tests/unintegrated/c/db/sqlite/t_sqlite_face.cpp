/*******************************************************************************
* djinterp [c]                                                 t_sqlite_face.cpp
*
*   The C++ face of the SQLite layer, sqlite_face.hpp, compiled as C++ at
* every level from ISO strict C++98 to C++26 and driven against in-memory
* databases and a file in the directory it is given. The layer's harness,
* t_sqlite.c, tests what each function does; this tests what the face adds
* -- ownership, string and list and text conversion, statements and rows,
* the guards, the static members, from C++11 moves and the f forms, and the
* forms each later level adds where the compiler has them: std::optional
* text and deduced guards (C++17), face::connection, std::span columns,
* UTF-8 strings and formats checked at compile time (C++20), std::expected
* results (C++23) -- and samples every module, each member being generated
* from its C prototype, its arguments in the C order. The summary names the
* level forms the build had.
*   Over a layer built without SQLite (D_CFG_SQLITE 0) it checks that the face
* reports what the stubs do. Below the 64-bit floor the face compiles to
* nothing, and so does this test. Defining D_TESTS_SQLITE_FACE_MISUSE as 1
* to 4 makes it misuse the face in a way that must not compile (see
* d_tests_sqlite_face_variadic).
*
*   Build: compile the layer's C sources as C, this file as C++, and link them
* with SQLite -- ci/check_db_sqlite.sh does, at every level.
*
*
* path:      /tests/unintegrated/c/db/sqlite/t_sqlite_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.14
*                                                            revised: 2026.10.26
*******************************************************************************/
// std
#include <cstdio>   // std::printf, std::remove
#include <cstring>  // std::strcmp, std::strstr
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "../../../../../inc/djinterp/c/db/sqlite/sqlite_face.hpp"  // under
                                                                     // test
// re_std
#include "../../../../../inc/re_std/cstdint/cstdint.hpp"  // re_std::int64_t,
                                                         // INT64_MAX


#if defined(INT64_MAX)

namespace face = djinterp::face;

namespace
{

// RESULT_OK, RESULT_ERROR, RESULT_MISUSE
//   constants: the SQLite result codes the checks compare with, by value:
// sqlite3.h, which spells `long long`, cannot be included at ISO strict
// C++98, which is the point of the face.
const int RESULT_OK     = 0;   // SQLITE_OK
const int RESULT_ERROR  = 1;   // SQLITE_ERROR
const int RESULT_MISUSE = 21;  // SQLITE_MISUSE

int g_checks   = 0;
int g_failures = 0;

#if ( (D_ENV_LANG_IS_CPP20_OR_HIGHER) &&                                      \
      (D_ENV_CPP_FEATURE_LANG_CONSTINIT) )
// g_unopened
//   a face at namespace scope, constant-initialized: no constructor runs at
// start-up, since an empty face is made at compile time.
constinit face::sqlite g_unopened;
#endif  // C++20 constinit

/*
d_tests_sqlite_face_expect
  Counts one check, and prints it when it fails.
*/
void
d_tests_sqlite_face_expect(
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
d_tests_sqlite_face_same
  Whether a C string is the text expected; NULL is no text.
*/
bool
d_tests_sqlite_face_same(
    const char* _text,
    const char* _expected
)
{
    return ( (_text) &&
             (std::strcmp(_text, _expected) == 0) );
}

}  // namespace

// D_TESTS_SQLITE_FACE_CHECK
//   macro: records one check with its source text and line, which only the
// preprocessor can supply.
#define D_TESTS_SQLITE_FACE_CHECK(_cond)                                      \
    d_tests_sqlite_face_expect((_cond), #_cond, __LINE__)

// D_TESTS_SQLITE_FACE_FORMS
//   constant: the forms of the levels above C++11 this build has, for the
// summary; empty when it has none.
#define D_TESTS_SQLITE_FACE_FORMS                                             \
    D_TESTS_SQLITE_FACE_FORM_OPTIONAL D_TESTS_SQLITE_FACE_FORM_CONCEPTS       \
    D_TESTS_SQLITE_FACE_FORM_SPAN D_TESTS_SQLITE_FACE_FORM_CHAR8              \
    D_TESTS_SQLITE_FACE_FORM_CHECK D_TESTS_SQLITE_FACE_FORM_EXPECTED          \
    D_TESTS_SQLITE_FACE_FORM_DELETED

#if D_INTERNAL_DB_FACE_OPTIONAL
    #define D_TESTS_SQLITE_FACE_FORM_OPTIONAL " optional"
#else
    #define D_TESTS_SQLITE_FACE_FORM_OPTIONAL ""
#endif
#if D_INTERNAL_DB_FACE_CONCEPTS
    #define D_TESTS_SQLITE_FACE_FORM_CONCEPTS " concepts"
#else
    #define D_TESTS_SQLITE_FACE_FORM_CONCEPTS ""
#endif
#if D_INTERNAL_DB_FACE_SPAN
    #define D_TESTS_SQLITE_FACE_FORM_SPAN " span"
#else
    #define D_TESTS_SQLITE_FACE_FORM_SPAN ""
#endif
#if D_INTERNAL_DB_FACE_CHAR8
    #define D_TESTS_SQLITE_FACE_FORM_CHAR8 " char8_t"
#else
    #define D_TESTS_SQLITE_FACE_FORM_CHAR8 ""
#endif
#if D_INTERNAL_DB_FACE_FORMAT_CHECK
    #define D_TESTS_SQLITE_FACE_FORM_CHECK " checked-formats"
#else
    #define D_TESTS_SQLITE_FACE_FORM_CHECK ""
#endif
#if D_INTERNAL_DB_FACE_EXPECTED
    #define D_TESTS_SQLITE_FACE_FORM_EXPECTED " expected"
#else
    #define D_TESTS_SQLITE_FACE_FORM_EXPECTED ""
#endif
#if ( (D_ENV_LANG_IS_CPP26_OR_HIGHER) &&                                      \
      (D_ENV_CPP_FEATURE_LANG_DELETED_FUNCTION) )
    #define D_TESTS_SQLITE_FACE_FORM_DELETED " deleted-reasons"
#else
    #define D_TESTS_SQLITE_FACE_FORM_DELETED ""
#endif

namespace
{

/*
d_tests_sqlite_face_static
  The members that need no connection, in both forms where a member
returns text, and db_face.hpp's descriptions.
*/
void
d_tests_sqlite_face_static(
    bool _stubs
)
{
    face::text quoted;

    std::printf("static members\n");

    D_TESTS_SQLITE_FACE_CHECK(face::sqlite::library_version() != D_NULLPTR);
    D_TESTS_SQLITE_FACE_CHECK(
        (face::sqlite::library_version_number() == 0) == _stubs);
    D_TESTS_SQLITE_FACE_CHECK(face::sqlite::library_at_least(3, 7, 17) ==
                              !_stubs);
    D_TESTS_SQLITE_FACE_CHECK(
        face::sqlite::quote_identifier("a\"b") ==
        std::string((_stubs) ? "" : "\"a\"\"b\""));
    D_TESTS_SQLITE_FACE_CHECK(
        face::sqlite::quote_literal(std::string("it's")) ==
        std::string((_stubs) ? "" : "'it''s'"));
    D_TESTS_SQLITE_FACE_CHECK(face::sqlite::quote_identifier(0).empty());

    // the exact forms: the layer's text, owned by the caller's text, which
    // is null where the layer gave none
    face::sqlite::quote_identifier("a\"b", quoted);
    D_TESTS_SQLITE_FACE_CHECK(
        (_stubs) ? (quoted.is_null())
                 : (d_tests_sqlite_face_same(quoted.c_str(), "\"a\"\"b\"")));
    face::sqlite::quote_literal("it's", quoted);
    D_TESTS_SQLITE_FACE_CHECK(
        (_stubs) ? (quoted.is_null())
                 : (d_tests_sqlite_face_same(quoted.c_str(), "'it''s'")));
    face::sqlite::quote_identifier(0, quoted);
    D_TESTS_SQLITE_FACE_CHECK(quoted.is_null());

    D_TESTS_SQLITE_FACE_CHECK(
        face::sqlite::result_message(D_DB_NOT_FOUND) != D_NULLPTR);
    D_TESTS_SQLITE_FACE_CHECK(
        d_tests_sqlite_face_same(face::status_message(D_DB_NOT_FOUND),
                                 "not found"));
    D_TESTS_SQLITE_FACE_CHECK(face::type_name(D_DB_TYPE_STRING) != D_NULLPTR);
    D_TESTS_SQLITE_FACE_CHECK(!face::sqlite::is_database_file("nothing.db"));

    return;
}

/*
d_tests_sqlite_face_ownership
  Owning, giving up, swapping and replacing a connection; from C++11,
moving it; from C++20, a face constant-initialized.
*/
void
d_tests_sqlite_face_ownership(
    void
)
{
    face::sqlite db;

    std::printf("ownership\n");

#if ( (D_ENV_LANG_IS_CPP20_OR_HIGHER) &&                                      \
      (D_ENV_CPP_FEATURE_LANG_CONSTINIT) )
    D_TESTS_SQLITE_FACE_CHECK(!g_unopened.is_open());
#endif  // C++20 constinit

    D_TESTS_SQLITE_FACE_CHECK(!db.is_open());
    D_TESTS_SQLITE_FACE_CHECK(db.handle() == D_NULLPTR);
    D_TESTS_SQLITE_FACE_CHECK(db.open_memory() == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(db.is_open());

    struct sqlite3* const first = db.handle();

    // a failed open leaves the connection as it was
    D_TESTS_SQLITE_FACE_CHECK(
        db.open_readonly("t_sqlite_face_no/such/file.db") != RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(db.handle() == first);
    D_TESTS_SQLITE_FACE_CHECK(
        db.open_with("t_sqlite_face_no/such/file.db",
                     D_SQLITE_OPEN_READWRITE) != RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(db.handle() == first);
    D_TESTS_SQLITE_FACE_CHECK(
        db.open_with("unused.db", 0u, std::string("unused")) ==
        RESULT_MISUSE);
    D_TESTS_SQLITE_FACE_CHECK(db.handle() == first);

    // a connection opened with flags replaces it
    {
        face::sqlite flagged;

        D_TESTS_SQLITE_FACE_CHECK(
            flagged.open_with("t_sqlite_face_flags",
                              D_SQLITE_OPEN_READWRITE | D_SQLITE_OPEN_CREATE |
                              D_SQLITE_OPEN_MEMORY) == RESULT_OK);
        D_TESTS_SQLITE_FACE_CHECK(flagged.is_open());
        D_TESTS_SQLITE_FACE_CHECK(flagged.exec("SELECT 1") == RESULT_OK);
    }

    face::sqlite other;

    other.swap(db);
    D_TESTS_SQLITE_FACE_CHECK( (!db.is_open()) &&
                               (other.handle() == first) );
    swap(db, other);
    D_TESTS_SQLITE_FACE_CHECK(db.handle() == first);

    struct sqlite3* const raw = db.release();

    D_TESTS_SQLITE_FACE_CHECK( (raw == first) && (!db.is_open()) );

    face::sqlite adopted(raw);

    D_TESTS_SQLITE_FACE_CHECK(adopted.handle() == raw);
    D_TESTS_SQLITE_FACE_CHECK(adopted.exec("SELECT 1") == RESULT_OK);

#if D_MOVE_ENABLED
    face::sqlite moved(static_cast<face::sqlite&&>(adopted));

    D_TESTS_SQLITE_FACE_CHECK( (!adopted.is_open()) &&
                               (moved.handle() == raw) );
    D_TESTS_SQLITE_FACE_CHECK(static_cast<bool>(moved));
    adopted = static_cast<face::sqlite&&>(moved);
    D_TESTS_SQLITE_FACE_CHECK( (adopted.handle() == raw) &&
                               (!moved) );
#endif  // D_MOVE_ENABLED

    adopted.reset();
    D_TESTS_SQLITE_FACE_CHECK(!adopted.is_open());
    D_TESTS_SQLITE_FACE_CHECK(adopted.close() == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(adopted.exec("SELECT 1") == RESULT_MISUSE);
    D_TESTS_SQLITE_FACE_CHECK(!adopted.has_table(0, "anything"));

    return;
}

/*
d_tests_sqlite_face_conversions
  string_arg from a C string, a std::string and NULL; a column array; a
name list and a vector; a column list; text, null and not, and a
std::string; the numbers, booleans and enumerations a member writes.
*/
void
d_tests_sqlite_face_conversions(
    face::sqlite& _db
)
{
    static const struct d_db_column people[] =
    {
        { "id",   D_DB_TYPE_BIG_INTEGER, D_DB_COLUMN_PRIMARY_KEY, 0, 0u, 0u,
          0, 0u },
        { "name", D_DB_TYPE_STRING,      D_DB_COLUMN_NOT_NULL, "'?'", 20u,
          0u, 0, 0u }
    };
    const std::string        notes("notes");
    face::name_list          names;
    std::vector<std::string> copied;
    face::column_list        columns;
    face::text               value;
    std::string              plain("unchanged");
    re_std::int64_t          count  = -1;
    double                   real   = 0.0;
    bool                     exists = false;

    std::printf("conversions\n");

    D_TESTS_SQLITE_FACE_CHECK(
        _db.create_table_sql(0, notes, "id INTEGER PRIMARY KEY, body TEXT",
                             D_DB_IF_NOT_EXISTS) == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(_db.create_table(std::string("main"), "people",
                                               people, 0u) == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(_db.has_table(0, "notes"));
    D_TESTS_SQLITE_FACE_CHECK(
        (_db.has_table_s("main", notes, exists) == RESULT_OK) && (exists));
    D_TESTS_SQLITE_FACE_CHECK(_db.list_tables(0, names) == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK( (names.size() == 2u)        &&
                               (names.contains("notes"))   &&
                               (names.contains(notes))     &&
                               (!names.contains("other"))  &&
                               (!names.contains(0)) );
    D_TESTS_SQLITE_FACE_CHECK(
        static_cast<std::size_t>(names.end() - names.begin()) == 2u);
    D_TESTS_SQLITE_FACE_CHECK(_db.list_tables(0, copied) == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK( (copied.size() == 2u) &&
                               (copied[0] == names[0]) );
    D_TESTS_SQLITE_FACE_CHECK(_db.list_tables("nowhere", copied) != 0);
    D_TESTS_SQLITE_FACE_CHECK(copied.empty());
    D_TESTS_SQLITE_FACE_CHECK(_db.list_columns(0, "people", columns) ==
                              RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK( (columns.size() == 2u)                    &&
                               (columns.find("id"))                      &&
                               (columns.find("id")->primary_key == 1)    &&
                               (columns[1].not_null)                     &&
                               (!columns.find("nobody")) );

    D_TESTS_SQLITE_FACE_CHECK(
        _db.exec("INSERT INTO notes (body) VALUES (NULL), ('x')") ==
        RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(
        _db.query_text(value, "SELECT body FROM notes WHERE id = 1") ==
        RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK( (value.is_null()) && (value.str().empty()) );
    D_TESTS_SQLITE_FACE_CHECK(
        _db.query_text(value, "SELECT body FROM notes WHERE id = 2") ==
        RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(d_tests_sqlite_face_same(value.c_str(), "x"));
    D_TESTS_SQLITE_FACE_CHECK(
        _db.query_text(plain, "SELECT body FROM notes WHERE id = 2") ==
        RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(plain == "x");
    D_TESTS_SQLITE_FACE_CHECK(
        _db.query_text(value, "SELECT body FROM notes WHERE id = 9") ==
        D_DB_NOT_FOUND);
    D_TESTS_SQLITE_FACE_CHECK(value.is_null());
    D_TESTS_SQLITE_FACE_CHECK(
        (_db.query_int64(count, "SELECT count(*) FROM notes") == RESULT_OK) &&
        (count == 2));
    D_TESTS_SQLITE_FACE_CHECK(
        (_db.query_double(real, "SELECT 2.5") == RESULT_OK) && (real == 2.5));
    D_TESTS_SQLITE_FACE_CHECK(
        (_db.count_rows(0, "notes", count) == RESULT_OK) && (count == 2));

    return;
}

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

/*
d_tests_sqlite_face_cpp17
  C++17's forms: text as a std::optional<std::string>, empty for SQL NULL
and for a failure; and guards that deduce their face. Over the notes
d_tests_sqlite_face_conversions made: 1 NULL, 2 'x'.
*/
void
d_tests_sqlite_face_cpp17(
    face::sqlite& _db
)
{
    std::printf("C++17 forms\n");

#if D_INTERNAL_DB_FACE_OPTIONAL
    std::optional<std::string> body(std::string("stale"));
    std::optional<std::string> mode;

    D_TESTS_SQLITE_FACE_CHECK(
        _db.query_text(body, "SELECT body FROM notes WHERE id = 1") ==
        RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(!body.has_value());
    D_TESTS_SQLITE_FACE_CHECK(
        _db.query_text(body, "SELECT body FROM notes WHERE id = 2") ==
        RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK( (body.has_value()) && (*body == "x") );
    D_TESTS_SQLITE_FACE_CHECK(
        _db.query_text(body, "SELECT body FROM notes WHERE id = 9") ==
        D_DB_NOT_FOUND);
    D_TESTS_SQLITE_FACE_CHECK(!body.has_value());
    D_TESTS_SQLITE_FACE_CHECK(
        (_db.query_textf(body, "SELECT body FROM notes WHERE id = %d", 2) ==
         RESULT_OK) &&
        (body == std::string("x")));
    D_TESTS_SQLITE_FACE_CHECK(
        (_db.get_pragma(0, "journal_mode", mode) == RESULT_OK) &&
        (mode == std::string("memory")));
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

#if D_ENV_CPP_FEATURE_LANG_DEDUCTION_GUIDES
    {
        face::transaction work(_db);

        D_TESTS_SQLITE_FACE_CHECK(work.is_active());

        {
            face::savepoint step(_db, "step");

            D_TESTS_SQLITE_FACE_CHECK( (step.is_active()) &&
                                       (step.release() == RESULT_OK) );
        }

        D_TESTS_SQLITE_FACE_CHECK(work.rollback() == RESULT_OK);
    }

    D_TESTS_SQLITE_FACE_CHECK(!_db.in_transaction());
#endif  // D_ENV_CPP_FEATURE_LANG_DEDUCTION_GUIDES

    return;
}

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#if D_INTERNAL_DB_FACE_CONCEPTS

/*
d_tests_sqlite_face_generic
  Code written for any face, through face::connection: a transaction begun
and rolled back, its guard deduced.
*/
template<face::connection Database>
bool
d_tests_sqlite_face_generic(
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
d_tests_sqlite_face_cpp20
  C++20's forms: the face as a face::connection; a column array as a
std::span, or anything that makes one; UTF-8 strings; and a format read at
compile time, in the shared dialect and in sqlite3_mprintf's.
*/
void
d_tests_sqlite_face_cpp20(
    face::sqlite& _db
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
    static_assert(face::connection<face::sqlite>,
                  "face::sqlite is a face::connection");
    D_TESTS_SQLITE_FACE_CHECK(d_tests_sqlite_face_generic(_db));
#endif  // D_INTERNAL_DB_FACE_CONCEPTS

#if D_INTERNAL_DB_FACE_SPAN
    const std::vector<struct d_db_column> held(columns, columns + 1);

    D_TESTS_SQLITE_FACE_CHECK(
        _db.create_table(0, "spanned", std::span<const d_db_column>(columns),
                         0u) == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(
        _db.create_table(0, "held", held, 0u) == RESULT_OK);
#else
    D_TESTS_SQLITE_FACE_CHECK(
        _db.create_table(0, "spanned", columns, 0u) == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(
        _db.create_table(0, "held", columns, 0u) == RESULT_OK);
#endif  // D_INTERNAL_DB_FACE_SPAN

#if D_INTERNAL_DB_FACE_CHAR8
    const std::u8string name(u8"held");

    D_TESTS_SQLITE_FACE_CHECK(_db.has_table(0, u8"spanned"));
    D_TESTS_SQLITE_FACE_CHECK(_db.has_table(u8"main", name));
#endif  // D_INTERNAL_DB_FACE_CHAR8

#if D_INTERNAL_DB_FACE_FORMAT_CHECK
    // a format checked at compile time is a pointer to the literal
    constexpr face::sqlite::format<const char*, const char*> both(
        "SELECT count(*) FROM %T");

    static_assert(sizeof(both) == sizeof(const char*),
                  "a checked format is its pointer");
    D_TESTS_SQLITE_FACE_CHECK(
        d_tests_sqlite_face_same(both.c_str(), "SELECT count(*) FROM %T"));
    D_TESTS_SQLITE_FACE_CHECK(
        (_db.query_int64f(count, both, "main", "spanned") == RESULT_OK) &&
        (count == 0));
#endif  // D_INTERNAL_DB_FACE_FORMAT_CHECK

    // the same calls compile at every level from C++11; from C++20 each
    // literal is read against its arguments first
    D_TESTS_SQLITE_FACE_CHECK(
        (_db.query_int64f(count, "SELECT count(*) FROM %I", "held") ==
         RESULT_OK) &&
        (count == 0));
    D_TESTS_SQLITE_FACE_CHECK(
        (_db.query_int64f(count, "SELECT %lld + %d",
                          static_cast<long long>(40), 2) == RESULT_OK) &&
        (count == 42));

    return;
}

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#if D_INTERNAL_DB_FACE_EXPECTED

/*
d_tests_sqlite_face_cpp23
  C++23's forms: a result written through a parameter, returned instead,
as a std::expected that holds the value or the layer's result. Over the
notes and people d_tests_sqlite_face_conversions made.
*/
void
d_tests_sqlite_face_cpp23(
    face::sqlite& _db
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
    const auto tables    = _db.list_tables(0);
    const auto people    = _db.list_columns(0, "people");
    const auto exists    = _db.has_table_s(0, "people");
    const auto nothing   = face::sqlite().query_int64("SELECT 1");

    D_TESTS_SQLITE_FACE_CHECK( (count.has_value()) && (*count == 2) );
    D_TESTS_SQLITE_FACE_CHECK( (!missing.has_value()) &&
                               (missing.error() == D_DB_NOT_FOUND) );
    D_TESTS_SQLITE_FACE_CHECK( (null_body.has_value()) &&
                               (null_body->is_null()) );
    D_TESTS_SQLITE_FACE_CHECK( (body.has_value()) &&
                               (d_tests_sqlite_face_same(body->c_str(),
                                                         "x")) );
    D_TESTS_SQLITE_FACE_CHECK( (tables.has_value()) &&
                               (tables->contains("notes")) );
    D_TESTS_SQLITE_FACE_CHECK( (people.has_value()) &&
                               (people->find("name")) );
    D_TESTS_SQLITE_FACE_CHECK( (exists.has_value()) && (*exists) );
    D_TESTS_SQLITE_FACE_CHECK( (!nothing.has_value()) &&
                               (nothing.error() == RESULT_MISUSE) );

    return;
}

#endif  // D_INTERNAL_DB_FACE_EXPECTED

/*
d_tests_sqlite_face_statements
  Rows and statements: a statement prepared into a face, its parameters
described and bound -- text from a std::string too -- run for a count and
for rows; rows read by place and by name, their values converted; a
statement finalized by hand, by its destructor, and on a failed prepare,
which leaves it empty; from C++11 one moved and swapped, and the f form;
from C++23 the statement, the rows, a column's place and a conversion
returned.
*/
void
d_tests_sqlite_face_statements(
    face::sqlite& _db
)
{
    face::sqlite_statement insert;
    face::sqlite_statement select;
    face::rows             found;
    re_std::int64_t        changes = -1;
    re_std::int64_t        number  = 0;
    double                 real    = 0.0;
    bool                   truth   = false;
    std::size_t            index   = 99u;

    std::printf("statements and rows\n");

    D_TESTS_SQLITE_FACE_CHECK(
        _db.exec("CREATE TABLE kept (id INTEGER, flag BOOLEAN, ratio REAL, "
                 "body TEXT, raw BLOB)") == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(!insert.is_prepared());
    D_TESTS_SQLITE_FACE_CHECK(
        _db.prepare(insert, "INSERT INTO kept VALUES (?, ?, ?, :body, ?)")
        == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK( (insert.is_prepared()) &&
                               (insert.handle() != 0) );
    D_TESTS_SQLITE_FACE_CHECK(insert.statement_connection() == _db.handle());
    D_TESTS_SQLITE_FACE_CHECK(insert.parameter_count() == 5u);
    D_TESTS_SQLITE_FACE_CHECK(
        (insert.parameter_index(":body", index) == RESULT_OK) &&
        (index == 4u));
    D_TESTS_SQLITE_FACE_CHECK(insert.bind_int64(1u, 7) == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(insert.bind_bool(2u, true) == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(insert.bind_double(3u, 0.5) == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(insert.bind_text(4u, std::string("it's")) ==
                              RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(insert.bind_blob(5u, "\0\1", 2u) ==
                              RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(
        (insert.execute_update(changes) == RESULT_OK) && (changes == 1));
    D_TESTS_SQLITE_FACE_CHECK(insert.clear_bindings() == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(insert.bind_null(1u) == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(insert.bind_text(4u, 0) == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(insert.execute_query(found) == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK( (found.column_count() == 0u) &&
                               (found.empty()) );

    // rows, by place and by name, converted
    D_TESTS_SQLITE_FACE_CHECK(
        _db.prepare(select, "SELECT id, flag, ratio, body, raw FROM kept "
                            "WHERE id IS NOT NULL") == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(select.execute_query(found) == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK( (found.row_count() == 1u)       &&
                               (found.column_count() == 5u)    &&
                               (!found.empty()) );
    D_TESTS_SQLITE_FACE_CHECK(
        (d_tests_sqlite_face_same(found.column(3).name, "body")) &&
        (found.column(1).type == D_DB_TYPE_BOOLEAN)               &&
        (d_tests_sqlite_face_same(found.column(1).type_name, "BOOLEAN")));
    D_TESTS_SQLITE_FACE_CHECK(
        (face::value_int64(found.value(0u, 0u), number) == RESULT_OK) &&
        (number == 7));
    D_TESTS_SQLITE_FACE_CHECK(
        (face::value_bool(found.value(0u, 1u), truth) == RESULT_OK) &&
        (truth));
    D_TESTS_SQLITE_FACE_CHECK(
        (face::value_double(found.value(0u, 2u), real) == RESULT_OK) &&
        (real == 0.5));
    D_TESTS_SQLITE_FACE_CHECK(
        face::value_int64(found.value(0u, 3u), number) == D_DB_MISMATCH);
    D_TESTS_SQLITE_FACE_CHECK(
        d_tests_sqlite_face_same(found.value(0u, 3u)->as.text, "it's"));
    D_TESTS_SQLITE_FACE_CHECK( (found.value(0u, 4u)->type ==
                                D_DB_VALUE_BLOB) &&
                               (found.value(0u, 4u)->size == 2u) );
    D_TESTS_SQLITE_FACE_CHECK(found.value(1u, 0u) == 0);
    D_TESTS_SQLITE_FACE_CHECK(
        (found.column_index("RATIO", index) == RESULT_OK) && (index == 2u));
    D_TESTS_SQLITE_FACE_CHECK(found.column_index("nobody", index) ==
                              D_DB_NOT_FOUND);
    D_TESTS_SQLITE_FACE_CHECK(found.core().row_count == 1u);
    found.clear();
    D_TESTS_SQLITE_FACE_CHECK( (found.row_count() == 0u) &&
                               (found.core().columns == 0) );

    // the connection's own counts and rows
    D_TESTS_SQLITE_FACE_CHECK(
        (_db.exec_changes(changes, "UPDATE kept SET flag = 0") ==
         RESULT_OK) &&
        (changes == 2));
    D_TESTS_SQLITE_FACE_CHECK(
        _db.query_rows(found, "SELECT count(*) AS n FROM kept") ==
        RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK( (found.row_count() == 1u) &&
                               (found.value(0u, 0u)->as.integer == 2) );

    // finalized by hand, and emptied by a prepare that fails
    D_TESTS_SQLITE_FACE_CHECK(select.finalize() == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(!select.is_prepared());
    D_TESTS_SQLITE_FACE_CHECK(_db.prepare(select, "SELECT 1") == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(_db.prepare(select, "SELEC 1") ==
                              RESULT_ERROR);
    D_TESTS_SQLITE_FACE_CHECK(!select.is_prepared());
    D_TESTS_SQLITE_FACE_CHECK(_db.prepare(select, "SELECT 1; SELECT 2") ==
                              RESULT_MISUSE);

    // handed over, and taken back
    D_TESTS_SQLITE_FACE_CHECK(_db.prepare(select, "SELECT 2") == RESULT_OK);
    {
        struct sqlite3_stmt* const raw = select.release();

        D_TESTS_SQLITE_FACE_CHECK( (raw != 0) && (!select.is_prepared()) );
        select.reset(raw);
        D_TESTS_SQLITE_FACE_CHECK(select.handle() == raw);
    }
    insert.swap(select);
    D_TESTS_SQLITE_FACE_CHECK( (insert.parameter_count() == 0u) &&
                               (select.parameter_count() == 5u) );
    swap(insert, select);
    D_TESTS_SQLITE_FACE_CHECK(insert.parameter_count() == 5u);

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    {
        face::sqlite_statement moved(static_cast<face::sqlite_statement&&>(
                                         insert));

        D_TESTS_SQLITE_FACE_CHECK( (moved) && (!insert) );
        insert = static_cast<face::sqlite_statement&&>(moved);
        D_TESTS_SQLITE_FACE_CHECK( (insert) && (!moved) );

        face::rows taken(static_cast<face::rows&&>(found));

        D_TESTS_SQLITE_FACE_CHECK( (taken.row_count() == 1u) &&
                                   (found.row_count() == 0u) );
    }
    D_TESTS_SQLITE_FACE_CHECK(
        _db.preparef(select, "SELECT id FROM %I WHERE body = %Q", "kept",
                     "it's") == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(
        (select.execute_update(changes) == RESULT_OK) && (changes == 1));
    D_TESTS_SQLITE_FACE_CHECK(
        _db.query_rowsf(found, "SELECT %d + %d", 2, 3) == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(found.value(0u, 0u)->as.integer == 5);
    D_TESTS_SQLITE_FACE_CHECK(
        (_db.execf_changes(changes, "DELETE FROM kept WHERE id IS %s",
                           "NULL") == RESULT_OK) &&
        (changes == 1));
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_INTERNAL_DB_FACE_EXPECTED
    {
        auto       prepared = _db.prepare("SELECT id, body FROM kept");
        const auto refused  = _db.prepare("SELEC");
        const auto all      = _db.query_rows("SELECT id FROM kept");

        D_TESTS_SQLITE_FACE_CHECK( (prepared.has_value()) &&
                                   (prepared->is_prepared()) );
        D_TESTS_SQLITE_FACE_CHECK( (!refused.has_value()) &&
                                   (refused.error() == RESULT_ERROR) );
        D_TESTS_SQLITE_FACE_CHECK( (all.has_value()) &&
                                   (all->row_count() == 1u) );

        // nothing more without a statement
        if (prepared)
        {
            const auto rows_of = prepared->execute_query();
            const auto count   = prepared->execute_update();

            D_TESTS_SQLITE_FACE_CHECK( (rows_of.has_value()) &&
                                       (rows_of->row_count() == 1u) );
            D_TESTS_SQLITE_FACE_CHECK( (count.has_value()) && (*count == 1) );
            D_TESTS_SQLITE_FACE_CHECK(
                (rows_of) &&
                (rows_of->column_index("body") == std::size_t(1u)));
            D_TESTS_SQLITE_FACE_CHECK(
                (rows_of) &&
                (face::value_int64(rows_of->value(0u, 0u)) ==
                 re_std::int64_t(7)));
            D_TESTS_SQLITE_FACE_CHECK(
                (rows_of) &&
                (face::value_double(rows_of->value(0u, 1u)).error() ==
                 D_DB_MISMATCH));
        }
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    return;
}

/*
d_tests_sqlite_face_cursors
  Rows a few at a time through the statement face: pages no longer than asked
for, and a short one ending the run; the connection free meanwhile, and a fetch
after the end beginning another run; a run closed part-way; from C++23 a page
returned.
*/
void
d_tests_sqlite_face_cursors(
    face::sqlite& _db
)
{
    face::sqlite_statement select;
    face::rows             page;

    std::printf("rows a few at a time\n");

    D_TESTS_SQLITE_FACE_CHECK(
        _db.prepare(select,
                    "SELECT 1 UNION ALL SELECT 2 "
                    "UNION ALL SELECT 3") == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(select.open_cursor() == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(select.fetch_rows(2u, page) == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK( (page.row_count() == 2u) &&
                               (page.column_count() == 1u) );
    D_TESTS_SQLITE_FACE_CHECK(_db.exec("SELECT 1") == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(select.fetch_rows(2u, page) == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK( (page.row_count() == 1u) &&
                               (page.value(0u, 0u)->as.integer == 3) );
    D_TESTS_SQLITE_FACE_CHECK(select.fetch_rows(2u, page) == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(page.row_count() == 2u);
    D_TESTS_SQLITE_FACE_CHECK(select.close_cursor() == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(_db.exec("SELECT 1") == RESULT_OK);

    // closed part-way, and again
    D_TESTS_SQLITE_FACE_CHECK(select.open_cursor() == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(select.fetch_rows(1u, page) == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(select.close_cursor() == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(_db.exec("SELECT 1") == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(select.close_cursor() == RESULT_OK);

#if D_INTERNAL_DB_FACE_EXPECTED
    // a page returned: three rows fill the first exactly, and the next is
    // empty, its columns still described
    D_TESTS_SQLITE_FACE_CHECK(select.open_cursor() == RESULT_OK);
    {
        const auto full = select.fetch_rows(3u);
        const auto rest = select.fetch_rows(3u);

        D_TESTS_SQLITE_FACE_CHECK( (full.has_value()) &&
                                   (full->row_count() == 3u) );
        D_TESTS_SQLITE_FACE_CHECK( (rest.has_value())           &&
                                   (rest->row_count() == 0u)    &&
                                   (rest->column_count() == 1u) );
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    D_TESTS_SQLITE_FACE_CHECK(select.finalize() == RESULT_OK);

    return;
}

/*
d_tests_sqlite_face_modules
  A member or two of each module beyond the ones above: connections,
databases, schema, settings and maintenance.
*/
void
d_tests_sqlite_face_modules(
    face::sqlite& _db
)
{
    enum d_sqlite_journal_mode mode    = D_SQLITE_JOURNAL_DELETE;
    re_std::int32_t            version = 0;
    bool                       enabled = false;
    bool                       ok      = false;
    face::text                 setting;
    face::sqlite               copy;
    std::vector<std::string>   databases;

    std::printf("modules\n");

    D_TESTS_SQLITE_FACE_CHECK(_db.exec("SELECT * FROM nope") == RESULT_ERROR);
    D_TESTS_SQLITE_FACE_CHECK( (_db.error_message()) &&
                               (std::strstr(_db.error_message(), "nope")) );
    D_TESTS_SQLITE_FACE_CHECK(
        d_tests_sqlite_face_same(_db.database_path(0), ""));
    D_TESTS_SQLITE_FACE_CHECK(!_db.is_readonly(0));
    D_TESTS_SQLITE_FACE_CHECK(_db.list_databases(databases) == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK( (databases.size() >= 1u) &&
                               (databases[0] == "main") );
    D_TESTS_SQLITE_FACE_CHECK(_db.create_index(0, "notes_body", "notes",
                                               "body", 0u) == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(_db.has_index(0, "notes_body"));

    // RENAME COLUMN is 3.25.0's; before it the layer says so, and changes
    // nothing
    if (face::sqlite::supports(D_SQLITE_FEATURE_RENAME_COLUMN))
    {
        D_TESTS_SQLITE_FACE_CHECK(
            _db.rename_column(0, "notes", "body", "text") == RESULT_OK);
        D_TESTS_SQLITE_FACE_CHECK( (_db.has_column(0, "notes", "text")) &&
                                   (!_db.has_column(0, "notes", "body")) );
    }
    else
    {
        D_TESTS_SQLITE_FACE_CHECK(
            _db.rename_column(0, "notes", "body", "text") ==
            D_DB_UNSUPPORTED);
        D_TESTS_SQLITE_FACE_CHECK(_db.has_column(0, "notes", "body"));
    }

    D_TESTS_SQLITE_FACE_CHECK(_db.set_foreign_keys(true) == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(
        (_db.get_foreign_keys(enabled) == RESULT_OK) && (enabled));
    D_TESTS_SQLITE_FACE_CHECK(_db.set_user_version(0, 7) == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(
        (_db.get_user_version(0, version) == RESULT_OK) && (version == 7));
    D_TESTS_SQLITE_FACE_CHECK(
        (_db.get_journal_mode(0, mode) == RESULT_OK) &&
        (mode == D_SQLITE_JOURNAL_MEMORY));
    D_TESTS_SQLITE_FACE_CHECK(
        (_db.get_pragma(0, "user_version", setting) == RESULT_OK) &&
        (d_tests_sqlite_face_same(setting.c_str(), "7")));
    D_TESTS_SQLITE_FACE_CHECK(
        (_db.integrity_check(0, ok) == RESULT_OK) && (ok));
    D_TESTS_SQLITE_FACE_CHECK(copy.open_memory() == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(
        face::sqlite::copy_database(_db, 0, copy, 0) == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(copy.has_table(0, "people"));

    return;
}

/*
d_tests_sqlite_face_guards
  The transaction guard: committed, rolled back by hand, rolled back when
its scope ends, and one whose beginning failed; the savepoint guard:
released, rolled back by hand, and rolled back when its scope ends, named
by a literal or by a string that outlives it.
*/
void
d_tests_sqlite_face_guards(
    face::sqlite& _db
)
{
    const std::string dropped_name("dropped");
    re_std::int64_t   count = 0;

    std::printf("guards\n");

    D_TESTS_SQLITE_FACE_CHECK(
        _db.exec("CREATE TABLE g (v INTEGER)") == RESULT_OK);

    {
        face::transaction<face::sqlite> work(_db);

        D_TESTS_SQLITE_FACE_CHECK( (work.result() == RESULT_OK) &&
                                   (work.is_active())           &&
                                   (_db.in_transaction()) );
        D_TESTS_SQLITE_FACE_CHECK(
            _db.exec("INSERT INTO g VALUES (1)") == RESULT_OK);
        D_TESTS_SQLITE_FACE_CHECK(work.commit() == RESULT_OK);
        D_TESTS_SQLITE_FACE_CHECK( (!work.is_active()) &&
                                   (!_db.in_transaction()) );
        D_TESTS_SQLITE_FACE_CHECK(work.commit() == D_DB_INVALID_ARGUMENT);
    }

    {
        face::transaction<face::sqlite> work(_db,
                                             D_SQLITE_TRANSACTION_IMMEDIATE);

        D_TESTS_SQLITE_FACE_CHECK(
            _db.exec("INSERT INTO g VALUES (2)") == RESULT_OK);
    }

    D_TESTS_SQLITE_FACE_CHECK(!_db.in_transaction());

    {
        face::transaction<face::sqlite> work(_db);

        D_TESTS_SQLITE_FACE_CHECK(
            _db.exec("INSERT INTO g VALUES (3)") == RESULT_OK);
        D_TESTS_SQLITE_FACE_CHECK(work.rollback() == RESULT_OK);
        D_TESTS_SQLITE_FACE_CHECK(!work.is_active());
    }

    D_TESTS_SQLITE_FACE_CHECK(
        _db.begin_transaction(D_SQLITE_TRANSACTION_DEFERRED) == RESULT_OK);

    {
        face::transaction<face::sqlite> inner(_db);

        D_TESTS_SQLITE_FACE_CHECK( (inner.result() != RESULT_OK) &&
                                   (!inner.is_active()) );
    }

    D_TESTS_SQLITE_FACE_CHECK(_db.in_transaction());

    {
        face::savepoint<face::sqlite> kept(_db, "kept");

        D_TESTS_SQLITE_FACE_CHECK(
            (kept.result() == RESULT_OK) &&
            (d_tests_sqlite_face_same(kept.name(), "kept")));
        D_TESTS_SQLITE_FACE_CHECK(
            _db.exec("INSERT INTO g VALUES (4)") == RESULT_OK);
        D_TESTS_SQLITE_FACE_CHECK(kept.release() == RESULT_OK);
        D_TESTS_SQLITE_FACE_CHECK(kept.release() == D_DB_INVALID_ARGUMENT);
    }

    {
        face::savepoint<face::sqlite> dropped(_db, dropped_name);

        D_TESTS_SQLITE_FACE_CHECK(dropped.name() == dropped_name.c_str());
        D_TESTS_SQLITE_FACE_CHECK(
            _db.exec("INSERT INTO g VALUES (5)") == RESULT_OK);
    }

    {
        face::savepoint<face::sqlite> undone(_db, "undone");

        D_TESTS_SQLITE_FACE_CHECK(
            _db.exec("INSERT INTO g VALUES (6)") == RESULT_OK);
        D_TESTS_SQLITE_FACE_CHECK(undone.rollback() == RESULT_OK);
        D_TESTS_SQLITE_FACE_CHECK(!undone.is_active());
    }

    D_TESTS_SQLITE_FACE_CHECK(_db.commit() == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(
        (_db.query_int64(count, "SELECT sum(v) FROM g") == RESULT_OK) &&
        (count == 1 + 4));

    return;
}

/*
d_tests_sqlite_face_files
  A database file: made, opened, recognised, and deleted, through the
static members and a second face.
*/
void
d_tests_sqlite_face_files(
    const std::string& _directory
)
{
    const std::string path = _directory + "/t_sqlite_face.db";
    face::sqlite      db;

    std::printf("files\n");

    D_TESTS_SQLITE_FACE_CHECK(
        face::sqlite::create_database_file(path, 0u) == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(face::sqlite::is_database_file(path));
    D_TESTS_SQLITE_FACE_CHECK(db.open(path) == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(
        (db.database_path(0)) &&
        (std::strstr(db.database_path(0), "t_sqlite_face.db")));
    D_TESTS_SQLITE_FACE_CHECK(db.close() == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(
        face::sqlite::delete_database_file(path, 0u) == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(!face::sqlite::is_database_file(path));

    return;
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

/*
d_tests_sqlite_face_variadic
  The f forms, from C++11: in sqlite3_mprintf's conversions, and in the
shared dialect, which a format naming %I or %T is read in; values quoted
into SQL, and read back through each kind of result; and a format built at
run time, through face::runtime_format.
*/
void
d_tests_sqlite_face_variadic(
    face::sqlite& _db
)
{
    const std::string built("SELECT count(*) FROM f");
    re_std::int64_t   count = 0;
    double            real  = 0.0;
    face::text        value;
    std::string       plain;

    std::printf("f forms\n");

    D_TESTS_SQLITE_FACE_CHECK(
        _db.execf("CREATE TABLE f (n INTEGER, t TEXT)") == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(
        _db.execf("INSERT INTO \"%w\" VALUES (%d, %Q), (%lld, %Q)", "f", 1,
                  "it's", static_cast<long long>(2), nullptr) == RESULT_OK);
    D_TESTS_SQLITE_FACE_CHECK(
        (_db.query_int64f(count, "SELECT sum(n) FROM \"%w\".\"%w\"",
                          "main", "f") == RESULT_OK) &&
        (count == 3));
    D_TESTS_SQLITE_FACE_CHECK(
        (_db.query_int64f(count, "SELECT sum(n) FROM %T WHERE t = %Q",
                          "main", "f", "it's") == RESULT_OK) &&
        (count == 1));
    D_TESTS_SQLITE_FACE_CHECK(
        (_db.query_doublef(real, "SELECT %g", 0.25) == RESULT_OK) &&
        (real == 0.25));
    D_TESTS_SQLITE_FACE_CHECK(
        (_db.query_textf(value, "SELECT t FROM f WHERE n = %d", 1) ==
         RESULT_OK) &&
        (d_tests_sqlite_face_same(value.c_str(), "it's")));
    D_TESTS_SQLITE_FACE_CHECK(
        (_db.query_textf(plain, "SELECT t FROM f WHERE n = %u", 2u) ==
         RESULT_OK) &&
        (plain.empty()));
    D_TESTS_SQLITE_FACE_CHECK(
        (_db.query_int64f(count, face::runtime_format(built)) ==
         RESULT_OK) &&
        (count == 2));
    D_TESTS_SQLITE_FACE_CHECK(
        (_db.query_textf(value, face::runtime_format("SELECT %Q"), "rt") ==
         RESULT_OK) &&
        (d_tests_sqlite_face_same(value.c_str(), "rt")));

    face::text taken(static_cast<face::text&&>(value));

    D_TESTS_SQLITE_FACE_CHECK( (value.is_null()) &&
                               (static_cast<bool>(taken)) );

#if defined(D_TESTS_SQLITE_FACE_MISUSE)
    // must not compile, each refused by the face, in its own words where it
    // has them (ci/check_db_sqlite.sh runs each from the level that refuses
    // it, and expects those words):
    //   1  a std::string through C's `...` (C++11): "an f form takes
    //      numbers"
    //   2  a literal format its argument does not match (C++20):
    //      "the_format_argument_does_not_match_its_conversion"
    //   3  a std::string as a format (C++11), which goes through
    //      runtime_format: no form takes it
    //   4  a guard made and dropped at once, which begins and ends at once
    //      (C++17): its warning, made an error
    #if (D_TESTS_SQLITE_FACE_MISUSE == 1)
        D_TESTS_SQLITE_FACE_CHECK(_db.execf("SELECT %Q", plain) == RESULT_OK);
    #elif (D_TESTS_SQLITE_FACE_MISUSE == 2)
        D_TESTS_SQLITE_FACE_CHECK(_db.execf("SELECT %d", "two") == RESULT_OK);
    #elif (D_TESTS_SQLITE_FACE_MISUSE == 3)
        D_TESTS_SQLITE_FACE_CHECK(_db.execf(built) == RESULT_OK);
    #elif (D_TESTS_SQLITE_FACE_MISUSE == 4)
        face::transaction<face::sqlite>{_db};
    #endif
#endif  // defined(D_TESTS_SQLITE_FACE_MISUSE)

    return;
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

/*
d_tests_sqlite_face_stubs
  The face over the stubs: nothing opens, nothing is listed, no guard
begins, no text, statement or rows come, in any level's forms.
*/
void
d_tests_sqlite_face_stubs(
    void
)
{
    face::sqlite             db;
    std::vector<std::string> names(3u, "kept?");
    face::text               value;
    face::sqlite_statement   statement;
    face::rows               found;

    std::printf("over the stubs\n");

    D_TESTS_SQLITE_FACE_CHECK(db.open_memory() == D_DB_UNSUPPORTED);
    D_TESTS_SQLITE_FACE_CHECK(!db.is_open());
    D_TESTS_SQLITE_FACE_CHECK(
        db.open_with("t.db", D_SQLITE_OPEN_READONLY) == D_DB_UNSUPPORTED);
    D_TESTS_SQLITE_FACE_CHECK(!db.is_open());
    D_TESTS_SQLITE_FACE_CHECK(db.list_tables(0, names) == D_DB_UNSUPPORTED);
    D_TESTS_SQLITE_FACE_CHECK(names.empty());
    D_TESTS_SQLITE_FACE_CHECK(db.query_text(value, "SELECT 1") ==
                              D_DB_UNSUPPORTED);
    D_TESTS_SQLITE_FACE_CHECK(value.is_null());
    D_TESTS_SQLITE_FACE_CHECK(db.prepare(statement, "SELECT 1") ==
                              D_DB_UNSUPPORTED);
    D_TESTS_SQLITE_FACE_CHECK(!statement.is_prepared());
    D_TESTS_SQLITE_FACE_CHECK(statement.open_cursor() == D_DB_UNSUPPORTED);
    D_TESTS_SQLITE_FACE_CHECK(statement.fetch_rows(1u, found) ==
                              D_DB_UNSUPPORTED);
    D_TESTS_SQLITE_FACE_CHECK(statement.close_cursor() == D_DB_UNSUPPORTED);
    D_TESTS_SQLITE_FACE_CHECK(db.query_rows(found, "SELECT 1") ==
                              D_DB_UNSUPPORTED);
    D_TESTS_SQLITE_FACE_CHECK(found.empty());

#if D_INTERNAL_DB_FACE_OPTIONAL
    std::optional<std::string> maybe(std::string("kept?"));

    D_TESTS_SQLITE_FACE_CHECK(
        (db.query_text(maybe, "SELECT 1") == D_DB_UNSUPPORTED) &&
        (!maybe.has_value()));
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

#if D_INTERNAL_DB_FACE_EXPECTED
    const auto none = db.list_tables(0);

    D_TESTS_SQLITE_FACE_CHECK( (!none.has_value()) &&
                               (none.error() == D_DB_UNSUPPORTED) );
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    {
        face::transaction<face::sqlite> work(db);

        D_TESTS_SQLITE_FACE_CHECK( (work.result() == D_DB_UNSUPPORTED) &&
                                   (!work.is_active()) );
    }

    D_TESTS_SQLITE_FACE_CHECK(db.close() == D_DB_OK);

    return;
}

}  // namespace

/*
main
  The static members; then, over SQLite, a database's worth of members, or,
over the stubs, what the stubs report. argv[1] is the directory for the
file test, "." when it is not given.
*/
int
main(
    int    _argc,
    char** _argv
)
{
    const bool        stubs     = (face::sqlite::library_version_number() ==
                                   0);
    const std::string directory = (_argc > 1) ? _argv[1] : ".";

    d_tests_sqlite_face_static(stubs);

    // over SQLite, or over the stubs
    if (stubs)
    {
        d_tests_sqlite_face_stubs();
    }
    else
    {
        face::sqlite db;

        d_tests_sqlite_face_ownership();
        D_TESTS_SQLITE_FACE_CHECK(db.open_memory() == RESULT_OK);
        d_tests_sqlite_face_conversions(db);
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
        d_tests_sqlite_face_cpp17(db);
#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
        d_tests_sqlite_face_cpp20(db);
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER
#if D_INTERNAL_DB_FACE_EXPECTED
        d_tests_sqlite_face_cpp23(db);
#endif  // D_INTERNAL_DB_FACE_EXPECTED
        d_tests_sqlite_face_statements(db);
    d_tests_sqlite_face_cursors(db);
        d_tests_sqlite_face_modules(db);
        d_tests_sqlite_face_guards(db);
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
        d_tests_sqlite_face_variadic(db);
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER
        d_tests_sqlite_face_files(directory);
    }

    std::printf("%d checks, %d failed (%s, as C++ %ld; forms:%s)\n",
                g_checks,
                g_failures,
                (stubs) ? "built without SQLite"
                        : face::sqlite::library_version(),
                static_cast<long>(__cplusplus),
                (sizeof(D_TESTS_SQLITE_FACE_FORMS) > 1u)
                    ? D_TESTS_SQLITE_FACE_FORMS
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
    std::printf("the SQLite face is absent below the 64-bit floor\n");

    return 0;
}

#endif  // defined(INT64_MAX)
