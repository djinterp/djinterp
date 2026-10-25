/*******************************************************************************
* djinterp [parse]                                         arg_completion_pwsh.c
*
* The completion script for PowerShell, declared in arg_internal.h (11.2.4).
*   The script runs in a block of its own, whose variables hold the model --
* each command's row, the subcommands' and the options' names with their
* commands, targets and descriptions, each option's row, the positionals'
* keys and rows, the choices -- and turn it into dictionaries that compare
* names by ordinal, as the parse does, where PowerShell's own hash tables
* would ignore case. The completer, a closure over them registered for the
* program as a native command, walks the words before the cursor as the
* parse takes them and returns CompletionResult objects -- names with their
* descriptions as tool tips, values quoted where PowerShell needs them, files
* through PowerShell's own completer. Where nothing fits a word, it returns
* the word itself, so that PowerShell does not offer files in its place.
*   Every byte the script holds is ASCII: text with other characters is
* written as a concatenation of quoted runs and [char] values, so that the
* script survives being read through any code page, as Windows PowerShell
* reads a program's output.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_completion_pwsh.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "./arg_internal.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // buffers, statuses
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t


#if (D_INTERNAL_ARG_PARSER_COMPLETION == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// KIND_LETTERS
//   constant: each d_arg_internal_complete_kind as the script spells it:
// `-` for none, `c` for choices, `f` for files, `d` for directories and `n`
// for nothing.
static const char KIND_LETTERS[] = "-cfdn";

// REPLACEMENT
//   constant: U+FFFD, which stands for a byte that starts no UTF-8 sequence.
static const uint32_t REPLACEMENT = 0xFFFDu;

// PWSH_LINES
//   constant: the block's own part: the dictionaries, then the completer --
// whose functions keep the walk's state in the hash table $s: cmd and cf,
// the command reached and its switches; opt, need and room, the option
// whose values the words are among, the values it still needs and how many
// more it may take (-1 for any number); only, set past `--`; used and
// counts, the subcommands each command was given and the values each
// positional took; extra, the switches of the command a word no positional
// takes ends at.
static const char* const PWSH_LINES[] =
{
    "    $ordinal = [System.StringComparer]::Ordinal",
    "    $sk = [Collections.Generic.Dictionary[string,int]]::new(",
    "        $ordinal)",
    "    for ($i = 0; $i -lt $sn.Count; $i++) {",
    "        $sk[[string]$sc[$i] + ':' + $sn[$i]] = $st[$i]",
    "    }",
    "    $ok = [Collections.Generic.Dictionary[string,int]]::new(",
    "        $ordinal)",
    "    for ($i = 0; $i -lt $on.Count; $i++) {",
    "        $ok[[string]$oc[$i] + ':' + $on[$i]] = $ov[$i]",
    "    }",
    "    $pk = [Collections.Generic.Dictionary[string,string]]::new(",
    "        $ordinal)",
    "    for ($i = 0; $i -lt $ps.Count; $i++) {",
    "        $pk[$ps[$i]] = $pv[$i]",
    "    }",
    "    $complete = {",
    "        param($wordToComplete, $commandAst, $cursorPosition)",
    "        $s = @{ cmd = 0; cf = ''; opt = -1; need = 0; room = 0",
    "                only = $false; extra = ''; used = @{}; counts = @{} }",
    "        $ord = [System.StringComparison]::Ordinal",
    "        $any = [System.StringComparison]::OrdinalIgnoreCase",
    "        $completers = [Management.Automation.CompletionCompleters]",
    "        $result = [Management.Automation.CompletionResult]",
    "        $constant = ('System.Management.Automation.Language.' +",
    "                     'StringConstantExpressionAst') -as [type]",
    "        function Cmd([int]$L) {",
    "            # the command's row",
    "            $e = $cmds[$L].Split(' ')",
    "            @{ p = [int]$e[0]; f = $e[1]; m = [int]$e[2]",
    "               sa = [int]$e[3]; sc = [int]$e[4]",
    "               oa = [int]$e[5]; oc = [int]$e[6] }",
    "        }",
    "        function Info([int]$o) {",
    "            # the option's row",
    "            $e = $info[$o].Split(' ')",
    "            @{ n = [int]$e[0]; x = [int]$e[1]; k = $e[2]",
    "               va = [int]$e[3]; vc = [int]$e[4] }",
    "        }",
    "        function First([string]$c) {",
    "            # whether the character may begin a name",
    "            ($c.Length -gt 0) -and",
    "                ($c -cnotmatch '^[-!\\s\\x00-\\x1f\\x7f]')",
    "        }",
    "        function Used([int]$L) {",
    "            # the subcommands the command was given",
    "            if ($s.used.ContainsKey($L)) {",
    "                return $s.used[$L]",
    "            }",
    "            return 0",
    "        }",
    "        function Find([string]$name) {",
    "            # the option named at the command or, falling",
    "            # through, above it; -1 for none",
    "            $L = $s.cmd",
    "            while ($true) {",
    "                $v = 0",
    "                if ($ok.TryGetValue([string]$L + ':' + $name,",
    "                                    [ref]$v)) {",
    "                    return $v",
    "                }",
    "                $c = Cmd $L",
    "                if ($c.f.Contains('f') -and ($L -ne 0)) {",
    "                    $L = $c.p",
    "                }",
    "                else {",
    "                    return -1",
    "                }",
    "            }",
    "        }",
    "        function Target([string]$w) {",
    "            # the subcommand the word names, from the command up,",
    "            # and the command it is under; $null for none",
    "            $L = $s.cmd",
    "            while ($true) {",
    "                $c = Cmd $L",
    "                if (($c.m -eq 0) -or ((Used $L) -lt $c.m) -or",
    "                    (-not $c.f.Contains('s'))) {",
    "                    $t = 0",
    "                    if ($sk.TryGetValue([string]$L + ':' + $w,",
    "                                        [ref]$t)) {",
    "                        return @($t, $L)",
    "                    }",
    "                }",
    "                if ($c.f.Contains('s') -and ($L -ne 0)) {",
    "                    $L = $c.p",
    "                }",
    "                else {",
    "                    return $null",
    "                }",
    "            }",
    "        }",
    "        function Class([string]$w) {",
    "            # what the word is to the command",
    "            if ($w -ceq '--') {",
    "                return 'mark'",
    "            }",
    "            if ($null -ne (Target $w)) {",
    "                return 'sub'",
    "            }",
    "            if ($w.StartsWith('--', $ord)) {",
    "                if (($w.Length -gt 2) -and (First $w.Substring(2, 1))) {",
    "                    return 'long'",
    "                }",
    "                return 'none'",
    "            }",
    "            if ($w.StartsWith('-', $ord) -and ($w.Length -gt 1)) {",
    "                $first = $w.Substring(1, 1)",
    "                if (-not (First $first)) {",
    "                    return 'none'",
    "                }",
    "                if (($w -cmatch '^-([0-9]|\\.[0-9])') -and",
    "                    (-not $ok.ContainsKey([string]$s.cmd + ':-' +",
    "                                          $first))) {",
    "                    return 'none'",
    "                }",
    "                return 'short'",
    "            }",
    "            if ($s.cf.Contains('w') -and $w.StartsWith('/', $ord) -and",
    "                ($w.Length -gt 1) -and (First $w.Substring(1, 1))) {",
    "                return 'win'",
    "            }",
    "            if (($w -ceq '++') -and ($s.cmd -ne 0)) {",
    "                return 'term'",
    "            }",
    "            return 'none'",
    "        }",
    "        function Take([bool]$count) {",
    "            # the positional that takes a word, from the command",
    "            # up, counted when asked; -1 for none",
    "            $L = $s.cmd",
    "            while ($true) {",
    "                $slot = 0",
    "                while ($true) {",
    "                    $e = $null",
    "                    if (-not $pk.TryGetValue([string]$L + ':' + $slot,",
    "                                             [ref]$e)) {",
    "                        break",
    "                    }",
    "                    $e = $e.Split(' ')",
    "                    $o = [int]$e[0]",
    "                    $c = 0",
    "                    if ($s.counts.ContainsKey($o)) {",
    "                        $c = $s.counts[$o]",
    "                    }",
    "                    if (([int]$e[1] -lt 0) -or ($c -lt [int]$e[1])) {",
    "                        if ($count) {",
    "                            $s.counts[$o] = $c + 1",
    "                        }",
    "                        return $o",
    "                    }",
    "                    $slot++",
    "                }",
    "                $c = Cmd $L",
    "                if ($c.f.Contains('f') -and ($L -ne 0)) {",
    "                    $L = $c.p",
    "                }",
    "                else {",
    "                    $s.extra = $c.f",
    "                    return -1",
    "                }",
    "            }",
    "        }",
    "        function Given([int]$o, [string]$value) {",
    "            # the option met with a value -- empty for none: what",
    "            # it still needs, and what more it may take",
    "            if ($o -lt 0) {",
    "                return",
    "            }",
    "            $i = Info $o",
    "            if ($i.x -eq 0) {",
    "                return",
    "            }",
    "            $got = 0",
    "            if ($value.Length -gt 0) {",
    "                $got = 1",
    "            }",
    "            $s.need = [Math]::Max(0, $i.n - $got)",
    "            if ($i.x -lt 0) {",
    "                $s.room = -1",
    "            }",
    "            elseif ($got -gt $i.n) {",
    "                $s.room = $i.x - $got",
    "            }",
    "            else {",
    "                $s.room = $i.x - $i.n",
    "            }",
    "            if (($s.need -gt 0) -or ($s.room -ne 0)) {",
    "                $s.opt = $o",
    "            }",
    "        }",
    "        function Named([string]$w, [string]$at) {",
    "            # a long or Windows-style name met, with its value",
    "            $i = $w.IndexOf($at)",
    "            $name = $w",
    "            $value = ''",
    "            if ($i -ge 0) {",
    "                $name = $w.Substring(0, $i)",
    "                $value = $w.Substring($i + 1)",
    "            }",
    "            if ($at -ceq '=') {",
    "                $o = Find $name",
    "            }",
    "            else {",
    "                $o = Find ('--' + $name.Substring(1))",
    "                if ($o -lt 0) {",
    "                    $o = Find ('-' + $name.Substring(1))",
    "                }",
    "            }",
    "            Given $o $value",
    "        }",
    "        function Step([string]$w) {",
    "            # one word the parse meets",
    "            switch (Class $w) {",
    "                'mark' {",
    "                    $s.only = $true",
    "                }",
    "                'sub' {",
    "                    $t = Target $w",
    "                    $s.used[$t[1]] = (Used $t[1]) + 1",
    "                    $s.cmd = $t[0]",
    "                    $s.cf = (Cmd $s.cmd).f",
    "                }",
    "                'term' {",
    "                    $s.cmd = (Cmd $s.cmd).p",
    "                    $s.cf = (Cmd $s.cmd).f",
    "                }",
    "                'long' {",
    "                    Named $w '='",
    "                }",
    "                'win' {",
    "                    Named $w ':'",
    "                }",
    "                'short' {",
    "                    $rest = $w.Substring(1)",
    "                    while ($rest.Length -gt 0) {",
    "                        $o = Find ('-' + $rest.Substring(0, 1))",
    "                        $rest = $rest.Substring(1)",
    "                        if ($o -lt 0) {",
    "                            break",
    "                        }",
    "                        if ((Info $o).x -ne 0) {",
    "                            Given $o $rest",
    "                            break",
    "                        }",
    "                    }",
    "                }",
    "                default {",
    "                    $null = Take $true",
    "                }",
    "            }",
    "        }",
    "        function Result([string]$text, [string]$pre, [string]$type,",
    "                        [string]$tip) {",
    "            # a completion of the text, quoted where it must be",
    "            $q = $text",
    "            if (($q.Length -eq 0) -or",
    "                ($q -match '[\\s''\"`$(){}<>;,&|@#\\u2018-\\u201b]')) {",
    "                $quotes = '[''\\u2018-\\u201b]'",
    "                $q = \"'\" + ($q -replace $quotes, '$0$0') + \"'\"",
    "            }",
    "            if ([string]::IsNullOrEmpty($tip)) {",
    "                $tip = $text",
    "            }",
    "            $list = $text",
    "            if ($list.Length -eq 0) {",
    "                $list = \"''\"",
    "            }",
    "            $result::new($pre + $q, $list, $type, $tip)",
    "        }",
    "        function Kind([string]$k, [int]$va, [int]$vc, [string]$pre,",
    "                      [string]$word) {",
    "            # the values of a kind -- c, choices; f, files; d,",
    "            # directories -- that start with the word, after $pre",
    "            if ($k -ceq 'c') {",
    "                for ($i = $va; $i -lt $va + $vc; $i++) {",
    "                    if ($vv[$i].StartsWith($word, $any)) {",
    "                        Result $vv[$i] $pre 'ParameterValue' ''",
    "                    }",
    "                }",
    "            }",
    "            elseif (($k -ceq 'f') -or ($k -ceq 'd')) {",
    "                $all = $completers::CompleteFilename($word)",
    "                foreach ($r in $all) {",
    "                    if (($k -ceq 'f') -or",
    "                        ($r.ResultType -eq 'ProviderContainer')) {",
    "                        $result::new($pre + $r.CompletionText,",
    "                                     $r.ListItemText, $r.ResultType,",
    "                                     $r.ToolTip)",
    "                    }",
    "                }",
    "            }",
    "        }",
    "        function Values([int]$o, [string]$pre, [string]$word) {",
    "            # the values the option takes",
    "            $i = Info $o",
    "            Kind $i.k $i.va $i.vc $pre $word",
    "        }",
    "        function Names([string]$which, [string]$word) {",
    "            # the names -- s, subcommands; o, options -- that start",
    "            # with the word, each once with its description: at the",
    "            # command and, for options falling through, above it",
    "            $L = $s.cmd",
    "            $seen = @{}",
    "            while ($true) {",
    "                $c = Cmd $L",
    "                $a = $c.sa",
    "                $z = $c.sa + $c.sc",
    "                $names = $sn",
    "                $texts = $sd",
    "                $type = 'Command'",
    "                if ($which -ceq 'o') {",
    "                    $a = $c.oa",
    "                    $z = $c.oa + $c.oc",
    "                    $names = $on",
    "                    $texts = $od",
    "                    $type = 'ParameterName'",
    "                }",
    "                for ($i = $a; $i -lt $z; $i++) {",
    "                    $name = $names[$i]",
    "                    if ((-not $seen.ContainsKey($name)) -and",
    "                        $name.StartsWith($word, $any)) {",
    "                        $seen[$name] = 1",
    "                        Result $name '' $type $texts[$i]",
    "                    }",
    "                }",
    "                if (($which -ceq 'o') -and $c.f.Contains('f') -and",
    "                    ($L -ne 0)) {",
    "                    $L = $c.p",
    "                }",
    "                else {",
    "                    return",
    "                }",
    "            }",
    "        }",
    "        function Offer([string]$cur) {",
    "            # what may come at the cursor",
    "            if ($s.need -gt 0) {",
    "                Values $s.opt '' $cur",
    "                return",
    "            }",
    "            if ($s.only) {",
    "                $o = Take $false",
    "                if ($o -ge 0) {",
    "                    Values $o '' $cur",
    "                }",
    "                return",
    "            }",
    "            if ($s.opt -ge 0) {",
    "                Values $s.opt '' $cur",
    "            }",
    "            if ($cur -cmatch '^--[^=]*=') {",
    "                $i = $cur.IndexOf('=')",
    "                $o = Find $cur.Substring(0, $i)",
    "                if ($o -ge 0) {",
    "                    $pre = $cur.Substring(0, $i + 1)",
    "                    Values $o $pre $cur.Substring($i + 1)",
    "                }",
    "            }",
    "            elseif ($cur.StartsWith('-', $ord)) {",
    "                Names 'o' $cur",
    "            }",
    "            else {",
    "                $o = Take $false",
    "                if ($o -ge 0) {",
    "                    Values $o '' $cur",
    "                }",
    "                elseif ($s.extra.Contains('e')) {",
    "                    Kind 'f' 0 0 '' $cur",
    "                }",
    "                $c = Cmd $s.cmd",
    "                if (($c.sc -gt 0) -and",
    "                    (($c.m -eq 0) -or ((Used $s.cmd) -lt $c.m))) {",
    "                    Names 's' $cur",
    "                }",
    "                elseif (($o -lt 0) -and (-not $s.extra.Contains('e'))) {",
    "                    Names 'o' $cur",
    "                }",
    "            }",
    "        }",
    "        function Main {",
    "            # the words to the cursor walked, then what may come",
    "            $words = @()",
    "            $elements = $commandAst.CommandElements",
    "            for ($i = 1; $i -lt $elements.Count; $i++) {",
    "                $e = $elements[$i]",
    "                if ($e.Extent.EndOffset -ge $cursorPosition) {",
    "                    break",
    "                }",
    "                if ($e -is $constant) {",
    "                    $words += , $e.Value",
    "                }",
    "                else {",
    "                    $words += , $e.Extent.Text",
    "                }",
    "            }",
    "            $s.cf = (Cmd 0).f",
    "            foreach ($w in $words) {",
    "                if ($s.need -gt 0) {",
    "                    $s.need--",
    "                    if (($s.need -eq 0) -and ($s.room -eq 0)) {",
    "                        $s.opt = -1",
    "                    }",
    "                    continue",
    "                }",
    "                if ($s.only) {",
    "                    $null = Take $true",
    "                    continue",
    "                }",
    "                if ($s.opt -ge 0) {",
    "                    if ($w -ceq '--') {",
    "                        $s.opt = -1",
    "                        continue",
    "                    }",
    "                    if (((Class $w) -ceq 'none') -and ($s.room -ne 0)) {",
    "                        if ($s.room -gt 0) {",
    "                            $s.room--",
    "                        }",
    "                        if ($s.room -eq 0) {",
    "                            $s.opt = -1",
    "                        }",
    "                        continue",
    "                    }",
    "                    $s.opt = -1",
    "                }",
    "                Step $w",
    "            }",
    "            $cur = $wordToComplete",
    "            if ($cur -cmatch \"^'(.*)'$\") {",
    "                $cur = $Matches[1] -replace \"''\", \"'\"",
    "            }",
    "            elseif ($cur -cmatch '^\"(.*)\"$') {",
    "                $cur = $Matches[1]",
    "            }",
    "            elseif ($cur -cmatch '^[''\"]') {",
    "                $cur = $cur.Substring(1)",
    "            }",
    "            Offer $cur",
    "        }",
    "        $results = @(Main)",
    "        if (($results.Count -eq 0) -and",
    "            ($wordToComplete.Length -gt 0)) {",
    "            # the word as it stands, so that PowerShell does not offer",
    "            # files where nothing fits",
    "            $w = $wordToComplete",
    "            $result::new($w, $w, 'ParameterValue', $w)",
    "        }",
    "        else {",
    "            $results",
    "        }",
    "    }.GetNewClosure()"
};

// PWSH_LINE_COUNT
//   constant: how many lines PWSH_LINES holds.
static const size_t PWSH_LINE_COUNT =
    sizeof(PWSH_LINES) / sizeof(PWSH_LINES[0]);

//==============================================================================
// 1.  QUOTING
//==============================================================================

/**
 * @brief Decodes the code point UTF-8 text holds at a position.
 *
 * @param[in]  _text  the text.
 * @param[in]  _at    the position, within the text.
 * @param[out] _size  receives the sequence's length in bytes; 1 for a byte that
 *                    starts no valid sequence.
 * @return the code point; U+FFFD for a byte that starts no valid sequence.
 *
 * The code point UTF-8 text holds at `_at`, its length in bytes stored in
 * `*_size`: U+FFFD, one byte long, for a byte that starts no valid sequence.
 */
