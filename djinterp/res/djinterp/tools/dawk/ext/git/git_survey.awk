# git_survey.awk -- what a repository holds, and whether what it records of
# its submodules agrees.
#
#   dawk -x git -f git_survey.awk [-v quiet=1] [repository]...
#
# Each repository named -- the working directory's, without one -- is
# described in a few lines: where it is, its branch and HEAD, its branches,
# tags and remotes, each branch's upstream and how far ahead and behind of
# it the branch is, its index and its work tree's status counted by kind.
# Beneath, what disagrees:
#     a submodule .gitmodules names that has no path or no url, or whose
#       path is no gitlink in HEAD's tree or in the index
#     a gitlink HEAD's tree or the index holds that no submodule names
#     a branch whose upstream is configured but gone
#     what git refuses in .gitmodules or in the repository's configuration
# A bare repository's .gitmodules is HEAD's.
#
#   quiet   1 to print only what disagrees.
#
# Exit status: 0 when nothing disagrees, 1 when something does, 2 when a
# repository could not be read.

BEGIN {
    status = 0
    surveyed = 0
    for (i = 1; i < ARGC; i++)
        if (ARGV[i] != "") {
            survey(ARGV[i])
            surveyed++
        }
    if (surveyed == 0)
        survey(".")
    exit status
}


# survey(path): the repository a path is in, described, and what in it
# disagrees.
function survey(path,    r) {
    r = git_repo(path)
    if (r < 0) {
        printf "%s: not a repository: %s\n", path, git_error()
        status = 2
        return
    }
    pending = ""
    describe(r, path)
    branches(r)
    if (git_work_tree(r) != "")
        work(r)
    submodules(r)
    problems_of(r, git_config_file(r), "config")
    if (pending != "") {
        if (quiet)
            print path ":"
        printf "%s", pending
        if (status == 0)
            status = 1
    }
    git_close(r)
}


# describe(r, path): where a repository is, and what HEAD is.
function describe(r, path,    branch, head) {
    if (quiet)
        return
    branch = git_branch(r)
    head = git_rev(r, "HEAD")
    printf "%s: %s, %s\n", path, git_dir(r),
           (git_work_tree(r) == "") ? "bare" : "work tree " git_work_tree(r)
    if (head == -1)
        printf "  HEAD: %s, no commit yet\n",
               (branch == "") ? "detached" : branch
    else
        printf "  HEAD: %s at %s, %s\n",
               (branch == "") ? "detached" : branch, git_short(r, "HEAD"),
               git_commit(r, "HEAD", "subject")
    printf "  %s, %s, %s; %s\n",
           plural(count(git_refs(r, "refs/heads/")), "branch"),
           plural(count(git_refs(r, "refs/tags/")), "tag"),
           plural(count(git_refs(r, "refs/remotes/")), "remote ref"),
           remotes(r)
}


# remotes(r): the remotes a repository's configuration names, with their
# urls.
function remotes(r,    i, key, name, seen, text) {
    text = ""
    for (i = 1; i <= git_settings(r); i++) {
        key = git_setting(r, i, "key")
        if (key !~ /^remote\..+\.url$/)
            continue
        name = substr(key, 8, length(key) - 11)
        if (name in seen)
            continue
        seen[name] = 1
        text = text ((text == "") ? "" : ", ") name " " \
               git_config(r, "remote." name ".url")
    }
    return (text == "") ? "no remotes" : "remotes " text
}


# branches(r): each branch's upstream, and how far ahead and behind of it
# the branch is; an upstream configured but gone disagrees.
function branches(r,    names, m, i, name, merge, up, ahead, behind) {
    m = split(git_refs(r, "refs/heads/"), names, "\n")
    for (i = 1; i <= m; i++) {
        if (names[i] == "")
            continue
        name = substr(names[i], 12)
        merge = git_config(r, "branch." name ".merge")
        if (merge == "")
            continue
        up = git_rev(r, name "@{upstream}")
        if (up == -1) {
            disagree(sprintf("branch %s: its upstream %s is gone", name,
                             upstream_name(r, name, merge)))
            continue
        }
        if (quiet)
            continue
        ahead = count(git_log(r, names[i] " ^" up))
        behind = count(git_log(r, up " ^" names[i]))
        printf "  %s: tracks %s, %s\n", name, upstream_name(r, name, merge),
               (ahead + behind == 0) ? "even" : \
               sprintf("%d ahead, %d behind", ahead, behind)
    }
}


