/******************************************************************************
* djinterp [event]                                        event_dispatcher.hpp
*
* The facade:
*   The unified front end of the event system. event_dispatcher composes an
* event_registry (typed subscription and dispatch) with an event_queue
* (deferred delivery), and adds the bridge to the fused evaluation path. It is
* the one type most users name: bind a handler, fire an event now or queue it
* for later, process the queue, merge subsystems, or compile a static word
* into a fused step.
*
*   This header is the C++ successor to event_handler.hpp; the facade is
* renamed event_handler -> event_dispatcher to free the word "handler" for the
* step primitive, and fire() now returns the enriched dispatch_result rather
* than a bare count. It absorbs the former event_handler_traits.hpp and
* event_handler_concepts.hpp.
*
* FORMAL CORRESPONDENCE ("Definition of an Event"):
*   immediate fire        delta_rho at once   -- fire<_Event>(...)
*   queue       q in Sigma-hat*               -- event_queue
*   enqueue     q . (e,a)  (right concat)      -- queue<_Event>(...)
*   process     fold of delta_rho over q,      -- process / process_all
*               rho read at processing time
*               (deferral coherence: fire == enqueue-then-process)
*   merge       rho (+) rho'                    -- merge(other)
*   fused drive hat-h over a trace             -- drive(step, first, last)
*   coherence   fused == erased (static rho)    -- see drive's COHERENCE LAW
*
* PORTABLE ACROSS:
*   C++11, C++14, C++17, C++20, C++23, C++26
*
* 
* path:      /inc/djinterp/core/event/event_dispatcher.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.11
******************************************************************************/

#ifndef DJINTERP_EVENT_DISPATCHER_
#define DJINTERP_EVENT_DISPATCHER_ 1

// require the C++ framework header
#ifndef DJINTERP_
    #error "event_dispatcher.hpp requires djinterp.h to be included first"
#endif

#ifndef __cplusplus
    #error "event_dispatcher.hpp can only be used in C++ compilation mode"
#endif

#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "event_dispatcher.hpp requires C++11 or higher"
#endif

#include <cstddef>
#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../djinterp.hpp"
#include "./event_registry.hpp"


NS_DJINTERP


// =========================================================================
// I.   INTERNAL QUEUE SUPPORT
// =========================================================================

NS_INTERNAL

    // dispatch_tuple_impl
    //   function: unpacks a payload tuple and forwards its elements to the
    // registry's typed dispatch.
    template<typename _Event,
             typename _Tuple,
             std::size_t... _I>
    dispatch_result dispatch_tuple_impl(event_registry& _reg,
                                        _Tuple&         _payload,
                                        index_sequence<_I...>)
    {
        return _reg.template dispatch<_Event>(std::get<_I>(_payload)...);
    }

    // dispatch_tuple
    //   function: dispatches an occurrence held as a payload tuple through
    // the registry (the registry being read at call time).
    template<typename _Event,
             typename _Tuple>
    dispatch_result dispatch_tuple(event_registry& _reg,
                                   _Tuple&         _payload)
    {
        return dispatch_tuple_impl<_Event>(
            _reg, _payload,
            make_index_sequence<std::tuple_size<_Tuple>::value>{});
    }

    // erased_event
    //   struct: a type-erased queued occurrence. `type_key` records the
    // originating event type (kappa) for inspection; `replay` re-dispatches
    // the captured payload against whatever registry is supplied at
    // processing time.
    struct erased_event
    {
        std::size_t                          type_key;
        std::function<void(event_registry&)> replay;
    };

NS_END  // internal


// =========================================================================
// II.  EVENT QUEUE
// =========================================================================

// event_queue
//   class: a word of deferred occurrences (q in Sigma-hat*). enqueue appends
// an occurrence with its payload captured by value; process folds dispatch
// over a prefix of the word, reading the supplied registry at processing
// time (delivery-time binding). Occurrences enqueued re-entrantly during
// processing land after the current prefix and are deferred to a later
// process() call.
class event_queue
{
private:
    using queue_type = std::vector<internal::erased_event>;

public:
    event_queue()
    {
    };

