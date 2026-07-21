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
* VIII. TYPED FAST-PATH  (de-erased)
* IX.   FILTERABLE CONTAINER TRAITS  (folded from filterable_traits.hpp)
* X.    FILTER STRUCTURAL TRAITS & CONCEPTS
*
*
* path:      \inc\functional\filter.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.02.19
******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_FILTER_
#define DJINTERP_FUNCTIONAL_FILTER_ 1

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
// is the filter transducer folding reduce_ct / a value_list (see transducer.hpp
// and reduce.hpp).  The callable vocabulary (is_callable / callable_result_t /
// is_predicate) comes from functional_traits.

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
        : m_elements(std::move(_elements)),
          m_indices(std::move(_indices)),
          m_status(m_elements.empty()
                   ? filter_result_status::empty
                   : filter_result_status::success)
    {}

    // error constructor
    explicit filter_result(
        filter_result_status _status,
        std::string          _msg = ""
    ) : m_status(_status),
        m_error_message(std::move(_msg))
    {}

    D_NODISCARD bool
    ok() const
    {
        return m_status == filter_result_status::success;
    }

    D_NODISCARD bool
    empty() const { return m_elements.empty(); }

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
    //   NOTE: this const overload is explicitly lvalue-ref-qualified
    // (const &) so it can coexist with the rvalue (&&) overload below.
    // A non-ref-qualified const member cannot be overloaded against a
    // ref-qualified one; the original left this unqualified, which is
    // ill-formed. (fixed 2026-05-27)
    D_NODISCARD
    filter_chain<_Type> build_chain() const &
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


///////////////////////////////////////////////////////////////////////////////
///             VIII. TYPED FAST-PATH  (de-erased)                          ///
///////////////////////////////////////////////////////////////////////////////
// The filter_chain / filter_builder above type-erase each operation via
// filter_op_fn = std::function, which is required for the heterogeneous
// storage that filter_union / filter_intersection / filter_difference
// rely on (they hold a std::vector<filter_chain<_Type>>).
//
//   For the common single-chain case, the typed fast-path below avoids
// std::function entirely: each operation wraps its predecessor in a
// stored-by-value step functor producing an index vector, exactly as
// fn_builder does. This inlines the whole chain. When a caller needs the
// erased form (to feed a combinator), typed_filter::to_chain() lowers the
// typed chain into a filter_chain<_Type> by wrapping it in one
// std::function.

