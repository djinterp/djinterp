/*******************************************************************************
* djinterp [cpp][container]                                table_tests_shape.cpp
*
* table_shape.hpp: the type-level shape.
*
*   Converted on 2026.11.16 from test_table_shape.cpp, an assert-based
* program no leaf built: each assertion is one check, in its order.
*
*
* path:      /tests/djinterp/cpp/container/table/table_tests_shape.cpp
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
#include <djinterp/cpp/container/table/table_shape.hpp>  // table_shape.hpp


NS_DJINTERP
NS_TESTING

// The fixtures of the original program, in its order.  Unnamed, so a
// fixture name reused by another section never breaks the one-definition
// rule.
namespace
{

// std
// djinterp

// --- a static homogeneous grid: 3 x 3 of int ---
using grid = DTableShape<3, int, int, int>;

// --- a dynamic heterogeneous record: n x 3 (int, string, double) ---
using rec = DTableShape<dynamic_extent, int, std::string, double>;

// --- the other two realization corners ---
// n x 2 double
using dyn_homo   = DTableShape<dynamic_extent, double, double>;
// 4 x 2 record
using stat_rec   = DTableShape<4, int, char>;

// --- width-0 edge case: vacuously homogeneous, element void ---
using empty0 = DTableShape<0>;

// --- single-column shapes are homogeneous ---
using one_col = DTableShape<5, int>;

// --- detection ---

}  // namespace


/*
tests_table_shape
  the type-level shape and its queries
  Tests the following:
  - each assertion of the original block, as one check, in its order
*/
bool tests_table_shape()
{
    D_TBL_CHECK(grid::rank == 2);  // rank
    D_TBL_CHECK(grid::width == 3);  // width
    D_TBL_CHECK(grid::height == 3);  // height
    D_TBL_CHECK(grid::static_height);  // static height
    D_TBL_CHECK(grid::is_homogeneous);  // homogeneous
    D_TBL_CHECK(std::is_same<grid::element_type, int>::value);  // element int
    D_TBL_CHECK(std::is_same<grid::column_type_t<2>, int>::value);  // col2 int
    // grid->static_homo
    D_TBL_CHECK(realization_of<grid>::value
                == DRealizationKind::static_homogeneous);
    D_TBL_CHECK(rec::width == 3);  // rec width
    D_TBL_CHECK(rec::height == dynamic_extent);  // rec dynamic
    D_TBL_CHECK(!rec::static_height);  // rec not static
    D_TBL_CHECK(!rec::is_homogeneous);  // rec heterogeneous
    D_TBL_CHECK(std::is_same<rec::column_type_t<0>, int>::value);  // rec col0
    // rec col1
    D_TBL_CHECK(std::is_same<rec::column_type_t<1>, std::string>::value);
    // rec col2
    D_TBL_CHECK(std::is_same<rec::column_type_t<2>, double>::value);
    // rec cols tuple
    D_TBL_CHECK(std::is_same<rec::columns_type,
                std::tuple<int,std::string,double>>::value);
    // rec->dyn_record
    D_TBL_CHECK(realization_of<rec>::value == DRealizationKind::dynamic_record);
    // dyn homo
    D_TBL_CHECK(realization_of<dyn_homo>::value
                == DRealizationKind::dynamic_homogeneous);
    // stat record
    D_TBL_CHECK(realization_of<stat_rec>::value
                == DRealizationKind::static_record);
    D_TBL_CHECK(empty0::width == 0);  // empty width 0
    D_TBL_CHECK(empty0::is_homogeneous);  // empty vacuously homo
    // empty element void
    D_TBL_CHECK(std::is_same<empty0::element_type, void>::value);
    // empty static homo
    D_TBL_CHECK(realization_of<empty0>::value
                == DRealizationKind::static_homogeneous);
    D_TBL_CHECK(one_col::is_homogeneous);  // single col homo
    // single col elem
    D_TBL_CHECK(std::is_same<one_col::element_type, int>::value);
    D_TBL_CHECK(is_table_shape<grid>::value);  // is_shape grid
    D_TBL_CHECK(!is_table_shape<int>::value);  // is_shape not int
    D_TBL_CHECK(is_table_shape<const rec&>::value);  // is_shape strips cvref

    return true;
}

NS_END  // testing
NS_END  // djinterp
