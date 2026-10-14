/*******************************************************************************
* djinterp [parse]                                           arg_completion_sh.c
*
* Completion scripts for bash and zsh, declared in arg_internal.h (11.2.4).
*   Both scripts hold the model as shell functions -- one `case` statement
* each, which bash from 3.2 on and zsh run alike -- and walk the words
* before the cursor as the parse takes them, in functions the two share:
* which subcommand the words reach, which option's values they are among,
* which positional takes the next word. Each then offers what may come at
* the cursor its own way: bash rejoins the words COMP_WORDBREAKS split and
* fills COMPREPLY as readline replaces the word, escaping what it must; zsh
* gives names and their descriptions to _describe, choices to compadd and
* paths to _files.
*   The script's functions are named after the program, through
* d_arg_internal_complete_ident, so that scripts for several programs live
* side by side; `@@` in the lines below stands for that name.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_completion_sh.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "./arg_internal.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // buffers, statuses


#if (D_INTERNAL_ARG_PARSER_COMPLETION == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// KIND_LETTERS
//   constant: each d_arg_internal_complete_kind as the scripts spell it:
// `-` for none, `c` for choices, `f` for files, `d` for directories and `n`
// for nothing.
static const char KIND_LETTERS[] = "-cfdn";

// WALK_LINES
//   constant: the functions bash and zsh share -- the walk over the words
// before the cursor -- which read the model through the data functions and
// keep their state in the caller's variables: cmd and cf, the command
// reached and its switches; opt, need and room, the option whose values the
// words are among, the values it still needs, and how many more it may take
// (-1 for any number); only, set past `--`; used and counts, the
// subcommands each command was given and the values each positional took;
// extra, the switches of the command a word no positional takes ends at.
static const char* const WALK_LINES[] =
{
    "__djarg_@@_first() {",
    "    # whether `$1` may begin a name",
    "    case $1 in",
    "        ''|-|'!'|[[:space:]]|[[:cntrl:]]) return 1 ;;",
    "    esac",
    "    return 0",
    "}",
    "__djarg_@@_find() {",
    "    # o: the option named `$1` at the command or, falling",
    "    # through, above it",
    "    local L=$cmd p f m r",
    "    o=",
    "    while :; do",
    "        _djarg_@@_opt \"$L\" \"$1\"",
    "        if [[ -n $r ]]; then",
    "            o=$r",
    "            return",
    "        fi",
    "        _djarg_@@_cmd \"$L\"",
    "        if [[ $f == *f* ]] && (( L != 0 )); then",
    "            L=$p",
    "        else",
    "            return",
    "        fi",
    "    done",
    "}",
    "__djarg_@@_target() {",
    "    # t: the subcommand `$1` names, from the command up, and l:",
    "    # the command it is under",
    "    local L=$cmd p f m r",
    "    t= l=",
    "    while :; do",
    "        _djarg_@@_cmd \"$L\"",
    "        if (( m == 0 )) || (( ${used[$L]:-0} < m )) ||",
    "           [[ $f != *s* ]]; then",
    "            _djarg_@@_sub \"$L\" \"$1\"",
    "            if [[ -n $r ]]; then",
    "                t=$r l=$L",
    "                return",
    "            fi",
    "        fi",
    "        if [[ $f == *s* ]] && (( L != 0 )); then",
    "            L=$p",
    "        else",
    "            return",
    "        fi",
    "    done",
    "}",
    "__djarg_@@_class() {",
    "    # r: what `$1` is to the command -- mark, sub, long, short,",
    "    # win, term or none",
    "    local w=$1",
    "    r=none",
    "    if [[ $w == -- ]]; then",
    "        r=mark",
    "        return",
    "    fi",
    "    __djarg_@@_target \"$w\"",
    "    if [[ -n $t ]]; then",
    "        r=sub",
    "        return",
    "    fi",
    "    case $w in",
    "        --?*)",
    "            if __djarg_@@_first \"${w:2:1}\"; then",
    "                r=long",
    "            fi",
    "            return",
    "            ;;",
    "        -?*)",
    "            if ! __djarg_@@_first \"${w:1:1}\"; then",
    "                return",
    "            fi",
    "            case ${w:1:2} in",
    "                [0-9]*|.[0-9])",
    "                    _djarg_@@_opt \"$cmd\" \"-${w:1:1}\"",
    "                    if [[ -z $r ]]; then",
    "                        r=none",
    "                        return",
    "                    fi",
    "                    ;;",
    "            esac",
    "            r=short",
    "            return",
    "            ;;",
    "    esac",
    "    if [[ $cf == *w* ]] && [[ $w == /?* ]] &&",
    "       __djarg_@@_first \"${w:1:1}\"; then",
    "        r=win",
    "    elif [[ $w == ++ ]] && (( cmd != 0 )); then",
    "        r=term",
    "    fi",
    "}",
    "__djarg_@@_take() {",
    "    # o: the positional that takes a word, from the command up,",
    "    # counted when `$1` is 1; extra: the last command's flags",
    "    local L=$cmd s r y p f m",
    "    o=",
    "    while :; do",
    "        s=0",
    "        while :; do",
    "            _djarg_@@_pos \"$L\" \"$s\"",
    "            if [[ -z $r ]]; then",
    "                break",
    "            fi",
    "            if (( y < 0 )) || (( ${counts[$r]:-0} < y )); then",
    "                o=$r",
    "                if (( $1 )); then",
    "                    counts[$r]=$(( ${counts[$r]:-0} + 1 ))",
    "                fi",
    "                return",
    "            fi",
    "            s=$(( s + 1 ))",
    "        done",
    "        _djarg_@@_cmd \"$L\"",
    "        if [[ $f == *f* ]] && (( L != 0 )); then",
    "            L=$p",
    "        else",
    "            extra=$f",
    "            return",
    "        fi",
    "    done",
    "}",
    "__djarg_@@_given() {",
    "    # the option found, met with the value `$1` -- empty for",
    "    # none: what it still needs, and what more it may take",
    "    local n x k got=0",
    "    if [[ -z $o ]]; then",
    "        return",
    "    fi",
    "    _djarg_@@_info \"$o\"",
    "    if (( x == 0 )); then",
    "        return",
    "    fi",
    "    if [[ -n $1 ]]; then",
    "        got=1",
    "    fi",
    "    need=$(( n - got ))",
    "    if (( need < 0 )); then",
    "        need=0",
    "    fi",
    "    if (( x < 0 )); then",
    "        room=-1",
    "    elif (( got > n )); then",
    "        room=$(( x - got ))",
    "    else",
    "        room=$(( x - n ))",
    "    fi",
    "    if (( need > 0 )) || (( room != 0 )); then",
    "        opt=$o",
    "    fi",
    "}",
    "__djarg_@@_step() {",
    "    # one word the parse meets",
    "    local w=$1 r t l o name rest n x k",
    "    __djarg_@@_class \"$w\"",
    "    case $r in",
    "        mark)",
    "            only=1",
    "            ;;",
    "        sub)",
    "            used[$l]=$(( ${used[$l]:-0} + 1 ))",
    "            cmd=$t",
    "            _djarg_@@_cmd \"$cmd\"",
    "            cf=$f",
    "            ;;",
    "        term)",
    "            _djarg_@@_cmd \"$cmd\"",
    "            cmd=$p",
    "            _djarg_@@_cmd \"$cmd\"",
    "            cf=$f",
    "            ;;",
    "        long)",
    "            name=${w%%=*}",
    "            rest=${w#\"$name\"}",
    "            __djarg_@@_find \"$name\"",
    "            __djarg_@@_given \"${rest:1}\"",
    "            ;;",
    "        win)",
    "            name=${w%%:*}",
    "            rest=${w#\"$name\"}",
    "            __djarg_@@_find \"--${name:1}\"",
    "            if [[ -z $o ]]; then",
    "                __djarg_@@_find \"-${name:1}\"",
    "            fi",
    "            __djarg_@@_given \"${rest:1}\"",
    "            ;;",
    "        short)",
    "            rest=${w:1}",
    "            while [[ -n $rest ]]; do",
    "                name=-${rest:0:1}",
    "                rest=${rest:1}",
    "                __djarg_@@_find \"$name\"",
    "                if [[ -z $o ]]; then",
    "                    break",
    "                fi",
    "                _djarg_@@_info \"$o\"",
    "                if (( x != 0 )); then",
    "                    __djarg_@@_given \"$rest\"",
    "                    break",
    "                fi",
    "            done",
    "            ;;",
    "        *)",
    "            __djarg_@@_take 1",
    "            ;;",
    "    esac",
    "}"
};

// BASH_LINES
//   constant: bash's own functions -- the words, rejoined; their quoting
// removed; what may come at the cursor; the reply as readline takes it --
// and the completion function itself.
static const char* const BASH_LINES[] =
{
    "__djarg_@@_words() {",
    "    # words and cword: the line to the cursor in words, rejoining",
    "    # what COMP_WORDBREAKS split; tail: the part readline replaces",
    "    local line=${COMP_LINE:0:COMP_POINT} i=0 w lead",
    "    words=() cword=-1 tail=",
    "    while (( i <= COMP_CWORD )); do",
    "        w=${COMP_WORDS[i]}",
    "        lead=${line%%[![:space:]]*}",
    "        line=${line#\"$lead\"}",
    "        if (( i == COMP_CWORD )); then",
    "            w=$line",
    "        fi",
    "        line=${line#\"$w\"}",
    "        if [[ -n $lead ]] || (( cword < 0 )); then",
    "            cword=$(( cword + 1 ))",
    "            words[cword]=$w",
    "        else",
    "            words[cword]=${words[cword]}$w",
    "        fi",
    "        i=$(( i + 1 ))",
    "    done",
    "    tail=$w",
    "}",
    "__djarg_@@_unq() {",
    "    # r: `$1` with its quotes and escapes removed",
    "    local s=$1 out= ch q=",
    "    case $s in",
    "        *[\\\\\\'\\\"]*) ;;",
    "        *) r=$s; return ;;",
    "    esac",
    "    while [[ -n $s ]]; do",
    "        ch=${s:0:1} s=${s:1}",
    "        if [[ $q == \"'\" ]]; then",
    "            if [[ $ch == \"'\" ]]; then",
    "                q=",
    "            else",
    "                out=$out$ch",
    "            fi",
    "        elif [[ $q == '\"' ]]; then",
    "            if [[ $ch == '\"' ]]; then",
    "                q=",
    "            elif [[ $ch == '\\' ]]; then",
    "                case ${s:0:1} in",
    "                    '\"'|'\\'|'$'|'`') out=$out${s:0:1} s=${s:1} ;;",
    "                    *) out=$out$ch ;;",
    "                esac",
    "            else",
    "                out=$out$ch",
    "            fi",
    "        else",
    "            case $ch in",
    "                \"'\"|'\"') q=$ch ;;",
    "                '\\') out=$out${s:0:1} s=${s:1} ;;",
    "                *) out=$out$ch ;;",
    "            esac",
    "        fi",
    "    done",
    "    r=$out",
    "}",
    "__djarg_@@_kind() {",
    "    # the values of kind `$1` -- c, the choices in c; f, files; d,",
    "    # directories -- that start with `$3`, after `$2`",
    "    local v",
    "    case $1 in",
    "        c)",
    "            if (( ${#c[@]} > 0 )); then",
    "                for v in \"${c[@]}\"; do",
    "                    if [[ $v == \"$3\"* ]]; then",
    "                        reply+=( \"$2$v\" )",
    "                    fi",
    "                done",
    "            fi",
    "            ;;",
    "        f|d)",
    "            files=1",
    "            while IFS= read -r v; do",
    "                reply+=( \"$2$v\" )",
    "            done < <(compgen -\"$1\" -- \"$3\")",
    "            ;;",
    "    esac",
    "}",
    "__djarg_@@_values() {",
    "    # the values option `$1` takes that start with `$3`, after `$2`",
    "    local n x k c=()",
    "    _djarg_@@_info \"$1\"",
    "    if [[ $k == c ]]; then",
    "        _djarg_@@_vals \"$1\"",
    "    fi",
    "    __djarg_@@_kind \"$k\" \"$2\" \"$3\"",
    "}",
    "__djarg_@@_names() {",
    "    # the names list `$1` gives that start with `$2`, each once:",
    "    # at the command and, for options falling through, above it",
    "    local L=$cmd c v p f m seen==",
    "    while :; do",
    "        c=()",
    "        \"$1\" \"$L\"",
    "        if (( ${#c[@]} > 0 )); then",
    "            for v in \"${c[@]}\"; do",
    "                if [[ $v == \"$2\"* ]] && [[ $seen != *\"=$v=\"* ]]; then",
    "                    seen=$seen$v=",
    "                    reply+=( \"$v\" )",
    "                fi",
    "            done",
    "        fi",
    "        _djarg_@@_cmd \"$L\"",
    "        if [[ $1 == *_opts ]] && [[ $f == *f* ]] && (( L != 0 ))",
    "        then",
    "            L=$p",
    "        else",
    "            return",
    "        fi",
    "    done",
    "}",
    "__djarg_@@_reply() {",
    "    # COMPREPLY: the reply as readline replaces the word -- less",
    "    # what precedes the last word break, and escaped unless quoted",
    "    # or completed as file names",
    "    local s=$tail ch pre v q",
    "    while [[ -n $s ]]; do",
    "        ch=${s:0:1} s=${s:1}",
    "        case $COMP_WORDBREAKS in",
    "            *\"$ch\"*) ;;",
    "            *) break ;;",
    "        esac",
    "        if [[ -z $s ]]; then",
    "            tail=",
    "        fi",
    "    done",
    "    __djarg_@@_unq \"${words[cword]%\"$tail\"}\"",
    "    pre=$r",
    "    COMPREPLY=()",
    "    if (( ${#reply[@]} > 0 )); then",
    "        for v in \"${reply[@]}\"; do",
    "            if [[ $v != \"$pre\"* ]]; then",
    "                continue",
    "            fi",
    "            v=${v#\"$pre\"}",
    "            case $tail in",
    "                \\'*|\\\"*) ;;",
    "                *)",
    "                    if (( ! files )); then",
    "                        printf -v q '%q' \"$v\"",
    "                        v=$q",
    "                    fi",
    "                    ;;",
    "            esac",
    "            COMPREPLY+=( \"$v\" )",
    "        done",
    "    fi",
    "    if (( files )) && type compopt >/dev/null 2>&1; then",
    "        compopt -o filenames",
    "    fi",
    "}",
    "__djarg_@@_offer() {",
    "    # reply: what may come at the cursor, `$cur`",
    "    if (( need > 0 )); then",
    "        __djarg_@@_values \"$opt\" \"\" \"$cur\"",
    "        return",
    "    fi",
    "    if (( only )); then",
    "        __djarg_@@_take 0",
    "        if [[ -n $o ]]; then",
    "            __djarg_@@_values \"$o\" \"\" \"$cur\"",
    "        fi",
    "        return",
    "    fi",
    "    if [[ -n $opt ]]; then",
    "        __djarg_@@_values \"$opt\" \"\" \"$cur\"",
    "    fi",
    "    case $cur in",
    "        --*=*)",
    "            __djarg_@@_find \"${cur%%=*}\"",
    "            if [[ -n $o ]]; then",
    "                __djarg_@@_values \"$o\" \"${cur%%=*}=\" \"${cur#*=}\"",
    "            fi",
    "            ;;",
    "        -*)",
    "            __djarg_@@_names _djarg_@@_opts \"$cur\"",
    "            ;;",
    "        *)",
    "            __djarg_@@_take 0",
    "            if [[ -n $o ]]; then",
    "                __djarg_@@_values \"$o\" \"\" \"$cur\"",
    "            elif [[ $extra == *e* ]]; then",
    "                __djarg_@@_kind f \"\" \"$cur\"",
    "            fi",
    "            _djarg_@@_cmd \"$cmd\"",
    "            c=()",
    "            if (( m == 0 )) || (( ${used[$cmd]:-0} < m )); then",
    "                _djarg_@@_subs \"$cmd\"",
    "            fi",
    "            if (( ${#c[@]} > 0 )); then",
    "                __djarg_@@_names _djarg_@@_subs \"$cur\"",
    "            elif [[ -z $o ]] && [[ $extra != *e* ]]; then",
    "                __djarg_@@_names _djarg_@@_opts \"$cur\"",
    "            fi",
    "            ;;",
    "    esac",
    "}",
    "_djarg_@@() {",
    "    local words cword tail cur r t l o p f m n x k cf w i",
    "    local cmd=0 opt= need=0 room=0 only=0 extra= files=0 nocase=0",
    "    local -a used counts reply c",
    "    if shopt -q nocasematch; then",
    "        nocase=1",
    "        shopt -u nocasematch",
    "    fi",
    "    __djarg_@@_words",
    "    _djarg_@@_cmd 0",
    "    cf=$f",
    "    i=1",
    "    while (( i < cword )); do",
    "        __djarg_@@_unq \"${words[i]}\"",
    "        w=$r",
    "        i=$(( i + 1 ))",
    "        if (( need > 0 )); then",
    "            need=$(( need - 1 ))",
    "            if (( need == 0 )) && (( room == 0 )); then",
    "                opt=",
    "            fi",
    "            continue",
    "        fi",
    "        if (( only )); then",
    "            __djarg_@@_take 1",
    "            continue",
    "        fi",
    "        if [[ -n $opt ]]; then",
    "            if [[ $w == -- ]]; then",
    "                opt=",
    "                continue",
    "            fi",
    "            __djarg_@@_class \"$w\"",
    "            if [[ $r == none ]] && (( room != 0 )); then",
    "                if (( room > 0 )); then",
    "                    room=$(( room - 1 ))",
    "                fi",
    "                if (( room == 0 )); then",
    "                    opt=",
    "                fi",
    "                continue",
    "            fi",
    "            opt=",
    "        fi",
    "        __djarg_@@_step \"$w\"",
    "    done",
    "    __djarg_@@_unq \"${words[cword]}\"",
    "    cur=$r",
    "    reply=()",
    "    __djarg_@@_offer",
    "    __djarg_@@_reply",
    "    if (( nocase )); then",
    "        shopt -s nocasematch",
    "    fi",
    "}"
};

// ZSH_LINES
//   constant: zsh's own functions -- what may come at the cursor, offered
// through the completion system -- and the completion function itself.
static const char* const ZSH_LINES[] =
{
    "__djarg_@@_values() {",
    "    # offers the values option `$1` takes",
    "    local n x k c=()",
    "    _djarg_@@_info \"$1\"",
    "    case $k in",
    "        c)",
    "            _djarg_@@_vals \"$1\"",
    "            compadd -a c",
    "            ;;",
    "        f)",
    "            _files",
    "            ;;",
    "        d)",
    "            _files -/",
    "            ;;",
    "    esac",
    "}",
    "__djarg_@@_names() {",
    "    # offers the names list `$1` gives, each once with its",
    "    # description, as `$2` and `$3`: at the command and, for",
    "    # options falling through, above it",
    "    local L=$cmd c d p f m i",
    "    local -a specs",
    "    local -A seen",
    "    while :; do",
    "        c=() d=()",
    "        \"$1\" \"$L\"",
    "        i=1",
    "        while (( i <= ${#c[@]} )); do",
    "            if (( ! ${+seen[${c[i]}]} )); then",
    "                seen[${c[i]}]=1",
    "                specs+=( \"${c[i]//:/\\\\:}:${d[i]}\" )",
    "            fi",
    "            i=$(( i + 1 ))",
    "        done",
    "        _djarg_@@_cmd \"$L\"",
    "        if [[ $1 == *_opts ]] && [[ $f == *f* ]] && (( L != 0 ))",
    "        then",
    "            L=$p",
    "        else",
    "            break",
    "        fi",
    "    done",
    "    _describe -t \"$2\" \"$3\" specs",
    "}",
    "__djarg_@@_offer() {",
    "    # offers what may come at the cursor, `$cur`",
    "    if (( need > 0 )); then",
    "        __djarg_@@_values \"$opt\"",
    "        return",
    "    fi",
    "    if (( only )); then",
    "        __djarg_@@_take 0",
    "        if [[ -n $o ]]; then",
    "            __djarg_@@_values \"$o\"",
    "        fi",
    "        return",
    "    fi",
    "    if [[ -n $opt ]]; then",
    "        __djarg_@@_values \"$opt\"",
    "    fi",
    "    case $cur in",
    "        --*=*)",
    "            __djarg_@@_find \"${cur%%=*}\"",
    "            if [[ -n $o ]]; then",
    "                compset -P 1 '*='",
    "                __djarg_@@_values \"$o\"",
    "            fi",
    "            ;;",
    "        -*)",
    "            __djarg_@@_names _djarg_@@_opts options option",
    "            ;;",
    "        *)",
    "            __djarg_@@_take 0",
    "            if [[ -n $o ]]; then",
    "                __djarg_@@_values \"$o\"",
    "            elif [[ $extra == *e* ]]; then",
    "                _files",
    "            fi",
    "            _djarg_@@_cmd \"$cmd\"",
    "            c=()",
    "            if (( m == 0 )) || (( ${used[$cmd]:-0} < m )); then",
    "                _djarg_@@_subs \"$cmd\"",
    "            fi",
    "            if (( ${#c[@]} > 0 )); then",
    "                __djarg_@@_names _djarg_@@_subs commands command",
    "            elif [[ -z $o ]] && [[ $extra != *e* ]]; then",
    "                __djarg_@@_names _djarg_@@_opts options option",
    "            fi",
    "            ;;",
    "    esac",
    "}",
    "_djarg_@@() {",
    "    local cur r t l o p f m n x k cf w i d",
    "    local cmd=0 opt= need=0 room=0 only=0 extra=",
    "    local -a c",
    "    local -A used counts",
    "    _djarg_@@_cmd 0",
    "    cf=$f",
    "    i=2",
    "    while (( i < CURRENT )); do",
    "        w=${(Q)words[i]}",
    "        i=$(( i + 1 ))",
    "        if (( need > 0 )); then",
    "            need=$(( need - 1 ))",
    "            if (( need == 0 )) && (( room == 0 )); then",
    "                opt=",
    "            fi",
    "            continue",
    "        fi",
    "        if (( only )); then",
    "            __djarg_@@_take 1",
    "            continue",
    "        fi",
    "        if [[ -n $opt ]]; then",
    "            if [[ $w == -- ]]; then",
    "                opt=",
    "                continue",
    "            fi",
    "            __djarg_@@_class \"$w\"",
    "            if [[ $r == none ]] && (( room != 0 )); then",
    "                if (( room > 0 )); then",
    "                    room=$(( room - 1 ))",
    "                fi",
    "                if (( room == 0 )); then",
    "                    opt=",
    "                fi",
    "                continue",
    "            fi",
    "            opt=",
    "        fi",
    "        __djarg_@@_step \"$w\"",
    "    done",
    "    cur=${(Q)PREFIX}",
    "    __djarg_@@_offer",
    "}"
};

// WALK_LINE_COUNT, BASH_LINE_COUNT, ZSH_LINE_COUNT
//   constants: how many lines each of the three holds.
static const size_t WALK_LINE_COUNT =
    sizeof(WALK_LINES) / sizeof(WALK_LINES[0]);
static const size_t BASH_LINE_COUNT =
    sizeof(BASH_LINES) / sizeof(BASH_LINES[0]);
static const size_t ZSH_LINE_COUNT =
    sizeof(ZSH_LINES) / sizeof(ZSH_LINES[0]);

//==============================================================================
// 1.  QUOTING
//==============================================================================

/*
d_arg_internal_complete_sh_text
  Text inside single quotes: each `'` closed, escaped and reopened, as
'\'' does it; as a line of a description, each control character a space.
*/
D_STATIC void
d_arg_internal_complete_sh_text(
    struct d_arg_buffer* _out,
    struct d_pack_text   _text,
    bool                 _line
)
{
    // each byte
    for (size_t index = 0u; index < _text.length; ++index)
    {
        const char          c    = _text.data[index];
        const unsigned char byte = (unsigned char)c;

        if (c == '\'')
        {
            (void)d_arg_buffer_append_cstr(_out,
                                           "'\\''");
        }
        else if ( (_line) &&
                  ( (byte < 0x20u) ||
                    (byte == 0x7Fu) ) )
        {
            (void)d_arg_buffer_append_char(_out,
                                           ' ');
        }
        else
        {
            (void)d_arg_buffer_append_char(_out,
                                           c);
        }
    }

    return;
}

