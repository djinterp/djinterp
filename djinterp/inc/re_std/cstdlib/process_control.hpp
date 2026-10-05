/*******************************************************************************
* djinterp [re_std]                                          process_control.hpp
*
* the process-termination functions (re-exports):
*   abort / exit / _Exit / atexit, plus quick_exit / at_quick_exit where
* the runtime has them. Re-exported because each one is a contract with
* the C runtime's startup and teardown machinery -- the registered
* handler lists live there, and a reimplementation would maintain a
* second list the runtime never consults.
*
*   THE FOUR TERMINATION PATHS DIFFER IN WHAT THEY RUN:
*
*     exit        static destructors, then atexit handlers, then flush
*                 and close streams. The orderly path.
*     quick_exit  at_quick_exit handlers only. No static destructors, no
*                 stream flushing. For shutting down when destructor
*                 order is unsafe -- during a crash, or with threads
*                 still running.
*     _Exit       nothing at all. Terminates immediately.
*     abort       nothing, and terminates abnormally (SIGABRT), so a
*                 core dump or debugger trap results.
*
*   Buffered output is LOST on the last three. That is the intended
* behaviour, not an oversight, but it surprises people often enough to
* be worth stating here.
*
*   THE quick_exit PAIR IS GATED:
*   Both are C++11, and both are absent from Apple's libc to this day.
* Where they are missing the names are not declared rather than being
* redirected to _Exit -- silently substituting a function that skips the
* at_quick_exit handlers would turn a compile error into a shutdown that
* quietly does less than the author asked for.
*
*   EXIT_SUCCESS and EXIT_FAILURE are macros and have no re_std::
* spelling; including this header makes them available as <cstdlib> does.
*
*
* path:      /inc/re_std/cstdlib/process_control.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CSTDLIB_PROCESS_CONTROL_HPP
#define RE_STD_CSTDLIB_PROCESS_CONTROL_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstdlib>


// RE_STD_HAS_QUICK_EXIT
//   constant: 1 if std::quick_exit and std::at_quick_exit are declared.
// Overridable for a runtime the checks below do not know about.
#ifndef RE_STD_HAS_QUICK_EXIT
    #if defined(__APPLE__)
        #define RE_STD_HAS_QUICK_EXIT  0
    #else
        #define RE_STD_HAS_QUICK_EXIT  1
    #endif
#endif


namespace re_std
{

    // abort
    //   function: terminate abnormally. No handlers, no flushing.
    using ::std::abort;

    // exit
    //   function: terminate normally -- static destructors, then atexit
    // handlers, then stream teardown.
    using ::std::exit;

    // _Exit
    //   function: terminate immediately, running nothing.
    using ::std::_Exit;

    // atexit
    //   function: register a handler for exit. Handlers run in reverse
    // registration order.
    using ::std::atexit;

#if RE_STD_HAS_QUICK_EXIT

    // quick_exit
    //   function: terminate running only the at_quick_exit handlers.
    using ::std::quick_exit;

    // at_quick_exit
    //   function: register a handler for quick_exit. A separate list from
    // atexit's -- neither path runs the other's handlers.
    using ::std::at_quick_exit;

#endif  // RE_STD_HAS_QUICK_EXIT

}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_CSTDLIB_PROCESS_CONTROL_HPP
