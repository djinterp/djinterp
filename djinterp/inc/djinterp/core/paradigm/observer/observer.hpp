/*******************************************************************************
* djinterp [core]                                                   observer.hpp
*
*   Standalone observer pattern in three cost tiers.  No UI knowledge.  No
* component coupling.  Pure pattern - attach it to whatever you like.
*
*   The user controls every byte:
*
*     ┌────────────────────────────────────────────────────────────────────┐
*     │ Tier        Class             sizeof         Heap    Captures     │
*     ├────────────────────────────────────────────────────────────────────┤
*     │ 0  minimal  delegate<Sig>     8  (fn_ptr)    no      no *         │
*     │ 1  fixed    event<Sig,N>      8N (fn_ptrs)   no      no *         │
*     │ 2  dynamic  observer<Sig>     ~24 (vector)   yes     yes          │
*     └────────────────────────────────────────────────────────────────────┘
*     * unless Callable is overridden to std::function or similar
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
*   The Callable template parameter on each class controls what gets stored.
* Default for delegate/event is a raw function pointer (zero overhead,
* stateless lambdas decay to it).  Default for observer is std::function
* (supports captures, heap-allocates if needed).  Override freely:
*
*     delegate<void(int), std::function<void(int)>>   // single, with captures
*     event<void(int), 4, std::function<void(int)>>   // 4 inline, with captures
*     observer<void(int), void(*)(int)>               // dynamic, fn_ptr only
*
*
* path:      /inc/djinterp/core/paradigm/observer/observer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.05.19
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef  DJINTERP_PARADIGM_OBSERVER_OBSERVER_HPP
#define  DJINTERP_PARADIGM_OBSERVER_OBSERVER_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <vector>
// djinterp
#include "../../../djinterp.hpp"
#include "../../meta/type_traits.hpp"  // conjunction
#include "../../meta/type_utility.hpp"  // void_t


NS_DJINTERP


// ═══════════════════════════════════════════════════════════════════════════════
//  §1  SIGNATURE DECOMPOSITION
// ═══════════════════════════════════════════════════════════════════════════════
//   Extracts return type, argument types, function pointer type, and arity
// from a function signature like R(Args...).

template<typename>
struct sig_decompose;

template<typename Ret,
          typename... Args>
struct sig_decompose<Ret(Args...)>
{
    using return_type  = Ret;
    using fn_ptr_type  = Ret(*)(Args...);

    static constexpr std::size_t arity = sizeof...(Args);
};




// ═══════════════════════════════════════════════════════════════════════════════
//  §2  CONNECTION HANDLE  (for observer<> tier only)
// ═══════════════════════════════════════════════════════════════════════════════
//   A connection is a lightweight handle that marks a slot as dead when
// disconnected or destroyed (via scoped_connection).  The observer owns the
// actual callable; the connection just flips a boolean.
//
//   Cost: one shared_ptr<bool> per connection (16 bytes typical).
//   delegate and event do NOT use this - they have cheaper disconnect
// mechanisms (null the pointer, index-based clear).

// connection
//   Handle to a single observer-slot attachment.
class connection
{
public:
    connection() = default;

    explicit connection(std::shared_ptr<bool> alive)
        : alive_(std::move(alive))
    {}

    // disconnect
    //   Marks the slot as dead.  The observer will skip it on next notify
    // and may reclaim the storage on compact().
    void disconnect()
    {
        if (alive_) *alive_ = false;
    }

    // connected
    //   True if the slot is still live.
    D_NODISCARD bool connected() const noexcept
    {
        return alive_ && *alive_;
    }

    // reset
    //   Release the handle without disconnecting.  The slot remains live
    // but this handle can no longer control it.
    void reset() noexcept { alive_.reset(); }

private:
    std::shared_ptr<bool> alive_;
};

// scoped_connection
//   RAII wrapper.  Disconnects on destruction.  Move-only.
class scoped_connection
{
public:
    scoped_connection() = default;

    /*implicit*/ scoped_connection(connection c)
        : conn_(std::move(c))
    {}

    ~scoped_connection() { conn_.disconnect(); }

    // move
    scoped_connection(scoped_connection&& o) noexcept
        : conn_(std::move(o.conn_))
    {}

    scoped_connection& operator=(scoped_connection&& o) noexcept
    {
        if (this != &o) {
            conn_.disconnect();
            conn_ = std::move(o.conn_);
        }
        return *this;
    }

    // no copy
    scoped_connection(const scoped_connection&) = delete;
    scoped_connection& operator=(const scoped_connection&) = delete;

    void disconnect() { conn_.disconnect(); }
    D_NODISCARD bool connected() const noexcept { return conn_.connected(); }

    // release
    //   Surrender ownership - the slot remains live, but this guard
    // will no longer disconnect it on destruction.
    connection release() noexcept
    {
        connection c = std::move(conn_);
        conn_ = connection{};
        return c;
    }