NS_INTERNAL

    // typed_identity: seed; selects every index.
    template<typename _Type>
    struct typed_identity
    {
        std::vector<std::size_t>
        operator()(const std::vector<_Type>& _in) const
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

    // typed_where: keeps indices whose element satisfies _Pred.
    template<typename _Type,
             typename _Prev,
             typename _Pred>
    class typed_where
    {
    public:
        typed_where(const _Prev& _prev, const _Pred& _pred)
            : m_prev(_prev), m_pred(_pred) {}

        std::vector<std::size_t>
        operator()(const std::vector<_Type>& _in) const
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
        _Prev m_prev;
        _Pred m_pred;
    };

    // typed_take: keeps the first _n surviving indices.
    template<typename _Type,
             typename _Prev>
    class typed_take
    {
    public:
        typed_take(const _Prev& _prev, std::size_t _n)
            : m_prev(_prev), m_n(_n) {}

        std::vector<std::size_t>
        operator()(const std::vector<_Type>& _in) const
        {
            std::vector<std::size_t> prev = m_prev(_in);

            if (m_n < prev.size()) { prev.resize(m_n); }

            return prev;
        }

    private:
        _Prev       m_prev;
        std::size_t m_n;
    };

    // typed_skip: drops the first _n surviving indices.
    template<typename _Type,
             typename _Prev>
    class typed_skip
    {
    public:
        typed_skip(const _Prev& _prev, std::size_t _n)
            : m_prev(_prev), m_n(_n) {}

        std::vector<std::size_t>
        operator()(const std::vector<_Type>& _in) const
        {
            std::vector<std::size_t> prev = m_prev(_in);

            if (m_n >= prev.size()) { return std::vector<std::size_t>(); }

            return std::vector<std::size_t>(prev.begin() +
                static_cast<std::vector<std::size_t>::difference_type>(m_n),
                prev.end());
        }

    private:
        _Prev       m_prev;
        std::size_t m_n;
    };

NS_END  // internal


// typed_filter
//   class: a single typed filter chain. _Chain is the concrete composed
// index-producing functor (vector<_Type> -> vector<size_t>). Each
// fluent operation returns a new typed_filter with a wrapped _Chain.
template<typename _Type,
         typename _Chain = internal::typed_identity<_Type> >
class typed_filter
{
public:
    typedef _Chain chain_type;

    explicit typed_filter(_Chain _chain) : m_chain(std::move(_chain)) {}

    // create: seeds a typed filter selecting all elements.
    static typed_filter<_Type, internal::typed_identity<_Type> >
    create()
    {
        return typed_filter<_Type, internal::typed_identity<_Type> >(
            internal::typed_identity<_Type>());
    }

    // where
    template<typename _Pred,
             typename = typename std::enable_if<
                 is_predicate<_Pred, const _Type&>::value>::type>
    D_NODISCARD
    typed_filter<_Type, internal::typed_where<_Type, _Chain, _Pred> >
    where(_Pred _pred) const
    {
        typedef internal::typed_where<_Type, _Chain, _Pred> new_chain;

        return typed_filter<_Type, new_chain>(new_chain(m_chain, _pred));
    }

    // take_first
    D_NODISCARD
    typed_filter<_Type, internal::typed_take<_Type, _Chain> >
    take_first(std::size_t _n) const
    {
        typedef internal::typed_take<_Type, _Chain> new_chain;

        return typed_filter<_Type, new_chain>(new_chain(m_chain, _n));
    }

    // skip_first
    D_NODISCARD
    typed_filter<_Type, internal::typed_skip<_Type, _Chain> >
    skip_first(std::size_t _n) const
    {
        typedef internal::typed_skip<_Type, _Chain> new_chain;

        return typed_filter<_Type, new_chain>(new_chain(m_chain, _n));
    }

    // apply: run the chain, returning a filter_result.
    D_NODISCARD
    filter_result<_Type>
    apply(const std::vector<_Type>& _input) const
    {
        std::vector<std::size_t> indices = m_chain(_input);
        std::vector<_Type>       elements;

        elements.reserve(indices.size());

        for (std::size_t idx : indices)
        {
            elements.push_back(_input[idx]);
        }

        return filter_result<_Type>(std::move(elements),
                                    std::move(indices));
    }

    // to_chain
    //   method: lowers the typed chain into a std::function-backed
    // filter_chain<_Type>, for use with the set-theoretic combinators
    // (filter_union / filter_intersection / filter_difference) that
    // require homogeneous storage. This is the one place the typed
    // path pays for a single std::function wrap.
    D_NODISCARD
    filter_chain<_Type> to_chain() const
    {
        _Chain chain = m_chain;
        filter_chain<_Type> result;

        result.add(filter_op_fn<_Type>(
            [chain](const std::vector<_Type>& _in)
                -> std::vector<std::size_t>
            {
                return chain(_in);
            }));

        return result;
    }

    D_NODISCARD
    const _Chain& chain() const { return m_chain; }

    template<typename _T, typename _C>
    friend class typed_filter;

private:
    _Chain m_chain;
};


// make_typed_filter
//   function: seeds a typed filter for the given element type.
template<typename _Type>
D_NODISCARD
typed_filter<_Type, internal::typed_identity<_Type> >
make_typed_filter()
{
    return typed_filter<_Type, internal::typed_identity<_Type> >::create();
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
//   NOTE: the void_t / detector / is_detected / detected_or_t idiom below is
// the same one the functional_traits aggregator uses. When this header is
// compiled together with that aggregator, the two definitions are identical
// templates in djinterp::internal and so do not conflict; if a future change
// makes them diverge, gate one behind an include guard.

NS_INTERNAL

    // ---- detection idiom ----
    template<typename...>
    struct make_void { typedef void type; };
    template<typename... _Ts>
    using void_t = typename make_void<_Ts...>::type;

    template<typename _Default,
             typename _AlwaysVoid,
             template<typename...> class _Op,
             typename... _Args>
    struct detector
    {
        typedef std::false_type value_t;
        typedef _Default        type;
    };

    template<typename _Default,
             template<typename...> class _Op,
             typename... _Args>
    struct detector<_Default, void_t<_Op<_Args...> >, _Op, _Args...>
    {
        typedef std::true_type  value_t;
        typedef _Op<_Args...>   type;
    };

    struct nonesuch
    {
        nonesuch()                      = delete;
        ~nonesuch()                     = delete;
        nonesuch(const nonesuch&)       = delete;
        void operator=(const nonesuch&) = delete;
    };

    template<template<typename...> class _Op,
             typename... _Args>
    using is_detected =
        typename detector<nonesuch, void, _Op, _Args...>::value_t;

    template<typename _Default,
             template<typename...> class _Op,
             typename... _Args>
    using detected_or_t =
        typename detector<_Default, void, _Op, _Args...>::type;

    // begin_expression
    //   trait: expression alias for begin() detection.
    template<typename _Type>
    using begin_expression = decltype(std::begin(std::declval<_Type&>()));

    // end_expression
    //   trait: expression alias for end() detection.
    template<typename _Type>
    using end_expression = decltype(std::end(std::declval<_Type&>()));

    // value_type_expr
    //   trait: expression alias for nested value_type detection.
    template<typename _Type>
    using value_type_expr = typename _Type::value_type;

    // push_back_expression
    //   trait: expression alias for push_back() detection.
    template<typename _Type>
    using push_back_expression = decltype(
        std::declval<_Type&>().push_back(
            std::declval<typename _Type::value_type>()));

    // insert_expression
    //   trait: expression alias for insert() detection.
    template<typename _Type>
    using insert_expression = decltype(
        std::declval<_Type&>().insert(
            std::declval<_Type&>().end(),
            std::declval<typename _Type::value_type>()));

    // size_expression
    //   trait: expression alias for size() detection.
    template<typename _Type>
    using size_expression = decltype(std::declval<const _Type&>().size());

    // empty_expression
    //   trait: expression alias for empty() detection.
    template<typename _Type>
    using empty_expression = decltype(std::declval<const _Type&>().empty());

    // iterator_expression
    //   trait: expression alias for nested iterator detection.
    template<typename _Type>
    using iterator_expression = typename _Type::iterator;

    // const_iterator_expression
    //   trait: expression alias for nested const_iterator detection.
    template<typename _Type>
    using const_iterator_expression = typename _Type::const_iterator;

NS_END  // internal

// has_begin
//   trait: detects whether std::begin(_Type&) is well-formed.
template<typename _Type>
struct has_begin
{
    static D_CONSTEXPR bool value =
        internal::is_detected<internal::begin_expression, _Type>::value;
};

// has_end
//   trait: detects whether std::end(_Type&) is well-formed.
template<typename _Type>
struct has_end
{
    static D_CONSTEXPR bool value =
        internal::is_detected<internal::end_expression, _Type>::value;
};

// has_value_type
//   trait: detects whether _Type::value_type exists.
template<typename _Type>
struct has_value_type
{
    static D_CONSTEXPR bool value =
        internal::is_detected<internal::value_type_expr, _Type>::value;
};

// has_push_back
//   trait: detects whether _Type has a push_back(value_type) member.
template<typename _Type>
struct has_push_back
{
    static D_CONSTEXPR bool value =
        internal::is_detected<internal::push_back_expression, _Type>::value;
};

// has_insert
//   trait: detects whether _Type has an insert(iterator, value_type)
// member.
template<typename _Type>
struct has_insert
{
    static D_CONSTEXPR bool value =
        internal::is_detected<internal::insert_expression, _Type>::value;
};

// has_size
//   trait: detects whether _Type has a size() const member.
template<typename _Type>
struct has_size
{
    static D_CONSTEXPR bool value =
        internal::is_detected<internal::size_expression, _Type>::value;
};

// has_empty
//   trait: detects whether _Type has an empty() const member.
template<typename _Type>
struct has_empty
{
    static D_CONSTEXPR bool value =
        internal::is_detected<internal::empty_expression, _Type>::value;
};

// has_iterator
//   trait: detects whether _Type::iterator exists.
template<typename _Type>
struct has_iterator
{
    static D_CONSTEXPR bool value =
        internal::is_detected<internal::iterator_expression, _Type>::value;
};

// has_const_iterator
//   trait: detects whether _Type::const_iterator exists.
template<typename _Type>
struct has_const_iterator
{
    static D_CONSTEXPR bool value =
        internal::is_detected<internal::const_iterator_expression,
                              _Type>::value;
};


NS_INTERNAL

    // filter_method_expr
    //   trait: expression alias detecting a .filter() member that accepts a
    // unary predicate.  The predicate signature is bool(const value_type&).
    template<typename _Type>
    using filter_method_expr = decltype(
        std::declval<const _Type&>().filter(
            std::declval<bool(*)(const typename _Type::value_type&)>()));

NS_END  // internal

// has_filter_method
//   trait: detects whether _Type has a filter(predicate) member function.
template<typename _Type>
struct has_filter_method
{
    static D_CONSTEXPR bool value =
        internal::is_detected<internal::filter_method_expr, _Type>::value;
};

// is_iterable
//   trait: true when _Type supports range-based iteration via std::begin
// and std::end.
template<typename _Type>
struct is_iterable
{
    static D_CONSTEXPR bool value =
        ( has_begin<_Type>::value &&
          has_end<_Type>::value );
};

// is_output_capable
//   trait: true when _Type supports at least one insertion method
// (push_back or iterator-based insert), enabling construction of a filtered
// result container.
template<typename _Type>
struct is_output_capable
{
    static D_CONSTEXPR bool value =
        ( has_push_back<_Type>::value ||
          has_insert<_Type>::value );
};

// is_filterable
//   trait: true when _Type satisfies the complete filterable contract:
// iterable, exposes value_type, and supports result construction.
template<typename _Type>
struct is_filterable
{
private:
    typedef typename std::remove_cv<
                typename std::remove_reference<_Type>::type>::type clean_type;

public:
    static D_CONSTEXPR bool value =
        ( is_iterable<clean_type>::value    &&
          has_value_type<clean_type>::value &&
          is_output_capable<clean_type>::value );
};

// filterable_value_t
//   type: extracts value_type from a filterable container, or nonesuch if
// unavailable.
template<typename _Type>
using filterable_value_t =
    internal::detected_or_t<internal::nonesuch,
                            internal::value_type_expr, _Type>;


///////////////////////////////////////////////////////////////////////////////
///             X.    FILTER STRUCTURAL TRAITS & CONCEPTS                   ///
///////////////////////////////////////////////////////////////////////////////
//   Where section IX answers "is this CONTAINER filterable?", this section
// answers "is this type one of the filter module's own shapes?": a filter
// operation (the filter_op_fn protocol), an applicable filter (chain /
// builder / typed_filter — anything with .apply over a vector), or a filter
// result. Detection is expression-based via the same is_detected idiom, so a
// raw lambda of the right shape is recognised as readily as filter_op_fn or a
// typed chain.

NS_INTERNAL

    // op_call_expr
    //   trait: expression alias for invoking _Fn on a const vector<_Elem>&.
    template<typename _Fn,
             typename _Elem>
    using op_call_expr = decltype(
        std::declval<const _Fn&>()(
            std::declval<const std::vector<_Elem>&>()));

    // filter_apply_expr
    //   trait: expression alias for _Type.apply(const vector<_Elem>&).
    template<typename _Type,
             typename _Elem>
    using filter_apply_expr = decltype(
        std::declval<const _Type&>().apply(
            std::declval<const std::vector<_Elem>&>()));

    // result_ok_expr / result_indices_expr / result_elements_expr
    //   traits: expression aliases for the filter_result inspection surface.
    template<typename _Type>
    using result_ok_expr = decltype(std::declval<const _Type&>().ok());

    template<typename _Type>
    using result_indices_expr =
        decltype(std::declval<const _Type&>().indices());

    template<typename _Type>
    using result_elements_expr =
        decltype(std::declval<const _Type&>().elements());

    // strip
    //   helper: removes reference and cv-qualifiers so the traits below may
    // be queried on references and const types alike.
    template<typename _Type>
    struct strip
    {
        typedef typename std::remove_cv<
                    typename std::remove_reference<_Type>::type>::type type;
    };

NS_END  // internal


// is_filter_operation
//   trait: true when _Fn is callable as (const std::vector<_Elem>&) and the
// result is convertible to std::vector<std::size_t> -- the filter_op_fn
// protocol. Satisfied by filter_op_fn<_Elem>, the internal typed chains, the
// make_*_op results, and any user lambda of the same shape.
template<typename _Fn,
         typename _Elem>
struct is_filter_operation
{
private:
    typedef typename internal::strip<_Fn>::type clean_fn;
    typedef internal::detected_or_t<internal::nonesuch,
                                    internal::op_call_expr,
                                    clean_fn, _Elem> result_t;

public:
    static D_CONSTEXPR bool value =
        std::is_convertible<result_t,
                            std::vector<std::size_t> >::value;
};


// is_filter_applicable
//   trait: true when _Type exposes .apply(const std::vector<_Elem>&) -- the
// shape shared by filter_chain, filter_builder, and typed_filter. The lifted
// "can I run this over a vector<_Elem>?" question.
template<typename _Type,
         typename _Elem>
struct is_filter_applicable
{
    static D_CONSTEXPR bool value =
        internal::is_detected<internal::filter_apply_expr,
                              typename internal::strip<_Type>::type,
                              _Elem>::value;
};


// is_filter_result
//   trait: true when _Type exposes the filter_result inspection surface
// (ok / indices / elements). Element-type independent.
template<typename _Type>
struct is_filter_result
{
private:
    typedef typename internal::strip<_Type>::type clean_type;

public:
    static D_CONSTEXPR bool value =
        ( internal::is_detected<internal::result_ok_expr,
                                clean_type>::value       &&
          internal::is_detected<internal::result_indices_expr,
                                clean_type>::value       &&
          internal::is_detected<internal::result_elements_expr,
                                clean_type>::value );
};


// ---- convenience aliases ----
// Variable templates are a C++14 feature; gate the *_v shorthands so the
// header stays clean under -std=c++11 -pedantic. Pre-C++14 callers use the
// ::value form.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

// is_filterable_v
//   constant: shorthand for is_filterable<_Type>::value.
template<typename _Type>
static D_CONSTEXPR bool is_filterable_v = is_filterable<_Type>::value;

// is_iterable_v
//   constant: shorthand for is_iterable<_Type>::value.
template<typename _Type>
static D_CONSTEXPR bool is_iterable_v = is_iterable<_Type>::value;

// has_filter_method_v
//   constant: shorthand for has_filter_method<_Type>::value.
template<typename _Type>
static D_CONSTEXPR bool has_filter_method_v = has_filter_method<_Type>::value;

// is_filter_operation_v
//   constant: shorthand for is_filter_operation<_Fn, _Elem>::value.
template<typename _Fn,
         typename _Elem>
static D_CONSTEXPR bool is_filter_operation_v =
    is_filter_operation<_Fn, _Elem>::value;

// is_filter_applicable_v
//   constant: shorthand for is_filter_applicable<_Type, _Elem>::value.
template<typename _Type,
         typename _Elem>
static D_CONSTEXPR bool is_filter_applicable_v =
    is_filter_applicable<_Type, _Elem>::value;

// is_filter_result_v
//   constant: shorthand for is_filter_result<_Type>::value.
template<typename _Type>
static D_CONSTEXPR bool is_filter_result_v = is_filter_result<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


// ---- concepts (C++20) ----
#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// filterable_c
//   concept: satisfied when _Type is a filterable container (section IX).
template<typename _Type>
concept filterable_c = is_filterable<_Type>::value;

// filter_operation_c
//   concept: satisfied when _Fn models the filter_op_fn protocol over
// _Elem -- callable on a const vector<_Elem>& with a result convertible to
// vector<size_t>. Delegates to the trait so it needs no <concepts> include.
template<typename _Fn,
         typename _Elem>
concept filter_operation_c = is_filter_operation<_Fn, _Elem>::value;

// filter_applicable_c
//   concept: satisfied when _Type can be applied over a vector<_Elem> --
// i.e. exposes .apply(const vector<_Elem>&). Modelled by filter_chain,
// filter_builder, and typed_filter.
template<typename _Type,
         typename _Elem>
concept filter_applicable_c =
    requires(const _Type& _t, const std::vector<_Elem>& _in)
    {
        _t.apply(_in);
    };

// filter_result_c
//   concept: satisfied when _Type exposes the filter_result inspection
// surface (ok / indices / elements).
template<typename _Type>
concept filter_result_c =
    requires(const _Type& _r)
    {
        _r.ok();
        _r.indices();
        _r.elements();
    };

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_FILTER_
