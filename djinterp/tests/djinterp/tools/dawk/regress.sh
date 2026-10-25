#!/bin/sh
# regress.sh -- the interpreter against outputs fixed in advance.
#
# diff_awk.sh asks whether dawk agrees with the awk installed; that is evidence
# and not authority, and it says nothing where the two are wrong together.
# Each case here is a defect the one-true-awk T.* suite found, with the output
# POSIX gives, worked out by hand, so each stays fixed whatever awk is on the
# system. A case is a name, the output wanted -- its last line the exit
# status -- and dawk's arguments; standard input is $INPUT, through printf %b.
#
#   sh tests/djinterp/tools/dawk/regress.sh [build/dawk]
DAWK=${1:-build/dawk}
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
pass=0
fail=0
INPUT=''

check() {
    name=$1
    want=$2
    shift 2
    got=$(printf '%b' "$INPUT" | "$DAWK" "$@" 2>&1; echo "status $?")
    if [ "$got" = "$want" ]; then
        pass=$((pass + 1))
    else
        fail=$((fail + 1))
        echo "FAIL $name"
        printf '%s\n' "$want" | sed 's/^/    want: /'
        printf '%s\n' "$got"  | sed 's/^/    got:  /'
    fi
}

nl='
'
tab=$(printf '\t')

# -- the empty statement is a body, not a terminator before one -------------
check 'while (...) ;' "4${nl}status 0" \
      'BEGIN { i = 0; while (i++ < 3) ; print i }'
check 'for (...) ;' "3${nl}status 0" \
      'BEGIN { for (i = 0; i < 3; i++) ; print i }'
check 'if (...) ; else' "else${nl}end${nl}status 0" \
      'BEGIN { if (0) ; else print "else"; print "end" }'
check 'for (...) on one line, ; on the next' "8${nl}status 0" \
      "$(printf 'BEGIN {\n    for (k = 10; k > 8; --k)\n        ;\n    print k\n}')"

# -- sub and gsub ------------------------------------------------------------
check 'gsub on a string constant is a syntax error' \
      "-:1:9: syntax error: gsub's third argument must be a variable, field or array element${nl}status 2" \
      'BEGIN { gsub(/ana/, "anda", "banana") }'
check 'gsub & expanding past the buffer' "1202${nl}status 0" \
      'BEGIN { s = sprintf("%300s", ""); gsub(/ /, "ab", s); gsub(/(ab)+/, "<&&>", s); print length(s) }'
check 'gsub: no empty match where a match ended' "-a-c- X 1${nl}status 0" \
      'BEGIN { s = "abc"; gsub(/b*/, "-", s); t = "aaa"; n = gsub(/a*/, "X", t); print s, t, n }'
check 'sub evaluates its target subscript once' "2 xbx yay${nl}status 0" \
      'BEGIN { i = 1; a[1] = "xax"; a[2] = "yay"; sub(/a/, "b", a[i++]); print i, a[1], a[2] }'

# -- an lvalue's subscript is evaluated once ---------------------------------
check 'x[++n] += 1 and a[i++]++' "1 2 2 1${nl}status 0" \
      'BEGIN { n = 0; x[++n] += 2; i = 1; a[i++]++; print n, x[1], i, a[1] }'

# -- RS="" paragraph mode ----------------------------------------------------
INPUT='a b\nc d\n\ne f\n'
check 'RS="": the last record keeps no newline' \
      "1: [a b${nl}c d]${nl}2: [e f]${nl}status 0" \
      'BEGIN { RS = "" } { print NR ": [" $0 "]" }'
INPUT='a:b\nc:d\n'
check 'RS="": a newline separates fields whatever FS is' "4 c${nl}status 0" \
      'BEGIN { RS = ""; FS = ":" } { print NF, $3 }'
INPUT=''

# -- $0 is rebuilt with the OFS of the last field assignment -----------------
check 'OFS changed after a field assignment' \
      "a:b:3333:d${nl}a<>b<>3333<>x${nl}status 0" \
      'BEGIN { OFS = ":"; $0 = "a b c d"; $3 = "3333"; OFS = "<>"; print; $4 = "x"; print }'

