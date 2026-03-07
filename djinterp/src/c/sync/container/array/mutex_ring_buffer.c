#include "../../../../../inc/c/sync/container/array/mutex_ring_buffer.h"


/*
d_mutex_ring_buffer_new
  Initializes a new empty `d_mutex_ring_buffer` with an amount of memory
allocated to it that is equal to the product of the two parameters.
Both parameters must be nonzero, otherwise NULL will be returned.

Parameter(s):
  _capacity:     the maximum allowable number of elements that this circular
                 array can contain. Value must be nonzero.
  _element_size: the size, in bytes, of each individual element held by the
                 array. Value must be nonzero.
Return:
  Either:
  - a new empty `d_mutex_ring_buffer`, or 
  - NULL, if either of the following was true:
    - Either/both parameter(s) `_capacity` and/or `_element_size` were equal
      to 0, or
    - Both parameters were nonzero, but memory allocation was unsuccessful.
*/
struct d_mutex_ring_buffer* 
d_mutex_ring_buffer_new
(
    size_t _capacity, 
    size_t _element_size
) 
{
    struct d_mutex_ring_buffer* new_ring_buffer;

    // both parameters must be non-zero
    if ( (_capacity) &&
         (_element_size) )
    {
        new_ring_buffer = malloc(sizeof(struct d_mutex_ring_buffer));

        // ensure that the `d_mutex_ring_buffer` was initialized successfully
        if (new_ring_buffer)
        {
            new_ring_buffer->buffer = malloc(_capacity * _element_size);

            // ensure that the buffer was initialized successfully
            if (new_ring_buffer->buffer)
            {
                new_ring_buffer->capacity = _capacity;
                new_ring_buffer->element_size = _element_size;
                new_ring_buffer->head = 0;
                new_ring_buffer->tail = 0;
                new_ring_buffer->count = 0;

                // ensure that the mutex was initialized successfully
                if (!d_mutex_init(&new_ring_buffer->mutex))
                {
                    free(new_ring_buffer->buffer);
                    free(new_ring_buffer);
                    return NULL;
                }
            }
            else
            {
                free(new_ring_buffer);
                return NULL;
            }
        }
    }

    return NULL;
}

/*
d_mutex_ring_buffer_clear
  Empties the `d_mutex_ring_buffer` by resetting its internal pointers.
This function does not deallocate any memory.

Parameter(s):
  _ring_buffer: the `d_mutex_ring_buffer` to be cleared. If NULL, function
                returns without doing anything.
Return:
  none
*/
void
d_mutex_ring_buffer_clear
(
    struct d_mutex_ring_buffer* _ring_buffer
)
{
    if (_ring_buffer)
    {
        d_mutex_lock(&_ring_buffer->mutex);
        _ring_buffer->head = 0;
        _ring_buffer->tail = 0;
        _ring_buffer->count = 0;
        d_mutex_unlock(&_ring_buffer->mutex);
    }

    return;
}

/*
d_mutex_ring_buffer_count
  Returns the current number of elements in the `d_mutex_ring_buffer`.

Parameter(s):
  _ring_buffer: the `d_mutex_ring_buffer` whose element count is to be 
                returned. If NULL, function returns 0.
Return:
  A `size_t` value corresponding to the number of elements currently in the 
  buffer.
*/
size_t 
d_mutex_ring_buffer_count
(
    const struct d_mutex_ring_buffer* _ring_buffer
) 
{
    size_t count;

    if (_ring_buffer)
    {
        d_mutex_lock((d_mutex_t*)&_ring_buffer->mutex);
        count = _ring_buffer->count;
        d_mutex_unlock((d_mutex_t*)&_ring_buffer->mutex);

        return count;
    }

    return 0;
}

