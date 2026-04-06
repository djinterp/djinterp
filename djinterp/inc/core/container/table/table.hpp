/******************************************************************************
* djinterp [container]                                               table.hpp
*
* djinterp unified table header:
*   A versatile table class supporting multiple specification forms with full
* container_traits compliance. All classification is detected structurally via
* SFINAE — no tags, no registration, no base-class requirements.
*
*   SPECIFICATION FORMS:
*   - DIMENSIONAL (homogeneous matrix):
*       table<int, 4, 5>                 // 4 rows, 5 columns of int
*       table<double, 3, 3, my_config>   // 3x3 with custom config
*
*   - TYPED COLUMNS (heterogeneous — forward declared):
*       typed_table<10, int, double>     // 10 rows, typed columns
*
*   CONFIG FEATURES (all tagless — detected via SFINAE):
*     header_rows/cols:      header regions at top/left
*     header_depth:          multi-level hierarchical headers
*     multi_header:          column grouping descriptors (level/col/col_span)
*     footer_rows/cols:      footer regions at bottom/right
*     total_rows/cols:       summary/totals rows/columns
*     total_row_position:    totals placement (before_data or after_data)
*     total_col_position:    totals column placement
*     spans:                 cell-level merge descriptors
*     splits:                cell-level split descriptors
*     partitions:            logical sub-regions
*     col_merges/col_splits: structural column merge/split
*     row_merges/row_splits: structural row merge/split
*
*   CONTAINER_TRAITS CLASSIFICATION (auto-detected):
*     Lifetime:    immutable (fixed structure, mutable cell values)
*     Bounds:      bounded (constexpr fixed size)
*     Storage:     static (no allocator, compile-time capacity)
*     Iteration:   contiguous (pointer-based iterators + data())
*     Ordering:    ordered (random-access)
*     Sorted:      unsorted
*     Uniqueness:  allows duplicates
*     Structure:   flat
*     Backing:     fundamental (owns storage)
*
*   STORAGE IS PROTECTED:
*     The m_storage member is protected, allowing derived classes to access it
*   while hiding implementation details from users.
*
*   CELL LAYOUT ACCESSORS:
*     Merge, split, and partition query methods are provided separately in
*   table_layout.hpp as free-function templates.
*
*
* path:      /inc/djinterp/container/table/table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2024.11.14
******************************************************************************/

#ifndef DJINTERP_TABLE_
#define DJINTERP_TABLE_ 1

#include <array>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <type_traits>
#include "..\..\djinterp.h"
#include "..\meta\table_traits.hpp"


// D_CFG_TABLE_MAX_DIMENSIONS
//   macro: configurable maximum number of dimensions for table types.
// Override by defining before including this header.
#ifndef D_CFG_TABLE_MAX_DIMENSIONS
    #define D_CFG_TABLE_MAX_DIMENSIONS 64
#endif


