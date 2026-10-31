/*******************************************************************************
* djinterp [cpp]                                                mongodb_face.hpp
*
* The C++ face of djinterp's MongoDB layer.
*   face::mongodb owns a handle, the layer's struct dj_mongodb around the
* driver's client, and closes it when it goes; every function of the layer
* (mongodb.h) is a member of the same name, the handle argument dropped, each
* with its C function's contract:
*     face::mongodb db;
*     if ( (db.connect_uri("mongodb://db.example/?replicaSet=rs0") == 0) &&
*          (db.use_database("shop") == 0) )
*     {
*         face::transaction<face::mongodb> work(db);
*         face::text                       id;
*
*         db.insert_one(0, "notes", "{\"body\": \"first\"}", id);
*         work.commit();
*     }
* Results are the layer's: 0, the server's code as it sent it, a failure of
* the driver's or libbson's packed above every code a server uses, or a
* negative D_DB_*. Functions that need no handle -- results, the driver's
* versions, its log -- are static. db_face.hpp describes what the face adds,
* how strings, lists and text cross it, and what each language level adds
* beside the exact forms: from C++17 std::optional text, from C++20 formats
* checked at compile time, in JSON's dialect, from C++23 std::expected
* results.
*   connect, connect_uri, connect_with and adopt replace the handle only when
* they succeed; a failure leaves the face as it was, and connect_with puts
* the reason in the text, where one is given. A member called on a face with
* no handle passes NULL to the layer, which answers D_DB_INVALID_ARGUMENT, or
* false, 0 or NULL. A transaction's mode, for face::transaction, is its
* options as JSON -- readConcern, writeConcern, readPreference,
* maxCommitTimeMS -- or NULL for the handle's own; the server has no
* savepoints, so face::savepoint guards none.
*   face::mongodb_cursor owns a cursor, the layer's struct dj_mongodb_cursor,
* and closes it when it goes; find_cursor, aggregate_cursor and
* command_cursor fill one, and every function of mongodb_cursor.h that takes
* a cursor is its member, the cursor argument and the "cursor_" of its name
* dropped:
*     face::mongodb_cursor cursor;
*     face::rows           page;
*     if (db.find_cursor(0, "notes", 0, "{\"batchSize\": 100}", cursor) == 0)
*     {
*         while ( (!cursor.is_done()) &&
*                 (cursor.fetch(100, page) == 0) )
*         {
*             // each document of the page, a row
*         }
*     }
* face::mongodb_change_stream owns a change stream likewise, which the
* watch_ members fill, its members fetch and resume_token. Close every
* cursor and stream -- let it go -- before the handle it reads through.
*   The members that forward are generated, by scripts/gen_db_face.py from the
* layer's prototypes and scripts/db_face/mongodb_face.hpp.in, and keep the C
* functions' contracts, in the headers each part names; those written by
* hand in the template -- the ones that connect, adopt, close or hand over
* the handle, a cursor or a stream, and the transaction mode's beginning --
* carry their own. Edit the template, not this file.
*
*
* path:      /inc/djinterp/cpp/db/mongodb/mongodb_face.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.10.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FACE
    ----
    1.  The cursor
         1.  mongodb_cursor
              1.  Ownership
              2.  Reading
    2.  The change stream
         1.  mongodb_change_stream
              1.  Ownership
              2.  Reading
    3.  The connection
         1.  mongodb
              1.  Ownership
              2.  Results and the driver
              3.  Servers and errors
              4.  Connections
              5.  Commands
              6.  Documents
              7.  Cursors and change streams
              8.  Databases
              9.  Collections, views and indexes
              10. Transactions
              11. Settings
              12. GridFS
              13. Maintenance
    4.  Swapping
2.  COST
    ----
    1.  Sizes and moves
*/

#ifndef DJINTERP_DB_MONGODB_MONGODB_FACE_HPP
#define DJINTERP_DB_MONGODB_MONGODB_FACE_HPP 1

// std
#include <cstddef>  // std::size_t
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "../db_face.hpp"  // string_arg, text, the lists, the levels
#include "../../../c/db/mongodb/mongodb.h"     // the layer
// re_std
#include "../../../../re_std/cpp/cstdint/cstdint.hpp"  // re_std::int64_t, INT64_MAX

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


// 1.1    The cursor
//------------------------------------------------------------------------------
// 1.1.1
// mongodb_cursor
//   class: a cursor -- the layer's struct dj_mongodb_cursor, over a query the
// server has begun -- owned: closed when the object goes, which tells the
// server too, if the documents are not done. Made empty, and filled by
// face::mongodb's find_cursor, aggregate_cursor and command_cursor; every
// function of mongodb_cursor.h that takes a cursor is a member, the
// "cursor_" of its name dropped, whose contract it keeps. Close it before
// the handle it reads through. Not copyable; movable from C++11.
class mongodb_cursor
{
public:
    // 1.1.1.1
    // Ownership
    //   a cursor made empty holds none; whatever a cursor holds, it closes
    // when it goes.
    D_CONSTEXPR mongodb_cursor() D_NOEXCEPT
        : m_cursor(D_NULLPTR)
    {}

    /**
     * @brief Makes a cursor that owns one opened elsewhere.
     *
     * @param[in] _owned  the cursor, or `NULL` for none.
     * @post The object closes `_owned`; the caller no longer does.
     */
    explicit D_CONSTEXPR mongodb_cursor(
        struct dj_mongodb_cursor* _owned
    ) D_NOEXCEPT
        : m_cursor(_owned)
    {}

    ~mongodb_cursor()
    {
        // a cursor, when there is one; an empty object calls nothing
        if (m_cursor)
        {
            (void)d_mongodb_cursor_close(m_cursor);
        }
    }

#if D_MOVE_ENABLED
    // moving -- the cursor passes to the object moved to, which closes what
    // it held; the object moved from holds none
    mongodb_cursor(
        mongodb_cursor&& _other
    ) D_NOEXCEPT
        : m_cursor(_other.release())
    {}

    mongodb_cursor&
    operator=(
        mongodb_cursor&& _other
    ) D_NOEXCEPT
    {
        reset(_other.release());

        return *this;
    }
#endif  // D_MOVE_ENABLED

    // the handle -- the cursor, for the layer's own calls, NULL when there
    // is none; and whether there is one
    D_NODISCARD struct dj_mongodb_cursor*
    handle() const D_NOEXCEPT
    {
        return m_cursor;
    }

    D_NODISCARD bool
    is_open() const D_NOEXCEPT
    {
        return (m_cursor != D_NULLPTR);
    }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    D_EXPLICIT_BOOL operator bool() const D_NOEXCEPT
    {
        return is_open();
    }
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    /**
     * @brief Gives up the cursor without closing it.
     *
     * @return the cursor, which the caller then closes with
     *         d_mongodb_cursor_close; `NULL` when there is none.
     * @post The object holds no cursor.
     */
    D_NODISCARD struct dj_mongodb_cursor*
    release() D_NOEXCEPT
    {
        struct dj_mongodb_cursor* const held = m_cursor;

        m_cursor = D_NULLPTR;

        return held;
    }

    /**
     * @brief Closes the cursor held, if any, and takes another.
     *
     * @param[in] _owned  the cursor to own, or `NULL` for none; the one
     *                    already held is kept, not closed.
     * @post The cursor that was held, if another, is closed.
     */
    void
    reset(
        struct dj_mongodb_cursor* _owned = D_NULLPTR
    ) D_NOEXCEPT
    {
        // what is held is closed, unless it is what comes
        if (_owned != m_cursor)
        {
            // a cursor, when there is one
            if (m_cursor)
            {
                (void)d_mongodb_cursor_close(m_cursor);
            }

            m_cursor = _owned;
        }

        return;
    }

    /**
     * @brief Closes the cursor, as d_mongodb_cursor_close does.
     *
     * @return 0.
     * @post The object holds no cursor.
     */
    int
    close() D_NOEXCEPT
    {
        return d_mongodb_cursor_close(release());
    }

    /**
     * @brief Closes the cursor held, if any, and hands its slot to a
     *        layer's opening to fill.
     *
     * @return where the layer writes the cursor it opens; the object owns
     *         what is written there.
     * @post The object holds no cursor until the opening fills the slot,
     *       and holds none when the opening fails.
     */
    struct dj_mongodb_cursor**
    out() D_NOEXCEPT
    {
        reset();

        return &m_cursor;
    }

    // swap -- the two objects trade cursors
    void
    swap(
        mongodb_cursor& _other
    ) D_NOEXCEPT
    {
        struct dj_mongodb_cursor* const held = m_cursor;

        m_cursor        = _other.m_cursor;
        _other.m_cursor = held;

        return;
    }

