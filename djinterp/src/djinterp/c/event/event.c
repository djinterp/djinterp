/*******************************************************************************
* djinterp [c]                                                           event.c
*
* TBA
*
*
* path:      /src/djinterp/c/event/event.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.20
*******************************************************************************/
#include "../../../../inc/djinterp/c/event/event.h"


/*
d_event_new
  Initializes a new `d_event` with the specified ID and NULL context.

Parameter(s):
  _event_id: the unique identifier for this event
Return:
  A pointer to a new `d_event` if successful, or NULL if not.
*/
struct d_event*
d_event_new
(
    d_event_id _event_id
)
{
    struct d_event* new_event;

    new_event = malloc(sizeof(struct d_event));

    // ensure that memory allocation was successful
    if (!new_event)
    {
        return NULL;
    }

    new_event->id      = _event_id;
    new_event->context = NULL;

    return new_event;
}

/*
d_event_new_ctx
  Initializes a new `d_event` with the specified ID and caller-owned context.
  The event system does NOT take ownership of the context pointer - the caller
is responsible for its lifetime and cleanup.

Parameter(s):
  _event_id: the unique identifier for this event
  _context:  opaque pointer to caller-owned data (may be NULL)
Return:
  A pointer to a new `d_event` if successful, or NULL if not.
*/
struct d_event*
d_event_new_ctx
(
    d_event_id _event_id,
    void*      _context
)
{
    struct d_event* new_event;

    new_event = malloc(sizeof(struct d_event));

    if (!new_event)
    {
        return NULL;
    }

    new_event->id      = _event_id;
    new_event->context = _context;

    return new_event;
}

/*
d_event_listener_new
  Allocates and initializes a new `d_event_listener` with all fields set by the
caller.
  To create a d_event_listener with default values, with only 'id' and
'callback' specified by the user, see function 'd_event_listener_new_default'.

Parameter(s):
  _event_id: the value indicating which d_events will match with this
             `d_event_listener` when a `d_event` is fired (assuming this
             `d_event_listener` is enabled).
  _callback: the function that is called when a `d_event` with a matching 'id'
             is fired.  The callback receives the event's context pointer as
             its sole argument.
  _enabled:  if this `d_event_listener` is initially enabled, must evaluate
             to `true`; otherwise initially disabled.
Return:
  A pointer to the initialized d_event_listener if successful; otherwise NULL.
*/
struct d_event_listener*
d_event_listener_new
(
    d_event_id   _event_id,
    fn_callback  _callback,
    bool         _enabled
)
{
    struct d_event_listener* new_listener;

    if (_callback)
    {
        new_listener = malloc(sizeof(struct d_event_listener));

        if (new_listener)
        {
            new_listener->id      = _event_id;
            new_listener->fn      = _callback;
            new_listener->enabled = _enabled;

            return new_listener;
        }
    }

    return NULL;
}

/*
d_event_listener_new_default
  Allocates and initializes a new d_event_listener with 'enabled' set to true.
  To create a d_event_listener where all initial field values are specified by
the user, see function 'd_event_listener_new'.

Parameter(s):
  _event_id: the value indicating which `d_event`s will match with this
             `d_event_listener` when a d_event is fired (assuming this
             `d_event_listener` is enabled).
  _callback: the function that is called when a `d_event` with a matching 'id'
             is fired.  The callback receives the event's context pointer as
             its sole argument.
Return:
  A pointer to the initialized `d_event_listener` if successful; otherwise NULL.
*/
struct d_event_listener*
d_event_listener_new_default
(
    d_event_id  _event_id,
    fn_callback _callback
)
{
    struct d_event_listener* new_listener;

    if (_callback)
    {
        new_listener = malloc(sizeof(struct d_event_listener));

        if (new_listener)
        {
            new_listener->id      = _event_id;
            new_listener->fn      = _callback;
            new_listener->enabled = true;

            return new_listener;
        }
    }

    return NULL;
}

/*
d_event_compare
  Compares two `d_event`s by ID, then by context pointer (shallow).

Parameter(s):
  _event1: first `d_event` to compare.
  _event2: second `d_event` to compare.
Return:
  An integer value corresponding to:
  - negative: if _event1 < _event2
  - 0: if _event1 == _event2
  - positive: if _event1 > _event2
*/
int
d_event_compare
(
    const struct d_event* _event1,
    const struct d_event* _event2
)
{
    if ( (!_event1) &&
         (!_event2) )
    {
        return 0;
    }
    else if (!_event1)
    {
        return -1;
    }
    else if (!_event2)
    {
        return 1;
    }

    // compare IDs
    if (_event1->id != _event2->id)
    {
        return (_event1->id < _event2->id)
            ? -1
            : 1;
    }

    // same ID - compare context pointers (shallow)
    if (_event1->context == _event2->context)
    {
        return 0;
    }

    return (_event1->context < _event2->context)
        ? -1
        : 1;
}

