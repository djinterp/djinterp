/******************************************************************************
* djinterp [event]                                             event_handler.h
*
*   
*
* file:      \inc\event\event_handler.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.10.05
******************************************************************************/

#ifndef DJINTERP_C_EVENT_HANDLER_
#define DJINTERP_C_EVENT_HANDLER_ 1

#include <stddef.h>
#include <stdlib.h>
#include "../djinterp.h"
#include "../dmemory.h"
#include "../container/array/circular_array.h"
#include "./event.h"
#include "./event_table.h"


// d_event_handler
//   struct: Single-threaded event handler using hash table for listeners
struct d_event_handler
{
    struct d_circular_array*   events;    // queue of pending events
    struct d_event_hash_table* listeners; // hash table of event listeners
};


// creation and destruction
struct d_event_handler* d_event_handler_new(size_t _events_capacity, 
                                            size_t _listeners_capacity);
// listener management
bool d_event_handler_bind(struct d_event_handler* _handler, 
                          struct d_event_listener* _listener);

struct d_event_listener* d_event_handler_get_listener(const struct d_event_handler* _handler, 
                                                      d_event_id _id);

bool d_event_handler_unbind(struct d_event_handler* _handler, d_event_id _id);

bool d_event_handler_enable_listener(struct d_event_handler* _handler, d_event_id _id);
bool d_event_handler_disable_listener(struct d_event_handler* _handler, d_event_id _id);

// event operations
ssize_t d_event_handler_fire_event(struct d_event_handler* _handler, 
                                    const struct d_event* _event);

bool d_event_handler_queue_event(struct d_event_handler* _handler,
                                  const struct d_event* _event);

size_t d_event_handler_process_events(struct d_event_handler* _handler,
                                       size_t _max_events);

// query functions
size_t d_event_handler_listener_count(const struct d_event_handler* _handler);
size_t d_event_handler_enabled_count(const struct d_event_handler* _handler);
size_t d_event_handler_pending_events(const struct d_event_handler* _handler);

// cleanup
void d_event_handler_free(struct d_event_handler* _handler);


#endif	// DJINTERP_C_EVENT_HANDLER_