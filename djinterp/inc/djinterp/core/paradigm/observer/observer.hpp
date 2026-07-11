/*******************************************************************************
* djinterp [paradigm]                                              observer.hpp
*
*   Standalone observer pattern in three cost tiers.  No UI knowledge.  No
* component coupling.  Pure pattern - attach it to whatever you like.
*
*   The user controls every byte:
*
*     ----------------------------------------------------------------------
*     | Tier        Class             sizeof         Heap    Captures      |
*     |--------------------------------------------------------------------|
*     | 0  minimal  delegate<Sig>     8  (fn_ptr)    no      no *          |
*     | 1  fixed    event<Sig,N>      8N (fn_ptrs)   no      no *          |
*     | 2  dynamic  observer<Sig>     ~24 (vector)   yes     yes           |
*     ----------------------------------------------------------------------
*     * unless _Callable is overridden to std::function or similar
*
*   All three share the same interface shape:
*     .connect(callable)    attach an observer
*     .notify(args...)      invoke all observers
*     .disconnect_all()     detach all
*     .count()              number of live observers
*     operator()            alias for notify
*
*   What differs is cost, capacity, and lifetime management:
*     - delegate:  single slot.  connect() replaces.  no connection handle.
*     - event:     N slots.  connect() returns slot_id.  no heap.
*     - observer:  unlimited.  connect() returns connection handle.  RAII.
*
*   The _Callable template parameter on each class controls what gets stored.
* Default for delegate/event is a raw function pointer (zero overhead,
* stateless lambdas decay to it).  Default for observer is std::function
* (supports captures, heap-allocates if needed).  Override freely:
*     delegate<void(int), std::function<void(int)>>   // single, with captures
*     event<void(int), 4, std::function<void(int)>>   // 4 inline, with captures
*     observer<void(int), void(*)(int)>               // dynamic, fn_ptr only
*
*
* path:      /inc/djinterp/core/paradigm/observer/observer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                        created: 2025.05.19
*******************************************************************************/

#ifndef DJINTERP_PARADIGM_OBSERVER_
#define DJINTERP_PARADIGM_OBSERVER_ 1

// std
#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../djinterp.hpp"


NS_DJINTERP


// -------------------------------------------------------------------------
//  1  SIGNATURE DECOMPOSITION
// -------------------------------------------------------------------------

// sig_decompose
//   trait: extracts return type, argument types, function-pointer type, and
// arity from a function signature of the form R(Args...).  The primary
// template is left undefined; only the R(Args...) specialisation is usable.
template<typename _Signature>
struct sig_decompose;

// sig_decompose<_Ret(_Args...)>
//   trait: specialisation for function-type signatures.
template<typename    _Ret,
         typename... _Args>
struct sig_decompose<_Ret(_Args...)>
{
    using return_type = _Ret;
    using fn_ptr_type = _Ret(*)(_Args...);

    static constexpr std::size_t arity = sizeof...(_Args);
};


// -------------------------------------------------------------------------
//  2  CONNECTION HANDLE  (for observer<> tier only)
// -------------------------------------------------------------------------
//   A connection is a lightweight handle that marks a slot as dead when
// disconnected or destroyed (via scoped_connection).  The observer owns the
// actual callable; the connection just flips a boolean.
//
//   Cost: one shared_ptr<bool> per connection (16 bytes typical).
//   delegate and event do NOT use this - they have cheaper disconnect
// mechanisms (null the pointer, index-based clear).

// connection
//   class: handle to a single observer-slot attachment.  Marking the handle
// disconnected flips a shared boolean the owning observer reads on notify.
class connection
{
public:
    // default constructor
    connection() = default;

    // adopt an existing liveness flag
    explicit connection(
        std::shared_ptr<bool> _alive
    )
        : m_alive(std::move(_alive))
    {}

    // disconnect
    //   method: marks the slot as dead.  The observer skips it on the next
    // notify and may reclaim the storage on compact().
    void
    disconnect()
    {
        if (m_alive)
        {
            *m_alive = false;
        }

        return;
    }

