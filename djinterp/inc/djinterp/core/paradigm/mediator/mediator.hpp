/******************************************************************************
* djinterp [paradigm]                                             mediator.hpp
*
* Mediator Pattern Module:
*   Provides a comprehensive, abstract, version-portable foundation for the
* mediator pattern. Centralises colleague-to-colleague communication so that
* participants interact through a single coordinating object rather than
* holding direct references to one another.
*
*   DESIGN:
*   The module is organised in five layers:
*     1. TRAITS — SFINAE-based detection of mediator protocol conformance
*        (notify, subscribe, set_mediator) on arbitrary types.
*     2. CLASSIC — abstract virtual mediator/colleague base classes for
*        runtime polymorphic dispatch.
*     3. EVENT — type-indexed event bus where handlers register for
*        specific event types; dispatch uses compat::any for type erasure,
*        making it available from C++11 without RTTI.
*     4. SIGNAL — lightweight signal/slot callback registry providing
*        named channels with multi-cast notification.
*     5. STATIC — CRTP-based compile-time mediator with zero virtual
*        overhead; the colleague set is fixed at compile time.
*
*   PORTABILITY:
*   - C++11  : classic mediator/colleague, event_bus (via compat::any),
*              signal_hub, colleague_base, SFINAE traits
*   - C++14  : generic lambda handlers, make_* factories
*   - C++17  : if constexpr dispatch, std::string_view channel names,
*              variant_event_bus for closed event sets
*   - C++20  : concept-constrained mediator_for, colleague_of, handler_for
*

* path:      /inc/djinterp/core/paradigm/mediator/mediator.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.12
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    CONFIGURATION & FEATURE GATES
      --------------------------------
      i.    D_MEDIATOR_HAS_IF_CONSTEXPR
      ii.   D_MEDIATOR_HAS_CONCEPTS
      iii.  D_MEDIATOR_HAS_STRING_VIEW
      iv.   D_MEDIATOR_HAS_VARIANT
      v.    D_MEDIATOR_DEFAULT_MAX_COLLEAGUES

II.   MEDIATOR TRAITS
      ------------------
      i.    has_notify_method
      ii.   has_subscribe_method
      iii.  has_set_mediator_method
      iv.   has_on_event_method
      v.    is_mediator_like
      vi.   is_colleague_like
      vii.  mediator_capability (aggregate)

III.  CLASSIC MEDIATOR (C++11+)
      ----------------------------
      i.    colleague_id
      ii.   mediator_base
      iii.  colleague_base
      iv.   concrete_mediator

IV.   EVENT BUS (C++11+, via compat::any)
      ---------------------------------------
      i.    event_handler_base (internal)
      ii.   typed_event_handler (internal)
      iii.  event_bus

V.    VARIANT EVENT BUS (C++17+)
      ------------------------------
      i.    variant_event_bus

VI.   SIGNAL HUB (C++11+)
      -----------------------
      i.    signal_id
      ii.   connection (internal)
      iii.  signal_hub

VII.  STATIC MEDIATOR — CRTP (C++11+)
      -----------------------------------
      i.    static_colleague
      ii.   static_mediator

VIII. DISPATCH POLICIES
      --------------------
      i.    broadcast_policy
      ii.   targeted_policy
      iii.  filtered_policy

IX.   CONVENIENCE FACTORIES (C++14+)
      ----------------------------------
      i.    make_event_bus
      ii.   make_signal_hub

X.    CONCEPT-CONSTRAINED INTERFACES (C++20+)
      -------------------------------------------
      i.    mediator_for (concept)
      ii.   colleague_of (concept)
      iii.  handler_for (concept)
*/

#ifndef DJINTERP_PARADIGM_MEDIATOR_
#define DJINTERP_PARADIGM_MEDIATOR_ 1

// std
#include <cstddef>
#include <functional>
#include <type_traits>
#include <vector>
// djinterp
#include "../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "compat/std/any.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <utility>
    #include <memory>
    #include <algorithm>
    #include <string>
#endif

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #include <string_view>
    #include <variant>
    #include <optional>
#endif


///////////////////////////////////////////////////////////////////////////////
///          I.    CONFIGURATION & FEATURE GATES                            ///
///////////////////////////////////////////////////////////////////////////////

