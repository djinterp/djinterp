/******************************************************************************
* djinterp [container]                                              table.hpp
*
*   The fundamental, owning, single-rank djinterp table.  Built directly on
* `table_base` from table_common.hpp - the option pack drives every axis,
* the storage is concrete, and the canonical alias surface is published
* automatically so the twelve-axis trait machinery classifies instances
* without further wiring.
*
*   For multi-dimensional / nested forms see `table_overlay.hpp` (formerly
* this file's contents, now living under its kind-accurate name).  For
* runtime-dimension database-backed tables see `database_table.hpp`.  Both
* share `table_common.hpp` with this file.
*
*   STORAGE MODEL:
*   The default storage is std::vector<_Type, _Allocator> - dynamic, flat,
* unbounded, multi.  The `underlying_key` option allows the user to
* substitute any container template with a one-arg shape:
*
*     table<int, std::allocator<int>,
*           underlying_key, std::deque<int>>           // deque-backed
*     table<my_record, my_alloc,
*           comparator_key, std::less<my_record>,
*           multiplicity_key, unique_tag>              // sorted set semantics
*           // (operations gated; backing remains a vector kept sorted)
*
*   Operations (push_back, insert, erase, find, ...) are gated on the
* resolved axis tags via `if constexpr` and SFINAE.  Calls that conflict
* with the table's classification (e.g. push_back on a sorted-unique
* table) become hard errors at the call site.
*
*   TWELVE-AXIS CLASSIFICATION:
*   Out of the box, `table<int>` classifies as:
*     axis 1 lifetime       mutable_storage
*     axis 2 iteration      random_access (std::vector iterators)
*     axis 3 ordering       unordered (no comparator_key surfaced)
*     axis 4 bounds         unbounded
*     axis 5 multiplicity   multi
*     axis 6 structure      flat
*     axis 7 storage        dynamic
*     axis 8 thread safety  none
*     axis 9 underlying     fundamental (no underlying_key surfaced)
*    axis 10 binary         unsupported (no encode/decode)
*    axis 11 database       unsupported
*    axis 12 text           unsupported
*
*   Every off-default position is opted into by adding the appropriate
* option to the pack.  See containers_howto.md for the full key/value
* matrix.
*
* DEPENDENCIES:
*   djinterp.hpp           - NS_DJINTERP, D_CONSTEXPR
*   table_common.hpp       - table_base, axis option keys, default tags
*
* TABLE OF CONTENTS
* =================
* I.    Internal helpers
*       1. select_storage  (choose underlying via underlying_key)
* II.   table class
* III.  Detection trait (is_fundamental_table)
* IV.   Classification (fundamental_table_class)
*
*
* path:      /inc/djinterp/container/table/table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_TABLE_
#define DJINTERP_TABLE_ 1

#include <cstddef>
#include <initializer_list>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
#include "../../djinterp.hpp"
#include "./table_common.hpp"


NS_DJINTERP


    // =========================================================================
    // I.   INTERNAL HELPERS
    // =========================================================================

    NS_INTERNAL

        // -----------------------------------------------------------------
        //  1. select_storage_for
        // -----------------------------------------------------------------

        // select_storage_for
        //   trait: resolves the concrete storage type for `table` from
        // its option pack.  When the user supplies underlying_key, that
        // type is used; otherwise the default is std::vector<_Type,
        // _Allocator>.  The user is responsible for ensuring any custom
        // underlying accepts _Type / _Allocator (or for binding the
        // alias template before passing).
        template<typename    _Type,
                 typename    _Allocator,
                 typename... _Options>
        struct select_storage_for
        {
            using type =
                typename with_options_pack<_Options...>::template option_t<
                    underlying_key,
                    std::vector<_Type, _Allocator>>;
        };

    NS_END  // internal


    // =========================================================================
    // II.  table CLASS
    // =========================================================================

    // table
    //   class: fundamental owning djinterp table.  Stores elements of
    // `_Type` using `_Allocator`.  Every other axis is option-driven via
    // `_Options...` and resolved through `table_base`.
    //
    //   Inheritance is purely for alias surfacing - `table_base` is
    // stateless.  All storage is owned by this class through `m_storage`.
    //
    //   The trait detection idioms applied to instances of this class
    // automatically pick up the right classification along every one of
    // the twelve axes because the canonical aliases come from
    // `table_base`.
    template<typename    _Type,
             typename    _Allocator = std::allocator<_Type>,
             typename... _Options>
    class table
        : public table_base<
            _Type,
            _Allocator,
            typename internal::select_storage_for<
                _Type, _Allocator, _Options...>::type::iterator,
            typename internal::select_storage_for<
                _Type, _Allocator, _Options...>::type::const_iterator,
            _Options...>
    {
    private:
        using storage_type = typename internal::select_storage_for<
            _Type, _Allocator, _Options...>::type;

        using base = table_base<
            _Type,
            _Allocator,
            typename storage_type::iterator,
            typename storage_type::const_iterator,
            _Options...>;

    public:
        // -----------------------------------------------------------------
        //  type aliases pulled in from base (visible to clients)
        // -----------------------------------------------------------------
        using typename base::value_type;
        using typename base::allocator_type;
        using typename base::size_type;
        using typename base::difference_type;
        using typename base::reference;
        using typename base::const_reference;
        using typename base::pointer;
        using typename base::const_pointer;
        using typename base::config_type;

        // -----------------------------------------------------------------
        //  iterator aliases shadow base
        //   The fundamental table always uses its storage's native
        // iterator types.  iterator_key / const_iterator_key from the
        // option pack are still surfaced on `base` for ADAPTER tables
        // (table_overlay, lookup_table, ...) that wrap iteration; they
        // do NOT influence the fundamental table's begin/end.
        // -----------------------------------------------------------------
        using iterator       = typename storage_type::iterator;
        using const_iterator = typename storage_type::const_iterator;

        // self_type
        //   type: convenience alias for the full table type.
        using self_type = table<_Type, _Allocator, _Options...>;

        // -----------------------------------------------------------------
        //  static axis flags  (derived purely from the option pack)
        // -----------------------------------------------------------------

        // has_comparator
        //   value: true when comparator_key was opted into.
        static constexpr bool has_comparator =
            base::template has_option_v<comparator_key>;

        // has_hasher
        //   value: true when hasher_key was opted into.
        static constexpr bool has_hasher =
            base::template has_option_v<hasher_key>;

        // is_unique
        //   value: true when multiplicity_key resolves to unique_tag.
        static constexpr bool is_unique =
            std::is_same<
                typename base::template option_t<multiplicity_key, multi_tag>,
                unique_tag>::value;

        // is_bounded
        //   value: true when bounds_key resolves to bounded_tag or
        // size_interval_key was opted into.
        static constexpr bool is_bounded =
            ( std::is_same<
                typename base::template option_t<bounds_key, unbounded_tag>,
                bounded_tag>::value ||
              base::template has_option_v<size_interval_key> );


        // =================================================================
        //  CONSTRUCTORS
        // =================================================================

        // table()
        //   constructor: default - empty table with default-constructed
        // storage.
        D_CONSTEXPR table() = default;

        // table(allocator)
        //   constructor: empty table with a user-provided allocator.
        D_CONSTEXPR explicit table(
            const _Allocator& _alloc
        ) noexcept
            : m_storage(_alloc)
        {}

        // table(count, value, allocator)
        //   constructor: fill - count copies of value.
        D_CONSTEXPR table(
            size_type         _count,
            const _Type&      _value,
            const _Allocator& _alloc = _Allocator()
        )
            : m_storage(_count, _value, _alloc)
        {}

        // table(initializer_list, allocator)
        //   constructor: from initializer_list.
        D_CONSTEXPR table(
            std::initializer_list<_Type> _init,
            const _Allocator&            _alloc = _Allocator()
        )
            : m_storage(_init, _alloc)
        {}

        // table(input_it, input_it, allocator)
        //   constructor: range - from a pair of input iterators.
        template<typename _InputIt>
        D_CONSTEXPR table(
            _InputIt          _first,
            _InputIt          _last,
            const _Allocator& _alloc = _Allocator()
        )
            : m_storage(_first, _last, _alloc)
        {}

        D_CONSTEXPR table(const table&)            = default;
        D_CONSTEXPR table(table&&)                 = default;
        D_CONSTEXPR table& operator=(const table&) = default;
        D_CONSTEXPR table& operator=(table&&)      = default;


        // =================================================================
        //  CAPACITY
        // =================================================================

        // size
        //   function: number of elements currently stored.
        D_CONSTEXPR size_type
        size() const noexcept
        {
            return m_storage.size();
        }

        // empty
        //   function: true when no elements are stored.
        D_CONSTEXPR bool
        empty() const noexcept
        {
            return m_storage.empty();
        }

        // capacity
        //   function: storage capacity in elements (where applicable).
        D_CONSTEXPR size_type
        capacity() const noexcept
        {
            return m_storage.capacity();
        }

        // reserve
        //   function: pre-allocate space for at least _n elements.
        D_CONSTEXPR void
        reserve(size_type _n)
        {
            m_storage.reserve(_n);

            return;
        }


        // =================================================================
        //  ITERATION
        // =================================================================

        D_CONSTEXPR iterator       begin()        noexcept { return m_storage.begin();  }
        D_CONSTEXPR const_iterator begin()  const noexcept { return m_storage.begin();  }
        D_CONSTEXPR const_iterator cbegin() const noexcept { return m_storage.cbegin(); }
        D_CONSTEXPR iterator       end()          noexcept { return m_storage.end();    }
        D_CONSTEXPR const_iterator end()    const noexcept { return m_storage.end();    }
        D_CONSTEXPR const_iterator cend()   const noexcept { return m_storage.cend();   }


        // =================================================================
        //  ELEMENT ACCESS
        // =================================================================

        // operator[]
        //   function: indexed element access (no bounds check).
        D_CONSTEXPR reference
        operator[](size_type _i)
        {
            return m_storage[_i];
        }

        D_CONSTEXPR const_reference
        operator[](size_type _i) const
        {
            return m_storage[_i];
        }

        // data
        //   function: raw pointer to underlying buffer (when available).
        D_CONSTEXPR pointer
        data() noexcept
        {
            return m_storage.data();
        }

        D_CONSTEXPR const_pointer
        data() const noexcept
        {
            return m_storage.data();
        }


        // =================================================================
        //  MUTATION
        // =================================================================
        //   Operations are gated on the resolved axis tags.  push_back is
        // available only when the table is NOT sorted-unique (otherwise
        // ordered insert is the right entry point).

        // push_back
        //   function: append _v.  Available on flat / unsorted tables.
        template<bool _Enable = ( !has_comparator && !is_unique ),
                 typename = typename std::enable_if<_Enable>::type>
        D_CONSTEXPR void
        push_back(const _Type& _v)
        {
            m_storage.push_back(_v);

            return;
        }

        // push_back (rvalue)
        template<bool _Enable = ( !has_comparator && !is_unique ),
                 typename = typename std::enable_if<_Enable>::type>
        D_CONSTEXPR void
        push_back(_Type&& _v)
        {
            m_storage.push_back(static_cast<_Type&&>(_v));

            return;
        }

        // clear
        //   function: remove all elements.
        D_CONSTEXPR void
        clear() noexcept
        {
            m_storage.clear();

            return;
        }


        // =================================================================
        //  UNDERLYING ACCESS
        // =================================================================

        // storage
        //   function: const reference to the underlying storage container.
        // Useful for generic algorithms operating on the raw container.
        D_CONSTEXPR const storage_type&
        storage() const noexcept
        {
            return m_storage;
        }

        // storage (mutable)
        D_CONSTEXPR storage_type&
        storage() noexcept
        {
            return m_storage;
        }

        // get_allocator
        //   function: copy of the allocator used by storage.
        D_CONSTEXPR allocator_type
        get_allocator() const noexcept
        {
            return m_storage.get_allocator();
        }

    private:
        storage_type m_storage;
    };


    // =========================================================================
    // III. DETECTION TRAIT
    // =========================================================================

    // is_fundamental_table
    //   trait: true iff _Type is an instantiation of `table<...>`.
    //   This is identity detection (matches the class template directly).
    // For structural detection of "anything that walks like a table", use
    // the existing trait surface from container_traits.hpp - any type
    // built on table_base will classify correctly.
    template<typename _Type>
    struct is_fundamental_table : std::false_type
    {};

    template<typename    _Type,
             typename    _Allocator,
             typename... _Options>
    struct is_fundamental_table<table<_Type, _Allocator, _Options...>>
        : std::true_type
    {};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_fundamental_table_v =
        is_fundamental_table<_Type>::value;
#endif


    // =========================================================================
    // IV.  CLASSIFICATION
    // =========================================================================

    // fundamental_table_class
    //   struct: aggregates the per-axis classification of a `table<...>`
    // instantiation by reading its resolved alias surface.  Members
    // mirror the twelve-axis layout from containers.md.
    //
    //   This struct is meaningful ONLY for instantiations of the literal
    // `table` class template.  For arbitrary types-built-on-table_base,
    // use the existing per-axis traits (is_sorted_container_v,
    // is_unique_container_v, is_bounded_container_v, ...) directly.
    template<typename _Type>
    struct fundamental_table_class
    {
        static constexpr bool is_table = false;
    };

    // fundamental_table_class (specialization)
    template<typename    _Type,
             typename    _Allocator,
             typename... _Options>
    struct fundamental_table_class<table<_Type, _Allocator, _Options...>>
    {
    private:
        using table_t = table<_Type, _Allocator, _Options...>;

    public:
        static constexpr bool is_table       = true;

        // axis 3 - ordering
        static constexpr bool has_comparator = table_t::has_comparator;
        static constexpr bool has_hasher     = table_t::has_hasher;

        // axis 4 - bounds
        static constexpr bool is_bounded     = table_t::is_bounded;

        // axis 5 - multiplicity
        static constexpr bool is_unique      = table_t::is_unique;

        // axis 6 - structure (table is always flat by contract)
        static constexpr bool is_flat        = true;

        // axis 7 - storage (table is always dynamic by contract)
        static constexpr bool is_dynamic     = true;

        // axis 9 - underlying (table is fundamental unless underlying_key
        // was opted into - but at that point the table is overlay-shaped
        // even though it's still owned)
        static constexpr bool is_overlay     =
            table_t::template has_option_v<underlying_key>;

        // multi-dimensional rank
        static constexpr std::size_t rank    = table_t::rank;
    };


NS_END  // djinterp


#endif  // DJINTERP_TABLE_