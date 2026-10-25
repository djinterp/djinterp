# git_changelog.awk -- a repository's history since a tag, as Markdown.
#
#   dawk -x git -f git_changelog.awk [-v repo=path] [-v from=rev] [-v to=rev]
#                                    [-v title=text] [-v types=0]
#                                    [-v authors=1]
#
# The commits `to` reaches -- HEAD by default -- that `from` does not, the
# newest first; merges are left out. Without from, it is the newest tag `to`
# reaches other than one on `to` itself -- the tag before the release `to`
# is -- and, when there is none, all of history.
#   Each commit is an item: its subject and its short name, and its author's
# name with authors. With types -- the default -- a subject written as a
# Conventional Commit, type(scope)!: description, is grouped under its
# type's heading, its scope in bold and the type taken off; breaking changes
# -- a `!`, or a BREAKING CHANGE: footer -- come first, under their own.
# Any other subject is among "Other changes".
#
#   repo     a path in the repository; the working directory by default.
#   title    the heading; "Changes since <from>", or "Changes".
#   types    0 for one list, in history's order.
#   authors  1 to name each commit's author.
#
# Exit status: 0, or 2 when the repository or a revision cannot be read.

BEGIN {
    r = git_repo((repo == "") ? "." : repo)
    if (r < 0)
        fail("not a repository: " git_error())
    if (to == "")
        to = "HEAD"
    if (types == "")
        types = 1
    head = git_rev(r, to "^{commit}")
    if (head == -1)
        fail(to ": " git_error())
    if (from == "")
        from = last_tag(r, head)
    else if (git_rev(r, from "^{commit}") == -1)
        fail(from ": " git_error())
    gather(r)
    if (title == "")
        title = (from == "") ? "Changes" : "Changes since " from
    print "# " title
    if (count == 0) {
        print ""
        print "No changes."
    } else if (types)
        by_type()
    else
        section("", "")
    exit 0
}


# fail(why): the run stopped, said why.
function fail(why) {
    printf "git_changelog: %s\n", why
    exit 2
}


# last_tag(r, head): the newest tag head reaches, other than one on head
# itself; "" for none.
function last_tag(r, head,    names, m, i, tag, commit, tagged, walk, at) {
    m = split(git_refs(r, "refs/tags/"), names, "\n")
    for (i = 1; i <= m; i++) {
        if (names[i] == "")
            continue
        tag = substr(names[i], 11)
        commit = git_rev(r, names[i] "^{commit}")
        if (commit != -1 && !(commit in tagged))
            tagged[commit] = tag
    }
    m = split(git_log(r, head), walk, "\n")
    for (at = 2; at <= m; at++)
        if (walk[at] in tagged)
            return tagged[walk[at]]
    return ""
}


# gather(r): each commit to reaches and from does not, merges aside, its
# subject read for its type.
function gather(r,    list, m, i, c, subject, lead) {
    list = git_log(r, (from == "") ? to : to " ^" from)
    if (list == -1)
        fail(git_error())
    m = split(list, commits, "\n")
    for (i = 1; i <= m; i++) {
        c = commits[i]
        if (c == "" || git_commit(r, c, "parent_count") > 1)
            continue
        count++
        subject = git_commit(r, c, "subject")
        short[count] = git_short(r, c)
        author[count] = git_commit(r, c, "author")
        kind[count] = ""
        scope[count] = ""
        text[count] = subject
        whole[count] = subject
        breaking[count] = (git_commit(r, c, "body") ~ \
                           /(^|\n)BREAKING[ -]CHANGE: /)
        if (match(subject, /^[A-Za-z]+(\([^)]*\))?!?: /)) {
            lead = substr(subject, 1, RLENGTH - 2)
            text[count] = substr(subject, RLENGTH + 1)
            if (lead ~ /!$/) {
                breaking[count] = 1
                lead = substr(lead, 1, length(lead) - 1)
            }
            if (match(lead, /\(.*\)$/)) {
                scope[count] = substr(lead, RSTART + 1, RLENGTH - 2)
                lead = substr(lead, 1, RSTART - 1)
            }
            kind[count] = tolower(lead)
        }
    }
}


# by_type(): breaking changes first, then each type's, in a fixed order,
# then the rest.
function by_type(    order, m, i, heading, known, k) {
    m = split("feat:Features fix:Fixes perf:Performance " \
              "refactor:Refactoring docs:Documentation test:Tests " \
              "build:Build ci:Continuous_integration style:Style " \
              "chore:Chores revert:Reverts", order, " ")
    section("breaking", "Breaking changes")
    for (i = 1; i <= m; i++) {
        k = substr(order[i], 1, index(order[i], ":") - 1)
        heading = substr(order[i], index(order[i], ":") + 1)
        gsub(/_/, " ", heading)
        known[k] = 1
        section(k, heading)
    }
    for (i = 1; i <= count; i++)
        if (!breaking[i] && !(kind[i] in known))
            kind[i] = "other"
    section("other", "Other changes")
}


# section(k, heading): the commits of a kind under a heading -- all of
# them, for no kind.
function section(k, heading,    i, shown) {
    for (i = 1; i <= count; i++) {
        if (k == "breaking" && !breaking[i])
            continue
        if (k != "" && k != "breaking" && (breaking[i] || kind[i] != k))
            continue
        if (!shown++) {
            print ""
            if (heading != "") {
                print "## " heading
                print ""
            }
        }
        print item(i)
    }
}


# item(i): a commit's line.
function item(i,    line) {
    line = "- "
    if (types && scope[i] != "")
        line = line "**" scope[i] ":** "
    line = line ((types) ? text[i] : whole[i])
    line = line " (`" short[i] "`)"
    if (authors)
        line = line " -- " author[i]
    return line
}
