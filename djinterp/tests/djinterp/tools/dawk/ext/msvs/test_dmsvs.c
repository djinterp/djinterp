/*******************************************************************************
* djinterp [tools]                                                  test_dmsvs.c
*
* Exercises `dmsvs.h`: kinds by name; the XML element tree, its places, its
* attributes and its text, decoded as XML decodes them; a solution in either
* form, each record with the text it spans; conditions and configurations;
* GUIDs made from names, relative paths and escaping; and every refusal,
* with where it is reported.
*   With --corpus, it reads every Visual Studio file under a directory
* instead and proves each reading sound: every element lies within its
* parent, its tags where its places say, its name the one its end tag
* repeats, and its line the one its offset gives.  With --dump, it prints a
* file's element tree as tests/djinterp/tools/dawk/ext/msvs/xml_tree.py prints the
* same file's, read by Python's own XML parser, so the two can be compared.
*
*   test_dmsvs                    the unit checks
*   test_dmsvs --corpus <dir>...  every Visual Studio file under each
*   test_dmsvs --dump <file>      the file's element tree
*
* path:      /tests/djinterp/tools/dawk/ext/msvs/test_dmsvs.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/msvs/dmsvs.h"  // corresponding header
// std
#include <stdio.h>   // printf, putchar, snprintf
#include <stdlib.h>  // malloc, free, qsort
#include <string.h>  // strcmp, strncmp, strlen, strspn, strstr, memcmp, memcpy,
                     // memset
// djinterp
#include "../../../../../../inc/djinterp/tools/dawk/dfs.h"      // d_fs_names
#include "../../../../../../inc/djinterp/tools/dawk/dsplice.h"  // d_text_read


static int g_checks = 0;
static int g_failed = 0;


/*
d_tests_check
  Counts one check, and names it when it fails.
*/
static void
d_tests_check(
    bool        _condition,
    const char* _what
)
{
    g_checks++;

    // a failed check is named, so a green run says what it checked
    if (!_condition)
    {
        g_failed++;
        printf("  FAIL  %s\n", _what);
    }

    return;
}


/*
d_tests_starts
  Whether a text begins with another.
*/
static bool
d_tests_starts(
    const char* _text,
    const char* _start
)
{
    return (strncmp(_text, _start, strlen(_start)) == 0);
}


/*
d_tests_read
  Reads a text that must read; a failure is reported as the check named.
*/
static bool
d_tests_read(
    struct d_msvs*   _msvs,
    const char*      _text,
    enum d_msvs_kind _kind,
    const char*      _what
)
{
    char       why[256];
    const bool read = d_msvs_read(_msvs, _text, strlen(_text), _kind, why,
                                  sizeof(why));

    g_checks++;

    // the reason is what a reader of the failure needs
    if (!read)
    {
        printf("  FAIL  %s: %s\n", _what, why);
        g_failed++;
    }

    return read;
}


/*
d_tests_attribute_is
  Whether an element's attribute decodes to the text expected.
*/
static bool
d_tests_attribute_is(
    const struct d_msvs* _msvs,
    size_t               _element,
    const char*          _name,
    const char*          _expected
)
{
    size_t start  = 0;
    size_t length = 0;
    char   value[512];

    // an attribute that is not there is no value
    if ( (_element >= _msvs->element_count) ||
         (!d_msvs_attribute(_msvs, &_msvs->elements[_element], _name,
                            &start, &length)) ||
         (length >= sizeof(value)) )
    {
        return false;
    }

    (void)d_msvs_value(_msvs, start, length, value);

    return (strcmp(value, _expected) == 0);
}


/*
d_tests_text_is
  Whether an element's text decodes to the text expected.
*/
static bool
d_tests_text_is(
    const struct d_msvs* _msvs,
    size_t               _element,
    const char*          _expected
)
{
    char value[512];

    // an element that is not there has no text
    if ( (_element >= _msvs->element_count) ||
         ((_msvs->elements[_element].content_end -
           _msvs->elements[_element].content_start) >= sizeof(value)) )
    {
        return false;
    }

    (void)d_msvs_text(_msvs, &_msvs->elements[_element], value);

    return (strcmp(value, _expected) == 0);
}


/*
d_tests_kinds
  Tests the following:
  - every kind by its name's ending, in any case
*/
static void
d_tests_kinds(void)
{
    printf("kinds\n");

    d_tests_check( (d_msvs_kind_of("a/djinterp.slnx") == D_MSVS_SOLUTION) &&
                   (d_msvs_kind_of("old.SLN") == D_MSVS_SOLUTION) &&
                   (d_msvs_kind_of("x.vcxproj") == D_MSVS_PROJECT) &&
                   (d_msvs_kind_of("x.vcxproj.filters") == D_MSVS_FILTERS) &&
                   (d_msvs_kind_of("x.vcxproj.user") == D_MSVS_USER) &&
                   (d_msvs_kind_of("x.csproj") == D_MSVS_PROJECT) &&
                   (d_msvs_kind_of("Directory.Build.props")
                    == D_MSVS_PROPS) &&
                   (d_msvs_kind_of("x.targets") == D_MSVS_PROPS),
                   "each kind by its ending");
    d_tests_check( (d_msvs_kind_of("x.xml") == D_MSVS_OTHER) &&
                   (d_msvs_kind_of(".sln") == D_MSVS_OTHER) &&
                   (d_msvs_kind_of(NULL) == D_MSVS_OTHER),
                   "...and nothing else");

    return;
}


