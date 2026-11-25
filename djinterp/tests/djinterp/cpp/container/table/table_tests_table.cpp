/*******************************************************************************
* djinterp [cpp][container]                                table_tests_table.cpp
*
* table.hpp: the dynamic rank-2 table - construction, the axis constants,
* writable access, row and column structure, sizing, the algorithms, equality,
* the serial pair and detection.
*
*
* path:      /tests/djinterp/cpp/container/table/table_tests_table.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.16
*                                                            revised: 2026.11.16
*******************************************************************************/
// std
#include <cstddef>      // std::size_t
#include <stdexcept>    // std::invalid_argument, std::length_error
#include <string>       // std::string
#include <type_traits>  // std::is_same
#include <vector>       // std::vector
// djinterp
#include "table_tests.hpp"  // D_TBL_CHECK, the declarations
#include <djinterp/cpp/container/table/table.hpp>  // under test
// re_std
#include "../../../../../inc/re_std/cpp/cstdint/cstdint.hpp"  // uint_least64_t


NS_DJINTERP
NS_TESTING

namespace
{

using tb_int_table = DTable<int>;
using tb_row       = internal::basic_row_view<const int>;

// tb_cells
//   function: a table's cells in row-major order.
std::vector<int> tb_cells(const tb_int_table& _t)
{
    return std::vector<int>(_t.begin(), _t.end());
}

// tb_big
//   constant: 2 to half the bits of std::size_t, so its square does not fit
// std::size_t (2^32 on a 64-bit target, 2^16 on a 32-bit one).
const std::size_t tb_big = (static_cast<std::size_t>(1) << ((sizeof(std::size_t) * 4)));

}  // namespace


/*
tests_table_construction
  every way to make a table, and the shapes each refuses
  Tests the following:
  - default 0 x 0; sized and value-initialised; sized and filled
  - nested lists; a ragged list throws std::invalid_argument
  - from_flat adopts a buffer; a size mismatch throws std::invalid_argument
  - a shape whose cell count overflows std::size_t throws std::length_error,
    from the constructors and from from_flat
*/
bool tests_table_construction()
{
    tb_int_table empty;
    D_TBL_CHECK(empty.rows() == 0);
    D_TBL_CHECK(empty.cols() == 0);
    D_TBL_CHECK(empty.empty());

    tb_int_table zeros(2, 3);
    D_TBL_CHECK(zeros.rows() == 2);
    D_TBL_CHECK(zeros.cols() == 3);
    D_TBL_CHECK(zeros.size() == 6);
    D_TBL_CHECK(zeros.count(0) == 6);

    tb_int_table sevens(3, 2, 7);
    D_TBL_CHECK(sevens.size() == 6);
    D_TBL_CHECK(sevens.count(7) == 6);

    tb_int_table lit{ { 1, 2, 3 }, { 4, 5, 6 } };
    D_TBL_CHECK(lit.rows() == 2);
    D_TBL_CHECK(lit.cols() == 3);
    D_TBL_CHECK(lit(1, 2) == 6);

    D_TBL_CHECK(tbl_throws<std::invalid_argument>([]
    {
        tb_int_table ragged{ { 1, 2 }, { 3 } };
        (void)ragged;
    }));

    tb_int_table flat = tb_int_table::from_flat({ 1, 2, 3, 4, 5, 6 }, 3, 2);
    D_TBL_CHECK(flat.rows() == 3);
    D_TBL_CHECK(flat(2, 1) == 6);
    D_TBL_CHECK(tbl_throws<std::invalid_argument>([]
    {
        (void)tb_int_table::from_flat({ 1, 2, 3 }, 2, 2);
    }));

    D_TBL_CHECK(tbl_throws<std::length_error>([]
    {
        tb_int_table t(tb_big, tb_big);
        (void)t;
    }));
    D_TBL_CHECK(tbl_throws<std::length_error>([]
    {
        tb_int_table t(tb_big, tb_big, 1);
        (void)t;
    }));
    D_TBL_CHECK(tbl_throws<std::length_error>([]
    {
        (void)tb_int_table::from_flat({}, tb_big, tb_big);
    }));

    return true;
}

