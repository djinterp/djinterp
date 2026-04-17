/*******************************************************************************
* djinterp [paradigm]                                              observer.hpp
*
*   Standalone observer pattern in three cost tiers.  No UI knowledge.  No
* component coupling.  Pure pattern — attach it to whatever you like.
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
*
*     delegate<void(int), std::function<void(int)>>   // single, with captures
*     event<void(int), 4, std::function<void(int)>>   // 4 inline, with captures
*     observer<void(int), void(*)(int)>               // dynamic, fn_ptr only
*
*
* file:      /inc/djinterp/paradigm/pattern/observer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                           date: 2025.05.19
*******************************************************************************/

#ifndef  DJINTERP_PATTERN_OBSERVER_
#define  DJINTERP_PATTERN_OBSERVER_ 1

#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <vector>

#include <djinterp>


NS_DJINTERP
NS_PATTERN


// ═══════════════════════════════════════════════════════════════════════════════
//  §1  SIGNATURE DECOMPOSITION
// ═══════════════════════════════════════════════════════════════════════════════
//   Extracts return type, argument types, function pointer type, and arity
// from a function signature like R(Args...).

template <typename>
struct sig_decompose;

template <typename _Ret, typename... _Args>
struct sig_decompose<_Ret(_Args...)>
{
    using return_type  = _Ret;
    using fn_ptr_type  = _Ret(*)(_Args...);

    static constexpr std::size_t arity = sizeof...(_Args);
};


/*****************************************************************************/

// ═══════════════════════════════════════════════════════════════════════════════
//  §2  CONNECTION HANDLE  (for observer<> tier only)
// ═══════════════════════════════════════════════════════════════════════════════
//   A connection is a lightweight handle that marks a slot as dead when
// disconnected or destroyed (via scoped_connection).  The observer owns the
// actual callable; the connection just flips a boolean.
//
//   Cost: one shared_ptr<bool> per connection (16 bytes typical).
//   delegate and event do NOT use this — they have cheaper disconnect
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
    [[nodiscard]] bool connected() const noexcept 
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


/*****************************************************************************/

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
    [[nodiscard]] bool connected() const noexcept { return conn_.connected(); }

    // release
    //   Surrender ownership — the slot remains live, but this guard
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


/*****************************************************************************/

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

    [[nodiscard]] std::size_t size() const noexcept { return conns_.size(); }

private:
    std::vector<connection> conns_;
};


/*****************************************************************************/

// ═══════════════════════════════════════════════════════════════════════════════
//  §3  TIER 0 — DELEGATE
// ═══════════════════════════════════════════════════════════════════════════════
//   Single callable slot.  Default storage: raw function pointer.
//
//   Cost:       sizeof(_Callable)  —  typically 8 bytes for fn_ptr.
//   Heap:       never (unless _Callable itself allocates).
//   Captures:   no (with fn_ptr default).  Override _Callable for captures.
//   Lifetime:   no connection handle.  connect() replaces.  disconnect() nulls.
//
//   Stateless lambdas decay to function pointers and work out of the box:
//     delegate<void(int)> d;
//     d.connect([](int x) { printf("%d\n", x); });   // fine — decays to fn_ptr
//
//   Capturing lambdas require explicit _Callable override:
//     delegate<void(int), std::function<void(int)>> d;
//     d.connect([&](int x) { obj.handle(x); });      // fine — std::function

// primary template (unspecialised)
template <typename _Signature,
          typename _Callable = typename sig_decompose<_Signature>::fn_ptr_type>
class delegate;

// partial specialisation that decomposes R(Args...)
template <typename _Ret, typename... _Args, typename _Callable>
class delegate<_Ret(_Args...), _Callable>
{
public:
    // ── type aliases ─────────────────────────────────────────────────────
    using signature_type = _Ret(_Args...);
    using callable_type  = _Callable;
    using return_type    = _Ret;

