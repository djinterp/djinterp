# wiki_survey.awk -- what a wiki's pages hold, and which of their links
# lead nowhere.
#
#   dawk -x wiki -f wiki_survey.awk [-v quiet=1] <page or directory>...
#
# Each page named, or found beneath a directory named, is read and described
# in a line: its dialect, its sections, its links by kind, its templates and
# its regions. Beneath each, what is broken:
#     a link to a page none of the pages read is -- a page in another of
#       MediaWiki's namespaces, Help:, Template:, is left alone
#     a link to an anchor its page does not have, this page's or another's
#     a link to a file or an image that is not there, read from the page's
#       directory or from the wiki's, the directory named
#     what reading the page found odd: a region left open, a template or a
#       table never closed
# After every page, the pages no other page links to: Home, the sidebar,
# the footer and MediaWiki's Main Page aside.
#
#   quiet   1 to print only what is broken.
#
# Exit status: 0 when nothing is broken, 1 when something is, 2 when a page
# could not be read.

BEGIN {
    status = 0
    for (i = 1; i < ARGC; i++)
        if (ARGV[i] != "")
            gather(ARGV[i])
    for (i = 1; i <= count; i++)
        survey(i)
    orphans()
    printf "%s: %s, %d broken; %s\n", plural(count, "page"),
           plural(links, "link"), broken, plural(lonely, "orphan")
    exit status
}


# gather(path): a page, or every page beneath a directory, opened and its
# anchors known.
function gather(path,    found, m, names, i) {
    if (wiki_exists(path) == 2) {
        found = wiki_pages(path, 1)
        m = split(found, names, "\n")
        for (i = 1; i <= m; i++)
            if (names[i] != "")
                gather_page(names[i], path)
        return
    }
    gather_page(path, dirname(path))
}


# gather_page(path, root): a page opened, by its key, with its anchors.
function gather_page(path, root,    p, key, h) {
    p = wiki_open(path)
    if (p < 0) {
        printf "%s: cannot be read: %s\n", path, wiki_error()
        status = 2
        return
    }
    count++
    page[count] = p
    file[count] = path
    top[count] = root
    key = wiki_page_key(wiki_page(p), p)
    keyed[key] = count
    pagekey[count] = key
    for (h = 1; h <= wiki_headings(p); h++)
        anchors[count, anchor_key(wiki_heading(p, h, "anchor"), p)] = 1
}


# survey(n): the n-th page's line, and what in it is broken.
function survey(n,    p, l, kind, line, s, kinds, problem, target, text) {
    p = page[n]
    pending = ""
    split("", kinds)
    for (l = 1; l <= wiki_links(p); l++) {
        kind = wiki_link(p, l, "kind")
        kinds[kind]++
        links++
        if (wiki_link(p, l, "form") == "definition")
            continue
        line = wiki_link(p, l, "line")
        target = wiki_link(p, l, "page")
        if (kind == "page")
            check_page(n, p, l, line, target)
        else if (kind == "anchor")
            check_anchor(n, n, p, l, line)
        else if ((kind == "file" || kind == "image") && target != "" &&
                 target !~ /^[A-Za-z][A-Za-z0-9+.-]*:/)
            check_file(n, p, line, target)
    }
    for (s = 1; s <= wiki_problems(p); s++)
        problem_at(wiki_problem(p, s, "line"), wiki_problem(p, s, "what"))
    if (!quiet) {
        text = sprintf("%s: %s, %s, %s%s", file[n], wiki_dialect(p),
                       plural(wiki_sections(p), "section"),
                       plural(wiki_links(p), "link"), kinds_text(kinds))
        if (wiki_templates(p) > 0)
            text = text ", " plural(wiki_templates(p), "template")
        if (wiki_regions(p) > 0)
            text = text ", " plural(wiki_regions(p), "region")
        print text
    } else if (pending != "")
        print file[n] ":"
    if (pending != "")
        printf "%s", pending
}


