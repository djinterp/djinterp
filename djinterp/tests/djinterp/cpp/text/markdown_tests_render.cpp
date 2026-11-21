/*******************************************************************************
* djinterp [test]                                      markdown_tests_render.cpp
*
*   The groups for markdown/markdown_template.hpp's four renderings of a tree
* built on its default backend: Markdown that reads back as the same tree,
* CommonMark's and GFM's HTML, plain text, and CommonMark's XML. Each
* expected text is CommonMark's (or GFM's, for tables and task items); the
* Markdown ones were also read back by a CommonMark parser, and gave the
* HTML written here.
*   Lists' looseness, task items' boxes and tables are built through the
* facades' setters and the table factories; a list item's bullet, and the
* kinds with no factory, are set on the default backend's nodes.
*
*
* path:      /tests/djinterp/cpp/text/markdown_tests_render.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/
#include "markdown_tests.hpp"  // D_TESTS_TEXT_CHECK, the groups, builders
// std
#include <string>  // std::string
#include <vector>  // std::vector


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

/*
d_tests_markdown_block_of
  A block of any kind, for the kinds with no factory: a paragraph, made
again as the kind.
*/
static md_new_block
d_tests_markdown_block_of(
    djinterp::markdown::DMarkdownBlockKind _kind
)
{
    md_new_block block = djinterp::markdown::make_paragraph<md_backend>();

    block.backend_handle()->block_kind = _kind;

    return block;
}

/*
d_tests_markdown_inline_of
  An inline of any kind, holding a text: a text, made again as the kind.
*/
static md_new_inline
d_tests_markdown_inline_of(
    djinterp::markdown::DMarkdownInlineKind _kind,
    const std::string&                      _text
)
{
    md_new_inline in = d_tests_markdown_text(_text);

    in.backend_handle()->inline_kind = _kind;

    return in;
}

/*
d_tests_markdown_wrapping
  An inline wrapping one text: emphasis, strong, a link.
*/
static md_new_inline
d_tests_markdown_wrapping(
    md_new_inline      _wrapper,
    const std::string& _text
)
{
    _wrapper->add_inline(d_tests_markdown_text(_text));

    return _wrapper;
}


/*
d_tests_markdown_render_markdown
  Tests the following:
  - text escapes what CommonMark gives a meaning, '<' and '&' included
  - a code span outnumbers its backticks, and is padded where it starts or
    ends with one, or with spaces around text (two spans side by side can't
    be written apart: their backticks read as one run)
  - a destination with a space goes in angle brackets; a title's quotes
    are escaped; an image's alternative text is escaped
  - a fence outnumbers the fences inside its code, and turns to tildes for
    an info string with a backtick
  - a thematic break is ***, which no paragraph can take as an underline;
    a quote prefixes every line; a hard break is two spaces
*/
void
d_tests_markdown_render_markdown(void)
{
    using namespace djinterp::markdown;

    md_document  doc;
    md_block     root = doc.root_block();
    md_new_block para = make_paragraph<md_backend>();

    root.add_block(d_tests_markdown_holding(make_heading<md_backend>(2),
                                            "Title <x> & 1.5"));
    para->add_inline(d_tests_markdown_text("See "));
    para->add_inline(make_code_span<md_backend>("a`b"));
    para->add_inline(d_tests_markdown_text(" and "));
    para->add_inline(d_tests_markdown_wrapping(
        make_link<md_backend>("https://example.com/a b", "T \"q\""),
        "site"));
    root.add_block(std::move(para));
    para = make_paragraph<md_backend>();
    para->add_inline(make_image<md_backend>("p.png", "a [b]"));
    para->add_inline(make_code_span<md_backend>("`x`"));
    para->add_inline(d_tests_markdown_text(" "));
    para->add_inline(make_code_span<md_backend>(" a "));
    root.add_block(std::move(para));
    root.add_block(make_thematic_break<md_backend>());
    root.add_block(make_fenced_code_block<md_backend>("x\n```\ny\n", "c"));
    root.add_block(make_fenced_code_block<md_backend>("z\n", "a`b"));

    md_new_block quote = make_block_quote<md_backend>();

    quote->add_block(d_tests_markdown_paragraph("q1"));
    quote->add_block(d_tests_markdown_paragraph("q2"));
    root.add_block(std::move(quote));
    para = d_tests_markdown_paragraph("a");
    para->add_inline(make_hard_break<md_backend>());
    para->add_inline(d_tests_markdown_text("b"));
    root.add_block(std::move(para));

    D_TESTS_TEXT_CHECK(d_tests_text_same(
        doc.render_to_markdown().c_str(),
        "## Title \\<x\\> \\& 1\\.5\n\n"
        "See ``a`b`` and "
        "[site](<https://example.com/a b> \"T \\\"q\\\"\")\n\n"
        "![a \\[b\\]](p.png)`` `x` `` `  a  `\n\n"
        "***\n\n"
        "````c\nx\n```\ny\n````\n\n"
        "~~~a`b\nz\n~~~\n\n"
        "> q1\n>\n> q2\n\n"
        "a  \nb\n\n"));

    return;
}