    static constexpr std::size_t capacity = 1;

    // ── construction ─────────────────────────────────────────────────────
    delegate() = default;
    /*implicit*/ delegate(_Callable fn) : fn_(std::move(fn)) {}

    // ── connect / disconnect ─────────────────────────────────────────────

    // connect
    //   Replaces the current callable.  Any previous callable is discarded.
    void connect(_Callable fn) { fn_ = std::move(fn); }

    // disconnect
    //   Clears the callable.
    void disconnect() { fn_ = _Callable{}; }

    // disconnect_all
    //   Same as disconnect() — provided for interface uniformity.
    void disconnect_all() { disconnect(); }

    // ── query ────────────────────────────────────────────────────────────

    // connected
    //   True if a callable is attached.
    [[nodiscard]] bool connected() const noexcept 
    { 
        return static_cast<bool>(fn_); 
    }

    // count
    //   Returns 0 or 1.
    [[nodiscard]] std::size_t count() const noexcept 
    { 
        return connected() ? 1 : 0; 
    }

    // ── notify ───────────────────────────────────────────────────────────

    // notify
    //   Invokes the callable if connected.
    //   For non-void return types: returns R{} if not connected.
    decltype(auto) notify(_Args... args) const
    {
        if constexpr (std::is_void_v<_Ret>) {
            if (fn_) fn_(args...);
        } 
        else {
            if (fn_) return fn_(args...);
            return _Ret{};
        }
    }

    // operator()
    //   Alias for notify().
    decltype(auto) operator()(_Args... args) const { return notify(args...); }

    // ── access ───────────────────────────────────────────────────────────

    // get
    //   Direct access to the stored callable.
    _Callable&       get()       noexcept { return fn_; }
    const _Callable& get() const noexcept { return fn_; }

private:
    _Callable fn_{};
};


/*****************************************************************************/

// ═══════════════════════════════════════════════════════════════════════════════
//  §4  TIER 1 — EVENT
// ═══════════════════════════════════════════════════════════════════════════════
//   Fixed-capacity inline array of callables.  No heap.  No connection handles.
//
//   Cost:       _Capacity × sizeof(_Callable) + sizeof(size_t).
//               e.g. event<void(int), 4> ≈ 40 bytes with fn_ptr.
//   Heap:       never (unless _Callable itself allocates).
//   Captures:   no (with fn_ptr default).  Override _Callable for captures.
//   Lifetime:   connect() returns a slot_id (std::size_t).  disconnect(id)
//               nulls that slot.  Slots are reused on next connect().
//
//   Intended for scenarios where the observer count is known at compile time:
//     event<void(int, int), 4>  on_resize;
//     auto id = on_resize.connect(handle_resize);
//     on_resize.notify(80, 24);
//     on_resize.disconnect(id);

// primary template (unspecialised)
template <typename _Signature,
          std::size_t _Capacity,
          typename _Callable = typename sig_decompose<_Signature>::fn_ptr_type>
class event;

// slot_id
//   Index returned by event::connect().  Cheaper than a connection handle —
// just an integer.  The user is responsible for not using a stale id after
// disconnection (it will simply address a null slot, which is a safe no-op).
using slot_id = std::size_t;

// sentinel for "no free slot"
inline constexpr slot_id no_slot = static_cast<slot_id>(-1);

// partial specialisation
template <typename _Ret, typename... _Args, std::size_t _Capacity, typename _Callable>
class event<_Ret(_Args...), _Capacity, _Callable>
{
public:
    // ── type aliases ─────────────────────────────────────────────────────
    using signature_type = _Ret(_Args...);
    using callable_type  = _Callable;
    using return_type    = _Ret;

    static constexpr std::size_t capacity = _Capacity;

    // ── construction ─────────────────────────────────────────────────────
    event() { slots_.fill(_Callable{}); }

    // ── connect / disconnect ─────────────────────────────────────────────

