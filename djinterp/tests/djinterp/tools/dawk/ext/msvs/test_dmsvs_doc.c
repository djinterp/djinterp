/*******************************************************************************
* djinterp [tools]                                              test_dmsvs_doc.c
*
* Exercises `dmsvs_doc.h`: each change, with the text it leaves, to the byte;
* handles that follow their elements across changes and name nothing once
* theirs is gone; refusals that leave a document as it was; and saving --
* the directories made, a file that holds the text already left alone, a
* text refused for a path of another kind.
*   With --fuzz, it edits every Visual Studio file under a directory at
* random instead, a few hundred changes to each, and after each proves the
* change did what it said and nothing more: every element it did not touch
* still there under its handle, with its name, its attributes and its text.
*
*   test_dmsvs_doc [<dir>]                          the unit checks, writing
*                                                   beneath <dir>, or here
*   test_dmsvs_doc --fuzz <seed> <dir>...           random changes to every
*                                                   file
*   test_dmsvs_doc --fuzz-into <out> <seed> <dir>.. the same, each file then
*                                                   saved into <out>
*
* path:      /tests/djinterp/tools/dawk/ext/msvs/test_dmsvs_doc.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/msvs/dmsvs_doc.h"  // corresponding header
// std
#include <stdio.h>   // printf, snprintf, remove
#include <stdlib.h>  // malloc, free, strtoul
#include <string.h>  // strcmp, strlen, strstr, strrchr, memcmp, memcpy
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
d_tests_text_is
  Whether a document's text is a text, showing it when it is not.
*/
static bool
d_tests_text_is(
    const struct d_msvs_doc* _doc,
    const char*              _expected
)
{
    const struct d_msvs* const records = d_msvs_doc_records(_doc);
    const bool                 same    =
        (records->length == strlen(_expected)) &&
        (memcmp(records->text, _expected, records->length) == 0);

    // what came instead is shown, for a reader of the failure
    if (!same)
    {
        printf("        got [%.*s]\n", (int)records->length, records->text);
    }

    return same;
}


/*
d_tests_make
  A document that must be made.
*/
static struct d_msvs_doc*
d_tests_make(
    const char* _path,
    const char* _text
)
{
    char                     why[256];
    struct d_msvs_doc* const doc = d_msvs_doc_make(_path, _text,
                                                   strlen(_text), why,
                                                   sizeof(why));

    d_tests_check(doc != NULL, "the document is made");

    // the reason is what a reader of the failure needs
    if (!doc)
    {
        printf("        %s\n", why);
    }

    return doc;
}


/*
d_tests_value
  An element's attribute, decoded, into a buffer; "" when absent.
*/
static const char*
d_tests_value(
    const struct d_msvs_doc* _doc,
    size_t                   _element,
    const char*              _name,
    char*                    _out
)
{
    const struct d_msvs* const records = d_msvs_doc_records(_doc);
    size_t                     start   = 0;
    size_t                     length  = 0;

    _out[0] = '\0';

    // the value decoded, when the attribute is there
    if (d_msvs_attribute(records, &records->elements[_element], _name,
                         &start, &length))
    {
        (void)d_msvs_value(records, start, length, _out);
    }

    return _out;
}


