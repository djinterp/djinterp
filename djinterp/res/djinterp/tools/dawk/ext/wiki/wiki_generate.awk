# wiki_generate.awk -- a wiki of a source tree's API, made from the comments
# in its headers.
#
#   dawk -x wiki -f wiki_generate.awk -v out=DIRECTORY
#        [-v dialect=markdown|mediawiki] [-v project=NAME] [-v strip=PREFIX]
#        [-v patterns="*.h *.hpp"] [-v only=PREFIX] [-v lines=N]
#        [-v undocumented=0] [-v dry=1] <source directory>...
#
# A page for each header beneath the directories named, and for each module:
#     a header's page: its path; what the comment that begins it says, its
#       paragraphs, lists and indented text; its table of contents, when a
#       comment holds one; and each declaration with the comment just above
#       it -- a Doxygen comment's brief, details, parameters, return and
#       notes, or the // comments a declaration follows -- each under the
#       heading of the part of the file it is in, as //==== and //---- rules
#       mark parts; a class's members under the class; and the headers it
#       includes from the tree, as links
#     a module's page, a module being the first directory of a header's path
#       from the root: each of its headers with the first sentence of what
#       the header's comment says
#     Home, with the modules; and _Sidebar, with Home and the modules
# What is made is kept in each page between <!-- dawk:begin api --> and
# <!-- dawk:end api -->. A page that is there already is changed only there,
# so what is written outside the region stays; a page made new has the
# region alone. A page whose text is as it was is not written.
#
#   out           where the pages go: the wiki's directory.
#   dialect       markdown, a GitHub wiki's, when not given; or mediawiki.
#   project       the name Home gives the wiki: the last part of the first
#                 directory named when not given.
#   strip         what is taken from the front of each header's path to name
#                 it: the directory named, and the `/` after it, when not
#                 given.
#   patterns      the headers' names: "*.h *.hh *.hpp *.hxx" when not given.
#   only          the headers whose names, as stripped, begin so.
#   lines         the most lines of code shown for one declaration: 40.
#   width         the most bytes of a line of code shown: 160.
#   undocumented  0 to leave out declarations with no comment.
#   dry           1 to say what would be written, and write nothing.
#
# Exit status: 0, or 2 when a header could not be read or a page written.

