/******************************************************************************
* djinterp [event]                                          event_listener.hpp
*
* Event listener management:
*   Type-erased listener registration, lookup, and dispatch for the event
* system. Provides the subscription layer that stores multiple listeners per
* event type, keyed by a compile-time type identity, and dispatches to them
* in insertion order with propagation control.
*
* COMPONENTS:
*   djinterp::listener_id         - opaque handle for listener management
*   djinterp::listener_registry   - multi-listener subscription store with
*                                   bind, unbind, enable, disable, and
*                                   typed dispatch
*
* INTERNAL COMPONENTS:
*   djinterp::internal::type_id_value<_Event>  - per-type unique key
*   djinterp::internal::listener_entry         - type-erased listener slot
*
* DESIGN:
*   - One-to-many: each event type maps to a vector of listener entries,
*     dispatched in insertion order.
*   - Type erasure: callbacks are wrapped in std::function<void(void*)>;
*     the void* points to a dispatch_args tuple constructed at dispatch
*     time. The typed lambda captured at bind time casts it back.
*   - Propagation: listeners receive an event_context& they can mark as
*     consumed to stop further dispatch within a single fire/dispatch.
*   - Enable/disable: listeners can be toggled without removing them,
*     preserving insertion order and avoiding re-binding.
*
* FEATURE DEPENDENCIES:
*   D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES  - move semantics
*   D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES - parameter packs
*   D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES    - using aliases
*   D_ENV_CPP_FEATURE_LANG_LAMBDAS            - lambda callbacks
*
* PORTABLE ACROSS:
*   C++11, C++14, C++17, C++20, C++23, C++26
*
* path:      \inc\event\event_listener.hpp
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.03.11
******************************************************************************/

#ifndef DJINTERP_CPP_EVENT_LISTENER_
#define DJINTERP_CPP_EVENT_LISTENER_ 1

// require the C++ framework header
#ifndef DJINTERP_
    #error "event_listener.hpp requires djinterp.h to be included first"
#endif

#ifndef __cplusplus
    #error "event_listener.hpp can only be used in C++ compilation mode"
#endif

#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "event_listener.hpp requires C++11 or higher"
#endif

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <unordered_map>

#include "event_traits.hpp"


NS_DJINTERP


// =========================================================================
// I.   LISTENER IDENTIFICATION
// =========================================================================

// listener_id
//   struct: opaque handle returned from bind(), used for unbind(),
// enable(), and disable() operations. The value 0 is reserved as
// an invalid/null sentinel.
struct listener_id
{
    std::uint64_t value;

    bool operator==(const listener_id& _other) const
    {
        return (value == _other.value);
    };

    bool operator!=(const listener_id& _other) const
    {
        return (value != _other.value);
    };

    // is_valid
    //   returns true if this id refers to a real listener (non-zero).
    bool is_valid() const
    {
        return (value != 0);
    };

    // null
    //   returns an invalid listener_id sentinel.
    static listener_id null()
    {
        listener_id id;
        id.value = 0;

        return id;
    };
};


// =========================================================================
// II.  LISTENER STORAGE
// =========================================================================

NS_INTERNAL

    // type_id_value
    //   function: returns a unique identifier for each type, usable
    // as a key for the type-erased listener map. Each instantiation
    // of this function template has its own static variable whose
    // address serves as a globally unique per-type token.
    template<typename _Event>
    std::size_t type_id_value()
    {
        static const char anchor = '\0';

        return reinterpret_cast<std::size_t>(&anchor);
    };

    // listener_entry
    //   struct: type-erased listener storage with enable/disable
    // support. The `invoke` callable expects a void* pointing to a
    // std::tuple<event_context&, args_type> constructed by the
    // dispatch site.
    struct listener_entry
    {
        listener_id                    id;
        std::function<void(void*)>     invoke;
        bool                           enabled;
    };

NS_END  // internal


// =========================================================================
// III. LISTENER REGISTRY
// =========================================================================

