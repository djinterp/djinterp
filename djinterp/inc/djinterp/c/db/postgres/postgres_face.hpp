/*******************************************************************************
* djinterp [c]                                                 postgres_face.hpp
*
* The C++ face of djinterp's PostgreSQL layer.
*   face::postgres owns a connection, libpq's PGconn (struct pg_conn), and
* closes it when it goes; every function of the layer (postgres.h) is a
* member of the same name, the connection argument dropped, each with its C
* function's contract:
*     face::postgres db;
*     face::text     why;
*     if (db.connect_string("host=db.example dbname=shop", why) == 0)
*     {
*         face::transaction<face::postgres> work(db);
*
*         db.create_table_sql(0, "notes", "id bigint PRIMARY KEY, body text",
*                             D_DB_IF_NOT_EXISTS);
*         work.commit();
*     }
* Results are the layer's: 0, the server's SQLSTATE packed, or a negative
* D_DB_*. Functions that need no connection -- results, libpq's version,
* quoting a name -- are static. db_face.hpp describes what the face adds,
* how strings, lists and text cross it, and what each language level adds
* beside the exact forms: from C++17 std::optional text, from C++20 formats
* checked at compile time and std::span columns, from C++23 std::expected
* results.
*   connect, connect_string and connect_with replace the connection only when
* they succeed; a failure leaves the face as it was, and puts libpq's reason
* in the text, where one is given. Their forms that do not block --
* connect_start and its kin -- replace it with the connection being made
* when they answer D_DB_WAIT, and connect_continue goes on with the
* connection held, which the face holds no more once it fails. A member
* called on a face with no connection passes NULL to the layer, which
* answers D_DB_INVALID_ARGUMENT, or false, 0 or NULL.
*   face::postgres_statement owns a prepared statement, the layer's struct
* d_postgres_statement, whose copy the server holds, and finalizes it when it
* goes; prepare and preparef fill one, and every function of
* postgres_statement.h that takes a statement is its member, the statement
* argument dropped:
*     face::postgres_statement insert;
*     if (db.prepare(insert, "INSERT INTO notes (body) VALUES ($1)") == 0)
*     {
*         re_std::int64_t changes = 0;
*
*         insert.bind_text(1, "first");
*         insert.execute_update(changes);
*     }
* face::rows (db_face.hpp) holds the rows a query or a statement returned.
* Finalize a statement before closing its connection.
*   The members that forward are generated, by tools/gen_db_face.py from the
* layer's prototypes and tools/db_face/postgres_face.hpp.in, and keep the C
* functions' contracts, in the headers each part names; those written by
* hand in the template -- the ones that connect, close or hand over the
* connection or the statement, and result_sqlstate -- carry their own. Edit
* the template, not this file.
*
*
* path:      /inc/djinterp/c/db/postgres/postgres_face.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.14
*                                                            revised: 2026.10.24
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FACE
    ----
    1.  The statement
         1.  postgres_statement
              1.  Ownership
              2.  Describing, binding and running
    2.  The connection
         1.  postgres
              1.  Ownership
              2.  Results and library
              3.  Servers and errors
              4.  Connections
              5.  Queries
              6.  Statements
              7.  Databases, schemas and extensions
              8.  Schema objects
              9.  Transactions
              10. Settings
              11. Maintenance
              12. Protocol
    3.  Swapping
2.  COST
    ----
    1.  Sizes and moves
*/

#ifndef DJINTERP_C_DB_POSTGRES_POSTGRES_FACE_HPP
#define DJINTERP_C_DB_POSTGRES_POSTGRES_FACE_HPP 1

// std
#include <cstddef>  // std::size_t
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "../db_face.hpp"  // string_arg, text, the lists, the levels
#include "./postgres.h"    // the layer
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
// postgres_statement
//   class: a prepared statement -- the layer's struct d_postgres_statement,
// whose copy the server holds -- owned: finalized when the object goes.
// Made empty, and filled by face::postgres's prepare and preparef; every
// function of postgres_statement.h that takes a statement is a member of
// the same name, whose contract it keeps. Finalize it before closing its
// connection. Not copyable; movable from C++11.
class postgres_statement
{
public:
    // 1.1.1.1
    // Ownership
    //   a statement made empty holds none; whatever a statement holds, it
    // finalizes when it goes.
    D_CONSTEXPR postgres_statement() D_NOEXCEPT
        : m_statement(D_NULLPTR)
    {}

    /**
     * @brief Makes a statement that owns one prepared elsewhere.
     *
     * @param[in] _owned  the statement, or `NULL` for none.
     * @post The object finalizes `_owned`; the caller no longer does.
     */
    explicit D_CONSTEXPR postgres_statement(
        struct d_postgres_statement* _owned
    ) D_NOEXCEPT
        : m_statement(_owned)
    {}

    ~postgres_statement()
    {
        // a statement, when there is one; an empty object calls nothing
        if (m_statement)
        {
            (void)d_postgres_finalize(m_statement);
        }
    }

#if D_MOVE_ENABLED
    // moving -- the statement passes to the object moved to, which
    // finalizes what it held; the object moved from holds none
    postgres_statement(
        postgres_statement&& _other
    ) D_NOEXCEPT
        : m_statement(_other.release())
    {}

    postgres_statement&
    operator=(
        postgres_statement&& _other
    ) D_NOEXCEPT
    {
        reset(_other.release());

        return *this;
    }
#endif  // D_MOVE_ENABLED

    // the handle -- the statement, for the layer's own calls, NULL when
    // there is none; and whether there is one
    D_NODISCARD struct d_postgres_statement*
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
     *         d_postgres_finalize; `NULL` when there is none.
     * @post The object holds no statement.
     */
    D_NODISCARD struct d_postgres_statement*
    release() D_NOEXCEPT
    {
        struct d_postgres_statement* const held = m_statement;

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
        struct d_postgres_statement* _owned = D_NULLPTR
    ) D_NOEXCEPT
    {
        // what is held is finalized, unless it is what comes
        if (_owned != m_statement)
        {
            // a statement, when there is one
            if (m_statement)
            {
                (void)d_postgres_finalize(m_statement);
            }

            m_statement = _owned;
        }

        return;
    }

