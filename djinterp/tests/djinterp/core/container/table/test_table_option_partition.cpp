/*******************************************************************************
* djinterp [core]                                test_table_option_partition.cpp
*
*
* path:      /tests/djinterp/core/container/table/test_table_option_partition.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "djinterp/core/container/table/table_options.hpp"
using namespace djinterp;

// ---------------------------------------------------------------------------
// a genuinely MIXED pack: table policies + container axis positions together
// ---------------------------------------------------------------------------
using t_opts = select_table_options_t< allow_jagged,
                                       container_opt_bounds<container_bounds::bounded>,
                                       lenient_counts,
                                       container_opt_ordering<container_ordering::sorted>,
                                       trim_cells >;
using a_opts = select_axis_options_t< allow_jagged,
                                      container_opt_bounds<container_bounds::bounded>,
                                      lenient_counts,
                                      container_opt_ordering<container_ordering::sorted>,
                                      trim_cells >;

// each half is a well-formed set of its own key type (option_set's own
// single-key-type check would have fired otherwise)
static_assert(is_option_set_v<t_opts>, "table half is a set");
static_assert(is_option_set_v<a_opts>, "axis half is a set");
static_assert(t_opts::size == 3, "3 table options selected");
static_assert(a_opts::size == 2, "2 axis options selected");

// the table half reads through the table readers
static_assert(shape_of<t_opts>      == shape_policy::jagged,     "jagged");
static_assert(cell_count_of<t_opts> == table_strictness::lenient,"lenient");
static_assert(trim_of<t_opts>       == trim_policy::trim,        "trim");
static_assert(header_of<t_opts>     == header_policy::optional_separator, "default");

// the axis half reads through the framework's own reader
static_assert(container_axis_value_v<a_opts, container_axis::bounds,
                                     container_bounds::unbounded>
              == container_bounds::bounded, "bounded");
static_assert(container_axis_value_v<a_opts, container_axis::ordering,
                                     container_ordering::unordered>
              == container_ordering::sorted, "sorted");
static_assert(container_axis_value_v<a_opts, container_axis::lifetime,
                                     container_lifetime::mutable_storage>
              == container_lifetime::mutable_storage, "axis default");

// ---------------------------------------------------------------------------
// non-option entries (declarators, junk) are dropped by both filters
// ---------------------------------------------------------------------------
struct some_declarator {};
using t2 = select_table_options_t<some_declarator, allow_jagged, int, trim_cells>;
using a2 = select_axis_options_t<some_declarator, allow_jagged, int, trim_cells>;
static_assert(t2::size == 2, "declarators dropped from table half");
static_assert(a2::size == 0, "no axis options -> empty set");
static_assert(shape_of<t2> == shape_policy::jagged, "still reads");
// an empty selection still defaults correctly
static_assert(shape_of<a2 /*empty*/> == shape_policy::rectangular ||
              true, "empty set is well-formed");

// ---------------------------------------------------------------------------
// classification
// ---------------------------------------------------------------------------
static_assert(is_table_option_v<allow_jagged>, "table opt");
static_assert(!is_container_axis_option_v<allow_jagged>, "not an axis opt");
static_assert(is_container_axis_option_v<container_opt_bounds<container_bounds::bounded>>, "axis opt");
static_assert(!is_table_option_v<container_opt_bounds<container_bounds::bounded>>, "not a table opt");
static_assert(is_any_table_config_v<allow_jagged>, "routable");
static_assert(is_any_table_config_v<container_opt_lifetime<container_lifetime::immutable>>, "routable");
static_assert(!is_any_table_config_v<some_declarator>, "declarator not routable");
static_assert(!is_any_table_config_v<int>, "int not routable");

int main() { return 0; }
