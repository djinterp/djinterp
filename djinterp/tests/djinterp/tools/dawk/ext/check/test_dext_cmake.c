/*******************************************************************************
* djinterp [tools]                                             test_dext_cmake.c
*
* Exercises `dext_cmake.h` and what the checker reads of a CMake file beside
* it: the tree a sheet selects -- commands, arguments, comments, blocks, a
* refusal -- by the selectors a sheet would write, the geometry a layout
* property compares, the banner and the delimiters in `#` comments, and the
* facts a file under build/cmake carries.  A node that gains an attribute
* after another node has one keeps every attribute it had, which the file
* node depends on once the CMake tree marks it.
*
*   test_dext_cmake [directory]    the directory holds the files it writes
*
* path:      /tests/djinterp/tools/dawk/ext/check/test_dext_cmake.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/check/dext_cmake.h"  // corresponding header
// std
#include <stdio.h>   // printf, fopen, fputs, fclose, snprintf
#include <string.h>  // strcmp, strlen, strstr
// djinterp
#include "../../../../../../inc/djinterp/tools/dawk/dbanner.h"   // d_banner_build
#include "../../../../../../inc/djinterp/tools/dawk/dfs.h"       // d_fs_kind_of
#include "../../../../../../inc/djinterp/tools/dawk/dmatch.h"    // d_match_rule
#include "../../../../../../inc/djinterp/tools/dawk/dsection.h"  // d_section_build_marked
#include "../../../../../../inc/djinterp/tools/dawk/dss.h"       // d_dss_parse
// the one directory call the test needs, which ISO C does not have
#if defined(_WIN32)
    #include <direct.h>    // _mkdir
    #define D_TESTS_MKDIR(_path) _mkdir(_path)
#else
    #include <sys/stat.h>  // mkdir
    #define D_TESTS_MKDIR(_path) mkdir((_path), 0755)
#endif


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
d_tests_attribute
  A node's attribute by name: its text, "" for one with no value, or NULL
for one it does not have.
*/
static const char*
d_tests_attribute(
    struct d_node_tree* _tree,
    uint32_t            _node,
    const char*         _name
)
{
    const struct d_node* const node = d_node_at(_tree, _node);

    // a node that is not there has nothing
    if (!node)
    {
        return NULL;
    }

    for (uint32_t at = 0; at < node->attribute_count; ++at)
    {
        const struct d_node_attribute* const attribute =
            d_node_attribute_at(_tree, node->first_attribute + at);

        if ( (attribute) &&
             (strcmp(d_node_name(_tree, attribute->name), _name) == 0) )
        {
            return (attribute->value == D_DSS_NO_INDEX)
                 ? ""
                 : d_node_text(_tree, attribute->value);
        }
    }

    return NULL;
}


/*
d_tests_is
  Whether a node's attribute holds a text.
*/
static bool
d_tests_is(
    struct d_node_tree* _tree,
    uint32_t            _node,
    const char*         _name,
    const char*         _value
)
{
    const char* const value = d_tests_attribute(_tree, _node, _name);

    return ( (value) &&
             (strcmp(value, _value) == 0) );
}


/*
d_tests_find
  The `_nth` node, from 0, of a type.
*/
static uint32_t
d_tests_find(
    struct d_node_tree* _tree,
    const char*         _type,
    size_t              _nth
)
{
    size_t seen = 0;

    for (uint32_t at = 0; at < (uint32_t)d_node_count(_tree); ++at)
    {
        if ( (strcmp(d_node_name(_tree, d_node_at(_tree, at)->type),
                     _type) == 0) &&
             (seen++ == _nth) )
        {
            return at;
        }
    }

    return D_DSS_NO_INDEX;
}


