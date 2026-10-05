/*******************************************************************************
* djinterp [c]                                                   buffer_common.c
*
* TBA
*
*
* path:      /src/djinterp/c/container/buffer/buffer_common.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.29
*******************************************************************************/
#include "../../../../../inc/djinterp/c/container/buffer/buffer_common.h"



// =============================================================================
// INTERNAL HELPERS
// =============================================================================

/*
d_internal_buffer_common_element_at
  return a pointer to the element at _index in a contiguous array starting at
  _base with the given _element_size.

Parameter(s):
  _base:         the start of the element storage.
  _index:        the element index to address.
  _element_size: the size, in bytes, of a single element.
Return:
  A pointer to the addressed element. No bounds check is performed; the caller
  has already established the index is in range.
*/
D_STATIC_INLINE void*
d_internal_buffer_common_element_at
(
    const void* _base,
    size_t      _index,
    size_t      _element_size
)
{
    return (char*)_base + (_index * _element_size);
}

/*
d_internal_buffer_common_move_elements
  shift a range of elements within a contiguous buffer. Handles overlapping
  regions via memmove.

Parameter(s):
  _base:         the start of the element storage.
  _element_size: the size, in bytes, of a single element.
  _from:         index of the first element to move.
  _to:           index the range is moved to.
  _n:            the number of elements to move.
Return:
  none.
*/
D_STATIC_INLINE void
d_internal_buffer_common_move_elements
(
    void*  _base,
    size_t _element_size,
    size_t _from,
    size_t _to,
    size_t _n
)
{
    memmove(d_internal_buffer_common_element_at(_base, _to, _element_size),
            d_internal_buffer_common_element_at(_base, _from, _element_size),
            (_n * _element_size));
}

/*
d_internal_buffer_common_chunk_list_push
  link a new chunk onto the tail of _list.

Parameter(s):
  _list:  the chunk list to append to.
  _chunk: the chunk to append; becomes the new tail.
Return:
  none.
*/
D_STATIC_INLINE void
d_internal_buffer_common_chunk_list_push
(
    struct d_buffer_chunk_list* _list,
    struct d_buffer_chunk*      _chunk
)
{
    // add to tail
    if (_list->tail)
    {
        _list->tail->next = _chunk;
    }
    else
    {
        _list->head = _chunk;
    }

    _list->tail = _chunk;
    ++_list->chunk_count;

    return;
}


// =============================================================================
// I.    INITIALIZATION
// =============================================================================

/*
d_buffer_common_init_sized
  Initializes a buffer with a specified capacity.

Parameter(s):
  _destination:      out pointer to receive the allocation.
  _count:            out pointer to receive the element count (set to 0).
  _capacity:         out pointer to receive the capacity.
  _element_size:     size of each element in bytes.
  _initial_capacity: desired initial capacity; defaults to
                     D_BUFFER_DEFAULT_CAPACITY if 0.
Return:
  A boolean value corresponding to either:
  - true, if initialization succeeded, or
  - false, if allocation failed or parameters are invalid.
*/
bool
d_buffer_common_init_sized
(
    void**  _destination,
    size_t* _count,
    size_t* _capacity,
    size_t  _element_size,
    size_t  _initial_capacity
)
{
    size_t cap;

    // validate parameters
    if ( (!_destination) ||
         (!_count)       ||
         (!_capacity)    ||
         (!_element_size) )
    {
        return false;
    }

    // default capacity
    cap = _initial_capacity;

    if (cap == 0)
    {
        cap = D_BUFFER_DEFAULT_CAPACITY;
    }

    // allocate
    *(_destination) = d_buffer_common_alloc(_element_size, cap);

    // ensure that memory allocation was successful
    if (!*(_destination))
    {
        return false;
    }

    *(_count)    = 0;
    *(_capacity) = cap;

    return true;
}

/*
d_buffer_common_init_from_data
  Initializes a buffer by copying data from a source array.

Parameter(s):
  _destination:    out pointer to receive the allocation.
  _count:          out pointer to receive the element count.
  _capacity:       out pointer to receive the capacity.
  _element_size:   size of each element in bytes.
  _source:         pointer to source data.
  _source_count:   number of elements in source.
  _extra_capacity: additional capacity beyond source count.
Return:
  A boolean value corresponding to either:
  - true, if initialization succeeded, or
  - false, if allocation failed or parameters are invalid.
*/
bool
d_buffer_common_init_from_data
(
    void**      _destination,
    size_t*     _count,
    size_t*     _capacity,
    size_t      _element_size,
    const void* _source,
    size_t      _source_count,
    size_t      _extra_capacity
)
{
    size_t total;

    // validate parameters
    if ( (!_destination) ||
         (!_count)       ||
         (!_capacity)    ||
         (!_element_size) )
    {
        return false;
    }

    // validate source
    if ( (!_source) &&
         (_source_count > 0) )
    {
        return false;
    }

    // calculate total capacity
    total = _source_count + _extra_capacity;

    if (total < D_BUFFER_DEFAULT_CAPACITY)
    {
        total = D_BUFFER_DEFAULT_CAPACITY;
    }

    // allocate
    *(_destination) = d_buffer_common_alloc(_element_size, total);

    // ensure that memory allocation was successful
    if (!*(_destination))
    {
        return false;
    }

    // copy source data
    if (_source_count > 0)
    {
        d_memcpy(*(_destination), _source, _source_count * _element_size);
    }

    *(_count)    = _source_count;
    *(_capacity) = total;

    return true;
}

/*
d_buffer_common_init_from_args
  Initializes a buffer from a va_list of element pointers.

Parameter(s):
  _destination:  out pointer to receive the allocation.
  _count:        out pointer to receive the element count.
  _capacity:     out pointer to receive the capacity.
  _element_size: size of each element in bytes.
  _arg_count:    number of variadic arguments.
  _args:         va_list of element pointers.
Return:
  A boolean value corresponding to either:
  - true, if initialization succeeded, or
  - false, if allocation failed or parameters are invalid.
*/
bool
d_buffer_common_init_from_args
(
    void**  _destination,
    size_t* _count,
    size_t* _capacity,
    size_t  _element_size,
    size_t  _arg_count,
    va_list _args
)
{
    size_t cap;
    size_t i;
    void*  arg;

    // validate parameters
    if ( (!_destination) ||
         (!_count)       ||
         (!_capacity)    ||
         (!_element_size) )
    {
        return false;
    }

    // calculate capacity
    cap = (_arg_count > D_BUFFER_DEFAULT_CAPACITY)
              ? _arg_count
              : D_BUFFER_DEFAULT_CAPACITY;

    // allocate
    *(_destination) = d_buffer_common_alloc(_element_size, cap);

    // ensure that memory allocation was successful
    if (!*(_destination))
    {
        return false;
    }

    // copy each argument
    for (i = 0; i < _arg_count; ++i)
    {
        arg = va_arg(_args, void*);

        d_memcpy(d_internal_buffer_common_element_at(*(_destination),
                                                     i,
                                                     _element_size),
                 arg,
                 _element_size);
    }

    *(_count)    = _arg_count;
    *(_capacity) = cap;

    return true;
}

/*
d_buffer_common_init_copy
  Initializes a buffer as an exact copy of source data.

Parameter(s):
  _destination:  out pointer to receive the allocation.
  _count:        out pointer to receive the element count.
  _capacity:     out pointer to receive the capacity.
  _element_size: size of each element in bytes.
  _source:       pointer to source data.
  _source_count: number of elements in source.
Return:
  A boolean value corresponding to either:
  - true, if initialization succeeded, or
  - false, if allocation failed or parameters are invalid.
*/
bool
d_buffer_common_init_copy
(
    void**      _destination,
    size_t*     _count,
    size_t*     _capacity,
    size_t      _element_size,
    const void* _source,
    size_t      _source_count
)
{
    return d_buffer_common_init_from_data(_destination,
                                          _count,
                                          _capacity,
                                          _element_size,
                                          _source,
                                          _source_count,
                                          0);
}

