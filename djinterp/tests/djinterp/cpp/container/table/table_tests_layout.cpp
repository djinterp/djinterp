/*******************************************************************************
* djinterp [cpp][container]                               table_tests_layout.cpp
*
* table_layout.hpp: regions, their relations, layouts as covers, splits,
* detection, and the runtime layout.
*
*   Converted on 2026.11.16 from test_table_layout.cpp, an assert-based
* program no leaf built: each assertion is one check, in its order.
*
*
* path:      /tests/djinterp/cpp/container/table/table_tests_layout.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.16
*                                                            revised: 2026.11.16
*******************************************************************************/
// std
#include <type_traits>  // std::is_same
// djinterp
#include "table_tests.hpp"  // D_TBL_CHECK, the declarations
#include <type_traits>  // std::is_same
#include <djinterp/cpp/container/table/table_layout.hpp>  // table_layout.hpp


NS_DJINTERP
NS_TESTING

// The fixtures of the original program, in its order.  Unnamed, so a
// fixture name reused by another section never breaks the one-definition
// rule.
namespace
{

// std
// djinterp

using single = DRegion<0, 0, 1, 1>;          // a singleton cell
using wide   = DRegion<0, 1, 1, 3>;          // "coordinate" spanning x,y,z (1x3)
using block  = DRegion<2, 0, 2, 2>;          // a 2x2 block

// anchor = min_lex R_C


//      row 0:  |     |      coordinate      |     (1x3 merge at (0,1))
//      row 1:  | id  |  x  |  y  |  z |           (singletons)
using coord_layout = DTableLayout<wide>;

// every position under the merge reports the merge as owner -> same anchor
// T[i] = T[anchor(cell(i))]: (0,2) and (0,3) defer to (0,1)
// an uncovered position is its own singleton

// trivial layout = Gamma_0

// overlapping merges are NOT a valid cover
using overlapping = DTableLayout<DRegion<0,0,2,2>, DRegion<1,1,2,2>>;
// touching-but-not-overlapping is fine
using touching = DTableLayout<DRegion<0,0,1,2>, DRegion<0,2,1,2>>;

// split a 1x3 into three 1x1s
using good_split = DSplit<wide, DRegion<0,1,1,1>, DRegion<0,2,1,1>, DRegion<0,3,1,1>>;
// a piece missing -> area short
using short_split = DSplit<wide, DRegion<0,1,1,1>, DRegion<0,2,1,1>>;
// overlapping pieces -> not a partition
using overlap_split = DSplit<block, DRegion<2,0,2,1>, DRegion<2,0,2,2>>;
// a piece outside the parent
using outside_split = DSplit<wide, DRegion<0,1,1,1>, DRegion<0,2,1,1>, DRegion<5,5,1,1>>;
// 2x2 block split into two 1x2 halves
using half_split = DSplit<block, DRegion<2,0,1,2>, DRegion<3,0,1,2>>;


// constexpr region_value works at compile time too
constexpr
DRegionValue rv(
    0,
    1,
    1,
    3
);

}  // namespace


/*
tests_table_layout_region_basics
  a region's extent, merge flags and anchor
  Tests the following:
  - each assertion of the original block, as one check, in its order
*/
bool tests_table_layout_region_basics()
{
    D_TBL_CHECK(single::extent == 1);  // singleton extent
    D_TBL_CHECK(!single::is_merge);  // singleton is not a merge
    D_TBL_CHECK(wide::extent == 3);  // wide extent 3
    D_TBL_CHECK(wide::is_merge);  // wide is a merge
    D_TBL_CHECK(wide::merged_along_cols);  // wide merged along cols
    D_TBL_CHECK(!wide::merged_along_rows);  // wide not merged along rows
    D_TBL_CHECK(block::extent == 4);  // block extent 4
    D_TBL_CHECK(wide::anchor_row == 0 && wide::anchor_col == 1);  // wide anchor
    // block anchor
    D_TBL_CHECK(block::anchor_row == 2 && block::anchor_col == 0);

    return true;
}