# check_page(n, p, l, line, target): a link to a page the wiki has, and to
# an anchor that page has.
function check_page(n, p, l, line, target,    key, to) {
    key = wiki_page_key(target, p)
    if (key in keyed) {
        to = keyed[key]
        linked[to]++
        if (to != n)
            linked_from_other[to]++
        check_anchor(n, to, p, l, line)
        return
    }
    # a namespace of MediaWiki's other than the wiki's own pages
    if (wiki_dialect(p) == "mediawiki" && target ~ /^[^:]+:/)
        return
    broke(line, sprintf("a link to %s: no such page", quoted(target)))
}


# check_anchor(n, to, p, l, line): a link's anchor, on the page it leads to.
function check_anchor(n, to, p, l, line,    anchor) {
    anchor = wiki_link(p, l, "anchor")
    if (anchor == "")
        return
    if (!((to, anchor_key(anchor, p)) in anchors))
        broke(line, sprintf("a link to %s: no such anchor %s",
                            quoted(((to == n) ? "" : wiki_page(page[to])) \
                                   "#" anchor),
                            (to == n) ? "here" : "there"))
}


# check_file(n, p, line, target): a file a link names, from the page's
# directory or the wiki's.
function check_file(n, p, line, target,    path) {
    path = target
    sub(/[?#].*$/, "", path)
    gsub(/%20/, " ", path)
    if (path ~ /^\//) {
        if (wiki_exists(top[n] path) != 0)
            return
    } else if (wiki_exists(dirname(file[n]) "/" path) != 0 ||
               wiki_exists(top[n] "/" path) != 0)
        return
    broke(line, sprintf("a link to %s: no such file", quoted(target)))
}


# orphans(): the pages no other page links to, and the special pages aside.
function orphans(    n, text) {
    for (n = 1; n <= count; n++) {
        if (tolower(pagekey[n]) ~ /^(home|sidebar|footer|header|main page)$/)
            continue
        if (linked_from_other[n] > 0)
            continue
        lonely++
        text = text sprintf("    %s\n", file[n])
    }
    if (lonely > 0) {
        print "pages no other page links to:"
        printf "%s", text
        status = (status == 2) ? 2 : 1
    }
}


# anchor_key(anchor, p): an anchor as a page's anchors compare: GitHub's in
# lower case, MediaWiki's with spaces and underscores alike.
function anchor_key(anchor, p) {
    if (wiki_dialect(p) == "mediawiki") {
        gsub(/[ _]+/, "_", anchor)
        sub(/^_/, "", anchor)
        sub(/_$/, "", anchor)
        return anchor
    }
    return tolower(anchor)
}


# broke(line, what), problem_at(line, what): what is broken, to print after
# the page's line.
function broke(line, what) {
    broken++
    status = (status == 2) ? 2 : 1
    pending = pending sprintf("    line %d: %s\n", line, what)
}

function problem_at(line, what) {
    broken++
    status = (status == 2) ? 2 : 1
    pending = pending sprintf("    line %d: %s\n", line, what)
}


# kinds_text(kinds): the links by kind, in a set order, as " (3 page, ...)".
function kinds_text(kinds,    order, m, i, text) {
    m = split("page anchor url mail file image category tag", order, " ")
    for (i = 1; i <= m; i++)
        if (kinds[order[i]] > 0)
            text = text sprintf("%s%d %s", (text == "") ? "" : ", ",
                                kinds[order[i]], order[i])
    return (text == "") ? "" : " (" text ")"
}


# dirname(path): the directory a path is in, "." for none.
function dirname(path) {
    if (path !~ /\//)
        return "."
    sub(/\/[^\/]*$/, "", path)
    return (path == "") ? "/" : path
}


# plural(n, word): a count and its word, "1 page", "2 pages".
function plural(n, word) {
    return (n + 0) " " word ((n == 1) ? "" : "s")
}


# quoted(text): text in double quotes.
function quoted(text) {
    return "\"" text "\""
}