/*
d_tests_markdown_render_lists
  Tests the following:
  - an ordered list is numbered from its start, not bulleted
  - a nested list is indented to its item's content: two columns under a
    bullet, three under "1."
  - an item's second paragraph is indented, after a blank line, and stays
    in the item
  - a loose list puts a blank line between items; a task item has its box;
    the first item's bullet is every item's
  - a list in a quote is quoted line by line
  - a list right after another of its kind takes the other bullet or
    delimiter; an indented code block after a list is set off by cmark's
    "<!-- end list -->"
*/
void
d_tests_markdown_render_lists(void)
{
    using namespace djinterp::markdown;

    static const char* const numbers[] = { "one", "two" };
    static const char* const loose[]   = { "x", "y" };

    md_document  doc;
    md_block     root = doc.root_block();
    md_new_block item = d_tests_markdown_item("a");
    md_new_block list = make_list<md_backend>(false);

    root.add_block(d_tests_markdown_list(true, 3, numbers, 2u));
    item->add_block(d_tests_markdown_list(false, 1, numbers, 1u));
    list->add_block(std::move(item));
    root.add_block(std::move(list));
    item = d_tests_markdown_item("a");
    item->add_block(d_tests_markdown_list(true, 1, loose, 1u));
    list = make_list<md_backend>(true, 1);
    list->add_block(std::move(item));
    root.add_block(std::move(list));
    item = d_tests_markdown_item("p1");
    item->add_block(d_tests_markdown_paragraph("p2"));
    list = make_list<md_backend>(false);
    list->add_block(std::move(item));
    root.add_block(std::move(list));
    list = d_tests_markdown_list(false, 1, loose, 2u);
    list->set_loose(true);
    root.add_block(std::move(list));

    D_TESTS_TEXT_CHECK(d_tests_text_same(
        doc.render_to_markdown().c_str(),
        "3. one\n4. two\n\n"
        "- a\n  - one\n\n"
        "1. a\n   1. x\n\n"
        "- p1\n\n  p2\n\n"
        "* x\n\n* y\n\n"));

    md_document  other;
    md_new_block quote = make_block_quote<md_backend>();

    root = other.root_block();
    item = d_tests_markdown_item("done");
    item->set_task(true);
    item.backend_handle()->list_bullet = '+';
    list = make_list<md_backend>(false);
    list->add_block(std::move(item));
    item = d_tests_markdown_item("todo");
    item->set_task(false);
    list->add_block(std::move(item));
    root.add_block(std::move(list));
    quote->add_block(d_tests_markdown_list(false, 1, loose, 2u));
    root.add_block(std::move(quote));

    D_TESTS_TEXT_CHECK(d_tests_text_same(
        other.render_to_markdown().c_str(),
        "+ [x] done\n+ [ ] todo\n\n"
        "> - x\n> - y\n\n"));

    md_document  apart;
    md_new_block code = d_tests_markdown_block_of(
                           DMarkdownBlockKind::indented_code_block);

    root = apart.root_block();
    root.add_block(d_tests_markdown_list(false, 1, numbers, 1u));
    root.add_block(d_tests_markdown_list(false, 1, numbers + 1, 1u));
    root.add_block(d_tests_markdown_list(true, 1, loose, 1u));
    root.add_block(d_tests_markdown_list(true, 5, loose + 1, 1u));
    root.add_block(d_tests_markdown_list(false, 1, loose, 1u));
    code.backend_handle()->text = "k\n";
    root.add_block(std::move(code));

    D_TESTS_TEXT_CHECK(d_tests_text_same(
        apart.render_to_markdown().c_str(),
        "- one\n\n* two\n\n1. x\n\n5) y\n\n- x\n\n"
        "<!-- end list -->\n\n    k\n\n"));

    return;
}


