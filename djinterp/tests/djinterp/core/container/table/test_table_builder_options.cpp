/*******************************************************************************
* djinterp [core]                                 test_table_builder_options.cpp
*
*
* path:      /tests/djinterp/core/container/table/test_table_builder_options.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "djinterp/core/container/table/table_builder.hpp"
using namespace djinterp;
struct A{}; struct B{}; struct C{}; struct D{};

// ---------------------------------------------------------------------------
// options declared INLINE among the declarators
// ---------------------------------------------------------------------------
using t = table_builder< columns<int,int,int>,
                         allow_jagged,
                         container_opt_bounds<container_bounds::bounded>,
                         row<A,B,C> >;
static_assert(t::width == 3 && t::height == 1, "fold ignores the options");
static_assert(t::shape_strictness == shape_policy::jagged, "policy read");
static_assert(t::cell_count_strictness == table_strictness::exact, "default grade");
// the axis half is routed to the container's own reader
static_assert(container_axis_value_v<t::axis_options_type, container_axis::bounds,
                                     container_bounds::unbounded>
              == container_bounds::bounded, "axis honoured");
static_assert(t::table_options_type::size == 1, "1 table option");
static_assert(t::axis_options_type::size  == 1, "1 axis option");

// ---------------------------------------------------------------------------
// the strictness actually gates the declaration
// ---------------------------------------------------------------------------
// a SHORT row (2 cells in a 3-column table) is admitted under pad ...
using padded = table_builder< columns<int,int,int>, pad_counts, row<A,B> >;
static_assert(padded::widths_conform, "pad admits a shortfall");
// ... and under lenient
using lax = table_builder< columns<int,int,int>, lenient_counts, row<A,B> >;
static_assert(lax::widths_conform, "lenient admits it");
// a LONG row (4 cells) is admitted under truncate
using trunc = table_builder< columns<int,int,int>, truncate_counts, row<A,B,C,D> >;
static_assert(trunc::widths_conform, "truncate admits a surplus");
// exact (the default) demands one-to-one
using exactly = table_builder< columns<int,int,int>, row<A,B,C> >;
static_assert(exactly::widths_conform, "exact match conforms");

// a merged cell counts for its whole box when fitting the width
using merged_fit = table_builder< columns<int,int,int>, row<A, merged_cell<1,2,B>> >;
static_assert(merged_fit::widths_conform, "1 + 2 = 3 fits exactly");

// a flat rows<> must fill whole rows under exact
using flat_ok = table_builder< columns<int,int>, rows<A,B,C,D> >;
static_assert(flat_ok::widths_conform && flat_ok::height == 2, "4/2 = 2 rows");
using flat_partial = table_builder< columns<int,int>, lenient_counts, rows<A,B,C> >;
static_assert(flat_partial::widths_conform, "lenient admits a partial last row");

int main() { return 0; }
