# ninja_tool.awk -- ninja's own tools, answered from a build loaded as ninja
# loads it, without ninja: targets, commands, query and rules.
#
#   dawk -x ninja -f ninja_tool.awk -- [-C dir] [-f file] [-V version]
#                                     -t tool [argument...]
#
# The arguments are ninja's, and so is what each tool writes, byte for
# byte, and the first line of what it says when it fails:
#
#   -C dir      the directory ninja runs in, which the manifest and what it
#               includes are read from.
#   -f file     the manifest: build.ninja by default.
#   -V version  the ninja to answer as, from 1.11.1: the newest by default.
#               It is this script's own; ninja has no such option.
#   -t tool     one of:
#     targets [depth N]      the roots, and what they are built from, N
#                            levels down, 1 by default, 0 for all of them
#     targets all            every output, and the rule that makes it
#     targets rule [name]    the outputs of a rule, sorted; with no name,
#                            every input no edge makes
#     commands [-s] [target...]
#                            the commands that build the targets, or the
#                            defaults, each after those of its inputs; -s,
#                            each target's own alone
#     query target...        a target's edge in, its inputs in their
#                            sections, its validations, and what it is an
#                            input of and validates
#     rules [-d]             the manifest's rules, sorted, with -d each
#                            one's description
#
# A target is a path, made canonical; a path and `^` is the first output of
# the first edge that takes the path in. A dyndep file is not read -- query
# warns of one that is not there, as ninja does -- and no build or deps log
# is: the answers are the manifest's alone.
#
# Exit status: ninja's -- 0, or 1 when the manifest does not load or a tool
# refuses what it was asked -- and 2 for a tool this script does not answer.

