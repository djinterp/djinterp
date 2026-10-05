/*******************************************************************************
* djinterp [core]                                                   consumer.hpp
*
* First-class consumers (sinks) for functional dataflow (C++).
*   A consumer is any callable of signature `void(const A&)`. This module
* lifts consumers into first-class values that can be combined, adapted,
* filtered, mapped, and broadcast. Consumers complement producers and
* accumulators: where accumulators absorb a stream and yield a value,
* consumers absorb a stream and produce only side effects (logging,
* writing, counting through an out-parameter, etc.).
*   All combinators are fully typed and SFINAE-constrained. Type erasure
* via std::function is opt-in (see boxed_consumer) for cases where a
* concrete consumer type is required at compile time, e.g. for storage
* in heterogeneous containers.
*   The predicate SFINAE structural traits and C++20 concepts in
* Section 0 describe the consumer vocabulary (consumer-ness, predicate-
* ness, transformer-ness, boxability, and contramap result type) so the
* combinators and downstream code can constrain and introspect on it.
*
* USAGE:
*   // primitives
*   auto print = consumers::print_to(std::cout);
*   auto store = consumers::write_to(my_vector);
*   // adapters
*   auto evens_only = consumers::filtered(print,
*                                         [](int x){ return x % 2 == 0; });
*   auto doubled    = consumers::mapped(print, [](int x){ return x * 2; });
*   // broadcast
*   auto both = consumers::tee(print, store);
*   // batching: only fires every N elements
*   auto batched_flush = consumers::batched(print, 100);
*   // application
*   for (int x : data) { both(x); }
*
*
* path:      /inc/djinterp/core/functional/consumer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.20
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    PREDICATE SFINAE STRUCTURAL TRAITS & CONCEPTS
      ---------------------------------------------

I.    INTERNAL CONSUMER HELPER CLASSES
      --------------------------------
      1.    print_to_helper
      2.    write_to_helper
      3.    discard_helper
      4.    count_into_helper
      5.    ref_consumer_helper                 (consumer<T>& as callable)
      6.    filtered_consumer_helper
      7.    mapped_consumer_helper              (contramap)
      8.    tee_consumer_helper                 (variadic broadcast)
      9.    batched_consumer_helper             (every-N invocation)
      10.   take_consumer_helper                (first-N then ignore)
      11.   drop_consumer_helper                (skip first-N)
      12.   conditional_consumer_helper         (predicate branch)
      13.   fallback_consumer_helper            (try primary, else secondary)

II.   CONSUMER FACTORIES  (namespace consumers)
      -----------------------------------------
      1.    print_to                            (stream sink)
      2.    write_to                            (container append)
      3.    discard                             (null sink)
      4.    count_into                          (increment counter)
      5.    filtered                            (predicate gate)
      6.    mapped                              (input transform / contramap)
      7.    tee                                 (broadcast)
      8.    batched
      9.    take
      10.   drop
      11.   conditional
      12.   fallback

III.  TYPE ERASURE
      ------------
      1.    boxed_consumer<T>                   (std::function<void(const T&)>)
      2.    box                                 (any consumer -> boxed_consumer)
*/


#ifndef DJINTERP_FUNCTIONAL_CONSUMER_HPP
#define DJINTERP_FUNCTIONAL_CONSUMER_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <functional>
#include <iterator>
#include <ostream>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "../meta/type_utility.hpp"  // void_t
#include "./functional_common.hpp"   // is_predicate
NS_DJINTERP


//   DUAL DOMAIN (boundary).  A consumer's purpose is a side effect - printing,
// writing, counting through an out-parameter - which is inherently a RUNTIME
// act and has no constant-evaluation analog.  What lifts to compile time is
// COMPOSITION: the adapters below (filtered / mapped / tee / batched / take /
// drop / conditional) are D_CONSTEXPR_CPP14-constructible, so a consumer pipeline
// can be assembled in a constant expression, and the values threaded through it
// may be carrier leaves (val_t / type_t) as readily as ordinary values - only
// the terminal effect is deferred to run time.  A consumer is thus the
// value-domain RUNTIME sink of the dataflow; there is deliberately no
// type-level or compile-time "consumption", because a side effect cannot occur
// during translation.

