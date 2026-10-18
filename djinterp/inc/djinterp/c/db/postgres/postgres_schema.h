/*******************************************************************************
* djinterp [c]                                                 postgres_schema.h
*
* Tables, columns, indexes, views, triggers and sequences in a PostgreSQL
* schema.
*   Every function takes the schema to act in, and NULL means the current one,
* current_schema(): the layer names it in every statement, so a temporary
* table, or one further along the search path, is never acted on by mistake.
* "pg_temp" means the session's temporary schema, as it does in SQL: a table
* created there is a temporary table, gone when the session ends. Names are
* quoted and match exactly, case included.
*   Each kind of object has the same verbs: has, create, drop, list. A create
* or drop is idempotent under D_DB_IF_NOT_EXISTS or D_DB_IF_EXISTS. Where the
* server has the clause -- tables, indexes, sequences, and every drop -- it is
* the server's; for a view or a trigger, which have none, the layer runs the
* CREATE and reads "already exists" as success, under a savepoint inside a
* transaction, so that the transaction survives -- atomic either way. A name
* counts as taken as the server's own clause counts it: a view's by any
* relation (table, view, index, sequence), since they share the schema's
* names. Lists come sorted by name, byte by byte.
*   Index names, too, belong to the schema, so the index functions take the
* schema and the name, as SQLite's do. Trigger names belong to their table,
* so the trigger functions take the table, as MariaDB's index functions do;
* a trigger lives in its table's schema, which its definition names.
*   A table can be described column by column with struct d_db_column, which
* maps to PostgreSQL's types as section 1 tabulates, or by its definition as
* SQL.
*   Statements that change the schema are transactional in PostgreSQL: inside
* a transaction they roll back with it, save those that cannot run in one
* (D_POSTGRES_CONCURRENTLY).
*
*
* path:      /inc/djinterp/c/db/postgres/postgres_schema.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Type mapping
2.  TABLES
    ------
    1.  Existence
    2.  Creating and dropping
    3.  Changing
    4.  Rows
    5.  Listing
3.  COLUMNS
    -------
    1.  Existence
    2.  Adding, renaming and dropping
    3.  Listing
4.  INDEXES
    -------
    1.  Existence
    2.  Creating and dropping
    3.  Listing
5.  VIEWS
    -----
    1.  Existence
    2.  Creating and dropping
    3.  Listing
6.  TRIGGERS
    --------
    1.  Existence
    2.  Creating and dropping
    3.  Listing
7.  SEQUENCES
    ---------
    1.  Existence
    2.  Creating and dropping
    3.  Listing
    4.  Values
*/

#ifndef DJINTERP_C_DB_POSTGRES_POSTGRES_SCHEMA_H
#define DJINTERP_C_DB_POSTGRES_POSTGRES_SCHEMA_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../djinterp.h"     // framework root
#include "../db_common.h"       // d_db_column, the lists, D_DB_*
#include "./postgres_common.h"  // pg_conn, the layer's results and flags
// re_std
#include "../../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's
                                                 // floor; int64_t

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Type mapping
//------------------------------------------------------------------------------
// How a struct d_db_column's type is declared:
//
//     d_db_type        declared as
//     NONE             refused: PostgreSQL needs a type; give type_sql
//     BOOLEAN          boolean
//     INTEGER          integer
//     BIG_INTEGER      bigint
//     FLOATING_POINT   double precision
//     DECIMAL          numeric(size, scale); without a size, numeric, of any
//                      precision and scale, which a scale cannot accompany
//     STRING           varchar(size); without a size text, which has no limit
//                      and can be a key
//     BINARY           bytea, which has no limit; a size is not enforced
//     DATE             date
//     TIME, DATETIME   time, timestamp; with a size, time(size) and
//                      timestamp(size), keeping that many digits of a second,
//                      up to 6
//     TIMESTAMP        timestamptz, or timestamptz(size) likewise: a point in
//                      time, which the server stores in UTC
//     JSON             jsonb, the binary form, which can be indexed
//     XML              xml, which needs a server built with libxml
//     UUID             uuid
//
// AUTO_INCREMENT declares the column GENERATED BY DEFAULT AS IDENTITY, which
// a program can still set by hand, and which needs an integer type; the
// layer, as every layer does, takes it only on the sole primary-key column.
// `type_sql`, when given, is declared as written instead. `default_sql`
// follows DEFAULT as written: a literal, now(), or any expression.