    // ---- enqueue (right concatenation) ----

    // enqueue
    //   appends an occurrence of _Event, capturing its payload by value. The
    // registry is not consulted here; binding happens at processing time.
    template<typename _Event,
             typename... _Args>
    void enqueue(_Args&&... _args)
    {
        using payload_t = typename event_traits<clean_t<_Event>>::payload_type;

        // capture the payload by value in a shared_ptr so the replay closure
        // remains copyable (a std::function requirement) regardless of the
        // payload's own copy semantics
        auto payload_ptr = std::make_shared<payload_t>(
            std::forward<_Args>(_args)...);

        internal::erased_event ev;
        ev.type_key = internal::type_key<clean_t<_Event>>();
        ev.replay =
            [payload_ptr](event_registry& _reg)
            {
                internal::dispatch_tuple<_Event>(_reg, *payload_ptr);
            };

        m_queue.push_back(std::move(ev));
    };

    // ---- process (the deferred fold) ----

    // process
    //   dispatches up to _max queued occurrences (0 means all currently
    // queued) against _registry, in order, then drops the processed prefix.
    // The registry is read here, at processing time.
    //
    //   The prefix to process is moved into an isolated local batch and
    // removed from the queue *before* any replay runs, so a handler that
    // enqueues re-entrantly during processing cannot reallocate or
    // invalidate the occurrences currently being replayed. Such re-entrant
    // enqueues land in the queue and are deferred to a later call.
    // returns: the number of occurrences processed.
    std::size_t process(event_registry& _registry,
                        std::size_t      _max = 0)
    {
        std::size_t available = m_queue.size();
        std::size_t limit     =
            ( (_max == 0) || (_max > available) )
                ? available
                : _max;

        if (limit == 0)
        {
            return 0;
        }

        // detach the prefix from the live queue into a stable local batch
        std::vector<internal::erased_event> batch;
        batch.reserve(limit);

        for (std::size_t i = 0; i < limit; ++i)
        {
            batch.push_back(std::move(m_queue[i]));
        }

        m_queue.erase(m_queue.begin(),
                      m_queue.begin() + static_cast<std::ptrdiff_t>(limit));

        // fold over the detached batch; re-entrant enqueues mutate m_queue,
        // never this batch
        for (std::size_t i = 0; i < batch.size(); ++i)
        {
            batch[i].replay(_registry);
        }

        return limit;
    };

    // process_all
    //   dispatches all currently queued occurrences against _registry.
    // returns: the number of occurrences processed.
    std::size_t process_all(event_registry& _registry)
    {
        return process(_registry, 0);
    };

    // ---- queue state ----

    // pending
    //   returns the number of occurrences currently queued.
    std::size_t pending() const
    {
        return m_queue.size();
    };

    // empty
    //   returns true if no occurrences are queued.
    bool empty() const
    {
        return m_queue.empty();
    };

    // clear
    //   discards all queued occurrences without dispatching them.
    void clear()
    {
        m_queue.clear();
    };

private:
    queue_type m_queue;
};


// =========================================================================
// III. EVENT DISPATCHER (the facade)
// =========================================================================

// event_dispatcher
//   class: the unified event front end, composing an event_registry and an
// event_queue. Binding, masking, and queries delegate to the registry;
// deferred delivery delegates to the queue; the queue is always processed
// against this dispatcher's own registry.
class event_dispatcher
{
public:
    event_dispatcher()
    {
    };

    // ---- binding ----

    // bind
    //   registers a handler for _Event (compatibility checked at compile
    // time by the registry).
    // returns: a handler_id handle for later management.
    template<typename _Event,
             typename _Callable>
    handler_id bind(_Callable&& _fn)
    {
        return m_registry.template bind<_Event>(
            std::forward<_Callable>(_fn));
    };

    // unbind
    //   removes the handler identified by _id.
    bool unbind(handler_id _id)
    {
        return m_registry.unbind(_id);
    };

    // enable
    //   enables the handler identified by _id.
    bool enable(handler_id _id)
    {
        return m_registry.enable(_id);
    };