///////////////////////////////////////////////////////////////////////////////
///             0.    PREDICATE SFINAE STRUCTURAL TRAITS & CONCEPTS         ///
///////////////////////////////////////////////////////////////////////////////
//   Self-contained detection vocabulary for the consumer machinery.
// Every predicate reduces to a `static constexpr bool value` (or, for
// the type-yielding traits, a `::type`), built on the core ::void_t
// SFINAE sink declared in djinterp.hpp.  A consumer is a callable of
// shape `void(const T&)`; a predicate is a callable `const T& -> bool`
// (functional_common.hpp's is_predicate<P, const T&>, the module's one
// predicate trait); a transformer is a callable `const T& -> non-void`.
// The C++20 concept mirrors follow at the end of the section, gated on
// concept support.

NS_INTERNAL

    // consumer_call_result_helper
    //   trait: SFINAE result-type extractor for the call expression
    // Function(const Arg&) (primary: no `type`, soft failure).
    template<typename AlwaysVoid,
             typename Function,
             typename Arg>
    struct consumer_call_result_helper
    {};

    // consumer_call_result_helper (well-formed specialization)
    //   trait: yields the result type of Function(const Arg&) when
    // that call expression is well-formed.
    template<typename Function,
             typename Arg>
    struct consumer_call_result_helper<
        void_t<decltype(std::declval<const Function&>()(
            std::declval<const Arg&>()))>,
        Function,
        Arg>
    {
        using type = decltype(std::declval<const Function&>()(
            std::declval<const Arg&>()));
    };

    // is_callable_with_const_ref_helper
    //   trait: detection sink for Function(const Arg&) (primary:
    // false).
    template<typename AlwaysVoid,
             typename Function,
             typename Arg>
    struct is_callable_with_const_ref_helper : std::false_type
    {};

    // is_callable_with_const_ref_helper (well-formed specialization)
    //   trait: true when Function(const Arg&) is a valid call.
    template<typename Function,
             typename Arg>
    struct is_callable_with_const_ref_helper<
        void_t<decltype(std::declval<const Function&>()(
            std::declval<const Arg&>()))>,
        Function,
        Arg> : std::true_type
    {};

    // is_consumer_result_helper
    //   trait: void-result branch of is_consumer, selected by the
    // Callable flag.  Primary (false flag): not callable, so the
    // result type is never named -- guarantees SFINAE-safety.
    template<bool     Callable,
             typename Consumer,
             typename Type>
    struct is_consumer_result_helper : std::false_type
    {};

    // is_consumer_result_helper (callable branch)
    //   trait: when callable, the value is whether the call result is
    // void (the defining shape of a consumer sink).
    template<typename Consumer,
             typename Type>
    struct is_consumer_result_helper<true, Consumer, Type>
    {
        static constexpr bool value =
            std::is_void<
                typename consumer_call_result_helper<void, Consumer, Type>::type
            >::value;
    };

    // is_transformer_result_helper
    //   trait: non-void-result branch of is_transformer, selected by the
    // Callable flag.  Primary (false flag): not callable, so the result
    // type is never named -- guarantees SFINAE-safety.
    template<bool     Callable,
             typename Function,
             typename Type>
    struct is_transformer_result_helper : std::false_type
    {};

    // is_transformer_result_helper (callable branch)
    //   trait: when callable, the value is whether the call result is
    // non-void.
    template<typename Function,
             typename Type>
    struct is_transformer_result_helper<true, Function, Type>
    {
        static constexpr bool value =
            !std::is_void<
                typename consumer_call_result_helper<void, Function, Type>::type
            >::value;
    };

NS_END  // internal

// consumer_result
//   trait: result type of invoking Consumer with a `const Type&`.
// Has a `::type` member only when that call expression is well-formed,
// making consumer_result_t SFINAE-friendly.
template<typename Consumer,
         typename Type>
struct consumer_result
{
    using type =
        typename internal::consumer_call_result_helper<void, Consumer, Type>::type;
};

// consumer_result_t
//   type: convenience alias for consumer_result<...>::type.
template<typename Consumer,
         typename Type>
using consumer_result_t = typename consumer_result<Consumer, Type>::type;

// is_consumer
//   trait: true when Consumer is callable as `void(const Type&)` --
// i.e. accepts a `const Type&` and returns void.  This is the defining
// structural property of a consumer sink.
template<typename Consumer,
         typename Type>
