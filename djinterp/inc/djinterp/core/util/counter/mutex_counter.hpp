/******************************************************************************
* djinterp [util]                                            mutex_counter.hpp
*
* Lock-policy-based thread-safe counter.
*   Wraps the base `counter<_ValueType>` with a configurable lock policy
* from the threadsafe module.  Every public operation acquires
* either a read lock (accessors) or a write lock (mutations) through the
* policy's RAII guards.
*
*   The underlying counter is accessed only through the lock — no public
* method exposes an unguarded reference to the internal state.
*
* LOCK ORDERING:
*   Parent locks are always acquired before child locks.  Children must
* not hold references back to the parent to avoid deadlock.
*
* POLICY COMPATIBILITY:
*   Any policy from lock_policy.hpp or lock_policy_c.hpp is accepted:
*   - null_lock_policy         (compiles to nothing — useful for testing)
*   - exclusive_lock_policy    (std::mutex)
*   - shared_lock_policy       (std::shared_mutex, concurrent reads)
*   - timed_lock_policy        (std::timed_mutex)
*   - shared_timed_lock_policy (std::shared_timed_mutex)
*   - c_exclusive, c_shared, c_recursive, c_spinlock
*
* SEE ALSO:
*   counter.hpp         — unsynchronized base
*   atomic_counter.hpp  — lock-free std::atomic variant
*
*   PORTABILITY:
*   Requires C++17 or later.
*
*
* path:      /inc/djinterp/util/counter/mutex_counter.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.07
******************************************************************************/

#ifndef DJINTERP_UTILITY_COUNTER_MUTEX_
#define DJINTERP_UTILITY_COUNTER_MUTEX_ 1

#include <cstdint>
#include <limits>
#include "../../djinterp.hpp"
#include "../../sync/lock_policy.hpp"
#include "../../sync/lock_guard.hpp"
#include "./counter.hpp"


NS_DJINTERP
NS_UTIL


// mutex_counter
//   class: a thread-safe bounded counter with configurable lock policy.
//
//   Wraps `counter<_ValueType>` by composition.  Read operations acquire
// a shared lock (when the policy supports it); write operations acquire
// an exclusive lock.  Children are mutex_counters with the same policy,
// each with their own mutex.
//
//   Template parameter `_ValueType` must be an arithmetic type.
//   Template parameter `_Policy` must be a lock policy struct.
template<typename _ValueType = std::int64_t,
         typename _Policy    = threadsafe::default_lock_policy>
class mutex_counter
{
    static_assert(std::is_arithmetic_v<_ValueType>,
                  "`_ValueType` must be an arithmetic type.");

private:
    using self_type     = mutex_counter<_ValueType, _Policy>;
    using base_type     = counter<_ValueType>;
    using children_type = std::vector<self_type>;
    using observed_type = std::vector<self_type*>;
    using read_guard    = threadsafe::scoped_read_lock<_Policy>;
    using write_guard   = threadsafe::scoped_write_lock<_Policy>;

public:
    using value_type       = _ValueType;
    using size_type        = std::size_t;
    using lock_policy_type = _Policy;
    using mutex_type       = typename _Policy::mutex_type;

    // --- policy descriptors ---
    static constexpr bool is_threadsafe = _Policy::is_threadsafe;
    static constexpr bool is_shared     = _Policy::is_shared;
    static constexpr bool is_timed      = _Policy::is_timed;

    // -----------------------------------------------------------------
    // constructors
    // -----------------------------------------------------------------

    // mutex_counter()
    //   constructor: default-constructs a counter at zero with no bounds.
    mutex_counter()
        : m_counter(),
          m_children(),
          m_observed(),
          m_mutex()
    {}

    // mutex_counter(_initial, _min, _max)
    //   constructor: constructs a counter with an initial value and
    // optional min/max bounds.
    mutex_counter(
		value_type _initial,
		value_type _min = std::numeric_limits<value_type>::lowest(),
		value_type _max = std::numeric_limits<value_type>::max()
	)
        : m_counter(_initial, _min, _max),
          m_children(),
          m_observed(),
          m_mutex()
    {}

    // non-copyable (mutex is non-copyable)
    mutex_counter(const mutex_counter&)            = delete;
    mutex_counter& operator=(const mutex_counter&) = delete;

    // -----------------------------------------------------------------
    // operations (write-locked)
    // -----------------------------------------------------------------

    // increment
    //   increments the counter by `_amount`. returns false and clamps
    // to max if the operation would exceed the upper bound.
    bool increment(
		value_type _amount = value_type{1}
	)
    {
        write_guard guard(m_mutex);

        return m_counter.increment(_amount);
    }

    // decrement
    //   decrements the counter by `_amount`. returns false and clamps
    // to min if the operation would exceed the lower bound.
    bool decrement(
		value_type _amount = value_type{1}
	)
    {
        write_guard guard(m_mutex);

        return m_counter.decrement(_amount);
    }

    // reset
    //   resets the counter to its initial value. does not affect
    // children or observed counters.
    void reset()
    {
        write_guard guard(m_mutex);

        m_counter.reset();

        return;
    }

