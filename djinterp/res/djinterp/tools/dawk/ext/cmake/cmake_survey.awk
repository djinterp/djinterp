# cmake_survey.awk -- what a tree of CMake files holds, file by file and as
# a whole.
#
#   dawk -x cmake -T build/cmake -f cmake_survey.awk [-v files=0]
#
# Every CMakeLists.txt, *.cmake and *.cmake.in under the folder is read the
# way CMake reads it. A file CMake or a generator wrote is counted and left
# at that. For each hand-written file the survey shows the form its header
# takes -- the djinterp banner, `#*****`; a `# =====` rule; another comment;
# or none -- its lines, its commands and its blocks, and names any block
# left open or closed twice. Then the whole: how many files of each kind,
# the commands most used, the functions and macros the tree defines and how
# often each is called, and the targets it builds.
#
#   files   0 to leave out the line for each file.
#
# The paths can come from a list instead of a folder: each line of the input
# is a path. Exit status: 0 when every file reads and every block pairs, 1
# when a file was refused or a block does not pair.

BEGIN {
    files = (files == "") ? 1 : files + 0

    if (files)
        printf "%-64s %-8s %6s %8s %6s\n", "file", "header", "lines",
               "commands", "blocks"
}

# a path whose name is not CMake's is not surveyed
!/(^|\/)(CMakeLists\.txt|[^\/]*\.cmake|[^\/]*\.cmake\.in)$/ {
    next
}

{
    path = $0
    seen++

    F = cmake_open(path)

    # a file CMake would refuse is named, and counted
    if (F < 0) {
        refused[++refusals] = cmake_error()
        next
    }

    # what a generator wrote is counted, and no more
    if (cmake_generated(F) || path ~ /(^|\/)CMakeFiles\//) {
        generated++
        cmake_close(F)
        next
    }

    commands = cmake_commands(F)

    written++
    form  = header(path)
    lines = count_lines(path)
    forms[form]++
    total_lines += lines
    total_commands += commands
    blocks = 0

    for (n = 1; n <= commands; n++) {
        name = tolower(cmake_name(F, n))
        used[name]++

        # a block that opens is counted; one that does not pair is named
        if (cmake_command(F, n, "role") == "open")
            blocks++
        if (cmake_command(F, n, "unpaired")) {
            unpaired[++unpairs] = path ":" cmake_command(F, n, "line") ": " \
                                  cmake_name(F, n) " does not pair"
        }

        # a function or macro the tree defines, by the name it gives
        if (name == "function" || name == "macro")
            defined[tolower(cmake_value(F, n, 1))] = name

        # a target, by the command that makes it
        if (name == "add_executable" || name == "add_library" ||
            name == "add_custom_target")
            targets[name]++
    }

    total_blocks += blocks

    if (files)
        printf "%-64s %-8s %6d %8d %6d\n", path, form, lines, commands, blocks

    cmake_close(F)
}

END {
    printf "\n%d CMake files: %d hand-written, %d generated, %d refused\n",
           seen, written, generated, refusals
    printf "headers: %d banner, %d rule, %d other comment, %d none\n",
           forms["banner"], forms["rule"], forms["comment"], forms["none"]
    printf "%d lines, %d commands, %d blocks, %d unpaired\n", total_lines,
           total_commands, total_blocks, unpairs

    show_top("commands most used", used, 12)

    # the tree's own functions and macros, with how often each is called
    count = 0
    for (name in defined) {
        calls[name] = used[name] + 0
        count++
    }
    if (count)
        show_top("functions and macros defined here, by calls", calls, count)

    printf "targets: %d add_executable, %d add_library, %d add_custom_target\n",
           targets["add_executable"], targets["add_library"],
           targets["add_custom_target"]

    for (at = 1; at <= refusals; at++)
        print "refused: " refused[at]
    for (at = 1; at <= unpairs; at++)
        print "unpaired: " unpaired[at]

    exit (refusals || unpairs) ? 1 : 0
}

# header gives the form a file's first line takes
function header(path,    first) {
    first = ""
    getline first < path
    close(path)
    sub(/\r$/, "", first)
    if (first ~ /^#\*\*\*+$/)
        return "banner"
    if (first ~ /^# ?(=|-)(=|-)(=|-)+$/)
        return "rule"
    if (first ~ /^#/)
        return "comment"
    return "none"
}

# count_lines counts a file's lines, a last one with no ending included
function count_lines(path,    line, lines) {
    lines = 0
    while ((getline line < path) > 0)
        lines++
    close(path)
    return lines
}

# show_top prints the `limit` largest counts of a table, largest first, ties
# in name order
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
        printf "  %-40s %6d\n", names[best], table[names[best]]
        names[best] = ""
    }
}
