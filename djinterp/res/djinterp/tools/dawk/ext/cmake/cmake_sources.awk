# cmake_sources.awk -- the source lists of a CMake file's targets: shown, or
# changed in place.
#
#   dawk -x cmake -f cmake_sources.awk -v list=<CMakeLists.txt>
#        [-v target=<name>] [-v add='<file> ...'] [-v remove='<file> ...']
#        [-v apply=1] [-v out=<file>]
#
# A source list is the run of arguments that names a target's files:
#     add_executable(<target> [WIN32] [MACOSX_BUNDLE] [EXCLUDE_FROM_ALL] ...)
#     add_library(<target> [STATIC|SHARED|MODULE|OBJECT|INTERFACE]
#                 [EXCLUDE_FROM_ALL] ...)
#     target_sources(<target> PRIVATE|PUBLIC|INTERFACE ...)
#     any command whose first argument is the target and that has a SOURCES
#     keyword, as the tree's own djinterp_add_test_executable does.
# The run ends at the command's end or at the next keyword -- an argument in
# capitals, PRIVATE, DEFINES -- and a comment among it is no part of it.
#
#   list    the CMake file. Without add or remove, every list it holds is
#           shown, or only the target's.
#   target  the target to change; add and remove need it. Its first list is
#           the one changed.
#   add     files to add, separated by spaces, compared by what CMake reads
#           each argument as; one the list has already is left. A file whose
#           name holds a space is added in a run of its own. A list kept
#           in order keeps its order, the files going in in order; another
#           is added to at its end, in the order given. A file
#           is quoted when the list's own files are, or when it must be.
#   remove  files to take out of the list.
#   apply   1 to write the file. Without it the run only shows what it
#           would do.
#   out     where the changed file goes; the list itself by default.
#
# Each change takes the layout of the list: a file on a line of its own,
# indented as its neighbours, where they stand one to a line. Exit status:
# 0 when all is well, 1 when the target or a file to remove is not there,
# 2 when the file could not be read or written.

BEGIN {
    # the file to read is required, and a change needs its target
    if (list == "") {
        print "cmake_sources: -v list=<CMakeLists.txt> is required" \
              > "/dev/stderr"
        exit 2
    }
    if ((add != "" || remove != "") && target == "") {
        print "cmake_sources: add and remove need -v target=<name>" \
              > "/dev/stderr"
        exit 2
    }

    F = cmake_open(list)
    if (F < 0) {
        print "cmake_sources: " cmake_error() > "/dev/stderr"
        exit 2
    }

    status = 0

    # without a change, every list is shown
    if (add == "" && remove == "") {
        shown = 0
        for (n = 1; n <= cmake_commands(F); n++) {
            if (!sources(n))
                continue
            if (target != "" && cmake_value(F, n, 1) != target)
                continue
            shown++
            printf "%s (%s, line %d):\n", cmake_value(F, n, 1),
                   cmake_name(F, n), cmake_command(F, n, "line")
            for (k = first; k <= last; k++)
                if (!keyword(n, k))
                    printf "    %s\n", cmake_arg(F, n, k)
        }
        if (target != "" && !shown) {
            print "cmake_sources: no source list for " target > "/dev/stderr"
            status = 1
        }
        exit status
    }

    # the target's first list is the one changed
    for (n = 1; n <= cmake_commands(F); n++)
        if (sources(n) && cmake_value(F, n, 1) == target)
            break
    if (n > cmake_commands(F)) {
        print "cmake_sources: no source list for " target > "/dev/stderr"
        exit 1
    }

    printf "%s (%s, line %d of %s):\n", target, cmake_name(F, n),
           cmake_command(F, n, "line"), list

    quoted = (last >= first && cmake_arg(F, n, last, "kind") == "quoted")
    sorted = in_order(n)

    # each change is made at once, so the list is found again before each
    count = split(remove, gone, " ")
    for (at = 1; at <= count; at++) {
        sources(n)
        k = find(n, gone[at])
        if (!k) {
            printf "  not in the list: %s\n", gone[at]
            status = 1
            continue
        }
        text = cmake_arg(F, n, k)
        if (cmake_remove_arg(F, n, k) < 0) {
            printf "  refused %s: %s\n", gone[at], cmake_error()
            status = 1
            continue
        }
        printf "  - %s\n", text
    }

    count = split(add, wanted, " ")

    # into a list kept in order, the files go in order too
    if (sorted)
        sort_list(wanted, count)

    for (at = 1; at <= count; at++) {
        file = wanted[at]
        sources(n)
        if (find(n, file)) {
            printf "  already there: %s\n", file
            continue
        }
        # quoted as the list is, or when it must be
        text = (quoted) ? cmake_quote(file) : cmake_argument(file)
        if (place(n, text, file) < 0) {
            printf "  refused %s: %s\n", file, cmake_error()
            status = 1
            continue
        }
        printf "  + %s\n", text
    }

    # nothing is written unless asked, and nothing at all when nothing changed
    if (!cmake_changes(F)) {
        print "nothing to change"
        exit status
    }
    if (!apply) {
        printf "dry run: nothing written; -v apply=1 writes %s\n",
               (out == "") ? list : out
        exit status
    }
    if (cmake_save(F, out) < 0) {
        print "cmake_sources: " cmake_error() > "/dev/stderr"
        exit 2
    }
    printf "wrote %s\n", (out == "") ? list : out
    exit status
}

