/*******************************************************************************
* djinterp [cpp]                                                      filter.hpp
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
*   auto result = DFilterBuilder<int>::build()
*       .skip_first(5)
*       .where([](int x) { return x > 0; })
*       .take_first(10)
*       .distinct()
*       .apply(my_data);
*
*   // combinator
*   auto combined = filter_union(
*       DFilterBuilder<int>::build().where(is_positive).build_chain(),
*       DFilterBuilder<int>::build().where(is_even).build_chain());
*   auto result = combined.apply(my_data);
*
*
* path:      /inc/djinterp/cpp/functional/filter.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    FILTER OPERATION TYPE
      ---------------------

II.   FILTER OPERATION FACTORIES
      --------------------------

III.  FILTER RESULT
      -------------

IV.   FILTER CHAIN
      ------------

V.    FILTER COMBINATORS
      ------------------

VI.   FILTER ITERATOR
      ---------------

VII.  FLUENT FILTER BUILDER
      ---------------------

VIII. TYPED FAST-PATH  (de-erased)
      ----------------------------

IX.   FILTERABLE CONTAINER TRAITS  (folded from filterable_traits.hpp)
      ----------------------------------------------------------------

X.    FILTER STRUCTURAL TRAITS & CONCEPTS
      -----------------------------------
*/

#ifndef DJINTERP_FUNCTIONAL_FILTER_HPP
#define DJINTERP_FUNCTIONAL_FILTER_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../djinterp.hpp"
#include "../meta/member_types.hpp"  // has_value_type
#include "../meta/type_utility.hpp"  // void_t, clean
#include "../meta/type_traits.hpp"   // is_detected, detected_or_t, nonesuch
#include "./functional_common.hpp"


NS_DJINTERP

// is_callable / is_predicate are flat djinterp traits (the functional sub-
// namespace has been retired; see the functional_traits aggregator). The
// unqualified references in the builder/typed-filter SFINAE below resolve to
// them directly.


//   DUAL DOMAIN (boundary).  filter is a runtime collection-processing library:
// its positional / predicate / set-theoretic operations and lazy iteration
// traverse and materialize sequences at run time.  What lifts to compile time is
// COMPOSITION - the operation objects and the fluent builder are
// constexpr-constructible, so a filter pipeline's shape is fixed during
// translation - and the predicate threaded through may read carrier leaves
// (val_t / type_t).  The COMPILE-TIME analog of predicate-filtering a sequence
// is the filter transducer folding reduce_ct / a DValueList (see transducer.hpp
// and reduce.hpp).  The callable vocabulary (is_callable / callable_result_t /
// is_predicate) comes from functional_traits.

///////////////////////////////////////////////////////////////////////////////
///             I.    FILTER OPERATION TYPE                                 ///
///////////////////////////////////////////////////////////////////////////////

// DFilterOpFn
//   type: a filter operation is a function that accepts an input
// vector and returns the indices of elements that pass.
template<typename Type>
using DFilterOpFn = std::function<
    std::vector<std::size_t>(const std::vector<Type>&)>;