    // connected
    //   accessor: true if the slot is still live.
    D_NODISCARD bool
    connected() const noexcept
    {
        return (m_alive && *m_alive);
    }

    // reset
    //   method: releases the handle without disconnecting.  The slot remains
    // live but this handle can no longer control it.
    void
    reset() noexcept
    {
        m_alive.reset();

        return;
    }

private:
    std::shared_ptr<bool> m_alive;
};

// scoped_connection
//   class: RAII guard that disconnects its connection on destruction.
// Move-only.
class scoped_connection
{
public:
    // default constructor
    scoped_connection() = default;

    // adopt a connection (implicit by design)
    /*implicit*/ scoped_connection(
        connection _c
    )
        : m_conn(std::move(_c))
    {}

    // destructor
    //   disconnects the held connection.
    ~scoped_connection()
    {
        m_conn.disconnect();
    }

    // move constructor
    scoped_connection(
        scoped_connection&& _o
    ) noexcept
        : m_conn(std::move(_o.m_conn))
    {}

    // move assignment
    scoped_connection&
    operator=(scoped_connection&& _o) noexcept
    {
        if (this != &_o)
        {
            m_conn.disconnect();
            m_conn = std::move(_o.m_conn);
        }

        return *this;
    }

    // copy construction / assignment are deleted (move-only)
    scoped_connection(const scoped_connection&)            = delete;
    scoped_connection& operator=(const scoped_connection&) = delete;

    // disconnect
    //   method: disconnects the held connection early.
    void
    disconnect()
    {
        m_conn.disconnect();

        return;
    }

    // connected
    //   accessor: true if the held connection is still live.
    D_NODISCARD bool
    connected() const noexcept
    {
        return m_conn.connected();
    }

    // release
    //   method: surrenders ownership.  The slot stays live, but this guard
    // will no longer disconnect it on destruction.
    connection
    release() noexcept
    {
        connection c = std::move(m_conn);
        m_conn = connection{};

        return c;
    }

private:
    connection m_conn;
};

// scoped_connections
//   class: owns a set of connections and disconnects them all on
// destruction.
class scoped_connections
{
public:
    // destructor
    //   disconnects every held connection.
    ~scoped_connections()
    {
        disconnect_all();
    }

    // add
    //   method: takes ownership of another connection.
    void
    add(connection _c)
    {
        m_conns.push_back(std::move(_c));

        return;
    }

    // operator+=
    //   operator: shorthand for add().
    scoped_connections&
    operator+=(connection _c)
    {
        m_conns.push_back(std::move(_c));

        return *this;
    }

    // disconnect_all
    //   method: disconnects and drops every held connection.
    void
    disconnect_all()
    {
        for (auto& c : m_conns)
        {
            c.disconnect();
        }

        m_conns.clear();

        return;
    }

    // size
    //   accessor: number of connections currently held.
    D_NODISCARD std::size_t
    size() const noexcept
    {
        return m_conns.size();
    }

private:
    std::vector<connection> m_conns;
};


// -------------------------------------------------------------------------
//  3  TIER 0 - DELEGATE
// -------------------------------------------------------------------------
//   Single callable slot.  Default storage: raw function pointer.
//
//   Cost:       sizeof(_Callable)  -  typically 8 bytes for fn_ptr.
//   Heap:       never (unless _Callable itself allocates).
//   Captures:   no (with fn_ptr default).  Override _Callable for captures.
//   Lifetime:   no connection handle.  connect() replaces.  disconnect()
//               nulls.
//
//   Stateless lambdas decay to function pointers and work out of the box:
//     delegate<void(int)> d;
//     d.connect([](int x) { printf("%d\n", x); });   // fine - decays
//
//   Capturing lambdas require an explicit _Callable override:
//     delegate<void(int), std::function<void(int)>> d;
//     d.connect([&](int x) { obj.handle(x); });       // fine - std::function

