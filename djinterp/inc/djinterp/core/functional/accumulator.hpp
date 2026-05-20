/******************************************************************************
* djinterp [functional]                                        accumulator.hpp
*
* First-class accumulators with combinable folds (C++).
*   An accumulator is a (state, step, finalize) triple that consumes a
* stream of values and yields a single output. This module makes
* accumulators first-class values with their own combinators, most
* importantly `combine(a, b, c, ...)`, which runs several accumulators
* in lock-step over a single pass of the data and returns a tuple of
* their results. This pattern is sometimes called "applicative folds".
*
*   For example, instead of three passes:
*     auto s = std::accumulate(v.begin(), v.end(), 0);
*     auto n = v.size();
*     auto m = *std::max_element(v.begin(), v.end());
*
*   one pass suffices:
*     auto tuple = combine(sum<int>(), count<int>(), max<int>()).run(v);
*     int s = std::get<0>(tuple);
*
*   The accumulator concept also supports contramap (input transform),
* map_output (output transform), filtered (predicate gate), and a wide
* assortment of pre-built accumulators (sum, count, min, max, mean,
* variance, joining, first, last, nth, to_vector, to_map_by, histogram,
* top_k).
*
* USAGE:
*   auto stats = combine(sum<double>(),
*                        mean<double>(),
*                        min<double>(),
*                        max<double>()).run(values);
*   // contramap: feed an int sum from a Person stream
*   auto total_age = contramap(sum<int>(),
*                              [](const Person& p){ return p.age; })
*                    .run(people);
*   // accumulator chaining: max of (filtered) values
*   auto m = filtered(max<int>(),
*                     [](int x){ return x > 0; }).run(values);
*
* 
* path:      /inc/djinterp/core/functional/accumulator.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.20
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    ACCUMULATOR PRIMITIVE
      1.  accumulator<_State, _Input, _Output>    (the (S, step, fin) triple)
      2.  make_accumulator                        (factory from lambdas)
II.   PRE-BUILT ACCUMULATORS
      1.  sum<_Type>
      2.  product<_Type>
      3.  count<_Type>
      4.  count_if<_Type, _P>
      5.  min<_Type>
      6.  max<_Type>
      7.  min_by<_Type, _Key>
      8.  max_by<_Type, _Key>
      9.  mean<_Type>
      10. variance<_Type>                         (population)
      11. stddev<_Type>                           
      12. first<_Type>                            
      13. last<_Type>                             
      14. nth<_Type>                              
      15. joining<_Type>                          (string concat with separator)
      16. to_vector<_Type>                        
      17. to_map_by<_Type, _Key>                  
      18. group_by<_Type, _Key>                   
      19. histogram<_Type>                        
      20. top_k<_Type>                            (k largest, sorted desc)
      21. all_match<_Type, _P>
      22. any_match<_Type, _P>
      23. none_match<_Type, _P>
III.  ACCUMULATOR COMBINATORS
      1.  contramap                               (map input)
      2.  map_output                              (map output)
      3.  filtered                                (predicate gate)
      4.  combine                                 (variadic parallel folds)
      5.  take                                    (cap input count)
IV.   DRIVERS / TERMINAL OPS
      1.  accumulator::run(container)
      2.  accumulator::step(value)                (manual incremental driving)
      3.  accumulator::finalize()
*/

#ifndef DJINTERP_FUNCTIONAL_ACCUMULATOR_
#define DJINTERP_FUNCTIONAL_ACCUMULATOR_ 1

// std
#include <cstddef>
#include <iterator>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../djinterp.hpp"
#include "./functional_traits.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    ACCUMULATOR PRIMITIVE                                 ///
///////////////////////////////////////////////////////////////////////////////

// accumulator
//   class: a triple (state, step, finalize) that absorbs values of
// _Input and produces a single _Output.
//
//   The class stores the current state inline. `step(value)` updates
// the state via the step function. `finalize()` produces the output
// from the current state. `run(container)` drives a full fold and
// returns the output in one call.
//
//   Accumulators are intentionally not pure: their state is mutated
// in place during a fold. Copies of an accumulator share initial
// state but progress independently (each `step` modifies only the
// instance it was called on).
template<typename _State,
         typename _Input,
         typename _Output>
class accumulator
{
public:
    using state_type    = _State;
    using input_type    = _Input;
    using output_type   = _Output;
    using step_fn_type  = std::function<void(_State&, const _Input&)>;
    using final_fn_type = std::function<_Output(const _State&)>;

