/******************************************************************************
* djinterp [functional]                                           producer.hpp
*
* First-class producers (sources) for functional dataflow (C++).
*   A producer is a pull-driven source of values: a callable of signature
* `producer_step<A>()` returning a maybe-value indicating whether more data
* is available. This module elevates producers to first-class status with
* combinators for iteration, unfolding, repetition, cycling, and
* concatenation.
*   Producers are the dual of consumers: where consumers absorb values
* indefinitely without producing them, producers emit values indefinitely
* without absorbing them. Together with accumulators (state-folding sinks)
* and transformers, they form a complete dataflow vocabulary.
*
*   Producers are inherently lazy. Many of the producers in this module are
* notionally infinite (`repeat`, `cycle`, `iterate`); they only become
* finite when paired with a sink that takes a bounded prefix.
*
* USAGE:
*   // explicit iteration via the producer interface
*   auto fibs = producers::iterate(std::make_pair(0, 1),
*       [](std::pair<int,int> p){
*           return std::make_pair(p.second, p.first + p.second);
*       });
*   for (int i = 0; i < 10; ++i)
*   {
*       auto step = fibs();             // step.has_value == true here
*       std::cout << step.value.first << '\n';
*   }
*
*   // bounded consumption via take_n
*   auto first_ten = producers::take_n(fibs, 10).collect();
*
*   // sequencing: 1..3 then 100..102
*   auto seq = producers::concat(producers::range(1, 4),
*                                producers::range(100, 103));
*
* 
* path:      /inc/djinterp/core/functional/producer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.20
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    PRODUCER STEP TYPE
      1.  producer_step<T>                    (has_value + value)
II.   INTERNAL PRODUCER HELPER CLASSES
      1.  iterate_helper                      (seed + step function)
      2.  unfold_helper                       (state -> maybe value)
      3.  range_helper                        (numeric range)
      4.  repeat_helper                       (infinite single value)
      5.  repeat_n_helper                     (n-bounded single value)
      6.  cycle_helper                        (repeat container forever)
      7.  generate_helper                     (call nullary fn forever)
      8.  empty_producer_helper
      9.  single_helper                       (one-shot value)
      10. take_n_helper                       (first n then exhaust)
      11. drop_n_helper                       (skip first n)
      12. concat_helper                       (sequence two producers)
      13. interleave_helper                   (alternate between two)
      14. transform_helper                    (apply f to outputs)
      15. filter_helper                       (keep matching outputs)
III.  PRODUCER FACTORIES   (namespace producers)
      1.  iterate
      2.  unfold
      3.  range / iota
      4.  repeat / repeat_n
      5.  cycle
      6.  generate
      7.  empty
      8.  single
      9.  take_n / drop_n
      10. concat / interleave
      11. transform / filter
      12. from_container
IV.   TERMINAL CONVENIENCES
      1.  collect                             (drain to vector)
      2.  for_each                            (apply consumer until exhaust)
      3.  fold                                (drain via accumulator step)
*/

#ifndef DJINTERP_FUNCTIONAL_PRODUCER_
#define DJINTERP_FUNCTIONAL_PRODUCER_ 1

// std
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../djinterp.hpp"
#include "./functional_traits.hpp"


NS_DJINTERP

///////////////////////////////////////////////////////////////////////////////
///             I.    PRODUCER STEP TYPE                                    ///
///////////////////////////////////////////////////////////////////////////////

// producer_step
//   struct: result of one pull from a producer. has_value indicates
// whether a value was produced; value is meaningful only when
// has_value is true. This is a minimal Maybe<T> dedicated to the
// producer protocol; producers do not depend on a full maybe<T>
// module to avoid circular include order.
template<typename _Type>
struct producer_step
{
    bool   has_value;
    _Type  value;

    D_CONSTEXPR
    producer_step()
        : has_value(false)
        , value()
    {}

    D_CONSTEXPR
    explicit producer_step(
        const _Type& _value
    )
        : has_value(true)
        , value(_value)
    {}

    D_CONSTEXPR
    explicit producer_step(
        _Type&& _value
    )
        : has_value(true)
        , value(std::move(_value))
    {}
};


