#!/bin/sh
# cmake.sh -- the -x cmake functions, cmake_survey.awk and cmake_sources.awk
# against outputs fixed in advance, and the CMake reader against CMake.
#
# Each case runs dawk over a file written here and compares what it prints,
# its status, and, when it writes, the bytes it wrote; where cmake is
# installed, values written as arguments, and a file changed by every kind
# of change, are run under CMake to read back as asked. The reader's own
# checks follow: every CMake file under build/cmake read whole and changed
# at random, when the tree has one, and the arguments of cmake_values.cmake
# against CMake's own reading, with LF and with CR LF, when cmake is
# installed.
#
#   sh tests/djinterp/tools/dawk/ext/cmake/cmake.sh [build/dawk] [build/test_dcmake]
#
# test_dcmake_doc, beside test_dcmake, makes the random changes.
DAWK=${1:-build/dawk}
READER=${2:-build/test_dcmake}
DOC=$(dirname "$READER")/test_dcmake_doc
HERE=tests/djinterp/tools/dawk/ext/cmake
RES=res/djinterp/tools/dawk/ext/cmake
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
pass=0
fail=0
nl='
'

# same NAME WANT ARGS...: dawk's output and status must be WANT exactly,
# its last line the status
same() {
    name=$1
    want=$2
    shift 2
    got=$("$DAWK" "$@" 2>&1; echo "status $?")
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

# -- reading -----------------------------------------------------------------
cat > "$work/CMakeLists.txt" <<'EOF'
cmake_minimum_required(VERSION 3.16)
function(add_leaf _name)
    IF(WIN32)
        set(_flags "/W4 /utf-8") # trailing
    else()
        set(_flags [[-Wall]] -Da="b c")
    endif()
endfunction()
EOF
same 'every field of every command' "1 8
1 cmake_minimum_required 1 1 1 0   0 0 2 []
2 function 2 1 2 0 function open 8 0 2 []
3 IF 3 5 3 1 if open 7 0 1 [    ]
4 set 4 9 4 2   0 0 2 [        ]
5 else 5 5 5 1 if branch 3 0 0 [    ]
6 set 6 9 6 2   0 0 3 [        ]
7 endif 7 5 7 1 if close 3 0 0 [    ]
8 endfunction 8 1 8 0 function close 2 0 0 []
status 0" \
    -x cmake -v f="$work/CMakeLists.txt" 'BEGIN {
        F = cmake_open(f); n = cmake_commands(F); print F, n
        for (i = 1; i <= n; i++)
            print i, cmake_name(F, i), cmake_command(F, i, "line"),
                  cmake_command(F, i, "column"),
                  cmake_command(F, i, "end_line"),
                  cmake_command(F, i, "depth"), cmake_command(F, i, "block"),
                  cmake_command(F, i, "role"),
                  cmake_command(F, i, "partner"),
                  cmake_command(F, i, "unpaired"), cmake_args(F, i),
                  "[" cmake_command(F, i, "indent") "]"
    }'
same 'arguments as written, as read, as passed, and their kinds' \
"[\"/W4 /utf-8\"] [/W4 /utf-8] [/W4 /utf-8] quoted 4 20
[[[-Wall]]] [-Wall] [-Wall] bracket 6 20
[-Da=\"b c\"] [-Da=\"b c\"] [-Da=\"b c\"] unquoted 6 30
legacy 1 0, paren [] [(]
status 0" \
    -x cmake -v f="$work/CMakeLists.txt" 'BEGIN {
        F = cmake_open(f)
        print "[" cmake_arg(F, 4, 2) "] [" cmake_value(F, 4, 2) "] [" \
              cmake_arg(F, 4, 2, "items") "]", cmake_arg(F, 4, 2, "kind"),
              cmake_arg(F, 4, 2, "line"), cmake_arg(F, 4, 2, "column")
        for (k = 2; k <= 3; k++)
            print "[" cmake_arg(F, 6, k) "] [" cmake_value(F, 6, k) "] [" \
                  cmake_arg(F, 6, k, "items") "]",
                  cmake_arg(F, 6, k, "kind"), cmake_arg(F, 6, k, "line"),
                  cmake_arg(F, 6, k, "column")
        G = cmake_new("x.cmake", "if((A))\nendif()\n")
        print "legacy", cmake_arg(F, 6, 3, "legacy"),
              cmake_arg(F, 6, 2, "legacy") ", paren [" \
              cmake_arg(F, 6, 2, "paren") "] [" cmake_arg(G, 1, 1, "paren") "]"
    }'
printf 'show(a;b ;c "x;y" a\\;b [x;y]z)\n' > "$work/lists.cmake"
same 'the arguments CMake passes for each, a line each' "a|b
c
x;y
a;b
[x;y]z
status 0" \
    -x cmake -v f="$work/lists.cmake" 'BEGIN {
        F = cmake_open(f)
        for (k = 1; k <= cmake_args(F, 1); k++) {
            s = cmake_arg(F, 1, k, "items"); gsub(/\n/, "|", s); print s
        }
    }'
