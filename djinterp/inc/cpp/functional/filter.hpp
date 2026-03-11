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
* USAGE:
*   // fluent builder
*   auto result = d_filter<int>::build()
*       .skip_first(5)
*       .where([](int x) { return x > 0; })
*       .take_first(10)
*       .distinct()
*       .apply(my_data);
*
*   // combinator
*   auto combined = filter_union(
*       d_filter<int>::build().where(is_positive).build_chain(),
*       d_filter<int>::build().where(is_even).build_chain());
*   auto result = combined.apply(my_data);
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
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include "./djinterp.h"
#include "./env.h"
#include "./cpp_features.h"
#include "./functional.hpp"
#include "./functional_traits.hpp"


NS_DJINTERP
NS_FUNCTIONAL


///////////////////////////////////////////////////////////////////////////////
///             I.    FILTER RESULT                                         ///
///////////////////////////////////////////////////////////////////////////////

// d_filter_result_status
//   enum: status of a filter operation.
enum class d_filter_result_status
{
    success    =  0,
    empty      =  1,
    error      = -1,
    invalid    = -2,
    no_memory  = -3
};

// d_filter_result
//   class: result of applying a filter operation.
template<typename _Type>
class d_filter_result
{
private:
    std::vector<_Type>        m_elements;
    std::vector<std::size_t>  m_indices;
    d_filter_result_status    m_status;
    std::string               m_error_message;

public:
    // success constructor
    d_filter_result(std::vector<_Type>&&       _elements,
                    std::vector<std::size_t>&& _indices)
        : m_elements(std::move(_elements))
        , m_indices(std::move(_indices))
        , m_status(m_elements.empty()
                   ? d_filter_result_status::empty
                   : d_filter_result_status::success)
    {}

    // error constructor
    explicit d_filter_result(d_filter_result_status _status,
                             std::string            _msg = "")
        : m_status(_status)
        , m_error_message(std::move(_msg))
    {}

    D_FUNCTIONAL_NODISCARD bool ok()    const { return m_status == d_filter_result_status::success; }
    D_FUNCTIONAL_NODISCARD bool empty() const { return m_elements.empty(); }

    D_FUNCTIONAL_NODISCARD d_filter_result_status    status()        const { return m_status; }
    D_FUNCTIONAL_NODISCARD const std::string&        error_message() const { return m_error_message; }
    D_FUNCTIONAL_NODISCARD std::size_t               count()         const { return m_elements.size(); }
    D_FUNCTIONAL_NODISCARD const std::vector<_Type>& elements()      const { return m_elements; }
    D_FUNCTIONAL_NODISCARD std::vector<_Type>        take_elements()       { return std::move(m_elements); }

    D_FUNCTIONAL_NODISCARD
    const std::vector<std::size_t>& indices() const { return m_indices; }

    typename std::vector<_Type>::const_iterator begin() const
    { return m_elements.begin(); }

    typename std::vector<_Type>::const_iterator end() const
    { return m_elements.end(); }
};


