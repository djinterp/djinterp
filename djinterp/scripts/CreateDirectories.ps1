<# CreateDirectories.ps1

Creates destination directories from matching source files and/or source folders.
Designed to stay close in style to DirectoryTree.ps1: PowerShell 5.1+, simple
helper functions, multi-dot suffix matching, optional hidden items, depth parsing,
and pipeline-friendly optional objects via -PassThru.

Core behavior:
- Matching files create destination folders named after the file stem.
  Example: foo.h -> <Destination>\foo
- With -IncludeSubfolders or -Depth, relative source subfolder paths are preserved.
  Example: src\api\foo.h -> dest\api\foo
- With -Flatten, relative source subfolder paths are ignored.
  Example: src\api\foo.h -> dest\foo
- With -CopyFolders, source folders become destination folders too.
- With -CopyMatchingFolders, only folders matching -Folders/-FolderPatterns/-FolderRegex
  become destination folders.
- With -ListMatchingFolders, folders that contain matching files are also created.
- With -ListFile or -ListLines, source scanning is skipped and the indented list is
  used instead. Each nonblank line is one folder name; indentation determines nesting.

Modes:
- Default: create missing folders, skip existing folders.
- -Query: show what is missing and what is already done; make no changes.
- -Delete: delete matching destination folders instead of creating them.
- -Overwrite: if a matching folder already exists, delete it and recreate it.
- -DryRun: make no changes and print the plan color-coded.

Colors in -DryRun/-Query:
- Green  = add/create
- Red    = delete/collision
- Yellow = overwrite
- Cyan   = already done
- Gray   = missing/no-op

WARNING:
- -Delete removes matching destination folders recursively.
- -Overwrite removes existing matching destination folders recursively, then recreates them.

.EXAMPLE
    .\CreateDirectories.ps1 `
        -Source "C:\some_folder\some_subfolder\folder" `
        -Destination "C:\some_other_folder\some_other_subfolder" `
        -Extensions ".h", ".c" `
        -IncludeNoExtension `
        -ListMatchingFolders `
        -IncludeHidden

.EXAMPLE
    .\CreateDirectories.ps1 -Source .\src -Destination .\out -Extensions .h,.hpp -IncludeSubfolders -Depth 3 -DryRun

.EXAMPLE
    .\CreateDirectories.ps1 -Source .\src -Destination .\out -Extensions .h,.c -Query

.EXAMPLE
    .\CreateDirectories.ps1 -Source .\src -Destination .\out -Extensions .h,.c -Delete -DryRun

.EXAMPLE
    .\CreateDirectories.ps1 -ListFile .\folders.txt -Destination .\out -DryRun

folders.txt:
    core
        parser
        lexer
    tests
#>

[CmdletBinding(SupportsShouldProcess = $true)]
param
(
    [string[]] $Source,

    [Parameter(Mandatory)]
    [string[]] $Destination,

    [object] $Depth,

    [string[]] $Extensions,
    [string[]] $Files,
    [string[]] $FilePatterns,
    [string[]] $FileRegex,

    [switch] $IncludeNoExtension,
    [switch] $ExcludeNoExtension,

    [switch] $IncludeHidden,

    [switch] $IncludeSubfolders,
    [switch] $Flatten,

    [switch] $CopyFolders,
    [switch] $CopyMatchingFolders,
    [switch] $ListMatchingFolders,

    [string[]] $Folders,
    [string[]] $FolderPatterns,
    [string[]] $FolderRegex,

    [string[]] $ExcludeFolders,
    [string[]] $ExcludeFolderPatterns,
    [string[]] $ExcludeFolderRegex,

    [string[]] $DescendFolderPatterns,
    [string[]] $DescendFolderRegex,

    [string] $ListFile,
    [string[]] $ListLines,

    [switch] $Query,
    [switch] $Delete,
    [switch] $Overwrite,
    [switch] $DryRun,

    [string[]] $OutFile,

    [switch] $PassThru
)

$DepthWasSpecified = $PSBoundParameters.ContainsKey('Depth')