/*
d_tests_building
  Tests the following:
  - elements added to a root holding only space, to an empty element, after
    a last child and before a sibling, each on a line of its own
  - attributes and text, escaped, read back as given
  - a byte-order mark and CRLF kept, and the file's step of indenting used
*/
static void
d_tests_building(void)
{
    static const char skeleton[] =
        "\xEF\xBB\xBF<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n"
        "<Project xmlns=\"x\">\r\n"
        "</Project>";
    static const char built[] =
        "\xEF\xBB\xBF<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n"
        "<Project xmlns=\"x\">\r\n"
        "  <ItemGroup Label=\"A &amp; B\">\r\n"
        "    <ClInclude Include=\"a.h\" />\r\n"
        "    <ClCompile Include=\"m&lt;n&quot;'.c\">\r\n"
        "      <Filter>Source Files</Filter>\r\n"
        "    </ClCompile>\r\n"
        "  </ItemGroup>\r\n"
        "  <Text>1 &lt; 2 &amp;&amp; 3 &gt; 2</Text>\r\n"
        "</Project>";
    const char* const label[]   = { "Label", "A & B" };
    const char* const include[] = { "Include", "m<n\"'.c" };
    const char* const header[]  = { "Include", "a.h" };
    char              value[64];

    printf("building\n");

    struct d_msvs_doc* const doc = d_tests_make("x.vcxproj", skeleton);

    // the checks below need the document made
    if (!doc)
    {
        return;
    }

    const struct d_msvs_new_element group   = { "ItemGroup", NULL, label,
                                                1u };
    const struct d_msvs_new_element compile = { "ClCompile", NULL, include,
                                                1u };
    const struct d_msvs_new_element filter  = { "Filter", "Source Files",
                                                NULL, 0u };
    const struct d_msvs_new_element first   = { "ClInclude", NULL, header,
                                                1u };
    const struct d_msvs_new_element text    = { "Text", "1 < 2 && 3 > 2",
                                                NULL, 0u };

    const size_t items = d_msvs_doc_insert(doc, 0u, D_MSVS_NONE, &group);
    const size_t item  = d_msvs_doc_insert(doc, items, D_MSVS_NONE, &compile);

    d_tests_check( (items == 1u) && (item == 2u),
                   "an element's index is where it was written");
    d_tests_check(d_msvs_doc_insert(doc, item, D_MSVS_NONE, &filter) == 3u,
                  "a child of an empty element opens it");
    d_tests_check(d_msvs_doc_insert(doc, items, item, &first) == 2u,
                  "an element before a sibling");
    d_tests_check(d_msvs_doc_insert(doc, 0u, D_MSVS_NONE, &text) == 5u,
                  "an element after the last child");
    d_tests_check(d_tests_text_is(doc, built), "the text built, to the byte");
    d_tests_check( (strcmp(d_tests_value(doc, 3u, "Include", value),
                           "m<n\"'.c") == 0) &&
                   (strcmp(d_tests_value(doc, 1u, "Label", value),
                           "A & B") == 0),
                   "the values read back as given");

    const struct d_msvs* const records = d_msvs_doc_records(doc);
    char                       content[64];

    (void)d_msvs_text(records, &records->elements[5], content);
    d_tests_check( (strcmp(content, "1 < 2 && 3 > 2") == 0) &&
                   (records->bom) && (records->crlf) &&
                   (d_msvs_doc_changes(doc) == 5u),
                   "the text read back, the mark and the line endings kept");

    d_msvs_doc_free(doc);

    static const char tabbed[] =
        "<Solution>\n"
        "\t<Configurations>\n"
        "\t\t<Platform Name=\"x64\"/>\n"
        "\t</Configurations>\n"
        "</Solution>\n";
    static const char tabbed_built[] =
        "<Solution>\n"
        "\t<Configurations>\n"
        "\t\t<Platform Name=\"x64\"/>\n"
        "\t</Configurations>\n"
        "\t<Project Path=\"a.vcxproj\">\n"
        "\t\t<BuildDependency Project=\"b.vcxproj\" />\n"
        "\t</Project>\n"
        "</Solution>\n";
    const char* const path[]    = { "Path", "a.vcxproj" };
    const char* const depends[] = { "Project", "b.vcxproj" };
    const struct d_msvs_new_element project    = { "Project", NULL, path,
                                                   1u };
    const struct d_msvs_new_element dependency = { "BuildDependency", NULL,
                                                   depends, 1u };

    struct d_msvs_doc* const solution = d_tests_make("x.slnx", tabbed);

    if (!solution)
    {
        return;
    }

    const size_t added = d_msvs_doc_insert(solution, 0u, D_MSVS_NONE,
                                           &project);

    d_tests_check( (added == 3u) &&
                   (d_msvs_doc_insert(solution, added, D_MSVS_NONE,
                                      &dependency) == 4u) &&
                   (d_tests_text_is(solution, tabbed_built)),
                   "the file's own step: a tab");

    d_msvs_doc_free(solution);

    return;
}