/*
d_buffer_common_init_fill
  Initializes a buffer filled with copies of a single value.

Parameter(s):
  _destination:  out pointer to receive the allocation.
  _count:        out pointer to receive the element count.
  _capacity:     out pointer to receive the capacity.
  _element_size: size of each element in bytes.
  _size:         number of copies to make.
  _value:        pointer to the value to replicate.
Return:
  A boolean value corresponding to either:
  - true, if initialization succeeded, or
  - false, if allocation failed or parameters are invalid.
*/
bool
d_buffer_common_init_fill
(
    void**      _destination,
    size_t*     _count,
    size_t*     _capacity,
    size_t      _element_size,
    size_t      _size,
    const void* _value
)
{
    size_t cap;
    size_t i;

    // validate parameters
    if ( (!_destination) ||
         (!_count)       ||
         (!_capacity)    ||
         (_element_size == 0) ||
         (!_value) )
    {
        return false;
    }

    // calculate capacity
    cap = _size > D_BUFFER_DEFAULT_CAPACITY
              ? _size
              : D_BUFFER_DEFAULT_CAPACITY;

    // allocate
    *(_destination) = d_buffer_common_alloc(_element_size, cap);

    // ensure that memory allocation was successful
    if (!*(_destination))
    {
        return false;
    }

    // fill with copies of _value
    for (i = 0; i < _size; ++i)
    {
        d_memcpy(d_internal_buffer_common_element_at(*(_destination),
                                                     i,
                                                     _element_size),
                 _value,
                 _element_size);
    }

    *(_count)    = _size;
    *(_capacity) = cap;

    return true;
}


// =============================================================================
// II.   CAPACITY MANAGEMENT
// =============================================================================

/*
d_buffer_common_alloc
  Allocates and zero-initializes memory for a buffer.

Parameter(s):
  _element_size: size of each element in bytes.
  _capacity:     number of elements to allocate.
Return:
  A pointer to the allocated memory, or NULL on failure.
*/
void*
d_buffer_common_alloc
(
    size_t _element_size,
    size_t _capacity
)
{
    void* mem;

    // validate parameters
    if ( (_element_size == 0) ||
         (_capacity == 0) )
    {
        return NULL;
    }

    // check for overflow
    if (_capacity > SIZE_MAX / _element_size)
    {
        return NULL;
    }

    // allocate
    mem = malloc(_capacity * _element_size);

    // ensure that memory allocation was successful
    if (mem)
    {
        // zero-initialize
        d_memset(mem, 0, (_capacity * _element_size));
    }

    return mem;
}

/*
d_buffer_common_calc_growth
  Calculates the new capacity when growing a buffer.

Parameter(s):
  _current_capacity:  current capacity.
  _required_capacity: minimum required capacity.
Return:
  The new capacity, which is at least _required_capacity.
*/
size_t
d_buffer_common_calc_growth
(
    size_t _current_capacity,
    size_t _required_capacity
)
{
    size_t grown;

    // handle zero capacity
    if (_current_capacity == 0)
    {
        _current_capacity = D_BUFFER_DEFAULT_CAPACITY;
    }

    // grow by growth factor
    grown = (size_t)((double)_current_capacity * D_BUFFER_GROWTH_FACTOR);

    // ensure we actually grew
    if (grown <= _current_capacity)
    {
        grown = _current_capacity + 1;
    }

    // return the larger of grown or required
    return (grown > _required_capacity)
        ? grown
        : _required_capacity;
}

/*
d_buffer_common_ensure_capacity
  Ensures a buffer has at least the required capacity.

Parameter(s):
  _elements:          pointer to the buffer data pointer.
  _capacity:          pointer to the capacity.
  _element_size:      size of each element in bytes.
  _required_capacity: minimum required capacity.
Return:
  A boolean value corresponding to either:
  - true, if the capacity is sufficient or reallocation succeeded, or
  - false, if reallocation failed or parameters are invalid.
*/
bool
d_buffer_common_ensure_capacity
(
    void**  _elements,
    size_t* _capacity,
    size_t  _element_size,
    size_t  _required_capacity
)
{
    size_t new_cap;
    void*  new_mem;

    // validate parameters
    if ( (!_elements) ||
         (!_capacity) ||
         (!_element_size) )
    {
        return false;
    }

    // already have enough capacity
    if (*(_capacity) >= _required_capacity)
    {
        return true;
    }

    // calculate new capacity
    new_cap = d_buffer_common_calc_growth(*(_capacity), _required_capacity);

    // check for overflow
    if (new_cap > (SIZE_MAX / _element_size))
    {
        return false;
    }

    // reallocate
    new_mem = realloc(*(_elements), new_cap * _element_size);

    // ensure that memory reallocation was successful
    if (!new_mem)
    {
        return false;
    }

    // zero-initialize new space
    d_memset((char*)new_mem + (*(_capacity) * _element_size),
             0,
             ( (new_cap - *(_capacity)) * _element_size ));

    *(_elements) = new_mem;
    *(_capacity) = new_cap;

    return true;
}

/*
d_buffer_common_resize_to_fit
  Resizes a buffer to exactly fit its current element count.

Parameter(s):
  _elements:     pointer to the buffer data pointer.
  _capacity:     pointer to the capacity.
  _element_size: size of each element in bytes.
  _count:        current number of elements.
Return:
  A boolean value corresponding to either:
  - true, if the resize succeeded, or
  - false, if reallocation failed or parameters are invalid.
*/
bool
d_buffer_common_resize_to_fit
(
    void**  _elements,
    size_t* _capacity,
    size_t  _element_size,
    size_t  _count
)
{
    void* new_mem;

    // validate parameters
    if ( (!_elements) ||
         (!_capacity) ||
         (!_element_size) )
    {
        return false;
    }

    // handle empty buffer
    if (_count == 0)
    {
        free(*(_elements));

        *(_elements) = NULL;
        *(_capacity) = 0;

        return true;
    }

    // already the right size
    if (*(_capacity) == _count)
    {
        return true;
    }

    // reallocate
    new_mem = realloc(*(_elements), (_count * _element_size));

    // ensure that memory reallocation was successful
    if (!new_mem)
    {
        return false;
    }

    *(_elements) = new_mem;
    *(_capacity) = _count;

    return true;
}


// =============================================================================
// III.  ELEMENT ACCESS
// =============================================================================

/*
d_buffer_common_get_element
  Returns a pointer to the element at the specified index.

Parameter(s):
  _elements:     pointer to the buffer data.
  _count:        number of occupied elements.
  _element_size: size of each element in bytes.
  _index:        index of the desired element.
Return:
  A pointer to the element, or NULL if out of bounds or parameters are
invalid.
*/
void*
d_buffer_common_get_element
(
    const void* _elements,
    size_t      _count,
    size_t      _element_size,
    d_index     _index
)
{
    // validate parameters
    if ( (!_elements)         ||
         (_element_size == 0) ||
         ((size_t)_index >= _count) )
    {
        return NULL;
    }

    return d_internal_buffer_common_element_at(_elements,
                                               (size_t)_index,
                                               _element_size);
}

/*
d_buffer_common_set_element
  Sets the value of an element at the specified index.

Parameter(s):
  _elements:     pointer to the buffer data.
  _count:        number of occupied elements.
  _element_size: size of each element in bytes.
  _index:        index of the element to set.
  _value:        pointer to the new value.
Return:
  A boolean value corresponding to either:
  - true, if the element was set successfully, or
  - false, if out of bounds or parameters are invalid.
*/
bool
d_buffer_common_set_element
(
    void*       _elements,
    size_t      _count,
    size_t      _element_size,
    d_index     _index,
    const void* _value
)
{
    // validate parameters
    if ( (!_elements)         ||
         (!_value)            ||
         (_element_size == 0) ||
         ((size_t)_index >= _count) )
    {
        return false;
    }

    d_memcpy(d_internal_buffer_common_element_at(_elements,
                                                 (size_t)_index,
                                                 _element_size),
             _value,
             _element_size);

    return true;
}

