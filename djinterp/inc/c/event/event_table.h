/******************************************************************************
* djinterp [event]                                               event_table.h
*
*   A data structure for handling events 
*   
*
* path:      /inc/c/event/event_table.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.10.04
******************************************************************************/

#ifndef DJINTERP_C_EVENT_TABLE_
#define DJINTERP_C_EVENT_TABLE_ 1

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include "../djinterp.h"
#include "../dmemory.h"
#include "./event.h"
#include "./event_table_common.h"


// d_event_hash_table
//   struct: single-threaded hash table for event listener storage
struct d_event_hash_table
{
    struct d_event_hash_node** buckets;
    size_t                     size;          // current number of buckets
    size_t                     count;         // number of elements
    size_t                     enabled_count; // number of enabled listeners
};

// d_event_hash_iterator
//   struct: iterator for traversing hash table contents
struct d_event_hash_iterator
{
    const struct d_event_hash_table* table;
    size_t                           bucket_index;
    struct d_event_hash_node*        current_node;
};

/******************************************************************************
* CREATION AND DESTRUCTION
******************************************************************************/

struct d_event_hash_table* d_event_hash_table_new(size_t _initial_size);
struct d_event_hash_table* d_event_hash_table_new_default(void);
void                       d_event_hash_table_free(struct d_event_hash_table* _table);
bool                       d_event_hash_table_insert(struct d_event_hash_table* _table, 
                                                      d_event_id                _key, 
                                                      struct d_event_listener*  _listener);
struct d_event_listener*   d_event_hash_table_lookup(const struct d_event_hash_table* _table, 
                                                      d_event_id                      _key);
bool                       d_event_hash_table_remove(struct d_event_hash_table* _table, 
                                                      d_event_id                 _key);
bool                       d_event_hash_table_contains(const struct d_event_hash_table* _table, 
                                                        d_event_id                      _key);
// table management
size_t                     d_event_hash_table_size(const struct d_event_hash_table* _table);
size_t                     d_event_hash_table_count(const struct d_event_hash_table* _table);
size_t                     d_event_hash_table_enabled_count(const struct d_event_hash_table* _table);
double                     d_event_hash_table_load_factor(const struct d_event_hash_table* _table);
bool                       d_event_hash_table_resize(struct d_event_hash_table* _table, 
                                                     size_t                     _new_size);
void                       d_event_hash_table_clear(struct d_event_hash_table* _table);

/******************************************************************************
* ITERATOR SUPPORT
******************************************************************************/

struct d_event_hash_iterator* d_event_hash_table_iterator_begin(const struct d_event_hash_table* _table);
bool                          d_event_hash_table_iterator_has_next(struct d_event_hash_iterator* _iterator);
struct d_event_listener*      d_event_hash_table_iterator_next(struct d_event_hash_iterator* _iterator, d_event_id* _key_out);
void                          d_event_hash_table_iterator_free(struct d_event_hash_iterator* _iterator);

/******************************************************************************
* STATISTICS
******************************************************************************/

struct d_event_hash_stats d_event_hash_table_get_stats(const struct d_event_hash_table* _table);


#endif	// DJINTERP_C_HASH_TABLE_