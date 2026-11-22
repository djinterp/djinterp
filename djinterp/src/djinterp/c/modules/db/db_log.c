/*******************************************************************************
* djinterp [c][db]                                                      db_log.c
*
* db's two tables of codes, defined, and their registration.
*
*
* path:      /src/djinterp/c/modules/db/db_log.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.19
*                                                            revised: 2026.11.19
*******************************************************************************/
#include "../../../../../inc/djinterp/c/modules/db/db_log.h"  // its header


//==============================================================================
// 1.  TABLES
//==============================================================================

// 1.1    The two tables
//------------------------------------------------------------------------------
D_LOG_CODES_DEFINE(db, "db", D_DB_LOG_CODES)
D_LOG_CODES_DEFINE(db_vendor, "db.vendor", D_DB_VENDOR_LOG_CODES)

// 1.2    Registering
//------------------------------------------------------------------------------
/*
d_db_log_add
  Both are tried, so a refusal of one does not keep the other out.
*/
int
d_db_log_add(void)
{
    const int own    = d_log_module_add(&d_log_module_db);
    const int vendor = d_log_module_add(&d_log_module_db_vendor);

    return ( (own == 0) &&
             (vendor == 0) ) ? 0 : -1;
}


//==============================================================================
// 2.  RECORDS
//==============================================================================

// 2.1    A vendor's message
//------------------------------------------------------------------------------
/*
d_db_log_vendor_message
  The knob decides at build time; the run's setting, when db has one, will
decide within what the knob allows.
*/
const char*
d_db_log_vendor_message(
    const char* _message
)
{
#if (D_INTERNAL_DB_LOG_VENDOR_MESSAGE == 1)
    return _message;
#else
    (void)_message;

    return NULL;
#endif  // D_INTERNAL_DB_LOG_VENDOR_MESSAGE
}