same 'finding commands and arguments' "4 6 0 4 0 2 0
status 0" \
    -x cmake -v f="$work/CMakeLists.txt" 'BEGIN {
        F = cmake_open(f)
        print cmake_find(F, "SET"), cmake_find(F, "set", 4),
              cmake_find(F, "set", 6), cmake_find(F, "set", "x"),
              cmake_find(F, "nothing"), cmake_find_arg(F, 6, "-Wall"),
              cmake_find_arg(F, 6, "-Wall", 2)
    }'
same 'a command as written, and the file as a whole' \
"set(_flags [[-Wall]] -Da=\"b c\")|0|8|0|1|$work/CMakeLists.txt
status 0" \
    -x cmake -v f="$work/CMakeLists.txt" 'BEGIN {
        F = cmake_open(f)
        print cmake_command(F, 6, "text") "|" cmake_generated(F) "|" \
              cmake_commands(F) "|" cmake_changes(F) "|" cmake_comments(F) \
              "|" cmake_path(F)
    }'
printf '#[==[ a\nbanner ]==]\nset(a # among\n    b) # after\n# own\n' \
    > "$work/comments.cmake"
same 'every field of every comment' "4
1 #[==[ a|banner ]==]| a|banner | 1 1 2 1 0 0
2 # among| among| 3 7 3 0 1 1
3 # after| after| 4 8 4 0 1 0
4 # own| own| 5 1 5 0 0 0
status 0" \
    -x cmake -v f="$work/comments.cmake" 'BEGIN {
        F = cmake_open(f); n = cmake_comments(F); print n
        for (i = 1; i <= n; i++) {
            t = cmake_comment(F, i, "text"); b = cmake_comment(F, i, "body")
            gsub(/\n/, "|", t); gsub(/\n/, "|", b)
            print i, t "|" b "|", cmake_comment(F, i, "line"),
                  cmake_comment(F, i, "column"),
                  cmake_comment(F, i, "end_line"),
                  cmake_comment(F, i, "bracket"),
                  cmake_comment(F, i, "trailing"),
                  cmake_comment(F, i, "command")
        }
    }'

# -- opening, and what is refused ----------------------------------------------
printf 'set(a "b\n' > "$work/broken.cmake"
same 'a file that cannot be read' "-1 cannot read: $work/none.cmake
status 0" \
    -x cmake -v f="$work/none.cmake" 'BEGIN { print cmake_open(f), cmake_error() }'
same 'a file CMake would refuse' \
"-1 $work/broken.cmake: line 1, column 7: the quoted argument is never closed
status 0" \
    -x cmake -v f="$work/broken.cmake" 'BEGIN { print cmake_open(f), cmake_error() }'
same 'what is not there gives -1, and says so' "-1 no file has the handle 7
-1 the file has no command 9
-1 command 1 has no argument 3
-1 the file has no comment 2
 no field colour
0 1 0
status 0" \
    -x cmake -v f="$work/CMakeLists.txt" 'BEGIN {
        F = cmake_open(f)
        print cmake_commands(7), cmake_error()
        print cmake_name(F, 9), cmake_error()
        print cmake_arg(F, 1, 3), cmake_error()
        print cmake_comment(F, 2, "text"), cmake_error()
        print cmake_command(F, 1, "colour"), cmake_error()
        print cmake_close(9), cmake_close(F), cmake_close(F)
    }'
printf '# Install script for directory: D:/x\nset(a)\n' > "$work/cmake_install.cmake"
same 'a generated file says so' "1 1
status 0" \
    -x cmake -v f="$work/cmake_install.cmake" 'BEGIN { F = cmake_open(f); print F, cmake_generated(F) }'
