#include "../../../inc/c/event/event_handler.h"


struct d_event_handler*
d_event_handler_new
(
    size_t _events_capacity,
    size_t _listeners_capacity
)
{
    struct d_event_handler* handler;

    handler = malloc(sizeof(struct d_event_handler));

    if (!handler)
    {
        return NULL;
    }

    handler->events = d_circular_array_new(_events_capacity, sizeof(struct d_event));

    if (!handler->events)
    {
        free(handler);

        return NULL;
    }

    handler->listeners = d_event_hash_table_new(_listeners_capacity);

    if (!handler->listeners)
    {
        d_circular_array_free(handler->events);
        free(handler);

        return NULL;
    }

    return handler;
}

bool
d_event_handler_bind
(
    struct d_event_handler*  _handler,
    struct d_event_listener* _listener
)
{
    if ( (!_handler) ||
         (!_listener) )
    {
        return false;
    }

    return d_event_hash_table_insert(_handler->listeners, _listener->id, _listener);
}

struct d_event_listener*
d_event_handler_get_listener
(
    const struct d_event_handler* _handler,
    d_event_id                    _id
)
{
    if (!_handler)
    {
        return NULL;
    }

    return d_event_hash_table_lookup(_handler->listeners, _id);
}

bool
d_event_handler_unbind
(
    struct d_event_handler* _handler,
    d_event_id              _id
)
{
    if (!_handler)
    {
        return false;
    }

    return d_event_hash_table_remove(_handler->listeners, _id);
}

bool
d_event_handler_enable_listener
(
    struct d_event_handler* _handler,
    d_event_id              _id
)
{
    struct d_event_listener* listener;

    if (!_handler)
    {
        return false;
    }

    listener = d_event_hash_table_lookup(_handler->listeners, _id);

    if (!listener)
    {
        return false;
    }

    if (!listener->enabled)
    {
        listener->enabled = true;
        _handler->listeners->enabled_count++;
    }

    return true;
}

bool
d_event_handler_disable_listener
(
    struct d_event_handler* _handler,
    d_event_id              _id
)
{
    struct d_event_listener* listener;

    if (!_handler)
    {
        return false;
    }

    listener = d_event_hash_table_lookup(_handler->listeners, _id);

    if (!listener)
    {
        return false;
    }

    if (listener->enabled)
    {
        listener->enabled = false;
        _handler->listeners->enabled_count--;
    }

    return true;
}

/*
d_event_handler_fire_event
  Fires an event by looking up the matching listener and invoking its callback.
  The event's context pointer is passed directly to the listener's fn_callback.

Parameter(s):
  _handler: the event handler containing bound listeners
  _event:   the event to fire
Return:
  1 if a listener was found and executed, 0 if no listener or disabled,
  -1 on invalid parameters.
*/
ssize_t
d_event_handler_fire_event
(
    struct d_event_handler* _handler,
    const struct d_event*   _event
)
{
    struct d_event_listener* listener;

    if ( (!_handler) ||
         (!_event) )
    {
        return -1;
    }

    listener = d_event_hash_table_lookup(_handler->listeners, _event->id);

    if ( (!listener) ||
         (!listener->enabled) )
    {
        return 0;
    }

    if (listener->fn)
    {
        listener->fn(_event->context);

        return 1;
    }

    return 0;
}

bool
d_event_handler_queue_event
(
    struct d_event_handler* _handler,
    const struct d_event*   _event
)
{
    if ( (!_handler) ||
         (!_event) )
    {
        return false;
    }

    return d_circular_array_push(_handler->events, _event);
}

size_t
d_event_handler_process_events
(
    struct d_event_handler* _handler,
    size_t                  _max_events
)
{
    size_t          processed;
    struct d_event* event;

    if (!_handler)
    {
        return 0;
    }

    processed = 0;

    while ( (processed < _max_events) &&
            (!d_circular_array_is_empty(_handler->events)) )
    {
        event = (struct d_event*)d_circular_array_pop(_handler->events);

        if (event)
        {
            d_event_handler_fire_event(_handler, event);
            processed++;
        }
        else
        {
            break;
        }
    }

    return processed;
}

size_t
d_event_handler_listener_count
(
    const struct d_event_handler* _handler
)
{
    if (!_handler)
    {
        return 0;
    }

    return d_event_hash_table_count(_handler->listeners);
}

size_t
d_event_handler_enabled_count
(
    const struct d_event_handler* _handler
)
{
    if (!_handler)
    {
        return 0;
    }

    return d_event_hash_table_enabled_count(_handler->listeners);
}

size_t
d_event_handler_pending_events
(
    const struct d_event_handler* _handler
)
{
    if ( (!_handler) ||
         (!_handler->events) )
    {
        return 0;
    }

    return _handler->events->count;
}

void
d_event_handler_free
(
    struct d_event_handler* _handler
)
{
    if (!_handler)
    {
        return;
    }

    d_circular_array_free(_handler->events);
    d_event_hash_table_free(_handler->listeners);

    free(_handler);

    return;
}