/*
d_arg_internal_complete_sh_word
  Text as one single-quoted word, after a space.
*/
D_STATIC void
d_arg_internal_complete_sh_word(
    struct d_arg_buffer* _out,
    struct d_pack_text   _text,
    bool                 _line
)
{
    (void)d_arg_buffer_append_cstr(_out,
                                   " '");
    d_arg_internal_complete_sh_text(_out,
                                    _text,
                                    _line);
    (void)d_arg_buffer_append_char(_out,
                                   '\'');

    return;
}

/*
d_arg_internal_complete_sh_entry
  An option's name, with its dashes, as one single-quoted word after a
space.
*/
D_STATIC void
d_arg_internal_complete_sh_entry(
    struct d_arg_buffer*                        _out,
    const struct d_arg_internal_complete_entry* _entry
)
{
    (void)d_arg_buffer_append_cstr(_out,
                                   (_entry->long_name) ? " '--" : " '-");
    d_arg_internal_complete_sh_text(_out,
                                    d_arg_internal_text_of(_entry->name),
                                    false);
    (void)d_arg_buffer_append_char(_out,
                                   '\'');

    return;
}

//==============================================================================
// 2.  THE DATA
//==============================================================================

/*
d_arg_internal_complete_sh_open
  A data function's first lines: its name, after the program's, and the
`case` over `_subject`.
*/
D_STATIC void
d_arg_internal_complete_sh_open(
    struct d_arg_buffer* _out,
    struct d_pack_text   _ident,
    const char*          _name,
    const char*          _subject
)
{
    (void)d_arg_buffer_append_cstr(_out,
                                   "_djarg_");
    (void)d_arg_buffer_append_text(_out,
                                   _ident);
    (void)d_arg_buffer_printf(_out,
                              "_%s() {\n    case %s in\n",
                              _name,
                              _subject);

    return;
}