/*
d_tests_tree
  Tests the following:
  - elements in document order, linked, with their places
  - a byte-order mark and CR LF, kept as facts
  - attributes raw and decoded, and text decoded as XML decodes it
*/
static void
d_tests_tree(void)
{
    static const char text[] =
        "\xEF\xBB\xBF<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n"
        "<Project DefaultTargets=\"Build\" xmlns=\"urn:x\">\r\n"
        "  <!-- a comment -->\r\n"
        "  <PropertyGroup Label='Globals'>\r\n"
        "    <Name>a &amp; b &#x41;</Name>\r\n"
        "    <Empty />\r\n"
        "    <Spaced Value=\"x&#10;y\tz\r\nw\">p\r\nq<![CDATA[<c>\r\n]]>"
        "</Spaced>\r\n"
        "  </PropertyGroup>\r\n"
        "</Project>\r\n";

    struct d_msvs msvs;

    printf("tree\n");

    if (d_tests_read(&msvs, text, D_MSVS_PROJECT, "a project"))
    {
        const struct d_msvs_element* const e = msvs.elements;

        d_tests_check( (msvs.element_count == 5u) && (msvs.bom) &&
                       (msvs.crlf) && (msvs.xml) &&
                       (msvs.kind == D_MSVS_PROJECT),
                       "five elements, a mark, CR LF");
        d_tests_check( d_msvs_is(&msvs, &e[0], "Project") &&
                       (e[0].parent == D_MSVS_NONE) &&
                       (e[0].first_child == 1u) && (e[0].child_count == 1u) &&
                       (e[1].parent == 0u) && (e[1].child_count == 3u) &&
                       (e[1].first_child == 2u) && (e[2].next_sibling == 3u) &&
                       (e[3].next_sibling == 4u) &&
                       (e[4].next_sibling == D_MSVS_NONE) &&
                       (e[4].depth == 2u),
                       "elements linked in document order");
        d_tests_check( (e[0].line == 2u) && (e[0].column == 1u) &&
                       (e[1].line == 4u) && (e[1].column == 3u) &&
                       (e[3].empty) && (e[3].content_start == e[3].end) &&
                       (e[4].end_line == 10u) && (e[0].end_line == 12u) &&
                       (msvs.text[e[0].end - 1u] == '>') &&
                       (memcmp(&msvs.text[e[1].content_end], "</Property",
                               10u) == 0),
                       "each element's places");
        d_tests_check( (d_msvs_child(&msvs, 1u, "Empty") == 3u) &&
                       (d_msvs_child(&msvs, 1u, "Nothing") == D_MSVS_NONE),
                       "a child by its name");
        d_tests_check( d_tests_attribute_is(&msvs, 1u, "Label", "Globals") &&
                       d_tests_attribute_is(&msvs, 0u, "xmlns", "urn:x") &&
                       !d_tests_attribute_is(&msvs, 0u, "Label", ""),
                       "attributes in either quote");
        d_tests_check(d_tests_attribute_is(&msvs, 4u, "Value",
                                           "x\ny z w"),
                      "a value's tabs and line endings are spaces, a "
                      "reference to one is kept");
        d_tests_check(d_tests_text_is(&msvs, 2u, "a & b A"),
                      "references in text");
        d_tests_check(d_tests_text_is(&msvs, 4u, "p\nq<c>\n"),
                      "line endings in text are LF, CDATA as it stands");
        d_tests_check( d_tests_text_is(&msvs, 3u, "") &&
                       d_tests_text_is(&msvs, 1u, ""),
                       "an empty element, and one holding others, have "
                       "no text");
    }

    d_msvs_release(&msvs);

    return;
}


/*
d_tests_platforms_alone
  Whether a .slnx naming platforms alone, as Visual Studio writes one for a
new C++ project, builds Debug and Release on each.
*/
static bool
d_tests_platforms_alone(void)
{
    static const char text[] =
        "<Solution>\r\n"
        "  <Configurations>\r\n"
        "    <Platform Name=\"x64\" />\r\n"
        "    <Platform Name=\"x86\" />\r\n"
        "  </Configurations>\r\n"
        "  <Project Path=\"a.vcxproj\" Id=\"a9da23ff-789e-488c-8c20-"
        "ff468f17ac7d\" />\r\n"
        "</Solution>\r\n";
    struct d_msvs msvs;
    const bool    read = d_msvs_read(&msvs, text, strlen(text),
                                     D_MSVS_SOLUTION, NULL, 0u);
    const bool    four = (read) && (msvs.configuration_count == 4u) &&
                         (strcmp(msvs.configurations[0], "Debug|x64") == 0) &&
                         (strcmp(msvs.configurations[3], "Release|x86") == 0);

    d_msvs_release(&msvs);

    return four;
}


