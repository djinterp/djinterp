/******************************************************************************
* djinterp [functional]                                         fn_builder.hpp
*
* Template fluent builder for constructing function chains (C++11+).
*   A type-safe builder that accumulates transformers and predicates,
* then executes the chain on input data. Fully typed via templates.
*
*   REFACTORED 2026-05-27: the chain is no longer a
* std::function<vector(vector)>. Each operation now wraps its
* predecessor in a stored-by-value typed step functor, so the builder
* carries a third template parameter, _Chain, naming the concrete
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
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.02.19
******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_FN_BUILDER_
#define DJINTERP_FUNCTIONAL_FN_BUILDER_ 1

// std
#include <algorithm>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../djinterp.hpp"
#include "./functional_traits.hpp"


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
    template<typename _InputType>
    struct identity_chain
    {
        std::vector<_InputType>
        operator()(const std::vector<_InputType>& _in) const
        {
            return _in;
        }
    };

    // map_chain
    //   applies _Fn to each element produced by the predecessor.
    template<typename _Prev,
             typename _Fn,
             typename _ResultType>
    class map_chain
    {
    public:
        map_chain(const _Prev& _prev, const _Fn& _fn)
            : m_prev(_prev), m_fn(_fn) {}

        template<typename _InputType>
        std::vector<_ResultType>
        operator()(const std::vector<_InputType>& _in) const
        {
            auto intermediate = m_prev(_in);
            std::vector<_ResultType> result;

            result.reserve(intermediate.size());

            for (const auto& element : intermediate)
            {
                result.push_back(m_fn(element));
            }

            return result;
        }

    private:
        _Prev m_prev;
        _Fn   m_fn;
    };

    // filter_chain_step
    //   keeps elements satisfying _Pred.
    template<typename _Prev,
             typename _Pred,
             typename _CurrentType>
    class filter_chain_step
    {
    public:
        filter_chain_step(const _Prev& _prev, const _Pred& _pred)
            : m_prev(_prev), m_pred(_pred) {}

        template<typename _InputType>
        std::vector<_CurrentType>
        operator()(const std::vector<_InputType>& _in) const
        {
            auto intermediate = m_prev(_in);
            std::vector<_CurrentType> result;

            for (const auto& element : intermediate)
            {
                if (m_pred(element)) { result.push_back(element); }
            }

            return result;
        }

    private:
        _Prev m_prev;
        _Pred m_pred;
    };

    // take_chain
    template<typename _Prev,
             typename _CurrentType>
    class take_chain
    {
    public:
        take_chain(const _Prev& _prev, std::size_t _n)
            : m_prev(_prev), m_n(_n) {}

        template<typename _InputType>
        std::vector<_CurrentType>
        operator()(const std::vector<_InputType>& _in) const
        {
            auto intermediate = m_prev(_in);
            std::size_t count = (m_n < intermediate.size())
                              ? m_n : intermediate.size();

            return std::vector<_CurrentType>(
                intermediate.begin(),
                intermediate.begin() +
                    static_cast<typename
                        std::vector<_CurrentType>::difference_type>(count));
        }

    private:
        _Prev       m_prev;
        std::size_t m_n;
    };

    // skip_chain
    template<typename _Prev,
             typename _CurrentType>
    class skip_chain
    {
    public:
        skip_chain(const _Prev& _prev, std::size_t _n)
            : m_prev(_prev), m_n(_n) {}

        template<typename _InputType>
        std::vector<_CurrentType>
        operator()(const std::vector<_InputType>& _in) const
        {
            auto intermediate = m_prev(_in);

            if (m_n >= intermediate.size())
            {
                return std::vector<_CurrentType>();
            }

            return std::vector<_CurrentType>(
                intermediate.begin() +
                    static_cast<typename
                        std::vector<_CurrentType>::difference_type>(m_n),
                intermediate.end());
        }

    private:
        _Prev       m_prev;
        std::size_t m_n;
    };

    // distinct_chain
    template<typename _Prev,
             typename _CurrentType>
    class distinct_chain
    {
    public:
        explicit distinct_chain(const _Prev& _prev) : m_prev(_prev) {}

        template<typename _InputType>
        std::vector<_CurrentType>
        operator()(const std::vector<_InputType>& _in) const
        {
            auto intermediate = m_prev(_in);
            std::vector<_CurrentType> result;

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
        _Prev m_prev;
    };

    // reversed_chain
    template<typename _Prev,
             typename _CurrentType>
    class reversed_chain
    {
    public:
        explicit reversed_chain(const _Prev& _prev) : m_prev(_prev) {}

        template<typename _InputType>
        std::vector<_CurrentType>
        operator()(const std::vector<_InputType>& _in) const
        {
            auto intermediate = m_prev(_in);

            std::reverse(intermediate.begin(), intermediate.end());

            return intermediate;
        }

    private:
        _Prev m_prev;
    };

    // sorted_chain
    template<typename _Prev,
             typename _Compare,
             typename _CurrentType>
    class sorted_chain
    {
    public:
        sorted_chain(const _Prev& _prev, const _Compare& _cmp)
            : m_prev(_prev), m_cmp(_cmp) {}

        template<typename _InputType>
        std::vector<_CurrentType>
        operator()(const std::vector<_InputType>& _in) const
        {
            auto intermediate = m_prev(_in);

            std::sort(intermediate.begin(), intermediate.end(), m_cmp);

            return intermediate;
        }

    private:
        _Prev    m_prev;
        _Compare m_cmp;
    };

    // flat_map_chain
    template<typename _Prev,
             typename _Fn,
             typename _ResultType>
    class flat_map_chain
    {
    public:
        flat_map_chain(const _Prev& _prev, const _Fn& _fn)
            : m_prev(_prev), m_fn(_fn) {}

        template<typename _InputType>
        std::vector<_ResultType>
        operator()(const std::vector<_InputType>& _in) const
        {
            auto intermediate = m_prev(_in);
            std::vector<_ResultType> result;

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
        _Prev m_prev;
        _Fn   m_fn;
    };

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///             II.   FN_BUILDER CLASS                                      ///
///////////////////////////////////////////////////////////////////////////////

// fn_builder
//   class: fluent builder for typed function chains. _InputType is the
// original input element type; _CurrentType is the element type after
// all accumulated operations; _Chain is the concrete composed chain
// functor type (vector<_InputType> -> vector<_CurrentType>).
template<typename _InputType,
         typename _CurrentType = _InputType,
         typename _Chain = internal::identity_chain<_InputType> >
class fn_builder
{
public:
    typedef _Chain chain_type;

    explicit fn_builder(_Chain _chain)
        : m_chain(std::move(_chain))
    {}

    ///////////////////////////////////////////////////////////////////////////
    ///         i.    BUILDER CREATION                                      ///
    ///////////////////////////////////////////////////////////////////////////

    // create
    //   static: a new empty builder seeded with the identity chain.
    static fn_builder<_InputType, _InputType,
                      internal::identity_chain<_InputType> >
    create()
    {
        return fn_builder<_InputType, _InputType,
                          internal::identity_chain<_InputType> >(
            internal::identity_chain<_InputType>());
    }

    ///////////////////////////////////////////////////////////////////////////
    ///         ii.   FLUENT OPERATIONS                                     ///
    ///////////////////////////////////////////////////////////////////////////

    // map
    template<typename _Fn,
             typename _ResultType = callable_result_t<_Fn, const _CurrentType&>,
             typename = typename std::enable_if<
                 is_callable<_Fn, const _CurrentType&>::value>::type>
    D_NODISCARD
    fn_builder<_InputType, _ResultType,
               internal::map_chain<_Chain, _Fn, _ResultType> >
    map(_Fn _fn) const
    {
        typedef internal::map_chain<_Chain, _Fn, _ResultType> new_chain;

        return fn_builder<_InputType, _ResultType, new_chain>(
            new_chain(m_chain, _fn));
    }

    // and_then (alias for map)
    template<typename _Fn,
             typename _ResultType = callable_result_t<_Fn, const _CurrentType&>,
             typename = typename std::enable_if<
                 is_callable<_Fn, const _CurrentType&>::value>::type>
    D_NODISCARD
    fn_builder<_InputType, _ResultType,
               internal::map_chain<_Chain, _Fn, _ResultType> >
    and_then(_Fn _fn) const
    {
        return map(std::move(_fn));
    }

    // filter
    template<typename _Pred,
             typename = typename std::enable_if<
                 is_predicate<_Pred, const _CurrentType&>::value>::type>
    D_NODISCARD
    fn_builder<_InputType, _CurrentType,
               internal::filter_chain_step<_Chain, _Pred, _CurrentType> >
    filter(_Pred _pred) const
    {
        typedef internal::filter_chain_step<_Chain, _Pred, _CurrentType>
            new_chain;

        return fn_builder<_InputType, _CurrentType, new_chain>(
            new_chain(m_chain, _pred));
    }

    // where (alias for filter)
    template<typename _Pred,
             typename = typename std::enable_if<
                 is_predicate<_Pred, const _CurrentType&>::value>::type>
    D_NODISCARD
    fn_builder<_InputType, _CurrentType,
               internal::filter_chain_step<_Chain, _Pred, _CurrentType> >
    where(_Pred _pred) const
    {
        return filter(std::move(_pred));
    }

    // take
    D_NODISCARD
    fn_builder<_InputType, _CurrentType,
               internal::take_chain<_Chain, _CurrentType> >
    take(std::size_t _n) const
    {
        typedef internal::take_chain<_Chain, _CurrentType> new_chain;

        return fn_builder<_InputType, _CurrentType, new_chain>(
            new_chain(m_chain, _n));
    }

    // skip
    D_NODISCARD
    fn_builder<_InputType, _CurrentType,
               internal::skip_chain<_Chain, _CurrentType> >
    skip(std::size_t _n) const
    {
        typedef internal::skip_chain<_Chain, _CurrentType> new_chain;

        return fn_builder<_InputType, _CurrentType, new_chain>(
            new_chain(m_chain, _n));
    }

    // distinct
    D_NODISCARD
    fn_builder<_InputType, _CurrentType,
               internal::distinct_chain<_Chain, _CurrentType> >
    distinct() const
    {
        typedef internal::distinct_chain<_Chain, _CurrentType> new_chain;

        return fn_builder<_InputType, _CurrentType, new_chain>(
            new_chain(m_chain));
    }

    // reversed
    D_NODISCARD
    fn_builder<_InputType, _CurrentType,
               internal::reversed_chain<_Chain, _CurrentType> >
    reversed() const
    {
        typedef internal::reversed_chain<_Chain, _CurrentType> new_chain;

        return fn_builder<_InputType, _CurrentType, new_chain>(
            new_chain(m_chain));
    }

    // sorted
    template<typename _Compare,
             typename = typename std::enable_if<
                 is_callable<_Compare,
                     const _CurrentType&, const _CurrentType&>::value>::type>
    D_NODISCARD
    fn_builder<_InputType, _CurrentType,
               internal::sorted_chain<_Chain, _Compare, _CurrentType> >
    sorted(_Compare _cmp) const
    {
        typedef internal::sorted_chain<_Chain, _Compare, _CurrentType>
            new_chain;

        return fn_builder<_InputType, _CurrentType, new_chain>(
            new_chain(m_chain, _cmp));
    }

    // flat_map
    template<typename _Fn,
             typename _InnerContainer = callable_result_t<
                 _Fn, const _CurrentType&>,
             typename _ResultType = typename std::decay<
                 decltype(*std::begin(
                     std::declval<const _InnerContainer&>()))>::type,
             typename = typename std::enable_if<
                 is_callable<_Fn, const _CurrentType&>::value>::type>
    D_NODISCARD
    fn_builder<_InputType, _ResultType,
               internal::flat_map_chain<_Chain, _Fn, _ResultType> >
    flat_map(_Fn _fn) const
    {
        typedef internal::flat_map_chain<_Chain, _Fn, _ResultType> new_chain;

        return fn_builder<_InputType, _ResultType, new_chain>(
            new_chain(m_chain, _fn));
    }

    ///////////////////////////////////////////////////////////////////////////
    ///         iii.  EXECUTION                                             ///
    ///////////////////////////////////////////////////////////////////////////

    // execute (vector)
    D_NODISCARD
    std::vector<_CurrentType>
    execute(const std::vector<_InputType>& _input) const
    {
        return m_chain(_input);
    }

    // execute (container)
    template<typename _Container,
             typename = typename std::enable_if<
                 std::is_convertible<
                     typename std::decay<decltype(*std::begin(
                         std::declval<const _Container&>()))>::type,
                     _InputType>::value>::type>
    D_NODISCARD
    std::vector<_CurrentType>
    execute(const _Container& _input) const
    {
        std::vector<_InputType> vec(std::begin(_input), std::end(_input));

        return m_chain(vec);
    }

    // execute (raw array)
    D_NODISCARD
    std::vector<_CurrentType>
    execute(const _InputType* _data, std::size_t _count) const
    {
        std::vector<_InputType> vec(_data, _data + _count);

        return m_chain(vec);
    }

    // operator() (shorthand for execute)
    template<typename _Container>
    D_NODISCARD
    std::vector<_CurrentType>
    operator()(const _Container& _input) const
    {
        return execute(_input);
    }

    // fold (terminal)
    template<typename _Acc,
             typename _Fn,
             typename = typename std::enable_if<
                 is_callable<_Fn, const _Acc&,
                     const _CurrentType&>::value>::type>
    D_NODISCARD
    _Acc
    fold(const std::vector<_InputType>& _input,
         _Acc                           _init,
         _Fn&&                          _fn) const
    {
        auto data = m_chain(_input);

        for (const auto& element : data)
        {
            _init = std::forward<_Fn>(_fn)(
                static_cast<const _Acc&>(_init), element);
        }

        return _init;
    }

    // count (terminal)
    D_NODISCARD
    std::size_t
    count(const std::vector<_InputType>& _input) const
    {
        return m_chain(_input).size();
    }

    // any (terminal)
    D_NODISCARD
    bool
    any(const std::vector<_InputType>& _input) const
    {
        return !m_chain(_input).empty();
    }

    // chain
    //   method: const access to the composed chain functor (for
    // introspection / boxing).
    D_NODISCARD
    const _Chain& chain() const { return m_chain; }

    // Grant access to private members for type-changing operations.
    template<typename _I, typename _C, typename _Ch>
    friend class fn_builder;

private:
    _Chain m_chain;
};


///////////////////////////////////////////////////////////////////////////////
///             III.  CONVENIENCE FACTORY                                   ///
///////////////////////////////////////////////////////////////////////////////

// make_builder
//   function: creates a new function chain builder for the given type.
template<typename _Type>
D_NODISCARD
fn_builder<_Type, _Type, internal::identity_chain<_Type> >
make_builder()
{
    return fn_builder<_Type, _Type,
                      internal::identity_chain<_Type> >::create();
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
template<typename _InputType,
         typename _OutputType>
class boxed_fn_builder
{
public:
    typedef std::function<std::vector<_OutputType>(
        const std::vector<_InputType>&)> chain_fn;

    // construct from any typed fn_builder whose CurrentType is
    // _OutputType.
    template<typename _Chain>
    explicit boxed_fn_builder(
        const fn_builder<_InputType, _OutputType, _Chain>& _b
    )
        : m_chain(_b.chain())
    {}

    D_NODISCARD
    std::vector<_OutputType>
    execute(const std::vector<_InputType>& _input) const
    {
        return m_chain(_input);
    }

    template<typename _Container>
    D_NODISCARD
    std::vector<_OutputType>
    operator()(const _Container& _input) const
    {
        std::vector<_InputType> vec(std::begin(_input), std::end(_input));

        return m_chain(vec);
    }

private:
    chain_fn m_chain;
};


// box_builder
//   function: erases a typed fn_builder into a boxed_fn_builder.
// Input/Output types are taken from the builder.
template<typename _InputType,
         typename _OutputType,
         typename _Chain>
D_NODISCARD
boxed_fn_builder<_InputType, _OutputType>
box_builder(const fn_builder<_InputType, _OutputType, _Chain>& _b)
{
    return boxed_fn_builder<_InputType, _OutputType>(_b);
}


///////////////////////////////////////////////////////////////////////////////
///             V.    FN_BUILDER SFINAE STRUCTURAL TRAITS & CONCEPTS        ///
///////////////////////////////////////////////////////////////////////////////
//   Detection vocabulary for the builder: whether a type is a typed
// fn_builder or its type-erased boxed_fn_builder counterpart, what input and
// current (output) element types a builder carries, and whether a callable is
// a valid mapper / predicate for a builder over a given element type. The
// mapper / predicate traits are expressed in terms of the shared is_callable /
// is_predicate detectors (const _Type& is exactly how the builder's fluent
// operations invoke their callables). Each predicate reduces to a `static
// constexpr bool value`; the extractors yield a `::type`. The C++20 concepts
// close the section.

NS_INTERNAL

    // is_fn_builder_helper
    //   helper: primary is std::false_type; the fn_builder<...> partial
    // specialization lifts it to std::true_type.
    template<typename _Type>
    struct is_fn_builder_helper
        : std::false_type
(};

    template<typename _InputType,
             typename _CurrentType,
             typename _Chain>
    struct is_fn_builder_helper<
        fn_builder<_InputType, _CurrentType, _Chain> >
        : std::true_type
(};

    // fn_builder_decompose_helper
    //   helper: primary exposes no members (soft failure for non-builders);
    // the fn_builder<...> specialization exposes the input and current
    // element types. fn_builder publishes only chain_type, so the element
    // types are recovered here by decomposition.
    template<typename _Type>
    struct fn_builder_decompose_helper
(};

    template<typename _InputType,
             typename _CurrentType,
             typename _Chain>
    struct fn_builder_decompose_helper<
        fn_builder<_InputType, _CurrentType, _Chain> >
    {
        using input_type   = _InputType;
        using current_type = _CurrentType;
    };

    // is_boxed_fn_builder_helper
    //   helper: detects the type-erased boxed_fn_builder<...>.
    template<typename _Type>
    struct is_boxed_fn_builder_helper
        : std::false_type
(};

    template<typename _InputType,
             typename _OutputType>
    struct is_boxed_fn_builder_helper<
        boxed_fn_builder<_InputType, _OutputType> >
        : std::true_type
(};

NS_END  // internal


// is_fn_builder
//   trait: true if _Type is a fn_builder<...> specialization, after
// stripping cv-qualifiers and references. False for every other type.
template<typename _Type>
struct is_fn_builder
    : internal::is_fn_builder_helper<typename std::decay<_Type>::type>::type
{
};


// is_boxed_fn_builder
//   trait: true if _Type is a boxed_fn_builder<...> specialization (the
// type-erased escape hatch), cv/ref stripped.
template<typename _Type>
struct is_boxed_fn_builder
    : internal::is_boxed_fn_builder_helper<
          typename std::decay<_Type>::type>::type
{
};


// fn_builder_input_type
//   trait: the original input element type _InputType of a builder.
// SFINAE-friendly: has a `::type` only when _Builder is a fn_builder.
template<typename _Builder>
struct fn_builder_input_type
{
    using type = typename internal::fn_builder_decompose_helper<
        typename std::decay<_Builder>::type>::input_type;
};

// fn_builder_input_type_t
//   alias: shorthand for fn_builder_input_type<_Builder>::type.
template<typename _Builder>
using fn_builder_input_type_t =
    typename fn_builder_input_type<_Builder>::type;


// fn_builder_current_type
//   trait: the current (output) element type _CurrentType of a builder --
// the element type its execute() yields. SFINAE-friendly.
template<typename _Builder>
struct fn_builder_current_type
{
    using type = typename internal::fn_builder_decompose_helper<
        typename std::decay<_Builder>::type>::current_type;
};

// fn_builder_current_type_t
//   alias: shorthand for fn_builder_current_type<_Builder>::type.
template<typename _Builder>
using fn_builder_current_type_t =
    typename fn_builder_current_type<_Builder>::type;


// is_fn_builder_mapper
//   trait: true if _Fn is callable as _Fn(const _Type&) -- the value-side
// shape accepted by fn_builder::map, and_then, and flat_map. The return
// type is unconstrained.
template<typename _Fn,
         typename _Type>
struct is_fn_builder_mapper
    : is_callable<_Fn, const _Type&>
{
};


// is_fn_builder_predicate
//   trait: true if _Pred is callable as _Pred(const _Type&) with a
// bool-convertible result -- the shape accepted by fn_builder::filter and
// where.
template<typename _Pred,
         typename _Type>
struct is_fn_builder_predicate
    : is_predicate<_Pred, const _Type&>
{
};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// is_fn_builder_v / is_boxed_fn_builder_v
//   variables: shorthands for the structural detectors. Available only when
// variable templates are supported (C++14+).
template<typename _Type>
static constexpr bool is_fn_builder_v = is_fn_builder<_Type>::value;

template<typename _Type>
static constexpr bool is_boxed_fn_builder_v =
    is_boxed_fn_builder<_Type>::value;

// is_fn_builder_mapper_v
//   variable: shorthand for is_fn_builder_mapper<_Fn, _Type>::value.
template<typename _Fn,
         typename _Type>
static constexpr bool is_fn_builder_mapper_v =
    is_fn_builder_mapper<_Fn, _Type>::value;

// is_fn_builder_predicate_v
//   variable: shorthand for is_fn_builder_predicate<_Pred, _Type>::value.
template<typename _Pred,
         typename _Type>
static constexpr bool is_fn_builder_predicate_v =
    is_fn_builder_predicate<_Pred, _Type>::value;
#endif


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
// fn_builder_type
//   concept: satisfied by any fn_builder<...> specialization (cv-ref
// stripped). The C++20 parallel of is_fn_builder.
template<typename _Type>
concept fn_builder_type = is_fn_builder<_Type>::value;

// boxed_fn_builder_type
//   concept: satisfied by any boxed_fn_builder<...> specialization. The
// C++20 parallel of is_boxed_fn_builder.
template<typename _Type>
concept boxed_fn_builder_type = is_boxed_fn_builder<_Type>::value;

// fn_builder_mapper_for
//   concept: satisfied when _Fn is a valid mapper over _Type. The C++20
// parallel of is_fn_builder_mapper.
template<typename _Fn,
         typename _Type>
concept fn_builder_mapper_for = is_fn_builder_mapper<_Fn, _Type>::value;

// fn_builder_predicate_for
//   concept: satisfied when _Pred is a valid predicate over _Type. The
// C++20 parallel of is_fn_builder_predicate.
template<typename _Pred,
         typename _Type>
concept fn_builder_predicate_for =
    is_fn_builder_predicate<_Pred, _Type>::value;
#endif


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_FN_BUILDER_