    // disable
    //   disables the handler identified by _id.
    bool disable(handler_id _id)
    {
        return m_registry.disable(_id);
    };

    // is_enabled
    //   queries whether the handler identified by _id is enabled.
    bool is_enabled(handler_id _id) const
    {
        return m_registry.is_enabled(_id);
    };

    // contains
    //   queries whether the handler identified by _id exists.
    bool contains(handler_id _id) const
    {
        return m_registry.contains(_id);
    };

    // ---- immediate dispatch ----

    // fire
    //   dispatches an occurrence of _Event immediately against the registry.
    // returns: the enriched (count, verdict) dispatch_result.
    template<typename _Event,
             typename... _Args>
    dispatch_result fire(_Args&&... _args)
    {
        return m_registry.template dispatch<_Event>(
            std::forward<_Args>(_args)...);
    };

    // ---- deferred dispatch ----

    // queue
    //   enqueues an occurrence of _Event for later processing (its payload
    // captured by value).
    template<typename _Event,
             typename... _Args>
    void queue(_Args&&... _args)
    {
        m_queue.template enqueue<_Event>(
            std::forward<_Args>(_args)...);
    };

    // process
    //   processes up to _max queued occurrences (0 means all) against this
    // dispatcher's registry.
    // returns: the number of occurrences processed.
    std::size_t process(std::size_t _max = 0)
    {
        return m_queue.process(m_registry, _max);
    };

    // process_all
    //   processes all queued occurrences against this dispatcher's registry.
    // returns: the number of occurrences processed.
    std::size_t process_all()
    {
        return m_queue.process_all(m_registry);
    };

    // ---- run and stage ----

    // run
    //   runs a homogeneous trace of _Event occurrences immediately (the
    // outer fold). See event_registry::run.
    template<typename _Event,
             typename _InputIt>
    run_result run(_InputIt _first,
                   _InputIt _last)
    {
        return m_registry.template run<_Event>(_first, _last);
    };

    // compile
    //   stages the static effective word for _Event into a single closed
    // fused_step (the staging operation). See event_registry::compile.
    template<typename _Event>
    fused_step<_Event> compile() const
    {
        return m_registry.template compile<_Event>();
    };

    // ---- merge ----

    // merge
    //   merges _other's registry into this one by pointwise concatenation.
    // returns: the number of handlers merged in.
    std::size_t merge(const event_dispatcher& _other)
    {
        return m_registry.merge(_other.m_registry);
    };

    // ---- typed queries ----

    // handler_count_for
    //   returns the number of handlers for a specific event type.
    template<typename _Event>
    std::size_t handler_count_for() const
    {
        return m_registry.template handler_count_for<_Event>();
    };

    // has_handlers_for
    //   returns true if any handler is registered for the given event type.
    template<typename _Event>
    bool has_handlers_for() const
    {
        return m_registry.template has_handlers_for<_Event>();
    };

    // ---- aggregate queries ----

    // handler_count
    //   returns the total number of registered handlers.
    std::size_t handler_count() const
    {
        return m_registry.handler_count();
    };

    // enabled_count
    //   returns the number of enabled handlers.
    std::size_t enabled_count() const
    {
        return m_registry.enabled_count();
    };

    // pending_events
    //   returns the number of occurrences currently queued.
    std::size_t pending_events() const
    {
        return m_queue.pending();
    };

    // ---- component access ----

    // registry
    //   returns a reference to the underlying event_registry.
    event_registry& registry()
    {
        return m_registry;
    };

