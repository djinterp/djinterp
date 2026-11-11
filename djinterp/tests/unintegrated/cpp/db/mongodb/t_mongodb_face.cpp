/*******************************************************************************
* djinterp [c]                                                t_mongodb_face.cpp
*
*   The C++ face of the MongoDB layer, mongodb_face.hpp, compiled as C++ at
* every level from ISO strict C++98 to C++26 and driven against a
* deployment. The layer's suite, mongodb_tests.c, tests what each function
* does; this tests what the face adds -- ownership of the handle, a cursor
* and a change stream, a failed connection's reason, string and list and
* text conversion, rows, the transaction guard and its mode, the static
* members, from C++11 moves and the f forms in JSON's dialect, and the forms
* each later level adds where the compiler has them: std::optional text and
* deduced guards (C++17), face::transactional, std::span settings, UTF-8
* strings and formats checked at compile time (C++20), std::expected results
* (C++23) -- and samples every module, each member being generated from its
* C prototype, its arguments in the C order. The summary names the level
* forms the build had.
*   The deployment is the one mongodb_tests.c reaches, by the same setting
* (DJINTERP_TEST_MONGODB_URI). The test works in a database of its own, and
* drops it. Without a deployment only the static checks run, unless
* DJINTERP_TEST_MONGODB_REQUIRED is 1. Over a layer built without the driver
* it checks that the face reports what the stubs do. Below the 64-bit floor
* the face compiles to nothing, and so does this test. Defining
* D_TESTS_MONGODB_FACE_MISUSE as 1 to 6 makes it misuse the face in a way
* that must not compile (see d_tests_mongodb_face_variadic).
*
*   Build: compile the layer's C sources and src/djinterp/c/sync/dmutex.c as
* C, this file as C++, and link them with the driver and threads --
* scripts/ci/check_db_mongodb.sh does, at every level.
*
*
* path:      /tests/unintegrated/cpp/db/mongodb/t_mongodb_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.11.10
*******************************************************************************/
// std
#include <cstdio>   // std::printf
#include <cstdlib>  // std::getenv
#include <cstring>  // std::memcmp, std::memcpy, std::strcmp, std::strstr
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "../../../../../inc/djinterp/cpp/db/mongodb/mongodb_face.hpp"  // under
                                                                       // test
// re_std
#include "../../../../../inc/re_std/cpp/cstdint/cstdint.hpp"  // re_std::int64_t,
                                                         // INT64_MAX


#if defined(INT64_MAX)

namespace face = djinterp::face;

namespace
{

int         g_checks   = 0;
int         g_failures = 0;
const char* g_uri      = "mongodb://127.0.0.1:27017/";

#if ( (D_ENV_LANG_IS_CPP20_OR_HIGHER) &&                                      \
      (D_ENV_CPP_FEATURE_LANG_CONSTINIT) )
// g_unopened
//   a face at namespace scope, constant-initialized: no constructor runs at
// start-up, since an empty face is made at compile time.
constinit face::mongodb g_unopened;
#endif  // C++20 constinit

/*
d_tests_mongodb_face_expect
  Counts one check, and prints it when it fails.
*/
void d_tests_mongodb_face_expect(
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
d_tests_mongodb_face_setting
  An environment variable, or a default when it is unset or empty.
*/
const char* d_tests_mongodb_face_setting(
    const char* _name,
    const char* _default
)
{
    const char* const value = std::getenv(_name);

    return ( (value) && (value[0] != '\0') ) ? value : _default;
}

/*
d_tests_mongodb_face_same
  Whether a C string is the text expected; NULL is no text.
*/
bool d_tests_mongodb_face_same(
    const char* _text,
    const char* _expected
)
{
    return ( (_text) &&
             (std::strcmp(_text, _expected) == 0) );
}

/*
d_tests_mongodb_face_holds
  Whether a C string holds a piece of text; NULL holds none.
*/
bool d_tests_mongodb_face_holds(
    const char* _text,
    const char* _piece
)
{
    return ( (_text) &&
             (std::strstr(_text, _piece) != 0) );
}

/*
d_tests_mongodb_face_row
  A row's document, as the handle's JSON format wrote it; NULL past the
rows, or for a document written as BSON.
*/
const char* d_tests_mongodb_face_row(
    const face::rows& _rows,
    std::size_t       _row
)
{
    const struct dj_db_value* const value = _rows.value(_row, 0u);

    return ( (value) &&
             (value->type == D_DB_VALUE_TEXT) ) ? value->as.text : 0;
}

/*
d_tests_mongodb_face_number
  The digits after a key in canonical Extended JSON, read as text:
"connectionId" : { "$numberInt" : "123" } gives "123"; "0" where the text
has none.
*/
std::string d_tests_mongodb_face_number(
    const char* _json,
    const char* _key
)
{
    std::string       digits;
    const char*       at   = (_json) ? std::strstr(_json, _key) : 0;
    static const char lead[] = "\" : \"";

    at = (at) ? std::strstr(at, lead) : 0;
    at = (at) ? at + (sizeof(lead) - 1u) : "0";

    // the digits, as far as they go
    while ( (*at >= '0') &&
            (*at <= '9') )
    {
        digits += *at++;
    }

    return (digits.empty()) ? std::string("0") : digits;
}

}  // namespace

