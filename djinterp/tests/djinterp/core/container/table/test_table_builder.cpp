/*******************************************************************************
* djinterp [core]                                         test_table_builder.cpp
*
*
* path:      /tests/djinterp/core/container/table/test_table_builder.cpp
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

// cell value tags standing in for the sketch's a1, b1, foo1, ...
struct a1{}; struct b1{}; struct c1{}; struct d1{};
struct a2{}; struct b2{}; struct c2{};
struct w1{}; struct x1{}; struct y1{}; struct z1{};
struct w2{}; struct x2{}; struct y2{}; struct z2{};
struct t1{}; struct t2{}; struct t3{}; struct t4{};

// ---------------------------------------------------------------------------
// 1. the "4d empty table, compile-time expressible" sketch line
//    table_builder<columns<..4..>, columns<..3..>, columns<..5..>>
// ---------------------------------------------------------------------------
using empty4d = table_builder< columns<t1, t2, t3, t4>,
                               columns<t1, t2, t3>,
                               columns<t1, t2, t3, t4, t1> >;
static_assert(empty4d::group_count == 3,   "3 column groups");
static_assert(empty4d::rank == 4,          "k = groups + 1 = 4");
static_assert(!empty4d::is_rank_2,         "not rank 2");
static_assert(empty4d::width  == 12,       "flattened width 4+3+5");
static_assert(empty4d::height == 0,        "no data -> empty");
static_assert(!empty4d::declares_data,     "declares no data");
static_assert(empty4d::independent_axis == table_axis::column_major, "columns lead");

// ---------------------------------------------------------------------------
// 2. "2d table with rows. infers individual rows based on columns
//     (e.g. every 4 elements = 1 row)"
// ---------------------------------------------------------------------------
using inferred = table_builder< columns<t1, t2, t3, t4>,
                                rows< a1, b1, c1, d1,
                                      a2, b2, c2, d1,
                                      a1, b1, c1, d1,
                                      a2, b2, c2, d1 > >;
static_assert(inferred::width  == 4,  "width 4");
static_assert(inferred::height == 4,  "16 entries / 4 = 4 rows (inferred)");
static_assert(inferred::rank   == 2,  "rank 2");
static_assert(inferred::is_rank_2,    "is rank 2");
static_assert(inferred::declares_data, "has data");
static_assert(!inferred::has_merges,  "no merges -> Gamma_0");

// ---------------------------------------------------------------------------
// 3. the 3 x 4 x n mixed declaration: two column groups (3 + 4 = 7 wide),
//    mixing row<> and rows<>
// ---------------------------------------------------------------------------
using mixed = table_builder< columns<t1, t2, t3>,
                             columns<t1, t2, t3, t4>,
                             row< a1, b1, c1, w1, x1, y1, z1 >,
                             row< a2, b2, c2, w2, x2, y2, z2 >,
                             rows< a1, b1, c1, w1, x1, y1, z1,
                                   a2, b2, c2, w2, x2, y2, z2 > >;
static_assert(mixed::group_count == 2,  "two groups");
static_assert(mixed::rank   == 3,       "3 x 4 x n -> rank 3");
static_assert(mixed::width  == 7,       "3 + 4 = 7 columns");
static_assert(mixed::height == 4,       "2 row<> + (14 entries / 7 = 2) = 4");

// ---------------------------------------------------------------------------
// 4. THE KEY CASE: a merge advances the cursor by its span
//    row<a2, merged_cell<1,2,b2>, w2, x2, y2, z2>  ==  1+2+1+1+1+1 = 7
// ---------------------------------------------------------------------------
using merged = table_builder< columns<t1, t2, t3>,
                              columns<t1, t2, t3, t4>,
                              row< a1, b1, c1, w1, x1, y1, z1 >,
                              row< a2, merged_cell<1, 2, b2>, w2, x2, y2, z2 > >;
static_assert(merged::width  == 7,   "still 7 columns wide");
static_assert(merged::height == 2,   "two rows");
static_assert(merged::has_merges,    "wears a merge");
static_assert(merged::merge_count == 1, "exactly one merge");

// the merge sits at row 1, column 1 and spans 2 columns -> its anchor is (1,1)
static_assert(merged::owner_of<1,1>::anchor_row == 1, "anchor row");
static_assert(merged::owner_of<1,1>::anchor_col == 1, "anchor col");
static_assert(merged::owner_of<1,1>::extent == 2,     "spans 2");
// the covered position (1,2) DEFERS to the anchor (1,1)  [T[i] = T[anchor(cell(i))]]
static_assert(merged::owner_of<1,2>::anchor_col == 1, "(1,2) defers to (1,1)");
static_assert(std::is_same<merged::owner_of<1,1>, merged::owner_of<1,2>>::value,
              "(1,1) and (1,2) are the same layout cell");
// the cell after the merge resumed at column 3, not column 2
static_assert(merged::owner_of<1,3>::anchor_col == 3, "w2 landed at column 3");
// row 0 is untouched: all singletons
static_assert(merged::owner_of<0,1>::extent == 1,     "row 0 singleton");

// ---------------------------------------------------------------------------
// 5. a split_cell also spans its box
//    row<a4, b4, c4, split_cell<1,3,3,row<w4,x4,y4>>, z4>  == 1+1+1+3+1 = 7
// ---------------------------------------------------------------------------
using with_split = table_builder< columns<t1, t2, t3>,
                                  columns<t1, t2, t3, t4>,
                                  row< a1, b1, c1,
                                       split_cell<1, 3, 3, row<w1, x1, y1>>,
                                       z1 > >;
static_assert(with_split::width  == 7,  "split spans 3 -> 7 wide");
static_assert(with_split::height == 1,  "one row");
static_assert(with_split::has_merges,   "the split registers a layout cell");
static_assert(with_split::owner_of<0,3>::extent == 3, "split spans 3 cols");
static_assert(with_split::owner_of<0,5>::anchor_col == 3, "(0,5) defers to (0,3)");
static_assert(with_split::owner_of<0,6>::extent == 1,  "z1 is its own cell");

// ---------------------------------------------------------------------------
// 6. cell<> is superfluous but accepted, and mixes with bare entries
// ---------------------------------------------------------------------------
using with_cells = table_builder< columns<t1, t2, t3>,
                                  row< cell<a1>, b1, cell<c1> > >;
static_assert(with_cells::width  == 3, "cell<> spans 1");
static_assert(with_cells::height == 1, "one row");
static_assert(!with_cells::has_merges, "no merges");
static_assert(std::is_same<cell_value_t<cell<a1>>, a1>::value, "cell unwraps");
static_assert(std::is_same<cell_value_t<merged_cell<1,2,b2>>, b2>::value, "merged unwraps");
static_assert(std::is_same<cell_value_t<a1>, a1>::value, "bare entry is its own value");

// ---------------------------------------------------------------------------
// 7. shape + realization: homogeneous vs record
// ---------------------------------------------------------------------------
using homo = table_builder< columns<int, int, int>, rows<a1,b1,c1> >;
static_assert(homo::is_homogeneous, "all int -> homogeneous");
static_assert(std::is_same<homo::element_type, int>::value, "element int");
static_assert(homo::realization == realization_kind::static_homogeneous, "-> static_table");

using record = table_builder< columns<int, std::string, double>, rows<a1,b1,c1> >;
static_assert(!record::is_homogeneous, "mixed -> heterogeneous");
static_assert(record::realization == realization_kind::static_record, "-> record_table");
static_assert(std::is_same<record::columns_type,
                           std::tuple<int, std::string, double>>::value, "column types");
static_assert(std::is_same<record::shape_type::column_type_t<1>, std::string>::value, "col 1");

// ---------------------------------------------------------------------------
// 8. val<> lifts an NTTP into the type-level cell vocabulary (C++17)
// ---------------------------------------------------------------------------
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
using valued = table_builder< columns<int, int, int>,
                              rows< val<1>, val<2>, val<3>,
                                    val<4>, val<5>, val<6> > >;
static_assert(valued::width  == 3, "3 wide");
static_assert(valued::height == 2, "2 rows inferred");
static_assert(val<42>::value == 42, "val carries the value");
#endif

// ---------------------------------------------------------------------------
// 9. detection
// ---------------------------------------------------------------------------
static_assert(is_table_builder<homo>::value,     "is_table_builder");
static_assert(!is_table_builder<int>::value,     "not a builder");
static_assert(is_columns<columns<int>>::value,   "is_columns");
static_assert(is_row<row<a1>>::value,            "is_row");
static_assert(is_rows<rows<a1>>::value,          "is_rows");
static_assert(!is_row<rows<a1>>::value,          "row != rows");

int main() { return 0; }
