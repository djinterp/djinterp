/*******************************************************************************
* djinterp [c]                                                       string_fn.c
*
* Definitions for the declarations in `string_fn.h`.
*   Every function here is portable C; none depends on a platform's own
* extensions, which is what lets the higher-level string types build on this
* layer everywhere.
*
*
* path:      /src/djinterp/c/string_fn.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.30
*                                                            revised: 2026.10.04
*******************************************************************************/
#include "../../../inc/djinterp/c/string_fn.h"  // corresponding header
// std
#include <errno.h>                                // EINVAL, ERANGE
#include <stdbool.h>                              // bool
#include <stddef.h>                               // size_t, NULL
#include <stdlib.h>                               // malloc
#include <string.h>                               // strlen, strspn, memcmp, ...
// djinterp
#include "../../../inc/djinterp/c/djinterp.h"        // framework root
#include "../../../inc/djinterp/c/memory/dmemory.h"  // d_memcpy


// ASCII character classes
//   string_fn classifies and folds case in ASCII only (decision 12 of the
// register): the C library's tolower and isalpha follow the current C
// locale, so the same bytes above 0x7F answered differently from one
// machine, or one setlocale call, to the next. Bytes outside ASCII are never
// letters, digits or white space here, and keep their case.

/*
d_internal_ascii_tolower
  'A' to 'Z' to 'a' to 'z'; every other value as it is.
*/
static int
d_internal_ascii_tolower(
    int _c
)
{
    return ( (_c >= 'A') &&
             (_c <= 'Z') ) ? (_c - 'A' + 'a') : _c;
}

/*
d_internal_ascii_toupper
  'a' to 'z' to 'A' to 'Z'; every other value as it is.
*/
static int
d_internal_ascii_toupper(
    int _c
)
{
    return ( (_c >= 'a') &&
             (_c <= 'z') ) ? (_c - 'a' + 'A') : _c;
}

/*
d_internal_ascii_isalpha
  An ASCII letter.
*/
static int
d_internal_ascii_isalpha(
    int _c
)
{
    return ( ( (_c >= 'a') && (_c <= 'z') ) ||
             ( (_c >= 'A') && (_c <= 'Z') ) );
}

/*
d_internal_ascii_isdigit
  An ASCII decimal digit, as isdigit is in every locale.
*/
static int
d_internal_ascii_isdigit(
    int _c
)
{
    return ( (_c >= '0') &&
             (_c <= '9') );
}

/*
d_internal_ascii_isalnum
  An ASCII letter or decimal digit.
*/
static int
d_internal_ascii_isalnum(
    int _c
)
{
    return ( (d_internal_ascii_isalpha(_c)) ||
             (d_internal_ascii_isdigit(_c)) );
}

/*
d_internal_ascii_isspace
  The six white-space characters of the C locale: space, and \t through \r.
*/
static int
d_internal_ascii_isspace(
    int _c
)
{
    return ( (_c == ' ') ||
             ( (_c >= '\t') && (_c <= '\r') ) );
}


// safe copy and concatenation
/*
d_strcpy_s
  Validates in C11's order -- destination, then size, then source -- so a NULL
source still empties a usable destination. Measures the source once and copies
text and terminator together; on overflow the destination is emptied rather
than truncated.
*/
int
d_strcpy_s(
    char* D_RESTRICT       _destination,
    size_t                 _dest_size,
    const char* D_RESTRICT _source
)
{
    if (_destination == NULL)
    {
        return EINVAL;
    }

    if (_dest_size == 0)
    {
        return ERANGE;
    }

    if (_source == NULL)
    {
        _destination[0] = '\0';

        return EINVAL;
    }

    const size_t src_len = strlen(_source);

    if (src_len >= _dest_size)
    {
        _destination[0] = '\0';

        return ERANGE;
    }

    d_memcpy(_destination,
             _source,
             src_len + 1);

    return 0;
}

/*
d_strncpy_s
  Bounds the source with d_strnlen at _count, so it need not be terminated
within that range, and terminates the copy explicitly. On overflow the
destination is emptied rather than truncated.
*/
int
d_strncpy_s(
    char* D_RESTRICT       _destination,
    size_t                 _dest_size,
    const char* D_RESTRICT _source,
    size_t                 _count
)
{
    if (_destination == NULL)
    {
        return EINVAL;
    }

    if (_dest_size == 0)
    {
        return ERANGE;
    }

    if (_source == NULL)
    {
        _destination[0] = '\0';

        return EINVAL;
    }

    const size_t src_len = d_strnlen(_source,
                                     _count);

    if (src_len >= _dest_size)
    {
        _destination[0] = '\0';

        return ERANGE;
    }

    d_memcpy(_destination,
             _source,
             src_len);
    _destination[src_len] = '\0';

    return 0;
}