// D_TESTS_MONGODB_FACE_CHECK
//   macro: records one check with its source text and line, which only the
// preprocessor can supply.
#define D_TESTS_MONGODB_FACE_CHECK(_cond)                                     \
    d_tests_mongodb_face_expect((_cond), #_cond, __LINE__)

// D_TESTS_MONGODB_FACE_FORMS
//   constant: the forms of the levels above C++11 this build has, for the
// summary; empty when it has none.
#define D_TESTS_MONGODB_FACE_FORMS                                            \
    D_TESTS_MONGODB_FACE_FORM_OPTIONAL D_TESTS_MONGODB_FACE_FORM_CONCEPTS     \
    D_TESTS_MONGODB_FACE_FORM_SPAN D_TESTS_MONGODB_FACE_FORM_CHAR8            \
    D_TESTS_MONGODB_FACE_FORM_CHECK D_TESTS_MONGODB_FACE_FORM_EXPECTED        \
    D_TESTS_MONGODB_FACE_FORM_DELETED

#if D_INTERNAL_DB_FACE_OPTIONAL
    #define D_TESTS_MONGODB_FACE_FORM_OPTIONAL " optional"
#else
    #define D_TESTS_MONGODB_FACE_FORM_OPTIONAL ""
#endif
#if D_INTERNAL_DB_FACE_CONCEPTS
    #define D_TESTS_MONGODB_FACE_FORM_CONCEPTS " concepts"
#else
    #define D_TESTS_MONGODB_FACE_FORM_CONCEPTS ""
#endif
#if D_INTERNAL_DB_FACE_SPAN
    #define D_TESTS_MONGODB_FACE_FORM_SPAN " span"
#else
    #define D_TESTS_MONGODB_FACE_FORM_SPAN ""
#endif
#if D_INTERNAL_DB_FACE_CHAR8
    #define D_TESTS_MONGODB_FACE_FORM_CHAR8 " char8_t"
#else
    #define D_TESTS_MONGODB_FACE_FORM_CHAR8 ""
#endif
#if D_INTERNAL_DB_FACE_FORMAT_CHECK
    #define D_TESTS_MONGODB_FACE_FORM_CHECK " checked-formats"
#else
    #define D_TESTS_MONGODB_FACE_FORM_CHECK ""
#endif
#if D_INTERNAL_DB_FACE_EXPECTED
    #define D_TESTS_MONGODB_FACE_FORM_EXPECTED " expected"
#else
    #define D_TESTS_MONGODB_FACE_FORM_EXPECTED ""
#endif
#if ( (D_ENV_LANG_IS_CPP26_OR_HIGHER) &&                                      \
      (D_ENV_CPP_FEATURE_LANG_DELETED_FUNCTION) )
    #define D_TESTS_MONGODB_FACE_FORM_DELETED " deleted-reasons"
#else
    #define D_TESTS_MONGODB_FACE_FORM_DELETED ""
#endif

namespace
{

/*
d_tests_mongodb_face_static
  The members that need no handle: results, the driver's versions, and its
log, whose level is put back as it was.
*/
void d_tests_mongodb_face_static(
    bool _stubs
)
{
    const int level = face::mongodb::get_log_level();

    std::printf("static members\n");

    D_TESTS_MONGODB_FACE_CHECK(
        (face::mongodb::client_version_number() == 0ul) == _stubs);
    D_TESTS_MONGODB_FACE_CHECK(
        (face::mongodb::client_version()[0] == '\0') == _stubs);
    D_TESTS_MONGODB_FACE_CHECK(
        (face::mongodb::bson_version()[0] == '\0') == _stubs);
    D_TESTS_MONGODB_FACE_CHECK(
        !face::mongodb::result_is_client(D_MONGODB_ERROR_DUPLICATE_KEY));
    D_TESTS_MONGODB_FACE_CHECK(
        face::mongodb::result_domain(D_MONGODB_LAYER_NO_MEMORY) ==
        static_cast<unsigned>(D_MONGODB_DOMAIN_LAYER));
    D_TESTS_MONGODB_FACE_CHECK(
        face::mongodb::result_code(D_MONGODB_ERROR_NAMESPACE_NOT_FOUND) ==
        26u);
    D_TESTS_MONGODB_FACE_CHECK(
        d_tests_mongodb_face_holds(
            face::mongodb::result_message(D_DB_UNSUPPORTED),
            (_stubs) ? "no driver" : "cannot do this"));
    D_TESTS_MONGODB_FACE_CHECK(
        face::mongodb::set_log_level(level) ==
        ((_stubs) ? D_DB_UNSUPPORTED : 0));
    D_TESTS_MONGODB_FACE_CHECK(face::mongodb::get_log_level() == level);
    D_TESTS_MONGODB_FACE_CHECK(face::mongodb::set_log_level(9) ==
                               ((_stubs) ? D_DB_UNSUPPORTED
                                         : D_DB_INVALID_ARGUMENT));

    return;
}

/*
d_tests_mongodb_face_ownership
  Connecting by a string, by options and by settings, failing to with a
reason, swapping, giving up, adopting and replacing a handle; from C++11,
moving it; from C++20, a face constant-initialized and settings as a span.
*/
void d_tests_mongodb_face_ownership(void)
{
    const struct dj_mongodb_options where   = { 0, 0, 0, 0, 0u, 0, 0, g_uri };
    const struct dj_mongodb_setting named[] =
    {
        { D_MONGODB_SETTING_APP_NAME, 0ul, "d_tests_face", 0 },
        { D_MONGODB_SETTING_CONNECT_TIMEOUT, 5000ul, 0, 0 }
    };
    const struct dj_mongodb_setting broken[] =
    {
        { D_MONGODB_SETTING_URI_OPTION, 0ul, "readPreference", "nonesuch" }
    };
    face::mongodb                  db;
    face::mongodb                  other;
    face::text                     why;

    std::printf("ownership\n");

#if ( (D_ENV_LANG_IS_CPP20_OR_HIGHER) &&                                      \
      (D_ENV_CPP_FEATURE_LANG_CONSTINIT) )
    D_TESTS_MONGODB_FACE_CHECK(!g_unopened.is_open());
#endif  // C++20 constinit

    D_TESTS_MONGODB_FACE_CHECK(!db.is_open());
    D_TESTS_MONGODB_FACE_CHECK(db.connect_uri(std::string(g_uri)) == 0);
    D_TESTS_MONGODB_FACE_CHECK(db.is_open());

    struct dj_mongodb* const first = db.handle();

    // a failed connection leaves the handle as it was, and says why
    D_TESTS_MONGODB_FACE_CHECK(db.connect_with(&where, broken, why) != 0);
    D_TESTS_MONGODB_FACE_CHECK( (!why.is_null()) &&
                                (std::strlen(why.c_str()) > 0u) );
    D_TESTS_MONGODB_FACE_CHECK(db.handle() == first);
    D_TESTS_MONGODB_FACE_CHECK(db.connect_uri("not a uri") != 0);
    D_TESTS_MONGODB_FACE_CHECK(db.handle() == first);

