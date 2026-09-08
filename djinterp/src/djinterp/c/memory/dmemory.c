/******************************************************************************
* djinterp [core]                                                    dmemory.c
*
*
* path:      /src/djinterp/c/dmemory.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2025.03.03
******************************************************************************/
#include "../../../../inc/djinterp/c/memory/dmemory.h"


/*
d_memcpy
  Cross-platform memory copy function that safely copies bytes from source
to destination buffer.

Parameter(s):
  _destination: pointer to the destination buffer where data will be copied.
  _source:      pointer to the source buffer containing data to copy.
  _amount:      number of bytes to copy from source to destination.

Return:
  A pointer value corresponding to either:
  - `_destination`, if the copy operation was successful, or
  - NULL, if any of the following conditions were true:
    - _destination was NULL,
    - _source was NULL.
*/
void*
d_memcpy(
    void*       _destination,
    const void* _source,
    size_t      _amount
)
{
    // validate the source and destination pointers
    if ( (!_destination) || 
         (!_source) )
    {
        return NULL;
    }

#if defined(_MSC_VER)
    // Microsoft compiler
    return memcpy(_destination, _source, _amount);

#elif ( defined(__GNUC__) ||                                                  \
        defined(__clang__) )
    // GCC or Clang
    return memcpy(_destination, _source, _amount);

#else
    // generic fallback
    char*       d = (char*)_destination;
    const char* s = (const char*)_source;

    while (_amount--)
    {
        *(d)++ = *(s)++;
    }

    return _destination;
#endif
}

/*
d_memcpy_s
  Cross-platform, secure memory copy function with bounds checking that
validates buffer sizes to prevent buffer overflows.

Parameter(s):
  _destination:      pointer to the destination buffer where data will be 
                     copied.
  _destination_size: size of the destination buffer in bytes.
  _source:           pointer to the source buffer containing data to copy.
  _amount:           number of bytes to copy from source to destination.
Return:
  An integer value corresponding to:
  - 0, if the copy operation was successful, or
  - EINVAL, if any of the following conditions were true:
    - _destination was NULL,
    - _source was NULL, or
  - ERANGE, if the following condition was true:
    - `_destination_size` was less than `_amount`.
*/
int
d_memcpy_s(
    void*       _destination,
    size_t      _destination_size,
    const void* _source,
    size_t      _amount
)
{
    // destination input validation
    if (_destination == NULL)
    {
        return EINVAL;
    }

    // destination input validation
    if (_source == NULL)
    {
        // clear destination buffer on error
        d_memset(_destination, 0, _destination_size);

        return EINVAL;
    }

    if (_destination_size < _amount)
    {
        // destination buffer too small
        d_memset(_destination, 0, _destination_size);
        return ERANGE;
    }

    // perform the copy operation
#if ( defined(_MSC_VER) &&                                                    \
      (_MSC_VER >= 1400) )
    // use secure function on Microsoft compilers
    return memcpy_s(_destination, _destination_size, _source, _amount);

#elif ( defined(__STDC_LIB_EXT1__) &&                                         \
        __STDC_WANT_LIB_EXT1__ )
    // use C11 secure functions if available
    return memcpy_s(_destination, _destSize, _source, _amount);

#else
    // use standard memcpy on other platforms
    memcpy(_destination, _source, _amount);
    return 0;
#endif
}

/*
d_memdup
  Cross-platform function for allocating a new memory region and copying the
specified number of bytes from the source into it.  Combines malloc and 
memcpy operations.

Parameter(s):
  _source: pointer to the source data to copy.
  _size:   number of bytes to copy.
Return:
  A pointer value corresponding to either:
  - a pointer to the newly allocated memory containing a copy of `_source`, if
    the operation was successful, or
  - NULL, if any of the following conditions were true:
    - `_source` was NULL,
    - memory allocation failed.
*/
void*
d_memdup(
    const void* _source,
    size_t      _size
)
{
    void* result;

    // validate the source pointer
    if (_source == NULL)
    {
        return NULL;
    }

    // allocate memory for the duplicate
    result = malloc(_size);

    // check if memory allocation was successful
    if (result == NULL)
    {

        return NULL;
    }

    // copy the source data into the duplicate
    if (d_memcpy_s(result,
                   _size,
                   _source,
                   _size) != 0)
    {
        free(result);

        return NULL;
    }

    return result;
}

/*
d_memdup_s
  Cross-platform, secure function for allocating a new memory region and 
copying the specified number of bytes from the source into it; combines
malloc and memcpy operations with proper error checking.

Parameter(s):
  _source: pointer to the source data to copy.
  _size:   size of the data to copy in bytes.
Return:
  A pointer value corresponding to either:
  - pointer to newly allocated memory containing a copy of _source, if the
    operation was successful, or
  - NULL, if any of the following conditions were true:
    - _source was NULL,
    - _size was 0,
    - memory allocation failed.
*/
void*
d_memdup_s(
    const void* _source,
    size_t      _size
)
{
    void* destination;

    // validate input parameters
    if ( (_source == NULL) || 
         (_size == 0) )
    {
        return NULL;
    }

    // attempt memory allocation
    destination = malloc(_size);

    if (destination == NULL)
    {
        return NULL;
    }

    // copy the data
    d_memcpy(destination, _source, _size);

    return destination;
}

/*
d_memset
  Cross-platform memory fill function that validates parameters to prevent
buffer overflows.

Parameter(s):
  _ptr:    pointer to the memory region to fill.
  _value:  the byte value to fill the memory with (converted to unsigned char).
  _amount: number of bytes to fill.
Return:
  A pointer value corresponding to either:
  - `_ptr`, if the operation was successful, or
  - NULL, if `_ptr` was NULL.
*/
void*
d_memset(
    void*  _ptr,
    int    _value,
    size_t _amount
)
{
    unsigned char* p;
    unsigned char  val;
    size_t         i;

    // validate the destination pointer
    if (_ptr == NULL)
    {
        return NULL;
    }

    p   = (unsigned char*)_ptr;
    val = (unsigned char)_value;

    // fill each byte with the value
    for (i = 0; i < _amount; i++)
    {
        p[i] = val;
    }
}

/*
d_memset_s
  Cross-platform, secure memory fill function with bounds checking that 
validates parameters to prevent buffer overflows.

Parameter(s):
  _destination:      pointer to the destination buffer to fill.
  _destination_size: size of the destination buffer in bytes.
  _ch:               byte value to fill the memory with.
  _count:            number of bytes to fill.
Return:
  An errno_t value corresponding to:
  - 0, if the operation was successful, or
  - EINVAL, if any of the following conditions were true:
    - _destination was NULL,
    - _destination_size was greater than or equal to RSIZE_MAX,
    - _count was greater than or equal to RSIZE_MAX, or
  - ERANGE, if the following condition was true:
    - _count was greater than _destination_size.
*/
errno_t
d_memset_s(
    void*   _destination,
    rsize_t _destination_size,
    int     _ch,
    rsize_t _count
)
{
    unsigned char  value;
    unsigned char* d;
    rsize_t        i;
    rsize_t        n;

    // parameter validation
    if (_destination == NULL)
    {
        return EINVAL;
    }

    if (_destination_size >= RSIZE_MAX)
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
    n = (_count < _destination_size) 
            ? _count
            : _destination_size;

    for (i = 0; i < n; i++) 
    {
        d[i] = value;
    }

    // if count > destsz, return error but still fill destsz bytes
    // use ERANGE for consistency with d_memcpy_s
    return (_count > _destination_size) 
        ? ERANGE 
        : 0;
}