// make_step
//   function: convenience builder for a producer_step containing a
// value. Decays the input type so that the resulting step holds a
// non-reference value.
template<typename _Type>
D_CONSTEXPR
producer_step<typename std::decay<_Type>::type>
make_step
(
    _Type&& _value
)
{
    return producer_step<typename std::decay<_Type>::type>(
        std::forward<_Type>(_value));
}


// no_step
//   function: produces an empty step of the given type, signalling
// exhaustion to a downstream consumer.
template<typename _Type>
D_CONSTEXPR
producer_step<_Type>
no_step()
{
    return producer_step<_Type>();
}


///////////////////////////////////////////////////////////////////////////////
///             II.   INTERNAL PRODUCER HELPER CLASSES                      ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL
    // iterate_helper
    //   helper: infinite producer driven by repeated application of
    // a step function to a seed. The seed is the first value emitted;
    // each subsequent value is _step(previous).
    template<typename _Seed,
             typename _Step>
    class iterate_helper
    {
    public:
        using value_type = _Seed;
        using step_type  = producer_step<_Seed>;

        template<typename _SeedFwd,
                 typename _StepFwd>
        D_CONSTEXPR
        iterate_helper(
            _SeedFwd&& _seed,
            _StepFwd&& _step
        )
            : m_current(std::forward<_SeedFwd>(_seed))
            , m_step(std::forward<_StepFwd>(_step))
            , m_first(true)
        {}

        step_type operator()() const
        {
            if (m_first)
            {
                m_first = false;

                return make_step(m_current);
            }

            m_current = m_step(m_current);

            return make_step(m_current);
        }

    private:
        mutable _Seed m_current;
        _Step         m_step;
        mutable bool  m_first;
    };


    // unfold_helper
    //   helper: open-ended producer in which the step function
    // examines the current state and may either yield a value (and
    // a new state) or signal exhaustion. _Step has the signature
    // producer_step<pair<Value, State>>(State).
    template<typename _State,
             typename _Step,
             typename _Value>
    class unfold_helper
    {
    public:
        using value_type = _Value;
        using step_type  = producer_step<_Value>;

        template<typename _StateFwd,
                 typename _StepFwd>
        D_CONSTEXPR
        unfold_helper(
            _StateFwd&& _state,
            _StepFwd&&  _step
        )
            : m_state(std::forward<_StateFwd>(_state))
            , m_step(std::forward<_StepFwd>(_step))
            , m_done(false)
        {}

        step_type operator()() const
        {
            if (m_done)
            {
                return no_step<_Value>();
            }

            auto result = m_step(m_state);

            if (!result.has_value)
            {
                m_done = true;

                return no_step<_Value>();
            }

            m_state = result.value.second;

            return make_step(result.value.first);
        }

    private:
        mutable _State m_state;
        _Step          m_step;
        mutable bool   m_done;
    };


    // range_helper
    //   helper: numeric range from _start (inclusive) to _end
    // (exclusive) with optional step. Direction is determined by the
    // sign of _step; a zero step is treated as exhausted.
    template<typename _Int>
    class range_helper
    {
    public:
        using value_type = _Int;
        using step_type  = producer_step<_Int>;

        D_CONSTEXPR
        range_helper(
            _Int _start,
            _Int _end,
            _Int _step
        )
            : m_current(_start)
            , m_end(_end)
            , m_step(_step)
        {}

        step_type operator()() const
        {
            // exhausted if step is zero, or if direction would not
            // close the gap
            if ( (m_step == _Int(0)) ||
                 ( (m_step > _Int(0)) && (m_current >= m_end) ) ||
                 ( (m_step < _Int(0)) && (m_current <= m_end) ) )
            {
                return no_step<_Int>();
            }

            _Int v = m_current;
            m_current = static_cast<_Int>(m_current + m_step);

            return make_step(v);
        }

    private:
        mutable _Int m_current;
        _Int         m_end;
        _Int         m_step;
    };


    // repeat_helper
    //   helper: produces a stored value indefinitely.
    template<typename _Value>
    class repeat_helper
    {
    public:
        using value_type = _Value;
        using step_type  = producer_step<_Value>;

        template<typename _ValueFwd>
        explicit D_CONSTEXPR
        repeat_helper(
            _ValueFwd&& _value
        )
            : m_value(std::forward<_ValueFwd>(_value))
        {}

        step_type operator()() const
        {
            return make_step(m_value);
        }

    private:
        _Value m_value;
    };


    // repeat_n_helper
    //   helper: produces a stored value exactly _n times, then
    // signals exhaustion.
    template<typename _Value>
    class repeat_n_helper
    {
    public:
        using value_type = _Value;
        using step_type  = producer_step<_Value>;

        template<typename _ValueFwd>
        D_CONSTEXPR
        repeat_n_helper(
            _ValueFwd&& _value,
            std::size_t _n
        )
            : m_value(std::forward<_ValueFwd>(_value))
            , m_remaining(_n)
        {}

        step_type operator()() const
        {
            if (m_remaining == 0)
            {
                return no_step<_Value>();
            }

            --m_remaining;

            return make_step(m_value);
        }

    private:
        _Value                      m_value;
        mutable std::size_t         m_remaining;
    };


    // cycle_helper
    //   helper: cycles through a container forever. Holds the
    // container by value to avoid lifetime hazards. Yields exhaustion
    // immediately if the container is empty (avoiding a hang).
    template<typename _Container>
    class cycle_helper
    {
    public:
        using value_type = typename std::decay<decltype(
            *std::begin(std::declval<const _Container&>()))>::type;
        using step_type  = producer_step<value_type>;

        template<typename _ContainerFwd>
        explicit D_CONSTEXPR
        cycle_helper(
            _ContainerFwd&& _container
        )
            : m_container(std::forward<_ContainerFwd>(_container))
            , m_it(std::begin(m_container))
        {}

        step_type operator()() const
        {
            if (m_container.empty())
            {
                return no_step<value_type>();
            }

            if (m_it == std::end(m_container))
            {
                m_it = std::begin(m_container);
            }

            value_type v = *m_it;
            ++m_it;

            return make_step(std::move(v));
        }

    private:
        _Container m_container;
        mutable typename _Container::const_iterator m_it;
    };


    // generate_helper
    //   helper: invokes a nullary function on each pull and yields
    // its result. Useful for random numbers, time samples, etc.
    template<typename _Function>
    class generate_helper
    {
    public:
        using value_type = typename std::decay<
            decltype(std::declval<_Function&>()())>::type;
        using step_type  = producer_step<value_type>;

        template<typename _FunctionFwd>
        explicit D_CONSTEXPR
        generate_helper(
            _FunctionFwd&& _function
        )
            : m_function(std::forward<_FunctionFwd>(_function))
        {}

        step_type operator()() const
        {
            return make_step(m_function());
        }

    private:
        mutable _Function m_function;
    };


    // empty_producer_helper
    //   helper: produces no values. Useful as an identity for concat.
    template<typename _Type>
    struct empty_producer_helper
    {
        using value_type = _Type;
        using step_type  = producer_step<_Type>;

        D_CONSTEXPR
        step_type operator()() const
        {
            return no_step<_Type>();
        }
    };


    // single_helper
    //   helper: produces a single stored value, then signals
    // exhaustion. Equivalent to repeat_n(_value, 1) but cheaper.
    template<typename _Value>
    class single_helper
    {
    public:
        using value_type = _Value;
        using step_type  = producer_step<_Value>;

        template<typename _ValueFwd>
        explicit D_CONSTEXPR
        single_helper(
            _ValueFwd&& _value
        )
            : m_value(std::forward<_ValueFwd>(_value))
            , m_done(false)
        {}

        step_type operator()() const
        {
            if (m_done)
            {
                return no_step<_Value>();
            }

            m_done = true;

            return make_step(m_value);
        }

    private:
        _Value       m_value;
        mutable bool m_done;
    };


    // take_n_helper
    //   helper: forwards at most _n values from an inner producer,
    // then signals exhaustion regardless of whether the inner
    // producer is exhausted.
    template<typename _Producer>
    class take_n_helper
    {
    public:
        using value_type = typename _Producer::value_type;
        using step_type  = producer_step<value_type>;

        template<typename _ProducerFwd>
        D_CONSTEXPR
        take_n_helper(
            _ProducerFwd&& _producer,
            std::size_t    _n
        )
            : m_producer(std::forward<_ProducerFwd>(_producer))
            , m_remaining(_n)
        {}

        step_type operator()() const
        {
            if (m_remaining == 0)
            {
                return no_step<value_type>();
            }

            auto step = m_producer();

            if (!step.has_value)
            {
                m_remaining = 0;

                return step;
            }

            --m_remaining;

            return step;
        }

        // collect
        //   convenience: pulls all remaining values into a vector and
        // returns it. Equivalent to repeatedly invoking operator()
        // until exhaustion.
        std::vector<value_type> collect() const
        {
            std::vector<value_type> result;

            while (true)
            {
                auto step = (*this)();

                if (!step.has_value)
                {
                    break;
                }

                result.push_back(std::move(step.value));
            }

            return result;
        }

    private:
        mutable _Producer    m_producer;
        mutable std::size_t  m_remaining;
    };


    // drop_n_helper
    //   helper: pulls and discards _n values from an inner producer
    // on first use, then forwards subsequent pulls. If the inner
    // producer is exhausted within the first _n pulls, the wrapper
    // is also exhausted.
    template<typename _Producer>
    class drop_n_helper
    {
    public:
        using value_type = typename _Producer::value_type;
        using step_type  = producer_step<value_type>;

        template<typename _ProducerFwd>
        D_CONSTEXPR
        drop_n_helper(
            _ProducerFwd&& _producer,
            std::size_t    _n
        )
            : m_producer(std::forward<_ProducerFwd>(_producer))
            , m_to_drop(_n)
        {}

        step_type operator()() const
        {
            while (m_to_drop > 0)
            {
                auto step = m_producer();

                if (!step.has_value)
                {
                    m_to_drop = 0;

                    return step;
                }

                --m_to_drop;
            }

            return m_producer();
        }

    private:
        mutable _Producer    m_producer;
        mutable std::size_t  m_to_drop;
    };


    // concat_helper
    //   helper: emits all values of _first, then all values of
    // _second. _first and _second must have the same value_type.
    template<typename _First,
             typename _Second>
    class concat_helper
    {
    public:
        using value_type = typename _First::value_type;
        using step_type  = producer_step<value_type>;

        template<typename _FirstFwd,
                 typename _SecondFwd>
        D_CONSTEXPR
        concat_helper(
            _FirstFwd&&  _first,
            _SecondFwd&& _second
        )
            : m_first(std::forward<_FirstFwd>(_first))
            , m_second(std::forward<_SecondFwd>(_second))
            , m_first_done(false)
        {}

        step_type operator()() const
        {
            if (!m_first_done)
            {
                auto step = m_first();

                if (step.has_value)
                {
                    return step;
                }

                m_first_done = true;
            }

            return m_second();
        }

    private:
        mutable _First   m_first;
        mutable _Second  m_second;
        mutable bool     m_first_done;
    };


    // interleave_helper
    //   helper: alternates one pull from each of two producers. If
    // either producer is exhausted, the wrapper signals exhaustion
    // immediately (does NOT fall back to the longer one).
    template<typename _First,
             typename _Second>
    class interleave_helper
    {
    public:
        using value_type = typename _First::value_type;
        using step_type  = producer_step<value_type>;

        template<typename _FirstFwd,
                 typename _SecondFwd>
        D_CONSTEXPR
        interleave_helper(
            _FirstFwd&&  _first,
            _SecondFwd&& _second
        )
            : m_first(std::forward<_FirstFwd>(_first))
            , m_second(std::forward<_SecondFwd>(_second))
            , m_turn(false)
        {}

        step_type operator()() const
        {
            step_type step = m_turn ? m_second() : m_first();
            m_turn = !m_turn;

            return step;
        }

    private:
        mutable _First   m_first;
        mutable _Second  m_second;
        mutable bool     m_turn;
    };


    // transform_helper
    //   helper: applies a function to each value emitted by an inner
    // producer. value_type is the result of the function applied to
    // the inner value_type.
    template<typename _Producer,
             typename _Function>
    class transform_helper
    {
    public:
        using source_type = typename _Producer::value_type;
        using value_type  = typename std::decay<decltype(
            std::declval<_Function&>()(
                std::declval<const source_type&>()))>::type;
        using step_type   = producer_step<value_type>;

        template<typename _ProducerFwd,
                 typename _FunctionFwd>
        D_CONSTEXPR
        transform_helper(
            _ProducerFwd&& _producer,
            _FunctionFwd&& _function
        )
            : m_producer(std::forward<_ProducerFwd>(_producer))
            , m_function(std::forward<_FunctionFwd>(_function))
        {}

        step_type operator()() const
        {
            auto step = m_producer();

            if (!step.has_value)
            {
                return no_step<value_type>();
            }

            return make_step(m_function(step.value));
        }

    private:
        mutable _Producer m_producer;
        mutable _Function m_function;
    };


    // filter_helper
    //   helper: forwards only those values from an inner producer
    // that satisfy a predicate. Pulls repeatedly from the inner
    // producer until a match or exhaustion is found.
    template<typename _Producer,
             typename _Predicate>
    class filter_helper
    {
    public:
        using value_type = typename _Producer::value_type;
        using step_type  = producer_step<value_type>;

        template<typename _ProducerFwd,
                 typename _PredicateFwd>
        D_CONSTEXPR
        filter_helper(
            _ProducerFwd&&  _producer,
            _PredicateFwd&& _predicate
        )
            : m_producer(std::forward<_ProducerFwd>(_producer))
            , m_predicate(std::forward<_PredicateFwd>(_predicate))
        {}

        step_type operator()() const
        {
            while (true)
            {
                auto step = m_producer();

                if (!step.has_value)
                {
                    return step;
                }

                if (m_predicate(step.value))
                {
                    return step;
                }
            }
        }

    private:
        mutable _Producer  m_producer;
        mutable _Predicate m_predicate;
    };

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///             III.  PRODUCER FACTORIES                                    ///
///////////////////////////////////////////////////////////////////////////////

