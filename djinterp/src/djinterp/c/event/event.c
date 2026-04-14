#include "../../../inc/c/event/event.h"


/*
d_event_new
  Initializes a new `d_event` with the specified ID.

Parameter(s):
  _id: The unique identifier for this event
Return:
  A pointer to a new `d_event` if successful, or NULL if not.
*/
struct d_event*
d_event_new
(
    d_event_id _event_id
)
{
    struct d_event* new_event = malloc(sizeof(struct d_event));

    // ensure memory allocation was successful
    if (!new_event)
    {
        return NULL;
    }

    new_event->id       = _event_id;
    new_event->args     = NULL;
    new_event->num_args = 0;

    return new_event;
}

/*
d_event_new_args
  Initializes a new `d_event` with arguments.
  The first parameter is required. The args parameter can be NULL with size 0
to create an event without arguments.

Parameter(s):
  _id:        the unique identifier for this event
  _args:      array of pointers to arguments (can be NULL if _args_size is 0)
  _args_size: number of arguments (can be 0 if _args is NULL)
Return:
  A pointer to a new `d_event` if successful, or NULL if not.
*/
struct d_event*
d_event_new_args
(
    d_event_id _event_id,
    void**     _args,
    size_t     _args_size
)
{
    struct d_event* new_event = malloc(sizeof(struct d_event));

    // ensure memory allocation was successful
    if (!new_event)
    {
        return NULL;
    }

    new_event->id       = _event_id;
    new_event->args     = _args;
    new_event->num_args = _args_size;

    return new_event;
}

/*
d_event_listener
  Allocates and initializes a new `d_event_listener` with all fields set by the
caller.
  To create a d_event_listener with default values, with only 'id' and 
'callback' specified by the user, see function 'd_event_listener_new_default'.

Parameter(s):
  _id:       the value indicating which d_events will match with this
             `d_event_listener` when a `d_event` is fired (assuming this
             `d_event_listener` is enabled).
  _callback: the function that is called when a `d_event` with a matching 'id'
             is fired.  The function (if it takes parameters) with use 
             arguments either included in this `d_event_listener` (under the 
             'binds' field) and/or in the `d_event` being processed (under the
             'args' field).
  _enabled:  if this `d_event_listener` is initialially enabled, must evaluate
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

        // ensure that memory allocation was successful
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
  Allocates and initializes a new d_event_listener with all fields except 'id'
and 'callback' set to default values (which are explained below).
A d_event_listener initialized by this function will have the 'id' and 'callback'
specified, 'enabled' set to 'true', 'binds' set to NULL.
  To create a d_event_listener without these default values, and where all
initial field values are specified by the user, see function
'd_event_listener_new'.

Parameter(s):
  _id:       the value indicating which `d_event`s will match with this
             `d_event_listener` when a d_event is fired (assuming this
             `d_event_listener` is enabled).
  _callback: the function that is called when a `d_event` with a matching 'id' 
             is fired.  The function (if it takes parameters) with use 
             arguments either included in this d_event_listener (under the 
             'binds' field) and/or in the `d_event` being processed (under 
             the 'args' field).
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

        // ensure that memory allocation was successful
        if (new_listener)
        {
            new_listener->id        = _event_id;
            new_listener->fn        = _callback;
            new_listener->enabled   = true;

            return new_listener;
        }
    }

    return NULL;
}

/*
d_event_listener_compare
  Compares two `d_event_listener`s by their IDs for sorting/searching
operations.
  
Parameter(s):
  _listener1: first `d_event_listener` to compare.
  _listener2: second `d_event_listener` to compare.
Return:
  An integer value corresponding to:
  - negative: if _listener1->id < _listener2->id
  - 0, if _listener1->id == _listener2->id  
  - positive if _listener1->id > _listener2->id
*/
int
d_event_compare
(
    const struct d_event* _event1,
    const struct d_event* _event2
)
{
    // handle NULL cases properly
    if ( (!_event1) &&
         (!_event2) )
    {
        return 0;  // both NULL, they are equal
    }
    else if (!_event1)
    {
        return -1;  // NULL is less than non-NULL
    }
    else  // if (!_listener2)
    {
        return 1;   // non-NULL is greater than NULL
    }
    
    // both are non-NULL, compare IDs
    if (_event1->id != _event2->id)
    {
        return (_event1->id < _event2->id)
            ? -1
            : 1;
    }
    else if (_event1->num_args != _event2->num_args)
    {
        return (_event1->num_args < _event2->num_args)
            ? -1
            : 1;
    }
    else  // if (_event1->args != event2->args)
    {
        return (_event1->args < _event2->args)
            ? -1
            : 1;
    }
}