/*
tests_table_layout_relations
  containment, overlap and nesting of regions
  Tests the following:
  - each assertion of the original block, as one check, in its order
*/
bool tests_table_layout_relations()
{
    D_TBL_CHECK(region_contains<wide, 0, 1>::value);  // wide contains (0,1)
    D_TBL_CHECK(region_contains<wide, 0, 3>::value);  // wide contains (0,3)
    D_TBL_CHECK(!region_contains<wide, 0, 4>::value);  // wide excludes (0,4)
    D_TBL_CHECK(!region_contains<wide, 1, 2>::value);  // wide excludes row 1
    D_TBL_CHECK(!regions_overlap<wide, block>::value);  // wide/block disjoint
    // block overlaps inner
    D_TBL_CHECK(regions_overlap<block, DRegion<3,1,1,1>>::value);
    // inner within block
    D_TBL_CHECK(region_within<DRegion<2,0,1,1>, block>::value);
    // too tall not within
    D_TBL_CHECK(!region_within<DRegion<2,0,3,1>, block>::value);

    return true;
}

/*
tests_table_layout_grouped_header
  a grouped header as a cover: every position's owner
  Tests the following:
  - each assertion of the original block, as one check, in its order
*/
bool tests_table_layout_grouped_header()
{
    D_TBL_CHECK(coord_layout::merge_count == 1);  // one merge
    D_TBL_CHECK(coord_layout::has_merges);  // has merges
    // owner (0,1)
    D_TBL_CHECK(std::is_same<coord_layout::owner_of<0,1>, wide>::value);
    // owner (0,2)
    D_TBL_CHECK(std::is_same<coord_layout::owner_of<0,2>, wide>::value);
    // owner (0,3)
    D_TBL_CHECK(std::is_same<coord_layout::owner_of<0,3>, wide>::value);
    // (0,2) defers to col 1
    D_TBL_CHECK(coord_layout::owner_of<0,2>::anchor_col == 1);
    // (0,3) defers to col 1
    D_TBL_CHECK(coord_layout::owner_of<0,3>::anchor_col == 1);
    // singleton (0,0)
    D_TBL_CHECK(std::is_same<coord_layout::owner_of<0,0>,
                DRegion<0,0,1,1>>::value);
    // singleton (1,2)
    D_TBL_CHECK(std::is_same<coord_layout::owner_of<1,2>,
                DRegion<1,2,1,1>>::value);
    // singleton anchors itself
    D_TBL_CHECK(coord_layout::owner_of<1,2>::anchor_col == 2);
    D_TBL_CHECK(DTrivialLayout::wears_no_merges);  // Gamma_0 has no merges
    // Gamma_0 all singletons
    D_TBL_CHECK(std::is_same<DTrivialLayout::owner_of<5,7>,
                DRegion<5,7,1,1>>::value);

    return true;
}

/*
tests_table_layout_validity
  a layout is valid when in bounds and pairwise disjoint
  Tests the following:
  - each assertion of the original block, as one check, in its order
*/
bool tests_table_layout_validity()
{
    D_TBL_CHECK(layout_valid<coord_layout, 2, 4>::value);  // valid in 2x4
    // out of bounds in 2x3
    D_TBL_CHECK(!layout_valid<coord_layout, 2, 3>::value);
    // two disjoint merges ok
    D_TBL_CHECK(layout_valid<DTableLayout<wide, block>, 4, 4>::value);
    // overlapping merges invalid
    D_TBL_CHECK(!layout_valid<overlapping, 4, 4>::value);
    D_TBL_CHECK(layout_valid<touching, 1, 4>::value);  // adjacent merges ok
    // Gamma_0 always valid
    D_TBL_CHECK(layout_valid<DTrivialLayout, 3, 3>::value);

    return true;
}