///////////////////////////////////////////////////////////////////////////////
///             II.   FILTER OPERATION (TYPE-ERASED)                        ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // filter_operation_base
    //   helper: abstract base for type-erased filter operations.
    template<typename _Type>
    class filter_operation_base
    {
    public:
        virtual ~filter_operation_base() = default;

        virtual std::vector<std::size_t>
        get_indices(const std::vector<_Type>& _input) const = 0;

        virtual std::unique_ptr<filter_operation_base> clone() const = 0;
    };

    // take_first_op
    template<typename _Type>
    class take_first_op : public filter_operation_base<_Type>
    {
        std::size_t m_n;
    public:
        explicit take_first_op(std::size_t _n) : m_n(_n) {}

        std::vector<std::size_t>
        get_indices(const std::vector<_Type>& _input) const override
        {
            std::vector<std::size_t> result;
            std::size_t limit = (m_n < _input.size())
                              ? m_n : _input.size();

            for (std::size_t i = 0; i < limit; ++i)
            {
                result.push_back(i);
            }

            return result;
        }

        std::unique_ptr<filter_operation_base<_Type>> clone() const override
        {
            return std::unique_ptr<filter_operation_base<_Type>>(
                new take_first_op(m_n));
        }
    };

    // take_last_op
    template<typename _Type>
    class take_last_op : public filter_operation_base<_Type>
    {
        std::size_t m_n;
    public:
        explicit take_last_op(std::size_t _n) : m_n(_n) {}

        std::vector<std::size_t>
        get_indices(const std::vector<_Type>& _input) const override
        {
            std::vector<std::size_t> result;
            std::size_t start = (m_n >= _input.size())
                              ? 0 : _input.size() - m_n;

            for (std::size_t i = start; i < _input.size(); ++i)
            {
                result.push_back(i);
            }

            return result;
        }

        std::unique_ptr<filter_operation_base<_Type>> clone() const override
        {
            return std::unique_ptr<filter_operation_base<_Type>>(
                new take_last_op(m_n));
        }
    };

    // skip_first_op
    template<typename _Type>
    class skip_first_op : public filter_operation_base<_Type>
    {
        std::size_t m_n;
    public:
        explicit skip_first_op(std::size_t _n) : m_n(_n) {}

        std::vector<std::size_t>
        get_indices(const std::vector<_Type>& _input) const override
        {
            std::vector<std::size_t> result;
            std::size_t start = (m_n < _input.size())
                              ? m_n : _input.size();

            for (std::size_t i = start; i < _input.size(); ++i)
            {
                result.push_back(i);
            }

            return result;
        }

        std::unique_ptr<filter_operation_base<_Type>> clone() const override
        {
            return std::unique_ptr<filter_operation_base<_Type>>(
                new skip_first_op(m_n));
        }
    };

    // skip_last_op
    template<typename _Type>
    class skip_last_op : public filter_operation_base<_Type>
    {
        std::size_t m_n;
    public:
        explicit skip_last_op(std::size_t _n) : m_n(_n) {}

        std::vector<std::size_t>
        get_indices(const std::vector<_Type>& _input) const override
        {
            std::vector<std::size_t> result;
            std::size_t limit = (m_n >= _input.size())
                              ? 0 : _input.size() - m_n;

            for (std::size_t i = 0; i < limit; ++i)
            {
                result.push_back(i);
            }

            return result;
        }

        std::unique_ptr<filter_operation_base<_Type>> clone() const override
        {
            return std::unique_ptr<filter_operation_base<_Type>>(
                new skip_last_op(m_n));
        }
    };

    // take_nth_op
    template<typename _Type>
    class take_nth_op : public filter_operation_base<_Type>
    {
        std::size_t m_n;
    public:
        explicit take_nth_op(std::size_t _n) : m_n(_n) {}

        std::vector<std::size_t>
        get_indices(const std::vector<_Type>& _input) const override
        {
            std::vector<std::size_t> result;

            if (m_n == 0) { return result; }

            for (std::size_t i = 0; i < _input.size(); i += m_n)
            {
                result.push_back(i);
            }

            return result;
        }

        std::unique_ptr<filter_operation_base<_Type>> clone() const override
        {
            return std::unique_ptr<filter_operation_base<_Type>>(
                new take_nth_op(m_n));
        }
    };

    // range_op
    template<typename _Type>
    class range_op : public filter_operation_base<_Type>
    {
        std::size_t m_start;
        std::size_t m_end;
    public:
        range_op(std::size_t _start, std::size_t _end)
            : m_start(_start), m_end(_end) {}

        std::vector<std::size_t>
        get_indices(const std::vector<_Type>& _input) const override
        {
            std::vector<std::size_t> result;
            std::size_t limit = (m_end < _input.size())
                              ? m_end : _input.size();

            for (std::size_t i = m_start; i < limit; ++i)
            {
                result.push_back(i);
            }

            return result;
        }

        std::unique_ptr<filter_operation_base<_Type>> clone() const override
        {
            return std::unique_ptr<filter_operation_base<_Type>>(
                new range_op(m_start, m_end));
        }
    };

    // slice_op
    template<typename _Type>
    class slice_op : public filter_operation_base<_Type>
    {
        std::size_t m_start;
        std::size_t m_end;
        std::size_t m_step;
    public:
        slice_op(std::size_t _start, std::size_t _end, std::size_t _step)
            : m_start(_start), m_end(_end), m_step(_step) {}

        std::vector<std::size_t>
        get_indices(const std::vector<_Type>& _input) const override
        {
            std::vector<std::size_t> result;

            if (m_step == 0) { return result; }

            std::size_t limit = (m_end < _input.size())
                              ? m_end : _input.size();

            for (std::size_t i = m_start; i < limit; i += m_step)
            {
                result.push_back(i);
            }

            return result;
        }

        std::unique_ptr<filter_operation_base<_Type>> clone() const override
        {
            return std::unique_ptr<filter_operation_base<_Type>>(
                new slice_op(m_start, m_end, m_step));
        }
    };

    // where_op
    template<typename _Type>
    class where_op : public filter_operation_base<_Type>
    {
        std::function<bool(const _Type&)> m_pred;
    public:
        explicit where_op(std::function<bool(const _Type&)> _pred)
            : m_pred(std::move(_pred)) {}

        std::vector<std::size_t>
        get_indices(const std::vector<_Type>& _input) const override
        {
            std::vector<std::size_t> result;

            for (std::size_t i = 0; i < _input.size(); ++i)
            {
                if (m_pred(_input[i]))
                {
                    result.push_back(i);
                }
            }

            return result;
        }

        std::unique_ptr<filter_operation_base<_Type>> clone() const override
        {
            return std::unique_ptr<filter_operation_base<_Type>>(
                new where_op(m_pred));
        }
    };

    // where_not_op
    template<typename _Type>
    class where_not_op : public filter_operation_base<_Type>
    {
        std::function<bool(const _Type&)> m_pred;
    public:
        explicit where_not_op(std::function<bool(const _Type&)> _pred)
            : m_pred(std::move(_pred)) {}

        std::vector<std::size_t>
        get_indices(const std::vector<_Type>& _input) const override
        {
            std::vector<std::size_t> result;

            for (std::size_t i = 0; i < _input.size(); ++i)
            {
                if (!m_pred(_input[i]))
                {
                    result.push_back(i);
                }
            }

            return result;
        }

        std::unique_ptr<filter_operation_base<_Type>> clone() const override
        {
            return std::unique_ptr<filter_operation_base<_Type>>(
                new where_not_op(m_pred));
        }
    };

    // indices_op
    template<typename _Type>
    class indices_op : public filter_operation_base<_Type>
    {
        std::vector<std::size_t> m_indices;
    public:
        explicit indices_op(std::vector<std::size_t> _indices)
            : m_indices(std::move(_indices)) {}

        std::vector<std::size_t>
        get_indices(const std::vector<_Type>& _input) const override
        {
            std::vector<std::size_t> result;

            for (auto idx : m_indices)
            {
                if (idx < _input.size())
                {
                    result.push_back(idx);
                }
            }

            return result;
        }

        std::unique_ptr<filter_operation_base<_Type>> clone() const override
        {
            return std::unique_ptr<filter_operation_base<_Type>>(
                new indices_op(m_indices));
        }
    };

    // distinct_op
    template<typename _Type>
    class distinct_op : public filter_operation_base<_Type>
    {
        std::function<bool(const _Type&, const _Type&)> m_eq;
    public:
        explicit distinct_op(
            std::function<bool(const _Type&, const _Type&)> _eq)
            : m_eq(std::move(_eq)) {}

        std::vector<std::size_t>
        get_indices(const std::vector<_Type>& _input) const override
        {
            std::vector<std::size_t> result;
            std::vector<std::size_t> seen;

            for (std::size_t i = 0; i < _input.size(); ++i)
            {
                bool is_dup = false;

                for (auto j : seen)
                {
                    if (m_eq(_input[i], _input[j]))
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
        }

        std::unique_ptr<filter_operation_base<_Type>> clone() const override
        {
            return std::unique_ptr<filter_operation_base<_Type>>(
                new distinct_op(m_eq));
        }
    };

    // reverse_op
    template<typename _Type>
    class reverse_op : public filter_operation_base<_Type>
    {
    public:
        std::vector<std::size_t>
        get_indices(const std::vector<_Type>& _input) const override
        {
            std::vector<std::size_t> result;

            for (std::size_t i = _input.size(); i > 0; --i)
            {
                result.push_back(i - 1);
            }

            return result;
        }

        std::unique_ptr<filter_operation_base<_Type>> clone() const override
        {
            return std::unique_ptr<filter_operation_base<_Type>>(
                new reverse_op());
        }
    };

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///             III.  FILTER CHAIN                                          ///
///////////////////////////////////////////////////////////////////////////////

// d_filter_chain
//   class: chain of sequential filter operations.
template<typename _Type>
class d_filter_chain
{
private:
    using op_ptr = std::unique_ptr<internal::filter_operation_base<_Type>>;

    std::vector<op_ptr> m_operations;

public:
    d_filter_chain() = default;

    // move constructor and assignment
    d_filter_chain(d_filter_chain&&) = default;
    d_filter_chain& operator=(d_filter_chain&&) = default;

    // copy via clone
    d_filter_chain(const d_filter_chain& _other)
    {
        m_operations.reserve(_other.m_operations.size());

        for (const auto& op : _other.m_operations)
        {
            m_operations.push_back(op->clone());
        }
    }

    // add
    //   method: adds an operation to the chain.
    void add(op_ptr&& _op)
    {
        m_operations.push_back(std::move(_op));

        return;
    }

    // apply
    //   method: applies the chain to input data and returns a filter result.
    D_FUNCTIONAL_NODISCARD
    d_filter_result<_Type>
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
            auto relative_indices = op->get_indices(sub);

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

        return d_filter_result<_Type>(std::move(result_elements),
                                      std::move(current_indices));
    }

    // length
    D_FUNCTIONAL_NODISCARD
    std::size_t length() const { return m_operations.size(); }

    D_FUNCTIONAL_NODISCARD
    bool is_empty() const { return m_operations.empty(); }

    void clear() { m_operations.clear(); return; }
};


///////////////////////////////////////////////////////////////////////////////
///             IV.   FILTER COMBINATORS                                    ///
///////////////////////////////////////////////////////////////////////////////

// filter_union
//   function: applies union semantics (OR) over multiple filter chains.
// An element is included if it passes any of the chains.
template<typename _Type>
D_FUNCTIONAL_NODISCARD
d_filter_result<_Type>
filter_union(const std::vector<d_filter_chain<_Type>>& _chains,
             const std::vector<_Type>&                  _input)
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

    return d_filter_result<_Type>(std::move(elements),
                                  std::move(indices));
}

