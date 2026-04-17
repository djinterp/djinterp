/******************************************************************************
* djinterp [functional]                                            filter.hpp
*
* Template collection filtering with expression-based selection (C++).
*   Provides a comprehensive, fully typed filtering framework that supports
* positional operations (take, skip, range, slice), predicate-based
* selection, index-based access, transformation operations (distinct,
* reverse), sequential chaining, set-theoretic combinators (union,
* intersection, difference), lazy iteration, and a fluent builder.
*
*   Unlike the C version which uses void*, element_size, and function
* pointers, all operations are parameterized on the element type and
* accept any callable (lambdas, function objects, function pointers, etc.)
* with compile-time SFINAE validation.
*
*   Filter operations are type-erased via std::function rather than
* virtual dispatch, keeping the module free of inheritance hierarchies.
*
* USAGE:
*   // fluent builder
*   auto result = filter_builder<int>::build()
*       .skip_first(5)
*       .where([](int x) { return x > 0; })
*       .take_first(10)
*       .distinct()
*       .apply(my_data);
*
*   // combinator
*   auto combined = filter_union(
*       filter_builder<int>::build().where(is_positive).build_chain(),
*       filter_builder<int>::build().where(is_even).build_chain());
*   auto result = combined.apply(my_data);
*
* TABLE OF CONTENTS
* =================
* I.    FILTER OPERATION TYPE
* II.   FILTER OPERATION FACTORIES
* III.  FILTER RESULT
* IV.   FILTER CHAIN
* V.    FILTER COMBINATORS
* VI.   FILTER ITERATOR
* VII.  FLUENT FILTER BUILDER
*
*
* path:      \inc\functional\filter.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.19
******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_FILTER_HPP_
#define DJINTERP_FUNCTIONAL_FILTER_HPP_ 1

#include <algorithm>
#include <cstddef>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include "../djinterp.hpp"
#include "../../c/env.h"
#include "./functional.hpp"
#include "./functional_traits.hpp"


NS_DJINTERP
NS_FUNCTIONAL


///////////////////////////////////////////////////////////////////////////////
///             I.    FILTER OPERATION TYPE                                 ///
///////////////////////////////////////////////////////////////////////////////

// filter_op_fn
//   type: a filter operation is a function that accepts an input
// vector and returns the indices of elements that pass.
template<typename _Type>
using filter_op_fn = std::function<
    std::vector<std::size_t>(const std::vector<_Type>&)>;


///////////////////////////////////////////////////////////////////////////////
///             II.   FILTER OPERATION FACTORIES                            ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // make_take_first_op
    //   helper: keeps the first _n elements.
    template<typename _Type>
    filter_op_fn<_Type>
    make_take_first_op(std::size_t _n)
    {
        return [_n](const std::vector<_Type>& _input)
            -> std::vector<std::size_t>
        {
            std::vector<std::size_t> result;
            std::size_t limit = (_n < _input.size())
                              ? _n : _input.size();

            for (std::size_t i = 0; i < limit; ++i)
            {
                result.push_back(i);
            }

            return result;
        };
    }

    // make_take_last_op
    //   helper: keeps the last _n elements.
    template<typename _Type>
    filter_op_fn<_Type>
    make_take_last_op(std::size_t _n)
    {
        return [_n](const std::vector<_Type>& _input)
            -> std::vector<std::size_t>
        {
            std::vector<std::size_t> result;
            std::size_t start = (_n >= _input.size())
                              ? 0 : _input.size() - _n;

            for (std::size_t i = start; i < _input.size(); ++i)
            {
                result.push_back(i);
            }

            return result;
        };
    }

    // make_skip_first_op
    //   helper: removes the first _n elements.
    template<typename _Type>
    filter_op_fn<_Type>
    make_skip_first_op(std::size_t _n)
    {
        return [_n](const std::vector<_Type>& _input)
            -> std::vector<std::size_t>
        {
            std::vector<std::size_t> result;
            std::size_t start = (_n < _input.size())
                              ? _n : _input.size();

            for (std::size_t i = start; i < _input.size(); ++i)
            {
                result.push_back(i);
            }

            return result;
        };
    }

    // make_skip_last_op
    //   helper: removes the last _n elements.
    template<typename _Type>
    filter_op_fn<_Type>
    make_skip_last_op(std::size_t _n)
    {
        return [_n](const std::vector<_Type>& _input)
            -> std::vector<std::size_t>
        {
            std::vector<std::size_t> result;
            std::size_t limit = (_n >= _input.size())
                              ? 0 : _input.size() - _n;

            for (std::size_t i = 0; i < limit; ++i)
            {
                result.push_back(i);
            }

            return result;
        };
    }

    // make_take_nth_op
    //   helper: keeps every _n-th element.
    template<typename _Type>
    filter_op_fn<_Type>
    make_take_nth_op(std::size_t _n)
    {
        return [_n](const std::vector<_Type>& _input)
            -> std::vector<std::size_t>
        {
            std::vector<std::size_t> result;

            if (_n == 0)
            {
                return result;
            }

            for (std::size_t i = 0; i < _input.size(); i += _n)
            {
                result.push_back(i);
            }

            return result;
        };
    }

    // make_range_op
    //   helper: keeps elements in [start, end).
    template<typename _Type>
    filter_op_fn<_Type>
    make_range_op(std::size_t _start, std::size_t _end)
    {
        return [_start, _end](const std::vector<_Type>& _input)
            -> std::vector<std::size_t>
        {
            std::vector<std::size_t> result;
            std::size_t limit = (_end < _input.size())
                              ? _end : _input.size();

            for (std::size_t i = _start; i < limit; ++i)
            {
                result.push_back(i);
            }

            return result;
        };
    }

    // make_slice_op
    //   helper: keeps elements in [start, end) with given step.
    template<typename _Type>
    filter_op_fn<_Type>
    make_slice_op(std::size_t _start,
                  std::size_t _end,
                  std::size_t _step)
    {
        return [_start, _end, _step](const std::vector<_Type>& _input)
            -> std::vector<std::size_t>
        {
            std::vector<std::size_t> result;

            if (_step == 0)
            {
                return result;
            }

            std::size_t limit = (_end < _input.size())
                              ? _end : _input.size();

            for (std::size_t i = _start; i < limit; i += _step)
            {
                result.push_back(i);
            }

            return result;
        };
    }

    // make_where_op
    //   helper: keeps elements satisfying a predicate.
    template<typename _Type>
    filter_op_fn<_Type>
    make_where_op(std::function<bool(const _Type&)> _pred)
    {
        return [_pred](const std::vector<_Type>& _input)
            -> std::vector<std::size_t>
        {
            std::vector<std::size_t> result;

            for (std::size_t i = 0; i < _input.size(); ++i)
            {
                if (_pred(_input[i]))
                {
                    result.push_back(i);
                }
            }

            return result;
        };
    }

    // make_where_not_op
    //   helper: keeps elements failing a predicate.
    template<typename _Type>
    filter_op_fn<_Type>
    make_where_not_op(std::function<bool(const _Type&)> _pred)
    {
        return [_pred](const std::vector<_Type>& _input)
            -> std::vector<std::size_t>
        {
            std::vector<std::size_t> result;

            for (std::size_t i = 0; i < _input.size(); ++i)
            {
                if (!_pred(_input[i]))
                {
                    result.push_back(i);
                }
            }

            return result;
        };
    }

    // make_indices_op
    //   helper: keeps elements at the given indices.
    template<typename _Type>
    filter_op_fn<_Type>
    make_indices_op(std::vector<std::size_t> _indices)
    {
        return [_indices](const std::vector<_Type>& _input)
            -> std::vector<std::size_t>
        {
            std::vector<std::size_t> result;

            for (auto idx : _indices)
            {
                if (idx < _input.size())
                {
                    result.push_back(idx);
                }
            }

            return result;
        };
    }

    // make_distinct_op
    //   helper: removes duplicates per an equality function.
    template<typename _Type>
    filter_op_fn<_Type>
    make_distinct_op(
        std::function<bool(const _Type&, const _Type&)> _eq)
    {
        return [_eq](const std::vector<_Type>& _input)
            -> std::vector<std::size_t>
        {
            std::vector<std::size_t> result;
            std::vector<std::size_t> seen;

            for (std::size_t i = 0; i < _input.size(); ++i)
            {
                bool is_dup = false;

                for (auto j : seen)
                {
                    if (_eq(_input[i], _input[j]))
                    {
                        is_dup = true;
                        break;
                    }
                }

                if (!is_dup)
                {
                    result.push_back(i);
                    seen.push_back(i);
                }
            }

            return result;
        };
    }

    // make_reverse_op
    //   helper: reverses element order.
    template<typename _Type>
    filter_op_fn<_Type>
    make_reverse_op()
    {
        return [](const std::vector<_Type>& _input)
            -> std::vector<std::size_t>
        {
            std::vector<std::size_t> result;

            for (std::size_t i = _input.size(); i > 0; --i)
            {
                result.push_back(i - 1);
            }

            return result;
        };
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///             III.  FILTER RESULT                                         ///
///////////////////////////////////////////////////////////////////////////////

// filter_result_status
//   enum: status of a filter operation.
enum class filter_result_status
{
    success    =  0,
    empty      =  1,
    error      = -1,
    invalid    = -2,
    no_memory  = -3
};

// filter_result
//   class: result of applying a filter operation.
template<typename _Type>
class filter_result
{
private:
    std::vector<_Type>       m_elements;
    std::vector<std::size_t> m_indices;
    filter_result_status     m_status;
    std::string              m_error_message;

public:
    // success constructor
    filter_result(std::vector<_Type>&&       _elements,
                  std::vector<std::size_t>&& _indices)
        : m_elements(std::move(_elements))
        , m_indices(std::move(_indices))
        , m_status(m_elements.empty()
                   ? filter_result_status::empty
                   : filter_result_status::success)
    {
    }

    // error constructor
    explicit filter_result(filter_result_status _status,
                           std::string          _msg = "")
        : m_status(_status)
        , m_error_message(std::move(_msg))
    {
    }

    D_NODISCARD
    bool ok() const
    {
        return m_status == filter_result_status::success;
    }

    D_NODISCARD
    bool empty() const { return m_elements.empty(); }

    D_NODISCARD
    filter_result_status status() const { return m_status; }

    D_NODISCARD
    const std::string& error_message() const
    {
        return m_error_message;
    }

    D_NODISCARD
    std::size_t count() const { return m_elements.size(); }

    D_NODISCARD
    const std::vector<_Type>& elements() const
    {
        return m_elements;
    }

    D_NODISCARD
    std::vector<_Type> take_elements()
    {
        return std::move(m_elements);
    }

    D_NODISCARD
    const std::vector<std::size_t>& indices() const
    {
        return m_indices;
    }

    typename std::vector<_Type>::const_iterator begin() const
    {
        return m_elements.begin();
    }

    typename std::vector<_Type>::const_iterator end() const
    {
        return m_elements.end();
    }
};


///////////////////////////////////////////////////////////////////////////////
///             IV.   FILTER CHAIN                                          ///
///////////////////////////////////////////////////////////////////////////////

// filter_chain
//   class: chain of sequential filter operations.
template<typename _Type>
class filter_chain
{
private:
    std::vector<filter_op_fn<_Type>> m_operations;

public:
    filter_chain() = default;

    filter_chain(const filter_chain&)            = default;
    filter_chain& operator=(const filter_chain&) = default;
    filter_chain(filter_chain&&)                 = default;
    filter_chain& operator=(filter_chain&&)      = default;

    // add
    //   method: adds an operation to the chain.
    void add(filter_op_fn<_Type> _op)
    {
        m_operations.push_back(std::move(_op));

        return;
    }

    // apply
    //   method: applies the chain to input data and returns a
    // filter result.
    D_NODISCARD
    filter_result<_Type>
    apply(const std::vector<_Type>& _input) const
    {
        // start with all indices
        std::vector<std::size_t> current_indices;

        current_indices.reserve(_input.size());

        for (std::size_t i = 0; i < _input.size(); ++i)
        {
            current_indices.push_back(i);
        }

        // apply each operation in sequence
        for (const auto& op : m_operations)
        {
            // build a temporary sub-vector for this operation
            std::vector<_Type> sub;

            sub.reserve(current_indices.size());

            for (auto idx : current_indices)
            {
                sub.push_back(_input[idx]);
            }

            // get indices relative to the sub-vector
            auto relative_indices = op(sub);

            // map back to original indices
            std::vector<std::size_t> new_indices;

            new_indices.reserve(relative_indices.size());

            for (auto rel_idx : relative_indices)
            {
                new_indices.push_back(current_indices[rel_idx]);
            }

            current_indices = std::move(new_indices);
        }

        // collect results
        std::vector<_Type> result_elements;

        result_elements.reserve(current_indices.size());

        for (auto idx : current_indices)
        {
            result_elements.push_back(_input[idx]);
        }

        return filter_result<_Type>(std::move(result_elements),
                                    std::move(current_indices));
    }

    // length
    D_NODISCARD
    std::size_t length() const { return m_operations.size(); }

    D_NODISCARD
    bool is_empty() const { return m_operations.empty(); }

    void clear()
    {
        m_operations.clear();

        return;
    }
};


///////////////////////////////////////////////////////////////////////////////
///             V.    FILTER COMBINATORS                                    ///
///////////////////////////////////////////////////////////////////////////////

// filter_union
//   function: applies union semantics (OR) over multiple filter
// chains.  An element is included if it passes any of the chains.
template<typename _Type>
D_NODISCARD
filter_result<_Type>
filter_union(const std::vector<filter_chain<_Type>>& _chains,
             const std::vector<_Type>&                _input)
{
    std::vector<bool> included(_input.size(), false);

    for (const auto& chain : _chains)
    {
        auto result = chain.apply(_input);

        for (auto idx : result.indices())
        {
            included[idx] = true;
        }
    }

    std::vector<_Type>       elements;
    std::vector<std::size_t> indices;

    for (std::size_t i = 0; i < _input.size(); ++i)
    {
        if (included[i])
        {
            elements.push_back(_input[i]);
            indices.push_back(i);
        }
    }

    return filter_result<_Type>(std::move(elements),
                                std::move(indices));
}

// filter_intersection
//   function: applies intersection semantics (AND) over multiple
// chains.  An element is included only if it passes all chains.
template<typename _Type>
D_NODISCARD
filter_result<_Type>
filter_intersection(
    const std::vector<filter_chain<_Type>>& _chains,
    const std::vector<_Type>&               _input)
{
    std::vector<std::size_t> hit_count(_input.size(), 0);

    for (const auto& chain : _chains)
    {
        auto result = chain.apply(_input);

        for (auto idx : result.indices())
        {
            ++hit_count[idx];
        }
    }

    std::vector<_Type>       elements;
    std::vector<std::size_t> indices;
    std::size_t              chain_count = _chains.size();

    for (std::size_t i = 0; i < _input.size(); ++i)
    {
        if (hit_count[i] == chain_count)
        {
            elements.push_back(_input[i]);
            indices.push_back(i);
        }
    }

    return filter_result<_Type>(std::move(elements),
                                std::move(indices));
}

// filter_difference
//   function: applies difference semantics (A - B).
// An element is included if it passes _include but not _exclude.
template<typename _Type>
D_NODISCARD
filter_result<_Type>
filter_difference(const filter_chain<_Type>& _include,
                  const filter_chain<_Type>& _exclude,
                  const std::vector<_Type>&  _input)
{
    auto included = _include.apply(_input);
    auto excluded = _exclude.apply(_input);

    std::vector<bool> excluded_set(_input.size(), false);

    for (auto idx : excluded.indices())
    {
        excluded_set[idx] = true;
    }

    std::vector<_Type>       elements;
    std::vector<std::size_t> indices;

    for (auto idx : included.indices())
    {
        if (!excluded_set[idx])
        {
            elements.push_back(_input[idx]);
            indices.push_back(idx);
        }
    }

    return filter_result<_Type>(std::move(elements),
                                std::move(indices));
}


///////////////////////////////////////////////////////////////////////////////
///             VI.   FILTER ITERATOR                                       ///
///////////////////////////////////////////////////////////////////////////////

// filter_iterator
//   class: lazily iterates over filtered results.
template<typename _Type>
class filter_iterator
{
private:
    const std::vector<_Type>* m_input;
    std::vector<std::size_t>  m_indices;
    std::size_t               m_pos;

public:
    filter_iterator(const std::vector<_Type>&  _input,
                    const filter_chain<_Type>& _chain)
        : m_input(&_input)
        , m_pos(0)
    {
        auto result = _chain.apply(_input);

        m_indices = result.indices();
    }

    D_NODISCARD
    bool has_next() const { return m_pos < m_indices.size(); }

    D_NODISCARD
    const _Type& next()
    {
        return (*m_input)[m_indices[m_pos++]];
    }

    void reset()
    {
        m_pos = 0;

        return;
    }

    D_NODISCARD
    std::size_t remaining() const
    {
        return m_indices.size() - m_pos;
    }
};


///////////////////////////////////////////////////////////////////////////////
///             VII.  FLUENT FILTER BUILDER                                 ///
///////////////////////////////////////////////////////////////////////////////

// filter_builder
//   class: fluent builder for constructing filter chains.
template<typename _Type>
class filter_builder
{
private:
    filter_chain<_Type> m_chain;

public:
    filter_builder() = default;

    // build
    //   static: creates a new fluent filter builder.
    static filter_builder build() { return filter_builder(); }

    // take_first
    filter_builder& take_first(std::size_t _n)
    {
        m_chain.add(internal::make_take_first_op<_Type>(_n));

        return *this;
    }

    // take_last
    filter_builder& take_last(std::size_t _n)
    {
        m_chain.add(internal::make_take_last_op<_Type>(_n));

        return *this;
    }

    // take_nth
    filter_builder& take_nth(std::size_t _n)
    {
        m_chain.add(internal::make_take_nth_op<_Type>(_n));

        return *this;
    }

    // skip_first
    filter_builder& skip_first(std::size_t _n)
    {
        m_chain.add(internal::make_skip_first_op<_Type>(_n));

        return *this;
    }

    // skip_last
    filter_builder& skip_last(std::size_t _n)
    {
        m_chain.add(internal::make_skip_last_op<_Type>(_n));

        return *this;
    }

    // head (take first 1)
    filter_builder& head() { return take_first(1); }

    // tail (take last 1)
    filter_builder& tail() { return take_last(1); }

    // init (all except last)
    filter_builder& init() { return skip_last(1); }

    // rest (all except first)
    filter_builder& rest() { return skip_first(1); }

    // range [start, end)
    filter_builder& range(std::size_t _start, std::size_t _end)
    {
        m_chain.add(internal::make_range_op<_Type>(_start, _end));

        return *this;
    }

    // slice [start:end:step]
    filter_builder& slice(std::size_t _start,
                          std::size_t _end,
                          std::size_t _step)
    {
        m_chain.add(
            internal::make_slice_op<_Type>(_start, _end, _step));

        return *this;
    }

    // where (predicate filter)
    template<typename _Pred,
             typename = typename std::enable_if<
                 is_predicate<_Pred, const _Type&>::value
             >::type>
    filter_builder& where(_Pred _pred)
    {
        m_chain.add(internal::make_where_op<_Type>(
            std::function<bool(const _Type&)>(
                std::move(_pred))));

        return *this;
    }

    // where_not (negated predicate)
    template<typename _Pred,
             typename = typename std::enable_if<
                 is_predicate<_Pred, const _Type&>::value
             >::type>
    filter_builder& where_not(_Pred _pred)
    {
        m_chain.add(internal::make_where_not_op<_Type>(
            std::function<bool(const _Type&)>(
                std::move(_pred))));

        return *this;
    }

    // at (single index)
    filter_builder& at(std::size_t _index)
    {
        m_chain.add(internal::make_indices_op<_Type>(
            std::vector<std::size_t>{_index}));

        return *this;
    }

    // at_indices (multiple indices)
    filter_builder& at_indices(std::vector<std::size_t> _indices)
    {
        m_chain.add(internal::make_indices_op<_Type>(
            std::move(_indices)));

        return *this;
    }

    // distinct (custom equality)
    template<typename _Eq,
             typename = typename std::enable_if<
                 is_callable<_Eq, const _Type&, const _Type&>::value
             >::type>
    filter_builder& distinct(_Eq _eq)
    {
        m_chain.add(internal::make_distinct_op<_Type>(
            std::function<bool(const _Type&, const _Type&)>(
                std::move(_eq))));

        return *this;
    }

    // distinct (default operator==)
    filter_builder& distinct()
    {
        return distinct(
            [](const _Type& _a, const _Type& _b)
            {
                return _a == _b;
            });
    }

    // reverse
    filter_builder& reverse()
    {
        m_chain.add(internal::make_reverse_op<_Type>());

        return *this;
    }

    // apply (execute the chain)
    D_NODISCARD
    filter_result<_Type>
    apply(const std::vector<_Type>& _input) const
    {
        return m_chain.apply(_input);
    }

    // apply (container)
    template<typename _Container,
             typename = typename std::enable_if<
                 std::is_convertible<
                     typename std::decay<decltype(*std::begin(
                         std::declval<const _Container&>()))>::type,
                     _Type>::value
             >::type>
    D_NODISCARD
    filter_result<_Type>
    apply(const _Container& _input) const
    {
        std::vector<_Type> vec(std::begin(_input),
                               std::end(_input));

        return m_chain.apply(vec);
    }

    // build_chain (extract the chain for use in combinators)
    D_NODISCARD
    filter_chain<_Type> build_chain() const
    {
        return m_chain;
    }

    // build_chain (move)
    D_NODISCARD
    filter_chain<_Type> build_chain() &&
    {
        return std::move(m_chain);
    }

    // iterator
    D_NODISCARD
    filter_iterator<_Type>
    iterator(const std::vector<_Type>& _input) const
    {
        return filter_iterator<_Type>(_input, m_chain);
    }

    // any_match
    D_NODISCARD
    bool any_match(const std::vector<_Type>& _input) const
    {
        return !m_chain.apply(_input).empty();
    }

    // all_match
    D_NODISCARD
    bool all_match(const std::vector<_Type>& _input) const
    {
        return m_chain.apply(_input).count() == _input.size();
    }

    // none_match
    D_NODISCARD
    bool none_match(const std::vector<_Type>& _input) const
    {
        return m_chain.apply(_input).empty();
    }

    // count_matches
    D_NODISCARD
    std::size_t count_matches(const std::vector<_Type>& _input) const
    {
        return m_chain.apply(_input).count();
    }
};


NS_END  // functional
NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_FILTER_HPP_
