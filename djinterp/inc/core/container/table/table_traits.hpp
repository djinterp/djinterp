/******************************************************************************
* djinterp [meta]                                             table_traits.hpp
*
* djinterp table traits header (pure tagless paradigm):
*   This header provides comprehensive SFINAE-based type traits for compile-time
* table introspection using a purely tagless configuration paradigm. ALL
* configuration is detected through member presence and values on user-defined
* structs - no special tag types, builders, or library-provided config classes.
*
*   DESIGN PHILOSOPHY:
*   Users define plain structs with constexpr static members. Traits detect
* which members exist and extract their values, defaulting appropriately when
* absent. This enables:
*   - Complete user control over configuration types
*   - No library-imposed vocabulary types
*   - Maximum extensibility
*   - Clean separation between user code and library internals
*
*   USAGE:
*   ```cpp
*   // Define your own config - just a plain struct
*   struct my_report {
*       static constexpr std::size_t header_rows = 2;
*       static constexpr std::size_t header_cols = 1;
*       static constexpr std::size_t total_rows  = 1;
*   };
*   table<int, 10, 5, my_report> t1;
*
*   // For merged cells, define spans with row/col/row_span/col_span members
*   struct merged_header {
*       static constexpr std::size_t row = 0;
*       static constexpr std::size_t col = 1;
*       static constexpr std::size_t row_span = 1;
*       static constexpr std::size_t col_span = 3;
*   };
*   struct complex_config {
*       static constexpr std::size_t header_rows = 2;
*       using spans = std::tuple<merged_header>;
*   };
*   table<int, 10, 5, complex_config> t2;
*
*   // Basic table - no config needed
*   table<int, 4, 5> t3;
*   ```
*
*   RECOGNIZED CONFIG MEMBERS:
*   - header_rows:  std::size_t - number of header rows at top
*   - header_cols:  std::size_t - number of header columns at left
*   - footer_rows:  std::size_t - number of footer rows at bottom
*   - footer_cols:  std::size_t - number of footer columns at right
*   - total_rows:   std::size_t - number of summary rows before footer
*   - total_cols:   std::size_t - number of summary columns before footer cols
*   - spans:        tuple of types, each with row/col/row_span/col_span members
*
*   PORTABILITY:
*   Compatible with C++11 and later. Uses portable trait access patterns.
*
* author(s): Samuel 'teer' Neal-Blim
* file:      \inc\meta\table_traits.hpp                       date: 2025.01.30
******************************************************************************/

#ifndef DJINTERP_TABLE_TRAITS_
#define DJINTERP_TABLE_TRAITS_ 1

#include <cstddef>
#include <tuple>
#include <type_traits>
#include "..\..\djinterp.h"
#include "..\..\type_traits.hpp"


