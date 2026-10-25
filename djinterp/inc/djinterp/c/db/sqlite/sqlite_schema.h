/*******************************************************************************
* djinterp [c]                                                   sqlite_schema.h
*
* Tables, columns, indexes, views and triggers in an SQLite database.
*   Every function takes the schema to act in -- "main", "temp", or the name a
* database was attached as -- and NULL means "main". Names are quoted, and
* match as SQLite matches them, ignoring ASCII case. The lists leave out the
* objects SQLite makes for itself, whose names begin with "sqlite_".
*   Each kind of object has the same verbs: has, create, drop, list. A create
* or drop is idempotent under D_DB_IF_NOT_EXISTS or D_DB_IF_EXISTS. A table
* can be described column by column with struct d_db_column, which maps to
* SQLite's types as section 1 tabulates, or by its definition as SQL.
*   Changes SQLite added late are checked against the library in use, and
* refused with D_DB_UNSUPPORTED below their release instead of failing as a
* syntax error: STRICT tables (3.37.0), WITHOUT ROWID (3.8.2), RENAME COLUMN
* (3.25.0) and DROP COLUMN (3.35.0).
*
*
* path:      /inc/djinterp/c/db/sqlite/sqlite_schema.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TABLE FLAGS AND TYPES
    ---------------------
    1.  Table flags
         1.  D_SQLITE_STRICT
         2.  D_SQLITE_WITHOUT_ROWID
    2.  Type mapping
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
*/

#ifndef DJINTERP_C_DB_SQLITE_SQLITE_SCHEMA_H
#define DJINTERP_C_DB_SQLITE_SQLITE_SCHEMA_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../djinterp.h"   // framework root
#include "../db_common.h"     // d_db_column, the lists, D_DB_IF_*
#include "./sqlite_common.h"  // sqlite3, the layer's results
// re_std
#include "../../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's
                                                 // floor; int64_t

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict C++98
// wherever int64_t is `long long`: a 32-bit target, 64-bit Windows or macOS --
// the header compiles to nothing (the owner's ruling of 2026.10.03 on round 3's
// question 1, (a)).
#if defined(INT64_MAX)


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TABLE FLAGS AND TYPES
//==============================================================================


// 1.1    Table flags
//------------------------------------------------------------------------------
// 1.1.1
// D_SQLITE_STRICT
//   constant: a STRICT table, whose columns accept only values of their
// declared type (3.37.0).
#define D_SQLITE_STRICT         (D_DB_VENDOR_FLAGS << 0)

// 1.1.2
// D_SQLITE_WITHOUT_ROWID
//   constant: a WITHOUT ROWID table, stored by its primary key, which it must
// therefore have (3.8.2).
#define D_SQLITE_WITHOUT_ROWID  (D_DB_VENDOR_FLAGS << 1)

// 1.2    Type mapping
//------------------------------------------------------------------------------
// How a struct d_db_column's type is declared. SQLite stores by affinity, not
// type, so each maps to the name that gives the right affinity; in a STRICT
// table, to one of the five names STRICT allows.
//
//     d_db_type                 declared as       in a STRICT table
//     NONE                      (no type)         ANY
//     BOOLEAN                   INTEGER           INTEGER
//     INTEGER, BIG_INTEGER      INTEGER           INTEGER
//     FLOATING_POINT            REAL              REAL
//     DECIMAL                   NUMERIC           TEXT, which keeps digits
//     STRING                    TEXT              TEXT
//     BINARY                    BLOB              BLOB
//     DATE, TIME, DATETIME,     TEXT              TEXT
//     TIMESTAMP
//     JSON, XML, UUID           TEXT              TEXT
//
// Both integer types declare INTEGER because only a column declared exactly
// INTEGER PRIMARY KEY becomes the rowid, and only it may AUTOINCREMENT. SQLite
// enforces no length, so `size` and `scale` are not used. `type_sql`, when
// given, is declared as written instead.


//==============================================================================
// 2.  TABLES
//==============================================================================


