/*******************************************************************************
* djinterp [cpp][utility]                               counter_tests_parity.cpp
*
* The counter suite's parity tests. The C++ counter is the C counter's
* counterpart, not a face over it (the owner's ruling of 2026-11-12), so the two
* are kept one behaviour by test: a counter<std::int64_t> and a struct
* dj_counter (c/util/counter/counter.h), given the same range and the same
* steps, must hold the same value after every step and report the same cuts.
*   One difference is by design and is not compared: an unbounded C counter is
* never at its minimum or maximum, while a default C++ counter, whose range is
* the whole of int64_t, is at both ends of it. A C counter bounded by that whole
* range is compared instead, and agrees.
*
*
* path:      /tests/djinterp/cpp/util/counter/counter_tests_parity.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/
// std
#include <cstddef>  // std::size_t
// djinterp
#include "counter_tests.hpp"  // the suite's declarations
#include "../../../../../inc/djinterp/c/util/counter/counter.h"  // dj_counter


NS_DJINTERP
NS_TESTING


namespace
{

// k_parity_values
//   value: the starts and amounts the parity tests try: both ends of the
// int64_t range and their neighbours, small steps either side of zero, and a
// step larger than any bounded range here.
const std::int64_t k_parity_values[] =
{
    k_lowest,
    k_lowest + 1,
    -1000,
    -7,
    -1,
    0,
    1,
    7,
    1000,
    k_highest - 1,
    k_highest
};

// k_parity_value_count
//   value: the number of entries in k_parity_values.
const std::size_t k_parity_value_count =
    (sizeof(k_parity_values) / sizeof(k_parity_values[0]));

// parity_range
//   struct: one range both counters are held in. `is_bounded` false is the C
// counter with no bounds, set by d_counter_init, against a default-ranged C++
// counter; `lower` and `upper` are then the whole of int64_t.
struct parity_range
{
    std::int64_t lower;
    std::int64_t upper;
    bool         is_bounded;
};

// k_parity_ranges
//   value: the ranges tried: none, the whole of int64_t, ranges about zero,
// a single value, a half range on either side, and two values at either end.
const parity_range k_parity_ranges[] =
{
    { k_lowest,      k_highest,    false },
    { k_lowest,      k_highest,    true  },
    { -1000,         1000,         true  },
    { -7,            7,            true  },
    { 0,             0,            true  },
    { 0,             k_highest,    true  },
    { k_lowest,      0,            true  },
    { k_highest - 1, k_highest,    true  },
    { k_lowest,      k_lowest + 1, true  }
};

// k_parity_range_count
//   value: the number of entries in k_parity_ranges.
const std::size_t k_parity_range_count =
    (sizeof(k_parity_ranges) / sizeof(k_parity_ranges[0]));

// parity_pair
//   struct: the two counters under comparison, built alike.
struct parity_pair
{
    DCtr              cpp;
    struct dj_counter c;
};

// parity_make
//   function: sets both counters at `_initial` in `_range`. Returns false
// when the C counter refuses the start, which a start inside the range never
// is.
bool
parity_make(
    parity_pair&        _pair,
    const parity_range& _range,
    std::int64_t        _initial
)
{
    _pair.cpp = DCtr(_initial,
                     _range.lower,
                     _range.upper);

    if (!_range.is_bounded)
    {
        d_counter_init(&_pair.c,
                       _initial);

        return true;
    }

    const struct d_interval_imax bounds =
        d_interval_imax_make(static_cast<d_math_imax>(_range.lower),
                             static_cast<d_math_imax>(_range.upper),
                             D_INTERVAL_CLOSED,
                             0);

    return d_counter_init_bounded(&_pair.c,
                                  _initial,
                                  &bounds);
}

// parity_step
//   function: one increment (`_is_decrement` false) or decrement of both
// counters by `_amount`. Returns true when both report the same cut.
bool
parity_step(
    parity_pair& _pair,
    bool         _is_decrement,
    std::int64_t _amount
)
{
    const bool cpp_exact = (_is_decrement)
                               ? _pair.cpp.decrement(_amount)
                               : _pair.cpp.increment(_amount);
    const bool c_exact   = (_is_decrement)
                               ? d_counter_decrement(&_pair.c, _amount)
                               : d_counter_increment(&_pair.c, _amount);

    return (cpp_exact == c_exact);
}

// parity_same
//   function: whether the two counters hold the same value and, where the C
// counter is bounded, sit at the same ends.
bool
parity_same(
    const parity_pair&  _pair,
    const parity_range& _range
)
{
    if (_pair.cpp.value() != d_counter_value(&_pair.c))
    {
        return false;
    }

    if (!_range.is_bounded)
    {
        return true;
    }

    return ( (_pair.cpp.at_min() == d_counter_at_min(&_pair.c)) &&
             (_pair.cpp.at_max() == d_counter_at_max(&_pair.c)) );
}

// parity_report
//   function: prints one disagreement, with what led to it.
void
parity_report(
    const char*         _what,
    const parity_range& _range,
    std::int64_t        _initial,
    const parity_pair&  _pair
)
{
    std::printf("    [parity] %s: range [%lld, %lld]%s, start %lld: "
                "C++ %lld, C %lld\n",
                _what,
                static_cast<long long>(_range.lower),
                static_cast<long long>(_range.upper),
                (_range.is_bounded) ? "" : " (C unbounded)",
                static_cast<long long>(_initial),
                static_cast<long long>(_pair.cpp.value()),
                static_cast<long long>(d_counter_value(&_pair.c)));

    return;
}

// parity_in_range
//   function: whether `_value` is a start `_range` accepts.
bool
parity_in_range(
    const parity_range& _range,
    std::int64_t        _value
)
{
    return ( (_range.lower <= _value) &&
             (_value <= _range.upper) );
}



// parity_step_by
//   function: step `_step` of the 2 x k_parity_value_count steps the tests
// try (the first half increments, the second decrements) on both counters.
void
parity_step_by(
    parity_pair& _pair,
    std::size_t  _step
)
{
    (void)parity_step(_pair,
                      (_step >= k_parity_value_count),
                      k_parity_values[_step % k_parity_value_count]);

    return;
}

// parity_merge_one
//   function: one merge case: a destination at `_start` in `_range`, moved by
// step `_dest_step`, takes the movement of a source started at value
// `_source_start` and moved by step `_source_step`. The atomic counter makes
// the same merge from the same state. Returns false, having reported it, when
// any of the three disagrees.
bool
parity_merge_one(
    const parity_range& _range,
    std::int64_t        _start,
    std::size_t         _dest_step,
    std::size_t         _source_start,
    std::size_t         _source_step
)
{
    const parity_range           whole = k_parity_ranges[0];
    parity_pair                  dest;
    parity_pair                  source;
    atomic_counter<std::int64_t> shared(_start,
                                        _range.lower,
                                        _range.upper);

    if ( (!parity_make(dest, _range, _start)) ||
         (!parity_make(source, whole, k_parity_values[_source_start])) )
    {
        return false;
    }

    parity_step_by(dest, _dest_step);
    parity_step_by(source, _source_step);

    // the atomic counter, stepped as dest was
    if (_dest_step >= k_parity_value_count)
    {
        (void)shared.decrement(
            k_parity_values[_dest_step % k_parity_value_count]);
    }
    else
    {
        (void)shared.increment(k_parity_values[_dest_step]);
    }

    const bool shared_exact = shared.merge(source.cpp);
    const bool cpp_exact    = dest.cpp.merge(source.cpp);
    const bool c_exact      = d_counter_merge(&dest.c, &source.c);

    if ( (cpp_exact != c_exact)                        ||
         (shared_exact != c_exact)                     ||
         (shared.value() != d_counter_value(&dest.c))  ||
         (!parity_same(dest, _range)) )
    {
        parity_report("merge", _range, _start, dest);

        return false;
    }

    return true;
}

// parity_merge_self
//   function: a counter at `_start` in `_range`, moved by step `_step`,
// merged into itself. Returns false, having reported it, when the two
// disagree.
bool
parity_merge_self(
    const parity_range& _range,
    std::int64_t        _start,
    std::size_t         _step
)
{
    parity_pair self;

    if (!parity_make(self, _range, _start))
    {
        return false;
    }

    parity_step_by(self, _step);

    const bool cpp_exact = self.cpp.merge(self.cpp);
    const bool c_exact   = d_counter_merge(&self.c, &self.c);

    if ( (cpp_exact != c_exact) ||
         (!parity_same(self, _range)) )
    {
        parity_report("merge into itself", _range, _start, self);

        return false;
    }

    return true;
}

}  // namespace


/*
tests_counter_parity_steps
  Two steps from every start, against the C counter.
  Tests the following:
  - for every range, every start inside it, and every pair of steps (each an
    increment or a decrement by any of the values), both counters report the
    same cut at each step
  - and hold the same value afterwards, at the same ends of the range
*/
bool tests_counter_parity_steps()
{
    std::size_t r = 0;

    for (r = 0; r < k_parity_range_count; ++r)
    {
        const parity_range& range = k_parity_ranges[r];
        std::size_t         s     = 0;

        for (s = 0; s < k_parity_value_count; ++s)
        {
            const std::int64_t start = k_parity_values[s];
            std::size_t        step  = 0;

            if (!parity_in_range(range, start))
            {
                continue;
            }

            // 2 operations x the values, twice over
            for (step = 0; step < (4u * k_parity_value_count *
                                   k_parity_value_count); ++step)
            {
                const std::size_t first  = (step / (2u * k_parity_value_count));
                const std::size_t second = (step % (2u * k_parity_value_count));
                parity_pair       pair;

                D_CTR_CHECK(parity_make(pair, range, start));

                if ( (!parity_step(pair,
                                   (first >= k_parity_value_count),
                                   k_parity_values[first %
                                                   k_parity_value_count])) ||
                     (!parity_same(pair, range)) ||
                     (!parity_step(pair,
                                   (second >= k_parity_value_count),
                                   k_parity_values[second %
                                                   k_parity_value_count])) ||
                     (!parity_same(pair, range)) )
                {
                    parity_report("steps", range, start, pair);

                    return false;
                }
            }
        }
    }

    return true;
}

