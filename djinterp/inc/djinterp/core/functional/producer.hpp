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
*   auto fibs = iterate(std::make_pair(0, 1),
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
*   auto first_ten = take_n(fibs, 10).collect();
*
*   // sequencing: 1..3 then 100..102
*   auto seq = concat(range(1, 4), range(100, 103));
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
III.  PRODUCER FACTORIES   (flat in djinterp)
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
V.    PRODUCER TRAITS & CONCEPTS
      1.  is_producer_step<T>                 (is T a producer_step?)
      2.  is_producer<T>                      (does T model the protocol?)
      3.  producer_value_type<T>              (the emitted value type)
      4.  producer_step_type / producer       (C++20 concepts)
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
#include "./function_traits.hpp"
#include "./functor.hpp"
#include "./foldable.hpp"
#include "../meta/carrier.hpp"      // val_t / type_t leaves for the compile-time unfold
#include "../meta/value_list.hpp"   // materialization target of the compile-time unfold


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
    // producer_base
    //   CRTP mixin giving every producer helper a uniform collect()
    //   terminal. Previously collect() was defined only on a few helpers
    //   (take_n / drop_n / single), so range(...).collect(),
    //   transform(...).collect(), etc. did not compile despite the
    //   documented `.collect()` usage. Deriving every helper from this
    //   base provides collect() once, in terms of the derived operator().
    //   (added 2026-05-30)
    template<typename _Derived>
    class producer_base
    {
    public:
        // collect
        //   pulls all remaining values into a vector by repeatedly
        // invoking the derived producer until exhaustion.
        template<typename _D = _Derived>
        std::vector<typename _D::value_type>
        collect() const
        {
            const _Derived& self = static_cast<const _Derived&>(*this);
            std::vector<typename _D::value_type> result;

            while (true)
            {
                auto step = self();

                if (!step.has_value)
                {
                    break;
                }

                result.push_back(std::move(step.value));
            }

            return result;
        }
    };

    // iterate_helper
    //   helper: infinite producer driven by repeated application of
    // a step function to a seed. The seed is the first value emitted;
    // each subsequent value is _step(previous).
    template<typename _Seed,
             typename _Step>
    class iterate_helper
        : public producer_base<iterate_helper<_Seed, _Step>>
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
        : public producer_base<unfold_helper<_State, _Step, _Value>>
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
        : public producer_base<range_helper<_Int>>
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
        : public producer_base<repeat_helper<_Value>>
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
        : public producer_base<repeat_n_helper<_Value>>
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
        : public producer_base<cycle_helper<_Container>>
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
        : public producer_base<generate_helper<_Function>>
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
        : public producer_base<empty_producer_helper<_Type>>
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
        : public producer_base<single_helper<_Value>>
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
        : public producer_base<take_n_helper<_Producer>>
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
        : public producer_base<drop_n_helper<_Producer>>
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
        : public producer_base<concat_helper<_First, _Second>>
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
        : public producer_base<interleave_helper<_First, _Second>>
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
        : public producer_base<transform_helper<_Producer, _Function>>
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
        : public producer_base<filter_helper<_Producer, _Predicate>>
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

// NOTE: the producer factories below were previously nested in a
// `namespace producers` sub-namespace; they are now flat in djinterp so
// callers write `iterate(...)`, `range(...)`, etc. directly. The internal
// helper classes they return remain in `djinterp::internal`.

    // iterate
    //   function: infinite producer iterate(x, f) yielding x, f(x),
    // f(f(x)), and so on. Useful for arithmetic progressions,
    // recurrences, and other deterministic sequences.
    template<typename _Seed,
             typename _Step>
    D_CONSTEXPR
    internal::iterate_helper<typename std::decay<_Seed>::type,
                             typename std::decay<_Step>::type>
    iterate(
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
    unfold(
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
    range(
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
    range(
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
    iota(
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
    repeat(
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
    repeat_n(
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
    cycle(
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
    generate(
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
    single(
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
    take_n(
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
    drop_n(
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
    concat(
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
    interleave(
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
    transform(
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
    filter(
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
        : public internal::producer_base<from_container_producer<_Container> >
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
    from_container(
        _Container&& _container
    )
    {
        return from_container_producer<
            typename std::decay<_Container>::type>(
                std::forward<_Container>(_container));
    }


///////////////////////////////////////////////////////////////////////////////
///             IV.   TERMINAL CONVENIENCES                                 ///
///////////////////////////////////////////////////////////////////////////////

// collect
//   function: drains a producer (which must be finite or already
// bounded) into a std::vector. Pulls until the producer signals
// exhaustion.
template<typename _Producer>
D_NODISCARD std::vector<typename _Producer::value_type>
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


///////////////////////////////////////////////////////////////////////////////
///             V.    PRODUCER TRAITS & CONCEPTS                            ///
///////////////////////////////////////////////////////////////////////////////
// SFINAE-friendly structural introspection of the producer protocol. A
// "producer" is a const-invocable nullary callable whose result is a
// producer_step<V>, and which advertises that V via a nested value_type.
// These traits surface that contract as first-class, and the C++20 concepts
// wrap them for use in requires-clauses.

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

NS_INTERNAL

    // producer_make_void
    //   trait: header-local map from any type sequence to void; the
    // foundation for the detection idiom below. Kept local so the trait
    // block carries no dependency on an external void_t facility.
    template<typename...>
    struct producer_make_void
    {
        typedef void type;
    };

    // producer_void_t
    //   type: header-local alias for producer_make_void<...>::type.
    template<typename... _Types>
    using producer_void_t = typename producer_make_void<_Types...>::type;


    // is_producer_step_helper
    //   trait: detects whether _Type is a producer_step specialization
    // (primary / failure case).
    template<typename _Type>
    struct is_producer_step_helper : std::false_type
    {};

    // is_producer_step_helper<producer_step<_Value>>
    //   trait: success specialization for producer_step.
    template<typename _Value>
    struct is_producer_step_helper<producer_step<_Value> > : std::true_type
    {};


    // is_producer_helper
    //   trait: detects whether _Type satisfies the producer protocol --
    // it has a nested value_type, is const-invocable with no arguments,
    // and the call result is a producer_step. Primary / failure case.
    template<typename _Type,
             typename _AlwaysVoid = void>
    struct is_producer_helper : std::false_type
    {};

    // is_producer_helper (success case)
    //   trait: specialization that fires only when every protocol element
    // is well-formed.
    template<typename _Type>
    struct is_producer_helper<
        _Type,
        producer_void_t<
            typename _Type::value_type,
            decltype(std::declval<const _Type&>()())> >
        : is_producer_step_helper<
              typename std::decay<
                  decltype(std::declval<const _Type&>()())>::type>
    {};

NS_END  // internal


// is_producer_step
//   trait: true if _Type (decayed) is a producer_step specialization.
template<typename _Type>
struct is_producer_step
    : internal::is_producer_step_helper<typename std::decay<_Type>::type>
{};


// is_producer
//   trait: true if _Type (decayed) models the producer protocol: a nested
// value_type, const-nullary-invocable, returning a producer_step.
template<typename _Type>
struct is_producer
    : internal::is_producer_helper<typename std::decay<_Type>::type>
{};


// producer_value_type
//   trait: extracts the value_type a producer emits. Only well-formed when
// is_producer<_Type>::value is true; intended for use behind that guard.
template<typename _Type>
struct producer_value_type
{
    typedef typename std::decay<_Type>::type::value_type type;
};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_producer_step_v
    //   value: convenience alias for is_producer_step<_Type>::value.
    template<typename _Type>
    static constexpr bool is_producer_step_v =
        is_producer_step<_Type>::value;

    // is_producer_v
    //   value: convenience alias for is_producer<_Type>::value.
    template<typename _Type>
    static constexpr bool is_producer_v = is_producer<_Type>::value;
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    // producer_step_type
    //   concept: satisfied by any producer_step specialization.
    template<typename _Type>
    concept producer_step_type = is_producer_step<_Type>::value;

    // producer
    //   concept: satisfied by any type modeling the producer protocol.
    template<typename _Type>
    concept producer = is_producer<_Type>::value;

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#if D_ENV_LANG_IS_CPP17_OR_HIGHER
///////////////////////////////////////////////////////////////////////////////
///             X.    COMPILE-TIME UNFOLD DRIVER                            ///
///////////////////////////////////////////////////////////////////////////////
//   The producers above are the RUNTIME source driver: a stateful pull source
// whose unfold step has the shape  State -> producer_step<pair<Value, State>>
// (see unfold_helper), materialized eagerly by producer_base::collect().  This
// section adds the COMPILE-TIME driver: it runs a pure unfold step over carrier
// states (val_t / type_t) to a fixed point during translation, materializing
// the produced value carriers into a value_list - the same source description
// drives both a runtime computation and a constant-evaluated one.
//
//   The compile-time step result is a DISTINCT type per case (some_t vs none_t)
// rather than producer_step<...>: producer_step records exhaustion in a runtime
// bool field, which a type-level driver cannot branch on, whereas some_t /
// none_t put the decision in the type so template recursion can dispatch on it.
// A step is an ordinary constexpr callable that returns one or the other, e.g.
//
//   template<auto _Limit>
//   struct iota_step
//   {
//       template<auto _I>
//       D_CONSTEXPR auto operator()(val_t<_I>) const
//       {
//           if constexpr (_I < _Limit) return some_t<val_t<_I>, val_t<_I + 1>>{};
//           else                       return none_t{};
//       }
//   };
//   using first_five = unfold_ct_t<iota_step<5>, val_t<0>>;  // value_list<0,1,2,3,4>

// none_t
//   compile-time unfold step result: the source is exhausted.
struct none_t
{};

// some_t
//   compile-time unfold step result: a value carrier _Value together with the
// next state carrier _Next.  Distinct from none_t at the type level so the
// driver can branch by type.
template<typename _Value,
         typename _Next>
struct some_t
{
    using value = _Value;
    using next  = _Next;
};

NS_INTERNAL

    // unfold_ct_helper
    //   metafunction: type-level recursion that drives a constexpr unfold _Step
    // from a state to exhaustion, growing the accumulated value_list _Acc.
    // Dispatched on the step's result type _StepResult (none_t or some_t<V,N>).
    template<typename _Step,
             typename _Acc,
             typename _StepResult>
    struct unfold_ct_helper;

    // exhausted: the accumulated list is the result.
    template<typename _Step,
             typename _Acc>
    struct unfold_ct_helper<_Step, _Acc, none_t>
    {
        using type = _Acc;
    };

    // yielded value carrier _Value with next state carrier _Next: append the
    // value to the list and recurse on the next state.
    template<typename _Step,
             typename _Acc,
             typename _Value,
             typename _Next>
    struct unfold_ct_helper<_Step, _Acc, some_t<_Value, _Next>>
    {
        using grown = decltype(
            append(std::declval<_Acc>(), std::declval<_Value>()));

        using type = typename unfold_ct_helper<
            _Step,
            grown,
            decltype(std::declval<const _Step&>()(std::declval<_Next>()))
        >::type;
    };

NS_END

// unfold_ct_t
//   type: the value_list produced by running constexpr unfold _Step from the
// initial state carrier _Init to exhaustion at compile time.
template<typename _Step,
         typename _Init>
using unfold_ct_t = typename internal::unfold_ct_helper<
    _Step,
    value_list<>,
    decltype(std::declval<const _Step&>()(std::declval<_Init>()))
>::type;

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER




///////////////////////////////////////////////////////////////////////////////
///             XI.   FUNCTOR INSTANCE                                      ///
///////////////////////////////////////////////////////////////////////////////
//   A producer is a Functor: the flat transform(producer, f) is its map. This
// teaches the generic functor_map (functor.hpp) to drive a producer through
// the one canonical name, so the same call that maps a maybe / result / view
// also maps a producer. A producer's mapped type is
// internal::transform_helper<P, F> -- it depends on the mapping function F, so
// there is no single F<U> to rebind; the result type follows from the existing
// transform and is deduced. Keyed on is_producer, mutually exclusive with the
// monad bridge in functor.hpp (a producer is not a monad) and the view
// instance.

template<typename _Producer>
struct functor_traits<
    _Producer,
    typename std::enable_if<is_producer<_Producer>::value>::type>
{
    using is_specialized = std::true_type;
    using value_type     = typename producer_value_type<_Producer>::type;

    // map
    //   functorial map by delegating to the flat transform combinator (the
    // existing per-type fmap for producers). Lazy: the wrapped producer is
    // pulled only when the result producer is invoked.
    template<typename _ProducerArg,
             typename _Function>
    static
    D_CONSTEXPR
    auto map(
        _ProducerArg&& _producer,
        _Function&&    _function
    )
    -> decltype(::djinterp::transform(
           std::forward<_ProducerArg>(_producer),
           std::forward<_Function>(_function)))
    {
        return ::djinterp::transform(
            std::forward<_ProducerArg>(_producer),
            std::forward<_Function>(_function));
    }
};


///////////////////////////////////////////////////////////////////////////////
///             XII.  FOLDABLE INSTANCE                                     ///
///////////////////////////////////////////////////////////////////////////////
//   A producer is a Foldable: its elements are collapsed by pulling the source
// to exhaustion and threading the reducer through them. This teaches the
// generic fold_left (foldable.hpp) -- and therefore fold_to_vector,
// fold_length, fold_any, fold_all, fold_right, ... -- to drive a producer
// through the one canonical name. Keyed on is_producer so the single instance
// covers every producer; mutually exclusive with the maybe / result / view
// instances. The fold is non-destructive (a copy is pulled, leaving the
// caller's producer untouched); an infinite producer must be bounded before
// folding.

template<typename _Producer>
struct foldable_traits<
    _Producer,
    typename std::enable_if<is_producer<_Producer>::value>::type>
{
    using is_specialized = std::true_type;
    using value_type     = typename producer_value_type<_Producer>::type;

    // fold_left
    //   strict left fold by pulling a copy of the producer to exhaustion; the
    // accumulator is threaded by move so collecting folds stay O(n).
    //   D_CONSTEXPR -- a producer is not a literal type before C++20, and
    // the pull loop needs relaxed constexpr.
    template<typename _Acc,
             typename _Function>
    static
    D_CONSTEXPR
    _Acc fold_left(
        const _Producer& _producer,
        _Acc             _init,
        _Function        _function
    )
    {
        _Producer _cursor = _producer;

        while (true)
        {
            producer_step<value_type> _step = _cursor();

            if (!_step.has_value)
            {
                break;
            }

            _init = _function(std::move(_init), _step.value);
        }

        return _init;
    }
};


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_PRODUCER_