/*
d_tests_markdown_html_table
  A table in HTML: header cells as th, body cells as td, each with its
column's alignment.
*/
static void
d_tests_markdown_html_table(void)
{
    using namespace djinterp::markdown;

    md_document  tables;
    md_new_block table = make_table<md_backend>(
                             std::vector<DMarkdownTableAlignment>(
                                 1u, DMarkdownTableAlignment::left));

    table->set_column_alignment(2u, DMarkdownTableAlignment::right);
    for (int part = 0; part < 2; ++part)
    {
        md_new_block section = (part == 0) ? make_table_header<md_backend>()
                                           : make_table_body<md_backend>();
        md_new_block row     = make_table_row<md_backend>();

        for (int column = 0; column < 3; ++column)
        {
            row->add_block(d_tests_markdown_holding(
                make_table_cell<md_backend>(),
                std::string(1, static_cast<char>(
                    ((part == 0) ? 'A' : '1') + column))));
        }
        section->add_block(std::move(row));
        table->add_block(std::move(section));
    }
    tables.root_block().add_block(std::move(table));

    D_TESTS_TEXT_CHECK(d_tests_text_same(
        tables.render_to_html().c_str(),
        "<table>\n<thead>\n<tr>\n"
        "<th align=\"left\">A</th>\n<th>B</th>\n<th align=\"right\">C</th>\n"
        "</tr>\n</thead>\n<tbody>\n<tr>\n"
        "<td align=\"left\">1</td>\n<td>2</td>\n<td align=\"right\">3</td>\n"
        "</tr>\n</tbody>\n</table>\n"));

    return;
}


