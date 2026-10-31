<#
.SYNOPSIS
  Mirror directory folders from SourceDir into DestDir, and optionally copy matching files.

.DESCRIPTION
  - Creates folders found in SourceDir under DestDir (folders-only).
  - Optional: copies files that match filters (Include/Exclude wildcards, NameRegex, size, modified date).
  - Preserves relative folder/file paths from SourceDir to DestDir.

.EXAMPLE
  # Create all folders (recursive) from src into dest, no files
  .\Copy-DirectoryStructure.ps1 -SourceDir "C:\src" -DestDir "D:\dest" -Recurse

.EXAMPLE
  # Create folders + copy only .h and .c files modified in last 30 days
  .\Copy-DirectoryStructure.ps1 -SourceDir "C:\src" -DestDir "D:\dest" -Recurse -CopyFiles `
      -Include "*.h","*.c" -NewerThan (Get-Date).AddDays(-30)

.EXAMPLE
  # Copy files that match a regex, and are between 1KB and 5MB
  .\Copy-DirectoryStructure.ps1 -SourceDir "C:\src" -DestDir "D:\dest" -Recurse -CopyFiles `
      -NameRegex '\.(png|jpg|webp)$' -MinBytes 1024 -MaxBytes (5MB)

.NOTES
  Works on Windows PowerShell 5.1+ and PowerShell 7+.
#>

[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [Parameter(Mandatory)]
    [ValidateNotNullOrEmpty()]
    [string] $SourceDir,

    [Parameter(Mandatory)]
    [ValidateNotNullOrEmpty()]
    [string] $DestDir,

    # If set, mirror subfolders recursively. If not, only top-level subfolders.
    [switch] $Recurse,

    # Optional: Copy files that match criteria
    [switch] $CopyFiles,

    # Wildcard patterns to include/exclude (applies to file *names*)
    [string[]] $Include = @("*"),
    [string[]] $Exclude = @(),

    # Optional regex on the file name (full name, not path). Example: '\.(h|c)$'
    [string] $NameRegex,

    # Optional size filters
    [Nullable[long]] $MinBytes,
    [Nullable[long]] $MaxBytes,

    # Optional last write time filters
    [Nullable[datetime]] $NewerThan,
    [Nullable[datetime]] $OlderThan,

    # Copy behavior
    [switch] $Overwrite,

    # If set, preserves LastWriteTime on copied files (Copy-Item usually does anyway, but this is explicit)
    [switch] $PreserveTimestamps
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Get-RelativePath {
    param(
        [Parameter(Mandatory)][string] $BasePath,
        [Parameter(Mandatory)][string] $FullPath
    )

    $base = (Resolve-Path -LiteralPath $BasePath).Path.TrimEnd('\','/')
    $full = (Resolve-Path -LiteralPath $FullPath).Path

    # Uri-based relative path (works in PS 5.1 / .NET Framework too)
    $baseUri = [Uri]::new(($base + [IO.Path]::DirectorySeparatorChar))
    $fullUri = [Uri]::new($full)
    $relUri  = $baseUri.MakeRelativeUri($fullUri)
    $rel     = [Uri]::UnescapeDataString($relUri.ToString())

    return $rel -replace '/', [IO.Path]::DirectorySeparatorChar
}

function Matches-AnyWildcard {
    param(
        [Parameter(Mandatory)][string] $Name,
        [Parameter(Mandatory)][string[]] $Patterns
    )
    foreach ($p in $Patterns) {
        if ($Name -like $p) { return $true }
    }
    return $false
}

# Validate source exists
if (-not (Test-Path -LiteralPath $SourceDir -PathType Container)) {
    throw "SourceDir does not exist or is not a directory: $SourceDir"
}

# Ensure dest exists
if (-not (Test-Path -LiteralPath $DestDir -PathType Container)) {
    if ($PSCmdlet.ShouldProcess($DestDir, "Create destination directory")) {
        New-Item -ItemType Directory -Path $DestDir -Force | Out-Null
    }
}

# Collect directories
$dirParams = @{
    LiteralPath = $SourceDir
    Directory   = $true
}
if ($Recurse) { $dirParams.Recurse = $true }

$dirs = Get-ChildItem @dirParams

# Create directories in destination, preserving relative paths
foreach ($d in $dirs) {
    $rel = Get-RelativePath -BasePath $SourceDir -FullPath $d.FullName
    $targetDir = Join-Path -Path $DestDir -ChildPath $rel

    if ($PSCmdlet.ShouldProcess($targetDir, "Create directory")) {
        New-Item -ItemType Directory -Path $targetDir -Force | Out-Null
    }
}

if (-not $CopyFiles) {
    return
}

# Collect files
$fileParams = @{
    LiteralPath = $SourceDir
    File        = $true
}
if ($Recurse) { $fileParams.Recurse = $true }

$files = Get-ChildItem @fileParams

# Filter + copy
foreach ($f in $files) {
    $name = $f.Name

    # Include/Exclude wildcards
    if ($Include.Count -gt 0 -and -not (Matches-AnyWildcard -Name $name -Patterns $Include)) { continue }
    if ($Exclude.Count -gt 0 -and (Matches-AnyWildcard -Name $name -Patterns $Exclude)) { continue }

    # Regex filter
    if ($NameRegex -and ($name -notmatch $NameRegex)) { continue }

    # Size filters
    if ($PSBoundParameters.ContainsKey('MinBytes') -and $f.Length -lt $MinBytes) { continue }
    if ($PSBoundParameters.ContainsKey('MaxBytes') -and $f.Length -gt $MaxBytes) { continue }
    
    # Date filters (LastWriteTime)
    if ($PSBoundParameters.ContainsKey('NewerThan') -and $f.LastWriteTime -lt $NewerThan) { continue }
    if ($PSBoundParameters.ContainsKey('OlderThan') -and $f.LastWriteTime -gt $OlderThan) { continue }


    # Destination path preserving relative structure
    $relFile = Get-RelativePath -BasePath $SourceDir -FullPath $f.FullName
    $destFile = Join-Path -Path $DestDir -ChildPath $relFile
    $destParent = Split-Path -Path $destFile -Parent

    if (-not (Test-Path -LiteralPath $destParent)) {
        if ($PSCmdlet.ShouldProcess($destParent, "Create parent directory for file")) {
            New-Item -ItemType Directory -Path $destParent -Force | Out-Null
        }
    }

    $copyParams = @{
        LiteralPath = $f.FullName
        Destination = $destFile
        Force       = $Overwrite.IsPresent
    }

    if (-not $Overwrite -and (Test-Path -LiteralPath $destFile)) {
        continue
    }

    if ($PSCmdlet.ShouldProcess($destFile, "Copy file")) {
        Copy-Item @copyParams
        if ($PreserveTimestamps) {
            # Explicitly set timestamps to match source (LastWriteTime is most common)
            (Get-Item -LiteralPath $destFile).LastWriteTime = $f.LastWriteTime
        }
    }
}