namespace producers
{

    // iterate
    //   function: infinite producer iterate(x, f) yielding x, f(x),
    // f(f(x)), and so on. Useful for arithmetic progressions,
    // recurrences, and other deterministic sequences.
    template<typename _Seed,
             typename _Step>
    D_CONSTEXPR
    internal::iterate_helper<typename std::decay<_Seed>::type,
                             typename std::decay<_Step>::type>
    iterate
    (
        _Seed&& _seed,
        _Step&& _step
    )
    {
        return internal::iterate_helper<
            typename std::decay<_Seed>::type,
            typename std::decay<_Step>::type>(
                std::forward<_Seed>(_seed),
                std::forward<_Step>(_step));
    }


    // unfold
    //   function: builds a producer from a state seed and a step
    // function of signature producer_step<pair<Value, State>>(State).
    // Returning an empty step signals exhaustion. _Value must be
    // supplied explicitly because the step's return type is generally
    // not deducible from the state alone.
    template<typename _Value,
             typename _State,
             typename _Step>
    D_CONSTEXPR
    internal::unfold_helper<typename std::decay<_State>::type,
                            typename std::decay<_Step>::type,
                            _Value>
    unfold
    (
        _State&& _state,
        _Step&&  _step
    )
    {
        return internal::unfold_helper<
            typename std::decay<_State>::type,
            typename std::decay<_Step>::type,
            _Value>(
                std::forward<_State>(_state),
                std::forward<_Step>(_step));
    }