private:
    connection conn_;
};

// scoped_connections
//   Holds multiple connections; disconnects all on destruction.
class scoped_connections
{
public:
    ~scoped_connections() { disconnect_all(); }

    void add(connection c) { conns_.push_back(std::move(c)); }

    scoped_connections& operator+=(connection c)
    {
        conns_.push_back(std::move(c));
        return *this;
    }

    void disconnect_all()
    {
        for (auto& c : conns_) c.disconnect();
        conns_.clear();
    }

    D_NODISCARD std::size_t size() const noexcept { return conns_.size(); }

private:
    std::vector<connection> conns_;
};


// ═══════════════════════════════════════════════════════════════════════════════
//  §3  TIER 0 - DELEGATE
// ═══════════════════════════════════════════════════════════════════════════════
//   Single callable slot.  Default storage: raw function pointer.
//
//   Cost:       sizeof(Callable)  -  typically 8 bytes for fn_ptr.
//   Heap:       never (unless Callable itself allocates).
//   Captures:   no (with fn_ptr default).  Override Callable for captures.
//   Lifetime:   no connection handle.  connect() replaces.  disconnect() nulls.
//
//   Stateless lambdas decay to function pointers and work out of the box:
//     delegate<void(int)> d;
//     d.connect([](int x) { printf("%d\n", x); });   // fine - decays to fn_ptr
//
//   Capturing lambdas require explicit Callable override:
//     delegate<void(int), std::function<void(int)>> d;
//     d.connect([&](int x) { obj.handle(x); });      // fine - std::function

// primary template (unspecialised)
template<typename Signature,
          typename Callable = typename sig_decompose<Signature>::fn_ptr_type>
class delegate;

// partial specialisation that decomposes R(Args...)
template<typename Ret, typename... Args, typename Callable>
class delegate<Ret(Args...), Callable>
{
public:
    // ── type aliases ─────────────────────────────────────────────────────
    using signature_type = Ret(Args...);
    using callable_type  = Callable;
    using return_type    = Ret;

    static constexpr std::size_t capacity = 1;

    // ── construction ─────────────────────────────────────────────────────
    delegate() = default;
    /*implicit*/ delegate(Callable fn) : fn_(std::move(fn)) {}

    // ── connect / disconnect ─────────────────────────────────────────────

    // connect
    //   Replaces the current callable.  Any previous callable is discarded.
    void connect(Callable fn) { fn_ = std::move(fn); }

    // disconnect
    //   Clears the callable.
    void disconnect() { fn_ = Callable{}; }

    // disconnect_all
    //   Same as disconnect() - provided for interface uniformity.
    void disconnect_all() { disconnect(); }

    // ── query ────────────────────────────────────────────────────────────

    // connected
    //   True if a callable is attached.
    D_NODISCARD bool connected() const noexcept
    {
        return static_cast<bool>(fn_);
    }