/*
d_buffer_common_peek_front
  Returns a pointer to the first element.

Parameter(s):
  _elements:     pointer to the buffer data.
  _count:        number of occupied elements.
  _element_size: size of each element in bytes.
Return:
  A pointer to the first element, or NULL if empty or parameters are
invalid.
*/
void*
d_buffer_common_peek_front
(
    const void* _elements,
    size_t      _count,
    size_t      _element_size
)
{
    // validate parameters
    if ( (!_elements)  ||
         (_count == 0) ||
         (!_element_size) )
    {
        return NULL;
    }

    return (void*)_elements;
}

/*
d_buffer_common_peek_back
  Returns a pointer to the last element.

Parameter(s):
  _elements:     pointer to the buffer data.
  _count:        number of occupied elements.
  _element_size: size of each element in bytes.
Return:
  A pointer to the last element, or NULL if empty or parameters are
invalid.
*/
void*
d_buffer_common_peek_back
(
    const void* _elements,
    size_t      _count,
    size_t      _element_size
)
{
    // validate parameters
    if ( (!_elements)  ||
         (_count == 0) ||
         (!_element_size) )
    {
        return NULL;
    }

    return d_internal_buffer_common_element_at(_elements,
                                               (_count - 1),
                                               _element_size);
}


// =============================================================================
// IV.   INSERTION (RESIZE MODE)
// =============================================================================

/*
d_buffer_common_append_element
  Appends a single element to the end of the buffer.

Parameter(s):
  _elements:     pointer to the buffer data.
  _count:        pointer to the element count.
  _capacity:     current capacity.
  _element_size: size of each element in bytes.
  _value:        pointer to the value to append.
Return:
  A boolean value corresponding to either:
  - true, if the element was appended successfully, or
  - false, if capacity is insufficient or parameters are invalid.
*/
bool
d_buffer_common_append_element
(
    void*       _elements,
    size_t*     _count,
    size_t      _capacity,
    size_t      _element_size,
    const void* _value
)
{
    // validate parameters
    if ( (!_elements) ||
         (!_count)    ||
         (!_value)    ||
         (!_element_size) )
    {
        return false;
    }

    // check capacity
    if (*(_count) >= _capacity)
    {
        return false;
    }

    // copy element
    d_memcpy(d_internal_buffer_common_element_at(_elements,
                                                 *(_count),
                                                 _element_size),
             _value,
             _element_size);

    ++(*(_count));

    return true;
}

/*
d_buffer_common_append_data
  Appends multiple elements to the end of the buffer.

Parameter(s):
  _elements:     pointer to the buffer data.
  _count:        pointer to the element count.
  _capacity:     current capacity.
  _element_size: size of each element in bytes.
  _data:         pointer to the data to append.
  _data_count:   number of elements to append.
Return:
  A boolean value corresponding to either:
  - true, if the data was appended successfully, or
  - false, if capacity is insufficient or parameters are invalid.
*/
bool
d_buffer_common_append_data
(
    void*       _elements,
    size_t*     _count,
    size_t      _capacity,
    size_t      _element_size,
    const void* _data,
    size_t      _data_count
)
{
    // validate parameters
    if ( (!_elements) ||
         (!_count)    ||
         (!_data)     ||
         (!_element_size) )
    {
        return false;
    }

    // check capacity
    if ( (*(_count)+_data_count) > _capacity)
    {
        return false;
    }

    // copy data
    d_memcpy(d_internal_buffer_common_element_at(_elements,
                                                 *(_count),
                                                 _element_size),
             _data,
             (_data_count * _element_size));

    *(_count) += _data_count;

    return true;
}

/*
d_buffer_common_prepend_element
  Prepends a single element to the beginning of the buffer.

Parameter(s):
  _elements:     pointer to the buffer data.
  _count:        pointer to the element count.
  _capacity:     current capacity.
  _element_size: size of each element in bytes.
  _value:        pointer to the value to prepend.
Return:
  A boolean value corresponding to either:
  - true, if the element was prepended successfully, or
  - false, if capacity is insufficient or parameters are invalid.
*/
bool
d_buffer_common_prepend_element
(
    void*       _elements,
    size_t*     _count,
    size_t      _capacity,
    size_t      _element_size,
    const void* _value
)
{
    // validate parameters
    if ( (!_elements) ||
         (!_count)    ||
         (!_value)    ||
         (!_element_size) )
    {
        return false;
    }

    // check capacity
    if (*(_count) >= _capacity)
    {
        return false;
    }

    // shift existing elements
    d_internal_buffer_common_move_elements(_elements,
                                           _element_size,
                                           0,
                                           1,
                                           *(_count));

    // copy new element
    d_memcpy(_elements, _value, _element_size);

    ++(*(_count));

    return true;
}

/*
d_buffer_common_prepend_data
  Prepends multiple elements to the beginning of the buffer.

Parameter(s):
  _elements:     pointer to the buffer data.
  _count:        pointer to the element count.
  _capacity:     current capacity.
  _element_size: size of each element in bytes.
  _data:         pointer to the data to prepend.
  _data_count:   number of elements to prepend.
Return:
  A boolean value corresponding to either:
  - true, if the data was prepended successfully, or
  - false, if capacity is insufficient or parameters are invalid.
*/
bool
d_buffer_common_prepend_data
(
    void*       _elements,
    size_t*     _count,
    size_t      _capacity,
    size_t      _element_size,
    const void* _data,
    size_t      _data_count
)
{
    // validate parameters
    if ( (!_elements) ||
         (!_count)    ||
         (!_data)     ||
         (!_element_size) )
    {
        return false;
    }

    // check capacity
    if ( (*(_count)+_data_count) > _capacity)
    {
        return false;
    }

    // shift existing elements
    d_internal_buffer_common_move_elements(_elements,
                                           _element_size,
                                           0,
                                           _data_count,
                                           *(_count));

    // copy new data
    d_memcpy(_elements, _data, (_data_count * _element_size));

    *(_count) += _data_count;

    return true;
}

/*
d_buffer_common_insert_element
  Inserts a single element at the specified index.

Parameter(s):
  _elements:     pointer to the buffer data.
  _count:        pointer to the element count.
  _capacity:     current capacity.
  _element_size: size of each element in bytes.
  _value:        pointer to the value to insert.
  _index:        index at which to insert.
Return:
  A boolean value corresponding to either:
  - true, if the element was inserted successfully, or
  - false, if capacity is insufficient, out of bounds, or parameters are
invalid.
*/
bool
d_buffer_common_insert_element
(
    void*       _elements,
    size_t*     _count,
    size_t      _capacity,
    size_t      _element_size,
    const void* _value,
    d_index     _index
)
{
    size_t idx;

    // validate parameters
    if ( (!_elements) ||
         (!_count)    ||
         (!_value)    ||
         (!_element_size) )
    {
        return false;
    }

    idx = (size_t)_index;

    // validate index and capacity
    if ( (idx > *(_count)) ||
         (*(_count) >= _capacity))
    {
        return false;
    }

    // shift elements if inserting before end
    if (idx < *(_count))
    {
        d_internal_buffer_common_move_elements(_elements,
                                               _element_size,
                                               idx,
                                               idx + 1,
                                               *(_count)-idx);
    }

    // copy new element
    d_memcpy(d_internal_buffer_common_element_at(_elements,
                                                 idx,
                                                 _element_size),
             _value,
             _element_size);

    ++(*(_count));

    return true;
}

