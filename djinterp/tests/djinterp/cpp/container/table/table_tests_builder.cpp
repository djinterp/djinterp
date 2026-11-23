/*******************************************************************************
* djinterp [cpp][container]                              table_tests_builder.cpp
*
* table_builder.hpp: the declaration grammar (columns, rows, merges, splits,
* cell wrappers, val_t) and the shape it folds to.
*
*   Converted on 2026.11.16 from test_table_builder.cpp, an assert-based
* program no leaf built: each assertion is one check, in its order.
*
*
* path:      /tests/djinterp/cpp/container/table/table_tests_builder.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.16
*                                                            revised: 2026.11.16
*******************************************************************************/
// std
#include <type_traits>  // std::is_same
// djinterp
#include "table_tests.hpp"  // D_TBL_CHECK, the declarations
#include <string>  // std::string
#include <type_traits>  // std::is_same
#include <djinterp/cpp/container/table/table_builder.hpp>  // table_builder.hpp


NS_DJINTERP
NS_TESTING

// The fixtures of the original program, in its order.  Unnamed, so a
// fixture name reused by another section never breaks the one-definition
// rule.
namespace
{

// std
// djinterp

// cell value tags standing in for the sketch's a1, b1, foo1, ...
struct a1{}; struct b1{}; struct c1{}; struct d1{};
struct a2{}; struct b2{}; struct c2{};
struct w1{}; struct x1{}; struct y1{}; struct z1{};
struct w2{}; struct x2{}; struct y2{}; struct z2{};
struct t1{}; struct t2{}; struct t3{}; struct t4{};

//    table_builder<columns<..4..>, columns<..3..>, columns<..5..>>
using empty4d = DTableBuilder< DColumns<t1, t2, t3, t4>,
                               DColumns<t1, t2, t3>,
                               DColumns<t1, t2, t3, t4, t1> >;

//     (e.g. every 4 elements = 1 row)"
using inferred = DTableBuilder< DColumns<t1, t2, t3, t4>,
                                DRows< a1, b1, c1, d1,
                                      a2, b2, c2, d1,
                                      a1, b1, c1, d1,
                                      a2, b2, c2, d1 > >;

//    mixing row<> and rows<>
using mixed = DTableBuilder< DColumns<t1, t2, t3>,
                             DColumns<t1, t2, t3, t4>,
                             DRow< a1, b1, c1, w1, x1, y1, z1 >,
                             DRow< a2, b2, c2, w2, x2, y2, z2 >,
                             DRows< a1, b1, c1, w1, x1, y1, z1,
                                   a2, b2, c2, w2, x2, y2, z2 > >;

//    row<a2, merged_cell<1,2,b2>, w2, x2, y2, z2>  ==  1+2+1+1+1+1 = 7
using merged = DTableBuilder< DColumns<t1, t2, t3>,
                              DColumns<t1, t2, t3, t4>,
                              DRow< a1, b1, c1, w1, x1, y1, z1 >,
                              DRow< a2, DMergedCell<1, 2, b2>, w2, x2, y2, z2 > >;

// the merge sits at row 1, column 1 and spans 2 columns -> its anchor is (1,1)
// the covered position (1,2) DEFERS to the anchor (1,1)  [T[i] = T[anchor(cell(i))]]
// the cell after the merge resumed at column 3, not column 2
// row 0 is untouched: all singletons

//    row<a4, b4, c4, split_cell<1,3,3,row<w4,x4,y4>>, z4>  == 1+1+1+3+1 = 7
using with_split = DTableBuilder< DColumns<t1, t2, t3>,
                                  DColumns<t1, t2, t3, t4>,
                                  DRow< a1, b1, c1,
                                       DSplitCell<1, 3, 3, DRow<w1, x1, y1>>,
                                       z1 > >;

using with_cells = DTableBuilder< DColumns<t1, t2, t3>,
                                  DRow< cell<a1>, b1, cell<c1> > >;

using homo = DTableBuilder< DColumns<int, int, int>, DRows<a1,b1,c1> >;

using record = DTableBuilder< DColumns<int, std::string, double>, DRows<a1,b1,c1> >;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
using valued = DTableBuilder< DColumns<int, int, int>,
                              DRows< val_t<1>, val_t<2>, val_t<3>,
                                    val_t<4>, val_t<5>, val_t<6> > >;
#endif

}  // namespace


