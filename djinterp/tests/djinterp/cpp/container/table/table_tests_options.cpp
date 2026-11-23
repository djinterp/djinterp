/*******************************************************************************
* djinterp [cpp][container]                              table_tests_options.cpp
*
* table_options.hpp: the option vocabulary and its defaults.
*
*   Converted on 2026.11.16 from test_table_options.cpp, an assert-based
* program no leaf built: each assertion is one check, in its order.
*
*
* path:      /tests/djinterp/cpp/container/table/table_tests_options.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.16
*                                                            revised: 2026.11.16
*******************************************************************************/
// std
#include <type_traits>  // std::is_same
// djinterp
#include "table_tests.hpp"  // D_TBL_CHECK, the declarations
#include <djinterp/cpp/container/table/table_options.hpp>   // pulls option_set.hpp (guarded)


NS_DJINTERP
NS_TESTING

// The fixtures of the original program, in its order.  Unnamed, so a
// fixture name reused by another section never breaks the one-definition
// rule.
namespace
{

// djinterp


// a set built from named options
using opts = DTableOptionSet<DAllowJagged, DLenientCounts, DTrimCells>;

// explicitly-set categories read back their grade

// unset categories fall through to the defaults

// the parameterized alias forms work too
using opts3 = DTableOptionSet< DShapeOpt<DShapePolicy::sparse>,
                                DCellCountOpt<DTableStrictness::pad>,
                                DPlaceholderFitOpt<DTableStrictness::truncate>,
                                DAnchorOpt<DAnchorPolicy::declared> >;

// an empty set: everything defaults
using none = DTableOptionSet<>;

// a plain option_set (not via the alias) consumes identically
using opts2 = option_set<DStrictCounts, DEnforceDomain>;

// detection

}  // namespace


/*
tests_table_options
  the option vocabulary and its defaults
  Tests the following:
  - each assertion of the original block, as one check, in its order
*/
bool tests_table_options()
{
    D_TBL_CHECK(shape_of<opts> == DShapePolicy::jagged);  // shape jagged
    // cc lenient
    D_TBL_CHECK(cell_count_of<opts> == DTableStrictness::lenient);
    D_TBL_CHECK(trim_of<opts> == DTrimPolicy::trim);  // trim
    // hdr default
    D_TBL_CHECK(header_of<opts> == DHeaderPolicy::optional_separator);
    // anchor default
    D_TBL_CHECK(anchor_of<opts> == DAnchorPolicy::lexicographic_least);
    D_TBL_CHECK(pipe_of<opts> == DPipePolicy::require_borders);  // pipe default
    // type default
    D_TBL_CHECK(type_conformance_of<opts> == DTypePolicy::require);
    // pf default
    D_TBL_CHECK(placeholder_fit_of<opts> == DTableStrictness::exact);
    D_TBL_CHECK(domain_of<opts> == DDomainPolicy::ignore);  // domain default
    D_TBL_CHECK(shape_of<opts3> == DShapePolicy::sparse);  // shape sparse
    D_TBL_CHECK(cell_count_of<opts3> == DTableStrictness::pad);  // cc pad
    // pf truncate
    D_TBL_CHECK(placeholder_fit_of<opts3> == DTableStrictness::truncate);
    // anchor declared
    D_TBL_CHECK(anchor_of<opts3> == DAnchorPolicy::declared);
    D_TBL_CHECK(shape_of<none> == DShapePolicy::rectangular);  // empty->rect
    // empty->exact
    D_TBL_CHECK(cell_count_of<none> == DTableStrictness::exact);
    D_TBL_CHECK(cell_count_of<opts2> == DTableStrictness::exact);  // cc exact
    D_TBL_CHECK(domain_of<opts2> == DDomainPolicy::enforce);  // domain enforce
    D_TBL_CHECK(is_table_option_v<DAllowJagged>);  // ito yes
    // ito yes 2
    D_TBL_CHECK(is_table_option_v<DShapeOpt<DShapePolicy::jagged>>);
    D_TBL_CHECK(!is_table_option_v<int>);  // ito no (int)
    D_TBL_CHECK(!is_table_option_v<option<0, int>>);  // ito no (foreign key)
    // pack yes
    D_TBL_CHECK(is_table_option_pack_v<DAllowJagged, DLenientCounts>);
    D_TBL_CHECK(!is_table_option_pack_v<DAllowJagged, int>);  // pack no

    return true;
}

NS_END  // testing
NS_END  // djinterp