NS_DJINTERP
NS_CONTAINER

    // =========================================================================
    // I.   DETECTION IDIOM
    // =========================================================================

    NS_INTERNAL
        // detector
        //   trait: primary template (failure case).
        template <typename    _Default,
                  typename    _AlwaysVoid,
                  template <typename...> typename _Op,
                  typename... _Args>
        struct detector
        {
            using value_t = std::false_type;
            using type    = _Default;
        };

        // detector
        //   trait: specialization (success case).
        template <typename    _Default,
                  template <typename...> typename _Op,
                  typename... _Args>
        struct detector<_Default, void_t<_Op<_Args...>>, _Op, _Args...>
        {
            using value_t = std::true_type;
            using type    = _Op<_Args...>;
        };

        // nonesuch
        //   type: placeholder for detection failures.
        struct nonesuch
        {
            nonesuch()                      = delete;
            ~nonesuch()                     = delete;
            nonesuch(const nonesuch&)       = delete;
            void operator=(const nonesuch&) = delete;
        };

        // is_detected
        //   trait: true if _Op<_Args...> is well-formed.
        template <template <typename...> typename _Op,
                  typename... _Args>
        using is_detected = typename detector<nonesuch, void, _Op, _Args...>::value_t;

        // detected_t
        //   type: _Op<_Args...> if well-formed, else nonesuch.
        template <template <typename...> typename _Op,
                  typename... _Args>
        using detected_t = typename detector<nonesuch, void, _Op, _Args...>::type;

        // detected_or_t
        //   type: _Op<_Args...> if well-formed, else _Default.
        template <typename    _Default,
                  template <typename...> typename _Op,
                  typename... _Args>
        using detected_or_t = typename detector<_Default, void, _Op, _Args...>::type;

    NS_END  // internal


    // =========================================================================
    // II.  MEMBER DETECTION OPERATIONS
    // =========================================================================

    NS_INTERNAL

        // config member detectors
        template <typename _C> using detect_header_rows  = decltype(_C::header_rows);
        template <typename _C> using detect_header_cols  = decltype(_C::header_cols);
        template <typename _C> using detect_footer_rows  = decltype(_C::footer_rows);
        template <typename _C> using detect_footer_cols  = decltype(_C::footer_cols);
        template <typename _C> using detect_total_rows   = decltype(_C::total_rows);
        template <typename _C> using detect_total_cols   = decltype(_C::total_cols);
        template <typename _C> using detect_spans        = typename _C::spans;
        template <typename _C> using detect_multi_header = typename _C::multi_header;

        // span member detectors (for user-defined span types)
        template <typename _S> using detect_span_row      = decltype(_S::row);
        template <typename _S> using detect_span_col      = decltype(_S::col);
        template <typename _S> using detect_span_row_span = decltype(_S::row_span);
        template <typename _S> using detect_span_col_span = decltype(_S::col_span);

    NS_END  // internal


    // =========================================================================
    // III. VALUE EXTRACTION TRAITS
    // =========================================================================

    // get_header_rows
    //   trait: extracts header_rows from config, defaulting to 0.
    template <typename _Config,
              bool     _Has = internal::is_detected<internal::detect_header_rows, _Config>::value>
    struct get_header_rows : std::integral_constant<std::size_t, 0>
    {
    };

    template <typename _Config>
    struct get_header_rows<_Config, true>
        : std::integral_constant<std::size_t, _Config::header_rows>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template <typename _Config>
    constexpr std::size_t get_header_rows_v = get_header_rows<_Config>::value;
#endif

    // get_header_cols
    //   trait: extracts header_cols from config, defaulting to 0.
    template <typename _Config,
              bool     _Has = internal::is_detected<internal::detect_header_cols, _Config>::value>
    struct get_header_cols : std::integral_constant<std::size_t, 0>
    {
    };

    template <typename _Config>
    struct get_header_cols<_Config, true>
        : std::integral_constant<std::size_t, _Config::header_cols>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template <typename _Config>
    constexpr std::size_t get_header_cols_v = get_header_cols<_Config>::value;
#endif

    // get_footer_rows
    //   trait: extracts footer_rows from config, defaulting to 0.
    template <typename _Config,
              bool     _Has = internal::is_detected<internal::detect_footer_rows, _Config>::value>
    struct get_footer_rows : std::integral_constant<std::size_t, 0>
    {
    };

    template <typename _Config>
    struct get_footer_rows<_Config, true>
        : std::integral_constant<std::size_t, _Config::footer_rows>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template <typename _Config>
    constexpr std::size_t get_footer_rows_v = get_footer_rows<_Config>::value;
#endif

    // get_footer_cols
    //   trait: extracts footer_cols from config, defaulting to 0.
    template <typename _Config,
              bool     _Has = internal::is_detected<internal::detect_footer_cols, _Config>::value>
    struct get_footer_cols : std::integral_constant<std::size_t, 0>
    {
    };

    template <typename _Config>
    struct get_footer_cols<_Config, true>
        : std::integral_constant<std::size_t, _Config::footer_cols>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template <typename _Config>
    constexpr std::size_t get_footer_cols_v = get_footer_cols<_Config>::value;
#endif

    // get_total_rows
    //   trait: extracts total_rows from config, defaulting to 0.
    template <typename _Config,
              bool     _Has = internal::is_detected<internal::detect_total_rows, _Config>::value>
    struct get_total_rows : std::integral_constant<std::size_t, 0>
    {
    };

    template <typename _Config>
    struct get_total_rows<_Config, true>
        : std::integral_constant<std::size_t, _Config::total_rows>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template <typename _Config>
    constexpr std::size_t get_total_rows_v = get_total_rows<_Config>::value;
