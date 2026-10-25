/*******************************************************************************
* djinterp [env]                                                       env_tls.c
*
*   Runtime implementation of the probe env_tls.h declares: whether a TLS
* library negotiates TLS 1.3 on the system the program runs on. Only
* Schannel's answer needs the running system, which Windows reports through
* RtlGetVersion; every other library's is its build's compile-time flag.
*
*
* path:      /src/djinterp/env/net/tls/env_tls.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.06
*                                                            revised: 2026.10.06
*******************************************************************************/
#include "../../../../../inc/djinterp/env/net/tls/env_tls.h"  // corresponding header

#if D_ENV_TLS_HAS_SCHANNEL
    // windows
    #include <windows.h>  // GetModuleHandleW, GetProcAddress, RTL_OSVERSIONINFOW
#endif


#if D_ENV_TLS_HAS_SCHANNEL

// SCHANNEL_TLS1_3_BUILD
//   constant: the first Windows build whose Schannel negotiates TLS 1.3,
// Windows Server 2022's; Windows 11's is 22000.
static const DWORD SCHANNEL_TLS1_3_BUILD = 20348;

// rtl_get_version_fn
//   type: ntdll's RtlGetVersion, which returns an NTSTATUS, 0 on success.
typedef LONG (WINAPI* rtl_get_version_fn)(PRTL_OSVERSIONINFOW);

/*
d_env_tls_internal_schannel_has_tls1_3
  File-local: asks ntdll's RtlGetVersion for the running release. Unlike
GetVersionEx, it answers truthfully without a compatibility manifest, which
would otherwise make the program see Windows 8. It is looked up rather than
linked, since only the driver kit declares it; the cast goes through a
function type without parameters, which -Wcast-function-type accepts. A
process whose ntdll lacks it is older than any Windows with TLS 1.3.
*/
static int
d_env_tls_internal_schannel_has_tls1_3(void)
{
    const HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");

    // every process maps ntdll; without it there is nothing to ask
    if (!ntdll)
    {
        return 0;
    }

    const rtl_get_version_fn get_version =
        (rtl_get_version_fn)(void (*)(void))GetProcAddress(ntdll,
                                                           "RtlGetVersion");
    RTL_OSVERSIONINFOW info = { 0 };

    info.dwOSVersionInfoSize = sizeof(info);

    // an unanswered question answers no
    if ( (!get_version) ||
         (get_version(&info) != 0) )
    {
        return 0;
    }

    return ( (info.dwMajorVersion > 10) ||
             ( (info.dwMajorVersion == 10) &&
               (info.dwBuildNumber >= SCHANNEL_TLS1_3_BUILD) ) ) ? 1 : 0;
}

#else

/*
d_env_tls_internal_schannel_has_tls1_3
  File-local: Schannel is Windows's alone, and absent here.
*/
static int
d_env_tls_internal_schannel_has_tls1_3(void)
{
    return 0;
}

#endif  // D_ENV_TLS_HAS_SCHANNEL

/*
d_env_tls_backend_has_tls1_3
  Every library but Schannel answers with its build's flag, so a caller asks
one question whichever library it holds.
*/
int
d_env_tls_backend_has_tls1_3(
    int _backend
)
{
    switch (_backend)
    {
        case D_ENV_TLS_BACKEND_OPENSSL:
            return D_ENV_TLS_OPENSSL_HAS_TLS1_3;
        case D_ENV_TLS_BACKEND_MBEDTLS:
            return D_ENV_TLS_MBEDTLS_HAS_TLS1_3;
        case D_ENV_TLS_BACKEND_GNUTLS:
            return D_ENV_TLS_GNUTLS_HAS_TLS1_3;
        case D_ENV_TLS_BACKEND_WOLFSSL:
            return D_ENV_TLS_WOLFSSL_HAS_TLS1_3;
        case D_ENV_TLS_BACKEND_SCHANNEL:
            return d_env_tls_internal_schannel_has_tls1_3();
        case D_ENV_TLS_BACKEND_SECURETRANSPORT:
            return D_ENV_TLS_SECURETRANSPORT_HAS_TLS1_3;
        default:
            return 0;
    }
}