/*
d_tests_changing
  Tests the following:
  - an attribute's value replaced, in its own quotes; one added; one removed
  - an element's text replaced, and an empty element given one
  - an element removed with its line, and one sharing its line without it
*/
static void
d_tests_changing(void)
{
    static const char before[] =
        "<Project>\n"
        "  <PropertyGroup Label='Globals' Condition=\"a\">\n"
        "    <ProjectName>old</ProjectName>\n"
        "    <Keyword/>\n"
        "    <Gone>1</Gone>\n"
        "  </PropertyGroup>\n"
        "  <A/><B/>\n"
        "</Project>\n";
    static const char after[] =
        "<Project>\n"
        "  <PropertyGroup Label='It&apos;s' Id=\"7\">\n"
        "    <ProjectName>new &amp; better</ProjectName>\n"
        "    <Keyword>Win32Proj</Keyword>\n"
        "  </PropertyGroup>\n"
        "  <B/>\n"
        "</Project>\n";

    printf("changing\n");

    struct d_msvs_doc* const doc = d_tests_make("x.vcxproj", before);

    if (!doc)
    {
        return;
    }

    d_tests_check( (d_msvs_doc_set_attribute(doc, 1u, "Label", "It's")) &&
                   (d_msvs_doc_set_attribute(doc, 1u, "Id", "7")) &&
                   (d_msvs_doc_remove_attribute(doc, 1u, "Condition")),
                   "attributes replaced, added and removed");
    d_tests_check( (d_msvs_doc_set_text(doc, 2u, "new & better")) &&
                   (d_msvs_doc_set_text(doc, 3u, "Win32Proj")),
                   "texts replaced and given");
    d_tests_check( (d_msvs_doc_remove(doc, 4u)) &&
                   (d_msvs_doc_remove(doc, 4u)),
                   "elements removed");
    d_tests_check(d_tests_text_is(doc, after), "the text changed, to the byte");
    d_tests_check( (!d_msvs_doc_remove_attribute(doc, 1u, "Nothing")) &&
                   (strstr(d_msvs_doc_problem(doc), "no such") != NULL),
                   "an attribute not there is not removed");

    d_msvs_doc_free(doc);

    return;
}


/*
d_tests_handles
  Tests the following:
  - a handle names its element after a change before it, after it and in it
  - the handle of an element removed names nothing, and is not given again
*/
static void
d_tests_handles(void)
{
    static const char text[] =
        "<Project>\n"
        "  <A />\n"
        "  <B>\n"
        "    <C />\n"
        "  </B>\n"
        "  <D />\n"
        "</Project>\n";
    const struct d_msvs_new_element e = { "E", "e", NULL, 0u };

    printf("handles\n");

    struct d_msvs_doc* const doc = d_tests_make("x.props", text);

    if (!doc)
    {
        return;
    }

    const size_t a = d_msvs_doc_handle(doc, 1u);
    const size_t b = d_msvs_doc_handle(doc, 2u);
    const size_t c = d_msvs_doc_handle(doc, 3u);
    const size_t d = d_msvs_doc_handle(doc, 4u);

    d_tests_check( (a != 0u) && (b != a) && (c != b) && (d != c) &&
                   (d_msvs_doc_handle(doc, 5u) == 0u),
                   "each element its own handle, and none past the last");

    const size_t added = d_msvs_doc_insert(doc, 0u,
                                           d_msvs_doc_element(doc, a), &e);
    const size_t made  = d_msvs_doc_handle(doc, added);

    d_tests_check( (added == 1u) &&
                   (d_msvs_doc_element(doc, a) == 2u) &&
                   (d_msvs_doc_element(doc, b) == 3u) &&
                   (d_msvs_doc_element(doc, c) == 4u) &&
                   (d_msvs_doc_element(doc, d) == 5u) &&
                   (made > d),
                   "every handle after an insertion follows its element");

    d_tests_check( (d_msvs_doc_remove(doc, d_msvs_doc_element(doc, b))) &&
                   (d_msvs_doc_element(doc, b) == D_MSVS_NONE) &&
                   (d_msvs_doc_element(doc, c) == D_MSVS_NONE) &&
                   (d_msvs_doc_element(doc, d) == 3u) &&
                   (d_msvs_doc_element(doc, made) == 1u),
                   "the handles of what was removed name nothing");

    d_tests_check( (d_msvs_doc_set_text(doc, d_msvs_doc_element(doc, a),
                                        "x")) &&
                   (d_msvs_doc_element(doc, a) == 2u) &&
                   (d_msvs_doc_element(doc, d) == 3u) &&
                   (d_msvs_doc_handle(doc, 3u) == d),
                   "an empty element opened keeps its handle");

    d_msvs_doc_free(doc);

    return;
}


