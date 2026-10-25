/*******************************************************************************
* djinterp [tools]                                                  test_dwiki.c
*
* Exercises `dwiki.h` and `dwiki_doc.h`: dialects, and pages' names, files,
* keys and anchors; Markdown read as a GitHub wiki reads it, and MediaWiki's
* wikitext as MediaWiki does, record by record; every writer of both, each
* read back as what it was asked to write; and the documents: every change
* a page takes, every refusal leaving the page as it was, line endings and
* the byte-order mark kept, and saving.
*   With --corpus, it reads every page under the directories named instead,
* and proves each reading sound: every record within the text and within
* the record that holds it, in order, on the line its offset gives; the
* sections made from the headings; and the document over the page holding
* its bytes.  With --fuzz, it makes random changes to every page under the
* directories, and proves of each that it was kept only if every record
* outside what it changed reads as it did, and that a page refused one is as
* it was.  With --dump, it prints a page's records, one to a line, as JSON,
* for wiki_oracle.py to compare with other readers'.
*
*   test_dwiki [directory]               the unit checks; saving writes
*                                        beneath the directory, or here
*   test_dwiki --corpus <dir>...         every page under each
*   test_dwiki --fuzz <seed> <dir>...    random changes to every page
*   test_dwiki --dump <file> [dialect]   a page's records
*
* path:      /tests/djinterp/tools/dawk/ext/wiki/test_dwiki.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.19
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/wiki/dwiki.h"  // corresponding header
// std
#include <stddef.h>  // ptrdiff_t
#include <stdio.h>   // printf, putchar, fopen, fread, fclose, snprintf
#include <stdlib.h>  // malloc, free, qsort
#include <string.h>  // strcmp, strlen, strstr, memcmp, memcpy, memset
// djinterp
#include "../../../../../../inc/djinterp/tools/dawk/dfs.h"                 // d_fs_names
#include "../../../../../../inc/djinterp/tools/dawk/ext/wiki/dwiki_doc.h"  // d_wiki_doc


static int g_checks = 0;
static int g_failed = 0;


//==============================================================================
// helpers
//==============================================================================


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
d_tests_is
  Counts one check that a text is the one expected, and shows both when it
is not.
*/
static void
d_tests_is(
    const char* _got,
    const char* _want,
    const char* _what
)
{
    const char* const got = (_got) ? _got : "(none)";

    g_checks++;

    // both, between brackets, so a space at the end shows
    if (strcmp(got, _want) != 0)
    {
        g_failed++;
        printf("  FAIL  %s\n        want [%s]\n        got  [%s]\n", _what,
               _want, got);
    }

    return;
}


/*
d_tests_count_is
  Counts one check that a number is the one expected.
*/
static void
d_tests_count_is(
    size_t      _got,
    size_t      _want,
    const char* _what
)
{
    g_checks++;

    // both numbers, for the reader of the failure
    if (_got != _want)
    {
        g_failed++;
        printf("  FAIL  %s: want %lu, got %lu\n", _what,
               (unsigned long)_want, (unsigned long)_got);
    }

    return;
}