function Parse-Depth
(
    [object] $d
)
{
    if ($null -eq $d)
    {
        return [int]::MaxValue
    }

    if ($d -is [int])
    {
        return [int]$d
    }

    $s = [string]$d

    if ($s.Trim().ToLowerInvariant() -eq 'max')
    {
        return [int]::MaxValue
    }

    $n = 0

    if ([int]::TryParse($s, [ref]$n))
    {
        if ($n -lt 0)
        {
            return 0
        }

        return $n
    }

    return [int]::MaxValue
}

function Normalize-Suffixes
(
    [string[]] $suffixes
)
{
    if (-not $suffixes)
    {
        return @()
    }

    $out = @()

    foreach ($x in $suffixes)
    {
        if ($null -eq $x)
        {
            continue
        }

        $s = $x.Trim()

        if ($s.Length -eq 0)
        {
            continue
        }

        if ($s[0] -ne '.')
        {
            $s = '.' + $s
        }

        $out += $s.ToLowerInvariant()
    }

    return $out | Select-Object -Unique
}

function Normalize-Names
(
    [string[]] $names
)
{
    if (-not $names)
    {
        return @()
    }

    $out = @()

    foreach ($n in $names)
    {
        if ($null -eq $n)
        {
            continue
        }

        $s = $n.Trim()

        if ($s.Length -eq 0)
        {
            continue
        }

        $out += $s.ToLowerInvariant()
    }

    return $out | Select-Object -Unique
}

function Convert-ToFullPath
(
    [string] $p
)
{
    if ($null -eq $p)
    {
        throw "Path is null."
    }

    $expanded = [System.Environment]::ExpandEnvironmentVariables($p)

    if ([System.IO.Path]::IsPathRooted($expanded))
    {
        return [System.IO.Path]::GetFullPath($expanded)
    }

    return [System.IO.Path]::GetFullPath((Join-Path -Path (Get-Location).Path -ChildPath $expanded))
}

function Resolve-ExistingFullPath
(
    [string] $p
)
{
    $item = Get-Item -LiteralPath $p -ErrorAction Stop
    return $item.FullName
}

