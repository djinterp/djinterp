/*******************************************************************************
* djinterp [cpp][container]                                arena_tests_links.cpp
*
*   arena.hpp sections I to IV: the link flags, tree_link_policy's link count
* and index assignment, and the five preset policies.
*
*
* path:      /tests/djinterp/cpp/container/arena/arena_tests_links.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.16
*                                                            revised: 2026.11.16
*******************************************************************************/
// djinterp
#include "arena_tests.hpp"  // D_AN_CHECK, fixtures, the declarations
#include <djinterp/cpp/container/arena/arena.hpp>  // the header under test


NS_DJINTERP
NS_TESTING

/*
tests_arena_link_flags
  the seven link flags are distinct single bits, in the documented order
  Tests the following:
  - first_child, next_sibling, parent, prev_sibling and last_child are 1 to 16
  - left and right are 32 and 64
  - no two flags share a bit, so any combination is unambiguous
*/
bool tests_arena_link_flags()
{
    const unsigned flags[] = { DTreeLink::first_child,  DTreeLink::next_sibling,
                               DTreeLink::parent,       DTreeLink::prev_sibling,
                               DTreeLink::last_child,   DTreeLink::left,
                               DTreeLink::right };
    unsigned seen = 0;

    D_AN_CHECK(DTreeLink::first_child  == 1u);
    D_AN_CHECK(DTreeLink::next_sibling == 2u);
    D_AN_CHECK(DTreeLink::parent       == 4u);
    D_AN_CHECK(DTreeLink::prev_sibling == 8u);
    D_AN_CHECK(DTreeLink::last_child   == 16u);
    D_AN_CHECK(DTreeLink::left         == 32u);
    D_AN_CHECK(DTreeLink::right        == 64u);

    for (unsigned f : flags)
    {
        D_AN_CHECK((f & (f - 1u)) == 0u);   // a single bit
        D_AN_CHECK((seen & f) == 0u);       // not seen before
        seen |= f;
    }

    D_AN_CHECK(seen == 127u);

    return true;
}

/*
tests_arena_link_policy_presets
  each preset has the links its name says, and only those
  Tests the following:
  - lcrs: first_child and next_sibling (2 links)
  - parented_lcrs: lcrs plus parent (3)
  - full_nary: all five n-ary links (5)
  - binary: left and right (2); parented_binary: plus parent (3)
  - each preset's flags member is the OR of its links
*/
bool tests_arena_link_policy_presets()
{
    D_AN_CHECK(DLcrsLinkPolicy::num_links == 2);
    D_AN_CHECK(DLcrsLinkPolicy::has_first_child);
    D_AN_CHECK(DLcrsLinkPolicy::has_next_sibling);
    D_AN_CHECK(!DLcrsLinkPolicy::has_parent);
    D_AN_CHECK(!DLcrsLinkPolicy::has_prev_sibling);
    D_AN_CHECK(!DLcrsLinkPolicy::has_last_child);
    D_AN_CHECK(!DLcrsLinkPolicy::has_left);
    D_AN_CHECK(!DLcrsLinkPolicy::has_right);
    D_AN_CHECK(DLcrsLinkPolicy::flags ==
               (DTreeLink::first_child | DTreeLink::next_sibling));

    D_AN_CHECK(DParentedLcrsLinkPolicy::num_links == 3);
    D_AN_CHECK(DParentedLcrsLinkPolicy::has_parent);
    D_AN_CHECK(!DParentedLcrsLinkPolicy::has_prev_sibling);

    D_AN_CHECK(DFullNaryLinkPolicy::num_links == 5);
    D_AN_CHECK(DFullNaryLinkPolicy::has_first_child);
    D_AN_CHECK(DFullNaryLinkPolicy::has_next_sibling);
    D_AN_CHECK(DFullNaryLinkPolicy::has_parent);
    D_AN_CHECK(DFullNaryLinkPolicy::has_prev_sibling);
    D_AN_CHECK(DFullNaryLinkPolicy::has_last_child);
    D_AN_CHECK(!DFullNaryLinkPolicy::has_left);
    D_AN_CHECK(!DFullNaryLinkPolicy::has_right);
    D_AN_CHECK(DFullNaryLinkPolicy::flags == 31u);

    D_AN_CHECK(DBinaryLinkPolicy::num_links == 2);
    D_AN_CHECK(DBinaryLinkPolicy::has_left);
    D_AN_CHECK(DBinaryLinkPolicy::has_right);
    D_AN_CHECK(!DBinaryLinkPolicy::has_first_child);
    D_AN_CHECK(!DBinaryLinkPolicy::has_parent);
    D_AN_CHECK(DBinaryLinkPolicy::flags ==
               (DTreeLink::left | DTreeLink::right));

    D_AN_CHECK(DParentedBinaryLinkPolicy::num_links == 3);
    D_AN_CHECK(DParentedBinaryLinkPolicy::has_parent);
    D_AN_CHECK(DParentedBinaryLinkPolicy::has_left);
    D_AN_CHECK(DParentedBinaryLinkPolicy::has_right);

    return true;
}

/*
tests_arena_link_policy_indices
  enabled links take sequential indices in flag-bit order; disabled links read
  std::size_t(-1) (the header said null_node until 2026.11.16)
  Tests the following:
  - full_nary: first_child 0, next_sibling 1, parent 2, prev_sibling 3,
    last_child 4; left and right disabled
  - parented_binary: parent (bit 4) comes before left (32) and right (64)
  - a policy of right alone puts right at index 0
  - every enabled index is below num_links
*/
bool tests_arena_link_policy_indices()
{
    using right_only = DTreeLinkPolicy<DTreeLink::right>;
    const std::size_t none = static_cast<std::size_t>(-1);

    D_AN_CHECK(DFullNaryLinkPolicy::first_child_idx  == 0);
    D_AN_CHECK(DFullNaryLinkPolicy::next_sibling_idx == 1);
    D_AN_CHECK(DFullNaryLinkPolicy::parent_idx       == 2);
    D_AN_CHECK(DFullNaryLinkPolicy::prev_sibling_idx == 3);
    D_AN_CHECK(DFullNaryLinkPolicy::last_child_idx   == 4);
    D_AN_CHECK(DFullNaryLinkPolicy::left_idx         == none);
    D_AN_CHECK(DFullNaryLinkPolicy::right_idx        == none);

    D_AN_CHECK(DParentedBinaryLinkPolicy::parent_idx == 0);
    D_AN_CHECK(DParentedBinaryLinkPolicy::left_idx   == 1);
    D_AN_CHECK(DParentedBinaryLinkPolicy::right_idx  == 2);
    D_AN_CHECK(DParentedBinaryLinkPolicy::first_child_idx == none);

    D_AN_CHECK(DLcrsLinkPolicy::first_child_idx  == 0);
    D_AN_CHECK(DLcrsLinkPolicy::next_sibling_idx == 1);
    D_AN_CHECK(DLcrsLinkPolicy::parent_idx       == none);

    D_AN_CHECK(right_only::num_links == 1);
    D_AN_CHECK(right_only::right_idx == 0);
    D_AN_CHECK(right_only::left_idx  == none);

    D_AN_CHECK(DTreeLinkPolicy<0u>::num_links == 0);

    return true;
}

NS_END  // testing
NS_END  // djinterp
