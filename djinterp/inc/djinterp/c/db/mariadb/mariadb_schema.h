/*******************************************************************************
* djinterp [c]                                                  mariadb_schema.h
*
* Tables, columns, indexes, views and triggers in a MariaDB or MySQL database.
*   Every function takes the database to act in, and NULL means the
* connection's current one. Names are quoted, and match as the server matches
* them: databases, tables and views as its lower_case_table_names setting
* says -- by case where it is 0, as on Linux -- and triggers so too, except on
* MySQL 8, which never matches them by case; column and index names never by
* case.
*   Each kind of object has the same verbs: has, create, drop, list. A create
* or drop is idempotent under D_DB_IF_NOT_EXISTS or D_DB_IF_EXISTS on both
* servers: where one has no IF [NOT] EXISTS for an object -- MySQL for indexes
* and views, and for triggers before 8.0.29 -- the layer asks whether the
* object exists first, which is not atomic, so two connections racing to
* create one may still collide. A name counts as taken as MariaDB's own clause
* counts it: a view's by any table or view. An index name is unique only
* within its table, so the index functions take the table as well. Lists come
* sorted by name, byte by byte.
*   A table can be described column by column with struct d_db_column, which
* maps to the servers' types as section 1 tabulates, or by its definition as
* SQL. Either way it is created with the layer's default character set,
* D_CFG_MARIADB_CHARSET (utf8mb4 unless the build says otherwise).
*   Both servers commit any open transaction before a statement that changes
* the schema; nothing here can be rolled back.
*
*
* path:      /inc/djinterp/c/db/mariadb/mariadb_schema.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.23
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
    2.  Adding, redefining, renaming and dropping
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

#ifndef DJINTERP_C_DB_MARIADB_MARIADB_SCHEMA_H
#define DJINTERP_C_DB_MARIADB_MARIADB_SCHEMA_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../djinterp.h"    // framework root
#include "../db_common.h"      // d_db_column, the lists, D_DB_*
#include "./mariadb_common.h"  // d_mariadb, the layer's results
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
// 1.  TYPES
//==============================================================================


// 1.1    Type mapping
//------------------------------------------------------------------------------
// How a struct d_db_column's type is declared, the same on both servers:
//
//     d_db_type        declared as
//     NONE             refused: both servers need a type; give type_sql
//     BOOLEAN          BOOLEAN, which both store as TINYINT(1)
//     INTEGER          INT
//     BIG_INTEGER      BIGINT
//     FLOATING_POINT   DOUBLE
//     DECIMAL          DECIMAL(size, scale), a size of 0 meaning 10
//     STRING           VARCHAR(size); without a size LONGTEXT, as unbounded
//                      as the other layers' text, or VARCHAR(255) for a key
//                      column -- primary or unique -- which LONGTEXT cannot
//                      be
//     BINARY           VARBINARY(size); without a size LONGBLOB, or
//                      VARBINARY(255) for a key column
//     DATE             DATE
//     TIME, DATETIME   TIME, DATETIME; with a size, TIME(size) and
//                      DATETIME(size), keeping that many digits of a second
//     TIMESTAMP        TIMESTAMP, or TIMESTAMP(size) likewise, declared NULL
//                      unless NOT NULL is asked for, so that no server adds
//                      a default of its own; a NOT NULL one without a default
//                      may still take the server's where its
//                      explicit_defaults_for_timestamp is off
//     JSON             JSON, which MariaDB checks with JSON_VALID
//     XML              LONGTEXT
//     UUID             CHAR(36) CHARACTER SET ascii
//
// AUTO_INCREMENT needs the column to be the whole primary key. `type_sql`,
// when given, is declared as written instead. `default_sql` follows DEFAULT
// as written: a literal, CURRENT_TIMESTAMP, or an expression where the
// server takes one -- MariaDB from 10.2.1, MySQL from 8.0.13, which wants it
// in parentheses.


//==============================================================================
// 2.  TABLES
//==============================================================================


// 2.1    Existence
//------------------------------------------------------------------------------
// existence -- whether the database has a base table of that name (a view is
// not one, nor a temporary table). The plain form answers false for any
// failure; the _s form returns 0 and writes the answer, or the failure, the
// answer then false.
bool   d_mariadb_has_table(struct d_mariadb* _db,
                           const char*       _schema,
                           const char*       _table) D_NOTHROW;
int    d_mariadb_has_table_s(struct d_mariadb* _db,
                             const char*       _schema,
                             const char*       _table,
                             bool*             _exists) D_NOTHROW;

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
 * @param[in] _schema   the database to create it in; `NULL` for the current.
 * @param[in] _table    the table's name.
 * @param[in] _columns  the columns, in order. Several marked
 *                      D_DB_COLUMN_PRIMARY_KEY make a composite key, in the
 *                      order their key_place numbers them, or theirs;
 *                      D_DB_COLUMN_AUTO_INCREMENT is valid only on a sole
 *                      key column.
 * @param[in] _count    how many columns; at least one.
 * @param[in] _flags    0, or D_DB_IF_NOT_EXISTS.
 * @return 0; the client's error number for a failure (1050, the table
 *         exists; 1046, no database is current); CR_OUT_OF_MEMORY (2008); or
 *         D_DB_INVALID_ARGUMENT for a `NULL` argument, an empty name, an
 *         unknown flag or type, a NONE type without type_sql,
 *         AUTO_INCREMENT without a sole primary key, or key places that
 *         skip, repeat, or fall outside the key.
 */