/*
tests_table_axes
  the declared positions on the container axes
  Tests the following:
  - mutable dynamic storage, fully mutable, ordered, unbounded, iterable,
    multi, hierarchical; neither compile-time iterable nor valued
  - the member types name the views and cursors of table_base.hpp
*/
bool tests_table_axes()
{
    D_TBL_CHECK(tb_int_table::lifetime == DContainerLifetime::mutable_storage);
    D_TBL_CHECK(tb_int_table::storage_kind
                == DContainerStorageKind::dynamic_storage);
    D_TBL_CHECK(tb_int_table::mutability_grade == DMutability::fully_mutable);
    D_TBL_CHECK(tb_int_table::ordering == DContainerOrdering::ordered);
    D_TBL_CHECK(tb_int_table::bounds == DContainerBounds::unbounded);
    D_TBL_CHECK(tb_int_table::iterability == DContainerIterability::iterable);
    D_TBL_CHECK(tb_int_table::multiplicity_grade
                == DContainerMultiplicity::multi);
    D_TBL_CHECK(tb_int_table::structure == DContainerStructure::hierarchical);
    D_TBL_CHECK(!tb_int_table::compile_time_iterable);
    D_TBL_CHECK(!tb_int_table::compile_time_values);
    D_TBL_CHECK(tb_int_table::rank == 2);

    D_TBL_CHECK(std::is_same<tb_int_table::row_type,
                             internal::basic_row_view<int>>::value);
    D_TBL_CHECK(std::is_same<tb_int_table::column_type,
                             internal::basic_column_view<int>>::value);
    D_TBL_CHECK(std::is_same<tb_int_table::row_iterator,
                             internal::row_cursor<int>>::value);
    D_TBL_CHECK(std::is_same<tb_int_table::iterator, int*>::value);

    return true;
}

/*
tests_table_access
  writable access through every path
  Tests the following:
  - operator() and at() write; at() throws std::out_of_range off the box
  - a row view and a column view write through to the cells
  - the cell iterators and the row iterators write
  - the const surface still answers on a non-const table
*/
bool tests_table_access()
{
    tb_int_table t(2, 3);

    t(0, 0) = 1;
    t.at(0, 1) = 2;
    t.row(0)[2] = 3;
    t[1][0] = 4;
    t.column(1)[1] = 5;
    *(t.end() - 1) = 6;
    D_TBL_CHECK(tb_cells(t) == std::vector<int>({ 1, 2, 3, 4, 5, 6 }));

    D_TBL_CHECK(tbl_throws<std::out_of_range>([&t] { (void)t.at(2, 0); }));
    D_TBL_CHECK(tbl_throws<std::out_of_range>([&t] { (void)t.at(0, 3); }));

    for (int* p = t.begin(); p != t.end(); ++p)
    {
        *p *= 10;
    }
    (*(t.row_begin() + 1))[1] = 55;
    D_TBL_CHECK(t(1, 1) == 55);
    D_TBL_CHECK(t(0, 2) == 30);
    D_TBL_CHECK((t.row_end() - t.row_begin()) == 2);

    const tb_int_table& c = t;
    D_TBL_CHECK(c.row(1)[2] == 60);
    D_TBL_CHECK(c.column(0)[1] == 40);
    D_TBL_CHECK(*c.begin() == 10);

    return true;
}