/*
d_tests_markdown_html_inlines
  Inlines and leaf blocks in HTML.
*/
static void
d_tests_markdown_html_inlines(void)
{
    using namespace djinterp::markdown;

    md_document  inlines;
    md_new_block para = d_tests_markdown_paragraph("a<b>&");

    para->add_inline(d_tests_markdown_wrapping(make_emphasis<md_backend>(),
                                               "em"));
    para->add_inline(d_tests_markdown_wrapping(make_strong<md_backend>(),
                                               "st"));
    para->add_inline(make_code_span<md_backend>("<c>"));
    para->add_inline(d_tests_markdown_wrapping(
        make_link<md_backend>("u?a=1&b=2", "t"), "l"));
    para->add_inline(make_image<md_backend>("my pic.png", "al", "it"));
    para->add_inline(make_autolink<md_backend>("https://x.y"));
    para->add_inline(make_autolink<md_backend>("me@x.y"));
    para->inline_child_at(7).backend_handle()->inline_kind =
        DMarkdownInlineKind::autolink_email;
    para->add_inline(make_hard_break<md_backend>());
    para->add_inline(d_tests_markdown_inline_of(DMarkdownInlineKind::entity,
                                                "&copy;"));
    inlines.root_block().add_block(std::move(para));
    inlines.root_block().add_block(d_tests_markdown_holding(
        make_heading<md_backend>(3), "H"));
    inlines.root_block().add_block(
        make_fenced_code_block<md_backend>("<x>", "c"));
    inlines.root_block().add_block(make_thematic_break<md_backend>());

    md_new_block quote = make_block_quote<md_backend>();

    quote->add_block(d_tests_markdown_paragraph("q"));
    inlines.root_block().add_block(std::move(quote));

    D_TESTS_TEXT_CHECK(d_tests_text_same(
        inlines.render_to_html().c_str(),
        "<p>a&lt;b&gt;&amp;<em>em</em><strong>st</strong>"
        "<code>&lt;c&gt;</code><a href=\"u?a=1&amp;b=2\" title=\"t\">l</a>"
        "<img src=\"my%20pic.png\" alt=\"al\" title=\"it\" />"
        "<a href=\"https://x.y\">https://x.y</a>"
        "<a href=\"mailto:me@x.y\">me@x.y</a><br />\n&copy;</p>\n"
        "<h3>H</h3>\n"
        "<pre><code class=\"language-c\">&lt;x&gt;</code></pre>\n"
        "<hr />\n"
        "<blockquote>\n<p>q</p>\n</blockquote>\n"));

    return;
}


/*
d_tests_markdown_render_html
  Tests the following:
  - a tight list's items hold their text without a paragraph; a loose
    list's hold paragraphs, as does a list one of whose items holds two
    paragraphs; an ordered list says where it starts
  - a nested list follows its item's text on a new line; a task item has
    a disabled checkbox
  - a table: header cells as th, body cells as td, each with its column's
    alignment
  - inlines: text escaped, emphasis, code, links with titles, images,
    autolinks (mailto: for an address), hard breaks, entities as they are;
    a destination percent-encoded, as CommonMark normalises it
  - headings, code with its language, rules and quotes
*/
void
d_tests_markdown_render_html(void)
{
    using namespace djinterp::markdown;

    static const char* const two[] = { "a", "b" };

    md_document  doc;
    md_block     root = doc.root_block();
    md_new_block list = d_tests_markdown_list(false, 1, two, 2u);
    md_new_block item = d_tests_markdown_item("a");

    root.add_block(std::move(list));
    root.add_block(d_tests_markdown_list(true, 3, two + 1, 1u));
    list = d_tests_markdown_list(false, 1, two, 1u);
    list->set_loose(true);
    root.add_block(std::move(list));
    item->add_block(d_tests_markdown_list(false, 1, two + 1, 1u));
    item->set_task(true);
    list = make_list<md_backend>(false);
    list->add_block(std::move(item));
    root.add_block(std::move(list));

    item = d_tests_markdown_item("p1");
    item->add_block(d_tests_markdown_paragraph("p2"));
    list = make_list<md_backend>(false);
    list->add_block(std::move(item));
    root.add_block(std::move(list));

    D_TESTS_TEXT_CHECK(d_tests_text_same(
        doc.render_to_html().c_str(),
        "<ul>\n<li>a</li>\n<li>b</li>\n</ul>\n"
        "<ol start=\"3\">\n<li>b</li>\n</ol>\n"
        "<ul>\n<li>\n<p>a</p>\n</li>\n</ul>\n"
        "<ul>\n<li><input type=\"checkbox\" disabled checked /> a\n"
        "<ul>\n<li>b</li>\n</ul>\n</li>\n</ul>\n"
        "<ul>\n<li>\n<p>p1</p>\n<p>p2</p>\n</li>\n</ul>\n"));

    d_tests_markdown_html_table();
    d_tests_markdown_html_inlines();

    return;
}