    // count
    //   Returns 0 or 1.
    D_NODISCARD std::size_t count() const noexcept
    {
        return connected() ? 1 : 0;
    }

    // ── notify ───────────────────────────────────────────────────────────

    // notify
    //   Invokes the callable if connected.
    //   For non-void return types: returns R{} if not connected.
    Ret notify(Args... args) const
    {
        return notify_dispatch(std::is_void<Ret>(), args...);
    }

    // notify_dispatch
    //   The two halves of notify(), chosen by whether Ret is void (C++11
    //   has neither decltype(auto) nor if constexpr). Only the half that
    //   matches Ret is ever instantiated.
    void notify_dispatch(std::true_type, Args... args) const
    {
        if (fn_) fn_(args...);
    }

    Ret notify_dispatch(std::false_type, Args... args) const
    {
        if (fn_) return fn_(args...);
        return Ret{};
    }

    // operator()
    //   Alias for notify().
    Ret operator()(Args... args) const { return notify(args...); }

    // ── access ───────────────────────────────────────────────────────────

    // get
    //   Direct access to the stored callable.
    Callable&       get()       noexcept { return fn_; }
    const Callable& get() const noexcept { return fn_; }

private:
    Callable fn_{};
};


// ═══════════════════════════════════════════════════════════════════════════════
//  §4  TIER 1 - EVENT
// ═══════════════════════════════════════════════════════════════════════════════
//   Fixed-capacity inline array of callables.  No heap.  No connection handles.
//
//   Cost:       Capacity × sizeof(Callable) + sizeof(size_t).
//               e.g. event<void(int), 4> ≈ 40 bytes with fn_ptr.
//   Heap:       never (unless Callable itself allocates).
//   Captures:   no (with fn_ptr default).  Override Callable for captures.
//   Lifetime:   connect() returns a slot_id (std::size_t).  disconnect(id)
//               nulls that slot.  Slots are reused on next connect().
//
//   Intended for scenarios where the observer count is known at compile time:
//     event<void(int, int), 4>  on_resize;
//     auto id = on_resize.connect(handle_resize);
//     on_resize.notify(80, 24);
//     on_resize.disconnect(id);

// primary template (unspecialised)
template<typename Signature,
          std::size_t Capacity,
          typename Callable = typename sig_decompose<Signature>::fn_ptr_type>
class event;

// slot_id
//   Index returned by event::connect().  Cheaper than a connection handle -
// just an integer.  The user is responsible for not using a stale id after
// disconnection (it will simply address a null slot, which is a safe no-op).
using slot_id = std::size_t;

// sentinel for "no free slot"
D_CONSTEXPR_INLINE_VAR slot_id no_slot = static_cast<slot_id>(-1);

// partial specialisation
template<typename Ret, typename... Args, std::size_t Capacity, typename Callable>
class event<Ret(Args...), Capacity, Callable>
{
public:
    // ── type aliases ─────────────────────────────────────────────────────
    using signature_type = Ret(Args...);
    using callable_type  = Callable;
    using return_type    = Ret;

    static constexpr std::size_t capacity = Capacity;

    // ── construction ─────────────────────────────────────────────────────
    event() { slots_.fill(Callable{}); }

    // ── connect / disconnect ─────────────────────────────────────────────

    // connect
    //   Stores the callable in the first available (null) slot.
    //   Returns the slot_id, or no_slot if full.
    slot_id connect(Callable fn)
    {
        for (std::size_t i = 0; i < Capacity; ++i) {
            if (!static_cast<bool>(slots_[i])) {
                slots_[i] = std::move(fn);
                return i;
            }
        }
        return no_slot;
    }

    // disconnect
    //   Nulls the callable at the given slot.
    void disconnect(slot_id id)
    {
        if (id < Capacity) slots_[id] = Callable{};
    }

    // disconnect_all
    //   Nulls every slot.
    void disconnect_all()
    {
        for (auto& s : slots_) s = Callable{};
    }

    // ── query ────────────────────────────────────────────────────────────