    // options, settings as an array, as a pointer and a count, and from
    // C++20 as a span
    D_TESTS_MONGODB_FACE_CHECK(other.connect(&where) == 0);
    D_TESTS_MONGODB_FACE_CHECK(other.connect_with(&where, named) == 0);
    D_TESTS_MONGODB_FACE_CHECK( (other.connect_with(&where, named, 2u,
                                                    why) == 0) &&
                                (why.is_null()) );
#if D_INTERNAL_DB_FACE_SPAN
    D_TESTS_MONGODB_FACE_CHECK(
        other.connect_with(&where,
                           std::span<const struct dj_mongodb_setting>(named)) ==
        0);
#endif  // D_INTERNAL_DB_FACE_SPAN
    D_TESTS_MONGODB_FACE_CHECK(other.ping() == 0);
    other.swap(db);
    D_TESTS_MONGODB_FACE_CHECK(other.handle() == first);
    swap(db, other);
    D_TESTS_MONGODB_FACE_CHECK(db.handle() == first);

    // a client of the driver's, adopted and not owned
    {
        face::mongodb borrowed;

        D_TESTS_MONGODB_FACE_CHECK(borrowed.adopt(db.native(), false) == 0);
        D_TESTS_MONGODB_FACE_CHECK(borrowed.ping() == 0);
    }

    D_TESTS_MONGODB_FACE_CHECK(db.ping() == 0);

    struct dj_mongodb* const raw = db.release();

    D_TESTS_MONGODB_FACE_CHECK( (raw == first) && (!db.is_open()) );

    face::mongodb adopted(raw);

    D_TESTS_MONGODB_FACE_CHECK(adopted.exec("admin", "{\"ping\": 1}") == 0);

#if D_MOVE_ENABLED
    face::mongodb moved(static_cast<face::mongodb&&>(adopted));

    D_TESTS_MONGODB_FACE_CHECK( (!adopted.is_open()) &&
                                (moved.handle() == raw) );
    D_TESTS_MONGODB_FACE_CHECK(static_cast<bool>(moved));
    adopted = static_cast<face::mongodb&&>(moved);
    D_TESTS_MONGODB_FACE_CHECK(adopted.handle() == raw);
#endif  // D_MOVE_ENABLED