/*
d_tests_slnx
  Tests the following:
  - projects at the top and in folders, named, typed, with dependencies
  - the configurations, each build type on each platform
*/
static void
d_tests_slnx(void)
{
    static const char text[] =
        "<Solution>\n"
        "  <Configurations>\n"
        "    <BuildType Name=\"Debug\" />\n"
        "    <BuildType Name=\"Release\" />\n"
        "    <Platform Name=\"x64\" />\n"
        "    <Platform Name=\"ARM64\" />\n"
        "  </Configurations>\n"
        "  <Project Path=\"app/app.vcxproj\" Type=\"{8BC9CEB8-8B4A-11D0-"
        "8D11-00A0C91BC942}\" Id=\"ab-CD\">\n"
        "    <BuildDependency Project=\"lib/lib.vcxproj\" />\n"
        "    <Build Solution=\"Release|*\" Project=\"false\" />\n"
        "  </Project>\n"
        "  <Folder Name=\"/tools/\">\n"
        "    <Project Path=\"lib\\lib.vcxproj\" DisplayName=\"The &amp; "
        "Lib\" />\n"
        "    <File Path=\"readme.md\" />\n"
        "  </Folder>\n"
        "</Solution>\n";

    struct d_msvs msvs;

    printf("slnx\n");

    if (d_tests_read(&msvs, text, D_MSVS_SOLUTION, "a .slnx"))
    {
        const struct d_msvs_project* const p = msvs.projects;

        d_tests_check( (msvs.project_count == 2u) &&
                       (strcmp(p[0].name, "app") == 0) &&
                       (strcmp(p[0].path, "app/app.vcxproj") == 0) &&
                       (strcmp(p[0].type,
                               "8bc9ceb8-8b4a-11d0-8d11-00a0c91bc942") == 0) &&
                       (strcmp(p[0].id, "ab-cd") == 0) &&
                       (strcmp(p[0].folder, "") == 0) && (p[0].line == 8u),
                       "a project at the top");
        d_tests_check( (strcmp(p[1].name, "The & Lib") == 0) &&
                       (strcmp(p[1].folder, "/tools/") == 0) &&
                       (strcmp(p[1].type, "") == 0) &&
                       (strcmp(p[1].id, "") == 0),
                       "a project in a folder, by its display name");
        d_tests_check( (p[0].dependency_count == 1u) &&
                       (strcmp(msvs.dependencies[p[0].first_dependency],
                               "lib/lib.vcxproj") == 0) &&
                       (p[1].dependency_count == 0u),
                       "a project's dependencies");
        d_tests_check( (msvs.configuration_count == 4u) &&
                       (strcmp(msvs.configurations[0], "Debug|x64") == 0) &&
                       (strcmp(msvs.configurations[3],
                               "Release|ARM64") == 0),
                       "each build type on each platform");
        d_tests_check(d_tests_platforms_alone(),
                      "platforms alone are built as Debug and Release");
        d_tests_check( (msvs.folder_count == 1u) &&
                       (strcmp(msvs.folders[0].path, "/tools/") == 0) &&
                       (d_msvs_is(&msvs,
                                  &msvs.elements[msvs.folders[0].element],
                                  "Folder")) &&
                       (d_tests_starts(&text[msvs.folders[0].start],
                                       "<Folder")) &&
                       (d_tests_starts(&text[msvs.folders[0].end], "\n"
                                       "</Solution>")) &&
                       (d_tests_starts(&text[p[0].start], "<Project ")) &&
                       (d_tests_starts(&text[p[0].end], "\n  <Folder")) &&
                       (msvs.elements[p[0].element].start == p[0].start) &&
                       (msvs.section_count == 0u) &&
                       (msvs.global_start == D_MSVS_NONE),
                       "the folder, and each record's element");
    }

    d_msvs_release(&msvs);

    return;
}


/*
d_tests_sln
  Tests the following:
  - a .sln's projects, its folders, nested, and its dependencies by path
  - its configurations
*/
static void
d_tests_sln(void)
{
    static const char text[] =
        "\r\n"
        "Microsoft Visual Studio Solution File, Format Version 12.00\r\n"
        "# Visual Studio Version 17\r\n"
        "VisualStudioVersion = 17.0.31903.59\r\n"
        "Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = \"app\", "
        "\"app\\app.vcxproj\", \"{11111111-0000-0000-0000-000000000001}\"\r\n"
        "\tProjectSection(ProjectDependencies) = postProject\r\n"
        "\t\t{22222222-0000-0000-0000-000000000002} = "
        "{22222222-0000-0000-0000-000000000002}\r\n"
        "\t\t{99999999-0000-0000-0000-000000000009} = "
        "{99999999-0000-0000-0000-000000000009}\r\n"
        "\tEndProjectSection\r\n"
        "EndProject\r\n"
        "Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = \"lib\", "
        "\"lib\\lib.vcxproj\", \"{22222222-0000-0000-0000-000000000002}\"\r\n"
        "EndProject\r\n"
        "Project(\"{2150E333-8FDC-42A3-9474-1A3956D46DE8}\") = \"tools\", "
        "\"tools\", \"{33333333-0000-0000-0000-000000000003}\"\r\n"
        "EndProject\r\n"
        "Project(\"{2150E333-8FDC-42A3-9474-1A3956D46DE8}\") = \"dawk\", "
        "\"dawk\", \"{44444444-0000-0000-0000-000000000004}\"\r\n"
        "EndProject\r\n"
        "Global\r\n"
        "\tGlobalSection(SolutionConfigurationPlatforms) = preSolution\r\n"
        "\t\tDebug|x64 = Debug|x64\r\n"
        "\t\tRelease|x64 = Release|x64\r\n"
        "\tEndGlobalSection\r\n"
        "\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\r\n"
        "\t\t{11111111-0000-0000-0000-000000000001}.Debug|x64.ActiveCfg = "
        "Debug|x64\r\n"
        "\tEndGlobalSection\r\n"
        "\tGlobalSection(NestedProjects) = preSolution\r\n"
        "\t\t{22222222-0000-0000-0000-000000000002} = "
        "{44444444-0000-0000-0000-000000000004}\r\n"
        "\t\t{44444444-0000-0000-0000-000000000004} = "
        "{33333333-0000-0000-0000-000000000003}\r\n"
        "\tEndGlobalSection\r\n"
        "EndGlobal\r\n";

    struct d_msvs msvs;

    printf("sln\n");

    if (d_tests_read(&msvs, text, D_MSVS_SOLUTION, "a .sln"))
    {
        const struct d_msvs_project* const p = msvs.projects;

        d_tests_check( (!msvs.xml) && (msvs.crlf) &&
                       (msvs.project_count == 2u) &&
                       (strcmp(p[0].name, "app") == 0) &&
                       (strcmp(p[0].path, "app\\app.vcxproj") == 0) &&
                       (strcmp(p[0].id,
                               "11111111-0000-0000-0000-000000000001") == 0) &&
                       (p[0].line == 5u),
                       "the projects, the folders not among them");
        d_tests_check( (strcmp(p[1].folder, "/tools/dawk/") == 0) &&
                       (strcmp(p[0].folder, "") == 0),
                       "a project in nested folders");
        d_tests_check( (p[0].dependency_count == 2u) &&
                       (strcmp(msvs.dependencies[p[0].first_dependency],
                               "lib\\lib.vcxproj") == 0) &&
                       (strcmp(msvs.dependencies[p[0].first_dependency + 1u],
                               "99999999-0000-0000-0000-000000000009") == 0),
                       "a dependency by its path, or its id when unknown");
        d_tests_check( (msvs.configuration_count == 2u) &&
                       (strcmp(msvs.configurations[1], "Release|x64") == 0),
                       "the configurations");
        d_tests_check( (msvs.folder_count == 2u) &&
                       (strcmp(msvs.folders[0].path, "/tools/") == 0) &&
                       (strcmp(msvs.folders[1].path, "/tools/dawk/") == 0) &&
                       (strcmp(msvs.folders[1].id,
                               "44444444-0000-0000-0000-000000000004") == 0),
                       "the folders, each by its whole path");
        d_tests_check( (d_tests_starts(&text[p[0].start], "Project(")) &&
                       (d_tests_starts(&text[p[0].end], "Project(")) &&
                       (d_tests_starts(&text[p[0].end - 12u],
                                       "EndProject\r\n")) &&
                       (d_tests_starts(&text[p[0].depends],
                                       "\tProjectSection(Project")) &&
                       (d_tests_starts(&text[p[0].depends_close],
                                       "\tEndProjectSection")) &&
                       (p[1].depends == D_MSVS_NONE) &&
                       (d_tests_starts(&text[msvs.folders[0].start],
                                       "Project(\"{2150")) &&
                       (d_tests_starts(&text[msvs.folders[0].end],
                                       "Project(\"{2150")),
                       "each record's lines");
        d_tests_check( (msvs.section_count == 3u) &&
                       (strcmp(msvs.sections[2].name,
                               "NestedProjects") == 0) &&
                       (d_tests_starts(&text[msvs.sections[0].start],
                                       "\tGlobalSection(Solution")) &&
                       (d_tests_starts(&text[msvs.sections[0].content],
                                       "\t\tDebug|x64")) &&
                       (d_tests_starts(&text[msvs.sections[0].close],
                                       "\tEndGlobalSection")) &&
                       (d_tests_starts(&text[msvs.sections[0].end],
                                       "\tGlobalSection(ProjectConfig")) &&
                       (d_tests_starts(&text[msvs.global_start],
                                       "Global\r\n")) &&
                       (d_tests_starts(&text[msvs.global_close],
                                       "EndGlobal")) &&
                       (msvs.global_end == (sizeof(text) - 1u)),
                       "the global sections, and Global's own lines");
    }

    d_msvs_release(&msvs);

    return;
}