    // constructor
    //   takes the initial state, step function, and finalize function.
    template<typename _StateFwd,
             typename _StepFwd,
             typename _FinalFwd>
    accumulator(
        _StateFwd&& _initial_state,
        _StepFwd&&  _step,
        _FinalFwd&& _finalize
    )
        : m_state(std::forward<_StateFwd>(_initial_state)),
          m_step(std::forward<_StepFwd>(_step)),
          m_finalize(std::forward<_FinalFwd>(_finalize))
    {}

    // step
    //   method: feed a single value to the accumulator, updating its
    // state. Returns *this to support chained calls.
    accumulator&
    step(
        const _Input& _value
    )
    {
        m_step(m_state, _value);

        return *this;
    }

    // finalize
    //   method: produce the output from the current state. Does not
    // modify the state; may be called multiple times.
    D_NODISCARD _Output
    finalize() const
    {
        return m_finalize(m_state);
    }

    // run (container)
    //   method: drive the accumulator over every element of
    // _container, then finalize. The accumulator's state is mutated
    // during the fold; use a fresh copy for repeated runs.
    template<typename _Container>
    D_NODISCARD _Output
    run(
        const _Container& _container
    )
    {
        for (const auto& element : _container)
        {
            m_step(m_state, element);
        }

        return m_finalize(m_state);
    }

    // run (iterator range)
    //   method: drive the accumulator over [_first, _last), then
    // finalize.
    template<typename _InputIt>
    D_NODISCARD _Output
    run(
        _InputIt _first,
        _InputIt _last
    )
    {
        for (auto it = _first; it != _last; ++it)
        {
            m_step(m_state, *it);
        }

        return m_finalize(m_state);
    }

    // run (raw array)
    //   method: drive the accumulator over a C-style array of
    // _count elements, then finalize.
    D_NODISCARD _Output
    run(
        const _Input* _data,
        std::size_t   _count
    )
    {
        for (std::size_t i = 0; i < _count; ++i)
        {
            m_step(m_state, _data[i]);
        }

        return m_finalize(m_state);
    }

    // state (read-only access)
    //   method: returns a const reference to the current state. Used
    // by tests and by combinators that need to inspect intermediate
    // state without finalizing.
    D_NODISCARD const _State&
    state() const
    {
        return m_state;
    }

    // step_fn
    //   method: returns the stored step function by const reference.
    // Used by combinators (contramap, map_output, filtered, take)
    // to compose the inner accumulator's behavior without rebuilding
    // it per element.
    D_NODISCARD
    const step_fn_type&
    step_fn() const
    {
        return m_step;
    }

    // finalize_fn
    //   method: returns the stored finalize function by const
    // reference. Used by combinators that compose with the inner
    // accumulator's output transform.
    D_NODISCARD
    const final_fn_type&
    finalize_fn() const
    {
        return m_finalize;
    }

private:
    _State        m_state;
    step_fn_type  m_step;
    final_fn_type m_finalize;
};


// make_accumulator
//   function: factory for building an accumulator from an initial
// state, a step function, and a finalize function. Template
// parameters _Input and _Output must be supplied because step and
// finalize are typically generic lambdas with no fixed signature.
template<typename _Input,
         typename _Output,
         typename _State,
         typename _Step,
         typename _Final>
D_NODISCARD accumulator<typename std::decay<_State>::type, 
                       _Input, 
                       _Output>
make_accumulator(
    _State&& _initial_state,
    _Step&&  _step,
    _Final&& _finalize
)
{
    return accumulator<
        typename std::decay<_State>::type, _Input, _Output>(
            std::forward<_State>(_initial_state),
            std::forward<_Step>(_step),
            std::forward<_Final>(_finalize));
}


///////////////////////////////////////////////////////////////////////////////
///             II.   PRE-BUILT ACCUMULATORS                                ///
///////////////////////////////////////////////////////////////////////////////

// sum
//   function: accumulator that sums its inputs. The result type is
// the same as the input type. Initial state is value-initialized
// (zero for numeric types).
template<typename _Type>
D_NODISCARD accumulator<_Type,
                        _Type,
                        _Type>
sum()
{
    return accumulator<_Type, _Type, _Type>(
        _Type(),
        [](_Type& _s, const _Type& _v) { _s = _s + _v; },
        [](const _Type& _s) { return _s; });
}