/*
d_mutex_ring_buffer_is_empty
  Checks if the `d_mutex_ring_buffer` contains no elements.

Parameter(s):
  _ring_buffer: the `d_mutex_ring_buffer` to check. If NULL, function returns true.
Return:
  Either:
  - true if the buffer is empty or if _ring_buffer is NULL, or
  - false if the buffer contains at least one element.
*/
bool
d_mutex_ring_buffer_is_empty
(
    const struct d_mutex_ring_buffer* _ring_buffer
)
{
    bool is_empty;

    if (_ring_buffer)
    {

        d_mutex_lock((d_mutex_t*)&_ring_buffer->mutex);
        is_empty = (_ring_buffer->count == 0);
        d_mutex_unlock((d_mutex_t*)&_ring_buffer->mutex);
    }
    else
    {
        is_empty = true;
    }

    return is_empty;
}

/*
d_mutex_ring_buffer_is_full
  Checks if the `d_mutex_ring_buffer` has reached its capacity.

Parameter(s):
  _ring_buffer: the `d_mutex_ring_buffer` to check. If NULL, function returns false.
Return:
  Either:
  - true if the buffer has reached its capacity, or
  - false if the buffer has space available or if _ring_buffer is NULL.
*/
bool
d_mutex_ring_buffer_is_full
(
    const struct d_mutex_ring_buffer* _ring_buffer
)
{
    bool is_full;

    if (_ring_buffer)
    {
        d_mutex_lock((d_mutex_t*)&_ring_buffer->mutex);
        is_full = (_ring_buffer->count == _ring_buffer->capacity);
        d_mutex_unlock((d_mutex_t*)&_ring_buffer->mutex);
    }
    else
    {
        is_full = false;
    }

    return is_full;
}

/*
d_mutex_ring_buffer_peek
  Returns a pointer to the element at the head of the `d_mutex_ring_buffer`
without removing it. The returned pointer is valid until the next call to
d_mutex_ring_buffer_pop or until the buffer is freed.

Parameter(s):
  _ring_buffer: the `d_mutex_ring_buffer` from which to peek at an element.
                If NULL, function returns NULL.
Return:
  Either:
  - a pointer to the element at the head of the buffer, or
  - NULL if any of the following was true:
    - `_ring_buffer` was NULL
    - the buffer was empty
*/
void*
d_mutex_ring_buffer_peek
(
    const struct d_mutex_ring_buffer* _ring_buffer
)
{
    void* item;

    if (_ring_buffer)
    {
        d_mutex_lock((d_mutex_t*)&_ring_buffer->mutex);

        // check if the buffer is empty
        if (_ring_buffer->count > 0)
        {
            // calculate the position in the buffer
            item = (char*)_ring_buffer->buffer + (_ring_buffer->head * _ring_buffer->element_size);

            d_mutex_unlock((d_mutex_t*)&_ring_buffer->mutex);

            return item;
        }
        else
        {
            d_mutex_unlock((d_mutex_t*)&_ring_buffer->mutex);

            return NULL;
        }
    }

    return NULL;
}

/*
d_mutex_ring_buffer_pop
  Removes and returns a pointer to the element at the head of the
`d_mutex_ring_buffer`. The returned pointer is valid until the next call to 
this function or until the buffer is freed.

Parameter(s):
  _ring_buffer: the `d_mutex_ring_buffer` from which to remove an element.
                If NULL, function returns NULL.
Return:
  Either:
  - a pointer to the element removed from the buffer, or
  - NULL if any of the following was true:
    - _ring_buffer was NULL
    - the buffer was empty
*/
void*  
d_mutex_ring_buffer_pop
(
    struct d_mutex_ring_buffer* _ring_buffer
)
{
    void* item;

    if (_ring_buffer)
    {

        d_mutex_lock(&_ring_buffer->mutex);

        // Check if the buffer is empty
        if (_ring_buffer->count > 0)
        {

            // Calculate the position in the buffer
            item = (char*)_ring_buffer->buffer + (_ring_buffer->head * _ring_buffer->element_size);

            // Update the head and count
            _ring_buffer->head = (_ring_buffer->head + 1) % _ring_buffer->capacity;
            _ring_buffer->count--;

            d_mutex_unlock(&_ring_buffer->mutex);

            return item;
        }
        else
        {
            d_mutex_unlock(&_ring_buffer->mutex);

            return NULL;
        }
    }
    
    return NULL;
}