//==============================================================================
// 2.  TABLES
//==============================================================================


// 2.1    Existence
//------------------------------------------------------------------------------
// existence -- whether the schema has a table of that name, ordinary or
// partitioned (a view, a sequence or a foreign table is not one). The plain
// form answers false for any failure; the _s form returns 0 and writes the
// answer, or the failure, the answer then false.
bool   d_postgres_has_table(struct pg_conn* _db,
                            const char*     _schema,
                            const char*     _table) D_NOTHROW;
int    d_postgres_has_table_s(struct pg_conn* _db,
                              const char*     _schema,
                              const char*     _table,
                              bool*           _exists) D_NOTHROW;

// 2.2    Creating and dropping
//------------------------------------------------------------------------------
/**
 * @brief Creates a table from column descriptions.
 *
 * @note Six parameters, past the guide's five, because they are the
 *       statement's own parts; the columns are an array and its length, as C
 *       passes every array.
 *
 * @param[in] _db       the connection.
 * @param[in] _schema   the schema to create it in; `NULL` for the current,
 *                      "pg_temp" for a temporary table.
 * @param[in] _table    the table's name.
 * @param[in] _columns  the columns, in order. Several marked
 *                      D_DB_COLUMN_PRIMARY_KEY make a composite key;
 *                      D_DB_COLUMN_AUTO_INCREMENT is valid only on a sole
 *                      key column.
 * @param[in] _count    how many columns; at least one.
 * @param[in] _flags    0, or D_DB_IF_NOT_EXISTS.
 * @return 0; the SQLSTATE of a failure (42P07, the name is taken; 3F000, no
 *         such schema, or no current one); 53200; or D_DB_INVALID_ARGUMENT
 *         for a `NULL` argument, an empty name, an unknown flag or type, a
 *         NONE type without type_sql, a DECIMAL scale without a size, or
 *         AUTO_INCREMENT without a sole primary key.
 */
int    d_postgres_create_table(struct pg_conn*           _db,
                               const char*               _schema,
                               const char*               _table,
                               const struct d_db_column* _columns,
                               size_t                    _count,
                               unsigned                  _flags) D_NOTHROW;
/**
 * @brief Creates a table from its definition, written as SQL.
 *
 * @param[in] _db          the connection.
 * @param[in] _schema      the schema to create it in; `NULL` for the
 *                         current, "pg_temp" for a temporary table.
 * @param[in] _table       the table's name.
 * @param[in] _definition  what goes between the parentheses of CREATE TABLE:
 *                         column definitions, then table constraints, e.g.
 *                         "id bigint PRIMARY KEY, name text NOT NULL".
 * @param[in] _flags       0, or D_DB_IF_NOT_EXISTS.
 * @return as d_postgres_create_table.
 */
int    d_postgres_create_table_sql(struct pg_conn* _db,
                                   const char*     _schema,
                                   const char*     _table,
                                   const char*     _definition,
                                   unsigned        _flags) D_NOTHROW;
// drops a table, with its indexes and triggers; D_DB_IF_EXISTS makes a
// missing one a success, and D_POSTGRES_CASCADE drops the views and foreign
// keys that depend on it, where without it the server refuses (2BP01). 0,
// the SQLSTATE of a failure (42P01, no such table), or D_DB_INVALID_ARGUMENT.
int    d_postgres_drop_table(struct pg_conn* _db,
                             const char*     _schema,
                             const char*     _table,
                             unsigned        _flags) D_NOTHROW;

// 2.3    Changing
//------------------------------------------------------------------------------
// renames a table within its schema. 0, the SQLSTATE of a failure (42P01, no
// such table; 42P07, the new name is taken), or D_DB_INVALID_ARGUMENT.
int    d_postgres_rename_table(struct pg_conn* _db,
                               const char*     _schema,
                               const char*     _table,
                               const char*     _new_name) D_NOTHROW;