same 'a file of a text, and one CMake would refuse' "1 2 set(a) if(b) endif()
-1 n.cmake: line 1, column 1: the command's \`(\` is never closed
status 0" \
    -x cmake 'BEGIN {
        F = cmake_new("n.cmake", "set(a)\nif(b)\nendif()\n")
        print F, cmake_find(F, "if"), cmake_command(F, 1, "text"),
              cmake_command(F, 2, "text"), cmake_command(F, 3, "text")
        print cmake_new("n.cmake", "set(a"), cmake_error()
    }'

# -- changing arguments ---------------------------------------------------------
cat > "$work/edit.cmake" <<'EOF'
add_library(lib STATIC
    "a.c"
    # a comment
    "c.c" # trailing
    LINK x)
set()
set(a b c)
EOF
same 'changes are made at once, and counted, and saved' "1 1 1 1 1
1 1 1 8 1 3
status 0" \
    -x cmake -v f="$work/edit.cmake" 'BEGIN {
        F = cmake_open(f)
        print cmake_insert_after(F, 1, 3, cmake_quote("b.c")),
              cmake_insert_after(F, 1, 5, cmake_quote("d.c")),
              cmake_insert_arg(F, 2, 1, "x"), cmake_remove_arg(F, 3, 2),
              cmake_set_arg(F, 3, 2, "C")
        print cmake_insert_arg(F, 3, 1, "first"),
              cmake_insert_arg(F, 3, 4, "last"), cmake_remove_arg(F, 1, 8),
              cmake_changes(F), cmake_save(F), cmake_commands(F)
    }'
holds 'every change is in the file, and nothing else changed' "$work/edit.cmake" \
'add_library(lib STATIC\n    "a.c"\n    "b.c"\n    # a comment\n    "c.c" # trailing\n    "d.c"\n    LINK)\nset(x)\nset(first a C last)\n'
same 'each change is read at once, and the numbers move' "z.c 1 2 2
status 0" \
    -x cmake -v f="$work/edit.cmake" 'BEGIN {
        F = cmake_open(f); cmake_set_arg(F, 1, 3, "z.c"); cmake_remove_arg(F, 3, 1)
        print cmake_value(F, 1, 3), cmake_value(F, 3, 1) == "a", cmake_changes(F),
              cmake_find_arg(F, 3, "C")
    }'
same 'what a change refuses' "-1 not one argument: a b
-1 not one argument: # c
-1 not one argument: (
-1 command 3 has no argument 9
-1 command 3 has no argument 0
-1 no file has the handle 2
-1 the file has no command 99
status 0" \
    -x cmake -v f="$work/edit.cmake" 'BEGIN {
        F = cmake_open(f)
        print cmake_set_arg(F, 3, 1, "a b"), cmake_error()
        print cmake_insert_arg(F, 3, 1, "# c"), cmake_error()
        print cmake_insert_arg(F, 3, 1, "("), cmake_error()
        print cmake_remove_arg(F, 3, 9), cmake_error()
        print cmake_insert_after(F, 3, 0, "x"), cmake_error()
        print cmake_set_arg(2, 1, 1, "x"), cmake_error()
        print cmake_set_name(F, 99, "x"), cmake_error()
    }'
printf 'if((A) OR B)\nendif()\n' > "$work/parens.cmake"
same 'a parenthesis is not changed alone' "-1 argument 1 of command 1 is a parenthesis, which cannot be changed alone
-1 argument 3 of command 1 is a parenthesis, which cannot be changed alone
-1 command 2 has no argument 0
-1 command 2 has no argument 1
1 1 1
status 0" \
    -x cmake -v f="$work/parens.cmake" 'BEGIN {
        F = cmake_open(f)
        print cmake_remove_arg(F, 1, 1), cmake_error()
        print cmake_set_arg(F, 1, 3, "x"), cmake_error()
        print cmake_insert_arg(F, 2, 0, "x"), cmake_error()
        print cmake_insert_after(F, 2, 1, "x"), cmake_error()
        print cmake_insert_after(F, 1, 3, "AND"), cmake_insert_arg(F, 1, 2, "NOT"),
              cmake_save(F)
    }'
