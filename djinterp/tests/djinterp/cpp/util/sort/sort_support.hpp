/*******************************************************************************
* djinterp [test]                                               sort_support.hpp
*
*   shared support for the sort subsystem unit-test suites.
* This header is infrastructure, not a suite: it carries no tests_* functions
* of its own.  Everything the eleven sort suites need in common lives here so
* that each suite header stays a declaration list plus the few adapters that
* are specific to its module.
*
*   Nothing here uses a test framework assertion macro.  A DTest unit test is
* a plain bool() function, so the checks are accumulated by `verdict`, which
* is deliberately NON short-circuiting: every check in a test runs and is
* counted even after one has failed, so a single run reports the full damage
* rather than the first symptom.
*
*   Determinism is a hard requirement -- a coverage run has to be
* reproducible -- so the randomised inputs come from a local
* linear-congruential generator rather than <random> (whose distributions are
* not specified bit-for-bit) or std::rand (whose sequence is implementation
* defined).
*
*   contents:
*     I.    verdict                     (non-short-circuiting accumulator)
*     II.   element types               (DKeyedValue, DTrackedInt)
*     III.  instrumented comparators    (DCountingComparator,
*                                        DAffirmationCounter)
*     IV.   range predicates            (ordering / permutation / stability)
*     V.    deterministic generators    (lcg + the ten input shapes)
*     VI.   input batteries             (the shared corpus)
*     VII.  battery drivers             (the reference-oracle harness)
*     VIII. ordered-entry probe         (SFINAE around the _ordered shape)
*
*
* path:      /tests/djinterp/cpp/util/sort/sort_support.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.11.12
*******************************************************************************/

#ifndef DJINTERP_TESTS_SORT_SUPPORT_
#define DJINTERP_TESTS_SORT_SUPPORT_ 1

// std
#include <algorithm>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../../../../inc/djinterp/cpp/djinterp.hpp"  // framework root
// re_std
#include "../../../../../inc/re_std/cpp/cstdint/cstdint.hpp"  // uint32_t


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                    I.   VERDICT                                         ///
///////////////////////////////////////////////////////////////////////////////

// verdict
//   class: a non-short-circuiting conjunction accumulator.  check() records a
// condition and returns it unchanged, so a test may branch on a check while
// still contributing it to the aggregate.  ok() is the value a tests_*
// function returns; checks() and failures() are available for diagnostics.
class verdict
{
public:
    // verdict
    //   ctor: an empty verdict -- vacuously true.
    verdict()
        : m_ok(true),
          m_checks(0),
          m_failures(0)
    {
    }

    // check
    //   function: records _condition and returns it unchanged.
    bool check(
        bool _condition
    )
    {
        ++m_checks;

        if (!_condition)
        {
            m_ok = false;
            ++m_failures;
        }

        return _condition;
    }

    // ok
    //   accessor: true when every recorded check passed.
    bool ok() const
    {
        return m_ok;
    }

    // checks
    //   accessor: the number of checks recorded.
    std::size_t checks() const
    {
        return m_checks;
    }

    // failures
    //   accessor: the number of recorded checks that failed.
    std::size_t failures() const
    {
        return m_failures;
    }

private:
    bool        m_ok;
    std::size_t m_checks;
    std::size_t m_failures;
};


///////////////////////////////////////////////////////////////////////////////
///                    II.  ELEMENT TYPES                                   ///
///////////////////////////////////////////////////////////////////////////////

// DKeyedValue
//   struct: the element type used to observe stability.  `key` is what the
// sort orders by; `tag` records the element's position in the input, so a
// stable result leaves tags ascending inside every equal-key group.
struct DKeyedValue
{
    int key;
    int tag;
};

// make_keyed
//   function: builds a DKeyedValue without relying on aggregate
// initialisation at the call site.
inline
DKeyedValue make_keyed(
    int _key,
    int _tag
)
{
    DKeyedValue result;

    result.key = _key;
    result.tag = _tag;

    return result;
}