/*
d_buffer_common_insert_data
  Inserts multiple elements at the specified index.

Parameter(s):
  _elements:     pointer to the buffer data.
  _count:        pointer to the element count.
  _capacity:     current capacity.
  _element_size: size of each element in bytes.
  _data:         pointer to the data to insert.
  _data_count:   number of elements to insert.
  _index:        index at which to insert.
Return:
  A boolean value corresponding to either:
  - true, if the data was inserted successfully, or
  - false, if capacity is insufficient, out of bounds, or parameters are
invalid.
*/
bool
d_buffer_common_insert_data
(
    void*       _elements,
    size_t*     _count,
    size_t      _capacity,
    size_t      _element_size,
    const void* _data,
    size_t      _data_count,
    d_index     _index
)
{
    size_t idx;

    // validate parameters
    if ( (!_elements) ||
         (!_count)    ||
         (!_data)     ||
         (!_element_size) )
    {
        return false;
    }

    idx = (size_t)_index;

    // validate index and capacity
    if ( (idx > *(_count)) ||
         (*(_count)+_data_count > _capacity))
    {
        return false;
    }

    // shift elements if inserting before end
    if (idx < *(_count))
    {
        d_internal_buffer_common_move_elements(_elements,
                                               _element_size,
                                               idx,
                                               idx + _data_count,
                                               *(_count)-idx);
    }

    // copy new data
    d_memcpy(d_internal_buffer_common_element_at(_elements,
                                                 idx,
                                                 _element_size),
             _data,
             _data_count * _element_size);

    *(_count) += _data_count;

    return true;
}


// =============================================================================
// V.    INSERTION (APPEND MODE)
// =============================================================================

/*
d_buffer_common_chunk_new
  Allocates a new chunk with the specified capacity.

Parameter(s):
  _element_size: size of each element in bytes.
  _capacity:     number of elements the chunk can hold.
Return:
  A pointer to the new chunk, or NULL on failure.
*/
struct d_buffer_chunk*
d_buffer_common_chunk_new
(
    size_t _element_size,
    size_t _capacity
)
{
    struct d_buffer_chunk* chunk;

    // validate parameters
    if ( (!_element_size) ||
         (!_capacity) )
    {
        return NULL;
    }

    // allocate chunk structure
    chunk = malloc(sizeof(struct d_buffer_chunk));

    // ensure that memory allocation was successful
    if (!chunk)
    {
        return NULL;
    }

    // allocate chunk payload
    chunk->elements = d_buffer_common_alloc(_element_size, _capacity);

    // ensure that memory allocation was successful
    if (!chunk->elements)
    {
        free(chunk);

        return NULL;
    }

    chunk->count    = 0;
    chunk->capacity = _capacity;
    chunk->next     = NULL;

    return chunk;
}

/*
d_buffer_common_chunk_free
  Frees a chunk and its payload.

Parameter(s):
  _chunk: pointer to the chunk; may be NULL.
Return:
  none.
*/
void
d_buffer_common_chunk_free
(
    struct d_buffer_chunk* _chunk
)
{
    // validate parameter
    if (!_chunk)
    {
        return;
    }

    free(_chunk->elements);
    free(_chunk);

    return;
}

/*
d_buffer_common_chunk_list_init
  Initializes a chunk list to empty state.

Parameter(s):
  _list: pointer to the chunk list.
Return:
  none.
*/
void
d_buffer_common_chunk_list_init
(
    struct d_buffer_chunk_list* _list
)
{
    // validate parameter
    if (!_list)
    {
        return;
    }

    _list->head        = NULL;
    _list->tail        = NULL;
    _list->chunk_count = 0;
    _list->total_count = 0;

    return;
}

/*
d_buffer_common_chunk_list_free
  Frees all chunks in a list and resets it.

Parameter(s):
  _list: pointer to the chunk list; may be NULL.
Return:
  none.
*/
void
d_buffer_common_chunk_list_free
(
    struct d_buffer_chunk_list* _list
)
{
    struct d_buffer_chunk* cur;
    struct d_buffer_chunk* next;

    // validate parameter
    if (!_list)
    {
        return;
    }

    cur = _list->head;

    // free each chunk
    while (cur)
    {
        next = cur->next;

        d_buffer_common_chunk_free(cur);

        cur = next;
    }

    d_buffer_common_chunk_list_init(_list);

    return;
}

/*
d_buffer_common_append_element_chunked
  Appends a single element using chunked storage.

Parameter(s):
  _list:           pointer to the chunk list.
  _element_size:   size of each element in bytes.
  _value:          pointer to the value to append.
  _chunk_capacity: capacity for new chunks (if needed).
Return:
  A boolean value corresponding to either:
  - true, if the element was appended successfully, or
  - false, if allocation failed or parameters are invalid.
*/
bool
d_buffer_common_append_element_chunked
(
    struct d_buffer_chunk_list* _list,
    size_t                      _element_size,
    const void*                 _value,
    size_t                      _chunk_capacity
)
{
    size_t                 cap;
    struct d_buffer_chunk* chunk;

    // validate parameters
    if ( (!_list)  ||
         (!_value) ||
         (!_element_size) )
    {
        return false;
    }

    // use existing tail if it has space
    if ( (_list->tail) &&
         (_list->tail->count < _list->tail->capacity) )
    {
        d_memcpy(d_internal_buffer_common_element_at(_list->tail->elements,
                                                     _list->tail->count,
                                                     _element_size),
                 _value,
                 _element_size);

        ++_list->tail->count;
        ++_list->total_count;

        return true;
    }

    // allocate a new chunk: at least 1, at least _chunk_capacity
    cap = (_chunk_capacity > 1)
          ? _chunk_capacity
          : 1;

    chunk = d_buffer_common_chunk_new(_element_size, cap);

    // ensure that memory allocation was successful
    if (!chunk)
    {
        return false;
    }

    // copy element
    d_memcpy(chunk->elements, _value, _element_size);

    chunk->count = 1;

    // add chunk to list
    d_internal_buffer_common_chunk_list_push(_list, chunk);

    _list->total_count += 1;

    return true;
}

/*
d_buffer_common_append_data_chunked
  Appends multiple elements using chunked storage.

Parameter(s):
  _list:           pointer to the chunk list.
  _element_size:   size of each element in bytes.
  _data:           pointer to the data to append.
  _data_count:     number of elements to append.
  _chunk_capacity: capacity for new chunks (if needed).
Return:
  A boolean value corresponding to either:
  - true, if the data was appended successfully, or
  - false, if allocation failed or parameters are invalid.
*/
bool
d_buffer_common_append_data_chunked
(
    struct d_buffer_chunk_list* _list,
    size_t                      _element_size,
    const void*                 _data,
    size_t                      _data_count,
    size_t                      _chunk_capacity
)
{
    const char*            src;
    size_t                 remaining;
    size_t                 space;
    size_t                 batch;
    size_t                 cap;
    struct d_buffer_chunk* chunk;

    // validate parameters
    if ( (!_list)             ||
         (!_data)             ||
         (_element_size == 0) ||
         (_data_count == 0) )
    {
        return false;
    }

    src       = (const char*)_data;
    remaining = _data_count;

    // fill existing tail space first
    if ( (_list->tail) &&
         (_list->tail->count < _list->tail->capacity) )
    {
        space = (_list->tail->capacity - _list->tail->count);
        batch = remaining < space
            ? remaining
            : space;

        d_memcpy(d_internal_buffer_common_element_at(_list->tail->elements,
                                                     _list->tail->count,
                                                     _element_size),
                 src,
                 (batch * _element_size) );

        _list->tail->count += batch;
        _list->total_count += batch;

        src       += (batch * _element_size);
        remaining -= batch;
    }

    // allocate new chunks for the rest
    while (remaining > 0)
    {
        // chunk capacity: at least _remaining, at least _chunk_capacity
        cap = _chunk_capacity > remaining
                  ? _chunk_capacity
                  : remaining;

        chunk = d_buffer_common_chunk_new(_element_size, cap);

        // ensure that memory allocation was successful
        if (!chunk)
        {
            return false;
        }

        batch = (remaining < cap)
            ? remaining
            : cap;

        d_memcpy(chunk->elements, src, batch * _element_size);

        chunk->count = batch;

        // add chunk to list
        d_internal_buffer_common_chunk_list_push(_list, chunk);

        _list->total_count += batch;

        src       += (batch * _element_size);
        remaining -= batch;
    }

    return true;
}