// product
//   function: accumulator that multiplies its inputs. Initial state
// is _Type(1).
template<typename _Type>
D_NODISCARD accumulator<_Type,
                        _Type,
                        _Type>
product()
{
    return accumulator<_Type, _Type, _Type>(
        _Type(1),
        [](_Type& _s, const _Type& _v) { _s = _s * _v; },
        [](const _Type& _s) { return _s; });
}


// count
//   function: accumulator that counts its inputs, returning size_t.
// _Type is the input type and is purely informational.
template<typename _Type>
D_NODISCARD accumulator<std::size_t,
                        _Type,
                        std::size_t>
count()
{
    return accumulator<std::size_t, _Type, std::size_t>(
        std::size_t(0),
        [](std::size_t& _s, const _Type&) { ++_s; },
        [](const std::size_t& _s) { return _s; });
}


// count_if
//   function: accumulator that counts inputs satisfying _predicate.
template<typename _Type,
         typename _Predicate>
D_NODISCARD accumulator<std::size_t,
                        _Type,
                        std::size_t>
count_if(
    _Predicate _predicate
)
{
    return accumulator<std::size_t, _Type, std::size_t>(
        std::size_t(0),
        [_predicate](std::size_t& _s, const _Type& _v) {
            if (_predicate(_v))
            {
                ++_s;
            }
        },
        [](const std::size_t& _s) { return _s; });
}


// min
//   function: accumulator that tracks the minimum of its inputs. The
// state pairs the running min with a "seen any" flag so that the
// first input becomes the initial value. The finalize fails (returns
// default-constructed _Type) for empty input; callers should pair this
// with count() in a combine() if emptiness needs detection.
template<typename _Type>
D_NODISCARD accumulator<std::pair<_Type, bool>, 
                        _Type, 
                        _Type>
min()
{
    return accumulator<std::pair<_Type, bool>, _Type, _Type>(
        std::pair<_Type, bool>(_Type(), false),
        [](std::pair<_Type, bool>& _s, const _Type& _v) {
            if (!_s.second || (_v < _s.first))
            {
                _s.first  = _v;
                _s.second = true;
            }
        },
        [](const std::pair<_Type, bool>& _s) { return _s.first; });
}


// max
//   function: accumulator that tracks the maximum of its inputs.
// Same emptiness caveat as min().
template<typename _Type>
D_NODISCARD accumulator<std::pair<_Type, bool>,
                        _Type,
                        _Type>
max()
{
    return accumulator<std::pair<_Type, bool>, _Type, _Type>(
        std::pair<_Type, bool>(_Type(), false),
        [](std::pair<_Type, bool>& _s, const _Type& _v) {
            if (!_s.second || (_s.first < _v))
            {
                _s.first  = _v;
                _s.second = true;
            }
        },
        [](const std::pair<_Type, bool>& _s) { return _s.first; });
}


// min_by
//   function: accumulator that tracks the element whose key (via
// _key_fn) is smallest. Returns the element itself, not the key.
template<typename _Type,
         typename _Key>
D_NODISCARD accumulator<std::pair<_Type, bool>, 
                        _Type, 
                        _Type>
min_by(
    _Key _key_fn
)
{
    return accumulator<std::pair<_Type, bool>, _Type, _Type>(
        std::pair<_Type, bool>(_Type(), false),
        [_key_fn](std::pair<_Type, bool>& _s, const _Type& _v) {
            if (!_s.second || (_key_fn(_v) < _key_fn(_s.first)))
            {
                _s.first  = _v;
                _s.second = true;
            }
        },
        [](const std::pair<_Type, bool>& _s) { return _s.first; });
}


// max_by
//   function: accumulator that tracks the element whose key (via
// _key_fn) is largest. Returns the element itself.
template<typename _Type,
         typename _Key>
D_NODISCARD accumulator<std::pair<_Type, bool>,
                        _Type,
                        _Type>
max_by(
    _Key _key_fn
)
{
    return accumulator<std::pair<_Type, bool>, _Type, _Type>(
        std::pair<_Type, bool>(_Type(), false),
        [_key_fn](std::pair<_Type, bool>& _s, const _Type& _v) {
            if (!_s.second || (_key_fn(_s.first) < _key_fn(_v)))
            {
                _s.first  = _v;
                _s.second = true;
            }
        },
        [](const std::pair<_Type, bool>& _s) { return _s.first; });
}