    // count
    //   Number of non-null slots.
    D_NODISCARD std::size_t count() const noexcept
    {
        std::size_t n = 0;
        for (auto& s : slots_)
            if (static_cast<bool>(s)) ++n;
        return n;
    }

    // full
    //   True if every slot is occupied.
    D_NODISCARD bool full() const noexcept { return count() == Capacity; }

    // empty
    //   True if no slot is occupied.
    D_NODISCARD bool empty() const noexcept { return count() == 0; }

    // slot_connected
    //   True if the given slot is live.
    D_NODISCARD bool slot_connected(slot_id id) const noexcept
    {
        return id < Capacity && static_cast<bool>(slots_[id]);
    }

    // ── notify ───────────────────────────────────────────────────────────

    // notify
    //   Invokes all non-null slots.  Order: 0 -> Capacity-1.
    void notify(Args... args) const
    {
        for (auto& s : slots_)
            if (static_cast<bool>(s)) s(args...);
    }

    // operator()
    void operator()(Args... args) const { notify(args...); }

    // ── access ───────────────────────────────────────────────────────────

    // at
    //   Direct access to the callable at a given slot.
    Callable&       at(slot_id id)       { return slots_[id]; }
    const Callable& at(slot_id id) const { return slots_[id]; }

    // compact
    //   Defragments: moves all live slots to the front.  Invalidates
    // previously returned slot_ids - call only when you discard all ids.
    void compact()
    {
        std::size_t write = 0;
        for (std::size_t read = 0; read < Capacity; ++read) {
            if (static_cast<bool>(slots_[read])) {
                if (write != read) {
                    slots_[write] = std::move(slots_[read]);
                    slots_[read] = Callable{};
                }
                ++write;
            }
        }
    }

private:
    std::array<Callable, Capacity> slots_;
};




// ═══════════════════════════════════════════════════════════════════════════════
//  §5  TIER 2 - OBSERVER
// ═══════════════════════════════════════════════════════════════════════════════
//   Dynamic, connection-tracked, unlimited capacity.
//
//   Cost:       ~24 bytes base (std::vector) + per-slot:
//                 sizeof(Callable) + sizeof(shared_ptr<bool>)
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

// primary template
template<typename Signature,
          typename Callable = std::function<Signature>>
class observer;

// partial specialisation
template<typename Ret, typename... Args, typename Callable>
class observer<Ret(Args...), Callable>
{
public:
    // ── type aliases ─────────────────────────────────────────────────────
    using signature_type = Ret(Args...);
    using callable_type  = Callable;
    using return_type    = Ret;

    static constexpr std::size_t capacity = 0;  // unbounded

    // ── connect / disconnect ─────────────────────────────────────────────

    // connect
    //   Appends a callable.  Returns a connection handle for lifetime
    // management.
    connection connect(Callable fn)
    {
        auto alive = std::make_shared<bool>(true);
        slots_.push_back({ std::move(fn), alive });
        return connection(std::move(alive));
    }

    // operator+=
    //   Shorthand for connect().
    connection operator+=(Callable fn) { return connect(std::move(fn)); }

    // disconnect_all
    //   Marks every slot as dead and clears the vector.
    void disconnect_all()
    {
        for (auto& s : slots_) *s.alive = false;
        slots_.clear();
    }

    // ── query ────────────────────────────────────────────────────────────

    // count
    //   Number of live (connected) slots.
    D_NODISCARD std::size_t count() const noexcept
    {
        std::size_t n = 0;
        for (auto& s : slots_)
            if (*s.alive) ++n;
        return n;
    }

    // empty
    D_NODISCARD bool empty() const noexcept { return count() == 0; }

    // size_including_dead
    //   Total vector size, including dead-but-not-yet-compacted slots.
    D_NODISCARD std::size_t size_including_dead() const noexcept
    {
        return slots_.size();
    }

    // ── notify ───────────────────────────────────────────────────────────

