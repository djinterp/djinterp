/*******************************************************************************
* djinterp [net]                                                  ssh_internal.h
*
* SSH module internals: what the module's source files share.
*   No public symbols. Only the sources under src/djinterp/net/ssh/
* include this header, and ssh.h does not; its contents may change in
* any revision.
*
*
* path:      /inc/djinterp/net/ssh/ssh_internal.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.10.12
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES AND CONSTANTS
    -------------------
    1.  Sizes
         1.  D_SSH_INTERNAL_*
    2.  Known hosts
         1.  d_ssh_internal_scan
2.  FUNCTIONS
    ---------
    1.  Bytes and text
    2.  Status
    3.  Known hosts
    4.  Sessions
    5.  Engines
*/

#ifndef DJINTERP_NET_SSH_SSH_INTERNAL_H
#define DJINTERP_NET_SSH_SSH_INTERNAL_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../c/djinterp.h"   // framework root
#include "./ssh_common.h"       // d_ssh_error
#include "./ssh_known_hosts.h"  // d_ssh_host_match
#include "./ssh_session.h"      // d_ssh_session
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // uint32_t, uint64_t


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES AND CONSTANTS
//==============================================================================


// 1.1    Sizes
//------------------------------------------------------------------------------
// 1.1.1
// D_SSH_INTERNAL_*
//   enum: sizes more than one source file relies on.
enum
{
    D_SSH_INTERNAL_HOST_MAX  = 272,  // "[host]:port" and its NUL
    D_SSH_INTERNAL_TYPES_MAX = 256   // key types recorded for a host
};

// 1.2    Known hosts
//------------------------------------------------------------------------------
// 1.2.1
// d_ssh_internal_scan
//   struct: what reading known_hosts files has learned about one host and,
// once its key has arrived, about that key.
struct d_ssh_internal_scan
{
    const char*          name;      // the host as known_hosts records it
    const unsigned char* key;       // the presented key, or NULL
    size_t               key_size;  // its length
    char*                line;      // the line buffer
    unsigned char*       blob;      // the decoded key buffer
    bool                 recorded;  // a plain line names the host
    bool                 found;     // ... with the presented key
    bool                 revoked;   // a @revoked line names the key
    char                 types[D_SSH_INTERNAL_TYPES_MAX];  // types recorded
};


//==============================================================================
// 2.  FUNCTIONS
//==============================================================================


// 2.1    Bytes and text
//------------------------------------------------------------------------------
// bytes and text -- defined in ssh_common.c.
//   The loads and stores are big-endian; d_ssh_internal_name_list_valid
// reports whether `_length` bytes form an RFC 4251 name-list. ASCII case
// folding is net's (net_common.h).
D_NODISCARD uint32_t
d_ssh_internal_load32(const unsigned char* _bytes);
void
d_ssh_internal_store32(unsigned char* _bytes,
                       uint32_t       _value);
void
d_ssh_internal_store64(unsigned char* _bytes,
                       uint64_t       _value);
D_NODISCARD bool
d_ssh_internal_name_list_valid(const char* _list,
                               size_t      _length);

// 2.2    Status
//------------------------------------------------------------------------------
// status -- defined in ssh_common.c.
//   Whether a status leaves a session unusable; only these move a session
// to D_SSH_STATE_FAILED.
D_NODISCARD bool
d_ssh_internal_is_fatal(enum d_ssh_error _status);

// 2.3    Known hosts
//------------------------------------------------------------------------------
// known hosts -- defined in ssh_known_hosts.c.
//   d_ssh_internal_host_valid tells whether a host can be recorded, and
// d_ssh_internal_host_string writes the lowercase name it is recorded
// under. A scan reads any number of files for one host and, when `_key` is
// set, that key: init, then file for each file, then verdict, then free,
// which is also safe after a failed init.
D_NODISCARD bool
d_ssh_internal_host_valid(const char* _host);
D_NODISCARD enum d_ssh_error
d_ssh_internal_host_string(const char*  _host,
                           unsigned int _port,
                           char*        _name,
                           size_t       _capacity);
D_NODISCARD enum d_ssh_error
d_ssh_internal_scan_init(struct d_ssh_internal_scan* _scan,
                         const char*                 _name,
                         const unsigned char*        _key,
                         size_t                      _key_size);
D_NODISCARD enum d_ssh_error
d_ssh_internal_scan_file(struct d_ssh_internal_scan* _scan,
                         const char*                 _path);
D_NODISCARD enum d_ssh_host_match
d_ssh_internal_scan_verdict(const struct d_ssh_internal_scan* _scan);
void
d_ssh_internal_scan_free(struct d_ssh_internal_scan* _scan);

// 2.4    Sessions
//------------------------------------------------------------------------------
// sessions -- defined in ssh_session.c.
//   d_ssh_internal_note records why the last call failed: `_detail`, or
// else the engine's description, copied. d_ssh_internal_fail ends the
// session with `_status`. d_ssh_internal_settle applies an engine result:
// a fatal one fails the session, any other failure is described.
void
d_ssh_internal_note(struct d_ssh_session* _session,
                    const char*           _detail);
enum d_ssh_error
d_ssh_internal_fail(struct d_ssh_session* _session,
                    enum d_ssh_error      _status,
                    const char*           _detail);
enum d_ssh_error
d_ssh_internal_settle(struct d_ssh_session* _session,
                      enum d_ssh_error      _status);

// 2.5    Engines
//------------------------------------------------------------------------------
// engines -- defined in ssh_libssh2.c.
//   Acquire and release libssh2's global state; both do nothing where
// libssh2 is not compiled.
D_NODISCARD enum d_ssh_error
d_ssh_internal_libssh2_startup(void);
void
d_ssh_internal_libssh2_shutdown(void);


D_EXTERN_C_END


#endif  // DJINTERP_NET_SSH_SSH_INTERNAL_H
