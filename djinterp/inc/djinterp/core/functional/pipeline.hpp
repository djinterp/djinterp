/******************************************************************************
* djinterp [functional]                                          pipeline.hpp
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
* path:      /inc/djinterp/core/functional/pipeline.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.02.19
******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_PIPELINE_
#define DJINTERP_FUNCTIONAL_PIPELINE_ 1

// std
#include <algorithm>
#include <cstddef>
#include <functional>
#include <map>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../djinterp.hpp"
#include "./functional_traits.hpp"


NS_DJINTERP


//   DUAL DOMAIN (boundary).  pipeline holds intermediate results and runs its
// map / filter / fold / take / ... stages over them: that materialization is a
// runtime act.  COMPOSITION lifts - the pipeline object and its chained stages
// are constexpr-constructible, so the chain's type is fixed during translation -
// while the traversal that produces values runs later.  pipeline is the
// value-domain RUNTIME face of a staged dataflow; its COMPILE-TIME counterpart
// is the transducer chain folding reduce_ct / a value_list (see transducer.hpp,
// reduce.hpp, producer.hpp).  Stage callables are constrained through
// functional_traits (is_callable / callable_result_t / is_predicate).

///////////////////////////////////////////////////////////////////////////////
///             I.    PIPELINE CLASS                                        ///
///////////////////////////////////////////////////////////////////////////////

// function_pipeline
//   class: typed function pipeline for chaining operations.
// Holds a vector of intermediate results. Each operation produces a new
// pipeline with the transformed data. If an error occurs at any stage,
// subsequent operations are no-ops and the error is propagated.
template<typename _Type>
class function_pipeline
{
private:
    std::vector<_Type> m_data;
    bool               m_has_error;
    int                m_error_code;

    // private constructor for internal use
    explicit function_pipeline(
        std::vector<_Type>&& _data,
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
    template<typename _Container,
             typename = typename std::enable_if<
                 std::is_convertible<
                     typename std::decay<decltype(*std::begin(
                         std::declval<const _Container&>()))>::type,
                     _Type>::value
             >::type>
    static function_pipeline from(const _Container& _input)
    {
        return function_pipeline(
            std::vector<_Type>(std::begin(_input), std::end(_input)));
    }

    // from (move)
    //   static: creates a pipeline by moving a vector.
    static function_pipeline from(std::vector<_Type>&& _data)
    {
        return function_pipeline(std::move(_data));
    }

    // from (initializer list)
    //   static: creates a pipeline from an initializer list.
    static function_pipeline from(std::initializer_list<_Type> _init)
    {
        return function_pipeline(std::vector<_Type>(_init));
    }

    // from (raw array)
    //   static: creates a pipeline from a C-style array.
    static function_pipeline from(const _Type* _data, std::size_t _count)
    {
        return function_pipeline(std::vector<_Type>(_data, _data + _count));
    }

    // of (variadic)
    //   static: creates a pipeline from variadic arguments.
    template<typename... _Args,
             typename = typename std::enable_if<
                 (sizeof...(_Args) > 0)
             >::type>
    static function_pipeline of(_Args&&... _args)
    {
        std::vector<_Type> data;

        data.reserve(sizeof...(_Args));

        // fold expression emulation for C++11
        int dummy[] = { (data.push_back(
            std::forward<_Args>(_args)), 0)... };
        (void)dummy;

        return function_pipeline(std::move(data));
    }

    // error
    //   static: creates an error pipeline.
    static function_pipeline error(int _code = -1)
    {
        return function_pipeline(std::vector<_Type>(), true, _code);
    }


    ///////////////////////////////////////////////////////////////////////////
    ///         ii.   CHAINABLE OPERATIONS                                  ///
    ///////////////////////////////////////////////////////////////////////////

    // map
    //   method: applies a transformer to each element, producing a pipeline
    // of the result type.
    template<typename _Fn,
             typename _ResultType = callable_result_t<_Fn, const _Type&>,
             typename = typename std::enable_if<
                 is_callable<_Fn, const _Type&>::value
             >::type>
    D_NODISCARD D_CONSTEXPR function_pipeline<_ResultType>
    map(
        _Fn&& _fn
    ) const
    {
        if (m_has_error)
        {
            return function_pipeline<_ResultType>::error(m_error_code);
        }

        std::vector<_ResultType> result;

        result.reserve(m_data.size());

        for (const auto& element : m_data)
        {
            result.push_back(std::forward<_Fn>(_fn)(element));
        }

        return function_pipeline<_ResultType>::from(std::move(result));
    }

    // filter
    //   method: keeps only elements satisfying the predicate.
    template<typename _Pred,
             typename = typename std::enable_if<
                 is_predicate<_Pred, const _Type&>::value
             >::type>
    D_NODISCARD
    D_CONSTEXPR
    function_pipeline filter(_Pred&& _pred) const
    {
        if (m_has_error)
        {
            return function_pipeline::error(m_error_code);
        }

        std::vector<_Type> result;

        for (const auto& element : m_data)
        {
            if (std::forward<_Pred>(_pred)(element))
            {
                result.push_back(element);
            }
        }

        return function_pipeline(std::move(result));
    }

    // filter_not
    //   method: keeps elements that fail the predicate.
    template<typename _Pred,
             typename = typename std::enable_if<
                 is_predicate<_Pred, const _Type&>::value
             >::type>
    D_NODISCARD
    D_CONSTEXPR
    function_pipeline filter_not(_Pred&& _pred) const
    {
        return filter([&_pred](const _Type& _e)
        {
            return !_pred(_e);
        });
    }

    // fold
    //   method: folds all elements into a single accumulated value.
    template<typename _Acc,
             typename _Fn,
             typename = typename std::enable_if<
                 is_callable<_Fn, const _Acc&, const _Type&>::value
             >::type>
    D_NODISCARD
    D_CONSTEXPR
    _Acc
    fold(_Acc _init, _Fn&& _fn) const
    {
        if (m_has_error)
        {
            return _init;
        }

        for (const auto& element : m_data)
        {
            _init = std::forward<_Fn>(_fn)(
                static_cast<const _Acc&>(_init), element);
        }

        return _init;
    }

    // for_each
    //   method: applies a consumer to each element and returns the
    // same pipeline (for continued chaining).
    template<typename _Fn,
             typename = typename std::enable_if<
                 is_callable<_Fn, const _Type&>::value
             >::type>
    const function_pipeline& for_each(_Fn&& _fn) const
    {
        if (!m_has_error)
        {
            for (const auto& element : m_data)
            {
                std::forward<_Fn>(_fn)(element);
            }
        }

        return *this;
    }

    // take
    //   method: keeps only the first _n elements.
    D_NODISCARD
    D_CONSTEXPR
    function_pipeline take(std::size_t _n) const
    {
        if (m_has_error)
        {
            return function_pipeline::error(m_error_code);
        }

        std::size_t actual = (_n < m_data.size()) ? _n : m_data.size();

        return function_pipeline(std::vector<_Type>(
            m_data.begin(),
            m_data.begin() + static_cast<typename
                std::vector<_Type>::difference_type>(actual)));
    }

    // take_last
    //   method: keeps only the last _n elements.
    D_NODISCARD
    D_CONSTEXPR
    function_pipeline take_last(std::size_t _n) const
    {
        if (m_has_error)
        {
            return function_pipeline::error(m_error_code);
        }

        if (_n >= m_data.size())
        {
            return function_pipeline(std::vector<_Type>(m_data));
        }

        return function_pipeline(std::vector<_Type>(
            m_data.begin() + static_cast<typename
                std::vector<_Type>::difference_type>(
                    m_data.size() - _n),
            m_data.end()));
    }

    // take_while
    //   method: takes elements while the predicate is true.
    template<typename _Pred,
             typename = typename std::enable_if<
                 is_predicate<_Pred, const _Type&>::value
             >::type>
    D_NODISCARD
    D_CONSTEXPR
    function_pipeline take_while(_Pred&& _pred) const
    {
        if (m_has_error)
        {
            return function_pipeline::error(m_error_code);
        }

        std::vector<_Type> result;

        for (const auto& element : m_data)
        {
            if (!std::forward<_Pred>(_pred)(element))
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
    D_CONSTEXPR
    function_pipeline skip(std::size_t _n) const
    {
        if (m_has_error)
        {
            return function_pipeline::error(m_error_code);
        }

        if (_n >= m_data.size())
        {
            return function_pipeline(std::vector<_Type>());
        }

        return function_pipeline(std::vector<_Type>(
            m_data.begin() + static_cast<typename
                std::vector<_Type>::difference_type>(_n),
            m_data.end()));
    }

    // skip_while
    //   method: skips elements while the predicate is true.
    template<typename _Pred,
             typename = typename std::enable_if<
                 is_predicate<_Pred, const _Type&>::value
             >::type>
    D_NODISCARD
    D_CONSTEXPR
    function_pipeline skip_while(_Pred&& _pred) const
    {
        if (m_has_error)
        {
            return function_pipeline::error(m_error_code);
        }

        std::vector<_Type> result;
        bool               skipping = true;

        for (const auto& element : m_data)
        {
            if (skipping && std::forward<_Pred>(_pred)(element))
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
    D_CONSTEXPR
    function_pipeline slice(std::size_t _start,
                     std::size_t _end,
                     std::size_t _step = 1) const
    {
        if (m_has_error || _step == 0)
        {
            return function_pipeline::error(m_has_error ? m_error_code : -1);
        }

        std::vector<_Type> result;
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
    D_CONSTEXPR
    function_pipeline distinct() const
    {
        if (m_has_error)
        {
            return function_pipeline::error(m_error_code);
        }

        std::vector<_Type> result;

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
    template<typename _Eq,
             typename = typename std::enable_if<
                 is_callable<_Eq, const _Type&, const _Type&>::value
             >::type>
    D_NODISCARD
    D_CONSTEXPR
    function_pipeline distinct(_Eq&& _eq) const
    {
        if (m_has_error)
        {
            return function_pipeline::error(m_error_code);
        }

        std::vector<_Type> result;

        for (const auto& element : m_data)
        {
            bool found = false;

            for (const auto& existing : result)
            {
                if (std::forward<_Eq>(_eq)(element, existing))
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
    D_CONSTEXPR
    function_pipeline reversed() const
    {
        if (m_has_error)
        {
            return function_pipeline::error(m_error_code);
        }

        std::vector<_Type> result(m_data.rbegin(), m_data.rend());

        return function_pipeline(std::move(result));
    }

    // sorted
    //   method: returns a pipeline sorted by the given comparator.
    template<typename _Compare,
             typename = typename std::enable_if<
                 is_callable<_Compare, const _Type&, const _Type&>::value
             >::type>
    D_NODISCARD
    D_CONSTEXPR
    function_pipeline sorted(_Compare&& _cmp) const
    {
        if (m_has_error)
        {
            return function_pipeline::error(m_error_code);
        }

        std::vector<_Type> result(m_data);

        std::sort(result.begin(), result.end(),
                  std::forward<_Compare>(_cmp));

        return function_pipeline(std::move(result));
    }

    // sorted (default ordering)
    //   method: returns a pipeline sorted with operator<.
    D_NODISCARD
    D_CONSTEXPR
    function_pipeline sorted() const
    {
        return sorted([](const _Type& _a, const _Type& _b)
        {
            return _a < _b;
        });
    }

    // flat_map
    //   method: maps each element to a container, then flattens.
    template<typename _Fn,
             typename _InnerContainer = callable_result_t<_Fn, const _Type&>,
             typename _ResultType = typename std::decay<
                 decltype(*std::begin(
                     std::declval<const _InnerContainer&>()))>::type,
             typename = typename std::enable_if<
                 is_callable<_Fn, const _Type&>::value
             >::type>
    D_NODISCARD
    D_CONSTEXPR
    function_pipeline<_ResultType>
    flat_map(_Fn&& _fn) const
    {
        if (m_has_error)
        {
            return function_pipeline<_ResultType>::error(m_error_code);
        }

        std::vector<_ResultType> result;

        for (const auto& element : m_data)
        {
            auto inner = std::forward<_Fn>(_fn)(element);

            for (const auto& inner_element : inner)
            {
                result.push_back(inner_element);
            }
        }

        return function_pipeline<_ResultType>::from(std::move(result));
    }

    // partition_pipe
    //   method: returns a pair of pipelines: (passing, failing).
    template<typename _Pred,
             typename = typename std::enable_if<
                 is_predicate<_Pred, const _Type&>::value
             >::type>
    D_NODISCARD
    D_CONSTEXPR
    std::pair<function_pipeline, function_pipeline>
    partition_pipe(_Pred&& _pred) const
    {
        if (m_has_error)
        {
            return std::make_pair(
                function_pipeline::error(m_error_code),
                function_pipeline::error(m_error_code));
        }

        std::vector<_Type> pass;
        std::vector<_Type> fail;

        for (const auto& element : m_data)
        {
            if (std::forward<_Pred>(_pred)(element))
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
    template<typename _KeyFn,
             typename _KeyType = callable_result_t<_KeyFn, const _Type&>,
             typename = typename std::enable_if<
                 is_callable<_KeyFn, const _Type&>::value
             >::type>
    D_NODISCARD
    D_CONSTEXPR
    std::map<_KeyType, std::vector<_Type>>
    group_by(_KeyFn&& _key_fn) const
    {
        std::map<_KeyType, std::vector<_Type>> result;

        if (!m_has_error)
        {
            for (const auto& element : m_data)
            {
                result[std::forward<_KeyFn>(_key_fn)(element)]
                    .push_back(element);
            }
        }

        return result;
    }

    // zip_with (pipeline)
    //   method: combines with another pipeline using a binary function.
    template<typename _Other,
             typename _Fn,
             typename _ResultType = callable_result_t<
                 _Fn, const _Type&, const _Other&>,
             typename = typename std::enable_if<
                 is_callable<_Fn, const _Type&, const _Other&>::value
             >::type>
    D_NODISCARD
    D_CONSTEXPR
    function_pipeline<_ResultType>
    zip_with(const function_pipeline<_Other>& _other, _Fn&& _fn) const
    {
        if (m_has_error || _other.has_error())
        {
            return function_pipeline<_ResultType>::error(
                m_has_error ? m_error_code : _other.error_code());
        }

        std::vector<_ResultType> result;
        const auto&              other_data = _other.data();
        std::size_t limit = (m_data.size() < other_data.size())
                          ? m_data.size() : other_data.size();

        result.reserve(limit);

        for (std::size_t i = 0; i < limit; ++i)
        {
            result.push_back(std::forward<_Fn>(_fn)(
                m_data[i], other_data[i]));
        }

        return function_pipeline<_ResultType>::from(std::move(result));
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
    std::vector<_Type> to_vector() const &
    {
        return m_data;
    }

    // to_vector (move)
    //   method: moves the pipeline data out.
    D_NODISCARD
    D_CONSTEXPR
    std::vector<_Type> to_vector() &&
    {
        return std::move(m_data);
    }

    // reduce
    //   method: reduces elements using a binary operation. Requires
    // non-empty pipeline.
    template<typename _Fn,
             typename = typename std::enable_if<
                 is_callable<_Fn, const _Type&, const _Type&>::value
             >::type>
    D_NODISCARD
    D_CONSTEXPR
    _Type
    reduce(_Fn&& _fn) const
    {
        _Type acc = m_data[0];

        for (std::size_t i = 1; i < m_data.size(); ++i)
        {
            acc = std::forward<_Fn>(_fn)(
                static_cast<const _Type&>(acc), m_data[i]);
        }

        return acc;
    }

    // any
    //   method: returns true if any element satisfies the predicate.
    template<typename _Pred,
             typename = typename std::enable_if<
                 is_predicate<_Pred, const _Type&>::value
             >::type>
    D_NODISCARD
    D_CONSTEXPR
    bool any(_Pred&& _pred) const
    {
        if (m_has_error) { return false; }

        for (const auto& element : m_data)
        {
            if (std::forward<_Pred>(_pred)(element))
            {
                return true;
            }
        }

        return false;
    }

    // all
    //   method: returns true if all elements satisfy the predicate.
    template<typename _Pred,
             typename = typename std::enable_if<
                 is_predicate<_Pred, const _Type&>::value
             >::type>
    D_NODISCARD
    D_CONSTEXPR
    bool all(_Pred&& _pred) const
    {
        if (m_has_error) { return false; }

        for (const auto& element : m_data)
        {
            if (!std::forward<_Pred>(_pred)(element))
            {
                return false;
            }
        }

        return true;
    }

    // none
    //   method: returns true if no element satisfies the predicate.
    template<typename _Pred,
             typename = typename std::enable_if<
                 is_predicate<_Pred, const _Type&>::value
             >::type>
    D_NODISCARD
    D_CONSTEXPR
    bool none(_Pred&& _pred) const
    {
        return !any(std::forward<_Pred>(_pred));
    }

    // count
    //   method: returns the number of elements satisfying the predicate.
    template<typename _Pred,
             typename = typename std::enable_if<
                 is_predicate<_Pred, const _Type&>::value
             >::type>
    D_NODISCARD
    D_CONSTEXPR
    std::size_t count(_Pred&& _pred) const
    {
        if (m_has_error) { return 0; }

        std::size_t n = 0;

        for (const auto& element : m_data)
        {
            if (std::forward<_Pred>(_pred)(element))
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
    const std::vector<_Type>& data() const { return m_data; }

    D_NODISCARD
    D_CONSTEXPR
    const _Type& operator[](std::size_t _idx) const { return m_data[_idx]; }

    // begin/end for range-for support
    typename std::vector<_Type>::const_iterator begin() const
    { return m_data.begin(); }

    typename std::vector<_Type>::const_iterator end() const
    { return m_data.end(); }
};


///////////////////////////////////////////////////////////////////////////////
///             II.   CONVENIENCE FACTORY                                   ///
///////////////////////////////////////////////////////////////////////////////

// pipeline_from (free function)
//   function: creates a pipeline from a container.
template<typename _Container,
         typename _ValueType = typename std::decay<
             decltype(*std::begin(std::declval<const _Container&>()))>::type>
D_NODISCARD function_pipeline<_ValueType>
pipeline_from(
    const _Container& _input
)
{
    return function_pipeline<_ValueType>::from(_input);
}

// pipeline_from (raw array)
//   function: creates a pipeline from a C-style array.
template<typename _Type>
D_NODISCARD function_pipeline<_Type>
pipeline_from(
    const _Type* _data,
    std::size_t  _count
)
{
    return function_pipeline<_Type>::from(_data, _count);
}


///////////////////////////////////////////////////////////////////////////////
///             III.  PIPELINE SFINAE STRUCTURAL TRAITS & CONCEPTS          ///
///////////////////////////////////////////////////////////////////////////////
//   Detection vocabulary for function_pipeline: whether a type is a pipeline,
// what element type it carries, and whether a callable is a valid mapper /
// predicate for a pipeline over a given element type. The mapper / predicate
// traits are expressed in terms of the shared is_callable / is_predicate
// detectors (const _Type& is exactly how the pipeline's own methods invoke
// their callables). Each predicate reduces to a `static constexpr bool
// value`; pipeline_value_type yields a `::type`. The C++20 concepts close the
// section.

NS_INTERNAL

    // is_pipeline_helper
    //   helper: primary is std::false_type; the function_pipeline<_T>
    // partial specialization lifts it to std::true_type. Kept internal so
    // the public is_pipeline can decay its argument before matching.
    template<typename _Type>
    struct is_pipeline_helper
        : std::false_type
(};

    template<typename _T>
    struct is_pipeline_helper<function_pipeline<_T>>
        : std::true_type
(};

    // pipeline_decompose_helper
    //   helper: primary exposes no members (soft failure for non-pipeline
    // types); the function_pipeline<_T> specialization exposes the element
    // type. function_pipeline does not publish a value_type alias, so the
    // type is recovered here by decomposition.
    template<typename _Type>
    struct pipeline_decompose_helper
(};

    template<typename _T>
    struct pipeline_decompose_helper<function_pipeline<_T>>
    {
        using value_type = _T;
    };

NS_END  // internal


// is_pipeline
//   trait: true if _Type is a function_pipeline<_U> specialization, after
// stripping cv-qualifiers and references. False for every other type.
template<typename _Type>
struct is_pipeline
    : internal::is_pipeline_helper<typename std::decay<_Type>::type>::type
{
};


// pipeline_value_type
//   trait: the element type _T of a function_pipeline<_T>. SFINAE-friendly:
// has a `::type` only when _Pipeline is (a cv/ref-qualified) pipeline.
template<typename _Pipeline>
struct pipeline_value_type
{
    using type = typename internal::pipeline_decompose_helper<
        typename std::decay<_Pipeline>::type>::value_type;
};

// pipeline_value_type_t
//   alias: shorthand for pipeline_value_type<_Pipeline>::type.
template<typename _Pipeline>
using pipeline_value_type_t = typename pipeline_value_type<_Pipeline>::type;


// is_pipeline_mapper
//   trait: true if _Fn is callable as _Fn(const _Type&) -- the value-side
// shape accepted by pipeline::map, flat_map, group_by, and for_each. The
// return type is unconstrained.
template<typename _Fn,
         typename _Type>
struct is_pipeline_mapper
    : is_callable<_Fn, const _Type&>
{
};


// is_pipeline_predicate
//   trait: true if _Pred is callable as _Pred(const _Type&) with a
// bool-convertible result -- the shape accepted by pipeline::filter,
// take_while, skip_while, any, all, none, count, and partition_pipe.
template<typename _Pred,
         typename _Type>
struct is_pipeline_predicate
    : is_predicate<_Pred, const _Type&>
{
};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// is_pipeline_v
//   variable: shorthand for is_pipeline<_Type>::value. Available only when
// variable templates are supported (C++14+).
template<typename _Type>
static constexpr bool is_pipeline_v = is_pipeline<_Type>::value;

// is_pipeline_mapper_v
//   variable: shorthand for is_pipeline_mapper<_Fn, _Type>::value.
template<typename _Fn,
         typename _Type>
static constexpr bool is_pipeline_mapper_v =
    is_pipeline_mapper<_Fn, _Type>::value;

// is_pipeline_predicate_v
//   variable: shorthand for is_pipeline_predicate<_Pred, _Type>::value.
template<typename _Pred,
         typename _Type>
static constexpr bool is_pipeline_predicate_v =
    is_pipeline_predicate<_Pred, _Type>::value;
#endif


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
// pipeline_type
//   concept: satisfied by any function_pipeline<_U> specialization (cv-ref
// stripped). The C++20 parallel of is_pipeline.
template<typename _Type>
concept pipeline_type = is_pipeline<_Type>::value;

// pipeline_mapper_for
//   concept: satisfied when _Fn is a valid mapper over _Type. The C++20
// parallel of is_pipeline_mapper.
template<typename _Fn,
         typename _Type>
concept pipeline_mapper_for = is_pipeline_mapper<_Fn, _Type>::value;

// pipeline_predicate_for
//   concept: satisfied when _Pred is a valid predicate over _Type. The
// C++20 parallel of is_pipeline_predicate.
template<typename _Pred,
         typename _Type>
concept pipeline_predicate_for = is_pipeline_predicate<_Pred, _Type>::value;
#endif


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_PIPELINE_