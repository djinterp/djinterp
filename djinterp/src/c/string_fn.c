#include "..\inc\string_fn.h"


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
    char* token_start;

    if (_delim == NULL || _saveptr == NULL)
    {
        return NULL;
    }
    
    if (_str != NULL)
    {
        *_saveptr = _str;
    }
    
    token_start = *_saveptr;
    
    if (token_start == NULL)
    {
        return NULL;
    }
    
    // skip leading delimiters
    token_start += strspn(token_start, _delim);
    
    if (*token_start == '\0')
    {
        *_saveptr = NULL;
        return NULL;
    }
    
    // find end of token
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
    size_t len;

    if (_str == NULL)
    {
        return 0;
    }
    
    len = 0;
    
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
    size_t needle_len;

    if (_haystack == NULL || _needle == NULL)
    {
        return NULL;
    }
    
    if (*_needle == '\0')
    {
        return (char*)_haystack;
    }
    
    needle_len = strlen(_needle);
    
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
    char* original;

    if (_str == NULL)
    {
        return NULL;
    }
    
    original = _str;
    
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
    char* original;

    if (_str == NULL)
    {
        return NULL;
    }
    
    original = _str;
    
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
    size_t len;
    char*  start;
    char*  end;

    if (_str == NULL)
    {
        return NULL;
    }
    
    len = strlen(_str);
    
    if (len <= 1)
    {
        return _str;
    }
    
    start = _str;
    end   = _str + len - 1;
    
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
    size_t      msg_len;
    const char* msg;

    if ( (_buf    == NULL) || 
         (_buflen == 0) )
    {
        return EINVAL;
    }
    
    // simplified implementation;
    // a full implementation would have a proper error message table
    msg = "Unknown error";
    
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


/******************************************************************************
 * xi.   LENGTH-AWARE COMPARISON
 *****************************************************************************/