/*
d_buffer_common_consolidate
  Flattens all chunks into the primary allocation.

Parameter(s):
  _elements:     pointer to the buffer data pointer.
  _count:        pointer to the element count.
  _capacity:     pointer to the capacity.
  _element_size: size of each element in bytes.
  _list:         pointer to the chunk list.
Return:
  A boolean value corresponding to either:
  - true, if consolidation succeeded, or
  - false, if reallocation failed or parameters are invalid.
*/
bool
d_buffer_common_consolidate
(
    void**                      _elements,
    size_t*                     _count,
    size_t*                     _capacity,
    size_t                      _element_size,
    struct d_buffer_chunk_list* _list
)
{
    size_t                 total;
    void*                  new_mem;
    char*                  dst;
    struct d_buffer_chunk* cur;

    // validate parameters
    if ( (!_elements) ||
         (!_count)    ||
         (!_capacity) ||
         (!_list)     ||
         (!_element_size) )
    {
        return false;
    }

    // no chunks to consolidate
    if (_list->chunk_count == 0)
    {
        return true;
    }

    total = *(_count) + _list->total_count;

    // grow if necessary
    if (total > *(_capacity))
    {
        // check for overflow
        if (total > SIZE_MAX / _element_size)
        {
            return false;
        }

        new_mem = realloc(*(_elements), (total * _element_size));

        // ensure that memory reallocation was successful
        if (!new_mem)
        {
            return false;
        }

        *(_elements) = new_mem;
        *(_capacity) = total;
    }

    // copy chunks into primary allocation
    dst = (char*)(*(_elements)) + (*(_count) * _element_size);
    cur = _list->head;

    while (cur)
    {
        if (cur->count > 0)
        {
            d_memcpy(dst, cur->elements, (cur->count * _element_size));

            dst += (cur->count * _element_size);
        }

        cur = cur->next;
    }

    *(_count) = total;

    d_buffer_common_chunk_list_free(_list);

    return true;
}

/*
d_buffer_common_total_count
  Returns the total element count across primary and chunks.

Parameter(s):
  _primary_count: number of elements in primary buffer.
  _list:          pointer to the chunk list; may be NULL.
Return:
  The total element count.
*/
size_t
d_buffer_common_total_count
(
    size_t                            _primary_count,
    const struct d_buffer_chunk_list* _list
)
{
    size_t total;

    total = _primary_count;

    if (_list)
    {
        total += _list->total_count;
    }

    return total;
}

/*
d_buffer_common_get_element_chunked
  Returns a pointer to an element across primary + chunks.

Parameter(s):
  _primary_elements: pointer to the primary buffer data.
  _primary_count:    number of elements in primary buffer.
  _element_size:     size of each element in bytes.
  _list:             pointer to the chunk list; may be NULL.
  _index:            index of the desired element.
Return:
  A pointer to the element, or NULL if out of bounds.
*/
void*
d_buffer_common_get_element_chunked
(
    const void*                       _primary_elements,
    size_t                            _primary_count,
    size_t                            _element_size,
    const struct d_buffer_chunk_list* _list,
    d_index                           _index
)
{
    size_t                       idx;
    const struct d_buffer_chunk* cur;

    idx = (size_t)_index;

    // check primary buffer
    if (idx < _primary_count)
    {
        return d_internal_buffer_common_element_at(_primary_elements,
                                                   idx,
                                                   _element_size);
    }

    idx -= _primary_count;

    // check chunk list
    if (!_list)
    {
        return NULL;
    }

    cur = _list->head;

    while (cur)
    {
        if (idx < cur->count)
        {
            return d_internal_buffer_common_element_at(cur->elements,
                                                       idx,
                                                       _element_size);
        }

        idx -= cur->count;
        cur  = cur->next;
    }

    return NULL;
}


// =============================================================================
// VI.   REMOVAL
// =============================================================================

/*
d_buffer_common_remove_element
  Removes the element at the specified index.

Parameter(s):
  _elements:     pointer to the buffer data.
  _count:        pointer to the element count.
  _element_size: size of each element in bytes.
  _index:        index of the element to remove.
Return:
  A boolean value corresponding to either:
  - true, if the element was removed successfully, or
  - false, if out of bounds or parameters are invalid.
*/
bool
d_buffer_common_remove_element
(
    void*   _elements,
    size_t* _count,
    size_t  _element_size,
    d_index _index
)
{
    size_t idx;

    // validate parameters
    if ( (!_elements) ||
         (!_count)    ||
         (!_element_size) )
    {
        return false;
    }

    idx = (size_t)_index;

    // validate index
    if (idx >= *(_count))
    {
        return false;
    }

    // shift elements if not removing last
    if (idx < (*(_count) - 1) )
    {
        d_internal_buffer_common_move_elements(_elements,
                                               _element_size,
                                               (idx + 1),
                                               idx,
                                               (*(_count) - idx) - 1);
    }

    --(*(_count));

    // zero the now-unused slot
    d_memset(d_internal_buffer_common_element_at(_elements,
                                                 *(_count),
                                                 _element_size),
             0,
             _element_size);

    return true;
}

/*
d_buffer_common_remove_range
  Removes a range of elements.

Parameter(s):
  _elements:     pointer to the buffer data.
  _count:        pointer to the element count.
  _element_size: size of each element in bytes.
  _start:        first index to remove.
  _end:          one-past-last index to remove.
Return:
  A boolean value corresponding to either:
  - true, if the range was removed successfully, or
  - false, if out of bounds or parameters are invalid.
*/
bool
d_buffer_common_remove_range
(
    void*   _elements,
    size_t* _count,
    size_t  _element_size,
    d_index _start,
    d_index _end
)
{
    size_t s;
    size_t e;
    size_t removed;

    // validate parameters
    if ( (!_elements) ||
         (!_count)    ||
         (!_element_size) )
    {
        return false;
    }

    s = (size_t)_start;
    e = (size_t)_end;

    // validate range
    if ( (s > e) ||
         (e > *(_count)) )
    {
        return false;
    }

    removed = e - s;

    // no-op if removing zero elements
    if (removed == 0)
    {
        return true;
    }

    // shift elements if not removing from end
    if (e < *(_count))
    {
        d_internal_buffer_common_move_elements(_elements,
                                               _element_size,
                                               e,
                                               s,
                                               (*(_count) - e));
    }

    *(_count) -= removed;

    // zero the now-unused slots
    d_memset(d_internal_buffer_common_element_at(_elements,
                                                 *(_count),
                                                 _element_size),
             0,
             (removed * _element_size));

    return true;
}

/*
d_buffer_common_consume_front
  Removes elements from the front of the buffer.

Parameter(s):
  _elements:     pointer to the buffer data.
  _count:        pointer to the element count.
  _element_size: size of each element in bytes.
  _amount:       number of elements to remove.
Return:
  A boolean value corresponding to either:
  - true, if elements were removed successfully, or
  - false, if insufficient elements or parameters are invalid.
*/
bool
d_buffer_common_consume_front
(
    void*   _elements,
    size_t* _count,
    size_t  _element_size,
    size_t  _amount
)
{
    // validate parameters
    if ( (!_elements) ||
         (!_count)    ||
         (!_element_size) )
    {
        return false;
    }

    // validate amount
    if (_amount > *(_count))
    {
        return false;
    }

    // no-op if removing zero elements
    if (_amount == 0)
    {
        return true;
    }

    // shift remaining elements
    d_internal_buffer_common_move_elements(_elements,
                                           _element_size,
                                           _amount,
                                           0,
                                           *(_count)-_amount);

    *(_count) -= _amount;

    // zero the now-unused slots
    d_memset(d_internal_buffer_common_element_at(_elements,
                                                 *(_count),
                                                 _element_size),
             0,
             (_amount * _element_size));

    return true;
}