/*
d_arg_internal_complete_sh_close
  A data function's last lines: the `case` for anything else, doing
`_fallback`, and the ends.
*/
D_STATIC void
d_arg_internal_complete_sh_close(
    struct d_arg_buffer* _out,
    const char*          _fallback
)
{
    (void)d_arg_buffer_printf(_out,
                              "        *) %s ;;\n    esac\n}\n",
                              _fallback);

    return;
}

/*
d_arg_internal_complete_sh_commands
  _cmd: each command's parent, switches and most subcommands, as p, f and
m; and _sub: the command each subcommand name leads to under a command, as
r, empty for none.
*/
D_STATIC void
d_arg_internal_complete_sh_commands(
    const struct d_arg_internal_model* _model,
    struct d_pack_text                 _ident,
    struct d_arg_buffer*               _out
)
{
    d_arg_internal_complete_sh_open(_out,
                                    _ident,
                                    "cmd",
                                    "$1");

    // each command
    for (size_t index = 0u; index < _model->command_count; ++index)
    {
        const struct d_arg_internal_complete_command* const command =
            &_model->commands[index];

        (void)d_arg_buffer_printf(_out,
                                  "        %zu) p=%zu f=%s m=%zu ;;\n",
                                  index,
                                  command->parent,
                                  command->flags,
                                  command->most);
    }

    d_arg_internal_complete_sh_close(_out,
                                     "p=0 f=- m=0");
    d_arg_internal_complete_sh_open(_out,
                                    _ident,
                                    "sub",
                                    "$1:$2");

    // each subcommand name
    for (size_t index = 0u; index < _model->name_count; ++index)
    {
        const struct d_arg_internal_complete_name* const name =
            &_model->names[index];

        (void)d_arg_buffer_printf(_out,
                                  "        '%zu:",
                                  name->command);
        d_arg_internal_complete_sh_text(_out,
                                        d_arg_internal_text_of(name->name),
                                        false);
        (void)d_arg_buffer_printf(_out,
                                  "') r=%zu ;;\n",
                                  name->target);
    }

    d_arg_internal_complete_sh_close(_out,
                                     "r=");

    return;
}