    /**
     * @brief Finalizes the statement, as d_postgres_finalize does.
     *
     * @return d_postgres_finalize's result: 0, or the SQLSTATE, packed,
     *         of a failure to close the server's copy.
     * @post The object holds no statement, whatever the result.
     */
    int
    finalize() D_NOEXCEPT
    {
        return d_postgres_finalize(release());
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
    struct d_postgres_statement**
    out() D_NOEXCEPT
    {
        reset();

        return &m_statement;
    }

    // swap -- the two objects trade statements
    void
    swap(
        postgres_statement& _other
    ) D_NOEXCEPT
    {
        struct d_postgres_statement* const held = m_statement;

        m_statement        = _other.m_statement;
        _other.m_statement = held;

        return;
    }

    // 1.1.1.2
    // Describing, binding and running
    //   postgres_statement.h's functions that take a statement.
    D_NODISCARD std::size_t
    parameter_count() const D_NOEXCEPT
    {
        return d_postgres_parameter_count(m_statement);
    }

    D_NODISCARD pg_conn*
    statement_connection() const D_NOEXCEPT
    {
        return d_postgres_statement_connection(m_statement);
    }

    int
    prepare_continue(
        unsigned      _events,
        d_db_pending& _pending
    ) D_NOEXCEPT
    {
        return d_postgres_prepare_continue(m_statement, _events, &_pending);
    }

    int
    bind_null(
        std::size_t _index
    ) D_NOEXCEPT
    {
        return d_postgres_bind_null(m_statement, _index);
    }

    int
    bind_bool(
        std::size_t _index,
        bool        _value
    ) D_NOEXCEPT
    {
        return d_postgres_bind_bool(m_statement, _index, _value);
    }

    int
    bind_int64(
        std::size_t     _index,
        re_std::int64_t _value
    ) D_NOEXCEPT
    {
        return d_postgres_bind_int64(m_statement, _index, _value);
    }

    int
    bind_double(
        std::size_t _index,
        double      _value
    ) D_NOEXCEPT
    {
        return d_postgres_bind_double(m_statement, _index, _value);
    }

    int
    bind_text(
        std::size_t _index,
        string_arg  _text
    ) D_NOEXCEPT
    {
        return d_postgres_bind_text(m_statement, _index, _text.c_str());
    }

    int
    bind_blob(
        std::size_t _index,
        const void* _bytes,
        std::size_t _size
    ) D_NOEXCEPT
    {
        return d_postgres_bind_blob(m_statement, _index, _bytes, _size);
    }

    int
    clear_bindings() D_NOEXCEPT
    {
        return d_postgres_clear_bindings(m_statement);
    }

    int
    execute_update(
        re_std::int64_t& _changes
    ) D_NOEXCEPT
    {
        return d_postgres_execute_update(m_statement, &_changes);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<re_std::int64_t, int>
    execute_update() D_NOEXCEPT
    {
        re_std::int64_t value{};
        const int       result = d_postgres_execute_update(m_statement, &value);

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
        return d_postgres_execute_query(m_statement, _rows.out());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<rows, int>
    execute_query() D_NOEXCEPT
    {
        rows      value{};
        const int result = d_postgres_execute_query(m_statement, value.out());

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
        return d_postgres_open_cursor(m_statement);
    }

    int
    fetch_rows(
        std::size_t _max,
        rows&       _rows
    ) D_NOEXCEPT
    {
        return d_postgres_fetch_rows(m_statement, _max, _rows.out());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<rows, int>
    fetch_rows(
        std::size_t _max
    ) D_NOEXCEPT
    {
        rows      value{};
        const int result = d_postgres_fetch_rows(m_statement,
                                                 _max,
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
    close_cursor() D_NOEXCEPT
    {
        return d_postgres_close_cursor(m_statement);
    }

    int
    execute_start(
        d_db_pending& _pending
    ) D_NOEXCEPT
    {
        return d_postgres_execute_start(m_statement, &_pending);
    }

    int
    execute_update_continue(
        unsigned         _events,
        d_db_pending&    _pending,
        re_std::int64_t& _changes
    ) D_NOEXCEPT
    {
        return d_postgres_execute_update_continue(m_statement,
                                                  _events,
                                                  &_pending,
                                                  &_changes);
    }

    int
    execute_query_continue(
        unsigned      _events,
        d_db_pending& _pending,
        rows&         _rows
    ) D_NOEXCEPT
    {
        return d_postgres_execute_query_continue(m_statement,
                                                 _events,
                                                 &_pending,
                                                 _rows.out());
    }

    int
    open_cursor_start(
        d_db_pending& _pending
    ) D_NOEXCEPT
    {
        return d_postgres_open_cursor_start(m_statement, &_pending);
    }

    int
    open_cursor_continue(
        unsigned      _events,
        d_db_pending& _pending
    ) D_NOEXCEPT
    {
        return d_postgres_open_cursor_continue(m_statement, _events, &_pending);
    }

    int
    fetch_rows_start(
        std::size_t   _max,
        d_db_pending& _pending,
        rows&         _rows
    ) D_NOEXCEPT
    {
        return d_postgres_fetch_rows_start(m_statement,
                                           _max,
                                           &_pending,
                                           _rows.out());
    }

    int
    fetch_rows_continue(
        unsigned      _events,
        d_db_pending& _pending,
        rows&         _rows
    ) D_NOEXCEPT
    {
        return d_postgres_fetch_rows_continue(m_statement,
                                              _events,
                                              &_pending,
                                              _rows.out());
    }

private:
    D_DELETED_FN_MSG(postgres_statement(const postgres_statement&),
                     "a statement owns what the server prepared; move it")
    D_DELETED_FN_MSG(postgres_statement& operator=(const postgres_statement&),
                     "a statement owns what the server prepared; move it")

    struct d_postgres_statement* m_statement;
};

// 1.2    The connection
//------------------------------------------------------------------------------
// 1.2.1
// postgres
//   class: a PostgreSQL connection, owned: closed when the object goes.
// Every member forwards to the layer's function of the same name, whose
// contract it keeps. Not copyable; movable from C++11.
class postgres
{
public:
    // transaction_mode
    //   type: how a transaction begins, for face::transaction.
    typedef enum d_postgres_transaction_mode transaction_mode;

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
    D_CONSTEXPR postgres() D_NOEXCEPT
        : m_db(D_NULLPTR)
    {}

    /**
     * @brief Makes a face that owns a connection made elsewhere, by libpq's
     *        own calls, say.
     *
     * @param[in] _owned  the connection, or `NULL` for none.
     * @post The face closes `_owned`; the caller no longer does.
     */
    explicit D_CONSTEXPR postgres(
        struct pg_conn* _owned
    ) D_NOEXCEPT
        : m_db(_owned)
    {}

    ~postgres()
    {
        // a connection, when there is one; an empty face calls nothing
        if (m_db)
        {
            (void)d_postgres_close(m_db);
        }
    }

#if D_MOVE_ENABLED
    // moving -- the connection passes to the face moved to, which closes
    // what it held; the face moved from holds none
    postgres(
        postgres&& _other
    ) D_NOEXCEPT
        : m_db(_other.release())
    {}

    postgres&
    operator=(
        postgres&& _other
    ) D_NOEXCEPT
    {
        reset(_other.release());

        return *this;
    }
#endif  // D_MOVE_ENABLED

    // the handle -- the connection, for the layer's or libpq's own calls,
    // NULL when there is none; and whether there is one
    D_NODISCARD struct pg_conn*
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
     *         d_postgres_close; `NULL` when there is none.
     * @post The face holds no connection.
     */
    D_NODISCARD struct pg_conn*
    release() D_NOEXCEPT
    {
        struct pg_conn* const held = m_db;

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
        struct pg_conn* _owned = D_NULLPTR
    ) D_NOEXCEPT
    {
        // what is held closes, unless it is what comes
        if (_owned != m_db)
        {
            // a connection, when there is one
            if (m_db)
            {
                (void)d_postgres_close(m_db);
            }

            m_db = _owned;
        }

        return;
    }

    // swap -- the two faces trade connections
    void
    swap(
        postgres& _other
    ) D_NOEXCEPT
    {
        struct pg_conn* const held = m_db;

        m_db        = _other.m_db;
        _other.m_db = held;

        return;
    }

    // default_transaction_mode -- how face::transaction begins when told
    // nothing: as BEGIN does, in the session's isolation level and access
    // mode
    D_NODISCARD static D_CONSTEXPR transaction_mode
    default_transaction_mode() D_NOEXCEPT
    {
        return D_POSTGRES_TRANSACTION_DEFAULT;
    }

    // 1.2.1.2
    // Results and library
    //   postgres_common.h's results and libpq's version: static, since
    // they need no connection.
    D_NODISCARD static int
    result_from_sqlstate(
        string_arg _sqlstate
    ) D_NOEXCEPT
    {
        return d_postgres_result_from_sqlstate(_sqlstate.c_str());
    }

    D_NODISCARD static const char*
    result_message(
        int _result
    ) D_NOEXCEPT
    {
        return d_postgres_result_message(_result);
    }

    D_NODISCARD static int
    client_version_number() D_NOEXCEPT
    {
        return d_postgres_client_version_number();
    }

    D_NODISCARD static bool
    client_supports(
        d_postgres_client_feature _feature
    ) D_NOEXCEPT
    {
        return d_postgres_client_supports(_feature);
    }

    /**
     * @brief Spells a result's SQLSTATE into a buffer, as
     *        d_postgres_result_sqlstate does.
     *
     * @param[in]  _result    a result the layer returned.
     * @param[out] _sqlstate  receives the five characters and a terminator:
     *                        "00000" for 0, and "" for a negative result or
     *                        one no SQLSTATE packs to.
     * @return `true` when the result packs a SQLSTATE, 0 included.
     */
    static bool
    result_sqlstate(
        int  _result,
        char (&_sqlstate)[6]
    ) D_NOEXCEPT
    {
        return d_postgres_result_sqlstate(_result, _sqlstate);
    }

    /**
     * @brief Spells a result's SQLSTATE, as d_postgres_result_sqlstate does.
     *
     * @param[in] _result  a result the layer returned.
     * @return the five characters: "00000" for 0, and "" for a negative
     *         result or one no SQLSTATE packs to.
     */
    D_NODISCARD static std::string
    result_sqlstate(
        int _result
    )
    {
        char spelled[6] = "";

        return (d_postgres_result_sqlstate(_result, spelled))
                   ? std::string(spelled)
                   : std::string();
    }

    // 1.2.1.3
    // Servers and errors
    //   postgres_common.h's server versions and features, and the
    // connection's latest error.
    D_NODISCARD const char*
    server_version() const D_NOEXCEPT
    {
        return d_postgres_server_version(m_db);
    }

    D_NODISCARD int
    server_version_number() const D_NOEXCEPT
    {
        return d_postgres_server_version_number(m_db);
    }

    D_NODISCARD bool
    server_at_least(
        unsigned _major,
        unsigned _minor
    ) const D_NOEXCEPT
    {
        return d_postgres_server_at_least(m_db, _major, _minor);
    }

    D_NODISCARD bool
    supports(
        d_postgres_feature _feature
    ) const D_NOEXCEPT
    {
        return d_postgres_supports(m_db, _feature);
    }

    D_NODISCARD const char*
    parameter_status(
        string_arg _name
    ) const D_NOEXCEPT
    {
        return d_postgres_parameter_status(m_db, _name.c_str());
    }

    D_NODISCARD const char*
    error_message() const D_NOEXCEPT
    {
        return d_postgres_error_message(m_db);
    }

    bool
    result_is_own(
        int _result
    ) D_NOEXCEPT
    {
        return d_postgres_result_is_own(m_db, _result);
    }

    // 1.2.1.4
    // Connections
    //   postgres_connection.h.

    /**
     * @brief Connects to a server, as d_postgres_connect does, in place of
     *        the connection held.
     *
     * @param[in] _options  where and as whom; `NULL` leaves every field to
     *                      libpq.
     * @return d_postgres_connect's result.
     * @post On success the face holds the new connection, and the one it
     *       held is closed; on failure the face is as it was.
     */
    int
    connect(
        const struct d_postgres_options* _options
    ) D_NOEXCEPT
    {
        struct pg_conn* made   = D_NULLPTR;
        const int       result = d_postgres_connect(_options,
                                                    &made,
                                                    D_NULLPTR);

        return m_take(result, made);
    }

    /**
     * @brief Connects as connect does, and says why a connection could not
     *        be made.
     *
     * @param[in]  _options  where and as whom; `NULL` leaves every field to
     *                       libpq.
     * @param[out] _message  libpq's reason when no connection could be made;
     *                       null otherwise.
     * @return d_postgres_connect's result.
     * @post As for connect.
     */
    int
    connect(
        const struct d_postgres_options* _options,
        text&                            _message
    ) D_NOEXCEPT
    {
        struct pg_conn* made   = D_NULLPTR;
        const int       result = d_postgres_connect(_options,
                                                    &made,
                                                    _message.out());

        return m_take(result, made);
    }

    /**
     * @brief Connects as a connection string says, as
     *        d_postgres_connect_string does, in place of the connection held.
     *
     * @param[in] _conninfo  keywords and values, or a URI; `NULL` or "" leaves
     *                       everything to libpq.
     * @return d_postgres_connect_string's result.
     * @post As for connect.
     */
    int
    connect_string(
        string_arg _conninfo
    ) D_NOEXCEPT
    {
        struct pg_conn* made   = D_NULLPTR;
        const int       result = d_postgres_connect_string(_conninfo.c_str(),
                                                           &made,
                                                           D_NULLPTR);

        return m_take(result, made);
    }

    /**
     * @brief Connects as connect_string does, and says why a connection
     *        could not be made.
     *
     * @param[in]  _conninfo  keywords and values, or a URI; `NULL` or ""
     *                        leaves everything to libpq.
     * @param[out] _message   libpq's reason when no connection could be
     *                        made, or the string could not be read; null
     *                        otherwise.
     * @return d_postgres_connect_string's result.
     * @post As for connect.
     */
    int
    connect_string(
        string_arg _conninfo,
        text&      _message
    ) D_NOEXCEPT
    {
        struct pg_conn* made   = D_NULLPTR;
        const int       result = d_postgres_connect_string(_conninfo.c_str(),
                                                           &made,
                                                           _message.out());

        return m_take(result, made);
    }

    /**
     * @brief Connects with libpq's keywords and values and a connection
     *        string, whose parts override them, as d_postgres_connect_with
     *        does, in place of the connection held.
     *
     * @param[in] _conninfo  keywords and values, a URI, or a database's
     *                       name; `NULL` or "" for none.
     * @param[in] _keywords  libpq's connection keywords.
     * @param[in] _values    their values, in the same order; a `NULL` or
     *                       empty value is left out.
     * @param[in] _count     how many keywords there are.
     * @return d_postgres_connect_with's result.
     * @post As for connect.
     */
    int
    connect_with(
        string_arg         _conninfo,
        const char* const* _keywords,
        const char* const* _values,
        std::size_t        _count
    ) D_NOEXCEPT
    {
        struct pg_conn* made   = D_NULLPTR;
        const int       result = d_postgres_connect_with(_conninfo.c_str(),
                                                         _keywords,
                                                         _values,
                                                         _count,
                                                         &made,
                                                         D_NULLPTR);

        return m_take(result, made);
    }

    /**
     * @brief Connects as connect_with does, and says why a connection could
     *        not be made.
     *
     * @param[in]  _conninfo  keywords and values, a URI, or a database's
     *                        name; `NULL` or "" for none.
     * @param[in]  _keywords  libpq's connection keywords.
     * @param[in]  _values    their values, in the same order; a `NULL` or
     *                        empty value is left out.
     * @param[in]  _count     how many keywords there are.
     * @param[out] _message   libpq's reason when no connection could be
     *                        made, the string could not be read, or a
     *                        keyword is not libpq's; null otherwise.
     * @return d_postgres_connect_with's result.
     * @post As for connect.
     */
    int
    connect_with(
        string_arg         _conninfo,
        const char* const* _keywords,
        const char* const* _values,
        std::size_t        _count,
        text&              _message
    ) D_NOEXCEPT
    {
        struct pg_conn* made   = D_NULLPTR;
        const int       result = d_postgres_connect_with(_conninfo.c_str(),
                                                         _keywords,
                                                         _values,
                                                         _count,
                                                         &made,
                                                         _message.out());

        return m_take(result, made);
    }

    // connect_with -- the keywords and values as arrays of one size, the
    // count; from C++20 as std::spans, which must be the same size, or the
    // layer answers D_DB_INVALID_ARGUMENT; each with the reason too
    template<std::size_t Count>
    int
    connect_with(
        string_arg        _conninfo,
        const char* const (&_keywords)[Count],
        const char* const (&_values)[Count]
    ) D_NOEXCEPT
    {
        return connect_with(_conninfo, _keywords, _values, Count);
    }

    template<std::size_t Count>
    int
    connect_with(
        string_arg        _conninfo,
        const char* const (&_keywords)[Count],
        const char* const (&_values)[Count],
        text&             _message
    ) D_NOEXCEPT
    {
        return connect_with(_conninfo, _keywords, _values, Count, _message);
    }

#if D_INTERNAL_DB_FACE_SPAN
    int
    connect_with(
        string_arg                   _conninfo,
        std::span<const char* const> _keywords,
        std::span<const char* const> _values
    ) noexcept
    {
        // keywords without values, or values without keywords
        if (_keywords.size() != _values.size())
        {
            return D_DB_INVALID_ARGUMENT;
        }

        return connect_with(_conninfo,
                            _keywords.data(),
                            _values.data(),
                            _keywords.size());
    }

    int
    connect_with(
        string_arg                   _conninfo,
        std::span<const char* const> _keywords,
        std::span<const char* const> _values,
        text&                        _message
    ) noexcept
    {
        // keywords without values, or values without keywords
        if (_keywords.size() != _values.size())
        {
            _message.reset();

            return D_DB_INVALID_ARGUMENT;
        }

        return connect_with(_conninfo,
                            _keywords.data(),
                            _values.data(),
                            _keywords.size(),
                            _message);
    }
#endif  // D_INTERNAL_DB_FACE_SPAN

    /**
     * @brief Closes the connection, as d_postgres_close does.
     *
     * @return 0.
     * @post The face holds no connection.
     */
    int
    close() D_NOEXCEPT
    {
        return d_postgres_close(release());
    }

    /**
     * @brief Starts a connection, as d_postgres_connect_start does, to
     *        replace the connection held.
     *
     * @param[in]  _options  where and as whom; `NULL` leaves every field to
     *                       libpq.
     * @param[out] _pending  receives what to wait for before continuing.
     * @return d_postgres_connect_start's result.
     * @post On D_DB_WAIT the face holds the connection being made, and the
     *       one it held is closed; on failure the face is as it was.
     */
    int
    connect_start(
        const struct d_postgres_options* _options,
        d_db_pending&                    _pending
    ) D_NOEXCEPT
    {
        struct pg_conn* made   = D_NULLPTR;
        const int       result = d_postgres_connect_start(_options,
                                                          &made,
                                                          &_pending,
                                                          D_NULLPTR);

        return m_take_started(result, made);
    }

    /**
     * @brief Starts a connection as connect_start does, and says why one
     *        could not be.
     *
     * @param[in]  _options  where and as whom; `NULL` leaves every field to
     *                       libpq.
     * @param[out] _pending  receives what to wait for before continuing.
     * @param[out] _message  libpq's reason when no connection could be
     *                       started; null otherwise.
     * @return d_postgres_connect_start's result.
     * @post As for connect_start.
     */
    int
    connect_start(
        const struct d_postgres_options* _options,
        d_db_pending&                    _pending,
        text&                            _message
    ) D_NOEXCEPT
    {
        struct pg_conn* made   = D_NULLPTR;
        const int       result = d_postgres_connect_start(_options,
                                                          &made,
                                                          &_pending,
                                                          _message.out());

        return m_take_started(result, made);
    }

    /**
     * @brief Starts a connection as a connection string says, as
     *        d_postgres_connect_string_start does, to replace the
     *        connection held.
     *
     * @param[in]  _conninfo  keywords and values, or a URI; `NULL` or ""
     *                        leaves everything to libpq.
     * @param[out] _pending   receives what to wait for before continuing.
     * @return d_postgres_connect_string_start's result.
     * @post As for connect_start.
     */
    int
    connect_string_start(
        string_arg    _conninfo,
        d_db_pending& _pending
    ) D_NOEXCEPT
    {
        struct pg_conn* made   = D_NULLPTR;
        const int       result = d_postgres_connect_string_start(
                                     _conninfo.c_str(),
                                     &made,
                                     &_pending,
                                     D_NULLPTR);

        return m_take_started(result, made);
    }

    /**
     * @brief Starts a connection as connect_string_start does, and says why
     *        one could not be.
     *
     * @param[in]  _conninfo  keywords and values, or a URI; `NULL` or ""
     *                        leaves everything to libpq.
     * @param[out] _pending   receives what to wait for before continuing.
     * @param[out] _message   libpq's reason when no connection could be
     *                        started, or the string could not be read; null
     *                        otherwise.
     * @return d_postgres_connect_string_start's result.
     * @post As for connect_start.
     */
    int
    connect_string_start(
        string_arg    _conninfo,
        d_db_pending& _pending,
        text&         _message
    ) D_NOEXCEPT
    {
        struct pg_conn* made   = D_NULLPTR;
        const int       result = d_postgres_connect_string_start(
                                     _conninfo.c_str(),
                                     &made,
                                     &_pending,
                                     _message.out());

        return m_take_started(result, made);
    }

    /**
     * @brief Starts a connection with libpq's keywords and values and a
     *        connection string, as d_postgres_connect_with_start does, to
     *        replace the connection held.
     *
     * @param[in]  _conninfo  keywords and values, a URI, or a database's
     *                        name; `NULL` or "" for none.
     * @param[in]  _keywords  libpq's connection keywords.
     * @param[in]  _values    their values, in the same order; a `NULL` or
     *                        empty value is left out.
     * @param[in]  _count     how many keywords there are.
     * @param[out] _pending   receives what to wait for before continuing.
     * @return d_postgres_connect_with_start's result.
     * @post As for connect_start.
     */
    int
    connect_with_start(
        string_arg         _conninfo,
        const char* const* _keywords,
        const char* const* _values,
        std::size_t        _count,
        d_db_pending&      _pending
    ) D_NOEXCEPT
    {
        struct pg_conn* made   = D_NULLPTR;
        const int       result = d_postgres_connect_with_start(
                                     _conninfo.c_str(),
                                     _keywords,
                                     _values,
                                     _count,
                                     &made,
                                     &_pending,
                                     D_NULLPTR);

        return m_take_started(result, made);
    }

    /**
     * @brief Starts a connection as connect_with_start does, and says why
     *        one could not be.
     *
     * @param[in]  _conninfo  keywords and values, a URI, or a database's
     *                        name; `NULL` or "" for none.
     * @param[in]  _keywords  libpq's connection keywords.
     * @param[in]  _values    their values, in the same order; a `NULL` or
     *                        empty value is left out.
     * @param[in]  _count     how many keywords there are.
     * @param[out] _pending   receives what to wait for before continuing.
     * @param[out] _message   libpq's reason when no connection could be
     *                        started, the string could not be read, or a
     *                        keyword is not libpq's; null otherwise.
     * @return d_postgres_connect_with_start's result.
     * @post As for connect_start.
     */
    int
    connect_with_start(
        string_arg         _conninfo,
        const char* const* _keywords,
        const char* const* _values,
        std::size_t        _count,
        d_db_pending&      _pending,
        text&              _message
    ) D_NOEXCEPT
    {
        struct pg_conn* made   = D_NULLPTR;
        const int       result = d_postgres_connect_with_start(
                                     _conninfo.c_str(),
                                     _keywords,
                                     _values,
                                     _count,
                                     &made,
                                     &_pending,
                                     _message.out());

        return m_take_started(result, made);
    }

    /**
     * @brief Continues the connection a start began, as
     *        d_postgres_connect_continue does.
     *
     * @param[in]     _events   what came, as d_db_wait.
     * @param[in,out] _pending  what the start or the last continuation
     *                          wrote; receives what to wait for next.
     * @return d_postgres_connect_continue's result.
     * @post On a failure the face holds no connection: the layer finished
     *       the one being made.
     */
    int
    connect_continue(
        unsigned      _events,
        d_db_pending& _pending
    ) D_NOEXCEPT
    {
        struct pg_conn* db     = m_db;
        const int       result = d_postgres_connect_continue(&db,
                                                             _events,
                                                             &_pending,
                                                             D_NULLPTR);

        m_db = db;

        return result;
    }

    /**
     * @brief Continues the connection as connect_continue does, and says
     *        why it could not be made.
     *
     * @param[in]     _events   what came, as d_db_wait.
     * @param[in,out] _pending  what the start or the last continuation
     *                          wrote; receives what to wait for next.
     * @param[out]    _message  libpq's reason when the connection could not
     *                          be made, "timeout expired" for its deadline;
     *                          null otherwise.
     * @return d_postgres_connect_continue's result.
     * @post As for connect_continue.
     */
    int
    connect_continue(
        unsigned      _events,
        d_db_pending& _pending,
        text&         _message
    ) D_NOEXCEPT
    {
        struct pg_conn* db     = m_db;
        const int       result = d_postgres_connect_continue(&db,
                                                             _events,
                                                             &_pending,
                                                             _message.out());

        m_db = db;

        return result;
    }

    // cancel_finish -- abandons a cancel request under way, as
    // d_postgres_cancel_finish does; NULL is accepted. Returns 0
    static int
    cancel_finish(
        struct pg_cancel_conn* _cancel
    ) D_NOEXCEPT
    {
        return d_postgres_cancel_finish(_cancel);
    }

    int
    ping() D_NOEXCEPT
    {
        return d_postgres_ping(m_db);
    }

    D_NODISCARD int
    backend_pid() const D_NOEXCEPT
    {
        return d_postgres_backend_pid(m_db);
    }

    int
    reconnect() D_NOEXCEPT
    {
        return d_postgres_reconnect(m_db);
    }

    int
    reconnect_start(
        d_db_pending& _pending
    ) D_NOEXCEPT
    {
        return d_postgres_reconnect_start(m_db, &_pending);
    }

    int
    reconnect_continue(
        unsigned      _events,
        d_db_pending& _pending
    ) D_NOEXCEPT
    {
        return d_postgres_reconnect_continue(m_db, _events, &_pending);
    }

    int
    cancel(
        text& _message
    ) D_NOEXCEPT
    {
        return d_postgres_cancel(m_db, _message.out());
    }

    int
    cancel(
        std::string& _message
    )
    {
        text      text_value;
        const int result = d_postgres_cancel(m_db, text_value.out());

        _message = text_value.str();

        return result;
    }

#if D_INTERNAL_DB_FACE_OPTIONAL
    int
    cancel(
        std::optional<std::string>& _message
    )
    {
        text      text_value;
        const int result = d_postgres_cancel(m_db, text_value.out());

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
    cancel_start(
        pg_cancel_conn*& _cancel,
        d_db_pending&    _pending,
        text&            _message
    ) D_NOEXCEPT
    {
        return d_postgres_cancel_start(m_db,
                                       &_cancel,
                                       &_pending,
                                       _message.out());
    }

    int
    cancel_start(
        pg_cancel_conn*& _cancel,
        d_db_pending&    _pending,
        std::string&     _message
    )
    {
        text      text_value;
        const int result = d_postgres_cancel_start(m_db,
                                                   &_cancel,
                                                   &_pending,
                                                   text_value.out());

        _message = text_value.str();

        return result;
    }

#if D_INTERNAL_DB_FACE_OPTIONAL
    int
    cancel_start(
        pg_cancel_conn*&            _cancel,
        d_db_pending&               _pending,
        std::optional<std::string>& _message
    )
    {
        text      text_value;
        const int result = d_postgres_cancel_start(m_db,
                                                   &_cancel,
                                                   &_pending,
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

    static int
    cancel_continue(
        pg_cancel_conn*& _cancel,
        unsigned         _events,
        d_db_pending&    _pending,
        text&            _message
    ) D_NOEXCEPT
    {
        return d_postgres_cancel_continue(&_cancel,
                                          _events,
                                          &_pending,
                                          _message.out());
    }

    int
    cancel_continue(
        pg_cancel_conn*& _cancel,
        unsigned         _events,
        d_db_pending&    _pending,
        std::string&     _message
    )
    {
        text      text_value;
        const int result = d_postgres_cancel_continue(&_cancel,
                                                      _events,
                                                      &_pending,
                                                      text_value.out());

        _message = text_value.str();

        return result;
    }

#if D_INTERNAL_DB_FACE_OPTIONAL
    int
    cancel_continue(
        pg_cancel_conn*&            _cancel,
        unsigned                    _events,
        d_db_pending&               _pending,
        std::optional<std::string>& _message
    )
    {
        text      text_value;
        const int result = d_postgres_cancel_continue(&_cancel,
                                                      _events,
                                                      &_pending,
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

    // 1.2.1.5
    // Queries
    //   postgres_query.h; the f forms from C++11, their formats checked
    // from C++20 (db_face.hpp, 2.2 to 2.5).
    int
    exec(
        string_arg _sql
    ) D_NOEXCEPT
    {
        return d_postgres_exec(m_db, _sql.c_str());
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

        return d_postgres_execf(m_db, _format.c_str(), _args...);
    }
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    int
    exec_changes(
        re_std::int64_t& _changes,
        string_arg       _sql
    ) D_NOEXCEPT
    {
        return d_postgres_exec_changes(m_db, &_changes, _sql.c_str());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<re_std::int64_t, int>
    exec_changes(
        string_arg _sql
    ) D_NOEXCEPT
    {
        re_std::int64_t value{};
        const int       result = d_postgres_exec_changes(m_db,
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

        return d_postgres_execf_changes(m_db,
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
        const int       result = d_postgres_execf_changes(m_db,
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
        return d_postgres_query_int64(m_db, &_value, _sql.c_str());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<re_std::int64_t, int>
    query_int64(
        string_arg _sql
    ) D_NOEXCEPT
    {
        re_std::int64_t value{};
        const int       result = d_postgres_query_int64(m_db,
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

        return d_postgres_query_int64f(m_db,
                                       &_value,
                                       _format.c_str(),
                                       _args...);
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
        const int       result = d_postgres_query_int64f(m_db,
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
        return d_postgres_query_double(m_db, &_value, _sql.c_str());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<double, int>
    query_double(
        string_arg _sql
    ) D_NOEXCEPT
    {
        double    value{};
        const int result = d_postgres_query_double(m_db, &value, _sql.c_str());

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

        return d_postgres_query_doublef(m_db,
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
        const int result = d_postgres_query_doublef(m_db,
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
        return d_postgres_query_text(m_db, _value.out(), _sql.c_str());
    }

    int
    query_text(
        std::string& _value,
        string_arg   _sql
    )
    {
        text      text_value;
        const int result = d_postgres_query_text(m_db,
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
        const int result = d_postgres_query_text(m_db,
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
        const int result = d_postgres_query_text(m_db,
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

        return d_postgres_query_textf(m_db,
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
        const int result = d_postgres_query_textf(m_db,
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
        const int result = d_postgres_query_textf(m_db,
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
        const int result = d_postgres_query_textf(m_db,
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
    query_bool(
        bool&      _value,
        string_arg _sql
    ) D_NOEXCEPT
    {
        return d_postgres_query_bool(m_db, &_value, _sql.c_str());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<bool, int>
    query_bool(
        string_arg _sql
    ) D_NOEXCEPT
    {
        bool      value{};
        const int result = d_postgres_query_bool(m_db, &value, _sql.c_str());

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
    query_boolf(
        bool&           _value,
        format<Args...> _format,
        Args...         _args
    ) D_NOEXCEPT
    {
        internal::check_varargs<Args...>();

        return d_postgres_query_boolf(m_db, &_value, _format.c_str(), _args...);
    }
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_INTERNAL_DB_FACE_EXPECTED
    template<typename... Args>
    D_NODISCARD std::expected<bool, int>
    query_boolf(
        format<Args...> _format,
        Args...         _args
    ) D_NOEXCEPT
    {
        internal::check_varargs<Args...>();

        bool      value{};
        const int result = d_postgres_query_boolf(m_db,
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
    query_rows(
        rows&      _rows,
        string_arg _sql
    ) D_NOEXCEPT
    {
        return d_postgres_query_rows(m_db, _rows.out(), _sql.c_str());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<rows, int>
    query_rows(
        string_arg _sql
    ) D_NOEXCEPT
    {
        rows      value{};
        const int result = d_postgres_query_rows(m_db,
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

        return d_postgres_query_rowsf(m_db,
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
        const int result = d_postgres_query_rowsf(m_db,
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
    query_rows_params(
        rows&              _rows,
        string_arg         _sql,
        const char* const* _values,
        std::size_t        _count
    ) D_NOEXCEPT
    {
        return d_postgres_query_rows_params(m_db,
                                            _rows.out(),
                                            _sql.c_str(),
                                            _values,
                                            _count);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<rows, int>
    query_rows_params(
        string_arg         _sql,
        const char* const* _values,
        std::size_t        _count
    ) D_NOEXCEPT
    {
        rows      value{};
        const int result = d_postgres_query_rows_params(m_db,
                                                        value.out(),
                                                        _sql.c_str(),
                                                        _values,
                                                        _count);

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
        _quoted.reset(d_postgres_quote_identifier(_name.c_str()));

        return;
    }

    D_NODISCARD static std::string
    quote_identifier(
        string_arg _name
    )
    {
        const text result(d_postgres_quote_identifier(_name.c_str()));

        return result.str();
    }

    void
    quote_literal(
        string_arg _text,
        text&      _quoted
    ) const D_NOEXCEPT
    {
        _quoted.reset(d_postgres_quote_literal(m_db, _text.c_str()));

        return;
    }

    D_NODISCARD std::string
    quote_literal(
        string_arg _text
    ) const
    {
        const text result(d_postgres_quote_literal(m_db, _text.c_str()));

        return result.str();
    }

    // 1.2.1.6
    // Statements
    //   postgres_statement.h's preparing, into a
    // face::postgres_statement, whose members its other functions are.
    int
    prepare(
        postgres_statement& _statement,
        string_arg          _sql
    ) D_NOEXCEPT
    {
        return d_postgres_prepare(m_db, _statement.out(), _sql.c_str());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<postgres_statement, int>
    prepare(
        string_arg _sql
    ) D_NOEXCEPT
    {
        postgres_statement value{};
        const int          result = d_postgres_prepare(m_db,
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
        postgres_statement& _statement,
        format<Args...>     _format,
        Args...             _args
    ) D_NOEXCEPT
    {
        internal::check_varargs<Args...>();

        return d_postgres_preparef(m_db,
                                   _statement.out(),
                                   _format.c_str(),
                                   _args...);
    }
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_INTERNAL_DB_FACE_EXPECTED
    template<typename... Args>
    D_NODISCARD std::expected<postgres_statement, int>
    preparef(
        format<Args...> _format,
        Args...         _args
    ) D_NOEXCEPT
    {
        internal::check_varargs<Args...>();

        postgres_statement value{};
        const int          result = d_postgres_preparef(m_db,
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
    close_prepared(
        string_arg _name
    ) D_NOEXCEPT
    {
        return d_postgres_close_prepared(m_db, _name.c_str());
    }

    int
    prepare_start(
        postgres_statement& _statement,
        string_arg          _sql,
        d_db_pending&       _pending
    ) D_NOEXCEPT
    {
        return d_postgres_prepare_start(m_db,
                                        _statement.out(),
                                        _sql.c_str(),
                                        &_pending);
    }

    // 1.2.1.7
    // Databases, schemas and extensions
    //   postgres_database.h.
    D_NODISCARD bool
    has_database(
        string_arg _name
    ) const D_NOEXCEPT
    {
        return d_postgres_has_database(m_db, _name.c_str());
    }

    int
    has_database_s(
        string_arg _name,
        bool&      _exists
    ) const D_NOEXCEPT
    {
        return d_postgres_has_database_s(m_db, _name.c_str(), &_exists);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<bool, int>
    has_database_s(
        string_arg _name
    ) const D_NOEXCEPT
    {
        bool      value{};
        const int result = d_postgres_has_database_s(m_db,
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
        return d_postgres_create_database(m_db, _name.c_str(), _flags);
    }

    int
    drop_database(
        string_arg _name,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        return d_postgres_drop_database(m_db, _name.c_str(), _flags);
    }

    int
    list_databases(
        name_list& _names
    ) const D_NOEXCEPT
    {
        return d_postgres_list_databases(m_db, _names.out());
    }

    int
    list_databases(
        std::vector<std::string>& _names
    ) const
    {
        name_list names;
        const int result = d_postgres_list_databases(m_db, names.out());

        names.to_vector(_names);

        return result;
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<name_list, int>
    list_databases() const D_NOEXCEPT
    {
        name_list value{};
        const int result = d_postgres_list_databases(m_db, value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    D_NODISCARD const char*
    current_database() const D_NOEXCEPT
    {
        return d_postgres_current_database(m_db);
    }

    D_NODISCARD bool
    has_schema(
        string_arg _name
    ) const D_NOEXCEPT
    {
        return d_postgres_has_schema(m_db, _name.c_str());
    }

    int
    has_schema_s(
        string_arg _name,
        bool&      _exists
    ) const D_NOEXCEPT
    {
        return d_postgres_has_schema_s(m_db, _name.c_str(), &_exists);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<bool, int>
    has_schema_s(
        string_arg _name
    ) const D_NOEXCEPT
    {
        bool      value{};
        const int result = d_postgres_has_schema_s(m_db, _name.c_str(), &value);

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    create_schema(
        string_arg _name,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        return d_postgres_create_schema(m_db, _name.c_str(), _flags);
    }

    int
    drop_schema(
        string_arg _name,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        return d_postgres_drop_schema(m_db, _name.c_str(), _flags);
    }

    int
    list_schemas(
        name_list& _names
    ) const D_NOEXCEPT
    {
        return d_postgres_list_schemas(m_db, _names.out());
    }

    int
    list_schemas(
        std::vector<std::string>& _names
    ) const
    {
        name_list names;
        const int result = d_postgres_list_schemas(m_db, names.out());

        names.to_vector(_names);

        return result;
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<name_list, int>
    list_schemas() const D_NOEXCEPT
    {
        name_list value{};
        const int result = d_postgres_list_schemas(m_db, value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    current_schema(
        text& _name
    ) const D_NOEXCEPT
    {
        return d_postgres_current_schema(m_db, _name.out());
    }

    int
    current_schema(
        std::string& _name
    ) const
    {
        text      text_value;
        const int result = d_postgres_current_schema(m_db, text_value.out());

        _name = text_value.str();

        return result;
    }

#if D_INTERNAL_DB_FACE_OPTIONAL
    int
    current_schema(
        std::optional<std::string>& _name
    ) const
    {
        text      text_value;
        const int result = d_postgres_current_schema(m_db, text_value.out());

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
    current_schema() const D_NOEXCEPT
    {
        text      value{};
        const int result = d_postgres_current_schema(m_db, value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    D_NODISCARD bool
    has_extension(
        string_arg _name
    ) const D_NOEXCEPT
    {
        return d_postgres_has_extension(m_db, _name.c_str());
    }

    int
    has_extension_s(
        string_arg _name,
        bool&      _exists
    ) const D_NOEXCEPT
    {
        return d_postgres_has_extension_s(m_db, _name.c_str(), &_exists);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<bool, int>
    has_extension_s(
        string_arg _name
    ) const D_NOEXCEPT
    {
        bool      value{};
        const int result = d_postgres_has_extension_s(m_db,
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
    create_extension(
        string_arg _name,
        string_arg _schema,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        return d_postgres_create_extension(m_db,
                                           _name.c_str(),
                                           _schema.c_str(),
                                           _flags);
    }

    int
    drop_extension(
        string_arg _name,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        return d_postgres_drop_extension(m_db, _name.c_str(), _flags);
    }

    int
    list_extensions(
        name_list& _names
    ) const D_NOEXCEPT
    {
        return d_postgres_list_extensions(m_db, _names.out());
    }

    int
    list_extensions(
        std::vector<std::string>& _names
    ) const
    {
        name_list names;
        const int result = d_postgres_list_extensions(m_db, names.out());

        names.to_vector(_names);

        return result;
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<name_list, int>
    list_extensions() const D_NOEXCEPT
    {
        name_list value{};
        const int result = d_postgres_list_extensions(m_db, value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    // 1.2.1.8
    // Schema objects
    //   postgres_schema.h.
    D_NODISCARD bool
    has_table(
        string_arg _schema,
        string_arg _table
    ) const D_NOEXCEPT
    {
        return d_postgres_has_table(m_db, _schema.c_str(), _table.c_str());
    }

    int
    has_table_s(
        string_arg _schema,
        string_arg _table,
        bool&      _exists
    ) const D_NOEXCEPT
    {
        return d_postgres_has_table_s(m_db,
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
        const int result = d_postgres_has_table_s(m_db,
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
        return d_postgres_create_table(m_db,
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
        return d_postgres_create_table(m_db,
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
        return d_postgres_create_table(m_db,
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
        return d_postgres_create_table_sql(m_db,
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
        return d_postgres_drop_table(m_db,
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
        return d_postgres_rename_table(m_db,
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
        return d_postgres_truncate_table(m_db, _schema.c_str(), _table.c_str());
    }

    int
    count_rows(
        string_arg       _schema,
        string_arg       _table,
        re_std::int64_t& _count
    ) const D_NOEXCEPT
    {
        return d_postgres_count_rows(m_db,
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
        const int       result = d_postgres_count_rows(m_db,
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
        return d_postgres_list_tables(m_db, _schema.c_str(), _names.out());
    }

    int
    list_tables(
        string_arg                _schema,
        std::vector<std::string>& _names
    ) const
    {
        name_list names;
        const int result = d_postgres_list_tables(m_db,
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
        const int result = d_postgres_list_tables(m_db,
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
        return d_postgres_has_column(m_db,
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
        return d_postgres_has_column_s(m_db,
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
        const int result = d_postgres_has_column_s(m_db,
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
        return d_postgres_add_column(m_db,
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
        return d_postgres_add_column_sql(m_db,
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
        return d_postgres_rename_column(m_db,
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
        return d_postgres_drop_column(m_db,
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
        return d_postgres_list_columns(m_db,
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
        const int   result = d_postgres_list_columns(m_db,
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
        return d_postgres_has_index(m_db, _schema.c_str(), _index.c_str());
    }

    int
    has_index_s(
        string_arg _schema,
        string_arg _index,
        bool&      _exists
    ) const D_NOEXCEPT
    {
        return d_postgres_has_index_s(m_db,
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
        const int result = d_postgres_has_index_s(m_db,
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
        return d_postgres_create_index(m_db,
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
        return d_postgres_drop_index(m_db,
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
        return d_postgres_list_indexes(m_db,
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
        const int result = d_postgres_list_indexes(m_db,
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
        const int result = d_postgres_list_indexes(m_db,
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
        return d_postgres_has_view(m_db, _schema.c_str(), _view.c_str());
    }

    int
    has_view_s(
        string_arg _schema,
        string_arg _view,
        bool&      _exists
    ) const D_NOEXCEPT
    {
        return d_postgres_has_view_s(m_db,
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
        const int result = d_postgres_has_view_s(m_db,
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
        return d_postgres_create_view(m_db,
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
        return d_postgres_drop_view(m_db,
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
        return d_postgres_list_views(m_db, _schema.c_str(), _names.out());
    }

    int
    list_views(
        string_arg                _schema,
        std::vector<std::string>& _names
    ) const
    {
        name_list names;
        const int result = d_postgres_list_views(m_db,
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
        const int result = d_postgres_list_views(m_db,
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
        string_arg _trigger,
        string_arg _table
    ) const D_NOEXCEPT
    {
        return d_postgres_has_trigger(m_db,
                                      _schema.c_str(),
                                      _trigger.c_str(),
                                      _table.c_str());
    }

    int
    has_trigger_s(
        string_arg _schema,
        string_arg _trigger,
        string_arg _table,
        bool&      _exists
    ) const D_NOEXCEPT
    {
        return d_postgres_has_trigger_s(m_db,
                                        _schema.c_str(),
                                        _trigger.c_str(),
                                        _table.c_str(),
                                        &_exists);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<bool, int>
    has_trigger_s(
        string_arg _schema,
        string_arg _trigger,
        string_arg _table
    ) const D_NOEXCEPT
    {
        bool      value{};
        const int result = d_postgres_has_trigger_s(m_db,
                                                    _schema.c_str(),
                                                    _trigger.c_str(),
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
    create_trigger(
        string_arg _trigger,
        string_arg _definition,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        return d_postgres_create_trigger(m_db,
                                         _trigger.c_str(),
                                         _definition.c_str(),
                                         _flags);
    }

    int
    drop_trigger(
        string_arg _schema,
        string_arg _trigger,
        string_arg _table,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        return d_postgres_drop_trigger(m_db,
                                       _schema.c_str(),
                                       _trigger.c_str(),
                                       _table.c_str(),
                                       _flags);
    }

    int
    list_triggers(
        string_arg _schema,
        string_arg _table,
        name_list& _names
    ) const D_NOEXCEPT
    {
        return d_postgres_list_triggers(m_db,
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
        const int result = d_postgres_list_triggers(m_db,
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
        const int result = d_postgres_list_triggers(m_db,
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
    has_sequence(
        string_arg _schema,
        string_arg _sequence
    ) const D_NOEXCEPT
    {
        return d_postgres_has_sequence(m_db,
                                       _schema.c_str(),
                                       _sequence.c_str());
    }

    int
    has_sequence_s(
        string_arg _schema,
        string_arg _sequence,
        bool&      _exists
    ) const D_NOEXCEPT
    {
        return d_postgres_has_sequence_s(m_db,
                                         _schema.c_str(),
                                         _sequence.c_str(),
                                         &_exists);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<bool, int>
    has_sequence_s(
        string_arg _schema,
        string_arg _sequence
    ) const D_NOEXCEPT
    {
        bool      value{};
        const int result = d_postgres_has_sequence_s(m_db,
                                                     _schema.c_str(),
                                                     _sequence.c_str(),
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
    create_sequence(
        string_arg _schema,
        string_arg _sequence,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        return d_postgres_create_sequence(m_db,
                                          _schema.c_str(),
                                          _sequence.c_str(),
                                          _flags);
    }

    int
    drop_sequence(
        string_arg _schema,
        string_arg _sequence,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        return d_postgres_drop_sequence(m_db,
                                        _schema.c_str(),
                                        _sequence.c_str(),
                                        _flags);
    }

    int
    list_sequences(
        string_arg _schema,
        name_list& _names
    ) const D_NOEXCEPT
    {
        return d_postgres_list_sequences(m_db, _schema.c_str(), _names.out());
    }

    int
    list_sequences(
        string_arg                _schema,
        std::vector<std::string>& _names
    ) const
    {
        name_list names;
        const int result = d_postgres_list_sequences(m_db,
                                                     _schema.c_str(),
                                                     names.out());

        names.to_vector(_names);

        return result;
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<name_list, int>
    list_sequences(
        string_arg _schema
    ) const D_NOEXCEPT
    {
        name_list value{};
        const int result = d_postgres_list_sequences(m_db,
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

    int
    next_value(
        string_arg       _schema,
        string_arg       _sequence,
        re_std::int64_t& _value
    ) D_NOEXCEPT
    {
        return d_postgres_next_value(m_db,
                                     _schema.c_str(),
                                     _sequence.c_str(),
                                     &_value);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<re_std::int64_t, int>
    next_value(
        string_arg _schema,
        string_arg _sequence
    ) D_NOEXCEPT
    {
        re_std::int64_t value{};
        const int       result = d_postgres_next_value(m_db,
                                                       _schema.c_str(),
                                                       _sequence.c_str(),
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
    last_value(
        re_std::int64_t& _value
    ) D_NOEXCEPT
    {
        return d_postgres_last_value(m_db, &_value);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<re_std::int64_t, int>
    last_value() D_NOEXCEPT
    {
        re_std::int64_t value{};
        const int       result = d_postgres_last_value(m_db, &value);

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
    //   postgres_transaction.h; face::transaction and face::savepoint guard
    // them.
    int
    begin_transaction(
        d_postgres_transaction_mode _mode
    ) D_NOEXCEPT
    {
        return d_postgres_begin_transaction(m_db, _mode);
    }

    int
    commit() D_NOEXCEPT
    {
        return d_postgres_commit(m_db);
    }

    int
    rollback() D_NOEXCEPT
    {
        return d_postgres_rollback(m_db);
    }

    D_NODISCARD bool
    in_transaction() const D_NOEXCEPT
    {
        return d_postgres_in_transaction(m_db);
    }

    D_NODISCARD bool
    in_failed_transaction() const D_NOEXCEPT
    {
        return d_postgres_in_failed_transaction(m_db);
    }

    D_NODISCARD d_postgres_transaction_state
    transaction_status() const D_NOEXCEPT
    {
        return d_postgres_transaction_status(m_db);
    }

    int
    create_savepoint(
        string_arg _name
    ) D_NOEXCEPT
    {
        return d_postgres_create_savepoint(m_db, _name.c_str());
    }

    int
    release_savepoint(
        string_arg _name
    ) D_NOEXCEPT
    {
        return d_postgres_release_savepoint(m_db, _name.c_str());
    }

    int
    rollback_to_savepoint(
        string_arg _name
    ) D_NOEXCEPT
    {
        return d_postgres_rollback_to_savepoint(m_db, _name.c_str());
    }

    // 1.2.1.10
    // Settings
    //   postgres_settings.h.
    int
    set_search_path(
        string_arg _path
    ) D_NOEXCEPT
    {
        return d_postgres_set_search_path(m_db, _path.c_str());
    }

    int
    get_search_path(
        text& _path
    ) const D_NOEXCEPT
    {
        return d_postgres_get_search_path(m_db, _path.out());
    }

    int
    get_search_path(
        std::string& _path
    ) const
    {
        text      text_value;
        const int result = d_postgres_get_search_path(m_db, text_value.out());

        _path = text_value.str();

        return result;
    }

#if D_INTERNAL_DB_FACE_OPTIONAL
    int
    get_search_path(
        std::optional<std::string>& _path
    ) const
    {
        text      text_value;
        const int result = d_postgres_get_search_path(m_db, text_value.out());

        // the text; nothing for SQL NULL, and for a failure
        if (text_value.is_null())
        {
            _path.reset();
        }
        else
        {
            _path = text_value.str();
        }

        return result;
    }
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<text, int>
    get_search_path() const D_NOEXCEPT
    {
        text      value{};
        const int result = d_postgres_get_search_path(m_db, value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    set_statement_timeout(
        unsigned _milliseconds
    ) D_NOEXCEPT
    {
        return d_postgres_set_statement_timeout(m_db, _milliseconds);
    }

    int
    get_statement_timeout(
        unsigned& _milliseconds
    ) const D_NOEXCEPT
    {
        return d_postgres_get_statement_timeout(m_db, &_milliseconds);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<unsigned, int>
    get_statement_timeout() const D_NOEXCEPT
    {
        unsigned  value{};
        const int result = d_postgres_get_statement_timeout(m_db, &value);

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    set_lock_timeout(
        unsigned _milliseconds
    ) D_NOEXCEPT
    {
        return d_postgres_set_lock_timeout(m_db, _milliseconds);
    }

    int
    get_lock_timeout(
        unsigned& _milliseconds
    ) const D_NOEXCEPT
    {
        return d_postgres_get_lock_timeout(m_db, &_milliseconds);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<unsigned, int>
    get_lock_timeout() const D_NOEXCEPT
    {
        unsigned  value{};
        const int result = d_postgres_get_lock_timeout(m_db, &value);

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
        d_postgres_isolation _level
    ) D_NOEXCEPT
    {
        return d_postgres_set_isolation_level(m_db, _level);
    }

    int
    get_isolation_level(
        d_postgres_isolation& _level
    ) const D_NOEXCEPT
    {
        return d_postgres_get_isolation_level(m_db, &_level);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<d_postgres_isolation, int>
    get_isolation_level() const D_NOEXCEPT
    {
        d_postgres_isolation value{};
        const int            result = d_postgres_get_isolation_level(m_db,
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
    set_client_encoding(
        string_arg _encoding
    ) D_NOEXCEPT
    {
        return d_postgres_set_client_encoding(m_db, _encoding.c_str());
    }

    int
    get_client_encoding(
        text& _encoding
    ) const D_NOEXCEPT
    {
        return d_postgres_get_client_encoding(m_db, _encoding.out());
    }

    int
    get_client_encoding(
        std::string& _encoding
    ) const
    {
        text      text_value;
        const int result = d_postgres_get_client_encoding(m_db,
                                                          text_value.out());

        _encoding = text_value.str();

        return result;
    }

#if D_INTERNAL_DB_FACE_OPTIONAL
    int
    get_client_encoding(
        std::optional<std::string>& _encoding
    ) const
    {
        text      text_value;
        const int result = d_postgres_get_client_encoding(m_db,
                                                          text_value.out());

        // the text; nothing for SQL NULL, and for a failure
        if (text_value.is_null())
        {
            _encoding.reset();
        }
        else
        {
            _encoding = text_value.str();
        }

        return result;
    }
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<text, int>
    get_client_encoding() const D_NOEXCEPT
    {
        text      value{};
        const int result = d_postgres_get_client_encoding(m_db, value.out());

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
        return d_postgres_set_time_zone(m_db, _zone.c_str());
    }

    int
    get_time_zone(
        text& _zone
    ) const D_NOEXCEPT
    {
        return d_postgres_get_time_zone(m_db, _zone.out());
    }

    int
    get_time_zone(
        std::string& _zone
    ) const
    {
        text      text_value;
        const int result = d_postgres_get_time_zone(m_db, text_value.out());

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
        const int result = d_postgres_get_time_zone(m_db, text_value.out());

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
        const int result = d_postgres_get_time_zone(m_db, value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    set_parameter(
        string_arg _name,
        string_arg _value
    ) D_NOEXCEPT
    {
        return d_postgres_set_parameter(m_db, _name.c_str(), _value.c_str());
    }

    int
    get_parameter(
        string_arg _name,
        text&      _value
    ) const D_NOEXCEPT
    {
        return d_postgres_get_parameter(m_db, _name.c_str(), _value.out());
    }

    int
    get_parameter(
        string_arg   _name,
        std::string& _value
    ) const
    {
        text      text_value;
        const int result = d_postgres_get_parameter(m_db,
                                                    _name.c_str(),
                                                    text_value.out());

        _value = text_value.str();

        return result;
    }

#if D_INTERNAL_DB_FACE_OPTIONAL
    int
    get_parameter(
        string_arg                  _name,
        std::optional<std::string>& _value
    ) const
    {
        text      text_value;
        const int result = d_postgres_get_parameter(m_db,
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
    get_parameter(
        string_arg _name
    ) const D_NOEXCEPT
    {
        text      value{};
        const int result = d_postgres_get_parameter(m_db,
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

    // 1.2.1.11
    // Maintenance
    //   postgres_maintenance.h.
    int
    vacuum(
        string_arg _schema,
        string_arg _table,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        return d_postgres_vacuum(m_db, _schema.c_str(), _table.c_str(), _flags);
    }

    int
    analyze(
        string_arg _schema,
        string_arg _table
    ) D_NOEXCEPT
    {
        return d_postgres_analyze(m_db, _schema.c_str(), _table.c_str());
    }

    int
    reindex(
        string_arg _schema,
        string_arg _table,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        return d_postgres_reindex(m_db,
                                  _schema.c_str(),
                                  _table.c_str(),
                                  _flags);
    }

    int
    checkpoint() D_NOEXCEPT
    {
        return d_postgres_checkpoint(m_db);
    }

    int
    table_size(
        string_arg       _schema,
        string_arg       _table,
        re_std::int64_t& _bytes
    ) const D_NOEXCEPT
    {
        return d_postgres_table_size(m_db,
                                     _schema.c_str(),
                                     _table.c_str(),
                                     &_bytes);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<re_std::int64_t, int>
    table_size(
        string_arg _schema,
        string_arg _table
    ) const D_NOEXCEPT
    {
        re_std::int64_t value{};
        const int       result = d_postgres_table_size(m_db,
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
    database_size(
        string_arg       _name,
        re_std::int64_t& _bytes
    ) const D_NOEXCEPT
    {
        return d_postgres_database_size(m_db, _name.c_str(), &_bytes);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<re_std::int64_t, int>
    database_size(
        string_arg _name
    ) const D_NOEXCEPT
    {
        re_std::int64_t value{};
        const int       result = d_postgres_database_size(m_db,
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

    // 1.2.1.12
    // Protocol
    //   postgres_protocol.h: COPY in, notifications, and statements sent
    // without waiting.
    int
    copy_in_start(
        string_arg _sql
    ) D_NOEXCEPT
    {
        return d_postgres_copy_in_start(m_db, _sql.c_str());
    }

    int
    copy_in_data(
        const void* _data,
        std::size_t _size
    ) D_NOEXCEPT
    {
        return d_postgres_copy_in_data(m_db, _data, _size);
    }

    int
    copy_in_end(
        string_arg       _error,
        re_std::int64_t& _changes
    ) D_NOEXCEPT
    {
        return d_postgres_copy_in_end(m_db, _error.c_str(), &_changes);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<re_std::int64_t, int>
    copy_in_end(
        string_arg _error
    ) D_NOEXCEPT
    {
        re_std::int64_t value{};
        const int       result = d_postgres_copy_in_end(m_db,
                                                        _error.c_str(),
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
    listen(
        string_arg _channel
    ) D_NOEXCEPT
    {
        return d_postgres_listen(m_db, _channel.c_str());
    }

    int
    unlisten(
        string_arg _channel
    ) D_NOEXCEPT
    {
        return d_postgres_unlisten(m_db, _channel.c_str());
    }

    int
    notify(
        string_arg _channel,
        string_arg _payload
    ) D_NOEXCEPT
    {
        return d_postgres_notify(m_db, _channel.c_str(), _payload.c_str());
    }

    int
    next_notification(
        text& _channel,
        text& _payload,
        int&  _pid
    ) D_NOEXCEPT
    {
        return d_postgres_next_notification(m_db,
                                            _channel.out(),
                                            _payload.out(),
                                            &_pid);
    }

    int
    next_notification(
        std::string& _channel,
        std::string& _payload,
        int&         _pid
    )
    {
        text      channel_value;
        text      payload_value;
        const int result = d_postgres_next_notification(m_db,
                                                        channel_value.out(),
                                                        payload_value.out(),
                                                        &_pid);

        _channel = channel_value.str();
        _payload = payload_value.str();

        return result;
    }

#if D_INTERNAL_DB_FACE_OPTIONAL
    int
    next_notification(
        std::optional<std::string>& _channel,
        std::optional<std::string>& _payload,
        int&                        _pid
    )
    {
        text      channel_value;
        text      payload_value;
        const int result = d_postgres_next_notification(m_db,
                                                        channel_value.out(),
                                                        payload_value.out(),
                                                        &_pid);

        // the text; nothing for SQL NULL, and for a failure
        if (channel_value.is_null())
        {
            _channel.reset();
        }
        else
        {
            _channel = channel_value.str();
        }

        // the text; nothing for SQL NULL, and for a failure
        if (payload_value.is_null())
        {
            _payload.reset();
        }
        else
        {
            _payload = payload_value.str();
        }

        return result;
    }
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

    int
    send_query(
        string_arg _sql
    ) D_NOEXCEPT
    {
        return d_postgres_send_query(m_db, _sql.c_str());
    }

    int
    next_result(
        rows&            _rows,
        re_std::int64_t& _changes
    ) D_NOEXCEPT
    {
        return d_postgres_next_result(m_db, _rows.out(), &_changes);
    }

    D_NODISCARD bool
    is_busy() const D_NOEXCEPT
    {
        return d_postgres_is_busy(m_db);
    }

    int
    consume_input() D_NOEXCEPT
    {
        return d_postgres_consume_input(m_db);
    }

    int
    set_chunked_rows(
        int _rows
    ) D_NOEXCEPT
    {
        return d_postgres_set_chunked_rows(m_db, _rows);
    }

    int
    send_query_start(
        string_arg    _sql,
        d_db_pending& _pending
    ) D_NOEXCEPT
    {
        return d_postgres_send_query_start(m_db, _sql.c_str(), &_pending);
    }

    int
    send_query_continue(
        unsigned      _events,
        d_db_pending& _pending
    ) D_NOEXCEPT
    {
        return d_postgres_send_query_continue(m_db, _events, &_pending);
    }

    int
    next_result_start(
        d_db_pending&    _pending,
        rows&            _rows,
        re_std::int64_t& _changes
    ) D_NOEXCEPT
    {
        return d_postgres_next_result_start(m_db,
                                            &_pending,
                                            _rows.out(),
                                            &_changes);
    }

    int
    next_result_continue(
        unsigned         _events,
        d_db_pending&    _pending,
        rows&            _rows,
        re_std::int64_t& _changes
    ) D_NOEXCEPT
    {
        return d_postgres_next_result_continue(m_db,
                                               _events,
                                               &_pending,
                                               _rows.out(),
                                               &_changes);
    }

    D_NODISCARD d_postgres_pipeline
    pipeline_status() const D_NOEXCEPT
    {
        return d_postgres_pipeline_status(m_db);
    }

    int
    enter_pipeline() D_NOEXCEPT
    {
        return d_postgres_enter_pipeline(m_db);
    }

    int
    exit_pipeline() D_NOEXCEPT
    {
        return d_postgres_exit_pipeline(m_db);
    }

    int
    pipeline_sync() D_NOEXCEPT
    {
        return d_postgres_pipeline_sync(m_db);
    }

    int
    pipeline_flush() D_NOEXCEPT
    {
        return d_postgres_pipeline_flush(m_db);
    }

    int
    lo_import(
        string_arg    _path,
        unsigned int& _oid
    ) D_NOEXCEPT
    {
        return d_postgres_lo_import(m_db, _path.c_str(), &_oid);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<unsigned int, int>
    lo_import(
        string_arg _path
    ) D_NOEXCEPT
    {
        unsigned int value{};
        const int    result = d_postgres_lo_import(m_db, _path.c_str(), &value);

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    lo_export(
        unsigned int _oid,
        string_arg   _path
    ) D_NOEXCEPT
    {
        return d_postgres_lo_export(m_db, _oid, _path.c_str());
    }

private:
    D_DELETED_FN_MSG(postgres(const postgres&),
                     "a face owns its connection; move it")
    D_DELETED_FN_MSG(postgres& operator=(const postgres&),
                     "a face owns its connection; move it")

    // m_take -- a connect's result; the connection it made replaces the
    // face's when it succeeded. Each connect calls the layer first, in a
    // statement of its own, since the order a call's arguments are
    // evaluated in is the compiler's.
    int
    m_take(
        int             _result,
        struct pg_conn* _made
    ) D_NOEXCEPT
    {
        // a failed connect made nothing to keep
        if (_result == 0)
        {
            reset(_made);
        }

        return _result;
    }

    // m_take_started -- a start's result; the connection being made replaces
    // the face's when it answered D_DB_WAIT
    int
    m_take_started(
        int             _result,
        struct pg_conn* _made
    ) D_NOEXCEPT
    {
        // a failed start made nothing to keep
        if (_result == D_DB_WAIT)
        {
            reset(_made);
        }

        return _result;
    }

    struct pg_conn* m_db;
};

// 1.3    Swapping
//------------------------------------------------------------------------------
// swapping -- the two connections, or statements, trade places, as std::swap
// would have them, for code that swaps unqualified
inline void
swap(
    postgres_statement& _left,
    postgres_statement& _right
) D_NOEXCEPT
{
    _left.swap(_right);

    return;
}

inline void
swap(
    postgres& _left,
    postgres& _right
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
D_STATIC_ASSERT(sizeof(postgres_statement) ==
                    sizeof(struct d_postgres_statement*),
                "face::postgres_statement is its handle, and no more");
D_STATIC_ASSERT(sizeof(postgres) == sizeof(struct pg_conn*),
                "face::postgres is its connection, and no more");
#if D_MOVE_ENABLED
D_STATIC_ASSERT(
    ( (std::is_nothrow_move_constructible<postgres_statement>::value) &&
      (std::is_nothrow_move_assignable<postgres_statement>::value) ),
    "face::postgres_statement moves without throwing");
D_STATIC_ASSERT( ( (std::is_nothrow_move_constructible<postgres>::value) &&
                   (std::is_nothrow_move_assignable<postgres>::value) ),
                 "face::postgres moves without throwing");
#endif  // D_MOVE_ENABLED


}  // namespace face
}  // namespace djinterp


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_DB_POSTGRES_POSTGRES_FACE_HPP
