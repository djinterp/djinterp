# git_ignore.awk -- patterns added to an ignore file, or taken out of it,
# the rest of it left as it was; or which paths it, or a repository,
# ignores.
#
#   dawk -x git -f git_ignore.awk [-v file=path] [-v comment=text] pattern...
#   dawk -x git -f git_ignore.awk -v mode=remove [-v file=path] pattern...
#   dawk -x git -f git_ignore.awk -v mode=check [-v file=path] [-v repo=path]
#                                 path...
#
# The patterns or paths are the operands; with none, standard input's lines
# are.
#
#   file     the ignore file: .gitignore by default, made when it is not
#            there.
#   mode     add, the default: each pattern the file does not hold already,
#              as written, added at its end -- beneath a comment, when one
#              is given and the file has no such line. A line git would not
#              read as a pattern, a comment or a blank, is refused.
#            remove: each line that is the pattern, as written, taken out.
#            check: each path, and what decides it, as git check-ignore -v
#              --non-matching writes it: the file's patterns alone, when
#              file is given; else the rules of the repository the
#              working directory -- or repo -- is in.
#   comment  the comment added patterns go beneath, `#` before it if it has
#            none.
#
# Exit status: add and remove, 0, or 2 when the file cannot be read or
# saved; check, 0 when a pattern decides some path -- a `!` pattern among
# them, as check-ignore -v has it -- 1 when none does, and 2 when the rules
# cannot be read.

BEGIN {
    named = (file != "")
    if (!named)
        file = ".gitignore"
    if (mode == "")
        mode = "add"
    if (mode != "add" && mode != "remove" && mode != "check") {
        printf "git_ignore: no mode %s: add, remove or check\n", mode
        finished = 1
        result = 2
        exit 2
    }
    for (i = 1; i < ARGC; i++)
        if (ARGV[i] != "") {
            items[++count] = ARGV[i]
            ARGV[i] = ""
        }
    if (count > 0)
        exit finish()
}

$0 != "" {
    items[++count] = $0
}

END {
    # an exit in BEGIN comes here too, with its work done
    if (!finished)
        exit finish()
    exit result
}


# finish(): the mode's work on every item; the exit status.
function finish() {
    finished = 1
    if (mode == "check")
        result = check()
    else
        result = change()
    return result
}


# change(): the patterns added, or taken out, and the file saved.
function change(    f, saved, done) {
    f = git_open(file, "ignore")
    if (f < 0)
        f = git_new(file, "ignore")
    if (f < 0) {
        printf "%s: %s\n", file, git_error()
        return 2
    }
    done = (mode == "add") ? add(f) : remove(f)
    saved = git_save(f)
    if (saved < 0) {
        printf "%s: cannot be saved: %s\n", file, git_error()
        return 2
    }
    printf "%s: %s %s%s\n", file, (mode == "add") ? "added" : "removed",
           plural(done, "pattern"), (saved == 0) ? ", unchanged" : ""
    return 0
}


# add(f): each pattern the file does not hold already, at its end; how
# many were added.
function add(f,    have, i, p, line, added, noted) {
    for (p = 1; p <= git_lines(f); p++)
        have[git_line(f, p, "text")] = 1
    for (i = 1; i <= count; i++) {
        if (items[i] in have) {
            printf "%s: already there\n", items[i]
            continue
        }
        if (comment != "" && !noted) {
            noted = 1
            if (!(note() in have))
                git_add_line(f, note())
        }
        line = git_add_line(f, items[i])
        if (line < 0) {
            printf "%s: refused: %s\n", items[i], git_error()
            continue
        }
        if (git_line(f, line, "kind") != "pattern") {
            printf "%s: refused: git reads it as a %s, not a pattern\n",
                   items[i], git_line(f, line, "kind")
            git_remove_line(f, line)
            continue
        }
        have[items[i]] = 1
        added++
    }
    return added + 0
}


# note(): the comment added patterns go beneath, as a line.
function note() {
    return (comment ~ /^#/) ? comment : "# " comment
}


# remove(f): each line that is a pattern, as written, taken out; how many
# were.
function remove(f,    i, l, removed, found) {
    for (i = 1; i <= count; i++) {
        found = 0
        for (l = git_lines(f); l >= 1; l--)
            if (git_line(f, l, "kind") == "pattern" && \
                git_line(f, l, "text") == items[i]) {
                git_remove_line(f, l)
                found++
            }
        if (found == 0)
            printf "%s: not there\n", items[i]
        removed += found
    }
    return removed + 0
}


# check(): each path and what decides it; 0 when a pattern decides one,
# else 1.
function check(    h, i, rule, any) {
    h = named ? git_open(file, "ignore") : git_repo((repo == "") ? "." : repo)
    if (h < 0) {
        printf "%s: %s\n", named ? file : ((repo == "") ? "." : repo),
               git_error()
        return 2
    }
    for (i = 1; i <= count; i++) {
        rule = git_ignore_rule(h, items[i])
        if (rule == -1) {
            printf "%s: %s\n", items[i], git_error()
            return 2
        }
        printf "%s\t%s\n", (rule == "") ? "::" : rule, items[i]
        if (rule != "")
            any = 1
    }
    git_close(h)
    return any ? 0 : 1
}


# plural(n, word): n and a word, plural when n is not 1.
function plural(n, word) {
    return n " " word ((n == 1) ? "" : "s")
}
