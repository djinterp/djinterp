<#
.SYNOPSIS
  Front-end wrapper for djinterp macro generators.

.DESCRIPTION
  Modes:
    --for_each            -> generator_macro_tuple.py
    --varg-count          -> generator_macro_varg_count.py
    --varg-get-arg        -> generator_macro_varg_get_arg.py
    --varg-has-args       -> generator_macro_varg_has_args.py

  Meta mode:
    --generate-all-modules
      Generates BOTH non-MSVC (interval-bucketed) and MSVC-capped outputs.
      Produces multiple headers under the chosen output directory.

  Interval notation:
    You may pass intervals as:
      [start,end,step]   inclusive
      (start,end,step)   exclusive (this front-end adjusts endpoints)
    or mixed like (start,end,step] etc.

    The Python generators expect the normalized, inclusive form:
      (start,end,step)

  Diagnostics:
    --debug               Prints the exact command line before each Python call.

.NOTES
  - This script intentionally avoids List[string].AddRange(...) coercion issues
    by building plain PowerShell string arrays.
  - In meta mode, MSVC outputs are always generated as separate files:
      * core (non-tuple) header
      * one tuple header per arity
      * varg count, varg get_arg, and varg has_args headers
      * inc header
#>

[CmdletBinding(PositionalBinding = $false)]
param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]] $Args
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Get-PythonCommand {
    $py = Get-Command python -ErrorAction SilentlyContinue
    if ($py) {
        return @{ Exe = "python"; Extra = @() }
    }

    $pyLauncher = Get-Command py -ErrorAction SilentlyContinue
    if ($pyLauncher) {
        # Force python3 if available via launcher
        return @{ Exe = "py"; Extra = @("-3") }
    }

    throw "Python was not found on PATH (tried: python, py)."
}

function Convert-IntervalNotation {
    param([string]$interval)

    # Already normalized "(a,b,step)"
    if ($interval -match '^\([^,]+,[^,]+,[^,]+\)$') {
        return $interval
    }

    # Parse bracket/paren interval: [start,end,step] or (start,end,step) or mixed
    $full = [regex]::Match($interval, '^([\[\(])([^,]+),([^,]+),([^,]+)([\]\)])$')
    if (-not $full.Success) {
        # Support shorthand [start,end] (assumes step=1)
        $short = [regex]::Match($interval, '^([\[\(])([^,]+),([^,]+)([\]\)])$')
        if ($short.Success) {
            return Convert-IntervalNotation ("{0}{1},{2},1{3}" -f $short.Groups[1].Value, $short.Groups[2].Value, $short.Groups[3].Value, $short.Groups[4].Value)
        }
        return $interval
    }

    $open  = $full.Groups[1].Value
    $start = $full.Groups[2].Value.Trim()
    $end   = $full.Groups[3].Value.Trim()
    $step  = $full.Groups[4].Value.Trim()
    $close = $full.Groups[5].Value

    $adjStart = $start
    $adjEnd   = $end

    if ($open -eq '(') {
        if ($step -match '^-?\d+$' -and $start -match '^-?\d+$') {
            $adjStart = ([int]$start + [int]$step).ToString()
        }
    }

    if ($close -eq ')') {
        if ($step -match '^-?\d+$' -and $end -match '^-?\d+$') {
            $adjEnd = ([int]$end - [int]$step).ToString()
        }
    }

    return "($adjStart,$adjEnd,$step)"
}

function Has-Flag {
    param([string[]]$tokens, [string]$flag)
    return ($tokens -contains $flag)
}

function Get-FlagValue {
    param(
        [string[]]$tokens,
        [string]$flag,
        $defaultValue = $null          # NOTE: intentionally untyped so $null is not coerced to ""
    )

    for ($i = 0; $i -lt $tokens.Length; $i++) {
        if ($tokens[$i] -eq $flag -and ($i + 1) -lt $tokens.Length) {
            $next = $tokens[$i + 1]
            if ($next -and -not $next.StartsWith('-')) {
                return $next
            }
        }
    }

    return $defaultValue
}

