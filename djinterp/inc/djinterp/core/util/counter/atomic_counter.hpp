/******************************************************************************
* djinterp [util]                                           atomic_counter.hpp
*
* Lock-free atomic counter with bounded increment/decrement.
*   Uses std::atomic with CAS loops to enforce [min, max] bounds without
* any mutex.  Suitable for high-contention counters where lock-free
* progress is required (e.g. reference counts, connection limits,
* concurrent work-item tracking).
*
*   Unlike mutex_counter, atomic_counter does NOT support children
* (owning or observed).  Nesting requires coordination that cannot be
* expressed lock-free.  If nesting is needed, use mutex_counter instead.
*
* DESIGN:
*   All operations use compare_exchange_weak CAS loops with configurable
* memory ordering.  The default ordering is acquire-release, which is
* correct for most producer-consumer patterns.  Callers may override
* for relaxed counting or sequentially-consistent fences.
*
* RELATIONSHIP TO atomic_size:
*   atomic_size (in container::threadsafe) is an unbounded semantic
* wrapper around std::atomic<size_t>.  atomic_counter adds bounded
* clamping, an initial-value reset, and supports signed types.
*
* SEE ALSO:
*   counter.hpp        — unsynchronized base
*   mutex_counter.hpp  — lock-policy-based variant (supports children)
*   atomic.hpp         — atomic_size, atomic_version (container module)
*
*   PORTABILITY:
*   Requires C++11 or later.  C++20 adds wait/notify support.
*
*
* path:      /inc/djinterp/util/counter/atomic_counter.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.07
******************************************************************************/

#ifndef DJINTERP_UTILITY_COUNTER_ATOMIC_
#define DJINTERP_UTILITY_COUNTER_ATOMIC_ 1

#include <atomic>
#include <cstdint>
#include <limits>
#include <type_traits>
#include "../../djinterp.hpp"


NS_DJINTERP
NS_UTIL


// =========================================================================
// atomic_counter
//   class: a lock-free bounded counter using std::atomic with CAS loops.
//
//   Increment and decrement clamp at the configured [min, max] bounds
// and return false when a limit prevents the full operation.  All
// operations are wait-free when uncontended and lock-free under
// contention (CAS retry).
//
//   Does NOT support children or nesting.  Use mutex_counter for
// hierarchical counters.
//
//   Template parameter `_ValueType` must be an integral type
// (floating-point atomics lack the required CAS semantics on most
// platforms).
// =========================================================================
template<typename _ValueType = std::int64_t>
class atomic_counter
{
    static_assert(std::is_integral_v<_ValueType>,
                  "`_ValueType` must be an integral type.");

public:
    using value_type = _ValueType;

    // --- policy descriptors (for trait compatibility) ---
    static constexpr bool is_threadsafe = true;

    // -----------------------------------------------------------------
    // constructors
    // -----------------------------------------------------------------

    // atomic_counter()
    //   constructor: default-constructs at zero with no bounds.
    atomic_counter() noexcept
        : m_value(value_type{0}),
          m_initial(value_type{0}),
          m_min(std::numeric_limits<value_type>::lowest()),
          m_max(std::numeric_limits<value_type>::max())
    {
    }

    // atomic_counter(_initial, _min, _max)
    //   constructor: constructs with an initial value and optional
    // min/max bounds.
    atomic_counter(
		value_type _initial,
		value_type _min = std::numeric_limits<value_type>::lowest(),
		value_type _max = std::numeric_limits<value_type>::max()
	) noexcept
        : m_value(_initial),
          m_initial(_initial),
          m_min(_min),
          m_max(_max)
    {
    }

    // non-copyable (atomics are non-copyable)
    atomic_counter(const atomic_counter&)            = delete;
    atomic_counter& operator=(const atomic_counter&) = delete;

    // -----------------------------------------------------------------
    // operations
    // -----------------------------------------------------------------

    // increment
    //   atomically increments by `_amount`. returns false and clamps
    // to max if the operation would exceed the upper bound.
    //
    // uses a CAS loop: loads the current value, computes the desired
    // value (clamped), and attempts to swap.  Retries on contention.
    bool increment(
		value_type        _amount  = value_type{1},
		std::memory_order _success = std::memory_order_acq_rel,
		std::memory_order _failure = std::memory_order_acquire
	) noexcept
    {
        value_type current = m_value.load(
                                 std::memory_order_relaxed);

        for (;;)
        {
            // would exceed upper bound
            if (current + _amount > m_max)
            {
                // already at max — nothing to do
                if (current >= m_max)
                {
                    return false;
                }

                // clamp to max
                if (m_value.compare_exchange_weak(
                        current, m_max,
                        _success, _failure))
                {
                    return false;
                }

                // CAS failed — retry with updated current
                continue;
            }

            // normal increment
            if (m_value.compare_exchange_weak(
                    current, current + _amount,
                    _success, _failure))
            {
                return true;
            }

            // CAS failed — current was reloaded, retry
        }
    }

    // decrement
    //   atomically decrements by `_amount`. returns false and clamps
    // to min if the operation would exceed the lower bound.
    bool decrement(
		value_type        _amount  = value_type{1},
		std::memory_order _success = std::memory_order_acq_rel,
		std::memory_order _failure = std::memory_order_acquire
	) noexcept
    {
        value_type current = m_value.load(
                                 std::memory_order_relaxed);

        for (;;)
        {
            // would exceed lower bound
            if (current - _amount < m_min)
            {
                // already at min — nothing to do
                if (current <= m_min)
                {
                    return false;
                }

                // clamp to min
                if (m_value.compare_exchange_weak(
                        current, m_min,
                        _success, _failure))
                {
                    return false;
                }

                continue;
            }

            // normal decrement
            if (m_value.compare_exchange_weak(
                    current, current - _amount,
                    _success, _failure))
            {
                return true;
            }
        }
    }