NS_DJINTERP
NS_CONTAINER


    // =========================================================================
    // I.   FORWARD DECLARATIONS
    // =========================================================================

    template<typename    _Type,
             std::size_t _Rows,
             std::size_t _Cols,
             typename    _Config>
    class table;

    template<std::size_t _Rows,
             typename... _ColTypes>
    class typed_table;


    // =========================================================================
    // II.  INTERNAL STORAGE AND DIMENSIONAL UTILITIES
    // =========================================================================

    NS_INTERNAL

        // table_storage_type
        //   enum: identifies the internal storage representation of a table.
        // Selected automatically based on column type homogeneity.
        enum class table_storage_type
        {
            matrix,
            tuple
        };


        // =====================================================================
        // II.a  MATRIX STORAGE (homogeneous)
        // =====================================================================

        // matrix_storage
        //   struct: contiguous flat storage for homogeneous tables.
        // Stores _Rows * _Cols elements of type _Type in a single
        // std::array, enabling pointer-based contiguous iteration and
        // direct data() access.
        template<typename    _Type,
                 std::size_t _Rows,
                 std::size_t _Cols>
        struct matrix_storage
        {
            static_assert((_Rows > 0),
                          "Row count must be greater than zero.");
            static_assert((_Cols > 0),
                          "Column count must be greater than zero.");

            static constexpr std::size_t total_cells = _Rows * _Cols;

            std::array<_Type, total_cells> cells;
        };


        // =====================================================================
        // II.b  DIMENSION HELPERS
        // =====================================================================

        // row_col_to_index
        //   trait: converts a (row, col) pair to a flat index for
        // row-major storage.
        template<std::size_t _Row,
                 std::size_t _Col,
                 std::size_t _Cols>
        struct row_col_to_index
        {
            static_assert((_Col < _Cols),
                          "Column index out of range.");

            static constexpr std::size_t value = (_Row * _Cols) + _Col;
        };

    NS_END  // internal


    // =========================================================================
    // III. TABLE CLASS
    // =========================================================================
    //
    // The primary 2D homogeneous table. Satisfies container_traits structural
    // detection for all core axes:
    //   - value_type, size_type, difference_type   (core aliases)
    //   - pointer / const_pointer                  (contiguous detection)
    //   - iterator / const_iterator                (contiguous pointers)
    //   - begin() / end() / cbegin() / cend()      (iterable)
    //   - rbegin() / rend()                        (reverse iterable)
    //   - size() / max_size() / empty()            (sized, bounded, fixed)
    //   - data()                                   (contiguous storage)
    //   - operator[]                               (random access)
    //
    // Omitted by design (activates correct negative detections):
    //   - No allocator_type          → static storage
    //   - No push_back/insert/erase  → immutable structure
    //   - No key_type/hasher         → unsorted, allows duplicates
    //   - No node_type/depth_type    → flat
    //   - No backing_container_type  → fundamental
    //

    // table
    //   class: fixed-size two-dimensional table with contiguous homogeneous
    // storage and full container_traits structural compliance.
    template<typename    _Type,
             std::size_t _Rows,
             std::size_t _Cols,
             typename    _Config = empty_config>
    class table
    {
    private:
        using storage_type = internal::matrix_storage<_Type, _Rows, _Cols>;

    public:
        // -----------------------------------------------------------------
        //  standard container type aliases (structural detection targets)
        // -----------------------------------------------------------------
        using value_type      = _Type;
        using size_type       = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference       = value_type&;
        using const_reference = const value_type&;
        using pointer         = value_type*;
        using const_pointer   = const value_type*;

        // -----------------------------------------------------------------
        //  iterator types (pointer-based for contiguous detection)
        // -----------------------------------------------------------------
        using iterator               = pointer;
        using const_iterator         = const_pointer;
        using reverse_iterator       = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        // -----------------------------------------------------------------
        //  self type and config
        // -----------------------------------------------------------------
        using self_type   = table<_Type, _Rows, _Cols, _Config>;
        using config_type = _Config;

        // -----------------------------------------------------------------
        //  dimension traits (from table_traits.hpp)
        // -----------------------------------------------------------------
        using dimensions = table_dimensions<_Rows, _Cols, _Config>;

        // -----------------------------------------------------------------
        //  compile-time dimensional constants
        // -----------------------------------------------------------------
        static constexpr size_type num_rows    = _Rows;
        static constexpr size_type num_cols    = _Cols;
        static constexpr size_type total_cells = _Rows * _Cols;

        // -----------------------------------------------------------------
        //  compile-time storage classification constants
        //  (assists constexpr detection probes)
        // -----------------------------------------------------------------
        static constexpr size_type storage_rep =
            static_cast<size_type>(internal::table_storage_type::matrix);


        // =================================================================
        //  constructors
        // =================================================================

        // table()
        //   constructor: default constructs all cells.
        D_CONSTEXPR table() = default;

        // table(std::initializer_list)
        //   constructor: initializes cells from a flat initializer list
        // in row-major order.
        D_CONSTEXPR table(
                std::initializer_list<value_type> _init
            )
        {
            size_type i = 0;

            for (const auto& _val : _init)
            {
                // bounds check
                if (i >= total_cells)
                {
                    break;
                }

                m_storage.cells[i] = _val;
                ++i;
            }
        }

        // table(const value_type&)
        //   constructor: fills all cells with a single value.
        D_CONSTEXPR explicit table(
                const value_type& _fill_value
            )
        {
            for (size_type i = 0; i < total_cells; ++i)
            {
                m_storage.cells[i] = _fill_value;
            }
        }


        // =================================================================
        //  capacity (structural detection: sized, bounded, fixed)
        // =================================================================

        // size
        //   function: returns the total number of cells.
        // Detected by: has_size_accessor_v, has_constexpr_size_v,
        //              has_fixed_size_v (size == max_size).
        static D_CONSTEXPR size_type size() noexcept
        {
            return total_cells;
        }

        // max_size
        //   function: returns the maximum number of cells.
        // Detected by: has_max_size_accessor_v, has_constexpr_max_size_v,
        //              has_bounded_capacity_v.
        static D_CONSTEXPR size_type max_size() noexcept
        {
            return total_cells;
        }

        // min_size
        //   function: returns the minimum number of cells.
        // Detected by: has_min_size_accessor_v, has_constexpr_min_size_v,
        //              has_bounded_minimum_v.
        static D_CONSTEXPR size_type min_size() noexcept
        {
            return total_cells;
        }

        // empty
        //   function: returns whether the table has zero cells.
        static D_CONSTEXPR bool empty() noexcept
        {
            return (total_cells == 0);
        }

        // rows
        //   function: returns the number of rows.
        static D_CONSTEXPR size_type rows() noexcept
        {
            return _Rows;
        }

        // cols
        //   function: returns the number of columns.
        static D_CONSTEXPR size_type cols() noexcept
        {
            return _Cols;
        }


        // =================================================================
        //  element access
        // =================================================================

        // data
        //   function: returns a pointer to the underlying contiguous storage.
        // Detected by: has_data_accessor_v (contiguous storage / array).
        D_CONSTEXPR pointer data() noexcept
        {
            return m_storage.cells.data();
        }

        // data (const)
        //   function: returns a const pointer to the underlying storage.
        D_CONSTEXPR const_pointer data() const noexcept
        {
            return m_storage.cells.data();
        }

        // operator[]
        //   function: flat-indexed element access without bounds checking.
        // Detected by: random-access detection probes.
        D_CONSTEXPR reference operator[](
                size_type _index
            ) noexcept
        {
            return m_storage.cells[_index];
        }

        // operator[] (const)
        //   function: flat-indexed const element access.
        D_CONSTEXPR const_reference operator[](
                size_type _index
            ) const noexcept
        {
            return m_storage.cells[_index];
        }

        // at
        //   function: flat-indexed element access with bounds checking.
        D_CONSTEXPR reference at(
                size_type _index
            )
        {
            // bounds validation
            if (_index >= total_cells)
            {
                throw std::out_of_range(
                    "table::at: flat index out of range.");
            }

            return m_storage.cells[_index];
        }

        // at (const)
        //   function: flat-indexed const element access with bounds checking.
        D_CONSTEXPR const_reference at(
                size_type _index
            ) const
        {
            // bounds validation
            if (_index >= total_cells)
            {
                throw std::out_of_range(
                    "table::at: flat index out of range.");
            }

            return m_storage.cells[_index];
        }

        // at (row, col)
        //   function: two-dimensional element access with bounds checking.
        D_CONSTEXPR reference at(
                size_type _row,
                size_type _col
            )
        {
            // bounds validation
            if ( (_row >= _Rows) ||
                 (_col >= _Cols) )
            {
                throw std::out_of_range(
                    "table::at: row or column index out of range.");
            }

            return m_storage.cells[(_row * _Cols) + _col];
        }

        // at (row, col, const)
        //   function: two-dimensional const element access with bounds
        // checking.
        D_CONSTEXPR const_reference at(
                size_type _row,
                size_type _col
            ) const
        {
            // bounds validation
            if ( (_row >= _Rows) ||
                 (_col >= _Cols) )
            {
                throw std::out_of_range(
                    "table::at: row or column index out of range.");
            }

            return m_storage.cells[(_row * _Cols) + _col];
        }

        // cell
        //   function: unchecked two-dimensional element access.
        D_CONSTEXPR reference cell(
                size_type _row,
                size_type _col
            ) noexcept
        {
            return m_storage.cells[(_row * _Cols) + _col];
        }

        // cell (const)
        //   function: unchecked two-dimensional const element access.
        D_CONSTEXPR const_reference cell(
                size_type _row,
                size_type _col
            ) const noexcept
        {
            return m_storage.cells[(_row * _Cols) + _col];
        }

        // front
        //   function: returns a reference to the first cell.
        D_CONSTEXPR reference front() noexcept
        {
            return m_storage.cells[0];
        }

        // front (const)
        //   function: returns a const reference to the first cell.
        D_CONSTEXPR const_reference front() const noexcept
        {
            return m_storage.cells[0];
        }

        // back
        //   function: returns a reference to the last cell.
        D_CONSTEXPR reference back() noexcept
        {
            return m_storage.cells[total_cells - 1];
        }

        // back (const)
        //   function: returns a const reference to the last cell.
        D_CONSTEXPR const_reference back() const noexcept
        {
            return m_storage.cells[total_cells - 1];
        }


        // =================================================================
        //  iteration (structural detection: iterable, contiguous)
        // =================================================================

        // begin
        //   function: returns an iterator to the first cell.
        // Detected by: has_begin_accessor_v, is_iterable_v.
        D_CONSTEXPR iterator begin() noexcept
        {
            return m_storage.cells.data();
        }

        // begin (const)
        //   function: returns a const iterator to the first cell.
        D_CONSTEXPR const_iterator begin() const noexcept
        {
            return m_storage.cells.data();
        }

        // end
        //   function: returns an iterator past the last cell.
        // Detected by: has_end_accessor_v, is_iterable_v.
        D_CONSTEXPR iterator end() noexcept
        {
            return m_storage.cells.data() + total_cells;
        }

        // end (const)
        //   function: returns a const iterator past the last cell.
        D_CONSTEXPR const_iterator end() const noexcept
        {
            return m_storage.cells.data() + total_cells;
        }

        // cbegin
        //   function: returns a const iterator to the first cell.
        // Detected by: has_cbegin_accessor_v, has_const_iteration_v.
        D_CONSTEXPR const_iterator cbegin() const noexcept
        {
            return m_storage.cells.data();
        }

        // cend
        //   function: returns a const iterator past the last cell.
        // Detected by: has_cend_accessor_v, has_const_iteration_v.
        D_CONSTEXPR const_iterator cend() const noexcept
        {
            return m_storage.cells.data() + total_cells;
        }

        // rbegin
        //   function: returns a reverse iterator to the last cell.
        // Detected by: has_rbegin_accessor_v, has_reverse_iteration_v.
        D_CONSTEXPR reverse_iterator rbegin() noexcept
        {
            return reverse_iterator(end());
        }

        // rbegin (const)
        //   function: returns a const reverse iterator to the last cell.
        D_CONSTEXPR const_reverse_iterator rbegin() const noexcept
        {
            return const_reverse_iterator(end());
        }

        // rend
        //   function: returns a reverse iterator before the first cell.
        // Detected by: has_rend_accessor_v, has_reverse_iteration_v.
        D_CONSTEXPR reverse_iterator rend() noexcept
        {
            return reverse_iterator(begin());
        }

        // rend (const)
        //   function: returns a const reverse iterator before the first cell.
        D_CONSTEXPR const_reverse_iterator rend() const noexcept
        {
            return const_reverse_iterator(begin());
        }

        // crbegin
        //   function: returns a const reverse iterator to the last cell.
        // Detected by: has_const_reverse_iteration_v.
        D_CONSTEXPR const_reverse_iterator crbegin() const noexcept
        {
            return const_reverse_iterator(cend());
        }

        // crend
        //   function: returns a const reverse iterator before the first cell.
        // Detected by: has_const_reverse_iteration_v.
        D_CONSTEXPR const_reverse_iterator crend() const noexcept
        {
            return const_reverse_iterator(cbegin());
        }


        // =================================================================
        //  modifiers (cell-level only; structure is immutable)
        // =================================================================

        // fill
        //   function: sets all cells to the given value.
        D_CONSTEXPR void fill(
                const value_type& _value
            )
        {
            for (size_type i = 0; i < total_cells; ++i)
            {
                m_storage.cells[i] = _value;
            }

            return;
        }

        // swap
        //   function: exchanges contents with another table of the same
        // type and dimensions.
        D_CONSTEXPR void swap(
                self_type& _other
            ) noexcept(std::is_nothrow_swappable<value_type>::value)
        {
            m_storage.cells.swap(_other.m_storage.cells);

            return;
        }


        // =================================================================
        //  config-aware accessors
        // =================================================================

        // header_rows
        //   function: returns the number of header rows from config.
        static D_CONSTEXPR size_type header_rows() noexcept
        {
            return get_header_rows<_Config>::value;
        }

        // header_cols
        //   function: returns the number of header columns from config.
        static D_CONSTEXPR size_type header_cols() noexcept
        {
            return get_header_cols<_Config>::value;
        }

        // footer_rows
        //   function: returns the number of footer rows from config.
        static D_CONSTEXPR size_type footer_rows() noexcept
        {
            return get_footer_rows<_Config>::value;
        }

        // footer_cols
        //   function: returns the number of footer columns from config.
        static D_CONSTEXPR size_type footer_cols() noexcept
        {
            return get_footer_cols<_Config>::value;
        }

        // header_depth
        //   function: returns the number of hierarchical header levels.
        static D_CONSTEXPR size_type header_depth() noexcept
        {
            return get_header_depth<_Config>::value;
        }

        // total_row_position
        //   function: returns the total row placement indicator.
        static D_CONSTEXPR size_type total_row_position() noexcept
        {
            return get_total_row_position<_Config>::value;
        }

        // total_col_position
        //   function: returns the total column placement indicator.
        static D_CONSTEXPR size_type total_col_position() noexcept
        {
            return get_total_col_position<_Config>::value;
        }

        // has_multi_level_header
        //   function: returns whether the config defines multi-level
        // headers.
        static D_CONSTEXPR bool has_multi_level_header() noexcept
        {
            return container::has_multi_level_header<_Config>::value;
        }

        // data_row_start
        //   function: returns the first data row index.
        static D_CONSTEXPR size_type data_row_start() noexcept
        {
            return dimensions::data_row_start;
        }

        // data_col_start
        //   function: returns the first data column index.
        static D_CONSTEXPR size_type data_col_start() noexcept
        {
            return dimensions::data_col_start;
        }

        // data_rows
        //   function: returns the number of data rows.
        static D_CONSTEXPR size_type data_rows() noexcept
        {
            return dimensions::data_rows;
        }

        // data_cols
        //   function: returns the number of data columns.
        static D_CONSTEXPR size_type data_cols() noexcept
        {
            return dimensions::data_cols;
        }

        // data_cells
        //   function: returns the number of data cells.
        static D_CONSTEXPR size_type data_cells() noexcept
        {
            return dimensions::data_cells;
        }


        // =================================================================
        //  comparison operators
        // =================================================================

        // operator==
        //   function: element-wise equality comparison.
        D_CONSTEXPR bool operator==(
                const self_type& _other
            ) const noexcept
        {
            return (m_storage.cells == _other.m_storage.cells);
        }

        // operator!=
        //   function: element-wise inequality comparison.
        D_CONSTEXPR bool operator!=(
                const self_type& _other
            ) const noexcept
        {
            return (m_storage.cells != _other.m_storage.cells);
        }

        // operator<
        //   function: lexicographic less-than comparison.
        D_CONSTEXPR bool operator<(
                const self_type& _other
            ) const noexcept
        {
            return (m_storage.cells < _other.m_storage.cells);
        }

        // operator<=
        //   function: lexicographic less-than-or-equal comparison.
        D_CONSTEXPR bool operator<=(
                const self_type& _other
            ) const noexcept
        {
            return (m_storage.cells <= _other.m_storage.cells);
        }

        // operator>
        //   function: lexicographic greater-than comparison.
        D_CONSTEXPR bool operator>(
                const self_type& _other
            ) const noexcept
        {
            return (m_storage.cells > _other.m_storage.cells);
        }

        // operator>=
        //   function: lexicographic greater-than-or-equal comparison.
        D_CONSTEXPR bool operator>=(
                const self_type& _other
            ) const noexcept
        {
            return (m_storage.cells >= _other.m_storage.cells);
        }

    protected:
        storage_type m_storage;
    };


    // =========================================================================
    // IV.  TABLE SFINAE TRAITS (tagless structural detection)
    // =========================================================================
    //
    // These traits detect table-specific properties via SFINAE on the
    // structural interface exposed by table and typed_table. No tags or
    // base-class checks — purely member-presence detection.
    //

    NS_INTERNAL

        // detect_num_rows
        //   trait: detection operation for static num_rows member.
        template<typename _T>
        using detect_num_rows = decltype(std::integral_constant<
            std::size_t, _T::num_rows>{});

        // detect_num_cols
        //   trait: detection operation for static num_cols member.
        template<typename _T>
        using detect_num_cols = decltype(std::integral_constant<
            std::size_t, _T::num_cols>{});

        // detect_total_cells
        //   trait: detection operation for static total_cells member.
        template<typename _T>
        using detect_total_cells = decltype(std::integral_constant<
            std::size_t, _T::total_cells>{});

        // detect_config_type
        //   trait: detection operation for config_type alias.
        template<typename _T>
        using detect_config_type = typename _T::config_type;

        // detect_dimensions
        //   trait: detection operation for dimensions alias.
        template<typename _T>
        using detect_dimensions = typename _T::dimensions;

        // detect_rows_method
        //   trait: detection operation for static rows() method.
        template<typename _T>
        using detect_rows_method = decltype(_T::rows());

        // detect_cols_method
        //   trait: detection operation for static cols() method.
        template<typename _T>
        using detect_cols_method = decltype(_T::cols());

        // detect_cell_method
        //   trait: detection operation for cell(row, col) method.
        template<typename _T>
        using detect_cell_method = decltype(
            std::declval<_T&>().cell(std::size_t{}, std::size_t{}));

    NS_END  // internal

    // is_table_type
    //   trait: true if a type exposes the structural interface of a djinterp
    // table (num_rows, num_cols, total_cells, config_type, rows(), cols(),
    // cell()).
    template<typename _Type,
             typename = void>
    struct is_table_type : std::false_type
    {
    };

    // is_table_type (specialization)
    //   trait: SFINAE success case — all table structural probes well-formed.
    template<typename _Type>
    struct is_table_type<_Type,
        void_t<internal::detect_num_rows<_Type>,
               internal::detect_num_cols<_Type>,
               internal::detect_total_cells<_Type>,
               internal::detect_config_type<_Type>,
               internal::detect_rows_method<_Type>,
               internal::detect_cols_method<_Type>,
               internal::detect_cell_method<_Type>>>
        : std::true_type
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_table_type_v
    //   value: convenience variable template for is_table_type.
    template<typename _Type>
    constexpr bool is_table_type_v = is_table_type<_Type>::value;
