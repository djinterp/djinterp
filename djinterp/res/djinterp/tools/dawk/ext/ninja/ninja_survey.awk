# ninja_survey.awk -- what a tree of Ninja files holds, file by file and as
# a whole, and what each build in it comes to when ninja loads it.
#
#   dawk -x ninja -T build -f ninja_survey.awk [-v files=0] [-v load=0]
#
# Every *.ninja under the folder is read as ninja's parser reads it, and
# for each the survey shows its lines and its statements of each kind:
# variables, rules, builds, pools, defaults, includes and subninjas. A file
# the parser refuses is named, with ninja's reason and line. Then each
# build.ninja is loaded as ninja loads it, in its own directory, with what
# it includes: its edges, its nodes, its roots and its defaults, or ninja's
# refusal, and any warning ninja would give. Then the whole: how many
# statements of each kind, the rules most used by the builds, and the
# pools named.
#
#   files   0 to leave out the line for each file.
#   load    0 to read each file alone, and load none.
#
# The paths can come from a list instead of a folder: each line of the input
# is a path. Exit status: 0 when every file reads and every build loads, 1
# when one does not.

BEGIN {
    files = (files == "") ? 1 : files + 0
    load = (load == "") ? 1 : load + 0
    split("variable rule build pool default include subninja", kinds, " ")

    if (files)
        printf "%-56s %6s %5s %5s %6s %5s %5s\n", "file", "lines", "vars",
               "rules", "builds", "pools", "other"
}

# a path whose name is not a Ninja file's is not surveyed
!/\.ninja$/ {
    next
}

{
    path = $0
    seen++

    F = ninja_open(path)

    # a file ninja's parser would refuse is named, and counted
    if (F < 0) {
        refused[++refusals] = ninja_error()
        next
    }

    read++
    split("", count)
    statements = ninja_statements(F)
    for (n = 1; n <= statements; n++) {
        kind = ninja_statement(F, n, "kind")
        count[kind]++
        total[kind]++

        # a build's rule, counted; a pool, named
        if (kind == "build")
            used[ninja_statement(F, n, "name")]++
        else if (kind == "pool")
            pools[ninja_statement(F, n, "name")] = path
    }
    lines = line_count(F)
    total_lines += lines
    total_comments += ninja_comments(F)
    other = count["default"] + count["include"] + count["subninja"]

    if (files)
        printf "%-56s %6d %5d %5d %6d %5d %5d\n", path, lines,
               count["variable"], count["rule"], count["build"],
               count["pool"], other

    # a manifest, to be loaded once every file is read
    if (load && path ~ /(^|\/)build\.ninja$/)
        manifests[++manifest_count] = path

    ninja_close(F)
}

END {
    printf "\n%d Ninja files: %d read, %d refused\n", seen, read, refusals
    printf "%d lines, %d comments;", total_lines, total_comments
    for (k = 1; k <= 7; k++)
        printf " %d %s", total[kinds[k]], kinds[k] ((k < 7) ? "," : "\n")

    for (m = 1; m <= manifest_count; m++)
        survey_build(manifests[m])

    show_top("rules most used", used, 12)

    names = ""
    for (name in pools)
        names = names " " name
    if (names != "")
        print "pools:" names

    for (at = 1; at <= refusals; at++)
        print "refused: " refused[at]
    for (at = 1; at <= failures; at++)
        print "would not load: " failed[at]

    exit (refusals || failures) ? 1 : 0
}

# survey_build loads a manifest in its own directory, as ninja -C would
function survey_build(path,    dir, g, roots, defaults, warnings, w, i) {
    dir = path
    if (!sub(/\/[^\/]*$/, "", dir))
        dir = "."
    g = ninja_load("build.ninja", dir)
    if (g < 0) {
        failed[++failures] = path ": " ninja_error()
        return
    }
    roots = lines_in(ninja_roots(g))
    defaults = lines_in(ninja_defaults(g))
    printf "build %s: %d files, %d edges, %d nodes, %d roots, %d defaults\n",
           path, lines_in(ninja_files(g)), ninja_edges(g), ninja_nodes(g),
           roots, defaults
    warnings = split(ninja_warnings(g), w, "\n")
    for (i = 1; i <= warnings; i++)
        print "  warning: " w[i]
    ninja_unload(g)
}

# lines_in counts the items of a list, one to a line
function lines_in(list,    item) {
    return split(list, item, "\n")
}

# line_count counts a file's lines, a last one with no ending included
function line_count(F,    text, n) {
    text = ninja_text(F)
    n = gsub(/\n/, "\n", text)
    return n + ((text != "" && substr(text, length(text)) != "\n") ? 1 : 0)
}

# show_top prints the most-used names of a table, most used first, a tie
# by name
function show_top(title, table, limit,    name, names, count, at, best, done) {
    count = 0
    for (name in table)
        names[++count] = name

    printf "%s:\n", title
    for (done = 0; done < limit && done < count; done++) {
        best = 0
        for (at = 1; at <= count; at++) {
            if (names[at] == "")
                continue
            if (best == 0 || table[names[at]] > table[names[best]] ||
                (table[names[at]] == table[names[best]] &&
                 names[at] < names[best]))
                best = at
        }
        printf "  %-48s %6d\n", names[best], table[names[best]]
        names[best] = ""
    }
}
