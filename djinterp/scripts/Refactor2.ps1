<# Refactor.ps1

Scope (as requested):
1) Project files:
   - ONLY under:  <RepoRoot>\.config\.msvs\testing\
   - Files: *.vcxproj and (optionally) *.vcxproj.filters
   - Changes:
     * inc/*   -> inc/c/*
     * src/*   -> src/c/*
     * tests/* -> tests/c/*
     * normalize "\" -> "/" in touched path strings

2) Source/header files:
   - ONLY under: <RepoRoot>\inc\c\, <RepoRoot>\src\c\, <RepoRoot>\tests\c\
   - Files: .h, .hpp, .c, .cpp, and NO extension
   - Changes:
     * ONLY on #include lines:
       - normalize "\" -> "/"
       - rewrite path segments "inc/", "src/", "tests/" -> "inc/c/", "src/c/", "tests/c/" (avoids double insert)

Usage:
  .\Refactor.ps1 -RepoRoot "D:\sam\code\projects\frameworks\djinterp\djinterp" -WhatIf
  .\Refactor.ps1 -RepoRoot "D:\sam\code\projects\frameworks\djinterp\djinterp" -Backup
#>

[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [Parameter(Mandatory)]
    [string] $RepoRoot,

    [switch] $IncludeFilters = $true,

    [switch] $UpdateVcxproj = $true,

    [switch] $UpdateIncludes = $true,

    [switch] $Backup
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Test-StartsWithBytes([byte[]] $bytes, [byte[]] $prefix) {
    if ($bytes.Length -lt $prefix.Length) { return $false }
    for ($i = 0; $i -lt $prefix.Length; $i++) {
        if ($bytes[$i] -ne $prefix[$i]) { return $false }
    }
    return $true
}

function Read-TextFilePreserveEncoding([string] $Path) {
    $bytes = [System.IO.File]::ReadAllBytes($Path)

    $offset   = 0
    $encoding = $null
    $preamble = @()

    if (Test-StartsWithBytes $bytes ([byte[]](0xEF,0xBB,0xBF))) {
        $encoding = New-Object System.Text.UTF8Encoding($true)
        $preamble = $encoding.GetPreamble()
        $offset   = 3
    }
    elseif (Test-StartsWithBytes $bytes ([byte[]](0xFF,0xFE,0x00,0x00))) {
        $encoding = New-Object System.Text.UTF32Encoding($false, $true)
        $preamble = $encoding.GetPreamble()
        $offset   = 4
    }
    elseif (Test-StartsWithBytes $bytes ([byte[]](0x00,0x00,0xFE,0xFF))) {
        $encoding = New-Object System.Text.UTF32Encoding($true, $true)
        $preamble = $encoding.GetPreamble()
        $offset   = 4
    }
    elseif (Test-StartsWithBytes $bytes ([byte[]](0xFF,0xFE))) {
        $encoding = New-Object System.Text.UnicodeEncoding($false, $true)
        $preamble = $encoding.GetPreamble()
        $offset   = 2
    }
    elseif (Test-StartsWithBytes $bytes ([byte[]](0xFE,0xFF))) {
        $encoding = New-Object System.Text.UnicodeEncoding($true, $true)
        $preamble = $encoding.GetPreamble()
        $offset   = 2
    }
    else {
        $encoding = New-Object System.Text.UTF8Encoding($false)
        $preamble = @()
        $offset   = 0
    }

    $text = $encoding.GetString($bytes, $offset, $bytes.Length - $offset)

    [pscustomobject]@{
        Text     = $text
        Encoding = $encoding
        Preamble = [byte[]]$preamble
    }
}

function Write-TextFilePreserveEncoding(
    [string] $Path,
    [string] $Text,
    [System.Text.Encoding] $Encoding,
    [byte[]] $Preamble
) {
    $body = $Encoding.GetBytes($Text)

    if ($Preamble -and $Preamble.Length -gt 0) {
        $out = New-Object byte[] ($Preamble.Length + $body.Length)
        [System.Buffer]::BlockCopy($Preamble, 0, $out, 0, $Preamble.Length)
        [System.Buffer]::BlockCopy($body, 0, $out, $Preamble.Length, $body.Length)
        [System.IO.File]::WriteAllBytes($Path, $out)
    }
    else {
        [System.IO.File]::WriteAllBytes($Path, $body)
    }
}

function Ensure-Backup([string] $Path) {
    if (-not $Backup) { return }
    $bak = $Path + ".bak"
    if (-not (Test-Path $bak)) {
        [System.IO.File]::Copy($Path, $bak)
    }
}

function Normalize-Slashes([string] $Text) {
    if ($null -eq $Text) { return $Text }
    return ($Text -replace '\\', '/')
}

function Add-CSubdir-Normalized([string] $Input, [string] $Dir) {
    # Input is assumed to have "/" separators only.
    # Inserts "/c/" after "inc|src|tests/" but avoids if already ".../dir/c/..."
    $boundary = '(^|[/" \t\)\(;])'

    $pattern1 = "(?i)$boundary$Dir/(?!c(?:/|;|$))"
    $repl1    = ('$1' + $Dir + '/c/')

    $out = [regex]::Replace($Input, $pattern1, $repl1)

    # Token "dir" in semicolon list: ...;dir;... or end
    $pattern2 = "(?i)$boundary$Dir(?=(;|$))"
    $repl2    = ('$1' + $Dir + '/c')

    $out = [regex]::Replace($out, $pattern2, $repl2)
    return $out
}

function Update-Paths([string] $Text) {
    $out = Normalize-Slashes $Text
    foreach ($d in @("inc","src","tests")) {
        $out = Add-CSubdir-Normalized -Input $out -Dir $d
    }
    return $out
}

function Update-VcxprojText([string] $Text) {
    $out = $Text

    # Include/Remove/Exclude attribute values
    $attrPattern = '(?<pre>\b(?:Include|Remove|Exclude)\s*=\s*")(?<val>[^"]+)(?<post>")'
    $out = [regex]::Replace($out, $attrPattern, {
        param($m)
        $val = $m.Groups["val"].Value
        $new = Update-Paths $val
        return $m.Groups["pre"].Value + $new + $m.Groups["post"].Value
    })

    # AdditionalIncludeDirectories + IncludePath contents
    foreach ($tag in @("AdditionalIncludeDirectories", "IncludePath")) {
        $tagPattern = "(?s)(?<pre><$tag>)(?<val>.*?)(?<post></$tag>)"
        $out = [regex]::Replace($out, $tagPattern, {
            param($m)
            $val = $m.Groups["val"].Value
            $new = Update-Paths $val
            return $m.Groups["pre"].Value + $new + $m.Groups["post"].Value
        })
    }

    return $out
}

function Rewrite-IncludePathInLine([string] $Line) {
    # Only touch #include lines, only inside "..." or <...>
    $pattern = '^(?<lead>\s*#\s*include\s*)(?<delim>[<"])(?<path>[^>"]+)(?<end>[>"])'
    $m = [regex]::Match($Line, $pattern)
    if (-not $m.Success) { return $Line }

    $p  = $m.Groups["path"].Value
    $p2 = Normalize-Slashes $p

    # inc/, src/, tests/ -> inc/c/, src/c/, tests/c/
    # (avoids double insert when /c/ already follows)
    $p3 = [regex]::Replace(
        $p2,
        '(?i)(^|/)(inc|src|tests)(/)(?!c(/|$))',
        '$1$2$3c$3'
    )

    # Files are now one level deeper (in /c/ subdir), so relative
    # paths that start with "../" need one more "../" to compensate.
    if ($p3 -match '^\.\.(/|$)') {
        $p3 = "../" + $p3
    }

    if ($p3 -eq $p) {
        if ($p2 -eq $p) { return $Line }
        $p3 = $p2
    }

    return $m.Groups["lead"].Value + $m.Groups["delim"].Value + $p3 + $m.Groups["end"].Value
}

function Update-IncludesInText([string] $Text) {
    $parts = [regex]::Split($Text, "(\r\n|\n|\r)")
    for ($i = 0; $i -lt $parts.Length; $i++) {
        if (($i % 2) -eq 0) {
            $parts[$i] = Rewrite-IncludePathInLine $parts[$i]
        }
    }
    return ($parts -join "")
}

# ----------------- Paths (fixed scope) -----------------

$ProjectRoot = Join-Path $RepoRoot ".config\.msvs\testing"
$CodeRoots   = @(
    (Join-Path $RepoRoot "inc\c"),
    (Join-Path $RepoRoot "src\c"),
    (Join-Path $RepoRoot "tests\c")
)

# ----------------- Project updates -----------------

$projFiles = @()
if ($UpdateVcxproj -and (Test-Path $ProjectRoot)) {
    $projPatterns = @("*.vcxproj")
    if ($IncludeFilters) { $projPatterns += "*.vcxproj.filters" }

    $projFiles =
        @(
            $projPatterns |
            ForEach-Object {
                Get-ChildItem -Path $ProjectRoot -Recurse -File -Filter $_ -ErrorAction SilentlyContinue
            }
        ) |
        Sort-Object FullName -Unique

    $projFiles = @($projFiles)  # IMPORTANT: never leave it as $null
}

[int] $projChanged = 0
foreach ($f in $projFiles) {
    $read = Read-TextFilePreserveEncoding $f.FullName
    $old  = $read.Text
    $new  = Update-VcxprojText $old

    if ($new -ne $old) {
        $projChanged++
        if ($PSCmdlet.ShouldProcess($f.FullName, "Update vcxproj paths (and normalize slashes)")) {
            Ensure-Backup $f.FullName
            Write-TextFilePreserveEncoding -Path $f.FullName -Text $new -Encoding $read.Encoding -Preamble $read.Preamble
            Write-Host "Updated project: $($f.FullName)"
        }
    }
}

# ----------------- Include updates -----------------

$codeFiles = @()
if ($UpdateIncludes) {
    foreach ($dir in $CodeRoots) {
        if (Test-Path $dir) {
            $codeFiles += Get-ChildItem -Path $dir -Recurse -File -ErrorAction SilentlyContinue
        }
    }

    $allowed = @(".h",".hpp",".c",".cpp")
    $codeFiles =
        @(
            $codeFiles |
            Where-Object {
                $ext = if ($_.Extension) { $_.Extension.ToLowerInvariant() } else { "" }
                ($allowed -contains $ext) -or [string]::IsNullOrEmpty($ext)
            } |
            Sort-Object FullName -Unique
        )
}

[int] $codeChanged = 0
foreach ($f in $codeFiles) {
    $read = Read-TextFilePreserveEncoding $f.FullName
    $old  = $read.Text
    $new  = Update-IncludesInText $old

    if ($new -ne $old) {
        $codeChanged++

        # Show per-line before/after for changed #include lines
        if ($WhatIfPreference -or (-not $PSCmdlet.ShouldProcess($f.FullName, "Rewrite #include paths (normalize slashes, inc|src|tests -> */c)"))) {
            $oldLines = [regex]::Split($old, "\r\n|\n|\r")
            $newLines = [regex]::Split($new, "\r\n|\n|\r")
            $count = [Math]::Max($oldLines.Length, $newLines.Length)
            $hasDiff = $false
            for ($i = 0; $i -lt $count; $i++) {
                $ol = if ($i -lt $oldLines.Length) { $oldLines[$i] } else { "" }
                $nl = if ($i -lt $newLines.Length) { $newLines[$i] } else { "" }
                if ($ol -ne $nl) {
                    if (-not $hasDiff) {
                        Write-Host "  $($f.FullName)" -ForegroundColor Cyan
                        $hasDiff = $true
                    }
                    $lineNum = $i + 1
                    Write-Host ("    L{0}  - {1}" -f $lineNum, $ol) -ForegroundColor Red
                    Write-Host ("    L{0}  + {1}" -f $lineNum, $nl) -ForegroundColor Green
                }
            }
        }
        else {
            Ensure-Backup $f.FullName
            Write-TextFilePreserveEncoding -Path $f.FullName -Text $new -Encoding $read.Encoding -Preamble $read.Preamble
            Write-Host "Updated includes: $($f.FullName)"
        }
    }
}

Write-Host ""
Write-Host "Done."
Write-Host ("  Project root: {0}" -f $ProjectRoot)
Write-Host ("  Project files changed: {0} / {1}" -f $projChanged, $projFiles.Count)
Write-Host ("  Include files changed: {0} / {1}" -f $codeChanged,  $codeFiles.Count)
Write-Host "Tip: run with -WhatIf first; add -Backup to write .bak files."