struct is_consumer
    : internal::is_consumer_result_helper<
          internal::is_callable_with_const_ref_helper<
              void, Consumer, Type>::value,
          Consumer, Type>
{};

// (the predicate shape filtered / conditional require -- callable with a
// `const Type&`, the result convertible to bool -- is functional_common.hpp's
// is_predicate<Predicate, const Type&>; this header used to define its own
// is_predicate<Predicate, Type>, which no translation unit could include
// beside functional_common.hpp's.)

// is_transformer
//   trait: true when Function is callable with a `const Type&` and
// produces a non-void result (the shape mapped / contramap require).
template<typename Function,
         typename Type>
struct is_transformer
    : internal::is_transformer_result_helper<
          internal::is_callable_with_const_ref_helper<
              void, Function, Type>::value,
          Function, Type>
{};

// is_boxable
//   trait: true when Consumer can be type-erased into a
// boxed_consumer<Type> -- i.e. it is convertible to
// std::function<void(const Type&)>.  Every value that satisfies
// is_consumer<Consumer, Type> is boxable, but the check is stated in
// terms of the std::function target so it also covers plain function
// pointers and lambdas directly.
template<typename Consumer,
         typename Type>
struct is_boxable
    : std::is_convertible<Consumer, std::function<void(const Type&)>>
{};

// contramap_input
//   trait: given a transformer Function : A -> B and a downstream
// input type Input (an A), names the value type B that the mapped
// consumer forwards to its inner consumer.  Alias of
// consumer_result.  Present so callers can compute the inner
// consumer's required element type from the transform.
template<typename Function,
         typename Input>
struct contramap_input
{
    using type = consumer_result_t<Function, Input>;
};

// contramap_input_t
//   type: convenience alias for contramap_input<...>::type.
template<typename Function,
         typename Input>
using contramap_input_t = typename contramap_input<Function, Input>::type;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_consumer_v
    //   value: convenience alias for is_consumer<...>::value.
    template<typename Consumer,
             typename Type>
    constexpr bool is_consumer_v = is_consumer<Consumer, Type>::value;

    // is_transformer_v
    //   value: convenience alias for is_transformer<...>::value.
    template<typename Function,
             typename Type>
    constexpr bool is_transformer_v = is_transformer<Function, Type>::value;

    // is_boxable_v
    //   value: convenience alias for is_boxable<...>::value.
    template<typename Consumer,
             typename Type>
    constexpr bool is_boxable_v = is_boxable<Consumer, Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // consumes
    //   concept: satisfied when Consumer is a consumer of Type, i.e.
    // callable as `void(const Type&)`.
    template<typename Consumer,
             typename Type>
    concept consumes = is_consumer<Consumer, Type>::value;

    // predicate_for
    //   concept: satisfied when Predicate is a bool-returning callable
    // over `const Type&` -- Predicate<Predicate, const Type&>, in this
    // section's consumer-side spelling.
    template<typename Predicate,
             typename Type>
    concept predicate_for = is_predicate<Predicate, const Type&>::value;

    // transformer_for
    //   concept: satisfied when Function maps a `const Type&` to a
    // non-void result.
    template<typename Function,
             typename Type>
    concept transformer_for = is_transformer<Function, Type>::value;

    // boxable_as
    //   concept: satisfied when Consumer can be erased into a
    // boxed_consumer<Type>.
    template<typename Consumer,
             typename Type>
    concept boxable_as = is_boxable<Consumer, Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


