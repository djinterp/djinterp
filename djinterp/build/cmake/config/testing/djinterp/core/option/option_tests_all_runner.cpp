/******************************************************************************
* djinterp [test]                                   option_tests_all_runner.cpp
*
*   Aggregate runner for the entire option layer.  It unions the ten option
* suites - option, option_set, option_set_compare, option_override,
* option_compose, option_builder, option_factory, option_generator,
* option_diff, and optionator - into one run, lowers them to a single six-kind
* test_suite tree, drives one console report, and (via the option set below)
* renders one PDF per suite bundled into a single .zip.
*
*   WHY FORWARD-DECLARATIONS AND NOT SUITE-HEADER INCLUDES:
*   The event aggregate can #include its suite headers because those suites
* implement DTEST_SPEC_MODE, which drops each header's fixtures so the
* otherwise-colliding fixture names never meet.  The option suites were NOT
* built for that mode: their headers define fixtures at namespace scope
* (arg_a/b/c appears in both option and option_factory; same_v appears in four
* of the suites; ...), so pulling them all into one translation unit would be a
* redefinition storm.  This runner therefore includes NONE of the suite headers
* - it needs only each suite's block-providers, whose signatures are stable
* (`::djinterp::test::block_spec name()`), so they are forward-declared here and
* resolved at link against the section translation units.  Each section TU still
* includes exactly one suite header, so no fixture is ever multiply-defined in a
* single TU; identical cross-suite type/inline-variable definitions satisfy the
* ODR at link.
*
*   OUTPUT VIA TEST_OPTIONS - PER-MODULE PDFs, BUNDLED INTO A .zip:
*   Every knob rides one option set (test_options.hpp):
*     document       -> pdf                    emit PDF (not the txt default)
*     split          -> per_module             one PDF per suite (ten documents)
*     pack           -> archive                bundle those documents...
*     archive_format -> zip                    ...into a single .zip container
*     output_file    -> "option_tests.zip"     the container's path
*   With split == per_module the ten report modules each render to their own PDF
* (option.pdf ... optionator.pdf); pack == archive then collects them into
* option_tests.zip.  The `zip` writer has a self-contained STORED fallback
* (test_zip_store.hpp) that needs no third-party library, so the bundle is
* always produced - a standard .zip that opens by double-click.  For ten LOOSE
* PDFs instead, drop the pack / archive_format lines; for one combined PDF, drop
* the split line too.
*
*   PACKAGING IS OPT-IN in the report builder, so this runner defines
* D_TEST_REPORT_ENABLE_ARCHIVE (below, before the includes) to compile the
* archive path in.  The per-suite runners, which each emit a single PDF, do not
* define it and pay nothing for it.
*
*   FINDING THE FILE:
*   output_file is a RELATIVE name, so the .zip lands in the process's current
* working directory (under Visual Studio, the Debugging working directory, not
* the folder holding the .exe).  Use an absolute path to pin it, or watch the
* console for the "wrote report: ..." line.
*
*   run_suite returns a process exit code - 0 iff every leaf verdict across all
* ten suites passed - which main hands straight back.  To run one suite alone,
* use that suite's own runner.
*
* path:      /build/cmake/config/testing/djinterp/core/option/option_tests_all_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.08
******************************************************************************/

#define D_TEST_REPORT_ENABLE_ARCHIVE   // compile the archive packaging path into the builder

// std
#include <initializer_list>
#include <string>
#include <vector>
// djinterp - the framework only (NOT the suite headers; see the note above)
#include <djinterp/test/test_defaults.hpp>   // module_spec / block_spec, run_suite, test_option_set, archive + zip-store


// ---------------------------------------------------------------------------
// block-provider forward declarations (defined in each suite's section TUs)
// ---------------------------------------------------------------------------
NS_DJINTERP
NS_TESTING

    // option.hpp
    ::djinterp::test::block_spec option_sentinels_block();
    ::djinterp::test::block_spec option_core_block();
    ::djinterp::test::block_spec option_is_option_block();
    ::djinterp::test::block_spec option_concepts_block();

    // option_set.hpp
    ::djinterp::test::block_spec option_set_expand_block();
    ::djinterp::test::block_spec option_set_checks_block();
    ::djinterp::test::block_spec option_set_core_block();
    ::djinterp::test::block_spec option_set_values_block();
    ::djinterp::test::block_spec option_set_queries_block();
    ::djinterp::test::block_spec option_set_concepts_block();

    // option_set_compare.hpp
    ::djinterp::test::block_spec option_set_compare_keylist_block();
    ::djinterp::test::block_spec option_set_compare_congruity_block();
    ::djinterp::test::block_spec option_set_compare_carriers_block();
    ::djinterp::test::block_spec option_set_compare_value_eq_block();

    // option_override.hpp
    ::djinterp::test::block_spec option_override_helpers_block();
    ::djinterp::test::block_spec option_override_merge_block();
    ::djinterp::test::block_spec option_override_lazy_block();
    ::djinterp::test::block_spec option_override_engine_block();
    ::djinterp::test::block_spec option_override_policies_block();

    // option_compose.hpp
    ::djinterp::test::block_spec option_compose_defopt_block();
    ::djinterp::test::block_spec option_compose_with_option_block();
    ::djinterp::test::block_spec option_compose_with_options_block();
    ::djinterp::test::block_spec option_compose_compose_block();

    // option_builder.hpp
    ::djinterp::test::block_spec option_builder_wrap_block();
    ::djinterp::test::block_spec option_builder_lift_block();
    ::djinterp::test::block_spec option_builder_pipeline_block();

    // option_factory.hpp
    ::djinterp::test::block_spec option_factory_carrier_block();
    ::djinterp::test::block_spec option_factory_make_type_block();
    ::djinterp::test::block_spec option_factory_make_value_block();

    // option_generator.hpp
    ::djinterp::test::block_spec option_generator_specs_block();
    ::djinterp::test::block_spec option_generator_parse_block();
    ::djinterp::test::block_spec option_generator_emit_block();
    ::djinterp::test::block_spec option_generator_frontend_block();

    // option_diff.hpp
    ::djinterp::test::block_spec option_diff_ct_keys_block();
    ::djinterp::test::block_spec option_diff_ct_values_block();
    ::djinterp::test::block_spec option_diff_ct_merge_block();
    ::djinterp::test::block_spec option_diff_rt_keys_block();
    ::djinterp::test::block_spec option_diff_rt_values_block();
    ::djinterp::test::block_spec option_diff_rt_merge_block();

    // optionator.hpp
    ::djinterp::test::block_spec optionator_configures_block();
    ::djinterp::test::block_spec optionator_descriptors_block();
    ::djinterp::test::block_spec optionator_internals_block();
    ::djinterp::test::block_spec optionator_pipeline_block();
    ::djinterp::test::block_spec optionator_traits_block();