#endif

    // get_total_cols
    //   trait: extracts total_cols from config, defaulting to 0.
    template <typename _Config,
              bool     _Has = internal::is_detected<internal::detect_total_cols, _Config>::value>
    struct get_total_cols : std::integral_constant<std::size_t, 0>
    {
    };

    template <typename _Config>
    struct get_total_cols<_Config, true>
        : std::integral_constant<std::size_t, _Config::total_cols>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template <typename _Config>
    constexpr std::size_t get_total_cols_v = get_total_cols<_Config>::value;
#endif

    // get_spans
    //   trait: extracts spans tuple from config, defaulting to empty tuple.
    template <typename _Config,
              bool     _Has = internal::is_detected<internal::detect_spans, _Config>::value>
    struct get_spans
    {
        using type = std::tuple<>;
    };

    template <typename _Config>
    struct get_spans<_Config, true>
    {
        using type = typename _Config::spans;
    };

    template <typename _Config>
    using get_spans_t = typename get_spans<_Config>::type;


    // =========================================================================
    // IV.  FEATURE DETECTION TRAITS
    // =========================================================================

    // has_header_rows
    //   trait: true if config specifies non-zero header rows.
    template <typename _Config>
    struct has_header_rows
        : std::integral_constant<bool, (get_header_rows<_Config>::value > 0)>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template <typename _Config>
    constexpr bool has_header_rows_v = has_header_rows<_Config>::value;
#endif

    // has_header_cols
    //   trait: true if config specifies non-zero header columns.
    template <typename _Config>
    struct has_header_cols
        : std::integral_constant<bool, (get_header_cols<_Config>::value > 0)>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template <typename _Config>
    constexpr bool has_header_cols_v = has_header_cols<_Config>::value;
#endif

    // has_footer_rows
    //   trait: true if config specifies non-zero footer rows.
    template <typename _Config>
    struct has_footer_rows
        : std::integral_constant<bool, (get_footer_rows<_Config>::value > 0)>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template <typename _Config>
    constexpr bool has_footer_rows_v = has_footer_rows<_Config>::value;
#endif

    // has_footer_cols
    //   trait: true if config specifies non-zero footer columns.
    template <typename _Config>
    struct has_footer_cols
        : std::integral_constant<bool, (get_footer_cols<_Config>::value > 0)>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template <typename _Config>
    constexpr bool has_footer_cols_v = has_footer_cols<_Config>::value;
#endif

    // has_total_rows
    //   trait: true if config specifies non-zero total rows.
    template <typename _Config>
    struct has_total_rows
        : std::integral_constant<bool, (get_total_rows<_Config>::value > 0)>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template <typename _Config>
    constexpr bool has_total_rows_v = has_total_rows<_Config>::value;
#endif

    // has_total_cols
    //   trait: true if config specifies non-zero total columns.
    template <typename _Config>
    struct has_total_cols
        : std::integral_constant<bool, (get_total_cols<_Config>::value > 0)>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template <typename _Config>
    constexpr bool has_total_cols_v = has_total_cols<_Config>::value;
#endif

    // has_spans
    //   trait: true if config specifies any merged cell spans.
    template <typename _Config>
    struct has_spans
        : std::integral_constant<bool, (std::tuple_size<get_spans_t<_Config>>::value > 0)>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template <typename _Config>
    constexpr bool has_spans_v = has_spans<_Config>::value;
#endif

    // is_config_type
    //   trait: true if type has any recognized config member.
    template <typename _Type>
    struct is_config_type
        : std::integral_constant<bool,
            ( internal::is_detected<internal::detect_header_rows,  _Type>::value ||
              internal::is_detected<internal::detect_header_cols,  _Type>::value ||
              internal::is_detected<internal::detect_footer_rows,  _Type>::value ||
              internal::is_detected<internal::detect_footer_cols,  _Type>::value ||
              internal::is_detected<internal::detect_total_rows,   _Type>::value ||
              internal::is_detected<internal::detect_total_cols,   _Type>::value ||
              internal::is_detected<internal::detect_spans,        _Type>::value ||
              internal::is_detected<internal::detect_multi_header, _Type>::value )>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template <typename _Type>
    constexpr bool is_config_type_v = is_config_type<_Type>::value;