// delegate
//   class: tier-0 single-slot observer.  Primary template; only the
// R(Args...) specialisation below is defined.
template<typename _Signature,
         typename _Callable = typename sig_decompose<_Signature>::fn_ptr_type>
class delegate;

// delegate<_Ret(_Args...), _Callable>
//   class: single callable slot.  connect() replaces the current callable;
// there is no connection handle.
template<typename    _Ret,
         typename... _Args,
         typename    _Callable>
class delegate<_Ret(_Args...), _Callable>
{
public:
    using signature_type = _Ret(_Args...);
    using callable_type  = _Callable;
    using return_type    = _Ret;

    static constexpr std::size_t capacity = 1;

    // default constructor
    delegate() = default;

    // wraps an existing callable (implicit by design)
    /*implicit*/ delegate(
        _Callable _fn
    )
        : m_fn(std::move(_fn))
    {}

    // connect
    //   method: replaces the current callable; any previous one is
    // discarded.
    void
    connect(_Callable _fn)
    {
        m_fn = std::move(_fn);

        return;
    }

    // disconnect
    //   method: clears the stored callable.
    void
    disconnect()
    {
        m_fn = _Callable{};

        return;
    }

    // disconnect_all
    //   method: alias for disconnect(); provided for interface uniformity.
    void
    disconnect_all()
    {
        disconnect();

        return;
    }

    // connected
    //   accessor: true if a callable is attached.
    D_NODISCARD bool
    connected() const noexcept
    {
        return static_cast<bool>(m_fn);
    }

    // count
    //   accessor: returns 0 or 1.
    D_NODISCARD std::size_t
    count() const noexcept
    {
        return (connected() ? 1 : 0);
    }

    // notify
    //   method: invokes the callable if one is attached.  For non-void
    // return types, yields a default-constructed _Ret when unattached.
    decltype(auto)
    notify(_Args... _args) const
    {
        if constexpr (std::is_void_v<_Ret>)
        {
            if (m_fn)
            {
                m_fn(_args...);
            }
        }
        else
        {
            if (m_fn)
            {
                return m_fn(_args...);
            }

            return _Ret{};
        }
    }

    // operator()
    //   operator: alias for notify().
    decltype(auto)
    operator()(_Args... _args) const
    {
        return notify(_args...);
    }

    // get
    //   accessor: direct access to the stored callable.
    _Callable&
    get() noexcept
    {
        return m_fn;
    }

    const _Callable&
    get() const noexcept
    {
        return m_fn;
    }

private:
    _Callable m_fn{};
};


// -------------------------------------------------------------------------
//  4  TIER 1 - EVENT
// -------------------------------------------------------------------------
//   Fixed-capacity inline array of callables.  No heap.  No connection
// handles.
//
//   Cost:       _Capacity * sizeof(_Callable) + sizeof(size_t).
//               e.g. event<void(int), 4> is ~40 bytes with fn_ptr.
//   Heap:       never (unless _Callable itself allocates).
//   Captures:   no (with fn_ptr default).  Override _Callable for captures.
//   Lifetime:   connect() returns a slot_id (std::size_t).  disconnect(id)
//               nulls that slot.  Slots are reused on the next connect().
//
//   Intended for scenarios where the observer count is known at compile
// time:
//     event<void(int, int), 4>  on_resize;
//     auto id = on_resize.connect(handle_resize);
//     on_resize.notify(80, 24);
//     on_resize.disconnect(id);

// event
//   class: tier-1 fixed-capacity observer.  Primary template; only the
// R(Args...) specialisation below is defined.
template<typename    _Signature,
         std::size_t _Capacity,
         typename    _Callable =
                     typename sig_decompose<_Signature>::fn_ptr_type>
class event;

// slot_id
//   type: index returned by event::connect().  Cheaper than a connection
// handle - just an integer.  A stale id after disconnection simply addresses
// a null slot, which is a safe no-op.
using slot_id = std::size_t;