    // 1.1.1.2
    // Reading
    //   mongodb_cursor.h's functions that take a cursor.
    int
    fetch(
        std::size_t _max,
        rows&       _rows
    ) D_NOEXCEPT
    {
        return d_mongodb_cursor_fetch(m_cursor, _max, _rows.out());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<rows, int>
    fetch(
        std::size_t _max
    ) D_NOEXCEPT
    {
        rows      value{};
        const int result = d_mongodb_cursor_fetch(m_cursor, _max, value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    D_NODISCARD bool
    is_done() const D_NOEXCEPT
    {
        return d_mongodb_cursor_is_done(m_cursor);
    }

private:
    D_DELETED_FN_MSG(mongodb_cursor(const mongodb_cursor&),
                     "a cursor owns what the server began; move it")
    D_DELETED_FN_MSG(mongodb_cursor& operator=(const mongodb_cursor&),
                     "a cursor owns what the server began; move it")

    struct dj_mongodb_cursor* m_cursor;
};

// 1.2    The change stream
//------------------------------------------------------------------------------
// 1.2.1
// mongodb_change_stream
//   class: a change stream -- the layer's struct dj_mongodb_change_stream --
// owned: closed when the object goes. Made empty, and filled by
// face::mongodb's watch_collection, watch_database and watch_deployment;
// every function of mongodb_cursor.h that takes a stream is a member, the
// "change_stream_" of its name dropped, whose contract it keeps. Close it
// before the handle it reads through. Not copyable; movable from C++11.
class mongodb_change_stream
{
public:
    // 1.2.1.1
    // Ownership
    //   a stream made empty holds none; whatever a stream holds, it closes
    // when it goes.
    D_CONSTEXPR mongodb_change_stream() D_NOEXCEPT
        : m_stream(D_NULLPTR)
    {}

    /**
     * @brief Makes a stream that owns one opened elsewhere.
     *
     * @param[in] _owned  the stream, or `NULL` for none.
     * @post The object closes `_owned`; the caller no longer does.
     */
    explicit D_CONSTEXPR mongodb_change_stream(
        struct dj_mongodb_change_stream* _owned
    ) D_NOEXCEPT
        : m_stream(_owned)
    {}

    ~mongodb_change_stream()
    {
        // a stream, when there is one; an empty object calls nothing
        if (m_stream)
        {
            (void)d_mongodb_change_stream_close(m_stream);
        }
    }

#if D_MOVE_ENABLED
    // moving -- the stream passes to the object moved to, which closes what
    // it held; the object moved from holds none
    mongodb_change_stream(
        mongodb_change_stream&& _other
    ) D_NOEXCEPT
        : m_stream(_other.release())
    {}

    mongodb_change_stream&
    operator=(
        mongodb_change_stream&& _other
    ) D_NOEXCEPT
    {
        reset(_other.release());

        return *this;
    }
#endif  // D_MOVE_ENABLED

    // the handle -- the stream, for the layer's own calls, NULL when there
    // is none; and whether there is one
    D_NODISCARD struct dj_mongodb_change_stream*
    handle() const D_NOEXCEPT
    {
        return m_stream;
    }

    D_NODISCARD bool
    is_open() const D_NOEXCEPT
    {
        return (m_stream != D_NULLPTR);
    }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    D_EXPLICIT_BOOL operator bool() const D_NOEXCEPT
    {
        return is_open();
    }
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    /**
     * @brief Gives up the stream without closing it.
     *
     * @return the stream, which the caller then closes with
     *         d_mongodb_change_stream_close; `NULL` when there is none.
     * @post The object holds no stream.
     */
    D_NODISCARD struct dj_mongodb_change_stream*
    release() D_NOEXCEPT
    {
        struct dj_mongodb_change_stream* const held = m_stream;

        m_stream = D_NULLPTR;

        return held;
    }

    /**
     * @brief Closes the stream held, if any, and takes another.
     *
     * @param[in] _owned  the stream to own, or `NULL` for none; the one
     *                    already held is kept, not closed.
     * @post The stream that was held, if another, is closed.
     */
    void
    reset(
        struct dj_mongodb_change_stream* _owned = D_NULLPTR
    ) D_NOEXCEPT
    {
        // what is held is closed, unless it is what comes
        if (_owned != m_stream)
        {
            // a stream, when there is one
            if (m_stream)
            {
                (void)d_mongodb_change_stream_close(m_stream);
            }

            m_stream = _owned;
        }

        return;
    }

    /**
     * @brief Closes the stream, as d_mongodb_change_stream_close does.
     *
     * @return 0.
     * @post The object holds no stream.
     */
    int
    close() D_NOEXCEPT
    {
        return d_mongodb_change_stream_close(release());
    }

    /**
     * @brief Closes the stream held, if any, and hands its slot to a
     *        layer's watch to fill.
     *
     * @return where the layer writes the stream it opens; the object owns
     *         what is written there.
     * @post The object holds no stream until the watch fills the slot, and
     *       holds none when the watch fails.
     */
    struct dj_mongodb_change_stream**
    out() D_NOEXCEPT
    {
        reset();

        return &m_stream;
    }

    // swap -- the two objects trade streams
    void
    swap(
        mongodb_change_stream& _other
    ) D_NOEXCEPT
    {
        struct dj_mongodb_change_stream* const held = m_stream;

        m_stream        = _other.m_stream;
        _other.m_stream = held;

        return;
    }

    // 1.2.1.2
    // Reading
    //   mongodb_cursor.h's functions that take a stream.
    int
    fetch(
        std::size_t _max,
        rows&       _rows
    ) D_NOEXCEPT
    {
        return d_mongodb_change_stream_fetch(m_stream, _max, _rows.out());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<rows, int>
    fetch(
        std::size_t _max
    ) D_NOEXCEPT
    {
        rows      value{};
        const int result = d_mongodb_change_stream_fetch(m_stream,
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
    resume_token(
        text& _token
    ) const D_NOEXCEPT
    {
        return d_mongodb_change_stream_resume_token(m_stream, _token.out());
    }

    int
    resume_token(
        std::string& _token
    ) const
    {
        text      text_value;
        const int result = d_mongodb_change_stream_resume_token(
            m_stream,
            text_value.out());

        _token = text_value.str();

        return result;
    }

#if D_INTERNAL_DB_FACE_OPTIONAL
    int
    resume_token(
        std::optional<std::string>& _token
    ) const
    {
        text      text_value;
        const int result = d_mongodb_change_stream_resume_token(
            m_stream,
            text_value.out());

        // the text; nothing for SQL NULL, and for a failure
        if (text_value.is_null())
        {
            _token.reset();
        }
        else
        {
            _token = text_value.str();
        }

        return result;
    }
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<text, int>
    resume_token() const D_NOEXCEPT
    {
        text      value{};
        const int result = d_mongodb_change_stream_resume_token(m_stream,
                                                                value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

private:
    D_DELETED_FN_MSG(mongodb_change_stream(const mongodb_change_stream&),
                     "a stream owns what the server began; move it")
    D_DELETED_FN_MSG(
        mongodb_change_stream& operator=(const mongodb_change_stream&),
        "a stream owns what the server began; move it")

    struct dj_mongodb_change_stream* m_stream;
};

// 1.3    The connection
//------------------------------------------------------------------------------
// 1.3.1
// mongodb
//   class: a MongoDB handle, owned: closed when the object goes. Every
// member forwards to the layer's function of the same name, whose contract
// it keeps. Not copyable; movable from C++11.
class mongodb
{
public:
    // transaction_mode
    //   type: how a transaction begins, for face::transaction: its options
    // as JSON, as d_mongodb_begin_transaction_with reads them, or `NULL` for
    // the handle's read and write concerns and read preference.
    typedef const char* transaction_mode;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // format
    //   type: an f form's format, for the arguments that follow it: a
    // literal in JSON's dialect, read at compile time from C++20, or a
    // format runtime_format marked (db_face.hpp, 2.3).
    template<typename... Args>
    using format = format_arg<internal::json_dialect, Args...>;
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    // 1.3.1.1
    // Ownership
    //   a face made empty holds no handle; whatever a face holds, it closes
    // when it goes.
    D_CONSTEXPR mongodb() D_NOEXCEPT
        : m_db(D_NULLPTR)
    {}

    /**
     * @brief Makes a face that owns a handle made elsewhere, by the layer's
     *        own calls, say.
     *
     * @param[in] _owned  the handle, or `NULL` for none.
     * @post The face closes `_owned`; the caller no longer does.
     */
    explicit D_CONSTEXPR mongodb(
        struct dj_mongodb* _owned
    ) D_NOEXCEPT
        : m_db(_owned)
    {}

    ~mongodb()
    {
        // a handle, when there is one; an empty face calls nothing
        if (m_db)
        {
            (void)d_mongodb_close(m_db);
        }
    }

#if D_MOVE_ENABLED
    // moving -- the handle passes to the face moved to, which closes what
    // it held; the face moved from holds none
    mongodb(
        mongodb&& _other
    ) D_NOEXCEPT
        : m_db(_other.release())
    {}

    mongodb&
    operator=(
        mongodb&& _other
    ) D_NOEXCEPT
    {
        reset(_other.release());

        return *this;
    }
#endif  // D_MOVE_ENABLED

    // the handle -- the layer's, for its own calls, NULL when there is none;
    // and whether there is one
    D_NODISCARD struct dj_mongodb*
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
     * @return the handle, which the caller then closes with d_mongodb_close;
     *         `NULL` when there is none.
     * @post The face holds no handle.
     */
    D_NODISCARD struct dj_mongodb*
    release() D_NOEXCEPT
    {
        struct dj_mongodb* const held = m_db;

        m_db = D_NULLPTR;

        return held;
    }

    /**
     * @brief Closes the handle held, if any, and takes another.
     *
     * @param[in] _owned  the handle to own, or `NULL` for none; the one
     *                    already held is kept, not closed.
     * @post The handle that was held, if another, is closed, and every
     *       cursor and stream read through it must have been closed before.
     */
    void
    reset(
        struct dj_mongodb* _owned = D_NULLPTR
    ) D_NOEXCEPT
    {
        // what is held closes, unless it is what comes
        if (_owned != m_db)
        {
            // a handle, when there is one
            if (m_db)
            {
                (void)d_mongodb_close(m_db);
            }

            m_db = _owned;
        }

        return;
    }

    // swap -- the two faces trade handles
    void
    swap(
        mongodb& _other
    ) D_NOEXCEPT
    {
        struct dj_mongodb* const held = m_db;

        m_db        = _other.m_db;
        _other.m_db = held;

        return;
    }

    // default_transaction_mode -- how face::transaction begins when told
    // nothing: with no options, the handle's concerns and preference
    D_NODISCARD static D_CONSTEXPR transaction_mode
    default_transaction_mode() D_NOEXCEPT
    {
        return D_NULLPTR;
    }

    // 1.3.1.2
    // Results and the driver
    //   mongodb_common.h's results, the driver's versions and its log:
    // static, since they need no handle.
    D_NODISCARD static bool
    result_is_client(
        int _result
    ) D_NOEXCEPT
    {
        return d_mongodb_result_is_client(_result);
    }

    D_NODISCARD static unsigned
    result_domain(
        int _result
    ) D_NOEXCEPT
    {
        return d_mongodb_result_domain(_result);
    }

    D_NODISCARD static unsigned
    result_code(
        int _result
    ) D_NOEXCEPT
    {
        return d_mongodb_result_code(_result);
    }

    D_NODISCARD static bool
    result_is_unreachable(
        int _result
    ) D_NOEXCEPT
    {
        return d_mongodb_result_is_unreachable(_result);
    }

    D_NODISCARD static const char*
    result_message(
        int _result
    ) D_NOEXCEPT
    {
        return d_mongodb_result_message(_result);
    }

    D_NODISCARD static const char*
    client_version() D_NOEXCEPT
    {
        return d_mongodb_client_version();
    }

    D_NODISCARD static unsigned long
    client_version_number() D_NOEXCEPT
    {
        return d_mongodb_client_version_number();
    }

    D_NODISCARD static const char*
    bson_version() D_NOEXCEPT
    {
        return d_mongodb_bson_version();
    }

    static int
    set_log_level(
        int _level
    ) D_NOEXCEPT
    {
        return d_mongodb_set_log_level(_level);
    }

    D_NODISCARD static int
    get_log_level() D_NOEXCEPT
    {
        return d_mongodb_get_log_level();
    }

    // 1.3.1.3
    // Servers and errors
    //   mongodb_common.h's server, its features, the driver's client, and
    // the handle's latest failure.
    D_NODISCARD const char*
    server_version() const D_NOEXCEPT
    {
        return d_mongodb_server_version(m_db);
    }

    D_NODISCARD unsigned long
    server_version_number() const D_NOEXCEPT
    {
        return d_mongodb_server_version_number(m_db);
    }

    D_NODISCARD bool
    server_at_least(
        unsigned _major,
        unsigned _minor,
        unsigned _patch
    ) const D_NOEXCEPT
    {
        return d_mongodb_server_at_least(m_db, _major, _minor, _patch);
    }

    D_NODISCARD int
    server_wire_version() const D_NOEXCEPT
    {
        return d_mongodb_server_wire_version(m_db);
    }

    D_NODISCARD dj_mongodb_topology
    server_topology() const D_NOEXCEPT
    {
        return d_mongodb_server_topology(m_db);
    }

    D_NODISCARD const char*
    server_replica_set() const D_NOEXCEPT
    {
        return d_mongodb_server_replica_set(m_db);
    }

    D_NODISCARD bool
    supports(
        dj_mongodb_feature _feature
    ) const D_NOEXCEPT
    {
        return d_mongodb_supports(m_db, _feature);
    }

    D_NODISCARD void*
    native() const D_NOEXCEPT
    {
        return d_mongodb_native(m_db);
    }

    D_NODISCARD int
    error_code() const D_NOEXCEPT
    {
        return d_mongodb_error_code(m_db);
    }

    D_NODISCARD const char*
    error_message() const D_NOEXCEPT
    {
        return d_mongodb_error_message(m_db);
    }

    D_NODISCARD const char*
    error_name() const D_NOEXCEPT
    {
        return d_mongodb_error_name(m_db);
    }

    D_NODISCARD bool
    error_has_label(
        string_arg _label
    ) const D_NOEXCEPT
    {
        return d_mongodb_error_has_label(m_db, _label.c_str());
    }

    int
    error_reply(
        text& _reply
    ) const D_NOEXCEPT
    {
        return d_mongodb_error_reply(m_db, _reply.out());
    }

    int
    error_reply(
        std::string& _reply
    ) const
    {
        text      text_value;
        const int result = d_mongodb_error_reply(m_db, text_value.out());

        _reply = text_value.str();

        return result;
    }

#if D_INTERNAL_DB_FACE_OPTIONAL
    int
    error_reply(
        std::optional<std::string>& _reply
    ) const
    {
        text      text_value;
        const int result = d_mongodb_error_reply(m_db, text_value.out());

        // the text; nothing for SQL NULL, and for a failure
        if (text_value.is_null())
        {
            _reply.reset();
        }
        else
        {
            _reply = text_value.str();
        }

        return result;
    }
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<text, int>
    error_reply() const D_NOEXCEPT
    {
        text      value{};
        const int result = d_mongodb_error_reply(m_db, value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    // 1.3.1.4
    // Connections
    //   mongodb_connection.h.

    /**
     * @brief Connects, as d_mongodb_connect does, in place of the handle
     *        held.
     *
     * @param[in] _options  where and as whom; `NULL` leaves every field to
     *                      the driver.
     * @return d_mongodb_connect's result.
     * @post On success the face holds the new handle, and the one it held
     *       is closed; on failure the face is as it was.
     */
    int
    connect(
        const struct dj_mongodb_options* _options
    ) D_NOEXCEPT
    {
        struct dj_mongodb* made   = D_NULLPTR;
        const int         result = d_mongodb_connect(_options, &made);

        return m_take(result, made);
    }

    /**
     * @brief Connects as a connection string says, as d_mongodb_connect_uri
     *        does, in place of the handle held.
     *
     * @param[in] _uri  the connection string, mongodb:// or mongodb+srv://.
     * @return d_mongodb_connect_uri's result.
     * @post As for connect.
     */
    int
    connect_uri(
        string_arg _uri
    ) D_NOEXCEPT
    {
        struct dj_mongodb* made   = D_NULLPTR;
        const int         result = d_mongodb_connect_uri(_uri.c_str(), &made);

        return m_take(result, made);
    }

    /**
     * @brief Connects with settings applied first, as
     *        d_mongodb_connect_with does, in place of the handle held.
     *
     * @param[in] _options   where and as whom; `NULL` leaves every field to
     *                       the driver.
     * @param[in] _settings  the settings; may be `NULL` when `_count` is 0.
     * @param[in] _count     how many there are.
     * @return d_mongodb_connect_with's result.
     * @post As for connect.
     */
    int
    connect_with(
        const struct dj_mongodb_options* _options,
        const struct dj_mongodb_setting* _settings,
        std::size_t                     _count
    ) D_NOEXCEPT
    {
        struct dj_mongodb* made   = D_NULLPTR;
        const int         result = d_mongodb_connect_with(_options,
                                                          _settings,
                                                          _count,
                                                          &made,
                                                          D_NULLPTR);

        return m_take(result, made);
    }

    /**
     * @brief Connects as connect_with does, and says why a handle could not
     *        be made.
     *
     * @param[in]  _options   where and as whom; `NULL` leaves every field to
     *                        the driver.
     * @param[in]  _settings  the settings; may be `NULL` when `_count` is 0.
     * @param[in]  _count     how many there are.
     * @param[out] _message   the reason when no handle could be made: the
     *                        driver's, the server's or the layer's; null
     *                        otherwise.
     * @return d_mongodb_connect_with's result.
     * @post As for connect.
     */
    int
    connect_with(
        const struct dj_mongodb_options* _options,
        const struct dj_mongodb_setting* _settings,
        std::size_t                     _count,
        text&                           _message
    ) D_NOEXCEPT
    {
        struct dj_mongodb* made   = D_NULLPTR;
        const int         result = d_mongodb_connect_with(_options,
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
        const struct dj_mongodb_options* _options,
        const struct dj_mongodb_setting  (&_settings)[Count]
    ) D_NOEXCEPT
    {
        return connect_with(_options, _settings, Count);
    }

    template<std::size_t Count>
    int
    connect_with(
        const struct dj_mongodb_options* _options,
        const struct dj_mongodb_setting  (&_settings)[Count],
        text&                           _message
    ) D_NOEXCEPT
    {
        return connect_with(_options, _settings, Count, _message);
    }

#if D_INTERNAL_DB_FACE_SPAN
    int
    connect_with(
        const struct dj_mongodb_options*           _options,
        std::span<const struct dj_mongodb_setting> _settings
    ) noexcept
    {
        return connect_with(_options, _settings.data(), _settings.size());
    }

    int
    connect_with(
        const struct dj_mongodb_options*           _options,
        std::span<const struct dj_mongodb_setting> _settings,
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
     * @brief Makes a handle around a client of the driver's, as
     *        d_mongodb_adopt does, in place of the handle held.
     *
     * @param[in] _native  the driver's mongoc_client_t.
     * @param[in] _owned   whether the handle destroys the client when it
     *                     closes.
     * @return d_mongodb_adopt's result.
     * @post As for connect; on failure the client is the caller's still.
     */
    int
    adopt(
        void* _native,
        bool  _owned
    ) D_NOEXCEPT
    {
        struct dj_mongodb* made   = D_NULLPTR;
        const int         result = d_mongodb_adopt(_native, _owned, &made);

        return m_take(result, made);
    }

    /**
     * @brief Closes the handle, as d_mongodb_close does.
     *
     * @return 0.
     * @post The face holds no handle.
     */
    int
    close() D_NOEXCEPT
    {
        return d_mongodb_close(release());
    }

    int
    ping() D_NOEXCEPT
    {
        return d_mongodb_ping(m_db);
    }

    D_NODISCARD re_std::int64_t
    changes() const D_NOEXCEPT
    {
        return d_mongodb_changes(m_db);
    }

    int
    last_insert_id(
        text& _id
    ) const D_NOEXCEPT
    {
        return d_mongodb_last_insert_id(m_db, _id.out());
    }

    int
    last_insert_id(
        std::string& _id
    ) const
    {
        text      text_value;
        const int result = d_mongodb_last_insert_id(m_db, text_value.out());

        _id = text_value.str();

        return result;
    }

#if D_INTERNAL_DB_FACE_OPTIONAL
    int
    last_insert_id(
        std::optional<std::string>& _id
    ) const
    {
        text      text_value;
        const int result = d_mongodb_last_insert_id(m_db, text_value.out());

        // the text; nothing for SQL NULL, and for a failure
        if (text_value.is_null())
        {
            _id.reset();
        }
        else
        {
            _id = text_value.str();
        }

        return result;
    }
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<text, int>
    last_insert_id() const D_NOEXCEPT
    {
        text      value{};
        const int result = d_mongodb_last_insert_id(m_db, value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    // 1.3.1.5
    // Commands
    //   mongodb_query.h; the f forms from C++11, their formats checked from
    // C++20 (db_face.hpp, 2.2 to 2.5).
    int
    exec(
        string_arg _database,
        string_arg _command
    ) D_NOEXCEPT
    {
        return d_mongodb_exec(m_db, _database.c_str(), _command.c_str());
    }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    template<typename... Args>
    int
    execf(
        string_arg      _database,
        format<Args...> _format,
        Args...         _args
    ) D_NOEXCEPT
    {
        internal::check_varargs<Args...>();

        return d_mongodb_execf(m_db,
                               _database.c_str(),
                               _format.c_str(),
                               _args...);
    }
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    int
    exec_changes(
        string_arg       _database,
        re_std::int64_t& _changes,
        string_arg       _command
    ) D_NOEXCEPT
    {
        return d_mongodb_exec_changes(m_db,
                                      _database.c_str(),
                                      &_changes,
                                      _command.c_str());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<re_std::int64_t, int>
    exec_changes(
        string_arg _database,
        string_arg _command
    ) D_NOEXCEPT
    {
        re_std::int64_t value{};
        const int       result = d_mongodb_exec_changes(m_db,
                                                        _database.c_str(),
                                                        &value,
                                                        _command.c_str());

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
        string_arg       _database,
        re_std::int64_t& _changes,
        format<Args...>  _format,
        Args...          _args
    ) D_NOEXCEPT
    {
        internal::check_varargs<Args...>();

        return d_mongodb_execf_changes(m_db,
                                       _database.c_str(),
                                       &_changes,
                                       _format.c_str(),
                                       _args...);
    }
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_INTERNAL_DB_FACE_EXPECTED
    template<typename... Args>
    D_NODISCARD std::expected<re_std::int64_t, int>
    execf_changes(
        string_arg      _database,
        format<Args...> _format,
        Args...         _args
    ) D_NOEXCEPT
    {
        internal::check_varargs<Args...>();

        re_std::int64_t value{};
        const int       result = d_mongodb_execf_changes(m_db,
                                                         _database.c_str(),
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
    command(
        string_arg _database,
        text&      _reply,
        string_arg _command
    ) D_NOEXCEPT
    {
        return d_mongodb_command(m_db,
                                 _database.c_str(),
                                 _reply.out(),
                                 _command.c_str());
    }

    int
    command(
        string_arg   _database,
        std::string& _reply,
        string_arg   _command
    )
    {
        text      text_value;
        const int result = d_mongodb_command(m_db,
                                             _database.c_str(),
                                             text_value.out(),
                                             _command.c_str());

        _reply = text_value.str();

        return result;
    }

#if D_INTERNAL_DB_FACE_OPTIONAL
    int
    command(
        string_arg                  _database,
        std::optional<std::string>& _reply,
        string_arg                  _command
    )
    {
        text      text_value;
        const int result = d_mongodb_command(m_db,
                                             _database.c_str(),
                                             text_value.out(),
                                             _command.c_str());

        // the text; nothing for SQL NULL, and for a failure
        if (text_value.is_null())
        {
            _reply.reset();
        }
        else
        {
            _reply = text_value.str();
        }

        return result;
    }
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<text, int>
    command(
        string_arg _database,
        string_arg _command
    ) D_NOEXCEPT
    {
        text      value{};
        const int result = d_mongodb_command(m_db,
                                             _database.c_str(),
                                             value.out(),
                                             _command.c_str());

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
    commandf(
        string_arg      _database,
        text&           _reply,
        format<Args...> _format,
        Args...         _args
    ) D_NOEXCEPT
    {
        internal::check_varargs<Args...>();

        return d_mongodb_commandf(m_db,
                                  _database.c_str(),
                                  _reply.out(),
                                  _format.c_str(),
                                  _args...);
    }
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    template<typename... Args>
    int
    commandf(
        string_arg      _database,
        std::string&    _reply,
        format<Args...> _format,
        Args...         _args
    )
    {
        internal::check_varargs<Args...>();

        text      text_value;
        const int result = d_mongodb_commandf(m_db,
                                              _database.c_str(),
                                              text_value.out(),
                                              _format.c_str(),
                                              _args...);

        _reply = text_value.str();

        return result;
    }
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_INTERNAL_DB_FACE_OPTIONAL
    template<typename... Args>
    int
    commandf(
        string_arg                  _database,
        std::optional<std::string>& _reply,
        format<Args...>             _format,
        Args...                     _args
    )
    {
        internal::check_varargs<Args...>();

        text      text_value;
        const int result = d_mongodb_commandf(m_db,
                                              _database.c_str(),
                                              text_value.out(),
                                              _format.c_str(),
                                              _args...);

        // the text; nothing for SQL NULL, and for a failure
        if (text_value.is_null())
        {
            _reply.reset();
        }
        else
        {
            _reply = text_value.str();
        }

        return result;
    }
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

#if D_INTERNAL_DB_FACE_EXPECTED
    template<typename... Args>
    D_NODISCARD std::expected<text, int>
    commandf(
        string_arg      _database,
        format<Args...> _format,
        Args...         _args
    ) D_NOEXCEPT
    {
        internal::check_varargs<Args...>();

        text      value{};
        const int result = d_mongodb_commandf(m_db,
                                              _database.c_str(),
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
        string_arg _database,
        rows&      _rows,
        string_arg _command
    ) D_NOEXCEPT
    {
        return d_mongodb_query_rows(m_db,
                                    _database.c_str(),
                                    _rows.out(),
                                    _command.c_str());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<rows, int>
    query_rows(
        string_arg _database,
        string_arg _command
    ) D_NOEXCEPT
    {
        rows      value{};
        const int result = d_mongodb_query_rows(m_db,
                                                _database.c_str(),
                                                value.out(),
                                                _command.c_str());

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
        string_arg      _database,
        rows&           _rows,
        format<Args...> _format,
        Args...         _args
    ) D_NOEXCEPT
    {
        internal::check_varargs<Args...>();

        return d_mongodb_query_rowsf(m_db,
                                     _database.c_str(),
                                     _rows.out(),
                                     _format.c_str(),
                                     _args...);
    }
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_INTERNAL_DB_FACE_EXPECTED
    template<typename... Args>
    D_NODISCARD std::expected<rows, int>
    query_rowsf(
        string_arg      _database,
        format<Args...> _format,
        Args...         _args
    ) D_NOEXCEPT
    {
        internal::check_varargs<Args...>();

        rows      value{};
        const int result = d_mongodb_query_rowsf(m_db,
                                                 _database.c_str(),
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

    // 1.3.1.6
    // Documents
    //   mongodb_document.h.
    int
    insert_one(
        string_arg _database,
        string_arg _collection,
        string_arg _document,
        text&      _id
    ) D_NOEXCEPT
    {
        return d_mongodb_insert_one(m_db,
                                    _database.c_str(),
                                    _collection.c_str(),
                                    _document.c_str(),
                                    _id.out());
    }

    int
    insert_one(
        string_arg   _database,
        string_arg   _collection,
        string_arg   _document,
        std::string& _id
    )
    {
        text      text_value;
        const int result = d_mongodb_insert_one(m_db,
                                                _database.c_str(),
                                                _collection.c_str(),
                                                _document.c_str(),
                                                text_value.out());

        _id = text_value.str();

        return result;
    }

#if D_INTERNAL_DB_FACE_OPTIONAL
    int
    insert_one(
        string_arg                  _database,
        string_arg                  _collection,
        string_arg                  _document,
        std::optional<std::string>& _id
    )
    {
        text      text_value;
        const int result = d_mongodb_insert_one(m_db,
                                                _database.c_str(),
                                                _collection.c_str(),
                                                _document.c_str(),
                                                text_value.out());

        // the text; nothing for SQL NULL, and for a failure
        if (text_value.is_null())
        {
            _id.reset();
        }
        else
        {
            _id = text_value.str();
        }

        return result;
    }
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<text, int>
    insert_one(
        string_arg _database,
        string_arg _collection,
        string_arg _document
    ) D_NOEXCEPT
    {
        text      value{};
        const int result = d_mongodb_insert_one(m_db,
                                                _database.c_str(),
                                                _collection.c_str(),
                                                _document.c_str(),
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
    insert_many(
        string_arg       _database,
        string_arg       _collection,
        string_arg       _documents,
        unsigned         _flags,
        re_std::int64_t& _inserted
    ) D_NOEXCEPT
    {
        return d_mongodb_insert_many(m_db,
                                     _database.c_str(),
                                     _collection.c_str(),
                                     _documents.c_str(),
                                     _flags,
                                     &_inserted);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<re_std::int64_t, int>
    insert_many(
        string_arg _database,
        string_arg _collection,
        string_arg _documents,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        re_std::int64_t value{};
        const int       result = d_mongodb_insert_many(m_db,
                                                       _database.c_str(),
                                                       _collection.c_str(),
                                                       _documents.c_str(),
                                                       _flags,
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
    find(
        string_arg _database,
        string_arg _collection,
        string_arg _filter,
        string_arg _options,
        rows&      _rows
    ) D_NOEXCEPT
    {
        return d_mongodb_find(m_db,
                              _database.c_str(),
                              _collection.c_str(),
                              _filter.c_str(),
                              _options.c_str(),
                              _rows.out());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<rows, int>
    find(
        string_arg _database,
        string_arg _collection,
        string_arg _filter,
        string_arg _options
    ) D_NOEXCEPT
    {
        rows      value{};
        const int result = d_mongodb_find(m_db,
                                          _database.c_str(),
                                          _collection.c_str(),
                                          _filter.c_str(),
                                          _options.c_str(),
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
    find_one(
        string_arg _database,
        string_arg _collection,
        string_arg _filter,
        string_arg _options,
        rows&      _rows
    ) D_NOEXCEPT
    {
        return d_mongodb_find_one(m_db,
                                  _database.c_str(),
                                  _collection.c_str(),
                                  _filter.c_str(),
                                  _options.c_str(),
                                  _rows.out());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<rows, int>
    find_one(
        string_arg _database,
        string_arg _collection,
        string_arg _filter,
        string_arg _options
    ) D_NOEXCEPT
    {
        rows      value{};
        const int result = d_mongodb_find_one(m_db,
                                              _database.c_str(),
                                              _collection.c_str(),
                                              _filter.c_str(),
                                              _options.c_str(),
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
    find_keyed(
        string_arg _database,
        string_arg _collection,
        string_arg _filter,
        string_arg _options,
        rows&      _rows
    ) D_NOEXCEPT
    {
        return d_mongodb_find_keyed(m_db,
                                    _database.c_str(),
                                    _collection.c_str(),
                                    _filter.c_str(),
                                    _options.c_str(),
                                    _rows.out());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<rows, int>
    find_keyed(
        string_arg _database,
        string_arg _collection,
        string_arg _filter,
        string_arg _options
    ) D_NOEXCEPT
    {
        rows      value{};
        const int result = d_mongodb_find_keyed(m_db,
                                                _database.c_str(),
                                                _collection.c_str(),
                                                _filter.c_str(),
                                                _options.c_str(),
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
    update_one(
        string_arg               _database,
        string_arg               _collection,
        string_arg               _filter,
        string_arg               _update,
        string_arg               _options,
        dj_mongodb_write_counts& _counts
    ) D_NOEXCEPT
    {
        return d_mongodb_update_one(m_db,
                                    _database.c_str(),
                                    _collection.c_str(),
                                    _filter.c_str(),
                                    _update.c_str(),
                                    _options.c_str(),
                                    &_counts);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<dj_mongodb_write_counts, int>
    update_one(
        string_arg _database,
        string_arg _collection,
        string_arg _filter,
        string_arg _update,
        string_arg _options
    ) D_NOEXCEPT
    {
        dj_mongodb_write_counts value{};
        const int               result = d_mongodb_update_one(
            m_db,
            _database.c_str(),
            _collection.c_str(),
            _filter.c_str(),
            _update.c_str(),
            _options.c_str(),
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
    update_many(
        string_arg               _database,
        string_arg               _collection,
        string_arg               _filter,
        string_arg               _update,
        string_arg               _options,
        dj_mongodb_write_counts& _counts
    ) D_NOEXCEPT
    {
        return d_mongodb_update_many(m_db,
                                     _database.c_str(),
                                     _collection.c_str(),
                                     _filter.c_str(),
                                     _update.c_str(),
                                     _options.c_str(),
                                     &_counts);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<dj_mongodb_write_counts, int>
    update_many(
        string_arg _database,
        string_arg _collection,
        string_arg _filter,
        string_arg _update,
        string_arg _options
    ) D_NOEXCEPT
    {
        dj_mongodb_write_counts value{};
        const int               result = d_mongodb_update_many(
            m_db,
            _database.c_str(),
            _collection.c_str(),
            _filter.c_str(),
            _update.c_str(),
            _options.c_str(),
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
    replace_one(
        string_arg               _database,
        string_arg               _collection,
        string_arg               _filter,
        string_arg               _replacement,
        string_arg               _options,
        dj_mongodb_write_counts& _counts
    ) D_NOEXCEPT
    {
        return d_mongodb_replace_one(m_db,
                                     _database.c_str(),
                                     _collection.c_str(),
                                     _filter.c_str(),
                                     _replacement.c_str(),
                                     _options.c_str(),
                                     &_counts);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<dj_mongodb_write_counts, int>
    replace_one(
        string_arg _database,
        string_arg _collection,
        string_arg _filter,
        string_arg _replacement,
        string_arg _options
    ) D_NOEXCEPT
    {
        dj_mongodb_write_counts value{};
        const int               result = d_mongodb_replace_one(
            m_db,
            _database.c_str(),
            _collection.c_str(),
            _filter.c_str(),
            _replacement.c_str(),
            _options.c_str(),
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
    delete_one(
        string_arg       _database,
        string_arg       _collection,
        string_arg       _filter,
        string_arg       _options,
        re_std::int64_t& _deleted
    ) D_NOEXCEPT
    {
        return d_mongodb_delete_one(m_db,
                                    _database.c_str(),
                                    _collection.c_str(),
                                    _filter.c_str(),
                                    _options.c_str(),
                                    &_deleted);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<re_std::int64_t, int>
    delete_one(
        string_arg _database,
        string_arg _collection,
        string_arg _filter,
        string_arg _options
    ) D_NOEXCEPT
    {
        re_std::int64_t value{};
        const int       result = d_mongodb_delete_one(m_db,
                                                      _database.c_str(),
                                                      _collection.c_str(),
                                                      _filter.c_str(),
                                                      _options.c_str(),
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
    delete_many(
        string_arg       _database,
        string_arg       _collection,
        string_arg       _filter,
        string_arg       _options,
        re_std::int64_t& _deleted
    ) D_NOEXCEPT
    {
        return d_mongodb_delete_many(m_db,
                                     _database.c_str(),
                                     _collection.c_str(),
                                     _filter.c_str(),
                                     _options.c_str(),
                                     &_deleted);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<re_std::int64_t, int>
    delete_many(
        string_arg _database,
        string_arg _collection,
        string_arg _filter,
        string_arg _options
    ) D_NOEXCEPT
    {
        re_std::int64_t value{};
        const int       result = d_mongodb_delete_many(m_db,
                                                       _database.c_str(),
                                                       _collection.c_str(),
                                                       _filter.c_str(),
                                                       _options.c_str(),
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
    count_documents(
        string_arg       _database,
        string_arg       _collection,
        string_arg       _filter,
        string_arg       _options,
        re_std::int64_t& _count
    ) const D_NOEXCEPT
    {
        return d_mongodb_count_documents(m_db,
                                         _database.c_str(),
                                         _collection.c_str(),
                                         _filter.c_str(),
                                         _options.c_str(),
                                         &_count);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<re_std::int64_t, int>
    count_documents(
        string_arg _database,
        string_arg _collection,
        string_arg _filter,
        string_arg _options
    ) const D_NOEXCEPT
    {
        re_std::int64_t value{};
        const int       result = d_mongodb_count_documents(m_db,
                                                           _database.c_str(),
                                                           _collection.c_str(),
                                                           _filter.c_str(),
                                                           _options.c_str(),
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
    estimated_document_count(
        string_arg       _database,
        string_arg       _collection,
        string_arg       _options,
        re_std::int64_t& _count
    ) const D_NOEXCEPT
    {
        return d_mongodb_estimated_document_count(m_db,
                                                  _database.c_str(),
                                                  _collection.c_str(),
                                                  _options.c_str(),
                                                  &_count);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<re_std::int64_t, int>
    estimated_document_count(
        string_arg _database,
        string_arg _collection,
        string_arg _options
    ) const D_NOEXCEPT
    {
        re_std::int64_t value{};
        const int       result = d_mongodb_estimated_document_count(
            m_db,
            _database.c_str(),
            _collection.c_str(),
            _options.c_str(),
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
    distinct(
        string_arg _database,
        string_arg _collection,
        string_arg _field,
        string_arg _filter,
        string_arg _options,
        rows&      _rows
    ) D_NOEXCEPT
    {
        return d_mongodb_distinct(m_db,
                                  _database.c_str(),
                                  _collection.c_str(),
                                  _field.c_str(),
                                  _filter.c_str(),
                                  _options.c_str(),
                                  _rows.out());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<rows, int>
    distinct(
        string_arg _database,
        string_arg _collection,
        string_arg _field,
        string_arg _filter,
        string_arg _options
    ) D_NOEXCEPT
    {
        rows      value{};
        const int result = d_mongodb_distinct(m_db,
                                              _database.c_str(),
                                              _collection.c_str(),
                                              _field.c_str(),
                                              _filter.c_str(),
                                              _options.c_str(),
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
    aggregate(
        string_arg _database,
        string_arg _collection,
        string_arg _pipeline,
        string_arg _options,
        rows&      _rows
    ) D_NOEXCEPT
    {
        return d_mongodb_aggregate(m_db,
                                   _database.c_str(),
                                   _collection.c_str(),
                                   _pipeline.c_str(),
                                   _options.c_str(),
                                   _rows.out());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<rows, int>
    aggregate(
        string_arg _database,
        string_arg _collection,
        string_arg _pipeline,
        string_arg _options
    ) D_NOEXCEPT
    {
        rows      value{};
        const int result = d_mongodb_aggregate(m_db,
                                               _database.c_str(),
                                               _collection.c_str(),
                                               _pipeline.c_str(),
                                               _options.c_str(),
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
    find_one_and_update(
        string_arg _database,
        string_arg _collection,
        string_arg _filter,
        string_arg _update,
        string_arg _options,
        rows&      _rows
    ) D_NOEXCEPT
    {
        return d_mongodb_find_one_and_update(m_db,
                                             _database.c_str(),
                                             _collection.c_str(),
                                             _filter.c_str(),
                                             _update.c_str(),
                                             _options.c_str(),
                                             _rows.out());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<rows, int>
    find_one_and_update(
        string_arg _database,
        string_arg _collection,
        string_arg _filter,
        string_arg _update,
        string_arg _options
    ) D_NOEXCEPT
    {
        rows      value{};
        const int result = d_mongodb_find_one_and_update(m_db,
                                                         _database.c_str(),
                                                         _collection.c_str(),
                                                         _filter.c_str(),
                                                         _update.c_str(),
                                                         _options.c_str(),
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
    find_one_and_replace(
        string_arg _database,
        string_arg _collection,
        string_arg _filter,
        string_arg _replacement,
        string_arg _options,
        rows&      _rows
    ) D_NOEXCEPT
    {
        return d_mongodb_find_one_and_replace(m_db,
                                              _database.c_str(),
                                              _collection.c_str(),
                                              _filter.c_str(),
                                              _replacement.c_str(),
                                              _options.c_str(),
                                              _rows.out());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<rows, int>
    find_one_and_replace(
        string_arg _database,
        string_arg _collection,
        string_arg _filter,
        string_arg _replacement,
        string_arg _options
    ) D_NOEXCEPT
    {
        rows      value{};
        const int result = d_mongodb_find_one_and_replace(m_db,
                                                          _database.c_str(),
                                                          _collection.c_str(),
                                                          _filter.c_str(),
                                                          _replacement.c_str(),
                                                          _options.c_str(),
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
    find_one_and_delete(
        string_arg _database,
        string_arg _collection,
        string_arg _filter,
        string_arg _options,
        rows&      _rows
    ) D_NOEXCEPT
    {
        return d_mongodb_find_one_and_delete(m_db,
                                             _database.c_str(),
                                             _collection.c_str(),
                                             _filter.c_str(),
                                             _options.c_str(),
                                             _rows.out());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<rows, int>
    find_one_and_delete(
        string_arg _database,
        string_arg _collection,
        string_arg _filter,
        string_arg _options
    ) D_NOEXCEPT
    {
        rows      value{};
        const int result = d_mongodb_find_one_and_delete(m_db,
                                                         _database.c_str(),
                                                         _collection.c_str(),
                                                         _filter.c_str(),
                                                         _options.c_str(),
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
    bulk_write(
        string_arg               _database,
        string_arg               _collection,
        string_arg               _operations,
        unsigned                 _flags,
        string_arg               _options,
        dj_mongodb_write_counts& _counts
    ) D_NOEXCEPT
    {
        return d_mongodb_bulk_write(m_db,
                                    _database.c_str(),
                                    _collection.c_str(),
                                    _operations.c_str(),
                                    _flags,
                                    _options.c_str(),
                                    &_counts);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<dj_mongodb_write_counts, int>
    bulk_write(
        string_arg _database,
        string_arg _collection,
        string_arg _operations,
        unsigned   _flags,
        string_arg _options
    ) D_NOEXCEPT
    {
        dj_mongodb_write_counts value{};
        const int               result = d_mongodb_bulk_write(
            m_db,
            _database.c_str(),
            _collection.c_str(),
            _operations.c_str(),
            _flags,
            _options.c_str(),
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
    client_bulk_write(
        string_arg               _operations,
        unsigned                 _flags,
        string_arg               _options,
        dj_mongodb_write_counts& _counts
    ) D_NOEXCEPT
    {
        return d_mongodb_client_bulk_write(m_db,
                                           _operations.c_str(),
                                           _flags,
                                           _options.c_str(),
                                           &_counts);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<dj_mongodb_write_counts, int>
    client_bulk_write(
        string_arg _operations,
        unsigned   _flags,
        string_arg _options
    ) D_NOEXCEPT
    {
        dj_mongodb_write_counts value{};
        const int               result = d_mongodb_client_bulk_write(
            m_db,
            _operations.c_str(),
            _flags,
            _options.c_str(),
            &value);

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    // 1.3.1.7
    // Cursors and change streams
    //   mongodb_cursor.h's openings, into a face::mongodb_cursor or a
    // face::mongodb_change_stream, whose members its other functions are.
    int
    find_cursor(
        string_arg      _database,
        string_arg      _collection,
        string_arg      _filter,
        string_arg      _options,
        mongodb_cursor& _cursor
    ) D_NOEXCEPT
    {
        return d_mongodb_find_cursor(m_db,
                                     _database.c_str(),
                                     _collection.c_str(),
                                     _filter.c_str(),
                                     _options.c_str(),
                                     _cursor.out());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<mongodb_cursor, int>
    find_cursor(
        string_arg _database,
        string_arg _collection,
        string_arg _filter,
        string_arg _options
    ) D_NOEXCEPT
    {
        mongodb_cursor value{};
        const int      result = d_mongodb_find_cursor(m_db,
                                                      _database.c_str(),
                                                      _collection.c_str(),
                                                      _filter.c_str(),
                                                      _options.c_str(),
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
    aggregate_cursor(
        string_arg      _database,
        string_arg      _collection,
        string_arg      _pipeline,
        string_arg      _options,
        mongodb_cursor& _cursor
    ) D_NOEXCEPT
    {
        return d_mongodb_aggregate_cursor(m_db,
                                          _database.c_str(),
                                          _collection.c_str(),
                                          _pipeline.c_str(),
                                          _options.c_str(),
                                          _cursor.out());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<mongodb_cursor, int>
    aggregate_cursor(
        string_arg _database,
        string_arg _collection,
        string_arg _pipeline,
        string_arg _options
    ) D_NOEXCEPT
    {
        mongodb_cursor value{};
        const int      result = d_mongodb_aggregate_cursor(m_db,
                                                           _database.c_str(),
                                                           _collection.c_str(),
                                                           _pipeline.c_str(),
                                                           _options.c_str(),
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
    command_cursor(
        string_arg      _database,
        string_arg      _command,
        mongodb_cursor& _cursor
    ) D_NOEXCEPT
    {
        return d_mongodb_command_cursor(m_db,
                                        _database.c_str(),
                                        _command.c_str(),
                                        _cursor.out());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<mongodb_cursor, int>
    command_cursor(
        string_arg _database,
        string_arg _command
    ) D_NOEXCEPT
    {
        mongodb_cursor value{};
        const int      result = d_mongodb_command_cursor(m_db,
                                                         _database.c_str(),
                                                         _command.c_str(),
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
    watch_collection(
        string_arg             _database,
        string_arg             _collection,
        string_arg             _pipeline,
        string_arg             _options,
        mongodb_change_stream& _stream
    ) D_NOEXCEPT
    {
        return d_mongodb_watch_collection(m_db,
                                          _database.c_str(),
                                          _collection.c_str(),
                                          _pipeline.c_str(),
                                          _options.c_str(),
                                          _stream.out());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<mongodb_change_stream, int>
    watch_collection(
        string_arg _database,
        string_arg _collection,
        string_arg _pipeline,
        string_arg _options
    ) D_NOEXCEPT
    {
        mongodb_change_stream value{};
        const int             result = d_mongodb_watch_collection(
            m_db,
            _database.c_str(),
            _collection.c_str(),
            _pipeline.c_str(),
            _options.c_str(),
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
    watch_database(
        string_arg             _database,
        string_arg             _pipeline,
        string_arg             _options,
        mongodb_change_stream& _stream
    ) D_NOEXCEPT
    {
        return d_mongodb_watch_database(m_db,
                                        _database.c_str(),
                                        _pipeline.c_str(),
                                        _options.c_str(),
                                        _stream.out());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<mongodb_change_stream, int>
    watch_database(
        string_arg _database,
        string_arg _pipeline,
        string_arg _options
    ) D_NOEXCEPT
    {
        mongodb_change_stream value{};
        const int             result = d_mongodb_watch_database(
            m_db,
            _database.c_str(),
            _pipeline.c_str(),
            _options.c_str(),
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
    watch_deployment(
        string_arg             _pipeline,
        string_arg             _options,
        mongodb_change_stream& _stream
    ) D_NOEXCEPT
    {
        return d_mongodb_watch_deployment(m_db,
                                          _pipeline.c_str(),
                                          _options.c_str(),
                                          _stream.out());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<mongodb_change_stream, int>
    watch_deployment(
        string_arg _pipeline,
        string_arg _options
    ) D_NOEXCEPT
    {
        mongodb_change_stream value{};
        const int             result = d_mongodb_watch_deployment(
            m_db,
            _pipeline.c_str(),
            _options.c_str(),
            value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    // 1.3.1.8
    // Databases
    //   mongodb_database.h.
    D_NODISCARD bool
    has_database(
        string_arg _name
    ) const D_NOEXCEPT
    {
        return d_mongodb_has_database(m_db, _name.c_str());
    }

    int
    has_database_s(
        string_arg _name,
        bool&      _exists
    ) const D_NOEXCEPT
    {
        return d_mongodb_has_database_s(m_db, _name.c_str(), &_exists);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<bool, int>
    has_database_s(
        string_arg _name
    ) const D_NOEXCEPT
    {
        bool      value{};
        const int result = d_mongodb_has_database_s(m_db,
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
    drop_database(
        string_arg _name,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        return d_mongodb_drop_database(m_db, _name.c_str(), _flags);
    }

    int
    list_databases(
        name_list& _names
    ) const D_NOEXCEPT
    {
        return d_mongodb_list_databases(m_db, _names.out());
    }

    int
    list_databases(
        std::vector<std::string>& _names
    ) const
    {
        name_list names;
        const int result = d_mongodb_list_databases(m_db, names.out());

        names.to_vector(_names);

        return result;
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<name_list, int>
    list_databases() const D_NOEXCEPT
    {
        name_list value{};
        const int result = d_mongodb_list_databases(m_db, value.out());

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
        return d_mongodb_use_database(m_db, _name.c_str());
    }

    int
    current_database(
        text& _name
    ) const D_NOEXCEPT
    {
        return d_mongodb_current_database(m_db, _name.out());
    }

    int
    current_database(
        std::string& _name
    ) const
    {
        text      text_value;
        const int result = d_mongodb_current_database(m_db, text_value.out());

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
        const int result = d_mongodb_current_database(m_db, text_value.out());

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
        const int result = d_mongodb_current_database(m_db, value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    // 1.3.1.9
    // Collections, views and indexes
    //   mongodb_schema.h.
    D_NODISCARD bool
    has_collection(
        string_arg _database,
        string_arg _name
    ) const D_NOEXCEPT
    {
        return d_mongodb_has_collection(m_db, _database.c_str(), _name.c_str());
    }

    int
    has_collection_s(
        string_arg _database,
        string_arg _name,
        bool&      _exists
    ) const D_NOEXCEPT
    {
        return d_mongodb_has_collection_s(m_db,
                                          _database.c_str(),
                                          _name.c_str(),
                                          &_exists);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<bool, int>
    has_collection_s(
        string_arg _database,
        string_arg _name
    ) const D_NOEXCEPT
    {
        bool      value{};
        const int result = d_mongodb_has_collection_s(m_db,
                                                      _database.c_str(),
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
    create_collection(
        string_arg _database,
        string_arg _name,
        string_arg _options,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        return d_mongodb_create_collection(m_db,
                                           _database.c_str(),
                                           _name.c_str(),
                                           _options.c_str(),
                                           _flags);
    }

    int
    drop_collection(
        string_arg _database,
        string_arg _name,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        return d_mongodb_drop_collection(m_db,
                                         _database.c_str(),
                                         _name.c_str(),
                                         _flags);
    }

    int
    rename_collection(
        string_arg _database,
        string_arg _from,
        string_arg _to_database,
        string_arg _to,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        return d_mongodb_rename_collection(m_db,
                                           _database.c_str(),
                                           _from.c_str(),
                                           _to_database.c_str(),
                                           _to.c_str(),
                                           _flags);
    }

    int
    list_collections(
        string_arg _database,
        name_list& _names
    ) const D_NOEXCEPT
    {
        return d_mongodb_list_collections(m_db,
                                          _database.c_str(),
                                          _names.out());
    }

    int
    list_collections(
        string_arg                _database,
        std::vector<std::string>& _names
    ) const
    {
        name_list names;
        const int result = d_mongodb_list_collections(m_db,
                                                      _database.c_str(),
                                                      names.out());

        names.to_vector(_names);

        return result;
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<name_list, int>
    list_collections(
        string_arg _database
    ) const D_NOEXCEPT
    {
        name_list value{};
        const int result = d_mongodb_list_collections(m_db,
                                                      _database.c_str(),
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
    collection_info(
        string_arg _database,
        string_arg _name,
        text&      _info
    ) const D_NOEXCEPT
    {
        return d_mongodb_collection_info(m_db,
                                         _database.c_str(),
                                         _name.c_str(),
                                         _info.out());
    }

    int
    collection_info(
        string_arg   _database,
        string_arg   _name,
        std::string& _info
    ) const
    {
        text      text_value;
        const int result = d_mongodb_collection_info(m_db,
                                                     _database.c_str(),
                                                     _name.c_str(),
                                                     text_value.out());

        _info = text_value.str();

        return result;
    }

#if D_INTERNAL_DB_FACE_OPTIONAL
    int
    collection_info(
        string_arg                  _database,
        string_arg                  _name,
        std::optional<std::string>& _info
    ) const
    {
        text      text_value;
        const int result = d_mongodb_collection_info(m_db,
                                                     _database.c_str(),
                                                     _name.c_str(),
                                                     text_value.out());

        // the text; nothing for SQL NULL, and for a failure
        if (text_value.is_null())
        {
            _info.reset();
        }
        else
        {
            _info = text_value.str();
        }

        return result;
    }
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<text, int>
    collection_info(
        string_arg _database,
        string_arg _name
    ) const D_NOEXCEPT
    {
        text      value{};
        const int result = d_mongodb_collection_info(m_db,
                                                     _database.c_str(),
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

    D_NODISCARD bool
    has_view(
        string_arg _database,
        string_arg _name
    ) const D_NOEXCEPT
    {
        return d_mongodb_has_view(m_db, _database.c_str(), _name.c_str());
    }

    int
    has_view_s(
        string_arg _database,
        string_arg _name,
        bool&      _exists
    ) const D_NOEXCEPT
    {
        return d_mongodb_has_view_s(m_db,
                                    _database.c_str(),
                                    _name.c_str(),
                                    &_exists);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<bool, int>
    has_view_s(
        string_arg _database,
        string_arg _name
    ) const D_NOEXCEPT
    {
        bool      value{};
        const int result = d_mongodb_has_view_s(m_db,
                                                _database.c_str(),
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
    create_view(
        string_arg _database,
        string_arg _name,
        string_arg _source,
        string_arg _pipeline,
        string_arg _options,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        return d_mongodb_create_view(m_db,
                                     _database.c_str(),
                                     _name.c_str(),
                                     _source.c_str(),
                                     _pipeline.c_str(),
                                     _options.c_str(),
                                     _flags);
    }

    int
    drop_view(
        string_arg _database,
        string_arg _name,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        return d_mongodb_drop_view(m_db,
                                   _database.c_str(),
                                   _name.c_str(),
                                   _flags);
    }

    int
    list_views(
        string_arg _database,
        name_list& _names
    ) const D_NOEXCEPT
    {
        return d_mongodb_list_views(m_db, _database.c_str(), _names.out());
    }

    int
    list_views(
        string_arg                _database,
        std::vector<std::string>& _names
    ) const
    {
        name_list names;
        const int result = d_mongodb_list_views(m_db,
                                                _database.c_str(),
                                                names.out());

        names.to_vector(_names);

        return result;
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<name_list, int>
    list_views(
        string_arg _database
    ) const D_NOEXCEPT
    {
        name_list value{};
        const int result = d_mongodb_list_views(m_db,
                                                _database.c_str(),
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
        string_arg _database,
        string_arg _collection,
        string_arg _name
    ) const D_NOEXCEPT
    {
        return d_mongodb_has_index(m_db,
                                   _database.c_str(),
                                   _collection.c_str(),
                                   _name.c_str());
    }

    int
    has_index_s(
        string_arg _database,
        string_arg _collection,
        string_arg _name,
        bool&      _exists
    ) const D_NOEXCEPT
    {
        return d_mongodb_has_index_s(m_db,
                                     _database.c_str(),
                                     _collection.c_str(),
                                     _name.c_str(),
                                     &_exists);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<bool, int>
    has_index_s(
        string_arg _database,
        string_arg _collection,
        string_arg _name
    ) const D_NOEXCEPT
    {
        bool      value{};
        const int result = d_mongodb_has_index_s(m_db,
                                                 _database.c_str(),
                                                 _collection.c_str(),
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
    create_index(
        string_arg _database,
        string_arg _collection,
        string_arg _name,
        string_arg _keys,
        string_arg _options,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        return d_mongodb_create_index(m_db,
                                      _database.c_str(),
                                      _collection.c_str(),
                                      _name.c_str(),
                                      _keys.c_str(),
                                      _options.c_str(),
                                      _flags);
    }

    int
    drop_index(
        string_arg _database,
        string_arg _collection,
        string_arg _name,
        unsigned   _flags
    ) D_NOEXCEPT
    {
        return d_mongodb_drop_index(m_db,
                                    _database.c_str(),
                                    _collection.c_str(),
                                    _name.c_str(),
                                    _flags);
    }

    int
    list_indexes(
        string_arg _database,
        string_arg _collection,
        name_list& _names
    ) const D_NOEXCEPT
    {
        return d_mongodb_list_indexes(m_db,
                                      _database.c_str(),
                                      _collection.c_str(),
                                      _names.out());
    }

    int
    list_indexes(
        string_arg                _database,
        string_arg                _collection,
        std::vector<std::string>& _names
    ) const
    {
        name_list names;
        const int result = d_mongodb_list_indexes(m_db,
                                                  _database.c_str(),
                                                  _collection.c_str(),
                                                  names.out());

        names.to_vector(_names);

        return result;
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<name_list, int>
    list_indexes(
        string_arg _database,
        string_arg _collection
    ) const D_NOEXCEPT
    {
        name_list value{};
        const int result = d_mongodb_list_indexes(m_db,
                                                  _database.c_str(),
                                                  _collection.c_str(),
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
    describe_indexes(
        string_arg _database,
        string_arg _collection,
        rows&      _rows
    ) const D_NOEXCEPT
    {
        return d_mongodb_describe_indexes(m_db,
                                          _database.c_str(),
                                          _collection.c_str(),
                                          _rows.out());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<rows, int>
    describe_indexes(
        string_arg _database,
        string_arg _collection
    ) const D_NOEXCEPT
    {
        rows      value{};
        const int result = d_mongodb_describe_indexes(m_db,
                                                      _database.c_str(),
                                                      _collection.c_str(),
                                                      value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    // 1.3.1.10
    // Transactions
    //   mongodb_transaction.h; face::transaction guards them.

    /**
     * @brief Begins a transaction with the options of a mode, as
     *        d_mongodb_begin_transaction_with does, for face::transaction.
     *
     * @param[in] _mode  the options, as JSON, or `NULL` for none: the
     *                   handle's concerns and preference.
     * @return d_mongodb_begin_transaction_with's result.
     */
    int
    begin_transaction(
        transaction_mode _mode
    ) D_NOEXCEPT
    {
        return d_mongodb_begin_transaction_with(m_db, _mode);
    }

    int
    begin_transaction() D_NOEXCEPT
    {
        return d_mongodb_begin_transaction(m_db);
    }

    int
    begin_transaction_with(
        string_arg _options
    ) D_NOEXCEPT
    {
        return d_mongodb_begin_transaction_with(m_db, _options.c_str());
    }

    int
    commit() D_NOEXCEPT
    {
        return d_mongodb_commit(m_db);
    }

    int
    rollback() D_NOEXCEPT
    {
        return d_mongodb_rollback(m_db);
    }

    D_NODISCARD bool
    in_transaction() const D_NOEXCEPT
    {
        return d_mongodb_in_transaction(m_db);
    }

    D_NODISCARD bool
    in_failed_transaction() const D_NOEXCEPT
    {
        return d_mongodb_in_failed_transaction(m_db);
    }

    // 1.3.1.11
    // Settings
    //   mongodb_settings.h.
    int
    set_read_concern(
        string_arg _concern
    ) D_NOEXCEPT
    {
        return d_mongodb_set_read_concern(m_db, _concern.c_str());
    }

    int
    get_read_concern(
        text& _concern
    ) const D_NOEXCEPT
    {
        return d_mongodb_get_read_concern(m_db, _concern.out());
    }

    int
    get_read_concern(
        std::string& _concern
    ) const
    {
        text      text_value;
        const int result = d_mongodb_get_read_concern(m_db, text_value.out());

        _concern = text_value.str();

        return result;
    }

#if D_INTERNAL_DB_FACE_OPTIONAL
    int
    get_read_concern(
        std::optional<std::string>& _concern
    ) const
    {
        text      text_value;
        const int result = d_mongodb_get_read_concern(m_db, text_value.out());

        // the text; nothing for SQL NULL, and for a failure
        if (text_value.is_null())
        {
            _concern.reset();
        }
        else
        {
            _concern = text_value.str();
        }

        return result;
    }
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<text, int>
    get_read_concern() const D_NOEXCEPT
    {
        text      value{};
        const int result = d_mongodb_get_read_concern(m_db, value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    set_write_concern(
        string_arg _concern
    ) D_NOEXCEPT
    {
        return d_mongodb_set_write_concern(m_db, _concern.c_str());
    }

    int
    get_write_concern(
        text& _concern
    ) const D_NOEXCEPT
    {
        return d_mongodb_get_write_concern(m_db, _concern.out());
    }

    int
    get_write_concern(
        std::string& _concern
    ) const
    {
        text      text_value;
        const int result = d_mongodb_get_write_concern(m_db, text_value.out());

        _concern = text_value.str();

        return result;
    }

#if D_INTERNAL_DB_FACE_OPTIONAL
    int
    get_write_concern(
        std::optional<std::string>& _concern
    ) const
    {
        text      text_value;
        const int result = d_mongodb_get_write_concern(m_db, text_value.out());

        // the text; nothing for SQL NULL, and for a failure
        if (text_value.is_null())
        {
            _concern.reset();
        }
        else
        {
            _concern = text_value.str();
        }

        return result;
    }
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<text, int>
    get_write_concern() const D_NOEXCEPT
    {
        text      value{};
        const int result = d_mongodb_get_write_concern(m_db, value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    set_read_preference(
        string_arg _preference
    ) D_NOEXCEPT
    {
        return d_mongodb_set_read_preference(m_db, _preference.c_str());
    }

    int
    get_read_preference(
        text& _preference
    ) const D_NOEXCEPT
    {
        return d_mongodb_get_read_preference(m_db, _preference.out());
    }

    int
    get_read_preference(
        std::string& _preference
    ) const
    {
        text      text_value;
        const int result = d_mongodb_get_read_preference(m_db,
                                                         text_value.out());

        _preference = text_value.str();

        return result;
    }

#if D_INTERNAL_DB_FACE_OPTIONAL
    int
    get_read_preference(
        std::optional<std::string>& _preference
    ) const
    {
        text      text_value;
        const int result = d_mongodb_get_read_preference(m_db,
                                                         text_value.out());

        // the text; nothing for SQL NULL, and for a failure
        if (text_value.is_null())
        {
            _preference.reset();
        }
        else
        {
            _preference = text_value.str();
        }

        return result;
    }
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<text, int>
    get_read_preference() const D_NOEXCEPT
    {
        text      value{};
        const int result = d_mongodb_get_read_preference(m_db, value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    int
    set_document_format(
        dj_mongodb_document_format _format
    ) D_NOEXCEPT
    {
        return d_mongodb_set_document_format(m_db, _format);
    }

    D_NODISCARD dj_mongodb_document_format
    get_document_format() const D_NOEXCEPT
    {
        return d_mongodb_get_document_format(m_db);
    }

    int
    set_max_time_ms(
        re_std::int64_t _milliseconds
    ) D_NOEXCEPT
    {
        return d_mongodb_set_max_time_ms(m_db, _milliseconds);
    }

    D_NODISCARD re_std::int64_t
    get_max_time_ms() const D_NOEXCEPT
    {
        return d_mongodb_get_max_time_ms(m_db);
    }

    // 1.3.1.12
    // GridFS
    //   mongodb_gridfs.h.
    int
    gridfs_upload(
        string_arg  _database,
        string_arg  _bucket,
        string_arg  _filename,
        const void* _data,
        std::size_t _size,
        string_arg  _metadata,
        text&       _id
    ) D_NOEXCEPT
    {
        return d_mongodb_gridfs_upload(m_db,
                                       _database.c_str(),
                                       _bucket.c_str(),
                                       _filename.c_str(),
                                       _data,
                                       _size,
                                       _metadata.c_str(),
                                       _id.out());
    }

    int
    gridfs_upload(
        string_arg   _database,
        string_arg   _bucket,
        string_arg   _filename,
        const void*  _data,
        std::size_t  _size,
        string_arg   _metadata,
        std::string& _id
    )
    {
        text      text_value;
        const int result = d_mongodb_gridfs_upload(m_db,
                                                   _database.c_str(),
                                                   _bucket.c_str(),
                                                   _filename.c_str(),
                                                   _data,
                                                   _size,
                                                   _metadata.c_str(),
                                                   text_value.out());

        _id = text_value.str();

        return result;
    }

#if D_INTERNAL_DB_FACE_OPTIONAL
    int
    gridfs_upload(
        string_arg                  _database,
        string_arg                  _bucket,
        string_arg                  _filename,
        const void*                 _data,
        std::size_t                 _size,
        string_arg                  _metadata,
        std::optional<std::string>& _id
    )
    {
        text      text_value;
        const int result = d_mongodb_gridfs_upload(m_db,
                                                   _database.c_str(),
                                                   _bucket.c_str(),
                                                   _filename.c_str(),
                                                   _data,
                                                   _size,
                                                   _metadata.c_str(),
                                                   text_value.out());

        // the text; nothing for SQL NULL, and for a failure
        if (text_value.is_null())
        {
            _id.reset();
        }
        else
        {
            _id = text_value.str();
        }

        return result;
    }
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<text, int>
    gridfs_upload(
        string_arg  _database,
        string_arg  _bucket,
        string_arg  _filename,
        const void* _data,
        std::size_t _size,
        string_arg  _metadata
    ) D_NOEXCEPT
    {
        text      value{};
        const int result = d_mongodb_gridfs_upload(m_db,
                                                   _database.c_str(),
                                                   _bucket.c_str(),
                                                   _filename.c_str(),
                                                   _data,
                                                   _size,
                                                   _metadata.c_str(),
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
    gridfs_upload_file(
        string_arg _database,
        string_arg _bucket,
        string_arg _filename,
        string_arg _path,
        string_arg _metadata,
        text&      _id
    ) D_NOEXCEPT
    {
        return d_mongodb_gridfs_upload_file(m_db,
                                            _database.c_str(),
                                            _bucket.c_str(),
                                            _filename.c_str(),
                                            _path.c_str(),
                                            _metadata.c_str(),
                                            _id.out());
    }

    int
    gridfs_upload_file(
        string_arg   _database,
        string_arg   _bucket,
        string_arg   _filename,
        string_arg   _path,
        string_arg   _metadata,
        std::string& _id
    )
    {
        text      text_value;
        const int result = d_mongodb_gridfs_upload_file(m_db,
                                                        _database.c_str(),
                                                        _bucket.c_str(),
                                                        _filename.c_str(),
                                                        _path.c_str(),
                                                        _metadata.c_str(),
                                                        text_value.out());

        _id = text_value.str();

        return result;
    }

#if D_INTERNAL_DB_FACE_OPTIONAL
    int
    gridfs_upload_file(
        string_arg                  _database,
        string_arg                  _bucket,
        string_arg                  _filename,
        string_arg                  _path,
        string_arg                  _metadata,
        std::optional<std::string>& _id
    )
    {
        text      text_value;
        const int result = d_mongodb_gridfs_upload_file(m_db,
                                                        _database.c_str(),
                                                        _bucket.c_str(),
                                                        _filename.c_str(),
                                                        _path.c_str(),
                                                        _metadata.c_str(),
                                                        text_value.out());

        // the text; nothing for SQL NULL, and for a failure
        if (text_value.is_null())
        {
            _id.reset();
        }
        else
        {
            _id = text_value.str();
        }

        return result;
    }
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<text, int>
    gridfs_upload_file(
        string_arg _database,
        string_arg _bucket,
        string_arg _filename,
        string_arg _path,
        string_arg _metadata
    ) D_NOEXCEPT
    {
        text      value{};
        const int result = d_mongodb_gridfs_upload_file(m_db,
                                                        _database.c_str(),
                                                        _bucket.c_str(),
                                                        _filename.c_str(),
                                                        _path.c_str(),
                                                        _metadata.c_str(),
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
    gridfs_download(
        string_arg   _database,
        string_arg   _bucket,
        string_arg   _id,
        void*&       _data,
        std::size_t& _size
    ) D_NOEXCEPT
    {
        return d_mongodb_gridfs_download(m_db,
                                         _database.c_str(),
                                         _bucket.c_str(),
                                         _id.c_str(),
                                         &_data,
                                         &_size);
    }

    int
    gridfs_download_file(
        string_arg _database,
        string_arg _bucket,
        string_arg _id,
        string_arg _path
    ) D_NOEXCEPT
    {
        return d_mongodb_gridfs_download_file(m_db,
                                              _database.c_str(),
                                              _bucket.c_str(),
                                              _id.c_str(),
                                              _path.c_str());
    }

    int
    gridfs_delete(
        string_arg _database,
        string_arg _bucket,
        string_arg _id
    ) D_NOEXCEPT
    {
        return d_mongodb_gridfs_delete(m_db,
                                       _database.c_str(),
                                       _bucket.c_str(),
                                       _id.c_str());
    }

    int
    gridfs_find_id(
        string_arg _database,
        string_arg _bucket,
        string_arg _filename,
        text&      _id
    ) D_NOEXCEPT
    {
        return d_mongodb_gridfs_find_id(m_db,
                                        _database.c_str(),
                                        _bucket.c_str(),
                                        _filename.c_str(),
                                        _id.out());
    }

    int
    gridfs_find_id(
        string_arg   _database,
        string_arg   _bucket,
        string_arg   _filename,
        std::string& _id
    )
    {
        text      text_value;
        const int result = d_mongodb_gridfs_find_id(m_db,
                                                    _database.c_str(),
                                                    _bucket.c_str(),
                                                    _filename.c_str(),
                                                    text_value.out());

        _id = text_value.str();

        return result;
    }

#if D_INTERNAL_DB_FACE_OPTIONAL
    int
    gridfs_find_id(
        string_arg                  _database,
        string_arg                  _bucket,
        string_arg                  _filename,
        std::optional<std::string>& _id
    )
    {
        text      text_value;
        const int result = d_mongodb_gridfs_find_id(m_db,
                                                    _database.c_str(),
                                                    _bucket.c_str(),
                                                    _filename.c_str(),
                                                    text_value.out());

        // the text; nothing for SQL NULL, and for a failure
        if (text_value.is_null())
        {
            _id.reset();
        }
        else
        {
            _id = text_value.str();
        }

        return result;
    }
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<text, int>
    gridfs_find_id(
        string_arg _database,
        string_arg _bucket,
        string_arg _filename
    ) D_NOEXCEPT
    {
        text      value{};
        const int result = d_mongodb_gridfs_find_id(m_db,
                                                    _database.c_str(),
                                                    _bucket.c_str(),
                                                    _filename.c_str(),
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
    gridfs_find(
        string_arg _database,
        string_arg _bucket,
        string_arg _filter,
        string_arg _options,
        rows&      _rows
    ) D_NOEXCEPT
    {
        return d_mongodb_gridfs_find(m_db,
                                     _database.c_str(),
                                     _bucket.c_str(),
                                     _filter.c_str(),
                                     _options.c_str(),
                                     _rows.out());
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<rows, int>
    gridfs_find(
        string_arg _database,
        string_arg _bucket,
        string_arg _filter,
        string_arg _options
    ) D_NOEXCEPT
    {
        rows      value{};
        const int result = d_mongodb_gridfs_find(m_db,
                                                 _database.c_str(),
                                                 _bucket.c_str(),
                                                 _filter.c_str(),
                                                 _options.c_str(),
                                                 value.out());

        // the value, or the layer's result when it failed
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return value;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    // 1.3.1.13
    // Maintenance
    //   mongodb_maintenance.h.
    int
    collection_size(
        string_arg       _database,
        string_arg       _collection,
        re_std::int64_t& _bytes
    ) const D_NOEXCEPT
    {
        return d_mongodb_collection_size(m_db,
                                         _database.c_str(),
                                         _collection.c_str(),
                                         &_bytes);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<re_std::int64_t, int>
    collection_size(
        string_arg _database,
        string_arg _collection
    ) const D_NOEXCEPT
    {
        re_std::int64_t value{};
        const int       result = d_mongodb_collection_size(m_db,
                                                           _database.c_str(),
                                                           _collection.c_str(),
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
        string_arg       _database,
        re_std::int64_t& _bytes
    ) const D_NOEXCEPT
    {
        return d_mongodb_database_size(m_db, _database.c_str(), &_bytes);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<re_std::int64_t, int>
    database_size(
        string_arg _database
    ) const D_NOEXCEPT
    {
        re_std::int64_t value{};
        const int       result = d_mongodb_database_size(m_db,
                                                         _database.c_str(),
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
    compact(
        string_arg _database,
        string_arg _collection
    ) D_NOEXCEPT
    {
        return d_mongodb_compact(m_db, _database.c_str(), _collection.c_str());
    }

    int
    validate(
        string_arg _database,
        string_arg _collection,
        unsigned   _flags,
        bool&      _valid,
        text&      _report
    ) const D_NOEXCEPT
    {
        return d_mongodb_validate(m_db,
                                  _database.c_str(),
                                  _collection.c_str(),
                                  _flags,
                                  &_valid,
                                  _report.out());
    }

    int
    validate(
        string_arg   _database,
        string_arg   _collection,
        unsigned     _flags,
        bool&        _valid,
        std::string& _report
    ) const
    {
        text      text_value;
        const int result = d_mongodb_validate(m_db,
                                              _database.c_str(),
                                              _collection.c_str(),
                                              _flags,
                                              &_valid,
                                              text_value.out());

        _report = text_value.str();

        return result;
    }

#if D_INTERNAL_DB_FACE_OPTIONAL
    int
    validate(
        string_arg                  _database,
        string_arg                  _collection,
        unsigned                    _flags,
        bool&                       _valid,
        std::optional<std::string>& _report
    ) const
    {
        text      text_value;
        const int result = d_mongodb_validate(m_db,
                                              _database.c_str(),
                                              _collection.c_str(),
                                              _flags,
                                              &_valid,
                                              text_value.out());

        // the text; nothing for SQL NULL, and for a failure
        if (text_value.is_null())
        {
            _report.reset();
        }
        else
        {
            _report = text_value.str();
        }

        return result;
    }
#endif  // D_INTERNAL_DB_FACE_OPTIONAL

private:
    D_DELETED_FN_MSG(mongodb(const mongodb&),
                     "a face owns its handle; move it")
    D_DELETED_FN_MSG(mongodb& operator=(const mongodb&),
                     "a face owns its handle; move it")

    // m_take -- a connection's result; the handle made replaces the face's
    // when it succeeded
    int
    m_take(
        int               _result,
        struct dj_mongodb* _made
    ) D_NOEXCEPT
    {
        // a failed connection made nothing to keep
        if (_result == 0)
        {
            reset(_made);
        }

        return _result;
    }

    struct dj_mongodb* m_db;
};

// 1.4    Swapping
//------------------------------------------------------------------------------
// swapping -- the two handles, cursors or streams trade places, as std::swap
// would have them, for code that swaps unqualified
inline void
swap(
    mongodb_cursor& _left,
    mongodb_cursor& _right
) D_NOEXCEPT
{
    _left.swap(_right);

    return;
}

inline void
swap(
    mongodb_change_stream& _left,
    mongodb_change_stream& _right
) D_NOEXCEPT
{
    _left.swap(_right);

    return;
}

inline void
swap(
    mongodb& _left,
    mongodb& _right
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
// the face is its handle, and a cursor and a stream theirs, and from C++11
// each moves without throwing (db_face.hpp, 6)
D_STATIC_ASSERT(sizeof(mongodb_cursor) == sizeof(struct dj_mongodb_cursor*),
                "face::mongodb_cursor is its handle, and no more");
D_STATIC_ASSERT(sizeof(mongodb_change_stream) ==
                    sizeof(struct dj_mongodb_change_stream*),
                "face::mongodb_change_stream is its handle, and no more");
D_STATIC_ASSERT(sizeof(mongodb) == sizeof(struct dj_mongodb*),
                "face::mongodb is its handle, and no more");
#if D_MOVE_ENABLED
D_STATIC_ASSERT(
    ( (std::is_nothrow_move_constructible<mongodb_cursor>::value) &&
      (std::is_nothrow_move_assignable<mongodb_cursor>::value) ),
    "face::mongodb_cursor moves without throwing");
D_STATIC_ASSERT(
    ( (std::is_nothrow_move_constructible<mongodb_change_stream>::value) &&
      (std::is_nothrow_move_assignable<mongodb_change_stream>::value) ),
    "face::mongodb_change_stream moves without throwing");
D_STATIC_ASSERT( ( (std::is_nothrow_move_constructible<mongodb>::value) &&
                   (std::is_nothrow_move_assignable<mongodb>::value) ),
                 "face::mongodb moves without throwing");
#endif  // D_MOVE_ENABLED


}  // namespace face
}  // namespace djinterp


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_DB_MONGODB_MONGODB_FACE_HPP
