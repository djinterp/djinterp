/*******************************************************************************
* djinterp [cpp][container]                                      table_tests.hpp
*
* Shared header for the container/table/ suite: the D_TBL_CHECK macro and its
* routing function, tbl_throws, the declarations of every section's tests,
* and table_spec().  Each section includes its own header and nothing else of
* the table directory, so every header is compiled alone.
*
*   The section .cpp files that DEFINE these predicates:
*     table_tests_base.cpp             - table_base.hpp
*     table_tests_table.cpp            - table.hpp
*     table_tests_fixed.cpp            - fixed_table.hpp
*     table_tests_static.cpp           - static_table.hpp
*     table_tests_keyed.cpp            - keyed_table.hpp
*     table_tests_sorted.cpp           - sorted_table.hpp
*     table_tests_constrained.cpp      - constrained_table.hpp
*     table_tests_metadata.cpp         - table_metadata.hpp
*     table_tests_shape.cpp            - table_shape.hpp
*     table_tests_layout.cpp           - table_layout.hpp
*     table_tests_builder.cpp          - table_builder.hpp
*     table_tests_transpose.cpp        - table_builder.hpp
*     table_tests_builder_options.cpp  - table_builder.hpp
*     table_tests_options.cpp          - table_options.hpp
*     table_tests_option_partition.cpp - table_options.hpp
*     table_tests_model.cpp            - table_model.hpp
*     table_tests_parser.cpp           - table_parser.hpp
*     table_tests_template.cpp         - table_template.hpp
*
*   The floor suite (2026.11.16): the ten assert-based test_table_*.cpp programs,
* built by no leaf, are converted into the shape .. template sections; the
* eight table headers no program reached have sections written for them.
*
*
* path:      /tests/djinterp/cpp/container/table/table_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.16
*                                                            revised: 2026.11.16
*******************************************************************************/
#ifndef DJINTERP_CONTAINER_TABLE_TESTS_
#define DJINTERP_CONTAINER_TABLE_TESTS_ 1

// std
#include <cstdio>   // std::printf
// djinterp
#include <djinterp/cpp/djinterp.hpp>  // framework root
#ifdef DTEST_SPEC_MODE
#include <djinterp/cpp/modules/test/test_defaults.hpp>  // module_spec (spec)
#endif


// the name ::djinterp::test exists in both faces for the dt alias below
namespace djinterp { namespace test {} }

NS_DJINTERP
NS_TESTING

// dt names DTest's entities (djinterp::test).
namespace dt = ::djinterp::test;


// table_check
//   function: routes one D_TBL_CHECK evaluation.  Prints the failing
// expression and its location and hands the condition back.
inline
bool table_check(
    bool        _condition,
    const char* _expression,
    const char* _file,
    int         _line
)
{
    if (!_condition)
    {
        std::printf("    [FAIL] %s:%d: %s\n", _file, _line, _expression);
    }

    return _condition;
}

