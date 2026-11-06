/*******************************************************************************
* djinterp [test]                                              t_env_printer.cpp
*
*   The environment printer's C++ harness (env/env_printer.hpp), at every level
* from C++98: the C++ unit's report and its two C++ sections, the `features`
* filter, DEnvPrinter configured every way, filters and field hooks from
* functors and -- from C++11 -- lambdas, every destination, the dump's round
* trip, a printer made in the djin and let go of when it goes. A standalone
* program, as the other tests not yet in DTest are: every section as JSON goes
* to the file its first argument names and as CSV to the one its second names
* (t_env_printer_cpp.json and .csv without them) for a parser to read. It ends
* with "N checks, M failed" and exits 0 only when nothing failed.
*
*   Built with D_CFG_ENV_PRINTER_CPP_FEATURES=1, as the ladder builds it, the
* feature table is checked whole; without it, its note is.
*
*
* path:      /tests/unintegrated/cpp/env/t_env_printer.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.11.04
*******************************************************************************/
// std
#include <cstdio>   // std::printf, std::fopen, std::fclose, std::tmpfile
#include <cstring>  // std::strcmp, std::strlen
#include <iomanip>  // std::setw
#include <sstream>  // std::ostringstream
#include <string>   // std::string
// djinterp
#include "../../../../inc/djinterp/env/env_printer.hpp"  // the C++ face