/*
tests_table_layout_split
  a split's pieces must tile the parent exactly
  Tests the following:
  - each assertion of the original block, as one check, in its order
*/
bool tests_table_layout_split()
{
    D_TBL_CHECK(good_split::piece_count == 3);  // 3 pieces
    D_TBL_CHECK(split_valid<good_split>::value);  // exact tiling valid
    D_TBL_CHECK(!split_valid<short_split>::value);  // incomplete tiling invalid
    // overlapping pieces invalid
    D_TBL_CHECK(!split_valid<overlap_split>::value);
    // piece outside parent invalid
    D_TBL_CHECK(!split_valid<outside_split>::value);
    D_TBL_CHECK(split_valid<half_split>::value);  // halves tile the block

    return true;
}

/*
tests_table_layout_detection
  the detection traits, and constexpr region_value
  Tests the following:
  - each assertion of the original block, as one check, in its order
*/
bool tests_table_layout_detection()
{
    D_TBL_CHECK(is_region<wide>::value);  // is_region
    D_TBL_CHECK(!is_region<int>::value);  // not region
    D_TBL_CHECK(is_layout<coord_layout>::value);  // is_layout
    // is_layout strips cvref
    D_TBL_CHECK(is_layout<const DTrivialLayout&>::value);
    D_TBL_CHECK(is_split<good_split>::value);  // is_split
    D_TBL_CHECK(!is_split<wide>::value);  // region is not split
    D_TBL_CHECK(rv.extent() == 3);  // rv extent
    D_TBL_CHECK(rv.is_merge());  // rv is merge
    D_TBL_CHECK(rv.contains(0, 3));  // rv contains
    D_TBL_CHECK(!rv.contains(0, 4));  // rv excludes
    D_TBL_CHECK(rv.anchor_col() == 1);  // rv anchor

    return true;
}

/*
tests_table_layout_runtime
  the runtime layout: merges, owners, bounds and overlap
  Tests the following:
  - each assertion of the original block, as one check, in its order
*/
bool tests_table_layout_runtime()
{
    // -----------------------------------------------------------------------
    // VII. runtime layout: same vocabulary, value level
    // -----------------------------------------------------------------------
    DRuntimeLayout L;
    D_TBL_CHECK(L.wears_no_merges());
    // singleton
    D_TBL_CHECK(L.owner_of(3, 4).row0 == 3 && L.owner_of(3, 4).col0 == 4);
    D_TBL_CHECK(L.valid(10, 10));

    // the same "coordinate over x,y,z" merge, built as the parser would
    L.add_merge(0, 1, 1, 3);
    D_TBL_CHECK(L.has_merges());
    D_TBL_CHECK(L.merge_count() == 1);

    // covered positions defer to the anchor (0,1)
    D_TBL_CHECK(L.owner_of(0, 2).anchor_col() == 1);
    D_TBL_CHECK(L.owner_of(0, 3).anchor_col() == 1);
    D_TBL_CHECK(L.owner_of(0, 2).extent() == 3);
    // uncovered position -> singleton at itself
    D_TBL_CHECK(L.owner_of(0, 0).anchor_col() == 0);
    D_TBL_CHECK(!L.owner_of(0, 0).is_merge());
    D_TBL_CHECK(L.owner_of(1, 2).anchor_row() == 1);

    // bounds
    D_TBL_CHECK(L.valid(2, 4));
    D_TBL_CHECK(!L.valid(2, 3));   // the merge runs past width 3

    // overlap rejection
    DRuntimeLayout M;
    M.add_merge(0, 0, 2, 2);
    M.add_merge(1, 1, 2, 2);
    D_TBL_CHECK(!M.valid(4, 4));

    // adjacent merges are fine
    DRuntimeLayout N;
    N.add_merge(0, 0, 1, 2);
    N.add_merge(0, 2, 1, 2);
    D_TBL_CHECK(N.valid(1, 4));

    return true;
}

NS_END  // testing
NS_END  // djinterp