// filter_intersection
//   function: applies intersection semantics (AND) over multiple chains.
// An element is included only if it passes all chains.
template<typename _Type>
D_FUNCTIONAL_NODISCARD
d_filter_result<_Type>
filter_intersection(const std::vector<d_filter_chain<_Type>>& _chains,
                    const std::vector<_Type>&                  _input)
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

    return d_filter_result<_Type>(std::move(elements),
                                  std::move(indices));
}

// filter_difference
//   function: applies difference semantics (A - B).
// An element is included if it passes _include but not _exclude.
template<typename _Type>
D_FUNCTIONAL_NODISCARD
d_filter_result<_Type>
filter_difference(const d_filter_chain<_Type>& _include,
                  const d_filter_chain<_Type>& _exclude,
                  const std::vector<_Type>&    _input)
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

    return d_filter_result<_Type>(std::move(elements),
                                  std::move(indices));
}


///////////////////////////////////////////////////////////////////////////////
///             V.    FILTER ITERATOR                                       ///
///////////////////////////////////////////////////////////////////////////////

// d_filter_iterator
//   class: lazily iterates over filtered results.
template<typename _Type>
class d_filter_iterator
{
private:
    const std::vector<_Type>* m_input;
    std::vector<std::size_t>  m_indices;
    std::size_t               m_pos;

public:
    d_filter_iterator(const std::vector<_Type>& _input,
                      const d_filter_chain<_Type>&    _chain)
        : m_input(&_input)
        , m_pos(0)
    {
        auto result = _chain.apply(_input);

        m_indices = result.indices();
    }

