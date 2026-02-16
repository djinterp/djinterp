#include "..\..\inc\c\string_fn.h"


/*
d_strcpy_s
  Safe string copy function compatible with C11 strcpy_s.

Parameter(s):
  _destination:      destination buffer to copy string into
  _dest_size: size of destination buffer including null terminator space
  _src:       source null-terminated string to copy
Return:
  An integer value corresponding to either:
  0 on success, or error code:
  - EINVAL if _destination or _src is NULL
  - ERANGE if _dest_size is 0 or _src is too long to fit in _destination
*/
int
d_strcpy_s
(
    char* restrict       _destination,
    size_t               _dest_size,
    const char* restrict _src
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
    
    if (_src == NULL)
    {
        _destination[0] = '\0';
        return EINVAL;
    }
    
    size_t src_len = strlen(_src);
    
    if (src_len >= _dest_size)
    {
        _destination[0] = '\0';
        return ERANGE;
    }
    
    d_memcpy(_destination, _src, src_len + 1);
    return 0;
}

/*
d_strncpy_s
  Safe bounded string copy function compatible with C11 strncpy_s.

Parameter(s):
  _destination:      destination buffer to copy string into
  _dest_size: size of destination buffer including null terminator space
  _src:       source null-terminated string to copy
  _count:     maximum number of characters to copy from source
Return:
  0 on success, or error code:
  - EINVAL if _destination or _src is NULL
  - ERANGE if _dest_size is 0 or result would not fit in _destination
*/
int
d_strncpy_s
(
    char* restrict       _destination,
    size_t              _dest_size,
    const char* restrict _src,
    size_t              _count
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
    
    if (_src == NULL)
    {
        _destination[0] = '\0';
        return EINVAL;
    }
    
    size_t src_len = d_strnlen(_src, _count);
    
    if (src_len >= _dest_size)
    {
        _destination[0] = '\0';
        return ERANGE;
    }
    
    d_memcpy(_destination, _src, src_len);
    _destination[src_len] = '\0';
    return 0;
}

/*
d_strcat_s
  Safe string concatenation function compatible with C11 strcat_s.

Parameter(s):
  _destination:      destination buffer containing existing string
  _dest_size: total size of destination buffer including null terminator space
  _src:       source null-terminated string to append
Return:
  0 on success, or error code:
  - EINVAL if _destination or _src is NULL
  - ERANGE if result would not fit in _destination
*/
int
d_strcat_s
(
    char* restrict       _destination,
    size_t              _dest_size,
    const char* restrict _src
)
{
    if (_destination == NULL || _src == NULL)
    {
        if (_destination != NULL && _dest_size > 0)
        {
            _destination[0] = '\0';
        }
        return EINVAL;
    }
    
    if (_dest_size == 0)
    {
        return ERANGE;
    }
    
    size_t dest_len = d_strnlen(_destination, _dest_size);
    size_t src_len = strlen(_src);
    
    if (dest_len + src_len >= _dest_size)
    {
        _destination[0] = '\0';
        return ERANGE;
    }
    
    d_memcpy(_destination + dest_len, _src, src_len + 1);
    return 0;
}

/*
d_strncat_s
  Safe bounded string concatenation function compatible with C11 strncat_s.

Parameter(s):
  _destination:      destination buffer containing existing string
  _dest_size: total size of destination buffer including null terminator space
  _src:       source null-terminated string to append
  _count:     maximum number of characters to append from source
Return:
  0 on success, or error code:
  - EINVAL if _destination or _src is NULL
  - ERANGE if result would not fit in _destination
*/
int
d_strncat_s
(
    char* restrict       _destination,
    size_t              _dest_size,
    const char* restrict _src,
    size_t              _count
)
{
    if (_destination == NULL || _src == NULL)
    {
        if (_destination != NULL && _dest_size > 0)
        {
            _destination[0] = '\0';
        }
        return EINVAL;
    }
    
    if (_dest_size == 0)
    {
        return ERANGE;
    }
    
    size_t dest_len = d_strnlen(_destination, _dest_size);
    size_t src_len = d_strnlen(_src, _count);
    
    if (dest_len + src_len >= _dest_size)
    {
        _destination[0] = '\0';
        return ERANGE;
    }
    
    d_memcpy(_destination + dest_len, _src, src_len);
    _destination[dest_len + src_len] = '\0';
    return 0;
}

