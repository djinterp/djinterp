/*******************************************************************************
* djinterp [c][log]                                                  log_codes.h
*
*   The code tables of the modules below the log (the log's decision 87):
* the stream's, compress's and archive's first, and every other module the
* log is built on. Such a module never logs (decision 67); it returns its
* codes, and its table's rows sit beside its enum, in its own header, as a
* macro that includes nothing of the log's (log.h, 2.4). The two lines that
* make the rows a table the log can register, D_LOG_CODES_DECLARE and
* D_LOG_CODES_DEFINE, are the log's: a header and a unit a module, under
* log/codes/, built wherever the log is, since the modules below it are in
* the runtime.
*   The log registers every such table itself, before settings are read, so
* log.<module>.level can be set and d_log_code_find finds the module's codes
* in every program that has both. No module includes a header above it, and
* no caller has to remember to register.
*   A table lands in its own lane's header; its two lines, its row in the
* log's list and its unit in log_codes.cmake come in the same window.
*
* HOW TO USE
* ----------
*   A caller of a module below the log that logs one of its codes includes
* that module's header here, log/codes/<module>_codes.h, and logs the code as
* any module's: D_LOG(d_log_code_ostream_<name>, ...); or finds it by the
* number the module returned, d_log_code_find("ostream", 7u, number). The
* log registers the tables from d_log_register and from d_log_init; a
* program that wants them sooner calls d_log_codes_add.
*
* OPTIONS
* -------
*   log.<module>.level and the log's other options by module, for each
* table registered (log_design.html).
*
* CONFIGURATION
* -------------
*   D_CFG_LOG_CODES_BELOW (cfg_log.h): 1, the default, has the log register
* them; 0 leaves them to the program, through d_log_codes_add.
* D_CFG_LOG_MODULES_MAX counts them with every other table, and
* D_CFG_<MODULE>_LOG_CODES builds a module's names and templates or not.
*
*
* path:      /inc/djinterp/c/log/log_codes.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.25
*                                                            revised: 2026.11.25
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE TABLES BELOW THE LOG
    ------------------------
    1.  d_log_codes_add
*/

#ifndef DJINTERP_C_LOG_CODES_H
#define DJINTERP_C_LOG_CODES_H 1

// djinterp
#include "../djinterp.h"  // framework root
#include "./log.h"        // dj_log_module, d_log_module_add


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  THE TABLES BELOW THE LOG
//==============================================================================

/**
 * @brief Registers the code table of every module below the log built into
 *        the program, as d_log_module_add does each: once, a second call
 *        changing nothing, and each module a scope of the log's djin.
 *        Register before reading settings.
 *
 * @note The log calls it from d_log_register and d_log_init where
 *       D_CFG_LOG_CODES_BELOW is 1. None has landed yet: the stream's,
 *       compress's and archive's come in the window their tables land.
 *
 * @return 0 when every table is registered; -1 when one was refused (a
 *         number twice, a name another module's, or no room under
 *         D_CFG_LOG_MODULES_MAX), after the rest were tried.
 */
int d_log_codes_add(void);


D_EXTERN_C_END


#endif  // DJINTERP_C_LOG_CODES_H