    // connect
    //   Stores the callable in the first available (null) slot.
    //   Returns the slot_id, or no_slot if full.
    slot_id connect(_Callable fn)
    {
        for (std::size_t i = 0; i < _Capacity; ++i) {
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
        if (id < _Capacity) slots_[id] = _Callable{};
    }

    // disconnect_all
    //   Nulls every slot.
    void disconnect_all()
    {
        for (auto& s : slots_) s = _Callable{};
    }

    // ── query ────────────────────────────────────────────────────────────

    // count
    //   Number of non-null slots.
    [[nodiscard]] std::size_t count() const noexcept
    {
        std::size_t n = 0;
        for (auto& s : slots_)
            if (static_cast<bool>(s)) ++n;
        return n;
    }

    // full
    //   True if every slot is occupied.
    [[nodiscard]] bool full() const noexcept { return count() == _Capacity; }

    // empty
    //   True if no slot is occupied.
    [[nodiscard]] bool empty() const noexcept { return count() == 0; }

    // slot_connected
    //   True if the given slot is live.
    [[nodiscard]] bool slot_connected(slot_id id) const noexcept
    {
        return id < _Capacity && static_cast<bool>(slots_[id]);
    }

    // ── notify ───────────────────────────────────────────────────────────

    // notify
    //   Invokes all non-null slots.  Order: 0 → _Capacity-1.
    void notify(_Args... args) const
    {
        for (auto& s : slots_)
            if (static_cast<bool>(s)) s(args...);
    }

    // operator()
    void operator()(_Args... args) const { notify(args...); }

    // ── access ───────────────────────────────────────────────────────────

    // at
    //   Direct access to the callable at a given slot.
    _Callable&       at(slot_id id)       { return slots_[id]; }
    const _Callable& at(slot_id id) const { return slots_[id]; }

    // compact
    //   Defragments: moves all live slots to the front.  Invalidates
    // previously returned slot_ids — call only when you discard all ids.
    void compact()
    {
        std::size_t write = 0;
        for (std::size_t read = 0; read < _Capacity; ++read) {
            if (static_cast<bool>(slots_[read])) {
                if (write != read) {
                    slots_[write] = std::move(slots_[read]);
                    slots_[read] = _Callable{};
                }
                ++write;
            }
        }
    }

private:
    std::array<_Callable, _Capacity> slots_;
};


/*****************************************************************************/

// ═══════════════════════════════════════════════════════════════════════════════
//  §5  TIER 2 — OBSERVER
// ═══════════════════════════════════════════════════════════════════════════════
//   Dynamic, connection-tracked, unlimited capacity.
//
//   Cost:       ~24 bytes base (std::vector) + per-slot:
//                 sizeof(_Callable) + sizeof(shared_ptr<bool>)
//               Typical: ~40 bytes per slot with std::function.
//   Heap:       yes — both the vector and connection tracking allocate.
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
template <typename _Signature,
          typename _Callable = std::function<_Signature>>
class observer;

// partial specialisation
template <typename _Ret, typename... _Args, typename _Callable>
class observer<_Ret(_Args...), _Callable>
{
public:
    // ── type aliases ─────────────────────────────────────────────────────
    using signature_type = _Ret(_Args...);
    using callable_type  = _Callable;
    using return_type    = _Ret;

    static constexpr std::size_t capacity = 0;  // unbounded

    // ── connect / disconnect ─────────────────────────────────────────────

    // connect
    //   Appends a callable.  Returns a connection handle for lifetime
    // management.
    connection connect(_Callable fn)
    {
        auto alive = std::make_shared<bool>(true);
        slots_.push_back({ std::move(fn), alive });
        return connection(std::move(alive));
    }

    // operator+=
    //   Shorthand for connect().
    connection operator+=(_Callable fn) { return connect(std::move(fn)); }

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
    [[nodiscard]] std::size_t count() const noexcept
    {
        std::size_t n = 0;
        for (auto& s : slots_)
            if (*s.alive) ++n;
        return n;
    }

