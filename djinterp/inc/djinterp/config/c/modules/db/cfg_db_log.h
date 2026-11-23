/*******************************************************************************
* djinterp [c][db]                                                  cfg_db_log.h
*
* Build-time configuration for what db gives the log.
*   A vendor's message can carry a user's data - the value a constraint
* refused, the text of a statement that did not parse - so whether db's
* records of a vendor's failure carry it is a knob (the log's decision 75),
* and a setting of db's for a run, db.log.vendor_message, whose default it
* is. db logs neither statements nor parameters, so with the message left
* out no user data reaches the log through db.
*
*   targets:  db/db_log.h -> D_INTERNAL_DB_LOG_VENDOR_MESSAGE
*   requires: cfg_common.h
*
* HOW TO USE
* ----------
*     cc -DD_CFG_DB_LOG_VENDOR_MESSAGE=0 ...   // no vendor message, ever
*
* OPTIONS
* -------
*   db.log.vendor_message, for a run, within what the knob allows, when db
* has its settings.
*
* CONFIGURATION
* -------------
*   D_CFG_DB_LOG_VENDOR_MESSAGE, below: 1 by default.
*
*
* path:      /inc/djinterp/config/c/modules/db/cfg_db_log.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.19
*                                                            revised: 2026.11.19
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  RECORDS
    -------
    1.  A vendor's message
         1.  D_CFG_DB_LOG_VENDOR_MESSAGE
2.  VALIDATION
    ----------
3.  RESOLVED VALUES
    ---------------
*/

#ifndef DJINTERP_CONFIG_C_DB_CFG_DB_LOG_H
#define DJINTERP_CONFIG_C_DB_CFG_DB_LOG_H 1

// djinterp
#include "../../../cfg_common.h"  // D_CFG_NORM, D_CFG_IS_BOOL


//==============================================================================
// 1.  RECORDS
//==============================================================================


// 1.1    A vendor's message
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_DB_LOG_VENDOR_MESSAGE
//   configuration: 1, the default, lets a record of a vendor's failure carry
// the vendor's message as a fact; 0 leaves it out of every record, whatever
// a run's setting says. The product, code and SQLSTATE are carried either
// way.
#ifndef D_CFG_DB_LOG_VENDOR_MESSAGE
#   define D_CFG_DB_LOG_VENDOR_MESSAGE 1
#endif  // D_CFG_DB_LOG_VENDOR_MESSAGE


//==============================================================================
// 2.  VALIDATION
//==============================================================================

#if !D_CFG_IS_BOOL(D_CFG_DB_LOG_VENDOR_MESSAGE)
#   error "cfg_db_log: D_CFG_DB_LOG_VENDOR_MESSAGE must be 0 or 1"
#endif  // D_CFG_DB_LOG_VENDOR_MESSAGE


//==============================================================================
// 3.  RESOLVED VALUES
//==============================================================================

// D_INTERNAL_DB_LOG_VENDOR_MESSAGE
//   constant: D_CFG_DB_LOG_VENDOR_MESSAGE, as 0 or 1.
#define D_INTERNAL_DB_LOG_VENDOR_MESSAGE                                       \
    D_CFG_NORM(D_CFG_DB_LOG_VENDOR_MESSAGE)


#endif  // DJINTERP_CONFIG_C_DB_CFG_DB_LOG_H