/*
d_strcat_s
  Measures the existing string with d_strnlen bounded by the buffer, so an
unterminated destination reads as full and fails with ERANGE instead of
overrunning. On failure the destination is emptied.
*/
int
d_strcat_s(
    char* D_RESTRICT       _destination,
    size_t                 _dest_size,
    const char* D_RESTRICT _source
)
{
    if ( (_destination == NULL) ||
         (_source == NULL) )
    {
        if ( (_destination != NULL) &&
             (_dest_size > 0) )
        {
            _destination[0] = '\0';
        }

        return EINVAL;
    }

    if (_dest_size == 0)
    {
        return ERANGE;
    }

    const size_t dest_len = d_strnlen(_destination,
                                      _dest_size);
    const size_t src_len = strlen(_source);

    if (dest_len + src_len >= _dest_size)
    {
        _destination[0] = '\0';

        return ERANGE;
    }

    d_memcpy(_destination + dest_len,
             _source,
             src_len + 1);

    return 0;
}

/*
d_strncat_s
  As d_strcat_s, with the appended length bounded by d_strnlen at _count and
the terminator written explicitly.
*/
int
d_strncat_s(
    char* D_RESTRICT       _destination,
    size_t                 _dest_size,
    const char* D_RESTRICT _source,
    size_t                 _count
)
{
    if ( (_destination == NULL) ||
         (_source == NULL) )
    {
        if ( (_destination != NULL) &&
             (_dest_size > 0) )
        {
            _destination[0] = '\0';
        }

        return EINVAL;
    }

    if (_dest_size == 0)
    {
        return ERANGE;
    }

    const size_t dest_len = d_strnlen(_destination,
                                      _dest_size);
    const size_t src_len = d_strnlen(_source,
                                     _count);

    if (dest_len + src_len >= _dest_size)
    {
        _destination[0] = '\0';

        return ERANGE;
    }

    d_memcpy(_destination + dest_len,
             _source,
             src_len);
    _destination[dest_len + src_len] = '\0';

    return 0;
}

// duplication
/*
d_strdup
  Measures once and copies the terminator along with the text into a malloc'd
buffer.
*/
char*
d_strdup(
    const char* _str
)
{
    if (_str == NULL)
    {
        return NULL;
    }

    const size_t len = strlen(_str) + 1;
    char* copy = malloc(len);

    if (copy != NULL)
    {
        d_memcpy(copy,
                 _str,
                 len);
    }

    return copy;
}

/*
d_strndup
  Bounds the length with d_strnlen, so the source need not be terminated
within _n, and terminates the copy explicitly.
*/
char*
d_strndup(
    const char* _str,
    size_t      _n
)
{
    if (_str == NULL)
    {
        return NULL;
    }

    const size_t len = d_strnlen(_str,
                                 _n);
    char* copy = malloc(len + 1);

    if (copy != NULL)
    {
        d_memcpy(copy,
                 _str,
                 len);
        copy[len] = '\0';
    }

    return copy;
}

// case-insensitive comparison
/*
d_strcasecmp
  Settles the NULL cases, then compares lowercased characters, read as
unsigned char, until either string ends; the final difference also orders
strings of unequal length.
*/
int
d_strcasecmp(
    const char* _s1,
    const char* _s2
)
{
    if ( (_s1 == NULL) &&
         (_s2 == NULL) )
    {
        return 0;
    }

    if (_s1 == NULL)
    {
        return -1;
    }

    if (_s2 == NULL)
    {
        return 1;
    }

    while ( (*_s1) &&
            (*_s2) )
    {
        const int c1 = d_internal_ascii_tolower((unsigned char)*_s1);
        const int c2 = d_internal_ascii_tolower((unsigned char)*_s2);

        if (c1 != c2)
        {
            return c1 - c2;
        }

        _s1++;
        _s2++;
    }

    return ( d_internal_ascii_tolower((unsigned char)*_s1) -
             d_internal_ascii_tolower((unsigned char)*_s2) );
}

/*
d_strncasecmp
  As d_strcasecmp, stopping after _n characters; a zero count compares equal
before any NULL check.
*/
int
d_strncasecmp(
    const char* _s1,
    const char* _s2,
    size_t      _n
)
{
    if (_n == 0)
    {
        return 0;
    }

    if ( (_s1 == NULL) &&
         (_s2 == NULL) )
    {
        return 0;
    }

    if (_s1 == NULL)
    {
        return -1;
    }

    if (_s2 == NULL)
    {
        return 1;
    }

    while ( (_n > 0) &&
            (*_s1)   &&
            (*_s2) )
    {
        const int c1 = d_internal_ascii_tolower((unsigned char)*_s1);
        const int c2 = d_internal_ascii_tolower((unsigned char)*_s2);

        if (c1 != c2)
        {
            return c1 - c2;
        }

        _s1++;
        _s2++;
        _n--;
    }

    if (_n == 0)
    {
        return 0;
    }

    return ( d_internal_ascii_tolower((unsigned char)*_s1) -
             d_internal_ascii_tolower((unsigned char)*_s2) );
}

