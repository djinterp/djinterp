/*******************************************************************************
* djinterp [cpp][container]                      table_tests_builder_options.cpp
*
* table_builder.hpp with table_options.hpp: options riding on a builder.
*
*   Converted on 2026.11.16 from test_table_builder_options.cpp, an assert-based
* program no leaf built: each assertion is one check, in its order.
*
*
* path:      /tests/djinterp/cpp/container/table/table_tests_builder_options.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.16
*                                                            revised: 2026.11.16
*******************************************************************************/
// std
#include <type_traits>  // std::is_same
// djinterp
#include "table_tests.hpp"  // D_TBL_CHECK, the declarations
#include <djinterp/cpp/container/table/table_builder.hpp>  // table_builder.hpp


NS_DJINTERP
NS_TESTING

// The fixtures of the original program, in its order.  Unnamed, so a
// fixture name reused by another section never breaks the one-definition
// rule.
namespace
{

// djinterp
struct A{}; struct B{}; struct C{}; struct D{};

// options declared INLINE among the declarators
using t = DTableBuilder< DColumns<int,int,int>,
                         DAllowJagged,
                         DContainerOptBounds<DContainerBounds::bounded>,
                         DRow<A,B,C> >;
// the axis half is routed to the container's own reader

// the strictness actually gates the declaration
// a SHORT row (2 cells in a 3-column table) is admitted under pad ...
using padded = DTableBuilder< DColumns<int,int,int>, DPadCounts, DRow<A,B> >;
// ... and under lenient
using lax = DTableBuilder< DColumns<int,int,int>, DLenientCounts, DRow<A,B> >;
// a LONG row (4 cells) is admitted under truncate
using trunc = DTableBuilder< DColumns<int,int,int>, DTruncateCounts, DRow<A,B,C,D> >;
// exact (the default) demands one-to-one
using exactly = DTableBuilder< DColumns<int,int,int>, DRow<A,B,C> >;

// a merged cell counts for its whole box when fitting the width
using merged_fit = DTableBuilder< DColumns<int,int,int>, DRow<A, DMergedCell<1,2,B>> >;

// a flat rows<> must fill whole rows under exact
using flat_ok = DTableBuilder< DColumns<int,int>, DRows<A,B,C,D> >;
using flat_partial = DTableBuilder< DColumns<int,int>, DLenientCounts, DRows<A,B,C> >;

}  // namespace


/*
tests_table_builder_options
  options ride on a builder and reach its shape
  Tests the following:
  - each assertion of the original block, as one check, in its order
*/
bool tests_table_builder_options()
{
    D_TBL_CHECK(t::width == 3 && t::height == 1);  // fold ignores the options
    D_TBL_CHECK(t::shape_strictness == DShapePolicy::jagged);  // policy read
    // default grade
    D_TBL_CHECK(t::cell_count_strictness == DTableStrictness::exact);
    // axis honoured
    D_TBL_CHECK(container_axis_value_v<t::axis_options_type,
                DContainerAxis::bounds, DContainerBounds::unbounded> == DContainerBounds::bounded);
    D_TBL_CHECK(t::table_options_type::size == 1);  // 1 table option
    D_TBL_CHECK(t::axis_options_type::size == 1);  // 1 axis option
    D_TBL_CHECK(padded::widths_conform);  // pad admits a shortfall
    D_TBL_CHECK(lax::widths_conform);  // lenient admits it
    D_TBL_CHECK(trunc::widths_conform);  // truncate admits a surplus
    D_TBL_CHECK(exactly::widths_conform);  // exact match conforms
    D_TBL_CHECK(merged_fit::widths_conform);  // 1 + 2 = 3 fits exactly
    // 4/2 = 2 rows
    D_TBL_CHECK(flat_ok::widths_conform && flat_ok::height == 2);
    // lenient admits a partial last row
    D_TBL_CHECK(flat_partial::widths_conform);

    return true;
}

NS_END  // testing
NS_END  // djinterp