function Ensure-TrailingSlash
(
    [string] $p
)
{
    if ($p.EndsWith('\') -or $p.EndsWith('/'))
    {
        return $p
    }

    return $p + '\'
}

function Get-RelativePath
(
    [string] $baseDir,
    [string] $childPath
)
{
    $b = Ensure-TrailingSlash (Convert-ToFullPath $baseDir)
    $c = Convert-ToFullPath $childPath

    if ($c.TrimEnd('\') -ieq $b.TrimEnd('\'))
    {
        return ""
    }

    $baseUri  = New-Object System.Uri -ArgumentList $b
    $childUri = New-Object System.Uri -ArgumentList $c
    $relUri   = $baseUri.MakeRelativeUri($childUri)

    return ([System.Uri]::UnescapeDataString($relUri.ToString())).Replace('/', '\')
}

function Join-RelativePath
(
    [string] $left,
    [string] $right
)
{
    if ($null -eq $left)
    {
        $left = ""
    }

    if ($null -eq $right)
    {
        $right = ""
    }

    $l = $left.Trim().Trim('\', '/')
    $r = $right.Trim().Trim('\', '/')

    if ($l.Length -eq 0)
    {
        return $r
    }

    if ($r.Length -eq 0)
    {
        return $l
    }

    return (Join-Path -Path $l -ChildPath $r)
}

function Join-DestinationRelative
(
    [string] $destRoot,
    [string] $rel
)
{
    $root = Convert-ToFullPath $destRoot

    if ($null -eq $rel)
    {
        return $root
    }

    $r = $rel.Trim().Trim('\', '/')

    if ($r.Length -eq 0)
    {
        return $root
    }

    return (Join-Path -Path $root -ChildPath $r)
}

function Test-AnyWildcard
(
    [string]   $text,
    [string[]] $patterns
)
{
    if (-not $patterns)
    {
        return $false
    }

    foreach ($p in $patterns)
    {
        if ($null -eq $p)
        {
            continue
        }

        if ($p.Trim().Length -eq 0)
        {
            continue
        }

        if ($text -like $p)
        {
            return $true
        }
    }

    return $false
}

function Test-AnyRegex
(
    [string]   $text,
    [string[]] $regexes
)
{
    if (-not $regexes)
    {
        return $false
    }

    foreach ($r in $regexes)
    {
        if ($null -eq $r)
        {
            continue
        }

        if ($r.Trim().Length -eq 0)
        {
            continue
        }

        if ([regex]::IsMatch($text, $r, [System.Text.RegularExpressions.RegexOptions]::IgnoreCase))
        {
            return $true
        }
    }

    return $false
}

function Test-FileMatches
(
    [System.IO.FileInfo] $file,
    [string[]]           $suffixesLower,
    [string[]]           $namesLower,
    [string[]]           $patterns,
    [string[]]           $regexes,
    [bool]               $includeNoExt,
    [bool]               $excludeNoExt
)
{
    if ($null -eq $file)
    {
        return $false
    }

    $name  = $file.Name
    $lower = $name.ToLowerInvariant()
    $noExt = [string]::IsNullOrEmpty($file.Extension)

    if ($excludeNoExt -and $noExt)
    {
        return $false
    }

    $hasFilters = $false

    if ($suffixesLower -and ($suffixesLower.Count -gt 0)) { $hasFilters = $true }
    if ($namesLower    -and ($namesLower.Count    -gt 0)) { $hasFilters = $true }
    if ($patterns      -and ($patterns.Count      -gt 0)) { $hasFilters = $true }
    if ($regexes       -and ($regexes.Count       -gt 0)) { $hasFilters = $true }
    if ($includeNoExt)                                  { $hasFilters = $true }

    if (-not $hasFilters)
    {
        return $true
    }

    if ($namesLower -and ($namesLower.Count -gt 0))
    {
        if ($namesLower -contains $lower)
        {
            return $true
        }
    }

    if ($suffixesLower -and ($suffixesLower.Count -gt 0))
    {
        foreach ($sfx in $suffixesLower)
        {
            if ($lower.EndsWith($sfx))
            {
                return $true
            }
        }
    }

    if (Test-AnyWildcard -text $name -patterns $patterns)
    {
        return $true
    }

    if (Test-AnyRegex -text $name -regexes $regexes)
    {
        return $true
    }

    if ($includeNoExt -and $noExt)
    {
        return $true
    }

    return $false
}

function Get-FileStemForDirectory
(
    [System.IO.FileInfo] $file,
    [string[]]           $suffixesLower
)
{
    $name  = $file.Name
    $lower = $name.ToLowerInvariant()

    if ($suffixesLower -and ($suffixesLower.Count -gt 0))
    {
        $ordered = @($suffixesLower | Sort-Object Length -Descending)

        foreach ($sfx in $ordered)
        {
            if ($lower.EndsWith($sfx))
            {
                $stem = $name.Substring(0, $name.Length - $sfx.Length)

                if ($stem.Length -gt 0)
                {
                    return $stem
                }
            }
        }
    }

    if ([string]::IsNullOrEmpty($file.Extension))
    {
        return $file.Name
    }

    return [System.IO.Path]::GetFileNameWithoutExtension($file.Name)
}

function Test-FolderExcluded
(
    [System.IO.DirectoryInfo] $dir,
    [string[]]               $excludeNamesLower,
    [string[]]               $excludePatterns,
    [string[]]               $excludeRegexes
)
{
    if ($null -eq $dir)
    {
        return $true
    }

    $name  = $dir.Name
    $lower = $name.ToLowerInvariant()

    if ($excludeNamesLower -and ($excludeNamesLower.Count -gt 0))
    {
        if ($excludeNamesLower -contains $lower)
        {
            return $true
        }
    }

    if (Test-AnyWildcard -text $name -patterns $excludePatterns)
    {
        return $true
    }

    if (Test-AnyRegex -text $name -regexes $excludeRegexes)
    {
        return $true
    }

    return $false
}

function Test-FolderExplicitMatch
(
    [System.IO.DirectoryInfo] $dir,
    [string[]]               $namesLower,
    [string[]]               $patterns,
    [string[]]               $regexes
)
{
    if ($null -eq $dir)
    {
        return $false
    }

    $name  = $dir.Name
    $lower = $name.ToLowerInvariant()

    $hasFilters = $false

    if ($namesLower -and ($namesLower.Count -gt 0)) { $hasFilters = $true }
    if ($patterns   -and ($patterns.Count   -gt 0)) { $hasFilters = $true }
    if ($regexes    -and ($regexes.Count    -gt 0)) { $hasFilters = $true }

    if (-not $hasFilters)
    {
        return $false
    }

    if ($namesLower -and ($namesLower.Count -gt 0))
    {
        if ($namesLower -contains $lower)
        {
            return $true
        }
    }

    if (Test-AnyWildcard -text $name -patterns $patterns)
    {
        return $true
    }

    if (Test-AnyRegex -text $name -regexes $regexes)
    {
        return $true
    }

    return $false
}

function Test-FolderAllowedForTraversal
(
    [System.IO.DirectoryInfo] $dir,
    [string[]]               $excludeNamesLower,
    [string[]]               $excludePatterns,
    [string[]]               $excludeRegexes,
    [string[]]               $descendPatterns,
    [string[]]               $descendRegexes
)
{
    if (Test-FolderExcluded -dir $dir -excludeNamesLower $excludeNamesLower -excludePatterns $excludePatterns -excludeRegexes $excludeRegexes)
    {
        return $false
    }

    $hasDescendFilters = $false

    if ($descendPatterns -and ($descendPatterns.Count -gt 0)) { $hasDescendFilters = $true }
    if ($descendRegexes  -and ($descendRegexes.Count  -gt 0)) { $hasDescendFilters = $true }

    if (-not $hasDescendFilters)
    {
        return $true
    }

    if (Test-AnyWildcard -text $dir.Name -patterns $descendPatterns)
    {
        return $true
    }

    if (Test-AnyRegex -text $dir.Name -regexes $descendRegexes)
    {
        return $true
    }

    return $false
}

function New-Candidate
(
    [string] $sourcePath,
    [string] $destinationRoot,
    [string] $relativePath,
    [string] $kind
)
{
    $target = Join-DestinationRelative -destRoot $destinationRoot -rel $relativePath

    return [pscustomobject]@{
        Source          = $sourcePath
        DestinationRoot = (Convert-ToFullPath $destinationRoot)
        RelativePath    = $relativePath
        TargetPath      = $target
        Kind            = $kind
    }
}

function Add-Candidate
(
    [System.Collections.Generic.List[object]] $list,
    [hashtable]                              $seen,
    [object]                                 $candidate
)
{
    if ($null -eq $candidate)
    {
        return
    }

    if ($null -eq $candidate.RelativePath)
    {
        return
    }

    if ($candidate.RelativePath.Trim().Length -eq 0)
    {
        return
    }

    $key = $candidate.TargetPath.ToLowerInvariant()

    if (-not $seen.ContainsKey($key))
    {
        $seen[$key] = $true
        $list.Add($candidate)
    }
}

function Get-Subdirs
(
    [string] $dirPath,
    [bool]   $includeHidden
)
{
    $p = @{
        LiteralPath = $dirPath
        Directory   = $true
        ErrorAction = 'SilentlyContinue'
    }

    if ($includeHidden)
    {
        $p.Force = $true
    }

    return @(Get-ChildItem @p | Sort-Object Name)
}

function Get-FilesInDir
(
    [string] $dirPath,
    [bool]   $includeHidden
)
{
    $p = @{
        LiteralPath = $dirPath
        File        = $true
        ErrorAction = 'SilentlyContinue'
    }

    if ($includeHidden)
    {
        $p.Force = $true
    }

    return @(Get-ChildItem @p | Sort-Object Name)
}

function Get-EffectiveDepth
()
{
    if ($DepthWasSpecified)
    {
        return Parse-Depth $Depth
    }

    if ($IncludeSubfolders)
    {
        return [int]::MaxValue
    }

    if ($CopyFolders -or $CopyMatchingFolders -or $ListMatchingFolders)
    {
        return 1
    }

    return 0
}

function Get-CandidatesFromSource
(
    [string]   $sourcePath,
    [string]   $destinationRoot,
    [int]      $depthLimit,
    [string[]] $suffixesLower,
    [string[]] $fileNamesLower,
    [string[]] $folderNamesLower,
    [string[]] $excludeFolderNamesLower
)
{
    $candidates = New-Object System.Collections.Generic.List[object]
    $seen       = @{}

    try
    {
        $sourceItem = Get-Item -LiteralPath $sourcePath -ErrorAction Stop
    }
    catch
    {
        Write-Warning ("Source not found: {0}" -f $sourcePath)
        return @()
    }

    if (-not $sourceItem.PSIsContainer)
    {
        $file = [System.IO.FileInfo]$sourceItem

        if (Test-FileMatches -file $file -suffixesLower $suffixesLower -namesLower $fileNamesLower `
            -patterns $FilePatterns -regexes $FileRegex -includeNoExt $IncludeNoExtension `
            -excludeNoExt $ExcludeNoExtension)
        {
            $leaf = Get-FileStemForDirectory -file $file -suffixesLower $suffixesLower
            $candidate = New-Candidate -sourcePath $file.FullName -destinationRoot $destinationRoot -relativePath $leaf -kind 'File'
            Add-Candidate -list $candidates -seen $seen -candidate $candidate
        }

        return $candidates.ToArray()
    }

    $rootDir = $sourceItem.FullName

    function Walk-Dir
    (
        [string] $dirPath,
        [int]    $level
    )
    {
        $dirInfo = Get-Item -LiteralPath $dirPath -ErrorAction SilentlyContinue

        if ($null -eq $dirInfo)
        {
            return
        }

        $relDir = Get-RelativePath -baseDir $rootDir -childPath $dirPath

        $matchedFileCount = 0
        $files = Get-FilesInDir -dirPath $dirPath -includeHidden $IncludeHidden

        foreach ($f in $files)
        {
            if (Test-FileMatches -file $f -suffixesLower $suffixesLower -namesLower $fileNamesLower `
                -patterns $FilePatterns -regexes $FileRegex -includeNoExt $IncludeNoExtension `
                -excludeNoExt $ExcludeNoExtension)
            {
                $matchedFileCount++

                $leaf = Get-FileStemForDirectory -file $f -suffixesLower $suffixesLower

                if ($Flatten)
                {
                    $relTarget = $leaf
                }
                else
                {
                    $relTarget = Join-RelativePath -left $relDir -right $leaf
                }

                $candidate = New-Candidate -sourcePath $f.FullName -destinationRoot $destinationRoot -relativePath $relTarget -kind 'File'
                Add-Candidate -list $candidates -seen $seen -candidate $candidate
            }
        }

        if ($relDir.Trim().Length -gt 0)
        {
            $includeFolder = $false

            if ($CopyFolders)
            {
                if (-not (Test-FolderExcluded -dir $dirInfo -excludeNamesLower $excludeFolderNamesLower `
                    -excludePatterns $ExcludeFolderPatterns -excludeRegexes $ExcludeFolderRegex))
                {
                    $includeFolder = $true
                }
            }

            if (-not $includeFolder)
            {
                if ($CopyMatchingFolders)
                {
                    if (-not (Test-FolderExcluded -dir $dirInfo -excludeNamesLower $excludeFolderNamesLower `
                        -excludePatterns $ExcludeFolderPatterns -excludeRegexes $ExcludeFolderRegex))
                    {
                        if (Test-FolderExplicitMatch -dir $dirInfo -namesLower $folderNamesLower -patterns $FolderPatterns -regexes $FolderRegex)
                        {
                            $includeFolder = $true
                        }
                    }
                }
            }

            if (-not $includeFolder)
            {
                if ($ListMatchingFolders -and ($matchedFileCount -gt 0))
                {
                    if (-not (Test-FolderExcluded -dir $dirInfo -excludeNamesLower $excludeFolderNamesLower `
                        -excludePatterns $ExcludeFolderPatterns -excludeRegexes $ExcludeFolderRegex))
                    {
                        $includeFolder = $true
                    }
                }
            }

            if ($includeFolder)
            {
                if ($Flatten)
                {
                    $relFolderTarget = $dirInfo.Name
                }
                else
                {
                    $relFolderTarget = $relDir
                }

                $candidate2 = New-Candidate -sourcePath $dirInfo.FullName -destinationRoot $destinationRoot -relativePath $relFolderTarget -kind 'Folder'
                Add-Candidate -list $candidates -seen $seen -candidate $candidate2
            }
        }

        if ($level -lt $depthLimit)
        {
            $subs = Get-Subdirs -dirPath $dirPath -includeHidden $IncludeHidden

            foreach ($d in $subs)
            {
                if (Test-FolderAllowedForTraversal -dir $d -excludeNamesLower $excludeFolderNamesLower `
                    -excludePatterns $ExcludeFolderPatterns -excludeRegexes $ExcludeFolderRegex `
                    -descendPatterns $DescendFolderPatterns -descendRegexes $DescendFolderRegex)
                {
                    Walk-Dir -dirPath $d.FullName -level ($level + 1)
                }
            }
        }
    }

    Walk-Dir -dirPath $rootDir -level 0

    return $candidates.ToArray()
}

function Get-IndentWidth
(
    [string] $line
)
{
    $m = [regex]::Match($line, '^\s*')
    $leading = $m.Value
    $expanded = $leading.Replace("`t", "    ")
    return $expanded.Length
}

function Parse-IndentedList
(
    [string[]] $lines
)
{
    $out   = New-Object System.Collections.Generic.List[string]
    $stack = New-Object System.Collections.Generic.List[object]

    foreach ($raw in $lines)
    {
        if ($null -eq $raw)
        {
            continue
        }

        $text = $raw.Trim()

        if ($text.Length -eq 0)
        {
            continue
        }

        if ($text.StartsWith('#'))
        {
            continue
        }

        $indent = Get-IndentWidth $raw

        while (($stack.Count -gt 0) -and ($indent -le [int]$stack[$stack.Count - 1].Indent))
        {
            $stack.RemoveAt($stack.Count - 1)
        }

        $parent = ""

        if ($stack.Count -gt 0)
        {
            $parent = [string]$stack[$stack.Count - 1].Path
        }

        $path = Join-RelativePath -left $parent -right $text

        $out.Add($path)

        $stack.Add([pscustomobject]@{
            Indent = $indent
            Path   = $path
        })
    }

    return @($out | Select-Object -Unique)
}

function Get-CandidatesFromList
(
    [string[]] $relativePaths,
    [string[]] $destinationRoots,
    [string]   $sourceLabel
)
{
    $out  = New-Object System.Collections.Generic.List[object]
    $seen = @{}

    foreach ($dest in $destinationRoots)
    {
        foreach ($rel in $relativePaths)
        {
            if ($null -eq $rel)
            {
                continue
            }

            if ($rel.Trim().Length -eq 0)
            {
                continue
            }

            $candidate = New-Candidate -sourcePath $sourceLabel -destinationRoot $dest -relativePath $rel -kind 'List'
            Add-Candidate -list $out -seen $seen -candidate $candidate
        }
    }

    return $out.ToArray()
}

function Get-DestinationForSource
(
    [int]      $index,
    [int]      $sourceCount,
    [string[]] $destinations
)
{
    if ($destinations.Count -eq 1)
    {
        return $destinations[0]
    }

    if ($destinations.Count -eq $sourceCount)
    {
        return $destinations[$index]
    }

    throw "Destination must contain either one path or the same number of paths as Source."
}

function New-PlanItem
(
    [object] $candidate
)
{
    $target = $candidate.TargetPath
    $exists = Test-Path -LiteralPath $target
    $isDirectory = $false
    $operation = $null
    $message = $null

    if ($exists)
    {
        $item = Get-Item -LiteralPath $target -Force -ErrorAction SilentlyContinue

        if (($null -ne $item) -and $item.PSIsContainer)
        {
            $isDirectory = $true
        }
    }

    if ($Delete)
    {
        if ($exists)
        {
            $operation = 'Delete'
            $message   = 'delete matching destination folder'
        }
        else
        {
            $operation = 'Missing'
            $message   = 'nothing to delete'
        }
    }
    elseif ($Query)
    {
        if ($exists -and $isDirectory)
        {
            $operation = 'Done'
            $message   = 'already done'
        }
        elseif ($exists -and (-not $isDirectory))
        {
            $operation = 'Collision'
            $message   = 'target exists but is not a directory'
        }
        else
        {
            $operation = 'Missing'
            $message   = 'missing'
        }
    }
    else
    {
        if ($exists -and $isDirectory)
        {
            if ($Overwrite)
            {
                $operation = 'Overwrite'
                $message   = 'delete and recreate folder'
            }
            else
            {
                $operation = 'Exists'
                $message   = 'already exists; skipped'
            }
        }
        elseif ($exists -and (-not $isDirectory))
        {
            if ($Overwrite)
            {
                $operation = 'Overwrite'
                $message   = 'replace file with folder'
            }
            else
            {
                $operation = 'Collision'
                $message   = 'target exists but is not a directory'
            }
        }
        else
        {
            $operation = 'Add'
            $message   = 'create folder'
        }
    }

    return [pscustomobject]@{
        Operation       = $operation
        Message         = $message
        Exists          = $exists
        IsDirectory     = $isDirectory
        Source          = $candidate.Source
        Kind            = $candidate.Kind
        DestinationRoot = $candidate.DestinationRoot
        RelativePath    = $candidate.RelativePath
        TargetPath      = $candidate.TargetPath
    }
}

function Get-PlanColor
(
    [string] $operation
)
{
    switch ($operation)
    {
        'Add'       { return 'Green' }
        'Delete'    { return 'Red' }
        'Overwrite' { return 'Yellow' }
        'Done'      { return 'Cyan' }
        'Exists'    { return 'Cyan' }
        'Collision' { return 'Red' }
        'Missing'   { return 'DarkGray' }
        default     { return 'Gray' }
    }
}

function Format-PlanLine
(
    [object] $planItem
)
{
    return ("{0,-10} {1}" -f $planItem.Operation.ToUpperInvariant(), $planItem.TargetPath)
}

function Write-PlanLine
(
    [object] $planItem
)
{
    $color = Get-PlanColor -operation $planItem.Operation
    $line  = Format-PlanLine -planItem $planItem
    Write-Host $line -ForegroundColor $color
}

function Invoke-PlanItem
(
    [object] $planItem
)
{
    $op = $planItem.Operation
    $target = $planItem.TargetPath

    if (($op -eq 'Missing') -or ($op -eq 'Done') -or ($op -eq 'Exists') -or ($op -eq 'Collision'))
    {
        Write-Verbose ("{0}: {1}" -f $op, $target)
        return
    }

    if ($DryRun -or $Query)
    {
        return
    }

    if ($op -eq 'Add')
    {
        if ($PSCmdlet.ShouldProcess($target, 'Create directory'))
        {
            New-Item -ItemType Directory -Path $target -Force | Out-Null
            Write-Verbose ("Created: {0}" -f $target)
        }

        return
    }

    if ($op -eq 'Overwrite')
    {
        if ($PSCmdlet.ShouldProcess($target, 'Overwrite directory'))
        {
            if (Test-Path -LiteralPath $target)
            {
                Remove-Item -LiteralPath $target -Recurse -Force
            }

            New-Item -ItemType Directory -Path $target -Force | Out-Null
            Write-Verbose ("Overwrote: {0}" -f $target)
        }

        return
    }

    if ($op -eq 'Delete')
    {
        if ($PSCmdlet.ShouldProcess($target, 'Delete directory recursively'))
        {
            Remove-Item -LiteralPath $target -Recurse -Force
            Write-Verbose ("Deleted: {0}" -f $target)
        }

        return
    }
}

# -----------------------------
# Defaults / setup
# -----------------------------

if ($ExcludeNoExtension)
{
    $IncludeNoExtension = $false
}

if ($Delete -and $Overwrite)
{
    throw "Use either -Delete or -Overwrite, not both."
}

$listMode = $false

if (($ListFile -and ($ListFile.Trim().Length -gt 0)) -or ($ListLines -and ($ListLines.Count -gt 0)))
{
    $listMode = $true
}

if (-not $listMode)
{
    if (-not $Source -or ($Source.Count -eq 0))
    {
        throw "Supply -Source path(s), or use -ListFile/-ListLines."
    }
}

$suffixesLower            = Normalize-Suffixes $Extensions
$fileNamesLower           = Normalize-Names $Files
$folderNamesLower         = Normalize-Names $Folders
$excludeFolderNamesLower  = Normalize-Names $ExcludeFolders
$depthLimit               = Get-EffectiveDepth

$allCandidates = New-Object System.Collections.Generic.List[object]
$globalSeen    = @{}

# -----------------------------
# Candidate collection
# -----------------------------

if ($listMode)
{
    $rawLines = @()

    if ($ListFile -and ($ListFile.Trim().Length -gt 0))
    {
        $rawLines += @(Get-Content -LiteralPath $ListFile -ErrorAction Stop)
    }

    if ($ListLines -and ($ListLines.Count -gt 0))
    {
        $rawLines += $ListLines
    }

    $relativePaths = Parse-IndentedList -lines $rawLines
    $sourceLabel = '<List>'

    if ($ListFile)
    {
        $sourceLabel = Convert-ToFullPath $ListFile
    }

    $listCandidates = Get-CandidatesFromList -relativePaths $relativePaths -destinationRoots $Destination -sourceLabel $sourceLabel

    foreach ($c in $listCandidates)
    {
        Add-Candidate -list $allCandidates -seen $globalSeen -candidate $c
    }
}
else
{
    for ($i = 0; $i -lt $Source.Count; $i++)
    {
        $src  = $Source[$i]
        $dest = Get-DestinationForSource -index $i -sourceCount $Source.Count -destinations $Destination

        $sourceCandidates = Get-CandidatesFromSource -sourcePath $src -destinationRoot $dest -depthLimit $depthLimit `
            -suffixesLower $suffixesLower -fileNamesLower $fileNamesLower -folderNamesLower $folderNamesLower `
            -excludeFolderNamesLower $excludeFolderNamesLower

        foreach ($c2 in $sourceCandidates)
        {
            Add-Candidate -list $allCandidates -seen $globalSeen -candidate $c2
        }
    }
}

# -----------------------------
# Planning / execution
# -----------------------------

$plan = New-Object System.Collections.Generic.List[object]

foreach ($candidate in $allCandidates)
{
    $plan.Add((New-PlanItem -candidate $candidate))
}

$plan = @($plan | Sort-Object TargetPath)

$textLines = New-Object System.Collections.Generic.List[string]

if ($plan.Count -eq 0)
{
    $msg = "No matching directories were found to create, query, or delete."
    $textLines.Add($msg)
    Write-Host $msg -ForegroundColor DarkGray
}
else
{
    foreach ($p in $plan)
    {
        $line = Format-PlanLine -planItem $p
        $textLines.Add($line)

        if ($DryRun -or $Query)
        {
            Write-PlanLine -planItem $p
        }

        Invoke-PlanItem -planItem $p
    }
}

# -----------------------------
# Output files / passthrough
# -----------------------------

if ($OutFile -and ($OutFile.Count -gt 0))
{
    foreach ($f in $OutFile)
    {
        if ($null -eq $f)
        {
            continue
        }

        if ($f.Trim().Length -eq 0)
        {
            continue
        }

        $textLines | Out-File -Encoding utf8 -Width 300 -FilePath $f
    }
}

if ($PassThru)
{
    $plan
}