    const event_registry& registry() const
    {
        return m_registry;
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

    // table
    //   returns a reference to the underlying event_table.
    event_table& table()
    {
        return m_registry.table();
    };

    const event_table& table() const
    {
        return m_registry.table();
    };

private:
    event_registry m_registry;
    event_queue    m_queue;
};


// =========================================================================
// IV.  FUSED DRIVE (the fused path's outer fold)
// =========================================================================

// drive_result
//   struct: aggregate result of driving a trace through a fused step. The
// fused path is deliberately not instrumented per handler (that is the
// erased dispatch's role); only occurrence and consume counts are reported.
struct drive_result
{
    std::size_t occurrences;
    std::size_t consumed_count;
};

// drive
//   runs a homogeneous trace through a precompiled fused_step as a single
// loop -- the fused path's outer fold. Each element of [_first, _last) must
// be (convertible to) the event's payload tuple; it is copied per occurrence.
//
// COHERENCE LAW: for a registry rho held fixed (no bind/unbind/enable/disable
// between the snapshot and the runs), the fused and erased paths agree --
//
//     auto step = dispatcher.compile<E>();
//     drive(step, first, last).occurrences     == run<E>(first, last).occurrences
//     drive(step, first, last).consumed_count  == run<E>(first, last).consumed_count
//
// with identical per-occurrence handler side effects in identical order. This
// is the operational form of the fused/erased coherence proposition and the
// contract any fused build must preserve.
template<typename _Event,
         typename _InputIt>
drive_result drive(const fused_step<_Event>& _step,
                   _InputIt                  _first,
                   _InputIt                  _last)
{
    using payload_t = typename fused_step<_Event>::payload_type;

    drive_result agg;
    agg.occurrences    = 0;
    agg.consumed_count = 0;

    for (; _first != _last; ++_first)
    {
        payload_t payload(*_first);
        verdict   outcome = _step.run_one(payload);

        ++agg.occurrences;

        if (consumed(outcome))
        {
            ++agg.consumed_count;
        }
    }

    return agg;
}


// =========================================================================
// V.   STRUCTURAL DETECTION HELPERS
// =========================================================================

NS_INTERNAL

    // ---- management op detection ----

    // has_dispatcher_unbind
    //   trait: detects unbind(handler_id) returning bool.
    template<typename _Dispatcher,
             typename = void>
    struct has_dispatcher_unbind
    {
        static constexpr bool value = false;
    };