    D_FUNCTIONAL_NODISCARD
    bool has_next() const { return m_pos < m_indices.size(); }

    D_FUNCTIONAL_NODISCARD
    const _Type& next()
    {
        return (*m_input)[m_indices[m_pos++]];
    }

    void reset() { m_pos = 0; return; }

    D_FUNCTIONAL_NODISCARD
    std::size_t remaining() const
    {
        return m_indices.size() - m_pos;
    }
};


///////////////////////////////////////////////////////////////////////////////
///             VI.   FLUENT FILTER BUILDER                                 ///
///////////////////////////////////////////////////////////////////////////////

// d_filter
//   class: fluent builder for constructing filter chains.
template<typename _Type>
class d_filter
{
private:
    d_filter_chain<_Type> m_chain;

public:
    d_filter() = default;

    // build
    //   static: creates a new fluent filter builder.
    static d_filter build() { return d_filter(); }

    // take_first
    d_filter& take_first(std::size_t _n)
    {
        m_chain.add(std::unique_ptr<internal::filter_operation_base<_Type>>(
            new internal::take_first_op<_Type>(_n)));

        return *this;
    }

    // take_last
    d_filter& take_last(std::size_t _n)
    {
        m_chain.add(std::unique_ptr<internal::filter_operation_base<_Type>>(
            new internal::take_last_op<_Type>(_n)));

        return *this;
    }