/*
d_arg_internal_complete_sh_options
  _opt: the option each name stands for under a command, as r, empty for
none; and _info: each option's fewest and most values after a name, and
the letter of its kind, as n, x and k.
*/
D_STATIC void
d_arg_internal_complete_sh_options(
    const struct d_arg_internal_model* _model,
    struct d_pack_text                 _ident,
    struct d_arg_buffer*               _out
)
{
    d_arg_internal_complete_sh_open(_out,
                                    _ident,
                                    "opt",
                                    "$1:$2");

    // each option name
    for (size_t index = 0u; index < _model->entry_count; ++index)
    {
        const struct d_arg_internal_complete_entry* const entry =
            &_model->entries[index];

        (void)d_arg_buffer_printf(_out,
                                  "        '%zu:%s",
                                  _model->options[entry->option].command,
                                  (entry->long_name) ? "--" : "-");
        d_arg_internal_complete_sh_text(_out,
                                        d_arg_internal_text_of(entry->name),
                                        false);
        (void)d_arg_buffer_printf(_out,
                                  "') r=%zu ;;\n",
                                  entry->option);
    }

    d_arg_internal_complete_sh_close(_out,
                                     "r=");
    d_arg_internal_complete_sh_open(_out,
                                    _ident,
                                    "info",
                                    "$1");

    // each option
    for (size_t index = 0u; index < _model->option_count; ++index)
    {
        const struct d_arg_internal_complete_option* const option =
            &_model->options[index];

        (void)d_arg_buffer_printf(_out,
                                  "        %zu) n=%d x=%d k=%c ;;\n",
                                  index,
                                  option->least,
                                  option->most,
                                  KIND_LETTERS[option->kind]);
    }

    d_arg_internal_complete_sh_close(_out,
                                     "n=0 x=0 k=-");

    return;
}