    template<typename _Dispatcher>
    struct has_dispatcher_unbind<_Dispatcher,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<_Dispatcher&>().unbind(
                    std::declval<handler_id>())),
                bool
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_dispatcher_enable
    //   trait: detects enable(handler_id) returning bool.
    template<typename _Dispatcher,
             typename = void>
    struct has_dispatcher_enable
    {
        static constexpr bool value = false;
    };

    template<typename _Dispatcher>
    struct has_dispatcher_enable<_Dispatcher,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<_Dispatcher&>().enable(
                    std::declval<handler_id>())),
                bool
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_dispatcher_disable
    //   trait: detects disable(handler_id) returning bool.
    template<typename _Dispatcher,
             typename = void>
    struct has_dispatcher_disable
    {
        static constexpr bool value = false;
    };

    template<typename _Dispatcher>
    struct has_dispatcher_disable<_Dispatcher,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<_Dispatcher&>().disable(
                    std::declval<handler_id>())),
                bool
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_dispatcher_is_enabled
    //   trait: detects is_enabled(handler_id) const returning bool.
    template<typename _Dispatcher,
             typename = void>
    struct has_dispatcher_is_enabled
    {
        static constexpr bool value = false;
    };

    template<typename _Dispatcher>
    struct has_dispatcher_is_enabled<_Dispatcher,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<const _Dispatcher&>().is_enabled(
                    std::declval<handler_id>())),
                bool
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_dispatcher_contains
    //   trait: detects contains(handler_id) const returning bool.
    template<typename _Dispatcher,
             typename = void>
    struct has_dispatcher_contains
    {
        static constexpr bool value = false;
    };

    template<typename _Dispatcher>
    struct has_dispatcher_contains<_Dispatcher,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<const _Dispatcher&>().contains(
                    std::declval<handler_id>())),
                bool
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // ---- count op detection ----

    // has_dispatcher_handler_count
    //   trait: detects handler_count() const returning size_t.
    template<typename _Dispatcher,
             typename = void>
    struct has_dispatcher_handler_count
    {
        static constexpr bool value = false;
    };

    template<typename _Dispatcher>
    struct has_dispatcher_handler_count<_Dispatcher,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<const _Dispatcher&>().handler_count()),
                std::size_t
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_dispatcher_enabled_count
    //   trait: detects enabled_count() const returning size_t.
    template<typename _Dispatcher,
             typename = void>
    struct has_dispatcher_enabled_count
    {
        static constexpr bool value = false;
    };

    template<typename _Dispatcher>
    struct has_dispatcher_enabled_count<_Dispatcher,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<const _Dispatcher&>().enabled_count()),
                std::size_t
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_dispatcher_pending_events
    //   trait: detects pending_events() const returning size_t.
    template<typename _Dispatcher,
             typename = void>
    struct has_dispatcher_pending_events
    {
        static constexpr bool value = false;
    };

    template<typename _Dispatcher>
    struct has_dispatcher_pending_events<_Dispatcher,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<const _Dispatcher&>().pending_events()),
                std::size_t
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // ---- queue op detection ----

    // has_dispatcher_process
    //   trait: detects process(size_t) returning size_t.
    template<typename _Dispatcher,
             typename = void>
    struct has_dispatcher_process
    {
        static constexpr bool value = false;
    };

    template<typename _Dispatcher>
    struct has_dispatcher_process<_Dispatcher,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<_Dispatcher&>().process(
                    std::declval<std::size_t>())),
                std::size_t
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_dispatcher_process_all
    //   trait: detects process_all() returning size_t.
    template<typename _Dispatcher,
             typename = void>
    struct has_dispatcher_process_all
    {
        static constexpr bool value = false;
    };

    template<typename _Dispatcher>
    struct has_dispatcher_process_all<_Dispatcher,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<_Dispatcher&>().process_all()),
                std::size_t
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // ---- typed op detection ----

    // dispatcher_has_bind_impl
    //   trait: detects bind<_Event>(callable) returning handler_id.
    template<typename _Dispatcher,
             typename _Event,
             typename _Callable,
             typename = void>
    struct dispatcher_has_bind_impl
    {
        static constexpr bool value = false;
    };

    template<typename _Dispatcher,
             typename _Event,
             typename _Callable>
    struct dispatcher_has_bind_impl<_Dispatcher, _Event, _Callable,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<_Dispatcher&>().template bind<_Event>(
                    std::declval<_Callable>())),
                handler_id
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // dispatcher_has_fire_impl
    //   trait: detects fire<_Event>(args...) returning dispatch_result.
    template<typename _Dispatcher,
             typename _Event,
             typename _Void,
             typename... _Args>
    struct dispatcher_has_fire_impl
    {
        static constexpr bool value = false;
    };

    template<typename _Dispatcher,
             typename _Event,
             typename... _Args>
    struct dispatcher_has_fire_impl<_Dispatcher, _Event,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<_Dispatcher&>().template fire<_Event>(
                    std::declval<_Args>()...)),
                dispatch_result
            >::value
        >::type,
        _Args...>
    {
        static constexpr bool value = true;
    };

    // dispatcher_has_queue_impl
    //   trait: detects queue<_Event>(args...) as a well-formed expression.
    template<typename _Dispatcher,
             typename _Event,
             typename _Void,
             typename... _Args>
    struct dispatcher_has_queue_impl
    {
        static constexpr bool value = false;
    };

    template<typename _Dispatcher,
             typename _Event,
             typename... _Args>
    struct dispatcher_has_queue_impl<_Dispatcher, _Event,
        decltype(static_cast<void>(
            std::declval<_Dispatcher&>().template queue<_Event>(
                std::declval<_Args>()...)
        )),
        _Args...>
    {
        static constexpr bool value = true;
    };

NS_END  // internal


// =========================================================================
// VI.  EVENT DISPATCHER TRAITS
// =========================================================================

// event_dispatcher_traits
//   trait: compile-time structural detection for types that satisfy the
// event dispatcher facade interface. Validates the management, count, and
// queue operations expected of a dispatcher.
//
// note: structural check only; semantic contracts are not verified.
template<typename _Dispatcher>
struct event_dispatcher_traits
{
    // ---- management detection ----