int    d_mariadb_create_table(struct d_mariadb*         _db,
                              const char*               _schema,
                              const char*               _table,
                              const struct d_db_column* _columns,
                              size_t                    _count,
                              unsigned                  _flags) D_NOTHROW;
/**
 * @brief Creates a table from its definition, written as SQL.
 *
 * @param[in] _db          the connection.
 * @param[in] _schema      the database to create it in; `NULL` for the
 *                         current.
 * @param[in] _table       the table's name.
 * @param[in] _definition  what goes between the parentheses of CREATE TABLE:
 *                         column definitions, then table constraints, e.g.
 *                         "id INT PRIMARY KEY, name VARCHAR(80) NOT NULL".
 * @param[in] _flags       0, or D_DB_IF_NOT_EXISTS.
 * @return as d_mariadb_create_table.
 */
int    d_mariadb_create_table_sql(struct d_mariadb* _db,
                                  const char*       _schema,
                                  const char*       _table,
                                  const char*       _definition,
                                  unsigned          _flags) D_NOTHROW;
// drops a table, with its indexes and triggers; D_DB_IF_EXISTS makes a
// missing one a success. 0, the client's error number (1051, unknown table),
// or D_DB_INVALID_ARGUMENT.
int    d_mariadb_drop_table(struct d_mariadb* _db,
                            const char*       _schema,
                            const char*       _table,
                            unsigned          _flags) D_NOTHROW;

// 2.3    Changing
//------------------------------------------------------------------------------
// renames a table within its database. 0, the client's error number (1146, no
// such table; 1050, the new name is taken), or D_DB_INVALID_ARGUMENT.
int    d_mariadb_rename_table(struct d_mariadb* _db,
                              const char*       _schema,
                              const char*       _table,
                              const char*       _new_name) D_NOTHROW;

// 2.4    Rows
//------------------------------------------------------------------------------
// rows -- _truncate_table empties a table with TRUNCATE TABLE, which also
// restarts its AUTO_INCREMENT; _count_rows counts its rows into _count,
// written only on success. 0, the client's error number (1146, no such
// table), or D_DB_INVALID_ARGUMENT.
int    d_mariadb_truncate_table(struct d_mariadb* _db,
                                const char*       _schema,
                                const char*       _table) D_NOTHROW;