/*
d_tests_writing
  Tests the following:
  - a GUID made from a name, as Python's uuid.uuid5 makes it in the same
    namespace
  - relative paths, climbing and descending, in any case and either
    separator
  - escaping, of text and of an attribute's value in either quotes
*/
static void
d_tests_writing(void)
{
    char guid[37];
    char seed[201];

    printf("writing\n");

    memset(seed, 'x', 200u);
    seed[200] = '\0';

    d_msvs_guid("", 0u, guid);
    d_tests_check(strcmp(guid, "4c95a43c-dcfd-5285-9e0e-3fa3ae515878") == 0,
                  "the GUID of no name");
    d_msvs_guid("abc", 3u, guid);
    d_tests_check(strcmp(guid, "c8a9f80b-5306-5530-997b-7ae52b9a8be3") == 0,
                  "the GUID of a short name");
    d_msvs_guid("testing/djinterp/test/test_common/test_common_tests", 51u,
                guid);
    d_tests_check(strcmp(guid, "2750c52d-2579-5e77-b3a5-7342b2b98b46") == 0,
                  "the GUID of a project's name");
    d_msvs_guid(seed, 200u, guid);
    d_tests_check(strcmp(guid, "0723f7e3-5650-57b9-aaba-031e7c2acded") == 0,
                  "the GUID of a name longer than a block");

    struct
    {
        const char* directory;
        const char* path;
        char        separator;
        const char* expected;
    } const relatives[] =
    {
        { ".config/.msvs/testing/djinterp/test",
          ".config/.msvs/djinterp.props", '\\',
          "..\\..\\..\\djinterp.props" },
        { ".config/.msvs/testing/djinterp/test", "tests/djinterp/test/a.cpp",
          '\\', "..\\..\\..\\..\\..\\tests\\djinterp\\test\\a.cpp" },
        { "a/b", "a/b/c/d.vcxproj", '/', "c/d.vcxproj" },
        { "A\\B", "a/b", '/', "." },
        { "./a/./b/", "a/c/../b/x", '\\', "x" },
        { "", "src/x.c", '\\', "src\\x.c" },
        { "/root/a", "/root/b/c", '/', "../b/c" }
    };

    // each path, from its directory
    for (size_t at = 0; at < (sizeof(relatives) / sizeof(relatives[0]));
         ++at)
    {
        char* const relative = d_msvs_relative(relatives[at].directory,
                                               relatives[at].path,
                                               relatives[at].separator);

        d_tests_check( (relative) &&
                       (strcmp(relative, relatives[at].expected) == 0),
                       relatives[at].expected);
        free(relative);
    }

    d_tests_check(d_msvs_relative("../a", "b", '/') == NULL,
                  "a directory above where the two begin is refused");

    char         out[64];
    const size_t text  = d_msvs_escape("a<b&c>\"d'\t\r\n", 12u, '\0', out);

    d_tests_check( (strcmp(out, "a&lt;b&amp;c&gt;\"d'\t&#13;\n") == 0) &&
                   (text == strlen(out)),
                   "text escaped");
    (void)d_msvs_escape("a<b&c>\"d'\t\r\n", 12u, '"', out);
    d_tests_check(strcmp(out, "a&lt;b&amp;c&gt;&quot;d'&#9;&#13;&#10;") == 0,
                  "a value in double quotes escaped");
    (void)d_msvs_escape("a<b&c>\"d'\t\r\n", 12u, '\'', out);
    d_tests_check(strcmp(out, "a&lt;b&amp;c&gt;\"d&apos;&#9;&#13;&#10;") == 0,
                  "a value in single quotes escaped");

    return;
}


