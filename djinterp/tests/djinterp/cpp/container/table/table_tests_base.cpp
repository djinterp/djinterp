/*******************************************************************************
* djinterp [cpp][container]                                 table_tests_base.cpp
*
* table_base.hpp: the row and column views, the cursors, and the CRTP
* read-only rank-2 surface.  The surface is driven through tb_grid, a minimal
* derived class over a fixed buffer, so the header is compiled with nothing
* else of the table directory.
*
*
* path:      /tests/djinterp/cpp/container/table/table_tests_base.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.16
*                                                            revised: 2026.11.16
*******************************************************************************/
// std
#include <cstddef>      // std::size_t, std::ptrdiff_t
#include <functional>   // std::greater
#include <iterator>     // std::distance
#include <stdexcept>    // std::out_of_range
#include <type_traits>  // std::is_same
// djinterp
#include "table_tests.hpp"  // D_TBL_CHECK, the declarations
#include <djinterp/cpp/container/table/table_base.hpp>  // under test


NS_DJINTERP
NS_TESTING

namespace
{

// tb_grid
//   type: the smallest table table_base can serve - a 2 x 3 grid of ints over
// an owned buffer, supplying the three hooks data(), rows() and cols().
struct tb_grid
    : public DTableBase<tb_grid, int, std::size_t, std::ptrdiff_t>
{
    int         cells[6];
    std::size_t r;
    std::size_t c;

    tb_grid(int _a, int _b, int _c, int _d, int _e, int _f)
        : cells{ _a, _b, _c, _d, _e, _f },
          r(2),
          c(3)
    {}

    const int*  data() const noexcept { return cells; }
    std::size_t rows() const noexcept { return r; }
    std::size_t cols() const noexcept { return c; }
};

// tb_shape_only
//   type: a stand-in with row_count/column_count and a row-major data(), the
// three members content_equals reads, for a comparison across types.
struct tb_shape_only
{
    int vals[6];

    std::size_t row_count()    const { return 2; }
    std::size_t column_count() const { return 3; }
    const int*  data()         const { return vals; }
};

}  // namespace


/*
tests_table_base_row_view
  basic_row_view: the rank-1 subtable T[r]
  Tests the following:
  - a default view is empty with a null data pointer
  - size, data, operator[], front, back and the iterator range
  - at() throws std::out_of_range past the end
  - a view over non-const cells writes through
*/
bool tests_table_base_row_view()
{
    int buf[4] = { 10, 20, 30, 40 };

    internal::basic_row_view<const int> none;
    D_TBL_CHECK(none.empty());
    D_TBL_CHECK(none.size() == 0);
    D_TBL_CHECK(none.data() == nullptr);

    internal::basic_row_view<const int> v(buf, 4);
    D_TBL_CHECK(!v.empty());
    D_TBL_CHECK(v.size() == 4);
    D_TBL_CHECK(v.data() == buf);
    D_TBL_CHECK(v[2] == 30);
    D_TBL_CHECK(v.at(3) == 40);
    D_TBL_CHECK(v.front() == 10);
    D_TBL_CHECK(v.back() == 40);
    D_TBL_CHECK(std::distance(v.begin(), v.end()) == 4);

    bool threw = false;
    try
    {
        (void)v.at(4);
    }
    catch (const std::out_of_range&)
    {
        threw = true;
    }
    D_TBL_CHECK(threw);

    internal::basic_row_view<int> w(buf, 4);
    w[1] = 21;
    *w.begin() = 11;
    D_TBL_CHECK(buf[0] == 11);
    D_TBL_CHECK(buf[1] == 21);
    D_TBL_CHECK(std::is_same<internal::basic_row_view<const int>::value_type,
                             int>::value);

    return true;
}