// tokenization
/*
d_strtok_r
  strspn skips the leading delimiters and strpbrk finds the token's end, which
is overwritten with '\0'. *_saveptr is left just past it, or NULL once the
input is exhausted.
*/
char*
d_strtok_r(
    char* D_RESTRICT       _str,
    const char* D_RESTRICT _delim,
    char** D_RESTRICT      _saveptr
)
{
    if ( (_delim == NULL) ||
         (_saveptr == NULL) )
    {
        return NULL;
    }

    if (_str != NULL)
    {
        *_saveptr = _str;
    }

    char* token_start = *_saveptr;

    if (token_start == NULL)
    {
        return NULL;
    }

    // skip leading delimiters
    token_start += strspn(token_start,
                          _delim);

    if (*token_start == '\0')
    {
        *_saveptr = NULL;

        return NULL;
    }

    // find end of token
    char* token_end = strpbrk(token_start,
                              _delim);

    if (token_end != NULL)
    {
        *token_end = '\0';
        *_saveptr = token_end + 1;
    }
    else
    {
        *_saveptr = NULL;
    }

    return token_start;
}

// bounded length
/*
d_strnlen
  Scans at most _maxlen bytes, so an unterminated buffer is safe to measure.
*/
size_t
d_strnlen(
    const char* _str,
    size_t      _maxlen
)
{
    if (_str == NULL)
    {
        return 0;
    }

    size_t len = 0;

    while ( (len < _maxlen) &&
            (_str[len] != '\0') )
    {
        len++;
    }

    return len;
}

// case-insensitive search
/*
d_strcasestr
  Tries d_strncasecmp at each position of _haystack in turn: O(n * m), with no
skip table.
*/
char*
d_strcasestr(
    const char* _haystack,
    const char* _needle
)
{
    if ( (_haystack == NULL) ||
         (_needle == NULL) )
    {
        return NULL;
    }

    if (*_needle == '\0')
    {
        return (char*)_haystack;
    }

    const size_t needle_len = strlen(_needle);

    while (*_haystack != '\0')
    {
        if (d_strncasecmp(_haystack,
                          _needle,
                          needle_len) == 0)
        {
            return (char*)_haystack;
        }

        _haystack++;
    }

    return NULL;
}

// case conversion
/*
d_strlwr
  Rewrites each character through tolower, read as unsigned char so negative
char values stay defined.
*/
char*
d_strlwr(
    char* _str
)
{
    if (_str == NULL)
    {
        return NULL;
    }

    char* original = _str;

    while (*_str != '\0')
    {
        *_str = (char)d_internal_ascii_tolower((unsigned char)*_str);
        _str++;
    }

    return original;
}

/*
d_strupr
  Rewrites each character through toupper, read as unsigned char so negative
char values stay defined.
*/
char*
d_strupr(
    char* _str
)
{
    if (_str == NULL)
    {
        return NULL;
    }

    char* original = _str;

    while (*_str != '\0')
    {
        *_str = (char)d_internal_ascii_toupper((unsigned char)*_str);
        _str++;
    }

    return original;
}

// reversal
/*
d_strrev
  Swaps from both ends toward the middle; a string of length 0 or 1 returns at
once.
*/
char*
d_strrev(
    char* _str
)
{
    if (_str == NULL)
    {
        return NULL;
    }

    const size_t len = strlen(_str);

    if (len <= 1)
    {
        return _str;
    }

    char* start = _str;
    char* end   = _str + len - 1;

    while (start < end)
    {
        const char temp = *start;
        *start = *end;
        *end = temp;
        start++;
        end--;
    }

    return _str;
}

// character search
/*
d_strchrnul
  Scans until the character or the terminator, whichever comes first.
*/
char*
d_strchrnul(
    const char* _str,
    int         _c
)
{
    if (_str == NULL)
    {
        return NULL;
    }

    while ( (*_str != '\0') &&
            (*_str != (char)_c) )
    {
        _str++;
    }

    return (char*)_str;
}

