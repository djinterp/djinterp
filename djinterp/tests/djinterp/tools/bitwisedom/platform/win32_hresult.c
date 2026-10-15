/*******************************************************************************
* bitwisedom [tests]                                             win32_hresult.c
*
* The HRESULT example against winerror.h: every field, facility and
* code, and the macros that build them, compared with the platform's
* own at compile time. Built where the build targets Windows.
*
* path:      /tests/djinterp/tools/bitwisedom/platform/win32_hresult.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
*                                                            revised: 2026.10.10
*******************************************************************************/

#include <stdint.h>              // uint32_t
#include <windows.h>             // winerror.h
#include "win32_hresult.h"       // the example, as bitwisedom-gen wrote it


// SAME
//   ours is theirs, compared as the 32 unsigned bits both are: winerror.h's
//   codes are signed HRESULTs, the example's unsigned.
#define SAME(_ours, _theirs)                                                   \
    _Static_assert((uint32_t)(_ours) == (uint32_t)(_theirs),                   \
                   #_ours " is " #_theirs)


// the fields
SAME(WIN_HR_SEVERITY, (unsigned long)SEVERITY_ERROR << 31);
SAME(WIN_HR_NTSTATUS, FACILITY_NT_BIT);
SAME(WIN_HR_FACILITY_GET(E_ACCESSDENIED), HRESULT_FACILITY(E_ACCESSDENIED));
SAME(WIN_HR_CODE_GET(E_ACCESSDENIED), HRESULT_CODE(E_ACCESSDENIED));

// the facilities, in place
SAME(WIN_FACILITY_NULL, (unsigned long)FACILITY_NULL << 16);
SAME(WIN_FACILITY_RPC, (unsigned long)FACILITY_RPC << 16);
SAME(WIN_FACILITY_DISPATCH, (unsigned long)FACILITY_DISPATCH << 16);
SAME(WIN_FACILITY_STORAGE, (unsigned long)FACILITY_STORAGE << 16);
SAME(WIN_FACILITY_ITF, (unsigned long)FACILITY_ITF << 16);
SAME(WIN_FACILITY_WIN32, (unsigned long)FACILITY_WIN32 << 16);
SAME(WIN_FACILITY_WINDOWS, (unsigned long)FACILITY_WINDOWS << 16);
SAME(WIN_FACILITY_SECURITY, (unsigned long)FACILITY_SECURITY << 16);
SAME(WIN_FACILITY_CONTROL, (unsigned long)FACILITY_CONTROL << 16);

// the codes
SAME(WIN_S_OK, S_OK);
SAME(WIN_S_FALSE, S_FALSE);
SAME(WIN_E_NOTIMPL, E_NOTIMPL);
SAME(WIN_E_NOINTERFACE, E_NOINTERFACE);
SAME(WIN_E_POINTER, E_POINTER);
SAME(WIN_E_ABORT, E_ABORT);
SAME(WIN_E_FAIL, E_FAIL);
SAME(WIN_E_UNEXPECTED, E_UNEXPECTED);
SAME(WIN_E_ACCESSDENIED, E_ACCESSDENIED);
SAME(WIN_E_HANDLE, E_HANDLE);
SAME(WIN_E_OUTOFMEMORY, E_OUTOFMEMORY);
SAME(WIN_E_INVALIDARG, E_INVALIDARG);

// the macros, against winerror.h's
SAME(WIN_MAKE_HRESULT(1, FACILITY_WIN32, 5),
     MAKE_HRESULT(1, FACILITY_WIN32, 5));
SAME(WIN_MAKE_HRESULT(0, FACILITY_ITF, 0x200),
     MAKE_HRESULT(0, FACILITY_ITF, 0x200));
_Static_assert(WIN_FAILED(E_FAIL) && !WIN_FAILED(S_FALSE), "FAILED");
_Static_assert(WIN_SUCCEEDED(S_OK) && !WIN_SUCCEEDED(E_FAIL), "SUCCEEDED");

// HRESULT_FROM_WIN32 is an inline function where INLINE_HRESULT_FROM_WIN32
// is defined, and no constant; __HRESULT_FROM_WIN32 is its macro
#if defined(__HRESULT_FROM_WIN32)
SAME(WIN_HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
     __HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
#else
SAME(WIN_HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
     HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
#endif