/*
d_tests_matches
  How many nodes of a CMake text's tree a one-rule sheet matches.
*/
static size_t
d_tests_matches(
    const char* _rule,
    const char* _text
)
{
    struct d_dss_error        error;
    struct d_dss_sheet* const sheet = d_dss_parse(_rule, strlen(_rule),
                                                  &error);
    struct d_node_tree* const tree  = d_node_tree_new();
    size_t                    hits  = 0;

    // a sheet that does not parse matches nothing, and says so
    if ( (!sheet) ||
         (!tree) )
    {
        printf("  sheet did not parse: %s\n", _rule);
        d_dss_free(sheet);
        d_node_tree_free(tree);

        return (size_t)-1;
    }

    (void)d_ext_cmake_build(tree, D_DSS_NO_INDEX, _text, strlen(_text),
                            "CMakeLists.txt", NULL);

    for (uint32_t node = 0; node < (uint32_t)d_node_count(tree); ++node)
    {
        for (size_t rule = 0; rule < d_dss_rule_count(sheet); ++rule)
        {
            if (d_match_rule(sheet, tree, node,
                             d_dss_rule_at(sheet, (uint32_t)rule)))
            {
                hits++;
            }
        }
    }

    d_node_tree_free(tree);
    d_dss_free(sheet);

    return hits;
}


/*
d_tests_claims
  Tests the following:
  - CMake's file names are claimed, and nothing else
*/
static void
d_tests_claims(void)
{
    printf("claims\n");

    d_tests_check(d_ext_cmake_accepts("CMakeLists.txt") &&
                  d_ext_cmake_accepts("a/b/CMakeLists.txt") &&
                  d_ext_cmake_accepts("a\\b\\run.cmake") &&
                  d_ext_cmake_accepts("config.cmake.in"),
                  "CMakeLists.txt, *.cmake and *.cmake.in are CMake's");
    d_tests_check(!d_ext_cmake_accepts("notes.txt") &&
                  !d_ext_cmake_accepts("a.cmake.bak") &&
                  !d_ext_cmake_accepts(".cmake") &&
                  !d_ext_cmake_accepts("CMakeLists.txt/x.c") &&
                  !d_ext_cmake_accepts(NULL),
                  "...and nothing else is");

    return;
}


// the text most checks below read: a function holding an if with both
// branches, comments in every place, and an argument of every kind
static const char LEAF[] =
    "# a leaf\n"                                          // 1
    "cmake_minimum_required(VERSION 3.16)\n"              // 2
    "\n"                                                  // 3
    "function(add_leaf _name)\n"                          // 4
    "    # inside the function\n"                         // 5
    "    IF(WIN32 AND (NOT MINGW))\n"                     // 6
    "        set(_flags \"/W4 /utf-8\") # trailing\n"     // 7
    "    elseif(UNIX)\n"                                  // 8
    "        Set(_flags -Wall [[-Wextra]] -Da=\"b c\")\n" // 9
    "    else()\n"                                        // 10
    "    endif()\n"                                       // 11
    "    add_executable(${_name}\n"                       // 12
    "        main.c   # the program\n"                    // 13
    "        util.c)\n"                                   // 14
    "endfunction()\n";                                    // 15


