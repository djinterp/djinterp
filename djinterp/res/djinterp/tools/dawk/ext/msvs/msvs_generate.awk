# msvs_generate.awk -- Visual Studio projects and solutions for the tests,
# made from the CMake testing tree.
#
#   dawk -x cmake -x msvs -f msvs_generate.awk [-v root=<dir>]
#        [-v only=<rel>] [-v all=1] [-v form=slnx|sln] [-v toolset=<name>]
#        [-v template=<vcxproj>] [-v dry=1]
#
# The CMake tree is walked as CMake walks it: from
# build/cmake/config/testing, into each subdirectory of a directory whose
# CMakeLists.txt calls djinterp_add_test_subtree(). Each file is run as CMake
# would run the part of it that names a test, for a Visual Studio build
# (MSVC and WIN32 set): set, unset, option, list, string, file(GLOB),
# get_filename_component, the find_ commands, which find nothing; if,
# elseif and else, with CMake's conditions; foreach; function and macro,
# defined and called; break, continue and return; and
# djinterp_add_test_executable, djinterp_add_test_subtree, add_library,
# target_compile_definitions, target_compile_options,
# target_include_directories and set_target_properties. Any other command
# is passed over, and what cannot be followed -- a while, a replacement
# naming a regular expression's group -- is said.
# For each test, at testing/<rel>:
#     .config/.msvs/testing/<rel>/<target>.vcxproj
#     .config/.msvs/testing/<rel>/<target>.vcxproj.filters
#     .config/.msvs/testing/<rel>/<target>.vcxproj.user, when there is none
#     .config/.msvs/testing/<rel>/<target>.slnx, the test and the runtime
# for each aggregate, djinterp_add_test_subtree(<name>) at testing/<rel>:
#     .config/.msvs/testing/<rel>/<name>.slnx, every test beneath it, each in
#     the folder of its directory
# and for each library the root CMakeLists.txt adds -- djinterp_c_runtime,
# which every test links -- in the folder CMake gives it:
#     .config/.msvs/modules/<folder>/<library>.vcxproj, and its filters
#
# A project is the template's text with its Globals set -- ProjectGuid,
# RootNamespace, ProjectName, and DTestPath, testing\<rel>, which names its
# output and intermediate directories -- its djinterp.props import pointed
# at .config\.msvs\djinterp.props from where it is, the test's own
# definitions and include directories put before the template's, and its
# items: its sources, each by its path from the project; the headers of the
# include directories under inc, and those beside its sources named for the
# test; and a reference to the runtime. Its filters show each file under
# Source, Header or Test Files and the directory it is in, as
# "Test Files\djinterp\tests\djinterp\test". Every GUID is made from a name,
# so a project made again is the same project, and a file whose text would
# not change is not written.
#
#   root      the tree's root; the directory dawk runs in by default.
#   only      a part of the testing tree, djinterp/core: only the tests and
#             aggregates within it are made.
#   all       1 to make every test the tree holds, reached by an aggregate
#             or not.
#   form      slnx, by default, or sln: the form of the solutions made.
#   toolset   a PlatformToolset for every project, ClangCL or v143; the
#             template's, $(DefaultPlatformToolset), by default.
#   template  the project to start each from; msvs_test.vcxproj, beside
#             this script under res, by default.
#   dry       1 to say what would be made, and write nothing.
#
# A test within a block, which is not followed, is said on standard error.
# Exit status: 0 when all is well, 2 when a file could not be read or
# written.

BEGIN {
    if (root == "")
        root = "."
    if (form == "")
        form = "slnx"
    if (form != "slnx" && form != "sln") {
        print "msvs_generate: -v form= is slnx or sln" > "/dev/stderr"
        exit 2
    }
    if (template == "")
        template = "res/djinterp/tools/dawk/ext/msvs/msvs_test.vcxproj"

    SEP     = (form == "slnx") ? "/" : "\\"
    CONFIG  = "build/cmake/config"
    TESTING = CONFIG "/testing"
    OUT     = ".config/.msvs"
    PROPS   = OUT "/djinterp.props"
    only    = norm(only)
    status  = 0
    written = 0
    kept    = 0

    # the libraries every test links, then the tests
    evaluate(CONFIG "/CMakeLists.txt", CONFIG, 1)
    walk(TESTING)

    for (l = 1; l <= nlib; l++)
        make_library(l)
    for (t = 1; t <= ntest; t++)
        if (within(test_rel[t]))
            make_test(t)
    for (a = 1; a <= nagg; a++)
        if (within(agg_rel[a]))
            make_aggregate(a)

    printf "%d tests, %d aggregates, %d libraries: %s%d written, " \
           "%d unchanged\n", made_tests, made_aggs, nlib,
           (dry ? "(dry run) " : ""), written, kept
    exit status
}


# ---- paths ------------------------------------------------------------------

# norm(p): a path with `.` and empty parts dropped and `..` taken back; "."
# for the root.
function norm(p,    n, i, parts, out, k) {
    gsub(/\\/, "/", p)
    n = split(p, parts, "/")
    k = 0
    for (i = 1; i <= n; i++) {
        if (parts[i] == "" || parts[i] == ".")
            continue
        if (parts[i] == ".." && k > 0 && out[k] != "..") {
            k--
            continue
        }
        out[++k] = parts[i]
    }
    p = ""
    for (i = 1; i <= k; i++)
        p = p (i > 1 ? "/" : "") out[i]
    return (p == "") ? "." : p
}