#endif

    // is_basic_config
    //   trait: true if config has no special features.
    template <typename _Config>
    struct is_basic_config
        : std::integral_constant<bool,
            ( !has_header_rows<_Config>::value &&
              !has_header_cols<_Config>::value &&
              !has_footer_rows<_Config>::value &&
              !has_footer_cols<_Config>::value &&
              !has_total_rows<_Config>::value  &&
              !has_total_cols<_Config>::value  &&
              !has_spans<_Config>::value )>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template <typename _Config>
    constexpr bool is_basic_config_v = is_basic_config<_Config>::value;
#endif


    // =========================================================================
    // V.   SPAN TRAITS (detects user-defined span types)
    // =========================================================================

    // is_span_type
    //   trait: true if type has row/col/row_span/col_span members.
    template <typename _Type>
    struct is_span_type
        : std::integral_constant<bool,
            ( internal::is_detected<internal::detect_span_row,      _Type>::value &&
              internal::is_detected<internal::detect_span_col,      _Type>::value &&
              internal::is_detected<internal::detect_span_row_span, _Type>::value &&
              internal::is_detected<internal::detect_span_col_span, _Type>::value )>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template <typename _Type>
    constexpr bool is_span_type_v = is_span_type<_Type>::value;
#endif

    // get_span_row
    //   trait: extracts row from span type.
    template <typename _Span,
              bool     _Has = internal::is_detected<internal::detect_span_row, _Span>::value>
    struct get_span_row : std::integral_constant<std::size_t, 0>
    {
    };

    template <typename _Span>
    struct get_span_row<_Span, true>
        : std::integral_constant<std::size_t, _Span::row>
    {
    };

    // get_span_col
    //   trait: extracts col from span type.
    template <typename _Span,
              bool     _Has = internal::is_detected<internal::detect_span_col, _Span>::value>
    struct get_span_col : std::integral_constant<std::size_t, 0>
    {
    };

    template <typename _Span>
    struct get_span_col<_Span, true>
        : std::integral_constant<std::size_t, _Span::col>
    {
    };

    // get_span_row_span
    //   trait: extracts row_span from span type.
    template <typename _Span,
              bool     _Has = internal::is_detected<internal::detect_span_row_span, _Span>::value>
    struct get_span_row_span : std::integral_constant<std::size_t, 1>
    {
    };

    template <typename _Span>
    struct get_span_row_span<_Span, true>
        : std::integral_constant<std::size_t, _Span::row_span>
    {
    };

    // get_span_col_span
    //   trait: extracts col_span from span type.
    template <typename _Span,
              bool     _Has = internal::is_detected<internal::detect_span_col_span, _Span>::value>
    struct get_span_col_span : std::integral_constant<std::size_t, 1>
    {
    };

    template <typename _Span>
    struct get_span_col_span<_Span, true>
        : std::integral_constant<std::size_t, _Span::col_span>
    {
    };

    // span_contains
    //   trait: checks if a cell position falls within a span.
    template <std::size_t _Row,
              std::size_t _Col,
              typename    _Span>
    struct span_contains
        : std::integral_constant<bool,
            ( (_Row >= get_span_row<_Span>::value) &&
              (_Row <  get_span_row<_Span>::value + get_span_row_span<_Span>::value) &&
              (_Col >= get_span_col<_Span>::value) &&
              (_Col <  get_span_col<_Span>::value + get_span_col_span<_Span>::value) )>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template <std::size_t _Row, std::size_t _Col, typename _Span>
    constexpr bool span_contains_v = span_contains<_Row, _Col, _Span>::value;