/*
tests_table_base_column_view
  basic_column_view and column_cursor: the projection T[*,c]
  Tests the following:
  - a strided view reads every stride-th cell, checked and unchecked
  - at() throws std::out_of_range past the last row
  - the cursor is random access: ++, --, +=, -=, +, -, difference, []
  - the six comparisons order cursors by position
*/
bool tests_table_base_column_view()
{
    // a 3 x 2 buffer; column 1 is { 2, 4, 6 }
    const int buf[6] = { 1, 2, 3, 4, 5, 6 };

    internal::basic_column_view<const int> none;
    D_TBL_CHECK(none.empty());

    internal::basic_column_view<const int> col(buf + 1, 3, 2);
    D_TBL_CHECK(col.size() == 3);
    D_TBL_CHECK(!col.empty());
    D_TBL_CHECK(col[0] == 2);
    D_TBL_CHECK(col[2] == 6);
    D_TBL_CHECK(col.at(1) == 4);

    bool threw = false;
    try
    {
        (void)col.at(3);
    }
    catch (const std::out_of_range&)
    {
        threw = true;
    }
    D_TBL_CHECK(threw);

    int sum = 0;
    for (int x : col)
    {
        sum += x;
    }
    D_TBL_CHECK(sum == 12);

    using cursor = internal::column_cursor<const int>;
    D_TBL_CHECK(std::is_same<cursor::iterator_category,
                             std::random_access_iterator_tag>::value);

    cursor a = col.begin();
    cursor b = col.end();
    D_TBL_CHECK((b - a) == 3);
    D_TBL_CHECK(a[1] == 4);
    D_TBL_CHECK(*(a + 2) == 6);
    D_TBL_CHECK(*(b - 1) == 6);

    cursor m = a;
    ++m;
    D_TBL_CHECK(*m == 4);
    m++;
    D_TBL_CHECK(*m == 6);
    --m;
    D_TBL_CHECK(*m == 4);
    m--;
    D_TBL_CHECK(m == a);
    m += 2;
    D_TBL_CHECK(*m == 6);
    m -= 1;
    D_TBL_CHECK(*m == 4);

    D_TBL_CHECK(a < m);
    D_TBL_CHECK(m > a);
    D_TBL_CHECK(a <= a);
    D_TBL_CHECK(m >= a);
    D_TBL_CHECK(a != m);
    D_TBL_CHECK(!(a == m));

    return true;
}

/*
tests_table_base_row_cursor
  row_cursor: sweeping rows, each yielded as a view by value
  Tests the following:
  - dereference yields the row's view; operator-> reaches its members
  - arithmetic, difference and comparisons follow the row index
*/
bool tests_table_base_row_cursor()
{
    const int buf[6] = { 1, 2, 3, 4, 5, 6 };

    using cursor = internal::row_cursor<const int>;
    cursor first(buf, 0, 3);
    cursor last(buf, 2, 3);

    D_TBL_CHECK((last - first) == 2);
    D_TBL_CHECK((*first)[0] == 1);
    D_TBL_CHECK(first->size() == 3);
    D_TBL_CHECK(first[1][2] == 6);

    cursor m = first;
    ++m;
    D_TBL_CHECK((*m)[0] == 4);
    m--;
    D_TBL_CHECK(m == first);
    m += 2;
    D_TBL_CHECK(m == last);
    m -= 1;
    D_TBL_CHECK((*m).front() == 4);
    D_TBL_CHECK(((first + 1) - 1) == first);

    D_TBL_CHECK(first < last);
    D_TBL_CHECK(last > first);
    D_TBL_CHECK(first <= first);
    D_TBL_CHECK(last >= first);
    D_TBL_CHECK(first != last);

    return true;
}