    // empty
    [[nodiscard]] bool empty() const noexcept { return count() == 0; }

    // size_including_dead
    //   Total vector size, including dead-but-not-yet-compacted slots.
    [[nodiscard]] std::size_t size_including_dead() const noexcept 
    { 
        return slots_.size(); 
    }

    // ── notify ───────────────────────────────────────────────────────────

    // notify
    //   Invokes all live slots.  Dead slots are skipped.
    void notify(_Args... args) const
    {
        for (auto& s : slots_)
            if (*s.alive) s.fn(args...);
    }

    // operator()
    void operator()(_Args... args) const { notify(args...); }

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
        _Callable              fn;
        std::shared_ptr<bool>  alive;
    };

    mutable std::vector<slot_entry> slots_;
};


/*****************************************************************************/

// ═══════════════════════════════════════════════════════════════════════════════
//  §6  OBSERVER TRAITS  (SFINAE detection)
// ═══════════════════════════════════════════════════════════════════════════════
//   Mirrors the menu_traits pattern in menu.hpp: detail namespace holds
// fine-grained detectors, top-level traits compose them.
//
//   These let generic code discover observer capabilities at compile time
// without coupling to concrete types:
//
//     template <typename T>
//     void maybe_attach(T& obs) {
//         if constexpr (observer_traits::is_observable_v<T>) {
//             obs.connect(my_handler);
//         }
//     }

namespace observer_traits {
namespace detail 
{
    // ── method detectors ─────────────────────────────────────────────────

    // has_connect
    //   type trait: T has a .connect(...) method
    template <typename, 
              typename = void>
    struct has_connect : std::false_type {};

    template <typename _Type>
    struct has_connect<_Type, std::void_t<
        decltype(std::declval<_Type>().connect(std::declval<typename _Type::callable_type>()))
    >> : std::true_type {};

    // has_notify
    //   type trait: T has a .notify(...) method
    //   (we check for the presence of operator() as proxy, since notify's
    //    argument types vary)
    template <typename, 
              typename = void>
    struct has_notify : std::false_type {};

    template <typename _Type>
    struct has_notify<_Type, std::void_t<
        decltype(&_Type::notify)
    >> : std::true_type {};

    // has_disconnect_all
    template <typename, 
              typename = void>
    struct has_disconnect_all : std::false_type {};

    template <typename _Type>
    struct has_disconnect_all<_Type, std::void_t<
        decltype(std::declval<_Type>().disconnect_all())
    >> : std::true_type {};

    // has_count
    template <typename, 
              typename = void>
    struct has_count : std::false_type {};

    template <typename _Type>
    struct has_count<_Type, std::void_t<
        decltype(std::declval<_Type>().count())
    >> : std::true_type {};

    // has_compact
    template <typename, 
              typename = void>
    struct has_compact : std::false_type {};

    template <typename _Type>
    struct has_compact<_Type, std::void_t<
        decltype(std::declval<_Type>().compact())
    >> : std::true_type {};

    /***********************************************************************/

    // ── type alias detectors ─────────────────────────────────────────────

    // has_signature_type
    template <typename, 
              typename = void>
    struct has_signature_type : std::false_type {};

    template <typename _Type>
    struct has_signature_type<_Type, std::void_t<
        typename _Type::signature_type
    >> : std::true_type {};

    // has_callable_type
    template <typename, 
              typename = void>
    struct has_callable_type : std::false_type {};

    template <typename _Type>
    struct has_callable_type<_Type, std::void_t<
        typename _Type::callable_type
    >> : std::true_type {};

    // has_return_type
    template <typename, 
              typename = void>
    struct has_return_type : std::false_type {};

    template <typename _Type>
    struct has_return_type<_Type, std::void_t<
        typename _Type::return_type
    >> : std::true_type {};

    /***********************************************************************/

