<# DirectoryTree.ps1

One big (optionally pruned) tree printer for PowerShell 5.1+.

Features:
- Root context offset:
    -RootContext 3
    -RootContext "foo"
    -RootContext "foo", 99
- -SkipContextLevels collapses intermediate context levels using "..."
- Multi-dot suffix matching (e.g. ".vcxproj.filters")
- Multiple sections with different filters via -Sections
- Per-section OutFile(s) + IncludeInCommon
- Header path on/off: -ShowHeaderPath / -NoHeaderPath

Notes:
- This script always emits the merged output to the pipeline.
- If -OutFile is supplied, it writes the merged output to those files too.

.EXAMPLE
	.\DirectoryTree.ps1 -Root "..\" -Extensions ".h", ".c" -IncludeNoExtension -ListMatchingFolders -IncludeHidden | Out-File -Encoding utf8 -Width 300 ".\tree.txt"
#>

[CmdletBinding()]
param
(
    [Parameter(Mandatory)]
    [string] $Root,

    [object] $Depth = [int]::MaxValue,

    [string[]] $Extensions,
    [string[]] $Files,

    [switch] $IncludeNoExtension,
    [switch] $ExcludeNoExtension,

    [switch] $IncludeHidden,

    [string] $FolderPrefix,
    [string] $FilePrefix,

    [hashtable] $TreeChars,

    [string[]] $OutFile,

    [switch] $ShowHeaderPath,
    [switch] $NoHeaderPath,

    [switch] $ListMatchingFolders,

    [object[]] $RootContext,

    [switch] $SkipContextLevels,

    [hashtable[]] $Sections
)

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

function Resolve-FullPath
(
    [string] $p
)
{
    $item = Get-Item -LiteralPath $p -ErrorAction Stop
    return $item.FullName
}