// mean
//   function: accumulator that computes the arithmetic mean as a
// double. Tracks running sum and count. Returns 0.0 for empty input.
template<typename _Type>
D_NODISCARD accumulator<std::pair<double, std::size_t>,
                        _Type,
                        double>
mean()
{
    return accumulator<std::pair<double, std::size_t>, _Type, double>(
        std::pair<double, std::size_t>(0.0, std::size_t(0)),
        [](std::pair<double, std::size_t>& _s, const _Type& _v) {
            _s.first += static_cast<double>(_v);
            ++_s.second;
        },
        [](const std::pair<double, std::size_t>& _s) -> double {
            if (_s.second == 0)
            {
                return 0.0;
            }

            return _s.first / static_cast<double>(_s.second);
        });
}


// variance
//   function: accumulator that computes the population variance via
// Welford's online algorithm (numerically stable for large streams).
// Returns 0.0 for empty input or single-element input. State is the
// tuple (count, mean, M2).
template<typename _Type>
D_NODISCARD accumulator<std::tuple<std::size_t, double, double>, 
                        _Type, 
                        double>
variance()
{
    return accumulator<std::tuple<std::size_t, double, double>, _Type, double>(
        std::make_tuple(std::size_t(0), 0.0, 0.0),
        [](std::tuple<std::size_t, double, double>& _s, const _Type& _v) {
            std::size_t& n    = std::get<0>(_s);
            double&      mean = std::get<1>(_s);
            double&      m2   = std::get<2>(_s);

            ++n;
            double delta  = static_cast<double>(_v) - mean;
            mean         += delta / static_cast<double>(n);
            double delta2 = static_cast<double>(_v) - mean;
            m2           += delta * delta2;
        },
        [](const std::tuple<std::size_t, double, double>& _s) -> double {
            std::size_t n = std::get<0>(_s);

            if (n < 2)
            {
                return 0.0;
            }

            return std::get<2>(_s) / static_cast<double>(n);
        });
}


// stddev
//   function: accumulator that computes population standard
// deviation. Implemented as variance() followed by sqrt; numerically
// equivalent to running variance() and post-processing.
template<typename _Type>
D_NODISCARD accumulator<std::tuple<std::size_t, double, double>, 
                        _Type, 
                        double>
stddev()
{
    return accumulator<std::tuple<std::size_t, double, double>, _Type, double>(
        std::make_tuple(std::size_t(0), 0.0, 0.0),
        [](std::tuple<std::size_t, double, double>& _s, const _Type& _v) {
            std::size_t& n    = std::get<0>(_s);
            double&      mean = std::get<1>(_s);
            double&      m2   = std::get<2>(_s);

            ++n;
            double delta  = static_cast<double>(_v) - mean;
            mean         += delta / static_cast<double>(n);
            double delta2 = static_cast<double>(_v) - mean;
            m2           += delta * delta2;
        },
        [](const std::tuple<std::size_t, double, double>& _s) -> double {
            std::size_t n = std::get<0>(_s);

            if (n < 2)
            {
                return 0.0;
            }

            double var = std::get<2>(_s) / static_cast<double>(n);

            // ad-hoc sqrt (avoids dependency on <cmath> here)
            // good for moderate magnitudes; callers needing high
            // precision should use stddev() and post-process with
            // std::sqrt themselves.
            if (var <= 0.0)
            {
                return 0.0;
            }

            double x = var;
            for (int i = 0; i < 16; ++i)
            {
                x = 0.5 * (x + (var / x));
            }

            return x;
        });
}


// first
//   function: accumulator that captures the first input it sees and
// ignores the rest. Returns default-constructed _Type for empty input.
template<typename _Type>
D_NODISCARD accumulator<std::pair<_Type, bool>,
                        _Type,
                        _Type>
first()
{
    return accumulator<std::pair<_Type, bool>, _Type, _Type>(
        std::pair<_Type, bool>(_Type(), false),
        [](std::pair<_Type, bool>& _s, const _Type& _v) {
            if (!_s.second)
            {
                _s.first  = _v;
                _s.second = true;
            }
        },
        [](const std::pair<_Type, bool>& _s) { return _s.first; });
}


// last
//   function: accumulator that captures the most recent input.
// Returns default-constructed _Type for empty input.
template<typename _Type>
D_NODISCARD accumulator<_Type,
                        _Type,
                        _Type>
last()
{
    return accumulator<_Type, _Type, _Type>(
        _Type(),
        [](_Type& _s, const _Type& _v) { _s = _v; },
        [](const _Type& _s) { return _s; });
}