    // notify
    //   Invokes all live slots.  Dead slots are skipped.
    void notify(Args... args) const
    {
        for (auto& s : slots_)
            if (*s.alive) s.fn(args...);
    }

    // operator()
    void operator()(Args... args) const { notify(args...); }

    // ── maintenance ──────────────────────────────────────────────────────

    // compact
    //   Erases dead slots, freeing their memory.  Does not invalidate
    // live connection handles.  Call periodically if churn is high.
    void compact()
    {
        slots_.erase(
            std::remove_if(slots_.begin(), slots_.end(),
                [](const slot_entry& s) { return !*s.alive; }),
            slots_.end()
        );
    }

    // reserve
    //   Pre-allocate vector capacity.
    void reserve(std::size_t n) { slots_.reserve(n); }

private:
    struct slot_entry
    {
        Callable               fn;
        std::shared_ptr<bool>  alive;
    };

    mutable std::vector<slot_entry> slots_;
};




// ═══════════════════════════════════════════════════════════════════════════════
//  §6  OBSERVER TRAITS  (SFINAE detection)
// ═══════════════════════════════════════════════════════════════════════════════
//   Mirrors the menu_traits pattern in menu.hpp: detail namespace holds
// fine-grained detectors, top-level traits compose them.
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
    // ── method detectors ─────────────────────────────────────────────────

    // has_connect
    //   type trait: T has a .connect(...) method
    template<typename,
              typename = void>
    struct has_connect : std::false_type
    {};

    template<typename Type>
    struct has_connect<Type, void_t<
        decltype(std::declval<Type>().connect(std::declval<typename Type::callable_type>()))
    >> : std::true_type {};

    // has_notify
    //   type trait: T has a .notify(...) method
    //   (we check for the presence of operator() as proxy, since notify's
    //    argument types vary)
    template<typename,
              typename = void>
    struct has_notify : std::false_type
    {};

    template<typename Type>
    struct has_notify<Type, void_t<
        decltype(&Type::notify)
    >> : std::true_type {};

    // has_disconnect_all
    template<typename,
              typename = void>
    struct has_disconnect_all : std::false_type
    {};

    template<typename Type>
    struct has_disconnect_all<Type, void_t<
        decltype(std::declval<Type>().disconnect_all())
    >> : std::true_type {};

    // has_count
    template<typename,
              typename = void>
    struct has_count : std::false_type
    {};

    template<typename Type>
    struct has_count<Type, void_t<
        decltype(std::declval<Type>().count())
    >> : std::true_type {};

    // has_compact
    template<typename,
              typename = void>
    struct has_compact : std::false_type
    {};

    template<typename Type>
    struct has_compact<Type, void_t<
        decltype(std::declval<Type>().compact())
    >> : std::true_type {};

    /***********************************************************************/

    // ── type alias detectors ─────────────────────────────────────────────

    // has_signature_type
    template<typename,
              typename = void>
    struct has_signature_type : std::false_type
    {};

    template<typename Type>
    struct has_signature_type<Type, void_t<
        typename Type::signature_type
    >> : std::true_type {};

    // has_callable_type
    template<typename,
              typename = void>
    struct has_callable_type : std::false_type
    {};

    template<typename Type>
    struct has_callable_type<Type, void_t<
        typename Type::callable_type
    >> : std::true_type {};

    // has_return_type
    template<typename,
              typename = void>
    struct has_return_type : std::false_type
    {};

    template<typename Type>
    struct has_return_type<Type, void_t<
        typename Type::return_type
    >> : std::true_type {};

    /***********************************************************************/

    // ── capacity detector ────────────────────────────────────────────────

    // has_static_capacity
    //   True if T::capacity is a valid static constexpr member.
    template<typename,
              typename = void>
    struct has_static_capacity : std::false_type
    {};

    template<typename Type>
    struct has_static_capacity<Type, void_t<
        decltype(Type::capacity)
    >> : std::true_type {};

    /***********************************************************************/

    // ── disconnect style detectors ───────────────────────────────────────

    // has_disconnect_void
    //   T has .disconnect() with no arguments (delegate-style).
    template<typename,
              typename = void>
    struct has_disconnect_void : std::false_type
    {};

    template<typename Type>
    struct has_disconnect_void<Type, void_t<
        decltype(std::declval<Type>().disconnect())
    >> : std::true_type {};

    // has_disconnect_by_id
    //   T has .disconnect(slot_id) (event-style).
    template<typename,
              typename = void>
    struct has_disconnect_by_id : std::false_type
    {};

    template<typename Type>
    struct has_disconnect_by_id<Type, void_t<
        decltype(std::declval<Type>().disconnect(std::declval<slot_id>()))
    >> : std::true_type {};

    // connect_returns_connection
    //   T.connect(callable) returns a connection (observer-style).
    template<typename,
              typename = void>
    struct connect_returns_connection : std::false_type
    {};

    template<typename Type>
    struct connect_returns_connection<Type, typename std::enable_if<
        std::is_same<
            decltype(std::declval<Type>().connect(
                std::declval<typename Type::callable_type>())),
            connection
        >::value
    >::type> : std::true_type {};

    // connect_returns_slot_id
    //   T.connect(callable) returns a slot_id (event-style).
    template<typename,
              typename = void>
    struct connect_returns_slot_id : std::false_type
    {};

    template<typename Type>
    struct connect_returns_slot_id<Type, typename std::enable_if<
        std::is_same<
            decltype(std::declval<Type>().connect(
                std::declval<typename Type::callable_type>())),
            slot_id
        >::value
    >::type> : std::true_type {};

    // connect_returns_void
    //   T.connect(callable) returns void (delegate-style).
    template<typename,
              typename = void>
    struct connect_returns_void : std::false_type
    {};

    template<typename Type>
    struct connect_returns_void<Type, typename std::enable_if<
        std::is_void<
            decltype(std::declval<Type>().connect(
                std::declval<typename Type::callable_type>()))
        >::value
    >::type> : std::true_type {};

}   // NS_INTERNAL