#endif

    NS_INTERNAL

        // detect_data_method
        //   trait: detection operation for data() method returning a pointer.
        template<typename _T>
        using detect_data_method = decltype(std::declval<_T&>().data());

    NS_END  // internal

    // is_homogeneous_table
    //   trait: true if the type is a table with homogeneous (matrix) storage.
    // Detected by presence of data() returning a non-void pointer, combined
    // with the table structural interface.
    template<typename _Type,
             typename = void>
    struct is_homogeneous_table : std::false_type
    {
    };

    // is_homogeneous_table (specialization)
    //   trait: SFINAE success — table type with data() accessor.
    template<typename _Type>
    struct is_homogeneous_table<_Type,
        void_t<internal::detect_data_method<_Type>,
               internal::detect_num_rows<_Type>,
               internal::detect_cell_method<_Type>>>
        : std::true_type
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_homogeneous_table_v
    //   value: convenience variable template for is_homogeneous_table.
    template<typename _Type>
    constexpr bool is_homogeneous_table_v =
        is_homogeneous_table<_Type>::value;
#endif

    // has_table_config
    //   trait: true if the table type has a non-empty configuration.
    template<typename _Type,
             typename = void>
    struct has_table_config : std::false_type
    {
    };

    // has_table_config (specialization)
    //   trait: SFINAE success — table type whose config_type is recognized.
    template<typename _Type>
    struct has_table_config<_Type,
        void_t<internal::detect_config_type<_Type>,
               internal::detect_num_rows<_Type>>>
        : is_config_type<typename _Type::config_type>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_table_config_v
    //   value: convenience variable template for has_table_config.
    template<typename _Type>
    constexpr bool has_table_config_v = has_table_config<_Type>::value;