// error strings
// D_INTERNAL_STRING_FN_STRERROR
//   macro: where d_strerror_r gets a description: 1, strerror_s (the Windows
// C runtimes); 2, POSIX's strerror_r, which returns an int; 3, glibc's GNU
// strerror_r, which returns a char* and replaces the POSIX one when the
// build defines _GNU_SOURCE; 0, none, and d_strerror_r keeps its own table.
// glibc and musl declare strerror_r only where the build's feature-test
// macros make POSIX.1-2001 names visible (the C guide's rule): glibc says so
// in _POSIX_C_SOURCE, musl only in the macros the build defined. macOS and
// the BSDs always declare it.
#if defined(D_ENV_PLATFORM_WINDOWS)
    #define D_INTERNAL_STRING_FN_STRERROR 1
#elif ( ( (defined(_POSIX_C_SOURCE)) &&                                        \
          (_POSIX_C_SOURCE >= 200112L) )     ||                                \
        ( (defined(_XOPEN_SOURCE)) &&                                          \
          (_XOPEN_SOURCE >= 600) )           ||                                \
        (defined(_GNU_SOURCE))               ||                                \
        (defined(_DEFAULT_SOURCE))           ||                                \
        (defined(_BSD_SOURCE))               ||                                \
        (defined(D_ENV_PLATFORM_MACOS))      ||                                \
        (defined(D_ENV_PLATFORM_UNIX)) )
    #if ( (defined(__GLIBC__)) &&                                              \
          (defined(_GNU_SOURCE)) )
        #define D_INTERNAL_STRING_FN_STRERROR 3
    #else
        #define D_INTERNAL_STRING_FN_STRERROR 2
    #endif
#else
    #define D_INTERNAL_STRING_FN_STRERROR 0
#endif

/*
d_internal_strerror_platform
  The platform's description of _errnum, written to _local (_size bytes) or
returned from the C library's own storage; NULL where the platform has none,
or none for this number.
*/
static const char*
d_internal_strerror_platform(
    int    _errnum,
    char*  _local,
    size_t _size
)
{
#if (D_INTERNAL_STRING_FN_STRERROR == 1)
    return (strerror_s(_local, _size, _errnum) == 0) ? _local : NULL;
#elif (D_INTERNAL_STRING_FN_STRERROR == 2)
    return (strerror_r(_errnum, _local, _size) == 0) ? _local : NULL;
#elif (D_INTERNAL_STRING_FN_STRERROR == 3)
    return strerror_r(_errnum, _local, _size);
#else
    (void)_errnum;
    (void)_local;
    (void)_size;

    return NULL;
#endif
}

/*
d_internal_strerror_table
  The fallback, where the platform describes nothing: 0, EINVAL and ERANGE,
and "Unknown error" for every other number.
*/
static const char*
d_internal_strerror_table(
    int _errnum
)
{
    switch (_errnum)
    {
        case 0:      return "success";
        case EINVAL: return "Invalid argument";
        case ERANGE: return "Result too large";
        default:     return "Unknown error";
    }
}

/*
d_strerror_r
  Delegates to the platform (decision 11 of the register), through a local
buffer, so the caller's buffer is written only with a whole description and
is untouched when that does not fit. The local buffer holds any message a C
library writes; a number the platform does not know falls back to the table.
*/
int
d_strerror_r(
    int    _errnum,
    char*  _buf,
    size_t _buflen
)
{
    if ( (_buf    == NULL) ||
         (_buflen == 0) )
    {
        return EINVAL;
    }

    char        local[256];
    const char* msg = d_internal_strerror_platform(_errnum,
                                                   local,
                                                   sizeof(local));

    // the platform has no description for it
    if (msg == NULL)
    {
        msg = d_internal_strerror_table(_errnum);
    }

    const size_t msg_len = strlen(msg);

    if (msg_len >= _buflen)
    {
        return ERANGE;
    }

    d_memcpy(_buf,
             msg,
             msg_len + 1);

    return 0;
}

/******************************************************************************
 * xi.   LENGTH-AWARE COMPARISON
 *****************************************************************************/

// length-aware comparison
/*
d_strcmp_n
  Settles the NULL cases, compares the common prefix with memcmp, and breaks a
tie on length, so embedded NULs take part.
*/
int
d_strcmp_n(
    const char* _s1,
    size_t      _s1_len,
    const char* _s2,
    size_t      _s2_len
)
{
    // null handling
    if ( (!_s1) &&
         (!_s2) )
    {
        return 0;
    }

    if (!_s1)
    {
        return -1;
    }

    if (!_s2)
    {
        return 1;
    }

    // compare up to the shorter length
    const size_t min_len = (_s1_len < _s2_len)
        ? _s1_len
        : _s2_len;
    const int result  = memcmp(_s1,
                               _s2,
                               min_len);

    if (result != 0)
    {
        return result;
    }

    // equal up to min_len; shorter string is "less"
    if (_s1_len < _s2_len)
    {
        return -1;
    }

    if (_s1_len > _s2_len)
    {
        return 1;
    }

    return 0;
}

