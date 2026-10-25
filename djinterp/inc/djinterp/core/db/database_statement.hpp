/*******************************************************************************
* djinterp [core]                                         database_statement.hpp
*
* Statements and result sets over the C layers, once for every vendor.
*   buffered_result_set is database.hpp's result_set over the rows a C layer
* returned (face::rows): every row read when the query ran, held in one
* allocation, read through a cursor in any direction, and usable after the
* connection that made it is gone. Each value keeps the type the database
* gave it, and converts to another only where nothing is lost, as the C
* layers' conversions do.
*   streaming_result_set<Layer> is a result_set over a statement's run read a
* page at a time, as its cursor reaches the rows, rather than all at once: a
* page in memory, the cursor moving forward only. On MariaDB, MySQL and
* PostgreSQL the run holds the connection until it ends, and the session
* refuses everything else meanwhile, with the layer's own result for it.
*   prepared_statement<Layer> is database.hpp's statement over a face's
* prepared statement, for the layer Layer names. It belongs to its
* connection's session, which closes it when the connection closes, so that
* no statement outlives the connection it runs on; a closed statement then
* refuses to run, with query_exception. A failure throws, as the connection's
* do, with the layer's result.
*   A layer is described to the templates by a class of its own -- sqlite_layer
* in sqlite.hpp, mariadb_layer in mysql_common.hpp, postgres_layer in
* postgres.hpp -- that names:
*     statement_face    the face's statement class
*     handle_type       the connection handle a statement belongs to, as
*                       statement_face::statement_connection() returns it
*     utc_offset        what follows a point in time bound as text: "+00:00"
*                       where the database reads an offset, "" where it reads
*                       a UTC date and time as it is
*     connection_lost   static, noexcept: whether a result means the database
*                       dropped the connection
*     raise             static, never returning: throws the exception for a
*                       failed call, given the handle, the failure_kind, the
*                       operation's name and the result -- connection_exception
*                       for a lost connection, which it may learn of from the
*                       handle where the result alone cannot say
*     holds_connection  static constexpr bool: whether a statement's run,
*                       read a page at a time, holds the connection until it
*                       ends -- false for SQLite, which steps a statement while
*                       the connection runs others
*     busy_code         static constexpr int, where holds_connection: the
*                       layer's result for a call the run would refuse --
*                       2014 for MariaDB and MySQL, 08000 packed for
*                       PostgreSQL
*   Neither class is thread-safe: a connection, its statements and its result
* sets belong to one thread at a time, as each client's own do.
*
*
* path:      /inc/djinterp/core/db/database_statement.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FAILURES
    --------
    1.  Kinds
         1.  failure_kind
2.  SESSIONS
    --------
    1.  The session
         1.  internal::session
3.  RESULT SETS
    -----------
    1.  Buffered result sets
         1.  buffered_result_set
    2.  Streaming result sets
         1.  streaming_result_set
4.  STATEMENTS
    ----------
    1.  Prepared statements
         1.  prepared_statement
    2.  Definitions
    3.  Streams' definitions
*/

#ifndef DJINTERP_DB_DATABASE_STATEMENT_HPP
#define DJINTERP_DB_DATABASE_STATEMENT_HPP 1

// djinterp
#include "../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: the floor

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>      // std::size_t
#include <memory>       // std::shared_ptr, std::unique_ptr
#include <optional>     // std::optional
#include <string>       // std::string
#include <type_traits>  // std::is_same_v, std::decay_t
#include <utility>      // std::declval, std::move
#include <variant>      // std::visit
#include <vector>       // std::vector
// djinterp
#include "../../djinterp.hpp"       // framework root
#include "../../c/db/db_face.hpp"   // face::rows, face::string_arg, d_db_*
#include "./database.hpp"           // value, result_set, statement, the
                                    // exceptions
// re_std
#include "../../../re_std/cstdint/cstdint.hpp"  // re_std::int32_t, int64_t,
                                                // uint8_t


NS_DJINTERP


//==============================================================================
// 1.  FAILURES
//==============================================================================


// 1.1    Kinds
//------------------------------------------------------------------------------
// 1.1.1
// failure_kind
//   enum: which exception a failed call raises, unless its result means the
// connection was lost, which always raises connection_exception.
enum class failure_kind
{
    connection,   // connection_exception: opening, closing, settings
    query,        // query_exception: preparing, binding, running
    transaction   // transaction_exception: transactions and savepoints
};


//==============================================================================
// 2.  SESSIONS
//==============================================================================


// 2.1    The session
//------------------------------------------------------------------------------
// 2.1.1
// internal::session
//   class: what a connection shares with the statements prepared on it: the
// statements still open, which close_all() closes before the connection
// closes; the rows the latest statement changed; whether a statement found
// the connection lost; and the stream whose run holds the connection, while
// one does, with the result the layer would give anything else meanwhile.
// Held by std::shared_ptr, so that a statement that outlives its connection
// still has a session to leave. Not copyable.
NS_INTERNAL

    class session
    {
    public:
        // closer
        //   type: finalizes one statement, given the address it enrolled.
        using closer = void (*)(void*) noexcept;

        session() noexcept;

        session(const session&)            = delete;
        session& operator=(const session&) = delete;

        /**
         * @brief Enrolls a statement, to be closed by close_all().
         *
         * @param[in] _statement  the statement's address, the closer's
         *                        argument.
         * @param[in] _close      finalizes it.
         * @throws std::bad_alloc when the session cannot grow.
         */
        void enroll(void*  _statement,
                    closer _close);

        // withdrawing, closing -- withdraw forgets one statement, which
        // closes itself; close_all closes every statement enrolled and forgets
        // them all, as the connection closes
        void withdraw(void* _statement) noexcept;
        void close_all() noexcept;

        // state -- the rows the latest statement changed, as each run
        // records them; whether a statement found the connection lost; and
        // starting afresh for a new connection
        void            record(re_std::int64_t _changes) noexcept;
        re_std::int64_t changes() const noexcept;
        void            mark_lost() noexcept;
        bool            lost() const noexcept;
        void            renew() noexcept;

        // holding -- hold names the stream whose run holds the connection,
        // and the result the layer gives any other call meanwhile; release
        // forgets it, when it is that stream; holder answers it, NULL while
        // the connection is free, and busy_code its result. close_all and
        // renew free the connection too.
        void  hold(const void* _stream,
                   int         _busy_code) noexcept;
        void  release(const void* _stream) noexcept;
        const void*
              holder() const noexcept;
        int   busy_code() const noexcept;

    private:
        // entry
        //   struct: one statement enrolled.
        struct entry
        {
            void*  statement;
            closer close;
        };

        std::vector<entry> m_entries;
        re_std::int64_t    m_changes;
        bool               m_lost;
        const void*        m_holder;
        int                m_busy_code;
    };