///////////////////////////////////////////////////////////////////////////////
///             II.   FILTER OPERATION FACTORIES                            ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL
    // make_take_first_op
    //   helper: keeps the first _n elements.
    template<typename Type>
    DFilterOpFn<Type> make_take_first_op(
        std::size_t _n
    )
    {
        return [_n](const std::vector<Type>& _input)
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
    template<typename Type>
    DFilterOpFn<Type> make_take_last_op(
        std::size_t _n
    )
    {
        return [_n](const std::vector<Type>& _input)
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
    template<typename Type>
    DFilterOpFn<Type> make_skip_first_op(
        std::size_t _n
    )
    {
        return [_n](const std::vector<Type>& _input)
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
    template<typename Type>
    DFilterOpFn<Type> make_skip_last_op(
        std::size_t _n
    )
    {
        return [_n](const std::vector<Type>& _input)
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
    template<typename Type>
    DFilterOpFn<Type> make_take_nth_op(
        std::size_t _n
    )
    {
        return [_n](const std::vector<Type>& _input)
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
    template<typename Type>
    DFilterOpFn<Type> make_range_op(
        std::size_t _start,
        std::size_t _end
    )
    {
        return [_start, _end](const std::vector<Type>& _input)
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
    template<typename Type>
    DFilterOpFn<Type> make_slice_op(
        std::size_t _start,
        std::size_t _end,
        std::size_t _step
    )
    {
        return [_start, _end, _step](const std::vector<Type>& _input)
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
    template<typename Type>
    DFilterOpFn<Type> make_where_op(
        std::function<bool(const Type&)> _pred
    )
    {
        return [_pred](const std::vector<Type>& _input)
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
    template<typename Type>
    DFilterOpFn<Type> make_where_not_op(
        std::function<bool(const Type&)> _pred
    )
    {
        return [_pred](const std::vector<Type>& _input)
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
    template<typename Type>
    DFilterOpFn<Type> make_indices_op(
        std::vector<std::size_t> _indices
    )
    {
        return [_indices](const std::vector<Type>& _input)
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
    template<typename Type>
    DFilterOpFn<Type> make_distinct_op(
        std::function<bool(const Type&, const Type&)> _eq
    )
    {
        return [_eq](const std::vector<Type>& _input)
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
    template<typename Type>
    DFilterOpFn<Type> make_reverse_op()
    {
        return [](const std::vector<Type>& _input)
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

// DFilterResultStatus
//   enum: status of a filter operation.
enum class DFilterResultStatus
{
    success    =  0,
    empty      =  1,
    error      = -1,
    invalid    = -2,
    no_memory  = -3
};

// DFilterResult
//   class: result of applying a filter operation.
template<typename Type>
class DFilterResult
{
private:
    std::vector<Type>       m_elements;
    std::vector<std::size_t> m_indices;
    DFilterResultStatus     m_status;
    std::string              m_error_message;

public:
    // success constructor
    DFilterResult(std::vector<Type>&&       _elements,
                  std::vector<std::size_t>&& _indices)
        : m_elements(std::move(_elements)),
          m_indices(std::move(_indices)),
          m_status(m_elements.empty()
                   ? DFilterResultStatus::empty
                   : DFilterResultStatus::success)
    {}

    // error constructor
    explicit DFilterResult(
        DFilterResultStatus _status,
        std::string          _msg = ""
    ) : m_status(_status),
        m_error_message(std::move(_msg))
    {}

    D_NODISCARD bool ok() const
    {
        return m_status == DFilterResultStatus::success;
    }

    D_NODISCARD bool
    empty() const { return m_elements.empty(); }

    D_NODISCARD
    DFilterResultStatus status() const { return m_status; }

    D_NODISCARD
    const std::string& error_message() const
    {
        return m_error_message;
    }

    D_NODISCARD
    std::size_t count() const { return m_elements.size(); }

    D_NODISCARD
    const std::vector<Type>& elements() const
    {
        return m_elements;
    }

    D_NODISCARD
    std::vector<Type> take_elements()
    {
        return std::move(m_elements);
    }

    D_NODISCARD
    const std::vector<std::size_t>& indices() const
    {
        return m_indices;
    }

    typename std::vector<Type>::const_iterator begin() const
    {
        return m_elements.begin();
    }

    typename std::vector<Type>::const_iterator end() const
    {
        return m_elements.end();
    }
};


///////////////////////////////////////////////////////////////////////////////
///             IV.   FILTER CHAIN                                          ///
///////////////////////////////////////////////////////////////////////////////

// DFilterChain
//   class: chain of sequential filter operations.
template<typename Type>
class DFilterChain
{
private:
    std::vector<DFilterOpFn<Type>> m_operations;

public:
    DFilterChain() = default;

    DFilterChain(
        const DFilterChain&
    ) = default;
    DFilterChain& operator=(
        const DFilterChain&
    ) = default;
    DFilterChain(
        DFilterChain&&
    ) = default;
    DFilterChain& operator=(
        DFilterChain&&
    ) = default;

    // add
    //   method: adds an operation to the chain.
    void add(
        DFilterOpFn<Type> _op
    )
    {
        m_operations.push_back(std::move(_op));

        return;
    }

    // apply
    //   method: applies the chain to input data and returns a
    // filter result.
    D_NODISCARD
    DFilterResult<Type> apply(
        const std::vector<Type>& _input
    ) const
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
            std::vector<Type> sub;

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
        std::vector<Type> result_elements;

        result_elements.reserve(current_indices.size());

        for (auto idx : current_indices)
        {
            result_elements.push_back(_input[idx]);
        }

        return DFilterResult<Type>(std::move(result_elements),
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
template<typename Type>
D_NODISCARD
DFilterResult<Type> filter_union(
    const std::vector<DFilterChain<Type>>& _chains,
    const std::vector<Type>&               _input
)
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

    std::vector<Type>       elements;
    std::vector<std::size_t> indices;

    for (std::size_t i = 0; i < _input.size(); ++i)
    {
        if (included[i])
        {
            elements.push_back(_input[i]);
            indices.push_back(i);
        }
    }

    return DFilterResult<Type>(std::move(elements),
                                std::move(indices));
}

// filter_intersection
//   function: applies intersection semantics (AND) over multiple
// chains.  An element is included only if it passes all chains.
template<typename Type>
D_NODISCARD
DFilterResult<Type> filter_intersection(
    const std::vector<DFilterChain<Type>>& _chains,
    const std::vector<Type>&               _input
)
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

    std::vector<Type>       elements;
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

    return DFilterResult<Type>(std::move(elements),
                                std::move(indices));
}

// filter_difference
//   function: applies difference semantics (A - B).
// An element is included if it passes _include but not _exclude.
template<typename Type>
D_NODISCARD
DFilterResult<Type> filter_difference(
    const DFilterChain<Type>& _include,
    const DFilterChain<Type>& _exclude,
    const std::vector<Type>&  _input
)
{
    auto included = _include.apply(_input);
    auto excluded = _exclude.apply(_input);

    std::vector<bool> excluded_set(_input.size(), false);

    for (auto idx : excluded.indices())
    {
        excluded_set[idx] = true;
    }

    std::vector<Type>       elements;
    std::vector<std::size_t> indices;

    for (auto idx : included.indices())
    {
        if (!excluded_set[idx])
        {
            elements.push_back(_input[idx]);
            indices.push_back(idx);
        }
    }

    return DFilterResult<Type>(std::move(elements),
                                std::move(indices));
}


///////////////////////////////////////////////////////////////////////////////
///             VI.   FILTER ITERATOR                                       ///
///////////////////////////////////////////////////////////////////////////////

// DFilterIterator
//   class: lazily iterates over filtered results.
template<typename Type>
class DFilterIterator
{
private:
    const std::vector<Type>* m_input;
    std::vector<std::size_t>  m_indices;
    std::size_t               m_pos;

public:
    DFilterIterator(const std::vector<Type>&  _input,
                    const DFilterChain<Type>& _chain)
        : m_input(&_input)
        , m_pos(0)
    {
        auto result = _chain.apply(_input);

        m_indices = result.indices();
    }

    D_NODISCARD
    bool has_next() const { return m_pos < m_indices.size(); }

    D_NODISCARD
    const Type& next()
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

// DFilterBuilder
//   class: fluent builder for constructing filter chains.
template<typename Type>
class DFilterBuilder
{
private:
    DFilterChain<Type> m_chain;

public:
    DFilterBuilder() = default;

    // build
    //   static: creates a new fluent filter builder.
    static DFilterBuilder build() { return DFilterBuilder(); }

    // take_first
    DFilterBuilder& take_first(
        std::size_t _n
    )
    {
        m_chain.add(internal::make_take_first_op<Type>(_n));

        return *this;
    }

    // take_last
    DFilterBuilder& take_last(
        std::size_t _n
    )
    {
        m_chain.add(internal::make_take_last_op<Type>(_n));

        return *this;
    }

    // take_nth
    DFilterBuilder& take_nth(
        std::size_t _n
    )
    {
        m_chain.add(internal::make_take_nth_op<Type>(_n));

        return *this;
    }

    // skip_first
    DFilterBuilder& skip_first(
        std::size_t _n
    )
    {
        m_chain.add(internal::make_skip_first_op<Type>(_n));

        return *this;
    }

    // skip_last
    DFilterBuilder& skip_last(
        std::size_t _n
    )
    {
        m_chain.add(internal::make_skip_last_op<Type>(_n));

        return *this;
    }

    // head (take first 1)
    DFilterBuilder& head() { return take_first(1); }

    // tail (take last 1)
    DFilterBuilder& tail() { return take_last(1); }

    // init (all except last)
    DFilterBuilder& init() { return skip_last(1); }

    // rest (all except first)
    DFilterBuilder& rest() { return skip_first(1); }

    // range [start, end)
    DFilterBuilder& range(
        std::size_t _start,
        std::size_t _end
    )
    {
        m_chain.add(internal::make_range_op<Type>(_start, _end));

        return *this;
    }

    // slice [start:end:step]
    DFilterBuilder& slice(
        std::size_t _start,
        std::size_t _end,
        std::size_t _step
    )
    {
        m_chain.add(
            internal::make_slice_op<Type>(_start, _end, _step));

        return *this;
    }

    // where (predicate filter)
    template<typename Pred,
             typename = typename std::enable_if<
                 is_predicate<Pred, const Type&>::value
             >::type>
    DFilterBuilder& where(Pred _pred)
    {
        m_chain.add(internal::make_where_op<Type>(
            std::function<bool(const Type&)>(
                std::move(_pred))));

        return *this;
    }

    // where_not (negated predicate)
    template<typename Pred,
             typename = typename std::enable_if<
                 is_predicate<Pred, const Type&>::value
             >::type>
    DFilterBuilder& where_not(Pred _pred)
    {
        m_chain.add(internal::make_where_not_op<Type>(
            std::function<bool(const Type&)>(
                std::move(_pred))));

        return *this;
    }

    // at (single index)
    DFilterBuilder& at(
        std::size_t _index
    )
    {
        m_chain.add(internal::make_indices_op<Type>(
            std::vector<std::size_t>{_index}));

        return *this;
    }

    // at_indices (multiple indices)
    DFilterBuilder& at_indices(
        std::vector<std::size_t> _indices
    )
    {
        m_chain.add(internal::make_indices_op<Type>(
            std::move(_indices)));

        return *this;
    }

    // distinct (custom equality)
    template<typename Eq,
             typename = typename std::enable_if<
                 is_callable<Eq, const Type&, const Type&>::value
             >::type>
    DFilterBuilder& distinct(Eq _eq)
    {
        m_chain.add(internal::make_distinct_op<Type>(
            std::function<bool(const Type&, const Type&)>(
                std::move(_eq))));

        return *this;
    }

    // distinct (default operator==)
    DFilterBuilder& distinct()
    {
        return distinct(
            [](const Type& _a, const Type& _b)
            {
                return _a == _b;
            });
    }

    // reverse
    DFilterBuilder& reverse()
    {
        m_chain.add(internal::make_reverse_op<Type>());

        return *this;
    }

    // apply (execute the chain)
    D_NODISCARD
    DFilterResult<Type> apply(
        const std::vector<Type>& _input
    ) const
    {
        return m_chain.apply(_input);
    }

    // apply (container)
    template<typename Container,
             typename = typename std::enable_if<
                 std::is_convertible<
                     typename std::decay<decltype(*std::begin(
                         std::declval<const Container&>()))>::type,
                     Type>::value
             >::type>
    D_NODISCARD
    DFilterResult<Type> apply(
        const Container& _input
    ) const
    {
        std::vector<Type> vec(std::begin(_input),
                               std::end(_input));

        return m_chain.apply(vec);
    }

    // build_chain (extract the chain for use in combinators)
    //   NOTE: this const overload is explicitly lvalue-ref-qualified
    // (const &) so it can coexist with the rvalue (&&) overload below.
    // A non-ref-qualified const member cannot be overloaded against a
    // ref-qualified one; the original left this unqualified, which is
    // ill-formed. (fixed 2026-05-27)
    D_NODISCARD
    DFilterChain<Type> build_chain() const &
    {
        return m_chain;
    }

    // build_chain (move)
    D_NODISCARD
    DFilterChain<Type> build_chain() &&
    {
        return std::move(m_chain);
    }

    // iterator
    D_NODISCARD
    DFilterIterator<Type> iterator(
        const std::vector<Type>& _input
    ) const
    {
        return DFilterIterator<Type>(_input, m_chain);
    }

    // any_match
    D_NODISCARD
    bool any_match(
        const std::vector<Type>& _input
    ) const
    {
        return !m_chain.apply(_input).empty();
    }

    // all_match
    D_NODISCARD
    bool all_match(
        const std::vector<Type>& _input
    ) const
    {
        return m_chain.apply(_input).count() == _input.size();
    }

    // none_match
    D_NODISCARD
    bool none_match(
        const std::vector<Type>& _input
    ) const
    {
        return m_chain.apply(_input).empty();
    }

    // count_matches
    D_NODISCARD
    std::size_t count_matches(
        const std::vector<Type>& _input
    ) const
    {
        return m_chain.apply(_input).count();
    }
};


///////////////////////////////////////////////////////////////////////////////
///             VIII. TYPED FAST-PATH  (de-erased)                          ///
///////////////////////////////////////////////////////////////////////////////
// The DFilterChain / DFilterBuilder above type-erase each operation via
// DFilterOpFn = std::function, which is required for the heterogeneous
// storage that filter_union / filter_intersection / filter_difference
// rely on (they hold a std::vector<DFilterChain<Type>>).
//
//   For the common single-chain case, the typed fast-path below avoids
// std::function entirely: each operation wraps its predecessor in a
// stored-by-value step functor producing an index vector, exactly as
// DFnBuilder does. This inlines the whole chain. When a caller needs the
// erased form (to feed a combinator), DTypedFilter::to_chain() lowers the
// typed chain into a DFilterChain<Type> by wrapping it in one
// std::function.

NS_INTERNAL

    // DTypedIdentity: seed; selects every index.
    template<typename Type>
    struct DTypedIdentity
    {
        std::vector<std::size_t> operator()(
            const std::vector<Type>& _in
        ) const
        {
            std::vector<std::size_t> idx;

            idx.reserve(_in.size());

            for (std::size_t i = 0; i < _in.size(); ++i)
            {
                idx.push_back(i);
            }

            return idx;
        }
    };

    // DTypedWhere: keeps indices whose element satisfies Pred.
    template<typename Type,
             typename Prev,
             typename Pred>
    class DTypedWhere
    {
    public:
        DTypedWhere(const Prev& _prev, const Pred& _pred)
            : m_prev(_prev), m_pred(_pred) {}

        std::vector<std::size_t> operator()(
            const std::vector<Type>& _in
        ) const
        {
            std::vector<std::size_t> prev = m_prev(_in);
            std::vector<std::size_t> result;

            for (std::size_t idx : prev)
            {
                if (m_pred(_in[idx])) { result.push_back(idx); }
            }

            return result;
        }

    private:
        Prev m_prev;
        Pred m_pred;
    };

    // DTypedTake: keeps the first _n surviving indices.
    template<typename Type,
             typename Prev>
    class DTypedTake
    {
    public:
        DTypedTake(const Prev& _prev, std::size_t _n)
            : m_prev(_prev), m_n(_n) {}

        std::vector<std::size_t> operator()(
            const std::vector<Type>& _in
        ) const
        {
            std::vector<std::size_t> prev = m_prev(_in);

            if (m_n < prev.size()) { prev.resize(m_n); }

            return prev;
        }

    private:
        Prev        m_prev;
        std::size_t m_n;
    };

    // DTypedSkip: drops the first _n surviving indices.
    template<typename Type,
             typename Prev>
    class DTypedSkip
    {
    public:
        DTypedSkip(const Prev& _prev, std::size_t _n)
            : m_prev(_prev), m_n(_n) {}

        std::vector<std::size_t> operator()(
            const std::vector<Type>& _in
        ) const
        {
            std::vector<std::size_t> prev = m_prev(_in);

            if (m_n >= prev.size()) { return std::vector<std::size_t>(); }

            return std::vector<std::size_t>(prev.begin() +
                static_cast<std::vector<std::size_t>::difference_type>(m_n),
                prev.end());
        }

    private:
        Prev        m_prev;
        std::size_t m_n;
    };

NS_END  // internal


// DTypedFilter
//   class: a single typed filter chain. Chain is the concrete composed
// index-producing functor (vector<Type> -> vector<size_t>). Each
// fluent operation returns a new DTypedFilter with a wrapped Chain.
template<typename Type,
         typename Chain = internal::DTypedIdentity<Type> >
class DTypedFilter
{
public:
    typedef Chain chain_type;

    explicit DTypedFilter(Chain _chain) : m_chain(std::move(_chain)) {}

    // create: seeds a typed filter selecting all elements.
    static DTypedFilter<Type, internal::DTypedIdentity<Type> >
    create()
    {
        return DTypedFilter<Type, internal::DTypedIdentity<Type> >(
            internal::DTypedIdentity<Type>());
    }

    // where
    template<typename Pred,
             typename = typename std::enable_if<
                 is_predicate<Pred, const Type&>::value>::type>
    D_NODISCARD
    DTypedFilter<Type, internal::DTypedWhere<Type, Chain, Pred> >
    where(Pred _pred) const
    {
        typedef internal::DTypedWhere<Type, Chain, Pred> new_chain;

        return DTypedFilter<Type, new_chain>(new_chain(m_chain, _pred));
    }

    // take_first
    D_NODISCARD
    DTypedFilter<Type, internal::DTypedTake<Type, Chain> >
    take_first(std::size_t _n) const
    {
        typedef internal::DTypedTake<Type, Chain> new_chain;

        return DTypedFilter<Type, new_chain>(new_chain(m_chain, _n));
    }

    // skip_first
    D_NODISCARD
    DTypedFilter<Type, internal::DTypedSkip<Type, Chain> >
    skip_first(std::size_t _n) const
    {
        typedef internal::DTypedSkip<Type, Chain> new_chain;

        return DTypedFilter<Type, new_chain>(new_chain(m_chain, _n));
    }

    // apply: run the chain, returning a DFilterResult.
    D_NODISCARD
    DFilterResult<Type>
    apply(const std::vector<Type>& _input) const
    {
        std::vector<std::size_t> indices = m_chain(_input);
        std::vector<Type>       elements;

        elements.reserve(indices.size());

        for (std::size_t idx : indices)
        {
            elements.push_back(_input[idx]);
        }

        return DFilterResult<Type>(std::move(elements),
                                    std::move(indices));
    }

    // to_chain
    //   method: lowers the typed chain into a std::function-backed
    // DFilterChain<Type>, for use with the set-theoretic combinators
    // (filter_union / filter_intersection / filter_difference) that
    // require homogeneous storage. This is the one place the typed
    // path pays for a single std::function wrap.
    D_NODISCARD
    DFilterChain<Type> to_chain() const
    {
        Chain chain = m_chain;
        DFilterChain<Type> result;

        result.add(DFilterOpFn<Type>(
            [chain](const std::vector<Type>& _in)
                -> std::vector<std::size_t>
            {
                return chain(_in);
            }));

        return result;
    }

    D_NODISCARD
    const Chain& chain() const { return m_chain; }

    template<typename T, typename C>
    friend class DTypedFilter;

private:
    Chain m_chain;
};


// make_typed_filter
//   function: seeds a typed filter for the given element type.
template<typename Type>
D_NODISCARD
DTypedFilter<Type, internal::DTypedIdentity<Type> > make_typed_filter()
{
    return DTypedFilter<Type, internal::DTypedIdentity<Type> >::create();
}


///////////////////////////////////////////////////////////////////////////////
///             IX.   FILTERABLE CONTAINER TRAITS                           ///
///////////////////////////////////////////////////////////////////////////////
//   Folded in from filterable_traits.hpp (which this header supersedes).
// SFINAE-based detection of whether a container type supports filtering:
// iteration (begin/end), element access (value_type), output construction
// (push_back / insert), and an optional native .filter() method. The
// composite is_filterable aggregates these.
//
//   This section answers only the filter's own questions. The general ones
// have one owner each, outside the functional module: is_iterable is
// container/iterator/iterator_traits.hpp's (C++17 and up), has_value_type
// is meta/member_types.hpp's, and has_push_back / has_insert are
// container/traits/container_traits.hpp's. This section used to define all
// of them itself, with has_begin, has_end, has_size and has_empty besides;
// once the namespaces were flattened into djinterp, no unit could include it
// beside container_traits.hpp or iterator_traits.hpp. The composite below
// detects iteration and its two insertions privately, in internal, so the
// header keeps its C++11 floor and pulls in no container header for them.
// (changed 2026-11-09)
//
//   The detection idiom underneath -- is_detected, detected_or_t, nonesuch --
// is core/meta/type_traits.hpp's. (This header used to keep its own copy in
// djinterp::internal, as accumulator.hpp did, on the belief that identical
// definitions do not conflict; within one translation unit they do, so no
// unit could include it beside either.)

NS_INTERNAL

    // begin_expression / end_expression
    //   trait: expression aliases for std::begin / std::end on an lvalue,
    // the iteration is_filterable reads.
    template<typename Type>
    using begin_expression = decltype(std::begin(std::declval<Type&>()));

    template<typename Type>
    using end_expression = decltype(std::end(std::declval<Type&>()));

    // is_filter_iterable
    //   trait: true when std::begin and std::end both apply to a Type lvalue.
    template<typename Type>
    struct is_filter_iterable
        : std::integral_constant<bool,
              ( is_detected<begin_expression, Type>::value &&
                is_detected<end_expression, Type>::value )>
    {};

    // value_type_expr
    //   trait: expression alias for nested value_type detection.
    template<typename Type>
    using value_type_expr = typename Type::value_type;

    // push_back_expression
    //   trait: expression alias for push_back() detection.
    template<typename Type>
    using push_back_expression = decltype(
        std::declval<Type&>().push_back(
            std::declval<typename Type::value_type>()));

    // insert_expression
    //   trait: expression alias for insert(iterator, value) detection.
    template<typename Type>
    using insert_expression = decltype(
        std::declval<Type&>().insert(
            std::declval<Type&>().end(),
            std::declval<typename Type::value_type>()));

    // filter_method_expr
    //   trait: expression alias detecting a .filter() member that accepts a
    // unary predicate.  The predicate signature is bool(const value_type&).
    template<typename Type>
    using filter_method_expr = decltype(
        std::declval<const Type&>().filter(
            std::declval<bool(*)(const typename Type::value_type&)>()));

NS_END  // internal

// has_filter_method
//   trait: detects whether Type has a filter(predicate) member function.
template<typename Type>
struct has_filter_method
{
    static D_CONSTEXPR bool value =
        is_detected<internal::filter_method_expr, Type>::value;
};

// is_output_capable
//   trait: true when Type supports at least one insertion method
// (push_back or iterator-based insert), enabling construction of a filtered
// result container.
template<typename Type>
struct is_output_capable
{
    static D_CONSTEXPR bool value =
        ( is_detected<internal::push_back_expression, Type>::value ||
          is_detected<internal::insert_expression, Type>::value );
};

// is_filterable
//   trait: true when Type satisfies the complete filterable contract:
// iterable, exposes value_type, and supports result construction.
template<typename Type>
struct is_filterable
{
private:
    typedef typename std::remove_cv<
                typename std::remove_reference<Type>::type>::type clean_type;

public:
    static D_CONSTEXPR bool value =
        ( internal::is_filter_iterable<clean_type>::value &&
          has_value_type<clean_type>::value               &&
          is_output_capable<clean_type>::value );
};

// filterable_value_t
//   type: extracts value_type from a filterable container, or nonesuch if
// unavailable.
template<typename Type>
using filterable_value_t =
    detected_or_t<nonesuch,
                            internal::value_type_expr, Type>;


///////////////////////////////////////////////////////////////////////////////
///             X.    FILTER STRUCTURAL TRAITS & CONCEPTS                   ///
///////////////////////////////////////////////////////////////////////////////
//   Where section IX answers "is this CONTAINER filterable?", this section
// answers "is this type one of the filter module's own shapes?": a filter
// operation (the DFilterOpFn protocol), an applicable filter (chain /
// builder / DTypedFilter — anything with .apply over a vector), or a filter
// result. Detection is expression-based via the same is_detected idiom, so a
// raw lambda of the right shape is recognised as readily as DFilterOpFn or a
// typed chain.

NS_INTERNAL

    // op_call_expr
    //   trait: expression alias for invoking Fn on a const vector<Elem>&.
    template<typename Fn,
             typename Elem>
    using op_call_expr = decltype(
        std::declval<const Fn&>()(
            std::declval<const std::vector<Elem>&>()));

    // filter_apply_expr
    //   trait: expression alias for Type.apply(const vector<Elem>&).
    template<typename Type,
             typename Elem>
    using filter_apply_expr = decltype(
        std::declval<const Type&>().apply(
            std::declval<const std::vector<Elem>&>()));

    // result_ok_expr / result_indices_expr / result_elements_expr
    //   traits: expression aliases for the DFilterResult inspection surface.
    template<typename Type>
    using result_ok_expr = decltype(std::declval<const Type&>().ok());

    template<typename Type>
    using result_indices_expr =
        decltype(std::declval<const Type&>().indices());

    template<typename Type>
    using result_elements_expr =
        decltype(std::declval<const Type&>().elements());

NS_END  // internal


// is_filter_operation
//   trait: true when Fn is callable as (const std::vector<Elem>&) and the
// result is convertible to std::vector<std::size_t> -- the DFilterOpFn
// protocol. Satisfied by DFilterOpFn<Elem>, the internal typed chains, the
// make_*_op results, and any user lambda of the same shape.
template<typename Fn,
         typename Elem>
struct is_filter_operation
{
private:
    typedef typename clean<Fn>::type clean_fn;
    typedef detected_or_t<nonesuch,
                                    internal::op_call_expr,
                                    clean_fn, Elem> result_t;

public:
    static D_CONSTEXPR bool value =
        std::is_convertible<result_t,
                            std::vector<std::size_t> >::value;
};


// is_filter_applicable
//   trait: true when Type exposes .apply(const std::vector<Elem>&) -- the
// shape shared by DFilterChain, DFilterBuilder, and DTypedFilter. The lifted
// "can I run this over a vector<Elem>?" question.
template<typename Type,
         typename Elem>
struct is_filter_applicable
{
    static D_CONSTEXPR bool value =
        is_detected<internal::filter_apply_expr,
                              typename clean<Type>::type,
                              Elem>::value;
};


// is_filter_result
//   trait: true when Type exposes the DFilterResult inspection surface
// (ok / indices / elements). Element-type independent.
template<typename Type>
struct is_filter_result
{
private:
    typedef typename clean<Type>::type clean_type;

public:
    static D_CONSTEXPR bool value =
        ( is_detected<internal::result_ok_expr,
                                clean_type>::value       &&
          is_detected<internal::result_indices_expr,
                                clean_type>::value       &&
          is_detected<internal::result_elements_expr,
                                clean_type>::value );
};


// ---- convenience aliases ----
// Variable templates are a C++14 feature; gate the *_v shorthands so the
// header stays clean under -std=c++11 -pedantic. Pre-C++14 callers use the
// ::value form.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

// is_filterable_v
//   constant: shorthand for is_filterable<Type>::value.
template<typename Type>
static D_CONSTEXPR bool is_filterable_v = is_filterable<Type>::value;

// has_filter_method_v
//   constant: shorthand for has_filter_method<Type>::value.
template<typename Type>
static D_CONSTEXPR bool has_filter_method_v = has_filter_method<Type>::value;

// is_filter_operation_v
//   constant: shorthand for is_filter_operation<Fn, Elem>::value.
template<typename Fn,
         typename Elem>
static D_CONSTEXPR bool is_filter_operation_v =
    is_filter_operation<Fn, Elem>::value;

// is_filter_applicable_v
//   constant: shorthand for is_filter_applicable<Type, Elem>::value.
template<typename Type,
         typename Elem>
static D_CONSTEXPR bool is_filter_applicable_v =
    is_filter_applicable<Type, Elem>::value;

// is_filter_result_v
//   constant: shorthand for is_filter_result<Type>::value.
template<typename Type>
static D_CONSTEXPR bool is_filter_result_v = is_filter_result<Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


// ---- concepts (C++20) ----
#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// filterable_c
//   concept: satisfied when Type is a filterable container (section IX).
template<typename Type>
concept filterable_c = is_filterable<Type>::value;

// filter_operation_c
//   concept: satisfied when Fn models the DFilterOpFn protocol over
// Elem -- callable on a const vector<Elem>& with a result convertible to
// vector<size_t>. Delegates to the trait so it needs no <concepts> include.
template<typename Fn,
         typename Elem>
concept filter_operation_c = is_filter_operation<Fn, Elem>::value;

// filter_applicable_c
//   concept: satisfied when Type can be applied over a vector<Elem> --
// i.e. exposes .apply(const vector<Elem>&). Modelled by DFilterChain,
// DFilterBuilder, and DTypedFilter.
template<typename Type,
         typename Elem>
concept filter_applicable_c =
    requires(const Type& _t, const std::vector<Elem>& _in)
    {
        _t.apply(_in);
    };

// filter_result_c
//   concept: satisfied when Type exposes the DFilterResult inspection
// surface (ok / indices / elements).
template<typename Type>
concept filter_result_c =
    requires(const Type& _r)
    {
        _r.ok();
        _r.indices();
        _r.elements();
    };

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_FUNCTIONAL_FILTER_HPP