# phys(p): where a path from the root is, from the directory dawk runs in.
function phys(p) {
    return (root == "." || p ~ /^\//) ? p : root "/" p
}

# unphys(p): a path found under the root, as a path from it.
function unphys(p) {
    if (root != "." && index(p, root "/") == 1)
        p = substr(p, length(root) + 2)
    return norm(p)
}

function dirname(p) {
    return (p ~ /\//) ? substr(p, 1, match(p, /\/[^\/]*$/) - 1) : "."
}

function basename(p) {
    sub(/^.*\//, "", p)
    return p
}

# within(rel): whether a part of the testing tree is one asked for.
function within(rel) {
    return (only == "." || rel == only || index(rel, only "/") == 1)
}

# under(rel, top): rel's path below top, "" for top itself, or "-" when it
# is not below it.
function under(rel, top) {
    if (top == "." || top == "")
        return rel
    if (rel == top)
        return ""
    return (index(rel, top "/") == 1) ? substr(rel, length(top) + 2) : "-"
}

# backslashed(p): a path with `\` between its parts.
function backslashed(p) {
    gsub(/\//, "\\", p)
    return p
}

# macro_dir(d): an include directory as the projects name one: by the
# djinterp.props macro for the root it lies under.
function macro_dir(d,    rest) {
    d = norm(d)
    if (d == "inc" || index(d, "inc/") == 1)
        return "$(DFrameworkIncRoot)" backslashed(substr(d, 4))
    if (d == "tests" || index(d, "tests/") == 1)
        return "$(DFrameworkTestsRoot)" backslashed(substr(d, 6))
    if (d == "src" || index(d, "src/") == 1)
        return "$(DFrameworkSrcRoot)" backslashed(substr(d, 4))
    if (d == CONFIG || index(d, CONFIG "/") == 1)
        return "$(DFrameworkCMakeConfigRoot)" \
               backslashed(substr(d, length(CONFIG) + 1))
    return "$(DFrameworkRootDir)\\" backslashed(d)
}


# ---- reading CMake ----------------------------------------------------------
# A small interpreter over the commands dcmake reads, its blocks by their
# openers' partners: if, elseif and else; foreach; function and macro,
# defined and then called; break, continue and return.  Variables are V,
# a function's saved and restored around its call, PARENT_SCOPE reaching
# the caller's.

# expand(text): ${NAME} references replaced by their values, innermost
# first; $ENV{...} and an unknown name are "".
function expand(text,    guard, name, value) {
    guard = 0
    while (match(text, /\$(ENV)?\{[^${}]*\}/) && guard++ < 1000) {
        name = substr(text, RSTART, RLENGTH)
        value = ""
        if (name !~ /^\$ENV/) {
            name = substr(name, 3, length(name) - 3)
            value = (name in V) ? V[name] : ""
        }
        text = substr(text, 1, RSTART - 1) value \
               substr(text, RSTART + RLENGTH)
    }
    return text
}

# args(n, out): command n's arguments as CMake reads them: an unquoted one
# expanded and split at `;`, a quoted one expanded and whole, a bracket one
# as written.  How many there are.
function args(n, out,    k, count, kind, value, parts, m, i) {
    count = 0
    for (k = 1; k <= cmake_args(CF, n); k++) {
        kind  = cmake_arg(CF, n, k, "kind")
        value = cmake_value(CF, n, k)
        if (kind == "bracket") {
            out[++count] = value
            continue
        }
        value = expand(value)
        if (kind == "quoted") {
            out[++count] = value
            continue
        }
        m = split(value, parts, ";")
        for (i = 1; i <= m; i++)
            if (parts[i] != "")
                out[++count] = parts[i]
    }
    return count
}

# list_of(name, out): a variable's list.  How many it holds.
function list_of(name, out,    m, parts, i, count) {
    count = 0
    m = split((name in V) ? V[name] : "", parts, ";")
    for (i = 1; i <= m; i++)
        if (parts[i] != "")
            out[++count] = parts[i]
    return count
}

# joined(items, from, to): items from..to as a CMake list.
function joined(items, from, to,    i, value) {
    value = ""
    for (i = from; i <= to; i++)
        value = value (i > from ? ";" : "") items[i]
    return value
}

# assign(name, value): a variable set where the command runs, or, with
# PARENT_SCOPE, where the function was called from.
function assign(name, value, parent) {
    # the roots are the generator's, whatever a file says
    if (name ~ /^DJINTERP_(PROJECT|INC|SRC|TESTS|BIN|CONFIG)_ROOT$/)
        return
    if (parent) {
        if (DEPTH > 0)
            PS[DEPTH, name] = value
        return
    }
    V[name] = value
}

# glob(pattern, out, count): adds the files a pattern names, from the root;
# the new count.
function glob(pattern, out, count,    found, m, parts, i) {
    found = msvs_glob(phys(norm(pattern)))
    m = split(found, parts, "\n")
    for (i = 1; i <= m; i++)
        if (parts[i] != "")
            out[++count] = unphys(parts[i])
    return count
}

# sort_list(items, count): items in byte order, as CMake sorts.
function sort_list(items, count,    i, j, held) {
    for (i = 2; i <= count; i++) {
        held = items[i]
        for (j = i - 1; j >= 1 && items[j] > held; j--)
            items[j + 1] = items[j]
        items[j + 1] = held
    }
}

# keyword(word): whether an argument is one of a command's keywords.
function keyword(word) {
    return (word ~ /^(SOURCES|DEFINES|INCLUDES|LIBRARIES|PRIVATE|PUBLIC)$/ ||
            word == "INTERFACE")
}

# target_named(name): the test or library of that name: "t<number>",
# "l<number>", or "".
function target_named(name,    t) {
    for (t = 1; t <= ntest; t++)
        if (test_name[t] == name)
            return "t" t
    for (t = 1; t <= nlib; t++)
        if (lib_name[t] == name)
            return "l" t
    return ""
}

# add_to(target, what, value): a definition, option or include directory
# given a test or a library after it was made.
function add_to(target, what, value,    kind, n) {
    kind = substr(target, 1, 1)
    n = substr(target, 2) + 0
    if (kind == "t")
        test_add(n, what, value)
    else if (kind == "l")
        lib_add(n, what, value)
}

function test_add(t, what, value) {
    TEST[t, what, ++TEST[t, what]] = value
}

function lib_add(l, what, value) {
    LIB[l, what, ++LIB[l, what]] = value
}

# ---- conditions ----

# falsy(value): a constant CMake holds false.
function falsy(value,    upper) {
    upper = toupper(value)
    return (value == "" || upper ~ /^(0|OFF|NO|FALSE|N|IGNORE|NOTFOUND)$/ ||
            upper ~ /-NOTFOUND$/)
}

# truthy(value): a constant CMake holds true.
function truthy(value,    upper) {
    upper = toupper(value)
    return (upper ~ /^(ON|YES|TRUE|Y)$/ ||
            (value ~ /^-?[0-9.]+$/ && value + 0 != 0))
}

# deref(word): a word's value, the variable it names when it names one.
function deref(word) {
    return (word in V) ? V[word] : word
}

# condition(a, count): what if() makes of its arguments: parentheses, NOT,
# AND and OR, the unary tests and the binary comparisons.
function condition(a, count,    i) {
    delete CT
    for (i = 1; i <= count; i++)
        CT[i] = a[i]
    CN = count
    CP = 1
    return cond_or()
}

function cond_or(    value) {
    value = cond_and()
    while (CP <= CN && CT[CP] == "OR") {
        CP++
        value = cond_and() || value
    }
    return value
}

function cond_and(    value) {
    value = cond_not()
    while (CP <= CN && CT[CP] == "AND") {
        CP++
        value = cond_not() && value
    }
    return value
}

function cond_not() {
    if (CP <= CN && CT[CP] == "NOT") {
        CP++
        return !cond_not()
    }
    return cond_primary()
}

function cond_primary(    word, op, right, left, list, m, parts, i) {
    word = CT[CP++]
    if (word == "(") {
        word = cond_or()
        if (CT[CP] == ")")
            CP++
        return word
    }
    if (word == "EXISTS")
        return msvs_exists(phys(norm(CT[CP++]))) > 0
    if (word == "IS_DIRECTORY")
        return msvs_exists(phys(norm(CT[CP++]))) == 2
    if (word == "DEFINED")
        return (CT[CP++] in V)
    if (word == "TARGET")
        return target_named(CT[CP++]) != ""
    if (word == "COMMAND")
        return (tolower(CT[CP++]) in FN_START)
    if (word == "IS_ABSOLUTE")
        return (CT[CP++] ~ /^(\/|[A-Za-z]:)/)
    op = (CP <= CN) ? CT[CP] : ""
    if (op ~ /^(STREQUAL|EQUAL|LESS|GREATER|LESS_EQUAL|GREATER_EQUAL)$/ ||
        op ~ /^(STRLESS|STRGREATER|MATCHES|IN_LIST)$/) {
        CP++
        right = CT[CP++]
        left = deref(word)
        if (op == "STREQUAL")
            return left == deref(right)
        if (op == "EQUAL")
            return left + 0 == deref(right) + 0
        if (op == "LESS")
            return left + 0 < deref(right) + 0
        if (op == "GREATER")
            return left + 0 > deref(right) + 0
        if (op == "LESS_EQUAL")
            return left + 0 <= deref(right) + 0
        if (op == "GREATER_EQUAL")
            return left + 0 >= deref(right) + 0
        if (op == "STRLESS")
            return left < deref(right)
        if (op == "STRGREATER")
            return left > deref(right)
        if (op == "MATCHES")
            return left ~ right
        list = (right in V) ? V[right] : ""
        m = split(list, parts, ";")
        for (i = 1; i <= m; i++)
            if (parts[i] == word)
                return 1
        return 0
    }
    # a constant, or the variable a word names
    if (truthy(word))
        return 1
    if (falsy(word))
        return 0
    return (word in V) && !falsy(V[word])
}

# ---- commands ----

# run(first, last): the commands from first to last, each block whole.
function run(first, last,    n, role, name, ender) {
    n = first
    while (n <= last && !BREAK && !CONTINUE && !RETURN) {
        role = cmake_command(CF, n, "role")
        name = tolower(cmake_name(CF, n))
        if (role == "open") {
            ender = cmake_command(CF, n, "partner")
            if (ender <= n)
                ender = last + 1
            if (name == "if")
                run_if(n, ender)
            else if (name == "foreach")
                run_foreach(n, ender)
            else if (name == "function" || name == "macro")
                define(n, ender, name == "macro")
            else if (name == "while")
                PASSED = PASSED " while@" cmake_command(CF, n, "line")
            else
                run(n + 1, ender - 1)
            n = ender + 1
            continue
        }
        if (role == "")
            command(n, name)
        n++
    }
}

# run_if(open, ender): the first branch whose condition holds.
function run_if(open, ender,    at, b, k, i, a, count, to) {
    k = 0
    b[++k] = open
    for (at = open + 1; at < ender; at++)
        if (cmake_command(CF, at, "role") == "branch" &&
            cmake_command(CF, at, "partner") == open)
            b[++k] = at
    for (i = 1; i <= k; i++) {
        to = (i < k) ? b[i + 1] - 1 : ender - 1
        if (tolower(cmake_name(CF, b[i])) == "else") {
            run(b[i] + 1, to)
            return
        }
        delete a
        count = args(b[i], a)
        if (condition(a, count)) {
            run(b[i] + 1, to)
            return
        }
    }
}

# run_foreach(open, ender): the body once for each item, with its variable
# set: items, IN LISTS, IN ITEMS, or RANGE.
function run_foreach(open, ender,    a, count, var, items, m, i, j, mode, \
                     parts, p, low, high, step) {
    delete a
    count = args(open, a)
    var = a[1]
    m = 0
    mode = "items"
    if (a[2] == "RANGE") {
        low = (count >= 4) ? a[3] + 0 : 0
        high = (count >= 4) ? a[4] + 0 : a[3] + 0
        step = (count >= 5) ? a[5] + 0 : 1
        for (i = low; step > 0 && i <= high; i += step)
            items[++m] = i
    }
    else {
        for (i = 2; i <= count; i++) {
            if (a[i] == "IN")
                continue
            if (a[i] == "LISTS" || a[i] == "ITEMS" || a[i] == "ZIP_LISTS") {
                mode = tolower(a[i])
                continue
            }
            if (mode == "lists") {
                p = split((a[i] in V) ? V[a[i]] : "", parts, ";")
                for (j = 1; j <= p; j++)
                    if (parts[j] != "")
                        items[++m] = parts[j]
            }
            else
                items[++m] = a[i]
        }
    }
    for (i = 1; i <= m && !RETURN; i++) {
        V[var] = items[i]
        run(open + 1, ender - 1)
        CONTINUE = 0
        if (BREAK) {
            BREAK = 0
            break
        }
    }
}

# define(open, ender, macro): a function or macro, to be called later.
function define(open, ender, macro,    a, count, name, i) {
    delete a
    count = args(open, a)
    name = tolower(a[1])
    FN_START[name] = open + 1
    FN_END[name]   = ender - 1
    FN_MACRO[name] = macro
    FN_COUNT[name] = count - 1
    for (i = 2; i <= count; i++)
        FN_PARAM[name, i - 1] = a[i]
}

# invoke(name, a, count): a function or macro called: its parameters set,
# ARGC, ARGV and ARGN, its body run; a function's variables then put back,
# but for what it set PARENT_SCOPE.
function invoke(name, a, count,    saved, key, i, rest, parts) {
    if (!FN_MACRO[name]) {
        for (key in V)
            saved[key] = V[key]
    }
    DEPTH++
    for (i = 1; i <= FN_COUNT[name]; i++)
        V[FN_PARAM[name, i]] = (i <= count) ? a[i] : ""
    V["ARGC"] = count
    V["ARGV"] = joined(a, 1, count)
    V["ARGN"] = joined(a, FN_COUNT[name] + 1, count)
    for (i = 1; i <= count; i++)
        V["ARGV" (i - 1)] = a[i]
    run(FN_START[name], FN_END[name])
    RETURN = 0
    if (!FN_MACRO[name]) {
        delete V
        for (key in saved)
            V[key] = saved[key]
    }
    for (key in PS) {
        split(key, parts, SUBSEP)
        if (parts[1] == DEPTH) {
            V[parts[2]] = PS[key]
            delete PS[key]
        }
    }
    DEPTH--
}

# command(n, name): one command, outside any block or in one being run.
function command(n, name,    a, count, i, k, items, m, var, mode, part, \
                 kept_items, keep, held, target, what, value, parent) {
    delete a
    count = args(n, a)

    if (name in FN_START) {
        invoke(name, a, count)
        return
    }
    if (name == "return") {
        RETURN = 1
        return
    }
    if (name == "break") {
        BREAK = 1
        return
    }
    if (name == "continue") {
        CONTINUE = 1
        return
    }
    if (name == "message") {
        if (a[1] == "FATAL_ERROR" || a[1] == "SEND_ERROR")
            RETURN = 1
        return
    }

    if (name == "set" && count >= 1) {
        m = 0
        parent = 0
        delete items
        for (i = 2; i <= count; i++) {
            if (a[i] == "CACHE")
                break
            if (a[i] == "PARENT_SCOPE") {
                parent = 1
                break
            }
            items[++m] = a[i]
        }
        assign(a[1], joined(items, 1, m), parent)
    }
    else if (name == "unset" && count >= 1) {
        if (a[2] == "PARENT_SCOPE" && DEPTH > 0)
            PS[DEPTH, a[1]] = ""
        else
            delete V[a[1]]
    }
    else if (name == "option" && count >= 2) {
        if (!(a[1] in V))
            V[a[1]] = (count >= 3) ? a[3] : "OFF"
    }
    else if (name == "find_library" || name == "find_path" ||
             name == "find_program" || name == "find_file") {
        if (!(a[1] in V))
            V[a[1]] = a[1] "-NOTFOUND"
    }
    else if (name == "get_filename_component" && count >= 3) {
        mode = a[3]
        part = norm(a[2])
        if (mode == "DIRECTORY" || mode == "PATH")
            part = dirname(part)
        else if (mode == "NAME")
            part = basename(part)
        else if (mode == "NAME_WE") {
            part = basename(part)
            sub(/\..*$/, "", part)
        }
        else if (mode == "EXT") {
            part = basename(part)
            part = (match(part, /\./)) ? substr(part, RSTART) : ""
        }
        assign(a[1], part, 0)
    }
    else if (name == "string" && count >= 3)
        command_string(a, count)
    else if (name == "file" && count >= 2 && a[1] == "GLOB") {
        m = 0
        delete items
        for (i = 3; i <= count; i++) {
            if (a[i] == "CONFIGURE_DEPENDS" || a[i] == "FOLLOW_SYMLINKS")
                continue
            if (a[i] == "LIST_DIRECTORIES" || a[i] == "RELATIVE") {
                i++
                continue
            }
            m = glob(a[i], items, m)
        }
        sort_list(items, m)
        assign(a[2], joined(items, 1, m), 0)
    }
    else if (name == "list" && count >= 2)
        command_list(a, count)
    else if (name == "djinterp_add_test_executable" && count >= 1) {
        ntest++
        test_name[ntest] = a[1]
        test_dir[ntest]  = DIR
        test_rel[ntest]  = under(DIR, TESTING)
        what = ""
        for (i = 2; i <= count; i++) {
            if (keyword(a[i])) {
                what = tolower(a[i])
                continue
            }
            # a generator expression is CMake's alone to read
            if (a[i] ~ /\$</)
                continue
            if (what == "sources")
                test_add(ntest, "source", norm(a[i]))
            else if (what == "includes")
                test_add(ntest, "include", norm(a[i]))
            else if (what == "defines")
                test_add(ntest, "define", a[i])
            else if (what == "libraries")
                test_add(ntest, "library", a[i])
        }
    }
    else if (name == "add_library" && TOP && count >= 2 &&
             a[2] !~ /^(INTERFACE|IMPORTED|ALIAS)$/) {
        nlib++
        lib_name[nlib]   = a[1]
        lib_folder[nlib] = ""
        for (i = 2; i <= count; i++)
            if (a[i] !~ /^(STATIC|SHARED|OBJECT|MODULE|EXCLUDE_FROM_ALL)$/ &&
                a[i] !~ /\$</)
                lib_add(nlib, "source", norm(a[i]))
    }
    else if ((name == "target_compile_definitions" ||
              name == "target_compile_options" ||
              name == "target_include_directories") && count >= 2) {
        target = target_named(a[1])
        what = "include"
        if (name == "target_compile_definitions")
            what = "define"
        else if (name == "target_compile_options")
            what = "option"
        for (i = 2; target != "" && i <= count; i++) {
            if (keyword(a[i]) || a[i] ~ /\$</ || a[i] == "SYSTEM" ||
                a[i] == "BEFORE" || a[i] == "AFTER")
                continue
            # an option is MSVC's when it begins with `/`
            if (what == "option" && a[i] !~ /^\//)
                continue
            add_to(target, what, (what == "include") ? norm(a[i]) : a[i])
        }
    }
    else if (name == "set_target_properties" && count >= 4) {
        target = target_named(a[1])
        for (i = 2; i < count; i++)
            if (a[i] == "FOLDER" && substr(target, 1, 1) == "l")
                lib_folder[substr(target, 2) + 0] = a[i + 1]
    }
    else if (name == "djinterp_add_test_subtree" && count >= 1)
        aggregate = a[1]
}

# command_list(a, count): list(APPEND|FILTER|REMOVE_ITEM|SORT|
# REMOVE_DUPLICATES|LENGTH|GET ...).
function command_list(a, count,    mode, var, items, m, i, k, kept_items, \
                      keep, held, seen) {
    mode = a[1]
    var  = a[2]
    delete items
    m = list_of(var, items)
    if (mode == "LENGTH") {
        assign(a[3], m, 0)
        return
    }
    if (mode == "GET") {
        held = ""
        for (i = 3; i < count; i++) {
            k = a[i] + 0
            k = (k < 0) ? m + k + 1 : k + 1
            held = held (held != "" ? ";" : "") items[k]
        }
        assign(a[count], held, 0)
        return
    }
    if (mode == "APPEND")
        for (i = 3; i <= count; i++)
            items[++m] = a[i]
    else if (mode == "FILTER" && count >= 5) {
        k = 0
        for (i = 1; i <= m; i++) {
            keep = (items[i] ~ a[5]) ? 1 : 0
            if (a[3] == "EXCLUDE")
                keep = !keep
            if (keep)
                kept_items[++k] = items[i]
        }
        delete items
        for (i = 1; i <= k; i++)
            items[i] = kept_items[i]
        m = k
    }
    else if (mode == "REMOVE_ITEM" || mode == "REMOVE_DUPLICATES") {
        k = 0
        for (i = 1; i <= m; i++) {
            keep = !(items[i] in seen) || mode == "REMOVE_ITEM"
            for (held = 3; mode == "REMOVE_ITEM" && held <= count; held++)
                if (items[i] == a[held])
                    keep = 0
            seen[items[i]] = 1
            if (keep)
                kept_items[++k] = items[i]
        }
        delete items
        for (i = 1; i <= k; i++)
            items[i] = kept_items[i]
        m = k
    }
    else if (mode == "SORT")
        sort_list(items, m)
    else
        return
    assign(var, joined(items, 1, m), 0)
}

# command_string(a, count): string(REPLACE|REGEX REPLACE|TOLOWER|TOUPPER|
# APPEND|CONCAT ...); a replacement naming a group is not followed.
function command_string(a, count,    mode, input, i, out, re, with, done) {
    mode = a[1]
    if (mode == "TOLOWER" || mode == "TOUPPER") {
        assign(a[3], (mode == "TOLOWER") ? tolower(a[2]) : toupper(a[2]), 0)
        return
    }
    if (mode == "APPEND" || mode == "CONCAT") {
        out = (mode == "APPEND" && (a[2] in V)) ? V[a[2]] : ""
        for (i = 3; i <= count; i++)
            out = out a[i]
        assign(a[2], out, 0)
        return
    }
    if (mode == "REPLACE" && count >= 4) {
        input = ""
        for (i = 5; i <= count; i++)
            input = input a[i]
        out = ""
        while (a[2] != "" && (i = index(input, a[2])) > 0) {
            out = out substr(input, 1, i - 1) a[3]
            input = substr(input, i + length(a[2]))
        }
        assign(a[4], out input, 0)
        return
    }
    if (mode == "REGEX" && a[2] == "REPLACE" && count >= 5) {
        re = a[3]
        with = a[4]
        input = ""
        for (i = 6; i <= count; i++)
            input = input a[i]
        if (with ~ /\\[0-9]/) {
            PASSED = PASSED " string(REGEX REPLACE \\N)@" CURRENT_LINE
            assign(a[5], input, 0)
            return
        }
        out = ""
        done = 0
        while (input != "" && match(input, re) && done++ < 10000) {
            out = out substr(input, 1, RSTART - 1) with
            if (RLENGTH == 0) {
                out = out substr(input, RSTART, 1)
                input = substr(input, RSTART + 1)
            }
            else
                input = substr(input, RSTART + RLENGTH)
            # an anchored pattern matches once
            if (re ~ /^\^/)
                break
        }
        assign(a[5], out input, 0)
    }
}

# evaluate(file, dir, top): runs a CMakeLists.txt; `top` for the root's,
# where the libraries are.  Whether it calls djinterp_add_test_subtree is
# left in `aggregate`.
function evaluate(file, dir, top) {
    delete V
    delete FN_START
    delete FN_END
    delete FN_MACRO
    delete FN_COUNT
    delete FN_PARAM
    V["CMAKE_CURRENT_SOURCE_DIR"] = dir
    V["CMAKE_CURRENT_LIST_DIR"]   = dir
    V["CMAKE_SOURCE_DIR"]         = CONFIG
    V["CMAKE_CURRENT_LIST_FILE"]  = file
    V["DJINTERP_PROJECT_ROOT"]    = "."
    V["DJINTERP_INC_ROOT"]        = "inc"
    V["DJINTERP_SRC_ROOT"]        = "src"
    V["DJINTERP_TESTS_ROOT"]      = "tests"
    V["DJINTERP_BIN_ROOT"]        = "bin"
    V["DJINTERP_CONFIG_ROOT"]     = CONFIG
    V["MSVC"]                     = "1"
    V["WIN32"]                    = "1"
    V["BUILD_TESTING"]            = "ON"
    V["CMAKE_CONFIGURATION_TYPES"] = "Debug;Release"
    V["CMAKE_C_COMPILER_ID"]      = (toolset == "ClangCL") ? "Clang" : "MSVC"
    V["CMAKE_CXX_COMPILER_ID"]    = V["CMAKE_C_COMPILER_ID"]
    V["CMAKE_CXX_COMPILER_FRONTEND_VARIANT"] = "MSVC"
    aggregate = ""
    PASSED    = ""
    DIR       = dir
    TOP       = top
    BREAK     = 0
    CONTINUE  = 0
    RETURN    = 0
    DEPTH     = 0

    # the file's handle, CF, is what every cmake_* call here reads
    CF = cmake_open(phys(file))
    if (CF < 0) {
        print "msvs_generate: " cmake_error() > "/dev/stderr"
        status = 2
        return
    }

    run(1, cmake_commands(CF))
    cmake_close(CF)

    # what was not followed is said
    if (PASSED != "")
        printf "msvs_generate: %s: not followed:%s\n", file, PASSED \
               > "/dev/stderr"
}

# walk(dir): a directory of the testing tree, and, when it is an aggregate
# or every test is asked for, each directory in it.
function walk(dir,    file, found, m, subs, i) {
    file = dir "/CMakeLists.txt"
    aggregate = ""
    if (msvs_exists(phys(file)) == 1) {
        evaluate(file, dir, 0)
        if (aggregate != "") {
            nagg++
            agg_name[nagg] = aggregate
            agg_dir[nagg]  = dir
            agg_rel[nagg]  = under(dir, TESTING)
        }
    }

    # CMake goes on only from an aggregate
    if (aggregate == "" && !all)
        return

    found = msvs_glob(phys(dir) "/*", "dirs")
    m = split(found, subs, "\n")
    for (i = 1; i <= m; i++)
        if (subs[i] != "")
            walk(unphys(subs[i]))
}


# ---- writing ----------------------------------------------------------------

# save(e, what): saves a file, counting it; false when it could not be.
function save(e, what,    result) {
    if (dry) {
        printf "would write %s\n", vcxproj_path(e)
        written++
        vcxproj_close(e)
        return 1
    }
    result = vcxproj_save(e)
    if (result < 0) {
        print "msvs_generate: " what ": " vcxproj_error() > "/dev/stderr"
        status = 2
        vcxproj_close(e)
        return 0
    }
    if (result == 1)
        written++
    else
        kept++
    vcxproj_close(e)
    return 1
}

function save_solution(s, what,    result) {
    if (dry) {
        printf "would write %s\n", sln_path(s)
        written++
        sln_close(s)
        return 1
    }
    result = sln_save(s)
    if (result < 0) {
        print "msvs_generate: " what ": " sln_error() > "/dev/stderr"
        status = 2
    }
    else if (result == 1)
        written++
    else
        kept++
    sln_close(s)
    return (result > 0)
}

# child_text(parent, name, text): the child of that name given the text,
# made last when there is none.
function child_text(parent, name, text,    c) {
    c = child_named(parent, name)
    if (c)
        return vcxproj_set_text(c, text)
    return vcxproj_add(parent, name, text) > 0
}

# child_named(parent, name): the first child of that name, or 0.
function child_named(parent, name,    c) {
    for (c = vcxproj_first(parent); c; c = vcxproj_next(c))
        if (vcxproj_name(c) == name)
            return c
    return 0
}

# globals(p, name, path): a project's Globals set for a name and a path in
# the testing tree or the modules.
function globals(p, name, path, guid,    g, space) {
    for (g = vcxproj_find(p, "PropertyGroup"); g;
         g = vcxproj_find(p, "PropertyGroup", g))
        if (vcxproj_attr(g, "Label") == "Globals")
            break
    if (!g)
        g = vcxproj_add(p, "PropertyGroup", "", "Label", "Globals")
    space = name
    gsub(/[^A-Za-z0-9_]/, "_", space)
    child_text(g, "ProjectGuid", "{" guid "}")
    child_text(g, "RootNamespace", space)
    child_text(g, "ProjectName", name)
    child_text(g, "DTestPath", backslashed(path))
}

# point_props(p, from): every import of djinterp.props pointed at the props
# from the project's directory.
function point_props(p, from,    e, relative) {
    relative = msvs_relative(from, PROPS)
    for (e = vcxproj_find(p, "Import"); e; e = vcxproj_find(p, "Import", e))
        if (vcxproj_attr(e, "Project") ~ /djinterp\.props$/)
            vcxproj_set_attr(e, "Project", relative)
}

# before_each(p, name, list): `list`, ending in `;`, put before the text of
# every element of that name in a ClCompile of an ItemDefinitionGroup,
# leaving out what the text holds already.
function before_each(p, name, list,    e, text, m, parts, i, put) {
    for (e = vcxproj_find(p, name); e; e = vcxproj_find(p, name, e)) {
        if (vcxproj_name(vcxproj_parent(e)) != "ClCompile")
            continue
        text = vcxproj_text(e)
        put = ""
        m = split(list, parts, ";")
        for (i = 1; i <= m; i++)
            if (parts[i] != "" && index(";" text ";", ";" parts[i] ";") == 0)
                put = put parts[i] ";"
        if (put != "")
            vcxproj_set_text(e, put text)
    }
}

# each_text(p, name, text): every element of that name given the text.
function each_text(p, name, text,    e) {
    for (e = vcxproj_find(p, name); e; e = vcxproj_find(p, name, e))
        vcxproj_set_text(e, text)
}

# category(path): the filter a file shows under, by where it is and what it
# is.
function category(path) {
    if (index(path, "tests/") == 1)
        return "Test Files"
    if (path ~ /\.(h|hh|hpp|hxx|h\+\+|inl|ipp|tpp)$/)
        return "Header Files"
    return "Source Files"
}

# filter_of(path): the filter a file shows in: its category, then
# djinterp and the directory it is in.
function filter_of(path,    d) {
    d = dirname(path)
    return category(path) "\\djinterp" ((d == ".") ? "" : "\\" backslashed(d))
}

# is_header(path): whether a file is included rather than compiled.
function is_header(path) {
    return (path ~ /\.(h|hh|hpp|hxx|h\+\+|inl|ipp|tpp)$/)
}

# standard_filters(f): the three filters Visual Studio begins a project
# with, with its own identifiers, and the tests', in the order every filter
# is then kept in.
function standard_filters(f,    g, e) {
    g = vcxproj_add(f, "ItemGroup")
    e = vcxproj_add(g, "Filter", "", "Include", "Header Files")
    vcxproj_add(e, "UniqueIdentifier", "{93995380-89BD-4b04-88EB-625FBE52EBFB}")
    vcxproj_add(e, "Extensions", "h;hh;hpp;hxx;h++;hm;inl;inc;ipp;xsd")
    e = vcxproj_add(g, "Filter", "", "Include", "Resource Files")
    vcxproj_add(e, "UniqueIdentifier", "{67DA6AB6-F800-4c08-8B7A-83BB121AAD01}")
    vcxproj_add(e, "Extensions",
                "rc;ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe;resx;" \
                "tiff;tif;png;wav;mfcribbon-ms")
    e = vcxproj_add(g, "Filter", "", "Include", "Source Files")
    vcxproj_add(e, "UniqueIdentifier", "{4FC737F1-C7A5-4376-A066-2A32D752A2FF}")
    vcxproj_add(e, "Extensions",
                "cpp;c;cc;cxx;c++;cppm;ixx;def;odl;idl;hpj;bat;asm;asmx")
    e = vcxproj_add(g, "Filter", "", "Include", "Test Files")
    vcxproj_add(e, "UniqueIdentifier", "{" msvs_guid("filter:Test Files") "}")
}

# add_file(p, f, from, path): a file, to a project and to its filters.
function add_file(p, f, from, path,    type, include) {
    type = is_header(path) ? "ClInclude" : "ClCompile"
    include = msvs_relative(from, path)
    if (vcxproj_add_item(p, type, include) < 0 ||
        vcxproj_add_item(f, type, include, filter_of(path)) < 0) {
        print "msvs_generate: " path ": " vcxproj_error() > "/dev/stderr"
        status = 2
    }
}

# headers(dir, stem, out, count): the headers in a directory, those whose
# names begin with the stem when one is given; the new count.
function headers(dir, stem, out, count,    patterns, m, i) {
    m = split("*.h *.hh *.hpp *.hxx *.inl *.ipp *.tpp", patterns, " ")
    for (i = 1; i <= m; i++)
        count = glob(dir "/" stem patterns[i], out, count)
    return count
}

# files_of(t, out): a test's files -- its sources; its include directories'
# headers under inc; the headers beside its sources and in its include
# directories under tests, named for it -- each once, in order.  How many.
function files_of(t, out,    count, i, stem, seen, found, m, k, d, dirs, nd) {
    count = 0
    stem = test_name[t]
    sub(/_tests$/, "", stem)
    for (i = 1; i <= TEST[t, "source"]; i++) {
        out[++count] = TEST[t, "source", i]
        d = dirname(TEST[t, "source", i])
        if (!(d in dirs) && index(d, "build/") != 1)
            dirs[d] = ++nd
    }
    for (i = 1; i <= TEST[t, "include"]; i++) {
        d = TEST[t, "include", i]
        m = 0
        delete found
        if (index(d, "inc/") == 1)
            m = headers(d, "", found, 0)
        else if (index(d, "tests/") == 1 && !(d in dirs))
            dirs[d] = ++nd
        for (k = 1; k <= m; k++)
            out[++count] = found[k]
    }
    for (d in dirs) {
        m = 0
        delete found
        m = headers(d, stem, found, 0)
        for (k = 1; k <= m; k++)
            out[++count] = found[k]
    }
    k = 0
    for (i = 1; i <= count; i++)
        if (!(out[i] in seen)) {
            seen[out[i]] = 1
            out[++k] = out[i]
        }
    return k
}

# library_path(l): where a library's project goes, under the modules.
function library_path(l,    folder) {
    folder = (lib_folder[l] != "") ? lib_folder[l] : "djinterp"
    return OUT "/modules/" norm(folder) "/" lib_name[l] ".vcxproj"
}

function library_guid(l) {
    return msvs_guid("djinterp:" library_path(l))
}

# reference(p, from, l): a project's reference to a library's.
function reference(p, from, l,    r) {
    r = vcxproj_add_item(p, "ProjectReference",
                         msvs_relative(from, library_path(l)))
    if (r > 0)
        child_text(r, "Project", "{" library_guid(l) "}")
}

# new_project(path, what): a project begun from the template; 0 when it
# could not be.
function new_project(path, what,    p) {
    p = vcxproj_new(phys(path), phys(template))
    if (p < 0) {
        print "msvs_generate: " what ": " vcxproj_error() > "/dev/stderr"
        status = 2
        return 0
    }
    if (toolset != "")
        each_text(p, "PlatformToolset", toolset)
    return p
}

# without_tests(p): a project's definitions with the tests' taken out, for a
# library's project.
function without_tests(p,    e, text, m, parts, i, put) {
    for (e = vcxproj_find(p, "PreprocessorDefinitions"); e;
         e = vcxproj_find(p, "PreprocessorDefinitions", e)) {
        m = split(vcxproj_text(e), parts, ";")
        put = ""
        for (i = 1; i <= m; i++)
            if (parts[i] !~ /^D_TEST(ING|_OUTPUT_DIR)=/ && parts[i] != "")
                put = put (put != "" ? ";" : "") parts[i]
        vcxproj_set_text(e, put)
    }
}

# make_library(l): a library's project and its filters.
function make_library(l,    path, from, p, f, i, e) {
    path = library_path(l)
    from = dirname(path)
    p = new_project(path, lib_name[l])
    if (!p)
        return
    globals(p, lib_name[l], substr(from, length(OUT) + 2), library_guid(l))
    point_props(p, from)
    each_text(p, "ConfigurationType", "StaticLibrary")
    each_text(p, "OutDir", "$(DFrameworkLibRoot)\\$(Platform)-$(Configuration)\\")
    for (e = vcxproj_find(p, "Link"); e; e = vcxproj_find(p, "Link", e))
        if (vcxproj_name(vcxproj_parent(e)) == "ItemDefinitionGroup")
            vcxproj_remove(e)
    without_tests(p)
    f = vcxproj_new(phys(path ".filters"))
    standard_filters(f)
    for (i = 1; i <= LIB[l, "source"]; i++)
        add_file(p, f, from, LIB[l, "source", i])
    save(p, path)
    save(f, path ".filters")
}

# make_test(t): a test's project, filters, user file and solution.
function make_test(t,    from, path, p, f, u, count, files, i, list, l, \
                   s, guid, e) {
    from = OUT "/testing/" test_rel[t]
    path = from "/" test_name[t] ".vcxproj"
    guid = msvs_guid("djinterp:" path)
    p = new_project(path, test_name[t])
    if (!p)
        return
    made_tests++
    globals(p, test_name[t], "testing/" test_rel[t], guid)
    point_props(p, from)

    list = ""
    for (i = 1; i <= TEST[t, "define"]; i++)
        list = list TEST[t, "define", i] ";"
    if (list != "")
        before_each(p, "PreprocessorDefinitions", list)
    list = ""
    for (i = 1; i <= TEST[t, "include"]; i++)
        list = list macro_dir(TEST[t, "include", i]) ";"
    if (list != "")
        before_each(p, "AdditionalIncludeDirectories", list)
    list = ""
    for (i = 1; i <= TEST[t, "option"]; i++)
        list = list (i > 1 ? " " : "") TEST[t, "option", i]
    if (list != "")
        for (e = vcxproj_find(p, "ClCompile"); e;
             e = vcxproj_find(p, "ClCompile", e))
            if (vcxproj_name(vcxproj_parent(e)) == "ItemDefinitionGroup")
                child_text(e, "AdditionalOptions",
                           list " %(AdditionalOptions)")

    f = vcxproj_new(phys(path ".filters"))
    standard_filters(f)
    count = files_of(t, files)
    for (i = 1; i <= count; i++)
        add_file(p, f, from, files[i])

    # the runtime every test links, and any library it names
    for (l = 1; l <= nlib; l++)
        reference(p, from, l)

    save(p, path)
    save(f, path ".filters")

    if (msvs_exists(phys(path ".user")) == 0) {
        u = vcxproj_new(phys(path ".user"))
        vcxproj_add(u, "PropertyGroup")
        save(u, path ".user")
    }

    s = new_solution(from "/" test_name[t] "." form)
    if (!s)
        return
    sln_add_project(s, test_name[t] ".vcxproj", "", guid)
    add_libraries(s, from)
    save_solution(s, test_name[t])
}

# new_solution(path): a solution of the form asked for, building Debug and
# Release for x64 and x86; 0 when it could not be made.
function new_solution(path,    s) {
    s = sln_new(phys(path))
    if (s < 0) {
        print "msvs_generate: " path ": " sln_error() > "/dev/stderr"
        status = 2
        return 0
    }
    sln_add_config(s, "Debug|x64")
    sln_add_config(s, "Release|x64")
    sln_add_config(s, "Debug|x86")
    sln_add_config(s, "Release|x86")
    return s
}

# add_libraries(s, from): the libraries' projects, in a folder of their own.
function add_libraries(s, from,    l, relative) {
    for (l = 1; l <= nlib; l++) {
        relative = msvs_relative(from, library_path(l), SEP)
        sln_add_project(s, relative, "runtime", library_guid(l))
    }
}

# make_aggregate(a): every test beneath an aggregate, each in the folder of
# its directory below it.
function make_aggregate(a,    from, s, t, below, folder, path) {
    from = norm(OUT "/testing/" agg_rel[a])
    s = new_solution(from "/" agg_name[a] "." form)
    if (!s)
        return
    made_aggs++
    for (t = 1; t <= ntest; t++) {
        below = under(test_rel[t], agg_rel[a])
        if (below == "-" || !within(test_rel[t]))
            continue
        folder = (below ~ /\//) ? dirname(below) : ""
        path = OUT "/testing/" test_rel[t] "/" test_name[t] ".vcxproj"
        if (sln_add_project(s, msvs_relative(from, path, SEP), folder,
                            msvs_guid("djinterp:" path)) < 0) {
            print "msvs_generate: " agg_name[a] ": " sln_error() \
                  > "/dev/stderr"
            status = 2
        }
    }
    add_libraries(s, from)
    save_solution(s, agg_name[a])
}
