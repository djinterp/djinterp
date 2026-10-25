/*******************************************************************************
* djinterp [c]                                                  mariadb_face.hpp
*
* The C++ face of djinterp's MariaDB and MySQL layer.
*   face::mariadb owns a connection -- the layer's handle, struct d_mariadb,
* over the client's MYSQL -- and closes it when it goes; every function of
* the layer (mariadb.h) is a member of the same name, the connection
* argument dropped, each with its C function's contract:
*     const d_mariadb_options where = { "db.example", "app", secret, "shop",
*                                       0u, 0 };
*     face::mariadb db;
*     if (db.connect(&where) == 0)
*     {
*         face::transaction<face::mariadb> work(db);
*
*         db.create_table_sql(0, "notes", "id INT PRIMARY KEY, body TEXT",
*                             D_DB_IF_NOT_EXISTS);
*         work.commit();
*     }
* One face for both servers, through either client library. Results are the
* layer's: 0, the client's error number, or a negative D_DB_*. Functions
* that need no connection -- the client's version, quoting a name -- are
* static. db_face.hpp describes what the face adds, how strings, lists and
* text cross it, and what each language level adds beside the exact forms:
* from C++17 std::optional text, from C++20 formats checked at compile time
* and std::span columns, from C++23 std::expected results.
*   connect, connect_with and adopt replace the connection only when they
* succeed; a failure leaves the face as it was. A member called on a face
* with no connection passes NULL to the layer, which answers
* D_DB_INVALID_ARGUMENT, or false, 0 or NULL.
*   face::mariadb_statement owns a prepared statement, the layer's struct
* d_mariadb_statement over the client's MYSQL_STMT, and finalizes it when it
* goes; prepare and preparef fill one, and every function of
* mariadb_statement.h that takes a statement is its member, the statement
* argument dropped:
*     face::mariadb_statement insert;
*     if (db.prepare(insert, "INSERT INTO notes (body) VALUES (?)") == 0)
*     {
*         re_std::int64_t changes = 0;
*
*         insert.bind_text(1, "first");
*         insert.execute_update(changes);
*     }
* face::rows (db_face.hpp) holds the rows a query or a statement returned.
* Finalize a statement before closing its connection.
*   The members that forward are generated, by tools/gen_db_face.py from the
* layer's prototypes and tools/db_face/mariadb_face.hpp.in, and keep the C
* functions' contracts, in the headers each part names; those written by
* hand in the template -- the ones that connect, close or hand over the
* connection or the statement -- carry their own. Edit the template, not
* this file.
*
*
* path:      /inc/djinterp/c/db/mariadb/mariadb_face.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.14
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FACE
    ----
    1.  The statement
         1.  mariadb_statement
              1.  Ownership
              2.  Describing, binding and running
    2.  The connection
         1.  mariadb
              1.  Ownership
              2.  Library
              3.  Servers and errors
              4.  Connections
              5.  Queries
              6.  Statements
              7.  Databases
              8.  Schema
              9.  Transactions
              10. Settings
              11. Maintenance
    3.  Swapping
2.  COST
    ----
    1.  Sizes and moves
*/

#ifndef DJINTERP_C_DB_MARIADB_MARIADB_FACE_HPP
#define DJINTERP_C_DB_MARIADB_MARIADB_FACE_HPP 1

// std
#include <cstddef>  // std::size_t
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "../db_face.hpp"  // string_arg, text, the lists, the levels
#include "./mariadb.h"     // the layer
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
// mariadb_statement
//   class: a prepared statement -- the layer's struct d_mariadb_statement,
// over the client's MYSQL_STMT -- owned: finalized when the object goes.
// Made empty, and filled by face::mariadb's prepare and preparef; every
// function of mariadb_statement.h that takes a statement is a member of the
// same name, whose contract it keeps. Finalize it before closing its
// connection. Not copyable; movable from C++11.
class mariadb_statement
{
public:
    // 1.1.1.1
    // Ownership
    //   a statement made empty holds none; whatever a statement holds, it
    // finalizes when it goes.
    D_CONSTEXPR mariadb_statement() D_NOEXCEPT
        : m_statement(D_NULLPTR)
    {}

    /**
     * @brief Makes a statement that owns one prepared elsewhere.
     *
     * @param[in] _owned  the statement, or `NULL` for none.
     * @post The object finalizes `_owned`; the caller no longer does.
     */
    explicit D_CONSTEXPR mariadb_statement(
        struct d_mariadb_statement* _owned
    ) D_NOEXCEPT
        : m_statement(_owned)
    {}

    ~mariadb_statement()
    {
        // a statement, when there is one; an empty object calls nothing
        if (m_statement)
        {
            (void)d_mariadb_finalize(m_statement);
        }
    }

#if D_MOVE_ENABLED
    // moving -- the statement passes to the object moved to, which
    // finalizes what it held; the object moved from holds none
    mariadb_statement(
        mariadb_statement&& _other
    ) D_NOEXCEPT
        : m_statement(_other.release())
    {}

    mariadb_statement&
    operator=(
        mariadb_statement&& _other
    ) D_NOEXCEPT
    {
        reset(_other.release());

        return *this;
    }
#endif  // D_MOVE_ENABLED

    // the handle -- the statement, for the layer's own calls, NULL when
    // there is none; and whether there is one
    D_NODISCARD struct d_mariadb_statement*
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
     *         d_mariadb_finalize; `NULL` when there is none.
     * @post The object holds no statement.
     */
    D_NODISCARD struct d_mariadb_statement*
    release() D_NOEXCEPT
    {
        struct d_mariadb_statement* const held = m_statement;

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
        struct d_mariadb_statement* _owned = D_NULLPTR
    ) D_NOEXCEPT
    {
        // what is held is finalized, unless it is what comes
        if (_owned != m_statement)
        {
            // a statement, when there is one
            if (m_statement)
            {
                (void)d_mariadb_finalize(m_statement);
            }

            m_statement = _owned;
        }

        return;
    }

