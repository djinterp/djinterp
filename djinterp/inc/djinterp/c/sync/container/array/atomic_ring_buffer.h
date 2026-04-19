/*******************************************************************************
* djinterp [threadsafe][container]                         atomic_ring_buffer.h
*
* 
*
* author(s): Samuel 'teer' Neal-Blim
* link:   TBA
* file:   \inc\sync\container\array\atomic_ring_buffer.h       date: 2025.05.11
*******************************************************************************/

#ifndef DJINTERP_SYNC_RING_BUFFER_
#define	DJINTERP_SYNC_RING_BUFFER_ 1

#include <stdlib.h>
#include "../../../../djinterp"
#include "../../../../datomic.h"
#include "../../../../dmemory.h"


// d_atomic_ring_buffer
//   struct: 
struct d_atomic_ring_buffer
{
    void*         buffer;
    size_t        capacity;
    size_t        element_size;
    atomic_size_t count;
    atomic_size_t head;
    atomic_size_t tail;
};

struct d_atomic_ring_buffer* d_atomic_ring_buffer_new(size_t, size_t);

void   d_atomic_ring_buffer_clear(struct d_atomic_ring_buffer*);
size_t d_atomic_ring_buffer_count(const struct d_atomic_ring_buffer*);
bool   d_atomic_ring_buffer_is_empty(const struct d_atomic_ring_buffer*);
bool   d_atomic_ring_buffer_is_full(const struct d_atomic_ring_buffer*);
bool   d_atomic_ring_buffer_push(struct d_atomic_ring_buffer*, const void*);
void*  d_atomic_ring_buffer_pop(struct d_atomic_ring_buffer*);
bool   d_atomic_ring_buffer_pop_copy(struct d_atomic_ring_buffer*, void*);
void*  d_atomic_ring_buffer_peek(const struct d_atomic_ring_buffer*);

void d_atomic_ring_buffer_free(struct d_atomic_ring_buffer*);


#endif	// DJINTERP_THREADSAFE_RING_BUFFER_