function Get-MultiValuesAfterFlag {
    param(
        [string[]]$tokens,
        [string]$flag
    )

    $values = @()
    for ($i = 0; $i -lt $tokens.Length; $i++) {
        if ($tokens[$i] -eq $flag) {
            $j = $i + 1
            while ($j -lt $tokens.Length) {
                $t = $tokens[$j]
                if ($t.StartsWith('-') -and -not ($t -match '^[\[\(]')) {
                    break
                }
                $values += $t
                $j++
            }
            break
        }
    }
    return ,$values
}

function Parse-IntervalMax {
    param([string]$normalizedInterval)

    if ($normalizedInterval -match '^\(([^,]+),([^,]+),([^,]+)\)$') {
        $end = $Matches[2]
        if ($end -match '^-?\d+$') {
            return [int]$end
        }
    }

    return $null
}

function Get-IntervalMaxima {
    param([string[]]$normalizedIntervals)

    $maxima = @()
    foreach ($iv in $normalizedIntervals) {
        $m = Parse-IntervalMax $iv
        if ($null -ne $m) {
            $maxima += $m
        }
    }

    # Unique + sort
    $maxima = $maxima | Sort-Object -Unique
    return ,$maxima
}

function Ensure-Dir {
    param([string]$path)
    if (-not (Test-Path $path)) {
        New-Item -ItemType Directory -Force -Path $path | Out-Null
    }
}

function Resolve-OutRoot {
    param(
        [string]$outRoot,
        [string]$baseDir
    )

    if ([string]::IsNullOrWhiteSpace($outRoot)) {
        $outRoot = ".\\gen"
    }

    # If user passes a relative path, resolve it relative to the script directory
    # (NOT the process working directory, which can be e.g. System32).
    if (-not [System.IO.Path]::IsPathRooted($outRoot)) {
        $outRoot = Join-Path $baseDir $outRoot
    }

    return [System.IO.Path]::GetFullPath($outRoot)
}

function Write-TextUtf8NoBom {
    param([string]$path, [string]$text)
    $enc = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($path, $text, $enc)
}

function Get-IncludeGuardFromFileName {
    param([string]$fileName)

    # e.g. "for_each256.h" -> "DJINTERP_FOR_EACH256_H"
    $base = [System.IO.Path]::GetFileName($fileName)
    $base = $base.ToUpperInvariant()
    $base = ($base -replace '[^A-Z0-9]+', '_')
    $base = $base.Trim('_')

    return ("DJINTERP_{0}" -f $base)
}

function Get-RelativeIncludePath {
    param(
        [string]$fromFilePath,
        [string]$toFilePath
    )

    $fromDir = [System.IO.Path]::GetDirectoryName([System.IO.Path]::GetFullPath($fromFilePath))
    if (-not $fromDir.EndsWith([System.IO.Path]::DirectorySeparatorChar)) {
        $fromDir += [System.IO.Path]::DirectorySeparatorChar
    }

    $fromUri = New-Object System.Uri($fromDir)
    $toUri   = New-Object System.Uri([System.IO.Path]::GetFullPath($toFilePath))

    $rel = $fromUri.MakeRelativeUri($toUri).ToString()
    $rel = [System.Uri]::UnescapeDataString($rel)

    # C/C++ includes are happy with forward slashes.
    return ($rel -replace '\\', '/')
}

function Write-AliasHeader {
    param(
        [string]$outRoot,
        [string]$aliasRelPath,
        [string]$targetRelPath
    )

    $aliasAbs  = Join-Path $outRoot $aliasRelPath
    $targetAbs = Join-Path $outRoot $targetRelPath

    if (-not (Test-Path $targetAbs)) {
        Write-Warning ("Alias target missing, skipping: {0}" -f $targetRelPath)
        return
    }

    $aliasDir = Split-Path -Parent $aliasAbs
    if ($aliasDir) {
        Ensure-Dir $aliasDir
    }

    $guard   = Get-IncludeGuardFromFileName $aliasRelPath
    $include = Get-RelativeIncludePath -fromFilePath $aliasAbs -toFilePath $targetAbs

    $txt = @"
#ifndef $guard
#define $guard
#include "$include"
#endif
"@

    Write-TextUtf8NoBom $aliasAbs $txt
}