///////////////////////////////////////////////////////////////////////////////
///             I.    INTERNAL CONSUMER HELPER CLASSES                      ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // print_to_helper
    //   helper: writes each value to an output stream followed by an
    // optional separator. The stream is held by reference; users are
    // responsible for the stream outliving the consumer.
    template<typename Stream,
             typename Sep>
    class print_to_helper
    {
    public:
        D_CONSTEXPR print_to_helper(
            Stream& _stream,
            Sep      _sep
        )
            : m_stream(&_stream),
              m_sep(std::move(_sep))
        {}

        template<typename Value>
        void
        operator()(
            const Value& _value
        ) const
        {
            (*m_stream) << _value << m_sep;

            return;
        }

    private:
        Stream* m_stream;
        Sep      m_sep;
    };


    // write_to_helper
    //   helper: appends each received element to a container via
    // push_back. The container is held by pointer to allow non-const
    // mutation of an externally-owned object.
    template<typename Container>
    class write_to_helper
    {
    public:
        explicit D_CONSTEXPR write_to_helper(
            Container& _container
        )
            : m_container(&_container)
        {}

        template<typename Value>
        void
        operator()(
            const Value& _value
        ) const
        {
            m_container->push_back(_value);

            return;
        }

    private:
        Container* m_container;
    };


    // discard_helper
    //   helper: ignores every input. Useful as a default sink or as
    // an explicit "drain" terminal.
    struct discard_helper
    {
        template<typename Value>
        D_CONSTEXPR_CPP14 void
        operator()(
            const Value&
        ) const noexcept
        {
            return;
        }
    };


    // count_into_helper
    //   helper: increments an externally-held counter on every input,
    // discarding the value itself. Provides observable progress without
    // accumulating data.
    class count_into_helper
    {
    public:
        explicit D_CONSTEXPR count_into_helper(
            std::size_t& _counter
        )
            : m_counter(&_counter)
        {}

        template<typename Value>
        D_CONSTEXPR_CPP14
        void
        operator()(
            const Value&
        ) const
        {
            ++(*m_counter);

            return;
        }

    private:
        std::size_t* m_counter;
    };


    // filtered_consumer_helper
    //   helper: forwards an input to the inner consumer only when the
    // predicate returns true. Acts as a pre-call gate.
    template<typename Consumer,
             typename Predicate>
    class filtered_consumer_helper
    {
    public:
        template<typename ConsumerFwd,
                 typename PredicateFwd>
        D_CONSTEXPR filtered_consumer_helper(
            ConsumerFwd&&  _consumer,
            PredicateFwd&& _predicate
        )
            : m_consumer(std::forward<ConsumerFwd>(_consumer)),
              m_predicate(std::forward<PredicateFwd>(_predicate))
        {}

        template<typename Value>
        D_CONSTEXPR_CPP14
        void
        operator()(
            const Value& _value
        ) const
        {
            if (m_predicate(_value))
            {
                m_consumer(_value);
            }

            return;
        }

    private:
        Consumer   m_consumer;
        Predicate m_predicate;
    };


    // mapped_consumer_helper
    //   helper: applies a transformer to inputs before they reach the
    // inner consumer. This is the contramap operation: f : A -> B
    // turns a consumer<B> into a consumer<A>.
    template<typename Consumer,
             typename Function>
    class mapped_consumer_helper
    {
    public:
        template<typename ConsumerFwd,
                 typename FunctionFwd>
        D_CONSTEXPR mapped_consumer_helper(
            ConsumerFwd&& _consumer,
            FunctionFwd&& _function
        )
            : m_consumer(std::forward<ConsumerFwd>(_consumer)),
              m_function(std::forward<FunctionFwd>(_function))
        {}

        template<typename Value>
        D_CONSTEXPR_CPP14
        void
        operator()(
            const Value& _value
        ) const
        {
            m_consumer(m_function(_value));

            return;
        }

    private:
        Consumer m_consumer;
        Function m_function;
    };


    // tee_consumer_helper
    //   helper: variadic broadcast. Holds N consumers in a tuple and
    // invokes them in order for every received value. The fold is
    // implemented recursively for C++11 portability (no fold
    // expressions until C++17).
    template<typename... Consumers>
    class tee_consumer_helper
    {
    public:
        template<typename... ConsumersFwd>
        explicit D_CONSTEXPR tee_consumer_helper(
            ConsumersFwd&&... _consumers
        )
            : m_consumers(std::forward<ConsumersFwd>(_consumers)...)
        {}

        template<typename Value>
        D_CONSTEXPR_CPP14
        void
        operator()(
            const Value& _value
        ) const
        {
            invoke_all(_value,
                       std::integral_constant<std::size_t, 0>{});

            return;
        }

    private:
        // invoke_all (recursive case)
        //   invokes the I-th consumer on _value then advances to I+1.
        template<typename Value,
                 std::size_t I>
        typename std::enable_if<(I < sizeof...(Consumers))>::type
        invoke_all(
            const Value& _value,
            std::integral_constant<std::size_t, I>
        ) const
        {
            std::get<I>(m_consumers)(_value);

            invoke_all(_value,
                       std::integral_constant<std::size_t, I + 1>{});

            return;
        }

        // invoke_all (base case)
        //   terminates the recursion at the past-end index.
        template<typename Value,
                 std::size_t I>
        typename std::enable_if<(I == sizeof...(Consumers))>::type
        invoke_all(
            const Value&,
            std::integral_constant<std::size_t, I>
        ) const
        {
            return;
        }

        std::tuple<Consumers...> m_consumers;
    };

    // batched_consumer_helper
    //   helper: counts inputs and forwards only every N-th one to the
    // inner consumer.  N == 1 forwards every input;  N == 0 is treated
    // as N == 1 to avoid divide-by-zero.
    template<typename Consumer>
    class batched_consumer_helper
    {
    public:
        template<typename ConsumerFwd>
        D_CONSTEXPR batched_consumer_helper(
            ConsumerFwd&& _consumer,
            std::size_t    _stride
        )
            : m_consumer(std::forward<ConsumerFwd>(_consumer)),
              m_stride( (_stride < 1)
                  ? 1
                  : _stride),
              m_count(0)
        {}

        template<typename Value>
        D_CONSTEXPR_CPP14
        void
        operator()(
            const Value& _value
        ) const
        {
            ++m_count;

            if (m_count >= m_stride)
            {
                m_consumer(_value);
                m_count = 0;
            }

            return;
        }

    private:
        Consumer            m_consumer;
        std::size_t         m_stride;
        mutable std::size_t m_count;
    };


    // take_consumer_helper
    //   helper: forwards at most _n inputs to the inner consumer; all
    // subsequent inputs are silently dropped.
    template<typename Consumer>
    class take_consumer_helper
    {
    public:
        template<typename ConsumerFwd>
        D_CONSTEXPR take_consumer_helper(
            ConsumerFwd&& _consumer,
            std::size_t    _n
        )
            : m_consumer(std::forward<ConsumerFwd>(_consumer)),
              m_n(_n),
              m_seen(0)
        {}

        template<typename Value>
        D_CONSTEXPR_CPP14
        void
        operator()(
            const Value& _value
        ) const
        {
            if (m_seen < m_n)
            {
                m_consumer(_value);
                ++m_seen;
            }

            return;
        }

    private:
        Consumer            m_consumer;
        std::size_t         m_n;
        mutable std::size_t m_seen;
    };


    // drop_consumer_helper
    //   helper: silently drops the first _n inputs; thereafter forwards
    // every input to the inner consumer.
    template<typename Consumer>
    class drop_consumer_helper
    {
    public:
        template<typename ConsumerFwd>
        D_CONSTEXPR drop_consumer_helper(
            ConsumerFwd&& _consumer,
            std::size_t    _n
        )
            : m_consumer(std::forward<ConsumerFwd>(_consumer)),
              m_n(_n),
              m_seen(0)
        {}

        template<typename Value>
        D_CONSTEXPR_CPP14
        void
        operator()(
            const Value& _value
        ) const
        {
            if (m_seen < m_n)
            {
                ++m_seen;

                return;
            }

            m_consumer(_value);

            return;
        }

    private:
        Consumer            m_consumer;
        std::size_t         m_n;
        mutable std::size_t m_seen;
    };


    // conditional_consumer_helper
    //   helper: routes each input to one of two inner consumers based
    // on a predicate. The predicate is evaluated once per input.
    template<typename Predicate,
             typename IfTrue,
             typename IfFalse>
    class conditional_consumer_helper
    {
    public:
        template<typename PFwd,
                 typename TFwd,
                 typename FFwd>
        D_CONSTEXPR conditional_consumer_helper(
            PFwd&& _predicate,
            TFwd&& _if_true,
            FFwd&& _if_false
        )
            : m_predicate(std::forward<PFwd>(_predicate)),
              m_if_true(std::forward<TFwd>(_if_true)),
              m_if_false(std::forward<FFwd>(_if_false))
        {}

        template<typename Value>
        D_CONSTEXPR_CPP14
        void
        operator()(
            const Value& _value
        ) const
        {
            if (m_predicate(_value))
            {
                m_if_true(_value);
            }
            else
            {
                m_if_false(_value);
            }

            return;
        }

    private:
        Predicate m_predicate;
        IfTrue     m_if_true;
        IfFalse    m_if_false;
    };

    // fallback_consumer_helper
    //   helper: invokes a primary consumer; if it throws, falls back
    // to the secondary. Other exceptions still propagate. When
    // exceptions are disabled or undesired, prefer conditional_consumer
    // for explicit branching.
    template<typename Primary,
             typename Secondary>
    class fallback_consumer_helper
    {
    public:
        template<typename PFwd,
                 typename SFwd>
        D_CONSTEXPR fallback_consumer_helper(
            PFwd&& _primary,
            SFwd&& _secondary
        )
            : m_primary(std::forward<PFwd>(_primary)),
              m_secondary(std::forward<SFwd>(_secondary))
        {}

        template<typename Value>
        void
        operator()(
            const Value& _value
        ) const
        {
            try
            {
                m_primary(_value);
            }
            catch (...)
            {
                m_secondary(_value);
            }

            return;
        }

    private:
        Primary    m_primary;
        Secondary m_secondary;
    };

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///             II.   CONSUMER FACTORIES                                    ///
///////////////////////////////////////////////////////////////////////////////

