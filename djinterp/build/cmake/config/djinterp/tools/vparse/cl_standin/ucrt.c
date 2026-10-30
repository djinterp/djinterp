/*******************************************************************************
* vparse [cl_standin]                                                     ucrt.c
*
* What the Universal CRT has and MinGW-w64's msvcrt does not, for the stand-in
* for cl to link.
*   A unit the stand-in compiles reads as cl's, so it takes the Microsoft
* runtime's branches, and where one calls a function the Universal CRT added,
* the msvcrt.dll MinGW-w64 links against has no such export: the link fails
* where cl's would not. Each function here is one of those, written over what
* msvcrt has, to the Universal CRT's contract; link compiles this file for
* every program it links.
*   THE NAMES ARE THE RUNTIME'S. A unit calls such a function through its
* import pointer, `__imp_` and the name, which lld fills from a definition of
* the name itself and from nothing else (it has no alias for an import), so
* each is defined under the name the Universal CRT gives it, reserved though
* that is: this file stands where the runtime does, as identity.h stands
* where the compiler does. MinGW-w64's own headers declare these names for
* import, which link's flags for this file say is expected.
*   djinterp's argument parser reads the environment through _dupenv_s under
* cl, and is the reason this file exists.
*
* path:      /build/cmake/config/djinterp/tools/vparse/cl_standin/ucrt.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/
// std
#include <errno.h>   // EINVAL, ENOMEM
#include <stddef.h>  // size_t
#include <stdlib.h>  // getenv, malloc
#include <string.h>  // strlen, memcpy


/**
 * @brief The Universal CRT's _dupenv_s: a copy of an environment variable's
 *        value.
 *
 * @param[out] _buffer  receives the copy, which the caller frees with free;
 *                      NULL when the variable is not set.
 * @param[out] _count   receives the copy's size, its NUL included; 0 when
 *                      the variable is not set. May be NULL.
 * @param[in]  _name    the variable's name.
 * @return 0, also for a variable that is not set; EINVAL for a NULL
 *         `_buffer` or `_name`; ENOMEM when the copy cannot be made.
 *
 * getenv's value is read once and copied at once, so the copy is whole.
 */
int _dupenv_s(
    char**      _buffer,
    size_t*     _count,
    const char* _name
)
{
    // nowhere to put it, or nothing to look for
    if ( (!_buffer) ||
         (!_name) )
    {
        return EINVAL;
    }

    const char* value = getenv(_name);

    *_buffer = NULL;

    // a count, for a caller that asked
    if (_count)
    {
        *_count = 0u;
    }

    // not set: no copy, and no failure
    if (!value)
    {
        return 0;
    }

    const size_t size = strlen(value) + 1u;
    char*        copy = malloc(size);

    // no memory for the copy
    if (!copy)
    {
        return ENOMEM;
    }

    memcpy(copy,
           value,
           size);
    *_buffer = copy;

    // its size, for a caller that asked
    if (_count)
    {
        *_count = size;
    }

    return 0;
}