// listener_registry
//   class: manages listener subscriptions. Stores multiple listeners
// per event type, keyed by a type-erased event ID. Provides bind,
// unbind, enable, disable, and typed dispatch operations.
class listener_registry
{
private:
    using entry_type = internal::listener_entry;
    using entry_list = std::vector<entry_type>;
    using registry   = std::unordered_map<std::size_t, entry_list>;

public:
    listener_registry()
        : m_next_id(1)
        , m_total_count(0)
        , m_enabled_count(0)
    {
    };

    // bind
    //   registers a callable as a listener for _Event. The callable
    // must accept (event_context&, Args...) where Args... matches
    // _Event::args_type.
    // returns: a listener_id handle for later management.
    template<typename _Event,
             typename _Callable>
    listener_id bind(_Callable&& _fn)
    {
        using traits    = event_traits<_Event>;
        using args_type = typename traits::args_type;

        // static verification that the callable is compatible
        static_assert(
            event_listener_traits<
                typename std::decay<_Callable>::type,
                _Event>::is_compatible,
            "Callable must accept (event_context&, Args...) "
            "matching the event's args_type.");

        // generate unique id
        listener_id lid;
        lid.value = m_next_id++;

        // type-erase the callback: wrap the callable in a
        // shared_ptr for C++11-compatible lambda capture, then
        // cast the void* dispatch args back to the typed tuple.
        using fn_type = typename std::decay<_Callable>::type;

        auto fn_ptr = std::make_shared<fn_type>(
            std::forward<_Callable>(_fn));

        auto callback =
            [fn_ptr](void* _raw_args)
            {
                using dispatch_args =
                    std::tuple<event_context&, args_type>;

                auto& pack =
                    *static_cast<dispatch_args*>(_raw_args);

                auto& ctx  = std::get<0>(pack);
                auto& args = std::get<1>(pack);

                // check consumed before invoking
                if (!ctx.is_consumed())
                {
                    invoke_with_context(*fn_ptr, ctx, args,
                        internal::make_index_sequence<
                            std::tuple_size<args_type>::value>{});
                }
            };

        entry_type entry;
        entry.id      = lid;
        entry.invoke  = std::move(callback);
        entry.enabled = true;

        std::size_t key = internal::type_id_value<_Event>();
        m_registry[key].push_back(std::move(entry));
        ++m_total_count;
        ++m_enabled_count;

        return lid;
    };

    // unbind
    //   removes the listener identified by _id.
    // returns: true if the listener was found and removed.
    bool unbind(listener_id _id)
    {
        for (auto& kv : m_registry)
        {
            auto& entries = kv.second;

            for (auto it = entries.begin();
                 it != entries.end();
                 ++it)
            {
                if (it->id == _id)
                {
                    if (it->enabled)
                    {
                        --m_enabled_count;
                    }

                    entries.erase(it);
                    --m_total_count;

                    return true;
                }
            }
        }

        return false;
    };

    // enable
    //   enables the listener identified by _id.
    // returns: true if the listener was found and enabled.
    bool enable(listener_id _id)
    {
        entry_type* entry = find_entry(_id);

        if ( (!entry) ||
             (entry->enabled) )
        {
            return false;
        }

        entry->enabled = true;
        ++m_enabled_count;

        return true;
    };

    // disable
    //   disables the listener identified by _id. Disabled listeners
    // remain registered but are skipped during dispatch.
    // returns: true if the listener was found and disabled.
    bool disable(listener_id _id)
    {
        entry_type* entry = find_entry(_id);

        if ( (!entry) ||
             (!entry->enabled) )
        {
            return false;
        }

        entry->enabled = false;
        --m_enabled_count;

        return true;
    };

    // is_enabled
    //   queries whether the listener identified by _id is currently
    // enabled.
    // returns: true if found and enabled, false otherwise.
    bool is_enabled(listener_id _id) const
    {
        const entry_type* entry = find_entry_const(_id);

        if (!entry)
        {
            return false;
        }

        return entry->enabled;
    };

