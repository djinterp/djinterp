/*******************************************************************************
* djinterp [c][db]                                                      db_log.h
*
* db's codes for the log: its own results, "db", and the kinds a vendor's
* failure is sorted into, "db.vendor" (the log's decisions 75 to 77).
*   db_common.h holds both tables as text; this header declares them, and
* db_log.c defines them, so that only a program built with the log links
* them. A program that logs db's failures registers the two once, before it
* reads settings, with d_db_log_add; then d_log_code_find finds "db:-3" and
* "db.vendor:3", and a debug build checks their keys. A record of a vendor's
* failure takes its code from the vendor's kind (d_sqlite_kind and the
* others), and carries the vendor's product, code, SQLSTATE and message as
* facts -- the message only as d_db_log_vendor_message allows.
*
* HOW TO USE
* ----------
*     d_db_log_add();
*     ...
*     enum dj_db_vendor_kind kind = d_sqlite_kind(result);
*     const struct dj_log_code* code =
*         d_log_code_find("db.vendor", 9u, (int32_t)kind);
*     const char* message =
*         d_db_log_vendor_message(d_sqlite_error_message(db));
*
* OPTIONS
* -------
*   None yet: the setting db.log.vendor_message, for a run, comes with db's
* settings.
*
* CONFIGURATION
* -------------
*   D_CFG_DB_LOG_VENDOR_MESSAGE (cfg_db_log.h): 0 leaves a vendor's message
* out of every record; 1, the default, lets it in.
*
*
* path:      /inc/djinterp/c/modules/db/db_log.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.19
*                                                            revised: 2026.11.19
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TABLES
    ------
    1.  The two tables
    2.  Registering
2.  RECORDS
    -------
    1.  A vendor's message
*/

#ifndef DJINTERP_C_DB_LOG_H
#define DJINTERP_C_DB_LOG_H 1

// djinterp
#include "../../djinterp.h"                                // framework root
#include "../../log/log.h"                                 // the tables' kit
#include "db_common.h"                                     // the tables, kinds
#include "../../../config/c/modules/db/cfg_db_log.h"       // its knob


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TABLES
//==============================================================================


// 1.1    The two tables
//------------------------------------------------------------------------------
// d_log_module_db and d_log_module_db_vendor, and a code apiece:
// d_log_code_db_not_found, d_log_code_db_vendor_constraint, ...
D_LOG_CODES_DECLARE(db, D_DB_LOG_CODES)
D_LOG_CODES_DECLARE(db_vendor, D_DB_VENDOR_LOG_CODES)

// 1.2    Registering
//------------------------------------------------------------------------------
/**
 * @brief Registers db's two tables with the log, as d_log_module_add does:
 *        once, before settings are read and threads start.
 *
 * @return 0 when both are registered, or already were; -1 when the log
 *         refuses either.
 */
int d_db_log_add(void);


//==============================================================================
// 2.  RECORDS
//==============================================================================


// 2.1    A vendor's message
//------------------------------------------------------------------------------
/**
 * @brief The vendor's message as a record may carry it: itself where the
 *        build lets messages in (D_CFG_DB_LOG_VENDOR_MESSAGE), NULL where
 *        it does not, so that a caller adds the fact only for a message it
 *        is given back.
 *
 * @param[in] _message  the vendor's message; may be NULL.
 * @return `_message`, or NULL.
 */
const char* d_db_log_vendor_message(
    const char* _message
);


D_EXTERN_C_END


#endif  // DJINTERP_C_DB_LOG_H
