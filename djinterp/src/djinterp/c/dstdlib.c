/*******************************************************************************
* djinterp [c]                                                         dstdlib.c
*
* Definitions for the declarations in `dstdlib.h`.
*   Under the Microsoft runtime (cl and clang-cl), which refuses getenv under
* /sdl, a variable is read with getenv_s and _dupenv_s; on Windows, MinGW
* included, it is set and removed with _putenv_s, so that getenv reads what
* was set; elsewhere getenv, setenv and unsetenv, which POSIX declares only
* to a unit that asks, as this one does before its first include.
*
*
* path:      /src/djinterp/c/dstdlib.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.26
*                                                            revised: 2026.11.26
*******************************************************************************/
// setenv and unsetenv are POSIX's, which a strict C build hides unless the
// unit asks for them
#if ( (!defined(_WIN32))          &&                                           \
      (!defined(_POSIX_C_SOURCE)) &&                                           \
      (!defined(_XOPEN_SOURCE))   &&                                           \
      (!defined(_GNU_SOURCE)) )
    #define _POSIX_C_SOURCE 200809L
#endif

#include "../../../inc/djinterp/c/dstdlib.h"  // corresponding header
// std
#include <errno.h>   // EINVAL, ENOENT, ENOMEM, ERANGE, errno
#include <stdlib.h>  // getenv(_s), _dupenv_s, setenv, unsetenv, _putenv_s,
                     // malloc, free
#include <string.h>  // memcpy, strchr, strlen


/*
d_internal_env_name_ok
  Whether a name can be a variable's: not NULL, not empty, and no '='.
*/
static
int
d_internal_env_name_ok(
    const char* _name
)
{
    return ( (_name != NULL)  &&
             (_name[0] != '\0') &&
             (strchr(_name, '=') == NULL) );
}


//==============================================================================
// 1.  THE ENVIRONMENT
//==============================================================================


// 1.1    Reading a variable
//------------------------------------------------------------------------------

/*
d_getenv_copy
  Asks first how long the value is, then copies it only when it fits: under
the Microsoft runtime getenv_s's first call, with no buffer, gives the size
the value needs with its terminator, 0 when it is not set.
*/
int
d_getenv_copy(
    const char* _name,
    char*       _buffer,
    size_t      _size,
    size_t*     _length
)
{
    size_t length = 0u;
    int    found  = 0;

    if (_length != NULL)
    {
        *_length = 0u;
    }

    if ( (!d_internal_env_name_ok(_name)) ||
         ( (_buffer == NULL) &&
           (_size != 0u) ) )
    {
        return EINVAL;
    }

    if (_size != 0u)
    {
        _buffer[0] = '\0';
    }

#if D_ENV_COMPILER_MSVC_FAMILY
    {
        size_t needed = 0u;

        if (getenv_s(&needed, NULL, 0u, _name) != 0)
        {
            return EINVAL;
        }

        found  = (needed != 0u);
        length = found ? (needed - 1u) : 0u;

        if ( (found)            &&
             (length < _size) &&
             (getenv_s(&needed, _buffer, _size, _name) != 0) )
        {
            _buffer[0] = '\0';

            return ERANGE;
        }
    }
#else
    {
        const char* const text = getenv(_name);

        found  = (text != NULL);
        length = found ? strlen(text) : 0u;

        if ( (found) &&
             (length < _size) )
        {
            memcpy(_buffer, text, length + 1u);
        }
    }
#endif  // D_ENV_COMPILER_MSVC_FAMILY

    if (!found)
    {
        return ENOENT;
    }

    if (_length != NULL)
    {
        *_length = length;
    }

    return (length < _size) ? 0 : ERANGE;
}

/*
d_getenv_dup
  _dupenv_s under the Microsoft runtime, whose copy is free()'d as any other;
elsewhere getenv's text, copied at once.
*/
int
d_getenv_dup(
    const char* _name,
    char**      _value
)
{
    if (_value != NULL)
    {
        *_value = NULL;
    }

    if ( (!d_internal_env_name_ok(_name)) ||
         (_value == NULL) )
    {
        return EINVAL;
    }

#if D_ENV_COMPILER_MSVC_FAMILY
    {
        char*   copy   = NULL;
        size_t  size   = 0u;
        errno_t result = _dupenv_s(&copy, &size, _name);

        if (result != 0)
        {
            free(copy);

            return (result == ENOMEM) ? ENOMEM : EINVAL;
        }

        if (copy == NULL)
        {
            return ENOENT;
        }

        *_value = copy;
    }
#else
    {
        const char* const text = getenv(_name);
        size_t            size;
        char*             copy;

        if (text == NULL)
        {
            return ENOENT;
        }

        size = strlen(text) + 1u;
        copy = (char*)malloc(size);

        if (copy == NULL)
        {
            return ENOMEM;
        }

        memcpy(copy, text, size);
        *_value = copy;
    }
#endif  // D_ENV_COMPILER_MSVC_FAMILY

    return 0;
}


// 1.2    Setting and removing a variable
//------------------------------------------------------------------------------

/*
d_setenv
  _putenv_s on Windows, which removes the variable for a value of ""; and so
here, elsewhere, unsetenv for that value, and setenv for any other.
*/
int
d_setenv(
    const char* _name,
    const char* _value
)
{
    if ( (!d_internal_env_name_ok(_name)) ||
         (_value == NULL) )
    {
        return EINVAL;
    }

    if (_value[0] == '\0')
    {
        return d_unsetenv(_name);
    }

#if defined(_WIN32)
    return (_putenv_s(_name, _value) == 0) ? 0 : ENOMEM;
#else
    if (setenv(_name, _value, 1) == 0)
    {
        return 0;
    }

    return (errno == EINVAL) ? EINVAL : ENOMEM;
#endif  // _WIN32
}

/*
d_unsetenv
  _putenv_s with a value of "" on Windows; unsetenv elsewhere. Neither fails
for a variable that is not set.
*/
int
d_unsetenv(
    const char* _name
)
{
    if (!d_internal_env_name_ok(_name))
    {
        return EINVAL;
    }

#if defined(_WIN32)
    return (_putenv_s(_name, "") == 0) ? 0 : ENOMEM;
#else
    if (unsetenv(_name) == 0)
    {
        return 0;
    }

    return (errno == EINVAL) ? EINVAL : ENOMEM;
#endif  // _WIN32
}