NS_END  // internal


//==============================================================================
// 3.  RESULT SETS
//==============================================================================


// 3.1    Buffered result sets
//------------------------------------------------------------------------------
// 3.1.1
// buffered_result_set
//   class: the rows a query returned, all of them, read through a cursor. The
// cursor starts before the first row (current_row() -1) and stops after the
// last (current_row() == row_count()); next, previous, first, last and
// absolute move it and answer whether it is on a row, absolute counting from
// 0 and, for a negative row, back from the end, -1 being the last.
//   A value keeps the type the database gave it: NULL; an integer, as a
// boolean in a boolean column, as re_std::int32_t in an integer column whose
// value fits, otherwise re_std::int64_t; a real number; text; bytes. get_bool,
// get_int, get_long and get_double read a value as their type where it is one
// or converts to it exactly, as db_common.h's conversions say, and throw
// type_conversion_exception, D_DB_MISMATCH, where it does not; get_string
// reads text, bytes, and a number written as value_to_string writes it;
// get_binary reads bytes, and text as its bytes. Each answers std::nullopt
// for NULL. A value read with no current row, a column index past the last
// and a name no column has throw std::out_of_range. Not copyable; movable.
class buffered_result_set : public result_set<buffered_result_set>
{
public:
    // construction -- empty, or holding the rows a layer filled
    buffered_result_set() noexcept;
    explicit buffered_result_set(face::rows&& _rows) noexcept;

    ~buffered_result_set() = default;

    buffered_result_set(buffered_result_set&& _other) noexcept;
    buffered_result_set& operator=(buffered_result_set&& _other) noexcept;

    // navigation -- each moves the cursor and answers whether it is on a row
    bool next() noexcept;
    bool previous() noexcept;
    bool first() noexcept;
    bool last() noexcept;
    bool absolute(re_std::int64_t _row) noexcept;

    // metadata -- column_name, column_type and column_index throw
    // std::out_of_range for a column that is not there; column_index finds a
    // name exactly, or failing that without ASCII case
    std::size_t     column_count() const noexcept;
    std::string     column_name(std::size_t _index) const;
    field_type      column_type(std::size_t _index) const;
    std::size_t     column_index(const std::string& _name) const;
    re_std::int64_t row_count() const noexcept;
    re_std::int64_t current_row() const noexcept;

    // values by column index -- the current row's, as the class describes
    value                          get_value(std::size_t _index) const;
    bool                           is_null(std::size_t _index) const;
    std::optional<bool>            get_bool(std::size_t _index) const;
    std::optional<re_std::int32_t> get_int(std::size_t _index) const;
    std::optional<re_std::int64_t> get_long(std::size_t _index) const;
    std::optional<double>          get_double(std::size_t _index) const;
    std::optional<std::string>     get_string(std::size_t _index) const;
    std::optional<std::vector<re_std::uint8_t>>
                                   get_binary(std::size_t _index) const;

    // values by column name -- column_index(name), then as by index
    value                          get_value(const std::string& _name) const;
    bool                           is_null(const std::string& _name) const;
    std::optional<bool>            get_bool(const std::string& _name) const;
    std::optional<re_std::int32_t> get_int(const std::string& _name) const;
    std::optional<re_std::int64_t> get_long(const std::string& _name) const;
    std::optional<double>          get_double(const std::string& _name) const;
    std::optional<std::string>     get_string(const std::string& _name) const;
    std::optional<std::vector<re_std::uint8_t>>
                                   get_binary(const std::string& _name) const;

    /**
     * @brief Reads the current row as values by column name.
     *
     * @return the row; where two columns share a name, the first's value.
     * @throws std::out_of_range when the cursor is on no row.
     */
    row get_current_row() const;

    /**
     * @brief Reads the rows after the cursor, as get_current_row reads each,
     *        and moves the cursor after the last: every row, for a result set
     *        no one has moved through.
     *
     * @return the rows, in order.
     */
    result_rows get_all_rows();

    /**
     * @brief Releases the rows.
     *
     * @post The result set is empty, its cursor before the first row; every
     *       reference obtained from rows() is invalid.
     */
    void close() noexcept;

    // rows -- the C rows themselves, for reading values exactly as the
    // layer holds them (face::rows, db_common.h's d_db_value)
    const face::rows& rows() const noexcept;

private:
    // m_require_row, m_value -- std::out_of_range unless the cursor is on a
    // row; and the current row's value in a column, or std::out_of_range
    void                     m_require_row(const char* _caller) const;
    const struct d_db_value& m_value(std::size_t _index,
                                     const char* _caller) const;

    face::rows      m_rows;
    re_std::int64_t m_cursor;
};

// 3.2    Streaming result sets
//------------------------------------------------------------------------------
// prepared_statement, declared for the streams that run over it (4.1.1)
template<typename Layer>
class prepared_statement;

// stream_page_size
//   constant: the rows a stream reads a page at a time where a program names
// no number (prepared_statement::execute_stream; a connection's
// stream_query).
inline constexpr std::size_t stream_page_size = 256u;

// 3.2.1
// streaming_result_set
//   class template: the rows of a statement's run, read a page at a time as
// the cursor reaches them (prepared_statement::execute_stream; a
// connection's stream_query), one page held. The cursor moves forward: next()
// moves one row on, reading the next page when the one held is spent; last()
// and absolute() to a later row read ahead, keeping the page they land in;
// previous(), and first() or absolute() to a row before the cursor, throw
// query_exception, D_DB_UNSUPPORTED -- first() excepted while the first page
// is the one held, and a negative absolute(), which reads to the end and
// counts back, where the row is in the last page. row_count() is -1 until
// the last page has been read; current_row() counts from 0 across pages.
// Values, the columns and get_current_row() read the current page as
// buffered_result_set reads its rows; the columns come with every page, the
// last too, though it be empty.
//   On MariaDB, MySQL and PostgreSQL the run holds the connection until it
// ends (Layer::holds_connection): every other operation of the connection,
// and every other statement's run, throws query_exception with
// Layer::busy_code without calling the layer. On every vendor the
// statement's own values cannot change meanwhile: binding or clearing them
// throws query_exception, D_DB_INVALID_ARGUMENT. The run ends with its last
// page -- one shorter than the page size --, a failure, close() or the
// destructor; running the statement again, closing it, or closing the
// connection ends it too, and closes the stream, whose next move throws
// query_exception. A failure reading a page throws, with the layer's result,
// and ends the run. Not copyable or movable: its statement knows it by its
// address.
template<typename Layer>
class streaming_result_set : public result_set<streaming_result_set<Layer>>
{
public:
    using layer_type = Layer;