/*
d_tests_selectors
  Tests the following:
  - each node type and attribute a sheet would select by
  - blocks hold their opener first, their closer last, their branches and
    their body between
  - comments stand beneath what holds them
*/
static void
d_tests_selectors(void)
{
    printf("selectors\n");

    d_tests_check(d_tests_matches("file[lang=cmake] { a: b; }", LEAF) == 1u,
                  "the file is CMake");
    d_tests_check(d_tests_matches("command { a: b; }", LEAF) == 10u,
                  "ten commands");
    d_tests_check(d_tests_matches("command[name=set] { a: b; }", LEAF) == 2u,
                  "a name is compared in lower case");
    d_tests_check( (d_tests_matches("command[case=upper] { a: b; }",
                                    LEAF) == 1u) &&
                   (d_tests_matches("command[case=mixed] { a: b; }",
                                    LEAF) == 1u),
                   "...and its case is a fact of its own");
    d_tests_check(d_tests_matches("block[kind=function][name=add_leaf] "
                                  "> block[kind=if] { a: b; }", LEAF) == 1u,
                  "a block within a block, a function named");
    d_tests_check( (d_tests_matches("block[kind=if] > command:first-child"
                                    "[role=open] { a: b; }", LEAF) == 1u) &&
                   (d_tests_matches("block[kind=if] > command:last-child"
                                    "[name=endif] { a: b; }", LEAF) == 1u) &&
                   (d_tests_matches("block[kind=if] > command[role=branch] "
                                    "{ a: b; }", LEAF) == 2u),
                   "a block's opener, closer and branches");
    d_tests_check(d_tests_matches("block[kind=if] > command[name=set] "
                                  "{ a: b; }", LEAF) == 2u,
                  "a body stands beside its branches");
    d_tests_check( (d_tests_matches("command[depth=\"2\"] { a: b; }",
                                    LEAF) == 2u) &&
                   (d_tests_matches("command[name=add_executable]"
                                    "[depth=\"1\"][arguments=\"3\"]"
                                    "[lines=\"3\"] { a: b; }", LEAF) == 1u),
                   "depth, argument count and line span");
    d_tests_check( (d_tests_matches("argument[kind=quoted]"
                                    "[value=\"/W4 /utf-8\"] { a: b; }",
                                    LEAF) == 1u) &&
                   (d_tests_matches("argument[kind=bracket][value=\"-Wextra\"] "
                                    "{ a: b; }", LEAF) == 1u) &&
                   (d_tests_matches("argument[legacy] { a: b; }",
                                    LEAF) == 1u) &&
                   (d_tests_matches("argument[paren] { a: b; }",
                                    LEAF) == 2u),
                   "an argument's kind, value and forms");
    d_tests_check( (d_tests_matches("file > comment { a: b; }",
                                    LEAF) == 1u) &&
                   (d_tests_matches("block > comment:not([trailing]) "
                                    "{ a: b; }", LEAF) == 1u) &&
                   (d_tests_matches("block > comment[trailing] { a: b; }",
                                    LEAF) == 1u) &&
                   (d_tests_matches("command > comment[trailing] "
                                    "{ a: b; }", LEAF) == 1u),
                   "comments beneath the file, a block, and a command");
    d_tests_check(d_tests_matches("command[unpaired], block[unclosed], "
                                  "refusal { a: b; }", LEAF) == 0u,
                  "a sound file has nothing unpaired, unclosed or refused");

    return;
}


/*
d_tests_broken
  Tests the following:
  - an unpaired closer, and a block never closed
  - a refusal, where it stands, and the commands before it
  - a generated file is marked, and given no tree
*/
static void
d_tests_broken(void)
{
    printf("broken\n");

    d_tests_check( (d_tests_matches("command[unpaired] { a: b; }",
                                    "endif()\nfunction(f)\n") == 2u) &&
                   (d_tests_matches("block[unclosed] { a: b; }",
                                    "endif()\nfunction(f)\n") == 1u),
                   "an unpaired closer, and a block never closed");
    d_tests_check( (d_tests_matches("block[kind=foreach]:not([unclosed]) "
                                    "> command[name=endforeach] { a: b; }",
                                    "foreach(x a)\nif(y)\nendforeach()\n")
                    == 1u) &&
                   (d_tests_matches("block[kind=if][unclosed] { a: b; }",
                                    "foreach(x a)\nif(y)\nendforeach()\n")
                    == 1u),
                   "a closer passing over a block closes its own");

    struct d_node_tree* const tree = d_node_tree_new();
    struct d_ext_cmake_stats  stats;
    static const char         text[] = "set(a)\nset(b \"c\n";

    (void)d_ext_cmake_build(tree, D_DSS_NO_INDEX, text, sizeof(text) - 1u,
                            NULL, &stats);

    const uint32_t refusal = d_tests_find(tree, "refusal", 0u);

    d_tests_check( (stats.refused) && (stats.commands == 1u) &&
                   (refusal != D_DSS_NO_INDEX) &&
                   (d_node_at(tree, refusal)->line == 2u) &&
                   (d_node_at(tree, refusal)->start_column == 7u) &&
                   (strstr(d_node_text(tree, d_node_at(tree, refusal)->text),
                           "never closed") != NULL),
                   "a refusal says where and why, after what came before");
    d_node_tree_free(tree);

    struct d_node_tree* const generated = d_node_tree_new();
    static const char         install[] =
        "# Install script for directory: D:/x\nset(a)\n";

    const uint32_t file = d_ext_cmake_build(generated, D_DSS_NO_INDEX,
                                            install, sizeof(install) - 1u,
                                            NULL, &stats);

    d_tests_check( (stats.generated) &&
                   (d_tests_attribute(generated, file, "generated")) &&
                   (d_tests_is(generated, file, "lang", "cmake")) &&
                   (d_tests_find(generated, "command", 0u)
                    == D_DSS_NO_INDEX),
                   "a generated file is marked, and given no tree");
    d_node_tree_free(generated);

    struct d_node_tree* const listed = d_node_tree_new();

    (void)d_ext_cmake_build(listed, D_DSS_NO_INDEX, "set(a)\n", 7u,
                            "out/CMakeFiles/3.28/CMakeCCompiler.cmake",
                            &stats);
    d_tests_check(stats.generated, "...as is any file under CMakeFiles");
    d_node_tree_free(listed);

    return;
}