// 2.4    Rows
//------------------------------------------------------------------------------
// rows -- _truncate_table empties a table with TRUNCATE TABLE ... RESTART
// IDENTITY, which also restarts the sequences behind its identity and serial
// columns, and is refused for a table another table's foreign key references
// (0A000); _count_rows counts its rows into _count, written only on success.
// 0, the SQLSTATE of a failure (42P01, no such table), or
// D_DB_INVALID_ARGUMENT.
int    d_postgres_truncate_table(struct pg_conn* _db,
                                 const char*     _schema,
                                 const char*     _table) D_NOTHROW;
int    d_postgres_count_rows(struct pg_conn* _db,
                             const char*     _schema,
                             const char*     _table,
                             int64_t*        _count) D_NOTHROW;

// 2.5    Listing
//------------------------------------------------------------------------------
/**
 * @brief Lists a schema's tables, ordinary and partitioned, sorted by name.
 *
 * @param[in]  _db      the connection.
 * @param[in]  _schema  the schema; `NULL` for the current, "pg_temp" for the
 *                      session's temporary tables.
 * @param[out] _names   receives the names; written only on success, empty
 *                      for a schema that does not exist.
 * @return 0, the SQLSTATE of a failure, 53200, or D_DB_INVALID_ARGUMENT for a
 *         `NULL` connection or list, or an empty schema.
 * @post On success the caller releases `_names` with d_db_name_list_free.
 */
int    d_postgres_list_tables(struct pg_conn*        _db,
                              const char*            _schema,
                              struct d_db_name_list* _names) D_NOTHROW;


//==============================================================================
// 3.  COLUMNS
//==============================================================================


// 3.1    Existence
//------------------------------------------------------------------------------
// existence -- whether a table, or a view, has a column of that name; the
// system's own columns (ctid, xmin, ...) are not counted, and a table that
// does not exist has none. As for tables, the _s form reports a failure.
bool   d_postgres_has_column(struct pg_conn* _db,
                             const char*     _schema,
                             const char*     _table,
                             const char*     _column) D_NOTHROW;
int    d_postgres_has_column_s(struct pg_conn* _db,
                               const char*     _schema,
                               const char*     _table,
                               const char*     _column,
                               bool*           _exists) D_NOTHROW;

// 3.2    Adding, renaming and dropping
//------------------------------------------------------------------------------
// adding -- a column described, or written as SQL ("note text"), appended to
// the table. 0, the SQLSTATE of a failure (42701, the name is taken; 42P01, no
// such table), 53200, or D_DB_INVALID_ARGUMENT, as d_postgres_create_table
// judges a column.
int    d_postgres_add_column(struct pg_conn*           _db,
                             const char*               _schema,
                             const char*               _table,
                             const struct d_db_column* _column) D_NOTHROW;
int    d_postgres_add_column_sql(struct pg_conn* _db,
                                 const char*     _schema,
                                 const char*     _table,
                                 const char*     _definition) D_NOTHROW;
// renaming and dropping -- 0, the SQLSTATE of a failure (42703, no such
// column; 42701, the new name is taken; 2BP01, a view uses it), or
// D_DB_INVALID_ARGUMENT. PostgreSQL lets a table lose its last column.
int    d_postgres_rename_column(struct pg_conn* _db,
                                const char*     _schema,
                                const char*     _table,
                                const char*     _column,
                                const char*     _new_name) D_NOTHROW;
int    d_postgres_drop_column(struct pg_conn* _db,
                              const char*     _schema,
                              const char*     _table,
                              const char*     _column) D_NOTHROW;

// 3.3    Listing
//------------------------------------------------------------------------------
/**
 * @brief Describes a table's columns, or a view's, in order.
 *
 * Each column's `type` is the server's full type, as format_type writes it
 * ("character varying(80)", "timestamp with time zone", "numeric(10,2)");
 * `default_sql` is the default as SQL, as the server keeps it
 * ("'abc'::text", "now()"), and NULL for none, for a default of NULL, for an
 * identity column, whose values come from its sequence, and for a generated
 * one, whose expression is not a default; `primary_key` is the column's
 * 1-based place in the primary key, or 0; `generated` is true for a
 * generated column (PostgreSQL 12), stored or, from 18, virtual.
 *
 * @param[in]  _db       the connection.
 * @param[in]  _schema   the schema; `NULL` for the current.
 * @param[in]  _table    the table or view.
 * @param[out] _columns  receives the columns; written only on success, and
 *                       empty for a table without any.
 * @return 0, D_DB_NOT_FOUND for a table that does not exist, the SQLSTATE of
 *         a failure, 53200, or D_DB_INVALID_ARGUMENT for a `NULL` argument or
 *         an empty name.
 * @post On success the caller releases `_columns` with
 *       d_db_column_list_free.
 */
