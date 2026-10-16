/*******************************************************************************
* djinterp [core]                                           test_table_shape.cpp
*
*
* path:      /tests/djinterp/core/container/table/test_table_shape.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <string>
#include <type_traits>
// djinterp
#include "djinterp/core/container/table/table_shape.hpp"
using namespace djinterp;

// --- a static homogeneous grid: 3 x 3 of int ---
using grid = table_shape<3, int, int, int>;
static_assert(grid::rank          == 2,     "rank");
static_assert(grid::width         == 3,     "width");
static_assert(grid::height        == 3,     "height");
static_assert(grid::static_height,          "static height");
static_assert(grid::is_homogeneous,         "homogeneous");
static_assert(std::is_same<grid::element_type, int>::value, "element int");
static_assert(std::is_same<grid::column_type_t<2>, int>::value, "col2 int");
static_assert(realization_of<grid>::value == realization_kind::static_homogeneous, "grid->static_homo");

// --- a dynamic heterogeneous record: n x 3 (int, string, double) ---
using rec = table_shape<dynamic_extent, int, std::string, double>;
static_assert(rec::width  == 3,             "rec width");
static_assert(rec::height == dynamic_extent,"rec dynamic");
static_assert(!rec::static_height,          "rec not static");
static_assert(!rec::is_homogeneous,         "rec heterogeneous");
static_assert(std::is_same<rec::column_type_t<0>, int>::value,         "rec col0");
static_assert(std::is_same<rec::column_type_t<1>, std::string>::value, "rec col1");
static_assert(std::is_same<rec::column_type_t<2>, double>::value,      "rec col2");
static_assert(std::is_same<rec::columns_type, std::tuple<int,std::string,double>>::value, "rec cols tuple");
static_assert(realization_of<rec>::value == realization_kind::dynamic_record, "rec->dyn_record");

// --- the other two realization corners ---
using dyn_homo   = table_shape<dynamic_extent, double, double>;   // n x 2 double
using stat_rec   = table_shape<4, int, char>;                     // 4 x 2 record
static_assert(realization_of<dyn_homo>::value == realization_kind::dynamic_homogeneous, "dyn homo");
static_assert(realization_of<stat_rec>::value == realization_kind::static_record,       "stat record");

// --- width-0 edge case: vacuously homogeneous, element void ---
using empty0 = table_shape<0>;
static_assert(empty0::width == 0,           "empty width 0");
static_assert(empty0::is_homogeneous,       "empty vacuously homo");
static_assert(std::is_same<empty0::element_type, void>::value, "empty element void");
static_assert(realization_of<empty0>::value == realization_kind::static_homogeneous, "empty static homo");

// --- single-column shapes are homogeneous ---
using one_col = table_shape<5, int>;
static_assert(one_col::is_homogeneous,      "single col homo");
static_assert(std::is_same<one_col::element_type, int>::value, "single col elem");

// --- detection ---
static_assert(is_table_shape<grid>::value,  "is_shape grid");
static_assert(!is_table_shape<int>::value,  "is_shape not int");
static_assert(is_table_shape<const rec&>::value, "is_shape strips cvref");

int main() { return 0; }
