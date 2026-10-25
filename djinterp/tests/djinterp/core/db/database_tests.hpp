/*******************************************************************************
* djinterp [database]                                         database_tests.hpp
*
* djinterp database unit-test header:
*   Declarations, check macros, and shared test fixtures (mock CRTP
* helpers, instrumentation "probe" structs, and compile-time detection
* traits) for the unit tests of `db/database.hpp`.
*
*   The suite is split, one translation unit per semantic section of the
* module, mirroring the header's own section layout:
*     - database_tests_enums.cpp        (enumerations + type aliases)
*     - database_tests_exceptions.cpp   (exception hierarchy)
*     - database_tests_config.cpp       (connection_config / pool_config)
*     - database_tests_vendor.cpp       (vendor_info / vendor_traits)
*     - database_tests_result_set.cpp   (result_set CRTP base)
*     - database_tests_statement.cpp    (statement CRTP base)
*     - database_tests_transaction.cpp  (transaction RAII template)
*     - database_tests_connection.cpp   (connection CRTP base)
*     - database_tests_pool.cpp         (connection_pool / pooled_connection)
*
*   Every test function is `bool tests_<entity>()`, declared flat in
* `djinterp::testing`, returning true on success. The runner
* (build/cmake/config/.../database_tests_runner.cpp) drives them.
*
*   TESTING STRATEGY FOR THE CRTP BASES
*   ===================================
*   result_set, statement, and connection are CRTP bases whose public
* methods forward to the derived class, a "helper" here. A helper that names
* its member identically hides the base method, so tests always invoke the
* forwarding path THROUGH A BASE REFERENCE, e.g.
*
*     mock_result_set m(&probe);
*     result_set<mock_result_set>& rs = m;
*     rs.next();                       // exercises result_set::next()
*
*   Each helper records its calls/arguments into an external probe and
* returns distinct sentinel values, so a test can assert both that the
* base forwarded the call/return (sentinel) and that it forwarded the
* argument (probe capture).
*
*   SFINAE-GATED METHODS
*   ====================
*   Several base methods are enabled only where the helper can do what they
* need (chunk 4.2's contract, database.hpp 4.1.1):
*     - result_set's absolute and statement's bind_value and by-name binds,
*       where the helper declares the member itself, overloaded or not; a
*       helper that declares no member of the name leaves it off.
*     - connection's savepoints, isolation accessors, begin_transaction(
*       isolation_level) and set_auto_commit, where the helper has the hook
*       database_connection implements them over, named after the operation
*       with the suffix `_impl`. mock_conn has both: the operation, which
*       the base forwards to, and the hook, which its gate asks for.
*   Every other connection method forwards to the helper's member of the
* same name -- in_transaction, get_state and get_auto_commit among them,
* which the database answers rather than the base -- so a helper declares
* each one a test calls: a forwarder whose member the helper lacks would call
* itself. transaction<> likewise asks its connection in_transaction(), after
* a commit or a rollback that failed.
*   Detection traits (defined here) assert each gate is on/off as
* intended.
*
*
* path:      /tests/djinterp/core/db/database_tests.hpp
* link(s):   TBA
* author(s): djinterp test-suite                                    created: TBA
*                                                            revised: 2026.10.23
*******************************************************************************/

#ifndef DJINTERP_DATABASE_TESTS_
#define DJINTERP_DATABASE_TESTS_ 1

#include "../../../../inc/djinterp/env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER

// below C++17, core/db's floor, the suite compiles to nothing, and the runner
// says so
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <chrono>
#include <cstdio>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>
// djinterp
#include "../../../../inc/djinterp/core/db/database.hpp"
// re_std
#include "../../../../inc/re_std/cstdint/cstdint.hpp"  // re_std::int64_t

NS_DJINTERP
namespace testing
{
    // =======================================================================
    // check macros
    // =======================================================================

