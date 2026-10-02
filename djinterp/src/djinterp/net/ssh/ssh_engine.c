/*******************************************************************************
* djinterp [net]                                                    ssh_engine.c
*
*   Definitions for ssh_engine.h: the library-wide lifetime calls, which
* reach every compiled engine, and the choice of default engine.
*
*
* path:      /src/djinterp/net/ssh/ssh_engine.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/
#include "../../../../inc/djinterp/net/ssh/ssh_engine.h"  // corresponding header
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../inc/djinterp/net/ssh/ssh_common.h"  // d_ssh_status
#include "../../../../inc/djinterp/net/ssh/ssh_libssh2.h"  // d_ssh_engine_libssh2
#include "../../../../inc/djinterp/net/ssh/ssh_internal.h"  // d_ssh_internal_*


/*
d_ssh_library_init
  Starts every compiled engine; with none compiled, there is nothing to do.
*/
enum d_ssh_status
d_ssh_library_init(void)
{
    return d_ssh_internal_libssh2_startup();
}

/*
d_ssh_library_cleanup
  Balances d_ssh_library_init, engine by engine.
*/
void
d_ssh_library_cleanup(void)
{
    d_ssh_internal_libssh2_shutdown();

    return;
}

/*
d_ssh_engine_default
  The only built-in engine, where it is compiled.
*/
const struct d_ssh_engine_vtable*
d_ssh_engine_default(void)
{
    return d_ssh_engine_libssh2();
}