function Join-RootRelative
(
    [string] $rootDir,
    [string] $rel
)
{
    if ($null -eq $rel)
    {
        return $rootDir
    }

    if ($rel.Trim().Length -eq 0)
    {
        return $rootDir
    }

    $r = $rel.Trim()

    $r = $r.TrimStart('\', '/')
    $r = $r.TrimEnd('\', '/')

    if ($r.Length -eq 0)
    {
        return $rootDir
    }

    return (Join-Path -Path $rootDir -ChildPath $r)
}

function Compute-ContextBase
(
    [string]   $rootDir,
    [object[]] $ctx
)
{
    if (-not $ctx)
    {
        return $rootDir
    }

    if ($ctx.Count -eq 0)
    {
        return $rootDir
    }

    $levels = $null
    $marker = $null
    $maxN   = $null

    foreach ($c in $ctx)
    {
        if ($c -is [int])
        {
            if ($null -eq $levels)
            {
                $levels = [int]$c
            }
            else
            {
                $maxN = [int]$c
            }
        }
        else
        {
            $s = [string]$c

            if ($s.Trim().Length -gt 0)
            {
                $marker = $s.Trim()
            }
        }
    }

    if (($null -ne $levels) -and ($null -eq $marker))
    {
        $cur = $rootDir

        for ($i = 0; $i -lt $levels; $i++)
        {
            $p = Split-Path -Parent $cur

            if (($null -eq $p) -or ($p.Length -eq 0) -or ($p -eq $cur))
            {
                break
            }

            $cur = $p
        }

        return $cur
    }

    if (($null -ne $marker) -and ($null -eq $maxN))
    {
        $maxN = [int]::MaxValue
    }

    if ($null -ne $marker)
    {
        $cur   = $rootDir
        $steps = 0

        while ($steps -le $maxN)
        {
            $leaf = Split-Path -Leaf $cur

            if ($leaf -ieq $marker)
            {
                return $cur
            }

            $p = Split-Path -Parent $cur

            if (($null -eq $p) -or ($p.Length -eq 0) -or ($p -eq $cur))
            {
                break
            }

            $cur = $p
            $steps++
        }

        return $rootDir
    }

    return $rootDir
}

function Get-RelativeSegments
(
    [string] $baseDir,
    [string] $childDir
)
{
    $b = $baseDir.TrimEnd('\')
    $c = $childDir.TrimEnd('\')

    if (-not $c.StartsWith($b, [System.StringComparison]::OrdinalIgnoreCase))
    {
        return @()
    }

    $rest = $c.Substring($b.Length).TrimStart('\')

    if ($rest.Length -eq 0)
    {
        return @()
    }

    return $rest.Split('\') | Where-Object { $_ -and $_.Trim().Length -gt 0 }
}

function New-Node
(
    [string] $name
)
{
    return @{
        Name     = $name
        Children = @{}
        Files    = New-Object System.Collections.Generic.List[string]
    }
}

function Ensure-Child
(
    [hashtable] $node,
    [string]    $name
)
{
    if (-not $node.Children.ContainsKey($name))
    {
        $node.Children[$name] = (New-Node $name)
    }

    return $node.Children[$name]
}

function Add-Files
(
    [hashtable] $node,
    [string[]]  $files
)
{
    foreach ($f in $files)
    {
        if ($null -eq $f)
        {
            continue
        }

        if ($f.Length -eq 0)
        {
            continue
        }

        $node.Files.Add($f)
    }
}

function Merge-Node
(
    [hashtable] $into,
    [hashtable] $from
)
{
    Add-Files -node $into -files ($from.Files | Select-Object -Unique)

    foreach ($k in $from.Children.Keys)
    {
        $childFrom = $from.Children[$k]
        $childInto = Ensure-Child -node $into -name $k
        Merge-Node -into $childInto -from $childFrom
    }
}

function Get-MatchingFilesInDir
(
    [string]   $dirPath,
    [string[]] $suffixesLower,
    [string[]] $namesLower,
    [bool]     $includeNoExt,
    [bool]     $hasFilters,
    [bool]     $includeHidden
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

    $files = @(Get-ChildItem @p)

    if (-not $hasFilters)
    {
        return @($files | Sort-Object Name | ForEach-Object { $_.Name })
    }

    $out = New-Object System.Collections.Generic.List[string]

    foreach ($f in $files)
    {
        $name  = $f.Name
        $lower = $name.ToLowerInvariant()

        $matched = $false

        if ($namesLower -and ($namesLower.Count -gt 0))
        {
            if ($namesLower -contains $lower)
            {
                $matched = $true
            }
        }

        if ((-not $matched) -and $suffixesLower -and ($suffixesLower.Count -gt 0))
        {
            foreach ($sfx in $suffixesLower)
            {
                if ($lower.EndsWith($sfx))
                {
                    $matched = $true
                    break
                }
            }
        }

        if (-not $matched)
        {
            if ($includeNoExt)
            {
                if ([string]::IsNullOrEmpty($f.Extension))
                {
                    $matched = $true
                }
            }
        }

        if ($matched)
        {
            $out.Add($name)
        }
    }

    return @($out | Sort-Object)
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

function Build-PrunedTree
(
    [string]   $baseDir,
    [int]      $depthLimit,
    [string[]] $suffixesLower,
    [string[]] $namesLower,
    [bool]     $includeNoExt,
    [bool]     $includeHidden
)
{
    $hasFilters = $false

    if ($suffixesLower -and ($suffixesLower.Count -gt 0))
    {
        $hasFilters = $true
    }

    if ($namesLower -and ($namesLower.Count -gt 0))
    {
        $hasFilters = $true
    }

    if ($includeNoExt)
    {
        $hasFilters = $true
    }

    function Build-NodeRec
    (
        [string] $dirPath,
        [int]    $level
    )
    {
        $files = Get-MatchingFilesInDir -dirPath $dirPath -suffixesLower $suffixesLower -namesLower $namesLower `
            -includeNoExt $includeNoExt -hasFilters $hasFilters -includeHidden $includeHidden

        $childrenNodes = @()

        if ($level -lt $depthLimit)
        {
            $subs = Get-Subdirs -dirPath $dirPath -includeHidden $includeHidden

            foreach ($d in $subs)
            {
                $child = Build-NodeRec -dirPath $d.FullName -level ($level + 1)

                if ($null -ne $child)
                {
                    $childrenNodes += $child
                }
            }
        }

        if (($files.Count -gt 0) -or ($childrenNodes.Count -gt 0))
        {
            $node = New-Node (Split-Path -Leaf $dirPath)

            Add-Files -node $node -files $files

            foreach ($cn in $childrenNodes)
            {
                $node.Children[$cn.Name] = $cn
            }

            return $node
        }

        return $null
    }

    return Build-NodeRec -dirPath $baseDir -level 0
}

function Make-ContextWrapper
(
    [string]    $contextBase,
    [string]    $rootDir,
    [hashtable] $rootNode,
    [bool]      $skipLevels
)
{
    if ($contextBase -ieq $rootDir)
    {
        return $rootNode
    }

    $segs = Get-RelativeSegments -baseDir $contextBase -childDir $rootDir

    if (-not $segs)
    {
        return $rootNode
    }

    if ($segs.Count -eq 0)
    {
        return $rootNode
    }

    $wrapper = New-Node (Split-Path -Leaf $contextBase)

    if ($skipLevels)
    {
        $dots = Ensure-Child -node $wrapper -name '...'
        $rootLeaf = $segs[$segs.Count - 1]
        $r = Ensure-Child -node $dots -name $rootLeaf
        Merge-Node -into $r -from $rootNode
        return $wrapper
    }

    $cur = $wrapper

    for ($i = 0; $i -lt $segs.Count; $i++)
    {
        $cur = Ensure-Child -node $cur -name $segs[$i]
    }

    Merge-Node -into $cur -from $rootNode
    return $wrapper
}

function Build-FullTree
(
    [string]   $baseDir,
    [int]      $depthLimit,
    [string[]] $suffixesLower,
    [string[]] $namesLower,
    [bool]     $includeNoExt,
    [bool]     $includeHidden
)
{
    $hasFilters = $false

    if ($suffixesLower -and ($suffixesLower.Count -gt 0))
    {
        $hasFilters = $true
    }

    if ($namesLower -and ($namesLower.Count -gt 0))
    {
        $hasFilters = $true
    }

    if ($includeNoExt)
    {
        $hasFilters = $true
    }

    function Build-NodeRec
    (
        [string] $dirPath,
        [int]    $level
    )
    {
        $files = Get-MatchingFilesInDir -dirPath $dirPath -suffixesLower $suffixesLower -namesLower $namesLower `
            -includeNoExt $includeNoExt -hasFilters $hasFilters -includeHidden $includeHidden

        $node = New-Node (Split-Path -Leaf $dirPath)

        Add-Files -node $node -files $files

        if ($level -lt $depthLimit)
        {
            $subs = Get-Subdirs -dirPath $dirPath -includeHidden $includeHidden

            foreach ($d in $subs)
            {
                $child = Build-NodeRec -dirPath $d.FullName -level ($level + 1)
                $node.Children[$child.Name] = $child
            }
        }

        return $node
    }

    return Build-NodeRec -dirPath $baseDir -level 0
}


function Print-TreeLines
(
    [hashtable] $node,
    [hashtable] $chars,
    [string]    $folderPrefix,
    [string]    $filePrefix
)
{
    $lines = New-Object System.Collections.Generic.List[string]

    function Print-Children
    (
        [hashtable] $n,
        [string]    $prefix
    )
    {
        $dirNames  = @($n.Children.Keys | Sort-Object)
        $fileNames = @($n.Files | Sort-Object -Unique)

        $entries = @()

        foreach ($dn in $dirNames)
        {
            $entries += @{ Kind = 'Dir'; Name = $dn }
        }

        foreach ($fn in $fileNames)
        {
            $entries += @{ Kind = 'File'; Name = $fn }
        }

        for ($i = 0; $i -lt $entries.Count; $i++)
        {
            $e    = $entries[$i]
            $last = ($i -eq $entries.Count - 1)

            $branch   = $null
            $nextPref = $null

            if ($last)
            {
                $branch   = $chars.BranchLast
                $nextPref = $prefix + $chars.Spacer
            }
            else
            {
                $branch   = $chars.BranchMid
                $nextPref = $prefix + $chars.Vertical
            }

            if ($e.Kind -eq 'Dir')
            {
                $child = $n.Children[$e.Name]
                $lines.Add($prefix + $branch + $folderPrefix + $child.Name)
                Print-Children -n $child -prefix $nextPref
            }
            else
            {
                $lines.Add($prefix + $branch + $filePrefix + $e.Name)
            }
        }
    }

    Print-Children -n $node -prefix ""
    return $lines.ToArray()
}

# -----------------------------
# Defaults / setup
# -----------------------------

$showHeader = $true

if ($PSBoundParameters.ContainsKey('NoHeaderPath'))
{
    $showHeader = $false
}

if ($PSBoundParameters.ContainsKey('ShowHeaderPath'))
{
    $showHeader = $true
}

if (-not $PSBoundParameters.ContainsKey('FolderPrefix'))
{
    $FolderPrefix = [System.Char]::ConvertFromUtf32(0x1F4C1) + ' '
}

if (-not $PSBoundParameters.ContainsKey('FilePrefix'))
{
    $FilePrefix = [System.Char]::ConvertFromUtf32(0x1F4C4) + ' '
}

$chars = @{
    BranchMid  = ([char]0x251C + [char]0x2500 + [char]0x2500 + ' ')
    BranchLast = ([char]0x2514 + [char]0x2500 + [char]0x2500 + ' ')
    Vertical   = ([char]0x2502 + '   ')
    Spacer     = '    '
}

if ($TreeChars)
{
    foreach ($k in $chars.Keys)
    {
        if ($TreeChars.ContainsKey($k) -and ($null -ne $TreeChars[$k]))
        {
            $chars[$k] = [string]$TreeChars[$k]
        }
    }
}

$rootDir = Resolve-FullPath $Root

$item = Get-Item -LiteralPath $rootDir -ErrorAction Stop

if (-not $item.PSIsContainer)
{
    $rootDir = Split-Path -Parent $rootDir
}

if ($ExcludeNoExtension)
{
    $IncludeNoExtension = $false
}

$contextBase = Compute-ContextBase -rootDir $rootDir -ctx $RootContext

# -----------------------------
# Effective sections
# -----------------------------

$effectiveSections = @()

if ($Sections -and ($Sections.Count -gt 0))
{
    $effectiveSections = $Sections
}
else
{
    $effectiveSections = @(
        @{
            RelativePaths     = @("")
            Extensions        = $Extensions
            Files             = $Files
            IncludeNoExtension = $IncludeNoExtension
            ExcludeNoExtension = $ExcludeNoExtension
            Depth             = $Depth
            IncludeInCommon   = $true
        }
    )
}

# -----------------------------
# Build merged tree
# -----------------------------

$mergedRootNode = New-Node (Split-Path -Leaf $rootDir)

$sectionOutputs = @()

foreach ($sec in $effectiveSections)
{
    $relPaths = $null

    if ($sec.ContainsKey('RelativePaths'))
    {
        $relPaths = $sec.RelativePaths
    }
    elseif ($sec.ContainsKey('RelativePath'))
    {
        $relPaths = $sec.RelativePath
    }

    if ($null -eq $relPaths)
    {
        $relPaths = @("")
    }

    if ($relPaths -isnot [System.Array])
    {
        $relPaths = @($relPaths)
    }

    $secExt = $Extensions

    if ($sec.ContainsKey('Extensions'))
    {
        $secExt = $sec.Extensions
    }

    $secFiles = $Files

    if ($sec.ContainsKey('Files'))
    {
        $secFiles = $sec.Files
    }

    $secIncNoExt = $IncludeNoExtension

    if ($sec.ContainsKey('IncludeNoExtension'))
    {
        $secIncNoExt = [bool]$sec.IncludeNoExtension
    }

    $secExcNoExt = $ExcludeNoExtension

    if ($sec.ContainsKey('ExcludeNoExtension'))
    {
        $secExcNoExt = [bool]$sec.ExcludeNoExtension
    }

    if ($secExcNoExt)
    {
        $secIncNoExt = $false
    }

    $secDepthObj = $Depth

    if ($sec.ContainsKey('Depth'))
    {
        $secDepthObj = $sec.Depth
    }

    $secDepth = Parse-Depth $secDepthObj

    $includeInCommon = $true

    if ($sec.ContainsKey('IncludeInCommon'))
    {
        $includeInCommon = [bool]$sec.IncludeInCommon
    }

    $secOutFiles = $null

    if ($sec.ContainsKey('OutFile'))
    {
        $secOutFiles = $sec.OutFile
    }
    elseif ($sec.ContainsKey('OutFiles'))
    {
        $secOutFiles = $sec.OutFiles
    }

    if ($secOutFiles -and ($secOutFiles -isnot [System.Array]))
    {
        $secOutFiles = @($secOutFiles)
    }

    $suffixesLower = Normalize-Suffixes $secExt
    $namesLower    = Normalize-Names   $secFiles

    foreach ($rp in $relPaths)
    {
        $base = Join-RootRelative -rootDir $rootDir -rel ([string]$rp)

        if (-not (Test-Path -LiteralPath $base))
        {
            continue
        }

        $node = $null

		if ($ListMatchingFolders)
		{
			$node = Build-PrunedTree -baseDir $base -depthLimit $secDepth -suffixesLower $suffixesLower -namesLower $namesLower `
				-includeNoExt $secIncNoExt -includeHidden $IncludeHidden
		}
		else
		{
			$node = Build-FullTree -baseDir $base -depthLimit $secDepth -suffixesLower $suffixesLower -namesLower $namesLower `
				-includeNoExt $secIncNoExt -includeHidden $IncludeHidden
		}

        if ($includeInCommon -and ($null -ne $node))
        {
            $segs = Get-RelativeSegments -baseDir $rootDir -childDir $base

            $attach = $mergedRootNode

            foreach ($s in $segs)
            {
                $attach = Ensure-Child -node $attach -name $s
            }

            Merge-Node -into $attach -from $node
        }

        if ($secOutFiles -and ($secOutFiles.Count -gt 0))
        {
            $secRoot = New-Node (Split-Path -Leaf $rootDir)

            if ($null -ne $node)
            {
                $segs2 = Get-RelativeSegments -baseDir $rootDir -childDir $base
                $attach2 = $secRoot

                foreach ($s2 in $segs2)
                {
                    $attach2 = Ensure-Child -node $attach2 -name $s2
                }

                Merge-Node -into $attach2 -from $node
            }

            $wrappedSec = Make-ContextWrapper -contextBase $contextBase -rootDir $rootDir -rootNode $secRoot -skipLevels $SkipContextLevels

            $secLines = New-Object System.Collections.Generic.List[string]

            if ($showHeader)
            {
                $secLines.Add($contextBase)
            }
            else
            {
                $secLines.Add($FolderPrefix + (Split-Path -Leaf $contextBase))
            }

            $secLines.AddRange([string[]](Print-TreeLines -node $wrappedSec -chars $chars -folderPrefix $FolderPrefix -filePrefix $FilePrefix))

            $sectionOutputs += @{
                OutFiles = $secOutFiles
                Lines    = $secLines
            }
        }
    }
}

# Wrap merged tree with context
$wrapped = Make-ContextWrapper -contextBase $contextBase -rootDir $rootDir -rootNode $mergedRootNode -skipLevels $SkipContextLevels

$mainLines = New-Object System.Collections.Generic.List[string]

if ($showHeader)
{
    $mainLines.Add($contextBase)
}
else
{
    $mainLines.Add($FolderPrefix + (Split-Path -Leaf $contextBase))
}

$mainLines.AddRange([string[]](Print-TreeLines -node $wrapped -chars $chars -folderPrefix $FolderPrefix -filePrefix $FilePrefix))

# Write main output files
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

        $mainLines | Out-File -Encoding utf8 -Width 300 -FilePath $f
    }
}

# Write per-section output files
foreach ($so in $sectionOutputs)
{
    foreach ($f2 in $so.OutFiles)
    {
        if ($null -eq $f2)
        {
            continue
        }

        if ($f2.Trim().Length -eq 0)
        {
            continue
        }

        $so.Lines | Out-File -Encoding utf8 -Width 300 -FilePath $f2
    }
}

# Emit to pipeline
$mainLines
