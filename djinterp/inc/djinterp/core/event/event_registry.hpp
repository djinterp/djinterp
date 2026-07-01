/******************************************************************************
* djinterp [event]                                                registry.hpp
*
* The registry:
*   The typed subscription layer of the event system -- the realization of
* the registry rho and the dispatch/run folds. It wraps event_table with
* compile-time type safety: bind() verifies handler compatibility via
* static_assert and type-erases the callback; dispatch() evaluates the
* effective word for one occurrence (the fold of the handler monoid, the
* inner fold); run() folds dispatch over a homogeneous trace (the outer
* fold). It also exposes the registry merge (+) and the staging operation
* compile(), which evaluates a static word once into a single closed step.
*
*   This header is the C++ successor to event_listener.hpp; listener_registry
* is renamed event_registry to match the formal vocabulary.
*
* FORMAL CORRESPONDENCE ("Definition of an Event"):
*   registry  rho in prod_e H_e*      -- event_registry (table keyed by kappa)
*   dispatch  delta_rho               -- dispatch<_Event>(...) : the inner fold
*                                        of seq over mask_m(rho_e)
*   run       run_rho                 -- run<_Event>(first, last) : the outer
*                                        fold over a trace
*   enriched dispatch (count,verdict) -- dispatch_result
*   merge     rho (+) rho'            -- merge(other)
*   staging   hat-h, computed once    -- compile<_Event>() -> fused_step
*
* DESIGN:
*   - Typed bind: bind<_Event>() static_asserts handler_traits compatibility,
*     wraps the callable into a verdict(void*) closure, and delegates storage
*     to event_table. State S is ambient (captured); the verdict is returned.
*   - Typed dispatch: builds the payload tuple, folds seq over the bucket's
*     enabled entries (disabled entries are masked out, i.e. act as skip),
*     and stops early on a consume (the left zero). Snapshot semantics: the
*     word in force at the start of the occurrence is the one folded.
*   - All storage, mask, and lookup operations delegate to event_table.
*
* PORTABLE ACROSS:
*   C++11, C++14, C++17, C++20, C++23, C++26
*
* 
* path:      /inc/djinterp/core/event/registry.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.11
******************************************************************************/

#ifndef DJINTERP_EVENT_REGISTRY_
#define DJINTERP_EVENT_REGISTRY_ 1

// require the C++ framework header
#ifndef DJINTERP_
    #error "registry.hpp requires djinterp.h to be included first"
#endif

#ifndef __cplusplus
    #error "registry.hpp can only be used in C++ compilation mode"
#endif

#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "registry.hpp requires C++11 or higher"
#endif

// std
#include <cstddef>
#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "./event_table.hpp"


NS_DJINTERP


// =========================================================================
// I.   DISPATCH AND RUN RESULTS
// =========================================================================

// dispatch_result
//   struct: enriched result of dispatching one occurrence -- the pair
// (count, final verdict) of the instrumentation remark. Evaluating dispatch
// in the product with (N, +) counts invoked letters; the final verdict is
// the outcome of the folded word.
struct dispatch_result
{
    std::size_t invoked;
    verdict     outcome;

    // consumed
    //   true if the occurrence was consumed by one of its handlers.
    bool consumed() const
    {
        return (outcome == verdict::consume);
    };
};

// run_result
//   struct: aggregate result of running a trace -- the number of occurrences
// processed, the total handler invocations across them, and how many
// occurrences ended in a consume.
struct run_result
{
    std::size_t occurrences;
    std::size_t handlers_invoked;
    std::size_t consumed_count;
};


// =========================================================================
// II.  FUSED STEP (the staging proposition)
// =========================================================================

// fused_step
//   class: a static registry word for one event type, evaluated once into a
// single closed step (hat-h). Holds a snapshot of the effective word (the
// enabled entries, in order) taken at compile() time; invoking it folds that
// word over one occurrence's payload with no table lookup and no re-reading
// of the registry. This is the loop form of the fused path: subsequent
// registry edits do not affect a fused_step (its binding time is compile,
// not delivery).
template<typename _Event>
class fused_step
{
public:
    // payload_type
    //   type: the event's payload tuple A_e.
    using payload_type =
        typename event_traits<clean_t<_Event>>::payload_type;

    explicit fused_step(
        std::vector<std::function<verdict(void*)>> _word
    )
        : m_word(std::move(_word))
    {
    };

    // operator()
    //   folds the snapshotted word over one occurrence built from the given
    // arguments, stopping early on a consume. A fused_step is itself a
    // handler (verdict(payload...)) and so may be invoked directly, fed to
    // functional::transduce, or driven over a trace.
    template<typename... _Args>
    verdict operator()(_Args&&... _args) const
    {
        payload_type payload(std::forward<_Args>(_args)...);

        return fold(payload);
    };

