/*******************************************************************************
* djinterp [core]                                       test_table_transpose.cpp
*
*
* path:      /tests/djinterp/core/container/table/test_table_transpose.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <string>
#include <type_traits>
// djinterp
#include "djinterp/core/container/table/table_builder.hpp"
using namespace djinterp;

struct a1{}; struct b1{}; struct c1{};
struct a2{}; struct b2{}; struct c2{};
struct T1{}; struct T2{}; struct T3{};

// ---------------------------------------------------------------------------
// 1. transpose_decl: the declarator-level swap
// ---------------------------------------------------------------------------
static_assert(std::is_same<transpose_decl_t<columns<T1,T2>>, rows<T1,T2>>::value,   "cols->rows");
static_assert(std::is_same<transpose_decl_t<rows<T1,T2>>, columns<T1,T2>>::value,   "rows->cols");
static_assert(std::is_same<transpose_decl_t<row<a1,b1>>, column<a1,b1>>::value,     "row->column");
static_assert(std::is_same<transpose_decl_t<column<a1,b1>>, row<a1,b1>>::value,     "column->row");
// a spanning cell's box turns with the table
static_assert(std::is_same<transpose_decl_t<merged_cell<1,3,b1>>,
                           merged_cell<3,1,b1>>::value, "merge box swaps");
static_assert(std::is_same<transpose_decl_t<split_cell<1,3,3,row<a1,b1,c1>>>,
                           split_cell<3,1,3,column<a1,b1,c1>>>::value, "split box + content swap");
// a bare cell is orientation-free
static_assert(std::is_same<transpose_decl_t<a1>, a1>::value, "bare cell unchanged");
// transposition is an involution: T(T(x)) == x
static_assert(std::is_same<transpose_decl_t<transpose_decl_t<row<a1, merged_cell<1,2,b1>>>>,
                           row<a1, merged_cell<1,2,b1>>>::value, "involution");

// ---------------------------------------------------------------------------
// 2. leading declarator picks the independent axis
// ---------------------------------------------------------------------------
using colmajor = table_builder< columns<T1,T2,T3>, row<a1,b1,c1>, row<a2,b2,c2> >;
static_assert(colmajor::independent_axis == table_axis::column_major, "columns lead");
static_assert(colmajor::width  == 3, "3 cols");
static_assert(colmajor::height == 2, "2 rows");
static_assert(colmajor::declared_width  == 3, "declared 3 wide");
static_assert(colmajor::declared_height == 2, "declared 2 tall");

// the SAME table declared inversely: every columns/rows and row/column swapped
using rowmajor = table_builder< rows<T1,T2,T3>, column<a1,b1,c1>, column<a2,b2,c2> >;
static_assert(rowmajor::independent_axis == table_axis::row_major, "rows lead");
// literally declared as 3 rows x 2 columns ...
static_assert(rowmajor::declared_height == 3, "declared 3 tall");
static_assert(rowmajor::declared_width  == 2, "declared 2 wide");
// ... and canonically stored as its transpose (2 rows x 3 columns, column-typed)
static_assert(rowmajor::width  == 3, "canonical 3 cols");
static_assert(rowmajor::height == 2, "canonical 2 rows");

// KEY: the inverse declaration folds to exactly the same canonical model
static_assert(std::is_same<colmajor::shape_type, rowmajor::shape_type>::value,
              "inverse declaration -> same canonical shape");
static_assert(std::is_same<colmajor::layout_type, rowmajor::layout_type>::value,
              "inverse declaration -> same canonical layout");

// ---------------------------------------------------------------------------
// 3. the normalized declaration is the literal column-wise rewrite
// ---------------------------------------------------------------------------
static_assert(std::is_same<rowmajor::normalized_decls_type,
                           type_list< columns<T1,T2,T3>,
                                      row<a1,b1,c1>,
                                      row<a2,b2,c2> >>::value,
              "row-major normalizes to the column-wise declaration");
// a column-major declaration normalizes to itself
static_assert(std::is_same<colmajor::normalized_decls_type,
                           type_list< columns<T1,T2,T3>,
                                      row<a1,b1,c1>,
                                      row<a2,b2,c2> >>::value,
              "column-major normalizes to itself");

// ---------------------------------------------------------------------------
// 4. a merge survives the inverse declaration, with its box turned
//    column-wise: row<a1, merged_cell<1,2,b1>>  -> a 1x2 merge at (0,1)
//    row-wise:    column<a1, merged_cell<2,1,b1>> -> transposes to the same
// ---------------------------------------------------------------------------
using merged_col = table_builder< columns<T1,T2,T3>, row<a1, merged_cell<1,2,b1>> >;
static_assert(merged_col::has_merges,               "has merge");
static_assert(merged_col::owner_of<0,1>::extent == 2, "spans 2");
static_assert(merged_col::owner_of<0,2>::anchor_col == 1, "defers to anchor");

using merged_row = table_builder< rows<T1,T2,T3>, column<a1, merged_cell<2,1,b1>> >;
static_assert(std::is_same<merged_col::layout_type, merged_row::layout_type>::value,
              "the merge transposes into the same canonical cover");
static_assert(std::is_same<merged_col::shape_type, merged_row::shape_type>::value,
              "same canonical shape");

// ---------------------------------------------------------------------------
// 5. flat data declared inversely (a non-leading columns<> is flat data)
// ---------------------------------------------------------------------------
using flat_col = table_builder< columns<T1,T2>, rows<a1,b1, a2,b2, a1,b1> >;
static_assert(flat_col::width == 2 && flat_col::height == 3, "3 rows of 2");

using flat_row = table_builder< rows<T1,T2>, columns<a1,b1, a2,b2, a1,b1> >;
static_assert(std::is_same<flat_col::shape_type, flat_row::shape_type>::value,
              "inverse flat declaration -> same canonical shape");
static_assert(flat_row::declared_height == 2 && flat_row::declared_width == 3,
              "declared as 2 rows x 3 cols");

// ---------------------------------------------------------------------------
// 6. regression: the earlier column-major suite still folds identically
// ---------------------------------------------------------------------------
using merged7 = table_builder< columns<T1,T2,T3>, columns<T1,T2,T3,T1>,
                               row< a1,b1,c1, a2,b2,c2,a1 >,
                               row< a2, merged_cell<1,2,b2>, a1,b1,c1,a2 > >;
static_assert(merged7::width == 7 && merged7::height == 2, "7 wide, 2 tall");
static_assert(merged7::rank == 3,                       "two groups -> rank 3");
static_assert(merged7::owner_of<1,2>::anchor_col == 1,  "(1,2) defers to (1,1)");
static_assert(merged7::owner_of<1,3>::anchor_col == 3,  "resumes at col 3");

int main() { return 0; }