/*
d_tests_read
  Reads a text that must read; a failure is reported as the check named.
*/
static bool
d_tests_read(
    struct d_wiki*      _wiki,
    const char*         _text,
    enum d_wiki_dialect _dialect,
    const char*         _what
)
{
    char       why[256];
    const bool read = d_wiki_read(_wiki, _text, strlen(_text), _dialect, why,
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
d_tests_link_to
  The first link whose target, as written, is the one named; D_WIKI_NONE
for none.
*/
static size_t
d_tests_link_to(
    const struct d_wiki* _wiki,
    const char*          _target
)
{
    // each link, in the page's order
    for (size_t at = 0; at < _wiki->link_count; ++at)
    {
        // a link written to that target
        if ( (_wiki->links[at].target) &&
             (strcmp(_wiki->links[at].target, _target) == 0) )
        {
            return at;
        }
    }

    return D_WIKI_NONE;
}


/*
d_tests_template_named
  The first template of the name; D_WIKI_NONE for none.
*/
static size_t
d_tests_template_named(
    const struct d_wiki* _wiki,
    const char*          _name
)
{
    // each template, in the page's order
    for (size_t at = 0; at < _wiki->template_count; ++at)
    {
        // a template of that name
        if (strcmp(_wiki->templates[at].name, _name) == 0)
        {
            return at;
        }
    }

    return D_WIKI_NONE;
}


/*
d_tests_slice_is
  Whether the text between two offsets of a page is the text expected.
*/
static bool
d_tests_slice_is(
    const struct d_wiki* _wiki,
    size_t               _start,
    size_t               _end,
    const char*          _want
)
{
    return ( (_start != D_WIKI_NONE) &&
             (_end >= _start) &&
             (_end <= _wiki->length) &&
             (strlen(_want) == _end - _start) &&
             (memcmp(_wiki->text + _start, _want, _end - _start) == 0) );
}


/*
d_tests_content_is
  Whether a block's content, its lines without their markers, is the text
expected.
*/
static bool
d_tests_content_is(
    const struct d_wiki* _wiki,
    size_t               _block,
    const char*          _want
)
{
    struct d_wiki_buffer content = { NULL, 0u, 0u, false };
    bool                 same    = false;

    // a block's content, when it has any
    if ( (_block < _wiki->block_count) &&
         (d_wiki_content(_wiki, _block, &content)) )
    {
        same = (strcmp((content.text) ? content.text : "", _want) == 0);
    }

    d_wiki_buffer_free(&content);

    return same;
}


//==============================================================================
// dialects and names
//==============================================================================


/*
d_tests_dialects
  Dialects by a file's extension and by name, and their names.
*/
static void
d_tests_dialects(void)
{
    puts("dialects");

    d_tests_check(d_wiki_dialect_of("wiki/Home.md") == D_WIKI_MARKDOWN,
                  ".md is Markdown");
    d_tests_check(d_wiki_dialect_of("Home.markdown") == D_WIKI_MARKDOWN,
                  ".markdown is Markdown");
    d_tests_check(d_wiki_dialect_of("Main.mediawiki") == D_WIKI_MEDIAWIKI,
                  ".mediawiki is MediaWiki's");
    d_tests_check(d_wiki_dialect_of("a/Main.wiki") == D_WIKI_MEDIAWIKI,
                  ".wiki is MediaWiki's");
    d_tests_check(d_wiki_dialect_of("Main.wikitext") == D_WIKI_MEDIAWIKI,
                  ".wikitext is MediaWiki's");
    d_tests_check(d_wiki_dialect_of("notes.txt") == D_WIKI_OTHER,
                  ".txt is no dialect's");
    d_tests_check(d_wiki_dialect_of("md") == D_WIKI_OTHER,
                  "a name with no extension is no dialect's");
    d_tests_check(d_wiki_dialect_named("GFM") == D_WIKI_MARKDOWN,
                  "gfm names Markdown, in any case");
    d_tests_check(d_wiki_dialect_named("gollum") == D_WIKI_MARKDOWN,
                  "gollum names Markdown");
    d_tests_check(d_wiki_dialect_named("wikitext") == D_WIKI_MEDIAWIKI,
                  "wikitext names MediaWiki's");
    d_tests_check(d_wiki_dialect_named("rst") == D_WIKI_OTHER,
                  "rst names no dialect");
    d_tests_is(d_wiki_dialect_name(D_WIKI_MARKDOWN), "markdown",
               "Markdown's name");
    d_tests_is(d_wiki_dialect_name(D_WIKI_MEDIAWIKI), "mediawiki",
               "MediaWiki's name");

    return;
}


/*
d_tests_name_is
  Counts one check of the page a file holds.
*/
static void
d_tests_name_is(
    const char*         _path,
    enum d_wiki_dialect _dialect,
    const char*         _want
)
{
    char* const name = d_wiki_page_name(_path, _dialect);
    char        what[160];

    (void)snprintf(what, sizeof(what), "the page %s holds", _path);
    d_tests_is(name, _want, what);
    free(name);

    return;
}


/*
d_tests_file_is
  Counts one check of the file that holds a page.
*/
static void
d_tests_file_is(
    const char*         _name,
    enum d_wiki_dialect _dialect,
    const char*         _want
)
{
    char* const file = d_wiki_page_file(_name, _dialect);
    char        what[160];

    (void)snprintf(what, sizeof(what), "the file of the page %s", _name);
    d_tests_is(file, _want, what);
    free(file);

    return;
}


/*
d_tests_key_is
  Counts one check of a name as names compare.
*/
static void
d_tests_key_is(
    const char*         _name,
    enum d_wiki_dialect _dialect,
    const char*         _want
)
{
    struct d_wiki_buffer key = { NULL, 0u, 0u, false };
    char                 what[160];

    (void)d_wiki_page_key(&key, _name, _dialect);
    (void)snprintf(what, sizeof(what), "the key of [%s] in %s", _name,
                   d_wiki_dialect_name(_dialect));
    d_tests_is(key.text, _want, what);
    d_wiki_buffer_free(&key);

    return;
}


/*
d_tests_anchor_is
  Counts one check of the anchor a title is linked by.
*/
static void
d_tests_anchor_is(
    const char*         _title,
    enum d_wiki_dialect _dialect,
    const char*         _want
)
{
    struct d_wiki_buffer anchor = { NULL, 0u, 0u, false };
    char                 what[160];

    (void)d_wiki_anchor(&anchor, _dialect, _title);
    (void)snprintf(what, sizeof(what), "the anchor of [%s] in %s", _title,
                   d_wiki_dialect_name(_dialect));
    d_tests_is(anchor.text, _want, what);
    d_wiki_buffer_free(&anchor);

    return;
}


/*
d_tests_names
  Pages' names from their files and back, keys and anchors.
*/
static void
d_tests_names(void)
{
    puts("names");

    // a page from its file, the dialect the file's or the one given
    d_tests_name_is("wiki/Page-Name.md", D_WIKI_OTHER, "Page Name");
    d_tests_name_is("Page_Name.mediawiki", D_WIKI_OTHER, "Page Name");
    d_tests_name_is("a/b/My-Page.wiki", D_WIKI_OTHER, "My Page");
    d_tests_name_is("snake_case.md", D_WIKI_OTHER, "snake_case");
    d_tests_name_is("snake_case.txt", D_WIKI_MEDIAWIKI, "snake case.txt");
    d_tests_name_is("_Sidebar.md", D_WIKI_OTHER, "_Sidebar");

    // a page's file
    d_tests_file_is("Page Name", D_WIKI_MARKDOWN, "Page-Name.md");
    d_tests_file_is("Page Name", D_WIKI_MEDIAWIKI, "Page-Name.mediawiki");
    d_tests_file_is("a/b: c?", D_WIKI_MARKDOWN, "a-b--c-.md");
    d_tests_file_is("x \"q\" <y>|z", D_WIKI_MARKDOWN, "x--q---y--z.md");

    // keys: a GitHub wiki's in lower case, MediaWiki's first letter only
    d_tests_key_is("  page   name ", D_WIKI_MARKDOWN, "page name");
    d_tests_key_is("Page_Name", D_WIKI_MARKDOWN, "page name");
    d_tests_key_is("Page_Name", D_WIKI_MEDIAWIKI, "page Name");
    d_tests_key_is("Page-Name", D_WIKI_MEDIAWIKI, "page Name");
    d_tests_key_is(":Category:Foo", D_WIKI_MEDIAWIKI, "category:Foo");
    d_tests_key_is("\xc3\x89" "COLE de-x", D_WIKI_MARKDOWN,
                   "\xc3\xa9" "cole de x");
    d_tests_key_is("\xc3\x89" "COLE", D_WIKI_MEDIAWIKI, "\xc3\xa9" "COLE");
    d_tests_check(d_wiki_same_page("page name", "Page-Name",
                                   D_WIKI_MARKDOWN),
                  "a GitHub wiki's names compare in any case, - a space");
    d_tests_check(!d_wiki_same_page("Page Name", "Page name",
                                    D_WIKI_MEDIAWIKI),
                  "MediaWiki's names compare in case after the first");
    d_tests_check(d_wiki_same_page("page name", "Page_name",
                                   D_WIKI_MEDIAWIKI),
                  "MediaWiki's first letter is in any case, _ a space");
    d_tests_check(!d_wiki_same_page("Alpha", "Beta", D_WIKI_MARKDOWN),
                  "two names name two pages");

    // anchors, before they are made unique on a page
    d_tests_anchor_is("Hello, World!", D_WIKI_MARKDOWN, "hello-world");
    d_tests_anchor_is("Hello, World!", D_WIKI_MEDIAWIKI, "Hello,_World!");
    d_tests_anchor_is("a -- b", D_WIKI_MARKDOWN, "a----b");
    d_tests_anchor_is("C++ & C#", D_WIKI_MARKDOWN, "c--c");
    d_tests_anchor_is("  Spaces  and_under ", D_WIKI_MEDIAWIKI,
                      "Spaces_and_under");
    d_tests_anchor_is("\xc3\x9cn Title", D_WIKI_MARKDOWN,
                      "\xc3\xbcn-title");

    return;
}


//==============================================================================
// reading Markdown
//==============================================================================


static const char g_markdown[] =
    "---\n"
    "title: front matter\n"
    "---\n"
    "# Top level\n"
    "\n"
    "Lead [[Usage]], [[Shown|Other Page#part]], [text](Other-Page.md#x"
    " \"t\")\n"
    "and [ref][r], [r], <https://a.example/>, www.example.com and"
    " me@example.com.\n"
    "\n"
    "[r]: https://r.example/path \"Title\"\n"
    "\n"
    "Setext :bulb: idea\n"
    "==================\n"
    "\n"
    "## Usage ##\n"
    "\n"
    "![logo](images/logo.png) [file](docs/manual.pdf) [up](#top-level)\n"
    "\n"
    "```c\n"
    "int x;\n"
    "[[not a link]]\n"
    "```\n"
    "\n"
    "    indented code\n"
    "\n"
    "> quoted [[Quoted Page]]\n"
    "\n"
    "- one\n"
    "  - two [[_TOC_]]\n"
    "\n"
    "| a | b |\n"
    "|---|---|\n"
    "| [[x\\|y]] | `|` |\n"
    "\n"
    "## Usage\n"
    "\n"
    "<!-- dawk:begin gen -->\n"
    "made\n"
    "<!-- dawk:end gen -->\n"
    "\n"
    "<a href=\"Raw-Page\">raw</a> \\[not a link\\] `[[code]]`\n";


/*
d_tests_markdown_blocks
  The blocks of g_markdown: front matter, headings of both kinds, code of
both kinds, a quote, a list within a list, a table, and a comment.
*/
static void
d_tests_markdown_blocks(
    const struct d_wiki* _wiki
)
{
    size_t kinds[D_WIKI_METADATA + 1];
    size_t fence = D_WIKI_NONE;

    memset(kinds, 0, sizeof(kinds));

    // how many of each kind; the fenced code block
    for (size_t at = 0; at < _wiki->block_count; ++at)
    {
        kinds[_wiki->blocks[at].kind]++;

        // the first block of code
        if ( (_wiki->blocks[at].kind == D_WIKI_CODE) &&
             (fence == D_WIKI_NONE) )
        {
            fence = at;
        }
    }

    d_tests_check(_wiki->blocks[0].kind == D_WIKI_METADATA,
                  "front matter is the page's first block");
    d_tests_count_is(_wiki->blocks[0].end_line, 3u,
                     "the front matter ends on line 3");
    d_tests_count_is(kinds[D_WIKI_HEADING], 4u, "4 headings are blocks");
    d_tests_count_is(kinds[D_WIKI_CODE], 2u, "2 blocks of code");
    d_tests_count_is(kinds[D_WIKI_QUOTE], 1u, "1 quote");
    d_tests_count_is(kinds[D_WIKI_ITEM], 2u, "2 items");
    d_tests_count_is(kinds[D_WIKI_TABLE], 1u, "1 table");
    d_tests_count_is(kinds[D_WIKI_DEFINITION], 1u, "1 definition");
    d_tests_count_is(kinds[D_WIKI_COMMENT], 2u, "2 comments");
    d_tests_check( (fence != D_WIKI_NONE) &&
                   (d_tests_slice_is(_wiki, _wiki->blocks[fence].info_start,
                                     _wiki->blocks[fence].info_end, "c")) &&
                   (d_tests_content_is(_wiki, fence,
                                       "int x;\n[[not a link]]")),
                  "a fenced block's language and content");
    d_tests_check( (fence != D_WIKI_NONE) &&
                   (d_tests_content_is(_wiki, fence + 1u, "indented code")),
                  "an indented block's content");

    // the inner item within the outer
    for (size_t at = 0; at < _wiki->block_count; ++at)
    {
        const struct d_wiki_block* const block = &_wiki->blocks[at];

        // an item within another
        if ( (block->kind == D_WIKI_ITEM) &&
             (block->depth > 0u) )
        {
            d_tests_check( (block->parent != D_WIKI_NONE) &&
                           (_wiki->blocks[block->parent].start
                                <= block->start) &&
                           (_wiki->blocks[block->parent].end >= block->end),
                           "an item within an item lies within it");
        }
    }

    return;
}


/*
d_tests_markdown_headings
  The headings and sections of g_markdown: titles as text, anchors as
GitHub makes them, a repeated title's made unique, and the sections'
nesting.
*/
static void
d_tests_markdown_headings(
    const struct d_wiki* _wiki
)
{
    d_tests_count_is(_wiki->heading_count, 4u, "4 headings");

    // nothing to look at when the count is wrong
    if (_wiki->heading_count != 4u)
    {
        return;
    }

    d_tests_is(_wiki->headings[0].title, "Top level", "an ATX title");
    d_tests_is(_wiki->headings[0].anchor, "top-level", "its anchor");
    d_tests_count_is(_wiki->headings[0].line, 4u, "it is on line 4");
    d_tests_is(_wiki->headings[1].title, "Setext \xf0\x9f\x92\xa1 idea",
               "a setext title, its emoji shown");
    d_tests_check( (_wiki->headings[1].setext) &&
                   (_wiki->headings[1].level == 1u),
                   "a setext heading of level 1");
    d_tests_is(_wiki->headings[1].anchor, "setext--idea",
               "an emoji is no part of an anchor");
    d_tests_is(_wiki->headings[2].title, "Usage",
               "a closing sequence is no part of a title");
    d_tests_is(_wiki->headings[2].anchor, "usage", "the first usage");
    d_tests_is(_wiki->headings[3].anchor, "usage-1",
               "a repeated anchor is made unique");
    d_tests_check(_wiki->headings[2].top, "a heading at the top level");

    // the lead, two sections of level 1, two of level 2 within the second
    d_tests_count_is(_wiki->section_count, 5u, "4 sections and the lead");

    // nothing more to look at when the count is wrong
    if (_wiki->section_count != 5u)
    {
        return;
    }

    d_tests_check( (_wiki->sections[0].start == 0u) &&
                   (_wiki->sections[0].end == _wiki->headings[0].start),
                   "the lead is what comes before the first heading");
    d_tests_check( (_wiki->sections[3].parent == 2u) &&
                   (_wiki->sections[4].parent == 2u),
                   "a level-2 section is within the level-1 before it");
    d_tests_check( (_wiki->sections[2].end == _wiki->length) &&
                   (_wiki->sections[2].own_end == _wiki->sections[3].start),
                   "a section's own text ends at its first subsection");

    return;
}


/*
d_tests_markdown_links
  The links of g_markdown, of every form, each with where it leads.
*/
static void
d_tests_markdown_links(
    const struct d_wiki* _wiki
)
{
    size_t at = d_tests_link_to(_wiki, "Usage");

    d_tests_check( (at != D_WIKI_NONE) &&
                   (_wiki->links[at].form == D_WIKI_FORM_WIKI) &&
                   (_wiki->links[at].to == D_WIKI_TO_PAGE) &&
                   (strcmp(_wiki->links[at].label, "Usage") == 0) &&
                   (_wiki->links[at].line == 6u) &&
                   (_wiki->links[at].column == 6u),
                   "[[Usage]]: a page, shown by its name, at 6:6");

    at = d_tests_link_to(_wiki, "Other Page#part");
    d_tests_check( (at != D_WIKI_NONE) &&
                   (strcmp(_wiki->links[at].page, "Other Page") == 0) &&
                   (strcmp(_wiki->links[at].anchor, "part") == 0) &&
                   (strcmp(_wiki->links[at].label, "Shown") == 0),
                   "[[Shown|Other Page#part]]: Gollum's label first");

    at = d_tests_link_to(_wiki, "Other-Page.md#x");
    d_tests_check( (at != D_WIKI_NONE) &&
                   (_wiki->links[at].form == D_WIKI_FORM_INLINE) &&
                   (_wiki->links[at].to == D_WIKI_TO_PAGE) &&
                   (strcmp(_wiki->links[at].page, "Other-Page") == 0) &&
                   (d_tests_slice_is(_wiki, _wiki->links[at].title_start,
                                     _wiki->links[at].title_end, "\"t\"")),
                   "an inline link to a page's file, at an anchor, titled");

    at = d_tests_link_to(_wiki, "https://r.example/path");
    d_tests_check( (at != D_WIKI_NONE) &&
                   (_wiki->links[at].form == D_WIKI_FORM_REFERENCE) &&
                   (_wiki->links[at].definition != D_WIKI_NONE) &&
                   (_wiki->links[_wiki->links[at].definition].form
                        == D_WIKI_FORM_DEFINITION) &&
                   (strcmp(_wiki->links[at].label, "ref") == 0),
                   "a full reference leads where its definition does");
    d_tests_check( (at != D_WIKI_NONE) &&
                   (at + 1u < _wiki->link_count) &&
                   (_wiki->links[at + 1u].form == D_WIKI_FORM_REFERENCE) &&
                   (strcmp(_wiki->links[at + 1u].label, "r") == 0),
                   "a shortcut reference is shown by its label");

    at = d_tests_link_to(_wiki, "https://a.example/");
    d_tests_check( (at != D_WIKI_NONE) &&
                   (_wiki->links[at].form == D_WIKI_FORM_AUTO) &&
                   (_wiki->links[at].to == D_WIKI_TO_URL),
                   "an autolink");

    at = d_tests_link_to(_wiki, "http://www.example.com");
    d_tests_check( (at != D_WIKI_NONE) &&
                   (_wiki->links[at].form == D_WIKI_FORM_BARE),
                   "GitHub's bare www. link, its target a URL");

    at = d_tests_link_to(_wiki, "mailto:me@example.com");
    d_tests_check( (at != D_WIKI_NONE) &&
                   (_wiki->links[at].to == D_WIKI_TO_MAIL),
                   "GitHub's bare address, the full stop after it not, "
                   "its target mailto:");

    at = d_tests_link_to(_wiki, "images/logo.png");
    d_tests_check( (at != D_WIKI_NONE) &&
                   (_wiki->links[at].to == D_WIKI_TO_IMAGE) &&
                   (_wiki->links[at].embedded),
                   "an image, embedded");
    at = d_tests_link_to(_wiki, "docs/manual.pdf");
    d_tests_check( (at != D_WIKI_NONE) &&
                   (_wiki->links[at].to == D_WIKI_TO_FILE),
                   "a link to a file");
    at = d_tests_link_to(_wiki, "#top-level");
    d_tests_check( (at != D_WIKI_NONE) &&
                   (_wiki->links[at].to == D_WIKI_TO_ANCHOR) &&
                   (strcmp(_wiki->links[at].anchor, "top-level") == 0),
                   "a link to an anchor on the page");
    at = d_tests_link_to(_wiki, "_TOC_");
    d_tests_check( (at != D_WIKI_NONE) &&
                   (_wiki->links[at].to == D_WIKI_TO_TAG),
                   "Gollum's [[_TOC_]] is a tag");
    at = d_tests_link_to(_wiki, "Quoted Page");
    d_tests_check( (at != D_WIKI_NONE) &&
                   (_wiki->links[at].block != D_WIKI_NONE) &&
                   (_wiki->blocks[_wiki->links[at].block].kind
                        == D_WIKI_PARAGRAPH),
                   "a link in a quote is in the quote's paragraph");
    at = d_tests_link_to(_wiki, "y");
    d_tests_check( (at != D_WIKI_NONE) &&
                   (strcmp(_wiki->links[at].label, "x") == 0),
                   "a table's \\| is Gollum's |");
    at = d_tests_link_to(_wiki, "Raw-Page");
    d_tests_check( (at != D_WIKI_NONE) &&
                   (_wiki->links[at].form == D_WIKI_FORM_HTML),
                   "<a href> is a link");
    d_tests_check( (d_tests_link_to(_wiki, "not a link") == D_WIKI_NONE) &&
                   (d_tests_link_to(_wiki, "code") == D_WIKI_NONE),
                   "no link in code, nor in escaped brackets");

    return;
}


/*
d_tests_markdown
  g_markdown read, record by record; and what is odd about a page kept as
its problems.
*/
static void
d_tests_markdown(void)
{
    struct d_wiki wiki;

    puts("reading Markdown");

    // the page, every record
    if (d_tests_read(&wiki, g_markdown, D_WIKI_MARKDOWN, "g_markdown"))
    {
        d_tests_markdown_blocks(&wiki);
        d_tests_markdown_headings(&wiki);
        d_tests_markdown_links(&wiki);
        d_tests_count_is(wiki.region_count, 1u, "1 region");
        d_tests_check( (wiki.region_count == 1u) &&
                       (strcmp(wiki.regions[0].name, "gen") == 0) &&
                       (d_tests_slice_is(&wiki, wiki.regions[0].content,
                                         wiki.regions[0].close, "made\n")),
                       "the region's name and text");
        d_tests_count_is(d_wiki_find_region(&wiki, "gen"), 0u,
                         "the region found by name");
        d_tests_count_is(wiki.problem_count, 0u, "nothing odd");
        d_tests_check( (!wiki.bom) && (!wiki.crlf), "LF, no BOM");
        d_wiki_release(&wiki);
    }

    // a region never ended, and one begun twice
    if (d_tests_read(&wiki,
                     "<!-- dawk:begin a -->\nx\n<!-- dawk:end a -->\n\n"
                     "<!-- dawk:begin a -->\ny\n<!-- dawk:end a -->\n\n"
                     "<!-- dawk:begin b -->\nz\n",
                     D_WIKI_MARKDOWN, "regions gone wrong"))
    {
        d_tests_count_is(wiki.problem_count, 2u,
                         "a duplicate and an unended region are problems");
        d_tests_check( (wiki.problem_count == 2u) &&
                       (wiki.problems[0].line == 5u) &&
                       (wiki.problems[1].line == 9u),
                       "each problem on its line");
        d_wiki_release(&wiki);
    }

    // line endings and the byte-order mark
    if (d_tests_read(&wiki, "\xef\xbb\xbf# A\r\n\r\ntext [[B]]\r\n",
                     D_WIKI_MARKDOWN, "CRLF with a BOM"))
    {
        d_tests_check( (wiki.bom) && (wiki.crlf),
                       "CRLF and a BOM are seen");
        d_tests_check( (wiki.heading_count == 1u) &&
                       (strcmp(wiki.headings[0].title, "A") == 0),
                       "a title without its CR");
        d_tests_check( (wiki.link_count == 1u) &&
                       (wiki.links[0].line == 3u) &&
                       (wiki.links[0].column == 6u),
                       "a link's place past CRs");
        d_wiki_release(&wiki);
    }

    return;
}


//==============================================================================
// reading MediaWiki's
//==============================================================================


static const char g_mediawiki[] =
    "{{Infobox\n"
    "| name = Thing\n"
    "| site = [[Site Page|the site]]\n"
    "}}\n"
    "'''Thing''' is a [[thing]]s page. See [https://e.example/a the site]"
    " or https://e.example/b.\n"
    "\n"
    "== Usage ==\n"
    "Use {{tmpl|one|key=two|{{inner}}}} and {{#if:{{{1|}}}|yes|no}}"
    " on {{PAGENAME}}.\n"
    "<nowiki>[[not a link]]</nowiki> <!-- [[nor this]] -->\n"
    "\n"
    "=== Detail ===\n"
    "[[File:Logo.png|thumb|The [[Logo Page|logo]]]]\n"
    "[[Category:Things]] [[:Category:Other]]\n"
    "* one\n"
    "** two\n"
    "<syntaxhighlight lang=\"c\">\n"
    "int x; [[no]]\n"
    "</syntaxhighlight>\n"
    "== Usage ==\n"
    "text<ref>[[Ref Page]]</ref>\n";


/*
d_tests_mediawiki_links
  The links of g_mediawiki: internal, with a trail and a label; external
and bare; images and categories; and none where there are none.
*/
static void
d_tests_mediawiki_links(
    const struct d_wiki* _wiki
)
{
    size_t at = d_tests_link_to(_wiki, "thing");

    d_tests_check( (at != D_WIKI_NONE) &&
                   (strcmp(_wiki->links[at].label, "things") == 0) &&
                   (d_tests_slice_is(_wiki, _wiki->links[at].start,
                                     _wiki->links[at].end, "[[thing]]s")),
                   "a link's trail is its label's, and its text's");
    at = d_tests_link_to(_wiki, "Site Page");
    d_tests_check( (at != D_WIKI_NONE) &&
                   (strcmp(_wiki->links[at].label, "the site") == 0) &&
                   (_wiki->links[at].parent
                        == d_tests_template_named(_wiki, "Infobox")),
                   "a piped link, within the template it is an argument of");
    at = d_tests_link_to(_wiki, "https://e.example/a");
    d_tests_check( (at != D_WIKI_NONE) &&
                   (_wiki->links[at].form == D_WIKI_FORM_EXTERNAL) &&
                   (strcmp(_wiki->links[at].label, "the site") == 0),
                   "an external link and its label");
    at = d_tests_link_to(_wiki, "https://e.example/b");
    d_tests_check( (at != D_WIKI_NONE) &&
                   (_wiki->links[at].form == D_WIKI_FORM_BARE),
                   "a bare URL, without the full stop after it");
    at = d_tests_link_to(_wiki, "File:Logo.png");
    d_tests_check( (at != D_WIKI_NONE) &&
                   (_wiki->links[at].to == D_WIKI_TO_IMAGE) &&
                   (_wiki->links[at].embedded),
                   "a file's image, put in place");
    d_tests_check(d_tests_link_to(_wiki, "Logo Page") != D_WIKI_NONE,
                  "a link within an image's caption");
    at = d_tests_link_to(_wiki, "Category:Things");
    d_tests_check( (at != D_WIKI_NONE) &&
                   (_wiki->links[at].to == D_WIKI_TO_CATEGORY),
                   "a category the page is put in");
    at = d_tests_link_to(_wiki, ":Category:Other");
    d_tests_check( (at != D_WIKI_NONE) &&
                   (_wiki->links[at].to == D_WIKI_TO_PAGE),
                   "a link to a category's page, after `:`");
    d_tests_check(d_tests_link_to(_wiki, "Ref Page") != D_WIKI_NONE,
                  "a link within <ref>");
    d_tests_check( (d_tests_link_to(_wiki, "not a link") == D_WIKI_NONE) &&
                   (d_tests_link_to(_wiki, "nor this") == D_WIKI_NONE) &&
                   (d_tests_link_to(_wiki, "no") == D_WIKI_NONE),
                   "none in <nowiki>, a comment, or code");

    return;
}


/*
d_tests_mediawiki_templates
  The templates of g_mediawiki, each of its kind, with its arguments.
*/
static void
d_tests_mediawiki_templates(
    const struct d_wiki* _wiki
)
{
    const size_t infobox = d_tests_template_named(_wiki, "Infobox");
    const size_t tmpl    = d_tests_template_named(_wiki, "tmpl");
    const size_t inner   = d_tests_template_named(_wiki, "inner");
    const size_t test    = d_tests_template_named(_wiki, "#if");

    d_tests_check( (infobox != D_WIKI_NONE) &&
                   (_wiki->templates[infobox].argument_count == 2u) &&
                   (strcmp(_wiki->arguments[_wiki->templates[infobox]
                                                .first_argument].key,
                           "name") == 0),
                   "a template's named arguments");
    d_tests_check( (tmpl != D_WIKI_NONE) &&
                   (_wiki->templates[tmpl].argument_count == 3u),
                   "{{tmpl|one|key=two|{{inner}}}}: 3 arguments");

    // the arguments of tmpl: 1, key, 2
    if ( (tmpl != D_WIKI_NONE) &&
         (_wiki->templates[tmpl].argument_count == 3u) )
    {
        const struct d_wiki_argument* const first =
            &_wiki->arguments[_wiki->templates[tmpl].first_argument];

        d_tests_is(first[0].key, "1", "a first positional argument is 1");
        d_tests_is(first[1].key, "key", "a named argument");
        d_tests_is(first[2].key, "2",
                   "positions count past a named argument");
        d_tests_check(d_tests_slice_is(_wiki, first[1].value_start,
                                       first[1].value_end, "two"),
                      "a named argument's value");
    }

    d_tests_check( (inner != D_WIKI_NONE) &&
                   (_wiki->templates[inner].parent == tmpl),
                   "a template within another's argument");
    d_tests_check( (test != D_WIKI_NONE) &&
                   (_wiki->templates[test].kind == D_WIKI_FUNCTION),
                   "{{#if:...}} is a parser function");
    d_tests_check( (test != D_WIKI_NONE) &&
                   (test + 1u < _wiki->template_count) &&
                   (_wiki->templates[test + 1u].kind == D_WIKI_PARAMETER) &&
                   (strcmp(_wiki->templates[test + 1u].name, "1") == 0),
                   "{{{1|}}} is a parameter");
    d_tests_check( (d_tests_template_named(_wiki, "PAGENAME")
                        != D_WIKI_NONE) &&
                   (_wiki->templates[d_tests_template_named(_wiki,
                                                            "PAGENAME")]
                        .kind == D_WIKI_VARIABLE),
                   "{{PAGENAME}} is a variable");

    return;
}


/*
d_tests_kind_of
  The kind of the first template of a name, or -1 for none.
*/
static int
d_tests_kind_of(
    const struct d_wiki* _wiki,
    const char*          _name
)
{
    const size_t at = d_tests_template_named(_wiki, _name);

    return (at == D_WIKI_NONE) ? -1 : (int)_wiki->templates[at].kind;
}


/*
d_tests_mediawiki_magic
  Templates' names read by MediaWiki's own magic words: a variable in its
own case, a function before its `:`, the prefixes taken off, comments
gone.
*/
static void
d_tests_mediawiki_magic(void)
{
    struct d_wiki wiki;

    // each kind, by MediaWiki's words
    if (d_tests_read(&wiki, "{{PAGENAME}} {{pagename}} {{lc:X}} "
                     "{{safesubst:Foo|x}} {{subst:Bar}} {{msgnw:Baz}} "
                     "{{1x<!-- c -->\n|a}} {{CURRENTMONTHNAMEGEN}} "
                     "{{#if:a|b}}\n", D_WIKI_MEDIAWIKI, "magic words"))
    {
        d_tests_check(d_tests_kind_of(&wiki, "PAGENAME") == D_WIKI_VARIABLE,
                      "a variable, in its own case");
        d_tests_check(d_tests_kind_of(&wiki, "pagename") == D_WIKI_TEMPLATE,
                      "a variable's name in another case is a template's");
        d_tests_check(d_tests_kind_of(&wiki, "lc") == D_WIKI_FUNCTION,
                      "a function of MediaWiki's, before its `:`");
        d_tests_check(d_tests_kind_of(&wiki, "Foo") == D_WIKI_TEMPLATE,
                      "safesubst: is taken off a template's name");
        d_tests_check(d_tests_kind_of(&wiki, "subst") == D_WIKI_FUNCTION,
                      "subst: substitutes on saving, a function shown");
        d_tests_check(d_tests_kind_of(&wiki, "Baz") == D_WIKI_TEMPLATE,
                      "msgnw: is taken off a template's name");
        d_tests_check(d_tests_kind_of(&wiki, "1x") == D_WIKI_TEMPLATE,
                      "a comment and a line ending are no part of a name");
        d_tests_check(d_tests_kind_of(&wiki, "CURRENTMONTHNAMEGEN") ==
                          D_WIKI_VARIABLE,
                      "each of MediaWiki's variables");
        d_tests_check(d_tests_kind_of(&wiki, "#if") == D_WIKI_FUNCTION,
                      "an extension's function, after `#`");
        d_wiki_release(&wiki);
    }

    return;
}


/*
d_tests_mediawiki_headings
  Headings as MediaWiki's preprocessor and finalizeHeadings make them: the
anchors, told apart; which are the page's own; and those that are none.
*/
static void
d_tests_mediawiki_headings(void)
{
    struct d_wiki wiki;

    // anchors told apart without ASCII's case, each number one not taken
    if (d_tests_read(&wiki, "== a ==\n== A ==\n== a_2 ==\n== b ==\n"
                     "== 0 ==\n== &nbsp; X &nbsp; ==\n",
                     D_WIKI_MEDIAWIKI, "anchors") &&
        (wiki.heading_count == 6u))
    {
        d_tests_is(wiki.headings[1].anchor, "A_2", "a repeat in any case");
        d_tests_is(wiki.headings[2].anchor, "a_2_2",
                   "a number another anchor has is passed");
        d_tests_is(wiki.headings[4].anchor, "0_2",
                   "\"0\" is PHP's false, which a legacy form keeps");
        d_tests_is(wiki.headings[5].anchor, "_X",
                   "space at an anchor's end goes, at its start stays");
        d_wiki_release(&wiki);
    }

    // which headings are the page's own
    if (d_tests_read(&wiki, "[[link\n== One ==\n{{x|\n== Two ==\n}}\n"
                     "<includeonly>\n== Three ==\n[[In Only]]\n"
                     "</includeonly>\n== [[Foo|\nbar]] ==\n"
                     "===a -{b===\n", D_WIKI_MEDIAWIKI, "headings"))
    {
        size_t top = 0;

        // the page's own headings
        for (size_t at = 0; at < wiki.heading_count; ++at)
        {
            top += (wiki.headings[at].top) ? 1u : 0u;
        }

        d_tests_count_is(top, 1u, "only one heading is the page's own");
        d_tests_check( (wiki.heading_count > 0u) &&
                       (strcmp(wiki.headings[0].title, "One") == 0) &&
                       (wiki.headings[0].top),
                       "a heading after a [[ never closed");
        d_tests_check(d_tests_link_to(&wiki, "In Only") != D_WIKI_NONE,
                      "what <includeonly> holds is still read");
        d_wiki_release(&wiki);
    }

    // a title's tags: the sanitizer's go, another is text
    if (d_tests_read(&wiki, "== a <tag>b</tag> ==\n== c <b>d</b> ==\n",
                     D_WIKI_MEDIAWIKI, "tags in titles") &&
        (wiki.heading_count == 2u))
    {
        d_tests_is(wiki.headings[0].title, "a <tag>b</tag>",
                   "a tag the sanitizer does not keep is text");
        d_tests_is(wiki.headings[1].title, "c d",
                   "a tag the sanitizer keeps shows nothing");
        d_wiki_release(&wiki);
    }

    return;
}


/*
d_tests_mediawiki_urls
  URLs ended as MediaWiki ends them, and titles read as it reads them.
*/
static void
d_tests_mediawiki_urls(void)
{
    struct d_wiki wiki;

    // what ends a URL
    if (d_tests_read(&wiki, "http://e.example/x''y''\n\n"
                     "http://e.example/a&amp;\n\nhttp://e.example/z\\\n\n"
                     "[http://[::1]/p x] [[//e.example/r]]\n\n"
                     "{|\n| http://e.example/t||next\n|}\n",
                     D_WIKI_MEDIAWIKI, "URLs"))
    {
        d_tests_check(d_tests_link_to(&wiki, "http://e.example/x") !=
                          D_WIKI_NONE,
                      "two apostrophes end a URL, emphasis by then");
        d_tests_check(d_tests_link_to(&wiki, "http://e.example/a&") !=
                          D_WIKI_NONE,
                      "a reference's `;` stays");
        d_tests_check(d_tests_link_to(&wiki, "http://e.example/z") !=
                          D_WIKI_NONE,
                      "a backslash trails a URL");
        d_tests_check(d_tests_link_to(&wiki, "http://[::1]/p") != D_WIKI_NONE,
                      "an IPv6 address, in brackets");
        d_tests_check(d_tests_link_to(&wiki, "//e.example/r") != D_WIKI_NONE,
                      "[[//...]] is no title, but [//...] within it");
        d_tests_check(d_tests_link_to(&wiki, "http://e.example/t") !=
                          D_WIKI_NONE,
                      "a table's cells end a URL");
        d_wiki_release(&wiki);
    }

    // titles percent-decoded, and {{!}} the pipe
    if (d_tests_read(&wiki, "[[7%25 Solution]] [[Category%3AFoo]] "
                     "[[Main Page{{!}}Else]] [[7%2525 Bad]]\n",
                     D_WIKI_MEDIAWIKI, "titles"))
    {
        const size_t category = d_tests_link_to(&wiki, "Category:Foo");
        const size_t piped    = d_tests_link_to(&wiki, "Main Page");

        d_tests_check(d_tests_link_to(&wiki, "7% Solution") != D_WIKI_NONE,
                      "a title percent-decoded");
        d_tests_check( (category != D_WIKI_NONE) &&
                       (wiki.links[category].to == D_WIKI_TO_CATEGORY),
                       "its namespace, decoded");
        d_tests_check( (piped != D_WIKI_NONE) &&
                       (strcmp(wiki.links[piped].label, "Else") == 0),
                       "{{!}} is the pipe in a link");
        d_tests_check(d_tests_link_to(&wiki, "7%25 Bad") == D_WIKI_NONE,
                      "a title that holds %25 once decoded is none");
        d_wiki_release(&wiki);
    }

    return;
}


/*
d_tests_mediawiki_images
  An image's link=, its caption's URLs, and a gallery's lines.
*/
static void
d_tests_mediawiki_images(void)
{
    struct d_wiki wiki;

    // link=, a caption shown, a gallery
    if (d_tests_read(&wiki, "[[File:A.png|link=Page Z|thumb|cap "
                     "http://e.example/c]] [[File:A.png|link=https://e."
                     "example/o|plain http://e.example/n]]\n"
                     "<gallery>\nB.png|caption [[In Gallery]]\n"
                     "</gallery>\n", D_WIKI_MEDIAWIKI, "images"))
    {
        const size_t option  = d_tests_link_to(&wiki, "Page Z");
        const size_t url     = d_tests_link_to(&wiki, "https://e.example/o");
        const size_t gallery = d_tests_link_to(&wiki, "File:B.png");

        d_tests_check( (option != D_WIKI_NONE) &&
                       (wiki.links[option].form == D_WIKI_FORM_OPTION) &&
                       (wiki.links[option].to == D_WIKI_TO_PAGE),
                       "an image's link= to a page");
        d_tests_check( (url != D_WIKI_NONE) &&
                       (wiki.links[url].to == D_WIKI_TO_URL),
                       "an image's link= to a URL");
        d_tests_check(d_tests_link_to(&wiki, "http://e.example/c") !=
                          D_WIKI_NONE,
                      "a bare URL in a caption shown");
        d_tests_check(d_tests_link_to(&wiki, "http://e.example/n") ==
                          D_WIKI_NONE,
                      "none in a caption that is only alt text");
        d_tests_check( (gallery != D_WIKI_NONE) &&
                       (wiki.links[gallery].form == D_WIKI_FORM_GALLERY) &&
                       (strcmp(wiki.links[gallery].label,
                               "caption In Gallery") == 0),
                       "a gallery's line, in File:, its caption its label");
        d_tests_check(d_tests_link_to(&wiki, "In Gallery") != D_WIKI_NONE,
                      "a link in a gallery's caption");
        d_wiki_release(&wiki);
    }

    return;
}


/*
d_tests_mediawiki
  g_mediawiki read, record by record; a redirect; and what is odd kept as
problems.
*/
static void
d_tests_mediawiki(void)
{
    struct d_wiki wiki;

    puts("reading MediaWiki's");

    // the page, every record
    if (d_tests_read(&wiki, g_mediawiki, D_WIKI_MEDIAWIKI, "g_mediawiki"))
    {
        d_tests_count_is(wiki.heading_count, 3u, "3 headings");

        // the titles and anchors, when there are as many as there should be
        if (wiki.heading_count == 3u)
        {
            d_tests_is(wiki.headings[0].title, "Usage", "a title, trimmed");
            d_tests_is(wiki.headings[1].anchor, "Detail", "an anchor");
            d_tests_is(wiki.headings[2].anchor, "Usage_2",
                       "a repeated anchor is MediaWiki's _2");
            d_tests_count_is(wiki.headings[1].level, 3u, "=== is level 3");
        }

        d_tests_count_is(wiki.section_count, 4u, "3 sections and the lead");
        d_tests_mediawiki_links(&wiki);
        d_tests_mediawiki_templates(&wiki);
        d_tests_count_is(wiki.problem_count, 0u, "nothing odd");

        // the code's content is between its tags
        for (size_t at = 0; at < wiki.block_count; ++at)
        {
            // each block of code
            if (wiki.blocks[at].kind == D_WIKI_CODE)
            {
                d_tests_check(d_tests_content_is(&wiki, at,
                                                 "int x; [[no]]"),
                              "<syntaxhighlight>'s content");
            }
        }

        d_wiki_release(&wiki);
    }

    // a redirect
    if (d_tests_read(&wiki, "#REDIRECT [[New Name#There]]\n",
                     D_WIKI_MEDIAWIKI, "a redirect"))
    {
        d_tests_check( (wiki.redirect != D_WIKI_NONE) &&
                       (strcmp(wiki.links[wiki.redirect].page,
                               "New Name") == 0),
                       "a redirect is to its link's page");
        d_wiki_release(&wiki);
    }

    // a template and a tag never closed
    if (d_tests_read(&wiki, "a {{open|x\n\nb <ref>never\n",
                     D_WIKI_MEDIAWIKI, "never closed"))
    {
        d_tests_check(wiki.problem_count >= 1u,
                      "what is never closed is a problem");
        d_tests_count_is(wiki.template_count, 0u,
                         "an unclosed template is text");
        d_wiki_release(&wiki);
    }

    puts("reading MediaWiki's, as MediaWiki 1.43 does");
    d_tests_mediawiki_magic();
    d_tests_mediawiki_headings();
    d_tests_mediawiki_urls();
    d_tests_mediawiki_images();

    return;
}


//==============================================================================
// writing
//==============================================================================


/*
d_tests_written_is
  Counts one check of what a writer wrote, and frees it.
*/
static void
d_tests_written_is(
    bool                  _written,
    struct d_wiki_buffer* _out,
    const char*           _want,
    const char*           _what
)
{
    d_tests_check(_written, _what);
    d_tests_is(_out->text, _want, _what);
    d_wiki_buffer_free(_out);

    return;
}


/*
d_tests_link_reads
  Counts one check that a link written reads back as its page, anchor and
label.
*/
static void
d_tests_link_reads(
    enum d_wiki_dialect _dialect,
    const char*         _page,
    const char*         _anchor,
    const char*         _label
)
{
    struct d_wiki_buffer out  = { NULL, 0u, 0u, false };
    struct d_wiki        wiki;
    char                 what[200];
    bool                 same = false;

    (void)snprintf(what, sizeof(what), "a link to [%s#%s] shown as [%s] "
                   "reads back, in %s", _page, (_anchor) ? _anchor : "",
                   (_label) ? _label : "", d_wiki_dialect_name(_dialect));

    // written, and read
    if ( (d_wiki_write_page_link(&out, _dialect, _page, _anchor, _label)) &&
         (d_wiki_read(&wiki, out.text, out.length, _dialect, NULL, 0u)) )
    {
        same = (wiki.link_count == 1u) &&
               (d_wiki_same_page(wiki.links[0].page, _page, _dialect)) &&
               (strcmp(wiki.links[0].anchor, (_anchor) ? _anchor : "")
                    == 0) &&
               (strcmp(wiki.links[0].label, (_label) ? _label : _page)
                    == 0);
        d_wiki_release(&wiki);
    }

    d_tests_check(same, what);
    d_wiki_buffer_free(&out);

    return;
}


/*
d_tests_escape_reads
  Counts one check that text escaped reads back as itself: one paragraph,
with no link.
*/
static void
d_tests_escape_reads(
    enum d_wiki_dialect _dialect,
    const char*         _text
)
{
    struct d_wiki_buffer out  = { NULL, 0u, 0u, false };
    struct d_wiki        wiki;
    char                 what[200];
    bool                 same = false;

    (void)snprintf(what, sizeof(what), "[%s] escaped reads as text, in %s",
                   _text, d_wiki_dialect_name(_dialect));

    // written, and read
    if ( (d_wiki_write_escape(&out, _dialect, _text)) &&
         (d_wiki_read(&wiki, out.text, out.length, _dialect, NULL, 0u)) )
    {
        same = (wiki.link_count == 0u) &&
               (wiki.template_count == 0u) &&
               (wiki.heading_count == 0u) &&
               (wiki.block_count == 1u) &&
               (wiki.blocks[0].kind == D_WIKI_PARAGRAPH);
        d_wiki_release(&wiki);
    }

    d_tests_check(same, what);
    d_wiki_buffer_free(&out);

    return;
}


/*
d_tests_writing_markdown
  Markdown's writers.
*/
static void
d_tests_writing_markdown(void)
{
    const enum d_wiki_dialect d   = D_WIKI_MARKDOWN;
    struct d_wiki_buffer      out = { NULL, 0u, 0u, false };

    d_tests_written_is(d_wiki_write_heading(&out, d, 2u, "Usage"), &out,
                       "## Usage\n", "a heading");
    d_tests_written_is(d_wiki_write_heading(&out, d, 1u, "Ends in #"), &out,
                       "# Ends in # #\n",
                       "a title's last # kept by a closing sequence");
    d_tests_check(!d_wiki_write_heading(&out, d, 7u, "x"),
                  "no heading of level 7");
    d_wiki_buffer_free(&out);
    d_tests_written_is(d_wiki_write_page_link(&out, d, "Other Page", "usage",
                                              "shown"),
                       &out, "[[shown|Other Page#usage]]",
                       "Gollum's link, its label first");
    d_tests_written_is(d_wiki_write_page_link(&out, d, "A|B", NULL, NULL),
                       &out, "[A|B](A|B)",
                       "a page Gollum's form cannot hold, linked inline");
    d_tests_written_is(d_wiki_write_url_link(&out, d, "https://e.example/a",
                                             NULL),
                       &out, "<https://e.example/a>", "an autolink");
    d_tests_written_is(d_wiki_write_url_link(&out, d, "https://e.example/a",
                                             "the [site]"),
                       &out, "[the [site]](https://e.example/a)",
                       "a labelled link, its brackets balanced");
    d_tests_written_is(d_wiki_write_url_link(&out, d, "https://e.example/a",
                                             "a ]b *c*"),
                       &out, "[a \\]b *c*](https://e.example/a)",
                       "a label is markup, but for an unbalanced bracket");
    d_tests_written_is(d_wiki_write_image(&out, d, "logo.png", "The logo"),
                       &out, "![The logo](logo.png)", "an image");
    d_tests_written_is(d_wiki_write_code(&out, d, "int x;\n```\ny", "c"),
                       &out, "````c\nint x;\n```\ny\n````\n",
                       "a fence longer than any run of ` in the code");
    d_tests_written_is(d_wiki_write_inline_code(&out, d, "a`b"), &out,
                       "``a`b``", "inline code longer than its `");
    d_tests_written_is(d_wiki_write_item(&out, d, "deep", 3u, true), &out,
                       "        1. deep\n", "an ordered item, two deep");
    d_tests_written_is(d_wiki_write_table(&out, d,
                                          "Name\tWhat\n[[a|b]]\tx | y\n"),
                       &out,
                       "| Name | What |\n| --- | --- |\n"
                       "| [[a|b]] | x \\| y |\n",
                       "a table, a cell's | escaped but within [[...]]");
    d_tests_written_is(d_wiki_write_region(&out, d, "gen", "a\nb"), &out,
                       "<!-- dawk:begin gen -->\na\nb\n"
                       "<!-- dawk:end gen -->\n",
                       "a region");
    d_tests_check(!d_wiki_write_region(&out, d, "two words", "x"),
                  "a region's name is one word");
    d_wiki_buffer_free(&out);

    // links and escaped text read back
    d_tests_link_reads(d, "Other Page", NULL, NULL);
    d_tests_link_reads(d, "Other Page", "part", "Shown");
    d_tests_link_reads(d, "Odd]] page", NULL, "a [label]");
    d_tests_escape_reads(d, "*a* _b_ [c](d) <e> `f` # g www.h.com "
                            "http://i.j me@k.l [[m]] :bulb:");
    d_tests_escape_reads(d, "# not a heading");
    d_tests_escape_reads(d, "1. not an item");

    return;
}


/*
d_tests_writing_mediawiki
  MediaWiki's writers.
*/
static void
d_tests_writing_mediawiki(void)
{
    const enum d_wiki_dialect d   = D_WIKI_MEDIAWIKI;
    struct d_wiki_buffer      out = { NULL, 0u, 0u, false };

    d_tests_written_is(d_wiki_write_heading(&out, d, 2u, "Usage"), &out,
                       "== Usage ==\n", "a heading");
    d_tests_written_is(d_wiki_write_page_link(&out, d, "Other Page", "usage",
                                              "shown"),
                       &out, "[[Other Page#usage|shown]]",
                       "a link, its label last");
    d_tests_written_is(d_wiki_write_page_link(&out, d, "Category:Foo", NULL,
                                              NULL),
                       &out, "[[:Category:Foo]]",
                       "a link to a category's page, after `:`");
    d_tests_check(!d_wiki_write_page_link(&out, d, "A]]B", NULL, NULL),
                  "no title holds ]]");
    d_tests_check(!d_wiki_write_page_link(&out, d, "A", NULL, "x]]y"),
                  "no label holds ]]");
    d_wiki_buffer_free(&out);
    d_tests_written_is(d_wiki_write_url_link(&out, d, "https://e.example/a",
                                             "the site"),
                       &out, "[https://e.example/a the site]",
                       "an external link");
    d_tests_written_is(d_wiki_write_image(&out, d, "logo.png", "The logo"),
                       &out, "[[File:logo.png|alt=The logo]]", "an image");
    d_tests_check(!d_wiki_write_image(&out, d, "logo.png", "a|b"),
                  "no image's text holds |");
    d_wiki_buffer_free(&out);
    d_tests_written_is(d_wiki_write_code(&out, d, "int x;", "c"), &out,
                       "<syntaxhighlight lang=\"c\">\nint x;\n"
                       "</syntaxhighlight>\n",
                       "code highlighted");
    d_tests_written_is(d_wiki_write_code(&out, d, "a < b", NULL), &out,
                       "<pre>\na &lt; b\n</pre>\n",
                       "code with no language, escaped in <pre>");
    d_tests_written_is(d_wiki_write_inline_code(&out, d, "x</nowiki>y"),
                       &out, "<code><nowiki>x&lt;/nowiki>y</nowiki></code>",
                       "inline code, its </nowiki> escaped");
    d_tests_written_is(d_wiki_write_item(&out, d, "deep", 3u, true), &out,
                       "### deep\n", "an ordered item, two deep");
    d_tests_written_is(d_wiki_write_table(&out, d,
                                          "Name\tWhat\n[[a|b]]\tx | y\n"),
                       &out,
                       "{| class=\"wikitable\"\n! Name !! What\n|-\n"
                       "| [[a|b]] || x &#124; y\n|}\n",
                       "a table, a cell's | escaped but within [[...]]");
    d_tests_written_is(d_wiki_write_comment(&out, d, "a --> b"), &out,
                       "<!-- a - - > b -->", "a comment's -- broken");

    // links and escaped text read back
    d_tests_link_reads(d, "Other Page", NULL, NULL);
    d_tests_link_reads(d, "Other Page", "part", "Shown");
    d_tests_link_reads(d, "Category:Foo", NULL, "the category");
    d_tests_escape_reads(d, "'''a''' [[b]] [http://c d] {{e}} <f> "
                            "http://g.h ~~~~ __TOC__");
    d_tests_escape_reads(d, "* not an item");
    d_tests_escape_reads(d, "== not a heading ==");
    d_tests_escape_reads(d, "matrix::operator[] mailto:a@b.c news:x");

    return;
}


/*
d_tests_toc
  A table of contents: the top headings, a level deeper than the one before
at most, each a link to its anchor.
*/
static void
d_tests_toc(void)
{
    struct d_wiki        wiki;
    struct d_wiki_buffer out = { NULL, 0u, 0u, false };

    puts("tables of contents");

    // Markdown's
    if (d_tests_read(&wiki, "# A\n\n### B, b\n\n## C\n\n## C\n",
                     D_WIKI_MARKDOWN, "a page to list"))
    {
        d_tests_written_is(d_wiki_write_toc(&out, &wiki, 1u, 6u), &out,
                           "- [A](#a)\n    - [B, b](#b-b)\n    - [C](#c)\n"
                           "    - [C](#c-1)\n",
                           "a list of links, never two levels deeper");
        d_tests_written_is(d_wiki_write_toc(&out, &wiki, 2u, 2u), &out,
                           "- [C](#c)\n- [C](#c-1)\n",
                           "only the levels asked for");
        d_wiki_release(&wiki);
    }

    // MediaWiki's
    if (d_tests_read(&wiki, "== A ==\n=== B ===\n", D_WIKI_MEDIAWIKI,
                     "a page to list"))
    {
        d_tests_written_is(d_wiki_write_toc(&out, &wiki, 2u, 6u), &out,
                           "* [[#A|A]]\n** [[#B|B]]\n",
                           "MediaWiki's list of links");
        d_wiki_release(&wiki);
    }

    return;
}


//==============================================================================
// documents
//==============================================================================


/*
d_tests_changed
  Counts one check of a change: kept or refused as expected, and the page's
text after it.  A refused change must leave the text as it was.
*/
static void
d_tests_changed(
    struct d_wiki_doc* _doc,
    bool               _kept,
    bool               _want_kept,
    const char*        _want,
    const char*        _what
)
{
    size_t            length = 0;
    const char* const text   = d_wiki_doc_text(_doc, &length);

    g_checks++;

    // kept or refused, as it should be, with the text expected
    if ( (_kept != _want_kept) ||
         (length != strlen(_want)) ||
         (memcmp(text, _want, length) != 0) )
    {
        g_failed++;
        printf("  FAIL  %s: %s%s%s\n        want [%s]\n        got  [%.*s]\n",
               _what, (_kept) ? "kept" : "refused",
               (_kept) ? "" : ": ",
               (_kept) ? "" : d_wiki_doc_problem(_doc), _want, (int)length,
               text);
    }

    return;
}


/*
d_tests_doc_make
  A document of a text that must read.
*/
static struct d_wiki_doc*
d_tests_doc_make(
    const char* _path,
    const char* _text
)
{
    char                     why[256];
    struct d_wiki_doc* const doc = d_wiki_doc_make(_path, _text, strlen(_text),
                                                   D_WIKI_OTHER, why,
                                                   sizeof(why));

    d_tests_check(doc != NULL, _path);

    // why it did not read
    if (!doc)
    {
        printf("        %s\n", why);
    }

    return doc;
}


static const char g_doc_page[] =
    "# Title\n"
    "\n"
    "Intro [a link](Old-Page) and [[Old Page]] and [[Text|Old Page#sec]].\n"
    "\n"
    "## Usage\n"
    "\n"
    "Use it.\n"
    "\n"
    "### Sub\n"
    "\n"
    "sub text\n"
    "\n"
    "## Notes\n"
    "\n"
    "[r]: http://old.example.com\n"
    "See [ref][r].\n";


/*
d_tests_doc_markdown_links
  A Markdown page's links changed: each form, a label added, a reference's
definition, and a link undone.
*/
static void
d_tests_doc_markdown_links(
    struct d_wiki_doc* _doc
)
{
    d_tests_changed(_doc, d_wiki_doc_set_link(_doc, 0u, "New-Page", NULL),
                    true,
                    "# Title\n\nIntro [a link](New-Page) and [[Old Page]] and"
                    " [[Text|Old Page#sec]].\n\n## Usage\n\nUse it.\n\n"
                    "### Sub\n\nsub text\n\n## Notes\n\n"
                    "[r]: http://old.example.com\nSee [ref][r].\n",
                    "an inline link's destination");
    d_tests_changed(_doc, d_wiki_doc_set_link(_doc, 1u, "New Page", NULL),
                    true,
                    "# Title\n\nIntro [a link](New-Page) and [[New Page]] and"
                    " [[Text|Old Page#sec]].\n\n## Usage\n\nUse it.\n\n"
                    "### Sub\n\nsub text\n\n## Notes\n\n"
                    "[r]: http://old.example.com\nSee [ref][r].\n",
                    "Gollum's page");
    d_tests_changed(_doc, d_wiki_doc_set_link(_doc, 2u, "New Page#sec",
                                              "Shown"),
                    true,
                    "# Title\n\nIntro [a link](New-Page) and [[New Page]] and"
                    " [[Shown|New Page#sec]].\n\n## Usage\n\nUse it.\n\n"
                    "### Sub\n\nsub text\n\n## Notes\n\n"
                    "[r]: http://old.example.com\nSee [ref][r].\n",
                    "Gollum's page and label");
    d_tests_changed(_doc, d_wiki_doc_set_link(_doc, 1u, NULL, "Added"),
                    true,
                    "# Title\n\nIntro [a link](New-Page) and [[Added|New Page]]"
                    " and [[Shown|New Page#sec]].\n\n## Usage\n\nUse it.\n\n"
                    "### Sub\n\nsub text\n\n## Notes\n\n"
                    "[r]: http://old.example.com\nSee [ref][r].\n",
                    "a label added to Gollum's link, before its page");
    d_tests_changed(_doc, d_wiki_doc_set_link(_doc, 4u,
                                              "https://new.example.com",
                                              NULL),
                    true,
                    "# Title\n\nIntro [a link](New-Page) and [[Added|New Page]]"
                    " and [[Shown|New Page#sec]].\n\n## Usage\n\nUse it.\n\n"
                    "### Sub\n\nsub text\n\n## Notes\n\n"
                    "[r]: https://new.example.com\nSee [ref][r].\n",
                    "a reference, changed in its definition");
    d_tests_changed(_doc, d_wiki_doc_set_link(_doc, 0u, "a b", NULL), true,
                    "# Title\n\nIntro [a link](<a b>) and [[Added|New Page]]"
                    " and [[Shown|New Page#sec]].\n\n## Usage\n\nUse it.\n\n"
                    "### Sub\n\nsub text\n\n## Notes\n\n"
                    "[r]: https://new.example.com\nSee [ref][r].\n",
                    "a destination with a space, in <>");
    d_tests_changed(_doc, d_wiki_doc_unlink(_doc, 0u), true,
                    "# Title\n\nIntro a link and [[Added|New Page]]"
                    " and [[Shown|New Page#sec]].\n\n## Usage\n\nUse it.\n\n"
                    "### Sub\n\nsub text\n\n## Notes\n\n"
                    "[r]: https://new.example.com\nSee [ref][r].\n",
                    "a link undone to its text");

    return;
}


/*
d_tests_doc_markdown_sections
  A Markdown page's sections changed: a body, a title, one inserted, one
removed, a region added and set; and refusals, each leaving the page.
*/
static void
d_tests_doc_markdown_sections(
    struct d_wiki_doc* _doc
)
{
    const char* const head = "# Title\n\nIntro a link and [[Added|New Page]]"
                             " and [[Shown|New Page#sec]].\n\n";
    const char* const tail = "## Notes\n\n[r]: https://new.example.com\n"
                             "See [ref][r].\n";
    char              want[1024];

    (void)snprintf(want, sizeof(want), "%s## Usage\n\nNew text.\n\n### Sub\n\n"
                   "sub text\n\n%s", head, tail);
    d_tests_changed(_doc, d_wiki_doc_set_body(_doc, 2u, "New text."), true,
                    want, "a section's body, its blank lines kept");
    d_tests_changed(_doc, d_wiki_doc_set_body(_doc, 2u, "x\n\n## Bad\n"),
                    false, want, "a body that holds a heading is refused");
    d_tests_changed(_doc, d_wiki_doc_set_body(_doc, 2u, "x\n\n```\nopen"),
                    false, want, "a body whose fence runs on is refused");
    (void)snprintf(want, sizeof(want), "%s## Usage\n\nNew text.\n\n"
                   "### Subsection # #\n\nsub text\n\n%s", head, tail);
    d_tests_changed(_doc, d_wiki_doc_rename_section(_doc, 3u, "Subsection #"),
                    true, want, "a title retitled, its # kept");
    (void)snprintf(want, sizeof(want), "%s## Usage\n\nNew text.\n\n"
                   "### Subsection # #\n\nsub text\n\n## Inserted\n\n"
                   "Its body.\n\n%s", head, tail);
    d_tests_changed(_doc, d_wiki_doc_insert_section(_doc, 4u, 2u, "Inserted",
                                                    "Its body."),
                    true, want, "a section inserted before another");
    (void)snprintf(want, sizeof(want), "%s## Usage\n\nNew text.\n\n"
                   "<!-- dawk:begin api -->\nmade\n<!-- dawk:end api -->\n\n"
                   "### Subsection # #\n\nsub text\n\n## Inserted\n\n"
                   "Its body.\n\n%s", head, tail);
    d_tests_changed(_doc, d_wiki_doc_add_region(_doc, "api", "made", 2u),
                    true, want, "a region added at a section's own end");
    d_tests_changed(_doc, d_wiki_doc_add_region(_doc, "api", "again", 1u),
                    false, want, "a region's name is the page's once");
    (void)snprintf(want, sizeof(want), "%s## Usage\n\nNew text.\n\n"
                   "<!-- dawk:begin api -->\nremade\ntwice\n"
                   "<!-- dawk:end api -->\n\n"
                   "### Subsection # #\n\nsub text\n\n## Inserted\n\n"
                   "Its body.\n\n%s", head, tail);
    d_tests_changed(_doc, d_wiki_doc_set_region(_doc, "api", "remade\ntwice"),
                    true, want, "a region's text set");
    d_tests_changed(_doc, d_wiki_doc_set_region(_doc, "api", "```\ncode"),
                    false, want, "a region whose fence runs on is refused");
    (void)snprintf(want, sizeof(want), "%s## Usage\n\nNew text.\n\n"
                   "<!-- dawk:begin api -->\nremade\ntwice\n"
                   "<!-- dawk:end api -->\n\n## Inserted\n\n"
                   "Its body.\n\n%s", head, tail);
    d_tests_changed(_doc, d_wiki_doc_remove_section(_doc, 3u), true, want,
                    "a section removed");
    (void)snprintf(want, sizeof(want), "%s## Usage\n\nNew text.\n\n"
                   "remade\ntwice\n\n## Inserted\n\nIts body.\n\n%s", head,
                   tail);
    d_tests_changed(_doc, d_wiki_doc_remove_region(_doc, "api", true), true,
                    want, "a region's markers removed, its text kept");
    d_tests_count_is(d_wiki_doc_changes(_doc), 14u,
                     "every change kept is counted");

    return;
}


/*
d_tests_doc_mediawiki
  A MediaWiki page's links, templates' arguments, regions and sections
changed.
*/
static void
d_tests_doc_mediawiki(void)
{
    struct d_wiki_doc* doc  = d_tests_doc_make("t.mediawiki",
                                               "{{Infobox\n| name = x\n"
                                               "| site = [[Old]]\n}}\n"
                                               "'''B''' [[Old]]s and"
                                               " [[Old|shown]].\n"
                                               "== Usage ==\n"
                                               "Text {{tmpl|a|b=c}}.\n");
    const char*        page = NULL;

    // nothing more to do when it did not read
    if (!doc)
    {
        return;
    }

    page = "{{Infobox\n| name = x\n| site = [[New]]\n}}\n'''B''' [[Old]]s"
           " and [[Old|shown]].\n== Usage ==\nText {{tmpl|a|b=c}}.\n";
    d_tests_changed(doc, d_wiki_doc_set_link(doc, 0u, "New", NULL), true,
                    page, "a link within a template's argument");
    page = "{{Infobox\n| name = x\n| site = [[New]]\n}}\n'''B''' [[New]]s"
           " and [[Old|shown]].\n== Usage ==\nText {{tmpl|a|b=c}}.\n";
    d_tests_changed(doc, d_wiki_doc_set_link(doc, 1u, "New", NULL), true,
                    page, "a link's page, its trail kept");
    page = "{{Infobox\n| name = x\n| site = [[New]]\n}}\n'''B''' "
           "[[:Category:Foo]]s and [[Old|shown]].\n== Usage ==\n"
           "Text {{tmpl|a|b=c}}.\n";
    d_tests_changed(doc, d_wiki_doc_set_link(doc, 1u, "Category:Foo", NULL),
                    true, page, "a link to a category's page gets its `:`");
    d_tests_changed(doc, d_wiki_doc_set_link(doc, 2u, "A]]B", NULL), false,
                    page, "a page no title can be is refused");
    page = "{{Infobox\n| name = y\n| site = [[New]]\n}}\n'''B''' "
           "[[:Category:Foo]]s and [[Old|shown]].\n== Usage ==\n"
           "Text {{tmpl|a|b=c}}.\n";
    d_tests_changed(doc, d_wiki_doc_set_argument(doc, 0u, "name", "y"), true,
                    page, "a named argument's value, its spaces kept");
    page = "{{Infobox\n| name = y\n| site = [[New]]\n| license = MIT\n}}\n"
           "'''B''' [[:Category:Foo]]s and [[Old|shown]].\n== Usage ==\n"
           "Text {{tmpl|a|b=c}}.\n";
    d_tests_changed(doc, d_wiki_doc_set_argument(doc, 0u, "license", "MIT"),
                    true, page, "an argument added as the others are laid");
    d_tests_changed(doc, d_wiki_doc_set_argument(doc, 1u, "b", "x|y"), false,
                    page, "a value that would split is refused");
    page = "{{Infobox\n| name = y\n| site = [[New]]\n| license = MIT\n}}\n"
           "'''B''' [[:Category:Foo]]s and [[Old|shown]].\n== Usage ==\n"
           "Text {{tmpl|a|b=c|second}}.\n";
    d_tests_changed(doc, d_wiki_doc_set_argument(doc, 1u, "2", "second"),
                    true, page, "a second positional argument");
    page = "{{Infobox\n| name = y\n| site = [[New]]\n| license = MIT\n}}\n"
           "'''B''' [[:Category:Foo]]s and [[Old|shown]].\n== Usage ==\n"
           "Text {{tmpl|a|second}}.\n";
    d_tests_changed(doc, d_wiki_doc_remove_argument(doc, 1u, "b"), true,
                    page, "an argument removed");
    page = "{{Infobox\n| name = y\n| site = [[New]]\n| license = MIT\n}}\n"
           "'''B''' [[:Category:Foo]]s and [[Old|shown]].\n\n== Intro ==\n"
           "body\n\n== Usage ==\nText {{tmpl|a|second}}.\n";
    d_tests_changed(doc, d_wiki_doc_insert_section(doc, 1u, 2u, "Intro",
                                                   "body"),
                    true, page,
                    "a section inserted, spaced as the page's headings are");
    page = "{{Infobox\n| name = y\n| site = [[New]]\n| license = MIT\n}}\n"
           "'''B''' [[:Category:Foo]]s and [[Old|shown]].\n\n== Intro ==\n"
           "body\n\n== Usage ==\nText {{tmpl|a|second}}.\n\n"
           "<!-- dawk:begin gen -->\n* a\n<!-- dawk:end gen -->\n";
    d_tests_changed(doc, d_wiki_doc_add_region(doc, "gen", "* a",
                                               D_WIKI_NONE),
                    true, page, "a region added at the page's end");
    d_wiki_doc_free(doc);

    return;
}


/*
d_tests_doc_endings
  A page's CR LF endings, given to what a change writes, and its byte-order
mark kept.
*/
static void
d_tests_doc_endings(void)
{
    struct d_wiki_doc* const doc = d_tests_doc_make("c.md",
                                                    "\xef\xbb\xbf# A\r\n\r\n"
                                                    "text\r\n\r\n## B\r\n\r\n"
                                                    "b\r\n");

    // nothing more to do when it did not read
    if (!doc)
    {
        return;
    }

    d_tests_changed(doc, d_wiki_doc_set_body(doc, 1u, "new\nlines"), true,
                    "\xef\xbb\xbf# A\r\n\r\nnew\r\nlines\r\n\r\n## B\r\n\r\n"
                    "b\r\n",
                    "a body written with the page's CR LF");
    d_tests_changed(doc, d_wiki_doc_insert_section(doc, 2u, 2u, "Mid", "m"),
                    true,
                    "\xef\xbb\xbf# A\r\n\r\nnew\r\nlines\r\n\r\n## Mid\r\n\r\n"
                    "m\r\n\r\n## B\r\n\r\nb\r\n",
                    "a section inserted with the page's CR LF");
    d_wiki_doc_free(doc);

    return;
}


/*
d_tests_doc_save
  A document saved beneath a directory: the directories it needs made, then
written only when it would change, and read back as it was saved.
*/
static void
d_tests_doc_save(
    const char* _directory
)
{
    char               path[512];
    char               why[256];
    struct d_wiki_doc* doc  = NULL;
    struct d_wiki_doc* back = NULL;
    size_t             a    = 0;
    size_t             b    = 0;

    puts("saving");
    (void)snprintf(path, sizeof(path), "%s/dwiki_saved/deep/er/Page.md",
                   _directory);
    doc = d_tests_doc_make(path, "# A\n\ntext\n");

    // nothing more to do when it did not read
    if (!doc)
    {
        return;
    }

    d_tests_check(!d_wiki_doc_changed(doc, &a, &b, &b),
                  "a page not changed says no change");
    why[0] = '\0';
    d_tests_check(d_wiki_doc_save(doc, NULL, why, sizeof(why))
                      == D_WIKI_SAVE_WRITTEN,
                  "a page saved, its directories made");

    // why it was not, when it was not
    if (why[0] != '\0')
    {
        printf("        %s\n", why);
    }

    d_tests_check(d_wiki_doc_save(doc, NULL, why, sizeof(why))
                      == D_WIKI_SAVE_UNCHANGED,
                  "a page saved again is left as it is");
    d_tests_check(d_wiki_doc_set_body(doc, 1u, "more"), "a change");

    // where it was: the body, its blank line with it
    {
        size_t start   = 0;
        size_t old_end = 0;
        size_t new_end = 0;

        const bool said = d_wiki_doc_changed(doc, &start, &old_end, &new_end);

        d_tests_check( (said) &&
                       (start == 4u) && (old_end == 10u) && (new_end == 10u) &&
                       (memcmp(d_wiki_doc_text(doc, &a) + start, "\nmore\n",
                               6u) == 0),
                       "a change says where it was: the body past the "
                       "heading");
    }

    d_tests_check(d_wiki_doc_save(doc, NULL, why, sizeof(why))
                      == D_WIKI_SAVE_WRITTEN,
                  "a changed page saved");
    back = d_wiki_doc_read(path, D_WIKI_OTHER, why, sizeof(why));
    d_tests_check( (back != NULL) &&
                   (d_wiki_doc_dialect(back) == D_WIKI_MARKDOWN) &&
                   (strcmp(d_wiki_doc_text(back, &a),
                           d_wiki_doc_text(doc, &b)) == 0) &&
                   (a == b),
                   "the page read back as it was saved");
    d_wiki_doc_free(back);
    (void)snprintf(path, sizeof(path), "%s/dwiki_saved/none/Page.md/x.md",
                   _directory);
    d_tests_check(d_wiki_doc_read(path, D_WIKI_OTHER, why, sizeof(why))
                      == NULL,
                  "a file that is not there does not read");
    d_wiki_doc_free(doc);

    return;
}


/*
d_tests_documents
  The documents: every change of both dialects, line endings, saving.
*/
static void
d_tests_documents(
    const char* _directory
)
{
    struct d_wiki_doc* const doc = d_tests_doc_make("t.md", g_doc_page);

    puts("documents");

    // the Markdown page's links, then its sections
    if (doc)
    {
        d_tests_doc_markdown_links(doc);
        d_tests_doc_markdown_sections(doc);
        d_wiki_doc_free(doc);
    }

    d_tests_doc_mediawiki();
    d_tests_doc_endings();
    d_tests_doc_save(_directory);

    return;
}


//==============================================================================
// --corpus: every page under a directory, read and proved sound
//==============================================================================


// d_tests_counts
//   struct: what a corpus or fuzz run counted.
struct d_tests_counts
{
    size_t pages;
    size_t records;
    size_t changes;
    size_t kept;
    size_t refused;
    size_t failed;
};


/*
d_tests_load
  A file's bytes, allocated and terminated; NULL when it does not read.
*/
static char*
d_tests_load(
    const char* _path,
    size_t*     _out_length
)
{
    FILE* const file   = fopen(_path, "rb");
    char*       text   = NULL;
    size_t      length = 0;
    size_t      got    = 0;
    char        chunk[8192];

    // a file that cannot be opened is not read
    if (!file)
    {
        return NULL;
    }

    // to its end, a chunk at a time
    while ( (got = fread(chunk, 1u, sizeof(chunk), file)) > 0u )
    {
        char* const grown = realloc(text, length + got + 1u);

        // memory that ran out loses what was read
        if (!grown)
        {
            free(text);
            (void)fclose(file);

            return NULL;
        }

        text = grown;
        memcpy(text + length, chunk, got);
        length += got;
    }

    (void)fclose(file);

    // an empty file is an empty text
    if (!text)
    {
        text = malloc(1u);
    }

    // terminated
    if (text)
    {
        text[length] = '\0';
    }

    *_out_length = length;

    return text;
}


/*
d_tests_sound_blocks
  Whether a page's blocks are sound: in order, each within the text and the
block that holds it, a level deeper, its segments within it, and on the
line its start is.
*/
static const char*
d_tests_sound_blocks(
    const struct d_wiki* _wiki
)
{
    // each block
    for (size_t at = 0; at < _wiki->block_count; ++at)
    {
        const struct d_wiki_block* const block = &_wiki->blocks[at];

        // within the text, in order
        if ( (block->start > block->end) ||
             (block->end > _wiki->length) ||
             ( (at > 0u) && (block->start < _wiki->blocks[at - 1u].start) ) )
        {
            return "a block lies outside the text, or out of order";
        }

        // on its start's line
        if (d_wiki_line_of(_wiki, block->start, NULL) != block->line)
        {
            return "a block's line is not its start's";
        }

        // within its parent, a level deeper
        if ( (block->parent != D_WIKI_NONE) &&
             ( (block->parent >= at) ||
               (_wiki->blocks[block->parent].start > block->start) ||
               (_wiki->blocks[block->parent].end < block->end) ||
               (_wiki->blocks[block->parent].depth + 1u != block->depth) ) )
        {
            return "a block lies outside the block that holds it";
        }

        // a block at the top is at depth 0
        if ( (block->parent == D_WIKI_NONE) &&
             (block->depth != 0u) )
        {
            return "a block at the top is deep";
        }

        // its segments, in order, within its lines
        for (size_t s = 0; s < block->segment_count; ++s)
        {
            const struct d_wiki_segment* const segment =
                &_wiki->segments[block->first_segment + s];

            // each segment within the block's lines
            if ( (block->first_segment + s >= _wiki->segment_count) ||
                 (segment->start > segment->end) ||
                 (segment->start < _wiki->line_starts[block->line - 1u]) ||
                 ( (segment->start < block->start) &&
                   (block->kind != D_WIKI_HTML) &&
                   (block->kind != D_WIKI_COMMENT) ) ||
                 (segment->end > block->end) )
            {
                return "a block's segment lies outside it";
            }
        }
    }

    return NULL;
}


/*
d_tests_sound_headings
  Whether a page's headings are sound, and the sections made from them.
*/
static const char*
d_tests_sound_headings(
    const struct d_wiki* _wiki
)
{
    // each heading, in order, its title within it
    for (size_t at = 0; at < _wiki->heading_count; ++at)
    {
        const struct d_wiki_heading* const heading = &_wiki->headings[at];

        // its title within it, at a level there is, in order
        if ( (heading->start > heading->title_start) ||
             (heading->title_start > heading->title_end) ||
             (heading->title_end > heading->end) ||
             (heading->end > _wiki->length) ||
             (heading->level < 1u) ||
             (heading->level > D_WIKI_MOST_LEVEL) ||
             ( (at > 0u) && (heading->start < _wiki->headings[at - 1u].start) )
             || (!heading->title) || (!heading->anchor) )
        {
            return "a heading lies outside the text, or out of order";
        }
    }

    // the lead, then each section after the one before, within its parent
    if ( (_wiki->section_count == 0u) ||
         (_wiki->sections[0].start != 0u) )
    {
        return "a page's lead is not its first section";
    }

    // each section after the lead
    for (size_t at = 1; at < _wiki->section_count; ++at)
    {
        const struct d_wiki_section* const section = &_wiki->sections[at];

        // within its parent, in order
        if ( (section->start < _wiki->sections[at - 1u].start) ||
             (section->start > section->body) ||
             (section->body > section->own_end) ||
             (section->own_end > section->end) ||
             (section->end > _wiki->length) ||
             (section->heading >= _wiki->heading_count) ||
             ( (section->parent != D_WIKI_NONE) &&
               ( (section->parent >= at) ||
                 (_wiki->sections[section->parent].end < section->end) ||
                 (_wiki->sections[section->parent].level >=
                      section->level) ) ) )
        {
            return "a section lies outside its parent, or out of order";
        }
    }

    return NULL;
}


/*
d_tests_sound_links
  Whether a page's links are sound: in order, within the text, their parts
within them, within their templates, and on the lines they say.
*/
static const char*
d_tests_sound_links(
    const struct d_wiki* _wiki
)
{
    // each link
    for (size_t at = 0; at < _wiki->link_count; ++at)
    {
        const struct d_wiki_link* const link   = &_wiki->links[at];
        size_t                          column = 0;

        // within the text, in order, with every text it has
        if ( (link->start > link->end) ||
             (link->end > _wiki->length) ||
             ( (at > 0u) && (link->start < _wiki->links[at - 1u].start) ) ||
             (!link->target) || (!link->page) || (!link->anchor) ||
             (!link->label) )
        {
            return "a link lies outside the text, or out of order";
        }

        // its target and its label within it
        if ( ( (link->label_start != D_WIKI_NONE) &&
               ( (link->label_start < link->start) ||
                 (link->label_end > link->end) ) ) ||
             ( (link->target_start != D_WIKI_NONE) &&
               (link->form != D_WIKI_FORM_REFERENCE) &&
               ( (link->target_start < link->start) ||
                 (link->target_end > link->end) ) ) )
        {
            return "a link's target or label lies outside it";
        }

        // on its start's line and column
        if ( (d_wiki_line_of(_wiki, link->start, &column) != link->line) ||
             (column != link->column) )
        {
            return "a link's line or column is not its start's";
        }

        // its template, its definition and its block, ones the page has
        if ( ( (link->parent != D_WIKI_NONE) &&
               ( (link->parent >= _wiki->template_count) ||
                 (_wiki->templates[link->parent].start > link->start) ||
                 (_wiki->templates[link->parent].end < link->end) ) ) ||
             ( (link->definition != D_WIKI_NONE) &&
               ( (link->definition >= _wiki->link_count) ||
                 (_wiki->links[link->definition].form
                      != D_WIKI_FORM_DEFINITION) ) ) ||
             ( (link->block != D_WIKI_NONE) &&
               (link->block >= _wiki->block_count) ) )
        {
            return "a link's template, definition or block is wrong";
        }
    }

    return NULL;
}


/*
d_tests_sound_templates
  Whether a page's templates, their arguments and its regions are sound.
*/
static const char*
d_tests_sound_templates(
    const struct d_wiki* _wiki
)
{
    // each template, its arguments within it
    for (size_t at = 0; at < _wiki->template_count; ++at)
    {
        const struct d_wiki_template* const tmpl = &_wiki->templates[at];

        // within the text and its parent
        if ( (tmpl->start > tmpl->name_start) ||
             (tmpl->name_start > tmpl->name_end) ||
             (tmpl->name_end > tmpl->end) ||
             (tmpl->end > _wiki->length) ||
             (tmpl->first_argument + tmpl->argument_count
                  > _wiki->argument_count) ||
             ( (tmpl->parent != D_WIKI_NONE) &&
               ( (tmpl->parent >= _wiki->template_count) ||
                 (_wiki->templates[tmpl->parent].start > tmpl->start) ||
                 (_wiki->templates[tmpl->parent].end < tmpl->end) ) ) )
        {
            return "a template lies outside the text, or its parent";
        }

        // each argument
        for (size_t a = 0; a < tmpl->argument_count; ++a)
        {
            const struct d_wiki_argument* const argument =
                &_wiki->arguments[tmpl->first_argument + a];

            // within its template
            if ( (argument->start < tmpl->name_end) ||
                 (argument->end > tmpl->end) ||
                 (argument->value_start > argument->value_end) ||
                 (argument->value_end > argument->end) ||
                 (!argument->key) )
            {
                return "an argument lies outside its template";
            }
        }
    }

    // each region, its markers about its text
    for (size_t at = 0; at < _wiki->region_count; ++at)
    {
        const struct d_wiki_region* const region = &_wiki->regions[at];

        // its markers about its text
        if ( (region->begin >= region->content) ||
             (region->content > region->close) ||
             (region->close >= region->end) ||
             (region->end > _wiki->length) )
        {
            return "a region's markers are out of order";
        }
    }

    // each problem, by line
    for (size_t at = 1; at < _wiki->problem_count; ++at)
    {
        // a problem before the one before it
        if (_wiki->problems[at].line < _wiki->problems[at - 1u].line)
        {
            return "the problems are out of order";
        }
    }

    return NULL;
}


/*
d_tests_corpus_file
  One page read and proved sound, and its document made of its bytes.
*/
static void
d_tests_corpus_file(
    const char*            _path,
    struct d_tests_counts* _counts
)
{
    size_t             length = 0;
    char* const        text   = d_tests_load(_path, &length);
    struct d_wiki      wiki;
    const char*        wrong  = NULL;
    struct d_wiki_doc* doc    = NULL;

    // a page that cannot be loaded is a failure of its own
    if (!text)
    {
        printf("  FAIL  %s: cannot be read\n", _path);
        _counts->failed++;

        return;
    }

    _counts->pages++;

    // read, and each kind of record proved sound
    if (!d_wiki_read(&wiki, text, length, d_wiki_dialect_of(_path), NULL,
                     0u))
    {
        wrong = "the page does not read";
    }
    else
    {
        _counts->records += wiki.block_count + wiki.heading_count +
                            wiki.link_count + wiki.template_count;
        wrong = d_tests_sound_blocks(&wiki);
        wrong = (wrong) ? wrong : d_tests_sound_headings(&wiki);
        wrong = (wrong) ? wrong : d_tests_sound_links(&wiki);
        wrong = (wrong) ? wrong : d_tests_sound_templates(&wiki);
        d_wiki_release(&wiki);
    }

    // a document of the page holds its bytes
    doc = d_wiki_doc_make(_path, text, length, D_WIKI_OTHER, NULL, 0u);

    // the document, when the reading was sound
    if (!wrong)
    {
        size_t held = 0;

        // the bytes the page has
        if ( (!doc) ||
             (memcmp(d_wiki_doc_text(doc, &held), text, length) != 0) ||
             (held != length) )
        {
            wrong = "the document does not hold the page's bytes";
        }
    }

    // what was wrong, with the page
    if (wrong)
    {
        printf("  FAIL  %s: %s\n", _path, wrong);
        _counts->failed++;
    }

    d_wiki_doc_free(doc);
    free(text);

    return;
}


/*
d_tests_compare_names
  Two names, for qsort, in byte order.
*/
static int
d_tests_compare_names(
    const void* _left,
    const void* _right
)
{
    return strcmp(*(char* const*)_left, *(char* const*)_right);
}


// d_tests_fn_page
//   function: what a walk does with each page it finds.
typedef void (*d_tests_fn_page)(const char*            _path,
                                struct d_tests_counts* _counts);


/*
d_tests_walk
  Every page under a directory, in name order, given to `_each`: a file
whose extension is a dialect's, past names that begin with `.`.
*/
static void
d_tests_walk(
    const char*            _directory,
    d_tests_fn_page        _each,
    struct d_tests_counts* _counts
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

    // in name order
    if (count > 1u)
    {
        qsort(names, count, sizeof(names[0]), d_tests_compare_names);
    }

    // each name the directory holds
    for (size_t at = 0; at < count; ++at)
    {
        const size_t span = strlen(_directory) + strlen(names[at]) + 2u;
        char* const  path = (names[at][0] != '.') ? malloc(span) : NULL;

        // a name that begins with `.` is passed over
        if (!path)
        {
            continue;
        }

        (void)snprintf(path, span, "%s/%s", _directory, names[at]);

        // a directory is walked, and a page given
        if (d_fs_kind_of(path) == D_FS_KIND_DIRECTORY)
        {
            d_tests_walk(path, _each, _counts);
        }
        else if (d_wiki_dialect_of(names[at]) != D_WIKI_OTHER)
        {
            _each(path, _counts);
        }

        free(path);
    }

    d_fs_names_free(names, count);

    return;
}


//==============================================================================
// --fuzz: random changes, each proved to touch only what it said
//==============================================================================


static unsigned long g_random = 1u;


/*
d_tests_random
  A number below `_below`, from a generator of the test's own, so a seed
makes the same changes on every system.
*/
static size_t
d_tests_random(
    size_t _below
)
{
    g_random = (g_random * 1103515245u + 12345u) & 0x7fffffffu;

    return (_below == 0u) ? 0u : (size_t)((g_random >> 8) % _below);
}


static const char* const g_targets[] =
{
    "Home", "Other Page", "Other-Page#usage", "#usage", "https://e.example/x",
    "mailto:a@b.example", "File:Logo.png", "images/logo.png", "A|B", "a]]b",
    "", "x\ny", "Category:Thing", "a b", "<x>"
};

static const char* const g_texts[] =
{
    "plain text", "* a\n* b", "```\ncode\n```", "| a | b |\n|---|---|\n| 1 |",
    "<!-- c -->", "## a heading", "{{tmpl|a}}", "[[Link]]", "line\n\n\nmore",
    "", "with | pipe", "with ]] close", "*emph*", "a\nb", "Title #",
    "\xc3\x9c" "n" "\xc3\xaf" "code", "== h ==", "{{open", "<ref>x</ref>",
    "    code"
};

static const char* const g_snippets[] =
{
    "\n", "\n\n", "#", "# ", "[[", "]]", "[", "](x)", "```", "    ", "|",
    "{{", "}}", "<!--", "-->", "*", "_", "<ref>", "</ref>", "<nowiki>", "==",
    "'''", "x", "\r\n", ":", "http://e.example ", "\n---\n", "> ", "- ",
    "<!-- dawk:begin gen -->\n", "<!-- dawk:end gen -->\n", "[r]: /x\n"
};


/*
d_tests_pick
  One of an array's texts, at random.
*/
static const char*
d_tests_pick(
    const char* const* _texts,
    size_t             _count
)
{
    return _texts[d_tests_random(_count)];
}


/*
d_tests_try
  One random change to a document: of a link, a section, a region, a
template's argument, or a splice of the text.  Gives whether it was kept,
and its name.
*/
static bool
d_tests_try(
    struct d_wiki_doc* _doc,
    const char**       _out_name
)
{
    const struct d_wiki* const wiki     = d_wiki_doc_records(_doc);
    const size_t               links    = wiki->link_count;
    const size_t               sections = wiki->section_count;
    const size_t               t        = sizeof(g_texts) / sizeof(g_texts[0]);
    const size_t               which    = d_tests_random(10u);
    const char* const          text     = d_tests_pick(g_texts, t);

    // a change that has nothing to change is a splice
    if ( (which < 3u) &&
         (links > 0u) )
    {
        const size_t link = d_tests_random(links);

        *_out_name = (which == 0u) ? "set_link" : (which == 1u) ? "set_label"
                                                                : "unlink";

        return (which == 0u)
                   ? d_wiki_doc_set_link(_doc, link,
                                         d_tests_pick(g_targets,
                                                      sizeof(g_targets) /
                                                      sizeof(g_targets[0])),
                                         NULL)
             : (which == 1u) ? d_wiki_doc_set_link(_doc, link, NULL, text)
                             : d_wiki_doc_unlink(_doc, link);
    }

    // a section's body, its title, one inserted or one removed
    if ( (which >= 3u) &&
         (which < 7u) )
    {
        const size_t section = d_tests_random(sections + 1u);

        *_out_name = (which == 3u) ? "set_body" : (which == 4u)
                                                      ? "rename_section"
                   : (which == 5u) ? "insert_section" : "remove_section";

        return (which == 3u) ? d_wiki_doc_set_body(_doc, section, text)
             : (which == 4u) ? d_wiki_doc_rename_section(_doc, section, text)
             : (which == 5u) ? d_wiki_doc_insert_section(_doc, section,
                                                         1u + d_tests_random(
                                                                  3u),
                                                         text, "body")
                             : d_wiki_doc_remove_section(_doc, section);
    }

    // a region added, or its text set
    if (which == 7u)
    {
        *_out_name = "region";

        return (d_tests_random(2u) == 0u)
                   ? d_wiki_doc_add_region(_doc, "gen", text,
                                           d_tests_random(sections + 1u))
                   : d_wiki_doc_set_region(_doc, "gen", text);
    }

    // a template's argument set or removed
    if ( (which == 8u) &&
         (wiki->template_count > 0u) )
    {
        *_out_name = "argument";

        return (d_tests_random(2u) == 0u)
                   ? d_wiki_doc_set_argument(_doc,
                                             d_tests_random(
                                                 wiki->template_count),
                                             (d_tests_random(2u) == 0u)
                                                 ? "1" : "key",
                                             text)
                   : d_wiki_doc_remove_argument(_doc,
                                                d_tests_random(
                                                    wiki->template_count),
                                                "1");
    }

    *_out_name = "splice";

    // a snippet put at random, perhaps over a few bytes
    {
        const size_t      start   = d_tests_random(wiki->length + 1u);
        const size_t      cut     = d_tests_random(6u);
        const size_t      end     = (start + cut < wiki->length)
                                        ? start + cut : wiki->length;
        const char* const snippet = d_tests_pick(g_snippets,
                                                 sizeof(g_snippets) /
                                                 sizeof(g_snippets[0]));

        return d_wiki_doc_splice(_doc, start, end, snippet, strlen(snippet));
    }
}


// d_tests_change
//   struct: where a change was: the old text's `start` to `old_end` is the
//   new text's `start` to `new_end`.
struct d_tests_change
{
    size_t start;
    size_t old_end;
    size_t new_end;
};


/*
d_tests_same_record
  Whether the `_a`-th record of a kind, 0 to 3 -- blocks, headings, links,
templates -- of the old page is the `_b`-th of the new, moved by `_shift`.
*/
static bool
d_tests_same_record(
    int                  _kind,
    const struct d_wiki* _old,
    const struct d_wiki* _new,
    const size_t         _which[2],
    ptrdiff_t            _shift
)
{
    const size_t a = _which[0];
    const size_t b = _which[1];

    // a block: its kind, its span, its depth and its level
    if (_kind == 0)
    {
        const struct d_wiki_block* const x = &_old->blocks[a];
        const struct d_wiki_block* const y = &_new->blocks[b];

        return ( (x->kind == y->kind) &&
                 ((ptrdiff_t)x->start + _shift == (ptrdiff_t)y->start) &&
                 ((ptrdiff_t)x->end + _shift == (ptrdiff_t)y->end) &&
                 (x->depth == y->depth) && (x->level == y->level) );
    }

    // a heading: its level, its span and its title
    if (_kind == 1)
    {
        const struct d_wiki_heading* const x = &_old->headings[a];
        const struct d_wiki_heading* const y = &_new->headings[b];

        return ( (x->level == y->level) &&
                 ((ptrdiff_t)x->start + _shift == (ptrdiff_t)y->start) &&
                 ((ptrdiff_t)x->end + _shift == (ptrdiff_t)y->end) &&
                 (strcmp(x->title, y->title) == 0) );
    }

    // a link: its form, its span, its label, and, but for a reference,
    // where it leads
    if (_kind == 2)
    {
        const struct d_wiki_link* const x = &_old->links[a];
        const struct d_wiki_link* const y = &_new->links[b];

        return ( (x->form == y->form) &&
                 ((ptrdiff_t)x->start + _shift == (ptrdiff_t)y->start) &&
                 ((ptrdiff_t)x->end + _shift == (ptrdiff_t)y->end) &&
                 (strcmp(x->label, y->label) == 0) &&
                 ( (x->form == D_WIKI_FORM_REFERENCE) ||
                   (strcmp(x->target, y->target) == 0) ) );
    }

    // a template: its kind, its span and its name
    return ( (_old->templates[a].kind == _new->templates[b].kind) &&
             ((ptrdiff_t)_old->templates[a].start + _shift ==
                  (ptrdiff_t)_new->templates[b].start) &&
             ((ptrdiff_t)_old->templates[a].end + _shift ==
                  (ptrdiff_t)_new->templates[b].end) &&
             (strcmp(_old->templates[a].name, _new->templates[b].name)
                  == 0) );
}


/*
d_tests_span_of
  Where a record of a kind, 0 to 3, begins and ends; and how many a page
has, for an index past them.
*/
static size_t
d_tests_span_of(
    int                  _kind,
    const struct d_wiki* _wiki,
    size_t               _index,
    size_t*              _out_start,
    size_t*              _out_end
)
{
    const size_t count = (_kind == 0) ? _wiki->block_count
                       : (_kind == 1) ? _wiki->heading_count
                       : (_kind == 2) ? _wiki->link_count
                                      : _wiki->template_count;

    // a record there is, its span
    if (_index < count)
    {
        *_out_start = (_kind == 0) ? _wiki->blocks[_index].start
                    : (_kind == 1) ? _wiki->headings[_index].start
                    : (_kind == 2) ? _wiki->links[_index].start
                                   : _wiki->templates[_index].start;
        *_out_end   = (_kind == 0) ? _wiki->blocks[_index].end
                    : (_kind == 1) ? _wiki->headings[_index].end
                    : (_kind == 2) ? _wiki->links[_index].end
                                   : _wiki->templates[_index].end;
    }

    return count;
}


/*
d_tests_next_outside
  The next record of a kind, from `*_at`, that lies wholly before `_start`
-- or, when `_end` is not D_WIKI_NONE, wholly from `_end` on, past
`_start` -- and moves `*_at` past it.  A record that ends where the change
begins is before it.  Gives D_WIKI_NONE when there is no more.
*/
static size_t
d_tests_next_outside(
    int                  _kind,
    const struct d_wiki* _wiki,
    size_t*              _at,
    size_t               _start,
    size_t               _end
)
{
    size_t first = 0;
    size_t last  = 0;

    // each record from the place, the first that lies where wanted
    while (*_at < d_tests_span_of(_kind, _wiki, *_at, &first, &last))
    {
        const size_t index = (*_at)++;

        // one before the change, or one after it
        if ( ( (_end == D_WIKI_NONE) && (last <= _start) ) ||
             ( (_end != D_WIKI_NONE) && (first >= _end) &&
               (last > _start) ) )
        {
            return index;
        }
    }

    return D_WIKI_NONE;
}


/*
d_tests_outside_same
  Whether every record of a kind outside what a change changed is the old
page's: the same before it, and the same moved after it.
*/
static bool
d_tests_outside_same(
    int                          _kind,
    const struct d_wiki*         _old,
    const struct d_wiki*         _new,
    const struct d_tests_change* _change
)
{
    const ptrdiff_t shift = (ptrdiff_t)_change->new_end -
                            (ptrdiff_t)_change->old_end;

    // before the change, then after it
    for (int pass = 0; pass < 2; ++pass)
    {
        size_t a = 0;
        size_t b = 0;

        // the next of each page's outside it, until neither has one
        for (;;)
        {
            size_t which[2];

            which[0] = d_tests_next_outside(_kind, _old, &a, _change->start,
                                            (pass == 0) ? D_WIKI_NONE
                                                        : _change->old_end);
            which[1] = d_tests_next_outside(_kind, _new, &b, _change->start,
                                            (pass == 0) ? D_WIKI_NONE
                                                        : _change->new_end);

            // both done: every one outside is as it was
            if ( (which[0] == D_WIKI_NONE) &&
                 (which[1] == D_WIKI_NONE) )
            {
                break;
            }

            // one more on one side, or two that differ
            if ( (which[0] == D_WIKI_NONE) ||
                 (which[1] == D_WIKI_NONE) ||
                 (!d_tests_same_record(_kind, _old, _new, which,
                                       (pass == 1) ? shift : 0)) )
            {
                return false;
            }
        }
    }

    return true;
}


/*
d_tests_verify
  Whether a change kept is sound: the text outside where the document says
it changed is the old page's, its records are those of its text read
afresh, and every record outside the change is the old page's.
*/
static const char*
d_tests_verify(
    const struct d_wiki*     _old,
    const struct d_wiki_doc* _doc
)
{
    size_t               length  = 0;
    const char* const    text    = d_wiki_doc_text(_doc, &length);
    const struct d_wiki* held    = d_wiki_doc_records(_doc);
    struct d_wiki        fresh;
    size_t               start   = 0;
    size_t               old_end = 0;
    size_t               new_end = 0;
    const char*          wrong   = NULL;

    // where it changed, and the bytes about it as they were
    if ( (!d_wiki_doc_changed(_doc, &start, &old_end, &new_end)) ||
         (start > old_end) || (old_end > _old->length) ||
         (start > new_end) || (new_end > length) ||
         (_old->length - old_end != length - new_end) )
    {
        return "the document does not say where it changed";
    }

    // every byte outside the change as it was
    if ( (memcmp(_old->text, text, start) != 0) ||
         (memcmp(_old->text + old_end, text + new_end, length - new_end)
              != 0) )
    {
        return "a byte outside the change changed";
    }

    // the text read afresh
    if (!d_wiki_read(&fresh, text, length, _old->dialect, NULL, 0u))
    {
        return "the changed text does not read";
    }

    // the document's records, then the old page's outside the change
    for (int kind = 0; ( (kind < 4) && (!wrong) ); ++kind)
    {
        const struct d_tests_change whole   = { length + 1u, length + 1u,
                                                length + 1u };
        const struct d_tests_change changed = { start, old_end, new_end };

        // all of the document's records are before a change past the end
        if (!d_tests_outside_same(kind, held, &fresh, &whole))
        {
            wrong = "the document's records are not its text's";
        }
        else if (!d_tests_outside_same(kind, _old, &fresh, &changed))
        {
            wrong = (kind == 0) ? "a block outside the change changed"
                  : (kind == 1) ? "a heading outside the change changed"
                  : (kind == 2) ? "a link outside the change changed"
                                : "a template outside the change changed";
        }
    }

    d_wiki_release(&fresh);

    return wrong;
}


/*
d_tests_fuzz_file
  Random changes to one page: each refused one proved to leave the page as
it was, each kept one proved sound.
*/
static void
d_tests_fuzz_file(
    const char*            _path,
    struct d_tests_counts* _counts
)
{
    struct d_wiki_doc* const doc = d_wiki_doc_read(_path, D_WIKI_OTHER, NULL,
                                                   0u);

    // a page that cannot be read is a failure of its own
    if (!doc)
    {
        printf("  FAIL  %s: cannot be read\n", _path);
        _counts->failed++;

        return;
    }

    _counts->pages++;

    // a round of changes, each against the page as it is
    for (int round = 0; round < 40; ++round)
    {
        size_t            length = 0;
        const char* const now    = d_wiki_doc_text(doc, &length);
        char* const       before = malloc(length + 1u);
        struct d_wiki     old;
        const char*       name   = "";
        const char*       wrong  = NULL;
        bool              kept   = false;

        // memory that ran out
        if (!before)
        {
            break;
        }

        memcpy(before, now, length + 1u);

        // the page as it is, read afresh
        if (!d_wiki_read(&old, before, length, d_wiki_doc_dialect(doc), NULL,
                         0u))
        {
            free(before);

            break;
        }

        kept = d_tests_try(doc, &name);
        _counts->changes++;

        // refused: as it was; kept: sound
        if (!kept)
        {
            size_t            after = 0;
            const char* const text  = d_wiki_doc_text(doc, &after);

            _counts->refused++;
            wrong = ( (after != length) ||
                      (memcmp(text, before, length) != 0) )
                        ? "a refused change changed the page" : NULL;
        }
        else
        {
            _counts->kept++;
            wrong = d_tests_verify(&old, doc);
        }

        // what was wrong, with the change
        if (wrong)
        {
            printf("  FAIL  %s: round %d, %s: %s\n", _path, round, name,
                   wrong);
            _counts->failed++;
        }

        d_wiki_release(&old);
        free(before);
    }

    d_wiki_doc_free(doc);

    return;
}


//==============================================================================
// --dump: a page's records as JSON lines
//==============================================================================


/*
d_tests_json
  Prints a text as a JSON string; `_length` of it, or to its end for -1.
*/
static void
d_tests_json(
    const char* _text,
    size_t      _length
)
{
    const size_t length = (_length == (size_t)-1)
                              ? ( (_text) ? strlen(_text) : 0u ) : _length;

    putchar('"');

    // each byte, escaped as JSON escapes it
    for (size_t at = 0; at < length; ++at)
    {
        const unsigned char c = (unsigned char)_text[at];

        // a quote and a backslash escaped
        if ( (c == '"') || (c == '\\') )
        {
            putchar('\\');
            putchar(c);
        }
        else if (c == '\n')
        {
            fputs("\\n", stdout);
        }
        else if (c == '\t')
        {
            fputs("\\t", stdout);
        }
        else if (c < 0x20u)
        {
            printf("\\u%04x", c);
        }
        else
        {
            putchar(c);
        }
    }

    putchar('"');

    return;
}


static const char* const g_block_kinds[] =
{
    "paragraph", "heading", "code", "preformatted", "quote", "item", "table",
    "rule", "html", "comment", "definition", "metadata"
};

static const char* const g_forms[] =
{
    "wiki", "inline", "reference", "auto", "bare", "external", "definition",
    "html", "option", "gallery"
};

static const char* const g_targets_named[] =
{
    "page", "anchor", "url", "mail", "file", "image", "category", "tag"
};


/*
d_tests_dump_blocks
  Prints a page's blocks and headings.
*/
static void
d_tests_dump_blocks(
    const struct d_wiki* _wiki
)
{
    // each block, a code block's content and language too
    for (size_t at = 0; at < _wiki->block_count; ++at)
    {
        const struct d_wiki_block* const block = &_wiki->blocks[at];

        printf("{\"t\":\"block\",\"kind\":\"%s\",\"line\":%lu,"
               "\"end_line\":%lu,\"depth\":%lu", g_block_kinds[block->kind],
               (unsigned long)block->line, (unsigned long)block->end_line,
               (unsigned long)block->depth);

        // a code block's content and language
        if (block->kind == D_WIKI_CODE)
        {
            struct d_wiki_buffer content = { NULL, 0u, 0u, false };

            (void)d_wiki_content(_wiki, at, &content);
            fputs(",\"content\":", stdout);
            d_tests_json(content.text, (size_t)-1);
            d_wiki_buffer_free(&content);

            // its language, written
            if (block->info_start != D_WIKI_NONE)
            {
                fputs(",\"info\":", stdout);
                d_tests_json(_wiki->text + block->info_start,
                             block->info_end - block->info_start);
            }
        }

        puts("}");
    }

    // each heading
    for (size_t at = 0; at < _wiki->heading_count; ++at)
    {
        const struct d_wiki_heading* const heading = &_wiki->headings[at];

        printf("{\"t\":\"heading\",\"level\":%lu,\"line\":%lu,\"top\":%d,"
               "\"start\":%lu,\"end\":%lu,\"title\":",
               (unsigned long)heading->level, (unsigned long)heading->line,
               (heading->top) ? 1 : 0, (unsigned long)heading->start,
               (unsigned long)heading->end);
        d_tests_json(heading->title, (size_t)-1);
        fputs(",\"anchor\":", stdout);
        d_tests_json(heading->anchor, (size_t)-1);
        puts("}");
    }

    return;
}


/*
d_tests_dump_links
  Prints a page's links and templates.
*/
static void
d_tests_dump_links(
    const struct d_wiki* _wiki
)
{
    // each link
    for (size_t at = 0; at < _wiki->link_count; ++at)
    {
        const struct d_wiki_link* const link = &_wiki->links[at];

        printf("{\"t\":\"link\",\"form\":\"%s\",\"to\":\"%s\","
               "\"embedded\":%d,\"line\":%lu,\"column\":%lu,"
               "\"start\":%lu,\"end\":%lu,\"parent\":%ld,\"target\":",
               g_forms[link->form], g_targets_named[link->to],
               (link->embedded) ? 1 : 0, (unsigned long)link->line,
               (unsigned long)link->column, (unsigned long)link->start,
               (unsigned long)link->end,
               (link->parent == D_WIKI_NONE) ? -1L : (long)link->parent);
        d_tests_json(link->target, (size_t)-1);
        fputs(",\"page\":", stdout);
        d_tests_json(link->page, (size_t)-1);
        fputs(",\"anchor\":", stdout);
        d_tests_json(link->anchor, (size_t)-1);
        fputs(",\"label\":", stdout);
        d_tests_json(link->label, (size_t)-1);
        puts("}");
    }

    // each template, with its arguments
    for (size_t at = 0; at < _wiki->template_count; ++at)
    {
        const struct d_wiki_template* const tmpl = &_wiki->templates[at];

        const size_t body = (tmpl->argument_count > 0u)
                                ? _wiki->arguments[tmpl->first_argument].start
                                : tmpl->end;

        printf("{\"t\":\"template\",\"index\":%lu,\"kind\":%d,"
               "\"line\":%lu,\"start\":%lu,\"end\":%lu,\"body\":%lu,"
               "\"parent\":%ld,\"name\":", (unsigned long)at,
               (int)tmpl->kind, (unsigned long)tmpl->line,
               (unsigned long)tmpl->start, (unsigned long)tmpl->end,
               (unsigned long)body,
               (tmpl->parent == D_WIKI_NONE) ? -1L : (long)tmpl->parent);
        d_tests_json(tmpl->name, (size_t)-1);
        fputs(",\"args\":[", stdout);

        // each argument, by key and value
        for (size_t a = 0; a < tmpl->argument_count; ++a)
        {
            const struct d_wiki_argument* const argument =
                &_wiki->arguments[tmpl->first_argument + a];

            fputs( (a > 0u) ? ",{\"key\":" : "{\"key\":", stdout);
            d_tests_json(argument->key, (size_t)-1);
            fputs(",\"value\":", stdout);
            d_tests_json(_wiki->text + argument->value_start,
                         argument->value_end - argument->value_start);
            putchar('}');
        }

        puts("]}");
    }

    return;
}


/*
d_tests_dump
  Prints a page's records, one to a line, as JSON: its blocks, headings,
links, templates, regions and problems.
*/
static int
d_tests_dump(
    const char* _path,
    const char* _dialect
)
{
    size_t                    length  = 0;
    char* const               text    = d_tests_load(_path, &length);
    const enum d_wiki_dialect dialect = (_dialect)
                                            ? d_wiki_dialect_named(_dialect)
                                            : d_wiki_dialect_of(_path);
    struct d_wiki             wiki;
    char                      why[256];

    // a page that cannot be loaded or read
    if ( (!text) ||
         (!d_wiki_read(&wiki, text, length, dialect, why, sizeof(why))) )
    {
        printf("%s: does not read\n", _path);
        free(text);

        return 1;
    }

    d_tests_dump_blocks(&wiki);
    d_tests_dump_links(&wiki);

    // its regions, then its problems
    for (size_t at = 0; at < wiki.region_count; ++at)
    {
        fputs("{\"t\":\"region\",\"name\":", stdout);
        d_tests_json(wiki.regions[at].name, (size_t)-1);
        printf(",\"line\":%lu}\n", (unsigned long)wiki.regions[at].line);
    }

    // each problem
    for (size_t at = 0; at < wiki.problem_count; ++at)
    {
        printf("{\"t\":\"problem\",\"line\":%lu,\"what\":",
               (unsigned long)wiki.problems[at].line);
        d_tests_json(wiki.problems[at].what, (size_t)-1);
        puts("}");
    }

    d_wiki_release(&wiki);
    free(text);

    return 0;
}


//==============================================================================
// main
//==============================================================================


/*
main
  The unit checks; with --corpus every page under the directories, with
--fuzz random changes to them, and with --dump a page's records.
*/
int
main(
    int    argc,
    char** argv
)
{
    struct d_tests_counts counts;

    memset(&counts, 0, sizeof(counts));

    // a corpus run reads pages and reports them, and nothing else
    if ( (argc > 2) &&
         (strcmp(argv[1], "--corpus") == 0) )
    {
        // every directory named
        for (int at = 2; at < argc; ++at)
        {
            d_tests_walk(argv[at], d_tests_corpus_file, &counts);
        }

        printf("\n%lu pages, %lu records: %lu failed\n",
               (unsigned long)counts.pages, (unsigned long)counts.records,
               (unsigned long)counts.failed);

        return ( (counts.failed == 0u) && (counts.pages > 0u) ) ? 0 : 1;
    }

    // a fuzz run changes copies held in memory, and writes nothing
    if ( (argc > 3) &&
         (strcmp(argv[1], "--fuzz") == 0) )
    {
        g_random = strtoul(argv[2], NULL, 10);

        // every directory named
        for (int at = 3; at < argc; ++at)
        {
            d_tests_walk(argv[at], d_tests_fuzz_file, &counts);
        }

        printf("\n%lu pages, %lu changes: %lu kept, %lu refused; "
               "%lu failed\n", (unsigned long)counts.pages,
               (unsigned long)counts.changes, (unsigned long)counts.kept,
               (unsigned long)counts.refused, (unsigned long)counts.failed);

        return ( (counts.failed == 0u) && (counts.pages > 0u) ) ? 0 : 1;
    }

    // a dump prints a page's records, for comparing with other readers'
    if ( (argc >= 3) &&
         (argc <= 4) &&
         (strcmp(argv[1], "--dump") == 0) )
    {
        return d_tests_dump(argv[2], (argc == 4) ? argv[3] : NULL);
    }

    d_tests_dialects();
    d_tests_names();
    d_tests_markdown();
    d_tests_mediawiki();
    puts("writing");
    d_tests_writing_markdown();
    d_tests_writing_mediawiki();
    d_tests_toc();
    d_tests_documents( (argc > 1) ? argv[1] : ".");

    printf("\n%d checks, %d failed\n", g_checks, g_failed);

    return (g_failed == 0) ? 0 : 1;
}