# keyword is whether argument k of command n is a keyword: an unquoted
# argument in capitals, digits and underscores
function keyword(n, k) {
    return cmake_arg(F, n, k, "kind") == "unquoted" &&
           cmake_value(F, n, k) ~ /^[A-Z][A-Z0-9_]+$/
}

# sources finds command n's source list, setting `first` and `last` to its
# first and last arguments; it gives 0 for a command with none. A list may
# be empty, `last` then standing before `first`.
function sources(n,    name, k, args, from) {
    name = tolower(cmake_name(F, n))
    args = cmake_args(F, n)
    from = 0

    if (args < 1)
        return 0

    if (name == "add_executable" || name == "add_library") {
        # an imported or an alias target has no sources
        if (cmake_value(F, n, 2) == "IMPORTED" ||
            cmake_value(F, n, 2) == "ALIAS")
            return 0
        from = 2
        while (from <= args && cmake_value(F, n, from) ~ \
               /^(WIN32|MACOSX_BUNDLE|EXCLUDE_FROM_ALL|STATIC|SHARED|MODULE|OBJECT|INTERFACE)$/)
            from++
    }
    else if (name == "target_sources") {
        from = 2
        while (from <= args && cmake_value(F, n, from) ~ \
               /^(PRIVATE|PUBLIC|INTERFACE)$/)
            from++
    }
    else {
        k = cmake_find_arg(F, n, "SOURCES", 1)
        if (!k)
            return 0
        from = k + 1
    }

    first = from
    last  = from - 1
    for (k = from; k <= args; k++) {
        # target_sources lists files under each scope in turn
        if (name == "target_sources" && cmake_value(F, n, k) ~ \
            /^(PRIVATE|PUBLIC|INTERFACE)$/) {
            last = k
            continue
        }
        if (keyword(n, k))
            break
        last = k
    }
    return 1
}

# find gives the argument of the list whose value is `file`, or 0
function find(n, file,    k) {
    for (k = first; k <= last; k++)
        if (!keyword(n, k) && cmake_value(F, n, k) == file)
            return k
    return 0
}

# in_order is whether the list's files stand in order by value
function in_order(n,    k, previous) {
    previous = ""
    for (k = first; k <= last; k++) {
        if (keyword(n, k))
            continue
        if (previous != "" && cmake_value(F, n, k) < previous)
            return 0
        previous = cmake_value(F, n, k)
    }
    return 1
}

# sort_list puts the first `count` entries of a list in order
function sort_list(list, count,    at, before, held) {
    for (at = 2; at <= count; at++) {
        held = list[at]
        for (before = at - 1; before >= 1 && list[before] > held; before--)
            list[before + 1] = list[before]
        list[before + 1] = held
    }
}

# place adds a file to the list: before the first that sorts after it, in a
# list kept in order, and after the last otherwise; into an empty list after
# what opens it
function place(n, text, file,    k) {
    if (last < first)
        return (first > 1) ? cmake_insert_after(F, n, first - 1, text) \
                           : cmake_insert_arg(F, n, first, text)
    if (sorted)
        for (k = first; k <= last; k++)
            if (!keyword(n, k) && cmake_value(F, n, k) > file)
                return cmake_insert_arg(F, n, k, text)
    return cmake_insert_after(F, n, last, text)
}