function Invoke-Py {
    param(
        [hashtable]$py,
        [string]$scriptPath,
        [string[]]$pyArgs
    )

    if (-not (Test-Path $scriptPath)) {
        throw "Python script not found: $scriptPath"
    }

    # Show command when --debug is active
    if ($script:DebugRun) {
        Write-Host "  [cmd] $($py.Exe) $scriptPath $($pyArgs -join ' ')" -ForegroundColor DarkGray
    }

    # Temporarily relax ErrorActionPreference so that stderr output from the
    # Python process (e.g. argparse usage lines, warnings) does not
    # immediately become a terminating error.  We check LASTEXITCODE instead.
    $saved = $ErrorActionPreference
    try {
        $ErrorActionPreference = "Continue"
        & $py.Exe @($py.Extra) $scriptPath @pyArgs
    } finally {
        $ErrorActionPreference = $saved
    }

    if ($LASTEXITCODE -ne 0) {
        throw "Python script failed (exit $LASTEXITCODE): $scriptPath"
    }
}

function Extract-TupleArgs {
    param(
        [string[]]$tokens,
        [switch]$DefaultRange
    )

    # If user provided any tuple selectors, forward them.
    $tupleArgs = @()

    $tupleRange = Get-FlagValue $tokens "--tuple-range" $null
    if (-not [string]::IsNullOrEmpty($tupleRange)) {
        $tupleArgs += @("--tuple-range", $tupleRange)
    }

    $tupleMin = Get-FlagValue $tokens "--tuple-min" $null
    if (-not [string]::IsNullOrEmpty($tupleMin)) {
        $tupleArgs += @("--tuple-min", $tupleMin)
    }

    $tupleMax = Get-FlagValue $tokens "--tuple-max" $null
    if (-not [string]::IsNullOrEmpty($tupleMax)) {
        $tupleArgs += @("--tuple-max", $tupleMax)
    }

    $tupleNaming = Get-FlagValue $tokens "--tuple-naming-convention" $null
    if (-not [string]::IsNullOrEmpty($tupleNaming)) {
        $tupleArgs += @("--tuple-naming-convention", $tupleNaming)
    }

    $tupleArities = Get-MultiValuesAfterFlag $tokens "--tuple-arities"
    if ($tupleArities.Count -gt 0) {
        $tupleArgs += "--tuple-arities"
        $tupleArgs += $tupleArities
    }

    # Default tuple range if requested and user provided none
    if ($DefaultRange -and $tupleArgs.Count -eq 0) {
        $tupleArgs += @("--tuple-range", "1-16")
    }

    return ,$tupleArgs
}

# --------------------------------------------------------------------------------------
# DEFAULT ALIAS NAMES (meta mode)
#   Use --alias / --alias-pair (and MSVC variants) to override these.
# --------------------------------------------------------------------------------------
$script:DefaultAliasForEach          = "for_each#.h"
$script:DefaultAliasForEachPair      = "for_each_pair#.h"
$script:DefaultAliasForEachMSVC      = "for_each_msvc.h"
$script:DefaultAliasForEachPairMSVC  = "for_each_pair_msvc.h"

# --------------------------------------------------------------------------------------
# META MODE: --generate-all-modules
# --------------------------------------------------------------------------------------