D_STATIC uint32_t
d_arg_internal_complete_pwsh_decode(
    struct d_pack_text _text,
    size_t             _at,
    size_t*            _size
)
{
    const unsigned char* const bytes = (const unsigned char*)_text.data + _at;
    const size_t               left  = _text.length - _at;
    const unsigned char        lead  = bytes[0];
    const size_t               size  = (lead < 0xC2u) ? 1u
                                     : (lead < 0xE0u) ? 2u
                                     : (lead < 0xF0u) ? 3u
                                     : (lead < 0xF5u) ? 4u
                                                      : 1u;
    const unsigned char        low   = (lead == 0xE0u) ? 0xA0u
                                     : (lead == 0xF0u) ? 0x90u
                                                       : 0x80u;
    const unsigned char        high  = (lead == 0xEDu) ? 0x9Fu
                                     : (lead == 0xF4u) ? 0x8Fu
                                                       : 0xBFu;
    uint32_t                   point = (size == 1u)
                                       ? lead
                                       : (uint32_t)lead &
                                         (0x7Fu >> size);

    *_size = 1u;

    // ASCII, a stray byte, or a sequence cut short
    if ( (lead < 0x80u)          ||
         (size == 1u)            ||
         (left < size)           ||
         (bytes[1] < low)        ||
         (bytes[1] > high) )
    {
        return (lead < 0x80u) ? lead : REPLACEMENT;
    }

    // each continuation byte
    for (size_t index = 1u; index < size; ++index)
    {
        if ((bytes[index] & 0xC0u) != 0x80u)
        {
            return REPLACEMENT;
        }

        point = (point << 6) | (uint32_t)(bytes[index] & 0x3Fu);
    }

    *_size = size;

    return point;
}