/*
d_strdup
  Duplicate a string by allocating memory and copying contents.

Parameter(s):
  _str: null-terminated string to duplicate
Return:
  Pointer to newly allocated string copy, or NULL if:
  - _str is NULL, or
  - memory allocation fails
*/
char*
d_strdup
(
    const char* _str
)
{
    if (_str == NULL)
    {
        return NULL;
    }
    
    size_t len = strlen(_str) + 1;
    char* copy = malloc(len);
    
    if (copy != NULL)
    {
        d_memcpy(copy, _str, len);
    }
    
    return copy;
}

/*
d_strndup
  Duplicate at most n characters of a string by allocating memory.

Parameter(s):
  _str: null-terminated string to duplicate
  _n:   maximum number of characters to copy
Return:
  Pointer to newly allocated string copy, or NULL if:
  - _str is NULL, or
  - memory allocation fails
*/
char*
d_strndup
(
    const char* _str,
    size_t     _n
)
{
    if (_str == NULL)
    {
        return NULL;
    }
    
    size_t len = d_strnlen(_str, _n);
    char* copy = malloc(len + 1);
    
    if (copy != NULL)
    {
        d_memcpy(copy, _str, len);
        copy[len] = '\0';
    }
    
    return copy;
}

/*
d_strcasecmp
  Compare two strings ignoring case differences.

Parameter(s):
  _s1: first null-terminated string to compare
  _s2: second null-terminated string to compare
Return:
  Integer value indicating relationship:
  - negative if _s1 < _s2,
  - zero if _s1 == _s2, or
  - positive if _s1 > _s2
*/
int
d_strcasecmp
(
    const char* _s1,
    const char* _s2
)
{
    if (_s1 == NULL && _s2 == NULL)
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
    
    while ( *(_s1) && 
            *(_s2) )
    {
        int c1 = tolower((unsigned char)*_s1);
        int c2 = tolower((unsigned char)*_s2);
        
        if (c1 != c2)
        {
            return c1 - c2;
        }
        
        _s1++;
        _s2++;
    }
    
    return tolower((unsigned char)*_s1) - tolower((unsigned char)*_s2);
}

/*
d_strncasecmp
  Compare at most n characters of two strings ignoring case differences.

Parameter(s):
  _s1: first null-terminated string to compare
  _s2: second null-terminated string to compare
  _n:  maximum number of characters to compare
Return:
  Integer value indicating relationship:
  - negative if _s1 < _s2,
  - zero if _s1 == _s2, or
  - positive if _s1 > _s2
*/
int
d_strncasecmp
(
    const char* _s1,
    const char* _s2,
    size_t     _n
)
{
    if (_n == 0)
    {
        return 0;
    }
    
    if (_s1 == NULL && _s2 == NULL)
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
    
    while (_n > 0 && *_s1 && *_s2)
    {
        int c1 = tolower((unsigned char)*_s1);
        int c2 = tolower((unsigned char)*_s2);
        
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
    
    return tolower((unsigned char)*_s1) - tolower((unsigned char)*_s2);
}

/*
d_strtok_r
  Thread-safe string tokenization function.

Parameter(s):
  _str:     string to tokenize (NULL to continue previous tokenization)
  _delim:   null-terminated string containing delimiter characters
  _saveptr: pointer to char* used internally to maintain state
Return:
  Pointer to next token, or NULL if no more tokens exist
*/
char*
d_strtok_r
(
    char* restrict       _str,
    const char* restrict _delim,
    char** restrict     _saveptr
)
{
    if (_delim == NULL || _saveptr == NULL)
    {
        return NULL;
    }
    
    char* token_start;
    
    if (_str != NULL)
    {
        *_saveptr = _str;
    }
    
    token_start = *_saveptr;
    
    if (token_start == NULL)
    {
        return NULL;
    }
    
    // Skip leading delimiters
    token_start += strspn(token_start, _delim);
    
    if (*token_start == '\0')
    {
        *_saveptr = NULL;
        return NULL;
    }
    
    // Find end of token
    char* token_end = strpbrk(token_start, _delim);
    
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

/*
d_strnlen
  Get length of string with maximum limit.

Parameter(s):
  _str:    null-terminated string to measure
  _maxlen: maximum number of characters to examine
Return:
  Length of string, or _maxlen if string is longer than _maxlen
*/
size_t
d_strnlen
(
    const char* _str,
    size_t     _maxlen
)
{
    if (_str == NULL)
    {
        return 0;
    }
    
    size_t len = 0;
    
    while (len < _maxlen && _str[len] != '\0')
    {
        len++;
    }
    
    return len;
}

/*
d_strcasestr
  Find first occurrence of substring in string, ignoring case.

Parameter(s):
  _haystack: null-terminated string to search within
  _needle:   null-terminated substring to search for
Return:
  Pointer to first occurrence of _needle in _haystack, or NULL if:
  - _needle is not found, or
  - either parameter is NULL
*/
char*
d_strcasestr
(
    const char* _haystack,
    const char* _needle
)
{
    if (_haystack == NULL || _needle == NULL)
    {
        return NULL;
    }
    
    if (*_needle == '\0')
    {
        return (char*)_haystack;
    }
    
    size_t needle_len = strlen(_needle);
    
    while (*_haystack != '\0')
    {
        if (d_strncasecmp(_haystack, _needle, needle_len) == 0)
        {
            return (char*)_haystack;
        }
        _haystack++;
    }
    
    return NULL;
}

/*
d_strlwr
  Convert string to lowercase in-place.

Parameter(s):
  _str: null-terminated string to convert
Return:
  Pointer to the modified string, or NULL if _str is NULL
*/
char*
d_strlwr
(
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
        *_str = (char)tolower((unsigned char)*_str);
        _str++;
    }
    
    return original;
}

/*
d_strupr
  Convert string to uppercase in-place.

Parameter(s):
  _str: null-terminated string to convert
Return:
  Pointer to the modified string, or NULL if _str is NULL
*/
char*
d_strupr
(
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
        *_str = (char)toupper((unsigned char)*_str);
        _str++;
    }
    
    return original;
}