    // run_one
    //   folds the snapshotted word over an already-built payload tuple,
    // stopping early on a consume. Lets a driver reuse a payload it already
    // holds without rebuilding it.
    verdict run_one(payload_type& _payload) const
    {
        return fold(_payload);
    };

    // size
    //   returns the number of handlers fused into this step.
    std::size_t size() const
    {
        return m_word.size();
    };

private:
    // fold
    //   the shared fold: invoke each snapshotted letter in order over the
    // payload, stopping on a consume (the left zero).
    verdict fold(payload_type& _payload) const
    {
        verdict outcome = verdict::pass;

        for (const auto& step : m_word)
        {
            outcome = step(static_cast<void*>(&_payload));

            // left zero: consume cuts off the remainder
            if (consumed(outcome))
            {
                break;
            }
        }

        return outcome;
    };

    std::vector<std::function<verdict(void*)>> m_word;
};


// =========================================================================
// III. EVENT REGISTRY
// =========================================================================

// event_registry
//   class: typed handler subscription manager built on top of event_table.
// Provides compile-time verified bind(), typed dispatch() and run(), the
// registry merge, and the staging operation compile(). All storage operations
// delegate to the underlying table.
class event_registry
{
public:
    event_registry()
    {
    };

    // ---- typed registration ----

    // bind
    //   registers a callable as a handler for _Event. The callable must be
    // invocable with the event's payload value domains and return void
    // (always-pass) or a verdict. Compatibility is verified at compile time
    // via static_assert.
    // returns: a handler_id handle for later management.
    template<typename _Event,
             typename _Callable>
    handler_id bind(_Callable&& _fn)
    {
        using payload_t = typename event_traits<clean_t<_Event>>::payload_type;
        using fn_t      = clean_t<_Callable>;

        // static verification that the callable is a compatible handler
        static_assert(
            handler_traits<fn_t, clean_t<_Event>>::is_compatible,
            "Handler must be invocable with the event's payload value "
            "domains and return void or verdict.");

        // type-erase the callback: wrap the callable in a shared_ptr for
        // C++11-compatible capture, then cast the void* payload back to the
        // typed tuple at dispatch time and fold-invoke.
        auto fn_ptr = std::make_shared<fn_t>(
            std::forward<_Callable>(_fn));

        std::function<verdict(void*)> callback =
            [fn_ptr](void* _raw) -> verdict
            {
                auto& payload = *static_cast<payload_t*>(_raw);

                return internal::apply_handler(
                    *fn_ptr, payload,
                    internal::make_index_sequence<
                        std::tuple_size<payload_t>::value>{});
            };

        return m_table.insert(key<_Event>(), std::move(callback));
    };

    // ---- typed dispatch (the inner fold) ----

    // dispatch
    //   evaluates the effective word for _Event over one occurrence built
    // from the given arguments: folds seq over the bucket's enabled entries,
    // stopping early on a consume.
    // returns: the enriched (count, verdict) dispatch_result.
    template<typename _Event,
             typename... _Args>
    dispatch_result dispatch(_Args&&... _args)
    {
        using payload_t = typename event_traits<clean_t<_Event>>::payload_type;

        payload_t payload(std::forward<_Args>(_args)...);

        return dispatch_payload<_Event>(payload);
    };

    // ---- typed run (the outer fold) ----

    // run
    //   folds dispatch over a homogeneous trace of _Event occurrences. Each
    // element of [_first, _last) must be (convertible to) the event's
    // payload tuple; it is copied per occurrence so that handlers taking
    // payload references mutate a per-occurrence copy rather than the source
    // range. The general heterogeneous trace is the event_queue.
    // returns: the aggregate run_result.
    template<typename _Event,
             typename _InputIt>
    run_result run(_InputIt _first,
                   _InputIt _last)
    {
        using payload_t = typename event_traits<clean_t<_Event>>::payload_type;

        run_result agg;
        agg.occurrences      = 0;
        agg.handlers_invoked = 0;
        agg.consumed_count   = 0;

        // outer fold: dispatch each occurrence of the trace in order
        for (; _first != _last; ++_first)
        {
            payload_t       payload(*_first);
            dispatch_result r = dispatch_payload<_Event>(payload);

            ++agg.occurrences;
            agg.handlers_invoked += r.invoked;

            if (r.consumed())
            {
                ++agg.consumed_count;
            }
        }

        return agg;
    };

    // ---- staging (the fused path) ----

