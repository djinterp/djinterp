/******************************************************************************
* djinterp [functional]                                         fn_builder.hpp
*
* Template fluent builder for constructing function chains (C++).
*   Provides a type-safe, SFINAE-constrained builder that accumulates
* transformers and predicates, then executes the chain on input data.
* Unlike the C version which uses void* and requires the caller to manage
* element sizes, this builder is fully typed via templates.
*
*   Supports type-changing transformations: if a transformer maps A -> B,
* subsequent predicates and transformers operate on B. The builder tracks
* the current element type at compile time via chained template types.
*
* USAGE:
*   auto result = fn_builder<int>::create()
*       .map([](int x) { return x * 2; })
*       .filter([](int x) { return x > 10; })
*       .map([](int x) { return std::to_string(x); })
*       .execute(input_vector);
*
* path:      \inc\functional\fn_builder.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.02.19
******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_FN_BUILDER_
#define DJINTERP_FUNCTIONAL_FN_BUILDER_ 1

// std
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "./djinterp.h"
#include "../env/env.h"
#include "./cpp_features.h"
#include "./functional_traits.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    FN_BUILDER CLASS                                      ///
///////////////////////////////////////////////////////////////////////////////

// fn_builder
//   class: fluent builder for constructing typed function chains.
// _InputType is the type of the original input elements. _CurrentType is
// the type of elements after all accumulated transformations. These may
// differ if map operations change the element type.
template<typename _InputType,
         typename _CurrentType = _InputType>
class fn_builder
{
public:
    // the chain function type: takes a vector of the input type and
    // returns a vector of the current type
    using chain_fn = std::function<std::vector<_CurrentType>(
        const std::vector<_InputType>&)>;

private:
    chain_fn m_chain;

    // private constructor for chaining
    explicit fn_builder(chain_fn&& _chain)
        : m_chain(std::move(_chain))
    {}

public:

    ///////////////////////////////////////////////////////////////////////////
    ///         i.    BUILDER CREATION                                      ///
    ///////////////////////////////////////////////////////////////////////////

    // create
    //   static: creates a new empty builder.
    static fn_builder<_InputType, _InputType> create()
    {
        return fn_builder<_InputType, _InputType>(
            [](const std::vector<_InputType>& _in)
                -> std::vector<_InputType>
            {
                return _in;
            });
    }

    ///////////////////////////////////////////////////////////////////////////
    ///         ii.   FLUENT OPERATIONS                                     ///
    ///////////////////////////////////////////////////////////////////////////

    // map
    //   method: appends a transformer to the chain. May change the
    // element type (A -> B).
    template<typename _Fn,
             typename _ResultType = callable_result_t<_Fn, const _CurrentType&>,
             typename = typename std::enable_if<
                 is_callable<_Fn, const _CurrentType&>::value
             >::type>
    D_NODISCARD
    fn_builder<_InputType, _ResultType>
    map(_Fn _fn) const
    {
        auto prev_chain = m_chain;

        return fn_builder<_InputType, _ResultType>(
            [prev_chain, _fn](const std::vector<_InputType>& _in)
                -> std::vector<_ResultType>
            {
                auto intermediate = prev_chain(_in);
                std::vector<_ResultType> result;

                result.reserve(intermediate.size());

                for (const auto& element : intermediate)
                {
                    result.push_back(_fn(element));
                }

                return result;
            });
    }

    // and_then
    //   method: alias for map, for readability in sequential chains.
    template<typename _Fn,
             typename _ResultType = callable_result_t<_Fn, const _CurrentType&>,
             typename = typename std::enable_if<
                 is_callable<_Fn, const _CurrentType&>::value
             >::type>
    D_NODISCARD
    fn_builder<_InputType, _ResultType>
    and_then(_Fn _fn) const
    {
        return map(std::move(_fn));
    }

    // filter
    //   method: appends a predicate to the chain. Element type is
    // preserved.
    template<typename _Pred,
             typename = typename std::enable_if<
                 is_predicate<_Pred, const _CurrentType&>::value
             >::type>
    D_NODISCARD
    fn_builder<_InputType, _CurrentType>
    filter(_Pred _pred) const
    {
        auto prev_chain = m_chain;

        return fn_builder<_InputType, _CurrentType>(
            [prev_chain, _pred](const std::vector<_InputType>& _in)
                -> std::vector<_CurrentType>
            {
                auto intermediate = prev_chain(_in);
                std::vector<_CurrentType> result;

                for (const auto& element : intermediate)
                {
                    if (_pred(element))
                    {
                        result.push_back(element);
                    }
                }

                return result;
            });
    }

    // where
    //   method: alias for filter, for readability in query-style chains.
    template<typename _Pred,
             typename = typename std::enable_if<
                 is_predicate<_Pred, const _CurrentType&>::value
             >::type>
    D_NODISCARD
    fn_builder<_InputType, _CurrentType>
    where(_Pred _pred) const
    {
        return filter(std::move(_pred));
    }

    // take
    //   method: keeps only the first _n elements.
    D_NODISCARD
    fn_builder<_InputType, _CurrentType>
    take(std::size_t _n) const
    {
        auto prev_chain = m_chain;

        return fn_builder<_InputType, _CurrentType>(
            [prev_chain, _n](const std::vector<_InputType>& _in)
                -> std::vector<_CurrentType>
            {
                auto intermediate = prev_chain(_in);
                std::size_t count = (_n < intermediate.size())
                                  ? _n : intermediate.size();

                return std::vector<_CurrentType>(
                    intermediate.begin(),
                    intermediate.begin() + static_cast<typename
                        std::vector<_CurrentType>::difference_type>(count));
            });
    }

    // skip
    //   method: removes the first _n elements.
    D_NODISCARD
    fn_builder<_InputType, _CurrentType>
    skip(std::size_t _n) const
    {
        auto prev_chain = m_chain;

        return fn_builder<_InputType, _CurrentType>(
            [prev_chain, _n](const std::vector<_InputType>& _in)
                -> std::vector<_CurrentType>
            {
                auto intermediate = prev_chain(_in);

                if (_n >= intermediate.size())
                {
                    return std::vector<_CurrentType>();
                }

                return std::vector<_CurrentType>(
                    intermediate.begin() + static_cast<typename
                        std::vector<_CurrentType>::difference_type>(_n),
                    intermediate.end());
            });
    }

    // distinct
    //   method: removes duplicate elements.
    D_NODISCARD
    fn_builder<_InputType, _CurrentType>
    distinct() const
    {
        auto prev_chain = m_chain;

        return fn_builder<_InputType, _CurrentType>(
            [prev_chain](const std::vector<_InputType>& _in)
                -> std::vector<_CurrentType>
            {
                auto intermediate = prev_chain(_in);
                std::vector<_CurrentType> result;

                for (const auto& element : intermediate)
                {
                    bool found = false;

                    for (const auto& existing : result)
                    {
                        if (element == existing)
                        {
                            found = true;
                            break;
                        }
                    }

                    if (!found)
                    {
                        result.push_back(element);
                    }
                }

                return result;
            });
    }

    // reversed
    //   method: reverses element order.
    D_NODISCARD
    fn_builder<_InputType, _CurrentType>
    reversed() const
    {
        auto prev_chain = m_chain;

        return fn_builder<_InputType, _CurrentType>(
            [prev_chain](const std::vector<_InputType>& _in)
                -> std::vector<_CurrentType>
            {
                auto intermediate = prev_chain(_in);

                std::reverse(intermediate.begin(), intermediate.end());

                return intermediate;
            });
    }

    // sorted
    //   method: sorts elements by the given comparator.
    template<typename _Compare,
             typename = typename std::enable_if<
                 is_callable<_Compare,
                     const _CurrentType&, const _CurrentType&>::value
             >::type>
    D_NODISCARD
    fn_builder<_InputType, _CurrentType>
    sorted(_Compare _cmp) const
    {
        auto prev_chain = m_chain;

        return fn_builder<_InputType, _CurrentType>(
            [prev_chain, _cmp](const std::vector<_InputType>& _in)
                -> std::vector<_CurrentType>
            {
                auto intermediate = prev_chain(_in);

                std::sort(intermediate.begin(), intermediate.end(), _cmp);

                return intermediate;
            });
    }

    // flat_map
    //   method: maps each element to a container, then flattens.
    template<typename _Fn,
             typename _InnerContainer = callable_result_t<
                 _Fn, const _CurrentType&>,
             typename _ResultType = typename std::decay<
                 decltype(*std::begin(
                     std::declval<const _InnerContainer&>()))>::type,
             typename = typename std::enable_if<
                 is_callable<_Fn, const _CurrentType&>::value
             >::type>
    D_NODISCARD
    fn_builder<_InputType, _ResultType>
    flat_map(_Fn _fn) const
    {
        auto prev_chain = m_chain;

        return fn_builder<_InputType, _ResultType>(
            [prev_chain, _fn](const std::vector<_InputType>& _in)
                -> std::vector<_ResultType>
            {
                auto intermediate = prev_chain(_in);
                std::vector<_ResultType> result;

                for (const auto& element : intermediate)
                {
                    auto inner = _fn(element);

                    for (const auto& inner_element : inner)
                    {
                        result.push_back(inner_element);
                    }
                }

                return result;
            });
    }


    ///////////////////////////////////////////////////////////////////////////
    ///         iii.  EXECUTION                                             ///
    ///////////////////////////////////////////////////////////////////////////

    // execute (vector)
    //   method: executes the accumulated chain on an input vector.
    D_NODISCARD
    std::vector<_CurrentType>
    execute(const std::vector<_InputType>& _input) const
    {
        return m_chain(_input);
    }

    // execute (container)
    //   method: executes the accumulated chain on any iterable container.
    template<typename _Container,
             typename = typename std::enable_if<
                 std::is_convertible<
                     typename std::decay<decltype(*std::begin(
                         std::declval<const _Container&>()))>::type,
                     _InputType>::value
             >::type>
    D_NODISCARD
    std::vector<_CurrentType>
    execute(const _Container& _input) const
    {
        std::vector<_InputType> vec(std::begin(_input),
                                    std::end(_input));

        return m_chain(vec);
    }

    // execute (raw array)
    //   method: executes the chain on a C-style array.
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

    // fold (terminal operation)
    //   method: executes the chain then folds the result.
    template<typename _Acc,
             typename _Fn,
             typename = typename std::enable_if<
                 is_callable<_Fn, const _Acc&,
                     const _CurrentType&>::value
             >::type>
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

    // count (terminal operation)
    //   method: executes the chain then counts matching elements.
    D_NODISCARD
    std::size_t
    count(const std::vector<_InputType>& _input) const
    {
        return m_chain(_input).size();
    }

    // any (terminal operation)
    //   method: executes the chain then checks if any element exists.
    D_NODISCARD
    bool
    any(const std::vector<_InputType>& _input) const
    {
        return !m_chain(_input).empty();
    }

    // Grant access to private constructor for type-changing operations
    template<typename _I, typename _C>
    friend class fn_builder;
};


///////////////////////////////////////////////////////////////////////////////
///             II.   CONVENIENCE FACTORY                                   ///
///////////////////////////////////////////////////////////////////////////////

// make_builder
//   function: creates a new function chain builder for the given type.
template<typename _Type>
D_NODISCARD fn_builder<_Type, _Type>
make_builder()
{
    return fn_builder<_Type, _Type>::create();
}


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_FN_BUILDER_