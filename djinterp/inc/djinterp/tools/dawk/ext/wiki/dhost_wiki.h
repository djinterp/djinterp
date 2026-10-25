/*******************************************************************************
* djinterp [tools]                                                  dhost_wiki.h
*
* The wiki extension: a wiki's pages, read and changed from awk.
*   A page is opened from its file, or made from text, and named by a handle,
* a number from 1; its dialect is its file's, Markdown as a GitHub wiki
* writes it or MediaWiki's, or one a program names. What a page holds is
* counted and read by index, from 1: its blocks, its sections -- the lead is
* section 0 -- its headings, its links, its templates, its regions, and the
* problems reading found. A change is kept only if the page outside it
* reads as it did and the change reads back as asked; a change refused gives
* -1, and wiki_error() says why. Indices are into the page as it is now: a
* change that adds or removes records moves those after it.
*   The writers make the text a page holds in either dialect, for a program
* that writes pages: a heading, a link, code, a list's item, a table, text
* escaped to read as itself, a region's markers. A dialect is named, or
* given as a page's handle.
*   Names: a page's name from its file's and back, whether two names name
* one page, a title's anchor; and the files of a directory, to find pages
* and what a program makes pages from.
*
* path:      /inc/djinterp/tools/dawk/ext/wiki/dhost_wiki.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.19
*                                                            revised: 2026.10.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Extension
         1.  d_host_wiki
2.  OPERATIONS
    ----------
    1.  The extension
*/

#ifndef DJINTERP_TOOLS_DAWK_EXT_WIKI_DHOST_WIKI_H
#define DJINTERP_TOOLS_DAWK_EXT_WIKI_DHOST_WIKI_H 1

// std
#include <stdbool.h>     // bool
// djinterp
#include "../../dinterp.h"  // d_awk_interp


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Extension
//------------------------------------------------------------------------------
// 1.1.1
// d_host_wiki
//   struct: the pages open, and the last failure of the wiki_* functions.
//   Layout is private.
struct d_host_wiki;


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    The extension
//------------------------------------------------------------------------------
// d_host_wiki_install adds these, as a program calls them; p is a page's
// handle, d a dialect's name -- markdown, mediawiki -- or a page's handle:
//
// pages
//   wiki_open(path[, dialect])     a page from its file: p, or -1
//   wiki_new(path[, dialect[, text]])
//                                  a page not yet on disk, of the text
//   wiki_save(p[, path])           1 written, 0 unchanged, -1 failed
//   wiki_close(p)                  1, or 0 for no page
//   wiki_error()                   why the last failure failed
//   wiki_path(p), wiki_dialect(p), wiki_text(p), wiki_changes(p)
//   wiki_page(p)                   the page's name, from its file's
//   wiki_problems(p)               how many; wiki_problem(p, i, field):
//                                  line, what
// blocks
//   wiki_blocks(p); wiki_block(p, i, field)
//                                  kind, line, end_line, depth, level,
//                                  marker, language, parent, text, content
// sections, the lead 0
//   wiki_sections(p)               how many headed sections
//   wiki_section(p, i, field)      title, level, anchor, line, end_line,
//                                  parent, path, subsections, text, body,
//                                  heading
//   wiki_find_section(p, name[, after])
//                                  the first after `after` whose title,
//                                  anchor or path -- titles apart by `/` --
//                                  is the name; 0 for none
//   wiki_set_body(p, i, text)      its own text, past its heading
//   wiki_set_section(p, i, text)   all of it, heading and subsections
//   wiki_rename_section(p, i, title)
//   wiki_insert_section(p, before, level, title[, body])
//                                  before a section, or after the last
//   wiki_add_section(p, parent, title[, body])
//                                  the last subsection of a section, a
//                                  level below it; of the page for 0
//   wiki_remove_section(p, i)
// headings, all, in templates and lists too
//   wiki_headings(p); wiki_heading(p, i, field)
//                                  level, title, anchor, line, top
// links
//   wiki_links(p); wiki_link(p, i, field)
//                                  form, kind, target, page, anchor, label,
//                                  line, column, text, embedded,
//                                  definition, template
//   wiki_find_link(p, name[, after])
//                                  the first after `after` to the page of
//                                  that name, or of that target; 0 for none
//   wiki_set_link(p, i, target[, label]); wiki_set_label(p, i, label)
//   wiki_unlink(p, i)              the link replaced by what it shows
// templates
//   wiki_templates(p); wiki_template(p, i, field)
//                                  name, kind, line, column, text, args,
//                                  parent
//   wiki_find_template(p, name[, after])
//   wiki_arg(p, t, key)            the value, a named one trimmed
//   wiki_has_arg(p, t, key); wiki_arg_key(p, t, n)
//   wiki_set_arg(p, t, key, value); wiki_remove_arg(p, t, key)
// regions
//   wiki_regions(p); wiki_region(p, i, field)
//                                  name, line, text
//   wiki_find_region(p, name)
//   wiki_set_region(p, name, text)
//   wiki_add_region(p, name, text[, section])
//                                  at a section's own text's end, or the
//                                  page's
//   wiki_remove_region(p, name[, keep])
// writing
//   wiki_heading_text(d, level, title)
//   wiki_link_text(d, page[, label[, anchor]])
//   wiki_url_text(d, url[, label]); wiki_image_text(d, source[, text])
//   wiki_code_text(d, code[, language]); wiki_inline_code(d, code)
//   wiki_item_text(d, text[, depth[, ordered]])
//   wiki_table_text(d, rows)       rows a line each, cells apart by tabs
//   wiki_escape(d, text); wiki_comment_text(d, text)
//   wiki_region_text(d, name, text)
//   wiki_toc(p[, least[, most]])   a list of links to its headings
// names and files
//   wiki_page_name(path[, d]); wiki_page_file(name[, d])
//   wiki_same_page(name, name[, d]); wiki_anchor(d, title)
//   wiki_anchors(d, titles)        the anchors of headings of these titles,
//                                  a line each, on a page in this order
//   wiki_page_key(name[, d])       the name as names compare, alike for two
//                                  that name one page
//   wiki_dialect_of(path)          markdown, mediawiki, or other
//   wiki_pages(directory[, deep])  the pages a directory holds, sorted, one
//                                  to a line; deep walks within it
//   wiki_files(directory, patterns[, deep])
//                                  the files whose names a pattern -- `*`,
//                                  `?`, [...] -- among the space-separated
//                                  matches
//   wiki_exists(path)              1 for a file, 2 for a directory
//   wiki_move(path, to)            a file renamed; never over another
struct d_host_wiki* d_host_wiki_new(void);
void                d_host_wiki_free(struct d_host_wiki* _host);
bool                d_host_wiki_install(struct d_host_wiki*  _host,
                                        struct d_awk_interp* _interp);


#endif  // DJINTERP_TOOLS_DAWK_EXT_WIKI_DHOST_WIKI_H