#endif

    // table_value_type_of
    //   trait: extracts the value_type from a table type, or void if not
    // a table.
    template<typename _Type,
             typename = void>
    struct table_value_type_of
    {
        using type = void;
    };

    // table_value_type_of (specialization)
    //   trait: extracts value_type when the type is a valid table.
    template<typename _Type>
    struct table_value_type_of<_Type,
        void_t<internal::detect_num_rows<_Type>,
               typename _Type::value_type>>
    {
        using type = typename _Type::value_type;
    };

    // table_value_type_of_t
    //   type: convenience alias for table_value_type_of<...>::type.
    template<typename _Type>
    using table_value_type_of_t = typename table_value_type_of<_Type>::type;

    // table_dimensions_of
    //   trait: extracts row and column counts from a table type.
    template<typename _Type,
             bool     _IsTable = is_table_type<_Type>::value>
    struct table_dimensions_of
    {
        static constexpr std::size_t rows  = 0;
        static constexpr std::size_t cols  = 0;
        static constexpr std::size_t cells = 0;
    };

    // table_dimensions_of (specialization)
    //   trait: extracts dimensions when the type is a valid table.
    template<typename _Type>
    struct table_dimensions_of<_Type, true>
    {
        static constexpr std::size_t rows  = _Type::num_rows;
        static constexpr std::size_t cols  = _Type::num_cols;
        static constexpr std::size_t cells = _Type::total_cells;
    };


    // =========================================================================
    // V.   TABLE COMPATIBILITY TRAITS
    // =========================================================================

    // tables_same_dimensions
    //   trait: true if two table types have identical row and column counts.
    // SFINAE-safe: yields false_type for non-table types.
    template<typename _TableA,
             typename _TableB,
             typename = void>
    struct tables_same_dimensions : std::false_type
    {
    };

    // tables_same_dimensions (specialization)
    //   trait: SFINAE success — both types are tables with matching dimensions.
    template<typename _TableA,
             typename _TableB>
    struct tables_same_dimensions<_TableA, _TableB,
        void_t<internal::detect_num_rows<_TableA>,
               internal::detect_num_cols<_TableA>,
               internal::detect_num_rows<_TableB>,
               internal::detect_num_cols<_TableB>>>
        : std::integral_constant<bool,
            ( (_TableA::num_rows == _TableB::num_rows) &&
              (_TableA::num_cols == _TableB::num_cols) )>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // tables_same_dimensions_v
    //   value: convenience variable template for tables_same_dimensions.
    template<typename _TableA,
             typename _TableB>
    constexpr bool tables_same_dimensions_v =
        tables_same_dimensions<_TableA, _TableB>::value;
