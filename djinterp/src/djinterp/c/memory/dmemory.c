/******************************************************************************
* djinterp [core]                                                    dmemory.c
*
*
* path:      /src/djinterp/c/memory/dmemory.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2025.03.03
*                                                          revised: 2026.09.09
******************************************************************************/
#include "../../../../inc/djinterp/c/memory/dmemory.h"


/*
d_memcpy
  Copies _amount bytes from _source to _destination after validating both
pointers.

Parameter(s):
  _destination: pointer to the destination buffer.
  _source:      pointer to the source buffer.
  _amount:      number of bytes to copy.
Return:
  _destination on success; NULL if _destination or _source is NULL.
*/
void*
d_memcpy(
    void*       _destination,
    const void* _source,
    size_t      _amount
)
{
    // validate source, destination pointers
    if ( (!_destination) ||
         (!_source) )
    {

        return NULL;
    }

    return memcpy(_destination, _source, _amount);
}


/*
d_memcpy_s
  Copies _amount bytes from _source to _destination with destination-bound
validation.

  When the platform provides a compatible bounds-checked memcpy, this function
adapts that implementation. Otherwise it performs the required validation and
uses the djinterp memcpy primitive.

Parameter(s):
  _destination:      pointer to the destination buffer.
  _destination_size: size of the destination buffer in bytes.
  _source:           pointer to the source buffer.
  _amount:           number of bytes to copy.
Return:
  0 on success; EINVAL for an invalid pointer; ERANGE when _amount exceeds
_destination_size.
*/
int
d_memcpy_s(
    void*       _destination,
    size_t      _destination_size,
    const void* _source,
    size_t      _amount
)
{
    // validate the destination pointer
    if (_destination == NULL)
    {

        return EINVAL;
    }

    // reject a null source and clear the destination on failure
    if (_source == NULL)
    {
        d_memset(_destination, 0, _destination_size);

        return EINVAL;
    }

    // reject an oversized copy and clear the destination on failure
    if (_destination_size < _amount)
    {
        d_memset(_destination, 0, _destination_size);

        return ERANGE;
    }

    // prefer the platform bounds-checked implementation when available
#if ( defined(_MSC_VER) && (_MSC_VER >= 1400) )
    return memcpy_s(_destination,
                    _destination_size,
                    _source,
                    _amount);

#elif ( defined(__STDC_LIB_EXT1__)      && \
        defined(__STDC_WANT_LIB_EXT1__) && \
        (__STDC_WANT_LIB_EXT1__ == 1) )
    return memcpy_s(_destination,
                    _destination_size,
                    _source,
                    _amount);

#else
    (void)d_memcpy(_destination,
                   _source,
                   _amount);

    return 0;
#endif
}


/*
d_memdup
  Allocates _size bytes and copies the source bytes into the new allocation.

Parameter(s):
  _source: pointer to the source data.
  _size:   number of bytes to duplicate.
Return:
  A pointer to the new allocation on success; NULL if _source is NULL or the
allocation fails.
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

    result = malloc(_size);

    // reject allocation failure
    if (result == NULL)
    {

        return NULL;
    }

    // copy through the framework's ordinary memory-copy primitive
    if (d_memcpy(result,
                 _source,
                 _size) == NULL)
    {
        free(result);

        return NULL;
    }

    return result;
}


/*
d_memdup_s
  Allocates _size bytes and duplicates the source using the bounds-checked
djinterp memory-copy primitive.

Parameter(s):
  _source: pointer to the source data.
  _size:   number of bytes to duplicate.
Return:
  A pointer to the new allocation on success; NULL if _source is NULL, _size is
0, allocation fails, or the copy operation fails.
*/
void*
d_memdup_s(
    const void* _source,
    size_t      _size
)
{
    void* destination;

    // validate the source and requested allocation size
    if ( (_source == NULL) ||
         (_size == 0) )
    {

        return NULL;
    }

    destination = malloc(_size);

    // reject allocation failure
    if (destination == NULL)
    {

        return NULL;
    }

    // copy through the framework's bounds-checked memory-copy primitive
    if (d_memcpy_s(destination,
                   _size,
                   _source,
                   _size) != 0)
    {
        free(destination);

        return NULL;
    }

    return destination;
}


/*
d_memset
  Fills _amount bytes beginning at _ptr with the byte value represented by
_value.

Parameter(s):
  _ptr:    pointer to the memory region to fill.
  _value:  byte value to write, converted as by memset.
  _amount: number of bytes to fill.
Return:
  _ptr on success; NULL if _ptr is NULL.
*/
void*
d_memset(
    void*  _ptr,
    int    _value,
    size_t _amount
)
{
    // validate the destination pointer
    if (_ptr == NULL)
    {

        return NULL;
    }

    return memset(_ptr, _value, _amount);
}


/*
d_memset_s
  Fills up to _destination_size bytes while validating the requested count
against the bounds-checked memory limits used by the framework.

Parameter(s):
  _destination:      pointer to the destination buffer.
  _destination_size: size of the destination buffer in bytes.
  _ch:               byte value to write.
  _count:            number of bytes requested.
Return:
  0 on success; EINVAL for an invalid destination or restricted size; ERANGE
when _count exceeds _destination_size.
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
    unsigned char* destination;
    rsize_t        amount;
    rsize_t        index;

    // validate the destination pointer
    if (_destination == NULL)
    {

        return EINVAL;
    }

    // validate the destination size against the framework limit
    if (_destination_size >= RSIZE_MAX)
    {

        return EINVAL;
    }

    // validate the requested count against the framework limit
    if (_count >= RSIZE_MAX)
    {

        return EINVAL;
    }

    value       = (unsigned char)_ch;
    destination = (unsigned char*)_destination;
    amount      = (_count < _destination_size)
        ? _count
        : _destination_size;

    // fill the valid portion even when the requested count is too large
    for (index = 0; index < amount; index++)
    {
        destination[index] = value;
    }

    // report a bounds failure after clearing the entire valid destination

    return (_count > _destination_size)
        ? ERANGE
        : 0;
}