int    d_postgres_list_columns(struct pg_conn*          _db,
                               const char*              _schema,
                               const char*              _table,
                               struct d_db_column_list* _columns) D_NOTHROW;


//==============================================================================
// 4.  INDEXES
//==============================================================================


// 4.1    Existence
//------------------------------------------------------------------------------
// existence -- whether the schema has an index of that name, those the
// server made for a primary key or a UNIQUE constraint ("t_pkey") included.
// As for tables, the _s form reports a failure.
bool   d_postgres_has_index(struct pg_conn* _db,
                            const char*     _schema,
                            const char*     _index) D_NOTHROW;
int    d_postgres_has_index_s(struct pg_conn* _db,
                              const char*     _schema,
                              const char*     _index,
                              bool*           _exists) D_NOTHROW;

// 4.2    Creating and dropping
//------------------------------------------------------------------------------
/**
 * @brief Creates an index on a table.
 *
 * @note Six parameters, past the guide's five, because each is a separate
 *       part of the statement, and a struct would only rename them.
 *
 * @param[in] _db       the connection.
 * @param[in] _schema   the schema the table is in, which the index joins;
 *                      `NULL` for the current.
 * @param[in] _index    the index's name, unique within the schema.
 * @param[in] _table    the table.
 * @param[in] _columns  the key as SQL, which the layer puts in parentheses:
 *                      "last_name, first_name", "lower(email)"; or, when it
 *                      begins with "(" or the keyword USING, the rest of the
 *                      statement after the table, as written: "USING gin
 *                      (doc)", "(email) WHERE deleted IS NULL",
 *                      "(id) INCLUDE (name)".
 * @param[in] _flags    D_DB_UNIQUE, D_DB_IF_NOT_EXISTS and
 *                      D_POSTGRES_CONCURRENTLY, in any combination.
 * @return 0, the SQLSTATE of a failure (42P07, the name is taken; 23505,
 *         duplicates under D_DB_UNIQUE; 25001, D_POSTGRES_CONCURRENTLY inside
 *         a transaction), 53200, or D_DB_INVALID_ARGUMENT for a `NULL`
 *         argument, an empty name or another flag.
 */
int    d_postgres_create_index(struct pg_conn* _db,
                               const char*     _schema,
                               const char*     _index,
                               const char*     _table,
                               const char*     _columns,
                               unsigned        _flags) D_NOTHROW;
// drops an index; D_DB_IF_EXISTS makes a missing one a success,
// D_POSTGRES_CONCURRENTLY drops it without blocking its table, and
// D_POSTGRES_CASCADE drops what depends on it; the server refuses the last
// two together (0A000), and an index a constraint owns (2BP01). 0, the
// SQLSTATE of a failure (42704, no such index), or D_DB_INVALID_ARGUMENT.
int    d_postgres_drop_index(struct pg_conn* _db,
                             const char*     _schema,
                             const char*     _index,
                             unsigned        _flags) D_NOTHROW;

// 4.3    Listing
//------------------------------------------------------------------------------
// lists a schema's indexes, or one table's when _table is not NULL, sorted by
// name, those a constraint owns -- a primary key's, a UNIQUE or EXCLUDE
// constraint's -- left out, as they are the constraint's to make and drop. 0,
// the SQLSTATE of a failure, 53200, or D_DB_INVALID_ARGUMENT; the caller
// releases the list with d_db_name_list_free.
int    d_postgres_list_indexes(struct pg_conn*        _db,
                               const char*            _schema,
                               const char*            _table,
                               struct d_db_name_list* _names) D_NOTHROW;


