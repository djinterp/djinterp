/*******************************************************************************
* djinterp [cpp][container]                            table_tests_transpose.cpp
*
* table_builder.hpp: the inverse (row-major) declaration and its fold to the
* column-wise normal form.
*
*   Converted on 2026.11.16 from test_table_transpose.cpp, an assert-based
* program no leaf built: each assertion is one check, in its order.
*
*
* path:      /tests/djinterp/cpp/container/table/table_tests_transpose.cpp
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

struct a1{}; struct b1{}; struct c1{};
struct a2{}; struct b2{}; struct c2{};
struct T1{}; struct T2{}; struct T3{};

// a spanning cell's box turns with the table
// a bare cell is orientation-free
// transposition is an involution: T(T(x)) == x

using colmajor = DTableBuilder< DColumns<T1,T2,T3>, DRow<a1,b1,c1>, DRow<a2,b2,c2> >;

// the SAME table declared inversely: every columns/rows and row/column swapped
using rowmajor = DTableBuilder< DRows<T1,T2,T3>, DColumn<a1,b1,c1>, DColumn<a2,b2,c2> >;
// literally declared as 3 rows x 2 columns ...
// ... and canonically stored as its transpose (2 rows x 3 columns, column-typed)

// KEY: the inverse declaration folds to exactly the same canonical model

// a column-major declaration normalizes to itself

//    column-wise: row<a1, merged_cell<1,2,b1>>  -> a 1x2 merge at (0,1)
//    row-wise:    column<a1, merged_cell<2,1,b1>> -> transposes to the same
using merged_col = DTableBuilder< DColumns<T1,T2,T3>, DRow<a1, DMergedCell<1,2,b1>> >;

using merged_row = DTableBuilder< DRows<T1,T2,T3>, DColumn<a1, DMergedCell<2,1,b1>> >;

using flat_col = DTableBuilder< DColumns<T1,T2>, DRows<a1,b1, a2,b2, a1,b1> >;

using flat_row = DTableBuilder< DRows<T1,T2>, DColumns<a1,b1, a2,b2, a1,b1> >;

using merged7 = DTableBuilder< DColumns<T1,T2,T3>, DColumns<T1,T2,T3,T1>,
                               DRow< a1,b1,c1, a2,b2,c2,a1 >,
                               DRow< a2, DMergedCell<1,2,b2>, a1,b1,c1,a2 > >;

}  // namespace


/*
tests_table_transpose_decl
  transpose_decl swaps the declarators
  Tests the following:
  - each assertion of the original block, as one check, in its order
*/
bool tests_table_transpose_decl()
{
    // cols->rows
    D_TBL_CHECK(std::is_same<transpose_decl_t<DColumns<T1,T2>>,
                DRows<T1,T2>>::value);
    // rows->cols
    D_TBL_CHECK(std::is_same<transpose_decl_t<DRows<T1,T2>>,
                DColumns<T1,T2>>::value);
    // row->column
    D_TBL_CHECK(std::is_same<transpose_decl_t<DRow<a1,b1>>,
                DColumn<a1,b1>>::value);
    // column->row
    D_TBL_CHECK(std::is_same<transpose_decl_t<DColumn<a1,b1>>,
                DRow<a1,b1>>::value);
    // merge box swaps
    D_TBL_CHECK(std::is_same<transpose_decl_t<DMergedCell<1,3,b1>>,
                DMergedCell<3,1,b1>>::value);
    // split box + content swap
    D_TBL_CHECK(std::is_same<transpose_decl_t<DSplitCell<1,3,3,DRow<a1,b1,c1>>>,
                DSplitCell<3,1,3,DColumn<a1,b1,c1>>>::value);
    // bare cell unchanged
    D_TBL_CHECK(std::is_same<transpose_decl_t<a1>, a1>::value);
    // involution
    D_TBL_CHECK(std::is_same<transpose_decl_t<transpose_decl_t<DRow<a1,
                DMergedCell<1,2,b1>>>>, DRow<a1, DMergedCell<1,2,b1>>>::value);

    return true;
}

