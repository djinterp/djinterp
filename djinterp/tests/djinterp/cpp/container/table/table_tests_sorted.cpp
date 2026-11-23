/*******************************************************************************
* djinterp [cpp][container]                               table_tests_sorted.cpp
*
* sorted_table.hpp: the rank-2 table kept sorted on a key column --
* construction, lookup, insertion with unique and repeated keys, refusal, and
* erasure.
*
*
* path:      /tests/djinterp/cpp/container/table/table_tests_sorted.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.16
*                                                            revised: 2026.11.16
*******************************************************************************/
// std
#include <cstddef>      // std::size_t
#include <functional>   // std::less, std::greater
#include <stdexcept>    // std::invalid_argument, std::out_of_range
#include <type_traits>  // std::is_same
#include <vector>       // std::vector
// djinterp
#include "table_tests.hpp"  // D_TBL_CHECK, the declarations
#include <djinterp/cpp/container/table/sorted_table.hpp>  // under test


NS_DJINTERP
NS_TESTING

namespace
{

// tb_sorted_cells
//   function: a sorted table's cells in row-major order.
template<typename Table>
std::vector<int> tb_sorted_cells(const Table& _t)
{
    return std::vector<int>(_t.begin(), _t.end());
}

}  // namespace


/*
tests_sorted_table_construction
  construction, the read-only face, and the declared positions
  Tests the following:
  - rows given in any order come out sorted on the key column
  - a key column other than 0; a descending comparator
  - shape, cell reads (at throws std::out_of_range), row views, iterators
  - base() exposes the backing table; is_sorted holds
  - the member constants and is_sorted_table
*/
bool tests_sorted_table_construction()
{
    DSortedTable<int> empty;
    D_TBL_CHECK(empty.empty());
    D_TBL_CHECK(empty.rows() == 0);

    DSortedTable<int> t{ { 3, 30 }, { 1, 10 }, { 2, 20 } };
    D_TBL_CHECK(tb_sorted_cells(t) == std::vector<int>({ 1, 10, 2, 20, 3,
                30 }));
    D_TBL_CHECK(t.rows() == 3);
    D_TBL_CHECK(t.cols() == 2);
    D_TBL_CHECK(t.row_count() == 3);
    D_TBL_CHECK(t.column_count() == 2);
    D_TBL_CHECK(t.size() == 6);
    D_TBL_CHECK(t(2, 1) == 30);
    D_TBL_CHECK(t.at(1, 0) == 2);
    D_TBL_CHECK(tbl_throws<std::out_of_range>([&t] { (void)t.at(3, 0); }));
    D_TBL_CHECK(t.row(0)[1] == 10);
    D_TBL_CHECK(t[1][1] == 20);
    D_TBL_CHECK((t.row_end() - t.row_begin()) == 3);
    D_TBL_CHECK((t.cend() - t.cbegin()) == 6);
    D_TBL_CHECK(t.is_sorted());
    D_TBL_CHECK(t.base().rows() == 3);
    D_TBL_CHECK(t.key_at(2) == 3);

    auto by_second = make_sorted_table<1>({ { 0, 9 }, { 0, 7 }, { 0, 8 } });
    D_TBL_CHECK(by_second.key_column == 1);
    D_TBL_CHECK(tb_sorted_cells(by_second) == std::vector<int>({ 0, 7, 0, 8, 0,
                9 }));

    DSortedTable<int, 0, std::greater<int>> down{ { 1 }, { 3 }, { 2 } };
    D_TBL_CHECK(tb_sorted_cells(down) == std::vector<int>({ 3, 2, 1 }));
    D_TBL_CHECK(down.is_sorted());

    using map_type = DSortedTable<int, 0, std::less<int>, true>;
    D_TBL_CHECK(map_type::unique_keys);
    D_TBL_CHECK(map_type::sorted_invariant);
    D_TBL_CHECK(map_type::multiplicity_grade == DContainerMultiplicity::unique);
    D_TBL_CHECK(!DSortedTable<int>::unique_keys);
    D_TBL_CHECK(DSortedTable<int>::rank == 2);
    D_TBL_CHECK(DSortedTable<int>::npos == static_cast<std::size_t>(-1));
    D_TBL_CHECK(is_sorted_table<map_type>::value);
    D_TBL_CHECK(is_sorted_table_v<const DSortedTable<int>&>);
    D_TBL_CHECK(!is_sorted_table<DTable<int>>::value);

    return true;
}