BEGIN {
    if (out == "" || ARGC < 2) {
        print "usage: dawk -x wiki -f wiki_generate.awk -v out=DIRECTORY " \
              "[-v dialect=markdown] <source directory>..."
        exit 2
    }
    if (dialect == "")
        dialect = "markdown"
    if (patterns == "")
        patterns = "*.h *.hh *.hpp *.hxx"
    if (lines == "")
        lines = 40
    if (width == "")
        width = 160
    # the words before a declaration's type, a banner's fields, and a line
    # that only lists names
    specifiers = "^(static|inline|extern|constexpr|const|virtual|explicit|" \
                 "friend|D_[A-Z_]*API)[ \t]"
    fields = "^(path|link\\(s\\)|links?|author\\(s\\)|authors?|created|" \
             "revised|copyright|license|version):"
    name_list = "^[A-Za-z_][A-Za-z0-9_:~]*" \
                "(,? (and )?[A-Za-z_][A-Za-z0-9_:~]*)*,?$"
    status = 0
    first_root = ARGV[1]
    sub(/\/+$/, "", first_root)
    if (project == "") {
        project = first_root
        sub(/.*\//, "", project)
    }

    # every header, named as stripped, sorted as found
    for (a = 1; a < ARGC; a++) {
        root = ARGV[a]
        sub(/\/+$/, "", root)
        cut = (strip != "") ? strip : root "/"
        found = wiki_files(root, patterns, 1)
        m = split(found, names, "\n")
        for (i = 1; i <= m; i++) {
            if (names[i] == "")
                continue
            rel = names[i]
            if (index(rel, cut) == 1)
                rel = substr(rel, length(cut) + 1)
            if (only != "" && index(rel, only) != 1)
                continue
            headers++
            header_path[headers] = names[i]
            header_rel[headers] = rel
            header_page[headers] = page_of(rel)
            by_rel[rel] = headers
        }
    }

    for (h = 1; h <= headers; h++)
        header(h)
    modules_pages()
    home()
    sidebar()
    printf "%d headers, %d modules: %d pages written, %d as they were\n",
           headers, module_count, written, same
    exit status
}


#==============================================================================
# names
#==============================================================================


# page_of(rel): the page a header's path names: its `/`s dashes.
function page_of(rel,    name) {
    name = rel
    gsub(/\//, "-", name)
    return name
}

# module_of(rel): the first directory of a path, or the project's name.
function module_of(rel) {
    if (rel !~ /\//)
        return project
    sub(/\/.*$/, "", rel)
    return rel
}

# module_page(module): the page a module is described on.
function module_page(module) {
    return "Module " module
}


#==============================================================================
# reading a header
#==============================================================================


# header(h): the h-th header read, and its page written.
function header(h,    path, line, n, got) {
    path = header_path[h]
    n = 0
    split("", text)
    while ((got = (getline line < path)) > 0) {
        sub(/\r$/, "", line)
        text[++n] = line
    }
    close(path)
    if (got < 0) {
        printf "%s: cannot be read\n", path
        status = 2
        return
    }
    code_language = (header_rel[h] ~ /\.h$/) ? "c" : "cpp"
    base_name = header_rel[h]
    sub(/.*\//, "", base_name)
    parse(n)
    module = module_of(header_rel[h])
    if (!(module in module_size)) {
        module_count++
        module_name[module_count] = module
    }
    module_size[module]++
    module_header[module, module_size[module]] = h
    header_summary[h] = summary()
    write_page(header_page[h], header_body(h))
}


# parse(n): the n lines of `text`, read into the header's parts: its
# banner, its contents, its entries, its includes.
function parse(n,    i, t, kind) {
    reset()
    for (i = 1; i <= n; i++) {
        t = trim(text[i])

        # a block comment: the banner, the contents, a Doxygen comment
        if (t ~ /^\/\*/) {
            i = block_comment(i, n)
            continue
        }
        # a line comment, a rule, a part's heading
        if (t ~ /^\/\//) {
            line_comment(t)
            continue
        }
        if (t == "") {
            blank()
            continue
        }
        seen_code = 1
        if (t ~ /^#/) {
            i = directive(t, i, n)
            continue
        }
        i = code(i, n)
    }
    finish_declaration()
}


# reset(): nothing read yet.
function reset() {
    seen_code = 0
    banner_done = 0
    split("", desc_kind)
    split("", desc_text)
    desc_n = 0
    brief = ""
    split("", toc_depth)
    split("", toc_text)
    toc_n = 0
    split("", entry_kind)
    split("", entry_title)
    split("", entry_doc)
    split("", entry_code)
    split("", entry_level)
    split("", entry_doxygen)
    split("", entry_label)
    split("", entry_lines)
    split("", entry_count)
    split("", entry_more)
    entry_n = 0
    split("", include_path)
    include_n = 0
    comment_n = 0
    split("", comment_line)
    run_open = 0
    run_documented = 0
    in_comment = 0
    rule_before = ""
    doxygen = ""
    pending = ""
    pending_lines = 0
    pending_depth = 0
    guard = ""
    scope_n = 0
    split("", scope_kind)
    split("", scope_name)
    split("", scope_depth)
    depth = 0
    group = 0
}


# block_comment(i, n): the block comment from line i; gives its last line.
function block_comment(i, n,    body, first, j, t, end, m, rows, line) {
    body = ""
    first = text[i]
    end = i
    # to the line that closes it
    for (j = i; j <= n; j++) {
        body = body ((j > i) ? "\n" : "") text[j]
        if ((j == i && index(substr(trim(text[j]), 3), "*/") > 0) ||
            (j > i && index(text[j], "*/") > 0)) {
            end = j
            break
        }
        end = j
    }
    t = trim(first)
    if (t ~ /^\/\*[*!][^*\/]/ || t ~ /^\/\*[*!]$/) {
        comment_n = 0
        doxygen = body
        return end
    }
    if (!seen_code && !banner_done) {
        banner(body)
        banner_done = 1
        return end
    }
    if (toc_block(body))
        return end
    # a C comment above a declaration is its comment, a line at a time
    abandon()
    run_open = 0
    comment_n = 0
    m = split(body, rows, "\n")
    for (j = 1; j <= m; j++) {
        line = rows[j]
        sub(/^[ \t]*\/\*+/, "", line)
        sub(/\*+\/[ \t]*$/, "", line)
        sub(/^[ \t]*\*[ \t]?/, "", line)
        if (trim(line) ~ /^[-=*]*$/ && (j == 1 || j == m))
            line = trim(line)
        sub(/^[ \t]*-+[ \t]+/, "", line)
        sub(/[ \t]+-+[ \t]*$/, "", line)
        if (trim(line) != "" || comment_n > 0)
            comment_line[++comment_n] = line
    }
    while (comment_n > 0 && trim(comment_line[comment_n]) == "")
        comment_n--
    return end
}


# line_comment(t): a // comment: kept for the declaration after it, or a
# rule, or the heading a rule marks.
function line_comment(t,    c, bare) {
    abandon()
    run_open = 0
    # a part's title between runs of slashes: ///  I.  TITLE ///
    if (t ~ /^\/\/\/+[^\/].*[A-Za-z].*\/\/\/+[ \t]*$/) {
        c = t
        sub(/^\/+/, "", c)
        sub(/\/+[ \t]*$/, "", c)
        part(1, c)
        comment_n = 0
        return
    }
    if (t ~ /^\/\/\/\/\/\/\/\/+[ \t]*$/) {
        rule_before = "="
        comment_n = 0
        return
    }
    c = t
    sub(/^\/\/[\/!]?/, "", c)
    sub(/^ /, "", c)
    bare = trim(c)
    # a rule of = or -
    if (bare ~ /^=+$/ && length(bare) >= 8) {
        if (rule_before == "=" && comment_n == 1)
            part(1, comment_line[1])
        rule_before = "="
        comment_n = 0
        return
    }
    if (bare ~ /^-+$/ && length(bare) >= 8) {
        if (comment_n >= 1)
            part(2, comment_line[comment_n])
        rule_before = "-"
        comment_n = 0
        return
    }
    if (rule_before == "=" && comment_n == 0)
        rule_before = "="
    else if (comment_n == 0)
        rule_before = ""
    # a line comment at the top, before any code: a banner of its own
    comment_line[++comment_n] = c
    doxygen_line = (t ~ /^\/\/[\/!]/)
    if (doxygen_line)
        comment_doxygen = 1
}


# blank(): a blank line ends a declaration begun, a comment's hold, and a
# run of declarations a comment says what they are; a run that none does
# goes on.
function blank() {
    if (run_documented)
        run_open = 0
    if (!seen_code && !banner_done && comment_n > 0) {
        banner(lines_text(comment_line, comment_n))
        banner_done = 1
    }
    abandon()
    comment_n = 0
    comment_doxygen = 0
    doxygen = ""
    rule_before = ""
}


# part(level, title): a part of the file, as a rule marks it.
function part(level, title) {
    title = trim(title)
    sub(/^([0-9]+(\.[0-9]+)*|[IVXLC]+)\.?[ \t]+/, "", title)
    if (title == "" || scope_n > 0)
        return
    entry_n++
    entry_kind[entry_n] = "part"
    entry_level[entry_n] = level
    entry_title[entry_n] = title
    group = level
    run_open = 0
}


# directive(t, i, n): an #include kept, a #define a declaration, the rest
# passed over, with the lines it is continued on.  Gives its last line.
function directive(t, i, n,    name, last) {
    last = i
    while (last < n && text[last] ~ /\\[ \t]*$/)
        last++
    if (t ~ /^#[ \t]*include[ \t]*"/) {
        name = t
        sub(/^#[ \t]*include[ \t]*"/, "", name)
        sub(/".*$/, "", name)
        include_path[++include_n] = name
        return last
    }
    if (t ~ /^#[ \t]*include[ \t]*</) {
        name = t
        sub(/^#[ \t]*include[ \t]*</, "", name)
        sub(/>.*$/, "", name)
        include_path[++include_n] = "<" name
        return last
    }
    if (t ~ /^#[ \t]*ifndef[ \t]/) {
        guard = t
        sub(/^#[ \t]*ifndef[ \t]+/, "", guard)
        sub(/[ \t].*$/, "", guard)
        return last
    }
    if (t ~ /^#[ \t]*define[ \t]/) {
        name = t
        sub(/^#[ \t]*define[ \t]+/, "", name)
        sub(/[^A-Za-z0-9_].*$/, "", name)
        # the guard's own define is not a declaration
        if (name == guard) {
            guard = ""
            comment_n = 0
            return last
        }
        guard = ""
        if (scope_n > 0 && scope_kind[scope_n] == "skip")
            return last
        entry(name, "macro", macro_text(i, last))
        return last
    }
    guard = ""
    return last
}


# macro_text(i, last): a #define, from line i to `last`, as shown: to its
# parameters' `)`, and `...` for the rest.
function macro_text(i, last,    t, j, open, line) {
    t = trim(text[i])
    open = parens(t)
    # the lines its parameters go on to
    for (j = i + 1; j <= last && open > 0; j++) {
        line = trim(text[j])
        sub(/[ \t]*\\$/, "", t)
        t = t " " line
        open += parens(line)
    }
    if (j <= last || t ~ /\\$/)
        sub(/[ \t]*\\$/, "", t)
    if (j <= last)
        t = t " ..."
    return t
}


# parens(t): how many more `(` than `)` a line has.
function parens(t,    copy) {
    copy = t
    return gsub(/\(/, "", copy) - gsub(/\)/, "", t)
}


#==============================================================================
# reading code
#==============================================================================


# code(i, n): code from line i: a declaration, gathered to its `;` or `{`,
# or the body it opens, read or passed over.  Gives the last line taken.
function code(i, n,    t, c, opens) {
    t = text[i]
    c = stripped(t)
    # within a body passed over, braces only
    if (scope_n > 0 && scope_kind[scope_n] == "skip") {
        depth += brace_count(c)
        if (depth <= scope_depth[scope_n])
            scope_n--
        comment_n = 0
        return i
    }
    # a close of a namespace's, an extern's or a class's body
    if (pending == "" && trim(c) ~ /^}/) {
        depth += brace_count(c)
        while (scope_n > 0 && depth <= scope_depth[scope_n])
            scope_n--
        comment_n = 0
        return i
    }
    pending = pending ((pending == "") ? "" : "\n") t
    pending_code = pending_code " " c
    pending_lines++
    if (pending_lines == 1)
        pending_start = i
    # a declaration ends at `;`, or opens a body at `{`
    opens = index(c, "{")
    if (opens > 0 && (index(c, ";") == 0 || index(c, ";") > opens))
        return body(i, n, c)
    if (c ~ /;[ \t]*$/ || c ~ /;[ \t]*\}?[ \t]*$/)
        finish_declaration()
    return i
}


# body(i, n): a declaration whose body opens on line i: a namespace's or
# an extern's read on; a class's read for its members; a struct's, enum's
# or union's shown whole; a function's passed over.
function body(i, n, first,    head, kind, name, j, c, start, shown) {
    head = trim(pending_code)
    kind = head_kind(head)
    start = depth
    depth += brace_count(first)
    if (kind == "namespace" || kind == "extern") {
        push(kind, "", start)
        clear_pending()
        comment_n = 0
        return i
    }
    if (kind == "class") {
        name = declared_name(head, kind)
        entry(name, "class", head_text(pending))
        push("class", name, start)
        clear_pending()
        run_open = 0
        return i
    }
    if (kind == "struct" || kind == "enum" || kind == "union") {
        shown = pending
        # to the line its body closes on, and the `;` after
        for (j = i + 1; j <= n && depth > start; j++) {
            c = stripped(text[j])
            depth += brace_count(c)
            shown = shown "\n" text[j]
        }
        name = declared_name(head, kind)
        entry(name, kind, shown)
        clear_pending()
        return j - 1
    }
    # a function's body, passed over: its declaration is its head
    name = declared_name(head, "function")
    if (name != "")
        entry(name, "function", head_text(pending))
    clear_pending()
    if (depth > start)
        push("skip", "", start)
    return i
}


# finish_declaration(): a declaration gathered to its `;`, kept.
function finish_declaration(    head, kind, name) {
    if (pending == "")
        return
    head = trim(pending_code)
    kind = head_kind(head)
    name = declared_name(head, kind)
    if (name != "" && kind != "namespace" && kind != "extern")
        entry(name, kind, pending)
    clear_pending()
}


# abandon(): a declaration not ended before a blank line is not one.
function abandon() {
    if (pending != "")
        clear_pending()
}


function clear_pending() {
    pending = ""
    pending_code = ""
    pending_lines = 0
}


# push(kind, name, depth): a body read within.
function push(kind, name, at) {
    scope_n++
    scope_kind[scope_n] = kind
    scope_name[scope_n] = name
    scope_depth[scope_n] = at
}


# head_kind(head): what a declaration declares, by its first words, and
# a `(` before any body: a function returns a struct as well as a type.
function head_kind(head,    h, paren, brace) {
    h = untemplated(head)
    while (h ~ specifiers && h !~ /^extern[ \t]*"/)
        sub(/^[A-Za-z_]+[ \t]+/, "", h)
    if (h ~ /^namespace([ \t]|$)/ || h ~ /^NS_[A-Z_]+/)
        return "namespace"
    if (h ~ /^extern[ \t]*"/)
        return "extern"
    if (h ~ /^typedef[ \t]/)
        return "typedef"
    if (h ~ /^using[ \t]/)
        return "alias"
    paren = index(h, "(")
    brace = index(h, "{")
    if (paren > 0 && (brace == 0 || paren < brace))
        return "function"
    if (h ~ /^class[ \t]/)
        return "class"
    if (h ~ /^struct[ \t]/)
        return "struct"
    if (h ~ /^union[ \t]/)
        return "union"
    if (h ~ /^enum[ \t]/)
        return "enum"
    return "variable"
}


# declared_name(head, kind): the name a declaration of a kind declares.
function declared_name(head, kind,    h, name, at) {
    h = untemplated(head)
    gsub(/[ \t]+/, " ", h)
    if (kind == "function") {
        at = index(h, "(")
        name = substr(h, 1, at - 1)
        sub(/ +$/, "", name)
        if (name ~ /operator/) {
            sub(/.*operator/, "operator", name)
            return name
        }
        sub(/.*[^A-Za-z0-9_:~]/, "", name)
        return name
    }
    if (h ~ /\(\* ?[A-Za-z_][A-Za-z0-9_]*\)/ && h ~ /typedef/) {
        match(h, /\(\* ?[A-Za-z_][A-Za-z0-9_]*\)/)
        name = substr(h, RSTART, RLENGTH)
        gsub(/[(* )]/, "", name)
        return name
    }
    if (h ~ /^(typedef )?(struct|union|enum|class)( class| struct)? /) {
        if (h ~ /^typedef / && h ~ /}/) {
            name = h
            sub(/.*}[ ]*/, "", name)
            sub(/[^A-Za-z0-9_].*$/, "", name)
            return name
        }
        name = h
        sub(/^(typedef )?(struct|union|enum|class)( class| struct)? /, "", name)
        sub(/^(alignas\([^)]*\) |D_[A-Z_]+ )*/, "", name)
        sub(/[^A-Za-z0-9_].*$/, "", name)
        return name
    }
    if (h ~ /^using [A-Za-z_]/) {
        name = h
        sub(/^using /, "", name)
        sub(/[^A-Za-z0-9_].*$/, "", name)
        return name
    }
    at = index(h, "(")
    if (at > 0 && h !~ /^typedef /) {
        name = substr(h, 1, at - 1)
        sub(/ +$/, "", name)
        if (name ~ /operator[^A-Za-z0-9_ ]*$/ || name ~ /operator [^ ]*$/) {
            sub(/.*operator/, "operator", name)
            return name
        }
        sub(/.*[^A-Za-z0-9_:~]/, "", name)
        return name
    }
    name = h
    sub(/[ ]*[=;\[{].*$/, "", name)
    sub(/ +$/, "", name)
    sub(/.*[^A-Za-z0-9_]/, "", name)
    return name
}


# untemplated(head): a declaration without the template<...> before it,
# its angle brackets matched.
function untemplated(head,    at, n, level, c) {
    head = trim(head)
    while (head ~ /^template[ \t]*</) {
        at = index(head, "<")
        n = length(head)
        level = 0
        for (; at <= n; at++) {
            c = substr(head, at, 1)
            if (c == "<")
                level++
            else if (c == ">" && --level == 0)
                break
        }
        head = trim(substr(head, at + 1))
    }
    return head
}


# head_text(text): a declaration's text to its body's `{`.
function head_text(text,    at) {
    at = index(text, "{")
    if (at > 0)
        text = substr(text, 1, at - 1)
    sub(/[ \t\n]+$/, "", text)
    return text
}


#==============================================================================
# what comments say
#==============================================================================


# entry(name, kind, shown): a declaration kept, with the comment above it.
function entry(name, kind, shown,    title) {
    if (name == "" || name == "static_assert") {
        comment_n = 0
        doxygen = ""
        return
    }
    if (scope_n > 0 && scope_kind[scope_n] == "class")
        title = scope_name[scope_n] "::" name
    else
        title = name
    # a run of declarations, under one comment or none, is one entry
    if (comment_n == 0 && doxygen == "" && run_open && entry_n > 0 &&
        entry_kind[entry_n] == "declaration") {
        if (!has_name(entry_title[entry_n], title)) {
            entry_title[entry_n] = entry_title[entry_n] ", " title
            entry_count[entry_n]++
        }
        if (entry_label[entry_n] != kind)
            entry_label[entry_n] = "declaration"
        add_code(entry_n, shown)
        return
    }
    if (doxygen == "" && comment_n == 0 && undocumented == "0")
        return
    entry_n++
    entry_kind[entry_n] = "declaration"
    entry_title[entry_n] = title
    entry_label[entry_n] = kind
    entry_count[entry_n] = 1
    entry_code[entry_n] = ""
    entry_lines[entry_n] = 0
    entry_more[entry_n] = 0
    add_code(entry_n, shown)
    entry_level[entry_n] = group + ((scope_n > 0 &&
                                     scope_kind[scope_n] == "class") ? 1 : 0)
    if (doxygen != "") {
        entry_doc[entry_n] = doxygen
        entry_doxygen[entry_n] = 1
    } else {
        entry_doc[entry_n] = lines_text(comment_line, comment_n)
        entry_doxygen[entry_n] = comment_doxygen
    }
    run_open = 1
    run_documented = (entry_doc[entry_n] != "")
    doxygen = ""
    comment_n = 0
    comment_doxygen = 0
}


# add_code(e, shown): a declaration's code added to an entry's, to `lines`
# lines of `width` bytes; the lines past them counted.
function add_code(e, shown,    m, rows, i, row) {
    m = split(shown, rows, "\n")
    for (i = 1; i <= m; i++) {
        if (entry_lines[e] >= lines + 0) {
            entry_more[e]++
            continue
        }
        row = rows[i]
        if (length(row) > width + 0)
            row = substr(row, 1, width + 0) " ..."
        entry_code[e] = entry_code[e] ((entry_lines[e] > 0) ? "\n" : "") row
        entry_lines[e]++
    }
}


# has_name(names, name): whether a list of names, apart by ", ", has one.
function has_name(names, name,    m, list, i) {
    m = split(names, list, ", ")
    for (i = 1; i <= m; i++)
        if (list[i] == name)
            return 1
    return 0
}


# banner(body): what the comment that begins a header says: its title
# line, its tag, its fields, and its paragraphs, lists and indented text.
function banner(body,    m, rows, i, line, t, para, indent, title_seen) {
    m = split(body, rows, "\n")
    para = ""
    for (i = 1; i <= m; i++) {
        line = rows[i]
        sub(/^[ \t]*\/\*+!?/, "", line)
        sub(/\*+\/[ \t]*$/, "", line)
        sub(/^[ \t]*\* ?/, "", line)
        sub(/^\/\/[\/!]? ?/, "", line)
        t = trim(line)
        # rules of stars, and the line that only titles the file
        if (t ~ /^[*=\/-]*$/ && t != "") {
            continue
        }
        if (!title_seen && t != "") {
            title_seen = 1
            if (t ~ /\[[A-Za-z0-9_]+\]/ ||
                t ~ /[ \t][A-Za-z0-9_.-]+\.[A-Za-z]+$/)
                continue
        }
        # a field, and what follows it, is not described
        if (t ~ fields || t ~ /^@(file|author|date|copyright|version)/)
            break
        if (t ~ /^@brief[ \t]/) {
            brief = t
            sub(/^@brief[ \t]+/, "", brief)
            continue
        }
        if (t == "") {
            para = flush_paragraph(para)
            continue
        }
        indent = match(line, /[^ \t]/) - 1
        # a list's item, and text set in as code
        if (t ~ /^([-*+]|[0-9]+[.)])[ \t]/) {
            para = flush_paragraph(para)
            desc_n++
            desc_kind[desc_n] = "item"
            desc_text[desc_n] = t
            sub(/^([-*+]|[0-9]+[.)])[ \t]+/, "", desc_text[desc_n])
            continue
        }
        if (indent >= 4 || is_art(t)) {
            para = flush_paragraph(para)
            if (desc_n > 0 && desc_kind[desc_n] == "code")
                desc_text[desc_n] = desc_text[desc_n] "\n" line
            else {
                desc_n++
                desc_kind[desc_n] = "code"
                desc_text[desc_n] = line
            }
            continue
        }
        if (indent >= 1 && indent <= 3)
            para = flush_paragraph(para)
        # a first line that names the file before saying what it is
        if (desc_n == 0 && para == "" &&
            index(t, base_name " ") == 1 &&
            match(t, /^[^ ]+ +(--|-|:) +/))
            t = substr(t, RLENGTH + 1)
        para = (para == "") ? t : para " " t
    }
    flush_paragraph(para)
    trim_code_blocks()
}


# is_art(t): whether a line is laid out by its columns, not prose: three
# spaces within it, or a rule of -, = and + with | between.
function is_art(t) {
    return (t ~ /[^ ]   +[^ ]/ || t ~ /^[-=+|]+$/ && length(t) >= 4 ||
            t ~ /[-=]+\+[-=]+/ || t ~ /^\|/ || t ~ / \| .* \| /)
}


# flush_paragraph(para): a paragraph kept; gives "".
function flush_paragraph(para) {
    if (para != "") {
        desc_n++
        desc_kind[desc_n] = "paragraph"
        desc_text[desc_n] = para
    }
    return ""
}


# trim_code_blocks(): the indent the lines of each block of code share,
# taken away.
function trim_code_blocks(    i, m, rows, j, least, k) {
    for (i = 1; i <= desc_n; i++) {
        if (desc_kind[i] != "code")
            continue
        m = split(desc_text[i], rows, "\n")
        least = 1000
        for (j = 1; j <= m; j++)
            if (rows[j] ~ /[^ \t]/) {
                k = match(rows[j], /[^ \t]/) - 1
                least = (k < least) ? k : least
            }
        desc_text[i] = ""
        for (j = 1; j <= m; j++)
            desc_text[i] = desc_text[i] ((j > 1) ? "\n" : "") \
                           substr(rows[j], least + 1)
    }
}


# toc_block(body): a comment that is a table of contents, kept; whether it
# was one.
function toc_block(body,    m, rows, i, line, t, k, widths, wn, w, found) {
    m = split(body, rows, "\n")
    found = 0
    wn = 0
    split("", widths)
    for (i = 1; i <= m; i++) {
        line = rows[i]
        sub(/^[ \t]*\/\*+/, "", line)
        sub(/\*+\/[ \t]*$/, "", line)
        t = trim(line)
        if (!found) {
            if (toupper(t) ~ /^(TABLE OF )?CONTENTS$/)
                found = 1
            else if (t != "")
                return 0
            continue
        }
        if (t == "" || t ~ /^[=-]+$/)
            continue
        # an entry's depth is where its text begins, past its number
        k = match(line, /[^ \t]/) - 1
        if (match(t, /^([0-9]+(\.[0-9]+)*|[IVXLC]+)\.?[ \t]+/))
            k += RLENGTH
        toc_n++
        toc_text[toc_n] = t
        sub(/^([0-9]+(\.[0-9]+)*|[IVXLC]+)\.?[ \t]+/, "", toc_text[toc_n])
        toc_depth[toc_n] = k
        if (!(k in widths)) {
            widths[k] = 1
            wn++
        }
    }
    # each depth by its rank among the indents
    for (i = 1; i <= toc_n; i++) {
        k = 1
        for (w in widths)
            if (w + 0 < toc_depth[i] + 0)
                k++
        toc_rank[i] = k
    }
    return found
}


# lines_text(rows, n): the lines of a comment, joined.
function lines_text(rows, n,    i, s) {
    s = ""
    for (i = 1; i <= n; i++)
        s = s ((i > 1) ? "\n" : "") rows[i]
    return s
}


# summary(): the first sentence of what the header's comment says.
function summary(    s, at) {
    if (brief != "")
        s = brief
    else {
        s = ""
        for (at = 1; at <= desc_n; at++)
            if (desc_kind[at] == "paragraph") {
                s = desc_text[at]
                break
            }
    }
    at = index(s ". ", ". ")
    if (at > 0 && at < length(s))
        s = substr(s, 1, at)
    return s
}


#==============================================================================
# writing a page
#==============================================================================


# header_body(h): the h-th header's page, as the region holds it.
function header_body(h,    t, i, level, d, kinds, under, last, title) {
    d = dialect
    t = sprintf("%s · module %s · %s\n\n",
                wiki_inline_code(d, header_rel[h]),
                wiki_link_text(d, module_page(module_of(header_rel[h])),
                               module_of(header_rel[h])),
                wiki_link_text(d, "Home"))
    if (brief != "")
        t = t prose(brief) "\n\n"
    for (i = 1; i <= desc_n; i++) {
        if (desc_kind[i] == "paragraph")
            t = t prose(desc_text[i]) "\n\n"
        else if (desc_kind[i] == "item") {
            t = t wiki_item_text(d, prose(desc_text[i]))
            if (i == desc_n || desc_kind[i + 1] != "item")
                t = t "\n"
        } else
            t = t wiki_code_text(d, desc_text[i]) "\n"
    }
    if (toc_n > 0) {
        page_anchors(d)
        t = t wiki_heading_text(d, 2, "Contents") "\n"
        last = 0
        for (i = 1; i <= toc_n; i++) {
            level = (toc_rank[i] > last + 1) ? last + 1 : toc_rank[i]
            last = level
            t = t wiki_item_text(d, toc_item(toc_text[i]), level)
        }
        t = t "\n"
    }
    under = 0
    for (i = 1; i <= entry_n; i++) {
        if (entry_kind[i] == "part") {
            title = wiki_escape(d, title_case(entry_title[i]))
            t = t wiki_heading_text(d, entry_level[i] + 1, title) "\n"
            under = 1
            continue
        }
        if (!under) {
            t = t wiki_heading_text(d, 2, "Declarations") "\n"
            under = 1
        }
        level = entry_level[i] + 2
        level = (level < 3) ? 3 : (level > 6) ? 6 : level
        t = t declaration(i, level)
    }
    t = t includes(h)
    return t
}


# declaration(i, level): the i-th entry: its heading -- three of a run's
# names at most -- what its comment says, and its code.
function declaration(i, level,    d, t, title, kind, more) {
    d = dialect
    title = heading_title(i)
    kind = entry_label[i]
    if (entry_count[i] > 1)
        kind = entry_count[i] " " kind ((kind ~ /s$/) ? "es" : "s")
    t = wiki_heading_text(d, level, wiki_escape(d, title)) "\n"
    if (entry_doxygen[i])
        t = t doxygen_text(entry_doc[i], kind)
    else
        t = t plain_text(entry_doc[i], entry_title[i], kind)
    more = (entry_more[i] > 0) ? "\n    ... " plural(entry_more[i],
                                                    "more line") : ""
    t = t wiki_code_text(d, entry_code[i] more, language()) "\n"
    return t
}


# plain_text(doc, title, kind): what // comments say: a section's number
# and the name they repeat dropped, and a `kind:` before the text set
# apart.
function plain_text(doc, title, kind,    m, rows, i, t, line, para, first,
                    label) {
    m = split(doc, rows, "\n")
    i = 1
    if (i <= m && trim(rows[i]) ~ /^[0-9]+(\.[0-9]+)*$/)
        i++
    if (i <= m && same_names(trim(rows[i]), title))
        i++
    label = kind
    para = ""
    t = ""
    first = 1
    for (; i <= m; i++) {
        line = trim(rows[i])
        if (first && match(line, /^[a-z][a-z ()]*:[ \t]/)) {
            label = substr(line, 1, RLENGTH)
            sub(/:[ \t]*$/, "", label)
            line = substr(line, RLENGTH + 1)
        }
        first = 0
        if (line == "") {
            if (para != "")
                t = t prose(para) "\n\n"
            para = ""
            continue
        }
        para = (para == "") ? line : para " " line
    }
    if (para != "")
        t = t prose(para) "\n\n"
    return emphasis(label) "\n\n" t
}


# doxygen_text(doc, kind): what a Doxygen comment says: its brief and its
# details, then a table of its parameters, then what it returns, then its
# notes, preconditions and postconditions.
function doxygen_text(doc, kind,    m, rows, i, line, tag, what, t, para,
                      params, param_n, returns, notes, d, direction, name) {
    d = dialect
    m = split(doc, rows, "\n")
    tag = "text"
    para = ""
    t = ""
    param_n = 0
    returns = ""
    notes = ""
    for (i = 1; i <= m + 1; i++) {
        line = (i <= m) ? rows[i] : "@end"
        sub(/^[ \t]*\/\*[*!]?/, "", line)
        sub(/\*\/[ \t]*$/, "", line)
        sub(/^[ \t]*\*[ \t]?/, "", line)
        sub(/^[ \t]*\/\/[\/!][ \t]?/, "", line)
        line = trim(line)
        if (line ~ /^[@\\][a-z]/ || line == "" && tag != "text") {
            # what the tag before said, kept
            if (tag == "text" && para != "")
                t = t prose(para) "\n\n"
            else if (tag == "brief" && para != "")
                t = t prose(para) "\n\n"
            else if (tag == "param" && para != "") {
                direction = ""
                if (match(para, /^\[[a-z,]+\][ \t]*/)) {
                    direction = substr(para, 2, RLENGTH - 2)
                    sub(/\][ \t]*$/, "", direction)
                    para = substr(para, RLENGTH + 1)
                }
                name = para
                sub(/[ \t].*$/, "", name)
                what = substr(para, length(name) + 1)
                params = params sprintf("%s\t%s\t%s\n", cell_code(name),
                                        direction, cell_text(trim(what)))
                param_n++
            } else if (tag == "return" && para != "")
                returns = returns ((returns == "") ? "" : " ") para
            else if (para != "" && tag != "end")
                notes = notes wiki_item_text(d, emphasis_inline(tag) " " \
                                             prose(para))
            para = ""
            if (line == "")
                continue
            tag = line
            sub(/^[@\\]/, "", tag)
            sub(/[^a-z].*$/, "", tag)
            if (tag == "returns" || tag == "retval")
                tag = "return"
            if (tag == "tparam")
                tag = "param"
            line = substr(line, length(tag) + 2)
            if (tag == "end")
                break
        }
        if (line == "")
            continue
        para = (para == "") ? trim(line) : para " " trim(line)
    }
    if (param_n > 0)
        t = t wiki_table_text(d, "Parameter\tDirection\tDescription\n" \
                                 params) "\n"
    if (returns != "")
        t = t emphasis_inline("Returns") " " prose(returns) "\n\n"
    if (notes != "")
        t = t notes "\n"
    return emphasis(kind) "\n\n" t
}


# toc_item(text): a contents' item, a link to the declaration it names
# when the page has one.
function toc_item(text,    i, names, m, j) {
    for (i = 1; i <= entry_n; i++) {
        if (entry_kind[i] != "declaration")
            continue
        m = split(entry_title[i], names, ", ")
        for (j = 1; j <= m; j++)
            if (names[j] == text)
                return wiki_link_text(dialect, "", text, entry_anchor[i])
    }
    return wiki_escape(dialect, title_case(text))
}


# page_anchors(d): each declaration's anchor, entry_anchor[i], as its page
# makes it unique among the headings before it: Contents, the parts and
# Declarations, in the order header_body writes them.
function page_anchors(d,    titles, slot, i, n, under, made) {
    titles = "Contents\n"
    n = 1
    under = 0
    for (i = 1; i <= entry_n; i++) {
        if (entry_kind[i] == "part") {
            titles = titles title_case(entry_title[i]) "\n"
            n++
            under = 1
            continue
        }
        if (!under) {
            titles = titles "Declarations\n"
            n++
            under = 1
        }
        titles = titles heading_title(i) "\n"
        slot[i] = ++n
    }
    split(wiki_anchors(d, titles), made, "\n")
    split("", entry_anchor)
    for (i in slot)
        entry_anchor[i] = made[slot[i]]
}


# heading_title(i): the i-th entry's heading, as declaration() writes it.
function heading_title(i,    names, m, title, k) {
    m = split(entry_title[i], names, ", ")
    if (m <= 3)
        return entry_title[i]
    title = names[1]
    for (k = 2; k <= 3; k++)
        title = title ", " names[k]
    return title " and " (m - 3) " more"
}


# includes(h): the headers the h-th includes from the tree, as links, and
# the others by name.
function includes(h,    i, t, target, d, rel) {
    if (include_n == 0)
        return ""
    d = dialect
    t = wiki_heading_text(d, 2, "Includes") "\n"
    for (i = 1; i <= include_n; i++) {
        target = include_path[i]
        rel = resolved(header_rel[h], target)
        if (rel in by_rel)
            t = t wiki_item_text(d, wiki_link_text(d, header_page[by_rel[rel]],
                                                   rel))
        else
            t = t wiki_item_text(d, wiki_inline_code(d,
                                     (target ~ /^</) ? target ">" : target))
    }
    return t "\n"
}


# resolved(from, target): an include's path, from the header including it,
# or from the root; "" for one outside the tree.
function resolved(from, target,    m, parts, n, i, out, k, path) {
    if (target ~ /^</) {
        target = substr(target, 2)
        if (target in by_rel)
            return target
        sub(/^[^\/]*\//, "", target)
        return target
    }
    path = from
    sub(/[^\/]*$/, "", path)
    path = path target
    n = split(path, parts, "/")
    k = 0
    for (i = 1; i <= n; i++) {
        if (parts[i] == "." || parts[i] == "")
            continue
        if (parts[i] == "..") {
            if (k > 0)
                k--
            continue
        }
        out[++k] = parts[i]
    }
    path = ""
    for (i = 1; i <= k; i++)
        path = path ((i > 1) ? "/" : "") out[i]
    return path
}


# language(): the language a header's code is written in.
function language() {
    return code_language
}


# prose(text): text as a page shows it: `code` set as code, the rest
# escaped to read as itself.
function prose(text,    t, at, close_at, code_text) {
    t = ""
    while ((at = index(text, "`")) > 0) {
        close_at = index(substr(text, at + 1), "`")
        if (close_at == 0)
            break
        t = t wiki_escape(dialect, substr(text, 1, at - 1))
        code_text = substr(text, at + 1, close_at - 1)
        t = t wiki_inline_code(dialect, code_text)
        text = substr(text, at + close_at + 1)
    }
    return t wiki_escape(dialect, text)
}


# cell_text(text), cell_code(text): a table's cell, as prose and as code.
function cell_text(text) {
    gsub(/\t/, " ", text)
    return prose(text)
}

function cell_code(text) {
    gsub(/\t/, " ", text)
    return wiki_inline_code(dialect, text)
}


# emphasis(text), emphasis_inline(text): text set apart, alone or within.
function emphasis(text) {
    return emphasis_inline(text)
}

function emphasis_inline(text) {
    if (text == "")
        return ""
    if (dialect == "mediawiki")
        return "''" wiki_escape(dialect, text) "''"
    return "*" wiki_escape(dialect, text) "*"
}


# title_case(text): a part's title in capitals, as many headers write
# them, with only its first letter kept so.
function title_case(text) {
    if (text ~ /[a-z]/)
        return text
    return substr(text, 1, 1) tolower(substr(text, 2))
}


# same_names(line, title): whether a comment's line repeats the names it
# documents.
function same_names(line, title,    names) {
    gsub(/[ \t]+/, " ", line)
    names = title
    sub(/, .*$/, "", names)
    sub(/.*::/, "", names)
    return (line == title || line == names ||
            (index(line, names ",") == 1 && line ~ name_list))
}


#==============================================================================
# index pages
#==============================================================================


# modules_pages(): each module's page, its headers in a table.
function modules_pages(    i, module, rows, k, h, d) {
    d = dialect
    for (i = 1; i <= module_count; i++) {
        module = module_name[i]
        rows = "Header\tWhat it is\n"
        for (k = 1; k <= module_size[module]; k++) {
            h = module_header[module, k]
            rows = rows sprintf("%s\t%s\n",
                                wiki_link_text(d, header_page[h],
                                               header_rel[h]),
                                cell_text(header_summary[h]))
        }
        write_page(module_page(module),
                   sprintf("%s · %s\n\n", wiki_link_text(d, "Home"),
                           plural(module_size[module], "header")) \
                   wiki_table_text(d, rows))
    }
}


# home(): the wiki's home page, with its modules.
function home(    i, t, d, module) {
    d = dialect
    t = wiki_escape(d, project) " · " plural(headers, "header") ", " \
        plural(module_count, "module") "\n\n"
    for (i = 1; i <= module_count; i++) {
        module = module_name[i]
        t = t wiki_item_text(d, wiki_link_text(d, module_page(module), module) \
                                " · " plural(module_size[module], "header"))
    }
    write_page("Home", t)
}


# sidebar(): the wiki's sidebar: Home, and each module.
function sidebar(    i, t, d) {
    d = dialect
    t = wiki_item_text(d, wiki_link_text(d, "Home"))
    for (i = 1; i <= module_count; i++)
        t = t wiki_item_text(d, wiki_link_text(d, module_page(module_name[i]),
                                               module_name[i]))
    write_page("_Sidebar", t)
}


#==============================================================================
# pages on disk
#==============================================================================


# write_page(name, body): a page's region set to the body: in the page on
# disk when it is there, or in a page of the region alone.
function write_page(name, body,    path, p, saved, ok) {
    path = out "/" wiki_page_file(name, dialect)
    if (wiki_exists(path) == 1) {
        p = wiki_open(path, dialect)
        if (p < 0) {
            printf "%s: cannot be read: %s\n", path, wiki_error()
            status = 2
            return
        }
        if (wiki_find_region(p, "api") > 0)
            ok = wiki_set_region(p, "api", body)
        else
            ok = wiki_add_region(p, "api", body, 0)
    } else {
        p = wiki_new(path, dialect, wiki_region_text(dialect, "api", body))
        ok = (p > 0) ? 1 : -1
    }
    if (ok < 0) {
        printf "%s: cannot be changed: %s\n", path, wiki_error()
        status = 2
        if (p > 0)
            wiki_close(p)
        return
    }
    if (dry) {
        printf "%s\n", path
        written++
        wiki_close(p)
        return
    }
    saved = wiki_save(p)
    if (saved < 0) {
        printf "%s: cannot be written: %s\n", path, wiki_error()
        status = 2
    } else if (saved == 0)
        same++
    else
        written++
    wiki_close(p)
}


#==============================================================================
# text
#==============================================================================


# stripped(line): a line of code with its comments, strings and characters
# gone, for its braces and `;`s to be counted.
function stripped(line,    out, i, c, n, quote) {
    out = ""
    n = length(line)
    for (i = 1; i <= n; i++) {
        c = substr(line, i, 1)
        if (in_comment) {
            if (c == "*" && substr(line, i + 1, 1) == "/") {
                in_comment = 0
                i++
            }
            continue
        }
        if (c == "/" && substr(line, i + 1, 1) == "/")
            break
        if (c == "/" && substr(line, i + 1, 1) == "*") {
            in_comment = 1
            i++
            continue
        }
        if (c == "\"" || c == "'") {
            quote = c
            for (i++; i <= n; i++) {
                c = substr(line, i, 1)
                if (c == "\\")
                    i++
                else if (c == quote)
                    break
            }
            out = out quote quote
            continue
        }
        out = out c
    }
    return out
}


# brace_count(code): how many more `{` than `}` a line of code has.
function brace_count(code,    opens, closes) {
    opens = gsub(/{/, "{", code)
    closes = gsub(/}/, "}", code)
    return opens - closes
}


# trim(text): text without space at either end.
function trim(text) {
    sub(/^[ \t]+/, "", text)
    sub(/[ \t]+$/, "", text)
    return text
}


# plural(n, word): a count and its word, "1 header", "2 headers".
function plural(n, word) {
    return (n + 0) " " word ((n == 1) ? "" : "s")
}
