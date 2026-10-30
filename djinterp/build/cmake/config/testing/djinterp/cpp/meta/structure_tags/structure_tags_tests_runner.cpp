#define DTEST_SPEC_MODE            // both suites expose their spec providers here

// std
#include <string>
// djinterp -- both tag suite headers; in spec mode each contributes only its
// spec provider (flat_spec / hierarchical_spec), no fixtures, so they coexist
// in this single translation unit without clashing.
#include "flat_tests.hpp"
#include "hierarchical_tests.hpp"

// structure_tags: an AGGREGATE runner that exercises both structural-kind tag
// suites in one executable.  This is possible because every test in the two
// suites now carries a module-unique name (tests_tag_flat_* / tests_tag_
// hierarchical_*) and each suite's fixtures are module-namespaced
// (flat_depth_probe / hierarchical_depth_probe) -- so linking both suites'
// section TUs together produces no duplicate symbols.
int main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("structure_tags_tests.pdf"));

    // Run both modules; combine their exit codes so the aggregate fails if
    // either module has a failing test.
    int rc = 0;
    rc |= dt::run_module(tt::flat_spec(),
                         "flat.hpp unit tests",
                         "The FLAT structural-kind tag.",
                         opts);
    rc |= dt::run_module(tt::hierarchical_spec(),
                         "hierarchical.hpp unit tests",
                         "The HIERARCHICAL structural-kind tag.",
                         opts);
    return rc;
}