    /**
     * @brief Ends the run, unless it has ended, and releases the page.
     */
    ~streaming_result_set();

    streaming_result_set(const streaming_result_set&)            = delete;
    streaming_result_set& operator=(const streaming_result_set&) = delete;
    streaming_result_set(streaming_result_set&&)                 = delete;
    streaming_result_set& operator=(streaming_result_set&&)      = delete;

    // navigation -- each answers whether the cursor is on a row; each throws
    // query_exception on a closed stream, and as the class describes for a
    // move backward or a page that fails
    bool next();
    bool previous();
    bool first();
    bool last();
    bool absolute(re_std::int64_t _row);

    // metadata -- as buffered_result_set's, of the page held; row_count -1
    // until the last page is read, then every row the run gave; current_row
    // from 0 across pages, -1 before the first row and row_count() after the
    // last
    std::size_t     column_count() const noexcept;
    std::string     column_name(std::size_t _index) const;
    field_type      column_type(std::size_t _index) const;
    std::size_t     column_index(const std::string& _name) const;
    re_std::int64_t row_count() const noexcept;
    re_std::int64_t current_row() const noexcept;

    // values by column index and by name -- the current row's, as
    // buffered_result_set reads them
    value                          get_value(std::size_t _index) const;
    bool                           is_null(std::size_t _index) const;
    std::optional<bool>            get_bool(std::size_t _index) const;
    std::optional<re_std::int32_t> get_int(std::size_t _index) const;
    std::optional<re_std::int64_t> get_long(std::size_t _index) const;
    std::optional<double>          get_double(std::size_t _index) const;
    std::optional<std::string>     get_string(std::size_t _index) const;
    std::optional<std::vector<re_std::uint8_t>>
                                   get_binary(std::size_t _index) const;
    value                          get_value(const std::string& _name) const;
    bool                           is_null(const std::string& _name) const;
    std::optional<bool>            get_bool(const std::string& _name) const;
    std::optional<re_std::int32_t> get_int(const std::string& _name) const;
    std::optional<re_std::int64_t> get_long(const std::string& _name) const;
    std::optional<double>          get_double(const std::string& _name) const;
    std::optional<std::string>     get_string(const std::string& _name) const;
    std::optional<std::vector<re_std::uint8_t>>
                                   get_binary(const std::string& _name) const;

    // rows -- the current row by column name, as buffered_result_set's; and
    // every row after the cursor, read to the run's end, the cursor after
    // the last
    row         get_current_row() const;
    result_rows get_all_rows();

    /**
     * @brief Ends the run, reading away whatever rows it had left, and
     *        releases the page.
     *
     * @post The stream is closed: a move throws query_exception, and the
     *       connection is free of it.
     */
    void close() noexcept;

    // state -- whether the stream is open, and whether its run has ended
    // (the last page read, or a failure); the page held, for reading values
    // exactly as the layer holds them
    bool                       is_open() const noexcept;
    bool                       is_ended() const noexcept;
    const buffered_result_set& page() const noexcept;

private:
    friend class prepared_statement<Layer>;

    // made by the statement it reads, which has begun the run
    streaming_result_set(prepared_statement<Layer>& _statement,
                         std::size_t                _page_size) noexcept;

    // m_fetch -- the next page, the run ended with a short one; a failure
    // ends the run and throws, naming `_operation`
    buffered_result_set m_fetch(const char* _operation);

    // m_end_run -- the run over: the connection free of it
    void m_end_run() noexcept;

    // m_detach -- the statement ending the stream: closed, its page released
    void m_detach() noexcept;

    // m_require_open, m_backward -- query_exception for a closed stream, and
    // D_DB_UNSUPPORTED for a move to a row the stream no longer holds
    void              m_require_open(const char* _operation) const;
    [[noreturn]] void m_backward(const char* _operation) const;

    prepared_statement<Layer>*                 m_statement;
    std::unique_ptr<prepared_statement<Layer>> m_owned;
    buffered_result_set                        m_page;
    std::size_t                                m_page_size;
    re_std::int64_t                            m_base;
    bool                                       m_ended;
    bool                                       m_open;
};


//==============================================================================
// 4.  STATEMENTS
//==============================================================================


// 4.1    Prepared statements
//------------------------------------------------------------------------------
// 4.1.1
// prepared_statement
//   class template: a statement a connection prepared, for the layer Layer
// describes (the header's description says what it names). Parameters are
// numbered from 1, as each layer numbers them, and written as the database
// writes them: `?` for SQLite, MariaDB and MySQL, `$1` for PostgreSQL. A
// parameter never bound is NULL; a value stays bound until another is bound
// or clear_parameters() clears them, across runs. Each run starts from the
// beginning and leaves the statement ready for the next. Binding by name
// exists where the layer names parameters (SQLite: `:name`, `@name`,
// `$name`, the prefix included). Every member that runs or binds throws when
// the statement is closed, and, with the layer's result, when the call
// fails. Not copyable or movable: the session knows it by its address, and a
// connection hands it out as std::unique_ptr.
template<typename Layer>
class prepared_statement : public statement<prepared_statement<Layer>>
{
public:
    using layer_type     = Layer;
    using statement_face = typename Layer::statement_face;

    /**
     * @brief Takes a statement a face prepared, and enrolls it in its
     *        connection's session.
     *
     * @param[in] _session    the connection's session.
     * @param[in] _statement  the face's statement, prepared; taken.
     * @throws std::bad_alloc when the session cannot grow; the statement is
     *         then finalized.
     */
    prepared_statement(std::shared_ptr<internal::session> _session,
                       statement_face&&                   _statement);

    /**
     * @brief Finalizes the statement, unless the session closed it first, and
     *        leaves the session.
     */
    ~prepared_statement();

    prepared_statement(const prepared_statement&)            = delete;
    prepared_statement& operator=(const prepared_statement&) = delete;
    prepared_statement(prepared_statement&&)                 = delete;
    prepared_statement& operator=(prepared_statement&&)      = delete;