int    d_mariadb_count_rows(struct d_mariadb* _db,
                            const char*       _schema,
                            const char*       _table,
                            int64_t*          _count) D_NOTHROW;

// 2.5    Listing
//------------------------------------------------------------------------------
/**
 * @brief Lists a database's base tables, sorted by name.
 *
 * @param[in]  _db      the connection.
 * @param[in]  _schema  the database; `NULL` for the current.
 * @param[out] _names   receives the names; written only on success, empty
 *                      for a database that does not exist.
 * @return 0, the client's error number for a failure, CR_OUT_OF_MEMORY
 *         (2008), or D_DB_INVALID_ARGUMENT for a `NULL` handle or list, or
 *         an empty schema.
 * @post On success the caller releases `_names` with d_db_name_list_free.
 */
int    d_mariadb_list_tables(struct d_mariadb*      _db,
                             const char*            _schema,
                             struct d_db_name_list* _names) D_NOTHROW;


//==============================================================================
// 3.  COLUMNS
//==============================================================================


// 3.1    Existence
//------------------------------------------------------------------------------
// existence -- whether a table has a column of that name. A table that does
// not exist has none. As for tables, the _s form reports a failure.
bool   d_mariadb_has_column(struct d_mariadb* _db,
                            const char*       _schema,
                            const char*       _table,
                            const char*       _column) D_NOTHROW;
int    d_mariadb_has_column_s(struct d_mariadb* _db,
                              const char*       _schema,
                              const char*       _table,
                              const char*       _column,
                              bool*             _exists) D_NOTHROW;

// 3.2    Adding, redefining, renaming and dropping
//------------------------------------------------------------------------------
// adding -- a column described, or written as SQL ("note TEXT"), appended to
// the table. 0, the client's error number (1060, the name is taken; 1146, no
// such table), CR_OUT_OF_MEMORY (2008), or D_DB_INVALID_ARGUMENT, as
// d_mariadb_create_table judges a column.
int    d_mariadb_add_column(struct d_mariadb*         _db,
                            const char*               _schema,
                            const char*               _table,
                            const struct d_db_column* _column) D_NOTHROW;
int    d_mariadb_add_column_sql(struct d_mariadb* _db,
                                const char*       _schema,
                                const char*       _table,
                                const char*       _definition) D_NOTHROW;
// redefining -- the column the description names given that type and those
// attributes, as MODIFY COLUMN gives them: an attribute left out, a default
// or NOT NULL, is dropped, and the column keeps its place and the keys and
// indexes it is part of. D_DB_COLUMN_PRIMARY_KEY and D_DB_COLUMN_UNIQUE
// describe a column that already is a key, choosing a type a key can have
// and allowing AUTO_INCREMENT; neither adds a key. 0, the client's error
// number (1054, no such column; 1146, no such table), CR_OUT_OF_MEMORY
// (2008), or D_DB_INVALID_ARGUMENT, as d_mariadb_add_column judges a column.
int    d_mariadb_modify_column(struct d_mariadb*         _db,
                               const char*               _schema,
                               const char*               _table,
                               const struct d_db_column* _column) D_NOTHROW;
// renaming and dropping -- RENAME COLUMN arrived in MySQL 8.0.3 and MariaDB
// 10.5.2, and an older server answers D_DB_UNSUPPORTED rather than have the
// layer restate the column's whole definition. 0, the client's error number
// (1054 renaming and 1091 dropping a column that is not there; 1090, the
// table's last column), or D_DB_INVALID_ARGUMENT.
int    d_mariadb_rename_column(struct d_mariadb* _db,
                               const char*       _schema,
                               const char*       _table,
                               const char*       _column,
                               const char*       _new_name) D_NOTHROW;
int    d_mariadb_drop_column(struct d_mariadb* _db,
                             const char*       _schema,
                             const char*       _table,
                             const char*       _column) D_NOTHROW;