/*
d_strcmp_n
  Compare two strings with known lengths. Compares lexicographically up to
the shorter length, then falls back to length comparison.

Parameter(s):
  _s1:     first string buffer to compare
  _s1_len: length of first string (excluding null terminator)
  _s2:     second string buffer to compare
  _s2_len: length of second string (excluding null terminator)
Return:
  An integer value corresponding to:
  - negative if _s1 < _s2,
  - zero if _s1 == _s2, or
  - positive if _s1 > _s2
*/
int
d_strcmp_n
(
    const char* _s1,
    size_t      _s1_len,
    const char* _s2,
    size_t      _s2_len
)
{
    size_t min_len;
    int    result;

    // null handling
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

    // compare up to the shorter length
    min_len = (_s1_len < _s2_len) ? _s1_len : _s2_len;
    result  = memcmp(_s1, _s2, min_len);

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
  Compare at most _n characters of two strings with known lengths.

Parameter(s):
  _s1:     first string buffer to compare
  _s1_len: length of first string (excluding null terminator)
  _s2:     second string buffer to compare
  _s2_len: length of second string (excluding null terminator)
  _n:      maximum number of characters to compare
Return:
  An integer value corresponding to:
  - negative if _s1 < _s2,
  - zero if _s1 == _s2 within _n characters, or
  - positive if _s1 > _s2
*/
int
d_strncmp_n
(
    const char* _s1,
    size_t      _s1_len,
    const char* _s2,
    size_t      _s2_len,
    size_t      _n
)
{
    size_t cmp_len1;
    size_t cmp_len2;
    size_t min_len;
    int    result;

    if (_n == 0)
    {
        return 0;
    }

    // null handling
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

    // clamp effective lengths to _n
    cmp_len1 = (_s1_len < _n) ? _s1_len : _n;
    cmp_len2 = (_s2_len < _n) ? _s2_len : _n;
    min_len  = (cmp_len1 < cmp_len2) ? cmp_len1 : cmp_len2;

    result = memcmp(_s1, _s2, min_len);

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
  Compare two strings with known lengths, ignoring case.

Parameter(s):
  _s1:     first string buffer to compare
  _s1_len: length of first string (excluding null terminator)
  _s2:     second string buffer to compare
  _s2_len: length of second string (excluding null terminator)
Return:
  An integer value corresponding to:
  - negative if _s1 < _s2,
  - zero if _s1 == _s2 (case-insensitive), or
  - positive if _s1 > _s2
*/
int
d_strcasecmp_n
(
    const char* _s1,
    size_t      _s1_len,
    const char* _s2,
    size_t      _s2_len
)
{
    size_t min_len;
    size_t i;

    // null handling
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

    // compare character-by-character up to the shorter length
    min_len = (_s1_len < _s2_len) ? _s1_len : _s2_len;

    for (i = 0; i < min_len; i++)
    {
        int c1 = tolower((unsigned char)_s1[i]);
        int c2 = tolower((unsigned char)_s2[i]);

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
  Compare at most _n characters of two strings with known lengths,
ignoring case.

Parameter(s):
  _s1:     first string buffer to compare
  _s1_len: length of first string (excluding null terminator)
  _s2:     second string buffer to compare
  _s2_len: length of second string (excluding null terminator)
  _n:      maximum number of characters to compare
Return:
  An integer value corresponding to:
  - negative if _s1 < _s2,
  - zero if _s1 == _s2 within _n characters (case-insensitive), or
  - positive if _s1 > _s2
*/
int
d_strncasecmp_n
(
    const char* _s1,
    size_t      _s1_len,
    const char* _s2,
    size_t      _s2_len,
    size_t      _n
)
{
    size_t cmp_len1;
    size_t cmp_len2;
    size_t min_len;
    size_t i;

    if (_n == 0)
    {
        return 0;
    }

    // null handling
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

    // clamp effective lengths to _n
    cmp_len1 = (_s1_len < _n) ? _s1_len : _n;
    cmp_len2 = (_s2_len < _n) ? _s2_len : _n;
    min_len  = (cmp_len1 < cmp_len2) ? cmp_len1 : cmp_len2;

    for (i = 0; i < min_len; i++)
    {
        int c1 = tolower((unsigned char)_s1[i]);
        int c2 = tolower((unsigned char)_s2[i]);

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
  Check if two strings with known lengths are identical.
Short-circuits on length mismatch before comparing contents.

Parameter(s):
  _s1:     first string buffer to compare
  _s1_len: length of first string (excluding null terminator)
  _s2:     second string buffer to compare
  _s2_len: length of second string (excluding null terminator)
Return:
  A boolean value corresponding to either:
  - true, if both strings are identical (or both NULL), or
  - false, if the strings differ in length or content
*/
bool
d_strequals
(
    const char* _s1,
    size_t      _s1_len,
    const char* _s2,
    size_t      _s2_len
)
{
    // null handling
    if (_s1 == NULL && _s2 == NULL)
    {
        return true;
    }
    if (_s1 == NULL || _s2 == NULL)
    {
        return false;
    }

    // fast path: different lengths cannot be equal
    if (_s1_len != _s2_len)
    {
        return false;
    }

    return (memcmp(_s1, _s2, _s1_len) == 0);
}

/*
d_strequals_nocase
  Check if two strings with known lengths are identical, ignoring case.
Short-circuits on length mismatch before comparing contents.

Parameter(s):
  _s1:     first string buffer to compare
  _s1_len: length of first string (excluding null terminator)
  _s2:     second string buffer to compare
  _s2_len: length of second string (excluding null terminator)
Return:
  A boolean value corresponding to either:
  - true, if both strings are equal ignoring case (or both NULL), or
  - false, if the strings differ
*/
bool
d_strequals_nocase
(
    const char* _s1,
    size_t      _s1_len,
    const char* _s2,
    size_t      _s2_len
)
{
    size_t i;

    // null handling
    if (_s1 == NULL && _s2 == NULL)
    {
        return true;
    }
    if (_s1 == NULL || _s2 == NULL)
    {
        return false;
    }

    // fast path: different lengths cannot be equal
    if (_s1_len != _s2_len)
    {
        return false;
    }

    for (i = 0; i < _s1_len; i++)
    {
        if (tolower((unsigned char)_s1[i]) !=
            tolower((unsigned char)_s2[i]))
        {
            return false;
        }
    }

    return true;
}


/******************************************************************************
 * xii.  VALIDATION
 *****************************************************************************/

/*
d_str_is_valid
  Check if a buffer of the given length contains a valid null-terminated
string (no embedded null bytes).

Parameter(s):
  _text:   pointer to the character buffer to validate
  _length: expected length of the string (excluding null terminator)
Return:
  A boolean value corresponding to either:
  - true, if _text is non-NULL and contains no embedded nulls within
    _length bytes, or
  - false, if _text is NULL or contains embedded null bytes
*/
bool
d_str_is_valid
(
    const char* _text,
    size_t      _length
)
{
    size_t i;

    if (_text == NULL)
    {
        return false;
    }

    for (i = 0; i < _length; i++)
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
  Check if all characters in the buffer are 7-bit ASCII (0x00-0x7F).

Parameter(s):
  _text:   pointer to the character buffer to check
  _length: number of characters to examine
Return:
  A boolean value corresponding to either:
  - true, if all characters are ASCII, or
  - false, if _text is NULL or any character has bit 7 set
*/
bool
d_str_is_ascii
(
    const char* _text,
    size_t      _length
)
{
    size_t i;

    if (_text == NULL)
    {
        return false;
    }

    for (i = 0; i < _length; i++)
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
  Check if all characters in the buffer are decimal digits ('0'-'9').

Parameter(s):
  _text:   pointer to the character buffer to check
  _length: number of characters to examine
Return:
  A boolean value corresponding to either:
  - true, if _length > 0 and all characters are digits, or
  - false, if _text is NULL, _length is 0, or any character is not a digit
*/
bool
d_str_is_numeric
(
    const char* _text,
    size_t      _length
)
{
    size_t i;

    if ( (_text == NULL) || 
         (_length == 0) )
    {
        return false;
    }

    for (i = 0; i < _length; i++)
    {
        if (!isdigit((unsigned char)_text[i]))
        {
            return false;
        }
    }

    return true;
}

/*
d_str_is_alpha
  Check if all characters in the buffer are alphabetic.

Parameter(s):
  _text:   pointer to the character buffer to check
  _length: number of characters to examine
Return:
  A boolean value corresponding to either:
  - true, if _length > 0 and all characters are alphabetic, or
  - false, if _text is NULL, _length is 0, or any character is not
    alphabetic
*/
bool
d_str_is_alpha
(
    const char* _text,
    size_t      _length
)
{
    size_t i;

    if ( (_text == NULL) || 
         (_length == 0) )
    {
        return false;
    }

    for (i = 0; i < _length; i++)
    {
        if (!isalpha((unsigned char)_text[i]))
        {
            return false;
        }
    }

    return true;
}

/*
d_str_is_alnum
  Check if all characters in the buffer are alphanumeric.

Parameter(s):
  _text:   pointer to the character buffer to check
  _length: number of characters to examine
Return:
  A boolean value corresponding to either:
  - true, if _length > 0 and all characters are alphanumeric, or
  - false, if _text is NULL, _length is 0, or any character is not
    alphanumeric
*/
bool
d_str_is_alnum
(
    const char* _text,
    size_t      _length
)
{
    size_t i;

    if ( (_text == NULL) || 
         (_length == 0) )
    {
        return false;
    }

    for (i = 0; i < _length; i++)
    {
        if (!isalnum((unsigned char)_text[i]))
        {
            return false;
        }
    }

    return true;
}

/*
d_str_is_whitespace
  Check if all characters in the buffer are whitespace.

Parameter(s):
  _text:   pointer to the character buffer to check
  _length: number of characters to examine
Return:
  A boolean value corresponding to either:
  - true, if _length > 0 and all characters are whitespace, or
  - false, if _text is NULL, _length is 0, or any character is not
    whitespace
*/
bool
d_str_is_whitespace
(
    const char* _text,
    size_t      _length
)
{
    size_t i;

    if ( (_text == NULL) || 
         (_length == 0) )
    {
        return false;
    }

    for (i = 0; i < _length; i++)
    {
        if (!isspace((unsigned char)_text[i]))
        {
            return false;
        }
    }

    return true;
}


/******************************************************************************
 * xiii. COUNTING
 *****************************************************************************/

/*
d_strcount_char
  Count the number of occurrences of a character in a buffer.

Parameter(s):
  _str: pointer to the character buffer to search
  _len: length of the buffer to examine
  _c:   character to count
Return:
  The number of times _c appears in the first _len bytes of _str, or
  0 if _str is NULL.
*/
size_t
d_strcount_char
(
    const char* _str,
    size_t      _len,
    char        _c
)
{
    size_t count;
    size_t i;

    if (_str == NULL)
    {
        return 0;
    }

    count = 0;

    for (i = 0; i < _len; i++)
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
  Count the number of non-overlapping occurrences of a substring in a
buffer.

Parameter(s):
  _str:    pointer to the character buffer to search
  _len:    length of the buffer to examine
  _substr: null-terminated substring to count
Return:
  The number of non-overlapping occurrences of _substr in _str, or
  0 if _str or _substr is NULL, or _substr is empty.
*/
size_t
d_strcount_substr
(
    const char* _str,
    size_t      _len,
    const char* _substr
)
{
    size_t      count;
    size_t      substr_len;
    const char* pos;
    const char* end;

    if ( (_str == NULL)    || 
         (_substr == NULL) || 
         (*_substr == '\0') )
    {
        return 0;
    }

    count      = 0;
    substr_len = strlen(_substr);

    if (substr_len > _len)
    {
        return 0;
    }

    pos = _str;
    end = _str + _len - substr_len + 1;

    while (pos < end)
    {
        if (memcmp(pos, _substr, substr_len) == 0)
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

/*
d_strhash
  Compute a hash value for a string buffer using the djb2 algorithm.

Parameter(s):
  _str: pointer to the character buffer to hash
  _len: length of the buffer
Return:
  A hash value for the string, or 0 if _str is NULL.
*/
size_t
d_strhash
(
    const char* _str,
    size_t      _len
)
{
    size_t hash;
    size_t i;

    if (_str == NULL)
    {
        return 0;
    }

    hash = 5381;

    for (i = 0; i < _len; i++)
    {
        hash = ((hash << 5) + hash) + (unsigned char)_str[i];
    }

    return hash;
}


/******************************************************************************
 * xv.   PREFIX, SUFFIX, AND CONTAINMENT
 *****************************************************************************/

/*
d_strstartswith
  Check if a string starts with a given prefix.

Parameter(s):
  _str:        pointer to the string buffer to check
  _str_len:    length of the string
  _prefix:     pointer to the prefix buffer
  _prefix_len: length of the prefix
Return:
  A boolean value corresponding to either:
  - true, if _str begins with _prefix, or
  - false, if it does not, or if either pointer is NULL
*/
bool
d_strstartswith
(
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

    return (memcmp(_str, _prefix, _prefix_len) == 0);
}

/*
d_strendswith
  Check if a string ends with a given suffix.

Parameter(s):
  _str:        pointer to the string buffer to check
  _str_len:    length of the string
  _suffix:     pointer to the suffix buffer
  _suffix_len: length of the suffix
Return:
  A boolean value corresponding to either:
  - true, if _str ends with _suffix, or
  - false, if it does not, or if either pointer is NULL
*/
bool
d_strendswith
(
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
  Check if a string contains a given null-terminated substring.

Parameter(s):
  _str:     pointer to the string buffer to search
  _str_len: length of the string
  _substr:  null-terminated substring to find
Return:
  A boolean value corresponding to either:
  - true, if _substr is found within _str, or
  - false, if it is not found, or if either pointer is NULL
*/
bool
d_strcontains
(
    const char* _str,
    size_t      _str_len,
    const char* _substr
)
{
    size_t      substr_len;
    const char* end;
    const char* pos;

    if ( (_str == NULL) || 
         (_substr == NULL) )
    {
        return false;
    }

    if (*_substr == '\0')
    {
        return true;
    }

    substr_len = strlen(_substr);

    if (substr_len > _str_len)
    {
        return false;
    }

    end = _str + _str_len - substr_len + 1;
    pos = _str;

    while (pos < end)
    {
        if (memcmp(pos, _substr, substr_len) == 0)
        {
            return true;
        }
        pos++;
    }

    return false;
}

/*
d_strcontains_char
  Check if a string contains a given character.

Parameter(s):
  _str:     pointer to the string buffer to search
  _str_len: length of the string
  _c:       character to find
Return:
  A boolean value corresponding to either:
  - true, if _c is found within _str, or
  - false, if it is not found, or if _str is NULL
*/
bool
d_strcontains_char
(
    const char* _str,
    size_t      _str_len,
    char        _c
)
{
    size_t i;

    if (_str == NULL)
    {
        return false;
    }

    for (i = 0; i < _str_len; i++)
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

/*
d_strchr_index
  Find the index of the first occurrence of a character in a buffer.

Parameter(s):
  _str: pointer to the character buffer to search
  _len: length of the buffer
  _c:   character to find
Return:
  A d_index value corresponding to either:
  - the zero-based index of the first occurrence of _c, or
  - D_STRING_NPOS if _c is not found or _str is NULL
*/
d_index
d_strchr_index
(
    const char* _str,
    size_t      _len,
    char        _c
)
{
    size_t i;

    if (_str == NULL)
    {
        return D_STRING_NPOS;
    }

    for (i = 0; i < _len; i++)
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
  Find the index of the first occurrence of a character in a buffer,
starting from a given offset.

Parameter(s):
  _str:   pointer to the character buffer to search
  _len:   length of the buffer
  _c:     character to find
  _start: index at which to begin the search
Return:
  A d_index value corresponding to either:
  - the zero-based index of the first occurrence of _c at or after
    _start, or
  - D_STRING_NPOS if _c is not found, _str is NULL, or _start >= _len
*/
d_index
d_strchr_index_from
(
    const char* _str,
    size_t      _len,
    char        _c,
    size_t      _start
)
{
    size_t i;

    if ( (_str == NULL) || 
         (_start >= _len) )
    {
        return D_STRING_NPOS;
    }

    for (i = _start; i < _len; i++)
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
  Find the index of the last occurrence of a character in a buffer.

Parameter(s):
  _str: pointer to the character buffer to search
  _len: length of the buffer
  _c:   character to find
Return:
  A d_index value corresponding to either:
  - the zero-based index of the last occurrence of _c, or
  - D_STRING_NPOS if _c is not found or _str is NULL
*/
d_index
d_strrchr_index
(
    const char* _str,
    size_t      _len,
    char        _c
)
{
    size_t i;

    if ( (_str == NULL) || 
         (_len == 0) )
    {
        return D_STRING_NPOS;
    }

    // search backwards from end
    i = _len;

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
  Find the index of the first occurrence of a substring in a buffer.

Parameter(s):
  _str:        pointer to the string buffer to search
  _str_len:    length of the string
  _substr:     pointer to the substring buffer to find
  _substr_len: length of the substring
Return:
  A d_index value corresponding to either:
  - the zero-based index of the first occurrence of _substr, or
  - D_STRING_NPOS if _substr is not found, or if either pointer is NULL
*/
d_index
d_strstr_index
(
    const char* _str,
    size_t      _str_len,
    const char* _substr,
    size_t      _substr_len
)
{
    size_t i;
    size_t limit;

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

    limit = _str_len - _substr_len + 1;

    for (i = 0; i < limit; i++)
    {
        if (memcmp(_str + i, _substr, _substr_len) == 0)
        {
            return (d_index)i;
        }
    }

    return D_STRING_NPOS;
}

/*
d_strstr_index_from
  Find the index of the first occurrence of a substring in a buffer,
starting from a given offset.

Parameter(s):
  _str:        pointer to the string buffer to search
  _str_len:    length of the string
  _substr:     pointer to the substring buffer to find
  _substr_len: length of the substring
  _start:      index at which to begin the search
Return:
  A d_index value corresponding to either:
  - the zero-based index of the first occurrence of _substr at or after
    _start, or
  - D_STRING_NPOS if _substr is not found, or if either pointer is NULL
*/
d_index
d_strstr_index_from
(
    const char* _str,
    size_t      _str_len,
    const char* _substr,
    size_t      _substr_len,
    size_t      _start
)
{
    size_t i;
    size_t limit;

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

    limit = _str_len - _substr_len + 1;

    for (i = _start; i < limit; i++)
    {
        if (memcmp(_str + i, _substr, _substr_len) == 0)
        {
            return (d_index)i;
        }
    }

    return D_STRING_NPOS;
}

/*
d_strrstr_index
  Find the index of the last occurrence of a substring in a buffer.

Parameter(s):
  _str:        pointer to the string buffer to search
  _str_len:    length of the string
  _substr:     pointer to the substring buffer to find
  _substr_len: length of the substring
Return:
  A d_index value corresponding to either:
  - the zero-based index of the last occurrence of _substr, or
  - D_STRING_NPOS if _substr is not found, or if either pointer is NULL
*/
d_index
d_strrstr_index
(
    const char* _str,
    size_t      _str_len,
    const char* _substr,
    size_t      _substr_len
)
{
    size_t i;

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
    i = _str_len - _substr_len + 1;

    while (i > 0)
    {
        i--;

        if (memcmp(_str + i, _substr, _substr_len) == 0)
        {
            return (d_index)i;
        }
    }

    return D_STRING_NPOS;
}

/*
d_strcasestr_index
  Find the index of the first case-insensitive occurrence of a substring
in a buffer.

Parameter(s):
  _str:        pointer to the string buffer to search
  _str_len:    length of the string
  _substr:     pointer to the substring buffer to find
  _substr_len: length of the substring
Return:
  A d_index value corresponding to either:
  - the zero-based index of the first case-insensitive match, or
  - D_STRING_NPOS if _substr is not found, or if either pointer is NULL
*/
d_index
d_strcasestr_index
(
    const char* _str,
    size_t      _str_len,
    const char* _substr,
    size_t      _substr_len
)
{
    size_t i;
    size_t limit;

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

    limit = _str_len - _substr_len + 1;

    for (i = 0; i < limit; i++)
    {
        if (d_strncasecmp(_str + i, _substr, _substr_len) == 0)
        {
            return (d_index)i;
        }
    }

    return D_STRING_NPOS;
}


/******************************************************************************
 * xvii. IN-PLACE CHARACTER REPLACEMENT
 *****************************************************************************/

/*
d_strreplace_char
  Replace all occurrences of a character with another character in a
buffer, in-place.

Parameter(s):
  _str: pointer to the mutable character buffer
  _len: length of the buffer
  _old: character to find and replace
  _new: character to substitute in place of _old
Return:
  The number of replacements made, or 0 if _str is NULL.
*/
size_t
d_strreplace_char
(
    char*  _str,
    size_t _len,
    char   _old,
    char   _new
)
{
    size_t count;
    size_t i;

    if (_str == NULL)
    {
        return 0;
    }

    count = 0;

    for (i = 0; i < _len; i++)
    {
        if (_str[i] == _old)
        {
            _str[i] = _new;
            count++;
        }
    }

    return count;
}