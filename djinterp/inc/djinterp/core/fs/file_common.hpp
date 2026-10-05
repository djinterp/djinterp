/*******************************************************************************
* djinterp [core]                                                file_common.hpp
*
* Common base for the C++ filesystem layer -- the C++ counterpart to the C
* file_common.h, which it derives from.
*   Every file_*.hpp module includes this one, directly or transitively, for
* the shared foundation: the framework prelude (namespace and the D_*
* qualifier kit), the C common's shared types and constants, and
* djinterp::error, the failure channel the whole layer reports through.
*   error is deliberately small. It wraps an errno-style int and nothing more,
* because that is all the c/fs modules produce: they report through errno
* (D_INTERNAL_FILE_SET_ERR sets it), not through a d_error type -- there is
* none to wrap. `error` gives that raw int a name, a message and a success
* test, so a C++ caller is not passing a bare `int& out` around and
* remembering which sign means trouble.
*   There is NO operator bool. `if (ec)` reads as either "if error" or "if ok"
* depending on who wrote it, and the fs methods already carry success in their
* return value -- so `error` answers only the unambiguous question, failed().
*
*
* path:      /inc/djinterp/core/fs/file_common.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.18
*                                                            revised: 2026.09.29
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  ERROR
    -----
    1.  Error codes
         1.  error
    2.  Comparison
*/

#ifndef DJINTERP_FS_FILE_COMMON_HPP
#define DJINTERP_FS_FILE_COMMON_HPP 1

// std
#include <cerrno>   // errno
#include <cstring>  // std::strerror
// djinterp
#include "../../djinterp.hpp"         // framework root
#include "../../c/fs/file_common.h"   // the C common this layer derives from


NS_DJINTERP


//==============================================================================
// 1.  ERROR
//==============================================================================


// 1.1    Error codes
//------------------------------------------------------------------------------
// 1.1.1
// error
//   class: a typed carrier for an errno-style code; value() == 0 is success.
class error
{
public:
    /**
     * @brief Constructs the success value -- no error.
     */
    error(void)
        : m_code(0)
    {}

    /**
     * @brief Constructs an error carrying a specific errno-style code.
     *
     * @param[in] _code  the code; 0 means success.
     */
    explicit error(
        int _code
    )
        : m_code(_code)
    {}

    /**
     * @brief Reports the raw code.
     *
     * @return 0 for success; anything else is an errno value suitable for
     *         strerror.
     */
    int value(void) const
    {
        return m_code;
    }

    /**
     * @brief Reports whether this carries a failure.
     *
     * @note The name is the whole point: there is no bool conversion to be
     *       read backwards.
     *
     * @return true for any non-zero code.
     */
    bool failed(void) const
    {
        return m_code != 0;
    }

    /**
     * @brief Reports the human-readable text for the code.
     *
     * @return the platform's message; never `NULL`, since strerror answers
     *         unknown codes too.
     */
    const char* message(void) const
    {
        return std::strerror(m_code);
    }

    /**
     * @brief Resets this to success.
     */
    void clear(void)
    {
        m_code = 0;
    }

    /**
     * @brief Sets a specific code, for the fs methods that know the exact
     *        reason (EBADF on a closed handle, EINVAL on an invalid path)
     *        without a live errno to read.
     *
     * @param[in] _code  the code; 0 means success.
     */
    void assign(int _code)
    {
        m_code = _code;
    }

    /**
     * @brief Captures the live errno.
     *
     * @note The fs methods call this the instant a C call has failed, before
     *       anything else can overwrite errno.
     *
     * @return an error carrying the current errno.
     */
    static error from_errno(void)
    {
        return error(errno);
    }

private:
    int m_code;
};

// 1.2    Comparison
//------------------------------------------------------------------------------
/**
 * @brief Compares two errors by code, so `ec == error()` is a success test
 *        and two failures with the same errno are equal.
 *
 * @param[in] _a  the first error.
 * @param[in] _b  the second error.
 * @return true when the codes match.
 */
inline bool
operator==(
    const error& _a,
    const error& _b
)
{
    return _a.value() == _b.value();
}

/**
 * @brief The negation of operator==.
 *
 * @param[in] _a  the first error.
 * @param[in] _b  the second error.
 * @return true when the codes differ.
 */
inline bool
operator!=(
    const error& _a,
    const error& _b
)
{
    return !(_a == _b);
}


NS_END  // djinterp


#endif  // DJINTERP_FS_FILE_COMMON_HPP