/*
tests_table_row_operations
  rows: append, insert, erase, pop
  Tests the following:
  - the first row of an empty table sets the width; later rows must match
    (std::invalid_argument)
  - push_row from a range
  - insert_row at the front, the middle and the end; past the end throws
  - erase_row; past the end throws; pop_row on an empty table does nothing
*/
bool tests_table_row_operations()
{
    tb_int_table t;
    t.push_row({ 1, 2 });
    D_TBL_CHECK(t.rows() == 1);
    D_TBL_CHECK(t.cols() == 2);
    D_TBL_CHECK(tbl_throws<std::invalid_argument>([&t] { t.push_row({ 1, 2,
                3 }); }));
    D_TBL_CHECK(t.rows() == 1);

    const std::vector<int> more = { 5, 6 };
    t.push_row(more.begin(), more.end());
    t.insert_row(0, { -1, 0 });
    t.insert_row(2, { 3, 4 });
    t.insert_row(4, { 7, 8 });
    D_TBL_CHECK(tb_cells(t) == std::vector<int>({ -1, 0, 1, 2, 3, 4, 5, 6, 7,
                8 }));
    D_TBL_CHECK(tbl_throws<std::out_of_range>([&t] { t.insert_row(6, { 0,
                0 }); }));

    t.erase_row(1);
    D_TBL_CHECK(t.rows() == 4);
    D_TBL_CHECK(t(1, 0) == 3);
    D_TBL_CHECK(tbl_throws<std::out_of_range>([&t] { t.erase_row(4); }));

    t.pop_row();
    D_TBL_CHECK(t.rows() == 3);
    D_TBL_CHECK(t(2, 1) == 6);

    tb_int_table none;
    none.pop_row();
    D_TBL_CHECK(none.rows() == 0);

    return true;
}

/*
tests_table_column_operations
  columns: insert, push, erase, pop
  Tests the following:
  - insert_column at the front, the middle and the end rebuilds each row
  - a column whose height is not the row count throws
    std::invalid_argument - on a table with no rows too, where an empty
    column widens the table and a non-empty one is refused, never dropped
  - insert_column past the end and erase_column past the end throw
  - erase_column and pop_column; pop_column with no columns does nothing
*/
bool tests_table_column_operations()
{
    tb_int_table t{ { 1, 3 }, { 4, 6 } };

    t.insert_column(1, { 2, 5 });
    t.insert_column(0, { 0, 3 });
    t.push_column({ 9, 9 });
    D_TBL_CHECK(t.cols() == 5);
    D_TBL_CHECK(tb_cells(t) == std::vector<int>({ 0, 1, 2, 3, 9, 3, 4, 5, 6,
                9 }));

    D_TBL_CHECK(tbl_throws<std::invalid_argument>([&t] { t.push_column({ 1 }); }));
    D_TBL_CHECK(tbl_throws<std::out_of_range>([&t] { t.insert_column(6, { 1,
                1 }); }));
    D_TBL_CHECK(t.cols() == 5);

    t.erase_column(0);
    t.pop_column();
    D_TBL_CHECK(tb_cells(t) == std::vector<int>({ 1, 2, 3, 4, 5, 6 }));
    D_TBL_CHECK(tbl_throws<std::out_of_range>([&t] { t.erase_column(3); }));

    // a table with no rows: an empty column widens it ...
    tb_int_table bare;
    bare.push_column({});
    bare.push_column({});
    D_TBL_CHECK(bare.rows() == 0);
    D_TBL_CHECK(bare.cols() == 2);

    // ... and a column with cells is refused, leaving the table as it was
    D_TBL_CHECK(tbl_throws<std::invalid_argument>([&bare] { bare.push_column({ 1, 2, 3 }); }));
    D_TBL_CHECK(bare.rows() == 0);
    D_TBL_CHECK(bare.cols() == 2);

    bare.erase_column(0);
    D_TBL_CHECK(bare.cols() == 1);
    bare.pop_column();
    bare.pop_column();
    D_TBL_CHECK(bare.cols() == 0);

    return true;
}