//==============================================================================
// 5.  VIEWS
//==============================================================================


// 5.1    Existence
//------------------------------------------------------------------------------
// existence -- whether the schema has a view of that name (a materialized
// view is not one). As for tables, the _s form reports a failure.
bool   d_postgres_has_view(struct pg_conn* _db,
                           const char*     _schema,
                           const char*     _view) D_NOTHROW;
int    d_postgres_has_view_s(struct pg_conn* _db,
                             const char*     _schema,
                             const char*     _view,
                             bool*           _exists) D_NOTHROW;

// 5.2    Creating and dropping
//------------------------------------------------------------------------------
// creating -- CREATE VIEW name AS _select. D_DB_IF_NOT_EXISTS makes a name
// already taken by any relation a success, whatever it is; D_POSTGRES_REPLACE
// replaces a view of the name, as CREATE OR REPLACE VIEW does, which the
// server allows only when the new query keeps the old columns, adding any
// new ones after them. The two flags exclude each other. Dropping --
// D_DB_IF_EXISTS makes a missing one a success, and D_POSTGRES_CASCADE drops
// the views that use it. Each returns 0, the SQLSTATE of a failure (42P07,
// the name is taken; 42P01, no such view), 53200, or D_DB_INVALID_ARGUMENT.
int    d_postgres_create_view(struct pg_conn* _db,
                              const char*     _schema,
                              const char*     _view,
                              const char*     _select,
                              unsigned        _flags) D_NOTHROW;
int    d_postgres_drop_view(struct pg_conn* _db,
                            const char*     _schema,
                            const char*     _view,
                            unsigned        _flags) D_NOTHROW;

// 5.3    Listing
//------------------------------------------------------------------------------
// lists a schema's views, sorted by name; as d_postgres_list_tables.
int    d_postgres_list_views(struct pg_conn*        _db,
                             const char*            _schema,
                             struct d_db_name_list* _names) D_NOTHROW;


//==============================================================================
// 6.  TRIGGERS
//==============================================================================


// 6.1    Existence
//------------------------------------------------------------------------------
// existence -- whether a table in the schema has a trigger of that name; the
// triggers PostgreSQL makes itself, for foreign keys, are not counted. As for
// tables, the _s form reports a failure.
bool   d_postgres_has_trigger(struct pg_conn* _db,
                              const char*     _schema,
                              const char*     _trigger,
                              const char*     _table) D_NOTHROW;
int    d_postgres_has_trigger_s(struct pg_conn* _db,
                                const char*     _schema,
                                const char*     _trigger,
                                const char*     _table,
                                bool*           _exists) D_NOTHROW;

// 6.2    Creating and dropping
//------------------------------------------------------------------------------
/**
 * @brief Creates a trigger.
 *
 * It takes no schema: PostgreSQL puts a trigger in its table's schema, and
 * the definition names the table -- qualified where it is not found along
 * the search path.
 *
 * @param[in] _db          the connection.
 * @param[in] _trigger     the trigger's name, unique on its table.
 * @param[in] _definition  everything after the name, as SQL: the timing and
 *                         events, ON and the table, and the action, e.g.
 *                         "AFTER INSERT ON app.orders FOR EACH ROW EXECUTE
 *                         PROCEDURE audit()" (EXECUTE FUNCTION, its newer
 *                         spelling, from PostgreSQL 11).
 * @param[in] _flags       D_DB_IF_NOT_EXISTS, which the layer settles itself,
 *                         since the server has no clause for it; or
 *                         D_POSTGRES_REPLACE, from PostgreSQL 14; or 0.
 * @return 0; the SQLSTATE of a failure (42710, the name is taken on that
 *         table; 42883, no such function); D_DB_UNSUPPORTED for
 *         D_POSTGRES_REPLACE before PostgreSQL 14; 53200; or
 *         D_DB_INVALID_ARGUMENT for a `NULL` argument, an empty name, both
 *         flags or another.
 */
int    d_postgres_create_trigger(struct pg_conn* _db,
                                 const char*     _trigger,
                                 const char*     _definition,
                                 unsigned        _flags) D_NOTHROW;