    // binding by index -- each throws query_exception for a parameter the
    // statement does not have, with the layer's result; bind_string also
    // for text that holds a NUL, D_DB_INVALID_ARGUMENT, which bind_binary
    // binds instead
    void bind_null(std::size_t _index);
    void bind_bool(std::size_t _index,
                   bool        _value);
    void bind_int(std::size_t     _index,
                  re_std::int32_t _value);
    void bind_long(std::size_t     _index,
                   re_std::int64_t _value);
    void bind_double(std::size_t _index,
                     double      _value);
    void bind_string(std::size_t        _index,
                     const std::string& _value);
    void bind_binary(std::size_t                         _index,
                     const std::vector<re_std::uint8_t>& _value);

    /**
     * @brief Binds a value of any of value's types: NULL, a boolean, an
     *        integer, a double, text, bytes, or a point in time, as text.
     *
     * @note A point in time is bound as value_to_string writes it, its UTC
     *       date and time, followed by the layer's utc_offset: "+00:00" for
     *       PostgreSQL, nothing for SQLite, MariaDB and MySQL. A MySQL or
     *       MariaDB TIMESTAMP column reads it in the session's time zone, so
     *       a program that stores points in time there sets time_zone to
     *       '+00:00' first.
     *
     * @param[in] _index  the parameter, from 1.
     * @param[in] _value  the value.
     * @throws as the bind member for the value's type throws.
     */
    void bind_value(std::size_t  _index,
                    const value& _value);

    // binding by name -- where the layer names parameters; each throws
    // query_exception, D_DB_NOT_FOUND, for a name no parameter has, and
    // otherwise as binding by index
    template<typename Face = statement_face,
             typename      = decltype(std::declval<const Face&>().
                                 parameter_index(
                                     std::declval<face::string_arg>(),
                                     std::declval<std::size_t&>()))>
    std::size_t parameter_index(const std::string& _name) const;

    template<typename Face = statement_face,
             typename      = decltype(std::declval<const Face&>().
                                 parameter_index(
                                     std::declval<face::string_arg>(),
                                     std::declval<std::size_t&>()))>
    void bind_null(const std::string& _name);

    template<typename Face = statement_face,
             typename      = decltype(std::declval<const Face&>().
                                 parameter_index(
                                     std::declval<face::string_arg>(),
                                     std::declval<std::size_t&>()))>
    void bind_bool(const std::string& _name,
                   bool               _value);

    template<typename Face = statement_face,
             typename      = decltype(std::declval<const Face&>().
                                 parameter_index(
                                     std::declval<face::string_arg>(),
                                     std::declval<std::size_t&>()))>
    void bind_int(const std::string& _name,
                  re_std::int32_t    _value);

    template<typename Face = statement_face,
             typename      = decltype(std::declval<const Face&>().
                                 parameter_index(
                                     std::declval<face::string_arg>(),
                                     std::declval<std::size_t&>()))>
    void bind_long(const std::string& _name,
                   re_std::int64_t    _value);

    template<typename Face = statement_face,
             typename      = decltype(std::declval<const Face&>().
                                 parameter_index(
                                     std::declval<face::string_arg>(),
                                     std::declval<std::size_t&>()))>
    void bind_double(const std::string& _name,
                     double             _value);

    template<typename Face = statement_face,
             typename      = decltype(std::declval<const Face&>().
                                 parameter_index(
                                     std::declval<face::string_arg>(),
                                     std::declval<std::size_t&>()))>
    void bind_string(const std::string& _name,
                     const std::string& _value);

    template<typename Face = statement_face,
             typename      = decltype(std::declval<const Face&>().
                                 parameter_index(
                                     std::declval<face::string_arg>(),
                                     std::declval<std::size_t&>()))>
    void bind_binary(const std::string&                  _name,
                     const std::vector<re_std::uint8_t>& _value);

    template<typename Face = statement_face,
             typename      = decltype(std::declval<const Face&>().
                                 parameter_index(
                                     std::declval<face::string_arg>(),
                                     std::declval<std::size_t&>()))>
    void bind_value(const std::string& _name,
                    const value&       _value);

    /**
     * @brief Runs the statement, and returns its rows.
     *
     * @return the rows, all of them, held apart from the statement and the
     *         connection.
     * @throws query_exception when the statement is closed or the run fails;
     *         connection_exception when the database dropped the connection.
     */
    std::unique_ptr<buffered_result_set> execute_query();

    /**
     * @brief Runs the statement, and counts the rows it changed.
     *
     * @return the rows it inserted, updated or deleted, or returned for a
     *         statement that returns rows, as the layer counts them; the
     *         connection's get_affected_rows() answers the same until another
     *         statement runs.
     * @throws as execute_query.
     */
    re_std::int64_t execute_update();

    /**
     * @brief Runs the statement, as execute_update does.
     *
     * @return true; a failure throws, as execute_update's does.
     */
    bool execute();

    /**
     * @brief Runs the statement, its rows to be read a page at a time as the
     *        stream's cursor reaches them (streaming_result_set).
     *
     * @note The stream's own run, if one is open, ends first. On MariaDB,
     *       MySQL and PostgreSQL the run holds the connection until it ends;
     *       on every vendor the statement's values cannot change until then.
     *       The first page is read before the call returns, so that the
     *       columns are known and a statement the database refuses fails
     *       here.
     *
     * @param[in] _page_size  the most rows a page holds; 1 or more.
     * @return the stream, which reads through this statement: it must not
     *         outlive it, and running the statement again closes it.
     * @throws query_exception when the statement is closed, for a page size
     *         of 0 (D_DB_INVALID_ARGUMENT), while another statement's stream
     *         holds the connection (Layer::busy_code), or when the run fails;
     *         connection_exception when the database dropped the connection.
     */
    std::unique_ptr<streaming_result_set<Layer>>
    execute_stream(std::size_t _page_size = stream_page_size);

    /**
     * @brief Runs a statement as execute_stream does, the stream taking the
     *        statement, which then lives as long as it: a connection's
     *        stream_query.
     *
     * @param[in] _statement  the statement; taken.
     * @param[in] _page_size  as execute_stream's.
     * @return the stream, holding the statement.
     * @throws as execute_stream; the statement is closed then.
     */
    static std::unique_ptr<streaming_result_set<Layer>>
    stream(std::unique_ptr<prepared_statement> _statement,
           std::size_t                         _page_size);

    // describing -- the number of the highest parameter; throws
    // query_exception when the statement is closed
    std::size_t parameter_count() const;

    // clearing, resetting -- clear_parameters makes every parameter NULL
    // again, and throws as binding does; reset does nothing, every run
    // starting from the beginning and leaving the statement ready
    void clear_parameters();
    void reset() noexcept;