/*
d_buffer_common_consume_back
  Removes elements from the back of the buffer.

Parameter(s):
  _elements:     pointer to the buffer data.
  _count:        pointer to the element count.
  _element_size: size of each element in bytes.
  _amount:       number of elements to remove.
Return:
  A boolean value corresponding to either:
  - true, if elements were removed successfully, or
  - false, if insufficient elements or parameters are invalid.
*/
bool
d_buffer_common_consume_back
(
    void*   _elements,
    size_t* _count,
    size_t  _element_size,
    size_t  _amount
)
{
    // validate parameters
    if ( (!_elements) ||
         (!_count)    ||
         (!_element_size) )
    {
        return false;
    }

    // validate amount
    if (_amount > *(_count))
    {
        return false;
    }

    *(_count) -= _amount;

    // zero the now-unused slots
    d_memset(d_internal_buffer_common_element_at(_elements,
                                                 *(_count),
                                                 _element_size),
             0,
             (_amount * _element_size));

    return true;
}


// =============================================================================
// VII.  STATE QUERIES
// =============================================================================

/*
d_buffer_common_clear
  Clears the buffer by setting count to zero.

Parameter(s):
  _count: pointer to the element count.
Return:
  none.
*/
void
d_buffer_common_clear
(
    size_t* _count
)
{
    // validate parameter
    if (_count)
    {
        *(_count) = 0;
    }

    return;
}

/*
d_buffer_common_is_empty
  Checks if the buffer is empty.

Parameter(s):
  _count: element count.
Return:
  A boolean value corresponding to either:
  - true, if _count is zero, or
  - false, otherwise.
*/
bool
d_buffer_common_is_empty
(
    size_t _count
)
{
    return (_count == 0);
}

/*
d_buffer_common_is_full
  Checks if the buffer is full.

Parameter(s):
  _count:    element count.
  _capacity: buffer capacity.
Return:
  A boolean value corresponding to either:
  - true, if _count >= _capacity, or
  - false, otherwise.
*/
bool
d_buffer_common_is_full
(
    size_t _count,
    size_t _capacity
)
{
    return (_count >= _capacity);
}

/*
d_buffer_common_utilization
  Calculates the buffer utilization ratio.

Parameter(s):
  _count:    element count.
  _capacity: buffer capacity.
Return:
  The utilization as a value between 0.0 and 1.0.
*/
double
d_buffer_common_utilization
(
    size_t _count,
    size_t _capacity
)
{
    // handle zero capacity
    return (_capacity)
        ? ( (double)_count / (double)_capacity )
        : 0.0;
}


// =============================================================================
// VIII. SEARCH
// =============================================================================

/*
d_buffer_common_contains
  Checks if the buffer contains a value.

Parameter(s):
  _elements:     pointer to the buffer data.
  _count:        number of occupied elements.
  _element_size: size of each element in bytes.
  _value:        pointer to the value to search for.
  _comparator:   comparison function.
Return:
  A boolean value corresponding to either:
  - true, if the value was found, or
  - false, otherwise.
*/
bool
d_buffer_common_contains
(
    const void*   _elements,
    size_t        _count,
    size_t        _element_size,
    const void*   _value,
    fn_comparator _comparator
)
{
    return d_buffer_common_find(_elements,
                                _count,
                                _element_size,
                                _value,
                                _comparator) >= 0;
}

/*
d_buffer_common_find
  Finds the first occurrence of a value.

Parameter(s):
  _elements:     pointer to the buffer data.
  _count:        number of occupied elements.
  _element_size: size of each element in bytes.
  _value:        pointer to the value to search for.
  _comparator:   comparison function.
Return:
  The index of the first occurrence, or -1 if not found.
*/
ssize_t
d_buffer_common_find
(
    const void*   _elements,
    size_t        _count,
    size_t        _element_size,
    const void*   _value,
    fn_comparator _comparator
)
{
    size_t i;

    // validate parameters
    if ( (!_elements)   ||
         (!_value)      ||
         (!_comparator) ||
         (!_element_size) )
    {
        return -1;
    }

    // search for value
    for (i = 0; i < _count; ++i)
    {
        if (_comparator(d_internal_buffer_common_element_at(_elements,
                                                            i,
                                                            _element_size),
                        _value) == 0)
        {
            return (ssize_t)i;
        }
    }

    return -1;
}

/*
d_buffer_common_find_last
  Finds the last occurrence of a value.

Parameter(s):
  _elements:     pointer to the buffer data.
  _count:        number of occupied elements.
  _element_size: size of each element in bytes.
  _value:        pointer to the value to search for.
  _comparator:   comparison function.
Return:
  The index of the last occurrence, or -1 if not found.
*/
ssize_t
d_buffer_common_find_last
(
    const void*   _elements,
    size_t        _count,
    size_t        _element_size,
    const void*   _value,
    fn_comparator _comparator
)
{
    size_t i;

    // validate parameters
    if ( (!_elements)   ||
         (!_value)      ||
         (!_comparator) ||
         (_element_size == 0) ||
         (_count == 0) )
    {
        return -1;
    }

    // search backwards for value
    for (i = _count; i > 0; --i)
    {
        if (_comparator(d_internal_buffer_common_element_at(_elements,
                                                            (i - 1),
                                                            _element_size),
                        _value) == 0)
        {
            return (ssize_t)(i - 1);
        }
    }

    return -1;
}


// =============================================================================
// IX.   FILTER
// =============================================================================

/*
d_buffer_common_filter
  Filters elements that satisfy a filter chain.

Parameter(s):
  _elements:     pointer to the buffer data.
  _count:        number of occupied elements.
  _element_size: size of each element in bytes.
  _chain:        filter chain to evaluate.
  _out_elements: out pointer to receive the result allocation.
  _out_count:    out pointer to receive the match count.
Return:
  A boolean value corresponding to either:
  - true, if filtering succeeded (including zero matches), or
  - false, if allocation failed or output pointers are NULL.
*/
bool
d_buffer_common_filter
(
    const void*                  _elements,
    size_t                       _count,
    size_t                       _element_size,
    const struct d_filter_chain* _chain,
    void**                       _out_elements,
    size_t*                      _out_count
)
{
    size_t      matches;
    size_t      i;
    size_t      w;
    const void* elem;
    void*       out;

    // validate output pointers
    if ( (!_out_elements) ||
         (!_out_count) )
    {
        return false;
    }

    *(_out_elements) = NULL;
    *(_out_count)    = 0;

    // vacuous success on empty input
    if ( (!_elements)  ||
         (_count == 0) ||
         (!_chain)     ||
         (!_element_size) )
    {
        return true;
    }

    // first pass: count matches
    matches = 0;

    for (i = 0; i < _count; ++i)
    {
        if (d_filter_chain_matches_element(
                _chain,
                d_internal_buffer_common_element_at(_elements,
                                                    i,
                                                    _element_size),
                _element_size))
        {
            ++matches;
        }
    }

    // no matches
    if (matches == 0)
    {
        return true;
    }

    // allocate result
    out = malloc(matches * _element_size);

    // ensure that memory allocation was successful
    if (!out)
    {
        return false;
    }

    // second pass: copy matching elements
    w = 0;

    for (i = 0; i < _count; ++i)
    {
        elem = d_internal_buffer_common_element_at(_elements,
                                                   i,
                                                   _element_size);

        if (d_filter_chain_matches_element(_chain,
                                           elem,
                                           _element_size))
        {
            d_memcpy(d_internal_buffer_common_element_at(out,
                                                         w,
                                                         _element_size),
                     elem,
                     _element_size);

            ++w;
        }
    }

    *(_out_elements) = out;
    *(_out_count)    = matches;

    return true;
}