    // range
    //   function: numeric range producer. Emits _start, _start + _step,
    // ..., stopping when _step's direction would not bring the value
    // closer to _end. _step may be negative for descending ranges.
    template<typename _Int>
    D_CONSTEXPR
    internal::range_helper<_Int>
    range
    (
        _Int _start,
        _Int _end,
        _Int _step
    )
    {
        return internal::range_helper<_Int>(_start, _end, _step);
    }


    // range (default step)
    //   function: half-open range [_start, _end) with step = 1.
    template<typename _Int>
    D_CONSTEXPR
    internal::range_helper<_Int>
    range
    (
        _Int _start,
        _Int _end
    )
    {
        return internal::range_helper<_Int>(_start, _end, _Int(1));
    }


    // iota
    //   function: alias for range with start and end, matching the
    // STL <numeric>/<ranges> naming convention.
    template<typename _Int>
    D_CONSTEXPR
    internal::range_helper<_Int>
    iota
    (
        _Int _start,
        _Int _end
    )
    {
        return internal::range_helper<_Int>(_start, _end, _Int(1));
    }


    // repeat
    //   function: infinite producer emitting _value on every pull.
    // Pair with take_n or a bounded consumer to make it finite.
    template<typename _Value>
    D_CONSTEXPR
    internal::repeat_helper<typename std::decay<_Value>::type>
    repeat
    (
        _Value&& _value
    )
    {
        return internal::repeat_helper<typename std::decay<_Value>::type>(
            std::forward<_Value>(_value));
    }


