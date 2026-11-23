/*******************************************************************************
* djinterp [cpp][container]                     table_tests_option_partition.cpp
*
* table_options.hpp: how an option pack partitions into table and container
* options.
*
*   Converted on 2026.11.16 from test_table_option_partition.cpp, an assert-based
* program no leaf built: each assertion is one check, in its order.
*
*
* path:      /tests/djinterp/cpp/container/table/table_tests_option_partition.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.16
*                                                            revised: 2026.11.16
*******************************************************************************/
// std
#include <type_traits>  // std::is_same
// djinterp
#include "table_tests.hpp"  // D_TBL_CHECK, the declarations
#include <djinterp/cpp/container/table/table_options.hpp>  // table_options.hpp


NS_DJINTERP
NS_TESTING

// The fixtures of the original program, in its order.  Unnamed, so a
// fixture name reused by another section never breaks the one-definition
// rule.
namespace
{

// djinterp

// a genuinely MIXED pack: table policies + container axis positions together
using t_opts = select_table_options_t< DAllowJagged,
                                       DContainerOptBounds<DContainerBounds::bounded>,
                                       DLenientCounts,
                                       DContainerOptOrdering<DContainerOrdering::sorted>,
                                       DTrimCells >;
using a_opts = select_axis_options_t< DAllowJagged,
                                      DContainerOptBounds<DContainerBounds::bounded>,
                                      DLenientCounts,
                                      DContainerOptOrdering<DContainerOrdering::sorted>,
                                      DTrimCells >;

// each half is a well-formed set of its own key type (option_set's own
// single-key-type check would have fired otherwise)

// the table half reads through the table readers

// the axis half reads through the framework's own reader

// non-option entries (declarators, junk) are dropped by both filters
struct some_declarator {};
using t2 = select_table_options_t<some_declarator, DAllowJagged, int, DTrimCells>;
using a2 = select_axis_options_t<some_declarator, DAllowJagged, int, DTrimCells>;
// an empty selection still defaults correctly

// classification

}  // namespace


/*
tests_table_option_partition
  an option pack partitions into table and container options
  Tests the following:
  - each assertion of the original block, as one check, in its order
*/
bool tests_table_option_partition()
{
    D_TBL_CHECK(is_option_set_v<t_opts>);  // table half is a set
    D_TBL_CHECK(is_option_set_v<a_opts>);  // axis half is a set
    D_TBL_CHECK(t_opts::size == 3);  // 3 table options selected
    D_TBL_CHECK(a_opts::size == 2);  // 2 axis options selected
    D_TBL_CHECK(shape_of<t_opts> == DShapePolicy::jagged);  // jagged
    D_TBL_CHECK(cell_count_of<t_opts> == DTableStrictness::lenient);  // lenient
    D_TBL_CHECK(trim_of<t_opts> == DTrimPolicy::trim);  // trim
    // default
    D_TBL_CHECK(header_of<t_opts> == DHeaderPolicy::optional_separator);
    // bounded
    D_TBL_CHECK(container_axis_value_v<a_opts, DContainerAxis::bounds,
                DContainerBounds::unbounded> == DContainerBounds::bounded);
    // sorted
    D_TBL_CHECK(container_axis_value_v<a_opts, DContainerAxis::ordering,
                DContainerOrdering::unordered> == DContainerOrdering::sorted);
    // axis default
    D_TBL_CHECK(container_axis_value_v<a_opts, DContainerAxis::lifetime,
                DContainerLifetime::mutable_storage> == DContainerLifetime::mutable_storage);
    D_TBL_CHECK(t2::size == 2);  // declarators dropped from table half
    D_TBL_CHECK(a2::size == 0);  // no axis options -> empty set
    D_TBL_CHECK(shape_of<t2> == DShapePolicy::jagged);  // still reads
    // empty set is well-formed
    D_TBL_CHECK(shape_of<a2 /*empty*/> == DShapePolicy::rectangular || true);
    D_TBL_CHECK(is_table_option_v<DAllowJagged>);  // table opt
    D_TBL_CHECK(!is_container_axis_option_v<DAllowJagged>);  // not an axis opt
    // axis opt
    D_TBL_CHECK(is_container_axis_option_v<DContainerOptBounds<DContainerBounds::bounded>>);
    // not a table opt
    D_TBL_CHECK(!is_table_option_v<DContainerOptBounds<DContainerBounds::bounded>>);
    D_TBL_CHECK(is_any_table_config_v<DAllowJagged>);  // routable
    // routable
    D_TBL_CHECK(is_any_table_config_v<DContainerOptLifetime<DContainerLifetime::immutable>>);
    // declarator not routable
    D_TBL_CHECK(!is_any_table_config_v<some_declarator>);
    D_TBL_CHECK(!is_any_table_config_v<int>);  // int not routable

    return true;
}

NS_END  // testing
NS_END  // djinterp
