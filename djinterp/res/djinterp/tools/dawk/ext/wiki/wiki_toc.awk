# wiki_toc.awk -- each page's table of contents, kept in a region.
#
#   dawk -x wiki -f wiki_toc.awk [-v least=N] [-v most=N] [-v name=toc]
#        [-v add=1] [-v dry=1] <page or directory>...
#
# A page that has a region of the name -- between <!-- dawk:begin toc -->
# and <!-- dawk:end toc -->, each on a line of its own -- has its text set
# to the page's table of contents: a list of links to its headings, from
# level `least`, the page's top level when not given, to level `most`, 6
# when not given, each a level deeper than the one before at most. A page
# whose contents are as they were is not written.
#
#   name  the region's name; toc when not given.
#   add   1 to add the region to a page that has none but has headings,
#         after its lead.
#   dry   1 to say which pages would change, and change nothing.
#
# Exit status: 0, or 2 when a page could not be read, changed or written.

BEGIN {
    if (name == "")
        name = "toc"
    status = 0
    for (i = 1; i < ARGC; i++)
        if (ARGV[i] != "")
            visit(ARGV[i])
    printf "%d pages: %d changed, %d as they were, %d without a %s region\n",
           pages, changed, same, without, name
    exit status
}


# visit(path): a page, or every page beneath a directory.
function visit(path,    found, m, names, i) {
    if (wiki_exists(path) == 2) {
        found = wiki_pages(path, 1)
        m = split(found, names, "\n")
        for (i = 1; i <= m; i++)
            if (names[i] != "")
                contents(names[i])
        return
    }
    contents(path)
}


# contents(path): one page's table of contents, set in its region.
function contents(path,    p, toc, ok, saved) {
    p = wiki_open(path)
    if (p < 0) {
        printf "%s: cannot be read: %s\n", path, wiki_error()
        status = 2
        return
    }
    pages++
    toc = (least != "") ? wiki_toc(p, least, (most != "") ? most : 6) \
                        : wiki_toc(p)
    if (most != "" && least == "")
        toc = wiki_toc(p, top_level(p), most)
    if (wiki_find_region(p, name) == 0) {
        if (!add || wiki_sections(p) == 0) {
            without++
            wiki_close(p)
            return
        }
        ok = wiki_add_region(p, name, toc, 0)
    } else if (wiki_region(p, wiki_find_region(p, name), "text") == toc) {
        same++
        wiki_close(p)
        return
    } else
        ok = wiki_set_region(p, name, toc)
    if (ok < 0) {
        printf "%s: cannot be changed: %s\n", path, wiki_error()
        status = 2
    } else if (dry) {
        printf "%s: would change\n", path
        changed++
    } else {
        saved = wiki_save(p)
        if (saved < 0) {
            printf "%s: cannot be written: %s\n", path, wiki_error()
            status = 2
        } else if (saved == 0)
            same++
        else
            changed++
    }
    wiki_close(p)
}


# top_level(p): the shallowest level of a page's sections, 1 for none.
function top_level(p,    s, level) {
    level = 7
    for (s = 1; s <= wiki_sections(p); s++)
        if (wiki_section(p, s, "level") < level)
            level = wiki_section(p, s, "level")
    return (level == 7) ? 1 : level
}