/*
d_strncmp_n
  Clamps both lengths to _n, then compares as d_strcmp_n does; a zero count
compares equal before any NULL check.
*/
int
d_strncmp_n(
    const char* _s1,
    size_t      _s1_len,
    const char* _s2,
    size_t      _s2_len,
    size_t      _n
)
{
    if (_n == 0)
    {
        return 0;
    }

    // null handling
    if ( (_s1 == NULL) &&
         (_s2 == NULL) )
    {
        return 0;
    }

    if (_s1 == NULL)
    {
        return -1;
    }

    if (_s2 == NULL)
    {
        return 1;
    }

    // clamp effective lengths to _n
    const size_t cmp_len1 = (_s1_len < _n) ? _s1_len : _n;
    const size_t cmp_len2 = (_s2_len < _n) ? _s2_len : _n;
    const size_t min_len  = (cmp_len1 < cmp_len2) ? cmp_len1 : cmp_len2;

    const int result = memcmp(_s1,
                              _s2,
                              min_len);

    if (result != 0)
    {
        return result;
    }

    // equal up to min_len; compare effective lengths
    if (cmp_len1 < cmp_len2)
    {
        return -1;
    }

    if (cmp_len1 > cmp_len2)
    {
        return 1;
    }

    return 0;
}

/*
d_strcasecmp_n
  As d_strcmp_n, comparing lowercased characters one at a time instead of with
memcmp.
*/
int
d_strcasecmp_n(
    const char* _s1,
    size_t      _s1_len,
    const char* _s2,
    size_t      _s2_len
)
{
    // null handling
    if ( (_s1 == NULL) &&
         (_s2 == NULL) )
    {
        return 0;
    }

    if (_s1 == NULL)
    {
        return -1;
    }

    if (_s2 == NULL)
    {
        return 1;
    }

    // compare character-by-character up to the shorter length
    const size_t min_len = (_s1_len < _s2_len) ? _s1_len : _s2_len;

    for (size_t i = 0; i < min_len; i++)
    {
        const int c1 = d_internal_ascii_tolower((unsigned char)_s1[i]);
        const int c2 = d_internal_ascii_tolower((unsigned char)_s2[i]);

        if (c1 != c2)
        {
            return c1 - c2;
        }
    }

    // equal up to min_len; shorter string is "less"
    if (_s1_len < _s2_len)
    {
        return -1;
    }

    if (_s1_len > _s2_len)
    {
        return 1;
    }

    return 0;
}

/*
d_strncasecmp_n
  As d_strncmp_n, comparing lowercased characters one at a time instead of
with memcmp.
*/
int
d_strncasecmp_n(
    const char* _s1,
    size_t      _s1_len,
    const char* _s2,
    size_t      _s2_len,
    size_t      _n
)
{
    if (_n == 0)
    {
        return 0;
    }

    // null handling
    if ( (_s1 == NULL) &&
         (_s2 == NULL) )
    {
        return 0;
    }

    if (_s1 == NULL)
    {
        return -1;
    }

    if (_s2 == NULL)
    {
        return 1;
    }

    // clamp effective lengths to _n
    const size_t cmp_len1 = (_s1_len < _n) ? _s1_len : _n;
    const size_t cmp_len2 = (_s2_len < _n) ? _s2_len : _n;
    const size_t min_len  = (cmp_len1 < cmp_len2) ? cmp_len1 : cmp_len2;

    for (size_t i = 0; i < min_len; i++)
    {
        const int c1 = d_internal_ascii_tolower((unsigned char)_s1[i]);
        const int c2 = d_internal_ascii_tolower((unsigned char)_s2[i]);

        if (c1 != c2)
        {
            return c1 - c2;
        }
    }

    // equal up to min_len; compare effective lengths
    if (cmp_len1 < cmp_len2)
    {
        return -1;
    }

    if (cmp_len1 > cmp_len2)
    {
        return 1;
    }

    return 0;
}

/*
d_strequals
  Rejects unequal lengths before comparing contents, then compares with
memcmp.
*/
bool
d_strequals(
    const char* _s1,
    size_t      _s1_len,
    const char* _s2,
    size_t      _s2_len
)
{
    // null handling
    if ( (_s1 == NULL) &&
         (_s2 == NULL) )
    {
        return true;
    }

    if ( (_s1 == NULL) ||
         (_s2 == NULL) )
    {
        return false;
    }

    // fast path: different lengths cannot be equal
    if (_s1_len != _s2_len)
    {
        return false;
    }

    return (memcmp(_s1,
                   _s2,
                   _s1_len) == 0);
}