/*
tests_table_builder_empty_4d
  an empty table of three column groups folds at compile time
  Tests the following:
  - each assertion of the original block, as one check, in its order
*/
bool tests_table_builder_empty_4d()
{
    D_TBL_CHECK(empty4d::group_count == 3);  // 3 column groups
    D_TBL_CHECK(empty4d::rank == 4);  // k = groups + 1 = 4
    D_TBL_CHECK(!empty4d::is_rank_2);  // not rank 2
    D_TBL_CHECK(empty4d::width == 12);  // flattened width 4+3+5
    D_TBL_CHECK(empty4d::height == 0);  // no data -> empty
    D_TBL_CHECK(!empty4d::declares_data);  // declares no data
    // columns lead
    D_TBL_CHECK(empty4d::independent_axis == DTableAxis::column_major);

    return true;
}

/*
tests_table_builder_row_inference
  rows are inferred from the columns of a 2d table
  Tests the following:
  - each assertion of the original block, as one check, in its order
*/
bool tests_table_builder_row_inference()
{
    D_TBL_CHECK(inferred::width == 4);  // width 4
    D_TBL_CHECK(inferred::height == 4);  // 16 entries / 4 = 4 rows (inferred)
    D_TBL_CHECK(inferred::rank == 2);  // rank 2
    D_TBL_CHECK(inferred::is_rank_2);  // is rank 2
    D_TBL_CHECK(inferred::declares_data);  // has data
    D_TBL_CHECK(!inferred::has_merges);  // no merges -> Gamma_0

    return true;
}

/*
tests_table_builder_mixed_groups
  two column groups with rows: the 3 x 4 x n declaration
  Tests the following:
  - each assertion of the original block, as one check, in its order
*/
bool tests_table_builder_mixed_groups()
{
    D_TBL_CHECK(mixed::group_count == 2);  // two groups
    D_TBL_CHECK(mixed::rank == 3);  // 3 x 4 x n -> rank 3
    D_TBL_CHECK(mixed::width == 7);  // 3 + 4 = 7 columns
    D_TBL_CHECK(mixed::height == 4);  // 2 row<> + (14 entries / 7 = 2) = 4

    return true;
}

/*
tests_table_builder_merge_advance
  a merge advances the cursor by its span
  Tests the following:
  - each assertion of the original block, as one check, in its order
*/
bool tests_table_builder_merge_advance()
{
    D_TBL_CHECK(merged::width == 7);  // still 7 columns wide
    D_TBL_CHECK(merged::height == 2);  // two rows
    D_TBL_CHECK(merged::has_merges);  // wears a merge
    D_TBL_CHECK(merged::merge_count == 1);  // exactly one merge
    D_TBL_CHECK(merged::owner_of<1,1>::anchor_row == 1);  // anchor row
    D_TBL_CHECK(merged::owner_of<1,1>::anchor_col == 1);  // anchor col
    D_TBL_CHECK(merged::owner_of<1,1>::extent == 2);  // spans 2
    // (1,2) defers to (1,1)
    D_TBL_CHECK(merged::owner_of<1,2>::anchor_col == 1);
    // (1,1) and (1,2) are the same layout cell
    D_TBL_CHECK(std::is_same<merged::owner_of<1,1>,
                merged::owner_of<1,2>>::value);
    // w2 landed at column 3
    D_TBL_CHECK(merged::owner_of<1,3>::anchor_col == 3);
    D_TBL_CHECK(merged::owner_of<0,1>::extent == 1);  // row 0 singleton

    return true;
}