// print_to
//   function: builds a consumer that writes each value to the
// given output stream, followed by _sep. _sep defaults to '\n'.
template<typename Stream,
            typename Sep>
D_CONSTEXPR
internal::print_to_helper<Stream,
                            typename std::decay<Sep>::type>
print_to(
    Stream& _stream,
    Sep&&   _separator
)
{
    return internal::print_to_helper<
        Stream,
        typename std::decay<Sep>::type>(
            _stream,
            std::forward<Sep>(_separator));
}


// print_to (default separator)
//   function: as print_to(stream, sep), with sep = '\n'.
template<typename Stream>
D_CONSTEXPR internal::print_to_helper<Stream,
                                        char>
print_to(
    Stream& _stream
)
{
    return internal::print_to_helper<Stream,
                                        char>(_stream, '\n');
}


// write_to
//   function: builds a consumer that appends each input to the
// given container via push_back. The container must outlive the
// consumer.
template<typename Container>
D_CONSTEXPR internal::write_to_helper<Container>
write_to(
    Container& _container
)
{
    return internal::write_to_helper<Container>(_container);
}


// discard
//   function: builds a consumer that drops every input. Useful as
// a default sink in branching constructs (conditional, fallback).
D_CONSTEXPR inline internal::discard_helper
discard()
{
    return internal::discard_helper{};
}