// operator<
//   function: lexicographic ordering on (key, tag).  This is a TOTAL order,
// used only by the permutation oracle -- never as the sort comparator, since
// a total order would make stability unobservable.
inline
bool operator<(
    const DKeyedValue& _a,
    const DKeyedValue& _b
)
{
    if (_a.key != _b.key)
    {
        return (_a.key < _b.key);
    }

    return (_a.tag < _b.tag);
}

// operator==
//   function: memberwise equality.
inline
bool operator==(
    const DKeyedValue& _a,
    const DKeyedValue& _b
)
{
    return ( (_a.key == _b.key) &&
             (_a.tag == _b.tag) );
}

// operator!=
//   function: the negation of memberwise equality.
inline
bool operator!=(
    const DKeyedValue& _a,
    const DKeyedValue& _b
)
{
    return (!(_a == _b));
}

// keyed_value_less
//   struct: ascending comparator on the KEY ALONE.  Equal keys are
// equivalent under this ordering, which is precisely what makes a stable
// algorithm distinguishable from an unstable one.
struct keyed_value_less
{
    bool operator()(
        const DKeyedValue& _a,
        const DKeyedValue& _b
    ) const
    {
        return (_a.key < _b.key);
    }
};

// DKeyedValueGreater
//   struct: descending comparator on the key alone.
struct DKeyedValueGreater
{
    bool operator()(
        const DKeyedValue& _a,
        const DKeyedValue& _b
    ) const
    {
        return (_a.key > _b.key);
    }
};

// DTrackedInt
//   struct: an int carrying a pointer to a shared swap counter.  The counter
// is incremented by the ADL-found swap() below, which std::iter_swap resolves
// to, so an algorithm's swap traffic can be measured without touching the
// algorithm.
struct DTrackedInt
{
    int          value;
    std::size_t* swaps;
};

// make_tracked
//   function: builds a DTrackedInt bound to _counter.
inline
DTrackedInt make_tracked(
    int          _value,
    std::size_t* _counter
)
{
    DTrackedInt result;

    result.value = _value;
    result.swaps = _counter;

    return result;
}

// swap
//   function: the ADL customisation point std::iter_swap resolves to for
// DTrackedInt.  It performs the exchange and then bumps the shared counter
// once.
//
//   The counter is read BEFORE the exchange: the two operands normally share
// one counter, but reading first keeps the accounting correct even when they
// do not.
inline
void swap(
    DTrackedInt& _a,
    DTrackedInt& _b
)
{
    std::size_t* counter;
    int          held_value;
    std::size_t* held_swaps;

    counter = _a.swaps;

    held_value = _a.value;
    held_swaps = _a.swaps;
    _a.value   = _b.value;
    _a.swaps   = _b.swaps;
    _b.value   = held_value;
    _b.swaps   = held_swaps;

    if (counter != nullptr)
    {
        ++(*counter);
    }

    return;
}

// tracked_int_less
//   struct: ascending comparator on a DTrackedInt's value.
struct tracked_int_less
{
    bool operator()(
        const DTrackedInt& _a,
        const DTrackedInt& _b
    ) const
    {
        return (_a.value < _b.value);
    }
};

// tracked_values
//   function: projects a DTrackedInt sequence onto its plain values, so the
// ordering can be checked with the ordinary int predicates.
inline
std::vector<int> tracked_values(
    const std::vector<DTrackedInt>& _data
)
{
    std::vector<int> result;
    std::size_t      i;

    result.reserve(_data.size());

    for (i = 0; i < _data.size(); ++i)
    {
        result.push_back(_data[i].value);
    }

    return result;
}


///////////////////////////////////////////////////////////////////////////////
///                    III. INSTRUMENTED COMPARATORS                        ///
///////////////////////////////////////////////////////////////////////////////