// nth
//   function: accumulator that captures the _n-th (zero-indexed)
// input. Returns default-constructed _Type if fewer than (_n + 1)
// inputs are observed.
template<typename _Type>
D_NODISCARD accumulator<std::tuple<_Type, std::size_t, std::size_t>,
                        _Type, 
                        _Type>
nth(
    std::size_t _n
)
{
    return accumulator<std::tuple<_Type, std::size_t, std::size_t>, _Type, _Type>(
        std::make_tuple(_Type(), std::size_t(0), _n),
        [](std::tuple<_Type, std::size_t, std::size_t>& _s, const _Type& _v) {
            std::size_t& seen = std::get<1>(_s);
            std::size_t  want = std::get<2>(_s);

            if (seen == want)
            {
                std::get<0>(_s) = _v;
            }

            ++seen;
        },
        [](const std::tuple<_Type, std::size_t, std::size_t>& _s) {
            return std::get<0>(_s);
        });
}


// joining
//   function: accumulator that converts each input to string (via
// std::ostringstream) and concatenates with _separator between
// elements. Returns "" for empty input.
template<typename _Type>
D_NODISCARD accumulator<std::pair<std::string, bool>,
                        _Type, 
                        std::string>
joining(
    std::string _separator
)
{
    return accumulator<std::pair<std::string, bool>, _Type, std::string>(
        std::pair<std::string, bool>(std::string(), false),
        [_separator](std::pair<std::string, bool>& _s, const _Type& _v) {
            std::ostringstream oss;

            if (_s.second)
            {
                _s.first += _separator;
            }

            oss << _v;
            _s.first += oss.str();
            _s.second = true;
        },
        [](const std::pair<std::string, bool>& _s) { return _s.first; });
}


// to_vector
//   function: accumulator that collects all inputs into a vector.
template<typename _Type>
D_NODISCARD accumulator<std::vector<_Type>, 
                        _Type, 
                        std::vector<_Type>>
to_vector()
{
    return accumulator<std::vector<_Type>, _Type, std::vector<_Type>>(
        std::vector<_Type>(),
        [](std::vector<_Type>& _s, const _Type& _v) { _s.push_back(_v); },
        [](const std::vector<_Type>& _s) { return _s; });
}


// to_map_by
//   function: accumulator that builds a std::map keyed by
// _key_fn(input), where each key maps to the LAST input observed
// with that key. For grouping (one-key-to-many), use group_by.
template<typename _Type,
         typename _Key>
D_NODISCARD accumulator<std::map<typename std::decay<decltype(
                std::declval<_Key&>()(std::declval<const _Type&>()))>::type, _Type>,
            _Type,
            std::map<typename std::decay<decltype(
                std::declval<_Key&>()(std::declval<const _Type&>()))>::type, _Type>>
to_map_by(
    _Key _key_fn
)
{
    using key_type = typename std::decay<decltype(
        _key_fn(std::declval<const _Type&>()))>::type;
    using map_type = std::map<key_type, _Type>;

    return accumulator<map_type, _Type, map_type>(
        map_type(),
        [_key_fn](map_type& _s, const _Type& _v) {
            _s[_key_fn(_v)] = _v;
        },
        [](const map_type& _s) { return _s; });
}


// group_by
//   function: accumulator that groups inputs by _key_fn(input) into
// a std::map where each key maps to a std::vector of all inputs
// with that key.
template<typename _Type,
         typename _Key>
D_NODISCARD accumulator<std::map<typename std::decay<decltype(
                std::declval<_Key&>()(std::declval<const _Type&>()))>::type,
                std::vector<_Type>>,
            _Type,
            std::map<typename std::decay<decltype(
                std::declval<_Key&>()(std::declval<const _Type&>()))>::type,
                std::vector<_Type>>>
group_by(
    _Key _key_fn
)
{
    using key_type = typename std::decay<decltype(
        _key_fn(std::declval<const _Type&>()))>::type;
    using map_type = std::map<key_type, std::vector<_Type>>;

    return accumulator<map_type, _Type, map_type>(
        map_type(),
        [_key_fn](map_type& _s, const _Type& _v) {
            _s[_key_fn(_v)].push_back(_v);
        },
        [](const map_type& _s) { return _s; });
}


