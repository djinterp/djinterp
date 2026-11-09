/*******************************************************************************
* djinterp [test]                                                t_env_printer.c
*
*   The environment printer's C harness (env/env_printer.h): the report a C unit
* collects, and the printer as a machine -- its defaults, every style,
* `sections`, `select`'s rules, scoped variables, templates set and refused, a
* program's own variables, the filter and the field hook, the dump and its round
* trip, the djin and its registry -- settings by full name from a file, the
* command line and the environment, by standing -- a printer made in it and one
* whose frame moved, the process's environment. A standalone program, as the
* other tests not yet in DTest are: it writes every section as JSON to the file
* its first argument names and as CSV to the one its second names (t_env_printer
* .json and .csv without them) for a parser to read, prints a line per group and
* one per failed check, ends with "N checks, M failed", and exits 0 only when
* nothing failed.
*
*   A TEMPLATE IS THE PROBE. Setting every part's template to "" but the
* entry's, which prints "{path}\n", turns a print into the list of paths that
* show, so what `select` and `sections` decide is compared as text.
*
*
* path:      /tests/unintegrated/c/env/t_env_printer.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.11.07
*******************************************************************************/
// std
#include <stdbool.h>  // bool
#include <stdio.h>    // printf, fopen, fclose, FILE
#include <stdlib.h>   // setenv
#include <string.h>   // strcmp, strstr, strncmp, strlen
// djinterp
#include "../../../../inc/djinterp/env/env_printer.h"         // the printer
#include "../../../../inc/djinterp/c/util/ostream/ostream.h"  // dj_ostream