/*
d_tests_refusals
  Tests the following:
  - a change that would leave the file unreadable, with the document as it
    was
  - names that are not XML's, text where an element is to go, the root
*/
static void
d_tests_refusals(void)
{
    static const char text[] =
        "<Project>\n"
        "  <A>text</A>\n"
        "  <B><C/></B>\n"
        "</Project>\n";
    const struct d_msvs_new_element bad   = { "1x", NULL, NULL, 0u };
    const char* const               odd[] = { "a b", "v" };
    const struct d_msvs_new_element worse = { "X", NULL, odd, 1u };
    const struct d_msvs_new_element fine  = { "X", NULL, NULL, 0u };

    printf("refusals\n");

    struct d_msvs_doc* const doc = d_tests_make("x.vcxproj", text);

    if (!doc)
    {
        return;
    }

    d_tests_check( (!d_msvs_doc_splice(doc, 10u, 10u, "<", 1u)) &&
                   (strstr(d_msvs_doc_problem(doc), "unreadable") != NULL) &&
                   (d_tests_text_is(doc, text)) &&
                   (d_msvs_doc_changes(doc) == 0u),
                   "a change leaving the file unreadable is not kept");
    d_tests_check( (!d_msvs_doc_splice(doc, 0u, 9u, "<Solution>", 10u)) &&
                   (d_tests_text_is(doc, text)),
                   "nor one making the file another kind");
    d_tests_check( (d_msvs_doc_insert(doc, 0u, D_MSVS_NONE, &bad)
                    == D_MSVS_NONE) &&
                   (d_msvs_doc_insert(doc, 0u, D_MSVS_NONE, &worse)
                    == D_MSVS_NONE) &&
                   (!d_msvs_doc_set_attribute(doc, 1u, "=", "v")),
                   "names that are not XML names");
    d_tests_check( (d_msvs_doc_insert(doc, 1u, D_MSVS_NONE, &fine)
                    == D_MSVS_NONE) &&
                   (strstr(d_msvs_doc_problem(doc), "holds text") != NULL),
                   "no element where text is");
    d_tests_check( (d_msvs_doc_insert(doc, 0u, 3u, &fine) == D_MSVS_NONE) &&
                   (!d_msvs_doc_set_text(doc, 2u, "t")) &&
                   (!d_msvs_doc_remove(doc, 0u)) &&
                   (!d_msvs_doc_remove(doc, 99u)) &&
                   (d_tests_text_is(doc, text)),
                   "a grandchild is no sibling; text is not set over elements; "
                   "the root stays");

    d_msvs_doc_free(doc);

    return;
}