/*
tests_table_transpose_leading_axis
  the leading declarator picks the independent axis
  Tests the following:
  - each assertion of the original block, as one check, in its order
*/
bool tests_table_transpose_leading_axis()
{
    // columns lead
    D_TBL_CHECK(colmajor::independent_axis == DTableAxis::column_major);
    D_TBL_CHECK(colmajor::width == 3);  // 3 cols
    D_TBL_CHECK(colmajor::height == 2);  // 2 rows
    D_TBL_CHECK(colmajor::declared_width == 3);  // declared 3 wide
    D_TBL_CHECK(colmajor::declared_height == 2);  // declared 2 tall
    // rows lead
    D_TBL_CHECK(rowmajor::independent_axis == DTableAxis::row_major);
    D_TBL_CHECK(rowmajor::declared_height == 3);  // declared 3 tall
    D_TBL_CHECK(rowmajor::declared_width == 2);  // declared 2 wide
    D_TBL_CHECK(rowmajor::width == 3);  // canonical 3 cols
    D_TBL_CHECK(rowmajor::height == 2);  // canonical 2 rows
    // inverse declaration -> same canonical shape
    D_TBL_CHECK(std::is_same<colmajor::shape_type,
                rowmajor::shape_type>::value);
    // inverse declaration -> same canonical layout
    D_TBL_CHECK(std::is_same<colmajor::layout_type,
                rowmajor::layout_type>::value);

    return true;
}

/*
tests_table_transpose_normalized
  the normal form is the literal column-wise rewrite
  Tests the following:
  - each assertion of the original block, as one check, in its order
*/
bool tests_table_transpose_normalized()
{
    // row-major normalizes to the column-wise declaration
    D_TBL_CHECK(std::is_same<rowmajor::normalized_decls_type,
                type_list< DColumns<T1,T2,T3>, DRow<a1,b1,c1>, DRow<a2,b2,c2> >>::value);
    // column-major normalizes to itself
    D_TBL_CHECK(std::is_same<colmajor::normalized_decls_type,
                type_list< DColumns<T1,T2,T3>, DRow<a1,b1,c1>, DRow<a2,b2,c2> >>::value);

    return true;
}

/*
tests_table_transpose_merge
  a merge survives the inverse declaration, its box turned
  Tests the following:
  - each assertion of the original block, as one check, in its order
*/
bool tests_table_transpose_merge()
{
    D_TBL_CHECK(merged_col::has_merges);  // has merge
    D_TBL_CHECK(merged_col::owner_of<0,1>::extent == 2);  // spans 2
    // defers to anchor
    D_TBL_CHECK(merged_col::owner_of<0,2>::anchor_col == 1);
    // the merge transposes into the same canonical cover
    D_TBL_CHECK(std::is_same<merged_col::layout_type,
                merged_row::layout_type>::value);
    // same canonical shape
    D_TBL_CHECK(std::is_same<merged_col::shape_type,
                merged_row::shape_type>::value);

    return true;
}

/*
tests_table_transpose_flat_data
  a non-leading columns<> declared inversely is flat data
  Tests the following:
  - each assertion of the original block, as one check, in its order
*/
bool tests_table_transpose_flat_data()
{
    D_TBL_CHECK(flat_col::width == 2 && flat_col::height == 3);  // 3 rows of 2
    // inverse flat declaration -> same canonical shape
    D_TBL_CHECK(std::is_same<flat_col::shape_type,
                flat_row::shape_type>::value);
    // declared as 2 rows x 3 cols
    D_TBL_CHECK(flat_row::declared_height == 2 && flat_row::declared_width
                == 3);

    return true;
}

/*
tests_table_transpose_regression
  the earlier column-major suite still folds identically
  Tests the following:
  - each assertion of the original block, as one check, in its order
*/
bool tests_table_transpose_regression()
{
    D_TBL_CHECK(merged7::width == 7 && merged7::height == 2);  // 7 wide, 2 tall
    D_TBL_CHECK(merged7::rank == 3);  // two groups -> rank 3
    // (1,2) defers to (1,1)
    D_TBL_CHECK(merged7::owner_of<1,2>::anchor_col == 1);
    D_TBL_CHECK(merged7::owner_of<1,3>::anchor_col == 3);  // resumes at col 3

    return true;
}

NS_END  // testing
NS_END  // djinterp
