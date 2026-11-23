/*******************************************************************************
* djinterp [cpp][container]                               table_tests_static.cpp
*
* static_table.hpp: the compile-time, read-only rank-2 table --
* construction, constant evaluation, map, equality, the serial pair and
* detection.
*
*
* path:      /tests/djinterp/cpp/container/table/table_tests_static.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.16
*                                                            revised: 2026.11.16
*******************************************************************************/
// std
#include <array>        // std::array
#include <string>       // std::string
#include <type_traits>  // std::is_same
#include <vector>       // std::vector
// djinterp
#include "table_tests.hpp"  // D_TBL_CHECK, the declarations
#include <djinterp/cpp/container/table/static_table.hpp>  // under test


NS_DJINTERP
NS_TESTING

namespace
{

using tb_static = DStaticTable<int, 2, 3>;

// tb_doubled
//   function: a constexpr cell map for static_table::map.
constexpr int tb_doubled(const int& _x)
{
    return (2 * _x);
}

}  // namespace


/*
tests_static_table_construction
  every way to make a static table, and its read-only face
  Tests the following:
  - default cells are value-initialised; the variadic and std::array
    constructors; filled() and make_static_table
  - the extents are static; it is full, compile-time iterable and valued
  - every iterator is const: the cells cannot be written through them
*/
bool tests_static_table_construction()
{
    const tb_static zero;
    D_TBL_CHECK(zero.count(0) == 6);

    const tb_static cells(1, 2, 3, 4, 5, 6);
    D_TBL_CHECK(cells(1, 2) == 6);
    D_TBL_CHECK(cells.row(0)[1] == 2);
    D_TBL_CHECK(cells.column(2)[0] == 3);

    const tb_static from_array(std::array<int, 6>{ { 6, 5, 4, 3, 2, 1 } });
    D_TBL_CHECK(from_array(0, 0) == 6);
    D_TBL_CHECK(tb_static::filled(4).count(4) == 6);

    auto made = make_static_table<1, 2>(8, 9);
    D_TBL_CHECK((std::is_same<decltype(made), DStaticTable<int, 1, 2>>::value));
    D_TBL_CHECK(made(0, 1) == 9);

    D_TBL_CHECK(tb_static::rows() == 2);
    D_TBL_CHECK(tb_static::cols() == 3);
    D_TBL_CHECK(tb_static::capacity() == 6);
    D_TBL_CHECK(tb_static::max_size() == 6);
    D_TBL_CHECK(tb_static::extent == 6);
    D_TBL_CHECK(cells.full());
    D_TBL_CHECK(tb_static::compile_time_iterable);
    D_TBL_CHECK(tb_static::compile_time_values);
    D_TBL_CHECK(tb_static::lifetime == DContainerLifetime::constexpr_storage);

    D_TBL_CHECK((std::is_same<decltype(*cells.begin()), const int&>::value));
    D_TBL_CHECK(std::vector<int>(cells.begin(), cells.end()) ==
                std::vector<int>({ 1, 2, 3, 4, 5, 6 }));

    return true;
}

/*
tests_static_table_constexpr
  constant evaluation and map
  Tests the following:
  - a constexpr static table answers cell reads, size and contains as
    constants
  - map, with a constexpr function, is itself a constant expression
  - map changes the cell type and keeps the shape
*/
bool tests_static_table_constexpr()
{
    constexpr tb_static t(1, 2, 3, 4, 5, 6);
    constexpr int cell = t(1, 1);
    constexpr std::size_t n = t.size();
    constexpr bool in = t.contains(1, 2);
    constexpr bool out = t.contains(2, 0);
    D_TBL_CHECK(cell == 5);
    D_TBL_CHECK(n == 6);
    D_TBL_CHECK(in);
    D_TBL_CHECK(!out);

    constexpr auto doubled = t.map(tb_doubled);
    constexpr int last = doubled(1, 2);
    D_TBL_CHECK(last == 12);

    auto words = t.map([](const int& _x) { return std::to_string(_x); });
    D_TBL_CHECK((std::is_same<decltype(words), DStaticTable<std::string, 2,
                3>>::value));
    D_TBL_CHECK(words(0, 2) == "3");

    return true;
}

/*
tests_static_table_serial
  equality, the serial pair, and detection
  Tests the following:
  - == and != compare cells, also as constants
  - encode_into / decode round-trip; a stream of another shape fails
  - is_static_table holds for static tables only
*/
bool tests_static_table_serial()
{
    constexpr tb_static a(1, 2, 3, 4, 5, 6);
    constexpr tb_static b(1, 2, 3, 4, 5, 6);
    constexpr tb_static c(0, 2, 3, 4, 5, 6);
    constexpr bool same = (a == b);
    D_TBL_CHECK(same);
    D_TBL_CHECK(a != c);

    DByteString bytes;
    a.encode_into(bytes);
    DByteReader reader(bytes);
    DDecodeResult<tb_static> back = tb_static::decode(reader);
    D_TBL_CHECK(back.ok);
    D_TBL_CHECK(back.value == a);

    DByteReader other(bytes);
    D_TBL_CHECK(!(DStaticTable<int, 3, 2>::decode(other).ok));

    D_TBL_CHECK(is_static_table<tb_static>::value);
    D_TBL_CHECK(is_static_table_v<const tb_static&>);
    D_TBL_CHECK(!is_static_table<int>::value);

    return true;
}

NS_END  // testing
NS_END  // djinterp