/*
tests_table_resize_and_capacity
  sizing: resize, reserve, fill, clear, shrink
  Tests the following:
  - resize keeps the overlapping top-left block and fills the rest
  - resize to the same shape changes nothing
  - reserve and reserve_rows grow capacity; reserve_rows needs a width
  - row_capacity is 0 with no width; max_size is positive
  - fill sets every cell; clear returns to 0 x 0; shrink_to_fit keeps cells
*/
bool tests_table_resize_and_capacity()
{
    tb_int_table t{ { 1, 2 }, { 3, 4 } };

    t.resize(3, 3, 9);
    D_TBL_CHECK(tb_cells(t) == std::vector<int>({ 1, 2, 9, 3, 4, 9, 9, 9, 9 }));
    t.resize(1, 2);
    D_TBL_CHECK(tb_cells(t) == std::vector<int>({ 1, 2 }));
    t.resize(1, 2, 7);
    D_TBL_CHECK(tb_cells(t) == std::vector<int>({ 1, 2 }));

    tb_int_table none;
    D_TBL_CHECK(none.row_capacity() == 0);
    none.reserve_rows(10);
    D_TBL_CHECK(none.capacity() == 0);
    none.reserve(12);
    D_TBL_CHECK(none.capacity() >= 12);
    D_TBL_CHECK(none.max_size() > 0);

    t.reserve_rows(8);
    D_TBL_CHECK(t.capacity() >= 16);
    D_TBL_CHECK(t.row_capacity() >= 8);

    t.resize(2, 2);
    t.fill(5);
    D_TBL_CHECK(t.count(5) == 4);

    t.shrink_to_fit();
    D_TBL_CHECK(t.size() == 4);
    D_TBL_CHECK(t.count(5) == 4);

    t.clear();
    D_TBL_CHECK(t.rows() == 0);
    D_TBL_CHECK(t.cols() == 0);
    t.push_row({ 1, 2, 3 });
    D_TBL_CHECK(t.cols() == 3);

    return true;
}

/*
tests_table_algorithms
  sort_rows, map, filter_rows, select_rows
  Tests the following:
  - sort_rows is stable under the row comparator
  - map changes the cell type and keeps the shape; map_inplace overwrites
    each cell with its image
  - filter_rows and filter_rows_inplace keep order and agree; with no
    survivors both keep the width
  - select_rows gathers from any table-like source, keeping its width
*/
bool tests_table_algorithms()
{
    tb_int_table t{ { 2, 0 }, { 1, 1 }, { 2, 2 }, { 1, 3 } };

    t.sort_rows([](tb_row _a, tb_row _b) { return (_a[0] < _b[0]); });
    D_TBL_CHECK(tb_cells(t) == std::vector<int>({ 1, 1, 1, 3, 2, 0, 2, 2 }));

    DTable<std::string> s = t.map([](const int& _x) { return std::to_string(_x); });
    D_TBL_CHECK(s.rows() == 4);
    D_TBL_CHECK(s.cols() == 2);
    D_TBL_CHECK(s(3, 1) == "2");
    D_TBL_CHECK((std::is_same<decltype(s)::cell_type, std::string>::value));

    t.map_inplace([](const int& _x) { return (2 * _x); });
    D_TBL_CHECK(t(1, 1) == 6);

    auto odd_second = [](tb_row _r) { return ((_r[1] / 2) % 2) == 1; };
    tb_int_table kept = t.filter_rows(odd_second);
    D_TBL_CHECK(tb_cells(kept) == std::vector<int>({ 2, 2, 2, 6 }));

    tb_int_table in_place = t;
    in_place.filter_rows_inplace(odd_second);
    D_TBL_CHECK(in_place == kept);

    auto none = [](tb_row) { return false; };
    tb_int_table nothing = t.filter_rows(none);
    tb_int_table nothing_in_place = t;
    nothing_in_place.filter_rows_inplace(none);
    D_TBL_CHECK(nothing.rows() == 0);
    D_TBL_CHECK(nothing.cols() == 2);
    D_TBL_CHECK(nothing == nothing_in_place);

    tb_int_table picked = select_rows(t, [](tb_row _r) { return (_r[0]
                                      == 4); });
    D_TBL_CHECK(tb_cells(picked) == std::vector<int>({ 4, 0, 4, 4 }));
    D_TBL_CHECK(select_rows(t, none).cols() == 2);

    return true;
}