/*
d_mutex_ring_buffer_pop_copy
  Removes the element at the head of the `d_mutex_ring_buffer` and
copies it to the provided destination.

Parameter(s):
  _ring_buffer: the `d_mutex_ring_buffer` from which to remove an element.
                If NULL, function returns false.
  _output:      pointer to the destination where the element will be copied.
                If NULL, function returns false.
Return:
  Either:
  - true if an element was successfully removed and copied, or
  - false if any of the following was true:
    - _ring_buffer was NULL
    - _dest was NULL
    - the buffer was empty.
*/
bool
d_mutex_ring_buffer_pop_copy
(
    struct d_mutex_ring_buffer* _ring_buffer,
    void*                       _output
) 
{
    void* element;

    if ( (_ring_buffer) &&
         (_output) )
    {
        d_mutex_lock(&_ring_buffer->mutex);

        // Check if the buffer is empty
        if (_ring_buffer->count > 0)
        {

            // calculate the position in the buffer
            element = (char*)_ring_buffer->buffer + (_ring_buffer->head * _ring_buffer->element_size);

            // copy the item to the destination
            d_memcpy(_output, element, _ring_buffer->element_size);

            // update the head and count
            _ring_buffer->head = (_ring_buffer->head + 1) % _ring_buffer->capacity;
            _ring_buffer->count--;

            d_mutex_unlock(&_ring_buffer->mutex);
            return true;
        }
        else
        {
            d_mutex_unlock(&_ring_buffer->mutex);

            return false;
        }
    }

    return false;
}

/*
d_mutex_ring_buffer_push
  Adds an element to the tail of the `d_mutex_ring_buffer`.

Parameter(s):
  _ring_buffer: the `d_mutex_ring_buffer` to which the element will be added.
                If NULL, function returns false.
  _element:     pointer to the element to be added. If NULL, function returns 
                false.
Return:
  Either:
  - true if the element was successfully added to the buffer, or
  - false if any of the following was true:
    - _ring_buffer was NULL
    - _item was NULL
    - the buffer was full.
*/
bool  
d_mutex_ring_buffer_push
(
    struct d_mutex_ring_buffer* _ring_buffer,
    const void*                 _element
)
{
    char* dest;

    if ( (_ring_buffer) &&
         (_element) )
    {

        d_mutex_lock(&_ring_buffer->mutex);

        // Check if the buffer is full
        if (_ring_buffer->count < _ring_buffer->capacity)
        {

            // Calculate the position in the buffer
            dest = (char*)_ring_buffer->buffer + (_ring_buffer->tail * _ring_buffer->element_size);

            // Copy the item to the buffer
            d_memcpy(dest, _element, _ring_buffer->element_size);

            // Update the tail and count
            _ring_buffer->tail = (_ring_buffer->tail + 1) % _ring_buffer->capacity;
            _ring_buffer->count++;

            d_mutex_unlock(&_ring_buffer->mutex);

            return true;
        }
        else
        {
            d_mutex_unlock(&_ring_buffer->mutex);

            return false;
        }
    }

    return false;
}

/*
d_mutex_ring_buffer_free
  Deallocates all memory associated with the `d_mutex_ring_buffer`.

Parameter(s):
  _ring_buffer: the `d_mutex_ring_buffer` to be freed. If NULL, function returns
       without doing anything.
Return:
  none
*/
void
d_mutex_ring_buffer_free
(
    struct d_mutex_ring_buffer* _ring_buffer
)
{
    if (_ring_buffer)
    {
        free(_ring_buffer->buffer);
        free(_ring_buffer);
    }

    return;
}