# -- number to text is not cut short ------------------------------------------
check 'CONVFMT and OFMT wider than 63 characters' \
      "202${nl}$(printf '1.25%068d' 0)${nl}status 0" \
      'BEGIN { CONVFMT = "%.200f"; s = 1.25 ""; print length(s); OFMT = "%.70f"; print 1.25 }'

# -- printf and sprintf ------------------------------------------------------
check 'sprintf: a wide field is not cut at 511 bytes' "600${nl}status 0" \
      'BEGIN { print length(sprintf("%600s", "x")) }'
check 'sprintf: a long %s is not cut at 511 bytes' "702${nl}status 0" \
      'BEGIN { s = sprintf("%700s", ""); print length(sprintf("<%s>", s)) }'
check 'sprintf: surplus arguments are evaluated' "2${nl}status 0" \
      'BEGIN { j = 1; s = sprintf("%d", 99, ++j); print j }'
check 'printf %c of a numeric string is a code' "AB${nl}status 0" \
      'BEGIN { split("65 66", a); printf "%c%c\n", a[1], a[2] }'
check 'printf %c of an empty string is nothing' "[][  ]${nl}status 0" \
      'BEGIN { printf "[%c][%2c]\n", "", "" }'
check 'printf %d and %x past 2^64' \
      "18446744073709551616 18446744073709551616${nl}status 0" \
      'BEGIN { printf "%d %x\n", 2^64, 2^64 }'

# -- string literals and command-line assignments ----------------------------
check 'a backslash-newline continues a string literal' "AB${nl}status 0" \
      "$(printf 'BEGIN { print "A\\\nB" }')"
check '-v decodes escapes' "a${tab}b${nl}status 0" \
      -v 'x=a\tb' 'BEGIN { print x }'
INPUT='line\n'
check 'an operand assignment decodes escapes' "1${nl}2${nl}status 0" \
      '{ print x }' 'x=1\n2'
INPUT=''
check 'an operand whose prefix is no name is a file' \
      "dawk: cannot open input file: 99_=foo${nl}status 2" \
      '{ print }' '99_=foo'
check '-v without = is a usage error' \
      "dawk: invalid -v option argument: x${nl}status 2" \
      -v x 'BEGIN { print x }'

# -- program files -----------------------------------------------------------
printf 'BEGIN { print "begin" }\n' > "$work/one.awk"
printf 'END { print "end" }\n' > "$work/two.awk"
printf 'BEGIN {\n  x = 1\n' > "$work/open.awk"
printf '  y = 2\n  print x, y )\n}\n' > "$work/close.awk"
INPUT='data\n'
check 'every -f file, in order' "begin${nl}end${nl}status 0" \
      -f "$work/one.awk" -f "$work/two.awk"
check 'a diagnostic names the -f file it is in' \
      "$work/close.awk:2:14: syntax error: expected a newline or ; before )${nl}status 2" \
      -f "$work/open.awk" -f "$work/close.awk"
INPUT='BEGIN { print "from stdin" }\n'
check '-f - reads the program from standard input' \
      "from stdin${nl}status 0" -f -
INPUT=''

# -- split -------------------------------------------------------------------
check 'split with a one-byte pattern splits as a pattern' "3 4${nl}status 0" \
      'BEGIN { print split("a  b", x, / /), split("a.b", y, /./) }'

# -- builtin arity, and diagnostics that name what they are about ------------
check 'a builtin called with too few arguments' \
      "-:1:9: syntax error: atan2 called with too few arguments${nl}status 2" \
      'BEGIN { atan2(1) }'
check 'split into something that is not an array' \
      "-:1:13: syntax error: split's second argument must be an array name${nl}status 2" \
      'BEGIN { n = split("a", 1) }'
check 'an undefined function is named' \
      "dawk: call to an undefined function: foo${nl}status 2" \
      'BEGIN { foo() }'
