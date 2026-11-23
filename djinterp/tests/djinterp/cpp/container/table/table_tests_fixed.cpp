/*******************************************************************************
* djinterp [cpp][container]                                table_tests_fixed.cpp
*
* fixed_table.hpp: the fixed-shape, mutable rank-2 table - construction,
* the static extents, mutation, the algorithms, equality, the serial pair and
* detection.
*
*
* path:      /tests/djinterp/cpp/container/table/table_tests_fixed.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.16
*                                                            revised: 2026.11.16
*******************************************************************************/
// std
#include <array>        // std::array
#include <cstddef>      // std::size_t
#include <stdexcept>    // std::out_of_range
#include <string>       // std::string
#include <type_traits>  // std::is_same
#include <vector>       // std::vector
// djinterp
#include "table_tests.hpp"  // D_TBL_CHECK, the declarations
#include <djinterp/cpp/container/table/fixed_table.hpp>  // under test


NS_DJINTERP
NS_TESTING

namespace
{

using tb_fixed = DFixedTable<int, 2, 3>;
using tb_frow  = internal::basic_row_view<const int>;

}  // namespace


/*
tests_fixed_table_construction
  every way to make a fixed table
  Tests the following:
  - default cells are value-initialised
  - the variadic constructor takes exactly Rows * Cols cells, row-major
  - the std::array constructor, filled() and make_fixed_table
  - a constexpr fixed table reads at compile time
*/
bool tests_fixed_table_construction()
{
    tb_fixed zero;
    D_TBL_CHECK(zero.count(0) == 6);

    tb_fixed cells(1, 2, 3, 4, 5, 6);
    D_TBL_CHECK(cells(0, 2) == 3);
    D_TBL_CHECK(cells(1, 0) == 4);

    tb_fixed from_array(std::array<int, 6>{ { 6, 5, 4, 3, 2, 1 } });
    D_TBL_CHECK(from_array(0, 0) == 6);

    tb_fixed sevens = tb_fixed::filled(7);
    D_TBL_CHECK(sevens.count(7) == 6);

    auto made = make_fixed_table<2, 2>(1, 2, 3, 4);
    D_TBL_CHECK((std::is_same<decltype(made), DFixedTable<int, 2, 2>>::value));
    D_TBL_CHECK(made(1, 1) == 4);

    constexpr DFixedTable<int, 2, 2> ce(1, 2, 3, 4);
    constexpr int corner = ce(1, 1);
    D_TBL_CHECK(corner == 4);

    return true;
}

/*
tests_fixed_table_extents
  the shape is the type's, known before any object exists
  Tests the following:
  - rows, cols, capacity and max_size are static and constexpr
  - extent, row_extent, column_extent, max_cells
  - a fixed table is always full; it is compile-time iterable, not valued
*/
bool tests_fixed_table_extents()
{
    D_TBL_CHECK(tb_fixed::rows() == 2);
    D_TBL_CHECK(tb_fixed::cols() == 3);
    D_TBL_CHECK(tb_fixed::capacity() == 6);
    D_TBL_CHECK(tb_fixed::max_size() == 6);
    D_TBL_CHECK(tb_fixed::extent == 6);
    D_TBL_CHECK(tb_fixed::max_cells == 6);
    D_TBL_CHECK(tb_fixed::row_extent == 2);
    D_TBL_CHECK(tb_fixed::column_extent == 3);
    D_TBL_CHECK(tb_fixed::compile_time_iterable);
    D_TBL_CHECK(!tb_fixed::compile_time_values);
    D_TBL_CHECK(tb_fixed::lifetime == DContainerLifetime::mutable_storage);

    tb_fixed t;
    D_TBL_CHECK(t.full());
    D_TBL_CHECK(t.size() == 6);
    D_TBL_CHECK(t.row_count() == 2);

    return true;
}

/*
tests_fixed_table_mutation
  writable access, fill and swap
  Tests the following:
  - operator(), at, row, column and the iterators write
  - at() throws std::out_of_range off the box
  - fill sets every cell; member and free swap exchange contents
*/
bool tests_fixed_table_mutation()
{
    tb_fixed t;
    t(0, 0) = 1;
    t.at(0, 1) = 2;
    t.row(0)[2] = 3;
    t[1][0] = 4;
    t.column(1)[1] = 5;
    *(t.end() - 1) = 6;
    D_TBL_CHECK(std::vector<int>(t.begin(), t.end()) ==
                std::vector<int>({ 1, 2, 3, 4, 5, 6 }));
    (*(t.row_begin() + 1))[0] = 40;
    D_TBL_CHECK(t(1, 0) == 40);

    D_TBL_CHECK(tbl_throws<std::out_of_range>([&t] { (void)t.at(2, 0); }));

    tb_fixed u = tb_fixed::filled(9);
    t.swap(u);
    D_TBL_CHECK(t.count(9) == 6);
    D_TBL_CHECK(u(1, 0) == 40);
    swap(t, u);
    D_TBL_CHECK(t(1, 0) == 40);

    t.fill(0);
    D_TBL_CHECK(t.count(0) == 6);

    return true;
}

/*
tests_fixed_table_algorithms
  sort_rows, map and map_inplace
  Tests the following:
  - sort_rows reorders whole rows by the comparator
  - map keeps the shape and changes the cell type; map_inplace overwrites
    each cell with its image
  - the read-only surface (count, is_row_sorted) sees the result
*/
bool tests_fixed_table_algorithms()
{
    DFixedTable<int, 3, 2> t(3, 30, 1, 10, 2, 20);

    t.sort_rows([](tb_frow _a, tb_frow _b) { return (_a[0] < _b[0]); });
    D_TBL_CHECK(std::vector<int>(t.begin(), t.end()) ==
                std::vector<int>({ 1, 10, 2, 20, 3, 30 }));

    auto s = t.map([](const int& _x) { return std::to_string(_x); });
    D_TBL_CHECK((std::is_same<decltype(s), DFixedTable<std::string, 3,
                2>>::value));
    D_TBL_CHECK(s(2, 1) == "30");

    t.map_inplace([](const int& _x) { return (_x + 1); });
    D_TBL_CHECK(t(0, 0) == 2);

    D_TBL_CHECK(t.count(21) == 1);
    D_TBL_CHECK(t.is_row_sorted([](tb_frow _a,
                tb_frow _b) { return (_a[0] < _b[0]); }));

    return true;
}

/*
tests_fixed_table_serial
  equality, the serial pair, and detection
  Tests the following:
  - == and != compare cells
  - encode_into / decode round-trip; a stream of another shape fails
  - is_fixed_table holds for fixed tables only
*/
bool tests_fixed_table_serial()
{
    const tb_fixed a(1, 2, 3, 4, 5, 6);
    const tb_fixed b(1, 2, 3, 4, 5, 6);
    const tb_fixed c(1, 2, 3, 4, 5, 7);
    D_TBL_CHECK(a == b);
    D_TBL_CHECK(a != c);

    DByteString bytes;
    a.encode_into(bytes);
    DByteReader reader(bytes);
    DDecodeResult<tb_fixed> back = tb_fixed::decode(reader);
    D_TBL_CHECK(back.ok);
    D_TBL_CHECK(back.value == a);

    DByteReader other(bytes);
    D_TBL_CHECK(!(DFixedTable<int, 3, 2>::decode(other).ok));

    D_TBL_CHECK(is_fixed_table<tb_fixed>::value);
    D_TBL_CHECK(is_fixed_table_v<const tb_fixed&>);
    D_TBL_CHECK(!is_fixed_table<int>::value);

    return true;
}

NS_END  // testing
NS_END  // djinterp
