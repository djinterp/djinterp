/******************************************************************************
* djinterp [event]                                          event_listener.hpp
*
* Event listener management:
*   Typed listener registration and dispatch for the event system. Provides
* the subscription layer that wraps event_table with compile-time type
* safety: bind() verifies callable compatibility via static_assert,
* dispatch() constructs typed argument tuples and invokes listeners with
* propagation control.
*
* COMPONENTS:
*   djinterp::listener_registry   - typed listener subscription manager
*
* DESIGN:
*   - Typed bind: bind<_Event>() static_asserts callable compatibility
*     using event_listener_traits, then type-erases the callback and
*     delegates storage to event_table.
*   - Typed dispatch: dispatch<_Event>() constructs a typed argument
*     tuple, packs it with an event_context, and iterates the table's
*     entries for that event type, invoking each enabled listener.
*   - All storage, enable/disable, and lookup operations are delegated
*     to the underlying event_table.
*   - Propagation: dispatch stops early when a listener consumes the
*     event via event_context::consume().
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
* 
* path:      /inc/djinterp/core/event/event_listener.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.11
******************************************************************************/

#ifndef DJINTERP_EVENT_LISTENER_
#define DJINTERP_EVENT_LISTENER_ 1

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
#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

#include "event_table.hpp"


NS_DJINTERP


// =========================================================================
// I.   LISTENER REGISTRY
// =========================================================================

// listener_registry
//   class: typed listener subscription manager built on top of
// event_table. Provides compile-time verified bind() and typed
// dispatch(), delegating all storage operations to the underlying
// table.
class listener_registry
{
public:
    listener_registry()
    {
    };

    // ---- typed registration ----

    // bind
    //   registers a callable as a listener for _Event. The callable
    // must accept (event_context&, Args...) where Args... matches
    // _Event::args_type. Compatibility is verified at compile time
    // via static_assert.
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

        // type-erase the callback: wrap the callable in a
        // shared_ptr for C++11-compatible lambda capture, then
        // cast the void* dispatch args back to the typed tuple.
        using fn_type = typename std::decay<_Callable>::type;

        auto fn_ptr = std::make_shared<fn_type>(
            std::forward<_Callable>(_fn));

        std::function<void(void*)> callback =
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

        std::size_t key = internal::type_id_value<_Event>();

        return m_table.insert(key, std::move(callback));
    };

    // ---- typed dispatch ----

    // dispatch
    //   invokes all enabled listeners for _Event with the given
    // arguments. Constructs a typed argument tuple, packs it with
    // an event_context, and iterates the table's entries. Stops
    // early if a listener consumes the event.
    // returns: number of listeners invoked.
    template<typename _Event,
             typename... _Args>
    std::size_t dispatch(_Args&&... _args)
    {
        using traits    = event_traits<_Event>;
        using args_type = typename traits::args_type;

        std::size_t key = internal::type_id_value<_Event>();

        auto* entries = m_table.entries_for(key);

        if (!entries)
        {
            return 0;
        }

        event_context ctx;
        args_type     args(std::forward<_Args>(_args)...);

        using dispatch_args =
            std::tuple<event_context&, args_type>;

        dispatch_args pack(ctx, args);

        std::size_t count = 0;

        for (auto& entry : *entries)
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

    // ---- delegated management ----

    // unbind
    //   removes the listener identified by _id.
    // returns: true if the listener was found and removed.
    bool unbind(listener_id _id)
    {
        return m_table.remove(_id);
    };

    // enable
    //   enables the listener identified by _id.
    // returns: true if the listener was found and enabled.
    bool enable(listener_id _id)
    {
        return m_table.enable(_id);
    };

    // disable
    //   disables the listener identified by _id.
    // returns: true if the listener was found and disabled.
    bool disable(listener_id _id)
    {
        return m_table.disable(_id);
    };

    // is_enabled
    //   queries whether the listener identified by _id is enabled.
    bool is_enabled(listener_id _id) const
    {
        return m_table.is_enabled(_id);
    };

    // contains
    //   queries whether the listener identified by _id exists.
    bool contains(listener_id _id) const
    {
        return m_table.contains(_id);
    };

    // ---- typed queries ----

    // listener_count_for
    //   returns the number of listeners for a specific event type.
    template<typename _Event>
    std::size_t listener_count_for() const
    {
        return m_table.count_for(
            internal::type_id_value<_Event>());
    };

    // has_listeners_for
    //   returns true if at least one listener is registered for
    // the given event type.
    template<typename _Event>
    bool has_listeners_for() const
    {
        return m_table.has_entries_for(
            internal::type_id_value<_Event>());
    };

    // clear_for
    //   removes all listeners for a specific event type.
    template<typename _Event>
    void clear_for()
    {
        m_table.clear_for(
            internal::type_id_value<_Event>());
    };

    // ---- delegated aggregate queries ----

    // listener_count
    //   returns the total number of registered listeners.
    std::size_t listener_count() const
    {
        return m_table.total_count();
    };

    // enabled_count
    //   returns the number of enabled listeners.
    std::size_t enabled_count() const
    {
        return m_table.enabled_count();
    };

    // type_count
    //   returns the number of distinct event types with listeners.
    std::size_t type_count() const
    {
        return m_table.type_key_count();
    };

    // clear
    //   removes all listeners from all events.
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
    // invoke_with_context
    //   unpacks the argument tuple and invokes the callable with
    // event_context as the first parameter.
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

    event_table m_table;
};


NS_END  // djinterp


#endif  // DJINTERP_EVENT_LISTENER_
