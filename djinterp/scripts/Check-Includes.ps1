<# Check-Includes.ps1

Validates all #include "..." directives in source files under a repo
by resolving them against the actual filesystem.

Checks:
  1. Relative to the including file's directory
  2. Relative to known include search roots (inc/c, src/c, tests/c)

Scans files under: inc/c, src/c, tests/c
File types:  .h, .hpp, .c, .cpp, and extensionless

Usage:
  .\Check-Includes.ps1 -RepoRoot "D:\sam\code\projects\frameworks\djinterp\djinterp"
  .\Check-Includes.ps1 -RepoRoot "..." -IncludeSearchRoots @("inc\c","src\c","tests\c")
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string] $RepoRoot,

    # Directories to scan for source files (relative to RepoRoot)
    [string[]] $ScanRoots = @("inc\c", "src\c", "tests\c"),

    # Additional include search paths (relative to RepoRoot)
    # These are tried when a relative-to-file resolution fails
    [string[]] $IncludeSearchRoots = @("inc\c", "src\c", "tests\c"),

    # Also report includes that resolved OK (verbose listing)
    [switch] $ShowResolved
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# ── Normalize RepoRoot ──────────────────────────────────────────────
$RepoRoot = (Resolve-Path $RepoRoot).Path

# ── Build the set of absolute include search directories ────────────
$searchDirs = @($RepoRoot)   # repo root itself is always searched
foreach ($rel in $IncludeSearchRoots) {
    $abs = Join-Path $RepoRoot $rel
    if (Test-Path $abs) {
        $searchDirs += (Resolve-Path $abs).Path
    }
}

# ── Collect source files to scan ────────────────────────────────────
$allowed = @(".h",".hpp",".c",".cpp")
$codeFiles = @()
foreach ($rel in $ScanRoots) {
    $abs = Join-Path $RepoRoot $rel
    if (-not (Test-Path $abs)) {
        Write-Warning "Scan root not found, skipping: $abs"
        continue
    }
    $codeFiles += Get-ChildItem -Path $abs -Recurse -File -ErrorAction SilentlyContinue |
        Where-Object {
            $ext = if ($_.Extension) { $_.Extension.ToLowerInvariant() } else { "" }
            ($allowed -contains $ext) -or [string]::IsNullOrEmpty($ext)
        }
}

$codeFiles = @($codeFiles | Sort-Object FullName -Unique)

Write-Host ("Scanning {0} source file(s) across {1} root(s)..." -f $codeFiles.Count, $ScanRoots.Count)
Write-Host ("Include search dirs: {0}" -f ($searchDirs -join "; "))
Write-Host ""

# ── Resolve a single include path ──────────────────────────────────
function Resolve-IncludePath {
    param(
        [string] $IncludePath,       # the string inside the quotes
        [string] $SourceFileDir      # directory of the file containing the #include
    )

    # Normalize to OS separators
    $normalized = $IncludePath.Replace("/", "\")

    # 1) Try relative to the source file's own directory
    $candidate = Join-Path $SourceFileDir $normalized
    try {
        $resolved = [System.IO.Path]::GetFullPath($candidate)
        if (Test-Path -LiteralPath $resolved -PathType Leaf) {
            return $resolved
        }
    } catch { }

    # 2) Try each include search directory
    foreach ($dir in $searchDirs) {
        $candidate = Join-Path $dir $normalized
        try {
            $resolved = [System.IO.Path]::GetFullPath($candidate)
            if (Test-Path -LiteralPath $resolved -PathType Leaf) {
                return $resolved
            }
        } catch { }
    }

    return $null
}

# ── Scan all files ──────────────────────────────────────────────────
$includePattern = '^\s*#\s*include\s*"(?<path>[^"]+)"'

[int] $totalIncludes  = 0
[int] $brokenIncludes = 0
[int] $okIncludes     = 0
[int] $filesWithErrors = 0

$errorsByFile = @{}

foreach ($f in $codeFiles) {
    $lines = [System.IO.File]::ReadAllLines($f.FullName)
    $fileDir = $f.DirectoryName
    $fileHasError = $false

    for ($i = 0; $i -lt $lines.Length; $i++) {
        $line = $lines[$i]
        $m = [regex]::Match($line, $includePattern)
        if (-not $m.Success) { continue }

        $totalIncludes++
        $incPath = $m.Groups["path"].Value
        $resolved = Resolve-IncludePath -IncludePath $incPath -SourceFileDir $fileDir

        if ($null -eq $resolved) {
            $brokenIncludes++
            $fileHasError = $true

            $relFile = $f.FullName.Substring($RepoRoot.Length + 1)
            $lineNum = $i + 1

            if (-not $errorsByFile.ContainsKey($relFile)) {
                $errorsByFile[$relFile] = @()
            }
            $errorsByFile[$relFile] += [pscustomobject]@{
                Line       = $lineNum
                Include    = $incPath
                SourceLine = $line.Trim()
            }
        }
        else {
            $okIncludes++
            if ($ShowResolved) {
                $relFile = $f.FullName.Substring($RepoRoot.Length + 1)
                $relResolved = $resolved.Substring($RepoRoot.Length + 1)
                Write-Host ("  OK  {0}:{1}  ""{2}"" -> {3}" -f $relFile, ($i+1), $incPath, $relResolved) -ForegroundColor DarkGray
            }
        }
    }

    if ($fileHasError) { $filesWithErrors++ }
}

# ── Report ──────────────────────────────────────────────────────────
if ($errorsByFile.Count -gt 0) {
    Write-Host "BROKEN INCLUDES:" -ForegroundColor Red
    Write-Host ("=" * 80) -ForegroundColor Red

    foreach ($file in ($errorsByFile.Keys | Sort-Object)) {
        Write-Host ""
        Write-Host "  $file" -ForegroundColor Cyan
        foreach ($err in $errorsByFile[$file]) {
            Write-Host ("    L{0,-5} #include ""{1}""" -f $err.Line, $err.Include) -ForegroundColor Yellow
        }
    }

    Write-Host ""
    Write-Host ("=" * 80) -ForegroundColor Red
}
else {
    Write-Host "All includes resolved successfully!" -ForegroundColor Green
}

Write-Host ""
Write-Host "Summary:"
Write-Host ("  Files scanned:        {0}" -f $codeFiles.Count)
Write-Host ("  Total #include `"...`": {0}" -f $totalIncludes)
Write-Host ("  Resolved OK:          {0}" -f $okIncludes) -ForegroundColor Green
Write-Host ("  BROKEN:               {0}" -f $brokenIncludes) -ForegroundColor $(if ($brokenIncludes -gt 0) { "Red" } else { "Green" })
Write-Host ("  Files with errors:    {0}" -f $filesWithErrors) -ForegroundColor $(if ($filesWithErrors -gt 0) { "Red" } else { "Green" })