    // fetch_increment
    //   atomically increments and returns the value BEFORE the
    // increment.  Returns the clamped-to value if the bound was hit.
    // `_succeeded` is set to false if clamping occurred.
    value_type fetch_increment(
		bool&             _succeeded,
		value_type        _amount  = value_type{1},
		std::memory_order _success = std::memory_order_acq_rel,
		std::memory_order _failure = std::memory_order_acquire
	) noexcept
    {
        value_type current = m_value.load(
                                 std::memory_order_relaxed);

        for (;;)
        {
            if (current + _amount > m_max)
            {
                if (current >= m_max)
                {
                    _succeeded = false;

                    return current;
                }

                if (m_value.compare_exchange_weak(
                        current, m_max,
                        _success, _failure))
                {
                    _succeeded = false;

                    return current;
                }

                continue;
            }

            if (m_value.compare_exchange_weak(
                    current, current + _amount,
                    _success, _failure))
            {
                _succeeded = true;

                return current;
            }
        }
    }

    // fetch_decrement
    //   atomically decrements and returns the value BEFORE the
    // decrement.  `_succeeded` is set to false if clamping occurred.
    value_type fetch_decrement(
		bool&             _succeeded,
		value_type        _amount  = value_type{1},
		std::memory_order _success = std::memory_order_acq_rel,
		std::memory_order _failure = std::memory_order_acquire
	) noexcept
    {
        value_type current = m_value.load(
                                 std::memory_order_relaxed);

        for (;;)
        {
            if (current - _amount < m_min)
            {
                if (current <= m_min)
                {
                    _succeeded = false;

                    return current;
                }

                if (m_value.compare_exchange_weak(
                        current, m_min,
                        _success, _failure))
                {
                    _succeeded = false;

                    return current;
                }

                continue;
            }

            if (m_value.compare_exchange_weak(
                    current, current - _amount,
                    _success, _failure))
            {
                _succeeded = true;

                return current;
            }
        }
    }

    // reset
    //   atomically stores the initial value.
    void reset(
		std::memory_order _order =
			std::memory_order_release
	) noexcept
    {
        m_value.store(m_initial, _order);

        return;
    }

    // -----------------------------------------------------------------
    // accessors
    // -----------------------------------------------------------------

    // load
    //   returns the current counter value.
    value_type load(
		std::memory_order _order =
			std::memory_order_acquire
	) const noexcept
    {
        return m_value.load(_order);
    }

    // value
    //   convenience alias for load() with default ordering.
    value_type value() const noexcept
    {
        return m_value.load(std::memory_order_acquire);
    }

    // initial
    //   returns the initial value the counter was constructed with.
    // immutable after construction — no synchronization needed.
    constexpr value_type initial() const noexcept
    {
        return m_initial;
    }

    // min
    //   returns the lower bound.
    // immutable after construction — no synchronization needed.
    constexpr value_type min() const noexcept
    {
        return m_min;
    }

    // max
    //   returns the upper bound.
    // immutable after construction — no synchronization needed.
    constexpr value_type max() const noexcept
    {
        return m_max;
    }

    // at_min
    //   returns true if the counter is at or below its lower bound.
    bool at_min(
		std::memory_order _order =
			std::memory_order_acquire
	) const noexcept
    {
        return (m_value.load(_order) <= m_min);
    }

    // at_max
    //   returns true if the counter is at or above its upper bound.
    bool at_max(
		std::memory_order _order =
			std::memory_order_acquire
	) const noexcept
    {
        return (m_value.load(_order) >= m_max);
    }

    // -----------------------------------------------------------------
    // CAS (for advanced use)
    // -----------------------------------------------------------------

    // compare_exchange_weak
    //   raw CAS passthrough for patterns not covered by
    // increment/decrement (e.g. conditional set).
    bool compare_exchange_weak(
		value_type&       _expected,
		value_type        _desired,
		std::memory_order _success = std::memory_order_acq_rel,
		std::memory_order _failure = std::memory_order_acquire
	) noexcept
    {
        return m_value.compare_exchange_weak(
            _expected, _desired, _success, _failure);
    }

    // compare_exchange_strong
    //   strong CAS passthrough.
    bool compare_exchange_strong(
		value_type&       _expected,
		value_type        _desired,
		std::memory_order _success = std::memory_order_acq_rel,
		std::memory_order _failure = std::memory_order_acquire
	) noexcept
    {
        return m_value.compare_exchange_strong(
            _expected, _desired, _success, _failure);
    }

    // -----------------------------------------------------------------
    // conversion
    // -----------------------------------------------------------------

    operator value_type() const noexcept
    {
        return m_value.load(std::memory_order_acquire);
    }

    // -----------------------------------------------------------------
    // C++20 wait / notify
    // -----------------------------------------------------------------

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    // wait
    //   blocks until the value differs from `_old`.
    void wait(
		value_type        _old,
		std::memory_order _order = std::memory_order_acquire
	) const noexcept
    {
        m_value.wait(_old, _order);
    }

    // notify_one
    //   unblocks one thread waiting on this counter.
    void notify_one() noexcept
    {
        m_value.notify_one();
    }

    // notify_all
    //   unblocks all threads waiting on this counter.
    void notify_all() noexcept
    {
        m_value.notify_all();
    }

#endif  // C++20

private:
    std::atomic<value_type> m_value;
    const value_type        m_initial;
    const value_type        m_min;
    const value_type        m_max;
};


NS_END  // util
NS_END  // djinterp


#endif  // DJINTERP_UTILITY_COUNTER_ATOMIC_