    // take_nth
    d_filter& take_nth(std::size_t _n)
    {
        m_chain.add(std::unique_ptr<internal::filter_operation_base<_Type>>(
            new internal::take_nth_op<_Type>(_n)));

        return *this;
    }

    // skip_first
    d_filter& skip_first(std::size_t _n)
    {
        m_chain.add(std::unique_ptr<internal::filter_operation_base<_Type>>(
            new internal::skip_first_op<_Type>(_n)));

        return *this;
    }

    // skip_last
    d_filter& skip_last(std::size_t _n)
    {
        m_chain.add(std::unique_ptr<internal::filter_operation_base<_Type>>(
            new internal::skip_last_op<_Type>(_n)));

        return *this;
    }

    // head (take first 1)
    d_filter& head() { return take_first(1); }

    // tail (take last 1)
    d_filter& tail() { return take_last(1); }

    // init (all except last)
    d_filter& init() { return skip_last(1); }

    // rest (all except first)
    d_filter& rest() { return skip_first(1); }

    // range [start, end)
    d_filter& range(std::size_t _start, std::size_t _end)
    {
        m_chain.add(std::unique_ptr<internal::filter_operation_base<_Type>>(
            new internal::range_op<_Type>(_start, _end)));

        return *this;
    }

    // slice [start:end:step]
    d_filter& slice(std::size_t _start,
                    std::size_t _end,
                    std::size_t _step)
    {
        m_chain.add(std::unique_ptr<internal::filter_operation_base<_Type>>(
            new internal::slice_op<_Type>(_start, _end, _step)));

        return *this;
    }