    // D_DB_CHECK
    //   macro: fail the enclosing bool test (print + return false) if the
    // condition is false.
    #define D_DB_CHECK(_cond)                                              \
        do                                                                 \
        {                                                                  \
            if (!(_cond))                                                  \
            {                                                              \
                std::printf("      [check] FAILED: %s\n"                   \
                            "        at %s:%d\n",                          \
                            #_cond, __FILE__, __LINE__);                   \
                return false;                                              \
            }                                                              \
        }                                                                  \
        while (0)

    // D_DB_CHECK_THROWS
    //   macro: fail unless evaluating _expr throws an exception of
    // (or derived from) type _ex.
    #define D_DB_CHECK_THROWS(_expr, _ex)                                  \
        do                                                                 \
        {                                                                  \
            bool _dj_threw = false;                                        \
                                                                           \
            try                                                            \
            {                                                              \
                (void)(_expr);                                             \
            }                                                              \
            catch (const _ex&)                                            \
            {                                                              \
                _dj_threw = true;                                          \
            }                                                              \
            catch (...)                                                    \
            {                                                              \
                _dj_threw = false;                                         \
            }                                                              \
                                                                           \
            if (!_dj_threw)                                                \
            {                                                              \
                std::printf("      [check] expected %s from %s\n"          \
                            "        at %s:%d\n",                          \
                            #_ex, #_expr, __FILE__, __LINE__);             \
                return false;                                              \
            }                                                              \
        }                                                                  \
        while (0)

    // D_DB_CHECK_NOTHROW
    //   macro: fail if evaluating _expr throws any exception.
    #define D_DB_CHECK_NOTHROW(_expr)                                      \
        do                                                                 \
        {                                                                  \
            try                                                            \
            {                                                              \
                (void)(_expr);                                             \
            }                                                              \
            catch (...)                                                    \
            {                                                              \
                std::printf("      [check] unexpected exception from %s\n" \
                            "        at %s:%d\n",                          \
                            #_expr, __FILE__, __LINE__);                   \
                return false;                                              \
            }                                                              \
        }                                                                  \
        while (0)


    // =======================================================================
    // sentinels (distinct return values used to prove forwarding)
    // =======================================================================

    // result_set sentinels
    inline constexpr std::size_t  k_rs_column_count = 7u;
    inline constexpr field_type   k_rs_column_type  = field_type::json;
    inline constexpr re_std::int64_t k_rs_row_count    = 123;
    inline constexpr re_std::int64_t k_rs_current_row  = 45;
    inline constexpr re_std::int32_t k_rs_int_by_index = 3200;
    inline constexpr re_std::int64_t k_rs_long         = 640000;
    inline constexpr double       k_rs_double       = 2.5;

    // statement sentinels
    inline constexpr int          k_stmt_query_val   = 5550;
    inline constexpr re_std::int64_t k_stmt_update_val  = 7770;
    inline constexpr std::size_t  k_stmt_param_count = 9u;

    // connection sentinels
    inline constexpr int          k_conn_query_val   = 1010;
    inline constexpr re_std::int64_t k_conn_update_val  = 2020;
    inline constexpr int          k_conn_prepare_val = 3030;
    inline constexpr re_std::int64_t k_conn_insert_id   = 424242;
    inline constexpr re_std::int64_t k_conn_affected    = 7;
    inline constexpr int          k_conn_error_code  = -55;


    // =======================================================================
    // result_set fixtures
    // =======================================================================

    // rs_probe
    //   struct: instrumentation for the result_set mock helpers.
    struct rs_probe
    {
        std::size_t  last_index            = 0;
        std::string  last_name;
        re_std::int64_t last_row              = 0;
        bool         closed                = false;
        int          next_calls            = 0;
        int          previous_calls        = 0;
        int          first_calls           = 0;
        int          last_calls            = 0;
        int          absolute_calls        = 0;
        int          get_current_row_calls = 0;
        int          get_all_rows_calls    = 0;
    };

    // mock_result_set
    //   class: a result_set helper implementing every impl method, plus the
    // SFINAE-gated absolute(). Records into an external rs_probe.
    class mock_result_set : public result_set<mock_result_set>
    {
    public:
        explicit mock_result_set(rs_probe* _probe)
            : m_probe(_probe)
        {}

        // navigation
        bool next()                     { ++m_probe->next_calls;     return true;  }
        bool previous()                 { ++m_probe->previous_calls; return false; }
        bool first()                    { ++m_probe->first_calls;    return true;  }
        bool last()                     { ++m_probe->last_calls;     return false; }
        bool absolute(re_std::int64_t _r)  { ++m_probe->absolute_calls; m_probe->last_row = _r; return true; }

        // metadata
        std::size_t column_count() const { return k_rs_column_count; }

        std::string column_name(std::size_t _i) const
        {
            m_probe->last_index = _i;
            return std::to_string(_i);
        }

        field_type column_type(std::size_t _i) const
        {
            m_probe->last_index = _i;
            return k_rs_column_type;
        }

        std::size_t column_index(const std::string& _n) const
        {
            m_probe->last_name = _n;
            return _n.size();
        }

        re_std::int64_t row_count()   const { return k_rs_row_count;   }
        re_std::int64_t current_row() const { return k_rs_current_row; }

        // data access by index
        value get_value(std::size_t _i) const
        {
            m_probe->last_index = _i;
            return value{ static_cast<re_std::int64_t>(_i) };
        }

        bool is_null(std::size_t _i) const
        {
            m_probe->last_index = _i;
            return true;
        }

        std::optional<bool> get_bool(std::size_t _i) const
        {
            m_probe->last_index = _i;
            return std::optional<bool>{ true };
        }

        std::optional<re_std::int32_t> get_int(std::size_t _i) const
        {
            m_probe->last_index = _i;
            return std::optional<re_std::int32_t>{ k_rs_int_by_index };
        }

        std::optional<re_std::int64_t> get_long(std::size_t _i) const
        {
            m_probe->last_index = _i;
            return std::optional<re_std::int64_t>{ k_rs_long };
        }

        std::optional<double> get_double(std::size_t _i) const
        {
            m_probe->last_index = _i;
            return std::optional<double>{ k_rs_double };
        }

        std::optional<std::string> get_string(std::size_t _i) const
        {
            m_probe->last_index = _i;
            return std::optional<std::string>{ "by_index" };
        }

        std::optional<std::vector<re_std::uint8_t>>
        get_binary(std::size_t _i) const
        {
            m_probe->last_index = _i;
            return std::optional<std::vector<re_std::uint8_t>>{
                std::vector<re_std::uint8_t>{ 1, 2, 3 } };
        }

        // data access by name (distinct sentinels derived from the name)
        value get_value(const std::string& _n) const
        {
            m_probe->last_name = _n;
            return value{ _n };
        }

        bool is_null(const std::string& _n) const
        {
            m_probe->last_name = _n;
            return false;
        }

        std::optional<bool> get_bool(const std::string& _n) const
        {
            m_probe->last_name = _n;
            return std::optional<bool>{ false };
        }

        std::optional<re_std::int32_t> get_int(const std::string& _n) const
        {
            m_probe->last_name = _n;
            return std::optional<re_std::int32_t>{
                static_cast<re_std::int32_t>(_n.size()) };
        }

        std::optional<re_std::int64_t> get_long(const std::string& _n) const
        {
            m_probe->last_name = _n;
            return std::optional<re_std::int64_t>{
                static_cast<re_std::int64_t>(_n.size()) };
        }

        std::optional<double> get_double(const std::string& _n) const
        {
            m_probe->last_name = _n;
            return std::optional<double>{
                static_cast<double>(_n.size()) };
        }

        std::optional<std::string> get_string(const std::string& _n) const
        {
            m_probe->last_name = _n;
            return std::optional<std::string>{ "name:" + _n };
        }

        std::optional<std::vector<re_std::uint8_t>>
        get_binary(const std::string& _n) const
        {
            m_probe->last_name = _n;
            return std::optional<std::vector<re_std::uint8_t>>{
                std::vector<re_std::uint8_t>{ 9, 8 } };
        }

        // convenience
        row get_current_row() const
        {
            ++m_probe->get_current_row_calls;
            row r;
            r["k"] = value{ k_rs_int_by_index };
            return r;
        }

        result_rows get_all_rows()
        {
            ++m_probe->get_all_rows_calls;
            return result_rows(2);
        }

        void close() { m_probe->closed = true; }

    private:
        rs_probe* m_probe;
    };

    // rs_no_absolute
    //   class: a result_set helper WITHOUT absolute(), used to verify the
    // absolute() gate is disabled.
    class rs_no_absolute : public result_set<rs_no_absolute>
    {
    public:
        bool next() { return true; }
    };

    // rs_has_absolute
    //   trait: true if `.absolute(re_std::int64_t)` is callable on _Type.
    template<typename _Type, typename = void>
    struct rs_has_absolute : std::false_type
    {};

    template<typename _Type>
    struct rs_has_absolute<_Type, std::void_t<decltype(
        std::declval<_Type&>().absolute(std::declval<re_std::int64_t>()))>>
        : std::true_type
    {};


    // =======================================================================
    // statement fixtures
    // =======================================================================

    // stmt_probe
    //   struct: instrumentation for the statement mock helpers.
    struct stmt_probe
    {
        std::string               last_method;
        std::size_t               last_index           = 0;
        std::string               last_name;
        bool                      v_bool               = false;
        re_std::int32_t              v_int                = 0;
        re_std::int64_t              v_long               = 0;
        double                    v_double             = 0.0;
        std::string               v_string;
        std::vector<re_std::uint8_t> v_binary;
        int                       execute_query_calls  = 0;
        int                       execute_update_calls = 0;
        int                       execute_calls        = 0;
        int                       clear_calls          = 0;
        int                       reset_calls          = 0;
        int                       close_calls          = 0;
    };

    // stmt_idx
    //   class: a statement helper with by-INDEX binds (the by-name gate is
    // consequently disabled). Also carries the execution/metadata impls.
    class stmt_idx : public statement<stmt_idx>
    {
    public:
        explicit stmt_idx(stmt_probe* _probe)
            : m_probe(_probe)
        {}

        void bind_null(std::size_t _i)
        {
            m_probe->last_method = "null";
            m_probe->last_index  = _i;
        }

        void bind_bool(std::size_t _i, bool _v)
        {
            m_probe->last_method = "bool";
            m_probe->last_index  = _i;
            m_probe->v_bool      = _v;
        }

        void bind_int(std::size_t _i, re_std::int32_t _v)
        {
            m_probe->last_method = "int";
            m_probe->last_index  = _i;
            m_probe->v_int       = _v;
        }

        void bind_long(std::size_t _i, re_std::int64_t _v)
        {
            m_probe->last_method = "long";
            m_probe->last_index  = _i;
            m_probe->v_long      = _v;
        }

        void bind_double(std::size_t _i, double _v)
        {
            m_probe->last_method = "double";
            m_probe->last_index  = _i;
            m_probe->v_double    = _v;
        }

        void bind_string(std::size_t _i, const std::string& _v)
        {
            m_probe->last_method = "string";
            m_probe->last_index  = _i;
            m_probe->v_string    = _v;
        }

        void bind_binary(std::size_t _i, const std::vector<re_std::uint8_t>& _v)
        {
            m_probe->last_method = "binary";
            m_probe->last_index  = _i;
            m_probe->v_binary    = _v;
        }

        int execute_query()
        {
            ++m_probe->execute_query_calls;
            return k_stmt_query_val;
        }

        re_std::int64_t execute_update()
        {
            ++m_probe->execute_update_calls;
            return k_stmt_update_val;
        }

        bool execute()
        {
            ++m_probe->execute_calls;
            return true;
        }

        std::size_t parameter_count() const { return k_stmt_param_count; }

        void clear_parameters() { ++m_probe->clear_calls; }
        void reset()            { ++m_probe->reset_calls; }
        void close()            { ++m_probe->close_calls; }

    private:
        stmt_probe* m_probe;
    };

    // stmt_name
    //   class: a statement helper with by-NAME binds only (single string
    // overloads), which enables the by-name gate.
    class stmt_name : public statement<stmt_name>
    {
    public:
        explicit stmt_name(stmt_probe* _probe)
            : m_probe(_probe)
        {}

        void bind_null(const std::string& _n)
        {
            m_probe->last_method = "null";
            m_probe->last_name   = _n;
        }

        void bind_bool(const std::string& _n, bool _v)
        {
            m_probe->last_method = "bool";
            m_probe->last_name   = _n;
            m_probe->v_bool      = _v;
        }

        void bind_int(const std::string& _n, re_std::int32_t _v)
        {
            m_probe->last_method = "int";
            m_probe->last_name   = _n;
            m_probe->v_int       = _v;
        }

        void bind_long(const std::string& _n, re_std::int64_t _v)
        {
            m_probe->last_method = "long";
            m_probe->last_name   = _n;
            m_probe->v_long      = _v;
        }

        void bind_double(const std::string& _n, double _v)
        {
            m_probe->last_method = "double";
            m_probe->last_name   = _n;
            m_probe->v_double    = _v;
        }

        void bind_string(const std::string& _n, const std::string& _v)
        {
            m_probe->last_method = "string";
            m_probe->last_name   = _n;
            m_probe->v_string    = _v;
        }

        void bind_binary(const std::string&               _n,
                         const std::vector<re_std::uint8_t>& _v)
        {
            m_probe->last_method = "binary";
            m_probe->last_name   = _n;
            m_probe->v_binary    = _v;
        }

    private:
        stmt_probe* m_probe;
    };

    // stmt_has_name_bind
    //   trait: true if `.bind_null(const std::string&)` is callable on
    // _Type. All by-name binds share one gate shape, so this stands in for
    // the family.
    template<typename _Type, typename = void>
    struct stmt_has_name_bind : std::false_type
    {};

    template<typename _Type>
    struct stmt_has_name_bind<_Type, std::void_t<decltype(
        std::declval<_Type&>().bind_null(std::declval<const std::string&>()))>>
        : std::true_type
    {};


    // =======================================================================
    // transaction fixtures
    // =======================================================================

    // txn_probe
    //   struct: instrumentation for the transaction mock connections.
    struct txn_probe
    {
        int             begin_calls       = 0;
        int             begin_iso_calls   = 0;
        int             commit_calls      = 0;
        int             rollback_calls    = 0;
        int             savepoint_calls   = 0;
        int             rollback_sp_calls = 0;
        int             release_sp_calls  = 0;
        isolation_level last_iso          = isolation_level::read_uncommitted;
        std::string     last_savepoint;
        bool            rollback_throws   = false;
    };

    // mock_txn_conn
    //   class: a connection stand-in for transaction<> tests, including
    // savepoint support.
    class mock_txn_conn
    {
    public:
        explicit mock_txn_conn(txn_probe* _probe)
            : m_probe(_probe)
        {}

        void begin_transaction()
        {
            ++m_probe->begin_calls;
            m_in_transaction = true;
        }

        void begin_transaction(isolation_level _i)
        {
            ++m_probe->begin_iso_calls;
            m_probe->last_iso = _i;
            m_in_transaction  = true;
        }

        void commit()
        {
            ++m_probe->commit_calls;
            m_in_transaction = false;
        }

        // a rollback that throws leaves the transaction open
        void rollback()
        {
            ++m_probe->rollback_calls;

            if (m_probe->rollback_throws)
            {
                throw transaction_exception("mock rollback failed");
            }

            m_in_transaction = false;
        }

        // asked by transaction<> after a commit or a rollback that failed
        bool in_transaction() const noexcept { return m_in_transaction; }

        void create_savepoint(const std::string& _n)
        {
            ++m_probe->savepoint_calls;
            m_probe->last_savepoint = _n;
        }

        void rollback_to_savepoint(const std::string& _n)
        {
            ++m_probe->rollback_sp_calls;
            m_probe->last_savepoint = _n;
        }

        void release_savepoint(const std::string& _n)
        {
            ++m_probe->release_sp_calls;
            m_probe->last_savepoint = _n;
        }

    private:
        txn_probe* m_probe;
        bool       m_in_transaction = false;
    };

    // mock_txn_conn_nosp
    //   class: a transaction connection stand-in WITHOUT savepoints, used to
    // verify the savepoint gates are disabled.
    class mock_txn_conn_nosp
    {
    public:
        explicit mock_txn_conn_nosp(txn_probe* _probe)
            : m_probe(_probe)
        {}

        void begin_transaction()                 { ++m_probe->begin_calls; }
        void begin_transaction(isolation_level)  { ++m_probe->begin_iso_calls; }
        void commit()                            { ++m_probe->commit_calls; }
        void rollback()                          { ++m_probe->rollback_calls; }

        // asked by transaction<> after a commit or a rollback that failed,
        // which this stand-in never has
        bool in_transaction() const noexcept     { return false; }

    private:
        txn_probe* m_probe;
    };

    // txn_has_savepoint
    //   trait: true if transaction<_Conn>::create_savepoint is enabled.
    template<typename _Type, typename = void>
    struct txn_has_savepoint : std::false_type
    {};

    template<typename _Type>
    struct txn_has_savepoint<_Type, std::void_t<decltype(
        std::declval<_Type&>().create_savepoint(
            std::declval<const std::string&>()))>>
        : std::true_type
    {};


    // =======================================================================
    // connection fixtures
    // =======================================================================

    // conn_probe
    //   struct: instrumentation for the connection mock helper.
    struct conn_probe
    {
        int             connect_calls        = 0;
        int             disconnect_calls     = 0;
        int             reconnect_calls      = 0;
        int             is_connected_calls   = 0;
        int             ping_calls           = 0;
        int             begin_calls          = 0;
        int             begin_iso_calls      = 0;
        int             commit_calls         = 0;
        int             rollback_calls       = 0;
        int             get_iso_calls        = 0;
        int             set_iso_calls        = 0;
        int             savepoint_calls      = 0;
        int             rollback_sp_calls    = 0;
        int             release_sp_calls     = 0;
        int             set_autocommit_calls = 0;
        std::string     last_query;
        std::string     last_savepoint;
        isolation_level last_set_iso         = isolation_level::read_uncommitted;
        bool            last_autocommit      = true;
        bool            is_connected_ret     = true;
        bool            ping_ret             = true;
    };

    // mock_conn
    //   class: a full connection helper implementing every impl method
    // (including all SFINAE-gated ones). begin_transaction uses a default
    // argument so it serves both the no-arg and isolation base paths.
    class mock_conn : public connection<mock_conn>
    {
    public:
        explicit mock_conn(conn_probe* _probe)
            : m_probe(_probe)
        {}

        // connection management
        void connect()    { ++m_probe->connect_calls;    }
        void disconnect() { ++m_probe->disconnect_calls; }
        void reconnect()  { ++m_probe->reconnect_calls;  }

        bool is_connected() const
        {
            ++m_probe->is_connected_calls;
            return m_probe->is_connected_ret;
        }

        bool ping() const
        {
            ++m_probe->ping_calls;
            return m_probe->ping_ret;
        }

        // query execution
        int execute_query(const std::string& _q)
        {
            m_probe->last_query = _q;
            return k_conn_query_val;
        }

        re_std::int64_t execute_update(const std::string& _q)
        {
            m_probe->last_query = _q;
            return k_conn_update_val;
        }

        bool execute(const std::string& _q)
        {
            m_probe->last_query = _q;
            return true;
        }

        int prepare(const std::string& _q)
        {
            m_probe->last_query = _q;
            return k_conn_prepare_val;
        }

        // transactions (default arg serves no-arg + isolation base paths);
        // the base asks the helper whether one is open
        void begin_transaction(isolation_level _i = isolation_level::default_level)
        {
            if (_i == isolation_level::default_level)
            {
                ++m_probe->begin_calls;
            }
            else
            {
                ++m_probe->begin_iso_calls;
                m_probe->last_set_iso = _i;
            }

            m_in_transaction = true;
        }

        void commit()   { ++m_probe->commit_calls;   m_in_transaction = false; }
        void rollback() { ++m_probe->rollback_calls; m_in_transaction = false; }

        bool in_transaction() const noexcept { return m_in_transaction; }

        isolation_level get_isolation_level() const
        {
            ++m_probe->get_iso_calls;
            return isolation_level::repeatable_read;
        }

        void set_isolation_level(isolation_level _l)
        {
            ++m_probe->set_iso_calls;
            m_probe->last_set_iso = _l;
        }

        // savepoints
        void create_savepoint(const std::string& _n)
        {
            ++m_probe->savepoint_calls;
            m_probe->last_savepoint = _n;
        }

        void rollback_to_savepoint(const std::string& _n)
        {
            ++m_probe->rollback_sp_calls;
            m_probe->last_savepoint = _n;
        }

        void release_savepoint(const std::string& _n)
        {
            ++m_probe->release_sp_calls;
            m_probe->last_savepoint = _n;
        }

        // metadata, and the state, which no mock connection leaves
        connection_state get_state() const noexcept
        {
            return connection_state::disconnected;
        }

        database_type get_database_type()   const { return database_type::sqlite; }
        std::string   get_database_name()   const { return "mockdb";              }
        std::string   get_server_version()  const { return "9.9.9";               }
        re_std::int64_t  get_last_insert_id()  const { return k_conn_insert_id;      }
        re_std::int64_t  get_affected_rows()   const { return k_conn_affected;       }

        // configuration; the base asks the helper for auto-commit
        void set_auto_commit(bool _e)
        {
            ++m_probe->set_autocommit_calls;
            m_probe->last_autocommit = _e;
        }

        bool get_auto_commit() const { return m_probe->last_autocommit; }

        // error handling
        std::string get_last_error()      const { return "no error";      }
        int         get_last_error_code() const { return k_conn_error_code; }

        // the hooks the base's gated operations ask for, which
        // database_connection implements its operations over: each does
        // what the operation of its name does here
        void begin_transaction_impl(isolation_level _i)
        {
            begin_transaction(_i);
        }

        isolation_level get_isolation_level_impl() const
        {
            return get_isolation_level();
        }

        void set_isolation_level_impl(isolation_level _l)
        {
            set_isolation_level(_l);
        }

        void create_savepoint_impl(const std::string& _n)
        {
            create_savepoint(_n);
        }

        void rollback_to_savepoint_impl(const std::string& _n)
        {
            rollback_to_savepoint(_n);
        }

        void release_savepoint_impl(const std::string& _n)
        {
            release_savepoint(_n);
        }

        void set_auto_commit_impl(bool _e)
        {
            set_auto_commit(_e);
        }

    private:
        conn_probe* m_probe;
        bool        m_in_transaction = false;
    };

    // mock_conn_minimal
    //   class: a connection helper WITHOUT the optional members, used to
    // verify the isolation, savepoint, and begin_transaction(isolation)
    // gates are disabled.
    class mock_conn_minimal : public connection<mock_conn_minimal>
    {
    public:
        void begin_transaction() { m_in_transaction = true; }

        // the base asks the helper whether a transaction is open
        bool in_transaction() const noexcept { return m_in_transaction; }

    private:
        bool m_in_transaction = false;
    };

    // conn_has_begin_iso
    //   trait: true if `.begin_transaction(isolation_level)` is callable.
    template<typename _Type, typename = void>
    struct conn_has_begin_iso : std::false_type
    {};

    template<typename _Type>
    struct conn_has_begin_iso<_Type, std::void_t<decltype(
        std::declval<_Type&>().begin_transaction(
            std::declval<isolation_level>()))>>
        : std::true_type
    {};

    // conn_has_get_iso
    //   trait: true if `.get_isolation_level()` is callable (const).
    template<typename _Type, typename = void>
    struct conn_has_get_iso : std::false_type
    {};

    template<typename _Type>
    struct conn_has_get_iso<_Type, std::void_t<decltype(
        std::declval<const _Type&>().get_isolation_level())>>
        : std::true_type
    {};

    // conn_has_set_iso
    //   trait: true if `.set_isolation_level(isolation_level)` is callable.
    template<typename _Type, typename = void>
    struct conn_has_set_iso : std::false_type
    {};

    template<typename _Type>
    struct conn_has_set_iso<_Type, std::void_t<decltype(
        std::declval<_Type&>().set_isolation_level(
            std::declval<isolation_level>()))>>
        : std::true_type
    {};

    // conn_has_savepoint
    //   trait: true if `.create_savepoint(const std::string&)` is callable.
    template<typename _Type, typename = void>
    struct conn_has_savepoint : std::false_type
    {};

    template<typename _Type>
    struct conn_has_savepoint<_Type, std::void_t<decltype(
        std::declval<_Type&>().create_savepoint(
            std::declval<const std::string&>()))>>
        : std::true_type
    {};


    // =======================================================================
    // connection_pool / pooled_connection fixtures
    // =======================================================================

    // pool_probe
    //   struct: shared instrumentation for the pooled-connection mocks.
    struct pool_probe
    {
        int  next_id       = 1;      // id generator
        int  live          = 0;      // currently-alive mock connections
        int  created       = 0;      // total constructed
        int  connect_calls = 0;      // total connect() calls
        int  ping_calls    = 0;      // total ping() calls
        bool factory_null  = false;  // factory returns nullptr when true
        int  ping_mode     = 0;      // 0 => true, 1 => false, 2 => throw
    };

    // mock_pool_conn
    //   class: a minimal connection stand-in for connection_pool tests. Only
    // connect() and ping() are exercised by the pool.
    class mock_pool_conn
    {
    public:
        explicit mock_pool_conn(pool_probe* _probe)
            : m_probe(_probe)
            , m_id(_probe ? _probe->next_id++ : 0)
        {
            if (m_probe)
            {
                ++m_probe->live;
                ++m_probe->created;
            }
        }

        ~mock_pool_conn()
        {
            if (m_probe)
            {
                --m_probe->live;
            }
        }

        mock_pool_conn(const mock_pool_conn&)            = delete;
        mock_pool_conn& operator=(const mock_pool_conn&) = delete;

        void connect()
        {
            if (m_probe)
            {
                ++m_probe->connect_calls;
            }

            m_connected = true;
        }

        bool ping() const
        {
            if (m_probe)
            {
                ++m_probe->ping_calls;

                if (m_probe->ping_mode == 2)
                {
                    throw connection_exception("mock ping failed");
                }

                return (m_probe->ping_mode != 1);
            }

            return true;
        }

        int  id()        const noexcept { return m_id;        }
        bool connected() const noexcept { return m_connected; }

    private:
        pool_probe* m_probe;
        int         m_id;
        bool        m_connected = false;
    };

    // make_pool_factory
    //   function: builds a connection factory bound to a pool_probe (returns
    // nullptr while probe->factory_null is set).
    inline std::function<std::unique_ptr<mock_pool_conn>()>
    make_pool_factory(pool_probe* _probe)
    {
        return [_probe]() -> std::unique_ptr<mock_pool_conn>
        {
            if ( (_probe) &&
                 (_probe->factory_null) )
            {
                return nullptr;
            }

            return std::make_unique<mock_pool_conn>(_probe);
        };
    }

    // testable_pool
    //   class: connection_pool subclass exposing the protected helpers so
    // their branches can be exercised directly.
    template<typename _Conn>
    class testable_pool : public connection_pool<_Conn>
    {
    public:
        using connection_pool<_Conn>::connection_pool;
        using connection_pool<_Conn>::create_connection;
        using connection_pool<_Conn>::validate_connection;
        using connection_pool<_Conn>::cleanup_idle_connections;
    };


    // =======================================================================
    // test declarations (flat in djinterp::testing)
    // =======================================================================

    // enumerations + type aliases (database_tests_enums.cpp)
    bool tests_database_type_enum();
    bool tests_isolation_level_enum();
    bool tests_connection_state_enum();
    bool tests_field_type_enum();
    bool tests_store_direction_enum();
    bool tests_value_variant();
    bool tests_type_aliases();

    // exception hierarchy (database_tests_exceptions.cpp)
    bool tests_exception_base();
    bool tests_exception_what_and_code();
    bool tests_connection_exception();
    bool tests_query_exception();
    bool tests_transaction_exception();
    bool tests_type_conversion_exception();
    bool tests_exception_polymorphism();

    // configuration structs (database_tests_config.cpp)
    bool tests_connection_config_defaults();
    bool tests_connection_config_mutation();
    bool tests_pool_config_defaults();
    bool tests_pool_config_mutation();

    // vendor metadata (database_tests_vendor.cpp)
    bool tests_vendor_info_defaults();
    bool tests_vendor_traits_primary_statics();
    bool tests_vendor_traits_get_info();
    bool tests_vendor_traits_make_default_config();

    // result_set (database_tests_result_set.cpp)
    bool tests_result_set_navigation();
    bool tests_result_set_absolute_sfinae();
    bool tests_result_set_metadata();
    bool tests_result_set_access_by_index();
    bool tests_result_set_access_by_name();
    bool tests_result_set_convenience();
    bool tests_result_set_move();

    // statement (database_tests_statement.cpp)
    bool tests_statement_bind_by_index();
    bool tests_statement_bind_by_name();
    bool tests_statement_bind_by_name_sfinae();
    bool tests_statement_execution();
    bool tests_statement_metadata();
    bool tests_statement_move();

    // transaction (database_tests_transaction.cpp)
    bool tests_transaction_begin_commit();
    bool tests_transaction_begin_isolation();
    bool tests_transaction_rollback();
    bool tests_transaction_dtor_rollback();
    bool tests_transaction_inactive_throws();
    bool tests_transaction_move_ctor();
    bool tests_transaction_move_assign();
    bool tests_transaction_savepoints();
    bool tests_transaction_savepoint_sfinae();
    bool tests_transaction_accessors();

    // connection (database_tests_connection.cpp)
    bool tests_connection_initial_state();
    bool tests_connection_lifecycle();
    bool tests_connection_execution();
    bool tests_connection_prepare();
    bool tests_connection_transactions();
    bool tests_connection_isolation_sfinae();
    bool tests_connection_savepoints();
    bool tests_connection_metadata();
    bool tests_connection_config_and_autocommit();
    bool tests_connection_error_handling();
    bool tests_connection_move();
    bool tests_connection_absent_sfinae();

    // connection_pool / pooled_connection (database_tests_pool.cpp)
    bool tests_pool_construction_preallocate();
    bool tests_pool_acquire_from_idle();
    bool tests_pool_acquire_create_new();
    bool tests_pool_acquire_exhausted();
    bool tests_pool_acquire_validate_recreate();
    bool tests_pool_release_basic();
    bool tests_pool_release_null_and_notfound();
    bool tests_pool_release_validate_drop();
    bool tests_pool_release_idle_cap();
    bool tests_pool_clear();
    bool tests_pool_resize_grow_shrink();
    bool tests_pool_counters_and_config();
    bool tests_pool_create_connection_failure();
    bool tests_pool_validate_connection_branches();
    bool tests_pool_cleanup_idle();
    bool tests_pooled_connection_accessors();
    bool tests_pooled_connection_move();
    bool tests_pooled_connection_release_idempotent();

}   // namespace testing
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_DATABASE_TESTS_
