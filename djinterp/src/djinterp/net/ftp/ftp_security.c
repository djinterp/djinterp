/*******************************************************************************
* djinterp [net]                                                  ftp_security.c
*
* Implementation of the security settings declared in ftp_security.h.
*
*
* path:      /src/djinterp/net/ftp/ftp_security.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/
#include "../../../../inc/djinterp/net/ftp/ftp_security.h"  // corresponding header
// std
#include <stdbool.h>  // bool
// djinterp
#include "./ftp_internal.h"  // shared helpers


//==============================================================================
// 2.  SECURITY
//==============================================================================

/*
d_ftp_protection_from_code
  As d_ftp_structure_from_code().
*/
bool
d_ftp_protection_from_code(
    char                   _code,
    enum d_ftp_protection* _out
)
{
    // parameter validation
    if (!_out)
    {
        return false;
    }

    switch (d_ftp_internal_to_lower(_code))
    {
        case 'c':
            *_out = D_FTP_PROTECTION_CLEAR;

            return true;

        case 's':
            *_out = D_FTP_PROTECTION_SAFE;

            return true;

        case 'e':
            *_out = D_FTP_PROTECTION_CONFIDENTIAL;

            return true;

        case 'p':
            *_out = D_FTP_PROTECTION_PRIVATE;

            return true;

        default:
            break;
    }

    return false;
}
