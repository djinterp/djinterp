/***********************************************************************
* re_std                                                    terminate.hpp
*
* the terminate facility:
*   terminate_handler, terminate(), set_terminate(), get_terminate().
* terminate() and the handler slot are runtime-provided, so the common
* path re-exports the std symbols. Two portability moves:
*   - get_terminate() was added to std in C++11. re_std back-ports it to
*     C++98 by shadowing the value passed through re_std::set_terminate
*     (same shadow scheme as <new>'s get_new_handler back-port). Caveat:
*     the shadow only sees handlers installed via re_std::set_terminate;
*     a direct std::set_terminate call is invisible to it.
*   - when <exception> is unavailable (freestanding) the whole facility
*     is reimplemented over a function-local-static handler slot, with
*     terminate() invoking the slot (default: abort()).
*
*
* path:      /inc/djinterp/re_std/exception/terminate.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.06.04
***********************************************************************/

#ifndef DJINTERP_RE_STD_EXCEPTION_TERMINATE_
#define DJINTERP_RE_STD_EXCEPTION_TERMINATE_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_CPP98_HAS_EXCEPTION

    #include <exception>

namespace re_std
{
    // terminate_handler
    //   typedef: using-declaration from std::terminate_handler
    //   (void (*)()).
    using std::terminate_handler;

    // terminate
    //   function: using-declaration from std::terminate. [[noreturn]].
    using std::terminate;

    #if D_ENV_LANG_IS_CPP11_OR_HIGHER

        // set_terminate
        //   function: using-declaration from std::set_terminate.
        using std::set_terminate;

        // get_terminate
        //   function: using-declaration from std::get_terminate (C++11+).
        using std::get_terminate;

    #else // C++98/03: get_terminate not yet in std -> shadow back-port

namespace internal
{
        // terminate_shadow
        //   function: function-local-static slot holding the last handler
        //   installed through re_std::set_terminate. Returned by the
        //   re_std::get_terminate back-port.
        inline terminate_handler& terminate_shadow()
        {
            static terminate_handler s_handler = 0;
            return s_handler;
        }

} // namespace internal

        // set_terminate
        //   function: records the handler in the shadow slot, then
        //   delegates to std::set_terminate. Returns the previous handler.
        inline terminate_handler set_terminate(terminate_handler _h) D_NOEXCEPT
        {
            internal::terminate_shadow() = _h;
            return std::set_terminate(_h);
        }

        // get_terminate
        //   function: back-port returning the shadowed handler (the value
        //   last passed to re_std::set_terminate). RE_STD AHEAD OF STD:
        //   surfaced on C++98, where std::get_terminate does not exist.
        inline terminate_handler get_terminate() D_NOEXCEPT
        {
            return internal::terminate_shadow();
        }

    #endif // D_ENV_LANG_IS_CPP11_OR_HIGHER

} // namespace re_std

#else // freestanding: reimplement the facility over a local handler slot

    #include <cstdlib> // abort

namespace re_std
{
    // terminate_handler
    //   typedef: pointer to a no-arg, no-return handler.
    typedef void (*terminate_handler)();

namespace internal
{
    // default_terminate
    //   function: fallback handler — aborts the process.
    inline void default_terminate()
    {
        abort();
    }

    // terminate_shadow
    //   function: function-local-static handler slot.
    inline terminate_handler& terminate_shadow()
    {
        static terminate_handler s_handler = &default_terminate;
        return s_handler;
    }

} // namespace internal

    // set_terminate
    //   function: installs a handler, returns the previous one.
    inline terminate_handler set_terminate(terminate_handler _h) D_NOEXCEPT
    {
        terminate_handler prev = internal::terminate_shadow();
        internal::terminate_shadow() = (_h != 0) ? _h : &internal::default_terminate;
        return prev;
    }

    // get_terminate
    //   function: returns the currently installed handler.
    inline terminate_handler get_terminate() D_NOEXCEPT
    {
        return internal::terminate_shadow();
    }

    // terminate
    //   function: invokes the current handler; aborts if it returns.
    inline void terminate() D_NOEXCEPT
    {
        terminate_handler h = internal::terminate_shadow();
        if (h != 0)
        {
            h();
        }
        abort();
    }

} // namespace re_std

#endif // D_ENV_CPP98_HAS_EXCEPTION

#endif  // DJINTERP_RE_STD_EXCEPTION_TERMINATE_
