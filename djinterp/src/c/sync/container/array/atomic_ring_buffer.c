#include "../../../../../inc/c/sync/container/array/atomic_ring_buffer.h"


/*
d_ring_buffer_new
  Initializes a new empty `d_atomic_ring_buffer` with an amount of memory
allocated to it that is equal to the product of the two parameters.
Both parameters must be nonzero, otherwise NULL will be returned.

Parameter(s):
  _capacity:     the maximum allowable number of elements that this circular
                 array can contain. Value must be nonzero.
  _element_size: the size, in bytes, of each individual element held by the
                 array. Value must be nonzero.
Return:
  Either:
  - a new empty `d_atomic_ring_buffer`, or NULL, if either of the following was 
    true:
    - Either/both parameter(s) `_capacity` and/or `_element_size` were equal
      to 0, or
    - Both parameters were nonzero, but memory allocation was unsuccessful.
*/
struct d_atomic_ring_buffer*
d_atomic_ring_buffer_new
(
    size_t _capacity,
    size_t _element_size
)
{
    struct d_atomic_ring_buffer* new_ring_buffer;

    if ( (_capacity) &&
         (_element_size) )
    {
        new_ring_buffer = malloc(sizeof(struct d_atomic_ring_buffer));
        
        // ensure that inital memory allocation for the `d_atomic_ring_buffer` was
        // successful
        if (new_ring_buffer)
        {
            // attempt to allocate full size of buffer
            new_ring_buffer->buffer = calloc(_capacity, _element_size);

            // ensure that allocation of buffer was successful
            if (new_ring_buffer->buffer)
            {
                new_ring_buffer->capacity = _capacity;
                new_ring_buffer->element_size = _element_size;

                // initialize atomic counters
                d_atomic_init_int(&new_ring_buffer->head, 0);
                d_atomic_init_int(&new_ring_buffer->tail, 0);
                d_atomic_init_int(&new_ring_buffer->count, 0);

                return new_ring_buffer;
            }

            // allocation of struct but NOT buffer was successful; abort
            else    
            {
                free(new_ring_buffer);
                return NULL;
            }
        }
    }

    // one or both paramters was 0
    return NULL;
}

/*
d_atomic_ring_buffer_clear
  Clears the `d_atomic_ring_buffer`, resetting it to empty state.

Parameter(s):
  _ring_buffer: 
Return:
  none
*/
void
d_atomic_ring_buffer_clear
(
    struct d_atomic_ring_buffer* _ring_buffer
)
{
    if (_ring_buffer)
    {
        // Atomically reset all counters
        d_atomic_store_int(&_ring_buffer->head, 0);
        d_atomic_store_int(&_ring_buffer->tail, 0);
        d_atomic_store_int(&_ring_buffer->count, 0);
    }

    return;
}

inline size_t 
d_atomic_ring_buffer_count
(
    const struct d_atomic_ring_buffer* _ring_buffer
) 
{
    return (_ring_buffer) ? d_atomic_load_int(&_ring_buffer->count) :
                            0;
}

/*
d_atomic_ring_buffer_is_empty


Parameter(s):
  _ring_buffer: the previously-initialized `d_atomic_ring_buffer` that will be
                cleared by this operation.
Return:
  none
*/
inline bool
d_atomic_ring_buffer_is_empty
(
    const struct d_atomic_ring_buffer* _ring_buffer
)
{
    return (_ring_buffer) ? (d_atomic_load_int(&_ring_buffer->count) == 0) :
                            true;
}

/*
d_atomic_ring_buffer_is_full


Parameter(s):
  _ring_buffer: the previously-initialized `d_atomic_ring_buffer` that will be
                cleared by this operation.
Return:
  none
*/
inline bool
d_atomic_ring_buffer_is_full
(
    const struct d_atomic_ring_buffer* _ring_buffer
)
{
    return (_ring_buffer) ? (d_atomic_load_int(&_ring_buffer->count) >= _ring_buffer->capacity) :
                            true;
}

/*
d_atomic_ring_buffer_peek
  Removes and returns the element at the `head` position in this 
`d_atomic_ring_buffer`. 
NOTE: The returned pointer is only valid until the next call to 
`d_ring_buffer_push`.

Parameter(s):
  _ring_buffer: the `d_atomic_ring_buffer` being modified by having its 
                foremost element (i.e., at the `head` position) removed, if
                and only if this `d_atomic_ring_buffer` is non-empty.
Return:
  Either:
  - a pointer to the removed element formely occupying the `head` position of
    this `d_atomic_ring_buffer`, or
  - NULL, if this `d_atomic_ring_buffer` is empty and thus contains no elements to
    remove.
*/
void*
d_atomic_ring_buffer_peek
(
    const struct d_atomic_ring_buffer* _ring_buffer
)
{
    long head_pos;
    char* src;

    if (_ring_buffer)
    {
        // Check if buffer is empty
        if (d_atomic_load_int(&_ring_buffer->count) > 0)
        {
            // get current head position
            head_pos = d_atomic_load_int(&_ring_buffer->head) % _ring_buffer->capacity;

            // Ensure proper memory ordering before reading
            d_atomic_thread_fence();

            // Return pointer to the element
            src = (char*)_ring_buffer->buffer + (head_pos * _ring_buffer->element_size);

            return src;
        }
    }

    return NULL;
}

