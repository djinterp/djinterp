/******************************************************************************
* djinterp [event]                                                  event.hpp
*
* C++ event system:
*   Type-safe, RAII-based event handling with compile-time argument
* verification and multi-listener dispatch. Supersedes the C event
* submodule (event.h, event_table.h, event_handler.h) with a unified,
* portable C++11+ interface that leverages the djinterp environment and
* trait infrastructure.
*
* MODULE STRUCTURE:
*   event_traits.hpp    - event tag detection, event_traits,
*                         event_listener_traits, event_context,
*                         declaration macros, C++20 concepts
*   event_listener.hpp  - listener_id, listener_entry,
*                         listener_registry
*   event.hpp (this)    - event_queue, event_handler,
*                         dispatch_result
*
* DESIGN:
*   - Tag-type event IDs: each event is a distinct struct carrying its
*     argument types via a nested `args_type` tuple alias, providing
*     full compile-time type safety.
*   - Type-erased callbacks via std::function, verified at bind time
*     using event_listener_traits and static_assert.
*   - One-to-many dispatch: multiple listeners per event ID, stored
*     in insertion order.
*   - Event propagation control: listeners receive an event_context
*     that can be marked consumed to stop further dispatch.
*   - Separated concerns: listener_registry manages subscriptions,
*     event_queue manages pending events, event_handler composes them.
*   - RAII throughout: no manual new/free pairs.
*   - noexcept policy: callbacks are NOT required to be noexcept by
*     default. Exceptions from callbacks propagate to the caller of
*     fire() or process(). Use is_nothrow_event_listener (C++20) or
*     event_listener_traits::is_nothrow to enforce at bind time.
*
* FEATURE DEPENDENCIES:
*   D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES  - move semantics
*   D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES - parameter packs
*   D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES    - using aliases
*   D_ENV_CPP_FEATURE_LANG_LAMBDAS            - lambda callbacks
*   D_ENV_CPP_FEATURE_STL_OPTIONAL            - dispatch_result
*
* PORTABLE ACROSS:
*   C++11, C++14, C++17, C++20, C++23, C++26
*
* NAMING CONVENTIONS:
*   djinterp::event_handler           handler;
*   djinterp::listener_id             id = handler.bind<my_event>(fn);
*   handler.fire<my_event>(args...);
*
* path:      \inc\event\event.hpp
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.03.11
******************************************************************************/

#ifndef DJINTERP_CPP_EVENT_
#define DJINTERP_CPP_EVENT_ 1

// require the C++ framework header
#ifndef DJINTERP_
    #error "event.hpp requires djinterp.h to be included first"
#endif

#ifndef __cplusplus
    #error "event.hpp can only be used in C++ compilation mode"
#endif

#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "event.hpp requires C++11 or higher"
#endif

#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <tuple>

#include "event_listener.hpp"


NS_DJINTERP


// =========================================================================
// I.   EVENT QUEUE
// =========================================================================

NS_INTERNAL

    // erased_event
    //   struct: type-erased event storage for queuing. Captures the
    // event's type key and a callable that replays the dispatch
    // through a listener_registry. The replay callable is constructed
    // at enqueue time when the event type is still known, preserving
    // type information across the type-erased queue boundary.
    struct erased_event
    {
        std::size_t                             type_key;
        std::function<void(listener_registry&)> replay;
    };

NS_END  // internal

// event_queue
//   class: stores pending events for deferred processing. Events are
// queued in FIFO order and replayed through a listener_registry.
// Arguments are captured by value at enqueue time via a shared_ptr
// to a tuple, ensuring they remain valid for deferred dispatch.
class event_queue
{
public:
    event_queue()
    {
    };

    // enqueue
    //   queues an event with its arguments for later dispatch.
    // arguments are captured by value; the event type information
    // is preserved in the replay closure.
    template<typename _Event,
             typename... _Args>
    void enqueue(_Args&&... _args)
    {
        using traits    = event_traits<_Event>;
        using args_type = typename traits::args_type;

        // capture arguments by value for deferred dispatch
        auto captured_args = std::make_shared<args_type>(
            std::forward<_Args>(_args)...);

        internal::erased_event ev;
        ev.type_key = internal::type_id_value<_Event>();
        ev.replay   =
            [captured_args](listener_registry& _reg)
            {
                apply_dispatch<_Event>(
                    _reg, *captured_args,
                    internal::make_index_sequence<
                        std::tuple_size<args_type>::value>{});
            };

        m_queue.push_back(std::move(ev));
    };

    // process
    //   dispatches up to _max_events queued events through the
    // provided registry. Events are dispatched in FIFO order.
    // returns: number of events dispatched.
    std::size_t process(listener_registry& _registry,
                        std::size_t        _max_events)
    {
        std::size_t count = 0;

        while ( (!m_queue.empty()) &&
                (count < _max_events) )
        {
            auto ev = std::move(m_queue.front());
            m_queue.pop_front();

            ev.replay(_registry);
            ++count;
        }

        return count;
    };

    // process_all
    //   dispatches all queued events through the provided registry.
    // returns: number of events dispatched.
    std::size_t process_all(listener_registry& _registry)
    {
        std::size_t count = m_queue.size();

        while (!m_queue.empty())
        {
            auto ev = std::move(m_queue.front());
            m_queue.pop_front();

            ev.replay(_registry);
        }

        return count;
    };