    /**
     * @brief Finalizes the statement, as d_mariadb_finalize does.
     *
     * @return d_mariadb_finalize's result: 0, or the client's error
     *         number when the server could not be told.
     * @post The object holds no statement, whatever the result.
     */
    int
    finalize() D_NOEXCEPT
    {
        return d_mariadb_finalize(release());
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
    struct d_mariadb_statement**
    out() D_NOEXCEPT
    {
        reset();

        return &m_statement;
    }

    // swap -- the two objects trade statements
    void
    swap(
        mariadb_statement& _other
    ) D_NOEXCEPT
    {
        struct d_mariadb_statement* const held = m_statement;

        m_statement        = _other.m_statement;
        _other.m_statement = held;

        return;
    }

    // 1.1.1.2
    // Describing, binding and running
    //   mariadb_statement.h's functions that take a statement.
    D_NODISCARD std::size_t
    parameter_count() const D_NOEXCEPT
    {
        return d_mariadb_parameter_count(m_statement);
    }

    D_NODISCARD d_mariadb*
    statement_connection() const D_NOEXCEPT
    {
        return d_mariadb_statement_connection(m_statement);
    }

    int
    bind_null(
        std::size_t _index
    ) D_NOEXCEPT
    {
        return d_mariadb_bind_null(m_statement, _index);
    }

    int
    bind_bool(
        std::size_t _index,
        bool        _value
    ) D_NOEXCEPT
    {
        return d_mariadb_bind_bool(m_statement, _index, _value);
    }

    int
    bind_int64(
        std::size_t     _index,
        re_std::int64_t _value
    ) D_NOEXCEPT
    {
        return d_mariadb_bind_int64(m_statement, _index, _value);
    }

    int
    bind_double(
        std::size_t _index,
        double      _value
    ) D_NOEXCEPT
    {
        return d_mariadb_bind_double(m_statement, _index, _value);
    }

    int
    bind_text(
        std::size_t _index,
        string_arg  _text
    ) D_NOEXCEPT
    {
        return d_mariadb_bind_text(m_statement, _index, _text.c_str());
    }

    int
    bind_blob(
        std::size_t _index,
        const void* _bytes,
        std::size_t _size
    ) D_NOEXCEPT
    {
        return d_mariadb_bind_blob(m_statement, _index, _bytes, _size);
    }

    int
    clear_bindings() D_NOEXCEPT
    {
        return d_mariadb_clear_bindings(m_statement);
    }

    int
    execute_update(
        re_std::int64_t& _changes
    ) D_NOEXCEPT
    {
        return d_mariadb_execute_update(m_statement, &_changes);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<re_std::int64_t, int>
    execute_update() D_NOEXCEPT
    {
        re_std::int64_t value{};
        const int       result = d_mariadb_execute_update(m_statement, &value);

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
        return d_mariadb_execute_query(m_statement, _rows.out());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<rows, int>
    execute_query() D_NOEXCEPT
    {
        rows      value{};
        const int result = d_mariadb_execute_query(m_statement, value.out());

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
        return d_mariadb_open_cursor(m_statement);
    }

    int
    fetch_rows(
        std::size_t _max,
        rows&       _rows
    ) D_NOEXCEPT
    {
        return d_mariadb_fetch_rows(m_statement, _max, _rows.out());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<rows, int>
    fetch_rows(
        std::size_t _max
    ) D_NOEXCEPT
    {
        rows      value{};
        const int result = d_mariadb_fetch_rows(m_statement, _max, value.out());

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
        return d_mariadb_close_cursor(m_statement);
    }

private:
    D_DELETED_FN_MSG(mariadb_statement(const mariadb_statement&),
                     "a statement owns what the server prepared; move it")
    D_DELETED_FN_MSG(mariadb_statement& operator=(const mariadb_statement&),
                     "a statement owns what the server prepared; move it")

    struct d_mariadb_statement* m_statement;
};

// 1.2    The connection
//------------------------------------------------------------------------------
// 1.2.1
// mariadb
//   class: a MariaDB or MySQL connection, owned: closed when the object goes.
// Every member forwards to the layer's function of the same name, whose
// contract it keeps. Not copyable; movable from C++11.
class mariadb
{
public:
    // transaction_mode
    //   type: how a transaction begins, for face::transaction.
    typedef enum d_mariadb_transaction_mode transaction_mode;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // format
    //   type: an f form's format, for the arguments that follow it: a
    // literal in the shared dialect, read at compile time from C++20, or a
    // format runtime_format marked (db_face.hpp, 2.3).
    template<typename... Args>
    using format = format_arg<internal::shared_dialect, Args...>;
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    // 1.2.1.1
    // Ownership
    //   a face made empty holds no connection; whatever a face holds, it
    // closes when it goes.
    D_CONSTEXPR mariadb() D_NOEXCEPT
        : m_db(D_NULLPTR)
    {}

    /**
     * @brief Makes a face that owns a handle made elsewhere: by the layer,
     *        or by d_mariadb_adopt.
     *
     * @param[in] _owned  the handle, or `NULL` for none.
     * @post The face closes `_owned`; the caller no longer does.
     */
    explicit D_CONSTEXPR mariadb(
        struct d_mariadb* _owned
    ) D_NOEXCEPT
        : m_db(_owned)
    {}

    ~mariadb()
    {
        // a connection, when there is one; an empty face calls nothing
        if (m_db)
        {
            (void)d_mariadb_close(m_db);
        }
    }

#if D_MOVE_ENABLED
    // moving -- the connection passes to the face moved to, which closes
    // what it held; the face moved from holds none
    mariadb(
        mariadb&& _other
    ) D_NOEXCEPT
        : m_db(_other.release())
    {}

    mariadb&
    operator=(
        mariadb&& _other
    ) D_NOEXCEPT
    {
        reset(_other.release());

        return *this;
    }
#endif  // D_MOVE_ENABLED

    // the handle -- the layer's handle, for its calls, NULL when there is
    // none (native() is the client's MYSQL*); and whether there is one
    D_NODISCARD struct d_mariadb*
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
     * @brief Gives up the handle without closing it.
     *
     * @return the handle, which the caller then closes with
     *         d_mariadb_close; `NULL` when there is none.
     * @post The face holds no connection.
     */
    D_NODISCARD struct d_mariadb*
    release() D_NOEXCEPT
    {
        struct d_mariadb* const held = m_db;

        m_db = D_NULLPTR;

        return held;
    }

    /**
     * @brief Closes the handle held, if any, and takes another.
     *
     * @param[in] _owned  the handle to own, or `NULL` for none; the one
     *                    already held is kept, not closed.
     * @post The handle that was held, if another, is closed, and every
     *       pointer obtained through it is invalid.
     */
    void
    reset(
        struct d_mariadb* _owned = D_NULLPTR
    ) D_NOEXCEPT
    {
        // what is held closes, unless it is what comes
        if (_owned != m_db)
        {
            // a connection, when there is one
            if (m_db)
            {
                (void)d_mariadb_close(m_db);
            }

            m_db = _owned;
        }

        return;
    }

    // swap -- the two faces trade connections
    void
    swap(
        mariadb& _other
    ) D_NOEXCEPT
    {
        struct d_mariadb* const held = m_db;

        m_db        = _other.m_db;
        _other.m_db = held;

        return;
    }

    // default_transaction_mode -- how face::transaction begins when told
    // nothing: READ WRITE
    D_NODISCARD static D_CONSTEXPR transaction_mode
    default_transaction_mode() D_NOEXCEPT
    {
        return D_MARIADB_TRANSACTION_READ_WRITE;
    }

    // 1.2.1.2
    // Library
    //   mariadb_common.h's client versions and result messages: static,
    // since they need no connection.
    D_NODISCARD static const char*
    client_version() D_NOEXCEPT
    {
        return d_mariadb_client_version();
    }

    D_NODISCARD static unsigned long
    client_version_number() D_NOEXCEPT
    {
        return d_mariadb_client_version_number();
    }

    D_NODISCARD static bool
    client_is_mariadb() D_NOEXCEPT
    {
        return d_mariadb_client_is_mariadb();
    }

    D_NODISCARD static const char*
    result_message(
        int _result
    ) D_NOEXCEPT
    {
        return d_mariadb_result_message(_result);
    }

    // 1.2.1.3
    // Servers and errors
    //   mariadb_common.h's server flavor, versions and features, the
    // client's own handle, and the connection's latest error.
    D_NODISCARD d_mariadb_flavor
    server_flavor() const D_NOEXCEPT
    {
        return d_mariadb_server_flavor(m_db);
    }

    D_NODISCARD const char*
    server_version() const D_NOEXCEPT
    {
        return d_mariadb_server_version(m_db);
    }

    D_NODISCARD unsigned long
    server_version_number() const D_NOEXCEPT
    {
        return d_mariadb_server_version_number(m_db);
    }

    D_NODISCARD bool
    server_at_least(
        d_mariadb_flavor _flavor,
        unsigned         _major,
        unsigned         _minor,
        unsigned         _patch
    ) const D_NOEXCEPT
    {
        return d_mariadb_server_at_least(m_db, _flavor, _major, _minor, _patch);
    }

    D_NODISCARD bool
    supports(
        d_mariadb_feature _feature
    ) const D_NOEXCEPT
    {
        return d_mariadb_supports(m_db, _feature);
    }

    D_NODISCARD void*
    native() const D_NOEXCEPT
    {
        return d_mariadb_native(m_db);
    }

    D_NODISCARD unsigned
    error_code() const D_NOEXCEPT
    {
        return d_mariadb_error_code(m_db);
    }

    D_NODISCARD const char*
    error_message() const D_NOEXCEPT
    {
        return d_mariadb_error_message(m_db);
    }

    D_NODISCARD const char*
    sqlstate() const D_NOEXCEPT
    {
        return d_mariadb_sqlstate(m_db);
    }

    // 1.2.1.4
    // Connections
    //   mariadb_connection.h.

    /**
     * @brief Connects to a server, as d_mariadb_connect does, in place of
     *        the connection held.
     *
     * @param[in] _options  where and as whom; `NULL` connects with every
     *                      field defaulted.
     * @return d_mariadb_connect's result.
     * @post On success the face holds the new connection, and the one it
     *       held is closed; on failure the face is as it was.
     */
    int
    connect(
        const struct d_mariadb_options* _options
    ) D_NOEXCEPT
    {
        struct d_mariadb* made   = D_NULLPTR;
        const int         result = d_mariadb_connect(_options, &made);

        return m_take(result, made);
    }

    /**
     * @brief Connects to a server with client settings applied first, as
     *        d_mariadb_connect_with does, in place of the connection held.
     *
     * @param[in] _options   where and as whom; `NULL` connects with every
     *                       field defaulted.
     * @param[in] _settings  the settings, applied in order.
     * @param[in] _count     how many there are.
     * @return d_mariadb_connect_with's result.
     * @post As for connect.
     */
    int
    connect_with(
        const struct d_mariadb_options* _options,
        const struct d_mariadb_setting* _settings,
        std::size_t                     _count
    ) D_NOEXCEPT
    {
        struct d_mariadb* made   = D_NULLPTR;
        const int         result = d_mariadb_connect_with(_options,
                                                          _settings,
                                                          _count,
                                                          &made,
                                                          D_NULLPTR);

        return m_take(result, made);
    }

    /**
     * @brief Connects as connect_with does, and says why a connection could
     *        not be made.
     *
     * @param[in]  _options   where and as whom; `NULL` connects with every
     *                        field defaulted.
     * @param[in]  _settings  the settings, applied in order.
     * @param[in]  _count     how many there are.
     * @param[out] _message   the reason when no connection was made, in the
     *                        client's words or the layer's; null otherwise.
     * @return d_mariadb_connect_with's result.
     * @post As for connect.
     */
    int
    connect_with(
        const struct d_mariadb_options* _options,
        const struct d_mariadb_setting* _settings,
        std::size_t                     _count,
        text&                           _message
    ) D_NOEXCEPT
    {
        struct d_mariadb* made   = D_NULLPTR;
        const int         result = d_mariadb_connect_with(_options,
                                                          _settings,
                                                          _count,
                                                          &made,
                                                          _message.out());

        return m_take(result, made);
    }

    // connect_with -- the settings as an array, its size the count; from
    // C++20 as a std::span; each with the reason too
    template<std::size_t Count>
    int
    connect_with(
        const struct d_mariadb_options* _options,
        const struct d_mariadb_setting  (&_settings)[Count]
    ) D_NOEXCEPT
    {
        return connect_with(_options, _settings, Count);
    }

    template<std::size_t Count>
    int
    connect_with(
        const struct d_mariadb_options* _options,
        const struct d_mariadb_setting  (&_settings)[Count],
        text&                           _message
    ) D_NOEXCEPT
    {
        return connect_with(_options, _settings, Count, _message);
    }

#if D_INTERNAL_DB_FACE_SPAN
    int
    connect_with(
        const struct d_mariadb_options*           _options,
        std::span<const struct d_mariadb_setting> _settings
    ) noexcept
    {
        return connect_with(_options, _settings.data(), _settings.size());
    }

    int
    connect_with(
        const struct d_mariadb_options*           _options,
        std::span<const struct d_mariadb_setting> _settings,
        text&                                     _message
    ) noexcept
    {
        return connect_with(_options,
                            _settings.data(),
                            _settings.size(),
                            _message);
    }
#endif  // D_INTERNAL_DB_FACE_SPAN

    /**
     * @brief Wraps a connection made with the client's own calls, as
     *        d_mariadb_adopt does, in place of the connection held.
     *
     * @param[in] _native  a connected MYSQL*, which is left as it is.
     * @param[in] _owned   whether closing the face closes it as well.
     * @return d_mariadb_adopt's result.
     * @post As for connect; on failure `_native` is the caller's, whatever
     *       `_owned` said.
     */
    int
    adopt(
        void* _native,
        bool  _owned
    ) D_NOEXCEPT
    {
        struct d_mariadb* made   = D_NULLPTR;
        const int         result = d_mariadb_adopt(_native, _owned, &made);

        return m_take(result, made);
    }

    /**
     * @brief Closes the connection, as d_mariadb_close does.
     *
     * @return 0.
     * @post The face holds no connection.
     */
    int
    close() D_NOEXCEPT
    {
        return d_mariadb_close(release());
    }

    int
    change_user(
        string_arg _user,
        string_arg _password,
        string_arg _database
    ) D_NOEXCEPT
    {
        return d_mariadb_change_user(m_db,
                                     _user.c_str(),
                                     _password.c_str(),
                                     _database.c_str());
    }

    int
    reset_connection() D_NOEXCEPT
    {
        return d_mariadb_reset_connection(m_db);
    }

    int
    ping() D_NOEXCEPT
    {
        return d_mariadb_ping(m_db);
    }

    D_NODISCARD unsigned long
    connection_id() const D_NOEXCEPT
    {
        return d_mariadb_connection_id(m_db);
    }

    int
    server_status(
        text& _status
    ) const D_NOEXCEPT
    {
        return d_mariadb_server_status(m_db, _status.out());
    }

    int
    server_status(
        std::string& _status
    ) const
    {
        text      text_value;
        const int result = d_mariadb_server_status(m_db, text_value.out());

        _status = text_value.str();

        return result;
    }

#if D_INTERNAL_DB_FACE_OPTIONAL
    int
    server_status(
        std::optional<std::string>& _status
    ) const
    {
        text      text_value;
        const int result = d_mariadb_server_status(m_db, text_value.out());

        // the text; nothing for SQL NULL, and for a failure
        if (text_value.is_null())
        {
            _status.reset();
        }
        else
        {
            _status = text_value.str();
        }

        return result;
    }
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<text, int>
    server_status() const D_NOEXCEPT
    {
        text      value{};
        const int result = d_mariadb_server_status(m_db, value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    D_NODISCARD re_std::uint64_t
    last_insert_id() const D_NOEXCEPT
    {
        return d_mariadb_last_insert_id(m_db);
    }

    D_NODISCARD re_std::int64_t
    changes() const D_NOEXCEPT
    {
        return d_mariadb_changes(m_db);
    }

    D_NODISCARD unsigned
    warning_count() const D_NOEXCEPT
    {
        return d_mariadb_warning_count(m_db);
    }

    D_NODISCARD int
    get_option(
        int   _option,
        void* _value
    ) const D_NOEXCEPT
    {
        return d_mariadb_get_option(m_db, _option, _value);
    }

    // 1.2.1.5
    // Queries
    //   mariadb_query.h; the f forms from C++11, their formats checked from
    // C++20 (db_face.hpp, 2.2 to 2.5).
    int
    exec(
        string_arg _sql
    ) D_NOEXCEPT
    {
        return d_mariadb_exec(m_db, _sql.c_str());
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

        return d_mariadb_execf(m_db, _format.c_str(), _args...);
    }
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    int
    exec_script(
        string_arg _sql
    ) D_NOEXCEPT
    {
        return d_mariadb_exec_script(m_db, _sql.c_str());
    }

    int
    exec_changes(
        re_std::int64_t& _changes,
        string_arg       _sql
    ) D_NOEXCEPT
    {
        return d_mariadb_exec_changes(m_db, &_changes, _sql.c_str());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<re_std::int64_t, int>
    exec_changes(
        string_arg _sql
    ) D_NOEXCEPT
    {
        re_std::int64_t value{};
        const int       result = d_mariadb_exec_changes(m_db,
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

        return d_mariadb_execf_changes(m_db,
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
        const int       result = d_mariadb_execf_changes(m_db,
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
        return d_mariadb_query_int64(m_db, &_value, _sql.c_str());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<re_std::int64_t, int>
    query_int64(
        string_arg _sql
    ) D_NOEXCEPT
    {
        re_std::int64_t value{};
        const int       result = d_mariadb_query_int64(m_db,
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

        return d_mariadb_query_int64f(m_db, &_value, _format.c_str(), _args...);
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
        const int       result = d_mariadb_query_int64f(m_db,
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
        return d_mariadb_query_double(m_db, &_value, _sql.c_str());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<double, int>
    query_double(
        string_arg _sql
    ) D_NOEXCEPT
    {
        double    value{};
        const int result = d_mariadb_query_double(m_db, &value, _sql.c_str());

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

        return d_mariadb_query_doublef(m_db,
                                       &_value,
                                       _format.c_str(),
                                       _args...);
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
        const int result = d_mariadb_query_doublef(m_db,
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
        return d_mariadb_query_text(m_db, _value.out(), _sql.c_str());
    }

    int
    query_text(
        std::string& _value,
        string_arg   _sql
    )
    {
        text      text_value;
        const int result = d_mariadb_query_text(m_db,
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
        const int result = d_mariadb_query_text(m_db,
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
        const int result = d_mariadb_query_text(m_db,
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
    query_textf(
        text&           _value,
        format<Args...> _format,
        Args...         _args
    ) D_NOEXCEPT
    {
        internal::check_varargs<Args...>();

        return d_mariadb_query_textf(m_db,
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
        const int result = d_mariadb_query_textf(m_db,
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
        const int result = d_mariadb_query_textf(m_db,
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
        const int result = d_mariadb_query_textf(m_db,
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
        return d_mariadb_query_rows(m_db, _rows.out(), _sql.c_str());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<rows, int>
    query_rows(
        string_arg _sql
    ) D_NOEXCEPT
    {
        rows      value{};
        const int result = d_mariadb_query_rows(m_db,
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
    query_rowsf(
        rows&           _rows,
        format<Args...> _format,
        Args...         _args
    ) D_NOEXCEPT
    {
        internal::check_varargs<Args...>();

        return d_mariadb_query_rowsf(m_db,
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
        const int result = d_mariadb_query_rowsf(m_db,
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
        _quoted.reset(d_mariadb_quote_identifier(_name.c_str()));

        return;
    }

    D_NODISCARD static std::string
    quote_identifier(
        string_arg _name
    )
    {
        const text result(d_mariadb_quote_identifier(_name.c_str()));

        return result.str();
    }

    void
    quote_literal(
        string_arg _text,
        text&      _quoted
    ) const D_NOEXCEPT
    {
        _quoted.reset(d_mariadb_quote_literal(m_db, _text.c_str()));

        return;
    }

    D_NODISCARD std::string
    quote_literal(
        string_arg _text
    ) const
    {
        const text result(d_mariadb_quote_literal(m_db, _text.c_str()));

        return result.str();
    }

    int
    set_query_attribute(
        string_arg _name,
        string_arg _value
    ) D_NOEXCEPT
    {
        return d_mariadb_set_query_attribute(m_db,
                                             _name.c_str(),
                                             _value.c_str());
    }

    int
    clear_query_attributes() D_NOEXCEPT
    {
        return d_mariadb_clear_query_attributes(m_db);
    }

    // 1.2.1.6
    // Statements
    //   mariadb_statement.h's preparing, into a
    // face::mariadb_statement, whose members its other functions are.
    int
    prepare(
        mariadb_statement& _statement,
        string_arg         _sql
    ) D_NOEXCEPT
    {
        return d_mariadb_prepare(m_db, _statement.out(), _sql.c_str());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<mariadb_statement, int>
    prepare(
        string_arg _sql
    ) D_NOEXCEPT
    {
        mariadb_statement value{};
        const int         result = d_mariadb_prepare(m_db,
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
        mariadb_statement& _statement,
        format<Args...>    _format,
        Args...            _args
    ) D_NOEXCEPT
    {
        internal::check_varargs<Args...>();

        return d_mariadb_preparef(m_db,
                                  _statement.out(),
                                  _format.c_str(),
                                  _args...);
    }
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_INTERNAL_DB_FACE_EXPECTED
    template<typename... Args>
    D_NODISCARD std::expected<mariadb_statement, int>
    preparef(
        format<Args...> _format,
        Args...         _args
    ) D_NOEXCEPT
    {
        internal::check_varargs<Args...>();

        mariadb_statement value{};
        const int         result = d_mariadb_preparef(m_db,
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

    // 1.2.1.7
    // Databases
    //   mariadb_database.h.
    D_NODISCARD bool
    has_database(
        string_arg _name
    ) const D_NOEXCEPT
    {
        return d_mariadb_has_database(m_db, _name.c_str());
    }

    int
    has_database_s(
        string_arg _name,
        bool&      _exists
    ) const D_NOEXCEPT
    {
        return d_mariadb_has_database_s(m_db, _name.c_str(), &_exists);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<bool, int>
    has_database_s(
        string_arg _name
    ) const D_NOEXCEPT
    {
        bool      value{};
        const int result = d_mariadb_has_database_s(m_db,
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

    int
    create_database(
        string_arg _name,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        return d_mariadb_create_database(m_db, _name.c_str(), _flags);
    }

    int
    drop_database(
        string_arg _name,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        return d_mariadb_drop_database(m_db, _name.c_str(), _flags);
    }

    int
    list_databases(
        name_list& _names
    ) const D_NOEXCEPT
    {
        return d_mariadb_list_databases(m_db, _names.out());
    }

    int
    list_databases(
        std::vector<std::string>& _names
    ) const
    {
        name_list names;
        const int result = d_mariadb_list_databases(m_db, names.out());

        names.to_vector(_names);

        return result;
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<name_list, int>
    list_databases() const D_NOEXCEPT
    {
        name_list value{};
        const int result = d_mariadb_list_databases(m_db, value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    use_database(
        string_arg _name
    ) D_NOEXCEPT
    {
        return d_mariadb_use_database(m_db, _name.c_str());
    }

    int
    current_database(
        text& _name
    ) const D_NOEXCEPT
    {
        return d_mariadb_current_database(m_db, _name.out());
    }

    int
    current_database(
        std::string& _name
    ) const
    {
        text      text_value;
        const int result = d_mariadb_current_database(m_db, text_value.out());

        _name = text_value.str();

        return result;
    }

#if D_INTERNAL_DB_FACE_OPTIONAL
    int
    current_database(
        std::optional<std::string>& _name
    ) const
    {
        text      text_value;
        const int result = d_mariadb_current_database(m_db, text_value.out());

        // the text; nothing for SQL NULL, and for a failure
        if (text_value.is_null())
        {
            _name.reset();
        }
        else
        {
            _name = text_value.str();
        }

        return result;
    }
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<text, int>
    current_database() const D_NOEXCEPT
    {
        text      value{};
        const int result = d_mariadb_current_database(m_db, value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    // 1.2.1.8
    // Schema
    //   mariadb_schema.h.
    D_NODISCARD bool
    has_table(
        string_arg _schema,
        string_arg _table
    ) const D_NOEXCEPT
    {
        return d_mariadb_has_table(m_db, _schema.c_str(), _table.c_str());
    }

    int
    has_table_s(
        string_arg _schema,
        string_arg _table,
        bool&      _exists
    ) const D_NOEXCEPT
    {
        return d_mariadb_has_table_s(m_db,
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
        const int result = d_mariadb_has_table_s(m_db,
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
        return d_mariadb_create_table(m_db,
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
        return d_mariadb_create_table(m_db,
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
        return d_mariadb_create_table(m_db,
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
        return d_mariadb_create_table_sql(m_db,
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
        return d_mariadb_drop_table(m_db,
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
        return d_mariadb_rename_table(m_db,
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
        return d_mariadb_truncate_table(m_db, _schema.c_str(), _table.c_str());
    }

    int
    count_rows(
        string_arg       _schema,
        string_arg       _table,
        re_std::int64_t& _count
    ) const D_NOEXCEPT
    {
        return d_mariadb_count_rows(m_db,
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
        const int       result = d_mariadb_count_rows(m_db,
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
        return d_mariadb_list_tables(m_db, _schema.c_str(), _names.out());
    }

    int
    list_tables(
        string_arg                _schema,
        std::vector<std::string>& _names
    ) const
    {
        name_list names;
        const int result = d_mariadb_list_tables(m_db,
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
        const int result = d_mariadb_list_tables(m_db,
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
        return d_mariadb_has_column(m_db,
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
        return d_mariadb_has_column_s(m_db,
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
        const int result = d_mariadb_has_column_s(m_db,
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
        return d_mariadb_add_column(m_db,
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
        return d_mariadb_add_column_sql(m_db,
                                        _schema.c_str(),
                                        _table.c_str(),
                                        _definition.c_str());
    }

    int
    modify_column(
        string_arg         _schema,
        string_arg         _table,
        const d_db_column& _column
    ) D_NOEXCEPT
    {
        return d_mariadb_modify_column(m_db,
                                       _schema.c_str(),
                                       _table.c_str(),
                                       &_column);
    }

    int
    rename_column(
        string_arg _schema,
        string_arg _table,
        string_arg _column,
        string_arg _new_name
    ) D_NOEXCEPT
    {
        return d_mariadb_rename_column(m_db,
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
        return d_mariadb_drop_column(m_db,
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
        return d_mariadb_list_columns(m_db,
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
        const int   result = d_mariadb_list_columns(m_db,
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
        string_arg _index,
        string_arg _table
    ) const D_NOEXCEPT
    {
        return d_mariadb_has_index(m_db,
                                   _schema.c_str(),
                                   _index.c_str(),
                                   _table.c_str());
    }

    int
    has_index_s(
        string_arg _schema,
        string_arg _index,
        string_arg _table,
        bool&      _exists
    ) const D_NOEXCEPT
    {
        return d_mariadb_has_index_s(m_db,
                                     _schema.c_str(),
                                     _index.c_str(),
                                     _table.c_str(),
                                     &_exists);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<bool, int>
    has_index_s(
        string_arg _schema,
        string_arg _index,
        string_arg _table
    ) const D_NOEXCEPT
    {
        bool      value{};
        const int result = d_mariadb_has_index_s(m_db,
                                                 _schema.c_str(),
                                                 _index.c_str(),
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
    create_index(
        string_arg _schema,
        string_arg _index,
        string_arg _table,
        string_arg _columns,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        return d_mariadb_create_index(m_db,
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
        string_arg _table,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        return d_mariadb_drop_index(m_db,
                                    _schema.c_str(),
                                    _index.c_str(),
                                    _table.c_str(),
                                    _flags);
    }

    int
    list_indexes(
        string_arg _schema,
        string_arg _table,
        name_list& _names
    ) const D_NOEXCEPT
    {
        return d_mariadb_list_indexes(m_db,
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
        const int result = d_mariadb_list_indexes(m_db,
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
        const int result = d_mariadb_list_indexes(m_db,
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
        return d_mariadb_has_view(m_db, _schema.c_str(), _view.c_str());
    }

    int
    has_view_s(
        string_arg _schema,
        string_arg _view,
        bool&      _exists
    ) const D_NOEXCEPT
    {
        return d_mariadb_has_view_s(m_db,
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
        const int result = d_mariadb_has_view_s(m_db,
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
        return d_mariadb_create_view(m_db,
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
        return d_mariadb_drop_view(m_db,
                                   _schema.c_str(),
                                   _view.c_str(),
                                   _flags);
    }

    int
    list_views(
        string_arg _schema,
        name_list& _names
    ) const D_NOEXCEPT
    {
        return d_mariadb_list_views(m_db, _schema.c_str(), _names.out());
    }

    int
    list_views(
        string_arg                _schema,
        std::vector<std::string>& _names
    ) const
    {
        name_list names;
        const int result = d_mariadb_list_views(m_db,
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
        const int result = d_mariadb_list_views(m_db,
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
        return d_mariadb_has_trigger(m_db, _schema.c_str(), _trigger.c_str());
    }

    int
    has_trigger_s(
        string_arg _schema,
        string_arg _trigger,
        bool&      _exists
    ) const D_NOEXCEPT
    {
        return d_mariadb_has_trigger_s(m_db,
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
        const int result = d_mariadb_has_trigger_s(m_db,
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
        return d_mariadb_create_trigger(m_db,
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
        return d_mariadb_drop_trigger(m_db,
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
        return d_mariadb_list_triggers(m_db,
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
        const int result = d_mariadb_list_triggers(m_db,
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
        const int result = d_mariadb_list_triggers(m_db,
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

    // 1.2.1.9
    // Transactions
    //   mariadb_transaction.h; face::transaction and face::savepoint guard
    // them.
    int
    begin_transaction(
        d_mariadb_transaction_mode _mode
    ) D_NOEXCEPT
    {
        return d_mariadb_begin_transaction(m_db, _mode);
    }

    int
    commit() D_NOEXCEPT
    {
        return d_mariadb_commit(m_db);
    }

    int
    rollback() D_NOEXCEPT
    {
        return d_mariadb_rollback(m_db);
    }

    D_NODISCARD bool
    in_transaction() const D_NOEXCEPT
    {
        return d_mariadb_in_transaction(m_db);
    }

    int
    create_savepoint(
        string_arg _name
    ) D_NOEXCEPT
    {
        return d_mariadb_create_savepoint(m_db, _name.c_str());
    }

    int
    release_savepoint(
        string_arg _name
    ) D_NOEXCEPT
    {
        return d_mariadb_release_savepoint(m_db, _name.c_str());
    }

    int
    rollback_to_savepoint(
        string_arg _name
    ) D_NOEXCEPT
    {
        return d_mariadb_rollback_to_savepoint(m_db, _name.c_str());
    }

    // 1.2.1.10
    // Settings
    //   mariadb_settings.h.
    int
    set_autocommit(
        bool _enabled
    ) D_NOEXCEPT
    {
        return d_mariadb_set_autocommit(m_db, _enabled);
    }

    int
    get_autocommit(
        bool& _enabled
    ) const D_NOEXCEPT
    {
        return d_mariadb_get_autocommit(m_db, &_enabled);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<bool, int>
    get_autocommit() const D_NOEXCEPT
    {
        bool      value{};
        const int result = d_mariadb_get_autocommit(m_db, &value);

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    set_isolation_level(
        d_mariadb_isolation _level
    ) D_NOEXCEPT
    {
        return d_mariadb_set_isolation_level(m_db, _level);
    }

    int
    get_isolation_level(
        d_mariadb_isolation& _level
    ) const D_NOEXCEPT
    {
        return d_mariadb_get_isolation_level(m_db, &_level);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<d_mariadb_isolation, int>
    get_isolation_level() const D_NOEXCEPT
    {
        d_mariadb_isolation value{};
        const int           result = d_mariadb_get_isolation_level(m_db,
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
    set_next_isolation_level(
        d_mariadb_isolation _level
    ) D_NOEXCEPT
    {
        return d_mariadb_set_next_isolation_level(m_db, _level);
    }

    int
    set_lock_wait_timeout(
        unsigned _seconds
    ) D_NOEXCEPT
    {
        return d_mariadb_set_lock_wait_timeout(m_db, _seconds);
    }

    int
    get_lock_wait_timeout(
        unsigned& _seconds
    ) const D_NOEXCEPT
    {
        return d_mariadb_get_lock_wait_timeout(m_db, &_seconds);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<unsigned, int>
    get_lock_wait_timeout() const D_NOEXCEPT
    {
        unsigned  value{};
        const int result = d_mariadb_get_lock_wait_timeout(m_db, &value);

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    set_charset(
        string_arg _charset
    ) D_NOEXCEPT
    {
        return d_mariadb_set_charset(m_db, _charset.c_str());
    }

    int
    get_charset(
        text& _charset
    ) const D_NOEXCEPT
    {
        return d_mariadb_get_charset(m_db, _charset.out());
    }

    int
    get_charset(
        std::string& _charset
    ) const
    {
        text      text_value;
        const int result = d_mariadb_get_charset(m_db, text_value.out());

        _charset = text_value.str();

        return result;
    }

#if D_INTERNAL_DB_FACE_OPTIONAL
    int
    get_charset(
        std::optional<std::string>& _charset
    ) const
    {
        text      text_value;
        const int result = d_mariadb_get_charset(m_db, text_value.out());

        // the text; nothing for SQL NULL, and for a failure
        if (text_value.is_null())
        {
            _charset.reset();
        }
        else
        {
            _charset = text_value.str();
        }

        return result;
    }
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<text, int>
    get_charset() const D_NOEXCEPT
    {
        text      value{};
        const int result = d_mariadb_get_charset(m_db, value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    set_time_zone(
        string_arg _zone
    ) D_NOEXCEPT
    {
        return d_mariadb_set_time_zone(m_db, _zone.c_str());
    }

    int
    get_time_zone(
        text& _zone
    ) const D_NOEXCEPT
    {
        return d_mariadb_get_time_zone(m_db, _zone.out());
    }

    int
    get_time_zone(
        std::string& _zone
    ) const
    {
        text      text_value;
        const int result = d_mariadb_get_time_zone(m_db, text_value.out());

        _zone = text_value.str();

        return result;
    }

#if D_INTERNAL_DB_FACE_OPTIONAL
    int
    get_time_zone(
        std::optional<std::string>& _zone
    ) const
    {
        text      text_value;
        const int result = d_mariadb_get_time_zone(m_db, text_value.out());

        // the text; nothing for SQL NULL, and for a failure
        if (text_value.is_null())
        {
            _zone.reset();
        }
        else
        {
            _zone = text_value.str();
        }

        return result;
    }
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<text, int>
    get_time_zone() const D_NOEXCEPT
    {
        text      value{};
        const int result = d_mariadb_get_time_zone(m_db, value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    set_sql_mode(
        string_arg _mode
    ) D_NOEXCEPT
    {
        return d_mariadb_set_sql_mode(m_db, _mode.c_str());
    }

    int
    get_sql_mode(
        text& _mode
    ) const D_NOEXCEPT
    {
        return d_mariadb_get_sql_mode(m_db, _mode.out());
    }

    int
    get_sql_mode(
        std::string& _mode
    ) const
    {
        text      text_value;
        const int result = d_mariadb_get_sql_mode(m_db, text_value.out());

        _mode = text_value.str();

        return result;
    }

#if D_INTERNAL_DB_FACE_OPTIONAL
    int
    get_sql_mode(
        std::optional<std::string>& _mode
    ) const
    {
        text      text_value;
        const int result = d_mariadb_get_sql_mode(m_db, text_value.out());

        // the text; nothing for SQL NULL, and for a failure
        if (text_value.is_null())
        {
            _mode.reset();
        }
        else
        {
            _mode = text_value.str();
        }

        return result;
    }
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<text, int>
    get_sql_mode() const D_NOEXCEPT
    {
        text      value{};
        const int result = d_mariadb_get_sql_mode(m_db, value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    set_variable(
        string_arg _name,
        string_arg _value
    ) D_NOEXCEPT
    {
        return d_mariadb_set_variable(m_db, _name.c_str(), _value.c_str());
    }

    int
    get_variable(
        string_arg _name,
        text&      _value
    ) const D_NOEXCEPT
    {
        return d_mariadb_get_variable(m_db, _name.c_str(), _value.out());
    }

    int
    get_variable(
        string_arg   _name,
        std::string& _value
    ) const
    {
        text      text_value;
        const int result = d_mariadb_get_variable(m_db,
                                                  _name.c_str(),
                                                  text_value.out());

        _value = text_value.str();

        return result;
    }

#if D_INTERNAL_DB_FACE_OPTIONAL
    int
    get_variable(
        string_arg                  _name,
        std::optional<std::string>& _value
    ) const
    {
        text      text_value;
        const int result = d_mariadb_get_variable(m_db,
                                                  _name.c_str(),
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
    get_variable(
        string_arg _name
    ) const D_NOEXCEPT
    {
        text      value{};
        const int result = d_mariadb_get_variable(m_db,
                                                  _name.c_str(),
                                                  value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    session_track(
        d_mariadb_track _kind,
        name_list&      _entries
    ) D_NOEXCEPT
    {
        return d_mariadb_session_track(m_db, _kind, _entries.out());
    }

    int
    session_track(
        d_mariadb_track           _kind,
        std::vector<std::string>& _entries
    )
    {
        name_list names;
        const int result = d_mariadb_session_track(m_db, _kind, names.out());

        names.to_vector(_entries);

        return result;
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<name_list, int>
    session_track(
        d_mariadb_track _kind
    ) D_NOEXCEPT
    {
        name_list value{};
        const int result = d_mariadb_session_track(m_db, _kind, value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    // 1.2.1.11
    // Maintenance
    //   mariadb_maintenance.h.
    int
    analyze_table(
        string_arg _schema,
        string_arg _table
    ) D_NOEXCEPT
    {
        return d_mariadb_analyze_table(m_db, _schema.c_str(), _table.c_str());
    }

    int
    optimize_table(
        string_arg _schema,
        string_arg _table
    ) D_NOEXCEPT
    {
        return d_mariadb_optimize_table(m_db, _schema.c_str(), _table.c_str());
    }

    int
    check_table(
        string_arg _schema,
        string_arg _table,
        bool&      _ok
    ) const D_NOEXCEPT
    {
        return d_mariadb_check_table(m_db,
                                     _schema.c_str(),
                                     _table.c_str(),
                                     &_ok);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<bool, int>
    check_table(
        string_arg _schema,
        string_arg _table
    ) const D_NOEXCEPT
    {
        bool      value{};
        const int result = d_mariadb_check_table(m_db,
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
    checksum_table(
        string_arg        _schema,
        string_arg        _table,
        re_std::uint64_t& _checksum
    ) const D_NOEXCEPT
    {
        return d_mariadb_checksum_table(m_db,
                                        _schema.c_str(),
                                        _table.c_str(),
                                        &_checksum);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<re_std::uint64_t, int>
    checksum_table(
        string_arg _schema,
        string_arg _table
    ) const D_NOEXCEPT
    {
        re_std::uint64_t value{};
        const int        result = d_mariadb_checksum_table(m_db,
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

private:
    D_DELETED_FN_MSG(mariadb(const mariadb&),
                     "a face owns its connection; move it")
    D_DELETED_FN_MSG(mariadb& operator=(const mariadb&),
                     "a face owns its connection; move it")

    // m_take -- a connect's result; the handle it made replaces the face's
    // when it succeeded. Each connect calls the layer first, in a statement
    // of its own, since the order a call's arguments are evaluated in is the
    // compiler's.
    int
    m_take(
        int               _result,
        struct d_mariadb* _made
    ) D_NOEXCEPT
    {
        // a failed connect made nothing to keep
        if (_result == 0)
        {
            reset(_made);
        }

        return _result;
    }

    struct d_mariadb* m_db;
};

// 1.3    Swapping
//------------------------------------------------------------------------------
// swapping -- the two connections, or statements, trade places, as std::swap
// would have them, for code that swaps unqualified
inline void
swap(
    mariadb_statement& _left,
    mariadb_statement& _right
) D_NOEXCEPT
{
    _left.swap(_right);

    return;
}

inline void
swap(
    mariadb& _left,
    mariadb& _right
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
// the face is its handle, and a statement its own, and from C++11 each moves
// without throwing (db_face.hpp, 6)
D_STATIC_ASSERT(sizeof(mariadb_statement) ==
                    sizeof(struct d_mariadb_statement*),
                "face::mariadb_statement is its handle, and no more");
D_STATIC_ASSERT(sizeof(mariadb) == sizeof(struct d_mariadb*),
                "face::mariadb is its handle, and no more");
#if D_MOVE_ENABLED
D_STATIC_ASSERT(
    ( (std::is_nothrow_move_constructible<mariadb_statement>::value) &&
      (std::is_nothrow_move_assignable<mariadb_statement>::value) ),
    "face::mariadb_statement moves without throwing");
D_STATIC_ASSERT( ( (std::is_nothrow_move_constructible<mariadb>::value) &&
                   (std::is_nothrow_move_assignable<mariadb>::value) ),
                 "face::mariadb moves without throwing");
#endif  // D_MOVE_ENABLED


}  // namespace face
}  // namespace djinterp


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_DB_MARIADB_MARIADB_FACE_HPP