    adopted.reset();
    D_TESTS_MONGODB_FACE_CHECK(!adopted.is_open());
    D_TESTS_MONGODB_FACE_CHECK(adopted.close() == 0);
    D_TESTS_MONGODB_FACE_CHECK(adopted.exec("admin", "{\"ping\": 1}") ==
                               D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_FACE_CHECK(!adopted.has_collection(0, "anything"));

    return;
}

/*
d_tests_mongodb_face_conversions
  string_arg from a C string, a std::string and NULL, the current
database; a name list and a vector; text, a std::string and the numbers a
member writes; rows read by place.
*/
void d_tests_mongodb_face_conversions(
    face::mongodb&     _db,
    const std::string& _name
)
{
    face::name_list               names;
    std::vector<std::string>      copied;
    face::text                    id;
    std::string                   plain("unchanged");
    face::rows                    found;
    struct dj_mongodb_write_counts counts = { 0, 0, 0, 0, 0 };
    re_std::int64_t               count  = -1;
    bool                          exists = false;

    std::printf("conversions\n");

    D_TESTS_MONGODB_FACE_CHECK(
        _db.insert_one(0, std::string("notes"), "{\"_id\": 1, \"b\": \"x\"}",
                       id) == 0);
    D_TESTS_MONGODB_FACE_CHECK(d_tests_mongodb_face_same(id.c_str(),
                                                         "{ \"$numberInt\" "
                                                         ": \"1\" }"));
    D_TESTS_MONGODB_FACE_CHECK(
        (_db.insert_one(_name, "notes", "{\"b\": \"y\"}", plain) == 0) &&
        (plain.find("$oid") != std::string::npos));
    D_TESTS_MONGODB_FACE_CHECK(
        (_db.last_insert_id(id) == 0) &&
        (d_tests_mongodb_face_same(id.c_str(), plain.c_str())));
    D_TESTS_MONGODB_FACE_CHECK(
        (_db.insert_many(0, "notes", "[{\"b\": \"z\"}, {\"b\": \"w\"}]", 0u,
                         count) == 0) &&
        (count == 2));
    D_TESTS_MONGODB_FACE_CHECK(
        (_db.update_many(0, "notes", "{\"b\": {\"$in\": [\"z\", \"w\"]}}",
                         "{\"$set\": {\"late\": true}}", 0, counts) == 0) &&
        (counts.matched == 2) && (counts.modified == 2));
    D_TESTS_MONGODB_FACE_CHECK(
        (_db.count_documents(0, "notes", 0, 0, count) == 0) && (count == 4));
    D_TESTS_MONGODB_FACE_CHECK(_db.has_collection(0, "notes"));
    D_TESTS_MONGODB_FACE_CHECK(
        (_db.has_collection_s(_name, "notes", exists) == 0) && (exists));
    D_TESTS_MONGODB_FACE_CHECK(_db.create_collection(0, "people", 0, 0u) ==
                               0);
    D_TESTS_MONGODB_FACE_CHECK(_db.list_collections(0, names) == 0);
    D_TESTS_MONGODB_FACE_CHECK( (names.size() == 2u)          &&
                                (names.contains("notes"))     &&
                                (names.contains(std::string("people"))) );
    D_TESTS_MONGODB_FACE_CHECK(_db.list_collections(_name, copied) == 0);
    D_TESTS_MONGODB_FACE_CHECK(copied.size() == 2u);
    D_TESTS_MONGODB_FACE_CHECK(
        _db.find(0, "notes", "{\"_id\": 1}", 0, found) == 0);
    D_TESTS_MONGODB_FACE_CHECK(
        (found.row_count() == 1u) && (found.column_count() == 1u) &&
        (found.column(0u).type == D_DB_TYPE_JSON) &&
        (d_tests_mongodb_face_same(
             d_tests_mongodb_face_row(found, 0u),
             "{ \"_id\" : { \"$numberInt\" : \"1\" }, \"b\" : \"x\" }")));
    D_TESTS_MONGODB_FACE_CHECK(
        (_db.current_database(id) == 0) &&
        (d_tests_mongodb_face_same(id.c_str(), _name.c_str())));
    D_TESTS_MONGODB_FACE_CHECK(_db.last_insert_id(plain) == D_DB_NOT_FOUND);
    D_TESTS_MONGODB_FACE_CHECK(_db.changes() == 2);

    return;
}

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

/*
d_tests_mongodb_face_cpp17
  C++17's forms: text as a std::optional<std::string>, empty for a failure;
and the transaction guard deducing its face, where the deployment has
transactions.
*/
void d_tests_mongodb_face_cpp17(
    face::mongodb&     _db,
    const std::string& _name
)
{
    std::printf("C++17 forms\n");

#if D_INTERNAL_DB_FACE_OPTIONAL
    std::optional<std::string> current;
    std::optional<std::string> reply(std::string("stale"));

    D_TESTS_MONGODB_FACE_CHECK(
        (_db.current_database(current) == 0) && (current == _name));
    D_TESTS_MONGODB_FACE_CHECK(
        _db.command(0, reply, "{\"noSuchCommand\": 1}") ==
        D_MONGODB_ERROR_COMMAND_NOT_FOUND);
    D_TESTS_MONGODB_FACE_CHECK(!reply.has_value());
    D_TESTS_MONGODB_FACE_CHECK(
        (_db.command(0, reply, "{\"ping\": 1}") == 0) &&
        (reply.has_value()));
#else
    (void)_name;
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

#if D_ENV_CPP_FEATURE_LANG_DEDUCTION_GUIDES
    // a transaction where there are any
    if (_db.supports(D_MONGODB_FEATURE_TRANSACTIONS))
    {
        face::transaction work(_db);

        D_TESTS_MONGODB_FACE_CHECK(work.is_active());
        D_TESTS_MONGODB_FACE_CHECK(work.rollback() == 0);
    }

    D_TESTS_MONGODB_FACE_CHECK(!_db.in_transaction());
#endif  // D_ENV_CPP_FEATURE_LANG_DEDUCTION_GUIDES

    return;
}

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#if D_INTERNAL_DB_FACE_CONCEPTS

/*
d_tests_mongodb_face_generic
  Code written for any face with transactions, through
face::transactional: a transaction begun and rolled back, its guard
deduced.
*/
template<face::transactional Database>
bool d_tests_mongodb_face_generic(
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
d_tests_mongodb_face_cpp20
  C++20's forms: the face as a face::transactional, and not a
face::connection, having no savepoints; UTF-8 strings; and a format read at
compile time, in JSON's dialect.
*/
void d_tests_mongodb_face_cpp20(
    face::mongodb& _db
)
{
    re_std::int64_t changes = -1;

    std::printf("C++20 forms\n");

#if D_INTERNAL_DB_FACE_CONCEPTS
    static_assert(face::transactional<face::mongodb>,
                  "face::mongodb is a face::transactional");
    static_assert(!face::connection<face::mongodb>,
                  "face::mongodb has no savepoints");

    // a transaction where there are any
    if (_db.supports(D_MONGODB_FEATURE_TRANSACTIONS))
    {
        D_TESTS_MONGODB_FACE_CHECK(d_tests_mongodb_face_generic(_db));
    }
#endif  // D_INTERNAL_DB_FACE_CONCEPTS

#if D_INTERNAL_DB_FACE_CHAR8
    const std::u8string name(u8"notes");

    D_TESTS_MONGODB_FACE_CHECK(_db.has_collection(0, u8"notes"));
    D_TESTS_MONGODB_FACE_CHECK(_db.has_collection(0, name));
#endif  // D_INTERNAL_DB_FACE_CHAR8

#if D_INTERNAL_DB_FACE_FORMAT_CHECK
    // a format checked at compile time is a pointer to the literal
    constexpr face::mongodb::format<const char*, int> both(
        "{\"insert\": %I, \"documents\": [{\"n\": %d}]}");

    static_assert(sizeof(both) == sizeof(const char*),
                  "a checked format is its pointer");
    D_TESTS_MONGODB_FACE_CHECK(
        (_db.execf_changes(0, changes, both, "c20", 1) == 0) &&
        (changes == 1));
#endif  // D_INTERNAL_DB_FACE_FORMAT_CHECK

    // the same calls compile at every level from C++11; from C++20 each
    // literal is read against its arguments first
    D_TESTS_MONGODB_FACE_CHECK(
        (_db.execf_changes(0, changes,
                           "{\"insert\": %Q, \"documents\": [{\"n\": %lld, "
                           "\"s\": %Q}]}",
                           "c20", static_cast<long long>(2), nullptr) ==
         0) &&
        (changes == 1));

    return;
}

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#if D_INTERNAL_DB_FACE_EXPECTED

/*
d_tests_mongodb_face_cpp23
  C++23's forms: a result written through a parameter, returned instead,
as a std::expected that holds the value or the layer's result. Over the
notes d_tests_mongodb_face_conversions made.
*/
void d_tests_mongodb_face_cpp23(
    face::mongodb&     _db,
    const std::string& _name
)
{
    std::printf("C++23 forms\n");

    const auto id      = _db.insert_one(0, "notes", "{\"_id\": 23}");
    const auto twice   = _db.insert_one(0, "notes", "{\"_id\": 23}");
    const auto count   = _db.count_documents(0, "notes", 0, 0);
    const auto names   = _db.list_collections(0);
    const auto found   = _db.find(0, "notes", "{\"_id\": 23}", 0);
    const auto exists  = _db.has_collection_s(0, "notes");
    const auto current = _db.current_database();
    const auto nothing = face::mongodb().count_documents(0, "notes", 0, 0);

    D_TESTS_MONGODB_FACE_CHECK( (id.has_value()) &&
                                (d_tests_mongodb_face_holds(id->c_str(),
                                                            "23")) );
    D_TESTS_MONGODB_FACE_CHECK( (!twice.has_value()) &&
                                (twice.error() ==
                                 D_MONGODB_ERROR_DUPLICATE_KEY) );
    D_TESTS_MONGODB_FACE_CHECK( (count.has_value()) && (*count == 5) );
    D_TESTS_MONGODB_FACE_CHECK( (names.has_value()) &&
                                (names->contains("notes")) );
    D_TESTS_MONGODB_FACE_CHECK( (found.has_value()) &&
                                (found->row_count() == 1u) );
    D_TESTS_MONGODB_FACE_CHECK( (exists.has_value()) && (*exists) );
    D_TESTS_MONGODB_FACE_CHECK(
        (current.has_value()) &&
        (d_tests_mongodb_face_same(current->c_str(), _name.c_str())));
    D_TESTS_MONGODB_FACE_CHECK( (!nothing.has_value()) &&
                                (nothing.error() == D_DB_INVALID_ARGUMENT) );

    // a cursor returned, which moves
    auto cursor = _db.find_cursor(0, "notes", 0, "{\"batchSize\": 2}");

    D_TESTS_MONGODB_FACE_CHECK( (cursor.has_value()) &&
                                (cursor->is_open()) );

    // its first page
    if (cursor.has_value())
    {
        const auto page = cursor->fetch(3u);

        D_TESTS_MONGODB_FACE_CHECK( (page.has_value()) &&
                                    (page->row_count() == 3u) );
    }

    return;
}

#endif  // D_INTERNAL_DB_FACE_EXPECTED

/*
d_tests_mongodb_face_cursors
  A cursor opened into a face and read a page at a time to its end; closed
by hand, and by its scope; handed over and moved; and a change stream,
where the deployment has them, whose event and resume token come through
the face.
*/
void d_tests_mongodb_face_cursors(
    face::mongodb& _db
)
{
    face::mongodb_cursor cursor;
    face::mongodb_cursor other;
    face::rows           page;
    std::size_t          seen = 0u;

    std::printf("cursors\n");

    D_TESTS_MONGODB_FACE_CHECK(!cursor.is_open());
    D_TESTS_MONGODB_FACE_CHECK(cursor.is_done());
    D_TESTS_MONGODB_FACE_CHECK(
        _db.find_cursor(0, "notes", 0, "{\"batchSize\": 1}", cursor) == 0);
    D_TESTS_MONGODB_FACE_CHECK( (cursor.is_open()) && (!cursor.is_done()) );

    // every page, to the short one
    while ( (!cursor.is_done()) &&
            (cursor.fetch(2u, page) == 0) )
    {
        seen += page.row_count();
    }

    D_TESTS_MONGODB_FACE_CHECK(seen == 4u);
    D_TESTS_MONGODB_FACE_CHECK(cursor.close() == 0);
    D_TESTS_MONGODB_FACE_CHECK(!cursor.is_open());
    D_TESTS_MONGODB_FACE_CHECK(cursor.fetch(1u, page) ==
                               D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_FACE_CHECK(
        _db.aggregate_cursor(0, "notes", "[{\"$match\": {}}]", 0, cursor) ==
        0);
    other.swap(cursor);
    D_TESTS_MONGODB_FACE_CHECK( (other.is_open()) && (!cursor.is_open()) );
    swap(cursor, other);

    struct dj_mongodb_cursor* const raw = cursor.release();

    D_TESTS_MONGODB_FACE_CHECK(!cursor.is_open());

    face::mongodb_cursor adopted(raw);

    D_TESTS_MONGODB_FACE_CHECK( (adopted.fetch(1u, page) == 0) &&
                                (page.row_count() == 1u) );

#if D_MOVE_ENABLED
    face::mongodb_cursor moved(static_cast<face::mongodb_cursor&&>(adopted));

    D_TESTS_MONGODB_FACE_CHECK( (!adopted.is_open()) &&
                                (moved.handle() == raw) );
    D_TESTS_MONGODB_FACE_CHECK(static_cast<bool>(moved));
#endif  // D_MOVE_ENABLED

    D_TESTS_MONGODB_FACE_CHECK(
        _db.command_cursor(0, "{\"listCollections\": 1}", other) == 0);
    D_TESTS_MONGODB_FACE_CHECK(
        _db.command_cursor(0, "{\"ping\": 1}", other) ==
        D_DB_INVALID_ARGUMENT);
    D_TESTS_MONGODB_FACE_CHECK(!other.is_open());

    // a change stream, where there are any
    if (_db.supports(D_MONGODB_FEATURE_CHANGE_STREAMS))
    {
        face::mongodb_change_stream stream;
        face::rows                  events;
        std::string                 token;

        D_TESTS_MONGODB_FACE_CHECK(
            _db.watch_collection(0, "watched", 0,
                                 "{\"maxAwaitTimeMS\": 1000}", stream) ==
            0);
        D_TESTS_MONGODB_FACE_CHECK(stream.is_open());
        D_TESTS_MONGODB_FACE_CHECK(
            _db.insert_one(0, "watched", "{\"seen\": 1}", token) == 0);

        // the event, within a few waits
        for (int wait = 0; (wait < 10) && (events.empty()); ++wait)
        {
            D_TESTS_MONGODB_FACE_CHECK(stream.fetch(1u, events) == 0);
        }

        D_TESTS_MONGODB_FACE_CHECK(
            d_tests_mongodb_face_holds(d_tests_mongodb_face_row(events, 0u),
                                       "\"insert\""));
        D_TESTS_MONGODB_FACE_CHECK(
            (stream.resume_token(token) == 0) && (!token.empty()));
        D_TESTS_MONGODB_FACE_CHECK(stream.close() == 0);
    }

    return;
}

/*
d_tests_mongodb_face_modules
  A member of every module beside those above: commands, documents,
settings, schema, GridFS, maintenance and databases, each its C function
called through the face.
*/
void d_tests_mongodb_face_modules(
    face::mongodb&     _db,
    const std::string& _name
)
{
    static const char             bytes[] = "face bytes";
    face::text                    reply;
    std::string                   concern;
    std::vector<std::string>      indexes;
    face::rows                    found;
    struct dj_mongodb_write_counts counts  = { 0, 0, 0, 0, 0 };
    re_std::int64_t               deleted = -1;
    re_std::int64_t               size    = -1;
    void*                         data    = 0;
    std::size_t                   length  = 0u;
    bool                          valid   = false;

    std::printf("modules\n");

    // commands, and documents
    D_TESTS_MONGODB_FACE_CHECK(
        (_db.command("admin", reply, "{\"ping\": 1}") == 0) &&
        (d_tests_mongodb_face_holds(reply.c_str(), "\"ok\"")));
    D_TESTS_MONGODB_FACE_CHECK(
        (_db.query_rows(0, found, "{\"find\": \"notes\"}") == 0) &&
        (found.row_count() == 4u));
    D_TESTS_MONGODB_FACE_CHECK(
        (_db.distinct(0, "notes", "b", 0, 0, found) == 0) &&
        (found.row_count() == 4u));
    D_TESTS_MONGODB_FACE_CHECK(
        (_db.find_one_and_update(0, "notes", "{\"_id\": 1}",
                                 "{\"$set\": {\"b\": \"v\"}}",
                                 "{\"returnDocument\": \"after\"}",
                                 found) == 0) &&
        (d_tests_mongodb_face_holds(d_tests_mongodb_face_row(found, 0u),
                                    "\"v\"")));
    D_TESTS_MONGODB_FACE_CHECK(
        (_db.bulk_write(0, "bulk",
                        "[{\"insertOne\": {\"document\": {\"k\": 1}}}, "
                        "{\"insertOne\": {\"document\": {\"k\": 2}}}]",
                        D_MONGODB_UNORDERED, 0, counts) == 0) &&
        (counts.inserted == 2));
    D_TESTS_MONGODB_FACE_CHECK(
        (_db.delete_many(0, "bulk", "{}", 0, deleted) == 0) &&
        (deleted == 2));

    // settings
    D_TESTS_MONGODB_FACE_CHECK(_db.set_write_concern("{\"w\": 1}") == 0);
    D_TESTS_MONGODB_FACE_CHECK(
        (_db.get_write_concern(concern) == 0) &&
        (concern.find("\"w\"") != std::string::npos));
    D_TESTS_MONGODB_FACE_CHECK(_db.set_max_time_ms(60000) == 0);
    D_TESTS_MONGODB_FACE_CHECK(_db.get_max_time_ms() == 60000);
    D_TESTS_MONGODB_FACE_CHECK(_db.set_max_time_ms(0) == 0);
    D_TESTS_MONGODB_FACE_CHECK(
        _db.set_document_format(D_MONGODB_JSON_RELAXED) == 0);
    D_TESTS_MONGODB_FACE_CHECK(_db.get_document_format() ==
                               D_MONGODB_JSON_RELAXED);
    D_TESTS_MONGODB_FACE_CHECK(
        _db.set_document_format(D_MONGODB_JSON_CANONICAL) == 0);

    // schema
    D_TESTS_MONGODB_FACE_CHECK(
        _db.create_index(0, "notes", "by_b", "{\"b\": 1}", 0, 0u) == 0);
    D_TESTS_MONGODB_FACE_CHECK(_db.has_index(0, "notes", "by_b"));
    D_TESTS_MONGODB_FACE_CHECK(
        (_db.list_indexes(0, "notes", indexes) == 0) &&
        (indexes.size() == 2u));
    D_TESTS_MONGODB_FACE_CHECK(
        (_db.describe_indexes(0, "notes", found) == 0) &&
        (found.row_count() == 2u));
    D_TESTS_MONGODB_FACE_CHECK(
        _db.create_view(0, "late", "notes", "[{\"$match\": {\"late\": "
                        "true}}]", 0, 0u) == 0);
    D_TESTS_MONGODB_FACE_CHECK(_db.has_view(0, "late"));
    D_TESTS_MONGODB_FACE_CHECK(_db.drop_view(0, "late", 0u) == 0);
    D_TESTS_MONGODB_FACE_CHECK(
        (_db.collection_info(0, "notes", reply) == 0) &&
        (d_tests_mongodb_face_holds(reply.c_str(), "\"notes\"")));

    // GridFS
    D_TESTS_MONGODB_FACE_CHECK(
        _db.gridfs_upload(0, 0, "f", bytes, sizeof(bytes), 0, reply) == 0);
    D_TESTS_MONGODB_FACE_CHECK(
        (_db.gridfs_download(0, 0, reply.c_str(), data, length) == 0) &&
        (length == sizeof(bytes)) &&
        (std::memcmp(data, bytes, sizeof(bytes)) == 0));
    d_db_free(data);
    D_TESTS_MONGODB_FACE_CHECK(
        (_db.gridfs_find_id(0, 0, "f", concern) == 0) &&
        (concern == reply.c_str()));
    D_TESTS_MONGODB_FACE_CHECK(_db.gridfs_delete(0, 0, reply.c_str()) == 0);

    // maintenance, and databases
    D_TESTS_MONGODB_FACE_CHECK(
        (_db.collection_size(0, "notes", size) == 0) && (size > 0));
    D_TESTS_MONGODB_FACE_CHECK(
        (_db.validate(0, "notes", 0u, valid, reply) == 0) && (valid) &&
        (!reply.is_null()));
    D_TESTS_MONGODB_FACE_CHECK(_db.has_database(_name));
    D_TESTS_MONGODB_FACE_CHECK(
        (_db.list_databases(indexes) == 0) && (!indexes.empty()));

    return;
}

/*
d_tests_mongodb_face_guards
  The transaction guard, where the deployment has transactions: committed;
rolled back when its scope ends; begun in a mode, its options; and one
whose transaction the server failed, which its scope ends.
*/
void d_tests_mongodb_face_guards(
    face::mongodb& _db
)
{
    re_std::int64_t count = -1;

    std::printf("guards\n");

    // transactions to guard
    if (!_db.supports(D_MONGODB_FEATURE_TRANSACTIONS))
    {
        std::printf("  no transactions here; the guards skipped\n");

        return;
    }

    D_TESTS_MONGODB_FACE_CHECK(_db.create_collection(0, "g", 0, 0u) == 0);

    {
        face::transaction<face::mongodb> work(_db);

        D_TESTS_MONGODB_FACE_CHECK( (work.result() == 0) &&
                                    (_db.in_transaction()) );
        D_TESTS_MONGODB_FACE_CHECK(
            _db.exec(0, "{\"insert\": \"g\", \"documents\": [{\"_id\": "
                        "1}]}") == 0);
        D_TESTS_MONGODB_FACE_CHECK(work.commit() == 0);
        D_TESTS_MONGODB_FACE_CHECK(!work.is_active());
    }

    {
        face::transaction<face::mongodb> work(
            _db, "{\"readConcern\": {\"level\": \"snapshot\"}}");

        D_TESTS_MONGODB_FACE_CHECK(work.is_active());
        D_TESTS_MONGODB_FACE_CHECK(
            _db.exec(0, "{\"insert\": \"g\", \"documents\": [{\"_id\": "
                        "2}]}") == 0);
    }

    D_TESTS_MONGODB_FACE_CHECK(!_db.in_transaction());

    {
        face::transaction<face::mongodb> work(_db);

        D_TESTS_MONGODB_FACE_CHECK(
            _db.exec(0, "{\"insert\": \"g\", \"documents\": [{\"_id\": "
                        "1}]}") == D_MONGODB_ERROR_DUPLICATE_KEY);
        D_TESTS_MONGODB_FACE_CHECK(_db.in_failed_transaction());
        D_TESTS_MONGODB_FACE_CHECK(work.commit() != 0);
    }

    D_TESTS_MONGODB_FACE_CHECK(!_db.in_transaction());

    {
        face::transaction<face::mongodb> refused(_db, "{\"frob\": 1}");

        D_TESTS_MONGODB_FACE_CHECK( (refused.result() ==
                                     D_DB_INVALID_ARGUMENT) &&
                                    (!refused.is_active()) );
    }

    D_TESTS_MONGODB_FACE_CHECK(
        (_db.count_documents(0, "g", 0, 0, count) == 0) && (count == 1));

    return;
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

/*
d_tests_mongodb_face_variadic
  The f forms, from C++11, in JSON's dialect: values written into a
command, read back through each kind of result; and a format built at run
time, through face::runtime_format.
*/
void d_tests_mongodb_face_variadic(
    face::mongodb&     _db,
    const std::string& _name
)
{
    const std::string built("{\"count\": \"f\"}");
    re_std::int64_t   changed = 0;
    face::text        reply;
    face::rows        found;

    std::printf("f forms\n");

    D_TESTS_MONGODB_FACE_CHECK(
        (_db.execf_changes(0, changed,
                           "{\"insert\": %I, \"documents\": [{\"n\": %d, "
                           "\"t\": %Q}, {\"n\": %lld, \"t\": %Q}]}",
                           "f", 1, "it's \"quoted\"",
                           static_cast<long long>(2), nullptr) == 0) &&
        (changed == 2));
    D_TESTS_MONGODB_FACE_CHECK(
        _db.execf(_name, "{\"update\": %Q, \"updates\": [{\"q\": {\"n\": "
                         "%u}, \"u\": {\"$set\": {\"r\": %g}}}]}",
                  "f", 2u, 0.5) == 0);
    D_TESTS_MONGODB_FACE_CHECK(
        (_db.query_rowsf(0, found, "{\"find\": %Q, \"filter\": {\"n\": "
                                   "%d}}", "f", 1) == 0) &&
        (d_tests_mongodb_face_holds(d_tests_mongodb_face_row(found, 0u),
                                    "\"it's \\\"quoted\\\"\"")));
    D_TESTS_MONGODB_FACE_CHECK(
        (_db.query_rowsf(0, found, "{\"find\": %Q, \"filter\": {\"t\": "
                                   "null}}", "f") == 0) &&
        (found.row_count() == 1u) &&
        (d_tests_mongodb_face_holds(d_tests_mongodb_face_row(found, 0u),
                                    "\"r\" : { \"$numberDouble\" : "
                                    "\"0.5\" }")));
    D_TESTS_MONGODB_FACE_CHECK(
        (_db.commandf(0, reply, face::runtime_format(built)) == 0) &&
        (d_tests_mongodb_face_holds(reply.c_str(),
                                    "\"n\" : { \"$numberInt\" : \"2\" }")));
    D_TESTS_MONGODB_FACE_CHECK(
        _db.commandf(0, reply, face::runtime_format("{\"count\": %T}"),
                     "a", "b") == D_DB_INVALID_ARGUMENT);

#if defined(D_TESTS_MONGODB_FACE_MISUSE)
    // must not compile, each refused by the face, in its own words where it
    // has them (scripts/ci/check_db_mongodb.sh runs each from the level that
    // refuses it, and expects those words):
    //   1  a std::string through C's `...` (C++11): "an f form takes
    //      numbers"
    //   2  a literal format its argument does not match (C++20):
    //      "the_format_argument_does_not_match_its_conversion"
    //   3  a std::string as a format (C++11), which goes through
    //      runtime_format: no form takes it
    //   4  a guard made and dropped at once, which begins and ends at once
    //      (C++17): its warning, made an error
    //   5  %T, which JSON's dialect has not (C++20):
    //      "the_format_has_a_conversion_its_dialect_lacks"
    //   6  a savepoint guard, which a server without savepoints cannot
    //      have (C++20): "face::savepoint guards a face"
    #if (D_TESTS_MONGODB_FACE_MISUSE == 1)
        D_TESTS_MONGODB_FACE_CHECK(_db.execf(0, "{\"drop\": %Q}", _name) ==
                                   0);
    #elif (D_TESTS_MONGODB_FACE_MISUSE == 2)
        D_TESTS_MONGODB_FACE_CHECK(_db.execf(0, "{\"n\": %d}", "two") == 0);
    #elif (D_TESTS_MONGODB_FACE_MISUSE == 3)
        D_TESTS_MONGODB_FACE_CHECK(_db.execf(0, built) == 0);
    #elif (D_TESTS_MONGODB_FACE_MISUSE == 4)
        face::transaction<face::mongodb>{_db};
    #elif (D_TESTS_MONGODB_FACE_MISUSE == 5)
        D_TESTS_MONGODB_FACE_CHECK(_db.execf(0, "{\"drop\": %T}", "a",
                                             "b") == 0);
    #elif (D_TESTS_MONGODB_FACE_MISUSE == 6)
        face::savepoint<face::mongodb> step(_db, "step");
    #endif
#endif  // defined(D_TESTS_MONGODB_FACE_MISUSE)

    return;
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

/*
d_tests_mongodb_face_session
  A database of the run's own, named after the server's number for the
connection, made current, worked in, and dropped with what it holds.
*/
void d_tests_mongodb_face_session(void)
{
    face::mongodb   db;
    face::text      hello;
    re_std::int64_t deleted = 0;

    D_TESTS_MONGODB_FACE_CHECK(db.connect_uri(g_uri) == 0);

    // the server's number for the connection, which names the database
    if (db.command("admin", hello, "{\"hello\": 1}") != 0)
    {
        (void)db.command("admin", hello, "{\"isMaster\": 1}");
    }

    const std::string name = "dtmface" +
                             d_tests_mongodb_face_number(hello.c_str(),
                                                         "\"connectionId\"");

    D_TESTS_MONGODB_FACE_CHECK(db.use_database(name) == 0);
    d_tests_mongodb_face_conversions(db, name);
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    d_tests_mongodb_face_cpp17(db, name);
#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    d_tests_mongodb_face_cpp20(db);
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER
    d_tests_mongodb_face_cursors(db);
#if D_INTERNAL_DB_FACE_EXPECTED
    d_tests_mongodb_face_cpp23(db, name);
#endif  // D_INTERNAL_DB_FACE_EXPECTED
    // what C++23's forms inserted, where they ran
    D_TESTS_MONGODB_FACE_CHECK(
        (db.delete_one(0, "notes", "{\"_id\": 23}", 0, deleted) == 0) &&
        (deleted == ((D_INTERNAL_DB_FACE_EXPECTED) ? 1 : 0)));
    d_tests_mongodb_face_modules(db, name);
    d_tests_mongodb_face_guards(db);
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    d_tests_mongodb_face_variadic(db, name);
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER
    D_TESTS_MONGODB_FACE_CHECK(db.drop_database(name, 0u) == 0);

    return;
}

/*
d_tests_mongodb_face_stubs
  The face over the stubs: nothing connects, nothing is listed, no guard
begins, no text, cursor, stream or rows come, in any level's forms.
*/
void d_tests_mongodb_face_stubs(void)
{
    const struct dj_mongodb_options where   = { 0, 0, 0, 0, 0u, 0, 0, g_uri };
    const struct dj_mongodb_setting named[] =
    {
        { D_MONGODB_SETTING_APP_NAME, 0ul, "d_tests_face", 0 }
    };
    face::mongodb                  db;
    std::vector<std::string>       names(3u, "kept?");
    face::text                     value;
    face::mongodb_cursor           cursor;
    face::mongodb_change_stream    stream;
    face::rows                     found;

    std::printf("over the stubs\n");

    D_TESTS_MONGODB_FACE_CHECK(db.connect(&where) == D_DB_UNSUPPORTED);
    D_TESTS_MONGODB_FACE_CHECK(db.connect_uri(g_uri) == D_DB_UNSUPPORTED);
    D_TESTS_MONGODB_FACE_CHECK(db.connect_with(&where, named, value) ==
                               D_DB_UNSUPPORTED);
    D_TESTS_MONGODB_FACE_CHECK( (!db.is_open()) &&
                                (d_tests_mongodb_face_holds(value.c_str(),
                                                            "no driver")) );
    D_TESTS_MONGODB_FACE_CHECK(db.adopt(&value, false) == D_DB_UNSUPPORTED);
    D_TESTS_MONGODB_FACE_CHECK(db.list_databases(names) == D_DB_UNSUPPORTED);
    D_TESTS_MONGODB_FACE_CHECK(names.empty());
    D_TESTS_MONGODB_FACE_CHECK(db.find_cursor(0, "c", 0, 0, cursor) ==
                               D_DB_UNSUPPORTED);
    D_TESTS_MONGODB_FACE_CHECK( (!cursor.is_open()) && (cursor.is_done()) );
    D_TESTS_MONGODB_FACE_CHECK(db.watch_deployment(0, 0, stream) ==
                               D_DB_UNSUPPORTED);
    D_TESTS_MONGODB_FACE_CHECK(!stream.is_open());
    D_TESTS_MONGODB_FACE_CHECK(db.query_rows(0, found, "{\"ping\": 1}") ==
                               D_DB_UNSUPPORTED);
    D_TESTS_MONGODB_FACE_CHECK(found.empty());
    D_TESTS_MONGODB_FACE_CHECK(!db.supports(D_MONGODB_FEATURE_TRANSACTIONS));

#if D_INTERNAL_DB_FACE_OPTIONAL
    std::optional<std::string> maybe(std::string("kept?"));

    D_TESTS_MONGODB_FACE_CHECK(
        (db.command(0, maybe, "{\"ping\": 1}") == D_DB_UNSUPPORTED) &&
        (!maybe.has_value()));
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

#if D_INTERNAL_DB_FACE_EXPECTED
    const auto none = db.list_collections(0);

    D_TESTS_MONGODB_FACE_CHECK( (!none.has_value()) &&
                                (none.error() == D_DB_UNSUPPORTED) );
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    {
        face::transaction<face::mongodb> work(db);

        D_TESTS_MONGODB_FACE_CHECK( (work.result() == D_DB_UNSUPPORTED) &&
                                    (!work.is_active()) );
    }

    D_TESTS_MONGODB_FACE_CHECK(db.close() == 0);

    return;
}

}  // namespace

/*
main
  The static members; then, over the driver, a session where a deployment
answers, or, over the stubs -- whose driver version is 0 -- what the stubs
report.
*/
int main(void)
{
    const bool stubs    = (face::mongodb::client_version_number() == 0ul);
    const bool required = (std::strcmp(
        d_tests_mongodb_face_setting("DJINTERP_TEST_MONGODB_REQUIRED", "0"),
        "1") == 0);

    g_uri = d_tests_mongodb_face_setting("DJINTERP_TEST_MONGODB_URI", g_uri);
    d_tests_mongodb_face_static(stubs);

    // the stubs, or a session where a deployment answers
    if (stubs)
    {
        d_tests_mongodb_face_stubs();
    }
    else
    {
        face::mongodb probe;

        // a deployment to reach, or the static checks alone
        if ( (probe.connect_uri(g_uri) == 0) &&
             (probe.ping() == 0) )
        {
            probe.close();
            d_tests_mongodb_face_ownership();
            d_tests_mongodb_face_session();
        }
        else
        {
            std::printf("  no deployment reached; the static checks only\n");
            D_TESTS_MONGODB_FACE_CHECK(!required);
        }
    }

    std::printf("%d checks, %d failed (%s%s, as C++ %ld; forms:%s)\n",
                g_checks,
                g_failures,
                (stubs) ? "built without the driver" : "driver ",
                face::mongodb::client_version(),
                static_cast<long>(__cplusplus),
                (sizeof(D_TESTS_MONGODB_FACE_FORMS) > 1u)
                    ? D_TESTS_MONGODB_FACE_FORMS
                    : " C++11's and below");

    return (g_failures == 0) ? 0 : 1;
}

#else

/*
main
  Below the 64-bit floor the face is absent, and there is nothing to test.
*/
int main(void)
{
    std::printf("the MongoDB face is absent below the 64-bit floor\n");

    return 0;
}

#endif  // defined(INT64_MAX)