/*
tests_sorted_table_lookup
  key_at, the bounds, find, contains_key, count_key
  Tests the following:
  - lower_bound and upper_bound bracket a key, present or not
  - find returns a row index or npos
  - contains_key; count_key counts a run of repeated keys
*/
bool tests_sorted_table_lookup()
{
    DSortedTable<int> t{ { 5, 0 }, { 1, 0 }, { 5, 1 }, { 9, 0 } };

    D_TBL_CHECK(t.lower_bound(5) == 1);
    D_TBL_CHECK(t.upper_bound(5) == 3);
    D_TBL_CHECK(t.lower_bound(6) == 3);
    D_TBL_CHECK(t.upper_bound(6) == 3);
    D_TBL_CHECK(t.lower_bound(0) == 0);
    D_TBL_CHECK(t.upper_bound(10) == 4);

    D_TBL_CHECK(t.find(9) == 3);
    D_TBL_CHECK(t.find(4) == t.npos);
    D_TBL_CHECK(t.contains_key(1));
    D_TBL_CHECK(!t.contains_key(2));
    D_TBL_CHECK(t.count_key(5) == 2);
    D_TBL_CHECK(t.count_key(2) == 0);

    return true;
}

/*
tests_sorted_table_insert
  insert with repeated and with unique keys
  Tests the following:
  - insert returns the row index taken
  - repeated keys keep their insertion order: a new equal key goes after
    its equals, as in keyed_table
  - with unique keys, an equivalent key overwrites that row in place
*/
bool tests_sorted_table_insert()
{
    DSortedTable<int> multi{ { 2, 1 }, { 1, 0 }, { 2, 2 } };
    D_TBL_CHECK(tb_sorted_cells(multi) == std::vector<int>({ 1, 0, 2, 1, 2,
                2 }));
    D_TBL_CHECK(multi.insert({ 2, 3 }) == 3);
    D_TBL_CHECK(multi.insert({ 0, 0 }) == 0);
    D_TBL_CHECK(tb_sorted_cells(multi) ==
                std::vector<int>({ 0, 0, 1, 0, 2, 1, 2, 2, 2, 3 }));
    D_TBL_CHECK(multi.is_sorted());

    DSortedTable<int, 0, std::less<int>, true> unique{ { 1, 10 }, { 2, 20 } };
    D_TBL_CHECK(unique.insert({ 2, 99 }) == 1);
    D_TBL_CHECK(unique.rows() == 2);
    D_TBL_CHECK(unique(1, 1) == 99);
    D_TBL_CHECK(unique.insert({ 3, 30 }) == 2);
    D_TBL_CHECK(unique.count_key(2) == 1);

    return true;
}

/*
tests_sorted_table_refusal
  rows that cannot be placed are refused, and leave the table as it was
  Tests the following:
  - a row without the key column throws std::invalid_argument, on an empty
    table too, before any cell is read
  - a row of the wrong width throws std::invalid_argument; on a unique-key
    table the row with the equivalent key survives
*/
bool tests_sorted_table_refusal()
{
    DSortedTable<int, 2> keyed_on_third;
    D_TBL_CHECK(tbl_throws<std::invalid_argument>([&keyed_on_third]
    {
        (void)keyed_on_third.insert({ 5 });
    }));
    D_TBL_CHECK(keyed_on_third.empty());
    D_TBL_CHECK(keyed_on_third.cols() == 0);
    D_TBL_CHECK(keyed_on_third.insert({ 0, 0, 5 }) == 0);

    DSortedTable<int, 0, std::less<int>, true> unique{ { 1, 10 }, { 2, 20 } };
    D_TBL_CHECK(tbl_throws<std::invalid_argument>([&unique]
    {
        (void)unique.insert({ 1, 99, 7 });
    }));
    D_TBL_CHECK(unique.rows() == 2);
    D_TBL_CHECK(unique.find(1) == 0);
    D_TBL_CHECK(unique(0, 1) == 10);

    DSortedTable<int> multi{ { 1, 10 } };
    D_TBL_CHECK(tbl_throws<std::invalid_argument>([&multi]
    {
        (void)multi.insert({ 2 });
    }));
    D_TBL_CHECK(tb_sorted_cells(multi) == std::vector<int>({ 1, 10 }));

    return true;
}

/*
tests_sorted_table_erase
  erase_at, erase_key, clear
  Tests the following:
  - erase_at removes one row; erase_key removes the whole run of a key and
    returns its length (0 when absent)
  - clear empties the table, and its next row sets a new width
*/
bool tests_sorted_table_erase()
{
    DSortedTable<int> t{ { 1, 0 }, { 2, 0 }, { 2, 1 }, { 3, 0 } };

    t.erase_at(0);
    D_TBL_CHECK(t.key_at(0) == 2);
    D_TBL_CHECK(t.erase_key(2) == 2);
    D_TBL_CHECK(t.erase_key(2) == 0);
    D_TBL_CHECK(tb_sorted_cells(t) == std::vector<int>({ 3, 0 }));

    t.clear();
    D_TBL_CHECK(t.empty());
    D_TBL_CHECK(t.insert({ 4, 4, 4 }) == 0);
    D_TBL_CHECK(t.cols() == 3);

    return true;
}

NS_END  // testing
NS_END  // djinterp
