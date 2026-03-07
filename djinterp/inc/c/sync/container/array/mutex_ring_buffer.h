/*******************************************************************************
* djinterp [threadsafe][container]                          mutex_ring_buffer.h
*
* 
*
* author(s): Samuel 'teer' Neal-Blim
* link:   TBA
* file:   \inc\sync\container\array\mutex_ring_buffer.h       date: 2025.05.13
*******************************************************************************/

#ifndef DJINTERP_SYNC_RING_BUFFER_
#define	DJINTERP_SYNC_RING_BUFFER_ 1

#include <stdlib.h>
#include "../../../../djinterp"
#include "../../../../dmutex.h"
#include "../../../../dmemory.h"


// d_mutex_ring_buffer
//   struct: 
struct d_mutex_ring_buffer
{
    void*      buffer;
    size_t     capacity;
    size_t     element_size;
    size_t     count;
    size_t     head;
    size_t     tail;
    d_mutex_t  mutex;
};

struct d_mutex_ring_buffer* d_mutex_ring_buffer_new(size_t, size_t);

void   d_mutex_ring_buffer_clear(struct d_mutex_ring_buffer*);
size_t d_mutex_ring_buffer_count(const struct d_mutex_ring_buffer*);
bool   d_mutex_ring_buffer_is_empty(const struct d_mutex_ring_buffer*);
bool   d_mutex_ring_buffer_is_full(const struct d_mutex_ring_buffer*);
bool   d_mutex_ring_buffer_push(struct d_mutex_ring_buffer*, const void*);
void*  d_mutex_ring_buffer_pop(struct d_mutex_ring_buffer*);
bool   d_mutex_ring_buffer_pop_copy(struct d_mutex_ring_buffer*, void*);
void*  d_mutex_ring_buffer_peek(const struct d_mutex_ring_buffer*);

void d_mutex_ring_buffer_free(struct d_mutex_ring_buffer*);


#endif	// DJINTERP_THREADSAFE_RING_BUFFER_