    /**
     * @brief Finalizes the statement, and leaves the session.
     *
     * @post The statement is closed: every member that runs or binds throws.
     */
    void close() noexcept;

    // state and handle -- whether the statement is still open, and the
    // layer's own statement, for its calls, NULL once closed
    bool is_open() const noexcept;
    auto get_native_handle() const noexcept;

private:
    friend class streaming_result_set<Layer>;

    // m_require_open -- query_exception for a closed statement
    void m_require_open(const char* _operation) const;

    // m_require_free -- query_exception, Layer's busy_code, while another
    // statement's stream holds the connection; m_require_unstreamed --
    // query_exception, D_DB_INVALID_ARGUMENT, while the statement's own
    // stream's run is open, for a call that would change its values
    void m_require_free(const char* _operation) const;
    void m_require_unstreamed(const char* _operation) const;

    // m_end_stream -- the statement's stream, if it has one, ended: its run
    // read away, the connection freed, the stream closed
    void m_end_stream() noexcept;

    // m_check -- nothing for 0; for any other result, marks a lost connection
    // in the session, then throws what the layer raises
    void m_check(int         _result,
                 const char* _operation) const;

    // m_close_from_session -- the session's closer for this class
    static void m_close_from_session(void* _self) noexcept;

    std::shared_ptr<internal::session> m_session;
    statement_face                     m_statement;
    streaming_result_set<Layer>*       m_stream;
};

// 4.2    Definitions
//------------------------------------------------------------------------------
// prepared_statement's members, which every vendor instantiates for its
// layer; each vendor's source instantiates its own explicitly, and its
// header declares that instantiation extern.

/*
prepared_statement::prepared_statement
  The face statement moves in first, so that if enrolling fails the member's
destructor finalizes it: nothing leaks, and nothing is left enrolled.
*/
template<typename Layer>
prepared_statement<Layer>::prepared_statement(
    std::shared_ptr<internal::session> _session,
    statement_face&&                   _statement
)
    : m_session(std::move(_session)),
      m_statement(std::move(_statement)),
      m_stream(nullptr)
{
    m_session->enroll(this, &prepared_statement::m_close_from_session);
}

/*
prepared_statement::~prepared_statement
  Withdrawing first: once the session forgets the statement, nothing else
can reach it while the member finalizes it; its stream, if it has one, ends
before the statement it reads goes.
*/
template<typename Layer>
prepared_statement<Layer>::~prepared_statement()
{
    m_session->withdraw(this);
    m_end_stream();
}

template<typename Layer>
void
prepared_statement<Layer>::bind_null(
    std::size_t _index
)
{
    m_require_open("bind_null");
    m_require_unstreamed("bind_null");
    m_check(m_statement.bind_null(_index), "bind_null");

    return;
}

template<typename Layer>
void
prepared_statement<Layer>::bind_bool(
    std::size_t _index,
    bool        _value
)
{
    m_require_open("bind_bool");
    m_require_unstreamed("bind_bool");
    m_check(m_statement.bind_bool(_index, _value), "bind_bool");

    return;
}

template<typename Layer>
void
prepared_statement<Layer>::bind_int(
    std::size_t     _index,
    re_std::int32_t _value
)
{
    m_require_open("bind_int");
    m_require_unstreamed("bind_int");
    m_check(m_statement.bind_int64(_index, _value), "bind_int");

    return;
}

template<typename Layer>
void
prepared_statement<Layer>::bind_long(
    std::size_t     _index,
    re_std::int64_t _value
)
{
    m_require_open("bind_long");
    m_require_unstreamed("bind_long");
    m_check(m_statement.bind_int64(_index, _value), "bind_long");

    return;
}

template<typename Layer>
void
prepared_statement<Layer>::bind_double(
    std::size_t _index,
    double      _value
)
{
    m_require_open("bind_double");
    m_require_unstreamed("bind_double");
    m_check(m_statement.bind_double(_index, _value), "bind_double");

    return;
}

/*
prepared_statement::bind_string
  The layers take text as a C string, so a NUL inside would end it early and
bind less than the caller gave; the text is refused instead, the way to bind
such bytes named in the message.
*/
template<typename Layer>
void
prepared_statement<Layer>::bind_string(
    std::size_t        _index,
    const std::string& _value
)
{
    m_require_open("bind_string");
    m_require_unstreamed("bind_string");

    // text with a NUL inside cannot cross as a C string
    if (_value.find('\0') != std::string::npos)
    {
        throw query_exception("bind_string: the text holds a NUL character, "
                              "which only bind_binary can bind",
                              D_DB_INVALID_ARGUMENT);
    }

    m_check(m_statement.bind_text(_index, _value.c_str()), "bind_string");

    return;
}

/*
prepared_statement::bind_binary
  An empty vector's data() may be NULL, which the layers read as SQL NULL;
an empty BLOB needs a pointer to no bytes, so one is supplied.
*/
template<typename Layer>
void
prepared_statement<Layer>::bind_binary(
    std::size_t                         _index,
    const std::vector<re_std::uint8_t>& _value
)
{
    static const re_std::uint8_t none = 0u;

    m_require_open("bind_binary");
    m_require_unstreamed("bind_binary");
    m_check(m_statement.bind_blob(_index,
                                  (_value.empty()) ? &none : _value.data(),
                                  _value.size()),
            "bind_binary");

    return;
}

/*
prepared_statement::bind_value
  One bind call per alternative; a point in time goes through value_to_string,
so that what is bound is what the utilities write and read back.
*/
template<typename Layer>
void
prepared_statement<Layer>::bind_value(
    std::size_t  _index,
    const value& _value
)
{
    std::visit(
        [this, _index, &_value](const auto& _held) -> void
        {
            using held_type = std::decay_t<decltype(_held)>;

            // each alternative to the bind for its type
            if constexpr (std::is_same_v<held_type, std::monostate>)
            {
                bind_null(_index);
            }
            else if constexpr (std::is_same_v<held_type, bool>)
            {
                bind_bool(_index, _held);
            }
            else if constexpr (std::is_same_v<held_type, re_std::int32_t>)
            {
                bind_int(_index, _held);
            }
            else if constexpr (std::is_same_v<held_type, re_std::int64_t>)
            {
                bind_long(_index, _held);
            }
            else if constexpr (std::is_same_v<held_type, double>)
            {
                bind_double(_index, _held);
            }
            else if constexpr (std::is_same_v<held_type, std::string>)
            {
                bind_string(_index, _held);
            }
            else if constexpr (std::is_same_v<held_type,
                                              std::vector<re_std::uint8_t>>)
            {
                bind_binary(_index, _held);
            }
            else
            {
                bind_string(_index,
                            value_to_string(_value) + Layer::utc_offset);
            }

            return;
        },
        _value);

    return;
}