/*
d_arg_internal_complete_sh_slots
  _pos: a command's positionals in the order they take words, each as its
option, r -- empty past the last -- and the most values it takes, y.
*/
D_STATIC void
d_arg_internal_complete_sh_slots(
    const struct d_arg_internal_model* _model,
    struct d_pack_text                 _ident,
    struct d_arg_buffer*               _out
)
{
    size_t command = 0u;
    size_t slot    = 0u;

    d_arg_internal_complete_sh_open(_out,
                                    _ident,
                                    "pos",
                                    "$1:$2");

    // each positional, counted within its command
    for (size_t index = 0u; index < _model->option_count; ++index)
    {
        const struct d_arg_internal_complete_option* const option =
            &_model->options[index];

        if (option->slots == 0)
        {
            continue;
        }

        slot    = (option->command == command) ? slot : 0u;
        command = option->command;
        (void)d_arg_buffer_printf(_out,
                                  "        %zu:%zu) r=%zu y=%d ;;\n",
                                  command,
                                  slot,
                                  index,
                                  option->slots);
        ++slot;
    }

    d_arg_internal_complete_sh_close(_out,
                                     "r=");

    return;
}

/*
d_arg_internal_complete_sh_list_names
  Names as single-quoted words: `_count` of the model's subcommand names
from `_first`, or of its option names, with their dashes.
*/
D_STATIC void
d_arg_internal_complete_sh_list_names(
    const struct d_arg_internal_model* _model,
    size_t                             _first,
    size_t                             _count,
    bool                               _options,
    struct d_arg_buffer*               _out
)
{
    // each name
    for (size_t index = _first; index < _first + _count; ++index)
    {
        if (_options)
        {
            d_arg_internal_complete_sh_entry(_out,
                                             &_model->entries[index]);
        }
        else
        {
            d_arg_internal_complete_sh_word(
                _out,
                d_arg_internal_text_of(_model->names[index].name),
                false);
        }
    }

    return;
}

