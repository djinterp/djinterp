/*******************************************************************************
* djinterp [net]                                                      ftp_path.h
*
* FTP pathnames: quoting and resolution.
*   The doubled-quote pathname encoding of 257 replies (RFC 959 Appendix II),
* and resolution of client paths against a working directory, clamped at the
* root so that ".." can never climb out of it.
*
*
* path:      /inc/djinterp/net/ftp/ftp_path.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  PATHS
    -----
    1.  Quoted pathnames
    2.  Resolution
*/

#ifndef DJINTERP_NET_FTP_FTP_PATH_H
#define DJINTERP_NET_FTP_FTP_PATH_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../c/djinterp.h"  // framework root
#include "./ftp_common.h"      // d_ftp_error, d_ftp_buffer


// Linkage: declared inside D_EXTERN_C_BEGIN / D_EXTERN_C_END so that C++
// translation units link against the C definitions in ftp_path.c.
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  PATHS
//==============================================================================


// 1.1    Quoted pathnames
//------------------------------------------------------------------------------
/**
 * @brief Appends a pathname quoted for a 257 reply, with each embedded '"'
 *        doubled (RFC 959 Appendix II).
 *
 * @param[in]     _path the pathname; must not contain CR or LF.
 * @param[in,out] _out  the buffer to append to.
 * @return D_FTP_OK, D_FTP_ERROR_INVALID_ARGUMENT, or
 *         D_FTP_ERROR_BUFFER_TOO_SMALL.
 */
enum d_ftp_error d_ftp_pathname_quote(const char*          _path,
                                      struct d_ftp_buffer* _out);
/**
 * @brief Appends the pathname from the text of a 257 reply, undoubling
 *        embedded quotes.
 *
 * @param[in]     _text   the reply text, e.g. "\"/pub\" is current".
 * @param[in]     _length its length in bytes.
 * @param[in,out] _out    the buffer to append to.
 * @return D_FTP_OK, D_FTP_ERROR_INVALID_ARGUMENT, D_FTP_ERROR_MALFORMED when
 *         no complete quoted name is present, or
 *         D_FTP_ERROR_BUFFER_TOO_SMALL.
 */
enum d_ftp_error d_ftp_pathname_unquote(const char*          _text,
                                        size_t               _length,
                                        struct d_ftp_buffer* _out);

// 1.2    Resolution
//------------------------------------------------------------------------------
/**
 * @brief Resolves a path against a working directory into a normalized
 *        absolute path.
 *
 * Only '/' separates (the TVFS convention, RFC 3659 6). Empty and "."
 * segments vanish, and ".." removes the previous segment but never climbs
 * above "/", so a server mapping "/" onto a root directory cannot be walked
 * out of it. The result has no trailing '/', except "/" itself.
 *
 * @param[in]     _base the working directory; NULL or "" means "/".
 *                      Ignored when `_path` is absolute.
 * @param[in]     _path the path to resolve.
 * @param[in,out] _out  the buffer to append to.
 * @return D_FTP_OK, D_FTP_ERROR_INVALID_ARGUMENT (including CR or LF in
 *         either path), or D_FTP_ERROR_BUFFER_TOO_SMALL.
 */
enum d_ftp_error d_ftp_path_resolve(const char*          _base,
                                    const char*          _path,
                                    struct d_ftp_buffer* _out);


D_EXTERN_C_END


#endif  // DJINTERP_NET_FTP_FTP_PATH_H
