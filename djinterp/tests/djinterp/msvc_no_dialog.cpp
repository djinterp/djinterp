/*******************************************************************************
* djinterp [tests]                                            msvc_no_dialog.cpp
*
*   Linked into every test program MSVC builds (djinterp_add_test_executable),
* so that a test that fails in the C runtime says so on standard error and
* ends, rather than showing a dialog nobody will close: an assertion, an
* invalid parameter or a heap check of the Debug runtime, abort(), and a
* crash Windows would report. Under CI a dialog leaves the program waiting
* until CTest stops it, and its cause unseen.
*   Before main, a static object's constructor:
*   - sends the runtime's warnings, errors and assertions to stderr
*     (_CrtSetReportMode and _CrtSetReportFile; nothing in a release build);
*   - writes the runtime's error messages to stderr (_set_error_mode);
*   - keeps abort() from asking Windows Error Reporting (_set_abort_behavior),
*     its message still written to stderr;
*   - turns off Windows' own boxes for a crash or a missing file
*     (SetErrorMode).
*   Every other compiler compiles this file to nothing.
*
*
* path:      /tests/djinterp/msvc_no_dialog.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.26
*                                                            revised: 2026.11.26
*******************************************************************************/

#if defined(_MSC_VER)

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
    #define NOMINMAX
#endif

// std
#include <crtdbg.h>   // _CrtSetReportMode, _CrtSetReportFile
#include <stdlib.h>   // _set_error_mode, _set_abort_behavior

// windows
#include <windows.h>  // SetErrorMode


namespace
{

    // no_dialog
    //   struct: its one object's constructor, run before main, turns the
    // runtime's and Windows' dialogs into messages on stderr.
    struct no_dialog
    {
        no_dialog()
        {
            ::SetErrorMode(SEM_FAILCRITICALERRORS   |
                           SEM_NOGPFAULTERRORBOX    |
                           SEM_NOOPENFILEERRORBOX);

            _set_error_mode(_OUT_TO_STDERR);
            _set_abort_behavior(0, _CALL_REPORTFAULT);

            _CrtSetReportMode(_CRT_WARN,   _CRTDBG_MODE_FILE);
            _CrtSetReportFile(_CRT_WARN,   _CRTDBG_FILE_STDERR);
            _CrtSetReportMode(_CRT_ERROR,  _CRTDBG_MODE_FILE);
            _CrtSetReportFile(_CRT_ERROR,  _CRTDBG_FILE_STDERR);
            _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
            _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
        }
    };

    no_dialog the_no_dialog;

}  // namespace

#endif  // _MSC_VER
