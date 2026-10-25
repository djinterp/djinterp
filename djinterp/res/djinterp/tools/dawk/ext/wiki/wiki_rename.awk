# wiki_rename.awk -- a page renamed, and every link to it with it.
#
#   dawk -x wiki -f wiki_rename.awk -v from="Old Name" -v to="New Name"
#        [-v dry=1] [-v labels=0] [-v redirect=1] <wiki directory>
#
# The page named `from`, among the pages beneath the directory, is moved to
# the file a page named `to` is kept in, beside it. Then every link to it,
# on every page, leads to `to` instead, at the anchor it led to, written as
# the link was written:
#     [[Old Name]]              [[New Name]]
#     [[Text|Old Name#usage]]   [[Text|New Name#usage]]
#     [text](Old-Name.md#x)     [text](New-Name.md#x)
#     [ref]: ../wiki/Old-Name   [ref]: ../wiki/New-Name
# and a link's own label that is the old name becomes the new one.
#
#   dry       1 to say what would change, and change nothing.
#   labels    0 to leave labels as they are.
#   redirect  1 to leave, for a MediaWiki page, a page at the old name that
#             redirects to the new.
#
# Exit status: 0 when the page is renamed; 1 when there is no such page, or
# a page has the new name already; 2 when a page could not be read, changed
# or written.

BEGIN {
    if (from == "" || to == "" || ARGC < 2) {
        print "usage: dawk -x wiki -f wiki_rename.awk -v from=NAME " \
              "-v to=NAME [-v dry=1] <wiki directory>"
        exit 2
    }
    root = ARGV[1]
    gather(root)
    source = find_page(from)
    if (source == 0) {
        printf "no page is named %s beneath %s\n", quoted(from), root
        exit 1
    }
    if (find_page(to) != 0 && find_page(to) != source) {
        printf "a page is named %s already: %s\n", quoted(to),
               file[find_page(to)]
        exit 1
    }
    status = 0
    moved = dirname(file[source]) "/" wiki_page_file(to, page[source])
    for (n = 1; n <= count; n++)
        relink(n)
    finish()
    exit status
}


# gather(directory): every page beneath it, open.
function gather(directory,    found, m, names, i, p) {
    found = wiki_pages(directory, 1)
    m = split(found, names, "\n")
    for (i = 1; i <= m; i++) {
        if (names[i] == "")
            continue
        p = wiki_open(names[i])
        if (p < 0) {
            printf "%s: cannot be read: %s\n", names[i], wiki_error()
            exit 2
        }
        count++
        page[count] = p
        file[count] = names[i]
    }
}


# find_page(name): the page of that name, 0 for none.
function find_page(name,    n) {
    for (n = 1; n <= count; n++)
        if (wiki_same_page(wiki_page(page[n]), name, page[n]))
            return n
    return 0
}


# relink(n): every link on the n-th page to the page renamed, renamed.
function relink(n,    p, l, target, label, raw, own) {
    p = page[n]
    for (l = 1; l <= wiki_links(p); l++) {
        if (wiki_link(p, l, "kind") != "page" ||
            !wiki_same_page(wiki_link(p, l, "page"), from, p))
            continue
        # a reference leads where its definition does, renamed by itself
        if (wiki_link(p, l, "form") == "reference" &&
            wiki_link(p, l, "definition") > 0)
            continue
        target = retarget(p, l)
        label = wiki_link(p, l, "label")
        raw = wiki_link(p, l, "text")
        own = (wiki_link(p, l, "form") == "wiki") ? (index(raw, "|") > 0) \
            : (wiki_link(p, l, "form") ~ /^(inline|reference)$/)
        if (labels != "0" && own && wiki_same_page(label, from, p))
            change(n, p, l, target, to)
        else
            change(n, p, l, target, "")
    }
}


# retarget(p, l): where the l-th link leads after the rename, written as it
# was: a page's name in [[...]], a path's last part elsewhere.
function retarget(p, l,    target, anchor, rest, directory, extension,
                  stem) {
    target = wiki_link(p, l, "target")
    anchor = wiki_link(p, l, "anchor")
    if (wiki_link(p, l, "form") == "wiki")
        return ((target ~ /^:/) ? ":" : "") to \
               ((anchor != "") ? "#" anchor : "")
    rest = ""
    if (match(target, /[?#].*$/)) {
        rest = substr(target, RSTART)
        target = substr(target, 1, RSTART - 1)
    }
    directory = ""
    if (match(target, /.*\//)) {
        directory = substr(target, 1, RLENGTH)
        target = substr(target, RLENGTH + 1)
    }
    extension = ""
    if (match(target, /\.(md|markdown|mediawiki|wiki)$/))
        extension = substr(target, RSTART)
    stem = wiki_page_file(to, p)
    sub(/\.[^.]*$/, "", stem)
    return directory stem extension rest
}


# change(n, p, l, target, label): one link changed, or said.
function change(n, p, l, target, label,    ok) {
    changes++
    if (dry) {
        printf "%s: line %d: %s -> %s%s\n", file[n],
               wiki_link(p, l, "line"), wiki_link(p, l, "target"), target,
               (label != "") ? " (" label ")" : ""
        return
    }
    ok = (label != "") ? wiki_set_link(p, l, target, label) \
                       : wiki_set_link(p, l, target)
    if (ok < 0) {
        printf "%s: line %d: cannot be changed: %s\n", file[n],
               wiki_link(p, l, "line"), wiki_error()
        status = 2
        return
    }
    touched[n] = 1
}


# finish(): the page moved, every page changed saved, and a redirect left.
function finish(    n, written, d) {
    if (dry) {
        printf "would move %s to %s, and change %s\n",
               file[page_number_of(from)], moved, plural(changes, "link")
        return
    }
    if (moved != file[source] && wiki_move(file[source], moved) < 0) {
        printf "%s: cannot be moved: %s\n", file[source], wiki_error()
        status = 2
        return
    }
    for (n = 1; n <= count; n++) {
        if (!touched[n] && n != source)
            continue
        if (wiki_save(page[n], (n == source) ? moved : file[n]) < 0) {
            printf "%s: cannot be written: %s\n", file[n], wiki_error()
            status = 2
        } else if (touched[n])
            written++
    }
    d = wiki_dialect(page[source])
    if (redirect && d == "mediawiki" && moved != file[source]) {
        n = wiki_new(file[source], d,
                     "#REDIRECT " wiki_link_text(d, to) "\n")
        if (n < 0 || wiki_save(n) < 0) {
            printf "%s: the redirect cannot be written: %s\n",
                   file[source], wiki_error()
            status = 2
        }
    }
    printf "moved %s to %s; %s changed, on %s\n", file[source], moved,
           plural(changes, "link"), plural(written, "page")
}


# page_number_of(name): the page of the name, for what dry run says.
function page_number_of(name) {
    return find_page(name)
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