// count_into
//   function: builds a consumer that increments _counter for each
// input. The counter must outlive the consumer.
inline internal::count_into_helper
count_into(
    std::size_t& _counter
)
{
    return internal::count_into_helper(_counter);
}


// filtered
//   function: wraps a consumer with a predicate gate. Only inputs
// for which _predicate returns true are passed through.
template<typename Consumer,
            typename Predicate>
D_CONSTEXPR internal::filtered_consumer_helper<
                typename std::decay<Consumer>::type,
                typename std::decay<Predicate>::type
>
filtered(
    Consumer&&  _consumer,
    Predicate&& _predicate
)
{
    return internal::filtered_consumer_helper<
        typename std::decay<Consumer>::type,
        typename std::decay<Predicate>::type>(
            std::forward<Consumer>(_consumer),
            std::forward<Predicate>(_predicate));
}


// mapped
//   function: wraps a consumer<B> with a transform f : A -> B,
// producing a consumer<A>. This is the contramap operation.
template<typename Consumer,
            typename Function>
D_CONSTEXPR internal::mapped_consumer_helper<
                typename std::decay<Consumer>::type,
                typename std::decay<Function>::type
>
mapped(
    Consumer&& _consumer,
    Function&& _function
)
{
    return internal::mapped_consumer_helper<
        typename std::decay<Consumer>::type,
        typename std::decay<Function>::type>(
            std::forward<Consumer>(_consumer),
            std::forward<Function>(_function));
}


// tee
//   function: variadic broadcast. Returns a consumer that invokes
// each of its inner consumers in order for every received value.
// tee(c) degenerates to c; tee() is ill-formed (use discard()).
template<typename... Consumers>
D_CONSTEXPR internal::tee_consumer_helper<
    typename std::decay<Consumers>::type...
