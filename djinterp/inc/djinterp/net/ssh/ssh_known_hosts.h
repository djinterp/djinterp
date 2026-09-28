/*******************************************************************************
* djinterp [net]                                               ssh_known_hosts.h
*
* OpenSSH known_hosts files: looking up a host and the key it presents,
* and recording a new one.
*   Plain, wildcard, negated, port-qualified, and hashed entries are all
* understood, as are @revoked markers. Lookup fails closed: a file that
* exists but cannot be read, or holds a line too long to hold, is an
* error, never an empty result.
*
*
* path:      /inc/djinterp/net/ssh/ssh_known_hosts.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Verdicts
         1.  d_ssh_host_match
2.  KNOWN HOSTS
    -----------
    1.  Lookup and recording
    2.  Names
*/

#ifndef DJINTERP_NET_SSH_SSH_KNOWN_HOSTS_H
#define DJINTERP_NET_SSH_SSH_KNOWN_HOSTS_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../c/djinterp.h"  // framework root
#include "./ssh_common.h"      // d_ssh_status


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Verdicts
//------------------------------------------------------------------------------
// 1.1.1
// d_ssh_host_match
//   enum: what the known_hosts files say about a host and the key it
// presented. A host counts as recorded if any line names it with a plain key
// of any type, so a host recorded with an Ed25519 key that presents an RSA key
// has CHANGED -- stricter than OpenSSH, which would treat the RSA key as new
// and, under accept-new, trust it.
enum d_ssh_host_match
{
    D_SSH_HOST_KNOWN   = 0,  // a recorded key equals the presented key
    D_SSH_HOST_UNKNOWN = 1,  // no key is recorded for the host
    D_SSH_HOST_CHANGED = 2,  // keys are recorded, none equal to it
    D_SSH_HOST_REVOKED = 3   // the key is marked @revoked for the host
};


//==============================================================================
// 2.  KNOWN HOSTS
//==============================================================================


// 2.1    Lookup and recording
//------------------------------------------------------------------------------
/**
 * @brief Looks up a host and the key it presented in one known_hosts file.
 *
 * @note a line is `[marker] hosts type key [comment]`. `hosts` is a comma
 *       list of patterns with `*` and `?` wildcards, compared without regard
 *       to case, where a matching `!pattern` excludes the line; or a hashed
 *       `|1|salt|hash` entry. A non-default port is written `[host]:port`.
 *       `\@revoked` marks a key as revoked for the hosts it names;
 *       `\@cert-authority` lines describe certificate authorities and are
 *       ignored, as are comments, blank lines, and malformed lines.
 * @note fails closed: a missing file records nothing, but one that cannot be
 *       read, or holds a line longer than 16 KiB, is an error rather than
 *       something to skip.
 *
 * @param[in]  _path      the file.
 * @param[in]  _host      the host name or address, as connected to.
 * @param[in]  _port      the port, 1 to 65535.
 * @param[in]  _key       the presented key blob.
 * @param[in]  _key_size  its length.
 * @param[out] _match     receives the verdict: REVOKED if any matching line
 *                        revokes the key, else KNOWN if one records it, else
 *                        CHANGED if one records another key, else UNKNOWN.
 * @return `D_SSH_OK`; `D_SSH_ERR_KNOWN_HOSTS` if the file cannot be read;
 *         `D_SSH_ERR_FORMAT` if `_host` holds whitespace or a character
 *         known_hosts reserves (`,*?![]|#`); `D_SSH_ERR_MEMORY`; or
 *         `D_SSH_ERR_ARGUMENT`.
 */
D_NODISCARD enum d_ssh_status
d_ssh_known_hosts_check(const char*            _path,
                        const char*            _host,
                        unsigned int           _port,
                        const unsigned char*   _key,
                        size_t                 _key_size,
                        enum d_ssh_host_match* _match);
/**
 * @brief Appends a host's key to a known_hosts file, creating the file if it
 *        does not exist (but not its directory).
 *
 * @note adds a line break first if the file does not end with one, so the
 *       new entry can never merge into the last line.
 *
 * @param[in] _path      the file.
 * @param[in] _host      the host name or address.
 * @param[in] _port      the port, 1 to 65535.
 * @param[in] _key       the key blob.
 * @param[in] _key_size  its length.
 * @param[in] _hashed    `true` to record the name as a salted HMAC-SHA1
 *                       (`|1|salt|hash`), `false` to record it in clear.
 * @return `D_SSH_OK`; `D_SSH_ERR_KNOWN_HOSTS` if the file cannot be written;
 *         `D_SSH_ERR_FORMAT` if `_host` or the blob is unusable;
 *         `D_SSH_ERR_UNSUPPORTED` or `D_SSH_ERR_IO` if `_hashed` is `true`
 *         and the random source is missing or failed; `D_SSH_ERR_MEMORY`; or
 *         `D_SSH_ERR_ARGUMENT`.
 */
D_NODISCARD enum d_ssh_status
d_ssh_known_hosts_add(const char*          _path,
                      const char*          _host,
                      unsigned int         _port,
                      const unsigned char* _key,
                      size_t               _key_size,
                      bool                 _hashed);

// 2.2    Names
//------------------------------------------------------------------------------
/**
 * @brief Names a known_hosts verdict.
 *
 * @param[in] _match  the verdict.
 * @return a static, human-readable name, or "unknown" for a value outside
 *         the enum.
 */
D_NODISCARD const char*
d_ssh_host_match_string(enum d_ssh_host_match _match);


D_EXTERN_C_END


#endif  // DJINTERP_NET_SSH_SSH_KNOWN_HOSTS_H