// 3.3    Listing
//------------------------------------------------------------------------------
/**
 * @brief Describes a table's columns, in order.
 *
 * Each column's `type` is the server's full type (e.g. "varchar(80)",
 * "int(11)" before MySQL 8.0.19 and on MariaDB, "int" after);
 * `default_sql` is the default as SQL -- a string default quoted, NULL for
 * none or for a default of NULL -- as MariaDB reports it, or as the layer
 * renders what MySQL reports, a string's bare value; `primary_key` is the
 * column's 1-based place in the primary key, or 0; `generated` is true for
 * a VIRTUAL or STORED generated column, whose expression is not a default.
 *
 * @param[in]  _db       the connection.
 * @param[in]  _schema   the database; `NULL` for the current.
 * @param[in]  _table    the table.
 * @param[out] _columns  receives the columns; written only on success.
 * @return 0, D_DB_NOT_FOUND for a table that does not exist, the client's
 *         error number for a failure, CR_OUT_OF_MEMORY (2008), or
 *         D_DB_INVALID_ARGUMENT for a `NULL` argument or an empty name.
 * @post On success the caller releases `_columns` with
 *       d_db_column_list_free.
 */
int    d_mariadb_list_columns(struct d_mariadb*        _db,
                              const char*              _schema,
                              const char*              _table,
                              struct d_db_column_list* _columns) D_NOTHROW;


//==============================================================================
// 4.  INDEXES
//==============================================================================


// 4.1    Existence
//------------------------------------------------------------------------------
// existence -- whether a table has an index of that name; its primary key is
// the index named PRIMARY. As for tables, the _s form reports a failure.
bool   d_mariadb_has_index(struct d_mariadb* _db,
                           const char*       _schema,
                           const char*       _index,
                           const char*       _table) D_NOTHROW;
int    d_mariadb_has_index_s(struct d_mariadb* _db,
                             const char*       _schema,
                             const char*       _index,
                             const char*       _table,
                             bool*             _exists) D_NOTHROW;

// 4.2    Creating and dropping
//------------------------------------------------------------------------------
/**
 * @brief Creates an index on a table.
 *
 * @note Six parameters, past the guide's five, because each is a separate
 *       part of the statement, and a struct would only rename them.
 *
 * @param[in] _db       the connection.
 * @param[in] _schema   the database; `NULL` for the current.
 * @param[in] _index    the index's name, unique within its table.
 * @param[in] _table    the table.
 * @param[in] _columns  the key parts as SQL, e.g. "last, first" or
 *                      "name(10)" for a prefix.
 * @param[in] _flags    D_DB_UNIQUE and D_DB_IF_NOT_EXISTS, in any
 *                      combination.
 * @return 0, the client's error number for a failure (1061, the name is
 *         taken; 1062, duplicates under D_DB_UNIQUE), CR_OUT_OF_MEMORY
 *         (2008), or D_DB_INVALID_ARGUMENT for a `NULL` argument, an empty
 *         name or another flag.
 */
int    d_mariadb_create_index(struct d_mariadb* _db,
                              const char*       _schema,
                              const char*       _index,
                              const char*       _table,
                              const char*       _columns,
                              unsigned          _flags) D_NOTHROW;
// drops an index from its table; D_DB_IF_EXISTS makes a missing one a
// success. 0, the client's error number (1091, no such index), or
// D_DB_INVALID_ARGUMENT.
int    d_mariadb_drop_index(struct d_mariadb* _db,
                            const char*       _schema,
                            const char*       _index,
                            const char*       _table,
                            unsigned          _flags) D_NOTHROW;

// 4.3    Listing
//------------------------------------------------------------------------------
// lists a table's indexes, its primary key left out, sorted by name. 0, the
// client's error number, CR_OUT_OF_MEMORY (2008), or D_DB_INVALID_ARGUMENT;
// the caller releases the list with d_db_name_list_free.
int    d_mariadb_list_indexes(struct d_mariadb*      _db,
                              const char*            _schema,
                              const char*            _table,
                              struct d_db_name_list* _names) D_NOTHROW;


