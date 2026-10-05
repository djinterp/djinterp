/*******************************************************************************
* djinterp [re_std]                                              environment.hpp
*
* getenv and system (re-exports):
*   The two functions that reach outside the process. Both are re-exported
* because both are the operating system's, not the library's.
*
*   getenv RETURNS A BORROWED, VOLATILE POINTER:
*   The returned string belongs to the environment block. It must not be
* freed or written through, and a later setenv / putenv may invalidate
* it. Callers that keep the value should copy it. getenv is also not
* thread-safe against concurrent modification of the environment.
*
*   system IS A SECURITY BOUNDARY:
*   The argument is handed to a command interpreter, so any part of it
* built from untrusted input is a shell injection. `system(RE_STD_NULLPTR)`
* is the one safe call -- it merely reports whether an interpreter is
* available. There is no portable escaping routine, which is why re_std
* offers none: a helper that looked like it made the call safe would be
* worse than the raw function.
*
*   setenv and putenv are deliberately absent: neither is in the C++
* standard's <cstdlib>, they are POSIX and Windows extensions with
* different signatures and different ownership rules, so surfacing them
* under a re_std:: name would promise portability that does not exist.
*
*
* path:      /inc/re_std/cstdlib/environment.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CSTDLIB_ENVIRONMENT_HPP
#define RE_STD_CSTDLIB_ENVIRONMENT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstdlib>


namespace re_std
{

    // getenv
    //   function: look up an environment variable, or null if unset. The
    // result is borrowed -- see the header comment.
    using ::std::getenv;

    // system
    //   function: run a string through the command interpreter. Never
    // build the argument from untrusted input.
    using ::std::system;

}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_CSTDLIB_ENVIRONMENT_HPP
