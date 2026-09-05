/******************************************************************************
* djinterp [core]                                              file_common.hpp
*
*   Common base for the C++ filesystem layer -- the C++ counterpart to the C
* file_common.h, which it derives from. Every file_*.hpp module includes this
* one, directly or transitively, for the shared foundation: the framework
* prelude (namespace + the D_* qualifier kit), the C common's shared types and
* constants, and djinterp::error, the failure channel the whole layer reports
* through.
*
*   error is deliberately small. It wraps an errno-style int and nothing more,
* because that is all the c/fs modules produce: they report through errno
* (D_INTERNAL_FILE_SET_ERR sets it), not through a d_error type -- there is none
* to wrap. `error` gives that raw int a name, a message, and a success test, so
* a C++ caller is not passing a bare `int& out` around and remembering which
* sign means trouble.
*
*   There is NO operator bool. `if (ec)` reads as either "if error" or "if ok"
* depending on who wrote it, and the fs methods already carry success in their
* return value -- so `error` answers only the unambiguous question, failed().
*
*
* path:      /inc/djinterp/core/fs/file_common.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.18
******************************************************************************/

#ifndef DJINTERP_FS_FILE_COMMON_
#define DJINTERP_FS_FILE_COMMON_ 1

// std
#include <cerrno>
#include <cstring>
// djinterp
// prelude: namespace + D_* qualifier kit
#include "../../djinterp.hpp"
#include "../../c/fs/file_common.h"   // the C common this layer derives from


NS_DJINTERP

// error
//   class: a typed carrier for an errno-style code. value() == 0 is success.
class error
{
public:
    // error
    //   function: the success value -- no error.
    error(void)
        : m_code(0)
    {}

    // error
    //   function: from a specific errno-style code.
    explicit error(int _code)
        : m_code(_code)
    {}

    // value
    //   function: the raw code. 0 means success; anything else is an errno
    // value suitable for strerror.
    int value(void) const
    {
        return m_code;
    }

    // failed
    //   function: true when this carries a failure. The name is the whole
    // point -- there is no bool conversion to be read backwards.
    bool failed(void) const
    {
        return m_code != 0;
    }

    // message
    //   function: the human-readable text for the code. Never NULL -- strerror
    // returns a string for unknown codes too.
    const char* message(void) const
    {
        return std::strerror(m_code);
    }

    // clear
    //   function: reset to success.
    void clear(void)
    {
        m_code = 0;
    }

    // assign
    //   function: set a specific code, for the fs methods that know the exact
    // reason (EBADF on a closed handle, EINVAL on an invalid path) without a
    // live errno to read.
    void assign(int _code)
    {
        m_code = _code;
    }

    // from_errno
    //   function: capture the live errno. The fs methods call this the instant
    // a C call has failed, before anything else can overwrite it.
    static error from_errno(void)
    {
        return error(errno);
    }

private:
    int m_code;
};


// operator==
//   function: compare by code, so `ec == error()` is a success test and two
// failures with the same errno are equal.
inline bool
operator==(
    const error& _a,
    const error& _b
)
{
    return _a.value() == _b.value();
}

// operator!=
//   function: the negation of operator==.
inline bool
operator!=(
    const error& _a,
    const error& _b
)
{
    return !(_a == _b);
}

NS_END  // djinterp

#endif  // DJINTERP_FS_FILE_COMMON_
