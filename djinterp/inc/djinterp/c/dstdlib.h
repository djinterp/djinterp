/*******************************************************************************
* djinterp [c]                                                         dstdlib.h
*
* Cross-platform variants of certain `stdlib.h` functions: the environment.
*   A variable's value is copied out, into the caller's buffer or into storage
* the caller frees, never handed back as the runtime's own pointer, which
* the next change to the environment may free. The Microsoft runtime refuses
* getenv under /sdl; there these are getenv_s, _dupenv_s and _putenv_s, and
* elsewhere getenv, setenv and unsetenv. Every function here says what went
* wrong as an errno code, the same on every platform.
*   A variable is set with a value, or not set: a value of "" removes it, as
* the Microsoft runtime must, so a program reads the same environment on
* every platform. None of these is safe against another thread's change to
* the environment, as the runtime's own are not.
*
* HOW TO USE
* ----------
*   d_getenv_copy(name, buffer, sizeof(buffer), &length) reads a variable
* into a buffer of the caller's; d_getenv_dup(name, &value) into storage the
* caller frees. d_setenv(name, value) and d_unsetenv(name) change one. Each
* answers 0, or the errno code that says why not: ENOENT for a variable that
* is not set.
*
* OPTIONS
* -------
*   None.
*
* CONFIGURATION
* -------------
*   None: the platform chooses the runtime's calls (D_ENV_COMPILER_MSVC_FAMILY
* for the Microsoft runtime, _WIN32 for Windows).
*
*
* path:      /inc/djinterp/c/dstdlib.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.26
*                                                            revised: 2026.11.26
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE ENVIRONMENT
    ---------------
    1.  Reading a variable
    2.  Setting and removing a variable
*/

#ifndef DJINTERP_C_DSTDLIB_H
#define DJINTERP_C_DSTDLIB_H 1

// std
#include <stddef.h>      // size_t
// djinterp
#include "./djinterp.h"  // framework root


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  THE ENVIRONMENT
//==============================================================================
// A name is refused, as EINVAL, when it is NULL, empty, or holds an '=',
// which no variable's name can.


// 1.1    Reading a variable
//------------------------------------------------------------------------------
/**
 * @brief Copies a variable's value into a bounded buffer (getenv_s
 *        equivalent).
 *
 * @note To learn whether a variable is set, and how long its value is, pass
 *       a NULL `_buffer` and a `_size` of 0: ERANGE and `*_length` say it is
 *       set and how long; ENOENT says it is not.
 *
 * @param[in]  _name    the variable's name.
 * @param[out] _buffer  receives the value and its terminator; may be NULL
 *                      when `_size` is 0. When the value is not copied it
 *                      holds the empty string, if `_size` is not 0.
 * @param[in]  _size    the size of `_buffer` in bytes, terminator included.
 * @param[out] _length  receives the value's length, its terminator not
 *                      counted, or 0 when it is not set; may be NULL.
 * @return 0 when the variable is set and its value fits; ERANGE when it is
 *         set and does not; ENOENT when it is not set; EINVAL for a name
 *         refused, or a NULL `_buffer` with a `_size` that is not 0.
 */
int d_getenv_copy(
    const char* _name,
    char*       _buffer,
    size_t      _size,
    size_t*     _length
);
/**
 * @brief Copies a variable's value into storage the caller frees (_dupenv_s
 *        equivalent).
 *
 * @param[in]  _name   the variable's name.
 * @param[out] _value  receives the copy, or NULL when there is none.
 * @return 0 when the variable is set and copied; ENOENT when it is not set;
 *         ENOMEM when the copy could not be allocated; EINVAL for a name
 *         refused or a NULL `_value`.
 * @post   on 0 the caller owns `*_value` and releases it with free().
 */
int d_getenv_dup(
    const char* _name,
    char**      _value
);

// 1.2    Setting and removing a variable
//------------------------------------------------------------------------------
/**
 * @brief Sets a variable, replacing any value it had (setenv equivalent).
 *
 * @note A `_value` of "" removes the variable, on every platform: the
 *       Microsoft runtime cannot hold an empty variable.
 *
 * @param[in] _name   the variable's name.
 * @param[in] _value  its value, which is copied.
 * @return 0, or EINVAL for a name refused or a NULL `_value`, or ENOMEM.
 */
int d_setenv(
    const char* _name,
    const char* _value
);
/**
 * @brief Removes a variable (unsetenv equivalent).
 *
 * @param[in] _name  the variable's name.
 * @return 0, whether it was set or not; EINVAL for a name refused; ENOMEM.
 */
int d_unsetenv(
    const char* _name
);


D_EXTERN_C_END


#endif  // DJINTERP_C_DSTDLIB_H