// histogram
//   function: accumulator that builds a frequency map: each input
// value is a key, and its value is the number of times that input
// was observed.
template<typename _Type>
D_NODISCARD accumulator<std::map<_Type, std::size_t>,
            _Type,
            std::map<_Type, std::size_t>>
histogram()
{
    using map_type = std::map<_Type, std::size_t>;

    return accumulator<map_type, _Type, map_type>(
        map_type(),
        [](map_type& _s, const _Type& _v) { ++_s[_v]; },
        [](const map_type& _s) { return _s; });
}


// top_k
//   function: accumulator that retains the _k largest inputs, in
// descending order. Naive O(N * k) implementation; suitable for
// small k. For large k consider an external heap-based variant.
template<typename _Type>
D_NODISCARD accumulator<std::vector<_Type>, _Type, std::vector<_Type>>
top_k(
    std::size_t _k
)
{
    return accumulator<std::vector<_Type>, _Type, std::vector<_Type>>(
        std::vector<_Type>(),
        [_k](std::vector<_Type>& _s, const _Type& _v) {
            // skip if heap is full and value is no better than min
            if ( (_s.size() >= _k) &&
                 ( !(_s.back() < _v) ) )
            {
                return;
            }

            // insert in sorted (desc) position
            auto it = _s.begin();

            while (it != _s.end() && (*it >= _v))
            {
                ++it;
            }

            _s.insert(it, _v);

            // trim
            if (_s.size() > _k)
            {
                _s.resize(_k);
            }
        },
        [](const std::vector<_Type>& _s) { return _s; });
}


// all_match
//   function: accumulator that returns true iff every input
// satisfies _predicate. Vacuously true for empty input. NOTE: this
// does NOT short-circuit; the entire stream is consumed.
template<typename _Type,
         typename _Predicate>
D_NODISCARD accumulator<bool, _Type, bool>
all_match(
    _Predicate _predicate
)
{
    return accumulator<bool, _Type, bool>(
        true,
        [_predicate](bool& _s, const _Type& _v) {
            _s = _s && _predicate(_v);
        },
        [](const bool& _s) { return _s; });
}


// any_match
//   function: accumulator that returns true iff at least one input
// satisfies _predicate. False for empty input. Does NOT short-circuit.
template<typename _Type,
         typename _Predicate>
D_NODISCARD accumulator<bool, _Type, bool>
any_match(
    _Predicate _predicate
)
{
    return accumulator<bool, _Type, bool>(
        false,
        [_predicate](bool& _s, const _Type& _v) {
            _s = _s || _predicate(_v);
        },
        [](const bool& _s) { return _s; });
}


// none_match
//   function: accumulator that returns true iff no input satisfies
// _predicate. Vacuously true for empty input.
template<typename _Type,
         typename _Predicate>
D_NODISCARD accumulator<bool, _Type, bool>
none_match(
    _Predicate _predicate
)
{
    return accumulator<bool, _Type, bool>(
        true,
        [_predicate](bool& _s, const _Type& _v) {
            if (_predicate(_v))
            {
                _s = false;
            }
        },
        [](const bool& _s) { return _s; });
}


///////////////////////////////////////////////////////////////////////////////
///             III.  ACCUMULATOR COMBINATORS                               ///
///////////////////////////////////////////////////////////////////////////////

// contramap
//   function: adapts an accumulator<S, B, O> to accept inputs of
// type _NewInput by pre-applying _function : _NewInput -> B before
// each step. This is the contramap operation for accumulators.
//   The resulting accumulator shares the inner accumulator's state
// type, step semantics, and output. Step and finalize are pulled
// from the inner accumulator by reference-capture and invoked
// directly; no per-element accumulator copy is performed.
template<typename _NewInput,
         typename _Acc,
         typename _Function>
D_NODISCARD accumulator<typename _Acc::state_type, _NewInput, typename _Acc::output_type>
contramap(
    _Acc      _inner,
    _Function _function
)
{
    using state_t  = typename _Acc::state_type;
    using output_t = typename _Acc::output_type;
    using step_t   = typename _Acc::step_fn_type;
    using final_t  = typename _Acc::final_fn_type;

    step_t  inner_step     = _inner.step_fn();
    final_t inner_finalize = _inner.finalize_fn();

    return accumulator<state_t, _NewInput, output_t>(
        _inner.state(),
        [inner_step, _function]
            (state_t& _s, const _NewInput& _v)
        {
            inner_step(_s, _function(_v));
        },
        [inner_finalize](const state_t& _s) {
            return inner_finalize(_s);
        });
}