NS_END  // testing
NS_END  // djinterp


namespace
{
    using block_provider = ::djinterp::test::block_spec (*)();

    // make_module
    //   fold a suite's block-providers into one module_spec (mirrors what each
    // suite's own runner does in its main()).
    ::djinterp::test::module_spec
    make_module(
        const char*                          _name,
        const char*                          _descriptor,
        std::initializer_list<block_provider> _blocks
    )
    {
        ::djinterp::test::module_spec mod;
        mod.name       = _name;
        mod.descriptor = _descriptor;
        for (block_provider _bp : _blocks)
        {
            mod.blocks.push_back(_bp());
        }
        return mod;
    }
}


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    std::vector<dt::module_spec> specs;

    specs.push_back(make_module(
        "option.hpp",
        "Core option<> type, is_option detection trait, and the "
        "Option/UnaryOption/ArgsOption concept analogs.",
        { tt::option_sentinels_block,
          tt::option_core_block,
          tt::option_is_option_block,
          tt::option_concepts_block }));

    specs.push_back(make_module(
        "option_set.hpp",
        "expansion, construction checks, pack surface, value face, queries, concepts",
        { tt::option_set_expand_block,
          tt::option_set_checks_block,
          tt::option_set_core_block,
          tt::option_set_values_block,
          tt::option_set_queries_block,
          tt::option_set_concepts_block }));

    specs.push_back(make_module(
        "option_set_compare.hpp",
        "key lists, key/type congruity, value carriers, parameterized value equality",
        { tt::option_set_compare_keylist_block,
          tt::option_set_compare_congruity_block,
          tt::option_set_compare_carriers_block,
          tt::option_set_compare_value_eq_block }));

    specs.push_back(make_module(
        "option_override.hpp",
        "option-aware override engine, merge_args_union, and ready-made policies",
        { tt::option_override_helpers_block,
          tt::option_override_merge_block,
          tt::option_override_lazy_block,
          tt::option_override_engine_block,
          tt::option_override_policies_block }));

    specs.push_back(make_module(
        "option_compose.hpp",
        "defopt surface, define-and-add, multi-surface folds, and compose-from-empty",
        { tt::option_compose_defopt_block,
          tt::option_compose_with_option_block,
          tt::option_compose_with_options_block,
          tt::option_compose_compose_block }));

    specs.push_back(make_module(
        "option_builder.hpp",
        "chunk wrapper, tuple-to-set lift, and the flat-schema pipeline",
        { tt::option_builder_wrap_block,
          tt::option_builder_lift_block,
          tt::option_builder_pipeline_block }));

    specs.push_back(make_module(
        "option_factory.hpp",
        "key_carrier bridge + compile-time / runtime option factories",
        { tt::option_factory_carrier_block,
          tt::option_factory_make_type_block,
          tt::option_factory_make_value_block }));

    specs.push_back(make_module(
        "option_generator.hpp",
        "flat NTTP stream -> populated option_set: specs, parse, emit, front-end",
        { tt::option_generator_specs_block,
          tt::option_generator_parse_block,
          tt::option_generator_emit_block,
          tt::option_generator_frontend_block }));

    specs.push_back(make_module(
        "option_diff.hpp",
        "compile-time + runtime key / value diff, diff summary, and merge",
        { tt::option_diff_ct_keys_block,
          tt::option_diff_ct_values_block,
          tt::option_diff_ct_merge_block,
          tt::option_diff_rt_keys_block,
          tt::option_diff_rt_values_block,
          tt::option_diff_rt_merge_block }));

    specs.push_back(make_module(
        "optionator.hpp",
        "option_set as a compile-time environment for an operator|-pluggable "
        "producer / transducer / consumer pipeline",
        { tt::optionator_configures_block,
          tt::optionator_descriptors_block,
          tt::optionator_internals_block,
          tt::optionator_pipeline_block,
          tt::optionator_traits_block }));

    // configure the report through the option set: one PDF per suite, all ten
    // bundled into a single .zip (stored fallback needs no external library).
    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::split>(dt::test_output_split::per_module);
    opts.set<dt::test_option::pack>(dt::test_output_pack::archive);
    opts.set<dt::test_option::archive_format>(dt::test_archive_format::zip);
    opts.set<dt::test_option::output_file>(std::string("option_tests.zip"));

    return dt::run_suite(
        "djinterp option layer",
        "Unit tests across option, option_set, option_set_compare, "
        "option_override, option_compose, option_builder, option_factory, "
        "option_generator, option_diff, and optionator.",
        specs,
        "djinterp option-layer unit tests",   // report title
        opts);                                // options: 10 PDFs -> option_tests.zip
}
