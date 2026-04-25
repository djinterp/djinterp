/******************************************************************************
* djinterp [container]                                        lookup_table.hpp
*
* djinterp lookup table header:
*   A lookup table is any djinterp table (or container) combined with a
* key-extraction function that enables keyed access. No assumptions are made
* about the underlying table's dimensionality, storage model, iteration
* level, ordering, or key uniqueness — the table is treated as an opaque
* container per container_traits.
*
*   TWO GROUPS:
*   1. existence_table  — confirms whether a key is present.
*      Provides: contains(key), count(key), count_if(pred).
*
*   2. lookup_table     — confirms presence AND retrieves elements by key.
*      Provides: find(key), find_or(key, fallback), contains(key),
*                count(key), count_if(pred).
*
*   SEARCH STRATEGY:
*   The search algorithm is selected at compile time via a strategy type
*   parameter. The default `auto_strategy` inspects the underlying table's
*   container_traits to choose the best path:
*     - sorted + contiguous/random-access → binary search via data()
*     - iterable                          → linear scan via begin()/end()
*   Users may override by specifying `linear_strategy` or `binary_strategy`
*   explicitly.
*
*   KEY EXTRACTION:
*   The _KeyExtract callable maps an element to its key. Any stateless
*   functor, lambda, or function pointer is accepted. For identity lookup
*   (the element IS the key), use `identity_extract`.
*
*   CONTAINER_TRAITS CLASSIFICATION:
*   Both existence_table and lookup_table expose `backing_container_type`
*   (the wrapped table), which classifies them as backed containers. They
*   forward the wrapped table's iteration, capacity, and element-access
*   interface so that all twelve classification axes pass through.
*
*   USAGE:
*   ```cpp
*   // existence check in a flat set of ints
*   auto flags = make_existence_table(
*       table<int, 1, 5>{1, 3, 5, 7, 9},
*       identity_extract{});
*   flags.contains(3);  // true
*
*   // keyed lookup in a table of entries
*   struct entry { int id; const char* name; };
*   auto by_id = make_lookup_table(
*       my_entry_table,
*       [](const entry& e) { return e.id; });
*   const entry* p = by_id.find(42);
*   ```
*
* path:      /inc/djinterp/container/table/lookup/lookup_table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.22
******************************************************************************/

#ifndef DJINTERP_CONTAINER_LOOKUP_TABLE_
#define DJINTERP_CONTAINER_LOOKUP_TABLE_ 1

#include <cstddef>
#include <type_traits>
#include <utility>
#include "..\..\djinterp.h"