if ((Has-Flag $Args "--generate-all-modules") -or (Has-Flag $Args "--generate-all")) {

    $script:DebugRun = (Has-Flag $Args "--debug")

    $scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

    $genForEach      = Join-Path $scriptDir "generator_macro_tuple.py"
    $genVargCount    = Join-Path $scriptDir "generator_macro_varg_count.py"
    $genVargGetArg   = Join-Path $scriptDir "generator_macro_varg_get_arg.py"
    $genVargHasArgs  = Join-Path $scriptDir "generator_macro_varg_has_args.py"
    $genInc          = Join-Path $scriptDir "macro_inc.py"

    $py = Get-PythonCommand

    # Out root
    $outRoot = Get-FlagValue $Args "--outdir" $null
    $outRoot = Resolve-OutRoot -outRoot $outRoot -baseDir $scriptDir

    # Intervals
    $intervalsRaw = Get-MultiValuesAfterFlag $Args "--intervals"
    if ($intervalsRaw.Count -eq 0) {
        $intervalsRaw = @(
            "[0,64,1]",
            "[65,128,1]",
            "[129,256,1]",
            "[257,512,1]",
            "[513,1024,1]"
        )
    }

    $intervals = @()
    foreach ($iv in $intervalsRaw) {
        $intervals += (Convert-IntervalNotation $iv)
    }

    $maxima = Get-IntervalMaxima $intervals

    # Alias headers
    $aliasEnabled         = -not (Has-Flag $Args "--no-alias")
    $aliasForEach         = Get-FlagValue $Args "--alias"           $script:DefaultAliasForEach
    $aliasForEachPair     = Get-FlagValue $Args "--alias-pair"      $script:DefaultAliasForEachPair
    $aliasForEachMSVC     = Get-FlagValue $Args "--alias-msvc"      $script:DefaultAliasForEachMSVC
    $aliasForEachPairMSVC = Get-FlagValue $Args "--alias-pair-msvc" $script:DefaultAliasForEachPairMSVC

    # Forward common switches
    $includeZero = @()
    if (Has-Flag $Args "--include-zero-arg") {
        $includeZero = @("--include-zero-arg")
    }

    $noWrappers = @("--no-wrappers")   # always suppress wrappers (one family per file)

    # Tuple selection args
    $tupleArgs = Extract-TupleArgs $Args -DefaultRange

    # Everything goes flat into $outRoot
    Ensure-Dir $outRoot

    # ==================================================================
    #  NON-MSVC: core families (one macro family per file)
    # ==================================================================
    $coreFamilies = @(
        @{ Family = "for_each";           Name = "for_each#.h" },
        @{ Family = "separator";          Name = "for_each_separator#.h" },
        @{ Family = "pair";               Name = "for_each_pair#.h" },
        @{ Family = "pair_separator";     Name = "for_each_pair_separator#.h" },
        @{ Family = "triple";             Name = "for_each_triple#.h" },
        @{ Family = "triple_separator";   Name = "for_each_triple_separator#.h" },
        @{ Family = "data_separator";     Name = "for_each_data_separator#.h" }
    )

    Write-Host "[all-modules] Non-MSVC: core families..." -ForegroundColor Green
    foreach ($fam in $coreFamilies) {
        Invoke-Py $py $genForEach (@(
            "--only", $fam.Family,
            "--no-tuples", "--no-wrappers",
            "--name", $fam.Name,
            "--outdir", $outRoot,
            "--intervals"
        ) + $intervals + @("--substitute", "max") + $includeZero)
    }

    # ==================================================================
    #  NON-MSVC: for_each_comma (core)
    # ==================================================================
    Write-Host "[all-modules] Non-MSVC: for_each_comma..." -ForegroundColor Green
    Invoke-Py $py $genForEach (@(
        "--only", "hardcoded", "--msvc-comma",
        "--no-tuples", "--no-wrappers",
        "--name", "for_each_comma#.h",
        "--outdir", $outRoot,
        "--intervals"
    ) + $intervals + @("--substitute", "max") + $includeZero)

    # ==================================================================
    #  NON-MSVC: tuple separator files
    # ==================================================================
    Write-Host "[all-modules] Non-MSVC: tuple separator files..." -ForegroundColor Green
    Invoke-Py $py $genForEach (@(
        "--pure-tuples", "--tuple-per-file",
        "--name", "for_each_%_tuple_sep#.h",
        "--outdir", $outRoot,
        "--intervals"
    ) + $intervals + @("--substitute", "max") + $tupleArgs + $includeZero + $noWrappers)

    # ==================================================================
    #  NON-MSVC: tuple comma files
    # ==================================================================
    Write-Host "[all-modules] Non-MSVC: tuple comma files..." -ForegroundColor Green
    Invoke-Py $py $genForEach (@(
        "--pure-tuples", "--tuple-per-file", "--msvc-comma",
        "--tuple-naming-convention", "D_INTERNAL_FOR_EACH_%_TUPLE_COMMA_#",
        "--name", "for_each_%_tuple_comma#.h",
        "--outdir", $outRoot,
        "--intervals"
    ) + $intervals + @("--substitute", "max") + $tupleArgs + $includeZero + $noWrappers)

    # ==================================================================
    #  NON-MSVC: varg utils (per max)
    # ==================================================================
    Write-Host "[all-modules] Non-MSVC: varg utils..." -ForegroundColor Green
    foreach ($m in $maxima) {
        Invoke-Py $py $genVargCount   @($m.ToString(), (Join-Path $outRoot ("varg_count{0}.h" -f $m)))
        Invoke-Py $py $genVargGetArg  @("1", $m.ToString(), (Join-Path $outRoot ("varg_get_arg{0}.h" -f $m)))
        Invoke-Py $py $genVargHasArgs @($m.ToString(), (Join-Path $outRoot ("varg_has_args{0}.h" -f $m)))
    }

    # ==================================================================
    #  NON-MSVC: inc tables (per max)
    # ==================================================================
    Write-Host "[all-modules] Non-MSVC: inc tables..." -ForegroundColor Green
    foreach ($m in $maxima) {
        Invoke-Py $py $genInc @("0", $m.ToString(), (Join-Path $outRoot ("inc{0}.h" -f $m)))
    }

    # ==================================================================
    #  MSVC: core families (one macro family per file)
    # ==================================================================
    $msvcCoreFamilies = @(
        @{ Family = "for_each";           Name = "for_each_mvc.h" },
        @{ Family = "separator";          Name = "for_each_separator_mvc.h" },
        @{ Family = "pair";               Name = "for_each_pair_mvc.h" },
        @{ Family = "pair_separator";     Name = "for_each_pair_separator_mvc.h" },
        @{ Family = "triple";             Name = "for_each_triple_mvc.h" },
        @{ Family = "triple_separator";   Name = "for_each_triple_separator_mvc.h" },
        @{ Family = "data_separator";     Name = "for_each_data_separator_mvc.h" }
    )

    Write-Host "[all-modules] MSVC: core families..." -ForegroundColor Green
    foreach ($fam in $msvcCoreFamilies) {
        Invoke-Py $py $genForEach (@(
            "--msvc", "--max", "127",
            "--only", $fam.Family,
            "--no-tuples", "--no-wrappers",
            "--name", $fam.Name,
            "--outdir", $outRoot
        ) + $includeZero)
    }

    # ==================================================================
    #  MSVC: for_each_comma
    # ==================================================================
    Write-Host "[all-modules] MSVC: for_each_comma..." -ForegroundColor Green
    Invoke-Py $py $genForEach (@(
        "--msvc", "--max", "127",
        "--only", "hardcoded", "--msvc-comma",
        "--no-tuples", "--no-wrappers",
        "--name", "for_each_comma_mvc.h",
        "--outdir", $outRoot
    ) + $includeZero)

    # ==================================================================
    #  MSVC: tuple separator + comma files
    # ==================================================================
    Write-Host "[all-modules] MSVC: tuple separator files..." -ForegroundColor Green
    Invoke-Py $py $genForEach (@(
        "--msvc", "--max", "127",
        "--tuples-only", "--tuple-per-file",
        "--name", "for_each_%_tuple_sep_mvc.h",
        "--outdir", $outRoot
    ) + $tupleArgs + $includeZero + $noWrappers)

    Write-Host "[all-modules] MSVC: tuple comma files..." -ForegroundColor Green
    Invoke-Py $py $genForEach (@(
        "--msvc", "--max", "127",
        "--pure-tuples", "--tuple-per-file", "--msvc-comma",
        "--tuple-naming-convention", "D_INTERNAL_FOR_EACH_%_TUPLE_COMMA_#",
        "--name", "for_each_%_tuple_comma_mvc.h",
        "--outdir", $outRoot
    ) + $tupleArgs + $includeZero + $noWrappers)

    # ==================================================================
    #  MSVC: varg utils + inc
    # ==================================================================
    Write-Host "[all-modules] MSVC: varg utils + inc..." -ForegroundColor Green
    $msvcMax = "126"
    Invoke-Py $py $genVargCount   @($msvcMax, (Join-Path $outRoot "varg_count_mvc.h"))
    Invoke-Py $py $genVargGetArg  @("1", $msvcMax, (Join-Path $outRoot "varg_get_arg_mvc.h"))
    Invoke-Py $py $genVargHasArgs @($msvcMax, (Join-Path $outRoot "varg_has_args_mvc.h"), "--mvc")
    Invoke-Py $py $genInc         @("0", $msvcMax, (Join-Path $outRoot "inc_mvc.h"))

    # ==================================================================
    #  Aliases
    # ==================================================================
    if ($aliasEnabled) {
        Write-Host "[all-modules] Aliases..." -ForegroundColor Green
        foreach ($m in $maxima) {
            Write-AliasHeader -outRoot $outRoot -aliasRelPath ($aliasForEach.Replace("#", $m.ToString()))         -targetRelPath ("for_each_1_tuple_sep{0}.h" -f $m)
            Write-AliasHeader -outRoot $outRoot -aliasRelPath ($aliasForEachPair.Replace("#", $m.ToString()))     -targetRelPath ("for_each_2_tuple_sep{0}.h" -f $m)
        }
        Write-AliasHeader -outRoot $outRoot -aliasRelPath $aliasForEachMSVC     -targetRelPath "for_each_1_tuple_sep_mvc.h"
        Write-AliasHeader -outRoot $outRoot -aliasRelPath $aliasForEachPairMSVC -targetRelPath "for_each_2_tuple_sep_mvc.h"
    }

    Write-Host "[all-modules] Done. Output: $outRoot" -ForegroundColor Green
    exit 0
}