// D_MEDIATOR_HAS_IF_CONSTEXPR
//   macro: 1 if if-constexpr is available (C++17+).
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #define D_MEDIATOR_HAS_IF_CONSTEXPR 1
#else
    #define D_MEDIATOR_HAS_IF_CONSTEXPR 0
#endif

// D_MEDIATOR_HAS_CONCEPTS
//   macro: 1 if concepts are available (C++20+).
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    #define D_MEDIATOR_HAS_CONCEPTS 1
#else
    #define D_MEDIATOR_HAS_CONCEPTS 0
#endif

// D_MEDIATOR_HAS_STRING_VIEW
//   macro: 1 if std::string_view is available (C++17+).
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #define D_MEDIATOR_HAS_STRING_VIEW 1
#else
    #define D_MEDIATOR_HAS_STRING_VIEW 0
#endif

// D_MEDIATOR_HAS_VARIANT
//   macro: 1 if std::variant is available (C++17+).
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #define D_MEDIATOR_HAS_VARIANT 1
#else
    #define D_MEDIATOR_HAS_VARIANT 0
#endif

// D_MEDIATOR_DEFAULT_MAX_COLLEAGUES
//   macro: default maximum colleague count for fixed-size arrays.
// Users may define this before including mediator.hpp.
#ifndef D_MEDIATOR_DEFAULT_MAX_COLLEAGUES
    #define D_MEDIATOR_DEFAULT_MAX_COLLEAGUES 64
#endif


NS_DJINTERP

#if D_ENV_LANG_IS_CPP11_OR_HIGHER


///////////////////////////////////////////////////////////////////////////////
///            II.   MEDIATOR TRAITS                                        ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // has_notify_method
    //   trait: detects _T::notify(args...) accepting a type-erased event.
    template<typename _T,
             typename = void>
    struct has_notify_method : std::false_type
    {};

    template<typename _T>
    struct has_notify_method<_T, D_VOID_T<decltype(
        std::declval<_T>().notify(
            std::declval<const compat::any&>()))
    >> : std::true_type
    {};

    // has_subscribe_method
    //   trait: detects _T::subscribe(...) accepting a callback or handler.
    template<typename _T,
             typename = void>
    struct has_subscribe_method : std::false_type
    {};

    template<typename _T>
    struct has_subscribe_method<_T, D_VOID_T<decltype(
        std::declval<_T>().subscribe(
            std::declval<std::function<void(const compat::any&)>>()))
    >> : std::true_type
    {};

    // has_set_mediator_method
    //   trait: detects _T::set_mediator(mediator*).
    template<typename _T,
             typename = void>
    struct has_set_mediator_method : std::false_type
    {};

    template<typename _T>
    struct has_set_mediator_method<_T, D_VOID_T<decltype(
        std::declval<_T>().set_mediator(
            std::declval<std::nullptr_t>()))
    >> : std::true_type
    {};

    // has_on_event_method
    //   trait: detects _T::on_event(any) handler.
    template<typename _T,
             typename = void>
    struct has_on_event_method : std::false_type
    {};

    template<typename _T>
    struct has_on_event_method<_T, D_VOID_T<decltype(
        std::declval<_T>().on_event(
            std::declval<const compat::any&>()))
    >> : std::true_type
    {};

NS_END  // internal

// is_mediator_like
//   trait: true if _T exposes a notify-capable interface.
template<typename _T>
struct is_mediator_like
{
    static constexpr bool value =
        internal::has_notify_method<_T>::value;
};

