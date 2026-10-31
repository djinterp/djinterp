<#
.SYNOPSIS
    Move files listed one per line, preserving paths relative to a common root.
.DESCRIPTION
    Accepts absolute paths and paths relative to -Root. By default, hidden files
    and files in hidden directories are skipped. Existing destination files are
    never overwritten. -VerifyAllFirst performs a strict preflight: any detected
    problem aborts the whole run without moving files. Preflight cannot guarantee
    that files will remain accessible when the actual moves occur.
.EXAMPLE
    .\Move-ListedFiles.ps1 -List 'C:\Users\Sam\Documents\retire.txt' `
        -Root 'C:\some\root\path' -Destination 'C:\foo\bar\example' `
        -Extensions .cpp,.h -IncludeHidden -DryRun -VerifyAllFirst -Verbose
.EXAMPLE
    .\Move-ListedFiles.ps1 -List .\retire.txt -Root C:\source `
        -Destination D:\archive -OnMissing Continue -OnError Continue
.NOTES
    Exit code 0: all selected files processed, or none selected.
    Exit code 2: at least one missing file, invalid path or move failure.
    Compatible with Windows PowerShell 5.1 and PowerShell 7 on Windows.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [Alias('FileList')]
    [string]$List,

    [Parameter(Mandatory = $true)]
    [string]$Root,

    [Parameter(Mandatory = $true)]
    [string]$Destination,

    [string[]]$Extensions = @(),
    [switch]$IncludeHidden,
    [switch]$DryRun,
    [switch]$VerifyAllFirst,

    [ValidateSet('Continue', 'Stop')]
    [string]$OnMissing = 'Stop',

    [ValidateSet('Continue', 'Stop')]
    [string]$OnError = 'Stop'
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

function Get-NormalizedDirectory {
    param([string]$Path)
    $full = [IO.Path]::GetFullPath($Path)
    $trimmed = $full.TrimEnd([char[]]@('\', '/'))
    if ($trimmed -match '^[A-Za-z]:$') { $trimmed += '\' }
    return $trimmed
}

try {
    $rootPath = Get-NormalizedDirectory -Path $Root
    $destinationPath = Get-NormalizedDirectory -Path $Destination
    $rootInfo = Get-Item -LiteralPath $rootPath -Force
    if (-not $rootInfo.PSIsContainer) { throw "Root is not a directory: $rootPath" }
    if (-not (Test-Path -LiteralPath $List -PathType Leaf)) {
        throw "List file does not exist: $List"
    }
    if (Test-Path -LiteralPath $destinationPath) {
        $destinationInfo = Get-Item -LiteralPath $destinationPath -Force
        if (-not $destinationInfo.PSIsContainer) {
            throw "Destination exists but is not a directory: $destinationPath"
        }
    }
    $lines = @(Get-Content -LiteralPath $List)
}
catch {
    Write-Warning "Setup failed: $($_.Exception.Message)"
    exit 2
}

$rootPrefix = $rootPath.TrimEnd([char[]]@('\', '/')) + '\'
$comparison = [StringComparison]::OrdinalIgnoreCase

# An empty extension set means there is no extension filter.
$extensionSet = @{}
foreach ($argument in $Extensions) {
    foreach ($part in ($argument -split ',')) {
        $ext = $part.Trim()
        if (-not $ext) { continue }
        if (-not $ext.StartsWith('.')) { $ext = '.' + $ext }
        if ($ext -notmatch '^\.[^\\/*?:"<>|.]+$') {
            Write-Warning "Invalid extension filter: $part"
            exit 2
        }
        $extensionSet[$ext] = $true
    }
}

# Build a plan without modifying the filesystem. Parsing problems are recorded
# as entries so that -VerifyAllFirst can report all of them together.
$entries = New-Object 'System.Collections.Generic.List[object]'
$seen = @{}
$lineNumber = 0
foreach ($line in $lines) {
    $lineNumber++
    $inputPath = $line.Trim()
    if (-not $inputPath) { continue }
    if ($inputPath.Length -ge 2 -and $inputPath[0] -eq '"' -and
        $inputPath[$inputPath.Length - 1] -eq '"') {
        $inputPath = $inputPath.Substring(1, $inputPath.Length - 2)
    }

    try {
        if ([IO.Path]::IsPathRooted($inputPath)) {
            $source = [IO.Path]::GetFullPath($inputPath)
        }
        else {
            $source = [IO.Path]::GetFullPath((Join-Path -Path $rootPath -ChildPath $inputPath))
        }
        if (-not $source.StartsWith($rootPrefix, $comparison)) {
            throw "File is outside Root ($rootPath): $source"
        }
        if ($extensionSet.Count -gt 0 -and
            -not $extensionSet.ContainsKey([IO.Path]::GetExtension($source))) {
            Write-Verbose "Line $lineNumber skipped by extension filter: $source"
            continue
        }
        if ($seen.ContainsKey($source)) {
            Write-Verbose "Line $lineNumber is a duplicate; skipping: $source"
            continue
        }
        $seen[$source] = $true
        $relativePath = $source.Substring($rootPrefix.Length)
        $target = Join-Path -Path $destinationPath -ChildPath $relativePath
        $entries.Add([pscustomobject]@{
            Line = $lineNumber; Source = $source; Target = $target
            ParseError = $null
        })
    }
    catch {
        $entries.Add([pscustomobject]@{
            Line = $lineNumber; Source = $inputPath; Target = $null
            ParseError = $_.Exception.Message
        })
    }
}

function Test-Candidate {
    param($Entry)

    if ($Entry.ParseError) {
        return [pscustomobject]@{ Status = 'Error'; Detail = $Entry.ParseError }
    }
    $source = $Entry.Source
    $target = $Entry.Target

    try {
        if (-not (Test-Path -LiteralPath $source)) {
            return [pscustomobject]@{ Status = 'Missing'; Detail = "File not found: $source" }
        }
        $sourceInfo = Get-Item -LiteralPath $source -Force
        if ($sourceInfo.PSIsContainer) {
            return [pscustomobject]@{ Status = 'Error'; Detail = "Source is a directory, not a file: $source" }
        }

        if (-not $IncludeHidden) {
            # Check both the file and each parent directory, including Root.
            $current = $source
            while ($true) {
                $info = Get-Item -LiteralPath $current -Force
                if (($info.Attributes -band [IO.FileAttributes]::Hidden) -ne 0) {
                    return [pscustomobject]@{
                        Status = 'Hidden'; Detail = "Hidden file or directory: $source"
                    }
                }
                if ($current.Equals($rootPath, $comparison)) { break }
                $parent = [IO.Path]::GetDirectoryName($current)
                if ([string]::IsNullOrEmpty($parent)) { break }
                $current = $parent
            }
        }

        if (Test-Path -LiteralPath $target) {
            return [pscustomobject]@{
                Status = 'Error'; Detail = "Destination already exists (no overwrite): $target"
            }
        }

        # Check for file-in-the-way conflicts in the destination's ancestors.
        $dir = [IO.Path]::GetDirectoryName($target)
        while (-not [IO.Directory]::Exists($dir)) {
            if ([IO.File]::Exists($dir)) {
                return [pscustomobject]@{
                    Status = 'Error'; Detail = "Destination parent is a file: $dir"
                }
            }
            $parent = [IO.Path]::GetDirectoryName($dir)
            if ([string]::IsNullOrEmpty($parent) -or $parent -eq $dir) { break }
            $dir = $parent
        }
    }
    catch {
        return [pscustomobject]@{
            Status = 'Error'; Detail = "Cannot inspect $source : $($_.Exception.Message)"
        }
    }
    return [pscustomobject]@{ Status = 'Ready'; Detail = '' }
}

function Report-Problem {
    param($Entry, $Check)
    Write-Warning ("Line {0}: {1}" -f $Entry.Line, $Check.Detail)
}

$moved = 0
$wouldMove = 0
$hiddenSkipped = 0
$failures = 0

if ($VerifyAllFirst) {
    Write-Host "Preflight: checking $($entries.Count) selected, unique list entries..."
    $ready = New-Object 'System.Collections.Generic.List[object]'
    foreach ($entry in $entries) {
        $check = Test-Candidate -Entry $entry
        switch ($check.Status) {
            'Ready'   { $ready.Add($entry); break }
            'Hidden'  {
                $hiddenSkipped++
                Write-Verbose "Line $($entry.Line) skipped: $($check.Detail)"
                break
            }
            default   {
                $failures++
                Report-Problem -Entry $entry -Check $check
                break
            }
        }
    }
    if ($failures -gt 0) {
        Write-Warning "Preflight failed: $failures problem(s). No files moved."
        exit 2
    }
    Write-Host "Preflight passed: $($ready.Count) file(s) ready; $hiddenSkipped hidden skipped."
    $entries = $ready
}

foreach ($entry in $entries) {
    # Recheck even after preflight: file state may have changed in the meantime.
    $check = Test-Candidate -Entry $entry
    if ($check.Status -eq 'Hidden') {
        if (-not $VerifyAllFirst) { $hiddenSkipped++ }
        Write-Verbose "Line $($entry.Line) skipped: $($check.Detail)"
        continue
    }
    if ($check.Status -eq 'Missing') {
        $failures++
        Report-Problem -Entry $entry -Check $check
        if ($OnMissing -eq 'Stop') {
            Write-Warning 'Stopped due to -OnMissing Stop.'
            exit 2
        }
        continue
    }
    if ($check.Status -eq 'Error') {
        $failures++
        Report-Problem -Entry $entry -Check $check
        if ($OnError -eq 'Stop') {
            Write-Warning 'Stopped due to -OnError Stop.'
            exit 2
        }
        continue
    }

    if ($DryRun) {
        Write-Host "WOULD MOVE: $($entry.Source) -> $($entry.Target)"
        $wouldMove++
        continue
    }

    try {
        $targetDir = [IO.Path]::GetDirectoryName($entry.Target)
        [void][IO.Directory]::CreateDirectory($targetDir)
        # A last-moment check reduces the risk of an accidental overwrite.
        if (Test-Path -LiteralPath $entry.Target) {
            throw "Destination appeared before move: $($entry.Target)"
        }
        Move-Item -LiteralPath $entry.Source -Destination $entry.Target -ErrorAction Stop
        Write-Verbose "Moved: $($entry.Source) -> $($entry.Target)"
        $moved++
    }
    catch {
        $failures++
        Write-Warning "Move failed, line $($entry.Line): $($_.Exception.Message)"
        if ($OnError -eq 'Stop') {
            Write-Warning 'Stopped due to -OnError Stop.'
            exit 2
        }
    }
}

if ($DryRun) {
    Write-Host "Dry run complete: $wouldMove would move, $hiddenSkipped hidden skipped, $failures failure(s)."
}
else {
    Write-Host "Complete: $moved moved, $hiddenSkipped hidden skipped, $failures failure(s)."
}
if ($failures -gt 0) { exit 2 }
exit 0