/*
d_tests_conditions
  Tests the following:
  - the configuration tests Visual Studio and CMake write, either way round
  - other conditions, and none
  - matching a tested configuration against one named
*/
static void
d_tests_conditions(void)
{
    char tested[64];

    printf("conditions\n");

    d_tests_check( (d_msvs_condition_of("'$(Configuration)|$(Platform)'=="
                                        "'Debug|x64'", tested,
                                        sizeof(tested))
                    == D_MSVS_CONFIGURATION) &&
                   (strcmp(tested, "Debug|x64") == 0),
                   "what CMake writes");
    d_tests_check( (d_msvs_condition_of(" '$(Configuration)|$(Platform)' == "
                                        "'Release|Win32' ", tested,
                                        sizeof(tested))
                    == D_MSVS_CONFIGURATION) &&
                   (strcmp(tested, "Release|Win32") == 0),
                   "what Visual Studio writes, spaced");
    d_tests_check( (d_msvs_condition_of("'Debug'=='$(Configuration)'",
                                        tested, sizeof(tested))
                    == D_MSVS_CONFIGURATION) &&
                   (strcmp(tested, "Debug|*") == 0) &&
                   (d_msvs_condition_of("'$(platform)'=='x64'", tested,
                                        sizeof(tested))
                    == D_MSVS_CONFIGURATION) &&
                   (strcmp(tested, "*|x64") == 0),
                   "one tested alone, either way round, in any case");
    d_tests_check( (d_msvs_condition_of("exists('a.props')", NULL, 0u)
                    == D_MSVS_UNKNOWN) &&
                   (d_msvs_condition_of("'$(Configuration)'!='Debug'", NULL,
                                        0u) == D_MSVS_UNKNOWN) &&
                   (d_msvs_condition_of("'$(Configuration)'=='$(X)'", NULL,
                                        0u) == D_MSVS_UNKNOWN) &&
                   (d_msvs_condition_of("'$(A)'=='b'", NULL, 0u)
                    == D_MSVS_UNKNOWN),
                   "any other condition");
    d_tests_check( (d_msvs_condition_of(NULL, NULL, 0u) == D_MSVS_ALWAYS) &&
                   (d_msvs_condition_of("  ", NULL, 0u) == D_MSVS_ALWAYS),
                   "no condition, or one of space");
    d_tests_check( d_msvs_config_matches("Debug|x64", "debug|X64") &&
                   d_msvs_config_matches("Debug|*", "Debug|ARM64") &&
                   d_msvs_config_matches("*|x64", "Release|x64") &&
                   d_msvs_config_matches("Debug|x64", "Debug") &&
                   !d_msvs_config_matches("Debug|x64", "Release|x64") &&
                   !d_msvs_config_matches("Debug|x64", "Debug|Win32"),
                   "a tested configuration against one named");

    return;
}