/*
tests_table_builder_split_span
  a split cell spans its box
  Tests the following:
  - each assertion of the original block, as one check, in its order
*/
bool tests_table_builder_split_span()
{
    D_TBL_CHECK(with_split::width == 7);  // split spans 3 -> 7 wide
    D_TBL_CHECK(with_split::height == 1);  // one row
    D_TBL_CHECK(with_split::has_merges);  // the split registers a layout cell
    D_TBL_CHECK(with_split::owner_of<0,3>::extent == 3);  // split spans 3 cols
    // (0,5) defers to (0,3)
    D_TBL_CHECK(with_split::owner_of<0,5>::anchor_col == 3);
    D_TBL_CHECK(with_split::owner_of<0,6>::extent == 1);  // z1 is its own cell

    return true;
}

/*
tests_table_builder_cell_wrapper
  cell<> is accepted and mixes with bare entries
  Tests the following:
  - each assertion of the original block, as one check, in its order
*/
bool tests_table_builder_cell_wrapper()
{
    D_TBL_CHECK(with_cells::width == 3);  // cell<> spans 1
    D_TBL_CHECK(with_cells::height == 1);  // one row
    D_TBL_CHECK(!with_cells::has_merges);  // no merges
    // cell unwraps
    D_TBL_CHECK(std::is_same<cell_value_t<cell<a1>>, a1>::value);
    // merged unwraps
    D_TBL_CHECK(std::is_same<cell_value_t<DMergedCell<1,2,b2>>, b2>::value);
    // bare entry is its own value
    D_TBL_CHECK(std::is_same<cell_value_t<a1>, a1>::value);

    return true;
}

/*
tests_table_builder_realization
  shape and realization: homogeneous against record
  Tests the following:
  - each assertion of the original block, as one check, in its order
*/
bool tests_table_builder_realization()
{
    D_TBL_CHECK(homo::is_homogeneous);  // all int -> homogeneous
    D_TBL_CHECK(std::is_same<homo::element_type, int>::value);  // element int
    // -> static_table
    D_TBL_CHECK(homo::realization == DRealizationKind::static_homogeneous);
    D_TBL_CHECK(!record::is_homogeneous);  // mixed -> heterogeneous
    // -> record_table
    D_TBL_CHECK(record::realization == DRealizationKind::static_record);
    // column types
    D_TBL_CHECK(std::is_same<record::columns_type, std::tuple<int, std::string,
                double>>::value);
    // col 1
    D_TBL_CHECK(std::is_same<record::shape_type::column_type_t<1>,
                std::string>::value);

    return true;
}

/*
tests_table_builder_val_t
  val_t<> lifts a non-type parameter into a cell (C++17)
  Tests the following:
  - each assertion of the original block, as one check, in its order
*/
bool tests_table_builder_val_t()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    D_TBL_CHECK(valued::width == 3);  // 3 wide
    D_TBL_CHECK(valued::height == 2);  // 2 rows inferred
    D_TBL_CHECK(val_t<42>::value == 42);  // val_t carries the value
#endif

    return true;
}

/*
tests_table_builder_detection
  the builder detection traits
  Tests the following:
  - each assertion of the original block, as one check, in its order
*/
bool tests_table_builder_detection()
{
    D_TBL_CHECK(is_table_builder<homo>::value);  // is_table_builder
    D_TBL_CHECK(!is_table_builder<int>::value);  // not a builder
    D_TBL_CHECK(is_columns<DColumns<int>>::value);  // is_columns
    D_TBL_CHECK(is_row<DRow<a1>>::value);  // is_row
    D_TBL_CHECK(is_rows<DRows<a1>>::value);  // is_rows
    D_TBL_CHECK(!is_row<DRows<a1>>::value);  // row != rows

    return true;
}

NS_END  // testing
NS_END  // djinterp