/**
 * @brief Tells whether a character goes inside single quotes as itself.
 *
 * @param[in] _point  the character.
 * @param[in] _line   whether it is in a line of a description, where a control
 *                    character becomes a space.
 * @return `true` for printable ASCII, and in a line for a control character;
 *         `false` otherwise.
 *
 * Whether a character goes inside single quotes as itself: printable ASCII
 * -- and, in a line of a description, a control character, which becomes a
 * space.
 */
D_STATIC bool
d_arg_internal_complete_pwsh_plain(
    uint32_t _point,
    bool     _line
)
{
    return ( (_point < 0x7Fu) &&
             ( (_point >= 0x20u) ||
               (_line) ) ) ||
           ( (_point == 0x7Fu) &&
             (_line) );
}

/**
 * @brief Appends a character outside single quotes, as [char] values.
 *
 * @param[in,out] _out    the buffer that gains the values, its failure kept in
 *                        it.
 * @param[in]     _point  the character.
 *
 * A character outside single quotes, as one [char] value per UTF-16 unit,
 * each after " + ".
 */
D_STATIC void
d_arg_internal_complete_pwsh_char(
    struct d_arg_buffer* _out,
    uint32_t             _point
)
{
    // in the basic plane
    if (_point < 0x10000u)
    {
        (void)d_arg_buffer_printf(_out,
                                  " + [char]0x%04lX",
                                  (unsigned long)_point);

        return;
    }

    (void)d_arg_buffer_printf(
        _out,
        " + [char]0x%04lX + [char]0x%04lX",
        (unsigned long)(0xD800u + ((_point - 0x10000u) >> 10)),
        (unsigned long)(0xDC00u + ((_point - 0x10000u) & 0x3FFu)));

    return;
}

