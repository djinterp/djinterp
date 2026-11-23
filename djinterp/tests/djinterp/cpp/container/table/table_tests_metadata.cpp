/*******************************************************************************
* djinterp [cpp][container]                             table_tests_metadata.cpp
*
* table_metadata.hpp: header cells, levels and stacks, the metadata record
* of key/value entries, and header conformance to a table's shape.
*
*
* path:      /tests/djinterp/cpp/container/table/table_tests_metadata.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.16
*                                                            revised: 2026.11.16
*******************************************************************************/
// std
#include <cstddef>      // std::size_t
#include <string>       // std::string
#include <type_traits>  // std::is_same, std::is_base_of
#include <vector>       // std::vector
// djinterp
#include "table_tests.hpp"  // D_TBL_CHECK, the declarations
#include <djinterp/cpp/container/table/table_metadata.hpp>  // under test


NS_DJINTERP
NS_TESTING

namespace
{

// tb_shape
//   type: just enough of a table for conforms_to - its two counts.
struct tb_shape
{
    std::size_t r;
    std::size_t c;

    std::size_t row_count()    const { return r; }
    std::size_t column_count() const { return c; }
};

}  // namespace


/*
tests_table_metadata_headers
  header_cell, header_level, header_stack and header_extent
  Tests the following:
  - a default cell spans 1; a constructed cell keeps its label and span
  - make_header_level gives one single-span cell per label
  - a level's extent is the sum of its spans; a stack's is its last
    (finest) level's, and an empty stack's is 0
  - has_header_extent detects the overloads
*/
bool tests_table_metadata_headers()
{
    DHeaderCell<std::string> blank;
    D_TBL_CHECK(blank.span == 1);
    D_TBL_CHECK(blank.label.empty());

    DHeaderCell<std::string> wide("coordinate", 3);
    D_TBL_CHECK(wide.label == "coordinate");
    D_TBL_CHECK(wide.span == 3);

    DHeaderLevel<std::string> leaves =
        make_header_level<std::string>({ "id", "x", "y", "z" });
    D_TBL_CHECK(leaves.size() == 4);
    D_TBL_CHECK(leaves[2].label == "y");
    D_TBL_CHECK(leaves[2].span == 1);
    D_TBL_CHECK(header_extent(leaves) == 4);

    DHeaderLevel<std::string> grouped;
    grouped.push_back(DHeaderCell<std::string>("", 1));
    grouped.push_back(DHeaderCell<std::string>("coordinate", 3));
    D_TBL_CHECK(header_extent(grouped) == 4);

    DHeaderStack<std::string> stack;
    D_TBL_CHECK(header_extent(stack) == 0);
    stack.push_back(grouped);
    stack.push_back(make_header_level<std::string>({ "id", "x", "y" }));
    D_TBL_CHECK(header_extent(stack) == 3);

    D_TBL_CHECK(internal::has_header_extent<DHeaderLevel<std::string>>::value);
    D_TBL_CHECK(internal::has_header_extent<DHeaderStack<std::string>>::value);
    D_TBL_CHECK(!internal::has_header_extent<int>::value);

    return true;
}

/*
tests_table_metadata_entries
  the key/value record table_metadata inherits, and detection
  Tests the following:
  - set, find, at, contains, erase, size, clear over string entries
  - table_metadata is a DContainerMetadata; is_table_metadata
*/
bool tests_table_metadata_entries()
{
    DTableMetadata<> m;
    D_TBL_CHECK(m.empty());

    m.set("title", "Results");
    m.set("units", "cm");
    m.set("title", "Final results");
    D_TBL_CHECK(m.size() == 2);
    D_TBL_CHECK(m.contains("units"));
    D_TBL_CHECK(m.at("title") == "Final results");
    D_TBL_CHECK(m.find("missing") == nullptr);
    D_TBL_CHECK(*m.find("units") == "cm");
    D_TBL_CHECK(m.erase("units"));
    D_TBL_CHECK(!m.erase("units"));
    D_TBL_CHECK(m.size() == 1);
    m.clear();
    D_TBL_CHECK(m.empty());

    D_TBL_CHECK((std::is_base_of<DContainerMetadata<>,
                DTableMetadata<>>::value));
    D_TBL_CHECK(is_table_metadata<DTableMetadata<>>::value);
    D_TBL_CHECK(is_table_metadata_v<const DTableMetadata<>&>);
    D_TBL_CHECK(!is_table_metadata<DContainerMetadata<>>::value);

    return true;
}

/*
tests_table_metadata_conformance
  column and row headers, and whether they fit a table
  Tests the following:
  - no headers at first; set, read, mutate and clear each kind
  - unset headers conform to any shape
  - set headers conform exactly when their extent equals the dimension;
    conforms_to checks both against a table's counts
  - a header type with no extent conforms trivially
*/
bool tests_table_metadata_conformance()
{
    DTableMetadata<> m;
    D_TBL_CHECK(!m.has_column_headers());
    D_TBL_CHECK(!m.has_row_headers());
    D_TBL_CHECK(m.conforms_columns(7));
    D_TBL_CHECK(m.conforms_to(tb_shape{ 5, 7 }));

    DHeaderStack<std::string> cols;
    cols.push_back(make_header_level<std::string>({ "id", "x", "y", "z" }));
    m.set_column_headers(cols);
    D_TBL_CHECK(m.has_column_headers());
    D_TBL_CHECK(header_extent(m.column_headers()) == 4);
    D_TBL_CHECK(m.conforms_columns(4));
    D_TBL_CHECK(!m.conforms_columns(3));

    DHeaderStack<std::string> rows;
    rows.push_back(make_header_level<std::string>({ "r0", "r1" }));
    m.set_row_headers(rows);
    D_TBL_CHECK(m.has_row_headers());
    D_TBL_CHECK(m.conforms_rows(2));
    D_TBL_CHECK(m.conforms_to(tb_shape{ 2, 4 }));
    D_TBL_CHECK(!m.conforms_to(tb_shape{ 3, 4 }));
    D_TBL_CHECK(!m.conforms_to(tb_shape{ 2, 5 }));

    m.row_headers().back().push_back(DHeaderCell<std::string>("r2"));
    D_TBL_CHECK(m.conforms_rows(3));

    m.clear_row_headers();
    D_TBL_CHECK(!m.has_row_headers());
    D_TBL_CHECK(m.conforms_rows(99));
    m.clear_column_headers();
    D_TBL_CHECK(m.conforms_to(tb_shape{ 1, 1 }));

    // labels with no header_extent overload: conformance is not checkable
    DTableMetadata<std::string, std::string, std::vector<int>> plain;
    plain.set_column_headers(std::vector<int>({ 1, 2 }));
    D_TBL_CHECK(plain.has_column_headers());
    D_TBL_CHECK(plain.conforms_columns(17));

    return true;
}

NS_END  // testing
NS_END  // djinterp