    // reset_all
    //   resets this counter and all owned children recursively.
    // observed counters are not reset.
    //
    // NOTE: acquires this counter's write lock, then each child's
    // write lock in sequence (parent-before-child ordering).
    void reset_all()
    {
        write_guard guard(m_mutex);

        m_counter.reset();

        for (auto& child : m_children)
        {
            child.reset_all();
        }

        return;
    }

    // -----------------------------------------------------------------
    // try-operations (non-blocking)
    // -----------------------------------------------------------------

    // try_increment
    //   attempts a non-blocking increment. returns false if the lock
    // could not be acquired or if the counter would exceed its upper
    // bound (clamped to max in the latter case).
    bool try_increment(
		value_type _amount = value_type{1}
	)
    {
        threadsafe::scoped_try_lock<_Policy> guard(m_mutex);

        if (!guard.owns_lock())
        {
            return false;
        }

        return m_counter.increment(_amount);
    }

    // try_decrement
    //   attempts a non-blocking decrement. returns false if the lock
    // could not be acquired or if the counter would exceed its lower
    // bound (clamped to min in the latter case).
    bool try_decrement(
		value_type _amount = value_type{1}
	)
    {
        threadsafe::scoped_try_lock<_Policy> guard(m_mutex);

        if (!guard.owns_lock())
        {
            return false;
        }

        return m_counter.decrement(_amount);
    }

    // -----------------------------------------------------------------
    // accessors (read-locked)
    // -----------------------------------------------------------------

    // value
    //   returns the current counter value.
    value_type value() const
    {
        read_guard guard(m_mutex);

        return m_counter.value();
    }

    // initial
    //   returns the initial value the counter was constructed with.
    value_type initial() const
    {
        read_guard guard(m_mutex);

        return m_counter.initial();
    }

    // min
    //   returns the lower bound.
    value_type min() const
    {
        read_guard guard(m_mutex);

        return m_counter.min();
    }

    // max
    //   returns the upper bound.
    value_type max() const
    {
        read_guard guard(m_mutex);

        return m_counter.max();
    }

    // at_min
    //   returns true if the counter is at its lower bound.
    bool at_min() const
    {
        read_guard guard(m_mutex);

        return m_counter.at_min();
    }

    // at_max
    //   returns true if the counter is at its upper bound.
    bool at_max() const
    {
        read_guard guard(m_mutex);

        return m_counter.at_max();
    }

    // -----------------------------------------------------------------
    // children (owning)
    // -----------------------------------------------------------------

    // add_child
    //   constructs and appends an owned child counter. returns a
    // reference to the newly added child.
    //
    // WARNING: the returned reference is invalidated if the
    // children vector reallocates on a subsequent add_child call.
    // The child has its own mutex for independent operation.
    self_type& add_child(
		value_type _initial = value_type{0},
		value_type _min     = std::numeric_limits<value_type>::lowest(),
		value_type _max     = std::numeric_limits<value_type>::max()
	)
    {
        write_guard guard(m_mutex);

        m_children.emplace_back(_initial,
                                _min,
                                _max);

        return m_children.back();
    }

    // child
    //   returns a reference to the owned child at `_index`.
    // the child has its own mutex; callers may operate on it
    // without holding the parent lock.
    self_type& child(
		size_type _index
	)
    {
        read_guard guard(m_mutex);

        return m_children[_index];
    }

    // child (const)
    //   returns a const reference to the owned child at `_index`.
    const self_type& child(
		size_type _index
	) const
    {
        read_guard guard(m_mutex);

        return m_children[_index];
    }

    // child_count
    //   returns the number of owned children.
    size_type child_count() const
    {
        read_guard guard(m_mutex);

        return m_children.size();
    }

    // -----------------------------------------------------------------
    // children (non-owning / observed)
    // -----------------------------------------------------------------

    // observe
    //   registers a non-owning reference to an external counter.
    // the caller is responsible for ensuring the observed counter
    // outlives this counter.
    void observe(
		self_type& _target
	)
    {
        write_guard guard(m_mutex);

        m_observed.push_back(&_target);

        return;
    }

    // observed
    //   returns a pointer to the observed counter at `_index`,
    // or nullptr if out of range.
    self_type* observed(
		size_type _index
	) const
    {
        read_guard guard(m_mutex);

        if (_index >= m_observed.size())
        {
            return nullptr;
        }

        return m_observed[_index];
    }

    // observed_count
    //   returns the number of observed (non-owning) children.
    size_type observed_count() const
    {
        read_guard guard(m_mutex);

        return m_observed.size();
    }

    // -----------------------------------------------------------------
    // mutex access
    // -----------------------------------------------------------------

    // mutex
    //   returns a reference to the underlying mutex for external
    // synchronization or use with portable_condvar.
    mutex_type& mutex() const noexcept
    {
        return m_mutex;
    }

private:
    base_type          m_counter;
    children_type      m_children;
    observed_type      m_observed;
    mutable mutex_type m_mutex;
};


NS_END  // util
NS_END  // djinterp


#endif  // DJINTERP_UTILITY_COUNTER_MUTEX_