    // ── capacity detector ────────────────────────────────────────────────

    // has_static_capacity
    //   True if T::capacity is a valid static constexpr member.
    template <typename, 
              typename = void>
    struct has_static_capacity : std::false_type {};

    template <typename _Type>
    struct has_static_capacity<_Type, std::void_t<
        decltype(_Type::capacity)
    >> : std::true_type {};

    /***********************************************************************/

    // ── disconnect style detectors ───────────────────────────────────────

    // has_disconnect_void
    //   T has .disconnect() with no arguments (delegate-style).
    template <typename,
              typename = void>
    struct has_disconnect_void : std::false_type {};

    template <typename _Type>
    struct has_disconnect_void<_Type, std::void_t<
        decltype(std::declval<_Type>().disconnect())
    >> : std::true_type {};

    // has_disconnect_by_id
    //   T has .disconnect(slot_id) (event-style).
    template <typename,
              typename = void>
    struct has_disconnect_by_id : std::false_type {};

    template <typename _Type>
    struct has_disconnect_by_id<_Type, std::void_t<
        decltype(std::declval<_Type>().disconnect(std::declval<slot_id>()))
    >> : std::true_type {};

    // connect_returns_connection
    //   T.connect(callable) returns a connection (observer-style).
    template <typename,
              typename = void>
    struct connect_returns_connection : std::false_type {};

    template <typename _Type>
    struct connect_returns_connection<_Type, std::enable_if_t<
        std::is_same_v<
            decltype(std::declval<_Type>().connect(
                std::declval<typename _Type::callable_type>())),
            connection
        >
    >> : std::true_type {};

    // connect_returns_slot_id
    //   T.connect(callable) returns a slot_id (event-style).
    template <typename,
              typename = void>
    struct connect_returns_slot_id : std::false_type {};

    template <typename _Type>
    struct connect_returns_slot_id<_Type, std::enable_if_t<
        std::is_same_v<
            decltype(std::declval<_Type>().connect(
                std::declval<typename _Type::callable_type>())),
            slot_id
        >
    >> : std::true_type {};

    // connect_returns_void
    //   T.connect(callable) returns void (delegate-style).
    template <typename,
              typename = void>
    struct connect_returns_void : std::false_type {};