/*
d_event_compare_deep
  Compares two `d_event`s by ID, then by context using the provided comparator.
  Falls back to shallow pointer comparison if no comparator is given or if
either context is NULL.

Parameter(s):
  _event1:     first `d_event` to compare.
  _event2:     second `d_event` to compare.
  _comparator: function to deep-compare context pointers (may be NULL).
Return:
  An integer value corresponding to:
  - negative: if _event1 < _event2
  - 0: if _event1 == _event2
  - positive: if _event1 > _event2
*/
int
d_event_compare_deep
(
    const struct d_event* _event1,
    const struct d_event* _event2,
    fn_comparator         _comparator
)
{
    int result;

    if ( (!_event1) &&
         (!_event2) )
    {
        return 0;
    }
    else if (!_event1)
    {
        return -1;
    }
    else if (!_event2)
    {
        return 1;
    }

    // compare IDs
    if (_event1->id != _event2->id)
    {
        return (_event1->id < _event2->id)
            ? -1
            : 1;
    }

    // same ID - compare context
    if ( (_comparator) &&
         (_event1->context) &&
         (_event2->context) )
    {
        result = _comparator(_event1->context, _event2->context);

        return result;
    }

    // fallback: shallow pointer comparison
    if (_event1->context == _event2->context)
    {
        return 0;
    }
    else if (!_event1->context)
    {
        return -1;
    }
    else if (!_event2->context)
    {
        return 1;
    }

    return (_event1->context < _event2->context)
        ? -1
        : 1;
}

/*
d_event_listener_compare
  Compares two `d_event_listener`s by their IDs for sorting/searching.

Parameter(s):
  _listener1: first `d_event_listener` to compare.
  _listener2: second `d_event_listener` to compare.
Return:
  An integer value corresponding to:
  - negative: if _listener1->id < _listener2->id
  - 0: if _listener1->id == _listener2->id
  - positive: if _listener1->id > _listener2->id
*/
int
d_event_listener_compare
(
    const struct d_event_listener* _listener1,
    const struct d_event_listener* _listener2
)
{
    if ( (!_listener1) &&
         (!_listener2) )
    {
        return 0;
    }
    else if (!_listener1)
    {
        return -1;
    }
    else if (!_listener2)
    {
        return 1;
    }

    if (_listener1->id < _listener2->id)
    {
        return -1;
    }
    else if (_listener1->id > _listener2->id)
    {
        return 1;
    }

    return 0;
}

/*
d_event_listener_find_index_of
  Finds the index of the first `d_event_listener` with the specified event ID.
  Also returns the count of listeners with that ID via output parameter.

Parameter(s):
  _listeners:       array of event listeners to search
  _listeners_count: number of elements in the array
  _event_id:        event ID to search for
  _num_occurrences: output parameter for count of matching listeners (can be NULL)
Return:
  Index of first matching listener, or -1 if not found.
*/
ssize_t
d_event_listener_find_index_of
(
    const struct d_event_listener** _listeners,
    size_t                          _listeners_count,
    d_event_id                      _event_id,
    size_t*                         _num_occurrences
)
{
    size_t  i;
    ssize_t first_index;
    size_t  match_count;

    if ( (!_listeners) ||
         (!_listeners_count) )
    {
        if (_num_occurrences)
        {
            *(_num_occurrences) = 0;
        }

        return -1;
    }

    first_index = -1;
    match_count = 0;

    for (i = 0; i < _listeners_count; i++)
    {
        if ( (_listeners[i]) &&
             (_listeners[i]->id == _event_id) )
        {
            if (first_index == -1)
            {
                first_index = (ssize_t)i;
            }

            match_count++;
        }
    }

    if (_num_occurrences)
    {
        *(_num_occurrences) = match_count;
    }

    return first_index;
}

/*
d_event_free
  Frees memory associated with an event.
  NOTE: does NOT free the context pointer - the caller owns the context
and is responsible for its lifetime.

Parameter(s):
  _event: event to free
Return:
  none
*/
void
d_event_free
(
    struct d_event* _event
)
{
    if (_event)
    {
        free(_event);
    }

    return;
}

/*
d_event_listener_free
  Frees memory associated with an event listener.

Parameter(s):
  _listener: event listener to free
Return:
  none
*/
void
d_event_listener_free
(
    struct d_event_listener* _listener
)
{
    if (_listener)
    {
        free(_listener);
    }

    return;
}