// D_TESTS_ENV_PRINTER_CHECK
//   macro: one check: counts it, and prints the expression and line on
// failure.
#define D_TESTS_ENV_PRINTER_CHECK(expr)                                        \
    d_tests_env_printer_check(((expr) ? 1 : 0), #expr, __LINE__)

// D_TESTS_ENV_PRINTER_TEXT
//   constant: the bytes a report is printed into.
#define D_TESTS_ENV_PRINTER_TEXT  32768u

static unsigned long d_tests_env_printer_checks;
static unsigned long d_tests_env_printer_failures;
static char          d_tests_env_printer_text[D_TESTS_ENV_PRINTER_TEXT];

// a report of fixed values, for checks of exact text: the C unit's own
// differs with the compiler, the target and the build
static const struct dj_env_entry D_TESTS_COMPILER[] =
{
    D_INTERNAL_ENV_PRINTER_TEXT("name", "Compiler", "GCC"),
    D_INTERNAL_ENV_PRINTER_TEXT("full_name",
                                "Full Name",
                                "GNU Compiler Collection")
};

static const struct dj_env_entry D_TESTS_OS[] =
{
    D_INTERNAL_ENV_PRINTER_TEXT("name", "OS", "Linux"),
    D_INTERNAL_ENV_PRINTER_TEXT("platform", "Platform", "Linux"),
    D_INTERNAL_ENV_PRINTER_BOOL("posix_like", "POSIX-like", 1),
    D_INTERNAL_ENV_PRINTER_BOOL("windows", "Windows", 0)
};

static const struct dj_env_entry D_TESTS_ARCH[] =
{
    D_INTERNAL_ENV_PRINTER_TEXT("name", "Architecture", "x86-64"),
    D_INTERNAL_ENV_PRINTER_INT("bits", "Bit Width", 64)
};

static const struct dj_env_entry D_TESTS_BUILD[] =
{
    D_INTERNAL_ENV_PRINTER_TEXT("type", "Build Type", "Debug"),
    D_INTERNAL_ENV_PRINTER_BOOL("debug", "Debug", 1)
};

static const struct dj_env_section D_TESTS_SECTIONS[] =
{
    { "compiler", "Compiler", NULL,
      D_TESTS_COMPILER, 2u, NULL, 0u, D_ENV_SECTION_COMPILER, 0 },
    { "os", "Operating System", NULL,
      D_TESTS_OS, 4u, NULL, 0u, D_ENV_SECTION_OS, 0 },
    { "architecture", "Architecture", NULL,
      D_TESTS_ARCH, 2u, NULL, 0u, D_ENV_SECTION_ARCH, 0 },
    { "build", "Build", NULL,
      D_TESTS_BUILD, 2u, NULL, 0u, D_ENV_SECTION_BUILD, 0 },
    { "cpp_features", "C++ Features", "not compiled in",
      NULL, 0u, NULL, 0u, D_ENV_SECTION_CPP_FEATURES, 0 }
};

static const struct dj_env_report D_TESTS_REPORT =
{
    D_TESTS_SECTIONS,
    5u
};

// the report d_tests_env_printer_run prints: the fixed one, or the unit's
static const struct dj_env_report* d_tests_env_printer_source;

/*
d_tests_env_printer_check
  Counts one check, and reports it when it failed.
*/
static bool
d_tests_env_printer_check(
    int         _ok,
    const char* _expression,
    int         _line
)
{
    ++d_tests_env_printer_checks;

    if (_ok == 0)
    {
        ++d_tests_env_printer_failures;
        printf("    FAILED line %d: %s\n", _line, _expression);
    }

    return (_ok != 0);
}

/*
d_tests_env_printer_run
  Prints the C unit's report with a printer into the harness's buffer,
NUL-terminated, and returns whether the print succeeded.
*/
static int
d_tests_env_printer_run(
    struct dj_env_printer* _printer
)
{
    struct dj_ostream       os;
    struct dj_ostream_fixed fixed;

    (void)d_ostream_init_fixed(&os,
                               &fixed,
                               d_tests_env_printer_text,
                               sizeof(d_tests_env_printer_text),
                               1);

    const int32_t printed = d_env_printer_print(
                                _printer,
                                &os,
                                (d_tests_env_printer_source != NULL)
                                    ? d_tests_env_printer_source
                                    : d_env_report_here());

    return ( (d_ostream_close(&os) != 0) &&
             (printed != 0) ) ? 1 : 0;
}

/*
d_tests_env_printer_paths
  The paths a configuration shows: every template empty but the entry's and
the note's, which print "{path}\n".
*/
static const char*
d_tests_env_printer_paths(
    const char* _script
)
{
    struct dj_env_printer printer;

    (void)d_env_printer_init(&printer);
    (void)d_env_printer_load(&printer,
                             "report_begin=\"\"; section_begin=\"\";"
                             "section_separator=\"\";"
                             "entry=\"{path}\\n\"; note=\"{path}\\n\"",
                             NULL);

    if ( (!d_env_printer_load(&printer, _script, NULL).is_ok) ||
         (d_tests_env_printer_run(&printer) == 0) )
    {
        return "(failed)";
    }

    return d_tests_env_printer_text;
}

/*
d_tests_env_printer_shows
  Whether a configuration shows exactly the paths expected.
*/
static int
d_tests_env_printer_shows(
    const char* _script,
    const char* _expected
)
{
    const char* const got = d_tests_env_printer_paths(_script);

    if (strcmp(got, _expected) != 0)
    {
        printf("    [%s]:\n%s---- expected ----\n%s----\n",
               _script, got, _expected);

        return 0;
    }

    return 1;
}

/*
d_tests_env_printer_report
  Tests the following:
  - a C unit's report: the eight sections a C unit has, in order
  - every name unique in its section and fit for a path; every label set
*/
static void
d_tests_env_printer_report(void)
{
    static const char* const SLUGS[] =
    {
        "language", "compiler", "os", "architecture", "posix",
        "preprocessor_limits", "build", "c_runtime"
    };
    const struct dj_env_report* const report = d_env_report_here();
    int                               names  = 1;

    printf("  the report\n");

    D_TESTS_ENV_PRINTER_CHECK(report == d_env_report_here());
    D_TESTS_ENV_PRINTER_CHECK(report->count == 8u);
    D_TESTS_ENV_PRINTER_CHECK(report->sections[7].entry_count == 53u);

    for (size_t s = 0u; s < report->count; ++s)
    {
        const struct dj_env_section* const section = &report->sections[s];

        D_TESTS_ENV_PRINTER_CHECK(strcmp(section->slug, SLUGS[s]) == 0);

        for (size_t i = 0u; i < section->entry_count; ++i)
        {
            const char* const name = section->entries[i].name;

            names &= ( (name != NULL)                              &&
                       (name[0] != '\0')                           &&
                       (strpbrk(name, ". ,*?") == NULL)            &&
                       (section->entries[i].label != NULL)         &&
                       (section->entries[i].label[0] != '\0') );

            for (size_t j = 0u; j < i; ++j)
            {
                names &= (strcmp(section->entries[j].name, name) != 0);
            }
        }
    }

    D_TESTS_ENV_PRINTER_CHECK(names != 0);

    return;
}

/*
d_tests_env_printer_defaults
  Tests the following:
  - the default text: title, sections, the label column fitting the
    section's longest label, booleans as yes and no
  - the measure pass agrees with the text; a short array is cut
  - d_env_print and d_env_print_file; a script that fails prints nothing
*/
static void
d_tests_env_printer_defaults(void)
{
    struct dj_env_printer   printer;
    struct dj_ostream       os;
    struct dj_ostream_fixed fixed;
    char                    small[32];

    printf("  the defaults\n");

    D_TESTS_ENV_PRINTER_CHECK(d_env_printer_init(&printer) == 1);
    D_TESTS_ENV_PRINTER_CHECK(d_tests_env_printer_run(&printer) == 1);
    D_TESTS_ENV_PRINTER_CHECK(strncmp(d_tests_env_printer_text,
                                      "djinterp environment\n\nLanguage\n",
                                      31u) == 0);
    D_TESTS_ENV_PRINTER_CHECK(strstr(d_tests_env_printer_text,
                                     "\n  Using C           yes\n") != NULL);
    D_TESTS_ENV_PRINTER_CHECK(strstr(d_tests_env_printer_text,
                                     "\n  __STDC_VERSION__  ") != NULL);
    D_TESTS_ENV_PRINTER_CHECK(strstr(d_tests_env_printer_text,
                                     "C Runtime") == NULL);
    D_TESTS_ENV_PRINTER_CHECK(strstr(d_tests_env_printer_text,
                                     "\nBuild\n  Build Type  ") != NULL);

    // measured, and cut
    const size_t length = strlen(d_tests_env_printer_text);

    (void)d_ostream_init_fixed(&os, &fixed, NULL, 0u, 0);
    D_TESTS_ENV_PRINTER_CHECK(d_env_print(&os, NULL) == 1);
    (void)d_ostream_close(&os);
    D_TESTS_ENV_PRINTER_CHECK(d_ostream_written(&os) == length);
    (void)d_ostream_init_fixed(&os, &fixed, small, sizeof(small), 1);
    (void)d_env_print(&os, NULL);
    (void)d_ostream_close(&os);
    D_TESTS_ENV_PRINTER_CHECK( (strncmp(small, d_tests_env_printer_text, 31u)
                                == 0) &&
                               (d_ostream_fixed_truncated(&fixed) != 0) );

    // a script that fails
    (void)d_ostream_init_fixed(&os, &fixed, NULL, 0u, 0);
    D_TESTS_ENV_PRINTER_CHECK(d_env_print(&os, "style=yaml") == 0);
    D_TESTS_ENV_PRINTER_CHECK( (d_ostream_status(&os) == D_OSTREAM_INVALID) &&
                               (d_ostream_written(&os) == 0u) );

    // null arguments
    (void)d_ostream_init_fixed(&os, &fixed, NULL, 0u, 0);
    D_TESTS_ENV_PRINTER_CHECK(d_env_printer_print(&printer, &os, NULL) == 0);
    D_TESTS_ENV_PRINTER_CHECK(d_ostream_status(&os) == D_OSTREAM_INVALID);
    D_TESTS_ENV_PRINTER_CHECK(d_env_printer_print(&printer, NULL,
                                                  d_env_report_here()) == 0);
    D_TESTS_ENV_PRINTER_CHECK(d_env_printer_init(NULL) == 0);
    D_TESTS_ENV_PRINTER_CHECK(!d_env_printer_put(NULL, "a=1").is_ok);

    return;
}

/*
d_tests_env_printer_selection
  Tests the following:
  - `sections`, by names and sets
  - `select`: a list starting with an inclusion starts from nothing, one
    starting with an exclusion from everything; the last match decides; a
    section's glob alone takes its items; '*' and '?'
  - a section with nothing to show is left out whole
*/
static void
d_tests_env_printer_selection(void)
{
    printf("  sections and select\n");

    D_TESTS_ENV_PRINTER_CHECK(d_tests_env_printer_shows(
        "sections=build",
        "build.type\nbuild.debug\nbuild.ndebug\nbuild.detected\n"));
    D_TESTS_ENV_PRINTER_CHECK(d_tests_env_printer_shows(
        "sections=core; select=build",
        "build.type\nbuild.debug\nbuild.ndebug\nbuild.detected\n"));
    D_TESTS_ENV_PRINTER_CHECK(d_tests_env_printer_shows(
        "sections=build,architecture; select=-build.type",
        "architecture.name\narchitecture.bits\narchitecture.pointer_bits\n"
        "architecture.endianness\narchitecture.detected\n"
        "build.debug\nbuild.ndebug\nbuild.detected\n"));
    D_TESTS_ENV_PRINTER_CHECK(d_tests_env_printer_shows(
        "sections=all; select=-*, +*.name, -os.*, +os.hosted",
        "compiler.name\nos.hosted\narchitecture.name\n"));
    D_TESTS_ENV_PRINTER_CHECK(d_tests_env_printer_shows(
        "sections=all; select=c_runtime.s?e*",
        "c_runtime.sse\nc_runtime.sse2\n"));
    D_TESTS_ENV_PRINTER_CHECK(d_tests_env_printer_shows(
        "sections=core; select=compiler.m*,-compiler.minor",
        "compiler.major\n"));
    D_TESTS_ENV_PRINTER_CHECK(d_tests_env_printer_shows(
        "sections=core; select=nothing.here",
        ""));
    D_TESTS_ENV_PRINTER_CHECK(d_tests_env_printer_shows(
        "sections=none",
        ""));
    D_TESTS_ENV_PRINTER_CHECK(d_tests_env_printer_shows(
        "sections=cpp",
        ""));

    return;
}

/*
d_tests_env_printer_styles
  Tests the following:
  - json's shape; show_title, prefix, true_text and false_text
  - a title with no section: no blank line after it; JSON stays JSON
  - key_width set, and scoped to one section
*/
static void
d_tests_env_printer_styles(void)
{
    struct dj_env_printer printer;

    printf("  styles and variables\n");

    d_tests_env_printer_source = &D_TESTS_REPORT;

    (void)d_env_printer_init(&printer);
    (void)d_env_printer_load(&printer, "style=json; sections=build", NULL);
    D_TESTS_ENV_PRINTER_CHECK(d_tests_env_printer_run(&printer) == 1);
    D_TESTS_ENV_PRINTER_CHECK(strcmp(d_tests_env_printer_text,
        "{\n"
        "  \"title\": \"djinterp environment\",\n"
        "  \"build\": {\n"
        "    \"type\": \"Debug\",\n"
        "    \"debug\": true\n"
        "  }\n"
        "}\n") == 0);

    (void)d_env_printer_put(&printer, "show_title=0");
    (void)d_env_printer_put(&printer, "sections=none");
    D_TESTS_ENV_PRINTER_CHECK( (d_tests_env_printer_run(&printer) == 1) &&
                               (strcmp(d_tests_env_printer_text, "{\n}\n")
                                == 0) );
    (void)d_env_printer_put(&printer, "show_title=1");
    D_TESTS_ENV_PRINTER_CHECK( (d_tests_env_printer_run(&printer) == 1) &&
                               (strcmp(d_tests_env_printer_text,
                                       "{\n  \"title\": "
                                       "\"djinterp environment\"\n}\n")
                                == 0) );

    // text: a title alone; then a prefix on every line, and the words
    (void)d_env_printer_load(&printer, "style=text", NULL);
    D_TESTS_ENV_PRINTER_CHECK( (d_tests_env_printer_run(&printer) == 1) &&
                               (strcmp(d_tests_env_printer_text,
                                       "djinterp environment\n") == 0) );
    (void)d_env_printer_load(&printer,
                             "sections=build; prefix=\"// \";"
                             "true_text=on; false_text=off; title=Env",
                             NULL);
    D_TESTS_ENV_PRINTER_CHECK( (d_tests_env_printer_run(&printer) == 1) &&
                               (strcmp(d_tests_env_printer_text,
                                       "// Env\n// \n// Build\n"
                                       "//   Build Type  Debug\n"
                                       "//   Debug       on\n") == 0) );

    // widths: set, and local to one section
    (void)d_env_printer_load(&printer,
                             "-prefix; title=T; show_title=0;"
                             "sections=build,architecture; key_width=14;"
                             "build.key_width=11",
                             NULL);
    D_TESTS_ENV_PRINTER_CHECK( (d_tests_env_printer_run(&printer) == 1) &&
                               (strstr(d_tests_env_printer_text,
                                       "  Bit Width       64\n") != NULL) &&
                               (strstr(d_tests_env_printer_text,
                                       "  Build Type   Debug\n") != NULL) );

    d_tests_env_printer_source = NULL;

    return;
}

/*
d_tests_env_printer_presets
  Tests the following:
  - a section with a note and nothing else: the note is its item, in text
    and in JSON, and select can hide it
  - markdown's, csv's and keyvalue's shapes
*/
static void
d_tests_env_printer_presets(void)
{
    struct dj_env_printer printer;

    d_tests_env_printer_source = &D_TESTS_REPORT;

    // a section with a note and nothing else: the note is its item
    (void)d_env_printer_init(&printer);
    (void)d_env_printer_load(&printer, "sections=cpp_features; show_title=0",
                             NULL);
    D_TESTS_ENV_PRINTER_CHECK( (d_tests_env_printer_run(&printer) == 1) &&
                               (strcmp(d_tests_env_printer_text,
                                       "C++ Features\n"
                                       "  (not compiled in)\n") == 0) );
    (void)d_env_printer_put(&printer, "style=json");
    D_TESTS_ENV_PRINTER_CHECK( (d_tests_env_printer_run(&printer) == 1) &&
                               (strcmp(d_tests_env_printer_text,
                                       "{\n  \"cpp_features\": {\n"
                                       "    \"note\": \"not compiled in\"\n"
                                       "  }\n}\n") == 0) );
    (void)d_env_printer_put(&printer, "select=-cpp_features.note");
    D_TESTS_ENV_PRINTER_CHECK( (d_tests_env_printer_run(&printer) == 1) &&
                               (strcmp(d_tests_env_printer_text, "{\n}\n")
                                == 0) );

    // markdown, csv, keyvalue
    d_env_printer_reset(&printer);
    (void)d_env_printer_load(&printer, "style=markdown; sections=build",
                             NULL);
    D_TESTS_ENV_PRINTER_CHECK( (d_tests_env_printer_run(&printer) == 1) &&
                               (strcmp(d_tests_env_printer_text,
                                       "# djinterp environment\n\n"
                                       "## Build\n\n"
                                       "| Key | Value |\n"
                                       "| --- | --- |\n"
                                       "| Build Type | Debug |\n"
                                       "| Debug | yes |\n") == 0) );
    (void)d_env_printer_load(&printer, "style=csv", NULL);
    D_TESTS_ENV_PRINTER_CHECK( (d_tests_env_printer_run(&printer) == 1) &&
                               (strcmp(d_tests_env_printer_text,
                                       "section,name,label,kind,value,"
                                       "supported,standard,description\n"
                                       "build,type,Build Type,text,Debug,,,\n"
                                       "build,debug,Debug,boolean,true,,,\n")
                                == 0) );
    (void)d_env_printer_load(&printer, "style=keyvalue; show_title=0", NULL);
    D_TESTS_ENV_PRINTER_CHECK( (d_tests_env_printer_run(&printer) == 1) &&
                               (strcmp(d_tests_env_printer_text,
                                       "build.type=Debug\n"
                                       "build.debug=true\n") == 0) );
    d_tests_env_printer_source = NULL;

    return;
}

/*
d_tests_env_printer_templates
  Tests the following:
  - a template set by text; one set by d_env_printer_set_template, and one
    that does not compile refused there with the one before it kept
  - a template that does not compile set by text: reported where, and
    printing refused until it is mended
  - a program's own variable in a template; a field's name refused for one
  - the presets given out; unknown ones null
*/
static void
d_tests_env_printer_templates(void)
{
    static const struct dj_option_env_var MARKER[] =
    {
        D_OPTION_ENV_TEXT("marker", "=>", "a program's own"),
        D_OPTION_ENV_TEXT("value", NULL, "a field's name")
    };
    struct dj_env_printer printer;

    printf("  templates\n");

    d_tests_env_printer_source = &D_TESTS_REPORT;

    (void)d_env_printer_init(&printer);
    (void)d_env_printer_load(&printer,
                             "sections=build; show_title=0;"
                             "entry=\"{name}={value!json};\"",
                             NULL);
    D_TESTS_ENV_PRINTER_CHECK( (d_tests_env_printer_run(&printer) == 1) &&
                               (strcmp(d_tests_env_printer_text,
                                       "Build\ntype=\"Debug\";debug=true;")
                                == 0) );

    // through set_template: kept, and refused
    D_TESTS_ENV_PRINTER_CHECK(d_env_printer_set_template(
        &printer, D_ENV_TEMPLATE_SECTION_BEGIN, "[{section}:{count}]").is_ok);
    D_TESTS_ENV_PRINTER_CHECK(!d_env_printer_set_template(
        &printer, D_ENV_TEMPLATE_SECTION_BEGIN, "[{sektion}]").is_ok);
    D_TESTS_ENV_PRINTER_CHECK(
        (d_env_printer_last_error(&printer)->part ==
         D_ENV_TEMPLATE_SECTION_BEGIN)                                &&
        (d_env_printer_last_error(&printer)->status ==
         D_OSTREAM_TEMPLATE_UNKNOWN_FIELD)                            &&
        (d_env_printer_last_error(&printer)->offset == 2u) );
    D_TESTS_ENV_PRINTER_CHECK( (d_tests_env_printer_run(&printer) == 1) &&
                               (strcmp(d_tests_env_printer_text,
                                       "[build:2]type=\"Debug\";debug=true;")
                                == 0) );
    D_TESTS_ENV_PRINTER_CHECK(!d_env_printer_set_template(&printer, 9, "")
                              .is_ok);

    // through text: made, reported, and printing refused until mended
    D_TESTS_ENV_PRINTER_CHECK(!d_env_printer_put(&printer,
                                                 "entry=\"{name:<{wide}}\"")
                              .is_ok);
    D_TESTS_ENV_PRINTER_CHECK(
        (d_env_printer_last_error(&printer)->part == D_ENV_TEMPLATE_ENTRY) &&
        (d_env_printer_last_error(&printer)->status ==
         D_OSTREAM_TEMPLATE_UNKNOWN_FIELD) );
    D_TESTS_ENV_PRINTER_CHECK(d_tests_env_printer_run(&printer) == 0);
    D_TESTS_ENV_PRINTER_CHECK(d_env_printer_put(&printer, "-entry").is_ok);
    D_TESTS_ENV_PRINTER_CHECK(
        d_env_printer_last_error(&printer)->status == D_OSTREAM_TEMPLATE_OK);
    D_TESTS_ENV_PRINTER_CHECK( (d_tests_env_printer_run(&printer) == 1) &&
                               (strstr(d_tests_env_printer_text,
                                       "  Debug       yes\n") != NULL) );

    // a program's own variable, set by text, and used
    const struct dj_option_result declared =
        d_env_printer_declare(&printer, &MARKER[0]);

    D_TESTS_ENV_PRINTER_CHECK( (declared.is_ok) &&
                               (declared.payload.value ==
                                D_ENV_FIELDS + D_ENV_PRINTER_VARS) );
    D_TESTS_ENV_PRINTER_CHECK(!d_env_printer_declare(&printer, &MARKER[1])
                              .is_ok);
    (void)d_env_printer_load(&printer,
                             "-section_begin; entry=\"{marker} {label}\\n\";"
                             "build.marker=\"->\"",
                             NULL);
    D_TESTS_ENV_PRINTER_CHECK( (d_tests_env_printer_run(&printer) == 1) &&
                               (strcmp(d_tests_env_printer_text,
                                       "Build\n-> Build Type\n-> Debug\n")
                                == 0) );

    // the presets
    D_TESTS_ENV_PRINTER_CHECK(strcmp(d_env_style_template(D_ENV_STYLE_JSON,
                                                         D_ENV_TEMPLATE_NOTE),
                                     "{prefix}    \"note\": {note!json}")
                              == 0);
    D_TESTS_ENV_PRINTER_CHECK(d_env_style_template(5, 0) == NULL);
    D_TESTS_ENV_PRINTER_CHECK(d_env_style_template(0, 9) == NULL);
    d_tests_env_printer_source = NULL;

    return;
}

/*
d_tests_env_printer_hide_false
  A filter: shows every entry but a false boolean.
*/
static int32_t
d_tests_env_printer_hide_false(
    void*                     _context,
    const struct dj_env_item* _item
)
{
    ++*(int*)_context;

    return !( (_item->entry != NULL) &&
              (_item->entry->kind == D_ENV_VALUE_BOOLEAN) &&
              (_item->entry->integer == 0L) );
}

/*
d_tests_env_printer_rename
  A field hook: renames the compiler's name's label, gives the field
"marker"-less programs none, and stops on a FIRST field for os.
*/
static int32_t
d_tests_env_printer_rename(
    void*                     _context,
    const struct dj_env_item* _item,
    uint32_t                  _field,
    struct dj_ostream_value*  _value
)
{
    (void)_context;

    if ( (_field == D_ENV_FIELD_LABEL)            &&
         (_item->entry != NULL)                   &&
         (strcmp(_item->section->slug, "compiler") == 0) &&
         (strcmp(_item->entry->name, "name") == 0) )
    {
        *_value = d_ostream_value_text("Toolchain");

        return 1;
    }

    if ( (_field == D_ENV_FIELD_FIRST) &&
         (_item->section != NULL)      &&
         (strcmp(_item->section->slug, "os") == 0) )
    {
        return -1;
    }

    return 0;
}

/*
d_tests_env_printer_hooks
  Tests the following:
  - a filter hides, and is asked
  - a field hook renames; one that stops ends the print as INVALID
  - a change made through d_env_printer_env is seen at the next print
*/
static void
d_tests_env_printer_hooks(void)
{
    struct dj_env_printer printer;
    int                   asked = 0;

    printf("  hooks\n");

    d_tests_env_printer_source = &D_TESTS_REPORT;

    (void)d_env_printer_init(&printer);
    (void)d_env_printer_load(&printer, "sections=os; show_title=0", NULL);
    d_env_printer_set_filter(&printer, d_tests_env_printer_hide_false, &asked);
    D_TESTS_ENV_PRINTER_CHECK( (d_tests_env_printer_run(&printer) == 1) &&
                               (asked > 0)                              &&
                               (strstr(d_tests_env_printer_text,
                                       "Windows") == NULL)              &&
                               (strstr(d_tests_env_printer_text,
                                       "Platform") != NULL) );
    d_env_printer_set_filter(&printer, NULL, NULL);

    // a hook that renames, then one that stops
    d_env_printer_set_field(&printer, d_tests_env_printer_rename, NULL);
    (void)d_env_printer_put(&printer, "sections=compiler");
    D_TESTS_ENV_PRINTER_CHECK( (d_tests_env_printer_run(&printer) == 1) &&
                               (strstr(d_tests_env_printer_text,
                                       "  Toolchain  GCC\n") != NULL) );
    (void)d_env_printer_load(&printer,
                             "sections=os; entry=\"{?first}*{/}{label}\"",
                             NULL);
    D_TESTS_ENV_PRINTER_CHECK(d_tests_env_printer_run(&printer) == 0);
    d_env_printer_set_field(&printer, NULL, NULL);

    // a change made behind the printer's back
    (void)d_option_env_set_int(d_env_printer_env(&printer),
                               D_OPTION_ENV_GLOBAL,
                               d_env_printer_var_id(&printer,
                                                    D_ENV_PRINTER_STYLE),
                               D_ENV_STYLE_JSON);
    (void)d_env_printer_put(&printer, "-entry");
    D_TESTS_ENV_PRINTER_CHECK( (d_tests_env_printer_run(&printer) == 1) &&
                               (d_tests_env_printer_text[0] == '{') );
    d_tests_env_printer_source = NULL;

    return;
}

/*
d_tests_env_printer_dump
  Tests the following:
  - a dump lists what is bound, scoped bindings and text quoted
  - loading a dump into a fresh printer prints the same report
  - with defaults, every variable is listed, the unbound commented out
*/
static void
d_tests_env_printer_dump(void)
{
    static char             first[D_TESTS_ENV_PRINTER_TEXT];
    static char             script[8192];
    struct dj_env_printer   printer;
    struct dj_env_printer   again;
    struct dj_ostream       os;
    struct dj_ostream_fixed fixed;

    printf("  dumps\n");

    (void)d_env_printer_init(&printer);
    (void)d_env_printer_load(&printer,
                             "style=markdown; sections=core,-posix;"
                             "title=\"Quoted \\\"title\\\"\\tand tab\";"
                             "build.true_text=TRUE; key_width=3;"
                             "select=\"-*.has_*, -os.id\";"
                             "item_separator=\"\"",
                             NULL);
    (void)d_tests_env_printer_run(&printer);
    memcpy(first, d_tests_env_printer_text, sizeof(first));

    (void)d_ostream_init_fixed(&os, &fixed, script, sizeof(script), 1);
    D_TESTS_ENV_PRINTER_CHECK(d_env_printer_dump(&printer, &os, 0) == 1);
    (void)d_ostream_close(&os);
    D_TESTS_ENV_PRINTER_CHECK(strstr(script,
        "title = \"Quoted \\\"title\\\"\\tand tab\"\n") != NULL);
    D_TESTS_ENV_PRINTER_CHECK(strstr(script, "build.true_text = \"TRUE\"\n")
                              != NULL);
    D_TESTS_ENV_PRINTER_CHECK(strstr(script, "\nsections = ") != NULL);
    D_TESTS_ENV_PRINTER_CHECK(strstr(script, "show_title") == NULL);

    (void)d_env_printer_init(&again);
    D_TESTS_ENV_PRINTER_CHECK(d_env_printer_load(&again, script, NULL).is_ok);
    D_TESTS_ENV_PRINTER_CHECK( (d_tests_env_printer_run(&again) == 1) &&
                               (strcmp(first, d_tests_env_printer_text)
                                == 0) );

    // with the defaults: every variable, the unbound commented out
    (void)d_ostream_init_fixed(&os, &fixed, script, sizeof(script), 1);
    D_TESTS_ENV_PRINTER_CHECK(d_env_printer_dump(&printer, &os, 1) == 1);
    (void)d_ostream_close(&os);
    D_TESTS_ENV_PRINTER_CHECK(strstr(script, "\n# show_title = true\n")
                              != NULL);
    D_TESTS_ENV_PRINTER_CHECK(strstr(script, "\n# -entry\n") != NULL);
    D_TESTS_ENV_PRINTER_CHECK(strstr(script,
        "# style (choice: text, json, markdown, csv, keyvalue): ") != NULL);
    (void)d_env_printer_init(&again);
    D_TESTS_ENV_PRINTER_CHECK(d_env_printer_load(&again, script, NULL).is_ok);
    D_TESTS_ENV_PRINTER_CHECK( (d_tests_env_printer_run(&again) == 1) &&
                               (strcmp(first, d_tests_env_printer_text)
                                == 0) );

    return;
}

/*
d_tests_env_printer_dump_into
  Writes a printer's dump into a buffer, NUL-terminated; 1 when it all fit.
*/
static int
d_tests_env_printer_dump_into(
    const struct dj_env_printer* _printer,
    char*                        _buffer,
    size_t                       _capacity
)
{
    struct dj_ostream       os;
    struct dj_ostream_fixed fixed;

    (void)d_ostream_init_fixed(&os, &fixed, _buffer, _capacity, 1);

    const int32_t dumped = d_env_printer_dump(_printer, &os, 0);

    return ( (d_ostream_close(&os) != 0) &&
             (dumped != 0) ) ? 1 : 0;
}

// the djin's groups: a registry of the harness's own, a printer made in the
// djin and one standing alone
static struct dj_option_env_registry d_tests_env_printer_registry;
static struct dj_env_printer         d_tests_env_printer_shared;
static struct dj_env_printer         d_tests_env_printer_alone;

// ROOT
//   constant: a variable the registry's root declares.
static const struct dj_option_env_var D_TESTS_ROOT[] =
{
    D_OPTION_ENV_TEXT("company", "Acme", "the root's own")
};

/*
d_tests_env_printer_djin_reads
  Tests the following:
  - the djin declares the printer's variables, in order, and the sections'
    scopes, before anything runs
  - registered, its settings arrive by full name, a scoped one among them
  - a printer made in it reads them, its own binding holds over theirs from
    the same source, and reset takes it back to them; a printer standing
    alone does not see them
*/
static void
d_tests_env_printer_djin_reads(void)
{
    struct dj_option_env* const          djin     = d_env_printer_djin();
    struct dj_option_env_registry* const registry =
        &d_tests_env_printer_registry;
    struct dj_env_printer* const         shared   = &d_tests_env_printer_shared;
    struct dj_env_printer* const         alone    = &d_tests_env_printer_alone;
    int                                  same     = 1;

    // its declarations are the printer's own, in order, ready from the start
    (void)d_env_printer_init(alone);
    D_TESTS_ENV_PRINTER_CHECK(d_option_env_var_count(djin) ==
                              (uint32_t)D_ENV_PRINTER_VARS);

    for (uint32_t i = 0u; i < (uint32_t)D_ENV_PRINTER_VARS; ++i)
    {
        const struct dj_option_env_var* const mine =
            d_option_env_var_at(djin, i);
        const struct dj_option_env_var* const its  =
            d_option_env_var_at(d_env_printer_env(alone), i);

        same = ( (same != 0)    &&
                 (mine != NULL) &&
                 (its != NULL)  &&
                 (mine == its) ) ? 1 : 0;
    }

    D_TESTS_ENV_PRINTER_CHECK(same == 1);
    D_TESTS_ENV_PRINTER_CHECK(d_option_env_find_scope(djin,
                                                      "cpp_features",
                                                      12u).is_ok);
    D_TESTS_ENV_PRINTER_CHECK(d_env_printer_var_id(alone,
                                                   D_ENV_PRINTER_TITLE) ==
                              (uint32_t)D_ENV_PRINTER_TITLE);

    // registered: settings by full name, a scoped one among them
    D_TESTS_ENV_PRINTER_CHECK(d_option_env_registry_init(registry,
                                                         "djintest").is_ok);
    D_TESTS_ENV_PRINTER_CHECK(d_option_env_declare(
        d_option_env_registry_frame(registry), &D_TESTS_ROOT[0]).is_ok);
    D_TESTS_ENV_PRINTER_CHECK(d_env_printer_register(registry).is_ok);
    D_TESTS_ENV_PRINTER_CHECK(d_option_env_registry_find(registry,
                                                         "env_printer",
                                                         11u) == djin);
    D_TESTS_ENV_PRINTER_CHECK(d_option_env_registry_load(
        registry,
        "env_printer.sections=build; env_printer.show_title=0;"
        "env_printer.build.prefix=\"> \"; env_printer.key_width=3",
        D_OPTION_ENV_SOURCE_FILE,
        NULL).is_ok);

    // a printer made in it reads them; one standing alone does not
    D_TESTS_ENV_PRINTER_CHECK(d_env_printer_init_in(shared, djin) == 1);
    D_TESTS_ENV_PRINTER_CHECK(d_env_printer_var_id(shared,
                                                   D_ENV_PRINTER_STYLE) ==
                              D_OPTION_ENV_ID(1u, D_ENV_PRINTER_STYLE));
    D_TESTS_ENV_PRINTER_CHECK( (d_tests_env_printer_run(shared) == 1) &&
                               (strcmp(d_tests_env_printer_text,
                                       "> Build\n"
                                       ">   Build Type  Debug\n"
                                       ">   Debug  yes\n") == 0) );
    D_TESTS_ENV_PRINTER_CHECK( (d_tests_env_printer_run(alone) == 1) &&
                               (strncmp(d_tests_env_printer_text,
                                        "djinterp environment\n",
                                        21u) == 0) );

    // its own binding holds over the djin's from the same source; a lower
    // source's does not; reset takes it back
    D_TESTS_ENV_PRINTER_CHECK(d_option_env_put_from(d_env_printer_env(shared),
                                                    "key_width=0",
                                                    D_OPTION_ENV_SOURCE_FILE)
                              .is_ok);
    D_TESTS_ENV_PRINTER_CHECK(d_env_printer_put(shared, "show_title").is_ok);
    D_TESTS_ENV_PRINTER_CHECK( (d_tests_env_printer_run(shared) == 1) &&
                               (strcmp(d_tests_env_printer_text,
                                       "> Build\n"
                                       ">   Build Type  Debug\n"
                                       ">   Debug       yes\n") == 0) );
    d_env_printer_reset(shared);
    D_TESTS_ENV_PRINTER_CHECK( (d_tests_env_printer_run(shared) == 1) &&
                               (strstr(d_tests_env_printer_text,
                                       ">   Debug  yes\n") != NULL) );

    return;
}

/*
d_tests_env_printer_djin_sources
  Tests the following:
  - a source of higher standing holds whatever the order
  - command-line options, a misspelling's suggestion; the environment under
    the registry's prefix
*/
static void
d_tests_env_printer_djin_sources(void)
{
    static char*                         ARGV[] =
    {
        "t_env_printer",
        "--env_printer.style=csv",
        "--no-env_printer.show_title",
        "--env_printer.styel=json",
        "operand"
    };
    struct dj_option_env* const          djin     = d_env_printer_djin();
    struct dj_option_env_registry* const registry =
        &d_tests_env_printer_registry;
    struct dj_env_printer* const         shared   = &d_tests_env_printer_shared;
    struct dj_option_result              result;
    int                                  at       = 1;
    char                                 suggestion[64];

    // the standing of sources, whatever their order
    (void)d_option_env_registry_put(registry,
                                    "env_printer.title=file",
                                    D_OPTION_ENV_SOURCE_FILE);
    (void)d_option_env_registry_put(registry,
                                    "env_printer.title=line",
                                    D_OPTION_ENV_SOURCE_COMMAND_LINE);
    (void)d_option_env_registry_put(registry,
                                    "env_printer.title=file again",
                                    D_OPTION_ENV_SOURCE_FILE);
    D_TESTS_ENV_PRINTER_CHECK( (strcmp(d_option_env_text(
                                           djin,
                                           D_OPTION_ENV_GLOBAL,
                                           d_env_printer_var_id(
                                               shared,
                                               D_ENV_PRINTER_TITLE)),
                                       "line") == 0) &&
                               (d_option_env_source(
                                    d_env_printer_env(shared),
                                    D_OPTION_ENV_GLOBAL,
                                    d_env_printer_var_id(
                                        shared,
                                        D_ENV_PRINTER_TITLE)) ==
                                D_OPTION_ENV_SOURCE_COMMAND_LINE) );

    // the command line, a misspelling suggested; the environment
    D_TESTS_ENV_PRINTER_CHECK(d_option_env_registry_take_option(
        registry, 5, ARGV, &at, NULL) == 1);
    ++at;
    D_TESTS_ENV_PRINTER_CHECK(d_option_env_registry_take_option(
        registry, 5, ARGV, &at, NULL) == 1);
    ++at;
    D_TESTS_ENV_PRINTER_CHECK(d_option_env_registry_take_option(
        registry, 5, ARGV, &at, &result) == -1);
    D_TESTS_ENV_PRINTER_CHECK( (d_option_env_registry_suggest(
                                    registry,
                                    "env_printer.styel",
                                    17u,
                                    suggestion,
                                    sizeof(suggestion)) == 17u) &&
                               (strcmp(suggestion,
                                       "env_printer.style") == 0) );
    ++at;
    D_TESTS_ENV_PRINTER_CHECK(d_option_env_registry_take_option(
        registry, 5, ARGV, &at, NULL) == 0);
    D_TESTS_ENV_PRINTER_CHECK( (d_tests_env_printer_run(shared) == 1) &&
                               (strncmp(d_tests_env_printer_text,
                                        "section,name,label,",
                                        19u) == 0) );
    (void)d_option_env_unset_from(djin,
                                  D_OPTION_ENV_GLOBAL,
                                  d_env_printer_var_id(shared,
                                                       D_ENV_PRINTER_STYLE),
                                  D_OPTION_ENV_SOURCE_COMMAND_LINE);
#if defined(_WIN32)
    (void)_putenv_s("DJINTEST_ENV_PRINTER_TRUE_TEXT", "Y");
#else
    (void)setenv("DJINTEST_ENV_PRINTER_TRUE_TEXT", "Y", 1);
#endif
    D_TESTS_ENV_PRINTER_CHECK(d_option_env_registry_import(registry,
                                                           "DJINTEST_").is_ok);
    D_TESTS_ENV_PRINTER_CHECK( (d_tests_env_printer_run(shared) == 1) &&
                               (strcmp(d_tests_env_printer_text,
                                       "> Build\n"
                                       ">   Build Type  Debug\n"
                                       ">   Debug  Y\n") == 0) );

    return;
}

/*
d_tests_env_printer_djin_last
  Tests the following:
  - a dump of a printer in it, loaded into one standing alone, prints the
    same report
  - a program's own variable takes the field after the printer's; a name a
    frame above has is refused; a variable the root declares is named
  - released, the djin can leave the registry, and its bindings with it
*/
static void
d_tests_env_printer_djin_last(void)
{
    static const struct dj_option_env_var OWN[] =
    {
        D_OPTION_ENV_TEXT("marker", "=>", "a program's own"),
        D_OPTION_ENV_TEXT("style", NULL, "the printer's own name"),
        D_OPTION_ENV_ROW(D_OPTION_ENV_KIND_TEXT, "headline", NULL, 0u, NULL,
                         0u, NULL, 0, 0, 0, 0.0, "company",
                         NULL, "an alias the root's has")
    };
    static char                           script[8192];
    static char                           first[D_TESTS_ENV_PRINTER_TEXT];
    struct dj_option_env* const           djin     = d_env_printer_djin();
    struct dj_option_env_registry* const  registry =
        &d_tests_env_printer_registry;
    struct dj_env_printer* const          shared   =
        &d_tests_env_printer_shared;
    struct dj_env_printer                 again;
    struct dj_option_result               result;

    // its dump, loaded into a printer standing alone, prints the same
    memcpy(first, d_tests_env_printer_text, sizeof(first));
    D_TESTS_ENV_PRINTER_CHECK(d_tests_env_printer_dump_into(shared,
                                                            script,
                                                            sizeof(script))
                              == 1);
    D_TESTS_ENV_PRINTER_CHECK(strstr(script, "build.prefix = \"> \"\n")
                              != NULL);
    (void)d_env_printer_init(&again);
    D_TESTS_ENV_PRINTER_CHECK(d_env_printer_load(&again, script, NULL).is_ok);
    D_TESTS_ENV_PRINTER_CHECK( (d_tests_env_printer_run(&again) == 1) &&
                               (strcmp(first, d_tests_env_printer_text)
                                == 0) );

    // a program's own variable, a name a frame above has, the root's own
    result = d_env_printer_declare(shared, &OWN[0]);
    D_TESTS_ENV_PRINTER_CHECK( (result.is_ok) &&
                               (result.payload.value ==
                                (uint32_t)(D_ENV_FIELDS +
                                           D_ENV_PRINTER_VARS)) );
    result = d_env_printer_declare(shared, &OWN[1]);
    D_TESTS_ENV_PRINTER_CHECK( (!result.is_ok) &&
                               (d_option_result_status(&result) ==
                                (int32_t)D_OPTION_STATUS_KEY_DUPLICATE) );
    result = d_env_printer_declare(shared, &OWN[2]);
    D_TESTS_ENV_PRINTER_CHECK( (!result.is_ok) &&
                               (d_option_result_status(&result) ==
                                (int32_t)D_OPTION_STATUS_KEY_DUPLICATE) );
    D_TESTS_ENV_PRINTER_CHECK(d_env_printer_load(
        shared,
        "section_begin=\"\"; entry=\"{marker}{company}:{name}\\n\"",
        NULL).is_ok);
    D_TESTS_ENV_PRINTER_CHECK( (d_tests_env_printer_run(shared) == 1) &&
                               (strcmp(d_tests_env_printer_text,
                                       "=>Acme:type\n=>Acme:debug\n")
                                == 0) );

    // released: the djin leaves the registry, its bindings with it
    d_env_printer_release(shared);
    d_env_printer_release(shared);
    D_TESTS_ENV_PRINTER_CHECK(d_tests_env_printer_run(shared) == 0);
    D_TESTS_ENV_PRINTER_CHECK(d_option_env_registry_remove(registry,
                                                           djin).is_ok);
    D_TESTS_ENV_PRINTER_CHECK( (d_option_env_registry_count(registry) == 0u) &&
                               (djin->parent == NULL)                         &&
                               (djin->set.count == 0u) );
    D_TESTS_ENV_PRINTER_CHECK(d_env_printer_init_in(shared, djin) == 1);
    D_TESTS_ENV_PRINTER_CHECK( (d_tests_env_printer_run(shared) == 1) &&
                               (strncmp(d_tests_env_printer_text,
                                        "djinterp environment\n",
                                        21u) == 0) );
    d_env_printer_release(shared);
    D_TESTS_ENV_PRINTER_CHECK(djin->child_count == 0u);

    return;
}

/*
d_tests_env_printer_djin
  The djin's groups, in order: each leaves the djin as the next needs it.
*/
static void
d_tests_env_printer_djin(void)
{
    printf("  the djin\n");

    d_tests_env_printer_source = &D_TESTS_REPORT;
    d_tests_env_printer_djin_reads();
    d_tests_env_printer_djin_sources();
    d_tests_env_printer_djin_last();
    d_tests_env_printer_source = NULL;

    return;
}

/*
d_tests_env_printer_marked
  A field hook: writes "@@" for the field its context holds the number of.
*/
static int32_t
d_tests_env_printer_marked(
    void*                     _context,
    const struct dj_env_item* _item,
    uint32_t                  _field,
    struct dj_ostream_value*  _value
)
{
    (void)_item;

    if (_field == *(const uint32_t*)_context)
    {
        *_value = d_ostream_value_text("@@");

        return 1;
    }

    return 0;
}

/*
d_tests_env_printer_moved
  Tests the following:
  - a printer whose frame is given a parent after it was made follows its
    variables' new ids, and keeps its declared fields' numbers
*/
static void
d_tests_env_printer_moved(void)
{
    static const struct dj_option_env_var OWN[] =
    {
        D_OPTION_ENV_TEXT("marker", "=>", "a program's own")
    };
    struct dj_env_printer                 host;
    struct dj_env_printer                 moved;
    struct dj_option_result               field;

    printf("  a frame moved\n");

    d_tests_env_printer_source = &D_TESTS_REPORT;

    (void)d_env_printer_init(&host);
    (void)d_env_printer_init(&moved);
    field = d_env_printer_declare(&moved, &OWN[0]);
    D_TESTS_ENV_PRINTER_CHECK(d_option_env_set_parent(
        d_env_printer_env(&moved), d_env_printer_env(&host)).is_ok);
    D_TESTS_ENV_PRINTER_CHECK(d_env_printer_var_id(&moved,
                                                   D_ENV_PRINTER_ENTRY) ==
                              D_OPTION_ENV_ID(1u, D_ENV_PRINTER_ENTRY));
    D_TESTS_ENV_PRINTER_CHECK(d_env_printer_load(
        &moved,
        "sections=build; show_title=0; section_begin=\"\";"
        "entry=\"{marker}{name}\\n\"",
        NULL).is_ok);
    D_TESTS_ENV_PRINTER_CHECK( (d_tests_env_printer_run(&moved) == 1) &&
                               (strcmp(d_tests_env_printer_text,
                                       "=>type\n=>debug\n") == 0) );
    D_TESTS_ENV_PRINTER_CHECK( (field.is_ok) &&
                               (d_env_printer_declare(&moved, &OWN[0])
                                .is_ok == false) );

    // the declared field's number is the one the template uses
    d_env_printer_set_field(&moved,
                            d_tests_env_printer_marked,
                            &field.payload.value);
    D_TESTS_ENV_PRINTER_CHECK( (d_tests_env_printer_run(&moved) == 1) &&
                               (strcmp(d_tests_env_printer_text,
                                       "@@type\n@@debug\n") == 0) );
    d_env_printer_release(&moved);
    d_tests_env_printer_source = NULL;

    return;
}

/*
d_tests_env_printer_environment
  Tests the following:
  - variables read from the process's environment under the prefix
*/
static void
d_tests_env_printer_environment(void)
{
    struct dj_env_printer printer;

    printf("  the process environment\n");

#if defined(_WIN32)
    (void)_putenv_s("DJINTERP_ENV_PRINTER_STYLE", "keyvalue");
    (void)_putenv_s("DJINTERP_ENV_PRINTER_SECTIONS", "build");
    (void)_putenv_s("DJINTERP_ENV_PRINTER_SHOW_TITLE", "no");
#else
    (void)setenv("DJINTERP_ENV_PRINTER_STYLE", "keyvalue", 1);
    (void)setenv("DJINTERP_ENV_PRINTER_SECTIONS", "build", 1);
    (void)setenv("DJINTERP_ENV_PRINTER_SHOW_TITLE", "no", 1);
#endif

    (void)d_env_printer_init(&printer);

    const struct dj_option_result imported =
        d_env_printer_import(&printer, D_ENV_PRINTER_ENV_PREFIX);

    D_TESTS_ENV_PRINTER_CHECK( (imported.is_ok) &&
                               (imported.payload.value == 3u) );
    D_TESTS_ENV_PRINTER_CHECK( (d_tests_env_printer_run(&printer) == 1) &&
                               (strncmp(d_tests_env_printer_text,
                                        "build.type=", 11u) == 0) );

    return;
}

/*
d_tests_env_printer_files
  Tests the following:
  - every section as JSON and as CSV, to files a parser reads
  - the text to the console
*/
static void
d_tests_env_printer_files(
    const char* _json,
    const char* _csv
)
{
    FILE* const json = fopen(_json, "wb");
    FILE* const csv  = fopen(_csv, "wb");

    printf("  files\n");

    D_TESTS_ENV_PRINTER_CHECK( (json != NULL) &&
                               (csv != NULL) );

    if ( (json == NULL) ||
         (csv == NULL) )
    {
        return;
    }

    D_TESTS_ENV_PRINTER_CHECK(d_env_print_file(json,
                                               "style=json; sections=all")
                              == 1);
    D_TESTS_ENV_PRINTER_CHECK(d_env_print_file(csv,
                                               "style=csv; sections=all")
                              == 1);
    D_TESTS_ENV_PRINTER_CHECK(fclose(json) == 0);
    D_TESTS_ENV_PRINTER_CHECK(fclose(csv) == 0);
    D_TESTS_ENV_PRINTER_CHECK(d_env_print_file(stdout, NULL) == 1);

    return;
}

/*
main
  Every group, then the count.
*/
int
main(
    int    _argc,
    char** _argv
)
{
    printf("t_env_printer\n");

    d_tests_env_printer_report();
    d_tests_env_printer_defaults();
    d_tests_env_printer_selection();
    d_tests_env_printer_styles();
    d_tests_env_printer_presets();
    d_tests_env_printer_templates();
    d_tests_env_printer_hooks();
    d_tests_env_printer_dump();
    d_tests_env_printer_djin();
    d_tests_env_printer_moved();
    d_tests_env_printer_environment();
    d_tests_env_printer_files((_argc > 1) ? _argv[1] : "t_env_printer.json",
                              (_argc > 2) ? _argv[2] : "t_env_printer.csv");

    printf("%lu checks, %lu failed\n",
           d_tests_env_printer_checks,
           d_tests_env_printer_failures);

    return (d_tests_env_printer_failures == 0u) ? 0 : 1;
}
