/*******************************************************************************
* djinterp [c/net]                                                     net_log.c
*
* net's table of codes, defined, and its registration.
*
*
* path:      /src/djinterp/c/modules/net/net_log.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.19
*                                                            revised: 2026.11.19
*******************************************************************************/
#include "../../../../../inc/djinterp/c/modules/net/net_log.h"  // its header


//==============================================================================
// 1.  THE TABLE
//==============================================================================

// 1.1    Defined
//------------------------------------------------------------------------------
D_LOG_CODES_DEFINE(net, "net", D_NET_LOG_CODES)

// 1.2    Registering
//------------------------------------------------------------------------------
/*
d_net_log_add
  d_log_module_add, for net's table.
*/
int
d_net_log_add(void)
{
    return d_log_module_add(&d_log_module_net);
}