    // pending
    //   returns the number of queued events.
    std::size_t pending() const
    {
        return m_queue.size();
    };

    // empty
    //   returns true if no events are queued.
    bool empty() const
    {
        return m_queue.empty();
    };

    // clear
    //   discards all queued events without dispatching them.
    void clear()
    {
        m_queue.clear();
    };

private:
    // apply_dispatch
    //   unpacks a tuple and forwards its elements to
    // registry.dispatch<_Event>(args...).
    template<typename    _Event,
             typename    _ArgsTuple,
             std::size_t... _I>
    static void apply_dispatch(listener_registry&          _reg,
                               _ArgsTuple&                 _args,
                               internal::index_sequence<_I...>)
    {
        _reg.dispatch<_Event>(std::get<_I>(_args)...);
    };

    std::deque<internal::erased_event> m_queue;
};


// =========================================================================
// II.  DISPATCH RESULT
// =========================================================================

// dispatch_result
//   struct: enriched return type for fire operations, providing
// both the count of invoked listeners and whether the event was
// consumed by one of them.
struct dispatch_result
{
    std::size_t listeners_invoked;
    bool        consumed;
};


// =========================================================================
// III. EVENT HANDLER (UNIFIED FACADE)
// =========================================================================

// event_handler
//   class: single-threaded event handler composing a listener_registry
// and an event_queue. Provides the primary user-facing API for the
// event system.
class event_handler
{
public:
    event_handler()
    {
    };

    // ---- listener management ----

    // bind
    //   registers a callable as a listener for _Event.
    // the callable must accept (event_context&, Args...) where
    // Args... matches _Event::args_type.
    // returns: a listener_id handle.
    template<typename _Event,
             typename _Callable>
    listener_id bind(_Callable&& _fn)
    {
        return m_listeners.bind<_Event>(
            std::forward<_Callable>(_fn));
    };

    // unbind
    //   removes a listener by id.
    // returns: true if the listener was found and removed.
    bool unbind(listener_id _id)
    {
        return m_listeners.unbind(_id);
    };

    // enable
    //   enables a previously disabled listener.
    // returns: true if the listener was found and enabled.
    bool enable(listener_id _id)
    {
        return m_listeners.enable(_id);
    };

    // disable
    //   disables a listener without removing it.
    // returns: true if the listener was found and disabled.
    bool disable(listener_id _id)
    {
        return m_listeners.disable(_id);
    };

    // is_enabled
    //   queries whether the listener is currently enabled.
    bool is_enabled(listener_id _id) const
    {
        return m_listeners.is_enabled(_id);
    };

    // contains
    //   queries whether a listener with the given id exists.
    bool contains(listener_id _id) const
    {
        return m_listeners.contains(_id);
    };

    // ---- immediate dispatch ----

    // fire
    //   immediately dispatches _Event to all enabled listeners.
    // returns: number of listeners invoked.
    template<typename _Event,
             typename... _Args>
    std::size_t fire(_Args&&... _args)
    {
        return m_listeners.dispatch<_Event>(
            std::forward<_Args>(_args)...);
    };

    // ---- deferred dispatch ----

    // queue
    //   enqueues an event for later processing.
    template<typename _Event,
             typename... _Args>
    void queue(_Args&&... _args)
    {
        m_queue.enqueue<_Event>(
            std::forward<_Args>(_args)...);
    };

    // process
    //   dispatches up to _max_events queued events.
    // returns: number of events dispatched.
    std::size_t process(std::size_t _max_events)
    {
        return m_queue.process(m_listeners, _max_events);
    };

    // process_all
    //   dispatches all queued events.
    // returns: number of events dispatched.
    std::size_t process_all()
    {
        return m_queue.process_all(m_listeners);
    };

    // ---- query functions ----

    // listener_count
    //   returns total number of registered listeners.
    std::size_t listener_count() const
    {
        return m_listeners.listener_count();
    };

    // enabled_count
    //   returns number of enabled listeners.
    std::size_t enabled_count() const
    {
        return m_listeners.enabled_count();
    };

    // pending_events
    //   returns number of queued events.
    std::size_t pending_events() const
    {
        return m_queue.pending();
    };

    // listener_count_for
    //   returns number of listeners for a specific event type.
    template<typename _Event>
    std::size_t listener_count_for() const
    {
        return m_listeners.listener_count_for<_Event>();
    };

    // has_listeners_for
    //   returns true if at least one listener is registered for
    // the given event type.
    template<typename _Event>
    bool has_listeners_for() const
    {
        return m_listeners.has_listeners_for<_Event>();
    };

    // ---- access to components ----

    // listeners
    //   returns a reference to the underlying listener_registry.
    listener_registry& listeners()
    {
        return m_listeners;
    };

    const listener_registry& listeners() const
    {
        return m_listeners;
    };

    // events
    //   returns a reference to the underlying event_queue.
    event_queue& events()
    {
        return m_queue;
    };

    const event_queue& events() const
    {
        return m_queue;
    };

private:
    listener_registry m_listeners;
    event_queue       m_queue;
};


NS_END  // djinterp


#endif  // DJINTERP_CPP_EVENT_
