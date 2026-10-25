/*******************************************************************************
* djinterp [c]                                                   sqlite_face.hpp
*
* The C++ face of djinterp's SQLite layer.
*   face::sqlite owns a connection, sqlite3*, and closes it when it goes;
* every function of the layer (sqlite.h) is a member of the same name, the
* connection argument dropped, each with its C function's contract:
*     face::sqlite db;
*     if (db.open("app.db") == SQLITE_OK)
*     {
*         face::transaction<face::sqlite> work(db);
*
*         db.create_table_sql(0, "notes", "id INTEGER PRIMARY KEY, body",
*                             D_DB_IF_NOT_EXISTS);
*         work.commit();
*     }
* Results are the layer's: SQLITE_OK (0), SQLite's result code, or a
* negative D_DB_*. Functions that need no connection -- versions, features,
* database files, quoting -- are static. db_face.hpp describes what the
* face adds, how strings, lists and text cross it, and what each language
* level adds beside the exact forms: from C++17 std::optional text, from
* C++20 formats checked at compile time and std::span columns, from C++23
* std::expected results.
*   open, open_readonly, open_memory and open_with replace the connection
* only when they succeed; a failure leaves the face as it was. A member
* called on a face with no connection passes NULL to the layer, which
* answers SQLITE_MISUSE, or false, 0 or NULL.
*   face::sqlite_statement owns a prepared statement, SQLite's own
* sqlite3_stmt, and finalizes it when it goes; prepare and preparef fill one,
* and every function of sqlite_statement.h that takes a statement is its
* member, the statement argument dropped:
*     face::sqlite_statement insert;
*     if (db.prepare(insert, "INSERT INTO notes (body) VALUES (?)") == 0)
*     {
*         re_std::int64_t changes = 0;
*
*         insert.bind_text(1, "first");
*         insert.execute_update(changes);
*     }
* face::rows (db_face.hpp) holds the rows a query or a statement returned.
*   The members that forward are generated, by tools/gen_db_face.py from the
* layer's prototypes and tools/db_face/sqlite_face.hpp.in, and keep the C
* functions' contracts, in the headers each part names; those written by
* hand in the template -- the ones that open, close or hand over the
* connection or the statement -- carry their own. Edit the template, not
* this file.
*
*
* path:      /inc/djinterp/c/db/sqlite/sqlite_face.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.14
*                                                            revised: 2026.10.15
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FACE
    ----
    1.  The statement
         1.  sqlite_statement
              1.  Ownership
              2.  Describing, binding and running
    2.  The connection
         1.  sqlite
              1.  Ownership
              2.  Library
              3.  Connections
              4.  Queries
              5.  Statements
              6.  Databases
              7.  Schema
              8.  Transactions
              9.  Settings
              10. Maintenance
    3.  Swapping
2.  COST
    ----
    1.  Sizes and moves
*/

#ifndef DJINTERP_C_DB_SQLITE_SQLITE_FACE_HPP
#define DJINTERP_C_DB_SQLITE_SQLITE_FACE_HPP 1

// std
#include <cstddef>  // std::size_t
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "../db_face.hpp"  // string_arg, text, the lists, the levels
#include "./sqlite.h"      // the layer
// re_std
#include "../../../../re_std/cstdint/cstdint.hpp"  // re_std::int64_t, INT64_MAX

// std, by level: what the forms each level adds take and return
// (db_face.hpp, 1)
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <type_traits>  // std::is_nothrow_move_constructible
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER
#if D_INTERNAL_DB_FACE_OPTIONAL
    #include <optional>  // std::optional
#endif  // D_INTERNAL_DB_FACE_OPTIONAL
#if D_INTERNAL_DB_FACE_SPAN
    #include <span>  // std::span
#endif  // D_INTERNAL_DB_FACE_SPAN
#if D_INTERNAL_DB_FACE_EXPECTED
    #include <expected>  // std::expected, std::unexpected
#endif  // D_INTERNAL_DB_FACE_EXPECTED

// 64-bit floor: the layer needs a 64-bit integer type, and below it the face
// compiles to nothing, as db_face.hpp does.
#if defined(INT64_MAX)


namespace djinterp
{
namespace face
{


//==============================================================================
// 1.  FACE
//==============================================================================


// 1.1    The statement
//------------------------------------------------------------------------------
// 1.1.1
// sqlite_statement
//   class: a prepared statement, SQLite's own sqlite3_stmt, owned: finalized
// when the object goes. Made empty, and filled by face::sqlite's prepare and
// preparef; every function of sqlite_statement.h that takes a statement is a
// member of the same name, whose contract it keeps. Not copyable; movable
// from C++11.
class sqlite_statement
{
public:
    // 1.1.1.1
    // Ownership
    //   a statement made empty holds none; whatever a statement holds, it
    // finalizes when it goes.
    D_CONSTEXPR sqlite_statement() D_NOEXCEPT
        : m_statement(D_NULLPTR)
    {}

    /**
     * @brief Makes a statement that owns one prepared elsewhere.
     *
     * @param[in] _owned  the statement, or `NULL` for none.
     * @post The object finalizes `_owned`; the caller no longer does.
     */
    explicit D_CONSTEXPR sqlite_statement(
        struct sqlite3_stmt* _owned
    ) D_NOEXCEPT
        : m_statement(_owned)
    {}

    ~sqlite_statement()
    {
        // a statement, when there is one; an empty object calls nothing
        if (m_statement)
        {
            (void)d_sqlite_finalize(m_statement);
        }
    }

#if D_MOVE_ENABLED
    // moving -- the statement passes to the object moved to, which
    // finalizes what it held; the object moved from holds none
    sqlite_statement(
        sqlite_statement&& _other
    ) D_NOEXCEPT
        : m_statement(_other.release())
    {}

    sqlite_statement&
    operator=(
        sqlite_statement&& _other
    ) D_NOEXCEPT
    {
        reset(_other.release());

        return *this;
    }
#endif  // D_MOVE_ENABLED

    // the handle -- the statement, for the layer's or SQLite's own calls,
    // NULL when there is none; and whether there is one
    D_NODISCARD struct sqlite3_stmt*
    handle() const D_NOEXCEPT
    {
        return m_statement;
    }

    D_NODISCARD bool
    is_prepared() const D_NOEXCEPT
    {
        return (m_statement != D_NULLPTR);
    }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    D_EXPLICIT_BOOL operator bool() const D_NOEXCEPT
    {
        return is_prepared();
    }
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    /**
     * @brief Gives up the statement without finalizing it.
     *
     * @return the statement, which the caller then finalizes with
     *         d_sqlite_finalize; `NULL` when there is none.
     * @post The object holds no statement.
     */
    D_NODISCARD struct sqlite3_stmt*
    release() D_NOEXCEPT
    {
        struct sqlite3_stmt* const held = m_statement;

        m_statement = D_NULLPTR;

        return held;
    }

    /**
     * @brief Finalizes the statement held, if any, and takes another.
     *
     * @param[in] _owned  the statement to own, or `NULL` for none; the one
     *                    already held is kept, not finalized.
     * @post The statement that was held, if another, is finalized.
     */
    void
    reset(
        struct sqlite3_stmt* _owned = D_NULLPTR
    ) D_NOEXCEPT
    {
        // what is held is finalized, unless it is what comes
        if (_owned != m_statement)
        {
            // a statement, when there is one
            if (m_statement)
            {
                (void)d_sqlite_finalize(m_statement);
            }

            m_statement = _owned;
        }

        return;
    }

    /**
     * @brief Finalizes the statement, as d_sqlite_finalize does.
     *
     * @return d_sqlite_finalize's result, SQLITE_OK.
     * @post The object holds no statement.
     */
    int
    finalize() D_NOEXCEPT
    {
        return d_sqlite_finalize(release());
    }