#endif

    // span_is_anchor
    //   trait: checks if a cell position is the anchor of a span.
    template <std::size_t _Row,
              std::size_t _Col,
              typename    _Span>
    struct span_is_anchor
        : std::integral_constant<bool,
            ( (_Row == get_span_row<_Span>::value) &&
              (_Col == get_span_col<_Span>::value) )>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template <std::size_t _Row, std::size_t _Col, typename _Span>
    constexpr bool span_is_anchor_v = span_is_anchor<_Row, _Col, _Span>::value;
#endif

    NS_INTERNAL

        // check_cell_in_spans
        //   trait: checks if cell is in any span from a tuple.
        template <std::size_t _Row,
                  std::size_t _Col,
                  typename    _Spans>
        struct check_cell_in_spans;

        template <std::size_t _Row,
                  std::size_t _Col>
        struct check_cell_in_spans<_Row, _Col, std::tuple<>> : std::false_type
        {
        };

        template <std::size_t _Row,
                  std::size_t _Col,
                  typename    _Head,
                  typename... _Tail>
        struct check_cell_in_spans<_Row, _Col, std::tuple<_Head, _Tail...>>
            : std::integral_constant<bool,
                ( span_contains<_Row, _Col, _Head>::value ||
                  check_cell_in_spans<_Row, _Col, std::tuple<_Tail...>>::value )>
        {
        };

        // find_cell_span
        //   trait: finds the span containing a cell.
        template <std::size_t _Row,
                  std::size_t _Col,
                  typename    _Spans>
        struct find_cell_span;

        template <std::size_t _Row,
                  std::size_t _Col>
        struct find_cell_span<_Row, _Col, std::tuple<>>
        {
            using type                  = void;
            static constexpr bool found = false;
        };

        template <std::size_t _Row,
                  std::size_t _Col,
                  typename    _Head,
                  typename... _Tail>
        struct find_cell_span<_Row, _Col, std::tuple<_Head, _Tail...>>
        {
        private:
            using rest = find_cell_span<_Row, _Col, std::tuple<_Tail...>>;

        public:
            using type = typename std::conditional<
                span_contains<_Row, _Col, _Head>::value,
                _Head,
                typename rest::type
            >::type;

            static constexpr bool found =
                ( span_contains<_Row, _Col, _Head>::value || rest::found );
        };

    NS_END  // internal

    // is_merged_cell
    //   trait: true if cell at (_Row, _Col) is in a merged region.
    template <std::size_t _Row,
              std::size_t _Col,
              typename    _Config>
    struct is_merged_cell
        : internal::check_cell_in_spans<_Row, _Col, get_spans_t<_Config>>
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template <std::size_t _Row, std::size_t _Col, typename _Config>
    constexpr bool is_merged_cell_v = is_merged_cell<_Row, _Col, _Config>::value;