/*
d_tests_refusals
  Tests the following:
  - each refusal, with the line and column it is reported at
*/
static void
d_tests_refusals(void)
{
    // d_tests_refusal
    //   struct: a text the reader must refuse, its kind, where, and why.
    struct d_tests_refusal
    {
        const char*      text;
        enum d_msvs_kind kind;
        size_t           line;
        size_t           column;
        const char*      words;
    };

    static const struct d_tests_refusal refusals[] =
    {
        { "<Project>\n  <A></B>\n</Project>", D_MSVS_PROJECT, 2u, 6u,
          "</B> ends <A>" },
        { "<Project>\n  <A>\n", D_MSVS_PROJECT, 2u, 3u, "never closed" },
        { "<Project/>\n<Project/>", D_MSVS_PROJECT, 2u, 1u,
          "second root" },
        { "<Project/>\nstray", D_MSVS_PROJECT, 2u, 1u,
          "outside the root" },
        { "<Project a=\"1\" b>\n</Project>", D_MSVS_PROJECT, 1u, 16u,
          "not name=\"value\"" },
        { "<Project a=\"1\" a='2'/>", D_MSVS_PROJECT, 1u, 16u,
          "given twice" },
        { "<Project a=\"1<\"/>", D_MSVS_PROJECT, 1u, 14u,
          "a < in an attribute" },
        { "<Project a=\"&b\"/>", D_MSVS_PROJECT, 1u, 13u,
          "begins no reference" },
        { "<Project a=\"1\"b=\"2\"/>", D_MSVS_PROJECT, 1u, 15u,
          "not set apart" },
        { "<Project>\n  a &x b\n</Project>", D_MSVS_PROJECT, 2u, 5u,
          "begins no reference" },
        { "<Project>&#0;</Project>", D_MSVS_PROJECT, 1u, 10u,
          "begins no reference" },
        { "<Project>]]></Project>", D_MSVS_PROJECT, 1u, 10u,
          "]]> outside" },
        { "<Project><1a/></Project>", D_MSVS_PROJECT, 1u, 10u,
          "not an XML name" },
        { "<Project></Project x>", D_MSVS_PROJECT, 1u, 10u,
          "more than its name" },
        { "<Project><!-- a -- b --></Project>", D_MSVS_PROJECT, 1u, 17u,
          "holding --" },
        { "\n<?xml version=\"1.0\"?><Project/>", D_MSVS_PROJECT, 2u, 1u,
          "not first" },
        { "<?xml encoding=\"utf-8\"?><Project/>", D_MSVS_PROJECT, 1u, 6u,
          "declaration that is not well formed" },
        { "<?xml version=\"1.0\" encoding=\"latin1\"?><Project/>",
          D_MSVS_PROJECT, 1u, 31u, "other than UTF-8" },
        { "<?xml version=\"2.0\"?><Project/>", D_MSVS_PROJECT, 1u, 16u,
          "declaration that is not well formed" },
        { "<?-pi?><Project/>", D_MSVS_PROJECT, 1u, 3u,
          "target is not an XML name" },
        { "<Project>\x01</Project>", D_MSVS_PROJECT, 1u, 10u,
          "control character" },
        { "<Project>\xC3(</Project>", D_MSVS_PROJECT, 1u, 10u,
          "not UTF-8" },
        { "<Project>\n  <A x=\"1></Project>", D_MSVS_PROJECT, 2u, 3u,
          "not well formed" },
        { "<Solution/>", D_MSVS_PROJECT, 1u, 1u, "must be <Project>" },
        { "<Project/>", D_MSVS_SOLUTION, 1u, 1u, "must be <Solution>" },
        { "plain text", D_MSVS_PROJECT, 1u, 1u, "must be XML" },
        { "\nplain text", D_MSVS_SOLUTION, 2u, 1u,
          "not a Visual Studio solution" },
        { "Microsoft Visual Studio Solution File, Format Version 12.00\n"
          "Project(\"{X}\") = \"a\"\n", D_MSVS_SOLUTION, 2u, 1u,
          "a Project line" },
        { "Microsoft Visual Studio Solution File, Format Version 12.00\n"
          "Project(\"{X}\") = \"a\", \"a.vcxproj\", \"{Y}\"\n",
          D_MSVS_SOLUTION, 2u, 1u, "never ended" },
        { "", D_MSVS_PROJECT, 1u, 1u, "must be XML" },
        { "  \n", D_MSVS_OTHER, 1u, 1u, "not a Visual Studio solution" }
    };

    printf("refusals\n");

    for (size_t at = 0; at < (sizeof(refusals) / sizeof(refusals[0])); ++at)
    {
        struct d_msvs msvs;
        char          why[256];
        char          what[320];
        const bool    read = d_msvs_read(&msvs, refusals[at].text,
                                         strlen(refusals[at].text),
                                         refusals[at].kind, why,
                                         sizeof(why));

        (void)snprintf(what, sizeof(what), "refused at %lu:%lu with \"%s\"",
                       (unsigned long)refusals[at].line,
                       (unsigned long)refusals[at].column,
                       refusals[at].words);
        d_tests_check( (!read) &&
                       (msvs.error_line == refusals[at].line) &&
                       (msvs.error_column == refusals[at].column) &&
                       (strstr(why, refusals[at].words) != NULL),
                       what);

        // what came instead is shown, for a reader of the failure
        if ( (read) ||
             (msvs.error_line != refusals[at].line) ||
             (msvs.error_column != refusals[at].column) ||
             (!strstr(why, refusals[at].words)) )
        {
            printf("        got %s\n", read ? "no refusal" : why);
        }

        d_msvs_release(&msvs);
    }

    return;
}


// d_tests_corpus
//   struct: what a corpus run counted.
struct d_tests_corpus
{
    size_t files;
    size_t kinds[6];
    size_t elements;
    size_t projects;
    size_t configurations;
    size_t failed;
};


/*
d_tests_sound_solution
  Proves a solution's records sound: each project's and folder's record its
element, or lines from Project( to past EndProject; each section's lines
from GlobalSection( to past EndGlobalSection, within Global.  Returns why
not, or NULL.
*/
static const char*
d_tests_sound_solution(
    const struct d_msvs* _msvs,
    size_t*              _where
)
{
    const char* const text = _msvs->text;

    // every project's record and every folder's
    for (size_t which = 0;
         which < (_msvs->project_count + _msvs->folder_count); ++which)
    {
        const bool   project = (which < _msvs->project_count);
        const size_t at      = (project) ? which
                                         : (which - _msvs->project_count);
        const size_t start   = (project) ? _msvs->projects[at].start
                                         : _msvs->folders[at].start;
        const size_t end     = (project) ? _msvs->projects[at].end
                                         : _msvs->folders[at].end;
        const size_t element = (project) ? _msvs->projects[at].element
                                         : _msvs->folders[at].element;

        *_where = start;

        // an element's record is the element
        if ( (_msvs->xml) &&
             ( (element >= _msvs->element_count) ||
               (_msvs->elements[element].start != start) ||
               (_msvs->elements[element].end != end) ) )
        {
            return "a record is not its element";
        }

        // a .sln's is its lines
        if ( (!_msvs->xml) &&
             ( (strncmp(&text[start], "Project(", 8u) != 0) ||
               (end > _msvs->length) ||
               (text[end - 1u] != '\n') ||
               (end < 12u) ||
               ( (strncmp(&text[end - 11u], "EndProject\n", 11u) != 0) &&
                 (strncmp(&text[end - 12u], "EndProject\r\n", 12u)
                  != 0) ) ) )
        {
            return "a record is not its lines";
        }
    }

    // every section's lines
    for (size_t which = 0; which < _msvs->section_count; ++which)
    {
        const struct d_msvs_section* const section = &_msvs->sections[which];

        *_where = section->start;

        // a section is its lines, and within Global
        if ( (section->start < _msvs->global_start) ||
             (section->end > _msvs->global_close) ||
             (strncmp(&text[section->start + strspn(&text[section->start],
                                                    " \t")],
                      "GlobalSection(", 14u) != 0) ||
             (strncmp(&text[section->close + strspn(&text[section->close],
                                                    " \t")],
                      "EndGlobalSection", 16u) != 0) ||
             (section->content > section->close) )
        {
            return "a section is not its lines";
        }
    }

    return NULL;
}


