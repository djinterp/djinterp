/*******************************************************************************
* djinterp [core]                                                   pipeline.hpp
*
* Template function pipeline for chaining operations (C++).
*   Provides a fully typed, SFINAE-constrained pipeline that holds
* intermediate results and supports chainable map, filter, fold, for_each,
* take, skip, take_while, skip_while, distinct, reverse, sort, flat_map,
* zip, partition, and group_by operations.
*
*   Unlike the C version which uses void* and element_size, this pipeline
* is parameterized on the element type and performs all operations with
* full type safety. Errors are tracked via an optional-like mechanism.
*
* USAGE:
*   auto result = function_pipeline::from(my_vector)
*       .filter([](int x) { return x > 0; })
*       .map([](int x) { return x * 2; })
*       .take(10)
*       .to_vector();
*
*   auto sum = function_pipeline::from(data)
*       .filter(is_valid)
*       .map(extract_value)
*       .fold(0, std::plus<int>{});
*
*
* path:      /inc/djinterp/core/functional/pipeline.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_PIPELINE_HPP
#define DJINTERP_FUNCTIONAL_PIPELINE_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <algorithm>
#include <cstddef>
#include <functional>
#include <map>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../djinterp.hpp"
#include "./functional_common.hpp"  // callable_result_t


NS_DJINTERP


//   DUAL DOMAIN (boundary).  pipeline holds intermediate results and runs its
// map / filter / fold / take / ... stages over them: that materialization is a
// runtime act.  COMPOSITION lifts - the pipeline object and its chained stages
// are constexpr-constructible, so the chain's type is fixed during translation
// - while the traversal that produces values runs later. pipeline is the
// value-domain RUNTIME face of a staged dataflow; its COMPILE-TIME counterpart
// is the transducer chain folding reduce_ct / a value_list (see transducer.hpp,
// reduce.hpp, producer.hpp). Stage callables are constrained through
// functional_traits (is_callable / callable_result_t / is_predicate).

///////////////////////////////////////////////////////////////////////////////
///             I.    PIPELINE CLASS                                        ///
///////////////////////////////////////////////////////////////////////////////

// function_pipeline
//   class: typed function pipeline for chaining operations.
// Holds a vector of intermediate results. Each operation produces a new
// pipeline with the transformed data. If an error occurs at any stage,
// subsequent operations are no-ops and the error is propagated.
template<typename Type>
class function_pipeline
{
private:
    std::vector<Type> m_data;
    bool               m_has_error;
    int                m_error_code;

    // private constructor for internal use
    explicit function_pipeline(
        std::vector<Type>&& _data,
        bool                 _has_error  = false,
        int                  _error_code = 0
    )
        : m_data(std::move(_data)),
          m_has_error(_has_error),
          m_error_code(_error_code)
    {}

public:

    ///////////////////////////////////////////////////////////////////////////
    ///         i.    PIPELINE CREATION                                     ///
    ///////////////////////////////////////////////////////////////////////////

    // default constructor (empty pipeline)
    function_pipeline()
        : m_data(),
          m_has_error(false),
          m_error_code(0)
    {}

    // from (container)
    //   static: creates a pipeline by copying elements from a container.
    template<typename Container,
             typename = typename std::enable_if<
                 std::is_convertible<
                     typename std::decay<decltype(*std::begin(
                         std::declval<const Container&>()))>::type,
                     Type>::value
             >::type>
    static function_pipeline from(const Container& _input)
    {
        return function_pipeline(
            std::vector<Type>(std::begin(_input), std::end(_input)));
    }

    // from (move)
    //   static: creates a pipeline by moving a vector.
    static function_pipeline from(std::vector<Type>&& _data)
    {
        return function_pipeline(std::move(_data));
    }

    // from (initializer list)
    //   static: creates a pipeline from an initializer list.
    static function_pipeline from(std::initializer_list<Type> _init)
    {
        return function_pipeline(std::vector<Type>(_init));
    }

    // from (raw array)
    //   static: creates a pipeline from a C-style array.
    static function_pipeline from(const Type* _data, std::size_t _count)
    {
        return function_pipeline(std::vector<Type>(_data, _data + _count));
    }

    // of (variadic)
    //   static: creates a pipeline from variadic arguments.
    template<typename... Args,
             typename = typename std::enable_if<
                 (sizeof...(Args) > 0)
             >::type>
    static function_pipeline of(Args&&... _args)
    {
        std::vector<Type> data;

        data.reserve(sizeof...(Args));

        // fold expression emulation for C++11
        int dummy[] = { (data.push_back(
            std::forward<Args>(_args)), 0)... };
        (void)dummy;

        return function_pipeline(std::move(data));
    }

    // error
    //   static: creates an error pipeline.
    static function_pipeline error(int _code = -1)
    {
        return function_pipeline(std::vector<Type>(), true, _code);
    }


    ///////////////////////////////////////////////////////////////////////////
    ///         ii.   CHAINABLE OPERATIONS                                  ///
    ///////////////////////////////////////////////////////////////////////////

    // map
    //   method: applies a transformer to each element, producing a pipeline
    // of the result type.
    template<typename Fn,
             typename ResultType = callable_result_t<Fn, const Type&>,
             typename = typename std::enable_if<
                 is_callable<Fn, const Type&>::value
             >::type>
    D_NODISCARD D_CONSTEXPR_CPP14 function_pipeline<ResultType>
    map(
        Fn&& _fn
    ) const
    {
        if (m_has_error)
        {
            return function_pipeline<ResultType>::error(m_error_code);
        }

        std::vector<ResultType> result;

        result.reserve(m_data.size());

        for (const auto& element : m_data)
        {
            result.push_back(std::forward<Fn>(_fn)(element));
        }

        return function_pipeline<ResultType>::from(std::move(result));
    }

    // filter
    //   method: keeps only elements satisfying the predicate.
    template<typename Pred,
             typename = typename std::enable_if<
                 is_predicate<Pred, const Type&>::value
             >::type>
    D_NODISCARD
    D_CONSTEXPR_CPP14
    function_pipeline filter(Pred&& _pred) const
    {
        if (m_has_error)
        {
            return function_pipeline::error(m_error_code);
        }

        std::vector<Type> result;

        for (const auto& element : m_data)
        {
            if (std::forward<Pred>(_pred)(element))
            {
                result.push_back(element);
            }
        }

        return function_pipeline(std::move(result));
    }

    // filter_not
    //   method: keeps elements that fail the predicate.
    template<typename Pred,
             typename = typename std::enable_if<
                 is_predicate<Pred, const Type&>::value
             >::type>
    D_NODISCARD
    D_CONSTEXPR
    function_pipeline filter_not(Pred&& _pred) const
    {
        return filter([&_pred](const Type& _e)
        {
            return !_pred(_e);
        });
    }

    // fold
    //   method: folds all elements into a single accumulated value.
    template<typename Acc,
             typename Fn,
             typename = typename std::enable_if<
                 is_callable<Fn, const Acc&, const Type&>::value
             >::type>
    D_NODISCARD
    D_CONSTEXPR_CPP14
    Acc
    fold(Acc _init, Fn&& _fn) const
    {
        if (m_has_error)
        {
            return _init;
        }

        for (const auto& element : m_data)
        {
            _init = std::forward<Fn>(_fn)(
                static_cast<const Acc&>(_init), element);
        }

        return _init;
    }

    // for_each
    //   method: applies a consumer to each element and returns the
    // same pipeline (for continued chaining).
    template<typename Fn,
             typename = typename std::enable_if<
                 is_callable<Fn, const Type&>::value
             >::type>
    const function_pipeline& for_each(Fn&& _fn) const
    {
        if (!m_has_error)
        {
            for (const auto& element : m_data)
            {
                std::forward<Fn>(_fn)(element);
            }
        }

        return *this;
    }

    // take
    //   method: keeps only the first _n elements.
    D_NODISCARD
    D_CONSTEXPR_CPP14
    function_pipeline take(std::size_t _n) const
    {
        if (m_has_error)
        {
            return function_pipeline::error(m_error_code);
        }

        std::size_t actual = (_n < m_data.size()) ? _n : m_data.size();

        return function_pipeline(std::vector<Type>(
            m_data.begin(),
            m_data.begin() + static_cast<typename
                std::vector<Type>::difference_type>(actual)));
    }

    // take_last
    //   method: keeps only the last _n elements.
    D_NODISCARD
    D_CONSTEXPR_CPP14
    function_pipeline take_last(std::size_t _n) const
    {
        if (m_has_error)
        {
            return function_pipeline::error(m_error_code);
        }

        if (_n >= m_data.size())
        {
            return function_pipeline(std::vector<Type>(m_data));
        }

        return function_pipeline(std::vector<Type>(
            m_data.begin() + static_cast<typename
                std::vector<Type>::difference_type>(
                    m_data.size() - _n),
            m_data.end()));
    }

    // take_while
    //   method: takes elements while the predicate is true.
    template<typename Pred,
             typename = typename std::enable_if<
                 is_predicate<Pred, const Type&>::value
             >::type>
    D_NODISCARD
    D_CONSTEXPR_CPP14
    function_pipeline take_while(Pred&& _pred) const
    {
        if (m_has_error)
        {
            return function_pipeline::error(m_error_code);
        }

        std::vector<Type> result;

        for (const auto& element : m_data)
        {
            if (!std::forward<Pred>(_pred)(element))
            {
                break;
            }

            result.push_back(element);
        }

        return function_pipeline(std::move(result));
    }

    // skip
    //   method: removes the first _n elements.
    D_NODISCARD
    D_CONSTEXPR_CPP14
    function_pipeline skip(std::size_t _n) const
    {
        if (m_has_error)
        {
            return function_pipeline::error(m_error_code);
        }

        if (_n >= m_data.size())
        {
            return function_pipeline(std::vector<Type>());
        }

        return function_pipeline(std::vector<Type>(
            m_data.begin() + static_cast<typename
                std::vector<Type>::difference_type>(_n),
            m_data.end()));
    }

    // skip_while
    //   method: skips elements while the predicate is true.
    template<typename Pred,
             typename = typename std::enable_if<
                 is_predicate<Pred, const Type&>::value
             >::type>
    D_NODISCARD
    D_CONSTEXPR_CPP14
    function_pipeline skip_while(Pred&& _pred) const
    {
        if (m_has_error)
        {
            return function_pipeline::error(m_error_code);
        }

        std::vector<Type> result;
        bool               skipping = true;

        for (const auto& element : m_data)
        {
            if (skipping && std::forward<Pred>(_pred)(element))
            {
                continue;
            }

            skipping = false;
            result.push_back(element);
        }

        return function_pipeline(std::move(result));
    }

    // slice
    //   method: takes elements in range [start, end) with given step.
    D_NODISCARD
    D_CONSTEXPR_CPP14
    function_pipeline slice(std::size_t _start,
                     std::size_t _end,
                     std::size_t _step = 1) const
    {
        if (m_has_error || _step == 0)
        {
            return function_pipeline::error(m_has_error ? m_error_code : -1);
        }

        std::vector<Type> result;
        std::size_t        limit = (_end < m_data.size())
                                 ? _end : m_data.size();

        for (std::size_t i = _start; i < limit; i += _step)
        {
            result.push_back(m_data[i]);
        }

        return function_pipeline(std::move(result));
    }

    // distinct
    //   method: removes duplicate elements using operator==.
    D_NODISCARD
    D_CONSTEXPR_CPP14
    function_pipeline distinct() const
    {
        if (m_has_error)
        {
            return function_pipeline::error(m_error_code);
        }

        std::vector<Type> result;

        for (const auto& element : m_data)
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

        return function_pipeline(std::move(result));
    }

    // distinct (with comparator)
    //   method: removes duplicate elements using a custom equality function.
    template<typename Eq,
             typename = typename std::enable_if<
                 is_callable<Eq, const Type&, const Type&>::value
             >::type>
    D_NODISCARD
    D_CONSTEXPR_CPP14
    function_pipeline distinct(Eq&& _eq) const
    {
        if (m_has_error)
        {
            return function_pipeline::error(m_error_code);
        }

        std::vector<Type> result;

        for (const auto& element : m_data)
        {
            bool found = false;

            for (const auto& existing : result)
            {
                if (std::forward<Eq>(_eq)(element, existing))
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

        return function_pipeline(std::move(result));
    }

    // reversed
    //   method: returns a pipeline with elements in reverse order.
    D_NODISCARD
    D_CONSTEXPR_CPP14
    function_pipeline reversed() const
    {
        if (m_has_error)
        {
            return function_pipeline::error(m_error_code);
        }

        std::vector<Type> result(m_data.rbegin(), m_data.rend());

        return function_pipeline(std::move(result));
    }

    // sorted
    //   method: returns a pipeline sorted by the given comparator.
    template<typename Compare,
             typename = typename std::enable_if<
                 is_callable<Compare, const Type&, const Type&>::value
             >::type>
    D_NODISCARD
    D_CONSTEXPR_CPP14
    function_pipeline sorted(Compare&& _cmp) const
    {
        if (m_has_error)
        {
            return function_pipeline::error(m_error_code);
        }

        std::vector<Type> result(m_data);

        std::sort(result.begin(), result.end(),
                  std::forward<Compare>(_cmp));

        return function_pipeline(std::move(result));
    }

    // sorted (default ordering)
    //   method: returns a pipeline sorted with operator<.
    D_NODISCARD
    D_CONSTEXPR
    function_pipeline sorted() const
    {
        return sorted([](const Type& _a, const Type& _b)
        {
            return _a < _b;
        });
    }

    // flat_map
    //   method: maps each element to a container, then flattens.
    template<typename Fn,
             typename InnerContainer = callable_result_t<Fn, const Type&>,
             typename ResultType = typename std::decay<
                 decltype(*std::begin(
                     std::declval<const InnerContainer&>()))>::type,
             typename = typename std::enable_if<
                 is_callable<Fn, const Type&>::value
             >::type>
    D_NODISCARD
    D_CONSTEXPR_CPP14
    function_pipeline<ResultType>
    flat_map(Fn&& _fn) const
    {
        if (m_has_error)
        {
            return function_pipeline<ResultType>::error(m_error_code);
        }

        std::vector<ResultType> result;

        for (const auto& element : m_data)
        {
            auto inner = std::forward<Fn>(_fn)(element);

            for (const auto& inner_element : inner)
            {
                result.push_back(inner_element);
            }
        }

        return function_pipeline<ResultType>::from(std::move(result));
    }

    // partition_pipe
    //   method: returns a pair of pipelines: (passing, failing).
    template<typename Pred,
             typename = typename std::enable_if<
                 is_predicate<Pred, const Type&>::value
             >::type>
    D_NODISCARD
    D_CONSTEXPR_CPP14
    std::pair<function_pipeline, function_pipeline>
    partition_pipe(Pred&& _pred) const
    {
        if (m_has_error)
        {
            return std::make_pair(
                function_pipeline::error(m_error_code),
                function_pipeline::error(m_error_code));
        }

        std::vector<Type> pass;
        std::vector<Type> fail;

        for (const auto& element : m_data)
        {
            if (std::forward<Pred>(_pred)(element))
            {
                pass.push_back(element);
            }
            else
            {
                fail.push_back(element);
            }
        }

        return std::make_pair(
            function_pipeline(std::move(pass)),
            function_pipeline(std::move(fail)));
    }

    // group_by
    //   method: groups elements by a key function.
    template<typename KeyFn,
             typename KeyType = callable_result_t<KeyFn, const Type&>,
             typename = typename std::enable_if<
                 is_callable<KeyFn, const Type&>::value
             >::type>
    D_NODISCARD
    D_CONSTEXPR_CPP14
    std::map<KeyType, std::vector<Type>>
    group_by(KeyFn&& _key_fn) const
    {
        std::map<KeyType, std::vector<Type>> result;

        if (!m_has_error)
        {
            for (const auto& element : m_data)
            {
                result[std::forward<KeyFn>(_key_fn)(element)]
                    .push_back(element);
            }
        }

        return result;
    }

    // zip_with (pipeline)
    //   method: combines with another pipeline using a binary function.
    template<typename Other,
             typename Fn,
             typename ResultType = callable_result_t<
                 Fn, const Type&, const Other&>,
             typename = typename std::enable_if<
                 is_callable<Fn, const Type&, const Other&>::value
             >::type>
    D_NODISCARD
    D_CONSTEXPR_CPP14
    function_pipeline<ResultType>
    zip_with(const function_pipeline<Other>& _other, Fn&& _fn) const
    {
        if (m_has_error || _other.has_error())
        {
            return function_pipeline<ResultType>::error(
                m_has_error ? m_error_code : _other.error_code());
        }

        std::vector<ResultType> result;
        const auto&              other_data = _other.data();
        std::size_t limit = (m_data.size() < other_data.size())
                          ? m_data.size() : other_data.size();

        result.reserve(limit);

        for (std::size_t i = 0; i < limit; ++i)
        {
            result.push_back(std::forward<Fn>(_fn)(
                m_data[i], other_data[i]));
        }

        return function_pipeline<ResultType>::from(std::move(result));
    }


    ///////////////////////////////////////////////////////////////////////////
    ///         iii.  TERMINAL OPERATIONS                                   ///
    ///////////////////////////////////////////////////////////////////////////

    // to_vector
    //   method: returns the pipeline data as a vector.
    //   NOTE: const overload is lvalue-ref-qualified (const &) so it
    // can coexist with the rvalue (&&) overload; the original left it
    // unqualified, which is ill-formed against a ref-qualified sibling.
    // (fixed 2026-05-27)
    D_NODISCARD
    D_CONSTEXPR
    std::vector<Type> to_vector() const &
    {
        return m_data;
    }

    // to_vector (move)
    //   method: moves the pipeline data out.
    D_NODISCARD
    D_CONSTEXPR_CPP14
    std::vector<Type> to_vector() &&
    {
        return std::move(m_data);
    }

    // reduce
    //   method: reduces elements using a binary operation. Requires
    // non-empty pipeline.
    template<typename Fn,
             typename = typename std::enable_if<
                 is_callable<Fn, const Type&, const Type&>::value
             >::type>
    D_NODISCARD
    D_CONSTEXPR_CPP14
    Type
    reduce(Fn&& _fn) const
    {
        Type acc = m_data[0];

        for (std::size_t i = 1; i < m_data.size(); ++i)
        {
            acc = std::forward<Fn>(_fn)(
                static_cast<const Type&>(acc), m_data[i]);
        }

        return acc;
    }

    // any
    //   method: returns true if any element satisfies the predicate.
    template<typename Pred,
             typename = typename std::enable_if<
                 is_predicate<Pred, const Type&>::value
             >::type>
    D_NODISCARD
    D_CONSTEXPR_CPP14
    bool any(Pred&& _pred) const
    {
        if (m_has_error) { return false; }

        for (const auto& element : m_data)
        {
            if (std::forward<Pred>(_pred)(element))
            {
                return true;
            }
        }

        return false;
    }

    // all
    //   method: returns true if all elements satisfy the predicate.
    template<typename Pred,
             typename = typename std::enable_if<
                 is_predicate<Pred, const Type&>::value
             >::type>
    D_NODISCARD
    D_CONSTEXPR_CPP14
    bool all(Pred&& _pred) const
    {
        if (m_has_error) { return false; }

        for (const auto& element : m_data)
        {
            if (!std::forward<Pred>(_pred)(element))
            {
                return false;
            }
        }

        return true;
    }

    // none
    //   method: returns true if no element satisfies the predicate.
    template<typename Pred,
             typename = typename std::enable_if<
                 is_predicate<Pred, const Type&>::value
             >::type>
    D_NODISCARD
    D_CONSTEXPR
    bool none(Pred&& _pred) const
    {
        return !any(std::forward<Pred>(_pred));
    }

    // count
    //   method: returns the number of elements satisfying the predicate.
    template<typename Pred,
             typename = typename std::enable_if<
                 is_predicate<Pred, const Type&>::value
             >::type>
    D_NODISCARD
    D_CONSTEXPR_CPP14
    std::size_t count(Pred&& _pred) const
    {
        if (m_has_error) { return 0; }

        std::size_t n = 0;

        for (const auto& element : m_data)
        {
            if (std::forward<Pred>(_pred)(element))
            {
                ++n;
            }
        }

        return n;
    }


    ///////////////////////////////////////////////////////////////////////////
    ///         iv.   ACCESSORS AND STATUS                                  ///
    ///////////////////////////////////////////////////////////////////////////

    D_NODISCARD
    D_CONSTEXPR
    std::size_t size() const { return m_data.size(); }

    D_NODISCARD
    D_CONSTEXPR
    bool empty() const { return m_data.empty(); }

    D_NODISCARD
    D_CONSTEXPR
    bool has_error() const { return m_has_error; }

    D_NODISCARD
    D_CONSTEXPR
    int error_code() const { return m_error_code; }

    D_NODISCARD
    D_CONSTEXPR
    const std::vector<Type>& data() const { return m_data; }

    D_NODISCARD
    D_CONSTEXPR
    const Type& operator[](std::size_t _idx) const { return m_data[_idx]; }

    // begin/end for range-for support
    typename std::vector<Type>::const_iterator begin() const
    { return m_data.begin(); }

    typename std::vector<Type>::const_iterator end() const
    { return m_data.end(); }
};


///////////////////////////////////////////////////////////////////////////////
///             II.   CONVENIENCE FACTORY                                   ///
///////////////////////////////////////////////////////////////////////////////

// pipeline_from (free function)
//   function: creates a pipeline from a container.
template<typename Container,
         typename ValueType = typename std::decay<
             decltype(*std::begin(std::declval<const Container&>()))>::type>
D_NODISCARD function_pipeline<ValueType>
pipeline_from(
    const Container& _input
)
{
    return function_pipeline<ValueType>::from(_input);
}

// pipeline_from (raw array)
//   function: creates a pipeline from a C-style array.
template<typename Type>
D_NODISCARD function_pipeline<Type>
pipeline_from(
    const Type* _data,
    std::size_t  _count
)
{
    return function_pipeline<Type>::from(_data, _count);
}


///////////////////////////////////////////////////////////////////////////////
///             III.  PIPELINE SFINAE STRUCTURAL TRAITS & CONCEPTS          ///
///////////////////////////////////////////////////////////////////////////////
//   Detection vocabulary for function_pipeline: whether a type is a pipeline,
// what element type it carries, and whether a callable is a valid mapper /
// predicate for a pipeline over a given element type. The mapper / predicate
// traits are expressed in terms of the shared is_callable / is_predicate
// detectors (const Type& is exactly how the pipeline's own methods invoke
// their callables). Each predicate reduces to a `static constexpr bool
// value`; pipeline_value_type yields a `::type`. The C++20 concepts close the
// section.

NS_INTERNAL

    // is_pipeline_helper
    //   helper: primary is std::false_type; the function_pipeline<T>
    // partial specialization lifts it to std::true_type. Kept internal so
    // the public is_pipeline can decay its argument before matching.
    template<typename Type>
    struct is_pipeline_helper
        : std::false_type
{};

    template<typename T>
    struct is_pipeline_helper<function_pipeline<T>>
        : std::true_type
{};

    // pipeline_decompose_helper
    //   helper: primary exposes no members (soft failure for non-pipeline
    // types); the function_pipeline<T> specialization exposes the element
    // type. function_pipeline does not publish a value_type alias, so the
    // type is recovered here by decomposition.
    template<typename Type>
    struct pipeline_decompose_helper
{};

    template<typename T>
    struct pipeline_decompose_helper<function_pipeline<T>>
    {
        using value_type = T;
    };

NS_END  // internal


// is_pipeline
//   trait: true if Type is a function_pipeline<U> specialization, after
// stripping cv-qualifiers and references. False for every other type.
template<typename Type>
struct is_pipeline
    : internal::is_pipeline_helper<typename std::decay<Type>::type>::type
{
};


// pipeline_value_type
//   trait: the element type T of a function_pipeline<T>. SFINAE-friendly:
// has a `::type` only when Pipeline is (a cv/ref-qualified) pipeline.
template<typename Pipeline>
struct pipeline_value_type
{
    using type = typename internal::pipeline_decompose_helper<
        typename std::decay<Pipeline>::type>::value_type;
};

// pipeline_value_type_t
//   alias: shorthand for pipeline_value_type<Pipeline>::type.
template<typename Pipeline>
using pipeline_value_type_t = typename pipeline_value_type<Pipeline>::type;


// is_pipeline_mapper
//   trait: true if Fn is callable as Fn(const Type&) -- the value-side
// shape accepted by pipeline::map, flat_map, group_by, and for_each. The
// return type is unconstrained.
template<typename Fn,
         typename Type>
struct is_pipeline_mapper
    : is_callable<Fn, const Type&>
{
};


// is_pipeline_predicate
//   trait: true if Pred is callable as Pred(const Type&) with a
// bool-convertible result -- the shape accepted by pipeline::filter,
// take_while, skip_while, any, all, none, count, and partition_pipe.
template<typename Pred,
         typename Type>
struct is_pipeline_predicate
    : is_predicate<Pred, const Type&>
{
};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// is_pipeline_v
//   variable: shorthand for is_pipeline<Type>::value. Available only when
// variable templates are supported (C++14+).
template<typename Type>
static constexpr bool is_pipeline_v = is_pipeline<Type>::value;

// is_pipeline_mapper_v
//   variable: shorthand for is_pipeline_mapper<Fn, Type>::value.
template<typename Fn,
         typename Type>
static constexpr bool is_pipeline_mapper_v =
    is_pipeline_mapper<Fn, Type>::value;

// is_pipeline_predicate_v
//   variable: shorthand for is_pipeline_predicate<Pred, Type>::value.
template<typename Pred,
         typename Type>
static constexpr bool is_pipeline_predicate_v =
    is_pipeline_predicate<Pred, Type>::value;
#endif


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
// pipeline_type
//   concept: satisfied by any function_pipeline<U> specialization (cv-ref
// stripped). The C++20 parallel of is_pipeline.
template<typename Type>
concept pipeline_type = is_pipeline<Type>::value;

// pipeline_mapper_for
//   concept: satisfied when Fn is a valid mapper over Type. The C++20
// parallel of is_pipeline_mapper.
template<typename Fn,
         typename Type>
concept pipeline_mapper_for = is_pipeline_mapper<Fn, Type>::value;

// pipeline_predicate_for
//   concept: satisfied when Pred is a valid predicate over Type. The
// C++20 parallel of is_pipeline_predicate.
template<typename Pred,
         typename Type>
concept pipeline_predicate_for = is_pipeline_predicate<Pred, Type>::value;
#endif


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_FUNCTIONAL_PIPELINE_HPP