    // has_unbind
    //   constant: true if unbind(handler_id) --> bool.
    static constexpr bool has_unbind =
        internal::has_dispatcher_unbind<clean_t<_Dispatcher>>::value;

    // has_enable
    //   constant: true if enable(handler_id) --> bool.
    static constexpr bool has_enable =
        internal::has_dispatcher_enable<clean_t<_Dispatcher>>::value;

    // has_disable
    //   constant: true if disable(handler_id) --> bool.
    static constexpr bool has_disable =
        internal::has_dispatcher_disable<clean_t<_Dispatcher>>::value;

    // has_is_enabled
    //   constant: true if is_enabled(handler_id) const --> bool.
    static constexpr bool has_is_enabled =
        internal::has_dispatcher_is_enabled<clean_t<_Dispatcher>>::value;

    // has_contains
    //   constant: true if contains(handler_id) const --> bool.
    static constexpr bool has_contains =
        internal::has_dispatcher_contains<clean_t<_Dispatcher>>::value;

    // ---- count detection ----

    // has_handler_count
    //   constant: true if handler_count() const --> size_t.
    static constexpr bool has_handler_count =
        internal::has_dispatcher_handler_count<clean_t<_Dispatcher>>::value;

    // has_enabled_count
    //   constant: true if enabled_count() const --> size_t.
    static constexpr bool has_enabled_count =
        internal::has_dispatcher_enabled_count<clean_t<_Dispatcher>>::value;

    // has_pending_events
    //   constant: true if pending_events() const --> size_t.
    static constexpr bool has_pending_events =
        internal::has_dispatcher_pending_events<clean_t<_Dispatcher>>::value;

    // ---- queue detection ----

    // has_process
    //   constant: true if process(size_t) --> size_t.
    static constexpr bool has_process =
        internal::has_dispatcher_process<clean_t<_Dispatcher>>::value;

    // has_process_all
    //   constant: true if process_all() --> size_t.
    static constexpr bool has_process_all =
        internal::has_dispatcher_process_all<clean_t<_Dispatcher>>::value;

    // ---- composite detection ----

    // is_event_dispatcher
    //   constant: true if _Dispatcher provides the full facade interface.
    static constexpr bool is_event_dispatcher =
        ( has_unbind         &&
          has_enable         &&
          has_disable        &&
          has_is_enabled     &&
          has_contains       &&
          has_handler_count  &&
          has_enabled_count  &&
          has_pending_events &&
          has_process        &&
          has_process_all );
};


// typed dispatcher detection (exposed for use in concepts and static_assert)

// event_dispatcher_has_bind
//   trait: true if _Dispatcher exposes bind<_Event>(_Callable) -> handler_id.
template<typename _Dispatcher,
         typename _Event,
         typename _Callable>
struct event_dispatcher_has_bind
{
    static constexpr bool value =
        internal::dispatcher_has_bind_impl<
            clean_t<_Dispatcher>, _Event, _Callable>::value;
};

// event_dispatcher_has_fire
//   trait: true if _Dispatcher exposes fire<_Event>(_Args...) ->
// dispatch_result.
template<typename _Dispatcher,
         typename _Event,
         typename... _Args>
struct event_dispatcher_has_fire
{
    static constexpr bool value =
        internal::dispatcher_has_fire_impl<
            clean_t<_Dispatcher>, _Event, void, _Args...>::value;
};

// event_dispatcher_has_queue
//   trait: true if _Dispatcher exposes queue<_Event>(_Args...) as a
// well-formed expression.
template<typename _Dispatcher,
         typename _Event,
         typename... _Args>
struct event_dispatcher_has_queue
{
    static constexpr bool value =
        internal::dispatcher_has_queue_impl<
            clean_t<_Dispatcher>, _Event, void, _Args...>::value;
};


// =========================================================================
// VII. CONCEPT CONSTRAINTS (C++20+)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// ---- core dispatcher concepts ----