// 2.1    Existence
//------------------------------------------------------------------------------
// existence -- whether the schema has a table of that name, virtual tables
// included. The plain form answers false for any failure; the _s form returns
// SQLITE_OK and writes the answer, or the result code of the failure, among
// them SQLITE_ERROR for a schema that is not attached.
bool d_sqlite_has_table(struct sqlite3* _db,
                        const char*     _schema,
                        const char*     _table) D_NOTHROW;
int  d_sqlite_has_table_s(struct sqlite3* _db,
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
 * @param[in] _schema   the schema to create it in; `NULL` for "main".
 * @param[in] _table    the table's name.
 * @param[in] _columns  the columns, in order. Several marked
 *                      D_DB_COLUMN_PRIMARY_KEY make a composite key, in the
 *                      order their key_place numbers them, or theirs;
 *                      D_DB_COLUMN_AUTO_INCREMENT is valid only on a sole
 *                      key column.
 * @param[in] _count    how many columns; at least one.
 * @param[in] _flags    D_DB_IF_NOT_EXISTS, D_SQLITE_STRICT and
 *                      D_SQLITE_WITHOUT_ROWID, in any combination.
 * @return SQLITE_OK; D_DB_UNSUPPORTED for a flag the library in use is too
 *         old for; SQLITE_ERROR when the table exists without
 *         D_DB_IF_NOT_EXISTS, or SQLite rejects the definition; SQLITE_NOMEM;
 *         or SQLITE_MISUSE for a `NULL` argument, an empty name, an unknown
 *         flag or type, AUTO_INCREMENT without a sole primary key, or key
 *         places that skip, repeat, or fall outside the key.
 */
int  d_sqlite_create_table(struct sqlite3*           _db,
                           const char*               _schema,
                           const char*               _table,
                           const struct d_db_column* _columns,
                           size_t                    _count,
                           unsigned                  _flags) D_NOTHROW;
/**
 * @brief Creates a table from its definition, written as SQL.
 *
 * @param[in] _db          the connection.
 * @param[in] _schema      the schema to create it in; `NULL` for "main".
 * @param[in] _table       the table's name.
 * @param[in] _definition  what goes between the parentheses of CREATE TABLE:
 *                         column definitions, then table constraints, e.g.
 *                         "id INTEGER PRIMARY KEY, name TEXT NOT NULL".
 * @param[in] _flags       as d_sqlite_create_table.
 * @return as d_sqlite_create_table.
 */
int  d_sqlite_create_table_sql(struct sqlite3* _db,
                               const char*     _schema,
                               const char*     _table,
                               const char*     _definition,
                               unsigned        _flags) D_NOTHROW;
/**
 * @brief Drops a table, with its indexes and triggers.
 *
 * @param[in] _db      the connection.
 * @param[in] _schema  the schema it is in; `NULL` for "main".
 * @param[in] _table   the table's name.
 * @param[in] _flags   D_DB_IF_EXISTS, or 0.
 * @return SQLITE_OK; SQLITE_ERROR when it does not exist without
 *         D_DB_IF_EXISTS; SQLITE_CONSTRAINT when enforced foreign keys refuse
 *         it; another SQLite result code; or SQLITE_MISUSE.
 */
int  d_sqlite_drop_table(struct sqlite3* _db,
                         const char*     _schema,
                         const char*     _table,
                         unsigned        _flags) D_NOTHROW;

// 2.3    Changing
//------------------------------------------------------------------------------
/**
 * @brief Renames a table, within its schema.
 *
 * @param[in] _db        the connection.
 * @param[in] _schema    the schema it is in; `NULL` for "main".
 * @param[in] _table     its current name.
 * @param[in] _new_name  its new name.
 * @return SQLITE_OK, the SQLite result code of the failure, or SQLITE_MISUSE.
 */
int  d_sqlite_rename_table(struct sqlite3* _db,
                           const char*     _schema,
                           const char*     _table,
                           const char*     _new_name) D_NOTHROW;
/**
 * @brief Deletes every row of a table, and restarts its AUTOINCREMENT
 *        sequence, as MySQL's TRUNCATE does.
 *
 * @note The two steps run inside a savepoint, so they happen together or not
 *       at all. After a failure, the result says what failed; sqlite3_errmsg
 *       does not, having seen the rollback.
 *
 * @param[in] _db      the connection.
 * @param[in] _schema  the schema it is in; `NULL` for "main".
 * @param[in] _table   the table's name.
 * @return SQLITE_OK, the SQLite result code of the failure, or SQLITE_MISUSE.
 */
int  d_sqlite_truncate_table(struct sqlite3* _db,
                             const char*     _schema,
                             const char*     _table) D_NOTHROW;

// 2.4    Rows
//------------------------------------------------------------------------------
/**
 * @brief Counts a table's rows.
 *
 * @param[in]  _db      the connection.
 * @param[in]  _schema  the schema it is in; `NULL` for "main".
 * @param[in]  _table   the table's name.
 * @param[out] _count   receives the count; written only on success.
 * @return SQLITE_OK, the SQLite result code of the failure, or SQLITE_MISUSE.
 */
int  d_sqlite_count_rows(struct sqlite3* _db,
                         const char*     _schema,
                         const char*     _table,
                         int64_t*        _count) D_NOTHROW;

// 2.5    Listing
//------------------------------------------------------------------------------
/**
 * @brief Lists a schema's tables, virtual tables included, by name.
 *
 * @param[in]  _db      the connection.
 * @param[in]  _schema  the schema; `NULL` for "main".
 * @param[out] _names   receives the names; written only on success.
 * @return SQLITE_OK, the SQLite result code of the failure, SQLITE_NOMEM, or
 *         SQLITE_MISUSE.
 * @post On success the caller releases `_names` with d_db_name_list_free.
 */
int  d_sqlite_list_tables(struct sqlite3*        _db,
                          const char*            _schema,
                          struct d_db_name_list* _names) D_NOTHROW;


//==============================================================================
// 3.  COLUMNS
//==============================================================================
// Columns are read with PRAGMA table_xinfo where the library has it (3.26.0),
// so generated columns count, and with PRAGMA table_info before that. The
// hidden columns of virtual tables are left out of both.


// 3.1    Existence
//------------------------------------------------------------------------------
// existence -- whether the table has a column of that name; false, not an
// error, when the table itself is missing. As has_table for the two forms.
bool d_sqlite_has_column(struct sqlite3* _db,
                         const char*     _schema,
                         const char*     _table,
                         const char*     _column) D_NOTHROW;
int  d_sqlite_has_column_s(struct sqlite3* _db,
                           const char*     _schema,
                           const char*     _table,
                           const char*     _column,
                           bool*           _exists) D_NOTHROW;

// 3.2    Adding, renaming and dropping
//------------------------------------------------------------------------------
/**
 * @brief Adds a column to a table, from its description.
 *
 * @note SQLite cannot add a column that is a PRIMARY KEY or UNIQUE, nor one
 *       NOT NULL without a DEFAULT; it refuses those with SQLITE_ERROR. The
 *       table's STRICT-ness picks the type mapping, as section 1 shows.
 *
 * @param[in] _db      the connection.
 * @param[in] _schema  the schema it is in; `NULL` for "main".
 * @param[in] _table   the table's name.
 * @param[in] _column  the column.
 * @return SQLITE_OK, the SQLite result code of the failure, SQLITE_NOMEM, or
 *         SQLITE_MISUSE for a `NULL` argument, an empty name or an unknown
 *         type.
 */
int  d_sqlite_add_column(struct sqlite3*           _db,
                         const char*               _schema,
                         const char*               _table,
                         const struct d_db_column* _column) D_NOTHROW;
/**
 * @brief Adds a column to a table, from its definition written as SQL.
 *
 * @param[in] _db          the connection.
 * @param[in] _schema      the schema it is in; `NULL` for "main".
 * @param[in] _table       the table's name.
 * @param[in] _definition  the column definition, e.g. "note TEXT DEFAULT ''".
 * @return SQLITE_OK, the SQLite result code of the failure, or SQLITE_MISUSE.
 */
int  d_sqlite_add_column_sql(struct sqlite3* _db,
                             const char*     _schema,
                             const char*     _table,
                             const char*     _definition) D_NOTHROW;
/**
 * @brief Renames a column, and every reference to it in the schema.
 *
 * @param[in] _db        the connection.
 * @param[in] _schema    the schema it is in; `NULL` for "main".
 * @param[in] _table     the table's name.
 * @param[in] _column    the column's current name.
 * @param[in] _new_name  its new name.
 * @return SQLITE_OK; D_DB_UNSUPPORTED before SQLite 3.25.0; the SQLite result
 *         code of the failure; or SQLITE_MISUSE.
 */
int  d_sqlite_rename_column(struct sqlite3* _db,
                            const char*     _schema,
                            const char*     _table,
                            const char*     _column,
                            const char*     _new_name) D_NOTHROW;
/**
 * @brief Drops a column from a table.
 *
 * @note SQLite refuses, with SQLITE_ERROR, to drop a column that is part of
 *       the primary key, UNIQUE, indexed, or named by a foreign key, a view,
 *       a trigger or a CHECK.
 *
 * @param[in] _db      the connection.
 * @param[in] _schema  the schema it is in; `NULL` for "main".
 * @param[in] _table   the table's name.
 * @param[in] _column  the column's name.
 * @return SQLITE_OK; D_DB_UNSUPPORTED before SQLite 3.35.0; the SQLite result
 *         code of the failure; or SQLITE_MISUSE.
 */
int  d_sqlite_drop_column(struct sqlite3* _db,
                          const char*     _schema,
                          const char*     _table,
                          const char*     _column) D_NOTHROW;

// 3.3    Listing
//------------------------------------------------------------------------------
/**
 * @brief Describes a table's columns, in the table's order.
 *
 * Generated columns (SQLite 3.31.0) are listed, `generated` true, in their
 * place among the others: from 3.26.0 the layer reads table_xinfo, which has
 * them, where table_info leaves them out. A virtual table's hidden columns
 * are not listed.
 *
 * @param[in]  _db       the connection.
 * @param[in]  _schema   the schema it is in; `NULL` for "main".
 * @param[in]  _table    the table's (or view's) name.
 * @param[out] _columns  receives the columns; written only on success.
 * @return SQLITE_OK; D_DB_NOT_FOUND when there is no such table or view; the
 *         SQLite result code of the failure; SQLITE_NOMEM; or SQLITE_MISUSE.
 * @post On success the caller releases `_columns` with d_db_column_list_free.
 */
int  d_sqlite_list_columns(struct sqlite3*          _db,
                           const char*              _schema,
                           const char*              _table,
                           struct d_db_column_list* _columns) D_NOTHROW;


//==============================================================================
// 4.  INDEXES
//==============================================================================


// 4.1    Existence
//------------------------------------------------------------------------------
// existence -- whether the schema has an index of that name. As has_table for
// the two forms.
bool d_sqlite_has_index(struct sqlite3* _db,
                        const char*     _schema,
                        const char*     _index) D_NOTHROW;
int  d_sqlite_has_index_s(struct sqlite3* _db,
                          const char*     _schema,
                          const char*     _index,
                          bool*           _exists) D_NOTHROW;

// 4.2    Creating and dropping
//------------------------------------------------------------------------------
/**
 * @brief Creates an index on a table.
 *
 * @note Six parameters, past the guide's five: they are CREATE INDEX's own
 *       parts.
 *
 * @param[in] _db       the connection.
 * @param[in] _schema   the schema the table is in; `NULL` for "main".
 * @param[in] _index    the index's name.
 * @param[in] _table    the table's name.
 * @param[in] _columns  what goes between the parentheses, as SQL, e.g.
 *                      "last_name, first_name", or "lower(email)" where the
 *                      library indexes expressions
 *                      (D_SQLITE_FEATURE_EXPRESSION_INDEX).
 * @param[in] _flags    D_DB_IF_NOT_EXISTS and D_DB_UNIQUE, in any
 *                      combination.
 * @return SQLITE_OK; SQLITE_ERROR when the index exists without
 *         D_DB_IF_NOT_EXISTS, or SQLite rejects the columns;
 *         SQLITE_CONSTRAINT when D_DB_UNIQUE meets duplicates; or
 *         SQLITE_MISUSE.
 */
int  d_sqlite_create_index(struct sqlite3* _db,
                           const char*     _schema,
                           const char*     _index,
                           const char*     _table,
                           const char*     _columns,
                           unsigned        _flags) D_NOTHROW;
/**
 * @brief Drops an index.
 *
 * @param[in] _db      the connection.
 * @param[in] _schema  the schema it is in; `NULL` for "main".
 * @param[in] _index   the index's name.
 * @param[in] _flags   D_DB_IF_EXISTS, or 0.
 * @return SQLITE_OK; SQLITE_ERROR when it does not exist without
 *         D_DB_IF_EXISTS, or it belongs to a constraint; or SQLITE_MISUSE.
 */
int  d_sqlite_drop_index(struct sqlite3* _db,
                         const char*     _schema,
                         const char*     _index,
                         unsigned        _flags) D_NOTHROW;

// 4.3    Listing
//------------------------------------------------------------------------------
/**
 * @brief Lists a schema's indexes by name, or one table's.
 *
 * @note The indexes SQLite makes for PRIMARY KEY and UNIQUE constraints are
 *       its own, named "sqlite_autoindex_...", and are not listed.
 *
 * @param[in]  _db      the connection.
 * @param[in]  _schema  the schema; `NULL` for "main".
 * @param[in]  _table   the table whose indexes to list; `NULL` for all.
 * @param[out] _names   receives the names; written only on success.
 * @return SQLITE_OK, the SQLite result code of the failure, SQLITE_NOMEM, or
 *         SQLITE_MISUSE.
 * @post On success the caller releases `_names` with d_db_name_list_free.
 */
int  d_sqlite_list_indexes(struct sqlite3*        _db,
                           const char*            _schema,
                           const char*            _table,
                           struct d_db_name_list* _names) D_NOTHROW;


//==============================================================================
// 5.  VIEWS
//==============================================================================


// 5.1    Existence
//------------------------------------------------------------------------------
// existence -- whether the schema has a view of that name. As has_table for
// the two forms.
bool d_sqlite_has_view(struct sqlite3* _db,
                       const char*     _schema,
                       const char*     _view) D_NOTHROW;
int  d_sqlite_has_view_s(struct sqlite3* _db,
                         const char*     _schema,
                         const char*     _view,
                         bool*           _exists) D_NOTHROW;

// 5.2    Creating and dropping
//------------------------------------------------------------------------------
/**
 * @brief Creates a view.
 *
 * @param[in] _db      the connection.
 * @param[in] _schema  the schema to create it in; `NULL` for "main".
 * @param[in] _view    the view's name.
 * @param[in] _select  the SELECT that defines it, as SQL.
 * @param[in] _flags   D_DB_IF_NOT_EXISTS, or 0.
 * @return SQLITE_OK; SQLITE_ERROR when it exists without
 *         D_DB_IF_NOT_EXISTS, or SQLite rejects the SELECT; or SQLITE_MISUSE.
 */
int  d_sqlite_create_view(struct sqlite3* _db,
                          const char*     _schema,
                          const char*     _view,
                          const char*     _select,
                          unsigned        _flags) D_NOTHROW;
/**
 * @brief Drops a view.
 *
 * @param[in] _db      the connection.
 * @param[in] _schema  the schema it is in; `NULL` for "main".
 * @param[in] _view    the view's name.
 * @param[in] _flags   D_DB_IF_EXISTS, or 0.
 * @return SQLITE_OK; SQLITE_ERROR when it does not exist without
 *         D_DB_IF_EXISTS; or SQLITE_MISUSE.
 */
int  d_sqlite_drop_view(struct sqlite3* _db,
                        const char*     _schema,
                        const char*     _view,
                        unsigned        _flags) D_NOTHROW;

// 5.3    Listing
//------------------------------------------------------------------------------
/**
 * @brief Lists a schema's views by name.
 *
 * @param[in]  _db      the connection.
 * @param[in]  _schema  the schema; `NULL` for "main".
 * @param[out] _names   receives the names; written only on success.
 * @return SQLITE_OK, the SQLite result code of the failure, SQLITE_NOMEM, or
 *         SQLITE_MISUSE.
 * @post On success the caller releases `_names` with d_db_name_list_free.
 */
int  d_sqlite_list_views(struct sqlite3*        _db,
                         const char*            _schema,
                         struct d_db_name_list* _names) D_NOTHROW;


//==============================================================================
// 6.  TRIGGERS
//==============================================================================


// 6.1    Existence
//------------------------------------------------------------------------------
// existence -- whether the schema has a trigger of that name. As has_table
// for the two forms.
bool d_sqlite_has_trigger(struct sqlite3* _db,
                          const char*     _schema,
                          const char*     _trigger) D_NOTHROW;
int  d_sqlite_has_trigger_s(struct sqlite3* _db,
                            const char*     _schema,
                            const char*     _trigger,
                            bool*           _exists) D_NOTHROW;

// 6.2    Creating and dropping
//------------------------------------------------------------------------------
/**
 * @brief Creates a trigger.
 *
 * @param[in] _db          the connection.
 * @param[in] _schema      the schema to create it in; `NULL` for "main".
 * @param[in] _trigger     the trigger's name.
 * @param[in] _definition  everything after the name, as SQL, e.g.
 *                         "AFTER INSERT ON t BEGIN UPDATE c SET n = n + 1;
 *                         END".
 * @param[in] _flags       D_DB_IF_NOT_EXISTS, or 0.
 * @return SQLITE_OK; SQLITE_ERROR when it exists without
 *         D_DB_IF_NOT_EXISTS, or SQLite rejects the definition; or
 *         SQLITE_MISUSE.
 */
int  d_sqlite_create_trigger(struct sqlite3* _db,
                             const char*     _schema,
                             const char*     _trigger,
                             const char*     _definition,
                             unsigned        _flags) D_NOTHROW;
/**
 * @brief Drops a trigger.
 *
 * @param[in] _db       the connection.
 * @param[in] _schema   the schema it is in; `NULL` for "main".
 * @param[in] _trigger  the trigger's name.
 * @param[in] _flags    D_DB_IF_EXISTS, or 0.
 * @return SQLITE_OK; SQLITE_ERROR when it does not exist without
 *         D_DB_IF_EXISTS; or SQLITE_MISUSE.
 */
int  d_sqlite_drop_trigger(struct sqlite3* _db,
                           const char*     _schema,
                           const char*     _trigger,
                           unsigned        _flags) D_NOTHROW;

// 6.3    Listing
//------------------------------------------------------------------------------
/**
 * @brief Lists a schema's triggers by name, or one table's.
 *
 * @param[in]  _db      the connection.
 * @param[in]  _schema  the schema; `NULL` for "main".
 * @param[in]  _table   the table whose triggers to list; `NULL` for all.
 * @param[out] _names   receives the names; written only on success.
 * @return SQLITE_OK, the SQLite result code of the failure, SQLITE_NOMEM, or
 *         SQLITE_MISUSE.
 * @post On success the caller releases `_names` with d_db_name_list_free.
 */
int  d_sqlite_list_triggers(struct sqlite3*        _db,
                            const char*            _schema,
                            const char*            _table,
                            struct d_db_name_list* _names) D_NOTHROW;


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_DB_SQLITE_SQLITE_SCHEMA_H