/*
d_strequals_nocase
  Rejects unequal lengths first, then compares lowercased characters one at a
time.
*/
bool
d_strequals_nocase(
    const char* _s1,
    size_t      _s1_len,
    const char* _s2,
    size_t      _s2_len
)
{
    // null handling
    if ( (_s1 == NULL) &&
         (_s2 == NULL) )
    {
        return true;
    }

    if ( (_s1 == NULL) ||
         (_s2 == NULL) )
    {
        return false;
    }

    // fast path: different lengths cannot be equal
    if (_s1_len != _s2_len)
    {
        return false;
    }

    for (size_t i = 0; i < _s1_len; i++)
    {
        if (d_internal_ascii_tolower((unsigned char)_s1[i]) !=
            d_internal_ascii_tolower((unsigned char)_s2[i]))
        {
            return false;
        }
    }

    return true;
}

/******************************************************************************
 * xii.  VALIDATION
 *****************************************************************************/

// validation
/*
d_str_is_valid
  Scans the first _length bytes for a '\0'; the byte at _length itself is not
examined.
*/
bool
d_str_is_valid(
    const char* _text,
    size_t      _length
)
{
    if (!_text)
    {
        return false;
    }

    for (size_t i = 0; i < _length; i++)
    {
        if (_text[i] == '\0')
        {
            return false;
        }
    }

    return true;
}

/*
d_str_is_ascii
  Tests each byte, read as unsigned char, against 127; an empty buffer passes.
*/
bool
d_str_is_ascii(
    const char* _text,
    size_t      _length
)
{
    if (_text == NULL)
    {
        return false;
    }

    for (size_t i = 0; i < _length; i++)
    {
        if ((unsigned char)_text[i] > 127)
        {
            return false;
        }
    }

    return true;
}

/*
d_str_is_numeric
  Applies isdigit to each byte, read as unsigned char; an empty buffer fails.
*/
bool
d_str_is_numeric(
    const char* _text,
    size_t      _length
)
{
    if ( (_text == NULL) ||
         (_length == 0) )
    {
        return false;
    }

    for (size_t i = 0; i < _length; i++)
    {
        if (!d_internal_ascii_isdigit((unsigned char)_text[i]))
        {
            return false;
        }
    }

    return true;
}

/*
d_str_is_alpha
  Applies isalpha, which follows the current C locale, to each byte, read as
unsigned char; an empty buffer fails.
*/
bool
d_str_is_alpha(
    const char* _text,
    size_t      _length
)
{
    if ( (_text == NULL) ||
         (_length == 0) )
    {
        return false;
    }

    for (size_t i = 0; i < _length; i++)
    {
        if (!d_internal_ascii_isalpha((unsigned char)_text[i]))
        {
            return false;
        }
    }

    return true;
}

/*
d_str_is_alnum
  Applies isalnum, which follows the current C locale, to each byte, read as
unsigned char; an empty buffer fails.
*/
bool
d_str_is_alnum(
    const char* _text,
    size_t      _length
)
{
    if ( (_text == NULL) ||
         (_length == 0) )
    {
        return false;
    }

    for (size_t i = 0; i < _length; i++)
    {
        if (!d_internal_ascii_isalnum((unsigned char)_text[i]))
        {
            return false;
        }
    }

    return true;
}

/*
d_str_is_whitespace
  Applies isspace, which follows the current C locale, to each byte, read as
unsigned char; an empty buffer fails.
*/
bool
d_str_is_whitespace(
    const char* _text,
    size_t      _length
)
{
    if ( (_text == NULL) ||
         (_length == 0) )
    {
        return false;
    }

    for (size_t i = 0; i < _length; i++)
    {
        if (!d_internal_ascii_isspace((unsigned char)_text[i]))
        {
            return false;
        }
    }

    return true;
}

/******************************************************************************
 * xiii. COUNTING
 *****************************************************************************/

// counting
/*
d_strcount_char
  Walks all _len bytes, so embedded NULs are counted rather than ending the
scan.
*/
size_t
d_strcount_char(
    const char* _str,
    size_t      _len,
    char        _c
)
{
    if (_str == NULL)
    {
        return 0;
    }

    size_t count = 0;

    for (size_t i = 0; i < _len; i++)
    {
        if (_str[i] == _c)
        {
            count++;
        }
    }

    return count;
}

