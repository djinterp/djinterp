/*******************************************************************************
* djinterp [cpp][container]                          table_tests_constrained.cpp
*
* constrained_table.hpp: the table under a row cap and an optional closed
* domain -- the cap, the domain, the read-only face, and removal.
*
*
* path:      /tests/djinterp/cpp/container/table/table_tests_constrained.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.16
*                                                            revised: 2026.11.16
*******************************************************************************/
// std
#include <cstddef>      // std::size_t
#include <stdexcept>    // std::length_error, std::out_of_range
#include <vector>       // std::vector
// djinterp
#include "table_tests.hpp"  // D_TBL_CHECK, the declarations
#include <djinterp/cpp/container/table/constrained_table.hpp>  // under test


NS_DJINTERP
NS_TESTING

/*
tests_constrained_table_capacity
  the row cap
  Tests the following:
  - max_rows and max_row_capacity are the template's; capacity is the cap
    times the width
  - rows are accepted until full; past it push_row throws
    std::length_error and try_push_row returns false, and the table is
    unchanged either way
  - the member constants and is_constrained_table
*/
bool tests_constrained_table_capacity()
{
    using capped = DConstrainedTable<int, 2>;

    D_TBL_CHECK(capped::max_rows() == 2);
    D_TBL_CHECK(capped::max_row_capacity == 2);
    D_TBL_CHECK(capped::bounded_overlay);
    D_TBL_CHECK(capped::rank == 2);
    D_TBL_CHECK(is_constrained_table<capped>::value);
    D_TBL_CHECK(is_constrained_table_v<const capped&>);
    D_TBL_CHECK(!is_constrained_table<DTable<int>>::value);

    auto t = make_constrained_table<2, int>();
    D_TBL_CHECK(t.empty());
    D_TBL_CHECK(!t.full());
    D_TBL_CHECK(!t.has_domain());

    t.push_row({ 1, 2, 3 });
    D_TBL_CHECK(t.capacity() == 6);
    D_TBL_CHECK(t.try_push_row({ 4, 5, 6 }));
    D_TBL_CHECK(t.full());

    D_TBL_CHECK(tbl_throws<std::length_error>([&t] { t.push_row({ 7, 8,
                9 }); }));
    D_TBL_CHECK(!t.try_push_row({ 7, 8, 9 }));
    D_TBL_CHECK(t.rows() == 2);
    D_TBL_CHECK(std::vector<int>(t.begin(), t.end()) ==
                std::vector<int>({ 1, 2, 3, 4, 5, 6 }));

    return true;
}

/*
tests_constrained_table_domain
  the closed domain [lo, hi]
  Tests the following:
  - with no domain every value is in it
  - in_domain is inclusive at both ends
  - a row with any cell outside throws std::out_of_range from push_row and
    is refused by try_push_row; the table is unchanged
  - domain_low and domain_high report the interval
*/
bool tests_constrained_table_domain()
{
    DConstrainedTable<int, 4> open_ended;
    D_TBL_CHECK(open_ended.in_domain(-1000000));

    auto t = make_domain_table<4>(0, 9);
    D_TBL_CHECK(t.has_domain());
    D_TBL_CHECK(t.domain_low() == 0);
    D_TBL_CHECK(t.domain_high() == 9);
    D_TBL_CHECK(t.in_domain(0));
    D_TBL_CHECK(t.in_domain(9));
    D_TBL_CHECK(!t.in_domain(-1));
    D_TBL_CHECK(!t.in_domain(10));

    t.push_row({ 0, 9 });
    D_TBL_CHECK(tbl_throws<std::out_of_range>([&t] { t.push_row({ 5, 10 }); }));
    D_TBL_CHECK(!t.try_push_row({ -1, 5 }));
    D_TBL_CHECK(t.try_push_row({ 3, 4 }));
    D_TBL_CHECK(t.rows() == 2);
    D_TBL_CHECK(t(1, 0) == 3);

    DConstrainedTable<int, 3> direct(1, 2);
    D_TBL_CHECK(direct.has_domain());
    D_TBL_CHECK(!direct.try_push_row({ 3 }));

    return true;
}

/*
tests_constrained_table_reads
  the read-only face, delegated to the backing table
  Tests the following:
  - shape, cell reads (at throws std::out_of_range), row views
  - the cell and row iterators; base() is the backing table
*/
bool tests_constrained_table_reads()
{
    auto t = make_constrained_table<3, int>();
    t.push_row({ 1, 2 });
    t.push_row({ 3, 4 });

    D_TBL_CHECK(t.rows() == 2);
    D_TBL_CHECK(t.cols() == 2);
    D_TBL_CHECK(t.row_count() == 2);
    D_TBL_CHECK(t.column_count() == 2);
    D_TBL_CHECK(t.size() == 4);
    D_TBL_CHECK(t(1, 0) == 3);
    D_TBL_CHECK(t.at(0, 1) == 2);
    D_TBL_CHECK(tbl_throws<std::out_of_range>([&t] { (void)t.at(2, 0); }));
    D_TBL_CHECK(t.row(1)[1] == 4);
    D_TBL_CHECK(t[0][0] == 1);
    D_TBL_CHECK((t.cend() - t.cbegin()) == 4);
    D_TBL_CHECK((t.row_end() - t.row_begin()) == 2);
    D_TBL_CHECK(t.base() == (DTable<int>{ { 1, 2 }, { 3, 4 } }));

    return true;
}

/*
tests_constrained_table_removal
  erase_row, pop_row, clear
  Tests the following:
  - removal frees room under the cap
  - clear empties the table and keeps the domain
*/
bool tests_constrained_table_removal()
{
    auto t = make_domain_table<2>(0, 5);
    t.push_row({ 1 });
    t.push_row({ 2 });
    D_TBL_CHECK(t.full());

    t.erase_row(0);
    D_TBL_CHECK(!t.full());
    D_TBL_CHECK(t(0, 0) == 2);
    D_TBL_CHECK(t.try_push_row({ 3 }));

    t.pop_row();
    D_TBL_CHECK(t.rows() == 1);

    t.clear();
    D_TBL_CHECK(t.empty());
    D_TBL_CHECK(t.has_domain());
    D_TBL_CHECK(!t.try_push_row({ 6 }));

    return true;
}

NS_END  // testing
NS_END  // djinterp