    template <typename _Type>
    struct connect_returns_void<_Type, std::enable_if_t<
        std::is_void_v<
            decltype(std::declval<_Type>().connect(
                std::declval<typename _Type::callable_type>()))
        >
    >> : std::true_type {};

}   // namespace detail


/*****************************************************************************/

// ── convenience aliases ──────────────────────────────────────────────────

template <typename _T> inline constexpr bool has_connect_v             = detail::has_connect<_T>::value;
template <typename _T> inline constexpr bool has_notify_v              = detail::has_notify<_T>::value;
template <typename _T> inline constexpr bool has_disconnect_all_v      = detail::has_disconnect_all<_T>::value;
template <typename _T> inline constexpr bool has_count_v               = detail::has_count<_T>::value;
template <typename _T> inline constexpr bool has_compact_v             = detail::has_compact<_T>::value;
template <typename _T> inline constexpr bool has_signature_type_v      = detail::has_signature_type<_T>::value;
template <typename _T> inline constexpr bool has_callable_type_v       = detail::has_callable_type<_T>::value;
template <typename _T> inline constexpr bool has_return_type_v         = detail::has_return_type<_T>::value;
template <typename _T> inline constexpr bool has_static_capacity_v     = detail::has_static_capacity<_T>::value;
template <typename _T> inline constexpr bool has_disconnect_void_v     = detail::has_disconnect_void<_T>::value;
template <typename _T> inline constexpr bool has_disconnect_by_id_v    = detail::has_disconnect_by_id<_T>::value;
template <typename _T> inline constexpr bool connect_returns_connection_v = detail::connect_returns_connection<_T>::value;
template <typename _T> inline constexpr bool connect_returns_slot_id_v    = detail::connect_returns_slot_id<_T>::value;
template <typename _T> inline constexpr bool connect_returns_void_v       = detail::connect_returns_void<_T>::value;


/*****************************************************************************/

// ═══════════════════════════════════════════════════════════════════════════════
//  COMPOSITE IDENTITY TRAITS
// ═══════════════════════════════════════════════════════════════════════════════

// is_observable
//   type trait: minimum requirement — has connect, has notify, has count.
template <typename _Type>
struct is_observable : std::conjunction<
    detail::has_connect<_Type>,
    detail::has_notify<_Type>,
    detail::has_count<_Type>
> {};

template <typename _Type>
inline constexpr bool is_observable_v = is_observable<_Type>::value;

/*****************************************************************************/

// is_delegate
//   type trait: single-slot observable.  capacity == 1, connect returns void.
template <typename _Type>
struct is_delegate : std::conjunction<
    is_observable<_Type>,
    detail::has_disconnect_void<_Type>,
    detail::connect_returns_void<_Type>
> {};

template <typename _Type>
inline constexpr bool is_delegate_v = is_delegate<_Type>::value;

/*****************************************************************************/

// is_event
//   type trait: fixed-capacity observable.  capacity > 0, connect returns slot_id.
template <typename _Type>
struct is_event : std::conjunction<
    is_observable<_Type>,
    detail::has_static_capacity<_Type>,
    detail::connect_returns_slot_id<_Type>
> {};

template <typename _Type>
inline constexpr bool is_event_v = is_event<_Type>::value;

/*****************************************************************************/

// is_observer
//   type trait: dynamic observable.  connect returns connection, has compact.
template <typename _Type>
struct is_observer : std::conjunction<
    is_observable<_Type>,
    detail::connect_returns_connection<_Type>,
    detail::has_compact<_Type>
> {};

template <typename _Type>
inline constexpr bool is_observer_v = is_observer<_Type>::value;

/*****************************************************************************/

// has_connection_tracking
//   type trait: connect returns a connection handle (observer only).
template <typename _Type>
struct has_connection_tracking : detail::connect_returns_connection<_Type> {};

template <typename _Type>
inline constexpr bool has_connection_tracking_v = has_connection_tracking<_Type>::value;

/*****************************************************************************/

// is_bounded
//   type trait: has a non-zero static capacity (delegate or event, not observer).
template <typename, typename = void>
struct is_bounded : std::false_type {};

template <typename _Type>
struct is_bounded<_Type, std::enable_if_t<
    detail::has_static_capacity<_Type>::value && (_Type::capacity > 0)
>> : std::true_type {};

template <typename _Type>
inline constexpr bool is_bounded_v = is_bounded<_Type>::value;

/*****************************************************************************/

// observable_arity
//   Extracts the arity (argument count) from an observable's signature_type.
template <typename _Type, typename = void>
struct observable_arity { static constexpr std::size_t value = 0; };

template <typename _Type>
struct observable_arity<_Type, std::enable_if_t<
    detail::has_signature_type<_Type>::value
>> {
    static constexpr std::size_t value = 
        sig_decompose<typename _Type::signature_type>::arity;
};

template <typename _Type>
inline constexpr std::size_t observable_arity_v = observable_arity<_Type>::value;


}   // namespace observer_traits


/*****************************************************************************/

// ═══════════════════════════════════════════════════════════════════════════════
//  §7  CONVENIENCE ALIASES
// ═══════════════════════════════════════════════════════════════════════════════

// observer_ptr<Sig>
//   Just a renamed delegate — one fn_ptr, 8 bytes, zero overhead.
//   Named to communicate intent: "I'm a single observation point."
template <typename _Signature>
using observer_ptr = delegate<_Signature>;


NS_END  // pattern
NS_END  // djinterp



#endif  // DJINTERP_PATTERN_OBSERVER_