/*
d_arg_internal_complete_sh_list_texts
  The descriptions of the names d_arg_internal_complete_sh_list_names
writes, as single-quoted words, each a line.
*/
D_STATIC void
d_arg_internal_complete_sh_list_texts(
    const struct d_arg_internal_model* _model,
    size_t                             _first,
    size_t                             _count,
    bool                               _options,
    struct d_arg_buffer*               _out
)
{
    // each description
    for (size_t index = _first; index < _first + _count; ++index)
    {
        d_arg_internal_complete_sh_word(
            _out,
            (_options)
            ? _model->options[_model->entries[index].option].summary
            : _model->names[index].summary,
            true);
    }

    return;
}

/*
d_arg_internal_complete_sh_list
  One command's line of _subs or _opts: its subcommand names, or its option
names, as the array c, and, when described, their descriptions as the
array d; nothing for a command without any.
*/
D_STATIC void
d_arg_internal_complete_sh_list(
    const struct d_arg_internal_model* _model,
    size_t                             _command,
    bool                               _options,
    bool                               _described,
    struct d_arg_buffer*               _out
)
{
    const struct d_arg_internal_complete_command* const command =
        &_model->commands[_command];
    const size_t first = (_options) ? command->first_entry
                                    : command->first_name;
    const size_t count = (_options) ? command->entries
                                    : command->names;

    // nothing to list
    if (count == 0u)
    {
        return;
    }

    (void)d_arg_buffer_printf(_out,
                              "        %zu) c=(",
                              _command);
    d_arg_internal_complete_sh_list_names(_model,
                                          first,
                                          count,
                                          _options,
                                          _out);

    // the descriptions, when asked for
    if (_described)
    {
        (void)d_arg_buffer_append_cstr(_out,
                                       " ) d=(");
        d_arg_internal_complete_sh_list_texts(_model,
                                              first,
                                              count,
                                              _options,
                                              _out);
    }

    (void)d_arg_buffer_append_cstr(_out,
                                   " ) ;;\n");

    return;
}