// no_slot
//   constant: sentinel slot_id meaning "no free slot".
inline constexpr slot_id no_slot = static_cast<slot_id>(-1);

// event<_Ret(_Args...), _Capacity, _Callable>
//   class: fixed-capacity inline array of callables with no heap use and no
// connection handles.
template<typename    _Ret,
         typename... _Args,
         std::size_t _Capacity,
         typename    _Callable>
class event<_Ret(_Args...), _Capacity, _Callable>
{
public:
    using signature_type = _Ret(_Args...);
    using callable_type  = _Callable;
    using return_type    = _Ret;

    static constexpr std::size_t capacity = _Capacity;

    // fills every slot with a null callable
    event()
    {
        m_slots.fill(_Callable{});
    }

    // connect
    //   method: stores the callable in the first free slot.  Returns the
    // slot_id, or no_slot if every slot is occupied.
    slot_id
    connect(_Callable _fn)
    {
        // claim the first null slot
        for (std::size_t i = 0; i < _Capacity; ++i)
        {
            if (!static_cast<bool>(m_slots[i]))
            {
                m_slots[i] = std::move(_fn);

                return i;
            }
        }

        return no_slot;
    }

    // disconnect
    //   method: nulls the callable at the given slot.
    void
    disconnect(slot_id _id)
    {
        if (_id < _Capacity)
        {
            m_slots[_id] = _Callable{};
        }

        return;
    }

    // disconnect_all
    //   method: nulls every slot.
    void
    disconnect_all()
    {
        for (auto& s : m_slots)
        {
            s = _Callable{};
        }

        return;
    }

    // count
    //   accessor: number of non-null slots.
    D_NODISCARD std::size_t
    count() const noexcept
    {
        std::size_t n = 0;

        // tally every occupied slot
        for (auto& s : m_slots)
        {
            if (static_cast<bool>(s))
            {
                ++n;
            }
        }

        return n;
    }

    // full
    //   accessor: true if every slot is occupied.
    D_NODISCARD bool
    full() const noexcept
    {
        return (count() == _Capacity);
    }

    // empty
    //   accessor: true if no slot is occupied.
    D_NODISCARD bool
    empty() const noexcept
    {
        return (count() == 0);
    }

    // slot_connected
    //   accessor: true if the given slot is live.
    D_NODISCARD bool
    slot_connected(slot_id _id) const noexcept
    {
        return ( (_id < _Capacity) &&
                 static_cast<bool>(m_slots[_id]) );
    }

    // notify
    //   method: invokes all non-null slots in order 0 -> _Capacity - 1.
    void
    notify(_Args... _args) const
    {
        // fire each occupied slot in index order
        for (auto& s : m_slots)
        {
            if (static_cast<bool>(s))
            {
                s(_args...);
            }
        }

        return;
    }

    // operator()
    //   operator: alias for notify().
    void
    operator()(_Args... _args) const
    {
        notify(_args...);

        return;
    }

    // at
    //   accessor: direct access to the callable at a given slot.
    _Callable&
    at(slot_id _id)
    {
        return m_slots[_id];
    }

    const _Callable&
    at(slot_id _id) const
    {
        return m_slots[_id];
    }

    // compact
    //   method: defragments by moving all live slots to the front.
    // Invalidates previously returned slot_ids - call only when every id
    // has been discarded.
    void
    compact()
    {
        std::size_t write = 0;

        // slide each live slot down to the first free index
        for (std::size_t read = 0; read < _Capacity; ++read)
        {
            if (static_cast<bool>(m_slots[read]))
            {
                if (write != read)
                {
                    m_slots[write] = std::move(m_slots[read]);
                    m_slots[read]  = _Callable{};
                }

                ++write;
            }
        }

        return;
    }

private:
    std::array<_Callable, _Capacity> m_slots;
};


