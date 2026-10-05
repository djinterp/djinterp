/*******************************************************************************
* djinterp [core]                                                 fn_builder.hpp
*
* Template fluent builder for constructing function chains (C++11+).
*   A type-safe builder that accumulates transformers and predicates,
* then executes the chain on input data. Fully typed via templates.
*
*   REFACTORED 2026-05-27: the chain is no longer a
* std::function<vector(vector)>. Each operation now wraps its
* predecessor in a stored-by-value typed step functor, so the builder
* carries a third template parameter, Chain, naming the concrete
* composed chain type. This removes the std::function indirection
* (heap allocation + indirect call per chain) and lets the compiler
* inline the whole pipeline.
*
*   A type-erased escape hatch, boxed_fn_builder, remains for callers
* who need a single concrete builder type (heterogeneous storage,
* returning a builder across an ABI boundary, runtime selection). It
* wraps the typed chain in a std::function.
*
*   Note: the chain produces std::vector at each stage, so execute()
* is not constexpr before C++20 (constexpr std::vector). The chain
* *composition* is compile-time in all modes; only the materialization
* is pegged to C++20.
*
* USAGE:
*   auto result = fn_builder<int>::create()
*       .map([](int x) { return x * 2; })
*       .filter([](int x) { return x > 10; })
*       .map([](int x) { return std::to_string(x); })
*       .execute(input_vector);
*
*
* path:      /inc/djinterp/core/functional/fn_builder.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_FN_BUILDER_HPP
#define DJINTERP_FUNCTIONAL_FN_BUILDER_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <algorithm>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../djinterp.hpp"
#include "./functional_common.hpp"  // callable_result_t


NS_DJINTERP


//   DUAL DOMAIN (boundary).  Since the 2026-05-27 refactor each step is a
// stored-by-value typed functor rather than a std::function, so BUILDING a chain
// is constexpr-constructible - the chain's type is fixed during translation.
// RUNNING the chain on input data is a runtime act (it consumes and produces
// collections).  fn_builder is thus the value-domain RUNTIME executor of a
// composed function chain; the COMPILE-TIME counterpart of the same map / filter
// vocabulary is the transducer chain folding reduce_ct / a value_list (see
// transducer.hpp and reduce.hpp).  Step callables are constrained through
// functional_traits (is_callable / callable_result_t / is_predicate).

///////////////////////////////////////////////////////////////////////////////
///             I.    CHAIN STEP FUNCTORS                                   ///
///////////////////////////////////////////////////////////////////////////////
// Each step functor takes a vector of the ORIGINAL input type and returns
// a vector of the current element type. Steps are composed by storing the
// predecessor chain by value and invoking it first.

NS_INTERNAL

    // identity_chain
    //   the seed of every builder: returns its input vector unchanged.
    template<typename InputType>
    struct identity_chain
    {
        std::vector<InputType>
        operator()(const std::vector<InputType>& _in) const
        {
            return _in;
        }
    };

    // map_chain
    //   applies Fn to each element produced by the predecessor.
    template<typename Prev,
             typename Fn,
             typename ResultType>
    class map_chain
    {
    public:
        map_chain(const Prev& _prev, const Fn& _fn)
            : m_prev(_prev), m_fn(_fn) {}

        template<typename InputType>
        std::vector<ResultType>
        operator()(const std::vector<InputType>& _in) const
        {
            auto intermediate = m_prev(_in);
            std::vector<ResultType> result;

            result.reserve(intermediate.size());

            for (const auto& element : intermediate)
            {
                result.push_back(m_fn(element));
            }

            return result;
        }

    private:
        Prev m_prev;
        Fn    m_fn;
    };

    // filter_chain_step
    //   keeps elements satisfying Pred.
    template<typename Prev,
             typename Pred,
             typename CurrentType>
    class filter_chain_step
    {
    public:
        filter_chain_step(const Prev& _prev, const Pred& _pred)
            : m_prev(_prev), m_pred(_pred) {}

        template<typename InputType>
        std::vector<CurrentType>
        operator()(const std::vector<InputType>& _in) const
        {
            auto intermediate = m_prev(_in);
            std::vector<CurrentType> result;

            for (const auto& element : intermediate)
            {
                if (m_pred(element)) { result.push_back(element); }
            }

            return result;
        }

    private:
        Prev m_prev;
        Pred m_pred;
    };

    // take_chain
    template<typename Prev,
             typename CurrentType>
    class take_chain
    {
    public:
        take_chain(const Prev& _prev, std::size_t _n)
            : m_prev(_prev), m_n(_n) {}

        template<typename InputType>
        std::vector<CurrentType>
        operator()(const std::vector<InputType>& _in) const
        {
            auto intermediate = m_prev(_in);
            std::size_t count = (m_n < intermediate.size())
                              ? m_n : intermediate.size();

            return std::vector<CurrentType>(
                intermediate.begin(),
                intermediate.begin() +
                    static_cast<typename
                        std::vector<CurrentType>::difference_type>(count));
        }

    private:
        Prev        m_prev;
        std::size_t m_n;
    };

    // skip_chain
    template<typename Prev,
             typename CurrentType>
    class skip_chain
    {
    public:
        skip_chain(const Prev& _prev, std::size_t _n)
            : m_prev(_prev), m_n(_n) {}

        template<typename InputType>
        std::vector<CurrentType>
        operator()(const std::vector<InputType>& _in) const
        {
            auto intermediate = m_prev(_in);

            if (m_n >= intermediate.size())
            {
                return std::vector<CurrentType>();
            }

            return std::vector<CurrentType>(
                intermediate.begin() +
                    static_cast<typename
                        std::vector<CurrentType>::difference_type>(m_n),
                intermediate.end());
        }

    private:
        Prev        m_prev;
        std::size_t m_n;
    };

    // distinct_chain
    template<typename Prev,
             typename CurrentType>
    class distinct_chain
    {
    public:
        explicit distinct_chain(const Prev& _prev) : m_prev(_prev) {}

        template<typename InputType>
        std::vector<CurrentType>
        operator()(const std::vector<InputType>& _in) const
        {
            auto intermediate = m_prev(_in);
            std::vector<CurrentType> result;

            for (const auto& element : intermediate)
            {
                bool found = false;

                for (const auto& existing : result)
                {
                    if (element == existing) { found = true; break; }
                }

                if (!found) { result.push_back(element); }
            }

            return result;
        }

    private:
        Prev m_prev;
    };

    // reversed_chain
    template<typename Prev,
             typename CurrentType>
    class reversed_chain
    {
    public:
        explicit reversed_chain(const Prev& _prev) : m_prev(_prev) {}

        template<typename InputType>
        std::vector<CurrentType>
        operator()(const std::vector<InputType>& _in) const
        {
            auto intermediate = m_prev(_in);

            std::reverse(intermediate.begin(), intermediate.end());

            return intermediate;
        }

    private:
        Prev m_prev;
    };

    // sorted_chain
    template<typename Prev,
             typename Compare,
             typename CurrentType>
    class sorted_chain
    {
    public:
        sorted_chain(const Prev& _prev, const Compare& _cmp)
            : m_prev(_prev), m_cmp(_cmp) {}

        template<typename InputType>
        std::vector<CurrentType>
        operator()(const std::vector<InputType>& _in) const
        {
            auto intermediate = m_prev(_in);

            std::sort(intermediate.begin(), intermediate.end(), m_cmp);

            return intermediate;
        }

    private:
        Prev     m_prev;
        Compare m_cmp;
    };

    // flat_map_chain
    template<typename Prev,
             typename Fn,
             typename ResultType>
    class flat_map_chain
    {
    public:
        flat_map_chain(const Prev& _prev, const Fn& _fn)
            : m_prev(_prev), m_fn(_fn) {}

        template<typename InputType>
        std::vector<ResultType>
        operator()(const std::vector<InputType>& _in) const
        {
            auto intermediate = m_prev(_in);
            std::vector<ResultType> result;

            for (const auto& element : intermediate)
            {
                auto inner = m_fn(element);

                for (const auto& inner_element : inner)
                {
                    result.push_back(inner_element);
                }
            }

            return result;
        }

    private:
        Prev m_prev;
        Fn    m_fn;
    };

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///             II.   FN_BUILDER CLASS                                      ///
///////////////////////////////////////////////////////////////////////////////

// fn_builder
//   class: fluent builder for typed function chains. InputType is the
// original input element type; CurrentType is the element type after
// all accumulated operations; Chain is the concrete composed chain
// functor type (vector<InputType> -> vector<CurrentType>).
template<typename InputType,
         typename CurrentType = InputType,
         typename Chain = internal::identity_chain<InputType> >
class fn_builder
{
public:
    typedef Chain chain_type;

    explicit fn_builder(Chain _chain)
        : m_chain(std::move(_chain))
    {}

    ///////////////////////////////////////////////////////////////////////////
    ///         i.    BUILDER CREATION                                      ///
    ///////////////////////////////////////////////////////////////////////////

    // create
    //   static: a new empty builder seeded with the identity chain.
    static fn_builder<InputType, InputType,
                      internal::identity_chain<InputType> >
    create()
    {
        return fn_builder<InputType, InputType,
                          internal::identity_chain<InputType> >(
            internal::identity_chain<InputType>());
    }

    ///////////////////////////////////////////////////////////////////////////
    ///         ii.   FLUENT OPERATIONS                                     ///
    ///////////////////////////////////////////////////////////////////////////

    // map
    template<typename Fn,
             typename ResultType = callable_result_t<Fn, const CurrentType&>,
             typename = typename std::enable_if<
                 is_callable<Fn, const CurrentType&>::value>::type>
    D_NODISCARD
    fn_builder<InputType, ResultType,
               internal::map_chain<Chain, Fn, ResultType> >
    map(Fn _fn) const
    {
        typedef internal::map_chain<Chain, Fn, ResultType> new_chain;

        return fn_builder<InputType, ResultType, new_chain>(
            new_chain(m_chain, _fn));
    }

    // and_then (alias for map)
    template<typename Fn,
             typename ResultType = callable_result_t<Fn, const CurrentType&>,
             typename = typename std::enable_if<
                 is_callable<Fn, const CurrentType&>::value>::type>
    D_NODISCARD
    fn_builder<InputType, ResultType,
               internal::map_chain<Chain, Fn, ResultType> >
    and_then(Fn _fn) const
    {
        return map(std::move(_fn));
    }

    // filter
    template<typename Pred,
             typename = typename std::enable_if<
                 is_predicate<Pred, const CurrentType&>::value>::type>
    D_NODISCARD
    fn_builder<InputType, CurrentType,
               internal::filter_chain_step<Chain, Pred, CurrentType> >
    filter(Pred _pred) const
    {
        typedef internal::filter_chain_step<Chain, Pred, CurrentType>
            new_chain;

        return fn_builder<InputType, CurrentType, new_chain>(
            new_chain(m_chain, _pred));
    }

    // where (alias for filter)
    template<typename Pred,
             typename = typename std::enable_if<
                 is_predicate<Pred, const CurrentType&>::value>::type>
    D_NODISCARD
    fn_builder<InputType, CurrentType,
               internal::filter_chain_step<Chain, Pred, CurrentType> >
    where(Pred _pred) const
    {
        return filter(std::move(_pred));
    }

    // take
    D_NODISCARD
    fn_builder<InputType, CurrentType,
               internal::take_chain<Chain, CurrentType> >
    take(std::size_t _n) const
    {
        typedef internal::take_chain<Chain, CurrentType> new_chain;

        return fn_builder<InputType, CurrentType, new_chain>(
            new_chain(m_chain, _n));
    }

    // skip
    D_NODISCARD
    fn_builder<InputType, CurrentType,
               internal::skip_chain<Chain, CurrentType> >
    skip(std::size_t _n) const
    {
        typedef internal::skip_chain<Chain, CurrentType> new_chain;

        return fn_builder<InputType, CurrentType, new_chain>(
            new_chain(m_chain, _n));
    }

    // distinct
    D_NODISCARD
    fn_builder<InputType, CurrentType,
               internal::distinct_chain<Chain, CurrentType> >
    distinct() const
    {
        typedef internal::distinct_chain<Chain, CurrentType> new_chain;

        return fn_builder<InputType, CurrentType, new_chain>(
            new_chain(m_chain));
    }

    // reversed
    D_NODISCARD
    fn_builder<InputType, CurrentType,
               internal::reversed_chain<Chain, CurrentType> >
    reversed() const
    {
        typedef internal::reversed_chain<Chain, CurrentType> new_chain;

        return fn_builder<InputType, CurrentType, new_chain>(
            new_chain(m_chain));
    }

    // sorted
    template<typename Compare,
             typename = typename std::enable_if<
                 is_callable<Compare,
                     const CurrentType&, const CurrentType&>::value>::type>
    D_NODISCARD
    fn_builder<InputType, CurrentType,
               internal::sorted_chain<Chain, Compare, CurrentType> >
    sorted(Compare _cmp) const
    {
        typedef internal::sorted_chain<Chain, Compare, CurrentType>
            new_chain;

        return fn_builder<InputType, CurrentType, new_chain>(
            new_chain(m_chain, _cmp));
    }

    // flat_map
    template<typename Fn,
             typename InnerContainer = callable_result_t<
                 Fn, const CurrentType&>,
             typename ResultType = typename std::decay<
                 decltype(*std::begin(
                     std::declval<const InnerContainer&>()))>::type,
             typename = typename std::enable_if<
                 is_callable<Fn, const CurrentType&>::value>::type>
    D_NODISCARD
    fn_builder<InputType, ResultType,
               internal::flat_map_chain<Chain, Fn, ResultType> >
    flat_map(Fn _fn) const
    {
        typedef internal::flat_map_chain<Chain, Fn, ResultType> new_chain;

        return fn_builder<InputType, ResultType, new_chain>(
            new_chain(m_chain, _fn));
    }

    ///////////////////////////////////////////////////////////////////////////
    ///         iii.  EXECUTION                                             ///
    ///////////////////////////////////////////////////////////////////////////

    // execute (vector)
    D_NODISCARD
    std::vector<CurrentType>
    execute(const std::vector<InputType>& _input) const
    {
        return m_chain(_input);
    }

    // execute (container)
    template<typename Container,
             typename = typename std::enable_if<
                 std::is_convertible<
                     typename std::decay<decltype(*std::begin(
                         std::declval<const Container&>()))>::type,
                     InputType>::value>::type>
    D_NODISCARD
    std::vector<CurrentType>
    execute(const Container& _input) const
    {
        std::vector<InputType> vec(std::begin(_input), std::end(_input));

        return m_chain(vec);
    }

    // execute (raw array)
    D_NODISCARD
    std::vector<CurrentType>
    execute(const InputType* _data, std::size_t _count) const
    {
        std::vector<InputType> vec(_data, _data + _count);

        return m_chain(vec);
    }

    // operator() (shorthand for execute)
    template<typename Container>
    D_NODISCARD
    std::vector<CurrentType>
    operator()(const Container& _input) const
    {
        return execute(_input);
    }

    // fold (terminal)
    template<typename Acc,
             typename Fn,
             typename = typename std::enable_if<
                 is_callable<Fn, const Acc&,
                     const CurrentType&>::value>::type>
    D_NODISCARD
    Acc
    fold(const std::vector<InputType>& _input,
         Acc                            _init,
         Fn&&                          _fn) const
    {
        auto data = m_chain(_input);

        for (const auto& element : data)
        {
            _init = std::forward<Fn>(_fn)(
                static_cast<const Acc&>(_init), element);
        }

        return _init;
    }

    // count (terminal)
    D_NODISCARD
    std::size_t
    count(const std::vector<InputType>& _input) const
    {
        return m_chain(_input).size();
    }

    // any (terminal)
    D_NODISCARD
    bool
    any(const std::vector<InputType>& _input) const
    {
        return !m_chain(_input).empty();
    }

    // chain
    //   method: const access to the composed chain functor (for
    // introspection / boxing).
    D_NODISCARD
    const Chain& chain() const { return m_chain; }

    // Grant access to private members for type-changing operations.
    template<typename I, typename C, typename Ch>
    friend class fn_builder;

private:
    Chain m_chain;
};


///////////////////////////////////////////////////////////////////////////////
///             III.  CONVENIENCE FACTORY                                   ///
///////////////////////////////////////////////////////////////////////////////

// make_builder
//   function: creates a new function chain builder for the given type.
template<typename Type>
D_NODISCARD
fn_builder<Type, Type, internal::identity_chain<Type> >
make_builder()
{
    return fn_builder<Type, Type,
                      internal::identity_chain<Type> >::create();
}


///////////////////////////////////////////////////////////////////////////////
///             IV.   TYPE ERASURE  (escape hatch)                          ///
///////////////////////////////////////////////////////////////////////////////

// boxed_fn_builder
//   class: type-erased builder of (vector<Input> -> vector<Output>).
// Wraps the typed chain in a std::function so the builder has a single
// concrete type, regardless of how it was composed. Use for
// heterogeneous storage, ABI boundaries, or runtime selection. Comes
// with the usual std::function overhead.
template<typename InputType,
         typename OutputType>
class boxed_fn_builder
{
public:
    typedef std::function<std::vector<OutputType>(
        const std::vector<InputType>&)> chain_fn;

    // construct from any typed fn_builder whose CurrentType is
    // OutputType.
    template<typename Chain>
    explicit boxed_fn_builder(
        const fn_builder<InputType, OutputType, Chain>& _b
    )
        : m_chain(_b.chain())
    {}

    D_NODISCARD
    std::vector<OutputType>
    execute(const std::vector<InputType>& _input) const
    {
        return m_chain(_input);
    }

    template<typename Container>
    D_NODISCARD
    std::vector<OutputType>
    operator()(const Container& _input) const
    {
        std::vector<InputType> vec(std::begin(_input), std::end(_input));

        return m_chain(vec);
    }

private:
    chain_fn m_chain;
};


// box_builder
//   function: erases a typed fn_builder into a boxed_fn_builder.
// Input/Output types are taken from the builder.
template<typename InputType,
         typename OutputType,
         typename Chain>
D_NODISCARD
boxed_fn_builder<InputType, OutputType>
box_builder(const fn_builder<InputType, OutputType, Chain>& _b)
{
    return boxed_fn_builder<InputType, OutputType>(_b);
}


///////////////////////////////////////////////////////////////////////////////
///             V.    FN_BUILDER SFINAE STRUCTURAL TRAITS & CONCEPTS        ///
///////////////////////////////////////////////////////////////////////////////
//   Detection vocabulary for the builder: whether a type is a typed
// fn_builder or its type-erased boxed_fn_builder counterpart, what input and
// current (output) element types a builder carries, and whether a callable is
// a valid mapper / predicate for a builder over a given element type. The
// mapper / predicate traits are expressed in terms of the shared is_callable /
// is_predicate detectors (const Type& is exactly how the builder's fluent
// operations invoke their callables). Each predicate reduces to a `static
// constexpr bool value`; the extractors yield a `::type`. The C++20 concepts
// close the section.

NS_INTERNAL

    // is_fn_builder_helper
    //   helper: primary is std::false_type; the fn_builder<...> partial
    // specialization lifts it to std::true_type.
    template<typename Type>
    struct is_fn_builder_helper
        : std::false_type
{};

    template<typename InputType,
             typename CurrentType,
             typename Chain>
    struct is_fn_builder_helper<
        fn_builder<InputType, CurrentType, Chain> >
        : std::true_type
{};

    // fn_builder_decompose_helper
    //   helper: primary exposes no members (soft failure for non-builders);
    // the fn_builder<...> specialization exposes the input and current
    // element types. fn_builder publishes only chain_type, so the element
    // types are recovered here by decomposition.
    template<typename Type>
    struct fn_builder_decompose_helper
{};

    template<typename InputType,
             typename CurrentType,
             typename Chain>
    struct fn_builder_decompose_helper<
        fn_builder<InputType, CurrentType, Chain> >
    {
        using input_type   = InputType;
        using current_type = CurrentType;
    };

    // is_boxed_fn_builder_helper
    //   helper: detects the type-erased boxed_fn_builder<...>.
    template<typename Type>
    struct is_boxed_fn_builder_helper
        : std::false_type
{};

    template<typename InputType,
             typename OutputType>
    struct is_boxed_fn_builder_helper<
        boxed_fn_builder<InputType, OutputType> >
        : std::true_type
{};

NS_END  // internal


// is_fn_builder
//   trait: true if Type is a fn_builder<...> specialization, after
// stripping cv-qualifiers and references. False for every other type.
template<typename Type>
struct is_fn_builder
    : internal::is_fn_builder_helper<typename std::decay<Type>::type>::type
{
};


// is_boxed_fn_builder
//   trait: true if Type is a boxed_fn_builder<...> specialization (the
// type-erased escape hatch), cv/ref stripped.
template<typename Type>
struct is_boxed_fn_builder
    : internal::is_boxed_fn_builder_helper<
          typename std::decay<Type>::type>::type
{
};


// fn_builder_input_type
//   trait: the original input element type InputType of a builder.
// SFINAE-friendly: has a `::type` only when Builder is a fn_builder.
template<typename Builder>
struct fn_builder_input_type
{
    using type = typename internal::fn_builder_decompose_helper<
        typename std::decay<Builder>::type>::input_type;
};

// fn_builder_input_type_t
//   alias: shorthand for fn_builder_input_type<Builder>::type.
template<typename Builder>
using fn_builder_input_type_t =
    typename fn_builder_input_type<Builder>::type;


// fn_builder_current_type
//   trait: the current (output) element type CurrentType of a builder --
// the element type its execute() yields. SFINAE-friendly.
template<typename Builder>
struct fn_builder_current_type
{
    using type = typename internal::fn_builder_decompose_helper<
        typename std::decay<Builder>::type>::current_type;
};

// fn_builder_current_type_t
//   alias: shorthand for fn_builder_current_type<Builder>::type.
template<typename Builder>
using fn_builder_current_type_t =
    typename fn_builder_current_type<Builder>::type;


// is_fn_builder_mapper
//   trait: true if Fn is callable as Fn(const Type&) -- the value-side
// shape accepted by fn_builder::map, and_then, and flat_map. The return
// type is unconstrained.
template<typename Fn,
         typename Type>
struct is_fn_builder_mapper
    : is_callable<Fn, const Type&>
{
};


// is_fn_builder_predicate
//   trait: true if Pred is callable as Pred(const Type&) with a
// bool-convertible result -- the shape accepted by fn_builder::filter and
// where.
template<typename Pred,
         typename Type>
struct is_fn_builder_predicate
    : is_predicate<Pred, const Type&>
{
};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// is_fn_builder_v / is_boxed_fn_builder_v
//   variables: shorthands for the structural detectors. Available only when
// variable templates are supported (C++14+).
template<typename Type>
static constexpr bool is_fn_builder_v = is_fn_builder<Type>::value;

template<typename Type>
static constexpr bool is_boxed_fn_builder_v =
    is_boxed_fn_builder<Type>::value;

// is_fn_builder_mapper_v
//   variable: shorthand for is_fn_builder_mapper<Fn, Type>::value.
template<typename Fn,
         typename Type>
static constexpr bool is_fn_builder_mapper_v =
    is_fn_builder_mapper<Fn, Type>::value;

// is_fn_builder_predicate_v
//   variable: shorthand for is_fn_builder_predicate<Pred, Type>::value.
template<typename Pred,
         typename Type>
static constexpr bool is_fn_builder_predicate_v =
    is_fn_builder_predicate<Pred, Type>::value;
#endif


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
// fn_builder_type
//   concept: satisfied by any fn_builder<...> specialization (cv-ref
// stripped). The C++20 parallel of is_fn_builder.
template<typename Type>
concept fn_builder_type = is_fn_builder<Type>::value;

// boxed_fn_builder_type
//   concept: satisfied by any boxed_fn_builder<...> specialization. The
// C++20 parallel of is_boxed_fn_builder.
template<typename Type>
concept boxed_fn_builder_type = is_boxed_fn_builder<Type>::value;

// fn_builder_mapper_for
//   concept: satisfied when Fn is a valid mapper over Type. The C++20
// parallel of is_fn_builder_mapper.
template<typename Fn,
         typename Type>
concept fn_builder_mapper_for = is_fn_builder_mapper<Fn, Type>::value;

// fn_builder_predicate_for
//   concept: satisfied when Pred is a valid predicate over Type. The
// C++20 parallel of is_fn_builder_predicate.
template<typename Pred,
         typename Type>
concept fn_builder_predicate_for =
    is_fn_builder_predicate<Pred, Type>::value;
#endif


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_FUNCTIONAL_FN_BUILDER_HPP