// D_TESTS_ENV_PRINTER_CHECK
//   macro: one check: counts it, and prints the expression and line on
// failure.
#define D_TESTS_ENV_PRINTER_CHECK(expr)                                        \
    check(((expr) ? true : false), #expr, __LINE__)

namespace
{

unsigned long g_checks   = 0;
unsigned long g_failures = 0;

// check
//   function: counts one check, and reports it when it failed.
void check(
    bool        _ok,
    const char* _expression,
    int         _line
)
{
    ++g_checks;

    if (!_ok)
    {
        ++g_failures;
        std::printf("    FAILED line %d: %s\n", _line, _expression);
    }

    return;
}

// lines
//   function: how many lines a text has.
std::size_t lines(
    const std::string& _text
)
{
    std::size_t count = 0;

    for (std::size_t i = 0; i < _text.size(); ++i)
    {
        count += (_text[i] == '\n') ? 1u : 0u;
    }

    return count;
}

// the feature section: its counts, from the report itself
const struct dj_env_section* features_section()
{
    const struct dj_env_report& report = djinterp::env_report();

    for (std::size_t i = 0; i < report.count; ++i)
    {
        if (report.sections[i].id == D_ENV_SECTION_CPP_FEATURES)
        {
            return &report.sections[i];
        }
    }

    return D_NULLPTR;
}

// HideUnsupported
//   functor: a filter that shows supported features and every entry.
struct HideUnsupported
{
    bool operator()(
        const struct dj_env_item& _item
    ) const
    {
        return ( (_item.feature == D_NULLPTR) ||
                 (_item.feature->value != 0L) );
    }
};

// Shout
//   functor: a field hook that gives every label in capitals' place
// "LABEL", and leaves the rest.
struct Shout
{
    int operator()(
        const struct dj_env_item& _item,
        uint32_t                  _field,
        struct dj_ostream_value&  _value
    ) const
    {
        if ( (_field == D_ENV_FIELD_LABEL) &&
             (_item.entry != D_NULLPTR) )
        {
            _value = d_ostream_value_text("LABEL");

            return 1;
        }

        return 0;
    }
};

// report
//   test: the C++ unit's report: ten sections, the C++ summary, and the
// feature table or its note.
void report()
{
    const struct dj_env_report&        env      = djinterp::env_report();
    const struct dj_env_section* const features = features_section();

    std::printf("  the report\n");

    D_TESTS_ENV_PRINTER_CHECK(env.count == 10u);
    D_TESTS_ENV_PRINTER_CHECK(std::strcmp(env.sections[8].slug,
                                          "cpp_summary") == 0);
    D_TESTS_ENV_PRINTER_CHECK(env.sections[8].entry_count == 17u);
    D_TESTS_ENV_PRINTER_CHECK(features != D_NULLPTR);

#if D_CFG_IS_ON(D_CFG_ENV_PRINTER_CPP_FEATURES)
    D_TESTS_ENV_PRINTER_CHECK( (features != D_NULLPTR) &&
                               (features->feature_count == 161u) &&
                               (features->note == D_NULLPTR) );
#else
    D_TESTS_ENV_PRINTER_CHECK( (features != D_NULLPTR) &&
                               (features->feature_count == 0u) &&
                               (features->note != D_NULLPTR) );
#endif  // D_CFG_ENV_PRINTER_CPP_FEATURES

    return;
}

// features
//   test: the `features` variable, and the feature line's form.
void features()
{
    const struct dj_env_section* const section   = features_section();
    std::size_t                        supported = 0;

    std::printf("  features\n");

    if ( (section == D_NULLPTR) ||
         (section->feature_count == 0u) )
    {
        std::printf("    (no feature table compiled in)\n");

        return;
    }

    for (std::size_t i = 0; i < section->feature_count; ++i)
    {
        supported += (section->features[i].value != 0L) ? 1u : 0u;
    }

    djinterp::DEnvPrinter printer("sections=cpp_features;"
                                  "report_begin=\"\"; section_begin=\"\";"
                                  "feature=\"{path}\\n\"");
    std::string           all;
    std::string           yes;
    std::string           no;

    D_TESTS_ENV_PRINTER_CHECK(printer.status() == D_OPTION_STATUS_OK);
    D_TESTS_ENV_PRINTER_CHECK(djinterp::print_env(printer, all));
    D_TESTS_ENV_PRINTER_CHECK(printer.put("features=supported"));
    D_TESTS_ENV_PRINTER_CHECK(djinterp::print_env(printer, yes));
    D_TESTS_ENV_PRINTER_CHECK(printer.put("features=unsupported"));
    D_TESTS_ENV_PRINTER_CHECK(djinterp::print_env(printer, no));
    D_TESTS_ENV_PRINTER_CHECK(lines(all) == section->feature_count);
    D_TESTS_ENV_PRINTER_CHECK(lines(yes) == supported);
    D_TESTS_ENV_PRINTER_CHECK(lines(no) == section->feature_count - supported);

    // the text style's feature line: sign, standard, name, value -- the
    // value this unit's compiler gives, 0 below C++11
    long lambdas = 0L;

    for (std::size_t i = 0; i < section->feature_count; ++i)
    {
        if (std::strcmp(section->features[i].name, "__cpp_lambdas") == 0)
        {
            lambdas = section->features[i].value;
        }
    }

    const char* const     sign = (lambdas != 0L) ? "+" : "-";
    djinterp::DEnvPrinter text("sections=cpp_features;"
                               "select=cpp_features.__cpp_lambdas");
    std::string           line;
    std::ostringstream    expected;

    expected << "\n  [" << sign << "] C++11  __cpp_lambdas  "
             << std::setw(6) << lambdas << "  Lambda expressions\n";
    D_TESTS_ENV_PRINTER_CHECK(djinterp::print_env(text, line));
    D_TESTS_ENV_PRINTER_CHECK(line.find(expected.str()) != std::string::npos);
    D_TESTS_ENV_PRINTER_CHECK(text.load("show_standards=0;"
                                        "show_descriptions=0; name_width=20"));
    line.clear();
    expected.str("");
    expected << "\n  [" << sign << "] __cpp_lambdas         "
             << std::setw(6) << lambdas << "\n";
    D_TESTS_ENV_PRINTER_CHECK(djinterp::print_env(text, line));
    D_TESTS_ENV_PRINTER_CHECK(line.find(expected.str()) != std::string::npos);

    // markdown's features table, and JSON's feature object
    D_TESTS_ENV_PRINTER_CHECK(text.load("style=markdown; -name_width"));
    line.clear();
    expected.str("");
    expected << "| `__cpp_lambdas` | " << lambdas << " | "
             << ((lambdas != 0L) ? "yes" : "no") << " |\n";
    D_TESTS_ENV_PRINTER_CHECK(djinterp::print_env(text, line));
    D_TESTS_ENV_PRINTER_CHECK(line.find("| Feature | Value | Supported |\n")
                              != std::string::npos);
    D_TESTS_ENV_PRINTER_CHECK(line.find(expected.str()) != std::string::npos);
    D_TESTS_ENV_PRINTER_CHECK(text.load("style=json; show_standards=1"));
    line.clear();
    expected.str("");
    expected << "\"__cpp_lambdas\": { \"value\": " << lambdas
             << ", \"supported\": " << ((lambdas != 0L) ? "true" : "false")
             << ", \"standard\": \"C++11\" }";
    D_TESTS_ENV_PRINTER_CHECK(djinterp::print_env(text, line));
    D_TESTS_ENV_PRINTER_CHECK(line.find(expected.str()) != std::string::npos);

    return;
}

// destinations
//   test: every destination prints the same report; the array measures and
// truncates as snprintf does.
void destinations()
{
    djinterp::DEnvPrinter printer("sections=core,cpp_summary");
    std::string           text;
    std::ostringstream    stream;
    char                  small[40];
    std::size_t           needed = 0;

    std::printf("  destinations\n");

    D_TESTS_ENV_PRINTER_CHECK(djinterp::print_env(printer, text));
    D_TESTS_ENV_PRINTER_CHECK(djinterp::print_env(printer, stream));
    D_TESTS_ENV_PRINTER_CHECK(stream.str() == text);

    std::string                       viastream;
    djinterp::DBufferedOutputStream<> out;

    D_TESTS_ENV_PRINTER_CHECK(djinterp::attach_std_string(out, viastream));
    D_TESTS_ENV_PRINTER_CHECK(djinterp::print_env(printer, out));
    D_TESTS_ENV_PRINTER_CHECK(out.close());
    D_TESTS_ENV_PRINTER_CHECK(viastream == text);

    // a FILE*, read back
    std::FILE* const file = std::tmpfile();

    D_TESTS_ENV_PRINTER_CHECK(file != D_NULLPTR);

    if (file != D_NULLPTR)
    {
        std::string back;
        char        chunk[256];
        std::size_t got = 0;

        D_TESTS_ENV_PRINTER_CHECK(djinterp::print_env(printer, file));
        std::rewind(file);

        while ((got = std::fread(chunk, 1, sizeof(chunk), file)) > 0)
        {
            back.append(chunk, got);
        }

        D_TESTS_ENV_PRINTER_CHECK(back == text);
        (void)std::fclose(file);
    }

    // an array: cut, and measured
    D_TESTS_ENV_PRINTER_CHECK(!printer.print(small, sizeof(small),
                                             djinterp::env_report(), &needed));
    D_TESTS_ENV_PRINTER_CHECK(needed == text.size());
    D_TESTS_ENV_PRINTER_CHECK(std::string(small) == text.substr(0, 39));

    // one call, with and without a script
    std::string once;

    D_TESTS_ENV_PRINTER_CHECK(djinterp::print_env(once,
                                                  "sections=core,cpp_summary"));
    D_TESTS_ENV_PRINTER_CHECK(once == text);
    D_TESTS_ENV_PRINTER_CHECK(!djinterp::print_env(once, "style=yaml"));

    std::string defaults;

    D_TESTS_ENV_PRINTER_CHECK(djinterp::print_env(defaults));
    D_TESTS_ENV_PRINTER_CHECK(defaults.find("djinterp environment\n\n"
                                            "Language\n") == 0);

    return;
}

// hooks
//   test: a functor filter, a functor field hook, and -- from C++11 -- the
// same from lambdas; the printer's errors and status.
void hooks()
{
    djinterp::DEnvPrinter printer("sections=compiler,cpp_features;"
                                  "report_begin=\"\"; section_begin=\"\";"
                                  "entry=\"{label}\\n\"; feature=\"+\\n\"");
    std::string           text;

    std::printf("  hooks\n");

    djinterp::DEnvFilterOf<HideUnsupported> filter =
        djinterp::make_env_filter(HideUnsupported());
    djinterp::DEnvFieldOf<Shout>            field  =
        djinterp::make_env_field(Shout());

    printer.set_filter(filter);
    printer.set_field(field);
    D_TESTS_ENV_PRINTER_CHECK(djinterp::print_env(printer, text));
    D_TESTS_ENV_PRINTER_CHECK(text.find("LABEL\n") == 0);
    D_TESTS_ENV_PRINTER_CHECK(text.find("Compiler") == std::string::npos);

    const struct dj_env_section* const section = features_section();
    std::size_t                        supported = 0;

    for (std::size_t i = 0;
         (section != D_NULLPTR) && (i < section->feature_count);
         ++i)
    {
        supported += (section->features[i].value != 0L) ? 1u : 0u;
    }

    std::size_t pluses = 0;

    for (std::size_t i = 0; i < text.size(); ++i)
    {
        pluses += (text[i] == '+') ? 1u : 0u;
    }

    D_TESTS_ENV_PRINTER_CHECK(pluses == supported);
    printer.clear_hooks();

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    int  asked  = 0;
    auto counts = djinterp::make_env_filter(
                      [&asked](const struct dj_env_item& _item) -> bool
                      {
                          ++asked;

                          return (_item.entry != nullptr);
                      });

    printer.set_filter(counts);
    text.clear();
    D_TESTS_ENV_PRINTER_CHECK(djinterp::print_env(printer, text));
    D_TESTS_ENV_PRINTER_CHECK(asked > 0);
    D_TESTS_ENV_PRINTER_CHECK(text.find('+') == std::string::npos);
    printer.clear_hooks();
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    // errors: a statement refused, a template refused and reported
    D_TESTS_ENV_PRINTER_CHECK(!printer.put("no_such=1"));
    D_TESTS_ENV_PRINTER_CHECK(printer.status() == D_OPTION_STATUS_KEY_ABSENT);
    D_TESTS_ENV_PRINTER_CHECK(!printer.set_template(D_ENV_TEMPLATE_ENTRY,
                                                    "{label!yaml}"));
    D_TESTS_ENV_PRINTER_CHECK(printer.error().status ==
                              D_OSTREAM_TEMPLATE_BAD_CONVERSION);
    D_TESTS_ENV_PRINTER_CHECK(printer.set_template(D_ENV_TEMPLATE_ENTRY,
                                                   D_NULLPTR));
    D_TESTS_ENV_PRINTER_CHECK(printer.status() == D_OPTION_STATUS_OK);

    return;
}

// dump
//   test: a dump loaded into a new printer prints the same; reset.
void dump()
{
    djinterp::DEnvPrinter printer("style=markdown; sections=all;"
                                  "select=-cpp_features.*,"
                                  "+cpp_features.__cpp_lib_*;"
                                  "cpp_summary.true_text=\"\\xE2\\x9C\\x93\"");
    std::string           first;
    std::string           second;

    std::printf("  dumps\n");

    D_TESTS_ENV_PRINTER_CHECK(djinterp::print_env(printer, first));

    const std::string     script = printer.dump();
    djinterp::DEnvPrinter again(script.c_str());

    D_TESTS_ENV_PRINTER_CHECK(again.status() == D_OPTION_STATUS_OK);
    D_TESTS_ENV_PRINTER_CHECK(djinterp::print_env(again, second));
    D_TESTS_ENV_PRINTER_CHECK(first == second);
    D_TESTS_ENV_PRINTER_CHECK(first.find("| C++26 | \xE2\x9C\x93 |")
                              != std::string::npos ||
                              first.find("| C++26 | no |")
                              != std::string::npos);

    again.reset();
    second.clear();
    D_TESTS_ENV_PRINTER_CHECK(djinterp::print_env(again, second));
    D_TESTS_ENV_PRINTER_CHECK(second.find("djinterp environment\n\n") == 0);
    D_TESTS_ENV_PRINTER_CHECK(again.dump().empty());

    return;
}

// djin
//   test: a printer made in the djin reads its bindings, binds its own over
// them from the same source, and lets go of it when it goes, so the djin can
// be registered after.
void djin()
{
    struct dj_option_env* const   djin = d_env_printer_djin();
    struct dj_option_env_registry registry;
    std::string                   text;

    std::printf("  the djin\n");

    D_TESTS_ENV_PRINTER_CHECK(d_option_env_put_from(djin,
                                                    "show_title=0",
                                                    D_OPTION_ENV_SOURCE_FILE)
                              .is_ok);

    {
        djinterp::DEnvPrinter printer(*djin, "sections=build");

        D_TESTS_ENV_PRINTER_CHECK(printer.status() ==
                                  static_cast<int32_t>(D_OPTION_STATUS_OK));
        D_TESTS_ENV_PRINTER_CHECK(djin->child_count == 1u);
        D_TESTS_ENV_PRINTER_CHECK(djinterp::print_env(printer, text));
        D_TESTS_ENV_PRINTER_CHECK(text.find("Build\n") == 0);
        D_TESTS_ENV_PRINTER_CHECK(
            d_option_env_put_from(printer.env(),
                                  "show_title=1",
                                  D_OPTION_ENV_SOURCE_FILE).is_ok);
        text.clear();
        D_TESTS_ENV_PRINTER_CHECK(djinterp::print_env(printer, text));
        D_TESTS_ENV_PRINTER_CHECK(text.find("djinterp environment\n") == 0);
    }

    D_TESTS_ENV_PRINTER_CHECK(djin->child_count == 0u);
    D_TESTS_ENV_PRINTER_CHECK(d_option_env_registry_init(&registry,
                                                         "cpptest").is_ok);
    D_TESTS_ENV_PRINTER_CHECK(d_env_printer_register(&registry).is_ok);
    D_TESTS_ENV_PRINTER_CHECK(djin->set.count == 0u);
    D_TESTS_ENV_PRINTER_CHECK(d_option_env_registry_remove(&registry,
                                                           djin).is_ok);

    return;
}

// files
//   test: every section as JSON and CSV, for the parsers.
void files(
    const char* _json,
    const char* _csv
)
{
    std::FILE* const json = std::fopen(_json, "wb");
    std::FILE* const csv  = std::fopen(_csv, "wb");

    std::printf("  files\n");

    D_TESTS_ENV_PRINTER_CHECK( (json != D_NULLPTR) &&
                               (csv != D_NULLPTR) );

    if ( (json == D_NULLPTR) ||
         (csv == D_NULLPTR) )
    {
        return;
    }

    D_TESTS_ENV_PRINTER_CHECK(djinterp::print_env(json,
                                                  "style=json; sections=all"));
    D_TESTS_ENV_PRINTER_CHECK(djinterp::print_env(csv,
                                                  "style=csv; sections=all"));
    D_TESTS_ENV_PRINTER_CHECK(std::fclose(json) == 0);
    D_TESTS_ENV_PRINTER_CHECK(std::fclose(csv) == 0);

    return;
}

}  // namespace


// main
//   function: every group, then the count.
int main(
    int    _argc,
    char** _argv
)
{
    std::printf("t_env_printer (C++%ld)\n", static_cast<long>(__cplusplus));

    report();
    features();
    destinations();
    hooks();
    dump();
    djin();
    files((_argc > 1) ? _argv[1] : "t_env_printer_cpp.json",
          (_argc > 2) ? _argv[2] : "t_env_printer_cpp.csv");

    std::printf("%lu checks, %lu failed\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