BEGIN {
    file = "build.ninja"
    dir = ""
    version = ""
    tool = ""
    count = 0

    # ninja's options, up to the tool; the tool's arguments after it
    for (i = 1; i < ARGC; i++) {
        arg = ARGV[i]
        ARGV[i] = ""
        if (tool != "")
            args[++count] = arg
        else if (arg == "-C" || arg == "-f" || arg == "-V" || arg == "-t") {
            value = ARGV[++i]
            ARGV[i] = ""
            if (arg == "-C")
                dir = value
            else if (arg == "-f")
                file = value
            else if (arg == "-V")
                version = value
            else
                tool = value
        } else {
            print "ninja_tool: not an option: " arg > "/dev/stderr"
            exit 2
        }
    }
    if (tool != "targets" && tool != "commands" && tool != "query" &&
        tool != "rules") {
        printf "ninja_tool: no tool %s: targets, commands, query or rules\n",
               tool > "/dev/stderr"
        exit 2
    }

    g = ninja_load(file, dir, version)

    # a manifest ninja would not load, in ninja's words
    if (g < 0) {
        problem = ninja_error()
        if (problem ~ /^ninja version \(/)
            fatal(problem)
        error(problem)
    }
    warnings = split(ninja_warnings(g), warning, "\n")
    for (i = 1; i <= warnings; i++)
        print "ninja: warning: " warning[i] > "/dev/stderr"

    if (tool == "targets")
        exit targets()
    if (tool == "commands")
        exit commands()
    if (tool == "query")
        exit query()
    exit rules()
}

# what ninja writes to standard error follows what it wrote before it, as
# ninja's does, standard output flushed first
function error(message) {
    fflush()
    print "ninja: error: " message > "/dev/stderr"
    exit 1
}

function fatal(message) {
    fflush()
    print "ninja: fatal: " message > "/dev/stderr"
    exit 1
}

# the version answered as is at least major.minor
function at_least(major, minor,    v) {
    if (version == "")
        return 1
    split(version, v, ".")
    return (v[1] + 0 > major) || (v[1] + 0 == major && v[2] + 0 >= minor)
}

# an edge's inputs, explicit, implicit and order-only, a line each
function inputs_of(e,    text, part, s, n) {
    split("inputs implicit order_only", part, " ")
    text = ""
    for (s = 1; s <= 3; s++) {
        n = ninja_edge(g, e, part[s])
        if (n != "")
            text = (text == "") ? n : text "\n" n
    }
    return text
}

# an edge's outputs, explicit and implicit, a line each
function outputs_of(e,    text, n) {
    text = ninja_edge(g, e, "outputs")
    n = ninja_edge(g, e, "implicit_outputs")
    if (n != "")
        text = (text == "") ? n : text "\n" n
    return text
}

# a node's in-edge, 0 for none, by its path
function in_edge(path) {
    return ninja_node(g, ninja_find_node(g, path), "in_edge") + 0
}

# ninja's EditDistance, replacements allowed, giving up past `most`
function distance(s1, s2, most,    m, n, row, x, y, best, previous, old, v) {
    m = length(s1)
    n = length(s2)
    for (x = 0; x <= n; x++)
        row[x] = x
    for (y = 1; y <= m; y++) {
        row[0] = y
        best = y
        previous = y - 1
        for (x = 1; x <= n; x++) {
            old = row[x]
            v = previous + (substr(s1, y, 1) == substr(s2, x, 1) ? 0 : 1)
            if (row[x - 1] + 1 < v)
                v = row[x - 1] + 1
            if (row[x] + 1 < v)
                v = row[x] + 1
            row[x] = v
            previous = old
            if (v < best)
                best = v
        }
        if (best > most)
            return most + 1
    }
    return row[n]
}

# the node nearest a path, within three edits, as ninja's SpellcheckNode
# finds it; "" for none
function suggestion(path,    nodes, n, best, found, d, candidate) {
    nodes = ninja_nodes(g)
    best = 4
    found = ""
    for (n = 1; n <= nodes; n++) {
        candidate = ninja_node(g, n, "path")
        d = distance(candidate, path, 3)
        if (d < best) {
            best = d
            found = candidate
        }
    }
    return found
}

# a target named on the command line, as ninja's CollectTarget finds it:
# its node, or 0, `why` saying why not
function target(name,    path, first, node, edges, e, out, nearest) {
    why = ""
    if (name == "") {
        why = "empty path"
        return 0
    }
    path = ninja_canonical(name)
    first = (substr(path, length(path)) == "^")
    if (first)
        path = substr(path, 1, length(path) - 1)
    node = ninja_find_node(g, path)

    # from 1.14, a path not found is looked for under $builddir
    if (!node && at_least(1, 14) && ninja_variable(g, "builddir") != "" &&
        tool == "query") {
        node = ninja_find_node(g, ninja_variable(g, "builddir") "/" path)
        if (node)
            path = ninja_node(g, node, "path")
    }
    if (!node) {
        why = "unknown target '" path "'"
        if (path == "clean")
            why = why ", did you mean 'ninja -t clean'?"
        else if (path == "help")
            why = why ", did you mean 'ninja -h'?"
        else if ((nearest = suggestion(path)) != "")
            why = why ", did you mean '" nearest "'?"
        return 0
    }
    if (!first)
        return node
    edges = ninja_node(g, node, "out_edges")
    if (edges == "") {
        why = "'" path "' has no out edge"
        return 0
    }
    split(edges, e, "\n")
    split(ninja_edge(g, e[1], "outputs"), out, "\n")
    return ninja_find_node(g, out[1])
}

# the tool's arguments as getopt takes them: each option letter seen, in
# `letters`, "?" for one not among `allowed`, which is named as getopt names
# it; the rest, in order, into `operands`, their count given
function options(allowed, operands,    i, n, k, c, done) {
    letters = ""
    n = 0
    done = 0
    for (i = 1; i <= count; i++) {
        if (!done && args[i] == "--")
            done = 1
        else if (!done && args[i] ~ /^-./) {
            for (k = 2; k <= length(args[i]); k++) {
                c = substr(args[i], k, 1)
                if (index(allowed, c) == 0 && letters !~ /\?/) {
                    fflush()
                    printf "%s: invalid option -- '%s'\n", tool,
                           c > "/dev/stderr"
                }
                letters = letters ((index(allowed, c) > 0) ? c : "?")
            }
        } else
            operands[++n] = args[i]
    }
    return n
}

# the targets named, or the defaults, or the roots, into `nodes`; their
# count, or -1 with `why` saying why not
function targets_of(names, named, nodes,    n, i, list, item, k) {
    n = 0
    if (named == 0) {
        list = ninja_defaults(g)
        if (list == "")
            list = roots()
        if (list == "" && ninja_edges(g) > 0) {
            why = "could not determine root nodes of build graph"
            return -1
        }
        k = split(list, item, "\n")
        for (i = 1; i <= k; i++)
            nodes[++n] = ninja_find_node(g, item[i])
        return n
    }
    for (i = 1; i <= named; i++) {
        nodes[++n] = target(names[i])
        if (!nodes[n])
            return -1
    }
    return n
}

function roots() {
    return ninja_roots(g)
}

# --- targets

function targets(    mode, rule, depth, list) {
    mode = args[1]
    depth = 1
    if (count >= 1) {
        if (mode == "rule") {
            rule = args[2]
            return (rule == "") ? sources() : outputs_of_rule(rule)
        } else if (mode == "depth") {
            if (count > 1)
                depth = int(args[2] + 0)
        } else if (mode == "all") {
            return every_output()
        } else {
            unknown_mode(mode)
        }
    }
    list = roots()
    if (list == "" && ninja_edges(g) > 0)
        error("could not determine root nodes of build graph")
    listed(list, depth, 0)
    return 0
}

function unknown_mode(mode,    words, w, best, found, d) {
    split("rule depth all", words, " ")
    best = 4
    found = ""
    for (w = 1; w <= 3; w++) {
        d = distance(words[w], mode, 3)
        if (d < best) {
            best = d
            found = words[w]
        }
    }
    if (found != "")
        error("unknown target tool mode '" mode "', did you mean '" found \
              "'?")
    error("unknown target tool mode '" mode "'")
}

# nodes by their paths, each with the rule that makes it, and below it what
# it is made from, to a depth
function listed(list, depth, indent,    n, p, i, pad, e, k) {
    n = split(list, p, "\n")
    pad = ""
    for (k = 0; k < indent; k++)
        pad = pad "  "
    for (i = 1; i <= n; i++) {
        e = in_edge(p[i])
        if (e) {
            print pad p[i] ": " ninja_edge(g, e, "rule")
            if (depth > 1 || depth <= 0)
                listed(inputs_of(e), depth - 1, indent + 1)
        } else
            print pad p[i]
    }
}

function every_output(    edges, e, n, p, i, rule) {
    edges = ninja_edges(g)
    for (e = 1; e <= edges; e++) {
        rule = ninja_edge(g, e, "rule")
        n = split(outputs_of(e), p, "\n")
        for (i = 1; i <= n; i++)
            print p[i] ": " rule
    }
    return 0
}

function sources(    edges, e, n, p, i) {
    edges = ninja_edges(g)
    for (e = 1; e <= edges; e++) {
        n = split(inputs_of(e), p, "\n")
        for (i = 1; i <= n; i++)
            if (!in_edge(p[i]))
                print p[i]
    }
    return 0
}

function outputs_of_rule(rule,    edges, e, n, p, i, seen, sorted, m) {
    edges = ninja_edges(g)
    m = 0
    for (e = 1; e <= edges; e++) {
        if (ninja_edge(g, e, "rule") != rule)
            continue
        n = split(outputs_of(e), p, "\n")
        for (i = 1; i <= n; i++)
            if (!(p[i] in seen)) {
                seen[p[i]] = 1
                sorted[++m] = p[i]
            }
    }
    sort(sorted, m)
    for (i = 1; i <= m; i++)
        print sorted[i]
    return 0
}

# an array sorted in place, bytewise, as std::set and std::map order
function sort(a, n,    i, j, t) {
    for (i = 2; i <= n; i++) {
        t = a[i]
        for (j = i - 1; j >= 1 && before(t, a[j]); j--)
            a[j + 1] = a[j]
        a[j + 1] = t
    }
}

function before(x, y) {
    return (x "") < (y "")
}

# --- commands

function commands(    names, named, single, nodes, n, i) {
    named = options("hs", names)
    if (letters ~ /[h?]/) {
        print "usage: ninja -t commands [options] [targets]\n" \
              "\n" \
              "options:\n" \
              "  -s     only print the final command to build " \
              "[target], not the whole chain"
        return 1
    }
    single = (letters ~ /s/)
    n = targets_of(names, named, nodes)
    if (n < 0)
        error(why)
    for (i = 1; i <= n; i++)
        command_of(ninja_node(g, nodes[i], "in_edge") + 0, single)
    return 0
}

# an edge's command, after its inputs' when all are asked for; once each
function command_of(e, single,    n, p, i, command) {
    if (!e || (e in printed))
        return
    printed[e] = 1
    if (!single) {
        n = split(inputs_of(e), p, "\n")
        for (i = 1; i <= n; i++)
            command_of(in_edge(p[i]), single)
    }
    if (ninja_edge(g, e, "phony"))
        return
    command = ninja_edge_var(g, e, "command")
    if (ninja_error() != "")
        fatal(ninja_error())
    print command
}

# --- query

function query(    i, node, e, path, n, p, k, label, part, s, edges, list,
                   m, q) {
    if (count == 0)
        error("expected a target to query")
    split("inputs implicit order_only", part, " ")
    for (i = 1; i <= count; i++) {
        node = target(args[i])
        if (!node)
            error(why)
        path = ninja_node(g, node, "path")
        print path ":"
        e = ninja_node(g, node, "in_edge") + 0
        if (e) {
            dyndep_warning(e)
            print "  input: " ninja_edge(g, e, "rule")
            for (s = 1; s <= 3; s++) {
                label = (s == 1) ? "" : (s == 2) ? "| " : "|| "
                n = split(ninja_edge(g, e, part[s]), p, "\n")
                for (k = 1; k <= n; k++)
                    print "    " label p[k]
            }
            n = split(ninja_edge(g, e, "validations"), p, "\n")
            if (n > 0) {
                print "  validations:"
                for (k = 1; k <= n; k++)
                    print "    " p[k]
            }
        }
        print "  outputs:"
        m = split(ninja_node(g, node, "out_edges"), edges, "\n")
        for (k = 1; k <= m; k++) {
            n = split(outputs_of(edges[k]), q, "\n")
            for (s = 1; s <= n; s++)
                print "    " q[s]
        }
        m = split(ninja_node(g, node, "validation_edges"), edges, "\n")
        if (m > 0) {
            print "  validation for:"
            for (k = 1; k <= m; k++) {
                n = split(outputs_of(edges[k]), q, "\n")
                for (s = 1; s <= n; s++)
                    print "    " q[s]
            }
        }
    }
    return 0
}

# the warning ninja gives as it loads an edge's dyndep file, when the file
# is not there; one that is there is not read
function dyndep_warning(e,    path, file, line) {
    path = ninja_edge_var(g, e, "dyndep", 1)
    if (path == "" || (e in dyndeps))
        return
    dyndeps[e] = 1
    file = (dir != "" && path !~ /^\//) ? dir "/" path : path
    fflush()
    if ((getline line < file) < 0)
        printf "ninja: warning: loading '%s': No such file or directory\n\n",
               ninja_canonical(path) > "/dev/stderr"
    close(file)
}

# --- rules

function rules(    names, describe, i, n, list, line, description) {
    options("hd", names)
    if (letters ~ /[h?]/) {
        print "usage: ninja -t rules [options]\n" \
              "\n" \
              "options:\n" \
              "  -d     also print the description of the rule\n" \
              "  -h     print this message"
        return 1
    }
    describe = (letters ~ /d/)
    n = split(ninja_rules(g), list, "\n")
    for (i = 1; i <= n; i++) {
        line = list[i]
        if (describe) {
            description = ninja_rule(g, list[i], "description")
            if (ninja_error() == "")
                line = line ": " description
        }
        print line
    }
    return 0
}