holds '...but may be stood beside' "$work/parens.cmake" 'if((NOT A) AND OR B)\nendif()\n'
printf 'if((A)AND B)\nset(y "a""b")\nset(z "a"x)\nset(a\n    b c # d\n)\n' \
    > "$work/touching.cmake"
same 'a change beside arguments written against it' "1 1 1 1 1 1
status 0" \
    -x cmake -v f="$work/touching.cmake" 'BEGIN {
        F = cmake_open(f)
        print cmake_insert_arg(F, 1, 3, "\"q\nr\""), cmake_set_arg(F, 2, 2, "c"),
              cmake_remove_arg(F, 3, 2), cmake_insert_after(F, 4, 2, "e"),
              cmake_insert_after(F, 4, 4, "f"), cmake_save(F)
    }'
holds '...sets the new apart, and keeps the old apart' "$work/touching.cmake" \
'if((A "q\nr" )AND B)\nset(y c "b")\nset(z x)\nset(a\n    b e c f # d\n)\n'
printf 'set(a b)\nset(\n    only\n)\nset(x y z)\nset(p\n    q)\n' > "$work/ends.cmake"
same 'an argument taken from either end of a list' "1 1 1 1 1 1
status 0" \
    -x cmake -v f="$work/ends.cmake" 'BEGIN {
        F = cmake_open(f)
        print cmake_remove_arg(F, 1, 1), cmake_remove_arg(F, 2, 1),
              cmake_remove_arg(F, 3, 3), cmake_remove_arg(F, 3, 1),
              cmake_remove_arg(F, 4, 2), cmake_save(F)
    }'
holds '...takes the space beside it, or its whole line, or closes up' \
    "$work/ends.cmake" 'set(b)\nset(\n)\nset(y)\nset(p)\n'
printf 'set(a\r\n    b\r\n    c)\r\n' > "$work/crlf.cmake"
same 'a CR LF file is changed in CR LF' "1 1 1 1
status 0" \
    -x cmake -v f="$work/crlf.cmake" -v o="$work/crlf.out.cmake" 'BEGIN {
        F = cmake_open(f)
        print cmake_insert_after(F, 1, 3, "d"), cmake_remove_arg(F, 1, 2),
              cmake_insert_command(F, 2, "set(e)"), cmake_save(F, o)
    }'
holds '...its endings kept and its own used' "$work/crlf.out.cmake" \
'set(a\r\n    c\r\n    d)\r\nset(e)\r\n'
holds '...and the file it was read from is untouched' "$work/crlf.cmake" \
'set(a\r\n    b\r\n    c)\r\n'

# -- changing commands ------------------------------------------------------------
cat > "$work/commands.cmake" <<'EOF'
project(p)

# the flags
set(f)
function(leaf)
  if(A)
    set(x) # why
  endif()
  while(B)
  endwhile()
endfunction()
EOF
same 'commands renamed, replaced, inserted and removed' "1 1 1 1 1 1 1 1 1 1
status 0" \
    -x cmake -v f="$work/commands.cmake" 'BEGIN {
        F = cmake_open(f)
        print cmake_set_name(F, 1, "PROJECT"),
              cmake_set_command(F, 5, "set(x \"y z\")"),
              cmake_insert_command(F, 2, "cmake_minimum_required(VERSION 3.16)"),
              cmake_insert_command(F, 7, "message(STATUS \"x\")\nset(y)"),
              cmake_insert_command(F, 11, "set(w)"),
              cmake_insert_command(F, 14, cmake_call("leaf")),
              cmake_insert_command(F, 5,
                  "foreach(s a b)\n    message(${s})\nendforeach()"),
              cmake_remove_command(F, 3),
              cmake_remove_command(F, 2, 2), cmake_save(F)
    }'