// -------------------------------------------------------------------------
//  5  TIER 2 - OBSERVER
// -------------------------------------------------------------------------
//   Dynamic, connection-tracked, unlimited capacity.
//
//   Cost:       ~24 bytes base (std::vector) + per-slot:
//                 sizeof(_Callable) + sizeof(shared_ptr<bool>)
//               Typical: ~40 bytes per slot with std::function.
//   Heap:       yes - both the vector and connection tracking allocate.
//   Captures:   yes (std::function default).
//   Lifetime:   connect() returns a connection handle.  The handle can be
//               stored in a scoped_connection for RAII, or disconnected
//               manually.  Destroying the observer invalidates all handles
//               (they become no-ops, not dangling).
//
//   This is the "batteries included" tier.  Use it when:
//     - You don't know how many observers you'll have
//     - You need capturing lambdas
//     - You want RAII lifetime management
//
//   Use delegate or event when you need tighter control.

// observer
//   class: tier-2 dynamic, connection-tracked observer.  Primary template;
// only the R(Args...) specialisation below is defined.
template<typename _Signature,
         typename _Callable = std::function<_Signature>>
class observer;

// observer<_Ret(_Args...), _Callable>
//   class: unlimited-capacity observer.  connect() returns a connection
// handle for RAII lifetime management; destroying the observer turns all
// outstanding handles into no-ops rather than dangling references.
template<typename    _Ret,
         typename... _Args,
         typename    _Callable>
class observer<_Ret(_Args...), _Callable>
{
public:
    using signature_type = _Ret(_Args...);
    using callable_type  = _Callable;
    using return_type    = _Ret;

    static constexpr std::size_t capacity = 0;   // unbounded

    // connect
    //   method: appends a callable and returns a connection handle for
    // lifetime management.
    connection
    connect(_Callable _fn)
    {
        auto alive = std::make_shared<bool>(true);
        m_slots.push_back({ std::move(_fn), alive });

        return connection(std::move(alive));
    }

    // operator+=
    //   operator: shorthand for connect().
    connection
    operator+=(_Callable _fn)
    {
        return connect(std::move(_fn));
    }

    // disconnect_all
    //   method: marks every slot dead and clears the backing vector.
    void
    disconnect_all()
    {
        for (auto& s : m_slots)
        {
            *s.alive = false;
        }

        m_slots.clear();

        return;
    }

    // count
    //   accessor: number of live (connected) slots.
    D_NODISCARD std::size_t
    count() const noexcept
    {
        std::size_t n = 0;

        // count only the slots still marked live
        for (auto& s : m_slots)
        {
            if (*s.alive)
            {
                ++n;
            }
        }

        return n;
    }

    // empty
    //   accessor: true if no slot is live.
    D_NODISCARD bool
    empty() const noexcept
    {
        return (count() == 0);
    }

    // size_including_dead
    //   accessor: total vector size, including dead-but-not-compacted slots.
    D_NODISCARD std::size_t
    size_including_dead() const noexcept
    {
        return m_slots.size();
    }

    // notify
    //   method: invokes every live slot; dead slots are skipped.
    void
    notify(_Args... _args) const
    {
        // fire each slot that is still marked live
        for (auto& s : m_slots)
        {
            if (*s.alive)
            {
                s.fn(_args...);
            }
        }

        return;
    }

    // operator()
    //   operator: alias for notify().
    void
    operator()(_Args... _args) const
    {
        notify(_args...);

        return;
    }

    // compact
    //   method: erases dead slots, freeing their memory.  Live connection
    // handles remain valid.  Call periodically when churn is high.
    void
    compact()
    {
        // drop every dead slot, preserving the order of the live ones
        m_slots.erase(
            std::remove_if(m_slots.begin(),
                           m_slots.end(),
                           [](const slot_entry& _s) { return !*_s.alive; }),
            m_slots.end());

        return;
    }