# upstream_name(r, name, merge): a branch's upstream as git names it,
# remote/branch, or the branch alone for a remote of ".".
function upstream_name(r, name, merge,    remote) {
    remote = git_config(r, "branch." name ".remote")
    sub(/^refs\/heads\//, "", merge)
    return (remote == "." || remote == "") ? merge : remote "/" merge
}


# work(r): the index and the work tree's status, counted by kind.
function work(r,    lines, m, i, x, y, staged, changed, untracked,
              unmerged) {
    if (quiet)
        return
    m = split(git_status(r), lines, "\n")
    for (i = 1; i <= m; i++) {
        if (lines[i] == "")
            continue
        x = substr(lines[i], 1, 1)
        y = substr(lines[i], 2, 1)
        if (x == "?")
            untracked++
        else if (x == "U" || y == "U" || (x == "A" && y == "A") || \
                 (x == "D" && y == "D"))
            unmerged++
        else {
            if (x != " ")
                staged++
            if (y != " ")
                changed++
        }
    }
    printf "  index: %s; %d staged, %d changed, %d untracked, %d unmerged\n",
           plural(git_index(r), "entry"), staged, changed, untracked,
           unmerged
}


# submodules(r): .gitmodules against the gitlinks HEAD's tree and the
# index hold.
function submodules(r,    m, links, i, name, path, url, named, n) {
    split("", links)
    split("", named)
    gitlinks(r, links)
    m = modules(r)
    if (m < 0)
        n = 0
    else
        n = git_sections(m)
    for (i = 1; i <= n; i++) {
        if (git_section(m, i, "name") != "submodule" || \
            git_section(m, i, "subsection") == "")
            continue
        name = git_section(m, i, "subsection")
        path = git_config(m, "submodule." name ".path")
        url = git_config(m, "submodule." name ".url")
        if (path == "") {
            disagree(sprintf("submodule %s: no path", name))
            continue
        }
        named[path] = 1
        if (url == "")
            disagree(sprintf("submodule %s: no url", name))
        if (!(path in links))
            disagree(sprintf("submodule %s: %s is no gitlink in HEAD or the "\
                             "index", name, path))
    }
    for (path in links)
        if (!(path in named))
            disagree(sprintf("gitlink %s, in %s: no submodule names it",
                             path, links[path]))
    if (m >= 0) {
        problems_of_file(m, ".gitmodules")
        if (!quiet && n > 0)
            printf "  %s\n", plural(n, "submodule")
        git_close(m)
    }
}


# modules(r): .gitmodules opened -- the work tree's, or a bare
# repository's HEAD's -- or -1 for none.
function modules(r,    tree) {
    tree = git_work_tree(r)
    if (tree != "")
        return git_open(tree "/.gitmodules", "config")
    if (!git_exists(r, "HEAD:.gitmodules"))
        return -1
    return git_new(".gitmodules", "config", git_cat(r, "HEAD:.gitmodules"))
}


# gitlinks(r, links): the paths HEAD's tree and the index hold gitlinks
# at, each with where it is.
function gitlinks(r, links,    m, lines, i, tab, path, n) {
    if (git_exists(r, "HEAD")) {
        m = split(git_tree(r, "HEAD", 1), lines, "\n")
        for (i = 1; i <= m; i++) {
            if (lines[i] !~ /^160000 /)
                continue
            tab = index(lines[i], "\t")
            links[substr(lines[i], tab + 1)] = "HEAD"
        }
    }
    n = (git_work_tree(r) == "") ? 0 : git_index(r)
    for (i = 1; i <= n; i++) {
        if (git_index_entry(r, i, "mode") != "160000")
            continue
        path = git_index_entry(r, i, "path")
        links[path] = (path in links) ? "HEAD and the index" : "the index"
    }
}


# problems_of(r, path, name): what git refuses in a configuration file.
function problems_of(r, path, name,    f) {
    f = git_open(path, "config")
    if (f < 0)
        return
    problems_of_file(f, name)
    git_close(f)
}


# problems_of_file(f, name): what git refuses in a file open.
function problems_of_file(f, name,    i) {
    for (i = 1; i <= git_problems(f); i++)
        disagree(sprintf("%s:%d: %s", name, git_problem(f, i, "line"),
                         git_problem(f, i, "what")))
}


# disagree(what): something that disagrees, kept to print after the
# repository's lines.
function disagree(what) {
    pending = pending "  ! " what "\n"
}


# count(list): how many lines a list has.
function count(list) {
    return (list == "" || list == -1) ? 0 : gsub(/\n/, "\n", list)
}


# plural(n, word): n and a word, plural when n is not 1.
function plural(n, word) {
    if (n == 1)
        return n " " word
    if (word ~ /(ch|sh|s|x)$/)
        return n " " word "es"
    if (word ~ /[^aeiou]y$/)
        return n " " substr(word, 1, length(word) - 1) "ies"
    return n " " word "s"
}