check 'NF set to a negative value is refused' \
      "dawk: NF set to a negative value${nl}status 2" \
      'BEGIN { NF = -5 }'
INPUT='x\n'
check 'a field numbered by NaN is refused' \
      "dawk: attempt to access a field < 0${nl}status 2" \
      '{ print $(log(-1)) }'
INPUT=''

# -- a statement ends at a newline, a ;, or a brace -------------------------
check 'two statements with nothing between' \
      "-:1:17: syntax error: expected a newline or ; before print${nl}status 2" \
      'BEGIN { print 1 print 2 }'
check 'return and exit take a signed value' "-1 2${nl}status 254" \
      'function f() { return -1 } BEGIN { print f(), -f() + 1; exit -2 }'
check 'a statement may follow a block on its line' "3${nl}status 0" \
      'BEGIN { while (i++ < 2) { } print i }'

# -- fflush, as POSIX.1-2024 has it ----------------------------------------
check 'fflush: 0 for all or standard output, -1 for a name not open' \
      "a${nl}0 -1 0${nl}status 0" \
      'BEGIN { print "a"; print fflush(), fflush("nothere"), fflush("/dev/stdout") }'
check 'fflush takes one argument at most' \
      "-:1:9: syntax error: fflush called with too many arguments${nl}status 2" \
      'BEGIN { fflush(1, 2) }'

# -- an exit in a function ends the statement that called it ----------------
check 'exit f(): the status the exit within f set' "status 1" \
      'function f() { exit 1 } BEGIN { exit f() }'
check 'print f(): nothing printed once f exits' "in${nl}end${nl}status 3" \
      'function f() { print "in"; exit 3 } BEGIN { print f(); print "no" } END { print "end" }'
check 'an exit two calls down, in an assignment and a condition' "status 5" \
      'function g() { exit 5 } function f() { g(); print "no" } BEGIN { x = f(); if (f()) print "no" }'
INPUT='a\nb\n'
check 'an exit in a pattern ends the input, and runs END' "end 1${nl}status 9" \
      'function f() { exit 9 } f() { print "no" } END { print "end", NR }'
INPUT=''

# -- host functions: an extension's, behind -x --------------------------------
check 'without -x, an extension function is undefined' \
      "dawk: call to an undefined function: story_slides${nl}status 2" \
      'BEGIN { story_slides() }'
check 'an unknown extension is refused' \
      "dawk: unknown extension: nope${nl}status 2" \
      -x nope 'BEGIN { }'
check '-x without a name is refused' \
      "dawk: option -x needs a value${nl}status 2" \
      -x
check 'a host function called with too few arguments' \
      "dawk: story_open called with too few arguments${nl}status 2" \
      -x story 'BEGIN { story_open() }'
check 'a host function called with too many arguments' \
      "dawk: story_match called with too many arguments${nl}status 2" \
      -x story 'BEGIN { story_match("x", "y", 1, 2) }'
check 'a function the program defines wins over a host function' \
      "mine${nl}status 0" \
      -x story 'function story_slides() { return "mine" } BEGIN { print story_slides() }'
check 'a host function that fails is named' \
      "dawk: story_slides: no story is open; call story_open before story_slides${nl}status 2" \
      -x story 'BEGIN { story_slides() }'
check 'a host function that refuses returns, and says why' \
      "-1 no driver reads this kind of file: x.txt${nl}status 0" \
      -x story 'BEGIN { print story_open("x.txt"), story_error() }'

# -- output that cannot be written is an error -------------------------------
(trap '' PIPE; "$DAWK" 'BEGIN { print "hi" }' 2>/dev/null
 echo $? > "$work/status") | :
status=$(cat "$work/status")
if [ "$status" = 2 ]; then
    pass=$((pass + 1))
else
    fail=$((fail + 1))
    echo "FAIL a write to a closed pipe exits 2, not $status"
fi

printf '\n%d passed, %d failed\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
