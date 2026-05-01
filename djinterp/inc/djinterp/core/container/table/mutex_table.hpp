/******************************************************************************
* djinterp [container]                                     mutex_table.hpp
*
*   Threadsafe wrapper around the fundamental owning `table`.  Adds locked
* passthrough methods for the table's interface (push_back, clear, at)
* on top of the generic visitors inherited from mutex_table_base.
*
*   THREAD SAFETY:
*   All public methods acquire the policy-driven lock for the duration
* of the operation.  The default policy (default_lock_policy) is
* shared_lock_policy on C++17+ - reads can run concurrently, writes are
* exclusive.
*
*   Element access returns by VALUE rather than by reference: a reference
* into the wrapped storage would dangle the moment the lock releases.
* For repeated access to the same element, use with_read to keep the
* lock held across multiple reads.
*
*   STORAGE / OWNERSHIP:
*   The wrapped table is owned by value.  Mutations happen in place under
* the write lock - this is NOT a copy-on-write wrapper.  For COW
* semantics over the same shape, use `cow_state<table<...>>` from
* cow.hpp instead.
*
*   USAGE:
*   ```cpp
*   mutex_table<int> t;                       // shared_lock_policy
*   t.push_back(1);
*   t.push_back(2);
*
*   auto v0 = t.at(0);                             // read-locked copy
*   auto sum = t.with_read([](const auto& tab){    // batch read
*       int s = 0;
*       for (int x : tab) s += x;
*       return s;
*   });
*
*   auto snap = t.snapshot();                      // independent copy
*   ```
*
* DEPENDENCIES:
*   djinterp.hpp                  - NS_DJINTERP, D_CONSTEXPR
*   table.hpp                     - container::table
*   mutex_table_base.hpp     - mutex_table_base
*
* TABLE OF CONTENTS
* =================
* I.    mutex_table
* II.   make_mutex_table
*
*
* path:      /inc/djinterp/container/table/mutex_table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_MUTEX_TABLE_
#define DJINTERP_MUTEX_TABLE_ 1

#include <memory>
#include <type_traits>
#include <utility>
#include "../../djinterp.hpp"
#include "./table.hpp"
#include "./mutex_table_base.hpp"


NS_DJINTERP
NS_CONTAINER


    // =========================================================================
    // I.   mutex_table
    // =========================================================================

    // mutex_table
    //   class: locked wrapper around `table<_Type, _Allocator,
    // _Options...>`.  Adds locked passthrough methods for the most
    // common operations; arbitrary other operations are reachable
    // through the inherited with_read / with_write visitors.
    //
    //   The `_Policy` parameter is positional after _Allocator (so the
    // common case `mutex_table<int>` picks up the default policy)
    // but BEFORE _Options... so policy resolution does not collide with
    // the table's option keys.
    template<typename    _Type,
             typename    _Allocator = std::allocator<_Type>,
             typename    _Policy    = threadsafe::default_lock_policy,
             typename... _Options>
    class mutex_table
        : public mutex_table_base<
            table<_Type, _Allocator, _Options...>,
            _Policy>
    {
    private:
        using table_t = table<_Type, _Allocator, _Options...>;
        using base    = mutex_table_base<table_t, _Policy>;

    public:
        // -----------------------------------------------------------------
        //  type aliases pulled in from base
        // -----------------------------------------------------------------
        using typename base::wrapped_type;
        using typename base::lock_policy_type;
        using typename base::mutex_type;
        using typename base::lock_type;
        using typename base::read_lock_type;
        using typename base::write_lock_type;
        using typename base::value_type;
        using typename base::size_type;
        using typename base::difference_type;
        using typename base::underlying_container_type;

        // self_type
        //   type: convenience alias for the full wrapper type.
        using self_type = mutex_table<
            _Type, _Allocator, _Policy, _Options...>;


        // =================================================================
        //  CONSTRUCTORS
        // =================================================================

        // forward all base constructors (default, copy-from-wrapped,
        // move-from-wrapped, in-place).
        using base::base;


        // =================================================================
        //  ELEMENT ACCESS (locked)
        // =================================================================
        //   Element access returns BY VALUE.  Returning a reference
        // would dangle the moment the lock releases.  For batch access
        // without per-element relocking, use with_read.

        // at
        //   function: returns a copy of element _i under a read lock.
        // No bounds check (mirrors table::operator[]).
        value_type
        at(size_type _i) const
        {
            read_lock_type guard(this->m_mutex);

            return this->m_wrapped[_i];
        }

        // operator[]
        //   function: alias for at().  Returns by value to avoid
        // dangling-reference hazards.
        value_type
        operator[](size_type _i) const
        {
            read_lock_type guard(this->m_mutex);

            return this->m_wrapped[_i];
        }


        // =================================================================
        //  MUTATION (locked)
        // =================================================================
        //   Locked passthrough for the most common write operations.
        // For other writes, use with_write.  push_back is gated on the
        // wrapped table's own gating - if the wrapped table refuses
        // push_back (sorted, hashed), this overload is also unavailable.

        // push_back
        //   function: locked append of _v.  Available when the wrapped
        // table is unsorted and unhashed.
        template<typename _T = _Type,
                 typename = decltype(
                     std::declval<table_t&>().push_back(
                         std::declval<const _T&>()))>
        void
        push_back(const _Type& _v)
        {
            write_lock_type guard(this->m_mutex);

            this->m_wrapped.push_back(_v);

            return;
        }

        // push_back (rvalue)
        template<typename _T = _Type,
                 typename = decltype(
                     std::declval<table_t&>().push_back(
                         std::declval<_T&&>()))>
        void
        push_back(_Type&& _v)
        {
            write_lock_type guard(this->m_mutex);

            this->m_wrapped.push_back(static_cast<_Type&&>(_v));

            return;
        }

        // clear
        //   function: locked removal of all elements.
        void
        clear() noexcept
        {
            write_lock_type guard(this->m_mutex);

            this->m_wrapped.clear();

            return;
        }

        // reserve
        //   function: locked pre-allocation.
        void
        reserve(size_type _n)
        {
            write_lock_type guard(this->m_mutex);

            this->m_wrapped.reserve(_n);

            return;
        }
    };


    // =========================================================================
    // II.  make_mutex_table
    // =========================================================================

    // make_mutex_table
    //   function: factory for mutex_table with default allocator
    // and default policy.  Forwards constructor arguments to the
    // wrapped table.
    //
    // Example:
    //   auto ts = make_mutex_table<int>(
    //       {1, 2, 3, 4, 5});
    template<typename    _Type,
             typename    _Allocator = std::allocator<_Type>,
             typename    _Policy    = threadsafe::default_lock_policy,
             typename... _Args>
    auto
    make_mutex_table(_Args&&... _args)
        -> mutex_table<_Type, _Allocator, _Policy>
    {
        return mutex_table<_Type, _Allocator, _Policy>(
            std::piecewise_construct,
            static_cast<_Args&&>(_args)...);
    }


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_MUTEX_TABLE_