/*
d_event_listener_compare
  Compares two `d_event_listener`s by their IDs for sorting/searching
operations.
  
Parameter(s):
  _listener1: first `d_event_listener` to compare.
  _listener2: second `d_event_listener` to compare.
Return:
  An integer value corresponding to:
  - negative: if _listener1->id < _listener2->id
  - 0, if _listener1->id == _listener2->id  
  - positive if _listener1->id > _listener2->id
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
        
    // handle NULL cases properly
    if ( (!_event1) &&
         (!_event2) )
    {
        return 0;  // both NULL, they are equal
    }
    else if (!_event1)
    {
        return -1;  // NULL is less than non-NULL
    }
    else  // if (!_listener2)
    {
        return 1;   // non-NULL is greater than NULL
    }
    
    // both are non-NULL, compare IDs
    if (_event1->id != _event2->id)
    {
        return (_event1->id < _event2->id)
            ? -1
            : 1;
    }
    else if (_event1->num_args != _event2->num_args)
    {
        return (_event1->num_args < _event2->num_args)
            ? -1
            : 1;
    }
    else  // if (_event1->args != event2->args)
    {
        // deep compare args using the provided comparator
        if ( (_comparator)   && 
             (_event1->args) &&
             (_event2->args) )
        {
            result = _comparator(_event1->args, _event2->args);

            if (result != 0)
            {
                return result;
            }
        }
        else if ( (_event1->args) &&
                  (_event2->args) )
        {
            // fallback to shallow comparison if no comparator or one side is NULL
            if (!_event1->args)
            {
                return -1;
            }
            else if (!_event2->args)
            {
                return 1;
            }
            else
            {
                return (_event1->args < _event2->args)
                    ? -1
                    : 1;
            }
        }

        return 0;
    }
}


/*
d_event_listener_compare
  Compares two `d_event_listener`s by their IDs for sorting/searching
operations.
  
Parameter(s):
  _listener1: first `d_event_listener` to compare.
  _listener2: second `d_event_listener` to compare.
Return:
  An integer value corresponding to:
  - negative: if _listener1->id < _listener2->id
  - 0, if _listener1->id == _listener2->id  
  - positive if _listener1->id > _listener2->id
*/
int
d_event_listener_compare
(
    const struct d_event_listener* _listener1,
    const struct d_event_listener* _listener2
)
{
    // handle NULL cases properly
    if ( (!_listener1) && 
         (!_listener2) )
    {
        return 0;  // both NULL, they are equal
    }
    else if (!_listener1)
    {
        return -1;  // NULL is less than non-NULL
    }
    else  // if (!_listener2)
    {
        return 1;   // non-NULL is greater than NULL
    }
    
    // both are non-NULL, compare IDs
    if (_listener1->id < _listener2->id)
    {
        return -1;
    }
    else if (_listener1->id > _listener2->id)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/*
d_event_listener_find_index_of
  Finds the index of the first `d_event_listener` with the specified event ID.
  Also returns the count of listeners with that ID.
  
Parameter(s):
  _listeners: array of event listeners to search
  _event_id:  event ID to search for
  _count:     output parameter for count of matching listeners (can be NULL)
Return:
  Index of first matching listener, or -1 if not found
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
    size_t i;
    size_t first_index;
    size_t match_count;
    struct d_event_listener* listener;
    
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
            first_index = i;

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
        // free args if exists
        if (_event->args)
        {
            free(_event->args);
        }

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