/*
d_tests_saving
  Tests the following, writing beneath a directory:
  - the directories a path needs made, and the file written
  - a file holding the text already left alone
  - a text refused for a path of another kind
  - a .sln, which is text, spliced and saved
*/
static void
d_tests_saving(
    const char* _directory
)
{
    static const char text[] = "<Project>\n</Project>\n";
    static const char sln[]  =
        "Microsoft Visual Studio Solution File, Format Version 12.00\n"
        "Global\n"
        "EndGlobal\n";
    char              path[512];
    char              other[512];

    (void)snprintf(path, sizeof(path), "%s/test_dmsvs_doc.out/a/b/x.vcxproj",
                   _directory);
    (void)snprintf(other, sizeof(other), "%s/test_dmsvs_doc.out/x.sln",
                   _directory);
    printf("saving\n");

    (void)remove(path);

    struct d_msvs_doc* const doc = d_tests_make(path, text);

    if (!doc)
    {
        return;
    }

    d_tests_check(d_msvs_doc_save(doc, NULL) == D_MSVS_SAVE_WRITTEN,
                  "a new file written where its directories were made");
    d_tests_check(d_msvs_doc_save(doc, NULL) == D_MSVS_SAVE_UNCHANGED,
                  "a file holding the text already left alone");
    d_tests_check( (d_msvs_doc_save(doc, other) == D_MSVS_SAVE_FAILED) &&
                   (strstr(d_msvs_doc_problem(doc), "kind") != NULL) &&
                   (strcmp(d_msvs_doc_path(doc), path) == 0),
                   "a project is not saved as a solution");

    size_t      length = 0;
    char* const read   = d_text_read(path, &length);

    d_tests_check( (read) && (length == strlen(text)) &&
                   (memcmp(read, text, length) == 0),
                   "the file holds the text");
    free(read);
    d_msvs_doc_free(doc);

    struct d_msvs_doc* const solution = d_tests_make("s.sln", sln);

    if (!solution)
    {
        return;
    }

    static const char section[] = "\tGlobalSection(X) = preSolution\n"
                                  "\tEndGlobalSection\n";
    const size_t      at        = d_msvs_doc_records(solution)->global_close;

    d_tests_check( (d_msvs_doc_splice(solution, at, at, section,
                                      sizeof(section) - 1u)) &&
                   (d_msvs_doc_records(solution)->section_count == 1u),
                   "a .sln spliced");
    d_tests_check( (!d_msvs_doc_splice(solution, 0u, 9u, "", 0u)) &&
                   (d_msvs_doc_records(solution)->section_count == 1u),
                   "and refused when it would not read");

    d_msvs_doc_free(solution);

    return;
}


// d_tests_shape
//   struct: what a fuzzing run expects of each element it did not touch:
//   its handle, and its name, attributes and text, decoded; and where it
//   was, to know what a removal held.
struct d_tests_shape
{
    size_t handle;
    size_t start;
    size_t end;
    char*  name;
    char*  attributes;
    char*  text;
};


/*
d_tests_shape_of
  An element's name, its attributes raw, and its text decoded, as strings.
*/
static void
d_tests_shape_of(
    const struct d_msvs_doc* _doc,
    size_t                   _element,
    struct d_tests_shape*    _out
)
{
    const struct d_msvs* const         records = d_msvs_doc_records(_doc);
    const struct d_msvs_element* const element =
        &records->elements[_element];
    const size_t content = element->content_end - element->content_start;

    _out->handle     = d_msvs_doc_handle(_doc, _element);
    _out->start      = element->start;
    _out->end        = element->end;
    _out->name       = malloc(element->name_length + 1u);
    _out->attributes = malloc(element->attributes_length + 1u);
    _out->text       = malloc(content + 1u);

    memcpy(_out->name, &records->text[element->name], element->name_length);
    _out->name[element->name_length] = '\0';
    memcpy(_out->attributes, &records->text[element->attributes],
           element->attributes_length);
    _out->attributes[element->attributes_length] = '\0';
    (void)d_msvs_text(records, element, _out->text);

    return;
}


/*
d_tests_shapes_free
  Frees the shapes of a reading.
*/
static void
d_tests_shapes_free(
    struct d_tests_shape* _shapes,
    size_t                _count
)
{
    // each shape's strings
    for (size_t at = 0; at < _count; ++at)
    {
        free(_shapes[at].name);
        free(_shapes[at].attributes);
        free(_shapes[at].text);
    }

    free(_shapes);

    return;
}


/*
d_tests_random
  A 64-bit linear congruential step: random enough to choose changes, and
the same every run for a seed.
*/
static size_t
d_tests_random(
    unsigned long long* _state,
    size_t              _below
)
{
    *_state = (*_state * 6364136223846793005ull) + 1442695040888963407ull;

    return (_below > 0u) ? (size_t)((*_state >> 33) % _below) : 0u;
}


// d_tests_fuzzed
//   struct: what a fuzzing run counted, and the directory it saves each
//   file it changed into, or NULL.
struct d_tests_fuzzed
{
    size_t      files;
    size_t      changes;
    size_t      kept;
    size_t      failed;
    const char* into;
};