// is_colleague_like
//   trait: true if _T can participate in mediation (has set_mediator
// or on_event).
template<typename _T>
struct is_colleague_like
{
    static constexpr bool value =
        ( internal::has_set_mediator_method<_T>::value ||
          internal::has_on_event_method<_T>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename _T>
    constexpr bool is_mediator_like_v =
        is_mediator_like<_T>::value;

    template<typename _T>
    constexpr bool is_colleague_like_v =
        is_colleague_like<_T>::value;

#endif

// mediator_capability
//   struct: aggregate classification of a type's mediator conformance.
template<typename _T>
struct mediator_capability
{
    static constexpr bool has_notify =
        internal::has_notify_method<_T>::value;

    static constexpr bool has_subscribe =
        internal::has_subscribe_method<_T>::value;

    static constexpr bool has_set_mediator =
        internal::has_set_mediator_method<_T>::value;

    static constexpr bool has_on_event =
        internal::has_on_event_method<_T>::value;

    static constexpr bool is_mediator =
        is_mediator_like<_T>::value;

    static constexpr bool is_colleague =
        is_colleague_like<_T>::value;
};


///////////////////////////////////////////////////////////////////////////////
///          III.  CLASSIC MEDIATOR (C++11+)                                ///
///////////////////////////////////////////////////////////////////////////////

// The classic mediator uses virtual dispatch. Colleagues hold a pointer
// to mediator_base and call send(); the mediator's receive() routes
// events to the appropriate colleagues.

// colleague_id
//   type: opaque identifier for a registered colleague.
using colleague_id = std::size_t;

// mediator_base
//   class: abstract base for classic mediators. Concrete mediators
// override receive() to implement routing logic.
class mediator_base
{
public:
    virtual ~mediator_base()
    {}

    // receive
    //   function: called by a colleague to route an event through
    // the mediator. _sender identifies the originator; _event is
    // the type-erased payload.
    virtual void receive(
        colleague_id       _sender,
        const compat::any& _event
    ) = 0;
};

// colleague_base
//   class: abstract base for participants in classic mediation.
// Each colleague holds a mediator pointer and a unique id.
class colleague_base
{
public:
    colleague_base()
        : m_mediator(nullptr),
          m_id(0)
    {}

    virtual ~colleague_base()
    {}

    // set_mediator
    //   function: binds this colleague to a mediator.
    void
    set_mediator(
        mediator_base* _mediator,
        colleague_id   _id
    ) noexcept
    {
        m_mediator = _mediator;
        m_id       = _id;

        return;
    }

    // id
    //   function: returns this colleague's identifier.
    colleague_id
    id() const noexcept
    {
        return m_id;
    }

    // on_event
    //   function: called by the mediator to deliver an event.
    // Override in concrete colleagues.
    virtual void on_event(
        colleague_id       _sender,
        const compat::any& _event
    ) = 0;

protected:
    // send
    //   function: sends an event through the mediator. Only
    // callable by derived colleagues.
    void
    send(
        const compat::any& _event
    )
    {
        if (m_mediator)
        {
            m_mediator->receive(m_id, _event);
        }

        return;
    }

    mediator_base* mediator() const noexcept
    {
        return m_mediator;
    }

private:
    mediator_base* m_mediator;
    colleague_id   m_id;
};

// concrete_mediator
//   class: a generic concrete mediator that manages a dynamic set
// of colleagues. receive() broadcasts the event to all registered
// colleagues except the sender.
class concrete_mediator : public mediator_base
{
public:
    // add_colleague
    //   function: registers a colleague, assigns it an id, and
    // binds it to this mediator.
    colleague_id
    add_colleague(
        colleague_base* _colleague
    )
    {
        colleague_id cid = m_next_id++;
        _colleague->set_mediator(this, cid);
        m_colleagues.push_back(_colleague);

        return cid;
    }

    // remove_colleague
    //   function: unregisters a colleague by id.
    void
    remove_colleague(
        colleague_id _id
    )
    {
        m_colleagues.erase(
            std::remove_if(m_colleagues.begin(),
                           m_colleagues.end(),
                           [_id](colleague_base* c)
                           {
                               return (c->id() == _id);
                           }),
            m_colleagues.end());

        return;
    }

    // receive
    //   function: broadcasts an event to all colleagues except the
    // sender.
    void receive(
        colleague_id       _sender,
        const compat::any& _event
    ) override
    {
        for (auto* colleague : m_colleagues)
        {
            if (colleague->id() != _sender)
            {
                colleague->on_event(_sender, _event);
            }
        }

        return;
    }

    // colleague_count
    //   function: returns the number of registered colleagues.
    std::size_t
    colleague_count() const noexcept
    {
        return m_colleagues.size();
    }

private:
    std::vector<colleague_base*> m_colleagues;
    colleague_id                 m_next_id = 0;
};


///////////////////////////////////////////////////////////////////////////////
///           IV.   EVENT BUS (C++11+, via compat::any)                     ///
///////////////////////////////////////////////////////////////////////////////

// The event bus is a type-indexed publish/subscribe mediator. Handlers
// register for a specific event type _Event; when publish<_Event>(e)
// is called, all handlers for that type are invoked. Uses compat::any
// for type erasure and compat::any_type_id for the type index —
// available from C++11 with no RTTI.

NS_INTERNAL

    // event_handler_base
    //   class: type-erased handler interface.
    struct event_handler_base
    {
        virtual ~event_handler_base()
        {}

        virtual void invoke(const compat::any& _event) = 0;
        virtual compat::any_type_id event_type() const noexcept = 0;
    };

    // typed_event_handler
    //   class: concrete handler wrapping a callable for event type _Event.
    template<typename _Event,
             typename _Fn>
    class typed_event_handler : public event_handler_base
    {
    public:
        explicit typed_event_handler(
                _Fn _fn
            )
                : m_fn(std::move(_fn))
            {}

        void invoke(
            const compat::any& _event
        ) override
        {
            // safe to cast: dispatch is gated on type id
            m_fn(_event.template get<_Event>());

            return;
        }

        compat::any_type_id event_type() const noexcept override
        {
            return compat::any_type_id_of<_Event>::value;
        }

    private:
        _Fn m_fn;
    };

NS_END  // internal

// subscription_token
//   type: opaque handle returned by event_bus::subscribe, used
// to unsubscribe a specific handler.
using subscription_token = std::size_t;

// event_bus
//   class: type-indexed publish/subscribe mediator. Handlers
// subscribe to specific event types and are invoked when an event
// of that type is published.
//
// Usage:
//   event_bus bus;
//   auto tok = bus.subscribe<mouse_click>(
//       [](const mouse_click& e) { handle(e); });
//   bus.publish(mouse_click{42, 100});
//   bus.unsubscribe(tok);
class event_bus
{
public:
    // subscribe
    //   function: registers a handler for events of type _Event.
    // Returns a token that can be used to unsubscribe.
    template<typename _Event,
             typename _Fn>
    subscription_token
    subscribe(
        _Fn _fn
    )
    {
        subscription_token tok = m_next_token++;

        m_handlers.push_back(
            { tok,
              std::unique_ptr<internal::event_handler_base>(
                  new internal::typed_event_handler<_Event, _Fn>(
                      std::move(_fn))) });

        return tok;
    }

    // unsubscribe
    //   function: removes a handler by its token.
    void
    unsubscribe(
        subscription_token _token
    )
    {
        m_handlers.erase(
            std::remove_if(m_handlers.begin(),
                           m_handlers.end(),
                           [_token](const handler_entry& e)
                           {
                               return (e.token == _token);
                           }),
            m_handlers.end());

        return;
    }

    // publish
    //   function: dispatches an event to all handlers registered
    // for type _Event.
    template<typename _Event>
    void
    publish(
        const _Event& _event
    )
    {
        compat::any wrapped(_event);
        compat::any_type_id target_id =
            compat::any_type_id_of<_Event>::value;

        for (auto& entry : m_handlers)
        {
            if (entry.handler->event_type() == target_id)
            {
                entry.handler->invoke(wrapped);
            }
        }

        return;
    }

    // publish (move)
    //   function: dispatches a moved event.
    template<typename _Event>
    void
    publish(
        _Event&& _event
    )
    {
        compat::any wrapped(std::forward<_Event>(_event));
        compat::any_type_id target_id =
            compat::any_type_id_of<
                typename std::remove_reference<_Event>::type>::value;

        for (auto& entry : m_handlers)
        {
            if (entry.handler->event_type() == target_id)
            {
                entry.handler->invoke(wrapped);
            }
        }

        return;
    }

    // notify
    //   function: type-erased publish accepting a pre-wrapped
    // compat::any. Dispatches to all handlers whose event_type()
    // matches the any's stored type id.
    void
    notify(
        const compat::any& _event
    )
    {
        compat::any_type_id source_id = _event.type();

        for (auto& entry : m_handlers)
        {
            if (entry.handler->event_type() == source_id)
            {
                entry.handler->invoke(_event);
            }
        }

        return;
    }

    // handler_count
    //   function: total number of active subscriptions.
    std::size_t
    handler_count() const noexcept
    {
        return m_handlers.size();
    }

    // handler_count_for
    //   function: number of handlers registered for event type _Event.
    template<typename _Event>
    std::size_t
    handler_count_for() const noexcept
    {
        compat::any_type_id target_id =
            compat::any_type_id_of<_Event>::value;
        std::size_t count = 0;

        for (const auto& entry : m_handlers)
        {
            if (entry.handler->event_type() == target_id)
            {
                ++count;
            }
        }

        return count;
    }

    // clear
    //   function: removes all handlers.
    void
    clear()
    {
        m_handlers.clear();

        return;
    }

private:
    struct handler_entry
    {
        subscription_token                            token;
        std::unique_ptr<internal::event_handler_base> handler;
    };

    std::vector<handler_entry> m_handlers;
    subscription_token         m_next_token = 0;
};


///////////////////////////////////////////////////////////////////////////////
///          V.    VARIANT EVENT BUS (C++17+)                               ///
///////////////////////////////////////////////////////////////////////////////

#if D_MEDIATOR_HAS_VARIANT

// variant_event_bus
//   class: closed-set event bus parameterised on a fixed list of event
// types. Uses std::variant internally, enabling exhaustive handling and
// compile-time verification that all events are covered.
//
// Usage:
//   using bus = variant_event_bus<click, keypress, resize>;
//   bus b;
//   b.subscribe([](const click& c) { ... });
//   b.subscribe([](const keypress& k) { ... });
//   b.subscribe([](const resize& r) { ... });
//   b.publish(click{10, 20});
template<typename... _Events>
class variant_event_bus
{
public:
    using event_variant = std::variant<_Events...>;

    // subscribe
    //   function: registers a handler for a single event type _Event.
    template<typename _Event,
             typename _Fn>
    subscription_token
    subscribe(
        _Fn _fn
    )
    {
        static_assert(
            (std::is_same_v<_Event, _Events> || ...),
            "Event type is not in this variant_event_bus's type list.");

        subscription_token tok = m_next_token++;

        m_handlers.push_back(
            { tok,
              [fn = std::move(_fn)](const event_variant& _v)
              {
                  if (auto* p = std::get_if<_Event>(&_v))
                  {
                      fn(*p);
                  }
              } });

        return tok;
    }

    // subscribe_all
    //   function: registers a visitor-style handler that is invoked
    // for every event type. _Fn must be callable with each _Events&.
    template<typename _Fn>
    subscription_token
    subscribe_all(
        _Fn _fn
    )
    {
        subscription_token tok = m_next_token++;

        m_handlers.push_back(
            { tok,
              [fn = std::move(_fn)](const event_variant& _v)
              {
                  std::visit(fn, _v);
              } });

        return tok;
    }

    // unsubscribe
    //   function: removes a handler by its token.
    void
    unsubscribe(
        subscription_token _token
    )
    {
        m_handlers.erase(
            std::remove_if(m_handlers.begin(),
                           m_handlers.end(),
                           [_token](const handler_entry& e)
                           {
                               return (e.token == _token);
                           }),
            m_handlers.end());

        return;
    }

    // publish
    //   function: wraps the event in a variant and dispatches to all
    // handlers.
    template<typename _Event>
    void
    publish(
        const _Event& _event
    )
    {
        static_assert(
            (std::is_same_v<_Event, _Events> || ...),
            "Event type is not in this variant_event_bus's type list.");

        event_variant v(_event);

        for (auto& entry : m_handlers)
        {
            entry.handler(v);
        }

        return;
    }

    // handler_count
    std::size_t
    handler_count() const noexcept
    {
        return m_handlers.size();
    }

    // clear
    void
    clear()
    {
        m_handlers.clear();

        return;
    }

private:
    struct handler_entry
    {
        subscription_token                        token;
        std::function<void(const event_variant&)> handler;
    };

    std::vector<handler_entry> m_handlers;
    subscription_token         m_next_token = 0;
};

#endif  // D_MEDIATOR_HAS_VARIANT


///////////////////////////////////////////////////////////////////////////////
///           VI.   SIGNAL HUB (C++11+)                                     ///
///////////////////////////////////////////////////////////////////////////////

// The signal hub is a named-channel multicast mediator. Participants
// connect callables to named signals; emitting a signal invokes all
// connected callbacks. Channel names are std::string (C++11) or
// std::string_view (C++17).

// signal_id
//   type: channel identifier.
using signal_id = std::string;

// signal_hub
//   class: named-signal multicast mediator. Callbacks are registered
// under string keys and invoked when that signal is emitted.
//
// Usage:
//   signal_hub hub;
//   auto tok = hub.connect("button_clicked",
//       [](const compat::any& e) { handle(e); });
//   hub.emit("button_clicked", compat::any(42));
//   hub.disconnect(tok);
class signal_hub
{
public:
    using handler_fn = std::function<void(const compat::any&)>;

    // connect
    //   function: registers a handler on the named channel. Returns
    // a token for disconnection.
    subscription_token
    connect(
        const signal_id& _channel,
        handler_fn       _fn
    )
    {
        subscription_token tok = m_next_token++;
        m_slots.push_back({ tok, _channel, std::move(_fn) });

        return tok;
    }

    // connect (const char* overload)
    //   Disambiguates a string-literal channel: without this, a literal
    // converts equally well to const signal_id& and to std::string_view under
    // C++17, so connect("chan", fn) is ambiguous. A const char* parameter binds
    // the literal by array-to-pointer (a standard conversion), which is a
    // better match than either user-defined conversion.
    subscription_token
    connect(
        const char* _channel,
        handler_fn  _fn
    )
    {
        return connect(signal_id(_channel), std::move(_fn));
    }

#if D_MEDIATOR_HAS_STRING_VIEW
    // connect (string_view overload)
    subscription_token
    connect(
        std::string_view _channel,
        handler_fn       _fn
    )
    {
        return connect(signal_id(_channel), std::move(_fn));
    }
#endif

    // connect_typed
    //   function: convenience — registers a handler that automatically
    // extracts the event type from the compat::any wrapper.
    template<typename _Event,
             typename _Fn>
    subscription_token
    connect_typed(
        const signal_id& _channel,
        _Fn              _fn
    )
    {
        return connect(
            _channel,
            [fn = std::move(_fn)](const compat::any& _e)
            {
                if (_e.template holds<_Event>())
                {
                    fn(_e.template get<_Event>());
                }
            });
    }

    // disconnect
    //   function: removes a handler by its token.
    void
    disconnect(
        subscription_token _token
    )
    {
        m_slots.erase(
            std::remove_if(m_slots.begin(),
                           m_slots.end(),
                           [_token](const slot_entry& s)
                           {
                               return (s.token == _token);
                           }),
            m_slots.end());

        return;
    }

    // disconnect_all
    //   function: removes all handlers on a named channel.
    void
    disconnect_all(
        const signal_id& _channel
    )
    {
        m_slots.erase(
            std::remove_if(m_slots.begin(),
                           m_slots.end(),
                           [&_channel](const slot_entry& s)
                           {
                               return (s.channel == _channel);
                           }),
            m_slots.end());

        return;
    }

    // emit
    //   function: invokes all handlers connected to the named channel,
    // passing the type-erased event payload.
    void
    emit(
        const signal_id&   _channel,
        const compat::any& _event
    )
    {
        for (auto& slot : m_slots)
        {
            if (slot.channel == _channel)
            {
                slot.handler(_event);
            }
        }

        return;
    }

    // emit_typed
    //   function: convenience — wraps a typed event in compat::any
    // and emits on the channel.
    template<typename _Event>
    void
    emit_typed(
        const signal_id& _channel,
        const _Event&    _event
    )
    {
        emit(_channel, compat::any(_event));

        return;
    }

    // slot_count
    //   function: total number of connected slots.
    std::size_t
    slot_count() const noexcept
    {
        return m_slots.size();
    }

    // slot_count_for
    //   function: number of slots on a specific channel.
    std::size_t
    slot_count_for(
        const signal_id& _channel
    ) const noexcept
    {
        std::size_t count = 0;

        for (const auto& slot : m_slots)
        {
            if (slot.channel == _channel)
            {
                ++count;
            }
        }

        return count;
    }

    // has_channel
    //   function: true if at least one handler is connected to the
    // named channel.
    bool
    has_channel(
        const signal_id& _channel
    ) const noexcept
    {
        for (const auto& slot : m_slots)
        {
            if (slot.channel == _channel)
            {
                return true;
            }
        }

        return false;
    }

    // clear
    //   function: removes all slots on all channels.
    void
    clear()
    {
        m_slots.clear();

        return;
    }

private:
    struct slot_entry
    {
        subscription_token token;
        signal_id          channel;
        handler_fn         handler;
    };

    std::vector<slot_entry> m_slots;
    subscription_token      m_next_token = 0;
};


///////////////////////////////////////////////////////////////////////////////
///        VII.  STATIC MEDIATOR — CRTP (C++11+)                           ///
///////////////////////////////////////////////////////////////////////////////

// The static mediator uses CRTP and variadic templates to build a
// mediator whose colleague set is fixed at compile time. Zero virtual
// dispatch; all routing resolved statically.

// static_colleague
//   class: CRTP base for colleagues in a static mediator. _Derived is
// the concrete colleague; _Mediator is the static mediator type.
template<typename _Derived,
         typename _Mediator>
class static_colleague
{
public:
    static_colleague()
        : m_mediator(nullptr)
    {}

    void
    set_mediator(
        _Mediator* _med
    ) noexcept
    {
        m_mediator = _med;

        return;
    }

    _Mediator*
    mediator() const noexcept
    {
        return m_mediator;
    }

protected:
    // send
    //   function: sends an event through the static mediator.
    template<typename _Event>
    void
    send(
        const _Event& _event
    )
    {
        if (m_mediator)
        {
            m_mediator->template route<_Derived>(
                static_cast<_Derived&>(*this),
                _event);
        }

        return;
    }

private:
    _Mediator* m_mediator;
};

// static_mediator
//   class: CRTP-based compile-time mediator. _Derived implements
// the routing logic; _Colleagues is the fixed set of colleague types.
// Each colleague is stored by pointer.
//
// Usage:
//   class my_mediator
//       : public static_mediator<my_mediator, button, textbox, label>
//   {
//   public:
//       template<typename _Sender, typename _Event>
//       void route(_Sender& s, const _Event& e)
//       {
//           // custom routing logic
//           broadcast_except(s, e);
//       }
//   };
template<typename _Derived,
         typename... _Colleagues>
class static_mediator
{
public:
    static_mediator()
        : m_colleagues{}
    {}

    // register_colleague
    //   function: registers a colleague pointer at a specific
    // tuple index. Binds the colleague to this mediator.
    template<std::size_t _Index,
             typename    _Colleague>
    void
    register_colleague(
        _Colleague* _c
    )
    {
        static_assert(_Index < sizeof...(_Colleagues),
                      "Colleague index out of range.");
        std::get<_Index>(m_colleagues) = _c;
        _c->set_mediator(static_cast<_Derived*>(this));

        return;
    }

    // get_colleague
    //   function: retrieves a colleague pointer by index.
    template<std::size_t _Index>
    auto
    get_colleague() const
        -> typename std::tuple_element<_Index, std::tuple<_Colleagues*...>>::type
    {
        return std::get<_Index>(m_colleagues);
    }

protected:
    // broadcast_except
    //   function: sends an event to all colleagues except the sender.
    // Uses compile-time iteration over the tuple.
    template<typename _Sender,
             typename _Event>
    void
    broadcast_except(
        _Sender&      _sender,
        const _Event& _event
    )
    {
        broadcast_impl<0>(
            static_cast<void*>(&_sender),
            _event);

        return;
    }

private:
    // broadcast_impl — recursive (terminal case)
    template<std::size_t _I,
             typename    _Event>
    typename std::enable_if<(_I >= sizeof...(_Colleagues))>::type
    broadcast_impl(
        void*         /* _sender_addr */,
        const _Event& /* _event */
    )
    {
        return;
    }

    // broadcast_impl — recursive (active case)
    template<std::size_t _I,
             typename    _Event>
    typename std::enable_if<(_I < sizeof...(_Colleagues))>::type
    broadcast_impl(
        void*         _sender_addr,
        const _Event& _event
    )
    {
        auto* colleague = std::get<_I>(m_colleagues);

        if ( (colleague != nullptr) &&
             (static_cast<void*>(colleague) != _sender_addr) )
        {
            colleague->on_event(_event);
        }

        broadcast_impl<_I + 1>(_sender_addr, _event);

        return;
    }

    std::tuple<_Colleagues*...> m_colleagues;
};


///////////////////////////////////////////////////////////////////////////////
///          VIII. DISPATCH POLICIES                                        ///
///////////////////////////////////////////////////////////////////////////////

// Dispatch policies control how a mediator routes events. They are
// stateless structs with a single static dispatch() method that
// receives the handler list and the event.

// broadcast_policy
//   policy: delivers the event to every handler unconditionally.
struct broadcast_policy
{
    template<typename _HandlerRange,
             typename _Event>
    static void
    dispatch(
        _HandlerRange& _handlers,
        const _Event&  _event
    )
    {
        for (auto& h : _handlers)
        {
            h(_event);
        }

        return;
    }
};

// targeted_policy
//   policy: delivers the event only to a handler matching a target
// predicate. The predicate is passed alongside the event.
struct targeted_policy
{
    template<typename _HandlerRange,
             typename _Event,
             typename _Predicate>
    static void
    dispatch(
        _HandlerRange& _handlers,
        const _Event&  _event,
        _Predicate&    _pred
    )
    {
        for (auto& h : _handlers)
        {
            if (_pred(h))
            {
                h(_event);
            }
        }

        return;
    }
};

// filtered_policy
//   policy: delivers the event only if a filter predicate applied
// to the event itself returns true.
struct filtered_policy
{
    template<typename _HandlerRange,
             typename _Event,
             typename _Filter>
    static void
    dispatch(
        _HandlerRange& _handlers,
        const _Event&  _event,
        _Filter&       _filter
    )
    {
        if (_filter(_event))
        {
            for (auto& h : _handlers)
            {
                h(_event);
            }
        }

        return;
    }
};


///////////////////////////////////////////////////////////////////////////////
///         IX.   CONVENIENCE FACTORIES (C++14+)                           ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_LANG_IS_CPP14_OR_HIGHER

// make_event_bus
//   function: creates an event_bus.
inline event_bus
make_event_bus()
{
    return event_bus{};
}

// make_signal_hub
//   function: creates a signal_hub.
inline signal_hub
make_signal_hub()
{
    return signal_hub{};
}

// subscribe_many
//   function: subscribes multiple handlers at once, returning all
// tokens in a vector.
template<typename _Event,
         typename... _Fns>
inline std::vector<subscription_token>
subscribe_many(
    event_bus& _bus,
    _Fns&&...  _fns
)
{
    std::vector<subscription_token> tokens;
    tokens.reserve(sizeof...(_Fns));

    // fold via initialiser list
    using expand = int[];
    (void)expand{
        (tokens.push_back(
            _bus.template subscribe<_Event>(
                std::forward<_Fns>(_fns))), 0)...
    };

    return tokens;
}

// unsubscribe_all
//   function: unsubscribes a batch of tokens from an event bus.
inline void
unsubscribe_all(
    event_bus&                            _bus,
    const std::vector<subscription_token>& _tokens
)
{
    for (auto tok : _tokens)
    {
        _bus.unsubscribe(tok);
    }

    return;
}

#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER


///////////////////////////////////////////////////////////////////////////////
///        X.    CONCEPT-CONSTRAINED INTERFACES (C++20+)                   ///
///////////////////////////////////////////////////////////////////////////////

#if D_MEDIATOR_HAS_CONCEPTS

// mediator_for
//   concept: constrains a type that can mediate events for a set
// of colleague types. Must accept compat::any via receive() or
// notify().
template<typename _Mediator>
concept mediator_for = requires(_Mediator& _m,
                                const compat::any& _e)
{
    { _m.notify(_e) };
};

// colleague_of
//   concept: constrains a type that participates in mediation.
// Must accept events and be bindable to a mediator.
template<typename _Colleague,
         typename _Mediator>
concept colleague_of = requires(_Colleague& _c,
                                _Mediator*  _m)
{
    _c.set_mediator(_m);
    _c.on_event(std::declval<const compat::any&>());
};

// handler_for
//   concept: constrains a callable that can handle a specific event.
template<typename _Fn,
         typename _Event>
concept handler_for = std::invocable<_Fn, const _Event&>;

// constrained_subscribe
//   function: concept-constrained subscription to an event bus.
template<typename _Event,
         handler_for<_Event> _Fn>
inline subscription_token
constrained_subscribe(
    event_bus& _bus,
    _Fn        _fn
)
{
    return _bus.template subscribe<_Event>(std::move(_fn));
}

// constrained_connect
//   function: concept-constrained connection to a signal hub.
template<typename _Fn>
    requires std::invocable<_Fn, const compat::any&>
inline subscription_token
constrained_connect(
    signal_hub&      _hub,
    const signal_id& _channel,
    _Fn              _fn
)
{
    return _hub.connect(_channel, std::move(_fn));
}

#endif  // D_MEDIATOR_HAS_CONCEPTS


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


NS_END  // djinterp


#endif  // DJINTERP_PARADIGM_MEDIATOR_