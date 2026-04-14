/******************************************************************************
* djinterp [event]                                                     event.h
*
* 
*
* file:      /inc/c/event/event.h                                   
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2023.06.26
******************************************************************************/

#ifndef DJINTERP_C_EVENT_
#define	DJINTERP_C_EVENT_ 1

#include <stdlib.h>
#include "../djinterp.h"
#include "../dmemory.h"


// D_EVENT_LISTENER_DEFAULT_SIZE
//   predefined constant: the size of a `d_event_handler`
#define D_EVENT_LISTENER_DEFAULT_SIZE	64

// D_EVENT_ID_TYPE
//   type: 
#ifndef D_EVENT_ID_TYPE
	#define D_EVENT_ID_TYPE int
#endif	// D_EVENT_ID_TYPE

// d_event_id
//   type: a unique ID number used to distinguish between different events.
typedef D_EVENT_ID_TYPE d_event_id;

// d_event
//   struct: type corresponding to an event that represents a change in state.
struct d_event
{
	d_event_id id;
	void*      args;
	uint8_t    num_args;
};

// d_event_listener
//   struct: an event listener tha
struct d_event_listener
{
	d_event_id  id;
	fn_callback fn;
	bool        enabled;
};


struct d_event*          d_event_new(d_event_id _event_id);
struct d_event*          d_event_new_args(d_event_id _event_id, void** const, size_t);
struct d_event_listener* d_event_listener_new(d_event_id _event_id, fn_callback, bool);
struct d_event_listener* d_event_listener_new_default(d_event_id _event_id, fn_callback);

int                      d_event_compare(const struct d_event* _event1, const struct d_event* _event2);
int                      d_event_compare_deep(const struct d_event* _event1, const struct d_event* _event2, fn_comparator _comparator);
int                      d_event_listener_compare(const struct d_event_listener* _listener1, const struct d_event_listener* _listener2);
ssize_t                  d_event_listener_find_index_of(const struct d_event_listener** _listeners, size_t _listeners_count, d_event_id _event_id, size_t* _num_occurrences);

void                     d_event_free(struct d_event* _event);
void                     d_event_listener_free(struct d_event_listener* _listener);


#endif	// DJINTERP_C_EVENT_