>
tee(
    Consumers&&... _consumers
)
{
    return internal::tee_consumer_helper<
        typename std::decay<Consumers>::type...>(
            std::forward<Consumers>(_consumers)...);
}


// batched
//   function: wraps a consumer so that it fires only every _stride
// inputs. The wrapped consumer sees only those values that fall on
// a stride boundary; intermediate values are dropped.
template<typename Consumer>
D_CONSTEXPR internal::batched_consumer_helper<
    typename std::decay<Consumer>::type
>
batched(
    Consumer&& _consumer,
    std::size_t _stride
)
{
    return internal::batched_consumer_helper<
        typename std::decay<Consumer>::type>(
            std::forward<Consumer>(_consumer), _stride);
}


// take
//   function: wraps a consumer to fire on at most the first _n
// inputs; all subsequent inputs are silently dropped.
template<typename Consumer>
D_CONSTEXPR
internal::take_consumer_helper<
    typename std::decay<Consumer>::type
>
take(
    Consumer&& _consumer,
    std::size_t _n
)
{
    return internal::take_consumer_helper<
        typename std::decay<Consumer>::type>(
            std::forward<Consumer>(_consumer), _n);
}


// drop
//   function: wraps a consumer to silently drop the first _n
// inputs and fire on all subsequent inputs.
template<typename Consumer>
D_CONSTEXPR internal::drop_consumer_helper<
    typename std::decay<Consumer>::type
>
drop(
    Consumer&& _consumer,
    std::size_t _n
)
{
    return internal::drop_consumer_helper<
        typename std::decay<Consumer>::type>(
            std::forward<Consumer>(_consumer), _n);
}


// conditional
//   function: returns a consumer that routes each input to
// _if_true (when _predicate(input) is true) or _if_false
// (otherwise). The predicate is evaluated exactly once per input.
template<typename Predicate,
         typename IfTrue,
         typename IfFalse>
D_CONSTEXPR internal::conditional_consumer_helper<
                typename std::decay<Predicate>::type,
                typename std::decay<IfTrue>::type,
                typename std::decay<IfFalse>::type>
conditional(
    Predicate&& _predicate,
    IfTrue&&    _if_true,
    IfFalse&&   _if_false
)
{
    return internal::conditional_consumer_helper<
        typename std::decay<Predicate>::type,
        typename std::decay<IfTrue>::type,
        typename std::decay<IfFalse>::type>(
            std::forward<Predicate>(_predicate),
            std::forward<IfTrue>(_if_true),
            std::forward<IfFalse>(_if_false));
}


// fallback
//   function: returns a consumer that invokes _primary; if it
// throws, the exception is caught and _secondary is invoked
// instead. Exceptions from _secondary still propagate.
template<typename Primary,
         typename Secondary>
D_CONSTEXPR internal::fallback_consumer_helper<
                typename std::decay<Primary>::type,
                typename std::decay<Secondary>::type>
fallback(
    Primary&&   _primary,
    Secondary&& _secondary
)
{
    return internal::fallback_consumer_helper<
        typename std::decay<Primary>::type,
        typename std::decay<Secondary>::type>(
            std::forward<Primary>(_primary),
            std::forward<Secondary>(_secondary));
}

///////////////////////////////////////////////////////////////////////////////
///             III.  TYPE ERASURE                                          ///
///////////////////////////////////////////////////////////////////////////////

// boxed_consumer
//   type: type-erased consumer of T. Useful for storing heterogeneous
// consumer chains in containers, returning consumers from functions
// without exposing their concrete type, or assigning consumers from
// a runtime selection.
//
//   Comes with the usual std::function overhead (heap allocation for
// non-small targets, indirect call). For compile-time-fixed chains,
// prefer the unboxed consumer factories above.
template<typename Type>
using boxed_consumer = std::function<void(const Type&)>;

// box
//   function: wraps any consumer in a boxed_consumer<T>. Type T must
// be supplied as an explicit template argument since most consumers
// are themselves generic and have no single fixed input type.
//
//   Example: auto erased = box<int>(consumers::print_to(std::cout));
template<typename Type,
         typename Consumer>
inline boxed_consumer<Type>
box(
    Consumer&& _consumer
)
{
    return boxed_consumer<Type>(std::forward<Consumer>(_consumer));
}


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_FUNCTIONAL_CONSUMER_HPP