/*
d_buffer_common_filter_in_place
  Filters elements in place, preserving only those that satisfy the
filter chain.

Parameter(s):
  _elements:     pointer to the buffer data.
  _count:        pointer to the element count.
  _element_size: size of each element in bytes.
  _chain:        filter chain to evaluate.
Return:
  A boolean value corresponding to either:
  - true, if filtering succeeded, or
  - false, if parameters are invalid.
*/
bool
d_buffer_common_filter_in_place
(
    void*                        _elements,
    size_t*                      _count,
    size_t                       _element_size,
    const struct d_filter_chain* _chain
)
{
    size_t      write;
    size_t      read;
    const void* elem;

    // validate parameters
    if ( (!_elements) ||
         (!_count)    ||
         (_element_size == 0) ||
         (!_chain) )
    {
        return false;
    }

    write = 0;

    // compact matching elements
    for (read = 0; read < *(_count); ++read)
    {
        elem = d_internal_buffer_common_element_at(_elements,
                                                   read,
                                                   _element_size);

        if (d_filter_chain_matches_element(_chain,
                                           elem,
                                           _element_size))
        {
            if (write != read)
            {
                d_memcpy(d_internal_buffer_common_element_at(_elements,
                                                             write,
                                                             _element_size),
                         elem,
                         _element_size);
            }

            ++write;
        }
    }

    // zero unused slots
    if (write < *(_count))
    {
        d_memset(d_internal_buffer_common_element_at(_elements,
                                                     write,
                                                     _element_size),
                 0,
                 (*(_count)-write) * _element_size);
    }

    *(_count) = write;

    return true;
}

/*
d_buffer_common_filter_indices
  Returns the indices of elements that satisfy the filter chain.

Parameter(s):
  _elements:     pointer to the buffer data.
  _count:        number of occupied elements.
  _element_size: size of each element in bytes.
  _chain:        filter chain to evaluate.
  _out_indices:  out pointer to receive the index array.
  _out_count:    out pointer to receive the match count.
Return:
  A boolean value corresponding to either:
  - true, if filtering succeeded (including zero matches), or
  - false, if allocation failed or output pointers are NULL.
*/
bool
d_buffer_common_filter_indices
(
    const void*                  _elements,
    size_t                       _count,
    size_t                       _element_size,
    const struct d_filter_chain* _chain,
    d_index**                    _out_indices,
    size_t*                      _out_count
)
{
    size_t   matches;
    size_t   i;
    size_t   w;
    d_index* indices;

    // validate output pointers
    if ( (!_out_indices) ||
         (!_out_count) )
    {
        return false;
    }

    *(_out_indices) = NULL;
    *(_out_count) = 0;

    // vacuous success on empty input
    if ( (!_elements)  ||
         (_count == 0) ||
         (!_chain)     ||
         (!_element_size) )
    {
        return true;
    }

    // first pass: count matches
    matches = 0;

    for (i = 0; i < _count; ++i)
    {
        if (d_filter_chain_matches_element(
                _chain,
                d_internal_buffer_common_element_at(_elements,
                                                    i,
                                                    _element_size),
                _element_size))
        {
            ++matches;
        }
    }

    // no matches
    if (matches == 0)
    {
        return true;
    }

    // allocate index array
    indices = malloc(matches * sizeof(d_index));

    // ensure that memory allocation was successful
    if (!indices)
    {
        return false;
    }

    // second pass: record indices
    w = 0;

    for (i = 0; i < _count; ++i)
    {
        if (d_filter_chain_matches_element(
                _chain,
                d_internal_buffer_common_element_at(_elements,
                                                    i,
                                                    _element_size),
                _element_size))
        {
            indices[w++] = (d_index)i;
        }
    }

    *(_out_indices) = indices;
    *(_out_count)   = matches;

    return true;
}

/*
d_buffer_common_count_matching
  Returns the number of elements that satisfy the filter chain.

Parameter(s):
  _elements:     pointer to the buffer data.
  _count:        number of occupied elements.
  _element_size: size of each element in bytes.
  _chain:        filter chain to evaluate.
Return:
  The number of matching elements, or 0 if the buffer is empty or
_chain is NULL.
*/
size_t
d_buffer_common_count_matching
(
    const void*                  _elements,
    size_t                       _count,
    size_t                       _element_size,
    const struct d_filter_chain* _chain
)
{
    size_t matches;
    size_t i;

    // validate parameters
    if ( (!_elements)  ||
         (_count == 0) ||
         (!_chain)     ||
         (!_element_size) )
    {
        return 0;
    }

    matches = 0;

    for (i = 0; i < _count; ++i)
    {
        if (d_filter_chain_matches_element(
                _chain,
                d_internal_buffer_common_element_at(_elements,
                                                    i,
                                                    _element_size),
                _element_size))
        {
            ++matches;
        }
    }

    return matches;
}

/*
d_buffer_common_filter_chunked
  Filters across primary allocation + overflow chunks.

Parameter(s):
  _primary_elements: pointer to the primary buffer data.
  _primary_count:    number of elements in the primary buffer.
  _element_size:     size of each element in bytes.
  _list:             pointer to the chunk list.
  _chain:            filter chain to evaluate.
  _out_elements:     out pointer to receive the result allocation.
  _out_count:        out pointer to receive the match count.
Return:
  A boolean value corresponding to either:
  - true, if filtering succeeded (including zero matches), or
  - false, if allocation failed or output pointers are NULL.
*/
bool
d_buffer_common_filter_chunked
(
    const void*                       _primary_elements,
    size_t                            _primary_count,
    size_t                            _element_size,
    const struct d_buffer_chunk_list* _list,
    const struct d_filter_chain*      _chain,
    void**                            _out_elements,
    size_t*                           _out_count
)
{
    size_t                       total;
    size_t                       matches;
    size_t                       i;
    size_t                       w;
    const struct d_buffer_chunk* cur;
    const void*                  elem;
    void*                        out;

    // validate output pointers
    if ( (!_out_elements) ||
         (!_out_count) )
    {
        return false;
    }

    *(_out_elements) = NULL;
    *(_out_count)    = 0;

    // vacuous success
    if ( (!_chain) ||
         (!_element_size) )
    {
        return true;
    }

    total = d_buffer_common_total_count(_primary_count, _list);

    // empty input
    if (total == 0)
    {
        return true;
    }

    // first pass: count matches across primary + chunks
    matches = 0;

    for (i = 0; i < _primary_count; ++i)
    {
        elem = d_internal_buffer_common_element_at(_primary_elements,
                                                   i,
                                                   _element_size);

        if (d_filter_chain_matches_element(_chain,
                                           elem,
                                           _element_size))
        {
            ++matches;
        }
    }

    if (_list)
    {
        cur = _list->head;

        while (cur)
        {
            for (i = 0; i < cur->count; ++i)
            {
                elem = d_internal_buffer_common_element_at(cur->elements,
                                                           i,
                                                           _element_size);

                if (d_filter_chain_matches_element(_chain,
                                                   elem,
                                                   _element_size))
                {
                    ++matches;
                }
            }

            cur = cur->next;
        }
    }

    // no matches
    if (matches == 0)
    {
        return true;
    }

    // allocate result
    out = malloc(matches * _element_size);

    // ensure that memory allocation was successful
    if (!out)
    {
        return false;
    }

    // second pass: copy matching elements
    w = 0;

    for (i = 0; i < _primary_count; ++i)
    {
        elem = d_internal_buffer_common_element_at(_primary_elements,
                                                   i,
                                                   _element_size);

        if (d_filter_chain_matches_element(_chain,
                                           elem,
                                           _element_size))
        {
            d_memcpy(d_internal_buffer_common_element_at(out,
                                                         w,
                                                         _element_size),
                     elem,
                     _element_size);

            ++w;
        }
    }

    if (_list)
    {
        cur = _list->head;

        while (cur)
        {
            for (i = 0; i < cur->count; ++i)
            {
                elem = d_internal_buffer_common_element_at(cur->elements,
                                                           i,
                                                           _element_size);

                if (d_filter_chain_matches_element(_chain,
                                                   elem,
                                                   _element_size))
                {
                    d_memcpy(d_internal_buffer_common_element_at(out,
                                                                 w,
                                                                 _element_size),
                             elem,
                             _element_size);

                    ++w;
                }
            }

            cur = cur->next;
        }
    }

    *(_out_elements) = out;
    *(_out_count)    = matches;

    return true;
}