/**
 * @brief Appends one character of a string being written.
 *
 * @param[in,out] _out    the buffer that gains it, its failure kept in it.
 * @param[in]     _point  the character.
 * @param[in]     _line   whether the string is a line of a description.
 * @param[in,out] _open   whether a quoted run is open; updated.
 *
 * One character of a string being written: inside the quoted run -- one
 * opened, after " + ", when none is -- as itself, `'` doubled, or as a space
 * for a control character of a line; else after the run, closed, as [char]
 * values.
 */
D_STATIC void
d_arg_internal_complete_pwsh_put(
    struct d_arg_buffer* _out,
    uint32_t             _point,
    bool                 _line,
    bool*                _open
)
{
    // a character for a [char] value
    if (!d_arg_internal_complete_pwsh_plain(_point,
                                            _line))
    {
        (void)d_arg_buffer_append_cstr(_out,
                                       (*_open) ? "'" : "");
        *_open = false;
        d_arg_internal_complete_pwsh_char(_out,
                                          _point);

        return;
    }

    (void)d_arg_buffer_append_cstr(_out,
                                   (*_open) ? "" : " + '");
    *_open = true;
    (void)d_arg_buffer_append_cstr(
        _out,
        (_point == (uint32_t)'\'')
        ? "''"
        : ( (_point < 0x20u) ||
            (_point == 0x7Fu) )
          ? " "
          : "");

    // the character itself
    if ( (_point != (uint32_t)'\'') &&
         (_point >= 0x20u)          &&
         (_point < 0x7Fu) )
    {
        (void)d_arg_buffer_append_char(_out,
                                       (char)_point);
    }

    return;
}