    // contains
    //   queries whether the listener identified by _id exists.
    bool contains(listener_id _id) const
    {
        return (find_entry_const(_id) != nullptr);
    };

    // dispatch
    //   invokes all enabled listeners for _Event with the given
    // arguments. Stops early if a listener consumes the event.
    // returns: number of listeners invoked.
    template<typename _Event,
             typename... _Args>
    std::size_t dispatch(_Args&&... _args)
    {
        using traits    = event_traits<_Event>;
        using args_type = typename traits::args_type;

        std::size_t key = internal::type_id_value<_Event>();

        auto it = m_registry.find(key);

        if (it == m_registry.end())
        {
            return 0;
        }

        event_context ctx;
        args_type     args(std::forward<_Args>(_args)...);

        using dispatch_args =
            std::tuple<event_context&, args_type>;

        dispatch_args pack(ctx, args);

        std::size_t count = 0;

        for (auto& entry : it->second)
        {
            if (ctx.is_consumed())
            {
                break;
            }

            if (entry.enabled)
            {
                entry.invoke(static_cast<void*>(&pack));
                ++count;
            }
        }

        return count;
    };

    // listener_count
    //   returns the total number of registered listeners.
    std::size_t listener_count() const
    {
        return m_total_count;
    };

    // enabled_count
    //   returns the number of enabled listeners.
    std::size_t enabled_count() const
    {
        return m_enabled_count;
    };

    // listener_count_for
    //   returns the number of listeners for a specific event type.
    template<typename _Event>
    std::size_t listener_count_for() const
    {
        std::size_t key = internal::type_id_value<_Event>();

        auto it = m_registry.find(key);

        if (it == m_registry.end())
        {
            return 0;
        }

        return it->second.size();
    };

    // has_listeners_for
    //   returns true if at least one listener is registered for
    // the given event type.
    template<typename _Event>
    bool has_listeners_for() const
    {
        std::size_t key = internal::type_id_value<_Event>();

        auto it = m_registry.find(key);

        return ( (it != m_registry.end()) &&
                 (!it->second.empty()) );
    };

    // clear
    //   removes all listeners from all events.
    void clear()
    {
        m_registry.clear();
        m_total_count   = 0;
        m_enabled_count = 0;
    };

    // clear_for
    //   removes all listeners for a specific event type.
    template<typename _Event>
    void clear_for()
    {
        std::size_t key = internal::type_id_value<_Event>();

        auto it = m_registry.find(key);

        if (it != m_registry.end())
        {
            // adjust counts
            for (const auto& entry : it->second)
            {
                if (entry.enabled)
                {
                    --m_enabled_count;
                }

                --m_total_count;
            }

            m_registry.erase(it);
        }
    };

private:
    // find_entry
    //   locates a listener by id across all event buckets.
    entry_type* find_entry(listener_id _id)
    {
        for (auto& kv : m_registry)
        {
            for (auto& entry : kv.second)
            {
                if (entry.id == _id)
                {
                    return &entry;
                }
            }
        }

        return nullptr;
    };

    // find_entry_const
    //   const overload; locates a listener by id across all event
    // buckets.
    const entry_type* find_entry_const(listener_id _id) const
    {
        for (const auto& kv : m_registry)
        {
            for (const auto& entry : kv.second)
            {
                if (entry.id == _id)
                {
                    return &entry;
                }
            }
        }

        return nullptr;
    };

    // invoke_with_context
    //   unpacks the argument tuple and invokes the callable with
    // event_context as first parameter.
    template<typename _Callable,
             typename _ArgsTuple,
             std::size_t... _I>
    static void invoke_with_context(_Callable&      _fn,
                                    event_context&  _ctx,
                                    _ArgsTuple&     _args,
                                    internal::index_sequence<_I...>)
    {
        _fn(_ctx, std::get<_I>(_args)...);
    };

    registry      m_registry;
    std::uint64_t m_next_id;
    std::size_t   m_total_count;
    std::size_t   m_enabled_count;
};


NS_END  // djinterp


#endif  // DJINTERP_CPP_EVENT_LISTENER_