/*
d_arg_internal_complete_sh_names
  _subs and _opts: each command's subcommand names, and its option names,
as the array c -- and for zsh their descriptions, as the array d.
*/
D_STATIC void
d_arg_internal_complete_sh_names(
    const struct d_arg_internal_model* _model,
    struct d_pack_text                 _ident,
    bool                               _described,
    struct d_arg_buffer*               _out
)
{
    const char* const fallback = (_described) ? "c=() d=()" : "c=()";

    d_arg_internal_complete_sh_open(_out,
                                    _ident,
                                    "subs",
                                    "$1");

    // each command's subcommand names
    for (size_t index = 0u; index < _model->command_count; ++index)
    {
        d_arg_internal_complete_sh_list(_model,
                                        index,
                                        false,
                                        _described,
                                        _out);
    }

    d_arg_internal_complete_sh_close(_out,
                                     fallback);
    d_arg_internal_complete_sh_open(_out,
                                    _ident,
                                    "opts",
                                    "$1");

    // each command's option names
    for (size_t index = 0u; index < _model->command_count; ++index)
    {
        d_arg_internal_complete_sh_list(_model,
                                        index,
                                        true,
                                        _described,
                                        _out);
    }

    d_arg_internal_complete_sh_close(_out,
                                     fallback);

    return;
}

/*
d_arg_internal_complete_sh_values
  _vals: each option's choices, as the array c.
*/
D_STATIC void
d_arg_internal_complete_sh_values(
    const struct d_arg_internal_model* _model,
    struct d_pack_text                 _ident,
    struct d_arg_buffer*               _out
)
{
    d_arg_internal_complete_sh_open(_out,
                                    _ident,
                                    "vals",
                                    "$1");

    // each option with choices
    for (size_t index = 0u; index < _model->option_count; ++index)
    {
        const struct d_arg_internal_complete_option* const option =
            &_model->options[index];

        if (option->choices == 0u)
        {
            continue;
        }

        (void)d_arg_buffer_printf(_out,
                                  "        %zu) c=(",
                                  index);

        // each choice
        for (size_t choice = option->first_choice;
             choice < option->first_choice + option->choices;
             ++choice)
        {
            d_arg_internal_complete_sh_word(
                _out,
                d_arg_internal_text_of(_model->choices.items[choice]),
                false);
        }

        (void)d_arg_buffer_append_cstr(_out,
                                       " ) ;;\n");
    }

    d_arg_internal_complete_sh_close(_out,
                                     "c=()");

    return;
}

/*
d_arg_internal_complete_sh_data
  Every data function, the descriptions only when asked.
*/
D_STATIC void
d_arg_internal_complete_sh_data(
    const struct d_arg_internal_model* _model,
    struct d_pack_text                 _ident,
    bool                               _described,
    struct d_arg_buffer*               _out
)
{
    d_arg_internal_complete_sh_commands(_model,
                                        _ident,
                                        _out);
    d_arg_internal_complete_sh_options(_model,
                                       _ident,
                                       _out);
    d_arg_internal_complete_sh_slots(_model,
                                     _ident,
                                     _out);
    d_arg_internal_complete_sh_names(_model,
                                     _ident,
                                     _described,
                                     _out);
    d_arg_internal_complete_sh_values(_model,
                                      _ident,
                                      _out);

    return;
}

//==============================================================================
// 3.  THE SCRIPTS
//==============================================================================

/*
d_arg_internal_complete_sh_header
  A script's first comment: the shell, the program and how to load it.
*/
D_STATIC void
d_arg_internal_complete_sh_header(
    struct d_arg_buffer* _out,
    const char*          _shell,
    struct d_pack_text   _program,
    const char*          _loading
)
{
    (void)d_arg_buffer_printf(_out,
                              "# %s completion for ",
                              _shell);
    d_arg_internal_complete_sh_text(_out,
                                    _program,
                                    true);
    (void)d_arg_buffer_append_cstr(_out,
                                   ", generated by djinterp's arg_parser.\n");
    (void)d_arg_buffer_append_cstr(_out,
                                   _loading);

    return;
}