template<typename Layer>
template<typename Face,
         typename>
std::size_t
prepared_statement<Layer>::parameter_index(
    const std::string& _name
) const
{
    std::size_t index = 0u;

    m_require_open("parameter_index");

    const int result = m_statement.parameter_index(_name.c_str(), index);

    // a name no parameter has is the caller's to hear of by name
    if (result == D_DB_NOT_FOUND)
    {
        throw query_exception("parameter_index: the statement has no "
                              "parameter named " + _name,
                              D_DB_NOT_FOUND);
    }

    m_check(result, "parameter_index");

    return index;
}

template<typename Layer>
template<typename Face,
         typename>
void
prepared_statement<Layer>::bind_null(
    const std::string& _name
)
{
    bind_null(parameter_index(_name));

    return;
}

template<typename Layer>
template<typename Face,
         typename>
void
prepared_statement<Layer>::bind_bool(
    const std::string& _name,
    bool               _value
)
{
    bind_bool(parameter_index(_name), _value);

    return;
}

template<typename Layer>
template<typename Face,
         typename>
void
prepared_statement<Layer>::bind_int(
    const std::string& _name,
    re_std::int32_t    _value
)
{
    bind_int(parameter_index(_name), _value);

    return;
}

template<typename Layer>
template<typename Face,
         typename>
void
prepared_statement<Layer>::bind_long(
    const std::string& _name,
    re_std::int64_t    _value
)
{
    bind_long(parameter_index(_name), _value);

    return;
}

template<typename Layer>
template<typename Face,
         typename>
void
prepared_statement<Layer>::bind_double(
    const std::string& _name,
    double             _value
)
{
    bind_double(parameter_index(_name), _value);

    return;
}

template<typename Layer>
template<typename Face,
         typename>
void
prepared_statement<Layer>::bind_string(
    const std::string& _name,
    const std::string& _value
)
{
    bind_string(parameter_index(_name), _value);

    return;
}

template<typename Layer>
template<typename Face,
         typename>
void
prepared_statement<Layer>::bind_binary(
    const std::string&                  _name,
    const std::vector<re_std::uint8_t>& _value
)
{
    bind_binary(parameter_index(_name), _value);

    return;
}

template<typename Layer>
template<typename Face,
         typename>
void
prepared_statement<Layer>::bind_value(
    const std::string& _name,
    const value&       _value
)
{
    bind_value(parameter_index(_name), _value);

    return;
}

template<typename Layer>
std::unique_ptr<buffered_result_set>
prepared_statement<Layer>::execute_query()
{
    face::rows found;

    m_require_open("execute_query");
    m_end_stream();
    m_require_free("execute_query");
    m_check(m_statement.execute_query(found), "execute_query");

    return std::make_unique<buffered_result_set>(std::move(found));
}

/*
prepared_statement::execute_update
  The count goes to the session too, so that the connection's
get_affected_rows() answers for the latest statement run on it, however it
ran.
*/
template<typename Layer>
re_std::int64_t
prepared_statement<Layer>::execute_update()
{
    re_std::int64_t changes = 0;

    m_require_open("execute_update");
    m_end_stream();
    m_require_free("execute_update");
    m_check(m_statement.execute_update(changes), "execute_update");
    m_session->record(changes);

    return changes;
}

template<typename Layer>
bool
prepared_statement<Layer>::execute()
{
    re_std::int64_t changes = 0;

    m_require_open("execute");
    m_end_stream();
    m_require_free("execute");
    m_check(m_statement.execute_update(changes), "execute");
    m_session->record(changes);

    return true;
}

/*
prepared_statement::execute_stream
  The run begins in the layer before the stream exists, and the stream holds
the connection from then on; its first page is read last, so that a failure
there unwinds through the stream's destructor, which ends the run.
*/
template<typename Layer>
std::unique_ptr<streaming_result_set<Layer>>
prepared_statement<Layer>::execute_stream(
    std::size_t _page_size
)
{
    m_require_open("execute_stream");

    // a page holds at least one row
    if (_page_size == 0u)
    {
        throw query_exception("execute_stream: a page holds at least one row",
                              D_DB_INVALID_ARGUMENT);
    }

    m_end_stream();
    m_require_free("execute_stream");
    m_check(m_statement.open_cursor(), "execute_stream");

    std::unique_ptr<streaming_result_set<Layer>> made(
        new streaming_result_set<Layer>(*this, _page_size));

    m_stream = made.get();

    // the run holds the connection, where the layer's runs do
    if constexpr (Layer::holds_connection)
    {
        m_session->hold(m_stream, Layer::busy_code);
    }

    made->m_page = made->m_fetch("execute_stream");

    return made;
}

/*
prepared_statement::stream
  The statement moves into the stream only once the stream exists; until
then a failure leaves it with the caller's unique_ptr, which closes it.
*/
template<typename Layer>
std::unique_ptr<streaming_result_set<Layer>>
prepared_statement<Layer>::stream(
    std::unique_ptr<prepared_statement> _statement,
    std::size_t                         _page_size
)
{
    std::unique_ptr<streaming_result_set<Layer>> made =
        _statement->execute_stream(_page_size);

    made->m_owned = std::move(_statement);

    return made;
}

template<typename Layer>
std::size_t
prepared_statement<Layer>::parameter_count() const
{
    m_require_open("parameter_count");

    return m_statement.parameter_count();
}

template<typename Layer>
void
prepared_statement<Layer>::clear_parameters()
{
    m_require_open("clear_parameters");
    m_require_unstreamed("clear_parameters");
    m_check(m_statement.clear_bindings(), "clear_parameters");

    return;
}

template<typename Layer>
void
prepared_statement<Layer>::reset() noexcept
{
    return;
}

template<typename Layer>
void
prepared_statement<Layer>::close() noexcept
{
    m_session->withdraw(this);
    m_end_stream();
    (void)m_statement.finalize();

    return;
}

template<typename Layer>
bool
prepared_statement<Layer>::is_open() const noexcept
{
    return m_statement.is_prepared();
}

template<typename Layer>
auto
prepared_statement<Layer>::get_native_handle() const noexcept
{
    return m_statement.handle();
}