// ── convenience aliases ──────────────────────────────────────────────────

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Type> D_CONSTEXPR_INLINE_VAR bool has_connect_v                = internal::has_connect<Type>::value;
#endif
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Type> D_CONSTEXPR_INLINE_VAR bool has_notify_v                 = internal::has_notify<Type>::value;
#endif
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Type> D_CONSTEXPR_INLINE_VAR bool has_disconnect_all_v         = internal::has_disconnect_all<Type>::value;
#endif
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Type> D_CONSTEXPR_INLINE_VAR bool has_count_v                  = internal::has_count<Type>::value;
#endif
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Type> D_CONSTEXPR_INLINE_VAR bool has_compact_v                = internal::has_compact<Type>::value;
#endif
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Type> D_CONSTEXPR_INLINE_VAR bool has_signature_type_v         = internal::has_signature_type<Type>::value;
#endif
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Type> D_CONSTEXPR_INLINE_VAR bool has_callable_type_v          = internal::has_callable_type<Type>::value;
#endif
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Type> D_CONSTEXPR_INLINE_VAR bool has_return_type_v            = internal::has_return_type<Type>::value;
#endif
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Type> D_CONSTEXPR_INLINE_VAR bool has_static_capacity_v        = internal::has_static_capacity<Type>::value;
#endif
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Type> D_CONSTEXPR_INLINE_VAR bool has_disconnect_void_v        = internal::has_disconnect_void<Type>::value;
#endif
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Type> D_CONSTEXPR_INLINE_VAR bool has_disconnect_by_id_v       = internal::has_disconnect_by_id<Type>::value;
#endif
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Type> D_CONSTEXPR_INLINE_VAR bool connect_returns_connection_v = internal::connect_returns_connection<Type>::value;
#endif
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Type> D_CONSTEXPR_INLINE_VAR bool connect_returns_slot_id_v    = internal::connect_returns_slot_id<Type>::value;
#endif
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Type> D_CONSTEXPR_INLINE_VAR bool connect_returns_void_v       = internal::connect_returns_void<Type>::value;
#endif