/*
tests_table_base_surface
  table_base's read-only surface, through tb_grid
  Tests the following:
  - rank and depth are 2; the structure tags name a hierarchical table
  - shape: rank_of, row_count, column_count, size, empty, contains
  - cell access, checked (std::out_of_range) and unchecked
  - row, operator[], row_at, column, column_at (checked forms throw)
  - the cell iterators (forward, const, reverse) and the row iterators
*/
bool tests_table_base_surface()
{
    using base = DTableBase<tb_grid, int, std::size_t, std::ptrdiff_t>;

    D_TBL_CHECK(base::rank == 2);
    D_TBL_CHECK(base::depth == 2);
    D_TBL_CHECK(std::is_same<base::element_type, int>::value);
    D_TBL_CHECK(std::is_same<base::node_type, base::const_row_type>::value);
    D_TBL_CHECK(std::is_same<base::structure_category, DHierarchical>::value);

    const tb_grid g(1, 2, 3, 4, 5, 6);

    D_TBL_CHECK(g.rank_of() == 2);
    D_TBL_CHECK(g.row_count() == 2);
    D_TBL_CHECK(g.column_count() == 3);
    D_TBL_CHECK(g.size() == 6);
    D_TBL_CHECK(!g.empty());
    D_TBL_CHECK(g.contains(1, 2));
    D_TBL_CHECK(!g.contains(2, 0));
    D_TBL_CHECK(!g.contains(0, 3));

    D_TBL_CHECK(g(1, 0) == 4);
    D_TBL_CHECK(g.at(0, 2) == 3);
    D_TBL_CHECK(g.row(1)[1] == 5);
    D_TBL_CHECK(g[0][0] == 1);
    D_TBL_CHECK(g.row_at(1).back() == 6);
    D_TBL_CHECK(g.column(2)[1] == 6);
    D_TBL_CHECK(g.column_at(0)[1] == 4);
    D_TBL_CHECK(g.column(1).size() == 2);

    D_TBL_CHECK(tbl_throws<std::out_of_range>([&g] { (void)g.at(2, 0); }));
    D_TBL_CHECK(tbl_throws<std::out_of_range>([&g] { (void)g.at(0, 3); }));
    D_TBL_CHECK(tbl_throws<std::out_of_range>([&g] { (void)g.row_at(2); }));
    D_TBL_CHECK(tbl_throws<std::out_of_range>([&g] { (void)g.column_at(3); }));

    D_TBL_CHECK(std::distance(g.begin(), g.end()) == 6);
    D_TBL_CHECK(*g.cbegin() == 1);
    D_TBL_CHECK(*(g.cend() - 1) == 6);
    D_TBL_CHECK(*g.rbegin() == 6);
    D_TBL_CHECK(*(g.crend() - 1) == 1);
    D_TBL_CHECK(std::distance(g.rbegin(), g.rend()) == 6);
    D_TBL_CHECK(std::distance(g.crbegin(), g.crend()) == 6);
    D_TBL_CHECK((g.row_end() - g.row_begin()) == 2);
    D_TBL_CHECK((*(g.row_begin() + 1))[2] == 6);

    return true;
}

/*
tests_table_base_count_and_sortedness
  the Multiplicity and Sortedness faces, and content_equals
  Tests the following:
  - count is the number of cells equal to a value
  - is_sorted (default and with a comparator) reads the row-major order
  - is_row_sorted applies a row comparator to adjacent rows
  - content_equals compares shape and cells across types
*/
bool tests_table_base_count_and_sortedness()
{
    const tb_grid up(1, 2, 2, 3, 4, 5);
    const tb_grid down(6, 5, 4, 3, 2, 1);
    const tb_grid mixed(1, 9, 2, 0, 5, 5);

    D_TBL_CHECK(up.count(2) == 2);
    D_TBL_CHECK(up.count(7) == 0);
    D_TBL_CHECK(mixed.count(5) == 2);

    D_TBL_CHECK(up.is_sorted());
    D_TBL_CHECK(!down.is_sorted());
    D_TBL_CHECK(down.is_sorted(std::greater<int>()));
    D_TBL_CHECK(!mixed.is_sorted());

    // rows ordered by their first cell
    auto by_first = [](internal::basic_row_view<const int> _a,
                       internal::basic_row_view<const int> _b)
    {
        return (_a[0] < _b[0]);
    };
    D_TBL_CHECK(up.is_row_sorted(by_first));
    D_TBL_CHECK(!down.is_row_sorted(by_first));
    D_TBL_CHECK(!mixed.is_row_sorted(by_first));

    const tb_shape_only same = { { 1, 2, 2, 3, 4, 5 } };
    const tb_shape_only other = { { 1, 2, 2, 3, 4, 6 } };
    D_TBL_CHECK(up.content_equals(same));
    D_TBL_CHECK(!up.content_equals(other));
    D_TBL_CHECK(up.content_equals(up));
    D_TBL_CHECK(!up.content_equals(down));

    return true;
}

NS_END  // testing
NS_END  // djinterp