/*
tests_table_equality_and_swap
  ==, !=, and both swaps
  Tests the following:
  - equal shape and cells compare equal; a different cell or shape does not
  - a 0 x 3 and a 0 x 0 table differ (the width is part of the value)
  - member swap and free swap exchange whole tables
*/
bool tests_table_equality_and_swap()
{
    tb_int_table a{ { 1, 2 }, { 3, 4 } };
    tb_int_table b{ { 1, 2 }, { 3, 4 } };
    tb_int_table c{ { 1, 2 }, { 3, 5 } };
    tb_int_table d{ { 1, 2, 3, 4 } };

    D_TBL_CHECK(a == b);
    D_TBL_CHECK(!(a != b));
    D_TBL_CHECK(a != c);
    D_TBL_CHECK(a != d);
    D_TBL_CHECK(tb_int_table(0, 3) != tb_int_table());

    a.swap(d);
    D_TBL_CHECK(a.rows() == 1);
    D_TBL_CHECK(d == b);

    swap(a, c);
    D_TBL_CHECK(a.rows() == 2);
    D_TBL_CHECK(a(1, 1) == 5);
    D_TBL_CHECK(c.cols() == 4);

    return true;
}

/*
tests_table_serial
  encode_into / decode and the parameterised pair
  Tests the following:
  - the default pair round-trips a table of ints (the serial leaves are the
    arithmetic types; a table of strings does not encode)
  - encode_into_e / decode_e round-trip at another endianness and count
  - a truncated stream fails to decode
  - a shape whose cell count overflows std::size_t fails to decode rather
    than yield extents its buffer does not hold
*/
bool tests_table_serial()
{
    const tb_int_table t{ { 1, -2, 3 }, { 400, 5, 60000 } };

    DByteString bytes;
    t.encode_into(bytes);
    DByteReader reader(bytes);
    DDecodeResult<tb_int_table> back = tb_int_table::decode(reader);
    D_TBL_CHECK(back.ok);
    D_TBL_CHECK(back.value == t);

    DByteString little;
    t.encode_into_e<DSerialEndian::little, DSerialLength::u32>(little);
    DByteReader lreader(little);
    DDecodeResult<tb_int_table> lback =
        tb_int_table::decode_e<DSerialEndian::little,
                               DSerialLength::u32>(lreader);
    D_TBL_CHECK(lback.ok);
    D_TBL_CHECK(lback.value == t);

    DByteString cut(bytes.begin(), bytes.end() - 1);
    DByteReader creader(cut);
    D_TBL_CHECK(!tb_int_table::decode(creader).ok);

    // a big-endian u64 header claiming 2^32 x 2^32 and no cells
    DByteString hostile;
    const re_std::uint_least64_t big =
        (static_cast<re_std::uint_least64_t>(1) << 32);
    for (int k = 0; k < 2; ++k)
    {
        for (int s = 7; s >= 0; --s)
        {
            hostile.push_back(static_cast<byte>((big >> (8 * s)) & 0xFFu));
        }
    }
    DByteReader hreader(hostile);
    D_TBL_CHECK(!tb_int_table::decode(hreader).ok);

    return true;
}

/*
tests_table_detection
  is_table
  Tests the following:
  - true for table instantiations through cv and references; false otherwise
*/
bool tests_table_detection()
{
    D_TBL_CHECK(is_table<tb_int_table>::value);
    D_TBL_CHECK(is_table<const DTable<std::string>&>::value);
    D_TBL_CHECK(is_table_v<DTable<double, long>>);
    D_TBL_CHECK(!is_table<int>::value);
    D_TBL_CHECK(!is_table<std::vector<int>>::value);

    return true;
}

NS_END  // testing
NS_END  // djinterp