/*
d_tests_sound
  Proves a reading sound: every element within its parent, its tags at its
places, its end tag repeating its name, its children in order, its line
the one its offset gives.  Returns why not, or NULL.
*/
static const char*
d_tests_sound(
    const struct d_msvs* _msvs,
    size_t*              _where
)
{
    const char* const text = _msvs->text;
    size_t            line = 1;
    size_t            at   = 0;

    // every element, in document order
    for (size_t which = 0; which < _msvs->element_count; ++which)
    {
        const struct d_msvs_element* const e = &_msvs->elements[which];

        *_where = e->start;

        // the start tag is `<` and the name, at its place
        if ( (text[e->start] != '<') ||
             (e->name != (e->start + 1u)) ||
             (e->content_start > e->content_end) ||
             (e->content_end > e->end) ||
             (e->end > _msvs->length) ||
             (text[e->end - 1u] != '>') )
        {
            return "an element's tags are not where its places say";
        }

        // an end tag repeats the name; an empty element has none
        if ( (!e->empty) &&
             ( (text[e->content_end] != '<') ||
               (text[e->content_end + 1u] != '/') ||
               (memcmp(&text[e->content_end + 2u], &text[e->name],
                       e->name_length) != 0) ) )
        {
            return "an end tag does not repeat its element's name";
        }

        // within the parent's content, after the sibling before it
        if ( (e->parent != D_MSVS_NONE) &&
             ( (e->start < _msvs->elements[e->parent].content_start) ||
               (e->end > _msvs->elements[e->parent].content_end) ||
               (e->depth != (_msvs->elements[e->parent].depth + 1u)) ) )
        {
            return "an element is outside its parent";
        }

        if ( (e->next_sibling != D_MSVS_NONE) &&
             (_msvs->elements[e->next_sibling].start < e->end) )
        {
            return "siblings out of order";
        }

        // the line its offset gives
        while (at < e->start)
        {
            line += (text[at++] == '\n') ? 1u : 0u;
        }

        if (line != e->line)
        {
            return "an element's line is not the one its offset gives";
        }
    }

    return d_tests_sound_solution(_msvs, _where);
}


/*
d_tests_corpus_file
  Reads one file and proves the reading sound.
*/
static void
d_tests_corpus_file(
    const char*            _path,
    struct d_tests_corpus* _counts
)
{
    size_t                 length = 0;
    char* const            text   = d_text_read(_path, &length);
    const enum d_msvs_kind kind   = d_msvs_kind_of(_path);
    char                   why[256];

    _counts->files++;
    _counts->kinds[kind]++;

    // a file that cannot be read is a failure of its own
    if (!text)
    {
        printf("  FAIL  %s: cannot be read\n", _path);
        _counts->failed++;

        return;
    }

    struct d_msvs msvs;
    size_t        where   = 0;
    const bool    read    = d_msvs_read(&msvs, text, length, kind, why,
                                        sizeof(why));
    const char*   problem = (read) ? d_tests_sound(&msvs, &where) : NULL;

    _counts->elements       += msvs.element_count;
    _counts->projects       += msvs.project_count;
    _counts->configurations += msvs.configuration_count;

    // a refusal, or a reading that is not sound, fails the file
    if (!read)
    {
        printf("  FAIL  %s: %s\n", _path, why);
        _counts->failed++;
    }
    else if (problem)
    {
        printf("  FAIL  %s: %s at byte %lu\n", _path, problem,
               (unsigned long)where);
        _counts->failed++;
    }

    d_msvs_release(&msvs);
    free(text);

    return;
}


/*
d_tests_corpus_walk
  Every Visual Studio file under a directory, in name order.
*/
static void
d_tests_corpus_walk(
    const char*            _directory,
    struct d_tests_corpus* _counts
)
{
    char** names = NULL;
    size_t count = 0;

    // a directory that cannot be listed holds nothing to read
    if (!d_fs_names(_directory, &names, &count))
    {
        printf("  FAIL  %s: cannot be listed\n", _directory);
        _counts->failed++;

        return;
    }

    // each name the directory holds, in name order
    for (size_t at = 0; at < count; ++at)
    {
        const size_t span = strlen(_directory) + strlen(names[at]) + 2u;
        char* const  path = malloc(span);

        if (!path)
        {
            continue;
        }

        (void)snprintf(path, span, "%s/%s", _directory, names[at]);

        // a directory is walked, and a Visual Studio file read
        if (d_fs_kind_of(path) == D_FS_KIND_DIRECTORY)
        {
            d_tests_corpus_walk(path, _counts);
        }
        else if (d_msvs_kind_of(names[at]) != D_MSVS_OTHER)
        {
            d_tests_corpus_file(path, _counts);
        }

        free(path);
    }

    d_fs_names_free(names, count);

    return;
}


/*
d_tests_print_escaped
  Prints a value as xml_tree.py does: a backslash, a quote, a tab, a CR and
an LF escaped, and nothing else.
*/
static void
d_tests_print_escaped(
    const char* _text
)
{
    for (const char* at = _text; *at != '\0'; ++at)
    {
        // the five escaped, the rest as they are
        switch (*at)
        {
            case '\\': printf("\\\\"); break;
            case '"':  printf("\\\""); break;
            case '\t': printf("\\t");  break;
            case '\r': printf("\\r");  break;
            case '\n': printf("\\n");  break;
            default:   (void)putchar(*at); break;
        }
    }

    return;
}


// d_tests_attribute
//   struct: one attribute, decoded, for printing in name order.
struct d_tests_attribute
{
    char* name;
    char* value;
};


/*
d_tests_compare_attributes
  Name order, by bytes.
*/
static int
d_tests_compare_attributes(
    const void* _left,
    const void* _right
)
{
    return strcmp(((const struct d_tests_attribute*)_left)->name,
                  ((const struct d_tests_attribute*)_right)->name);
}


