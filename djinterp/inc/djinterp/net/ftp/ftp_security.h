/*******************************************************************************
* djinterp [net]                                                  ftp_security.h
*
* FTP security settings (RFC 2228, RFC 4217).
*   How TLS protects the control connection -- not at all, explicitly after
* AUTH TLS, or implicitly from the first byte -- and the PROT levels that
* protect the data connection.
*
*
* path:      /inc/djinterp/net/ftp/ftp_security.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Security
         1.  d_ftp_security
         2.  d_ftp_protection
2.  SECURITY
    --------
    1.  Parameter codes
*/

#ifndef DJINTERP_NET_FTP_FTP_SECURITY_H
#define DJINTERP_NET_FTP_FTP_SECURITY_H 1

// std
// djinterp
#include "../../c/djinterp.h"  // framework root


// Linkage: declared inside D_EXTERN_C_BEGIN / D_EXTERN_C_END so that C++
// translation units link against the C definitions in ftp_security.c.
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Security
//------------------------------------------------------------------------------
// 1.1.1
// d_ftp_security
//   enum: whether and how the control connection is protected by TLS.
enum d_ftp_security
{
    D_FTP_SECURITY_NONE = 0,  // plain FTP: credentials travel in the clear
    D_FTP_SECURITY_EXPLICIT,  // AUTH TLS on the control port (RFC 4217)
    D_FTP_SECURITY_IMPLICIT   // TLS from the first byte, on port 990
};

// 1.1.2
// d_ftp_protection
//   enum: the data-channel protection level named by PROT (RFC 2228); the
// values are the codes. Over TLS only CLEAR and PRIVATE are defined
// (RFC 4217 9).
enum d_ftp_protection
{
    D_FTP_PROTECTION_CLEAR        = 'C',  // no protection
    D_FTP_PROTECTION_SAFE         = 'S',  // integrity only
    D_FTP_PROTECTION_CONFIDENTIAL = 'E',  // confidentiality only
    D_FTP_PROTECTION_PRIVATE      = 'P'   // integrity and confidentiality
};


//==============================================================================
// 2.  SECURITY
//==============================================================================


// 2.1    Parameter codes
//------------------------------------------------------------------------------
// A single-letter PROT argument, read case-insensitively; returns false,
// leaving `_out` untouched, for an unknown code.
bool d_ftp_protection_from_code(char                   _code,
                                enum d_ftp_protection* _out);


D_EXTERN_C_END


#endif  // DJINTERP_NET_FTP_FTP_SECURITY_H