/*
d_strcount_substr
  Slides a memcmp window across the buffer, jumping past each match so the
occurrences counted never overlap.
*/
size_t
d_strcount_substr(
    const char* _str,
    size_t      _len,
    const char* _substr
)
{
    if ( (_str == NULL)    ||
         (_substr == NULL) ||
         (*_substr == '\0') )
    {
        return 0;
    }

    size_t count      = 0;
    const size_t substr_len = strlen(_substr);

    if (substr_len > _len)
    {
        return 0;
    }

    const char* pos = _str;
    const char* end = _str + _len - substr_len + 1;

    while (pos < end)
    {
        if (memcmp(pos,
                   _substr,
                   substr_len) == 0)
        {
            count++;
            pos += substr_len;
        }
        else
        {
            pos++;
        }
    }

    return count;
}

/******************************************************************************
 * xiv.  HASH
 *****************************************************************************/

// hashing
/*
d_strhash
  djb2 (hash * 33 + c from a seed of 5381), reading bytes as unsigned char so
the result does not depend on the signedness of char.
*/
size_t
d_strhash(
    const char* _str,
    size_t      _len
)
{
    if (_str == NULL)
    {
        return 0;
    }

    size_t hash = 5381;

    for (size_t i = 0; i < _len; i++)
    {
        hash = ((hash << 5) + hash) + (unsigned char)_str[i];
    }

    return hash;
}

/******************************************************************************
 * xv.   PREFIX, SUFFIX, AND CONTAINMENT
 *****************************************************************************/

// prefix, suffix, and containment
/*
d_strstartswith
  Rejects a prefix longer than the string, then compares with memcmp.
*/
bool
d_strstartswith(
    const char* _str,
    size_t      _str_len,
    const char* _prefix,
    size_t      _prefix_len
)
{
    if ( (_str == NULL) ||
         (_prefix == NULL) )
    {
        return false;
    }

    if (_prefix_len > _str_len)
    {
        return false;
    }

    return (memcmp(_str,
                   _prefix,
                   _prefix_len) == 0);
}

/*
d_strendswith
  Rejects a suffix longer than the string, which keeps the offset from
underflowing, then compares the tail with memcmp.
*/
bool
d_strendswith(
    const char* _str,
    size_t      _str_len,
    const char* _suffix,
    size_t      _suffix_len
)
{
    if ( (_str == NULL) ||
         (_suffix == NULL) )
    {
        return false;
    }

    if (_suffix_len > _str_len)
    {
        return false;
    }

    return (memcmp(_str + _str_len - _suffix_len,
                   _suffix,
                   _suffix_len) == 0);
}

/*
d_strcontains
  Slides a memcmp window across the buffer; an empty substring is found at
once.
*/
bool
d_strcontains(
    const char* _str,
    size_t      _str_len,
    const char* _substr
)
{
    if ( (_str == NULL) ||
         (_substr == NULL) )
    {
        return false;
    }

    if (*_substr == '\0')
    {
        return true;
    }

    const size_t substr_len = strlen(_substr);

    if (substr_len > _str_len)
    {
        return false;
    }

    const char* end = _str + _str_len - substr_len + 1;
    const char* pos = _str;

    while (pos < end)
    {
        if (memcmp(pos,
                   _substr,
                   substr_len) == 0)
        {
            return true;
        }

        pos++;
    }

    return false;
}

/*
d_strcontains_char
  Walks all _str_len bytes.
*/
bool
d_strcontains_char(
    const char* _str,
    size_t      _str_len,
    char        _c
)
{
    if (_str == NULL)
    {
        return false;
    }

    for (size_t i = 0; i < _str_len; i++)
    {
        if (_str[i] == _c)
        {
            return true;
        }
    }

    return false;
}

/******************************************************************************
 * xvi.  INDEX-RETURNING SEARCH
 *****************************************************************************/

// index-returning search
/*
d_strchr_index
  Walks the buffer forward and returns the first matching index.
*/
d_index
d_strchr_index(
    const char* _str,
    size_t      _len,
    char        _c
)
{
    if (_str == NULL)
    {
        return D_STRING_NPOS;
    }

    for (size_t i = 0; i < _len; i++)
    {
        if (_str[i] == _c)
        {
            return (d_index)i;
        }
    }

    return D_STRING_NPOS;
}

/*
d_strchr_index_from
  Walks the buffer forward from _start.
*/
d_index
d_strchr_index_from(
    const char* _str,
    size_t      _len,
    char        _c,
    size_t      _start
)
{
    if ( (_str == NULL) ||
         (_start >= _len) )
    {
        return D_STRING_NPOS;
    }

    for (size_t i = _start; i < _len; i++)
    {
        if (_str[i] == _c)
        {
            return (d_index)i;
        }
    }

    return D_STRING_NPOS;
}

