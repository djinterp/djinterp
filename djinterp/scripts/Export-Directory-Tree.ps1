<#
.SYNOPSIS
    Outputs the directory structure of a folder and its subfolders, filtering by file extensions.

.DESCRIPTION
    Displays a tree-like structure of directories and files, with options to filter by
    file extensions, include files without extensions, and control visibility of hidden items.

.PARAMETER Path
    The root folder path to scan. Defaults to the current directory.

.PARAMETER Extensions
    Array of file extensions to include (e.g., ".txt", ".ps1", ".tar.gz").
    Matching is done by checking if the filename ends with the provided text (case-insensitive).
    If not specified, all files are included.

.PARAMETER IncludeNoExtension
    Include files that have no extension at all (i.e., filename has no dot suffix).

.PARAMETER IncludeHidden
    Include hidden files and folders in the output. By default, hidden items are excluded.

.PARAMETER IncludeRootFolder
    Display the root folder name at the top of the output.
    By default, only the contents are shown.

.PARAMETER ExcludeNoMatchDirectories
    Exclude directories that contain no matching files anywhere in their subtree.

.PARAMETER ExcludeEmptyFolders
    Exclude directories that are empty (contain no children after hidden filtering).

.EXAMPLE
	.\Export-Directory-Tree.ps1        `
	  -Path "D:\example\foo\bar\etc\"  `
	  -Extensions @('.c', '.h')        `
	  -IncludeRootFolder               `
	  -IncludeNoExtension              `
	  -IncludeHidden                   `
	  -ExcludeNoMatchDirectories       `
	  -ExcludeEmptyFolders

.EXAMPLE
    .\Export-Directory-Tree.ps1 -Path "C:\MyFolder" -Extensions ".txt",".log"

.EXAMPLE
    .\Export-Directory-Tree.ps1 -Path "C:\MyFolder" -IncludeNoExtension -IncludeHidden

.EXAMPLE
    .\Export-Directory-Tree.ps1 -Extensions ".ps1",".psm1" -IncludeRootFolder -ExcludeNoMatchDirectories
#>

[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [string]$Path = (Get-Location).Path,

    [Parameter()]
    [string[]]$Extensions,

    [Parameter()]
    [switch]$IncludeNoExtension,

    [Parameter()]
    [switch]$IncludeHidden,

    [Parameter()]
    [switch]$IncludeRootFolder,

    [Parameter()]
    [switch]$ExcludeNoMatchDirectories,

    [Parameter()]
    [switch]$ExcludeEmptyFolders
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Normalize-Extensions {
    param([string[]]$Exts)

    if (-not $Exts) { return @() }

    $norm = New-Object System.Collections.Generic.List[string]
    foreach ($e in $Exts) {
        if ($null -eq $e) { continue }
        $t = $e.Trim()
        if ($t.Length -eq 0) { continue }
        if ($t[0] -ne '.') { $t = '.' + $t }
        $norm.Add($t)
    }
    return $norm.ToArray()
}

function Test-IsHiddenItem {
    param([System.IO.FileSystemInfo]$Item)
    return (($Item.Attributes -band [System.IO.FileAttributes]::Hidden) -ne 0)
}

function Test-HasNoExtension {
    param([System.IO.FileInfo]$File)
    # "no dot suffix" (treats ".gitignore" as having an extension)
    return ($File.Name -notmatch '\.[^\.]+$')
}

function Test-ExtensionMatch {
    param(
        [System.IO.FileInfo]$File,
        [string[]]$NormExts
    )

    if ($null -eq $NormExts -or $NormExts.Count -eq 0) {
        return $true  # no filter => everything matches
    }

    foreach ($ext in $NormExts) {
        if ($File.Name.EndsWith($ext, [System.StringComparison]::OrdinalIgnoreCase)) {
            return $true
        }
    }

    return $false
}

function Should-IncludeFile {
    param(
        [System.IO.FileInfo]$File,
        [string[]]$NormExts,
        [bool]$ShowNoExtension
    )

    if (Test-ExtensionMatch -File $File -NormExts $NormExts) {
        return $true
    }

    if ($ShowNoExtension -and (Test-HasNoExtension -File $File)) {
        return $true
    }

    return $false
}

function Get-ChildrenFiltered {
    param(
        [string]$Dir,
        [bool]$ShowHidden
    )

    $items = Get-ChildItem -LiteralPath $Dir -Force:$ShowHidden -ErrorAction SilentlyContinue

    if (-not $ShowHidden) {
        $items = $items | Where-Object { -not (Test-IsHiddenItem $_) }
    }

    $dirs  = $items | Where-Object { $_.PSIsContainer } | Sort-Object Name
    $files = $items | Where-Object { -not $_.PSIsContainer } | Sort-Object Name

    return [pscustomobject]@{
        Directories = @($dirs)
        Files       = @($files)
    }
}

# Cache for subtree summaries to avoid re-walking dirs repeatedly
$script:DirSummaryCache = @{}

function Get-DirSummary {
    param(
        [string]$Dir,
        [string[]]$NormExts,
        [bool]$ShowNoExtension,
        [bool]$ShowHidden
    )

    if ($script:DirSummaryCache.ContainsKey($Dir)) {
        return $script:DirSummaryCache[$Dir]
    }

    $children = Get-ChildrenFiltered -Dir $Dir -ShowHidden $ShowHidden
    $dirs  = $children.Directories
    $files = $children.Files

    $isEmpty = (($dirs.Count + $files.Count) -eq 0)

    $hasMatchHere = $false
    foreach ($f in $files) {
        $fi = [System.IO.FileInfo]$f
        if (Should-IncludeFile -File $fi -NormExts $NormExts -ShowNoExtension $ShowNoExtension) {
            $hasMatchHere = $true
            break
        }
    }

    $hasMatchBelow = $false
    foreach ($d in $dirs) {
        $childSum = Get-DirSummary -Dir $d.FullName -NormExts $NormExts -ShowNoExtension $ShowNoExtension -ShowHidden $ShowHidden
        if ($childSum.HasMatch) {
            $hasMatchBelow = $true
            break
        }
    }

    $sum = [pscustomobject]@{
        HasMatch = ($hasMatchHere -or $hasMatchBelow)
        IsEmpty  = $isEmpty
    }

    $script:DirSummaryCache[$Dir] = $sum
    return $sum
}

function Show-DirectoryTree {
    param(
        [string]$CurrentPath,
        [string]$Prefix,
        [string[]]$NormExts,
        [bool]$ShowNoExtension,
        [bool]$ShowHidden,
        [bool]$PruneNoMatchDirs,
        [bool]$SkipEmptyDirs
    )

    $children = Get-ChildrenFiltered -Dir $CurrentPath -ShowHidden $ShowHidden
    $directories = $children.Directories
    $files       = $children.Files

    # Decide which subdirectories to show (based on the two new flags)
    $shownDirs = New-Object System.Collections.Generic.List[object]
    foreach ($d in $directories) {
        $sum = Get-DirSummary -Dir $d.FullName -NormExts $NormExts -ShowNoExtension $ShowNoExtension -ShowHidden $ShowHidden

        if ($SkipEmptyDirs -and $sum.IsEmpty) { continue }
        if ($PruneNoMatchDirs -and (-not $sum.HasMatch)) { continue }

        $shownDirs.Add($d)
    }

    # Decide which files to show (based on extension/no-extension rules)
    $shownFiles = New-Object System.Collections.Generic.List[object]
    foreach ($f in $files) {
        $fi = [System.IO.FileInfo]$f
        if (Should-IncludeFile -File $fi -NormExts $NormExts -ShowNoExtension $ShowNoExtension) {
            $shownFiles.Add($f)
        }
    }

    # Combine for display (dirs first, then files)
    $allItems = @($shownDirs.ToArray() + $shownFiles.ToArray())

    for ($i = 0; $i -lt $allItems.Count; $i++) {
        $item = $allItems[$i]
        $isLastItem = ($i -eq ($allItems.Count - 1))

        # ASCII tree connectors
        $connector = if ($isLastItem) { '\-- ' } else { '|-- ' }
        $nextPad   = if ($isLastItem) { '    ' } else { '|   ' }

        if ($item.PSIsContainer) {
            Write-Output "$Prefix$connector$($item.Name)\"
            Show-DirectoryTree -CurrentPath $item.FullName `
                               -Prefix ($Prefix + $nextPad) `
                               -NormExts $NormExts `
                               -ShowNoExtension $ShowNoExtension `
                               -ShowHidden $ShowHidden `
                               -PruneNoMatchDirs $PruneNoMatchDirs `
                               -SkipEmptyDirs $SkipEmptyDirs
        }
        else {
            Write-Output "$Prefix$connector$($item.Name)"
        }
    }
}

# Validate path
if (-not (Test-Path -LiteralPath $Path -PathType Container)) {
    throw "Path '$Path' does not exist or is not a directory."
}

$normExts = Normalize-Extensions -Exts $Extensions
$rootItem = Get-Item -LiteralPath $Path

if ($IncludeRootFolder) {
    Write-Output "$($rootItem.Name)\"
}

Show-DirectoryTree -CurrentPath $Path `
                   -Prefix "" `
                   -NormExts $normExts `
                   -ShowNoExtension ([bool]$IncludeNoExtension) `
                   -ShowHidden ([bool]$IncludeHidden) `
                   -PruneNoMatchDirs ([bool]$ExcludeNoMatchDirectories) `
                   -SkipEmptyDirs ([bool]$ExcludeEmptyFolders)