    // reserve
    //   method: pre-allocates backing-vector capacity.
    void
    reserve(std::size_t _n)
    {
        m_slots.reserve(_n);

        return;
    }

private:
    // slot_entry
    //   struct: one stored callable paired with its liveness flag.
    struct slot_entry
    {
        _Callable             fn;
        std::shared_ptr<bool> alive;
    };

    mutable std::vector<slot_entry> m_slots;
};


// -------------------------------------------------------------------------
//  6  OBSERVER TRAITS  (SFINAE detection)
// -------------------------------------------------------------------------
//   Mirrors the menu_traits pattern in menu.hpp: the internal namespace
// holds fine-grained detectors, top-level traits compose them.
//
//   These let generic code discover observer capabilities at compile time
// without coupling to concrete types:
//
//     template<typename T>
//     void maybe_attach(T& obs) {
//         if constexpr (observer_traits::is_observable_v<T>) {
//             obs.connect(my_handler);
//         }
//     }

namespace observer_traits
{
NS_INTERNAL

    // -- method detectors -------------------------------------------------

    // has_connect
    //   trait: true if _Type exposes a .connect(callable_type) method.
    template<typename _Type,
             typename = void>
    struct has_connect : std::false_type
    {};

    template<typename _Type>
    struct has_connect<_Type, std::void_t<
        decltype(std::declval<_Type>().connect(
            std::declval<typename _Type::callable_type>()))
    >> : std::true_type
    {};

    // has_notify
    //   trait: true if _Type exposes a .notify(...) method.  Detected via
    // the address of the member, since notify's argument types vary.
    template<typename _Type,
             typename = void>
    struct has_notify : std::false_type
    {};

    template<typename _Type>
    struct has_notify<_Type, std::void_t<
        decltype(&_Type::notify)
    >> : std::true_type
    {};

    // has_disconnect_all
    //   trait: true if _Type exposes a .disconnect_all() method.
    template<typename _Type,
             typename = void>
    struct has_disconnect_all : std::false_type
    {};

    template<typename _Type>
    struct has_disconnect_all<_Type, std::void_t<
        decltype(std::declval<_Type>().disconnect_all())
    >> : std::true_type
    {};

    // has_count
    //   trait: true if _Type exposes a .count() method.
    template<typename _Type,
             typename = void>
    struct has_count : std::false_type
    {};

    template<typename _Type>
    struct has_count<_Type, std::void_t<
        decltype(std::declval<_Type>().count())
    >> : std::true_type
    {};

    // has_compact
    //   trait: true if _Type exposes a .compact() method.
    template<typename _Type,
             typename = void>
    struct has_compact : std::false_type
    {};

    template<typename _Type>
    struct has_compact<_Type, std::void_t<
        decltype(std::declval<_Type>().compact())
    >> : std::true_type
    {};

    // -- type-alias detectors ---------------------------------------------

    // has_signature_type
    //   trait: true if _Type defines a nested signature_type.
    template<typename _Type,
             typename = void>
    struct has_signature_type : std::false_type
    {};

    template<typename _Type>
    struct has_signature_type<_Type, std::void_t<
        typename _Type::signature_type
    >> : std::true_type
    {};

    // has_callable_type
    //   trait: true if _Type defines a nested callable_type.
    template<typename _Type,
             typename = void>
    struct has_callable_type : std::false_type
    {};

    template<typename _Type>
    struct has_callable_type<_Type, std::void_t<
        typename _Type::callable_type
    >> : std::true_type
    {};

    // has_return_type
    //   trait: true if _Type defines a nested return_type.
    template<typename _Type,
             typename = void>
    struct has_return_type : std::false_type
    {};

    template<typename _Type>
    struct has_return_type<_Type, std::void_t<
        typename _Type::return_type
    >> : std::true_type
    {};

    // -- capacity detector ------------------------------------------------

    // has_static_capacity
    //   trait: true if _Type::capacity is a valid static constexpr member.
    template<typename _Type,
             typename = void>
    struct has_static_capacity : std::false_type
    {};