    // repeat_n
    //   function: emits _value exactly _n times, then exhausts.
    template<typename _Value>
    D_CONSTEXPR
    internal::repeat_n_helper<typename std::decay<_Value>::type>
    repeat_n
    (
        _Value&&     _value,
        std::size_t  _n
    )
    {
        return internal::repeat_n_helper<typename std::decay<_Value>::type>(
            std::forward<_Value>(_value), _n);
    }


    // cycle
    //   function: infinite producer that emits the elements of
    // _container in order, restarting from the beginning each time
    // the end is reached. Empty containers exhaust immediately.
    template<typename _Container>
    D_CONSTEXPR
    internal::cycle_helper<typename std::decay<_Container>::type>
    cycle
    (
        _Container&& _container
    )
    {
        return internal::cycle_helper<
            typename std::decay<_Container>::type>(
                std::forward<_Container>(_container));
    }


    // generate
    //   function: infinite producer that invokes a nullary function
    // on every pull and emits its result.
    template<typename _Function>
    D_CONSTEXPR
    internal::generate_helper<typename std::decay<_Function>::type>
    generate
    (
        _Function&& _function
    )
    {
        return internal::generate_helper<
            typename std::decay<_Function>::type>(
                std::forward<_Function>(_function));
    }


    // empty
    //   function: producer that yields no values. The element type
    // must be explicit since there is no other source for it.
    template<typename _Type>
    D_CONSTEXPR
    internal::empty_producer_helper<_Type>
    empty()
    {
        return internal::empty_producer_helper<_Type>{};
    }