/*
d_arg_internal_complete_bash
  The header, the data, the shared walk, bash's own functions, and the
completion function registered for the program.
*/
int
d_arg_internal_complete_bash(
    const struct d_arg_internal_model* _model,
    struct d_pack_text                 _program,
    struct d_arg_buffer*               _out
)
{
    struct d_arg_buffer ident  = D_ARG_BUFFER_INIT;
    int                 status = d_arg_internal_complete_ident(_program,
                                                               &ident);

    d_arg_internal_complete_sh_header(
        _out,
        "bash",
        _program,
        "# Source it in bash, or save it among bash-completion's "
        "completions.\n\n");
    d_arg_internal_complete_sh_data(_model,
                                    d_arg_buffer_text(&ident),
                                    false,
                                    _out);
    (void)d_arg_internal_complete_lines(WALK_LINES,
                                        WALK_LINE_COUNT,
                                        d_arg_buffer_text(&ident),
                                        _out);
    (void)d_arg_internal_complete_lines(BASH_LINES,
                                        BASH_LINE_COUNT,
                                        d_arg_buffer_text(&ident),
                                        _out);
    (void)d_arg_buffer_append_cstr(_out,
                                   "complete -F _djarg_");
    (void)d_arg_buffer_append_text(_out,
                                   d_arg_buffer_text(&ident));
    d_arg_internal_complete_sh_word(_out,
                                    _program,
                                    false);
    (void)d_arg_buffer_append_char(_out,
                                   '\n');
    status = ( (status == D_ARG_OK) &&
               (_out->failed == 0) )
             ? D_ARG_OK
             : D_ARG_ERROR_OUT_OF_MEMORY;
    d_arg_buffer_release(&ident);

    return status;
}

/*
d_arg_internal_complete_zsh_tag
  Whether the program's name can follow `#compdef`: not empty, not an
option of it, and without a space or a control character.
*/
D_STATIC bool
d_arg_internal_complete_zsh_tag(
    struct d_pack_text _program
)
{
    // not empty, nor an option
    if ( (_program.length == 0u) ||
         (_program.data[0] == '-') )
    {
        return false;
    }

    // no space or control character
    for (size_t index = 0u; index < _program.length; ++index)
    {
        if ((unsigned char)_program.data[index] <= 0x20u)
        {
            return false;
        }
    }

    return true;
}

/*
d_arg_internal_complete_zsh_footer
  How the script ends: autoloaded as _<program> -- the file's name, as
`#compdef` loads it -- it registers itself and completes at once; sourced,
it registers itself when compinit is loaded, and says what is missing
when it is not.
*/
D_STATIC void
d_arg_internal_complete_zsh_footer(
    struct d_arg_buffer* _out,
    struct d_pack_text   _program,
    struct d_pack_text   _ident
)
{
    struct d_arg_buffer name = D_ARG_BUFFER_INIT;

    (void)d_arg_buffer_append_cstr(&name,
                                   "compdef _djarg_");
    (void)d_arg_buffer_append_text(&name,
                                   _ident);
    d_arg_internal_complete_sh_word(&name,
                                    _program,
                                    false);
    (void)d_arg_buffer_append_cstr(_out,
                                   "if [[ ${funcstack[1]-} == '_");
    d_arg_internal_complete_sh_text(_out,
                                    _program,
                                    false);
    (void)d_arg_buffer_printf(_out,
                              "' ]]; then\n    %s\n    _djarg_",
                              (name.data != NULL) ? name.data : "");
    (void)d_arg_buffer_append_text(_out,
                                   _ident);
    (void)d_arg_buffer_printf(_out,
                              " \"$@\"\nelif (( ${+functions[compdef]} ));"
                              " then\n    %s\nelse\n    print -ru2 --",
                              (name.data != NULL) ? name.data : "");
    d_arg_internal_complete_sh_word(_out,
                                    _program,
                                    true);
    (void)d_arg_buffer_append_cstr(_out,
                                   " 'completion needs compinit first'\n"
                                   "fi\n");
    _out->failed |= name.failed;
    d_arg_buffer_release(&name);

    return;
}

/*
d_arg_internal_complete_zsh
  `#compdef` for the program, when its name allows; the header, the data
with descriptions, the shared walk, zsh's own functions, and the footer.
*/
int
d_arg_internal_complete_zsh(
    const struct d_arg_internal_model* _model,
    struct d_pack_text                 _program,
    struct d_arg_buffer*               _out
)
{
    struct d_arg_buffer ident  = D_ARG_BUFFER_INIT;
    int                 status = d_arg_internal_complete_ident(_program,
                                                               &ident);

    // loaded by `#compdef` when saved as _<program>
    if (d_arg_internal_complete_zsh_tag(_program))
    {
        (void)d_arg_buffer_append_cstr(_out,
                                       "#compdef ");
        (void)d_arg_buffer_append_text(_out,
                                       _program);
        (void)d_arg_buffer_append_char(_out,
                                       '\n');
    }

    d_arg_internal_complete_sh_header(
        _out,
        "zsh",
        _program,
        "# Source it after compinit, or save it as _<program> in a "
        "directory of $fpath.\n\n");
    d_arg_internal_complete_sh_data(_model,
                                    d_arg_buffer_text(&ident),
                                    true,
                                    _out);
    (void)d_arg_internal_complete_lines(WALK_LINES,
                                        WALK_LINE_COUNT,
                                        d_arg_buffer_text(&ident),
                                        _out);
    (void)d_arg_internal_complete_lines(ZSH_LINES,
                                        ZSH_LINE_COUNT,
                                        d_arg_buffer_text(&ident),
                                        _out);
    d_arg_internal_complete_zsh_footer(_out,
                                       _program,
                                       d_arg_buffer_text(&ident));
    status = ( (status == D_ARG_OK) &&
               (_out->failed == 0) )
             ? D_ARG_OK
             : D_ARG_ERROR_OUT_OF_MEMORY;
    d_arg_buffer_release(&ident);

    return status;
}

#endif  // D_INTERNAL_ARG_PARSER_COMPLETION