/*
d_tests_geometry
  Tests the following:
  - a node's line, columns and width are its first line's
*/
static void
d_tests_geometry(void)
{
    struct d_node_tree* const tree = d_node_tree_new();

    printf("geometry\n");

    (void)d_ext_cmake_build(tree, D_DSS_NO_INDEX, LEAF, sizeof(LEAF) - 1u,
                            NULL, NULL);

    const struct d_node* const command =
        d_node_at(tree, d_tests_find(tree, "command", 3u));
    const struct d_node* const quoted =
        d_node_at(tree, d_tests_find(tree, "argument", 11u));
    const struct d_node* const spanning =
        d_node_at(tree, d_tests_find(tree, "command", 8u));

    d_tests_check( (command) && (command->line == 7u) &&
                   (command->start_column == 9u) &&
                   (command->width == 24u) && (command->end_column == 32u),
                   "a one-line command's place and width");
    d_tests_check( (quoted) && (quoted->line == 7u) &&
                   (quoted->start_column == 20u) &&
                   (quoted->width == 12u),
                   "an argument's place and width, quotes and all");
    d_tests_check( (spanning) && (spanning->line == 12u) &&
                   (spanning->start_column == 5u) &&
                   (spanning->width == 23u),
                   "a command spanning lines is measured on its first");
    d_node_tree_free(tree);

    return;
}


/*
d_tests_relocation
  Tests the following:
  - a node given an attribute after another node has one keeps all of its
    own, the new one last
*/
static void
d_tests_relocation(void)
{
    struct d_node_tree* const tree = d_node_tree_new();

    printf("relocation\n");

    const uint32_t first  = d_node_add(tree, D_DSS_NO_INDEX, "file");
    const uint32_t second = d_node_add(tree, first, "command");

    (void)d_node_set_attribute(tree, first, "path", "a/b.cmake");
    (void)d_node_set_attribute(tree, first, "top", "tools");
    (void)d_node_set_attribute(tree, second, "name", "set");
    (void)d_node_set_attribute(tree, first, "lang", "cmake");
    (void)d_node_set_attribute(tree, second, "depth", "0");
    (void)d_node_set_attribute(tree, first, "generated", NULL);

    d_tests_check( (d_node_at(tree, first)->attribute_count == 4u) &&
                   d_tests_is(tree, first, "path", "a/b.cmake") &&
                   d_tests_is(tree, first, "top", "tools") &&
                   d_tests_is(tree, first, "lang", "cmake") &&
                   d_tests_is(tree, first, "generated", ""),
                   "every attribute of a relocated run is kept");
    d_tests_check( (d_node_at(tree, second)->attribute_count == 2u) &&
                   d_tests_is(tree, second, "name", "set") &&
                   d_tests_is(tree, second, "depth", "0"),
                   "...and the other node's are untouched");
    d_node_tree_free(tree);

    return;
}


/*
d_tests_write
  Writes a text to a file, making the directories on its way.
*/
static bool
d_tests_write(
    const char* _path,
    const char* _text
)
{
    FILE* const out = fopen(_path, "wb");
    bool        ok  = (out != NULL);

    ok = ok && (fputs(_text, out) >= 0);

    // a stream that fails to close has not written what it held
    if ( (out) &&
         (fclose(out) != 0) )
    {
        ok = false;
    }

    return ok;
}