    template<typename _Type>
    struct has_static_capacity<_Type, std::void_t<
        decltype(_Type::capacity)
    >> : std::true_type
    {};

    // -- disconnect-style detectors ---------------------------------------

    // has_disconnect_void
    //   trait: true if _Type has .disconnect() with no arguments
    // (delegate-style).
    template<typename _Type,
             typename = void>
    struct has_disconnect_void : std::false_type
    {};

    template<typename _Type>
    struct has_disconnect_void<_Type, std::void_t<
        decltype(std::declval<_Type>().disconnect())
    >> : std::true_type
    {};

    // has_disconnect_by_id
    //   trait: true if _Type has .disconnect(slot_id) (event-style).
    template<typename _Type,
             typename = void>
    struct has_disconnect_by_id : std::false_type
    {};

    template<typename _Type>
    struct has_disconnect_by_id<_Type, std::void_t<
        decltype(std::declval<_Type>().disconnect(std::declval<slot_id>()))
    >> : std::true_type
    {};

    // connect_returns_connection
    //   trait: true if _Type.connect(callable) returns a connection
    // (observer-style).
    template<typename _Type,
             typename = void>
    struct connect_returns_connection : std::false_type
    {};

    template<typename _Type>
    struct connect_returns_connection<_Type, std::enable_if_t<
        std::is_same_v<
            decltype(std::declval<_Type>().connect(
                std::declval<typename _Type::callable_type>())),
            connection>
    >> : std::true_type
    {};

    // connect_returns_slot_id
    //   trait: true if _Type.connect(callable) returns a slot_id
    // (event-style).
    template<typename _Type,
             typename = void>
    struct connect_returns_slot_id : std::false_type
    {};

    template<typename _Type>
    struct connect_returns_slot_id<_Type, std::enable_if_t<
        std::is_same_v<
            decltype(std::declval<_Type>().connect(
                std::declval<typename _Type::callable_type>())),
            slot_id>
    >> : std::true_type
    {};

    // connect_returns_void
    //   trait: true if _Type.connect(callable) returns void
    // (delegate-style).
    template<typename _Type,
             typename = void>
    struct connect_returns_void : std::false_type
    {};

    template<typename _Type>
    struct connect_returns_void<_Type, std::enable_if_t<
        std::is_void_v<
            decltype(std::declval<_Type>().connect(
                std::declval<typename _Type::callable_type>()))>
    >> : std::true_type
    {};

NS_END  // internal


    // -- convenience `_v` aliases -----------------------------------------
    //   each mirrors the matching internal detector as an inline variable.

    template<typename _Type>
    inline constexpr bool has_connect_v =
        internal::has_connect<_Type>::value;

    template<typename _Type>
    inline constexpr bool has_notify_v =
        internal::has_notify<_Type>::value;

    template<typename _Type>
    inline constexpr bool has_disconnect_all_v =
        internal::has_disconnect_all<_Type>::value;

    template<typename _Type>
    inline constexpr bool has_count_v =
        internal::has_count<_Type>::value;

    template<typename _Type>
    inline constexpr bool has_compact_v =
        internal::has_compact<_Type>::value;

    template<typename _Type>
    inline constexpr bool has_signature_type_v =
        internal::has_signature_type<_Type>::value;

    template<typename _Type>
    inline constexpr bool has_callable_type_v =
        internal::has_callable_type<_Type>::value;

    template<typename _Type>
    inline constexpr bool has_return_type_v =
        internal::has_return_type<_Type>::value;

    template<typename _Type>
    inline constexpr bool has_static_capacity_v =
        internal::has_static_capacity<_Type>::value;

    template<typename _Type>
    inline constexpr bool has_disconnect_void_v =
        internal::has_disconnect_void<_Type>::value;

    template<typename _Type>
    inline constexpr bool has_disconnect_by_id_v =
        internal::has_disconnect_by_id<_Type>::value;