/*
d_tests_markdown_render_plaintext
  Tests the following:
  - a heading's and a paragraph's text end their lines; emphasis and links
    give their text, an image its alternative text, an autolink its address
  - a table's cells are separated by tabs, and its rows end their lines
  - a code block's text keeps its own last newline, and gains no other
*/
void
d_tests_markdown_render_plaintext(void)
{
    using namespace djinterp::markdown;

    md_document  doc;
    md_block     root  = doc.root_block();
    md_new_block para  = d_tests_markdown_paragraph("a ");
    md_new_block table = make_table<md_backend>();
    md_new_block row   = make_table_row<md_backend>();

    root.add_block(d_tests_markdown_holding(make_heading<md_backend>(1),
                                            "Title"));
    para->add_inline(d_tests_markdown_wrapping(make_emphasis<md_backend>(),
                                               "b"));
    para->add_inline(d_tests_markdown_text(" "));
    para->add_inline(d_tests_markdown_wrapping(
        make_link<md_backend>("https://c/"), "c"));
    para->add_inline(make_image<md_backend>("p.png", "pic"));
    para->add_inline(make_autolink<md_backend>("https://x.y"));
    root.add_block(std::move(para));
    row->add_block(d_tests_markdown_holding(make_table_cell<md_backend>(),
                                            "A"));
    row->add_block(d_tests_markdown_holding(make_table_cell<md_backend>(),
                                            "B"));
    table->add_block(std::move(row));
    root.add_block(std::move(table));
    root.add_block(make_fenced_code_block<md_backend>("code\n"));

    D_TESTS_TEXT_CHECK(d_tests_text_same(
        doc.render_to_plaintext().c_str(),
        "Title\na b cpichttps://x.y\nA\tB\ncode\n"));

    return;
}


/*
d_tests_markdown_render_xml
  Tests the following:
  - the declaration, the DTD and the document's namespace
  - elements as cmark names them, indented two spaces a level; an empty
    one closes itself
  - text kept as it is and escaped; an autolink is a link holding its
    address; a list's type, start, delimiter and tightness; a code block's
    info; a heading's level
*/
void
d_tests_markdown_render_xml(void)
{
    using namespace djinterp::markdown;

    static const char* const one[] = { "x" };

    md_document  doc;
    md_block     root = doc.root_block();
    md_new_block para = d_tests_markdown_paragraph("Hi ");

    para->add_inline(d_tests_markdown_wrapping(make_emphasis<md_backend>(),
                                               "there"));
    para->add_inline(make_autolink<md_backend>("https://x.y"));
    root.add_block(std::move(para));
    root.add_block(d_tests_markdown_holding(make_heading<md_backend>(1),
                                            "T"));
    root.add_block(make_thematic_break<md_backend>());
    root.add_block(make_fenced_code_block<md_backend>("a<b\n", "c"));
    root.add_block(d_tests_markdown_list(true, 2, one, 1u));

    D_TESTS_TEXT_CHECK(d_tests_text_same(
        doc.render_to_xml().c_str(),
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
        "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
        "  <paragraph>\n"
        "    <text xml:space=\"preserve\">Hi </text>\n"
        "    <emph>\n"
        "      <text xml:space=\"preserve\">there</text>\n"
        "    </emph>\n"
        "    <link destination=\"https://x.y\" title=\"\">\n"
        "      <text xml:space=\"preserve\">https://x.y</text>\n"
        "    </link>\n"
        "  </paragraph>\n"
        "  <heading level=\"1\">\n"
        "    <text xml:space=\"preserve\">T</text>\n"
        "  </heading>\n"
        "  <thematic_break />\n"
        "  <code_block info=\"c\" xml:space=\"preserve\">a&lt;b\n"
        "</code_block>\n"
        "  <list type=\"ordered\" start=\"2\" delim=\"period\" "
        "tight=\"true\">\n"
        "    <item>\n"
        "      <paragraph>\n"
        "        <text xml:space=\"preserve\">x</text>\n"
        "      </paragraph>\n"
        "    </item>\n"
        "  </list>\n"
        "</document>\n"));

    return;
}

#endif  // C++11