/**
 * @brief Appends text as a PowerShell string after a space.
 *
 * @param[in,out] _out   the buffer that gains the string, its failure kept in
 *                       it.
 * @param[in]     _text  the text.
 * @param[in]     _line  whether it is a line of a description, each control
 *                       character then a space.
 *
 * Text as a PowerShell string after a space: in single quotes, each `'`
 * doubled, when every character goes inside them as itself; otherwise as a
 * concatenation, in parentheses, of the empty string, quoted runs and [char]
 * values. As a line of a description, each control character a space.
 */
D_STATIC void
d_arg_internal_complete_pwsh_text(
    struct d_arg_buffer* _out,
    struct d_pack_text   _text,
    bool                 _line
)
{
    bool   plain = true;
    size_t size  = 1u;

    // whether one quoted run holds it
    for (size_t at = 0u; at < _text.length; at += size)
    {
        plain = ( (plain) &&
                  (d_arg_internal_complete_pwsh_plain(
                       d_arg_internal_complete_pwsh_decode(_text,
                                                           at,
                                                           &size),
                       _line)) );
    }

    (void)d_arg_buffer_append_cstr(_out,
                                   (plain) ? " '" : " (''");

    bool open = plain;

    // each character
    for (size_t at = 0u; at < _text.length; at += size)
    {
        const uint32_t point = d_arg_internal_complete_pwsh_decode(_text,
                                                                   at,
                                                                   &size);

        d_arg_internal_complete_pwsh_put(_out,
                                         point,
                                         _line,
                                         &open);
    }

    (void)d_arg_buffer_append_cstr(_out,
                                   (open) ? "'" : "");
    (void)d_arg_buffer_append_cstr(_out,
                                   (plain) ? "" : ")");

    return;
}

/**
 * @brief Appends an option's name with its dashes, as a PowerShell string after
 *        a space.
 *
 * @param[in,out] _out    the buffer that gains the string, its failure kept in
 *                        it.
 * @param[in]     _entry  the name's entry in the model.
 *
 * An option's name with its dashes, as a PowerShell string after a space.
 */
D_STATIC void
d_arg_internal_complete_pwsh_name(
    struct d_arg_buffer*                        _out,
    const struct d_arg_internal_complete_entry* _entry
)
{
    struct d_arg_buffer name = D_ARG_BUFFER_INIT;

    (void)d_arg_buffer_append_cstr(&name,
                                   (_entry->long_name) ? "--" : "-");
    (void)d_arg_buffer_append_cstr(&name,
                                   _entry->name);
    d_arg_internal_complete_pwsh_text(_out,
                                      d_arg_buffer_text(&name),
                                      false);
    _out->failed |= name.failed;
    d_arg_buffer_release(&name);

    return;
}

//==============================================================================
// 2.  THE DATA
//==============================================================================

// d_arg_internal_complete_pwsh_value
//   type: writes an array's value for one record of the model -- the line
// it takes, indented -- or nothing, for a record the array leaves out.
typedef void (*d_arg_internal_complete_pwsh_value)(
    const struct d_arg_internal_model* _model,
    size_t                             _index,
    struct d_arg_buffer*               _out);

// d_arg_internal_complete_pwsh_table
//   struct: one array of the script's data: its name, how many records of
// the model it covers, and the function that writes each record's value.
struct d_arg_internal_complete_pwsh_table
{
    const char*                        name;
    size_t                             count;
    d_arg_internal_complete_pwsh_value value;
};