template<typename Layer>
void
prepared_statement<Layer>::m_require_open(
    const char* _operation
) const
{
    // a statement the session or close() finalized runs nothing
    if (!m_statement.is_prepared())
    {
        throw query_exception(std::string(_operation) +
                              ": the statement is closed");
    }

    return;
}

/*
prepared_statement::m_check
  A statement cannot reach its connection object, which may have moved; the
session is what they share, so a lost connection is marked there, and the
connection reads it before its next operation. A result that means a lost
connection marks it at once; a layer that learns more as it raises -- the
MariaDB layer's 2020, a statement's failure or the connection's -- says so
by the exception it throws, a connection_exception for a statement's
failure being only ever a lost connection's.
*/
template<typename Layer>
void
prepared_statement<Layer>::m_check(
    int         _result,
    const char* _operation
) const
{
    // success needs nothing
    if (_result == 0)
    {
        return;
    }

    // a dropped connection is the connection's to hear of too
    if (Layer::connection_lost(_result))
    {
        m_session->mark_lost();
    }

    try
    {
        Layer::raise(m_statement.statement_connection(),
                     failure_kind::query,
                     _operation,
                     _result);
    }
    catch (const connection_exception&)
    {
        m_session->mark_lost();

        throw;
    }

    return;
}

/*
prepared_statement::m_require_free
  The session names the stream that holds the connection; the statement's
own was ended before this is asked.
*/
template<typename Layer>
void
prepared_statement<Layer>::m_require_free(
    const char* _operation
) const
{
    // another statement's run holds the connection
    if ( (m_session->holder() != nullptr) &&
         (m_session->holder() != m_stream) )
    {
        throw query_exception(std::string(_operation) +
                              ": a stream's run holds the connection until "
                              "it is read to its end or closed",
                              m_session->busy_code());
    }

    return;
}

template<typename Layer>
void
prepared_statement<Layer>::m_require_unstreamed(
    const char* _operation
) const
{
    // the statement's own run reads the values it began with
    if ( (m_stream != nullptr) &&
         (!m_stream->m_ended) )
    {
        throw query_exception(std::string(_operation) +
                              ": the statement's stream is open; its values "
                              "cannot change until the run ends",
                              D_DB_INVALID_ARGUMENT);
    }

    return;
}

/*
prepared_statement::m_end_stream
  The link goes first, so that the stream, told it is closed, does not come
back to end what is already ending; the layer reads the rest of the run
away, whatever it held.
*/
template<typename Layer>
void
prepared_statement<Layer>::m_end_stream() noexcept
{
    // no stream to end
    if (m_stream == nullptr)
    {
        return;
    }

    streaming_result_set<Layer>* const ending = m_stream;

    m_stream = nullptr;
    (void)m_statement.close_cursor();
    m_session->release(ending);
    ending->m_detach();

    return;
}

template<typename Layer>
void
prepared_statement<Layer>::m_close_from_session(
    void* _self
) noexcept
{
    prepared_statement* const self = static_cast<prepared_statement*>(_self);

    self->m_end_stream();
    (void)self->m_statement.finalize();

    return;
}


// 4.3    Streams' definitions
//------------------------------------------------------------------------------
// streaming_result_set's members, instantiated with prepared_statement's.

template<typename Layer>
streaming_result_set<Layer>::streaming_result_set(
    prepared_statement<Layer>& _statement,
    std::size_t                _page_size
) noexcept
    : m_statement(&_statement),
      m_owned(),
      m_page(),
      m_page_size(_page_size),
      m_base(0),
      m_ended(false),
      m_open(true)
{}

/*
streaming_result_set::~streaming_result_set
  The run ends before the statement a stream_query stream owns goes with it.
*/
template<typename Layer>
streaming_result_set<Layer>::~streaming_result_set()
{
    close();
}

/*
streaming_result_set::next
  The page held is read to its end first; then the next, its rows counted
after the page's. An empty last page is held all the same, for its columns.
*/
template<typename Layer>
bool
streaming_result_set<Layer>::next()
{
    m_require_open("next");

    // a row of the page held, or the end of the run
    if ( (m_page.next()) ||
         (m_ended) )
    {
        return (m_page.current_row() < m_page.row_count());
    }

    buffered_result_set more = m_fetch("next");

    m_base += m_page.row_count();
    m_page  = std::move(more);

    return m_page.next();
}

template<typename Layer>
bool
streaming_result_set<Layer>::previous()
{
    m_require_open("previous");
    m_backward("previous");

    return false;
}

/*
streaming_result_set::first
  The first page still held is the one place the first row can be found.
*/
template<typename Layer>
bool
streaming_result_set<Layer>::first()
{
    m_require_open("first");

    // the first page gone
    if (m_base != 0)
    {
        m_backward("first");
    }

    return m_page.first();
}

/*
streaming_result_set::last
  Pages are read to the run's end; an empty last page, which only says the
run is over, leaves the page before it held, whose last row is the run's.
*/
template<typename Layer>
bool
streaming_result_set<Layer>::last()
{
    m_require_open("last");

    // every page to the end, keeping the last that has rows
    while (!m_ended)
    {
        buffered_result_set more = m_fetch("last");

        if (more.row_count() > 0)
        {
            m_base += m_page.row_count();
            m_page  = std::move(more);
        }
    }

    return m_page.last();
}

/*
streaming_result_set::absolute
  A row from the end needs the run's length, so the stream reads to it; a
row ahead is reached a page at a time; a row behind, or beyond the last
page held, is out of reach.
*/
template<typename Layer>
bool
streaming_result_set<Layer>::absolute(
    re_std::int64_t _row
)
{
    m_require_open("absolute");

    // from the end: the length first, then the row within the last page
    if (_row < 0)
    {
        (void)last();

        const re_std::int64_t target = row_count() + _row;

        // a place before the last page: within reach only when the run fit
        // in one page, and then only the place before the first row
        if (target < m_base)
        {
            if (m_base != 0)
            {
                m_backward("absolute");
            }

            return m_page.absolute(-(m_page.row_count() + 1));
        }

        return m_page.absolute(target - m_base);
    }

    // a row behind the cursor
    if (_row < current_row())
    {
        m_backward("absolute");
    }

    // pages read until the row's, or the end
    while ( (_row >= (m_base + m_page.row_count())) &&
            (!m_ended) )
    {
        buffered_result_set more = m_fetch("absolute");

        m_base += m_page.row_count();
        m_page  = std::move(more);
    }

    return m_page.absolute(_row - m_base);
}

template<typename Layer>
std::size_t
streaming_result_set<Layer>::column_count() const noexcept
{
    return m_page.column_count();
}