    // where (predicate filter)
    template<typename _Pred,
             typename = typename std::enable_if<
                 is_predicate<_Pred, const _Type&>::value
             >::type>
    d_filter& where(_Pred _pred)
    {
        m_chain.add(std::unique_ptr<internal::filter_operation_base<_Type>>(
            new internal::where_op<_Type>(
                std::function<bool(const _Type&)>(std::move(_pred)))));

        return *this;
    }

    // where_not (negated predicate)
    template<typename _Pred,
             typename = typename std::enable_if<
                 is_predicate<_Pred, const _Type&>::value
             >::type>
    d_filter& where_not(_Pred _pred)
    {
        m_chain.add(std::unique_ptr<internal::filter_operation_base<_Type>>(
            new internal::where_not_op<_Type>(
                std::function<bool(const _Type&)>(std::move(_pred)))));

        return *this;
    }

    // at (single index)
    d_filter& at(std::size_t _index)
    {
        m_chain.add(std::unique_ptr<internal::filter_operation_base<_Type>>(
            new internal::indices_op<_Type>(
                std::vector<std::size_t>{_index})));

        return *this;
    }

    // at_indices (multiple indices)
    d_filter& at_indices(std::vector<std::size_t> _indices)
    {
        m_chain.add(std::unique_ptr<internal::filter_operation_base<_Type>>(
            new internal::indices_op<_Type>(std::move(_indices))));

        return *this;
    }

    // distinct (custom equality)
    template<typename _Eq,
             typename = typename std::enable_if<
                 is_callable<_Eq, const _Type&, const _Type&>::value
             >::type>
    d_filter& distinct(_Eq _eq)
    {
        m_chain.add(std::unique_ptr<internal::filter_operation_base<_Type>>(
            new internal::distinct_op<_Type>(
                std::function<bool(const _Type&, const _Type&)>(
                    std::move(_eq)))));

        return *this;
    }

    // distinct (default operator==)
    d_filter& distinct()
    {
        return distinct([](const _Type& _a, const _Type& _b)
        {
            return _a == _b;
        });
    }

    // reverse
    d_filter& reverse()
    {
        m_chain.add(std::unique_ptr<internal::filter_operation_base<_Type>>(
            new internal::reverse_op<_Type>()));

        return *this;
    }

    // apply (execute the chain)
    D_FUNCTIONAL_NODISCARD
    d_filter_result<_Type>
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
    D_FUNCTIONAL_NODISCARD
    d_filter_result<_Type>
    apply(const _Container& _input) const
    {
        std::vector<_Type> vec(std::begin(_input), std::end(_input));

        return m_chain.apply(vec);
    }

    // build_chain (extract the chain for use in combinators)
    D_FUNCTIONAL_NODISCARD
    d_filter_chain<_Type> build_chain() const
    {
        return m_chain;
    }

    // build_chain (move)
    D_FUNCTIONAL_NODISCARD
    d_filter_chain<_Type> build_chain() &&
    {
        return std::move(m_chain);
    }

    // iterator
    D_FUNCTIONAL_NODISCARD
    d_filter_iterator<_Type>
    iterator(const std::vector<_Type>& _input) const
    {
        return d_filter_iterator<_Type>(_input, m_chain);
    }

    // any_match
    D_FUNCTIONAL_NODISCARD
    bool any_match(const std::vector<_Type>& _input) const
    {
        return !m_chain.apply(_input).empty();
    }

    // all_match
    D_FUNCTIONAL_NODISCARD
    bool all_match(const std::vector<_Type>& _input) const
    {
        return m_chain.apply(_input).count() == _input.size();
    }

    // none_match
    D_FUNCTIONAL_NODISCARD
    bool none_match(const std::vector<_Type>& _input) const
    {
        return m_chain.apply(_input).empty();
    }

    // count_matches
    D_FUNCTIONAL_NODISCARD
    std::size_t count_matches(const std::vector<_Type>& _input) const
    {
        return m_chain.apply(_input).count();
    }
};


NS_END  // functional
NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_FILTER_HPP_