/**
 * @brief Appends an array variable of the script block.
 *
 * @param[in]     _model  the model.
 * @param[in]     _table  the array's name, how many records it covers, and the
 *                        function that writes each one's value.
 * @param[in,out] _out    the buffer that gains the array, its failure kept in
 *                        it.
 *
 * An array variable of the block: a value for each record the table covers,
 * each on a line of its own.
 */
D_STATIC void
d_arg_internal_complete_pwsh_array(
    const struct d_arg_internal_model*               _model,
    const struct d_arg_internal_complete_pwsh_table* _table,
    struct d_arg_buffer*                             _out
)
{
    (void)d_arg_buffer_printf(_out,
                              "    $%s = @(\n",
                              _table->name);

    // each record's value
    for (size_t index = 0u; index < _table->count; ++index)
    {
        _table->value(_model,
                      index,
                      _out);
    }

    (void)d_arg_buffer_append_cstr(_out,
                                   "    )\n");

    return;
}

/**
 * @brief Appends a command's row of the `cmds` array.
 *
 * @param[in]     _model  the model.
 * @param[in]     _index  the command's place in the model.
 * @param[in,out] _out    the buffer that gains the value, its failure kept in
 *                        it.
 *
 * A command's row: its parent, switches and most subcommands, then where
 * its subcommand names begin and how many, and where its option names begin
 * and how many.
 */
D_STATIC void
d_arg_internal_complete_pwsh_cmds(
    const struct d_arg_internal_model* _model,
    size_t                             _index,
    struct d_arg_buffer*               _out
)
{
    const struct d_arg_internal_complete_command* const command =
        &_model->commands[_index];

    (void)d_arg_buffer_printf(_out,
                              "        '%zu %s %zu %zu %zu %zu %zu'\n",
                              command->parent,
                              command->flags,
                              command->most,
                              command->first_name,
                              command->names,
                              command->first_entry,
                              command->entries);

    return;
}

/**
 * @brief Appends a subcommand name.
 *
 * @param[in]     _model  the model.
 * @param[in]     _index  the name's place in the model.
 * @param[in,out] _out    the buffer that gains the value, its failure kept in
 *                        it.
 *
 * A subcommand name.
 */
D_STATIC void
d_arg_internal_complete_pwsh_sn(
    const struct d_arg_internal_model* _model,
    size_t                             _index,
    struct d_arg_buffer*               _out
)
{
    (void)d_arg_buffer_append_cstr(_out,
                                   "       ");
    d_arg_internal_complete_pwsh_text(
        _out,
        d_arg_internal_text_of(_model->names[_index].name),
        false);
    (void)d_arg_buffer_append_char(_out,
                                   '\n');

    return;
}

/**
 * @brief Appends the command a subcommand name is under.
 *
 * @param[in]     _model  the model.
 * @param[in]     _index  the name's place in the model.
 * @param[in,out] _out    the buffer that gains the value, its failure kept in
 *                        it.
 *
 * The command a subcommand name is under.
 */
D_STATIC void
d_arg_internal_complete_pwsh_sc(
    const struct d_arg_internal_model* _model,
    size_t                             _index,
    struct d_arg_buffer*               _out
)
{
    (void)d_arg_buffer_printf(_out,
                              "        %zu\n",
                              _model->names[_index].command);

    return;
}

/**
 * @brief Appends the command a subcommand name leads to.
 *
 * @param[in]     _model  the model.
 * @param[in]     _index  the name's place in the model.
 * @param[in,out] _out    the buffer that gains the value, its failure kept in
 *                        it.
 *
 * The command a subcommand name leads to.
 */
D_STATIC void
d_arg_internal_complete_pwsh_st(
    const struct d_arg_internal_model* _model,
    size_t                             _index,
    struct d_arg_buffer*               _out
)
{
    (void)d_arg_buffer_printf(_out,
                              "        %zu\n",
                              _model->names[_index].target);

    return;
}

/**
 * @brief Appends a subcommand name's description.
 *
 * @param[in]     _model  the model.
 * @param[in]     _index  the name's place in the model.
 * @param[in,out] _out    the buffer that gains the value, its failure kept in
 *                        it.
 *
 * A subcommand name's description.
 */
D_STATIC void
d_arg_internal_complete_pwsh_sd(
    const struct d_arg_internal_model* _model,
    size_t                             _index,
    struct d_arg_buffer*               _out
)
{
    (void)d_arg_buffer_append_cstr(_out,
                                   "       ");
    d_arg_internal_complete_pwsh_text(_out,
                                      _model->names[_index].summary,
                                      true);
    (void)d_arg_buffer_append_char(_out,
                                   '\n');

    return;
}

/**
 * @brief Appends an option name, with its dashes.
 *
 * @param[in]     _model  the model.
 * @param[in]     _index  the name's place in the model.
 * @param[in,out] _out    the buffer that gains the value, its failure kept in
 *                        it.
 *
 * An option name, with its dashes.
 */
D_STATIC void
d_arg_internal_complete_pwsh_on(
    const struct d_arg_internal_model* _model,
    size_t                             _index,
    struct d_arg_buffer*               _out
)
{
    (void)d_arg_buffer_append_cstr(_out,
                                   "       ");
    d_arg_internal_complete_pwsh_name(_out,
                                      &_model->entries[_index]);
    (void)d_arg_buffer_append_char(_out,
                                   '\n');

    return;
}

/**
 * @brief Appends the command an option name is under: its option's.
 *
 * @param[in]     _model  the model.
 * @param[in]     _index  the name's place in the model.
 * @param[in,out] _out    the buffer that gains the value, its failure kept in
 *                        it.
 *
 * The command an option name is under: its option's.
 */