    template<typename _Type>
    inline constexpr bool connect_returns_connection_v =
        internal::connect_returns_connection<_Type>::value;

    template<typename _Type>
    inline constexpr bool connect_returns_slot_id_v =
        internal::connect_returns_slot_id<_Type>::value;

    template<typename _Type>
    inline constexpr bool connect_returns_void_v =
        internal::connect_returns_void<_Type>::value;


    // -- composite identity traits ----------------------------------------

    // is_observable
    //   trait: minimum observable surface - has connect, notify, and count.
    template<typename _Type>
    struct is_observable : std::conjunction<
        internal::has_connect<_Type>,
        internal::has_notify<_Type>,
        internal::has_count<_Type>>
    {};

    template<typename _Type>
    inline constexpr bool is_observable_v =
        is_observable<_Type>::value;

    // is_delegate
    //   trait: single-slot observable - capacity == 1, connect returns void.
    template<typename _Type>
    struct is_delegate : std::conjunction<
        is_observable<_Type>,
        internal::has_disconnect_void<_Type>,
        internal::connect_returns_void<_Type>>
    {};

    template<typename _Type>
    inline constexpr bool is_delegate_v =
        is_delegate<_Type>::value;

    // is_event
    //   trait: fixed-capacity observable - has static capacity, connect
    // returns a slot_id.
    template<typename _Type>
    struct is_event : std::conjunction<
        is_observable<_Type>,
        internal::has_static_capacity<_Type>,
        internal::connect_returns_slot_id<_Type>>
    {};

    template<typename _Type>
    inline constexpr bool is_event_v =
        is_event<_Type>::value;

    // is_observer
    //   trait: dynamic observable - connect returns a connection, has
    // compact.
    template<typename _Type>
    struct is_observer : std::conjunction<
        is_observable<_Type>,
        internal::connect_returns_connection<_Type>,
        internal::has_compact<_Type>>
    {};

    template<typename _Type>
    inline constexpr bool is_observer_v =
        is_observer<_Type>::value;

    // has_connection_tracking
    //   trait: true if connect returns a connection handle (observer only).
    template<typename _Type>
    struct has_connection_tracking
        : internal::connect_returns_connection<_Type>
    {};

    template<typename _Type>
    inline constexpr bool has_connection_tracking_v =
        has_connection_tracking<_Type>::value;

    // is_bounded
    //   trait: true if _Type has a non-zero static capacity (delegate or
    // event, not observer).
    template<typename _Type,
             typename = void>
    struct is_bounded : std::false_type
    {};

    template<typename _Type>
    struct is_bounded<_Type, std::enable_if_t<
        ( internal::has_static_capacity<_Type>::value &&
          (_Type::capacity > 0) )
    >> : std::true_type
    {};

    template<typename _Type>
    inline constexpr bool is_bounded_v =
        is_bounded<_Type>::value;

    // observable_arity
    //   trait: extracts the arity (argument count) from an observable's
    // signature_type; 0 when there is no signature_type.
    template<typename _Type,
             typename = void>
    struct observable_arity
    {
        static constexpr std::size_t value = 0;
    };

    template<typename _Type>
    struct observable_arity<_Type, std::enable_if_t<
        internal::has_signature_type<_Type>::value
    >>
    {
        static constexpr std::size_t value =
            sig_decompose<typename _Type::signature_type>::arity;
    };

    template<typename _Type>
    inline constexpr std::size_t observable_arity_v =
        observable_arity<_Type>::value;

}   // namespace observer_traits


// -------------------------------------------------------------------------
//  7  CONVENIENCE ALIASES
// -------------------------------------------------------------------------

// observer_ptr
//   type: alias for delegate<Sig> - one fn_ptr, 8 bytes, zero overhead.
// Named to communicate intent: "a single observation point."
template<typename _Signature>
using observer_ptr = delegate<_Signature>;


NS_END  // djinterp


#endif  // DJINTERP_PARADIGM_OBSERVER_