/*
tests_counter_parity_merge
  Merging another counter's movement, against the C counter.
  Tests the following:
  - a destination in every range, moved by one step, takes a source's
    movement (the source started anywhere, then moved by one step) with the
    same cut and to the same value as the C counter does, and so does an
    atomic_counter in the same state
  - a counter merged into itself agrees too
*/
bool tests_counter_parity_merge()
{
    const std::size_t steps = (2u * k_parity_value_count);
    std::size_t       r     = 0;

    for (r = 0; r < k_parity_range_count; ++r)
    {
        const parity_range& range = k_parity_ranges[r];
        std::size_t         s     = 0;

        for (s = 0; s < k_parity_value_count; ++s)
        {
            const std::int64_t start = k_parity_values[s];
            std::size_t        d     = 0;

            if (!parity_in_range(range, start))
            {
                continue;
            }

            for (d = 0; d < steps; ++d)
            {
                std::size_t o = 0;

                for (o = 0; o < (k_parity_value_count * steps); ++o)
                {
                    D_CTR_CHECK(parity_merge_one(range,
                                                 start,
                                                 d,
                                                 (o / steps),
                                                 (o % steps)));
                }

                D_CTR_CHECK(parity_merge_self(range, start, d));
            }
        }
    }

    return true;
}

/*
tests_counter_parity_compare_reset
  Ordering and reset, against the C counter.
  Tests the following:
  - any two values order the same way in both
  - a reset after a step returns both to the start
*/
bool tests_counter_parity_compare_reset()
{
    std::size_t a = 0;

    for (a = 0; a < k_parity_value_count; ++a)
    {
        std::size_t b = 0;

        for (b = 0; b < k_parity_value_count; ++b)
        {
            const parity_range whole = k_parity_ranges[1];
            parity_pair        left;
            parity_pair        right;

            D_CTR_CHECK(parity_make(left, whole, k_parity_values[a]));
            D_CTR_CHECK(parity_make(right, whole, k_parity_values[b]));

            D_CTR_CHECK(left.cpp.compare(right.cpp) ==
                        d_counter_compare(&left.c, &right.c));

            // a step, then a reset
            (void)parity_step(left, false, k_parity_values[b]);

            left.cpp.reset();
            d_counter_reset(&left.c);

            D_CTR_CHECK(left.cpp.value() == k_parity_values[a]);
            D_CTR_CHECK(d_counter_value(&left.c) == k_parity_values[a]);
        }
    }

    return true;
}


NS_END  // testing
NS_END  // djinterp
