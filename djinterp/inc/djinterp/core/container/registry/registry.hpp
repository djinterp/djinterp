/**
 * @file registry.hpp
 * @brief A fully customizable, compile-time configurable registry system.
 *
 * The registry provides a flexible framework for managing cvars (configuration
 * variables) and commands with customizable storage, parsing, serialization,
 * and validation strategies.
 *
 * Key design principles:
 * - 100% customizable input/output types
 * - Storage can be anything: arrays, maps, flat tables, etc.
 * - Supports 0 to N cvars and 0 to N commands
 * - Works with text, binary, or any custom format
 * - All constraints verified at compile-time via SFINAE
 */

#ifndef DJINTERP_REGISTRY_
#define DJINTERP_REGISTRY_

#include <cstddef>
#include <type_traits>
#include <utility>
#include "../../djinterp.hpp"


NS_DJINTERP
NS_CONTAINER


NS_INTERNAL
    template<typename _Derived>
    struct registry_base
    {

    };
NS_END  // internal


// Tag: "use column N of the tuple as the key"
template<std::size_t N>
struct column
{};

// Primary template (unchanged)
template<typename _Key,
         typename _Value,
         typename _Compare   = std::less<_Key>,
         typename _Allocator = std::allocator<_Key>>
class registry
{
    // simple key -> value storage
};

// Partial specialization: rows are tuples, keyed by column N
template<std::size_t _N,
         typename... _Columns,
         typename    _Compare,
         typename    _Allocator>
class registry<column<_N>,
               std::tuple<_Columns...>,
               _Compare,
               _Allocator>
{
public:
    using row_type = std::tuple<_Columns...>;
    using key_type = std::tuple_element_t<_N, row_type>;

    static_assert(_N < sizeof...(_Columns), "column index out of range");

private:
    // Extracts the key from a row
    struct key_extract 
    {
        const key_type& operator()(const row_type& row) const
        {
            return std::get<N>(row);
        }
    };

    // Comparator wraps the user-supplied _Compare (defaulted to std::less<key_type>)
    struct row_compare 
    {
        _Compare comp;
        bool operator()(const row_type& a, const row_type& b) const 
        {
            return comp(std::get<N>(a), std::get<N>(b));
        }
    };

    std::set<row_type, row_compare> _rows;

public:
    void insert(const row_type& row)
    {
        _rows.insert(row); 
    }

    auto find(const key_type& key) const {
        // build a dummy row with the key in the right slot to leverage set::find
        // — or use a transparent comparator instead:
        for (auto it = _rows.begin(); it != _rows.end(); ++it)
        {
            if (std::get<N>(*it) == key)
            {
                return it;
            }
        }

        return _rows.end();
    }
};

NS_END  // namespace djinterp


#endif // DJINTERP_REGISTRY_