/*
d_tests_dump
  Prints a file's elements, one to a line: its depth, its local name, its
attributes in name order but for namespace declarations, and, for one
holding no element, its text.
*/
static int
d_tests_dump(
    const char* _path
)
{
    size_t      length = 0;
    char* const text   = d_text_read(_path, &length);
    char        why[256];

    // a file that cannot be read has nothing to print
    if (!text)
    {
        printf("cannot read %s\n", _path);

        return 1;
    }

    struct d_msvs msvs;
    const bool    read = d_msvs_read(&msvs, text, length,
                                     d_msvs_kind_of(_path), why,
                                     sizeof(why));

    // each element of the tree, as Python's printer prints it
    for (size_t at = 0; (read) && (at < msvs.element_count); ++at)
    {
        const struct d_msvs_element* const e = &msvs.elements[at];
        const char*                        name = &text[e->name];
        size_t                             span = e->name_length;
        const char* const colon = memchr(name, ':', span);

        // the local name, as Python's parser gives it
        if (colon)
        {
            span -= (size_t)(colon - name) + 1u;
            name  = colon + 1;
        }

        printf("%lu %.*s", (unsigned long)e->depth, (int)span, name);

        struct d_tests_attribute attributes[64];
        size_t                   count = 0;
        const char*              raw   = &text[e->attributes];
        size_t                   walk  = 0;

        // every attribute, by walking them as name="value"
        while ( (walk < e->attributes_length) &&
                (count < 64u) )
        {
            while ( (walk < e->attributes_length) &&
                    ( (raw[walk] == ' ') || (raw[walk] == '\t') ||
                      (raw[walk] == '\r') || (raw[walk] == '\n') ) )
            {
                walk++;
            }

            const size_t from = walk;

            while ( (walk < e->attributes_length) &&
                    (raw[walk] != '=') && (raw[walk] != ' ') &&
                    (raw[walk] != '\t') && (raw[walk] != '\r') &&
                    (raw[walk] != '\n') )
            {
                walk++;
            }

            if (walk == from)
            {
                break;
            }

            char* const attribute = malloc((walk - from) + 1u);
            size_t      start     = 0;
            size_t      value_length = 0;

            memcpy(attribute, &raw[from], walk - from);
            attribute[walk - from] = '\0';

            (void)d_msvs_attribute(&msvs, e, attribute, &start,
                                   &value_length);

            char* const value = malloc(value_length + 1u);

            (void)d_msvs_value(&msvs, start, value_length, value);

            attributes[count].name  = attribute;
            attributes[count].value = value;
            count++;

            walk = (start + value_length + 1u) - e->attributes;
        }

        qsort(attributes, count, sizeof(attributes[0]),
              d_tests_compare_attributes);

        // its attributes in name order, as Python gives them
        for (size_t which = 0; which < count; ++which)
        {
            // namespace declarations are no attribute to Python
            if ( (strcmp(attributes[which].name, "xmlns") != 0) &&
                 (strncmp(attributes[which].name, "xmlns:", 6u) != 0) )
            {
                printf(" %s=\"", attributes[which].name);
                d_tests_print_escaped(attributes[which].value);
                printf("\"");
            }

            free(attributes[which].name);
            free(attributes[which].value);
        }

        // the text of an element holding no element
        if (e->child_count == 0u)
        {
            char* const value = malloc((e->content_end - e->content_start)
                                       + 1u);

            (void)d_msvs_text(&msvs, e, value);
            printf(" text=\"");
            d_tests_print_escaped(value);
            printf("\"");
            free(value);
        }

        printf("\n");
    }

    // a refusal is reported in place of the tree
    if (!read)
    {
        printf("refused: %s\n", why);
    }

    d_msvs_release(&msvs);
    free(text);

    return read ? 0 : 1;
}


/*
main
  The unit checks; with --corpus every Visual Studio file under the
directories, and with --dump a file's element tree.
*/
int
main(
    int    argc,
    char** argv
)
{
    // a corpus run reads files and reports them, and nothing else
    if ( (argc > 2) &&
         (strcmp(argv[1], "--corpus") == 0) )
    {
        struct d_tests_corpus counts;

        memset(&counts, 0, sizeof(counts));

        // every directory named
        for (int at = 2; at < argc; ++at)
        {
            d_tests_corpus_walk(argv[at], &counts);
        }

        printf("\n%lu files (%lu solutions, %lu projects, %lu filters, "
               "%lu user, %lu props), %lu elements, %lu projects listed, "
               "%lu configurations: %lu failed\n",
               (unsigned long)counts.files,
               (unsigned long)counts.kinds[D_MSVS_SOLUTION],
               (unsigned long)counts.kinds[D_MSVS_PROJECT],
               (unsigned long)counts.kinds[D_MSVS_FILTERS],
               (unsigned long)counts.kinds[D_MSVS_USER],
               (unsigned long)counts.kinds[D_MSVS_PROPS],
               (unsigned long)counts.elements,
               (unsigned long)counts.projects,
               (unsigned long)counts.configurations,
               (unsigned long)counts.failed);

        return ( (counts.failed == 0u) && (counts.files > 0u) ) ? 0 : 1;
    }

    // a dump prints a tree, for comparing with Python's
    if ( (argc == 3) &&
         (strcmp(argv[1], "--dump") == 0) )
    {
        return d_tests_dump(argv[2]);
    }

    d_tests_kinds();
    d_tests_tree();
    d_tests_slnx();
    d_tests_sln();
    d_tests_writing();
    d_tests_conditions();
    d_tests_refusals();

    printf("\n%d checks, %d failed\n", g_checks, g_failed);

    return (g_failed == 0) ? 0 : 1;
}
