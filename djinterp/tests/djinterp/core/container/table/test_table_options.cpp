/*******************************************************************************
* djinterp [core]                                         test_table_options.cpp
*
*
* path:      /tests/djinterp/core/container/table/test_table_options.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "djinterp/core/container/table/table_options.hpp"   // pulls option_set.hpp (guarded)

using namespace djinterp;

// a set built from named options
using opts = table_option_set<allow_jagged, lenient_counts, trim_cells>;

// explicitly-set categories read back their grade
static_assert(shape_of<opts>      == shape_policy::jagged,               "shape jagged");
static_assert(cell_count_of<opts> == table_strictness::lenient,          "cc lenient");
static_assert(trim_of<opts>       == trim_policy::trim,                  "trim");

// unset categories fall through to the defaults
static_assert(header_of<opts>          == header_policy::optional_separator, "hdr default");
static_assert(anchor_of<opts>          == anchor_policy::lexicographic_least,"anchor default");
static_assert(pipe_of<opts>            == pipe_policy::require_borders,       "pipe default");
static_assert(type_conformance_of<opts> == type_policy::require,              "type default");
static_assert(placeholder_fit_of<opts> == table_strictness::exact,           "pf default");
static_assert(domain_of<opts>          == domain_policy::ignore,             "domain default");

// the parameterized alias forms work too
using opts3 = table_option_set< shape_opt<shape_policy::sparse>,
                                cell_count_opt<table_strictness::pad>,
                                placeholder_fit_opt<table_strictness::truncate>,
                                anchor_opt<anchor_policy::declared> >;
static_assert(shape_of<opts3>          == shape_policy::sparse,     "shape sparse");
static_assert(cell_count_of<opts3>     == table_strictness::pad,    "cc pad");
static_assert(placeholder_fit_of<opts3> == table_strictness::truncate, "pf truncate");
static_assert(anchor_of<opts3>         == anchor_policy::declared,  "anchor declared");

// an empty set: everything defaults
using none = table_option_set<>;
static_assert(shape_of<none>      == shape_policy::rectangular,   "empty->rect");
static_assert(cell_count_of<none> == table_strictness::exact,     "empty->exact");

// a plain option_set (not via the alias) consumes identically
using opts2 = option_set<strict_counts, enforce_domain>;
static_assert(cell_count_of<opts2> == table_strictness::exact,   "cc exact");
static_assert(domain_of<opts2>     == domain_policy::enforce,    "domain enforce");

// detection
static_assert(is_table_option_v<allow_jagged>,                 "ito yes");
static_assert(is_table_option_v<shape_opt<shape_policy::jagged>>, "ito yes 2");
static_assert(!is_table_option_v<int>,                         "ito no (int)");
static_assert(!is_table_option_v<option<0, int>>,              "ito no (foreign key)");
static_assert(is_table_option_pack_v<allow_jagged, lenient_counts>,   "pack yes");
static_assert(!is_table_option_pack_v<allow_jagged, int>,            "pack no");

int main() { return 0; }