NS_DJINTERP
NS_CONTAINER


    // =========================================================================
    // I.   SEARCH STRATEGIES
    // =========================================================================

    // linear_strategy
    //   struct: forces linear scan regardless of table traits.
    struct linear_strategy
    {
    };

    // binary_strategy
    //   struct: forces binary search. Requires the table to expose data()
    // and the elements to be sorted by extracted key.
    struct binary_strategy
    {
    };

    // auto_strategy
    //   struct: selects binary search when the table exposes key_compare
    // (sorted invariant) and data() (contiguous), otherwise linear.
    struct auto_strategy
    {
    };


    // =========================================================================
    // II.  KEY EXTRACTORS
    // =========================================================================

    // identity_extract
    //   struct: key extractor that returns the element itself. Use when
    // the element IS the key (existence check in a flat set).
    struct identity_extract
    {
        template<typename _Element>
        D_CONSTEXPR const _Element& operator()(
                const _Element& _e
            ) const noexcept
        {
            return _e;
        }
    };


    // =========================================================================
    // III. INTERNAL SEARCH ALGORITHMS
    // =========================================================================

    NS_INTERNAL

        // linear_find
        //   function: scans [_first, _last) for an element whose extracted
        // key matches _target. Returns a pointer to the match or nullptr.
        template<typename _Iter,
                 typename _Key,
                 typename _Extract>
        D_CONSTEXPR const typename std::remove_reference<
            decltype(*std::declval<_Iter>())>::type*
        linear_find(
                _Iter          _first,
                _Iter          _last,
                const _Key&    _target,
                _Extract       _extract
            )
        {
            for (; _first != _last; ++_first)
            {
                if (_extract(*_first) == _target)
                {
                    return &(*_first);
                }
            }

            return nullptr;
        }

        // linear_contains
        //   function: scans [_first, _last) for key presence.
        template<typename _Iter,
                 typename _Key,
                 typename _Extract>
        D_CONSTEXPR bool
        linear_contains(
                _Iter          _first,
                _Iter          _last,
                const _Key&    _target,
                _Extract       _extract
            )
        {
            for (; _first != _last; ++_first)
            {
                if (_extract(*_first) == _target)
                {
                    return true;
                }
            }

            return false;
        }

        // linear_count
        //   function: counts elements in [_first, _last) whose extracted
        // key matches _target.
        template<typename _Iter,
                 typename _Key,
                 typename _Extract>
        D_CONSTEXPR std::size_t
        linear_count(
                _Iter          _first,
                _Iter          _last,
                const _Key&    _target,
                _Extract       _extract
            )
        {
            std::size_t n = 0;

            for (; _first != _last; ++_first)
            {
                if (_extract(*_first) == _target)
                {
                    ++n;
                }
            }

            return n;
        }

        // binary_lower_bound
        //   function: binary search over contiguous storage. Returns the
        // index of the first element whose extracted key is >= _target,
        // or _n if all keys are less.
        template<typename _Element,
                 typename _Key,
                 typename _Extract>
        D_CONSTEXPR std::size_t
        binary_lower_bound(
                const _Element* _data,
                std::size_t     _n,
                const _Key&     _target,
                _Extract        _extract
            )
        {
            std::size_t lo = 0;
            std::size_t hi = _n;

            while (lo < hi)
            {
                std::size_t mid = lo + (hi - lo) / 2;

                if (_extract(_data[mid]) < _target)
                {
                    lo = mid + 1;
                }
                else
                {
                    hi = mid;
                }
            }

            return lo;
        }

        // binary_find
        //   function: binary search returning a pointer to the matching
        // element, or nullptr if not found.
        template<typename _Element,
                 typename _Key,
                 typename _Extract>
        D_CONSTEXPR const _Element*
        binary_find(
                const _Element* _data,
                std::size_t     _n,
                const _Key&     _target,
                _Extract        _extract
            )
        {
            std::size_t idx = binary_lower_bound(_data,
                                                 _n,
                                                 _target,
                                                 _extract);

            if ( (idx < _n) &&
                 (_extract(_data[idx]) == _target) )
            {
                return &_data[idx];
            }

            return nullptr;
        }

        // binary_contains
        //   function: binary search for key presence.
        template<typename _Element,
                 typename _Key,
                 typename _Extract>
        D_CONSTEXPR bool
        binary_contains(
                const _Element* _data,
                std::size_t     _n,
                const _Key&     _target,
                _Extract        _extract
            )
        {
            return (binary_find(_data,
                                _n,
                                _target,
                                _extract) != nullptr);
        }


        // =================================================================
        //  strategy resolution
        // =================================================================

        // has_sorted_contiguous
        //   trait: true if _Table exposes key_compare (sorted) and data()
        // (contiguous storage).
        template<typename _Table,
                 typename = void>
        struct has_sorted_contiguous : std::false_type
        {
        };

        template<typename _Table>
        struct has_sorted_contiguous<_Table,
            void_t<typename _Table::key_compare,
                   decltype(std::declval<const _Table&>().data()),
                   decltype(std::declval<const _Table&>().size())>>
            : std::true_type
        {
        };

        // resolve_strategy
        //   trait: maps auto_strategy to concrete strategy based on table
        // traits. linear_strategy and binary_strategy pass through.
        template<typename _Strategy,
                 typename _Table>
        struct resolve_strategy
        {
            using type = _Strategy;
        };

        template<typename _Table>
        struct resolve_strategy<auto_strategy, _Table>
        {
            using type = typename std::conditional<
                has_sorted_contiguous<_Table>::value,
                binary_strategy,
                linear_strategy
            >::type;
        };

        template<typename _Strategy,
                 typename _Table>
        using resolve_strategy_t =
            typename resolve_strategy<_Strategy, _Table>::type;

    NS_END  // internal


    // =========================================================================
    // IV.  EXISTENCE TABLE
    // =========================================================================
    //
    // Group 1: confirms whether a key is present in the underlying table.
    // Does NOT provide element retrieval — only boolean membership queries.
    // Suitable for flag sets, permission checks, and any context where you
    // need "is X in the table?" without caring about associated data.
    //

    // existence_table
    //   class: wraps any djinterp container and adds key-presence queries
    // via a user-supplied key extractor. The underlying table is accessible
    // through the table() accessor; all container_traits pass through.
    template<typename _Table,
             typename _KeyExtract,
             typename _Strategy = auto_strategy>
    class existence_table
    {
    private:
        using resolved_strategy = internal::resolve_strategy_t<
            _Strategy, _Table>;
        using use_binary = std::is_same<resolved_strategy,
                                        binary_strategy>;

    public:
        // -----------------------------------------------------------------
        //  type aliases
        // -----------------------------------------------------------------
        using table_type             = _Table;
        using extract_type           = _KeyExtract;
        using strategy_type          = _Strategy;
        using backing_container_type = _Table;
        using size_type              = std::size_t;
        using key_type               = typename std::decay<decltype(
            std::declval<_KeyExtract>()(
                std::declval<const typename _Table::value_type&>()))>::type;

        // -----------------------------------------------------------------
        //  constructors
        // -----------------------------------------------------------------

        // existence_table(table, extract)
        //   constructor: takes ownership of the table and key extractor.
        D_CONSTEXPR explicit existence_table(
                _Table      _table,
                _KeyExtract _extract = _KeyExtract{}
            )
            : m_table(static_cast<_Table&&>(_table)),
              m_extract(static_cast<_KeyExtract&&>(_extract))
        {
        }

        D_CONSTEXPR existence_table()                                = default;
        D_CONSTEXPR existence_table(const existence_table&)          = default;
        D_CONSTEXPR existence_table(existence_table&&)               = default;
        D_CONSTEXPR existence_table& operator=(const existence_table&) = default;
        D_CONSTEXPR existence_table& operator=(existence_table&&)    = default;

        // -----------------------------------------------------------------
        //  table access
        // -----------------------------------------------------------------

        // table
        //   function: returns a const reference to the underlying table.
        D_CONSTEXPR const _Table& table() const noexcept
        {
            return m_table;
        }

        // table
        //   function: returns a mutable reference to the underlying table.
        D_CONSTEXPR _Table& table() noexcept
        {
            return m_table;
        }

        // extract
        //   function: returns the key extractor.
        D_CONSTEXPR const _KeyExtract& extract() const noexcept
        {
            return m_extract;
        }

        // -----------------------------------------------------------------
        //  existence queries
        // -----------------------------------------------------------------

        // contains
        //   function: returns true if any element's extracted key matches
        // _target.
        D_CONSTEXPR bool contains(
                const key_type& _target
            ) const
        {
            if constexpr (use_binary::value)
            {
                return internal::binary_contains(m_table.data(),
                                                 m_table.size(),
                                                 _target,
                                                 m_extract);
            }
            else
            {
                return internal::linear_contains(m_table.begin(),
                                                 m_table.end(),
                                                 _target,
                                                 m_extract);
            }
        }

        // count
        //   function: returns the number of elements whose extracted key
        // matches _target.
        D_CONSTEXPR size_type count(
                const key_type& _target
            ) const
        {
            return internal::linear_count(m_table.begin(),
                                          m_table.end(),
                                          _target,
                                          m_extract);
        }

        // count_if
        //   function: returns the number of elements for which
        // _pred(element) is true.
        template<typename _Pred>
        D_CONSTEXPR size_type count_if(
                _Pred _pred
            ) const
        {
            size_type n = 0;

            for (auto it = m_table.begin(); it != m_table.end(); ++it)
            {
                if (_pred(*it))
                {
                    ++n;
                }
            }

            return n;
        }

        // -----------------------------------------------------------------
        //  forwarded capacity
        // -----------------------------------------------------------------

        // size
        //   function: forwards to the underlying table's size().
        D_CONSTEXPR auto size() const
            -> decltype(std::declval<const _Table&>().size())
        {
            return m_table.size();
        }

        // empty
        //   function: forwards to the underlying table's empty().
        D_CONSTEXPR auto empty() const
            -> decltype(std::declval<const _Table&>().empty())
        {
            return m_table.empty();
        }

        // -----------------------------------------------------------------
        //  forwarded iteration
        // -----------------------------------------------------------------

        D_CONSTEXPR auto begin()  const -> decltype(std::declval<const _Table&>().begin())
        {
            return m_table.begin();
        }

        D_CONSTEXPR auto end()    const -> decltype(std::declval<const _Table&>().end())
        {
            return m_table.end();
        }

        D_CONSTEXPR auto cbegin() const -> decltype(std::declval<const _Table&>().cbegin())
        {
            return m_table.cbegin();
        }

        D_CONSTEXPR auto cend()   const -> decltype(std::declval<const _Table&>().cend())
        {
            return m_table.cend();
        }

    protected:
        _Table      m_table;
        _KeyExtract m_extract;
    };


    // =========================================================================
    // V.   LOOKUP TABLE
    // =========================================================================
    //
    // Group 2: confirms presence AND retrieves elements by key. Provides
    // everything existence_table does plus find() and find_or(). Suitable
    // for option registries, symbol tables, and any key → value mapping
    // over a djinterp container.
    //

    // lookup_table
    //   class: wraps any djinterp container and adds keyed element retrieval
    // via a user-supplied key extractor. The underlying table is accessible
    // through the table() accessor; all container_traits pass through.
    template<typename _Table,
             typename _KeyExtract,
             typename _Strategy = auto_strategy>
    class lookup_table
    {
    private:
        using resolved_strategy = internal::resolve_strategy_t<
            _Strategy, _Table>;
        using use_binary = std::is_same<resolved_strategy,
                                        binary_strategy>;

    public:
        // -----------------------------------------------------------------
        //  type aliases
        // -----------------------------------------------------------------
        using table_type             = _Table;
        using extract_type           = _KeyExtract;
        using strategy_type          = _Strategy;
        using backing_container_type = _Table;
        using element_type           = typename _Table::value_type;
        using value_type             = typename _Table::value_type;
        using size_type              = std::size_t;
        using key_type               = typename std::decay<decltype(
            std::declval<_KeyExtract>()(
                std::declval<const element_type&>()))>::type;

        // -----------------------------------------------------------------
        //  constructors
        // -----------------------------------------------------------------

        // lookup_table(table, extract)
        //   constructor: takes ownership of the table and key extractor.
        D_CONSTEXPR explicit lookup_table(
                _Table      _table,
                _KeyExtract _extract = _KeyExtract{}
            )
            : m_table(static_cast<_Table&&>(_table)),
              m_extract(static_cast<_KeyExtract&&>(_extract))
        {
        }

        D_CONSTEXPR lookup_table()                             = default;
        D_CONSTEXPR lookup_table(const lookup_table&)          = default;
        D_CONSTEXPR lookup_table(lookup_table&&)               = default;
        D_CONSTEXPR lookup_table& operator=(const lookup_table&) = default;
        D_CONSTEXPR lookup_table& operator=(lookup_table&&)    = default;

        // -----------------------------------------------------------------
        //  table access
        // -----------------------------------------------------------------

        // table
        //   function: returns a const reference to the underlying table.
        D_CONSTEXPR const _Table& table() const noexcept
        {
            return m_table;
        }

        // table
        //   function: returns a mutable reference to the underlying table.
        D_CONSTEXPR _Table& table() noexcept
        {
            return m_table;
        }

        // extract
        //   function: returns the key extractor.
        D_CONSTEXPR const _KeyExtract& extract() const noexcept
        {
            return m_extract;
        }

        // -----------------------------------------------------------------
        //  lookup (element retrieval)
        // -----------------------------------------------------------------

        // find
        //   function: returns a pointer to the first element whose
        // extracted key matches _target, or nullptr if not found.
        D_CONSTEXPR const element_type* find(
                const key_type& _target
            ) const
        {
            if constexpr (use_binary::value)
            {
                return internal::binary_find(m_table.data(),
                                             m_table.size(),
                                             _target,
                                             m_extract);
            }
            else
            {
                return internal::linear_find(m_table.begin(),
                                             m_table.end(),
                                             _target,
                                             m_extract);
            }
        }

        // find (mutable)
        //   function: returns a mutable pointer to the matching element.
        D_CONSTEXPR element_type* find(
                const key_type& _target
            )
        {
            // delegate to const version and cast
            return const_cast<element_type*>(
                static_cast<const lookup_table*>(this)->find(_target));
        }

        // find_or
        //   function: returns a reference to the matching element, or
        // _fallback if no match exists.
        D_CONSTEXPR const element_type& find_or(
                const key_type&    _target,
                const element_type& _fallback
            ) const
        {
            const element_type* p = find(_target);

            if (p)
            {
                return *p;
            }

            return _fallback;
        }

        // -----------------------------------------------------------------
        //  existence queries
        // -----------------------------------------------------------------

        // contains
        //   function: returns true if any element's extracted key matches
        // _target.
        D_CONSTEXPR bool contains(
                const key_type& _target
            ) const
        {
            return (find(_target) != nullptr);
        }

        // count
        //   function: returns the number of elements whose extracted key
        // matches _target.
        D_CONSTEXPR size_type count(
                const key_type& _target
            ) const
        {
            return internal::linear_count(m_table.begin(),
                                          m_table.end(),
                                          _target,
                                          m_extract);
        }

        // count_if
        //   function: returns the number of elements for which
        // _pred(element) is true.
        template<typename _Pred>
        D_CONSTEXPR size_type count_if(
                _Pred _pred
            ) const
        {
            size_type n = 0;

            for (auto it = m_table.begin(); it != m_table.end(); ++it)
            {
                if (_pred(*it))
                {
                    ++n;
                }
            }

            return n;
        }

        // -----------------------------------------------------------------
        //  forwarded capacity
        // -----------------------------------------------------------------

        D_CONSTEXPR auto size() const
            -> decltype(std::declval<const _Table&>().size())
        {
            return m_table.size();
        }

        D_CONSTEXPR auto empty() const
            -> decltype(std::declval<const _Table&>().empty())
        {
            return m_table.empty();
        }

        // -----------------------------------------------------------------
        //  forwarded iteration
        // -----------------------------------------------------------------

        D_CONSTEXPR auto begin()  const -> decltype(std::declval<const _Table&>().begin())
        {
            return m_table.begin();
        }

        D_CONSTEXPR auto end()    const -> decltype(std::declval<const _Table&>().end())
        {
            return m_table.end();
        }

        D_CONSTEXPR auto cbegin() const -> decltype(std::declval<const _Table&>().cbegin())
        {
            return m_table.cbegin();
        }

        D_CONSTEXPR auto cend()   const -> decltype(std::declval<const _Table&>().cend())
        {
            return m_table.cend();
        }

        // -----------------------------------------------------------------
        //  forwarded contiguous access (when available)
        // -----------------------------------------------------------------

        // data
        //   function: forwards to the underlying table's data() when
        // the table provides contiguous storage.
        D_CONSTEXPR auto data() const
            -> decltype(std::declval<const _Table&>().data())
        {
            return m_table.data();
        }

        D_CONSTEXPR auto data()
            -> decltype(std::declval<_Table&>().data())
        {
            return m_table.data();
        }

    protected:
        _Table      m_table;
        _KeyExtract m_extract;
    };


    // =========================================================================
    // VI.  FACTORY FUNCTIONS
    // =========================================================================

    // make_existence_table
    //   function: constructs an existence_table by moving the given table
    // and key extractor into place.
    template<typename _Strategy  = auto_strategy,
             typename _Table,
             typename _KeyExtract>
    D_CONSTEXPR auto
    make_existence_table(
            _Table      _table,
            _KeyExtract _extract
        )
        -> existence_table<typename std::decay<_Table>::type,
                           typename std::decay<_KeyExtract>::type,
                           _Strategy>
    {
        using table_t   = typename std::decay<_Table>::type;
        using extract_t = typename std::decay<_KeyExtract>::type;

        return existence_table<table_t, extract_t, _Strategy>{
            static_cast<_Table&&>(_table),
            static_cast<_KeyExtract&&>(_extract)
        };
    }

    // make_lookup_table
    //   function: constructs a lookup_table by moving the given table
    // and key extractor into place.
    template<typename _Strategy  = auto_strategy,
             typename _Table,
             typename _KeyExtract>
    D_CONSTEXPR auto
    make_lookup_table(
            _Table      _table,
            _KeyExtract _extract
        )
        -> lookup_table<typename std::decay<_Table>::type,
                        typename std::decay<_KeyExtract>::type,
                        _Strategy>
    {
        using table_t   = typename std::decay<_Table>::type;
        using extract_t = typename std::decay<_KeyExtract>::type;

        return lookup_table<table_t, extract_t, _Strategy>{
            static_cast<_Table&&>(_table),
            static_cast<_KeyExtract&&>(_extract)
        };
    }


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_LOOKUP_TABLE_