// DCountingComparator
//   class: wraps a comparator and counts the calls made through it.
//
//   The counter is held BY POINTER rather than by value because every sort
// entry point in the subsystem takes its comparator BY VALUE and copies it
// again into DOrderComparator: a member counter would be incremented on a
// copy the caller cannot see.  An external counter survives every copy.
template<typename Compare>
class DCountingComparator
{
public:
    // DCountingComparator
    //   ctor: wraps _compare and bumps *_counter on every call.
    DCountingComparator(
        const Compare& _compare,
        std::size_t*   _counter
    )
        : m_compare(_compare),
          m_counter(_counter)
    {
    }

    // operator()
    //   function: records the call, then forwards to the wrapped comparator.
    template<typename Left,
             typename Right>
    bool operator()(
        const Left& _a,
        const Right& _b
    ) const
    {
        if (m_counter != nullptr)
        {
            ++(*m_counter);
        }

        return m_compare(_a, _b);
    }

private:
    Compare     m_compare;
    std::size_t* m_counter;
};

// make_counting
//   function: deduces Compare and builds a DCountingComparator sharing
// _counter.
template<typename Compare>
DCountingComparator<Compare> make_counting(
    Compare     _compare,
    std::size_t* _counter
)
{
    return DCountingComparator<Compare>(_compare, _counter);
}

// DAffirmationCounter
//   class: wraps a comparator and counts the calls that answered TRUE -- the
// precedences the comparator affirmed.
//
//   An exchange-based algorithm moves elements exactly when its comparator
// affirms a strict precedence, so this count measures its exchange traffic
// whether the exchange is a swap or the carried-element assignment the
// bubble and insertion primitives use.  A swap counter sees only the first.
// The counter is held by pointer for the reason DCountingComparator's is.
template<typename Compare>
class DAffirmationCounter
{
public:
    // DAffirmationCounter
    //   ctor: wraps _compare and bumps *_counter on every TRUE answer.
    DAffirmationCounter(
        const Compare& _compare,
        std::size_t*   _counter
    )
        : m_compare(_compare),
          m_counter(_counter)
    {
    }

    // operator()
    //   function: forwards to the wrapped comparator and records a TRUE.
    template<typename Left,
             typename Right>
    bool operator()(
        const Left&  _left,
        const Right& _right
    ) const
    {
        bool answer;

        answer = m_compare(_left, _right);

        if ( (answer) &&
             (m_counter != nullptr) )
        {
            ++(*m_counter);
        }

        return answer;
    }

private:
    Compare      m_compare;
    std::size_t* m_counter;
};

// make_affirmation_counter
//   function: deduces Compare and builds a DAffirmationCounter sharing
// _counter.
template<typename Compare>
DAffirmationCounter<Compare> make_affirmation_counter(
    Compare      _compare,
    std::size_t* _counter
)
{
    return DAffirmationCounter<Compare>(_compare, _counter);
}


///////////////////////////////////////////////////////////////////////////////
///                    IV.  RANGE PREDICATES                                ///
///////////////////////////////////////////////////////////////////////////////

// is_sorted_range
//   function: true when no element in [_first, _last) strictly precedes its
// predecessor under _compare.  Hand-rolled rather than std::is_sorted so the
// suites never depend on the standard algorithm they are validating against.
template<typename Iterator,
         typename Compare>
bool is_sorted_range(
    Iterator _first,
    Iterator _last,
    Compare  _compare
)
{
    Iterator previous;
    Iterator current;

    if (_first == _last)
    {
        return true;
    }

    previous = _first;
    current  = _first;
    ++current;

    for (; current != _last; ++previous, ++current)
    {
        // an inversion: the successor strictly precedes its predecessor
        if (_compare(*current, *previous))
        {
            return false;
        }
    }

    return true;
}

// is_permutation_of
//   function: true when _a and _b hold the same elements with the same
// multiplicities.  The oracle sorts copies of both with the standard library
// and compares -- an independent implementation, so a bug in the subsystem
// cannot mask itself.
template<typename Type>
bool is_permutation_of(
    const std::vector<Type>& _a,
    const std::vector<Type>& _b
)
{
    std::vector<Type> left;
    std::vector<Type> right;

    if (_a.size() != _b.size())
    {
        return false;
    }

    left  = _a;
    right = _b;

    std::sort(left.begin(), left.end());
    std::sort(right.begin(), right.end());

    return (left == right);
}

