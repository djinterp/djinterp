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
*                       operation's name and the result
*   Neither class is thread-safe: a connection, its statements and its result
* sets belong to one thread at a time, as each client's own do.
*
*
* path:      /inc/djinterp/core/db/database_statement.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
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
4.  STATEMENTS
    ----------
    1.  Prepared statements
         1.  prepared_statement
    2.  Definitions
*/

#ifndef DJINTERP_DB_DATABASE_STATEMENT_HPP
#define DJINTERP_DB_DATABASE_STATEMENT_HPP 1

// djinterp
#include "../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: this header's floor

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
// closes; the rows the latest statement changed; and whether a statement
// found the connection lost. Held by std::shared_ptr, so that a statement
// that outlives its connection still has a session to leave. Not copyable.
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
    // m_require_open -- query_exception for a closed statement
    void m_require_open(const char* _operation) const;

    // m_check -- nothing for 0; for any other result, marks a lost connection
    // in the session, then throws what the layer raises
    void m_check(int         _result,
                 const char* _operation) const;

    // m_close_from_session -- the session's closer for this class
    static void m_close_from_session(void* _self) noexcept;

    std::shared_ptr<internal::session> m_session;
    statement_face                     m_statement;
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
      m_statement(std::move(_statement))
{
    m_session->enroll(this, &prepared_statement::m_close_from_session);
}

/*
prepared_statement::~prepared_statement
  Withdrawing first: once the session forgets the statement, nothing else
can reach it while the member finalizes it.
*/
template<typename Layer>
prepared_statement<Layer>::~prepared_statement()
{
    m_session->withdraw(this);
}

template<typename Layer>
void
prepared_statement<Layer>::bind_null(
    std::size_t _index
)
{
    m_require_open("bind_null");
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
    m_check(m_statement.execute_update(changes), "execute_update");
    m_session->record(changes);

    return changes;
}

template<typename Layer>
bool
prepared_statement<Layer>::execute()
{
    (void)execute_update();

    return true;
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
connection reads it before its next operation.
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

    Layer::raise(m_statement.statement_connection(),
                 failure_kind::query,
                 _operation,
                 _result);

    return;
}

template<typename Layer>
void
prepared_statement<Layer>::m_close_from_session(
    void* _self
) noexcept
{
    (void)static_cast<prepared_statement*>(_self)->m_statement.finalize();

    return;
}


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_DB_DATABASE_STATEMENT_HPP