holds '...each in the layout about it' "$work/commands.cmake" \
'PROJECT(p)\n\n# the flags\nfunction(leaf)\n  foreach(s a b)\n      message(${s})\n  endforeach()\n  if(A)\n    set(x "y z") # why\n    message(STATUS "x")\n    set(y)\n  endif()\n  while(B)\n    set(w)\n  endwhile()\nendfunction()\nleaf()\n'
same 'what a change of commands refuses' "-1 the change would change the part the command takes in its block
-1 the change would change the part the command takes in its block
-1 not commands: line 1, column 5: a command must end its line
-1 the change would leave a block unpaired
-1 the change would change the command at line 8, outside it
-1 not commands: the text has none
-1 not a command's name: 1x
-1 the file has no commands 99 to 99
-1 the change would change the command at line 14, outside it
status 0" \
    -x cmake -v f="$work/commands.cmake" 'BEGIN {
        F = cmake_open(f)
        print cmake_set_name(F, 7, "else"), cmake_error()
        print cmake_set_command(F, 9, "else()"), cmake_error()
        print cmake_set_command(F, 7, "a() b()"), cmake_error()
        print cmake_insert_command(F, 16, "if(C)"), cmake_error()
        print cmake_insert_command(F, 9, "endif()"), cmake_error()
        print cmake_insert_command(F, 1, "# only"), cmake_error()
        print cmake_set_name(F, 1, "1x"), cmake_error()
        print cmake_remove_command(F, 99), cmake_error()
        print cmake_remove_command(F, 11), cmake_error()
    }'
same 'saving: unchanged, and to a new directory' "0 1 1
status 0" \
    -x cmake -v f="$work/commands.cmake" -v o="$work/new/dir/out.cmake" 'BEGIN {
        F = cmake_open(f)
        print cmake_save(F), cmake_save(F, o), cmake_save(cmake_new(o "2", "x()"))
    }'
holds '...the new file holding what was asked' "$work/new/dir/out.cmake2" 'x()'

# -- writing ----------------------------------------------------------------------
same 'the writers' "\"a \\\"b\\\" \\\\c\" [=[x]]y]=] -1 a bracket argument cannot hold a CR LF
plain.c \"a b\" \"\" \"[x\" \"=x\" \"a;b\"
set(a \"b c\" \"\" \"d;e\") x() -1 not a command's name: 1x
a|b q]]q -1 not one argument: a b
3|a|b|c 4|a||b| 0 1
status 0" \
    -x cmake 'BEGIN {
        print cmake_quote("a \"b\" \\c"), cmake_bracket("x]]y"),
              cmake_bracket("a\r\nb"), cmake_error()
        print cmake_argument("plain.c"), cmake_argument("a b"),
              cmake_argument(""), cmake_argument("[x"), cmake_argument("=x"),
              cmake_argument("a;b")
        print cmake_call("set", "a", "b c", "", "d;e"), cmake_call("x"),
              cmake_call("1x", "a"), cmake_error()
        s = cmake_unquote("\"a\\tb\""); gsub(/\t/, "|", s)
        print s, cmake_unquote("[==[q]]q]==]"), cmake_unquote("a b"),
              cmake_error()
        a = cmake_split("a;b;;c;"); gsub(/\n/, "|", a)
        b = cmake_split("a;;b;", 1); gsub(/\n/, "|", b)
        print split(cmake_split("a;b;;c;"), x, "\n") "|" a,
              split(cmake_split("a;;b;", 1), y, "\n") "|" b,
              length(cmake_split("")), split(cmake_split("[a;b];c"), z, "\n") - 1
    }'