// =============================================================================
// X.    COPY
// =============================================================================

/*
d_buffer_common_copy_to
  Copies elements from source to destination.

Parameter(s):
  _source:               pointer to source data.
  _source_count:         number of elements in source.
  _element_size:         size of each element in bytes.
  _destination:          pointer to destination buffer.
  _destination_capacity: capacity of destination in elements.
  _copied_count:         out pointer to receive the copy count; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the copy succeeded, or
  - false, if parameters are invalid.
*/
bool
d_buffer_common_copy_to
(
    const void* _source,
    size_t      _source_count,
    size_t      _element_size,
    void*       _destination,
    size_t      _destination_capacity,
    size_t*     _copied_count
)
{
    size_t to_copy;

    // validate parameters
    if ( (!_source)      ||
         (!_destination) ||
         (!_element_size) )
    {
        return false;
    }

    to_copy = (_source_count < _destination_capacity)
                  ? _source_count
                  : _destination_capacity;

    d_memcpy(_destination, _source, to_copy * _element_size);

    if (_copied_count)
    {
        *(_copied_count) = to_copy;
    }

    return true;
}

/*
d_buffer_common_copy_range_to
  Copies a range of elements from source to destination.

Parameter(s):
  _source:               pointer to source data.
  _source_count:         number of elements in source.
  _element_size:         size of each element in bytes.
  _start:                first index to copy.
  _end:                  one-past-last index to copy.
  _destination:          pointer to destination buffer.
  _destination_capacity: capacity of destination in elements.
  _copied_count:         out pointer to receive the copy count; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the copy succeeded, or
  - false, if out of range or parameters are invalid.
*/
bool
d_buffer_common_copy_range_to
(
    const void* _source,
    size_t      _source_count,
    size_t      _element_size,
    d_index     _start,
    d_index     _end,
    void*       _destination,
    size_t      _destination_capacity,
    size_t*     _copied_count
)
{
    size_t s;
    size_t e;
    size_t range;
    size_t to_copy;

    // validate parameters
    if ( (!_source)      ||
         (!_destination) ||
         (!_element_size) )
    {
        return false;
    }

    s = (size_t)_start;
    e = (size_t)_end;

    // validate range
    if ( (s > e) ||
         (e > _source_count) )
    {
        return false;
    }

    range   = e - s;
    to_copy = (range < _destination_capacity)
                  ? range
                  : _destination_capacity;

    d_memcpy(_destination,
             d_internal_buffer_common_element_at(_source, s, _element_size),
             to_copy * _element_size);

    if (_copied_count)
    {
        *(_copied_count) = to_copy;
    }

    return true;
}


// =============================================================================
// XI.   ORDERING
// =============================================================================

/*
d_buffer_common_reverse
  Reverses the order of elements in the buffer.

Parameter(s):
  _elements:     pointer to the buffer data.
  _count:        number of occupied elements.
  _element_size: size of each element in bytes.
Return:
  A boolean value corresponding to either:
  - true, if the reversal succeeded or was a no-op, or
  - false, if _elements is NULL.
*/
bool
d_buffer_common_reverse
(
    void*  _elements,
    size_t _count,
    size_t _element_size
)
{
    char   stack_buf[256];
    char*  tmp;
    size_t lo;
    size_t hi;
    void*  a;
    void*  b;

    // validate parameters
    if (!_elements)
    {
        return false;
    }

    // trivial case
    if ( (_element_size == 0) ||
         (_count < 2) )
    {
        return true;
    }

    // use stack for small elements, heap for large
    tmp = (_element_size <= sizeof(stack_buf))
              ? stack_buf
              : malloc(_element_size);

    // ensure that memory allocation was successful
    if (!tmp)
    {
        return false;
    }

    // swap from ends toward centre
    lo = 0;
    hi = _count - 1;

    while (lo < hi)
    {
        a = d_internal_buffer_common_element_at(_elements, lo, _element_size);
        b = d_internal_buffer_common_element_at(_elements, hi, _element_size);

        d_memcpy(tmp, a,   _element_size);
        d_memcpy(a,   b,   _element_size);
        d_memcpy(b,   tmp, _element_size);

        ++lo;
        --hi;
    }

    // free heap buffer if used
    if (tmp != stack_buf)
    {
        free(tmp);
    }

    return true;
}

/*
d_buffer_common_sort
  Sorts the buffer elements using qsort.

Parameter(s):
  _elements:     pointer to the buffer data.
  _count:        number of occupied elements.
  _element_size: size of each element in bytes.
  _comparator:   comparison function.
Return:
  none.
*/
void
d_buffer_common_sort
(
    void*         _elements,
    size_t        _count,
    size_t        _element_size,
    fn_comparator _comparator
)
{
    // validate parameters
    if ( (!_elements)   ||
         (_count < 2)   ||
         (_element_size == 0) ||
         (!_comparator) )
    {
        return;
    }

    qsort(_elements, _count, _element_size, _comparator);

    return;
}


// =============================================================================
// XII.  VALIDATION
// =============================================================================

/*
d_buffer_common_validate_params
  Checks that _element_size is non-zero.

Parameter(s):
  _element_size: size of each element in bytes.
Return:
  A boolean value corresponding to either:
  - true, if _element_size > 0, or
  - false, otherwise.
*/
bool
d_buffer_common_validate_params
(
    size_t _element_size
)
{
    return _element_size > 0;
}

/*
d_buffer_common_validate_capacity
  Checks that _capacity is non-zero.

Parameter(s):
  _capacity: capacity to validate.
Return:
  A boolean value corresponding to either:
  - true, if _capacity > 0, or
  - false, otherwise.
*/
bool
d_buffer_common_validate_capacity
(
    size_t _capacity
)
{
    return _capacity > 0;
}

/*
d_buffer_common_validate_index
  Checks that _index is within bounds.

Parameter(s):
  _index: index to validate.
  _count: number of occupied elements.
Return:
  A boolean value corresponding to either:
  - true, if _index < _count, or
  - false, otherwise.
*/
bool
d_buffer_common_validate_index
(
    d_index _index,
    size_t  _count
)
{
    return (size_t)_index < _count;
}


// =============================================================================
// XIII. DESTRUCTION
// =============================================================================

/*
d_buffer_common_free_data
  Frees the buffer data allocation.

Parameter(s):
  _elements: pointer to the buffer data; may be NULL.
Return:
  none.
*/
void
d_buffer_common_free_data
(
    void* _elements
)
{
    free(_elements);

    return;
}

/*
d_buffer_common_free_data_deep
  Frees each pointed-to element and then frees the pointer array.

Parameter(s):
  _count:    number of elements.
  _elements: array of pointers to free.
  _free_fn:  function to call on each non-NULL element.
Return:
  none.
*/
void
d_buffer_common_free_data_deep
(
    size_t  _count,
    void**  _elements,
    fn_free _free_fn
)
{
    size_t i;

    // validate parameters
    if ( (!_elements) ||
         (!_free_fn) )
    {
        return;
    }

    // free each element
    for (i = 0; i < _count; ++i)
    {
        if (_elements[i])
        {
            _free_fn(_elements[i]);
        }
    }

    free(_elements);

    return;
}
