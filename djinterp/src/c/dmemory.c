#include "..\..\inc\c\dmemory.h"


/*
d_memcpy
  Cross-platform memory copy function that safely copies bytes from source
to destination buffer.

Parameter(s):
  _destination: pointer to the destination buffer where data will be copied.
  _source:      pointer to the source buffer containing data to copy.
  _count:       number of bytes to copy from source to destination.

Return:
  A pointer value corresponding to either:
  - `_destination`, if the copy operation was successful, or
  - NULL, if any of the following conditions were true:
    - _destination was NULL,
    - _source was NULL.
*/
void*
d_memcpy
(
    void*       _destination,
    const void* _source,
    size_t      _count
)
{
    if ( (_destination == NULL) || 
         (_source == NULL) )
    {
        return NULL;
    }

#if defined(_MSC_VER)
    // Microsoft compiler
    return memcpy(_destination, _source, _count);

#elif defined(__GNUC__) || defined(__clang__)
    // GCC or Clang
    return memcpy(_destination, _source, _count);

#else
    // generic fallback
    char*       d = (char*)_destination;
    const char* s = (const char*)_source;

    while (_count--) 
    {
        *(d)++ = *(s)++;
    }

    return _destination;
#endif
}

/*
d_memcpy_s
  Secure cross-platform memory copy function with bounds checking that
  validates buffer sizes to prevent buffer overflows.

Parameter(s):
  _destination: Pointer to the destination buffer where data will be copied.
  _destSize:    Size of the destination buffer in bytes.
  _source:      Pointer to the source buffer containing data to copy.
  _count:       Number of bytes to copy from source to destination.
Return:
  An integer value corresponding to either:
  - 0, if the copy operation was successful, or
  - EINVAL, if any of the following conditions were true:
    - _destination was NULL,
    - _source was NULL, or
  - ERANGE, if the following condition was true:
    - _destSize was less than _count.
*/
int
d_memcpy_s
(
    void*       _destination,
    size_t      _destSize,
    const void* _source,
    size_t      _count
)
{
    // input validation
    if (_destination == NULL)
    {
        return EINVAL;
    }

    if (_source == NULL)
    {
        // clear destination buffer on error
        memset(_destination, 0, _destSize);
        return EINVAL;
    }

    if (_destSize < _count)
    {
        // destination buffer too small
        memset(_destination, 0, _destSize);
        return ERANGE;
    }

    // perform the copy operation
#if defined(_MSC_VER) && _MSC_VER >= 1400
    // use secure function on Microsoft compilers
    return memcpy_s(_destination, _destSize, _source, _count);

#elif defined(__STDC_LIB_EXT1__) && __STDC_WANT_LIB_EXT1__
    // use C11 secure functions if available
    return memcpy_s(_destination, _destSize, _source, _count);

#else
    // use standard memcpy on other platforms
    memcpy(_destination, _source, _count);
    return 0;

#endif
}

/*
d_memdup_s
  Safely allocate memory and copy data into it, combining malloc and memcpy
  operations with proper error checking.

Parameter(s):
  _src:  Pointer to the source data to copy.
  _size: Size of the data to copy in bytes.
Return:
  A pointer value corresponding to either:
  - pointer to newly allocated memory containing a copy of _src, if the
    operation was successful, or
  - NULL, if any of the following conditions were true:
    - _src was NULL,
    - _size was 0,
    - memory allocation failed.
*/
void*
d_memdup_s
(
    const void* _src,
    size_t      _size
)
{
    void* dest;

    // validate input parameters
    if ( (_src == NULL) || 
         (_size == 0) )
    {
        return NULL;
    }

    // attempt memory allocation
    dest = malloc(_size);

    if (dest == NULL)
    {
        return NULL;
    }

    // copy the data
    memcpy(dest, _src, _size);

    return dest;
}

/*
d_memset
  Fill a memory region with a specified byte value using a simple loop-based
  implementation.

Parameter(s):
  _ptr:    Pointer to the memory region to fill.
  _value:  The byte value to fill the memory with (converted to unsigned char).
  _amount: Number of bytes to fill.
Return:
  A pointer value corresponding to either:
  - _ptr, if the operation was successful, or
  - NULL, if _ptr was NULL.
*/
void*
d_memset
(
    void*  _ptr,
    int    _value,
    size_t _amount
)
{
    unsigned char* p;
    unsigned char  val;

    if (_ptr == NULL)
    {
        return NULL;
    }

    p   = (unsigned char*)_ptr;
    val = (unsigned char)_value;

    // fill each byte with the value
    for (size_t i = 0; i < _amount; i++)
    {
        p[i] = val;
    }

    return _ptr;
}

/*
d_memset_s
  Secure memory fill function with bounds checking that validates parameters
  to prevent buffer overflows.

Parameter(s):
  _destination:   Pointer to the destination buffer to fill.
  _destsz: Size of the destination buffer in bytes.
  _ch:     The byte value to fill the memory with.
  _count:  Number of bytes to fill.
Return:
  An errno_t value corresponding to either:
  - 0, if the operation was successful, or
  - EINVAL, if any of the following conditions were true:
    - _destination was NULL,
    - _destsz was greater than or equal to RSIZE_MAX,
    - _count was greater than or equal to RSIZE_MAX, or
  - ERANGE, if the following condition was true:
    - _count was greater than _destsz.
*/
errno_t
d_memset_s
(
    void*   _destination,
    rsize_t _destsz,
    int     _ch,
    rsize_t _count
)
{
    unsigned char  value;
    unsigned char* d;
    rsize_t        n;

    // parameter validation
    if (_destination == NULL)
    {
        return EINVAL;
    }

    if (_destsz >= RSIZE_MAX)
    {
        return EINVAL;
    }

    if (_count >= RSIZE_MAX)
    {
        return EINVAL;
    }

    value = (unsigned char)_ch;
    d     = (unsigned char*)_destination;

    // fill the lesser of count or destsz bytes
    n = (_count < _destsz) ? _count : _destsz;

    for (rsize_t i = 0; i < n; i++) 
    {
        d[i] = value;
    }

    // if count > destsz, return error but still fill destsz bytes
    // use ERANGE for consistency with d_memcpy_s
    return (_count > _destsz) 
        ? ERANGE 
        : 0;
}