#endif

    // is_span_anchor
    //   trait: true if cell at (_Row, _Col) is anchor of a merged region.
    template <std::size_t _Row,
              std::size_t _Col,
              typename    _Config>
    struct is_span_anchor
    {
    private:
        using spans      = get_spans_t<_Config>;
        using found_span = internal::find_cell_span<_Row, _Col, spans>;

    public:
        static constexpr bool value =
            ( found_span::found &&
              span_is_anchor<_Row, _Col, typename found_span::type>::value );
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template <std::size_t _Row, std::size_t _Col, typename _Config>
    constexpr bool is_span_anchor_v = is_span_anchor<_Row, _Col, _Config>::value;
#endif


    // =========================================================================
    // VI.  CELL REGION CLASSIFICATION
    // =========================================================================

    // cell_region
    //   enum: identifies the logical region a cell belongs to.
    enum class cell_region
    {
        header,
        header_col,
        data,
        total,
        total_col,
        footer,
        footer_col,
        header_total,
        corner_top_left,
        corner_top_right,
        corner_bot_left,
        corner_bot_right
    };

    // cell_position
    //   trait: determines the region of a cell at compile time.
    template <std::size_t _Row,
              std::size_t _Col,
              std::size_t _TotalRows,
              std::size_t _TotalCols,
              typename    _Config>
    struct cell_position
    {
    private:
        static constexpr std::size_t hdr_rows = get_header_rows<_Config>::value;
        static constexpr std::size_t hdr_cols = get_header_cols<_Config>::value;
        static constexpr std::size_t ftr_rows = get_footer_rows<_Config>::value;
        static constexpr std::size_t ftr_cols = get_footer_cols<_Config>::value;
        static constexpr std::size_t tot_rows = get_total_rows<_Config>::value;
        static constexpr std::size_t tot_cols = get_total_cols<_Config>::value;

        static constexpr bool in_header_row = (_Row < hdr_rows);
        static constexpr bool in_header_col = (_Col < hdr_cols);
        static constexpr bool in_footer_row = (_Row >= _TotalRows - ftr_rows);
        static constexpr bool in_footer_col = (_Col >= _TotalCols - ftr_cols);
        static constexpr bool in_total_row  =
            ( (_Row >= _TotalRows - ftr_rows - tot_rows) &&
              (_Row <  _TotalRows - ftr_rows) );
        static constexpr bool in_total_col  =
            ( (_Col >= _TotalCols - ftr_cols - tot_cols) &&
              (_Col <  _TotalCols - ftr_cols) );

    public:
        static constexpr cell_region region =
            ( (in_header_row && in_header_col) ? cell_region::corner_top_left  :
              (in_header_row && in_footer_col) ? cell_region::corner_top_right :
              (in_footer_row && in_header_col) ? cell_region::corner_bot_left  :
              (in_footer_row && in_footer_col) ? cell_region::corner_bot_right :
              (in_header_row && in_total_col)  ? cell_region::header_total     :
              (in_header_row)                  ? cell_region::header           :
              (in_footer_row)                  ? cell_region::footer           :
              (in_header_col)                  ? cell_region::header_col       :
              (in_footer_col)                  ? cell_region::footer_col       :
              (in_total_row)                   ? cell_region::total            :
              (in_total_col)                   ? cell_region::total_col        :
              cell_region::data );

        static constexpr bool is_data_cell   = (region == cell_region::data);
        static constexpr bool is_header_cell =
            ( (region == cell_region::header) ||
              (region == cell_region::corner_top_left) );
    };


    // =========================================================================
    // VII. DIMENSION COMPUTATION
    // =========================================================================

    // table_dimensions
    //   trait: computes all dimension metrics for a table.
    template <std::size_t _Rows,
              std::size_t _Cols,
              typename    _Config>
    struct table_dimensions
    {
        static constexpr std::size_t total_rows  = _Rows;
        static constexpr std::size_t total_cols  = _Cols;
        static constexpr std::size_t total_cells = _Rows * _Cols;

        static constexpr std::size_t header_rows  = get_header_rows<_Config>::value;
        static constexpr std::size_t header_cols  = get_header_cols<_Config>::value;
        static constexpr std::size_t footer_rows  = get_footer_rows<_Config>::value;
        static constexpr std::size_t footer_cols  = get_footer_cols<_Config>::value;
        static constexpr std::size_t summary_rows = get_total_rows<_Config>::value;
        static constexpr std::size_t summary_cols = get_total_cols<_Config>::value;

        static constexpr std::size_t data_rows =
            _Rows - header_rows - footer_rows - summary_rows;
        static constexpr std::size_t data_cols =
            _Cols - header_cols - footer_cols - summary_cols;
        static constexpr std::size_t data_cells = data_rows * data_cols;

        static constexpr std::size_t data_row_start = header_rows;
        static constexpr std::size_t data_col_start = header_cols;
        static constexpr std::size_t data_row_end   = _Rows - footer_rows - summary_rows;
        static constexpr std::size_t data_col_end   = _Cols - footer_cols - summary_cols;

        static_assert((header_rows + footer_rows + summary_rows <= _Rows),
                      "Combined special rows exceed total rows.");
        static_assert((header_cols + footer_cols + summary_cols <= _Cols),
                      "Combined special columns exceed total columns.");
    };


    // =========================================================================
    // VIII. EMPTY CONFIG (for basic tables)
    // =========================================================================

    // empty_config
    //   struct: minimal config for basic tables with no features.
    struct empty_config
    {
    };


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_TABLE_TRAITS_