#endif

    // tables_same_type
    //   trait: true if two table types have the same dimensions and
    // value_type. SFINAE-safe: yields false_type for non-table types.
    template<typename _TableA,
             typename _TableB,
             typename = void>
    struct tables_same_type : std::false_type
    {
    };

    // tables_same_type (specialization)
    //   trait: SFINAE success — both types are tables with matching
    // dimensions and value_type.
    template<typename _TableA,
             typename _TableB>
    struct tables_same_type<_TableA, _TableB,
        void_t<internal::detect_num_rows<_TableA>,
               internal::detect_num_rows<_TableB>,
               typename _TableA::value_type,
               typename _TableB::value_type>>
        : std::integral_constant<bool,
            ( tables_same_dimensions<_TableA, _TableB>::value &&
              std::is_same<typename _TableA::value_type,
                           typename _TableB::value_type>::value )>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // tables_same_type_v
    //   value: convenience variable template for tables_same_type.
    template<typename _TableA,
             typename _TableB>
    constexpr bool tables_same_type_v =
        tables_same_type<_TableA, _TableB>::value;
#endif

    // tables_element_convertible
    //   trait: true if elements of _TableFrom are convertible to elements
    // of _TableTo. SFINAE-safe: yields false_type for non-table types.
    template<typename _TableFrom,
             typename _TableTo,
             typename = void>
    struct tables_element_convertible : std::false_type
    {
    };

    // tables_element_convertible (specialization)
    //   trait: SFINAE success — both types are tables with convertible
    // value_types.
    template<typename _TableFrom,
             typename _TableTo>
    struct tables_element_convertible<_TableFrom, _TableTo,
        void_t<internal::detect_num_rows<_TableFrom>,
               internal::detect_num_rows<_TableTo>,
               typename _TableFrom::value_type,
               typename _TableTo::value_type>>
        : std::integral_constant<bool,
            std::is_convertible<typename _TableFrom::value_type,
                                typename _TableTo::value_type>::value>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // tables_element_convertible_v
    //   value: convenience variable template for tables_element_convertible.
    template<typename _TableFrom,
             typename _TableTo>
    constexpr bool tables_element_convertible_v =
        tables_element_convertible<_TableFrom, _TableTo>::value;