/*
d_tests_fuzz_change
  One change, chosen at random, to a document; the element it touches goes
to `_touched`, and the element it adds a child to, to `_parent`, each
D_MSVS_NONE when there is none.  Returns whether the change was kept.
*/
static bool
d_tests_fuzz_change(
    struct d_msvs_doc*  _doc,
    unsigned long long* _state,
    size_t*             _touched,
    size_t*             _parent
)
{
    static const char* const names[]  = { "ClCompile", "Filter", "X", "a:b" };
    static const char* const values[] = { "", "a&b", "<x>", "q\"'t", "\t\n",
                                          "$(Dir)\\a.c", "\xC3\xA9" };
    const struct d_msvs* const records = d_msvs_doc_records(_doc);
    const size_t               element =
        d_tests_random(_state, records->element_count);
    const size_t               which   = d_tests_random(_state, 6u);
    const char* const          name    = names[d_tests_random(_state, 4u)];
    const char* const          value   = values[d_tests_random(_state, 7u)];
    const char* const          pair[]  = { "Include", value };

    *_touched = element;
    *_parent  = D_MSVS_NONE;

    // set an attribute, remove one, set a text, add, add before, remove
    if (which == 0u)
    {
        return d_msvs_doc_set_attribute(_doc, element,
                                        (d_tests_random(_state, 2u) == 0u)
                                        ? "Include" : "Condition", value);
    }

    // remove an attribute
    if (which == 1u)
    {
        return d_msvs_doc_remove_attribute(_doc, element, "Include");
    }

    // set a text
    if (which == 2u)
    {
        return d_msvs_doc_set_text(_doc, element, value);
    }

    const struct d_msvs_new_element added = { name,
                                              (d_tests_random(_state, 2u)
                                               == 0u) ? NULL : value,
                                              pair, 1u };

    // add a child last
    if (which == 3u)
    {
        *_parent  = element;
        *_touched = D_MSVS_NONE;

        return (d_msvs_doc_insert(_doc, element, D_MSVS_NONE, &added)
                != D_MSVS_NONE);
    }

    // add a sibling before
    if (which == 4u)
    {
        const size_t parent = records->elements[element].parent;

        *_parent  = parent;
        *_touched = D_MSVS_NONE;

        return ( (parent != D_MSVS_NONE) &&
                 (d_msvs_doc_insert(_doc, parent, element, &added)
                  != D_MSVS_NONE) );
    }

    *_parent = records->elements[element].parent;

    return d_msvs_doc_remove(_doc, element);
}


/*
d_tests_fuzz_file
  Random changes to one file, each proved to have touched only what it
said.
*/
static void
d_tests_fuzz_file(
    const char*            _path,
    unsigned long long*    _state,
    struct d_tests_fuzzed* _counts
)
{
    char                     why[256];
    struct d_msvs_doc* const doc = d_msvs_doc_read(_path, why, sizeof(why));

    // only XML files have elements to change
    if ( (!doc) ||
         (!d_msvs_doc_records(doc)->xml) )
    {
        d_msvs_doc_free(doc);

        return;
    }

    _counts->files++;

    // two hundred changes to the file
    for (size_t round = 0; round < 200u; ++round)
    {
        const struct d_msvs* records = d_msvs_doc_records(doc);
        const size_t         count   = records->element_count;
        struct d_tests_shape* const shapes = malloc(count * sizeof(*shapes));

        // every element's shape, before the change
        for (size_t at = 0; at < count; ++at)
        {
            d_tests_shape_of(doc, at, &shapes[at]);
        }

        size_t     touched = D_MSVS_NONE;
        size_t     parent  = D_MSVS_NONE;
        const bool kept    = d_tests_fuzz_change(doc, _state, &touched,
                                                 &parent);

        _counts->changes++;
        _counts->kept += (kept) ? 1u : 0u;
        records        = d_msvs_doc_records(doc);

        // every element not touched is where its handle says, as it was
        for (size_t at = 0; at < count; ++at)
        {
            const size_t now   = d_msvs_doc_element(doc, shapes[at].handle);
            const bool   below = (touched != D_MSVS_NONE) &&
                                 (shapes[at].start >= shapes[touched].start) &&
                                 (shapes[at].end <= shapes[touched].end);

            // the touched element, and a parent given a child, may differ
            if ( (kept) &&
                 ( (at == touched) || (at == parent) ) )
            {
                continue;
            }

            struct d_tests_shape shape;

            if (now == D_MSVS_NONE)
            {
                // only a removal takes elements, and only those it held
                if ( (!kept) ||
                     (!below) )
                {
                    printf("  FAIL  %s: an element untouched lost its "
                           "handle\n", _path);
                    _counts->failed++;
                }

                continue;
            }

            d_tests_shape_of(doc, now, &shape);

            // its name, its attributes and its text as they were
            if ( (strcmp(shape.name, shapes[at].name) != 0) ||
                 (strcmp(shape.attributes, shapes[at].attributes) != 0) ||
                 (strcmp(shape.text, shapes[at].text) != 0) )
            {
                printf("  FAIL  %s: <%s> changed though untouched, after "
                       "change %lu\n", _path, shapes[at].name,
                       (unsigned long)round);
                _counts->failed++;
            }

            free(shape.name);
            free(shape.attributes);
            free(shape.text);
        }

        d_tests_shapes_free(shapes, count);
    }

    // the file as changed, for another reader to read
    if (_counts->into)
    {
        const char* const name = strrchr(_path, '/');
        char              path[600];

        (void)snprintf(path, sizeof(path), "%s/%lu.%s", _counts->into,
                       (unsigned long)_counts->files,
                       (name) ? (name + 1) : _path);

        // the file as changed, for another reader to read
        if (d_msvs_doc_save(doc, path) == D_MSVS_SAVE_FAILED)
        {
            printf("  FAIL  %s: %s\n", path, d_msvs_doc_problem(doc));
            _counts->failed++;
        }
    }

    d_msvs_doc_free(doc);

    return;
}