//==============================================================================
// 5.  VIEWS
//==============================================================================


// 5.1    Existence
//------------------------------------------------------------------------------
// existence -- whether the database has a view of that name. As for tables,
// the _s form reports a failure.
bool   d_mariadb_has_view(struct d_mariadb* _db,
                          const char*       _schema,
                          const char*       _view) D_NOTHROW;
int    d_mariadb_has_view_s(struct d_mariadb* _db,
                            const char*       _schema,
                            const char*       _view,
                            bool*             _exists) D_NOTHROW;

// 5.2    Creating and dropping
//------------------------------------------------------------------------------
// creating -- CREATE VIEW name AS _select; D_DB_IF_NOT_EXISTS makes an
// existing one a success, whatever its query, and a table of the name too, as
// MariaDB's own clause has it. Dropping -- D_DB_IF_EXISTS makes a missing one
// a success. Each returns 0, the client's error number (1050,
// the name is taken; 4092 or 1051, no such view), CR_OUT_OF_MEMORY (2008), or
// D_DB_INVALID_ARGUMENT.
int    d_mariadb_create_view(struct d_mariadb* _db,
                             const char*       _schema,
                             const char*       _view,
                             const char*       _select,
                             unsigned          _flags) D_NOTHROW;
int    d_mariadb_drop_view(struct d_mariadb* _db,
                           const char*       _schema,
                           const char*       _view,
                           unsigned          _flags) D_NOTHROW;

// 5.3    Listing
//------------------------------------------------------------------------------
// lists a database's views, sorted by name; as d_mariadb_list_tables.
int    d_mariadb_list_views(struct d_mariadb*      _db,
                            const char*            _schema,
                            struct d_db_name_list* _names) D_NOTHROW;


//==============================================================================
// 6.  TRIGGERS
//==============================================================================


// 6.1    Existence
//------------------------------------------------------------------------------
// existence -- whether the database has a trigger of that name. As for
// tables, the _s form reports a failure.
bool   d_mariadb_has_trigger(struct d_mariadb* _db,
                             const char*       _schema,
                             const char*       _trigger) D_NOTHROW;
int    d_mariadb_has_trigger_s(struct d_mariadb* _db,
                               const char*       _schema,
                               const char*       _trigger,
                               bool*             _exists) D_NOTHROW;

// 6.2    Creating and dropping
//------------------------------------------------------------------------------
// creating -- CREATE TRIGGER name _definition, where the definition is the
// rest of the statement, e.g. "AFTER INSERT ON t FOR EACH ROW SET @n = @n +
// 1", its table resolved as the server resolves it: in the current database
// unless qualified, and it must be the trigger's. D_DB_IF_NOT_EXISTS makes an
// existing one a success. Dropping --
// D_DB_IF_EXISTS makes a missing one a success. Each returns 0, the client's
// error number (1359, the name is taken; 1360, no such trigger),
// CR_OUT_OF_MEMORY (2008), or D_DB_INVALID_ARGUMENT.
int    d_mariadb_create_trigger(struct d_mariadb* _db,
                                const char*       _schema,
                                const char*       _trigger,
                                const char*       _definition,
                                unsigned          _flags) D_NOTHROW;
int    d_mariadb_drop_trigger(struct d_mariadb* _db,
                              const char*       _schema,
                              const char*       _trigger,
                              unsigned          _flags) D_NOTHROW;

// 6.3    Listing
//------------------------------------------------------------------------------
// lists a database's triggers, or only those on one table when _table is not
// NULL, sorted by name; as d_mariadb_list_tables.
int    d_mariadb_list_triggers(struct d_mariadb*      _db,
                               const char*            _schema,
                               const char*            _table,
                               struct d_db_name_list* _names) D_NOTHROW;


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_DB_MARIADB_MARIADB_SCHEMA_H