#endif


    // =========================================================================
    // VI.  TABLE CLASSIFICATION
    // =========================================================================
    //
    // Aggregates all table-specific structural detections into a single
    // compile-time classification struct. Analogous to container_class<T>
    // for the container_traits system, but specific to table properties.
    //

    // table_class
    //   struct: compile-time classification of a table type. Aggregates
    // table identity, structural immutability, homogeneity, config
    // features, and dimension information. All members are static
    // constexpr, determined purely by structural SFINAE.
    template<typename _Type,
             bool     _IsTable = is_table_type<_Type>::value>
    struct table_class
    {
        // identity
        static constexpr bool is_table      = false;
        static constexpr bool is_homogeneous = false;
        static constexpr bool has_config     = false;

        // structural mutability
        static constexpr bool structurally_immutable = false;
        static constexpr bool value_mutable          = false;
        static constexpr bool shape_modifiers        = false;

        // dimensions
        static constexpr std::size_t num_rows    = 0;
        static constexpr std::size_t num_cols    = 0;
        static constexpr std::size_t total_cells = 0;

        // layout
        static constexpr bool has_merged_cells = false;
        static constexpr bool has_split_cells  = false;

        // multi-level header
        static constexpr bool        has_multi_level    = false;
        static constexpr std::size_t header_depth_value = 0;

        // partitions
        static constexpr bool        has_table_partitions = false;
        static constexpr std::size_t partition_count      = 0;

        // structural column/row merges and splits
        static constexpr bool        has_structural_features = false;
        static constexpr std::size_t col_merge_count  = 0;
        static constexpr std::size_t col_split_count  = 0;
        static constexpr std::size_t row_merge_count  = 0;
        static constexpr std::size_t row_split_count  = 0;
    };

    // table_class (specialization)
    //   struct: classification for types that satisfy the table structural
    // interface.
    template<typename _Type>
    struct table_class<_Type, true>
    {
    private:
        using config = typename _Type::config_type;
        using cc     = table_config_class<config>;

    public:
        // identity
        static constexpr bool is_table       = true;
        static constexpr bool is_homogeneous =
            is_homogeneous_table<_Type>::value;
        static constexpr bool has_config     = has_table_config<_Type>::value;

        // structural mutability (from table_traits.hpp)
        static constexpr bool structurally_immutable =
            is_structurally_immutable<_Type>::value;
        static constexpr bool value_mutable =
            is_value_mutable<_Type>::value;
        static constexpr bool shape_modifiers =
            has_shape_modifiers<_Type>::value;

        // dimensions
        static constexpr std::size_t num_rows    = _Type::num_rows;
        static constexpr std::size_t num_cols    = _Type::num_cols;
        static constexpr std::size_t total_cells = _Type::total_cells;

        // layout (from config classification)
        static constexpr bool has_merged_cells = cc::has_merged_cells;
        static constexpr bool has_split_cells  = cc::has_split_cells;

        // multi-level header
        static constexpr bool        has_multi_level    = cc::has_multi_level;
        static constexpr std::size_t header_depth_value = cc::header_depth;

        // partitions
        static constexpr bool        has_table_partitions = cc::has_table_partitions;
        static constexpr std::size_t partition_count      = cc::partition_count;

        // structural column/row merges and splits
        static constexpr bool        has_structural_features = cc::has_structural_features;
        static constexpr std::size_t col_merge_count  = cc::col_merge_count;
        static constexpr std::size_t col_split_count  = cc::col_split_count;
        static constexpr std::size_t row_merge_count  = cc::row_merge_count;
        static constexpr std::size_t row_split_count  = cc::row_split_count;

        // config detail
        static constexpr bool        has_regions    = cc::has_regions;
        static constexpr bool        is_basic       = cc::is_basic;
        static constexpr std::size_t merge_count    = cc::merge_count;
        static constexpr std::size_t split_count    = cc::split_count;
    };


    // =========================================================================
    // VII. NON-MEMBER FUNCTIONS
    // =========================================================================

    // swap
    //   function: exchanges the contents of two tables.
    template<typename    _Type,
             std::size_t _Rows,
             std::size_t _Cols,
             typename    _Config>
    D_CONSTEXPR void swap(
            table<_Type, _Rows, _Cols, _Config>& _lhs,
            table<_Type, _Rows, _Cols, _Config>& _rhs
        ) noexcept(noexcept(_lhs.swap(_rhs)))
    {
        _lhs.swap(_rhs);

        return;
    }


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_TABLE_