// is_event_dispatcher_type
//   concept: constrains types satisfying the event dispatcher facade
// structural requirements.
template<typename _Dispatcher>
concept is_event_dispatcher_type =
    event_dispatcher_traits<clean_t<_Dispatcher>>::is_event_dispatcher;

// event_dispatcher_type
//   concept: readable spelling of is_event_dispatcher_type.
template<typename _Type>
concept event_dispatcher_type =
    is_event_dispatcher_type<clean_t<_Type>>;

// non_event_dispatcher_type
//   concept: constrains types that are not event dispatchers.
template<typename _Type>
concept non_event_dispatcher_type =
    !event_dispatcher_type<_Type>;

// processing_event_dispatcher_type
//   concept: constrains dispatchers exposing process(size_t).
template<typename _Type>
concept processing_event_dispatcher_type =
    event_dispatcher_type<_Type> &&
    event_dispatcher_traits<clean_t<_Type>>::has_process;

// draining_event_dispatcher_type
//   concept: constrains dispatchers exposing process_all().
template<typename _Type>
concept draining_event_dispatcher_type =
    event_dispatcher_type<_Type> &&
    event_dispatcher_traits<clean_t<_Type>>::has_process_all;


// ---- typed capability concepts ----

// event_dispatcher_bindable_to
//   concept: constrains (dispatcher, callable, event) triples where the
// callable is a valid handler for the event and the dispatcher can bind it.
template<typename _Dispatcher,
         typename _Callable,
         typename _Event>
concept event_dispatcher_bindable_to =
    event_dispatcher_type<_Dispatcher> &&
    handler_for<_Callable, _Event> &&
    event_dispatcher_has_bind<clean_t<_Dispatcher>, _Event, _Callable>::value;

// event_dispatcher_fireable_for
//   concept: constrains (dispatcher, event, args...) where the dispatcher
// can fire the event with those arguments.
template<typename _Dispatcher,
         typename _Event,
         typename... _Args>
concept event_dispatcher_fireable_for =
    event_dispatcher_type<_Dispatcher> &&
    is_event<clean_t<_Event>> &&
    event_dispatcher_has_fire<clean_t<_Dispatcher>, _Event, _Args...>::value;

// event_dispatcher_queueable_for
//   concept: constrains (dispatcher, event, args...) where the dispatcher
// can queue the event with those arguments.
template<typename _Dispatcher,
         typename _Event,
         typename... _Args>
concept event_dispatcher_queueable_for =
    event_dispatcher_type<_Dispatcher> &&
    is_event<clean_t<_Event>> &&
    event_dispatcher_has_queue<clean_t<_Dispatcher>, _Event, _Args...>::value;


// ---- composite capability concepts ----

// firing_event_dispatcher_for
//   concept: a dispatcher that can both bind a handler for _Event and fire
// that event with _Args.
template<typename _Dispatcher,
         typename _Callable,
         typename _Event,
         typename... _Args>
concept firing_event_dispatcher_for =
    event_dispatcher_bindable_to<_Dispatcher, _Callable, _Event> &&
    event_dispatcher_fireable_for<_Dispatcher, _Event, _Args...>;

// queueing_event_dispatcher_for
//   concept: a dispatcher that can both bind a handler for _Event and queue
// that event with _Args.
template<typename _Dispatcher,
         typename _Callable,
         typename _Event,
         typename... _Args>
concept queueing_event_dispatcher_for =
    event_dispatcher_bindable_to<_Dispatcher, _Callable, _Event> &&
    event_dispatcher_queueable_for<_Dispatcher, _Event, _Args...>;

// full_event_dispatcher_for
//   concept: a dispatcher that can bind a handler for _Event and both fire
// and queue that event with _Args.
template<typename _Dispatcher,
         typename _Callable,
         typename _Event,
         typename... _Args>
concept full_event_dispatcher_for =
    event_dispatcher_bindable_to<_Dispatcher, _Callable, _Event> &&
    event_dispatcher_fireable_for<_Dispatcher, _Event, _Args...> &&
    event_dispatcher_queueable_for<_Dispatcher, _Event, _Args...>;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_EVENT_DISPATCHER_