D_STATIC void
d_arg_internal_complete_pwsh_oc(
    const struct d_arg_internal_model* _model,
    size_t                             _index,
    struct d_arg_buffer*               _out
)
{
    (void)d_arg_buffer_printf(
        _out,
        "        %zu\n",
        _model->options[_model->entries[_index].option].command);

    return;
}

/**
 * @brief Appends the option a name stands for.
 *
 * @param[in]     _model  the model.
 * @param[in]     _index  the name's place in the model.
 * @param[in,out] _out    the buffer that gains the value, its failure kept in
 *                        it.
 *
 * The option a name stands for.
 */
D_STATIC void
d_arg_internal_complete_pwsh_ov(
    const struct d_arg_internal_model* _model,
    size_t                             _index,
    struct d_arg_buffer*               _out
)
{
    (void)d_arg_buffer_printf(_out,
                              "        %zu\n",
                              _model->entries[_index].option);

    return;
}

/**
 * @brief Appends an option name's description: its option's.
 *
 * @param[in]     _model  the model.
 * @param[in]     _index  the name's place in the model.
 * @param[in,out] _out    the buffer that gains the value, its failure kept in
 *                        it.
 *
 * An option name's description: its option's.
 */
D_STATIC void
d_arg_internal_complete_pwsh_od(
    const struct d_arg_internal_model* _model,
    size_t                             _index,
    struct d_arg_buffer*               _out
)
{
    (void)d_arg_buffer_append_cstr(_out,
                                   "       ");
    d_arg_internal_complete_pwsh_text(
        _out,
        _model->options[_model->entries[_index].option].summary,
        true);
    (void)d_arg_buffer_append_char(_out,
                                   '\n');

    return;
}

/**
 * @brief Appends an option's row of the `info` array.
 *
 * @param[in]     _model  the model.
 * @param[in]     _index  the option's place in the model.
 * @param[in,out] _out    the buffer that gains the value, its failure kept in
 *                        it.
 *
 * An option's row: its fewest and most values after a name, the letter of
 * its kind, and where its choices begin and how many.
 */
D_STATIC void
d_arg_internal_complete_pwsh_info(
    const struct d_arg_internal_model* _model,
    size_t                             _index,
    struct d_arg_buffer*               _out
)
{
    const struct d_arg_internal_complete_option* const option =
        &_model->options[_index];

    (void)d_arg_buffer_printf(_out,
                              "        '%d %d %c %zu %zu'\n",
                              option->least,
                              option->most,
                              KIND_LETTERS[option->kind],
                              option->first_choice,
                              option->choices);

    return;
}

/**
 * @brief Finds where a positional comes among its command's.
 *
 * @param[in] _model  the model.
 * @param[in] _index  the positional's place in the model.
 * @return how many positionals of the same command come before it.
 *
 * Where a positional comes among its command's: the positionals before it
 * under the same command.
 */
D_STATIC size_t
d_arg_internal_complete_pwsh_slot(
    const struct d_arg_internal_model* _model,
    size_t                             _index
)
{
    size_t slot = 0u;

    // each option before it
    for (size_t index = 0u; index < _index; ++index)
    {
        if ( (_model->options[index].slots != 0) &&
             (_model->options[index].command ==
              _model->options[_index].command) )
        {
            ++slot;
        }
    }

    return slot;
}

/**
 * @brief Appends a positional's key, as "command:slot".
 *
 * @param[in]     _model  the model.
 * @param[in]     _index  the option's place in the model; one that is no
 *                        positional writes nothing.
 * @param[in,out] _out    the buffer that gains the value, its failure kept in
 *                        it.
 *
 * A positional's key, as "command:slot"; nothing for another option.
 */
D_STATIC void
d_arg_internal_complete_pwsh_ps(
    const struct d_arg_internal_model* _model,
    size_t                             _index,
    struct d_arg_buffer*               _out
)
{
    // a positional only
    if (_model->options[_index].slots == 0)
    {
        return;
    }

    (void)d_arg_buffer_printf(
        _out,
        "        '%zu:%zu'\n",
        _model->options[_index].command,
        d_arg_internal_complete_pwsh_slot(_model,
                                          _index));

    return;
}

/**
 * @brief Appends a positional's row: its option and the most values it takes.
 *
 * @param[in]     _model  the model.
 * @param[in]     _index  the option's place in the model; one that is no
 *                        positional writes nothing.
 * @param[in,out] _out    the buffer that gains the value, its failure kept in
 *                        it.
 *
 * A positional's row: its option and the most values it takes; nothing for
 * another option.
 */
D_STATIC void
d_arg_internal_complete_pwsh_pv(
    const struct d_arg_internal_model* _model,
    size_t                             _index,
    struct d_arg_buffer*               _out
)
{
    // a positional only
    if (_model->options[_index].slots == 0)
    {
        return;
    }

    (void)d_arg_buffer_printf(_out,
                              "        '%zu %d'\n",
                              _index,
                              _model->options[_index].slots);

    return;
}

/**
 * @brief Appends a choice.
 *
 * @param[in]     _model  the model.
 * @param[in]     _index  the choice's place in the model.
 * @param[in,out] _out    the buffer that gains the value, its failure kept in
 *                        it.
 *
 * A choice.
 */
D_STATIC void
d_arg_internal_complete_pwsh_vv(
    const struct d_arg_internal_model* _model,
    size_t                             _index,
    struct d_arg_buffer*               _out
)
{
    (void)d_arg_buffer_append_cstr(_out,
                                   "       ");
    d_arg_internal_complete_pwsh_text(
        _out,
        d_arg_internal_text_of(_model->choices.items[_index]),
        false);
    (void)d_arg_buffer_append_char(_out,
                                   '\n');

    return;
}