/*
d_tests_fuzz_walk
  Every Visual Studio file under a directory, in the order it is read.
*/
static void
d_tests_fuzz_walk(
    const char*            _path,
    unsigned long long*    _state,
    struct d_tests_fuzzed* _counts
)
{
    char** names = NULL;
    size_t count = 0;

    // a file is changed; a directory is walked
    if (d_fs_kind_of(_path) != D_FS_KIND_DIRECTORY)
    {
        // a Visual Studio file is changed
        if (d_msvs_kind_of(_path) != D_MSVS_OTHER)
        {
            d_tests_fuzz_file(_path, _state, _counts);
        }

        return;
    }

    if (!d_fs_names(_path, &names, &count))
    {
        return;
    }

    // each name the directory holds
    for (size_t at = 0; at < count; ++at)
    {
        const size_t length = strlen(_path) + strlen(names[at]) + 2u;
        char* const  child  = malloc(length);

        (void)snprintf(child, length, "%s/%s", _path, names[at]);
        d_tests_fuzz_walk(child, _state, _counts);
        free(child);
    }

    d_fs_names_free(names, count);

    return;
}


int
main(
    int    argc,
    char** argv
)
{
    // random changes to real files, when asked for, kept when asked to be
    if ( (argc >= 4) &&
         ( (strcmp(argv[1], "--fuzz") == 0) ||
           ( (argc >= 5) && (strcmp(argv[1], "--fuzz-into") == 0) ) ) )
    {
        const bool            into   = (strcmp(argv[1], "--fuzz-into") == 0);
        unsigned long long    state  = strtoul(argv[(into) ? 3 : 2], NULL,
                                               10);
        struct d_tests_fuzzed counts = { 0u, 0u, 0u, 0u,
                                         (into) ? argv[2] : NULL };

        // every file or directory named
        for (int at = (into) ? 4 : 3; at < argc; ++at)
        {
            d_tests_fuzz_walk(argv[at], &state, &counts);
        }

        printf("%lu files, %lu changes, %lu kept: %lu failed\n",
               (unsigned long)counts.files, (unsigned long)counts.changes,
               (unsigned long)counts.kept, (unsigned long)counts.failed);

        return ( (counts.failed == 0u) && (counts.files > 0u) ) ? 0 : 1;
    }

    d_tests_building();
    d_tests_changing();
    d_tests_handles();
    d_tests_refusals();
    d_tests_saving((argc > 1) ? argv[1] : ".");

    printf("\n%d checks, %d failed\n", g_checks, g_failed);

    return (g_failed == 0) ? 0 : 1;
}