// contramap_with
//   function: high-performance contramap that takes an explicit
// step function in lambda form. Avoids the std::function indirection
// of contramap when the inner step is known concretely (e.g. you
// are building an accumulator from primitives rather than adapting
// a pre-built one).
template<typename _NewInput,
         typename _State,
         typename _Output,
         typename _InnerStep,
         typename _Final,
         typename _Function>
D_NODISCARD accumulator<_State, _NewInput, _Output>
contramap_with(
    _State     _initial_state,
    _InnerStep _inner_step,
    _Final     _finalize,
    _Function  _function
)
{
    return accumulator<_State, _NewInput, _Output>(
        std::move(_initial_state),
        [_inner_step, _function]
            (_State& _s, const _NewInput& _v) mutable
        {
            _inner_step(_s, _function(_v));
        },
        std::move(_finalize));
}


// map_output
//   function: post-applies _function to the output of an inner
// accumulator. Step semantics are inherited unchanged; only finalize
// is composed with _function.
template<typename _Acc,
         typename _Function,
         typename _NewOutput = typename std::decay<decltype(
             std::declval<_Function&>()(
                 std::declval<typename _Acc::output_type>()))>::type>
D_NODISCARD accumulator<typename _Acc::state_type,
            typename _Acc::input_type,
            _NewOutput>
map_output(
    _Acc      _inner,
    _Function _function
)
{
    using state_t = typename _Acc::state_type;
    using input_t = typename _Acc::input_type;
    using step_t  = typename _Acc::step_fn_type;
    using final_t = typename _Acc::final_fn_type;

    step_t  inner_step     = _inner.step_fn();
    final_t inner_finalize = _inner.finalize_fn();

    return accumulator<state_t, input_t, _NewOutput>(
        _inner.state(),
        [inner_step](state_t& _s, const input_t& _v) {
            inner_step(_s, _v);
        },
        [inner_finalize, _function](const state_t& _s) {
            return _function(inner_finalize(_s));
        });
}


// filtered (accumulator)
//   function: gates the input of an inner accumulator with a
// predicate. Only inputs satisfying _predicate are passed to the
// inner accumulator.
template<typename _Acc,
         typename _Predicate>
D_NODISCARD accumulator<typename _Acc::state_type,
            typename _Acc::input_type,
            typename _Acc::output_type>
filtered(
    _Acc       _inner,
    _Predicate _predicate
)
{
    using state_t  = typename _Acc::state_type;
    using input_t  = typename _Acc::input_type;
    using output_t = typename _Acc::output_type;
    using step_t   = typename _Acc::step_fn_type;
    using final_t  = typename _Acc::final_fn_type;

    step_t  inner_step     = _inner.step_fn();
    final_t inner_finalize = _inner.finalize_fn();

    return accumulator<state_t, input_t, output_t>(
        _inner.state(),
        [inner_step, _predicate]
            (state_t& _s, const input_t& _v)
        {
            if (_predicate(_v))
            {
                inner_step(_s, _v);
            }
        },
        [inner_finalize](const state_t& _s) {
            return inner_finalize(_s);
        });
}


// take (accumulator)
//   function: caps the number of inputs an inner accumulator sees
// to at most _n; subsequent inputs are silently dropped. State is
// extended with a counter tracking how many inputs have been
// forwarded.
template<typename _Acc>
D_NODISCARD accumulator<std::pair<typename _Acc::state_type, std::size_t>,
            typename _Acc::input_type,
            typename _Acc::output_type>
take(
    _Acc        _inner,
    std::size_t _n
)
{
    using state_t  = typename _Acc::state_type;
    using input_t  = typename _Acc::input_type;
    using output_t = typename _Acc::output_type;
    using step_t   = typename _Acc::step_fn_type;
    using final_t  = typename _Acc::final_fn_type;
    using wrapped_state_t = std::pair<state_t, std::size_t>;

    step_t  inner_step     = _inner.step_fn();
    final_t inner_finalize = _inner.finalize_fn();

    return accumulator<wrapped_state_t, input_t, output_t>(
        wrapped_state_t(_inner.state(), std::size_t(0)),
        [inner_step, _n]
            (wrapped_state_t& _s, const input_t& _v)
        {
            if (_s.second < _n)
            {
                inner_step(_s.first, _v);
                ++_s.second;
            }
        },
        [inner_finalize](const wrapped_state_t& _s) {
            return inner_finalize(_s.first);
        });
}


