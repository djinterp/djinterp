/*******************************************************************************
* djinterp [net]                                                   ssh_libssh2.h
*
* The built-in libssh2 engine.
*   Compiled where env_ssh.h finds libssh2 1.9.0 or later, the first with
* Ed25519 host keys, and D_CFG_SSH_LIBSSH2 is on; otherwise
* d_ssh_engine_libssh2 returns NULL.
*
*
* path:      /inc/djinterp/net/ssh/ssh_libssh2.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  ENGINE
    ------
    1.  libssh2
*/

#ifndef DJINTERP_NET_SSH_SSH_LIBSSH2_H
#define DJINTERP_NET_SSH_SSH_LIBSSH2_H 1

// djinterp
#include "../../c/djinterp.h"  // framework root
#include "./ssh_engine.h"      // d_ssh_engine_vtable


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  ENGINE
//==============================================================================


// 1.1    libssh2
//------------------------------------------------------------------------------
/**
 * @brief Returns the libssh2 engine.
 *
 * @note host keys arrive through libssh2 but are verified by the session.
 *       libssh2 reports an exit status of 0 when the server sent none.
 * @note a `kex` list keeps libssh2's extension-negotiation and strict key
 *       exchange markers, which replacing its own list would otherwise drop.
 *
 * @return the engine, or `NULL` if D_INTERNAL_SSH_LIBSSH2 is 0.
 */
D_NODISCARD const struct d_ssh_engine_vtable*
d_ssh_engine_libssh2(void);


D_EXTERN_C_END


#endif  // DJINTERP_NET_SSH_SSH_LIBSSH2_H