/*
d_tests_banner
  Tests the following:
  - a banner in `#` comments, field by field, at the same columns as C's
  - the facts of a file under build/cmake: its root, path and subsystem
  - delimiters in `#` comments
*/
static void
d_tests_banner(
    const char* _work
)
{
    static const char text[] =
"#*******************************************************************************\n"
"# djinterp [tools]                                                CMakeLists.txt\n"
"#\n"
"# A leaf.\n"
"#\n"
"# path:      /build/cmake/config/testing/djinterp/tools/dawk/CMakeLists.txt\n"
"# link(s):   TBA\n"
"# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10\n"
"#                                                            revised: 2026.10.16\n"
"#*******************************************************************************\n"
"cmake_minimum_required(VERSION 3.16)\n"
"\n"
"#------------------------------------------------------------------------------\n"
"# helpers\n"
"# ============================================================================\n";

    char root[512];
    char path[640];
    bool made = true;

    printf("banner\n");

    // the directories the file's path names, one at a time
    static const char* const steps[] =
    {
        "build", "build/cmake", "build/cmake/config",
        "build/cmake/config/testing", "build/cmake/config/testing/djinterp",
        "build/cmake/config/testing/djinterp/tools",
        "build/cmake/config/testing/djinterp/tools/dawk"
    };

    for (size_t at = 0; at < (sizeof(steps) / sizeof(steps[0])); ++at)
    {
        (void)snprintf(path, sizeof(path), "%s/%s", _work, steps[at]);

        // a directory that is there already is made
        if (d_fs_kind_of(path) != D_FS_KIND_DIRECTORY)
        {
            made = made && (D_TESTS_MKDIR(path) == 0);
        }
    }

    (void)snprintf(root, sizeof(root), "%s/build/cmake", _work);
    (void)snprintf(path, sizeof(path),
                   "%s/config/testing/djinterp/tools/dawk/CMakeLists.txt",
                   root);
    d_tests_check(made && d_tests_write(path, text), "the file is written");

    struct d_node_tree* const tree   = d_node_tree_new();
    const uint32_t            banner = d_banner_build(tree, path, root);
    const uint32_t            file   = (banner != D_DSS_NO_INDEX)
                                     ? d_node_at(tree, banner)->parent
                                     : D_DSS_NO_INDEX;

    d_tests_check(banner != D_DSS_NO_INDEX, "a `#` banner is a banner");
    d_tests_check( d_tests_is(tree, file, "top", "tools") &&
                   d_tests_is(tree, file, "root", "build/cmake") &&
                   d_tests_is(tree, file, "path",
                              "config/testing/djinterp/tools/dawk/"
                              "CMakeLists.txt") &&
                   d_tests_is(tree, file, "name", "CMakeLists.txt"),
                   "the file's root, path, name and subsystem");

    const uint32_t tag   = d_tests_find(tree, "tag", 0u);
    const uint32_t close = d_tests_find(tree, "rule", 1u);
    size_t         fields = 0;

    for (uint32_t at = 0; at < (uint32_t)d_node_count(tree); ++at)
    {
        const struct d_node* const node = d_node_at(tree, at);

        if ( (strcmp(d_node_name(tree, node->type), "field") == 0) &&
             (node->present) )
        {
            fields++;
        }
    }

    d_tests_check( (tag != D_DSS_NO_INDEX) &&
                   (strcmp(d_node_text(tree, d_node_at(tree, tag)->text),
                           "tools") == 0) &&
                   (d_node_at(tree, tag)->start_column == 13u),
                   "the title's tag, where C's would be");
    d_tests_check( (fields == 5u) &&
                   (close != D_DSS_NO_INDEX) &&
                   (d_node_at(tree, close)->line == 10u) &&
                   d_tests_is(tree, close, "position", "close"),
                   "every field read, and the closing rule");
    d_tests_check(d_section_build_marked(tree, file, path, "#") == 2u,
                  "`#` delimiters, of both forms");
    d_tests_check(d_section_build_marked(tree, file, path, "//") == 0u,
                  "...which are not C's");
    d_node_tree_free(tree);

    return;
}


/*
main
  Runs the checks, writing into the directory given or the current one.
*/
int
main(
    int    argc,
    char** argv
)
{
    const char* const work = (argc > 1) ? argv[1] : ".";

    d_tests_claims();
    d_tests_selectors();
    d_tests_broken();
    d_tests_geometry();
    d_tests_relocation();
    d_tests_banner(work);

    printf("\n%d checks, %d failed\n", g_checks, g_failed);

    return (g_failed == 0) ? 0 : 1;
}