/*
d_strrchr_index
  Walks backward from the end, decrementing before each test so the size_t
index never wraps.
*/
d_index
d_strrchr_index(
    const char* _str,
    size_t      _len,
    char        _c
)
{
    if ( (_str == NULL) ||
         (_len == 0) )
    {
        return D_STRING_NPOS;
    }

    // search backwards from end
    size_t i = _len;

    while (i > 0)
    {
        i--;

        if (_str[i] == _c)
        {
            return (d_index)i;
        }
    }

    return D_STRING_NPOS;
}

/*
d_strstr_index
  Slides a memcmp window forward; an empty substring matches at 0 without
searching.
*/
d_index
d_strstr_index(
    const char* _str,
    size_t      _str_len,
    const char* _substr,
    size_t      _substr_len
)
{
    if ( (_str == NULL) ||
         (_substr == NULL) )
    {
        return D_STRING_NPOS;
    }

    // empty substring is always found at position 0
    if (_substr_len == 0)
    {
        return 0;
    }

    if (_substr_len > _str_len)
    {
        return D_STRING_NPOS;
    }

    const size_t limit = _str_len - _substr_len + 1;

    for (size_t i = 0; i < limit; i++)
    {
        if (memcmp(_str + i,
                   _substr,
                   _substr_len) == 0)
        {
            return (d_index)i;
        }
    }

    return D_STRING_NPOS;
}

/*
d_strstr_index_from
  Slides a memcmp window forward from _start; an empty substring matches at
_start itself when that lies within the string.
*/
d_index
d_strstr_index_from(
    const char* _str,
    size_t      _str_len,
    const char* _substr,
    size_t      _substr_len,
    size_t      _start
)
{
    if ( (_str == NULL) ||
         (_substr == NULL) )
    {
        return D_STRING_NPOS;
    }

    if (_substr_len == 0)
    {
        return (_start <= _str_len) ? (d_index)_start : D_STRING_NPOS;
    }

    if ( (_start >= _str_len) ||
         (_substr_len > _str_len - _start) )
    {
        return D_STRING_NPOS;
    }

    const size_t limit = _str_len - _substr_len + 1;

    for (size_t i = _start; i < limit; i++)
    {
        if (memcmp(_str + i,
                   _substr,
                   _substr_len) == 0)
        {
            return (d_index)i;
        }
    }

    return D_STRING_NPOS;
}

/*
d_strrstr_index
  Slides a memcmp window backward from the last possible position; an empty
substring matches at the end.
*/
d_index
d_strrstr_index(
    const char* _str,
    size_t      _str_len,
    const char* _substr,
    size_t      _substr_len
)
{
    if ( (_str == NULL) ||
         (_substr == NULL) )
    {
        return D_STRING_NPOS;
    }

    if (_substr_len == 0)
    {
        return (d_index)_str_len;
    }

    if (_substr_len > _str_len)
    {
        return D_STRING_NPOS;
    }

    // search backwards from the last possible position
    size_t i = _str_len - _substr_len + 1;

    while (i > 0)
    {
        i--;

        if (memcmp(_str + i,
                   _substr,
                   _substr_len) == 0)
        {
            return (d_index)i;
        }
    }

    return D_STRING_NPOS;
}

/*
d_strcasestr_index
  Slides a d_strncasecmp_n window forward. Like the case-sensitive searches,
it compares exactly _substr_len characters at each position, embedded NULs
included (decision 30 of the register; it went through d_strncasecmp, which
stops at a '\0').
*/
d_index
d_strcasestr_index(
    const char* _str,
    size_t      _str_len,
    const char* _substr,
    size_t      _substr_len
)
{
    if ( (_str == NULL) ||
         (_substr == NULL) )
    {
        return D_STRING_NPOS;
    }

    if (_substr_len == 0)
    {
        return 0;
    }

    if (_substr_len > _str_len)
    {
        return D_STRING_NPOS;
    }

    const size_t limit = _str_len - _substr_len + 1;

    for (size_t i = 0; i < limit; i++)
    {
        if (d_strncasecmp_n(_str + i,
                            _substr_len,
                            _substr,
                            _substr_len,
                            _substr_len) == 0)
        {
            return (d_index)i;
        }
    }

    return D_STRING_NPOS;
}

/******************************************************************************
 * xvii. IN-PLACE CHARACTER REPLACEMENT
 *****************************************************************************/

// character replacement
/*
d_strreplace_char
  Walks all _len bytes, rewriting matches in place and counting them.
*/
size_t
d_strreplace_char(
    char*  _str,
    size_t _len,
    char   _old,
    char   _new
)
{
    if (_str == NULL)
    {
        return 0;
    }

    size_t count = 0;

    for (size_t i = 0; i < _len; i++)
    {
        if (_str[i] == _old)
        {
            _str[i] = _new;
            count++;
        }
    }

    return count;
}