/*
d_strrev
  Reverse a string in-place.

Parameter(s):
  _str: null-terminated string to reverse
Return:
  Pointer to the reversed string, or NULL if _str is NULL
*/
char*
d_strrev
(
    char* _str
)
{
    if (_str == NULL)
    {
        return NULL;
    }
    
    size_t len = strlen(_str);
    
    if (len <= 1)
    {
        return _str;
    }
    
    char* start = _str;
    char* end = _str + len - 1;
    
    while (start < end)
    {
        char temp = *start;
        *start = *end;
        *end = temp;
        start++;
        end--;
    }
    
    return _str;
}

/*
d_strchrnul
  Find character in string or return pointer to end of string.

Parameter(s):
  _str: null-terminated string to search
  _c:   character to find
Return:
  Pointer to first occurrence of _c in _str, or pointer to null terminator
  if _c is not found. Returns NULL only if _str is NULL.
*/
char*
d_strchrnul
(
    const char* _str,
    int        _c
)
{
    if (_str == NULL)
    {
        return NULL;
    }
    
    while (*_str != '\0' && *_str != (char)_c)
    {
        _str++;
    }
    
    return (char*)_str;
}

/*
d_strerror_r
  Thread-safe error string function.

Parameter(s):
  _errnum: error number to get description for
  _buf:    buffer to store error string
  _buflen: size of buffer
Return:
  0 on success, or error code if buffer is too small or invalid parameters
*/
int
d_strerror_r
(
    int    _errnum,
    char*  _buf,
    size_t _buflen
)
{
    size_t msg_len;

    if ( (_buf    == NULL) || 
         (_buflen == 0) )
    {
        return EINVAL;
    }
    
    // this is a simplified implementation
    // in a full implementation, you'd have a proper error message table
    const char* msg = "Unknown error";
    
    switch (_errnum)
    {
        case 0:        msg = "success";          break;
        case EINVAL:   msg = "Invalid argument"; break;
        case ERANGE:   msg = "Result too large"; break;
        default:       msg = "Unknown error";    break;
    }
    
    msg_len = strlen(msg);
    
    if (msg_len >= _buflen)
    {
        return ERANGE;
    }
    
    d_memcpy(_buf, msg, msg_len + 1);

    return 0;
}