template<typename Layer>
std::string
streaming_result_set<Layer>::column_name(
    std::size_t _index
) const
{
    return m_page.column_name(_index);
}

template<typename Layer>
field_type
streaming_result_set<Layer>::column_type(
    std::size_t _index
) const
{
    return m_page.column_type(_index);
}

template<typename Layer>
std::size_t
streaming_result_set<Layer>::column_index(
    const std::string& _name
) const
{
    return m_page.column_index(_name);
}

template<typename Layer>
re_std::int64_t
streaming_result_set<Layer>::row_count() const noexcept
{
    return (m_ended) ? (m_base + m_page.row_count())
                     : -1;
}

/*
streaming_result_set::current_row
  The page's place, counted after the pages before it; before the first row
only the first page is ever held.
*/
template<typename Layer>
re_std::int64_t
streaming_result_set<Layer>::current_row() const noexcept
{
    return m_base + m_page.current_row();
}

template<typename Layer>
value
streaming_result_set<Layer>::get_value(
    std::size_t _index
) const
{
    return m_page.get_value(_index);
}

template<typename Layer>
bool
streaming_result_set<Layer>::is_null(
    std::size_t _index
) const
{
    return m_page.is_null(_index);
}

template<typename Layer>
std::optional<bool>
streaming_result_set<Layer>::get_bool(
    std::size_t _index
) const
{
    return m_page.get_bool(_index);
}

template<typename Layer>
std::optional<re_std::int32_t>
streaming_result_set<Layer>::get_int(
    std::size_t _index
) const
{
    return m_page.get_int(_index);
}

template<typename Layer>
std::optional<re_std::int64_t>
streaming_result_set<Layer>::get_long(
    std::size_t _index
) const
{
    return m_page.get_long(_index);
}

template<typename Layer>
std::optional<double>
streaming_result_set<Layer>::get_double(
    std::size_t _index
) const
{
    return m_page.get_double(_index);
}

template<typename Layer>
std::optional<std::string>
streaming_result_set<Layer>::get_string(
    std::size_t _index
) const
{
    return m_page.get_string(_index);
}

template<typename Layer>
std::optional<std::vector<re_std::uint8_t>>
streaming_result_set<Layer>::get_binary(
    std::size_t _index
) const
{
    return m_page.get_binary(_index);
}

template<typename Layer>
value
streaming_result_set<Layer>::get_value(
    const std::string& _name
) const
{
    return m_page.get_value(_name);
}

template<typename Layer>
bool
streaming_result_set<Layer>::is_null(
    const std::string& _name
) const
{
    return m_page.is_null(_name);
}

template<typename Layer>
std::optional<bool>
streaming_result_set<Layer>::get_bool(
    const std::string& _name
) const
{
    return m_page.get_bool(_name);
}

template<typename Layer>
std::optional<re_std::int32_t>
streaming_result_set<Layer>::get_int(
    const std::string& _name
) const
{
    return m_page.get_int(_name);
}

template<typename Layer>
std::optional<re_std::int64_t>
streaming_result_set<Layer>::get_long(
    const std::string& _name
) const
{
    return m_page.get_long(_name);
}

template<typename Layer>
std::optional<double>
streaming_result_set<Layer>::get_double(
    const std::string& _name
) const
{
    return m_page.get_double(_name);
}

template<typename Layer>
std::optional<std::string>
streaming_result_set<Layer>::get_string(
    const std::string& _name
) const
{
    return m_page.get_string(_name);
}

template<typename Layer>
std::optional<std::vector<re_std::uint8_t>>
streaming_result_set<Layer>::get_binary(
    const std::string& _name
) const
{
    return m_page.get_binary(_name);
}

template<typename Layer>
row
streaming_result_set<Layer>::get_current_row() const
{
    return m_page.get_current_row();
}

template<typename Layer>
result_rows
streaming_result_set<Layer>::get_all_rows()
{
    result_rows all;

    // each row after the cursor, page after page
    while (next())
    {
        all.push_back(m_page.get_current_row());
    }

    return all;
}

/*
streaming_result_set::close
  The statement ends the run, and tells the stream so (m_detach); a stream
its statement already ended has nothing more to end.
*/
template<typename Layer>
void
streaming_result_set<Layer>::close() noexcept
{
    // the run, ended by the statement that reads it
    if (m_statement != nullptr)
    {
        m_statement->m_end_stream();
    }

    m_detach();

    return;
}

template<typename Layer>
bool
streaming_result_set<Layer>::is_open() const noexcept
{
    return m_open;
}

template<typename Layer>
bool
streaming_result_set<Layer>::is_ended() const noexcept
{
    return m_ended;
}

template<typename Layer>
const buffered_result_set&
streaming_result_set<Layer>::page() const noexcept
{
    return m_page;
}

/*
streaming_result_set::m_fetch
  The layer ends the run itself on a failure and on a short page; the
connection is freed before the failure is raised, so that the program's
next operation runs.
*/
template<typename Layer>
buffered_result_set
streaming_result_set<Layer>::m_fetch(
    const char* _operation
)
{
    face::rows found;
    const int  result = m_statement->m_statement.fetch_rows(m_page_size,
                                                            found);

    // a failure ends the run
    if (result != 0)
    {
        m_end_run();
        m_statement->m_check(result, _operation);
    }

    buffered_result_set page(std::move(found));

    // a short page is the last
    if (page.row_count() < static_cast<re_std::int64_t>(m_page_size))
    {
        m_end_run();
    }

    return page;
}

template<typename Layer>
void
streaming_result_set<Layer>::m_end_run() noexcept
{
    m_ended = true;

    // the connection, free of the run
    if (m_statement != nullptr)
    {
        m_statement->m_session->release(this);
    }

    return;
}

template<typename Layer>
void
streaming_result_set<Layer>::m_detach() noexcept
{
    m_statement = nullptr;
    m_ended     = true;
    m_open      = false;
    m_page.close();

    return;
}

template<typename Layer>
void
streaming_result_set<Layer>::m_require_open(
    const char* _operation
) const
{
    // a stream its statement or close() ended reads nothing more
    if (!m_open)
    {
        throw query_exception(std::string(_operation) +
                              ": the stream is closed");
    }

    return;
}

template<typename Layer>
void
streaming_result_set<Layer>::m_backward(
    const char* _operation
) const
{
    throw query_exception(std::string(_operation) +
                          ": a stream moves forward; the row is no longer "
                          "held",
                          D_DB_UNSUPPORTED);
}


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_DB_DATABASE_STATEMENT_HPP
