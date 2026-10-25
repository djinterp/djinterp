#!/bin/sh
# ninja.sh -- the -x ninja functions, ninja_tool.awk, ninja_set.awk and
# ninja_survey.awk against outputs fixed in advance, and the Ninja reader
# and loader against ninja.
#
# Each case runs dawk over a file written here and compares what it prints,
# its status, and, when it writes, the bytes it wrote. Where ninja is
# installed, ninja_tool.awk then answers as ninja does -- what each tool
# writes, its status, and the first line it writes to standard error --
# over every manifest in cases/, a CR LF one made here, and, where cmake is
# installed too, a build CMake generates for Ninja; NINJA names another
# ninja to answer as besides. The documents' own check follows: every Ninja
# file here changed at random.
#
#   sh tests/djinterp/tools/dawk/ext/ninja/ninja.sh [build/dawk] [build/test_dninja]
DAWK=${1:-build/dawk}
UNIT=${2:-build/test_dninja}
HERE=tests/djinterp/tools/dawk/ext/ninja
RES=res/djinterp/tools/dawk/ext/ninja
TOOL=$RES/ninja_tool.awk
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
pass=0
fail=0
skipped=''

# the scripts and the cases are read from wherever the run started
case "$DAWK" in /*) ;; *) DAWK=$(pwd)/$DAWK ;; esac
case "$UNIT" in /*) ;; *) UNIT=$(pwd)/$UNIT ;; esac
ROOT=$(pwd)

# expect NAME ARGS... <<'EOF': dawk's output and status, its last line the
# status, must be the text given on standard input exactly
expect() {
    name=$1
    shift
    want=$(cat)
    got=$("$DAWK" "$@" </dev/null 2>&1; echo "status $?")
    if [ "$got" = "$want" ]; then
        pass=$((pass + 1))
    else
        fail=$((fail + 1))
        echo "FAIL $name"
        printf '%s\n' "$want" | sed 's/^/    want: /'
        printf '%s\n' "$got"  | sed 's/^/    got:  /'
    fi
}

# holds NAME FILE WANT: the file must hold WANT, read through printf %b
holds() {
    if [ "$(od -An -c "$2")" = "$(printf '%b' "$3" | od -An -c)" ]; then
        pass=$((pass + 1))
    else
        fail=$((fail + 1))
        echo "FAIL $1"
        printf '%b' "$3" | od -c | sed 's/^/    want: /'
        od -c "$2" | sed 's/^/    got:  /'
    fi
}

# -- a file for the functions to read ------------------------------------------
mkdir -p "$work/f"
cat > "$work/f/build.ninja" <<'EOF'
# the build
cflags = -O2
rule cc
  command = cc $cflags -c $in -o $out
  # the rule's own
  description = CC $out
pool link
  depth = 2
build a.o b$ c.o | a.d: cc a.c | a.h || gen |@ check
  cflags = -g $cflags
build app: phony a.o
default app
include rules.ninja
EOF
printf 'rule gen\n  command = touch $out\n' > "$work/f/rules.ninja"
cd "$work/f" || exit 2

# -- reading -----------------------------------------------------------------
expect 'every statement, its fields' -x ninja 'BEGIN {
    F = ninja_open("build.ninja"); n = ninja_statements(F)
    print F, n, ninja_comments(F)
    for (i = 1; i <= n; i++)
        printf "%d %s [%s] [%s] [%s] %d %d %d %d\n", i,
               ninja_statement(F, i, "kind"), ninja_statement(F, i, "name"),
               ninja_statement(F, i, "value"),
               ninja_statement(F, i, "value_text"),
               ninja_statement(F, i, "line"),
               ninja_statement(F, i, "end_line"),
               ninja_paths(F, i), ninja_bindings(F, i)
}' <<'EOF'
1 7 2
1 variable [cflags] [-O2] [-O2] 2 2 0 0
2 rule [cc] [] [] 3 6 0 2
3 pool [link] [] [] 7 8 0 1
4 build [cc] [] [] 9 10 7 1
5 build [phony] [] [] 11 11 2 0
6 default [] [] [] 12 12 1 0
7 include [] [rules.ninja] [rules.ninja] 13 13 1 0
status 0
EOF

expect 'statements found; paths, bindings and comments read' -x ninja 'BEGIN {
    F = ninja_open("build.ninja"); b = ninja_find(F, "build", "b c.o")
    print b, ninja_find(F, "build", "app"), ninja_find(F, "rule", "cc"),
          ninja_find(F, "include", "rules.ninja"), ninja_find(F, "build"),
          ninja_find(F, "build", "", b), ninja_find(F, "pool", "nope")
    for (k = 1; k <= ninja_paths(F, b); k++)
        printf "%d [%s] [%s] %s %d\n", k, ninja_path_at(F, b, k),
               ninja_path_at(F, b, k, "value"),
               ninja_path_at(F, b, k, "section"),
               ninja_path_at(F, b, k, "line")
    print ninja_paths(F, b, "output"), ninja_paths(F, b, "implicit"),
          ninja_paths(F, b, "validation")
    printf "[%s] [%s] [%s] %d\n", ninja_binding(F, b, 1),
           ninja_binding(F, b, 1, "text"), ninja_binding(F, b, 1, "value"),
           ninja_binding(F, b, 1, "line")
    print ninja_find_binding(F, 2, "description"),
          ninja_find_binding(F, 2, "nope")
    for (i = 1; i <= ninja_comments(F); i++)
        printf "%d [%s] [%s] %d %d\n", i, ninja_comment(F, i, "text"),
               ninja_comment(F, i, "body"), ninja_comment(F, i, "line"),
               ninja_comment(F, i, "statement")
}' <<'EOF'
4 5 2 7 4 5 0
1 [a.o] [a.o] output 9
2 [b$ c.o] [b c.o] output 9
3 [a.d] [a.d] implicit_output 9
4 [a.c] [a.c] input 9
5 [a.h] [a.h] implicit 9
6 [gen] [gen] order_only 9
7 [check] [check] validation 9
2 1 1
[cflags] [-g $cflags] [-g ${cflags}] 10
2 0
1 [# the build] [ the build] 1 0
2 [# the rule's own] [ the rule's own] 5 2
status 0
EOF

# -- changes, and saving -----------------------------------------------------
cp build.ninja edit.ninja
expect 'every kind of change, kept' -x ninja 'BEGIN {
    F = ninja_open("edit.ninja")
    print ninja_set_value(F, 1, "-O1 -g"), ninja_set_name(F, 3, "linker")
    print ninja_set_binding(F, 2, "depfile", "$out.d"),
          ninja_set_binding(F, 4, "cflags", "-O0")
    print ninja_remove_binding(F, 4, "cflags"), ninja_bindings(F, 4)
    print ninja_insert_path(F, 4, "input", 2, "b.c"),
          ninja_set_path(F, 4, 1, "x.o"), ninja_remove_path(F, 4, 6)
    print ninja_insert(F, 6, "build lib.a: phony x.o\n"),
          ninja_replace(F, 7, "default app lib.a\n")
    print ninja_insert(F, ninja_statements(F) + 1, "subninja s.ninja"),
          ninja_changes(F)
    print ninja_remove(F, 8, 9), ninja_statements(F)
    print ninja_save(F), ninja_save(F), ninja_error() == ""
    print ninja_save(F, "out/copy.ninja"), ninja_path(F)
    print ninja_close(F), ninja_close(F)
}' <<'EOF'
1 1
1 1
1 0
1 1 1
1 1
1 11
1 7
1 0 1
1 edit.ninja
1 0
status 0
EOF
holds 'the file the changes made' edit.ninja \
'# the build\ncflags = -O1 -g\nrule cc\n  command = cc $cflags -c $in -o $out
  # the rule'"'"'s own\n  description = CC $out\n  depfile = $out.d
pool linker\n  depth = 2\nbuild x.o b$ c.o | a.d: cc a.c b.c || gen |@ check
build app: phony a.o\nbuild lib.a: phony x.o\ndefault app lib.a\n'
holds '...and its copy, its directory made' out/copy.ninja \
'# the build\ncflags = -O1 -g\nrule cc\n  command = cc $cflags -c $in -o $out
  # the rule'"'"'s own\n  description = CC $out\n  depfile = $out.d
pool linker\n  depth = 2\nbuild x.o b$ c.o | a.d: cc a.c b.c || gen |@ check
build app: phony a.o\nbuild lib.a: phony x.o\ndefault app lib.a\n'

expect 'changes refused, each saying why' -x ninja 'BEGIN {
    F = ninja_open("build.ninja")
    print ninja_set_binding(F, 2, "nope", "1"); print ninja_error()
    print ninja_set_value(F, 2, "x"); print ninja_error()
    print ninja_insert_path(F, 4, "nope", 1, "x"); print ninja_error()
    print ninja_insert(F, 99, "x = 1\n"); print ninja_error()
    print ninja_replace(F, 1, "rule r\n  command = c\n"); print ninja_error()
    print ninja_remove_path(F, 6, 1); print ninja_error()
    print ninja_changes(F), ninja_save(F)
}' <<'EOF'
-1
the change would leave the file unreadable: line 7: unexpected variable 'nope'
-1
the statement is no variable
-1
no section nope: output, implicit_output, input, implicit, order_only, validation or target
-1
the file has no statement 99
-1
not one variable: rule r
  command = c

-1
the change would leave the file unreadable: line 12: expected target name
0 0
status 0
EOF

expect 'a file made of a text, and one ninja would refuse' -x ninja 'BEGIN {
    F = ninja_new("new.ninja"); print F, ninja_statements(F)
    print ninja_insert(F, 1, "x = 1"), ninja_text(F)
    print ninja_new("bad.ninja", "x\n"); print ninja_error()
    print ninja_save(F, "build.ninja/x.ninja"); print ninja_error()
    print ninja_open("nope.ninja"); print ninja_error()
}' <<'EOF'
1 0
1 x = 1

-1
bad.ninja: line 1: expected '=', got newline
-1
cannot make the directories of build.ninja/x.ninja
-1
cannot read: nope.ninja
status 0
EOF

expect 'what names nothing gives -1, or "" for a field' -x ninja 'BEGIN {
    G = ninja_new("t.ninja", "rule r\n  command = c\n")
    print ninja_paths(G, 1, "nope"); print ninja_error()
    print "[" ninja_statement(G, 1, "nope") "]"; print ninja_error()
    print ninja_statement(G, 5, "kind"); print ninja_error()
    print ninja_path_at(G, 1, 1); print ninja_error()
    print ninja_binding(G, 1, 2); print ninja_error()
    print ninja_comment(G, 1, "text"); print ninja_error()
    print ninja_statements(9); print ninja_error()
    print ninja_statements(G), "[" ninja_error() "]"
}' <<'EOF'
-1
no section nope
[]
no field nope
-1
the file has no statement 5
-1
the statement has no such path
-1
the statement has no such binding
-1
the file has no such comment
-1
no file has the handle 9
1 []
status 0
EOF

# -- writing -------------------------------------------------------------------
expect 'paths and values escaped; paths canonical and shell-quoted' \
       -x ninja 'BEGIN {
    printf "[%s] [%s] [%s]\n", ninja_escape("a b:c$"),
           ninja_escape(" a:b", "value"), ninja_escape("x\ny", "newline")
    print ninja_escape("a|b"); print ninja_error()
    printf "[%s] [%s] [%s] [%s]\n", ninja_canonical("./a//b/../c"),
           ninja_canonical("a\\b\\..\\c", 1), ninja_shell_escape("it'"'"'s"),
           ninja_shell_escape("a b", 1)
}' <<'EOF'
[a$ b$:c$$] [$ a:b] [x$^y]
-1
a path cannot hold a `|`, a newline, a CR or a NUL
[a/c] [a/c] ['it'\''s'] ["a b"]
status 0
EOF

# -- builds --------------------------------------------------------------------
expect 'a build: its edges, each binding evaluated' -x ninja 'BEGIN {
    g = ninja_load("build.ninja")
    print g, ninja_edges(g), ninja_nodes(g)
    split("outputs implicit_outputs inputs implicit order_only validations",
          s, " ")
    for (e = 1; e <= ninja_edges(g); e++) {
        printf "%d %s %d [%s] %s:%d\n", e, ninja_edge(g, e, "rule"),
               ninja_edge(g, e, "phony"), ninja_edge(g, e, "pool"),
               ninja_edge(g, e, "file"), ninja_edge(g, e, "line")
        for (k = 1; k <= 6; k++) {
            t = ninja_edge(g, e, s[k])
            gsub(/\n/, ",", t)
            printf " %s=[%s]", s[k], t
        }
        printf "\n [%s] [%s] [%s]\n", ninja_edge_var(g, e, "command"),
               ninja_edge_var(g, e, "command", 1),
               ninja_edge_var(g, e, "description")
    }
}' <<'EOF'
1 2 8
1 cc 0 [] build.ninja:9
 outputs=[a.o,b c.o] implicit_outputs=[a.d] inputs=[a.c] implicit=[a.h] order_only=[gen] validations=[check]
 [cc -g -O2 -c a.c -o a.o 'b c.o'] [cc -g -O2 -c a.c -o a.o b c.o] [CC a.o 'b c.o']
2 phony 1 [] build.ninja:11
 outputs=[app] implicit_outputs=[] inputs=[a.o] implicit=[] order_only=[] validations=[]
 [] [] []
status 0
EOF

expect 'its nodes, targets, rules, pools, variables and files' \
       -x ninja 'function joined(t) { gsub(/\n/, ",", t); return t }
BEGIN {
    g = ninja_load("build.ninja")
    for (n = 1; n <= ninja_nodes(g); n++)
        printf "%d %s %d [%s] [%s]\n", n, ninja_node(g, n, "path"),
               ninja_node(g, n, "in_edge"),
               joined(ninja_node(g, n, "out_edges")),
               joined(ninja_node(g, n, "validation_edges"))
    print ninja_find_node(g, "./a.o"), ninja_find_node(g, "nope")
    print "[" joined(ninja_defaults(g)) "] [" joined(ninja_roots(g)) "]"
    print "[" joined(ninja_rules(g)) "] [" joined(ninja_pools(g)) "]"
    print "[" joined(ninja_files(g)) "] [" ninja_warnings(g) "]"
    print "[" joined(ninja_rule(g, "cc")) "]", ninja_rule(g, "cc", "description")
    print ninja_rule(g, "cc", "pool"); print ninja_error()
    print ninja_rule(g, "nope"); print ninja_error()
    print ninja_pool(g, "link"), ninja_pool(g, "console"), ninja_pool(g, "x")
    print "[" ninja_variable(g, "cflags") "] [" ninja_variable(g, "x") "]"
    print ninja_unload(g), ninja_unload(g); print ninja_edges(g)
    print ninja_error()
}' <<'EOF'
1 a.o 1 [2] []
2 b c.o 1 [] []
3 a.d 1 [] []
4 a.c 0 [1] []
5 a.h 0 [1] []
6 gen 0 [1] []
7 check 0 [] [1]
8 app 2 [] []
1 0
[app] [b c.o,a.d,app]
[cc,gen,phony] [console,link]
[build.ninja,rules.ninja] []
[command,description] CC ${out}
-1
the rule binds no pool
-1
the build has no rule nope
2 1 -1
[-O2] []
1 0
-1
no build has the handle 1
status 0
EOF

printf 'rule r\n  command = $a\nbuild o: r\n  a = x$^y\n' > nl.ninja
printf 'ninja_required_version = 1.14\n' > v14.ninja
cat nl.ninja >> v14.ninja
expect 'what ninja refuses to load, in its words, as a version of it' \
       -x ninja 'BEGIN {
    print ninja_load("nope.ninja"); print ninja_error()
    print ninja_load("nl.ninja"); print ninja_error()
    print ninja_load("v14.ninja", "", "1.11.1"); print ninja_error()
    g = ninja_load("v14.ninja")
    printf "%d [%s]\n", g, ninja_edge_var(g, 1, "command")
}' <<'EOF'
-1
loading 'nope.ninja': No such file or directory
-1
nl.ninja:4: using $^ escape requires specifying 'ninja_required_version' with version greater or equal 1.14
-1
ninja version (1.11.1) incompatible with build file ninja_required_version version (1.14).
1 [x
y]
status 0
EOF

# -- ninja_tool.awk ------------------------------------------------------------
expect 'ninja_tool: targets all' -x ninja -f "$ROOT/$TOOL" -- -t targets all <<'EOF'
a.o: cc
b c.o: cc
a.d: cc
app: phony
status 0
EOF
expect 'ninja_tool: targets depth 0' -x ninja -f "$ROOT/$TOOL" -- \
       -t targets depth 0 <<'EOF'
b c.o: cc
  a.c
  a.h
  gen
a.d: cc
  a.c
  a.h
  gen
app: phony
  a.o: cc
    a.c
    a.h
    gen
status 0
EOF
expect 'ninja_tool: commands, query, rules -d, targets rule' \
       -x ninja -f "$ROOT/$TOOL" -- -t commands <<'EOF'
cc -g -O2 -c a.c -o a.o 'b c.o'
status 0
EOF
expect 'ninja_tool: query' -x ninja -f "$ROOT/$TOOL" -- -t query a.o <<'EOF'
a.o:
  input: cc
    a.c
    | a.h
    || gen
  validations:
    check
  outputs:
    app
status 0
EOF
expect 'ninja_tool: rules -d' -x ninja -f "$ROOT/$TOOL" -- -t rules -d <<'EOF'
cc: CC ${out}
gen
phony
status 0
EOF
expect 'ninja_tool: targets rule, sorted' -x ninja -f "$ROOT/$TOOL" -- \
       -t targets rule cc <<'EOF'
a.d
a.o
b c.o
status 0
EOF
expect 'ninja_tool: a target not there, the nearest named' \
       -x ninja -f "$ROOT/$TOOL" -- -t commands a.p <<'EOF'
ninja: error: unknown target 'a.p', did you mean 'a.o'?
status 1
EOF
expect 'ninja_tool: a manifest ninja refuses' -x ninja -f "$ROOT/$TOOL" -- \
       -f nl.ninja -t targets <<'EOF'
ninja: error: nl.ninja:4: using $^ escape requires specifying 'ninja_required_version' with version greater or equal 1.14
status 1
EOF
expect 'ninja_tool: a manifest read as an older ninja' \
       -x ninja -f "$ROOT/$TOOL" -- -V 1.11.1 -f v14.ninja -t targets <<'EOF'
ninja: fatal: ninja version (1.11.1) incompatible with build file ninja_required_version version (1.14).
status 1
EOF
expect 'ninja_tool: a tool it does not answer' -x ninja -f "$ROOT/$TOOL" -- \
       -t browse <<'EOF'
ninja_tool: no tool browse: targets, commands, query or rules
status 2
EOF

# -- ninja_set.awk -------------------------------------------------------------
SET=$ROOT/$RES/ninja_set.awk
cp build.ninja set.ninja
expect 'ninja_set: variables, shown and not saved' -x ninja -f "$SET" \
       -v file=set.ninja cflags='-O1 -g' newvar='a b' ' lead= x' <<'EOF'
set.ninja:2: cflags: -O2 -> -O1 -g
set.ninja:3: newvar: (none) -> a b
 lead: refused: not a name ninja reads
1 of 3 changes refused; nothing saved
status 1
EOF
expect 'ninja_set: a build'"'"'s bindings, saved' -x ninja -f "$SET" \
       -v file=set.ninja -v build=a.o -v apply=1 cflags=-O3 extra='$x' <<'EOF'
set.ninja:10: cflags: -g $cflags -> -O3
set.ninja:11: extra: (none) -> $$x
set.ninja: saved
status 0
EOF
expect 'ninja_set: a rule'"'"'s, raw; one no rule may have, refused' \
       -x ninja -f "$SET" -v file=set.ninja -v rule=cc -v raw=1 -v apply=1 \
       depfile='$out.d' bogus=1 <<'EOF'
set.ninja:7: depfile: (none) -> $out.d
bogus: refused: the change would leave the file unreadable: line 8: unexpected variable 'bogus'
1 of 2 changes refused; nothing saved
status 1
EOF
expect 'ninja_set: a binding and a variable taken out' -x ninja -f "$SET" \
       -v file=set.ninja -v build=a.o -v apply=1 extra <<'EOF'
set.ninja:11: extra: $$x -> (removed)
set.ninja: saved
status 0
EOF
expect 'ninja_set: a build not in the file' -x ninja -f "$SET" \
       -v file=set.ninja -v build=nope x=1 <<'EOF'
ninja_set: set.ninja has no build nope
status 1
EOF
holds 'ninja_set: the file as saved' set.ninja \
'# the build\ncflags = -O2\nrule cc\n  command = cc $cflags -c $in -o $out
  # the rule'"'"'s own\n  description = CC $out\npool link\n  depth = 2
build a.o b$ c.o | a.d: cc a.c | a.h || gen |@ check\n  cflags = -O3
build app: phony a.o\ndefault app\ninclude rules.ninja\n'

# -- ninja_survey.awk ----------------------------------------------------------
mkdir -p "$work/tree/sub" "$work/tree/bad"
cp build.ninja rules.ninja "$work/tree/sub/"
printf 'x\n' > "$work/tree/bad/build.ninja"
printf 'build t: phony\n' > "$work/tree/other.ninja"
cd "$work" || exit 2
expect 'ninja_survey: a tree, file by file and whole' -x ninja -T tree \
       -f "$ROOT/$RES/ninja_survey.awk" <<'EOF'
file                                                      lines  vars rules builds pools other
tree/other.ninja                                              1     0     0      1     0     0
tree/sub/build.ninja                                         13     1     1      2     1     2
tree/sub/rules.ninja                                          2     0     1      0     0     0

4 Ninja files: 3 read, 1 refused
16 lines, 2 comments; 1 variable, 2 rule, 3 build, 1 pool, 1 default, 1 include, 0 subninja
build tree/sub/build.ninja: 2 files, 2 edges, 8 nodes, 3 roots, 1 defaults
rules most used:
  phony                                                 2
  cc                                                    1
pools: link
refused: tree/bad/build.ninja: line 1: expected '=', got newline
status 1
EOF
cd "$ROOT" || exit 2

# -- ninja itself ----------------------------------------------------------------
# answers NINJA VERSION DIR FILE ARGS...: ninja_tool.awk's must be ninja's
answers() {
    nj=$1
    version=$2
    dir=$3
    file=$4
    shift 4
    want=$(cd "$dir" && timeout 20 "$nj" -f "$file" -t "$@" 2>/dev/null;
           echo "status $?")
    want_err=$(cd "$dir" && timeout 20 "$nj" -f "$file" -t "$@" 2>&1 \
                   >/dev/null | head -n 1)
    got=$(cd "$dir" && "$DAWK" -x ninja -f "$ROOT/$TOOL" -- -V "$version" \
              -f "$file" -t "$@" 2>/dev/null; echo "status $?")
    got_err=$(cd "$dir" && "$DAWK" -x ninja -f "$ROOT/$TOOL" -- \
                  -V "$version" -f "$file" -t "$@" 2>&1 >/dev/null |
                  head -n 1)
    if [ "$got" = "$want" ] && [ "$got_err" = "$want_err" ]; then
        pass=$((pass + 1))
    else
        fail=$((fail + 1))
        echo "FAIL ninja $version, $file: -t $*"
        printf '%s\n' "$want" "$want_err" | head -n 12 | sed 's/^/    want: /'
        printf '%s\n' "$got" "$got_err" | head -n 12 | sed 's/^/    got:  /'
    fi
}

# every tool over a manifest, and query and commands for each path it names
over() {
    nj=$1
    version=$2
    dir=$3
    file=$4
    for tool in 'targets all' 'targets' 'targets depth 3' 'targets rule' \
                'targets rule r' 'commands' 'rules' 'rules -d'; do
        # shellcheck disable=SC2086
        answers "$nj" "$version" "$dir" "$file" $tool
    done
    (cd "$dir" && timeout 20 "$nj" -f "$file" -t targets all 2>/dev/null |
         sed 's/: [^:]*$//'
     cd "$dir" && timeout 20 "$nj" -f "$file" -t targets rule 2>/dev/null) |
        sort -u | head -n 60 > "$work/paths"
    while IFS= read -r path; do
        answers "$nj" "$version" "$dir" "$file" query "$path"
        answers "$nj" "$version" "$dir" "$file" commands -s "$path"
        answers "$nj" "$version" "$dir" "$file" commands "$path^"
    done < "$work/paths"
}

oracles=''
command -v ninja >/dev/null 2>&1 && oracles=ninja
[ -n "${NINJA:-}" ] && [ -x "$NINJA" ] && oracles="$oracles $NINJA"
mkdir -p "$work/cases"
cp "$HERE"/cases/*.ninja "$work/cases/"
printf 'rule r\r\n  command = c $in\r\nbuild a: r b\r\n' > "$work/cases/crlf.ninja"
if [ -z "$oracles" ]; then
    skipped="$skipped ninja"
else
    for nj in $oracles; do
        version=$("$nj" --version)
        for file in "$work"/cases/*.ninja; do
            over "$nj" "$version" "$work/cases" "${file##*/}"
        done
    done