if command -v cmake > /dev/null 2>&1; then
    # every value written as an argument reads back as itself under CMake
    values='plain|a b|a;b||x"y|back\\slash|[[x]]|=a|#hash|tab\there|new\nline|a]==]b|(paren)|semi\\;colon|cr\rhere|\\n|${not}'
    "$DAWK" -x cmake -v values="$values" 'BEGIN {
        print "function(show)"
        print "    message(\"argc=${ARGC}|[${ARGV0}]\")"
        print "endfunction()"
        print "set(not \"\\${not}\")"
        n = split(values, v, "|")
        for (i = 1; i <= n; i++) {
            print cmake_call("show", v[i])
            print "show(" cmake_quote(v[i]) ")"
            if (cmake_bracket(v[i]) != -1)
                print "show(" cmake_bracket(v[i]) ")"
        }
    }' > "$work/written.cmake"
    "$DAWK" -x cmake -v values="$values" 'BEGIN {
        n = split(values, v, "|")
        for (i = 1; i <= n; i++) {
            times = (cmake_bracket(v[i]) != -1) ? 3 : 2
            for (t = 1; t <= times; t++)
                print "argc=1|[" v[i] "]"
        }
    }' > "$work/written.want"
    if cmake -P "$work/written.cmake" > "$work/written.got" 2>&1 &&
       cmp -s "$work/written.want" "$work/written.got"; then
        pass=$((pass + 1))
    else
        fail=$((fail + 1))
        echo "FAIL a value written as an argument reads back otherwise under CMake"
        diff "$work/written.want" "$work/written.got" | sed 's/^/    /'
    fi
    # and a file changed by every kind of change runs under CMake as asked
    printf 'set(L a b)\nif(1)\n  message("[${L}]")\nendif()\n' > "$work/run.cmake"
    "$DAWK" -x cmake -v f="$work/run.cmake" 'BEGIN {
        F = cmake_open(f)
        cmake_insert_after(F, 1, 3, cmake_argument("c d"))
        cmake_remove_arg(F, 1, 3)
        cmake_set_name(F, 3, "MESSAGE")
        cmake_insert_command(F, 4, "message(\"in\")")
        cmake_insert_command(F, 6, cmake_call("message", "last;one"))
        cmake_insert_command(F, 1, "cmake_minimum_required(VERSION 3.16)")
        exit (cmake_save(F) == 1) ? 0 : 1
    }'
    if [ "$(cmake -P "$work/run.cmake" 2>&1)" = "[a;c d]
in
last;one" ]; then
        pass=$((pass + 1))
    else
        fail=$((fail + 1))
        echo "FAIL a changed file runs under CMake otherwise than asked"
        cmake -P "$work/run.cmake" 2>&1 | sed 's/^/    /'
        sed 's/^/    | /' "$work/run.cmake"
    fi
fi

# -- the survey -----------------------------------------------------------------
mkdir -p "$work/tree/sub" "$work/tree/CMakeFiles/3.28"
cp "$work/CMakeLists.txt" "$work/tree/CMakeLists.txt"
printf '#****\n# banner\n#****\nadd_executable(app a.c)\nadd_library(lib b.c)\nadd_leaf(x)\n' \
    > "$work/tree/sub/CMakeLists.txt"
printf '# ====\nendif()\n' > "$work/tree/sub/broken.cmake"
printf 'set(CMAKE_C_COMPILER cc)\n' > "$work/tree/CMakeFiles/3.28/CMakeCCompiler.cmake"
cp "$work/cmake_install.cmake" "$work/tree/cmake_install.cmake"
printf 'not cmake\n' > "$work/tree/notes.txt"
same 'the survey of a tree' "
5 CMake files: 3 hand-written, 2 generated, 0 refused
headers: 1 banner, 1 rule, 0 other comment, 1 none
16 lines, 12 commands, 2 blocks, 1 unpaired
commands most used:
  endif                                         2
  set                                           2
  add_executable                                1
  add_leaf                                      1
  add_library                                   1
  cmake_minimum_required                        1
  else                                          1
  endfunction                                   1
  function                                      1
  if                                            1
functions and macros defined here, by calls:
  add_leaf                                      1
targets: 1 add_executable, 1 add_library, 0 add_custom_target
unpaired: $work/tree/sub/broken.cmake:2: endif does not pair
status 1" \
    -x cmake -T "$work/tree" -f "$RES/cmake_survey.awk" -v files=0

# -- source lists -----------------------------------------------------------------
cat > "$work/sources.cmake" <<'EOF'
add_executable(app WIN32
    a.c
    c.c   # the core
    e.c)
target_sources(app PRIVATE x.c PUBLIC y.c)
djinterp_add_test_executable(tests
    SOURCES
        "t1.c"
        "t3.c"
    DEFINES
        D_TESTING=1)
EOF
same 'every list is shown' "app (add_executable, line 1):
    a.c
    c.c
    e.c
app (target_sources, line 5):
    x.c
    y.c
tests (djinterp_add_test_executable, line 6):
    \"t1.c\"
    \"t3.c\"
status 0" \
    -x cmake -f "$RES/cmake_sources.awk" -v list="$work/sources.cmake"
same 'a dry run shows the plan, and writes nothing' "app (add_executable, line 1 of $work/sources.cmake):
  - c.c
  already there: a.c
  + \"a;b.c\"
  + b.c
  already there: e.c
  + f.c
dry run: nothing written; -v apply=1 writes $work/sources.cmake
status 0" \
    -x cmake -f "$RES/cmake_sources.awk" -v list="$work/sources.cmake" \
    -v target=app -v 'add=b.c f.c a;b.c a.c e.c' -v remove=c.c
cp "$work/sources.cmake" "$work/before.cmake"
same '...and the same, applied' "app (add_executable, line 1 of $work/sources.cmake):
  - c.c
  already there: a.c
  + \"a;b.c\"
  + b.c
  already there: e.c
  + f.c
wrote $work/app.cmake
status 0" \
    -x cmake -f "$RES/cmake_sources.awk" -v list="$work/sources.cmake" \
    -v target=app -v 'add=b.c f.c a;b.c a.c e.c' -v remove=c.c -v apply=1 \
    -v out="$work/app.cmake"
holds '...in order, a name that must be quoted quoted' "$work/app.cmake" \
'add_executable(app WIN32\n    a.c\n    "a;b.c"\n    b.c\n    e.c\n    f.c)\ntarget_sources(app PRIVATE x.c PUBLIC y.c)\ndjinterp_add_test_executable(tests\n    SOURCES\n        "t1.c"\n        "t3.c"\n    DEFINES\n        D_TESTING=1)\n'
holds '...and the list left as it was' "$work/sources.cmake" "$(cat "$work/before.cmake")\n"
same 'apply writes the list in its own order and layout' "tests (djinterp_add_test_executable, line 6 of $work/sources.cmake):
  + \"t2.c\"
  + \"t4.c\"
wrote $work/sources.cmake
status 0" \
    -x cmake -f "$RES/cmake_sources.awk" -v list="$work/sources.cmake" \
    -v target=tests -v 'add=t2.c t4.c' -v apply=1
holds '...quoted as the list is, and the rest as it was' "$work/sources.cmake" \
'add_executable(app WIN32\n    a.c\n    c.c   # the core\n    e.c)\ntarget_sources(app PRIVATE x.c PUBLIC y.c)\ndjinterp_add_test_executable(tests\n    SOURCES\n        "t1.c"\n        "t2.c"\n        "t3.c"\n        "t4.c"\n    DEFINES\n        D_TESTING=1)\n'
same 'a target with no list, and a file not in it' "cmake_sources: no source list for nothing
status 1" \
    -x cmake -f "$RES/cmake_sources.awk" -v list="$work/sources.cmake" -v target=nothing -v add=x.c
same '...a file to remove that is not there' "app (add_executable, line 1 of $work/sources.cmake):
  not in the list: zz.c
nothing to change
status 1" \
    -x cmake -f "$RES/cmake_sources.awk" -v list="$work/sources.cmake" -v target=app -v remove=zz.c

# -- the reader, over the tree and against CMake --------------------------------
if [ -d build/cmake ]; then
    if out=$("$READER" --corpus build/cmake 2>&1); then
        pass=$((pass + 1))
    else
        fail=$((fail + 1))
        echo "FAIL the reader over build/cmake"
        printf '%s\n' "$out" | sed 's/^/    /'
    fi
    if [ -x "$DOC" ]; then
        if out=$("$DOC" --fuzz build/cmake 1 40 2>&1); then
            pass=$((pass + 1))
        else
            fail=$((fail + 1))
            echo "FAIL changes at random to build/cmake"
            printf '%s\n' "$out" | sed 's/^/    /'
        fi
    fi
fi
if command -v cmake > /dev/null 2>&1; then
    sed 's/$/\r/' "$HERE/cmake_values.cmake" > "$work/values_crlf.cmake"
    for cases in "$HERE/cmake_values.cmake" "$work/values_crlf.cmake"; do
        want=$(cmake -P "$cases" 2>&1 | tr -d '\r')
        got=$("$READER" --values "$cases" 2>&1 | tr -d '\r')
        if [ "$want" = "$got" ]; then
            pass=$((pass + 1))
        else
            fail=$((fail + 1))
            echo "FAIL the reader and CMake read $cases differently"
            diff "$(printf '%s\n' "$want" > "$work/w"; echo "$work/w")" \
                 "$(printf '%s\n' "$got" > "$work/g"; echo "$work/g")" | sed 's/^/    /'
        fi
    done
else
    echo "note: cmake is not installed; the values are not compared with it"
fi

echo
echo "cmake: $pass passed, $fail failed"
[ "$fail" -eq 0 ]
