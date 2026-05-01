/******************************************************************************
* djinterp [container]                                mutex_table_base.hpp
*
*   Common base for the threadsafe table wrappers.  Holds the wrapped
* container by value, owns a policy-driven mutex, and provides the
* shared locking surface every typed wrapper builds on:
*
*     with_read(fn)   - run fn under a read lock; return its result
*     with_write(fn)  - run fn under a write lock; return its result
*     size() / empty()
*     snapshot()      - locked deep copy of the wrapped value
*     replace(...)    - atomic full-state swap
*     mutex()         - direct mutex access (advanced use)
*     unsafe_unwrap() - direct wrapped access (advanced; caller holds lock)
*
*   The base also surfaces ::lock_type, ::mutex_type, ::read_lock_type,
* ::write_lock_type, and ::underlying_container_type so the existing
* trait system classifies wrappers correctly:
*     axis 8 (thread safety) - via lock_type / mutex_type
*     axis 9 (underlying)    - via underlying_container_type (overlay)
*
*   STORAGE MODEL:
*   The wrapped container is owned by value (m_wrapped).  No COW, no
* reference counting - writes mutate in place under the write lock, reads
* execute under the read lock.  For COW-style semantics over the same
* shape, compose with `cow_state` from cow.hpp instead.
*
*   EXTENSION CONTRACT:
*   Derived classes add operation-specific locked accessors (e.g.
* push_back on mutex_table, find_copy_or on
* mutex_lookup_table).  Derived classes MUST NOT add additional
* mutable members - all state lives on m_wrapped, all serialization
* happens through m_mutex.
*
* DEPENDENCIES:
*   djinterp.hpp                - NS_DJINTERP, D_CONSTEXPR
*   threadsafe/lock_policy.hpp  - default_lock_policy, policy structs
*
* TABLE OF CONTENTS
* =================
* I.    mutex_table_base
*
*
* path:      /inc/djinterp/container/table/mutex_table_base.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_MUTEX_TABLE_BASE_
#define DJINTERP_MUTEX_TABLE_BASE_ 1

#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "`mutex_table_base.hpp` requires C++11 or later "          \
           "(<mutex>, move semantics)."
#endif

#include <type_traits>
#include <utility>
#include "../../djinterp.hpp"
#include "../../threadsafe/lock_policy.hpp"


NS_DJINTERP
NS_CONTAINER


    // =========================================================================
    // I.   mutex_table_base
    // =========================================================================

    // mutex_table_base
    //   class: shared foundation for the threadsafe_X table wrappers.
    // Owns the wrapped container and a mutex, exposes generic locked
    // visitors, and surfaces the canonical alias surface so the
    // structural trait system classifies derived wrappers correctly.
    //
    //   Non-copyable (contains a mutex), non-movable for the same
    // reason.  Derived classes inherit these properties.
    //
    //   The wrapped container is constructed from forwarded arguments
    // via the in-place constructor; copy / move construction from an
    // existing wrapped value are also provided as conveniences.
    template<typename _Wrapped,
             typename _Policy = threadsafe::default_lock_policy>
    class mutex_table_base
    {
    public:
        // -----------------------------------------------------------------
        //  type aliases (policy + locking)
        // -----------------------------------------------------------------
        using wrapped_type     = _Wrapped;
        using lock_policy_type = _Policy;
        using mutex_type       = typename _Policy::mutex_type;
        using read_lock_type   = typename _Policy::read_lock_type;
        using write_lock_type  = typename _Policy::write_lock_type;

        // lock_type
        //   alias: the canonical "lock type" alias detected by axis-8
        // (thread-safety) traits.  We use the write lock type because
        // it's the strictest contract; container_threadsafe_class also
        // recognizes mutex_type independently for the actual level.
        using lock_type = write_lock_type;

        // -----------------------------------------------------------------
        //  type aliases (canonical surface from the wrapped type)
        //   These pull through the most universal aliases so the trait
        // surface still classifies the wrapper correctly along the
        // foundational axes.  Type-specific aliases (key_type,
        // element_type, cvar_type, etc.) are surfaced by the derived
        // wrappers, not here.
        // -----------------------------------------------------------------
        using underlying_container_type = _Wrapped;
        using value_type      = typename _Wrapped::value_type;
        using size_type       = typename _Wrapped::size_type;
        using difference_type = typename _Wrapped::difference_type;


        // =================================================================
        //  CONSTRUCTORS
        // =================================================================

        // mutex_table_base()
        //   constructor: default - empty wrapped, default-constructed
        // mutex.
        mutex_table_base() = default;

        // mutex_table_base(wrapped, lvalue)
        //   constructor: copy an existing wrapped value into the
        // wrapper.
        explicit mutex_table_base(
            const _Wrapped& _wrapped
        )
            : m_wrapped(_wrapped)
        {}

        // mutex_table_base(wrapped, rvalue)
        //   constructor: move an existing wrapped value into the
        // wrapper.
        explicit mutex_table_base(
            _Wrapped&& _wrapped
        ) noexcept(
            std::is_nothrow_move_constructible<_Wrapped>::value)
            : m_wrapped(static_cast<_Wrapped&&>(_wrapped))
        {}

        // mutex_table_base(in_place, args...)
        //   constructor: forwards args directly to the wrapped
        // container's constructor.
        template<typename... _Args>
        explicit mutex_table_base(
            std::piecewise_construct_t,
            _Args&&... _args
        )
            : m_wrapped(static_cast<_Args&&>(_args)...)
        {}

        // non-copyable (mutex), non-movable
        mutex_table_base(
            const mutex_table_base&)            = delete;
        mutex_table_base& operator=(
            const mutex_table_base&)            = delete;
        mutex_table_base(
            mutex_table_base&&)                 = delete;
        mutex_table_base& operator=(
            mutex_table_base&&)                 = delete;

        ~mutex_table_base() = default;


        // =================================================================
        //  GENERIC LOCKED VISITORS
        // =================================================================

        // with_read
        //   function: invokes _fn under a read lock with a const
        // reference to the wrapped value.  Returns whatever _fn
        // returns.  The lock is held for the duration of the call.
        //
        // Example:
        //   auto sz = ts.with_read([](const auto& t){ return t.size(); });
        template<typename _Fn>
        auto
        with_read(_Fn&& _fn) const
            -> decltype(static_cast<_Fn&&>(_fn)(
                std::declval<const _Wrapped&>()))
        {
            read_lock_type guard(m_mutex);

            return static_cast<_Fn&&>(_fn)(m_wrapped);
        }

        // with_write
        //   function: invokes _fn under a write lock with a mutable
        // reference to the wrapped value.  Returns whatever _fn
        // returns.  The lock is held for the duration of the call.
        //
        // Example:
        //   ts.with_write([](auto& t){
        //       t.push_back(1);
        //       t.push_back(2);
        //   });
        template<typename _Fn>
        auto
        with_write(_Fn&& _fn)
            -> decltype(static_cast<_Fn&&>(_fn)(
                std::declval<_Wrapped&>()))
        {
            write_lock_type guard(m_mutex);

            return static_cast<_Fn&&>(_fn)(m_wrapped);
        }


        // =================================================================
        //  CAPACITY (locked)
        // =================================================================

        // size
        //   function: count of elements in the wrapped container.
        size_type
        size() const
        {
            read_lock_type guard(m_mutex);

            return m_wrapped.size();
        }

        // empty
        //   function: true when the wrapped container is empty.
        bool
        empty() const
        {
            read_lock_type guard(m_mutex);

            return m_wrapped.empty();
        }


        // =================================================================
        //  SNAPSHOT / REPLACE
        // =================================================================

        // snapshot
        //   function: returns a deep copy of the wrapped container,
        // taken under a read lock.  The returned value is independent
        // of subsequent mutations to the source.
        //
        //   Requires _Wrapped to be copy-constructible.  For large or
        // non-copyable wrapped types, prefer the with_read visitor.
        _Wrapped
        snapshot() const
        {
            read_lock_type guard(m_mutex);

            return m_wrapped;
        }

        // replace
        //   function: atomically replaces the entire wrapped container.
        // The previous value is destroyed under the lock.
        void
        replace(const _Wrapped& _new_wrapped)
        {
            write_lock_type guard(m_mutex);

            m_wrapped = _new_wrapped;

            return;
        }

        // replace (rvalue)
        //   function: move overload of replace.
        void
        replace(_Wrapped&& _new_wrapped) noexcept(
            std::is_nothrow_move_assignable<_Wrapped>::value)
        {
            write_lock_type guard(m_mutex);

            m_wrapped = static_cast<_Wrapped&&>(_new_wrapped);

            return;
        }


        // =================================================================
        //  ADVANCED ACCESS
        // =================================================================

        // mutex
        //   function: returns a reference to the underlying mutex for
        // callers who need to coordinate locks externally (e.g. when
        // composing two threadsafe wrappers under a single critical
        // section).
        mutex_type&
        mutex() const noexcept
        {
            return m_mutex;
        }

        // unsafe_unwrap
        //   function: returns a const reference to the wrapped
        // container WITHOUT acquiring the lock.  The caller is
        // responsible for serialization - typically used inside a
        // with_read / with_write visitor or after manually acquiring
        // mutex().
        const _Wrapped&
        unsafe_unwrap() const noexcept
        {
            return m_wrapped;
        }

        // unsafe_unwrap (mutable)
        //   function: mutable counterpart.  Same caller-holds-lock
        // contract.
        _Wrapped&
        unsafe_unwrap() noexcept
        {
            return m_wrapped;
        }

    protected:
        _Wrapped           m_wrapped;
        mutable mutex_type m_mutex;
    };


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_MUTEX_TABLE_BASE_
