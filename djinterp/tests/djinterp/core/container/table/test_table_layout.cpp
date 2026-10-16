/*******************************************************************************
* djinterp [core]                                          test_table_layout.cpp
*
*
* path:      /tests/djinterp/core/container/table/test_table_layout.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <cassert>
#include <iostream>
#include <type_traits>
// djinterp
#include "djinterp/core/container/table/table_layout.hpp"
using namespace djinterp;

// ---------------------------------------------------------------------------
// I. region basics
// ---------------------------------------------------------------------------
using single = region<0, 0, 1, 1>;          // a singleton cell
using wide   = region<0, 1, 1, 3>;          // "coordinate" spanning x,y,z (1x3)
using block  = region<2, 0, 2, 2>;          // a 2x2 block

static_assert(single::extent == 1,          "singleton extent");
static_assert(!single::is_merge,            "singleton is not a merge");
static_assert(wide::extent == 3,            "wide extent 3");
static_assert(wide::is_merge,               "wide is a merge");
static_assert(wide::merged_along_cols,      "wide merged along cols");
static_assert(!wide::merged_along_rows,     "wide not merged along rows");
static_assert(block::extent == 4,           "block extent 4");
// anchor = min_lex R_C
static_assert(wide::anchor_row == 0 && wide::anchor_col == 1,  "wide anchor");
static_assert(block::anchor_row == 2 && block::anchor_col == 0, "block anchor");

// ---------------------------------------------------------------------------
// II. relations
// ---------------------------------------------------------------------------
static_assert(region_contains<wide, 0, 1>::value,  "wide contains (0,1)");
static_assert(region_contains<wide, 0, 3>::value,  "wide contains (0,3)");
static_assert(!region_contains<wide, 0, 4>::value, "wide excludes (0,4)");
static_assert(!region_contains<wide, 1, 2>::value, "wide excludes row 1");
static_assert(!regions_overlap<wide, block>::value, "wide/block disjoint");
static_assert(regions_overlap<block, region<3,1,1,1>>::value, "block overlaps inner");
static_assert(region_within<region<2,0,1,1>, block>::value, "inner within block");
static_assert(!region_within<region<2,0,3,1>, block>::value, "too tall not within");

// ---------------------------------------------------------------------------
// III. the "coordinate over x,y,z" grouped header, as a cover
//      row 0:  |     |      coordinate      |     (1x3 merge at (0,1))
//      row 1:  | id  |  x  |  y  |  z |           (singletons)
// ---------------------------------------------------------------------------
using coord_layout = layout<wide>;
static_assert(coord_layout::merge_count == 1,  "one merge");
static_assert(coord_layout::has_merges,        "has merges");

// every position under the merge reports the merge as owner -> same anchor
static_assert(std::is_same<coord_layout::owner_of<0,1>, wide>::value, "owner (0,1)");
static_assert(std::is_same<coord_layout::owner_of<0,2>, wide>::value, "owner (0,2)");
static_assert(std::is_same<coord_layout::owner_of<0,3>, wide>::value, "owner (0,3)");
// T[i] = T[anchor(cell(i))]: (0,2) and (0,3) defer to (0,1)
static_assert(coord_layout::owner_of<0,2>::anchor_col == 1, "(0,2) defers to col 1");
static_assert(coord_layout::owner_of<0,3>::anchor_col == 1, "(0,3) defers to col 1");
// an uncovered position is its own singleton
static_assert(std::is_same<coord_layout::owner_of<0,0>, region<0,0,1,1>>::value, "singleton (0,0)");
static_assert(std::is_same<coord_layout::owner_of<1,2>, region<1,2,1,1>>::value, "singleton (1,2)");
static_assert(coord_layout::owner_of<1,2>::anchor_col == 2, "singleton anchors itself");

// trivial layout = Gamma_0
static_assert(trivial_layout::wears_no_merges,  "Gamma_0 has no merges");
static_assert(std::is_same<trivial_layout::owner_of<5,7>, region<5,7,1,1>>::value, "Gamma_0 all singletons");

// ---------------------------------------------------------------------------
// IV. validity: in-bounds + pairwise disjoint
// ---------------------------------------------------------------------------
static_assert(layout_valid<coord_layout, 2, 4>::value,   "valid in 2x4");
static_assert(!layout_valid<coord_layout, 2, 3>::value,  "out of bounds in 2x3");
static_assert(layout_valid<layout<wide, block>, 4, 4>::value, "two disjoint merges ok");
// overlapping merges are NOT a valid cover
using overlapping = layout<region<0,0,2,2>, region<1,1,2,2>>;
static_assert(!layout_valid<overlapping, 4, 4>::value,   "overlapping merges invalid");
// touching-but-not-overlapping is fine
using touching = layout<region<0,0,1,2>, region<0,2,1,2>>;
static_assert(layout_valid<touching, 1, 4>::value,       "adjacent merges ok");
static_assert(layout_valid<trivial_layout, 3, 3>::value, "Gamma_0 always valid");

// ---------------------------------------------------------------------------
// V. split: pieces must tile the parent exactly
// ---------------------------------------------------------------------------
// split a 1x3 into three 1x1s
using good_split = split<wide, region<0,1,1,1>, region<0,2,1,1>, region<0,3,1,1>>;
static_assert(good_split::piece_count == 3, "3 pieces");
static_assert(split_valid<good_split>::value, "exact tiling valid");
// a piece missing -> area short
using short_split = split<wide, region<0,1,1,1>, region<0,2,1,1>>;
static_assert(!split_valid<short_split>::value, "incomplete tiling invalid");
// overlapping pieces -> not a partition
using overlap_split = split<block, region<2,0,2,1>, region<2,0,2,2>>;
static_assert(!split_valid<overlap_split>::value, "overlapping pieces invalid");
// a piece outside the parent
using outside_split = split<wide, region<0,1,1,1>, region<0,2,1,1>, region<5,5,1,1>>;
static_assert(!split_valid<outside_split>::value, "piece outside parent invalid");
// 2x2 block split into two 1x2 halves
using half_split = split<block, region<2,0,1,2>, region<3,0,1,2>>;
static_assert(split_valid<half_split>::value, "halves tile the block");

// ---------------------------------------------------------------------------
// VI. detection
// ---------------------------------------------------------------------------
static_assert(is_region<wide>::value,        "is_region");
static_assert(!is_region<int>::value,        "not region");
static_assert(is_layout<coord_layout>::value,"is_layout");
static_assert(is_layout<const trivial_layout&>::value, "is_layout strips cvref");
static_assert(is_split<good_split>::value,   "is_split");
static_assert(!is_split<wide>::value,        "region is not split");

// constexpr region_value works at compile time too
constexpr region_value rv(0, 1, 1, 3);
static_assert(rv.extent() == 3,              "rv extent");
static_assert(rv.is_merge(),                 "rv is merge");
static_assert(rv.contains(0, 3),             "rv contains");
static_assert(!rv.contains(0, 4),            "rv excludes");
static_assert(rv.anchor_col() == 1,          "rv anchor");

int main()
{
    // -----------------------------------------------------------------------
    // VII. runtime layout: same vocabulary, value level
    // -----------------------------------------------------------------------
    runtime_layout L;
    assert(L.wears_no_merges());
    assert(L.owner_of(3, 4).row0 == 3 && L.owner_of(3, 4).col0 == 4);  // singleton
    assert(L.valid(10, 10));

    // the same "coordinate over x,y,z" merge, built as the parser would
    L.add_merge(0, 1, 1, 3);
    assert(L.has_merges());
    assert(L.merge_count() == 1);

    // covered positions defer to the anchor (0,1)
    assert(L.owner_of(0, 2).anchor_col() == 1);
    assert(L.owner_of(0, 3).anchor_col() == 1);
    assert(L.owner_of(0, 2).extent() == 3);
    // uncovered position -> singleton at itself
    assert(L.owner_of(0, 0).anchor_col() == 0);
    assert(!L.owner_of(0, 0).is_merge());
    assert(L.owner_of(1, 2).anchor_row() == 1);

    // bounds
    assert(L.valid(2, 4));
    assert(!L.valid(2, 3));   // the merge runs past width 3

    // overlap rejection
    runtime_layout M;
    M.add_merge(0, 0, 2, 2);
    M.add_merge(1, 1, 2, 2);
    assert(!M.valid(4, 4));

    // adjacent merges are fine
    runtime_layout N;
    N.add_merge(0, 0, 1, 2);
    N.add_merge(0, 2, 1, 2);
    assert(N.valid(1, 4));

    std::cout << "runtime layout: all assertions passed\n";
    return 0;
}