/**
 * @brief Appends every array of the model.
 *
 * @param[in]     _model  the model.
 * @param[in,out] _out    the buffer that gains the arrays, its failure kept in
 *                        it.
 *
 * Every array of the model.
 */
D_STATIC void
d_arg_internal_complete_pwsh_data(
    const struct d_arg_internal_model* _model,
    struct d_arg_buffer*               _out
)
{
    const struct d_arg_internal_complete_pwsh_table arrays[] =
    {
        { "cmds", _model->command_count, d_arg_internal_complete_pwsh_cmds },
        { "sn",   _model->name_count,    d_arg_internal_complete_pwsh_sn   },
        { "sc",   _model->name_count,    d_arg_internal_complete_pwsh_sc   },
        { "st",   _model->name_count,    d_arg_internal_complete_pwsh_st   },
        { "sd",   _model->name_count,    d_arg_internal_complete_pwsh_sd   },
        { "on",   _model->entry_count,   d_arg_internal_complete_pwsh_on   },
        { "oc",   _model->entry_count,   d_arg_internal_complete_pwsh_oc   },
        { "ov",   _model->entry_count,   d_arg_internal_complete_pwsh_ov   },
        { "od",   _model->entry_count,   d_arg_internal_complete_pwsh_od   },
        { "info", _model->option_count,  d_arg_internal_complete_pwsh_info },
        { "ps",   _model->option_count,  d_arg_internal_complete_pwsh_ps   },
        { "pv",   _model->option_count,  d_arg_internal_complete_pwsh_pv   },
        { "vv",   _model->choices.count, d_arg_internal_complete_pwsh_vv   }
    };
    const size_t count = sizeof(arrays) / sizeof(arrays[0]);

    // each array
    for (size_t index = 0u; index < count; ++index)
    {
        d_arg_internal_complete_pwsh_array(_model,
                                           &arrays[index],
                                           _out);
    }

    return;
}

//==============================================================================
// 3.  THE SCRIPT
//==============================================================================

/**
 * @brief Appends a program's name as it goes in a comment.
 *
 * @param[in,out] _out      the buffer that gains the name, its failure kept in
 *                          it.
 * @param[in]     _program  the name.
 *
 * The program's name in a comment: printable ASCII as it is, every other
 * byte a `?`.
 */
D_STATIC void
d_arg_internal_complete_pwsh_comment(
    struct d_arg_buffer* _out,
    struct d_pack_text   _program
)
{
    // each byte
    for (size_t index = 0u; index < _program.length; ++index)
    {
        const unsigned char byte = (unsigned char)_program.data[index];

        (void)d_arg_buffer_append_char(_out,
                                       ( (byte >= 0x20u) &&
                                         (byte < 0x7Fu) )
                                       ? _program.data[index]
                                       : '?');
    }

    return;
}

/**
 * @brief Appends the line that registers the completer for a program's names.
 *
 * @param[in,out] _out      the buffer that gains the line, its failure kept in
 *                          it.
 * @param[in]     _program  the program's name.
 *
 * The names the completer is registered for: the program's, and with
 * ".exe" after it -- as PowerShell may know the command on Windows -- unless
 * it ends so already.
 */
D_STATIC void
d_arg_internal_complete_pwsh_commands(
    struct d_arg_buffer* _out,
    struct d_pack_text   _program
)
{
    struct d_arg_buffer exe = D_ARG_BUFFER_INIT;

    (void)d_arg_buffer_append_cstr(_out,
                                   "    Register-ArgumentCompleter -Native "
                                   "-CommandName @(");
    d_arg_internal_complete_pwsh_text(_out,
                                      _program,
                                      false);

    // the name with ".exe"
    if ( (_program.length < 4u) ||
         (!d_arg_internal_equal_nocase_cstr(
              d_arg_internal_span(_program.data + _program.length - 4u,
                                  4u),
              ".exe")) )
    {
        (void)d_arg_buffer_append_text(&exe,
                                       _program);
        (void)d_arg_buffer_append_cstr(&exe,
                                       ".exe");
        (void)d_arg_buffer_append_char(_out,
                                       ',');
        d_arg_internal_complete_pwsh_text(_out,
                                          d_arg_buffer_text(&exe),
                                          false);
        _out->failed |= exe.failed;
    }

    (void)d_arg_buffer_append_cstr(_out,
                                   ") -ScriptBlock $complete\n");
    d_arg_buffer_release(&exe);

    return;
}

/*
d_arg_internal_complete_pwsh
  The header; the block: the data, the dictionaries and the completer, and
its registration.
*/
int
d_arg_internal_complete_pwsh(
    const struct d_arg_internal_model* _model,
    struct d_pack_text                 _program,
    struct d_arg_buffer*               _out
)
{
    (void)d_arg_buffer_append_cstr(_out,
                                   "# PowerShell completion for ");
    d_arg_internal_complete_pwsh_comment(_out,
                                         _program);
    (void)d_arg_buffer_append_cstr(
        _out,
        ", generated by djinterp's arg_parser.\n"
        "# Run it in PowerShell -- from the profile, to keep it.\n\n& {\n");
    d_arg_internal_complete_pwsh_data(_model,
                                      _out);
    (void)d_arg_internal_complete_lines(PWSH_LINES,
                                        PWSH_LINE_COUNT,
                                        d_arg_internal_span("",
                                                            0u),
                                        _out);
    d_arg_internal_complete_pwsh_commands(_out,
                                          _program);
    (void)d_arg_buffer_append_cstr(_out,
                                   "}\n");

    return (_out->failed != 0)
           ? D_ARG_ERROR_OUT_OF_MEMORY
           : D_ARG_OK;
}

#endif  // D_INTERNAL_ARG_PARSER_COMPLETION
