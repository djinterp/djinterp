/*******************************************************************************
* djinterp [c][net]                                                    net_log.h
*
* net's codes for the log, "net" (the log's decisions 75 to 77).
*   net_common.h holds the table as text, beside dj_net_error; this header
* declares it, and net_log.c defines it, so that only a program built with
* the log links it. net logs nothing itself: a caller that logs a net
* failure registers the table once, before it reads settings, with
* d_net_log_add, and then finds a failure's code by its number, adding the
* facts only it knows -- the peer, the operation.
*
* HOW TO USE
* ----------
*     d_net_log_add();
*     ...
*     const struct dj_log_code* code =
*         d_log_code_find("net", 3u, (int32_t)error);
*
* OPTIONS
* -------
*   None: nothing in this header is chosen at run time.
*
* CONFIGURATION
* -------------
*   None of its own: D_CFG_LOG_CODES (cfg_log.h) decides whether names and
* templates are built.
*
*
* path:      /inc/djinterp/c/modules/net/net_log.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.19
*                                                            revised: 2026.11.19
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE TABLE
    ---------
    1.  Declared
    2.  Registering
*/

#ifndef DJINTERP_C_NET_LOG_H
#define DJINTERP_C_NET_LOG_H 1

// djinterp
#include "../../djinterp.h"   // framework root
#include "../../log/log.h"    // the table's kit
#include "net_common.h"       // the table


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  THE TABLE
//==============================================================================


// 1.1    Declared
//------------------------------------------------------------------------------
// d_log_module_net, and a code apiece: d_log_code_net_closed,
// d_log_code_net_host_not_found, ...
D_LOG_CODES_DECLARE(net, D_NET_LOG_CODES)

// 1.2    Registering
//------------------------------------------------------------------------------
/**
 * @brief Registers net's table with the log, as d_log_module_add does: once,
 *        before settings are read and threads start.
 *
 * @return 0 when it is registered, or already was; -1 when the log refuses
 *         it.
 */
int d_net_log_add(void);


D_EXTERN_C_END


#endif  // DJINTERP_C_NET_LOG_H