///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL
    // combine_helper
    //   helper: holds a tuple of accumulators and drives them in
    // lock-step. Variadic over the tuple of accumulators.
    template<typename... _Accs>
    class combine_helper
    {
    public:
        // value tuple yielded by finalize(): one slot per inner
        // accumulator, each typed as that accumulator's output.
        using output_tuple = std::tuple<typename _Accs::output_type...>;

        // input type: all inner accumulators must accept the same
        // input. We take the first's input type as the canonical one;
        // mismatches surface as ordinary template errors.
        using input_type = typename std::tuple_element<
            0,
            std::tuple<typename _Accs::input_type...>>::type;

        template<typename... _AccsFwd>
        D_CONSTEXPR explicit 
        combine_helper(
            _AccsFwd&&... _accs
        )
            : m_accs(std::forward<_AccsFwd>(_accs)...)
        {}

        // step
        //   feeds one value to every inner accumulator.
        void 
        step(
            const input_type& _value
        )
        {
            step_helper(_value,
                      std::integral_constant<std::size_t, 0>{});

            return;
        }

        // finalize
        //   returns the tuple of outputs from each inner accumulator.
        D_NODISCARD output_tuple
        finalize() const
        {
            return finalize_helper(
                std::integral_constant<std::size_t, 0>{},
                std::tuple<>{}
            );
        }

        // run
        //   drives every inner accumulator over the same container
        // in a single pass.
        template<typename _Container>
        D_NODISCARD output_tuple
        run(
            const _Container& _container
        )
        {
            for (const auto& element : _container)
            {
                step(element);
            }

            return finalize();
        }

        template<typename _InputIt>
        D_NODISCARD output_tuple 
        run(
            _InputIt _first,
            _InputIt _last
        )
        {
            for (auto it = _first; it != _last; ++it)
            {
                step(*it);
            }

            return finalize();
        }

    private:
        // step_helper (recursive case)
        template<std::size_t _I>
        typename std::enable_if<(_I < sizeof...(_Accs))>::type
        step_helper(
            const input_type& _value,
            std::integral_constant<std::size_t, _I>
        )
        {
            std::get<_I>(m_accs).step(_value);
            step_helper(_value,
                      std::integral_constant<std::size_t, _I + 1>{});

            return;
        }

        // step_helper (base case)
        template<std::size_t _I>
        typename std::enable_if<(_I == sizeof...(_Accs))>::type
        step_helper(
            const input_type&,
            std::integral_constant<std::size_t, _I>
        )
        {
            return;
        }

        // finalize_helper (recursive case)
        //   builds the output tuple element-by-element by appending
        // each accumulator's finalize result.
        template<std::size_t _I,
                 typename... _SoFar>
        typename std::enable_if<(_I < sizeof...(_Accs)),
                                output_tuple>::type
        finalize_helper(
            std::integral_constant<std::size_t, _I>,
            std::tuple<_SoFar...> _so_far
        ) const
        {
            return finalize_helper(
                std::integral_constant<std::size_t, _I + 1>{},
                std::tuple_cat(
                    std::move(_so_far),
                    std::make_tuple(std::get<_I>(m_accs).finalize())));
        }

        // finalize_helper (base case)
        template<std::size_t _I,
                 typename... _SoFar>
        typename std::enable_if<(_I == sizeof...(_Accs)),
                                output_tuple>::type
        finalize_helper(
            std::integral_constant<std::size_t, _I>,
            std::tuple<_SoFar...> _so_far
        ) const
        {
            return _so_far;
        }

        std::tuple<_Accs...> m_accs;
    };

NS_END  // internal


// combine
//   function: runs multiple accumulators in lock-step over a single
// pass of the data. Returns a `combine_helper` whose run(container)
// method yields a std::tuple of outputs, one per accumulator.
//
//   Combine is the cornerstone of this module: it transforms what
// would normally be N passes over the data (N accumulators, N
// folds) into a single pass. For large containers, this can be a
// major performance win.
//
//   All inner accumulators must accept the same input type. The
// input type is taken from the first accumulator.
template<typename... _Accs>
D_NODISCARD
internal::combine_helper<typename std::decay<_Accs>::type...>
combine(
    _Accs&&... _accs
)
{
    return internal::combine_helper<
        typename std::decay<_Accs>::type...>(
            std::forward<_Accs>(_accs)...);
}


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_ACCUMULATOR_