    /**
     * @brief Finalizes the statement held, if any, and hands its slot to a
     *        layer's prepare to fill.
     *
     * @return where the layer writes the statement it prepares; the object
     *         owns what is written there.
     * @post The object holds no statement until the prepare fills the slot,
     *       and holds none when the prepare fails.
     */
    struct sqlite3_stmt**
    out() D_NOEXCEPT
    {
        reset();

        return &m_statement;
    }

    // swap -- the two objects trade statements
    void
    swap(
        sqlite_statement& _other
    ) D_NOEXCEPT
    {
        struct sqlite3_stmt* const held = m_statement;

        m_statement        = _other.m_statement;
        _other.m_statement = held;

        return;
    }

    // 1.1.1.2
    // Describing, binding and running
    //   sqlite_statement.h's functions that take a statement.
    D_NODISCARD std::size_t
    parameter_count() const D_NOEXCEPT
    {
        return d_sqlite_parameter_count(m_statement);
    }

    int
    parameter_index(
        string_arg   _name,
        std::size_t& _index
    ) const D_NOEXCEPT
    {
        return d_sqlite_parameter_index(m_statement, _name.c_str(), &_index);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<std::size_t, int>
    parameter_index(
        string_arg _name
    ) const D_NOEXCEPT
    {
        std::size_t value{};
        const int   result = d_sqlite_parameter_index(m_statement,
                                                      _name.c_str(),
                                                      &value);

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    D_NODISCARD sqlite3*
    statement_connection() const D_NOEXCEPT
    {
        return d_sqlite_statement_connection(m_statement);
    }

    int
    bind_null(
        std::size_t _index
    ) D_NOEXCEPT
    {
        return d_sqlite_bind_null(m_statement, _index);
    }

    int
    bind_bool(
        std::size_t _index,
        bool        _value
    ) D_NOEXCEPT
    {
        return d_sqlite_bind_bool(m_statement, _index, _value);
    }

    int
    bind_int64(
        std::size_t     _index,
        re_std::int64_t _value
    ) D_NOEXCEPT
    {
        return d_sqlite_bind_int64(m_statement, _index, _value);
    }

    int
    bind_double(
        std::size_t _index,
        double      _value
    ) D_NOEXCEPT
    {
        return d_sqlite_bind_double(m_statement, _index, _value);
    }

    int
    bind_text(
        std::size_t _index,
        string_arg  _text
    ) D_NOEXCEPT
    {
        return d_sqlite_bind_text(m_statement, _index, _text.c_str());
    }

    int
    bind_blob(
        std::size_t _index,
        const void* _bytes,
        std::size_t _size
    ) D_NOEXCEPT
    {
        return d_sqlite_bind_blob(m_statement, _index, _bytes, _size);
    }

    int
    clear_bindings() D_NOEXCEPT
    {
        return d_sqlite_clear_bindings(m_statement);
    }

    int
    execute_update(
        re_std::int64_t& _changes
    ) D_NOEXCEPT
    {
        return d_sqlite_execute_update(m_statement, &_changes);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<re_std::int64_t, int>
    execute_update() D_NOEXCEPT
    {
        re_std::int64_t value{};
        const int       result = d_sqlite_execute_update(m_statement, &value);

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    execute_query(
        rows& _rows
    ) D_NOEXCEPT
    {
        return d_sqlite_execute_query(m_statement, _rows.out());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<rows, int>
    execute_query() D_NOEXCEPT
    {
        rows      value{};
        const int result = d_sqlite_execute_query(m_statement, value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    open_cursor() D_NOEXCEPT
    {
        return d_sqlite_open_cursor(m_statement);
    }

    int
    fetch_rows(
        std::size_t _max,
        rows&       _rows
    ) D_NOEXCEPT
    {
        return d_sqlite_fetch_rows(m_statement, _max, _rows.out());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<rows, int>
    fetch_rows(
        std::size_t _max
    ) D_NOEXCEPT
    {
        rows      value{};
        const int result = d_sqlite_fetch_rows(m_statement, _max, value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    close_cursor() D_NOEXCEPT
    {
        return d_sqlite_close_cursor(m_statement);
    }

private:
    D_DELETED_FN_MSG(sqlite_statement(const sqlite_statement&),
                     "a statement owns what SQLite prepared; move it")
    D_DELETED_FN_MSG(sqlite_statement& operator=(const sqlite_statement&),
                     "a statement owns what SQLite prepared; move it")

    struct sqlite3_stmt* m_statement;
};

// 1.2    The connection
//------------------------------------------------------------------------------
// 1.2.1
// sqlite
//   class: a SQLite connection, owned: closed when the object goes. Every
// member forwards to the layer's function of the same name, whose contract
// it keeps. Not copyable; movable from C++11.
class sqlite
{
public:
    // transaction_mode
    //   type: how a transaction begins, for face::transaction.
    typedef enum d_sqlite_transaction_mode transaction_mode;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // format
    //   type: an f form's format, for the arguments that follow it: a
    // literal in the layer's dialect, read at compile time from C++20, or a
    // format runtime_format marked (db_face.hpp, 2.3).
    template<typename... Args>
    using format = format_arg<internal::sqlite_dialect, Args...>;
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    // 1.2.1.1
    // Ownership
    //   a face made empty holds no connection; whatever a face holds, it
    // closes when it goes.
    D_CONSTEXPR sqlite() D_NOEXCEPT
        : m_db(D_NULLPTR)
    {}

    /**
     * @brief Makes a face that owns a connection opened elsewhere.
     *
     * @param[in] _owned  the connection, or `NULL` for none.
     * @post The face closes `_owned`; the caller no longer does.
     */
    explicit D_CONSTEXPR sqlite(
        struct sqlite3* _owned
    ) D_NOEXCEPT
        : m_db(_owned)
    {}

    ~sqlite()
    {
        // a connection, when there is one; an empty face calls nothing
        if (m_db)
        {
            (void)d_sqlite_close(m_db);
        }
    }

#if D_MOVE_ENABLED
    // moving -- the connection passes to the face moved to, which closes
    // what it held; the face moved from holds none
    sqlite(
        sqlite&& _other
    ) D_NOEXCEPT
        : m_db(_other.release())
    {}

    sqlite&
    operator=(
        sqlite&& _other
    ) D_NOEXCEPT
    {
        reset(_other.release());

        return *this;
    }
#endif  // D_MOVE_ENABLED

    // the handle -- the connection, for the layer's or SQLite's own calls,
    // NULL when there is none; and whether there is one
    D_NODISCARD struct sqlite3*
    handle() const D_NOEXCEPT
    {
        return m_db;
    }

    D_NODISCARD bool
    is_open() const D_NOEXCEPT
    {
        return (m_db != D_NULLPTR);
    }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    D_EXPLICIT_BOOL operator bool() const D_NOEXCEPT
    {
        return is_open();
    }
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    /**
     * @brief Gives up the connection without closing it.
     *
     * @return the connection, which the caller then closes with
     *         d_sqlite_close; `NULL` when there is none.
     * @post The face holds no connection.
     */
    D_NODISCARD struct sqlite3*
    release() D_NOEXCEPT
    {
        struct sqlite3* const held = m_db;

        m_db = D_NULLPTR;

        return held;
    }

    /**
     * @brief Closes the connection held, if any, and takes another.
     *
     * @param[in] _owned  the connection to own, or `NULL` for none; the one
     *                    already held is kept, not closed.
     * @post The connection that was held, if another, is closed, and every
     *       pointer obtained through it is invalid.
     */
    void
    reset(
        struct sqlite3* _owned = D_NULLPTR
    ) D_NOEXCEPT
    {
        // what is held closes, unless it is what comes
        if (_owned != m_db)
        {
            // a connection, when there is one
            if (m_db)
            {
                (void)d_sqlite_close(m_db);
            }

            m_db = _owned;
        }

        return;
    }

    // swap -- the two faces trade connections
    void
    swap(
        sqlite& _other
    ) D_NOEXCEPT
    {
        struct sqlite3* const held = m_db;

        m_db        = _other.m_db;
        _other.m_db = held;

        return;
    }

    // default_transaction_mode -- how face::transaction begins when told
    // nothing: DEFERRED
    D_NODISCARD static D_CONSTEXPR transaction_mode
    default_transaction_mode() D_NOEXCEPT
    {
        return D_SQLITE_TRANSACTION_DEFERRED;
    }

    // 1.2.1.2
    // Library
    //   sqlite_common.h's versions, features and result messages: static,
    // since they need no connection.
    D_NODISCARD static const char*
    library_version() D_NOEXCEPT
    {
        return d_sqlite_library_version();
    }

    D_NODISCARD static int
    library_version_number() D_NOEXCEPT
    {
        return d_sqlite_library_version_number();
    }

    D_NODISCARD static int
    header_version_number() D_NOEXCEPT
    {
        return d_sqlite_header_version_number();
    }

    D_NODISCARD static bool
    library_at_least(
        int _major,
        int _minor,
        int _patch
    ) D_NOEXCEPT
    {
        return d_sqlite_library_at_least(_major, _minor, _patch);
    }

    D_NODISCARD static bool
    supports(
        d_sqlite_feature _feature
    ) D_NOEXCEPT
    {
        return d_sqlite_supports(_feature);
    }

    D_NODISCARD static int
    library_threadsafe() D_NOEXCEPT
    {
        return d_sqlite_library_threadsafe();
    }

    D_NODISCARD static const char*
    result_message(
        int _result
    ) D_NOEXCEPT
    {
        return d_sqlite_result_message(_result);
    }

    // 1.2.1.3
    // Connections
    //   sqlite_connection.h, and sqlite_common.h's error message.

    /**
     * @brief Opens a database file, as d_sqlite_open does, in place of the
     *        connection held.
     *
     * @param[in] _path  the file, in UTF-8; ":memory:" opens a private
     *                   in-memory database.
     * @return d_sqlite_open's result.
     * @post On success the face holds the new connection, and the one it
     *       held is closed; on failure the face is as it was.
     */
    int
    open(
        string_arg _path
    ) D_NOEXCEPT
    {
        struct sqlite3* opened = D_NULLPTR;
        const int       result = d_sqlite_open(_path.c_str(), &opened);

        return m_take(result, opened);
    }

    /**
     * @brief Opens an existing database file for reading only, as
     *        d_sqlite_open_readonly does, in place of the connection held.
     *
     * @param[in] _path  the file, in UTF-8.
     * @return d_sqlite_open_readonly's result.
     * @post As for open.
     */
    int
    open_readonly(
        string_arg _path
    ) D_NOEXCEPT
    {
        struct sqlite3* opened = D_NULLPTR;
        const int       result = d_sqlite_open_readonly(_path.c_str(),
                                                        &opened);

        return m_take(result, opened);
    }

    /**
     * @brief Opens a private in-memory database, as d_sqlite_open_memory
     *        does, in place of the connection held.
     *
     * @return d_sqlite_open_memory's result.
     * @post As for open.
     */
    int
    open_memory() D_NOEXCEPT
    {
        struct sqlite3* opened = D_NULLPTR;
        const int       result = d_sqlite_open_memory(&opened);

        return m_take(result, opened);
    }

    /**
     * @brief Opens a database file as SQLite's open flags say, through the
     *        VFS named, as d_sqlite_open_with does, in place of the
     *        connection held.
     *
     * @param[in] _path   the file, in UTF-8, or a URI with
     *                    D_SQLITE_OPEN_URI.
     * @param[in] _flags  D_SQLITE_OPEN_* flags (sqlite_connection.h).
     * @param[in] _vfs    the VFS's name; none, or `NULL`, for the default.
     * @return d_sqlite_open_with's result.
     * @post As for open.
     */
    int
    open_with(
        string_arg _path,
        unsigned   _flags,
        string_arg _vfs = string_arg()
    ) D_NOEXCEPT
    {
        struct sqlite3* opened = D_NULLPTR;
        const int       result = d_sqlite_open_with(_path.c_str(), _flags,
                                                    _vfs.c_str(), &opened);

        return m_take(result, opened);
    }

    /**
     * @brief Closes the connection, as d_sqlite_close does.
     *
     * @return d_sqlite_close's result; SQLITE_OK when there is no
     *         connection.
     * @post The face holds no connection, whatever the result.
     */
    int
    close() D_NOEXCEPT
    {
        return d_sqlite_close(release());
    }

    int
    apply_defaults() D_NOEXCEPT
    {
        return d_sqlite_apply_defaults(m_db);
    }

    D_NODISCARD const char*
    database_path(
        string_arg _schema
    ) const D_NOEXCEPT
    {
        return d_sqlite_database_path(m_db, _schema.c_str());
    }

    D_NODISCARD bool
    is_readonly(
        string_arg _schema
    ) const D_NOEXCEPT
    {
        return d_sqlite_is_readonly(m_db, _schema.c_str());
    }

    D_NODISCARD re_std::int64_t
    last_insert_id() const D_NOEXCEPT
    {
        return d_sqlite_last_insert_id(m_db);
    }

    D_NODISCARD re_std::int64_t
    changes() const D_NOEXCEPT
    {
        return d_sqlite_changes(m_db);
    }

    D_NODISCARD re_std::int64_t
    total_changes() const D_NOEXCEPT
    {
        return d_sqlite_total_changes(m_db);
    }

    int
    load_extension(
        string_arg _path,
        string_arg _entry_point,
        text&      _message
    ) D_NOEXCEPT
    {
        return d_sqlite_load_extension(m_db,
                                       _path.c_str(),
                                       _entry_point.c_str(),
                                       _message.out());
    }

    int
    load_extension(
        string_arg   _path,
        string_arg   _entry_point,
        std::string& _message
    )
    {
        text      text_value;
        const int result = d_sqlite_load_extension(m_db,
                                                   _path.c_str(),
                                                   _entry_point.c_str(),
                                                   text_value.out());

        _message = text_value.str();

        return result;
    }

#if D_INTERNAL_DB_FACE_OPTIONAL
    int
    load_extension(
        string_arg                  _path,
        string_arg                  _entry_point,
        std::optional<std::string>& _message
    )
    {
        text      text_value;
        const int result = d_sqlite_load_extension(m_db,
                                                   _path.c_str(),
                                                   _entry_point.c_str(),
                                                   text_value.out());

        // the text; nothing for SQL NULL, and for a failure
        if (text_value.is_null())
        {
            _message.reset();
        }
        else
        {
            _message = text_value.str();
        }

        return result;
    }
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

    int
    enable_load_extension(
        bool _enabled
    ) D_NOEXCEPT
    {
        return d_sqlite_enable_load_extension(m_db, _enabled);
    }

    D_NODISCARD const char*
    error_message() const D_NOEXCEPT
    {
        return d_sqlite_error_message(m_db);
    }

    // 1.2.1.4
    // Queries
    //   sqlite_query.h; the f forms from C++11, their formats checked from
    // C++20 (db_face.hpp, 2.2 to 2.5).
    int
    exec(
        string_arg _sql
    ) D_NOEXCEPT
    {
        return d_sqlite_exec(m_db, _sql.c_str());
    }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    template<typename... Args>
    int
    execf(
        format<Args...> _format,
        Args...         _args
    ) D_NOEXCEPT
    {
        internal::check_varargs<Args...>();

        return d_sqlite_execf(m_db, _format.c_str(), _args...);
    }
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    int
    exec_changes(
        re_std::int64_t& _changes,
        string_arg       _sql
    ) D_NOEXCEPT
    {
        return d_sqlite_exec_changes(m_db, &_changes, _sql.c_str());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<re_std::int64_t, int>
    exec_changes(
        string_arg _sql
    ) D_NOEXCEPT
    {
        re_std::int64_t value{};
        const int       result = d_sqlite_exec_changes(m_db,
                                                       &value,
                                                       _sql.c_str());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    template<typename... Args>
    int
    execf_changes(
        re_std::int64_t& _changes,
        format<Args...>  _format,
        Args...          _args
    ) D_NOEXCEPT
    {
        internal::check_varargs<Args...>();

        return d_sqlite_execf_changes(m_db,
                                      &_changes,
                                      _format.c_str(),
                                      _args...);
    }
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_INTERNAL_DB_FACE_EXPECTED
    template<typename... Args>
    D_NODISCARD std::expected<re_std::int64_t, int>
    execf_changes(
        format<Args...> _format,
        Args...         _args
    ) D_NOEXCEPT
    {
        internal::check_varargs<Args...>();

        re_std::int64_t value{};
        const int       result = d_sqlite_execf_changes(m_db,
                                                        &value,
                                                        _format.c_str(),
                                                        _args...);

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    query_int64(
        re_std::int64_t& _value,
        string_arg       _sql
    ) D_NOEXCEPT
    {
        return d_sqlite_query_int64(m_db, &_value, _sql.c_str());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<re_std::int64_t, int>
    query_int64(
        string_arg _sql
    ) D_NOEXCEPT
    {
        re_std::int64_t value{};
        const int       result = d_sqlite_query_int64(m_db,
                                                      &value,
                                                      _sql.c_str());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    template<typename... Args>
    int
    query_int64f(
        re_std::int64_t& _value,
        format<Args...>  _format,
        Args...          _args
    ) D_NOEXCEPT
    {
        internal::check_varargs<Args...>();

        return d_sqlite_query_int64f(m_db, &_value, _format.c_str(), _args...);
    }
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_INTERNAL_DB_FACE_EXPECTED
    template<typename... Args>
    D_NODISCARD std::expected<re_std::int64_t, int>
    query_int64f(
        format<Args...> _format,
        Args...         _args
    ) D_NOEXCEPT
    {
        internal::check_varargs<Args...>();

        re_std::int64_t value{};
        const int       result = d_sqlite_query_int64f(m_db,
                                                       &value,
                                                       _format.c_str(),
                                                       _args...);

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    query_double(
        double&    _value,
        string_arg _sql
    ) D_NOEXCEPT
    {
        return d_sqlite_query_double(m_db, &_value, _sql.c_str());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<double, int>
    query_double(
        string_arg _sql
    ) D_NOEXCEPT
    {
        double    value{};
        const int result = d_sqlite_query_double(m_db, &value, _sql.c_str());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    template<typename... Args>
    int
    query_doublef(
        double&         _value,
        format<Args...> _format,
        Args...         _args
    ) D_NOEXCEPT
    {
        internal::check_varargs<Args...>();

        return d_sqlite_query_doublef(m_db, &_value, _format.c_str(), _args...);
    }
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_INTERNAL_DB_FACE_EXPECTED
    template<typename... Args>
    D_NODISCARD std::expected<double, int>
    query_doublef(
        format<Args...> _format,
        Args...         _args
    ) D_NOEXCEPT
    {
        internal::check_varargs<Args...>();

        double    value{};
        const int result = d_sqlite_query_doublef(m_db,
                                                  &value,
                                                  _format.c_str(),
                                                  _args...);

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    query_text(
        text&      _value,
        string_arg _sql
    ) D_NOEXCEPT
    {
        return d_sqlite_query_text(m_db, _value.out(), _sql.c_str());
    }

    int
    query_text(
        std::string& _value,
        string_arg   _sql
    )
    {
        text      text_value;
        const int result = d_sqlite_query_text(m_db,
                                               text_value.out(),
                                               _sql.c_str());

        _value = text_value.str();

        return result;
    }

#if D_INTERNAL_DB_FACE_OPTIONAL
    int
    query_text(
        std::optional<std::string>& _value,
        string_arg                  _sql
    )
    {
        text      text_value;
        const int result = d_sqlite_query_text(m_db,
                                               text_value.out(),
                                               _sql.c_str());

        // the text; nothing for SQL NULL, and for a failure
        if (text_value.is_null())
        {
            _value.reset();
        }
        else
        {
            _value = text_value.str();
        }

        return result;
    }
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<text, int>
    query_text(
        string_arg _sql
    ) D_NOEXCEPT
    {
        text      value{};
        const int result = d_sqlite_query_text(m_db, value.out(), _sql.c_str());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    template<typename... Args>
    int
    query_textf(
        text&           _value,
        format<Args...> _format,
        Args...         _args
    ) D_NOEXCEPT
    {
        internal::check_varargs<Args...>();

        return d_sqlite_query_textf(m_db,
                                    _value.out(),
                                    _format.c_str(),
                                    _args...);
    }
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    template<typename... Args>
    int
    query_textf(
        std::string&    _value,
        format<Args...> _format,
        Args...         _args
    )
    {
        internal::check_varargs<Args...>();

        text      text_value;
        const int result = d_sqlite_query_textf(m_db,
                                                text_value.out(),
                                                _format.c_str(),
                                                _args...);

        _value = text_value.str();

        return result;
    }
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_INTERNAL_DB_FACE_OPTIONAL
    template<typename... Args>
    int
    query_textf(
        std::optional<std::string>& _value,
        format<Args...>             _format,
        Args...                     _args
    )
    {
        internal::check_varargs<Args...>();

        text      text_value;
        const int result = d_sqlite_query_textf(m_db,
                                                text_value.out(),
                                                _format.c_str(),
                                                _args...);

        // the text; nothing for SQL NULL, and for a failure
        if (text_value.is_null())
        {
            _value.reset();
        }
        else
        {
            _value = text_value.str();
        }

        return result;
    }
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

#if D_INTERNAL_DB_FACE_EXPECTED
    template<typename... Args>
    D_NODISCARD std::expected<text, int>
    query_textf(
        format<Args...> _format,
        Args...         _args
    ) D_NOEXCEPT
    {
        internal::check_varargs<Args...>();

        text      value{};
        const int result = d_sqlite_query_textf(m_db,
                                                value.out(),
                                                _format.c_str(),
                                                _args...);

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    query_rows(
        rows&      _rows,
        string_arg _sql
    ) D_NOEXCEPT
    {
        return d_sqlite_query_rows(m_db, _rows.out(), _sql.c_str());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<rows, int>
    query_rows(
        string_arg _sql
    ) D_NOEXCEPT
    {
        rows      value{};
        const int result = d_sqlite_query_rows(m_db, value.out(), _sql.c_str());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    template<typename... Args>
    int
    query_rowsf(
        rows&           _rows,
        format<Args...> _format,
        Args...         _args
    ) D_NOEXCEPT
    {
        internal::check_varargs<Args...>();

        return d_sqlite_query_rowsf(m_db,
                                    _rows.out(),
                                    _format.c_str(),
                                    _args...);
    }
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_INTERNAL_DB_FACE_EXPECTED
    template<typename... Args>
    D_NODISCARD std::expected<rows, int>
    query_rowsf(
        format<Args...> _format,
        Args...         _args
    ) D_NOEXCEPT
    {
        internal::check_varargs<Args...>();

        rows      value{};
        const int result = d_sqlite_query_rowsf(m_db,
                                                value.out(),
                                                _format.c_str(),
                                                _args...);

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    static void
    quote_identifier(
        string_arg _name,
        text&      _quoted
    ) D_NOEXCEPT
    {
        _quoted.reset(d_sqlite_quote_identifier(_name.c_str()));

        return;
    }

    D_NODISCARD static std::string
    quote_identifier(
        string_arg _name
    )
    {
        const text result(d_sqlite_quote_identifier(_name.c_str()));

        return result.str();
    }

    static void
    quote_literal(
        string_arg _text,
        text&      _quoted
    ) D_NOEXCEPT
    {
        _quoted.reset(d_sqlite_quote_literal(_text.c_str()));

        return;
    }

    D_NODISCARD static std::string
    quote_literal(
        string_arg _text
    )
    {
        const text result(d_sqlite_quote_literal(_text.c_str()));

        return result.str();
    }

    // 1.2.1.5
    // Statements
    //   sqlite_statement.h's preparing, into a face::sqlite_statement, whose
    // members its other functions are.
    int
    prepare(
        sqlite_statement& _statement,
        string_arg        _sql
    ) D_NOEXCEPT
    {
        return d_sqlite_prepare(m_db, _statement.out(), _sql.c_str());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<sqlite_statement, int>
    prepare(
        string_arg _sql
    ) D_NOEXCEPT
    {
        sqlite_statement value{};
        const int        result = d_sqlite_prepare(m_db,
                                                   value.out(),
                                                   _sql.c_str());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    template<typename... Args>
    int
    preparef(
        sqlite_statement& _statement,
        format<Args...>   _format,
        Args...           _args
    ) D_NOEXCEPT
    {
        internal::check_varargs<Args...>();

        return d_sqlite_preparef(m_db,
                                 _statement.out(),
                                 _format.c_str(),
                                 _args...);
    }
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_INTERNAL_DB_FACE_EXPECTED
    template<typename... Args>
    D_NODISCARD std::expected<sqlite_statement, int>
    preparef(
        format<Args...> _format,
        Args...         _args
    ) D_NOEXCEPT
    {
        internal::check_varargs<Args...>();

        sqlite_statement value{};
        const int        result = d_sqlite_preparef(m_db,
                                                    value.out(),
                                                    _format.c_str(),
                                                    _args...);

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    // 1.2.1.6
    // Databases
    //   sqlite_database.h.
    D_NODISCARD bool
    has_database(
        string_arg _name
    ) const D_NOEXCEPT
    {
        return d_sqlite_has_database(m_db, _name.c_str());
    }

    int
    has_database_s(
        string_arg _name,
        bool&      _exists
    ) const D_NOEXCEPT
    {
        return d_sqlite_has_database_s(m_db, _name.c_str(), &_exists);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<bool, int>
    has_database_s(
        string_arg _name
    ) const D_NOEXCEPT
    {
        bool      value{};
        const int result = d_sqlite_has_database_s(m_db, _name.c_str(), &value);

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    attach_database(
        string_arg _path,
        string_arg _name
    ) D_NOEXCEPT
    {
        return d_sqlite_attach_database(m_db, _path.c_str(), _name.c_str());
    }

    int
    detach_database(
        string_arg _name
    ) D_NOEXCEPT
    {
        return d_sqlite_detach_database(m_db, _name.c_str());
    }

    int
    list_databases(
        name_list& _names
    ) const D_NOEXCEPT
    {
        return d_sqlite_list_databases(m_db, _names.out());
    }

    int
    list_databases(
        std::vector<std::string>& _names
    ) const
    {
        name_list names;
        const int result = d_sqlite_list_databases(m_db, names.out());

        names.to_vector(_names);

        return result;
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<name_list, int>
    list_databases() const D_NOEXCEPT
    {
        name_list value{};
        const int result = d_sqlite_list_databases(m_db, value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    static int
    create_database_file(
        string_arg _path,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        return d_sqlite_create_database_file(_path.c_str(), _flags);
    }

    static int
    delete_database_file(
        string_arg _path,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        return d_sqlite_delete_database_file(_path.c_str(), _flags);
    }

    D_NODISCARD static bool
    is_database_file(
        string_arg _path
    ) D_NOEXCEPT
    {
        return d_sqlite_is_database_file(_path.c_str());
    }

    int
    serialize(
        string_arg   _schema,
        void*&       _bytes,
        std::size_t& _size
    ) D_NOEXCEPT
    {
        return d_sqlite_serialize(m_db, _schema.c_str(), &_bytes, &_size);
    }

    int
    deserialize(
        string_arg  _schema,
        const void* _bytes,
        std::size_t _size,
        bool        _read_only
    ) D_NOEXCEPT
    {
        return d_sqlite_deserialize(m_db,
                                    _schema.c_str(),
                                    _bytes,
                                    _size,
                                    _read_only);
    }

    // 1.2.1.7
    // Schema
    //   sqlite_schema.h.
    D_NODISCARD bool
    has_table(
        string_arg _schema,
        string_arg _table
    ) const D_NOEXCEPT
    {
        return d_sqlite_has_table(m_db, _schema.c_str(), _table.c_str());
    }

    int
    has_table_s(
        string_arg _schema,
        string_arg _table,
        bool&      _exists
    ) const D_NOEXCEPT
    {
        return d_sqlite_has_table_s(m_db,
                                    _schema.c_str(),
                                    _table.c_str(),
                                    &_exists);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<bool, int>
    has_table_s(
        string_arg _schema,
        string_arg _table
    ) const D_NOEXCEPT
    {
        bool      value{};
        const int result = d_sqlite_has_table_s(m_db,
                                                _schema.c_str(),
                                                _table.c_str(),
                                                &value);

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    create_table(
        string_arg         _schema,
        string_arg         _table,
        const d_db_column* _columns,
        std::size_t        _count,
        unsigned           _flags
    ) D_NOEXCEPT
    {
        return d_sqlite_create_table(m_db,
                                     _schema.c_str(),
                                     _table.c_str(),
                                     _columns,
                                     _count,
                                     _flags);
    }

    template<std::size_t Count>
    int
    create_table(
        string_arg        _schema,
        string_arg        _table,
        const d_db_column (&_columns)[Count],
        unsigned          _flags
    ) D_NOEXCEPT
    {
        return d_sqlite_create_table(m_db,
                                     _schema.c_str(),
                                     _table.c_str(),
                                     _columns,
                                     Count,
                                     _flags);
    }

#if D_INTERNAL_DB_FACE_SPAN
    int
    create_table(
        string_arg                   _schema,
        string_arg                   _table,
        std::span<const d_db_column> _columns,
        unsigned                     _flags
    ) D_NOEXCEPT
    {
        return d_sqlite_create_table(m_db,
                                     _schema.c_str(),
                                     _table.c_str(),
                                     _columns.data(),
                                     _columns.size(),
                                     _flags);
    }
#endif  // D_INTERNAL_DB_FACE_SPAN

    int
    create_table_sql(
        string_arg _schema,
        string_arg _table,
        string_arg _definition,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        return d_sqlite_create_table_sql(m_db,
                                         _schema.c_str(),
                                         _table.c_str(),
                                         _definition.c_str(),
                                         _flags);
    }

    int
    drop_table(
        string_arg _schema,
        string_arg _table,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        return d_sqlite_drop_table(m_db,
                                   _schema.c_str(),
                                   _table.c_str(),
                                   _flags);
    }

    int
    rename_table(
        string_arg _schema,
        string_arg _table,
        string_arg _new_name
    ) D_NOEXCEPT
    {
        return d_sqlite_rename_table(m_db,
                                     _schema.c_str(),
                                     _table.c_str(),
                                     _new_name.c_str());
    }

    int
    truncate_table(
        string_arg _schema,
        string_arg _table
    ) D_NOEXCEPT
    {
        return d_sqlite_truncate_table(m_db, _schema.c_str(), _table.c_str());
    }

    int
    count_rows(
        string_arg       _schema,
        string_arg       _table,
        re_std::int64_t& _count
    ) const D_NOEXCEPT
    {
        return d_sqlite_count_rows(m_db,
                                   _schema.c_str(),
                                   _table.c_str(),
                                   &_count);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<re_std::int64_t, int>
    count_rows(
        string_arg _schema,
        string_arg _table
    ) const D_NOEXCEPT
    {
        re_std::int64_t value{};
        const int       result = d_sqlite_count_rows(m_db,
                                                     _schema.c_str(),
                                                     _table.c_str(),
                                                     &value);

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    list_tables(
        string_arg _schema,
        name_list& _names
    ) const D_NOEXCEPT
    {
        return d_sqlite_list_tables(m_db, _schema.c_str(), _names.out());
    }

    int
    list_tables(
        string_arg                _schema,
        std::vector<std::string>& _names
    ) const
    {
        name_list names;
        const int result = d_sqlite_list_tables(m_db,
                                                _schema.c_str(),
                                                names.out());

        names.to_vector(_names);

        return result;
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<name_list, int>
    list_tables(
        string_arg _schema
    ) const D_NOEXCEPT
    {
        name_list value{};
        const int result = d_sqlite_list_tables(m_db,
                                                _schema.c_str(),
                                                value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    D_NODISCARD bool
    has_column(
        string_arg _schema,
        string_arg _table,
        string_arg _column
    ) const D_NOEXCEPT
    {
        return d_sqlite_has_column(m_db,
                                   _schema.c_str(),
                                   _table.c_str(),
                                   _column.c_str());
    }

    int
    has_column_s(
        string_arg _schema,
        string_arg _table,
        string_arg _column,
        bool&      _exists
    ) const D_NOEXCEPT
    {
        return d_sqlite_has_column_s(m_db,
                                     _schema.c_str(),
                                     _table.c_str(),
                                     _column.c_str(),
                                     &_exists);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<bool, int>
    has_column_s(
        string_arg _schema,
        string_arg _table,
        string_arg _column
    ) const D_NOEXCEPT
    {
        bool      value{};
        const int result = d_sqlite_has_column_s(m_db,
                                                 _schema.c_str(),
                                                 _table.c_str(),
                                                 _column.c_str(),
                                                 &value);

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    add_column(
        string_arg         _schema,
        string_arg         _table,
        const d_db_column& _column
    ) D_NOEXCEPT
    {
        return d_sqlite_add_column(m_db,
                                   _schema.c_str(),
                                   _table.c_str(),
                                   &_column);
    }

    int
    add_column_sql(
        string_arg _schema,
        string_arg _table,
        string_arg _definition
    ) D_NOEXCEPT
    {
        return d_sqlite_add_column_sql(m_db,
                                       _schema.c_str(),
                                       _table.c_str(),
                                       _definition.c_str());
    }

    int
    rename_column(
        string_arg _schema,
        string_arg _table,
        string_arg _column,
        string_arg _new_name
    ) D_NOEXCEPT
    {
        return d_sqlite_rename_column(m_db,
                                      _schema.c_str(),
                                      _table.c_str(),
                                      _column.c_str(),
                                      _new_name.c_str());
    }

    int
    drop_column(
        string_arg _schema,
        string_arg _table,
        string_arg _column
    ) D_NOEXCEPT
    {
        return d_sqlite_drop_column(m_db,
                                    _schema.c_str(),
                                    _table.c_str(),
                                    _column.c_str());
    }

    int
    list_columns(
        string_arg   _schema,
        string_arg   _table,
        column_list& _columns
    ) const D_NOEXCEPT
    {
        return d_sqlite_list_columns(m_db,
                                     _schema.c_str(),
                                     _table.c_str(),
                                     _columns.out());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<column_list, int>
    list_columns(
        string_arg _schema,
        string_arg _table
    ) const D_NOEXCEPT
    {
        column_list value{};
        const int   result = d_sqlite_list_columns(m_db,
                                                   _schema.c_str(),
                                                   _table.c_str(),
                                                   value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    D_NODISCARD bool
    has_index(
        string_arg _schema,
        string_arg _index
    ) const D_NOEXCEPT
    {
        return d_sqlite_has_index(m_db, _schema.c_str(), _index.c_str());
    }

    int
    has_index_s(
        string_arg _schema,
        string_arg _index,
        bool&      _exists
    ) const D_NOEXCEPT
    {
        return d_sqlite_has_index_s(m_db,
                                    _schema.c_str(),
                                    _index.c_str(),
                                    &_exists);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<bool, int>
    has_index_s(
        string_arg _schema,
        string_arg _index
    ) const D_NOEXCEPT
    {
        bool      value{};
        const int result = d_sqlite_has_index_s(m_db,
                                                _schema.c_str(),
                                                _index.c_str(),
                                                &value);

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    create_index(
        string_arg _schema,
        string_arg _index,
        string_arg _table,
        string_arg _columns,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        return d_sqlite_create_index(m_db,
                                     _schema.c_str(),
                                     _index.c_str(),
                                     _table.c_str(),
                                     _columns.c_str(),
                                     _flags);
    }

    int
    drop_index(
        string_arg _schema,
        string_arg _index,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        return d_sqlite_drop_index(m_db,
                                   _schema.c_str(),
                                   _index.c_str(),
                                   _flags);
    }

    int
    list_indexes(
        string_arg _schema,
        string_arg _table,
        name_list& _names
    ) const D_NOEXCEPT
    {
        return d_sqlite_list_indexes(m_db,
                                     _schema.c_str(),
                                     _table.c_str(),
                                     _names.out());
    }

    int
    list_indexes(
        string_arg                _schema,
        string_arg                _table,
        std::vector<std::string>& _names
    ) const
    {
        name_list names;
        const int result = d_sqlite_list_indexes(m_db,
                                                 _schema.c_str(),
                                                 _table.c_str(),
                                                 names.out());

        names.to_vector(_names);

        return result;
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<name_list, int>
    list_indexes(
        string_arg _schema,
        string_arg _table
    ) const D_NOEXCEPT
    {
        name_list value{};
        const int result = d_sqlite_list_indexes(m_db,
                                                 _schema.c_str(),
                                                 _table.c_str(),
                                                 value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    D_NODISCARD bool
    has_view(
        string_arg _schema,
        string_arg _view
    ) const D_NOEXCEPT
    {
        return d_sqlite_has_view(m_db, _schema.c_str(), _view.c_str());
    }

    int
    has_view_s(
        string_arg _schema,
        string_arg _view,
        bool&      _exists
    ) const D_NOEXCEPT
    {
        return d_sqlite_has_view_s(m_db,
                                   _schema.c_str(),
                                   _view.c_str(),
                                   &_exists);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<bool, int>
    has_view_s(
        string_arg _schema,
        string_arg _view
    ) const D_NOEXCEPT
    {
        bool      value{};
        const int result = d_sqlite_has_view_s(m_db,
                                               _schema.c_str(),
                                               _view.c_str(),
                                               &value);

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    create_view(
        string_arg _schema,
        string_arg _view,
        string_arg _select,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        return d_sqlite_create_view(m_db,
                                    _schema.c_str(),
                                    _view.c_str(),
                                    _select.c_str(),
                                    _flags);
    }

    int
    drop_view(
        string_arg _schema,
        string_arg _view,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        return d_sqlite_drop_view(m_db, _schema.c_str(), _view.c_str(), _flags);
    }

    int
    list_views(
        string_arg _schema,
        name_list& _names
    ) const D_NOEXCEPT
    {
        return d_sqlite_list_views(m_db, _schema.c_str(), _names.out());
    }

    int
    list_views(
        string_arg                _schema,
        std::vector<std::string>& _names
    ) const
    {
        name_list names;
        const int result = d_sqlite_list_views(m_db,
                                               _schema.c_str(),
                                               names.out());

        names.to_vector(_names);

        return result;
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<name_list, int>
    list_views(
        string_arg _schema
    ) const D_NOEXCEPT
    {
        name_list value{};
        const int result = d_sqlite_list_views(m_db,
                                               _schema.c_str(),
                                               value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    D_NODISCARD bool
    has_trigger(
        string_arg _schema,
        string_arg _trigger
    ) const D_NOEXCEPT
    {
        return d_sqlite_has_trigger(m_db, _schema.c_str(), _trigger.c_str());
    }

    int
    has_trigger_s(
        string_arg _schema,
        string_arg _trigger,
        bool&      _exists
    ) const D_NOEXCEPT
    {
        return d_sqlite_has_trigger_s(m_db,
                                      _schema.c_str(),
                                      _trigger.c_str(),
                                      &_exists);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<bool, int>
    has_trigger_s(
        string_arg _schema,
        string_arg _trigger
    ) const D_NOEXCEPT
    {
        bool      value{};
        const int result = d_sqlite_has_trigger_s(m_db,
                                                  _schema.c_str(),
                                                  _trigger.c_str(),
                                                  &value);

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    create_trigger(
        string_arg _schema,
        string_arg _trigger,
        string_arg _definition,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        return d_sqlite_create_trigger(m_db,
                                       _schema.c_str(),
                                       _trigger.c_str(),
                                       _definition.c_str(),
                                       _flags);
    }

    int
    drop_trigger(
        string_arg _schema,
        string_arg _trigger,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        return d_sqlite_drop_trigger(m_db,
                                     _schema.c_str(),
                                     _trigger.c_str(),
                                     _flags);
    }

    int
    list_triggers(
        string_arg _schema,
        string_arg _table,
        name_list& _names
    ) const D_NOEXCEPT
    {
        return d_sqlite_list_triggers(m_db,
                                      _schema.c_str(),
                                      _table.c_str(),
                                      _names.out());
    }

    int
    list_triggers(
        string_arg                _schema,
        string_arg                _table,
        std::vector<std::string>& _names
    ) const
    {
        name_list names;
        const int result = d_sqlite_list_triggers(m_db,
                                                  _schema.c_str(),
                                                  _table.c_str(),
                                                  names.out());

        names.to_vector(_names);

        return result;
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<name_list, int>
    list_triggers(
        string_arg _schema,
        string_arg _table
    ) const D_NOEXCEPT
    {
        name_list value{};
        const int result = d_sqlite_list_triggers(m_db,
                                                  _schema.c_str(),
                                                  _table.c_str(),
                                                  value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    // 1.2.1.8
    // Transactions
    //   sqlite_transaction.h; face::transaction and face::savepoint guard
    // them.
    int
    begin_transaction(
        d_sqlite_transaction_mode _mode
    ) D_NOEXCEPT
    {
        return d_sqlite_begin_transaction(m_db, _mode);
    }

    int
    commit() D_NOEXCEPT
    {
        return d_sqlite_commit(m_db);
    }

    int
    rollback() D_NOEXCEPT
    {
        return d_sqlite_rollback(m_db);
    }

    D_NODISCARD bool
    in_transaction() const D_NOEXCEPT
    {
        return d_sqlite_in_transaction(m_db);
    }

    int
    create_savepoint(
        string_arg _name
    ) D_NOEXCEPT
    {
        return d_sqlite_create_savepoint(m_db, _name.c_str());
    }

    int
    release_savepoint(
        string_arg _name
    ) D_NOEXCEPT
    {
        return d_sqlite_release_savepoint(m_db, _name.c_str());
    }

    int
    rollback_to_savepoint(
        string_arg _name
    ) D_NOEXCEPT
    {
        return d_sqlite_rollback_to_savepoint(m_db, _name.c_str());
    }

    // 1.2.1.9
    // Settings
    //   sqlite_settings.h.
    int
    set_foreign_keys(
        bool _enabled
    ) D_NOEXCEPT
    {
        return d_sqlite_set_foreign_keys(m_db, _enabled);
    }

    int
    get_foreign_keys(
        bool& _enabled
    ) const D_NOEXCEPT
    {
        return d_sqlite_get_foreign_keys(m_db, &_enabled);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<bool, int>
    get_foreign_keys() const D_NOEXCEPT
    {
        bool      value{};
        const int result = d_sqlite_get_foreign_keys(m_db, &value);

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    set_busy_timeout(
        int _milliseconds
    ) D_NOEXCEPT
    {
        return d_sqlite_set_busy_timeout(m_db, _milliseconds);
    }

    int
    get_busy_timeout(
        int& _milliseconds
    ) const D_NOEXCEPT
    {
        return d_sqlite_get_busy_timeout(m_db, &_milliseconds);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<int, int>
    get_busy_timeout() const D_NOEXCEPT
    {
        int       value{};
        const int result = d_sqlite_get_busy_timeout(m_db, &value);

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    set_journal_mode(
        string_arg            _schema,
        d_sqlite_journal_mode _mode
    ) D_NOEXCEPT
    {
        return d_sqlite_set_journal_mode(m_db, _schema.c_str(), _mode);
    }

    int
    get_journal_mode(
        string_arg             _schema,
        d_sqlite_journal_mode& _mode
    ) const D_NOEXCEPT
    {
        return d_sqlite_get_journal_mode(m_db, _schema.c_str(), &_mode);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<d_sqlite_journal_mode, int>
    get_journal_mode(
        string_arg _schema
    ) const D_NOEXCEPT
    {
        d_sqlite_journal_mode value{};
        const int             result = d_sqlite_get_journal_mode(
            m_db,
            _schema.c_str(),
            &value);

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    set_synchronous(
        string_arg           _schema,
        d_sqlite_synchronous _value
    ) D_NOEXCEPT
    {
        return d_sqlite_set_synchronous(m_db, _schema.c_str(), _value);
    }

    int
    get_synchronous(
        string_arg            _schema,
        d_sqlite_synchronous& _value
    ) const D_NOEXCEPT
    {
        return d_sqlite_get_synchronous(m_db, _schema.c_str(), &_value);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<d_sqlite_synchronous, int>
    get_synchronous(
        string_arg _schema
    ) const D_NOEXCEPT
    {
        d_sqlite_synchronous value{};
        const int            result = d_sqlite_get_synchronous(m_db,
                                                               _schema.c_str(),
                                                               &value);

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    set_user_version(
        string_arg      _schema,
        re_std::int32_t _version
    ) D_NOEXCEPT
    {
        return d_sqlite_set_user_version(m_db, _schema.c_str(), _version);
    }

    int
    get_user_version(
        string_arg       _schema,
        re_std::int32_t& _version
    ) const D_NOEXCEPT
    {
        return d_sqlite_get_user_version(m_db, _schema.c_str(), &_version);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<re_std::int32_t, int>
    get_user_version(
        string_arg _schema
    ) const D_NOEXCEPT
    {
        re_std::int32_t value{};
        const int       result = d_sqlite_get_user_version(m_db,
                                                           _schema.c_str(),
                                                           &value);

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    set_application_id(
        string_arg      _schema,
        re_std::int32_t _id
    ) D_NOEXCEPT
    {
        return d_sqlite_set_application_id(m_db, _schema.c_str(), _id);
    }

    int
    get_application_id(
        string_arg       _schema,
        re_std::int32_t& _id
    ) const D_NOEXCEPT
    {
        return d_sqlite_get_application_id(m_db, _schema.c_str(), &_id);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<re_std::int32_t, int>
    get_application_id(
        string_arg _schema
    ) const D_NOEXCEPT
    {
        re_std::int32_t value{};
        const int       result = d_sqlite_get_application_id(m_db,
                                                             _schema.c_str(),
                                                             &value);

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    set_pragma(
        string_arg _schema,
        string_arg _pragma,
        string_arg _value
    ) D_NOEXCEPT
    {
        return d_sqlite_set_pragma(m_db,
                                   _schema.c_str(),
                                   _pragma.c_str(),
                                   _value.c_str());
    }

    int
    get_pragma(
        string_arg _schema,
        string_arg _pragma,
        text&      _value
    ) const D_NOEXCEPT
    {
        return d_sqlite_get_pragma(m_db,
                                   _schema.c_str(),
                                   _pragma.c_str(),
                                   _value.out());
    }

    int
    get_pragma(
        string_arg   _schema,
        string_arg   _pragma,
        std::string& _value
    ) const
    {
        text      text_value;
        const int result = d_sqlite_get_pragma(m_db,
                                               _schema.c_str(),
                                               _pragma.c_str(),
                                               text_value.out());

        _value = text_value.str();

        return result;
    }

#if D_INTERNAL_DB_FACE_OPTIONAL
    int
    get_pragma(
        string_arg                  _schema,
        string_arg                  _pragma,
        std::optional<std::string>& _value
    ) const
    {
        text      text_value;
        const int result = d_sqlite_get_pragma(m_db,
                                               _schema.c_str(),
                                               _pragma.c_str(),
                                               text_value.out());

        // the text; nothing for SQL NULL, and for a failure
        if (text_value.is_null())
        {
            _value.reset();
        }
        else
        {
            _value = text_value.str();
        }

        return result;
    }
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<text, int>
    get_pragma(
        string_arg _schema,
        string_arg _pragma
    ) const D_NOEXCEPT
    {
        text      value{};
        const int result = d_sqlite_get_pragma(m_db,
                                               _schema.c_str(),
                                               _pragma.c_str(),
                                               value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    // 1.2.1.10
    // Maintenance
    //   sqlite_maintenance.h, and copying a database from face to face.
    int
    vacuum(
        string_arg _schema
    ) D_NOEXCEPT
    {
        return d_sqlite_vacuum(m_db, _schema.c_str());
    }

    int
    vacuum_into(
        string_arg _schema,
        string_arg _path
    ) D_NOEXCEPT
    {
        return d_sqlite_vacuum_into(m_db, _schema.c_str(), _path.c_str());
    }

    int
    analyze(
        string_arg _schema
    ) D_NOEXCEPT
    {
        return d_sqlite_analyze(m_db, _schema.c_str());
    }

    int
    optimize(
        string_arg _schema
    ) D_NOEXCEPT
    {
        return d_sqlite_optimize(m_db, _schema.c_str());
    }

    int
    integrity_check(
        string_arg _schema,
        bool&      _ok
    ) const D_NOEXCEPT
    {
        return d_sqlite_integrity_check(m_db, _schema.c_str(), &_ok);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<bool, int>
    integrity_check(
        string_arg _schema
    ) const D_NOEXCEPT
    {
        bool      value{};
        const int result = d_sqlite_integrity_check(m_db,
                                                    _schema.c_str(),
                                                    &value);

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    quick_check(
        string_arg _schema,
        bool&      _ok
    ) const D_NOEXCEPT
    {
        return d_sqlite_quick_check(m_db, _schema.c_str(), &_ok);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<bool, int>
    quick_check(
        string_arg _schema
    ) const D_NOEXCEPT
    {
        bool      value{};
        const int result = d_sqlite_quick_check(m_db, _schema.c_str(), &value);

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    checkpoint(
        string_arg               _schema,
        d_sqlite_checkpoint_mode _mode
    ) D_NOEXCEPT
    {
        return d_sqlite_checkpoint(m_db, _schema.c_str(), _mode);
    }

    int
    backup_to_file(
        string_arg _schema,
        string_arg _path
    ) D_NOEXCEPT
    {
        return d_sqlite_backup_to_file(m_db, _schema.c_str(), _path.c_str());
    }

    int
    restore_from_file(
        string_arg _schema,
        string_arg _path
    ) D_NOEXCEPT
    {
        return d_sqlite_restore_from_file(m_db, _schema.c_str(), _path.c_str());
    }

    /**
     * @brief Copies one face's database over another's, as
     *        d_sqlite_copy_database does.
     *
     * @param[in] _source              the face to copy from.
     * @param[in] _source_schema       its database; `NULL` for "main".
     * @param[in] _destination         the face to copy to; another one.
     * @param[in] _destination_schema  its database; `NULL` for "main".
     * @return d_sqlite_copy_database's result.
     * @post On success the destination database is a copy of the source.
     */
    static int
    copy_database(
        sqlite&    _source,
        string_arg _source_schema,
        sqlite&    _destination,
        string_arg _destination_schema
    ) D_NOEXCEPT
    {
        return d_sqlite_copy_database(_source.m_db,
                                      _source_schema.c_str(),
                                      _destination.m_db,
                                      _destination_schema.c_str());
    }

private:
    D_DELETED_FN_MSG(sqlite(const sqlite&),
                     "a face owns its connection; move it")
    D_DELETED_FN_MSG(sqlite& operator=(const sqlite&),
                     "a face owns its connection; move it")

    // m_take -- an open's result; the connection it made replaces the
    // face's when it succeeded. Each open calls the layer first, in a
    // statement of its own, since the order a call's arguments are
    // evaluated in is the compiler's.
    int
    m_take(
        int             _result,
        struct sqlite3* _opened
    ) D_NOEXCEPT
    {
        // a failed open made nothing to keep
        if (_result == 0)
        {
            reset(_opened);
        }

        return _result;
    }

    struct sqlite3* m_db;
};

// 1.3    Swapping
//------------------------------------------------------------------------------
// swapping -- the two connections, or statements, trade places, as std::swap
// would have them, for code that swaps unqualified
inline void
swap(
    sqlite_statement& _left,
    sqlite_statement& _right
) D_NOEXCEPT
{
    _left.swap(_right);

    return;
}

inline void
swap(
    sqlite& _left,
    sqlite& _right
) D_NOEXCEPT
{
    _left.swap(_right);

    return;
}


//==============================================================================
// 2.  COST
//==============================================================================


// 2.1    Sizes and moves
//------------------------------------------------------------------------------
// the face is its connection, and a statement its handle, and from C++11
// each moves without throwing (db_face.hpp, 6)
D_STATIC_ASSERT(sizeof(sqlite_statement) == sizeof(struct sqlite3_stmt*),
                "face::sqlite_statement is its statement, and no more");
D_STATIC_ASSERT(sizeof(sqlite) == sizeof(struct sqlite3*),
                "face::sqlite is its connection, and no more");
#if D_MOVE_ENABLED
D_STATIC_ASSERT(
    ( (std::is_nothrow_move_constructible<sqlite_statement>::value) &&
      (std::is_nothrow_move_assignable<sqlite_statement>::value) ),
    "face::sqlite_statement moves without throwing");
D_STATIC_ASSERT( ( (std::is_nothrow_move_constructible<sqlite>::value) &&
                   (std::is_nothrow_move_assignable<sqlite>::value) ),
                 "face::sqlite moves without throwing");
#endif  // D_MOVE_ENABLED


}  // namespace face
}  // namespace djinterp


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_DB_SQLITE_SQLITE_FACE_HPP