    // single
    //   function: producer that emits _value once and then exhausts.
    template<typename _Value>
    D_CONSTEXPR
    internal::single_helper<typename std::decay<_Value>::type>
    single
    (
        _Value&& _value
    )
    {
        return internal::single_helper<typename std::decay<_Value>::type>(
            std::forward<_Value>(_value));
    }


    // take_n
    //   function: bounds an inner producer to its first _n outputs.
    // The resulting producer also exposes a .collect() method for
    // direct conversion to vector.
    template<typename _Producer>
    D_CONSTEXPR
    internal::take_n_helper<typename std::decay<_Producer>::type>
    take_n
    (
        _Producer&&  _producer,
        std::size_t  _n
    )
    {
        return internal::take_n_helper<
            typename std::decay<_Producer>::type>(
                std::forward<_Producer>(_producer), _n);
    }


    // drop_n
    //   function: discards the first _n outputs of an inner producer
    // and forwards the rest.
    template<typename _Producer>
    D_CONSTEXPR
    internal::drop_n_helper<typename std::decay<_Producer>::type>
    drop_n
    (
        _Producer&&  _producer,
        std::size_t  _n
    )
    {
        return internal::drop_n_helper<
            typename std::decay<_Producer>::type>(
                std::forward<_Producer>(_producer), _n);
    }


    // concat
    //   function: sequence two producers; emits all of _first, then
    // all of _second.
    template<typename _First,
             typename _Second>
    D_CONSTEXPR
    internal::concat_helper<typename std::decay<_First>::type,
                            typename std::decay<_Second>::type>
    concat
    (
        _First&&  _first,
        _Second&& _second
    )
    {
        return internal::concat_helper<
            typename std::decay<_First>::type,
            typename std::decay<_Second>::type>(
                std::forward<_First>(_first),
                std::forward<_Second>(_second));
    }


    // interleave
    //   function: alternate one pull from each of two producers.
    // Exhausts as soon as either inner producer exhausts.
    template<typename _First,
             typename _Second>
    D_CONSTEXPR
    internal::interleave_helper<typename std::decay<_First>::type,
                                typename std::decay<_Second>::type>
    interleave
    (
        _First&&  _first,
        _Second&& _second
    )
    {
        return internal::interleave_helper<
            typename std::decay<_First>::type,
            typename std::decay<_Second>::type>(
                std::forward<_First>(_first),
                std::forward<_Second>(_second));
    }


