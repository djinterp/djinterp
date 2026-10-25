# ninja_set.awk -- a Ninja file's variables, or the bindings of one of its
# rules, builds or pools, set or taken out, the rest of it left as it was.
#
#   dawk -x ninja -f ninja_set.awk [-v file=build.ninja] [-v apply=1]
#                                  [-v build=output | -v rule=name |
#                                   -v pool=name] [-v raw=1] change...
#
# Each change is an operand: name=value sets it, and a name alone takes it
# out. Without build, rule or pool, a name is the file's own variable: its
# last assignment takes the value, which is what the builds see once the
# file is read, or one is added after the file's last variable before its
# first rule, build or pool; taken out, every assignment of it goes. With
# one of them, a name is a binding of the build whose outputs name the
# output, or of the rule or the pool: its last binding takes the value, or
# one is added after its last; taken out, every binding of it goes.
#
#   file   the Ninja file: build.ninja by default.
#   apply  1 to save the file; without it the changes are only shown.
#   raw    1 to write a value as given, its `$` references and escapes
#          ninja's to read; without it the value is the text it is to
#          hold, escaped so ninja reads it back as given.
#
# Each change is shown as the line it is made on, the name, and the value
# before and after, as written; a change the file refuses -- one ninja
# would not read, or would read as more than was asked, such as a binding a
# rule may not have -- is named with why, and then nothing is saved.
#
# Exit status: 0 when every change was made (and, with apply, saved); 1
# when one was refused, or the build, rule or pool is not in the file; 2
# when the file cannot be read or written.

BEGIN {
    if (file == "")
        file = "build.ninja"
    count = 0
    for (i = 1; i < ARGC; i++) {
        if (ARGV[i] != "")
            changes[++count] = ARGV[i]
        ARGV[i] = ""
    }

    F = ninja_open(file)
    if (F < 0) {
        print "ninja_set: " ninja_error() > "/dev/stderr"
        exit 2
    }
    scope = target_statement()
    if (scope < 0)
        exit 1

    refused = 0
    for (c = 1; c <= count; c++)
        refused += !change(changes[c])

    if (refused) {
        printf "%d of %d changes refused; nothing saved\n", refused, count
        exit 1
    }
    if (!apply) {
        printf "%d changes; nothing saved without -v apply=1\n", count
        exit 0
    }
    saved = ninja_save(F)
    if (saved < 0) {
        print "ninja_set: " ninja_error() > "/dev/stderr"
        exit 2
    }
    printf "%s: %s\n", file, (saved == 1) ? "saved" : "unchanged"
    exit 0
}

# target_statement finds the build, rule or pool the changes are to: its
# statement, 0 for the file's own variables, -1 for one not there
function target_statement(    n, kind, name) {
    if (build != "") {
        kind = "build"
        name = build
    } else if (rule != "") {
        kind = "rule"
        name = rule
    } else if (pool != "") {
        kind = "pool"
        name = pool
    } else
        return 0
    n = ninja_find(F, kind, name)
    if (n == 0) {
        printf "ninja_set: %s has no %s %s\n", file, kind, name \
               > "/dev/stderr"
        return -1
    }
    return n
}

# change makes one change: 1 kept, 0 refused
function change(operand,    eq, name, value, written) {
    eq = index(operand, "=")
    name = (eq > 0) ? substr(operand, 1, eq - 1) : operand
    value = (eq > 0) ? substr(operand, eq + 1) : ""
    written = value
    if (eq > 0 && !raw) {
        written = ninja_escape(value, "value")
        if (ninja_error() != "")
            return refuse(name, ninja_error())
    }
    if (scope > 0)
        return (eq > 0) ? set_binding(name, written) : remove_binding(name)
    return (eq > 0) ? set_variable(name, written) : remove_variable(name)
}

function refuse(name, why) {
    printf "%s: refused: %s\n", name, why
    return 0
}

# --- bindings

function set_binding(name, written,    k, before) {
    k = ninja_find_binding(F, scope, name)
    before = (k > 0) ? ninja_binding(F, scope, k, "text") : "(none)"
    if (ninja_set_binding(F, scope, name, written) < 0)
        return refuse(name, ninja_error())
    k = ninja_find_binding(F, scope, name)
    shown(ninja_binding(F, scope, k, "line"), name, before, written)
    return 1
}

function remove_binding(name,    k, before, line) {
    k = ninja_find_binding(F, scope, name)
    if (k == 0)
        return refuse(name, "no binding " name)
    before = ninja_binding(F, scope, k, "text")
    line = ninja_binding(F, scope, k, "line")
    if (ninja_remove_binding(F, scope, name) < 0)
        return refuse(name, ninja_error())
    shown(line, name, before, "(removed)")
    return 1
}

# --- variables

# the last assignment of a name among the file's statements, 0 for none
function last_variable(name,    n, found) {
    found = 0
    n = 0
    while ((n = ninja_find(F, "variable", name, n)) > 0)
        found = n
    return found
}

# where a variable not yet assigned goes: after the last variable before
# the first rule, build or pool, or first
function new_place(    n, statements, kind, place) {
    statements = ninja_statements(F)
    place = 1
    for (n = 1; n <= statements; n++) {
        kind = ninja_statement(F, n, "kind")
        if (kind == "rule" || kind == "build" || kind == "pool")
            break
        if (kind == "variable")
            place = n + 1
    }
    return place
}

function set_variable(name, written,    n, before, place) {
    n = last_variable(name)
    if (n > 0) {
        before = ninja_statement(F, n, "value_text")
        if (ninja_set_value(F, n, written) < 0)
            return refuse(name, ninja_error())
        shown(ninja_statement(F, n, "line"), name, before, written)
        return 1
    }
    if (!is_name(name))
        return refuse(name, "not a name ninja reads")
    place = new_place()
    if (ninja_insert(F, place, name " = " written "\n") < 0)
        return refuse(name, ninja_error())
    shown(ninja_statement(F, place, "line"), name, "(none)", written)
    return 1
}

function remove_variable(name,    n, line, before, removed) {
    removed = 0
    while ((n = last_variable(name)) > 0) {
        line = ninja_statement(F, n, "line")
        before = ninja_statement(F, n, "value_text")
        if (ninja_remove(F, n) < 0)
            return refuse(name, ninja_error())
        shown(line, name, before, "(removed)")
        removed++
    }
    if (!removed)
        return refuse(name, "no variable " name)
    return 1
}

# is_name: whether a text is a name ninja reads, [A-Za-z0-9_.-]+
function is_name(name) {
    return name ~ /^[A-Za-z0-9_.-]+$/
}

function shown(line, name, before, after) {
    printf "%s:%d: %s: %s -> %s\n", file, line, name, before, after
}