# --------------------------------------------------------------------------------------
# NORMAL MODE: forward to one generator
# --------------------------------------------------------------------------------------

$script:DebugRun = (Has-Flag $Args "--debug")

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

# Mode selection
$mode = $null
$forward = @()

for ($i = 0; $i -lt $Args.Length; $i++) {
    $a = $Args[$i]

    if ($a -eq '--for_each' -or $a -eq '--for-each') {
        $mode = 'for_each'
        continue
    }

    if ($a -eq '--varg_utils' -or $a -eq '--varg-utils') {
        throw ("--varg_utils has been split into three individual modes:`n" +
               "  --varg-count     -> generator_macro_varg_count.py`n" +
               "  --varg-get-arg   -> generator_macro_varg_get_arg.py`n" +
               "  --varg-has-args  -> generator_macro_varg_has_args.py`n" +
               "Or use --generate-all-modules to run everything.")
    }

    if ($a -eq '--varg-count' -or $a -eq '--varg_count') {
        $mode = 'varg_count'
        continue
    }

    if ($a -eq '--varg-get-arg' -or $a -eq '--varg_get_arg') {
        $mode = 'varg_get_arg'
        continue
    }

    if ($a -eq '--varg-has-args' -or $a -eq '--varg_has_args') {
        $mode = 'varg_has_args'
        continue
    }

    # ignore meta flag if user passes it accidentally without wanting meta mode
    if ($a -eq '--generate-all-modules' -or $a -eq '--generate-all') {
        continue
    }

    # strip --debug before forwarding
    if ($a -eq '--debug') {
        continue
    }

    # strip meta-only alias flags before forwarding
    if ($a -eq '--no-alias') {
        continue
    }

    # strip meta-only layout flags before forwarding
    if ($a -eq '--flat-outdir' -or $a -eq '--flat') {
        continue
    }

    if ($a -eq '--alias' -or
        $a -eq '--alias-pair' -or
        $a -eq '--alias-msvc' -or
        $a -eq '--alias-pair-msvc')
    {
        if (($i + 1) -lt $Args.Length) {
            $next = $Args[$i + 1]
            if ($next -and -not $next.StartsWith('-')) {
                $i++ # skip value token
            }
        }
        continue
    }

    # Normalize --intervals list
    if ($a -eq '--intervals') {
        $forward += $a
        $i++
        while ($i -lt $Args.Length) {
            $t = $Args[$i]
            if ($t.StartsWith('-') -and -not ($t -match '^[\[\(]')) {
                $i--
                break
            }
            $forward += (Convert-IntervalNotation $t)
            $i++
        }
        continue
    }

    $forward += $a
}

if (-not $mode) {
    throw "Missing mode. Use --for_each, --varg-count, --varg-get-arg, --varg-has-args (or --generate-all-modules)."
}

switch ($mode) {
    'for_each'       { $generator = Join-Path $scriptDir 'generator_macro_tuple.py' }
    'varg_count'     { $generator = Join-Path $scriptDir 'generator_macro_varg_count.py' }
    'varg_get_arg'   { $generator = Join-Path $scriptDir 'generator_macro_varg_get_arg.py' }
    'varg_has_args'  { $generator = Join-Path $scriptDir 'generator_macro_varg_has_args.py' }
}

$py = Get-PythonCommand
Invoke-Py $py $generator $forward