// ═══════════════════════════════════════════════════════════════════════════════
//  COMPOSITE IDENTITY TRAITS
// ═══════════════════════════════════════════════════════════════════════════════

// is_observable
//   type trait: minimum requirement - has connect, has notify, has count.
template<typename Type>
struct is_observable : conjunction<
    internal::has_connect<Type>,
    internal::has_notify<Type>,
    internal::has_count<Type>
> {};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Type>
D_CONSTEXPR_INLINE_VAR bool is_observable_v = is_observable<Type>::value;
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES



// is_delegate
//   type trait: single-slot observable.  capacity == 1, connect returns void.
template<typename Type>
struct is_delegate : conjunction<
    is_observable<Type>,
    internal::has_disconnect_void<Type>,
    internal::connect_returns_void<Type>
> {};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Type>
D_CONSTEXPR_INLINE_VAR bool is_delegate_v = is_delegate<Type>::value;
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES



// is_event
//   type trait: fixed-capacity observable.  capacity > 0, connect returns slot_id.
template<typename Type>
struct is_event : conjunction<
    is_observable<Type>,
    internal::has_static_capacity<Type>,
    internal::connect_returns_slot_id<Type>
> {};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Type>
D_CONSTEXPR_INLINE_VAR bool is_event_v = is_event<Type>::value;
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES



// is_observer
//   type trait: dynamic observable.  connect returns connection, has compact.
template<typename Type>
struct is_observer : conjunction<
    is_observable<Type>,
    internal::connect_returns_connection<Type>,
    internal::has_compact<Type>
> {};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Type>
D_CONSTEXPR_INLINE_VAR bool is_observer_v = is_observer<Type>::value;
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES



// has_connection_tracking
//   type trait: connect returns a connection handle (observer only).
template<typename Type>
struct has_connection_tracking : internal::connect_returns_connection<Type> {};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Type>
D_CONSTEXPR_INLINE_VAR bool has_connection_tracking_v = has_connection_tracking<Type>::value;
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES



// is_bounded
//   type trait: has a non-zero static capacity (delegate or event, not observer).
template<typename Type,
              typename = void>
struct is_bounded : std::false_type
    {};

template<typename Type>
struct is_bounded<Type, typename std::enable_if<
    internal::has_static_capacity<Type>::value && (Type::capacity > 0)
>::type> : std::true_type {};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Type>
D_CONSTEXPR_INLINE_VAR bool is_bounded_v = is_bounded<Type>::value;
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES



// observable_arity
//   Extracts the arity (argument count) from an observable's signature_type.
template<typename Type, typename = void>
struct observable_arity { static constexpr std::size_t value = 0; };

template<typename Type>
struct observable_arity<Type, typename std::enable_if<
    internal::has_signature_type<Type>::value
>::type> {
    static constexpr std::size_t value =
        sig_decompose<typename Type::signature_type>::arity;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Type>
D_CONSTEXPR_INLINE_VAR std::size_t observable_arity_v = observable_arity<Type>::value;
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


}   // namespace observer_traits




// ═══════════════════════════════════════════════════════════════════════════════
//  §7  CONVENIENCE ALIASES
// ═══════════════════════════════════════════════════════════════════════════════

// observer_ptr<Sig>
//   Just a renamed delegate - one fn_ptr, 8 bytes, zero overhead.
//   Named to communicate intent: "I'm a single observation point."
template<typename Signature>
using observer_ptr = delegate<Signature>;


NS_END  // djinterp

#endif  // floor, for now



#endif  // DJINTERP_PARADIGM_OBSERVER_OBSERVER_HPP