// D_TBL_CHECK
//   macro: fail the current test - return false from the enclosing predicate -
// unless the condition holds, printing the expression that failed.  Variadic
// so a condition may contain top-level commas.  Unique letters (TBL) so
// co-compiled suites never collide on the macro name (D_TB_CHECK is taken).
#define D_TBL_CHECK(...)                                                      \
    do                                                                        \
    {                                                                         \
        if (!::djinterp::testing::table_check(                                \
                 (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__))            \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    }                                                                         \
    while (0)


#ifndef DTEST_SPEC_MODE  // fixtures -- normal mode only

// tbl_throws
//   function: whether calling _fn throws an exception of type Ex (any other
// exception propagates, failing the test loudly).
template<typename Ex,
         typename Fn>
bool tbl_throws(Fn _fn)
{
    try
    {
        _fn();
    }
    catch (const Ex&)
    {
        return true;
    }

    return false;
}

#endif  // !DTEST_SPEC_MODE


// ===========================================================================
// Declarations  (grouped by section file)
// ===========================================================================

// base: table_base.hpp
bool tests_table_base_row_view();
bool tests_table_base_column_view();
bool tests_table_base_row_cursor();
bool tests_table_base_surface();
bool tests_table_base_count_and_sortedness();

// table: table.hpp
bool tests_table_construction();
bool tests_table_axes();
bool tests_table_access();
bool tests_table_row_operations();
bool tests_table_column_operations();
bool tests_table_resize_and_capacity();
bool tests_table_algorithms();
bool tests_table_equality_and_swap();
bool tests_table_serial();
bool tests_table_detection();

// fixed: fixed_table.hpp
bool tests_fixed_table_construction();
bool tests_fixed_table_extents();
bool tests_fixed_table_mutation();
bool tests_fixed_table_algorithms();
bool tests_fixed_table_serial();

// static: static_table.hpp
bool tests_static_table_construction();
bool tests_static_table_constexpr();
bool tests_static_table_serial();

// keyed: keyed_table.hpp
bool tests_keyed_table_construction();
bool tests_keyed_table_lookup();
bool tests_keyed_table_insert_erase();
bool tests_keyed_table_multi();
bool tests_keyed_table_projection();

// sorted: sorted_table.hpp
bool tests_sorted_table_construction();
bool tests_sorted_table_lookup();
bool tests_sorted_table_insert();
bool tests_sorted_table_refusal();
bool tests_sorted_table_erase();

// constrained: constrained_table.hpp
bool tests_constrained_table_capacity();
bool tests_constrained_table_domain();
bool tests_constrained_table_reads();
bool tests_constrained_table_removal();

// metadata: table_metadata.hpp
bool tests_table_metadata_headers();
bool tests_table_metadata_entries();
bool tests_table_metadata_conformance();

// shape: table_shape.hpp
bool tests_table_shape();

// layout: table_layout.hpp
bool tests_table_layout_region_basics();
bool tests_table_layout_relations();
bool tests_table_layout_grouped_header();
bool tests_table_layout_validity();
bool tests_table_layout_split();
bool tests_table_layout_detection();
bool tests_table_layout_runtime();

// builder: table_builder.hpp
bool tests_table_builder_empty_4d();
bool tests_table_builder_row_inference();
bool tests_table_builder_mixed_groups();
bool tests_table_builder_merge_advance();
bool tests_table_builder_split_span();
bool tests_table_builder_cell_wrapper();
bool tests_table_builder_realization();
bool tests_table_builder_val_t();
bool tests_table_builder_detection();

// transpose: table_builder.hpp
bool tests_table_transpose_decl();
bool tests_table_transpose_leading_axis();
bool tests_table_transpose_normalized();
bool tests_table_transpose_merge();
bool tests_table_transpose_flat_data();
bool tests_table_transpose_regression();

// builder options: table_builder.hpp
bool tests_table_builder_options();

// options: table_options.hpp
bool tests_table_options();

// option partition: table_options.hpp
bool tests_table_option_partition();

// model: table_model.hpp
bool tests_table_model_compile_time();
bool tests_table_model_runtime();

// parser: table_parser.hpp
bool tests_table_parser();

// template: table_template.hpp
bool tests_table_template();


// ===========================================================================
// The spec  (spec mode only)
// ===========================================================================
#ifdef DTEST_SPEC_MODE

// table_spec
//   function: the authoritative description of this suite, one block per
// section file.
inline
dt::module_spec table_spec()
{
    return dt::module_spec{
        "table",
        "container/table/: the table base, the dynamic, fixed, static, keyed, "
        "sorted and constrained tables, metadata, and the declaration "
        "vocabulary (shape, layout, builder, options, model, parser, "
        "template).",
        {
            dt::block_spec{
                "base",
                "table_base.hpp: the views, the cursors and the CRTP read-only "
                "surface.",
                {
                    { "tests_table_base_row_view",
                      "basic_row_view reads, checks and writes one row.",
                      &tests_table_base_row_view },
                    { "tests_table_base_column_view",
                      "basic_column_view and its cursor walk a column by "
                      "stride.",
                      &tests_table_base_column_view },
                    { "tests_table_base_row_cursor",
                      "row_cursor yields each row as a view, with random "
                      "access.",
                      &tests_table_base_row_cursor },
                    { "tests_table_base_surface",
                      "The shape, cell, row, column and iterator surface.",
                      &tests_table_base_surface },
                    { "tests_table_base_count_and_sortedness",
                      "count, is_sorted, is_row_sorted and content_equals.",
                      &tests_table_base_count_and_sortedness },
                }
            },
            dt::block_spec{
                "table",
                "table.hpp: the dynamic table.",
                {
                    { "tests_table_construction",
                      "Every constructor, and the shapes each refuses.",
                      &tests_table_construction },
                    { "tests_table_axes",
                      "The declared axis positions and member types.",
                      &tests_table_axes },
                    { "tests_table_access",
                      "Writable access through cells, rows, columns and "
                      "iterators.",
                      &tests_table_access },
                    { "tests_table_row_operations",
                      "push_row, insert_row, erase_row and pop_row.",
                      &tests_table_row_operations },
                    { "tests_table_column_operations",
                      "insert_column, push_column, erase_column, pop_column, "
                      "with a column's height checked.",
                      &tests_table_column_operations },
                    { "tests_table_resize_and_capacity",
                      "resize, reserve, fill, clear and shrink_to_fit.",
                      &tests_table_resize_and_capacity },
                    { "tests_table_algorithms",
                      "sort_rows, map, filter_rows and select_rows.",
                      &tests_table_algorithms },
                    { "tests_table_equality_and_swap",
                      "Equality includes the width; both swaps exchange "
                      "tables.",
                      &tests_table_equality_and_swap },
                    { "tests_table_serial",
                      "The serial pair round-trips and refuses corrupt "
                      "shapes.",
                      &tests_table_serial },
                    { "tests_table_detection",
                      "is_table recognises table instantiations only.",
                      &tests_table_detection },
                }
            },
            dt::block_spec{
                "fixed",
                "fixed_table.hpp: the fixed-shape mutable table.",
                {
                    { "tests_fixed_table_construction",
                      "Every constructor, make_fixed_table and constexpr "
                      "reads.",
                      &tests_fixed_table_construction },
                    { "tests_fixed_table_extents",
                      "The static extents; a fixed table is always full.",
                      &tests_fixed_table_extents },
                    { "tests_fixed_table_mutation",
                      "Writable access, fill and both swaps.",
                      &tests_fixed_table_mutation },
                    { "tests_fixed_table_algorithms",
                      "sort_rows, map and map_inplace.",
                      &tests_fixed_table_algorithms },
                    { "tests_fixed_table_serial",
                      "Equality, the serial pair, and is_fixed_table.",
                      &tests_fixed_table_serial },
                }
            },
            dt::block_spec{
                "static",
                "static_table.hpp: the compile-time read-only table.",
                {
                    { "tests_static_table_construction",
                      "Every constructor; the extents; const iteration.",
                      &tests_static_table_construction },
                    { "tests_static_table_constexpr",
                      "Reads and map are constant expressions.",
                      &tests_static_table_constexpr },
                    { "tests_static_table_serial",
                      "Equality, the serial pair, and is_static_table.",
                      &tests_static_table_serial },
                }
            },
            dt::block_spec{
                "keyed",
                "keyed_table.hpp: the record table kept in key order.",
                {
                    { "tests_keyed_table_construction",
                      "Records come out in key order; the constants and "
                      "detection.",
                      &tests_keyed_table_construction },
                    { "tests_keyed_table_lookup",
                      "key_of, the bounds, find, index_of, contains_key, "
                      "count_key.",
                      &tests_keyed_table_lookup },
                    { "tests_keyed_table_insert_erase",
                      "insert replaces an equal key; erase_at, erase_key, "
                      "clear.",
                      &tests_keyed_table_insert_erase },
                    { "tests_keyed_table_multi",
                      "The multimap keeps equal keys in insertion order.",
                      &tests_keyed_table_multi },
                    { "tests_keyed_table_projection",
                      "A functor projection and a descending comparator.",
                      &tests_keyed_table_projection },
                }
            },
            dt::block_spec{
                "sorted",
                "sorted_table.hpp: the table kept sorted on a key column.",
                {
                    { "tests_sorted_table_construction",
                      "Rows come out sorted; the read-only face and constants.",
                      &tests_sorted_table_construction },
                    { "tests_sorted_table_lookup",
                      "The bounds, find, contains_key and count_key.",
                      &tests_sorted_table_lookup },
                    { "tests_sorted_table_insert",
                      "Equal keys keep insertion order; unique keys "
                      "overwrite.",
                      &tests_sorted_table_insert },
                    { "tests_sorted_table_refusal",
                      "Unplaceable rows are refused and change "
                      "nothing.",
                      &tests_sorted_table_refusal },
                    { "tests_sorted_table_erase",
                      "erase_at, erase_key and clear.",
                      &tests_sorted_table_erase },
                }
            },
            dt::block_spec{
                "constrained",
                "constrained_table.hpp: the row cap and the closed domain.",
                {
                    { "tests_constrained_table_capacity",
                      "The cap refuses rows past max_rows, both ways.",
                      &tests_constrained_table_capacity },
                    { "tests_constrained_table_domain",
                      "The closed domain refuses rows with a cell outside it.",
                      &tests_constrained_table_domain },
                    { "tests_constrained_table_reads",
                      "The read-only face delegates to the backing table.",
                      &tests_constrained_table_reads },
                    { "tests_constrained_table_removal",
                      "Removal frees room; clear keeps the domain.",
                      &tests_constrained_table_removal },
                }
            },
            dt::block_spec{
                "metadata",
                "table_metadata.hpp: headers, entries and conformance.",
                {
                    { "tests_table_metadata_headers",
                      "Header cells, levels, stacks and their extents.",
                      &tests_table_metadata_headers },
                    { "tests_table_metadata_entries",
                      "The inherited key/value entries; is_table_metadata.",
                      &tests_table_metadata_entries },
                    { "tests_table_metadata_conformance",
                      "Column and row headers, and whether they fit a table.",
                      &tests_table_metadata_conformance },
                }
            },
            dt::block_spec{
                "shape",
                "table_shape.hpp (converted from test_table_shape.cpp).",
                {
                    { "tests_table_shape",
                      "The type-level shape and its queries.",
                      &tests_table_shape },
                }
            },
            dt::block_spec{
                "layout",
                "table_layout.hpp (converted from test_table_layout.cpp).",
                {
                    { "tests_table_layout_region_basics",
                      "A region's extent, merge flags and anchor.",
                      &tests_table_layout_region_basics },
                    { "tests_table_layout_relations",
                      "Containment, overlap and nesting of regions.",
                      &tests_table_layout_relations },
                    { "tests_table_layout_grouped_header",
                      "A grouped header as a cover: every position's owner.",
                      &tests_table_layout_grouped_header },
                    { "tests_table_layout_validity",
                      "A layout is valid when in bounds and pairwise disjoint.",
                      &tests_table_layout_validity },
                    { "tests_table_layout_split",
                      "A split's pieces must tile the parent exactly.",
                      &tests_table_layout_split },
                    { "tests_table_layout_detection",
                      "The detection traits, and constexpr region_value.",
                      &tests_table_layout_detection },
                    { "tests_table_layout_runtime",
                      "The runtime layout: merges, owners, bounds and overlap.",
                      &tests_table_layout_runtime },
                }
            },
            dt::block_spec{
                "builder",
                "table_builder.hpp (converted from test_table_builder.cpp).",
                {
                    { "tests_table_builder_empty_4d",
                      "An empty table of three column groups folds at compile "
                      "time.",
                      &tests_table_builder_empty_4d },
                    { "tests_table_builder_row_inference",
                      "Rows are inferred from the columns of a 2d table.",
                      &tests_table_builder_row_inference },
                    { "tests_table_builder_mixed_groups",
                      "Two column groups with rows: the 3 x 4 x n declaration.",
                      &tests_table_builder_mixed_groups },
                    { "tests_table_builder_merge_advance",
                      "A merge advances the cursor by its span.",
                      &tests_table_builder_merge_advance },
                    { "tests_table_builder_split_span",
                      "A split cell spans its box.",
                      &tests_table_builder_split_span },
                    { "tests_table_builder_cell_wrapper",
                      "cell<> is accepted and mixes with bare entries.",
                      &tests_table_builder_cell_wrapper },
                    { "tests_table_builder_realization",
                      "Shape and realization: homogeneous against record.",
                      &tests_table_builder_realization },
                    { "tests_table_builder_val_t",
                      "val_t<> lifts a non-type parameter into a cell (C++17).",
                      &tests_table_builder_val_t },
                    { "tests_table_builder_detection",
                      "The builder detection traits.",
                      &tests_table_builder_detection },
                }
            },
            dt::block_spec{
                "transpose",
                "table_builder.hpp, inverse declarations (converted from "
                "test_table_transpose.cpp).",
                {
                    { "tests_table_transpose_decl",
                      "transpose_decl swaps the declarators.",
                      &tests_table_transpose_decl },
                    { "tests_table_transpose_leading_axis",
                      "The leading declarator picks the independent axis.",
                      &tests_table_transpose_leading_axis },
                    { "tests_table_transpose_normalized",
                      "The normal form is the literal column-wise rewrite.",
                      &tests_table_transpose_normalized },
                    { "tests_table_transpose_merge",
                      "A merge survives the inverse declaration, its box "
                      "turned.",
                      &tests_table_transpose_merge },
                    { "tests_table_transpose_flat_data",
                      "A non-leading columns<> declared inversely is flat "
                      "data.",
                      &tests_table_transpose_flat_data },
                    { "tests_table_transpose_regression",
                      "The earlier column-major suite still folds identically.",
                      &tests_table_transpose_regression },
                }
            },
            dt::block_spec{
                "builder options",
                "table_builder.hpp with options (converted from "
                "test_table_builder_options.cpp).",
                {
                    { "tests_table_builder_options",
                      "Options ride on a builder and reach its shape.",
                      &tests_table_builder_options },
                }
            },
            dt::block_spec{
                "options",
                "table_options.hpp (converted from test_table_options.cpp).",
                {
                    { "tests_table_options",
                      "The option vocabulary and its defaults.",
                      &tests_table_options },
                }
            },
            dt::block_spec{
                "option partition",
                "table_options.hpp (converted from "
                "test_table_option_partition.cpp).",
                {
                    { "tests_table_option_partition",
                      "An option pack partitions into table and container "
                      "options.",
                      &tests_table_option_partition },
                }
            },
            dt::block_spec{
                "model",
                "table_model.hpp (converted from test_table_model.cpp).",
                {
                    { "tests_table_model_compile_time",
                      "The compile-time model, and the builder-to-model arrow.",
                      &tests_table_model_compile_time },
                    { "tests_table_model_runtime",
                      "The runtime value model.",
                      &tests_table_model_runtime },
                }
            },
            dt::block_spec{
                "parser",
                "table_parser.hpp (converted from test_table_parser.cpp).",
                {
                    { "tests_table_parser",
                      "Text tables parse into the runtime model.",
                      &tests_table_parser },
                }
            },
            dt::block_spec{
                "template",
                "table_template.hpp (converted from test_table_template.cpp).",
                {
                    { "tests_table_template",
                      "Table templates and their interpolation.",
                      &tests_table_template },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_TABLE_TESTS_