fi

# a build CMake writes for Ninja, which ninja must run beside it
if [ -n "$oracles" ] && command -v cmake >/dev/null 2>&1; then
    mkdir -p "$work/cm/src/lib"
    cat > "$work/cm/src/CMakeLists.txt" <<'EOF'
cmake_minimum_required(VERSION 3.16)
project(demo C)
add_library(leaf STATIC lib/leaf.c)
target_compile_definitions(leaf PRIVATE LEAF=1)
add_executable(app main.c)
target_link_libraries(app PRIVATE leaf)
add_custom_command(OUTPUT gen.h COMMAND ${CMAKE_COMMAND} -E touch gen.h)
add_custom_target(gen ALL DEPENDS gen.h)
EOF
    printf 'int leaf(void) { return LEAF; }\n' > "$work/cm/src/lib/leaf.c"
    printf 'int leaf(void);\nint main(void) { return leaf(); }\n' \
        > "$work/cm/src/main.c"
    if cmake -G Ninja -S "$work/cm/src" -B "$work/cm/build" \
             >/dev/null 2>&1; then
        for nj in $oracles; do
            over "$nj" "$("$nj" --version)" "$work/cm/build" build.ninja
        done
        cp "$work/cm/build/build.ninja" "$work/cases/cmake.ninja"
        cp "$work/cm/build/CMakeFiles/rules.ninja" \
           "$work/cases/cmake_rules.ninja"
    else
        skipped="$skipped cmake"
    fi
else
    skipped="$skipped cmake"
fi

# -- the documents' own check: changes at random -------------------------------
if out=$("$UNIT" --fuzz "$work/cases" 1 200 2>&1); then
    pass=$((pass + 1))
else
    fail=$((fail + 1))
    echo "FAIL changes at random to every Ninja file here"
    printf '%s\n' "$out" | tail -n 20 | sed 's/^/    /'
fi

[ -n "$skipped" ] && echo "not installed, so not compared:$skipped"
printf '\nninja: %d passed, %d failed\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