    // transform
    //   function: producer wrapper that applies _function to each
    // value emitted by _producer.
    template<typename _Producer,
             typename _Function>
    D_CONSTEXPR
    internal::transform_helper<typename std::decay<_Producer>::type,
                               typename std::decay<_Function>::type>
    transform
    (
        _Producer&& _producer,
        _Function&& _function
    )
    {
        return internal::transform_helper<
            typename std::decay<_Producer>::type,
            typename std::decay<_Function>::type>(
                std::forward<_Producer>(_producer),
                std::forward<_Function>(_function));
    }


    // filter
    //   function: producer wrapper that emits only those inner values
    // satisfying _predicate. Each pull may invoke the inner producer
    // multiple times until a match (or exhaustion) is found.
    template<typename _Producer,
             typename _Predicate>
    D_CONSTEXPR
    internal::filter_helper<typename std::decay<_Producer>::type,
                            typename std::decay<_Predicate>::type>
    filter
    (
        _Producer&&  _producer,
        _Predicate&& _predicate
    )
    {
        return internal::filter_helper<
            typename std::decay<_Producer>::type,
            typename std::decay<_Predicate>::type>(
                std::forward<_Producer>(_producer),
                std::forward<_Predicate>(_predicate));
    }


    // from_container
    //   function: turns any iterable container into a finite producer.
    // The container is held by value; pass by std::move to avoid a
    // copy. The producer exhausts when the iterator reaches end().
    template<typename _Container>
    class from_container_producer
    {
    public:
        using container_type = typename std::decay<_Container>::type;
        using value_type     = typename std::decay<decltype(
            *std::begin(std::declval<const container_type&>()))>::type;
        using step_type      = producer_step<value_type>;

        template<typename _ContainerFwd>
        explicit D_CONSTEXPR
        from_container_producer(
            _ContainerFwd&& _container
        )
            : m_container(std::forward<_ContainerFwd>(_container))
            , m_it(std::begin(m_container))
            , m_end(std::end(m_container))
        {}

        step_type operator()() const
        {
            if (m_it == m_end)
            {
                return no_step<value_type>();
            }

            value_type v = *m_it;
            ++m_it;

            return make_step(std::move(v));
        }

    private:
        container_type m_container;
        mutable typename container_type::const_iterator m_it;
        typename container_type::const_iterator         m_end;
    };

    // from_container (factory)
    template<typename _Container>
    D_CONSTEXPR
    from_container_producer<typename std::decay<_Container>::type>
    from_container
    (
        _Container&& _container
    )
    {
        return from_container_producer<
            typename std::decay<_Container>::type>(
                std::forward<_Container>(_container));
    }

}   // namespace producers


///////////////////////////////////////////////////////////////////////////////
///             IV.   TERMINAL CONVENIENCES                                 ///
///////////////////////////////////////////////////////////////////////////////

// collect
//   function: drains a producer (which must be finite or already
// bounded) into a std::vector. Pulls until the producer signals
// exhaustion.
template<typename _Producer>
D_NODISCARD
std::vector<typename _Producer::value_type>
collect
(
    _Producer& _producer
)
{
    std::vector<typename _Producer::value_type> result;

    while (true)
    {
        auto step = _producer();

        if (!step.has_value)
        {
            break;
        }

        result.push_back(std::move(step.value));
    }

    return result;
}


// for_each (producer)
//   function: pulls every value from _producer and forwards it to
// _consumer, until the producer signals exhaustion.
template<typename _Producer,
         typename _Consumer>
void
for_each
(
    _Producer& _producer,
    _Consumer  _consumer
)
{
    while (true)
    {
        auto step = _producer();

        if (!step.has_value)
        {
            break;
        }

        _consumer(step.value);
    }

    return;
}

// fold (producer)
//   function: drains a producer through a binary step function and
// accumulator, returning the final accumulated value.
template<typename _Producer,
         typename _Acc,
         typename _Step>
D_NODISCARD _Acc
fold
(
    _Producer& _producer,
    _Acc       _init,
    _Step      _step
)
{
    while (true)
    {
        auto step = _producer();

        if (!step.has_value)
        {
            break;
        }

        _init = _step(static_cast<const _Acc&>(_init), step.value);
    }

    return _init;
}

NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_PRODUCER_