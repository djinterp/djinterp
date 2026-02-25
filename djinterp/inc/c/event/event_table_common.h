/******************************************************************************
* djinterp [container]                                    event_table_common.h
*
*   
*
* file:      /inc/event/event_table_common.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.10.04
******************************************************************************/

#ifndef DJINTERP_C_EVENT_TABLE_COMMON_
#define DJINTERP_C_EVENT_TABLE_COMMON_ 1

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "../djinterp.h"
#include "../dmemory.h"
#include "./event.h"


// D_EVENT_HASH_TABLE_DEFAULT_SIZE
//   constant: 
#define D_EVENT_HASH_TABLE_DEFAULT_SIZE     101

// D_EVENT_HASH_TABLE_LOAD_FACTOR
//   constant: 
#define D_EVENT_HASH_TABLE_LOAD_FACTOR      0.75

// D_EVENT_HASH_TABLE_RESIZE_FACTOR
//   constant: 
#define D_EVENT_HASH_TABLE_RESIZE_FACTOR    2


// d_event_hash_node
//   struct: hash table node for chaining collision resolution
struct d_event_hash_node
{                             
    d_event_id                key;
    struct d_event_listener*  value;
    struct d_event_hash_node* next;
};

// d_event_hash_stats
//   struct: hash table statistics
struct d_event_hash_stats
{
    size_t total_buckets;
    size_t used_buckets;
    size_t total_elements;
    size_t max_chain_length;
    double average_chain_length;
    double load_factor;
};


size_t d_event_hash_function(d_event_id _key, size_t _table_size);
size_t d_event_hash_simple(d_event_id _key, size_t _table_size);
size_t d_event_hash_next_prime(size_t _n);

struct d_event_hash_node* d_event_hash_node_new(d_event_id _key, struct d_event_listener* _value);
void d_event_hash_node_free(struct d_event_hash_node* _node);

// statistics printing
char* d_event_hash_table_stats_to_string(const struct d_event_hash_stats* _stats, const char* _label);


#endif	// DJINTERP_C_EVENT_TABLE_COMMON_