    // compile
    //   evaluates the static effective word for _Event once into a single
    // closed step (hat-h). Captures a snapshot of the enabled entries (the
    // mask applied at compile time) so that the returned fused_step can drive
    // a trace as one loop, independently of later registry edits.
    template<typename _Event>
    fused_step<_Event> compile() const
    {
        std::vector<std::function<verdict(void*)>> word;

        const auto* entries = m_table.entries_for(key<_Event>());

        if (entries)
        {
            for (const auto& entry : *entries)
            {
                // mask applied at compile time: only enabled letters survive
                if (entry.enabled)
                {
                    word.push_back(entry.invoke);
                }
            }
        }

        return fused_step<_Event>(std::move(word));
    };

    // ---- delegated management ----

    // unbind
    //   removes the handler identified by _id.
    // returns: true if the handler was found and removed.
    bool unbind(handler_id _id)
    {
        return m_table.remove(_id);
    };

    // enable
    //   enables the handler identified by _id.
    // returns: true if the handler was found and enabled.
    bool enable(handler_id _id)
    {
        return m_table.enable(_id);
    };

    // disable
    //   disables the handler identified by _id.
    // returns: true if the handler was found and disabled.
    bool disable(handler_id _id)
    {
        return m_table.disable(_id);
    };

    // is_enabled
    //   queries whether the handler identified by _id is enabled.
    bool is_enabled(handler_id _id) const
    {
        return m_table.is_enabled(_id);
    };

    // contains
    //   queries whether the handler identified by _id exists.
    bool contains(handler_id _id) const
    {
        return m_table.contains(_id);
    };

    // ---- registry merge ----

    // merge
    //   merges _other into this registry by pointwise concatenation,
    // re-keying the merged handlers. Realizes rho (+) rho' (identity: the
    // empty registry).
    // returns: the number of handlers merged in.
    std::size_t merge(const event_registry& _other)
    {
        return m_table.merge(_other.m_table);
    };

    // ---- typed queries ----

    // handler_count_for
    //   returns the number of handlers for a specific event type.
    template<typename _Event>
    std::size_t handler_count_for() const
    {
        return m_table.count_for(key<_Event>());
    };

    // has_handlers_for
    //   returns true if at least one handler is registered for the given
    // event type.
    template<typename _Event>
    bool has_handlers_for() const
    {
        return m_table.has_entries_for(key<_Event>());
    };

    // clear_for
    //   removes all handlers for a specific event type.
    template<typename _Event>
    void clear_for()
    {
        m_table.clear_for(key<_Event>());
    };

    // ---- delegated aggregate queries ----

    // handler_count
    //   returns the total number of registered handlers.
    std::size_t handler_count() const
    {
        return m_table.total_count();
    };

    // enabled_count
    //   returns the number of enabled handlers.
    std::size_t enabled_count() const
    {
        return m_table.enabled_count();
    };

    // type_count
    //   returns the number of distinct event types with handlers.
    std::size_t type_count() const
    {
        return m_table.type_key_count();
    };

    // clear
    //   removes all handlers from all events.
    void clear()
    {
        m_table.clear();
    };

    // ---- table access ----

    // table
    //   returns a reference to the underlying event_table.
    event_table& table()
    {
        return m_table;
    };

    const event_table& table() const
    {
        return m_table;
    };

private:
    // key
    //   returns the canonical erasure key (kappa) for _Event, normalized
    // through clean_t so bind, dispatch, and queries always agree.
    template<typename _Event>
    static std::size_t key()
    {
        return internal::type_key<clean_t<_Event>>();
    };

    // dispatch_payload
    //   folds the effective word for _Event over an already-built payload
    // tuple: enabled entries are invoked in order; disabled entries are
    // masked out (act as skip); a consume cuts off the remainder.
    template<typename _Event>
    dispatch_result dispatch_payload(
        typename event_traits<clean_t<_Event>>::payload_type& _payload
    )
    {
        dispatch_result result;
        result.invoked = 0;
        result.outcome = verdict::pass;

        auto* entries = m_table.entries_for(key<_Event>());

        if (!entries)
        {
            return result;
        }

        // inner fold: seq over the bucket's enabled entries
        for (auto& entry : *entries)
        {
            if (entry.enabled)
            {
                result.outcome = entry.invoke(static_cast<void*>(&_payload));
                ++result.invoked;

                // left zero: consume cuts off the remainder for this
                // occurrence
                if (consumed(result.outcome))
                {
                    break;
                }
            }
        }

        return result;
    };

    event_table m_table;
};


NS_END  // djinterp


#endif  // DJINTERP_EVENT_REGISTRY_