/*
d_atomic_ring_buffer_pop
  Removes and returns the element at the `head` position in this 
`d_atomic_ring_buffer`. 
NOTE: The returned pointer is only valid until the next call to 
`d_ring_buffer_push`.

Parameter(s):
  _ring_buffer: the `d_atomic_ring_buffer` being modified by having its 
                   foremost element (i.e., at the `head` position) removed, if
                   and only if this `d_atomic_ring_buffer` is non-empty
Return:
  Either:
  - a pointer to the removed element formely occupying the `head` position of
    this `d_atomic_ring_buffer`, or
  - NULL, if this `d_atomic_ring_buffer` is empty and thus contains no elements to
    remove.
*/
void*  
d_atomic_ring_buffer_pop
(
    struct d_atomic_ring_buffer* _ring_buffer
)
{
    long current_count;
    long head_pos;
    char* src;

    if (_ring_buffer)
    {
        // First get current count (optimistic check)
        current_count = d_atomic_load_int(&_ring_buffer->count);

        if (current_count > 0)
        {
            // Try to claim an element by decrementing count
            current_count = d_atomic_fetch_sub_int(&_ring_buffer->count, 1);

            // Check if we actually succeeded in claiming an element
            if (current_count > 0)
            {
                // Get a unique head position and increment atomically
                head_pos = d_atomic_fetch_add_int(&_ring_buffer->head, 1) % _ring_buffer->capacity;

                // Ensure proper memory ordering before reading
                d_atomic_thread_fence();

                // Return pointer to the element
                src = (char*)_ring_buffer->buffer + (head_pos * _ring_buffer->element_size);
                return src;
            }
            else
            {
                // buffer became empty before we could claim an element, undo decrement
                d_atomic_fetch_add_int(&_ring_buffer->count, 1);
                return NULL;
            }
        }
    }

    return NULL;
}

bool
d_atomic_ring_buffer_pop_copy
(
    struct d_atomic_ring_buffer* _ring_buffer,
    void*                      _output
) 
{
    void* element;

    if ( (_ring_buffer) &&
         (_output) )
    {
        element = d_atomic_ring_buffer_pop(_ring_buffer);

        if (element)
        {
            d_memcpy(_output, element, _ring_buffer->element_size);
            return true;
        }
        else
        {
            return false;
        }
    }

    return false;
}

/*
d_ring_buffer_push
  Removes and returns the element at the `head` position in this 
`d_atomic_ring_buffer`. 
NOTE: The returned pointer is only valid until the next call to 
`d_ring_buffer_push`.

Parameter(s):
  _ring_buffer: the `d_atomic_ring_buffer` being modified by having its 
                   foremost element (i.e., at the `head` position) removed, if
                   and only if this `d_atomic_ring_buffer` is non-empty
Return:
  Either:
  - a pointer to the removed element formely occupying the `head` position of
    this `d_atomic_ring_buffer`, or
  - NULL, if this `d_atomic_ring_buffer` is empty and thus contains no elements to
    remove.
*/
bool  
d_atomic_ring_buffer_push
(
    struct d_atomic_ring_buffer* _ring_buffer,
    const void*                _element
)
{
    long current_count;
    long tail_pos;
    char* dest;

    if ( (_ring_buffer) &&
         (_element) )
    {
        // get current count (optimistic check)
        current_count = d_atomic_load_int(&_ring_buffer->count);

        if (current_count < _ring_buffer->capacity)
        {
            // try to claim a slot by incrementing count
            current_count = d_atomic_fetch_add_int(&_ring_buffer->count, 1);

            // check if we actually succeeded in claiming a slot
            if (current_count < _ring_buffer->capacity)
            {
                // get a unique tail position and increment atomically
                tail_pos = d_atomic_fetch_add_int(&_ring_buffer->tail, 1) % _ring_buffer->capacity;

                // ensure proper memory ordering before writing
                d_atomic_thread_fence();

                // write the element to the buffer
                dest = (char*)_ring_buffer->buffer + (tail_pos * _ring_buffer->element_size);
                d_memcpy(dest, _element, _ring_buffer->element_size);

                // ensure the write is visible to other threads
                d_atomic_thread_fence();

                return true;
            }
            else
            {
                // buffer became full before we could claim a slot, undo increment
                d_atomic_fetch_sub_int(&_ring_buffer->count, 1);
                return false;
            }
        }
    }
    
    return false;
}

/*
d_atomic_ring_buffer_free
  Frees the memory associated with the given `d_atomic_ring_buffer`.

Parameter(s):
  _ring_buffer: the `d_atomic_ring_buffer` being destroyed.
Return:
  none
*/
void
d_atomic_ring_buffer_free
(
    struct d_atomic_ring_buffer* _ring_buffer
)
{
    if (_ring_buffer)
    {
        free(_ring_buffer->buffer);
        free(_ring_buffer);
    }

    return;
}