// ranges_equal
//   function: elementwise equality of two vectors.
template<typename Type>
bool ranges_equal(
    const std::vector<Type>& _a,
    const std::vector<Type>& _b
)
{
    return (_a == _b);
}

// is_stable_result
//   function: true when every equal-key group in _values carries ascending
// tags.  Valid only on a result already known to be ordered by key, where
// equal keys are contiguous and consecutive pairs therefore cover each group
// completely.
inline
bool is_stable_result(
    const std::vector<DKeyedValue>& _values
)
{
    std::size_t i;

    if (_values.size() < 2)
    {
        return true;
    }

    for (i = 1; i < _values.size(); ++i)
    {
        // two equivalents that swapped places: the sort is not stable
        if ( (_values[i].key == _values[i - 1].key) &&
             (_values[i].tag <  _values[i - 1].tag) )
        {
            return false;
        }
    }

    return true;
}

// is_max_heap
//   function: true when [_first, _last) satisfies the max-heap invariant
// under _compare -- no parent strictly precedes a child.  Used to validate
// heap_sift_down in isolation from the full sort.  The range spelling matches
// is_sorted_range above, so the heap suites probe a shrinking heap the same
// way every other predicate here takes a range.
template<typename Iterator,
         typename Compare>
bool is_max_heap(
    Iterator _first,
    Iterator _last,
    Compare  _compare
)
{
    std::ptrdiff_t size;
    std::ptrdiff_t parent;
    std::ptrdiff_t child;

    size = static_cast<std::ptrdiff_t>(_last - _first);

    for (parent = 0; parent < size; ++parent)
    {
        child = ((2 * parent) + 1);

        if ( (child < size) &&
             (_compare(*(_first + parent), *(_first + child))) )
        {
            return false;
        }

        ++child;

        if ( (child < size) &&
             (_compare(*(_first + parent), *(_first + child))) )
        {
            return false;
        }
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
///                    V.   DETERMINISTIC GENERATORS                        ///
///////////////////////////////////////////////////////////////////////////////

// lcg
//   class: a 32-bit linear-congruential generator (Numerical Recipes
// constants).  Small, dependency-free and bit-for-bit reproducible on every
// platform, which <random>'s distributions and std::rand are not.
class DLcg
{
public:
    // lcg
    //   ctor: seeds the generator.
    explicit DLcg(
        re_std::uint32_t _seed
    )
        : m_state(_seed)
    {
    }

    // next
    //   function: advances the state and returns it.
    re_std::uint32_t next()
    {
        m_state = ((m_state * 1664525u) + 1013904223u);

        return m_state;
    }

    // next_below
    //   function: returns a value in [0, _bound).  The high bits are used
    // because the low bits of an LCG have short periods.
    int next_below(
        int _bound
    )
    {
        re_std::uint32_t drawn;

        if (_bound <= 0)
        {
            return 0;
        }

        drawn = (next() >> 16);

        return static_cast<int>(drawn % static_cast<re_std::uint32_t>(_bound));
    }

private:
    re_std::uint32_t m_state;
};

// battery_shape_count
//   constant: the number of distinct input shapes in the standard battery.
const std::size_t battery_shape_count = 10;

// shape_ascending
//   function: 0, 1, 2, ... -- the adaptive best case, and the input on which
// an early-exit pass must terminate after a single sweep.
inline
std::vector<int> shape_ascending(
    std::size_t _size
)
{
    std::vector<int> data;
    std::size_t      i;

    data.reserve(_size);

    for (i = 0; i < _size; ++i)
    {
        data.push_back(static_cast<int>(i));
    }

    return data;
}

// shape_descending
//   function: the strict reverse -- the quadratic worst case, and the input
// that forces the maximum number of swaps.
inline
std::vector<int> shape_descending(
    std::size_t _size
)
{
    std::vector<int> data;
    std::size_t      i;

    data.reserve(_size);

    for (i = 0; i < _size; ++i)
    {
        data.push_back(static_cast<int>(_size - i));
    }

    return data;
}

// shape_constant
//   function: every element identical.  Exercises the all-equivalent
// partition in quicksort and the never-swap path everywhere else.
inline
std::vector<int> shape_constant(
    std::size_t _size
)
{
    std::vector<int> data;
    std::size_t      i;

    data.reserve(_size);

    for (i = 0; i < _size; ++i)
    {
        data.push_back(7);
    }

    return data;
}

// shape_two_valued
//   function: alternating 0 and 1 -- maximal duplication with a real
// inversion count.
inline
std::vector<int> shape_two_valued(
    std::size_t _size
)
{
    std::vector<int> data;
    std::size_t      i;

    data.reserve(_size);

    for (i = 0; i < _size; ++i)
    {
        data.push_back(static_cast<int>(i % 2));
    }

    return data;
}

// shape_few_unique
//   function: four distinct values in a scrambled order.
inline
std::vector<int> shape_few_unique(
    std::size_t _size
)
{
    std::vector<int> data;
    std::size_t      i;

    data.reserve(_size);

    for (i = 0; i < _size; ++i)
    {
        data.push_back(static_cast<int>((i * 7) % 4));
    }

    return data;
}

// shape_sawtooth
//   function: repeated ascending runs of sixteen -- many already-ordered
// stretches separated by single large drops.
inline
std::vector<int> shape_sawtooth(
    std::size_t _size
)
{
    std::vector<int> data;
    std::size_t      i;

    data.reserve(_size);

    for (i = 0; i < _size; ++i)
    {
        data.push_back(static_cast<int>(i % 16));
    }

    return data;
}

// shape_organ_pipe
//   function: ascending to the midpoint then descending -- the median of
// three lands on the maximum, the pathological pivot choice.
inline
std::vector<int> shape_organ_pipe(
    std::size_t _size
)
{
    std::vector<int> data;
    std::size_t      i;

    data.reserve(_size);

    for (i = 0; i < _size; ++i)
    {
        if (i < (_size / 2))
        {
            data.push_back(static_cast<int>(i));
        }
        else
        {
            data.push_back(static_cast<int>(_size - i));
        }
    }

    return data;
}

// shape_nearly_sorted
//   function: ascending with every eighth adjacent pair transposed --
// the input an adaptive algorithm must still finish quickly, and the one
// that exercises a bubble pass whose last swap is far from the end.
inline
std::vector<int> shape_nearly_sorted(
    std::size_t _size
)
{
    std::vector<int> data;
    std::size_t      i;
    int              held;

    data = shape_ascending(_size);

    for (i = 0; (i + 1) < _size; i += 8)
    {
        held            = data[i];
        data[i]         = data[i + 1];
        data[i + 1]     = held;
    }

    return data;
}

// shape_random
//   function: pseudo-random values drawn from the local generator, with a
// deliberately small range so duplicates are common.
inline
std::vector<int> shape_random(
    std::size_t _size
)
{
    std::vector<int> data;
    std::size_t      i;
    DLcg              source(2166136261u);

    data.reserve(_size);

    for (i = 0; i < _size; ++i)
    {
        data.push_back(source.next_below(1000));
    }

    return data;
}

// shape_single_outlier
//   function: ascending with the minimum parked at the very end.  One
// element has to travel the whole range: the maximal-shift case for
// insertion sort and the maximal-distance case for a bubble pass.
inline
std::vector<int> shape_single_outlier(
    std::size_t _size
)
{
    std::vector<int> data;

    data = shape_ascending(_size);

    if (!data.empty())
    {
        data[data.size() - 1] = -1;
    }

    return data;
}

// shape_name
//   function: the identifying name of shape index _shape.
inline
const char* shape_name(
    std::size_t _shape
)
{
    static const char* const names[] =
    {
        "ascending",
        "descending",
        "constant",
        "two_valued",
        "few_unique",
        "sawtooth",
        "organ_pipe",
        "nearly_sorted",
        "random",
        "single_outlier"
    };

    if (_shape >= battery_shape_count)
    {
        return "unknown";
    }

    return names[_shape];
}

// make_shape
//   function: builds shape _shape at length _size.
inline
std::vector<int> make_shape(
    std::size_t _shape,
    std::size_t _size
)
{
    switch (_shape)
    {
        case 0:
        {
            return shape_ascending(_size);
        }

        case 1:
        {
            return shape_descending(_size);
        }

        case 2:
        {
            return shape_constant(_size);
        }

        case 3:
        {
            return shape_two_valued(_size);
        }

        case 4:
        {
            return shape_few_unique(_size);
        }

        case 5:
        {
            return shape_sawtooth(_size);
        }

        case 6:
        {
            return shape_organ_pipe(_size);
        }

        case 7:
        {
            return shape_nearly_sorted(_size);
        }

        case 8:
        {
            return shape_random(_size);
        }

        case 9:
        {
            return shape_single_outlier(_size);
        }

        default:
        {
            break;
        }
    }

    return shape_ascending(_size);
}


///////////////////////////////////////////////////////////////////////////////
///                    VI.  INPUT BATTERIES                                 ///
///////////////////////////////////////////////////////////////////////////////

// DBatteryCase
//   struct: one input in the standard corpus, carrying enough identity to
// name the failure it produces.
struct DBatteryCase
{
    const char*      shape;
    std::size_t      size;
    std::vector<int> data;
};

// battery_size_count
//   constant: the number of lengths in the standard battery.
const std::size_t battery_size_count = 17;

// battery_size_at
//   function: the length at index _index of the standard battery.
//
//   The ladder is not arbitrary.  0/1/2/3 are the degenerate and minimal
// ranges; 16/17/18 straddle quicksort's default insertion threshold, so it is
// observed both below and above its cut-off; 32/33, 64, 256/257 sit either
// side of powers of two, where merge sort's pass count changes parity; 127
// and 512 give depth without cost.
inline
std::size_t battery_size_at(
    std::size_t _index
)
{
    static const std::size_t sizes[] =
    {
        0, 1, 2, 3, 5, 8, 13, 16, 17, 18, 32, 33, 64, 127, 256, 257, 512
    };

    if (_index >= battery_size_count)
    {
        return 0;
    }

    return sizes[_index];
}

// standard_battery
//   function: the cross-product of every length and every shape -- the
// shared int corpus each algorithm is driven over.
inline
std::vector<DBatteryCase> standard_battery()
{
    std::vector<DBatteryCase> cases;
    std::size_t               size_index;
    std::size_t               shape_index;
    DBatteryCase              entry;

    cases.reserve(battery_size_count * battery_shape_count);

    for (size_index = 0; size_index < battery_size_count; ++size_index)
    {
        for (shape_index = 0;
             shape_index < battery_shape_count;
             ++shape_index)
        {
            entry.shape = shape_name(shape_index);
            entry.size  = battery_size_at(size_index);
            entry.data  = make_shape(shape_index,
                                     battery_size_at(size_index));

            cases.push_back(entry);
        }
    }

    return cases;
}

// DKeyedBatteryCase
//   struct: one stability input, carrying the number of distinct keys that
// produced it.
struct DKeyedBatteryCase
{
    std::size_t              keys;
    std::size_t              size;
    std::vector<DKeyedValue> data;
};

// keyed_modulus_count
//   constant: the number of key cardinalities in the stability battery.
const std::size_t keyed_modulus_count = 6;

// keyed_modulus_at
//   function: the key cardinality at index _index.  1 makes every element
// equivalent (the hardest stability case); the large final value makes
// collisions rare, so the ordering itself is exercised too.
inline
std::size_t keyed_modulus_at(
    std::size_t _index
)
{
    static const std::size_t moduli[] =
    {
        1, 2, 3, 5, 17, 1000003
    };

    if (_index >= keyed_modulus_count)
    {
        return 1;
    }

    return moduli[_index];
}

// keyed_battery
//   function: the stability corpus.  Keys are scrambled by a coprime stride
// so the input is never already ordered, and tags are the input positions, so
// a stable result leaves them ascending within each key group.
inline
std::vector<DKeyedBatteryCase> keyed_battery()
{
    static const std::size_t sizes[] =
    {
        0, 1, 2, 3, 5, 8, 16, 17, 33, 64, 128, 257
    };

    std::vector<DKeyedBatteryCase> cases;
    std::size_t                     size_index;
    std::size_t                     modulus_index;
    std::size_t                     i;
    std::size_t                     modulus;
    DKeyedBatteryCase              entry;

    for (size_index = 0; size_index < 12; ++size_index)
    {
        for (modulus_index = 0;
             modulus_index < keyed_modulus_count;
             ++modulus_index)
        {
            modulus = keyed_modulus_at(modulus_index);

            entry.keys = modulus;
            entry.size = sizes[size_index];
            entry.data.clear();
            entry.data.reserve(sizes[size_index]);

            for (i = 0; i < sizes[size_index]; ++i)
            {
                entry.data.push_back(
                    make_keyed(static_cast<int>((i * 7919) % modulus),
                               static_cast<int>(i)));
            }

            cases.push_back(entry);
        }
    }

    return cases;
}


///////////////////////////////////////////////////////////////////////////////
///                    VII. BATTERY DRIVERS                                 ///
///////////////////////////////////////////////////////////////////////////////

// verify_battery_reference
//   function: drives _sorter over the whole standard battery and compares
// each result against a std::stable_sort reference.
//
//   Exact equality is legitimate even for an UNSTABLE algorithm here: the
// elements are plain ints, so the multiset alone determines the ordered
// sequence and every correct sort must produce the identical vector.  It is a
// strictly stronger post-condition than "ordered and a permutation", and it
// catches a dropped or duplicated element that an ordering check alone would
// miss.
//
//   Sorter is anything callable as _sorter(begin, end) -- a function pointer
// adapter or a functor.
template<typename Sorter,
         typename Compare>
bool verify_battery_reference(
    Sorter  _sorter,
    Compare _compare,
    verdict& _verdict
)
{
    std::vector<DBatteryCase> cases;
    std::vector<int>          work;
    std::vector<int>          expected;
    std::size_t               i;

    cases = standard_battery();

    for (i = 0; i < cases.size(); ++i)
    {
        work     = cases[i].data;
        expected = cases[i].data;

        std::stable_sort(expected.begin(), expected.end(), _compare);

        _sorter(work.begin(), work.end());

        _verdict.check(work == expected);
    }

    return _verdict.ok();
}

// verify_battery_raw
//   function: the same sweep driven through RAW POINTERS rather than vector
// iterators, so the algorithms are proved to work on the most primitive model
// of a random-access iterator.
//
//   One spare slot is reserved before data() is taken: an empty vector may
// return a null pointer, and the sorts form (last - first) and, in the
// quicksort partitioner, (last - 1).  Reserving guarantees a real address for
// the empty case so first == last is still a valid comparison.
template<typename Sorter,
         typename Compare>
bool verify_battery_raw(
    Sorter  _sorter,
    Compare _compare,
    verdict& _verdict
)
{
    std::vector<DBatteryCase> cases;
    std::vector<int>          work;
    std::vector<int>          expected;
    std::size_t               i;
    int*                      base;

    cases = standard_battery();

    for (i = 0; i < cases.size(); ++i)
    {
        work     = cases[i].data;
        expected = cases[i].data;

        std::stable_sort(expected.begin(), expected.end(), _compare);

        work.reserve(work.size() + 1);
        base = work.data();

        _sorter(base, base + work.size());

        _verdict.check(work == expected);
    }

    return _verdict.ok();
}

// verify_stability_battery
//   function: drives _sorter over the keyed corpus and checks ordering,
// permutation, and -- when _expect_stable -- that equivalents kept their
// input order.
//
//   The stability check is asserted only for algorithms that claim it: an
// unstable algorithm is permitted to preserve order by accident, so the
// negative is not an assertion, it is measured separately where it matters.
template<typename Sorter>
bool verify_stability_battery(
    Sorter  _sorter,
    bool     _expect_stable,
    verdict& _verdict
)
{
    std::vector<DKeyedBatteryCase> cases;
    std::vector<DKeyedValue>        work;
    std::size_t                     i;

    cases = keyed_battery();

    for (i = 0; i < cases.size(); ++i)
    {
        work = cases[i].data;

        _sorter(work.begin(), work.end());

        _verdict.check(is_sorted_range(work.begin(),
                                       work.end(),
                                       keyed_value_less()));
        _verdict.check(is_permutation_of(work, cases[i].data));

        if (_expect_stable)
        {
            _verdict.check(is_stable_result(work));
        }
    }

    return _verdict.ok();
}


///////////////////////////////////////////////////////////////////////////////
///                    VIII. ORDERED-ENTRY PROBE                            ///
///////////////////////////////////////////////////////////////////////////////

// D_TESTS_SORT_ORDERED_PROBE
//   macro: for the entry point FN, defines
//     NAME_ordered_probe<It, Cmp>        -- true_type iff
//                                           FN(It, It, Cmp, sort_order::value)
//                                           is a viable call
//     NAME_call_ordered(..., tag)        -- tag-dispatched invocation
//
//   The six *_sort_ordered entry points are meant to share one shape, and a
// generic caller swapping one algorithm for another leans on that.  Naming an
// entry point whose fourth parameter had drifted to another type would be a
// hard compile error that took the whole suite down; the probe moves the
// question into overload resolution, where it is SFINAE-friendly, so a
// mismatch is reported as a failed check and the suite still builds.  The
// false_type overload of NAME_call_ordered keeps a body that could not be
// instantiated from ever being instantiated.
#define D_TESTS_SORT_ORDERED_PROBE(NAME, FN)                                  \
    template<typename Void,                                                   \
             typename Iterator,                                               \
             typename Compare>                                                \
    struct NAME##_ordered_probe_impl                                          \
        : std::false_type                                                     \
    {                                                                         \
    };                                                                        \
                                                                              \
    template<typename Iterator,                                               \
             typename Compare>                                                \
    struct NAME##_ordered_probe_impl<                                         \
        decltype(void(FN(std::declval<Iterator>(),                            \
                         std::declval<Iterator>(),                            \
                         std::declval<Compare>(),                             \
                         std::declval< ::djinterp::sort_order::value>()))),   \
        Iterator,                                                             \
        Compare>                                                              \
        : std::true_type                                                      \
    {                                                                         \
    };                                                                        \
                                                                              \
    template<typename Iterator,                                               \
             typename Compare>                                                \
    struct NAME##_ordered_probe                                               \
        : NAME##_ordered_probe_impl<void, Iterator, Compare>                  \
    {                                                                         \
    };                                                                        \
                                                                              \
    template<typename Iterator,                                               \
             typename Compare>                                                \
    void NAME##_call_ordered(                                                 \
        Iterator              _first,                                         \
        Iterator              _last,                                          \
        Compare               _compare,                                       \
        ::djinterp::sort_order::value _order,                                 \
        std::true_type                                                        \
    )                                                                         \
    {                                                                         \
        FN(_first, _last, _compare, _order);                                  \
                                                                              \
        return;                                                               \
    }                                                                         \
                                                                              \
    template<typename Iterator,                                               \
             typename Compare>                                                \
    void NAME##_call_ordered(                                                 \
        Iterator,                                                             \
        Iterator,                                                             \
        Compare,                                                              \
        ::djinterp::sort_order::value,                                        \
        std::false_type                                                       \
    )                                                                         \
    {                                                                         \
        return;                                                               \
    }


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_SORT_SUPPORT_