// drops a table's trigger; D_DB_IF_EXISTS makes a missing one a success, and
// D_POSTGRES_CASCADE drops what depends on it. 0, the SQLSTATE of a failure
// (42704, no such trigger; 42P01, no such table, which D_DB_IF_EXISTS forgives
// as well), or D_DB_INVALID_ARGUMENT.
int    d_postgres_drop_trigger(struct pg_conn* _db,
                               const char*     _schema,
                               const char*     _trigger,
                               const char*     _table,
                               unsigned        _flags) D_NOTHROW;

// 6.3    Listing
//------------------------------------------------------------------------------
// lists the triggers on a schema's tables, or on one table when _table is not
// NULL, sorted by name, PostgreSQL's own left out. Since PostgreSQL names
// triggers per table, a name appears once for each table that has a trigger
// of that name. As d_postgres_list_tables.
int    d_postgres_list_triggers(struct pg_conn*        _db,
                                const char*            _schema,
                                const char*            _table,
                                struct d_db_name_list* _names) D_NOTHROW;


//==============================================================================
// 7.  SEQUENCES
//==============================================================================


// 7.1    Existence
//------------------------------------------------------------------------------
// existence -- whether the schema has a sequence of that name, those behind
// identity and serial columns included. As for tables, the _s form reports a
// failure.
bool   d_postgres_has_sequence(struct pg_conn* _db,
                               const char*     _schema,
                               const char*     _sequence) D_NOTHROW;
int    d_postgres_has_sequence_s(struct pg_conn* _db,
                                 const char*     _schema,
                                 const char*     _sequence,
                                 bool*           _exists) D_NOTHROW;

// 7.2    Creating and dropping
//------------------------------------------------------------------------------
// creating -- CREATE SEQUENCE without options: a bigint counting up by 1 from
// 1; other options are SQL's, through d_postgres_exec. D_DB_IF_NOT_EXISTS
// makes a name already taken by any relation a success. Dropping --
// D_DB_IF_EXISTS makes a missing one a success, and D_POSTGRES_CASCADE drops
// the defaults that use it. Each returns 0, the SQLSTATE of a failure (42P07,
// the name is taken; 42P01, no such sequence; 2BP01, an identity column owns
// it), 53200, or D_DB_INVALID_ARGUMENT.
int    d_postgres_create_sequence(struct pg_conn* _db,
                                  const char*     _schema,
                                  const char*     _sequence,
                                  unsigned        _flags) D_NOTHROW;
int    d_postgres_drop_sequence(struct pg_conn* _db,
                                const char*     _schema,
                                const char*     _sequence,
                                unsigned        _flags) D_NOTHROW;

// 7.3    Listing
//------------------------------------------------------------------------------
// lists a schema's sequences, sorted by name, those behind identity and
// serial columns included (named by PostgreSQL: orders_id_seq); as
// d_postgres_list_tables.
int    d_postgres_list_sequences(struct pg_conn*        _db,
                                 const char*            _schema,
                                 struct d_db_name_list* _names) D_NOTHROW;

// 7.4    Values
//------------------------------------------------------------------------------
// values -- advances a sequence and writes the value it hands out, as
// nextval() does, never handing it out again, even when the transaction rolls
// back; written only on success. 0, D_DB_NOT_FOUND for a sequence that does
// not exist, the SQLSTATE of a failure (2200H, the sequence has reached its
// limit), or D_DB_INVALID_ARGUMENT.
int    d_postgres_next_value(struct pg_conn* _db,
                             const char*     _schema,
                             const char*     _sequence,
                             int64_t*        _value) D_NOTHROW;
// the value nextval() last handed out in the session, from any sequence --
// an identity or serial column's included -- as lastval() reads it; written
// only on success. 0; D_DB_NOT_FOUND before the session's first; the
// SQLSTATE of a failure, 25P02 in a failed transaction; or
// D_DB_INVALID_ARGUMENT. Inside a transaction it is asked under a savepoint,
// so that the first case leaves the transaction as it was.
int    d_postgres_last_value(struct pg_conn* _db,
                             int64_t*        _value) D_NOTHROW;


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_DB_POSTGRES_POSTGRES_SCHEMA_H
